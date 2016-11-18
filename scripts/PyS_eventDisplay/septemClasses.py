# this file implements all the classes used for the septem event display
import numpy as np


class chip:
    # this class implements a basics single chip 
    def __init__(self):
        self.width         = 14.1
        self.bond_height   = 2
        self.height_active = 14.1
        self.height_full   = self.height_active + self.bond_height

class septem_row:
    # this class implements a single row of chips of the septem board
    def __init__(self, nChips, x_dist, x_offset, y_t_offset, y_b_offset, dist_to_row_below):
        # nChips: number of chips in row
        # x_dist: distance in x direction between each chip
        # x_offset: offset of left edge of first chip in row from
        #           left side of center row
        self.nChips            = nChips
        self.chip_list         = [chip() for _ in range(self.nChips)]
        self.x_dist            = x_dist
        self.x_offset          = x_offset
        self.y_t_offset        = y_t_offset
        self.y_b_offset        = y_b_offset
        self.dist_to_row_below = dist_to_row_below
        self.left              = 0
        self.right             = 0
        self.wspace            = 0
        self.top               = 0
        self.bottom            = 0
        # calculate width and height of row, based on chips and dist
        self.width          = self.nChips * self.chip_list[0].width + (self.nChips - 1) * self.x_dist
        self.height_active  = self.chip_list[0].height_active
        self.height_full    = self.chip_list[0].height_full + self.dist_to_row_below

    def calc_gridSpec(self, x_size, y_size):
        # using calc gridspec one calculates the coordinates of the row on
        # the figure in relative canvas coordinates
        # include padding by adding or subtracting from left, right, top, bottom
        self.left      = self.x_offset / x_size 
        self.right     = self.left + self.width / x_size#1.0 - self.x_offset / x_size
        self.wspace    = self.x_dist / x_size
        #self.hspace    = self.chip_list[0].height_full / y_size
        self.top       = self.y_t_offset / y_size
        self.bottom    = self.top - self.height_active / y_size#self.y_b_offset / y_size

class septem:
    # this class implements the septem board, being built from 3 septem rows
    def __init__(self, padding, fig_x_size, fig_y_size, scaling_factor):
        # self.pChip: simple prototype chip to use the sizes in calculations
        #             for the septem board
        # set number of chips for septem board
        self.nChips = 7

        # include a padding all around the septem event display of 'padding'
        # use size of figure to scale septem accordingly to have it always properly
        # scaled for the given figure
        self.pChip  = chip()
        # take the inverse of the scaling factor (want 1/2 as input to scale to half size)
        self.scaling_factor = 1.0 / scaling_factor

        # first calculate the ratio of the figure
        self.fig_ratio = float(fig_x_size) / float(fig_y_size)

        # distances between different rows in absolute coordinates
        self.y_row1_row2 = 0.38
        self.y_row2_row3 = 3.1
        # size in y direction of whole septem board in absolute coordinates
        self.y_size      = 3 * self.pChip.height_full + self.y_row1_row2 + self.y_row2_row3

        # already define row2_x_dist here (in absolute coordinates) to calculate x_size
        self.row2_x_dist = 0.35
        # 3 chips * width + 2 * distance between chips (in absolute coordinates)
        self.x_size      = 3 * self.pChip.width + (3 - 1)*self.row2_x_dist

        # calculate the ratio of the septem board
        self.ratio = float(self.x_size) / float(self.y_size)

        # and now create the row objects
        self.create_rows()

        # now calculate the needed ratio to get the correct scaling of the septem on any 
        # figure scale. fig_ratio / own ratio
        ratio = self.fig_ratio / self.ratio

        # finally scale from absolute to relative coordinates
        self.row1.calc_gridSpec(self.x_size * ratio * self.scaling_factor, self.y_size * self.scaling_factor)
        self.row2.calc_gridSpec(self.x_size * ratio * self.scaling_factor, self.y_size * self.scaling_factor)
        self.row3.calc_gridSpec(self.x_size * ratio * self.scaling_factor, self.y_size * self.scaling_factor)

        # self.change_size(0.1)
        
    def create_rows(self):
        # this function creates the row objects for the septem class
        
        # calculation of row 1 top and bottom (in abs. coords.):
        # (top need to add padding to top of row 1)
        row1_y_top    = self.y_size - self.pChip.bond_height# - padding * self.y_size
        # bottom in abs. coords.
        row1_y_bottom = 2 * self.pChip.height_full + self.y_row1_row2 + self.y_row2_row3
        # offset of left side from septem in abs. coords.
        row1_x_offset = 6.95# + padding * self.x_size
        # now create the first row with all absolute coordinates
        self.row1     = septem_row(2, 0.85, row1_x_offset, row1_y_top, row1_y_bottom, self.y_row1_row2)

        # calculation of row 2 top and bottom (top & bottom of row2 not affected by padding):
        row2_y_top    = self.y_size - self.pChip.height_full - self.y_row1_row2
        row2_y_bottom = self.pChip.height_full + self.y_row2_row3 + self.pChip.bond_height
        # no offset for row2, defines our left most position in abs. coords.
        row2_x_offset = 0#padding * self.x_size
        self.row2     = septem_row(3, self.row2_x_dist, row2_x_offset, row2_y_top, row2_y_bottom, self.y_row2_row3)

        # calculation of row 3 top and bottom (add padding to bottom):
        row3_y_top    = self.y_size - 2 * self.pChip.height_full - self.y_row1_row2 - self.y_row2_row3
        row3_y_bottom = self.pChip.bond_height# + padding * self.y_size
        row3_x_offset = 7.22# + padding * self.x_size
        self.row3     = septem_row(2, 0.35, row3_x_offset, row3_y_top, row3_y_bottom, 0)

    def change_size(self, padding):
        # NOT WORKING YET
        
        # now update the coordinates such that we get the correct coordinates for the figure size
        # and padding
        self.row1.left = self.row1.left + padding 
        self.row2.left = self.row2.left + padding 
        self.row3.left = self.row3.left + padding 

        self.row1.right = self.row1.right - padding 
        self.row2.right = self.row2.right - padding 
        self.row3.right = self.row3.right - padding 

        # self.row1.top = self.row1.top - padding 
        # self.row2.top = self.row2.top - padding
        # self.row3.top = self.row3.top - padding 

        # self.row1.bottom = self.row1.bottom + padding 
        # self.row2.bottom = self.row2.bottom + padding
        # self.row3.bottom = self.row3.bottom + padding

        self.row1.wspace *= (1.0 - 2 * padding)
        self.row2.wspace *= (1.0 - 2 * padding)
        self.row3.wspace *= (1.0 - 2 * padding)

