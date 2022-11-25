#include <time.h>
#include <memory>
#include <algorithm>
#include <iostream>

#include "AppPreferences.hpp"
#include "statusObject.hpp"

namespace rest_webserver
{
  portMUX_TYPE StatusObject::statMutex{0U, 0U};
  bool StatusObject::is_init{false};
  WlanState StatusObject::wlanState{WlanState::DISCONNECTED};
  MeasureState StatusObject::msgState{MeasureState::MEASURE_UNKNOWN};
  bool StatusObject::http_active{false};

  void StatusObject::init()
  {
    spinlock_initialize(&StatusObject::statMutex);
    //vPortCPUInitializeMutex(&StatusObject::statMutex);
    StatusObject::is_init = true;
  }

  void StatusObject::setMeasures(std::shared_ptr<env_dataset> dataset)
  {
    if (!StatusObject::is_init)
      init();

    //portENTER_CRITICAL(&StatusObject::statMutex);
    std::for_each(dataset->begin(), dataset->end(), [](const env_measure_t n) {
      std::cout << n.addr << std::endl;
    });
    // if (_sensor_count > 0)
    // {
    //   StatusObject::sensor_count = _sensor_count;
    //   for (int idx = 0; idx < Prefs::TEMPERATURE_SENSOR_MAX_COUNT + 1; ++idx)
    //   {
    //     if (idx < _sensor_count)
    //     {
    //       StatusObject::m_values[idx] = _values[idx];
    //     }
    //     else
    //     {
    //       StatusObject::m_values[idx] = {};
    //     }
    //   }
    // }
    //portEXIT_CRITICAL(&StatusObject::statMutex);
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

  void StatusObject::setMeasureState(MeasureState _st)
  {
    StatusObject::msgState = _st;
  }

  MeasureState StatusObject::getMeasureState()
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

  void StatusObject::saveTask(void *ptr)
  {

    // time_t rawtime;
    // struct tm *timeinfo;

    // time(&rawtime);
    // timeinfo = localtime(&rawtime);
    // printf("The current date/time is: %s", asctime(timeinfo));
  }

}
