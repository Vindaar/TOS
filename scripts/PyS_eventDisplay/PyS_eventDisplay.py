#!/usr/bin/env python

import os
import time
import pyinotify
import argparse

import numpy as np
import matplotlib
#matplotlib.use('TkAgg')
#matplotlib.use('WXAgg')
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import matplotlib.animation as animation
from matplotlib.lines import Line2D

from septemModule.septemClasses import chip, septem, customColorbar, MyFuncAnimation, TempHandler, EventsHandler
from septemModule.septemFiles import create_files_from_path_combined, read_zero_suppressed_data_file, read_zsub_mp, create_filename_from_event_number, create_occupancy_filename, create_pickle_filename, check_occupancy_dump_exist, dump_occupancy_data, load_occupancy_dump, create_list_of_files, create_list_of_files_dumb, get_temp_filename, write_centroids_to_file
from septemModule.septemPlot  import plot_file_general, plot_fadc_file, plot_occupancy, plot_pixel_histogram
from septemModule.septemMisc import add_line_to_header, get_batch_num_hours_for_run, get_occupancy_batch_header, fill_classes_from_file_data_mp, calc_centroid

import multiprocessing as mp
from multiprocessing.managers import BaseManager, Namespace, NamespaceProxy

# number of pixels per dimension of a timepix chip
NPIX = 256

# create global lock on the multiprocessing
lock = mp.Lock()

