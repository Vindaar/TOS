#ifndef HV_FADC_Obj_H
#define HV_FADC_Obj_H 1

// This header file describes the HV_FADC object, which
// combines the V1729a_VME and vmemodule modules into
// a single module.
// This may be called in a program to access both 
// devices sitting in one VME crate over a single
// USB controller
// To be used in TOS

// written by Sebastian Schmidt


// include both modules which are to be combined in a single class
#include "V1729a_VME.h"
// CVmeModule class contained in
#include "vmemodule.h"
/* need to include the hardware controller for the CVmeModule */
#include "vmecontroller.h"
// and the usb controller (includes CVmUsb)
#include "vmusb.h"

// include High Level Functions
#include "High-Level-functions_VME.h"

// C++
#include <vector>
#include <string>

// QT
#include <QString>

// included for user input function
#include "tosCommandCompletion.hpp"

// define a few macros
#define DEFAULT_BASE_ADDRESS_HV                                   0x4400
#define DEFAULT_S_ADDRESS_FADC                                    1
#define DEFAULT_SET_KILL_ENABLE                                   true
#define DEFAULT_ANODE_GRID_GROUP_FLAG                             true
#define DEFAULT_ANODE_GRID_GROUP_NUMBER                           0
#define DEFAULT_MONITOR_TRIP_GROUP_FLAG                           true
#define DEFAULT_MONITOR_TRIP_GROUP_NUMBER                         1
#define DEFAULT_RAMPING_GROUP_FLAG                                true
#define DEFAULT_RAMPING_GROUP_NUMBER                              2
#define DEFAULT_GRID_CHANNEL_NUMBER                               0
#define DEFAULT_ANODE_CHANNEL_NUMBER                              1
#define DEFAULT_CATHODE_CHANNEL_NUMBER                            2
#define DEFAULT_ANODE_GRID_GROUP_MASTER_CHANNEL                   0 // grid is master channel as default
#define DEFAULT_ANODE_GRID_GROUP_MODE                             1

#define DEFAULT_GRID_VOLTAGE_SET                                  295
#define DEFAULT_GRID_VOLTAGE_NOMINAL                              295
#define DEFAULT_GRID_VOLTAGE_BOUND                                0
#define DEFAULT_GRID_CURRENT_SET                                  0.005 // in mA
#define DEFAULT_GRID_CURRENT_NOMINAL                              0.005 // in mA
#define DEFAULT_GRID_CURRENT_BOUND                                0

#define DEFAULT_ANODE_VOLTAGE_SET                                 345
#define DEFAULT_ANODE_VOLTAGE_NOMINAL                             345
#define DEFAULT_ANODE_VOLTAGE_BOUND                               0
#define DEFAULT_ANODE_CURRENT_SET                                 0.005 // in mA
#define DEFAULT_ANODE_CURRENT_NOMINAL                             0.005 // in mA
#define DEFAULT_ANODE_CURRENT_BOUND                               0

#define DEFAULT_CATHODE_VOLTAGE_SET                               1845
#define DEFAULT_CATHODE_VOLTAGE_NOMINAL                           1845
#define DEFAULT_CATHODE_VOLTAGE_BOUND                             0
#define DEFAULT_CATHODE_CURRENT_SET                               0.010 // in mA
#define DEFAULT_CATHODE_CURRENT_NOMINAL                           0.010 // in mA
#define DEFAULT_CATHODE_CURRENT_BOUND                             0

#define DEFAULT_MODULE_VOLTAGE_RAMP_SPEED                         1
#define DEFAULT_MODULE_CURRENT_RAMP_SPEED                         50
#define DEFAULT_CHECK_MODULE_TIME_INTERVAL                        60

// FADC macros
#define DEFAULT_FADC_TRIGGER_TYPE                                 7
#define DEFAULT_FADC_FREQUENCY                                    2
#define DEFAULT_FADC_POSTTRIG                                     80
#define DEFAULT_FADC_PRETRIG                                      15000
#define DEFAULT_FADC_TRIGGER_THRESHOLD_REGISTER_ALL               2033

// HV macros
#define DEFAULT_HV_SLEEP_TIME                                     10 // in milli seconds



class HV_FADC_Obj
{
  public:
    // Creator
    // either create by using addresses
    HV_FADC_Obj(int sAddress_fadc, int baseAddress_hv, std::string iniFilePath);
    // or reading everything from .ini file (and calling InitHFOForTOS)
    HV_FADC_Obj(std::string iniFilePath);
    // sAddress_fadc is a multiplicative number for the FADC
    // Address (which on default is set to 0x10000)
    // baseAddress_hv is the Address of the HV
    // default: 0x4400

    // Destructor
    virtual ~HV_FADC_Obj();

    /*************** Functions      ***************/




