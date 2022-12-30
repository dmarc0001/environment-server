#include <esp_log.h>
#include <lwip/err.h>
#include <lwip/sys.h>

#include "ledStripe.hpp"
#include "statusObject.hpp"
#include "appPreferences.hpp"

namespace webserver
{
  constexpr int64_t WLANlongActionDist = 5000000LL;
  constexpr int64_t WLANshortActionDist = 30000LL;
  constexpr int64_t MeasureActionFlushDist = 40000LL;
  constexpr int64_t MeasureLongActionDist = 5100000LL;
  constexpr int64_t MeasureShortActionDist = 1000000LL;
  constexpr int64_t HTTPActionFlashDist = 35000LL;
  constexpr int64_t HTTPActionDarkDist = 60000LL;

  const char *LedStripe::tag{"hardware"};
  led_strip_t LedStripe::strip{};
  rgb_t LedStripe::curr_color[Prefs::LED_STRIPE_COUNT]{};

  const rgb_t LedStripe::wlan_discon_colr{rgb_from_code(0x00323200)};
  const rgb_t LedStripe::wlan_search_colr{rgb_from_code(0x00664106)};
  const rgb_t LedStripe::wlan_connect_colr{rgb_from_code(0x00020200)};
  const rgb_t LedStripe::wlan_connect_and_sync_colr{rgb_from_code(0x00000800)};
  const rgb_t LedStripe::wlan_fail_col{rgb_from_code(0x000a0000)};
  const rgb_t LedStripe::measure_unknown_colr{rgb_from_code(0x00808000)};
  const rgb_t LedStripe::measure_action_colr{rgb_from_code(0x00705000)};
  const rgb_t LedStripe::measure_nominal_colr{LedStripe::wlan_connect_and_sync_colr};
  const rgb_t LedStripe::measure_warn_colr{rgb_from_code(0x00ff4000)};
  const rgb_t LedStripe::measure_err_colr{rgb_from_code(0x00800000)};
  const rgb_t LedStripe::measure_crit_colr{rgb_from_code(0x00ff1010)};
  const rgb_t LedStripe::http_active{rgb_from_code(0x00808000)};
  bool LedStripe::is_running{false};

