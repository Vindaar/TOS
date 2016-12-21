/**********************************************************************/
/*                                                        console.cpp */
/*  TOS - Timepix Operating Software                                  */
/*  with addtional readout software of a CAEN V1729a FADC             */
/*  based on:                                                         */
/*                                                                    */
/*                                                         20.07.2009 */
/*                                                    Christian Kahra */
/*                                     chrkahra@students.uni-mainz.de */
/*                                        Institut fuer Physik - ETAP */
/*                              Johannes-Gutenberg Universitaet Mainz */
/*                                                                    */
/*  with changes by Michael Lupberger (Time Pix)                      */
/*  and Alexander Deisting (FADC)                                     */
/*                                                                    */
/**********************************************************************/

#include "console.hpp"
#include "V1729a.h"
#include "pc.hpp"
#include "hvFadcManager.hpp"
#include "timepix.hpp"
#include "fpga.hpp"
#include "caseHeader.h"

#include <boost/filesystem.hpp>


//C~tor
Console::Console():
         _nbOfChips(0),
         _preload(0),
         _hvFadcManager(NULL),
         _hvFadcManagerActive(false),
	 _prompt(DEFAULT_USER_INPUT_PROMPT)
{
#if DEBUG==2
    std::cout<<"Enter Console::Console()"<<std::endl;
#endif
    // before we create the pc object, we ask for the number of chips
    const char *prompt = "How many chips are on your board? (1-8)";
    std::set<std::string> allowedNumChips;
    std::string input;
    // make sure only 1 to 8 chips are typed in. More than 9 (due to illogical
    // numbering of chips starting from 1 instead of 0) and we will get a
    // segmentation fault in the timepix creator. Size of arrays for chips
    // are hardcoded at the moment to allow for 9 chips.
    for(int l = 1; l < 9; l++) allowedNumChips.insert(std::to_string(l));
    input = getUserInputNumericalDefault(prompt, &allowedNumChips);
    if(input==""){_nbOfChips=1;}
    else{
	_nbOfChips=(unsigned short) atoi(&input[0]);
    }
    std::cout << "Number of Chips: " << _nbOfChips << std::endl << std::endl;

    // create a pointer to a timepix object, with the correct number of chips
    _tp = new Timepix(_nbOfChips);
    // and now create a PC object and hand it the timepix object pointer
    pc = new PC(_tp);

    ok = pc->okay();
    input.clear();
 
    //get preload bytes
    _preload = getPreload();
    pc->fpga->tp->SetNumChips(_nbOfChips,_preload);
}

Console::Console(std::string iniFilePath):
    _nbOfChips(1),
    _preload(0),
    // _fadc(dev),
    _hvFadcManagerActive(true),
    _prompt(DEFAULT_USER_INPUT_PROMPT)
{
#if DEBUG==2
    std::cout<<"Enter Console::Console()"<<std::endl;
#endif
    // create a pointer to a timepix object, with the correct number of chips
    _tp = new Timepix(_nbOfChips);
    // and now create a PC object and hand it the timepix object pointer
    pc = new PC(_tp);
    
    _hvFadcManager = new hvFadcManager(iniFilePath);

    // now the HV_FADC_Obj should be set up and running 
    // HV voltages ramped up

    //init FADC
    pc->initHV_FADC(_hvFadcManager, _hvFadcManagerActive);
    ok = pc->okay();
    
    std::cout << "Warning: In FADC-Mode one can only use one Chip" << std::endl;

    //get preoload
    _preload = getPreload();
    pc->fpga->tp->SetNumChips(_nbOfChips,_preload);
}



Console::~Console()
{
    if(_hvFadcManagerActive){
        delete _hvFadcManager;
    }
    delete pc;
    delete _tp;
}


unsigned short Console::getPreload()
{
    std::string input;
    unsigned short preload = 0;

    const char *prompt = "How many clock cycles do you want to add to the preload counter (usually 0 for a single chip, 3 for octoboard)";
    input = getUserInput(prompt);

    if(input==""){preload=0;}
    else{
	preload=(unsigned short) atoi(&input[0]);
    }
    return preload;
}


int Console::okay(){
#if DEBUG==2
    std::cout<<"Enter Console::okay()"<<std::endl;	
#endif
    return ok;
}


void Console::ConsoleMain(){
#if DEBUG==2
    std::cout<<"Enter Console::ConsoleMain()"<<std::endl;	
#endif	
    //pc->fpga->tp->SaveFSRToFile(FSRFileName.c_str());
    //pc->fpga->tp->ChessMatrix(48,112);
    //pc->fpga->tp->SaveMatrixToFile(MatrixFileName.c_str());

    //call the main "command-function" w or wo FADC commands

    // this function only calls the UserInterface
    UserInterface();
  
    return;
}


void Console::CommandActivateHvFadcManager(){
    // this function activates the usage of the HV_FADC manager
    // after TOS was called without command line arguments 
    // e.g. not with ./TOS -v
    // This function does
    //       - ask for config file to use to initialize HV_FADC object
    //       - set _hvFadcManagerActive flag to true
    //       - initialize _hvFadcManager

    // flags and variables for getUserInput 
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = true;
    std::string input;
    bool activateHFM = true;
    
    if (activateHFM == true){
	// will activate HFM
	std::string iniFilePath;

	const char *promptConfig = "Give the (relative) path to the HFMSettings.ini: ";
	iniFilePath = getUserInput(promptConfig, numericalInput, allowDefaultOnEmptyInput);
	if (iniFilePath == "quit") return;
	if (iniFilePath == ""){
	    iniFilePath = "../config/HFM_settings.ini";
	}


	// set HFM flag to active (only after last user input call!)
	_hvFadcManagerActive = true;
	
	_hvFadcManager = new hvFadcManager(iniFilePath);
	
	//init FADC
	pc->initHV_FADC(_hvFadcManager, _hvFadcManagerActive);
	ok = pc->okay();
    }


}

