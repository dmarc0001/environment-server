#include "common.hpp"
#include "statics.hpp"
#include "ledStripe.hpp"
#include "appPreferences.hpp"

namespace EnvServer
{
  const char *LEDStripe::tag{ "LEDStripe" };
  CRGB LEDStripe::leds[ Prefs::LED_STRIPE_COUNT ]{};
  CRGB LEDStripe::shadow_leds[ Prefs::LED_STRIPE_COUNT ]{};

  /**
   * init led system
   */
  void LEDStripe::init()
  {
    FastLED.addLeds< WS2811, Prefs::LED_STRIPE_RMT_TX_GPIO, Prefs::LED_RGB_ORDER >( leds, Prefs::LED_STRIPE_COUNT )
        .setCorrection( TypicalLEDStrip );
    FastLED.setBrightness( Prefs::LED_STRIP_BRIGHTNESS );
    // currentPalette = RainbowColors_p;
    // currentBlending = LINEARBLEND;
    FastLED.show();
    elog.log( DEBUG, "%s: initialized...", LEDStripe::tag );
  }

  /**
   * switch LED on or off
   */
  void LEDStripe::setLed( uint8_t led, bool onOff )
  {
    if ( onOff )
    {
      // ON
      if ( led == Prefs::LED_ALL )
      {
        // all LED
        FastLED.show();
      }
      if ( led < Prefs::LED_STRIPE_COUNT )
      {
        // okay, one LED switch ON => restore shadow to current
        leds[ led ] = shadow_leds[ led ];
        FastLED.show();
      }
    }
    else
    {
      // LED OFF
      if ( led == Prefs::LED_ALL )
      {
        // all LED
        CRGB loc_led{};
        FastLED.showColor( loc_led );
      }
      if ( led < Prefs::LED_STRIPE_COUNT )
      {
        // okay one ld switch OFF
        leds[ led ].r = 0;
        leds[ led ].g = 0;
        leds[ led ].b = 0;
        FastLED.show();
      }
    }
  }

  /**
   * LED (alle oder eine) auf einen Wert setzten
   */
  void LEDStripe::setLed( uint8_t led, uint8_t red, uint8_t green, uint8_t blue )
  {
    if ( led == Prefs::LED_ALL )
    {
      // all LED
      for ( uint8_t i = 0; i < Prefs::LED_STRIPE_COUNT; ++i )
      {
        // okay one ld switch OFF
        shadow_leds[ i ].r = leds[ i ].r = red;
        shadow_leds[ i ].g = leds[ i ].g = green;
        shadow_leds[ i ].b = leds[ i ].b = blue;
      }
      FastLED.show();
    }
    if ( led < Prefs::LED_STRIPE_COUNT )
    {
      // okay one ld switch OFF
      shadow_leds[ led ].r = leds[ led ].r = red;
      shadow_leds[ led ].g = leds[ led ].g = green;
      shadow_leds[ led ].b = leds[ led ].b = blue;
      FastLED.show();
    }
  }

  /**
   * eine oder alle LED auf eine Farbe setzten
   */
  void LEDStripe::setLed( uint8_t led, CRGB &rgb )
  {
    if ( led == Prefs::LED_ALL )
    {
      // all LED
      for ( uint8_t i = 0; i < Prefs::LED_STRIPE_COUNT; ++i )
      {
        // okay one ld switch OFF
        shadow_leds[ i ] = leds[ i ] = rgb;
      }
      FastLED.show();
    }
    if ( led < Prefs::LED_STRIPE_COUNT )
    {
      // okay one ld switch OFF
      shadow_leds[ led ] = leds[ led ] = rgb;
      FastLED.show();
    }
  }

  /**
   * eine oder alle LED auf eine RAW Farbe in RGB setzten
   */
  void LEDStripe::setLed( uint8_t led, uint32_t color )
  {
    if ( led == Prefs::LED_ALL )
    {
      // all LED
      for ( uint8_t i = 0; i < Prefs::LED_STRIPE_COUNT; ++i )
      {
        // okay one ld switch OFF
        shadow_leds[ i ] = leds[ i ] = CRGB( color );
      }
      FastLED.show();
    }
    if ( led < Prefs::LED_STRIPE_COUNT )
    {
      // okay one ld switch OFF
      shadow_leds[ led ] = leds[ led ] = CRGB( color );
      FastLED.show();
    }
  }

}  // namespace EnvServer
