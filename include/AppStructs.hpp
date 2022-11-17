#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace rest_webserver
{

  enum WlanState : uint8_t
  {
    DISCONNECTED,
    SEARCHING,
    CONNECTED,
    FAILED
  };

}
