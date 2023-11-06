#pragma once
#include "common.hpp"
#include "statics.hpp"
#include "appPreferences.hpp"
#include <ESP32AnalogRead.h>

namespace EnvServer
{
  class AckuVoltage
  {
    private:
    static ESP32AnalogRead adc;

    public:
    static void init();
    static float readValue();
  };
}  // namespace EnvServer
