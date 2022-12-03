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
    static const char *tag;                      //! TAG for esp log
    static bool is_init;                         //! was object initialized
    static bool is_running;                      //! is save Task running?
    static SemaphoreHandle_t fileSem;            // is access to files busy
    static WlanState wlanState;                  //! is wlan disconnected, connected etc....
    static MeasureState msgState;                //! which state ist the mesure
    static bool http_active;                     //! was an acces via http?
    static std::shared_ptr<env_dataset> dataset; //! set of mesures

  public:
    static void init();
    static void start();
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
