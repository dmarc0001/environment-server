#include <string.h>
#include <string>
#include <stdio.h>
#include <esp_log.h>
#include "appPreferences.hpp"
#include "statusObject.hpp"
#include "filesystemChecker.hpp"

namespace webserver
{
  const char *FsCheckObject::tag{"fs_checker"};
  bool FsCheckObject::is_init{false};
  bool FsCheckObject::is_running{false};

  /**
   * start (if not running yet) the file-system-checker
  */
  void FsCheckObject::start()
  {
    if (FsCheckObject::is_running)
    {
      ESP_LOGE(FsCheckObject::tag, "filesystem chceker task is running, abort.");
    }
    else
    {
      //xTaskCreate(FsCheckObject::filesystemTask, "fs-check-task", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY, NULL);
    }
  }

  /**
   * init a few things if not done
  */
  void FsCheckObject::init()
  {
    ESP_LOGI(FsCheckObject::tag, "init filesystem checker object...");
    // nothing to do at this time ;-)
    FsCheckObject::is_init = true;
  }

  /**
   * infinity task....
  */
  void FsCheckObject::filesystemTask(void *)
  {
    FsCheckObject::is_running = true;
    if (!FsCheckObject::is_init)
      init();
    // for security, if init do nothing
    StatusObject::init();
    //
    // construct filename for marker for last check
    //
    std::string fileName(Prefs::WEB_PATH);
    fileName += "/last_fscheck.dat";
    // at first, do nothing, let oher things at first
    vTaskDelay(pdMS_TO_TICKS(10250));
    // infinity
    while (true)
    {
      // wait for next garbage disposal
      vTaskDelay(pdMS_TO_TICKS(13560));
      if (StatusObject::getWlanState() != WlanState::TIMESYNCED)
      {
        //
        // no timesync, no time for compare, next round....
        //
        continue;
      }
      timeval val;
      uint32_t lastTimestamp{0UL};
      gettimeofday(&val, nullptr);
      uint32_t timestamp = val.tv_sec;
      if (!FsCheckObject::checkExistStatFile(fileName, timestamp))
      {
        continue;
      }
      //
      // file exist
      //
      lastTimestamp = FsCheckObject::getLastTimestamp(fileName);
      //
      // is the last timestamp older than the difference from prefs?
      //
      if (timestamp - lastTimestamp > Prefs::FILESYS_CHECK_INTERVAL)
      {
        ESP_LOGI(FsCheckObject::tag, "make an filesystem check...");
        //
        // make something
        //
        FsCheckObject::computeFilesysCheck(timestamp);
        //
        // mark the current timestamp
        //
        ESP_LOGI(FsCheckObject::tag, "update marker file <%s>...", fileName.c_str());
        gettimeofday(&val, nullptr);
        timestamp = val.tv_sec;
        FsCheckObject::updateStatFile(fileName, timestamp);
        // next iteration in a few munutes
        vTaskDelay(pdMS_TO_TICKS(Prefs::FILESYS_CHECK_TEST_INTERVAL * 1000U));
        continue;
      }
    }
  }

  uint32_t FsCheckObject::getLastTimestamp(std::string fileName)
  {
    ESP_LOGI(FsCheckObject::tag, "marker file <%s> open...", fileName.c_str());
    auto fd = fopen(fileName.c_str(), "r");
    uint32_t lastTimestamp{0UL};
    //
    if (fd)
    {
      char buffer[128];
      auto line = fgets(&buffer[0], 128, fd);
      if (strlen(line) > 0)
      {
        ESP_LOGI(FsCheckObject::tag, "marker file <%s> contains <%s>...", fileName.c_str(), line);
        std::string tmstmp{line};
        lastTimestamp = std::stoi(tmstmp);
      }
      fclose(fd);
    }
    return lastTimestamp;
  }

  bool FsCheckObject::updateStatFile(std::string fileName, uint32_t timestamp)
  {
    auto fd = fopen(fileName.c_str(), "w");
    if (fd)
    {
      auto wval = std::to_string(timestamp);
      fputs(wval.c_str(), fd);
      fclose(fd);
      return true;
    }
    else
    {
      ESP_LOGE(FsCheckObject::tag, "can't create marker file <%s>...", fileName.c_str());
      return false;
    }
  }

  bool FsCheckObject::checkExistStatFile(std::string fileName, uint32_t timestamp)
  {
    struct stat file_stat;

    if (stat(fileName.c_str(), &file_stat) != 0)
    {
      timestamp = FsCheckObject::getMidnight(timestamp);
      // File not exist, set midnight (GMT) as first marker
      ESP_LOGI(FsCheckObject::tag, "create marker file <%s>...", fileName.c_str());
      auto fd = fopen(fileName.c_str(), "w");
      if (fd)
      {
        auto wval = std::to_string(timestamp);
        fputs(wval.c_str(), fd);
        fclose(fd);
      }
      else
      {
        ESP_LOGE(FsCheckObject::tag, "can't create marker file <%s>...", fileName.c_str());
      }
      return false;
    }
    return true;
  }

