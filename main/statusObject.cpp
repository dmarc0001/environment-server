#include <time.h>
#include <memory>
#include <algorithm>
#include <esp_log.h>
#include <stdio.h>
#include <cJSON.h>
#include "appPreferences.hpp"
#include "statusObject.hpp"
#include "filesystemChecker.hpp"

namespace webserver
{
  const char *StatusObject::tag{ "status_object" };
  bool StatusObject::is_init{ false };
  bool StatusObject::is_running{ false };
  int StatusObject::currentVoltage{ 3300 };  //! current voltage in mV
  bool StatusObject::isBrownout{ false };    //! is voltage too low
  bool StatusObject::isLowAcku{ false };     //! is low acku
  SemaphoreHandle_t StatusObject::measureFileSem{ nullptr };
  SemaphoreHandle_t StatusObject::ackuFileSem{ nullptr };

  WlanState StatusObject::wlanState{ WlanState::DISCONNECTED };
  MeasureState StatusObject::msgState{ MeasureState::MEASURE_UNKNOWN };
  bool StatusObject::http_active{ false };
  std::shared_ptr< env_dataset > StatusObject::dataset = std::shared_ptr< env_dataset >( new env_dataset() );

  /**
   * init object and start file writer task
   */
  void StatusObject::init()
  {
    ESP_LOGI( StatusObject::tag, "init status object..." );
    // semaphore for using file writing
    vSemaphoreCreateBinary( StatusObject::measureFileSem );
    vSemaphoreCreateBinary( StatusObject::ackuFileSem );
    StatusObject::is_init = true;
    StatusObject::dataset->clear();
    ESP_LOGI( StatusObject::tag, "init status object...OK, start task..." );
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
    uint8_t errorCounter{ 0 };  // count if file semaphore not availible
    //
    ESP_LOGI( StatusObject::tag, "task started..." );
    xSemaphoreGive( StatusObject::measureFileSem );
    //
    // infinity loop
    //
    while ( true )
    {
      vTaskDelay( pdMS_TO_TICKS( 1403 ) );
      if ( !StatusObject::dataset->empty() )
      {
        //
        // there are data, try to save in file
        //
        std::string daylyFileName( Prefs::WEB_DAYLY_FILE );
        FILE *fd = nullptr;

        if ( StatusObject::getIsBrownout() )
        {
          ESP_LOGW( StatusObject::tag, "can't write data, voltage to low!" );
          vTaskDelay( pdMS_TO_TICKS( 15000 ) );
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
            if ( m_elem.addr > 0 )
            {
              cJSON_AddItemToObject( mObj, Prefs::JSON_SENSOR_ID_NAME,
                                     cJSON_CreateString( std::to_string( m_elem.addr ).substr( 1, 6 ).c_str() ) );
            }
            else
            {
              cJSON_AddItemToObject( mObj, Prefs::JSON_SENSOR_ID_NAME, cJSON_CreateString( "0" ) );
            }
            memset( &buffer[ 0 ], 0, 16 );
            sprintf( &buffer[ 0 ], "%3.1f", m_elem.temp );
            cJSON_AddItemToObject( mObj, Prefs::JSON_TEMPERATURE_NAME, cJSON_CreateString( &buffer[ 0 ] ) );
            memset( &buffer[ 0 ], 0, 16 );
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
            fd = fopen( daylyFileName.c_str(), "a" );
            if ( fd )
            {
              ESP_LOGI( StatusObject::tag, "datafile <%s> opened...", daylyFileName.c_str() );
              char *jsonPrintString = cJSON_PrintUnformatted( dataSetObj );
              fputs( jsonPrintString, fd );
              fflush( fd );
              fputs( "\n", fd );
              fclose( fd );
              cJSON_Delete( dataSetObj );
              cJSON_free( jsonPrintString );  // !!!!!!! memory leak if not
              ESP_LOGI( StatusObject::tag, "datafile <%s> written and closed...", daylyFileName.c_str() );
            }
            else
            {
              while ( !StatusObject::dataset->empty() )
              {
                StatusObject::dataset->erase( StatusObject::dataset->begin() );
              }
              ESP_LOGE( StatusObject::tag, "datafile <%s> can't open, data lost...", daylyFileName.c_str() );
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
            ESP_LOGE( StatusObject::tag, "can't get the semaphore for write datafile, garbage collector running to long?" );
            ESP_LOGE( StatusObject::tag, "Data may lost lost!" );
            vTaskDelay( pdMS_TO_TICKS( 800 ) );
            if ( errorCounter > 10 )
            {
              ESP_LOGE( StatusObject::tag, "too many errors in spiffs... warning!!!!!!" );
              if ( errorCounter > 15 )
              {
                //
                // maybe the whole system is bad
                //
                ESP_LOGE( StatusObject::tag, " seems the SPIFFS filesystem hat an problem, restart controller!" );
                vTaskDelay( pdMS_TO_TICKS( 5000 ) );
                esp_restart();
              }
              //
              // maybe the filesystemchcekr hung, reset this
              //
              FsCheckObject::start();
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
    if ( StatusObject::is_running )
    {
      ESP_LOGE( StatusObject::tag, "data save task is running, abort." );
    }
    else
    {
      xTaskCreate( StatusObject::saveTask, "data-task", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY, NULL );
    }
  }
}  // namespace webserver
