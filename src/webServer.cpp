#include <memory>
#include <cJSON.h>
#include <esp_chip_info.h>
#include "webServer.hpp"
#include "statics.hpp"
#include "appPreferences.hpp"
#include "statusObject.hpp"

namespace EnvServer
{
  const char *EnvWebServer::tag{ "EnvWebServer" };
  // instantiate a webserver
  AsyncWebServer EnvWebServer::server( 80 );

  /**
   * ini a few things?
   */
  void EnvWebServer::init()
  {
    //
    // maybe a few things to init?
    //
    StatusObject::init();
  }

  /**
   * start the webserver service
   */
  void EnvWebServer::start()
  {
    elog.log( INFO, "%s: start webserver...", EnvWebServer::tag );
    // Cache responses for 1 minutes (60 seconds)
    EnvWebServer::server.serveStatic( "/", SPIFFS, "/spiffs/" ).setCacheControl( "max-age=60" );
    //
    // response filters
    //
    EnvWebServer::server.on( "/", HTTP_GET, EnvWebServer::onIndex );
    EnvWebServer::server.on( "/index\\.html", HTTP_GET, EnvWebServer::onIndex );
    // EnvWebServer::server.on( "^\\/api\\/v1\\/(month|week|today|version|interval|current)$", HTTP_GET, EnvWebServer::onApiV1 );
    EnvWebServer::server.on( "^\\/api\\/v1\\/(.*)$", HTTP_GET, EnvWebServer::onApiV1 );
    EnvWebServer::server.on( "^\\/.*$", HTTP_GET, EnvWebServer::onFilesReq );
    EnvWebServer::server.onNotFound( EnvWebServer::notFound );
    EnvWebServer::server.begin();
    elog.log( DEBUG, "%s: start webserver...OK", EnvWebServer::tag );
  }

  /**
   * stop the webserver service comlete
   */
  void EnvWebServer::stop()
  {
    EnvWebServer::server.reset();
    EnvWebServer::server.end();
  }

  /**
   * response for index request
   */
  void EnvWebServer::onIndex( AsyncWebServerRequest *request )
  {
    String file( "/index.html" );
    EnvWebServer::deliverFileToHttpd( file, request );
    // request->send( 200, "text/plain", "Hello, INDEX" );
  }

  /**
   * response for request a file (for a directory my fail)
   */
  void EnvWebServer::onFilesReq( AsyncWebServerRequest *request )
  {
    String file( request->url() );
    EnvWebServer::deliverFileToHttpd( file, request );
  }

  /**
   * response for an api request, Version 1
   */
  void EnvWebServer::onApiV1( AsyncWebServerRequest *request )
  {
    String parameter = request->pathArg( 0 );
    elog.log( DEBUG, "%s: api version 1 call <%s>", EnvWebServer::tag, parameter );
    if ( parameter.equals( "today" ) )
    {
      EnvWebServer::apiGetTodayData( request );
    }
    else if ( parameter.equals( "week" ) )
    {
      EnvWebServer::apiGetWeekData( request );
    }
    else if ( parameter.equals( "month" ) )
    {
      EnvWebServer::apiGetMonthData( request );
    }
    else if ( parameter.equals( "interval" ) )
    {
      EnvWebServer::apiRestHandlerInterval( request );
    }
    else if ( parameter.equals( "version" ) )
    {
      EnvWebServer::apiVersionInfoGetHandler( request );
    }
    else if ( parameter.equals( "info" ) )
    {
      EnvWebServer::apiSystemInfoGetHandler( request );
    }
    else if ( parameter.equals( "current" ) )
    {
      EnvWebServer::apiRestHandlerCurrent( request );
    }
    else
    {
      request->send( 300, "text/plain", "fail api call v1 for <" + parameter + ">" );
    }
  }

  /**
   * request for environment data for today
   */
  void EnvWebServer::apiGetTodayData( AsyncWebServerRequest *request )
  {
    String file( Prefs::WEB_DAYLY_FILE );
    elog.log( DEBUG, "%s: getTodayData...", EnvWebServer::tag );
    return EnvWebServer::deliverFileToHttpd( file, request );
    // deliverJdataFilesToHttpd( file, request );
  }

  /**
   * get json data for last week
   */
  void EnvWebServer::apiGetWeekData( AsyncWebServerRequest *request )
  {
    // TODO: for future use implement!
    String file( Prefs::WEB_DAYLY_FILE );
    return EnvWebServer::deliverFileToHttpd( file, request );
  }

  /**
   * get json data for last month
   */
  void EnvWebServer::apiGetMonthData( AsyncWebServerRequest *request )
  {
    // TODO: for future use implement!
    String file( Prefs::WEB_DAYLY_FILE );
    return EnvWebServer::deliverFileToHttpd( file, request );
  }

  void EnvWebServer::apiSystemInfoGetHandler( AsyncWebServerRequest *request )
  {
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info( &chip_info );
    cJSON_AddStringToObject( root, "version", IDF_VER );
    cJSON_AddNumberToObject( root, "cores", chip_info.cores );
    const char *sys_info = cJSON_Print( root );
    String info( sys_info );
    request->send( 200, "application/json", info );
    free( ( void * ) sys_info );
    cJSON_Delete( root );
  }

