#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

#include "ackuVoltage.hpp"
#include "statusObject.hpp"
#include "appPreferences.hpp"

namespace webserver
{

  using namespace Prefs;

  const char *AckuVoltage::tag{"acku_voltage"};
  bool AckuVoltage::is_running{false};
  esp_adc_cal_characteristics_t AckuVoltage::adc1_chars;

  /**
   * start low prio task for scanning the acku value
  */
  void AckuVoltage::start()
  {
    if (AckuVoltage::is_running)
    {
      ESP_LOGE(AckuVoltage::tag, "acku measure task is running, abort.");
    }
    else
    {
      // for security, if done do nothing
      StatusObject::init();
      xTaskCreate(AckuVoltage::ackuTask, "acku-measure", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY, NULL);
    }
  }

  /**
   * task for scanning the acku value. if no value to measure, deactivate
  */
  void AckuVoltage::ackuTask(void *)
  {
    AckuVoltage::is_running = true;

    bool cali_enable = AckuVoltage::adcCalibrationInit();

    ret = esp_adc_cal_check_efuse(ADC_CALI_SCHEME);
    if (ret == ESP_ERR_NOT_SUPPORTED)
    {
      ESP_LOGW(AckuVoltage::tag, "calibration scheme not supported, skip software calibration");
    }
    else if (ret == ESP_ERR_INVALID_VERSION)
    {
      ESP_LOGW(AckuVoltage::tag, "eFuse not burnt, skip software calibration");
    }
    else if (ret == ESP_OK)
    {
      cali_enable = true;
      ESP_LOGI(AckuVoltage::tag, "calibre adc...");
      // 0 dB attenuation (ADC_ATTEN_DB_0) gives full-scale voltage 1.1 V
      // 2.5 dB attenuation (ADC_ATTEN_DB_2_5) gives full-scale voltage 1.5 V
      // 6 dB attenuation (ADC_ATTEN_DB_6) gives full-scale voltage 2.2 V
      // 11 dB attenuation (ADC_ATTEN_DB_11) gives full-scale voltage 3.9 V (see note below)
      esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ACKU_ADC_WIDTH, 0, &adc1_chars);
      ESP_LOGI(AckuVoltage::tag, "calibre adc successful...");
    }
    else
    {
      ESP_LOGE(AckuVoltage::tag, "Invalid arg");
    }
    //
    // calibration done... (or failed)
    // infinity loop
    //
    vTaskDelay(pdMS_TO_TICKS(234));
    while (true)
    {
      uint32_t voltage{0};
      int adc_raw = adc1_get_raw(ACKU_MEASURE_CHANNEL);
      //ESP_LOGD(AckuVoltage::tag, "raw  data: %d", adc_raw);
      if (cali_enable)
      {
        voltage = esp_adc_cal_raw_to_voltage(adc_raw, &AckuVoltage::adc1_chars);
        ESP_LOGE(AckuVoltage::tag, "acku current: %d mV (raw: %d)", voltage, adc_raw);
        StatusObject::setVoltage(voltage);
      }
      else
      {
        StatusObject::setVoltage(0);
      }
      //
      // sleep for a while, acku needs som time for changing
      //
      vTaskDelay(pdMS_TO_TICKS(1003));
      //vTaskDelay(pdMS_TO_TICKS(45013));
    }
  }

  /**
   * init calibration 
   * https://github.com/espressif/esp-idf/blob/v4.4/examples/peripherals/adc/single_read/single_read/main/single_read.c
  */
  bool AckuVoltage::adcCalibrationInit()
  {
    using namespace Prefs;
    esp_err_t ret;
    bool cali_enable = false;

    ESP_LOGI(AckuVoltage::tag, "init adc calibration...");
    ret = esp_adc_cal_check_efuse(ADC_CALI_SCHEME);
    adc1_config_width(ACKU_ADC_WIDTH);
    adc1_config_channel_atten(ACKU_MEASURE_CHANNEL, ACKU_ATTENT);
    if (ret == ESP_ERR_NOT_SUPPORTED)
    {
      ESP_LOGW(AckuVoltage::tag, "Calibration scheme not supported, skip software calibration");
    }
    else if (ret == ESP_ERR_INVALID_VERSION)
    {
      ESP_LOGW(AckuVoltage::tag, "eFuse not burnt, skip software calibration");
    }
    else if (ret == ESP_OK)
    {
      cali_enable = true;
      esp_adc_cal_characterize(ADC_UNIT_1, ACKU_ATTENT, ACKU_ADC_WIDTH, 0, &AckuVoltage::adc1_chars);
      ESP_LOGI(AckuVoltage::tag, "init adc calibration successful...");
    }
    else
    {
      ESP_LOGE(AckuVoltage::tag, "Invalid arg");
    }
    return cali_enable;
  }

}
