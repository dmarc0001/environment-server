#include "common.hpp"
#include "statusObject.hpp"
#include "statics.hpp"
#include "ledStripe.hpp"
#include "appPreferences.hpp"
#include "ledColorsDefinition.hpp"

namespace EnvServer
{
  //
  // timer definitions
  //
  constexpr int64_t WLANlongActionDist = 5000000LL;
  constexpr int64_t WLANshortActionDist = 20000LL;
  constexpr int64_t MeasureActionFlushDist = 40000LL;
  constexpr int64_t MeasureLongActionDist = 10100000LL;
  constexpr int64_t MeasureShortActionDist = 1000000LL;
  constexpr int64_t HTTPActionFlashDist = 35000LL;
  constexpr int64_t HTTPActionDarkDist = 60000LL;

  //
  // color definitions
  //
  CRGB LEDStripe::wlan_discon_colr{ Prefs::LED_COLOR_WLAN_DISCONN };
  CRGB LEDStripe::wlan_search_colr{ Prefs::LED_COLOR_WLAN_SEARCH };
  CRGB LEDStripe::wlan_connect_colr{ Prefs::LED_COLOR_WLAN_SEARCH };
  CRGB LEDStripe::wlan_connect_and_sync_colr{ Prefs::LED_COLOR_WLAN_CONNECT_AND_SYNC };
  CRGB LEDStripe::wlan_fail_col{ Prefs::LED_COLOR_WLAN_FAIL };
  CRGB LEDStripe::measure_unknown_colr{ Prefs::LED_COLOR_MEASURE_UNKNOWN };
  CRGB LEDStripe::measure_action_colr{ Prefs::LED_COLOR_MEASURE_ACTION };
  CRGB LEDStripe::measure_nominal_colr{ Prefs::LED_COLOR_MEASURE_NOMINAL };
  CRGB LEDStripe::measure_warn_colr{ Prefs::LED_COLOR_MEASURE_WARN };
  CRGB LEDStripe::measure_err_colr{ Prefs::LED_COLOR_MEASURE_ERROR };
  CRGB LEDStripe::measure_crit_colr{ Prefs::LED_COLOR_MEASURE_CRIT };
  CRGB LEDStripe::http_active{ Prefs::LED_COLOR_HTTP_ACTIVE };

