#!/usr/bin/env python
# this is a short script, which is used in order to plot a hits per time
# plot for individual pixels (or pixel regions) based on a bunch of 
# cPickled occupancy data batches

import numpy as np
import cPickle
import os
import matplotlib
matplotlib.use('TKagg')
from septemModule.septemClasses import pixelParser
from septemModule.septemMisc import get_iter_batch_from_header_text
from septemModule.septemFiles import load_occupancy_dump
import matplotlib.pyplot as plt

def main(args):
    # get all files in given folder
    try:
        folders = [f for f in args if '-' not in f and '/' in f]
        #folder = args[0]
    except IndexError:
        raise IndexError("Please give a folder containing cPickled occupancy batches as argument.")


    args_dict = {}
    if "--nchips" in args:
        try:
            ind = args.index("--nchips")
            args_dict["nchips"] = int(args[ind + 1])
        except IndexError:
            print 'If you enter --nchips please enter a valid number of chips.'
            args_dict["nchips"] = 1
    else:
        args_dict["nchips"] = 1
    if "--log" in args:
        args_dict["log"] = True
    else:
        args_dict["log"] = False
    if "--npix" in args:
        # if npix in args we simply get the total number of pixels,
        # which were active in the given region and plot that, instead
        # of the mean pixel values, which we do in the normal case
        args_dict["npix"] = True
    else:
        args_dict["npix"] = False


    # before we run over all files and extracat the correct pixel areas from each
    # we need to parse these pixels
    pixel_parser = pixelParser()
    n1 = "fancy_pix"
    n2 = "list of pixs"
    dy = "dyke3"
    #pixel_parser.add_pixels(n1, (10, 100), chip = 1)
    #pixel_parser.add_pixels(n2, [(10, 100), (32, 100), (13, 42)], chip = 1)
    # add e.g. roughly top dyke region
    #pixel_parser.add_pixels(dy, [0, 0, 10, 255], chip = 3)
    name_chips = 'Chip #%i'
    for i in xrange(args_dict["nchips"]):
        pixel_parser.add_pixels((name_chips % i), [0, 0, 255, 255], chip = i)  

    # now we wish to run over all given folders (if we wish to append several folders to one
    # batch)
    batch_iter = 0
    batch_lengths = []
    for folder in folders:
        batches = []
        files = os.listdir(folder)
        files = [f for f in files if ".dat" in f]
        for f in files:
            # first load dumped files
            path = os.path.join(folder, f)
            data, header_text = load_occupancy_dump(path)
            # from list of arrays (data) we need to extract hits
            batch_num = get_iter_batch_from_header_text(header_text)
            # we add batch_iter to batch_num so that for several different runs,
            # which came one after another, we can append these
            pixel_parser.extract_hits(data, batch_num + batch_iter)
            batches.append(int(batch_num + batch_iter))
            if f == files[-1]:
                batch_iter += max(batches)
                batch_lengths.append(max(batches))
        
    
    # times1, hits1 = pixel_parser.get_hits_per_time_for_name(n1)
    # times2, hits2 = pixel_parser.get_hits_per_time_for_name(dy)
    # times3, hits3 = pixel_parser.get_hits_per_time_for_name(n2)

    for i in xrange(args_dict["nchips"]):
        name = (name_chips % i)
        if args_dict["npix"] is False:
            t, h = pixel_parser.get_hits_per_time_for_name(name)
            plt.plot(t, h, label=name, linestyle='', marker='x', markersize=4)
        else:
            t, np = pixel_parser.get_npix_per_time_for_name(name)
            plt.plot(t, np, label=name, linestyle='', marker='x', markersize=4)
        if len(batch_lengths) > 1:
            # in this case plot vertical lines, which show beginning and end of 
            # different runs
            tot_b = float(sorted(t)[-1])
            for b in batch_lengths:
                print('%i x pos to plot line %s %s %s' % (i, b, tot_b, b / tot_b))
                plt.axvline(x = b, ymin = 0, ymax=1, linewidth = 2, color = 'k')

    # plt.plot(times1, hits1, 'r.', label=n1)
    # plt.plot(times3, hits3, 'midnightblue', marker='v', label=n2)
    # plt.plot(times2, hits2, color='sienna', marker='^', linestyle = '', label=dy)
    plt.legend()
    plt.xlabel('Time / h')
    if args_dict["log"]:
        plt.yscale("log")
    if args_dict["npix"] is False:
        plt.ylabel('Mean hit rate / #')
        plt.savefig('hitrate_per_time.pdf')
    else:
        plt.ylabel('Npix / #')
        plt.savefig('npix_per_time.pdf')



    plt.show()
    


if __name__=="__main__":
    import sys
    main(sys.argv[1:])
