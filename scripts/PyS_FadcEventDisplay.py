#!/usr/bin/env python

import numpy as np
import matplotlib.pyplot as plt
import pylab


def main(args):
    
    if len(args) > 0:
        f = args[0]
        try:
            f = open(f, 'r').readlines()
        except IOError:
            print('Error: Input file %s is not a valid file' % args[0])
    else:
        print 'No argument given. Please give a file to read'

    fadc_values = []
    for line in f:
        line = line.strip()
        if "#" not in line:
            fadc_values.append(float(line))

    fadc_values = np.asarray(fadc_values)

    n_values = np.size(fadc_values)
    

    plt.plot(np.arange(n_values), fadc_values)
    plt.show()



if __name__=="__main__":
    import sys
    main(sys.argv[1:])
