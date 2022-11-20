#!/bin/bash
#
# generiere und flashe das www-verzeichnis


#$IDF_PATH/tools/mkspiffs --create www --debug 5 --all-files --block 4096 --page 256 --size 1536000 spiffs-www.bin

$IDF_PATH/components/spiffs/spiffsgen.py 1536000 www spiffs-www.bin
