#include "HV_FADC_Obj.h"
#include <QDir>
#include <QSettings>
#include <QCoreApplication>
#include "const.h"


// FUN TEST

// used to control while loops to read / write to HV and
// to create error logs
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
// used to write error logs
#include <fstream>

// either create object using addresses for FADC and HV
HV_FADC_Obj::HV_FADC_Obj(int sAddress_fadc, int baseAddress_hv, std::string iniFilePath){
    // upon initialisation of HV_FADC_Obj, we need to create the
    // V1729a_VME (FADC) instance and vmemodule (HV) instance
    // using the base Addresses

    // TODO: make sure that this baseAddress_hv is actually the one
    // given to the function and not the private int variable
    // of the object itself with the same name
    // initialize flag to false
    hvFadcObjSettingsReadFlag = false;
    hvFadcObjInitFlag         = false;

    iniFile = QString::fromStdString(iniFilePath);

    // initialize Controller
    Controller.initController(0);
    HV_module   = new CVmeModule(&Controller, baseAddress_hv);

    std::cout << "creating.. " << HV_module->IsConnected() <<std::endl;
    std::cout << "creating2.. " << HV_module->IsConnected() <<std::endl;


    FADC_module = new V1729a_VME(&Controller, sAddress_fadc);
    
    // now create FADC functions object
    FADC_Functions = new HighLevelFunction_VME(FADC_module);

    // set the object created flag to true
    hvFadcObjCreatedFlag = true;

    FADC_module->reset();

    ReadHFOSettings();
    
}

// or create object and immediately initialize object for TOS using .ini file
HV_FADC_Obj::HV_FADC_Obj(std::string iniFilePath){
    // upon initialisation of HV_FADC_Obj, we need to initialize the
    // controller

    // initialize Controller
    Controller.initController(0);
    
    // set the object created flag and settings read to false
    hvFadcObjCreatedFlag      = false;
    hvFadcObjSettingsReadFlag = false;
    hvFadcObjInitFlag         = false;
    // and set the ini file path variable to the given input
    iniFile = QString::fromStdString(iniFilePath);
    
    // read settings to be able to access addresses to create 
    // controller, vmemodule and fadc module
    ReadHFOSettings();


    std::cout << "ADDRESS" << baseAddress_hv << std::endl;
    HV_module   = new CVmeModule(&Controller, baseAddress_hv);

    std::cout << "creating.. " << HV_module->IsConnected() <<std::endl;
    std::cout << "creating2.. " << HV_module->IsConnected() <<std::endl;

    FADC_module = new V1729a_VME(&Controller, sAddress_fadc);
    
    // now create FADC functions object
    FADC_Functions = new HighLevelFunction_VME(FADC_module);

    // set the object created flag to true
    hvFadcObjCreatedFlag = true;

    FADC_module->reset();

    // now object properly created
    
}


HV_FADC_Obj::~HV_FADC_Obj() {

    // Before we delete the FADC and HV modules,
    // we should properly shut both of them down
    // for FADC: reset?
    FADC_module->reset();
    
    // TODO: check, if voltages already ramped down?
    // yes, this

    // call shut down function, if not already shut down
    int toShutdown = 0;
    toShutdown = H_CheckHVModuleIsGood(false);
    if (toShutdown == 0){
	std::string input;
	const char *prompt = "Do you wish to shutdown the HV? (y / N)";
	input = getUserInputNonNumericalDefault(prompt);
	if ((input.compare("y") == 0) ||
	    (input.compare("Y") == 0)){
	    ShutDownHFOForTOS();
	}
	else{
	    std::cout << "Leaving HV turned on." << std::endl;
	}
    }

    // HV: shut down controller
    Controller.closeController();

    delete(FADC_module);
    //delete(Controller);
    delete(HV_module);
}

int HV_FADC_Obj::H_ConnectModule(){
    return HV_module->IsConnected();
}

