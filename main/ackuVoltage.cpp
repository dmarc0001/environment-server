#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cJSON.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_adc/adc_continuous.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

#include "ackuVoltage.hpp"
#include "statusObject.hpp"
#include "appPreferences.hpp"

namespace webserver
{

  using namespace Prefs;

  const char *AckuVoltage::tag{ "acku_voltage" };
  bool AckuVoltage::is_running{ false };
  esp_adc_cal_characteristics_t AckuVoltage::adc1_chars;

  /**
   * start low prio task for scanning the acku value
   */
  void AckuVoltage::start()
  {
    if ( AckuVoltage::is_running )
    {
      ESP_LOGE( AckuVoltage::tag, "acku measure task is running, abort." );
    }
    else
    {
      // for security, if done do nothing
      StatusObject::init();
      xTaskCreate( AckuVoltage::ackuTask, "acku-measure", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY, NULL );
    }
  }

  /**
   * task for scanning the acku value. if no value to measure, deactivate
   */
  void AckuVoltage::ackuTask( void * )
  {
    AckuVoltage::is_running = true;
    static uint32_t measures[ CURRENT_SMOOTH_COUNT ];
    bool cali_enable = AckuVoltage::adcCalibrationInit();
    uint32_t idx = 0;

    // fill array with defaults
    for ( auto i = 0; i < CURRENT_SMOOTH_COUNT; i++ )
    {
      measures[ i ] = ACKU_LOWER_VALUE;
    }
    vTaskDelay( pdMS_TO_TICKS( 234 ) );
    // infinity loop
    while ( true )
    {
      // obsolete : adc_power_acquire();
      vTaskDelay( pdMS_TO_TICKS( 1 ) );
      uint32_t voltage{ 0U };
      int adc_raw = adc1_get_raw( ACKU_MEASURE_CHANNEL );
      // ESP_LOGD(AckuVoltage::tag, "raw  data: %d", adc_raw);
      if ( cali_enable )
      {
        //
        // the measure is on R-Bridge a 100k, the value is the
        // half of the current value
        //
        measures[ idx ] = 2 * esp_adc_cal_raw_to_voltage( adc_raw, &AckuVoltage::adc1_chars );
        for ( auto i = 0; i < CURRENT_SMOOTH_COUNT; i++ )
        {
          voltage += measures[ i ];
        }
        voltage = static_cast< uint32_t >( floor( voltage / CURRENT_SMOOTH_COUNT ) );
        ESP_LOGD( AckuVoltage::tag, "acku smooth: %d mV (measured %d raw: %d)", voltage, measures[ idx ], adc_raw );
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
      // obsolete : adc_power_release();
      //
      // write a acku log file
      //
      if ( !StatusObject::getIsBrownout() && ( StatusObject::getWlanState() == WlanState::TIMESYNCED ) )
      {
        // write acku value to logfile
        std::string fileName( Prefs::ACKU_LOG_FILE );
        ESP_LOGD( AckuVoltage::tag, "acku value to file <%s>)", fileName.c_str() );
        //
        // if filesystemchecker want to write, prevent this
        // read is walways possible
        //
        if ( xSemaphoreTake( StatusObject::fileSem, pdMS_TO_TICKS( 1000 ) ) == pdTRUE )
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
          xSemaphoreGive( StatusObject::fileSem );
        }
      }
      //
      // sleep for a while, acku needs som time for changing
      //
      // vTaskDelay(pdMS_TO_TICKS(1003));
      vTaskDelay( pdMS_TO_TICKS( 45013 ) );
    }
  }

  /**
   * init calibration
   * https://github.com/espressif/esp-idf/blob/v4.4/examples/peripherals/adc/single_read/single_read/main/single_read.c
   */
  bool AckuVoltage::adcCalibrationInit()
  {
    using namespace Prefs;
    esp_err_t ret;
    bool cali_enable = false;

    ESP_LOGI( AckuVoltage::tag, "init adc calibration..." );
    // obsolete adc_power_acquire();
    vTaskDelay( pdMS_TO_TICKS( 1 ) );
    ret = esp_adc_cal_check_efuse( ADC_CALI_SCHEME );
    adc1_config_width( ACKU_ADC_WIDTH );
    adc1_config_channel_atten( ACKU_MEASURE_CHANNEL, ACKU_ATTENT );
    if ( ret == ESP_ERR_NOT_SUPPORTED )
    {
      ESP_LOGW( AckuVoltage::tag, "Calibration scheme not supported, skip software calibration" );
    }
    else if ( ret == ESP_ERR_INVALID_VERSION )
    {
      ESP_LOGW( AckuVoltage::tag, "eFuse not burnt, skip software calibration" );
    }
    else if ( ret == ESP_OK )
    {
      cali_enable = true;
      esp_adc_cal_characterize( ADC_UNIT_1, ACKU_ATTENT, ACKU_ADC_WIDTH, 0, &AckuVoltage::adc1_chars );
      ESP_LOGI( AckuVoltage::tag, "init adc calibration successful..." );
    }
    else
    {
      ESP_LOGE( AckuVoltage::tag, "Invalid arg" );
    }
    // obsolete: adc_power_release();
    return cali_enable;
  }
}  // namespace webserver
