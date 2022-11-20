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

  enum MsgState : uint8_t
  {
    MSG_NOMINAL,
    MSG_WARN,
    MSG_ERR,
    MSG_CRIT
  };

}