void HV_FADC_Obj::ReadHFOSettings(){
    // This function reads the .ini file from the 
    // member variable iniFile, which is set in the creator
    // this function is called in the creator    

    // create path for iniFile 
    #ifdef __WIN32__
    // in case of using windows, we need to call the GetCurrentDirectory function
    // as an input it receives: DWORD WINAPI GetCurrentDirectory(_In_  DWORD  nBufferLength, _Out_ LPTSTR lpBuffer);
    // create TCHAR and char string of length MAX_INI_PATH_LENGTH defined in header of this file
    TCHAR string[MAX_INI_PATH_LENGTH];
    char  char_string[MAX_INI_PATH_LENGTH];
    // call windows function with its weird arguments
    GetCurrentDirectory(MAX_INI_PATH_LENGTH, string);
    // QT function needs a char string as input, cannot deal with TCHAR, thus need to convert
    WideCharToMultiByte(CP_ACP, 0, string, wcslen(string)+1, char_string, MAX_INI_PATH_LENGTH, NULL, NULL);
    QDir dir(char_string);
    #else
    QDir dir(get_current_dir_name());
    #endif
    // TODO: check proper path
    //       or rather: add option to give full path or
    //       relative?
    iniFile = dir.absolutePath() + '/' + iniFile;
    std::cout << "Path to Ini File: " << iniFile.toStdString() << std::endl;

    // now we have read the QString with the path
    // to the iniFile
    // create a QSettings object from this file
    // settings object contains every setting from file
    // TODO: check if file exists
    QSettings settings(iniFile, QSettings::IniFormat);
    
    // flag which tracks, if one or more settings was not found in the HFOsettings.ini
    bool containsNotFlag = false;
    

    // check settings object for each expected variable
    // if variable not contained, raise exception?
    if (settings.contains("baseAddress_hv")){
	// to get the baseAddress from the hexadecimal string
	// in the .ini file, we need to do some ugly conversion
	// first to QString, then to std::string, then to int,
	// interpreted as hex int
	QString temp;
	temp = settings.value("baseAddress_hv").toString();
	baseAddress_hv = stoi(temp.toStdString(), nullptr, 16);	
    }
    else{
	containsNotFlag = true;
	// if setting cannot be found, set default base address
	// defined as macro in HV_FADC_Obj.h
	// TODO: think of some way to properly end the program
	// if settings cannot be found
	baseAddress_hv = DEFAULT_BASE_ADDRESS_HV;
    }

    if (settings.contains("sAddress_fadc")){
	// get the sAddress for the fadc
	sAddress_fadc = settings.value("sAddress_fadc").toInt();
    }
    else{
	containsNotFlag = true;
	sAddress_fadc = DEFAULT_S_ADDRESS_FADC;
    }

    if (settings.contains("setKillEnable")){
	setKillEnable = settings.value("setKillEnable").toBool();    
    }
    else{
	containsNotFlag = true;
	setKillEnable = DEFAULT_SET_KILL_ENABLE;
    }

    if (settings.contains("anodeGridGroupFlag")){
	anodeGridGroupFlag = settings.value("anodeGridGroupFlag").toBool();
    }
    else{
	containsNotFlag = true;
	anodeGridGroupFlag = DEFAULT_ANODE_GRID_GROUP_FLAG;
    }

    if (settings.contains("anodeGridGroupMasterChannel")){
	anodeGridGroupMasterChannel = settings.value("anodeGridGroupMasterChannel").toInt();
    }
    else{
	containsNotFlag = true;
	anodeGridGroupMasterChannel = DEFAULT_ANODE_GRID_GROUP_MASTER_CHANNEL;
    }

    if (settings.contains("anodeGridGroupNumber")){
	anodeGridGroupNumber = settings.value("anodeGridGroupNumber").toInt();
    }
    else{
	containsNotFlag = true;
	anodeGridGroupNumber = DEFAULT_ANODE_GRID_GROUP_NUMBER;
    }


    if (settings.contains("monitorTripGroupFlag")){
	monitorTripGroupFlag = settings.value("monitorTripGroupFlag").toBool();
    }
    else{
	containsNotFlag = true;
	monitorTripGroupFlag = DEFAULT_MONITOR_TRIP_GROUP_FLAG;
    }

    if (settings.contains("monitorTripGroupNumber")){
	monitorTripGroupNumber = settings.value("monitorTripGroupNumber").toInt();
    }
    else{
	containsNotFlag = true;
	monitorTripGroupNumber = DEFAULT_MONITOR_TRIP_GROUP_NUMBER;
    }

    if (settings.contains("rampingGroupFlag")){
	rampingGroupFlag = settings.value("rampingGroupFlag").toBool();
    }
    else{
	containsNotFlag = true;
	rampingGroupFlag = DEFAULT_RAMPING_GROUP_FLAG;
    }

    if (settings.contains("rampingGroupNumber")){
	rampingGroupNumber = settings.value("rampingGroupNumber").toInt();
    }
    else{
	containsNotFlag = true;
	rampingGroupNumber = DEFAULT_RAMPING_GROUP_NUMBER;
    }


    // **************************************************
    // *************** GRID VARIABLES *******************
    // **************************************************

    if (settings.contains("gridChannelNumber")){
	gridChannelNumber = settings.value("gridChannelNumber").toInt();
    }
    else{
	containsNotFlag = true;
	gridChannelNumber = DEFAULT_GRID_CHANNEL_NUMBER;
    }

    if (settings.contains("gridVoltageSet")){
	gridVoltageSet = settings.value("gridVoltageSet").toFloat();
    }
    else{
	containsNotFlag = true;
	gridVoltageSet = DEFAULT_GRID_VOLTAGE_SET;
    }

    if (settings.contains("gridVoltageNominal")){
	gridVoltageNominal = settings.value("gridVoltageNominal").toFloat();
    }
    else{
	containsNotFlag = true;
	gridVoltageNominal = DEFAULT_GRID_VOLTAGE_NOMINAL;
    }

    if (settings.contains("gridVoltageBound")){
	gridVoltageBound = settings.value("gridVoltageBound").toFloat();
    }
    else{
	containsNotFlag = true;
	gridVoltageBound = DEFAULT_GRID_VOLTAGE_BOUND;
    }

    if (settings.contains("gridCurrentSet")){
	gridCurrentSet = settings.value("gridCurrentSet").toFloat();
    }
    else{
	containsNotFlag = true;
	gridCurrentSet = DEFAULT_GRID_CURRENT_SET;
    }

    if (settings.contains("gridCurrentNominal")){
	bool ok;
	ok = true;
	gridCurrentNominal = settings.value("gridCurrentNominal").toFloat(&ok);
	if(ok == false){
	    std::cout << "ERROR: Could not convert gridCurrentNominal to float!" << std::endl;
	}
    }
    else{
	containsNotFlag = true;
	gridCurrentNominal = DEFAULT_GRID_CURRENT_NOMINAL;
    }

    if (settings.contains("gridCurrentBound")){
	gridCurrentBound = settings.value("gridCurrentBound").toFloat();
    }
    else{
	containsNotFlag = true;
	gridCurrentBound = DEFAULT_GRID_CURRENT_BOUND;
    }

    // **************************************************
    // *************** ANODE VARIABLES ******************
    // **************************************************

    if (settings.contains("anodeChannelNumber")){
	anodeChannelNumber = settings.value("anodeChannelNumber").toInt();
    }
    else{
	containsNotFlag = true;
	anodeChannelNumber = DEFAULT_ANODE_CHANNEL_NUMBER;
    }

    if (settings.contains("anodeVoltageSet")){
	anodeVoltageSet = settings.value("anodeVoltageSet").toFloat();
    }
    else{
	containsNotFlag = true;
	anodeVoltageSet = DEFAULT_ANODE_VOLTAGE_SET;
    }

    if (settings.contains("anodeVoltageNominal")){
	anodeVoltageNominal = settings.value("anodeVoltageNominal").toFloat();
    }
    else{
	containsNotFlag = true;
	anodeVoltageNominal = DEFAULT_ANODE_VOLTAGE_NOMINAL;
    }

    if (settings.contains("anodeVoltageBound")){
	anodeVoltageBound = settings.value("anodeVoltageBound").toFloat();
    }
    else{
	containsNotFlag = true;
	anodeVoltageBound = DEFAULT_ANODE_VOLTAGE_BOUND;
    }

    if (settings.contains("anodeCurrentSet")){
	anodeCurrentSet = settings.value("anodeCurrentSet").toFloat();
    }
    else{
	containsNotFlag = true;
	anodeCurrentSet = DEFAULT_ANODE_CURRENT_SET;
    }

    if (settings.contains("anodeCurrentNominal")){
	anodeCurrentNominal = settings.value("anodeCurrentNominal").toFloat();
    }
    else{
	containsNotFlag = true;
	anodeCurrentNominal = DEFAULT_ANODE_CURRENT_NOMINAL;
    }

    if (settings.contains("anodeCurrentBound")){
	anodeCurrentBound = settings.value("anodeCurrentBound").toFloat();
    }
    else{
	containsNotFlag = true;
	anodeCurrentBound = DEFAULT_ANODE_CURRENT_BOUND;
    }

    // **************************************************
    // *************** CATHODE VARIABLES ******************
    // **************************************************

    if (settings.contains("cathodeChannelNumber")){
	cathodeChannelNumber = settings.value("cathodeChannelNumber").toInt();
    }
    else{
	containsNotFlag = true;
	cathodeChannelNumber = DEFAULT_CATHODE_CHANNEL_NUMBER;
    }

    if (settings.contains("cathodeVoltageSet")){
	cathodeVoltageSet = settings.value("cathodeVoltageSet").toFloat();
    }
    else{
	containsNotFlag = true;
	cathodeVoltageSet = DEFAULT_CATHODE_VOLTAGE_SET;
    }

    if (settings.contains("cathodeVoltageNominal")){
	cathodeVoltageNominal = settings.value("cathodeVoltageNominal").toFloat();
    }
    else{
	containsNotFlag = true;
	cathodeVoltageNominal = DEFAULT_CATHODE_VOLTAGE_NOMINAL;
    }

    if (settings.contains("cathodeVoltageBound")){
	cathodeVoltageBound = settings.value("cathodeVoltageBound").toFloat();
    }
    else{
	containsNotFlag = true;
	cathodeVoltageBound = DEFAULT_CATHODE_VOLTAGE_BOUND;
    }

    if (settings.contains("cathodeCurrentSet")){
	cathodeCurrentSet = settings.value("cathodeCurrentSet").toFloat();
    }
    else{
	containsNotFlag = true;
	cathodeCurrentSet = DEFAULT_CATHODE_CURRENT_SET;
    }

    if (settings.contains("cathodeCurrentNominal")){
	cathodeCurrentNominal = settings.value("cathodeCurrentNominal").toFloat();
    }
    else{
	containsNotFlag = true;
	cathodeCurrentNominal = DEFAULT_CATHODE_CURRENT_NOMINAL;
    }

    if (settings.contains("cathodeCurrentBound")){
	cathodeCurrentBound = settings.value("cathodeCurrentBound").toFloat();
    }
    else{
	containsNotFlag = true;
	cathodeCurrentBound = DEFAULT_CATHODE_CURRENT_BOUND;
    }


    // **************************************************
    // ***************** MODULE SETTINGS ****************
    // **************************************************

    if (settings.contains("moduleVoltageRampSpeed")){
	moduleVoltageRampSpeed = settings.value("moduleVoltageRampSpeed").toFloat();
    }
    else{
	containsNotFlag = true;
	moduleVoltageRampSpeed = DEFAULT_MODULE_VOLTAGE_RAMP_SPEED;
    }

    if (settings.contains("moduleCurrentRampSpeed")){
	moduleCurrentRampSpeed = settings.value("moduleCurrentRampSpeed").toFloat();
    }
    else{
	containsNotFlag = true;
	moduleCurrentRampSpeed = DEFAULT_MODULE_CURRENT_RAMP_SPEED;
    }

    if (settings.contains("checkModuleTimeInterval")){
	checkModuleTimeInterval = settings.value("checkModuleTimeInterval").toInt();
    }
    else{
	containsNotFlag = true;
	checkModuleTimeInterval = DEFAULT_CHECK_MODULE_TIME_INTERVAL;
    }

    // **************************************************
    // ***************** FADC SETTINGS ******************
    // **************************************************

    if (settings.contains("fadcTriggerType")){
	fadcTriggerType = settings.value("fadcTriggerType").toInt();
    }
    else{
	containsNotFlag = true;
	fadcTriggerType = DEFAULT_FADC_TRIGGER_TYPE;
    }

    if (settings.contains("fadcFrequency")){
	fadcFrequency = settings.value("fadcFrequency").toInt();
    }
    else{
	containsNotFlag = true;
	fadcFrequency = DEFAULT_FADC_FREQUENCY;
    }

    if (settings.contains("fadcPosttrig")){
	fadcPosttrig = settings.value("fadcPosttrig").toInt();
    }
    else{
	containsNotFlag = true;
	fadcPosttrig = DEFAULT_FADC_POSTTRIG;
    }

    if (settings.contains("fadcPretrig")){
	fadcPretrig = settings.value("fadcPretrig").toInt();
    }
    else{
	containsNotFlag = true;
	fadcPretrig = DEFAULT_FADC_PRETRIG;
    }

    if (settings.contains("fadcTriggerThresholdRegisterAll")){
	fadcTriggerThresholdRegisterAll = settings.value("fadcTriggerThresholdRegisterAll").toInt();
    }
    else{
	containsNotFlag = true;
	fadcTriggerThresholdRegisterAll = DEFAULT_FADC_TRIGGER_THRESHOLD_REGISTER_ALL;
    }

    // set flag to true, which says whether settings have been read

    if (containsNotFlag == true){
	std::cout << "One or more settings could not be found in the .ini file."
		  << std::endl;
    }
    hvFadcObjSettingsReadFlag = true;

}



