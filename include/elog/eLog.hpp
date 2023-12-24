#pragma once
#include <Arduino.h>
#include <vector>
#include <stdarg.h>
#include <inttypes.h>
#include <WString.h>
#include <IPAddress.h>
#include <Udp.h>

#ifndef LOGGER_DISABLE_TIME
#include <TimeLib.h>
#endif

#ifndef LOGGER_DISABLE_SPIFFS
#include <LittleFS.h>
#endif

#ifndef LOGGER_DISABLE_SD
#include <SdFat.h>
#endif

#ifndef LOG_MAX_HEX_STRING_LENGTH
#define LOG_MAX_HEX_STRING_LENGTH 250
#endif

#define LOG_SPIFFS_DIR_NAME "/logs"
#define LOG_SPIFFS_MAX_FILENAME_LEN 30

namespace logger
{
//
// syslog addendum
//
#ifndef NOT_SYSLOG
  constexpr const char *SYSLOG_NILVALUE{ "-" };
  constexpr const uint8_t SYSLOG_PROTO_IETF{ 0 };
  constexpr const uint8_t SYSLOG_PROTO_BSD{ 1 };
  constexpr const uint16_t LOG_KERN{ 0 << 3 };      /* kernel messages */
  constexpr const uint16_t LOG_USER{ 1 << 3 };      /* random user-level messages */
  constexpr const uint16_t LOG_MAIL{ 2 << 3 };      /* mail system */
  constexpr const uint16_t LOG_DAEMON{ 3 << 3 };    /* system daemons */
  constexpr const uint16_t LOG_AUTH{ 4 << 3 };      /* security/authorization messages */
  constexpr const uint16_t LOG_SYSLOG{ 5 << 3 };    /* messages generated internally by syslogd */
  constexpr const uint16_t LOG_LPR{ 6 << 3 };       /* line printer subsystem */
  constexpr const uint16_t LOG_NEWS{ 7 << 3 };      /* network news subsystem */
  constexpr const uint16_t LOG_UUCP{ 8 << 3 };      /* UUCP subsystem */
  constexpr const uint16_t LOG_CRON{ 9 << 3 };      /* clock daemon */
  constexpr const uint16_t LOG_AUTHPRIV{ 10 << 3 }; /* security/authorization messages (private) */
  constexpr const uint16_t LOG_FTP{ 11 << 3 };      /* ftp daemon */
  /* other codes through 15 reserved for system use */
  constexpr const uint16_t LOG_LOCAL0{ 16 << 3 }; /* reserved for local use */
  constexpr const uint16_t LOG_LOCAL1{ 17 << 3 }; /* reserved for local use */
  constexpr const uint16_t LOG_LOCAL2{ 18 << 3 }; /* reserved for local use */
  constexpr const uint16_t LOG_LOCAL3{ 19 << 3 }; /* reserved for local use */
  constexpr const uint16_t LOG_LOCAL4{ 20 << 3 }; /* reserved for local use */
  constexpr const uint16_t LOG_LOCAL5{ 21 << 3 }; /* reserved for local use */
  constexpr const uint16_t LOG_LOCAL6{ 22 << 3 }; /* reserved for local use */
  constexpr const uint16_t LOG_LOCAL7{ 23 << 3 }; /* reserved for local use */

  constexpr const uint16_t LOG_PRIMASK{ 0x07 };   /* mask to extract priority part (internal) */
                                                  /* extract priority */
  constexpr const uint16_t LOG_FACMASK{ 0x03f8 }; /* mask to extract facility part */
  constexpr uint16_t log_pri( uint16_t pri )
  {
    return pri & logger::LOG_PRIMASK;
  }
  constexpr uint16_t log_mask( u_int16_t pri )
  {
    return 1 << pri;
  }
  constexpr uint16_t log_makepri( u_int16_t fac, uint16_t pri )
  {
    return ( fac << 3 ) | pri;
  }
  constexpr uint16_t log_fac( uint16_t pri )
  {
    return ( pri & LOG_FACMASK ) >> 3;
  }
  // #define LOG_NFACILITIES 24 /* current number of facilities */
  constexpr uint16_t log_upto( uint16_t pri )
  {
    return ( 1 << ( pri + 1 ) ) - 1;
  }
#endif

