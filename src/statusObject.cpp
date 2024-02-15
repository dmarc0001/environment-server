#include <time.h>
#include <memory>
#include <cstring>
#include <algorithm>
#include <esp_log.h>
#include <stdio.h>
#include <cJSON.h>
#include "statics.hpp"
#include "appPreferences.hpp"
#include "statusObject.hpp"
#include "ledStripe.hpp"
#include "ledColorsDefinition.hpp"

namespace EnvServer
{
  const char *StatusObject::tag{ "StatusObject" };
  bool StatusObject::is_init{ false };
  bool StatusObject::is_running{ false };
  bool StatusObject::is_spiffs{ false };
  int StatusObject::currentVoltage{ 3300 };       //! current voltage in mV
  bool StatusObject::isBrownout{ false };         //! is voltage too low
  bool StatusObject::isLowAcku{ false };          //! is low acku
  bool StatusObject::isFilesystemcheck{ false };  // is an filesystemcheck running or requested
  bool StatusObject::isDataSend{ false };         // sould data send to a server
  size_t StatusObject::todayFileSize{ 0 };        // filesize of the today.json file
  size_t StatusObject::weekFileSize{ 0 };         // filesize of the week.json file
  size_t StatusObject::monthFileSize{ 0 };        // filesize of the month.json file
  size_t StatusObject::ackuFileSize{ 0 };         //! filesize acku trace file
  size_t StatusObject::fsTotalSpace{ 0 };         //! filesystem total Space
  size_t StatusObject::fsUsedSpace{ 0 };          //! filesystem free Space

  WlanState StatusObject::wlanState{ WlanState::DISCONNECTED };
  MeasureState StatusObject::msgState{ MeasureState::MEASURE_UNKNOWN };
  bool StatusObject::http_active{ false };
  std::shared_ptr< env_dataset > StatusObject::dataset = std::shared_ptr< env_dataset >( new env_dataset() );
  //
  SemaphoreHandle_t StatusObject::measureFileSem{ nullptr };
  SemaphoreHandle_t StatusObject::ackuFileSem{ nullptr };

  /**
   * init object and start file writer task
   */
  void StatusObject::init()
  {
    using namespace logger;

    if ( StatusObject::is_init )
    {
      elog.log( DEBUG, "%s: always initialized... Igpore call", StatusObject::tag );
      return;
    }
    CRGB color( Prefs::LED_COLOR_BLACK );
    elog.log( INFO, "%s: init status object...", StatusObject::tag );
    //
    // init the filesystem for log and web
    //
    elog.log( DEBUG, "%s: init filesystem...", StatusObject::tag );
    if ( !SPIFFS.begin( false, Prefs::WEB_PATH, 8, Prefs::WEB_PARTITION_LABEL ) )
    {
      color = CRGB( Prefs::LED_COLOR_FORMATTING );
      LEDStripe::setLed( Prefs::LED_ALL, color );
      delay( 1000 );
      elog.log( INFO, "%s: init failed, FORMAT filesystem...", StatusObject::tag );
      if ( !SPIFFS.format() )
      {
        // there is an error BAD!
        color = CRGB( Prefs::LED_COLOR_ALERT );
        LEDStripe::setLed( Prefs::LED_ALL, color );
        elog.log( ERROR, "%s: An Error has occurred while mounting SPIFFS!", StatusObject::tag );
        delay( 5000 );
      }
      else
      {
        elog.log( INFO, "%s: FORMAT filesystem successful...", StatusObject::tag );
        // is okay
        StatusObject::is_spiffs = true;
      }
      ESP.restart();
    }
    else
    {
      // is okay
      StatusObject::is_spiffs = true;
      elog.log( DEBUG, "%s: init filesystem...OK", StatusObject::tag );
    }
    vSemaphoreCreateBinary( StatusObject::measureFileSem );
    vSemaphoreCreateBinary( StatusObject::ackuFileSem );
    StatusObject::is_init = true;
    StatusObject::dataset->clear();
    elog.log( DEBUG, "%s: init status object...OK", StatusObject::tag );
    StatusObject::start();
  }

  /**
   * set wlan state into object
   */
  void StatusObject::setWlanState( WlanState _state )
  {
    StatusObject::wlanState = _state;
  }

  /**
   * get wlan state from object
   */
  WlanState StatusObject::getWlanState()
  {
    return StatusObject::wlanState;
  }

  /**
   * set the status of the measure
   */
  void StatusObject::setMeasureState( MeasureState _st )
  {
    StatusObject::msgState = _st;
  }

  /**
   * give thenstsate of the measure
   */
  MeasureState StatusObject::getMeasureState()
  {
    return StatusObject::msgState;
  }

  /**
   * set is httpd active (active request of a file/data)
   */
  void StatusObject::setHttpActive( bool _http )
  {
    StatusObject::http_active = _http;
  }

  /**
   * get the active state of http daemon
   */
  bool StatusObject::getHttpActive()
  {
    return StatusObject::http_active;
  }