void HV_FADC_Obj::InitHFOForTOS(){
    std::cout << "Entering Init HFO for TOS" << std::endl;

    // First we read the Settings file
    //     done in ReadHFOSettings() called in creator
    // in case hvFadcObjCreatedFlag is false 
    // we use the read settings to create the objects properly
    

    // define variables

    ModuleStatusSTRUCT  moduleStatus = { 0 };
    ModuleControlSTRUCT moduleControl = { 0 };
    ChControlSTRUCT     channelControl = { 0 };
    int gridChannelNumberBinary;
    int anodeChannelNumberBinary;
    int cathodeChannelNumberBinary;

    // use the read settings (ReadHFOSettings() called in creator)
    // to create the objects, if not yet created
    // check if FADC and HV modules have been created
    if (hvFadcObjCreatedFlag == false){

        HV_module   = new CVmeModule(&Controller, baseAddress_hv);
        std::cout << "creating.. " << HV_module->IsConnected() <<std::endl;
        FADC_module = new V1729a_VME(&Controller, sAddress_fadc);
	// create FADC high level functions
        FADC_Functions = new HighLevelFunction_VME(FADC_module);
	// and reset FADC
        FADC_module->reset();

	hvFadcObjCreatedFlag = true;
    }

    // now we have all values to set up the HV for TOS
    // first set kill enable
    // enable Kill Mode
    std::cout << "Set Kill Enable: " << setKillEnable << std::endl;
    H_SetModuleKillEnable(setKillEnable);

    // TODO: properly check for kill enabled
    int timeout = 1000;
    while (timeout > 0){
	bool killEnabled = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	moduleStatus.Word = H_GetModuleStatus();
	killEnabled = moduleStatus.Bit.IsKillEnable;
	if(killEnabled){
	    break;
	}
	else{
	    timeout--;
	}
    }
    if (timeout == 0){
	std::cout << "TIMEOUT: Module could not be set to SetKillEnable" << std::endl;
	// for now throw exception without anything else
	// TODO: set this up properly!!!
	// throw();
    }
    else{
	std::cout << "Module successfully set to SetKillEnable = True" << std::endl;
    }
       
    // now create anodeGrid group. 
    // use channel 0 and 1 for grid and anode respectively
    // first create set group
    // then monitor group
    // GroupMemberSTRUCT1 anodeGridGroupMembers;
    // check which channels are used and assign properly:
    // probably nicer, if we create a
    uint16_t anodeGridGroupMembersInt;
    uint16_t monitorAndRampGroupMembersInt;
    // if channels 0 and 1 are used, the first two bits
    // of the Word will be set to 1, all others 0,
    // thus == 3
    anodeGridGroupMembersInt = 0;
    monitorAndRampGroupMembersInt = 0;
    // we read anodeChannelNumber as an int from the .ini file
    // now we redefine the variable not as an int in decimal, but
    // as an int in binary. Therefore, we shift a '1'
    // 'anodeChannelNumber' places to the left
    // e.g.: anodeChannelNumber = 3
    // gridChannelNumber = 1 << 3 --> 00000001 << 3 --> 000001000
    gridChannelNumberBinary    = 1 << gridChannelNumber;
    // we do the same with the anodeChannelNumber and cathodeChannelNumber
    anodeChannelNumberBinary   = 1 << anodeChannelNumber;
    cathodeChannelNumberBinary = 1 << cathodeChannelNumber;
    // This way we can simply add both channel numbers to obtain the correct 
    // bit word (as uint16_t) for the MemberList1.Word and do not have to
    // take care of the Channel0 to Channel15 names of the Bits for the Word.
    anodeGridGroupMembersInt = anodeChannelNumberBinary + gridChannelNumberBinary;
    // and set this word as memberList for anodeGridSetOnGroup
    anodeGridSetOnGroup.MemberList1.Word = anodeGridGroupMembersInt;

    monitorAndRampGroupMembersInt = gridChannelNumberBinary + anodeChannelNumberBinary + cathodeChannelNumberBinary;
    monitorTripGroup.MemberList1.Word = monitorAndRampGroupMembersInt;
    rampingGroup.MemberList1.Word     = monitorAndRampGroupMembersInt;

    // define master channel of SetGroup
    // define anode as master Channel; include both in .ini file
    anodeGridSetOnGroup.Type1.Set.Bit.GroupType     = GROUP_TYPE_SET;
    anodeGridSetOnGroup.Type1.Set.Bit.MasterChannel = anodeGridGroupMasterChannel;
    anodeGridSetOnGroup.Type1.Set.Bit.Mode          = DEFAULT_ANODE_GRID_GROUP_MODE;
    anodeGridSetOnGroup.Type1.Set.Bit.Control       = GROUP_SET_SETON;

    monitorTripGroup.Type1.Monitor.Bit.GroupType = GROUP_TYPE_MONITOR;
    monitorTripGroup.Type1.Monitor.Bit.Monitor 	 = GROUP_MONITOR_ISTRIP;
    // set Mode to 1. That way, the action of the group is called,
    // if one of the group channels reports a 1 for the bit, that is being
    // checked. In our case:
    // Channel.Bit.CurrentTrip == 1 for one channel means group shut down
    monitorTripGroup.Type1.Monitor.Bit.Mode      = 1; 
    monitorTripGroup.Type1.Monitor.Bit.Action  	 = GROUP_ACTION_SHUT_MODULE;

    rampingGroup.Type1.Status.Bit.GroupType = GROUP_TYPE_STATUS;
    rampingGroup.Type1.Status.Bit.Item      = GROUP_STATUS_ISRAMP;

    // now need to set group on HV
    // simply done by calling 
    H_SetFlexGroup(anodeGridGroupNumber,   anodeGridSetOnGroup);
    H_SetFlexGroup(monitorTripGroupNumber, monitorTripGroup);
    H_SetFlexGroup(rampingGroupNumber,     rampingGroup);


    // TODO: need to set ModuleEventGroupMask for the groups,
    //       in order to register group events for the wanted 
    //       things
    
    // ModuleEventGroupMask: set everything to 0 except 
    //                       gridGroupNumbers of all 3 defined groups
    // ChannelEventMask: set all three of our channel masks to the following pattern:
    //                   MaskEventTrip = 1
    //                   MaskEventEndOfRamp = 1
    //                   MaskEventVoltageLimit = 1
    //                   MaskEventCurrentLimit = 1
    //                   MaskEventEmergency = 1
    //                   rest to 0.
    ChEventMaskSTRUCT ChEventMask         = { 0 };
    ChEventMask.Bit.MaskEventCurrentTrip  = 1;
    ChEventMask.Bit.MaskEventEndOfRamping = 1;
    ChEventMask.Bit.MaskEventVoltageLimit = 1;
    ChEventMask.Bit.MaskEventCurrentLimit = 1;
    ChEventMask.Bit.MaskEventEmergency    = 1;



    timeout = 1000;
    while(timeout > 0){
        // write this ChannelEventMask to all three of our channels
        H_SetChannelEventMask(gridChannelNumber,    ChEventMask.Word);
        H_SetChannelEventMask(anodeChannelNumber,   ChEventMask.Word);
        H_SetChannelEventMask(cathodeChannelNumber, ChEventMask.Word);

        uint32_t gem;
        uint32_t aem;
        uint32_t cem;

        gem = H_GetChannelEventMask(gridChannelNumber);
        aem = H_GetChannelEventMask(anodeChannelNumber);
        cem = H_GetChannelEventMask(cathodeChannelNumber);

	if ((gem == ChEventMask.Word) &&
	    (aem == ChEventMask.Word) &&
	    (cem == ChEventMask.Word)){
	    break;
	}
	
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	timeout--;
    }
    if (timeout == 0){
	std::cout << "Could not set Channel event masks." << std::endl;
    }
    

    // moduleEventGroupMask is a 32bit integer, where each bit
    // corresponds to the activation of one group. This means, since
    // we wish to set the GroupMask to our three Groups, we need to set
    // each bit for the number of each group. Incidentally, the group numbers
    // are the same as the channel numbers, thus we can use the bitwise
    // created integer for the GroupMembers containing all three channels
    uint32_t moduleEventGroupMask = monitorAndRampGroupMembersInt;
    H_SetModuleEventGroupMask(moduleEventGroupMask);
    // now we can check the channel events and group events

    // groups set up. now set voltages, currents
    //****************************************************************
    // set voltage set
    H_SetChannelVoltageSet(gridChannelNumber, gridVoltageSet);
    H_SetChannelVoltageSet(anodeChannelNumber, anodeVoltageSet);
    H_SetChannelVoltageSet(cathodeChannelNumber, cathodeVoltageSet);
    // set current set
    H_SetChannelCurrentSet(gridChannelNumber, gridCurrentSet);
    H_SetChannelCurrentSet(anodeChannelNumber, anodeCurrentSet);
    H_SetChannelCurrentSet(cathodeChannelNumber, cathodeCurrentSet);

    // TODO: think about bounds
    // set voltage bound
    // set current bound

    // before we can set the channels to ON, reset the Event Status
    int ok;
    ok = H_ClearAllChannelEventStatus();
    if (ok != 0){
	std::cout << "Could not reset Event status.\n" 
		  << "Probably will not start ramping."
		  << std::endl;
    }


    timeout = 1000;
    while(timeout > 0){
        // all that is left to do, is start the ramp up
        // that means set channels to SetOn = 1
        // for the anodeGridGroup, we only set the master channel
        // to SetOn = 1
	ChStatusSTRUCT gridStatus = { 0 };
	ChStatusSTRUCT anodeStatus = { 0 };
	ChStatusSTRUCT cathodeStatus = { 0 };
        uint32_t gem;
        uint32_t aem;
        uint32_t cem;

	gridStatus.Word    = H_GetChannelStatus(gridChannelNumber);
	anodeStatus.Word   = H_GetChannelStatus(anodeChannelNumber);
	cathodeStatus.Word = H_GetChannelStatus(cathodeChannelNumber);

        gem = H_GetChannelEventMask(gridChannelNumber);
        aem = H_GetChannelEventMask(anodeChannelNumber);
        cem = H_GetChannelEventMask(cathodeChannelNumber);

        std::cout << "gem\t" << gem << std::endl;
        std::cout << "aem\t" << aem << std::endl;
        std::cout << "cem\t" << cem << std::endl;
	std::cout << "ChEventMask2INNNN\t" << ChEventMask.Word << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(500));


    

	if(gridStatus.Bit.isOn == 0 ||
	   anodeStatus.Bit.isOn == 0 ||
	   cathodeStatus.Bit.isOn == 0){
            // can be exchanged by H_SetChannelOn(int channel)
            // channelControl.Word = H_GetChannelControl(anodeGridGroupMasterChannel);
            // channelControl.Bit.setON = 1;
            // H_SetChannelControl(anodeGridGroupMasterChannel, channelControl.Word);
	    std::cout << "trying to set channels to on!" << std::endl;
            H_SetChannelOn(anodeGridGroupMasterChannel);

            // channelControl.Word = H_GetChannelControl(cathodeChannelNumber);
            // channelControl.Bit.setON = 1;
            // H_SetChannelControl(cathodeChannelNumber, channelControl.Word);
	    H_SetChannelOn(cathodeChannelNumber);
	}
	else if (gridStatus.Bit.isOn == 1 &&
		 anodeStatus.Bit.isOn == 1 &&
		 cathodeStatus.Bit.isOn == 1){
	    std::cout << "All channels set to ON" << std::endl;
	    break;
	}
	else{
	    std::cout << "Status " 
		      << gridStatus.Bit.isOn << "\t" 
		      << anodeStatus.Bit.isOn << "\t"
		      << cathodeStatus.Bit.isOn << std::endl;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
	timeout--;
    }
    // channels should now start to ramp




    timeout = 1000;
    while(timeout > 0){
        uint32_t gem;
        uint32_t aem;
        uint32_t cem;

        gem = H_GetChannelEventMask(gridChannelNumber);
        aem = H_GetChannelEventMask(anodeChannelNumber);
        cem = H_GetChannelEventMask(cathodeChannelNumber);

        std::cout << "gem\t" << gem << std::endl;
        std::cout << "aem\t" << aem << std::endl;
        std::cout << "cem\t" << cem << std::endl;
	std::cout << "ChEventMask2\t" << ChEventMask.Word << std::endl;

	if ((gem == ChEventMask.Word) &&
	    (aem == ChEventMask.Word) &&
	    (cem == ChEventMask.Word)){
	    break;
	}
	
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	timeout--;
    }
    



    // before we start ramping up the HV modules, first set FADC settings
    std::cout << "Setting FADC settings" << std::endl;

    F_SetTriggerType(fadcTriggerType);
    F_SetFrequency(fadcFrequency);
    F_SetPosttrig(fadcPosttrig);
    F_SetPretrig(fadcPretrig);

    // TODO: find out what this was supposed to be
    //F_SetTriggerThresholdRegisterAll(fadcTriggerThresholdRegisterAll);
    
    //F_PrintSettings();

    // now check while is ramping
    bool rampUpFlag = true;
    H_CheckModuleIsRamping(rampUpFlag);

    hvFadcObjInitFlag = true;

    // moduleStatus.Word = H_GetModuleStatus();
    // killEnabled = moduleStatus.Bit.IsKillEnable;

    // std::cout << "Is Kill enabled? " << killEnabled << std::endl;

    // moduleControl.Word = H_GetModuleControl();
    // moduleControl.Bit.SetStop = 1;
    // H_SetModuleControl(moduleControl.Word);

    // std::this_thread::sleep_for(std::chrono::seconds(1));

}

