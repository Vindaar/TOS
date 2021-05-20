#include "hvFadcManager.hpp"
//#include <QDir>
//#include <QSettings>
//#include <QCoreApplication>
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


void BackgroundTempLoop(hvFadcManager *hfm, std::atomic_bool &loop_continue, std::atomic_bool &hvModInitFlag){
    /* this function is the safety interlock function of the detector. The moment
       the HFM is activated, this function is started in a separate thread. It
       reads the temperature all the time and checks if it is within safe bounds.
       inputs:
       hvFadcManager *hfm: allows access to check temps fn and dump error log fn
       std::atomic_bool &loop_continue: reference to bool variable, which can
       stop this thread, once TOS is to be shut down
       std::atomic_bool &hvModInitFlag: reference to the flag, which shows whether
       the hvFadcManager was initalized (HV ramped!)

       If not, it will:
           if HV is on:
	       print WARNINGS
	       shut down HV
           if HV is off:
	       print WARNINGs

       If in safe bounds:
           be happy :)
       NOTE: warnings will also be printed to stdout, to make sure user becomes
       aware of the problem! At the same time written to temp log file.
       (if HV was on, HV error dump file will be created too!)
       NOTE2: this function is explicitly NOT part of hvFadcManager, because handing
       a non-static member function to a separate thread is not allowed.
       However, this function only needs to know about some hvFadcManager functions
       which are non-vital (check temps and dump an error log; except that Shutdown
       functino I suppose....). Not exactly nice, but necessary atm.
       TODO: find nicer way to handle this!
       Thus, a pointer of the hvFadcManager object calling this function is
       handed (via this).
    */

    std::string path_name = "./log";
    AtomicTemps temps;
    // set atomic temps to 10 first (to be in the safety bounds at the beginning)
    temps.first = 10;
    temps.second = 10;
    // need another thread, which runs the actual temp log, so that this (already
    // separate thread from the main TOS thread) can perform the actual temperature
    // checks!
    // loop_continue variable is used to stop the thread in case TOS is shut down
    std::thread temp_loop(init_and_log_temp, std::ref(loop_continue), path_name, std::ref(temps));

    bool tempsGood = false;
    //int hvGood = -1;

    // loop and check temps every 500ms
    while(loop_continue == true){
	if(hvModInitFlag == true){
	    // HV should be activated! get HV and temps status
	    //hvGood = hfm->H_CheckHVModuleIsGood(true);
	    // now set the current temperature
	    hfm->SetCurrentTemps(temps);
	    // and check if it's good
	    tempsGood = hfm->CheckIfTempsGood();
	    if(tempsGood == false){
		// in this case ramp down HV immediately
		std::cout << "WARNING: Temperatures are out of safety bounds! \n"
			  << "Temp IMB    = " << temps.first  << "\n"
			  << "Temp Septem = " << temps.second << "\n"
			  << "Ramp down of HV will begin immediately!\n"
			  << "See $TOS/log/HV_error.log for further details."
			  << std::endl;
		// write event number -1 to indicate problem happened outside of run
		hfm->H_DumpErrorLogToFile(-1);
		// TODO: exchange this by an actual function dedicated to
		// ramping down the channels using the groups?
		hfm->ShutDownHFMForTOS();
	    }
	}
	else if(hvModInitFlag == false){
	    // in this case still check temperature
	    // now set the current temperature
	    hfm->SetCurrentTemps(temps);
	    // and check if it's good
	    tempsGood = hfm->CheckIfTempsGood();
	    if(tempsGood == false){
		std::cout << "WARNING: Temperatures are out of safety bounds! "
			  << "Temp IMB    = " << temps.first  << "\n"
			  << "Temp Septem = " << temps.second << "\n"
			  << "HV currently not ramped. Take care of this NOW!"
			  << "See $TOS/log/temp_log.txt for further details."
			  << std::endl;
	    }
	}
	// after one check wait a short time (half a second seems fine, temperatures
	// are only updated every 5 seconds anyway)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    temp_loop.join();
}


// either create object using addresses for HV
hvFadcManager::hvFadcManager(std::string iniFilePath):
    _createdFlag(false),
    _settingsReadFlag(false),
    _hvModInitFlag(false),
    _hvFadcInitFlag(false),
    _fadcTriggerInLastFrame(0),
    _loop_stop(false),
    _scint1_counter(0),
    _scint2_counter(0)
{
    // upon initialisation of hvFadcManager, we need to create the
    // vmemodule (HV) instance using the base Addresses

    // set the path of the ini file to the member variable
    //iniFile = QString::fromStdString(iniFilePath);
    iniFile = iniFilePath;

    // first read HV settings from ini file
    ReadHFMConfig();

    // create the basic event mask we use for all our channels
    // ChannelEventMask: set all three of our channel masks to the following pattern:
    //                   MaskEventTrip = 1
    //                   MaskEventEndOfRamp = 1
    //                   MaskEventVoltageLimit = 1
    //                   MaskEventCurrentLimit = 1
    //                   MaskEventEmergency = 1
    //                   rest to 0.
    // done by creating set of strings corresponding to options
    std::set<std::string> _eventMaskSet = {"CurrentTrip",
                                          "EndOfRamping",
                                          "VoltageLimit",
                                          "CurrentLimit",
                                          "Emergency"};

    _sleepAcqTime     = 0;
    _sleepTriggerTime = 0;

    // set gridEventStatusLastIter, anodeEventStatusLastIter, cathodeEventStatusLastIter
    // TODO: check if this is still doing what we want :)
    gridEventStatusLastIter = { };
    anodeEventStatusLastIter = { };
    cathodeEventStatusLastIter = { };

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
    if (good_connection == false){
        std::cout << "hvFadcManager() Error: could not establish connection to HV."
		  << std::endl;
    }

    // set the object created flag to true
    _createdFlag = true;

    // now start separate thread, which checks for temperatures. Runs until TOS is shut down
    // (or at least hvFadcManager is deactivated again)
    std::thread *loop_thread;
    // set variable, which stops the separate thread when this object is deleted
    _safety_temp_loop_continue = true;
    loop_thread = new std::thread(BackgroundTempLoop, this, std::ref(_safety_temp_loop_continue), std::ref(_hvModInitFlag));

    // set the thread pointer owned by this object to the thread we just created
    // such that we can join the thread safely before deleting this object
    _temp_safety_loop = loop_thread;

    // finally set the FADC settings on the device (so that FADC readout is
    // available immediately)
        // before we start ramping up the HV modules, first set FADC settings
    std::cout << "Setting FADC settings" << std::endl;
    SetFadcSettings();

}

hvFadcManager::~hvFadcManager() {

    // Before we delete the HV module,
    // we should properly shut both of them down

    // TODO: check, if voltages already ramped down?
    // yes, this

    // call shut down function, if not already shut down
    //int toShutdown = 0;
    //toShutdown = H_CheckHVModuleIsGood(false);
    bool toShutdown = false;
    toShutdown = CheckToShutdown();
    if (toShutdown == true){
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

    // TODO::
    // need to delete channels and groups
    // run over group list, call delete for group
    // run over channel list, call delete for channel
    // - write function to PRINT ALL GROUPS
    // - write function to PRINT ALL CHANNELS
    //                     EVENT STATUS
    //                     ETC

    // stop the temperature check thread by setting the
    // loop_continue to false
    _safety_temp_loop_continue = false;
    // and safely join it
    _temp_safety_loop->join();
    // before deleting it
    delete(_temp_safety_loop);

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
    // and sets the group on the module
    // channels.
    // input is:
    // hvFlexGroup group: a flex group object to be added

    bool good;

    // set group on module
    good = ptrGroup->SetGroupOnModule();
    if (good == true){
        // successfully set on module
        // only add to list in this case!
        _flexGroupList.push_back(ptrGroup);
    }
    // else{
    //  // could not be set on module
    // }
    // and add to group list
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
    while( (timeout > 0) &&
           (_moduleEventGroupMask == moduleEventGroupMask) ){
        // first set module event group mask
        H_SetModuleEventGroupMask(static_cast<uint32_t> (moduleEventGroupMask));
        // now wait default time
        HV_module->sleepModule();
        // and update module (which updates _moduleEventGroupMask
        HV_module->updateModule();
        _moduleEventGroupMask = HV_module->GetModuleEventGroupMask();
        timeout--;
    }
    if (timeout < 1){
        std::cout << "TIMEOUT: could not set module event group mask!" << std::endl;
        std::cout << "returning false" << std::endl;
        good = false;
    }
    else{
        std::cout << "timeout in setModuleEventGroupMask " << timeout << std::endl;
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
            good = good && ptrChannel->turnOn();
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
        //              channelNumbers.end(),
        //              chStatusVector.begin(),
        //              [this](int channel){
        //                  return this->H_GetChannelStatusStruct(channel);
        //              } );
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

void hvFadcManager::InitHVForTOS(){
    std::cout << "Entering Init HV" << std::endl;

    // First we read the Settings file
    //     done in ReadHFMConfig() called in creator
    // in case _createdFlag is false
    // we use the read settings to create the objects properly


    // define variables

    // TODO: channelControl and moduleControl currently not in use
    //       might be a good idea to implement and check them
    // ModuleStatusSTRUCT  moduleStatus = { 0 };
    // ModuleControlSTRUCT moduleControl = { 0 };
    // ChControlSTRUCT     channelControl = { 0 };

    // use the read settings (ReadHFMConfig() called in creator)
    // to create the objects, if not yet created
    // check if HV module has been created
    if (_createdFlag == false){
        Controller.initController(0);
        HV_module   = new hvModule(&Controller, baseAddress_hv);
        FADC_module = new V1729a_VME(&Controller, sAddress_fadc);
        // create FADC high level functions
        FADC_Functions = new HighLevelFunction_VME(FADC_module);
        // and reset FADC + set settings on device
        FADC_module->reset();
	SetFadcSettings();

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

    // define lists which will store the members of flex groups
    std::list<hvChannel *> anodeGridMembers;
    std::list<hvChannel *> monitorTripMembers;
    std::list<hvChannel *> rampingMembers;

    // now simply run over dummyChannelVector to create all channels defined in the ini file
    // and add correct channels to groups
    BOOST_FOREACH( dummyHvChannel channel, _dummyChannelVector ){
        bool good = false;
        hvChannel *channelPtr;
        channelPtr = new hvChannel(HV_module, channel.name, channel.number);
        AddChannel(channelPtr);

        // now set channel event mask, voltages and currents
        good = channelPtr->setVoltage(channel.voltageSet);
	channelPtr->setVoltageBound(channel.voltageBound);
        good = channelPtr->setCurrent(channel.currentSet);
        if (good == false) return;

        // only add grid and anode to anode Grid group
        if( (channel.name == "grid") ||
            (channel.name == "anode") ||
            (channel.name == "Ring1") ){
            anodeGridMembers.push_back(channelPtr);
        }
        if (channel.name == "grid"){
            anodeGridGroupMasterChannel = channel.number;
        }
        // but add every channel to trip group and ramping group
        if (channel.name != "szintillator"){
            monitorTripMembers.push_back(channelPtr);
        }
        rampingMembers.push_back(channelPtr);
    }

    // create flex groups
    hvSetGroup *anodeGridSetOnGroup;
    std::cout << "some stuff\n"
              << anodeGridGroupMasterChannel << std::endl;
    anodeGridSetOnGroup = new hvSetGroup(HV_module,
                                         "anodeGridSetOnGroup",
                                         anodeGridMembers,
                                         anodeGridGroupNumber,
                                         anodeGridGroupMasterChannel);

    hvMonitorGroup *monitorTripGroup;
    monitorTripGroup = new hvMonitorGroup(HV_module,
                                          "monitorTripGroup",
                                          monitorTripMembers,
                                          monitorTripGroupNumber);


    hvStatusGroup *rampingGroup;
    rampingGroup = new hvStatusGroup(HV_module,
                                     "rampingGroup",
                                     rampingMembers,
                                     rampingGroupNumber);

    // now with our fancy groups, we can just set the options we would like to
    // set
    // define master channel of SetGroup
    // define anode as master Channel (in constructor); include both in .ini file
    anodeGridSetOnGroup->setMode(DEFAULT_ANODE_GRID_GROUP_MODE);
    anodeGridSetOnGroup->setControl("set_on");

    // monitor group, need to set monitoring to tripping
    monitorTripGroup->setMonitor("is_tripped");
    // set Mode to 1. That way, the action of the group is called,
    // if one of the group channels reports a 1 for the bit, that is being
    // checked. In our case:
    // Channel.Bit.CurrentTrip == 1 for one channel means group shut down
    monitorTripGroup->setMode(1);
    monitorTripGroup->setAction("shut_group");

    rampingGroup->setMonitor("is_ramping");

    // now add the three groups to the handler
    AddFlexGroup(anodeGridSetOnGroup);
    AddFlexGroup(monitorTripGroup);
    AddFlexGroup(rampingGroup);

    // call functino to set module event group mask for all active channels we're
    // using and make sure it works. Function uses _channelMap to set mask
    bool good = true;
    good = SetModuleEventGroupMask();
    if (good == false) return;

    // now clear the channel status as far as necessary
    good = ClearAllChannelEventStatus();
    if (good != true){
        std::cout << "Could not reset Event status.\n"
                  << "Probably will not start ramping."
                  << std::endl;
    }

    // set the module ramp speed
    std::cout << "Setting module voltage ramp speed to " << moduleVoltageRampSpeed << std::endl;
    H_SetModuleVoltageRampSpeed(moduleVoltageRampSpeed);

    // and clear module event status
    good = HV_module->clearModuleEventStatusAndCheck();
    if (good != true){
        std::cout << "Could not reset module event status.\n"
                  << "Probably will not start ramping."
                  << std::endl;
    }

    // bool variable used to check whether channels already ramped up
    // from a previous usage of TOS
    bool alreadyInBounds = false;
    alreadyInBounds = CheckAllChannelsInVoltageBound();
    std::cout << "already in bounds? " << alreadyInBounds << std::endl;

    std::string inputRamp;
    std::set<std::string> allowedInputs = {"Y", "y", "N", "n"};
    std::cout << "Do you wish to ramp up the channels now? (Y / n)" << std::endl;
    inputRamp = getUserInputNonNumericalDefault("> ", &allowedInputs);
    if ( (inputRamp == "") ||
         (inputRamp == "Y") ||
         (inputRamp == "y") ){
        RampChannels(alreadyInBounds);
    }

    _hvModInitFlag = true;

    // and all should be good :)
}

void hvFadcManager::RampChannels(bool alreadyInBounds){
    // this function starts the ramp up of the channels
    // now set all channels to on

    // performs a slow ramp up of the module
    // given a set of voltages, it ramps them slowly (not by changing the
    // ramping speed, but rather by adapting the voltages so that they are
    // kept within a certain potential difference). The ratio of the different
    // channels to be ramped is calculated up front and this is used to
    // always keep the channels at the same ratio (to keep the gradient flat)

    if (!alreadyInBounds){
	// get max voltage so that we can calculate necessary ratio of all voltages
	// get the voltage of the
	auto maxChannel = std::max_element(_channelList.begin(),
						 _channelList.end());
	const float maxVoltage = (*maxChannel)->getVoltage();

	// create a map of the current voltages (the targets we will aim for) and
	// the scaling factors needed for each channel based on target voltage and
	// max voltage
	std::map<int, float> scalingRatios;
	std::for_each(
	    _channelList.begin(),
	    _channelList.end(),
	    [&scalingRatios, maxVoltage](hvChannel *ptrChannel){
		const int number = ptrChannel->getChannelNumber();
		const float voltage = ptrChannel->getVoltage();
		const float ratio = float(voltage) / float(maxVoltage);
		scalingRatios.insert(std::make_pair(number, ratio));
	    } );

	// number of steps we use to ramp the voltage
	// TODO: move this to the ini file!
	const int nSteps = 150;

	// set the trip current to a "ramping current" by multiplying it by 10
	const int currentMultiplier = 10;
	std::for_each(
	    _channelList.begin(),
	    _channelList.end(),
	    [currentMultiplier](hvChannel *ptrChannel){
		const float current = ptrChannel->getCurrent();
		ptrChannel->setCurrent(current * currentMultiplier);
	    } );


	for(int i = int(nSteps / 10); i <= nSteps; i++){
	    // set the voltages of all channels to the current iteration
	    // wait for all channels to be in voltage bound
	    // once done, continue
	    std::for_each(
		_channelList.begin(),
		_channelList.end(),
		[&scalingRatios, i, nSteps, maxVoltage](hvChannel *ptrChannel){
		    const int chNumber = ptrChannel->getChannelNumber();
		    const float voltage = maxVoltage * scalingRatios[chNumber] * (i / float(nSteps));
		    ptrChannel->setVoltage(voltage, false);
		} );

	    if (i == int(nSteps / 10)){
		// in the first iteration set all channels to on
		bool good = false;
		good = SetAllChannelsOn();
		if (good == false) return;
	    }

	    // call check hv module to print current voltages
	    H_CheckHVModuleIsGood(true);

	    while(!CheckAllChannelsRamped(false)){
		H_CheckHVModuleIsGood(true);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		continue;
	    }
	    // once it is done, we continue to the next voltage
	}

	// set the trip current back to a "operating current"
	std::for_each(
	    _channelList.begin(),
	    _channelList.end(),
	    [currentMultiplier](hvChannel *ptrChannel){
		const float current = ptrChannel->getCurrent();
		ptrChannel->setCurrent(current / currentMultiplier);
	    } );

    }

    // TODO: implement the above in a separate function, which will also be called for
    // the ramp down. This can then be done in a separate thread as well.

    // // create seperate thread, which loops and will be stopped, if we type stop in terminal
    // if (alreadyInBounds == false){
    //     // now check if ramping started
    //     bool rampUpFlag = true;
    //     std::thread loop_thread(&hvFadcManager::CheckModuleIsRamping, this, rampUpFlag);
    //     _loop_stop = true;
    //     //loop_thread.start();
    //     const char *waitingPrompt = "> ";
    //     std::set<std::string> allowedStrings = {"stop", "q"};
    //     std::string input;
    //     input = getUserInputNonNumericalNoDefault(waitingPrompt, &allowedStrings);
    //     if( (input != "stop") ||
    //         (input == "q") ){
    //         std::cout << "setting loop stop to false" << std::endl;
    //         _loop_stop = false;
    //     }
    //     loop_thread.join();
    // }

}


void hvFadcManager::CheckModuleIsRamping(bool rampUpFlag){
    // this is a convenience function, which checks whether the
    // active channels (the ones added to _channelList) are
    // currently ramping

    // we can loop over all channels of _channelList and use the isRamping()
    // function

    int timeout = 100;
    bool doneChecking = false;

    while( (_loop_stop == true) &&
           (timeout > 0) &&
           (doneChecking == false) ){
        // in this case we are ramping up
        // print the current voltage and current
        // and check whether the channels are on and voltage controlled
        bool localControlled = true;
        std::for_each( _channelList.begin(), _channelList.end(), [&localControlled, &rampUpFlag](hvChannel *ptrChannel){
                ptrChannel->printVoltageAndCurrentMeasured();

                if (rampUpFlag == true){
                    // if we ramp up, check for onVoltageControlledRamped
                    localControlled = localControlled &&
			ptrChannel->onVoltageControlledRamped(false);
                }
                else{
                    // if we're ramping down, check for finishedRampingDown
                    localControlled = localControlled &&
			ptrChannel->finishedRampingDown();
                }
            } );

        // now set the allControlled by voltage bool to the local controlled bool
        // thus we leave the while loop on next iteration, if all channels are on and
        // controlled by voltage
        doneChecking = localControlled;
        // wait 1000 milliseconds. Do not need super fast updates to printed values
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        timeout--;
    }
    if(timeout < 1){
        std::cout << "TIMEOUT: did not ramp up or down before timeout."
                  << std::endl;
    }

    std::cout << "timeout in CheckModuleIsRamping " << timeout << std::endl;

}

bool hvFadcManager::CheckAllChannelsRamped(bool printFlag){
    // the function loops over all channels in _channelList and calls
    // the onVoltageControlledRamped function of hvChannel, thus checking
    // whether all channels are turned On, voltage controlled, ended ramping
    // and within voltage bounds
    bool good = true;

    std::for_each(
	_channelList.begin(),
	_channelList.end(),
	[&good, printFlag](hvChannel *ptrChannel){
	    good = good && ptrChannel->onVoltageControlledRamped(printFlag);
	} );

    return good;
}

bool hvFadcManager::CheckAllChannelsInVoltageBound(){
    // this function loops over all channels in _channelList and calls
    // the voltageInBounds function to see whether all channels are
    // within voltage bounds
    bool good = true;

    std::for_each( _channelList.begin(), _channelList.end(), [&good](hvChannel *ptrChannel){
            good = good && ptrChannel->voltageInBounds();
        } );

    return good;
}

bool hvFadcManager::CheckToShutdown(){
    // this function checks whether the module needs to be shutdown
    // upon exit of TOS (if the user wants to)
    // done by checking the channel status of the channels in _channelList
    // if non zero, we need to shut down

    bool toShutdown = false;
    std::for_each( _channelList.begin(), _channelList.end(), [&toShutdown](hvChannel *ptrChannel){
            // we add to toShutdown, since if any channel is on, we need to shut it down
            toShutdown += ptrChannel->channelStatusNonZero();
        } );

    std::cout << "toShutdown " << toShutdown << std::endl;
    return toShutdown;
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

	// first check event status of the HV module
        bool good;
        good = HV_module->isEventStatusGood();
        if (good == false){
            if (verbose == true){
                std::cout << "One event of the module was triggered. "
                          << "Printing module event status"
                          << std::endl;
                HV_module->printEventStatus();
            }
            return -1;
        }
        // --------------------

        // this function basically only does the following:
        // reads the current voltages and currents
        // compares them to the values we set in the beginning
        // additionally check whether module is in IsControlledVoltage,
        // which should be set, if our module is at the set level
        // and isOn is set

        std::for_each( _channelList.begin(), _channelList.end(), [&good](hvChannel *ptrChannel){
                good = good && ptrChannel->onVoltageControlledRamped();
            } );
        if (good == false){
            std::cout << "One of the channels is not voltage controlled." << std::endl;
            return -1;
        }
        else if (good == true){
            // in this case the module is in good shape and all channels are ramped
            return 0;
        }
    }//end if (_createdFlag == true)
    else{
        // this should prevent seg faults
        return -1;
    }

    std::cout << "This should never be reached." << std::endl;
    return -1;

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




// #############################################################################
// ###################### Functions related to printing ########################
// #############################################################################


void hvFadcManager::printAllChannelStatus(){
    // this function runs over the channel list and prints the channel
    // status for each
    std::for_each( _channelList.begin(), _channelList.end(), [](hvChannel *ptrChannel){
            ptrChannel->printChannelStatus();
        } );
}

void hvFadcManager::printAllChannelEventStatus(){
    // this function runs over the channel list and prints the channel
    // status for each
    std::for_each( _channelList.begin(), _channelList.end(), [](hvChannel *ptrChannel){
            ptrChannel->printChannelEventStatus();
        } );
}

void hvFadcManager::printModuleStatus(){
    // print module status
    HV_module->printStatus();
}

void hvFadcManager::printModuleEventStatus(){
    // print module event status
    HV_module->printEventStatus();
}

void hvFadcManager::H_DumpErrorLogToFile(int event){
    /* int event: if function is called from a run, we
                  include the event after which error log is
		  created
		  If function was called from outside event, event number will be
		  set to -1!
       int temp_IMB: temperature of the intermediate board when this function
                  was called. If 0, probably not included (default is 0)
       int temp_septem: temperature of the septemboard.
    */
    int temp_imb    = _currentTemps.first;
    int temp_septem = _currentTemps.second;

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

    logfile << "#### Temperatures ####\n"
	    << "temp_IMB:    " << temp_imb << "\n"
	    << "temp_Septem: " << temp_septem << "\n";

}


void hvFadcManager::ShutDownHFMForTOS(){
    // this function is called upon deleting the hvFadcManager or
    // and thus when shutting down TOS


    // only need to perform shutdown if _hvModInitFlag is true
    // TODO: init flag not the correct way to decide whether shutdown or not
    // instead of using hvObjInitFlag, we can use CheckHVModuleIsGood as
    // a way to decide
    bool toShutdown = 0;
    toShutdown = CheckToShutdown();

    if(toShutdown == true){
        // TODO: rewrite the shutdown function!!!
	std::cout << "Shutdown currently not proplerly implemented. Use isegControl" << std::endl;
	return;

        std::cout << std::endl << "Starting shutdown of HV for TOS" << std::endl << std::endl;

        // TODO: ramp down
        //       check for any Events
        //       perform a doClear
        //       setStatus to 0x0000 (everything off)
        //       set KillEnable == False
        //
        // ModuleStatusSTRUCT  moduleStatus = { 0 };
        ModuleControlSTRUCT moduleControl = { };
        // ChControlSTRUCT     channelControl = { 0 };

        // // ramp down voltages (use anode Grid Group master channel to ramp down group)
        // // and cathode channel
        // channelControl.Word = H_GetChannelControl(anodeGridGroupMasterChannel);
        // channelControl.Bit.setON = 0;
        // H_SetChannelControl(anodeGridGroupNumber, channelControl.Word);

        // channelControl.Word = H_GetChannelControl(cathodeChannelNumber);
        // channelControl.Bit.setON = 0;
        // H_SetChannelControl(cathodeChannelNumber, channelControl.Word);

        // run over all channels and turn them off
        std::for_each( _channelList.begin(), _channelList.end(), [](hvChannel *ptrChannel){
                ptrChannel->turnOff();
            } );

        // check if the module is ramping down
        bool rampUpFlag = false;
        CheckModuleIsRamping(rampUpFlag);


        // TODO: before we perform DoClear, we should check events!

        // perform a DoClear (that is reset every event)
        // and set Kill Enable to 0
        moduleControl.Word = H_GetModuleControl();
        moduleControl.Bit.DoClear = 1;
        // do not disable kill enable
        // moduleControl.Bit.SetKillEnable = 0;
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
            good = good && ptrChannel->clearChannelEventStatus();
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
                            good = good && ptrChannel->setVoltageNominal(
				this->gridVoltageNominal
				);
                            good = good && ptrChannel->setCurrentNominal(
				this->gridCurrentNominal
				);
                        }
                        else if ( ptrChannel->getChannelName() == "anode" ){
                            good = good && ptrChannel->setVoltageNominal(
				this->anodeVoltageNominal
				);
                            good = good && ptrChannel->setCurrentNominal(
				this->anodeCurrentNominal
				);
                        }
                        else if ( ptrChannel->getChannelName() == "cathode" ){
                            good = good && ptrChannel->setVoltageNominal(
				this->cathodeVoltageNominal
				);
                            good = good && ptrChannel->setCurrentNominal(
				this->cathodeCurrentNominal
				);
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
// ################## Functions related to single channels #####################
// #############################################################################

std::set<std::string> hvFadcManager::PrintActiveChannels(){
    // this function 1. prints all active channels (and if they're on or off)
    // and also returns a set of strings, which are allowed to be used for
    // user input
    std::set<std::string> allowedStrings;

    std::cout << "isOn state of currently active channels (0 = off, 1 = on):" << std::endl;
    std::for_each( _channelList.begin(), _channelList.end(), [&allowedStrings](hvChannel *ptrChannel){
            // print channel information regarding on / off state
            ptrChannel->printChannelIsOn();

            // add channel number to allowed strings
            std::string number;
            number = std::to_string(ptrChannel->getChannelNumber());
            allowedStrings.insert(number);
        } );

    return allowedStrings;
}

void hvFadcManager::TurnChannelOnOff(){
    // this function provides a user interface to turn channels on or off by hand

    // first print the current channels of the channelList (with names and numbers)
    // and their current isOn state
    // also create a set of allowed strings for input
    std::set<std::string> allowedStrings;
    allowedStrings = PrintActiveChannels();

    allowedStrings.insert("all");

    std::cout << "Select channel to switch (on <-> off) / type quit: " << std::endl;
    std::cout << "All to switch all channels (careful if some on, some off!)" << std::endl;
    std::string input;
    input = getUserInputNonNumericalNoDefault("> ", &allowedStrings);
    if(input == "quit"){
        return;
    }
    else{
        // run over channelList and switch channel with name 'input'
        std::for_each( _channelList.begin(), _channelList.end(), [&input](hvChannel *ptrChannel){
                // get channel number
                std::string number;
                if (input != "all") {
                    number = std::to_string(ptrChannel->getChannelNumber());
                    // and switch correct channel
                    if(input == number){
                        ptrChannel->switchOnOff();
                    }
                }
                else{
                    ptrChannel->switchOnOff();
                }
            } );
    }

}

bool hvFadcManager::ClearChannelEventStatus(){
    // this function provides a user interface to clear the channel event status
    // of one or more channels
    bool good = false;
    // create a set of allowed strings for input
    std::set<std::string> allowedStrings;
    std::for_each( _channelList.begin(), _channelList.end(), [&allowedStrings](hvChannel *ptrChannel){
            // print channel information
            ptrChannel->printChannelName();

            // add channel number to allowed strings
            std::string number;
            number = std::to_string(ptrChannel->getChannelNumber());
            allowedStrings.insert(number);
        } );

    std::cout << "Select channel for which to clear event status / type quit: " << std::endl;
    std::string input;
    input = getUserInputNonNumericalNoDefault("> ", &allowedStrings);
    if(input == "quit"){
        return good;
    }
    else{
        // run over channelList and switch channel with name 'input'
        std::for_each( _channelList.begin(), _channelList.end(), [&input, &good](hvChannel *ptrChannel){
                // get channel number
                std::string number;
                number = std::to_string(ptrChannel->getChannelNumber());
                // and switch correct channel
                if(input == number){
                    good = ptrChannel->clearChannelEventStatus();
                }
            } );
    }

    return good;
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

bool hvFadcManager::CreateChannel(std::string channelName, int voltage){
    // this function creates a basic channel with a channel name and a
    // voltage target, but a default event mask, current etc.
    bool good = true;

    hvChannel *newChannel;
    // the channel number of the new channel will simply be
    // the size of the channel list (good, since we start counting at 0)
    int channelNumber;
    channelNumber = _channelList.size();
    newChannel = new hvChannel(HV_module, channelName, channelNumber);

    // set channel event mask to our default
    good = newChannel->setChannelEventMask(_eventMaskSet);
    // first set nominal values
    // nominal values can only be set in special mode I believe?
    //good = newChannel->setVoltageNominal(DEFAULT_VOLTAGE_NOMINAL);
    //good = newChannel->setCurrentNominal(DEFAULT_CURRENT_NOMINAL);
    // now set voltage and current
    good = newChannel->setVoltage(voltage);
    newChannel->setVoltageBound(DEFAULT_VOLTAGE_BOUND);
    good = newChannel->setCurrent(DEFAULT_CURRENT_SET);

    // and finally add channel to channel list
    std::cout << "adding channel " << channelName
              << " # " << channelNumber
              << std::endl;
    AddChannel(newChannel);

    return good;
}

void hvFadcManager::RemoveChannelByNumber(int channelNumber){
    // this function removes a channel by its number 'channelNumber'
    // from the channel list (and ramps it down)

    // loop over channel list, check if channel with name channelNumber
    // exists, if so pop it
    std::for_each( _channelList.begin(), _channelList.end(), [&channelNumber, this](hvChannel *ptrChannel){
            int number;
            number = ptrChannel->getChannelNumber();

            if (number == channelNumber){
                // first remove the element from the list
                this->_channelList.remove(ptrChannel);
                // and then delete the channel, thus shutting it down
                ptrChannel->SetRampDownUponDelete(true);
                delete(ptrChannel);
            }
        } );

    // all good
}

void hvFadcManager::RemoveChannelByName(std::string channelName){
    // this function removes the channel 'channelName'
    // from the channel list (and ramps it down)

    // loop over channel list, check if channel with name channelName
    // exists, if so pop it
    std::for_each( _channelList.begin(), _channelList.end(), [&channelName, this](hvChannel *ptrChannel){
            std::string name;
            name = ptrChannel->getChannelName();

            if (name == channelName){
                // first remove the element from the list
                this->_channelList.remove(ptrChannel);
                // and then delete the channel, thus shutting it down
                ptrChannel->SetRampDownUponDelete(true);
                delete(ptrChannel);
            }
        } );

    // all good
}

void hvFadcManager::PrintChannel(int channelNumber){
    // this function calls the basic printing function of the channel
    // with channel number channelNumber
    std::for_each( _channelList.begin(), _channelList.end(), [&channelNumber](hvChannel *ptrChannel){
            int number;
            number = ptrChannel->getChannelNumber();
            if (number == channelNumber){
                // call the printing function
                ptrChannel->printChannel();
            }
        } );

}

void hvFadcManager::SetChannelValue(std::string key, int channelNumber, std::string value){
    // this function sets voltage, current, or nominal values of a specific channel
    std::for_each( _channelList.begin(), _channelList.end(), [&channelNumber, &key, &value](hvChannel *ptrChannel){
            int number;
            number = ptrChannel->getChannelNumber();
            if (number == channelNumber){
                if (key == "voltage"){
                    ptrChannel->setVoltage(std::stoi(value));
                }
                else if (key == "voltageNominal"){
                    ptrChannel->setVoltageNominal(std::stoi(value));
                }
                else if (key == "current"){
                    ptrChannel->setCurrent(std::stod(value));
                }
                else if (key == "currentNominal"){
                    ptrChannel->setCurrent(std::stod(value));
                }
            }
        } );

}


// #############################################################################
// #################### Functions related to FADC ##############################
// #############################################################################

void hvFadcManager::SetFadcSettings(){
    // this function applies the fadc settings, which were read from
    // the HFM_settings.ini

    // before we set any settings, reset FADC
    F_Reset();

    // set the trigger type. typically == 3, bit register, first two bits == 1
    //F_SetTriggerType(fadcTriggerType);
    std::cout << "setting fadc trigger type : " << fadcTriggerType << std::endl;
    FADC_Functions->setTriggerTypeH( fadcTriggerType );
    // set FADC frequency, typically 2 GHz
    //F_SetFrequency(fadcFrequency);
    FADC_Functions->setFrequencyH( fadcFrequency );
    // set FADC post trigger. time which FADC takes data after trigger happened
    //F_SetPosttrig(fadcPosttrig);
    FADC_Functions->setPosttrigH( fadcPosttrig );
    // set FADC pre trigger. Time after acquisition started, which has to pass,
    // before trigger is accepted
    //F_SetPretrig(fadcPretrig);
    FADC_Functions->setPretrigH( fadcPretrig );
    // select the trigger source, i.e. the channel which is allowed to trigger events
    // typically == 1, since bit 0 == 1 from bit register, for channel 0
    //F_SetTriggerChannelSource(fadcChannelSource);
    FADC_Functions->setTriggerChannelSourceH( fadcChannelSource );
    // set the trigger threshold for all channels
    FADC_Functions->setTriggerThresholdRegisterAll( fadcTriggerThresholdRegisterAll );
    //F_PrintSettings();

    // set MODE_REGISTER of FADC (by default we set it to 0b010 in order to enable
    // 14 bit readout. Need this variable to properly set the trigger threshold
    // as a range! Range of TriggerThresholdRegister not from 0 - 4096 but 0 - 16384
    F_SetModeRegister( fadcModeRegister );

    _hvFadcInitFlag = true;

}

void hvFadcManager::StartFadcPedestalRun(){
    // this function performs an FADC pedestal run
    // that means, it will do several FADC data acquisitions
    // without any inputs on the channel (number defined in
    // HFM_settings.ini) with a time also defined in settings
    // output will be written to pedestalRun folder within data folder

    // define a list in which we store the vectors of each pedestal run
    std::list< std::vector<int> > pedestalRunList;
    // fadc has 4 channels. TODO: define globally
    int nChannels = 4;

    for (int i = 0; i < fadcPedestalNumRuns; i++){
        // we start by performing general reset of FADC
        F_Reset();
        // now set up FADC in the same way we're going to use it for data taking
        SetFadcSettings();
        // now start FADC acquisition
        F_StartAcquisition();
        // wait for time read from HFM_settings.ini
        std::this_thread::sleep_for(std::chrono::milliseconds(fadcPedestalRunTime));
        // send software trigger to stop acquisition
        F_SendSoftwareTrigger();
        // and read the data
        std::vector<int> fadcData = F_GetAllData(nChannels);
        // and add data to pedestalRunList
        pedestalRunList.push_front(fadcData);
    }

    // once we're done with all pedestal runs, we can calculate the mean value of each value
    // of the fadcData vectors
    std::vector<int> meanFadcData;
    // create empty vector of same size of 'any' element of pedestalRunList (note: this assumes
    // that each fadcData vector has same size! should be a good assumptions, else somethings
    // very wrong!)
    meanFadcData.resize( pedestalRunList.front().size(), 0);
    std::for_each( pedestalRunList.begin(),
                   pedestalRunList.end(),
                   [&meanFadcData, this](std::vector<int> fadcData){
            for(unsigned int i = 0; i < fadcData.size(); i++){
                meanFadcData[i] += fadcData[i] / this->fadcPedestalNumRuns;
            }
        } );

    // get the fadc parameter map (needed for header of output file)
    std::map<std::string, int> fadcParams;
    fadcParams = GetFadcParameterMap();
    // and set the PedestalRun flag in the map to 1
    fadcParams["PedestalRun"] = 1;

    // create the filename to which we write the data (with pedestalRun flag equal true)
    std::string filename;
    filename = buildFileName("", true, 0);

    // now we can write this vector to some file
    writeFadcData(filename, fadcParams, meanFadcData);
}

std::string hvFadcManager::buildFileName(std::string filePath, bool pedestalRunFlag, int eventNumber){
    // this function builds the filename for the output of the FADC data
    // inputs:
    //     std::string filePath: if this is non zero, filename is appended to filePath
    //     bool pedestalRunFlag: this flag determines whether it's a pedestal calibration run
    //                             true: uses /data/pedestalRuns folder
    // TODO: finish this function as to use it for all cases?

    if( (filePath == "") &&
        (pedestalRunFlag == true) ){
        filePath = "data/pedestalRuns/";
    }
    else if( (filePath == "") &&
             (pedestalRunFlag == false) ){
        filePath = "data/singleFrames/";
    }

    std::stringstream buildFileName;
    std::string fileName = "";

    //FIXME!!
    if (pedestalRunFlag == true){
        // depending on whether it is a pedestal run, the filename will start with...
        buildFileName << "pedestalRun";
    }
    else{
        // ... or ...
        buildFileName << "/data";
    }
    buildFileName << std::setw(6) << std::setfill('0') << eventNumber
                  << ".txt";

    fileName = filePath + buildFileName.str();

    if (pedestalRunFlag == true){
        // in case this is a pedestal run also append the fadc flag
        fileName += "-fadc";
    }

    return fileName;
}


bool hvFadcManager::writeFadcData(std::string filename, std::map<std::string, int> fadcParams, std::vector<int> fadcData){
    // this function is used to write FADC data to the file 'filename'

    // define flag which is used to return whether file was written
    bool good = false;

    // only does anything, if FADC is active!
    if (_hvFadcInitFlag == true){

        // create output file stream
        std::ofstream outFile(filename);


        std::cout << "writing FADC data to " << filename << std::endl;
        // TODO: change local variable here!!!!
        int channels = 4;


        //write data to output file
        if( outFile.is_open() )
        {
        //write petrig/postrig/etc to file
        outFile << "# nb of channels: "  << fadcParams["NumChannels"]  << std::endl;
        outFile << "# channel mask: "    << fadcParams["ChannelMask"]  << std::endl;
        outFile << "# posttrig: "        << fadcParams["PostTrig"]     << std::endl;
        outFile << "# pretrig: "         << fadcParams["PreTrig"]      << std::endl;
        outFile << "# triggerrecord: "   << fadcParams["TriggerRec"]   << std::endl;
        outFile << "# frequency: "       << fadcParams["Frequency"]    << std::endl;
        outFile << "# sampling mode: "   << fadcParams["ModeRegister"] << std::endl;
        outFile << "# pedestal run: "    << fadcParams["PedestalRun"]  << std::endl;
        outFile << "# Data:" << std::endl;


        // TODO: why unsigned int used? problematic, since channels is int.
        // -> changed iData and iVector to int
        //"skip" 'first sample', 'venier' and 'rest baseline' (manual p27)
        std::cout << "number of channels?! " << channels << std::endl;
        for(int iData = 0; iData < 3*channels; iData++)
            outFile << "# " << fadcData[iData] << std::endl;

        //print the actual data to file
        for(int iVector = 3*channels; iVector < 2563*channels; iVector++)
            outFile << fadcData[iVector] << std::endl;

        //"skip the remaining data points
        for(unsigned int iRest = channels * 2563; iRest < fadcData.size(); iRest++)
            outFile << "# " << fadcData.at(iRest) << std::endl;

        good = true;

        }//end of if( outFile.is_open() )
        else{
        // could not write to file
        good = false;
        }

        //close output file
        outFile.close();
    }
    else{
        std::cout << "FADC is not active. Call ActivateHFM and SetFadcSettings before starting readout."
                  << std::endl;
    }

    return good;
}




// #############################################################################
// #################### Read HV settings #######################################
// #############################################################################

void hvFadcManager::ReadHFMConfig(){
    // This function reads the .ini file from the
    // member variable iniFile, which is set in the creator
    // this function is called in the creator

    // // create path for iniFile
    // #ifdef __WIN32__
    // // in case of using windows, we need to call the GetCurrentDirectory function
    // // as an input it receives: DWORD WINAPI GetCurrentDirectory(_In_  DWORD  nBufferLength, _Out_ LPTSTR lpBuffer);
    // // create TCHAR and char string of length MAX_INI_PATH_LENGTH defined in header of this file
    // TCHAR string[MAX_INI_PATH_LENGTH];
    // char  char_string[MAX_INI_PATH_LENGTH];
    // // call windows function with its weird arguments
    // GetCurrentDirectory(MAX_INI_PATH_LENGTH, string);
    // // QT function needs a char string as input, cannot deal with TCHAR, thus need to convert
    // WideCharToMultiByte(CP_ACP, 0, string, wcslen(string)+1, char_string, MAX_INI_PATH_LENGTH, NULL, NULL);
    // QDir dir(char_string);
    // #else
    // QDir dir(get_current_dir_name());
    // #endif
    // // TODO: check proper path
    // //       or rather: add option to give full path or
    // //       relative?

#ifdef __WIN32__
    // in case of using windows, we need to call the GetCurrentDirectory function
    // as an input it receives: DWORD WINAPI GetCurrentDirectory(_In_  DWORD  nBufferLength, _Out_ LPTSTR lpBuffer);
    // create TCHAR and char string of length MAX_INI_PATH_LENGTH defined in header of this file
    TCHAR string[MAX_INI_PATH_LENGTH];
    char  char_string[MAX_INI_PATH_LENGTH];
    // call windows function with its weird arguments
    GetCurrentDirectory(MAX_INI_PATH_LENGTH, string);
    // need a char string, cannot deal with TCHAR, thus need to convert
    WideCharToMultiByte(CP_ACP, 0, string, wcslen(string)+1, char_string, MAX_INI_PATH_LENGTH, NULL, NULL);
    std::string prePath(char_string);
#else
    std::string prePath;
    prePath = getcwd(NULL, 0);//get_current_dir_name();
#endif
    iniFile = prePath + '/' + iniFile;
    std::cout << "Path to Ini File: " << iniFile << std::endl;

    // // now we have read the QString with the path
    // // to the iniFile
    // // create a QSettings object from this file
    // // settings object contains every setting from file
    // // TODO: check if file exists
    // QSettings settings(iniFile, QSettings::IniFormat);

    // // flag which tracks, if one or more settings was not found in the HVsettings.ini
    // bool containsNotFlag = false;

    boost::property_tree::ptree pt;
    try{
	boost::property_tree::ini_parser::read_ini(iniFile, pt);
    }
    catch (boost::property_tree::ini_parser::ini_parser_error const&){
	std::cout << "Could not find .ini file. Please enter path to .ini" << std::endl;
	std::string input_path;
	const char* prompt = "> ";
	input_path = getUserInputNonNumericalNoDefault(prompt);
	iniFile = prePath + '/' + input_path;
	boost::property_tree::ini_parser::read_ini(iniFile, pt);
    }
    std::cout << pt.get<std::string>("General.baseAddress_hv") << std::endl;
    std::cout << pt.get<std::string>("HvChannels.0_Name") << std::endl;


    // now simply get all values from the property tree created by boost
    // general section
    sAddress_fadc  = pt.get_optional<int>("General.sAddress_fadc").get_value_or(DEFAULT_S_ADDRESS_FADC);
    // boost PropertyTree cannot deal with hex integers. Thus base addresses in config
    // file need to be given as base 10 ints
    baseAddress_hv = pt.get_optional<int>("General.baseAddress_hv").get_value_or(DEFAULT_BASE_ADDRESS_HV);
    // hvModule section
    setKillEnable  = pt.get_optional<bool>("HvModule.setKillEnable").get_value_or(DEFAULT_SET_KILL_ENABLE);
    moduleVoltageRampSpeed  = pt.get_optional<float>("HvModule.moduleVoltageRampSpeed").get_value_or(DEFAULT_MODULE_VOLTAGE_RAMP_SPEED);
    moduleCurrentRampSpeed  = pt.get_optional<float>("HvModule.moduleCurrentRampSpeed").get_value_or(DEFAULT_MODULE_CURRENT_RAMP_SPEED);
    checkModuleTimeInterval = pt.get_optional<int>("HvModule.checkModuleTimeInterval").get_value_or(DEFAULT_CHECK_MODULE_TIME_INTERVAL);

    // hvGroups
    anodeGridGroupFlag          = pt.get_optional<bool>("HvGroups.anodeGridGroupFlag").get_value_or(DEFAULT_ANODE_GRID_GROUP_FLAG);
    anodeGridGroupMasterChannel = pt.get_optional<int>("HvGroups.anodeGridGroupMasterChannel").get_value_or(DEFAULT_ANODE_GRID_GROUP_MASTER_CHANNEL);
    anodeGridGroupNumber        = pt.get_optional<int>("HvGroups.anodeGridGroupNumber").get_value_or(DEFAULT_ANODE_GRID_GROUP_NUMBER);
    monitorTripGroupFlag        = pt.get_optional<bool>("HvGroups.monitorTripGroupFlag").get_value_or(DEFAULT_MONITOR_TRIP_GROUP_FLAG);
    monitorTripGroupNumber      = pt.get_optional<int>("HvGroups.monitorTripGroupNumber").get_value_or(DEFAULT_MONITOR_TRIP_GROUP_NUMBER);
    rampingGroupFlag            = pt.get_optional<bool>("HvGroups.rampingGroupFlag").get_value_or(DEFAULT_RAMPING_GROUP_FLAG);
    rampingGroupNumber          = pt.get_optional<int>("HvGroups.rampingGroupNumber").get_value_or(DEFAULT_RAMPING_GROUP_NUMBER);

    // Fadc
    fadcTriggerType                 = pt.get_optional<int>("Fadc.fadcTriggerType").get_value_or(DEFAULT_FADC_TRIGGER_TYPE);
    fadcFrequency                   = pt.get_optional<int>("Fadc.fadcFrequency").get_value_or(DEFAULT_FADC_FREQUENCY);
    fadcPosttrig                    = pt.get_optional<int>("Fadc.fadcPosttrig").get_value_or(DEFAULT_FADC_POSTTRIG);
    fadcPretrig                     = pt.get_optional<int>("Fadc.fadcPretrig").get_value_or(DEFAULT_FADC_PRETRIG);
    fadcTriggerThresholdRegisterAll = pt.get_optional<int>("Fadc.fadcTriggerThresholdRegisterAll").get_value_or(DEFAULT_FADC_TRIGGER_THRESHOLD_REGISTER_ALL);
    fadcPedestalRunTime             = pt.get_optional<int>("Fadc.fadcPedestalRunTime").get_value_or(DEFAULT_FADC_PEDESTAL_RUN_TIME);
    fadcPedestalNumRuns             = pt.get_optional<int>("Fadc.fadcPedestalNumRuns").get_value_or(DEFAULT_FADC_PEDESTAL_NUM_RUNS);
    fadcChannelSource               = pt.get_optional<int>("Fadc.fadcChannelSource").get_value_or(DEFAULT_FADC_CHANNEL_SOURCE);
    fadcModeRegister                = pt.get_optional<int>("Fadc.fadcModeRegister").get_value_or(DEFAULT_FADC_MODE_REGISTER);

    // Temperature
    _safeUpperTempIMB    = pt.get_optional<int>("Temperature.safeUpperTempIMB").get_value_or(DEFAULT_SAFE_UPPER_IMB_TEMP);
    _safeUpperTempSeptem = pt.get_optional<int>("Temperature.safeUpperTempSeptem").get_value_or(DEFAULT_SAFE_UPPER_SEPTEM_TEMP);
    _safeLowerTempIMB    = pt.get_optional<int>("Temperature.safeLowerTempIMB").get_value_or(DEFAULT_SAFE_LOWER_IMB_TEMP);
    _safeLowerTempSeptem = pt.get_optional<int>("Temperature.safeLowerTempSeptem").get_value_or(DEFAULT_SAFE_LOWER_SEPTEM_TEMP);

    // now loop over HvChannels section to create all of the channels
    int nHvChannels = 0;
    for (auto& key : pt.get_child("HvChannels")) {
        std::cout << key.first << "=" << key.second.get_value<std::string>() << "\n";
        // set the nHvChannels variable to the number of elements in the dummyChannelVector
        // minus 1, so that we can use nHvChannels to access the elements of the list
        nHvChannels = _dummyChannelVector.size() - 1;

        if (key.first.find("Name") != std::string::npos){
            // if Name is in the current key, we add a new dummy channel to the dummyChannelVector
            dummyHvChannel channel;
            channel.name = key.second.get_value<std::string>();
            _dummyChannelVector.push_back(channel);
        }
        else if(key.first.find("Number") != std::string::npos){
            _dummyChannelVector[nHvChannels].number         = key.second.get_value<int>();
        }
        else if(key.first.find("VoltageSet") != std::string::npos){
            _dummyChannelVector[nHvChannels].voltageSet     = key.second.get_value<float>();
        }
        else if(key.first.find("VoltageNominal") != std::string::npos){
            _dummyChannelVector[nHvChannels].voltageNominal = key.second.get_value<float>();
        }
        else if(key.first.find("VoltageBound") != std::string::npos){
            _dummyChannelVector[nHvChannels].voltageBound   = key.second.get_value<float>();
        }
        else if(key.first.find("CurrentSet") != std::string::npos){
            _dummyChannelVector[nHvChannels].currentSet     = key.second.get_value<float>();
        }
        else if(key.first.find("CurrentNominal") != std::string::npos){
            _dummyChannelVector[nHvChannels].currentNominal = key.second.get_value<float>();
        }
        else if(key.first.find("CurrentBound") != std::string::npos){
            _dummyChannelVector[nHvChannels].currentBound   = key.second.get_value<float>();
        }
    }

    _settingsReadFlag = true;

}


// #############################################################################
// ########################### FADC functions ##################################
// #############################################################################
// these are custom functions related to the FADC

std::map<std::string, int> hvFadcManager::GetFadcParameterMap(){
    // this function creates the current FADC parameter map
    // simply done by calling several FADC functions

    std::map<std::string, int> fadcParams;

    fadcParams["NumChannels"]  = F_GetNbOfChannels();
    fadcParams["ChannelMask"]  = F_GetChannelMask();
    fadcParams["PostTrig"]     = F_GetPosttrig();
    fadcParams["PreTrig"]      = F_GetPretrig();
    fadcParams["TriggerRec"]   = F_GetTriggerRecord();
    fadcParams["Frequency"]    = F_GetFrequency();
    fadcParams["ModeRegister"] = F_GetModeRegister();
    // pedestal run is a flag, which simply defines, whether a data file
    // contains a pedestal calibration run or normal data
    // default is false / 0.
    fadcParams["PedestalRun"]  = 0;

    return fadcParams;
}

void hvFadcManager::SetFadcTriggerInLastFrame(int numClock){
    // sets the member variable, which stores the clock cycle at which the
    // FADC triggered in the last frame, which was taken
    // note: this member variable will not be set to 0 (except at initialization
    //       of the hvFadcManager)
    _fadcTriggerInLastFrame = numClock;
}

int hvFadcManager::GetFadcTriggerInLastFrame(){
    // returns the member variable, which stores the clock cycle at which the
    // FADC triggered in the last frame, which was taken
    return _fadcTriggerInLastFrame;
}



// #############################################################################
// ########################### Scintillator functions ##########################
// #############################################################################

void hvFadcManager::SetScintillatorCounters(unsigned short scint1_counter,
                                            unsigned short scint2_counter){
    // this function sets the global timepix variables for the scintillor counters,
    // which count the number of clock cycles between the last scintillator signals
    // and the FADC trigger, which ended the shutter
    _scint1_counter = scint1_counter;
    _scint2_counter = scint2_counter;
    std::cout << "setting scinti values " << _scint1_counter << "\t" << _scint2_counter << std::endl;
}

std::pair<unsigned short, unsigned short> hvFadcManager::GetScintillatorCounters(){
    // this function retuns a pair of unsigned shorts, i.e. the scintillator counter
    // variables, set in SetScintillatorCounters (called from Communication, if FADC
    // triggered in a frame)
    std::pair<unsigned short, unsigned short> scint_counter_pair;
    // create the pair
    scint_counter_pair = std::make_pair(_scint1_counter, _scint2_counter);
    // and return
    return scint_counter_pair;
}


// #############################################################################
// ########################### Safety related temperature ######################
// #############################################################################
// defines functions, which are safety related (of the detector) and thus
// are defined in hvFadcManager, instead of simply the MCP2210 functions.
// this is because this way the safety procedures already defined because of
// HV can be reused

bool hvFadcManager::CheckIfTempsGood(){
    // checks whether the temperatures of IMB and Septem are within the bounds
    // defined by the safe[Upper, Lower][IMB, Septem]Temp variables

    // get temperatures from _currentTemps member variable, which should
    // have been set before the call to this function
    const int temp_imb = _currentTemps.first;
    const int temp_septem = _currentTemps.second;
    bool tempsGood = false;

    if(temp_imb < _safeUpperTempIMB &&
       temp_imb > _safeLowerTempIMB &&
       temp_septem < _safeUpperTempSeptem &&
       temp_septem > _safeLowerTempSeptem){
	tempsGood = true;
    }
    else{
	tempsGood = false;
    }

    // and return
    return tempsGood;
}

void hvFadcManager::SetCurrentTemps(Temps temps){
    // sets the given AtomicTemps as the current temperatures
    // passed by value to copy the temperatures
    _currentTemps = temps;
}

Temps hvFadcManager::GetCurrentTemps(){
    // getter for SetCurrentTemps(), used in WriteTempsInRunFolder()
    // helper thread to print current temperatures to run folder
    return _currentTemps;
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

void hvFadcManager::setSleepTriggerTime(int time){
    std::cout << "setting sleep trigger time to " << time << std::endl;
    _sleepTriggerTime = time;
}

void hvFadcManager::setSleepAcqTime(int time){
    std::cout << "setting sleep acq time to " << time << std::endl;
    _sleepAcqTime = time;
}

void         hvFadcManager::F_StartAcquisition() throw(){
    std::this_thread::sleep_for(std::chrono::microseconds(_sleepAcqTime));
    FADC_module->startAcquisition();
    if (_sleepTriggerTime != 0){
        std::this_thread::sleep_for(std::chrono::microseconds(_sleepTriggerTime));
        F_SendSoftwareTrigger();
    }
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
