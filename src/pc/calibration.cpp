/* This file contains all calibration functions, which are already rewritten using
   FrameArrays. Currently TO calibration and SCurve
   Part of the PC class.
 */

#include "pc.hpp"

void PC::AllChipsSetUniformMatrix(std::set<unsigned short> chip_set,
				  std::map<std::string, boost::any> parameter_map){
    // inputs:
    // 	 std::string TOmode:    a string defining whether we do TOT or TOA calibration
    // 	 int step:              the current step we're in
    // 	 int pixels_per_column: the number of active rows per single step
    // this function simply runs over all chips until nChips and creates the matrices
    // necessary for TO calibration and saves them to file

    int step              = boost::any_cast<int>(parameter_map["step"]);
    int pixels_per_column = boost::any_cast<int>(parameter_map["pixels_per_column"]);
    std::string TOmode    = boost::any_cast<std::string>(parameter_map["TOmode"]);

    // now run over all chips (note _ in front of chip_set!)
    for (auto chip : _chip_set){
        // first create a uniform matrix for either TOT or TOA
        if (TOmode == "TOT"){
            fpga->tp->UniformMatrix(1,0,0,0,0,chip);
        }
        else if (TOmode == "TOA"){
            fpga->tp->UniformMatrix(1,1,0,0,0,chip);
        }
	else if (TOmode == "Medipix"){
            fpga->tp->UniformMatrix(0,0,0,0,0,chip);
	}

        // on top of this now also set the masking and test pulses for all
        // pixels which are considered in the current step
        // only done for chips we actually want to calibrate (!), i.e. part of chip_set
        // try to find chip in the chip_set, if found we set the mask and test pulse
        auto it_chip_in_set = chip_set.find(chip);
        if (it_chip_in_set != chip_set.end()){
            // set mask and test pulse
            fpga->tp->Spacing_row(       step, pixels_per_column, chip);
            fpga->tp->Spacing_row_TPulse(step, pixels_per_column, chip);
	    //fpga->tp->Spacing_row(       0, 256, chip);
            //fpga->tp->Spacing_row_TPulse(0, 256, chip);
        }
        // now load the correct threshold.txt for each chip
        fpga->tp->LoadThresholdFromFile(GetThresholdFileName(chip), chip);
        // and save the current matrix to the correct files
        fpga->tp->SaveMatrixToFile(GetMatrixFileName(chip), chip);

	// set DAC 14 (CTPR) for each chip to 0
        // chip since we start at 0, and not 1....
        fpga->tp->SetDAC(14, chip, 0);
    }

    // now we have set the matrices, but we still have not written them to the
    // chips. perform a WriteReadFSR and then set the matrix. This still leaves
    // the matrices in the pixels. so read out all chips once
    fpga->WriteReadFSR();

    // create temporary object to store readout data after setting matrix, which
    // we do not care about (need to empty all pixels again)
    std::map<int, Frame> temp_map;
    for (auto chip : _chip_set){
	Frame frame;
	temp_map.insert(std::pair<int, Frame>(chip, frame));
    }    
    fpga->SetMatrix();
    usleep(2000);
    fpga->SerialReadOut(&temp_map);
    // now pixels should be empty

    // now we can enable the test pulses for the chips
    fpga->EnableTPulse(1);

}

