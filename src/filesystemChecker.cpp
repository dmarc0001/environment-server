#include <SPIFFS.h>
#include <stdlib.h>
#include <esp_spiffs.h>
#include <regex>
#include "appPreferences.hpp"
#include "statusObject.hpp"
#include "filesystemChecker.hpp"

namespace EnvServer
{
  constexpr int L_BUFSIZE = 769;

  const char *FsCheckObject::tag{ "FsCheckObject" };
  bool FsCheckObject::is_init{ false };
  TaskHandle_t FsCheckObject::taskHandle{ nullptr };
  int FsCheckObject::todayDay{ -1 };

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
      logger.log( Prefs::LOGID, INFO, "%s: start filesystemchecker task...", FsCheckObject::tag );
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
    logger.log( Prefs::LOGID, INFO, "%s: init filesystemchecker task...", FsCheckObject::tag );
    // nothing to do at this time ;-)
    FsCheckObject::is_init = true;
    if ( FsCheckObject::checkFileSysSizes() )
    {
      // TODO: filesystem check and shrink
    }
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
    logger.log( Prefs::LOGID, INFO, "%s: start filesystemchecker... runnning... ", FsCheckObject::tag );
    String fileName( Prefs::FILE_CHECK_FILE_NAME );
    // at first, do nothing, let oher things at first
    delay( 32109 );
    // infinity
    while ( true )
    {
      if ( StatusObject::getWlanState() != WlanState::TIMESYNCED )
      {
        //
        // no timesync, no time for compare, next round....
        //
        continue;
      }
      FsCheckObject::getTodayFileName();
      // wait for next garbage disposal or request
      for ( uint8_t i = 0; i < Prefs::FILESYS_CHECK_SLEEP_TIME_MULTIPLIER; ++i )
      {
        if ( StatusObject::getFsCheckReq() )
        {
          //
          // if an request for filesystem check?
          //
          break;
        }
        delay( Prefs::FILESYS_CHECK_SLEEP_TIME_MS );
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
      //
      lastTimestamp = FsCheckObject::getLastTimestamp( fileName );
      //
      // is the last timestamp older than the difference from prefs?
      //
      if ( ( timestamp - lastTimestamp > Prefs::FILESYS_CHECK_INTERVAL ) || StatusObject::getFsCheckReq() )
      {
        logger.log( Prefs::LOGID, INFO, "%s: make an garbage disposal...", FsCheckObject::tag );
        //
        // make something
        //
        StatusObject::setFsCheckReq( false );
        FsCheckObject::computeFilesysCheck( timestamp );
        //
        // mark the current timestamp
        //
        logger.log( Prefs::LOGID, INFO, "%s: update marker file <%s> (when was the garbage disposal)......", FsCheckObject::tag,
                    fileName.c_str() );
        gettimeofday( &val, nullptr );
        timestamp = val.tv_sec;
        FsCheckObject::updateStatFile( fileName, timestamp );
        continue;
      }
    }
  }

  /**
   * check filesystem sizes and free space
   */
  int FsCheckObject::checkFileSysSizes()
  {
    size_t flash_total;
    size_t flash_used;
    size_t flash_free;

    logger.log( Prefs::LOGID, DEBUG, "%s: check filesystem size", FsCheckObject::tag );
    esp_err_t errorcode = esp_spiffs_info( Prefs::WEB_PARTITION_LABEL, &flash_total, &flash_used );
    if ( errorcode == ESP_OK )
    {
      StatusObject::setFsTotalSpace( flash_total );
      StatusObject::setFsUsedSpace( flash_used );
      flash_free = flash_total - flash_used;
      logger.log( Prefs::LOGID, DEBUG, "%s: SPIFFS total %07d, used %07d, free %07d, min-free: %07d", FsCheckObject::tag, flash_total,
                  flash_used, flash_free, Prefs::FILESYS_MIN_FILE_SYSTEM_FREE_SIZE );
      //
      // TODO: esp_err_t esp_spiffs_gc(const char *partition_label, size_t size_to_gc)
      //
      if ( Prefs::FILESYS_MIN_FILE_SYSTEM_FREE_SIZE > flash_free )
      {
        logger.log( Prefs::LOGID, WARNING, "%s: free memory too low, action needed", FsCheckObject::tag );
        return -1;
      }
      return 0;
    }
    else
    {
      logger.log( Prefs::LOGID, ERROR, "%s: can't check SPIFFS memory!", FsCheckObject::tag );
    }
    return -1;
  }

  /**
   * read the timestamp from file
   */
  uint32_t FsCheckObject::getLastTimestamp( const String &fileName )
  {
    logger.log( Prefs::LOGID, DEBUG, "%s: marker file <%s> open......", FsCheckObject::tag, fileName.c_str() );
    auto fd = SPIFFS.open( fileName, "r", false );
    uint32_t lastTimestamp{ 0UL };
    //
    if ( fd )
    {
      char buffer[ 42 ];
      size_t len = fd.readBytes( buffer, 42 );
      // buffer[ len ] = 0;
      String line( buffer, len );
      if ( len > 0 && line.length() > 0 )
      {
        // get time as unix timestamp
        lastTimestamp = static_cast< uint32_t >( line.toInt() );
        logger.log( Prefs::LOGID, DEBUG, "%s: marker file <%s> countains <%s> (%d)...", FsCheckObject::tag, fileName.c_str(),
                    line.c_str(), lastTimestamp );
      }
      fd.close();
    }
    return lastTimestamp;
  }