  // Loglevels. The lower level, the more serious.
  enum Loglevel
  {
    EMERGENCY = 0,
    ALERT = 1,
    CRITICAL = 2,
    ERROR = 3,
    WARNING = 4,
    NOTICE = 5,
    INFO = 6,
    DEBUG = 7,
  };

  // Options that can be used for addFileLogging.
  enum LogFileOptions
  {
    FILE_NO_OPTIONS = 0,
    FILE_NO_STAMP = 1,
  };

  /* All user provided settings of the loginstance is stored in this structure */
  struct LogService
  {
    Stream *serialPortPtr;          // Pointer to the serial port where log messages should be sent to
    char *serialServiceName;        // The servicename that is stamped on each log message
    Loglevel serialWantedLoglevel;  // Target loglevel. Everything below or equal to this level is logged
    bool serialEnabled;             // Serial logging is enabled
    const char *sdFileName;         // Filename of the logfile on sd-card (it will be inside LOGXXXXX dir)
    Loglevel sdWantedLoglevel;      // Target loglevel. Everything below or equal to this level is logged
    LogFileOptions sdOptions;       // Options about timestamps inside the logfile
    bool sdEnabled;                 // File logging is enabled
    int32_t sdFileCreteLastTry;     // Keep track of when we last time tried to create the logfile
    Loglevel syslogWantedLoglevel;  // Target loglevel. Everything below or equal to this level is logged
    bool syslogEnabled;             //! syslog enabled

#ifndef LOGGER_DISABLE_SD
    SdFile sdFileHandle;  // sdfat filehandle for the open file
#endif

    char *spiffsServiceName;        // The servicename that is stamped on each log message
    Loglevel spiffsWantedLoglevel;  // Target loglevel. Everything below or equal to this level is logged
    bool spiffsEnabled;             // Spiffs logging enabled
#ifndef NOT_SYSLOG
    UDP *_client;                 //! clients object
    uint8_t _protocol;            //! IETF or BSD
    IPAddress _ip;                //! Server IP
    const char *_server;          //! Server name
    uint16_t _port;               //! Server Port
    const char *_deviceHostname;  //! own hostname
    const char *_appName;         //! own appname
    uint16_t _pri;                //! log prio
    uint8_t _priMask = 0xff;      //! log prio mask
    bool _isOnline;               //! is device online
#endif
  };

  /* All global settings for this lib is store in this structure */
  struct LogSettings
  {
    uint16_t maxLogMessageSize;
    uint16_t maxLogMessages;

    Stream *internalLogDevice;
    Loglevel internalLogLevel;
    bool discardMsgWhenBufferFull;
    uint32_t reportStatusEvery;

    uint32_t sdReconnectEvery;
    uint32_t sdSyncFilesEvery;
    uint32_t sdTryCreateFileEvery;

    uint32_t spiffsSyncEvery;
    uint32_t spiffsCheckSpaceEvery;
    uint32_t spiffsMinimumSpace;
  };

  /* Structure that is written to the ringbuffer. It gives the WriterTask all the information
     needed to generate the logline with stamp, level, service etc. */
  struct LogLineEntry
  {
    bool logFile;       // Do we want logging to file on this handle?
    bool logSerial;     // Do we want logging to serial on this handle?
    bool logSpiffs;     // Do we want logging to spiffs on this handle?
    bool logSyslog;     // do we log to syslog?
    Loglevel loglevel;  // Loglevel
    uint32_t logTime;   // Time in milliseconds the log message was created
    LogService *service;
    bool internalLogMessage;  // If true, this is sent to internal log device
  };

