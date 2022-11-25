#include <string.h>
#include <cstring>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <mdns.h>

#include "AppPreferences.hpp"
#include "wifi.hpp"
#include "statusObject.hpp"

namespace rest_webserver
{
    //
    // global static things instances
    //
    int WifiThings::s_retry_num{0};
    EventGroupHandle_t WifiThings::s_wifi_event_group{};
    const char *WifiThings::tag{"wifi_things"};

    esp_err_t WifiThings::init()
    {
        ESP_LOGI(WifiThings::tag, "init wifi...");
        // init structures
        StatusObject::setWlanState(WlanState::DISCONNECTED);
        WifiThings::s_wifi_event_group = xEventGroupCreate();
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        // register handler for wifi events
        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &event_handler,
                                                            NULL,
                                                            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &event_handler,
                                                            NULL,
                                                            &instance_got_ip));
        //
        // create wifi config
        //
        wifi_config_t wifi_config = {
            .sta{
                .ssid = {},
                .password = {},
                .scan_method = WIFI_FAST_SCAN,
                .bssid_set = false,
                .bssid = {},
                .channel = 0,
                .listen_interval = 0,
                .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                .threshold = {.rssi = 0,
                              .authmode = WIFI_AUTH_WPA2_PSK},
                .pmf_cfg = {.capable = true,
                            .required = false},
                .rm_enabled = 0,
                .btm_enabled = 0,
                .mbo_enabled = 0, // For IDF 4.4 and higher
                .reserved = {},
                .sae_pwe_h2e = {} // For IDF 4.4 and higher
            }};
        //
        // add passwd and ssi
        //
        std::memcpy(&wifi_config.sta.ssid, Prefs::WIFI_SSID, strlen(Prefs::WIFI_SSID));
        std::memcpy(&wifi_config.sta.password, Prefs::WIFI_PASS, strlen(Prefs::WIFI_PASS));
        //
        // set wifi to my desired mode and start wifi
        //
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
        //
        ESP_LOGI(WifiThings::tag, "wifi_init_sta finished.");

        // Waiting until either the connection is established
        // (WIFI_CONNECTED_BIT) or connection failed for the maximum
        // number of re-tries (WIFI_FAIL_BIT).
        // The bits are set by event_handler() (see above)
        //
        EventBits_t bits = xEventGroupWaitBits(WifiThings::s_wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               portMAX_DELAY);
        //
        // xEventGroupWaitBits() returns the bits before the call returned,
        // hence we can test which event actually happened.
        //
        if (bits & WIFI_CONNECTED_BIT)
        {
            ESP_LOGI(WifiThings::tag, "connected to ap SSID:%s password:%s",
                     Prefs::WIFI_SSID, Prefs::WIFI_PASS);
            ESP_LOGI(WifiThings::tag, "init mdns...");
            WifiThings::init_mdns();
            return ESP_OK;
        }
        else if (bits & WIFI_FAIL_BIT)
        {
            ESP_LOGI(WifiThings::tag, "Failed to connect to SSID:%s, password:%s",
                     Prefs::WIFI_SSID, Prefs::WIFI_PASS);
            return ESP_FAIL;
        }
        else
        {
            ESP_LOGE(WifiThings::tag, "UNEXPECTED EVENT");
            return ESP_FAIL;
        }
    }

    /**
   * init multicast dns (for small networks like my fritz! )
   */
    void WifiThings::init_mdns(void)
    {
        mdns_init();
        mdns_hostname_set(Prefs::WIFI_HOSTNAME);
        mdns_instance_name_set(Prefs::MDNS_INSTANCE);
        mdns_txt_item_t serviceTxtData[] = {{"board", "esp32"}, {"path", "/"}};
        ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                         sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
    }

    /**
     * Wifi Events....
    */
    void WifiThings::event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
    {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
        {
            StatusObject::setWlanState(WlanState::SEARCHING);
            esp_wifi_connect();
        }
        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            if (WifiThings::s_retry_num < Prefs::WIFI_MAXIMUM_RETRY)
            {
                StatusObject::setWlanState(WlanState::SEARCHING);
                esp_wifi_connect();
                WifiThings::s_retry_num++;
                ESP_LOGI(WifiThings::tag, "retry to connect to the AP");
            }
            else
            {
                xEventGroupSetBits(WifiThings::s_wifi_event_group, WIFI_FAIL_BIT);
                StatusObject::setWlanState(WlanState::FAILED);
            }
            ESP_LOGI(WifiThings::tag, "connect to the AP fail");
        }
        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(WifiThings::tag, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            StatusObject::setWlanState(WlanState::CONNECTED);
            WifiThings::s_retry_num = 0;
            xEventGroupSetBits(WifiThings::s_wifi_event_group, WIFI_CONNECTED_BIT);
            sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
            sntp_setoperatingmode(SNTP_OPMODE_POLL);
            sntp_setservername(1, "pool.ntp.org");
            sntp_set_time_sync_notification_cb(WifiThings::time_sync_notification_cb);
            sntp_init();
        }
    }

    void WifiThings::time_sync_notification_cb(struct timeval *tv)
    {
        using namespace Prefs;
        sntp_sync_status_t state = sntp_get_sync_status();
        switch (state)
        {
        case SNTP_SYNC_STATUS_COMPLETED:
            ESP_LOGI(WifiThings::tag, "Notification: time status sync competed!");
            if (StatusObject::getWlanState() == WlanState::CONNECTED)
            {
                StatusObject::setWlanState(WlanState::TIMESYNCED);
            }
            break;
        default:
            ESP_LOGI(WifiThings::tag, "Notification: time status not synced!");
            if (StatusObject::getWlanState() == WlanState::TIMESYNCED)
            {
                StatusObject::setWlanState(WlanState::CONNECTED);
            }
        }
    }

}
