#include <string.h>
#include <string>
#include <stdio.h>
#include <sys/stat.h>
#include <esp_log.h>
#include "appPreferences.hpp"
#include "statusObject.hpp"
#include "filesystemChecker.hpp"

namespace webserver
{
  constexpr int L_BUFSIZE = 512;

  const char *FsCheckObject::tag{ "fs_checker" };
  bool FsCheckObject::is_init{ false };
  TaskHandle_t FsCheckObject::taskHandle{ nullptr };

  /**
   * start (if not running yet) the file-system-checker
   */
  void FsCheckObject::start()
  {
    if ( FsCheckObject::taskHandle )
    {
      vTaskDelete( FsCheckObject::taskHandle );
      FsCheckObject::taskHandle = nullptr;
    }
    else
    {
      // start the task, lowest priority
      xTaskCreate( FsCheckObject::filesystemTask, "fs-check-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY,
                   &FsCheckObject::taskHandle );
    }
  }

  /**
   * init a few things if not done
   */
  void FsCheckObject::init()
  {
    ESP_LOGI( FsCheckObject::tag, "init filesystem checker object..." );
    // nothing to do at this time ;-)
    FsCheckObject::is_init = true;
  }

  /**
   * infinity task....
   */
  void FsCheckObject::filesystemTask( void * )
  {
    if ( !FsCheckObject::is_init )
      FsCheckObject::init();
    // for security, if init do nothing
    StatusObject::init();
    //
    // construct filename for marker for last check
    //
    std::string fileName( Prefs::FILE_CHECK_FILE_NAME );
    // at first, do nothing, let oher things at first
    vTaskDelay( pdMS_TO_TICKS( 32109 ) );
    // infinity
    while ( true )
    {
      // wait for next garbage disposal
      vTaskDelay( pdMS_TO_TICKS( Prefs::FILESYS_CHECK_SLEEP_TIME_MS ) );
      if ( StatusObject::getWlanState() != WlanState::TIMESYNCED )
      {
        //
        // no timesync, no time for compare, next round....
        //
        continue;
      }
      if ( StatusObject::getIsBrownout() )
      {
        //
        // no current, do not write the flash!!!!
        //
        continue;
      }
      timeval val;
      uint32_t lastTimestamp{ 0UL };
      gettimeofday( &val, nullptr );
      uint32_t timestamp = val.tv_sec;
      if ( !FsCheckObject::checkExistStatFile( fileName, timestamp ) )
      {
        // file was created, next round please
        continue;
      }
      //
      // file exist
      // when was thel last running of the garbage disposal
      lastTimestamp = FsCheckObject::getLastTimestamp( fileName );
      //
      // is the last timestamp older than the difference from prefs?
      //
      if ( timestamp - lastTimestamp > Prefs::FILESYS_CHECK_INTERVAL )
      {
        ESP_LOGI( FsCheckObject::tag, "make an garbage disposal..." );
        //
        // make something
        //
        FsCheckObject::computeFilesysCheck( timestamp );
        //
        // mark the current timestamp
        //
        ESP_LOGI( FsCheckObject::tag, "update marker file <%s> (when was the garbage disposal)...", fileName.c_str() );
        gettimeofday( &val, nullptr );
        timestamp = val.tv_sec;
        FsCheckObject::updateStatFile( fileName, timestamp );
        continue;
      }
    }
  }

  uint32_t FsCheckObject::getLastTimestamp( std::string fileName )
  {
    // ESP_LOGI(FsCheckObject::tag, "marker file <%s> open...", fileName.c_str());
    auto fd = fopen( fileName.c_str(), "r" );
    uint32_t lastTimestamp{ 0UL };
    //
    if ( fd )
    {
      char buffer[ 128 ];
      auto line = fgets( &buffer[ 0 ], 128, fd );
      if ( strlen( line ) > 0 )
      {
        // get time as unix timestamp
        std::string tmstmp{ line };
        lastTimestamp = std::stoi( tmstmp );
        // ESP_LOGI(FsCheckObject::tag, "marker file <%s> contains <%s>...", fileName.c_str(), line);
      }
      fclose( fd );
    }
    return lastTimestamp;
  }

