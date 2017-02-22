# this file contains the functions related to file handling in
# regards to septem event display

import os
import scandir
import numpy as np
from septemClasses import eventHeader, chipHeaderData
import collections

def read_zero_suppressed_data_file(filepath, header_only = False):#, out_q1):
    # this function reads a single file, created by the septem board
    # with the zero suppressed readout
    # the optional flag header_only can be used to only read the header.
    # in this case the pix data array will be set to []
    # the zero suppressed files are setup as follows:
    # double hash ' ## ' indicates the file header (information about run and event)
    # single hash ' # '  indicates the header for a single chip
    if header_only == True:
        # in case we only want the header, give the sizehint equal to 1
        # for very large files, not the whole file will be read, speeding up the
        # process
        f = open(filepath, 'r').readlines(sizehint = 1)
    else:
        # else read whole file (until EOF)
        f = open(filepath, 'r').readlines()

    evHeader  = eventHeader(filepath)
    chpHeaderList = []
    for line in f:
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
        
        # for el in files:
        #     if "data" in el and "fadc" not in el:
        #         #n += 1
        #         if test == True:
        #             filesDict[el] = ""
        #         else:
        #             eventFiles.append(el)
        #     elif "fadc" in el:
        #         #n += 1
        #         # in case there is an FADC file, strip the FADC flag off it
        #         # and assign the value to the dictionary
        #         eventName = el.rstrip("-fadc")
        #         if test == True:
        #             filesDict[eventName] = el
        #         #print 'happens!\n\n\n', el, eventName
        #         else:
        #             files_fadc.append(el)

    # print eventFiles
    # print files_fadc
    #if test == False:
        # eventFiles.sort()
        # files_fadc.sort()
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
    #                now the total number (which we then cannot deduce from the number
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
            
    