    // what does this need to be able to do?
    //**************************************************
    //************ General purpose functions ***********
    //************ and convenience functions ***********
    //************ based on module functions ***********
    //**************************************************

    void ReadHFOSettings();
    void InitHFOForTOS();
    void ShutDownHFOForTOS();
    void H_CheckModuleIsRamping(bool rampUpFlag);
    
    // this function is used during a Run in order to check
    // whether the HV module is good
    // this is done every checkModuleTimeInterval (from 
    // HFOSettings.ini) seconds
    int H_CheckHVModuleIsGood(bool verbose = true);
    void H_DumpErrorLogToFile(int event);

    void H_SetNominalValues();
    int H_ClearAllChannelEventStatus();
    int H_ConnectModule();
    
    // These two functions are convenience functions, which 
    // write a group (from a GroupSTRUCT struct to the 
    // HV module. Internally it simply calls both the
    // H_SetModuleFlexGroupType and H_SetModuleFlexGroupMemberList
    // functions, which write 32bit to the module.
    void H_SetFlexGroup(int group, GroupSTRUCT groupObject);
    GroupSTRUCT H_GetFlexGroup(int group);






    //**************************************************
    /*************** FADC functions *******************/
    //**************************************************
    // These functions just redefine FADC_module functions
    // as functions of the HV_FADC_Obj 
    // ALL of these functions start with F_ for FADC 
    // For descriptions of these functions take a look at
    // "V1729a_VME.h"
    // CAREFUL: Some functions still do not work correctly,
    //          judging by the comments in the "V1729a_VME.h"
    
    void         F_Reset() throw();
    void         F_StartAcquisition() throw();
    void         F_SetFrequency( const unsigned short& frequency ) throw();
    unsigned int F_GetFrequency() throw();
    virtual void F_SetReadMode( const unsigned short& mode ) throw();
    virtual unsigned int F_GetReadMode() throw();
    unsigned int F_GetFPGAVersion() throw();
     /***********  F_Trigger settings  ***********/
    void         F_SetTriggerThresholdDACAll(const unsigned int threshold) throw();
    unsigned int F_GetTriggerThresholdDACAll() throw();
    void         F_SetTriggerThresholdDACPerChannel(const unsigned short chNb, const unsigned int threshold) throw();
    unsigned int F_GetTriggerThresholdDACPerChannel(const unsigned short chNb) throw();
    void         F_LoadTriggerThresholdDAC() throw();
    unsigned int F_GetTriggerRecord() throw();
    void         F_SendSoftwareTrigger() throw();
    void         F_SetPretrig( const unsigned short& bits ) throw();
    unsigned short F_GetPretrig() throw();
    void         F_SetPosttrig( const unsigned short& bits ) throw();
    unsigned int F_GetPosttrig() throw();
    void         F_SetTriggerType( const unsigned short& type ) throw();
    unsigned short F_GetTriggerType() throw();
    void         F_SetTriggerChannelSource( const unsigned short& mask ) throw();
    unsigned short F_GetTriggerChannelSource() throw();
    /***********  Readout settings  ***********/
    unsigned short F_GetModeRegister() throw();
    void           F_SetModeRegister(const unsigned short& mode) throw();
    void           F_SetColToRead( const unsigned short& col ) throw();
    unsigned short F_GetColToRead() throw();
    void           F_SetChannelMask( const unsigned short& mask ) throw();
    unsigned short F_GetChannelMask() throw();
    void           F_SetNbOfChannels(const unsigned short& nbOfChannels) throw();
    unsigned short F_GetNbOfChannels() throw();
    void           F_SetPostStopLatency( const unsigned short& latency ) throw();
    unsigned short F_GetPostStopLatency() throw();
    void           F_SetPostLatencyPretrig( const unsigned short& latency ) throw();
    unsigned short F_GetPostLatencyPretrig() throw();
    void           F_SetRAMAddress( const unsigned short& add ) throw();
    unsigned short F_GetRAMAddress() throw();
    std::vector< int > F_GetAllData( const unsigned short& nbChannels) throw();
    std::vector< int > F_GetAllDataBlockMode() throw();
    int            F_GetDataAt( const unsigned short& address = 0 ) throw();
    /***********  Interrupt settings  ***********/
    void           F_ReleaseInterrupt() throw();
    bool           F_ReadDeviceInterrupt() throw();
    bool           F_ReadInterrupt() throw();







    //**************************************************
    /*************** HV functions   *******************/
    //**************************************************
    /* These functions just redefine HV_module functions
       as functions of the HV_FADC_Obj 
       ALL of these functions start with H_ for FADC */

    bool   H_IsConnected(void) { return HV_module->IsConnected(); }

