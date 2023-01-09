#include <inttypes.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ds18x20.h>
#include <led_strip.h>
#include <esp_log.h>
#include <esp_err.h>
#include "appPreferences.hpp"

namespace webserver
{
  class TempMeasure
  {
    private:
    static const char *tag;

    public:
    static void start();
    static TaskHandle_t taskHandle;

    private:
    static void measureTask( void * );
  };

}  // namespace webserver
