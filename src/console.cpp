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
#include <string>



//C~tor
Console::Console():
         _nbOfChips(0),
         _preload(0),
         _hvFadcObj(NULL),
         _hvFadcObjActive(false)
{
#if DEBUG==2
    std::cout<<"Enter Console::Console()"<<std::endl;
#endif

    ok = pc.okay();
    std::string input="";
  
    //get nb of chips
    const char *prompt = "How many chips are on your board? (1-8)";
    input = getUserInput(prompt);
    if(input==""){_nbOfChips=1;}
    else{
	_nbOfChips=(unsigned short) atoi(&input[0]);
    }
    std::cout << "Number of Chips: " << _nbOfChips << std::endl << std::endl;
    input.clear();
 
    //get preload bytes
    _preload = getPreload();
    pc.fpga.tp.SetNumChips(_nbOfChips,_preload);
}

Console::Console(std::string iniFilePath):
    _nbOfChips(1),
    _preload(0),
    // _fadc(dev),
    _hvFadcObjActive(true)
{
#if DEBUG==2
    std::cout<<"Enter Console::Console()"<<std::endl;
#endif

    bool useHvFadc = true;
    _hvFadcObj = new HV_FADC_Obj(iniFilePath);

    // now the HV_FADC_Obj should be set up and running 
    // HV voltages ramped up

    //init FADC
    pc.initHV_FADC(_hvFadcObj, useHvFadc);
    ok = pc.okay();
    
    std::cout << "Warning: In FADC-Mode one can only use one Chip" << std::endl;

    //get preoload
    _preload = getPreload();
    pc.fpga.tp.SetNumChips(_nbOfChips,_preload);
}