    // --- HV Module commands ----------------------------------------------------
    int    H_GetModuleDeviceClass(void);
    uint32_t H_GetModulePlacedChannels(void);
    int    H_GetModuleSerialNumber(void);
    uint32_t H_GetModuleFirmwareRelease(void);

    void   H_DoClear(void);
    uint16_t H_GetModuleBaseAddress() { return HV_module->GetModuleBaseAddress(); }
    void   H_SetModuleBaseAddress(uint16_t ba);

    float  H_GetModuleTemperature(void);
    float  H_GetModuleSupplyP5(void);
    float  H_GetModuleSupplyP12(void);
    float  H_GetModuleSupplyN12(void);

    uint32_t H_GetModuleAdcSamplesPerSecond(void);
    void   H_SetModuleAdcSamplesPerSecond(uint32_t sps);
    uint32_t H_GetModuleDigitalFilter(void);
    void   H_SetModuleDigitalFilter(uint32_t filter);

    float  H_GetModuleVoltageLimit(void);
    float  H_GetModuleCurrentLimit(void);

    uint32_t H_GetModuleStatus(void);
    uint32_t H_GetModuleEventStatus(void);
    void   H_SetModuleEventStatus(uint32_t status);
    void   H_ClearModuleEventStatus(void) { HV_module->SetModuleEventStatus(0xFFFFFFFF); }
    uint32_t H_GetModuleEventChannelStatus(void);
    void   H_ClearModuleEventChannelStatus(void);
    uint32_t H_GetModuleEventChannelMask(void);
    void   H_SetModuleEventChannelMask(uint32_t mask);
    uint32_t H_GetModuleEventGroupStatus(void);
    uint32_t H_GetModuleEventGroupMask(void);
    void   H_SetModuleEventGroupMask(uint32_t mask);
    void   H_ClearModuleEventGroupStatus(void);
    uint32_t H_GetModuleEventMask(void);
    void   H_SetModuleEventMask(uint32_t mask);
    uint32_t H_GetModuleControl(void);
    void   H_SetModuleControl(uint32_t control);

    float  H_GetModuleVoltageRampSpeed(void);
    void   H_SetModuleVoltageRampSpeed(float vramp);
    float  H_GetModuleCurrentRampSpeed(void);
    void   H_SetModuleCurrentRampSpeed(float iramp);

    bool   H_GetModuleKillEnable(void);
    void   H_SetModuleKillEnable(bool enable);

    int    H_GetModuleChannelNumber(void);

    void   H_SetModuleEmergencyOff(void);
    void   H_ClearModuleEmergencyOff(void);

    uint32_t H_GetModuleRestartTime(void);
    void   H_SetModuleRestartTime(uint32_t restartTime);

    void   H_SetAllChannelsOn(void);
    void   H_SetAllChannelsOff(void);
    void   H_SetAllChannelsVoltageSet(float vset);
    void   H_SetAllChannelsCurrentSet(float iset);

    // --- HV Channel commands ---------------------------------------------------
    uint32_t H_GetChannelStatus(int channel);
    uint32_t H_GetChannelEventStatus(int channel);
    void   H_SetChannelEventStatus(int channel, uint32_t status);
    void   H_ClearChannelEventStatus(int channel) { HV_module->SetChannelEventStatus(channel, 0xFFFFFFFF); }

    uint32_t H_GetChannelEventMask(int channel);
    void   H_SetChannelEventMask(int channel, uint32_t mask);
    uint32_t H_GetChannelControl(int channel);
    void   H_SetChannelControl(int channel, uint32_t control);

    void   H_SetChannelEmergencyOff(int channel);
    void   H_ClearChannelEmergencyOff(int channel);

    float  H_GetChannelVoltageNominal(int channel);
    float  H_GetChannelCurrentNominal(int channel);

    float  H_GetChannelVoltageSet(int channel);
    void   H_SetChannelVoltageSet(int channel, float vset);
    float  H_GetChannelVoltageMeasure(int channel);
    float  H_GetChannelCurrentSet(int channel);
    void   H_SetChannelCurrentSet(int channel, float iset);
    float  H_GetChannelCurrentMeasure(int channel);

    float  H_GetChannelVoltageIlkMaxSet(int channel);
    void   H_SetChannelVoltageIlkMaxSet(int channel, float ilkmax);
    float  H_GetChannelVoltageIlkMinSet(int channel);
    void   H_SetChannelVoltageIlkMinSet(int channel, float ilkmin);

    float  H_GetChannelCurrentIlkMaxSet(int channel);
    void   H_SetChannelCurrentIlkMaxSet(int channel, float ilkmax);
    float  H_GetChannelCurrentIlkMinSet(int channel);
    void   H_SetChannelCurrentIlkMinSet(int channel, float ilkmin);

    void   H_SetChannelOn(int channel);
    void   H_SetChannelOff(int channel);

