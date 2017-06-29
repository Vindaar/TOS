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
    int nChips = 0;
    nChips = CommandSetNumChips(false);
    std::cout << "Number of Chips: " << nChips << std::endl << std::endl;

    // create a pointer to a timepix object, with the correct number of chips
    _tp = new Timepix(nChips);
    // and now create a PC object and hand it the timepix object pointer
    pc = new PC(_tp);

    // now that we have created timepix object with correct number of chips
    // call SetNumChips to write that number to _nbOfChips and determine preload
    SetNumChips(nChips);

    // now using the number of chips, create the _chip_set
    _chip_set = CreateChipSet(_nbOfChips);
    // now given this chip set, we hand a reference to this chip set to
    // the PC, FPGA and Timepix class
    UpdateChipSetReference();

    ok = pc->okay();
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
    // now using the number of chips, create the _chip_set
    _chip_set = CreateChipSet(_nbOfChips);
    // now given this chip set, we hand a reference to this chip set to
    // the PC, FPGA and Timepix class
    UpdateChipSetReference();
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
	    iniFilePath = "./config/HFM_settings.ini";
	}


	// set HFM flag to active (only after last user input call!)
	_hvFadcManagerActive = true;
	
	_hvFadcManager = new hvFadcManager(iniFilePath);
	
	//init FADC
	pc->initHV_FADC(_hvFadcManager, _hvFadcManagerActive);
	ok = pc->okay();
    }


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
	    for(auto chip : _chip_set){
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
    for (auto chip : _chip_set){
        pc->fpga->tp->Spacing(space,0,chip);
    }
    return 0;
}

int Console::CommandSetNumChips(bool callSetNumChips){
    // this function uses getUserInput to get the number
    // of chips to be set and then calls the
    // SetNumChips(int nChips) function to actually
    // set this number internally
    // inputs:
    //     bool callSetNumChips: determines whether we call SetNumChips
    //         to set number of chips to member variable _nbOfChips and
    //         call function to get preload immediately (== true) or whether 
    //         to return number of chips (== false).
    //         Default is == true
    int nChips;

    const char *prompt = "How many chips are on your board? (1-8)";
    std::set<std::string> allowedNumChips;
    std::string input;
    // make sure only 1 to 8 chips are typed in. More than 9 (due to illogical
    // numbering of chips starting from 1 instead of 0) and we will get a
    // segmentation fault in the timepix creator. Size of arrays for chips
    // are hardcoded at the moment to allow for 9 chips.
    for(int l = 1; l < 9; l++) allowedNumChips.insert(std::to_string(l));
    input = getUserInputNumericalDefault(prompt, &allowedNumChips);
    if (input == "quit"){
	return -1;
    }
    else if (input == ""){
	nChips = 1;
    }
    else{
	nChips = (unsigned short) atoi(&input[0]);
    }

    if (callSetNumChips == true){
	// got user input, now call
	SetNumChips(nChips);
	return 0;
    }
    else{
	return nChips;
    }
}

void Console::SetNumChips(unsigned short nChips){
    // this function sets the number of chips to nChips
    // it is not called Command*, because it is not callable
    // by the user from ther user interface, but rather is the internal
    // function to set the number of chips, which is called by
    // CommandSetNumChips and at other points in the code (e.g. setting
    // number of chips to 1, in order to use the HV_FADC object)
    _nbOfChips = nChips;
    // using new nChips value, create new chip_set
    _chip_set = CreateChipSet(nChips);

    _preload = getPreload();
    pc->fpga->tp->SetNumChips(_nbOfChips,_preload);
    pc->fpga->WriteReadFSR();
    pc->fpga->WriteReadFSR();
    for (auto chip : _chip_set){
	pc->fpga->tp->GetChipID(chip);
    }

    // after we have set the number of chips again, pass down the new chip set
    UpdateChipSetReference();
}

