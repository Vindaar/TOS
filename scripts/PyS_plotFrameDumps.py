#!/usr/bin/env python

import matplotlib as mpl
mpl.use("TKAgg")
mpl.interactive(False)
import matplotlib.pyplot as plt
import numpy as np
import os

def get_frame_from_file(path, file):
    filename = path + file
    #frame = readdlm(filename)
    frame = np.loadtxt(filename)

    return frame

def main(args):

    path = "/home/schmidt/TOS/tmp/framedumps/"
    files = os.listdir(path)
    #f, ax = subplots()
    fig = plt.figure()
    subplot = fig.add_subplot(111)

    frame = get_frame_from_file(path, files[0])
    image = subplot.imshow(frame, interpolation="none", axes=subplot)
    #plt[:draw]()
    plt.show(block=False)
    for f in files[1:]:
        #print("Working on ", f)
        frame = get_frame_from_file(path, f)
        image.set_data(frame)
        image.autoscale()
        fig.canvas.draw()

    #plt.show()

if __name__=="__main__":
    import sys
    main(sys.argv[1:])
