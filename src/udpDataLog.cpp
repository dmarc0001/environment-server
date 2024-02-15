#include "common.hpp"
#include "udpDataLog.hpp"

namespace EnvServer
{
  UDBDataLog::UDBDataLog(){};

  UDBDataLog::UDBDataLog( UDP &client, const IPAddress addr, uint16_t port, const char *deviceHostname, const char *appName )
      : _client( &client ), _ip( addr ), _port( port ), _serverName( deviceHostname ), _appName( appName )
  {
  }

  void UDBDataLog::setUdpDataLog( UDP &client, const IPAddress addr, uint16_t port, const char *deviceHostname, const char *appName )
  {
    _client = &client;
    _ip = addr;
    _port = port;
    _serverName = deviceHostname;
    _appName = appName;
  }

  bool UDBDataLog::sendMeasure( const String & )
  {
    // TODO: implement
    return true;
  }
}  // namespace EnvServer