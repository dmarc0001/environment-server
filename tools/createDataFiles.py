#!/usr/bin/python3
# -*- coding: utf-8 -*-
#

from datetime import datetime
from random import uniform
from time import sleep
import re

lineTemplate = '<"timestamp":"{:d}","data":[<"id":"13654914070250903080","temp":"{:.3f}","humidy":"-100.000000">,<"id":"2918332558598846760","temp":"{:.3f}","humidy":"-100.000000">,<"id":"4071254063206621992","temp":"{:.3f}","humidy":"-100.000000">,<"id":"9907919180278756136","temp":"{:.3f}","humidy":"-100.000000">,<"id":"0","temp":"{:.3f}","humidy":"{:.3f}">]>\n'


def main(_currentTimestamp: int, _intervallSec: int):
    """ main function """
    #
    # start 35 days back
    #
    startTimeStamp = _currentTimestamp - (35 * 24 * 60 * 60)
    print("start create files....")
    startTimeStamp = makeMonth(startTimeStamp, _intervallSec, "month.json")
    startTimeStamp = makeWeek(startTimeStamp, _intervallSec, "week.json")
    makeToday(startTimeStamp, _currentTimestamp, _intervallSec, "today.json")


def makeToday(c_time: int, endtime: int, intr: int, filename: str):
    """ file for today """
    print("create file <{}>...".format(filename))
    # print("start: <{:d}> end: <{:d}>".format(c_time, endtime))
    # sleep(3)
    mFile = open(filename, "w")
    isFirst = True
    if mFile:
        while c_time < endtime:
            # create dataset
            if isFirst:
                data = ""
                isFirst = False
            else:
                data = ","
            data += lineTemplate.format(c_time, uniform(18.0, 22.5), uniform(18.0, 22.5), uniform(
                18.0, 22.5), uniform(18.0, 22.5), uniform(18.0, 22.5), uniform(35.0, 75.0))
            data = data.replace("<", "{").replace(">", "}")
            # print(data)
            # write dataset
            mFile.write(data)
            # add intervall
            c_time += intr
        mFile.close
    return c_time


def makeWeek(c_time: int, intr: int, filename: str):
    """ make Week """
    print("create file <{}>...".format(filename))
    # sleep(3)
    endtime = c_time + (6 * 24 * 60 * 60)
    mFile = open(filename, "w")
    isFirst = True
    if mFile:
        while c_time < endtime:
            # create dataset
            if isFirst:
                data = ""
                isFirst = False
            else:
                data = ","
            data += lineTemplate.format(c_time, uniform(18.0, 22.5), uniform(18.0, 22.5), uniform(
                18.0, 22.5), uniform(18.0, 22.5), uniform(18.0, 22.5), uniform(35.0, 75.0))
            data = data.replace("<", "{").replace(">", "}")
            # print(data)
            # write dataset
            mFile.write(data)
            # add intervall
            c_time += intr
        mFile.close
    return c_time


def makeMonth(c_time: int, intr: int, filename: str):
    """ Make month """
    print("create file <{}>...".format(filename))
    # sleep(3)
    endtime = c_time + (28 * 24 * 60 * 60)
    mFile = open(filename, "w")
    isFirst = True
    if mFile:
        while c_time < endtime:
            # create dataset
            if isFirst:
                data = ""
                isFirst = False
            else:
                data = ","
            data += lineTemplate.format(c_time, uniform(18.0, 22.5), uniform(18.0, 22.5), uniform(
                18.0, 22.5), uniform(18.0, 22.5), uniform(18.0, 22.5), uniform(35.0, 75.0))
            data = data.replace("<", "{").replace(">", "}")
            # print(data)
            # write dataset
            mFile.write(data)
            # add intervall
            c_time += intr
        mFile.close
    return c_time


if __name__ == '__main__':
    intervallSec = 120
    currentTimestamp = int(datetime.now().timestamp())
    main(currentTimestamp, intervallSec)
