#!/bin/bash
#
# hole messdaten aktuell

THOST=192.168.1.43

if [ -e today.json ] ; then
  rm -f today.json
fi

if [ -e temporary.json ] ; then
  rm -f temporary.json
fi

if [ -e last_fscheck.dat ] ; then
  rm -f last_fscheck.dat
fi

wget http://$THOST/today.json
wget http://$THOST/temporary.json
wget http://$THOST/last_fscheck.dat

