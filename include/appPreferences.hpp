#pragma once
#include <DHT.h>
#include <FastLED.h>
#include <IPAddress.h>
#include <driver/rtc_io.h>
#include <esp_wifi.h>
#include <stdint.h>
#include <string>
#include <Elog.h>
#include <Preferences.h>

namespace Prefs
{
  //
  // some values are depend from compiling mode...
  //
#ifdef BUILD_DEBUG
  constexpr LogLevel D_LOG_LEVEL = DEBUG;
  constexpr int D_MININTERVAL = 600;
#else
#ifdef BUILD_RELEASE
  constexpr LogLevel D_LOG_LEVEL = INFO;
  constexpr int D_MININTERVAL = 600;
#else
  constexpr LogLevel D_LOG_LEVEL = WARNING;
  constexpr int D_MININTERVAL = 600;
#endif
#endif
  constexpr const uint8_t LOGID{ 5 };                                                                      //! logId for elog
  constexpr const char *DEFAULT_HOSTNAME{ "sensor" };                                                      //! default hostname network
  constexpr const char *APPNAME{ "en-app" };                                                               //! app name
  constexpr const LogLevel LOG_LEVEL = D_LOG_LEVEL;                                                        //! loglevel for App
  constexpr const char *MOUNTPOINT{ "/spiffs" };                                                           //! virtual path wegserver
  constexpr const char *WEB_DATA_PATH{ "/data/" };                                                         //! virtual path mesure data
  constexpr const char *WEB_DAYLY_MEASURE_FILE_NAME{ "%04d-%02d-%02d-mesure.csv" };                        //! data dayly for pressure
  constexpr const char *WEB_DAYLY_ACKU_FILE_NAME{ "%04d-%02d-%02d-acku.csv" };                             //! data dayly for pressure
  constexpr const char *WEB_DAYLY_FILE_PATTERN{ "^/data/\\d\\d\\d\\d-\\d\\d-\\d\\d-(mesure|acku).csv$" };  //! filename pattern
  constexpr const char *WEB_PARTITION_LABEL{ "mydata" };              //! label of the spiffs or null
  constexpr const char *JSON_TIMESTAMP_NAME{ "ti" };                  //! timestamp name in json
  constexpr const char *JSON_DEVICE_NAME{ "dev" };                    //! device name for data recorder
  constexpr const char *JSON_TEMPERATURE_NAME{ "te" };                //! temperature name in json
  constexpr const char *JSON_HUMIDY_NAME{ "hu" };                     //! humidy name in json
  constexpr const char *JSON_SENSOR_ID_NAME{ "id" };                  //! sensor id name in json
  constexpr const char *JSON_DATAOBJECT_NAME{ "da" };                 //! dataobject name in json for dataset per timestamp
  constexpr const char *JSON_ACKU_CURRENT_NAME{ "cu" };               //! dataobject name in json for dataset per timestamp
  constexpr gpio_num_t LED_STRIPE_RMT_TX_GPIO = GPIO_NUM_4;           //! control pin GPIO für led control
  constexpr int LED_STRIP_BRIGHTNESS = 255;                           //! brightness led stripe
  constexpr EOrder LED_RGB_ORDER = GRB;                               //! what order is the red/green/blue byte
  constexpr uint32_t LED_STRIP_RESOLUTION_HZ = 10000000;              //! 10MHz resolution, 1 tick = 0.1us
  constexpr uint8_t LED_STRIPE_COUNT = 3;                             //! count of rgb-led
  constexpr uint8_t LED_WLAN = 0;                                     //! indicator WLAN
  constexpr uint8_t LED_MEASURE = 1;                                  //! indicator generall messages
  constexpr uint8_t LED_HTTP = 2;                                     //! indicator http activity
  constexpr uint8_t LED_ALL = 255;                                    //! indicator means all led'S
  constexpr uint32_t WIFI_MAXIMUM_RETRY{ 5 };                         //! Max connection retries
  constexpr wifi_auth_mode_t WIFI_AUTH{ WIFI_AUTH_WPA2_PSK };         //! wifi auth method
  constexpr uint8_t SENSOR_DHT_SENSOR_TYPE = DHT11;                   //! Sensortyp for tem and humidy
  constexpr gpio_num_t SENSOR_DHT_GPIO = GPIO_NUM_5;                  //! port for sensor humidy and temperature (pullup resistor)
  constexpr gpio_num_t SENSOR_TEMPERATURE_GPIO = GPIO_NUM_18;         //! port fpr ds18b20 temperature sensor(es) (pullup resistor)
  constexpr uint8_t SENSOR_TEMPERATURE_MAX_COUNT = 4;                 //! max sensor count to measure
  constexpr uint8_t SENSOR_TEMPERATURE_MAX_COUNT_LIB = 8;             //! max sensor count to measure in library DAllas Temperature
  constexpr int MEASURE_INTERVAL_SEC = D_MININTERVAL;                 //! interval between two measures
  constexpr int MEASURE_SCAN_SENSOR_INTERVAL = 6000;                  //! scan for new sensors
  constexpr int MEASURE_WARN_TO_CRIT_COUNT = 4;                       //! how many failed mesures to critical display?
  constexpr int MEASURE_MAX_DATASETS_IN_RAM = 20;                     //! how many unsaved datasets in RAM before critical error
  constexpr gpio_num_t ACKU_MEASURE_GPIO = GPIO_NUM_35;               //! AD Port for Acku
  constexpr int ACKU_BROWNOUT_VALUE = 2700;                           //! mV: voltage when brownout => no SPIFFS writings!
  constexpr int ACKU_BROWNOUT_LOWEST = 1800;                          //! mV: voltage when esp not running => invalid measure!
  constexpr int ACKU_LOWER_VALUE = 2950;                              //! mV: lower than this, LED minimum
  constexpr uint32_t ACKU_CURRENT_SMOOTH_COUNT = 5U;                  //! smooting vlaue for acku value
  constexpr const char *FILE_CHECK_FILE_NAME{ "/data/fscheck.dat" };  //! timestamps for time check
  constexpr size_t FILESYS_MIN_FILE_SYSTEM_FREE_SIZE = 300000;        //! minimal free size on filesystem
  constexpr uint32_t FILESYS_CHECK_INTERVAL = 24U * 60U * 60U;        //! interval between two checks
  constexpr uint32_t FILESYS_CHECK_SLEEP_TIME_MS = 8121;              //! sleeptime für filecheck
  constexpr uint32_t FILESYS_CHECK_SLEEP_TIME_MULTIPLIER = 64;        //! multipiler for sleep ms

  class LocalPrefs
  {
    private:
    static const char *tag;    //! logging tag
    static bool wasInit;       //! was the prefs object initialized?
    static Preferences lPref;  // static preferences object

    public:
    static void init();                   //! init the preferences Object and Preferences
    static String getTimeZone();          //! get my timezone
    static bool setTimeZone( String & );  //! set my timezone
    static String getHostName();          //! get my own hostname
    static bool setHostName( String & );  //! set my Hostname
    static uint8_t getLogLevel();         //! get Logging
    static bool setLogLevel( uint8_t );   //! set Logging

    private:
    static bool getIfPrefsInit();        //! internal, is preferences initialized?
    static bool setIfPrefsInit( bool );  //! internal, set preferences initialized?
  };
}  // namespace Prefs

#include "appStructs.hpp"
