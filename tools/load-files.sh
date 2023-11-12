#!/bin/bash
#
# hole messdaten aktuell

THOST=192.168.1.40

DATAFILES="acku02.jdata acku01.jdata last_fscheck.dat today01.jdata today02.jdata"

for DFILE in $DATAFILES ; 
do
  echo "check $DFILE..."
  if [ -e $DFILE ] ; then
    rm -f $DFILE
  wget http://$THOST/$DFILE
  fi
done


# wget http://$THOST/today01.jdata
# wget http://$THOST/today02.jdata
# wget http://$THOST/last_fscheck.dat
# wget http://$THOST/acku01.jdata
# wget http://$THOST/acku02.jdata


