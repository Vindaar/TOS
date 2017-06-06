// this is the implementation of the HV channel

#include "hvInterface/hvChannel.hpp"
#include <bitset>

hvChannel::hvChannel(CVmeModule *hvModule, std::string channelName, int channelNumber)
    : _hvModule(hvModule),
      _channelName(channelName), 
      _channelNumber(channelNumber), 
      _sleepTime(DEFAULT_HV_SLEEP_TIME),
      _rampDownUponDelete(true){

    // creator of channel. Set name, number and voltages

    // initialize voltage and current to 0
    _voltageSet = 0;
    _currentSet = 0;
    // initialize nominal voltages
    _voltageNominal = 0;
    _currentNominal = 0;
    // and status structs as well
    _channelEventStatus = { 0 };
    _channelStatus      = { 0 };
    _channelEventMask   = { 0 };

}

hvChannel::~hvChannel(){
    // we check the rampDownUponDelete bool variable
    // to decide whether we ramp down upon deleting the object
    float voltageOff = 0;
    float currentOff = 0;

    // TODO: check to what value we wish to set the nominal value. Probably rather 
    // some default value
    // float voltageNominalOff = 0;
    // float currentNominalOff = 0;

    if(_rampDownUponDelete == true){
        // in this case ramp down channel
        bool good;
        good =  setVoltage(voltageOff);
        good *= setCurrent(currentOff);
        if(good == false){
            // either voltage or current could not be set
            // TODO: decide what to do
        }
       
        // setVoltageNominal(voltageNominalOff);
        // setCurrentNominal(currentNominalOff);

        // TODO: understand if it should be enough to turn off channel, once 
        // we have set the voltage and current to 0
        turnOff();
    }

}

void hvChannel::ReconnectToChannel(){
    // this function can be called to initialize the channel
    // object based on an already ramped up channel
    // i.e. we get the modules data for the channel number of this
    // channel, such that we can ramp up a channel of the module with
    // this object and then be able to stop the program completely,
    // without ramping down the channels and later reconnect to this
    // channel
    
    // reconnect simply works by updating the channel
    updateChannel();
}


void hvChannel::SetRampDownUponDelete(bool rampDownUponDelete){
    // this function allows one to set the rampDownUponDelete flag
    _rampDownUponDelete = rampDownUponDelete;    
}


// and current
void hvChannel::updateChannel(){
    // implements a whole update to the channel

    // TODO: check whether calling all these functions directly after another works
    // properly!
    
    // update voltage and current set
    _voltageSet = _hvModule->GetChannelVoltageSet(_channelNumber);
    _currentSet = _hvModule->GetChannelCurrentSet(_channelNumber);

    // update voltage and current nominal
    _voltageNominal = _hvModule->GetChannelVoltageNominal(_channelNumber);
    _currentNominal = _hvModule->GetChannelCurrentNominal(_channelNumber);

    // update measured voltage and current
    _voltageMeasured = _hvModule->GetChannelVoltageMeasure(_channelNumber);
    _currentMeasured = _hvModule->GetChannelCurrentMeasure(_channelNumber);

    // update channel, event channel status and channel event mask
    _channelEventStatus.Word = _hvModule->GetChannelEventStatus(_channelNumber);
    _channelStatus.Word      = _hvModule->GetChannelStatus(_channelNumber);
    _channelEventMask.Word   = _hvModule->GetChannelEventMask(_channelNumber);

    // update is On variable based on channel status
    _isOn                = _channelStatus.Bit.isOn;
    _isRamping           = _channelStatus.Bit.isRamping;
    _isVoltageControlled = _channelStatus.Bit.ControlledByVoltage;

    // event end of ramping
    _endOfRamping        = _channelEventStatus.Bit.EventEndOfRamping;

}

