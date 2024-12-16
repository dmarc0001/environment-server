/*
  main, hier startet es
*/
#include "common.hpp"
#include "main.hpp"
#include "ledStripe.hpp"
#include "ackuRead.hpp"
#include "tempMeasure.hpp"
#include "statusObject.hpp"
#include "wifiConfig.hpp"
#include "ledColorsDefinition.hpp"
#include "webServer.hpp"
#include "filesystemChecker.hpp"

// Set LED_BUILTIN if it is not defined by Arduino framework
// #define LED_BUILTIN 13

void setup()
{
  using namespace EnvServer;

  // Debug Ausgabe initialisieren
  Serial.begin( 115200 );
  Serial.println( "main: program started..." );
  // read persistent config
  Serial.println( "main: init local preferences..." );
  // Elog &elog = Elog::getInstance();
  Prefs::LocalPrefs::init();
  // correct loglevel
  LogLevel level = static_cast< LogLevel >( Prefs::LocalPrefs::getLogLevel() );
  // LogLevel level = DEBUG;
  // elog = Elog::getInstance();
  logger.registerSerial( Prefs::LOGID, level, "ENV", Serial, FLAG_TIME_LONG | FLAG_SERVICE_LONG );
  logger.log( Prefs::LOGID, INFO, "main: start with logging..." );
  // set my timezone, i deal with timestamps
  logger.log( Prefs::LOGID, DEBUG, "main: set timezone (%s)...", Prefs::LocalPrefs::getTimeZone().c_str() );
  setenv( "TZ", Prefs::LocalPrefs::getTimeZone().c_str(), 1 );
  tzset();
  static String hName( Prefs::LocalPrefs::getHostName() );
  logger.log( Prefs::LOGID, INFO, "main: hostname: <%s>...", hName.c_str() );
  logger.log( Prefs::LOGID, DEBUG, "main: init LED Stripe..." );
  LEDStripe::init();
  logger.log( Prefs::LOGID, DEBUG, "main: init statusobject..." );
  StatusObject::init();
  logger.log( Prefs::LOGID, DEBUG, "main: init acku watch..." );
  AckuVoltage::start();
  logger.log( Prefs::LOGID, DEBUG, "main: start sensor watching..." );
  TempMeasure::start();
  logger.log( Prefs::LOGID, DEBUG, "main: start wifi..." );
  WifiConfig::init();
  logger.log( Prefs::LOGID, DEBUG, "main: start filesystemchecker..." );
  FsCheckObject::start();
}

void loop()
{
  using namespace EnvServer;

  static uint16_t counter = 0;
  // next time logger time sync
  static unsigned long setNextTimeCorrect{ ( millis() * 1000UL * 21600UL ) };
  static auto connected = WlanState::DISCONNECTED;

  //
  // for webserver
  //
  // EnvServer::WifiConfig::wm.process();
  if ( setNextTimeCorrect < millis() )
  {
    //
    // somtimes correct elog time
    //
    logger.log( Prefs::LOGID, DEBUG, "main: logger time correction..." );
    setNextTimeCorrect = ( millis() * 1000UL * 21600UL );
    struct tm ti;
    if ( !getLocalTime( &ti ) )
    {
      logger.log( Prefs::LOGID, WARNING, "main: failed to obtain system time!" );
    }
    else
    {
      logger.log( Prefs::LOGID, DEBUG, "main: gotten system time!" );
      logger.provideTime( ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec );
      logger.log( Prefs::LOGID, DEBUG, "main: <%04d-%02d-%02d %02d:%02d:%02d>", ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
                  ti.tm_hour, ti.tm_min, ti.tm_sec );
      logger.log( Prefs::LOGID, DEBUG, "main: logger time correction...OK" );
    }
  }
  delay( 400 );
  if ( connected != StatusObject::getWlanState() )
  {
    auto new_connected = StatusObject::getWlanState();
    if ( connected == WlanState::DISCONNECTED || connected == WlanState::FAILED || connected == WlanState::SEARCHING )
    {
      //
      // was not functional for webservice
      //
      if ( new_connected == WlanState::CONNECTED || new_connected == WlanState::TIMESYNCED )
      {
        //
        // new connection, start webservice
        //
        logger.log( Prefs::LOGID, INFO, "main: ip connectivity found, start webserver." );
        EnvWebServer::start();
      }
      else
      {
        logger.log( Prefs::LOGID, WARNING, "main: ip connectivity lost, stop webserver." );
        EnvWebServer::stop();
      }
    }
    else
    {
      //
      // was functional for webservice
      //
      if ( !( new_connected == WlanState::CONNECTED || new_connected == WlanState::TIMESYNCED ) )
      {
        //
        // not longer functional
        //
        logger.log( Prefs::LOGID, WARNING, "main: ip connectivity lost!" );
        WifiConfig::reInit();
        // logger.log( Prefs::LOGID, WARNING, "main: ip connectivity lost, stop webserver." );
        // EnvWebServer::stop();
      }
    }
    // mark new value
    connected = new_connected;
  }
  // Elog::provideTime( 2023, 7, 15, 8, 12, 34 );  // We make up the time: 15th of july 2023 at 08:12:34
}
