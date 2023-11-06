#include <esp_log.h>
#include <lwip/err.h>
#include <lwip/sys.h>
#include <esp_timer.h>
#include "ledStripe.hpp"
#include "statusObject.hpp"
#include "appPreferences.hpp"

namespace webserver
{
  constexpr int64_t WLANlongActionDist = 10000000LL;
  constexpr int64_t WLANshortActionDist = 20000LL;
  constexpr int64_t MeasureActionFlushDist = 40000LL;
  constexpr int64_t MeasureLongActionDist = 10100000LL;
  constexpr int64_t MeasureShortActionDist = 1000000LL;
  constexpr int64_t HTTPActionFlashDist = 35000LL;
  constexpr int64_t HTTPActionDarkDist = 60000LL;

  const char *LedStripe::tag{ "hardware" };
  rmt_transmit_config_t LedStripe::txConfig{ 0, 0 };
  uint8_t LedStripe::ledStripePixels[ Prefs::LED_STRIPE_COUNT * 3 ]{ 0 };
  rmt_channel_handle_t LedStripe::ledChannel{ nullptr };
  rmt_encoder_handle_t LedStripe::ledEncoder{ nullptr };
  rgb_t LedStripe::curr_color[ Prefs::LED_STRIPE_COUNT ]{};
  TaskHandle_t LedStripe::taskHandle{ nullptr };

  const rgb_t LedStripe::wlan_discon_colr{ rgb_from_code( 0x00323200 ) };
  const rgb_t LedStripe::wlan_search_colr{ rgb_from_code( 0x00664106 ) };
  const rgb_t LedStripe::wlan_connect_colr{ rgb_from_code( 0x00020200 ) };
  const rgb_t LedStripe::wlan_connect_and_sync_colr{ rgb_from_code( 0x00000200 ) };
  const rgb_t LedStripe::wlan_fail_col{ rgb_from_code( 0x000a0000 ) };
  const rgb_t LedStripe::measure_unknown_colr{ rgb_from_code( 0x00808000 ) };
  const rgb_t LedStripe::measure_action_colr{ rgb_from_code( 0x00705000 ) };
  const rgb_t LedStripe::measure_nominal_colr{ LedStripe::wlan_connect_and_sync_colr };
  const rgb_t LedStripe::measure_warn_colr{ rgb_from_code( 0x00ff4000 ) };
  const rgb_t LedStripe::measure_err_colr{ rgb_from_code( 0x00800000 ) };
  const rgb_t LedStripe::measure_crit_colr{ rgb_from_code( 0x00ff1010 ) };
  const rgb_t LedStripe::http_active{ rgb_from_code( 0x00808000 ) };

