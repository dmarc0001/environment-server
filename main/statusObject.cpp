#include "AppPreferences.hpp"
#include "statusObject.hpp"

namespace rest_webserver
{
  portMUX_TYPE StatusObject::statMutex{0U, 0U};
  ds18x20_addr_t StatusObject::addrs[Prefs::TEMPERATURE_SENSOR_MAX_COUNT]{};
  float StatusObject::temps[Prefs::TEMPERATURE_SENSOR_MAX_COUNT]{};
  size_t StatusObject::sensor_count;
  float StatusObject::temperature{};
  float StatusObject::humidity{};
  WlanState StatusObject::wlanState{WlanState::DISCONNECTED};

  void StatusObject::setMeasures(size_t _sensor_count, ds18x20_addr_t _addrs[], float temperatures[], float _temp, float _humidy)
  {
    portENTER_CRITICAL(&StatusObject::statMutex);
    if (_sensor_count > 0)
    {
      StatusObject::sensor_count = _sensor_count;
      for (int idx = 0; idx < Prefs::TEMPERATURE_SENSOR_MAX_COUNT; ++idx)
      {
        if (idx < _sensor_count)
        {
          StatusObject::addrs[idx] = _addrs[idx];
          StatusObject::temps[idx] = temperatures[idx];
        }
        else
        {
          StatusObject::addrs[idx] = 0ULL;
          StatusObject::temps[idx] = .0f;
        }
      }
    }
    StatusObject::temperature = _temp;
    StatusObject::humidity = _humidy;
    portEXIT_CRITICAL(&StatusObject::statMutex);
  }

  void StatusObject::setWlanState(WlanState _state)
  {
    StatusObject::wlanState = _state;
  }

  WlanState StatusObject::getWlanState()
  {
    return StatusObject::wlanState;
  }
}
