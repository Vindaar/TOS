// this function implements a basic frame read from a timepix chip
//   - by Sebastian Schmidt

#include "frame.hpp"
#include "helper_functions.hpp"

// include assert to check for pixel value being < 11810
#include <cassert>

Frame::Frame(): 
    _pix_per_dimension(PIXPD),
    _LFSR_set(false)
{
    // in case of the empty creator, we simply create an empty frame, 
    // which is 0 initialized
    // note: in this case the size of the timepix chip defined by a macro
    // in the timepix.hpp header is used as dimension

    // initialize member variables to zero (except dimension and LFSR_lookup table)
    ResetMemberVariables();
}

Frame::Frame(FrameArray<int>& pixel_data):
    _pix_per_dimension(PIXPD),
    _LFSR_set(false)
{
    // in this case we create an empty frame first and then put in
    // the data from the pixel_data array

    // initialize member variables to zero (except dimension and LFSR_lookup table)
    ResetMemberVariables();

    // use the stack frame function to create the initialized frame
    // StackFrame(pixel_data);
    // set the given frame as the frame of this object
    SetFrame(pixel_data);

    // now our _pixel_data member variable contains the 2D array
}

Frame::~Frame(){
    // nothing to do here
}

FrameArray<int> Frame::GetPixelData(){
    return _pixel_data;
}

FrameArray<bool> Frame::GetPixelSet(){
    return _pixel_set;
}

void Frame::ResetMemberVariables(){
    // this function resets, i.e. sets to zero, all member variables

    // set the FrameArrays to 0 / false
    for(std::size_t x = 0; x < _pix_per_dimension; x++){
	for(std::size_t y = 0; y < _pix_per_dimension; y++){
	    _pixel_data[x][y] = 0;
	    _pixel_set[x][y]  = false;
	    _mask_data[x][y]  = false;
	}
    }
    
    // set the last partial frame variables to 0
    ResetLastPFrameVariables();

    // zero initialize member variables
    ResetFullFrameVariables();

}

void Frame::ResetLastPFrameVariables(){
    // set last partial frame variables to 0
    // zero initialize last partial frame member variables
    _lastPFrameSum      = 0;
    _lastPFrameMean     = 0.0;
    _lastPFrameHits     = 0;
    _lastPFrameSet      = 0;
    _lastPFrameVariance = 0.0;
}

void Frame::ResetFullFrameVariables(){
    // set full frame variables to 0
    _fullFrameSum      = 0;
    _fullFrameMean     = 0.0;
    _fullFrameHits     = 0;
    _fullFrameSet      = 0;
    _fullFrameVariance = 0.0;
}

void Frame::SetFrame(FrameArray<int>& pixel_data){
    // set the pixel_data of this Frame object to the pixel data given to
    // it by copying it

    for(std::size_t x = 0; x < _pix_per_dimension; x++){
    	for(std::size_t y = 0; y < _pix_per_dimension; y++){
    	    // note: IT MIGHT BE that due to the stupidity that is TOS
    	    // one might need to reverse the ordering of x and y for the 
    	    // pixel_data array (since Michael used them the other way round :/)
    	    _pixel_set[x][y] = true;
	    // increase number of set pixels by 1
	    _fullFrameSet++;
    	}
    }

    // assign pixel_data given to member variable (copy the content)
    _pixel_data = pixel_data;
}

void Frame::StackFrame(FrameArray<int> pixel_data){
    // stack the 2d array on top of the current frame
    
    // loop over all elements of _pixel_data and add pixel_data each time
    for(std::size_t x = 0; x < _pix_per_dimension; x++){
	for(std::size_t y = 0; y < _pix_per_dimension; y++){
	    // note: IT MIGHT BE that due to the stupidity that is TOS
	    // one might need to reverse the ordering of x and y for the 
	    // pixel_data array (since Michael used them the other way round :/)
	    _pixel_data[x][y] += pixel_data[x][y];
	    // set this pixel as set if it wasn't already
	    _pixel_set[x][y] = true;
	}
    }
}

void Frame::SetMask(FrameArray<bool> mask_array){
    for(std::size_t x = 0; x < _pix_per_dimension; x++){
	for(std::size_t y = 0; y < _pix_per_dimension; y++){
	    _mask_data[x][y] = mask_array[x][y];
	}
    }
}

