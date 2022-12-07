#include <time.h>
#include <memory>
#include <algorithm>
#include <esp_log.h>
#include <stdio.h>
#include <cJSON.h>
#include "appPreferences.hpp"
#include "statusObject.hpp"

namespace webserver
{
  const char *StatusObject::tag{"status_object"};
  bool StatusObject::is_init{false};
  bool StatusObject::is_running{false};
  SemaphoreHandle_t StatusObject::fileSem{nullptr};

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
    // semaphore for using file writing
    vSemaphoreCreateBinary(StatusObject::fileSem);
    StatusObject::is_init = true;
    StatusObject::dataset->clear();
    ESP_LOGI(StatusObject::tag, "init status object...OK, start task...");
    StatusObject::start();
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
    ESP_LOGI(StatusObject::tag, "task started...");
    xSemaphoreGive(StatusObject::fileSem);
    while (true)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
      if (!StatusObject::dataset->empty())
      {
        //
        // there are data, try to save in file(s)
        // one for every day
        //
        struct stat file_stat;
        std::string daylyFileName(Prefs::WEB_DAYLY_FILE);
        bool exist_file{false};
        FILE *fd = nullptr;

        ESP_LOGI(StatusObject::tag, "data for save exist...");
        // is file exist
        if (stat(daylyFileName.c_str(), &file_stat) == 0)
        {
          exist_file = true;
        }
        else
          ESP_LOGI(StatusObject::tag, "file save does not exist...");

        while (!StatusObject::dataset->empty())
        {
          // read one dataset
          auto elem = StatusObject::dataset->front();
          StatusObject::dataset->erase(StatusObject::dataset->begin());
          // create one object for one dataset
          cJSON *dataSetObj = cJSON_CreateObject();
          // make timestamp objekt item
          cJSON_AddItemToObject(dataSetObj, "timestamp", cJSON_CreateString(std::to_string(elem.timestamp).c_str()));
          // make array of measures
          auto m_dataset = elem.dataset;
          cJSON *mArray = cJSON_CreateArray();
          while (!m_dataset.empty())
          {
            // fill messure array
            auto m_elem = m_dataset.front();
            m_dataset.erase(m_dataset.begin());
            cJSON *mObj = cJSON_CreateObject();
            char buffer[16];
            if (m_elem.addr > 0)
            {
              cJSON_AddItemToObject(mObj, "id", cJSON_CreateString(std::to_string(m_elem.addr).substr(1, 6).c_str()));
            }
            else
            {
              cJSON_AddItemToObject(mObj, "id", cJSON_CreateString("0"));
            }
            memset(&buffer[0], 0, 16);
            sprintf(&buffer[0], "%3.1f", m_elem.temp);
            cJSON_AddItemToObject(mObj, "temp", cJSON_CreateString(&buffer[0]));
            memset(&buffer[0], 0, 16);
            sprintf(&buffer[0], "%3.1f", m_elem.humidy);
            cJSON_AddItemToObject(mObj, "humidy", cJSON_CreateString(&buffer[0]));
            // measure object to array
            cJSON_AddItemToArray(mArray, mObj);
          }
          // array as item to object
          cJSON_AddItemToObject(dataSetObj, "data", mArray);
          // dataSetObj complete
          // try to write to file
          // wait max 1000 ms
          //
          if (xSemaphoreTake(StatusObject::fileSem, pdMS_TO_TICKS(1000)) == pdTRUE)
          {
            // We were able to obtain the semaphore and can now access the
            // shared resource.
            // open File mode append
            fd = fopen(daylyFileName.c_str(), "a");
            if (fd)
            {
              ESP_LOGI(StatusObject::tag, "datafile <%s> opened...", daylyFileName.c_str());
              if (exist_file)
              {
                // exist the file, is there minimum on dataset,
                // an i need a comma to write
                fputs(",", fd);
              }
              else
              {
                // this is the first dataset, it's permitted
                // wo write a comma on first entry
                exist_file = true;
              }
              char *jsonPrintString = cJSON_PrintUnformatted(dataSetObj);
              fputs(jsonPrintString, fd);
              fflush(fd);
              fputs("\n", fd);
              fclose(fd);
              cJSON_Delete(dataSetObj);
              cJSON_free(jsonPrintString); // !!!!!!! memory leak
              ESP_LOGI(StatusObject::tag, "datafile <%s> written and closed...", daylyFileName.c_str());
            }
            else
            {
              while (!StatusObject::dataset->empty())
              {
                StatusObject::dataset->erase(StatusObject::dataset->begin());
              }
              ESP_LOGE(StatusObject::tag, "datafile <%s> can't open, data lost...", daylyFileName.c_str());
            }
            // We have finished accessing the shared resource.  Release the
            // semaphore.
            xSemaphoreGive(StatusObject::fileSem);
            // static uint32_t oldh{0UL};
            // uint32_t nowh = xPortGetFreeHeapSize();
            // uint32_t diff = nowh - oldh;
            // oldh = nowh;
            // ESP_LOGW(StatusObject::tag, "HEAP: %d, diff %d", nowh, diff);
          }
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
      xTaskCreate(StatusObject::saveTask, "data-task", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY, NULL);
    }
  }
}
