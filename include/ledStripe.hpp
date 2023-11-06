#pragma once
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "appPreferences.hpp"
#include "rgb.hpp"

namespace webserver
{
  typedef struct
  {
    uint32_t resolution; /*!< Encoder resolution, in Hz */
  } led_strip_encoder_config_t;

  typedef struct
  {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
  } rmt_led_strip_encoder_t;

  class LedStripe
  {
    private:
    static const char *tag;
    static rmt_transmit_config_t txConfig;                          //! send config (wo loop)
    static uint8_t ledStripePixels[ Prefs::LED_STRIPE_COUNT * 3 ];  //! aray for LED Pixesl (rgb)
    static rgb_t curr_color[ Prefs::LED_STRIPE_COUNT ];             //! currend led color
    static rmt_channel_handle_t ledChannel;                         //! led channel
    static rmt_encoder_handle_t ledEncoder;                         //! handle to led encoder
    static const rgb_t wlan_discon_colr;                            //! color if wlan disconnected
    static const rgb_t wlan_search_colr;                            //! color if wland connecting
    static const rgb_t wlan_connect_colr;                           //! color if wlan connected
    static const rgb_t wlan_connect_and_sync_colr;                  //! color if WLAN connected and time synced
    static const rgb_t wlan_fail_col;                               //! color if wlan failed
    static const rgb_t measure_unknown_colr;                        //! color if state unknown
    static const rgb_t measure_action_colr;                         //! color if state is while measuring
    static const rgb_t measure_nominal_colr;                        //! color msg all is nominal
    static const rgb_t measure_warn_colr;                           //! color msg warning
    static const rgb_t measure_err_colr;                            //! color msg error
    static const rgb_t measure_crit_colr;                           //! color msg critical
    static const rgb_t http_active;                                 //! indicates httpd activity

    public:
    static void start();  //! start the task (singleton)
    static TaskHandle_t taskHandle;

    private:
    static void ledTask( void * );
    static esp_err_t stripeInit();
    static int64_t wlanStateLoop( bool * );
    static int64_t measureStateLoop( bool * );
    static esp_err_t setLed( uint8_t, bool );
    static esp_err_t setLed( uint8_t, uint8_t, uint8_t, uint8_t );
    static esp_err_t setLed( uint8_t, rgb_t );
    static esp_err_t setLed( uint8_t, uint32_t );
    static esp_err_t rmt_new_led_strip_encoder( const led_strip_encoder_config_t *, rmt_encoder_handle_t * );
    static esp_err_t rmt_led_strip_encoder_reset( rmt_encoder_t * );
    static esp_err_t rmt_del_led_strip_encoder( rmt_encoder_t * );
    static size_t rmt_encode_led_strip( rmt_encoder_t *, rmt_channel_handle_t, const void *, size_t, rmt_encode_state_t * );
    static error_t encoderRetOnError( error_t, rmt_led_strip_encoder_t * );
  };
}  // namespace webserver

// GPIO18
