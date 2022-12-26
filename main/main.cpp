#include <string.h>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <lwip/err.h>
#include <lwip/sys.h>
#include "main.hpp"
#include "ledStripe.hpp"
#include "webServer.hpp"
#include "tempMeasure.hpp"
#include "filesystemChecker.hpp"
#include "ackuVoltage.hpp"
#include "appPreferences.hpp"
#include "version.hpp"

static const char *TAG{"wifi station"};
static const char *version{MY_APP_VERSION};

void app_main(void)
{
    //
    // tll me the version....
    //
    ESP_LOGI("", "\n\n");
    ESP_LOGI("VERSION", "%s\n\n", version);
    vTaskDelay(pdMS_TO_TICKS(3500));
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    // set my timezone, i deal with timestamps
    setenv("TZ", Prefs::TIMEZONE, 1);
    tzset();
    ESP_LOGI(TAG, "wifi/webserver init...");
    webserver::WebServer::init();
    ESP_LOGI(TAG, "webserver init...OK");
    ESP_LOGI(TAG, "service Tasks start...");
    webserver::TempMeasure::start();
    webserver::LedStripe::start();
    webserver::FsCheckObject::start();
    webserver::AckuVoltage::start();
    ESP_LOGI(TAG, "service Tasks start...OK");
    //
    // i'm boring
    //
    while (true)
    {
        webserver::WebServer::compute();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
