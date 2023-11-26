#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <FastLED.h>
#include "appPreferences.hpp"

namespace EnvServer
{

  class LEDStripe
  {
    private:
    static const char *tag;
    static CRGB leds[ Prefs::LED_STRIPE_COUNT ];         //! save current color for every led
    static CRGB shadow_leds[ Prefs::LED_STRIPE_COUNT ];  // #! shadow every color for darkness
    static CRGB wlan_discon_colr;                        //! color if wlan disconnected
    static CRGB wlan_search_colr;                        //! color if wland connecting
    static CRGB wlan_connect_colr;                       //! color if wlan connected
    static CRGB wlan_connect_and_sync_colr;              //! color if WLAN connected and time synced
    static CRGB wlan_fail_col;                           //! color if wlan failed
    static CRGB measure_unknown_colr;                    //! color if state unknown
    static CRGB measure_action_colr;                     //! color if state is while measuring
    static CRGB measure_nominal_colr;                    //! color msg all is nominal
    static CRGB measure_warn_colr;                       //! color msg warning
    static CRGB measure_err_colr;                        //! color msg error
    static CRGB measure_crit_colr;                       //! color msg critical
    static CRGB http_active;                             //! indicates httpd activity

    public:
    static TaskHandle_t taskHandle;

    public:
    static void init();                                                     //! init system
    static void setLed( uint8_t, bool, bool = true );                       //! led(s) on/off
    static void setLed( uint8_t, uint8_t, uint8_t, uint8_t, bool = true );  //! on or all led set color
    static void setLed( uint8_t, CRGB &, bool = true );                     //! one or all led set color
    static void setLed( uint8_t, uint32_t, bool = true );                   //! one or all led set color

    private:
    static void start();                        //! start led thread
    static void ledTask( void * );              //! the task fir LED
    static int64_t measureStateLoop( bool * );  //! check measures, return chenged led
    static int64_t wlanStateLoop( bool * );     //! chcek wlan state
  };

}  // namespace EnvServer
