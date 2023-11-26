#!/bin/bash
#
# make Version

DESTFILE=../include/version.hpp

echo "#pragma once" > $DESTFILE
echo " " >> $DESTFILE
echo "namespace Prefs" >> $DESTFILE
echo "{" >> $DESTFILE
VERSION=`git describe`
echo "  constexpr const char *VERSION{ \"$VERSION\" };" >> $DESTFILE
echo "}" >> $DESTFILE
echo "" >> $DESTFILE

