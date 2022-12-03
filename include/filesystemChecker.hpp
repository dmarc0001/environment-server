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
    static void start(); //! start the task if is not running

  private:
    static void init();                        //! init vars etc
    static void filesystemTask(void *);        // the ininite task
    static void computeFilesysCheck(uint32_t); //! compute the filesystem
    static uint32_t getMidnight(uint32_t);     //! give last midnight time in sec since 01.01.1970 (up to 2038)
  };
}
