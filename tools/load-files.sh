#!/bin/bash
#
# hole messdaten aktuell

if [ -e today.json ] ; then
  rm -f today.json
fi

if [ -e temporary.json ] ; then
  rm -f temporary.json
fi

if [ -e last_fscheck.dat ] ; then
  rm -f last_fscheck.dat
fi

wget http://192.168.1.40/today.json
wget http://192.168.1.40/temporary.json
wget http://192.168.1.40/last_fscheck.dat

