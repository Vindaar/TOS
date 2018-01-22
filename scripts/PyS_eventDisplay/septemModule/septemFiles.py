# this file contains the functions related to file handling in
# regards to septem event display

import os
from os.path import expanduser
import scandir
import numpy as np
import collections
import cPickle

import septemClasses
import septemMisc


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

def read_full_matrix_data_file(filepath):
    # this function reads a full matrix data file and returns it as
    # a numpy array in addition to an event header, which only contains
    # the filename as well as the full_matrix flag
    # inputs:
    #    filepath = path to the full matrix file to read
    # outputs:
    #    evHeader = dictionary containing `full_matrix_flag` as well as
    #               filename of event
    #    data = numpy array of 256 x 256 frame array
    f = open(filepath, 'r').readlines()
    evHeader = septemClasses.eventHeader(filepath)
    evHeader.attr["full_matrix_flag"] = True
    evHeader.attr["filepath"] = filepath

    data = np.zeros((256, 256))
    for i, line in enumerate(f):
        # parse each line as one row of data
        line = line.split()
        data[i] = [float(x) for x in line]

    return evHeader, data

def get_temp_filename(filepath):
    # this function returns the filename (incl. full path)
    # to the temperature log, which is currently in use.
    # In a normal run, the temperature is logged in the run
    # folder itself under the name temp_log.txt.
    # however, in case of using the FADC, it is instead
    # written to $TOS/log/temp_log.txt, since in this case
    # the temp log daemon is running in the background,
    # independent of being in a run
    basename = "temp_log.txt"
    # join given folder and basename
    filename = os.path.join(filepath, basename)
    # check if file exists
    in_run_folder = os.path.isfile(filename)
    if in_run_folder is True:
        return filename
    else:
        # else we check the log folder. expanduser to get
        # home directory of this user. We assume that TOS
        # is located in the home directory
        dirname = os.path.join(expanduser("~"), "TOS/log")
        filename = os.path.join(dirname, basename)
        in_log_folder = os.path.isfile(filename)
        if in_log_folder is True:
            return filename
        else:
            # in this case we could not find a temperature log file
            return ""

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
            dtime = septemMisc.convert_datetime_str_to_datetime(date)
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
    evHeader  = septemClasses.eventHeader(filepath)
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
                    
                chpHeader = septemClasses.chipHeaderData()
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

    # finally add a full_matrix flag to the event header, specifying that these
    # files are zero suppressed
    evHeader.attr["full_matrix_flag"] = False
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


def create_files_from_path_combined(folder, eventSet, fadcSet, full_matrix = False):
    # this function removes any files which are not chip events
    # or FADC events in a folder
    # functions returns list of two lists.
    # [septemFiles, fadcFiles]
    #files = scandir.walk(folder)#os.listdir(folder)
    #print files
    # filter files by data in file and no fadc flag in filename
    n = 0
    eventFiles = []

    # run over all files in the folder and add the FADC and event files,
    # if they meet our requirements for the names.
    # zero suppressed:
    #     - extract event numbers for each type and add to set
    # full matrix:
    #     - append full filename if it contains data or matrix
    for path, folder, files in scandir.walk(folder):
        if full_matrix == False:
            # zero suppressed frames
            for el in files:
                if "data" in el and "fadc" not in el:
                    el = int(el.split('/')[-1].lstrip('data').rstrip('.txt'))
                    if el not in eventSet:
                        eventSet.add(el)
                elif "fadc" in el:
                    el = int(el.split('/')[-1].lstrip('data').rstrip('.txt-fadc'))
                    if el not in fadcSet:
                        fadcSet.add(el)
        else:
            # Full matrix frames
            for el in files:
                if "data" in el or "matrix" in el:
                    eventFiles.append(el)
                elif "fadc" in el:
                    el = int(el.split('/')[-1].lstrip('data').rstrip('.txt-fadc'))
                    if el not in fadcSet:
                        fadcSet.add(el)                    
        
    if full_matrix == False:
        return [eventSet, fadcSet]
    else:
        return [sorted(eventFiles), fadcSet]


