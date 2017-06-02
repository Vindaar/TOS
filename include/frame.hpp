// this header implements a basic frame read from a timepix chip
//     - by Sebastian Schmidt

#ifndef _FRAME_HPP
#define _FRAME_HPP 1

#include "protocol_constants.hpp"

#include <array>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>


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
    Frame(FrameArray<int>& pixel_data);
    
    
    // destructor
    ~Frame();

    FrameArray<int> GetPixelData();

    // --------------------------------------------------
    // functions dealing with frames
    // --------------------------------------------------

    // stack frame, by simply adding all pixels of pixel_data to 
    // current frame
    // note: <int> parameter, since we use an int typed FrameArray for our pixel
    // data
    void StackFrame(FrameArray<int> pixel_data);
    // set frame (in constrast to stack frame, which adds the pixel values, this
    // function replaces the previous _pixel_data array
    void SetFrame(FrameArray<int>& pixel_data);
    // function to add a partial frame to the object
    // i.e. to build a full frame from many partial frames
    // also calculates sum, hits, mean, variance of this partial frame
    void SetPartialFrame(FrameArray<int> pixel_data,
			 int x_start, 
			 int x_step_size,
			 int y_start,
			 int y_step_size,
			 bool ignore_max_flag = true,
			 bool convert_from_LFSR = false);


    // function to set the _mask_data array from the mask array of the timepix class
    void SetMask(FrameArray<bool> mask_array);

    // this function converts the frame data from LFSR to normal pixel data
    // using the look up table
    void ConvertFrameFromLFSR(int x_start, int x_step_size, int y_start, int y_step_size);
    // wrapper around convert frame from lfsr to convert whole frame
    void ConvertFullFrameFromLFSR();
    // function to convert a single value from pseudo to real pixel value
    int ConvertPixelFromLFSR(int pseudo_pix_value);

    int DumpFrameToFile(std::string filename);
    

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

    // function to calculate sum, hits, mean and variance for current frame
    void CalcFullFrameVars(int x_offset = 0);

    
    // FUNCTION DEPRECATED
    // double CalcVariance(bool lastPFrameFlag = true,
    // 			int x_start = 0, 
    // 			int x_step_size = 1,
    // 			int y_start = 0,
    // 			int y_step_size = 1);
    
    // // wrapper function around CalcVariance, which simply calls CalcVariance
    // // with the parameters of the last partial frame call and lastPFrameFlag == true
    // double CalcLastPFrameVariance();

    // deprecated
    // void CalcSumHitsMean(bool lastPFrame,
    // 			 bool ignore_max_flag = true,
    // 			 int x_start = 0, 
    // 			 int x_step_size = 1,
    // 			 int y_start = 0,
    // 			 int y_step_size = 1);

    // functions which just return the private member variables storing the 
    // sum, hit and mean values
    int GetLastPFrameSum();
    int GetLastPFrameHits();
    int GetLastPFrameMean();
    int GetLastPFrameVariance();
    int GetFullFrameSum();
    int GetFullFrameHits();
    int GetFullFrameMean();
    int GetFullFrameVariance();


    // NOTE: THIS IS SUPPOSED TO BE IN PRIVATE. HERE FOR DEBUGGING. IF YOU
    // READ THIS, PUT IT BACK THERE!!!
    
    void ResetMemberVariables();
    
private:

    // --------------------------------------------------
    // general functions
    // --------------------------------------------------


    void ResetLastPFrameVariables();
    void ResetFullFrameVariables();
    // function to create own lfsr lookup table
    void CreateLFSRLookUpTable();

    // function for internal calculation of frame variables
    std::map<std::string, double> CalcSumHitsMeanVar(FrameArray<int> pixel_data,
						     bool pFrameFlag,
						     int x_start = 0, 
						     int x_step_size = 1,
						     int y_start = 0,
						     int y_step_size = 1,
						     bool convert_from_LFSR = false);


    // size of the frame per dimension
    const int _pix_per_dimension;

    // note: zero initialization in this way is the correct way to do it.
    // however, gcc gives a warning about it
    // the 2D vector, which contains the pixel data
    FrameArray<int>  _pixel_data{};
    // 2D mask array
    FrameArray<bool> _mask_data{};
    // LFSR look up table for full frame readout (pseudo random number counting)
    // initialized to 0
    int _LFSR_LookUpTable[16384] = {};
    bool _LFSR_set;
    
    // total frame variables
    int _fullFrameSum;
    double _fullFrameMean;
    int _fullFrameHits;
    double _fullFrameVariance;
    
    // last partial frame variables
    int _lastPFrameSum;
    double _lastPFrameMean;
    int _lastPFrameHits;
    double _lastPFrameVariance;

    // VARIABLES DEPRECATED
    // variables which store the last partial frame parameters
    int _lastPFrame_x_start; 
    int	_lastPFrame_x_step_size;
    int	_lastPFrame_y_start;
    int	_lastPFrame_y_step_size;

};

#endif
