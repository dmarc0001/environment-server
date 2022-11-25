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
    static bool is_running;                           //! is the task running
    static led_strip_t strip;                         //! STRIP describe struct
    static rgb_t curr_color[Prefs::LED_STRIPE_COUNT]; //! currend led color
    static const rgb_t wlan_discon_colr;              //! color if wlan disconnected
    static const rgb_t wlan_search_colr;              //! color if wland connecting
    static const rgb_t wlan_connect_colr;             //! color if wlan connected
    static const rgb_t wlan_connect_and_sync_colr;    //! color if WLAN connected and time synced
    static const rgb_t wlan_fail_col;                 //! color if wlan failed
    static const rgb_t measure_unknown_colr;          //! color if state unknown
    static const rgb_t measure_action_colr;           //! color if state is while measuring
    static const rgb_t measure_nominal_colr;          //! color msg all is nominal
    static const rgb_t measure_warn_colr;             //! color msg warning
    static const rgb_t measure_err_colr;              //! color msg error
    static const rgb_t measure_crit_colr;             //! color msg critical
    static const rgb_t http_active;                   //! indicates httpd activity

  public:
    static void start(); //! start the tsk (singleton)

  private:
    static void ledTask(void *);
    static esp_err_t setLed(uint8_t, bool);
    static esp_err_t setLed(uint8_t, uint8_t, uint8_t, uint8_t);
    static esp_err_t setLed(uint8_t, rgb_t);
    static esp_err_t setLed(uint8_t, uint32_t);
  };
}

//GPIO18
