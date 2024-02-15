#pragma once
#include <inttypes.h>
#include <IPAddress.h>
#include <Udp.h>
#include <cJSON.h>

namespace EnvServer
{
  class UDBDataLog
  {
    private:
    UDP *_client;
    IPAddress _ip;
    uint16_t _port;
    const char *_serverName;
    const char *_appName;

    public:
    UDBDataLog();
    UDBDataLog( UDP &client, const IPAddress, uint16_t, const char *deviceHostname = "-", const char *appName = "-" );
    void setUdpDataLog( UDP &client, const IPAddress, uint16_t, const char *deviceHostname = "-", const char *appName = "-" );
    bool sendMeasure( const String & );
  };
}  // namespace EnvServer
