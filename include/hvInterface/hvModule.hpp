#ifndef HV_MODULE_HPP
#define HV_MODULE_HPP 1

// this header defines the HV module
// it is a slight abstraction (and derived of) of the vmemodule and is used
// for the communication to the module

// CVmeModule class contained in
#include "vmemodule.h"
/* need to include the hardware controller for the CVmeModule */
#include "vmecontroller.h"
// and the usb controller (includes CVmUsb)
#include "vmusb.h"

// iseg definitions
#include "const.h"

// C++
#include <chrono>
#include <thread>


// HV macros
#define DEFAULT_HV_SLEEP_TIME                                     10 // in milli seconds

class hvModule: public CVmeModule
{
public:
    // aside from everything the vmemodule owns, we need:
    // creator and destructor
    hvModule(CVmeController *vmeController, int baseAddress);
    ~hvModule();

    // TODO: change
    void CheckModuleIsRamping(bool rampUpFlag);

    bool ConnectModule();
    // checks whether connection was successfully established to module
    bool CheckIsConnectionGood();
    // checks whether kill is enabled
    bool IsKillEnabled();
    // sets kill to enable
    bool SetKillEnable(bool setKillEnable);

    // this function sets the stop bit and checks if that happens
    bool setStopModule(bool stop);
    GroupSTRUCT GetFlexGroup(int group);
    void SetFlexGroup(int group, GroupSTRUCT groupObject);
    // function to update module member variables
    void updateModule();
    // convenience function to sleep time given by macro in this header
    void sleepModule();

    bool isStop();

    void H_SetNominalValues();
    bool ClearAllChannelEventStatus();
    int H_ConnectModule();

private:

    CVmUsb *Controller;

    ModuleControlSTRUCT _moduleControl;
    ModuleStatusSTRUCT  _moduleStatus;

    // variables for addresses of each device
    int baseAddress_hv;
    
    // member variable which is set to unequal 0, if conncetion was established correctly
    // and 0 if not (set to 0 by creator first)
    bool _isConnected;
    bool _hvModInitFlag;
    bool _hvModCreatedFlag;
    bool _killEnabled;
    
    bool _isStop;


    // module settings
    float moduleVoltageRampSpeed;
    float moduleCurrentRampSpeed;

    // module related variables
    uint32_t _moduleEventGroupMask;

    // flag which saves, if we created our own vmeController
    bool _createdOwnVmeController;


};



#endif
