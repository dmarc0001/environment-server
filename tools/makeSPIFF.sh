#!/bin/bash
#
# generiere und flashe das www-verzeichnis
IDF_PATH=/root/.platformio/packages/framework-espidf


echo "make spiffs file..."
$IDF_PATH/components/spiffs/spiffsgen.py 1536000 ../data spiffs-www.bin
#921600
#115200

echo "upload spiffs..."
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash --flash_mode dio --flash_freq 40m  0x190000 spiffs-www.bin
