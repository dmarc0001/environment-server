#include "ackuRead.hpp"
#include "statusObject.hpp"
#include <cJSON.h>

namespace EnvServer
{
  const char *AckuVoltage::tag{ "AckuVoltage" };
  TaskHandle_t AckuVoltage::taskHandle{ nullptr };

  ESP32AnalogRead AckuVoltage::adc{};

  void AckuVoltage::start()
  {
    AckuVoltage::adc.attach( Prefs::ACKU_MEASURE_GPIO );
    elog.log( DEBUG, "%s: ADC for acku initialized...", AckuVoltage::tag );
    elog.log( INFO, "%s: axcku tast started...", AckuVoltage::tag );

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

  void AckuVoltage::ackuTask( void * )
  {
    using namespace Prefs;

    static uint32_t measures[ ACKU_CURRENT_SMOOTH_COUNT ];
    uint32_t idx = 0;
    uint32_t m_value{ 0 };
    // esp_err_t retVal;

    // fill array with defaults
    for ( auto i = 0; i < ACKU_CURRENT_SMOOTH_COUNT; i++ )
    {
      measures[ i ] = ACKU_LOWER_VALUE;
    }
    delay( 233 );
    m_value = static_cast< uint32_t >( AckuVoltage::adc.readVoltage() * 2000.0f );
    for ( idx = 0; idx < ACKU_CURRENT_SMOOTH_COUNT; ++idx )
    {
      measures[ idx ] = m_value;
    }
    idx = 0;
    //
    // infinity loop
    //
    while ( true )
    {
      delay( 1 );
      uint32_t voltage{ 0 };
      measures[ idx ] = m_value = static_cast< uint32_t >( AckuVoltage::adc.readVoltage() * 2000.0f );
      for ( auto i = 0; i < ACKU_CURRENT_SMOOTH_COUNT; i++ )
      {
        voltage += measures[ i ];
      }
      voltage = static_cast< uint32_t >( floor( voltage / ACKU_CURRENT_SMOOTH_COUNT ) );
      elog.log( DEBUG, "%s: acku smooth: %d mV (measured %d )", AckuVoltage::tag, static_cast< int >( voltage ),
                static_cast< int >( m_value ) );
      StatusObject::setVoltage( voltage );
      ++idx;
      if ( idx >= ACKU_CURRENT_SMOOTH_COUNT )
      {
        idx = 0U;
      }
      //
      // write a acku log file
      //
      if ( !StatusObject::getIsBrownout() && ( StatusObject::getWlanState() == WlanState::TIMESYNCED ) )
      {
        // write acku value to logfile
        std::string fileName( Prefs::ACKU_LOG_FILE_01 );
        elog.log( DEBUG, "%s: acku value to file <%s>", AckuVoltage::tag, fileName.c_str() );
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
            cJSON_free( jsonPrintString );  // !!!!!!!
            // memory leak if not do it!
            elog.log( INFO, "%s: acku value wrote to file <%s>.", AckuVoltage::tag, fileName.c_str() );
          }
          else
          {
            elog.log( WARNING, "%s: acku value can't wrote to file <%s>!", AckuVoltage::tag, fileName.c_str() );
          }
          xSemaphoreGive( StatusObject::ackuFileSem );
        }
      }
      //
      // sleep for a while, acku needs som time for changing
      //
      delay( 132351 );
    }
  }

}  // namespace EnvServer
