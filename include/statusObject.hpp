#pragma once
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <ds18x20.h>
#include <led_strip.h>
#include "AppPreferences.hpp"

namespace webserver
{
  class StatusObject
  {
  private:
    static portMUX_TYPE statMutex;
    static bool is_init;
    static WlanState wlanState;
    static MeasureState msgState;
    static bool http_active;

  public:
    static void init();
    static void setMeasures(std::shared_ptr<env_dataset>);
    static void setWlanState(WlanState);
    static void setMeasureState(MeasureState);
    static MeasureState getMeasureState();
    static WlanState getWlanState();
    static void setHttpActive(bool);
    static bool getHttpActive();

  private:
    static void saveTask(void *);
  };
}
