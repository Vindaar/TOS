# functions not related to files, plotting or classes
from septemFiles import read_zero_suppressed_data_file, create_filename_from_event_number
from datetime import datetime
import numpy as np

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

def get_batch_num_hours_for_run(run_folder, eventSet):
    """ 
       This function returns the number of batches of 1 hour length
       for a given run. Calls calc_length_of_run internally
       string run_folder: a string containing the folder name
       set eventSet:      a set containing all events in the folder
    """

    # get first and last file in set
    eventsSorted = sorted(eventSet)
    nfiles = len(eventsSorted)
    first = run_folder + create_filename_from_event_number(eventSet, eventsSorted[0], nfiles, False)
    last  = run_folder + create_filename_from_event_number(eventSet, eventsSorted[-1], nfiles, False)
    
    nbatches = calc_length_of_run(first, last)

    return nbatches

def calc_length_of_run(first, last):
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
    
def get_iter_batch_from_header_text(header_text):
    # this function returns the iter_batch number from a given
    # header text
    header_els = header_text.split('\n')
    batch_lst  = [b for b in header_els if "iter_batch" in b]
    batch_num  = int(batch_lst[0].split(':')[-1])
    return batch_num
