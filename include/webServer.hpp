#pragma Once

#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"

namespace EnvServer
{

  class EnvWebServer
  {
    private:
    static const char *tag;        //! for debugging
    static AsyncWebServer server;  //! webserver ststic

    public:
    static void init();
    static void start();  // server.begin()

    private:
    static void notFound( AsyncWebServerRequest * );
  };

}  // namespace EnvServer