Console::~Console()
{
    if(_hvFadcObjActive){
        delete _hvFadcObj;
    }
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



// TODO: remove ConsoleMain() once UserInterfaceFADC and UserInterfaceNew are removed
void Console::ConsoleMain(){
#if DEBUG==2
    std::cout<<"Enter Console::ConsoleMain()"<<std::endl;	
#endif	
    //pc.fpga.tp.SaveFSRToFile(FSRFileName.c_str());
    //pc.fpga.tp.ChessMatrix(48,112);
    //pc.fpga.tp.SaveMatrixToFile(MatrixFileName.c_str());

    //call the main "command-function" w or wo FADC commands
    if(_hvFadcObjActive) UserInterfaceFadc();
    else{
    
	bool numericalInput = true;
	bool allowDefaultOnEmptyInput = false;
        const char *promptUserInterface = "Do you want to use the common (0) or the new Userinterface (1)? ";
	std::string command; 
	command = getUserInput(promptUserInterface, numericalInput, allowDefaultOnEmptyInput);
	switch( std::atoi( command.c_str() ) )
	{
    
	case 0:
	    UserInterface();
	    break;

	case 1:
	    UserInterfaceNew();
	    break;

	default:
	    std::cout << "Invalid input. Using new user interface as default." << std::endl;
	    UserInterfaceNew();

	}
    }
  
    return;
}


void Console::CommandActivateHvFadcObj(){
    // this function activates the usage of the HV_FADC object
    // after TOS was called without command line arguments 
    // e.g. not with ./TOS -v
    // This function does
    //       - warn user only 1 chip currently supported
    //           - check if only 1 chip in use, if so, continue
    //           - else ask if to change number of chips --> call SetNumChips
    //       - ask for config file to use to initialize HV_FADC object
    //       - set _hvFadcObjActive flag to true
    //       - initialize _hvFadcObj

    // flags and variables for getUserInput 
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = true;
    std::string input;
    bool activateHFO = true;

    
    if (_nbOfChips != 1){
	std::cout << "Usage of HFO (HV_FADC Object) currently limited to use of 1 chip" 
		  << std::endl;
	const char *promptNumChips = "Do you wish to set the number of Chips to 1? (y / N)";
	std::string input;
	input = getUserInput(promptNumChips, numericalInput, allowDefaultOnEmptyInput);
	if ((input == "y") ||
	    (input == "Y")){
	    // in this case, calls SetNumChips to set number of chips to 1
	    SetNumChips(1);
	}
	// if input empty, n or N do nothing
	else if ((input == "")  ||
		 (input == "n") ||
		 (input == "N")){
	    std::cout << "Number of chips will not be changed.\n"
		      << "Will not activate HFO" << std::endl;
	    activateHFO = false;
	}
    }
    // number of chips is either 1 or we will not activate HFO
    if (activateHFO == true){
	// will activate HFO
	std::string iniFilePath;

	// set HFO flag to active
	_hvFadcObjActive = true;
	
	
	const char *promptConfig = "Give the (relative) path to the HFOSettings.ini: ";
	iniFilePath = getUserInput(promptConfig, numericalInput, allowDefaultOnEmptyInput);
	if (iniFilePath == ""){
	    iniFilePath = "../config/HFOSettings.ini";
	}
	
	_hvFadcObj = new HV_FADC_Obj(iniFilePath);
	
	//init FADC
	pc.initHV_FADC(_hvFadcObj, _hvFadcObjActive);
	ok = pc.okay();
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
	    std::cout << buf << std::endl;
	    add_history( buf );
	}

	// now initialize a new std::string with the input buf
	// in order to start the case machine
	std::string ein(buf);
	
	if((ein.compare("GeneralReset")==0)||(ein.compare("1")==0)) 
	{
	    result=pc.fpga.GeneralReset();
	    if(result>10){ ErrorMessages(result); }
	    else{ std::cout<<"\tGeneralReset accomplished\n> "<<std::flush; }
	}
    
	else if(ein.compare("UserInterface")==0)
	{
	    if(_hvFadcObjActive) UserInterfaceFadc();
	    else UserInterfaceNew();

	    break;
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
	else if( (ein.compare("CountingLong")==0) ||
		 (ein.compare("2l")==0) ) 
	{
	    CommandCountingLong();
	}
	else if( ein.compare(0,16,"CountingTrigger")==0 )
	{
	    CommandCountingTrigger(ein.substr(16));
	}
	else if( ein.compare(0,3,"2t")==0 )
	{
	    CommandCountingTrigger(ein.substr(3));
	}
	else if( ein.compare(0,13,"CountingTime")==0 )
	{
	    CommandCountingTime(ein.substr(13));
	}
	else if( ein.compare(0,3,"2z")==0 )
	{
	    CommandCountingTime(ein.substr(3));
	}
	else if( ein.compare(0,3,"2f")==0 )
	{
	    CommandCountingTime_fast(ein.substr(3));
	}
	else if( ein.compare(0,3,"2l")==0 )
	{
	    CommandCountingTime_long(ein.substr(3));
	}
	else if( ein.compare(0,4,"2vl")==0 )
	{
	    CommandCountingTime_verylong(ein.substr(3));
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
	else if( ein.compare("Run")==0 )
	{
	    CommandRun();
	}
	else if( ein.compare("EnableTPulse")==0 )
	{
	    pc.fpga.EnableTPulse(1);
	}
	else if( ein.compare("DisableTPulse")==0 )
	{
	    pc.fpga.EnableTPulse(0);
	}
	else if( ein.compare("EnableFADCshutter")==0 )
	{
	    CommandFADCshutter();
	}
	else if( ein.compare("DisableFADCshutter")==0 )
	{
	    pc.fpga.EnableFADCshutter(0);
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
	else if( ein.compare("SCurveFast")==0 )
	{
	    CommandSCurveFast();
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
	else if( ein.compare(0,7,"SetDAC")==0 )
	{
	    CommandSetDAC(ein.substr(7));
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
	else if( ein.compare("SaveMatrix")==0 )
	{
	    CommandSaveMatrix();
	}
	else if( ein.compare("LoadMatrix")==0 )
	{
	    CommandLoadMatrix();
	}
	else if( ein.compare(0,8,"Trigger")==0 )
	{
	    CommandSwitchTriggerConnection(ein.substr(8));
	}
	else if( ein.compare(0,6,"SetIP")==0 )
	{
	    CommandSetIP(ein.substr(6));
	}
	else if( ein.compare("ShowIP")==0 )
	{
	    std::cout<<pc.fpga.ShowIP()<<"\n> "<<std::flush;
	}
	else if( ein.compare("MakeARP")==0 )
	{
	    pc.fpga.MakeARPEntry();
	}
	else if( ein.compare("quit")==0 )
	{
	    running=0;
	}
	else if( ein.compare("help")==0 )
	{
	    CommandHelp();
	}
	else if( ein.compare(0,8,"spacing")==0 )
	{
	    CommandSpacing(ein.substr(8));
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
	else if( ein.compare("EnableFastClock")==0 )
	{
	    pc.fpga.UseFastClock(1);
	}
	else if( ein.compare("DisableFastClock")==0 )
	{
	    pc.fpga.UseFastClock(0);
	}

	
	// ##################################################
	// ################## FADC related commands #########
	// ##################################################
	
	else if (ein.compare("PrintFADCSettings") == 0){
	    _hvFadcObj->FADC_Functions->printSettings();
	}
	else if (ein.compare("ResetFADC") == 0){
	    _hvFadcObj->F_Reset();
	}
	else if ((ein.compare("StartFadcAcquisition") == 0) ||
		 (ein.compare("StartFadcAcq")         == 0)){
	    _hvFadcObj->F_StartAcquisition();
	}
	else if (ein.compare("SendFadcSoftwareTrigger") == 0){
	    _hvFadcObj->F_SendSoftwareTrigger();	    
	}
	else if (ein.compare("ReadFadcInterrupt") == 0){
	    std::cout << "Interrupt: " << _hvFadcObj->F_ReadInterrupt() << std::endl;
	}
	else if (ein.compare("ReleaseFadcInterrupt") == 0){
	    _hvFadcObj->F_ReleaseInterrupt();
	}
	else if (ein.compare("SetFadcTriggerThresholdDACAll") == 0){
	    _hvFadcObj->F_SetTriggerThresholdDACAll( getInputValue() );
	}
	else if (ein.compare("GetFadcTriggerPerChannel") == 0){
	    std::cout << "getTriggerThreshold perChannel:" << std::endl;
	    std::cout << "Channel 1: " <<  _hvFadcObj->F_GetTriggerThresholdDACPerChannel(0) << std::endl;
	    std::cout << "Channel 2: " <<  _hvFadcObj->F_GetTriggerThresholdDACPerChannel(1) << std::endl;
	    std::cout << "Channel 3: " <<  _hvFadcObj->F_GetTriggerThresholdDACPerChannel(2) << std::endl;
	    std::cout << "Channel 4: " <<  _hvFadcObj->F_GetTriggerThresholdDACPerChannel(3) << std::endl;
	    std::cout << "getTriggerThreshold All: " << _hvFadcObj->F_GetTriggerThresholdDACAll() << std::endl << std::endl;
	}
	else if (ein.compare("SetFadcTriggerThresholdRegisterAll") == 0){
	    std::cout << "setTriggerThresholdRegisterAll returns: " 
		      <<  _hvFadcObj->FADC_Functions->setTriggerThresholdRegisterAll( getInputValue() ) << std::endl;
	}
	else if (ein.compare("GetFadcTriggerThresholdRegister") == 0){
	    _hvFadcObj->FADC_Functions->getTriggerThresholdRegister();
	}
	else if (ein.compare("LoadFadcTriggerThresholdDAC") == 0){
	    _hvFadcObj->F_LoadTriggerThresholdDAC();
	}
	else if (ein.compare("SetFadcTriggerType") == 0){
	    _hvFadcObj->F_SetTriggerType( getInputValue() );
	}
	else if (ein.compare("GetFadcTriggerType") == 0){
	    std::cout << "Trigger type: " << _hvFadcObj->F_GetTriggerType() << std::endl;
	}
	else if (ein.compare("SetFadcTriggerChannelSource") == 0){
	    _hvFadcObj->F_SetTriggerChannelSource( getInputValue() );
	}
	else if (ein.compare("GetFadcTriggerChannelSource") == 0){
	    std::cout << "Trigger channel source: " << _hvFadcObj->F_GetTriggerChannelSource() << std::endl;
	}
	else if (ein.compare("SetFadcPostTrig") == 0){
	    _hvFadcObj->F_SetPosttrig( getInputValue() );
	}
	else if (ein.compare("GetFadcPostTrig") == 0){
	    std::cout << "Posttrig: " << _hvFadcObj->F_GetPosttrig() << std::endl;
	}
	else if (ein.compare("SetFadcPreTrig") == 0){
	    _hvFadcObj->F_SetPretrig( getInputValue() );
	}
	else if (ein.compare("GetFadcPreTrig") == 0){
	    std::cout << "Pretrig: " << _hvFadcObj->F_GetPretrig() << std::endl;
	}
	else if (ein.compare("SetFadcChannelMask") == 0){
	    _hvFadcObj->F_SetChannelMask( getInputValue() );
	}
	else if (ein.compare("GetFadcChannelMask") == 0){
	    std::cout << "Channel mask: " << _hvFadcObj->F_GetChannelMask() << std::endl;
	}
	else if (ein.compare("SetFadcNumberOfChannels") == 0){
	    _hvFadcObj->F_SetNbOfChannels(getInputValue());
	}
	else if (ein.compare("GetFadcNumberOfChannels") == 0){
	    std::cout << "#Channels: " << _hvFadcObj->F_GetNbOfChannels() << std::endl;
	}
	else if (ein.compare("SetFadcModeRegister") == 0){
	    _hvFadcObj->F_SetModeRegister(static_cast<const unsigned short>(getInputValue()));
	}
	else if (ein.compare("GetFadcModeRegister") == 0){
	    std::cout << "mode register: " << _hvFadcObj->F_GetModeRegister() << std::endl;
	}
	else if (ein.compare("SetFadcFrequency") == 0){
	    _hvFadcObj->F_SetFrequency( getInputValue() );
	}
	else if (ein.compare("GetFadcFrequency") == 0){
	    std::cout << "Frequency: " << _hvFadcObj->F_GetFrequency() << std::endl;
	}
	else if (ein.compare("SetFadcReadMode") == 0){
	    _hvFadcObj->F_SetReadMode( getInputValue() );
	}
	else if (ein.compare("GetFadcReadMode") == 0){
	    std::cout << "Read mode: " << _hvFadcObj->F_GetReadMode() << std::endl;
	}
	else if (ein.compare("SetFadcPostStopLatency") == 0){
	    _hvFadcObj->F_SetPostStopLatency( getInputValue() );
	}
	else if (ein.compare("GetFadcPostStopLatency") == 0){
	    std::cout << "Post stop latency: " << _hvFadcObj->F_GetPostStopLatency() << std::endl;
	}
	else if (ein.compare("SetFadcPostLatencyPreTrig") == 0){
	    _hvFadcObj->F_SetPostLatencyPretrig( getInputValue() );
	}
	else if (ein.compare("GetFadcPostLatencyPretrig()") == 0){
	    std::cout << "Post latency pretrig: " << _hvFadcObj->F_GetPostLatencyPretrig() << std::endl;
	}

	// ##################################################
	// ################## HV_FADC related commands ######
	// ##################################################	

	// main function to call 
	else if ((ein.compare("InitHFO") == 0) ||
		 (ein.compare("InitHV_FADC") == 0))
	{
	    // if the HV_FADC object is initialized
	    if(_hvFadcObjActive == true){
		_hvFadcObj->InitHFOForTOS();
	    }
	    // if it is not initialized
	    else{
		std::cout << "Currently not using HV_FADC Object \n" 
			  << "Call ActivateHFO command and try again"
			  << std::endl;
	    }
	}

	else if (ein.compare("ShutdownHFO") == 0){
	    if (_hvFadcObjActive == true){
		_hvFadcObj->ShutdownHFOForTOS();
	    }
	}

	else if (ein.compare("ActivateHFO") == 0)
	{
	    CommandActivateHvFadcObj();
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




int Console::UserInterfaceFadc()
{
  std::cout << "Welcome to FADC Interface" << std::endl;
  printGeneralCommands(_hvFadcObjActive);

  
  
  while(_hvFadcObjActive){
    
    std::cout << ">";
    
    /*
     * For more details on all the functions, see the printCommands Functions
     * in the caseFunctions.cc file
     */
    
    std::string command; 
    std::cin >> command;    
    int errorVar = 0;

    switch( std::atoi( command.c_str() ) ){

      /*General Commands
       */
    
    case 0:
	printGeneralCommands(_hvFadcObjActive);
	break;

    case 7001:
	printFadcCommands();
	break;    

    case 50:
	printTosCommands();
	break;    

    case 57:
	if(_hvFadcObjActive)
	{
	    _hvFadcObj->F_SetTriggerType(0);
	    _hvFadcObj->F_SendSoftwareTrigger();
	    _hvFadcObjActive = false;
	    pc.initHV_FADC(NULL, _hvFadcObjActive);
	    //delete _fadcFunctions;
	    delete _hvFadcObj;
	}
	return 0;

    case 98:
	UserInterface();
	break;

    case 99:
	if(_hvFadcObjActive)
	{
	    _hvFadcObj->F_SetTriggerType(0);
	    _hvFadcObj->F_SendSoftwareTrigger();
	    //delete _fadcFunctions;
	    _hvFadcObjActive = false;
	    std::cout << "switching to TOS only" << std::endl;
        
	    delete _hvFadcObj;
	}
	UserInterfaceNew(_hvFadcObjActive);            
	return 0;

	/*FADC Commands
	 */
     
    case 7002:
	_hvFadcObj->FADC_Functions->printSettings();
	break;
    
    case 7003:
	_hvFadcObj->F_Reset();
	break;

    case 7004:
	_hvFadcObj->F_StartAcquisition();
	break;

    case 7005:
	_hvFadcObj->F_SendSoftwareTrigger();
	break;

    case 7006:
	std::cout << "Interrupt: " << _hvFadcObj->F_ReadInterrupt() << std::endl;
	break;
   
    case 7007:
	_hvFadcObj->F_ReleaseInterrupt();
	break;

    case 7008:
	_hvFadcObj->F_SetTriggerThresholdDACAll( getInputValue() );
	break;

    case 7009 :
	std::cout << "getTriggerThreshold perChannel:" << std::endl;
	std::cout << "Channel 1: " <<  _hvFadcObj->F_GetTriggerThresholdDACPerChannel(0) << std::endl;
	std::cout << "Channel 2: " <<  _hvFadcObj->F_GetTriggerThresholdDACPerChannel(1) << std::endl;
	std::cout << "Channel 3: " <<  _hvFadcObj->F_GetTriggerThresholdDACPerChannel(2) << std::endl;
	std::cout << "Channel 4: " <<  _hvFadcObj->F_GetTriggerThresholdDACPerChannel(3) << std::endl;
	std::cout << "getTriggerThreshold All: " << _hvFadcObj->F_GetTriggerThresholdDACAll() << std::endl << std::endl;
	break;

    case 7010:
	std::cout << "setTriggerThresholdRegisterAll returns: " 
		  <<  _hvFadcObj->FADC_Functions->setTriggerThresholdRegisterAll( getInputValue() ) << std::endl;
	break;
    
    case 7011:
	_hvFadcObj->FADC_Functions->getTriggerThresholdRegister();
	break;
    
    case 7012:
	_hvFadcObj->F_LoadTriggerThresholdDAC();
	break;

    case 7013:
	_hvFadcObj->F_SetTriggerType( getInputValue() );
	break;

    case 7014:
	std::cout << "Trigger type: " << _hvFadcObj->F_GetTriggerType() << std::endl;
	break;

    case 7015:
	_hvFadcObj->F_SetTriggerChannelSource( getInputValue() );
	break;

    case 7016:
	std::cout << "Trigger channel source: " << _hvFadcObj->F_GetTriggerChannelSource() << std::endl;
	break;

    case 7017:
	_hvFadcObj->F_SetPosttrig( getInputValue() );
	break;

    case 7018:
	std::cout << "Posttrig: " << _hvFadcObj->F_GetPosttrig() << std::endl;
	break;

    case 7019:
	_hvFadcObj->F_SetPretrig( getInputValue() );
	break;

    case 7020:
	std::cout << "Pretrig: " << _hvFadcObj->F_GetPretrig() << std::endl;
	break;

    case 7021:
	_hvFadcObj->F_SetChannelMask( getInputValue() );
	break;

    case 7022:
	std::cout << "Channel mask: " << _hvFadcObj->F_GetChannelMask() << std::endl;
	break;

    case 7023:
	_hvFadcObj->F_SetNbOfChannels(getInputValue());
	break;      

    case 7024:
	std::cout << "#Channels: " << _hvFadcObj->F_GetNbOfChannels() << std::endl;
	break;

    case 7027:
	_hvFadcObj->F_SetModeRegister(static_cast<const unsigned short>(getInputValue()));
	break;

    case 7028:
	std::cout << "mode register: " << _hvFadcObj->F_GetModeRegister() << std::endl;
	break;

    case 7029:
	_hvFadcObj->F_SetFrequency( getInputValue() );
	break;

    case 7030:
	std::cout << "Frequency: " << _hvFadcObj->F_GetFrequency() << std::endl;
	break;

    case 7031:
	_hvFadcObj->F_SetReadMode( getInputValue() );
	break;

    case 7032:
	std::cout << "Read mode: " << _hvFadcObj->F_GetReadMode() << std::endl;
	break;

    case 7033:
	_hvFadcObj->F_SetPostStopLatency( getInputValue() );
	break;

    case 7034:
	std::cout << "Post stop latency: " << _hvFadcObj->F_GetPostStopLatency() << std::endl;
	break;

    case 7035:
	_hvFadcObj->F_SetPostLatencyPretrig( getInputValue() );
	break;

    case 7036:
	std::cout << "Post latency pretrig: " << _hvFadcObj->F_GetPostLatencyPretrig() << std::endl;
	break;


	/*TOS Commands - code doubling from the commands of the 
	 *"userInterface function below
	 */

    case 1005:
	std::cout << "Ext-Trigger Bit: " << ReadoutFpgaExtTriggerBit() << std::endl;
	break;

    case 1006:
	std::cout << "Ext-Trigger Flag: " << ReadoutFpgaExtTriggerFlag() << std::endl;
	break;

    case 1007:
	std::cout << "Cleared Ext-Trigger Flag:" << std::endl;
	ClearFpgaExtTriggerFlag();
	break;

	//case 510:
	//break;

	//case 511:
	//break;

    case 52:
	errorVar=pc.fpga.GeneralReset();
	if(errorVar>10){
	    ErrorMessages(errorVar);
	}
	else{
	    std::cout<<"\t GeneralReset accomplished" <<std::endl;
	}
	break;

    case 53:
	CommandSetNumChips();
	break;

    case 54:
	std:: cout << "Switch Trigger Connection (0/tlu 1/lemo)" << std::endl;
	CommandSwitchTriggerConnection(getInputValue());
	break;

    case 1020:
	CommandCounting(1);
	break; 

    case 1021:
	CommandCounting(0);
	break;

    case 1022:
	CommandCountingLong();
	break;

    case 1023:
	std::cout << "counting-trigger-value" << std::endl;
	CommandCountingTrigger(getInputValue());
	break;

    case 1024:
	std::cout << "counting-time-value" << std::endl;
	CommandCountingTime(getInputValue());
	break;

    case 1025:
	std::cout << "counting-time-value fast" << std::endl;
	CommandCountingTime_fast(getInputValue());
	break;

    case 1026:
	std::cout << "counting-time-value long" << std::endl;
	CommandCountingTime_long(getInputValue());
	break;

    case 1027:
	std::cout << "counting-time-value very long" << std::endl;
	CommandCountingTime_verylong(getInputValue());
	break;

    case 1030:
	CommandReadOut();
	break;

    case 1031:
	CommandReadOut2();
	break;

    case 1040:
	CommandSetMatrix();
	break;

    case 1050:
	CommandWriteReadFSR();
	break;

    case 55:
	int tmp;
	tmp = CommandRun(_hvFadcObjActive);
	if(tmp == 1) pc.initHV_FADC(_hvFadcObj, true); 
	break;

    case 56:
	std::cout << "End measurement - someone should implement that" << std::endl;
	break;

    case 58:
	CommandHelp();
	break;

    case 591:
	std::cout << "enter IP adress - Format: [SetIP][Space][IPAdress]" << std::endl;
	command.clear();
	std::cin >> command;
	CommandSetIP(command.substr(6));
	break;

    case 592:
	std::cout << pc.fpga.ShowIP() << std::endl;
	break;

    case 60:
	pc.fpga.EnableTPulse(1);
	break;

    case 61:
	pc.fpga.EnableTPulse(0);
	break;

    case 62:
	CommandDACScan();
	break;

    case 63:
	CommandTHLScan();
	break;

    case 64:
	CommandSCurve();
	break;

    case 65:
	CommandSCurveFast();
	break;

	//case 66:
	//break;

    case 1060:
	CommandDoTHSopt();
	break;

    case 1070:
	CommandThresholdEqNoiseCenter();
	break;

    case 1080:
	CommandTOCalib();
	break;

    case 1081:
	CommandTOCalibFast();
	break;

    case 67:
	CommandLoadThreshold();
	break;

    case 68:
	CommandSaveFSR();
	break;

    case 69:
	CommandLoadFSR();
	break;

    case 70:
	std::cout << "Enter params: ";
	command.clear();
	std::cin >> command;
	CommandSetDAC(command);
	break;

    case 71:
	CommandVarChessMatrix();
	break;

    case 72:
	CommandUniformMatrix();
	break;

    case 73:
	CommandSaveMatrix();
	break;

    case 74:
	CommandLoadMatrix();
	break;

    case 75:
	pc.fpga.MakeARPEntry();
	break;

    case 76:
	std::cout << "enter spacing: " << std::endl;
	CommandSpacing(getInputValue());
	break;

    case 9999:
	_hvFadcObj->InitHFOForTOS();
	


	/*default
	 */

    default:
	std::cout << "Command not found. Try the FADC (1) Command list or the TOS (51) help" << std::endl;
    }//end switch
  }//end while

  return 0;
}


int Console::UserInterfaceNew(bool useHvFadc){
#if DEBUG==2
  std::cout<<"Enter Console::UserInterface()"<<std::endl;	
#endif

  std::cout << "Welcome to TOS Interface" << std::endl;
  printGeneralCommands(useHvFadc);
  int running=1;

  //std::cout << "> " << std::flush;
  while(running){

    std::cout << ">";
    /*
     * For more details on all the functions, see the TOS help function, the TOStorial
     * or the printCommands function in the caseFunctions.cc file
     */
    
    std::string command;
    std::cin >> command;    


    int errorVar = 0;

    switch( std::atoi( command.c_str() ) ){

    case 0:
      printGeneralCommands();
      break;

    case 7001:
      if(useHvFadc) printFadcCommands();
      else std::cout << "No fadc initialised" << std::endl;
      break;    
     
    case 7002:
      if(useHvFadc) _hvFadcObj->FADC_Functions->printSettings();
      else std::cout << "No fadc initialised" << std::endl;
      break;
    
    case 49:
      if(useHvFadc){
	UserInterfaceFadc();
        return 0;
      }
      else std::cout << "No fadc initialised" << std::endl;
      break;

    case 98:
      UserInterface();
      break;

    case 50:
      printTosCommands(useHvFadc);
      break;

      //case 510:
      //break;

      //case 511:
      //break;

    case 1005:
      std::cout << "Ext-Trigger Bit: " << ReadoutFpgaExtTriggerBit() << std::endl;
      break;

    case 1006:
      std::cout << "Ext-Trigger Flag: " << ReadoutFpgaExtTriggerFlag() << std::endl;
      break;

    case 1007:
      std::cout << "Cleared Ext-Trigger Flag:" << std::endl;
      ClearFpgaExtTriggerFlag();
      break;

    case 52:
      errorVar=pc.fpga.GeneralReset();
      if(errorVar>10){
        ErrorMessages(errorVar);
      }
      else{
        std::cout<<"\t GeneralReset accomplished" <<std::endl;
      }
      break;

      //else if(ein.compare("SetNumChips")==0){CommandSetNumChips();}
    case 53:
      CommandSetNumChips();
      break;

      //else if(ein.compare(0,8,"Trigger ")==0){CommandSwitchTriggerConnection(ein.substr(8));}
    case 54:
      std:: cout << "Switch Trigger Connection (0/tlu 1/lemo)" << std::endl;
      //CommandSwitchTriggerConnection(getAllInputValue(1));
      CommandSwitchTriggerConnection(getInputValue());
      break;

      //else if((ein.compare("Counting")==0)||(ein.compare("2")==0)) {CommandCounting(1);}
    case 1020:
      CommandCounting(1);
      break; 

      //else if((ein.compare("CountingStop")==0)||(ein.compare("2s")==0)) {CommandCounting(0);}
    case 1021:
      CommandCounting(0);
      break;

      //else if((ein.compare("CountingLong")==0)||(ein.compare("2l")==0)) {CommandCountingLong();}
    case 1022:
      CommandCountingLong();
      break;

      //else if(ein.compare(0,16,"CountingTrigger ")==0){CommandCountingTrigger(ein.substr(16));}
    case 1023:
      std::cout << "counting-trigger-value" << std::endl;
      //CommandCountingTrigger(getAllInputValue(0));
      CommandCountingTrigger(getInputValue());
      break;

      //else if(ein.compare(0,13,"CountingTime ")==0){CommandCountingTime(ein.substr(13));}
    case 1024:
      std::cout << "counting-time-value" << std::endl;
      //CommandCountingTime(getAllInputValue(0));
      CommandCountingTime(getInputValue());
      break;

      //else if(ein.compare(0,3,"2f ")==0){CommandCountingTime_fast(ein.substr(3));}
    case 1025:
      std::cout << "counting-time-value fast" << std::endl;
      //CommandCountingTime_fast(getAllInputValue(0));
      CommandCountingTime_fast(getInputValue());
      break;

      //else if(ein.compare(0,3,"2l ")==0){CommandCountingTime_long(ein.substr(3));}
    case 1026:
      std::cout << "counting-time-value long" << std::endl;
      //CommandCountingTime_long(getAllInputValue(0));
      CommandCountingTime_long(getInputValue());
      break;

      //else if(ein.compare(0,4,"2vl ")==0){CommandCountingTime_verylong(ein.substr(3));}
    case 1027:
      std::cout << "counting-time-value very long" << std::endl;
      //CommandCountingTime_verylong(getAllInputValue(0));
      CommandCountingTime_verylong(getInputValue());
      break;

      //else if((ein.compare("ReadOut")==0)||(ein.compare("3")==0)) {CommandReadOut();}
    case 1030:
      CommandReadOut();
      break;

      //else if((ein.compare("ReadOut2")==0)||(ein.compare("3a")==0)) {CommandReadOut2();}
    case 1031:
      CommandReadOut2();
      break;

      //else if((ein.compare("SetMatrix")==0)||(ein.compare("4")==0)) {CommandSetMatrix();}
    case 1040:
      CommandSetMatrix();
      break;

      //          else if(ein.compare("TestMatrix")==0){CommandTestMatrix();}
    
      //else if((ein.compare("WriteReadFSR")==0)||(ein.compare("5")==0)) {CommandWriteReadFSR();}
    case 1050:
      CommandWriteReadFSR();
      break;

      //else if(ein.compare("Run")==0){CommandRun();}
    case 55:
      int tmp;
      tmp  = CommandRun(useHvFadc);
      if(tmp == 1) pc.initHV_FADC(_hvFadcObj,true); 
      break;

    case 56:
      std::cout << "End measurement - someone should implemt that" << std::endl;
      break;

      //else if(ein.compare("quit")==0){running=0;}
    case 57:
      if(useHvFadc == true)
      {
        if(_hvFadcObj != NULL)
	{
          _hvFadcObj->F_SetTriggerType(0);
          _hvFadcObj->F_SendSoftwareTrigger();
	}
        delete _hvFadcObj;
      }
      // TODO: finish this by converting TOS to ncurses
      // for (std::list<std::string>::iterator it = commandList.begin(); it != commandList.end(); ++it){
      // 	  std:: cout << " " << *it << std::endl;
      // }
      running = 0;
      break;

      //else if(ein.compare("help")==0){CommandHelp();}
    case 58:
      CommandHelp();
      break;

      //else if(ein.compare(0,6,"SetIP ")==0){CommandSetIP(ein.substr(6));}
    case 591:
      std::cout << "enter IP adress - Format: [SetIP][Space][IPAdress]" << std::endl;
      command.clear();
      std::cin >> command;
      CommandSetIP(command.substr(6));
      break;

      //else if(ein.compare("ShowIP")==0){std::cout<<pc.fpga.ShowIP()<<"\n> "<<std::flush;}
    case 592:
      std::cout << pc.fpga.ShowIP() << std::endl;
      break;

      //else if(ein.compare("EnableTPulse")==0){pc.fpga.EnableTPulse(1);}
    case 60:
      pc.fpga.EnableTPulse(1);
      break;

      //else if(ein.compare("DisableTPulse")==0){pc.fpga.EnableTPulse(0);}
    case 61:
      pc.fpga.EnableTPulse(0);
      break;

      //else if(ein.compare("DACScan")==0){CommandDACScan();}
    case 62:
      CommandDACScan();
      break;

      //else if(ein.compare("THLScan")==0){CommandTHLScan();}
    case 63:
      CommandTHLScan();
      break;

      //else if(ein.compare("SCurve")==0){CommandSCurve();}
    case 64:
      CommandSCurve();
      break;

      //else if(ein.compare("SCurveFast")==0){CommandSCurveFast();}
    case 65:
      CommandSCurveFast();
      break;

      //		else if(ein.compare("Noise")==0){pc.NoiseScan();}

      //else if(ein.compare("ThresholdNoise")==0){CommandThresholdNoise();}
      //case 66:
      //break;

      //else if(ein.compare("THSopt")==0||(ein.compare("6")==0)){CommandDoTHSopt();}
    case 1060:
      CommandDoTHSopt();
      break;

      //else if(ein.compare("ThEqNoiseCenter")==0||(ein.compare("7")==0)) {CommandThresholdEqNoiseCenter();}
    case 1070:
      CommandThresholdEqNoiseCenter();
      break;

      //else if(ein.compare("TOCalib")==0||(ein.compare("8")==0)) {CommandTOCalib();}
    case 1080:
      CommandTOCalib();
      break;

      //else if(ein.compare("TOCalibFast")==0||(ein.compare("8a")==0)) {CommandTOCalibFast();}
    case 1081:
      CommandTOCalibFast();
      break;

      //		else if(ein.compare("FindDefPixel")==0){pc.FindDefPixel();}
      //		else if(ein.compare("ThrAdj")==0){pc.ThresholdAdjustmentNoise();}
		
      //          else if(ein.compare(0,5,"work ")==0){Commandwork(ein.substr(5)); running=0;}
		
      //          else if(ein.compare("single")==0){pc.singlescan();}
		
      //          else if(ein.compare(0,10,"speedtest ")==0){CommandSpeedTest(ein.substr(10));}
		
		
      //          else if(ein.compare("test")==0){pc.test();}
		
      //          else if(ein.compare("neu")==0){pc.NeuNoiseScan();}
      //          else if(ein.compare(0,4,"neu2")==0){if(ein.length()>4){CommandNeu2(ein.substr(4));}else{CommandNeu2("");}}
      //          else if(ein.compare("neu3")==0){pc.neu3();}

      //else if(ein.compare("LoadThreshold")==0){CommandLoadThreshold();}
    case 67:
      CommandLoadThreshold();
      break;

      //          else if(ein.compare("RauschScan")==0){CommandRauschScan();}

      //else if(ein.compare("SaveFSR")==0){CommandSaveFSR();}
    case 68:
      CommandSaveFSR();
      break;

      //else if((ein.compare("LoadFSR")==0)||(ein.compare("lf")==0)) {CommandLoadFSR();}
    case 69:
      CommandLoadFSR();
      break;

      //else if(ein.compare(0,7,"SetDAC ")==0){CommandSetDAC(ein.substr(7));}
    case 70:
      std::cout << "Enter params: ";
      command.clear();
      std::cin >> command;
      CommandSetDAC(command);
      break;

      //          else if(ein.compare(0,9,"SenseDAC ")==0){CommandSenseDAC(ein.substr(9));}
      //	        else if(ein.compare("ChessMatrix")==0){pc.fpga.tp.ChessMatrix(255,0); std::cout<<"ChessMatrix generated\n> "<<std::flush;}

      //else if(ein.compare("ChessMatrix")==0){CommandVarChessMatrix();}
    case 71:
      CommandVarChessMatrix();
      break;

      //else if((ein.compare("UniformMatrix")==0)||(ein.compare("um")==0)) {CommandUniformMatrix();}
    case 72:
      CommandUniformMatrix();
      break;

      //else if(ein.compare("SaveMatrix")==0){CommandSaveMatrix();}
    case 73:
      CommandSaveMatrix();
      break;

      //else if(ein.compare("LoadMatrix")==0){CommandLoadMatrix();}
    case 74:
      CommandLoadMatrix();
      break;

      //else if(ein.compare("MakeARP")==0){pc.fpga.MakeARPEntry();}
    case 75:
      pc.fpga.MakeARPEntry();
      break;

      //else if(ein.compare(0,8,"spacing ")==0){CommandSpacing(ein.substr(8));}
    case 76:
      std::cout << "enter spacing: " << std::endl;
      //CommandSpacing(getAllInputValue(0));
      CommandSpacing(getInputValue());
      break;

      //else{
    default:
      std::cout<<"command not found"<<std::endl;
      //}
      //std::cout<<"TOS> "<<std::flush;

    }//end switch
  }//end while

  return 0;
}


int Console::ReadoutFpgaExtTriggerBit()
{
  return pc.fpga.ReadoutFadcBit();
}


int Console::ReadoutFpgaExtTriggerFlag()
{
  return pc.fpga.ReadoutFadcFlag();
}


void Console::ClearFpgaExtTriggerFlag()
{
  pc.fpga.ClearFadcFlag();
}


void Console::ErrorMessages(int err){
	switch(err){
	    case 11: 
		std::cout << "Error "
			  << err 
			  << ": In FPGA::Communication (called by GeneralReset) - no valid file-descriptors\n> "
			  << std::flush; 
		break;
	    case 12: 
		std::cout << "Error "
			  << err 
			  << ": In FPGA::Communication (called by GeneralReset) - timeout\n> "
			  << std::flush; 
		break;
	    case 13: 
		std::cout << "Warning "
			  << err 
			  << ": In FPGA::Communication (called by GeneralReset) - wrong packet-number received\n> "
			  << std::flush; 
		break;
	    case 21: 
		std::cout << "Error "
			  << err 
			  << ": In FPGA::Communication (called by Counting) - no valid file-descriptors\n> "
			  << std::flush; 
		break;
	    case 22: 
		std::cout << "Error "
			  << err 
			  << ": In FPGA::Communication (called by Counting) - timeout\n> "
			  << std::flush; 
		break;
	    case 23: 
		std::cout << "Warning"
			  << err 
			  << ": In FPGA::Communication (called by Counting) - wrong packet-number received\n> "
			  << std::flush; 
		break;
	    case 24: 
		std::cout << "Warning"
			  << err 
			  << ": In Console::CommandCounting - Software thinks Timepix is already counting. Nothing has be done. Stop counting before you continue. If Software is wrong and Timepix is not counting, it will have any effect.\n> "
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
			  << ": In FPGA::Communication (called by ReadOut) - no valid file-descriptors\n> "
			  << std::flush; 
		break;
	    case 302: 
		std::cout << "Error "
			  << err 
			  << ": In FPGA::Communication (called by ReadOut) - timeout\n> "
			  << std::flush; 
		break;
	    case 303: 
		std::cout << "Warning "
			  << err 
			  << ": In FPGA::Communication (called by ReadOut) - wrong packet-number received\n> "
			  << std::flush; 
		break;
	    case 310: 
		std::cout << "Error "
			  << err 
			  << ": In FPGA::SaveData (called by ReadOut) - could not open DataFile\n> "
			  << std::flush; 
		break;
	    case 41: 
		std::cout << "Error "
			  << err 
			  << ": In FPGA::Communication (called by SetMatrix) - no valid file-descriptors\n> "
			  << std::flush; 
		break;
	    case 42: 
		std::cout << "Error "
			  << err 
			  << ": In FPGA::Communication (called by SetMatrix) - timeout\n> "
			  << std::flush; 
		break;
	    case 43: 
		std::cout << "Warning "
			  << err 
			  << ": In FPGA::Communication (called by SetMatrix) - wrong packet-number received\n> "
			  << std::flush; 
		break;
	    case 51: 
		std::cout << "Error "
			  << err 
			  << ": In FPGA::Communication (called by WriteReadFSR) - no valid file-descriptors\n> "
			  << std::flush; 
		break;
	    case 52: 
		std::cout << "Error "
			  << err 
			  << ": In FPGA::Communication (called by WriteReadFSR) - timeout\n> "
			  << std::flush; 
		break;
	    case 53: 
		std::cout << "Warning "
			  << err 
			  << ": In FPGA::Communication (called by WriteReadFSR) - wrong packet-number received\n> "
			  << std::flush; 
		break;
	    case 59: 
		std::cout << "Warning "
			  << err 
			  << ": In Timepix::ChipID (called by WriteReadFSR) - wrong ChipID -  received for one of the chips: "
			  << pc.fpga.ErrInfo<<", expected: "
			  << std::flush;
		for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
		    std::cout << "chip " << chip << ": " 
			      << pc.fpga.tp.GetChipID(chip) << "\n>" 
			      << std::flush;
		}
		break;
	    case 61: 
		std::cout << "Error "
			  << err 
			  << ": In Timepix::SetDAC (called by CommandSetDAC) - illegal value for this dac\n> "
			  << std::flush; 
		break;
	    case 62: 
		std::cout << "Error "
			  << err 
			  << ": In TimePix::SetDAC (called by CommandSetDAC) - illegal DAC number\n> "
			  << std::flush; 
		break;
	    case 63: 
		std::cout << "Error "
			  << err 
			  << ": In Console::SenseDAC - illegal DAC number\n> "
			  << std::flush; 
		break;
	    case 80: 
		std::cout << "Error "
			  << err 
			  << ": In TimePix::UniformMatrix - illegal input\n> "
			  << std::flush; 
		break;
	    case 81: 
		std::cout << "Error "
			  << err 
			  << ": In TimePix::VarChessMatrix - illegal input\n> "
			  << std::flush; 
		break;
	    case 82: 
		std::cout << "Error "
			  << err 
			  << ": In PC::ThresholdNoise - invalid ThrH\n> "
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
              << "\n\n> " 
              << std::flush; 

    return 1;
}


int Console::CommandSpacing(std::string ein){
// default version of CommandSpacing
    unsigned int space = 0;
    if(ein==""){space=0;}
    else if(ein.find_first_not_of("0123456789 ",0)==ein.npos){
	space=(unsigned int) atoi(&ein[0]);
    }
    else{std::cout<<"\tNon-numerical sign\n> "<<std::flush;}
    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
        pc.fpga.tp.Spacing(space,0,chip);
    }
    return 0;
}



int Console::CommandSpacing(unsigned int space){
// Command spacing used for FADC 
    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	pc.fpga.tp.Spacing(space,0,chip);
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
    pc.fpga.tp.SetNumChips(_nbOfChips,_preload);
    pc.fpga.WriteReadFSR();
    pc.fpga.WriteReadFSR();
    for (unsigned short chip = 1; chip <= pc.fpga.tp.GetNumChips(); chip++){
	pc.fpga.tp.GetChipID(chip);
    }
}


int Console::CommandSetOption(){
    unsigned short option = 0;
    std::string ein="";
    const char *prompt = "Option (0 to 255)";
    ein = getUserInput(prompt);
    if(ein==""){option=0;}
    else{
	option=(unsigned short) atoi(&ein[0]);
    }
    pc.fpga.tp.SetOption(option);
    pc.fpga.WriteReadFSR();
    pc.fpga.WriteReadFSR();
    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	pc.fpga.tp.GetChipID(chip);
    }
    return 0;
}


int Console::CommandRun(bool useHvFadc){
#if DEBUG==2
    std::cout<<"Enter Console::CommandCounting()"<<std::endl;	
#endif
  
    int result = 1;
    unsigned short runtimeFrames = 0;              //< run mode var  
    int shutter = 128;
    int runtime = 0;                               //< var to store the runtime or the nb of triggers
    unsigned short shutter_mode = 0;
    unsigned short run_mode = 0;

    // variables related to UserInput
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;
  
    //ProfilerStart();

    //Get input vals from user
    //defined runtime or trigger
    const char *promptRunParameter = "Please give parameters for run. Do you want to use a defined run time (0) or a record a defined number of triggers (1)?";
    input = getUserInput(promptRunParameter, numericalInput, allowDefaultOnEmptyInput);

    // exception handling is done in getUserInput, thus this conversion should 
    // always work
    runtimeFrames = std::stoi(input);
    std::cout << "Run parameter set to: " << runtimeFrames << std::endl;
    if (runtimeFrames == 1)
    {
	const char *promptTrigger = "(Run)\t Number of triggers you want = ";
	input = getUserInput(promptTrigger, numericalInput, allowDefaultOnEmptyInput);
	runtime = std::stoi(input);
	std::cout << "#Triggers set to: " << runtime <<  std::endl;
    }
    else if(runtimeFrames == 0)
    {
	const char *promptRuntime = "(Run)\t Runtime [sec]= ";
	input = getUserInput(promptRuntime, numericalInput, allowDefaultOnEmptyInput);
	runtime = std::stoi(input);
	std::cout << "(Run)\t Runtime [sec] set to: " << runtime << std::endl;
    }

    //Shutter mode
    const char *promptShutterMode = "(Run)\t Shutter mode (0= untiggered, 1 = external trigger, 2 = untiggered 2x faster clock, 3 = external trigger 2x faster clock, 4 = untriggered long ,5 = untriggered very long)";
    input = getUserInput(promptShutterMode, numericalInput, allowDefaultOnEmptyInput);
    shutter_mode = std::stoi(input);
    if(shutter_mode > 5)
    {
	std::cout << "Shutter mode invalid -> abort" << std::endl; 
	return 0;
    }

    //Shutter time
    const char *promptShutterTime = "(Run)\t Shutter (time [µsec] = (256*)(256*)46*x/freq[MHz], x<256), x= ";
    input = getUserInput(promptShutterTime, numericalInput, allowDefaultOnEmptyInput);
    shutter = std::stoi(input);
    if((shutter>255) || (shutter < 0))
    {
	std::cout << "(Run)\t Shutter value invalid" << std::endl; 
	return 0;
    }

    //full matrix or zero surpressed
    std::cout << "(Run)\t Run mode (0 = zero suppressed, 1 = complete matrix (slow)" << std::endl;
    if(useHvFadc) std::cout << "Choose zero surpressed if you want to use the FADC" << std::endl;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    run_mode = std::stoi(input);
    if(run_mode>1)
    {
	std::cout << "(Run)\t Run mode higher than 1 -> abort" << std::endl; 
	return 0;
    }
	
    /*
     * Set settings for the use of the fadc
     */
    if(useHvFadc)
    {
	// temporary int variable, used as flag for yes / no inputs
	int temp;
	std::cout << "Detected FADC - do you want to start a measurement with simultaneous Chip and Fadc readout? 1 = yes, 0 = no \n" 
		  << "WARNING: If Fadc is used, only one Chip is supported!"
		  << std::endl;
	
	//check if the user wants to use the fadc
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	temp  = std::stoi(input);
	if(temp != 1){
	    useHvFadc = false;
	    std::cout << "Don't use the Fadc" << std::endl; 
	}
	//set nb of chips to 1, if the user wants to use the fadc
	else{
	    if(_nbOfChips != 1) std::cout << "WARNING: Nb of chips set to one - preload-value is kept as " << _preload << std::endl;
	    _nbOfChips = 1;
	    pc.fpga.tp.SetNumChips(_nbOfChips,_preload);

	    //print fadc settings
	    _hvFadcObj->FADC_Functions->printSettings();

	    std::cout << "Return to main menu to change some Fadc settings? 1 = y, 0 = n \n" 
		      << "If yes, this aborts the run."
		      << std::endl;
	    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	    temp  = std::stoi(input);
	    if(temp == 1){
		std::cout << "Aborting Run - display menu by entering 0" << std::endl;
		return 0; 
	    }      
	}
    }//end of if(useHvFadc)

    if(runtime==0){
	std::cout << "\t\tRun starts now. <q> to stop\n> " << std::endl;
    }

    // Start measurement
    if(useHvFadc) result = pc.DoRun(runtimeFrames, runtime, shutter, 0, shutter_mode, run_mode, useHvFadc);
    else 
    {
	// if Fadc not used, init FADC with NULL and call standard DoRun function
	pc.initHV_FADC(NULL,false);
	result = pc.DoRun(runtimeFrames, runtime, shutter, 0, shutter_mode, run_mode);
    }

    // print error message
    if(result>0){
	ErrorMessages(90+result); 
	return -1;
	std::cout << "Run closed" << std::endl;
    }
  
    //FIXME: drop this?
    if((!useHvFadc) && _hvFadcObjActive) return 1;
    else return 0;	
}//end CommandRun


int Console::CommandCounting(int c){
#if DEBUG==2
    std::cout<<"Enter Console::CommandCounting()"<<std::endl;	
#endif
    int result=0;
    if(c!=0){
	if(pc.fpga.tp.IsCounting()!=0){ErrorMessages(24);}
	else{	
	    result=pc.fpga.Counting();
	    if(result>20){ErrorMessages(result);}
	    else{std::cout<<"\tCounting started\n> "<<std::flush;}
	}
    }
    else{
	if(pc.fpga.tp.IsCounting()==0){ErrorMessages(25);}
	result=pc.fpga.CountingStop();
	if(result>20){ErrorMessages(result);}
	else{std::cout<<"\tCounting stopped\n> "<<std::flush;}
    }
    return result;
}

// TODO: take out this function or rewrite
int Console::CommandCountingLong(){ //outdated, not used any more, rewrite!
#if DEBUG==2
    std::cout<<"Enter Console::CommandCountingLong()"<<std::endl;
#endif
    int result=0;
    int n;
    int t;
    std::string ein="";
    bool numericalInput           = true;
    bool allowDefaultOnEmptyInput = true;


    const char *promptOpeningTime = " Opening time [us] ";
    ein = getUserInput(promptOpeningTime, numericalInput, allowDefaultOnEmptyInput);
    if(ein==""){t=20000;}
    else{
	t=(unsigned int) atoi(ein.data());
    }

    const char *promptFrames = " Frames wanted ";
    ein = getUserInput(promptFrames, numericalInput, allowDefaultOnEmptyInput);
    if(ein==""){n=1;}
    else{
	n=(unsigned int) atoi(ein.data());
    }

    result=pc.fpga.GeneralReset();
    std::cout<<"Reset done"<<std::endl;
    result=pc.fpga.tp.LoadMatrixFromFile(pc.GetMatrixFileName(1),1);
    std::cout<<"Matrix loaded from "<<pc.GetMatrixFileName(1)<<"\n> "<<std::flush;
    result=CommandLoadFSR();
    result=pc.fpga.tp.SetDAC(6,1,312);
    result=CommandSetMatrix();
    result=CommandWriteReadFSR();
    result=CommandReadOut();
    result=CommandSetMatrix();
    result=CommandWriteReadFSR();
    result=CommandReadOut();
    std::cout<<"read back matrix done"<<std::endl;


    std::ostringstream sstream;
    for (int i=0; i<n; i++){
	if(pc.fpga.tp.IsCounting()!=0){ErrorMessages(24);}
	else{
	    result=pc.fpga.Counting();
	    if(result>20){ErrorMessages(result);}
	    else{std::cout<<"\tCounting started\n> "<<std::flush;}
	}
	usleep(t);
	if(pc.fpga.tp.IsCounting()==0){ErrorMessages(25);}
	result=pc.fpga.CountingStop();
	if(result>20){ErrorMessages(result);}
	else{std::cout<<"\tCounting stopped\n> "<<std::flush;}
	usleep(10000);
	std::string filename[8]= {""};
	const char* f[8];
	for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	    filename[chip]=pc.GetDataPathName();
	    filename[chip]+="/";
	    filename[chip]+=pc.GetDataFileName(chip);
	    filename[chip]+=sstream.str();
	    sstream.str("");
	    f[chip] = filename[chip].c_str();
	}
	//filename+=pc.GetDataFileName();
	result = pc.DoReadOut2(f[1],1);
	std::cout<<"CommandReadOut:"<<result<<std::endl;
	if(result<0){ErrorMessages(-result);}
	else{std::cout<<"\tReadOut accomplished\n> "<<std::flush;}
	/*sstream<<"data"<<MeasuringCounter<<".txt";
	  FileName=PathName+"/"; FileName+=sstream.str();
	  sstream.str("");
	  result=DoReadOut2(FileName.c_str());
	*/
	if (i%2000 == 0 && i>1){
	    result=pc.fpga.GeneralReset();
	    std::cout<<"Reset done"<<std::endl;
	    //result=pc.fpga.tp.LoadMatrixFromFile(pc.GetMatrixFileName());
	    //std::cout<<"Matrix loaded from "<<pc.GetMatrixFileName()<<"\n> "<<std::flush;
	    //result=CommandLoadFSR();
	    //result=pc.fpga.tp.SetDAC(6,312);
	    result=CommandSetMatrix();
	    result=CommandWriteReadFSR();
	    result=CommandReadOut();
	    result=CommandSetMatrix();
	    result=CommandWriteReadFSR();
	    result=CommandReadOut();
	    std::cout<<"read back matrix done"<<std::endl;
	}
    }
    return result;
}


int Console::CommandCountingTrigger(std::string ein){
    // default CommandCountingTrigger function
#if DEBUG==2
	std::cout<<"Enter Console::CommandCountingTrigger()"<<ein<<std::endl;	
#endif
	int result=0;
	unsigned int time;	
	if(ein.find_first_not_of("0123456789 ",0)==ein.npos){
		time=(unsigned int) atoi(&ein[0]);
	}
	else{std::cout<<"\tNon-numerical sign\n> "<<std::flush; return 0;}
	result=(time<256);
	if(result){
        result=pc.fpga.CountingTrigger(time);
		if(result>20){ErrorMessages(result);}
		else{std::cout<<"\tCountingTrigger accomplished\n> "<<std::flush;}
	}
	return 1;
}

int Console::CommandCountingTrigger(unsigned int time){
    // CommandCountingTrigger function used for Fadc
#if DEBUG==2
    std::cout<< "Enter Console::CommandCountingTrigger(unsigned int time) " << time << std::endl;	
#endif
    int result=0;
    result=(time<256);
    if(result){
	result=pc.fpga.CountingTrigger(time);
	if(result>20){ErrorMessages(result);}
	else{std::cout<<"\tCountingTrigger accomplished\n> "<<std::flush;}
    }
    return 1;
}

int Console::CommandCountingTime(std::string ein){
    // default CommandCountingTime function
#if DEBUG==2
    std::cout<<"Enter Console::CommandCountingTime(std::string ein)"<<ein<<std::endl;	
#endif
    int result=0;
    unsigned int time;	
    if(ein.find_first_not_of("0123456789 ",0)==ein.npos){
	time=(unsigned int) atoi(&ein[0]);
    }
    else{std::cout<<"\tNon-numerical sign\n> "<<std::flush; return 0;}
    result=(time<256);
    if(result){
        result=pc.fpga.CountingTime(time);
	if(result>20){ErrorMessages(result);}
	else{std::cout<<"\tCountingTime accomplished\n> "<<std::flush;}
    }
    return 1;
}

int Console::CommandCountingTime(unsigned int time){
    // CommandCountingTime function used for Fadc
#if DEBUG==2
    std::cout<<"Enter Console::CommandCountingTime() " << time <<std::endl;	
#endif
    int result=0;
    result=(time<256);
    if(result){
	result=pc.fpga.CountingTime(time);
	if(result>20){ErrorMessages(result);}
	else{std::cout<<"\tCountingTime accomplished\n> "<<std::flush;}
    }
    return 1;
}

int Console::CommandCountingTime_fast(std::string ein){
    // default CommandCountingTime_fast function
#if DEBUG==2
    std::cout<<"Enter Console::CommandCountingTime()"<<ein<<std::endl;
#endif
    int result=0;
    unsigned int time;
    if(ein.find_first_not_of("0123456789 ",0)==ein.npos){
	time=(unsigned int) atoi(&ein[0]);
    }
    else{std::cout<<"\tNon-numerical sign\n> "<<std::flush; return 0;}
    result=(time<256);
    if(result){
        result=pc.fpga.CountingTime_fast(time);
	if(result>20){ErrorMessages(result);}
	else{std::cout<<"\tCountingTime accomplished\n> "<<std::flush;}
    }
    return 1;
}

int Console::CommandCountingTime_fast(unsigned int time){
    // CommandCountingTime_fast function used for Fadc
#if DEBUG==2
    std::cout<<"Enter Console::CommandCountingTime() "<< time << std::endl;
#endif
    int result=0;
    result=(time<256);
    if(result){
	result=pc.fpga.CountingTime_fast(time);
	if(result>20){ErrorMessages(result);}
	else{std::cout<<"\tCountingTime accomplished\n> "<<std::flush;}
    }
    return 1;
}

int Console::CommandCountingTime_long(std::string ein){
    // default CommandCountingTime_long function
#if DEBUG==2
    std::cout<<"Enter Console::CommandCountingTime()"<<ein<<std::endl;
#endif
    int result=0;
    unsigned int time;
    if(ein.find_first_not_of("0123456789 ",0)==ein.npos){
	time=(unsigned int) atoi(&ein[0]);
    }
    else{std::cout<<"\tNon-numerical sign\n> "<<std::flush; return 0;}
    result=(time<256);
    if(result){
        result=pc.fpga.CountingTime_long(time);
	if(result>20){ErrorMessages(result);}
	else{std::cout<<"\tCountingTime accomplished\n> "<<std::flush;}
    }
    return 1;
}

int Console::CommandCountingTime_long(unsigned int time){
    // CommandCountingTime_long function used for Fadc
#if DEBUG==2
    std::cout<<"Enter Console::CommandCountingTime() "<< time <<std::endl;
#endif
    int result=0;
    result=(time<256);
    if(result){
	result=pc.fpga.CountingTime_long(time);
	if(result>20){ErrorMessages(result);}
	else{std::cout<<"\tCountingTime accomplished\n> "<<std::flush;}
    }
    return 1;
}

int Console::CommandCountingTime_verylong(std::string ein){
    // default CommandCountingTime_verylong function
#if DEBUG==2
    std::cout<<"Enter Console::CommandCountingTime()"<<ein<<std::endl;
#endif
    int result=0;
    unsigned int time;
    if(ein.find_first_not_of("0123456789 ",0)==ein.npos){
	time=(unsigned int) atoi(&ein[0]);
    }
    else{std::cout<<"\tNon-numerical sign\n> "<<std::flush; return 0;}
    result=(time<256);
    if(result){
        result=pc.fpga.CountingTime_verylong(time);
	if(result>20){ErrorMessages(result);}
	else{std::cout<<"\tCountingTime accomplished\n> "<<std::flush;}
    }
    return 1;
}

int Console::CommandCountingTime_verylong(unsigned int time){
    // CommandCountingTime_verylong function used for Fadc
#if DEBUG==2
    std::cout<<"Enter Console::CommandCountingTime() " << time <<std::endl;
#endif
    int result=0;
    result=(time<256);
    if(result){
	result=pc.fpga.CountingTime_verylong(time);
	if(result>20){ErrorMessages(result);}
	else{std::cout<<"\tCountingTime accomplished\n> "<<std::flush;}
    }
    return 1;
}


int Console::CommandReadOut(){
#if DEBUG==2
    std::cout << "Enter Console::CommandReadOut()"  <<  std::endl;	
#endif
    int result;
    std::string filename[9]= {""};
    const char* f[9];

    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	filename[chip]=pc.GetDataPathName();
	filename[chip]+="/";
	filename[chip]+=pc.GetDataFileName(chip);
	f[chip] = filename[chip].c_str();
    }
    result = pc.DoReadOut(f);
    std::cout << "CommandReadOut:" << result << std::endl;
    if(result<0){ErrorMessages(-result);}
    else{std::cout << "\tReadOut accomplished\n> " << std::flush;}
    return result;
}


int Console::CommandReadOut2(){
#if DEBUG==2
    std::cout << "Enter Console::CommandReadOut()" << std::endl;
#endif
    int result = 0;
    pc.fpga.DataChipFPGA(result);

    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	std::string filename=pc.GetDataPathName();
	filename+="/";
	filename+=pc.GetDataFileName(chip);
	result = pc.DoReadOut2(filename.c_str(),chip);
#if DEBUG==2
	std::cout << "DEBUG: Filename: " << filename << std::endl;
#endif    
	std::cout<<"CommandReadOut:"<< result <<" for chip "<< chip << std::endl;
	if(result<0){ErrorMessages(-result);}
	else{std::cout<<"\tReadOut accomplished\n> "<<std::flush;}
	//filename = "";
    }
    return result;
}


int Console::CommandSetMatrix(){
#if DEBUG==2
    std::cout<<"Enter Console::CommandSetMatrix()"<<std::endl;	
#endif
    int result;
    result=pc.fpga.SetMatrix();
    if(result>40){ErrorMessages(result);}
    else{std::cout<<"\tSetMatrix accomplished\n> "<<std::flush;}
    return result;
}

int Console::CommandSaveMatrix(){
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = false;

    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	std::string ein;
	const char* f=pc.GetMatrixFileName(chip);
	std::cout << "Matrix filename for chip "
		  << chip 
		  << ": (press ENTER to save in "
		  << pc.GetMatrixFileName(chip) 
		  << "): " 
		  << std::endl;
	ein = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	f=ein.c_str();
	pc.fpga.tp.SaveMatrixToFile(f,chip);
	std::cout<<"Matrix saved to "<<f<<"\n> "<<std::flush;
    }
    return 0;
}


int Console::CommandLoadMatrix(){
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = false;

    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	std::string ein;
	const char* f=pc.GetMatrixFileName(chip);
	std::cout << "Matrix filename for chip "
		  << chip
		  << " (press ENTER to load from "
		  << pc.GetMatrixFileName(chip) 
		  << "): " 
		  << std::endl;
	ein = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	f=ein.c_str();
	FILE* f1=fopen(f,"r"); 
	if(f1==NULL) {
	    std::cout<<"File not found"<<std::endl; 
	    return -1;
	}
	if (f1 != NULL) {
	    pc.fpga.tp.LoadMatrixFromFile(f,chip);
	    std::cout<<"Matrix loaded from "<<f<<"\n> "<<std::flush;
	}
	pc.fpga.tp.SaveMatrixToFile(pc.GetMatrixFileName(chip),chip);
	std::cout<<"Matrix saved to program folder as "<<pc.GetMatrixFileName(chip)<<"\n> "<<std::flush;
    }
    return 0;
}


int Console::CommandWriteReadFSR(){
#if DEBUG==2
    std::cout<<"Enter Console::CommandWriteReadFSR()\n"<<std::flush;
#endif
    int result;
    result=pc.fpga.WriteReadFSR();
    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	pc.fpga.tp.GetChipID(chip);
    }
    std::cout<<"\tWriteReadFSR accomplished\n> "<<std::flush;
    return result;
}


int Console::CommandSaveFSR(){
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = false;

    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	std::string ein;
	const char* f=pc.GetFSRFileName(chip);
	std::cout << "FSR filename for chip "
		  << chip
		  << ": (press ENTER to save in " 
		  << pc.GetFSRFileName(chip) 
		  << "): " 
		  << std::endl;
	ein = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	f=ein.c_str();
	pc.fpga.tp.SaveFSRToFile(f,chip);
	std::cout<<"FSR saved in "<<f<<"\n> "<<std::flush;
    }
    return 0;
}


