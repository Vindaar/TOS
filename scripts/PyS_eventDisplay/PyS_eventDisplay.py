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
from septemClasses import chip, septem_row, septem, eventHeader, chipHeaderData, Fadc
from septemFiles import create_files_from_path_combined
from septemPlot  import plot_file, plot_fadc_file
from septemFiles import read_zero_suppressed_data_file
import multiprocessing as mp
from profilehooks import profile
import collections
from multiprocessing.managers import BaseManager, Namespace, NamespaceProxy
import time
    
##################################################
########## WORK ON FILE ##########################
##################################################

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
        ns.filelist = create_files_from_path_combined(filepath, True)
        ns.filelistEvents = ns.filelist.keys()
        ns.filelistFadc   = ns.filelist.values()
        ns.nfiles         = len(ns.filelistEvents)
        refreshInterval   = ns.refreshInterval
        lock.release()
        time.sleep(refreshInterval)

# class which contains all necessary functions to work on the matplotlib graph
class WorkOnFile:
    def __init__(self, filepath, figure, septem, chip_subplots, fadcPlot, ns):
        self.filepath       = filepath
        self.filelist       = []
        self.filelistFadc   = []
        #self.filesDict      = filesDict
        self.ns             = ns

        self.fig            = figure
        self.i              = 0
        self.nfiles         = self.ns.nfiles
        self.septem         = septem
        self.chip_subplots  = chip_subplots
        self.fadcPlot       = fadcPlot
        self.fadcPlotLine   = self.fadcPlot.plot([], [], color = 'blue')
        # write the labels for the FADC plot (only needed to be done once, so no need to call
        # on each call to FADC plot)
        self.fadcPlot.set_xlabel('Time / clock cycles')
        self.fadcPlot.set_ylabel('U / fadc ticks')
        # initialize the FADC plot to be invisible, since possible that no FADC event
        # for first event
        self.fadcPlotLine[0].set_visible(False)

        # zero initialized numpy array
        temp_array         = np.zeros((256, 256))
        self.im_list       = [chip_subplots[i].imshow(temp_array, interpolation='none', axes=chip_subplots[i], vmin=0, vmax=250) for i in xrange(len(self.chip_subplots))]
        for im in self.im_list:
            im.set_cmap('viridis')

        # and now create the colorbar
        try:
            cbaxes = self.fig.add_axes([self.septem.row2.right - 0.015, self.septem.row3.bottom, 0.015, (self.septem.row3.top - self.septem.row3.bottom)])
            cb = plt.colorbar(self.im_list[-1], cax = cbaxes)
        except UnboundLocalError:
            print filename

    
    def connect(self):
        # connect both figures (septem and FADC windows) to the keypress event
        self.cidpress     = self.fig.canvas.mpl_connect('key_press_event', self.press)

    def disconnect(self):
        # disconnect both windows again
        self.fig.canvas.mpl_disconnect(self.cidpress)
        lock.acquire()
        self.ns.doRefresh = False
        lock.release()
        # upon disconnecting, we also stop the filelist updater thread

    def refresh_filepath_cont(self):
        # this function is called by the filelist updater thread, which continuously 
        # updates the filelist, based on the interval given by updateInterval
        filesDict = create_files_from_path_combined(self.filepath)


    def refresh_filepath(self):
        # this function refreshes from the filepath
        files, filesFadc = create_files_from_path_combined(self.filepath)
        self.filelist = files
        self.filelistFadc = filesFadc

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
            if self.i > 0:
                self.i -= 1
                print self.i
                self.work_on_file()
            else:
                self.work_on_file()
        elif c == 'e':
            # if e is typed, we jump to the end of the list and automatically
            # always get the last frame
            print 'jumping to last file and start automatically refreshing'
            self.loop_work_on_file_end()
        elif c == 'l':
            # if l is typed, we start to automatically go through all files
            # in the filelist
            print 'jumping to last file and start automatically refreshing'
            self.loop_work_on_file()
                        
                
        elif c == 'q':
            print ''
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
        ani     = MyFuncAnimation(self.fig, self.work_on_file_interactive_end, interval=500, blit=False)
        plt.show()
        
    def loop_work_on_file(self):
        # this function is used to automatically run over all files of a given frame
        ani = MyFuncAnimation(self.fig, self.work_on_file, interval=200, blit=False)
        plt.show()

    def work_on_file(self, i = None):
        # input i: i is the index for which we plot the file. if none is given, we simply use
        #          the member variable self.i
        #          allows one to use the animation functions in order to automatically run over all
        #          files

        if i is None:
            # if no i as argument given, use self.i
            i = self.i

        # set the filenames to None, to easier check whether files dictionary was already
        # populated
        filename     = None
        filenameFadc = None

        # acquire lock and get necessary elements from namespace
        lock.acquire()
        if self.ns.nfiles > 0:
            filename     = self.ns.filelistEvents[i]
            filenameFadc = self.ns.filelistFadc[i]
        lock.release()
        # now plot septem
        if filename is not None:
            plot_file(self.filepath, filename, self.septem, self.fig, self.chip_subplots, self.im_list)
            if filenameFadc is not "":
                # only call fadc plotting function, if there is a corresponding FADC event
                plot_fadc_file(self.filepath, filenameFadc, self.fadcPlot, self.fadcPlotLine)
            else:
                # else set fadc plot to invisible
                print "No FADC file found for this event."
                self.fadcPlotLine[0].set_visible(False)
                self.fig.canvas.draw()

    def work_on_file_interactive_end(self, i):
        # this function is being called by matplotlib animation to perform automatic updates
        # of the filelist and always plot the last element of the list
        # it is simply a wrapper around work_on_file, with the argument -1, as to always call the 
        # last element of the files dictionary
        self.work_on_file(-1)


