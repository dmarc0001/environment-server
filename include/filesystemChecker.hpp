#pragma once
#include <Arduino.h>
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

    public:
    static void start();  //! start the task if is not running
    static TaskHandle_t taskHandle;

    private:
    static void init();                                                                 //! init vars etc
    static void filesystemTask( void * );                                               //! the ininite task
    static bool checkExistStatFile( const String &, uint32_t );                         //! is an status file exist
    static bool updateStatFile( const String &, uint32_t );                             //! update stat file for new timestamp
    static uint32_t getLastTimestamp( const String & );                                 //! get last timestamp from stat file
    static bool renameFiles( const String &, const String &, SemaphoreHandle_t );       //! rename files
    static void computeFilesysCheck( uint32_t );                                        //! compute the filesystem
    static bool separateFromBorder( String &, String &, SemaphoreHandle_t, uint32_t );  //! move data before timestamp to other file
    static void getFileInfo();                                                          //! get sizes for data files
    static uint32_t getMidnight( uint32_t );  //! give last midnight time in sec since 01.01.1970 (up to 2038)
  };


    //   static SemaphoreHandle_t measureFileSem;        // is access to files busy
    // static SemaphoreHandle_t ackuFileSem;           // is access to files busy

}  // namespace EnvServer
