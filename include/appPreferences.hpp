#pragma once
#include <string>
#include <stdint.h>
#include <driver/rtc_io.h>
#include <driver/rmt.h>
#include <esp_wifi.h>
#include <led_strip.h>
#include <dht.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

namespace Prefs
{
  constexpr const char *TIMEZONE{ "CET-1" };                            //! my own timezone
  constexpr const char *MDNS_INSTANCE{ "esp rest server" };             //! instance nama of mdns process
  constexpr const char *WEB_PATH{ "/spiffs" };                          //! virtual path wegserver
  constexpr const char *WEB_DAYLY_FILE{ "/spiffs/today.jdata" };        //! virtual path today's file
  constexpr const char *WEB_WEEKLY_FILE{ "/spiffs/week.jdata" };        //! virtual path 7 day-history file
  constexpr const char *WEB_MONTHLY_FILE{ "/spiffs/month.jdata" };      //! virtual path 30 day history
  constexpr const char *WEB_TEMP_FILE{ "/spiffs/temporary.jdata" };     //! virtual path workerfile
  constexpr const char *JSON_TIMESTAMP_NAME{ "ti" };                    //! timestamp name in json
  constexpr const char *JSON_TEMPERATURE_NAME{ "te" };                  //! temperature name in json
  constexpr const char *JSON_HUMIDY_NAME{ "hu" };                       //! humidy name in json
  constexpr const char *JSON_SENSOR_ID_NAME{ "id" };                    //! sensor id name in json
  constexpr const char *JSON_DATAOBJECT_NAME{ "da" };                   //! dataobject name in json for dataset per timestamp
  constexpr const char *JSON_ACKU_CURRENT_NAME{ "cu" };                 //! dataobject name in json for dataset per timestamp
  constexpr size_t WEB_FILEPATH_MAX_SIZE{ 64 };                         //! max size of the filepath
  constexpr uint32_t WEB_SCRATCH_BUFSIZE{ 1440 };                       //! buffsize für http server answers
  constexpr gpio_num_t LED_STRIPE_RMT_TX_GPIO = GPIO_NUM_4;             //! control pin GPIO für led control
  constexpr led_strip_type_t LED_STRIPE_TYPE = LED_STRIP_WS2812;        //! type of led stripe
  constexpr rmt_channel_t LED_STRIPE_RMT_CHANNEL = RMT_CHANNEL_3;       //! which remote control channel
  constexpr uint32_t LED_STRIPE_COUNT = 3U;                             //! count of rgb-led
  constexpr uint8_t LED_WLAN = 0;                                       //! indicator WLAN
  constexpr uint8_t LED_MEASURE = 1;                                    //! indicator generall messages
  constexpr uint8_t LED_HTTP = 2;                                       //! indicator http activity
  constexpr uint8_t LED_ALL = 255;                                      //! indicator means all led'S
  constexpr uint32_t WIFI_MAXIMUM_RETRY{ 5 };                           //! Max connection retries
  constexpr wifi_auth_mode_t WIFI_AUTH{ WIFI_AUTH_WPA2_PSK };           //! wifi auth method
  constexpr dht_sensor_type_t SENSOR_DHT_SENSOR_TYPE = DHT_TYPE_DHT11;  //! Sensortyp for tem and humidy
  constexpr gpio_num_t SENSOR_DHT_GPIO = GPIO_NUM_5;                    //! port for sensor humidy and temperature (pullup resistor)
  constexpr gpio_num_t SENSOR_TEMPERATURE_GPIO = GPIO_NUM_18;           //! port fpr ds18b20 temperature sensor(es) (pullup resistor)
  constexpr uint8_t SENSOR_TEMPERATURE_MAX_COUNT = 4;                   //! max sensor count to measure
  constexpr int MEASURE_INTERVAL_SEC = 600;                             //! interval between two measures
  constexpr int MEASURE_SCAN_SENSOR_INTERVAL = 6000;                    //! scan for new sensors
  constexpr int MEASURE_WARN_TO_CRIT_COUNT = 4;                         //! how many failed mesures to critical display?
  constexpr int MEASURE_MAX_DATASETS_IN_RAM = 20;                       //! how many unsaved datasets in RAM before critical error
  constexpr uint32_t FILESYS_CHECK_INTERVAL = 12U * 60U * 60U;          //! interval between two checks
  constexpr const char *ACKU_LOG_FILE{ "/spiffs/acku.jdata" };          //! virtual path for acku log
  constexpr int ACKU_BROWNOUT_VALUE = 2700;                             //! mV: voltage when brownout => no SPIFFS writings!
  constexpr int ACKU_BROWNOUT_LOWEST = 1800;                            //! mV: voltage when esp not running => invalid measure!
  constexpr int ACKU_LOWER_VALUE = 3100;                                //! mV: lower than this, LED minimum
  constexpr adc_unit_t ACKU_MEASURE_ADC_UNIT = ADC_UNIT_1;              //! which adc unit for ADC
  constexpr adc_channel_t ACKU_MEASURE_CHANNEL = ADC_CHANNEL_7;         //! AD pin for acku in LOLIN32
  constexpr adc_bitwidth_t ACKU_ADC_WIDTH = ADC_BITWIDTH_12;            //! full resulution for adc, we hab enough time
  constexpr adc_atten_t ACKU_ATTENT = ADC_ATTEN_DB_11;                  //! max db
  // constexpr int MEASURE_INTERVAL_SEC = 20;                     //! interval between two measures
  // constexpr int MEASURE_SCAN_SENSOR_INTERVAL = 62;             //! scan for new sensors
}  // namespace Prefs

#include "appStructs.hpp"
