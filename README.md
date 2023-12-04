# environment sensor esp32

- simple web gui with graphs over 2 ore 24 hours
- led status (rgb led)
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

  
