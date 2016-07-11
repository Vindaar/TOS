#include "hvFadcManager.hpp"
#include <QDir>
#include <QSettings>
#include <QCoreApplication>
#include "const.h"

// used to control while loops to read / write to HV and
// to create error logs
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
// used to write error logs
#include <fstream>
#include <algorithm>
#include <functional>

// either create object using addresses for HV
hvFadcManager::hvFadcManager(std::string iniFilePath):
    _createdFlag(false),
    _settingsReadFlag(false),
    _hvModInitFlag(false)
{
    // upon initialisation of hvFadcManager, we need to create the
    // vmemodule (HV) instance using the base Addresses

    // set the path of the ini file to the member variable
    iniFile = QString::fromStdString(iniFilePath);

    // first read HV settings from ini file
    ReadHVSettings();

    // set gridEventStatusLastIter, anodeEventStatusLastIter, cathodeEventStatusLastIter
    // TODO: check if this is still doing what we want :)
    gridEventStatusLastIter = { 0 };
    anodeEventStatusLastIter = { 0 };
    cathodeEventStatusLastIter = { 0 };

    // initialize the USB Controller
    Controller.initController(0);
    // and create a new vmemodule to communicate via usb
    HV_module   = new hvModule(&Controller, baseAddress_hv);

    FADC_module = new V1729a_VME(&Controller, sAddress_fadc);
    
    // now create FADC functions object
    FADC_Functions = new HighLevelFunction_VME(FADC_module);
    FADC_module->reset();

    // TODO: think about throwing this code out
    // now we check, whether we can connect to the HV module
    bool good_connection;
    good_connection = H_CheckIsConnectionGood();

    // set the object created flag to true
    _createdFlag = true;
}