void PC::SingleChipReadoutCalc(int chip,
			       std::map<std::string, boost::any> parameter_map,
			       std::map<int, Frame> *frame_map,
			       FrameArray<int> &pixel_data){
    // inputs:
    //   int chip: chip number for which to do the calculation
    //   std::map<std::string, boost::any> parameter_map: heterogeneous map, storing
    //       parameters of current step, ctpr, etc...
    //   std::map<int, Frame> *frame_map: pointer to a map storing a whole frame
    //       (furhter built with each call), one for each chip
    //   FrameArray<int> &pixel_data: a reference to the frame array, which contains
    //       the data, which is to be added to the frame array of the current chip
    // this function calculates the variables needed for TOCalib based on the
    // zero suppressed readout of a single chip with spacing in x and y direction
    int sum  = 0;
    int hits = 0;
    double mean = 0.0;
    double var  = 0.0;

    // get the needed variables from the parameter map by using boost::any_cast
    int step                   = boost::any_cast<int>(parameter_map["step"]);
    int CTPR		       = boost::any_cast<int>(parameter_map["CTPR"]);
    int pixels_per_column      = boost::any_cast<int>(parameter_map["pixels_per_column"]);
    std::string callerFunction = boost::any_cast<std::string>(parameter_map["callerFunction"]);

    // frame to save the data into
    // FrameArray<int> pixel_data = {};
    // define the variable for step size in y direction
    int npix_per_dim = fpga->tp->GetPixelsPerDimension();
    int y_step_size  = npix_per_dim / pixels_per_column;

    // read the pixel_data for the current chip
    // need to hand address to pixel_data
    // TODO: check numhits for error, timeout etc?
    // if (callerFunction == "TOCalib"){
    // 	int numhits = fpga->DataFPGAPC(&pixel_data, chip);
    // }
    // else{
    // 	usleep(1000);
    // 	//int nChips = fpga->tp->GetNumChips();
    // 	std::map<int, Frame> fr_map;
    // 	// for(int i = 1; i <= nChips; i++){
    // 	//     // create a pair and insert into frame_map
    // 	//     Frame chip_frame;
    // 	//     fr_map.insert(std::pair<int, Frame>(i, chip_frame));
    // 	// }
    // 	Frame chip_frame;
    // 	fr_map.insert(std::pair<int, Frame>(chip, chip_frame));
    // 	//int numhits = fpga->DataFPGAPC(&pixel_data, chip);

    // 	int result  = fpga->SerialReadOut(&fr_map);
    // 	//int result  = fpga->SerialReadOut(&pixel_data);

	
    // 	pixel_data = fr_map[chip].GetPixelData();
    // (*frame_map)[chip].SetFrame(pixel_data);
    // (*frame_map)[chip].ConvertFullFrameFromLFSR();
    // (*frame_map)[chip].SetMask(fpga->tp->GetMaskArray(chip));	
    // (*frame_map)[chip].CalcFullFrameVars();
    // }
    // now set the pixel data of the correct frame in the frame_map
    // to eventually create the full frame
    //(*frame_map)[chip].ResetMemberVariables();
    bool convert_from_LFSR = false;
    if (callerFunction == "TOCalib"){
    	convert_from_LFSR = false;
    }
    else if (callerFunction == "SCurve"){
	// in case of SCurve we do full frame readout, hence we need to convert
	// the pixels from pseudo random to normal pix values
	convert_from_LFSR = true;
	// in case of SCurve we might also set the mask
	//(*frame_map)[chip].SetMask(fpga->tp->GetMaskArray(chip));
    }


    std::cout << "ctpr " << CTPR << " step " << step
	      << " x_step_size " << 32 
	      << " y_step_size " << y_step_size
	      << std::endl;

    (*frame_map)[chip].SetPartialFrame(pixel_data,
    				       CTPR,
    				       32,
    				       step,
    				       y_step_size,
    				       convert_from_LFSR);

    // and get the sum, mean and hits values form this partial frame
    sum  = (*frame_map)[chip].GetLastPFrameSum();
    hits = (*frame_map)[chip].GetLastPFrameHits();
    mean = (*frame_map)[chip].GetLastPFrameMean();
    var  = (*frame_map)[chip].GetLastPFrameVariance();
    // sum  = (*frame_map)[chip].GetFullFrameSum();
    // hits = (*frame_map)[chip].GetFullFrameHits();
    // mean = (*frame_map)[chip].GetFullFrameMean();
    // var  = (*frame_map)[chip].GetFullFrameVariance();

    
    // now calculate the standard deviation of the last partial frame
    double std;
    std = sqrt(var);

    // get variables needed for printing current status 
    int pulse = boost::any_cast<int>(parameter_map["pulse"]);
    // define thl and iter variables (thl for SCurve, iter for TOCalib)
    int thl  = 0;
    int iter = 0;
    if (callerFunction == "SCurve"){ 
	thl   = boost::any_cast<int>(parameter_map["thl"]);
	std::cout << "thl "  << thl  << "\t" << std::flush;
    }
    else if (callerFunction == "TOCalib"){
	iter  = boost::any_cast<int>(parameter_map["iteration"]);
	std::cout << "iter " << iter << "\t" << std::flush;
    }

    // and give some output to see what's going on :)
    std::cout << "pulse "    << pulse << "\t"
              << "chip "     << chip  << "\t"
              << "step "     << step  << "\t"
              << "CTPR "     << CTPR  << "\t"
              << "mean "     << mean  << "\t"
              << "hits "     << hits  << "\t"
              << "sum "      << sum   << "\t"
    	      << "variance " << var   << "\t"
    	      << "std "      << std
    	      << std::endl;

#if DEBUG==4
    std::string filename;
    if (callerFunction == "SCurve"){
	filename = GetFrameDumpFilename(thl, step, pulse);
    }
    else{
	filename = GetFrameDumpFilename(iter, step, pulse);
    }
    (*frame_map)[chip].DumpFrameToFile(filename);
#endif

}

