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

def calc_length_of_run(run_folder, eventSet):
    """
       This function calculates the total length of a run based on the
       timestamps in the data files and returns the time rounded to hours.
       string run_folder: a string containing the folder name
       set eventSet:      a set containing all events in the folder
    """

    # get first and last file in set
    eventsSorted = sorted(eventSet)
    nfiles = len(eventsSorted)
    first = run_folder + create_filename_from_event_number(eventSet, eventsSorted[0], nfiles, False)
    last  = run_folder + create_filename_from_event_number(eventSet, eventsSorted[-1], nfiles, False)
    
    
    evHeaderFirst, chpHeadersFirst = read_zero_suppressed_data_file(first, True)
    evHeaderLast, chpHeadersLast   = read_zero_suppressed_data_file(last, True)

    date_syntax = '%Y-%m-%d.%H:%M:%S'
    dateFirst = datetime.strptime(evHeaderFirst.attr["dateTime"], date_syntax)
    dateLast  = datetime.strptime(evHeaderLast.attr["dateTime"], date_syntax)

    diff = dateLast - dateFirst
    hours = np.round(diff.total_seconds() / 3600.)

    return int(hours)
    
    
    
