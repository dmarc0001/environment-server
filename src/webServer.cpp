#include <memory>
#include <cJSON.h>
#include <esp_chip_info.h>
#include "common.hpp"
#include "webServer.hpp"
#include "appPreferences.hpp"
#include "statusObject.hpp"
#include "version.hpp"

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
    logger.log( Prefs::LOGID, INFO, "%s: start webserver...", EnvWebServer::tag );
    // Cache responses for 1 minutes (60 seconds)
    EnvWebServer::server.serveStatic( "/", SPIFFS, Prefs::WEB_SITE_PATH ).setCacheControl( "max-age=60" );
    //
    // response filters
    //
    EnvWebServer::server.on( "/", HTTP_GET, EnvWebServer::onIndex );
    EnvWebServer::server.on( "/index\\.html", HTTP_GET, EnvWebServer::onIndex );
    EnvWebServer::server.on( "^\\/api\\/v1\\/set-(.*)\?(.*)$", HTTP_GET, EnvWebServer::onApiV1Set );
    EnvWebServer::server.on( "^\\/api\\/v1\\/(.*)$", HTTP_GET, EnvWebServer::onApiV1 );
    EnvWebServer::server.on( "^\\/.*$", HTTP_GET, EnvWebServer::onFilesReq );
    EnvWebServer::server.onNotFound( EnvWebServer::onNotFound );
    EnvWebServer::server.begin();
    logger.log( Prefs::LOGID, DEBUG, "%s: start webserver...OK", EnvWebServer::tag );
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
    String file( "/www/index.html" );
    StatusObject::setHttpActive( true );
    EnvWebServer::deliverFileToHttpd( file, request );
    // request->send( 200, "text/plain", "Hello, INDEX" );
  }

  /**
   * response for request a file (for a directory my fail)
   */
  void EnvWebServer::onFilesReq( AsyncWebServerRequest *request )
  {

    StatusObject::setHttpActive( true );
    String file( request->url() );
    EnvWebServer::deliverFileToHttpd( file, request );
  }

  /**
   * response for an api request, Version 1
   */
  void EnvWebServer::onApiV1( AsyncWebServerRequest *request )
  {
    StatusObject::setHttpActive( true );
    String parameter = request->pathArg( 0 );
    logger.log( Prefs::LOGID, DEBUG, "%s: api version 1 call <%s>", EnvWebServer::tag, parameter );
    if ( parameter.equals( "today" ) )
    {
      EnvWebServer::apiGetTodayData( request );
    }
    else if ( parameter.equals( "data" ) )
    {
      EnvWebServer::apiGetRestDataFileFrom( request );
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
    else if ( parameter.equals( "fscheck" ) )
    {
      EnvWebServer::apiRestFilesystemCheck( request );
    }
    else if ( parameter.equals( "fsstat" ) )
    {
      EnvWebServer::apiRestFilesystemStatus( request );
    }
    else
    {
      request->send( 300, "text/plain", "fail api call v1 for <" + parameter + ">" );
    }
    // TODO: implement loglevel change (Elog::globalSettings)
  }

  /**
   * compute set commands via API
   */
  void EnvWebServer::onApiV1Set( AsyncWebServerRequest *request )
  {
    StatusObject::setHttpActive( true );
    String verb = request->pathArg( 0 );
    String server, port;

    logger.log( Prefs::LOGID, DEBUG, "%s: api version 1 call set-%s", EnvWebServer::tag, verb );
    //
    // timezone set?
    //
    if ( verb.equals( "timezone" ) )
    {
      // timezone parameter find
      if ( request->hasParam( "timezone" ) )
      {
        String timezone = request->getParam( "timezone" )->value();
        logger.log( Prefs::LOGID, DEBUG, "%s: set-timezone, param: %s", EnvWebServer::tag, timezone.c_str() );
        Prefs::LocalPrefs::setTimeZone( timezone );
        request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + ">" );
        setenv( "TZ", timezone.c_str(), 1 );
        tzset();
        yield();
        sleep( 1 );
        ESP.restart();
        return;
      }
      else
      {
        logger.log( Prefs::LOGID, ERROR, "%s: set-timezone, param not found!", EnvWebServer::tag );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> param not found!" );
        return;
      }
    }
    else if ( verb.equals( "loglevel" ) )
    {
      // loglevel parameter find
      if ( request->hasParam( "level" ) )
      {
        String level = request->getParam( "level" )->value();
        logger.log( Prefs::LOGID, DEBUG, "%s: set-loglevel, param: %s", EnvWebServer::tag, level.c_str() );
        uint8_t numLevel = static_cast< uint8_t >( level.toInt() );
        Prefs::LocalPrefs::setLogLevel( numLevel );
        request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + ">" );
        yield();
        sleep( 1 );
        ESP.restart();
        return;
      }
      else
      {
        logger.log( Prefs::LOGID, ERROR, "%s: set-loglevel, param not found!", EnvWebServer::tag );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> param not found!" );
        return;
      }
    }
    else
    {
      request->send( 300, "text/plain", "fail api call v1 for <set-" + verb + ">" );
      return;
    }
    request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + "> RESTART CONTROLLER" );
    yield();
    sleep( 2 );
    ESP.restart();
  }
  /**
   * request for environment data for today
   */
  void EnvWebServer::apiGetTodayData( AsyncWebServerRequest *request )
  {
    logger.log( Prefs::LOGID, DEBUG, "%s: getTodayData...", EnvWebServer::tag );
    //
    // maybe ther are write accesses
    //
    if ( xSemaphoreTake( StatusObject::measureFileSem, pdMS_TO_TICKS( 1500 ) ) == pdTRUE )
    {
      String file( StatusObject::getTodayFileName() );
      EnvWebServer::deliverFileToHttpd( file, request );
      xSemaphoreGive( StatusObject::measureFileSem );
      return;
    }
    String msg = "Can't take semaphore!";
    EnvWebServer::onServerError( request, 303, msg );
  }

  void EnvWebServer::apiGetRestDataFileFrom( AsyncWebServerRequest *request )
  {
    {
      logger.log( Prefs::LOGID, DEBUG, "%s: apiGetRestDataFileFrom...", EnvWebServer::tag );
      if ( request->hasParam( "from" ) )
      {
        String dateNameStr = request->getParam( "from" )->value().substring( 0, 10 );
        String fileName( Prefs::WEB_DATA_PATH );
        fileName += dateNameStr;
        fileName += "-pressure.csv";
        logger.log( Prefs::LOGID, DEBUG, "%s: apiGetRestDataFileFrom try to deliver <%s>...", EnvWebServer::tag, fileName.c_str() );
        if ( SPIFFS.exists( fileName ) )
        {
          //
          // maybe their are write accesses
          if ( xSemaphoreTake( StatusObject::measureFileSem, pdMS_TO_TICKS( 1500 ) ) == pdTRUE )
          {
            EnvWebServer::deliverFileToHttpd( fileName, request );
            xSemaphoreGive( StatusObject::measureFileSem );
            return;
          }
          String msg = "Can't take semaphore!";
          EnvWebServer::onServerError( request, 303, msg );
          return;
        }
        String msg = "File <";
        msg += fileName.substring( 6 );
        msg += "> don't exist!";
        EnvWebServer::onServerError( request, 303, msg );
        return;
      }
      else
      {
        String msg = "no param <from> sent!";
        EnvWebServer::onServerError( request, 303, msg );
      }
    }
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
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject( root, "version", Prefs::VERSION );
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
    logger.log( Prefs::LOGID, DEBUG, "%s: request measure interval...", EnvWebServer::tag );
    cJSON *root = cJSON_CreateObject();
    char buffer[ 8 ];
    sprintf( &buffer[ 0 ], "%04d", Prefs::MEASURE_INTERVAL_SEC );
    cJSON_AddStringToObject( root, "interval", &buffer[ 0 ] );
    char *int_info = cJSON_PrintUnformatted( root );  // cJSON_Print(root);
    String info( int_info );
    request->send( 200, "application/json", info );
    cJSON_Delete( root );
    cJSON_free( int_info );  //!!!! important, memory leak
    logger.log( Prefs::LOGID, DEBUG, "%s: request interval  <%04d>...", EnvWebServer::tag, Prefs::MEASURE_INTERVAL_SEC );
  }

  /**
   * return json object for acku curent
   */
  void EnvWebServer::apiRestHandlerCurrent( AsyncWebServerRequest *request )
  {
    logger.log( Prefs::LOGID, DEBUG, "%s: request acku current...", EnvWebServer::tag );
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
    logger.log( Prefs::LOGID, DEBUG, "%s: request acku current <%f> done...", EnvWebServer::tag, cValue );
  }

  /**
   * reques an filesystemcheck
   */
  void EnvWebServer::apiRestFilesystemCheck( AsyncWebServerRequest *request )
  {
    logger.log( Prefs::LOGID, DEBUG, "%s: request trigger filesystemckeck...", EnvWebServer::tag );
    StatusObject::setFsCheckReq( true );
    request->send( 200, "application/json", "OK" );
  }

  void EnvWebServer::apiRestFilesystemStatus( AsyncWebServerRequest *request )
  {
    logger.log( Prefs::LOGID, DEBUG, "%s: request filesystem status...", EnvWebServer::tag );
    cJSON *root = cJSON_CreateObject();
    // total
    String val = String( StatusObject::getFsTotalSpace(), DEC );
    cJSON_AddStringToObject( root, "total", val.c_str() );
    // used space
    val = String( StatusObject::getFsUsedSpace() );
    cJSON_AddStringToObject( root, "used", val.c_str() );
    char *tmp_info = cJSON_PrintUnformatted( root );  // cJSON_Print(root);
    String info( tmp_info );
    request->send( 200, "application/json", info );
    cJSON_Delete( root );
    cJSON_free( tmp_info );  //!!!! important, memory leak
    logger.log( Prefs::LOGID, DEBUG, "%s: request filesystem status...OK", EnvWebServer::tag );
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
      logger.log( Prefs::LOGID, WARNING, "%s: SPIFFS not initialized, send file ABORT!", EnvWebServer::tag );
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
    logger.log( Prefs::LOGID, DEBUG, "%s: file <%s>: type: <%s>...", EnvWebServer::tag, filePath.c_str(), contentTypeMarker.c_str() );
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

  /**
   * handle non-physical files
   */
  void EnvWebServer::handleNotPhysicFileSources( String &filePath, AsyncWebServerRequest *request )
  {
    // TODO: implemtieren von virtuellen datenpdaden
    EnvWebServer::onNotFound( request );
  }

  /**
   * if there is an server error
   */
  void EnvWebServer::onServerError( AsyncWebServerRequest *request, int errNo, const String &msg )
  {
    StatusObject::setHttpActive( true );
    String myUrl( request->url() );
    logger.log( Prefs::LOGID, ERROR, "%s: Server ERROR: %03d - %s", EnvWebServer::tag, errNo, msg.c_str() );
    request->send( errNo, "text/plain", msg );
  }

  /**
   * File-Not-Fopuind Errormessage
   */
  void EnvWebServer::onNotFound( AsyncWebServerRequest *request )
  {
    StatusObject::setHttpActive( true );
    String myUrl( request->url() );
    logger.log( Prefs::LOGID, WARNING, "%s: url not found <%s>", EnvWebServer::tag, myUrl.c_str() );
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
