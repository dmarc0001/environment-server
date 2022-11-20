#include "AppPreferences.hpp"
#include "statusObject.hpp"

namespace rest_webserver
{
  portMUX_TYPE StatusObject::statMutex{0U, 0U};
  bool StatusObject::is_init{false};
  env_measure_t StatusObject::m_values[Prefs::TEMPERATURE_SENSOR_MAX_COUNT + 1]{};
  size_t StatusObject::sensor_count;
  WlanState StatusObject::wlanState{WlanState::DISCONNECTED};
  MsgState StatusObject::msgState{MsgState::MSG_NOMINAL};
  bool StatusObject::http_active{false};

  void StatusObject::init()
  {
    spinlock_initialize(&StatusObject::statMutex);
    //vPortCPUInitializeMutex(&StatusObject::statMutex);
    StatusObject::is_init = true;
  }

  void StatusObject::setMeasures(size_t _sensor_count, const env_measure_a &_values)
  {
    if (!StatusObject::is_init)
      init();
    portENTER_CRITICAL(&StatusObject::statMutex);
    if (_sensor_count > 0)
    {
      StatusObject::sensor_count = _sensor_count;
      for (int idx = 0; idx < Prefs::TEMPERATURE_SENSOR_MAX_COUNT + 1; ++idx)
      {
        if (idx < _sensor_count)
        {
          StatusObject::m_values[idx] = _values[idx];
        }
        else
        {
          StatusObject::m_values[idx] = {};
        }
      }
    }
    portEXIT_CRITICAL(&StatusObject::statMutex);
    // TODO: sichern in file
  }

  void StatusObject::setWlanState(WlanState _state)
  {
    StatusObject::wlanState = _state;
  }

  WlanState StatusObject::getWlanState()
  {
    return StatusObject::wlanState;
  }

  void StatusObject::setMsgState(MsgState _st)
  {
    StatusObject::msgState = _st;
  }

  MsgState StatusObject::getMsgState()
  {
    return StatusObject::msgState;
  }

  void StatusObject::setHttpActive(bool _http)
  {
    StatusObject::http_active = _http;
  }

  bool StatusObject::getHttpActive()
  {
    return StatusObject::http_active;
  }

}
