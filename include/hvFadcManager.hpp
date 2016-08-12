#ifndef HV_FADC_MANAGER_HPP
#define HV_FADC_MANAGER_HPP 1

// This header file describes the HV FADC manager, which
// is a convenience object for the combined communication with
// the ISEG HV module and the CAEN FADC
// (vmemodule controls the ISEG HV)
// This may be called in a program to access 
// the HV and control it by simple functions without having
// to worry about sending commands and not knowing whether
// they were actually done

// written by Sebastian Schmidt

// headers related to CAEN FADC
#include "V1729a_VME.h"
#include "High-Level-functions_VME.h"

// custom headers related to iseg module
#include "hvInterface/hvModule.hpp"
#include "hvInterface/hvChannel.hpp"
#include "hvInterface/hvFlexGroup.hpp"
#include "hvInterface/hvSetGroup.hpp"
#include "hvInterface/hvStatusGroup.hpp"
#include "hvInterface/hvMonitorGroup.hpp"
#include "const.h"

// C++
#include <sys/time.h>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <list>
#include <set>

// we need to include unistd.h, to get get_current_dir_name()
#include <unistd.h>

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
#define DEFAULT_ANODE_VOLTAGE_BOUND                               2.5
#define DEFAULT_ANODE_CURRENT_SET                                 0.005 // in mA
#define DEFAULT_ANODE_CURRENT_NOMINAL                             0.005 // in mA
#define DEFAULT_ANODE_CURRENT_BOUND                               2.5

#define DEFAULT_CATHODE_VOLTAGE_SET                               1845
#define DEFAULT_CATHODE_VOLTAGE_NOMINAL                           1845
#define DEFAULT_CATHODE_VOLTAGE_BOUND                             2.5
#define DEFAULT_CATHODE_CURRENT_SET                               0.010 // in mA
#define DEFAULT_CATHODE_CURRENT_NOMINAL                           0.010 // in mA
#define DEFAULT_CATHODE_CURRENT_BOUND                             0

#define DEFAULT_MODULE_VOLTAGE_RAMP_SPEED                         1
#define DEFAULT_MODULE_CURRENT_RAMP_SPEED                         50
#define DEFAULT_CHECK_MODULE_TIME_INTERVAL                        60

#define DEFAULT_VOLTAGE_BOUND                                     2.5
#define DEFAULT_VOLTAGE_NOMINAL         	                  4000
#define DEFAULT_CURRENT_SET                                       0.000050
#define DEFAULT_CURRENT_NOMINAL         	                  0.000500


// FADC macros
#define DEFAULT_FADC_TRIGGER_TYPE                                 7
#define DEFAULT_FADC_FREQUENCY                                    2
#define DEFAULT_FADC_POSTTRIG                                     80
#define DEFAULT_FADC_PRETRIG                                      15000
#define DEFAULT_FADC_TRIGGER_THRESHOLD_REGISTER_ALL               2033
#define DEFAULT_FADC_PEDESTAL_RUN_TIME                            100 // in milli seconds
#define DEFAULT_FADC_PEDESTAL_NUM_RUNS                            10 // number of pedestal runs for one calibration
#define DEFAULT_FADC_CHANNEL_SOURCE                               1 // channel 0 is trigger source, bit 0 = 1

// HV macros
#define DEFAULT_HV_SLEEP_TIME                                     10 // in milli seconds

// reading INI file macro
#define MAX_INI_PATH_LENGTH                                       300


// forward declare some 
// class hvChannel;
// class hvFlexGroup;
// class hvSetGroup;
// class hvStatusGroup;
// class hvMonitorGroup;


// note: abbreveations used:
//       HFM -- hvFadcManager
class hvFadcManager
{
public:
    // Creator
    // create by reading from ini file
    hvFadcManager(std::string iniFilePath);
    // baseAddress_hv is the Address of the HV
    // default: 0x4000

    // Destructor
    virtual ~hvFadcManager();

