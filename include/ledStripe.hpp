#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <driver/rmt.h>

#include "AppPreferences.hpp"

namespace rest_webserver
{
  class LedStripe
  {
  private:
    static const char *tag;
    static bool is_running;               //! is the task running
    static led_strip_t strip;             //! STRIP describe struct
    static rgb_t curr_color;              //! currend led color
    static const rgb_t wlan_discon_colr;  //! color if wlan disconnected
    static const rgb_t wlan_search_colr;  //! color if wland connecting
    static const rgb_t wlan_connect_colr; //! color if wlan connected
    static const rgb_t wlan_fail_col;     //! color if wlan failed
    static const rgb_t msg_nominal_col;   // color msg all is nominal

  public:
    static void start(); //! start the tsk (singleton)

  private:
    static void ledTask(void *);
    static esp_err_t setLed(bool);
    static esp_err_t setLed(uint8_t, uint8_t, uint8_t);
    static esp_err_t setLed(rgb_t);
    static esp_err_t setLed(uint32_t);
  };
}

//GPIO18
