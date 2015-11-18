#include "HV_FADC_Obj.h"
#include <QDir>
#include <QSettings>
#include <QCoreApplication>
#include <unistd.h>
#include "const.h"


#include <thread>
#include <chrono>

// either create object using addresses for FADC and HV
HV_FADC_Obj::HV_FADC_Obj(int sAddress_fadc, int baseAddress_hv){
    // upon initialisation of HV_FADC_Obj, we need to create the
    // V1729a_VME (FADC) instance and vmemodule (HV) instance
    // using the base Addresses

    // TODO: make sure that this baseAddress_hv is actually the one
    // given to the function and not the private int variable
    // of the object itself with the same name

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
    
}

// or create object and immediately initialize object for TOS using .ini file
HV_FADC_Obj::HV_FADC_Obj(QString iniFilePath){
    // upon initialisation of HV_FADC_Obj, we need to initialize the
    // controller

    // initialize Controller
    Controller.initController(0);
    
    // set the object created flag to false
    hvFadcObjectCreatedFlag = false;
    // and set the ini file path variable to the given input
    iniFile = iniFilePath;

    // and call the initialization for TOS    
    InitHFOForTOS(iniFile);
}





HV_FADC_Obj::~HV_FADC_Obj() {

    // Before we delete the FADC and HV modules,
    // we should properly shut both of them down
    // for FADC: reset?
    FADC_module->reset();
    
    // TODO: check, if voltages already ramped down?

    // call shut down function
    ShutDownHFOForTOS();

    // HV: shut down controller
    Controller.closeController();

    delete(FADC_module);
    //delete(Controller);
    delete(HV_module);
}



