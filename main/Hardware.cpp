#include <esp_log.h>
#include <lwip/err.h>
#include <lwip/sys.h>

#include "Hardware.hpp"
#include "AppPreferences.hpp"

namespace rest_webserver
{
  const char *Hardware::tag{"hardware"};
  const gpio_num_t Hardware::stripeRemoteTXPort{Prefs::LED_STRIPE_RMT_TX_GPIO};
  const led_strip_type_t Hardware::ledType{Prefs::LED_STRIPE_TYPE};
  uint32_t Hardware::ledCount{Prefs::LED_STRIPE_COUNT};
  led_strip_t Hardware::strip{};
  rgb_t Hardware::curr_color = {.r = 0x00, .g = 0x00, .b = 0x00};

  esp_err_t Hardware::init()
  {
    // ESP32-S2 RGB LED
    ESP_LOGI(tag, "initialize WS2812 led stripe...");
    //
    led_strip_install();

    led_strip_t _strip = {
        .type = Hardware::ledType,
        .is_rgbw = true,
#ifdef LED_STRIP_BRIGHTNESS
        .brightness = 255,
#endif
        .length = Hardware::ledCount,
        .gpio = Hardware::stripeRemoteTXPort,
        .channel = Prefs::LED_STRIPE_RMT_CHANNEL,
        .buf = nullptr,
    };
    Hardware::strip = _strip;
    esp_err_t result = led_strip_init(&Hardware::strip);
    if (result != ESP_OK)
    {
      ESP_LOGE(tag, "install WS2812 driver failed");
    }
    else
    {
      // Clear LED strip (turn off all LEDs)
      ESP_LOGI(tag, "clear led...");
      led_strip_set_pixel(&Hardware::strip, 0, Hardware::curr_color);
      led_strip_flush(&Hardware::strip);
      //
      ESP_LOGI(tag, "install WS2812 led stripe driver...OK");
    }

    // esp32 wrover
    // ESP_LOGD(Hardware::tag, "GPIO init...");
    // // hardware init...
    // gpio_config_t config_out = {.pin_bit_mask = BIT64(Prefs::LED_ATTN),
    //                             .mode = GPIO_MODE_OUTPUT,
    //                             .pull_up_en = GPIO_PULLUP_DISABLE,
    //                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
    //                             .intr_type = GPIO_INTR_DISABLE};
    // gpio_config(&config_out);
    // // AUS
    // ESP_LOGD(Hardware::tag, "LED off...");
    // Hardware::sw_led(false);
    return ESP_OK;
  }

  /**
   * switch LED on or off
   */
  esp_err_t Hardware::setLed(bool _onoff)
  {
    if (_onoff)
    {
      // set old value
      led_strip_set_pixel(&Hardware::strip, 0, Hardware::curr_color);
      return led_strip_flush(&Hardware::strip);
    }
    else
    {
      rgb_t col = {.r = 0x00, .g = 0x00, .b = 0x00};
      led_strip_set_pixel(&Hardware::strip, 0, col);
      return led_strip_flush(&Hardware::strip);
    }
  }

  esp_err_t Hardware::setLed(uint8_t r, uint8_t g, uint8_t b)
  {
    Hardware::curr_color.r = r;
    Hardware::curr_color.g = g;
    Hardware::curr_color.b = b;
    led_strip_set_pixel(&Hardware::strip, 0, Hardware::curr_color);
    return led_strip_flush(&Hardware::strip);
  }

  esp_err_t Hardware::setLed(uint32_t _rgba)
  {
    Hardware::curr_color.r = static_cast<uint8_t>(0xff & (_rgba >> 24));
    Hardware::curr_color.g = static_cast<uint8_t>(0xff & (_rgba >> 16));
    Hardware::curr_color.b = static_cast<uint8_t>(0xff & (_rgba >> 8));
    led_strip_set_pixel(&Hardware::strip, 0, Hardware::curr_color);
    return led_strip_flush(&Hardware::strip);
  }
}