bool hvChannel::setVoltage(float voltage){
    // we set a voltage for this channel by calling the parent function necessary

    bool good;
    int timeout = 1000;
    while( (timeout > 0) &&
           (_voltageSet != voltage) ){
        // first set voltage to voltageSet
        _hvModule->SetChannelVoltageSet(_channelNumber, voltage);
         // wait default time and...
        sleepChannel();
        // .. read current voltag, by updating channel
        updateChannel();
        timeout--;
    }
    if(timeout < 1){
        std::cout << "TIMEOUT: could not set voltage to " << voltage << " V" << std::endl;
        good = false;
    }
    else{
        std::cout << "timeout in setVoltage " << timeout << std::endl;
        good = true;
    }

    // if we're not in if, all is good
    return good;
}

bool hvChannel::setCurrent(float current){
    // we set a current for this channel by calling the parent function necessary

    bool good;
    int timeout = 1000;
    while( (timeout > 0) &&
           (_currentSet != current) ){
        // first set current to currentSet
        _hvModule->SetChannelCurrentSet(_channelNumber, current);
        // wait default time and...
        sleepChannel();
        // .. read current voltag, by updating channel
        updateChannel();
        timeout--;
    }
    if(timeout < 1){
        std::cout << "TIMEOUT: could not set current to " << current << " mA" << std::endl;
        good = false;
    }
    else{
        std::cout << "timeout in setCurrent " << timeout << std::endl;
        good = true;
    }

    // if we're not in if, all is good
    return good;
}



bool hvChannel::setVoltageNominal(float voltage){
    // we set a voltage for this channel by calling the parent function necessary

    bool good;
    int timeout = 1000;
    while( (timeout > 0) &&
           (_voltageNominal != voltage) ){
        // first set voltage to voltageSet
        _hvModule->SetChannelVoltageNominal(_channelNumber, voltage);
        // wait default time and...
        sleepChannel();
        // .. read current voltag, by updating channel
        updateChannel();
        timeout--;
    }
    if(timeout < 1){
        std::cout << "TIMEOUT: could not set voltage nominal to " << voltage << " V" << std::endl;
        good = false;
    }
    else{
        std::cout << "timeout in setVoltageNominal " << timeout << std::endl;
        good = true;
    }

    // if we're not in if, all is good
    return good;
}

bool hvChannel::setCurrentNominal(float current){
    // we set a current for this channel by calling the parent function necessary

    bool good;
    int timeout = 1000;
    while( (timeout > 0) &&
           (_currentNominal != current) ){
        // first set current to currentSet
        _hvModule->SetChannelCurrentNominal(_channelNumber, current);
        // wait default time and...
        sleepChannel();
        // .. read current voltag, by updating channel
        updateChannel();
        timeout--;
    }
    if(timeout < 1){
        std::cout << "TIMEOUT: could not set current nominal to " << current << " muA" << std::endl;
        good = false;
    }
    else{
        std::cout << "timeout in setCurrentNominal " << timeout << std::endl;
        good = true;
    }

    // if we're not in if, all is good
    return good;
}

void hvChannel::setVoltageBound(float voltageBound){
    // in contrast to the voltage and current set / nominal functions
    // the voltage bound in our case is simply a software bound. we check
    // whether the voltage is good ourselves
    _voltageBound = voltageBound;
}


// these two functions get the currently set voltage and current
float hvChannel::getVoltage(){
    // update channel...
    updateChannel();
    // ... and return
    return _voltageSet;
}

float hvChannel::getCurrent(){
    // update channel ...
    updateChannel();
    // ... and return
    return _currentSet;
}

// these two functions get the currently measured voltages and currents
float hvChannel::getVoltageMeasured(){
    // update channel ...
    updateChannel();
    // ... and return
    return _currentMeasured;
}
float hvChannel::getCurrentMeasured(){
    // update channel ...
    updateChannel();
    // ... and return
    return _currentMeasured;
}

// these two functions get the currently set nominal voltages and currents
float hvChannel::getVoltageNominal(){
    // update channel ...
    updateChannel();
    // ... and return
    return _currentNominal;
}
float hvChannel::getCurrentNominal(){
    // update channel ...
    updateChannel();
    // ... and return
    return _currentNominal;
}


// function, which prints channel name, number, currently measured voltage
// and set voltage
void hvChannel::printVoltageMeasured(){
    // this function updates channel and prints the current value
    // of voltage measured
    updateChannel();

    std::cout << _channelName 
              << " #" 
              << _channelNumber 
              << " :\t"
              << _voltageMeasured
              << " / "
              << _voltageSet
              << " V"
              << std::endl;

}