  /**
   * ask for software vesion
   */
  void EnvWebServer::apiVersionInfoGetHandler( AsyncWebServerRequest *request )
  {
    String version( "ESP32" );
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject( root, "version", version.c_str() );
    const char *sys_info = cJSON_Print( root );
    String info( sys_info );
    request->send( 200, "application/json", info );
    free( ( void * ) sys_info );
    cJSON_Delete( root );
  }
  /**
   * return jsom object with measure interval
   */
  void EnvWebServer::apiRestHandlerInterval( AsyncWebServerRequest *request )
  {
    elog.log( DEBUG, "%s: request measure interval...", EnvWebServer::tag );
    cJSON *root = cJSON_CreateObject();
    char buffer[ 8 ];
    sprintf( &buffer[ 0 ], "%04d", Prefs::MEASURE_INTERVAL_SEC );
    cJSON_AddStringToObject( root, "interval", &buffer[ 0 ] );
    char *int_info = cJSON_PrintUnformatted( root );  // cJSON_Print(root);
    String info( int_info );
    request->send( 200, "application/json", info );
    cJSON_Delete( root );
    cJSON_free( int_info );  //!!!! important, memory leak
    elog.log( DEBUG, "%s: request interval  <%04d>...", EnvWebServer::tag, Prefs::MEASURE_INTERVAL_SEC );
  }

  /**
   * return json object for acku curent
   */
  void EnvWebServer::apiRestHandlerCurrent( AsyncWebServerRequest *request )
  {
    elog.log( DEBUG, "%s: request acku current...", EnvWebServer::tag );
    cJSON *root = cJSON_CreateObject();
    char buffer[ 16 ];
    float cValue{ static_cast< float >( StatusObject::getVoltage() ) / 1000.0F };
    sprintf( &buffer[ 0 ], "%1.3f", cValue );
    cJSON_AddStringToObject( root, "current", &buffer[ 0 ] );
    char *tmp_info = cJSON_PrintUnformatted( root );  // cJSON_Print(root);
    String info( tmp_info );
    request->send( 200, "application/json", info );
    cJSON_Delete( root );
    cJSON_free( tmp_info );  //!!!! important, memory leak
    elog.log( DEBUG, "%s: request acku current <%f> done...", EnvWebServer::tag, cValue );
  }

  /**
   * deliver a file (physic pathname on the controller) via http
   */
  void EnvWebServer::deliverFileToHttpd( String &filePath, AsyncWebServerRequest *request )
  {
    String contentType( "text/plain" );
    String contentTypeMarker{ 0 };

    if ( !StatusObject::getIsSpiffsOkay() )
    {
      elog.log( WARNING, "%s: SPIFFS not initialized, send file ABORT!", EnvWebServer::tag );
      request->send( 500, "text/plain", "SPIFFS not initialized" );
      return;
    }
    //
    // next check if filename not exits
    // do this after file check, so i can overwrite this
    // behavior if an file is exist
    //
    if ( !SPIFFS.exists( filePath ) )
    {
      return EnvWebServer::handleNotPhysicFileSources( filePath, request );
    }
    //
    // set content type of file
    //
    contentTypeMarker = EnvWebServer::setContentTypeFromFile( contentType, filePath );
    elog.log( DEBUG, "%s: file <%s>: type: <%s>...", EnvWebServer::tag, filePath.c_str(), contentTypeMarker.c_str() );
    //
    // send via http server, he mak this chunked if need
    //
    AsyncWebServerResponse *response = request->beginResponse( SPIFFS, filePath, contentType, false );
    response->addHeader( "Server", "ESP Environment Server" );
    if ( contentTypeMarker.equals( "js.gz" ) )
    {
      response->addHeader( "Content-Encoding", "gzip" );
    }
    request->send( response );
  }

  void EnvWebServer::handleNotPhysicFileSources( String &filePath, AsyncWebServerRequest *request )
  {
    // TODO: implemtieren von virtuellen daten
    EnvWebServer::notFound( request );
  }

  void EnvWebServer::notFound( AsyncWebServerRequest *request )
  {
    String myUrl( request->url() );
    elog.log( WARNING, "%s: url not found <%s>", EnvWebServer::tag, myUrl.c_str() );
    request->send( 404, "text/plain", "URL not found: <" + myUrl + ">" );
  }

  /**
   * find out what is the content type fron the file extension
   */
  String EnvWebServer::setContentTypeFromFile( String &contentType, const String &filename )
  {
    String type = "text";

    if ( filename.endsWith( ".pdf" ) )
    {
      type = "pdf";
      contentType = "application/pdf";
      return type;
    }
    if ( filename.endsWith( ".html" ) )
    {
      type = "html";
      contentType = "text/html";
      return type;
    }
    if ( filename.endsWith( ".jpeg" ) )
    {
      type = "jpeg";
      contentType = "image/jpeg";
      return type;
    }
    if ( filename.endsWith( ".ico" ) )
    {
      type = "icon";
      contentType = "image/x-icon";
      return type;
    }
    if ( filename.endsWith( ".json" ) )
    {
      type = "json";
      contentType = "application/json";
      return type;
    }
    if ( filename.endsWith( ".jdata" ) )
    {
      // my own marker for my "raw" fileformat
      type = "jdata";
      contentType = "application/json";
      return type;
    }
    if ( filename.endsWith( "js.gz" ) )
    {
      type = "js.gz";
      contentType = "text/javascript";
      return type;
    }
    if ( filename.endsWith( ".js" ) )
    {
      type = "js";
      contentType = "text/javascript";
      return type;
    }
    if ( filename.endsWith( ".css" ) )
    {
      type = "css";
      contentType = "text/css";
      return type;
    }
    //
    // This is a limited set only
    // For any other type always set as plain text
    //
    contentType = "text/plain";
    return type;
  }

}  // namespace EnvServer
