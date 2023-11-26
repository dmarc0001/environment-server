#!/bin/bash
#
# flashe das www-verzeichnis
IDF_PATH=/root/.platformio/packages/framework-espidf
#921600
#115200
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 read_flash 0x190000 1536000 read-spiffs-www.bin
$IDF_PATH/tools/mkspiffs --unpack -d3 -b 4096 -p 256 ../read read-spiffs-www.bin