hvFadcManager::~hvFadcManager() {

    // Before we delete the HV module,
    // we should properly shut both of them down
    
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
	    ShutDownHFMForTOS();
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

hvModule* hvFadcManager::getHVModulePtr(){
    // this function returns the pointer to the hvModule the creator created
    // can be used to add channels externally from hvFadcManager
    return HV_module;
}

bool hvFadcManager::H_ConnectModule(){
    return HV_module->ConnectModule();
}


uint16_t hvFadcManager::GetBinaryRepFromNumberSet(std::set<int> numberSet){
    // this function is given a std::set<int> containing channels
    // for which a binary integer (each bit representing one channel;
    // if one bit is 1, channel active, if 0 inactive), is created
    uint16_t numberSetInt = 0;
    int newNumber;
    std::set<int>::iterator it;

    for(it = numberSet.begin(); it != numberSet.end(); ++it){
	// loop over all channelSet and assign integer from set
	// corresponding to iterator value to new int
	newNumber = *it;
	// we use this int to perform bitwise operation on 
	// channelNumberBinary
	// we shift a '1'
	// 'newChannelMember' places to the left
	// e.g.: newChannelNumber = 3
	// channelNumberBinary = 1 << 3 --> 00000001 << 3 --> 000001000
	int numberBinary = 0;
	numberBinary = 1 << newNumber;
	
	// and now add this binary number to the channelSetInt
	numberSetInt += numberBinary;
    }

    return numberSetInt;
}

std::set<int> hvFadcManager::GetChannelSetFromBinaryRep(uint32_t binaryRep){
    // this function creates a set of channel ints from a binary representation
    // of a member list

    std::set<int> channelSet;
    int n = 32;
    while(binaryRep != 0){
	int pow_of_2 = pow(2, n);
	if (binaryRep % pow_of_2 != binaryRep){
	    // if binary rep modulo 2^n is not binaryRep, n is a divisor
	    channelSet.insert(n);
	    // new value for rep is simply the rest of the division
	    binaryRep = binaryRep % pow_of_2; 
	}
	// decrease n by 1
	n--;
    }
    
    return channelSet;
}

void hvFadcManager::AddChannel(hvChannel *ptrChannel){
    // this function simply adds a channel that is being used to the 
    // channelList member variable. Thus the object is aware of all its
    // channels. The list is used for actions acting on all channels
    // input is: 
    // hvChannel channel: a channel object to be added
    _channelList.push_back(ptrChannel);
}

void hvFadcManager::AddFlexGroup(hvFlexGroup *ptrGroup){
    // this function adds a flex group to the flex group list member variable
    // channels. 
    // input is: 
    // hvFlexGroup group: a flex group object to be added
    _flexGroupList.push_back(ptrGroup);
}

bool hvFadcManager::SetModuleEventGroupMask(){
    // old comment describing module event group:
    // moduleEventGroupMask is a 32bit integer, where each bit
    // corresponds to the activation of one group. This means, since
    // we wish to set the GroupMask to our three Groups, we need to set
    // each bit for the number of each group. Incidentally, the group numbers
    // are the same as the channel numbers, thus we can use the bitwise
    // created integer for the GroupMembers containing all three channels

    // need to create a unit32_t containing bits for each
    // group which is active.

    std::set<int> groupSet;
    uint16_t activeGroupsBinary;
    std::for_each( _flexGroupList.begin(), _flexGroupList.end(), [&groupSet](hvFlexGroup *ptrGroup){
            int number;
            number = ptrGroup->getGroupNumber();
	    groupSet.insert(number);
	} );

    // now we need to create binary representation of channels
    // thus call GetBinary function
    activeGroupsBinary = GetBinaryRepFromNumberSet(groupSet);
	
    // however, for the event group mask, we need a uint32_t instead
    // of a uint16_t:
    uint32_t moduleEventGroupMask = activeGroupsBinary;

    // now that we have all we need, we can set and check the event group mask
    int timeout = 1000;
    bool good = false;
    while( (timeout > 1) &&
	   (_moduleEventGroupMask == moduleEventGroupMask) ){
	// first set module event group mask
        H_SetModuleEventGroupMask(static_cast<uint32_t> (moduleEventGroupMask));
	// now wait default time
	HV_module->sleepModule();
	// and update module (which updates _moduleEventGroupMask
	HV_module->updateModule();
	_moduleEventGroupMask = HV_module->GetModuleEventGroupMask();
    }
    if (timeout < 1){
	std::cout << "TIMEOUT: could not set module event group mask!" << std::endl;
	std::cout << "returning false" << std::endl;
	good = false;
    }
    else{
	// else everything is good
	good = true;
    }
    return good;
}

bool hvFadcManager::SetAllChannelsOn(){
    // this function loops over all active channels (part of channelList)
    // and calls their turn On function. Checks if all channels successfully
    // turned on
    bool good;
    good = true;
    
    // using for each we get each element of channelList (an hvChannel) and call 
    // its clearChannelEventStatus function. By multiplying good (equals to true 
    // before loop) with the result, good will only be true, if all channels return 
    // true
    std::for_each( _channelList.begin(), _channelList.end(), [&good](hvChannel *ptrChannel){
	    good *= ptrChannel->turnOn();
	} );
    if (good == true){
	std::cout << "All channels successfully turned on." << std::endl;
    }
    else{
	std::cout << "One or more channels could not be turned on!" << std::endl;
    }

    return good;
    // ##############################
    // NOTE: code below is still in here simply as an illustration
    // about std::transform for myself
    // ##############################
    // while(timeout > 0){
        // all that is left to do, is start the ramp up
        // that means set channels to SetOn = 1
        // for the anodeGridGroup, we only set the master channel
        // to SetOn = 1
	
	// now use transform to update Status.Word for each element of
	// channel status vector. this updates the elements of chStatusVector
	// transform uses last argument (function) and acts it with arguments contained
	// in 1st and 2nd argument vectors and puts the result of the function in 
	// vector of 3rd argument
	// std::transform( channelNumbers.begin(), 
	// 		channelNumbers.end(),  
	// 		chStatusVector.begin(),
	// 		[this](int channel){
	// 		    return this->H_GetChannelStatusStruct(channel);
	// 		} );
	// explanation of last line: usage of lambda function
	// problem: wish to use H_GetChannelStatusStruct, which is a non static member
	// function of hvFadcManager. Thus, it actually needs to arguments:
	//    - the object which owns the function (non static member functions
	//      only exist, if an object which owns it exists!)
	//    - and the argument (int channel)
	// lambda function acts as a wrapper around this fact. 
	// [this] means we hand the object, which owns this function (SetAllChannelsOn)
	// to the lambda function. then the arguments of the runction and we return 
	// what we wish to return
}

void hvFadcManager::InitHFMForTOS(){
    std::cout << "Entering Init HV" << std::endl;

    // First we read the Settings file
    //     done in ReadHVSettings() called in creator
    // in case _createdFlag is false 
    // we use the read settings to create the objects properly
    

    // define variables

    // TODO: channelControl and moduleControl currently not in use
    //       might be a good idea to implement and check them
    ModuleStatusSTRUCT  moduleStatus = { 0 };
    // ModuleControlSTRUCT moduleControl = { 0 };
    // ChControlSTRUCT     channelControl = { 0 };

    // use the read settings (ReadHVSettings() called in creator)
    // to create the objects, if not yet created
    // check if HV module has been created
    if (_createdFlag == false){
	Controller.initController(0);
        HV_module   = new hvModule(&Controller, baseAddress_hv);
        FADC_module = new V1729a_VME(&Controller, sAddress_fadc);
	// create FADC high level functions
        FADC_Functions = new HighLevelFunction_VME(FADC_module);
	// and reset FADC
        FADC_module->reset();
	bool good;
        std::cout << "creating... " << std::flush;
	good = H_CheckIsConnectionGood();
	_createdFlag = true;
	if (good == 0){
	    // break from initHV if connection could not be established
	    std::cout << "stopping InitHV" << std::endl;
	    return;
	}
    }
    
    // now call H_SetKillEnable to try and set kill to enable
    bool killEnabled;
    killEnabled = HV_module->SetKillEnable(setKillEnable);
    if (killEnabled == false) return;

    // first create channel objects
    // then create flex group from channel objects
    // create channel objects
    // since we're in a member function of hvFadcManager, we just hand the
    // this pointer to the hvChannel as an hvFadcManager *
    hvChannel *gridChannel;
    std::string gridChannelName("grid");
    gridChannel = new hvChannel(HV_module, gridChannelName, gridChannelNumber);
    hvChannel *anodeChannel;
    std::string anodeChannelName("anode");
    anodeChannel = new hvChannel(HV_module, anodeChannelName, anodeChannelNumber);
    hvChannel *cathodeChannel;
    std::string cathodeChannelName("cathode");
    cathodeChannel = new hvChannel(HV_module, cathodeChannelName, cathodeChannelNumber);

    AddChannel(gridChannel);
    AddChannel(anodeChannel);
    AddChannel(cathodeChannel);

    // create channel lists for groups
    std::list<hvChannel *> anodeGridMembers;
    std::list<hvChannel *> monitorTripMembers;
    std::list<hvChannel *> rampingMembers = monitorTripMembers;

    anodeGridMembers.push_back(gridChannel);
    anodeGridMembers.push_back(anodeChannel);
    
    monitorTripMembers.push_back(gridChannel);
    monitorTripMembers.push_back(anodeChannel);
    monitorTripMembers.push_back(cathodeChannel);
    
    // create flex groups
    hvSetGroup anodeGridSetOnGroup(HV_module,
				   "anodeGridSetOnGroup", 
				   anodeGridMembers,
				   anodeGridGroupNumber,
				   anodeGridGroupMasterChannel);

    hvMonitorGroup monitorTripGroup(HV_module,
				    "monitorTripGroup",
				    monitorTripMembers,
				    monitorTripGroupNumber);

				    
    hvStatusGroup rampingGroup(HV_module,
			       "rampingGroup",
			       rampingMembers,
			       rampingGroupNumber);

				   
    // now with our fancy groups, we can just set the options we would like to
    // set
    // define master channel of SetGroup
    // define anode as master Channel (in constructor); include both in .ini file
    anodeGridSetOnGroup.setMode(DEFAULT_ANODE_GRID_GROUP_MODE);
    anodeGridSetOnGroup.setControl("set_on");
    
    // monitor group, need to set monitoring to tripping
    monitorTripGroup.setMonitor("is_tripped");
    // set Mode to 1. That way, the action of the group is called,
    // if one of the group channels reports a 1 for the bit, that is being
    // checked. In our case:
    // Channel.Bit.CurrentTrip == 1 for one channel means group shut down
    monitorTripGroup.setMode(1); 
    monitorTripGroup.setAction("shut_module");

    rampingGroup.setMonitor("is_ramping");

    // TODO: need to set ModuleEventGroupMask for the groups,
    //       in order to register group events for the wanted 
    //       things
    
    // ModuleEventGroupMask: set everything to 0 except 
    //                       gridGroupNumbers of all 3 defined groups


    // ##################################################
    // Event mask related 

    // ChannelEventMask: set all three of our channel masks to the following pattern:
    //                   MaskEventTrip = 1
    //                   MaskEventEndOfRamp = 1
    //                   MaskEventVoltageLimit = 1
    //                   MaskEventCurrentLimit = 1
    //                   MaskEventEmergency = 1
    //                   rest to 0.
    // done by creating set of strings corresponding to options
    std::set<std::string> eventMaskSet = {"CurrentTrip", 
					  "EndOfRamping", 
					  "VoltageLimit", 
					  "CurrentLimit",
					  "Emergency"};

    // after defining the channel event mask, we need to write them
    // to the channels
    bool good;
    good = gridChannel->setChannelEventMask(eventMaskSet);
    if (good == false) return;
    good = anodeChannel->setChannelEventMask(eventMaskSet);
    if (good == false) return;
    good = cathodeChannel->setChannelEventMask(eventMaskSet);
    if (good == false) return;
    
    // call functino to set module event group mask for all active channels we're
    // using and make sure it works. Function uses _channelMap to set mask
    
    good = SetModuleEventGroupMask();
    if (good == false) return;

    // ##################################################
    // Set voltages etc

    // groups set up. now set voltages, currents
    //****************************************************************

    // set voltage set
    good = gridChannel->setVoltage(gridVoltageSet);
    if (good == false) return;
    good = anodeChannel->setVoltage(anodeVoltageSet);
    if (good == false) return;
    good = cathodeChannel->setVoltage(cathodeVoltageSet);
    if (good == false) return;

    // set current set
    good = gridChannel->setCurrent(gridCurrentSet);
    if (good == false) return;
    good = anodeChannel->setCurrent(anodeCurrentSet);
    if (good == false) return;
    good = cathodeChannel->setCurrent(cathodeCurrentSet);
    if (good == false) return;

    // TODO: think about bounds
    // set voltage bound
    // set current bound

    // ##################################################
    // starting up channels (set to On)

    // before we can set the channels to ON, reset the Event Status
    good = ClearAllChannelEventStatus();
    if (good != true){
	std::cout << "Could not reset Event status.\n" 
		  << "Probably will not start ramping."
		  << std::endl;
    }

    // now set all channels to on
    good = SetAllChannelsOn();
    if (good == false) return;

    // before we start ramping up the HV modules, first set FADC settings
    std::cout << "Setting FADC settings" << std::endl;

    F_SetTriggerType(fadcTriggerType);
    F_SetFrequency(fadcFrequency);
    F_SetPosttrig(fadcPosttrig);
    F_SetPretrig(fadcPretrig);
    // TODO: find out what this was supposed to be
    //F_SetTriggerThresholdRegisterAll(fadcTriggerThresholdRegisterAll);
    //F_PrintSettings();

    // now check if ramping started
    bool rampUpFlag = true;
    CheckModuleIsRamping(rampUpFlag);

    _hvModInitFlag = true;

    // moduleControl.Word = H_GetModuleControl();
    // moduleControl.Bit.SetStop = 1;
    // H_SetModuleControl(moduleControl.Word);

    // std::this_thread::sleep_for(std::chrono::seconds(1));

}

void hvFadcManager::CheckModuleIsRamping(bool rampUpFlag){
    // this is a convenience function, which checks whether the
    // active channels (the ones added to _channelList) are
    // currently ramping

    // we can loop over all channels of _channelList and use the isRamping()
    // function 

    int timeout = 100;
    while (timeout > 1){
        // first we define bool variables, to check whether all channels are 
        // on and ramping
	bool allRamping = true;
	bool allOn = true;
	std::for_each( _channelList.begin(), _channelList.end(), [&allOn,&allRamping](hvChannel *ptrChannel){
		allOn      *= ptrChannel->isOn();
		allRamping *= ptrChannel->isRamping();
            } );
        
        if(rampUpFlag == true){
            // in this case we are ramping up
            if( (allRamping == true) &&
                (allOn      == true) ){
                // if all are on and ramping, we print the currently measured voltage and current
                // for each channel
                std::for_each( _channelList.begin(), _channelList.end(), [&allOn,&allRamping](hvChannel *ptrChannel){
                        ptrChannel->printVoltageMeasured();
                        ptrChannel->printCurrentMeasured();
                    } );
            }            
            else if( (allRamping == false) &&
                     (allOn      == true) ){
                std::cout << "All channels ramped up successfully." << std::endl;
                std::for_each( _channelList.begin(), _channelList.end(), [&allOn,&allRamping](hvChannel *ptrChannel){
                        ptrChannel->printVoltageMeasured();
                        ptrChannel->printCurrentMeasured();
                    } );
                break;
            }
        }
        else{
            // in this case we are ramping down
            if( (allRamping == true) &&
                (allOn      == false) ){
                // if all are on and ramping, we print the currently measured voltage and current
                // for each channel
                std::for_each( _channelList.begin(), _channelList.end(), [&allOn,&allRamping](hvChannel *ptrChannel){
                        ptrChannel->printVoltageMeasured();
                        ptrChannel->printCurrentMeasured();
                    } );
            }            
            else if( (allRamping == false) &&
                     (allOn      == false) ){
                std::cout << "All channels ramped down successfully." << std::endl;
                std::for_each( _channelList.begin(), _channelList.end(), [&allOn,&allRamping](hvChannel *ptrChannel){
                        ptrChannel->printVoltageMeasured();
                        ptrChannel->printCurrentMeasured();
                    } );
                break;
            }
        }
        
        // wait 500 milliseconds. Do not need super fast updates to printed values
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    if(timeout < 1){
        std::cout << "TIMEOUT: did not ramp up or down before timeout."
                  << std::endl;
    }
 
}

bool hvFadcManager::H_CheckIsConnectionGood(){
    // wrapper around the hvModule function which returns whether the connection
    // to the module is good
    return HV_module->CheckIsConnectionGood();
}

int hvFadcManager::H_CheckHVModuleIsGood(bool verbose){
    // this function is called every checkModuleTimeInterval (from
    // HVSettings.ini) seconds and checks, whether all voltages
    // are good / if any events happened
    // if module tripped / voltages bad, stop Run immediately (return -1)
    // and shut down module (if Trip, is done automatically through 
    // monitoring group)

    // this function can only be called, if _createdFlag == true
    // hvObj
    if (_createdFlag == true){
        
        // variables to store the current EventStatus values
        // these will be compared to the values of the last call
        // of this function

	// TODO: event status variables currently not in use
	//       might be a good idea to implement and check
        // ChEventStatusSTRUCT gridEventStatus    = { 0 };
        // ChEventStatusSTRUCT anodeEventStatus   = { 0 };
        // ChEventStatusSTRUCT cathodeEventStatus = { 0 };

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
    
    }//end if (_createdFlag == true)
    else{
	// this should prevent seg faults
	return -1;
    }
}


void hvFadcManager::sleepModule(int sleepTime, std::string unit){
    // function which calls sleep for this thread for time given by 
    // macro in header
    
    if (sleepTime == 0){
	// if we hand default value of 0, we set the sleepTime
	// to the member variable (which is set to the default given
	// by a macro in the header)
	sleepTime = _sleepTime;
    }
    
    if (unit == "milliseconds"){
	std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    }
    else if (unit == "minutes"){
	std::this_thread::sleep_for(std::chrono::minutes(sleepTime));
    }
    else{
	std::cout << "Time unit currently not supported!" << std::endl;
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

void hvFadcManager::H_DumpErrorLogToFile(int event){
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


void hvFadcManager::ShutDownHFMForTOS(){
    // this function is called upon deleting the hvFadcManager or
    // and thus when shutting down TOS


    // only need to perform shutdown if _hvModInitFlag is true
    // TODO: init flag not the correct way to decide whether shutdown or not
    // instead of using hvObjInitFlag, we can use CheckHVModuleIsGood as 
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
        CheckModuleIsRamping(rampUpFlag);


        // TODO: before we perform DoClear, we should check events!
        
        // perform a DoClear (that is reset every event)
        // and set Kill Enable to 0
        moduleControl.Word = H_GetModuleControl();
        moduleControl.Bit.DoClear = 1;
        moduleControl.Bit.SetKillEnable = 0;
        H_SetModuleControl(moduleControl.Word);
        
        // this should properly shut down the device
        // set init flag to false again
        _hvModInitFlag = false;
    }
    else{
	std::cout << "HV was already shut down / not initialized yet." << std::endl;
    }

}


bool hvFadcManager::ClearAllChannelEventStatus(){
    // this function tries to reset the channel event status
    // of all our three used channels
    // this is done by looping over the channelList and calling the clear event status
    // function for each hvChannel

    bool good;
    good = true;
    // using for each we get each element of channelList (an hvChannel) and call 
    // its clearChannelEventStatus function. By multiplying good (equals to true 
    // before loop) with the result, good will only be true, if all channels return 
    // true
    std::for_each( _channelList.begin(), _channelList.end(), [&good](hvChannel *ptrChannel){
	    good *= ptrChannel->clearChannelEventStatus();
	} );
    if(good == true){
	std::cout << "All channels' ChannelEventStatus successfully reset." 
		  << std::endl;
    }
    else{
	std::cout << "Could not reset ChannelEventStatus of one or more channels" 
		  << std::endl;
    }
    
    return good;
}


void hvFadcManager::H_SetNominalValues(){

    //****************** Set Nominal values **************************

    int timeout;
    // only possible to call this function, if settings were read
    // and hv obj created
    if ((_createdFlag      == true) &&
	(_settingsReadFlag == true)){
        // first set module to stop, so we can set nominal voltages
        // get channel grid
	HV_module->setStopModule(true);

        // now we can set the nominal values
        std::cout.precision(15);
        timeout = 1000;
	bool good = true;
        while(timeout > 0){
	    if(HV_module->isStop() == true){
		good = true;
		// set voltages and currents for all channels
		std::for_each( _channelList.begin(), _channelList.end(), [&good,this](hvChannel *ptrChannel){
			if( ptrChannel->getChannelName() == "grid" ){
			    good *= ptrChannel->setVoltageNominal(this->gridVoltageNominal);
			    good *= ptrChannel->setCurrentNominal(this->gridCurrentNominal);
			}
			else if ( ptrChannel->getChannelName() == "anode" ){
			    good *= ptrChannel->setVoltageNominal(this->anodeVoltageNominal);
			    good *= ptrChannel->setCurrentNominal(this->anodeCurrentNominal);
			}
			else if ( ptrChannel->getChannelName() == "cathode" ){
			    good *= ptrChannel->setVoltageNominal(this->cathodeVoltageNominal);
			    good *= ptrChannel->setCurrentNominal(this->cathodeCurrentNominal);
			}
		    } );
		
		// if good is still true, after this, all channels could be set correctly
		if (good == true){
		    break;
		}
	    }
	    HV_module->sleepModule();
	    timeout--;
        }
        if (timeout == 0){
        	std::cout << "ERROR: Could not set nominal Voltages / Currents" << std::endl;
        	std::cout << "       Module did not enter IsStop == 1         " << std::endl;
        }

        // now revert SetStop back to 0
	HV_module->setStopModule(false);
    }
    else{
	std::cout << "Before calling SetNominalValues() the Settings need to be correctly "
		  << "read from the .ini file and the HV object has to be created.\n"
		  << "Nominal values NOT set." << std::endl;
    }

}


GroupSTRUCT hvFadcManager::H_GetFlexGroup(int group){
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


void hvFadcManager::H_SetFlexGroup(int group, GroupSTRUCT groupObject){
    // call both set flex group functions from the module
    // and write them to the HV module

    H_SetModuleFlexGroupMemberList(group, groupObject.MemberList1.Word);
    
    // we're using Type 1, because in our implementation we're only properly
    // supporting the module with 12 channels
    H_SetModuleFlexGroupType(group, groupObject.Type1.Word);
}


// #############################################################################
// ################## Functions related to voltage scheduler ###################
// #############################################################################

void hvFadcManager::AddElementToVoltageScheduler(float voltage, int time){
    // this is just a convenient wrapper around 
    // the same function receiving a pair of voltage and time
    std::pair<float, int> voltageTimePair;
    voltageTimePair = std::make_pair(voltage, time);
    AddElementToVoltageScheduler(voltageTimePair);
}

void hvFadcManager::AddElementToVoltageScheduler(std::pair<int, int> voltageTimePair){
    // this function adds the voltageTimePair to the _voltageScheduler
    _voltageScheduler.push_back(voltageTimePair);
}

void hvFadcManager::RemoveLastElementFromVoltageScheduler(){
    _voltageScheduler.pop_back();
}

void hvFadcManager::RemoveFirstElementFromVoltageScheduler(){
    _voltageScheduler.pop_front();
}

void hvFadcManager::RunVoltageScheduler(std::string channelIdentifier, std::string groupIdentifier){
    // this function runs the voltage scheduler
    // i.e. we go through the _voltageScheduler list, 
    // take the pair, ramp to the first element and wait
    // the second element minutes, after that, get next element
    // input arguments:
    // std::string channelIdenfitier: channel which is to be used for the scheduler
    // std::string groupIdentifier:   alternatively (defaults to "") the identifier
    //                                to a group can be given, thus we ramp the whole
    //                                group to said voltages

    // using for_each we do not need to define an iterator, which runs over the whole list,
    // but can rather use a lambda function, which takes a pair (element of list) as argument
    std::for_each( _voltageScheduler.begin(), _voltageScheduler.end(), [&channelIdentifier, &groupIdentifier, this] (std::pair<int, int> voltagePair){
	    // voltage pair should now be a pair of the _voltageScheduler
	    float voltage = voltagePair.first;
	    int time    = voltagePair.second;

	    // now check whether we want to ramp group or channel
	    if (groupIdentifier == ""){
		// in this case we want to ramp a single channels
		std::list<hvChannel *>::iterator it;
		//it = _channelList.end();
		for(it = _channelList.begin(); it != _channelList.end(); ++it){
		    hvChannel *channel;
		    channel = *it;
		    if( channel->getChannelName() == channelIdentifier ){
			// if we have the correct channel
			channel->setVoltage(voltage);
			// and set channel to on
			channel->turnOn();
		    }
		}
	    }
	    else if(channelIdentifier == ""){
		// if we want to ramp a whole group, set voltage for all channels of group
		// get group first
		std::list<hvFlexGroup *>::iterator itGroup;
		for(itGroup = _flexGroupList.begin(); itGroup != _flexGroupList.end(); ++itGroup){
		    hvFlexGroup *group;
		    group = *itGroup;
		    if( group->getGroupName() == groupIdentifier ){
			// in this case we have the correct group, set voltage
			group->setVoltageForGroup(voltage);
		    }
		}
	    }
	    
	    // after we have set the voltage, make sure we're ramping
	    CheckModuleIsRamping(true);
	    // and if ramping is done, wait the time we want to wait 
	    sleepModule(time, "minutes");
	} );

    // well I kind of doubt that this works!
}




// #############################################################################
// #################### Read HV settings #######################################
// #############################################################################

void hvFadcManager::ReadHVSettings(){
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
    
    // flag which tracks, if one or more settings was not found in the HVsettings.ini
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
	// defined as macro in hvFadcManager.h
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
    _settingsReadFlag = true;

}



// **************************************************
// *********** HV COMMANDS **************************
// **************************************************


// --- HV Module commands ----------------------------------------------------
int    hvFadcManager::H_GetModuleDeviceClass(void){
    return HV_module->GetModuleDeviceClass();
}

uint32_t hvFadcManager::H_GetModulePlacedChannels(void){
    return HV_module->GetModulePlacedChannels();
}

int    hvFadcManager::H_GetModuleSerialNumber(void){
    return HV_module->GetModuleSerialNumber();
}

uint32_t hvFadcManager::H_GetModuleFirmwareRelease(void){
    return HV_module->GetModuleFirmwareRelease();
}


void   hvFadcManager::H_DoClear(void){
    HV_module->DoClear();
}


void   hvFadcManager::H_SetModuleBaseAddress(uint16_t ba){
    HV_module->SetModuleBaseAddress(ba);
}


float  hvFadcManager::H_GetModuleTemperature(void){
    return HV_module->GetModuleTemperature();
}

float  hvFadcManager::H_GetModuleSupplyP5(void){
    return HV_module->GetModuleSupplyP5();
}

float  hvFadcManager::H_GetModuleSupplyP12(void){
    return HV_module->GetModuleSupplyP12();
}

float  hvFadcManager::H_GetModuleSupplyN12(void){
    return HV_module->GetModuleSupplyN12();
}


uint32_t hvFadcManager::H_GetModuleAdcSamplesPerSecond(void){
    return HV_module->GetModuleAdcSamplesPerSecond();
}

void   hvFadcManager::H_SetModuleAdcSamplesPerSecond(uint32_t sps){
    HV_module->SetModuleAdcSamplesPerSecond(sps);
}

uint32_t hvFadcManager::H_GetModuleDigitalFilter(void){
    return HV_module->GetModuleDigitalFilter();
}

void   hvFadcManager::H_SetModuleDigitalFilter(uint32_t filter){
    HV_module->SetModuleDigitalFilter(filter);
}


float  hvFadcManager::H_GetModuleVoltageLimit(void){
    return HV_module->GetModuleVoltageLimit();
}

float  hvFadcManager::H_GetModuleCurrentLimit(void){
    return HV_module->GetModuleCurrentLimit();
}


uint32_t hvFadcManager::H_GetModuleStatus(void){
    return HV_module->GetModuleStatus();
}

uint32_t hvFadcManager::H_GetModuleEventStatus(void){
    return HV_module->GetModuleEventStatus();
}

void   hvFadcManager::H_SetModuleEventStatus(uint32_t status){
    HV_module->SetModuleEventStatus(status);
}

uint32_t hvFadcManager::H_GetModuleEventChannelStatus(void){
    return HV_module->GetModuleEventChannelStatus();
}

void   hvFadcManager::H_ClearModuleEventChannelStatus(void){
    HV_module->ClearModuleEventChannelStatus();
}

uint32_t hvFadcManager::H_GetModuleEventChannelMask(void){
    return HV_module->GetModuleEventChannelMask();
}

void   hvFadcManager::H_SetModuleEventChannelMask(uint32_t mask){
    HV_module->SetModuleEventChannelMask(mask);
}

uint32_t hvFadcManager::H_GetModuleEventGroupStatus(void){
    return HV_module->GetModuleEventGroupStatus();
}

uint32_t hvFadcManager::hvFadcManager::H_GetModuleEventGroupMask(void){
    return HV_module->GetModuleEventGroupMask();
}

void   hvFadcManager::H_SetModuleEventGroupMask(uint32_t mask){
    HV_module->SetModuleEventGroupMask(mask);
}

void   hvFadcManager::H_ClearModuleEventGroupStatus(void){
    HV_module->ClearModuleEventGroupStatus();
}

uint32_t hvFadcManager::H_GetModuleEventMask(void){
    return HV_module->GetModuleEventMask();
}

void   hvFadcManager::H_SetModuleEventMask(uint32_t mask){
    HV_module->SetModuleEventMask(mask);
}

uint32_t hvFadcManager::H_GetModuleControl(void){
    return HV_module->GetModuleControl();
}

void   hvFadcManager::H_SetModuleControl(uint32_t control){
    HV_module->SetModuleControl(control);
}


float  hvFadcManager::H_GetModuleVoltageRampSpeed(void){
    return HV_module->GetModuleVoltageRampSpeed();
}

void   hvFadcManager::H_SetModuleVoltageRampSpeed(float vramp){
    HV_module->SetModuleVoltageRampSpeed(vramp);
}

float  hvFadcManager::H_GetModuleCurrentRampSpeed(void){
    return HV_module->GetModuleCurrentRampSpeed();
}

void   hvFadcManager::H_SetModuleCurrentRampSpeed(float iramp){
    HV_module->SetModuleCurrentRampSpeed(iramp);
}


bool   hvFadcManager::H_GetModuleKillEnable(void){
    return HV_module->GetModuleKillEnable();
}

void   hvFadcManager::H_SetModuleKillEnable(bool enable){
    HV_module->SetModuleKillEnable(enable);
}


int    hvFadcManager::H_GetModuleChannelNumber(void){
    return HV_module->GetModuleChannelNumber();
}


void   hvFadcManager::H_SetModuleEmergencyOff(void){
    HV_module->SetModuleEmergencyOff();
}

void   hvFadcManager::H_ClearModuleEmergencyOff(void){
    HV_module->ClearModuleEmergencyOff();
}


uint32_t hvFadcManager::H_GetModuleRestartTime(void){
    return HV_module->GetModuleRestartTime();
}

void   hvFadcManager::H_SetModuleRestartTime(uint32_t restartTime){
    HV_module->SetModuleRestartTime(restartTime);
}


void   hvFadcManager::H_SetAllChannelsOn(void){
    HV_module->SetAllChannelsOn();
}

void   hvFadcManager::H_SetAllChannelsOff(void){
    HV_module->SetAllChannelsOff();
}

void   hvFadcManager::H_SetAllChannelsVoltageSet(float vset){
    HV_module->SetAllChannelsVoltageSet(vset);
}

void   hvFadcManager::H_SetAllChannelsCurrentSet(float iset){
    HV_module->SetAllChannelsCurrentSet(iset);
}


// --- HV Channel commands ---------------------------------------------------
uint32_t       hvFadcManager::H_GetChannelStatus(int channel){
    return HV_module->GetChannelStatus(channel);
}

ChStatusSTRUCT hvFadcManager::H_GetChannelStatusStruct(int channel){
    // this function is equivalent to the above, but it
    // creates a ChStatusSTRUCT, which is returned instead
    ChStatusSTRUCT tempChStruct;
    tempChStruct.Word = H_GetChannelStatus(channel);
    
    return tempChStruct;
}

uint32_t hvFadcManager::H_GetChannelEventStatus(int channel){
    return HV_module->GetChannelEventStatus(channel);
}

void   hvFadcManager::H_SetChannelEventStatus(int channel, uint32_t status){
    HV_module->SetChannelEventStatus(channel, status);
}

uint32_t hvFadcManager::H_GetChannelEventMask(int channel){
    return HV_module->GetChannelEventMask(channel);
}

void   hvFadcManager::H_SetChannelEventMask(int channel, uint32_t mask){
    HV_module->SetChannelEventMask(channel, mask);
}

uint32_t hvFadcManager::H_GetChannelControl(int channel){
    return HV_module->GetChannelControl(channel);
}

void   hvFadcManager::H_SetChannelControl(int channel, uint32_t control){
    HV_module->SetChannelControl(channel, control);
}

void hvFadcManager::H_SetChannelEmergencyOff(int channel){
    HV_module->SetChannelEmergencyOff(channel);
}

void   hvFadcManager::H_ClearChannelEmergencyOff(int channel){
    HV_module->ClearChannelEmergencyOff(channel);
}


float  hvFadcManager::H_GetChannelVoltageNominal(int channel){
    return HV_module->GetChannelVoltageNominal(channel);
}

float  hvFadcManager::H_GetChannelCurrentNominal(int channel){
    return HV_module->GetChannelCurrentNominal(channel);
}


float  hvFadcManager::H_GetChannelVoltageSet(int channel){
    return HV_module->GetChannelVoltageSet(channel);
}

void   hvFadcManager::H_SetChannelVoltageSet(int channel, float vset){
    HV_module->SetChannelVoltageSet(channel, vset);
}

float  hvFadcManager::H_GetChannelVoltageMeasure(int channel){
    return HV_module->GetChannelVoltageMeasure(channel);
}

float  hvFadcManager::H_GetChannelCurrentSet(int channel){
    return HV_module->GetChannelCurrentSet(channel);
}

void   hvFadcManager::H_SetChannelCurrentSet(int channel, float iset){
    HV_module->SetChannelCurrentSet(channel, iset);
}

float  hvFadcManager::H_GetChannelCurrentMeasure(int channel){
    return HV_module->GetChannelCurrentMeasure(channel);
}


float  hvFadcManager::H_GetChannelVoltageIlkMaxSet(int channel){
    return HV_module->GetChannelVoltageIlkMaxSet(channel);
}

void   hvFadcManager::H_SetChannelVoltageIlkMaxSet(int channel, float ilkmax){
    HV_module->SetChannelVoltageIlkMaxSet(channel, ilkmax);
}

float  hvFadcManager::H_GetChannelVoltageIlkMinSet(int channel){
    return HV_module->GetChannelVoltageIlkMinSet(channel);
}

void   hvFadcManager::H_SetChannelVoltageIlkMinSet(int channel, float ilkmin){
    HV_module->SetChannelVoltageIlkMinSet(channel, ilkmin);
}


float  hvFadcManager::H_GetChannelCurrentIlkMaxSet(int channel){
    return HV_module->GetChannelCurrentIlkMaxSet(channel);
}

void   hvFadcManager::H_SetChannelCurrentIlkMaxSet(int channel, float ilkmax){
    HV_module->SetChannelCurrentIlkMaxSet(channel, ilkmax);
}

float  hvFadcManager::H_GetChannelCurrentIlkMinSet(int channel){
    return HV_module->GetChannelCurrentIlkMinSet(channel);
}

void   hvFadcManager::H_SetChannelCurrentIlkMinSet(int channel, float ilkmin){
    HV_module->SetChannelCurrentIlkMinSet(channel, ilkmin);
}


void   hvFadcManager::H_SetChannelOn(int channel){
    HV_module->SetChannelOn(channel);
}

void   hvFadcManager::H_SetChannelOff(int channel){
    HV_module->SetChannelOff(channel);
}


// --- HV Convenience commands -----------------------------------------------
QString hvFadcManager::H_FormatVoltage(double v){
    return HV_module->FormatVoltage(v);
}

QString hvFadcManager::H_FormatVoltageUnit(double v){
    return HV_module->FormatVoltageUnit(v);
}

QString hvFadcManager::H_GetVoltageUnit(void){
    return HV_module->GetVoltageUnit();
}


QString hvFadcManager::H_FormatCurrent(double i){
    return HV_module->FormatCurrent(i);
}

QString hvFadcManager::H_FormatCurrentUnit(double i){
    return HV_module->FormatCurrentUnit(i);
}

QString hvFadcManager::H_GetCurrentUnit(void){
    return HV_module->GetCurrentUnit();
}


// --- HV Interlock Out control / status commands ----------------------------
uint32_t hvFadcManager::H_GetModuleIlkOutStatus(void){
    return HV_module->GetModuleIlkOutStatus();
}

uint32_t hvFadcManager::H_GetModuleIlkOutControl(void){
    return HV_module->GetModuleIlkOutControl();
}

void   hvFadcManager::H_SetModuleIlkOutControl(uint32_t control){
    HV_module->SetModuleIlkOutControl(control);
}

uint32_t hvFadcManager::H_GetModuleIlkOutCount(void){
    return HV_module->GetModuleIlkOutCount();
}

uint32_t hvFadcManager::H_GetModuleIlkOutLastTrigger(void){
    return HV_module->GetModuleIlkOutLastTrigger();
}

uint32_t hvFadcManager::H_GetModuleIlkOutChnActualActive(void){
    return HV_module->GetModuleIlkOutChnActualActive();
}

uint32_t hvFadcManager::H_GetModuleIlkOutChnEverTriggered(void){
    return HV_module->GetModuleIlkOutChnEverTriggered();
}


// --- HV Variable Groups ----------------------------------------------------
uint32_t hvFadcManager::H_GetModuleFlexGroupMemberList(int group){
    return HV_module->GetModuleFlexGroupMemberList(group);
}

void   hvFadcManager::H_SetModuleFlexGroupMemberList(int group, uint32_t member){
    HV_module->SetModuleFlexGroupMemberList(group, member);
}

uint32_t hvFadcManager::H_GetModuleFlexGroupType(int group){
    return HV_module->GetModuleFlexGroupType(group);
}

void   hvFadcManager::H_SetModuleFlexGroupType(int group, uint32_t type){
    HV_module->SetModuleFlexGroupType(group, type);
}


float  hvFadcManager::H_GetChannelVoltageHardwareNominal(int channel){
    return HV_module->GetChannelVoltageHardwareNominal(channel);
}

void   hvFadcManager::H_SetChannelVoltageNominal(int channel, float maxset){
    HV_module->SetChannelVoltageNominal(channel, maxset);
}

float  hvFadcManager::H_GetChannelCurrentHardwareNominal(int channel){
    return HV_module->GetChannelCurrentHardwareNominal(channel);
}

void   hvFadcManager::H_SetChannelCurrentNominal(int channel, float maxset){
    HV_module->SetChannelCurrentNominal(channel, maxset);
}


// --- HV Special Control ----------------------------------------------------
void   hvFadcManager::H_SetModuleSpecialControlCommand(uint32_t command){
    HV_module->SetModuleSpecialControlCommand(command);
}

uint32_t hvFadcManager::H_GetModuleSpecialControlCommand(void){
    return HV_module->GetModuleSpecialControlCommand();
}

uint32_t hvFadcManager::H_GetModuleSpecialControlStatus(void){
    return HV_module->GetModuleSpecialControlStatus();
}

void   hvFadcManager::H_SendHexLine(QByteArray record){
    HV_module->SendHexLine(record);
}

void   hvFadcManager::H_ProgramModuleBaseAddress(uint16_t address){
    HV_module->ProgramModuleBaseAddress(address);
}

uint16_t hvFadcManager::H_VerifyModuleBaseAddress(void){
    return HV_module->VerifyModuleBaseAddress();
}




//**************************************************
//*************** FADC functions *******************
//**************************************************

// General FADC functions
void         hvFadcManager::F_Reset() throw(){
    FADC_module->reset();
}

void         hvFadcManager::F_StartAcquisition() throw(){
    FADC_module->startAcquisition();
}

void         hvFadcManager::F_SetFrequency( const unsigned short& frequency ) throw(){
    FADC_module->setFrequency( frequency );
}

unsigned int hvFadcManager::F_GetFrequency() throw(){
    return FADC_module->getFrequency();
}

void hvFadcManager::F_SetReadMode( const unsigned short& mode ) throw(){
    FADC_module->setReadMode( mode );
}

unsigned int hvFadcManager::F_GetReadMode() throw(){
    return FADC_module->getReadMode();
}

unsigned int hvFadcManager::F_GetFPGAVersion() throw(){
    return FADC_module->getFPGAVersion();
}

 /***********  hvFadcManager::F_Trigger settings  ***********/
void         hvFadcManager::F_SetTriggerThresholdDACAll(const unsigned int threshold) throw(){
    FADC_module->setTriggerThresholdDACAll(threshold);
}

unsigned int hvFadcManager::F_GetTriggerThresholdDACAll() throw(){
    return FADC_module->getTriggerThresholdDACAll();
}

void         hvFadcManager::F_SetTriggerThresholdDACPerChannel(const unsigned short chNb, const unsigned int threshold) throw(){
    FADC_module->setTriggerThresholdDACPerChannel(chNb, threshold);
}

unsigned int hvFadcManager::F_GetTriggerThresholdDACPerChannel(const unsigned short chNb) throw(){
    return FADC_module->getTriggerThresholdDACPerChannel(chNb);
}

void         hvFadcManager::F_LoadTriggerThresholdDAC() throw(){
    FADC_module->loadTriggerThresholdDAC();
}

unsigned int hvFadcManager::F_GetTriggerRecord() throw(){
    return FADC_module->getTriggerRecord();
}

void         hvFadcManager::F_SendSoftwareTrigger() throw(){
    FADC_module->sendSoftwareTrigger();
}

void         hvFadcManager::F_SetPretrig( const unsigned short& bits ) throw(){
    FADC_module->setPretrig( bits );
}

unsigned short hvFadcManager::F_GetPretrig() throw(){
    return FADC_module->getPretrig();
}

void         hvFadcManager::F_SetPosttrig( const unsigned short& bits ) throw(){
    FADC_module->setPosttrig( bits );
}

unsigned int hvFadcManager::F_GetPosttrig() throw(){
    return FADC_module->getPosttrig();
}

void         hvFadcManager::F_SetTriggerType( const unsigned short& type ) throw(){
    FADC_module->setTriggerType( type );
}

unsigned short hvFadcManager::F_GetTriggerType() throw(){
    return FADC_module->getTriggerType();
}

void         hvFadcManager::F_SetTriggerChannelSource( const unsigned short& mask ) throw(){
    FADC_module->setTriggerChannelSource( mask );
}

unsigned short hvFadcManager::F_GetTriggerChannelSource() throw(){
    return FADC_module->getTriggerChannelSource();
}

/***********  FADC Readout settings  ***********/
unsigned short hvFadcManager::F_GetModeRegister() throw(){
    return FADC_module->getModeRegister();
}

void           hvFadcManager::F_SetModeRegister(const unsigned short& mode) throw(){
    FADC_module->setModeRegister(mode);
}

void           hvFadcManager::F_SetColToRead( const unsigned short& col ) throw(){
    FADC_module->setColToRead( col );
}

unsigned short hvFadcManager::F_GetColToRead() throw(){
    return FADC_module->getColToRead();
}

void           hvFadcManager::F_SetChannelMask( const unsigned short& mask ) throw(){
    FADC_module->setChannelMask( mask );
}

unsigned short hvFadcManager::F_GetChannelMask() throw(){
    return FADC_module->getChannelMask();
}

void           hvFadcManager::F_SetNbOfChannels(const unsigned short& nbOfChannels) throw(){
    FADC_module->setNbOfChannels(nbOfChannels);
}

unsigned short hvFadcManager::F_GetNbOfChannels() throw(){
    return FADC_module->getNbOfChannels();
}

void           hvFadcManager::F_SetPostStopLatency( const unsigned short& latency ) throw(){
    FADC_module->setPostStopLatency( latency );
}

unsigned short hvFadcManager::F_GetPostStopLatency() throw(){
    return FADC_module->getPostStopLatency();
}

void           hvFadcManager::F_SetPostLatencyPretrig( const unsigned short& latency ) throw(){
    FADC_module->setPostLatencyPretrig( latency );
}

unsigned short hvFadcManager::F_GetPostLatencyPretrig() throw(){
    return FADC_module->getPostLatencyPretrig();
}

void           hvFadcManager::F_SetRAMAddress( const unsigned short& add ) throw(){
    FADC_module->setRAMAddress( add );
}

unsigned short hvFadcManager::F_GetRAMAddress() throw(){
    return FADC_module->getRAMAddress();
}

std::vector< int > hvFadcManager::F_GetAllData( const unsigned short& nbChannels) throw(){
    return FADC_module->getAllData( nbChannels);
}

std::vector< int > hvFadcManager::F_GetAllDataBlockMode() throw(){
    return FADC_module->getAllDataBlockMode();
}

int            hvFadcManager::F_GetDataAt( const unsigned short& add ) throw(){
    return FADC_module->getDataAt( add );
}

/***********  FADC Interrupt settings  ***********/
void           hvFadcManager::F_ReleaseInterrupt() throw(){
    FADC_module->releaseInterrupt();
}

bool           hvFadcManager::F_ReadDeviceInterrupt() throw(){
    return FADC_module->readDeviceInterrupt();
}

bool           hvFadcManager::F_ReadInterrupt() throw(){
    return FADC_module->readInterrupt();
}
