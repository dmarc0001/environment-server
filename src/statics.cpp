#include "common.hpp"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "udpDataLog.hpp"

namespace EnvServer
{
  logger::Elog elog;
  WiFiUDP udpClient;
  UDBDataLog dataLog;
}  // namespace EnvServer
