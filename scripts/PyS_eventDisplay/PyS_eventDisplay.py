#!/usr/bin/env python

import numpy as np
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import matplotlib.animation as animation
from matplotlib.lines import Line2D
import os
import time
from septemModule.septemClasses import chip, septem_row, septem, eventHeader, chipHeaderData, Fadc, customColorbar
from septemModule.septemFiles import create_files_from_path_combined, read_zero_suppressed_data_file, create_filename_from_event_number
from septemModule.septemPlot  import plot_file, plot_fadc_file, plot_occupancy, plot_pixel_histogram

import multiprocessing as mp
import collections
from multiprocessing.managers import BaseManager, Namespace, NamespaceProxy
import time
    
class myManager(BaseManager):
    pass

myManager.register('OrderedDict', collections.OrderedDict)

lock = mp.Lock()

from matplotlib import animation
class MyFuncAnimation(animation.FuncAnimation):
    """
    Unfortunately, it seems that the _blit_clear method of the Animation
    class contains an error in several matplotlib verions
    That's why, I fork it here and insert the latest git version of
    the function.
    """
    def _blit_clear(self, artists, bg_cache):
        # Get a list of the axes that need clearing from the artists that
        # have been drawn. Grab the appropriate saved background from the
        # cache and restore.
        axes = set(a.axes for a in artists)
        for a in axes:
            if a in bg_cache: # this is the previously missing line
                a.figure.canvas.restore_region(bg_cache[a])



def refresh(ns, filepath):
    while ns.doRefresh == True:
        lock.acquire()
        #print 'updating filelist'
        # NOTE: the following commented lines are used, if one reads the data into
        # and OrderedDict. However, sorting this dictionary is exceptionally slow
        # as I only realized now. Thus, we go back to the old version of simply
        # creating two seperate lists for events and FADC events. This is much faster
        # (about a factor of 2! on 50000 files: 150ms for creating the list and
        #  another 220ms for sorting of the OrderedDict)
        # ns.filelist       = create_files_from_path_combined(filepath, True)
        # ns.filelistEvents = ns.filelist.keys()
        # ns.filelistFadc   = ns.filelist.values()
        #ns.filelistEvents, ns.filelistFadc = create_files_from_path_combined(filepath, False)
        ns.eventSet, ns.fadcSet = create_files_from_path_combined(filepath,
                                                                  ns.eventSet,
                                                                  ns.fadcSet,
                                                                  False)
        ns.nfiles         = len(ns.eventSet)
        refreshInterval   = ns.refreshInterval
        lock.release()
        time.sleep(refreshInterval)
        #print 'done updating filelist'



##################################################
########## WORK ON FILE ##########################
##################################################

# NOTE: this class is included in the main script, because it is the workhorse
# of the whole GUI. Thus, it might not be wise to put it into the septemClasses
# file.

