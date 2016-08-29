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
    
##################################################
########## WORK ON FILE ##########################
##################################################



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



def CreateFadcObj(filePathName, out_q2):
    fadc = Fadc(filePathName)
    out_q2.put(fadc)

# class which contains all necessary functions to work on the matplotlib graph
class WorkOnFile:
    def __init__(self, filepath, figure, septem, chip_subplots, fadcPlot):
        self.filepath      = filepath
        self.filelist      = []
        self.filelistFadc  = []
        # now refresh the filepath to get the current filelists
        self.refresh_filepath()
        
        self.fig           = figure
        self.i             = 0
        self.nfiles        = len(self.filelist)
        self.septem        = septem
        self.chip_subplots = chip_subplots
        self.fadcPlot      = fadcPlot
        #self.fadcPlot.plot(np.arange(100), np.zeros(100), color='blue')
        self.fadcPlotLine  = self.fadcPlot.plot([], [], color = 'blue')
        #self.fadcPlot.add_line(self.fadcPlotLine)

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
        #self.cidpressFadc = self.figFadc.canvas.mpl_connect('key_press_event', self.press)
        self.work_on_file()

    def disconnect(self):
        # disconnect both windows again
        self.fig.canvas.mpl_disconnect(self.cidpress)
        #self.figFadc.canvas.mpl_disconnect(self.cidpress)

    def refresh_filepath(self):
        # this function refreshes from the filepath
        files, filesFadc = create_files_from_path_combined(self.filepath)
        if len(filesFadc) == 0:
            filesFadc = ['data000205.txt-fadc']
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
            #self.fig.close()
            #self.figFadc.close()
            plt.close('all')
            self.disconnect()
        # else:
        #     print ''
        #     print 'neither n nor b was pressed. Display same file again'
        #     self.work_on_file(self.filelist[self.i])

    def loop_work_on_file_end(self):
        # this function is used to automatically refresh the frames during a run, as to always
        # show the last frame
        self.refresh_filepath()
        print 'length of file list ', len(self.filelist)
        ani     = MyFuncAnimation(self.fig,     self.work_on_file_interactive_end, interval=500, blit=False)
        #aniFadc = animation.FuncAnimation(self.figFadc, self.work_on_file_interactive_end, interval=500)
        plt.show()

        
    def loop_work_on_file(self):
        # this function is used to automatically run over all files of a given frame
        self.refresh_filepath()
        print 'length of file list ', len(self.filelist)
        ani = MyFuncAnimation(self.fig, self.work_on_file_interactive, interval=200, blit=True)
        plt.show()

    def work_on_file(self):

        # NOTE: experimental multithreading
        # get the filename to work on
        filename     = self.filelist[self.i]
        filenameFadc = self.filelistFadc[self.i]

        # out_q1 = mp.Queue()
        # out_q2 = mp.Queue()

        # p1 = mp.Process(target=read_zero_suppressed_data_file, 
        #                 args=(self.filepath + filename, out_q1))
        # p2 = mp.Process(target=CreateFadcObj, 
        #                 args=(self.filepath + filenameFadc, out_q2))
        # # start the process
        # p1.start()
        # p2.start()

        # now plot septem
        plot_file(self.filepath, filename, self.septem, self.fig, self.chip_subplots, self.im_list)#, out_q1.get(), out_q1.get())
        # and fadc data
        plot_fadc_file(self.filepath, filenameFadc, self.fadcPlot, self.fadcPlotLine)#, out_q2.get())

        # p1.join()
        # p2.join()

        
    def work_on_file_interactive_end(self, i):
        # this function is being called by matplotlib animation to perform automatic updates
        # of the filelist and always plot the last element of the list
        # refresh filepath
        self.refresh_filepath()
        # and set filename to last element in list
        filename = self.filelist[-1]

        # now plot
        b = plot_file(self.filepath, filename, self.septem, self.fig, self.chip_subplots, self.im_list)
        # and fadc data
        if len(self.filelistFadc) > 0:
            filenameFadc = self.filelistFadc[-1]
            plot_fadc_file(self.filepath, filenameFadc, self.fadcPlot, self.fadcPlotLine)

        return b


    def work_on_file_interactive(self, i):
        # this function is being called by matplotlib animation to perform automatic updates
        # of the
        if i == len(self.filelist) - 1:
            # if i number of files in the filelist - 1, we refresh the list
            self.refresh_filepath()
        filename = self.filelist[i]

        filenameFadc = self.filelistFadc[i]
        # now plot
        b = plot_file(self.filepath, filename, self.septem, self.fig, self.chip_subplots, self.im_list)
        # and fadc data
        plot_fadc_file(self.filepath, filenameFadc, self.fadcPlot, self.fadcPlotLine)

        return b#self.chip_subplots

def main(args):

    # read a test file:
    # if len(args) > 0:
    #     filepath = args[0]
    # else:
    #     filepath = '/home/ingrid/PyS_eventDisplay/Run160813_19-03-12/data000105_1_190450879.txt'


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
        print 'stuff'
        for el in files:
            print el
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

    #grid.tight_layout(fig, rect=[0, 0, 0.5, 1])
    
    #grid.update(left=0.1, right=0.9, top=0.9, bottom=0.1)
    
        

    files = WorkOnFile(folder, fig, sep, chip_subplots, fadcPlot)
    files.connect()
    plt.show()
    #plot_file(filepath)


if __name__=="__main__":
    import sys
    main(sys.argv[1:])
