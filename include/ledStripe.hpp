#pragma once

#include <FastLED.h>
#include "appPreferences.hpp"

namespace EnvServer
{

  class LEDStripe
  {
    private:
    static const char *tag;
    static CRGB leds[ Prefs::LED_STRIPE_COUNT ];
    static CRGB shadow_leds[ Prefs::LED_STRIPE_COUNT ];

    public:
    static void init();                                        //! init system
    static void setLed( uint8_t, bool );                       //! led(s) on/off
    static void setLed( uint8_t, uint8_t, uint8_t, uint8_t );  //! on or all led set color
    static void setLed( uint8_t, CRGB & );                     //! one or all led set color
    static void setLed( uint8_t, uint32_t );                   //! one or all led set color
  };

}  // namespace EnvServer
