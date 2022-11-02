# this file implements the functions, which perform the plotting of the data
# for the septem board and the FADC data

import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib
from .septemFiles import read_zero_suppressed_data_file, read_full_matrix_data_file
from .septemClasses import Fadc
from .septemMisc import add_line_to_header, convert_datetime_str_to_datetime, block_mean
#from profilehooks import profile



def make_ticklabels_invisible(subplots):

    for i, ax in enumerate(subplots):
        #ax.text(0.5, 0.5, "ax%d" % (i+1), va="center", ha="center")
        for tl in ax.get_xticklabels() + ax.get_yticklabels():
            tl.set_visible(False)


def plot_file_general(filepath, filename, fig, chip_subplots, im_list, cb,
                      temps = None, full_matrix = False, downsample = 1):
    """
    function which decides between offline and online viewing based on type
    of temps. Depending on case read either from dictionary (offline) or
    use floats from tuple as temps in header (online). Or nothing in terms
    of None.
    It reads the file given by filename and extracts the correct temperature
    data for the datetime from the list of temps (if existing)
    inputs:
     filepath: string containing the path to the file to be read
     filename: name of the file to be read
     fig: the matplotlib.figure object in which the images are stored
     chip_subplots: a list of matplotlib.subplot objects (one for each chip)
       which contain the axes and on which the matplotlib.image objects are defined
     im_list: a list of the actual matplotlib.image objects as they were created
       on the subplots.
     cb: a custom colorbar object to set the colorbar to any value, define relative
       of absolute colorbars etc.
     temps: either a tuple or a dictionary containing temperature data for the detector
     full_matrix: a bool determining whether we read full matrix events or zero
       suppressed data
     downsample: an int of a factor by which we downsample the events before plotting
       them
    """

    header_text = ""
    # create full path to file
    filepathName = os.path.join(filepath, filename)
    # and read that file
    if full_matrix == False:
        evHeader, chpHeaderList = read_zero_suppressed_data_file(filepathName)
        # now add the header for this event. We add it to the last axes in the list
        # (for no special reason)
        header_text = evHeader.get_event_header_text()
    else:
        # we misuse the chpHeaderList variable name here to store the data from the
        # full matrix frame. Horrible misuse of a dynamically typed language...
        evHeader, chpHeaderList = read_full_matrix_data_file(filepathName)
        header_text = add_line_to_header("", "Path", filepath)
        header_text = add_line_to_header(header_text, "Filename", filename)
        header_text = add_line_to_header(header_text, "Full matrix", True)

    if header_text == "":
        return None

    if type(temps) is dict:
        # case: offline viewing. temps is a dictionary containing all temps
        # with datetime object as key
        dtime = convert_datetime_str_to_datetime(evHeader.attr["dateTime"])
        date = nearest_date(temp_dict, dtime)
        IMB_temp, septem_temp = temp_dict[date]
    elif type(temps) is tuple:
        # case: online viewing, temps is a tuple of both temparatures
        IMB_temp, septem_temp = temps

    if temps is not None:
        # add to header in case we obtained temps
        header_text = add_line_to_header(header_text,
                                         "IMB temp (C)",
                                         IMB_temp)
        header_text = add_line_to_header(header_text,
                                         "septem temp (C)",
                                         septem_temp)

    plot_file(evHeader, chpHeaderList, header_text, fig, chip_subplots, im_list, cb, downsample)


