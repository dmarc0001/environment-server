#pragma once
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <ds18x20.h>
#include <led_strip.h>
#include "appPreferences.hpp"

namespace webserver
{
  class StatusObject
  {
  private:
    static const char *tag;       //! TAG for esp log
    static bool is_init;          //! was object initialized
    static bool is_running;       //! is save Task running?
    static WlanState wlanState;   //! is wlan disconnected, connected etc....
    static MeasureState msgState; //! which state ist the mesure
    static bool http_active;      //! was an acces via http?
    static int currentVoltage;    //! current voltage in mV
    static bool isBrownout;       //! is voltage too low
  public:
    static std::shared_ptr<env_dataset> dataset; //! set of mesures
    static SemaphoreHandle_t fileSem;            // is access to files busy

  public:
    static void init();
    static void start();
    static void setWlanState(WlanState);
    static void setMeasureState(MeasureState);
    static MeasureState getMeasureState();
    static WlanState getWlanState();
    static void setHttpActive(bool);
    static bool getHttpActive();
    static bool getIsBrownout();
    static void setVoltage(int);
    static int getVoltage();

  private:
    static void saveTask(void *);
  };
}
