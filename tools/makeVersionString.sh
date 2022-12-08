#!/bin/bash
#
# make current version string

DEST=../include/version.hpp

VERSIONSTR=`git log --pretty="<%h> %ci %aE" | head -n 1`

cat <<EOF > $DEST
#pragma once

constexpr const char *MY_APP_VERSION{"$VERSIONSTR"};

EOF