int Console::CommandLoadFSR(){
#if DEBUG==2
    std::cout<<"Enter Console::CommandLoadFSR()"<<std::endl;	
#endif	
    int err = 0;
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = false;

    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	std::string ein;
	const char* f=pc.GetFSRFileName(chip);
	std::cout << "FSR filename for chip "
		  << chip
		  << " (press ENTER to load from "
		  << pc.GetFSRFileName(chip) 
		  << "): " 
		  << std::endl;
	ein = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	f=ein.c_str();
	FILE* f1=fopen(f,"r"); 
	if (f1 == NULL) {
	    std::cout << "File not found"
		      << std::endl; 
	    return -1;
	}
	if (f1 != NULL) {
	    err=pc.fpga.tp.LoadFSRFromFile(f,chip);
	    if(err==1){
		std::cout << "FSR loaded from " << f << "\n> " << std::flush;
	    }
	    else{
		std::cout << "Error in " << f << " in row " << -err << "\n> " 
			  << std::flush;
	    }
	}
	pc.fpga.tp.SaveFSRToFile(pc.GetFSRFileName(chip),chip);
	std::cout << "FSR saved to program folder as " << pc.GetFSRFileName(chip) << "\n> " << std::flush;
    }
    return err;
}


int Console::CommandLoadThreshold(){
#if DEBUG==2
    std::cout << "Enter Console::CommandLoadFSR()" << std::endl;	
#endif	
    int err = 0;
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = false;

    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	std::string ein;
	const char* f=pc.GetThresholdFileName(chip);
	std::cout << "Threshold filename for chip "
		  << chip
		  << " (press ENTER to load from "
		  << pc.GetThresholdFileName(chip) 
		  << "): " 
		  << std::endl;
	ein = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	f=ein.c_str();
	FILE* f1=fopen(f,"r");
	if(f1==NULL) {
	    std::cout << "File not found" << std::endl;
	    return -1;
	}
	if (f1 != NULL) {
	    err=pc.fpga.tp.LoadThresholdFromFile(f,chip);
	    if(err==-1){ 
		std::cout << "File " << f << " not found" << std::endl;
	    }
	    std::cout << "Threshold loaded from " << f << "\n> " << std::flush;
	}
	pc.fpga.tp.SaveThresholdToFile(pc.GetThresholdFileName(chip),chip);
	std::cout<<"Threshold saved to program folder as "<<pc.GetThresholdFileName(chip)<<"\n> "<<std::flush;
    }
    return err;
}


