#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ds18x20.h>

namespace rest_webserver
{

  enum WlanState : uint8_t
  {
    DISCONNECTED,
    SEARCHING,
    CONNECTED,
    TIMESYNCED,
    FAILED
  };

  enum MsgState : uint8_t
  {
    MSG_NOMINAL,
    MSG_WARN,
    MSG_ERR,
    MSG_CRIT
  };

  struct env_measure_t
  {
    uint32_t timestamp;
    ds18x20_addr_t addr;
    float temp;
    float humidy;
  };

  using env_measure_a = env_measure_t[];

}
