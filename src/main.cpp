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
  //
  // TODO: fake dass alles bereit ist
  //
  // fakeReady();
}

void loop()
{
  static uint16_t counter = 0;
  static uint32_t nextTime{ millis() + 800 };
  // next time logger time sync
  static unsigned long setNextTimeCorrect{ ( millis() * 1000UL * 21600UL ) };

  EnvServer::WifiConfig::loop();
  // if ( nextTime < millis() )
  // {
  //   LEDTest();
  //   nextTime = millis() + 1000;
  // }

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

void LEDTest()
{
  static uint8_t counter = 0;
  static uint8_t led = 0;
  CRGB local_color{};

  // wait for a second
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
  EnvServer::LEDStripe::setLed( led, local_color );
}

void fakeReady()
{
  using namespace EnvServer;
  // after start measure thrad!!!!
  elog.log( WARNING, "main: fake system stati..." );
  StatusObject::setMeasureState( MeasureState::MEASURE_NOMINAL );
}
