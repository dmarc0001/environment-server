#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"

namespace rest_webserver
{
  constexpr uint32_t WIFI_CONNECTED_BIT = 0x00000001;
  constexpr uint32_t WIFI_FAIL_BIT = 0x00000010;

  class WifiThings
  {
  private:
    static int s_retry_num;
    static EventGroupHandle_t s_wifi_event_group; // FreeRTOS event group to signal when we are connected
    static const char *tag;

  public:
    static esp_err_t init();

  private:
    static void init_mdns();
    static void event_handler(void *, esp_event_base_t, int32_t, void *);
  };
}