void PC::AllChipsSingleStepCtpr(std::set<unsigned short> chip_set,
				std::map<std::string, boost::any> parameter_map,
				std::map<int, Frame> *frame_map){
    // inputs:
    //   std::set<unsigned short> chip_set: set storing which chips we are working on
    //   std::map<std::string, boost::any> parameter_map: heterogeneous map storing
    //       the needed parameters, e.g. step, ctpr, etc...
    //   std::map<int, Frame> pointer to a map storing whole frames (to be built with
    //       each call), one for each chip
    // this function performs a single step and single CTPR value for all chips
    // note: function does not return anything, since frame_map stores the partial
    //       frames, which are read

    // get the needed parameters from the map using boost::any_cast
    int CTPR		       = boost::any_cast<int>(parameter_map["CTPR"]);
    std::string shutter_range  = boost::any_cast<std::string>(parameter_map["shutter_range"]);
    std::string shutter_time   = boost::any_cast<std::string>(parameter_map["shutter_time"]);
    std::string callerFunction = boost::any_cast<std::string>(parameter_map["callerFunction"]);

    // we only want to set the CTPR DAC for the chips we want to calibrate, hence
    // only run over chip_set
    // TODO: understand if we need to load the FSR for all chips, or only for the ones
    // we read out!!!
    for (auto chip : chip_set){
        //int current_chip = *it_chip_set;
        // now we need to load the FSR for each of the connected chips
        fpga->tp->LoadFSRFromFile(GetFSRFileName(chip), chip);
        // we use a bitshift to set the correct bit of the CTPR DAC
        // CTPR runs from 0 to 31, shift a 1 from 0th bit CTPR times
        // to the left


        // NOTE::THERE WAS STILL CTPRVAL == 255 ALL THE TIME!!! AHHH

	// NOTE:!!! a CTPR value of 0 (variable CTPR) results in
	//          CTPRval == 1 (obviously).
	// HOWEVER!!! THIS results in active x columns starting
	//            from 31 instead of 0, as one would expect!!!
	// so not x = 0, 32, 64 is active, but rather
	// x = 31, 63, ...
	// that's probably why in old TOS there was SCurveScan and
	// SCurveScan_meanchip where in SCurve the CTPR value would only
	// be set to the maximum value of an int (to toggle all bits to 1)!
	// NOTE AGAIN: this is DIRECTLY affected by the preload (since
	// the preload is taken into account when reading from non zero suppressed
	// readout!
	// NOTE: MEANING: for octoboard with only 7 working chips need to use
	// : preload of 7 -> then CTPRval = 1 results in x = 0, 32, 64 as one
	// would want!
	// HOWEVER: CheckOffset results in a best value of 6 instead of 7. And
	// writing and reading a chess matrix 
	

	
        //unsigned int CTPRval = 255;//;1 << CTPR;
	// NOTE: the calculation of CTPRval now takes into account that the CTPR
	//     value needs to be 1 larger than the offset one wants, also taking
	//     into account that an offset of CTPR == 32 should not overflow
	unsigned int CTPRval = 0;
	CTPRval = 1 << ((CTPR + 1) % 32);
        // and set the CTPR DAC to that value
        fpga->tp->SetDAC(14, chip, CTPRval);

	if (callerFunction == "SCurve"){
	    // in case of SCurve also set DAC 13. Value 7, why?!
	    fpga->tp->SetDAC(13, chip, 7);

	    int thl = boost::any_cast<int>(parameter_map["thl"]);
	    unsigned short thl_dac = 6;
	    SetDACallChips(thl_dac, thl, chip_set);
	}
    }
    // write the DAC values to the chips
    fpga->WriteReadFSR();
    fpga->WriteReadFSR();
    usleep(400);

    if (callerFunction == "TOCalib"){
	// call counting function to open shutter and apply test pulses
	// for TOCalib only done once and then read out all chips
	fpga->CountingTime(shutter_time, shutter_range);


	// and activate zero suppressed readout in case of TOCalib
        int temp_result = 0;
	temp_result = fpga->DataChipFPGA();

	// now we loop over all chips in the chip_set to read them out
	// we use a map to map mean_std_pairs to the chips
	for (auto chip : chip_set){
	    // first readout a single chip
	    FrameArray<int> pixel_data;
	    fpga->DataFPGAPC(&pixel_data, chip);
	    // call the function which performs the calculation for all chips and finally
	    // starts to built the whole frames
	    SingleChipReadoutCalc(chip, parameter_map, frame_map, pixel_data);
	}
    }
    else if (callerFunction == "SCurve"){
	// now we loop over all chips in the chip_set to read them out
	// we use a map to map mean_std_pairs to the chips

	std::map<int, Frame> readout_frame_map;
	// fill the readout_frame_map with frames for each chip, which we would
	// like to read out
	for (auto chip : chip_set){
	    Frame chip_frame;
	    readout_frame_map.insert(std::pair<int, Frame>(chip, chip_frame));
	}

	// call counting function to open shutter and apply test pulses
	// for SCurve need to open shutter after every chip (we read out all chips!)
	fpga->CountingTime(shutter_time, shutter_range);
	// now read out all chips of the chip_set into the temporary
	// readout_frame_map
	int result  = fpga->SerialReadOut(&readout_frame_map);
	if (result != 300){
	    std::cout << "AllChipsSingleStepCtpr Error: call to SerialReadout failed.\n"
		      << "    unexpected behaviour may occur.\n"
		      << "    error code: " << result
		      << std::endl;
	}
	//int result  = fpga->SerialReadOut(&pixel_data);
	for (auto chip : chip_set){
	    //int temp_result;
	    // get the correct pixel data from the temp. readout map
	    FrameArray<int> pixel_data;
	    pixel_data = readout_frame_map[chip].GetPixelData();
	    // call the function which performs the calculation for all chips and finally
	    // starts to built the whole frames
	    SingleChipReadoutCalc(chip, parameter_map, frame_map, pixel_data);
	}
    }


}