void hvChannel::printCurrentMeasured(){
    // this function updates channel and prints the current value
    // of current measured
    updateChannel();

    std::cout << _channelName 
              << " #" 
              << _channelNumber 
              << " :\t"
              << _currentMeasured
              << " / "
              << _currentSet
              << " mA"
              << std::endl;
}

void hvChannel::printVoltageAndCurrentMeasured(){
    // this function updates channel and prints the current value
    // of voltage and current combined in 1 line
    updateChannel();

    std::cout << _channelName 
              << " #" 
              << _channelNumber 
              << " :\t"
              << _voltageMeasured
              << " / "
              << _voltageSet
              << " V"
              << "\t"
              << _currentMeasured
              << " / "
              << _currentSet
              << " A"
              << std::endl;

}


void hvChannel::printVoltageCurrentNominal(){
    // this function updates the channel and then prints the
    // currently set nominal voltage and current
    updateChannel();

    std::cout << "voltage nominal: " << _voltageNominal << " V\n"
              << "current nominal: " << _currentNominal << " mA\n"
              << std::endl;
}

void hvChannel::printChannelName(){
    // this function prints the channel name and number
    std::cout << "#" << _channelNumber
              << " " << _channelName
              << std::endl;
}

void hvChannel::printChannelIsOn(){
    // this function prints channel name, number and isOn flag
    updateChannel();
    
    std::cout << "#" << _channelNumber
              << " " << _channelName
              << " : " << _isOn
              << std::endl;
}

void hvChannel::printChannelEventStatus(){
    // this function prints all bits of the channel event status
    updateChannel();
    
    std::cout << "Channel " << _channelName << " # " << _channelNumber << std::endl;;
    std::cout << "Current status of Channel Event Status bits:\n"
              << "     input error:             \t" << _channelEventStatus.Bit.EventInputError << "\n"
              << "     on to off:               \t" << _channelEventStatus.Bit.EventOnToOff << "\n"
              << "     end of ramping:          \t" << _channelEventStatus.Bit.EventEndOfRamping << "\n"
              << "     controlled by voltage:\t" << _channelEventStatus.Bit.EventControlledByVoltage << "\n"
              << "     controlled by current:\t" << _channelEventStatus.Bit.EventControlledByCurrent << "\n"
              << "     current bounds:       \t" << _channelEventStatus.Bit.EventCurrentBounds << "\n"
              << "     voltage bounds:       \t" << _channelEventStatus.Bit.EventVoltageBounds << "\n"
              << "     external inhibit:     \t" << _channelEventStatus.Bit.EventExternalInhibit << "\n"
              << "     current tripped:      \t" << _channelEventStatus.Bit.EventCurrentTrip << "\n"
              << "     current limit:        \t" << _channelEventStatus.Bit.EventCurrentLimit << "\n"
              << "     voltage limit:        \t" << _channelEventStatus.Bit.EventVoltageLimit << "\n"
              << "     emergency:            \t" << _channelEventStatus.Bit.EventEmergency
              << std::endl;
}

void hvChannel::printChannelStatus(){
    // this function prints all bits of the channel status
    updateChannel();

    std::cout << "Channel " << _channelName << " # " << _channelNumber << std::endl;;
    std::cout << "Current status of Channel Status bits:\n"
              << "    input error:              \t" << _channelStatus.Bit.InputError << "\n"
              << "    is on:            \t" << _channelStatus.Bit.isOn << "\n"
              << "    is ramping:       \t" << _channelStatus.Bit.isRamping << "\n"
              << "    controlled by voltage:\t" << _channelStatus.Bit.ControlledByVoltage << "\n"
              << "    controlled by current:\t" << _channelStatus.Bit.ControlledByCurrent << "\n"
              << "    current bounds:       \t" << _channelStatus.Bit.CurrentBounds << "\n"
              << "    voltage bounds:       \t" << _channelStatus.Bit.VoltageBounds << "\n"
              << "    external inhibit:     \t" << _channelStatus.Bit.ExternalInhibit << "\n"
              << "    current tripped:      \t" << _channelStatus.Bit.CurrentTrip << "\n"
              << "    current limit:        \t" << _channelStatus.Bit.CurrentLimit << "\n"
              << "    voltage limit:        \t" << _channelStatus.Bit.VoltageLimit << "\n"
              << "    is in emergency:      \t" << _channelStatus.Bit.isEmergency
              << std::endl;
}

