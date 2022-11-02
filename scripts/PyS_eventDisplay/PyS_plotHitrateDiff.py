#!/usr/bin/env python
# this script reads to cPickled files containing times and hit rates
# for ignore_full_frames occupancy calculations, calcs the difference
# and plots

import matplotlib
matplotlib.use('TKagg')
import numpy as np
import cPickle
import matplotlib.pyplot as plt

def main(args):
    
    iff_false = open('out/hitrate/cPickle_hitrate_per_time_iff_false.dat', 'r')
    iff_true  = open('out/hitrate/cPickle_hitrate_per_time_iff_true.dat', 'r')

    tup_false = cPickle.load(iff_false)
    tup_true  = cPickle.load(iff_true)

    iff_false.close()
    iff_true.close()

    name_f, t_f, h_f = tup_false
    name_t, t_t, h_t = tup_true
    
    h_f = np.asarray(h_f)
    h_t = np.asarray(h_t)
    diff = h_f - h_t

    assert t_f == t_t
    plt.grid()
    plt.plot(t_f, diff, label='iff_false - iff_true', marker='x', markersize='4', linestyle='')
    plt.semilogy()
    plt.legend()
    plt.xlabel('Time / h')
    plt.ylabel('Mean hit rate / #')
    plt.savefig('hitrate_diff_iff_true_false.pdf')

    plt.show()
    


if __name__=="__main__":
    import sys
    main(sys.argv[1:])