void HV_FADC_Obj::InitHFOForTOS(QString iniFilePath){
    // what do we need to do? 
    // we read the settings from a file, given to this function
    std::cout << "Entering Init HFO for TOS" << std::endl;
    iniFile = iniFilePath;

    // First we read the Settings file
    // in case HV_FADC_ObjectCreatedFlag is false 
    // we use the read settings to create the objects properly
    

    // define variables

    // TODO: in IniFile, need to set Vnominal values, Inominal values
    //       for that, first set setStop flag == 1.
    //       Vbounds, Ibounds
    ModuleStatusSTRUCT  moduleStatus = { 0 };
    ModuleControlSTRUCT moduleControl = { 0 };
    ChControlSTRUCT     channelControl = { 0 };
    int gridChannelNumberBinary;
    int anodeChannelNumberBinary;
    int cathodeChannelNumberBinary;

    //bool ok = true;
    
    // create path for iniFile 
    QDir dir(get_current_dir_name());
    // TODO: check proper path
    //       or rather: add option to give full path or
    //       relative?
    iniFile = dir.absolutePath() + '/' + iniFilePath;
    std::cout << "Path to Ini File: " << iniFile.toStdString() << std::endl;

    // now we have read the QString with the path
    // to the iniFile
    // create a QSettings object from this file
    // settings object contains every setting from file
    // TODO: check if file exists
    QSettings settings(iniFile, QSettings::IniFormat);
    

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
	sAddress_fadc = DEFAULT_S_ADDRESS_FADC;
    }

    if (settings.contains("setKillEnable")){
	setKillEnable = settings.value("setKillEnable").toBool();    
    }
    else{
	setKillEnable = DEFAULT_SET_KILL_ENABLE;
    }

    if (settings.contains("anodeGridGroupFlag")){
	anodeGridGroupFlag = settings.value("anodeGridGroupFlag").toBool();
    }
    else{
	anodeGridGroupFlag = DEFAULT_ANODE_GRID_GROUP_FLAG;
    }

    if (settings.contains("anodeGridGroupMasterChannel")){
	anodeGridGroupMasterChannel = settings.value("anodeGridGroupMasterChannel").toInt();
    }
    else{
	anodeGridGroupMasterChannel = DEFAULT_ANODE_GRID_GROUP_MASTER_CHANNEL;
    }

    if (settings.contains("anodeGridGroupNumber")){
	anodeGridGroupNumber = settings.value("anodeGridGroupNumber").toInt();
    }
    else{
	anodeGridGroupNumber = DEFAULT_ANODE_GRID_GROUP_NUMBER;
    }


    if (settings.contains("monitorTripGroupFlag")){
	monitorTripGroupFlag = settings.value("monitorTripGroupFlag").toBool();
    }
    else{
	monitorTripGroupFlag = DEFAULT_MONITOR_TRIP_GROUP_FLAG;
    }

    if (settings.contains("monitorTripGroupNumber")){
	monitorTripGroupNumber = settings.value("monitorTripGroupNumber").toInt();
    }
    else{
	monitorTripGroupNumber = DEFAULT_MONITOR_TRIP_GROUP_NUMBER;
    }

    if (settings.contains("rampingGroupFlag")){
	rampingGroupFlag = settings.value("rampingGroupFlag").toBool();
    }
    else{
	rampingGroupFlag = DEFAULT_RAMPING_GROUP_FLAG;
    }

    if (settings.contains("rampingGroupNumber")){
	rampingGroupNumber = settings.value("rampingGroupNumber").toInt();
    }
    else{
	rampingGroupNumber = DEFAULT_RAMPING_GROUP_NUMBER;
    }


    // **************************************************
    // *************** GRID VARIABLES *******************
    // **************************************************

    if (settings.contains("gridChannelNumber")){
	gridChannelNumber = settings.value("gridChannelNumber").toInt();
    }
    else{
	gridChannelNumber = DEFAULT_GRID_CHANNEL_NUMBER;
    }

    if (settings.contains("gridVoltageSet")){
	gridVoltageSet = settings.value("gridVoltageSet").toFloat();
    }
    else{
	gridVoltageSet = DEFAULT_GRID_VOLTAGE_SET;
    }

    if (settings.contains("gridVoltageNominal")){
	gridVoltageNominal = settings.value("gridVoltageNominal").toFloat();
    }
    else{
	gridVoltageNominal = DEFAULT_GRID_VOLTAGE_NOMINAL;
    }

    if (settings.contains("gridVoltageBound")){
	gridVoltageBound = settings.value("gridVoltageBound").toFloat();
    }
    else{
	gridVoltageBound = DEFAULT_GRID_VOLTAGE_BOUND;
    }

    if (settings.contains("gridCurrentSet")){
	gridCurrentSet = settings.value("gridCurrentSet").toFloat();
    }
    else{
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
	gridCurrentNominal = DEFAULT_GRID_CURRENT_NOMINAL;
    }

    if (settings.contains("gridCurrentBound")){
	gridCurrentBound = settings.value("gridCurrentBound").toFloat();
    }
    else{
	gridCurrentBound = DEFAULT_GRID_CURRENT_BOUND;
    }

    // **************************************************
    // *************** ANODE VARIABLES ******************
    // **************************************************

    if (settings.contains("anodeChannelNumber")){
	anodeChannelNumber = settings.value("anodeChannelNumber").toInt();
    }
    else{
	anodeChannelNumber = DEFAULT_ANODE_CHANNEL_NUMBER;
    }

    if (settings.contains("anodeVoltageSet")){
	anodeVoltageSet = settings.value("anodeVoltageSet").toFloat();
    }
    else{
	anodeVoltageSet = DEFAULT_ANODE_VOLTAGE_SET;
    }

    if (settings.contains("anodeVoltageNominal")){
	anodeVoltageNominal = settings.value("anodeVoltageNominal").toFloat();
    }
    else{
	anodeVoltageNominal = DEFAULT_ANODE_VOLTAGE_NOMINAL;
    }

    if (settings.contains("anodeVoltageBound")){
	anodeVoltageBound = settings.value("anodeVoltageBound").toFloat();
    }
    else{
	anodeVoltageBound = DEFAULT_ANODE_VOLTAGE_BOUND;
    }

    if (settings.contains("anodeCurrentSet")){
	anodeCurrentSet = settings.value("anodeCurrentSet").toFloat();
    }
    else{
	anodeCurrentSet = DEFAULT_ANODE_CURRENT_SET;
    }

    if (settings.contains("anodeCurrentNominal")){
	anodeCurrentNominal = settings.value("anodeCurrentNominal").toFloat();
    }
    else{
	anodeCurrentNominal = DEFAULT_ANODE_CURRENT_NOMINAL;
    }

    if (settings.contains("anodeCurrentBound")){
	anodeCurrentBound = settings.value("anodeCurrentBound").toFloat();
    }
    else{
	anodeCurrentBound = DEFAULT_ANODE_CURRENT_BOUND;
    }

    // **************************************************
    // *************** CATHODE VARIABLES ******************
    // **************************************************

    if (settings.contains("cathodeChannelNumber")){
	cathodeChannelNumber = settings.value("cathodeChannelNumber").toInt();
    }
    else{
	cathodeChannelNumber = DEFAULT_CATHODE_CHANNEL_NUMBER;
    }

    if (settings.contains("cathodeVoltageSet")){
	cathodeVoltageSet = settings.value("cathodeVoltageSet").toFloat();
    }
    else{
	cathodeVoltageSet = DEFAULT_CATHODE_VOLTAGE_SET;
    }

    if (settings.contains("cathodeVoltageNominal")){
	cathodeVoltageNominal = settings.value("cathodeVoltageNominal").toFloat();
    }
    else{
	cathodeVoltageNominal = DEFAULT_CATHODE_VOLTAGE_NOMINAL;
    }

    if (settings.contains("cathodeVoltageBound")){
	cathodeVoltageBound = settings.value("cathodeVoltageBound").toFloat();
    }
    else{
	cathodeVoltageBound = DEFAULT_CATHODE_VOLTAGE_BOUND;
    }

    if (settings.contains("cathodeCurrentSet")){
	cathodeCurrentSet = settings.value("cathodeCurrentSet").toFloat();
    }
    else{
	cathodeCurrentSet = DEFAULT_CATHODE_CURRENT_SET;
    }

    if (settings.contains("cathodeCurrentNominal")){
	cathodeCurrentNominal = settings.value("cathodeCurrentNominal").toFloat();
    }
    else{
	cathodeCurrentNominal = DEFAULT_CATHODE_CURRENT_NOMINAL;
    }

    if (settings.contains("cathodeCurrentBound")){
	cathodeCurrentBound = settings.value("cathodeCurrentBound").toFloat();
    }
    else{
	cathodeCurrentBound = DEFAULT_CATHODE_CURRENT_BOUND;
    }


    // **************************************************
    // ***************** MODULE SETTINGS ****************
    // **************************************************

    if (settings.contains("moduleVoltageRampSpeed")){
	moduleVoltageRampSpeed = settings.value("moduleVoltageRampSpeed").toFloat();
    }
    else{
	moduleVoltageRampSpeed = DEFAULT_MODULE_VOLTAGE_RAMP_SPEED;
    }

    if (settings.contains("moduleCurrentRampSpeed")){
	moduleCurrentRampSpeed = settings.value("moduleCurrentRampSpeed").toFloat();
    }
    else{
	moduleCurrentRampSpeed = DEFAULT_MODULE_CURRENT_RAMP_SPEED;
    }

    if (settings.contains("checkModuleTimeInterval")){
	checkModuleTimeInterval = settings.value("checkModuleTimeInterval").toInt();
    }
    else{
	checkModuleTimeInterval = DEFAULT_CHECK_MODULE_TIME_INTERVAL;
    }


    // use the just read settings to create the objects, if not
    // yet created
    // check if FADC and HV modules have been created
    if (HV_FADC_ObjectCreatedFlag == false){

        HV_module   = new CVmeModule(&Controller, baseAddress_hv);
        std::cout << "creating.. " << HV_module->IsConnected() <<std::endl;
        FADC_module = new V1729a_VME(&Controller, sAddress_fadc);
	// create FADC high level functions
        FADC_Functions = new HighLevelFunction_VME(FADC_module);
	// and reset FADC
        FADC_module->reset();

	
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

    // groups set up. now set voltages, currents


    //****************** Set Nominal values **************************
    
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


    timeout = 1000;
    while(timeout > 0){
        // all that is left to do, is start the ramp up
        // that means set channels to SetOn = 1
        // for the anodeGridGroup, we only set the master channel
        // to SetOn = 1
	ChStatusSTRUCT gridStatus = { 0 };
	ChStatusSTRUCT anodeStatus = { 0 };
	ChStatusSTRUCT cathodeStatus = { 0 };
	gridStatus.Word = H_GetChannelStatus(gridChannelNumber);
	anodeStatus.Word = H_GetChannelStatus(anodeChannelNumber);
	cathodeStatus.Word = H_GetChannelStatus(cathodeChannelNumber);

	if(gridStatus.Bit.isOn == 0 ||
	   anodeStatus.Bit.isOn == 0 ||
	   cathodeStatus.Bit.isOn == 0){
            // can be exchanged by H_SetChannelOn(int channel)
            channelControl.Word = H_GetChannelControl(anodeGridGroupMasterChannel);
            channelControl.Bit.setON = 1;
            H_SetChannelControl(anodeGridGroupMasterChannel, channelControl.Word);
            //H_SetChannelOn(anodeGridGroupMasterChannel);

            channelControl.Word = H_GetChannelControl(cathodeChannelNumber);
            channelControl.Bit.setON = 1;
            H_SetChannelControl(cathodeChannelNumber, channelControl.Word);
	}
	else if (gridStatus.Bit.isOn == 1 &&
		 anodeStatus.Bit.isOn == 1 &&
		 cathodeStatus.Bit.isOn == 1){
	    std::cout << "All channels set to ON" << std::endl;
	    break;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
	timeout--;
    }
    // channels should now start to ramp

    // now check while is ramping
    bool rampUpFlag = true;
    H_CheckModuleIsRamping(rampUpFlag);

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
    // TODO: define a CheckRamping convenience function, which
    // does all this?
    //************************************************************
    int timeout;


    ChEventStatusSTRUCT gridEventStatus = { 0 };
    ChEventStatusSTRUCT anodeEventStatus = { 0 };
    ChEventStatusSTRUCT cathodeEventStatus = { 0 };
    gridEventStatus.Word = H_GetChannelEventStatus(gridChannelNumber);
    anodeEventStatus.Word = H_GetChannelEventStatus(anodeChannelNumber);
    cathodeEventStatus.Word = H_GetChannelEventStatus(cathodeChannelNumber);

    timeout = 1000;
    while(timeout>0){
        uint16_t resetChannelEventStatus = 0;
        H_SetChannelEventStatus(gridChannelNumber, resetChannelEventStatus);
        H_SetChannelEventStatus(anodeChannelNumber, resetChannelEventStatus);
        H_SetChannelEventStatus(cathodeChannelNumber, resetChannelEventStatus);
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); 
	gridEventStatus.Word = H_GetChannelEventStatus(gridChannelNumber);
	anodeEventStatus.Word = H_GetChannelEventStatus(anodeChannelNumber);
	cathodeEventStatus.Word = H_GetChannelEventStatus(cathodeChannelNumber);
	if (gridEventStatus.Bit.EventEndOfRamping == 0 &&
	    anodeEventStatus.Bit.EventEndOfRamping == 0 &&
	    cathodeEventStatus.Bit.EventEndOfRamping == 0){
	    break;
	}
	timeout--;
    }
    if (timeout == 0){
	std::cout << "ERROR: Timeout while trying to reset EventStatus of channels." << std::endl;
    }
    gridEventStatus.Word = H_GetChannelEventStatus(gridChannelNumber);
    anodeEventStatus.Word = H_GetChannelEventStatus(anodeChannelNumber);
    cathodeEventStatus.Word = H_GetChannelEventStatus(cathodeChannelNumber);
    std::cout << "stuff" << std::endl << gridEventStatus.Bit.EventEndOfRamping << std::endl;


    // define rampUpFlag to differentiate between ramp up 
    // and ramp down
    // based on whether channel status is isOn == 1
    // bool rampUpFlag;
    // get channel status of all channels
    ChStatusSTRUCT gridStatus = { 0 };
    ChStatusSTRUCT anodeStatus = { 0 };
    ChStatusSTRUCT cathodeStatus = { 0 };
    // gridStatus.Word  = H_GetChannelStatus(gridChannelNumber);
    // anodeStatus.Word = H_GetChannelStatus(anodeChannelNumber);
    // cathodeStatus.Word = H_GetChannelStatus(cathodeChannelNumber);
    // if(gridStatus.Bit.isOn == 1 &&
    //    anodeStatus.Bit.isOn == 1 &&
    //    cathodeStatus.Bit.isOn == 1){
    // 	rampUpFlag = true;
    // }
    // else{
    // 	rampUpFlag = false;
    // }

    timeout = 1000;
    while(timeout > 0){
	// get the ramping Group back from the module
	// rampingGroup = H_GetFlexGroup(rampingGroupNumber);
	// TODO: currently not using rampinGroup, because unclear
	//       how status group works
	

	// ChEventStatusSTRUCT gridEventStatus = { 0 };
	// ChEventStatusSTRUCT anodeEventStatus = { 0 };
	// ChEventStatusSTRUCT cathodeEventStatus = { 0 };
	
	float gridVoltageMeasured;
	float anodeVoltageMeasured;
	float cathodeVoltageMeasured;

	float gridCurrentHardwareNominal;
	float anodeCurrentHardwareNominal;
	float cathodeCurrentHardwareNominal;

	std::this_thread::sleep_for(std::chrono::seconds(1)); 
	gridStatus.Word  = H_GetChannelStatus(gridChannelNumber);
	anodeStatus.Word = H_GetChannelStatus(anodeChannelNumber);
	cathodeStatus.Word = H_GetChannelStatus(cathodeChannelNumber);

	gridEventStatus.Word = H_GetChannelEventStatus(gridChannelNumber);
	anodeEventStatus.Word = H_GetChannelEventStatus(anodeChannelNumber);
	cathodeEventStatus.Word = H_GetChannelEventStatus(cathodeChannelNumber);
	

	ModuleControlSTRUCT moduleControl = { 0 };

	// moduleControl.Word = H_GetModuleControl();
	// moduleControl.Bit.DoClear = 1;
	// H_SetModuleControl(moduleControl.Word);

	if (gridStatus.Bit.isOn == 0 ||
	     anodeStatus.Bit.isOn == 0 ||
	     cathodeStatus.Bit.isOn == 0){
	    std::cout << "grid event status word " << gridEventStatus.Word << std::endl;
	    std::cout << "anode event status word " << anodeEventStatus.Word << std::endl;
	    std::cout << "cathode event status word " << cathodeEventStatus.Word << std::endl;
	}

	
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
			  << "Still ramping... Current Voltage of all channels is:" << "\n"
			  << "Grid Voltage / VoltageSet: " << gridVoltageMeasured << " / " << gridVoltageSet << "\n"
			  << "Anode Voltage / VoltageSet: " << anodeVoltageMeasured << " / " << anodeVoltageSet << "\n"
			  << "Cathode Voltage / VoltageSet: " << cathodeVoltageMeasured << " / " << cathodeVoltageSet << std::flush;
	    }
	    else if (gridEventStatus.Bit.EventEndOfRamping == 1 &&
	    	 anodeEventStatus.Bit.EventEndOfRamping == 1 &&
	    	 cathodeEventStatus.Bit.EventEndOfRamping == 1 &&
	    	 gridStatus.Bit.isOn == 1 &&
	    	 anodeStatus.Bit.isOn == 1 &&
	    	 cathodeStatus.Bit.isOn == 1){

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
	    else if (gridEventStatus.Bit.EventEndOfRamping == 1 &&
		     anodeEventStatus.Bit.EventEndOfRamping == 1 &&
		     cathodeEventStatus.Bit.EventEndOfRamping == 1 &&
		     gridStatus.Bit.isOn == 0 &&
		     anodeStatus.Bit.isOn == 0 &&
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

void HV_FADC_Obj::ShutDownHFOForTOS(){
    // this function is called upon deleting the HV_FADC_Obj or
    // and thus when shutting down TOS

    std::cout << std::endl << "Starting shutdown of HV for TOS" << std::endl << std::endl;
    
    // TODO: ramp down
    //       check for any Events
    //       perform a doClear
    //       setStatus to 0x0000 (everything off)
    //       set KillEnable == False
    //       
    ModuleStatusSTRUCT  moduleStatus = { 0 };
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