void hvChannel::printChannel(){
    // this function updates the channel and then calls all printing functions
    updateChannel();

    printVoltageMeasured();
    printCurrentMeasured();
    printVoltageCurrentNominal();
    printChannelStatus();
    printChannelEventStatus();
}


bool hvChannel::setChannelEventMask(std::set<std::string> eventMaskSet){
    // this function allows one to set the the event mask for this channel
    // input: 
    // std::set<std::string> eventMaskList: set of strings containing valid 
    //                                      options for masks to be set
    // we basically check all possible masks via if ... else if...
    // and if such an element is found in the eventMaskList
    
    std::set<std::string>::iterator maskSetEnd;
    maskSetEnd = eventMaskSet.end();
    ChEventMaskSTRUCT channelEventMask = { };

    // TODO: make sure this works! Doesn't seem like it!!!

    if( eventMaskSet.find("InputError")               != maskSetEnd ){
        channelEventMask.Bit.MaskEventInputError          = 1;
    }
    if( eventMaskSet.find("OnToOff")          != maskSetEnd ){
        channelEventMask.Bit.MaskEventOnToOff              = 1; 
    }
    if( eventMaskSet.find("EndOfRamping")             != maskSetEnd ){
        std::cout << "EOR" << std::endl;
        channelEventMask.Bit.MaskEventEndOfRamping         = 1; 
    }
    if( eventMaskSet.find("Emergency")        != maskSetEnd ){
        std::cout << "EMERG" << std::endl;
        channelEventMask.Bit.MaskEventEmergency            = 1; 
    }
    if( eventMaskSet.find("ControlledByCurrent") != maskSetEnd ){
        channelEventMask.Bit.MaskEventControlledByVoltage  = 1; 
    }
    if( eventMaskSet.find("ControlledByVoltage") != maskSetEnd ){
        channelEventMask.Bit.MaskEventControlledByCurrent  = 1; 
    }
    if( eventMaskSet.find("CurrentBounds")       != maskSetEnd ){
        channelEventMask.Bit.MaskEventCurrentBounds        = 1; 
    }
    if( eventMaskSet.find("VoltageBounds")       != maskSetEnd ){
        channelEventMask.Bit.MaskEventVoltageBounds        = 1; 
    }
    if( eventMaskSet.find("ExternalInhibit")     != maskSetEnd ){
        channelEventMask.Bit.MaskEventExternalInhibit      = 1; 
    }
    if( eventMaskSet.find("CurrentTrip")              != maskSetEnd ){
        std::cout << "CurTrip" << std::endl;
        channelEventMask.Bit.MaskEventCurrentTrip          = 1; 
    }
    if( eventMaskSet.find("CurrentLimit")             != maskSetEnd ){
        std::cout << "CurLim" << std::endl;
        channelEventMask.Bit.MaskEventCurrentLimit         = 1; 
    }
    if( eventMaskSet.find("VoltageLimit")             != maskSetEnd ){
        std::cout << "VolLim" << std::endl;
        channelEventMask.Bit.MaskEventVoltageLimit         = 1; 
    }

    // now we have filled channelEventMask with all bits given in 
    // eventMaskSet

    // now we can set the eventMask on the channel
    bool good;
    int timeout = 1000;
    while( (timeout > 0) &&
           (_channelEventMask.Word != channelEventMask.Word) ){
        _hvModule->SetChannelEventMask(_channelNumber, channelEventMask.Word);
        // wait default time and...
        sleepChannel();
        // .. read current channel event mask by updating channel
        updateChannel();
        timeout--;
    }
    if(timeout < 1){
        std::cout << "TIMEOUT: could not set channel event mask" << std::endl;
        good = false;
    }
    else{
        std::cout << "timeout in setChannelEventMask " << timeout << std::endl;
        good = true;
    }

    return good;
}