int Console::CommandSetDAC(std::string ein){
#if DEBUG==2
    std::cout<<"Enter Console::CommandSetDAC()"<<std::endl;	
#endif	
    if(ein.find_first_not_of("0123456789 ",0)==ein.npos){
	int pos=ein.find(" ",0); 
	int dac=(unsigned int) atoi(&ein[0]);
	unsigned short chip =(unsigned int) atoi(&ein[pos]);
	int pos2=ein.find(" ",3);
	std::cout<<"pos2="<<atoi(&ein[pos2])<<std::endl;
	int i = (unsigned int) atoi(&ein[pos2]);
	int err = 1;
	if (chip > pc.fpga.tp.GetNumChips()){ 
	    std::cout << "You only have " 
		      << pc.fpga.tp.GetNumChips() 
		      << " chips, please set DAC in format: DAC chip value" 
		      << std::endl;err=0;
	}
	else err=pc.fpga.tp.SetDAC(dac,chip,i);
	if(err!=1) ErrorMessages(61-err);
	else std::cout << "DAC " << dac 
		       << " (" << pc.fpga.tp.GetDACName(dac) << ") of chip " << chip 
		       << " set to " << i 
		       << std::endl;
    }
    return 1;
}


int Console::CommandShowFSR(){
    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	for(unsigned int i=0;i<18;i++){
	    std::cout << "\t" << pc.fpga.tp.GetDACName(i) << " \t " << pc.fpga.tp.GetDAC(i,chip) << std::endl;
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

    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	std::cout<<"Chip Number "<<chip<<std::endl;
	std::cout<<"\t Number cols per field="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	sl = std::stoi(input);
	std::cout<<"\t Number row per field="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	wl = std::stoi(input);
	std::cout<<"\t Black P0="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	sp0 = std::stoi(input);
	std::cout<<"\t Black P1="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	sp1 = std::stoi(input);
	std::cout<<"\t Black Mask="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	smask = std::stoi(input);
	std::cout<<"\t Black Test="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	stest = std::stoi(input);
	std::cout<<"\t Black Threshold="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	sth = std::stoi(input);
	std::cout<<"\t White P0="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	wp0 = std::stoi(input);
	std::cout<<"\t White P1="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	wp1 = std::stoi(input);
	std::cout<<"\t White Mask="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	wmask = std::stoi(input);
	std::cout<<"\t White Test="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	wtest = std::stoi(input);
	std::cout<<"\t White Threshold="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	wth = std::stoi(input);
	std::cin.ignore(1);
	err=pc.fpga.tp.VarChessMatrix(sl,wl,sp0,sp1,smask,stest,sth,wp0,wp1,wmask,wtest,wth,chip);
	if(err==0){std::cout<<"Matrix created\n> "<<std::flush;}
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

    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	std::cout<<"Chip Number "<<chip<<std::endl;
	std::cout<<"\t P0="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	p0 = std::stoi(input);
	std::cout<<"\t P1="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	p1 = std::stoi(input);
	std::cout<<"\t Mask="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	mask = std::stoi(input);
	std::cout<<"\t Test="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	test = std::stoi(input);
	std::cout<<"\t Threshold="<<std::flush;
	input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	th = std::stoi(input);
	std::cin.ignore(1);
	err=pc.fpga.tp.UniformMatrix(p0,p1,mask,test,th,chip);
	if(err==0){std::cout<<"Matrix created\n> "<<std::flush;}
	else{ErrorMessages(80);}
    }

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
    closeshutter = std::stoi(input);
    pc.fpga.tp.SetI2C(closeshutter);
    pc.fpga.EnableFADCshutter(1);
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
    std::cout << "Which chip do you want to DAC scan? (1-" << pc.fpga.tp.GetNumChips() << ") " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    chip = std::stoi(input);
    std::cout << "Choose DACs to scan (1=Yes, 0=No) \n" << std::flush;
    std::cout << "\t IKrum " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[0] = std::stoi(input);
    std::cout << "\t Disc " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[1] = std::stoi(input);
    std::cout << "\t Preamp " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[2] = std::stoi(input);
    std::cout << "\t BuffAnalogA " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[3] = std::stoi(input);
    std::cout << "\t BuffAnalogB " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[4] = std::stoi(input);
    std::cout << "\t Hist " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[5] = std::stoi(input);
    std::cout << "\t THL " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[6] = std::stoi(input);
    std::cout << "\t Vcas " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[7] = std::stoi(input);
    std::cout << "\t FBK " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[8] = std::stoi(input);
    std::cout << "\t GND " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[9] = std::stoi(input);
    std::cout << "\t THS " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[10] = std::stoi(input);
    std::cout << "\t BiasLVDS " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[11] = std::stoi(input);
    std::cout << "\t RefLVDS " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    DacsOn[12] = std::stoi(input);
    std::cout << "\t Coarse " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
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

    pc.DoDACScan(DACstoScan, chip);
    return 0;

}


