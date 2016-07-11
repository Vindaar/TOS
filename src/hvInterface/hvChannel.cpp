// this is the implementation of the HV channel

#include "hvInterface/hvChannel.hpp"

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
    _isOn = _channelStatus.Bit.isOn;
    _isRamping = _channelStatus.Bit.isRamping;

}

bool hvChannel::setVoltage(float voltage){
    // we set a voltage for this channel by calling the parent function necessary

    bool good;
    int timeout = 1000;
    while( (timeout > 1) &&
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
	good = true;
    }

    // if we're not in if, all is good
    return good;
}

bool hvChannel::setCurrent(float current){
    // we set a current for this channel by calling the parent function necessary

    bool good;
    int timeout = 1000;
    while( (timeout > 1) &&
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
	good = true;
    }

    // if we're not in if, all is good
    return good;
}



bool hvChannel::setVoltageNominal(float voltage){
    // we set a voltage for this channel by calling the parent function necessary

    bool good;
    int timeout = 1000;
    while( (timeout > 1) &&
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
	std::cout << "TIMEOUT: could not set voltage to " << voltage << " V" << std::endl;
	good = false;
    }
    else{
	good = true;
    }

    // if we're not in if, all is good
    return good;
}

bool hvChannel::setCurrentNominal(float current){
    // we set a current for this channel by calling the parent function necessary

    bool good;
    int timeout = 1000;
    while( (timeout > 1) &&
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
	std::cout << "TIMEOUT: could not set current to " << current << " mA" << std::endl;
	good = false;
    }
    else{
	good = true;
    }

    // if we're not in if, all is good
    return good;
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

void hvChannel::printChannelEventStatus(){
    // this function prints all bits of the channel event status
    updateChannel();
    
    std::cout << "Current status of Channel Event Status bits:\n"
	      << "input error:         	\t" << _channelEventStatus.Bit.EventInputError
	      << "on to off:           	\t" << _channelEventStatus.Bit.EventOnToOff
	      << "end of ramping:      	\t" << _channelEventStatus.Bit.EventEndOfRamping
	      << "controlled by voltage:\t" << _channelEventStatus.Bit.EventControlledByVoltage
	      << "controlled by current:\t" << _channelEventStatus.Bit.EventControlledByCurrent
	      << "current bounds:       \t" << _channelEventStatus.Bit.EventCurrentBounds
	      << "voltage bounds:       \t" << _channelEventStatus.Bit.EventVoltageBounds
	      << "external inhibit:     \t" << _channelEventStatus.Bit.EventExternalInhibit
	      << "current tripped:      \t" << _channelEventStatus.Bit.EventCurrentTrip
	      << "current limit:        \t" << _channelEventStatus.Bit.EventCurrentLimit
	      << "voltage limit:        \t" << _channelEventStatus.Bit.EventVoltageLimit
	      << "emergency:            \t" << _channelEventStatus.Bit.EventEmergency
	      << std::endl;
}

void hvChannel::printChannelStatus(){
    // this function prints all bits of the channel status
    updateChannel();
    
    std::cout << "Current status of Channel Status bits:\n"
	      << "input error:         	\t" << _channelStatus.Bit.InputError
	      << "is on:           	\t" << _channelStatus.Bit.isOn
	      << "is ramping:      	\t" << _channelStatus.Bit.isRamping
	      << "controlled by voltage:\t" << _channelStatus.Bit.ControlledByVoltage
	      << "controlled by current:\t" << _channelStatus.Bit.ControlledByCurrent
	      << "current bounds:       \t" << _channelStatus.Bit.CurrentBounds
	      << "voltage bounds:       \t" << _channelStatus.Bit.VoltageBounds
	      << "external inhibit:     \t" << _channelStatus.Bit.ExternalInhibit
	      << "current tripped:      \t" << _channelStatus.Bit.CurrentTrip
	      << "current limit:        \t" << _channelStatus.Bit.CurrentLimit
	      << "voltage limit:        \t" << _channelStatus.Bit.VoltageLimit
	      << "is in emergency:      \t" << _channelStatus.Bit.isEmergency
	      << std::endl;
}

void hvChannel::printChannel(){
    // this function updates the channel and then calls all printing functions
    updateChannel();

    printVoltageMeasured();
    printCurrentMeasured();
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
    ChEventMaskSTRUCT channelEventMask = { 0 };

    if( eventMaskSet.find("InputError") 	      != maskSetEnd ){
	channelEventMask.Bit.MaskEventInputError          = 1;
    }
    else if( eventMaskSet.find("OnToOff") 	      != maskSetEnd ){
	channelEventMask.Bit.MaskEventOnToOff 		   = 1;	
    }
    else if( eventMaskSet.find("EndOfRamping") 	      != maskSetEnd ){
	channelEventMask.Bit.MaskEventEndOfRamping	   = 1;	
    }
    else if( eventMaskSet.find("Emergency") 	      != maskSetEnd ){
	channelEventMask.Bit.MaskEventEmergency 	   = 1;	
    }
    else if( eventMaskSet.find("ControlledByCurrent") != maskSetEnd ){
	channelEventMask.Bit.MaskEventControlledByVoltage = 1;	
    }
    else if( eventMaskSet.find("ControlledByVoltage") != maskSetEnd ){
	channelEventMask.Bit.MaskEventControlledByCurrent = 1;	
    }
    else if( eventMaskSet.find("CurrentBounds")       != maskSetEnd ){
	channelEventMask.Bit.MaskEventCurrentBounds 	   = 1;	
    }
    else if( eventMaskSet.find("VoltageBounds")       != maskSetEnd ){
	channelEventMask.Bit.MaskEventVoltageBounds	    = 1;	
    }
    else if( eventMaskSet.find("ExternalInhibit")     != maskSetEnd ){
	channelEventMask.Bit.MaskEventExternalInhibit	    = 1;	
    }
    else if( eventMaskSet.find("CurrentTrip") 	      != maskSetEnd ){
	channelEventMask.Bit.MaskEventCurrentTrip	    = 1;	
    }
    else if( eventMaskSet.find("CurrentLimit") 	      != maskSetEnd ){
	channelEventMask.Bit.MaskEventCurrentLimit	    = 1;	
    }
    else if( eventMaskSet.find("VoltageLimit") 	      != maskSetEnd ){
	channelEventMask.Bit.MaskEventVoltageLimit 	   = 1;	
    }

    // now we have filled channelEventMask with all bits given in 
    // eventMaskSet

    // now we can set the eventMask on the channel
    bool good;
    int timeout = 1000;
    while( (timeout > 1) &&
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

bool hvChannel::clearChannelEventStatus(){
    // this function tries to clear the channel event status
    // returns false, if it does not work
    bool good;
    int timeout = 1000;
    uint32_t emptyChannelEventStatus = 0xFFFFFFFF;
    while( (timeout > 1) &&
	   (_channelEventStatus.Word != emptyChannelEventStatus) ){
	// clear channel event status
	_hvModule->ClearChannelEventStatus(_channelNumber);
	// sleep and update channel
	sleepChannel();
	updateChannel();
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
	good = true;
    }
    return good;
}


// function to turn channel on and off
// both return a bool to tell, whether action was successful
bool hvChannel::turnOn(){
    // check if channel is already turned on
    int timeout = 1000;
    while (_isOn == false){
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
    return _isOn;
}


bool hvChannel::turnOff(){
    // check if channel is already turned off
    // in this case turn on channel
    int timeout = 1000;
    while (_isOn == true){
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


void hvChannel::sleepChannel(){
    // sleep this thread for _sleep time in milli seconds
    std::this_thread::sleep_for(std::chrono::milliseconds(_sleepTime));
}
