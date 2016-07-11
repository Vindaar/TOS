// this is the implementation of the hvMonitorGroup, derived from the hvStatusGroup

#include "hvInterface/hvMonitorGroup.hpp"

hvMonitorGroup::hvMonitorGroup(hvModule *hvModule,
			       std::string groupName, 
			       std::list<hvChannel *> channelList,
			       int groupNumber)
    : hvStatusGroup(hvModule, groupName, channelList, groupNumber, "monitor"),
      _mode(DEFAULT_NOT_INITIALIZED),
      _action(DEFAULT_NOT_INITIALIZED){
    // allowedMonitors is filled in status group constructor
    // still need to set _allowedActions
    _allowedActions.insert(std::pair<std::string, int> ("no_action",   GROUP_ACTION_NO_ACTION) );
    _allowedActions.insert(std::pair<std::string, int> ("ramp_down",   GROUP_ACTION_RAMP_DOWN) );
    _allowedActions.insert(std::pair<std::string, int> ("shut_group",  GROUP_ACTION_SHUT_GROUP) );
    _allowedActions.insert(std::pair<std::string, int> ("shut_module", GROUP_ACTION_SHUT_MODULE) );

    // set type to monitor group
    _monitorStruct.Bit.GroupType = GROUP_TYPE_MONITOR;
}

hvMonitorGroup::~hvMonitorGroup(){
    // nothing to be done here, already dealt with in hvFlexGroup
}

void hvMonitorGroup::updateMonitorGroup(){
    // this function updates the monitor group, meaning it actually simply updates
    // the group Status Struct
    
    _monitorStruct.Bit.Monitor = _monitor;
    _monitorStruct.Bit.Mode    = _mode;
    _monitorStruct.Bit.Action  = _action;
    
    if( (_monitor != 0) &&
	(_mode    != DEFAULT_NOT_INITIALIZED) &&
	(_action  != DEFAULT_NOT_INITIALIZED) ){
	// if monitor, mode and action not 0 or uninitialized, we can
	// set _goodToWrite to true
	_goodToWrite = true;
	// and more importantly set the _groupStruct Type property, to our
	// _setStruct
	_groupStruct.Type1.Word = _monitorStruct.Word;
    }
}
    
void hvMonitorGroup::setMode(int mode){
    // this function sets the _mode variable to mode
    // TODO: put explanation of mode for monitor group here!!!
    _mode = mode;
    // ... and update group
    updateMonitorGroup();
}

int hvMonitorGroup::getMode(){
    // just return _mode
    return _mode;
}

bool hvMonitorGroup::setAction(std::string actionString){
    // this function allows to set the action taken, if the option we monitor
    // is active
    // actionString has to be element of _allowedActions filled in creator

    bool good = false;
    
    std::map<std::string, int>::iterator it;
    it = _allowedActions.find(actionString);
    if( it != _allowedActions.end() ){
	// in this case monitorString is element of allowed controls
	// so set control to correct integer value
	_action = it->second;

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
	std::for_each(_allowedActions.begin(), _allowedActions.end(), [&allowedStrings](std::pair<std::string, int> pair){
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
	    _action = _allowedActions[input];
	    good = true;
	}
    }

    if(good == true){
	// if good is true, we update the set group
	updateMonitorGroup();
    }
    return good;    
}

std::string hvMonitorGroup::getAction(){
    // this function returns the corresponding string, to the value
    // _action is assigned to
    
    // since we know, that _action can only be assigned to a valid value
    // (we only assign _action based on elements in map!), we can just run
    // over map to get correct key for value of _action
    std::string actionString;
    std::for_each(_allowedActions.begin(), 
		  _allowedActions.end(), 
		  [&actionString, this](std::pair<std::string, int> pair){
	    if(pair.second == this->_action){
		actionString = pair.first;
	    }
		  } );
    // actionString should contain correct string based on _action
    return actionString;
}