void Frame::SetPartialFrame(FrameArray<int> pixel_data, 
			    int x_start,
			    int x_step_size,
			    int y_start,
			    int y_step_size, 
			    bool convert_from_LFSR){
    // this function sets the data of pixel_data to this frame
    // by starting at x_start, y_start and going in steps of x_step_size and
    // y_step_size

    // we store the input x_start etc., variables in the _lastPFrame_x_start etc
    _lastPFrame_x_start     = x_start;
    _lastPFrame_x_step_size = x_step_size;
    _lastPFrame_y_start     = y_start;
    _lastPFrame_y_step_size = y_step_size;

    // reset the last partial frame variables
    ResetLastPFrameVariables();

    std::map<std::string, double> var_map;
    var_map = CalcSumHitsMeanVar(pixel_data,
				 true,
				 x_start,
				 x_step_size,
				 y_start,
				 y_step_size,
				 convert_from_LFSR);

    std::cout << "var map " << var_map["mean"] << std::endl;

    _lastPFrameHits     = var_map["hits"];
    _lastPFrameSet      = var_map["set"];    
    _lastPFrameSum      = var_map["sum"];
    _lastPFrameMean     = var_map["mean"];
    _lastPFrameVariance = var_map["var"];

    // std::cout << "lastPFrameMean "      << _lastPFrameMean 
    // 	      << " lastPFrameSum "      << _lastPFrameSum 
    // 	      << " lastPFrameHits "     << _lastPFrameHits
    // 	      << " lastPFrameVariance " << _lastPFrameVariance
    // 	      << std::endl;
}


void Frame::ConvertFullFrameFromLFSR(){
    // convert the whole frame from LFSR values to
    // normal pixel values
    // done by calling ConvertFrameFromLFSR with 0 values
    ConvertFrameFromLFSR(0, 1, 0, 1);    
}


void Frame::ConvertFrameFromLFSR(int x_start,
				 int x_step_size,
				 int y_start,
				 int y_step_size){
    // NOTE: in case this function is supposed to be used for partial frames,
    //       which were added previously, instead of calling this function
    //       the supposed way is to set the convert_from_LFSR flag in the SetPartialFrame()
    //       call, which will do the conversion while setting the frame
    // using the LFSR look up table, we convert the pixel data to normal
    // pixel data
    // x_offset can be used to use the same offset in x direction as is done
    // for the CTPR (if we're using test pulses as in case of SCurves)
    int pix = 0;
    int pix_value = 0;

    if (_LFSR_set == false){
	// create the LFSR lookup table, if it wasn't calculated yet
	CreateLFSRLookUpTable();
    }

    for(std::size_t x = x_start; x < _pix_per_dimension; x += x_step_size){
	for(std::size_t y = y_start; y < _pix_per_dimension; y += y_step_size){
	    pix       = _pixel_data[x][y];
	    pix_value = _LFSR_LookUpTable[pix];
	    if(pix_value >= 0 &&
	       pix_value != 11810){
		_pixel_data[x][y] = pix_value;
	    }
	    else{
		_pixel_data[x][y] = 0;
	    }
	}
    }
}

void Frame::CreateLFSRLookUpTable(){
    // this private function is used to create this objects' own LFSR lookup table

    int i = 0;
    int loop = 0;
    int lfsr = 0;
    int linear = 0;

    for(i = 0; i < 16384; ++i){
	// initialize look up table to 0
	_LFSR_LookUpTable[i]=0;
    }

    // set lfsr to start value of 3FFF (why this value?)
    lfsr=0x3FFF;

    for(loop = 0; loop < 11811; loop++)
    {
	// now we run over all possible values for the pixels (counting from 0 to 11810)
	// and assign each value (loop) for the correct element in the look up table
        _LFSR_LookUpTable[lfsr] = loop;
        linear = 0;
        if( (lfsr & 1) != ((lfsr >> 13) & 1)){
	    linear = 1;
	}
        lfsr = ((lfsr << 1) + linear) & 0x3FFF;
    }

    std::fstream outfile;
    outfile.open("/home/schmidt/lfsr.txt", std::fstream::out);

    outfile << "# i \t LFSR value" << std::endl;
    for(i = 0; i < 16384; ++i){
	// initialize look up table to 0
	outfile << i << "\t" << _LFSR_LookUpTable[i] << std::endl;
    }

    // now _LFSR_set flag to true (have calculated values)
    _LFSR_set = true;
}

