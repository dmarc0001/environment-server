#pragma once
#include <string>
#include <stdint.h>
#include <driver/rtc_io.h>
#include <driver/adc.h>
#include <driver/rmt.h>
#include <esp_wifi.h>
#include <led_strip.h>
#include <dht.h>

namespace Prefs
{
  constexpr const char *TIMEZONE{"CET-1"};                             //! my own timezone
  constexpr const char *MDNS_INSTANCE{"esp rest server"};              //! instance nama of mdns process
  constexpr const char *WEB_PATH{"/spiffs"};                           //! virtual path wegserver
  constexpr size_t WEB_FILEPATH_MAX_SIZE{64};                          //! max size of the filepath
  constexpr uint32_t WEB_SCRATCH_BUFSIZE{1024};                        //! buffsize für http server answers
  constexpr gpio_num_t LED_STRIPE_RMT_TX_GPIO = GPIO_NUM_5;            //! control pin GPIO für led control
  constexpr led_strip_type_t LED_STRIPE_TYPE = LED_STRIP_WS2812;       //! type of led stripe
  constexpr rmt_channel_t LED_STRIPE_RMT_CHANNEL = RMT_CHANNEL_3;      //! which remote control channel
  constexpr uint32_t LED_STRIPE_COUNT = 3U;                            //! count of rgb-led
  constexpr uint8_t LED_WLAN = 0;                                      //! indicator WLAN
  constexpr uint8_t LED_MEASURE = 1;                                   //! indicator generall messages
  constexpr uint8_t LED_HTTP = 2;                                      //! indicator http activity
  constexpr uint8_t LED_ALL = 255;                                     //! indicator means all led'S
  constexpr uint32_t WIFI_MAXIMUM_RETRY{5};                            //! Max connection retries
  constexpr wifi_auth_mode_t WIFI_AUTH{WIFI_AUTH_WPA2_PSK};            //! wifi auth method
  constexpr dht_sensor_type_t SENSOR_DHT_SENSOR_TYPE = DHT_TYPE_DHT11; //! Sensortyp for tem and humidy
  constexpr gpio_num_t SENSOR_DHT_GPIO = GPIO_NUM_18;                  //! port for sensor humidy and temperature
  constexpr gpio_num_t SENSOR_TEMPERATURE_GPIO = GPIO_NUM_4;           //! port fpr ds18b20 temperature sensor(es)
  constexpr uint8_t SENSOR_TEMPERATURE_MAX_COUNT = 4;                  //! max sensor count to measure
  //constexpr int MEASURE_INTERVAL_SEC = 120;                       //! interval between two measures
  //constexpr int MEASURE_SCAN_SENSOR_INTERVAL = 612;               //! scan for new sensors
  constexpr int MEASURE_INTERVAL_SEC = 20;                     //! interval between two measures
  constexpr int MEASURE_SCAN_SENSOR_INTERVAL = 62;             //! scan for new sensors
  constexpr int MEASURE_WARN_TO_CRIT_COUNT = 4;                //! how many failed mesures to critical display?
  constexpr int MEASURE_MAX_DATASETS_IN_RAM = 20;              //! how many unsaved datasets in RAM before critical error
  constexpr uint32_t FILESYS_CHECK_INTERVAL = 24U * 60U * 60U; //! interval between two checks
} // namespace Prefs

#include "appStructs.hpp"