  const char *LEDStripe::tag{ "LEDStripe" };
  TaskHandle_t LEDStripe::taskHandle{ nullptr };
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
    CRGB color( Prefs::LED_COLOR_MEASURE_UNKNOWN );
    LEDStripe::setLed( Prefs::LED_ALL, false, true );
    // FastLED.show();
    LEDStripe::start();
    elog.log( logger::DEBUG, "%s: initialized...", LEDStripe::tag );
  }

  void LEDStripe::start()
  {
    elog.log( logger::INFO, "%s: LEDStripe Task start...", LEDStripe::tag );

    if ( LEDStripe::taskHandle )
    {
      vTaskDelete( LEDStripe::taskHandle );
      LEDStripe::taskHandle = nullptr;
    }
    else
    {
      xTaskCreate( LEDStripe::ledTask, "led-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY, &LEDStripe::taskHandle );
    }
  }

  void LEDStripe::ledTask( void * )
  {
    using namespace logger;

    elog.log( INFO, "%s: LEDStripe Task starting...", LEDStripe::tag );
    using namespace Prefs;
    int64_t nextWLANLedActionTime{ WLANlongActionDist };
    int64_t nextMeasureLedActionTime{ MeasureActionFlushDist };
    int64_t nextHTTPLedActionTime{ HTTPActionDarkDist };
    int64_t nowTime = esp_timer_get_time();
    bool led_changed{ false };

    while ( true )
    {
      nowTime = esp_timer_get_time();
      //
      // ist it time for led action
      //
      if ( nextWLANLedActionTime < nowTime )
      {
        // make led things
        nextWLANLedActionTime = LEDStripe::wlanStateLoop( &led_changed );
      }
      if ( nextMeasureLedActionTime < nowTime )
      {
        // it ist time for LED action
        nextMeasureLedActionTime = measureStateLoop( &led_changed );
      }
      if ( nextHTTPLedActionTime < nowTime )
      {
        //
        // time for http-indicator-action
        //
        if ( StatusObject::getHttpActive() )
        {
          StatusObject::setHttpActive( false );
          // should set the led to "on"?
          LEDStripe::setLed( LED_HTTP, LEDStripe::http_active, false );
          // next time for activity check
          nextHTTPLedActionTime = esp_timer_get_time() + HTTPActionFlashDist;
          // activity rest
        }
        else
        {
          LEDStripe::setLed( LED_HTTP, false, false );
          // next time to activity check
          nextHTTPLedActionTime = esp_timer_get_time() + 60000LL;
        }
        led_changed = true;
      }
      if ( led_changed )
      {
        // set LED'S
        FastLED.show();
        led_changed = false;
      }
      //
      // what is the smallest time to next event?
      // need for delay
      //
      delay( 5 );
      // uint32_t currDelay;
      // nowTime = esp_timer_get_time();
      // int64_t shortestTime = ( nextHTTPLedActionTime < nextMeasureLedActionTime ) ? nextHTTPLedActionTime :
      // nextMeasureLedActionTime; shortestTime = ( shortestTime < nextWLANLedActionTime ) ? shortestTime : nextWLANLedActionTime;
      // // compute from microsecounds to millisecounds
      // currDelay = static_cast< uint32_t >( floor( ( shortestTime - nowTime ) / 1000 ) );
      // if ( currDelay )
      //   delay( currDelay );
    }
  }

  /**
   * loop for mesure state
   */
  int64_t LEDStripe::measureStateLoop( bool *led_changed )
  {
    using namespace Prefs;
    static uint8_t ledStage{ 0 };  // maybe more then two states
    static bool nominalLedActive{ false };
    uint64_t nextMActionTime = esp_timer_get_time() + MeasureLongActionDist;
    MeasureState state = StatusObject::getMeasureState();
    //
    // if acku low, flash red
    //
    if ( StatusObject::getLowAcku() )
    {
      if ( ledStage % 2 )
      {
        LEDStripe::setLed( LED_MEASURE, measure_crit_colr, false );
        nextMActionTime = esp_timer_get_time() + MeasureActionFlushDist;
      }
      else
      {
        LEDStripe::setLed( LED_MEASURE, false, false );
        nextMActionTime = esp_timer_get_time() + ( MeasureActionFlushDist << 3 );
      }
      if ( !( ledStage % 6 ) )
        nextMActionTime = esp_timer_get_time() + ( MeasureLongActionDist >> 1 );
      // else
      //   nextMActionTime = esp_timer_get_time() + MeasureActionFlushDist;
      ++ledStage;
      *led_changed = true;
      return nextMActionTime;
    }
    //
    // nominal operation, short flashes
    //
    if ( state == MeasureState::MEASURE_NOMINAL )
    {
      // flash rare
      if ( nominalLedActive )
      {
        LEDStripe::setLed( LED_MEASURE, false, false );
      }
      else
      {
        LEDStripe::setLed( LED_MEASURE, LEDStripe::measure_nominal_colr, false );
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
    switch ( ledStage )
    {
      case 0:
        // Measure state flash
        switch ( StatusObject::getMeasureState() )
        {
          case MeasureState::MEASURE_UNKNOWN:
            LEDStripe::setLed( LED_MEASURE, LEDStripe::measure_unknown_colr, false );
            break;
          case MeasureState::MEASURE_ACTION:
            LEDStripe::setLed( LED_MEASURE, LEDStripe::measure_action_colr, false );
            break;
          case MeasureState::MEASURE_NOMINAL:
            break;
          case MeasureState::MEASURE_WARN:
            LEDStripe::setLed( LED_MEASURE, LEDStripe::measure_warn_colr, false );
            break;
          case MeasureState::MEASURE_ERR:
            LEDStripe::setLed( LED_MEASURE, LEDStripe::measure_err_colr, false );
            break;
          default:
            LEDStripe::setLed( LED_MEASURE, LEDStripe::measure_crit_colr, false );
            break;
        }
        ++ledStage;
        nextMActionTime = esp_timer_get_time() + MeasureActionFlushDist;
        break;

      case 1:
      default:
        // dark
        LEDStripe::setLed( LED_MEASURE, false, false );
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
  int64_t LEDStripe::wlanStateLoop( bool *led_changed )
  {
    using namespace Prefs;
    static bool wlanLedSwitch{ true };
    static WlanState cWlanState{ WlanState::FAILED };
    if ( cWlanState != StatusObject::getWlanState() )
    {
      *led_changed = true;
      // mark state
      cWlanState = StatusObject::getWlanState();
      switch ( cWlanState )
      {
        case WlanState::DISCONNECTED:
          LEDStripe::setLed( LED_WLAN, LEDStripe::wlan_discon_colr, false );
          break;
        case WlanState::SEARCHING:
          LEDStripe::setLed( LED_WLAN, LEDStripe::wlan_search_colr, false );
          break;
        case WlanState::CONNECTED:
          LEDStripe::setLed( LED_WLAN, LEDStripe::wlan_connect_colr, false );
          break;
        case WlanState::TIMESYNCED:
          // do here nothing
          break;
        default:
        case WlanState::FAILED:
          LEDStripe::setLed( LED_WLAN, LEDStripe::wlan_fail_col, false );
          break;
      }
    }
    int64_t nextLoopTime = esp_timer_get_time() + WLANlongActionDist;
    // always this:
    // led off when low acku
    if ( cWlanState == WlanState::TIMESYNCED )
    {
      if ( StatusObject::getLowAcku() )
        LEDStripe::setLed( LED_WLAN, false, false );
      else if ( wlanLedSwitch )
      {
        LEDStripe::setLed( LED_WLAN, LEDStripe::wlan_connect_and_sync_colr, false );
        nextLoopTime = esp_timer_get_time() + WLANshortActionDist;
      }
      else
      {
        LEDStripe::setLed( LED_WLAN, false, false );
      }
      *led_changed = true;
    }
    wlanLedSwitch = !wlanLedSwitch;
    return nextLoopTime;
  }

  /**
   * switch LED on or off
   */
  void LEDStripe::setLed( uint8_t led, bool onOff, bool imediate )
  {
    if ( onOff )
    {
      // ON
      if ( led == Prefs::LED_ALL )
      {
        // all LED
        if ( imediate )
          FastLED.show();
      }
      if ( led < Prefs::LED_STRIPE_COUNT )
      {
        // okay, one LED switch ON => restore shadow to current
        leds[ led ] = shadow_leds[ led ];
        if ( imediate )
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
        if ( imediate )
          FastLED.showColor( loc_led );
      }
      if ( led < Prefs::LED_STRIPE_COUNT )
      {
        // okay one ld switch OFF
        leds[ led ].r = 0;
        leds[ led ].g = 0;
        leds[ led ].b = 0;
        if ( imediate )
          FastLED.show();
      }
    }
  }

  /**
   * LED (alle oder eine) auf einen Wert setzten
   */
  void LEDStripe::setLed( uint8_t led, uint8_t red, uint8_t green, uint8_t blue, bool imediate )
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
      if ( imediate )
        FastLED.show();
    }
    if ( led < Prefs::LED_STRIPE_COUNT )
    {
      // okay one ld switch OFF
      shadow_leds[ led ].r = leds[ led ].r = red;
      shadow_leds[ led ].g = leds[ led ].g = green;
      shadow_leds[ led ].b = leds[ led ].b = blue;
      if ( imediate )
        FastLED.show();
    }
  }

  /**
   * eine oder alle LED auf eine Farbe setzten
   */
  void LEDStripe::setLed( uint8_t led, CRGB &rgb, bool imediate )
  {
    if ( led == Prefs::LED_ALL )
    {
      // all LED
      for ( uint8_t i = 0; i < Prefs::LED_STRIPE_COUNT; ++i )
      {
        // okay one ld switch OFF
        shadow_leds[ i ] = leds[ i ] = rgb;
      }
      if ( imediate )
        FastLED.show();
    }
    if ( led < Prefs::LED_STRIPE_COUNT )
    {
      // okay one ld switch OFF
      shadow_leds[ led ] = leds[ led ] = rgb;
      if ( imediate )
        FastLED.show();
    }
  }

  /**
   * eine oder alle LED auf eine RAW Farbe in RGB setzten
   */
  void LEDStripe::setLed( uint8_t led, uint32_t color, bool imediate )
  {
    if ( led == Prefs::LED_ALL )
    {
      // all LED
      for ( uint8_t i = 0; i < Prefs::LED_STRIPE_COUNT; ++i )
      {
        // okay one ld switch OFF
        shadow_leds[ i ] = leds[ i ] = CRGB( color );
      }
      if ( imediate )
        FastLED.show();
    }
    if ( led < Prefs::LED_STRIPE_COUNT )
    {
      // okay one ld switch OFF
      shadow_leds[ led ] = leds[ led ] = CRGB( color );
      if ( imediate )
        FastLED.show();
    }
  }

}  // namespace EnvServer
