#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <ds18x20.h>
#include <led_strip.h>
#include "AppPreferences.hpp"

namespace rest_webserver
{
  class StatusObject
  {
  private:
    static portMUX_TYPE statMutex;
    static bool is_init;
    static ds18x20_addr_t addrs[Prefs::TEMPERATURE_SENSOR_MAX_COUNT];
    static float temps[Prefs::TEMPERATURE_SENSOR_MAX_COUNT];
    static float temperature, humidity;
    static size_t sensor_count;
    static WlanState wlanState;
    static MsgState msgState;
    static bool http_active;

  public:
    static void init();

  public:
    static void setMeasures(size_t, ds18x20_addr_t[], float[], float, float);
    static void setWlanState(WlanState);
    static void setMsgState(MsgState);
    static MsgState getMsgState();
    static WlanState getWlanState();
    static void setHttpActive(bool);
    static bool getHttpActive();
  };
}
