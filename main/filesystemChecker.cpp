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
    std::string fileName( Prefs::WEB_PATH );
    fileName += "/last_fscheck.dat";
    // at first, do nothing, let oher things at first
    vTaskDelay( pdMS_TO_TICKS( 10250 ) );
    // infinity
    while ( true )
    {
      // wait for next garbage disposal
      vTaskDelay( pdMS_TO_TICKS( 900301 ) );
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
   * copy only up to the border value in a new file
   */
  void FsCheckObject::computeFileWithLimit( std::string fileName, uint32_t border_timestamp, SemaphoreHandle_t _sem )
  {
    struct stat file_stat;
    FILE *rFile{ nullptr };
    FILE *wFile{ nullptr };
    int counter{ 0 };
    std::string delimiter_dp = ":";
    std::string delimiter_str = "\"";
    std::string tempFileName( Prefs::WEB_TEMP_FILE );
    ESP_LOGI( FsCheckObject::tag, "check the file <%s> for obsolete data...", fileName.c_str() );

    // if exist, of course
    if ( stat( fileName.c_str(), &file_stat ) == 0 )
    {
      //
      // file exist
      //
      ESP_LOGI( FsCheckObject::tag, "file <%s> exist...", fileName.c_str() );
      // open the file, reading mode
      rFile = fopen( fileName.c_str(), "r" );
      if ( rFile )
      {
        //
        // file is open for read
        //
        counter = 0;
        // buffer for single line
        char buffer[ L_BUFSIZE ];
        // buffer for security clear
        memset( &buffer[ 0 ], 0, L_BUFSIZE );
        // read line
        auto c_line = fgets( &buffer[ 0 ], L_BUFSIZE, rFile );
        while ( c_line )
        {
          // convert line to String f??r edit reasons
          std::string lineStr{ c_line };
          // search first double point (after JSON_TIMESTAMP_NAME)
          int dp_pos{ 0 };
          if ( ( dp_pos = lineStr.find( delimiter_dp ) ) != std::string::npos )
          {
            // set point to first character after double quote
            ++dp_pos;
            ++dp_pos;
            // and search next double quote
            int str_pos{ 0 };
            if ( ( str_pos = lineStr.find( delimiter_str, dp_pos ) ) != std::string::npos )
            {
              // extract value of timestring
              auto token = lineStr.substr( dp_pos, ( str_pos - dp_pos ) );
              ESP_LOGI( FsCheckObject::tag, "timestamp <%s> found...", token.c_str() );
              // convert to number unsigned long
              auto timestamp_current = std::stoul( token );
              if ( timestamp_current > border_timestamp )
              {
                // the timestamp is inner of the allowed value
                if ( !wFile )
                {
                  // okay i have to open a file
                  ESP_LOGI( FsCheckObject::tag, "open temporary file <%s>...", tempFileName.c_str() );
                  wFile = fopen( tempFileName.c_str(), "w" );
                }
                if ( wFile )
                {
                  // line ready for save
                  ++counter;
                  fputs( lineStr.c_str(), wFile );
                  if ( counter % 25 == 0 )
                  {
                    // a little sleep for the other processes
                    vTaskDelay( pdMS_TO_TICKS( 5 ) );
                  }
                }
              }
              else
              {
                // value is outer the allowed value, dismiss....
                // TODO: implement
              }
            }
          }
          // small break...
          vTaskDelay( pdMS_TO_TICKS( 13 ) );
          // next line with fresh buffer...
          memset( &buffer[ 0 ], 0, L_BUFSIZE );
          c_line = fgets( &buffer[ 0 ], L_BUFSIZE, rFile );
        }
        //
        // leave all clear
        //
        if ( wFile )
          fclose( wFile );
        wFile = nullptr;
        fclose( rFile );
        rFile = nullptr;
      }
    }
    else
    {
      //
      // there is no file to open...
      //
      ESP_LOGI( FsCheckObject::tag, "file <%s> not exist, abort...", fileName.c_str() );
      return;
    }
    // delete original file, rename tempfile to original filename
    if ( _sem )
    {
      if ( xSemaphoreTake( _sem, pdMS_TO_TICKS( 15000 ) ) == pdTRUE )
      {
        FsCheckObject::renameFiles( fileName, tempFileName );
        xSemaphoreGive( _sem );
      }
      else
      {
        ESP_LOGE( FsCheckObject::tag, "can't obtain semaphore for filesystem check..." );
      }
    }
    else
    {
      FsCheckObject::renameFiles( fileName, tempFileName );
    }
  }

  /**
   * delete original, rename temp to original
   */
  esp_err_t FsCheckObject::renameFiles( std::string &fileName, std::string &tempFileName )
  {
    ESP_LOGI( FsCheckObject::tag, "remove old and copy new file for <%s>...", fileName.c_str() );
    if ( remove( fileName.c_str() ) == 0 )
    {
      ESP_LOGI( FsCheckObject::tag, "removed <%s>, try to rename <%s>...", fileName.c_str(), tempFileName.c_str() );
      if ( rename( tempFileName.c_str(), fileName.c_str() ) == 0 )
      {
        ESP_LOGI( FsCheckObject::tag, "remove old and copy new file for <%s> successful.", fileName.c_str() );
        return ESP_OK;
      }
      else
      {
        ESP_LOGE( FsCheckObject::tag, "rename file <%s> to <%s> failed, ABORT...", tempFileName.c_str(), fileName.c_str() );
      }
    }
    else
    {
      ESP_LOGE( FsCheckObject::tag, "remove old file for <%s> failed, ABORT...", fileName.c_str() );
    }
    return ESP_FAIL;
  }

  /**
   * check if data depricated...
   */
  void FsCheckObject::computeFilesysCheck( uint32_t timestamp )
  {
    // 30 days back
    // uint32_t border_timestamp = timestamp - (30 * 24 * 60 * 60);
    // std::string fileName(Prefs::WEB_MONTHLY_FILE);
    uint32_t border_timestamp = timestamp - ( 24UL * 60UL * 60UL );
    std::string fileName = std::string( Prefs::WEB_DAYLY_FILE );
    FsCheckObject::computeFileWithLimit( fileName, border_timestamp, StatusObject::measureFileSem );
    vTaskDelay( pdMS_TO_TICKS( 250 ) );
    fileName = std::string( Prefs::ACKU_LOG_FILE );
    FsCheckObject::computeFileWithLimit( fileName, border_timestamp, StatusObject::ackuFileSem );
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
