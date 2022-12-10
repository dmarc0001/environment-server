#!/bin/bash
#
# hole messdaten aktuell

THOST=192.168.1.43
TODAY_DAT=today.data

if [ -e today.jdata ] ; then
  rm -f today.jdata
fi

if [ -e temporary.jdata ] ; then
  rm -f temporary.jdata
fi

if [ -e last_fscheck.dat ] ; then
  rm -f last_fscheck.dat
fi

wget http://$THOST/today.jdata
wget http://$THOST/temporary.jdata
wget http://$THOST/last_fscheck.dat


