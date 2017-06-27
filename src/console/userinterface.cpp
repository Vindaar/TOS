// this function contains the whole user interface of the TOS
#include "console.hpp"
#include <boost/algorithm/string/find.hpp>
#include "tosCommandCompletion.hpp"

int Console::UserInterface(){
#if DEBUG==2
    std::cout<<"Enter Console::UserInterface()"<<std::endl;     
#endif

    int running = 1;
    
    // create a char buffer, which stores the input from the readline() function
    char *buf;
    // define the attempted completion function (our custom TOS_Command_Completion
    // function defined in tosCommandCompletion.cpp
    rl_attempted_completion_function = TOS_Command_Completion;

    // define a boost range iterator to search within the input string
    // to conceal all FADC functions as long as the HFM was not init'd
    //boost::iterator_range<std::string::const_iterator> range;
  
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
        std::string input(buf);
	
	static const std::set<std::string> HFMCommandsSet = get_hfm_commands();
	auto is_hv_input = HFMCommandsSet.find(input);

	// first parse the normal TOS commands (all non-HFM commands)
	ParseNormalTosCommands(input, running);

        // ##################################################
        // ################## HV_FADC related commands ######
        // ##################################################   

        if (input.compare("ActivateHFM") == 0)
        {
            CommandActivateHvFadcManager();
        }

	else if (_hvFadcManagerActive == false &&
		 is_hv_input != HFMCommandsSet.end()){
	    std::cout << "HFM not initialized. Nothing to do. \n" 
		      << "Call ActivateHFM command and try again"
		      << std::endl;
	}

        else if (_hvFadcManagerActive == true){
            // allow access to HFM and FADC functions only in case the HFM is
	    // initialized
	    if (input.compare("ConnectModule") == 0){
		std::cout << _hvFadcManager->H_ConnectModule() << std::endl;
	    }

	    if (is_hv_input != HFMCommandsSet.end()){
		// call function to parse the (valid) command related to HFM
		ParseActiveHfmCommands(input);
	    }
	}

        // ##################################################
        // ################## MCP2210 related commands ######
        // ##################################################   

        else if (input.compare("TempLoopReadout") == 0){
            CommandTempLoopReadout();
        }       
        
        // if no other if was true, command not found
        else if( input.compare("")==0 )
        {
            // TODO: is this necessary??
            //std::cout << "" << std::flush;
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

    // now delete the array storing the possible TOS commands
    TOS_Command_Generator("", -1);

    return 0;
}

void Console::ParseNormalTosCommands(std::string input, int &running){
    // this function parses the 'normal' TOS commands without the
    // HFM usage

    int result = 0;
    
    if((input.compare("GeneralReset")==0)||(input.compare("1")==0)) 
    {
	result=pc->fpga->GeneralReset();
	if(result>10){ ErrorMessages(result); }
	else{ std::cout<<"\tGeneralReset accomplished\n"<<std::flush; }
    }
    
    else if( (input.compare("Counting")==0) ||
	     (input.compare("2")==0) )
    {
	CommandCounting(1);
    }
    else if( (input.compare("CountingStop")==0) ||
	     (input.compare("2s")==0) )
    {
	CommandCounting(0);
    }
    else if( (input.compare("CountingTrigger") == 0) ||
	     (input.compare("2t") == 0) )
    {
	CommandCountingTrigger();
    }
    else if( (input.compare("CountingTime")==0) ||
	     (input.compare("2z")==0 ))
    {
	// this function will not receive an argument anymore. Instead after binputg called
	// it will ask the user, which multiplier he wants to use (represents 
	// standard, long and verylong of the past)
	CommandCountingTime();
    }
    else if( (input.compare("ReadOut")==0)||
	     (input.compare("3")==0) ) 
    {
	CommandReadOut();
    }
    else if( (input.compare("ReadOut2")==0)||
	     (input.compare("3a")==0) ) 
    {
	CommandReadOut2();
    }
    else if( (input.compare("SetMatrix")==0)||
	     (input.compare("4")==0) ) 
    {
	CommandSetMatrix();
    }
    else if( (input.compare("WriteReadFSR")==0)||
	     (input.compare("5")==0) ) 
    {
	CommandWriteReadFSR();
    }
    else if( (input.compare("Run")==0) ||
	     (input.compare("run")==0) )
    {
	CommandRun(_hvFadcManagerActive);
    }
    else if( input.compare("EnableTPulse")==0 )
    {
	pc->fpga->EnableTPulse(1);
    }
    else if( input.compare("DisableTPulse")==0 )
    {
	pc->fpga->EnableTPulse(0);
    }
    else if( input.compare("EnableFADCshutter")==0 )
    {
	CommandFADCshutter();
    }
    else if( input.compare("DisableFADCshutter")==0 )
    {
	pc->fpga->EnableFADCshutter(0);
    }
    else if( input.compare("DACScan")==0 )
    {
	CommandDACScan();
    }
    else if( input.compare("THLScan")==0 )
    {
	CommandTHLScan();
    }
    else if( input.compare("SCurve")==0 )
    {
	CommandSCurve();
    }
    else if( input.compare("SCurveOld")==0 )
    {
	CommandSCurveOld();
    }
    else if( input.compare("i2creset")==0 )
    {
	Commandi2creset();
    }
    else if( input.compare("i2cDAC")==0 )
    {
	Commandi2cDAC();
    }
    else if( input.compare("i2cADC")==0 )
    {
	Commandi2cADC();
    }
    else if( input.compare("tpulse")==0 )
    {
	CommandTpulse();
    }
    else if( input.compare("TestTPulse")==0 )
    {
	CommandTestTPulse();
    }
    else if( (input.compare("THSopt")==0) ||
	     (input.compare("6" )==0) )
    {
	CommandDoTHSopt();
    }
    else if( (input.compare("ThEqNoiseCenter")==0) ||
	     (input.compare("7")==0) ) 
    {
	CommandThresholdEqNoiseCenter();
    }
    else if( (input.compare("TOCalib")==0) ||
	     (input.compare("8")==0) ) 
    {
	CommandTOCalib();
    }
    else if( (input.compare("TOCalibFast")==0) ||
	     (input.compare("8a")==0) ) 
    {
	CommandTOCalibFast();
    }
    else if( input.compare("LoadThreshold")==0)
    {
	CommandLoadThreshold();
    }
    else if( input.compare("SaveFSR")==0 )
    {
	CommandSaveFSR();
    }
    else if( (input.compare("LoadFSR")==0) ||
	     (input.compare("lf")==0)) 
    {
	CommandLoadFSR();
    }
    else if( (input.compare("LoadFSRAll")==0) ||
	     (input.compare("lfa")==0)) 
    {
	CommandLoadFSRAll();
    }
    else if( input.compare("SetDAC")==0 )
    {
	CommandSetDAC();
    }
    else if( input.compare("ShowFSR")==0 )
    {
	CommandShowFSR();
    }
    else if( input.compare("DefaultChessMatrix")==0 )
    {
	CommandVarChessMatrixDefault();
    }
    else if( input.compare("ChessMatrixAll")==0 )
    {
	CommandVarChessMatrix(true);
    }
    else if( input.compare("ChessMatrix")==0 )
    {
	CommandVarChessMatrix(false);
    }
    else if( (input.compare("UniformMatrix")==0) ||
	     (input.compare("um")==0) ) 
    {
	CommandUniformMatrix();
    }
    else if( (input.compare("UniformMatrixAllChips")==0) ||
	     (input.compare("uma")==0) ) 
    {
	CommandUniformMatrixAllChips();
    }
    else if( input.compare("GetMatrixAndDump")==0 )
    {
	CommandGetMatrixAsIntAndDump();
    }
    else if( input.compare("SaveMatrix")==0 )
    {
	CommandSaveMatrix();
    }
    else if( (input.compare("LoadMatrix")==0) ||
	     (input.compare("loadmatrix")==0) )
    {
	CommandLoadMatrix();
    }
    else if( input.compare("Trigger")==0 )
    {
	CommandSwitchTriggerConnection();
    }
    else if( input.compare("SetIP")==0 )
    {
	CommandSetIP();
    }
    else if( input.compare("ShowIP")==0 )
    {
	std::cout << pc->fpga->ShowIP() << "\n" << std::flush;
    }
    else if( (input.compare("MakeARP")==0) ||
	     (input.compare("makearp")==0) )
    {
	pc->fpga->MakeARPEntry();
    }
    else if( input.compare("quit")==0 )
    {
	running = 0;
    }
    else if( input.compare("help")==0 )
    {
	CommandHelp();
    }
    else if( input.compare("spacing")==0 )
    {
	CommandSpacing();
    }
    else if( input.compare("SetNumChips")==0 )
    {
	CommandSetNumChips();
    }
    else if( input.compare("SetOption")==0 )
    {
	CommandSetOption();
    }
    else if( input.compare("CheckOffset")==0 )
    {
	CommandCheckOffset();
    }
    else if( input.compare("Calibrate")==0 )
    {
	CommandCalibrate();
    }
    else if( input.compare("SetChipIDOffset")==0 )
    {
	CommandSetChipIdOffset();
    }
    else if( input.compare("EnableFastClock")==0 )
    {
	pc->fpga->UseFastClock(1);
    }
    else if( input.compare("DisableFastClock")==0 )
    {
	pc->fpga->UseFastClock(0);
    }
    else if (input.compare("SetCenterChip") == 0){
	CommandSetCenterChip();
    }
    else if (input.compare("PrintCenterChip") == 0){
	CommandPrintCenterChip();
    }
}


void Console::ParseActiveHfmCommands(std::string input){
    // this function parses the input in case the HFM is activated
    // and the input is a valid command from the HFM commands set

    if ((input.compare("InitHFM") == 0) ||
	(input.compare("InitHV_FADC") == 0)){
	_hvFadcManager->InitHFMForTOS();
    }
    // function to call to ramp up
    else if (input.compare("RampChannels") == 0){
	_hvFadcManager->RampChannels();
    }
    else if (input.compare("ShutdownHFM") == 0){
	_hvFadcManager->ShutDownHFMForTOS();
	// now delete the hvFadcManager
	if(_hvFadcManagerActive){
	    delete _hvFadcManager;
	}
	// and set the flag to false
	_hvFadcManagerActive = false;
    }
    else if (input.compare("CheckHVModuleIsGood") == 0){
	_hvFadcManager->H_CheckHVModuleIsGood();
    }

    else if (input.compare("TurnChannelOnOff") == 0){
	_hvFadcManager->TurnChannelOnOff();
    }

    else if (input.compare("ClearChannelEventStatus") == 0){
	_hvFadcManager->ClearChannelEventStatus();
    }

    else if (input.compare("PrintChannelStatus") == 0){
	_hvFadcManager->printAllChannelStatus();
    }
    else if (input.compare("PrintChannelEventStatus") == 0){
	_hvFadcManager->printAllChannelEventStatus();
    }
    else if (input.compare("PrintModuleStatus") == 0){
	_hvFadcManager->printModuleStatus();
    }
    else if (input.compare("PrintModuleEventStatus") == 0){
	_hvFadcManager->printModuleEventStatus();
    }
    else if (input.compare("AddChannel") == 0){
	CommandAddChannel();
    }
    else if (input.compare("RemoveChannel") == 0){
	CommandRemoveChannel();
    }
    else if (input.compare("AddFlexGroup") == 0){
	CommandAddFlexGroup();
    }
    else if (input.compare("RemoveFlexGroup") == 0){
	CommandRemoveFlexGroup();
    }
    else if (input.compare("PrintActiveChannels") == 0){
	_hvFadcManager->PrintActiveChannels();
    }

    else if (input.compare("SetChannelValue") == 0){
	CommandSetChannelValue();
    }

    
    // ##################################################
    // ################## FADC related commands #########
    // ##################################################

    
    else if (input.compare("PrintFADCSettings") == 0){
	_hvFadcManager->FADC_Functions->printSettings();
    }
    else if (input.compare("ResetFADC") == 0){
	_hvFadcManager->F_Reset();
    }
    else if (input.compare("SetFadcSettings") == 0){
	_hvFadcManager->SetFadcSettings();
    }
    else if (input.compare("StartFadcPedestalRun") == 0){
	CommandFadcPedestalRun();
    }
    else if ((input.compare("StartFadcAcquisition") == 0) ||
	     (input.compare("StartFadcAcq")         == 0)){
                    
	_hvFadcManager->F_StartAcquisition();
    }
    else if (input.compare("SendFadcSoftwareTrigger") == 0){
	_hvFadcManager->F_SendSoftwareTrigger();        
    }
    else if (input.compare("ReadFadcInterrupt") == 0){
	std::cout << "Interrupt: " << _hvFadcManager->F_ReadInterrupt() << std::endl;
    }
    else if (input.compare("ReleaseFadcInterrupt") == 0){
	_hvFadcManager->F_ReleaseInterrupt();
    }
    else if (input.compare("SetFadcTriggerThresholdDACAll") == 0){
	//_hvFadcManager->F_SetTriggerThresholdDACAll( getInputValue() );
	_hvFadcManager->F_SetTriggerThresholdDACAll( std::stoi(getUserInput(_prompt)) );
    }
    else if (input.compare("GetFadcTriggerPerChannel") == 0){
	std::cout << "getTriggerThreshold perChannel:" << std::endl;
	std::cout << "Channel 1: "
		  <<  _hvFadcManager->F_GetTriggerThresholdDACPerChannel(0)
		  << std::endl;
	std::cout << "Channel 2: "
		  <<  _hvFadcManager->F_GetTriggerThresholdDACPerChannel(1)
		  << std::endl;
	std::cout << "Channel 3: "
		  <<  _hvFadcManager->F_GetTriggerThresholdDACPerChannel(2)
		  << std::endl;
	std::cout << "Channel 4: "
		  <<  _hvFadcManager->F_GetTriggerThresholdDACPerChannel(3)
		  << std::endl;
	std::cout << "getTriggerThreshold All: "
		  << _hvFadcManager->F_GetTriggerThresholdDACAll()
		  << std::endl << std::endl;
    }
    else if (input.compare("SetFadcTriggerThresholdRegisterAll") == 0){
	// TODO: PUT INTO FUNCTION
	std::string input;
	input = getUserInputNonNumericalNoDefault(_prompt);
	unsigned int result;
	result = _hvFadcManager->FADC_Functions->setTriggerThresholdRegisterAll( std::stoi(input) );
	std::cout << "setTriggerThresholdRegisterAll returns " << result << std::endl;
    }
    else if (input.compare("GetFadcTriggerThresholdRegister") == 0){
	_hvFadcManager->FADC_Functions->getTriggerThresholdRegister();
    }
    else if (input.compare("LoadFadcTriggerThresholdDAC") == 0){
	_hvFadcManager->F_LoadTriggerThresholdDAC();
    }
    else if (input.compare("SetFadcTriggerType") == 0){
	_hvFadcManager->F_SetTriggerType( getInputValue() );
    }
    else if (input.compare("GetFadcTriggerType") == 0){
	std::cout << "Trigger type: " << _hvFadcManager->F_GetTriggerType() << std::endl;
    }
    else if (input.compare("SetFadcTriggerChannelSource") == 0){
	_hvFadcManager->F_SetTriggerChannelSource( getInputValue() );
    }
    else if (input.compare("GetFadcTriggerChannelSource") == 0){
	std::cout << "Trigger channel source: " << _hvFadcManager->F_GetTriggerChannelSource() << std::endl;
    }
    else if (input.compare("SetFadcPostTrig") == 0){
	_hvFadcManager->F_SetPosttrig( getInputValue() );
    }
    else if (input.compare("GetFadcPostTrig") == 0){
	std::cout << "Posttrig: " << _hvFadcManager->F_GetPosttrig() << std::endl;
    }
    else if (input.compare("SetFadcPreTrig") == 0){
	_hvFadcManager->F_SetPretrig( getInputValue() );
    }
    else if (input.compare("GetFadcPreTrig") == 0){
	std::cout << "Pretrig: " << _hvFadcManager->F_GetPretrig() << std::endl;
    }
    else if (input.compare("SetFadcChannelMask") == 0){
	_hvFadcManager->F_SetChannelMask( getInputValue() );
    }
    else if (input.compare("GetFadcChannelMask") == 0){
	std::cout << "Channel mask: " << _hvFadcManager->F_GetChannelMask() << std::endl;
    }
    else if (input.compare("SetFadcNumberOfChannels") == 0){
	_hvFadcManager->F_SetNbOfChannels(getInputValue());
    }
    else if (input.compare("GetFadcNumberOfChannels") == 0){
	std::cout << "#Channels: " << _hvFadcManager->F_GetNbOfChannels() << std::endl;
    }
    else if (input.compare("SetFadcModeRegister") == 0){
	_hvFadcManager->F_SetModeRegister(static_cast<const unsigned short>(getInputValue()));
    }
    else if (input.compare("GetFadcModeRegister") == 0){
	std::cout << "mode register: " << _hvFadcManager->F_GetModeRegister() << std::endl;
    }
    else if (input.compare("SetFadcFrequency") == 0){
	_hvFadcManager->F_SetFrequency( getInputValue() );
    }
    else if (input.compare("GetFadcFrequency") == 0){
	std::cout << "Frequency: " << _hvFadcManager->F_GetFrequency() << std::endl;
    }
    else if (input.compare("SetFadcReadMode") == 0){
	_hvFadcManager->F_SetReadMode( getInputValue() );
    }
    else if (input.compare("GetFadcReadMode") == 0){
	std::cout << "Read mode: " << _hvFadcManager->F_GetReadMode() << std::endl;
    }
    else if (input.compare("SetFadcPostStopLatency") == 0){
	_hvFadcManager->F_SetPostStopLatency( getInputValue() );
    }
    else if (input.compare("GetFadcPostStopLatency") == 0){
	std::cout << "Post stop latency: "
		  << _hvFadcManager->F_GetPostStopLatency()
		  << std::endl;
    }
    else if (input.compare("SetFadcPostLatencyPreTrig") == 0){
	_hvFadcManager->F_SetPostLatencyPretrig( getInputValue() );
    }
    else if (input.compare("GetFadcPostLatencyPretrig()") == 0){
	std::cout << "Post latency pretrig: "
		  << _hvFadcManager->F_GetPostLatencyPretrig()
		  << std::endl;
    }
    else if (input.compare("SetFadcSleepAcqTime") == 0){
	std::string input;
	input = getUserInputNumericalNoDefault(_prompt);
	_hvFadcManager->setSleepAcqTime(std::stoi(input));
    }
    else if (input.compare("SetFadcSleepTriggerTime") == 0){
	std::string input;
	input = getUserInputNumericalNoDefault(_prompt);
	_hvFadcManager->setSleepTriggerTime(std::stoi(input));
    }
}
