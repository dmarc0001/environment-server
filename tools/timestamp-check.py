#!/bin/python3
#
# timestamps checken

import json
from time import time
from datetime import datetime

#INPUT = 'today.json'
INPUT = 'acku.jdata'


def main():
    try:
        fhandle = open(INPUT, "r")
        jsonData = json.load(fhandle)
        fhandle.close()
    except IOError as err:
        print("io error: {}".format(err))
        return
    except json.JSONDecodeError as err:
        print("json error: {}".format(err))
        return
    print("data loadet...")
    now_timestamp = int(time())
    old_ts = now_timestamp
    # datei offen
    # for ixd=0; idx < jsonData.len(); idx++:
    for d_set in jsonData:
        ts = int(d_set['ti'])
        abs_diff = now_timestamp - ts
        hours = int(abs_diff / 3600)
        minutenrest = abs_diff % 3600
        minuten = int(minutenrest / 60)
        sekunden = minutenrest % 60
        abs_diff = "{:02d}:{:02d}:{:02d}".format(hours, minuten, sekunden)
        rel_diff = ts - old_ts
        minuten = int(rel_diff / 60)
        sekunden = rel_diff % 60
        rel_diff = "{:02d}:{:02d}".format(minuten, sekunden)
        print("time {} diff: {}(abs) {}(rel)".format(datetime.utcfromtimestamp(
            ts).strftime('%Y-%m-%d %H:%M:%S'), abs_diff, rel_diff))
        old_ts = ts


# run main if standallone
if __name__ == '__main__':
    main()
