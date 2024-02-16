/*
  main, hier startet es
*/
#include "elog/eLog.hpp"
#include "common.hpp"
#include "main.hpp"
#include "statics.hpp"
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
  using namespace logger;

  // Debug Ausgabe initialisieren
  Serial.begin( 115200 );
  Serial.println( "main: program started..." );
  // read persistent config
  Serial.println( "main: init local preferences..." );
  Prefs::LocalPrefs::init();
  // correct loglevel
  Loglevel level = static_cast< Loglevel >( Prefs::LocalPrefs::getLogLevel() );
  elog.addSerialLogging( Serial, "MAIN", level );  // Enable serial logging. We want only INFO or lower logleve.
  elog.setSyslogOnline( false );
  elog.addSyslogLogging( level );
  elog.log( INFO, "main: start with logging..." );
  // set my timezone, i deal with timestamps
  elog.log( DEBUG, "main: set timezone (%s)...", Prefs::LocalPrefs::getTimeZone().c_str() );
  setenv( "TZ", Prefs::LocalPrefs::getTimeZone().c_str(), 1 );
  tzset();
  static String hName( Prefs::LocalPrefs::getHostName() );
  elog.log( INFO, "main: hostname: <%s>...", hName.c_str() );
  //
  // check if syslog and/or datalog present
  //
  IPAddress addr( Prefs::LocalPrefs::getSyslogServer() );
  uint16_t port( Prefs::LocalPrefs::getSyslogPort() );
  elog.log( INFO, "main: syslog %s:%d", addr.toString().c_str(), port );
  if ( ( addr > 0 ) && ( port > 0 ) )
  {
    elog.log( INFO, "main: init syslog protocol to %s:%d...", addr.toString().c_str(), port );
    elog.setUdpClient( udpClient, addr, port, hName.c_str(), Prefs::SYSLOG_APPNAME, Prefs::SYSLOG_PRIO, Prefs::SYSLOG_PROTO );
  }
  elog.log( DEBUG, "main: init LED Stripe..." );
  LEDStripe::init();
  elog.log( DEBUG, "main: init statusobject..." );
  StatusObject::init();
  //
  // check if config for data server peresent
  //
  addr = Prefs::LocalPrefs::getDataServer();
  port = Prefs::LocalPrefs::getDataPort();
  elog.log( INFO, "main: datalog %s:%d", addr.toString().c_str(), port );
  if ( ( addr > 0 ) && ( port > 0 ) )
  {
    elog.log( INFO, "main: init data log protocol to %s:%d...", addr.toString().c_str(), port );
    UDPDataLog::setUdpDataLog( udpDataClient, addr, port, Prefs::SYSLOG_APPNAME );
    StatusObject::setIsDataSend( true );
  }
  elog.log( DEBUG, "main: init acku watch..." );
  AckuVoltage::start();
  elog.log( DEBUG, "main: start sensor watching..." );
  TempMeasure::start();
  elog.log( DEBUG, "main: start wifi..." );
  WifiConfig::init();
  elog.log( DEBUG, "main: start filesystemchecker..." );
  FsCheckObject::start();
}

void loop()
{
  using namespace EnvServer;
  using namespace logger;

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
    elog.log( DEBUG, "main: logger time correction..." );
    setNextTimeCorrect = ( millis() * 1000UL * 21600UL );
    struct tm ti;
    if ( !getLocalTime( &ti ) )
    {
      elog.log( WARNING, "main: failed to obtain system time!" );
    }
    else
    {
      elog.log( DEBUG, "main: gotten system time!" );
      Elog::provideTime( ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec );
      elog.log( DEBUG, "main: logger time correction...OK" );
    }
  }
  delay( 600 );
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
        elog.log( INFO, "main: ip connectivity found, start webserver." );
        EnvWebServer::start();
      }
      else
      {
        elog.log( WARNING, "main: ip connectivity lost, stop webserver." );
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
        elog.log( WARNING, "main: ip connectivity lost, stop webserver." );
        EnvWebServer::stop();
      }
    }
    // mark new value
    connected = new_connected;
  }
  // Elog::provideTime( 2023, 7, 15, 8, 12, 34 );  // We make up the time: 15th of july 2023 at 08:12:34
}
