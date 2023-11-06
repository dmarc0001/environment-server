#include "ackuRead.hpp"

namespace EnvServer
{
  ESP32AnalogRead AckuVoltage::adc{};

  void AckuVoltage::init()
  {
    AckuVoltage::adc.attach( Prefs::ACKU_MEASURE_GPIO );
    elog.log( DEBUG, "ADC for acku initialized..." );
  }

  float AckuVoltage::readValue()
  {
    return ( AckuVoltage::adc.readVoltage() * 1.92f );
  }
}  // namespace EnvServer
