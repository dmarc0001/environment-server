#include <esp_log.h>
#include <lwip/err.h>
#include <lwip/sys.h>

#include "ledStripe.hpp"
#include "statusObject.hpp"
#include "AppPreferences.hpp"

namespace rest_webserver
{
  const char *LedStripe::tag{"hardware"};
  led_strip_t LedStripe::strip{};
  rgb_t LedStripe::curr_color{.r = 0x00, .g = 0x00, .b = 0x00};
  const rgb_t LedStripe::wlan_discon_colr{.r = 0x32, .g = 0x32, .b = 0x00};
  const rgb_t LedStripe::wlan_search_colr{.r = 0x66, .g = 0x41, .b = 0x06};
  const rgb_t LedStripe::wlan_connect_colr{.r = 0x02, .g = 0x0a, .b = 0x02};
  const rgb_t LedStripe::wlan_fail_col{.r = 0xa0, .g = 0x0, .b = 0x0};
  const rgb_t LedStripe::msg_nominal_col{.r = 0x10, .g = 0xc0, .b = 0x10};
  bool LedStripe::is_running{false};
  int64_t nextLedActionTime{0LL};
  uint8_t ledStage{0};
  int64_t nowTime = esp_timer_get_time();

  void LedStripe::ledTask(void *)
  {
    LedStripe::is_running = true;
    // ESP32-S2 RGB LED
    ESP_LOGI(LedStripe::tag, "initialize WS2812 led stripe...");
    //
    led_strip_install();
    //
    // temporary object
    //
    led_strip_t _strip = {
        .type = Prefs::LED_STRIPE_TYPE,
        .is_rgbw = true,
#ifdef LED_STRIP_BRIGHTNESS
        .brightness = 255,
#endif
        .length = Prefs::LED_STRIPE_COUNT,
        .gpio = Prefs::LED_STRIPE_RMT_TX_GPIO,
        .channel = Prefs::LED_STRIPE_RMT_CHANNEL,
        .buf = nullptr,
    };
    //
    // copy to static object
    //
    LedStripe::strip = _strip;
    esp_err_t result = led_strip_init(&LedStripe::strip);
    if (result != ESP_OK)
    {
      ESP_LOGE(LedStripe::tag, "install WS2812 driver failed");
    }
    else
    {
      // Clear LED strip (turn off all LEDs)
      ESP_LOGI(LedStripe::tag, "clear led...");
      led_strip_set_pixel(&LedStripe::strip, 0, LedStripe::curr_color);
      led_strip_flush(&LedStripe::strip);
      //
      ESP_LOGI(LedStripe::tag, "install WS2812 led stripe driver...OK");
    }
    // ########################################################################
    // endless loop
    // ########################################################################
    while (true)
    {
      nowTime = esp_timer_get_time();
      //
      // ist it time for led action
      //
      if (nextLedActionTime < nowTime)
      {
        // it ist time for LED action
        switch (ledStage)
        {
        case 1:
          // WLAN state
          switch (StatusObject::getWlanState())
          {
          case WlanState::DISCONNECTED:
            rest_webserver::LedStripe::setLed(LedStripe::wlan_discon_colr);
            break;
          case WlanState::SEARCHING:
            rest_webserver::LedStripe::setLed(LedStripe::wlan_search_colr);
            break;
          case WlanState::CONNECTED:
            rest_webserver::LedStripe::setLed(LedStripe::wlan_connect_colr);
            break;
          case WlanState::FAILED:
            rest_webserver::LedStripe::setLed(LedStripe::wlan_fail_col);
            break;
          }
          nextLedActionTime = esp_timer_get_time() + 800000LL;
          break;

        case 2:
          // Message state
          rest_webserver::LedStripe::setLed(LedStripe::msg_nominal_col);
          nextLedActionTime = esp_timer_get_time() + 60000LL;
          break;

        case 3:
          rest_webserver::LedStripe::setLed(false);
          nextLedActionTime = esp_timer_get_time() + 450000LL;
          break;

        default:
          rest_webserver::LedStripe::setLed(false);
          ledStage = 0;
          break;
        }
        ++ledStage;
      }
    }
  }

  /**
   * switch LED on or off
   */
  esp_err_t LedStripe::setLed(bool _onoff)
  {
    if (_onoff)
    {
      // set old value
      led_strip_set_pixel(&LedStripe::strip, 0, LedStripe::curr_color);
      return led_strip_flush(&LedStripe::strip);
    }
    else
    {
      rgb_t col = {.r = 0x00, .g = 0x00, .b = 0x00};
      led_strip_set_pixel(&LedStripe::strip, 0, col);
      return led_strip_flush(&LedStripe::strip);
    }
  }

  esp_err_t LedStripe::setLed(rgb_t _rgb)
  {
    led_strip_set_pixel(&LedStripe::strip, 0, _rgb);
    return led_strip_flush(&LedStripe::strip);
  }
  /**
   * LED in RGB each param an color
  */
  esp_err_t LedStripe::setLed(uint8_t r, uint8_t g, uint8_t b)
  {
    LedStripe::curr_color.r = r;
    LedStripe::curr_color.g = g;
    LedStripe::curr_color.b = b;
    led_strip_set_pixel(&LedStripe::strip, 0, LedStripe::curr_color);
    return led_strip_flush(&LedStripe::strip);
  }

  /**
   * RGB as an single param
  */
  esp_err_t LedStripe::setLed(uint32_t _rgb)
  {
    LedStripe::curr_color.r = static_cast<uint8_t>(0xff & (_rgb >> 16));
    LedStripe::curr_color.g = static_cast<uint8_t>(0xff & (_rgb >> 8));
    LedStripe::curr_color.b = static_cast<uint8_t>(0xff & (_rgb));
    led_strip_set_pixel(&LedStripe::strip, 0, LedStripe::curr_color);
    return led_strip_flush(&LedStripe::strip);
  }

  void LedStripe::start()
  {
    if (LedStripe::is_running)
    {
      ESP_LOGE(LedStripe::tag, "led task is running, abort.");
    }
    else
    {
      //xTaskCreate(static_cast<void (*)(void *)>(&(rest_webserver::TempMeasure::measureTask)), "ds18x20", configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL);
      xTaskCreate(LedStripe::ledTask, "led-task", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY, NULL);
    }
  }

}