int Frame::ConvertPixelFromLFSR(int pseudo_pix_value){
    // function which converts a single pixel from LFSR values to real pixel values
    // using own member variable for LFSR lookup table 
    // provides check for whether _LFSR_LookUpTable was set yet
    int pix_value = 0;
    if(_LFSR_set == false){
	CreateLFSRLookUpTable();
    }
    
    pix_value = _LFSR_LookUpTable[pseudo_pix_value];
    return pix_value;
}


void Frame::CalcFullFrameVars(int x_offset){
    // this function simply calculates the _fullFrame variables, based on the
    // current _pixel_data array

    // first reset the full frame variables
    ResetFullFrameVariables();

    std::map<std::string, double> var_map;
    // in case of calculating the full frame variables, we simply hand the member variable
    // to the helper function
    var_map = CalcSumHitsMeanVar(_pixel_data, false, x_offset);
    
    _fullFrameHits     = var_map["hits"];
    _fullFrameSet      = var_map["set"];
    _fullFrameSum      = var_map["sum"];
    _fullFrameMean     = var_map["mean"];
    _fullFrameVariance = var_map["var"];
}

std::map<std::string, double> Frame::CalcSumHitsMeanVar(FrameArray<int> pixel_data,
							bool pFrameFlag,
							int x_start,
							int x_step_size,
							int y_start,
							int y_step_size,
							bool convert_from_LFSR){
    // inputs:
    //   FrameArray<int> pixel_data: the frame array for which the values are to be calculated
    //   bool pFrameFlag: a flag, which decides whether we need to set the _pixel_data array
    //                    based on the pixel_data array (to add partial frames)

    // this function performs the actual calculation of mean, sum, hits, variance
    // of (parts) of a frame array
    int sum  = 0;
    int set  = 0;
    int hits = 0;
    int pix_value = 0;
    double mean = 0.0;
    double var  = 0.0;
    double M2   = 0.0;
    int ignore_max_value = 11810;
    for(std::size_t x = x_start; x < _pix_per_dimension; x += x_step_size){
	for(std::size_t y = y_start; y < _pix_per_dimension; y += y_step_size){
	    // NOTE: see note from StackFrame
	    // TODO: CHECK HYPOTHESIS: not necessary to check for mask array! 
	    //       if mask was set on chip, why would I need to use a software
	    //       mask too?
	    // if we check for the max values and ignore it
	    if (convert_from_LFSR == false){
		pix_value = pixel_data[x][y];
	    }
	    else{
		// in this case convert the current value from pseudo random
		// value to real pixel value
		pix_value = ConvertPixelFromLFSR(pixel_data[x][y]);
	    }

	    // NOTE: left in here for now to debug CTPR stupidity with shifts etc
	    // if (pix_value > 12000){
	    //  	std::cout << "pix value is over 900 " << x << "\t" << y << std::endl;
	    // 	std::cout << "mean is " <<  mean << " for pix value " << pix_value
	    // 		  << " at pos " << x << " / " << y << std::endl;
	    // }
	    assert(pix_value <= ignore_max_value);
	    //if (pFrameFlag == true){
	    //  std::cout << "pix value " << pix_value << std::endl;
	    //}

	    if( ( ((pFrameFlag == true) && (pix_value != 0)) ||
		  ((pFrameFlag == false) && (_pixel_set[x][y] == true)) ) &&
		(pix_value != ignore_max_value) &&
		(_mask_data[x][y] == false) ){
		// if pFrameFlag is true we set a partial frame: in that case we wish to
		// have only pixels, which are 0
		// if pFrameFlag is false we calculate full frame variables. In that case
		// it's fine, if a pixel value is 0, we only care that the pixel was
		// set at some point (even if to 0 by initialization!)
		double delta  = 0;
		double delta2 = 0;
		if (pix_value > 0){
		    hits++;
		}
		// increase number of set pixels by 1 regardless of value
		set++;
		delta  = pix_value - mean;
		// we use number of set pixels instead of actual hits for calculation
		// of mean!
		mean  += delta / double(set);
		delta2 = pix_value - mean;
		M2    += delta * delta2;
		
		sum += pix_value;
		if (pFrameFlag == true){
		    // in case we run over a partial frame, we want to set the 
		    // pixels of the pixel_data array in the _pixel_data array
		    _pixel_data[x][y] = pix_value;
		}
	    }
	    if (pFrameFlag == true){
		// in case we run over a partial frame, we want to set the 
		// as `set` pixels, i.e. their values are written (even in case they
		// were 0!)
		_pixel_set[x][y] = true;
	    }
	}
    }

    // use number of set pixels as basis for calculation of variance
    if(set > 1){
	// mean is already calculated
	// calculate variance
	var = M2 / (set - 1);	
    }
    else{
	var = 0;
    }

    // NOTE: the following can be used to analyze weird frames with huge variances
    // if (var > 10000){
    // 	std::cout << "WARNING, WARNING, variance is huge : " << var << "\n"
    // 		  << "M2 " << M2 << "\n"
    // 		  << "hits " << hits << "\n"
    // 		  << "\n dumping to file var_huge.txt"
    // 		  << std::endl;
    // 	std::string filename = "tmp/framedumps/var_huge.txt";
    // 	DumpFrameToFile(filename);
    // }
    
    // now pack the sum, hits, mean and variance into a map and hand it back to the
    // caller
    std::map<std::string, double> var_map;

    var_map["sum"]  = sum;
    var_map["set"]  = set;    
    var_map["hits"] = hits;
    var_map["mean"] = mean;
    var_map["var"]  = var;

    return var_map;
}

