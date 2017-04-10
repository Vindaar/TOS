# functions not related to files, plotting or classes
from septemFiles import read_zero_suppressed_data_file, create_filename_from_event_number, work_on_read_data
from septemClasses import eventHeader, chipHeaderData
from datetime import datetime
import numpy as np
from time import sleep

def add_line_to_header(header, str_to_add, value):
    # adds an additional line to a header of a plot
    string = ("\n%s : " % str_to_add).ljust(25)
    string += str(value)
    header += string
    return header

def get_TOS_date_syntax():
    # returns the syntax used in TOS for all dates (incl time)
    # as a string, which can be used for python's datetime 
    # strptime function
    return '%Y-%m-%d.%H:%M:%S'


# DEPRECATED
# def calc_scaling_factor(batches, i):
#     # calculates the scaling factor for iteration i 
#     # for batches
#     val = batches - i
#     if val < 0:
#         # return inverse of 
#         return 1 / (b - i + 1)
#     else:
#         return 1

def get_scaling_factors(batches):
    # creates a list for the scaling factors to be applied to each
    # batch for the occupancy plots. In case of values smaller 0
    floor = int(np.floor(batches))
    rest  = batches - floor
    
    scaling = 1. / rest
    factors = [1 for _ in xrange(floor)]
    factors.append(scaling)

    return factors

def get_batch_num_hours_for_run(run_folder, eventSet, total_flag = False):
    """ 
       This function returns the number of batches of 1 hour length
       for a given run. Calls calc_length_of_run internally
       string run_folder: a string containing the folder name
       set eventSet:      a set containing all events in the folder
       bool total_flag:   determines whether we use beginning and end of run
                          (if True) or shutter opening time and number of events
                          (if False) for the calculation
    """

    # get first and last file in set
    eventsSorted = sorted(eventSet)
    nfiles = len(eventsSorted)
    first = run_folder + create_filename_from_event_number(eventSet, eventsSorted[0], nfiles, False)
    last  = run_folder + create_filename_from_event_number(eventSet, eventsSorted[-1], nfiles, False)
    
    if total_flag is True:
        nbatches = calc_total_length_of_run(first, last)
        return nbatches
    else:
        nbatches = calc_active_length_of_run(first, nfiles)
        # deal with the case of less than one hour of run time (needed in case
        # of very short shutter times)
        scaling_factors = get_scaling_factors(nbatches)
        # and now take ceiling of batches (round up)
        nbatches = int(np.ceil(nbatches))

        return nbatches, scaling_factors

    # should never happen
    return None

def calc_total_length_of_run(first, last):
    """
       This function calculates the total length of a run based on the
       timestamps in the data files and returns the time rounded to hours.
       string first: filename of first event in run
       string last:  filename of last event in run
    """
    
    evHeaderFirst, chpHeadersFirst = read_zero_suppressed_data_file(first, True)
    evHeaderLast, chpHeadersLast   = read_zero_suppressed_data_file(last, True)

    date_syntax = get_TOS_date_syntax()
    dateFirst = datetime.strptime(evHeaderFirst.attr["dateTime"], date_syntax)
    dateLast  = datetime.strptime(evHeaderLast.attr["dateTime"], date_syntax)

    diff = dateLast - dateFirst
    hours = np.round(diff.total_seconds() / 3600.)

    return int(hours)

def calc_active_length_of_run(event_file, nfiles):
    """ 
       This function calculates the total time the detector was actively taking
       data in a given run. In contrast to calc_total_length_of_run, which 
       uses the timestamps in the first and last frame as a reference.
       This function is especially useful in case of shutter times smaller
       than 1 seconds, because the dead time increases significantly.
       string event_file: filename of any event in a given run
       int nfiles: the number of total events in the run
    
       returns: total hours of active time in run
    """

    evHeader, chpHeaders = read_zero_suppressed_data_file(event_file, True)

    shutter_length = calc_shutter_length_from_event_header(evHeader)
    active_length  = shutter_length * nfiles
    
    # convert active_length (given in microseconds) to hours
    active_length = float(active_length / (1e6 * 3600))
    
    return active_length
    