#@profile
def plot_file(evHeader, chpHeaderList, header_text, fig, chip_subplots, im_list,
              cb, downsample):
    # function which performs actual plotting (i.e. updating of the data in the
    # images and redrawing) of the event display
    # inputs:
    #  evHeader: an eventHeader object, which contains the event header information,
    #    which will be written into the text boxes
    # chpHeaderList: a list of chipHeaderData objects, which contain the data for
    #    for each chip
    # header_text: the text for the box header so far, as it was created in
    #    `plot_file_general`
    # fig: the matplotlib.figure object in which the images are stored
    # chip_subplots: a list of matplotlib.subplot objects (one for each chip)
    #    which contain the axes and on which the matplotlib.image objects are defined
    # im_list: a list of the actual matplotlib.image objects as they were created
    #    on the subplots.
    # cb: a custom colorbar object to set the colorbar to any value, define relative
    #    of absolute colorbars etc.
    # downsample: an int of a factor by which we downsample the events before plotting
    #    them

    # TODO: rewrite the plotting parts such that it is more neatly combined for
    # full matrix and zero suppressed readout


    # using im_list we now define a variable for the number of chips we have
    # (single or septem). That way, we can distinguish in the loop in which
    # we set the data
    nChips = len(im_list)

    # determine whether we are plotting full matrix or zero suppressed frames
    full_matrix = evHeader.attr["full_matrix_flag"]

    # and remove the text symbol from before
    texts = fig.texts
    for i in range(len(texts)):
        texts[-1].remove()

    # and put it at the top
    header_box = fig.text(0.5, 0.9, header_text,
                          bbox={'facecolor':'blue', 'alpha':0.1, 'pad':15},
                          family = 'monospace',
                          ## NOTE: `transform` either never worked in python 2 & old matplotlib, or
                          ## behavior changed significantly
                          #transform = chip_subplots[-1].transAxes,
                          horizontalalignment = 'center',
                          verticalalignment = 'center',
                          multialignment = 'left')

    # define the variable, in which we store the number of hits for each chip to
    # print at the top left
    hits_text = "Hits\n"

    singleChip = True

    plots_to_hide = [i for i in range(nChips)]
    if full_matrix == False:
        for i, chpHeader in enumerate(chpHeaderList):
            chip_data = chpHeader.pixData
            # now get current chip number so that we plot on to the correct chip
            chipNum = int(chpHeader.attr["chipNumber"])
            if singleChip:
                #chipNum = 0
                print("Chip: ", chipNum)

            # using the chip number we can determine whether we still continue or stop now
            # (relevant for single chip plotting. Then we don't want to plot more than chip #1)
            if chipNum > nChips:
                # if the chip number is larger than nChips (note, not >=, because we start
                # counting chips at 1.
                plots_to_hide = plots_to_hide[:chipNum]
                break

            # and get the number of hits (we use numHits)
            numHits = chpHeader.attr["numHits"] # int(np.size(chip_data))
            # use both to create the hits box
            hits_text += "Chip #%i : %s" % (chipNum, numHits)
            if chipNum != 7:
                hits_text += "\n"
            try:
                chip_full_array = np.zeros((256, 256))
                # now input the chip data into the image array (need to invert
                # x and y column. imshow shows the data as row, column
                # which means y, x coordinates
                chip_full_array[chip_data[:,1], chip_data[:,0]] = chip_data[:,2]

                # now create an image, but rather use im_list to set the correct image data

                # apply potential downsampling
                if downsample != 1:
                    chip_full_array = block_mean(chip_full_array, downsample)

                im_list[chipNum].set_data(chip_full_array)

                # # now remove this chip from the plots_to_hide list
                plots_to_hide.remove(chipNum)
                im_list[chipNum].set_visible(True)


                if cb.flag == True:
                    # before we can set the colorscale, we need to get the array, which
                    # only contains nonzero elements
                    data_nonzero = chip_full_array[np.nonzero(chip_full_array)]
                    color_value  = np.percentile(data_nonzero, cb.value)

                    im_list[chipNum].set_clim(0, color_value)
                else:
                    im_list[chipNum].set_clim(0, cb.value)

                # not needed anymore
                #im = chip_subplots[chipNum-1].imshow(chip_full_array, interpolation='none', axes=chip_subplots[chipNum-1])#, vmin=0, vmax=250)

            except IndexError:
                print('IndexError: chip', chpHeader.attr["chipNumber"], ' has no hits')
                print(chip_data)
            except TypeError:
                print('TypeError: chip', chpHeader.attr["chipNumber"], ' has no hits')
    else:
        # in this case plot full matrix frames
        # get the full chip array for the event
        chip_full_array = chpHeaderList
        numHits = np.count_nonzero(chip_full_array)
        # NOTE: we currently only support a single chip for full matrix readout
        # might change, but set chipNum to 0
        chipNum = 0
        # use both to create the hits box
        hits_text += "Chip #%i : %s\n" % (chipNum, numHits)
        # now create an image, but rather use im_list to set the correct image data
        im_list[chipNum].set_data(chip_full_array)

        # # now remove this chip from the plots_to_hide list
        plots_to_hide.remove(chipNum)
        im_list[chipNum].set_visible(True)


        if cb.flag == True:
            # before we can set the colorscale, we need to get the array, which
            # only contains nonzero elements
            data_nonzero = chip_full_array[np.nonzero(chip_full_array)]
            if len(data_nonzero) > 0:
                color_value  = np.percentile(data_nonzero, cb.value)
            else:
                color_value  = 0

            im_list[chipNum].set_clim(0, color_value)
        else:
            im_list[chipNum].set_clim(0, cb.value)


    # now create the hits box
    hits_box = fig.text(0.2, 0.9, hits_text,
                        bbox={'facecolor':'blue', 'alpha':0.1, 'pad':15},
                        family = 'monospace',
                        #transform = chip_subplots[-1].transAxes,
                        horizontalalignment = 'center',
                        verticalalignment = 'center',
                        multialignment = 'left')

    # now set all plots invisible, which were not updated this time (only done, if we even
    # have more than 1 plot) and update the colorbar
    for i in plots_to_hide:
        im_list[i].set_visible(False)


    if nChips > 1:
        # update colorbar
        cb.update_normal(im_list[cb.chip])
        cb.update_normal(im_list[cb.chip])
    else:
        # only update colorbar in this case
        cb.update_normal(im_list[0])
        cb.update_normal(im_list[0])
        #cb.update_normal(im_list[0])
        # now we draw the canvas again. This is only done, because when using an animation
        # (auto updating), sometimes otherwise we'd end up with a blank canvas where the plots
        # should be.
        # alternatively, one can call the update_normal function again
        fig.canvas.draw()
        fig.canvas.draw()

    # NOTE: pure debugging to see if more and more artists are being created
    # axes = fig.get_axes()
    # for i, ax in enumerate(axes):
    #     print 'Axes number ', i
    #     print 'name ', ax.name
    #     print 'title ', ax.title
    #     children = ax.get_children()
    #     for child in children:
    #         print child
    #         # if i == 2:
    #         #     print matplotlib.artist.getp(child)
    #     print '\n\n'

    make_ticklabels_invisible(chip_subplots)

    # and now plot everythin
    plt.pause(0.000001)

    return chip_subplots + im_list



