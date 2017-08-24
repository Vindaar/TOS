//TODO: 
//*Find a better name!
//*Lose the VME since this functions should also work for the dummy and gpib FADC
//*Test this functions with the dummy class

#ifndef V1729a_HighLevelFunctions_VME_H
#define V1729a_HighLevelFunctions_VME_H 1


//PROJECT FILES
#include "V1729a.h"


// STD C++
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
//#include <unistd.h>

#define DEFAULT_FADC_SLEEP_TIME                10 // milliseconds

//class HighLevelFunction_VME : public HighLevelFunction{
class HighLevelFunction_VME{

public:
  
    //TODO: Write a load settings from file function

    /// C'tor
    HighLevelFunction_VME(V1729a* dev);

    /// D'tor
    ~HighLevelFunction_VME();



    /*********** General Functions ***********/
  


    /** Prints the actual settings of the FADC to
     * console or to a file
     **/
    void printSettings();



    /*********** Set Functions ***********/
   
  
  
    /** Since the FADC fails from time to time to set some parameters
     * to theire values, a few set functions were rewritten
     * to ensure in _nb trys, that the parameters are set in 
     * the intendet way
     **/  

    int setFrequencyH(const unsigned short& frequency);
    int setPosttrigH(const unsigned short& bits);
    int setPretrigH(const unsigned short& bits);

    /**
     *
     **/
    void preformTestMeasurement(); 



    /*********** Trigger Functions ***********/


  
    /** Trigger related "set functions" (as above)
     **/
    int setTriggerTypeH( const unsigned short& type );
    int setTriggerChannelSourceH( const unsigned short& mask );

    /** Sets the preload register for all channels to the given value and 
     * executes the load-Trigger-Threshold command. To keep track of the
     * set value its stored in the triggerThresholdRegister vector.
     * Returns 0 if succesfull, 1 otherwise. (at least it should)
     **/
    int setTriggerThresholdRegisterAll(const unsigned int threshold);

    // helper functions to convert from and to mV from fadc_ticks
    // returns trigger in mV calc'd from FADC ticks, returns int, because
    // trigger can be negative
    int calcTriggerThresholdFromTicks(unsigned int fadc_ticks);    
    int calcTriggerThresholdFromTicks(unsigned int fadc_ticks, bool bit_mode14);
    // returns FADC ticks from trigger voltage in mV
    unsigned int calcTriggerThresholdInTicks(int trigger_level);    
    unsigned int calcTriggerThresholdInTicks(int trigger_level, bool bit_mode14);

    // function to set trigger threshold in mV instead of FADC ticks
    // function overloaded, if first function is called, mode_register will
    // be read from device first, then calls 2nd function
    int setTriggerLevel(int trigger_level);
    int setTriggerLevel(int trigger_level, int mode_register);    


    ///returns the content of the triggerThresholdRegister vector
    void getTriggerThresholdRegister();


    /*********** Readout Functions ***********/



    ///correct Data for trigger position
    std::vector<std::vector<int> > correctData(std::vector<int> const& dataVec);
 

    /** print data to file (in the file the unit of time will be ns and the unit of
     * amplitude will be V
     **/
    void printDataToFile(std::vector<int> const& dataVec, std::string fileName = "rawData");


    void printDataToFile(std::vector<std::vector<int> > const& dataVec, std::string fileName = "Data");

    int getNumberOfActiveChannels(unsigned short channel_mask);

protected:



private:

    // function to wait a short time
    void sleepModule();

    ///Pointer to the device one works with
    V1729a* _currentDevice;
    ///var to set how often some functions should try something
    int const _nb; 
    ///vec to store the values of the triggerThresholdRegister (all, ch1, ch2, ch3, ch4)
    std::vector<unsigned short> _triggerThresholdRegister;

};//end of class definition



#endif
