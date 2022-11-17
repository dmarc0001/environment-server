#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <driver/rmt.h>
#include <led_strip.h>

namespace rest_webserver
{
  class Hardware
  {
  private:
    static const char *tag;
    static const gpio_num_t stripeRemoteTXPort; //! port für Kontrolle
    static const led_strip_type_t ledType;      // welcher LED Sreifenbtyp (LED_STRIP_WS2812)
    static uint32_t ledCount;                   // Anzahl der LED im Streifen
    static led_strip_t strip;                   //! STRIP Beschreibungs struktur
    static rgb_t curr_color;                    //! aktuelle LED Farbe

  public:
    static esp_err_t init();
    static esp_err_t setLed(bool);
    static esp_err_t setLed(uint8_t, uint8_t, uint8_t);
    static esp_err_t setLed(uint32_t);
  };
}

//GPIO18