  /* Structure to store status about how the messages are sent or discarded */
  struct LogStatus
  {
    uint32_t bufferMsgAdded;
    uint32_t bufferMsgNotAdded;
    uint32_t sdMsgWritten;
    uint32_t sdMsgNotWritten;
    uint32_t sdBytesWritten;
    uint32_t spiffsMsgWritten;
    uint32_t spiffsMsgNotWritten;
    uint32_t spiffsBytesWritten;
    uint32_t serialMsgWritten;
    uint32_t serialBytesWritten;
  };

  /* Structure for holding the real human time with millisecond precision */
  struct PrecisionTime
  {
    uint16_t millisecond;
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
  };

  /* The ringbuffer thats is used for buffering data for both serial and filesystem
     its a double buffer. When you push or pop data you provide both the LogLineEntry
     structure and the LogLineMessage (Text that is actually logged)
     When critical data manipulation is done we disable interrupts to avoid
     data damage. We disable this as short as possible */
  class LogRingBuff
  {
    public:
    void createBuffer( size_t logLineCapacity, size_t logLineMessageSize );
    bool push( const LogLineEntry &logLineEntry, const char *logLineMessage );
    bool pop( LogLineEntry &logLineEntry, char *logLineMessage );
    bool isEmpty();
    bool isFull();
    size_t size();
    size_t capacity();
    uint8_t percentageFull();

    private:
    LogLineEntry *logLineEntries = nullptr;
    char **logMessages;
    size_t logLineCapacity = 0;
    size_t logLineMessageSize = 0;
    size_t size_ = 0;
    size_t front_ = 0;
    size_t rear_ = 0;
  };

  /* When an instance is created we disable logging to file and serial.
     this has to be added with methods addFileLogging() and addSerialLogging()
     If you want to change any settings for this library, call Logger::globalSettings()
     before calling any other methods in this library */
  class Elog
  {
    private:
    static std::vector< Elog * > loggerInstances;  // for traversing logger instances from static methods

    static bool writerTaskHold;  // If you want to pause the writerTask, set this to true.

    LogService service;         // All instance info about sd, spiffs, serial, loglevel is stored here
    static bool serialEnabled;  // When some serial is added to an instance this is true. Used for reporting

    static LogRingBuff logRingBuff;  // Our ringbuffer that stores all log messages
    static LogSettings settings;     // Global settings for this library
    static LogStatus loggerStatus;   // Status for logging

    static void writerTask( void *parameter );
    static void outputSerial( const LogLineEntry &logLineEntry, const char *logLineMessage );
    static void outputSpiffs( const LogLineEntry &logLineEntry, const char *logLineMessage );

    static void getLogStamp( const uint32_t logTime, const Loglevel loglevel, char *output );
    static void getLogStamp( const uint32_t logTime, const Loglevel loglevel, const char *service, char *output );
    static uint8_t getServiceString( const char *serviceName, char *output );
    static uint8_t getLogLevelString( const Loglevel logLevel, char *output );

    static void logInternal( const Loglevel loglevel, const char *format, ... );
    static void writerTaskStart();
    static void addToRingbuffer( const LogLineEntry &logLineEntry, const char *logLineMessage );
    static void reportStatus();

#ifndef LOGGER_DISABLE_TIME
    static time_t providedTime;
    static uint32_t providedTimeAtMillis;
#endif
#ifndef LOGGER_DISABLE_SPIFFS
    static File spiffsFileHandle;
    static bool spiffsConfigured;
    static bool spiffsMounted;         // True when spiffs is mounted
    static char spiffsFileName[ 30 ];  // The filename of the current log file
    static bool spiffsDateFileWritten;

