#pragma once
#include <string>
#include <stdint.h>
#include <driver/rtc_io.h>
#include <driver/adc.h>
#include <driver/rmt.h>
#include <esp_wifi.h>

namespace Prefs
{
  constexpr const char *MDNS_INSTANCE{"esp rest server"};
  constexpr const char *WEB_PATH{"/www"};
  constexpr gpio_num_t LED_ATTN = GPIO_NUM_2; //! Aktivitätsanzeige
  constexpr uint32_t LED_OFF = 0U;            //! Status für LED OFF
  constexpr uint32_t LED_ON = 1U;             //! Status für LED ON

  constexpr const char *WIFI_SSID{"Controller\0"};
  constexpr const char *WIFI_PASS{"serverSaturnRinge2610\0"};
  constexpr const char *WIFI_HOSTNAME{"esp-rest-server"};
  constexpr uint32_t MAXIMUM_RETRY{5};
  // constexpr wifi_auth_mode_t WIFI_AUTH{ WIFI_AUTH_WPA_WPA2_PSK };
  constexpr wifi_auth_mode_t WIFI_AUTH{WIFI_AUTH_WPA2_PSK};
  constexpr uint32_t SCRATCH_BUFSIZE{10240};

} // namespace Prefs