class event:
    # this class stores a single event from the septem board (zero suppressed readout)
    def __init__(self):
        self.numActiveChips = 0
        
class eventHeader:
    # this class is used to store the data from the header of a single event
    # (zero suppressed from septem board)
    def __init__(self):
        # initialize an empty dictionary for all our header elements
        self.attr = {}
    def set_attribute(self, el):
        # this function is called to set one of the attributes
        # input: el: string of complete line
        if "[" and "]" not in el:
            # the following two keys are a problem: eventNumber accidentally
            # did not have (?) a :, dateTime has several : in line
            # problem_strings = ["eventNumber", "dateTime"]
            problem_strings = ["dateTime"]
            if all(x not in el for x in problem_strings):
                el  = el.split(":")
                val = el[1].strip()
                key = el[0].split()[-1]
            elif "dateTime" not in el:
                el  = el.split()
                val = el[-1].strip()
                key = el[1]
            else:
                el_key  = el.split(":")
                key = el_key[0].split()[-1]
                val = el.split()[-1]
            
            #print val, key
            self.attr[key] = val
        else:
            return

    def get_event_header_text(self, filename):
        # this function returns the string, which is used for
        # the header of the event display
        # columns to fill all strings to
        
        nFill = 24

        try:
            runNum = "Run # : ".ljust(nFill)
            runNum += self.attr["runNumber"] + "\n"
        except KeyError:
            print 'no run number contained, continue without'
            runNum = ''
        
        try:
            event = "Event # : ".ljust(nFill)
            event += self.attr["eventNumber"] + "\n"
        except KeyError:
            try:
                event = "Event # : ".ljust(nFill)
                event += self.attr["eventNumber:"] + "\n"
            except KeyError:
                print 'KeyError: eventNumber not found in file', filename
                print self.attr
                import sys
                sys.exit()
            
        date = "Date : ".ljust(nFill)
        date += self.attr["dateTime"] + "\n"
        
        shutter = "Shutter time / mode : ".ljust(nFill)
        shutter += self.attr["shutterTime"] + " / " + self.attr["shutterMode"] + "\n"

        fadcTrig = "FADC triggered : ".ljust(nFill)
        fadcTrig += self.attr["fadcReadout"] + "\n"
        
        if int(self.attr["fadcReadout"]) == 1:
            fadcTrClock = "    at : ".ljust(nFill)
            fadcTrClock += self.attr["fadcTriggerClock"] + " clock cycles\n"
        else:
            fadcTrClock = ""

        szint = "Szint clock : ".ljust(nFill)
        szint += self.attr["szint2ClockInt"] + "\n"
        
        fname = "Filename : ".ljust(nFill)
        fname += filename

        header = runNum + event + date + shutter + fadcTrig + fadcTrClock + szint + fname
          
        return header

