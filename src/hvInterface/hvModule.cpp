// this is the implementation of the hvModule class

#include "hvInterface/hvModule.hpp"

hvModule::hvModule(CVmeController *vmeController, int baseAddress)
    : CVmeModule(vmeController, baseAddress),
      _isConnected(0),
      _hvModInitFlag(false),
      _hvModCreatedFlag(false){
    // upon initialisation of hvModule, we need to create the
    // vmemodule (HV) instance using the base Addresses

    // first of all, if we are handed a NULL pointer to the constructor for
    // the vmeController, we need to initialize our own VmeController and 
    // thus point it to the VmeController member variable
    // initialize the USB Controller
    if (vmeController == NULL){
	VmeController = new CVmUsb();
	VmeController->initController(0);
	_createdOwnVmeController = true;
    }
    else{
	_createdOwnVmeController = false;
    }

    // now we check, whether we can connect to the HV module
    _isConnected = ConnectModule();

    // set the object created flag to true
    _hvModCreatedFlag = true;
}

hvModule::~hvModule(){
    // destructor needs to delete vmecontroller
    if (_createdOwnVmeController == true){
	delete(VmeController);
    }
}

bool hvModule::CheckIsConnectionGood(){
    // this function checks, whether the connection to the module
    // could be established and if not, try for a certain time
    // if we cannot establish connection, return false
    // Since it's exactly the same thing we do, when we try to connect to the module
    // for the first time, this is just a wrapper around that function
    return ConnectModule();
}

bool hvModule::ConnectModule(){
    // this function tries to connect to the module
    // returns:
    // bool good: true if connected, false otherwise
    
    // first call ConnectModule
    bool good;
    // connect module calls the IsConnected() function from vmemodule.h (function in header)
    // returns 0 if no connection established, 1 is connection is good
    good = IsConnected();

    int timeout;
    timeout = 1000;
    while( (good == 0) && 
	   (timeout > 0) ){
	// if no good connection is available, try to establish it
	good = IsConnected();
	sleepModule();
	timeout--;
    }
    if (timeout < 1){
	// in this case we timed out and could not establish a connection
	std::cout << "Connection to HV module could not be established" << std::endl;
	good = false;
    }
    else{
	good = true;
    }
    
    // set the member variable to good so that one can always see, if connection was established correctly
    _isConnected = good;

    return good;    
}

bool hvModule::IsKillEnabled(){
    // this function simply gets the module status of the module (by updating module)
    // and checks whether the IsKillEnable bit is activated
    bool killEnabled;
    updateModule();

    killEnabled = _moduleStatus.Bit.IsKillEnable;

    return killEnabled;
}

void hvModule::updateModule(){
    // this function updates the module member variables, i.e. it reads all 
    // functions for which the hvModule has a member variable

    _moduleEventGroupMask = GetModuleEventGroupMask(); 
    // TODO: add more

    _moduleControl.Word = GetModuleControl();
    _moduleStatus.Word  = GetModuleStatus();

    _isStop = _moduleStatus.Bit.IsStop;
}

bool hvModule::SetKillEnable(bool setKillEnable){
    // this function trys to set kill enable flag to value
    // given to function
    
    // now we have all values to set up the HV for TOS
    // first set kill enable
    // enable Kill Mode
    std::cout << "trying to set kill enable to: " << setKillEnable << std::endl;
    bool killEnabled = false;

    if(_isConnected){
	// only if connection was establish
        int timeout = 1000;
        while ( ( killEnabled == false ) &&
		(timeout > 0) ){
	    // set kill enable
	    SetModuleKillEnable(setKillEnable);
	    sleepModule();
	    // check if it had any effect
	    killEnabled = IsKillEnabled();
	    timeout--;
        }
        if (timeout == 0){
	    std::cout << "TIMEOUT: Module could not be set to SetKillEnable" << std::endl;
	    // for now throw exception without anything else
	    // TODO: set this up properly!!!
	    // throw();
        }
        else{
	    std::cout << "Module successfully set to SetKillEnable = " << setKillEnable << std::endl;
        }
    }
    else{
	std::cout << "connection to module is not established. Connect first" << std::endl;
    }

    return killEnabled;
}

GroupSTRUCT hvModule::GetFlexGroup(int group){
    // by Sebastian Schmidt
    // call both get flex group functions from the module
    // and return the group built from that
    // note: initialization with designated initializer illegal in c++
    // thus, initialization includes comments for readability
    GroupSTRUCT groupObject;

    groupObject.MemberList1.Word = GetModuleFlexGroupMemberList(group);
    
    // we're using Type 1, because in our implementation we're only properly
    // supporting the module with 12 channels
    groupObject.Type1.Word = GetModuleFlexGroupType(group);

    return groupObject;
}


void hvModule::SetFlexGroup(int group, GroupSTRUCT groupObject){
    // by Sebastian Schmidt
    // call both set flex group functions from the module
    // and write them to the HV module

    SetModuleFlexGroupMemberList(group, groupObject.MemberList1.Word);
    
    // we're using Type 1, because in our implementation we're only properly
    // supporting the module with 12 channels
    SetModuleFlexGroupType(group, groupObject.Type1.Word);
}

void hvModule::sleepModule(){
    // function which calls sleep for this thread for time given by 
    // macro in header

    std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_HV_SLEEP_TIME));
}

bool hvModule::isStop(){
     // function returns isStop bit from moduleStatus
    updateModule();
    return _isStop;
}


bool hvModule::setStopModule(bool stop){
    // this function gets the current module status, by updating the module
    // checks if the isStop bit is set to stop, if not, get module control
    // set bit the way we want and set
    bool good;

    updateModule();
    if(_isStop == stop){
	// in this case nothing to do, set good
	good = true;
    }

    int timeout = 1000;
    while( (timeout > 1) &&
	   (_isStop != stop) ){
	// update module, 
	updateModule();
	// set moduleControl bit setStop to 1
	_moduleControl.Bit.SetStop = 1;
	// and write to module
	SetModuleControl(_moduleControl.Word);
    }
    if (timeout < 1){
	std::cout << "TIMEOUT: could not set module to SetStop == " << good << std::endl;
	good = false;
    }
    else{
	good = true;
    }    
    
    return good;
}