ChEventMaskSTRUCT hvChannel::getChannelEventMask(){
    // update the channel 
    updateChannel();
    // and return
    return _channelEventMask;
}

bool hvChannel::clearChannelEventStatus(bool forceEmpty){
    // this function tries to clear the channel event status
    // returns false, if it does not work
    // input:
    //     bool forceEmpty: if this flag is set to true, we will try to force
    //                      the setting of an empty channel event status
    //                      this should only be done, if one uses this function
    //                      while shutting down a channel, since only if the
    //                      channel is off and has ramped down can we set the
    //                      event status to zero
    bool good;
    int timeout = 1000;
    // NOTE!!! calling the function ClearChannelEventStatus writes a 0xFFFFFFFF
    // to the channel event status register. However, this, for some reason,
    // resets all of the bits to 0! So we need to check, if all event status
    // bits are set to 0 upon being reset
    //uint32_t emptyChannelEventStatus = 0;
    ChEventStatusSTRUCT emptyChannelEventStatus = { };

    // first update channel
    updateChannel();
    // now check whether channel status is non zero (and forceEmpty is not set)
    if( (forceEmpty == false) &&
        (_channelStatus.Word != 0) ){
        // output that channel status non zero, so cannot clear all event status
        // bits
        std::cout << "Channel status is non-zero. Cannot reset every event status bit.\n"
                  << "Will clear all non set event bits for which channel status is zero."
                  << std::endl;
        emptyChannelEventStatus.Word = _channelStatus.Word;
        // not all event bits correspond to channel status exactly. i.e., we can reset
        // the EndOfRamping and OnToOff bits, even if (corresponding bit->) channel is ramping
        // and channel is on. Thus, set these two bits to zero in our event status, which we
        // want to set
        emptyChannelEventStatus.Bit.EventEndOfRamping = 0;
        emptyChannelEventStatus.Bit.EventOnToOff      = 0;
    }

    // debug output to see if correct bits are set in case channel status non zero
    std::bitset<32> bits(emptyChannelEventStatus.Word);
    std::cout << "bits empty ChEventSt " << bits.to_string() << std::endl;

    
    while( (timeout > 0) &&
           (_channelEventStatus.Word != emptyChannelEventStatus.Word) ){
        // clear channel event status. In case channel status bits are set, the
        // resulting _channelEventStatus should be correct
        _hvModule->ClearChannelEventStatus(_channelNumber);
        // sleep and update channel
        sleepChannel();
        updateChannel();
        timeout--;
    }
    if( timeout < 1 ){
        std::cout << "TIMEOUT: could not reset channel status!\n"
                  << "Do you wish to print channel status? (y/N)"
                  << std::endl;
        std::string input;
        std::set<std::string> allowedStrings = {"y", "Y", "n", "N"};
        input = getUserInputNonNumericalDefault("> ", &allowedStrings);
        if (input == "quit") good = false;
        else if( (input == "y") || 
                 (input == "Y") ){
            printChannelEventStatus();
        }
        else{
            good = false;
        }
    }
    else{
        // in this case all went fine and we successfully reset channel
        std::cout << "timeout in clearChannelEventStatus " << timeout << std::endl;
        good = true;
    }
    return good;
}


bool hvChannel::switchOnOff(){
    // this function simply switches channel on, if off and vice versa
    updateChannel();
    bool good = false;

    if(_isOn == true){
        good = turnOff();
    }
    else{
        good = turnOn();
    }

    return good;
}

// function to turn channel on and off
// both return a bool to tell, whether action was successful
bool hvChannel::turnOn(){
    // check if channel is already turned on
    int timeout = 1000;
    // first update channel to see whether channel might
    // be already on
    updateChannel();
    while( (timeout > 0) &&
           (_isOn == false) ){
        // set channel on
        _hvModule->SetChannelOn(_channelNumber);
        // sleep
        sleepChannel();
        // update channel (updates _isOn)
        updateChannel();
        timeout--;
    }
    if (timeout < 1){
        std::cout << "TIMEOUT: could not turn on channel! Check channel event status!"
                  << std::endl;
        // TODO: include flag to print event status in case of timeout to understand
        // what's going on
        // printEventStatus();
    }
    std::cout << "timeout in turnOn " << timeout << std::endl;
    return _isOn;
}


