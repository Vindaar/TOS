// this is the implementation of the status group derived from the
// hvFlexGroup

#include "hvInterface/hvStatusGroup.hpp"

hvStatusGroup::hvStatusGroup(hvModule *hvModule,
			     std::string groupName, 
			     std::list<hvChannel *> channelList, 
			     int groupNumber,
			     std::string groupType)
    : hvFlexGroup(hvModule, groupName, channelList, groupNumber, groupType),
      _monitor(0) {
    // the main thing we need to set in the creator of the set group, is the 
    // _allowedMonitors
    _allowedMonitors.insert(std::pair<std::string, int> ("is_on",                 GROUP_STATUS_ISON  ) );
    _allowedMonitors.insert(std::pair<std::string, int> ("is_ramping", 		  GROUP_STATUS_ISRAMP) );
    _allowedMonitors.insert(std::pair<std::string, int> ("is_current_controlled", GROUP_STATUS_ISCC  ) );
    _allowedMonitors.insert(std::pair<std::string, int> ("is_voltage_controlled", GROUP_STATUS_ISCV  ) );
    _allowedMonitors.insert(std::pair<std::string, int> ("current_is_in_bound",   GROUP_STATUS_ISIBND) );
    _allowedMonitors.insert(std::pair<std::string, int> ("voltage_is_in_bound",   GROUP_STATUS_ISVBND) );
    _allowedMonitors.insert(std::pair<std::string, int> ("is_emergency_inhib???", GROUP_STATUS_ISEINH) );
    _allowedMonitors.insert(std::pair<std::string, int> ("is_tripped", 		  GROUP_STATUS_ISTRIP) );
    _allowedMonitors.insert(std::pair<std::string, int> ("current_is_limited??",  GROUP_STATUS_ISILIM) );
    _allowedMonitors.insert(std::pair<std::string, int> ("voltage_is_limited??",  GROUP_STATUS_ISVLIM) );

    // and set status struct type correctly
    _statusStruct.Bit.GroupType = GROUP_TYPE_STATUS;
}

hvStatusGroup::~hvStatusGroup(){
    // nothing to be done here, all done by hvFlexGroup destructor
}

void hvStatusGroup::updateStatusGroup(){
    // this function updates the set group, meaning it actually simply updates
    // the group Status Struct
    
    _statusStruct.Bit.Item = _monitor;
    
    if(_monitor != 0){
	// if _control is not 0, we have changed it at some point to some valid value,
	// and since it's the only setting not set by default for the status struct, we can
	// set _goodToWrite to true
	_goodToWrite = true;
	// and more importantly set the _groupStruct Type property, to our
	// _setStruct
	_groupStruct.Type1.Word = _statusStruct.Word;
    }
}

bool hvStatusGroup::setMonitor(std::string monitorString){
    // this function is the essential part of the status group, since
    // it sets the _monitor variable, to the correct integer value for
    // the status struct
    // monitorString has to be element of _allowedMonitors filled in creator

    bool good = false;
    
    std::map<std::string, int>::iterator it;
    it = _allowedMonitors.find(monitorString);
    if( it != _allowedMonitors.end() ){
	// in this case monitorString is element of allowed controls
	// so set control to correct integer value
	_monitor = it->second;

	good = true;
    }
    else{
	// if the input string is not allowed, we call get user input to ask for
	// correct input
	std::set<std::string> allowedStrings;
	// first fill _allowedStrings set for getUserInput
	std::cout << "Error: non valid control string given to function.\n"
		  << "Please enter string of following list:"
		  << std::endl;
	std::for_each(_allowedMonitors.begin(), _allowedMonitors.end(), [&allowedStrings](std::pair<std::string, int> pair){
		allowedStrings.insert(pair.first);
		std::cout << pair.first << std::endl;
	    } );
	std::string input;
	input = getUserInputNonNumericalNoDefault("> ", &allowedStrings);
	if(input == "quit") good = false;
	else{
	    // in this case valid user input was given
	    // get correct value for key 'input'
	    
	    // don't need any checks, since we already checked every failure option
	    _monitor = _allowedMonitors[input];
	    good = true;
	}
    }

    if(good == true){
	// if good is true, we update the set group
	updateStatusGroup();
    }

    return good;    
}

std::string hvStatusGroup::getMonitor(){
    // this function returns the corresponding string, to the value
    // _monitor is assigned to
    
    // since we know, that _monitor can only be assigned to a valid value
    // (we only assign _monitor based on elements in map!), we can just run
    // over map to get correct key for value of _monitor
    std::string monitorString;
    std::for_each(_allowedMonitors.begin(), 
		  _allowedMonitors.end(), 
		  [&monitorString, this](std::pair<std::string, int> pair){
	    if(pair.second == this->_monitor){
		monitorString = pair.first;
	    }
		  } );
    // monitorString should contain correct string based on _monitor
    return monitorString;
}