  esp_err_t LedStripe::stripeInit()
  {
    // TODO: errorcodes read and respond
    ESP_LOGI( LedStripe::tag, "create RMT TX channel" );
    rmt_tx_channel_config_t txChannelConfig;
    txChannelConfig.gpio_num = Prefs::LED_STRIPE_RMT_TX_GPIO;        // gpio
    txChannelConfig.clk_src = RMT_CLK_SRC_DEFAULT;                   // select source clock
    txChannelConfig.resolution_hz = Prefs::LED_STRIP_RESOLUTION_HZ;  // resolution (led need high)
    txChannelConfig.mem_block_symbols = 64;                          // increase the block size can make the LED less flickering
    txChannelConfig.trans_queue_depth = 4;  // set the number of transactions that can be pending in the background
    ESP_ERROR_CHECK( rmt_new_tx_channel( &txChannelConfig, &LedStripe::ledChannel ) );

    ESP_LOGI( LedStripe::tag, "install led strip encoder" );
    led_strip_encoder_config_t encoder_config = {
        .resolution = Prefs::LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK( LedStripe::rmt_new_led_strip_encoder( &encoder_config, &LedStripe::ledEncoder ) );

    ESP_LOGI( LedStripe::tag, "Enable RMT TX channel" );
    ESP_ERROR_CHECK( rmt_enable( LedStripe::ledChannel ) );
    return ESP_OK;
  }

  void LedStripe::ledTask( void * )
  {
    using namespace Prefs;
    int64_t nextWLANLedActionTime{ WLANlongActionDist };
    int64_t nextMeasureLedActionTime{ MeasureActionFlushDist };
    int64_t nextHTTPLedActionTime{ HTTPActionDarkDist };
    int64_t nowTime = esp_timer_get_time();
    bool led_changed{ false };
    ESP_LOGI( LedStripe::tag, "initialize WS2812 led stripe..." );
    //
    // copy to static object
    //
    esp_err_t result = LedStripe::stripeInit();
    if ( result != ESP_OK )
    {
      ESP_LOGE( LedStripe::tag, "install WS2812 driver failed" );
    }
    else
    {
      // Clear LED strip (turn off all LEDs)
      ESP_LOGI( LedStripe::tag, "clear led..." );
      for ( int idx = 0; idx < LED_STRIPE_COUNT; ++idx )
      {
        int base = idx * 3;
        LedStripe::ledStripePixels[ base + 0 ] = 0;  // green
        LedStripe::ledStripePixels[ base + 1 ] = 0;  // blue
        LedStripe::ledStripePixels[ base + 2 ] = 0;  // red
      }
      // Flush RGB values to LEDs
      ESP_ERROR_CHECK( rmt_transmit( LedStripe::ledChannel, LedStripe::ledEncoder, LedStripe::ledStripePixels,
                                     sizeof( LedStripe::ledStripePixels ), &LedStripe::txConfig ) );
      //
      ESP_LOGI( LedStripe::tag, "install WS2812 led stripe driver...OK" );
    }
    // ########################################################################
    // endless loop
    // ########################################################################
    while ( true )
    {
      nowTime = esp_timer_get_time();
      //
      // ist it time for led action
      //
      if ( nextWLANLedActionTime < nowTime )
      {
        // make led things
        nextWLANLedActionTime = LedStripe::wlanStateLoop( &led_changed );
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
          LedStripe::setLed( LED_HTTP, LedStripe::http_active );
          // next time for activity check
          nextHTTPLedActionTime = esp_timer_get_time() + HTTPActionFlashDist;
          // activity rest
        }
        else
        {
          LedStripe::setLed( LED_HTTP, false );
          // next time to activity check
          nextHTTPLedActionTime = esp_timer_get_time() + 60000LL;
        }
        led_changed = true;
      }
      if ( led_changed )
      {
        rmt_transmit( LedStripe::ledChannel, LedStripe::ledEncoder, LedStripe::ledStripePixels, sizeof( LedStripe::ledStripePixels ),
                      &LedStripe::txConfig );
        led_changed = false;
      }
    }
  }

