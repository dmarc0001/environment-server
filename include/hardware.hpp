#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

namespace rest_webserver
{
  class Hardware
  {
  private:
    static const char *tag;
    static bool curr_led;

  public:
    static esp_err_t init();
    static void sw_led(bool);
    static void toggle_led();
  };
}