void HV_FADC_Obj::H_CheckModuleIsRamping(bool rampUpFlag){
    // convenience function which checks, whether
    // the channels we use are currently ramping
    // prints output of channel voltages every second
    //******************* CHECK RAMPING **************************

    //************************************************************
    int timeout;


    ChEventStatusSTRUCT gridEventStatus = { 0 };
    ChEventStatusSTRUCT anodeEventStatus = { 0 };
    ChEventStatusSTRUCT cathodeEventStatus = { 0 };
    gridEventStatus.Word    = H_GetChannelEventStatus(gridChannelNumber);
    anodeEventStatus.Word   = H_GetChannelEventStatus(anodeChannelNumber);
    cathodeEventStatus.Word = H_GetChannelEventStatus(cathodeChannelNumber);

    // reset the event status
    int ok;
    ok = H_ClearAllChannelEventStatus();
    if (ok != 0){
	std::cout << "Could not reset Event Status" << std::endl;
    }

    // define rampUpFlag to differentiate between ramp up 
    // and ramp down
    // based on whether channel status is isOn == 1
    // bool rampUpFlag;
    // get channel status of all channels
    ChStatusSTRUCT gridStatus = { 0 };
    ChStatusSTRUCT anodeStatus = { 0 };
    ChStatusSTRUCT cathodeStatus = { 0 };

    timeout = 1000;
    while(timeout > 0){
	// get the ramping Group back from the module
	// rampingGroup = H_GetFlexGroup(rampingGroupNumber);
	// TODO: currently not using rampinGroup, because unclear
	//       how status group works

	// TODO: change printed output -> convert to 1 line, which is called with '\r'
	

	// ChEventStatusSTRUCT gridEventStatus = { 0 };
	// ChEventStatusSTRUCT anodeEventStatus = { 0 };
	// ChEventStatusSTRUCT cathodeEventStatus = { 0 };
	
	float gridVoltageMeasured;
	float anodeVoltageMeasured;
	float cathodeVoltageMeasured;

	std::this_thread::sleep_for(std::chrono::seconds(1)); 
	gridStatus.Word    = H_GetChannelStatus(gridChannelNumber);
	anodeStatus.Word   = H_GetChannelStatus(anodeChannelNumber);
	cathodeStatus.Word = H_GetChannelStatus(cathodeChannelNumber);

	gridEventStatus.Word 	= H_GetChannelEventStatus(gridChannelNumber);
	anodeEventStatus.Word   = H_GetChannelEventStatus(anodeChannelNumber);
	cathodeEventStatus.Word = H_GetChannelEventStatus(cathodeChannelNumber);
	

	// ModuleControlSTRUCT moduleControl = { 0 };

	// moduleControl.Word = H_GetModuleControl();
	// moduleControl.Bit.DoClear = 1;
	// H_SetModuleControl(moduleControl.Word);

	if (rampUpFlag == 1){
	    std::cout << "Enter CheckIsRamping rampUpFlag is True\n";
	    //**************************************************
	    //****************** RAMP UP ***********************
	    //**************************************************
	    // ramp up and down distinguished, for easier handling
	    // check status for IsRamping bit and check ramp up


	    // printing in same line doesn't work like this
	    std::cout << '\r'
		      << "Printing flags\n"
		      << "\n"
		      << "gridStatus.Bit.isRamping == 1   " << gridStatus.Bit.isRamping << "\n"
		      << "anodeStatus.Bit.isRamping == 1   " << anodeStatus.Bit.isRamping << "\n"
		      << "cathodeStatus.Bit.isRamping == 1   " << cathodeStatus.Bit.isRamping << "\n"
		      << "gridStatus.Bit.isOn == 1   " << gridStatus.Bit.isOn << "\n"
		      << "anodeStatus.Bit.isOn == 1   " << anodeStatus.Bit.isOn << "\n"
		      << "cathodeStatus.Bit.isOn == 1   " << cathodeStatus.Bit.isOn << "\n"
		      << "\n";


	    if ((gridStatus.Bit.isRamping == 1 || 
		 anodeStatus.Bit.isRamping == 1 ||
	         cathodeStatus.Bit.isRamping == 1) && 
		gridStatus.Bit.isOn == 1 &&
		anodeStatus.Bit.isOn == 1 &&
		cathodeStatus.Bit.isOn == 1){
	        // one of the channels is still ramping
	        // therefore print current voltage / voltage Set
	        // of all 3 channels
	        gridVoltageMeasured = H_GetChannelVoltageMeasure(gridChannelNumber);
	        anodeVoltageMeasured = H_GetChannelVoltageMeasure(anodeChannelNumber);
	        cathodeVoltageMeasured = H_GetChannelVoltageMeasure(cathodeChannelNumber);

	        std::cout << '\r'
			  << "Still ramping... Current Voltage of all channels is:"     << "\n"
			  << "Grid Voltage / VoltageSet: "    << gridVoltageMeasured    << " / " << gridVoltageSet << "\n"
			  << "Anode Voltage / VoltageSet: "   << anodeVoltageMeasured   << " / " << anodeVoltageSet << "\n"
			  << "Cathode Voltage / VoltageSet: " << cathodeVoltageMeasured << " / " << cathodeVoltageSet << std::flush;
	    }
	    // else if all channels not ramping anymore, event end of ramping true and still ON
	    else if( gridStatus.Bit.isRamping == 0 && 
		     anodeStatus.Bit.isRamping == 0 &&
		     cathodeStatus.Bit.isRamping == 0 &&
		     gridEventStatus.Bit.EventEndOfRamping == 1 &&
		     anodeEventStatus.Bit.EventEndOfRamping == 1 &&
		     cathodeEventStatus.Bit.EventEndOfRamping == 1 &&
		     gridStatus.Bit.isOn == 1 &&
		     anodeStatus.Bit.isOn == 1 &&
		     cathodeStatus.Bit.isOn == 1){
		// this defines the channels as successfully ramped

	        gridVoltageMeasured = H_GetChannelVoltageMeasure(gridChannelNumber);
	        anodeVoltageMeasured = H_GetChannelVoltageMeasure(anodeChannelNumber);
	        cathodeVoltageMeasured = H_GetChannelVoltageMeasure(cathodeChannelNumber);

	        std::cout << "All channels successfully ramped up" << std::endl;
	        std::cout << "Grid Voltage / VoltageSet: " << gridVoltageMeasured << " / " << gridVoltageSet << std::endl;
	        std::cout << "Anode Voltage / VoltageSet: " << anodeVoltageMeasured << " / " << anodeVoltageSet << std::endl;
	        std::cout << "Cathode Voltage / VoltageSet: " << cathodeVoltageMeasured << " / " << cathodeVoltageSet << std::endl;

		//TODO: reset event status??


	        break;
	    }
	}//end ramp up if
	else if (rampUpFlag == 0){
	    std::cout << "Enter CheckIsRamping rampUpFlag is False" << std::endl;
	    //**************************************************
	    //****************** RAMP DOWN *********************
	    //**************************************************
	    // ramp up and down distinguished, for easier handling
	    // check status for IsRamping bit and check ramp up
	    if ((gridStatus.Bit.isRamping == 1 || 
		 anodeStatus.Bit.isRamping == 1 ||
		 cathodeStatus.Bit.isRamping == 1) && 
		(gridStatus.Bit.isOn == 0 &&
		 anodeStatus.Bit.isOn == 0 &&
		 cathodeStatus.Bit.isOn == 0)){
		// one of the channels is still ramping down
		// therefore print current voltage / voltage Set
		// of all 3 channels
		gridVoltageMeasured = H_GetChannelVoltageMeasure(gridChannelNumber);
		anodeVoltageMeasured = H_GetChannelVoltageMeasure(anodeChannelNumber);
		cathodeVoltageMeasured = H_GetChannelVoltageMeasure(cathodeChannelNumber);
		
		std::cout << "Still ramping down... Current Voltage of all channels is:" << std::endl;
		std::cout << "Grid Voltage / VoltageSet: " << gridVoltageMeasured << " / " << gridVoltageSet << std::endl;
		std::cout << "Anode Voltage / VoltageSet: " << anodeVoltageMeasured << " / " << anodeVoltageSet << std::endl;
		std::cout << "Cathode Voltage / VoltageSet: " << cathodeVoltageMeasured << " / " << cathodeVoltageSet << std::endl;
	    }
	    else if (gridEventStatus.Bit.EventEndOfRamping    == 1 &&
		     anodeEventStatus.Bit.EventEndOfRamping   == 1 &&
		     cathodeEventStatus.Bit.EventEndOfRamping == 1 &&
		     gridStatus.Bit.isOn    == 0 &&
		     anodeStatus.Bit.isOn   == 0 &&
		     cathodeStatus.Bit.isOn == 0){
		std::cout << "All channels successfully ramped down" << std::endl;
		break;
	    }
	}//end ramp down if
	// sleep for 1 second and check again if still ramping
	std::this_thread::sleep_for(std::chrono::seconds(1)); 
	timeout--;
    }

    if (timeout > 0){
	// in this case while loop above was stopped by break (and not timeout == 0)
	// that means ramping is done
	std::cout << "Ramping successfully finished" << std::endl;
    }

}



