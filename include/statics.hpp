#pragma once
#include "common.hpp"
#include <WiFi.h>
#include <WiFiUdp.h>

namespace EnvServer
{
  extern logger::Elog elog;
  extern WiFiUDP udpClient;

}  // namespace EnvServer
