#pragma once
#include <inttypes.h>
#include <IPAddress.h>
#include <Udp.h>
#include <cJSON.h>

namespace EnvServer
{
  class UDBDataSend
  {
    private:
    UDP *_client;
    IPAddress _ip;
    const char *_server;
    uint16_t _port;
    const char *_deviceHostname;
    const char *_appName;

    public:
    UDBDataSend( const char *server, uint16_t port, const char *deviceHostname = "-", const char *appName = "-" );
    bool sendMeasure( cJSON *dataSetObj );
  }
}  // namespace EnvServer
