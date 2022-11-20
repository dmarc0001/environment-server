
#include "tempMeasure.hpp"
#include "statusObject.hpp"
#include "AppPreferences.hpp"

namespace rest_webserver
{
  const char *TempMeasure::tag{"measure_thread"};
  bool TempMeasure::is_running{false};

  void TempMeasure::measureTask(void *pvParameter)
  {
    TempMeasure::is_running = true;
    // ds18x20_addr_t addrs[Prefs::TEMPERATURE_SENSOR_MAX_COUNT];
    // float temps[Prefs::TEMPERATURE_SENSOR_MAX_COUNT];
    size_t sensor_count = 0;
    ds18x20_addr_t addrs[Prefs::TEMPERATURE_SENSOR_MAX_COUNT];
    int64_t measure_interval = static_cast<int64_t>(Prefs::MEASURE_INTERVAL_SEC) * 1000000LL;
    int64_t scan_interval = static_cast<int64_t>(Prefs::MEASURE_SCAN_SENSOR_INTERVAL) * 1000000LL;

    //
    // DHT-11 init
    //
    ESP_LOGI(TempMeasure::tag, "init internal pullup for dht sensor...");
    gpio_set_pull_mode(Prefs::DHT_SENSOR_GPIO, GPIO_PULLUP_ONLY);
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
    ESP_LOGI(TempMeasure::tag, "init internal pullup for ds18x20 sensor...");
    gpio_set_pull_mode(Prefs::TEMPERATURE_SENSOR_GPIO, GPIO_PULLUP_ONLY);

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
        res = ds18x20_scan_devices(Prefs::TEMPERATURE_SENSOR_GPIO, addrs, Prefs::TEMPERATURE_SENSOR_MAX_COUNT, &sensor_count);
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
        if (sensor_count > Prefs::TEMPERATURE_SENSOR_MAX_COUNT)
          sensor_count = Prefs::TEMPERATURE_SENSOR_MAX_COUNT;
        vTaskDelay(pdMS_TO_TICKS(50));
        continue;
      }
      //############################################################
      // it is time for measure the temperature?
      //############################################################
      if (nextMeasureTime < nowTime)
      {
        // next mesure time set
        nextMeasureTime = nowTime + measure_interval;
        ESP_LOGI(TempMeasure::tag, "Measuring...");
        //
        // all addrs
        // plus 1 for dht11
        env_measure_t m_values[Prefs::TEMPERATURE_SENSOR_MAX_COUNT + 1]{};
        for (int addr_idx = 0; addr_idx < sensor_count; ++addr_idx)
        {
          m_values[addr_idx].addr = addrs[addr_idx];
          res = ds18x20_measure_and_read(Prefs::TEMPERATURE_SENSOR_GPIO, m_values[addr_idx].addr, &m_values[addr_idx].temp);
          if (res != ESP_OK)
          {
            ESP_LOGE(TempMeasure::tag, "Sensors (ds10x20) read error %d (%s)", res, esp_err_to_name(res));
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
          }
          else
          {
            float temp_c = m_values[addr_idx].temp;
            ESP_LOGI(TempMeasure::tag, "Sensor %08" PRIx32 "%08" PRIx32 " (%s) reports %.3f°C",
                     (uint32_t)(m_values[addr_idx].addr >> 32), (uint32_t)m_values[addr_idx].addr,
                     (m_values[addr_idx].addr & 0xff) == DS18B20_FAMILY_ID ? "DS18B20" : "DS18S20",
                     temp_c);
          }
          vTaskDelay(pdMS_TO_TICKS(125));
        }
        m_values[Prefs::TEMPERATURE_SENSOR_MAX_COUNT].addr = 0;
        res = dht_read_float_data(Prefs::DHT_SENSOR_TYPE, Prefs::DHT_SENSOR_GPIO, &m_values[Prefs::TEMPERATURE_SENSOR_MAX_COUNT].humidy, &m_values[Prefs::TEMPERATURE_SENSOR_MAX_COUNT].temp);
        if (res == ESP_OK)
          ESP_LOGI(TempMeasure::tag, "Humidity: %.1f%% Temp: %.1fC\n", m_values[Prefs::TEMPERATURE_SENSOR_MAX_COUNT].humidy, m_values[Prefs::TEMPERATURE_SENSOR_MAX_COUNT].temp);
        else
          ESP_LOGW(TempMeasure::tag, "Could not read data from dht11 sensor\n");
        vTaskDelay(pdMS_TO_TICKS(50));
        if (StatusObject::getWlanState() == WlanState::TIMESYNCED)
        {
          //
          // send to status object
          //
          timeval val;
          gettimeofday(&val, nullptr);
          //StatusObject::setMeasures(sensor_count, m_values);
        }
        else
        {
          ESP_LOGW(TempMeasure::tag, "time not sync, no save mesure values!");
        }
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }

  void TempMeasure::start()
  {
    if (TempMeasure::is_running)
    {
      ESP_LOGE(TempMeasure::tag, "temperature measure task is running, abort.");
    }
    else
    {
      //xTaskCreate(static_cast<void (*)(void *)>(&(rest_webserver::TempMeasure::measureTask)), "ds18x20", configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL);

      xTaskCreate(TempMeasure::measureTask, "temp-measure", configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL);
    }
  }

}
