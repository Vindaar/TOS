# this file implements the functions, which perform the plotting of the data
# for the septem board and the FADC data

import numpy as np
import matplotlib.pyplot as plt
import matplotlib
from septemFiles import read_zero_suppressed_data_file
from septemClasses import Fadc
from profilehooks import profile


def make_ticklabels_invisible(fig):
    for i, ax in enumerate(fig):
        #ax.text(0.5, 0.5, "ax%d" % (i+1), va="center", ha="center")
        for tl in ax.get_xticklabels() + ax.get_yticklabels():
            tl.set_visible(False)

#@profile
def plot_file(filepath, filename, sep, fig, chip_subplots, im_list):#, evHeader, chpHeaderList):

    # create full path to file
    filepathName = filepath + filename
    # and read that file
    evHeader, chpHeaderList = read_zero_suppressed_data_file(filepathName)

    # clear all axes
    # not necessary anymore, since we use image.set_data
    #for ax in chip_subplots:
    #    ax.cla()

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
                          transform = chip_subplots[-1].transAxes,
                          horizontalalignment = 'center',
                          verticalalignment = 'center',
                          multialignment = 'left')

    plots_to_hide = range(7)
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

            # now create an image, but rather use im_list to set the correct image data
            im_list[chipNum - 1].set_data(chip_full_array)
                        
            # # now remove this chip from the plots_to_hide list
            plots_to_hide.remove(chipNum - 1)
            im_list[chipNum - 1].set_visible(True)

            # not needed anymore
            #im = chip_subplots[chipNum-1].imshow(chip_full_array, interpolation='none', axes=chip_subplots[chipNum-1], vmin=0, vmax=250)
            if chipNum not in [6, 7]:
                # in case of chips 1 to 5, we need to invert the y axis. Bonds are below the chips,
                # thus (0, 0) coordinate is at bottom left. default plots (0, 0) top left
                chip_subplots[chipNum - 1].invert_yaxis()
            else:
                # in case of chips 6 and 7, the bond area is above the chips, meaning (0, 0) 
                # coordinate is at the top right. thus invert x axis
                chip_subplots[chipNum - 1].invert_xaxis()

        except IndexError:
            print 'IndexError: chip', chpHeader.attr["chipNumber"], ' has no hits'
            print chip_data
        except TypeError:
            print 'TypeError: chip', chpHeader.attr["chipNumber"], ' has no hits'

    # now set all plots invisible, which were not updated this time
    for i in plots_to_hide:
        print "removing ", i
        im_list[i].set_visible(False)
    
    make_ticklabels_invisible(chip_subplots)
    #fig.canvas.draw()

    # and now plot everythin
    plt.pause(0.01)

    return chip_subplots + im_list
    
    

#@profile
def plot_fadc_file(filepath, filename, fadcPlot, fadcPlotLine):#, fadc):

    print 'working on ', filename#self.filelist[self.i]
    # make sure fadc plot is visible
    fadcPlotLine[0].set_visible(True)


    # first we create an FADC object and give the filename to it
    
    # create full path to file
    filepathName = filepath + filename
    # creating the fadc object takes care of pedestal runs, temporal corrections
    # and the splitting by channels
    fadc = Fadc(filepathName)
    
    #channel0 = fadc.channel0
    #channel1 = fadc.channel1
    #channel2 = fadc.channel2
    channel3 = fadc.channel3
    
    #print 'channel!!!', channel3

    # and plot everything
    #fadcPlot.set_title(filepathName)
    #fadcPlot.plot(np.arange(np.size(channel0)), channel0, color='purple')
    #fadcPlot.plot(np.arange(np.size(channel1)), channel1, color='red')
    #fadcPlot.plot(np.arange(np.size(channel2)), channel2, color='green')
    #print fadcPlotLine
    fadcPlotLine[0].set_data(np.arange(np.size(channel3)), channel3)#, color='blue')

    fadcPlot.relim()
    fadcPlot.autoscale_view()

    #fadcPlotLine.set_visible(True)
    #fadcPlot.figure.canvas.draw()

    plt.pause(0.01)

    return fadcPlot
