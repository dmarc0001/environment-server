#!/bin/bash
#
# hole messdaten aktuell

THOST=192.168.1.40

DATAFILES="acku.jdata last_fscheck.dat temporary.jdata today.jdata"

for DFILE in $DATAFILES ; 
do
  echo "check $DFILE..."
  if [ -e $DFILE ] ; then
    rm -f $DFILE
  fi
done


wget http://$THOST/today.jdata
wget http://$THOST/temporary.jdata
wget http://$THOST/last_fscheck.dat
wget http://$THOST/acku.jdata