void PC::SingleIteration(std::set<unsigned short> chip_set,
			 std::map<std::string, boost::any> parameter_map,
			 std::map<int, std::pair<double, double>> *chip_mean_std_map){
    // inputs:
    //   std::set<unsigned short> chip_set: set storing all chips we work on
    //   std::map<std::string, boost::any> parameter_map: heterogeneous map storing
    //       all parameters, which are needed, e.g. step, pulse, etc..
    //   std::map<int, std::pair<int, double>> *chip_mean_std_map: a map containing
    //       a single pair of mean and std values for each chip, which are calculated
    //       from the full frames, which are built over one whole iteration.
    //       after every iteration we simply add the mean and std values of this
    //       iteration to the map. After all iterations are done, we divide by the
    //       number of iterations
    // this function performs a single iteration of the TOCalib function

    // leaves the following to do:
    // - steps
    //   - CTPR
    //     - chips
    // chips will be done in seperate function too

    // number of pixels in one dimension and number of chips
    int npix_per_dim = fpga->tp->GetPixelsPerDimension();
    // define a variable for number of chips for convenience
    int step_size = 0;

    int pixels_per_column      = boost::any_cast<int>(parameter_map["pixels_per_column"]);
    int CTPR_start             = boost::any_cast<int>(parameter_map["CTPR_start"]);
    int CTPR_stop              = boost::any_cast<int>(parameter_map["CTPR_stop"]);


    std::string callerFunction = boost::any_cast<std::string>(parameter_map["callerFunction"]);


    // we create a map of chips and frames, which is used to built the full frames
    // from the partial frames (read by step and CTPR)
    std::map<int, Frame> frame_map;
    // fill the map with empty frames, one for each chip we use
    for(auto chip : chip_set){
	// we create one empty frame for each chip, by creating a
	// Frame object. Frame creator initializes to zero
	Frame chip_frame;
	frame_map.insert( std::pair<int, Frame>(chip, chip_frame) );
    }
    
    if (callerFunction == "TOCalib"){
	step_size = 1;
    }
    else if (callerFunction == "SCurve"){
	step_size = 8;
    }

    for (int step = 0; step < (npix_per_dim / pixels_per_column); step += step_size){
        // per single step (meaning we use test pulses on pixels_per_column rows
        // of the whole chip), we further separate each row into several batches
        // of columns (using the CTPR DAC)

	// first store the current step in our heterogeneous map
	parameter_map["step"] = step;

        // the first thing to do within one step is to set the matrices for the
        // chips appropriately.
	AllChipsSetUniformMatrix(chip_set, parameter_map);

        for (int CTPR = CTPR_start; CTPR < CTPR_stop; CTPR++){
            // thus we divide each row into 32 batches
	    // and store the CTPR value in the map
	    parameter_map["CTPR"] = CTPR;

            // now we run over all chips
	    // SingleStepCtpr will perform a single shutter opening and closing of
	    // the parameters given in parameter_map, results saved to frames in frame_map
	    AllChipsSingleStepCtpr(chip_set, parameter_map, &frame_map);

        }
	// disable test pulses again
	fpga->EnableTPulse(0);
    }

    // now we basically have to calculate the important variables for the whole frame, which was
    // created in the iteration
    for (auto chip_pair : frame_map){
	// get the chip and frame from the frame map
	int chip = chip_pair.first;
	Frame chip_frame = chip_pair.second;

	double mean;
	double variance;
	double std;
	// calculate the sum, hits and mean values of the full frame
	// argument means we want to run over whole frame
	// TODO: CTPR_start should not be necessary! IF you're reading this, CHECK!!!
	chip_frame.CalcFullFrameVars(CTPR_start);
	mean     = chip_frame.GetFullFrameMean();
	// now calculate variance and std
	variance = chip_frame.GetFullFrameVariance();
	std      = sqrt(variance);

	// now get the current value of the chip_mean_std_map to add the
	// values of this iteration
	std::pair<double, double> current_pair;
	current_pair = (*chip_mean_std_map)[chip];
	current_pair.first  += mean;
	current_pair.second += std;
	// and set this pair again as the chips value
	(*chip_mean_std_map)[chip] = current_pair;

	// and print the values for this iteration and this chip
	int iter  = boost::any_cast<int>(parameter_map["iteration"]);
	int pulse = boost::any_cast<int>(parameter_map["pulse"]);

	std::cout << "\n \n"
		  << "iteration "  << iter
		  << " for pulse " << pulse << " done.\n"
		  << " chip "      << chip
		  << " mean "      << mean
		  << " variance "  << variance
		  << " std "       << std
		  << std::endl;

    }

}