    static void spiffsPrepare();
    static void spiffsListLogFiles( Stream &serialPort );
    static bool spiffsProcessCommand( Stream &serialPort, const char *command );
    static void spiffsPrintLogFile( Stream &serialPort, const char *filename );
    static void spiffsFormat( Stream &serialPort );
    static void spiffsFlush();
    static void spiffsWriteDateFile();
    static void spiffsEnsureFreeSpace( bool checkImmediately = false );
    static bool spiffsGetFileDate( uint8_t lognumber, char *output );
    static void spiffsLogDelete( uint16_t lognumber );
#endif

#ifndef LOGGER_DISABLE_SD
    static uint16_t sdLogNumber;
    static char directoryName[ 8 ];

    static uint8_t sdChipSelect;
    static uint32_t sdSpeed;

    static bool sdCardPresent;
    static int32_t sdCardLastReconnect;

    static bool sdConfigured;

    static SdFat sd;
    static SPIClass spi;
    static void createLogFileIfClosed( LogService *service );
    static void reconnectSd();
    static void closeAllFiles();
    static void sdSyncAllFiles();
    static void outputSd( LogLineEntry &logLineEntry, char *logLineMessage );
#endif

#ifndef NOT_SYSLOG
    static void sendLog( LogLineEntry &logLineEntry, char *logLineMessage );

    public:
    void addSyslogLogging( const Loglevel wantedLogLevel );
    void setSyslogOnline( bool );

    private:
#endif

    protected:
    // This is kept protected for the LoggerTimer to access these things
    static uint8_t getTimeString( uint32_t milliSeconds, char *output );
    static uint8_t getTimeStringMillis( uint32_t milliSeconds, char *output );
    static uint8_t getTimeStringReal( uint32_t milliseconds, char *output );
    static PrecisionTime GetRealTime();

    public:
    Elog();

    static void globalSettings( uint16_t maxLogMessageSize = 250,
                                uint16_t maxLogMessages = 20,
                                Stream &internalLogDevice = Serial,
                                Loglevel internalLogLevel = WARNING,
                                bool discardMsgWhenBufferFull = false,
                                uint32_t reportStatusEvery = 5000 );

    void addSerialLogging( Stream &serialPort, const char *serviceName, const Loglevel wantedLogLevel );
    static void configureSpiffs( uint32_t spiffsSyncEvery = 5000,
                                 uint32_t spiffsCheckSpaceEvery = 10000,
                                 uint32_t spiffsMinimumSpace = 50000 );
    void addSpiffsLogging( const char *serviceName, const Loglevel wantedLogLevel );
#ifndef NOT_SYSLOG
    void setUdpClient( UDP &client,
                       const char *server,
                       uint16_t port,
                       const char *deviceHostname = SYSLOG_NILVALUE,
                       const char *appName = SYSLOG_NILVALUE,
                       uint16_t priDefault = LOG_KERN,
                       uint8_t protocol = SYSLOG_PROTO_IETF );
    void setUdpClient( UDP &client,
                       IPAddress ip,
                       uint16_t port,
                       const char *deviceHostname = SYSLOG_NILVALUE,
                       const char *appName = SYSLOG_NILVALUE,
                       uint16_t priDefault = LOG_KERN,
                       uint8_t protocol = SYSLOG_PROTO_IETF );
#endif

    void log( const Loglevel logLevel, const char *format, ... );

    char *toHex( byte *data, uint16_t len );
    char *toHex( char *data );

#ifndef LOGGER_DISABLE_TIME
    static void provideTime( uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second );
#endif
#ifndef LOGGER_DISABLE_SPIFFS
    void spiffsQuery( Stream &serialPort );
#endif
#ifndef LOGGER_DISABLE_SD
    static void configureSd( SPIClass &spi,
                             uint8_t cs,
                             uint32_t speed = 2000000,
                             uint32_t sdReconnectEvery = 5000,
                             uint32_t sdSyncFilesEvery = 5000,
                             uint32_t sdTryCreateFileEvery = 5000 );
    void addSdLogging( const char *fileName, const Loglevel wantedLogLevel, const LogFileOptions options = FILE_NO_OPTIONS );
#endif
  };
}  // namespace logger
