#pragma once
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace webserver
{

  enum WlanState : uint8_t
  {
    DISCONNECTED,
    SEARCHING,
    CONNECTED,
    TIMESYNCED,
    FAILED
  };

  enum MeasureState : uint8_t
  {
    MEASURE_UNKNOWN,
    MEASURE_ACTION,
    MEASURE_NOMINAL,
    MEASURE_WARN,
    MEASURE_ERR,
    MEASURE_CRIT
  };

  struct env_dataset_t
  {
    // ds18x20_addr_t addr;
    float temp;
    float humidy;
  };

  struct env_measure_t
  {
    uint32_t timestamp;
    std::vector< env_dataset_t > dataset;
  };

  using env_dataset = std::vector< env_measure_t >;
  // using env_measure_a = env_measure_t[Prefs::TEMPERATURE_SENSOR_MAX_COUNT + 1];

}  // namespace webserver
