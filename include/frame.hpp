// this header implements a basic frame read from a timepix chip
//     - by Sebastian Schmidt

#ifndef _FRAME_HPP
#define _FRAME_HPP 1

// project related files
#include "header.hpp"

#include <map>
#include <boost/any.hpp>


// define a short hand for the data type in which frames are stored
// for creation of a 2D array to store the frame data
// using a template, so that we can either store ints as well as bool etc.
template<class T>
using FrameArray =  std::array<std::array<T, PIXPD>, PIXPD>;

class Frame{

public:

    // creator
    // either we create an empty frame (0 initialized)
    Frame();
    // or we create a frame by using an existing 2D array
    Frame(FrameArray<int> pixel_data);
    
    
    // destructor
    ~Frame();

    // --------------------------------------------------
    // general functions
    // --------------------------------------------------

    void ResetMemberVariables();
    void ResetLastPFrameVariables();

    // --------------------------------------------------
    // functions dealing with frames
    // --------------------------------------------------

    // stack frame, by simply adding all pixels of pixel_data to 
    // current frame
    // note: <int> parameter, since we use an int typed FrameArray for our pixel
    // data
    void StackFrame(FrameArray<int> pixel_data);
    // function to add a partial frame to the object
    // i.e. to build a full frame from many partial frames
    void SetPartialFrame(FrameArray<int> pixel_data,
			 int x_start, 
			 int x_step_size,
			 int y_start,
			 int y_step_size,
			 bool lfsr_flag = true);
    
    

    // --------------------------------------------------
    // functions dealing with frame access
    // --------------------------------------------------

    // functions to access specific rows, columns or elements
    // access column x, return the whole column as vector
    std::vector<int> At_x(int x);
    // access row y, return whole row as vector
    std::vector<int> At_y(int y);
    // access element (x, y), return int
    int At_xy(int x, int y);
    
    
    // --------------------------------------------------
    // functions dealing w/ calculations
    // --------------------------------------------------

    double CalcVariance(bool lastPFrameFlag = true,
			int x_start = 0, 
			int x_step_size = 1,
			int y_start = 0,
			int y_step_size = 1);
    
    // wrapper function around CalcVariance, which simply calls CalcVariance
    // with the parameters of the last partial frame call and lastPFrameFlag == true
    double CalcLastPFrameVariance();

    void CalcSumHitsMean(bool lastPFrame,
			 bool lfsr_flag = true,
			 int x_start = 0, 
			 int x_step_size = 1,
			 int y_start = 0,
			 int y_step_size = 1);

    // functions which just return the private member variables storing the 
    // sum, hit and mean values
    int GetLastPFrameSum();
    int GetLastPFrameHits();
    int GetLastPFrameMean();
    int GetFullFrameSum();
    int GetFullFrameHits();
    int GetFullFrameMean();

private:

    // size of the frame per dimension
    const int _pix_per_dimension;

    // note: zero initialization in this way is the correct way to do it.
    // however, gcc gives a warning about it
    // the 2D vector, which contains the pixel data
    FrameArray<int>  _pixel_data{};
    // 2D mask array
    FrameArray<bool> _mask_data{};
    
    // total frame variables
    int _fullFrameSum;
    int _fullFrameMean;
    int _fullFrameHits;
    double _fullFrameVariance;
    
    // last partial frame variables
    int _lastPFrameSum;
    int _lastPFrameMean;
    int _lastPFrameHits;
    double _lastPFrameVariance;

    // variables which store the last partial frame parameters
    int _lastPFrame_x_start; 
    int	_lastPFrame_x_step_size;
    int	_lastPFrame_y_start;
    int	_lastPFrame_y_step_size;

};

#endif