int Console::UserInterface(){
#if DEBUG==2
    std::cout<<"Enter Console::UserInterface()"<<std::endl;	
#endif

    int running=1;
    int result;
    
    // create a char buffer, which stores the input from the readline() function
    char *buf;
    // define the attempted completion function (our custom TOS_Command_Completion
    // function defined in tosCommandCompletion.cpp
    rl_attempted_completion_function = TOS_Command_Completion;
  
    // while loop runs the basic UserInterface
    // it checks, whether we're still running, prints 
    // TOS> 
    // to each line and reads the user input based on 
    // readline() function
    // ##################################################
    // IMPORTANT!!!
    // each string which is compared needs to be added to the
    // char array tosCommands in tosCommandComplention.cpp!
    // ##################################################
    while(running && ((buf = readline("TOS> ")) != NULL))
    {

	// exit ends the program
	if (strcmp (buf, "exit") == 0) break;
	// if buf starts with NULL, we simply read a new line
	else if (buf[0] == '\0'){
	    free(buf);
	    continue;
	}
	// if something else is typed, we add the command to our 
	// command history
	else{
	    add_history( buf );
	}

	// now initialize a new std::string with the input buf
	// in order to start the case machine
	std::string ein(buf);
	
	if((ein.compare("GeneralReset")==0)||(ein.compare("1")==0)) 
	{
	    result=pc->fpga->GeneralReset();
	    if(result>10){ ErrorMessages(result); }
	    else{ std::cout<<"\tGeneralReset accomplished\n"<<std::flush; }
	}
    
	else if( (ein.compare("Counting")==0) ||
		 (ein.compare("2")==0) )
	{
	    CommandCounting(1);
	}
	else if( (ein.compare("CountingStop")==0) ||
		 (ein.compare("2s")==0) )
	{
	    CommandCounting(0);
	}
	else if( (ein.compare("CountingTrigger") == 0) ||
		 (ein.compare("2t") == 0) )
	{
	    CommandCountingTrigger();
	}
	else if( (ein.compare("CountingTime")==0) ||
		 (ein.compare("2z")==0 ))
	{
	    // this function will not receive an argument anymore. Instead after being called
	    // it will ask the user, which multiplier he wants to use (represents 
	    // standard, long and verylong of the past)
	    CommandCountingTime();
	}
	else if( (ein.compare("ReadOut")==0)||
		 (ein.compare("3")==0) ) 
	{
	    CommandReadOut();
	}
	else if( (ein.compare("ReadOut2")==0)||
		 (ein.compare("3a")==0) ) 
	{
	    CommandReadOut2();
	}
	else if( (ein.compare("SetMatrix")==0)||
		 (ein.compare("4")==0) ) 
	{
	    CommandSetMatrix();
	}
	else if( (ein.compare("WriteReadFSR")==0)||
		 (ein.compare("5")==0) ) 
	{
	    CommandWriteReadFSR();
	}
	else if( (ein.compare("Run")==0) ||
		 (ein.compare("run")==0) )
	{
	    CommandRun(_hvFadcManagerActive);
	}
	else if( ein.compare("EnableTPulse")==0 )
	{
	    pc->fpga->EnableTPulse(1);
	}
	else if( ein.compare("DisableTPulse")==0 )
	{
	    pc->fpga->EnableTPulse(0);
	}
	else if( ein.compare("EnableFADCshutter")==0 )
	{
	    CommandFADCshutter();
	}
	else if( ein.compare("DisableFADCshutter")==0 )
	{
	    pc->fpga->EnableFADCshutter(0);
	}
	else if( ein.compare("DACScan")==0 )
	{
	    CommandDACScan();
	}
	else if( ein.compare("THLScan")==0 )
	{
	    CommandTHLScan();
	}
	else if( ein.compare("SCurve")==0 )
	{
	    CommandSCurve();
	}
	else if( ein.compare("i2creset")==0 )
	{
	    Commandi2creset();
	}
	else if( ein.compare("i2cDAC")==0 )
	{
	    Commandi2cDAC();
	}
	else if( ein.compare("i2cADC")==0 )
	{
	    Commandi2cADC();
	}
	else if( ein.compare("tpulse")==0 )
	{
	    CommandTpulse();
	}
	else if( ein.compare("TestTPulse")==0 )
	{
	    CommandTestTPulse();
	}
	else if( (ein.compare("THSopt")==0) ||
		 (ein.compare("6" )==0) )
	{
	    CommandDoTHSopt();
	}
	else if( (ein.compare("ThEqNoiseCenter")==0) ||
		 (ein.compare("7")==0) ) 
	{
	    CommandThresholdEqNoiseCenter();
	}
	else if( (ein.compare("TOCalib")==0) ||
		 (ein.compare("8")==0) ) 
	{
	    CommandTOCalib();
	}
	else if( (ein.compare("TOCalibFast")==0) ||
		 (ein.compare("8a")==0) ) 
	{
	    CommandTOCalibFast();
	}
	else if( ein.compare("LoadThreshold")==0)
	{
	    CommandLoadThreshold();
	}
	else if( ein.compare("SaveFSR")==0 )
	{
	    CommandSaveFSR();
	}
	else if( (ein.compare("LoadFSR")==0) ||
		 (ein.compare("lf")==0)) 
	{
	    CommandLoadFSR();
	}
	else if( (ein.compare("LoadFSRAll")==0) ||
		 (ein.compare("lfa")==0)) 
	{
	    CommandLoadFSRAll();
	}
	else if( ein.compare("SetDAC")==0 )
	{
	    CommandSetDAC();
	}
	else if( ein.compare("ShowFSR")==0 )
	{
	    CommandShowFSR();
	}
	else if( ein.compare("ChessMatrix")==0 )
	{
	    CommandVarChessMatrix();
	}
	else if( (ein.compare("UniformMatrix")==0) ||
		 (ein.compare("um")==0) ) 
	{
	    CommandUniformMatrix();
	}
	else if( (ein.compare("UniformMatrixAllChips")==0) ||
		 (ein.compare("uma")==0) ) 
	{
	    CommandUniformMatrixAllChips();
	}
	else if( ein.compare("SaveMatrix")==0 )
	{
	    CommandSaveMatrix();
	}
	else if( (ein.compare("LoadMatrix")==0) ||
		 (ein.compare("loadmatrix")==0) )
	{
	    CommandLoadMatrix();
	}
	else if( ein.compare("Trigger")==0 )
	{
	    CommandSwitchTriggerConnection();
	}
	else if( ein.compare("SetIP")==0 )
	{
	    CommandSetIP();
	}
	else if( ein.compare("ShowIP")==0 )
	{
	    std::cout<<pc->fpga->ShowIP()<<"\n"<<std::flush;
	}
	else if( (ein.compare("MakeARP")==0) ||
		 (ein.compare("makearp")==0) )
	{
	    pc->fpga->MakeARPEntry();
	}
	else if( ein.compare("quit")==0 )
	{
	    running=0;
	}
	else if( ein.compare("help")==0 )
	{
	    CommandHelp();
	}
	else if( ein.compare("spacing")==0 )
	{
	    CommandSpacing();
	}
	else if( ein.compare("SetNumChips")==0 )
	{
	    CommandSetNumChips();
	}
	else if( ein.compare("SetOption")==0 )
	{
	    CommandSetOption();
	}
	else if( ein.compare("CheckOffset")==0 )
	{
	    CommandCheckOffset();
	}
	else if( ein.compare("Calibrate")==0 )
	{
	    CommandCalibrate();
	}
	else if( ein.compare("SetChipIDOffset")==0 )
	{
	    CommandSetChipIdOffset();
	}
	else if( ein.compare("EnableFastClock")==0 )
	{
	    pc->fpga->UseFastClock(1);
	}
	else if( ein.compare("DisableFastClock")==0 )
	{
	    pc->fpga->UseFastClock(0);
	}

	
	// ##################################################
	// ################## FADC related commands #########
	// ##################################################
	
	else if (ein.compare("PrintFADCSettings") == 0){
	    _hvFadcManager->FADC_Functions->printSettings();
	}
	else if (ein.compare("ResetFADC") == 0){
	    _hvFadcManager->F_Reset();
	}
	else if (ein.compare("SetFadcSettings") == 0){
	    _hvFadcManager->SetFadcSettings();
	}
	else if (ein.compare("StartFadcPedestalRun") == 0){
	    CommandFadcPedestalRun();
	}
	else if ((ein.compare("StartFadcAcquisition") == 0) ||
		 (ein.compare("StartFadcAcq")         == 0)){
	    
	    _hvFadcManager->F_StartAcquisition();
	}
	else if (ein.compare("SendFadcSoftwareTrigger") == 0){
	    _hvFadcManager->F_SendSoftwareTrigger();	    
	}
	else if (ein.compare("ReadFadcInterrupt") == 0){
	    std::cout << "Interrupt: " << _hvFadcManager->F_ReadInterrupt() << std::endl;
	}
	else if (ein.compare("ReleaseFadcInterrupt") == 0){
	    _hvFadcManager->F_ReleaseInterrupt();
	}
	else if (ein.compare("SetFadcTriggerThresholdDACAll") == 0){
	    _hvFadcManager->F_SetTriggerThresholdDACAll( getInputValue() );
	}
	else if (ein.compare("GetFadcTriggerPerChannel") == 0){
	    std::cout << "getTriggerThreshold perChannel:" << std::endl;
	    std::cout << "Channel 1: " <<  _hvFadcManager->F_GetTriggerThresholdDACPerChannel(0) << std::endl;
	    std::cout << "Channel 2: " <<  _hvFadcManager->F_GetTriggerThresholdDACPerChannel(1) << std::endl;
	    std::cout << "Channel 3: " <<  _hvFadcManager->F_GetTriggerThresholdDACPerChannel(2) << std::endl;
	    std::cout << "Channel 4: " <<  _hvFadcManager->F_GetTriggerThresholdDACPerChannel(3) << std::endl;
	    std::cout << "getTriggerThreshold All: " << _hvFadcManager->F_GetTriggerThresholdDACAll() << std::endl << std::endl;
	}
	else if (ein.compare("SetFadcTriggerThresholdRegisterAll") == 0){
	    // TODO: PUT INTO FUNCTION
	    std::string input;
	    input = getUserInputNonNumericalNoDefault(_prompt);
	    unsigned int result;
	    result = _hvFadcManager->FADC_Functions->setTriggerThresholdRegisterAll( std::stoi(input) );
	    std::cout << "setTriggerThresholdRegisterAll returns " << result << std::endl;
	}
	else if (ein.compare("GetFadcTriggerThresholdRegister") == 0){
	    _hvFadcManager->FADC_Functions->getTriggerThresholdRegister();
	}
	else if (ein.compare("LoadFadcTriggerThresholdDAC") == 0){
	    _hvFadcManager->F_LoadTriggerThresholdDAC();
	}
	else if (ein.compare("SetFadcTriggerType") == 0){
	    _hvFadcManager->F_SetTriggerType( getInputValue() );
	}
	else if (ein.compare("GetFadcTriggerType") == 0){
	    std::cout << "Trigger type: " << _hvFadcManager->F_GetTriggerType() << std::endl;
	}
	else if (ein.compare("SetFadcTriggerChannelSource") == 0){
	    _hvFadcManager->F_SetTriggerChannelSource( getInputValue() );
	}
	else if (ein.compare("GetFadcTriggerChannelSource") == 0){
	    std::cout << "Trigger channel source: " << _hvFadcManager->F_GetTriggerChannelSource() << std::endl;
	}
	else if (ein.compare("SetFadcPostTrig") == 0){
	    _hvFadcManager->F_SetPosttrig( getInputValue() );
	}
	else if (ein.compare("GetFadcPostTrig") == 0){
	    std::cout << "Posttrig: " << _hvFadcManager->F_GetPosttrig() << std::endl;
	}
	else if (ein.compare("SetFadcPreTrig") == 0){
	    _hvFadcManager->F_SetPretrig( getInputValue() );
	}
	else if (ein.compare("GetFadcPreTrig") == 0){
	    std::cout << "Pretrig: " << _hvFadcManager->F_GetPretrig() << std::endl;
	}
	else if (ein.compare("SetFadcChannelMask") == 0){
	    _hvFadcManager->F_SetChannelMask( getInputValue() );
	}
	else if (ein.compare("GetFadcChannelMask") == 0){
	    std::cout << "Channel mask: " << _hvFadcManager->F_GetChannelMask() << std::endl;
	}
	else if (ein.compare("SetFadcNumberOfChannels") == 0){
	    _hvFadcManager->F_SetNbOfChannels(getInputValue());
	}
	else if (ein.compare("GetFadcNumberOfChannels") == 0){
	    std::cout << "#Channels: " << _hvFadcManager->F_GetNbOfChannels() << std::endl;
	}
	else if (ein.compare("SetFadcModeRegister") == 0){
	    _hvFadcManager->F_SetModeRegister(static_cast<const unsigned short>(getInputValue()));
	}
	else if (ein.compare("GetFadcModeRegister") == 0){
	    std::cout << "mode register: " << _hvFadcManager->F_GetModeRegister() << std::endl;
	}
	else if (ein.compare("SetFadcFrequency") == 0){
	    _hvFadcManager->F_SetFrequency( getInputValue() );
	}
	else if (ein.compare("GetFadcFrequency") == 0){
	    std::cout << "Frequency: " << _hvFadcManager->F_GetFrequency() << std::endl;
	}
	else if (ein.compare("SetFadcReadMode") == 0){
	    _hvFadcManager->F_SetReadMode( getInputValue() );
	}
	else if (ein.compare("GetFadcReadMode") == 0){
	    std::cout << "Read mode: " << _hvFadcManager->F_GetReadMode() << std::endl;
	}
	else if (ein.compare("SetFadcPostStopLatency") == 0){
	    _hvFadcManager->F_SetPostStopLatency( getInputValue() );
	}
	else if (ein.compare("GetFadcPostStopLatency") == 0){
	    std::cout << "Post stop latency: " << _hvFadcManager->F_GetPostStopLatency() << std::endl;
	}
	else if (ein.compare("SetFadcPostLatencyPreTrig") == 0){
	    _hvFadcManager->F_SetPostLatencyPretrig( getInputValue() );
	}
	else if (ein.compare("GetFadcPostLatencyPretrig()") == 0){
	    std::cout << "Post latency pretrig: " << _hvFadcManager->F_GetPostLatencyPretrig() << std::endl;
	}
	else if (ein.compare("SetFadcSleepAcqTime") == 0){
	    std::string input;
	    input = getUserInputNumericalNoDefault(_prompt);
	    _hvFadcManager->setSleepAcqTime(std::stoi(input));
	}
	else if (ein.compare("SetFadcSleepTriggerTime") == 0){
	    std::string input;
	    input = getUserInputNumericalNoDefault(_prompt);
	    _hvFadcManager->setSleepTriggerTime(std::stoi(input));
	}
	else if (ein.compare("SetCenterChip") == 0){
	    CommandSetCenterChip();
	}
	else if (ein.compare("PrintCenterChip") == 0){
	    CommandPrintCenterChip();
	}

	// ##################################################
	// ################## HV_FADC related commands ######
	// ##################################################	


	else if (ein.compare("ActivateHFM") == 0)
	{
	    CommandActivateHvFadcManager();
	}
	
	else if (ein.compare("ConnectModule") == 0)
	{
	    if(_hvFadcManagerActive == true){
		std::cout << _hvFadcManager->H_ConnectModule() << std::endl;
	    }
	}

	// main function to call
	else if ((ein.compare("InitHFM") == 0) ||
		 (ein.compare("InitHV_FADC") == 0))
	{
	    // if the HV_FADC object is initialized
	    if(_hvFadcManagerActive == true){
		_hvFadcManager->InitHFMForTOS();
	    }
	    // if it is not initialized
	    else{
		std::cout << "Currently not using HV_FADC Object \n" 
			  << "Call ActivateHFM command and try again"
			  << std::endl;
	    }
	}
	// function to call to ramp up
	else if (ein.compare("RampChannels") == 0)
	{
	    // if the HV_FADC object is initialized
	    if(_hvFadcManagerActive == true){
		_hvFadcManager->RampChannels();
	    }
	    // if it is not initialized
	    else{
		std::cout << "Currently not using HV_FADC Object \n" 
			  << "Call ActivateHFM command and try again"
			  << std::endl;
	    }
	}

	else if (ein.compare("ShutdownHFM") == 0){
	    if (_hvFadcManagerActive == true){
		_hvFadcManager->ShutDownHFMForTOS();
		// now delete the hvFadcManager
		if(_hvFadcManagerActive){
		    delete _hvFadcManager;
		}
		// and set the flag to false
		_hvFadcManagerActive = false;
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}

	else if (ein.compare("CheckHVModuleIsGood") == 0){
	    if (_hvFadcManagerActive == true){
		_hvFadcManager->H_CheckHVModuleIsGood();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}

	else if (ein.compare("TurnChannelOnOff") == 0){
	    if (_hvFadcManagerActive == true){
		_hvFadcManager->TurnChannelOnOff();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}

	else if (ein.compare("ClearChannelEventStatus") == 0){
	    if (_hvFadcManagerActive == true){
		_hvFadcManager->ClearChannelEventStatus();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}

	else if (ein.compare("PrintChannelStatus") == 0){
	    if (_hvFadcManagerActive == true){
		_hvFadcManager->printAllChannelStatus();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}
	else if (ein.compare("PrintChannelEventStatus") == 0){
	    if (_hvFadcManagerActive == true){
		_hvFadcManager->printAllChannelEventStatus();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}
	else if (ein.compare("PrintModuleStatus") == 0){
	    if (_hvFadcManagerActive == true){
		_hvFadcManager->printModuleStatus();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}
	else if (ein.compare("PrintModuleEventStatus") == 0){
	    if (_hvFadcManagerActive == true){
		_hvFadcManager->printModuleEventStatus();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}
	else if (ein.compare("AddChannel") == 0){
	    if (_hvFadcManagerActive == true){
		CommandAddChannel();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}
	else if (ein.compare("RemoveChannel") == 0){
	    if (_hvFadcManagerActive == true){
		CommandRemoveChannel();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}
	else if (ein.compare("AddFlexGroup") == 0){
	    if (_hvFadcManagerActive == true){
		CommandAddFlexGroup();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}
	else if (ein.compare("RemoveFlexGroup") == 0){
	    if (_hvFadcManagerActive == true){
		CommandRemoveFlexGroup();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}
	else if (ein.compare("PrintActiveChannels") == 0){
	    if (_hvFadcManagerActive == true){
		_hvFadcManager->PrintActiveChannels();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}

	else if (ein.compare("SetChannelValue") == 0){
	    if (_hvFadcManagerActive == true){
		CommandSetChannelValue();
	    }
	    else{
		std::cout << "HFM not initialized. Nothing to do."
			  << std::endl;
	    }
	}

	// ##################################################
	// ################## MCP2210 related commands ######
	// ##################################################	

	else if (ein.compare("TempLoopReadout") == 0){
	    CommandTempLoopReadout();
	}	
	    
	
	// if no other if was true, command not found
	else if( ein.compare("")==0 )
	{
	    // TODO: is this necessary??
	    std::cout << "" << std::flush;
	}
	else{
	    std::cout<<"command not found"<<std::endl;
	}

	// free the buffer pointer and point it to NULL again
	free(buf);
	buf = NULL;
	
    }// end while(running){}
    // if while loop ends and buffer not freed, free it.
    if (buf != NULL) free(buf);

    return 0;
}

int Console::ReadoutFpgaExtTriggerBit()
{
  return pc->fpga->ReadoutFadcBit();
}


int Console::ReadoutFpgaExtTriggerFlag()
{
  return pc->fpga->ReadoutFadcFlag();
}


void Console::ClearFpgaExtTriggerFlag()
{
  pc->fpga->ClearFadcFlag();
}


void Console::ErrorMessages(int err){
    switch(err){
	case 11: 
	    std::cout << "Error "
		      << err 
		      << ": In FPGA::Communication (called by GeneralReset) - no valid file-descriptors\n"
		      << std::flush; 
	    break;
	case 12: 
	    std::cout << "Error "
		      << err 
		      << ": In FPGA::Communication (called by GeneralReset) - timeout\n"
		      << std::flush; 
	    break;
	case 13: 
	    std::cout << "Warning "
		      << err 
		      << ": In FPGA::Communication (called by GeneralReset) - wrong packet-number received\n"
		      << std::flush; 
	    break;
	case 21: 
	    std::cout << "Error "
		      << err 
		      << ": In FPGA::Communication (called by Counting) - no valid file-descriptors\n"
		      << std::flush; 
	    break;
	case 22: 
	    std::cout << "Error "
		      << err 
		      << ": In FPGA::Communication (called by Counting) - timeout\n"
		      << std::flush; 
	    break;
	case 23: 
	    std::cout << "Warning"
		      << err 
		      << ": In FPGA::Communication (called by Counting) - wrong packet-number received\n"
		      << std::flush; 
	    break;
	case 24: 
	    std::cout << "Warning"
		      << err 
		      << ": In Console::CommandCounting - Software thinks Timepix is already counting. Nothing has be done. Stop counting before you continue. If Software is wrong and Timepix is not counting, it will have any effect.\n"
		      << std::flush;
	    break;
	case 25: 
	    std::cout << "Warning"
		      << err 
		      << ": In Console::CommandCounting - Software thinks Timepix is already not-counting. Anyway, a further stop-command will be sent now."
		      << std::endl; 
	    break;
	case 301: 
	    std::cout << "Error "
		      << err 
		      << ": In FPGA::Communication (called by ReadOut) - no valid file-descriptors\n"
		      << std::flush; 
	    break;
	case 302: 
	    std::cout << "Error "
		      << err 
		      << ": In FPGA::Communication (called by ReadOut) - timeout\n"
		      << std::flush; 
	    break;
	case 303: 
	    std::cout << "Warning "
		      << err 
		      << ": In FPGA::Communication (called by ReadOut) - wrong packet-number received\n"
		      << std::flush; 
	    break;
	case 310: 
	    std::cout << "Error "
		      << err 
		      << ": In FPGA::SaveData (called by ReadOut) - could not open DataFile\n"
		      << std::flush; 
	    break;
	case 41: 
	    std::cout << "Error "
		      << err 
		      << ": In FPGA::Communication (called by SetMatrix) - no valid file-descriptors\n"
		      << std::flush; 
	    break;
	case 42: 
	    std::cout << "Error "
		      << err 
		      << ": In FPGA::Communication (called by SetMatrix) - timeout\n"
		      << std::flush; 
	    break;
	case 43: 
	    std::cout << "Warning "
		      << err 
		      << ": In FPGA::Communication (called by SetMatrix) - wrong packet-number received\n"
		      << std::flush; 
	    break;
	case 51: 
	    std::cout << "Error "
		      << err 
		      << ": In FPGA::Communication (called by WriteReadFSR) - no valid file-descriptors\n"
		      << std::flush; 
	    break;
	case 52: 
	    std::cout << "Error "
		      << err 
		      << ": In FPGA::Communication (called by WriteReadFSR) - timeout\n"
		      << std::flush; 
	    break;
	case 53: 
	    std::cout << "Warning "
		      << err 
		      << ": In FPGA::Communication (called by WriteReadFSR) - wrong packet-number received\n"
		      << std::flush; 
	    break;
	case 59: 
	    std::cout << "Warning "
		      << err 
		      << ": In Timepix::ChipID (called by WriteReadFSR) - wrong ChipID -  received for one of the chips: "
		      << pc->fpga->ErrInfo<<", expected: "
		      << std::flush;
	    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
		std::cout << "chip " << chip << ": " 
			  << pc->fpga->tp->GetChipID(chip) << "\n>" 
			  << std::flush;
	    }
	    break;

	case 61: 
	    std::cout << "Error "
		      << err 
		      << ": In Timepix::SetDAC (called by CommandSetDAC) - illegal value for this dac\n"
		      << std::flush; 
	    break;
	case 62: 
	    std::cout << "Error "
		      << err 
		      << ": In TimePix::SetDAC (called by CommandSetDAC) - illegal DAC number\n"
		      << std::flush; 
	    break;
	case 63: 
	    std::cout << "Error "
		      << err 
		      << ": In Console::SenseDAC - illegal DAC number\n"
		      << std::flush; 
	    break;
	case 64:
	    std::cout << "Error "
		      << err
		      << ": In Timepix::SetDAC (called by CommandSetDAC) - illegal chip number\n"
		      << std::flush;
	    break;
	case 80: 
	    std::cout << "Error "
		      << err 
		      << ": In TimePix::UniformMatrix - illegal input\n"
		      << std::flush; 
	    break;
	case 81: 
	    std::cout << "Error "
		      << err 
		      << ": In TimePix::VarChessMatrix - illegal input\n"
		      << std::flush; 
	    break;
	case 82: 
	    std::cout << "Error "
		      << err 
		      << ": In PC::ThresholdNoise - invalid ThrH\n"
		      << std::flush; 
	    break;
    }

    return;
}


int Console::CommandHelp()
{
    std::cout << "Most important commands:\n\t"

              << "UserInterface\n\t"
              << "help                                       = print this help message \n\t"
              << "quit                                       = quit TOS \n\t"
              << "GeneralReset                               = 1: reset chip \n\t"
              << "Counting                                   = 2: open shutter manually \n\t"
              << "CountingStop                               = 2s: close shutter manually\n\t"

              << "CountingTrigger <shutter-time>             = 2t <shutter-time>: "
              << "open shutter after external trigger for t[µs] = 46* <shutter-time>/40 \n\t"
        
              << "CountingTime <shutter-time>                = 2z <shutter-time>: "
              << "open shutter for t[µs] = 46* <shutter-time>/40 \n\t"

              << "2f <shutter-time>: open shutter for t[µs]  = 46* <shutter-time>/40 "
              << "but with a 80MHz sampling clock \n\t"
        
              << "2l <shutter-time>: open shutter for t[µs]  = 46*256* <shutter-time>/40 \n\t"
              << "2vl <shutter-time>: open shutter for t[µs] = 46*256*256* <shutter-time>/40 \n\t"
              << "ReadOut                                    = 3: read matrix from chip, "
              << "non zero suppressed, save data in data/data.txt \n\t"
        
              << "ReadOut3                                   = 3a: read matrix from chip, "
              << "zero suppressed, save data in data/data.txt \n\t"
        
              << "SetMatrix                                  = 4:  send matrix data to chip "
              << "and hence set the matrix \n\t"

              << "WriteReadFSR                               = 5:  send fsr data to chip and "
              << "hence set the DACs \n\t"

              << "THSopt                                     = 6:  THS optimization \n\t"
              << "ThEqNoiseCenter                            = 7:  Threshold equalization "
              << "using the noise methode, threshold equalisation information will be saved "
	      << "in threshold.txt \n\t"

              << "TOCalib                                    = 8:  TOT and TOA "
              << "calibration, old methode \n\t"

              << "TOCalibFast                                = 8a: TOT and TOA "
              << "calibration, new methode, should be used, output will be in "
              << "TOACalib.txt/TOTCalib.txt \n\t"

              << "Run:                                       =     starts data taking, "
              << "specify run time, shutter length, run mode \n\t"

              << "EnableTPulse:                              =     enable testpulse bit, "
              << "stays until reset or DisableTPulse command; when enabled: test pulses "
              << "as defined with tpulse command in shutter window when using 2z, 2f, 2l, 2vl \n\t"

              << "DisableTPulse:                             =     disables testpulse bit \n\t"
              << "DACScan:                                   =     Scan through DAC values "
              << "for specified DACs and record voltage with ADC, saved in DACScan.txt \n\t"

              << "SCurve:                                    =     do a SCurve scan, "
              << "old methode, outputs lots of data files \n\t"

              << "SCurveFast:                                =     do a SCurve scan, "
              << "new methode, should be used. Data saved in data/chipX/voltageY \n\t"

              << "i2creset:                                  =     set DACs to "
              << "powerdown mode \n\t"

              << "i2cDAC:                                    =     use to set the "
              << "ext_DAC and ANIN DACs \n\t"

              << "i2cADC:                                    =     manually read "
              << "back level at ADC \n\t"

              << "tpulse:                                    =     creat a single "
              << "test pulse, specify test pulse parameters \n\t"

              << "LoadThreshold:                             =     Load threshold.txt "
              << "file into software; File that holds information of threshold equalisation \n\t"

              << "SaveFSR:                                   =     Save values for "
              << "DACs as currently in software to fsr.txt \n\t"

              << "LoadFSR                                    = lf: load DAC values "
              << "from fsr.txt into software; you need to send it to the chip by "
              << "command 5 to set it in the chip \n\t"

              << "SetDAC:                                    =     set the value of "
              << "a specific DAC in the software\n\t"

              << "UniformMatrix:                             =     generate a uniform "
              << "matrix in software; you need to send it to the chip by command 4 to set it in the chip \n\t"

              << "ChessMatrix:                               =     generate a chess "
              << "matrix \n\t"

              << "SaveMatrix:                                =     save matrix as "
              << "currently in software to matrix.txt \n\t"

              << "LoadMatrix:                                =     load matrix.txt "
              << "into software \n\t"

              << "MakeARP:                                   =     generate the ARP "
              << "entry needed for comminication with FPGA"

              << "\n\n"
              << "Clock frequency is 40 MHz\n"
              << "EnableFastClock                            =     to set clock to "
              << "80 MHz in shutter, shutter opening time calculation as for 40 MHz"

              << "DisableFastClock (default)                 =     to use 40 MHz "
              << "clock in shutter"
              << "\n\n" 
              << std::flush; 

    return 1;
}


int Console::CommandSpacing(){
    // TODO: set a sensible allowedString value for the input
    //       afaiu all values 0 to 255 allowed?
    std::string input;
    unsigned int space = 0;
    const char *promptSpacing = "Please provide a spacing> ";
    input = getUserInputNumericalNoDefault(promptSpacing);
    if (input == "quit") return -1;
    space = std::stoi(input);
    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
        pc->fpga->tp->Spacing(space,0,chip);
    }
    return 0;
}

int Console::CommandSetNumChips(){
    // this function uses getUserInput to get the number
    // of chips to be set and then calls the
    // SetNumChips(int nChips) function to actually
    // set this number internally
    int nChips;
    std::string ein="";
    const char *prompt = "How many chips are on your board? (1-8)";
    ein = getUserInput(prompt);
    if (ein == "quit") return -1;
    if (ein == "") {
	nChips = 1;
    }
    else{
	nChips = (unsigned short) atoi(&ein[0]);
    }
    // got user input, now call
    SetNumChips(nChips);
    
    return 0;
}

void Console::SetNumChips(int nChips){
    // this function sets the number of chips to nChips
    // it is not called Command*, because it is not callable
    // by the user from ther user interface, but rather is the internal
    // function to set the number of chips, which is called by
    // CommandSetNumChips and at other points in the code (e.g. setting
    // number of chips to 1, in order to use the HV_FADC object)
    _nbOfChips = nChips;

    _preload = getPreload();
    pc->fpga->tp->SetNumChips(_nbOfChips,_preload);
    pc->fpga->WriteReadFSR();
    pc->fpga->WriteReadFSR();
    for (unsigned short chip = 1; chip <= pc->fpga->tp->GetNumChips(); chip++){
	pc->fpga->tp->GetChipID(chip);
    }
}


int Console::CommandSetOption(){
    unsigned short option = 0;
    std::string ein="";
    const char *prompt = "Option (0 to 255)";
    ein = getUserInput(prompt);
    if (ein == "quit") return -1;
    if(ein==""){option=0;}
    else{
	option=(unsigned short) atoi(&ein[0]);
    }
    pc->fpga->tp->SetOption(option);
    pc->fpga->WriteReadFSR();
    pc->fpga->WriteReadFSR();
    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	pc->fpga->tp->GetChipID(chip);
    }
    return 0;
}


int Console::CommandRun(bool useHvFadc){
#if DEBUG==2
    std::cout<<"Enter Console::CommandCounting()"<<std::endl;	
#endif
  
    int result = 1;
    // TODO: probably a good idea to replace runtimeFrames variable at some point
    unsigned short runtimeFrames = 0;              //< run mode var  
    int shutterTime = 128;
    int runtime = 0;                               //< var to store the runtime or the nb of triggers
    std::string shutter_mode;
    unsigned short run_mode = 0;

    // variables related to UserInput
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    bool useFastClock;
    bool useExternalTrigger;
    std::string inputRunParameter;
    std::string inputTrigger;
    std::string inputShutterMode;
    std::string inputShutterTime;
    std::string inputFastClock;
    std::string inputZeroSuppression;
    std::string input;


    // this function works as follows:
    // asks user for time or external trigger
    // asks user for that selection number of triggers or runtime
    // asks user for shutter mode // ShutterRangeSelection
    // asks user for shutter time // ShutterTimeSelection
    // asks user for zero suppression
    // starts run: DoRun()


  
    //ProfilerStart();

    //Get input vals from user
    //defined runtime or trigger
    const char *promptRunParameter = "Please give parameters for run. Do you want to use a defined run time (0) or record a defined number of frames (1)?";
    std::set<std::string> allowedRunParameterStrings = {"0", "1"};
    inputRunParameter = getUserInputNumericalNoDefault(promptRunParameter, &allowedRunParameterStrings);
    if (inputRunParameter == "quit") return -1;

    // exception handling is done in getUserInput, thus this conversion should 
    // always work
    runtimeFrames = std::stoi(inputRunParameter);
    std::cout << "Run parameter set to: " << runtimeFrames << std::endl;
    if (runtimeFrames == 1)
    {
	const char *promptFrames = "(Run)\t Number of frames you want = ";
	input = getUserInput(promptFrames, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	runtime = std::stoi(input);
	std::cout << "(Run)\t #Frames set to: " << runtime <<  std::endl;
    }
    else if(runtimeFrames == 0)
    {
	const char *promptRuntime = "(Run)\t Runtime [sec]= ";
	input = getUserInput(promptRuntime, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	runtime = std::stoi(input);
	std::cout << "(Run)\t Runtime [sec] set to: " << runtime << std::endl;
    }


    // explanation of previous input to current input methods:
    // Shutter mode
    // 0 = untriggered, 
    //     inputTrigger == "noexternal" && inputShutterMode == "standard" && inputFastClock == "standard"
    // 1 = external trigger,
    //     inputTrigger == "external"   && inputShutterMode == "standard" && inputFastClock == "standard"
    // 2 = untriggered 2x faster clock, 
    //     inputTrigger == "noexternal" && inputShutterMode == "standard" && inputFastClock == "fastclock"
    // 3 = external trigger 2x faster clock, 
    //     inputTrigger == "external"   && inputShutterMode == "standard" && inputFastClock == "fastclock"
    // 4 = untriggered long
    //     inputTrigger == "noexternal" && inputShutterMode == "long"     && inputFastClock == "standard"
    // 5 = untriggered very long
    //     inputTrigger == "noexternal" && inputShutterMode == "verylong" && inputFastClock == "standard"

    // select fastclock, yes or no
    inputFastClock = FastClockSelection();
    if (inputFastClock == "quit") return -1;
    else if (inputFastClock == "standard"){
	useFastClock = true;
    }
    else{
	useFastClock = false;
    }

    // select trigger, external or not
    inputTrigger = TriggerSelection();
    if (inputTrigger == "quit") return -1;
    else if (inputTrigger == "noexternal"){
	useExternalTrigger = false;
    }
    else{
	useExternalTrigger = true;
    }

    // Shutter mode
    // for the shutter mode, we can call ShutterRangeSelection()
    if (useExternalTrigger == false){
	// only if we do not use an external trigger, we allow the selection of 
	// the different shutter modes
	inputShutterMode = ShutterRangeSelection();
	if (inputShutterMode == "quit") return -1;
	else{
	    shutter_mode = inputShutterMode;
	}
    }
    else{
	shutter_mode = "external";
    }

    //Shutter time; call ShutterTimeSelection with inputShutterMode (as an int) 
    //              as argument.
    inputShutterTime = ShutterTimeSelection(shutter_mode);
    if (inputShutterTime == "quit") return -1;
    shutterTime = std::stoi(inputShutterTime);

    //full matrix or zero surpressed
    std::cout << "(Run)\t Run mode (0 = zero suppressed, 1 = complete matrix (slow)" << std::endl;
    if(useHvFadc) std::cout << "Choose zero surpressed if you want to use the FADC" << std::endl;
    std::set<std::string> allowedZeroSuppressionStrings = {"zero",     "0", 
							   "complete", "1" };
    inputZeroSuppression = getUserInputNonNumericalNoDefault(_prompt, 
							     &allowedZeroSuppressionStrings);
    if (inputZeroSuppression == "quit") return -1;
    else if ( (inputZeroSuppression == "zero") ||
	      (inputZeroSuppression == "0") ){
	// in case of zero suppression, set run_mode to 0
	run_mode = 0;
    }
    else{
	// else, we use complete readout
	run_mode = 1;
    }
    /*
     * Set settings for the use of the fadc
     */
    if(useHvFadc)
    {
	// temporary int variable, used as flag for yes / no inputs
	int temp;
	std::cout << "Detected FADC - do you want to start a measurement with simultaneous Chip and Fadc readout? 1 = yes, 0 = no \n" 
		  << std::endl;
	
	//check if the user wants to use the fadc
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	temp  = std::stoi(input);
	if(temp != 1){
	    useHvFadc = false;
	    std::cout << "Don't use the Fadc" << std::endl; 
	}
	else{
	    //print fadc settings
	    _hvFadcManager->FADC_Functions->printSettings();

	    std::cout << "Return to main menu to change some Fadc settings? 1 = y, 0 = n \n" 
		      << "If yes, this aborts the run."
		      << std::endl;
	    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	    if (input == "quit") return -1;
	    temp  = std::stoi(input);
	    if(temp == 1){
		std::cout << "Aborting Run - display menu by entering 0" << std::endl;
		return 0; 
	    }      
	}
    }//end of if(useHvFadc)

    if(runtime==0){
	std::cout << "\t\tRun starts now. <q> to stop\n" << std::endl;
    }

    // Start measurement
    // TODO: finish changing Run function!
    if(useHvFadc){
	result = pc->DoRun(runtimeFrames, 
			  runtime, 
			  shutterTime, 
			  0, 
			  shutter_mode, 
			  run_mode, 
			  useFastClock, 
			  useExternalTrigger, 
			  useHvFadc);
    }
    else {
	// if Fadc not used, init FADC with NULL and call standard DoRun function
	pc->initHV_FADC(NULL,false);
	result = pc->DoRun(runtimeFrames, 
			  runtime, 
			  shutterTime, 
			  0, 
			  shutter_mode, 
			  run_mode,
			  useFastClock,
			  useExternalTrigger);
    }

    // print error message
    if(result>0){
	ErrorMessages(90+result); 
	return -1;
	std::cout << "Run closed" << std::endl;
    }
  
    //FIXME: drop this?
    if((!useHvFadc) && _hvFadcManagerActive) return 1;
    else return 0;	
}//end CommandRun


int Console::CommandCounting(int c){
#if DEBUG==2
    std::cout<<"Enter Console::CommandCounting()"<<std::endl;	
#endif
    int result=0;
    if(c!=0){
	if(pc->fpga->tp->IsCounting()!=0){ErrorMessages(24);}
	else{	
	    result=pc->fpga->Counting();
	    if(result>20){ErrorMessages(result);}
	    else{std::cout<<"\tCounting started\n"<<std::flush;}
	}
    }
    else{
	if(pc->fpga->tp->IsCounting()==0){ErrorMessages(25);}
	result=pc->fpga->CountingStop();
	if(result>20){ErrorMessages(result);}
	else{std::cout<<"\tCounting stopped\n"<<std::flush;}
    }
    return result;
}

std::set<int> Console::ChipSelection(){
    // this function provides the user input interface to select a 
    // set of chips based on all current active chips
    std::set<int> chip_set;
    std::string input;
    std::set<std::string> allowedStrings;

    std::cout << "Select which chips to use.\n"
	      << "the following chips are currently connected:" << std::endl;
    for(int i = 0; i < pc->fpga->tp->GetNumChips(); i++){
	std::cout << "#" 
		  << i+1 
		  << " : " 
		  << pc->fpga->tp->GetChipName(i+1) 
		  << std::endl;
    }
    std::cout << "type # of chip from above to select.\n"
	      << "select one after another\n"
	      << "type : { 0, all } to select all chips" << std::endl;
    
    // now create the allowed strings for the input based on the number of chips
    // the loop goes from 0 to <= num chips, because we also allow 0 as an input value 
    // to select all chips
    for(int i = 0; i <= pc->fpga->tp->GetNumChips(); i++) allowedStrings.insert(std::to_string(i));
    // and also add the string 'all' as well
    allowedStrings.insert("all");
    // and the string 'done' to indicate that the selection is complete
    allowedStrings.insert("done");
    
    do{
	// now print the currently active chips for the user
	std::cout << "currently selected chips: " << std::flush;
	if( chip_set.begin() == chip_set.end() ){
	    std::cout << "none" << std::endl;
	}
	else{
	    std::cout << std::endl;
	    // print current set
	    std::for_each( chip_set.begin(), chip_set.end(), [this](int chip){
		    std::cout << "#" 
			      << chip 
			      << " : " 
			      << this->pc->fpga->tp->GetChipName(chip)
			      << std::endl;
		} );
	    std::cout << "to finish selecting chips, type { done }" << std::endl;
	}

	input = getUserInputNonNumericalNoDefault(_prompt, &allowedStrings);
	if(input == "quit") return {-1};
	else if( (input == "0") ||
		 (input == "all") ){
	    // in case the user wants to use all chips, we create a temporary
	    // set, fill that with all chip ints, set input to "done" and 
	    // set chip_set to the temp set (temp set used to prevent problems, if 
	    // user first selects some chips individually and then all)
	    std::set<int> tempSet;
	    for(int i = 0; i < pc->fpga->tp->GetNumChips(); i++) tempSet.insert(i+1);
	    input = "done";
	    chip_set = tempSet;
	}
	else if (input != "done"){
	    chip_set.insert(std::stoi(input));
	}
    } while (input != "done");

    return chip_set;
}

std::string Console::TriggerSelection(){
    // this function is called to provide the user input interface
    // to select, whether to use an external trigger or not
    // function returns:
    //     "noexternal" no external trigger
    //     "external"   external trigger
    //     "quit"       calling function is supposed to quit
    
    std::string input;
    std::set<std::string> allowedTriggerStrings = {"noexternal", "noext", "0",
						   "external",   "ext",   "1"};
    std::cout << "Please select if you wish to use an external trigger:\n"
	      << "no external trigger: type { noexternal, noext, 0 }\n"
	      << "externa trigger:     type { external,   ext,   1 }"
	      << std::endl;

    input = getUserInputNonNumericalNoDefault(_prompt, &allowedTriggerStrings);
    if (input == "quit") return "quit";
    else if ( (input == "standard") ||
	      (input == "std")      ||
	      (input == "0") ){
	// return standard, no external trigger
	return "noexternal";
    }
    else{
	return "external";
    }
}

std::string Console::FastClockSelection(){
    // this function is called to provide the user input interface 
    // to select whether to work with or without fastclock
    // function returns:
    //     "fastclock"   use fastclock
    //     "nofastclock" do not use fastclock
    //     "quit"        quit function, which calls this function
    std::string input;
    
    std::cout << "Do you wish to work with or without fastclock?\n"
	      << "Standard:  type { standard,  std , s, 0 }\n"
	      << "Fastclock: type { fastclock, fast, f, 1 }"
	      << std::endl;
    std::set<std::string> allowedClockStrings = {"standard",   "std", "s", "0",
						 "fastclock", "fast", "f", "1"};
    input = getUserInputNonNumericalNoDefault(_prompt, &allowedClockStrings);
    if (input == "quit") return "quit";
    else if ( (input == "standard") ||
	      (input == "std")      ||
	      (input == "s")        ||
	      (input == "0") ){
	// activate fast clock
	return "nofastclock";
    }
    else{
	// in this case the input is a use fastclock input
	return "fastclock";
    }

    return "ERROR";
}

std::string Console::ShutterRangeSelection(){
    // this function is called to provide the user input interface
    // to select which shutter range to use
    // function returns:
    //     "standard" standard shutter length
    //     "long"     long shutter length
    //     "verylong" very long shutter length
    //     "quit"     calling function is supposed to quit
    std::string inputMode;

    std::cout << "Please choose the shutter range (choose one option from {} ):\n" 
	      << "Standard  - n = 0 : type { standard, std, 0 } range:   1.15 µs - 293.25 µs\n"
	      << "Long      - n = 1 : type { long,     l  , 1 } range: 294.40 µs -  75.07 ms\n"
	      << "Very long - n = 2 : type { verylong, vl , 2 } range:  75.37 ms -  19.22 s\n"
	      << "Shutter range is calculated by:\n"
	      << "time [µsec] = (256)^n*46*x/freq[MHz], x of {1, 255}, freq 40 MHz (independent of clock!)"
	      << std::endl;
    // create set of allowed strings
    std::set<std::string> allowedStrings = {"standard", "std", "0",
					    "long",     "l",   "1",
					    "verylong", "vl",  "2"};

    // use default prompt. Options explained in cout before
    inputMode = getUserInputNonNumericalNoDefault(_prompt, &allowedStrings);
    if (inputMode == "quit"){
	return "quit";
    }
    else if( (inputMode == "standard") ||
	     (inputMode == "std"     ) ||
	     (inputMode == "0"       )){
	return "standard";
    }
    else if( (inputMode == "long") ||
	     (inputMode == "l"   ) ||
	     (inputMode == "1"   ) ){
	return "long";
    }
    else if( (inputMode == "verylong") ||
	     (inputMode == "vl"      ) ||
	     (inputMode == "2"       ) ){
	return "verylong";
    }
    return "ERROR";
}

std::string Console::ShutterTimeSelection(int n){
    // this function is called to provide the user input interface
    // to select the time for the selected shutter range
    // int n: the power to which 256 is raised in shutter time, i.e. the shutter range:
    //        n = 0: standard
    //        n = 1: long
    //        n = 2: verylong
    // function returns:
    //     string in range 1 to 255 (needs to be converted to int with std::stoi
    //     "quit" calling function is supposed to quit

    // get user input to get valid value of x in range of 1 to 255
    std::string inputTime;
    std::set<std::string> allowedTimeStrings;
    // fill this set with all values from 1 to 255
    for( int l=1; l<256; l++) allowedTimeStrings.insert(std::to_string(l));
    // with n already selected, we can now provide the correct formula to 
    // calculate shutter range
    if (n == 0){
	std::cout << "Choose a time value from 1 to 255:\n"
		  << "Shutter range is calculated by:\n"
		  << "time [µsec] = 46*x/freq[MHz], x of {1, 255}, freq 40 MHz (independent of clock!)"
		  << std::endl;
    }
    else if (n == 1){
	std::cout << "Choose a time value from 1 to 255:\n"
		  << "Shutter range is calculated by:\n"
		  << "time [µsec] = 256*46*x/freq[MHz], x of {1, 255}, freq 40 MHz (independent of clock!)"
		  << std::endl;
    }
    else if (n == 2){
	std::cout << "Choose a time value from 1 to 255:\n"
		  << "Shutter range is calculated by:\n"
		  << "time [µsec] = 256^2*46*x/freq[MHz], x of {1, 255}, freq 40 MHz (independent of clock!)"
		  << std::endl;
    }
    inputTime = getUserInputNumericalNoDefault(_prompt, &allowedTimeStrings);
    if (inputTime == "quit") {
	return "quit";
    }
    // inputTime should now be a valid string for an integer between 1 and 255
    int shutterTime = std::pow(256, n)*46*std::stoi(inputTime) / 40;
    if (shutterTime > 1000000){
	// convert µs to seconds
	shutterTime /= 1000000;
	std::cout << "Shutter time of " << shutterTime << " s was selected." << std::endl;
    }
    else if (shutterTime > 1000){
	shutterTime /= 1000;
	std::cout << "Shutter time of " << shutterTime << " ms was selected." << std::endl;
    }
    else{
	std::cout << "Shutter time of " << shutterTime << " µs was selected." << std::endl;
    }
    
    return inputTime;
}

int Console::ShutterRangeToMode(std::string shutter_range){
    // this function converts the shutter range (string of type) to 
    // the mode n
    // done by calling the corresponding function, which is part of
    // the FPGA class
    int n;
    n = pc->fpga->ShutterRangeToMode(shutter_range);
    
    return n;
}

std::string Console::ShutterTimeSelection(std::string shutter_range){
    // this is an overload for the ShutterTimeSelection, so that one can also
    // hand the shutter range as a string instead of the mode n. we simply convert 
    // the string here and call the correct function
    int n;
    n = ShutterRangeToMode(shutter_range);

    return ShutterTimeSelection(n);
}

std::string Console::CalibrationSelection(){
    // this function handles user input in regards to selecting TOT or TOA 
    // calibration
    // function returns:
    //     "TOT" for TOT calibration
    //     "TOA" for TOA calibration
    std::string input;
    std::string output;
    
    std::cout << "Perform TOT or TOA calibration?" << std::endl;
    std::cout << "TOT : type { 0, TOT }\n" 
	      << "TOA : type { 1, TOA }" << std::endl;
    
    input = getUserInputNonNumericalNoDefault(_prompt, {"0", "TOT", "1", "TOA"});
    if(input == "quit"){ return input; }
    else if( (input == "0") ||
	     (input == "TOT")){ 
	output = "TOT";
    }
    else if( (input == "1") ||
	     (input == "TOA")){ 
	output = "TOA";
    }

    return output;
}

std::string Console::PulserSelection(){
    // this function handles user input in regards to pulser selection
    // i.e. internal or external pulser
    // function returns:
    //     "internal" internal pulser
    //     "external" external pulser
    std::string input;
    std::string output;

    std::cout << "Choose between internal and external pulser:" << std::endl;
    std::cout << "Internal : type { 0, internal, int }\n"
	      << "External : type { 1, external, ext }" << std::endl;
    input = getUserInputNonNumericalNoDefault(_prompt, 
					      {"0", "internal", "int", "1", "external", "ext"});
    if(input == "quit"){ return input; }
    else if( (input == "0") ||
	     (input == "internal") ||
	     (input == "int") ){
	output = "internal";
    }
    else if( (input == "1") ||
	     (input == "external") ||
	     (input == "ext") ){
	output = "external";
    }

    return output;
}

std::list<int> Console::PulseListCreator(std::string pulser){
    // this function provides the user input interface to create a list of 
    // pulses, which are to be used for TO calibration
    std::list<int> pulseList;
    std::string input;
    std::set<std::string> allowedStrings;

    // first we need to check, whether we're using an external pulser
    // or the internal one
    if( pulser == "external" ){
	std::cout << "Enter the value of the external pulser in mV: "
		  << std::endl;
	input = getUserInputNumericalNoDefault(_prompt);
	if( input == "quit" ) return { -1 };
	else{
	    pulseList.push_back(std::stoi(input));
	}
    }
    else{
	// in this case we use the internal pulser

	std::cout << "create a list of pulses in mV to be used.\n"
		  << "add one element at a time.\n"
		  << "once you're done, type { done }\n"
		  << "at any time type { default } to use the following pulses: "
		  << std::endl;
	// create list of default values:
	std::list<int> defaultList;
	defaultList.push_back(20);
	defaultList.push_back(30);
	defaultList.push_back(40);
	defaultList.push_back(50);
	defaultList.push_back(60);
	defaultList.push_back(70);
	defaultList.push_back(80);
	defaultList.push_back(90);
	defaultList.push_back(100);
	defaultList.push_back(150);
	defaultList.push_back(200);
	defaultList.push_back(250);
	defaultList.push_back(350);
	defaultList.push_back(450);
	std::for_each( defaultList.begin(), defaultList.end(), [](int pulse){
		std::cout << pulse << ", " << std::flush;
	    } );
	std::cout << std::endl;

	// first we will fill the allowed strings set
	// we allow all pulse heights from 0 to 1000mV
	// this is the value which is added to the 350mV baseline
	for(int i = 0; i < 1000; i++) allowedStrings.insert(std::to_string(i));
	// aside from this, we can also input "default", which will then load the
	// default list
	allowedStrings.insert("default");
	// and add "done" to finish adding
	allowedStrings.insert("done");
    
    
	do{
	    // print current list content
	    std::cout << "list contains: " << std::flush;
	    if( pulseList.begin() == pulseList.end() ){
		std::cout << "empty" << std::endl;
	    }
	    else{
		std::cout << std::endl;
		// print current list
		std::for_each( pulseList.begin(), pulseList.end(), [](int pulse){
			std::cout << pulse << ", " << std::flush;
		    } );
		std::cout << std::endl;
		std::cout << "to finish adding voltages, type { done }" << std::endl;
	    }

	    input = getUserInputNonNumericalNoDefault(_prompt, &allowedStrings);
	    if( input == "quit" ) return { -1 };
	    else if( input == "default" ){
		// in this case overwrite pulseList with defaultList
		input = "done";
		pulseList = defaultList;
	    }
	    else if( input != "done" ){
		pulseList.push_back(std::stoi(input));
	    }
	} while (input != "done");
    }
    
    return pulseList;
}

int Console::PixPerColumnSelection(){
    // this function handles user input in regards to the pixels per columns
    // to use, i.e. the spacing that is used during TO calibration
    // function returns:
    //     <int> corresponding to active rows for one frame
    std::string input;
    int output;

    std::cout << "Choose the number of active pixels per column,\n:" 
	      << "i.e. the number of active rows per event" << std::endl;
    std::cout << "allowed values = { 1, 2, 4, 8, 16 }"
	      << std::endl;
    input = getUserInputNumericalNoDefault(_prompt, 
					   {"1", "2", "4", "8", "16"});
    if(input == "quit"){ return -1; }
    else{
	output = std::stoi(input);
    }

    return output;
}


int Console::CommandCountingTrigger(){
    // default CommandCountingTrigger function
    // this function is used to set the time the shutter opens after 
    // a trigger from an external trigger
#if DEBUG==2
    std::cout<<"Enter Console::CommandCountingTrigger()" << std::endl;	
#endif
    int result=0;
    unsigned int time;
    std::string inputClock;
    std::string inputTime;

    // call FastClockSelection to open user input interface for fast clock selection
    inputClock = FastClockSelection();
    if (inputClock == "quit") return -1;
    else if ( inputClock == "fastclock" ){
	// activate fast clock
	pc->fpga->UseFastClock(1);
    }
    else{
	// in this case the input is a use fastclock input
	pc->fpga->UseFastClock(0);
    }

    // 
    // get user input to get valid value of x in range of 1 to 255
    // for this call ShutterTimeSelection() to get user input interface
    // in case we use an external trigger, only the standard shutter range is allowed
    inputTime = ShutterTimeSelection(0);
    if (inputTime == "quit"){
	// regardless of used option, turn off fast clock to make sure
	pc->fpga->UseFastClock(0);
	return -1;
    }
    time = std::stoi(inputTime);
    result=pc->fpga->CountingTrigger(time);
    if (result > 20){
	ErrorMessages(result);
    }
    else{
	std::cout << "\tCountingTrigger accomplished\n" << std::flush;
    }
    // regardless of the used option, turn off fast clock
    pc->fpga->UseFastClock(0);
    return 1;
}

int Console::CommandCountingTime(){
    // CommandCountingTime function, dealing with all different 'multiplier' cases 
    // (standard, long, verylong)
#if DEBUG==2
    std::cout<<"Enter Console::CommandCountingTime() " << time <<std::endl;	
#endif
    // use getUserInput to get the desired mode first
    std::string inputClock;
    std::string inputMode;
    std::string inputTime;
    int n;

    // call FastClockSelection to open user input interface for fast clock selection
    inputClock = FastClockSelection();
    if (inputClock == "quit") return -1;
    else if ( inputClock == "fastclock" ){
	// activate fast clock
	pc->fpga->UseFastClock(1);
    }
    else{
	// in this case the input is a use fastclock input
	pc->fpga->UseFastClock(0);
    }

    // now choose shutter range
    // for this call ShutterRangeSelection() to call user input interface
    inputMode = ShutterRangeSelection();
    if (inputMode == "quit"){
	// regardless of used option, turn off fast clock to make sure
	pc->fpga->UseFastClock(0);
	return -1;
    }
    else if( (inputMode == "standard") ){
	n = 0;
    }
    else if( (inputMode == "long") ){
	n = 1;
    }
    else{
	// since ShutterRangeSelection makes sure only specific input is allowed,
	// the only other option is verylong. So set n = 2
	n = 2;
    }
    // now we should have a valid n of {0, 1, 2}
    // get user input to get valid value of x in range of 1 to 255
    // for this call ShutterTimeSelection() to get user input interface
    inputTime = ShutterTimeSelection(n);
    if (inputTime == "quit") {
	// regardless of used option, turn off fast clock to make sure
	pc->fpga->UseFastClock(0);
	return -1;
    }
    // inputTime should now be a valid string for an integer between 1 and 255
    // instead of calling different fpga->CountingTime functions, we
    // will now hand a flag and a time to the normal CountingTime function

    int result;
    if (_hvFadcManagerActive == true){
	// enable fadc shutter
	pc->fpga->EnableFADCshutter(1);
	
        std::vector<std::thread> threads;

        threads.push_back(std::thread(&hvFadcManager::F_StartAcquisition, this->_hvFadcManager));

        int (FPGA::*CountingTime)(int, int) = &FPGA::CountingTime;
        threads.push_back(std::thread(CountingTime, this->pc->fpga, std::stoi(inputTime), n));

        for(auto& thread : threads){
            thread.join();
        }
        result = 0;
	pc->fpga->EnableFADCshutter(0);
    }
    else{
	result = pc->fpga->CountingTime(std::stoi(inputTime), n);
    }
    // before we open the shutter, we check if the FADC object is used.
    // if (_hvFadcManagerActive == true){
    // 	// if this is the case start FADC acquisition
    // 	std::cout << "HFM active, starting FADC data acquisition." << std::endl;
    // 	_hvFadcManager->F_StartAcquisition();
    // 	// also enable FADC shutter (not yet implemented)
    // 	// currently using software trigger after shutter is closed
    // }

    // int result;
    // result = pc->fpga->CountingTime(std::stoi(inputTime), n);
    if (result>20){ 
	ErrorMessages(result); 
    }
    else{
	std::cout << "\tCountingTime accomplished\n" << std::flush;
    }
    
    // now send software trigger to FADC to close frame
    if (_hvFadcManagerActive == true){
	std::cout << "sending software trigger..." << std::endl;
	_hvFadcManager->F_SendSoftwareTrigger();

    }

    // deactivate fastclock again
    pc->fpga->UseFastClock(0);
    return 1;
}

int Console::CommandReadOut(){
#if DEBUG==2
    std::cout << "Enter Console::CommandReadOut()"  <<  std::endl;	
#endif
    int result;
    std::string filename[9]= {""};
    std::string f[9];

    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	// filename[chip]=pc->GetDataPathName();
	// filename[chip]+="/";
	filename[chip] = pc->GetDataFileName(chip);
	f[chip] = filename[chip].c_str();
    }
    result = pc->DoReadOut(f);
    std::cout << "CommandReadOut:" << result << std::endl;
    if(result<0){ErrorMessages(-result);}
    else{std::cout << "\tReadOut accomplished\n" << std::flush;}
    return result;
}


int Console::CommandReadOut2(){
#if DEBUG==2
    std::cout << "Enter Console::CommandReadOut()" << std::endl;
#endif
    int result = 0;
    pc->fpga->DataChipFPGA(result);

    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	std::string filename;//=pc->GetDataPathName();
//	filename+="/";
	filename = pc->GetDataFileName(chip);

	// now check whether we're using the FADC
	if (_hvFadcManagerActive == true){
	    // in this case use 
	    result   = pc->DoReadOutFadc(filename, chip);
	}
	else{
	    result   = pc->DoReadOut2(filename, chip);
	}
#if DEBUG==2
	std::cout << "DEBUG: Filename: " << filename << std::endl;
#endif    
	std::cout<<"CommandReadOut:"<< result <<" for chip "<< chip << std::endl;
	if(result<0){ErrorMessages(-result);}
	else{std::cout<<"\tReadOut accomplished\n"<<std::flush;}
	//filename = "";
    }
    return result;
}


int Console::CommandSetMatrix(){
#if DEBUG==2
    std::cout<<"Enter Console::CommandSetMatrix()"<<std::endl;	
#endif
    int result;
    result=pc->fpga->SetMatrix();
    if(result>40){ErrorMessages(result);}
    else{std::cout<<"\tSetMatrix accomplished\n"<<std::flush;}
    return result;
}

int Console::CommandSaveMatrix(){
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = false;

    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	std::string ein;
	std::string f=pc->GetMatrixFileName(chip);
	std::cout << "Matrix filename for chip "
		  << chip 
		  << ": (press ENTER to save in "
		  << pc->GetMatrixFileName(chip) 
		  << "): " 
		  << std::endl;
	ein = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (ein == "quit") return -1;
	f=ein.c_str();
	pc->fpga->tp->SaveMatrixToFile(f,chip);
	std::cout<<"Matrix saved to "<<f<<"\n"<<std::flush;
    }
    return 0;
}


int Console::CommandLoadMatrix() {
    for (unsigned short chip_id = 1; chip_id <= pc->fpga->tp->GetNumChips(); chip_id++){
	    std::string default_path = pc->GetMatrixFileName(chip_id);

        std::cout << "Matrix filename for chip "
            << chip_id
            << " (press ENTER to load from "
            << default_path
            << "): "
            << std::endl;

        std::string filename;
        if (!getUserInputOrDefaultFile(_prompt, default_path, filename))
            return -1;

	    pc->fpga->tp->LoadMatrixFromFile(filename, chip_id);
	    std::cout << "Matrix loaded from " << filename << std::endl << std::flush;

	    pc->fpga->tp->SaveMatrixToFile(default_path, chip_id);
	    std::cout << "Matrix saved to program folder as " << default_path << std::endl << std::flush;
    }
    return 0;
}

int Console::CommandWriteReadFSR(){
#if DEBUG==2
    std::cout<<"Enter Console::CommandWriteReadFSR()\n"<<std::flush;
#endif
    int result;
    result=pc->fpga->WriteReadFSR();
    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	pc->fpga->tp->GetChipID(chip);
    }
    std::cout<<"\tWriteReadFSR accomplished\n"<<std::flush;
    return result;
}


int Console::CommandSaveFSR(){
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = false;

    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	std::string ein;
	std::string f=pc->GetFSRFileName(chip);
	std::cout << "FSR filename for chip "
		  << chip
		  << ": (press ENTER to save in " 
		  << pc->GetFSRFileName(chip) 
		  << "): " 
		  << std::endl;
	ein = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (ein == "quit") return -1;
	f=ein.c_str();
	pc->fpga->tp->SaveFSRToFile(f,chip);
	std::cout<<"FSR saved in "<<f<<"\n"<<std::flush;
    }
    return 0;
}


int Console::CommandLoadFSR(){
#if DEBUG==2
    std::cout<<"Enter Console::CommandLoadFSR()"<<std::endl;	
#endif	
    int err = 0;
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = true;

    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	std::string ein;
	std::string f=pc->GetFSRFileName(chip);
	std::cout << "FSR filename for chip "
		  << chip
		  << " (press ENTER to load from "
		  << pc->GetFSRFileName(chip) 
		  << "): " 
		  << std::endl;

	ein = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (ein == "quit") return -1;

	if(ein==""){
	    // per default we wish to load the fsr.txt file
	    f = pc->GetFSRFileName(chip);
	}
	else{
	    f=ein.c_str();
	}

	std::cout << "Trying to load file: " << f << std::endl;
	FILE* f1=fopen(f.c_str(),"r"); 
	if (f1 == NULL) {
	    std::cout << "File not found"
		      << std::endl; 
	    return -1;
	}
	if (f1 != NULL) {
	    err=pc->fpga->tp->LoadFSRFromFile(f,chip);
	    if(err==1){
		std::cout << "FSR loaded from " << f << "\n" << std::flush;
	    }
	    else{
		std::cout << "Error in " << f << " in row " << -err << "\n" 
			  << std::flush;
	    }
	}
	pc->fpga->tp->SaveFSRToFile(pc->GetFSRFileName(chip),chip);
	std::cout << "FSR saved to program folder as " << pc->GetFSRFileName(chip) << "\n" << std::flush;
    }
    return err;
}

int Console::CommandLoadFSRAll(){
    // this function simply loads a fsr file for all chips
    // on the default file names
    int err = 0;
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = true;

    std::string input;
    std::string f=pc->GetFSRFileName(1);
    std::cout << "Enter a FSR filename to be loaded for all chips.\n"
	      << "(press ENTER to load default " << f  
	      << std::endl;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    if(input==""){
	// per default we wish to load the fsr.txt file
	f = pc->GetFSRFileName(1);
    }
    else{
	f = input.c_str();
    }
    std::cout << "Trying to load file: " << f << std::endl;
    FILE* f1 = fopen(f.c_str(), "r"); 
    if (f1 == NULL) {
	std::cout << "File not found"
		  << std::endl; 
	return -1;
    }
    else{
	for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	    err=pc->fpga->tp->LoadFSRFromFile(f,chip);
	    if(err==1){
		std::cout << "FSR loaded from " << f << "\n" << std::flush;
	    }
	    else{
		std::cout << "Error in " << f << " in row " << -err << "\n" 
			  << std::flush;
	    }
	pc->fpga->tp->SaveFSRToFile(pc->GetFSRFileName(chip),chip);
	std::cout << "FSR saved to program folder as " << pc->GetFSRFileName(chip) << "\n" << std::flush;
	}
    }
    return err;
}


int Console::CommandLoadThreshold(){
#if DEBUG==2
    std::cout << "Enter Console::CommandLoadFSR()" << std::endl;	
#endif	
    int err = 0;
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = true;

    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	std::string ein;
	std::string f=pc->GetThresholdFileName(chip);
	std::cout << "Threshold filename for chip "
		  << chip
		  << " (press ENTER to load from "
		  << pc->GetThresholdFileName(chip) 
		  << "): " 
		  << std::endl;
	ein = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (ein == "quit") return -1;
	if(ein==""){
	    // per default we wish to load the fsr.txt file
	    f = pc->GetThresholdFileName(chip);
	}
	else{
	    f=ein.c_str();
	}
	FILE* f1=fopen(f.c_str(),"r");
	if(f1==NULL) {
	    std::cout << "File not found" << std::endl;
	    return -1;
	}
	if (f1 != NULL) {
	    err=pc->fpga->tp->LoadThresholdFromFile(f,chip);
	    if(err==-1){ 
		std::cout << "File " << f << " not found" << std::endl;
	    }
	    std::cout << "Threshold loaded from " << f << "\n" << std::flush;
	}
	pc->fpga->tp->SaveThresholdToFile(pc->GetThresholdFileName(chip),chip);
	std::cout<<"Threshold saved to program folder as "<<pc->GetThresholdFileName(chip)<<"\n"<<std::flush;
    }
    return err;
}


int Console::CommandSetDAC(){
#if DEBUG==2
    std::cout<<"Enter Console::CommandSetDAC()"<<std::endl;	
#endif	
    std::string  input;
    const char *promptDAC = "Please choose a DAC to set (#0 to #13)> ";
    input = getUserInputNumericalNoDefault(promptDAC);
    if (input == "quit") return -1;
    int dac = std::stoi(input);

    const char *promptChip = "Please choose a chip for which to set DAC> ";
    input = getUserInputNumericalNoDefault(promptChip);
    if (input == "quit") return -1;
    unsigned short chip = std::stoi(input);

    const char *promptValue = "Please provide the value to set> ";
    input = getUserInputNumericalNoDefault(promptValue);
    if (input == "quit") return -1;
    int i = std::stoi(input);
    int err = 1;

    if (chip > pc->fpga->tp->GetNumChips()){ 
	std::cout << "You only have " 
		  << pc->fpga->tp->GetNumChips() 
		  << " chips, please provide correct chip number." 
		  << std::endl;
	err=-3;
    }
    else err=pc->fpga->tp->SetDAC(dac, chip, i);
    if(err!=1) ErrorMessages(61-err);
    else std::cout << "DAC " << dac 
		   << " (" << pc->fpga->tp->GetDACName(dac) << ") of chip " << chip 
		   << " set to " << i 
		   << std::endl;
    return 1;
}


int Console::CommandShowFSR(){
    for (unsigned short chip = 1; chip <= pc->fpga->tp->GetNumChips(); chip++){
	std::string chip_name;
	chip_name = pc->fpga->tp->GetChipName(chip);
	std::cout << "current FSR for chip #" << chip 
		  << " with name " << chip_name
		  << std::endl;
	for(unsigned int i=0;i<18;i++){
	    std::cout << "\t" << pc->fpga->tp->GetDACName(i) << " \t " << pc->fpga->tp->GetDAC(i,chip) << std::endl;
	}
    }
    return 0;
}


int Console::CommandVarChessMatrix(){
#if DEBUG==2
    std::cout<<"Enter Console::CommandSenseDAC()"<<std::endl;	
#endif
    int sl,wl;
    int err = 0;
    int sp0,sp1,stest,smask,sth;
    int wp0,wp1,wtest,wmask,wth;

    // variables for getUserInput
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;
    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	std::cout<<"Chip Number "<<chip<<std::endl;
	std::cout<<"\t Number cols per field="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	sl = std::stoi(input);
	std::cout<<"\t Number row per field="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	wl = std::stoi(input);
	std::cout<<"\t Black P0="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	sp0 = std::stoi(input);
	std::cout<<"\t Black P1="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	sp1 = std::stoi(input);
	std::cout<<"\t Black Mask="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	smask = std::stoi(input);
	std::cout<<"\t Black Test="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	stest = std::stoi(input);
	std::cout<<"\t Black Threshold="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	sth = std::stoi(input);
	std::cout<<"\t White P0="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	wp0 = std::stoi(input);
	std::cout<<"\t White P1="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	wp1 = std::stoi(input);
	std::cout<<"\t White Mask="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	wmask = std::stoi(input);
	std::cout<<"\t White Test="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	wtest = std::stoi(input);
	std::cout<<"\t White Threshold="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	wth = std::stoi(input);
	err=pc->fpga->tp->VarChessMatrix(sl,wl,sp0,sp1,smask,stest,sth,wp0,wp1,wmask,wtest,wth,chip);
	if(err==0){std::cout<<"Matrix created\n"<<std::flush;}
	else{ErrorMessages(80);}
    }// end for loop
    return err;
}

int Console::CommandUniformMatrix(){
    int p0,p1,test,mask,th;
    int err = 0;

    // variables for getUserInput
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;

    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	std::cout<<"Chip Number "<<chip<<std::endl;
	std::cout<<"\t P0="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	p0 = std::stoi(input);
	std::cout<<"\t P1="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	p1 = std::stoi(input);
	std::cout<<"\t Mask="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	mask = std::stoi(input);
	std::cout<<"\t Test="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	test = std::stoi(input);
	std::cout<<"\t Threshold="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	th = std::stoi(input);
	err=pc->fpga->tp->UniformMatrix(p0,p1,mask,test,th,chip);
	if(err==0){std::cout<<"Matrix created\n"<<std::flush;}
	else{ErrorMessages(80);}
    }

    return err;	
}

int Console::CommandUniformMatrixAllChips(){
    // quick and dirty implementation to set same uniform matrix
    // for all chips
    // TODO: MAKE NEW AND NICE WITH USER INTERFACE FUNCTION FOR BOTH
    // UNIFORM MATRIX FUNCTIONS
    int p0,p1,test,mask,th;
    int err = 0;

    // variables for getUserInput
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;
    
    std::cout << "This matrix will be set for all active chips." << std::endl;
    std::cout<<"\t P0="<<std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    p0 = std::stoi(input);
    std::cout<<"\t P1="<<std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    p1 = std::stoi(input);
    std::cout<<"\t Mask="<<std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    mask = std::stoi(input);
    std::cout<<"\t Test="<<std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    test = std::stoi(input);
    std::cout<<"\t Threshold="<<std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    th = std::stoi(input);
    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	err += pc->fpga->tp->UniformMatrix(p0,p1,mask,test,th,chip);
    }
    if(err==0){std::cout<<"Matrix created\n"<<std::flush;}
    else{ErrorMessages(80);}


    return err;	
}

int Console::CommandFADCshutter(){
    // variables for getUserInput
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;

    std::cout << "How many clock cycles after FADC trigger the shutter should be closed "
	      << "(1-2 clock cycles delay in firmware) ? (1 to 16777215, 0 leads to failure)"
	      << std::endl;
    std::cout << "WARNING: don't put more clock cycles than shutter is long." << std::endl;
    int closeshutter = 0;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    closeshutter = std::stoi(input);
    pc->fpga->tp->SetI2C(closeshutter);
    pc->fpga->EnableFADCshutter(1);
    return 0;
}


int Console::CommandDACScan(){
#if DEBUG==2
    std::cout<<"Enter Console::CommandDACScan()"<<std::endl;
#endif
    // variables for getUserInput
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;

    int DacsOn[14]={1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    unsigned short chip;
    std::cout << "Which chip do you want to DAC scan? (1-" << pc->fpga->tp->GetNumChips() << ") " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    chip = std::stoi(input);
    std::cout << "Choose DACs to scan (1=Yes, 0=No) \n" << std::flush;
    std::cout << "\t IKrum " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[0] = std::stoi(input);
    std::cout << "\t Disc " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[1] = std::stoi(input);
    std::cout << "\t Preamp " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[2] = std::stoi(input);
    std::cout << "\t BuffAnalogA " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[3] = std::stoi(input);
    std::cout << "\t BuffAnalogB " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[4] = std::stoi(input);
    std::cout << "\t Hist " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[5] = std::stoi(input);
    std::cout << "\t THL " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[6] = std::stoi(input);
    std::cout << "\t Vcas " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[7] = std::stoi(input);
    std::cout << "\t FBK " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[8] = std::stoi(input);
    std::cout << "\t GND " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[9] = std::stoi(input);
    std::cout << "\t THS " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[10] = std::stoi(input);
    std::cout << "\t BiasLVDS " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[11] = std::stoi(input);
    std::cout << "\t RefLVDS " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[12] = std::stoi(input);
    std::cout << "\t Coarse " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DacsOn[13] = std::stoi(input);
    std::cin.ignore(1);

    int DACstoScan = 0;
    for(int i=0;i<14;i++){
	if (DacsOn[i]){
	    DACstoScan+=1 << i;
	}
	else {
	    DACstoScan+=0 << i;
	}
    }

    pc->DoDACScan(DACstoScan, chip);
    return 0;

}


int Console::CommandTHLScan(){
    // variables for getUserInput
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;

    unsigned short coarselow;
    unsigned short coarsehigh;
    std::cout << "Which chip do you want to THL scan? (1-"
	      << pc->fpga->tp->GetNumChips() << ") \n"
	      << "0 for all chips." << std::endl;

    std::string inputChips;
    inputChips = getUserInputNonNumericalDefault(_prompt);
    if(inputChips == "quit") return -1;
    std::cout << "Start coarse " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    coarselow = std::stoi(input);
    std::cout << "End coarse " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    coarsehigh = std::stoi(input);

    
    if (inputChips == "0"){
	// in this case perform for all chips
	for( int chip = 0; chip < pc->fpga->tp->GetNumChips(); chip++){
	    
	    pc->DoTHLScan(chip + 1, coarselow, coarsehigh);	    
	}
    }
    else{
	unsigned short chip;
	chip = std::stoi(inputChips);
	pc->DoTHLScan(chip, coarselow, coarsehigh);
    }
    

    return 0;
}

int Console::CommandSCurve(){
    // variables for getUserInput
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;

    unsigned short voltage = 0;
    unsigned short offset = 0;
    int time = 255;
    unsigned short StartTHL[9] = {0};
    unsigned short StopTHL[9] = {1023};
    std::cout << "Hello, this is fast S-Curve scan. I will do a THL scan and "
	      << "count how many counts there are in average on the chips. "
	      << "Scan will be done for one chip after the other from chip 1 to " 
	      <<  pc->fpga->tp->GetNumChips() 
	      << ") " 
	      << std::flush;
    std::cout << "Warning: Only CTPR = 1 will be used. Hence only column x = 0, "
	      << "x= 32, ... Make sure that NONE of these columns are dead. "
	      << "Otherwise put a column offset ( 0(no offset) to 31)" 
	      << std::flush;
    input      = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    offset     = std::stoi(input);
    std::cout << "What voltage did you set on pulser, put 0 for internal pulser?" 
	      << std::flush;
    input      = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    voltage    = std::stoi(input);
    std::cout << "Shutter time in clock cycles (0-255), LONG mode used "
	      << "(100 is ok for internal pulser) " 
	      << std::flush;
    input      = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    time       = std::stoi(input);
    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	std::cout << "Start (lower) THL for chip " << chip << " " << std::flush;
	input          = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	StartTHL[chip] = std::stoi(input);
	std::cout << "End (upper) THL for chip " << chip << " " << std::flush;
	input          = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	if (input == "quit") return -1;
	StopTHL[chip]  = std::stoi(input);
    }
    pc->DoSCurveScan(voltage,time,StartTHL,StopTHL,offset);
    return 0;
}


int Console::Commandi2creset(){
    pc->fpga->i2creset();
    return 0;
}


int Console::Commandi2cDAC(){
    unsigned short Umv = 0;
    unsigned short DACchannel = 0;
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;
    const char *promptDACchannel = "I2C DAC channel (0=not used,1=Ext_DAC,2=MUX low,3=MUX high): ";
    input      = getUserInput(promptDACchannel, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    DACchannel = std::stoi(input);
    const char *promptDACvoltage = "I2C DAC voltage (mV): ";
    input = getUserInput(promptDACvoltage, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    Umv   = std::stoi(input);
    
    pc->fpga->i2cDAC(Umv, DACchannel);
    return 0;
}


int Console::Commandi2cADC(){
    unsigned short channel = 0;
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;
    const char *promptADCchannel = "ADC channel to read: ";
    input = getUserInput(promptADCchannel, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    std::cout << "ADC channel is: " << channel << std::endl;
    pc->fpga->i2cADC(channel);
    return 0;
}


int Console::CommandTpulse(){
    unsigned short Npulses = 1;
    unsigned short div500kHz = 1;
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;
    const char *promptNumTestPulses = "Number of testpulses (1 - 5000): ";
    input   = getUserInput(promptNumTestPulses, numericalInput, allowDefaultOnEmptyInput);
    if (input == "quit") return -1;
    Npulses = std::stoi(input);
    const char *promptFreqTestPulse = "Testpulse frequency: Divide 500 kHz by (1 - 50): ";
    input     = getUserInput(promptFreqTestPulse, numericalInput, allowDefaultOnEmptyInput);    
    if (input == "quit") return -1;
    div500kHz = std::stoi(input);
    pc->fpga->tpulse(Npulses, div500kHz);
    return 0;
}

void Console::runTestPulses(){
    // this function is ran in a second thread (called in CommandTestTPulse) and simply
    // calls the tpulse function over and over again, as to have a tpulses being send
    // all the time, until the user types stop on the terminal
    while(_loop_stop == false){
	pc->fpga->tpulse(1000, 10);
	//std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }    
}

int Console::CommandTestTPulse(){
    // this function is simply used to test, whether the test pulses properly work
    // and the i2cDACs are properly set. To some extend similar to a mix of Commandi2cDAC and
    // CommandTpulse (as far as I understand!!)
    
    // first handle user input
    std::cout << "Please enter an upper (non-offset!) value for the i2cDAC in mV. Choose from x = {350, 2200}"
	      << std::endl;

    std::set<std::string> allowedValues;
    for( int l=350; l<2201; l++) allowedValues.insert(std::to_string(l));
    std::string input;
    input = getUserInputNumericalNoDefault(_prompt, &allowedValues);
    if (input == "quit") return -1;
    std::cout << input << " mV was chosen as upper bound." << std::endl;

    // now handle setting the DAC
    pc->fpga->i2cDAC(std::stoi(input), 2);
    pc->fpga->i2cDAC(350, 3);
    
    pc->fpga->EnableTPulse(1);
    // now loop over fpga->tpulse to pulse..
    // create seperate thread, which loops and will be stopped, if we type stop in terminal
    std::thread loop_thread(&Console::runTestPulses, this);
    _loop_stop = true;
    //loop_thread.start();
    const char *waitingPrompt = "test pulses running. type 'stop' to quit> ";
    std::set<std::string> allowedStrings = {"stop"};
    input = getUserInputNonNumericalNoDefault(waitingPrompt, &allowedStrings);
    if (input == "stop"){
	_loop_stop = false;
    }
    loop_thread.join();
    pc->fpga->EnableTPulse(0);
    
    return 0;
}

int Console::CommandDoTHSopt(){

    unsigned short doTHeq = 0;
    unsigned short pix_per_row_THeq = 1;
    unsigned short chp = 1;

    std::string ein0="";
    std::cout << "Hello, this is THS optimisation.  You have " 
	      << pc->fpga->tp->GetNumChips() 
	      << " chips, for which of them do you want to do the optimization? "
	      << "Put 0 if you want to go for all of them one after the other. " 
	      << std::endl;
    ein0 = getUserInput(_prompt);
    if (ein0 == "quit") return -1;
    if(ein0==""){ chp=1; }
    else{
	chp=(unsigned short) atoi(ein0.data());
    }
    std::cout << "Chip= " << chp << std::endl;
    std::string ein="";
    std::cout << "I will do a few threshold scans as in THeq, but with 16 pixels"
	      << " active per row at same time (but only one step). Only 4096 "
	      << "pixels are used. Do you want to do a THeq directly afterwards?"
	      << " (no extended coarse THeq possible) (1=yes, 0=no) " 
	      << std::endl;
    ein = getUserInput(_prompt);
    if (ein == "quit") return -1;
    if(ein==""){doTHeq=0;}
    else{
	doTHeq=(unsigned short) atoi(ein.data());
    }
    std::cout << "doTHeq= " << doTHeq << std::endl;

    if (doTHeq==1){
	std::string ein1="";
	std::cout << "How many pixel per row at same time for THeq later? "
		  << "1,2,4,8,16 (more is not good!)? But 0 if you want to "
		  << "decide later " 
		  << std::endl;
	ein1 = getUserInput(_prompt);
	if (ein1 == "quit") return -1;
	if(ein1==""){pix_per_row_THeq=1;}
	else{
	    pix_per_row_THeq =(unsigned short) atoi(ein1.data());
	}
    }
    short ths = 255;
    std::string ein2="";
    std::cout << "Set start value for ths from 20 to 255 (lower only if you are "
	      << "sure that best ths is near that value) " 
	      << std::endl;
    ein2 = getUserInput(_prompt);
    if (ein2 == "quit") return -1;
    if(ein2==""){ths=127;}
    else{
	ths=(unsigned short) atoi(ein2.data());
    }
    short ext_coarse = 0;
    std::string ein3 = "";
    std::cout << " Do you want to use extended coarse thl range (coarse 6 and 8 "
	      << "in addition to 7)? (1=yes, 0=no) " 
	      << std::endl;
    ein3 = getUserInput(_prompt);
    if (ein3 == "quit") return -1;
    if(ein3==""){ext_coarse=0;}
    else{
	ext_coarse=(unsigned short) atoi(ein3.data());
    }
    short max_thl = 1023;
    short min_thl = 0;
    if (ext_coarse==0){
	std::string ein4="";
	std::cout << " Use upper and lower thl bound? Then set upper first, then "
		  << "lower. Default is 1023 and 0. " 
		  << std::endl;
	ein4 = getUserInput(_prompt);
	if (ein4 == "quit") return -1;
	if(ein4==""){max_thl = 1023;}
	else{
	    max_thl=(unsigned short) atoi(ein4.data());
	}
	std::string ein5="";
	ein5 = getUserInput(_prompt);
	if (ein5 == "quit") return -1;
	if(ein5==""){min_thl = 0;}
	else{
	    min_thl=(unsigned short) atoi(ein5.data());
	}
    }
    if (chp == 0){
	for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	    pc->DoTHSopt(doTHeq, pix_per_row_THeq, chip, ths, ext_coarse, max_thl, min_thl);
	}
    }
    else pc->DoTHSopt(doTHeq, pix_per_row_THeq, chp, ths, ext_coarse, max_thl, min_thl);
    return 0;
}


int Console::CommandThresholdEqNoiseCenter(){
    std::string ein;
    unsigned short pix_per_row = 0;
    unsigned short chp = 1;
    short ext_coarse = 0;
    short max_thl = 1023;
    short min_thl = 0;

    std::cout << "You have " 
	      << pc->fpga->tp->GetNumChips() 
	      << "chips, for which of them do you want to do the threshold "
	      << "equalisation? Put 0 if you want to go for all of them "
	      << "one after the other." 
	      << std::endl;
    ein = getUserInput(_prompt);
    if (ein == "quit") return -1;
    if(ein==""){chp=1;}
    else{
	chp=(unsigned short) atoi(ein.data());
    }
    std::cout << "chip " << chp << std::endl;
    std::cout << " You have " 
	      << pc->fpga->tp->GetNumChips() 
	      << " chips, equalisation will be done for chip " 
	      << chp 
	      << std::flush;
    std::string ein1="";
    std::cout << " How many pixel per row at same time? 1,2,4,8,16 "
	      << "(more is not good!)? " 
	      << std::endl;
    ein1 = getUserInput(_prompt);
    if (ein1 == "quit") return -1;
    if(ein1==""){pix_per_row=1;}
    else{
	pix_per_row=(unsigned short) atoi(ein1.data());
    }
    std::cout << pix_per_row << " pixel per row at same time" << std::endl;
    std::string ein3="";
    const char *promptUseExtendedTHLRange = "Do you want to use extended coarse thl range (coarse 6 and 8 in addition to 7)? (1=yes, 0=no) ";
    ein3 = getUserInput(promptUseExtendedTHLRange);
    if (ein3 == "quit") return -1;
    if(ein3==""){ext_coarse=0;}
    else{
	ext_coarse = (unsigned short) atoi(ein3.data());
    }
    if (ext_coarse==0){
	std::string ein4="";
	std::cout << " Use upper and lower thl bound? Then set upper first, then lower. Default is 1023 and 0. " << std::flush;
	ein4 = getUserInput(_prompt);
	if (ein4 == "quit") return -1;
	if(ein4==""){max_thl = 1023;}
	else{
	    max_thl=(unsigned short) atoi(ein4.data());
	}
	std::string ein5="";
	ein5 = getUserInput(_prompt);
	if (ein5 == "quit") return -1;
	if(ein5==""){min_thl = 0;}
	else{
	    min_thl=(unsigned short) atoi(ein5.data());
	}
    }
    if (chp == 0){
	for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	    pc->DoThresholdEqCenter(pix_per_row, chip, ext_coarse, max_thl, min_thl);
	}
    }
    else pc->DoThresholdEqCenter(pix_per_row, chp, ext_coarse, max_thl, min_thl);
    return 0;
}


void Console::CommandTOCalib(){
    // This function does the TO calibration. It is based on the previous TOCalibFast, 
    // rewritten in a cleaner way
    // TODO: describe what the function does exactly
    // - choose which chips to calibrate
    // - choose TOT or TOA
    // - choose internal or external pulser
    // - choose pixels per column (i.e. at the same time this many rows will be
    //   pulsed at the same time
    // - choose shutter type and length
    // - 
    
    // variable which stores, whether we're doing TOT or TOA calibration
    std::string TOmode;
    // variable which stores pulser (internal or external)
    std::string pulser;
    int pixels_per_column;
    // shutter range
    std::string shutter_range;
    // shutter time
    std::string shutter_time;
    std::string input;

    // first we're going to let the user decide which chips to calibrate
    // define a set of ints, which will contain the number of the chips we're
    // going to use
    std::set<int> chip_set;
    chip_set = ChipSelection();

    // second we're going to determine whether the user wants to do a TOT or a 
    // TOA calibration
    TOmode = CalibrationSelection();
    if (TOmode == "quit") return;
    std::cout << "using " << TOmode << std::endl;

    // now we're asking if the user wants to use an internal or external pulser
    pulser = PulserSelection();
    if (pulser == "quit") return;
    std::cout << "using " << pulser << std::endl;

    // TODO: IN case of internal pulser, use default values for shutter range and time
    //       standard and 100

    // now we need to create a list of pulses, which is going to 
    // be used
    // if we use an external pulser, this list will only contain a single value
    // for the internal pulser, we create a list
    std::list<int> pulseList;
    pulseList = PulseListCreator(pulser);
    
    // now determine the pixels per columns to use
    pixels_per_column = PixPerColumnSelection();
    if (pixels_per_column == -1) return;

    // shutter type selection
    shutter_range = ShutterRangeSelection();
    
    // shutter time selection
    shutter_time  = ShutterTimeSelection(shutter_range);

    
    // in case of an external pulser, we will allow the user to add another voltages
    // to the list after the first voltage has finished

    // now I guess call the actual TO calib function?
    if (pulser == "internal"){
	// in case we use the internal pulser, we only allow to use the pulse list 
	// created before
	pc->TOCalib(chip_set, TOmode, pulser, pulseList, pixels_per_column, shutter_range, shutter_time);
    }
    else if (pulser == "external"){
	// in case of an external trigger, we ask whether the user wants to add 
	// another voltage after the first one has finished
	do{
	    pc->TOCalib(chip_set, TOmode, pulser, pulseList, pixels_per_column, shutter_range, shutter_time);
	    
	    // ask for more input
	    std::cout << "Do you wish to use another external pulse voltage? (y / n)" 
		      << std::endl;
	    input = getUserInputNonNumericalNoDefault(_prompt, {"y", "n", "Y", "N"});
	    if (input == "quit") return;
	    else if( (input == "y") ||
		     (input == "Y") ){
		// in this case ask for another value
		pulseList = PulseListCreator(pulser);
	    }
	    else{
		// in this case we just finish
		std::cout << TOmode << " calibration finished." << std::endl;
		
	    }
	} while ( (input != "n") || (input != "N") );
    }

    // :)

}


int Console::CommandTOCalibFast(){
    unsigned short pix_per_row = 1;
    unsigned short shuttertype = 1;
    unsigned short time = 1;
    unsigned short TOT = 0;
    unsigned short internalPulser = 0;
    const char *promptTOTorTOA = "TOT (0) or TOA (1)? ";
    std::string ein0="";
    ein0 = getUserInput(promptTOTorTOA);
    if (ein0 == "quit") return -1;
    if(ein0==""){TOT=0;}
    else{
	TOT=(unsigned short) atoi(ein0.data());
    }
    std::cout << TOT << " selected" << std::endl;
    std::cout << "Hello this is TOT/TOA calibration. You will have to specify "
	      << "spacing and shutter length. I will then ask you a voltage you "
	      << "set on the external pulser or use the internal pulser and "
	      << "record 4 frames(times pixel per row). Then ask you if you "
	      << "want to do another voltage if you use the external pulser. "
	      << std::endl;
    std::cout << "You have "
	      << pc->fpga->tp->GetNumChips() 
	      <<" chips, all of them will be calibrated at the same time. "
	      << std::endl;
    const char *promptExtIntPulser = "Do you want to use the external (0, default) or internal (1) pulser? ";
    std::string ein4="";
    ein4 = getUserInput(promptExtIntPulser);
    if (ein4 == "quit") return -1;
    if(ein4==""){internalPulser=0;}
    else{
	internalPulser=(unsigned short) atoi(ein4.data());
    }
    if (internalPulser == 0) { std::cout<<"External pulser selected"<<std::endl;}
    if (internalPulser == 1) { std::cout<<"Internal pulser selected"<<std::endl;}
    std::cout << "For the spacing: How many pixel per row at same time? "
	      << "1,2,4,8,16 (more is not good!)? "
	      << std::endl;
    std::string ein="";
    ein = getUserInput(_prompt);
    if (ein == "quit") return -1;
    if(ein==""){pix_per_row=1;}
    else{
	pix_per_row = (unsigned short) atoi(ein.data());
    }
    std::cout << pix_per_row << " pixel per row at same time" << std::endl;
    std::cout << "Shutter length: press 1 for short shutter (0-255 clock cycles);"
	      << " press 2 for long shutter (256 - 65280 clock cycles)" 
	      << std::endl;
    std::string ein2="";
    ein2 = getUserInput(_prompt);
    if (ein2 == "quit") return -1;
    if(ein2==""){shuttertype=1;}
    else{
	shuttertype = (unsigned short) atoi(ein2.data());
    }
    std::cout << "Set shutter length in clock cycles(0 -255) (if you have "
	      << "chosen long shutter this value will be multiplied by 256):" 
	      << std::endl;
    std::string ein3="";
    ein3 = getUserInput(_prompt);
    if (ein3 == "quit") return -1;
    if(ein3==""){time=1;}
    else{
	time=(unsigned short) atoi(ein3.data());
    }
    std::cout << "Ok, lets start!" << std::endl;

    pc->TOCalibFast(pix_per_row, shuttertype, time, TOT, internalPulser);
    return 0;
}


int Console::CommandCheckOffset(){
    unsigned short usepreload = pc->CheckOffset();
    std::cout << "Use preload " << usepreload << std::endl;
    return 0;
}


int Console::CommandCalibrate(){

    std::cout << "This is the calibration function." 
	      << std::endl;
    std::cout << "All chips or a selected chip will be calibrated." 
	      << std::endl;
    std::cout << "This can take long (~2 days for an octoboard, several "
	      << "hours for a single chip)." 
	      << std::endl;
    std::cout << "THS optimisation, threshold equalisation, S-curve scan, "
	      << "(optional: TOT calibration and  TOA calibration) will "
	      << "be done."
	      << std::endl;
    const char *promptTOT = "Do you want to do a TOT calibration (0: no, 1: yes)";
    std::string ein="";
    ein = getUserInput(promptTOT);
    if (ein == "quit") return -1;
    unsigned short doTOT = 0;
    if(ein==""){doTOT=0;}
    else{
	doTOT=(unsigned short) atoi(ein.data());
    }
    const char *promptTOA = "Do you want to do a TOA calibration (0: no, 1: yes)";
    std::string ein0="";
    ein0 = getUserInput(promptTOA);
    if (ein0 == "quit") return -1;
    unsigned short doTOA = 0;
    if(ein0==""){doTOA=0;}
    else{
	doTOA=(unsigned short) atoi(ein0.data());
    }
    unsigned short chp = 1;
    std::string ein1="";
    std::cout << ""  << std::endl;
    std::cout << "You have " 
	      <<  pc->fpga->tp->GetNumChips() 
	      << " chips" 
	      << std::endl;
    std::cout << "Which chips do you want to calibrate? Put 0 if you want to "
	      << "go for all of them one after the other. "
	      <<std::endl;
    ein1 = getUserInput(_prompt);
    if (ein1 == "quit") return -1;
    if(ein1==""){chp=1;}
    else{
	chp=(unsigned short) atoi(ein1.data());
    }
    std::cout << "Chip= " << chp << std::endl;

    std::cout << "THS optimisation parameteres:" << std::endl;
    short ths = 255;
    std::string ein2="";
    std::cout << "Set start value for the THS DAC from 20 to 255 (lower only "
	      << "if you are sure that best ths is near that value) " 
	      << std::endl;
    std::cout << "Press ENTER to use 127 (default) " 
	      << std::flush;
    ein2 = getUserInput(_prompt);
    if (ein2 == "quit") return -1;
    if(ein2==""){ths=127;}
    else{
	ths=(unsigned short) atoi(ein2.data());
    }
    short ext_coarse = 0;
    std::string ein3="";
    std::cout << "Do you want to use extended coarse thl range "
	      << "(coarse 6 and 8 in addition to 7)? (1=yes, 0=no) "
	      << std::flush;
    ein3 = getUserInput(_prompt);
    if (ein3 == "quit") return -1;
    if(ein3==""){ext_coarse=0;}
    else{
	ext_coarse=(unsigned short) atoi(ein3.data());
    }
    short max_thl = 1023;
    short min_thl = 0;
    if (ext_coarse==0){
	std::string ein4="";
	std::cout << " THL scan range: Set upper and lower bound. Default "
		  << "is 1023 and 0. "
		  << std::endl;
	const char *promptUpper = "Upper: ";
	ein4 = getUserInput(promptUpper);
	if (ein4 == "quit") return -1;
	if(ein4==""){max_thl = 1023;}
	else{
	    max_thl=(unsigned short) atoi(ein4.data());
	}
	std::string ein5="";
	const char *promptLower = "Lower: ";
	ein5 = getUserInput(promptLower);
	if (ein5 == "quit") return -1;
	if(ein5==""){min_thl = 0;}
	else{
	    min_thl=(unsigned short) atoi(ein5.data());
	}
    }
    unsigned short pix_per_row = 1;
    std::string ein6="";
    std::cout << "Threshold equalisation parameters:" 
	      << std::endl;
    std::cout << "(Spacing) How many active pixel per row at the same time? "
	      << "1,2,4,8,16 (more is not good!)?" 
	      << std::flush;
    ein6 = getUserInput(_prompt);
    if (ein6 == "quit") return -1;
    if(ein6==""){pix_per_row=1;}
    else{
	pix_per_row =(unsigned short) atoi(ein6.data());
    }

    unsigned short voltage = 0;
    unsigned short offset = 0;
    int time = 255;
    unsigned short StartTHL[9] = {0};
    unsigned short StopTHL[9] = {1023};
    // input string for getUserInput in the following few lines of code
    std::string input;
    std::cout << "S-Curve parameters (internal pulser will be used):" 
	      << std::endl;
    std::cout << "Warning: Only CTPR = 1 will be used. Hence only "
	      << "column x = 0, x= 32, ..." 
	      << std::endl;
    std::cout << "Make sure that NONE of this columns is dead and press 0. "
	      << "Otherwise put a column offset ( 0(no offset) to 31)" 
	      << std::flush;
    input      = getUserInput(_prompt, true, false);
    if (input == "quit") return -1;
    offset     = std::stoi(input);
    std::cout << "Shutter time in clock cycles (0-255), LONG mode used "
	      << "(100 is ok for internal pulser) " 
	      << std::flush;
    input      = getUserInput(_prompt, true, false);
    if (input == "quit") return -1;
    time       = std::stoi(input);
    for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	std::cout << "End (upper) THL for chip "   << chip << ": " << std::flush;
	input          = getUserInput(_prompt, true, false);
	if (input == "quit") return -1;
	StopTHL[chip]  = std::stoi(input);
	std::cout << "Start (lower) THL for chip " << chip << ": " << std::flush;
	input          = getUserInput(_prompt, true, false);
	if (input == "quit") return -1;
	StartTHL[chip] = std::stoi(input);
    }
    unsigned short pix_per_rowTO = 1;
    unsigned short shuttertype = 1;
    unsigned short timeTO = 1;
    unsigned short internalPulser = 1;
    if (doTOT == 1 || doTOA == 1) {
	std::cout << "TOT and/or TOA calibration parameters (internal "
		  << "pulser will be used): "
		  << std::endl;
	std::cout << "You have "
		  << pc->fpga->tp->GetNumChips() 
		  <<" chips, all of them will be calibrated at the same time. " 
		  << std::endl;
	std::cout << "For the spacing: How many pixel per row at same time? "
		  << "1,2,4,8,16 (more is not good!)? "
		  << std::endl;
	std::string ein7="";
	ein7 = getUserInput(_prompt);
	if (ein7 == "quit") return -1;
	if(ein7==""){pix_per_rowTO=1;}
	else{
	    pix_per_rowTO=(unsigned short) atoi(ein7.data());
	}
	std::cout << pix_per_rowTO 
		  << " pixel per row at same time"
		  <<std::endl;
	std::cout << "Shutter length: press 1 for short shutter "
		  << "(0-255 clock cycles); press 2 for long shutter "
		  << "(256 - 65280 clock cycles)"
		  << std::endl;
	std::string ein8="";
	ein8 = getUserInput(_prompt);
	if (ein8 == "quit") return -1;
	if(ein8==""){shuttertype=1;}
	else{
	    shuttertype=(unsigned short) atoi(ein8.data());
	}
	std::cout << "Set shutter length in clock cycles(0 -255) "
		  << "(if you have chosen long shutter this value will "
		  << "be multiplied by 256):"
		  << std::endl;
	std::string ein9="";
	ein9 = getUserInput(_prompt);
	if (ein9 == "quit") return -1;
	if(ein9==""){timeTO=1;}
	else{
	    timeTO=(unsigned short) atoi(ein9.data());
	}
	std::cout << "Ok, lets start!"<<std::endl;
    }
    if (chp == 0){
	for (unsigned short chip = 1;chip <= pc->fpga->tp->GetNumChips() ;chip++){
	    pc->DoTHSopt(0, 0, chip, ths, ext_coarse, max_thl, min_thl);
	    pc->DoThresholdEqCenter(pix_per_row, chip, ext_coarse, max_thl, min_thl);
	}
    }
    else {
	pc->DoTHSopt(0, 0, chp, ths, ext_coarse, max_thl, min_thl);
	pc->DoThresholdEqCenter(pix_per_row, chp, ext_coarse, max_thl, min_thl);
    }
    // TODO: this function here still trys to call the OLD DoSCurveScan, which was removed in commit
    //       2ae042ac7424a72e8f428f875c28d2132e7726d5
    //       if still needed after all, get back from there...
    pc->DoSCurveScan(voltage, time, StartTHL, StopTHL, offset);
    if (doTOT == 1) {
	pc->TOCalibFast(pix_per_rowTO, shuttertype, timeTO, 0, internalPulser);
    }
    if (doTOA == 1) {
	pc->TOCalibFast(pix_per_rowTO, shuttertype, timeTO, 1, internalPulser);
    }
    return 0;
}


int Console::CommandSwitchTriggerConnection(){
    // default CommandSwitchTriggerConnection function
#if DEBUG==2
    std::cout << "Enter Console::CommandCountingTime()"
	      << std::endl;	
#endif
    std::string input;
    const char *promptType = "Please provide a trigger connection type { tlu, lemo }> ";
    std::set<std::string> allowedStrings = {"tlu", "lemo"};
    input = getUserInputNonNumericalNoDefault(promptType, &allowedStrings);
    if (input == "quit") return -1;
    else if(input.compare("tlu")==0){
	pc->fpga->SwitchTriggerConnection(1);
    }
    else{
	// if tlu is not in input, lemo is (getUserInput takes care of 
	// only allowing strings in allowedStrings
	pc->fpga->SwitchTriggerConnection(0);
    }
    return 0;
}

void Console::DACScanLive(char dac, int val){
#if DEBUG==2
    std::cout<<"Enter Console::CommandDACScanLive()"<<std::endl;	
#endif
    if(dac>=0){std::cout << std::endl << pc->fpga->tp->GetDACName((unsigned int) dac) << "\n";}
    std::cout << val << "   ";
}


void Console::WrapperToDACScanLive(void* PointerToObject, char dac, int val){
#if DEBUG==2
    std::cout << "Enter Console::WrapperToDACScanLive()" << std::endl;	
#endif
    Console* mySelf = (Console*) PointerToObject;
    mySelf->DACScanLive(dac, val);
}


void Console::CommandSpeedTest(std::string ein){
    int wdh,freq,pos;
	
    if(ein==""){std::cout << "Falsche Eingabe" << std::endl; return;}
    else{
	if(ein.find_first_not_of("0123456789 ",0)==ein.npos){
	    pos=ein.find(" ",1);
	    wdh=atoi(&ein[0]);
	    freq=atoi(&ein[pos]);
	    pc->SpeedTest(wdh,freq);
	}
	else{
	    std::cout << "\tNon-numerical sign\n"
		      << std::flush; 
	    return;
	}
    }
}


void Console::CommandSetIP(){
    unsigned int i;
    int pos,n;
    int byte[4]={0};
    std::string str;
    std::stringstream ip;
    std::string input;
    const char *promptIP = "Please enter a new IP address> ";
    input = getUserInputNonNumericalNoDefault(promptIP);    
    if (input == "quit") return;
    else if((input.length()>5)&&(input.length()<16)&&(input.find_first_not_of("0123456789.",0)==input.npos)){
	n=0; i=0; pos=-1;
	while(n<3){
	    while( (i<input.length()) && (i-pos<4) && (input[i]!='.') ){++i;}
	    if(input[i]=='.'){
		str=input.substr(pos+1,i-pos-1);
		byte[n]=atoi(&str[0]);
	    }
	    else{
		std::cout << "Invalid IP\n" 
			  << std::flush; 
		return;
	    }
	    if(byte[n]>255){
		std::cout << "Invalid IP\n"
			  << std::flush; 
		return;
	    }
	    ++n; pos=i; ++i;
	}
	while( (i<input.length()) && (input[i]!='.') ){++i;}
	if( (input[i]=='.') || (i-pos>4) || (i-pos==1) ){
	    std::cout << "Invalid IP\n" 
		      << std::flush; 
	    return;
	}
	else{
	    str=input.substr(pos+1,i-pos-1);
	    byte[n]=atoi(&str[0]);
	}
	if(byte[n]>255){
	    std::cout << "Invalid IP\n" 
		      << std::flush; 
	    return;
	}   
    }
    else{
	std::cout << "Invalid IP\n" 
		  << std::flush; 
	return;
    }

    ip<<byte[0]<<'.'<<byte[1]<<'.'<<byte[2]<<'.'<<byte[3];
    pc->fpga->SetIP(ip.str());
}


void Console::CommandSetChipIdOffset(){
    // this function asks the user for input on the Chip ID offset (for the timepix class)
    std::set<std::string> allowedChipIdOffsets; 
    // fill this set with all values from 1 to 255
    for( int l=1; l<256; l++) allowedChipIdOffsets.insert(std::to_string(l));
    std::string input;
    std::cout << "Please enter a new Chip ID offset for the timepix chip. Choose from x = {1, 255}\n"
	      << "Typical values:\n"
	      << "1 chip:  188\n"
	      << "7 chips: 192\n"
	      << "8 chips: 193"
	      << std::endl;
    input = getUserInputNumericalNoDefault(_prompt, &allowedChipIdOffsets);

    int newChipIdOffset;
    if (input == "quit"){
	return;
    }
    else{
	newChipIdOffset = std::stoi(input);
    }
    
    pc->fpga->tp->SetChipIDOffset(newChipIdOffset);

    // to make sure that the new offset is good, read the Chip ID; done by calling CommandWriteReadFSR()
    std::cout << "Checking if new chip ID offset is good... calling WriteReadFSR()..." << std::endl;
    CommandWriteReadFSR();
}


void Console::CommandFadcPedestalRun(){
    // this function calls the StartFadcPedestalRun() function, which performs
    // a pedestal calibration of the FADC. For further information see its implementation
    // console function here makes sure user is aware that he/she should disconnect
    // all connected devices from the FADC

    std::string input;
    std::set<std::string> allowedStrings = {"continue"};
    std::cout << "Before starting the pedestal calibration run, please disconnect all "
	      << "devices from the FADC! Type { continue } once that is done."
	      << std::endl;
    
    input = getUserInputNonNumericalNoDefault(_prompt, &allowedStrings);
    if (input == "quit"){
	return;
    }
    else{
	std::cout << "starting FADC pedestal calibration run." << std::endl;
	_hvFadcManager->StartFadcPedestalRun();
    }
}


void Console::CommandAddChannel(){
    // this function provides a user interface to add a HV channel to
    // the current group of active channels
    std::string input;

    std::string channelName;
    std::cout << "Enter channel name:" << std::endl;
    input = getUserInputNonNumericalNoDefault(_prompt);
    if (input == "quit")
	return;
    else{
	channelName = input;
    }

    int voltage;
    std::cout << "Enter channel voltage:" << std::endl;
    std::set<std::string> allowedStrings;
    for(int i = 0; i <= 4000; i++) allowedStrings.insert(std::to_string(i));
    input = getUserInputNumericalNoDefault(_prompt, &allowedStrings);
    if (input == "quit")
	return;
    else{
	voltage = std::stoi(input);
    }

    bool good;
    good = _hvFadcManager->CreateChannel(channelName, voltage);
    if (good == false){
	std::cout << "Could not add new channel (probably could not set voltage on"
		  << " module."
		  << std::endl;
	return;
    }
    
}

void Console::CommandRemoveChannel(){
    // this function provides a user interface to remove a specific
    // channel from the channel list of the hvFadcManager
    std::string input;

    // first print the active channels
    std::set<std::string> allowedStrings;
    allowedStrings = _hvFadcManager->PrintActiveChannels();

    std::cout << "Enter number of channel to remove:" << std::endl;
    int channelNumber;
    input = getUserInputNumericalNoDefault(_prompt, &allowedStrings);
    if (input == "quit")
	return;
    else{
	channelNumber = std::stoi(input);
    }

    // now call function to remove the channel
    std::cout << "removing channel and shutting it down..." << std::endl;
    _hvFadcManager->RemoveChannelByNumber(channelNumber);
}

void Console::CommandAddFlexGroup(){
    // not implemented yet
    std::cout << "not implemented yet." << std::endl;
}

void Console::CommandRemoveFlexGroup(){
    // not implemented yet
    std::cout << "not implemented yet." << std::endl;
}


void Console::CommandSetChannelValue(){
    // this function allows one to set a value of a specific channel, e.g.
    // voltage, current, nominal values etc.
    std::string input;

    // first print the active channels
    std::set<std::string> allowedStrings;
    allowedStrings = _hvFadcManager->PrintActiveChannels();

    std::cout << "Choose a channel for which to set a value:" << std::endl;
    int channelNumber;
    input = getUserInputNumericalNoDefault(_prompt, &allowedStrings);
    if (input == "quit")
	return;
    else{
	channelNumber = std::stoi(input);
    }

    _hvFadcManager->PrintChannel(channelNumber);
    
    std::cout << "Choose a value to set:\n"
	      << "    voltage\n"
	      << "    current\n"
	      << "    voltageNominal\n"
	      << "    currentNominal\n"
	      << std::endl;
    std::set<std::string> allowedKeys = {"voltage",
					 "current",
					 "voltageNominal",
					 "currentNominal"};
    std::string inputKey;
    inputKey = getUserInputNonNumericalNoDefault(_prompt, &allowedKeys);
    if(inputKey == "quit") return;

    std::set<std::string> allowedValues;
    if( (inputKey == "voltage") ||
	(inputKey == "voltageNominal") ){
	for(int i = 0; i < 4000; i++) allowedValues.insert(std::to_string(i));
	std::cout << "enter " << inputKey
		  << " to set. Enter in V:"
		  << std::endl;
    }
    else if( (inputKey == "current") ||
	     (inputKey == "currentNominal") ){
	for(int i = 0; i < 2000; i++){
	    std::cout << "current selection currently broken" << std::endl;
	    float current;
	    current = float(i) / 1000.0;
	    allowedValues.insert(std::to_string(current));
	}
	std::cout << "enter " << inputKey
		  << " to set. Enter in mA:"
		  << std::endl;
    }

    std::string inputValue;
    inputValue = getUserInputNumericalNoDefault(_prompt, &allowedValues);
    if (inputValue == "quit") return;
    
    _hvFadcManager->SetChannelValue(inputKey, channelNumber, inputValue);
}


void Console::CommandSetCenterChip(){
    // this function provides a user interface to set the center chip member
    // variable of the PC object, which decides when the FADC is being read out
    std::string input="";
    const char *prompt = "Set the center chip (range: 1 to num_chips): ";
    std::set<std::string> allowedChipNum;
    // make sure only 1 to NumChips chips are typed in. More than 9 (due to illogical
    // numbering of chips starting from 1 instead of 0) and we will get a
    // segmentation fault in the timepix creator. Size of arrays for chips
    // are hardcoded at the moment to allow for 9 chips.
    for(int l = 1; l < _nbOfChips + 1; l++) allowedChipNum.insert(std::to_string(l));
    
    input = getUserInputNumericalNoDefault(prompt, &allowedChipNum);
    if (input == "quit") return;
    else{
	// in this case set number to nChips
	pc->SetCenterChip(std::stoi(input));
    }
}

void Console::CommandPrintCenterChip(){
    // this function can be used to print the center chip member variable of pc
    int chip;
    chip = pc->GetCenterChip();
    std::cout << "Center chip variable is currently set to : " << chip << std::endl;
}


// ######################################################################
// ################## MCP2210 related commands ##########################
// ######################################################################

void Console::CommandTempLoopReadout(){
    // function calls temp_auslese_main and simply loops over the
    // temperature readout function in a separate thread

    // now loop over fpga->tpulse to pulse..
    // create seperate thread, which loops and will be stopped, if we type stop in terminal
    std::atomic_bool loop_continue;
    loop_continue = true;

    std::thread loop_thread(temp_auslese_main, &loop_continue);
    const char *waitingPrompt = "temp readout running. type 'stop' to quit> ";
    std::string input;
    std::set<std::string> allowedStrings = {"stop"};
    input = getUserInputNonNumericalNoDefault(waitingPrompt, &allowedStrings);
    if (input == "stop"){
    	loop_continue = false;
    }
    loop_thread.join();

}