# class which contains all necessary functions to work on the matplotlib graph
class WorkOnFile:

    def __init__(self, 
                 filepath, 
                 figure, 
                 septem, 
                 chip_subplots, 
                 fadcPlot, 
                 ns, 
                 cb,
                 start_iter):
        self.filepath         = filepath
        self.current_filename = ""
        # and assign the namespace of the multiprocessing manager
        self.ns               = ns

        self.fig              = figure
        self.i                = start_iter
        self.nfiles           = self.ns.nfiles
        self.septem           = septem
        self.chip_subplots    = chip_subplots
        # from the chip_suplots list we now deduce, whether we plot for a single
        # chip or a septemboard
        self.single_chip_flag = False
        if len(self.chip_subplots) == 1:
            # turn to true if only one axis object in list
            self.single_chip_flag = True


        self.fadcPlot       = fadcPlot
        self.fadcPlotLine   = self.fadcPlot.plot([], [], color = 'blue')

        # write the labels for the FADC plot (only needed to be done once, so no need to call
        # on each call to FADC plot)
        self.fadcPlot.set_xlabel('Time / clock cycles')
        self.fadcPlot.set_ylabel('U / fadc ticks')
        # initialize the FADC plot to be invisible, since possible that no FADC event
        # for first event
        #self.fadcPlotLine[0].set_visible(False)

        # assign the colorbar object to a member variable
        self.cb = cb


        # animation related:
        self.ani_end_event_source = None
        self.ani_end_running      = False
        self.ani_loop_event_source = None
        self.ani_loop_running      = False

        

        # zero initialized numpy array
        temp_array         = np.zeros((256, 256))
        #self.im_list       = [chip_subplots[i].imshow(temp_array, interpolation='none', axes=chip_subplots[i], vmin=0, vmax=250) for i in xrange(len(self.chip_subplots))]
        self.im_list       = [chip_subplots[i].imshow(temp_array,
                                                      interpolation='none',
                                                      axes=chip_subplots[i],
                                                      vmin=0)
                              for i in xrange(len(self.chip_subplots))]
        # and set the colormaps for the plots
        for im in self.im_list:
            im.set_cmap('viridis')

        # now to create the colorbar
        try:
            # either set the colorbar to the center chip in case of the septemboard or 
            # to chip 0 in case of a single chip
            # NOTE: the hardcoding of the numbers here isn't really nice. But since it's 
            # a one time thing it should be excused.
            if len(self.chip_subplots) > 1:
                cbaxes = self.fig.add_axes([self.septem.row2.right - 0.015, 
                                            self.septem.row3.bottom, 
                                            0.015, 
                                            (self.septem.row3.top - self.septem.row3.bottom)])
                cb_object = plt.colorbar(self.im_list[self.cb.chip], cax = cbaxes)
            else:
                cbaxes = self.fig.add_axes([self.septem.row2.right + 0.005, 
                                            self.septem.row3.bottom + 0.036, 
                                            0.015, 
                                            (self.septem.row3.top - self.septem.row3.bottom)])
                cb_object = plt.colorbar(self.im_list[0], cax = cbaxes)
            self.fig.canvas.draw()
        except UnboundLocalError:
            print filename

        # and assign newly created colorbar as member variable
        self.cb.assign_colorbar(cb_object)

        # and now invert the correct plots, if septemboard is used
        if self.single_chip_flag == False:
            for i in xrange(8):
                if i not in [6, 7]:
                    # in case of chips 1 to 5, we need to invert the y axis. Bonds are below the chips,
                    # thus (0, 0) coordinate is at bottom left. default plots (0, 0) top left
                    self.chip_subplots[i - 1].invert_yaxis()
                else:
                    # in case of chips 6 and 7, the bond area is above the chips, meaning (0, 0) 
                    # coordinate is at the top right. thus invert x axis
                    self.chip_subplots[i - 1].invert_xaxis()
                    #self.chip_subplots[i - 1].invert_yaxis()
                    
            # for some reason I'm not entirely sure of right now, we also need to
            # invert the y axis of the last chip (7)
            self.chip_subplots[6].invert_yaxis()
        else:
            self.chip_subplots[0].invert_yaxis()
    
    def connect(self):
        # before we connect the key press events, we check if refresh ran once
        # files are read, before we accept any input
        # we check 50 times if the number of files is larger than 0 (which
        # indicates that we have finished reading the files in the folder
        # at least once), and wait 100ms after each check.
        for _ in xrange(50):
            lock.acquire()
            nfiles = self.ns.nfiles
            lock.release()
            if nfiles > 0:
                print 'nfiles : ', nfiles
                break
            else:
                print 'waiting...'
                time.sleep(0.1)

        # connect both figures (septem and FADC windows) to the keypress event
        self.cidpress     = self.fig.canvas.mpl_connect('key_press_event', self.press)

    def disconnect(self):
        # disconnect both windows again
        self.fig.canvas.mpl_disconnect(self.cidpress)
        lock.acquire()
        self.ns.doRefresh = False
        lock.release()
        # upon disconnecting, we also stop the filelist updater thread

    def press(self, event):
        c = event.key
        sys.stdout.flush()
        if c == 'n':
            print ''
            print 'keypress read:', c
            print 'going to next file #', self.i
            self.i += 1
            lock.acquire()
            self.nfiles = self.ns.nfiles
            lock.release()
            if self.i < self.nfiles:
                self.work_on_file()
            else:
                plt.close()
                print 'reached last file in folder'
                self.disconnect()
        elif c == 'b':
            print ''
            print 'keypress read:', c
            print 'going to previous file'
            #if self.i > 0:
            self.i -= 1
            print self.i
            self.work_on_file()
            #else:
            #    self.work_on_file()
        elif c == 'e':
            # if e is typed, we jump to the end of the list and automatically
            # always get the last frame
            if self.ani_end_running == False:
                print 'jumping to last file and start automatically refreshing'
                self.loop_work_on_file_end()
            else:
                # in the event this animation is running, we stop it
                self.stop_animation()
        elif c == 'l':
            # if l is typed, we start to automatically go through all files
            # in the filelist
            if self.ani_loop_running == False:
                print 'jumping to last file and start automatically refreshing'
                self.loop_work_on_file()
            else:
                # stop the animation
                self.stop_animation()
        elif c == 'o':
            # if o is typed, we create an occupancy plot of the whole run
            print 'creating occupancy plot...'
            self.create_occupancy_plot()
        elif c == 's':
            # if s is typed, we save the current figure
            print 'saving figure...'
            self.save_figure(self.current_filename)
        elif c == 'h':
            # if h is typed, we create the pixel histogram for each chip
            print 'creating pixel histogram...'
            self.create_pixel_histogram()
                        
                
        elif c == 'q':
            print ''
            # call stop animation to stop any running animations,
            # before closing (makes sure there's no error on exit)
            wait_flag = self.stop_animation()
            if wait_flag is True:
                # we built in a wait of 2 seconds, to make sure the animation is actually
                # stopped before we quit
                time.sleep(2)
            print c, 'was pressed. Exit program.'
            
            plt.close('all')
            self.disconnect()
        # else:
        #     print ''
        #     print 'neither n nor b was pressed. Display same file again'
        #     self.work_on_file(self.filelist[self.i])

    def loop_work_on_file_end(self):
        # this function is used to automatically refresh the frames during a run, as to always
        # show the last frame
        ani = MyFuncAnimation(self.fig, self.work_on_file_interactive_end, interval=300, blit=False)
        self.ani_end_event_source = ani#.event_source
        self.ani_end_running      = True
        plt.show()
        
    def loop_work_on_file(self):
        # this function is used to automatically run over all files of a given frame
        ani = MyFuncAnimation(self.fig, self.work_on_file, interval=200, blit=False)
        self.ani_loop_event_source = ani#.event_source
        self.ani_loop_running      = True
        plt.show()

    def stop_animation(self):
        # this function stops the func animations
        # TODO: make sure that we DO stop the animation. currently
        # have to press several times to stop. problematic, 
        # because this way we cannot set the flag to False again,
        # because then it couldn't be stopped

        # define wait_flag which is returned to signal to the quit function
        # of the GUI, whether it should wait 2 seconds or not
        wait_flag = False
        
        
        if self.ani_end_running == True:
            print 'stopping the run animation...'
            # using the animation's _stop function (THANKS matplotlib source code...)
            # we can stop the animation properly
            self.ani_end_event_source._stop()
        if self.ani_loop_running == True:
            print 'stopping the loop animation...'
            self.ani_loop_event_source._stop()
        # now wait for 0.5 seconds to make sure it stops
        if self.ani_end_running is True or self.ani_loop_running is True:
            # first set the running flags to False (if we end up in here
            # either one or both are NOT running anymore, so it is fine
            # if both are set to False)
            self.ani_end_running  = False
            self.ani_loop_running = False
            print 'waiting for animations to stop...'
            wait_flag = True
            time.sleep(0.5)

        # return the wait_flag
        return wait_flag
        
    def work_on_file(self, i = None):
        # input i: i is the index for which we plot the file. if none is given, we simply use
        #          the member variable self.i
        #          allows one to use the animation functions in order to automatically run over all
        #          files

        if i is None:
            # if no i as argument given, use self.i
            i = self.i
        elif i == -1:
            # in case we have called work_on_file_end(), we supply -1 to work_on_file.
            # this can be problematic, if we stop the function and want to go back
            # from this event, because -1 is relative to the number of current events. so
            # in case we have an ongoing run, this will change all the time, making it
            # impossible to go back (because more and more events are created)
            lock.acquire()
            self.i = self.ns.nfiles
            lock.release()
        else:
            # else, we just set self.i to i
            self.i = i

        # set the filenames to None, to easier check whether files dictionary was already
        # populated
        filename     = None
        filenameFadc = None

        # acquire lock and get necessary elements from namespace
        lock.acquire()
        if self.ns.nfiles > 0:
            # now given the index i, which corresponds to our event number
            # we need to create the filenames for the event and the FADC
            # events
            filename     = create_filename_from_event_number(self.ns.eventSet,
                                                             i,
                                                             self.ns.nfiles,
                                                             fadcFlag = False)
            filenameFadc = create_filename_from_event_number(self.ns.fadcSet,
                                                             i,
                                                             self.ns.nfiles,
                                                             fadcFlag = True)
        lock.release()
        # now plot septem
        if filename is not None:
            # before we plot the file, we set the file, we're going to plot as the
            # current file (to save the figure, if wanted)
            self.current_filename = self.filepath.split('/')[-1] + "_" + filename
            
            plot_file(self.filepath, 
                      filename, 
                      self.septem,
                      self.fig,
                      self.chip_subplots,
                      self.im_list,
                      self.cb)
            #if filenameFadc is not "":
            if filenameFadc is not None:
                # only call fadc plotting function, if there is a corresponding FADC event
                plot_fadc_file(self.filepath, 
                               filenameFadc, 
                               self.fadcPlot, 
                               self.fadcPlotLine)
            else:
                # else set fadc plot to invisible
                print("No FADC file found for event %s." % filename)
                self.fadcPlotLine[0].set_visible(True)
                #self.fadcPlot.set_title(self.filepath + filenameFadc)
                self.fig.canvas.draw()

    def work_on_file_interactive_end(self, i):
        # this function is being called by matplotlib animation to perform automatic updates
        # of the filelist and always plot the last element of the list
        # it is simply a wrapper around work_on_file, with the argument -1, as to always call the 
        # last element of the files dictionary
        self.work_on_file(-1)

    def create_occupancy_plot(self):
        # TODO: think about another way to run over all files. NOT SURE, but it seems
        # that creating all filenames from scratch (via the function from the event number)
        # is (in this case) a waste of resources.
        # Idea: create function, which does what create_filename_from_event_number does
        # but returns list of filenames from eventSet
        
        # create an occupancy plot. count number of times each pixel was hit during the 
        # whole run

        # create a list of numpy arrays. one array for each occupancy plot of each chip
        chip_arrays = np.zeros((self.septem.nChips, 256, 256))# for _ in xrange(self.septem.nChips)]

        # we run over all files of the event number set, i.e. all files of run
        for eventNumber in self.ns.eventSet:
            # first create the filename
            filename     = create_filename_from_event_number(self.ns.eventSet,
                                                             eventNumber,
                                                             self.ns.nfiles,
                                                             fadcFlag = False)
            # we create the event and chip header object
            evHeader, chpHeaderList = read_zero_suppressed_data_file(self.filepath + filename)
            # now go through each chip header and add data of frame
            # to chip_arrays
            for chpHeader in chpHeaderList:
                # get the data of the frame for this chip
                chip_data = chpHeader.pixData
                chip_num  = int(chpHeader.attr["chipNumber"])
                # and now add chip data to chip_arrays if non empty
                if np.size(chip_data) > 0:
                    chip_arrays[chip_num - 1, chip_data[:,1], chip_data[:,0]] += 1

            event_num = int(evHeader.attr["eventNumber"])
            if event_num % 1000 == 0:
                print event_num, ' events done.'

        # now create the header text box for the occupancy plot
        if len(self.ns.eventSet) > 0:
            # need to read one of the files to get the header. Choose first file, thus
            filename     = create_filename_from_event_number(self.ns.eventSet,
                                                             0,
                                                             self.ns.nfiles,
                                                             fadcFlag = False)
            evHeader, chpHeaderList = read_zero_suppressed_data_file(self.filepath + filename)
            header_text = evHeader.get_run_header_text()
        else:
            print 'filelist not filled yet or empty; returning.'
            return

        nEvents = "\n# events : ".ljust(25)
        nEvents += str(len(self.ns.eventSet))

        header_text += nEvents

        # before we plot the file, we set the file, we're going to plot as the
        # current file (to save the figure, if wanted)
        self.current_filename = "occupancy_" + self.filepath.split('/')[-2]

        plot_occupancy(self.filepath, 
                       header_text,
                       self.septem, 
                       self.fig, 
                       self.chip_subplots, 
                       self.im_list,
                       chip_arrays,
                       self.cb)

    def save_figure(self, output):
        # this function simply saves the currently shown plot as a png in
        # the current working directory
        output = output.rstrip(".txt") + ".png"
        plt.savefig(output)

    def create_pixel_histogram(self):
        # create the pixel histogram for all chips of the current run
        
        # define a list of lists for the number of hits for each chip
        nHitsList = [ [] for _ in xrange(self.septem.nChips)]

        for eventNumber in self.ns.eventSet:
            # first create the filename to supply it to read function
            filename     = create_filename_from_event_number(self.ns.eventSet,
                                                             eventNumber,
                                                             self.ns.nfiles,
                                                             fadcFlag = False)
            # we create the event and chip header object
            evHeader, chpHeaderList = read_zero_suppressed_data_file(self.filepath + filename)
            # now go through each chip header and add data of frame
            # to chip_arrays
            for chpHeader in chpHeaderList:
                # get the data of the frame for this chip
                chip_data = chpHeader.pixData
                chip_num  = int(chpHeader.attr["chipNumber"])
                # and now add chip data to chip_arrays if non empty
                nHits = np.size(chip_data)
                if nHits > 0:
                    # now if we have 1 hits or more, add event to histogram
                    # chipnum - 1, because we count from 1
                    nHitsList[chip_num - 1].append(nHits)
                
            event_num = int(evHeader.attr["eventNumber"])
            if event_num % 1000 == 0:
                print event_num, ' events done.' 

        plot_pixel_histogram(self.filepath,
                             self.fig,
                             self.septem,
                             self.chip_subplots,
                             self.im_list,
                             nHitsList,
                             self.single_chip_flag)


