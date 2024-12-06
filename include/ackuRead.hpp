#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "common.hpp"
#include "appPreferences.hpp"
#include <ESP32AnalogRead.h>

namespace EnvServer
{
  class AckuVoltage
  {
    private:
    static const char *tag;
    static ESP32AnalogRead adc;
    static TaskHandle_t taskHandle;

    public:
    static void start();

    private:
    static void ackuTask( void * );
  };
}  // namespace EnvServer
