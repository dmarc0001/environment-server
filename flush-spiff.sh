#!/bin/bash
#
# flashe das www-verzeichnis

esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 115200 write_flash --flash_mode dio --flash_freq 40m  0x190000 spiffs-www.bin