int Console::CommandTHLScan(){
    // variables for getUserInput
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;

    unsigned short chip;
    unsigned short coarselow;
    unsigned short coarsehigh;
    std::cout << "Which chip do you want to THL scan? (1-" << pc.fpga.tp.GetNumChips() << ") " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    chip = std::stoi(input);
    std::cout << "Start coarse " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    coarselow = std::stoi(input);
    std::cout << "End coarse " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    coarsehigh = std::stoi(input);
    pc.DoTHLScan(chip,coarselow,coarsehigh);
    return 0;
}

int Console::CommandSCurve(){
    // variables for getUserInput
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;

    unsigned short chip = 1;
    unsigned short coarselow = 7;
    unsigned short coarsehigh = 7;
    int time = 255;
    std::cout << "Which chip do you want to THL scan to write the files for the S-Curve scan? (1-" << pc.fpga.tp.GetNumChips() << ") " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    chip = std::stoi(input);
    std::cout << "Shutter time in clock cycles (0-255), LONG mode used " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    time = std::stoi(input);
    std::cout << "Start coarse " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    coarselow = std::stoi(input);
    std::cout << "End coarse " << std::flush;
    input = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    coarsehigh = std::stoi(input);
    pc.DoSCurveScan(chip,coarselow,coarsehigh,time);
    return 0;
}


