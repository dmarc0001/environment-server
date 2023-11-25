#pragma Once
#include <SPIFFS.h>
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
    static void init();   //! init http server
    static void start();  //! server.begin()
    static void stop();   //! server stop

    private:
    static void onIndex( AsyncWebServerRequest * );                               //! on index ("/" or "/index.html")
    static void onApiV1( AsyncWebServerRequest * );                               //! on url path "/api/v1/"
    static void onFilesReq( AsyncWebServerRequest * );                            //! on some file
    static void apiGetTodayData( AsyncWebServerRequest * );                       //! on api get today data
    static void apiGetWeekData( AsyncWebServerRequest * );                        //! on api get week data
    static void apiGetMonthData( AsyncWebServerRequest * );                       //! on api get month data
    static void apiSystemInfoGetHandler( AsyncWebServerRequest * );               //! deliver server info
    static void apiVersionInfoGetHandler( AsyncWebServerRequest * );              //! deliver esp infos
    static void apiRestHandlerInterval( AsyncWebServerRequest * );                //! deliver measure interval
    static void apiRestHandlerCurrent( AsyncWebServerRequest * );                 //! deliver acku current
    static void deliverFileToHttpd( String &, AsyncWebServerRequest * );          //! deliver content file via http
    static void handleNotPhysicFileSources( String &, AsyncWebServerRequest * );  //! handle virtual files/paths
    static String setContentTypeFromFile( String &, const String & );             //! find content type
    static void notFound( AsyncWebServerRequest * );
  };

}  // namespace EnvServer
