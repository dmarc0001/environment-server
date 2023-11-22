#include "webServer.hpp"
#include "statics.hpp"
#include "appPreferences.hpp"
#include "statusObject.hpp"

namespace EnvServer
{
  const char *EnvWebServer::tag{ "EnvWebServer" };

  AsyncWebServer EnvWebServer::server( 80 );

  void EnvWebServer::init()
  {
  }

  void EnvWebServer::start()
  {
    // Cache responses for 1 minutes (60 seconds)
    EnvWebServer::server.serveStatic( "/", SPIFFS, "/spiffs/" ).setCacheControl( "max-age=60" );
    EnvWebServer::server.on( "/", HTTP_GET, EnvWebServer::onIndex );
    EnvWebServer::server.on( "/index.html", HTTP_GET, EnvWebServer::onIndex );
    EnvWebServer::server.on( "^\\/api\\/v1\\/(month|week|today)$", HTTP_GET,
                             []( AsyncWebServerRequest *request ) { EnvWebServer::onApiV1( request ); } );

    // server.on( "/", HTTP_GET, []( AsyncWebServerRequest *request ) { request->send( 200, "text/plain", "Hello, world" ); } );

    // Send a GET request to <IP>/get?message=<message>
    // server.on( "/get", HTTP_GET,
    //            []( AsyncWebServerRequest *request )
    //            {
    //              String message;
    //              if ( request->hasParam( PARAM_MESSAGE ) )
    //              {
    //                message = request->getParam( PARAM_MESSAGE )->value();
    //              }
    //              else
    //              {
    //                message = "No message sent";
    //              }
    //              request->send( 200, "text/plain", "Hello, GET: " + message );
    //            } );

    // // Send a POST request to <IP>/post with a form field message set to <message>
    // server.on( "/post", HTTP_POST,
    //            []( AsyncWebServerRequest *request )
    //            {
    //              String message;
    //              if ( request->hasParam( PARAM_MESSAGE, true ) )
    //              {
    //                message = request->getParam( PARAM_MESSAGE, true )->value();
    //              }
    //              else
    //              {
    //                message = "No message sent";
    //              }
    //              request->send( 200, "text/plain", "Hello, POST: " + message );
    //            } );

    server.onNotFound( EnvWebServer::notFound );

    server.begin();

    EnvWebServer::server.begin();
  }

  void EnvWebServer::stop()
  {
    EnvWebServer::server.end();
  }

  void EnvWebServer::onIndex( AsyncWebServerRequest *request )
  {
    String file( Prefs::WEB_PATH );
    file += "/index.html";
    EnvWebServer::deliverFileToHttpd( file, request );
    // request->send( 200, "text/plain", "Hello, INDEX" );
  }

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

  void EnvWebServer::getTodayData( AsyncWebServerRequest *request )
  {
    FileList files{ std::string( Prefs::WEB_DAYLY_FILE_01 ), std::string( Prefs::WEB_DAYLY_FILE_02 ) };
    return EnvWebServer::deliverJdataFilesToHttpd( files, request );
  }

