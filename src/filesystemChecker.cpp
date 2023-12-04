#include <SPIFFS.h>
#include <stdlib.h>
#include "appPreferences.hpp"
#include "statics.hpp"
#include "statusObject.hpp"
#include "filesystemChecker.hpp"

namespace EnvServer
{
  constexpr int L_BUFSIZE = 769;

  const char *FsCheckObject::tag{ "FsCheckObject" };
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
      elog.log( INFO, "%s: start filesystemchecker task...", FsCheckObject::tag );
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
    elog.log( INFO, "%s: init filesystemchecker task...", FsCheckObject::tag );
    // nothing to do at this time ;-)
    FsCheckObject::is_init = true;
    FsCheckObject::getFileInfo();
  }

  /**
   * check the file sizes
   */
  void FsCheckObject::getFileInfo()
  {
    File fd;
    String fileName;
    elog.log( DEBUG, "%s: get file infos...", FsCheckObject::tag );
    StatusObject::setFsTotalSpace( SPIFFS.totalBytes() );
    elog.log( DEBUG, "%s: total SPIFFS space: %07d", FsCheckObject::tag, StatusObject::getFsTotalSpace() );
    StatusObject::setFsUsedSpace( SPIFFS.usedBytes() );
    elog.log( DEBUG, "%s: total SPIFFS space: %07d", FsCheckObject::tag, StatusObject::getFsUsedSpace() );

    if ( xSemaphoreTake( StatusObject::measureFileSem, pdMS_TO_TICKS( 1500 ) ) == pdTRUE )
    {
      // dayly file
      fileName = String( Prefs::WEB_DAYLY_FILE );
      fd = SPIFFS.open( fileName, "r", false );
      if ( fd )
      {
        elog.log( DEBUG, "%s: file %s opened, check size...", FsCheckObject::tag, fileName.c_str() );
        StatusObject::setTodayFileSize( fd.size() );
        fd.close();
        elog.log( DEBUG, "%s: file %s size is %07d...", FsCheckObject::tag, fileName.c_str(), StatusObject::getTodayFilseSize() );
      }
      else
        elog.log( DEBUG, "%s: file %s can't open!", FsCheckObject::tag, fileName.c_str() );
      // weekly file
      fileName = String( Prefs::WEB_WEEKLY_FILE );
      fd = SPIFFS.open( fileName, "r", false );
      if ( fd )
      {
        elog.log( DEBUG, "%s: file %s opened, check size...", FsCheckObject::tag, fileName.c_str() );
        StatusObject::setWeekFileSize( fd.size() );
        fd.close();
        elog.log( DEBUG, "%s: file %s size is %07d...", FsCheckObject::tag, fileName.c_str(), StatusObject::getWeekFilseSize() );
      }
      else
        elog.log( DEBUG, "%s: file %s can't open!", FsCheckObject::tag, fileName.c_str() );
      // month file
      fileName = String( Prefs::WEB_MONTHLY_FILE );
      fd = SPIFFS.open( fileName, "r", false );
      if ( fd )
      {
        elog.log( DEBUG, "%s: file %s opened, check size...", FsCheckObject::tag, fileName.c_str() );
        StatusObject::setMonthFileSize( fd.size() );
        fd.close();
        elog.log( DEBUG, "%s: file %s size is %07d...", FsCheckObject::tag, fileName.c_str(), StatusObject::getMonthFilseSize() );
      }
      else
        elog.log( DEBUG, "%s: file %d can't open!", FsCheckObject::tag, fileName.c_str() );
      // semaphore release
      xSemaphoreGive( StatusObject::measureFileSem );
    }

    if ( xSemaphoreTake( StatusObject::ackuFileSem, pdMS_TO_TICKS( 15000 ) ) == pdTRUE )
    {
      // acku file
      fileName = String( Prefs::ACKU_LOG_FILE_01 );
      fd = SPIFFS.open( fileName, "r", false );
      if ( fd )
      {
        elog.log( DEBUG, "%s: file %s opened, check size...", FsCheckObject::tag, fileName.c_str() );
        StatusObject::setAckuFileSize( fd.size() );
        fd.close();
        elog.log( DEBUG, "%s: file %s size is %07d...", FsCheckObject::tag, fileName.c_str(), StatusObject::getAckuFilseSize() );
      }
      else
        elog.log( DEBUG, "%s: file %s can't open!", FsCheckObject::tag, fileName.c_str() );
      // semaphore release
      xSemaphoreGive( StatusObject::ackuFileSem );
    }
    elog.log( DEBUG, "%s: get file infos...OK", FsCheckObject::tag );
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
    elog.log( INFO, "%s: start filesystemchecker... runnning... ", FsCheckObject::tag );
    String fileName( Prefs::FILE_CHECK_FILE_NAME );
    // at first, do nothing, let oher things at first
    delay( 32109 );
    // infinity
    while ( true )
    {
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
      //
      lastTimestamp = FsCheckObject::getLastTimestamp( fileName );
      //
      // is the last timestamp older than the difference from prefs?
      //
      if ( ( timestamp - lastTimestamp > Prefs::FILESYS_CHECK_INTERVAL ) || StatusObject::getFsCheckReq() )
      {
        elog.log( INFO, "%s: make an garbage disposal...", FsCheckObject::tag );
        //
        // make something
        //
        StatusObject::setFsCheckReq( false );
        FsCheckObject::computeFilesysCheck( timestamp );
        //
        // mark the current timestamp
        //
        elog.log( INFO, "%s: update marker file <%s> (when was the garbage disposal)......", FsCheckObject::tag, fileName.c_str() );
        gettimeofday( &val, nullptr );
        timestamp = val.tv_sec;
        FsCheckObject::updateStatFile( fileName, timestamp );
        continue;
      }
    }
  }

  /**
   * read the timestamp from file
   */
  uint32_t FsCheckObject::getLastTimestamp( const String &fileName )
  {
    elog.log( DEBUG, "%s: marker file <%s> open......", FsCheckObject::tag, fileName.c_str() );
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
        elog.log( DEBUG, "%s: marker file <%s> countains <%s> (%d)...", FsCheckObject::tag, fileName.c_str(), line.c_str(),
                  lastTimestamp );
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
    elog.log( DEBUG, "%s: update marker file <%s>, open......", FsCheckObject::tag, fileName.c_str() );
    auto fd = SPIFFS.open( fileName, "w", true );
    if ( fd )
    {
      char *result = ltoa( timestamp, cPtr, 10 );
      size_t len = strlen( result );
      // get unix timestampand write as string
      fd.write( buffer, len );
      fd.close();
      elog.log( DEBUG, "%s: update marker file <%s> DONE.", FsCheckObject::tag, fileName.c_str() );
      return true;
    }
    else
    {
      elog.log( ERROR, "%s: can't update marker file <%s>!", FsCheckObject::tag, fileName.c_str() );
      return false;
    }
  }

  /**
   * check if marker fiel exist, if not, create one
   */
  bool FsCheckObject::checkExistStatFile( const String &fileName, uint32_t timestamp )
  {
    struct stat file_stat;

    String virtualFileName = String( Prefs::WEB_PATH ) + fileName;
    if ( stat( virtualFileName.c_str(), &file_stat ) != 0 )
    {
      // simulate start at midnight
      timestamp = FsCheckObject::getMidnight( timestamp );
      // File not exist, set midnight (GMT) as first marker
      elog.log( DEBUG, "%s: create marker file <%s> DONE.", FsCheckObject::tag, fileName.c_str() );
      return FsCheckObject::updateStatFile( fileName, timestamp );
    }
    return true;
  }

  /**
   * delete original, rename temp to original
   */
  bool FsCheckObject::renameFiles( const String &fromFileName, const String &toFileName, SemaphoreHandle_t _sem )
  {
    bool retVal{ false };
    elog.log( DEBUG, "%s: remove old and copy new file for <%s>...", FsCheckObject::tag, fromFileName.c_str() );
    if ( xSemaphoreTake( _sem, pdMS_TO_TICKS( 15000 ) ) == pdTRUE )
    {
      //
      // remove old file (if exist)
      //
      if ( SPIFFS.remove( toFileName ) )
      {
        elog.log( DEBUG, "%s: removed <%s>, try to rename <%s> to <%s>......", FsCheckObject::tag, toFileName.c_str(),
                  fromFileName.c_str(), toFileName.c_str() );
      }
      else
      {
        elog.log( INFO, "%s: remove file <%s> failed (not exist?)......", FsCheckObject::tag, toFileName.c_str() );
      }
      //
      // move file to old file
      //
      if ( SPIFFS.rename( fromFileName, toFileName ) )
      {
        elog.log( DEBUG, "%s: move <%s> file to <%s> successful.", FsCheckObject::tag, fromFileName.c_str(), toFileName.c_str() );
        retVal = true;
      }
      else
      {
        elog.log( ERROR, "%s: move <%s> file to <%s> failed! ABORT!.", FsCheckObject::tag, fromFileName.c_str(), toFileName.c_str() );
        retVal = false;
      }
      // semaphore release
      xSemaphoreGive( _sem );
      return retVal;
    }
    else
    {
      elog.log( ERROR, "%s: can't obtain semaphore for filesystem check...", FsCheckObject::tag );
    }
    return false;
  }

  /**
   * check if data depricated...
   * TODO: make with no doublettes
   */
  void FsCheckObject::computeFilesysCheck( uint32_t timestamp )
  {
    //
    // at first make the weekly file
    // make an border timestamp for an border at a week (7 days)
    // and delete data before that
    //
    uint32_t borderTimestamp = FsCheckObject::getMidnight( timestamp ) - ( 68400 * 7 );
    String fromFile( Prefs::WEB_DAYLY_FILE );
    String toFile( Prefs::WEB_WEEKLY_FILE );
    if ( !FsCheckObject::separateFromBorder( fromFile, toFile, StatusObject::measureFileSem, borderTimestamp ) )
    {
      elog.log( ERROR, "%s: can't separate dayly from weekly data!", FsCheckObject::tag );
      return;
    }
    delay( 2500 );
    //
    // first make a border timestamp for data in dayly,
    // data where older as this timestamp have to copy in weekly data
    //
    borderTimestamp = FsCheckObject::getMidnight( timestamp ) - ( 68400 * 31 );
    fromFile = String( Prefs::WEB_WEEKLY_FILE );
    toFile = String( Prefs::WEB_MONTHLY_FILE );
    if ( !FsCheckObject::separateFromBorder( fromFile, toFile, StatusObject::measureFileSem, borderTimestamp ) )
    {
      elog.log( ERROR, "%s: cant separate dayly from weekly data!", FsCheckObject::tag );
      return;
    }
    delay( 2500 );
    //
    // check the length of the acku file
    // longest a week
    //
    borderTimestamp = FsCheckObject::getMidnight( timestamp ) - ( 68400 * 31 );
    fromFile = String( Prefs::ACKU_LOG_FILE_01 );
    toFile = String( "dummy" );
    if ( !FsCheckObject::separateFromBorder( fromFile, toFile, StatusObject::ackuFileSem, borderTimestamp ) )
    {
      elog.log( ERROR, "%s: cant separate dayly from weekly data!", FsCheckObject::tag );
      return;
    }

    delay( 2500 );
    //
    // last, check length of all files
    //
    FsCheckObject::getFileInfo();
  }

  bool FsCheckObject::separateFromBorder( String &fromFile, String &toFile, SemaphoreHandle_t sem, uint32_t borderTimestamp )
  {
    //
    // first open a fresh temfile
    //
    bool retVal{ true };
    bool isDestFile{ true };
    String tempFile( Prefs::WEB_TEMP_FILE );
    File fdTemp;
    File fdFrom;
    File fdDest;

    isDestFile = !toFile.equals( "dummy" );
    //
    // part one, separate from source to temp and dest
    //
    elog.log( INFO, "%s: separate data from file <%s>, part one", FsCheckObject::tag, fromFile.c_str() );
    if ( xSemaphoreTake( sem, pdMS_TO_TICKS( 15000 ) ) == pdTRUE )
    {
      // open temp file
      elog.log( INFO, "%s: open tempfile <%s> for writing...", FsCheckObject::tag, tempFile.c_str() );
      fdTemp = SPIFFS.open( tempFile, "w", true );
      if ( !fdTemp )
      {
        elog.log( ERROR, "%s: open tempfile <%s> for writing FAILED!", FsCheckObject::tag, tempFile.c_str() );
        retVal = false;
      }
      if ( retVal )
      {
        // open source file
        elog.log( INFO, "%s: open source file <%s> for reading...", FsCheckObject::tag, fromFile.c_str() );
        fdFrom = SPIFFS.open( fromFile, "r", false );
        if ( !fdFrom )
        {
          elog.log( ERROR, "%s: open source file <%s> for reading FAILED!", FsCheckObject::tag, fromFile.c_str() );
          fdTemp.close();
          retVal = false;
        }
      }
      if ( retVal )
      {
        if ( isDestFile )
        {
          // open destination file for append
          elog.log( INFO, "%s: open destination file <%s> for appending...", FsCheckObject::tag, toFile.c_str() );
          fdDest = SPIFFS.open( toFile, "a", false );
          if ( !fdDest )
          {
            elog.log( ERROR, "%s: open destination file <%s> for appending FAILED!", FsCheckObject::tag, fromFile.c_str() );
            elog.log( INFO, "%s: try open and create destination file <%s>...", FsCheckObject::tag, fromFile.c_str() );
            fdDest = SPIFFS.open( toFile, "a", true );
            if ( !fdDest )
            {
              elog.log( ERROR, "%s: open and create destination file <%s> FAILED!", FsCheckObject::tag, fromFile.c_str() );
              fdTemp.close();
              fdFrom.close();
              retVal = false;
            }
          }
        }
      }

      if ( retVal )
      {
        uint8_t buffer[ L_BUFSIZE ];
        const uint8_t *startPtr = buffer;
        uint8_t *currPtr = buffer;
        const uint8_t *endPtr = &buffer[ L_BUFSIZE - 1 ];
        memset( buffer, 0, L_BUFSIZE );
        // files are opened.
        // {"ti":"1700994992","da":[{"id":"28FF07A1A11603C6","te":"21.1","hu":"-100.0"},{"id":"28FF7F689316042D","te":"21.8","hu":"-100.0"},{"id":"HUM","te":"21.0","hu":"48.0"}]}
        while ( fdFrom.available() )
        {
          uint8_t input = static_cast< uint8_t >( fdFrom.read() );
          if ( currPtr < endPtr )
          {
            //
            // if buffer NOT full
            //
            *currPtr = input;
            currPtr++;
            //
            // is this an newline?
            //
            if ( input == '\n' )
            {
              // descide if temp or dest
              // have to find timestamp
              // mak a string from buffer
              String line( reinterpret_cast< char * >( &buffer[ 0 ] ) );
              uint timestampStart = line.indexOf( "\"ti\":\"" ) + 6;
              uint timestampEnd = line.indexOf( "\",\"da\"" );
              size_t len = currPtr - startPtr;
              if ( timestampStart > 1 && timestampEnd > 5 )
              {
                String timestampString = line.substring( timestampStart, timestampEnd );
                // elog.log( DEBUG, "%s: timestamp %s", FsCheckObject::tag, timestampString.c_str() );
                //  save ram
                line.clear();
                uint32_t timestamp = timestampString.toInt();
                if ( timestamp < borderTimestamp )
                {
                  // older data to destination
                  // elog.log( DEBUG, "%s: timestamp %s write to destfile", FsCheckObject::tag, timestampString.c_str() );
                  if ( isDestFile )
                  {
                    fdDest.write( startPtr, len );
                  }
                }
                else
                {
                  // younger data to temp, leave later in current file
                  // elog.log( DEBUG, "%s: timestamp %s write to tempfile", FsCheckObject::tag, timestampString.c_str() );
                  fdTemp.write( startPtr, len );
                }
              }
              else
              {
                //
                // if the search war negative
                //
                elog.log( DEBUG, "%s: timestamp write to tempfile, because not found marker", FsCheckObject::tag );
                fdTemp.write( startPtr, len );
              }
              // next round
              currPtr = buffer;
              memset( buffer, 0, L_BUFSIZE );
              continue;
            }
          }
          else
          {
            //
            // cant' happen, i think ;-)
            //
            elog.log( ERROR, "%s: buffer overflow while searching newline char..." );
            // end this routine
            retVal = false;
            break;
          }
        }
        //
        // if the buffer contains data
        //
        size_t len = currPtr - startPtr;
        if ( len > 0 )
        {
          // write data to dest
          fdTemp.write( startPtr, len );
        }
      }
      fdFrom.close();
      fdTemp.close();
      if ( isDestFile )
        fdDest.close();
      // semaphore release
      xSemaphoreGive( sem );
    }
    else
    {
      elog.log( ERROR, "%s: can't obtain semaphore for filesystem check...", FsCheckObject::tag );
      retVal = false;
    }
    if ( retVal )
    {
      delay( 1200 );
      //
      // Part two, rename tempfile to fromfile
      //
      elog.log( INFO, "%s: separate data from file <%s>, part two", FsCheckObject::tag, fromFile.c_str() );
      retVal = FsCheckObject::renameFiles( tempFile, fromFile, sem );
      if ( retVal )
        elog.log( DEBUG, "%s: separate data from file <%s>, part two SUCCESS", FsCheckObject::tag, fromFile.c_str() );
    }
    return retVal;
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