    /*************** Functions      ***************/

    // what does this need to be able to do?
    //**************************************************
    //************ General purpose functions ***********
    //************ and convenience functions ***********
    //************ based on module functions ***********
    //**************************************************
    
    // this function returns the pointer of the created hvModule
    // useful if one wants to add channels externally
    hvModule* getHVModulePtr();

    void ReadHVSettings();
    void InitHFMForTOS(); 
    void ShutDownHFMForTOS();
    void SetFadcSettings();

    // adds channel to _channelMap
    void AddChannel(hvChannel *ptrChannel);
    // adds flex group
    void AddFlexGroup(hvFlexGroup *ptrGroup);

    // function to remove a channel from the _channelList and shut that channel down
    void RemoveChannelByName(std::string channelName);
    void RemoveChannelByNumber(int channelNumber);
    // create a simple channel from a channel name and a target voltage
    bool CreateChannel(std::string channelName, int voltage);




    // function to create the binary representation of a set of numbers (channels, groups...) needed for module
    uint16_t GetBinaryRepFromNumberSet(std::set<int> numberSet);

    // inverse function to get binary rep from channels set
    // TODO: think unnecessary
    std::set<int> GetChannelSetFromBinaryRep(uint32_t binaryRep);

    // get a group struct from string identifier of a group
    // TODO: think unecessary
    GroupSTRUCT *GetFlexGroupStructFromString(std::string stringIdentifier);

    void CheckModuleIsRamping(bool rampUpFlag);
    bool CheckAllChannelsInVoltageBound();
    bool CheckToShutdown();
    bool SetAllChannelsOn();
    bool ClearAllChannelEventStatus();
    bool SetModuleEventGroupMask();
    void H_SetNominalValues();

    bool H_ConnectModule();
    bool H_CheckIsConnectionGood();
    // this function is used during a Run in order to check
    // whether the HV module is good
    // this is done every checkModuleTimeInterval (from 
    // HFOSettings.ini) seconds
    // TODO: change!
    int H_CheckHVModuleIsGood(bool verbose = true);
    void H_DumpErrorLogToFile(int event);

    // These two functions are convenience functions, which 
    // write a group (from a GroupSTRUCT struct to the 
    // HV module. Internally it simply calls both the
    // H_SetModuleFlexGroupType and H_SetModuleFlexGroupMemberList
    // functions, which write 32bit to the module.
    void H_SetFlexGroup(int group, GroupSTRUCT groupObject);
    GroupSTRUCT H_GetFlexGroup(int group);

    // printing functions
    void printAllChannelStatus();
    void printAllChannelEventStatus();
    void printModuleStatus();
    void printModuleEventStatus();
    // prints all active channels (and also returns set of string numbers)
    std::set<std::string> PrintActiveChannels();
    // function to call printing function of specific channel
    void PrintChannel(int channelNumber);

    // function to turn single channel on or off
    void TurnChannelOnOff();
    // function to set a specific value of a specific channel (voltage, current ...)
    void SetChannelValue(std::string key, int channelNumber, std::string value);
    // clear channel event status of single channel
    bool ClearChannelEventStatus();

    // functions related to voltage scheduler:
    void AddElementToVoltageScheduler(float voltage, int time);
    void AddElementToVoltageScheduler(std::pair<int, int> voltageTimePair);
    void RemoveLastElementFromVoltageScheduler();
    void RemoveFirstElementFromVoltageScheduler();
    void RunVoltageScheduler(std::string channelIdentifier, std::string groupIdentifier);

    void sleepModule(int sleepTime = 0, std::string unit = "milliseconds");
    void setSleepAcqTime(int time);
    void setSleepTriggerTime(int time);


    // ##################################################
    // general helper functions
    // ##################################################
    // function to build a filename for pedestal run, normal data run or single frame
    std::string buildFileName(std::string filePath, bool pedestalRunFlag);

