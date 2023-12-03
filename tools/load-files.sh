#!/bin/bash
#
# hole messdaten aktuell

THOST=env-sensor.fritz.box

DATAFILES="acku.jdata last_fscheck.dat today.jdata week.jdata month.jdata"

for DFILE in $DATAFILES ; 
do
  echo "check $DFILE..."
  if [ -e $DFILE ] ; then
    echo "remove old one..."
    rm -f $DFILE
  fi
  wget http://$THOST/$DFILE
done


# wget http://$THOST/today01.jdata
# wget http://$THOST/today02.jdata
# wget http://$THOST/last_fscheck.dat
# wget http://$THOST/acku01.jdata
# wget http://$THOST/acku02.jdata


