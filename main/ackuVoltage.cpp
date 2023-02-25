#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cJSON.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "ackuVoltage.hpp"
#include "statusObject.hpp"
#include "appPreferences.hpp"

namespace webserver
{

  using namespace Prefs;

  const char *AckuVoltage::tag{ "acku_voltage" };
  TaskHandle_t AckuVoltage::taskHandle{ nullptr };

  /**
   * start low prio task for scanning the acku value
   */
  void AckuVoltage::start()
  {
    if ( AckuVoltage::taskHandle )
    {
      vTaskDelete( AckuVoltage::taskHandle );
      AckuVoltage::taskHandle = nullptr;
    }
    else
    {
      // for security, if done do nothing
      StatusObject::init();
      xTaskCreate( AckuVoltage::ackuTask, "acku-measure", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY,
                   &AckuVoltage::taskHandle );
    }
  }

  /**
   * task for scanning the acku value. if no value to measure, deactivate
   */
  void AckuVoltage::ackuTask( void * )
  {
    static uint32_t measures[ CURRENT_SMOOTH_COUNT ];
    adc_cali_handle_t adc_cali_handle = NULL;
    uint32_t idx = 0;
    esp_err_t retVal;

    // fill array with defaults
    for ( auto i = 0; i < CURRENT_SMOOTH_COUNT; i++ )
    {
      measures[ i ] = ACKU_LOWER_VALUE;
    }
    vTaskDelay( pdMS_TO_TICKS( 234 ) );
    //
    // ADC Init
    //
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = { .unit_id = Prefs::ACKU_MEASURE_ADC_UNIT, .ulp_mode = ADC_ULP_MODE_DISABLE };
    retVal = adc_oneshot_new_unit( &init_config, &adc_handle );
    if ( retVal != ESP_OK )
    {
      ESP_LOGE( AckuVoltage::tag, "can't init adc unit, acku mesure task shutdown!" );
      return;
    }
    //
    // ADC Config
    //
    adc_oneshot_chan_cfg_t config = { .atten = Prefs::ACKU_ATTENT, .bitwidth = Prefs::ACKU_ADC_WIDTH };
    retVal = adc_oneshot_config_channel( adc_handle, Prefs::ACKU_MEASURE_CHANNEL, &config );
    if ( retVal != ESP_OK )
    {
      ESP_LOGE( AckuVoltage::tag, "can't config adc channel, acku mesure task shutdown!" );
      return;
    }
    //
    // calibre
    //
    bool cali_enable = AckuVoltage::adcCalibrationInit( &adc_cali_handle );
    //
    // infinity loop
    //
    while ( true )
    {
      vTaskDelay( pdMS_TO_TICKS( 1 ) );
      int m_voltage{ 0U };
      u_int32_t voltage{ 0 };
      int adc_raw{ 0 };
      retVal = adc_oneshot_read( adc_handle, Prefs::ACKU_MEASURE_CHANNEL, &adc_raw );
      if ( cali_enable && ( retVal == ESP_OK ) )
      {
        //
        // the measure is on R-Bridge a 100k, the value is the
        // half of the current value
        //
        retVal = adc_cali_raw_to_voltage( adc_cali_handle, adc_raw, &m_voltage );
        measures[ idx ] = 2 * static_cast< uint32_t >( m_voltage );
        for ( auto i = 0; i < CURRENT_SMOOTH_COUNT; i++ )
        {
          voltage += measures[ i ];
        }
        voltage = static_cast< uint32_t >( floor( voltage / CURRENT_SMOOTH_COUNT ) );
        ESP_LOGD( AckuVoltage::tag, "acku smooth: %d mV (measured %d raw: %d)", static_cast< int >( voltage ),
                  static_cast< int >( measures[ idx ] ), adc_raw );
        StatusObject::setVoltage( voltage );
        ++idx;
        if ( idx >= CURRENT_SMOOTH_COUNT )
        {
          idx = 0U;
        }
      }
      else
      {
        StatusObject::setVoltage( 0 );
      }
      //
      // write a acku log file
      //
      if ( !StatusObject::getIsBrownout() && ( StatusObject::getWlanState() == WlanState::TIMESYNCED ) )
      {
        // write acku value to logfile
        std::string fileName( Prefs::ACKU_LOG_FILE_01 );
        ESP_LOGD( AckuVoltage::tag, "acku value to file <%s>)", fileName.c_str() );
        //
        // if filesystemchecker want to write, prevent this
        // read is walways possible
        //
        if ( xSemaphoreTake( StatusObject::ackuFileSem, pdMS_TO_TICKS( 1000 ) ) == pdTRUE )
        {
          auto aFile = fopen( fileName.c_str(), "a" );
          if ( aFile )
          {
            timeval val;
            gettimeofday( &val, nullptr );
            auto timestamp = val.tv_sec;
            // write value to file with timestamp
            cJSON *dataSetObj = cJSON_CreateObject();
            // make timestamp objekt item
            cJSON_AddItemToObject( dataSetObj, Prefs::JSON_TIMESTAMP_NAME, cJSON_CreateString( std::to_string( timestamp ).c_str() ) );
            cJSON_AddItemToObject( dataSetObj, Prefs::JSON_ACKU_CURRENT_NAME,
                                   cJSON_CreateString( std::to_string( voltage ).c_str() ) );
            char *jsonPrintString = cJSON_PrintUnformatted( dataSetObj );
            fputs( jsonPrintString, aFile );
            fflush( aFile );
            fputs( "\n", aFile );
            fclose( aFile );
            cJSON_Delete( dataSetObj );
            cJSON_free( jsonPrintString );  // !!!!!!! memory leak if not
          }
          xSemaphoreGive( StatusObject::ackuFileSem );
        }
      }
      //
      // sleep for a while, acku needs som time for changing
      //
      vTaskDelay( pdMS_TO_TICKS( 45013 ) );
    }
  }

