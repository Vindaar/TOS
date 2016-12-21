#!/usr/bin/env python

# this scripts performs a fit to the data created by the ToT calibration function
# in TOS

import matplotlib.pyplot as plt
import numpy as np
from scipy.optimize import curve_fit
import pylab

def calibration_function(x, a, b, c, t):
    # this is the calibration function, which is supposed to
    # fit to the TOT data
    val = a*x + b - c / (x - t) 
    return val

def main(args):
    

    # set up some LaTeX plotting parameters
    # still need to change parameters
    fig_width_pt = 478.00812#246.0  # Get this from LaTeX using \showthe\columnwidth
    inches_per_pt = 1.0/72.27               # Convert pt to inchq
    golden_mean = (np.sqrt(5)-1.0)/2.0         # Aesthetic ratio
    fig_width = fig_width_pt*inches_per_pt  # width in inches
    fig_height = fig_width*golden_mean      # height in inches
    fig_size =  [fig_width,fig_height]
    params = {'backend': 'ps',
              'axes.labelsize':      10,#10,
              'axes.titlesize':      10,
              'font.size':           10,
              'legend.fontsize':     10,#10,
              'xtick.labelsize':     8,#8,
              'ytick.labelsize':     8,#8,
              'text.usetex':         True,
              'text.latex.preamble': [r'\usepackage{siunitx}'],
              'font.family':         'serif',
              'font.serif':          'cm',
              'figure.figsize':      fig_size}
    pylab.rcParams.update(params)




    if len(args) > 0:
        f = args[0]
    else:
        print 'No argument given. Assuming default name TOTCalib1.txt'
        f = 'TOTCalib1.txt'
    try:
        f = open(f, 'r').readlines()
    except IOError:
        print('Error: Input file %s is not a valid file' % args[0])

    pulses = []
    chips  = []
    means  = []
    stds   = []

    for line in f:
        #if '#' in line:
            # TODO: define name and get it from file
        line = line.split()
        if 'pulse' in line:
            pulse_arg = line.index('pulse')
            pulse     = float(line[pulse_arg+1])
            pulses.append(pulse)
        if 'chip' in line:
            chip_arg = line.index('chip')
            chip     = float(line[chip_arg+1])
            chips.append(chip)
        if 'mean' in line:
            mean_arg = line.index('mean')
            mean     = float(line[mean_arg+1])
            means.append(mean)
        if 'std' in line:
            std_arg = line.index('std')
            # get the std values by correcting the RMS values to STD
            # done by dividing by sqrt ( 4 * 256 * 256 ) = 512
            std     = float(line[std_arg+1]) / 512
            stds.append(std)
        print line

    print np.size(pulses)
    
    # set the start parameters and start the fitting
    start_parameter = [0.549194, 23.5359, 50.735, -1]#[0.149194, 23.5359, 205.735, -100]
    result = curve_fit(calibration_function, pulses, means, p0=start_parameter, sigma=std, full_output = True, maxfev=50000)

    popt = result[0]
    pcov = result[1]
    infodict = result[2:]
    # get the parameters from the fit
    a = popt[0]
    b = popt[1]
    c = popt[2]
    t = popt[3]

    # Calculate the reduced Chi^2:
    # n_dof: # degrees of freedom
    n_dof  = (np.size(infodict[0]['fvec']) - np.size(popt))
    chi_sq = np.sum(infodict[0]['fvec']**2) / n_dof
    print '--------------------------------------------------'
    print 'Parameters of calibration fit: m = slope, b = offset'
    print 'a =', a, '+-', np.sqrt(pcov[0][0])
    print 'b =', b, '+-', np.sqrt(pcov[1][1])
    print 'c =', c, '+-', np.sqrt(pcov[2][2])
    print 't =', t, '+-', np.sqrt(pcov[3][3])
    print 'Chi^2 / dof =', chi_sq

    pulses_cont = np.linspace(0, np.max(pulses), 1000)
    #means_cont  = calibration_function(pulses_cont, start_parameter[0], start_parameter[1], start_parameter[2], start_parameter[3]) #a, b, c, t)
    means_cont  = calibration_function(pulses_cont, a, b, c, t)

    print np.max(pulses)
    plt.ylim(0, np.max(means)*1.05)
    plt.xlim(0, 500)
    plt.errorbar(pulses, means, linestyle='', marker='x', yerr=std, color='midnightblue')
    plt.plot(pulses_cont, means_cont, color='sienna')

    plt.xlabel('$U_{\\text{inj}} / \si{\mV}$')
    plt.ylabel('$\\text{ToT} / \\text{Clock cycles}$')
    #plt.title('ToT Calibration of chip %s' % chipname)
    plt.savefig('ToT-Calibration.eps')

    plt.show()


if __name__=="__main__":
    import sys
    main(sys.argv[1:])
