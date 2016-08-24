# this file implements the functions, which perform the plotting of the data
# for the septem board and the FADC data

import numpy as np
import matplotlib.pyplot as plt
from septemFiles import read_zero_suppressed_data_file
from septemClasses import Fadc
from profilehooks import profile


def make_ticklabels_invisible(fig):
    for i, ax in enumerate(fig):
        #ax.text(0.5, 0.5, "ax%d" % (i+1), va="center", ha="center")
        for tl in ax.get_xticklabels() + ax.get_yticklabels():
            tl.set_visible(False)

#@profile
def plot_file(filepath, filename, sep, fig, chip_subplots):#, evHeader, chpHeaderList):

    # create full path to file
    filepathName = filepath + filename
    # and read that file
    evHeader, chpHeaderList = read_zero_suppressed_data_file(filepathName)

    # clear all axes
    for ax in chip_subplots:
        ax.cla()

    # and remove the text symbol from before
    texts = fig.texts
    if len(texts) > 0:
        texts[0].remove()
    # now add the header for this event. We add it to the last axes in the list
    # (for no special reason)
    # call function to get the header text
    header_text = evHeader.get_event_header_text(filename)
    # and put it at the top
    header_box = fig.text(0.5, 0.9, header_text,
                          bbox={'facecolor':'blue', 'alpha':0.1, 'pad':15},
                          family = 'monospace',
                          transform = ax.transAxes,
                          horizontalalignment = 'center',
                          verticalalignment = 'center',
                          multialignment = 'left')
    
    for i, chpHeader in enumerate(chpHeaderList):
        chip_data = chpHeader.pixData
        try:
            chip_full_array = np.zeros((256, 256))
            # now input the chip data into the image array (need to invert
            # x and y column. imshow shows the data as row, column
            # which means y, x coordinates
            chip_full_array[chip_data[:,1], chip_data[:,0]] = chip_data[:,2]

            # now get current chip number so that we plot on to the correct chip
            chipNum = int(chpHeader.attr["chipNumber"])
            im = chip_subplots[chipNum-1].imshow(chip_full_array, interpolation='none', axes=chip_subplots[chipNum-1])
            #chip_subplots[chipNum - 1].invert_yaxis()
            if chipNum not in [6, 7]:
                # in case of chips 1 to 5, we need to invert the y axis. Bonds are below the chips,
                # thus (0, 0) coordinate is at bottom left. default plots (0, 0) top left
                chip_subplots[chipNum - 1].invert_yaxis()
            else:
                # in case of chips 6 and 7, the bond area is above the chips, meaning (0, 0) 
                # coordinate is at the top right. thus invert x axis
                chip_subplots[chipNum - 1].invert_xaxis()
            #print "plotting for chip ", chipNum
            chip_full_array = np.ones((256,256))
            im.set_cmap('viridis')

        except IndexError:
            print 'IndexError: chip', chpHeader.attr["chipNumber"], ' has no hits'
            print chip_data
        except TypeError:
            print 'TypeError: chip', chpHeader.attr["chipNumber"], ' has no hits'


    make_ticklabels_invisible(chip_subplots)

    # and create colorbar
    try:
        cbaxes = fig.add_axes([sep.row2.right - 0.03, sep.row3.bottom, 0.03, (sep.row3.top - sep.row3.bottom)])
        cb = plt.colorbar(im, cax = cbaxes)
    except UnboundLocalError:
        print filename

    # and now plot everythin
    #plt.show()
    # plt.draw()
    plt.pause(0.01)

    return chip_subplots
    
    

#@profile
def plot_fadc_file(filepath, filename, fadcPlot):#, fadc):

    print 'working on ', filename#self.filelist[self.i]
    # first we create an FADC object and give the filename to it
    
    # create full path to file
    filepathName = filepath + filename
    # creating the fadc object takes care of pedestal runs, temporal corrections
    # and the splitting by channels
    fadc = Fadc(filepathName)
    
    # clear plot from before and write labels
    fadcPlot.cla()
    fadcPlot.set_xlabel('Time / clock cycles')
    fadcPlot.set_ylabel('U / fadc ticks')


    #channel0 = fadc.channel0
    #channel1 = fadc.channel1
    #channel2 = fadc.channel2
    channel3 = fadc.channel3

    # and plot everything
    #fadcPlot.set_title(filepathName)
    #fadcPlot.plot(np.arange(np.size(channel0)), channel0, color='purple')
    #fadcPlot.plot(np.arange(np.size(channel1)), channel1, color='red')
    #fadcPlot.plot(np.arange(np.size(channel2)), channel2, color='green')
    fadcPlot.plot(np.arange(np.size(channel3)), channel3, color='blue')

    plt.pause(0.01)

    return fadcPlot