void PC::TOCalib(std::string callerFunction,
		 std::set<unsigned short> chip_set,
		 std::string TOmode,
		 std::string pulser,
		 std::list<int> pulseList,
		 int pixels_per_column,
		 std::string shutter_range,
		 std::string shutter_time){
    // function which serves as a wrapper to call CalibrationMeta() with
    // a parameter map with correct parameters for TOCalib
    // input:
    //     std::string callerFunction: a string to distinguish whether TOCalib or SCurve called
    //                                 this function
    //     std::set<unsigned short> chip_set:     a set containing all chips for which we will do a
    //                                 TO calibration
    //     std::string TOmode:         a string defining whether we use TOT, TOA or
    //                                 Medipix mode
    //     std::string pulser:         a string defining whether we use an internal or
    //                                 external pulser
    //     std::list<int> pulseList:   a list containing the integers of the desired pulse
    //                                 values
    //     int pixels_per_column:      number of active rows per single step
    //     std::string shutter_range:  range of shutter opening (std, long, verylong)
    //     std::string shutter_time:   shutter time in range from 0 to 255


    // create hetereogeneous map to store variables needed for TOcalib
    // and enter defaults where necessary
    // define a heterogeneous map, which stores iteration parameters
    std::map<std::string, boost::any> parameter_map;
    parameter_map["TOmode"]            = TOmode;
    parameter_map["shutter_range"]     = shutter_range;
    parameter_map["shutter_time"]      = shutter_time;
    parameter_map["pixels_per_column"] = pixels_per_column;
    // add CTPR start and stop values
    parameter_map["CTPR_start"]        = 0;
    parameter_map["CTPR_stop"]         = 32;
    parameter_map["callerFunction"]    = callerFunction;

    // in case of the TOCalib:
    // this function follows the operation logic of the TOCalibFast function
    // logic tree of loops of TOCalibFast:
    // - voltages
    //   - iterations
    //     - steps
    //       - CTPR
    //         - chips

    // start by looping over all voltages, need iterator over list
    for(auto pulse : pulseList){
        // this is the main outer loop over all voltages

        // check if we're using an internal pulser. in this case we need to set the
        // correct DAC to the value we want (i.e. the current pulseList iterator + 350)
        if (pulser == "internal"){
	    SetTestpulseThresholds(pulse, callerFunction);
        }

	// create a map of chip numbers and a pair of (mean, std) values, which will be
	// handed to the function performing a single iteration
	std::map<int, std::pair<double, double>> chip_mean_std_map;
	chip_mean_std_map = GetZeroInitMeanStdMap(chip_set);

	// add current pulse height to parameter_map
	parameter_map["pulse"] = pulse;//*it_pulseList;

	int nIterations;
	// loop over iterations (typically we want to do 4 iterations)
	// we do 4 iterations to average over statistical fluctuations
	nIterations = 4;
	parameter_map["nIterations"] = nIterations;
	for(int iter = 0; iter < nIterations; iter++){
	    // add current iteration to parameter_map
	    parameter_map["iteration"]  = iter;
	    // and call the function, which performs a single iteration
	    SingleIteration(chip_set, parameter_map, &chip_mean_std_map);
	}
	// call function to print TOCalib data to file
	WriteTOCalibToFile(chip_set, parameter_map, chip_mean_std_map);
	
        if (pulser == "external"){
            std::cout << "external pulser voltage finished." << std::endl;
            // we will return to the CommandTOCalib function to ask for user input
            // regarding additional voltages on the external pulser
            // thus, we return here and ask for more input
            return;
        }
    }

    
}