    // --- HV Convenience commands -----------------------------------------------
    QString H_FormatVoltage(double v);
    QString H_FormatVoltageUnit(double v);
    QString H_GetVoltageUnit(void);

    QString H_FormatCurrent(double i);
    QString H_FormatCurrentUnit(double i);
    QString H_GetCurrentUnit(void);

    // --- HV Interlock Out control / status commands ----------------------------
    uint32_t H_GetModuleIlkOutStatus(void);
    uint32_t H_GetModuleIlkOutControl(void);
    void   H_SetModuleIlkOutControl(uint32_t control);
    uint32_t H_GetModuleIlkOutCount(void);
    uint32_t H_GetModuleIlkOutLastTrigger(void);
    uint32_t H_GetModuleIlkOutChnActualActive(void);
    uint32_t H_GetModuleIlkOutChnEverTriggered(void);

    // --- HV Variable Groups ----------------------------------------------------
    uint32_t H_GetModuleFlexGroupMemberList(int group);
    void   H_SetModuleFlexGroupMemberList(int group, uint32_t member);
    uint32_t H_GetModuleFlexGroupType(int group);
    void   H_SetModuleFlexGroupType(int group, uint32_t type);

    float  H_GetChannelVoltageHardwareNominal(int channel);
    void   H_SetChannelVoltageNominal(int channel, float maxset);
    float  H_GetChannelCurrentHardwareNominal(int channel);
    void   H_SetChannelCurrentNominal(int channel, float maxset);

    // --- HV Special Control ----------------------------------------------------
    void   H_SetModuleSpecialControlCommand(uint32_t command);
    uint32_t H_GetModuleSpecialControlCommand(void);
    uint32_t H_GetModuleSpecialControlStatus(void);
    void   H_SendHexLine(QByteArray record);

    void   H_ProgramModuleBaseAddress(uint16_t address);
    uint16_t H_VerifyModuleBaseAddress(void);









    // need a HighLevelFunction_VME object to use high level functions
    // of FADC. In Public, because...
    HighLevelFunction_VME* FADC_Functions;







  private:
    /*************** Member modules ***************/
    /* TODO: implement all functions of interest to us 
       as functions of HV_FADC_Obj and declare modules as
       private objects */
    
    // FADC member, based on V1729a_VME class (by Alexander Deisting)
    V1729a_VME *FADC_module;
    // HV member, based on VMEmodule (by ISEG)
    CVmeModule *HV_module;

    CVmUsb Controller;

    // variables for addresses of each device
    int baseAddress_hv;
    int sAddress_fadc;

    QString iniFile;
    // group related
    GroupSTRUCT anodeGridSetOnGroup;
    GroupSTRUCT monitorTripGroup;
    // ramping group is a Status Group, which checks
    // whether the channels are still ramping
    GroupSTRUCT rampingGroup;
    //int anodeGridGroup;
    int anodeGridGroupMasterChannel;
    int anodeGridGroupNumber;
    int monitorTripGroupNumber;
    int rampingGroupNumber;

    uint16_t anodeChannelNumber;
    uint16_t gridChannelNumber;
    uint16_t cathodeChannelNumber;

    // flags
    bool hvFadcObjCreatedFlag;
    bool hvFadcObjSettingsReadFlag;
    bool hvFadcObjInitFlag;
    bool setKillEnable;
    bool anodeGridGroupFlag;
    bool monitorTripGroupFlag;
    bool rampingGroupFlag;

    // module settings
    float moduleVoltageRampSpeed;
    float moduleCurrentRampSpeed;

    // monitor variables
    int checkModuleTimeInterval;
    // the event status variables which were current in the 
    // last call of H_CheckHVModuleIsGood()
    ChEventStatusSTRUCT gridEventStatusLastIter = { 0 };
    ChEventStatusSTRUCT anodeEventStatusLastIter = { 0 };
    ChEventStatusSTRUCT cathodeEventStatusLastIter = { 0 };

    

    // voltages and currents for grid, anode and cathode
    float gridVoltageSet;
    float gridVoltageNominal;
    float gridVoltageBound;
    float gridCurrentSet;
    float gridCurrentNominal;
    float gridCurrentBound;

    float anodeVoltageSet;
    float anodeVoltageNominal;
    float anodeVoltageBound;
    float anodeCurrentSet;
    float anodeCurrentNominal;
    float anodeCurrentBound;

    float cathodeVoltageSet;
    float cathodeVoltageNominal;
    float cathodeVoltageBound;
    float cathodeCurrentSet;
    float cathodeCurrentNominal;
    float cathodeCurrentBound;

    // Fadc settings variables
    int fadcTriggerType;
    int fadcFrequency;
    int fadcPosttrig;
    int fadcPretrig;
    int fadcTriggerThresholdRegisterAll;
    
  
};






#endif

