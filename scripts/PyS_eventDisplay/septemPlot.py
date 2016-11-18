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
def plot_file(filepath, filename, sep, fig, chip_subplots, im_list, cb_flag, cb_value):

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
    for i in range(len(texts)):
        texts[-1].remove()
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

    # define the variable, in which we store the number of hits for each chip to
    # print at the top left
    hits_text = "Hits\n"

    plots_to_hide = range(7)
    for i, chpHeader in enumerate(chpHeaderList):
        chip_data = chpHeader.pixData
        # now get current chip number so that we plot on to the correct chip
        chipNum = int(chpHeader.attr["chipNumber"])
        # and get the number of hits
        numHits = int(np.size(chip_data))#chpHeader.attr["numHits"])
        # use both to create the hits box
        hits_text += "Chip #%i : %i" % (chipNum, numHits)
        if chipNum != 7:
            hits_text += "\n"
        try:
            chip_full_array = np.zeros((256, 256))
            # now input the chip data into the image array (need to invert
            # x and y column. imshow shows the data as row, column
            # which means y, x coordinates
            chip_full_array[chip_data[:,1], chip_data[:,0]] = chip_data[:,2]

            # now create an image, but rather use im_list to set the correct image data
            im_list[chipNum - 1].set_data(chip_full_array)
                        
            # # now remove this chip from the plots_to_hide list
            plots_to_hide.remove(chipNum - 1)
            im_list[chipNum - 1].set_visible(True)

            print '\n stt', np.max(chip_data[:,2])
            #im_list[chipNum - 1].set_clim(0, np.percentile(chip_data[:,2], 80))
            if cb_flag == True:
                im_list[chipNum - 1].set_clim(0, np.percentile(chip_data[:,2], cb_value))
            else:
                im_list[chipNum - 1].set_clim(0, cb_value)

            # not needed anymore
            #im = chip_subplots[chipNum-1].imshow(chip_full_array, interpolation='none', axes=chip_subplots[chipNum-1])#, vmin=0, vmax=250)

        except IndexError:
            print 'IndexError: chip', chpHeader.attr["chipNumber"], ' has no hits'
            print chip_data
        except TypeError:
            print 'TypeError: chip', chpHeader.attr["chipNumber"], ' has no hits'


    # now create the hits box
    hits_box = fig.text(0.2, 0.9, hits_text,
                        bbox={'facecolor':'blue', 'alpha':0.1, 'pad':15},
                        family = 'monospace',
                        transform = chip_subplots[-1].transAxes,
                        horizontalalignment = 'center',
                        verticalalignment = 'center',
                        multialignment = 'left')

    # now set all plots invisible, which were not updated this time
    for i in plots_to_hide:
        im_list[i].set_visible(False)

    try:
        cbaxes = fig.add_axes([sep.row2.right - 0.015, sep.row3.bottom, 0.015, (sep.row3.top - sep.row3.bottom)])
        cb = plt.colorbar(im_list[3], cax = cbaxes)
        fig.canvas.draw()
    except UnboundLocalError:
        print filename
    
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
    fadcPlot.set_title(filepathName)
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


def plot_occupancy(filepath, sep, fig, chip_subplots, im_list, chip_arrays, cb_flag, cb_value):
    # this function plots the occupancy plots, which are created by the 
    # create_occupancy_plot function
    
    # first remove the texts from before, if any
    texts = fig.texts
    for i in range(len(texts)):
        texts[-1].remove()


    # define the variable, in which we store the number of hits for each chip to
    # print at the top left
    hits_text = "".ljust(10) + "Hits".ljust(10) + "max values\n"

    # now perform plotting
    plots_to_hide = range(7)
    for i, chip_array in enumerate(chip_arrays):
        # get number of non zero elements in this array
        numHits = np.count_nonzero(chip_array)
        maxVals = np.max(chip_array)
        # use both to create the hits box
        hits_text += ("Chip #%i : %i" % (i, numHits)).ljust(20)
        hits_text += str(int(maxVals))
        if i != 6:
            hits_text += "\n"
        try:
            # now create an image, but rather use im_list to set the correct image data
            im_list[i].set_data(chip_array)
                        
            # # now remove this chip from the plots_to_hide list
            plots_to_hide.remove(i)
            im_list[i].set_visible(True)

            if cb_flag == True:
                im_list[i].set_clim(0, np.percentile(chip_array, cb_value))
            else:
                im_list[i].set_clim(0, cb_value)

        except IndexError:
            print 'IndexError: chip', i, ' has no hits'
        except TypeError:
            print 'TypeError: chip', i, ' has no hits'


    # now create the hits box
    hits_box = fig.text(0.2, 0.9, hits_text,
                        bbox={'facecolor':'blue', 'alpha':0.1, 'pad':15},
                        family = 'monospace',
                        transform = chip_subplots[-1].transAxes,
                        horizontalalignment = 'center',
                        verticalalignment = 'center',
                        multialignment = 'left')

    # now set all plots invisible, which were not updated this time
    for i in plots_to_hide:
        im_list[i].set_visible(False)

    try:
        cbaxes = fig.add_axes([sep.row2.right - 0.015, sep.row3.bottom, 0.015, (sep.row3.top - sep.row3.bottom)])
        cb = plt.colorbar(im_list[3], cax = cbaxes)
        fig.canvas.draw()
    except UnboundLocalError:
        print 'something is bad'
    
    make_ticklabels_invisible(chip_subplots)

    # and now plot everythin
    plt.pause(0.01)