void PC::SCurve(std::string callerFunction,
		std::set<unsigned short> chip_set,
		std::string pulser,
		std::list<int> pulseList,
		std::string shutter_range,
		std::string shutter_time,
		int CTPR,
		std::pair<int, int> threshold_boundaries){
    // function which serves as a wrapper to call CalibrationMeta() with
    // a parameter map with correct parameters for SCurve
    // input:
    //     std::string callerFunction: a string to distinguish whether TOCalib or SCurve called
    //                                 this function
    //     std::set<unsigned short> chip_set:     a set containing all chips for which we will do a
    //                                 TO calibration
    //     std::string pulser:         a string defining whether we use an internal or
    //                                 external pulser
    //     std::list<int> pulseList:   a list containing the integers of the desired pulse
    //                                 values
    //     std::string shutter_range:  range of shutter opening (std, long, verylong)
    //     std::string shutter_time:   shutter time in range from 0 to 255
    //     int CTPR:                   the CTPR to be used for SCurve (columns which receive
    //                                 test pulses)
    //     std::pair<int, int> threshold_boundaries: the THL bounds to be used for the SCurve
    //                                 scan.

    // create hetereogeneous map to store variables needed for TOcalib
    // and enter defaults where necessary
    // define a heterogeneous map, which stores iteration parameters
    std::map<std::string, boost::any> parameter_map;
    std::string TOmode("Medipix");
    parameter_map["TOmode"]            = TOmode;
    parameter_map["shutter_range"]     = shutter_range;
    parameter_map["shutter_time"]      = shutter_time;
    // using a default of 8 pixels per column for SCurve
    parameter_map["pixels_per_column"] = 8;
    // add CTPR start and stop values
    parameter_map["CTPR_start"]        = CTPR;
    // stop one larger, because we only want a single CTPR value
    parameter_map["CTPR_stop"]         = CTPR + 1;
    // only one iteration, so add this to parameter_map
    parameter_map["iteration"]         = 1;
    //parameter_map["THL_bounds"]        = threshold_boundaries;
    parameter_map["callerFunction"]    = callerFunction;

    // in case of SCurve:
    // the function deviates slightly from the SCurveFast logic:
    // - voltages
    //   - one iteration
    //     - steps
    //       - one CTPR value
    //         - scan THL values for all chips

    // start by looping over all voltages, need iterator over list
    //std::list<int>::iterator it_pulseList;
    //for(it_pulseList = pulseList.begin(); it_pulseList != pulseList.end(); it_pulseList++){
    for(auto pulse : pulseList){
        // this is the main outer loop over all voltages

	// get the THL boundaries and create the map to store
	// the mean counts for each chip during this pulse
	int lower_THLbound = threshold_boundaries.first;
	int upper_THLbound = threshold_boundaries.second;

	// create a map, which stores the thl mean values for all chips
	// first index: chip number, contains:
	//     map of: <thl values, mean values>
	// note: can access specific thl value via second index to map
	std::map<int, std::map<int, double>> thl_mean_map;

        // check if we're using an internal pulser. in this case we need to set the
        // correct DAC to the value we want (i.e. the current pulseList iterator + 350)
        if (pulser == "internal"){
	    SetTestpulseThresholds(pulse, callerFunction);
        }

	// add current pulse height to parameter_map
	parameter_map["pulse"] = pulse;//*it_pulseList;

	for(int thl = lower_THLbound; thl < upper_THLbound; thl++){
	    // now iterate over all THL values for all steps and all chips
	    // set THL values for all chips and then call single iteration
	    // afterwards take frame map from this iteration and perform calcs
	    // necessary to get mean (corrected for LFSR lookup)
	    // then in here add that to vector of means etc. to store later
	    parameter_map["thl"] = thl;

            // create a map of chip numbers and a pair of (mean, std) values, which will be
	    // handed to the function performing a single iteration
	    // map needs to be created here to zero initialize it after every
	    // thl value
	    std::map<int, std::pair<double, double>> chip_mean_std_map;
	    chip_mean_std_map = GetZeroInitMeanStdMap(chip_set);
	
	    unsigned short thl_dac = 6;
	    SetDACallChips(thl_dac, thl, chip_set);
	    SingleIteration(chip_set, parameter_map, &chip_mean_std_map);
	    for(auto chip : chip_set){
		// iterate over chip set and push mean values back into vector
		double mean = chip_mean_std_map[chip].first;
		thl_mean_map[chip][thl] = mean;
	    }
	    std::cout << "THL value " << thl << " done." << std::endl;
	}
	// now have all necessary information in thl_mean_map

	// call function to print SCurve data to file
	WriteSCurveToFile(chip_set, thl_mean_map, pulse);

        if (pulser == "external"){
            std::cout << "external pulser voltage finished." << std::endl;
            // we will return to the CommandTOCalib function to ask for user input
            // regarding additional voltages on the external pulser
            // thus, we return here and ask for more input
            return;
        }
    }

    
}