int Console::CommandSCurveFast(){
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
	      <<  pc.fpga.tp.GetNumChips() 
	      << ") " 
	      << std::flush;
    std::cout << "Warning: Only CTPR = 1 will be used. Hence only column x = 0, "
	      << "x= 32, ... Make sure that NONE of this columns is dead. "
	      << "Otherwise put a column offset ( 0(no offset) to 31)" 
	      << std::flush;
    input      = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    offset     = std::stoi(input);
    std::cout << "What voltage did you set on pulser, put 0 for internal pulser?" 
	      << std::flush;
    input      = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    voltage    = std::stoi(input);
    std::cout << "Shutter time in clock cycles (0-255), LONG mode used "
	      << "(100 is ok for internal pulser) " 
	      << std::flush;
    input      = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
    time       = std::stoi(input);
    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	std::cout << "Start (lower) THL for chip " << chip << " " << std::flush;
	input          = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	StartTHL[chip] = std::stoi(input);
	std::cout << "End (upper) THL for chip " << chip << " " << std::flush;
	input          = getUserInput(_prompt, numericalInput, allowDefaultOnEmptyInput);
	StopTHL[chip]  = std::stoi(input);
    }
    pc.DoSCurveScan_meanChip(voltage,time,StartTHL,StopTHL,offset);
    return 0;
}


