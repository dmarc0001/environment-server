#!/bin/bash
#
# generiere und flashe das www-verzeichnis
IDF_PATH=/root/.platformio/packages/framework-espidf


$IDF_PATH/components/spiffs/spiffsgen.py 1536000 ../data spiffs-www.bin
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 115200 write_flash --flash_mode dio --flash_freq 40m  0x190000 spiffs-www.bin