void PC::WriteTOCalibToFile(std::set<unsigned short> chip_set,
			    std::map<std::string, boost::any> parameter_map,
			    std::map<int, std::pair<double, double>> chip_mean_std_map){
    // function which writes the data taken during TO calibration to a file
    // inputs:
    //     - std::set<unsigned short> chip_set: set of integers corresponding to chips, for
    //                               which TOCalib ran
    //     - std::map<int, std::pair<double, double>> chip_mean_std_map: map which
    //                               contains one pair of values for each chip of
    //                               type <mean, std>
    //     - int pulse: pulse height currently to be printed

    int pulse          = boost::any_cast<int>(parameter_map["pulse"]);
    int nIterations    = boost::any_cast<int>(parameter_map["nIterations"]);
    std::string TOmode = boost::any_cast<std::string>(parameter_map["TOmode"]);    

    // after all iterations we can now caluclate the final mean values of mean and std
    // and print them to a file
    for(auto chip : chip_set){
	// get pair of current chip
	std::pair<double, double> pair;
	pair = chip_mean_std_map[chip];

	// and calculate the mean value of the sum of means, which is stored
	// in the chip_mean_std_map
	double mean;
	double std;
	mean = pair.first  / nIterations;
	std  = pair.second / nIterations;

	// and write to file
	std::fstream f;
	std::string filename;
	if (TOmode == "TOT"){
	    filename = GetTOTCalibFileName(chip);
	}
	else{
	    filename = GetTOACalibFileName(chip);
	}

	f.open(filename, std::fstream::out | std::fstream::app);

	f << "pulse " << pulse << "\t"
	  << "chip "  << chip  << "\t"
	  << "mean "  << mean  << "\t"
	  << "std "   << std
	  << std::endl;

	f.close();
    }
}

void PC::WriteSCurveToFile(std::set<unsigned short> chip_set,
			   std::map<int, std::map<int, double>> thl_mean_map,
			   int pulse){
    // function which writes the data taken during SCurve to a file
    // inputs:
    //     - std::set<unsigned short> chip_set: set of integers corresponding to chips, for
    //                               which TOCalib ran
    //     - std::map<int, std::map<int, double>> thl_mean_map: map which
    //                               contains one map of values for each chip of
    //                               type <thl values, mean_values>
    //     - int pulse: pulse height currently to be printed

    for(auto chip : chip_set){
	// and write to file
	std::fstream f;
	std::string filename;
	filename = GetSCurveFileName(chip, pulse);
	// check if folder in which filename sits exists
	int folder_check;
	boost::filesystem::path p = GetPathFromFilename(filename);
	folder_check = CreateFolderIfNotExists(p);
	if (folder_check == -1){
	    std::cout << "Error: WriteSCurveToFile() Could not create folder." << std::endl;
	}

	f.open(filename, std::fstream::out);
	if(f.fail() == false){
	    // get the thl mean map of the current chip
	    std::map<int, double> current_thl_mean_map;
	    current_thl_mean_map = thl_mean_map[chip];
	
	    f << "# pulse height: " << pulse << std::endl;
	    f << "# THL \t Mean hits" << std::endl;
	    // now iterate over map of current chip and write file
	    for(auto const pair : current_thl_mean_map){
		int thl;
		double mean;
		thl  = pair.first;
		mean = pair.second;
		f << thl << "\t" << mean << std::endl;
	    }
	    // close file
	    f.close();
	}
	else{
	    std::cout << "Error: WriteSCurveToFile() encountered error on opening output file.\n"
		      << "       Probably could not create output folder.\n"
		      << "       Error was: " << filename << std::endl;
	}

    }
    
}

