#pragma once
#include <memory>
#include <inttypes.h>
#include <IPAddress.h>
#include <Udp.h>
#include <cJSON.h>
#include "appPreferences.hpp"

namespace EnvServer
{
  class UDPDataLog
  {
    private:
    static const char *tag;
    static bool isRunning;
    UDP *_client;
    IPAddress _ip;
    uint16_t _port;
    const char *_appName;

    public:
    static std::shared_ptr< env_dataset > dataset;  //! set of mesures

    public:
    UDPDataLog();
    UDPDataLog( UDP &client, const IPAddress, uint16_t, const char *appName = "-" );
    void setUdpDataLog( UDP &client, const IPAddress, uint16_t, const char *appName = "-" );

    private:
    void start();
    static void saveTask( void * );
  };
}  // namespace EnvServer