  /**
   * init calibration
   * https://github.com/espressif/esp-idf/blob/v4.4/examples/peripherals/adc/single_read/single_read/main/single_read.c
   */
  bool AckuVoltage::adcCalibrationInit( adc_cali_handle_t *out_handle )
  {
    using namespace Prefs;
    esp_err_t ret;
    adc_cali_handle_t handle = NULL;
    bool calibrated = false;

    ESP_LOGI( AckuVoltage::tag, "init adc calibration..." );
    vTaskDelay( pdMS_TO_TICKS( 1 ) );
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if ( !calibrated )
    {
      ESP_LOGI( AckuVoltage::tag, "calibration scheme version is %s", "Curve Fitting" );
      adc_cali_curve_fitting_config_t cali_config = {
          .unit_id = ACKU_MEASURE_ADC_UNIT, .atten = ACKU_ATTENT, .bitwidth = ACKU_ADC_WIDTH, .default_vref = 0 };
      ret = adc_cali_create_scheme_curve_fitting( &cali_config, &handle );
      if ( ret == ESP_OK )
      {
        calibrated = true;
      }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if ( !calibrated )
    {
      ESP_LOGI( AckuVoltage::tag, "calibration scheme version is %s", "Line Fitting" );
      adc_cali_line_fitting_config_t cali_config = {
          .unit_id = ACKU_MEASURE_ADC_UNIT, .atten = ACKU_ATTENT, .bitwidth = ACKU_ADC_WIDTH, .default_vref = 0 };
      ret = adc_cali_create_scheme_line_fitting( &cali_config, &handle );
      if ( ret == ESP_OK )
      {
        calibrated = true;
      }
    }
#endif

    *out_handle = handle;
    if ( ret == ESP_OK )
    {
      ESP_LOGI( AckuVoltage::tag, "Calibration Success" );
    }
    else if ( ret == ESP_ERR_NOT_SUPPORTED || !calibrated )
    {
      ESP_LOGW( AckuVoltage::tag, "eFuse not burnt, skip software calibration" );
    }
    else
    {
      ESP_LOGE( AckuVoltage::tag, "Invalid arg or no memory" );
    }

    return calibrated;
    // ret = esp_adc_cal_check_efuse( ADC_CALI_SCHEME );
    // adc1_config_width( ACKU_ADC_WIDTH );
    // adc1_config_channel_atten( ACKU_MEASURE_CHANNEL, ACKU_ATTENT );
    // if ( ret == ESP_ERR_NOT_SUPPORTED )
    // {
    //   ESP_LOGW( AckuVoltage::tag, "Calibration scheme not supported, skip software calibration" );
    // }
    // else if ( ret == ESP_ERR_INVALID_VERSION )
    // {
    //   ESP_LOGW( AckuVoltage::tag, "eFuse not burnt, skip software calibration" );
    // }
    // else if ( ret == ESP_OK )
    // {
    //   cali_enable = true;
    //   esp_adc_cal_characterize( ADC_UNIT_1, ACKU_ATTENT, ACKU_ADC_WIDTH, 0, &AckuVoltage::adc1_chars );
    //   ESP_LOGI( AckuVoltage::tag, "init adc calibration successful..." );
    // }
    // else
    // {
    //   ESP_LOGE( AckuVoltage::tag, "Invalid arg" );
    // }
    // return cali_enable;
  }

  // static void example_adc_calibration_deinit(adc_cali_handle_t handle)
  // {
  // #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  //     ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
  //     ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

  // #elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  //     ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
  //     ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
  // #endif
  // }
}  // namespace webserver