#@profile
def plot_fadc_file(filepath, filename, fadcPlot, fadcPlotLine):#, fadc):

    print('working on ', filename)#self.filelist[self.i]
    # make sure fadc plot is visible
    fadcPlotLine[0].set_visible(True)



    # first we create an FADC object and give the filename to it

    # create full path to file
    filepathName = os.path.join(filepath, filename)
    # creating the fadc object takes care of pedestal runs, temporal corrections
    # and the splitting by channels
    fadc = Fadc(filepathName)

    #channel0 = fadc.channel0
    #channel1 = fadc.channel1
    #channel2 = fadc.channel2
    channel3 = fadc.channel3

    # and plot everything
    # create title build from Run folder and filename:
    plot_title = filepath.rstrip("/").split("/")[-1] + "/" + filename
    fadcPlot.set_title(plot_title)
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


def plot_occupancy(filename,
                   header_text,
                   sep,
                   fig,
                   chip_subplots,
                   im_list,
                   chip_arrays,
                   cb):
    # this function plots the occupancy plots, which are created by the
    # create_occupancy_plot function

    # define nChips based on the chip_subplots list, to differentiate between
    # single chip and septemboard
    nChips = len(chip_subplots)

    # first remove the texts from before, if any
    texts = fig.texts
    for i in range(len(texts)):
        texts[-1].remove()

    # define the variable, in which we store the number of hits for each chip to
    # print at the top left
    hits_text = "".ljust(10) + "Hits".ljust(10) + "max values\n"

    # we're going to use the first event of the run to create the event
    # header for the occupancy plot
    header_box = fig.text(0.5, 0.9, header_text,
                          bbox={'facecolor':'blue', 'alpha':0.1, 'pad':15},
                          family = 'monospace',
                          #transform = chip_subplots[-1].transAxes,
                          horizontalalignment = 'center',
                          verticalalignment = 'center',
                          multialignment = 'left')


    # now perform plotting
    plots_to_hide = [i for i in range(7)]
    for i, chip_array in enumerate(chip_arrays):
        # using the iterator i, we determine if we break or not
        # (relevant for single chip plotting. Then we don't want to plot more than chip #1)
        if i > nChips:
            # if i is larger than nChips, we break
            break

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

            if cb.flag == True:
                # before we can set the colorscale, we need to get the array, which
                # only contains nonzero elements
                data_nonzero = chip_array[np.nonzero(chip_array)]
                color_value  = np.percentile(data_nonzero, cb.value)
                im_list[i].set_clim(0, color_value)
            else:
                im_list[i].set_clim(0, cb.value)

        except IndexError:
            print('IndexError: chip', i, ' has no hits')
        except TypeError:
            print('TypeError: chip', i, ' has no hits')

    # now create the hits box
    hits_box = fig.text(0.2, 0.9, hits_text,
                        bbox={'facecolor':'blue', 'alpha':0.1, 'pad':15},
                        family = 'monospace',
                        #transform = chip_subplots[-1].transAxes,
                        horizontalalignment = 'center',
                        verticalalignment = 'center',
                        multialignment = 'left')

    # now set all plots invisible, which were not updated this time (only if septemboard)
    if nChips > 1:
        for i in plots_to_hide:
            im_list[i].set_visible(False)
        # update colorbar
        cb.update_normal(im_list[cb.chip])
    else:
        # only update colorbar in this case
        cb.update_normal(im_list[0])

    make_ticklabels_invisible(chip_subplots)

    # now save the file
    plt.savefig(filename + ".pdf")

    # and now plot everythin
    plt.pause(0.01)