int HV_FADC_Obj::H_CheckHVModuleIsGood(bool verbose){
    // this function is called every checkModuleTimeInterval (from
    // HFOSettings.ini) seconds and checks, whether all voltages
    // are good / if any events happened
    // if module tripped / voltages bad, stop Run immediately (return -1)
    // and shut down module (if Trip, is done automatically through 
    // monitoring group)

    // this function can only be called, if hvFadcObjCreatedFlag == true
    // hvFadcObj
    if (hvFadcObjCreatedFlag == true){
        
        // variables to store the current EventStatus values
        // these will be compared to the values of the last call
        // of this function
        ChEventStatusSTRUCT gridEventStatus    = { 0 };
        ChEventStatusSTRUCT anodeEventStatus   = { 0 };
        ChEventStatusSTRUCT cathodeEventStatus = { 0 };

        // first need to check whether module tripped. 
        // check EventTrip
        // or rather check monitoring group
        uint32_t moduleEventGroupStatus = H_GetModuleEventGroupStatus();
        // if != 0, some event was triggered
        if (moduleEventGroupStatus != 0){
	    if (verbose == true){
		std::cout << "One of the groups triggered an Event. Abort Run" << std::endl;
		std::cout << "Probably the module tripped." << std::endl;
	    }
	    return -1;
        }    

        // this function basically only does the following:
        // reads the current voltages and currents
        // compares them to the values we set in the beginning
        // additionally check whether module is in IsControlledVoltage,
        // which should be set, if our module is at the set level
        // and isOn is set

        float gridVoltageMeasured;  
        float anodeVoltageMeasured; 
        float cathodeVoltageMeasured;
        
        gridVoltageMeasured    = H_GetChannelVoltageMeasure(gridChannelNumber);
        anodeVoltageMeasured   = H_GetChannelVoltageMeasure(anodeChannelNumber);
        cathodeVoltageMeasured = H_GetChannelVoltageMeasure(cathodeChannelNumber);

        if ((gridVoltageMeasured    >= 0.99*gridVoltageSet)  &&
	    (anodeVoltageMeasured   >= 0.99*anodeVoltageSet) &&
	    (cathodeVoltageMeasured >= 0.99*cathodeVoltageSet)){
	    if (verbose == true){
		std::cout << "All voltages within 1 percent of the set voltage." << std::endl;
		std::cout << "Grid / V\t Anode / V\t Cathode / V\n"
			  << gridVoltageMeasured << "\t\t " 
			  << anodeVoltageMeasured << "\t\t "
			  << cathodeVoltageMeasured << std::endl;
	    }
        }
        else{
	    if (verbose == true){
		std::cout << "Voltage outside of 1 percent of set voltage.\n" 
			  << std::endl;
	    }
	    return -1;
        }

        ChStatusSTRUCT gridStatus    = { 0 };
        ChStatusSTRUCT anodeStatus   = { 0 };
        ChStatusSTRUCT cathodeStatus = { 0 };
        gridStatus.Word    = H_GetChannelStatus(gridChannelNumber);
        anodeStatus.Word   = H_GetChannelStatus(anodeChannelNumber);
        cathodeStatus.Word = H_GetChannelStatus(cathodeChannelNumber);

        if ((gridStatus.Bit.ControlledByVoltage    == 1) &&
	    (gridStatus.Bit.isOn    == 1) &&
	    (anodeStatus.Bit.ControlledByVoltage   == 1) &&
	    (anodeStatus.Bit.isOn   == 1) &&
	    (cathodeStatus.Bit.ControlledByVoltage == 1) &&
	    (cathodeStatus.Bit.isOn == 1)){
	    if (verbose == true){
		std::cout << "All channels controlled by voltage and set to isOn.\n" 
			  << "All good, continue run." << std::endl;
	    }
	    return 0;

        }
        else{
	    if (verbose == true){
		std::cout << "One or more channels not controlled by voltage or turned off.\n" 
			  << "Probably module already tripped.\n" 
			  << "See HV_error_log.txt for more information\n"
			  << "Stopping run immediately." << std::endl;
	    }
	    return -1;
        }
    
    }//end if (hvFadcObjCreatedFlag == true)
    else{
	// this should prevent seg faults
	return -1;
    }
}



// the following function is only implemented here, because
// std::put_time is is only supported from gcc 5.2 onwards
// once gcc 5.2 is part of ubuntu repository, remove this function
// and use the commented code in H_DumpErrorLogToFile instead
// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

void HV_FADC_Obj::H_DumpErrorLogToFile(int event){
    // int event: if function is called from a run, we
    //            include the event after which error log is 
    //            created

    // this function dumps an error log to (relative path)
    // ../log/HV_error.log
    // content is appended to file.
    // contains: - GroupEventStatus
    //           - ChannelEventStatus
    //           - date & time

    // create ofstream object and open file to append to it
    std::ofstream logfile;
    logfile.open("../log/HV_error.log", std::ios::app);

    // prepare appending to file
    logfile << "\n" << "\n" 
	    << "######################### NEW ENTRY #########################" 
	    << "\n" << std::endl;

    // the commented code below would be nicer, but is only supported from
    // gcc 5.2 onwards
    // output date and time as YYYY-MM-DD hh:mm:ss using put_time and localtime
    // // get system time as time_point
    // std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    // // convert time_point to time_t
    // std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    // logfile << "Error log append happened at: " << std::put_time(std::localtime(&now_c), "%F %T") <<  "\n";
    logfile << "Error log append happened at: " << currentDateTime() <<  "\n";
    logfile << "During event #: " << event << "\n";

    
    // now get event status of groups and channels
    uint32_t moduleEventGroupStatus = H_GetModuleEventGroupStatus();

    ChEventStatusSTRUCT gridEventStatus = { 0 };
    ChEventStatusSTRUCT anodeEventStatus = { 0 };
    ChEventStatusSTRUCT cathodeEventStatus = { 0 };
    gridEventStatus.Word    = H_GetChannelEventStatus(gridChannelNumber);
    anodeEventStatus.Word   = H_GetChannelEventStatus(anodeChannelNumber);
    cathodeEventStatus.Word = H_GetChannelEventStatus(cathodeChannelNumber);

    logfile << "##### Event Group Status #####\n"
	    << "type: uint32_t \n"
	    << "Each bit corresponds to one group. If a bit is set, the "
	    << "corresponding group had an event\n"
	    << "moduleEventGroupStatus: " << moduleEventGroupStatus << "\n\n";
    
    logfile << "##### Channel Event Status #####\n"
	    << "Grid Event Status: \n"
	    << "\t EventInputError:          " << gridEventStatus.Bit.EventInputError << "\n"
	    << "\t EventOnToOff:             " << gridEventStatus.Bit.EventOnToOff << "\n"
	    << "\t EventEndOfRamping:        " << gridEventStatus.Bit.EventEndOfRamping << "\n"
	    << "\t EventEmergency:           " << gridEventStatus.Bit.EventEmergency << "\n"
	    << "\t EventControlledByVoltage: " << gridEventStatus.Bit.EventControlledByVoltage << "\n"
	    << "\t EventControlledByCurrent: " << gridEventStatus.Bit.EventControlledByCurrent << "\n"
	    << "\t EventCurrentBounds:       " << gridEventStatus.Bit.EventCurrentBounds << "\n"
	    << "\t EventExternalInhibit:     " << gridEventStatus.Bit.EventExternalInhibit << "\n"
	    << "\t EventCurrentTrip:         " << gridEventStatus.Bit.EventCurrentTrip << "\n"
	    << "\t EventCurrentLimit:        " << gridEventStatus.Bit.EventCurrentLimit << "\n"
	    << "\t EventVoltageLimit:        " << gridEventStatus.Bit.EventVoltageLimit << "\n"
	    << "\n";
   
    logfile << "Anode Event Status: \n"
	    << "\t EventInputError:          " << anodeEventStatus.Bit.EventInputError << "\n"
	    << "\t EventOnToOff:             " << anodeEventStatus.Bit.EventOnToOff << "\n"
	    << "\t EventEndOfRamping:        " << anodeEventStatus.Bit.EventEndOfRamping << "\n"
	    << "\t EventEmergency:           " << anodeEventStatus.Bit.EventEmergency << "\n"
	    << "\t EventControlledByVoltage: " << anodeEventStatus.Bit.EventControlledByVoltage << "\n"
	    << "\t EventControlledByCurrent: " << anodeEventStatus.Bit.EventControlledByCurrent << "\n"
	    << "\t EventCurrentBounds:       " << anodeEventStatus.Bit.EventCurrentBounds << "\n"
	    << "\t EventExternalInhibit:     " << anodeEventStatus.Bit.EventExternalInhibit << "\n"
	    << "\t EventCurrentTrip:         " << anodeEventStatus.Bit.EventCurrentTrip << "\n"
	    << "\t EventCurrentLimit:        " << anodeEventStatus.Bit.EventCurrentLimit << "\n"
	    << "\t EventVoltageLimit:        " << anodeEventStatus.Bit.EventVoltageLimit << "\n"
	    << "\n";

    logfile << "Cathode Event Status: \n"
	    << "\t EventInputError:          " << cathodeEventStatus.Bit.EventInputError << "\n"
	    << "\t EventOnToOff:             " << cathodeEventStatus.Bit.EventOnToOff << "\n"
	    << "\t EventEndOfRamping:        " << cathodeEventStatus.Bit.EventEndOfRamping << "\n"
	    << "\t EventEmergency:           " << cathodeEventStatus.Bit.EventEmergency << "\n"
	    << "\t EventControlledByVoltage: " << cathodeEventStatus.Bit.EventControlledByVoltage << "\n"
	    << "\t EventControlledByCurrent: " << cathodeEventStatus.Bit.EventControlledByCurrent << "\n"
	    << "\t EventCurrentBounds:       " << cathodeEventStatus.Bit.EventCurrentBounds << "\n"
	    << "\t EventExternalInhibit:     " << cathodeEventStatus.Bit.EventExternalInhibit << "\n"
	    << "\t EventCurrentTrip:         " << cathodeEventStatus.Bit.EventCurrentTrip << "\n"
	    << "\t EventCurrentLimit:        " << cathodeEventStatus.Bit.EventCurrentLimit << "\n"
	    << "\t EventVoltageLimit:        " << cathodeEventStatus.Bit.EventVoltageLimit << "\n"
	    << "\n";

    logfile << "##### Module related #####\n"
	    << "TO BE IMPLEMENTED \n\n";

}