class chipHeaderData:
    # this class is used to store the data from the chip header of a single event
    # (zero suppressed from septem board)
    # and store the data itself, which was read
    def __init__(self):
        self.attr = {}
        # list of pixels is a list of tuples, which stores the
        # active pixels in a given event. After all have been read, we create
        # a numpy array from this and destroy the list of pixels
        self.listOfPixels = []
        self.pixData      = None
    def set_attribute(self, el):
        # this function is called to set one of the attributes
        # input: el: string of complete line
        if "[" and "]" not in el:
            el  = el.split(":")
            val = el[1].strip()
            key = el[0].split()[-1]
            #print val, key
            self.attr[key] = val
        else:
            return
    def add_pixel(self, el):
        # this function is used to add a single pixel to the listOfPixels
        el = el.split()
        if len(el) == 3:
            pixel = (int(el[0]), int(el[1]), int(el[2]))
            if pixel[2] != 11810:
                self.listOfPixels.append(pixel)

    def convert_list_to_array(self):
        # this function is called after all data has been read from the data file
        # for this chip. We create a numpy array from the list of pixels and remove
        # the listOfPixels
        # now go over all tuples and add to the pixArray
        self.pixData = np.asarray(self.listOfPixels)
        del(self.listOfPixels)




class Fadc:
    # this class defines the fadc, includes the data, and is able
    # to apply the necessary corrections
    def __init__(self, filename):
        # the Fadc object is given the filename to its data file
        self.filename = filename
        # define variables for trigger record and post trigger
        self.trigger_rec = 0
        self.posttrig    = 0
        
        # and it immediately reads the data from it
        # fadc values stores the raw data from an fadc data file
        self.fadcValues = self.readFadcFile()
        # and define variables for channels
        self.channel0 = []
        self.channel1 = []
        self.channel2 = []
        self.channel3 = []

        self.pedestalDefaultPath = "/home/schmidt/TOS/bin/data/pedestalRuns/pedestalRun000042_1_182143774.txt-fadc"

        # now apply the pedestal run
        self.applyPedestalRun()
        # and the temporal correction
        self.performTemporalCorrection()

        # after both corrections were done, we can create
        # the individual channels from the data
        self.getChannelsFromFadcArray()
        
        self.pedestalApplied = False
        self.temporalApplied = False
        
    def getChannelsFromFadcArray(self):
        # this function splits the data read from the data file into the
        # 4 seperate channels
        # NOTE: done AFTER pedestal run and temporal correction!
        # FADC file contains data as:
        # channel1_value1, channel2_value1, channel3_value1, channel4_value1,
        # channel1_value2, ... in a list one after another

        # check for pedestal and temporal flag, if not set, perform now:
        if self.pedestalApplied is False:
            applyPedestalRun()
        if self.temporalApplied is False:
            performTemporalCorrection()
        # create the indices for each channel
        ch0_indices = np.arange(0, 4*2560, 4)
        ch1_indices = np.arange(1, 4*2560, 4) 
        ch2_indices = np.arange(2, 4*2560, 4)
        ch3_indices = np.arange(3, 4*2560, 4)
        # and use them to get arrays for each channel
        self.channel0 = np.asarray(self.fadcValues[ch0_indices])
        self.channel1 = np.asarray(self.fadcValues[ch1_indices])
        self.channel2 = np.asarray(self.fadcValues[ch2_indices])
        self.channel3 = np.asarray(self.fadcValues[ch3_indices])

    def applyPedestalRun(self, pedestalFileName = None):
        # this function reads a pedestal calibration run from pedestalFileName
        # and applies it to the fadc array fadc_values (simply the full vector received
        # from the FADC)

        # read pedestal run from pedestalFileName
        if pedestalFileName is not None:
            pedestal_values = self.readFadcFile(pedestalFileName)
        else:
            pedestal_values = self.readFadcFile(self.pedestalDefaultPath)
        # and subtract it from the fadcValues list
        self.fadcValues = self.fadcValues - pedestal_values

        # and set the pedestalAppplied to true
        self.pedestalApplied = True
    
    def performTemporalCorrection(self):
        # this function performs the temporal correction of the FADC vector array
        # due to the FADC using a cyclic memory, one needs to shift the data, which is
        # read from the FADC by n steps, in order to have t = 0 at index 0
        # done by shifting
        # nRoll = (TRIG_REC - POSTTRIG) * 20
        # to the left

        # calculate the number of indices to roll based on trigger record ans post trigger
        self.nRoll = (self.trigger_rec - self.posttrig) * 20
        print "trig rec, posttrig, roll ", self.trigger_rec, self.posttrig, self.nRoll
        # and roll the array
        self.fadcValues = np.roll(self.fadcValues, -self.nRoll)
        # set flag for temporal correction
        self.temporalApplied = True

    def readFadcFile(self, filename = None):
        # this function reads an FADC file from fadcFileName
        # needs to ignore all lines starting with '#'
        if filename == None:
            f = open(self.filename, 'r').readlines()
        else:
            f = open(filename, 'r').readlines()

        fadc_values = []
        for line in f:
            if filename == None:
                if 'postrig' in line:
                    line = line.split()
                    # get last element from line, post trigger
                    self.posttrig = int(line[-1])
                elif 'triggerrecord' in line:
                    line = line.split()
                    # get last element from line, trigger record
                    self.trigger_rec = int(line[-1])
                else:
                    line = line.strip()
                    if "#" not in line:
                        fadc_values.append(float(line))
            else:
                line = line.strip()
                if "#" not in line:
                    fadc_values.append(float(line))

        # convert to numpy array
        fadc_values = np.asarray(fadc_values)
        # and return
        return fadc_values