    // ##################################################
    // FADC custom functions
    std::map<std::string, int> GetFadcParameterMap();
    // function to do a pedestal calibration run of the FADC
    void StartFadcPedestalRun();
    // function to write FADC data to 'filename'
    bool writeFadcData(std::string filename, std::map<std::string, int> fadcParams, std::vector<int> fadcData);



    //**************************************************
    /*************** HV functions   *******************/
    //**************************************************
    /* These functions just redefine HV_module functions
       as functions of the hvFadcManager 
       ALL of these functions start with H_  */

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
    uint32_t       H_GetChannelStatus(int channel);
    ChStatusSTRUCT H_GetChannelStatusStruct(int channel);
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

    // need a HighLevelFunction_VME object to use high level functions
    // of FADC. In Public, because...
    HighLevelFunction_VME* FADC_Functions;


private:
    /*************** Member modules ***************/
    /* TODO: implement all functions of interest to us 
       as functions of hvFadcManager and declare modules as
       private objects */
    
    // HV member, based on hvModule based on VMEmodule (by ISEG)
    hvModule *HV_module;
    // FADC member, based on V1729a_VME class (by Alexander Deisting)
    V1729a_VME *FADC_module;
    // and a USB controller for communication
    CVmUsb Controller;

    // list which contains all set flex groups
    std::list<hvFlexGroup *> _flexGroupList;

    // list of channels in use (added by AddChannel)
    std::list<hvChannel *> _channelList;

    // voltageScheduler is a list of pairs containing voltages
    // and times (in minutes). the list is worked through from beginning
    // to end, ramping to first of pair and wait second of pair minutes
    // then, next element is taken 
    std::list<std::pair<float, int>> _voltageScheduler;

    // base addresses for devices
    int baseAddress_hv;
    int sAddress_fadc;

    // flags
    bool _createdFlag;
    bool _settingsReadFlag;
    bool _hvModInitFlag;
    bool anodeGridGroupFlag;
    bool setKillEnable;
    bool monitorTripGroupFlag;
    bool rampingGroupFlag;


    uint32_t _moduleEventGroupMask;

    // monitor variables
    int checkModuleTimeInterval;
    // the event status variables which were current in the 
    // last call of H_CheckHVModuleIsGood()
    ChEventStatusSTRUCT gridEventStatusLastIter;
    ChEventStatusSTRUCT anodeEventStatusLastIter;
    ChEventStatusSTRUCT cathodeEventStatusLastIter;

    int _sleepTime;

    // module settings
    float moduleVoltageRampSpeed;
    float moduleCurrentRampSpeed;

    // ##################################################
    // TOS related variables
    // ##################################################
    
    QString iniFile;

    // group related
    GroupSTRUCT* anodeGridSetOnGroup;
    GroupSTRUCT* monitorTripGroup;
    // ramping group is a Status Group, which checks
    // whether the channels are still ramping
    GroupSTRUCT* rampingGroup;
    //int anodeGridGroup;
    int anodeGridGroupMasterChannel;
    int anodeGridGroupNumber;
    int monitorTripGroupNumber;
    int rampingGroupNumber;

    uint16_t anodeChannelNumber;
    uint16_t gridChannelNumber;
    uint16_t cathodeChannelNumber;

    // the basic event mask we usually set for all channels
    std::set<std::string> _eventMaskSet;


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
    int fadcPedestalRunTime;
    int fadcPedestalNumRuns;
    int fadcChannelSource;


    int _sleepAcqTime;
    int _sleepTriggerTime;
    
    // special functions have no need to be public
    // --- HV Special Control ----------------------------------------------------
    void   H_SetModuleSpecialControlCommand(uint32_t command);
    uint32_t H_GetModuleSpecialControlCommand(void);
    uint32_t H_GetModuleSpecialControlStatus(void);
    void   H_SendHexLine(QByteArray record);

    void   H_ProgramModuleBaseAddress(uint16_t address);
    uint16_t H_VerifyModuleBaseAddress(void);



  
};






#endif

