#include <Esp.h>
#include "common.hpp"
#include "wifiConfig.hpp"
#include "statusObject.hpp"

namespace EnvServer
{
  const char *WifiConfig::tag{ "WifiConfig" };
  bool WifiConfig::is_sntp_init{ false };
  WiFiManager WifiConfig::wm;
  WiFiManagerParameter WifiConfig::custom_field;

  void WifiConfig::init()
  {
    char hostname[ 32 ];
    logger.log( Prefs::LOGID, INFO, "%s: initialize wifi...", WifiConfig::tag );
    uint16_t chip = static_cast< uint16_t >( ESP.getEfuseMac() >> 32 );
    snprintf( hostname, 32, "%s-%08X", Prefs::DEFAULT_HOSTNAME, chip );
    WiFi.setHostname( hostname );
    WiFi.mode( WIFI_STA );
    WiFi.onEvent( WifiConfig::wifiEventCallback );
    // reset settings - wipe credentials for testing
    // wm.resetSettings();
    // WifiConfig::wm.setConfigPortalBlocking( false );
    WifiConfig::wm.setConfigPortalBlocking( true );
    WifiConfig::wm.setConnectTimeout( 20 );
    WifiConfig::wm.setConfigPortalTimeout( 120 );  // 2 minutes up to auto connect again
    sntp_set_sync_mode( SNTP_SYNC_MODE_IMMED );
    sntp_setoperatingmode( SNTP_OPMODE_POLL );
    sntp_setservername( 1, Prefs::NTP_POOL_EUROPE_NAME );
    sntp_setservername( 2, Prefs::NTP_POOL_GERMANY_NAME );
    sntp_set_time_sync_notification_cb( WifiConfig::timeSyncNotificationCallback );
    if ( WifiConfig::wm.autoConnect( "EnvServerConfigAP" ) )
    {
      logger.log( Prefs::LOGID, INFO, "%s: wifi connected...", WifiConfig::tag );
      StatusObject::setWlanState( WlanState::CONNECTED );
      logger.log( Prefs::LOGID, DEBUG, "%s: try to sync time...", WifiConfig::tag );
      sntp_init();
      WifiConfig::is_sntp_init = true;
      WifiConfig::wm.stopWebPortal();
    }
    else
    {
      logger.log( Prefs::LOGID, WARNING, "%s: wifi not connected, access point running...", WifiConfig::tag );
      StatusObject::setWlanState( WlanState::DISCONNECTED );
      // WifiConfig::wm.setAPCallback( WifiConfig::configModeCallback );
      // set dark mode
      WifiConfig::wm.setClass( "invert" );
    }
    logger.log( Prefs::LOGID, INFO, "%s: initialize wifi...OK", WifiConfig::tag );
  }

  void WifiConfig::wifiEventCallback( arduino_event_t *event )
  {
    switch ( event->event_id )
    {
      case SYSTEM_EVENT_STA_CONNECTED:
        logger.log( Prefs::LOGID, INFO, "%s: device connected to accesspoint...", WifiConfig::tag );
        // if ( StatusObject::getWlanState() == WlanState::DISCONNECTED )
        //   StatusObject::setWlanState( WlanState::CONNECTED );
        break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
        logger.log( Prefs::LOGID, INFO, "%s: device disconnected from accesspoint...", WifiConfig::tag );
        StatusObject::setWlanState( WlanState::DISCONNECTED );
        break;
      case SYSTEM_EVENT_AP_STADISCONNECTED:
        logger.log( Prefs::LOGID, INFO, "%s: WIFI client disconnected...", WifiConfig::tag );
        sntp_stop();
        WifiConfig::is_sntp_init = false;
        break;
      case SYSTEM_EVENT_STA_GOT_IP:
        logger.log( Prefs::LOGID, INFO, "%s: device got ip <%s>...", WifiConfig::tag, WiFi.localIP().toString().c_str() );
        if ( StatusObject::getWlanState() == WlanState::DISCONNECTED )
          StatusObject::setWlanState( WlanState::CONNECTED );
        // sntp_init();
        if ( WifiConfig::is_sntp_init )
          sntp_restart();
        else
        {
          sntp_init();
          WifiConfig::is_sntp_init = true;
        }
        break;
      case SYSTEM_EVENT_STA_LOST_IP:
        logger.log( Prefs::LOGID, INFO, "%s: device lost ip...", WifiConfig::tag );
        StatusObject::setWlanState( WlanState::DISCONNECTED );
        sntp_stop();
        WifiConfig::is_sntp_init = false;
        break;
      default:
        break;
    }
  }

  void WifiConfig::timeSyncNotificationCallback( struct timeval * )
  {
    sntp_sync_status_t state = sntp_get_sync_status();
    switch ( state )
    {
      case SNTP_SYNC_STATUS_COMPLETED:
        logger.log( Prefs::LOGID, INFO, "%s: notification: time status sync completed!", WifiConfig::tag );
        ESP_LOGI( WebServer::tag, "notification: time status sync completed!" );
        if ( StatusObject::getWlanState() == WlanState::CONNECTED )
        {
          StatusObject::setWlanState( WlanState::TIMESYNCED );
        }
        struct tm ti;
        if ( !getLocalTime( &ti ) )
        {
          logger.log( Prefs::LOGID, WARNING, "%s: failed to obtain systemtime!", WifiConfig::tag );
        }
        else
        {
          logger.log( Prefs::LOGID, DEBUG, "%s: gotten system time!", WifiConfig::tag );
          logger.provideTime( ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec );
          logger.log( Prefs::LOGID, DEBUG, "%s: <%04d-%02d-%02d %02d:%02d:%02d>", WifiConfig::tag, ti.tm_year + 1900, ti.tm_mon + 1,
                      ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec );
        }
        break;
      default:
        logger.log( Prefs::LOGID, INFO, "%s: notification: time status NOT sync completed!", WifiConfig::tag );
        if ( StatusObject::getWlanState() == WlanState::TIMESYNCED )
        {
          StatusObject::setWlanState( WlanState::CONNECTED );
          if ( WifiConfig::is_sntp_init )
            sntp_restart();
          else
          {
            sntp_init();
            WifiConfig::is_sntp_init = true;
          }
        }
    }
  }

  void WifiConfig::configModeCallback( WiFiManager *myWiFiManager )
  {
    logger.log( Prefs::LOGID, INFO, "%s: config callback, enter config mode...", WifiConfig::tag );
    IPAddress apAddr = WiFi.softAPIP();
    logger.log( Prefs::LOGID, INFO, "%s: config callback: Access Point IP: <%s>...", WifiConfig::tag, apAddr.toString() );
    auto pSSID = myWiFiManager->getConfigPortalSSID();
    logger.log( Prefs::LOGID, DEBUG, "%s: config callback: AP SSID: <%s>...", pSSID.c_str() );
    logger.log( Prefs::LOGID, INFO, "%s: config callback...OK", WifiConfig::tag );
  }

}  // namespace EnvServer
