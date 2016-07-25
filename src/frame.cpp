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
    _fullFrameSum      = 0;
    _fullFrameMean     = 0;
    _fullFrameHits     = 0;
    _fullFrameVariance = 0;

}

void Frame::ResetLastPFrameVariables(){
    // set last partial frame variables to 0
    // zero initialize last partial frame member variables
    _lastPFrameSum      = 0;
    _lastPFrameMean     = 0;
    _lastPFrameHits     = 0;
    _lastPFrameVariance = 0;
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
			    bool lfsr_flag){
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

    int lfsr_ignore_value = 11810;
    
    for(int x = x_start; x < (_pix_per_dimension / x_step_size); x++){
	for(int y = y_start; y < (_pix_per_dimension / y_step_size); y++){
	    // NOTE: see note from StackFrame
	    // TODO: CHECK HYPOTHESIS: not necessary to check for mask array! 
	    //       if mask was set on chip, why would I need to use a software
	    //       mask too?
	    if(lfsr_flag == true){
		// if we check for lfsr values (in case data is given in 
		// lfsr values
		if( (pixel_data[x][y] != 0) &&
		    (pixel_data[x][y] != lfsr_ignore_value ) ){
		    _pixel_data[x][y] = pixel_data[x][y];
		    //std::cout << "val " << pixel_data[y][x] << std::endl;
		    //_pixel_data[x][y] = pixel_data[y][x];
		    
		    _lastPFrameSum += pixel_data[x][y];
		    //_lastPFrameSum += pixel_data[y][x];
		    _lastPFrameHits++;

		}
	    }
	    else{
		// even without lfsr flag, only add if value not zero...
		if (pixel_data[x][y] != 0){
		    _pixel_data[x][y] = pixel_data[x][y];

		    _lastPFrameSum += pixel_data[x][y];
		    _lastPFrameHits++;
		}
	    }
	}
    }

    if(_lastPFrameHits != 0){
	_lastPFrameMean = _lastPFrameSum / _lastPFrameHits;
    }
    else{
	_lastPFrameMean = 0;
    }

    std::cout << "lastPFrameMean "  << _lastPFrameMean 
	      << " lastPFrameSum "  << _lastPFrameSum 
	      << " lastPFrameHits " << _lastPFrameHits
	      << std::endl;
}

void Frame::CalcSumHitsMean(bool lastPFrameFlag,
			    bool lfsr_flag,
			    int x_start,
			    int x_step_size,
			    int y_start,
			    int y_step_size){
    // this function calculates the sum, hits and mean value of all
    // pixels of the current frame_data
    // arguments are not required. Defaults to full frame
    // TODO: check whether this function needs to have the input arguments it has!
    //       should only be needed for partial frame analysis, but that is done
    //       while adding partial frames

    int lfsr_ignore_value = 11810;

    _fullFrameSum = 0;
    _fullFrameMean = 0;
    _fullFrameHits = 0;

    for(int x = x_start; x < (_pix_per_dimension / x_step_size); x++){
	for(int y = y_start; y < (_pix_per_dimension / y_step_size); y++){
	    // NOTE: see note from StackFrame
	    if(lfsr_flag == true){
		// if we check for lfsr values (in case data is given in 
		// lfsr values
		if( (_pixel_data[x][y] != 0) && 
		    (_pixel_data[x][y] != lfsr_ignore_value) ){
		    //std::cout << "x " << x << " y " << y << std::endl; 
		    _fullFrameSum += _pixel_data[x][y];
		    _fullFrameHits++;
		}
	    }
	    else{
		if( _pixel_data[x][y] != lfsr_ignore_value ){
		    _fullFrameSum += _pixel_data[x][y];
		    _fullFrameHits++;
		}
	    }
	}
    }
    
    // and now calculate mean
    if (_fullFrameHits > 0){
	_fullFrameMean = _fullFrameSum / _fullFrameHits;
    }
    //std::cout << "fullframeMean " << _fullFrameMean << std::endl;
}

double Frame::CalcVariance(bool lastPFrameFlag,
			   int x_start, 
			   int x_step_size,
			   int y_start,
			   int y_step_size){
    // this function calculates the variance of the current frame_data
    // arguments are not required. Defaults to full frame
    // inputs:
    //   bool LastPFrameFlag: if this flag is set to true, we will calculate the
    //                        variance based on the _lastPFrame member variables
    //                        in this case x_start etc are ignored and the lastPFrame equivalents
    //                        are used
    //                        otherwise full frame variables are used

    if (lastPFrameFlag == true){
	// in both cases set used variance variable to 0
	_lastPFrameVariance = 0.0;
    }
    else{
    // in case we calc variance for whole frame, need to check, whether 
    // hits, mean and sum were calculated for full frame yet, if not, 
    // calculate now
	if( (_fullFrameSum  == 0) &&
	    (_fullFrameHits == 0) &&
	    (_fullFrameMean == 0) ){
	    _fullFrameVariance = 0.0;
	    CalcSumHitsMean(false);
	}
    }

    for(int x = x_start; x < (_pix_per_dimension / x_step_size); x++){
	for(int y = y_start; y < (_pix_per_dimension / y_step_size); y++){
	    // NOTE: see note from StackFrame
	    // TODO: check if we need to check for pixel values != 0!
	    if (lastPFrameFlag == true){
		if (_lastPFrameHits > 1){
		    double pix_minus_mean   = _pixel_data[x][y] - _lastPFrameMean;
		    _lastPFrameVariance += (pix_minus_mean * pix_minus_mean) / (_lastPFrameHits - 1);
		}
	    }
	    else{
		if (_fullFrameHits > 1){
		    double pix_minus_mean   = _pixel_data[x][y] - _fullFrameMean;
		    _fullFrameVariance  += (pix_minus_mean * pix_minus_mean) / (_fullFrameHits - 1);
		}
	    }
	}
    }

    if (lastPFrameFlag == true){
	return _lastPFrameVariance;
    }
    else{
	return _fullFrameVariance;
    }
}

double Frame::CalcLastPFrameVariance(){
    // wrapper function around CalcVariance, which simply calls CalcVariance
    // with the parameters of the last partial frame call and lastPFrameFlag == true
    
    return CalcVariance(true,
			_lastPFrame_x_start, 
			_lastPFrame_x_step_size, 
			_lastPFrame_y_start,
			_lastPFrame_y_step_size);
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
