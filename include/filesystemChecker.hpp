#pragma once
#include <common.hpp>
#include <memory>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "appPreferences.hpp"
#include "statusObject.hpp"

namespace EnvServer
{
  class FsCheckObject
  {
    private:
    static const char *tag;  //! TAG for esp log
    static bool is_init;     //! was object initialized
    static int todayDay;     //! which ist todays day

    public:
    static void start();  //! start the task if is not running
    static TaskHandle_t taskHandle;

    private:
    static void init();                                          //! init vars etc
    static void filesystemTask( void * );                        //! the ininite task
    static bool checkExistStatFile( const String &, uint32_t );  //! is an status file exist
    static bool updateStatFile( const String &, uint32_t );      //! update stat file for new timestamp
    static uint32_t getLastTimestamp( const String & );          //! get last timestamp from stat file
    static void computeFilesysCheck( uint32_t );                 //! compute the filesystem
    static void getFileInfo();                                   //! get sizes for data files
    static const String &getTodayFileName();                     //! get the today file name
    static int checkFileSysSizes();                              //! check filesystem size
    static uint32_t getMidnight( uint32_t );                     //! get mitnight time
  };

  //   static SemaphoreHandle_t measureFileSem;        // is access to files busy
  // static SemaphoreHandle_t ackuFileSem;           // is access to files busy

}  // namespace EnvServer