void HV_FADC_Obj::ShutDownHFOForTOS(){
    // this function is called upon deleting the HV_FADC_Obj or
    // and thus when shutting down TOS


    // only need to perform shutdown if hvFadcObjInitFlag is true
    // TODO: init flag not the correct way to decide whether shutdown or not
    // instead of using hvFadcObjInitFlag, we can use CheckHVModuleIsGood as 
    // a way to decide
    int shutdown = 0;
    shutdown = H_CheckHVModuleIsGood(false);

    if(shutdown == 0){

        std::cout << std::endl << "Starting shutdown of HV for TOS" << std::endl << std::endl;
        
        // TODO: ramp down
        //       check for any Events
        //       perform a doClear
        //       setStatus to 0x0000 (everything off)
        //       set KillEnable == False
        //       
        // ModuleStatusSTRUCT  moduleStatus = { 0 };
        ModuleControlSTRUCT moduleControl = { 0 };
        ChControlSTRUCT     channelControl = { 0 };    

        // ramp down voltages (use anode Grid Group master channel to ramp down group)
        // and cathode channel
        channelControl.Word = H_GetChannelControl(anodeGridGroupMasterChannel);
        channelControl.Bit.setON = 0;
        H_SetChannelControl(anodeGridGroupNumber, channelControl.Word);

        channelControl.Word = H_GetChannelControl(cathodeChannelNumber);
        channelControl.Bit.setON = 0;
        H_SetChannelControl(cathodeChannelNumber, channelControl.Word);

        // check if the module is ramping down
        bool rampUpFlag = false;
        H_CheckModuleIsRamping(rampUpFlag);


        // TODO: before we perform DoClear, we should check events!
        
        // perform a DoClear (that is reset every event)
        // and set Kill Enable to 0
        moduleControl.Word = H_GetModuleControl();
        moduleControl.Bit.DoClear = 1;
        moduleControl.Bit.SetKillEnable = 0;
        H_SetModuleControl(moduleControl.Word);
        
        // this should properly shut down the device
        // set init flag to false again
        hvFadcObjInitFlag = false;
    }
    else{
	std::cout << "HFO was already shut down / not initialized yet." << std::endl;
    }

}


int HV_FADC_Obj::H_ClearAllChannelEventStatus(){
    // this function tries to reset the channel event status
    // of all our three used channels
    // access to the channels works over 

    int timeout;
    ChEventStatusSTRUCT gridEventStatus    = { 0 };
    ChEventStatusSTRUCT anodeEventStatus   = { 0 };
    ChEventStatusSTRUCT cathodeEventStatus = { 0 };

    gridEventStatus.Word    = H_GetChannelEventStatus(gridChannelNumber);
    anodeEventStatus.Word   = H_GetChannelEventStatus(anodeChannelNumber);
    cathodeEventStatus.Word = H_GetChannelEventStatus(cathodeChannelNumber);

    if ((gridEventStatus.Word    == 0) &&
	(anodeEventStatus.Word   == 0) &&
	(cathodeEventStatus.Word == 0)){
	// event status already 0. return successfully
	return 0;
    }

    // TODO: this while loop is not what I need exactly
    timeout = 1000;
    while(timeout>0){
        // uint16_t resetChannelEventStatus = 1;
        // H_SetChannelEventStatus(gridChannelNumber,    resetChannelEventStatus);
        // H_SetChannelEventStatus(anodeChannelNumber,   resetChannelEventStatus);
        // H_SetChannelEventStatus(cathodeChannelNumber, resetChannelEventStatus);
	H_ClearChannelEventStatus(gridChannelNumber);
	H_ClearChannelEventStatus(anodeChannelNumber);
	H_ClearChannelEventStatus(cathodeChannelNumber);
        // sleep for a while...
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_HV_SLEEP_TIME)); 
	gridEventStatus.Word    = H_GetChannelEventStatus(gridChannelNumber);
	anodeEventStatus.Word   = H_GetChannelEventStatus(anodeChannelNumber);
	cathodeEventStatus.Word = H_GetChannelEventStatus(cathodeChannelNumber);
	if (gridEventStatus.Bit.EventEndOfRamping    == 0 &&
	    anodeEventStatus.Bit.EventEndOfRamping   == 0 &&
	    cathodeEventStatus.Bit.EventEndOfRamping == 0){
	    break;
	}
	timeout--;
    }
    if (timeout == 0){
	std::cout << "ERROR: Timeout while trying to reset EventStatus of channels." << std::endl;
	// return error
	return -1;
    }

    // successfully set to zero
    return 0;
}


void HV_FADC_Obj::H_SetNominalValues(){

    //****************** Set Nominal values **************************

    int timeout;
    ModuleControlSTRUCT moduleControl = { 0 };
    ModuleStatusSTRUCT  moduleStatus  = { 0 };

    // only possible to call this function, if settings were read
    // and hvFadc obj created
    if ((hvFadcObjCreatedFlag      == true) &&
	(hvFadcObjSettingsReadFlag == true)){
        // first set module to stop, so we can set nominal voltages
        // get channel grid

        moduleControl.Word = H_GetModuleControl();
        moduleControl.Bit.SetStop = 1;
        H_SetModuleControl(moduleControl.Word);

        // now we can set the nominal values
        std::cout.precision(15);
        timeout = 1000;
        while(timeout > 0){
        	moduleStatus.Word = H_GetModuleStatus();

        	if(moduleStatus.Bit.IsStop == 1){
        	    // voltages
                H_SetChannelVoltageNominal(gridChannelNumber, gridVoltageNominal);
                H_SetChannelVoltageNominal(anodeChannelNumber, anodeVoltageNominal);
                H_SetChannelVoltageNominal(cathodeChannelNumber, cathodeVoltageNominal);
                // currents
                H_SetChannelCurrentNominal(gridChannelNumber, gridCurrentNominal);
                H_SetChannelCurrentNominal(anodeChannelNumber, anodeCurrentNominal);
                H_SetChannelCurrentNominal(cathodeChannelNumber, cathodeCurrentNominal);

        	    // check if current of grid channel successfully changed from 0
        	    // if so, break from while loop
        	    float temp = H_GetChannelCurrentNominal(gridChannelNumber);
        	    if (temp != 0){
        		break;
        	    }
        	}

        	std::this_thread::sleep_for(std::chrono::milliseconds(5));
        	timeout--;
        }
        if (timeout == 0){
        	std::cout << "ERROR: Could not set nominal Voltages / Currents" << std::endl;
        	std::cout << "       Module did not enter IsStop == 1         " << std::endl;
        }

        // now revert SetStop back to 0
        moduleControl.Word = H_GetModuleControl();
        moduleControl.Bit.SetStop = 0;
        H_SetModuleControl(moduleControl.Word);    
    }
    else{
	std::cout << "Before calling SetNominalValues() the Settings need to be correctly "
		  << "read from the .ini file and the HV_FADC object has to be created.\n"
		  << "Nominal values NOT set." << std::endl;
    }

}



GroupSTRUCT HV_FADC_Obj::H_GetFlexGroup(int group){
    // call both get flex group functions from the module
    // and return the group built from that
    // note: initialization with designated initializer illegal in c++
    // thus, initialization includes comments for readability
    GroupSTRUCT groupObject;

    groupObject.MemberList1.Word = H_GetModuleFlexGroupMemberList(group);
    
    // we're using Type 1, because in our implementation we're only properly
    // supporting the module with 12 channels
    groupObject.Type1.Word = H_GetModuleFlexGroupType(group);

    return groupObject;
}


void HV_FADC_Obj::H_SetFlexGroup(int group, GroupSTRUCT groupObject){
    // call both set flex group functions from the module
    // and write them to the HV module

    H_SetModuleFlexGroupMemberList(group, groupObject.MemberList1.Word);
    
    // we're using Type 1, because in our implementation we're only properly
    // supporting the module with 12 channels
    H_SetModuleFlexGroupType(group, groupObject.Type1.Word);
}








//**************************************************
//*************** FADC functions *******************
//**************************************************




// General FADC functions
void         HV_FADC_Obj::F_Reset() throw(){
    FADC_module->reset();
}

void         HV_FADC_Obj::F_StartAcquisition() throw(){
    FADC_module->startAcquisition();
}

void         HV_FADC_Obj::F_SetFrequency( const unsigned short& frequency ) throw(){
    FADC_module->setFrequency( frequency );
}

unsigned int HV_FADC_Obj::F_GetFrequency() throw(){
    return FADC_module->getFrequency();
}

void HV_FADC_Obj::F_SetReadMode( const unsigned short& mode ) throw(){
    FADC_module->setReadMode( mode );
}

unsigned int HV_FADC_Obj::F_GetReadMode() throw(){
    return FADC_module->getReadMode();
}

unsigned int HV_FADC_Obj::F_GetFPGAVersion() throw(){
    return FADC_module->getFPGAVersion();
}

 /***********  HV_FADC_Obj::F_Trigger settings  ***********/
