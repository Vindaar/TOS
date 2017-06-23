// this function contains the whole user interface of the TOS
#include "console.hpp"

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
	else if( ein.compare("SCurveOld")==0 )
	{
	    CommandSCurveOld();
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
	else if( ein.compare("DefaultChessMatrix")==0 )
	{
	    CommandVarChessMatrixDefault();
	}
	else if( ein.compare("ChessMatrixAll")==0 )
	{
	    CommandVarChessMatrix(true);
	}
	else if( ein.compare("ChessMatrix")==0 )
	{
	    CommandVarChessMatrix(false);
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
	else if( ein.compare("GetMatrixAndDump")==0 )
	{
	    CommandGetMatrixAsIntAndDump();
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