int Console::Commandi2creset(){
    pc.fpga.i2creset();
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
    DACchannel = std::stoi(input);
    const char *promptDACvoltage = "I2C DAC voltage (mV): ";
    input = getUserInput(promptDACvoltage, numericalInput, allowDefaultOnEmptyInput);
    Umv   = std::stoi(input);
    pc.fpga.i2cDAC(Umv, DACchannel);
    return 0;
}


int Console::Commandi2cADC(){
    unsigned short channel = 0;
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;
    const char *promptADCchannel = "ADC channel to read: ";
    input = getUserInput(promptADCchannel, numericalInput, allowDefaultOnEmptyInput);
    std::cout << "ADC channel is: " << channel << std::endl;
    pc.fpga.i2cADC(channel);
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
    Npulses = std::stoi(input);
    const char *promptFreqTestPulse = "Testpulse frequency: Divide 500 kHz by (1 - 50): ";
    input     = getUserInput(promptFreqTestPulse, numericalInput, allowDefaultOnEmptyInput);    
    div500kHz = std::stoi(input);
    pc.fpga.tpulse(Npulses, div500kHz);
    return 0;
}


int Console::CommandDoTHSopt(){

    unsigned short doTHeq = 0;
    unsigned short pix_per_row_THeq = 1;
    unsigned short chp = 1;

    std::string ein0="";
    std::cout << "Hello, this is THS optimisation.  You have " 
	      << pc.fpga.tp.GetNumChips() 
	      << " chips, for which of them do you want to do the optimization? "
	      << "Put 0 if you want to go for all of them one after the other. " 
	      << std::endl;
    ein0 = getUserInput(_prompt);
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
	if(ein4==""){max_thl = 1023;}
	else{
	    max_thl=(unsigned short) atoi(ein4.data());
	}
	std::string ein5="";
	ein5 = getUserInput(_prompt);
	if(ein5==""){min_thl = 0;}
	else{
	    min_thl=(unsigned short) atoi(ein5.data());
	}
    }
    if (chp == 0){
	for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	    pc.DoTHSopt(doTHeq, pix_per_row_THeq, chip, ths, ext_coarse, max_thl, min_thl);
	}
    }
    else pc.DoTHSopt(doTHeq, pix_per_row_THeq, chp, ths, ext_coarse, max_thl, min_thl);
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
	      << pc.fpga.tp.GetNumChips() 
	      << "chips, for which of them do you want to do the threshold "
	      << "equalisation? Put 0 if you want to go for all of them "
	      << "one after the other." 
	      << std::endl;
    ein = getUserInput(_prompt);
    if(ein==""){chp=1;}
    else{
	chp=(unsigned short) atoi(ein.data());
    }
    std::cout << "chip " << chp << std::endl;
    std::cout << " You have " 
	      << pc.fpga.tp.GetNumChips() 
	      << " chips, equalisation will be done for chip " 
	      << chp 
	      << std::flush;
    std::string ein1="";
    std::cout << " How many pixel per row at same time? 1,2,4,8,16 "
	      << "(more is not good!)? " 
	      << std::endl;
    ein1 = getUserInput(_prompt);
    if(ein1==""){pix_per_row=1;}
    else{
	pix_per_row=(unsigned short) atoi(ein1.data());
    }
    std::cout << pix_per_row << " pixel per row at same time" << std::endl;
    std::string ein3="";
    const char *promptUseExtendedTHLRange = "Do you want to use extended coarse thl range (coarse 6 and 8 in addition to 7)? (1=yes, 0=no) ";
    ein3 = getUserInput(promptUseExtendedTHLRange);
    if(ein3==""){ext_coarse=0;}
    else{
	ext_coarse = (unsigned short) atoi(ein3.data());
    }
    if (ext_coarse==0){
	std::string ein4="";
	std::cout << " Use upper and lower thl bound? Then set upper first, then lower. Default is 1023 and 0. " << std::flush;
	ein4 = getUserInput(_prompt);
	if(ein4==""){max_thl = 1023;}
	else{
	    max_thl=(unsigned short) atoi(ein4.data());
	}
	std::string ein5="";
	ein5 = getUserInput(_prompt);
	if(ein5==""){min_thl = 0;}
	else{
	    min_thl=(unsigned short) atoi(ein5.data());
	}
    }
    if (chp == 0){
	for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	    pc.DoThresholdEqCenter(pix_per_row, chip, ext_coarse, max_thl, min_thl);
	}
    }
    else pc.DoThresholdEqCenter(pix_per_row, chp, ext_coarse, max_thl, min_thl);
    return 0;
}


