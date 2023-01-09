#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "appPreferences.hpp"

// ADC Calibration
#if CONFIG_IDF_TARGET_ESP32
#define ADC_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_VREF
#elif CONFIG_IDF_TARGET_ESP32S2
#define ADC_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP
#elif CONFIG_IDF_TARGET_ESP32C3
#define ADC_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP
#elif CONFIG_IDF_TARGET_ESP32S3
#define ADC_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP_FIT
#endif

constexpr uint32_t CURRENT_SMOOTH_COUNT = 5U;

namespace webserver
{
  class AckuVoltage
  {
    private:
    static const char *tag;

    public:
    static void start();
    static TaskHandle_t taskHandle;

    private:
    static void ackuTask( void * );
    static bool adcCalibrationInit( adc_cali_handle_t * );
  };

}  // namespace webserver
