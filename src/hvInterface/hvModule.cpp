#include "hvInterface/hvModule.hpp"
#include <bitset>


hvModule::hvModule(CVmeController *vmeController, int baseAddress)
    : CVmeModule(vmeController, baseAddress),
      _isConnected(0),
      _hvModInitFlag(false),
      _hvModCreatedFlag(false) {
    // upon initialisation of hvModule, we need to create the
    // vmemodule (HV) instance using the base Addresses

    // first of all, if we are handed a NULL pointer to the constructor for
    // the vmeController, we need to initialize our own VmeController and 
    // thus point it to the VmeController member variable
    // initialize the USB Controller
    if (vmeController == NULL) {
        VmeController = new CVmUsb();
        VmeController->initController(0);
        _createdOwnVmeController = true;
    } else {
        _createdOwnVmeController = false;
    }

    // now we check, whether we can connect to the HV module
    _isConnected = ConnectModule();

    // set the object created flag to true
    _hvModCreatedFlag = true;
}

hvModule::~hvModule() {
    // destructor needs to delete vmecontroller
    if (_createdOwnVmeController == true) {
        delete(VmeController);
    }
}

bool hvModule::CheckIsConnectionGood(){
    return ConnectModule();
}

bool hvModule::ConnectModule(){
    bool good;
    good = IsConnected();

    int timeout;
    timeout = 1000;

    while( (good == 0) && 
           (timeout > 0) ) {
        // if no good connection is available, try to establish it
        good = IsConnected();
        sleepModule();
        timeout--;
    }
    if (timeout < 1) {
        // in this case we timed out and could not establish a connection
        std::cout << "Connection to HV module could not be established" << std::endl;
        good = false;
    } else {
        std::cout << "timeout in ConnectModule " << timeout << std::endl;
        good = true;
    }
    
    // set the member variable to good so that one can always see, if connection was established correctly
    _isConnected = good;

    return good;    
}

bool hvModule::IsKillEnabled(){
    bool killEnabled;
    updateModule();

    killEnabled = _moduleStatus.Bit.IsKillEnable;

    return killEnabled;
}

void hvModule::updateModule(){
    _moduleEventGroupMask = GetModuleEventGroupMask(); 
    // TODO: add more

    _moduleControl.Word     = GetModuleControl();
    _moduleStatus.Word      = GetModuleStatus();
    _moduleEventStatus.Word = GetModuleEventStatus();

    _isStop = _moduleStatus.Bit.IsStop;
}

bool hvModule::SetKillEnable(bool setKillEnable){
    // now we have all values to set up the HV for TOS
    // first set kill enable
    // enable Kill Mode
    std::cout << "trying to set kill enable to: " << setKillEnable << std::endl;
    bool killEnabled = false;

    if(_isConnected){
        // only if connection was establish
        int timeout = 1000;

        while ((killEnabled == false) &&
               (timeout > 0) ) {
            // set kill enable
            SetModuleKillEnable(setKillEnable);
            sleepModule();
            // check if it had any effect
            killEnabled = IsKillEnabled();
            timeout--;
        }

        if (timeout == 0) {
            std::cout << "TIMEOUT: Module could not be set to SetKillEnable" << std::endl;
            // for now throw exception without anything else
            // TODO: set this up properly!!!
            // throw();
        } else {
            std::cout << "timeout in SetKillEnable " << timeout << std::endl;
            std::cout << "Module successfully set to SetKillEnable = " << setKillEnable << std::endl;
        }
    } else {
        std::cout << "connection to module is not established. Connect first" << std::endl;
    }

    return killEnabled;
}

GroupSTRUCT hvModule::GetFlexGroup(int group) {
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
    SetModuleFlexGroupMemberList(group, groupObject.MemberList1.Word);
    
    // we're using Type 1, because in our implementation we're only properly
    // supporting the module with 12 channels
    SetModuleFlexGroupType(group, groupObject.Type1.Word);
}

void hvModule::sleepModule(){
    std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_HV_SLEEP_TIME));
}

bool hvModule::isStop(){
    updateModule();
    return _isStop;
}


