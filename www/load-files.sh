#!/bin/bash
#
# hole messdaten aktuell

if [ -e today.json ] ; then
  rm -f today.json
fi

wget http://192.168.1.40/today.json

