#include <time.h>
#include <memory>
#include <algorithm>
#include <esp_log.h>
#include <stdio.h>
#include "AppPreferences.hpp"
#include "statusObject.hpp"

namespace webserver
{
  const char *StatusObject::tag{"StatusObject"};
  bool StatusObject::is_init{false};
  bool StatusObject::is_running{false};
  WlanState StatusObject::wlanState{WlanState::DISCONNECTED};
  MeasureState StatusObject::msgState{MeasureState::MEASURE_UNKNOWN};
  bool StatusObject::http_active{false};
  std::shared_ptr<env_dataset> StatusObject::dataset = std::shared_ptr<env_dataset>(new env_dataset());

  /**
   * init object and start file writer task
  */
  void StatusObject::init()
  {
    ESP_LOGI(StatusObject::tag, "init status object...");
    StatusObject::is_init = true;
    StatusObject::dataset->clear();
    StatusObject::start();
  }

  /**
   * Dataset copy into queue to write into file
  */
  void StatusObject::setMeasures(std::shared_ptr<env_dataset> dataset)
  {
    if (!StatusObject::is_init)
      init();
    //
    // do save that the vector not will overload, memory is not infinite
    //
    std::for_each(dataset->begin(), dataset->end(), [](const env_measure_t n) {
      StatusObject::dataset->push_back(n);
    });
  }

  /**
   * set wlan state into object
  */
  void StatusObject::setWlanState(WlanState _state)
  {
    StatusObject::wlanState = _state;
  }

  /**
   * get wlan state from object
  */
  WlanState StatusObject::getWlanState()
  {
    return StatusObject::wlanState;
  }

  /**
   * set the status of the measure
  */
  void StatusObject::setMeasureState(MeasureState _st)
  {
    StatusObject::msgState = _st;
  }

  /**
   * give thenstsate of the measure
  */
  MeasureState StatusObject::getMeasureState()
  {
    return StatusObject::msgState;
  }

  /**
   * set is httpd active (active request of a file/data)
  */
  void StatusObject::setHttpActive(bool _http)
  {
    StatusObject::http_active = _http;
  }

  /**
   * get the active state of http daemon
  */
  bool StatusObject::getHttpActive()
  {
    return StatusObject::http_active;
  }

  /**
   * RTOS Task to save data into file
  */
  void StatusObject::saveTask(void *ptr)
  {
    //
    // TODO: check count of files, delete if an file is too old
    // do this all few hours
    //
    while (true)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
      if (!StatusObject::dataset->empty())
      {
        //
        // there are data, try to save in file(s)
        // one for every day
        //
        time_t rawtime;
        struct tm *timeinfo;
        char buffer[48];
        char *buffer_ptr = static_cast<char *>(&buffer[0]);
        std::string fileTemplate(Prefs::WEB_PATH);
        fileTemplate += "/%F-measure.json";
        // get time
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        // make filename to buffer
        strftime(buffer_ptr, 48, fileTemplate.c_str(), timeinfo);
        // open File mode append
        FILE *fd = nullptr;
        fd = fopen(buffer_ptr, "a");
        if (fd)
        {
          // file is opened!
          ESP_LOGI(StatusObject::tag, "datafile <%s> opened...", buffer_ptr);
          while (!StatusObject::dataset->empty())
          {
            auto elem = StatusObject::dataset->front();
            StatusObject::dataset->erase(StatusObject::dataset->begin());
            auto timestamp = "[\"" + std::to_string(elem.timestamp) + "\", ";
            auto addr = "\"" + std::to_string(elem.addr) + "\", ";
            auto temp = "\"" + std::to_string(elem.temp) + "\", ";
            auto humidy = "\"" + std::to_string(elem.humidy) + "\" ],\n";
            fputs(timestamp.c_str(), fd);
            fputs(addr.c_str(), fd);
            fputs(temp.c_str(), fd);
            fputs(humidy.c_str(), fd);
          }
          fclose(fd);
          ESP_LOGI(StatusObject::tag, "datafile <%s> written and closed...", buffer_ptr);
        }
        else
        {
          while (!StatusObject::dataset->empty())
          {
            StatusObject::dataset->erase(StatusObject::dataset->begin());
          }
          ESP_LOGI(StatusObject::tag, "datafile <%s> can't open, data lost...", buffer_ptr);
        }
      }
    }
  }

  /**
   * start (if not running yet) the data writer task
  */
  void StatusObject::start()
  {
    if (StatusObject::is_running)
    {
      ESP_LOGE(StatusObject::tag, "data save task is running, abort.");
    }
    else
    {
      //xTaskCreate(static_cast<void (*)(void *)>(&(rest_webserver::TempMeasure::measureTask)), "ds18x20", configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL);
      xTaskCreate(StatusObject::saveTask, "data-task", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY, NULL);
    }
  }

}
