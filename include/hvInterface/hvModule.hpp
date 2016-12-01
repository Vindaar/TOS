#ifndef HV_MODULE_HPP
#define HV_MODULE_HPP 1

// Include CVmeModule class
#include "vmemodule.h"

// Include the hardware controller for the CVmeModule
#include "vmecontroller.h"

// Include the usb controller (includes CVmUsb)
#include "vmusb.h"

// iseg definitions
#include "const.h"

// C++
#include <chrono>
#include <thread>


// HV macros
#define DEFAULT_HV_SLEEP_TIME                                     10 // in milli seconds


/**
 * This class is a slight abstraction (and derived of) of the class vmemodule and is used
 * for the communication to the module.
 *
 * Before you can use this class, call ConnectModule.
 */
class hvModule: public CVmeModule
{
public:
    hvModule(CVmeController *vmeController, int baseAddress);
    ~hvModule();

    // TODO: change
    void CheckModuleIsRamping(bool rampUpFlag);

    /** Try to connect to the module.
     *
     * @return true, if connected. false otherwise.
     */
    bool ConnectModule();

    /** Check whether connection was successfully established to the module.
     *
     * This function checks, whether the connection to the module
     * could be established and if not, try for a certain time.
     * if we cannot establish connection, return false.
     * Since it's exactly the same thing we do, when we try to connect to the module
     * for the first time, this is just a wrapper around that function.
     *
     * @return True, if we can establish a connection. False otherwise.
     */
    bool CheckIsConnectionGood();

    /** Check whether kill is enabled.
     *
     * This function simply gets the module status of the module (by updating module)
     * and checks whether the IsKillEnable bit is activated.
     */
    bool IsKillEnabled();

    /* Set kill to enable.
     *
     * This function trys to set kill enable flag to value given to function.
     */
    bool SetKillEnable(bool setKillEnable);

    /* Set the stop bit and check if that happens.
     *
     * This function gets the current module status, by updating the module
     * checks if the isStop bit is set to stop, if not, get module control
     * set bit the way we want and set.
     */
    bool setStopModule(bool stop);

    /** Call both get flex group functions from the module.
     *
     * @author Sebastian Schmidt
     * @return the group built by the module
     */
    GroupSTRUCT GetFlexGroup(int group);

    /** Call both set flex group functions from the module and write them to the HV module.
     *
     * @author Sebastian Schmidt
     */
    void SetFlexGroup(int group, GroupSTRUCT groupObject);

    /** Update module member variables.
     *
     * This function updates the module member variables, i.e. it reads all
     * functions for which the hvModule has a member variable.
     */
    void updateModule();

    /** Sleep a given time.
     *
     * Convenience function which calls sleep for this thread for a constant time.
     */
    void sleepModule();

    /** Return isStop bit from moduleStatus.
     */
    bool isStop();

    void H_SetNominalValues();
    //bool ClearAllChannelEventStatus();

    /* Call the IsConnected() function from vmemodule.h.
     *
     * @return 0, if no connection established. 1, if connection is good.
     */
    int H_ConnectModule();

    /** Print the current module status.
     */
    void printStatus();

    /** Print the current module event status.
     */
    void printEventStatus();

    /** Reset the module event status.
     *
     * This function trys to clear the module event status.
     * Calls ClearModuleEventStatus() defined in vmemodule.h and checks, if done correctly.
     */
    bool clearModuleEventStatusAndCheck();

    /** Check the event status.
     *
     * This function is a simple check on the module event status.
     *
     * @return true, if no event status bit is set, false otherwise.
     */
    bool isEventStatusGood();

private:

    CVmUsb *Controller;

    ModuleControlSTRUCT _moduleControl;
    ModuleStatusSTRUCT  _moduleStatus;
    ModuleEventStatusSTRUCT _moduleEventStatus;
    

    /** variables for addresses of each device
     */
    int baseAddress_hv;
    
    /**
     * True, if connection was established correctly. False otherwise (default at creation).
     */
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

    /** flag which saves, if we created our own vmeController
     */
    bool _createdOwnVmeController;
};

#endif
