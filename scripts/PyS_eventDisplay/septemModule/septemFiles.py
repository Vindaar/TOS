# this file contains the functions related to file handling in
# regards to septem event display

import os
import scandir
import numpy as np
from septemClasses import eventHeader, chipHeaderData
#from septemMisc import convert_datetime_str_to_datetime
import collections
import cPickle


################################################################################
############################## Reading files ###################################
################################################################################

def read_zsub_mp(co_ns, list_of_files, qRead):
    """
    this function is called from a worker process, which reads 
    all files of the list of files and puts it into the queue qRead
    """
    # TODO: implement max cached batches 
    
    while co_ns.doRead is True:
        for i, f in enumerate(list_of_files):
            if i % 500 == 0:
                print('Reader process: finished %i events.' % i)
            if i % co_ns.batch_size == 0 and i == 0:
                # start reading first batch, create empty data list
                data = []
            elif i % co_ns.batch_size == 0:
                # in this case starting second batch
                # put data into queue and create empty list
                print('Reader process: finished %i events. Putting into queue.' % i)
                qRead.put(data)
                data = []
            # append content of file to data list
            data.append(open(f, 'r').readlines())

        # after having finished reading all files of list_of_files
        # put last data list into queue
        qRead.put(data)
        # set finishedReading to True
        co_ns.finishedReading = True
        co_ns.doRead = False
        print('Reader process: finished reading all files')


def read_multiple_zsup_data_files(list_of_files):
    # function to be called by a worker thread, which simply reads
    # a batch of files and returns a list of read data
    # internally calls read_zero_suppressed_data_file
    
    data = []
    for f in list_of_files:
        data.append(read_zero_suppressed_data_file(f, False))

    return data

def read_zero_suppressed_data_file(filepath, header_only = False):#, out_q1):
    # this function reads a single file, created by the septem board
    # with the zero suppressed readout
    # the optional flag header_only can be used to only read the header.
    # in this case the pix data array will be set to []
    if header_only == True:
        # in case we only want the header, give the sizehint equal to 1
        # for very large files, not the whole file will be read, speeding up the
        # process
        f = open(filepath, 'r').readlines(1)
    else:
        # else read whole file (until EOF)
        f = open(filepath, 'r').readlines()

    data = work_on_read_data(f, filepath, header_only)
    return data

def get_temp_filename():
    return "temp_log.txt"

def read_temparature_log(filepath):
    """ 
    reads a temparature log file created during a run from the MCP2210 
    returns a dictionary of
    {datetime object : (IMB_temp, septem_temp)}
    """
    filename = get_temp_filename()
    with open(filepath, 'r') as f:
        lines = f.readlines()

    temp_dict = {}
    for line in lines:
        if "#" not in line:
            line = line.split()
            imb, septem, date = line
            dtime = convert_datetime_str_to_datetime(date)
            temp_dict[dtime] = (float(imb), float(septem))

    return temp_dict


################################################################################
############################## reading helper functions ########################
################################################################################


def work_on_read_data(data, filepath = None, header_only = False):
    # the function which performs the splitting of the data
    # read by read_zero_suppressed_data_file
    # the zero suppressed files are setup as follows:
    # double hash ' ## ' indicates the file header (information about run and event)
    # single hash ' # '  indicates the header for a single chip
    evHeader  = eventHeader(filepath)
    chpHeaderList = []
    for line in data:
        line = line.strip()
        if '##' in line:
            # in this case read the file header
            evHeader.set_attribute(line)
        elif '#' in line:
            if "chipNumber" in line:
                # if chip Number is  in the line, this means that the last
                # chip is finished. therefore convert the listOfPixels to a
                # numpy array
                if len(chpHeaderList) > 0:
                    chpHeaderList[-1].convert_list_to_array()
                    
                chpHeader = chipHeaderData()
                chpHeaderList.append(chpHeader)
            # in this case read the chip header
            chpHeaderList[-1].set_attribute(line)
        elif header_only == False:
            # in this case we're reading actual pixel data
            chpHeaderList[-1].add_pixel(line)
        else:
            # else (header_only is True), we break from the loop
            break
    
    # after we're done reading the last file, we still need to convert the listOfPixels
    # of the last element in the chpHeaderList to an array:
    if len(chpHeaderList) > 0:
        chpHeaderList[-1].convert_list_to_array()
    return [evHeader, chpHeaderList]


def create_files_from_path(folder):
    # this function removes any files which are not chip events in a folder
    files = os.listdir(folder)
    # filter files by data in file and no fadc flag in filename
    n = 0
    eventFiles = []
    
    for el in files:
        if "data" in el and "fadc" not in el:
            n += 1
            eventFiles.append(el)

    eventFiles.sort()

    return eventFiles


def create_files_from_path_fadc(folder):
    # this function removes any non fadc files which are in a folder
    files = os.listdir(folder)
    # filter files by -fadc flag in filename
    print len(files)
    n = 0
    files_fadc = []
    for el in files:
        if "fadc" in el:
            # print el
            n += 1
            files_fadc.append(el)

    files_fadc.sort()

    return files_fadc