def refresh(ns, filepath, temp_handler):
    while ns.doRefresh == True:
        lock.acquire()
        # on first call we read the files, which are already in the folder
        if ns.full_matrix_flag == False:
            ns.eventSet, ns.fadcSet = create_files_from_path_combined(filepath,
                                                                      ns.eventSet,
                                                                      ns.fadcSet,
                                                                      full_matrix = False)
            ns.nfiles         = len(ns.eventSet)
            # if no files in folder, set empty_folder to True
            if ns.nfiles == 0:
                ns.empty_folder = True
                # in this case last file is 0
                ns.last_file = 0
            else:
                ns.empty_folder = False
                ns.last_file      = max(ns.eventSet)
        else:
            ns.files, ns.fadcSet = create_files_from_path_combined(filepath,
                                                                   ns.eventSet,
                                                                   ns.fadcSet,
                                                                   full_matrix = True)
            # set number of files
            ns.nfiles = len(ns.files)
            if ns.nfiles > 0:
                ns.empty_folder = False
            else:
                ns.empty_folder = True
            ns.last_file = ns.files[-1]

        # get the refresh interval as local var
        refreshInterval   = ns.refreshInterval
        lock.release()


        # get current temps (written to ns.currentTemps is new available)
        if temp_handler is not None:
            temp_handler.get_current_temps()

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
                 args_dict):
        self.filepath         = filepath
        self.current_filename = ""
        # and assign the namespace of the multiprocessing manager
        self.ns               = ns

        self.fig              = figure
        self.i                = args_dict["start_iter"]
        self.nfiles           = self.ns.nfiles
        self.septem           = septem
        self.chip_subplots    = chip_subplots
        # from the chip_suplots list we now deduce, whether we plot for a single
        # chip or a septemboard
        self.single_chip_flag = args_dict["single_chip_flag"]

        # check whether we expect full matrix or zero suppressed readout frames
        self.full_matrix_flag = args_dict["full_matrix_flag"]

        # check whether we downsample all frames by some factor, set pix_dim accordingly
        self.downsample = int(args_dict["downsample"])
        self.pix_dim = int(NPIX / self.downsample)

        # TODO: check if the following is now redundant that we include the single_chip_flag
        # in the args_dict
        # if len(self.chip_subplots) == 1:
        #     # turn to true if only one axis object in list
        #     self.single_chip_flag = True
        self.ignore_full_frames  = args_dict["ignore_full_frames"]
        self.fadc_triggered_only = args_dict["fadc_triggered_only"]
        self.calc_centroid_flag  = args_dict["calc_centroids"]

        self.batches_dict = { "nbatches"     : args_dict["nbatches"],
                              "batches_flag" : args_dict["batches_flag"] }

        self.fadcPlot       = fadcPlot
        self.fadcPlotLine   = self.fadcPlot.plot([], [], color = 'blue')

        # write the labels for the FADC plot (only needed to be done once, so no need to call
        # on each call to FADC plot)
        self.fadcPlot.set_xlabel('Time / clock cycles')
        self.fadcPlot.set_ylabel('U / V')
        # initialize the FADC plot to be invisible, since possible that no FADC event
        # for first event
        #self.fadcPlotLine[0].set_visible(False)

        # assign the colorbar object to a member variable
        self.cb = cb

        # set the color map
        self.colormap = args_dict["colormap"]

        # animation related:
        self.ani_end_event_source  = None
        self.ani_end_running       = False
        self.ani_loop_event_source = None
        self.ani_loop_running      = False

        # zero initialized numpy array
        temp_array         = np.zeros((self.pix_dim // self.downsample,
                                       self.pix_dim // self.downsample))
        #self.im_list       = [chip_subplots[i].imshow(temp_array, interpolation='none', axes=chip_subplots[i], vmin=0, vmax=250) for i in xrange(len(self.chip_subplots))]
        self.im_list       = [chip_subplots[i].imshow(temp_array,
                                                      interpolation='none',
                                                      axes=chip_subplots[i],
                                                      vmin=0)
                              for i in range(len(self.chip_subplots))]
        # and set the colormaps for the plots
        for im in self.im_list:
            im.set_cmap(self.colormap)

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
            print("Warning: UnboundLocalError in init of WorkOnFile")
            print(filename)

        # and assign newly created colorbar as member variable
        self.cb.assign_colorbar(cb_object)

        # and now invert the correct plots, if septemboard is used
        if self.single_chip_flag == False:
            for i in range(8):
                if i not in [6, 7]:
                    # in case of chips 1 to 5, we need to invert the y axis. Bonds are below the
                    # chips, thus (0, 0) coordinate is at bottom left. default
                    # plots (0, 0) top left
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


        if args_dict["occupancy"] is True:
            self.create_occupancy_plot(ignore_full_frames = self.ignore_full_frames, batches_dict = self.batches_dict)
            import sys
            sys.exit('Exiting after creation of occupancy plot.')


    def connect(self):
        # before we connect the key press events, we check if refresh ran once
        # files are read, before we accept any input
        # we check 50 times if the number of files is larger than 0 (which
        # indicates that we have finished reading the files in the folder
        # at least once), and wait 100ms after each check.
        for _ in range(50):
            lock.acquire()
            nfiles = self.ns.nfiles
            empty_folder = self.ns.empty_folder
            lock.release()
            if nfiles > 0 or empty_folder == True:
                print('nfiles : ', nfiles)
                break
            else:
                print('waiting...')
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
            print('')
            print('keypress read:', c)
            print('going to next file #', self.i)
            self.i += 1
            lock.acquire()
            self.nfiles = self.ns.nfiles
            lock.release()
            if self.i < self.nfiles:
                self.work_on_existing_file(self.i)
            else:
                plt.close()
                print('reached last file in folder')
                self.disconnect()
        elif c == 'b':
            print('')
            print('keypress read:', c)
            print('going to previous file')
            #if self.i > 0:
            self.i -= 1
            print(self.i)
            self.work_on_existing_file()
            #else:
            #    self.work_on_file()
        elif c == 'e':
            # if e is typed, we jump to the end of the list and automatically
            # always get the last frame
            if self.ani_end_running == False:
                print('jumping to last file and start automatically refreshing')
                self.loop_work_on_file_end()
            else:
                # in the event this animation is running, we stop it
                self.stop_animation()
        elif c == 'l':
            # if l is typed, we start to automatically go through all files
            # in the filelist
            if self.ani_loop_running == False:
                print('jumping to last file and start automatically refreshing')
                self.loop_work_on_file()
            else:
                # stop the animation
                self.stop_animation()
        elif c == 'o':
            # if o is typed, we create an occupancy plot of the whole run
            print('creating occupancy plot...')
            self.create_occupancy_plot(ignore_full_frames = self.ignore_full_frames, batches_dict = self.batches_dict)
        elif c == 's':
            # if s is typed, we save the current figure
            print('saving figure...')
            self.save_figure(self.current_filename)
        elif c == 'h':
            # if h is typed, we create the pixel histogram for each chip
            print('creating pixel histogram...')
            self.create_pixel_histogram()


        elif c == 'q':
            print('')
            # call stop animation to stop any running animations,
            # before closing (makes sure there's no error on exit)
            wait_flag = self.stop_animation()
            if wait_flag is True:
                # we built in a wait of 2 seconds, to make sure the animation is actually
                # stopped before we quit
                time.sleep(2)
            print(c, 'was pressed. Exit program.')

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
            print('stopping the run animation...')
            # using the animation's _stop function (THANKS matplotlib source code...)
            # we can stop the animation properly
            self.ani_end_event_source._stop()
        if self.ani_loop_running == True:
            print('stopping the loop animation...')
            self.ani_loop_event_source._stop()
        # now wait for 0.5 seconds to make sure it stops
        if self.ani_end_running is True or self.ani_loop_running is True:
            # first set the running flags to False (if we end up in here
            # either one or both are NOT running anymore, so it is fine
            # if both are set to False)
            self.ani_end_running  = False
            self.ani_loop_running = False
            print('waiting for animations to stop...')
            wait_flag = True
            time.sleep(0.5)

        # return the wait_flag
        return wait_flag

    def work_on_existing_file(self, i = None):
        # this function only works on existing files, not trying to run over
        # all files

        if i is None:
            # if no i as argument given, use self.i
            # however, if this is called, after looping on last frame, self.i might
            # be larger than number of files in folder (if not every event exists)
            # thus check for this condition and set i back to indexing by number
            # of event in list, not event number
            lock.acquire()
            if self.i > self.ns.nfiles:
                # set self.i to nfiles - 1 (index of last element)
                self.i = self.ns.nfiles - 1
            lock.release()
            i = self.i

        elif i == -1:
            # in case we have called work_on_file_end(), we supply -1 to work_on_file.
            # this can be problematic, if we stop the function and want to go back
            # from this event, because -1 is relative to the number of current events. so
            # in case we have an ongoing run, this will change all the time, making it
            # impossible to go back (because more and more events are created)
            lock.acquire()
            self.i = self.ns.last_file
            i = self.i
            lock.release()
        else:
            # else, we just set self.i to i
            self.i = i

        if self.full_matrix_flag == False:
            lock.acquire()
            lst_of_files = sorted(list(self.ns.eventSet))
            lock.release()
            eventNumber = lst_of_files[i]
        else:
            eventNumber = i
        self.work_on_file(eventNumber)


    def work_on_file(self, i = None, online_flag = False):
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
            self.i = self.ns.last_file
            i = self.i
            print("Reading file %i" % self.i)
            lock.release()
        else:
            # else, we just set self.i to i
            # TODO: fix this!!!
            pass
            # self.i = i

        if online_flag is True:
            # in this case we're doing online viewing, i.e. jumping to last file in
            # folder. Need to obtain temps from yielder thread
            temps = self.ns.currentTemps
            #print('temps are ', temps)
        else:
            temps = self.ns.temps_dict

        # set the filenames to None, to easier check whether files dictionary was already
        # populated
        filename     = None
        filenameFadc = None

        # acquire lock and get necessary elements from namespace
        lock.acquire()
        if self.ns.nfiles > 0 and self.full_matrix_flag == False:
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
        elif self.ns.files > 0 and self.full_matrix_flag == True:
            # for the full matrix frames, simply get the current frame from the
            # list of filenames
            filename = self.ns.files[i]
            filenameFadc = None
        lock.release()
        # now plot septem
        if filename is not None:
            # before we plot the file, we set the file, we're going to plot as the
            # current file (to save the figure, if wanted)
            self.current_filename = self.filepath.split('/')[-1] + "_" + filename

            plot_file_general(self.filepath,
                              filename,
                              self.fig,
                              self.chip_subplots,
                              self.im_list,
                              self.cb,
                              temps,
                              full_matrix = self.full_matrix_flag,
                              downsample = self.downsample)
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
        self.work_on_file(-1, online_flag = True)


    def create_occupancy_plot(self,
                              batches_dict,
                              ignore_full_frames = False):
        # this function checks whether a dumped occupancy plot can be found,
        # if so this one is plotted, else create_occupancy_data is called

        # during the creation of the occupancy plots, we stop the refreshing
        # of the file list, since it is unneccessary
        lock.acquire()
        self.ns.doRefresh = False
        lock.release()

        if self.fadc_triggered_only is True:
            print('Using only FADC triggered events for occupancy.')
            event_set = self.ns.fadcSet
        else:
            event_set = self.ns.eventSet

        batches_flag = batches_dict["batches_flag"]
        nbatches     = batches_dict["nbatches"]


        if batches_flag is True and nbatches is None:
            # in this case calculate nbatches to ~1 batch per hour
            nbatches, scaling_factors = get_batch_num_hours_for_run(self.filepath, event_set)
            print('Calculated to use %i batches for occupancy plot.' % nbatches)
            print('Last batch scaled by %f.' % scaling_factors[-1])
        elif batches_flag is False:
            # set batches to 1
            nbatches = 1

        # to scale batches, which contain less than one hour up
        scaling_factors = np.ones(nbatches, dtype = int)

        for i in range(nbatches):
            # get current scaling
            scaling_f = scaling_factors[i]

            # get filename for batch
            occupancy_filename = create_occupancy_filename(self.filepath, i)
            data_dump_filename = create_pickle_filename(occupancy_filename,
                                                        self.fadc_triggered_only)

            file_exists = check_occupancy_dump_exist(data_dump_filename)
            if file_exists is True:
                print('%s file exists, loading data...' % data_dump_filename)
                # now load dump and plot

                chip_arrays, header_text = load_occupancy_dump(data_dump_filename)
            else:
                print('No data dump found for file %s, reading data...' % data_dump_filename)
                chip_arrays, header_text = self.create_occupancy_data_batch_mp(event_set,
                                                                               ignore_full_frames,
                #chip_arrays, header_text = self.create_occupancy_data_batch(ignore_full_frames,
                                                                               i,
                                                                               nbatches)
            # apply scaling factor and add line to header text
            chip_arrays *= scaling_f
            header_text  = add_line_to_header(header_text,
                                              "scaling_factor",
                                              scaling_f)



            plot_occupancy(occupancy_filename,
                           header_text,
                           self.septem,
                           self.fig,
                           self.chip_subplots,
                           self.im_list,
                           chip_arrays,
                           self.cb)
        # now we can activate the refreshing of the filelist again
        lock.acquire()
        self.ns.doRefresh = True
        lock.release()


    def create_occupancy_data_batch_mp(self, event_set, ignore_full_frames, iter_batch, nbatches):
        # multithreaded version of create_occupancy_data_batch
        # see single threaded version for documentation
        benchmarking_flag = False

        # first create the header text box for the occupancy plot
        # call with full eventSet, because function only needs to read any file,
        # which exists in this case
        header_text = get_occupancy_batch_header(self.ns.eventSet,
                                                 self.ns.nfiles,
                                                 self.filepath,
                                                 ignore_full_frames,
                                                 nbatches,
                                                 iter_batch)

        if nbatches > 1:
            # in this case we save all plots
            save_figures = True
            # need sorted list for iteration
            eventNumbers = sorted(list(event_set))
            nEventsPerBatch = np.ceil(len(eventNumbers) / nbatches)
        else:
            # for now we create a sorted list in every case (even if we use every
            # single event and shouldn't care about sorting)
            # Do it to index eventNumbers to get start and end index
            eventNumbers    = sorted(list(event_set))
            nEventsPerBatch = len(eventNumbers)
        # elif self.fadc_triggered_only is True:
        #     # in case we only consider FADC triggered events, we need a
        #     # sorted list, because otherwise we cannot index eventNumbers
        #     eventNumbers = sorted
        # else:
        #     # assign iterable object 'events' our normal eventSet
        #     # in case of nbatches == 1, we don't care about the order in which
        #     # we iterate over the batches
        #     eventNumbers    = event_set
        #     nEventsPerBatch = len(eventNumbers)

        # calculate starting point
        nStart = eventNumbers[iter_batch * nEventsPerBatch]
        nEnd   = eventNumbers[(iter_batch + 1) * nEventsPerBatch - 1]
        if benchmarking_flag is True:
            list_of_files = create_list_of_files_dumb(self.filepath)
        else:
            # create list of files
            list_of_files = create_list_of_files(nStart, nEnd, event_set, self.filepath)

        print('List of files has entries: start: %i end: %i total: %i' % (nStart, nEnd, len(list_of_files)))
        print('nEvents %i batches %i nEventsPerBatch %i' % (len(eventNumbers), nbatches, nEventsPerBatch))

        # create a list of numpy arrays. one array for each occupancy plot of each chip
        chip_arrays = np.zeros((self.septem.nChips, self.pix_dim, self.pix_dim))# for _ in xrange(self.septem.nChips)]

        # create namespace Create Occupancy_NameSpace
        co_ns = mp.Manager().Namespace()

        # doRead controls worker thread, which only reads files
        co_ns.doRead = True
        # finished reading will be set to True by the reader process, once it
        # has finished reading all files of list_of_files
        co_ns.finishedReading = False
        # do work controls worker thread, which creates septemClasses
        # from doReads data
        co_ns.doWork = True
        # analogoue to finishedReading
        co_ns.finishedWorking = False
        # main thread will work on the data delivered by doWork
        # batch size, which is read and put into queue
        co_ns.batch_size  = 100
        # max cached is the max number of cached batches, before a process will wait
        co_ns.max_cached = 5
        # sleeping time
        co_ns.sleeping_time = 0.1
        # long sleep time (before finishing threads, wait this long and check if queue
        # still empty)
        co_ns.long_sleep    = 1

        qRead = mp.Queue()
        qWork = mp.Queue()

        pRead = mp.Process(target = read_zsub_mp, args = (co_ns, list_of_files, qRead) )
        pWork = mp.Process(target = fill_classes_from_file_data_mp, args = (co_ns, qRead, qWork) )

        pRead.start()
        pWork.start()

        # create an empty list, which will store tuples of centroids including the energy
        # (from the number of pixels), as (x, y, Energy)
        centroid_list = []

        while co_ns.finishedReading is False or co_ns.finishedWorking is False or qWork.empty() is False:
            # TODO: think about setting doRead and doWork from here based on finished bools?
            # run over loop as long as there is still something to read and work on
            # if co_ns.finishedReading is True:
            #     # in this case stop reader process
            #     co_ns.doRead = False
            # if co_ns.finishedWorking is True:
            #     # in this case stop worker process.
            #     # NOTE: in theory this should never happen before reader is
            #     # finished! Due to while statement this should if statement
            #     # should never be true
            #     assert co_ns.finishedReading is False
            #     co_ns.doWork = False

            if qWork.empty() is False:
                ev_ch_list = qWork.get()
                for ev_ch in ev_ch_list:
                    evHeader, chpHeaders = ev_ch
                    # now go through each chip header and add data of frame
                    # to chip_arrays
                    for chpHeader in chpHeaders:
                        # get the data of the frame for this chip
                        chip_data = chpHeader.pixData
                        chip_num  = int(chpHeader.attr["chipNumber"])

                        # and now add chip data to chip_arrays if non empty
                        npix = np.size(chip_data)
                        if npix > 0 and ignore_full_frames is False:
                            # TODO: rewrite this and next statement with one
                            # function to be called
                            if self.calc_centroid_flag is True:
                                centroid = calc_centroid(chip_data)
                                x_mean, y_mean, energy = centroid
                                if chip_num == self.cb.chip:
                                    centroid_list.append(centroid)
                                chip_arrays[chip_num, y_mean, x_mean] += 1
                            else:
                                chip_arrays[chip_num, chip_data[:,1], chip_data[:,0]] += 1
                        elif npix > 0 and ignore_full_frames is True and npix < 4097:
                            # if ignore_full_frames is True we drop all events with more
                            # than 4096 pixels
                            if self.calc_centroid_flag is True:
                                centroid = calc_centroid(chip_data)
                                x_mean, y_mean, energy = centroid
                                if chip_num == self.cb.chip:
                                    centroid_list.append(centroid)
                                chip_arrays[chip_num, y_mean, x_mean] += 1
                            else:
                                chip_arrays[chip_num, chip_data[:,1], chip_data[:,0]] += 1

                            event_num = int(evHeader.attr["eventNumber"])
                            if event_num % 1000 == 0:
                                print(event_num, ' events done.')
            else:
                print('Main process: is sleeping...')
                time.sleep(co_ns.sleeping_time)

        # assert both queues really are empty and we didn't skip data
        assert qRead.empty() is True
        assert qWork.empty() is True

        # after having finished reading and working, stop both threads
        co_ns.doRead = False
        co_ns.doWork = False

        print('Joining all processes.')
        #pRead.join()
        #pWork.join()

        # set current file (to save the figure, if wanted)
        self.current_filename = create_occupancy_filename(self.filepath, iter_batch)

        # before the plot, dump the file to a cPickle
        print('dumping data of batch %i' % (iter_batch))
        dump_occupancy_data(self.current_filename,
                            chip_arrays,
                            header_text,
                            self.fadc_triggered_only)

        if self.calc_centroid_flag is True:
            write_centroids_to_file(self.filepath, centroid_list)

        print('finished batch #%i at event #%i' % (iter_batch, nEnd))

        return chip_arrays, header_text


    def create_occupancy_data_batch(self, event_set, ignore_full_frames, iter_batch, nbatches):
        """
        create an occupancy plot. count number of times each pixel was hit during the
        whole run

        TODO: think about another way to run over all files. NOT SURE, but it seems
        that creating all filenames from scratch (via the function from the event number)
        is (in this case) a waste of resources.
        Idea: create function, which does what create_filename_from_event_number does
        but returns list of filenames from eventSet

        bool ignore_full_frames : this flag controls whether we drop all events from
                                  the occupancy creation, which have more than 4096
                                  active pixels. In that case data is lost (only 4096)
                                  pixels fit in zero suppressed readout.
        int iter_batch          : this integer determines the batch we're going to calculate
                                  in this function call
        int nbatches            : total number of batches for call of create_occupancy_plot
        """

        # first create the header text box for the occupancy plot
        # call with full event set, since we only need to read any file
        header_text = get_occupancy_batch_header(self.ns.eventSet,
                                                 self.ns.nfiles,
                                                 self.filepath,
                                                 ignore_full_frames,
                                                 nbatches,
                                                 iter_batch)

        if nbatches > 1:
            # in this case we save all plots
            save_figures = True
            # need sorted list for iteration
            eventNumbers = sorted(list(event_set))
            nEventsPerBatch = np.ceil(len(eventNumbers) / nbatches)
        else:
            # assign iterable object 'events' our normal eventSet
            # in case of nbatches == 1, we don't care about the order in which
            # we iterate over the batches
            eventNumbers    = event_set
            nEventsPerBatch = len(eventNumbers)

        # create a list of numpy arrays. one array for each occupancy plot of each chip
        chip_arrays = np.zeros((self.septem.nChips, self.pix_dim, self.pix_dim))# for _ in xrange(self.septem.nChips)]

        # calculate starting point for loop
        nStart = iter_batch * nEventsPerBatch

        # we run over all files of the event number set, i.e. all files of run
        for eventNumber in eventNumbers:
            if eventNumber < nStart:
                # continue if not yet reached start of dataset
                continue
            elif eventNumber == nStart:
                print("starting batch #%i at event #%i" % (iter_batch, nStart))
            # first create the filename
            filename = create_filename_from_event_number(event_set,
                                                         eventNumber,
                                                         self.ns.nfiles,
                                                         fadcFlag = False)
            # we create the event and chip header object
            filename = os.path.join(self.filepath, filename)

            evHeader, chpHeaderList = read_zero_suppressed_data_file(filename)
            # now go through each chip header and add data of frame
            # to chip_arrays
            for chpHeader in chpHeaderList:
                # get the data of the frame for this chip
                chip_data = chpHeader.pixData
                chip_num  = int(chpHeader.attr["chipNumber"])
                # and now add chip data to chip_arrays if non empty
                npix = np.size(chip_data)
                if npix > 0 and ignore_full_frames is False:
                    chip_arrays[chip_num - 1, chip_data[:,1], chip_data[:,0]] += 1
                elif npix > 0 and ignore_full_frames is True and npix < 4097:
                    # if ignore_full_frames is True we drop all events with more than 4096 pixels
                    chip_arrays[chip_num - 1, chip_data[:,1], chip_data[:,0]] += 1

            event_num = int(evHeader.attr["eventNumber"])
            if event_num % 1000 == 0:
                print(event_num, ' events done.')

            # now check whether we dump data and return
            if (eventNumber == max(eventNumbers) or
                eventNumber == (iter_batch + 1) * nEventsPerBatch - 1 and
                eventNumber > 0):
                # now plot the occupancy of the thus generated data
                # now set the file we're going to plot as the
                # current file (to save the figure, if wanted)
                self.current_filename = create_occupancy_filename(self.filepath, iter_batch)

                # before the plot, dump the file to a cPickle
                dump_occupancy_data(self.current_filename, chip_arrays, header_text)

                print('finished batch #%i at event #%i' % (iter_batch, eventNumber))

                return chip_arrays, header_text



    def save_figure(self, output):
        # this function simply saves the currently shown plot as a png in
        # the current working directory
        output = output.rstrip(".txt") + ".png"
        plt.savefig(output)

    def create_pixel_histogram(self):
        # create the pixel histogram for all chips of the current run

        # define a list of lists for the number of hits for each chip
        nHitsList = [ [] for _ in range(self.septem.nChips)]

        for eventNumber in self.ns.eventSet:
            # first create the filename to supply it to read function
            filename     = create_filename_from_event_number(self.ns.eventSet,
                                                             eventNumber,
                                                             self.ns.nfiles,
                                                             fadcFlag = False)
            # we create the event and chip header object
            filename = os.path.join(self.filepath, filename)
            evHeader, chpHeaderList = read_zero_suppressed_data_file(filename)
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
                print(event_num, ' events done.')

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
        #grid = gridspec.GridSpecFromSubplotSpec(3, 1, [row1, row2, row3])
        #grid = gridspec.GridSpecFromSubplotSpec(3, 1, subplot_spec = row1)#[row1, row2, row3])
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

def setup_temp_watcher(filepath, ns):
    # sets up the temparature file watcher. Can easily be extended
    # to check every file in run folder

    # then start watcher daemon, which watches over filepath and checks
    # whether files change / are added
    #wm_run   = pyinotify.WatchManager()
    wm_temp  = pyinotify.WatchManager()
    mask = pyinotify.IN_DELETE | pyinotify.IN_CREATE | pyinotify.IN_MODIFY

    temp_log_file = get_temp_filename(filepath)
    if temp_log_file != "":
        print("Temperature log file found at: %s" % temp_log_file)

        temp_handler = TempHandler(ns, lock)
        print('nsns', ns)
        #temp_handler.my_init(ns, maxqsize = 0)
        temp_notifier = pyinotify.ThreadedNotifier(wm_temp, temp_handler)
        temp_notifier.start()
        wdd_temps = wm_temp.add_watch(temp_log_file, mask, rec=False)
    else:
        temp_handler = None
        temp_notifier = None
        print("Warning: could not locate a temperature log file.")

    # run_handler  = EventsHandler(ns, lock)
    # run_notifier = pyinotify.ThreadedNotifier(wm_run, run_handler)
    # run_notifier.start()
    # wdd_run   = wm_run.add_watch(filepath, mask, rec=False)
    run_notifier = None
    run_handler = None

    #notifier.stop()

    return (temp_notifier, temp_handler), (run_notifier, run_handler)

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

    # args dict contains additional arguments to be given to the WorkOnFile object
    args_dict = {}

    parser = argparse.ArgumentParser(description = 'Python Septemboard TOS event display')
    parser.add_argument('run_folder',
                        help = "The folder, which contains the run to be plotted from")
    parser.add_argument('-o', '--occupancy', '--occupancy_flag',
                        action = 'store_true',
                        help = "Program will only create occupancy plot of given run folder")
    parser.add_argument('--fadc_triggered_only', action = 'store_true',
                        help = "Defines to only use FADC triggered events for occupancy")
    parser.add_argument('--calc_centroids', action = 'store_true',
                        help = """If set will calculate centroids of clusters during occupancy
                        creation and dumps to file centroid_hits.dat (for center chip)""")
    parser.add_argument('--cb_flag', default = 1, type = int,
                        help = """Sets color bar flag to absolute (1) or percentile of values (0).
                        In case of relative values, it is the percentile for each individual
                        chip, i.e. chips will have a different scale!""" )
    parser.add_argument('--cb_value', default = 80, type = int,
                        help = """Sets color bar max value to absolute (if --cb_flag true)
                        or relative value given""")
    parser.add_argument('--cb_chip', default = 3, type = int,
                        help = "Sets chip from which to determine color bar")
    parser.add_argument('--colormap', default = "viridis", type = str,
                        help = """Sets the color map to the given map""" )
    parser.add_argument('--downsample', default = 1, type = int,
                        help = """Allows to downsample the frames by the given factor. E.g. 2 results
                        in 128x128 frames.
                        WARNING: may result in undefined behaviour if used together with --full_matrix argument!""")
    parser.add_argument('--single_chip', action = 'store_true', dest = "single_chip_flag",
                        help = "Use single chip instead of Septemboard")
    parser.add_argument('--full_matrix', action = 'store_true', dest = "full_matrix_flag",
                        help = "Toggle to expect full matrix readout frames, instead of zero suppressed.")
    parser.add_argument('--start_iter', default = 0, type = int,
                        help = "Start at event of this index in run folder")
    parser.add_argument('--ignore_full_frames', action = 'store_true',
                        help = "Ignore completely full frames in occupancy plot")
    parser.add_argument('--batches_flag', action = "store_true",
                        help = "Use batches in occupancy plot.")
    parser.add_argument('--batches', default = None, type = int, dest = "nbatches",
                        help = """Use this many batches for occupancy plot. If 0 and
                        batches_flag == True, calc batches to be ~1 hour.""")

    args_dict = vars(parser.parse_args())
    folder = os.path.abspath(args_dict["run_folder"])

    print(args_dict)

    # create a custom colorbar object to store colorbar related properties;
    # initialize with values either as defaults from above or command line argument
    cb = customColorbar(args_dict["cb_flag"], args_dict["cb_value"], args_dict["cb_chip"])

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
    chip_subplots = create_chip_axes(sep, fig, args_dict["single_chip_flag"])


    # and create the namespace, which will be given to both threads, so they can access
    # the same resources
    ns                 = mp.Manager().Namespace()
    # define a flag for the refreshing thread, so it knows when to stop
    ns.doRefresh       = True
    # define the ordered dictionary, in which we store the filenames
    # ns.filelist        = filelistManager.OrderedDict()
    # individual lists, which are generated from the dictionary
    # ns.filelistEvents  = []
    # ns.filelistFadc    = []
    # these two sets are the successors to the previously used list of
    # files. We now only store the event numbers and create the corresponding
    # filenames on the fly from the path
    ns.eventSet        = set()
    ns.fadcSet         = set()
    ns.nfiles          = 0
    if args_dict["full_matrix_flag"] == True:
        # set the namespace of the refresh process such that it expects
        # full matrix frames, in effect relax the requirements on the
        # filenames and instead of reading sets of event numbers, get a
        # list of files, as back in the past
        ns.full_matrix_flag = True
        ns.files = []
    else:
        ns.full_matrix_flag = False
        ns.files = None
    # and the interval, in which the thread refreshes the filelist
    ns.refreshInterval = 0.0001
    print(ns)

    # temparature readout related
    ns.currentTemps = None
    ns.temps_dict   = None


    try:
        (temp_notifier, temp_watcher), (run_notifier, run_handler) = setup_temp_watcher(folder, ns)
        #temp_watcher = None
        # we start the file refreshing (second) thread first, because we only
        # want to accept key inputs, after the files have been read a single
        # time.
        # and the second thread, which performs the refreshing
        p2 = mp.Process(target = refresh, args = (ns, folder, temp_watcher))
        p2.start()


        # now create the main thread, which starts the plotting
        files = WorkOnFile(folder, fig, sep, chip_subplots, fadcPlot, ns, cb, args_dict)
        files.connect()

        plt.show()
        p2.join()
    except KeyboardInterrupt:
        if temp_notifier is not None:
            temp_notifier.stop()
        #run_notifier.stop()
        pass
    else:
        if temp_notifier is not None:
            temp_notifier.stop()
        #run_notifier.stop()
        pass

if __name__=="__main__":
    import sys
    main(sys.argv[1:])