  bool FsCheckObject::updateStatFile( std::string fileName, uint32_t timestamp )
  {
    auto fd = fopen( fileName.c_str(), "w" );
    if ( fd )
    {
      // get unix timestampand write as string
      auto wval = std::to_string( timestamp );
      fputs( wval.c_str(), fd );
      fclose( fd );
      return true;
    }
    else
    {
      ESP_LOGE( FsCheckObject::tag, "can't create marker file <%s>...", fileName.c_str() );
      return false;
    }
  }

  bool FsCheckObject::checkExistStatFile( std::string fileName, uint32_t timestamp )
  {
    struct stat file_stat;

    if ( stat( fileName.c_str(), &file_stat ) != 0 )
    {
      // simulate start at midnight
      timestamp = FsCheckObject::getMidnight( timestamp );
      // File not exist, set midnight (GMT) as first marker
      ESP_LOGI( FsCheckObject::tag, "create marker file <%s>...", fileName.c_str() );
      auto fd = fopen( fileName.c_str(), "w" );
      if ( fd )
      {
        auto wval = std::to_string( timestamp );
        fputs( wval.c_str(), fd );
        fclose( fd );
      }
      else
      {
        ESP_LOGE( FsCheckObject::tag, "can't create marker file <%s>...", fileName.c_str() );
      }
      return false;
    }
    return true;
  }

  /**
   * delete original, rename temp to original
   */
  esp_err_t FsCheckObject::renameFiles( std::string &fromFileName, std::string &toFileName, SemaphoreHandle_t _sem )
  {
    ESP_LOGI( FsCheckObject::tag, "remove old and copy new file for <%s>...", fromFileName.c_str() );
    if ( xSemaphoreTake( _sem, pdMS_TO_TICKS( 15000 ) ) == pdTRUE )
    {
      //
      // remove old file (if exist)
      //
      if ( remove( toFileName.c_str() ) == 0 )
      {
        ESP_LOGI( FsCheckObject::tag, "removed <%s>, try to rename <%s> to <%s>...", toFileName.c_str(), fromFileName.c_str(),
                  toFileName.c_str() );
      }
      else
      {
        ESP_LOGE( FsCheckObject::tag, "remove file <%s> failed...", toFileName.c_str() );
      }
      //
      // move file to old file
      //
      if ( rename( fromFileName.c_str(), toFileName.c_str() ) == 0 )
      {
        ESP_LOGI( FsCheckObject::tag, "move <%s> file to <%s> successful.", fromFileName.c_str(), toFileName.c_str() );
        // semaphore release
        xSemaphoreGive( _sem );
        return ESP_OK;
      }
      else
      {
        ESP_LOGE( FsCheckObject::tag, "move file <%s> to <%s> failed, ABORT...", fromFileName.c_str(), toFileName.c_str() );
      }
      // semaphore release
      xSemaphoreGive( _sem );
      return ESP_OK;
    }
    else
    {
      ESP_LOGE( FsCheckObject::tag, "can't obtain semaphore for filesystem check..." );
    }
    return ESP_FAIL;
  }

  /**
   * check if data depricated...
   */
  void FsCheckObject::computeFilesysCheck( uint32_t timestamp )
  {
    std::string fromFileName( Prefs::WEB_DAYLY_FILE_01 );
    std::string toFileName( Prefs::WEB_DAYLY_FILE_02 );
    FsCheckObject::renameFiles( fromFileName, toFileName, StatusObject::measureFileSem );
    vTaskDelay( pdMS_TO_TICKS( 250 ) );

    fromFileName = std::string( Prefs::ACKU_LOG_FILE_01 );
    toFileName = std::string( Prefs::ACKU_LOG_FILE_02 );
    FsCheckObject::renameFiles( fromFileName, toFileName, StatusObject::ackuFileSem );
    vTaskDelay( pdMS_TO_TICKS( 250 ) );
  }

  /**
   * what is the time from last midnight?
   */
  uint32_t FsCheckObject::getMidnight( uint32_t timestamp )
  {
    // how many seconds since midnight?
    uint32_t sinceMidnight = timestamp % ( 60 * 60 * 24 );
    // midnight timestamp
    uint32_t midnightTime = timestamp - sinceMidnight;
    return midnightTime;
  }
}  // namespace webserver