int Frame::CalcFrameDifference(FrameArray<int> pixel_data, bool convert_from_LFSR){
    // function which calculates the difference between the current _pixel_data
    // array and the given pixel_data
    // NOTE: it would be possible to use set partial frame and calc sum hit means for
    // this purpose, but that would change the Frames _pixel_data. Since we might want
    // to compare the _pixel_data with several different pixel_data (e.g. in case of
    // CheckOffset), we refrain from using that method here
    // returns the number of different pixels in the two frames, not the total
    // difference between the two by adding the diffs.

    if(convert_from_LFSR == true and _LFSR_set == false){
	// in case we need to convert pixel_data from LFSR to pix values and
	// LFSR was not calculate lookup table
	CreateLFSRLookUpTable();
    }

    int diff_val = 0;
    int errors = 0;
    // variable storing pixel of internal and external pixel_data array
    int pix_ext = 0;
    int pix_int = 0;
    for(std::size_t x = 0; x < _pix_per_dimension; x++){
	for(std::size_t y = 0; y < _pix_per_dimension; y++){
	    pix_ext = pixel_data[x][y];
	    pix_int = _pixel_data[x][y];
	    if(convert_from_LFSR == true){
		// in this case convert the given pixel
		// value first to actual count values
		pix_ext = _LFSR_LookUpTable[pix_ext];
	    }
	    // now calculate difference of pix_int and pix_ext
	    // using absolute value of both (and its difference), since  we want
	    // to make sure differences cannot sum to 0!
	    diff_val = std::abs( std::abs(pix_int) - std::abs(pix_ext) );
	    if(diff_val > 0){
		errors++;
	    }
	}
    }

    return errors;
}

int Frame::DumpFrameToFile(std::string filename){
    // this function simply dumps the current frame to a file
    // to be plotted

    //int ignore_max_value = 11810;
    int pix_value;
    std::fstream outfile;
    outfile.open(filename, std::fstream::out);
    if (!outfile.fail()){
        for(std::size_t y = 0; y < _pix_per_dimension; y++){
	    for(std::size_t x = 0; x < _pix_per_dimension; x++){
		pix_value = _pixel_data[x][y];
		outfile << pix_value << "\t" << std::flush;
		// if (pix_value != ignore_max_value) {
		//     outfile << pix_value << "\t" << std::flush;
		// }
		// else{
		//     outfile << 0 << "\t" << std::flush;
		// }
	    }
	    outfile << std::endl;
        }
        outfile.close();
	return 0;
    }
    else{
	return -1;
    }
}