void         HV_FADC_Obj::F_SetTriggerThresholdDACAll(const unsigned int threshold) throw(){
    FADC_module->setTriggerThresholdDACAll(threshold);
}

unsigned int HV_FADC_Obj::F_GetTriggerThresholdDACAll() throw(){
    return FADC_module->getTriggerThresholdDACAll();
}

void         HV_FADC_Obj::F_SetTriggerThresholdDACPerChannel(const unsigned short chNb, const unsigned int threshold) throw(){
    FADC_module->setTriggerThresholdDACPerChannel(chNb, threshold);
}

unsigned int HV_FADC_Obj::F_GetTriggerThresholdDACPerChannel(const unsigned short chNb) throw(){
    return FADC_module->getTriggerThresholdDACPerChannel(chNb);
}

void         HV_FADC_Obj::F_LoadTriggerThresholdDAC() throw(){
    FADC_module->loadTriggerThresholdDAC();
}

unsigned int HV_FADC_Obj::F_GetTriggerRecord() throw(){
    return FADC_module->getTriggerRecord();
}

void         HV_FADC_Obj::F_SendSoftwareTrigger() throw(){
    FADC_module->sendSoftwareTrigger();
}

void         HV_FADC_Obj::F_SetPretrig( const unsigned short& bits ) throw(){
    FADC_module->setPretrig( bits );
}

unsigned short HV_FADC_Obj::F_GetPretrig() throw(){
    return FADC_module->getPretrig();
}

void         HV_FADC_Obj::F_SetPosttrig( const unsigned short& bits ) throw(){
    FADC_module->setPosttrig( bits );
}

unsigned int HV_FADC_Obj::F_GetPosttrig() throw(){
    return FADC_module->getPosttrig();
}

void         HV_FADC_Obj::F_SetTriggerType( const unsigned short& type ) throw(){
    FADC_module->setTriggerType( type );
}

unsigned short HV_FADC_Obj::F_GetTriggerType() throw(){
    return FADC_module->getTriggerType();
}

void         HV_FADC_Obj::F_SetTriggerChannelSource( const unsigned short& mask ) throw(){
    FADC_module->setTriggerChannelSource( mask );
}

unsigned short HV_FADC_Obj::F_GetTriggerChannelSource() throw(){
    return FADC_module->getTriggerChannelSource();
}

/***********  FADC Readout settings  ***********/
unsigned short HV_FADC_Obj::F_GetModeRegister() throw(){
    return FADC_module->getModeRegister();
}

void           HV_FADC_Obj::F_SetModeRegister(const unsigned short& mode) throw(){
    FADC_module->setModeRegister(mode);
}

void           HV_FADC_Obj::F_SetColToRead( const unsigned short& col ) throw(){
    FADC_module->setColToRead( col );
}

unsigned short HV_FADC_Obj::F_GetColToRead() throw(){
    return FADC_module->getColToRead();
}

void           HV_FADC_Obj::F_SetChannelMask( const unsigned short& mask ) throw(){
    FADC_module->setChannelMask( mask );
}

unsigned short HV_FADC_Obj::F_GetChannelMask() throw(){
    return FADC_module->getChannelMask();
}

void           HV_FADC_Obj::F_SetNbOfChannels(const unsigned short& nbOfChannels) throw(){
    FADC_module->setNbOfChannels(nbOfChannels);
}

unsigned short HV_FADC_Obj::F_GetNbOfChannels() throw(){
    return FADC_module->getNbOfChannels();
}

void           HV_FADC_Obj::F_SetPostStopLatency( const unsigned short& latency ) throw(){
    FADC_module->setPostStopLatency( latency );
}

unsigned short HV_FADC_Obj::F_GetPostStopLatency() throw(){
    return FADC_module->getPostStopLatency();
}

void           HV_FADC_Obj::F_SetPostLatencyPretrig( const unsigned short& latency ) throw(){
    FADC_module->setPostLatencyPretrig( latency );
}

unsigned short HV_FADC_Obj::F_GetPostLatencyPretrig() throw(){
    return FADC_module->getPostLatencyPretrig();
}

void           HV_FADC_Obj::F_SetRAMAddress( const unsigned short& add ) throw(){
    FADC_module->setRAMAddress( add );
}

unsigned short HV_FADC_Obj::F_GetRAMAddress() throw(){
    return FADC_module->getRAMAddress();
}

std::vector< int > HV_FADC_Obj::F_GetAllData( const unsigned short& nbChannels) throw(){
    return FADC_module->getAllData( nbChannels);
}

std::vector< int > HV_FADC_Obj::F_GetAllDataBlockMode() throw(){
    return FADC_module->getAllDataBlockMode();
}

int            HV_FADC_Obj::F_GetDataAt( const unsigned short& add ) throw(){
    return FADC_module->getDataAt( add );
}

/***********  FADC Interrupt settings  ***********/
void           HV_FADC_Obj::F_ReleaseInterrupt() throw(){
    FADC_module->releaseInterrupt();
}

bool           HV_FADC_Obj::F_ReadDeviceInterrupt() throw(){
    return FADC_module->readDeviceInterrupt();
}

bool           HV_FADC_Obj::F_ReadInterrupt() throw(){
    return FADC_module->readInterrupt();
}


// **************************************************
// *********** HV COMMANDS **************************
// **************************************************



// --- HV Module commands ----------------------------------------------------
int    HV_FADC_Obj::H_GetModuleDeviceClass(void){
    return HV_module->GetModuleDeviceClass();
}

uint32_t HV_FADC_Obj::H_GetModulePlacedChannels(void){
    return HV_module->GetModulePlacedChannels();
}

int    HV_FADC_Obj::H_GetModuleSerialNumber(void){
    return HV_module->GetModuleSerialNumber();
}

uint32_t HV_FADC_Obj::H_GetModuleFirmwareRelease(void){
    return HV_module->GetModuleFirmwareRelease();
}


void   HV_FADC_Obj::H_DoClear(void){
    HV_module->DoClear();
}


void   HV_FADC_Obj::H_SetModuleBaseAddress(uint16_t ba){
    HV_module->SetModuleBaseAddress(ba);
}


float  HV_FADC_Obj::H_GetModuleTemperature(void){
    return HV_module->GetModuleTemperature();
}

float  HV_FADC_Obj::H_GetModuleSupplyP5(void){
    return HV_module->GetModuleSupplyP5();
}

float  HV_FADC_Obj::H_GetModuleSupplyP12(void){
    return HV_module->GetModuleSupplyP12();
}

float  HV_FADC_Obj::H_GetModuleSupplyN12(void){
    return HV_module->GetModuleSupplyN12();
}


uint32_t HV_FADC_Obj::H_GetModuleAdcSamplesPerSecond(void){
    return HV_module->GetModuleAdcSamplesPerSecond();
}

void   HV_FADC_Obj::H_SetModuleAdcSamplesPerSecond(uint32_t sps){
    HV_module->SetModuleAdcSamplesPerSecond(sps);
}

uint32_t HV_FADC_Obj::H_GetModuleDigitalFilter(void){
    return HV_module->GetModuleDigitalFilter();
}

void   HV_FADC_Obj::H_SetModuleDigitalFilter(uint32_t filter){
    HV_module->SetModuleDigitalFilter(filter);
}


float  HV_FADC_Obj::H_GetModuleVoltageLimit(void){
    return HV_module->GetModuleVoltageLimit();
}

float  HV_FADC_Obj::H_GetModuleCurrentLimit(void){
    return HV_module->GetModuleCurrentLimit();
}


uint32_t HV_FADC_Obj::H_GetModuleStatus(void){
    return HV_module->GetModuleStatus();
}

uint32_t HV_FADC_Obj::H_GetModuleEventStatus(void){
    return HV_module->GetModuleEventStatus();
}

void   HV_FADC_Obj::H_SetModuleEventStatus(uint32_t status){
    HV_module->SetModuleEventStatus(status);
}

uint32_t HV_FADC_Obj::H_GetModuleEventChannelStatus(void){
    return HV_module->GetModuleEventChannelStatus();
}

void   HV_FADC_Obj::H_ClearModuleEventChannelStatus(void){
    HV_module->ClearModuleEventChannelStatus();
}

uint32_t HV_FADC_Obj::H_GetModuleEventChannelMask(void){
    return HV_module->GetModuleEventChannelMask();
}

void   HV_FADC_Obj::H_SetModuleEventChannelMask(uint32_t mask){
    HV_module->SetModuleEventChannelMask(mask);
}

uint32_t HV_FADC_Obj::H_GetModuleEventGroupStatus(void){
    return HV_module->GetModuleEventGroupStatus();
}

uint32_t HV_FADC_Obj::HV_FADC_Obj::H_GetModuleEventGroupMask(void){
    return HV_module->GetModuleEventGroupMask();
}

void   HV_FADC_Obj::H_SetModuleEventGroupMask(uint32_t mask){
    HV_module->SetModuleEventGroupMask(mask);
}

void   HV_FADC_Obj::H_ClearModuleEventGroupStatus(void){
    HV_module->ClearModuleEventGroupStatus();
}

uint32_t HV_FADC_Obj::H_GetModuleEventMask(void){
    return HV_module->GetModuleEventMask();
}

void   HV_FADC_Obj::H_SetModuleEventMask(uint32_t mask){
    HV_module->SetModuleEventMask(mask);
}

uint32_t HV_FADC_Obj::H_GetModuleControl(void){
    return HV_module->GetModuleControl();
}

void   HV_FADC_Obj::H_SetModuleControl(uint32_t control){
    HV_module->SetModuleControl(control);
}


float  HV_FADC_Obj::H_GetModuleVoltageRampSpeed(void){
    return HV_module->GetModuleVoltageRampSpeed();
}

void   HV_FADC_Obj::H_SetModuleVoltageRampSpeed(float vramp){
    HV_module->SetModuleVoltageRampSpeed(vramp);
}

float  HV_FADC_Obj::H_GetModuleCurrentRampSpeed(void){
    return HV_module->GetModuleCurrentRampSpeed();
}