def get_iter_batch_from_header_text(header_text):
    # this function returns the iter_batch number from a given
    # header text
    header_els = header_text.split('\n')
    batch_lst  = [b for b in header_els if "iter_batch" in b]
    batch_num  = int(batch_lst[0].split(':')[-1])
    return batch_num


def get_occupancy_batch_header(eventSet, nfiles, filepath, ignore_full_frames, nbatches, iter_batch):
    """
    Function which returns a header for an occupancy batch. Prints arguments based on 
    first file in 'filepath' folder
    """
    if len(eventSet) > 0:
        # need to read one of the files to get the header. Choose first file, thus
        filename     = create_filename_from_event_number(eventSet,
                                                         0,
                                                         nfiles,
                                                         fadcFlag = False)
        evHeader, chpHeaderList = read_zero_suppressed_data_file(filepath + filename)
        header_text = evHeader.get_run_header_text()
    else:
        print 'filelist not filled yet or empty; returning.'
        return
    

    header_text = add_line_to_header(header_text, 
                                     "# events",
                                     nfiles)
    header_text = add_line_to_header(header_text, 
                                     "ignore_full_frames",
                                     ignore_full_frames)
    header_text = add_line_to_header(header_text, 
                                     "n_batches",
                                     nbatches)
    header_text = add_line_to_header(header_text, 
                                     "iter_batch",
                                     iter_batch)
    
    return header_text


def fill_classes_from_file_data_mp(co_ns, qRead, qWork):
    """
    this multiprocessing function is used to create objects from septemClasses
    and fill them with contents read from a separate thread using the
    read_zsub_mp function

    Namespace co_ns : a namespace containing the multiprocessing parameters
    Queue qRead : the queue containing the data, which is read in separate thread
    Queue qWork : the queue into which the output is put
    """
    
    while co_ns.doWork is True:
        if qRead.empty() is False:
            print('Worker process: retrieving from queue...')
            data  = qRead.get()
            #ev_ch = [ work_on_read_data(el) for el in data ]
            print('Worker process: data size %i' % len(data))
            ev_ch = []
            for i, el in enumerate(data):
                if i % 100 == 0:
                    print('Finshed working on %i files' % i)
                ev_ch.append(work_on_read_data(el))
            qWork.put(ev_ch)
        elif qRead.empty() is True and co_ns.finishedReading is False:
            # in this case sleep, because we're waiting for new elements in queue
            print('Worker process: is sleeping... waiting for reader process')
            sleep(co_ns.sleeping_time)
        elif qRead.empty() is True and co_ns.finishedReading is True:
            # perform a long sleep to make sure put was flushed into queue
            # before stopping the thread.
            print('Worker process: sleeping...')
            sleep(co_ns.long_sleep)
            if qRead.empty() is True:
                print('Worker process: finished all work.')
                co_ns.finishedWorking = True
                co_ns.doWork = False
            else:
                # simply pass if Queue suddenly not empty again
                print('Worker process: passing...')
                a = 1
                pass

def get_shutter_mode_dict():
    # this function returns the shutter mode dictionary, which 
    # creates correspondence between shutter modes given as string
    # and the mode given as an exponent for the calculation
    
    shutter_mode_dict = { "standard" : 0,
                          "std"      : 0,
                          "0"        : 0,
                          "long"     : 1,
                          "l"        : 1,
                          "1"        : 1,
                          "verylong" : 2,
                          "vl"       : 2,
                          "2"        : 2 }

    return shutter_mode_dict


def calc_shutter_length_from_event_header(evHeader):
    # this function calculates the shutter opening time in microseconds
    # calls calc_shutter_length internally
    
    shutterTime = int(evHeader.attr["shutterTime"])
    shutterMode = evHeader.attr["shutterMode"]

    shutterModeDict = get_shutter_mode_dict()
    shutterMode = shutterModeDict[shutterMode]

    length = calc_shutter_length(shutterTime, shutterMode)

    return length
    

def calc_shutter_length(time, mode):
    # this function calculates the shutter length based on the time
    # and shutter mode
    # return value given in microseconds
    
    length = 256 ** mode * 46 * time / 40.
    
    return length


    