def build_filename_string_from_event_number(number, fadcFlag = False):
    # this function builds the string of the filename for a
    # given event number. Function is completely dumb
    if fadcFlag is False:
        filename = "data" + str(number).zfill(6) + '.txt'
    else:
        filename = "data" + str(number).zfill(6) + '.txt-fadc'

    return filename
    

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
        # and return
        filename = build_filename_string_from_event_number(event_number, fadcFlag)
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
    # NOTE: previously it said os.path.basename(os.path.dirname(filepath))
    # for some (at the time of writing this) unknown reason. Leaves the name
    # of the parent folder, which we do not want?!
    folder_name = os.path.basename(filepath)
    name = ("occupancy_batch_%s_" % str(iBatch).zfill(3)) + folder_name
    out_path = os.path.join("out/", name)
    return out_path

def create_pickle_filename(filename, fadc_triggered_only):
    foldername = os.path.dirname(filename)
    basename   = os.path.basename(filename)
    if fadc_triggered_only is False:
        new_fname  = "cPickle_" + basename + ".dat"
    else:
        new_fname  = "cPickle_FADConly_" + basename + ".dat"
    pickle_filename = os.path.join(foldername, new_fname)
    return pickle_filename

def dump_occupancy_data(filename, data, header_text, fadc_triggered_only):
    # this function cPickles the data needed for an occupancy plot and 
    # dumps it
    pickle_filename = create_pickle_filename(filename, fadc_triggered_only)
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
    print('start creating files from %s to %s' % (nStart, nEnd))
    print(type(eventSet))
    for event in eventSet:
        if event < nStart:
            continue
        elif event >= nEnd:
            # a break from here is not safe, because the elements
            # in a set might be randomly ordered. Thus, we cannot
            # be sure to not encounter any elements smaller than
            # nEnd after a single event bigger
            # break
            continue
        filename = create_filename_from_event_number(eventSet,
                                                     event,
                                                     len(eventSet),
                                                     fadcFlag = False)
        list_of_files.append(os.path.join(filepath, filename))

    print('done creating files')
    # given list of files get inodes and sort by them
    # TODO: write support for nStart and nEnd using inodes function!
    if nStart == 0 and nEnd == len(eventSet):
        inodes = create_list_of_inodes(filepath)
    else:
        inodes = [os.stat(el).st_ino for el in list_of_files]
    # TODO FINIISNFINDISNDIANSDIX
    print('done getting inode')
    fi = [x for (y, x) in sorted(zip(inodes, list_of_files))]

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
    

def get_frame_from_file(folder, f, chip):
    """ 
    This is a convenience function, which returns a numpy array of the frame 'f',
    in folder 'folder' given to the function for chip 'chip'.
    """
    
    try:
        evH, chpHs = read_zero_suppressed_data_file(os.path.join(folder, f))
    except IOError:
        print("File not found, returning None.")
        return None
    except IndexError:
        print("File was probably an FADC file.")
        return None

    # extract pixel data of the given chip
    data  = chpHs[chip].pixData
    array = np.zeros((256, 256))

    # insert into empty frame
    array[data[:,1], data[:,0]] = data[:,2]

    return data, array

def write_centroids_to_file(filepath, centroid_list):
    # this function writes a file centroid_hits.dat, which contains the centroids
    # of clusters from the center chip and the energy given by the number of
    # hit pixels
    # we convert the centroids to mm before writing them to file
    
    filename = os.path.join(filepath, "centroid_hits.dat")
    print("Writing centroids to file %s" % filename)
    with open(filename, 'wb') as f:
        f.write("# x_mean / mm\t y_mean / mm\t Energy / keV\n")
        for centroid in centroid_list:
            x, y, E = centroid
            # convert position to mm by subtracting 127 pixels and multiplying
            # resulting pixel by 0.055 mm (55 mu pixel pitch)
            x = (x - 127) * 0.055
            y = (y - 127) * 0.055
            s = "%f\t %f\t %f\n" % (x, y, E)
            f.write(s)
