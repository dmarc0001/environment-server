/*
  main, hier startet es
*/
#include <Elog.h>
#include "common.hpp"
#include "main.hpp"
#include "statics.hpp"
#include "ledStripe.hpp"
#include "ackuRead.hpp"
#include "tempMeasure.hpp"
#include "statusObject.hpp"
#include "wifiConfig.hpp"
#include "ledColorsDefinition.hpp"

// Set LED_BUILTIN if it is not defined by Arduino framework
// #define LED_BUILTIN 13

void setup()
{
  using namespace EnvServer;

  // Debug Ausgabe initialisieren
  Serial.begin( 115200 );
  Serial.println( "main: program started..." );
  elog.addSerialLogging( Serial, "MAIN", DEBUG );  // Enable serial logging. We want only INFO or lower logleve.
  elog.log( INFO, "main: start with logging..." );
  // set my timezone, i deal with timestamps
  elog.log( DEBUG, "main: set timezone (%s)...", Prefs::TIMEZONE );
  setenv( "TZ", Prefs::TIMEZONE, 1 );
  tzset();
  elog.log( DEBUG, "main: init LED Stripe..." );
  LEDStripe::init();
  CRGB color( Prefs::LED_COLOR_BLACK );
  EnvServer::LEDStripe::setLed( Prefs::LED_ALL, color );
  elog.log( DEBUG, "main: init statusobject..." );
  StatusObject::init();
  elog.log( DEBUG, "main: init acku watch..." );
  AckuVoltage::start();
  elog.log( DEBUG, "main: start sensor watching..." );
  TempMeasure::start();
  elog.log( DEBUG, "main: start wifi..." );
  WifiConfig::init();
}

void loop()
{
  static uint16_t counter = 0;
  // next time logger time sync
  static unsigned long setNextTimeCorrect{ ( millis() * 1000UL * 21600UL ) };

  //
  // for webserver
  //
  // EnvServer::WifiConfig::wm.process();

  if ( setNextTimeCorrect < millis() )
  {
    EnvServer::elog.log( DEBUG, "main: logger time correction..." );
    setNextTimeCorrect = ( millis() * 1000UL * 21600UL );
    struct tm ti;
    if ( !getLocalTime( &ti ) )
    {
      EnvServer::elog.log( WARNING, "main: failed to obtain system time!" );
    }
    else
    {
      EnvServer::elog.log( DEBUG, "main: gotten system time!" );
      Elog::provideTime( ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec );
      EnvServer::elog.log( DEBUG, "main: logger time correction...OK" );
    }
  }

  // Elog::provideTime( 2023, 7, 15, 8, 12, 34 );  // We make up the time: 15th of july 2023 at 08:12:34
}
