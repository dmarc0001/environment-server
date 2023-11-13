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
    elog.log( INFO, "%s: initialize wifi...", WifiConfig::tag );
    WiFi.setHostname( Prefs::WIFI_DEFAULT_HOSTNAME );
    WiFi.mode( WIFI_STA );
    WiFi.onEvent( WifiConfig::wifiEventCallback );
    // reset settings - wipe credentials for testing
    // wm.resetSettings();
    WifiConfig::wm.setConfigPortalBlocking( false );
    WifiConfig::wm.setConnectTimeout( 20 );
    if ( WifiConfig::wm.autoConnect( "AutoConnectAP" ) )
    {
      elog.log( INFO, "%s: wifi connected...", WifiConfig::tag );
      StatusObject::setWlanState( WlanState::CONNECTED );
      elog.log( DEBUG, "%s: try to sync time...", WifiConfig::tag );
      sntp_set_sync_mode( SNTP_SYNC_MODE_IMMED );
      sntp_setoperatingmode( SNTP_OPMODE_POLL );
      sntp_setservername( 1, "pool.ntp.org" );
      sntp_set_time_sync_notification_cb( WifiConfig::timeSyncNotificationCallback );
      sntp_init();
      WifiConfig::is_sntp_init = true;
      // sntp_restart();
    }
    else
    {
      elog.log( WARNING, "%s: wifi not connected, access point running...", WifiConfig::tag );
      StatusObject::setWlanState( WlanState::DISCONNECTED );
      // test custom html(radio)
      // const char *custom_radio_str =
      //     "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> "
      //     "One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> "
      //     "Three";
      // new ( &WifiConfig::custom_field ) WiFiManagerParameter( custom_radio_str );  // custom html input
      // WifiConfig::wm.addParameter( &WifiConfig::custom_field );
      // WifiConfig::wm.setAPCallback( configModeCallback );
      // std::vector< const char * > menu = { "wifi", "info", "param", "sep", "restart", "exit" };
      // WifiConfig::wm.setMenu( menu );
      // set dark mode
      WifiConfig::wm.setClass( "invert" );
      // config portal timeout 30 Sek
      // WifiConfig::wm.setConfigPortalTimeout( 120 );
    }
    elog.log( INFO, "%s: initialize wifi...OK", WifiConfig::tag );
  }

  void WifiConfig::loop()
  {
    WifiConfig::wm.process();
  }

  void WifiConfig::wifiEventCallback( arduino_event_t *event )
  {
    switch ( event->event_id )
    {
      case SYSTEM_EVENT_STA_CONNECTED:
        elog.log( INFO, "%s: device connected to accesspoint...", WifiConfig::tag );
        if ( StatusObject::getWlanState() == WlanState::DISCONNECTED )
          StatusObject::setWlanState( WlanState::CONNECTED );
        break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
        elog.log( INFO, "%s: device disconnected from accesspoint...", WifiConfig::tag );
        StatusObject::setWlanState( WlanState::DISCONNECTED );
        break;
      case SYSTEM_EVENT_AP_STADISCONNECTED:
        elog.log( INFO, "%s: WIFI client disconnected...", WifiConfig::tag );
        break;
      case SYSTEM_EVENT_STA_GOT_IP:
        elog.log( INFO, "%s: device got ip <%s>...", WifiConfig::tag, WiFi.localIP().toString().c_str() );
        break;
      case SYSTEM_EVENT_STA_LOST_IP:
        elog.log( INFO, "%s: device lost ip...", WifiConfig::tag );
        StatusObject::setWlanState( WlanState::DISCONNECTED );
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
        elog.log( INFO, "%s: notification: time status sync completed!", WifiConfig::tag );
        ESP_LOGI( WebServer::tag, "notification: time status sync completed!" );
        if ( StatusObject::getWlanState() == WlanState::CONNECTED )
        {
          StatusObject::setWlanState( WlanState::TIMESYNCED );
        }
        break;
      default:
        elog.log( INFO, "%s: notification: time status NOT sync completed!", WifiConfig::tag );
        if ( StatusObject::getWlanState() == WlanState::TIMESYNCED )
        {
          StatusObject::setWlanState( WlanState::CONNECTED );
        }
    }
  }

  void WifiConfig::configModeCallback( WiFiManager *myWiFiManager )
  {
    elog.log( INFO, "%s: config callback, enter config mode...", WifiConfig::tag );
    IPAddress apAddr = WiFi.softAPIP();
    elog.log( INFO, "%s: config callback: Access Point IP: <%s>...", WifiConfig::tag, apAddr.toString() );
    auto pSSID = myWiFiManager->getConfigPortalSSID();
    elog.log( DEBUG, "%s: config callback: AP SSID: <%s>...", pSSID.c_str() );
    elog.log( INFO, "%s: config callback...OK", WifiConfig::tag );
  }

}  // namespace EnvServer
