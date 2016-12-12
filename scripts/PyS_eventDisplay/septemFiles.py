# this file contains the functions related to file handling in
# regards to septem event display

import os
import scandir
import numpy as np
from septemClasses import eventHeader, chipHeaderData
import collections

def read_zero_suppressed_data_file(filepath):#, out_q1):
    # this function reads a single file, created by the septem board
    # with the zero suppressed readout
    # the zero suppressed files are setup as follows:
    # double hash ' ## ' indicates the file header (information about run and event)
    # single hash ' # '  indicates the header for a single chip
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
        else:
            # in this case we're reading actual pixel data
            chpHeaderList[-1].add_pixel(line)
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


def create_files_from_path_combined(folder, test = False):
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
                #n += 1
                filesDict[el] = ""
                eventFiles.append(el)
            elif "fadc" in el:
                #n += 1
                # in case there is an FADC file, strip the FADC flag off it
                # and assign the value to the dictionary
                eventName = el.rstrip("-fadc")
                filesDict[eventName] = el
                #print 'happens!\n\n\n', el, eventName
                files_fadc.append(el)

#    print eventFiles
#    print files_fadc
    eventFiles.sort()
    files_fadc.sort()

    #filesDict.sort()
    #print filesDict
    filesDict = collections.OrderedDict( sorted( filesDict.items() ) )

    #print filesDict
    #print filesDict.keys()[205]

    #import sys
    #sys.exit()
    
    if test == False:
        return [eventFiles, files_fadc]
    else:
        return filesDict
