/*
  main, hier startet es
*/
#include "common.hpp"
#include "main.hpp"
#include "statics.hpp"
#include "ledStripe.hpp"
#include "ackuRead.hpp"

// Set LED_BUILTIN if it is not defined by Arduino framework
// #define LED_BUILTIN 13

void setup()
{
  using namespace EnvServer;

  // Debug Ausgabe initialisieren
  Serial.begin( 115200 );
  Serial.println( "program started..." );
  elog.addSerialLogging( Serial, "MAIN", DEBUG );  // Enable serial logging. We want only INFO or lower logleve.
  elog.log( ALERT, "start with logging..." );
  LEDStripe::init();
  EnvServer::LEDStripe::setLed( Prefs::LED_ALL, 0, 0, 0 );
  AckuVoltage::init();
}

void loop()
{
  static uint8_t counter = 0;
  static uint8_t ackucounter = 0;
  static uint8_t led = 0;
  CRGB local_color{};

  // wait for a second
  delay( 1000 );
  // turn the LED off by making the voltage LOW
  // EnvServer::elog.log( DEBUG, "tick..." );
  switch ( counter )
  {
    case 0:
      local_color.r = 255;
      local_color.g = 0;
      local_color.b = 0;
      ++counter;
      break;

    case 1:
      local_color.r = 0;
      local_color.g = 255;
      local_color.b = 0;
      ++counter;
      break;

    case 2:
      local_color.r = 0;
      local_color.g = 0;
      local_color.b = 255;
      ++counter;
      break;

    default:
      counter = 0;
      EnvServer::LEDStripe::setLed( led, 0, 0, 0 );
      ++led;
      if ( led >= Prefs::LED_STRIPE_COUNT )
      {
        led = 0;
      }
      break;
  }
  // EnvServer::elog.log( DEBUG, "led %1d, counter %02d, color r: %03d g:%03d b:%03d...", led, counter, local_color.r, local_color.g,
  //                      local_color.b );
  EnvServer::LEDStripe::setLed( led, local_color );
  if ( ++ackucounter > 4 )
  {
    float temp = EnvServer::AckuVoltage::readValue();
    EnvServer::elog.log( DEBUG, "acku %02.1f Volt...", temp );
    ackucounter = 0;
  }
}