void Console::UpdateChipSetReference(){
    // this function updates the reference to the chip set member
    // variable of the console object, such that PC, FPGA and Timepix
    // always have a (const!) reference to the chip set
    pc->SetChipSet(_chip_set);
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
    for (auto chip : _chip_set){
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
    std::string filename[9] = {""};
    std::string f[9];

    result = pc->DoReadOut();
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

    for (auto chip : _chip_set){
	std::string filename;//=pc->GetDataPathName();
//	filename+="/";
	filename = pc->GetDataFileName(chip);

	// now check whether we're using the FADC
	if (_hvFadcManagerActive == true){
	    // in this case use 
	    result   = pc->DoReadOutFadc(chip);
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

int Console::CommandWriteReadFSR(){
#if DEBUG==2
    std::cout<<"Enter Console::CommandWriteReadFSR()\n"<<std::flush;
#endif
    int result;
    result=pc->fpga->WriteReadFSR();
    for (auto chip : _chip_set){
	pc->fpga->tp->GetChipID(chip);
    }
    std::cout<<"\tWriteReadFSR accomplished\n"<<std::flush;
    return result;
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

    if (chip > _nbOfChips){ 
	std::cout << "You only have " 
		  << _nbOfChips 
		  << " chips, please provide correct chip number." 
		  << std::endl;
	err = 64;
    }
    else{
	err = pc->SetDACandWrite(dac, chip, i);
	std::cout << "NOTE: DAC set (and written to chip!). Take this message out "
		  << "later, once this is common knowledge." << std::endl;
    }
    if(err != 1) ErrorMessages(err);
    else{
	std::cout << "DAC " << dac 
		  << " (" << pc->fpga->tp->GetDACName(dac) << ") of chip " << chip 
		  << " set to " << i 
		  << std::endl;
    }
    return 1;
}


int Console::CommandShowFSR(){
    for (auto chip : _chip_set){
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

void Console::CommandVarChessMatrixDefault(){
    // this function creates a default chess matrix of 5 by 5 pixels

    int length 	   = 5;
    int width  	   = 5;
    int black_p0   = 1;
    int black_p1   = 0;
    int black_mask = 1;
    int black_test = 0;
    int black_thr  = 5;
    int white_p0   = 1;
    int white_p1   = 0;
    int white_mask = 1;
    int white_test = 1;
    int white_thr  = 5;
    int err        = 0;

    for (auto chip : _chip_set){
	err = pc->fpga->tp->VarChessMatrix(length,
					   width,
					   black_p0,
					   black_p1,
					   black_mask,
					   black_test,
					   black_thr,
					   white_p0,
					   white_p1,
					   white_mask,
					   white_test,
					   white_thr,
					   chip);
	if(err == 0){
	    std::cout << "Default Matrix created\n" << std::flush;
	}
	else{
	    ErrorMessages(80);
	}
    }

    return;
}

int Console::CommandVarChessMatrix(bool all_chips){
    // command to create a chess matrix to be written to each chip
    // inputs:
    //     bool all_chips: if this flag is true, we set a single chess matrix
    //                     for all chips
#if DEBUG==2
    std::cout<<"Enter Console::CommandSenseDAC()"<<std::endl;	
#endif
    int err = 0;

    // first we're going to let the user decide which chips to calibrate
    // define a set of ints, which will contain the number of the chips we're
    // going to use
    std::set<unsigned short> chip_set;
    chip_set = ChipSelection();

    std::map<std::string, int> chess_matrix_map;
    if (all_chips == true){
	chess_matrix_map = ChessMatrixSelection(0);
	if (chess_matrix_map.empty()){
	    return -1;
	}
    }
    
    for (auto chip : chip_set){
	// for the selected chips we now either set the same chess matrix (all_chips == true)
	// or individual matrices (all_chips == false)
	if (all_chips == false){
	    // in this case ask for each chip for a matrix
	    chess_matrix_map = ChessMatrixSelection(chip);
	    if (chess_matrix_map.empty()){
		return -1;
	    }	    
	}
	err = pc->fpga->tp->VarChessMatrix(chip, chess_matrix_map);
	if (err == 0){
	    std::cout << "Matrix created" << std::endl;
	}
	else{
	    ErrorMessages(80);
	}
    }

    return err;
}

int Console::CommandGetMatrixAsIntAndDump(){

    FrameArray<int> matrix;
    matrix = pc->fpga->tp->GetMatrixAsInts(1);
    std::string filename;
    Frame frame;
    frame.SetFrame(matrix);
    frame.ConvertFullFrameFromLFSR();
    filename = GetFrameDumpFilename(0, 0, 0);
    frame.DumpFrameToFile(filename);
    return 0;    
}

int Console::CommandUniformMatrix(){
    int p0,p1,test,mask,th;
    int err = 0;

    // variables for getUserInput
    bool numericalInput = true;
    bool allowDefaultOnEmptyInput = false;
    std::string input;

    for (auto chip : _chip_set){
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
    for (auto chip : _chip_set){
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
    std::cout << "Which chip do you want to DAC scan? (0-" << _nbOfChips << ") " << std::flush;
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

void Console::RunTHLScan(std::set<unsigned short> chip_set,
			 std::pair<int, int> coarse_boundaries,
			 std::pair<int, int> threshold_boundaries,
			 std::string shutter_range,
			 std::string shutter_time,			 
			 std::atomic_bool *loop_stop){

    // in this case perform for all chips
    for (auto chip : chip_set){
	pc->DoTHLScan(chip,
		      coarse_boundaries,
		      threshold_boundaries,
		      shutter_range,
		      shutter_time,
		      loop_stop);	    
    }
    return;
}

int Console::CommandTHLScan(){
    // variables for getUserInput
    std::string input;
    std::string shutter_range;
    std::string shutter_time;

    // first we're going to let the user decide which chips to calibrate
    // define a set of ints, which will contain the number of the chips we're
    // going to use
    std::set<unsigned short> chip_set;
    chip_set = ChipSelection();

    // coarse boundary seleection
    std::pair<int, int> coarse_boundaries;
    coarse_boundaries = CoarseBoundarySelection();
    if(coarse_boundaries.first  == 0 &&
       coarse_boundaries.second == 0) return -1;
    
    // threshold boundary seleection
    std::pair<int, int> threshold_boundaries;
    threshold_boundaries = THLBoundarySelection();
    if(threshold_boundaries.first  == 0 &&
       threshold_boundaries.second == 0) return -1;

    // shutter type selection
    shutter_range = ShutterRangeSelection();
    
    // shutter time selection
    shutter_time  = ShutterTimeSelection(shutter_range);


    // now loop over fpga->tpulse to pulse..
    // create seperate thread, which loops and will be stopped, if we type stop in terminal
        
    std::atomic_bool loop_stop;
    loop_stop = false;
    std::thread loop_thread(&Console::RunTHLScan,
			    this,
			    chip_set,
			    coarse_boundaries,
			    threshold_boundaries,
			    shutter_range,
			    shutter_time,
			    &loop_stop);
    //loop_thread.start();
    const char *waitingPrompt = "THL scan is running. type 'q' to quit> ";
    std::set<std::string> allowedStrings = {"q"};
    input = getUserInputNonNumericalNoDefault(waitingPrompt, &allowedStrings);
    if (input == "stop" || input == "q" || input == "quit"){
	loop_stop = true;
    }
    loop_thread.join();

    return 0;
}

void Console::CommandSCurve(){
    // This function does an SCurve scan. Scans THL values in a range
    // and outputs the mean number of active pixels due to test pulses
    // of a given height. Results in a bell shaped curve with a plateau
    // on either side
    // Based on previous SCurveFast function
    // rewritten in a cleaner way
    // TODO: describe what the function does exactly
    // - choose which chips to do SCurve scan for
    // - choose internal or external pulser
    // - choose pulse heights if internal pulser
    
    // variable which stores pulser (internal or external)
    std::string pulser;
    // variable which stores CTPR value
    std::string CTPR_str;
    int CTPR;
    // shutter range (set to 1 for SCurve scan)
    std::string shutter_range = "long";
    // shutter time
    std::string shutter_time;
    std::pair<int, int> threshold_boundaries;
    std::string input;

    // first we're going to let the user decide which chips to calibrate
    // define a set of ints, which will contain the number of the chips we're
    // going to use
    std::set<unsigned short> chip_set;
    chip_set = ChipSelection();

    // ask user for CTPR value to choose
    CTPR_str = CTPRSelection();
    if (CTPR_str == "quit") return;
    std::cout << "using " << CTPR_str << std::endl;
    CTPR = std::stoi(CTPR_str);
    
    
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
    std::string callerFunction = "SCurve";
    pulseList = PulseListCreator(pulser, callerFunction);
    if (pulseList.front() == -1) return;

    // shutter time selection
    std::cout << "Shutter range is set to 'long'." << std::endl;
    shutter_time  = ShutterTimeSelection(shutter_range);
    if (shutter_time == "quit") return;

    // threshold boundary seleection
    threshold_boundaries = THLBoundarySelection();
    if(threshold_boundaries.first  == 0 &&
       threshold_boundaries.second == 0) return;
    
    // in case of an external pulser, we will allow the user to add another voltages
    // to the list after the first voltage has finished

    if (pulser == "internal"){
	// in case we use the internal pulser, we only allow to use the pulse list 
	// created before
	pc->SCurve("SCurve",
		   chip_set,
		   pulser,
		   pulseList,
		   shutter_range,
		   shutter_time,
		   CTPR,
		   threshold_boundaries);
    }
    else if (pulser == "external"){
	// in case of an external trigger, we ask whether the user wants to add 
	// another voltage after the first one has finished
	do{
	    pc->SCurve("SCurve",
		       chip_set,
		       pulser,
		       pulseList,
		       shutter_range,
		       shutter_time,
		       CTPR,
		       threshold_boundaries);
	    
	    // ask for more input
	    std::cout << "Do you wish to use another external pulse voltage? (y / n)" 
		      << std::endl;
	    input = getUserInputNonNumericalNoDefault(_prompt, {"y", "n", "Y", "N"});
	    if (input == "quit") return;
	    else if( (input == "y") ||
		     (input == "Y") ){
		// in this case ask for another value
		pulseList = PulseListCreator(pulser, callerFunction);
	    }
	    else{
		// in this case we just finish
		std::cout << callerFunction << " calibration finished." << std::endl;
		
	    }
	} while ( (input != "n") || (input != "N") );
    }

}
    // :)



int Console::CommandSCurveOld(){    
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
    	      << "Scan will be done for one chip after the other from chip 0 to " 
    	      <<  _nbOfChips 
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
    for (auto chip : _chip_set){
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

    /*
      to create test pulses do one of the following:
      - set i2c DACs (2, upper and 3, lower) to some appropriate value
      --> loop over this
          - call fpga::tpulses function 
          - call fpga::EnableTPulse and set to true
          --> test pulses will be created
          - call fpga::EnableTPulse and set to false


      or do the following:
      - set i2c DACs (2, upper and 3, lower) to some appropriate value
      - call fpga::tpulses function 
      - call fpga::EnableTPulse and set to true
      -- loop over this:
          - call fpga::CountingTime function
	  --> test pulses will be created
     */
    std::cout << "producing test pulses, now!" << std::endl;
    std::cout << "loop stop is " << _loop_stop << std::endl;
    while(_loop_stop == false){
	pc->fpga->tpulse(1000, 10);
	pc->fpga->EnableTPulse(1);
	//pc->fpga->CountingTime(100, 1);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	pc->fpga->EnableTPulse(0);
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
    
    //pc->fpga->EnableTPulse(1);
    // now loop over fpga->tpulse to pulse..
    // create seperate thread, which loops and will be stopped, if we type stop in terminal
    std::thread loop_thread(&Console::runTestPulses, this);
    _loop_stop = false;
    //loop_thread.start();
    const char *waitingPrompt = "test pulses running. type 'stop' to quit> ";
    std::set<std::string> allowedStrings = {"stop"};
    input = getUserInputNonNumericalNoDefault(waitingPrompt, &allowedStrings);
    if (input == "stop" || input == "quit"){
	_loop_stop = true;
    }
    loop_thread.join();
    pc->fpga->EnableTPulse(0);
    
    return 0;
}

int Console::CommandDoTHSopt(){

    bool doTHeq = 0;
    short pix_per_col_THeq = 1;

    // first we're going to let the user decide which chips to calibrate
    // define a set of ints, which will contain the number of the chips we're
    // going to use
    std::set<unsigned short> chip_set = ChipSelection();
    if (chip_set.size() == 0){
	return -1;
    }

    std::string input = "";
    std::cout << "I will do a few threshold scans as in THeq, but with 16 pixels"
	      << " active per row at same time (but only one step). Only 4096 "
	      << "pixels are used. Do you want to do a THeq directly afterwards?"
	      << " (no extended coarse THeq possible) (yes / NO) " 
	      << std::endl;
    std::set<std::string> allowedStrings = {"y", "Y", "yes", "YES",
					    "n", "N", "no", "NO"};
    input = getUserInputNonNumericalDefault(_prompt, &allowedStrings);
    if (input == "quit") return -1;
    else if(input == ""){
	doTHeq = false;
    }
    else if (input == "yes" || input == "y" || input == "YES" || input == "Y"){
	doTHeq = true;
    }
    else{
	doTHeq = false;
    }
    std::cout << "doTHeq= " << doTHeq << std::endl;

    if (doTHeq == true){
	std::cout << "For THeq after THSopt choose:" << std::endl;
	pix_per_col_THeq = PixPerColumnSelection();
	if(pix_per_col_THeq == -1){
	    return -1;
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
    for (auto chip : chip_set){
	pc->DoTHSopt(doTHeq, pix_per_col_THeq, chip, ths, ext_coarse, max_thl, min_thl);
    }
    return 0;
}


int Console::CommandThresholdEqNoiseCenter(){
    std::string ein;
    short pix_per_col = 0;
    short ext_coarse = 0;
    short max_thl = 1023;
    short min_thl = 0;

    // first we're going to let the user decide which chips to calibrate
    // define a set of ints, which will contain the number of the chips we're
    // going to use
    std::set<unsigned short> chip_set = ChipSelection();
    if (chip_set.size() == 0){
	return -1;
    }

    // now determine the pixels per columns to use
    pix_per_col = PixPerColumnSelection();
    if (pix_per_col == -1) return -1;

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
    for (auto chip : chip_set){
	pc->DoThresholdEqCenter(pix_per_col, chip, ext_coarse, max_thl, min_thl);
    }
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
    std::set<unsigned short> chip_set;
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
    std::string callerFunction = "TOCalib";
    pulseList = PulseListCreator(pulser, callerFunction);
    
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
	pc->TOCalib(callerFunction,
		    chip_set,
		    TOmode,
		    pulser,
		    pulseList,
		    pixels_per_column,
		    shutter_range,
		    shutter_time);
    }
    else if (pulser == "external"){
	// in case of an external trigger, we ask whether the user wants to add 
	// another voltage after the first one has finished
	do{
	    pc->TOCalib(callerFunction,
			chip_set,
			TOmode,
			pulser,
			pulseList,
			pixels_per_column,
			shutter_range,
			shutter_time);
	    
	    // ask for more input
	    std::cout << "Do you wish to use another external pulse voltage? (y / n)" 
		      << std::endl;
	    input = getUserInputNonNumericalNoDefault(_prompt, {"y", "n", "Y", "N"});
	    if (input == "quit") return;
	    else if( (input == "y") ||
		     (input == "Y") ){
		// in this case ask for another value
		pulseList = PulseListCreator(pulser, callerFunction);
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
	      << _nbOfChips 
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
	      <<  _nbOfChips 
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
    for (auto chip : _chip_set){
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
		  << _nbOfChips 
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
	for (auto chip : _chip_set){
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
    const char *prompt = "Set the center chip (range: 0 to num_chips - 1): ";
    std::set<std::string> allowedChipNum;
    // make sure only 0 to NumChips - 1 chips are typed in
    for(auto chip : _chip_set){
	allowedChipNum.insert(std::to_string(chip));
    }
    
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

    bool log_flag = true;

    std::thread loop_thread(temp_auslese_main, &loop_continue, log_flag);
    const char *waitingPrompt = "temp readout running. type 'stop' to quit> ";
    std::string input;
    std::set<std::string> allowedStrings = {"stop"};
    input = getUserInputNonNumericalNoDefault(waitingPrompt, &allowedStrings);
    if (input == "stop"){
    	loop_continue = false;
    }
    loop_thread.join();

}