void PC::SetTestpulseThresholds(int pulse, std::string callerFunction){
    // this function sets the DACs needed for the lower and upper
    // threshold for the test pulses based on a given pulse height
    int voltage;
    int threshold_voltage;
    threshold_voltage = 350;
    voltage = pulse + threshold_voltage;
    // the upper bound for the DAC
    // fpga->i2cDAC(voltage, 3);
    // // the lower bound
    // fpga->i2cDAC(threshold_voltage, 2);
    fpga->i2cDAC(voltage, 2);
    fpga->i2cDAC(threshold_voltage, 3);
    if (callerFunction == "TOCalib"){
	// one pulse w/ frequency 50/500 kHz
	fpga->tpulse(1, 50);
    }
    else if (callerFunction == "SCurve"){
	// 1000 pulses w/ frequency 10/500 kHz
	fpga->tpulse(1000, 10);
    }
}

std::map<int, std::pair<double, double>> PC::GetZeroInitMeanStdMap(std::set<unsigned short> chip_set){
    // we will zero initialize the map, since we want to add mean and std
    // values after every iteration on the current value and calculate the mean
    // of this after all iterations are done

    std::map<int, std::pair<double, double>> chip_mean_std_map;
    for (auto chip : chip_set){
	// create a zero initialized pair for current chip
	double mean = 0;
	double std = 0;
	std::pair<double, double> pair;
	pair = std::make_pair(mean, std);

	// and insert element into map
	chip_mean_std_map[chip] = pair;
    }

    return chip_mean_std_map;
}



int PC::SCurveSingleTHL(unsigned short thl,
			unsigned short chip,
			int time,
			int step,
			int pulse){
    // NOTE: FUNCTION IS DEPRECATED! will be taken out soon
    /* This function performs a single thl scan for the SCurve function
       unsigned short thl: the threshold value to scan for
     */
    int result=0;
    int meancounts;
    bool dump_files;
#if DEBUG==4
    dump_files = true;
#else
    dump_files = false;
#endif

    // set THL DAC
    SetDACandWrite(6,chip,thl);
    // sleep shortly before we open shutter
    usleep(400);
    // calling CountingTime with second argument == 1
    // corresponds to n = 1, power of 256
    fpga->CountingTime(time, 1);

    // create an empty frame array to store the data for the readout
    FrameArray<int> pixel_data = {};
    result=fpga->SerialReadOut(&pixel_data);
    
    
    if(result==300){
	Frame thl_frame;
	
	thl_frame.StackFrame(pixel_data);
	thl_frame.ConvertFullFrameFromLFSR();
	thl_frame.SetMask(fpga->tp->GetMaskArray(chip));
	thl_frame.CalcFullFrameVars();//offset);
	meancounts = thl_frame.GetFullFrameMean();
	int hits = thl_frame.GetFullFrameHits();
	std::cout << "hits = " << hits << std::endl;

	if (dump_files){
	    std:: string filename;
	    filename = GetFrameDumpFilename (thl, step, pulse);
	    thl_frame. DumpFrameToFile (filename);
	}

    }
    else{
	return result;
    }

    return meancounts;
}


// void PC::FindPreloadUsingTestPulses(){
//     // this function attempts to find the corret preload to readout
//     // using test pulses 
//     // we create a map of chips and frames, which is used to built the full frames
//     // from the partial frames (read by step and CTPR)
//     std::map<int, Frame> frame_map;
//     // fill the map with empty frames, one for each chip we use
//     int nChips = tp->GetNumChips();
//     std::set<unsigned short> chip_set;
//     for(int i = 1; i < nChips; i++){
// 	chip_set.insert(i);
// 	// we create one empty frame for each chip, by creating a
// 	// Frame object. Frame creator initializes to zero
// 	Frame chip_frame;
// 	frame_map.insert( std::pair<int, Frame>(i, chip_frame) );
//     }


//     std::map<std::string, boost::any> parameter_map;
//     parameter_map["step"] = 0;
//     parameter_map["pixels_per_column"] = 8;
//     parameter_map["TOmode"] = "Medipix";
//     parameter_map["pulse"] = 200;

//     AllChipsSetUniformMatrix(chip_set, parameter_map, nChips, npix_per_dim);    
    
// }


/* Write a function which does the following:
   Apply test pulses similar to SCurve scan
   Then set CTPR value offset of 0 (CTPRval == 1), which should
   mean that columns x = 0, 32, etc. are turned on
   Then apply and read out using full matrix read out into frame map
   Set full frames without any mask.
   Use full frames to search for pixels where
      pix_value > 0.9 * # of test pulses
   for these pixels look at pattern in columns. 
   Is it x = 0, 32 then good. 
   Is it x = 0 + off, 32 + off etc, then
       preload = preload - off
   
   

 */