bool hvChannel::turnOff(){
    // check if channel is already turned off
    // in this case turn on channel
    int timeout = 1000;
    while( (timeout > 0) &&
           (_isOn == true) ){
        // set channel on
        _hvModule->SetChannelOff(_channelNumber);
        // sleep
        sleepChannel();
        // update channel (updates _isOn)
        updateChannel();
        timeout--;
    }
    if (timeout < 1){
        std::cout << "TIMEOUT: could not turn on channel! Check channel event status!"
                  << std::endl;
        // TODO: include flag to print event status in case of timeout to understand
        // what's going on
        printChannelEventStatus();
    }
    
    // return the inverse of _isOn, since we want to return true, if the channel
    // successfully turned off
    std::cout << "timeout in turnOff " << timeout << std::endl;
    return !_isOn;
}


// returns true or false, depending on whether channel is on
bool hvChannel::isOn(){
    // update channel and return isOn bit
    updateChannel();
    return _isOn;
}

// returns true or false, depending on whether channel is ramping
bool hvChannel::isRamping(){
    // update channel and return isOn bit
    updateChannel();
    return _isRamping;
}

bool hvChannel::endOfRamping(){
    // returns true, if the channel has finished ramping
    updateChannel();
    return _endOfRamping;
}

bool hvChannel::isVoltageControlled(){
    // update channel and return isVoltageControlled bit
    updateChannel();
    return _isVoltageControlled;
}

bool hvChannel::finishedRampingDown(){
    // function returns whether the channel has finished ramping down
    // determined by _isOn == false and _isRamping == true
    updateChannel();
    
    bool finishedRampingDown = false;
    if( (_isOn == false) &&
        (_isRamping == true) ){
        finishedRampingDown = false;
    }
    else{
        finishedRampingDown = true;
    }
    // now try to force the channel event status to zero. 
    // clearChannelEventStatus(true);
    // only once the channel event status is actually zero, can we
    // say the ramping down finished
    // if (_channelEventStatus.Word == 0){
    //  finishedRampingDown = true;
    // }
    // else{
    //  finishedRampingDown = false;
    // }

    return finishedRampingDown;
}

bool hvChannel::voltageInBounds(){
    // this function uses the _voltageBound to check, whether
    // the channel is within voltage bounds
    // update channel
    updateChannel();
    // define boundaries
    float voltageMin;
    float voltageMax;
    voltageMin = _voltageSet - _voltageBound;
    voltageMax = _voltageSet + _voltageBound;

    bool good = false;
    if( (_voltageMeasured > voltageMin) &&
        (_voltageMeasured < voltageMax) ){
        good = true;
    }
    else{
        good = false;
    }

    return good;
}

bool hvChannel::channelStatusNonZero(){
    // function returns true, if one or more bits of the
    // channel status are non zero (channel in some way active)
    updateChannel();
    bool active = false;
    if(_channelStatus.Word != 0){
        active = true;
    }
    else{
        active = false;
    }
    return active;
}

void hvChannel::sleepChannel(){
    // sleep this thread for _sleep time in milli seconds
    std::this_thread::sleep_for(std::chrono::milliseconds(_sleepTime));
}

bool hvChannel::onVoltageControlledRamped(bool printFlag){
    // this function returns whether the channel is turned on, controlled by voltage
    // and has finished ramping
    // and prints the voltage currently measured
    bool good;
    //updateChannel();

    // check is on and voltage controlled
    good  = isOn();
    good *= isVoltageControlled();
    // and has finished ramping
    good *= endOfRamping();
    // and finally also make sure our voltage is within bounds
    good *= voltageInBounds();

    if (printFlag == true){
        printVoltageAndCurrentMeasured();
    }

    return good;
}

