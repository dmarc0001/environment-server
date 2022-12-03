#include <string.h>
#include <string>
#include <esp_log.h>
#include "appPreferences.hpp"
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
      xTaskCreate(FsCheckObject::filesystemTask, "fs-check-task", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY, NULL);
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
    //
    // construct filename for marker for last check
    //
    struct stat file_stat;
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
        // next iteration
        continue;
      }
      //
      // file exist
      //
      ESP_LOGI(FsCheckObject::tag, "marker file <%s> open...", fileName.c_str());
      auto fd = fopen(fileName.c_str(), "r");
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
        // next iteration
        continue;
      }
    }
  }

  /**
   * check if data depricated...
  */
  void FsCheckObject::computeFilesysCheck(uint32_t timestamp)
  {
    // NOPE
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