  /**
   * loop for mesure state
   */
  int64_t LedStripe::measureStateLoop( bool *led_changed )
  {
    using namespace Prefs;
    static uint8_t ledStage{ 0 };  // maybe more then two states
    static bool nominalLedActive{ false };
    uint64_t nextMActionTime = esp_timer_get_time() + MeasureLongActionDist;
    webserver::MeasureState state = StatusObject::getMeasureState();
    //
    // if acku low, flash red
    //
    if ( StatusObject::getLowAcku() )
    {
      if ( ledStage % 2 )
      {
        LedStripe::setLed( LED_MEASURE, measure_crit_colr );
        nextMActionTime = esp_timer_get_time() + MeasureActionFlushDist;
      }
      else
      {
        LedStripe::setLed( LED_MEASURE, false );
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
        LedStripe::setLed( LED_MEASURE, false );
      }
      else
      {
        LedStripe::setLed( LED_MEASURE, LedStripe::measure_nominal_colr );
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
            LedStripe::setLed( LED_MEASURE, LedStripe::measure_unknown_colr );
            break;
          case MeasureState::MEASURE_ACTION:
            LedStripe::setLed( LED_MEASURE, LedStripe::measure_action_colr );
            break;
          case MeasureState::MEASURE_NOMINAL:
            break;
          case MeasureState::MEASURE_WARN:
            LedStripe::setLed( LED_MEASURE, LedStripe::measure_warn_colr );
            break;
          case MeasureState::MEASURE_ERR:
            LedStripe::setLed( LED_MEASURE, LedStripe::measure_err_colr );
            break;
          default:
            LedStripe::setLed( LED_MEASURE, LedStripe::measure_crit_colr );
            break;
        }
        ++ledStage;
        nextMActionTime = esp_timer_get_time() + MeasureActionFlushDist;
        break;

      case 1:
      default:
        // dark
        LedStripe::setLed( LED_MEASURE, false );
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
  int64_t LedStripe::wlanStateLoop( bool *led_changed )
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
          LedStripe::setLed( LED_WLAN, LedStripe::wlan_discon_colr );
          break;
        case WlanState::SEARCHING:
          LedStripe::setLed( LED_WLAN, LedStripe::wlan_search_colr );
          break;
        case WlanState::CONNECTED:
          LedStripe::setLed( LED_WLAN, LedStripe::wlan_connect_colr );
          break;
        case WlanState::TIMESYNCED:
          // do here nothing
          break;
        default:
        case WlanState::FAILED:
          LedStripe::setLed( LED_WLAN, LedStripe::wlan_fail_col );
          break;
      }
    }
    int64_t nextLoopTime = esp_timer_get_time() + WLANlongActionDist;
    // always this:
    // led off when low acku
    if ( cWlanState == WlanState::TIMESYNCED )
    {
      if ( StatusObject::getLowAcku() )
        LedStripe::setLed( LED_WLAN, false );
      else if ( wlanLedSwitch )
      {
        LedStripe::setLed( LED_WLAN, LedStripe::wlan_connect_and_sync_colr );
        nextLoopTime = esp_timer_get_time() + WLANshortActionDist;
      }
      else
      {
        LedStripe::setLed( LED_WLAN, false );
      }
      *led_changed = true;
    }
    wlanLedSwitch = !wlanLedSwitch;
    return nextLoopTime;
  }

  /**
   * switch LED on or off
   */
  esp_err_t LedStripe::setLed( uint8_t _led, bool _onoff )
  {
    if ( _onoff )
    {
      // set old value
      if ( _led == Prefs::LED_ALL )
      {
        for ( int idx = 0; idx < Prefs::LED_STRIPE_COUNT; ++idx )
        {
          int base = idx * 3;
          LedStripe::ledStripePixels[ base + 0 ] = LedStripe::curr_color[ idx ].g;  // green
          LedStripe::ledStripePixels[ base + 1 ] = LedStripe::curr_color[ idx ].b;  // blue
          LedStripe::ledStripePixels[ base + 2 ] = LedStripe::curr_color[ idx ].r;  // red
        }
      }
      else if ( _led < Prefs::LED_STRIPE_COUNT )
      {
        int base = _led * 3;
        LedStripe::ledStripePixels[ base + 0 ] = LedStripe::curr_color[ _led ].g;  // green
        LedStripe::ledStripePixels[ base + 1 ] = LedStripe::curr_color[ _led ].b;  // blue
        LedStripe::ledStripePixels[ base + 2 ] = LedStripe::curr_color[ _led ].r;  // red
      }
    }
    else
    {
      // set dark
      if ( _led == Prefs::LED_ALL )
      {
        for ( int idx = 0; idx < Prefs::LED_STRIPE_COUNT; ++idx )
        {
          int base = idx * 3;
          LedStripe::ledStripePixels[ base + 0 ] = 0;  // green
          LedStripe::ledStripePixels[ base + 1 ] = 0;  // blue
          LedStripe::ledStripePixels[ base + 2 ] = 0;  // red
        }
      }
      else if ( _led < Prefs::LED_STRIPE_COUNT )
      {
        int base = _led * 3;
        LedStripe::ledStripePixels[ base + 0 ] = 0;  // green
        LedStripe::ledStripePixels[ base + 1 ] = 0;  // blue
        LedStripe::ledStripePixels[ base + 2 ] = 0;  // red
      }
    }
    return rmt_transmit( LedStripe::ledChannel, LedStripe::ledEncoder, LedStripe::ledStripePixels,
                         sizeof( LedStripe::ledStripePixels ), &LedStripe::txConfig );
  }

  /**
   * set led color
   */
  esp_err_t LedStripe::setLed( uint8_t _led, rgb_t _rgb )
  {
    if ( _led == Prefs::LED_ALL )
    {
      for ( int idx = 0; idx < Prefs::LED_STRIPE_COUNT; ++idx )
      {
        LedStripe::curr_color[ idx ] = _rgb;
        int base = idx * 3;
        LedStripe::ledStripePixels[ base + 0 ] = LedStripe::curr_color[ idx ].g;  // green
        LedStripe::ledStripePixels[ base + 1 ] = LedStripe::curr_color[ idx ].b;  // blue
        LedStripe::ledStripePixels[ base + 2 ] = LedStripe::curr_color[ idx ].r;  // red
      }
    }
    else if ( _led < Prefs::LED_STRIPE_COUNT )
    {
      LedStripe::curr_color[ _led ] = _rgb;
      int base = _led * 3;
      LedStripe::ledStripePixels[ base + 0 ] = LedStripe::curr_color[ _led ].g;  // green
      LedStripe::ledStripePixels[ base + 1 ] = LedStripe::curr_color[ _led ].b;  // blue
      LedStripe::ledStripePixels[ base + 2 ] = LedStripe::curr_color[ _led ].r;  // red
    }
    return rmt_transmit( LedStripe::ledChannel, LedStripe::ledEncoder, LedStripe::ledStripePixels,
                         sizeof( LedStripe::ledStripePixels ), &LedStripe::txConfig );
  }

  /**
   * LED in RGB each param an color
   */
  esp_err_t LedStripe::setLed( uint8_t _led, uint8_t r, uint8_t g, uint8_t b )
  {
    rgb_t color{ .r = r, .g = g, .b = b };
    return LedStripe::setLed( _led, color );
  }

  /**
   * RGB as an single param
   */
  esp_err_t LedStripe::setLed( uint8_t _led, uint32_t _rgb )
  {
    rgb_t color{ .r = static_cast< uint8_t >( 0xff & ( _rgb >> 16 ) ),
                 .g = static_cast< uint8_t >( 0xff & ( _rgb >> 8 ) ),
                 .b = static_cast< uint8_t >( 0xff & ( _rgb ) ) };
    return LedStripe::setLed( _led, color );
  }

  /**
   * encode strip
   */
  size_t LedStripe::rmt_encode_led_strip( rmt_encoder_t *encoder,
                                          rmt_channel_handle_t channel,
                                          const void *primary_data,
                                          size_t data_size,
                                          rmt_encode_state_t *ret_state )
  {
    rmt_led_strip_encoder_t *led_encoder = __containerof( encoder, rmt_led_strip_encoder_t, base );
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_COMPLETE;
    rmt_encode_state_t state = RMT_ENCODING_COMPLETE;
    size_t encoded_symbols = 0;
    switch ( led_encoder->state )
    {
      case 0:  // send RGB data
        encoded_symbols += bytes_encoder->encode( bytes_encoder, channel, primary_data, data_size, &session_state );
        if ( session_state & RMT_ENCODING_COMPLETE )
        {
          led_encoder->state = 1;  // switch to next state when current encoding session finished
        }
        if ( session_state & RMT_ENCODING_MEM_FULL )
        {
          state = RMT_ENCODING_MEM_FULL;  // TODO: original was state |= RMT_ENCODING_MEM_FULL
          *ret_state = state;
          return encoded_symbols;
        }
      // fall-through
      case 1:  // send reset code
        encoded_symbols +=
            copy_encoder->encode( copy_encoder, channel, &led_encoder->reset_code, sizeof( led_encoder->reset_code ), &session_state );
        if ( session_state & RMT_ENCODING_COMPLETE )
        {
          led_encoder->state = 0;         // back to the initial encoding session
          state = RMT_ENCODING_COMPLETE;  // TODO: state |= RMT_ENCODING_COMPLETE;
        }
        if ( session_state & RMT_ENCODING_MEM_FULL )
        {
          state = RMT_ENCODING_MEM_FULL;  // TODO: was state |= RMT_ENCODING_MEM_FULL;
          *ret_state = state;
          return encoded_symbols;
        }
    }
    return encoded_symbols;
  }

  /**
   * delete the encoder
   */
  esp_err_t LedStripe::rmt_del_led_strip_encoder( rmt_encoder_t *encoder )
  {
    rmt_led_strip_encoder_t *led_encoder = __containerof( encoder, rmt_led_strip_encoder_t, base );
    rmt_del_encoder( led_encoder->bytes_encoder );
    rmt_del_encoder( led_encoder->copy_encoder );
    free( led_encoder );
    return ESP_OK;
  }

  /**
   * reset ENDODER
   */
  esp_err_t LedStripe::rmt_led_strip_encoder_reset( rmt_encoder_t *encoder )
  {
    rmt_led_strip_encoder_t *led_encoder = __containerof( encoder, rmt_led_strip_encoder_t, base );
    rmt_encoder_reset( led_encoder->bytes_encoder );
    rmt_encoder_reset( led_encoder->copy_encoder );
    led_encoder->state = 0;
    return ESP_OK;
  }

  /**
   * ENCODER
   */
  esp_err_t LedStripe::rmt_new_led_strip_encoder( const led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder )
  {
    esp_err_t ret = ESP_OK;
    rmt_led_strip_encoder_t *led_encoder = nullptr;
    if ( !( config && ret_encoder ) )
    {
      ESP_LOGE( LedStripe::tag, "led_encoder - invalid argument" );
      return LedStripe::encoderRetOnError( ESP_ERR_INVALID_ARG, led_encoder );
    }
    // ESP_GOTO_ON_FALSE( config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument" );
    led_encoder = static_cast< rmt_led_strip_encoder_t * >( calloc( 1, sizeof( rmt_led_strip_encoder_t ) ) );
    if ( !led_encoder )
    {
      ESP_LOGE( LedStripe::tag, "no mem for led stripe encoder" );
      return LedStripe::encoderRetOnError( ESP_ERR_NO_MEM, led_encoder );
    }
    // ESP_GOTO_ON_FALSE( led_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for led strip encoder" );
    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;
    // different led strip might have its own timing requirements, following parameter is for WS2812
    rmt_bytes_encoder_config_t bytes_encoder_config;
    bytes_encoder_config.bit0.level0 = 1;
    bytes_encoder_config.bit0.duration0 = 0.3 * config->resolution / 1000000;  // T0H=0.3us
    bytes_encoder_config.bit0.level1 = 0;
    bytes_encoder_config.bit0.duration1 = 0.9 * config->resolution / 1000000;  // T0L=0.9us
    //
    bytes_encoder_config.bit1.level0 = 1;
    bytes_encoder_config.bit1.duration0 = 0.9 * config->resolution / 1000000;  // T1H=0.9us
    bytes_encoder_config.bit1.level1 = 0;
    bytes_encoder_config.bit1.duration1 = 0.3 * config->resolution / 1000000;  // T1L=0.3us
                                                                               //
    bytes_encoder_config.flags.msb_first = 1;                                  // WS2812 transfer bit order: G7...G0R7...R0B7...B0
    ret = rmt_new_bytes_encoder( &bytes_encoder_config, &led_encoder->bytes_encoder );
    if ( ret != ESP_OK )
    {
      ESP_LOGE( LedStripe::tag, "create bytes encoder failed" );
      return LedStripe::encoderRetOnError( ret, led_encoder );
    }
    // ESP_GOTO_ON_ERROR( rmt_new_bytes_encoder( &bytes_encoder_config, &led_encoder->bytes_encoder ), err, TAG,
    //                    "create bytes encoder failed" );
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ret = rmt_new_copy_encoder( &copy_encoder_config, &led_encoder->copy_encoder );
    if ( ret != ESP_OK )
    {
      ESP_LOGE( LedStripe::tag, "create copy encoder failed" );
      return LedStripe::encoderRetOnError( ret, led_encoder );
    }
    // ESP_GOTO_ON_ERROR( rmt_new_copy_encoder( &copy_encoder_config, &led_encoder->copy_encoder ), err, TAG,
    //                    "create copy encoder failed" );
    uint32_t reset_ticks = config->resolution / 1000000 * 50 / 2;  // reset code duration defaults to 50us
    rmt_symbol_word_t sw;
    sw.level0 = 0;
    sw.duration0 = reset_ticks;
    sw.level1 = 0;
    sw.duration1 = reset_ticks;
    led_encoder->reset_code = sw;
    *ret_encoder = &led_encoder->base;
    return ESP_OK;
  }

  /**
   * procedure if an error occurs
   */
  error_t LedStripe::encoderRetOnError( error_t ret, rmt_led_strip_encoder_t *led_encoder )
  {
    if ( led_encoder )
    {
      if ( led_encoder->bytes_encoder )
      {
        rmt_del_encoder( led_encoder->bytes_encoder );
      }
      if ( led_encoder->copy_encoder )
      {
        rmt_del_encoder( led_encoder->copy_encoder );
      }
      free( led_encoder );
    }
    return ret;
  }

  void LedStripe::start()
  {
    if ( LedStripe::taskHandle )
    {
      vTaskDelete( LedStripe::taskHandle );
      LedStripe::taskHandle = nullptr;
    }
    else
    {
      // xTaskCreate(static_cast<void (*)(void *)>(&(rest_webserver::TempMeasure::measureTask)), "ds18x20", configMINIMAL_STACK_SIZE *
      // 4, NULL, 5, NULL);
      xTaskCreate( LedStripe::ledTask, "led-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY, &LedStripe::taskHandle );
    }
  }
}  // namespace webserver
