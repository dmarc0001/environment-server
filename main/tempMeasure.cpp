
#include "tempMeasure.hpp"
#include "statusObject.hpp"
#include "appPreferences.hpp"

namespace webserver
{
  const char *TempMeasure::tag{"measure_thread"};
  bool TempMeasure::is_running{false};

  void TempMeasure::measureTask(void *pvParameter)
  {
    TempMeasure::is_running = true;
    size_t sensor_count = 0;
    int warning_rounds_count = 0;
    ds18x20_addr_t addrs[Prefs::SENSOR_TEMPERATURE_MAX_COUNT];
    int64_t measure_interval = static_cast<int64_t>(Prefs::MEASURE_INTERVAL_SEC) * 1000000LL;
    int64_t scan_interval = static_cast<int64_t>(Prefs::MEASURE_SCAN_SENSOR_INTERVAL) * 1000000LL;

    //
    // DHT-11 init
    //
    ESP_LOGI(TempMeasure::tag, "init internal pullup for dht sensor...");
    gpio_set_pull_mode(Prefs::SENSOR_DHT_GPIO, GPIO_PULLUP_ONLY);
    //
    // There is no special initialization required before using the ds18x20
    // routines.  However, we make sure that the internal pull-up resistor is
    // enabled on the GPIO pin so that one can connect up a sensor without
    // needing an external pull-up (Note: The internal (~47k) pull-ups of the
    // ESP do appear to work, at least for simple setups (one or two sensors
    // connected with short leads), but do not technically meet the pull-up
    // requirements from the ds18x20 datasheet and may not always be reliable.
    // For a real application, a proper 4.7k external pull-up resistor is
    // recommended instead!)
    //ESP_LOGI(TempMeasure::tag, "init internal pullup for ds18x20 sensor...");
    //gpio_set_pull_mode(Prefs::TEMPERATURE_SENSOR_GPIO, GPIO_PULLUP_ONLY);

    esp_err_t res;
    while (true)
    {
      static int64_t nextSensorsScanTime{600000LL};
      static int64_t nextMeasureTime{2400000LL};
      int64_t nowTime = esp_timer_get_time();
      //############################################################
      // if enough time ist over, rescann ds18x20 devices
      // if count of sensors changes
      //############################################################
      if (nextSensorsScanTime < nowTime)
      {
        // set next time scan
        nextSensorsScanTime = nowTime + scan_interval;
        // mutex for measures
        res = ds18x20_scan_devices(Prefs::SENSOR_TEMPERATURE_GPIO, addrs, Prefs::SENSOR_TEMPERATURE_MAX_COUNT, &sensor_count);
        //
        if (res != ESP_OK)
        {
          ESP_LOGE(TempMeasure::tag, "Sensors scan error %d (%s)", res, esp_err_to_name(res));
          sensor_count = 0;
          // earlier checking
          nextSensorsScanTime = nowTime + (10ULL * 1000000LL);
          vTaskDelay(pdMS_TO_TICKS(50));
          continue;
        }
        // not found sensors?
        if (!sensor_count)
        {
          // earlier checking
          nextSensorsScanTime = nowTime + (10LL * 1000000LL);
          ESP_LOGW(TempMeasure::tag, "No ds18x20 sensors detected!");
          vTaskDelay(pdMS_TO_TICKS(50));
          continue;
        }
        ESP_LOGI(TempMeasure::tag, "%d sensors detected", sensor_count);
        // reay only max count sensors, even trough i have found more
        if (sensor_count > Prefs::SENSOR_TEMPERATURE_MAX_COUNT)
          sensor_count = Prefs::SENSOR_TEMPERATURE_MAX_COUNT;
        vTaskDelay(pdMS_TO_TICKS(50));
        continue;
      }
      //############################################################
      // it is time for measure the temperature?
      //############################################################
      if (nextMeasureTime < nowTime)
      {
        StatusObject::setMeasureState(MeasureState::MEASURE_ACTION);
        // next mesure time set
        nextMeasureTime = nowTime + measure_interval;
        if (StatusObject::getWlanState() != WlanState::TIMESYNCED)
        {
          ESP_LOGW(TempMeasure::tag, "time not sync, no save mesure values!");
          StatusObject::setMeasureState(MeasureState::MEASURE_WARN);
          ++warning_rounds_count;
          if (warning_rounds_count > Prefs::MEASURE_WARN_TO_CRIT_COUNT)
          {
            StatusObject::setMeasureState(MeasureState::MEASURE_CRIT);
          }
          continue;
        }
        ESP_LOGI(TempMeasure::tag, "Measuring...");
        timeval val;
        gettimeofday(&val, nullptr);
        //
        // all addrs
        // plus 1 for dht11
        std::shared_ptr<env_dataset> dataset = std::shared_ptr<env_dataset>(new env_dataset());
        for (int addr_idx = 0; addr_idx < sensor_count; ++addr_idx)
        {
          env_measure_t current;
          current.addr = addrs[addr_idx];
          current.timestamp = val.tv_sec;
          current.humidy = -100.0;
          res = ds18x20_measure_and_read(Prefs::SENSOR_TEMPERATURE_GPIO, current.addr, &current.temp);
          if (res != ESP_OK)
          {
            ESP_LOGE(TempMeasure::tag, "Sensors (ds10x20) read error %d (%s)", res, esp_err_to_name(res));
            StatusObject::setMeasureState(MeasureState::MEASURE_WARN);
            current.temp = -100.0;
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
          }
          else
          {
            float temp_c = current.temp;
            ESP_LOGI(TempMeasure::tag, "Sensor %08" PRIx32 "%08" PRIx32 " (%s) reports %.3f°C",
                     (uint32_t)(current.addr >> 32), (uint32_t)current.addr,
                     (current.addr & 0xff) == DS18B20_FAMILY_ID ? "DS18B20" : "DS18S20",
                     temp_c);
          }
          dataset->push_back(current);
          vTaskDelay(pdMS_TO_TICKS(50));
        }
        env_measure_t current_dht;
        current_dht.addr = 0;
        current_dht.timestamp = val.tv_sec;
        res = dht_read_float_data(Prefs::SENSOR_DHT_SENSOR_TYPE, Prefs::SENSOR_DHT_GPIO, &current_dht.humidy, &current_dht.temp);
        if (res == ESP_OK)
        {
          dataset->push_back(current_dht);
          ESP_LOGI(TempMeasure::tag, "Humidity: %.1f%% Temp: %.1fC", current_dht.humidy, current_dht.temp);
          if (StatusObject::getMeasureState() == MeasureState::MEASURE_ACTION)
          {
            // nur wenn keine Warnungen gesetzt wurden
            // TODO: Anzahl der Warndurchläufe zählen, dann später CRIT setzten
            StatusObject::setMeasureState(MeasureState::MEASURE_NOMINAL);
            warning_rounds_count = 0;
          }
        }
        else
        {
          ESP_LOGW(TempMeasure::tag, "Could not read data from dht11 sensor");
          StatusObject::setMeasureState(MeasureState::MEASURE_WARN);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
        StatusObject::setMeasures(dataset);
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }

  /**
   * start the measures as RTOS Task
  */
  void TempMeasure::start()
  {
    if (TempMeasure::is_running)
    {
      ESP_LOGE(TempMeasure::tag, "temperature measure task is running, abort.");
    }
    else
    {
      xTaskCreate(TempMeasure::measureTask, "temp-measure", configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL);
    }
  }

}