def create_files_from_path_combined(folder, eventSet, fadcSet, test = False):
    # this function removes any files which are not chip events
    # or FADC events in a folder
    # functions returns list of two lists.
    # [septemFiles, fadcFiles]
    #files = scandir.walk(folder)#os.listdir(folder)
    #print files
    # filter files by data in file and no fadc flag in filename
    n = 0
    eventFiles = []
    files_fadc = []

    
    # we store the files for events and FADC events in a dictionary, where
    # the key is the filename of the event, and the value is the FADC filename
    filesDict = {}
    for path, folder, files in scandir.walk(folder):#files:
        for el in files:
            if "data" in el and "fadc" not in el:
                el = int(el.split('/')[-1].lstrip('data').rstrip('.txt'))
                if el not in eventSet:
                    eventSet.add(el)
            elif "fadc" in el:
                el = int(el.split('/')[-1].lstrip('data').rstrip('.txt-fadc'))
                if el not in fadcSet:
                    fadcSet.add(el)
        
    if test == True:
        filesDict = collections.OrderedDict( sorted( filesDict.items() ) )

    #print filesDict
    #print filesDict.keys()[205]

    #import sys
    #sys.exit()
    
    if test == False:
    #     return [eventFiles, files_fadc]
        return [eventSet, fadcSet]
    else:
        return filesDict
    

def create_filename_from_event_number(event_num_set, event_number, nfiles, fadcFlag):
    # this function is used to create a full filename from a filepath,
    # given an event number and a set (either the event set or the fadc set)
    # and returns the full filename
    # input:
    # event_num_set: a set of numbers corresponding to the events, which exist
    # event_number:  the wanted event for which to create the filename (can be negative,
    #                see the first comments below)
    # nFiles:        the total number of events, which exist. This has to be given,
    #                because if we only create the filename for FADC files, we need to
    #                know the total number (which we then cannot deduce from the number
    #                of elements in the set)
    # fadcFlag:      controls whether we create an event filename or an FADC filename

    # first check, whether event_number is a negative number. In this case, we
    # want to subtract this from nFiles (to iterate from the back)
    if event_number < 0:
        # since event_number is negative, simply add the numbers
        event_number = nfiles + event_number

        # check whether event_number in event_num_set:
    if event_number in event_num_set:
        # if it is in the set, create the filename
        # differentiate between fadcFlag
        if fadcFlag is False:
            filename = "data" + str(event_number).zfill(6) + '.txt'
        else:
            filename = "data" + str(event_number).zfill(6) + '.txt-fadc'
        # and return
        return filename
    else:
        # if it's not in set, return None
        return None

def get_event_number_from_filename(filename):
    # this function is the inverse ov create_filename_from_event_number. It returns
    # the event number, which is associated to the file filename
    basename = os.path.basename(filename)
    if '-fadc' not in basename:
        num = basename.lstrip('data').rstrip('.txt')
    else:
        num = basename.lstrip('data').rstrip('.txt-fadc')
    return int(num)
    
def create_occupancy_filename(filepath, iBatch):
    # this function creates a correct filename for an occupancy plot
    folder_name = os.path.basename(os.path.dirname(filepath))
    name = ("occupancy_batch_%s_" % str(iBatch).zfill(3)) + folder_name
    out_path = os.path.join("out/", name)
    return out_path

def create_pickle_filename(filename):
    foldername = os.path.dirname(filename)
    basename   = os.path.basename(filename)
    new_fname  = "cPickle_" + basename + ".dat"
    pickle_filename = os.path.join(foldername, new_fname)
    return pickle_filename

def dump_occupancy_data(filename, data, header_text):
    # this function cPickles the data needed for an occupancy plot and 
    # dumps it
    pickle_filename = create_pickle_filename(filename)
    data_dump = open(pickle_filename, 'wb')
    
    # create a dictionary to store the data
    data_dict = {"chip_arrays"  : data,
                 "header_text" : header_text}
                    
    cPickle.dump(data_dict, data_dump, -1)
    data_dump.close()

def load_occupancy_dump(filename):
    # this function loads a data dump created by dump_occupancy_data()
    data_dump = open(filename, 'r')
    data_dict = cPickle.load(data_dump)
    data_dump.close()
    
    data         = data_dict["chip_arrays"]
    header_text  = data_dict["header_text"]

    return data, header_text

def check_occupancy_dump_exist(filename):
    # checks whether occupancy dump exists
    basename = os.path.basename(filename)

    outfolder = os.listdir('out/')
    
    if basename in outfolder:
        return True
    else:
        return False


def create_list_of_inodes(filepath):

    inodes = []
    for el in scandir.scandir(path=filepath):
        name = el.name
        if "data" in name and "fadc" not in name:
            inode = el.inode()
            inodes.append(inode)
    return inodes

def create_list_of_files(nStart, nEnd, eventSet, filepath):
    """
    This function creates a list of files from start event nStart to end 
    event nEnd using eventSet and prepending the filepath.
    """
    list_of_files = []
    print('start creating files')
    print(type(eventSet))
    for event in eventSet:
        if event < nStart:
            continue
        elif event >= nEnd:
            break
        filename = create_filename_from_event_number(eventSet,
                                                     event,
                                                     len(eventSet),
                                                     fadcFlag = False)
        list_of_files.append(os.path.join(filepath, filename))

    print('done creating files')
    # given list of files get inodes and sort by them
    # TODO: write support for nStart and nEnd using inodes function!
    inodes_2 = create_list_of_inodes(filepath)
    #inodes = [os.stat(el).st_ino for el in list_of_files]
    # TODO FINIISNFINDISNDIANSDIX
    print('done getting inode'))
    fi = [x for (y, x) in sorted(zip(inodes_2, list_of_files))]

    return fi

def create_list_of_files_dumb(filepath):
    """
    The function returns the list of files returned by os.listdir
    """

    files = os.listdir(filepath)
    files = [os.path.join(filepath, el) for el in files if "data" in el and ".txt" in el]

    inodes = [os.stat(el).st_ino for el in files]
    fi = [x for (y, x) in sorted(zip(inodes, files))]
    
    return fi
    
