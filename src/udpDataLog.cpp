#include <cstring>
#include "appPreferences.hpp"
#include "common.hpp"
#include "statics.hpp"
#include "statusObject.hpp"
#include "udpDataLog.hpp"

namespace EnvServer
{
  const char *UDPDataLog::tag{ "UDPDataLog" };
  bool UDPDataLog::isRunning{ false };
  std::shared_ptr< env_dataset > UDPDataLog::dataset = std::shared_ptr< env_dataset >( new env_dataset() );

  UDPDataLog::UDPDataLog(){};

  UDPDataLog::UDPDataLog( UDP &client, const IPAddress addr, uint16_t port, const char *appName )
      : _client( &client ), _ip( addr ), _port( port ), _appName( appName )
  {
  }

  void UDPDataLog::setUdpDataLog( UDP &client, const IPAddress addr, uint16_t port, const char *appName )
  {
    StatusObject::init();
    _client = &client;
    _ip = addr;
    _port = port;
    _appName = appName;
    this->start();
  }

  /**
   * RTOS Task to save data into file
   */
  void UDPDataLog::saveTask( void * )
  {
    using namespace logger;

    uint8_t errorCounter{ 0 };  // count if file semaphore not availible
    //
    elog.log( INFO, "%s: UDPDataLog Task started.", UDPDataLog::tag );
    //
    // infinity loop
    //
    while ( true )
    {
      delay( 1233 );
      if ( !UDPDataLog::dataset->empty() )
      {
        elog.log( INFO, "%s: try to send dataset...", UDPDataLog::tag );
        //
        // there are data, try to send to server
        // read one dataset
        //
        auto elem = UDPDataLog::dataset->front();
        UDPDataLog::dataset->erase( UDPDataLog::dataset->begin() );
        // create one object for one dataset
        cJSON *dataSetObj = cJSON_CreateObject();
        // make timestamp objekt item
        cJSON_AddItemToObject( dataSetObj, Prefs::JSON_TIMESTAMP_NAME,
                               cJSON_CreateString( std::to_string( elem.timestamp ).c_str() ) );
        cJSON_AddItemToObject( dataSetObj, Prefs::JSON_DEVICE_NAME, cJSON_CreateString( Prefs::LocalPrefs::getHostName().c_str() ) );
        // make array of measures
        auto m_dataset = elem.dataset;
        cJSON *mArray = cJSON_CreateArray();
        while ( !m_dataset.empty() )
        {
          // fill messure array
          auto m_elem = m_dataset.front();
          m_dataset.erase( m_dataset.begin() );
          cJSON *mObj = cJSON_CreateObject();
          char buffer[ 16 ];
          if ( m_elem.addr.length() > 16 )
          {
            cJSON_AddItemToObject( mObj, Prefs::JSON_SENSOR_ID_NAME, cJSON_CreateString( m_elem.addr.substring( 0, 15 ).c_str() ) );
          }
          else
          {
            cJSON_AddItemToObject( mObj, Prefs::JSON_SENSOR_ID_NAME, cJSON_CreateString( m_elem.addr.c_str() ) );
          }
          std::memset( &buffer[ 0 ], 0, 16 );
          sprintf( &buffer[ 0 ], "%3.1f", m_elem.temp );
          cJSON_AddItemToObject( mObj, Prefs::JSON_TEMPERATURE_NAME, cJSON_CreateString( &buffer[ 0 ] ) );
          std::memset( &buffer[ 0 ], 0, 16 );
          sprintf( &buffer[ 0 ], "%3.1f", m_elem.humidy );
          cJSON_AddItemToObject( mObj, Prefs::JSON_HUMIDY_NAME, cJSON_CreateString( &buffer[ 0 ] ) );
          // measure object to array
          cJSON_AddItemToArray( mArray, mObj );
        }
        // array as item to object
        cJSON_AddItemToObject( dataSetObj, Prefs::JSON_DATAOBJECT_NAME, mArray );
        //
        // dataSetObj complete
        // try to send to server
        //
        char *jsonPrintString = cJSON_PrintUnformatted( dataSetObj );
        String jsonString( jsonPrintString );
        jsonString += "\n";

        cJSON_free( jsonPrintString );  // !!!!!!! memory leak if not
        cJSON_Delete( dataSetObj );

        elog.log( INFO, "%s: dataset sent...", UDPDataLog::tag );
      }
    }
  }

  void UDPDataLog::start()
  {
    using namespace logger;

    elog.log( INFO, "%s: UDPDataLog Task start...", UDPDataLog::tag );

    if ( UDPDataLog::isRunning )
    {
      elog.log( ERROR, "%s: UDPDataLog Task is already running, abort.", UDPDataLog::tag );
    }
    else
    {
      xTaskCreate( UDPDataLog::saveTask, "datasend-task", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY, NULL );
    }
  }
}  // namespace EnvServer