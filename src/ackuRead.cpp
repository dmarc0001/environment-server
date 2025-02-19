#include "ackuRead.hpp"
#include "statusObject.hpp"
#include <cJSON.h>
#include <Elog.h>

namespace EnvServer
{
  const char *AckuVoltage::tag{ "AckuVoltage" };
  TaskHandle_t AckuVoltage::taskHandle{ nullptr };
  int AckuVoltage::todayDay{ -1 };

  ESP32AnalogRead AckuVoltage::adc{};

  void AckuVoltage::start()
  {
    AckuVoltage::adc.attach( Prefs::ACKU_MEASURE_GPIO );
    logger.log( Prefs::LOGID, DEBUG, "%s: ADC for acku initialized...", AckuVoltage::tag );
    logger.log( Prefs::LOGID, INFO, "%s: axcku tast started...", AckuVoltage::tag );

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
    xSemaphoreGive( StatusObject::ackuFileSem );

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
      logger.log( Prefs::LOGID, DEBUG, "%s: acku smooth: %d mV (measured %d )", AckuVoltage::tag, static_cast< int >( voltage ),
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
        String fileName( AckuVoltage::getTodayFileName() );
        logger.log( Prefs::LOGID, DEBUG, "%s: acku value to file <%s>", AckuVoltage::tag, fileName.c_str() );
        //
        // if filesystemchecker want to write, prevent this
        // read is walways possible
        //
        if ( xSemaphoreTake( StatusObject::ackuFileSem, pdMS_TO_TICKS( 1000 ) ) == pdTRUE )
        {
          auto fh = SPIFFS.open( fileName, "a", true );
          if ( fh )
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
            String jsonString( jsonPrintString );
            jsonString += "\n";
            fh.print( jsonString );
            fh.flush();
            fh.close();
            cJSON_Delete( dataSetObj );
            cJSON_free( jsonPrintString );  // !!!!!!!
            // memory leak if not do it!
            logger.log( Prefs::LOGID, INFO, "%s: acku value wrote to file <%s>.", AckuVoltage::tag, fileName.c_str() );
          }
          else
          {
            logger.log( Prefs::LOGID, WARNING, "%s: acku value can't wrote to file <%s>!", AckuVoltage::tag, fileName.c_str() );
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

  /**
   * get the filename for today
   */
  const String &AckuVoltage::getTodayFileName()
  {
    //  ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec
    struct tm ti;
    getLocalTime( &ti );
    if ( AckuVoltage::todayDay != ti.tm_mday )
    {
      logger.log( Prefs::LOGID, DEBUG, "%s: create acku today file name!", AckuVoltage::tag );
      char buffer[ 28 ];
      AckuVoltage::todayDay = ti.tm_mday;
      snprintf( buffer, 28, Prefs::WEB_DAYLY_ACKU_FILE_NAME, ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday );
      String fileName( Prefs::WEB_DATA_PATH );
      fileName += String( buffer );
      logger.log( Prefs::LOGID, DEBUG, "%s: acku today file name <%s>", AckuVoltage::tag, fileName.c_str() );
      StatusObject::setTodayAckuFileName( fileName );
    }
    return StatusObject::getTodayAckuFileName();
  }

}  // namespace EnvServer