  /**
   * RTOS Task to save data into file
   */
  void StatusObject::saveTask( void *ptr )
  {
    using namespace logger;

    uint8_t errorCounter{ 0 };  // count if file semaphore not availible
    //
    elog.log( INFO, "%s: StatusObject Task started.", StatusObject::tag );
    xSemaphoreGive( StatusObject::measureFileSem );
    //
    // infinity loop
    //
    while ( true )
    {
      delay( 1403 );
      if ( !StatusObject::dataset->empty() )
      {
        //
        // there are data, try to save in file
        //
        String daylyFileName( Prefs::WEB_DAYLY_FILE );

        if ( StatusObject::getIsBrownout() )
        {
          elog.log( WARNING, "%s: can't write data, voltage too low!", StatusObject::tag );
          delay( 15000 );
          // HEAP check, if to low, delete data
          uint32_t nowHeap = xPortGetFreeHeapSize();
          while ( nowHeap < 1024 )
          {
            StatusObject::dataset->erase( StatusObject::dataset->begin() );
            nowHeap = xPortGetFreeHeapSize();
          }
          continue;
        }
        ESP_LOGI( StatusObject::tag, "data for save exist..." );
        //
        // while datsa exist
        //
        while ( !StatusObject::dataset->empty() )
        {
          // read one dataset
          auto elem = StatusObject::dataset->front();
          StatusObject::dataset->erase( StatusObject::dataset->begin() );
          // create one object for one dataset
          cJSON *dataSetObj = cJSON_CreateObject();
          // make timestamp objekt item
          cJSON_AddItemToObject( dataSetObj, Prefs::JSON_TIMESTAMP_NAME,
                                 cJSON_CreateString( std::to_string( elem.timestamp ).c_str() ) );
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
          // dataSetObj complete
          // try to write to file
          // wait max 60 sec, then counter incrase and continue
          //
          if ( xSemaphoreTake( StatusObject::measureFileSem, pdMS_TO_TICKS( 60000 ) ) == pdTRUE )
          {
            errorCounter = 0;
            // We were able to obtain the semaphore and can now access the
            // shared resource.
            // open File mode append
            auto fh = SPIFFS.open( daylyFileName, "a", true );
            if ( fh )
            {
              elog.log( DEBUG, "%s: datafile <%s> opened...", StatusObject::tag, daylyFileName.c_str() );
              fh.print( "," );
              char *jsonPrintString = cJSON_PrintUnformatted( dataSetObj );
              String jsonString( jsonPrintString );
              jsonString += "\n";
              fh.print( jsonString );
              fh.flush();
              jsonString.clear();
              cJSON_free( jsonPrintString );  // !!!!!!! memory leak if not
              StatusObject::setTodayFileSize( fh.size() );
              fh.close();
              cJSON_Delete( dataSetObj );
              elog.log( INFO, "%s: datafile <%s> written and closed...", StatusObject::tag, daylyFileName.c_str() );
            }
            else
            {
              while ( !StatusObject::dataset->empty() )
              {
                StatusObject::dataset->erase( StatusObject::dataset->begin() );
              }
              elog.log( ERROR, "%s: datafile <%s> can't open, data lost!", StatusObject::tag, daylyFileName.c_str() );
            }
            // We have finished accessing the shared resource.  Release the
            // semaphore.
            xSemaphoreGive( StatusObject::measureFileSem );
          }
          else
          {
            //
            // timeout while wait for Data writing, maybe the cleaning routine is running
            // make an extra long little sleep
            //
            ++errorCounter;
            //
            elog.log( ERROR, "%s: can't get the semaphore for write datafile, garbage collector running to long?", StatusObject::tag );
            elog.log( ERROR, "%s: data may be lost!", StatusObject::tag );
            delay( 800 );
            if ( errorCounter > 10 )
            {
              elog.log( ERROR, "%s: too many errors in spiffs... warning!!!!!", StatusObject::tag );
              if ( errorCounter > 15 )
              {
                //
                // maybe the whole system is bad
                //
                elog.log( ERROR, "%s: seems the SPIFFS filesystem hat an problem, restart controller!", StatusObject::tag );
                delay( 5000 );
                esp_restart();
              }
              //
              // maybe the filesystemchcekr hung, reset this
              //
              // FsCheckObject::start();
            }
          }
        }
      }
    }
  }

  /**
   * is voltage too low for flash write ?
   */
  bool StatusObject::getIsBrownout()
  {
    return StatusObject::isBrownout;
  }

  /**
   * do consider voltage for the system
   */
  void StatusObject::setVoltage( int _val )
  {
    StatusObject::currentVoltage = _val;
    //
    // acku full, power on
    //
    if ( _val >= Prefs::ACKU_LOWER_VALUE )
    {
      // all fine
      StatusObject::isBrownout = false;
      StatusObject::isLowAcku = false;
      return;
    }
    //
    // value to small
    //
    if ( _val < Prefs::ACKU_BROWNOUT_LOWEST )
    {
      // while this current, controller is NOT running
      // ==> this value is an failure
      StatusObject::isBrownout = false;
      StatusObject::isLowAcku = false;
      return;
    }
    //
    // else, the value can be the truth....
    // but small
    //
    if ( _val >= Prefs::ACKU_LOWER_VALUE )
    {
      // is acku to low for flash writing
      StatusObject::isLowAcku = true;
      StatusObject::isBrownout = false;
    }
    if ( _val < Prefs::ACKU_BROWNOUT_VALUE )
    {
      // controller will shutdown, no write to FLUSH!!!!
      StatusObject::isBrownout = true;
      StatusObject::isLowAcku = true;
      return;
    }
    return;
  }

  int StatusObject::getVoltage()
  {
    return StatusObject::currentVoltage;
  }

  bool StatusObject::getLowAcku()
  {
    return StatusObject::isLowAcku;
  }

  /**
   * start (if not running yet) the data writer task
   */
  void StatusObject::start()
  {
    using namespace logger;

    elog.log( INFO, "%s: StatusObject Task start...", StatusObject::tag );

    if ( StatusObject::is_running )
    {
      elog.log( ERROR, "%s: StatusObject Task is already running, abort.", StatusObject::tag );
    }
    else
    {
      xTaskCreate( StatusObject::saveTask, "data-task", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY, NULL );
    }
  }
}  // namespace EnvServer
