// this function implements a basic frame read from a timepix chip
//   - by Sebastian Schmidt

#include "frame.hpp"

Frame::Frame(): 
    _pix_per_dimension(PIXPD)
{
    // in case of the empty creator, we simply create an empty frame, 
    // which is 0 initialized
    // note: in this case the size of the timepix chip defined by a macro
    // in the timepix.hpp header is used as dimension

    // initialize member variables to zero (except dimension)
    ResetMemberVariables();
    
}

Frame::Frame(FrameArray<int> pixel_data):
    _pix_per_dimension(PIXPD)
{
    // in this case we create an empty frame first and then put in
    // the data from the pixel_data array

    // initialize member variables to zero (except dimension)
    ResetMemberVariables();

    // use the stack frame function to create the initialized frame
    StackFrame(pixel_data);

    // now our _pixel_data member variable contains the 2D array
}

Frame::~Frame(){
    // nothing to do here
}

void Frame::ResetMemberVariables(){
    // this function resets, i.e. sets to zero, all member variables

    // set the FrameArrays to 0 / false
    for(std::size_t x = 0; x < _pix_per_dimension; x++){
	for(std::size_t y = 0; y < _pix_per_dimension; y++){
	    _pixel_data[x][y] = 0;
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
    _lastPFrameVariance = 0.0;
}

void Frame::ResetFullFrameVariables(){
    // set full frame variables to 0
    _fullFrameSum      = 0;
    _fullFrameMean     = 0.0;
    _fullFrameHits     = 0;
    _fullFrameVariance = 0.0;
}


void Frame::StackFrame(FrameArray<int> pixel_data){
    // stack the 2d array on top of the current frame
    
    // loop over all elements of _pixel_data and add pixel_data each time
    for(int x = 0; x < _pix_per_dimension; x++){
	for(int y = 0; y < _pix_per_dimension; y++){
	    // note: IT MIGHT BE that due to the stupidity that is TOS
	    // one might need to reverse the ordering of x and y for the 
	    // pixel_data array (since Michael used them the other way round :/)
	    _pixel_data[x][y] += pixel_data[x][y];	    
	}
    }
}

void Frame::SetPartialFrame(FrameArray<int> pixel_data, 
			    int x_start, 
			    int x_step_size,
			    int y_start,
			    int y_step_size, 
			    bool ignore_max_flag){
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
    var_map = CalcSumHitsMeanVar(pixel_data, true, x_start, x_step_size, y_start, y_step_size);
    
    _lastPFrameHits     = var_map["hits"];
    _lastPFrameSum      = var_map["sum"];
    _lastPFrameMean     = var_map["mean"];
    _lastPFrameVariance = var_map["var"];

    // std::cout << "lastPFrameMean "      << _lastPFrameMean 
    // 	      << " lastPFrameSum "      << _lastPFrameSum 
    // 	      << " lastPFrameHits "     << _lastPFrameHits
    // 	      << " lastPFrameVariance " << _lastPFrameVariance
    // 	      << std::endl;
}

void Frame::CalcFullFrameVars(){
    // this function simply calculates the _fullFrame variables, based on the
    // current _pixel_data array

    // first reset the full frame variables
    ResetFullFrameVariables();

    std::map<std::string, double> var_map;
    // in case of calculating the full frame variables, we simply hand the member variable
    // to the helper function
    var_map = CalcSumHitsMeanVar(_pixel_data, false);
    
    _fullFrameHits     = var_map["hits"];
    _fullFrameSum      = var_map["sum"];
    _fullFrameMean     = var_map["mean"];
    _fullFrameVariance = var_map["var"];
}

std::map<std::string, double> Frame::CalcSumHitsMeanVar(FrameArray<int> pixel_data,
							bool pFrameFlag,
							int x_start,
							int x_step_size,
							int y_start,
							int y_step_size){
    // inputs:
    //   FrameArray<int> pixel_data: the frame array for which the values are to be calculated
    //   bool pFrameFlag: a flag, which decides whether we need to set the _pixel_data array
    //                    based on the pixel_data array (to add partial frames)

    // this function performs the actual calculation of mean, sum, hits, variance
    // of (parts) of a frame array
    int sum  = 0;
    int hits = 0;
    double mean = 0.0;
    double var  = 0.0;
    double M2   = 0.0;
    int ignore_max_value = 11810;
    for(int x = x_start; x < _pix_per_dimension; x += x_step_size){
	for(int y = y_start; y < _pix_per_dimension; y += y_step_size){
	    // NOTE: see note from StackFrame
	    // TODO: CHECK HYPOTHESIS: not necessary to check for mask array! 
	    //       if mask was set on chip, why would I need to use a software
	    //       mask too?
	    // if we check for the max values and ignore it
	    if( (pixel_data[x][y] != 0) &&
		(pixel_data[x][y] != ignore_max_value ) ){
		double delta = 0;
		hits++;
		delta  = pixel_data[x][y] - mean;
		mean  += delta / hits;
		M2 += delta * (pixel_data[x][y] - mean);

		sum += pixel_data[x][y];
		if (pFrameFlag == true){
		    // in case we run over a partial frame, we want to set the 
		    // pixels of the pixel_data array in the _pixel_data array
		    _pixel_data[x][y] = pixel_data[x][y];
		}
	    }
	}
    }

    if(hits > 1){
	// mean is already calculated
	// calculate variance
	var = M2 / (hits - 1);	
    }
    else{
	var = 0;
    }

    // now pack the sum, hits, mean and variance into a map and hand it back to the
    // caller
    std::map<std::string, double> var_map;

    var_map["sum"]  = sum;
    var_map["hits"] = hits;
    var_map["mean"] = mean;
    var_map["var"]  = var;

    return var_map;
}

int Frame::GetLastPFrameHits(){
    // returns the hits of the last partial frame, which was set
    return _lastPFrameHits;
}

int Frame::GetLastPFrameMean(){
    // returns the mean of the last partial frame, which was set
    return _lastPFrameMean;
}

int Frame::GetLastPFrameSum(){
    // returns the sum of the last partial frame, which was set
    return _lastPFrameSum;
}

int Frame::GetLastPFrameVariance(){
    // returns the variance of the last partial frame, which was set
    return _lastPFrameVariance;
}

int Frame::GetFullFrameHits(){
    // returns the hits of the last partial frame, which was set
    return _fullFrameHits;
}

int Frame::GetFullFrameMean(){
    // returns the mean of the last partial frame, which was set
    return _fullFrameMean;
}

int Frame::GetFullFrameSum(){
    // returns the sum of the last partial frame, which was set
    return _fullFrameSum;
}

int Frame::GetFullFrameVariance(){
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


