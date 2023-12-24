#include <algorithm>
#include <esp_timer.h>
#include "statics.hpp"
#include "tempMeasure.hpp"
#include "statusObject.hpp"
#include "appPreferences.hpp"

namespace EnvServer
{
  TaskHandle_t TempMeasure::taskHandle{ nullptr };

  // static object init
  const char *TempMeasure::tag{ "TempMeasure" };
  // max sensors
  const uint8_t TempMeasure::maxSensors = std::min( Prefs::SENSOR_TEMPERATURE_MAX_COUNT, Prefs::SENSOR_TEMPERATURE_MAX_COUNT_LIB );
  // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
  OneWire TempMeasure::oneWire( Prefs::SENSOR_TEMPERATURE_GPIO );
  // Pass our oneWire reference to Dallas Temperature.
  DallasTemperature TempMeasure::sensors( &TempMeasure::oneWire );
  // object for DHT sensor
  DHT_Unified TempMeasure::dht( Prefs::SENSOR_DHT_GPIO, Prefs::SENSOR_DHT_SENSOR_TYPE );

  // addr for sensors
  // DeviceAddress TempMeasure::addrs{};

  void TempMeasure::measureTask( void *pvParameter )
  {
    using namespace logger;

    int warning_rounds_count = 0;
    uint8_t sensors_count = 0;
    int64_t measure_interval = static_cast< int64_t >( Prefs::MEASURE_INTERVAL_SEC ) * 1000000LL;
    int64_t scan_interval = static_cast< int64_t >( Prefs::MEASURE_SCAN_SENSOR_INTERVAL ) * 1000000LL;
    // init one wire sensors
    elog.log( DEBUG, "%s: TempMeasure::measureTask...", TempMeasure::tag );
    TempMeasure::dht.begin();
    TempMeasure::sensors.begin();
    sensors_count = TempMeasure::renewOneWire();

    // esp_err_t res;
    while ( true )
    {
      static int64_t nextSensorsScanTime{ 600000LL };
      static int64_t nextMeasureTime{ 2400000LL };
      int64_t nowTime = esp_timer_get_time();
      static WlanState oldTimeServerState{ WlanState::DISCONNECTED };
      // check what does the state do
      WlanState currentState = StatusObject::getWlanState();
      if ( oldTimeServerState != currentState )
      {
        // changed! Set new state
        oldTimeServerState = currentState;
        if ( currentState == WlanState::TIMESYNCED )
        {
          // just time synced!
          nextMeasureTime = nowTime + 100000LL;
        }
      }

      // ############################################################
      //  if enough time ist over, rescann ds18x20 devices
      //  if count of sensors changes
      // ############################################################
      if ( nextSensorsScanTime < nowTime )
      {
        // set next time scan
        nextSensorsScanTime = nowTime + scan_interval;
        sensors_count = TempMeasure::renewOneWire();
        // not found sensors
        if ( sensors_count == 0 )
        {
          elog.log( WARNING, "%s: not found one wire temp sensors!", TempMeasure::tag );
          // earlier checking
          nextSensorsScanTime = nowTime + ( 10ULL * 1000000LL );
          vTaskDelay( pdMS_TO_TICKS( 50 ) );
          continue;
        }
        elog.log( INFO, "%s: %d sensors detected.", TempMeasure::tag, sensors_count );
        // reay only max count sensors, even trough i have found more
        if ( sensors_count > TempMeasure::maxSensors )
          sensors_count = TempMeasure::maxSensors;
        vTaskDelay( pdMS_TO_TICKS( 50 ) );
        continue;
      }
      // ############################################################
      //  it is time for measure the temperature?
      // ############################################################
      if ( nextMeasureTime < nowTime )
      {
        StatusObject::setMeasureState( MeasureState::MEASURE_ACTION );
        // next mesure time set
        // first check if voltage is okay
        if ( StatusObject::getIsBrownout() )
        {
          elog.log( WARNING, "%s: can't write data, voltage to low!", TempMeasure::tag );
          StatusObject::setMeasureState( MeasureState::MEASURE_CRIT );
          vTaskDelay( pdMS_TO_TICKS( 5000 ) );
          nextMeasureTime = nowTime + measure_interval;
          continue;
        }
        // next time to measure
        nextMeasureTime = nowTime + measure_interval;
        // chcek if time synced
        if ( StatusObject::getWlanState() != WlanState::TIMESYNCED )
        {
          elog.log( WARNING, "%s: time not sync, no save measure values!", TempMeasure::tag );
          StatusObject::setMeasureState( MeasureState::MEASURE_WARN );
          ++warning_rounds_count;
          if ( warning_rounds_count > Prefs::MEASURE_WARN_TO_CRIT_COUNT )
          {
            StatusObject::setMeasureState( MeasureState::MEASURE_CRIT );
          }
          continue;
        }
        elog.log( DEBUG, "%s: measuring...", TempMeasure::tag );
        timeval val;
        gettimeofday( &val, nullptr );
        TempMeasure::sensors.requestTemperatures();
        elog.log( INFO, "%s: measuring...done", TempMeasure::tag );
        // dataset create, set timestamp for this dataset
        // create a new dataset for this round of measure
        env_measure_t current_measure;
        current_measure.timestamp = val.tv_sec;
        //
        // all addrs
        //
        for ( uint8_t addr_idx = 0; addr_idx < sensors_count; ++addr_idx )
        {
          // one measure
          env_dataset_t sensor_data;
          uint8_t devAddr[ 10 ];
          char buffer[ 24 ];
          TempMeasure::sensors.getAddress( &devAddr[ 0 ], addr_idx );
          // make hexa string
          sprintf( buffer, "%02X%02X%02X%02X%02X%02X%02X%02X", devAddr[ 0 ], devAddr[ 1 ], devAddr[ 2 ], devAddr[ 3 ], devAddr[ 4 ],
                   devAddr[ 5 ], devAddr[ 6 ], devAddr[ 7 ] );
          String addr( buffer );
          sensor_data.addr = addr;
          sensor_data.humidy = -100.0f;
          // sensor_data.temp = TempMeasure::sensors.getTempCByIndex( addr_idx );
          sensor_data.temp = TempMeasure::sensors.getTempC( devAddr );
          if ( sensor_data.temp == DEVICE_DISCONNECTED_C )
          {
            elog.log( ERROR, "%s: Sensors (ds18x20) read error (DISCONNECTED)!", TempMeasure::tag );
            StatusObject::setMeasureState( MeasureState::MEASURE_WARN );
            sensor_data.temp = -100.0f;
            vTaskDelay( pdMS_TO_TICKS( 250 ) );
            continue;
          }
          else
          {
            elog.log( DEBUG, "%s: Sensor %02d reports %.3f°C...", TempMeasure::tag, addr_idx, sensor_data.temp );
          }
          current_measure.dataset.push_back( sensor_data );
          vTaskDelay( pdMS_TO_TICKS( 50 ) );
        }
        // plus 1 for dht11
        env_dataset_t h_sensor_data;
        sensors_event_t event;
        h_sensor_data.addr = String( "HUM" );
        h_sensor_data.humidy = -100.0f;
        h_sensor_data.temp = -100.0f;
        TempMeasure::dht.temperature().getEvent( &event );
        if ( isnan( event.temperature ) )
        {
          elog.log( ERROR, "%s: Error while reading temperature in DHT sensor", TempMeasure::tag );
        }
        else
        {
          h_sensor_data.temp = event.temperature;
        }
        // Get humidity event and print its value.
        dht.humidity().getEvent( &event );
        if ( isnan( event.relative_humidity ) )
        {
          elog.log( ERROR, "%s: Error while reading humidy in DHT sensor", TempMeasure::tag );
        }
        else
        {
          h_sensor_data.humidy = event.relative_humidity;
        }
        // if data availible
        if ( h_sensor_data.humidy > -100.0f || h_sensor_data.temp > -100.0f )
        {
          current_measure.dataset.push_back( h_sensor_data );
          elog.log( DEBUG, "%s: DHT sensor reports humidy: %.1f%%, temperature: %.1fC", TempMeasure::tag, h_sensor_data.humidy,
                    h_sensor_data.temp );
          if ( StatusObject::getMeasureState() == MeasureState::MEASURE_ACTION )
          {
            // nur wenn keine Warnungen gesetzt wurden
            // TODO: Anzahl der Warndurchläufe zählen, dann später CRIT setzten
            StatusObject::setMeasureState( MeasureState::MEASURE_NOMINAL );
            warning_rounds_count = 0;
          }
        }
        elog.log( DEBUG, "%s: dataset send to queue...", TempMeasure::tag );
        StatusObject::dataset->push_back( current_measure );
        delay( 50 );
      }
      delay( 500 );
    }
  }

  /**
   * renew sensor list
   */
  uint8_t TempMeasure::renewOneWire()
  {
    // devices found?
    TempMeasure::oneWire.reset_search();
    uint8_t sensors_count = TempMeasure::sensors.getDS18Count();
    elog.log( logger::DEBUG, "%s: %03d DS18x20 devices found...", TempMeasure::tag, sensors_count );
    elog.log( logger::DEBUG, "%s: sensors parasite mode: %s...", TempMeasure::tag, sensors.isParasitePowerMode() ? "true" : "false" );
    return ( sensors_count );
  }

  /**
   * start the measures as RTOS Task
   */
  void TempMeasure::start()
  {
    elog.log( logger::INFO, "%s: start measure task...", TempMeasure::tag );
    if ( TempMeasure::taskHandle )
    {
      vTaskDelete( TempMeasure::taskHandle );
      TempMeasure::taskHandle = nullptr;
    }
    StatusObject::init();
    xTaskCreate( TempMeasure::measureTask, "temp-measure", configMINIMAL_STACK_SIZE * 4, nullptr, 6, &TempMeasure::taskHandle );
  }

}  // namespace EnvServer
