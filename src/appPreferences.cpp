#include "appPreferences.hpp"
#include "common.hpp"

namespace Prefs
{
  constexpr const char *CHECKVAL{ "confInit" };
  constexpr const char *INITVAL{ "wasInit" };
  constexpr const char *LOCAL_TIMEZONE{ "locTimeZone" };
  constexpr const char *LOCAL_HOSTNAME{ "hostName" };
  constexpr const char *DEBUGSETTING{ "debugging" };

  Preferences LocalPrefs::lPref;
  bool LocalPrefs::wasInit{ false };
  const char *LocalPrefs::tag{ "LocalPrefs" };

  /**
   * init the static preferences objectg
   */
  void LocalPrefs::init()
  {
    using namespace EnvServer;

    LocalPrefs::lPref.begin( APPNAME, false );
    if ( !LocalPrefs::getIfPrefsInit() )
    {
      Serial.println( "first-time-init preferences..." );
      char hostname[ 32 ];
      uint16_t chip = static_cast< uint16_t >( ESP.getEfuseMac() >> 32 );
      snprintf( hostname, 32, "%s-%08X", Prefs::DEFAULT_HOSTNAME, chip );
      String hn( &hostname[ 0 ] );
      LocalPrefs::lPref.putString( LOCAL_TIMEZONE, "GMT" );
      LocalPrefs::lPref.putString( LOCAL_HOSTNAME, hn );
      LocalPrefs::lPref.putUChar( DEBUGSETTING, DEBUG );
      LocalPrefs::setIfPrefsInit( true );
      Serial.println( "first-time-init preferences...DONE" );
    }
    LocalPrefs::wasInit = true;
  }

  /**
   * get local timezone
   */
  String LocalPrefs::getTimeZone()
  {
    return ( LocalPrefs::lPref.getString( LOCAL_TIMEZONE, "GMT" ) );
  }

  /**
   * set local timezone
   */
  bool LocalPrefs::setTimeZone( String &_zone )
  {
    return ( LocalPrefs::lPref.putString( LOCAL_TIMEZONE, _zone ) > 0 );
  }

  /**
   * get the local hostname
   */
  String LocalPrefs::getHostName()
  {
    return ( LocalPrefs::lPref.getString( LOCAL_HOSTNAME, Prefs::DEFAULT_HOSTNAME ) );
  }

  /**
   * set the local hostname
   */
  bool LocalPrefs::setHostName( String &_hostname )
  {
    return ( LocalPrefs::lPref.putString( LOCAL_HOSTNAME, _hostname ) > 0 );
  }

  uint8_t LocalPrefs::getLogLevel()
  {
#ifdef BUILD_DEBUG
    return ( LocalPrefs::lPref.getUChar( DEBUGSETTING, DEBUG ) );
#else
    return ( LocalPrefs::lPref.getUChar( DEBUGSETTING, INFO ) );
#endif
  }

  bool LocalPrefs::setLogLevel( uint8_t _set )
  {
    return ( LocalPrefs::lPref.putUChar( DEBUGSETTING, _set ) == 1 );
  }

  /**
   * Check if preferences was initialized once
   */
  bool LocalPrefs::getIfPrefsInit()
  {
    using namespace EnvServer;

    String defaultVal( "-" );
    String correctVal( INITVAL );
    // String retVal = LocalPrefs::lPref.getString( CHECKVAL, defaultVal );
    // logger.log( Prefs::LOGID, INFO, "%s: getIfPrefsInit correct: <%s> vs read <%s> (%s)...", LocalPrefs::tag, INITVAL, retVal.c_str(),
    //           correctVal == retVal ? "true" : "false" );
    // return ( correctVal == retVal );
    return ( correctVal == LocalPrefs::lPref.getString( CHECKVAL, defaultVal ) );
  }

  /**
   * set if prefs was initialized
   */
  bool LocalPrefs::setIfPrefsInit( bool _set )
  {
    // using namespace EnvServer;

    if ( _set )
    {
      // logger.log( Prefs::LOGID, INFO, "%s: setIfPrefsInit set <%s> to <%s> (%s)...", LocalPrefs::tag, CHECKVAL, INITVAL );
      // if set => set the correct value
      return ( LocalPrefs::lPref.putString( CHECKVAL, INITVAL ) > 0 );
    }
    // else remove the key, set the properties not valid
    // logger.log( Prefs::LOGID, INFO, "%s: setIfPrefsInit remove  <%s>...", LocalPrefs::tag, CHECKVAL );
    return LocalPrefs::lPref.remove( CHECKVAL );
  }

}  // end namespace Prefs