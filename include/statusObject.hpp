#pragma once
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <SPIFFS.h>
#include "appPreferences.hpp"

namespace EnvServer
{
  class StatusObject
  {
    private:
    static const char *tag;        //! TAG for esp log
    static bool is_init;           //! was object initialized
    static bool is_running;        //! is save Task running?
    static bool is_spiffs;         //! is fikesystem okay?
    static WlanState wlanState;    //! is wlan disconnected, connected etc....
    static MeasureState msgState;  //! which state ist the mesure
    static bool http_active;       //! was an acces via http?
    static int currentVoltage;     //! current voltage in mV
    static bool isBrownout;        //! is voltage too low
    static bool isLowAcku;         //! is low acku
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
      return ( is_spiffs );
    }
    static void setVoltage( int );
    static int getVoltage();
    static bool getLowAcku();

    private:
    static void saveTask( void * );
  };
}  // namespace EnvServer