void   HV_FADC_Obj::H_SetModuleCurrentRampSpeed(float iramp){
    HV_module->SetModuleCurrentRampSpeed(iramp);
}


bool   HV_FADC_Obj::H_GetModuleKillEnable(void){
    return HV_module->GetModuleKillEnable();
}

void   HV_FADC_Obj::H_SetModuleKillEnable(bool enable){
    HV_module->SetModuleKillEnable(enable);
}


int    HV_FADC_Obj::H_GetModuleChannelNumber(void){
    return HV_module->GetModuleChannelNumber();
}


void   HV_FADC_Obj::H_SetModuleEmergencyOff(void){
    HV_module->SetModuleEmergencyOff();
}

void   HV_FADC_Obj::H_ClearModuleEmergencyOff(void){
    HV_module->ClearModuleEmergencyOff();
}


uint32_t HV_FADC_Obj::H_GetModuleRestartTime(void){
    return HV_module->GetModuleRestartTime();
}

void   HV_FADC_Obj::H_SetModuleRestartTime(uint32_t restartTime){
    HV_module->SetModuleRestartTime(restartTime);
}


void   HV_FADC_Obj::H_SetAllChannelsOn(void){
    HV_module->SetAllChannelsOn();
}

void   HV_FADC_Obj::H_SetAllChannelsOff(void){
    HV_module->SetAllChannelsOff();
}

void   HV_FADC_Obj::H_SetAllChannelsVoltageSet(float vset){
    HV_module->SetAllChannelsVoltageSet(vset);
}

void   HV_FADC_Obj::H_SetAllChannelsCurrentSet(float iset){
    HV_module->SetAllChannelsCurrentSet(iset);
}


// --- HV Channel commands ---------------------------------------------------
uint32_t HV_FADC_Obj::H_GetChannelStatus(int channel){
    return HV_module->GetChannelStatus(channel);
}

uint32_t HV_FADC_Obj::H_GetChannelEventStatus(int channel){
    return HV_module->GetChannelEventStatus(channel);
}

void   HV_FADC_Obj::H_SetChannelEventStatus(int channel, uint32_t status){
    HV_module->SetChannelEventStatus(channel, status);
}

uint32_t HV_FADC_Obj::H_GetChannelEventMask(int channel){
    return HV_module->GetChannelEventMask(channel);
}

void   HV_FADC_Obj::H_SetChannelEventMask(int channel, uint32_t mask){
    HV_module->SetChannelEventMask(channel, mask);
}

uint32_t HV_FADC_Obj::H_GetChannelControl(int channel){
    return HV_module->GetChannelControl(channel);
}

void   HV_FADC_Obj::H_SetChannelControl(int channel, uint32_t control){
    HV_module->SetChannelControl(channel, control);
}

void HV_FADC_Obj::H_SetChannelEmergencyOff(int channel){
    HV_module->SetChannelEmergencyOff(channel);
}

void   HV_FADC_Obj::H_ClearChannelEmergencyOff(int channel){
    HV_module->ClearChannelEmergencyOff(channel);
}


float  HV_FADC_Obj::H_GetChannelVoltageNominal(int channel){
    return HV_module->GetChannelVoltageNominal(channel);
}

float  HV_FADC_Obj::H_GetChannelCurrentNominal(int channel){
    return HV_module->GetChannelCurrentNominal(channel);
}


float  HV_FADC_Obj::H_GetChannelVoltageSet(int channel){
    return HV_module->GetChannelVoltageSet(channel);
}

void   HV_FADC_Obj::H_SetChannelVoltageSet(int channel, float vset){
    HV_module->SetChannelVoltageSet(channel, vset);
}

float  HV_FADC_Obj::H_GetChannelVoltageMeasure(int channel){
    return HV_module->GetChannelVoltageMeasure(channel);
}

float  HV_FADC_Obj::H_GetChannelCurrentSet(int channel){
    return HV_module->GetChannelCurrentSet(channel);
}

void   HV_FADC_Obj::H_SetChannelCurrentSet(int channel, float iset){
    HV_module->SetChannelCurrentSet(channel, iset);
}

float  HV_FADC_Obj::H_GetChannelCurrentMeasure(int channel){
    return HV_module->GetChannelCurrentMeasure(channel);
}


float  HV_FADC_Obj::H_GetChannelVoltageIlkMaxSet(int channel){
    return HV_module->GetChannelVoltageIlkMaxSet(channel);
}

void   HV_FADC_Obj::H_SetChannelVoltageIlkMaxSet(int channel, float ilkmax){
    HV_module->SetChannelVoltageIlkMaxSet(channel, ilkmax);
}

float  HV_FADC_Obj::H_GetChannelVoltageIlkMinSet(int channel){
    return HV_module->GetChannelVoltageIlkMinSet(channel);
}

void   HV_FADC_Obj::H_SetChannelVoltageIlkMinSet(int channel, float ilkmin){
    HV_module->SetChannelVoltageIlkMinSet(channel, ilkmin);
}


float  HV_FADC_Obj::H_GetChannelCurrentIlkMaxSet(int channel){
    return HV_module->GetChannelCurrentIlkMaxSet(channel);
}

void   HV_FADC_Obj::H_SetChannelCurrentIlkMaxSet(int channel, float ilkmax){
    HV_module->SetChannelCurrentIlkMaxSet(channel, ilkmax);
}

float  HV_FADC_Obj::H_GetChannelCurrentIlkMinSet(int channel){
    return HV_module->GetChannelCurrentIlkMinSet(channel);
}

void   HV_FADC_Obj::H_SetChannelCurrentIlkMinSet(int channel, float ilkmin){
    HV_module->SetChannelCurrentIlkMinSet(channel, ilkmin);
}


void   HV_FADC_Obj::H_SetChannelOn(int channel){
    HV_module->SetChannelOn(channel);
}

void   HV_FADC_Obj::H_SetChannelOff(int channel){
    HV_module->SetChannelOff(channel);
}


// --- HV Convenience commands -----------------------------------------------
QString HV_FADC_Obj::H_FormatVoltage(double v){
    return HV_module->FormatVoltage(v);
}

QString HV_FADC_Obj::H_FormatVoltageUnit(double v){
    return HV_module->FormatVoltageUnit(v);
}

QString HV_FADC_Obj::H_GetVoltageUnit(void){
    return HV_module->GetVoltageUnit();
}


QString HV_FADC_Obj::H_FormatCurrent(double i){
    return HV_module->FormatCurrent(i);
}

QString HV_FADC_Obj::H_FormatCurrentUnit(double i){
    return HV_module->FormatCurrentUnit(i);
}

QString HV_FADC_Obj::H_GetCurrentUnit(void){
    return HV_module->GetCurrentUnit();
}


// --- HV Interlock Out control / status commands ----------------------------
uint32_t HV_FADC_Obj::H_GetModuleIlkOutStatus(void){
    return HV_module->GetModuleIlkOutStatus();
}

uint32_t HV_FADC_Obj::H_GetModuleIlkOutControl(void){
    return HV_module->GetModuleIlkOutControl();
}

void   HV_FADC_Obj::H_SetModuleIlkOutControl(uint32_t control){
    HV_module->SetModuleIlkOutControl(control);
}

uint32_t HV_FADC_Obj::H_GetModuleIlkOutCount(void){
    return HV_module->GetModuleIlkOutCount();
}

uint32_t HV_FADC_Obj::H_GetModuleIlkOutLastTrigger(void){
    return HV_module->GetModuleIlkOutLastTrigger();
}

uint32_t HV_FADC_Obj::H_GetModuleIlkOutChnActualActive(void){
    return HV_module->GetModuleIlkOutChnActualActive();
}

uint32_t HV_FADC_Obj::H_GetModuleIlkOutChnEverTriggered(void){
    return HV_module->GetModuleIlkOutChnEverTriggered();
}


// --- HV Variable Groups ----------------------------------------------------
uint32_t HV_FADC_Obj::H_GetModuleFlexGroupMemberList(int group){
    return HV_module->GetModuleFlexGroupMemberList(group);
}

void   HV_FADC_Obj::H_SetModuleFlexGroupMemberList(int group, uint32_t member){
    HV_module->SetModuleFlexGroupMemberList(group, member);
}

uint32_t HV_FADC_Obj::H_GetModuleFlexGroupType(int group){
    return HV_module->GetModuleFlexGroupType(group);
}

void   HV_FADC_Obj::H_SetModuleFlexGroupType(int group, uint32_t type){
    HV_module->SetModuleFlexGroupType(group, type);
}


float  HV_FADC_Obj::H_GetChannelVoltageHardwareNominal(int channel){
    return HV_module->GetChannelVoltageHardwareNominal(channel);
}

void   HV_FADC_Obj::H_SetChannelVoltageNominal(int channel, float maxset){
    HV_module->SetChannelVoltageNominal(channel, maxset);
}

float  HV_FADC_Obj::H_GetChannelCurrentHardwareNominal(int channel){
    return HV_module->GetChannelCurrentHardwareNominal(channel);
}

void   HV_FADC_Obj::H_SetChannelCurrentNominal(int channel, float maxset){
    HV_module->SetChannelCurrentNominal(channel, maxset);
}


// --- HV Special Control ----------------------------------------------------
void   HV_FADC_Obj::H_SetModuleSpecialControlCommand(uint32_t command){
    HV_module->SetModuleSpecialControlCommand(command);
}

uint32_t HV_FADC_Obj::H_GetModuleSpecialControlCommand(void){
    return HV_module->GetModuleSpecialControlCommand();
}

uint32_t HV_FADC_Obj::H_GetModuleSpecialControlStatus(void){
    return HV_module->GetModuleSpecialControlStatus();
}

void   HV_FADC_Obj::H_SendHexLine(QByteArray record){
    HV_module->SendHexLine(record);
}

void   HV_FADC_Obj::H_ProgramModuleBaseAddress(uint16_t address){
    HV_module->ProgramModuleBaseAddress(address);
}

uint16_t HV_FADC_Obj::H_VerifyModuleBaseAddress(void){
    return HV_module->VerifyModuleBaseAddress();
}

