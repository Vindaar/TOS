#!/usr/bin/env python

import numpy as np
import matplotlib
#matplotlib.use('pdf')
import matplotlib.pyplot as plt
import pylab
import os
from septemFiles import create_files_from_path_fadc, readFadcFile

import time


    

# class which contains all necessary functions to work on the matplotlib graph
class WorkOnFile:
    def __init__(self, filepath, filelist, figure, axarr):
        self.filepath = filepath
        self.filelist = filelist
        self.fig      = figure
        self.axarr    = axarr
        self.i        = 0
        self.nfiles   = len(filelist)
    
    def connect(self):
        self.cidpress = self.fig.canvas.mpl_connect('key_press_event', self.press)
        self.work_on_file(self.filelist[self.i])

    def refresh_filepath(self):
        # this function refreshes from the filepath
        files = create_files_from_path_fadc(self.filepath)
        self.filelist = files        

    def press(self, event):
        c = event.key
        sys.stdout.flush()
        if c == 'n':
            print ''
            print 'keypress read:', c
            print 'going to next FADC file #', self.i
            self.i += 1            
            if self.i < self.nfiles:
                self.work_on_file(self.filelist[self.i])
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
                self.work_on_file(self.filelist[self.i])
            else:
                self.work_on_file(self.filelist[self.i])
        elif c == 'e':
            # if e is typed, we jump to the end of the list and automatically
            # always get the last frame
            print 'jumping to last file and start automatically refreshing'
            self.loop_work_on_file()
                        
                
        elif c == 'q':
            print ''
            print c, 'was pressed. Exit program.'
            plt.close()
            self.disconnect()
        # else:
        #     print ''
        #     print 'neither n nor b was pressed. Display same file again'
        #     self.work_on_file(self.filelist[self.i])

    def loop_work_on_file(self):
        #while True:
        self.refresh_filepath()
        print 'length of file list ', len(self.filelist)
        self.work_on_file(self.filelist[-1])
        #time.sleep(1)

    def work_on_file(self, filename):

        print 'working on ', filename#self.filelist[self.i]
        
        # clear plot from before and write labels
        self.axarr.clear()
        self.axarr.set_xlabel('Time / clock cycles')
        self.axarr.set_ylabel('U / fadc ticks')

        filepath_name = self.filepath + "/" + filename

        fadc_values = readFadcFile(filepath_name)
        # for now hardcoded pedestal file
        pedestalFile = "/home/ingrid/TOS/bin/data/pedestalRuns/pedestalRun000042_1_182143774.txt-fadc"
        # apply the pedestal run
        fadc_values = applyPedestalRun(pedestalFile, fadc_values)
        # after applying the pedestal, also apply temporal correction
        fadc_values_t_corr = performTemporalCorrection(filepath_name, fadc_values)

        channel0, channel1, channel2, channel3 = getChannelsFromFadcArray(fadc_values)
        channel0_t, channel1_t, channel2_t, channel3_t = getChannelsFromFadcArray(fadc_values_t_corr)

        print np.size(channel0), np.size(channel1), np.size(channel2), np.size(channel3)
        print channel0, channel1, channel2, channel3

        # and plot everything
        self.axarr.set_title(filepath_name)
        self.axarr.plot(np.arange(np.size(channel0)), channel0, color='purple')
        self.axarr.plot(np.arange(np.size(channel1)), channel1, color='red')
        self.axarr.plot(np.arange(np.size(channel2)), channel2, color='green')
        self.axarr.plot(np.arange(np.size(channel3)), channel3, color='blue')
        self.axarr.plot(np.arange(np.size(channel3_t)), channel3_t, color='magenta')

        plt.draw()

    def disconnect(self):
        self.fig.canvas.mpl_disconnect(self.cidpress)
    

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
        files = create_files_from_path_fadc(folder)
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

    #plt.savefig('test.pdf')

    fig, axarr = plt.subplots(1, sharex=True, figsize=(10,8), dpi=100)
    
    files = WorkOnFile(folder, files, fig, axarr)
    files.connect()
    plt.show()

if __name__=="__main__":
    import sys
    main(sys.argv[1:])