def plot_pixel_histogram(filepath,
                         fig,
                         sep,
                         chip_subplots,
                         im_list,
                         nHitsList,
                         single_chip_flag):

    # this function plots the pixel histogram, i.e. a histogram of the number
    # of pixels active in an event of the whole run

    # define nChips based on the chip_subplots list, to differentiate between
    # single chip and septemboard
    nChips = len(chip_subplots)

    # first remove the texts from before, if any
    texts = fig.texts
    for i in range(len(texts)):
        texts[-1].remove()

    # define the variable, in which we store the number of hits for each chip to
    # print at the top left
    # hits_text = "".ljust(10) + "Hits".ljust(10) + "max values\n"

    # now perform plotting
    plots_to_hide = [i for i in range(7)]
    print(nChips, 'h')
    for i, hitsList in enumerate(nHitsList):
        # using the iterator i, we determine if we break or not
        # (relevant for single chip plotting. Then we don't want to plot more than chip #1)
        if i >= nChips:
            break

        # now either plot a single plot (if single chip), or one for each
        # chip

        im_list[i].set_visible(False)

        #fig2 = plt.figure(2)

        #plt.hist(hitsList, 100, histtype = 'bar')
        pos = chip_subplots[i].get_position()
        print(pos)

        print(pos.x0, pos.y0, pos.x1, pos.y1)
        histo = fig.add_axes([pos.x0 + 0.05 * pos.width, pos.y0, pos.width * 0.95, pos.height * 0.95])
        histo.hist(hitsList, 20, histtype = 'bar')

        #chip_subplots[i].hist(hitsList, 100, histtype = 'bar')
        #else:


        # hits_text += ("Chip #%i : %i" % (i, numHits)).ljust(20)
        # hits_text += str(int(maxVals))
        #if i != 6:
        #    hits_text += "\n"

        # TODO: still need plots_to_hide for case in which histogram of one chip
        # might be empty?
        # plots_to_hide.remove(i)


    # now create the hits box
    # hits_box = fig.text(0.2, 0.9, hits_text,
    #                     bbox={'facecolor':'blue', 'alpha':0.1, 'pad':15},
    #                     family = 'monospace',
    #                     transform = chip_subplots[-1].transAxes,
    #                     horizontalalignment = 'center',
    #                     verticalalignment = 'center',
    #                     multialignment = 'now')

    axes = fig.get_axes()
    for ax in axes:
        print(ax.get_children())

    #fig.canvas.draw()

    # left set all plots invisible, which were not updated this time (only if septemboard)
    #if nChips > 1:
    for i in range(nChips):
        print('visible no', i)
        im_list[i].set_visible(False)

    make_ticklabels_invisible(chip_subplots)

    # and now plot everythin
    plt.pause(0.01)