int Console::CommandTOCalib(){
    pc.TOCalib();
    return 0;
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
	      << pc.fpga.tp.GetNumChips() 
	      <<" chips, all of them will be calibrated at the same time. "
	      << std::endl;
    const char *promptExtIntPulser = "Do you want to use the external (0, default) or internal (1) pulser? ";
    std::string ein4="";
    ein4 = getUserInput(promptExtIntPulser);
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
    if(ein2==""){shuttertype=1;}
    else{
	shuttertype = (unsigned short) atoi(ein2.data());
    }
    std::cout << "Set shutter length in clock cycles(0 -255) (if you have "
	      << "chosen long shutter this value will be multiplied by 256):" 
	      << std::endl;
    std::string ein3="";
    ein3 = getUserInput(_prompt);
    if(ein3==""){time=1;}
    else{
	time=(unsigned short) atoi(ein3.data());
    }
    std::cout << "Ok, lets start!" << std::endl;

    pc.TOCalibFast(pix_per_row, shuttertype, time, TOT, internalPulser);
    return 0;
}


int Console::CommandCheckOffset(){
    unsigned short usepreload = pc.CheckOffset();
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
    unsigned short doTOT = 0;
    if(ein==""){doTOT=0;}
    else{
	doTOT=(unsigned short) atoi(ein.data());
    }
    const char *promptTOA = "Do you want to do a TOA calibration (0: no, 1: yes)";
    std::string ein0="";
    ein0 = getUserInput(promptTOA);
    unsigned short doTOA = 0;
    if(ein0==""){doTOA=0;}
    else{
	doTOA=(unsigned short) atoi(ein0.data());
    }
    unsigned short chp = 1;
    std::string ein1="";
    std::cout << ""  << std::endl;
    std::cout << "You have " 
	      <<  pc.fpga.tp.GetNumChips() 
	      << " chips" 
	      << std::endl;
    std::cout << "Which chips do you want to calibrate? Put 0 if you want to "
	      << "go for all of them one after the other. "
	      <<std::endl;
    ein1 = getUserInput(_prompt);
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
	if(ein4==""){max_thl = 1023;}
	else{
	    max_thl=(unsigned short) atoi(ein4.data());
	}
	std::string ein5="";
	const char *promptLower = "Lower: ";
	ein5 = getUserInput(promptLower);
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
    offset     = std::stoi(input);
    std::cout << "Shutter time in clock cycles (0-255), LONG mode used "
	      << "(100 is ok for internal pulser) " 
	      << std::flush;
    input      = getUserInput(_prompt, true, false);
    time       = std::stoi(input);
    for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	std::cout << "End (upper) THL for chip "   << chip << ": " << std::flush;
	input          = getUserInput(_prompt, true, false);
	StopTHL[chip]  = std::stoi(input);
	std::cout << "Start (lower) THL for chip " << chip << ": " << std::flush;
	input          = getUserInput(_prompt, true, false);
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
		  << pc.fpga.tp.GetNumChips() 
		  <<" chips, all of them will be calibrated at the same time. " 
		  << std::endl;
	std::cout << "For the spacing: How many pixel per row at same time? "
		  << "1,2,4,8,16 (more is not good!)? "
		  << std::endl;
	std::string ein7="";
	ein7 = getUserInput(_prompt);
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
	if(ein9==""){timeTO=1;}
	else{
	    timeTO=(unsigned short) atoi(ein9.data());
	}
	std::cout << "Ok, lets start!"<<std::endl;
    }
    if (chp == 0){
	for (unsigned short chip = 1;chip <= pc.fpga.tp.GetNumChips() ;chip++){
	    pc.DoTHSopt(0, 0, chip, ths, ext_coarse, max_thl, min_thl);
	    pc.DoThresholdEqCenter(pix_per_row, chip, ext_coarse, max_thl, min_thl);
	}
    }
    else {
	pc.DoTHSopt(0, 0, chp, ths, ext_coarse, max_thl, min_thl);
	pc.DoThresholdEqCenter(pix_per_row, chp, ext_coarse, max_thl, min_thl);
    }
    pc.DoSCurveScan_meanChip(voltage, time, StartTHL, StopTHL, offset);
    if (doTOT == 1) {
	pc.TOCalibFast(pix_per_rowTO, shuttertype, timeTO, 0, internalPulser);
    }
    if (doTOA == 1) {
	pc.TOCalibFast(pix_per_rowTO, shuttertype, timeTO, 1, internalPulser);
    }
    return 0;
}


int Console::CommandSwitchTriggerConnection(std::string ein){
    // default CommandSwitchTriggerConnection function
#if DEBUG==2
    std::cout << "Enter Console::CommandCountingTime()"
	      << ein
	      << std::endl;	
#endif
    if(ein.compare("tlu")==0){pc.fpga.SwitchTriggerConnection(1);}
    else if( ein.compare("lemo")==0 ){
	pc.fpga.SwitchTriggerConnection(0);
    }
    else{
	std::cout << "\tTrigger: Unknown option \""
		  << ein
		  <<"\". See help\n> "
		  <<std::flush; 
	return 1;
    }
    return 0;
}


int Console::CommandSwitchTriggerConnection(int trigCon){
    // CommandSwitchTriggerConnection function used for Fadc
#if DEBUG==2
    std::cout<<"Enter Console::CommandCountingTime() " << trigCon <<std::endl;	
#endif

    if(trigCon == 1){
	std::cout << "set to tlu" << std::endl;
	pc.fpga.SwitchTriggerConnection(1);
    }
    else if(trigCon == 0){
	pc.fpga.SwitchTriggerConnection(0);
	std::cout << "set to lemo" << std::endl;
    }
    else {std::cout<<"\tTrigger: Unknown option \""<< trigCon <<"\". See help\n> "<<std::flush; return 1;}
    return 0;
}




void Console::DACScanLive(char dac, int val){
#if DEBUG==2
    std::cout<<"Enter Console::CommandDACScanLive()"<<std::endl;	
#endif
    if(dac>=0){std::cout << std::endl << pc.fpga.tp.GetDACName((unsigned int) dac) << "\n";}
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
	    pc.SpeedTest(wdh,freq);
	}
	else{
	    std::cout << "\tNon-numerical sign\nTOS> "
		      << std::flush; 
	    return;
	}
    }
}


void Console::CommandSetIP(std::string ein){
    unsigned int i;
    int pos,n;
    int byte[4]={0};
    std::string str;
    std::stringstream ip;

    if((ein.length()>5)&&(ein.length()<16)&&(ein.find_first_not_of("0123456789.",0)==ein.npos)){
	n=0; i=0; pos=-1;
	while(n<3){
	    while( (i<ein.length()) && (i-pos<4) && (ein[i]!='.') ){++i;}
	    if(ein[i]=='.'){
		str=ein.substr(pos+1,i-pos-1);
		byte[n]=atoi(&str[0]);
	    }
	    else{
		std::cout << "Invalid IP\n> " 
			  << std::flush; 
		return;
	    }
	    if(byte[n]>255){
		std::cout << "Invalid IP\n> "
			  << std::flush; 
		return;
	    }
	    ++n; pos=i; ++i;
	}
	while( (i<ein.length()) && (ein[i]!='.') ){++i;}
	if( (ein[i]=='.') || (i-pos>4) || (i-pos==1) ){
	    std::cout << "Invalid IP\n> " 
		      << std::flush; 
	    return;
	}
	else{
	    str=ein.substr(pos+1,i-pos-1);
	    byte[n]=atoi(&str[0]);
	}
	if(byte[n]>255){
	    std::cout << "Invalid IP\n> " 
		      << std::flush; 
	    return;
	}   
    }
    else{
	std::cout << "Invalid IP\n> " 
		  << std::flush; 
	return;
    }

    ip<<byte[0]<<'.'<<byte[1]<<'.'<<byte[2]<<'.'<<byte[3];
    pc.fpga.SetIP(ip.str());
}
