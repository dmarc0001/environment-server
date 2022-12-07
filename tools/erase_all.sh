#!/bin/bash
#
# flashe das www-verzeichnis

esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 115200 erase_flash 


