#include "webServer.hpp"

namespace EnvServer
{
  const char *EnvWebServer::tag{ "EnvWebServer" };

  AsyncWebServer EnvWebServer::server( 80 );

  void EnvWebServer::init()
  {
  }

  void EnvWebServer::start()
  {
    server.on( "/", HTTP_GET, []( AsyncWebServerRequest *request ) { request->send( 200, "text/plain", "Hello, world" ); } );

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

  void EnvWebServer::notFound( AsyncWebServerRequest *request )
  {
    request->send( 404, "text/plain", "Not found" );
  }

}  // namespace EnvServer