def create_chip_axes(sep, fig, single_chip_flag):
    # this function receives the septem object and creates the axes for the chip layout
    # from it. The list of axes and the grid is returned
    
    # define a list of chip subplots
    chip_subplots = []

    if single_chip_flag is False:
        # need to add row 3 first, since this is actually chip 1, 2 from left to right
        row3 = gridspec.GridSpec(1, 2)
        row3.update(left=sep.row3.left, 
                    right=sep.row3.right, 
                    wspace=sep.row3.wspace, 
                    top=sep.row3.top, 
                    bottom = sep.row3.bottom, 
                    hspace=0)
        ch1 = fig.add_subplot(row3[0, 0])
        chip_subplots.append(ch1)
        ch2 = fig.add_subplot(row3[0, 1])
        chip_subplots.append(ch2)

        # now add row 2, with chips 3, 4 and 5, from left to right
        row2 = gridspec.GridSpec(1, 3)
        row2.update(left=sep.row2.left, 
                    right=sep.row2.right, 
                    wspace=sep.row2.wspace, 
                    top=sep.row2.top, 
                    bottom = sep.row2.bottom, 
                    hspace=0)
        # add the three center channels
        ch3 = fig.add_subplot(row2[0, 0])
        chip_subplots.append(ch3)
        ch4 = fig.add_subplot(row2[0, 1])
        chip_subplots.append(ch4)
        ch5 = fig.add_subplot(row2[0, 2])
        chip_subplots.append(ch5)

        # define the first row of subplots using gridSpec
        # first row is chips 7, 6 from left to right
        row1 = gridspec.GridSpec(1, 2)
        # updates the positions
        row1.update(left=sep.row1.left, 
                    right=sep.row1.right, 
                    wspace=sep.row1.wspace, 
                    top=sep.row1.top, 
                    bottom = sep.row1.bottom, 
                    hspace=0)
        # and add the correct chips to the grid
        ch6 = fig.add_subplot(row1[0, 1])
        chip_subplots.append(ch6)
        ch7 = fig.add_subplot(row1[0, 0])
        chip_subplots.append(ch7)

        # now create the grid object from the individual rows
        grid = gridspec.GridSpecFromSubplotSpec(3, 1, [row1, row2, row3])
    else:
        # in this case we only use a single chip
        chip = gridspec.GridSpec(1, 1)
        chip.update(left = sep.row2.left,
                    right = sep.row2.right, 
                    top = sep.row1.top, 
                    wspace = 0,
                    bottom = sep.row3.bottom,
                    hspace = 0)
        chipPlot = fig.add_subplot(chip[0])
        chip_subplots.append(chipPlot)
    
    return chip_subplots

