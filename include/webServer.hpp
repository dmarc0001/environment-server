#pragma Once
#include <list>
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"

namespace EnvServer
{
  using FileList = std::list< std::string >;

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
    static void getTodayData( AsyncWebServerRequest * );                          //! on api get today data
    static void getWeekData( AsyncWebServerRequest * );                           //! on api get week data
    static void getMonthData( AsyncWebServerRequest * );                          //! on api get month data
    static void deliverFileToHttpd( String &, AsyncWebServerRequest * );          //! deliver content file via http
    static void deliverJdataFilesToHttpd( FileList &, AsyncWebServerRequest * );  //! deliber my json file via http
    static void handleNotPhysicFileSources( String &, AsyncWebServerRequest * );  //! handle virtual files/paths
    static String setContentTypeFromFile( String &, const String & );             //! find content type
    static void notFound( AsyncWebServerRequest * );
  };

}  // namespace EnvServer
