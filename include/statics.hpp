#pragma once
#include "common.hpp"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "udpDataLog.hpp"

namespace EnvServer
{
  extern logger::Elog elog;
  extern WiFiUDP udpClient;
  extern UDBDataLog dataLog;

}  // namespace EnvServer
