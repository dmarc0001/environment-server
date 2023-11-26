#include <inttypes.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "appPreferences.hpp"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

namespace EnvServer
{
  class TempMeasure
  {
    private:
    static const char *tag;
    static OneWire oneWire;
    static DallasTemperature sensors;
    // static DeviceAddress addrs;
    static DHT_Unified dht;
    static const uint8_t maxSensors;

    public:
    static void start();
    static TaskHandle_t taskHandle;

    private:
    static void measureTask( void * );
    static uint8_t renewOneWire();
  };

}  // namespace EnvServer