int Frame::GetLastPFrameHits(){
    // returns the hits of the last partial frame, which was set
    return _lastPFrameHits;
}

int Frame::GetLastPFrameSet(){
    // returns the number of pixels set in the last partial frame
    return _lastPFrameSet;
}

double Frame::GetLastPFrameMean(){
    // returns the mean of the last partial frame, which was set
    return _lastPFrameMean;
}

int Frame::GetLastPFrameSum(){
    // returns the sum of the last partial frame, which was set
    return _lastPFrameSum;
}

double Frame::GetLastPFrameVariance(){
    // returns the variance of the last partial frame, which was set
    return _lastPFrameVariance;
}

int Frame::GetFullFrameHits(){
    // returns the hits of the last partial frame, which was set
    return _fullFrameHits;
}

double Frame::GetFullFrameMean(){
    // returns the mean of the last partial frame, which was set
    return _fullFrameMean;
}

int Frame::GetFullFrameSum(){
    // returns the sum of the last partial frame, which was set
    return _fullFrameSum;
}

int Frame::GetFullFrameSet(){
    // returns the number of pixels set on the whole frame
    return _fullFrameSet;
}

double Frame::GetFullFrameVariance(){
    // returns the variance of the last partial frame, which was set
    return _fullFrameVariance;
}

// void Frame::CalcSumHitsMean(bool lastPFrameFlag,
// 			    bool ignore_max_flag,
// 			    int x_start,
// 			    int x_step_size,
// 			    int y_start,
// 			    int y_step_size){
//     // THIS FUNCTION IS DEPRECATED AND WILL BE TAKEN OUT SOON
//     // this function calculates the sum, hits and mean value of all
//     // pixels of the current frame_data
//     // arguments are not required. Defaults to full frame
//     // TODO: check whether this function needs to have the input arguments it has!
//     //       should only be needed for partial frame analysis, but that is done
//     //       while adding partial frames

//     int ignore_max_value = 11810;

//     _fullFrameSum  = 0;
//     _fullFrameMean = 0;
//     _fullFrameHits = 0;

//     double M2 = 0;

//     for(int x = x_start; x < _pix_per_dimension; x += x_step_size){
// 	for(int y = y_start; y < _pix_per_dimension; y += y_step_size){
// 	    // NOTE: see note from StackFrame
// 	    if(ignore_max_flag == true){
// 		// if we ignore max values
// 		if( (_pixel_data[x][y] != 0) && 
// 		    (_pixel_data[x][y] != ignore_max_value) ){
// 		    double delta = 0;
// 		    // 
// 		    _fullFrameHits++;



// 		    _fullFrameHits++;
// 		    delta  = _pixel_data[x][y] - _fullFrameMean;
// 		    _fullFrameMean += delta / _fullFrameHits;

// 		    M2 += delta * (_pixel_data[x][y] - _fullFrameMean);

// 		    //_fullFrameSum += pixel_data[x][y];
// 		    _fullFrameSum += _pixel_data[x][y];



// 		}
// 	    }
// 	    else{
// 		// if we do not ignore max values 
// 		if( _pixel_data[x][y] != 0 ){
// 		    _fullFrameSum += _pixel_data[x][y];
// 		    _fullFrameHits++;
// 		}
// 	    }
// 	}
//     }
    
//     // and now calculate mean
//     if (_fullFrameHits > 1){
// 	// mean already calculated
// 	//_fullFrameMean = _fullFrameSum / _fullFrameHits;
// 	// calculate variance
// 	_fullFrameVariance = M2 / (_fullFrameHits - 1);	
//     }
//     else{
// 	_fullFrameVariance = 0;
//     }
// }

