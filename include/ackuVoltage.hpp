#pragma once

#include <stdio.h>
#include <stdlib.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <driver/adc.h>
// #include <esp_adc_cal.h>

#include "appPreferences.hpp"

//ADC Calibration
#if CONFIG_IDF_TARGET_ESP32
#define ADC_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_VREF
#elif CONFIG_IDF_TARGET_ESP32S2
#define ADC_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP
#elif CONFIG_IDF_TARGET_ESP32C3
#define ADC_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP
#elif CONFIG_IDF_TARGET_ESP32S3
#define ADC_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP_FIT
#endif

namespace webserver
{
  class AckuVoltage
  {
  private:
    static const char *tag;
    static bool is_running; //! is the task running
    static esp_adc_cal_characteristics_t adc1_chars;

  public:
    static void start();

  private:
    static void ackuTask(void *);
    static bool adcCalibrationInit();
  };

}
