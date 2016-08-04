#ifndef HV_CHANNEL_HPP
#define HV_CHANNEL_HPP 1
// this is the header for the channel class
// the channel class is an abstract class for an iseg HV channel
// it is derived from the HV_Obj, in order to be able to set channel properties
// by calling the necessary functions from the parent class

// HV macros
// TODO: understand if necessary here as well
#define DEFAULT_HV_SLEEP_TIME                                     10 // in milli seconds


#include "const.h"
#include "vmemodule.h"
#include "tosCommandCompletion.hpp"

// C++
#include <set>
#include <list>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

class hvChannel
{
public:
    // ##################################################
    // public member variables
    // ##################################################


    // creator and destructor
    hvChannel(CVmeModule *hvModule, std::string channelName, int channelNumber); 
    ~hvChannel();

    // function to get channel number
    int getChannelNumber() { return _channelNumber; };
    std::string getChannelName() { return _channelName; };

    // ##################################################
    // convenience functions regarding channels
    // ##################################################
    
    // the update channel function simply calls functions to update
    // the channel event status, channel status, the current voltage
    // and current
    void updateChannel();
    // if channel was used before, but not ramped down upon deletion of object
    // use this function to reconnect
    void ReconnectToChannel();

    // function to change bool variable which decides whether to shut down the
    // channel when we delete the channel object
    void SetRampDownUponDelete(bool rampDownUponDelete);

    bool setVoltage(float voltage);
    bool setCurrent(float current);

    bool setVoltageNominal(float voltage);
    bool setCurrentNominal(float current);
    
    bool setVoltageBound(float voltageBound);
    
    // get voltage and current only returns the set voltages and currents!
    float getVoltage();
    float getCurrent();

    // function to turn channel on and off
    // both return a bool to tell, whether action was successful
    bool turnOn();
    bool turnOff();
    // and function to switch on <-> off
    bool switchOnOff();

    // these two functions get the currently measured voltages and currents
    float getVoltageMeasured();
    float getCurrentMeasured();

    // functions to get nominal voltage and current
    float getVoltageNominal();
    float getCurrentNominal();    

    // function, which prints channel name, number, currently measured voltage
    // and set voltage
    // print channel name
    void printChannelName();
    // print channel name and on state
    void printChannelIsOn();
    // print voltage measured
    void printVoltageMeasured();
    // function, which prints channel name, number, currently measured current
    // and set current
    void printCurrentMeasured();
    // function to print the channel status
    void printChannelStatus();
    // function to print the channel event status
    void printChannelEventStatus();
    // function which calls all printing functions
    void printChannel();

    // functions to set and get channelEventMask
    bool setChannelEventMask(std::set<std::string> eventMaskList);
    // TODO: return std::set<std::string> instead!!!
    ChEventMaskSTRUCT getChannelEventMask();

    // function to clear channel event status
    bool clearChannelEventStatus(bool forceEmpty = false);

    // returns true or false, depending on whether channel is on
    bool isOn();
    // returns true or false, depending on whether channel is ramping
    bool isRamping();
    // returns true or false, depending on whether channel is voltage controlled
    bool isVoltageControlled();
    // returns true or false, depending on whether the channel has finished ramping
    bool endOfRamping();
    // returns true or false, depending on whether channel has finished ramping down
    bool finishedRampingDown();

    // function which checks whether our voltage is in bounds
    bool voltageInBounds();

    // function which checks, if channel is on, voltage controlled, and prints voltage
    bool onVoltageControlledRamped(bool printFlag = true);

    // function to check whether any bit of channel status is non zero
    bool channelStatusNonZero();
    

    // sleep the thread for _sleepTime in milli seconds
    void sleepChannel();


private:
    // an HV channel needs to own an HV_Obj, in order to be able to call
    // the functions to write to the ISEG module
    CVmeModule *_hvModule;

    // string identifier of this channel
    std::string _channelName;
    // channel number as int
    int _channelNumber;

    // 'low level' member variables regarding the status of
    // the channel. Not publich, because it's not supposed to 
    // the available to function / class owning a channel
    // instead, if properties of eventStatus of Status are needed
    // a convenience function is available (if not: write it yourself!)
    ChEventStatusSTRUCT _channelEventStatus;
    ChStatusSTRUCT _channelStatus;
    ChEventMaskSTRUCT _channelEventMask;
    
    // set containing all strings corresponding to masks, which are allowed
    // to be given to setChannelEventMask
    std::set<std::string> _allowedMasks;

    // float variables for currently set voltage and current
    // private, because we do not want to be able to set a new voltage,
    // but not have it set in the module!
    float _voltageSet;
    float _currentSet;
    
    float _voltageNominal;
    float _currentNominal;

    float _voltageMeasured;
    float _currentMeasured;

    float _voltageBound;

    // variables for status
    bool _isOn;
    bool _isRamping;
    bool _isVoltageControlled;

    // and variables event status
    bool _endOfRamping;
    
    // variable which stores default time to sleep
    int _sleepTime;

    // flag which decides, if we ramp down channel upon deletion
    bool _rampDownUponDelete;
    
};

#endif
