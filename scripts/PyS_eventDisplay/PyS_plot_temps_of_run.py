#!/usr/bin/env python

import matplotlib
matplotlib.use('TKagg')
import numpy as np
from datetime import datetime

import matplotlib.pyplot as plt


def get_TOS_date_syntax():
    # returns the syntax used in TOS for all dates (incl time)
    # as a string, which can be used for python's datetime 
    # strptime function
    return '%Y-%m-%d.%H:%M:%S'

def get_temp_filename():
    return "temp_log.txt"

def read_temp_file():
    # reads a temperature log file created during a run
    filename = get_temp_filename()
    with open(filename, 'r') as f:
        lines = f.readlines()

    imb    = []
    septem = []
    dates  = []
    for line in lines:
        if "#" not in line:
            line = line.split()
            imb.append(line[0])
            septem.append(line[1])
            dates.append(line[-1])

    date_syntax = get_TOS_date_syntax()
    dates = [datetime.strptime(el, date_syntax) for el in dates]
    
    return imb, septem, dates

def main(args):

    imb, septem, dates = read_temp_file()

    plt.plot(dates, imb)
    plt.plot(dates, septem)

    plt.show()
    
if __name__=="__main__":
    import sys
    main(sys.argv[1:])
