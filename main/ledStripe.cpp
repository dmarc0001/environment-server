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
  rgb_t LedStripe::curr_color[Prefs::LED_STRIPE_COUNT]{};
  const rgb_t LedStripe::wlan_discon_colr{.r = 0x32, .g = 0x32, .b = 0x00};
  const rgb_t LedStripe::wlan_search_colr{.r = 0x66, .g = 0x41, .b = 0x06};
  const rgb_t LedStripe::wlan_connect_colr{.r = 0x06, .g = 0x08, .b = 0x01};
  const rgb_t LedStripe::wlan_connect_and_sync_colr{.r = 0x01, .g = 0x08, .b = 0x01};
  const rgb_t LedStripe::wlan_fail_col{.r = 0xa0, .g = 0x0, .b = 0x0};
  const rgb_t LedStripe::measure_unknown_colr{.r = 0x80, .g = 0x80, .b = 0x00};
  const rgb_t LedStripe::measure_action_colr{.r = 0x70, .g = 0x50, .b = 0x00};
  const rgb_t LedStripe::measure_nominal_colr{LedStripe::wlan_connect_and_sync_colr};
  const rgb_t LedStripe::measure_warn_colr{.r = 0xff, .g = 0x40, .b = 0x00};
  const rgb_t LedStripe::measure_err_colr{.r = 0x80, .g = 0x00, .b = 0x00};
  const rgb_t LedStripe::measure_crit_colr{.r = 0xff, .g = 0x10, .b = 0x10};
  const rgb_t LedStripe::http_active{.r = 0xa0, .g = 0xa0, .b = 0x00};
  bool LedStripe::is_running{false};

  void LedStripe::ledTask(void *)
  {
    using namespace Prefs;
    int64_t nextWLANLedActionTime{800000LL};
    int64_t nextMeasureLedActionTime{100000LL};
    int64_t nextHTTPLedActionTime{3000000LL};
    uint8_t ledStage{0};
    int64_t nowTime = esp_timer_get_time();
    LedStripe::is_running = true;
    WlanState cWlanState{WlanState::FAILED};
    bool curr_http_state{false};
    bool led_changed{false};
    // ESP32-S2 RGB LED
    ESP_LOGI(LedStripe::tag, "initialize WS2812 led stripe...");
    //
    led_strip_install();
    //
    // temporary object
    //
    led_strip_t _strip = {
        .type = LED_STRIPE_TYPE,
        .is_rgbw = false,
#ifdef LED_STRIP_BRIGHTNESS
        .brightness = 255,
#endif
        .length = LED_STRIPE_COUNT,
        .gpio = LED_STRIPE_RMT_TX_GPIO,
        .channel = LED_STRIPE_RMT_CHANNEL,
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
      for (int idx = 0; idx < LED_STRIPE_COUNT; ++idx)
      {
        rgb_t col = {.r = 0, .g = 0, .b = 0};
        LedStripe::curr_color[idx] = col;
        led_strip_set_pixel(&LedStripe::strip, idx, LedStripe::curr_color[idx]);
      }
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
      if (nextWLANLedActionTime < nowTime)
      {
        if (cWlanState != StatusObject::getWlanState())
        {
          // only if changed
          cWlanState = StatusObject::getWlanState();
          switch (cWlanState)
          {
          case WlanState::DISCONNECTED:
            rest_webserver::LedStripe::setLed(LED_WLAN, LedStripe::wlan_discon_colr);
            break;
          case WlanState::SEARCHING:
            rest_webserver::LedStripe::setLed(LED_WLAN, LedStripe::wlan_search_colr);
            break;
          case WlanState::CONNECTED:
            rest_webserver::LedStripe::setLed(LED_WLAN, LedStripe::wlan_connect_colr);
            break;
          case WlanState::TIMESYNCED:
            rest_webserver::LedStripe::setLed(LED_WLAN, LedStripe::wlan_connect_and_sync_colr);
            break;
          default:
          case WlanState::FAILED:
            rest_webserver::LedStripe::setLed(LED_WLAN, LedStripe::wlan_fail_col);
            break;
          }
          led_changed = true;
        }
        nextWLANLedActionTime = esp_timer_get_time() + 800000LL;
      }
      if (nextMeasureLedActionTime < nowTime)
      {
        // it ist time for LED action
        switch (ledStage)
        {
        case 0:
          // Measure state flash
          nextMeasureLedActionTime = esp_timer_get_time() + 40000LL;
          switch (StatusObject::getMeasureState())
          {
          case MeasureState::MEASURE_UNKNOWN:
            rest_webserver::LedStripe::setLed(LED_MEASURE, LedStripe::measure_unknown_colr);
            break;
          case MeasureState::MEASURE_ACTION:
            rest_webserver::LedStripe::setLed(LED_MEASURE, LedStripe::measure_action_colr);
            break;
          case MeasureState::MEASURE_NOMINAL:
            rest_webserver::LedStripe::setLed(LED_MEASURE, LedStripe::measure_nominal_colr);
            break;
          case MeasureState::MEASURE_WARN:
            rest_webserver::LedStripe::setLed(LED_MEASURE, LedStripe::measure_warn_colr);
            break;
          case MeasureState::MEASURE_ERR:
            rest_webserver::LedStripe::setLed(LED_MEASURE, LedStripe::measure_err_colr);
            break;
          default:
            rest_webserver::LedStripe::setLed(LED_MEASURE, LedStripe::measure_crit_colr);
            break;
          }
          ++ledStage;
          break;

        case 1:
        default:
          if (StatusObject::getMeasureState() != MeasureState::MEASURE_NOMINAL)
          {
            // dark
            rest_webserver::LedStripe::setLed(LED_MEASURE, false);
            nextMeasureLedActionTime = esp_timer_get_time() + 1000000LL;
          }
          else
          {
            // if not dark, shorte sleep
            nextMeasureLedActionTime = esp_timer_get_time() + 500000LL;
          }
          ledStage = 0;
          break;
        }
        led_changed = true;
      }
      if (nextHTTPLedActionTime < nowTime)
      {
        if (StatusObject::getHttpActive() != curr_http_state)
        {
          if (StatusObject::getHttpActive())
          {
            StatusObject::setHttpActive(false);
            rest_webserver::LedStripe::setLed(LED_HTTP, LedStripe::http_active);
            nextHTTPLedActionTime = esp_timer_get_time() + 40000LL;
          }
          else
          {
            rest_webserver::LedStripe::setLed(LED_HTTP, false);
            nextHTTPLedActionTime = esp_timer_get_time() + 20000LL;
          }
        }
        led_changed = true;
      }
      if (led_changed)
        led_strip_flush(&LedStripe::strip);
      led_changed = false;
    }
  }

  /**
   * switch LED on or off
   */
  esp_err_t LedStripe::setLed(uint8_t _led, bool _onoff)
  {
    if (_onoff)
    {
      // set old value
      if (_led == Prefs::LED_ALL)
      {
        for (int idx = 0; idx < Prefs::LED_STRIPE_COUNT; ++idx)
        {
          led_strip_set_pixel(&LedStripe::strip, idx, LedStripe::curr_color[idx]);
        }
      }
      else
      {
        led_strip_set_pixel(&LedStripe::strip, _led, LedStripe::curr_color[_led]);
      }
    }
    else
    {
      // set dark
      rgb_t col = {.r = 0x00, .g = 0x00, .b = 0x00};
      if (_led == Prefs::LED_ALL)
      {
        for (int idx = 0; idx < Prefs::LED_STRIPE_COUNT; ++idx)
        {
          led_strip_set_pixel(&LedStripe::strip, idx, col);
        }
      }
      else
      {
        led_strip_set_pixel(&LedStripe::strip, _led, col);
      }
    }
    return led_strip_flush(&LedStripe::strip);
  }

  /**
   * set led color
  */
  esp_err_t LedStripe::setLed(uint8_t _led, rgb_t _rgb)
  {
    if (_led == Prefs::LED_ALL)
    {
      for (int idx = 0; idx < Prefs::LED_STRIPE_COUNT; ++idx)
      {
        LedStripe::curr_color[idx] = _rgb;
        led_strip_set_pixel(&LedStripe::strip, idx, _rgb);
      }
    }
    else
    {
      LedStripe::curr_color[_led] = _rgb;
      led_strip_set_pixel(&LedStripe::strip, _led, _rgb);
    }
    return led_strip_flush(&LedStripe::strip);
  }

  /**
   * LED in RGB each param an color
  */
  esp_err_t LedStripe::setLed(uint8_t _led, uint8_t r, uint8_t g, uint8_t b)
  {
    rgb_t color{.r = r, .g = g, .b = b};
    return LedStripe::setLed(_led, color);
  }

  /**
   * RGB as an single param
  */
  esp_err_t LedStripe::setLed(uint8_t _led, uint32_t _rgb)
  {
    rgb_t color{
        .r = static_cast<uint8_t>(0xff & (_rgb >> 16)),
        .g = static_cast<uint8_t>(0xff & (_rgb >> 8)),
        .b = static_cast<uint8_t>(0xff & (_rgb))};
    return LedStripe::setLed(_led, color);
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