  /**
   * deliver a file (physic pathname on the controller) via http
   */
  void EnvWebServer::deliverFileToHttpd( String &filePath, AsyncWebServerRequest *request )
  {
    struct stat file_stat
    {
      0
    };
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
    // its hardcodet path ?
    // do this after file check, so i can overwrite this
    // behavior if an file is exist
    //
    if ( stat( filePath.c_str(), &file_stat ) == -1 )
    {
      return EnvWebServer::handleNotPhysicFileSources( filePath, request );
    }
    //
    // set content type of file
    //
    contentTypeMarker = EnvWebServer::setContentTypeFromFile( contentType, filePath );
    elog.log( DEBUG, "%s: file <%s>: type: <%s>...", EnvWebServer::tag, filePath.c_str(), contentTypeMarker.c_str() );
    //
    // content-type from file find and set
    //
    elog.log( INFO, "%s: sending file : <%s> (%ld bytes)...", EnvWebServer::tag, filePath.c_str(), file_stat.st_size );

    //
    // make a chunked output
    //
    AsyncWebServerResponse *response =
        request->beginChunkedResponse( contentType,
                                       [ &contentTypeMarker, filePath ]( uint8_t *buffer, size_t maxLen, size_t index ) -> size_t
                                       {
                                         // Write up to "maxLen" bytes into "buffer" and return the
                                         // amount written. index equals the amount of bytes that have
                                         // been already sent You will be asked for more data until 0 is
                                         // returned Keep in mind that you can not delay or yield
                                         // waiting for more data!
                                         return EnvWebServer::readFromPhysicFile( contentTypeMarker, filePath, buffer, maxLen );
                                       } );
    elog.log( DEBUG, "%s: add header and send...", EnvWebServer::tag );
    response->addHeader( "Server", "ESP32 Environment Server" );
    if ( contentTypeMarker.equals( "js.gz" ) )
    {
      // i need this für zipped js lib
      response->addHeader( "Content-Encoding", "gzip" );
    }
    request->send( response );
  }

  /**
   * read chunks from physical file
   */
  size_t EnvWebServer::readFromPhysicFile( String &contentTypeMarker, const String &filePath, uint8_t *buffer, size_t maxLen )
  {
    static FILE *fd{ nullptr };
    size_t chunksize{ 0 };

    //
    // is a file opened?
    //
    if ( !fd )
    {
      //
      // then open the destination file
      //
      if ( contentTypeMarker.equals( "js.gz" ) )
      {
        fd = fopen( filePath.c_str(), "rb" );
      }
      else
      {
        fd = fopen( filePath.c_str(), "ra" );
      }
      if ( !fd )
      {
        elog.log( ERROR, "%s: failed to read existing file : <%s>, ABORT!", EnvWebServer::tag, filePath.c_str() );
        /* Respond with 500 Internal Server Error */
        // request->send( 500, "text/plain", "can't read file" );
        fd = nullptr;
        return 0;
      }
    }
    elog.log( DEBUG, "%s: opened file <%s>!", EnvWebServer::tag, filePath.c_str() );
    // check buffer to write
    if ( buffer == nullptr )
    {
      elog.log( ERROR, "%s: buffer not allocatred. ABORT!", EnvWebServer::tag );
      return 0;
    }
    elog.log( DEBUG, "%s: readFromPhysicFile, type: %s, maxlen: %04d, ...", EnvWebServer::tag, contentTypeMarker, maxLen );
    //
    //  chcek if data special
    //
    if ( contentTypeMarker.equals( "jdata" ) )
    {
      //
      // jdata need som extra work
    }
    else
    {
      //
      // read data a chuink while eof or buffer full
      //
      elog.log( DEBUG, "%s: readFromPhysicFile fread...", EnvWebServer::tag );
      delay( 200 );
      chunksize = fread( buffer, 1, maxLen, fd );
      if ( chunksize == 0 )
      {
        fclose( fd );
        fd = nullptr;
        elog.log( DEBUG, "%s: readFromPhysicFile file end, close file...", EnvWebServer::tag );
      }
      else
      {
        elog.log( DEBUG, "%s: readFromPhysicFile fread, read %04d bytes", EnvWebServer::tag, chunksize );
      }
      delay( 200 );
    }
    return chunksize;
  }

  void EnvWebServer::handleNotPhysicFileSources( String &filePath, AsyncWebServerRequest *request )
  {
    // TODO: implemtieren von virtuellen daten
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

  void EnvWebServer::getWeekData( AsyncWebServerRequest *request )
  {
  }

  void EnvWebServer::getMonthData( AsyncWebServerRequest *request )
  {
  }

  void EnvWebServer::notFound( AsyncWebServerRequest *request )
  {
    String myUrl( request->url() );
    elog.log( WARNING, "%s: url not found <%s>", EnvWebServer::tag, myUrl );
    request->send( 404, "text/plain", "URL not found: <" + myUrl + ">" );
  }

}  // namespace EnvServer
