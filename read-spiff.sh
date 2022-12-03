#!/bin/bash
#
# flashe das www-verzeichnis

esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 read_flash 0x190000 1536000 read-spiffs-www.bin
