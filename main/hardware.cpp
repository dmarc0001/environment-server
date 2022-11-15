#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "hardware.hpp"
#include "AppPreferences.hpp"

namespace rest_webserver
{
  const char *Hardware::tag{"hardware"};
  bool Hardware::curr_led{false};

  esp_err_t Hardware::init()
  {
    ESP_LOGD(Hardware::tag, "GPIO init...");
    // hardware init...
    gpio_config_t config_out = {.pin_bit_mask = BIT64(Prefs::LED_ATTN),
                                .mode = GPIO_MODE_OUTPUT,
                                .pull_up_en = GPIO_PULLUP_DISABLE,
                                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&config_out);
    // AUS
    ESP_LOGD(Hardware::tag, "LED off...");
    Hardware::sw_led(false);
    return ESP_OK;
  }

  /**
   * switch LED on or off
   */
  void Hardware::sw_led(bool _onoff)
  {
    uint32_t led_state{0};
    // LED on or off
    _onoff ? led_state = Prefs::LED_ON : led_state = Prefs::LED_OFF;
    gpio_set_level(Prefs::LED_ATTN, led_state);
    Hardware::curr_led = _onoff;
  }

  void Hardware::toggle_led()
  {
    uint32_t led_state{0};
    Hardware::curr_led ? led_state = Prefs::LED_OFF : led_state = Prefs::LED_ON;
    gpio_set_level(Prefs::LED_ATTN, led_state);
    Hardware::curr_led = !Hardware::curr_led;
  }

}
