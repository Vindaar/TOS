#!/usr/bin/env python
# this is a short script, which is used in order to plot a hits per time
# plot for individual pixels (or pixel regions) based on a bunch of 
# cPickled occupancy data batches

import numpy as np
import cPickle
import os
from septemModule.septemClasses import pixelParser
from septemModule.septemMisc import get_iter_batch_from_header_text
from septemModule.septemFiles import load_occupancy_dump
import matplotlib.pyplot as plt

def main(args):
    # get all files in given folder
    try:
        folder = args[0]
    except IndexError:
        raise IndexError("Please give a folder containing cPickled occupancy batches as argument.")

    files = os.listdir(folder)
    files = [f for f in files if ".dat" in f]

    # before we run over all files and extracat the correct pixel areas from each
    # we need to parse these pixels
    pixel_parser = pixelParser()
    n1 = "fancy_pix"
    n2 = "region_pix"
    pixel_parser.add_pixels(n1, (10, 100), chip = 1)
    pixel_parser.add_pixels(n2, [0, 0, 255, 255], chip = 1)
    
    for f in files:
        # first load dumped files
        path = os.path.join(folder, f)
        data, header_text = load_occupancy_dump(path)
        # from list of arrays (data) we need to extract hits
        batch_num = get_iter_batch_from_header_text(header_text)
        pixel_parser.extract_hits(data, batch_num)
        
    
    times1, hits1 = pixel_parser.get_hits_per_time_for_name(n1)
    times2, hits2 = pixel_parser.get_hits_per_time_for_name(n2)

    plt.plot(times1, hits1, label=n1)
    plt.plot(times2, hits2, label=n2)
    
    plt.savefig('test_pix_hits.pdf')
    


if __name__=="__main__":
    import sys
    main(sys.argv[1:])
