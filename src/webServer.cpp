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
    EnvWebServer::server.on( "^\\/api\\/v1\\/(month|week|today)$", HTTP_GET, EnvWebServer::onApiV1 );
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
    String timearea = request->pathArg( 0 );
    elog.log( DEBUG, "%s: api version 1 call <%s>", EnvWebServer::tag, timearea );
    if ( timearea == "today" )
    {
      EnvWebServer::getTodayData( request );
    }
    else if ( timearea == "week" )
    {
      EnvWebServer::getWeekData( request );
    }
    else if ( timearea == "month" )
    {
      EnvWebServer::getMonthData( request );
    }
    else
    {
      request->send( 300, "text/plain", "fail api call v1 for <" + timearea + ">" );
    }
  }

  /**
   * request for environment data for today
   */
  void EnvWebServer::getTodayData( AsyncWebServerRequest *request )
  {
    FileList files{ std::string( Prefs::WEB_DAYLY_FILE_01 ), std::string( Prefs::WEB_DAYLY_FILE_02 ) };
    return EnvWebServer::deliverJdataFilesToHttpd( files, request );
  }

  /**
   * get json data for last week
   */
  void EnvWebServer::getWeekData( AsyncWebServerRequest *request )
  {
  }

  /**
   * get json data for last month
   */
  void EnvWebServer::getMonthData( AsyncWebServerRequest *request )
  {
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
    request->send( SPIFFS, filePath, contentType, false );
  }

  void EnvWebServer::handleNotPhysicFileSources( String &filePath, AsyncWebServerRequest *request )
  {
    // TODO: implemtieren von virtuellen daten
    EnvWebServer::notFound( request );
  }

  void EnvWebServer::deliverJdataFilesToHttpd( FileList &, AsyncWebServerRequest * )
  {
    uint8_t filesCount{ 0 };
    //
    // at first: is SPIFFS ready?
    //
    // if ( !is_spiffs_ready )
    // {
    //   httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "SPIFFS not ready!" );
    //   return ESP_FAIL;
    // }
    // //
    // // set content type of file
    // //
    // httpd_resp_set_type( req, "application/json" );
    // //
    // // overall files
    // //
    // for ( auto readFile : files )
    // {
    //   size_t currentChunkSize{ 0 };
    //   auto fd = fopen( readFile.c_str(), "r" );
    //   if ( !fd )
    //   {
    //     if ( filesCount == 0 )
    //     {
    //       ESP_LOGE( WebServer::tag, "json-data: failed to read file : %s", readFile.c_str() );
    //       /* Respond with 500 Internal Server Error */
    //       std::string errMsg( "Failed to read existing file: " );
    //       errMsg.append( readFile );
    //       httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, errMsg.c_str() );
    //       return ESP_FAIL;
    //     }
    //     else
    //     {
    //       ESP_LOGE( WebServer::tag, "json-data: failed to read file : %s", readFile.c_str() );
    //       continue;
    //     }
    //   }
    //   if ( filesCount == 0 )
    //   {
    //     // JSON Array opener
    //     const char *startPtr{ "[\n" };
    //     httpd_resp_send_chunk( req, startPtr, 2 );
    //   }
    //   else
    //   {
    //     // first file was send, need a comma
    //     const char *commaPtr{ "," };
    //     httpd_resp_send_chunk( req, commaPtr, 1 );
    //   }
    //   // the whole file...
    //   do
    //   {
    //     std::unique_ptr< char > currentBuf( new char[ Prefs::WEB_SCRATCH_BUFSIZE ] );
    //     char *currentBufferPtr = currentBuf.get();
    //     // try to read the next chunk
    //     char *currentReadetBufferPtr = fgets( currentBufferPtr, Prefs::WEB_SCRATCH_BUFSIZE - 2, fd );
    //     if ( currentReadetBufferPtr )
    //     {
    //       // readed a line...
    //       // was there a line before?
    //       if ( currentChunkSize > 0 )
    //       {
    //         // last round was a line
    //         std::string myLine( "," );
    //         myLine.append( currentBufferPtr );
    //         currentChunkSize = myLine.size();
    //         httpd_resp_send_chunk( req, myLine.c_str(), currentChunkSize );
    //       }
    //       else
    //       {
    //         // this is the first line
    //         std::string myLine( currentBufferPtr );
    //         currentChunkSize = myLine.size();
    //         httpd_resp_send_chunk( req, myLine.c_str(), currentChunkSize );
    //       }
    //     }
    //     else
    //     {
    //       // not a line read, EOF
    //       currentChunkSize = 0;
    //     }
    //   } while ( currentChunkSize > 0 );
    //   fclose( fd );
    //   //
    //   // cont files
    //   //
    //   ++filesCount;
    // }
    // // JSON Array closener
    // const char *endPtr{ "]\n" };
    // httpd_resp_send_chunk( req, endPtr, 2 );
    // // end of transmussion
    // httpd_resp_sendstr_chunk( req, nullptr );
    // return ESP_OK;
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
