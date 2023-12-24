#include "common.hpp"
#include <WiFi.h>
#include <WiFiUdp.h>

namespace EnvServer
{
  logger::Elog elog;
  WiFiUDP udpClient;
}  // namespace EnvServer
