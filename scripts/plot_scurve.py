#!/usr/bin/env python
# super simple script to simply plot e.g. SCurve files (or rather any file, which
# simply has x, y coordinates as two columns)

import numpy as np
import matplotlib.pyplot as plt


def main(args):

    f = args[0]
    print f
    f = open(f, 'r').readlines()
    x_list = []
    y_list = []
    for line in f:
        x, y = line.split()
        x_list.append(x)
        y_list.append(y)

    print x_list
    print y_list
    plt.plot(x_list, y_list)
    plt.show()
    

if __name__=="__main__":
    import sys
    main(sys.argv[1:])
