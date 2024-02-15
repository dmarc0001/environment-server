#pragma once
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <SPIFFS.h>
#include "appPreferences.hpp"

namespace EnvServer
{
  /*
  ### json object ###
  {
    "ti":"1707679021",
    "da":
    [
      {
        "id":"28FF07A1A11603C6",
        "te":"20.8",
        "hu":"-100.0"
      },
      {
        "id":"28FF7F689316042D",
        "te":"20.7",
        "hu":"-100.0"
      },
      {
        "id":"HUM",
        "te":"21.0",
        "hu":"52.0"
      }
    ]
    }
  */
  class StatusObject
  {
    private:
    static const char *tag;         //! TAG for esp log
    static bool is_init;            //! was object initialized
    static bool is_running;         //! is save Task running?
    static bool is_spiffs;          //! is fikesystem okay?
    static WlanState wlanState;     //! is wlan disconnected, connected etc....
    static MeasureState msgState;   //! which state ist the mesure
    static bool http_active;        //! was an acces via http?
    static int currentVoltage;      //! current voltage in mV
    static bool isBrownout;         //! is voltage too low
    static bool isLowAcku;          //! is low acku
    static bool isFilesystemcheck;  // is an filesystemcheck running or requested
    static bool isDataSend;         //! should data send to a server
    static size_t todayFileSize;    // filesize of the today.json file
    static size_t weekFileSize;     // filesize of the weekly.json file
    static size_t monthFileSize;    // filesize of the month.json file
    static size_t ackuFileSize;     //! filesize acku trace file
    static size_t fsTotalSpace;     //! total space in FS
    static size_t fsUsedSpace;      //! free Space in FS

    public:
    static std::shared_ptr< env_dataset > dataset;  //! set of mesures
    static SemaphoreHandle_t measureFileSem;        // is access to files busy
    static SemaphoreHandle_t ackuFileSem;           // is access to files busy

    public:
    static void init();
    static void start();
    static void setWlanState( WlanState );
    static void setMeasureState( MeasureState );
    static MeasureState getMeasureState();
    static WlanState getWlanState();
    static void setHttpActive( bool );
    static bool getHttpActive();
    static bool getIsBrownout();
    static bool getIsSpiffsOkay()
    {
      return ( StatusObject::is_spiffs );
    }
    static void setVoltage( int );
    static int getVoltage();
    static bool getLowAcku();
    static void setFsCheckReq( bool _fsc )
    {
      StatusObject::isFilesystemcheck = _fsc;
    }
    static bool getFsCheckReq()
    {
      return StatusObject::isFilesystemcheck;
    }
    static void setIsDataSend( bool _set )
    {
      StatusObject::isDataSend = _set;
    }
    static bool getIsDataSend()
    {
      return StatusObject::isDataSend;
    }
    static void setTodayFileSize( size_t _size )
    {
      StatusObject::todayFileSize = _size;
    }
    static size_t getTodayFilseSize()
    {
      return StatusObject::todayFileSize;
    }
    static void setWeekFileSize( size_t _size )
    {
      StatusObject::weekFileSize = _size;
    }
    static size_t getWeekFilseSize()
    {
      return StatusObject::weekFileSize;
    }
    static void setMonthFileSize( size_t _size )
    {
      StatusObject::monthFileSize = _size;
    }
    static size_t getMonthFilseSize()
    {
      return StatusObject::monthFileSize;
    }
    static void setAckuFileSize( size_t _size )
    {
      StatusObject::ackuFileSize = _size;
    }
    static size_t getAckuFilseSize()
    {
      return StatusObject::ackuFileSize;
    }
    static void setFsTotalSpace( size_t _size )
    {
      StatusObject::fsTotalSpace = _size;
    }
    static size_t getFsTotalSpace()
    {
      return StatusObject::fsTotalSpace;
    }
    static void setFsUsedSpace( size_t _size )
    {
      StatusObject::fsUsedSpace = _size;
    }
    static size_t getFsUsedSpace()
    {
      return StatusObject::fsUsedSpace;
    }

    private:
    static void saveTask( void * );
  };
}  // namespace EnvServer