def main(args):

    singleFile = False
    if len(args) > 0:
        try:
            f = open(args[0], 'r')
            singleFile = True
            f.close()
        except IOError:
            folder = args[0]
    else:
        print 'No argument given. Please give a folder from which to read files'
        import sys
        sys.exit()


    # get list of files in folder
    if singleFile == False:
        files, filesFadc = create_files_from_path_combined(folder)
    else:
        path = args[0].split('/')[:-1]
        folder = ""
        for el in path:
            folder += el + '/'
        files = [args[0].split('/')[-1]]
        print folder
        print files


    print folder#, files
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

    # enable interactive plotting


    # define a list of chip subplots
    chip_subplots = []

    # need to add row 3 first, since this is actually chip 1, 2 from left to right
    row3 = gridspec.GridSpec(1, 2)
    row3.update(left=sep.row3.left, right=sep.row3.right, wspace=sep.row3.wspace, top=sep.row3.top, bottom = sep.row3.bottom, hspace=0)
    ch1 = fig.add_subplot(row3[0, 0])
    chip_subplots.append(ch1)
    ch2 = fig.add_subplot(row3[0, 1])
    chip_subplots.append(ch2)

    # now add row 2, with chips 3, 4 and 5, from left to right
    row2 = gridspec.GridSpec(1, 3)
    row2.update(left=sep.row2.left, right=sep.row2.right, wspace=sep.row2.wspace, top=sep.row2.top, bottom = sep.row2.bottom, hspace=0)
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
    row1.update(left=sep.row1.left, right=sep.row1.right, wspace=sep.row1.wspace, top=sep.row1.top, bottom = sep.row1.bottom, hspace=0)
    # and add the correct chips to the grid
    ch6 = fig.add_subplot(row1[0, 1])
    chip_subplots.append(ch6)
    ch7 = fig.add_subplot(row1[0, 0])
    chip_subplots.append(ch7)


    grid = gridspec.GridSpecFromSubplotSpec(3, 1, [row1, row2, row3])
    
    filelistManager = myManager()
    filelistManager.start()
    
    #tm = mp.Manager()
    
    ns                 = mp.Manager().Namespace()
    ns.doRefresh       = True
    ns.filelist        = filelistManager.OrderedDict()
    ns.filelistEvents  = []
    ns.filelistFadc    = []
    ns.nfiles          = 0
    ns.refreshInterval = 0.2
    print ns
    
    files = WorkOnFile(folder, fig, sep, chip_subplots, fadcPlot, ns)
    files.connect()

    
    p2 = mp.Process(target = refresh, args = (ns, folder) )
    p2.start()
    plt.show()
    p2.join()

if __name__=="__main__":
    import sys
    main(sys.argv[1:])