bool hvModule::setStopModule(bool stop){
    bool good;

    updateModule();
    if(_isStop == stop){
        // in this case nothing to do, set good
        good = true;
    }

    int timeout = 1000;
    while((timeout > 1) &&
          (_isStop != stop) ) {
        // update module,
        updateModule();
        // set moduleControl bit setStop to 1
        _moduleControl.Bit.SetStop = 1;
        // and write to module
        SetModuleControl(_moduleControl.Word);
    }

    if (timeout < 1) {
        std::cout << "TIMEOUT: could not set module to SetStop == " << good << std::endl;
        good = false;
    } else {
        std::cout << "timeout in setStopModule " << timeout << std::endl;
        good = true;
    }    
    
    return good;
}


void hvModule::printStatus(){
    updateModule();

    std::cout << "IsAdjustment "     << _moduleStatus.Bit.IsAdjustment << "\n"
              << "IsInterlockOut "    << _moduleStatus.Bit.IsInterlockOut << "\n"
              << "IsStop "            << _moduleStatus.Bit.IsStop << "\n"
              << "IsServiceNeeded "   << _moduleStatus.Bit.IsServiceNeeded << "\n"
              << "IsInputError "      << _moduleStatus.Bit.IsInputError << "\n"
              << "IsSpecialMode "     << _moduleStatus.Bit.IsSpecialMode << "\n"
              << "IsCommandComplete " << _moduleStatus.Bit.IsCommandComplete << "\n"
              << "IsNoSumError "      << _moduleStatus.Bit.IsNoSumError << "\n"
              << "IsNoRamp "          << _moduleStatus.Bit.IsNoRamp << "\n"
              << "IsSafetyLoopGood "  << _moduleStatus.Bit.IsSafetyLoopGood << "\n"
              << "IsEventActive "     << _moduleStatus.Bit.IsEventActive << "\n"
              << "IsModuleGood "      << _moduleStatus.Bit.IsModuleGood << "\n"
              << "IsSupplyGood "      << _moduleStatus.Bit.IsSupplyGood << "\n"
              << "IsTemperatureGood " << _moduleStatus.Bit.IsTemperatureGood << "\n"
              << "IsKillEnable "      << _moduleStatus.Bit.IsKillEnable
              << std::endl;
}


bool hvModule::clearModuleEventStatusAndCheck(){
    bool good = false;

    updateModule();
    if(_moduleEventStatus.Word == 0) {
        // in this case nothing to do, set good
        good = true;
    }

    // define an empty module event status and initialize to 0
    ModuleEventStatusSTRUCT moduleEventStatus = { };

    int timeout = 1000;
    while((timeout > 1) &&
          (_moduleEventStatus.Word != moduleEventStatus.Word)) {
        // call clear module event status function
        ClearModuleEventStatus();
        // sleep for a moment
        sleepModule();
        // and update update module to check in next iteration, if
        // _moduleEventStatus member variable is empty now
        updateModule();
    }
    if (timeout < 1){
        std::cout << "TIMEOUT: could not reset module event." << std::endl;
        good = false;
    } else {
        std::cout << "timeout in clearModuleEventStatus " << timeout << std::endl;
        good = true;
    }    
    
    return good;
}


void hvModule::printEventStatus(){
    updateModule();

    std::cout << "EventRestart "            << _moduleEventStatus.Bit.EventRestart << "\n"
              << "EventServiceNeeded "      << _moduleEventStatus.Bit.EventServiceNeeded << "\n"
              << "EventInputError "         << _moduleEventStatus.Bit.EventInputError << "\n"
              << "EventSafetyLoopNotGood "  << _moduleEventStatus.Bit.EventSafetyLoopNotGood << "\n"
              << "EventSupplyNotGood "      << _moduleEventStatus.Bit.EventSupplyNotGood << "\n"
              << "EventTemperatureNotGood " << _moduleEventStatus.Bit.EventTemperatureNotGood
              << std::endl;
}


bool hvModule::isEventStatusGood(){
    updateModule();
    bool good;
    
    if (_moduleEventStatus.Word == 0){
        good = true;
    } else {
        good = false;
    }

    std::bitset<16> bits(_moduleEventStatus.Word);
    std::cout << "bits of moduleEventStatus " << bits.to_string() << std::endl;
    return good;
}