  /**
   * write an timestamp in a (new) file
   */
  bool FsCheckObject::updateStatFile( const String &fileName, uint32_t timestamp )
  {
    uint8_t buffer[ 32 ];
    char *cPtr = reinterpret_cast< char * >( &buffer[ 0 ] );
    logger.log( Prefs::LOGID, DEBUG, "%s: update marker file <%s>, open......", FsCheckObject::tag, fileName.c_str() );
    auto fd = SPIFFS.open( fileName, "w", true );
    if ( fd )
    {
      char *result = ltoa( timestamp, cPtr, 10 );
      size_t len = strlen( result );
      // get unix timestampand write as string
      fd.write( buffer, len );
      fd.close();
      logger.log( Prefs::LOGID, DEBUG, "%s: update marker file <%s> DONE.", FsCheckObject::tag, fileName.c_str() );
      return true;
    }
    else
    {
      logger.log( Prefs::LOGID, ERROR, "%s: can't update marker file <%s>!", FsCheckObject::tag, fileName.c_str() );
      return false;
    }
  }

  /**
   * check if marker fiel exist, if not, create one
   */
  bool FsCheckObject::checkExistStatFile( const String &fileName, uint32_t timestamp )
  {
    struct stat file_stat;

    String virtualFileName = String( Prefs::MOUNTPOINT ) + fileName;
    if ( stat( virtualFileName.c_str(), &file_stat ) != 0 )
    {
      // simulate start at midnight
      timestamp = FsCheckObject::getMidnight( timestamp );
      // File not exist, set midnight (GMT) as first marker
      logger.log( Prefs::LOGID, DEBUG, "%s: create marker file <%s> DONE.", FsCheckObject::tag, fileName.c_str() );
      return FsCheckObject::updateStatFile( fileName, timestamp );
    }
    return true;
  }

  /**
   * check if data depricated...
   */
  void FsCheckObject::computeFilesysCheck( uint32_t timestamp )
  {
    //
    // check the free space on the flash and make some checks
    //
    if ( FsCheckObject::checkFileSysSizes() == 0 )
    {
      //
      // no problem...
      //
      return;
    }
    //
    // oh, i have something to do
    //
    logger.log( Prefs::LOGID, INFO, "%s: there is no enough space on flash device, make som tasks...!", FsCheckObject::tag );
    //
    // find files in path Prefs::DATA_PATH
    //
    // give root path
    File root = SPIFFS.open( String( Prefs::WEB_DATA_PATH ).substring( 0, strlen( Prefs::WEB_DATA_PATH ) - 1 ) );
    std::regex reg( Prefs::WEB_DAYLY_FILE_PATTERN );
    std::smatch match;
    std::string fname( root.getNextFileName().c_str() );
    std::vector< String > fileList;
    logger.log( Prefs::LOGID, INFO, "%s: filesystem check, search older files...", FsCheckObject::tag );
    //
    // find my files in filenames
    //
    while ( !fname.empty() )
    {
      logger.log( Prefs::LOGID, DEBUG, "%s: === found file <%s>", FsCheckObject::tag, fname.c_str() );
      //
      // is the Filename like my pattern
      //
      if ( std::regex_search( fname, match, reg ) )
      {
        //
        // filename matches
        // store in Vector
        //
        String matchedFileName( fname.c_str() );
        logger.log( Prefs::LOGID, DEBUG, "%s: +++ found file who match <%s> as delete candidate...", FsCheckObject::tag,
                    fname.c_str() );
        fileList.push_back( matchedFileName );
      }
      fname = root.getNextFileName().c_str();
      delay( 10 );
    }
    root.close();
    if ( fileList.empty() )
      return;
    //
    // TODO: try to delete only the oldest file(s)
    // okay check files against current
    //
    int prefix = strlen( Prefs::WEB_DATA_PATH );
    //
    // all filenames
    //
    for ( String fileName : fileList )
    {
      //
      // make from filename a dateTime
      //
      String nameShort = fileName.substring( prefix, prefix + 10 );
      String currentShort = StatusObject::getTodayFileName().substring( prefix, prefix + 10 );
      if ( nameShort.compareTo( currentShort ) != 0 )
      {
        logger.log( Prefs::LOGID, INFO, "%s: file <%s> is too old, delete it!", FsCheckObject::tag, nameShort.c_str() );
        SPIFFS.remove( fileName );
      }
      delay( 10 );
    }
    return;
  }

  /**
   * get the filename for today
   */
  const String &FsCheckObject::getTodayFileName()
  {
      //  ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec 
      struct tm ti;
      getLocalTime( &ti );
    if ( FsCheckObject::todayDay != ti.tm_mday )
    {
      logger.log( Prefs::LOGID, DEBUG, "%s: create measure today file name!", FsCheckObject::tag );
      char buffer[ 28 ];
      FsCheckObject::todayDay = ti.tm_mday;
      snprintf( buffer, 28, Prefs::WEB_DAYLY_MEASURE_FILE_NAME, ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday );
      String fileName( Prefs::WEB_DATA_PATH );
      fileName += String( buffer );
      StatusObject::setTodayFileName( fileName );
      logger.log( Prefs::LOGID, DEBUG, "%s: measure today file name <%s>", FsCheckObject::tag, fileName.c_str() );
    }
    return StatusObject::getTodayFileName();
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

}  // namespace EnvServer