  void LedStripe::ledTask(void *)
  {
    using namespace Prefs;
    int64_t nextWLANLedActionTime{WLANlongActionDist};
    int64_t nextMeasureLedActionTime{MeasureActionFlushDist};
    int64_t nextHTTPLedActionTime{HTTPActionDarkDist};
    int64_t nowTime = esp_timer_get_time();
    LedStripe::is_running = true;
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
        // make led things
        nextWLANLedActionTime = LedStripe::wlanStateLoop(&led_changed);
      }
      if (nextMeasureLedActionTime < nowTime)
      {
        // it ist time for LED action
        nextMeasureLedActionTime = measureStateLoop(&led_changed);
      }
      if (nextHTTPLedActionTime < nowTime)
      {
        //
        // time for http-indicator-action
        //
        if (StatusObject::getHttpActive())
        {
          StatusObject::setHttpActive(false);
          // should set the led to "on"?
          LedStripe::setLed(LED_HTTP, LedStripe::http_active);
          // next time for activity check
          nextHTTPLedActionTime = esp_timer_get_time() + HTTPActionFlashDist;
          // activity rest
        }
        else
        {
          LedStripe::setLed(LED_HTTP, false);
          // next time to activity check
          nextHTTPLedActionTime = esp_timer_get_time() + 60000LL;
        }
        led_changed = true;
      }
      if (led_changed)
      {
        led_strip_flush(&LedStripe::strip);
        led_changed = false;
      }
    }
  }

  /**
   * loop for mesure state
  */
  int64_t LedStripe::measureStateLoop(bool *led_changed)
  {
    using namespace Prefs;
    static uint8_t ledStage{0}; // maybe more then two states
    static bool nominalLedActive{false};
    uint64_t nextMActionTime = esp_timer_get_time() + MeasureLongActionDist;
    webserver::MeasureState state = StatusObject::getMeasureState();
    //
    // if acku low, flash red
    //
    if (StatusObject::getLowAcku())
    {
      if (ledStage % 2)
      {
        LedStripe::setLed(LED_MEASURE, measure_crit_colr);
        nextMActionTime = esp_timer_get_time() + MeasureActionFlushDist;
      }
      else
      {
        LedStripe::setLed(LED_MEASURE, false);
        nextMActionTime = esp_timer_get_time() + (MeasureActionFlushDist << 3);
      }
      if (!(ledStage % 6))
        nextMActionTime = esp_timer_get_time() + (MeasureLongActionDist >> 1);
      // else
      //   nextMActionTime = esp_timer_get_time() + MeasureActionFlushDist;
      ++ledStage;
      *led_changed = true;
      return nextMActionTime;
    }
    //
    // nominal operation, short flashes
    //
    if (state == MeasureState::MEASURE_NOMINAL)
    {
      // flash rare
      if (nominalLedActive)
      {
        LedStripe::setLed(LED_MEASURE, false);
      }
      else
      {
        LedStripe::setLed(LED_MEASURE, LedStripe::measure_nominal_colr);
        nextMActionTime = esp_timer_get_time() + MeasureActionFlushDist;
      }
      nominalLedActive = !nominalLedActive;
      //
      *led_changed = true;
      return nextMActionTime;
    }
    //
    // not nominal state
    //
    switch (ledStage)
    {
    case 0:
      // Measure state flash
      switch (StatusObject::getMeasureState())
      {
      case MeasureState::MEASURE_UNKNOWN:
        LedStripe::setLed(LED_MEASURE, LedStripe::measure_unknown_colr);
        break;
      case MeasureState::MEASURE_ACTION:
        LedStripe::setLed(LED_MEASURE, LedStripe::measure_action_colr);
        break;
      case MeasureState::MEASURE_NOMINAL:
        break;
      case MeasureState::MEASURE_WARN:
        LedStripe::setLed(LED_MEASURE, LedStripe::measure_warn_colr);
        break;
      case MeasureState::MEASURE_ERR:
        LedStripe::setLed(LED_MEASURE, LedStripe::measure_err_colr);
        break;
      default:
        LedStripe::setLed(LED_MEASURE, LedStripe::measure_crit_colr);
        break;
      }
      ++ledStage;
      nextMActionTime = esp_timer_get_time() + MeasureActionFlushDist;
      break;

    case 1:
    default:
      // dark
      LedStripe::setLed(LED_MEASURE, false);
      nextMActionTime = esp_timer_get_time() + MeasureShortActionDist;
      ledStage = 0;
      break;
    }
    *led_changed = true;
    return nextMActionTime;
  }

  /**
   * loop to compute LED for wlan state
  */
  int64_t LedStripe::wlanStateLoop(bool *led_changed)
  {
    using namespace Prefs;
    static bool wlanLedSwitch{true};
    static WlanState cWlanState{WlanState::FAILED};
    if (cWlanState != StatusObject::getWlanState())
    {
      *led_changed = true;
      // mark state
      cWlanState = StatusObject::getWlanState();
      switch (cWlanState)
      {
      case WlanState::DISCONNECTED:
        LedStripe::setLed(LED_WLAN, LedStripe::wlan_discon_colr);
        break;
      case WlanState::SEARCHING:
        LedStripe::setLed(LED_WLAN, LedStripe::wlan_search_colr);
        break;
      case WlanState::CONNECTED:
        LedStripe::setLed(LED_WLAN, LedStripe::wlan_connect_colr);
        break;
      case WlanState::TIMESYNCED:
        // do here nothing
        break;
      default:
      case WlanState::FAILED:
        LedStripe::setLed(LED_WLAN, LedStripe::wlan_fail_col);
        break;
      }
    }
    int64_t nextLoopTime = esp_timer_get_time() + WLANlongActionDist;
    // always this:
    // led off when low acku
    if (cWlanState == WlanState::TIMESYNCED)
    {
      if (StatusObject::getLowAcku())
        LedStripe::setLed(LED_WLAN, false);
      else if (wlanLedSwitch)
      {
        LedStripe::setLed(LED_WLAN, LedStripe::wlan_connect_and_sync_colr);
        nextLoopTime = esp_timer_get_time() + WLANshortActionDist;
      }
      else
      {
        LedStripe::setLed(LED_WLAN, false);
      }
      *led_changed = true;
    }
    wlanLedSwitch = !wlanLedSwitch;
    return nextLoopTime;
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
