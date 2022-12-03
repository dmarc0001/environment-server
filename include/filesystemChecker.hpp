#pragma once
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <ds18x20.h>
#include <led_strip.h>
#include "appPreferences.hpp"
#include "statusObject.hpp"

namespace webserver
{
  class FsCheckObject
  {
  private:
    static const char *tag; //! TAG for esp log
    static bool is_init;    //! was object initialized
    static bool is_running; //! is save Task running?

  public:
    static void init();
    static void start();

  private:
    static void filesystemTask(void *);
  };
}
