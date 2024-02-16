# environment sensor esp32

- simple web gui with graphs over 2 ore 24 hours
- led status (rgb led, 3 pieces WS2811 controlled)
- small api for requests (return json)
  - /api/v1/today : measure data for round about 24 hours ago
  - /api/v1/week : for round a week
  - /api/v1/month: for a month
  - /api/v1/interval: which interval are the mesure data
  - /api/v1/version: software version
  - /api/v1/info : idf/platformio version, count of cpu cores
  - /api/v1/current : acku voltage or chip viltagfe if no acku
  - /api/v1/fscheck : tesing purposes, trigger an filesystem chcek process
  - /api/v1/fsstat : amount of space in SPIFFS filesystem an measure files
  - /api/v1/set-datalog?server=192.168.1.44&port=63420 : set data log destination (UDP), IP, Port / IP 0 disable
  - /api/v1/set-syslog?server=192.168.1.44&port=512 : set syslog destination (UDP), IP, Port  / IP 0 disable
  - /api/v1/set-timezone?timezone=CET-1 : set internal timezone (for GUI)
  - /api/v1/set-loglevel?level=7 : set controller loglevel, only app



## liglevels (numeric)
    EMERGENCY = 0,
    ALERT = 1,
    CRITICAL = 2,
    ERROR = 3,
    WARNING = 4,
    NOTICE = 5,
    INFO = 6,
    DEBUG = 7,








  