// double Frame::CalcVariance(bool lastPFrameFlag,
// 			   int x_start, 
// 			   int x_step_size,
// 			   int y_start,
// 			   int y_step_size){
//     // THIS FUNCTION IS DEPRECATED AND WILL BE TAKEN OUT SOON
//     // this function calculates the variance of the current frame_data
//     // arguments are not required. Defaults to full frame
//     // inputs:
//     //   bool LastPFrameFlag: if this flag is set to true, we will calculate the
//     //                        variance based on the _lastPFrame member variables
//     //                        in this case x_start etc are ignored and the lastPFrame equivalents
//     //                        are used
//     //                        otherwise full frame variables are used

//     if (lastPFrameFlag == true){
// 	// in both cases set used variance variable to 0
// 	_lastPFrameVariance = 0.0;
//     }
//     else{
//     // in case we calc variance for whole frame, need to check, whether 
//     // hits, mean and sum were calculated for full frame yet, if not, 
//     // calculate now
// 	if( (_fullFrameSum  == 0) &&
// 	    (_fullFrameHits == 0) &&
// 	    (_fullFrameMean == 0) ){
// 	    _fullFrameVariance = 0.0;
// 	    CalcSumHitsMean(false);
// 	}
//     }

//     for(int x = x_start; x < _pix_per_dimension; x += x_step_size){
// 	for(int y = y_start; y < _pix_per_dimension; y += y_step_size){
// 	    // NOTE: see note from StackFrame
// 	    // TODO: check if we need to check for pixel values != 0!
// 	    if (lastPFrameFlag == true){
// 		if (_lastPFrameHits > 1){
// 		    double pix_minus_mean   = _pixel_data[x][y] - _lastPFrameMean;
// 		    _lastPFrameVariance += (pix_minus_mean * pix_minus_mean) / (_lastPFrameHits - 1);
// 		}
// 	    }
// 	    else{
// 		if (_fullFrameHits > 1){
// 		    double pix_minus_mean   = _pixel_data[x][y] - _fullFrameMean;
// 		    _fullFrameVariance  += (pix_minus_mean * pix_minus_mean) / (_fullFrameHits - 1);
// 		}
// 	    }
// 	}
//     }

//     if (lastPFrameFlag == true){
// 	return _lastPFrameVariance;
//     }
//     else{
// 	return _fullFrameVariance;
//     }
// }

// double Frame::CalcLastPFrameVariance(){
//     // THIS FUNCTION IS DEPRECATED AND WILL BE TAKEN OUT SOON!
//     // wrapper function around CalcVariance, which simply calls CalcVariance
//     // with the parameters of the last partial frame call and lastPFrameFlag == true
    
//     return CalcVariance(true,
// 			_lastPFrame_x_start, 
// 			_lastPFrame_x_step_size, 
// 			_lastPFrame_y_start,
// 			_lastPFrame_y_step_size);
// }





// body from set partial frame
    // for(int x = x_start; x < _pix_per_dimension; x += x_step_size){
    // 	for(int y = y_start; y < _pix_per_dimension; y += y_step_size){
    // 	    // NOTE: see note from StackFrame
    // 	    // TODO: CHECK HYPOTHESIS: not necessary to check for mask array! 
    // 	    //       if mask was set on chip, why would I need to use a software
    // 	    //       mask too?
    // 	    if(ignore_max_flag == true){
    // 		// if we check for the max values and ignore it
    // 		if( (pixel_data[x][y] != 0) &&
    // 		    (pixel_data[x][y] != ignore_max_value ) ){
    // 		    _pixel_data[x][y] = pixel_data[x][y];

    // 		    double delta = 0;

    // 		    //std::cout << "x " << x << " y " << y << " val " << pixel_data[x][y] << std::endl;
    // 		    _lastPFrameHits++;
    // 		    delta  = pixel_data[x][y] - _lastPFrameMean;
    // 		    _lastPFrameMean += delta / _lastPFrameHits;

    // 		    M2 += delta * (pixel_data[x][y] - _lastPFrameMean);

    // 		    _lastPFrameSum += pixel_data[x][y];

    // 		}
    // 	    }
    // 	    else{
    // 		// without the max value flag, only add if value not zero...
    // 		if (pixel_data[x][y] != 0){
    // 		    _pixel_data[x][y] = pixel_data[x][y];

    // 		    _lastPFrameSum += pixel_data[x][y];
    // 		    _lastPFrameHits++;
    // 		}
    // 	    }
    // 	}
    // }