  void FsCheckObject::computeFileWithLimit(std::string fileName, uint32_t timestamp, bool _lock = false)
  {
    struct stat file_stat;
    FILE *rFile{nullptr};
    FILE *wFile{nullptr};
    int counter{0};
    std::string delimiter_dp = ":";
    std::string delimiter_str = "\"";
    std::string delimiter_comma = ",";
    std::string tempFileName(Prefs::WEB_TEMP_FILE);
    ESP_LOGI(FsCheckObject::tag, "check the file <%s> for obsolete data...", fileName.c_str());

    // if exist, of course
    if (stat(fileName.c_str(), &file_stat) == 0)
    {
      //
      // file exist
      //
      ESP_LOGI(FsCheckObject::tag, "file <%s> exist...", fileName.c_str());
      // 30 days back
      uint32_t border_timestamp = timestamp - (30 * 24 * 60 * 60);
      rFile = fopen(fileName.c_str(), "r");
      if (rFile)
      {
        //
        // file is open for read
        //
        counter = 0;
        char buffer[128];
        auto c_line = fgets(&buffer[0], 128, rFile);
        while (c_line)
        {
          std::string lineStr{c_line};
          ESP_LOGI(FsCheckObject::tag, "line <%s>...", c_line);
          // search first double point
          int dp_pos;
          if ((dp_pos = lineStr.find(delimiter_dp)) != std::string::npos)
          {
            // point to "\""
            ++dp_pos;
            // point to first character
            ++dp_pos;
            int str_pos;
            if ((str_pos = lineStr.find(delimiter_str, dp_pos)) != std::string::npos)
            {
              // extract value of timestring
              auto token = lineStr.substr(dp_pos, str_pos);
              auto timestamp_current = std::stoul(token);
              if (timestamp_current > border_timestamp)
              {
                // the timestamp is inner of the allowed value
                if (!wFile)
                {
                  // okay i hav to open a file
                  wFile = fopen(tempFileName.c_str(), "w");
                }
                if (wFile)
                {
                  // was open successful
                  if (counter == 0)
                  {
                    if ((str_pos = lineStr.find(delimiter_comma)) != std::string::npos)
                    {
                      if (str_pos < 3)
                      {
                        lineStr.substr(str_pos);
                      }
                    }
                  }
                  // line ready for save
                  ++counter;
                  fputs(lineStr.c_str(), wFile);
                  fputs("\n", wFile);
                  if (counter % 25 == 0)
                  {
                    // a little sleep for the other processes
                    vTaskDelay(pdMS_TO_TICKS(5));
                  }
                }
              }
            }
          }
          // next line...
          vTaskDelay(pdMS_TO_TICKS(10));
          c_line = fgets(&buffer[0], 128, rFile);
        }
        if (wFile)
          fclose(wFile);
        wFile = nullptr;
        fclose(rFile);
        rFile = nullptr;
      }
    }
    else
    {
      ESP_LOGI(FsCheckObject::tag, "file <%s> not exist, abort...", fileName.c_str());
      return;
    }
    // delete orig, rename tempfile
    if (_lock)
    {
      if (xSemaphoreTake(StatusObject::fileSem, pdMS_TO_TICKS(1000)) == pdTRUE)
      {
        ESP_LOGI(FsCheckObject::tag, "remove old and copy new file for <%s>...", fileName.c_str());
        if (remove(fileName.c_str()) == 0)
        {
          if (rename("myfile.dat", "newfile.dat") != 0)
          {
            ESP_LOGI(FsCheckObject::tag, "remove old and copy new file for <%s> successful.", fileName.c_str());
          }
          else
          {
            ESP_LOGE(FsCheckObject::tag, "rename file for <%s> failed, ABORT...", fileName.c_str());
          }
        }
        else
        {
          ESP_LOGE(FsCheckObject::tag, "remove old file for <%s> failed, ABORT...", fileName.c_str());
        }
        xSemaphoreGive(StatusObject::fileSem);
      }
      else
      {
        ESP_LOGE(FsCheckObject::tag, "can't obtain semaphore for filesystem check...");
      }
    }
  }

  /**
   * check if data depricated...
   *  { "timestamp":"1670079220", "id":"13654914070250903080", "temp":"22.937500", "humidy":"-100.000000" }
   * ,{ "timestamp":"1670079220", "id":"2918332558598846760", "temp":"23.125000", "humidy":"-100.000000" }
   * ,{ "timestamp":"1670079220", "id":"4071254063206621992", "temp":"23.125000", "humidy":"-100.000000" }
   * ,{ "timestamp":"1670079220", "id":"9907919180278756136", "temp":"23.125000", "humidy":"-100.000000" }
   * ,{ "timestamp":"1670079220", "id":"0", "temp":"22.000000", "humidy":"62.000000" }
  */
  void FsCheckObject::computeFilesysCheck(uint32_t timestamp)
  {
    // 30 days back
    uint32_t border_timestamp = timestamp - (30 * 24 * 60 * 60);
    std::string fileName(Prefs::WEB_MONTHLY_FILE);
    FsCheckObject::computeFileWithLimit(fileName, border_timestamp);
    vTaskDelay(pdMS_TO_TICKS(250));
    border_timestamp = timestamp - (7 * 24 * 60 * 60);
    fileName = std::string(Prefs::WEB_WEEKLY_FILE);
    FsCheckObject::computeFileWithLimit(fileName, border_timestamp);
    vTaskDelay(pdMS_TO_TICKS(250));
    border_timestamp = timestamp - (24 * 60 * 60);
    fileName = std::string(Prefs::WEB_DAYLY_FILE);
    FsCheckObject::computeFileWithLimit(fileName, border_timestamp, true);
    vTaskDelay(pdMS_TO_TICKS(250));
  }

  /**
   * what is the time from last midnight?
  */
  uint32_t FsCheckObject::getMidnight(uint32_t timestamp)
  {
    // how many seconds since midnight?
    uint32_t sinceMidnight = timestamp % (60 * 60 * 24);
    // midnight timestamp
    uint32_t midnightTime = timestamp - sinceMidnight;
    return midnightTime;
  }
}
