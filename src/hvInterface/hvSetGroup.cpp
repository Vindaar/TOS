// this is the implementation of the set group, derived from the abstract hvFlexGroup

#include "hvInterface/hvSetGroup.hpp"

hvSetGroup::hvSetGroup(hvModule *hvModule,
		       std::string groupName, 
		       std::list<hvChannel *> channelList,
		       int groupNumber,
		       int masterChannel)
    : hvFlexGroup(hvModule, groupName, channelList, groupNumber, "set"), 
      _masterChannel(masterChannel), 
      _mode(DEFAULT_NOT_INITIALIZED),
      _control(0){
    // the main thing we need to set in the creator of the set group, is the 
    // _allowedControls
    _allowedControls.insert(std::pair<std::string, int> ("voltage",   GROUP_SET_VSET    ) );
    _allowedControls.insert(std::pair<std::string, int> ("current",   GROUP_SET_ISET    ) );
    _allowedControls.insert(std::pair<std::string, int> ("v_ilk_max", GROUP_SET_VILK_MAX) );
    _allowedControls.insert(std::pair<std::string, int> ("c_ilk_max", GROUP_SET_IILK_MAX) );
    _allowedControls.insert(std::pair<std::string, int> ("v_ilk_min", GROUP_SET_VILK_MIN) );
    _allowedControls.insert(std::pair<std::string, int> ("c_ilk_min", GROUP_SET_IILK_MIN) );
    _allowedControls.insert(std::pair<std::string, int> ("set_on",    GROUP_SET_SETON   ) );
    _allowedControls.insert(std::pair<std::string, int> ("emergency", GROUP_SET_EMCY    ) );
    _allowedControls.insert(std::pair<std::string, int> ("clone",     GROUP_SET_CLONE   ) );

    // set group set struct group type to the correct value for a set group

    _setStruct.Bit.GroupType = GROUP_TYPE_SET;
}

hvSetGroup::~hvSetGroup(){
    // nothing to do here, flexGroup destructor does all we need
}

void hvSetGroup::updateSetGroup(){
    // this function updates the set group, meaning it actually simply updates
    // the group set Struct
    
    _setStruct.Bit.MasterChannel = _masterChannel;
    _setStruct.Bit.Mode          = _mode;
    _setStruct.Bit.Control       = _control;

    
    if( (_control != 0) &&
	(_mode != DEFAULT_NOT_INITIALIZED) ){
	// if _control is not 0, and _mode not equal to unitialized macro,
	// we have changed it at some point to some valid value, so we can
	// set _goodToWrite to true
	_goodToWrite = true;
	// and more importantly set the _groupStruct Type property, to our
	// _setStruct
	_groupStruct.Type1.Word = _setStruct.Word;
    }
}

void hvSetGroup::setMasterChannel(int channel){
    // function to set master channel and updates group 
    _masterChannel = channel;
    updateSetGroup();
}

int hvSetGroup::getMasterChannel(){
    return _masterChannel;
}

void hvSetGroup::setMode(int mode){
    // this function sets the _mode variable to mode
    // TODO: put explanation of mode for set group here!!!
    _mode = mode;
    // ... and update group
    updateSetGroup();
}

int hvSetGroup::getMode(){
    // just return _mode
    return _mode;
}

bool hvSetGroup::setControl(std::string controlString){
    // the set control function sets the controlString to the
    // correct control option. If non valid option given, user input is called
    // if correct option is set (either immediately or after user input), return true
    // else return false (in case of user input "quit")
    bool good = false;
    
    std::map<std::string, int>::iterator it;
    it = _allowedControls.find(controlString);
    if( it != _allowedControls.end() ){
	// in this case controlString is element of allowed controls
	// so set control to correct integer value
	_control = it->second;

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
	std::for_each(_allowedControls.begin(), _allowedControls.end(), [&allowedStrings](std::pair<std::string, int> pair){
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
	    _control = _allowedControls[input];
	    good = true;
	}
    }

    if(good == true){
	// if good is true, we update the set group
	updateSetGroup();
    }
    
    return good;    
}

std::string hvSetGroup::getControl(){
    // this function returns the corresponding string, to the value
    // _control is assigned to
    
    // since we know, that _control can only be assigned to a valid value
    // (we only assign _control based on elements in map!), we can just run
    // over map to get correct key for value of _control
    std::string controlString;
    std::for_each(_allowedControls.begin(), 
		  _allowedControls.end(), 
		  [&controlString, this](std::pair<std::string, int> pair){
		      if(pair.second == this->_control){
			  controlString = pair.first;
		      }
		  } );
    // controlString should contain correct string based on _control
    return controlString;
}