def main(args):

    singleFile = False
    occupancyFlag = False
    # following flag used to differentiate between single chip and septemboard
    single_chip_flag = False

    # define flag and variable for the color bar settings
    # cb_flag defines that we not use a fixed value of the colorbar scale, but rather
    # the percentile given by cb_value of the center chip
    cb_flag  = True
    cb_value = 80
    # and the chip for which we set the colorbar (3 default, is the center chip of a
    # septemboard)
    cb_chip  = 3

    if len(args) > 0:
        try:
            f = open(args[0], 'r')
            singleFile = True
            f.close()
        except IOError:
            folder = args[0]

        # now we check whether the -o flag was given from the command line
        # this activates occupancy mode, i.e. it creates an occupancy plot of the whole run
        if "-o" in args:
            occupancy_flag = True
        if "--cb_flag" in args:
            try:
                ind  = args.index("--cb_flag")
                flag = args[ind + 1]
                if flag == "1":
                    cb_flag = True
                else:
                    cb_flag = False
            except IndexError:
                print 'If you enter cb_flag please enter 1 or 0 afterwards.'
                import sys
                sys.exit()
        if "--cb_value" in args:
            try:
                ind  = args.index("--cb_value")
                cb_value = float(args[ind + 1])
            except IndexError:
                print 'If you enter cb_value please enter a value to use afterwards.'
                import sys
                sys.exit()
        if "--cb_chip" in args:
            try:
                ind  = args.index("--cb_chip")
                cb_chip = int(args[ind + 1])
            except IndexError:
                print 'If you enter cb_chip please enter a value to use afterwards.'
                import sys
                sys.exit()
        if "--single_chip" in args:
            single_chip_flag = True
        if "--start_iter" in args:
            try:
                ind = args.index("--start_iter")
                start_iter = int(args[ind + 1])
            except IndexError:
                print 'If you enter --start_iter please enter a start value.'
                start_iter = 0
        else:
            start_iter = 0
    else:
        print 'No argument given. Please give a folder from which to read files'
        import sys
        sys.exit()


    # create a custom colorbar object to store colorbar related properties; 
    # initialize with values either as defaults from above or command line argument
    cb = customColorbar(cb_flag, cb_value, cb_chip)

    # get list of files in folder
    # if singleFile == False:
    #     files, filesFadc = create_files_from_path_combined(folder, None, None, False)
    if singleFile == True:
        path = args[0].split('/')[:-1]
        folder = ""
        for el in path:
            folder += el + '/'
        files = [args[0].split('/')[-1]]

    # initialize a septem object
    fig_x_size = 15
    fig_y_size = 10
    sep = septem(0.0, fig_x_size, fig_y_size, 0.8)
    # and use it to set the appropriate figure size
    x = fig_x_size
    y = fig_y_size# * sep.y_size / sep.x_size
    # define the figure for the septem board
    fig = plt.figure(figsize=(x, y))
    fig.canvas.set_window_title('This is a secret title!')
    # add a subplot for the FADC plot
    fadc = gridspec.GridSpec(1, 1)
    # set the fadc plot to some reasonable value.. 
    fadc.update(left=0.55, right=0.95, top=0.7, bottom=0.25)
    fadcPlot = fig.add_subplot(fadc[0])

    
    # create the axes for the chip layout 
    chip_subplots = create_chip_axes(sep, fig, single_chip_flag)


    # create a new filelist manager, which allows the communication between the main thread and 
    # the file checking thread
    filelistManager = myManager()
    filelistManager.start()
    
    # and create the namespace, which will be given to both threads, so they can access
    # the same resources
    ns                 = mp.Manager().Namespace()
    # define a flag for the refreshing thread, so it knows when to stop
    ns.doRefresh       = True
    # define the ordered dictionary, in which we store the filenames
    ns.filelist        = filelistManager.OrderedDict()
    # individual lists, which are generated from the dictionary
    # ns.filelistEvents  = []
    # ns.filelistFadc    = []
    # these two sets are the successors to the previously used list of
    # files. We now only store the event numbers and create the corresponding
    # filenames on the fly from the path
    ns.eventSet        = set()
    ns.fadcSet         = set()
    ns.nfiles          = 0
    # and the interval, in which the thread refreshes the filelist
    ns.refreshInterval = 0.0001
    print ns

    # we start the file refreshing (second) thread first, because we only
    # want to accept key inputs, after the files have been read a single
    # time.
    # and the second thread, which performs the refreshing
    p2 = mp.Process(target = refresh, args = (ns, folder) )
    p2.start()

    # now create the main thread, which starts the plotting
    files = WorkOnFile(folder, fig, sep, chip_subplots, fadcPlot, ns, cb, start_iter)
    files.connect()

    plt.show()
    p2.join()

if __name__=="__main__":
    import sys
    main(sys.argv[1:])
