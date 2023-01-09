#pragma once
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <ds18x20.h>
#include <led_strip.h>
#include "appPreferences.hpp"
#include "statusObject.hpp"

namespace webserver
{
  class FsCheckObject
  {
    private:
    static const char *tag;  //! TAG for esp log
    static bool is_init;     //! was object initialized

    public:
    static void start();  //! start the task if is not running
    static TaskHandle_t taskHandle;

    private:
    static void init();                                                            //! init vars etc
    static void filesystemTask( void * );                                          //! the ininite task
    static bool checkExistStatFile( std::string, uint32_t );                       //! is an status file exist
    static bool updateStatFile( std::string, uint32_t );                           //! update stat file for new timestamp
    static uint32_t getLastTimestamp( std::string );                               //! get last timestamp from stat file
    static void computeFileWithLimit( std::string, uint32_t, SemaphoreHandle_t );  //! check file for borders in timestamp
    static esp_err_t renameFiles( std::string &, std::string & );                  //! rename files
    static void computeFilesysCheck( uint32_t );                                   //! compute the filesystem
    static uint32_t getMidnight( uint32_t );  //! give last midnight time in sec since 01.01.1970 (up to 2038)
  };
}  // namespace webserver
