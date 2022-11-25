#include <string.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <esp_netif.h>
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "main.hpp"
#include "ledStripe.hpp"
#include "webServer.hpp"
#include "tempMeasure.hpp"
#include "AppPreferences.hpp"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
// #define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
// #define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
// #define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
// #define WIFI_CONNECTED_BIT BIT0
// #define WIFI_FAIL_BIT BIT1

static const char *TAG = "wifi station";

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    setenv("TZ", Prefs::TIMEZONE, 1);
    tzset();
    ESP_LOGI(TAG, "wifi/webserver init...");
    webserver::WebServer::init();
    ESP_LOGI(TAG, "webserver init...OK");
    ESP_LOGI(TAG, "service Tasks start...");
    webserver::TempMeasure::start();
    webserver::LedStripe::start();
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
