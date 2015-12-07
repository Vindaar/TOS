// this file contains the implementations of the tos command completion functions
// all possible commands have to be added to the (so far) const char* array in
// the header file
// They are based on the GNU readline library:
// https://cnswww.cns.cwru.edu/php/chet/readline/rltop.html

#include "tosCommandCompletion.hpp"

const char *tosCommands[] = {"GeneralReset", 
                             "1", 
                             "UserInterface", 
                             "Counting", 
                             "2", 
                             "CountingStop", 
                             "2s", 
                             "CountingLong", 
                             "2l", 
                             "CountingTrigger", 
                             "2t", 
                             "CountingTime", 
                             "2z", 
                             "2f", 
                             "2l", 
                             "2vl", 
                             "ReadOut", 
                             "3", 
                             "ReadOut2", 
                             "3a", 
                             "SetMatrix", 
                             "4", 
                             "WriteReadFSR", 
                             "5", 
                             "Run", 
                             "EnableTPulse", 
                             "DisableTPulse", 
                             "EnableFADCshutter", 
                             "DisableFADCshutter", 
                             "DACScan", 
                             "THLScan", 
                             "SCurve", 
                             "SCurveFast", 
                             "i2creset", 
                             "i2cDAC", 
                             "i2cADC", 
                             "tpulse", 
                             "THSopt", 
                             "6", 
                             "ThEqNoiseCenter", 
                             "7", 
                             "TOCalib", 
                             "8", 
                             "TOCalibFast", 
                             "8a", 
                             "LoadThreshold", 
                             "SaveFSR", 
                             "LoadFSR", 
                             "lf", 
                             "SetDAC ", 
                             "ShowFSR", 
                             "ChessMatrix", 
                             "UniformMatrix", 
                             "um", 
                             "SaveMatrix", 
                             "LoadMatrix", 
                             "Trigger", 
                             "SetIP", 
                             "ShowIP", 
                             "MakeARP", 
                             "quit", 
                             "help", 
                             "spacing ", 
                             "SetNumChips", 
                             "SetOption", 
                             "CheckOffset", 
                             "Calibrate", 
                             "EnableFastClock", 
                             "DisableFastClock",
// HV_FADC commands
			     "InitHFO",
			     "InitHV_FADC",
			     "ShutdownHFO",
			     "ActivateHFO",
			     "CheckHVModuleIsGood",
			     "ConnectModule",
// FADC commands
			     "PrintFADCSettings",
			     "ResetFADC",
			     "StartFadcAcquisition",
			     "StartFadcAcq",
			     "SendFadcSoftwareTrigger",
			     "ReadFadcInterrupt",
			     "ReleaseFadcInterrupt",
			     "SetFadcTriggerThresholdDACAll",
			     "GetFadcTriggerPerChannel",
			     "SetFadcTriggerThresholdRegisterAll",
			     "GetFadcTriggerThresholdRegister",
			     "LoadFadcTriggerThresholdDAC",
			     "SetFadcTriggerType",
			     "GetFadcTriggerType",
			     "SetFadcTriggerChannelSource",
			     "GetFadcTriggerChannelSource",
			     "SetFadcPostTrig",
			     "GetFadcPostTrig",
			     "SetFadcPreTrig",
			     "GetFadcPreTrig",
			     "SetFadcChannelMask",
			     "GetFadcChannelMask",
			     "SetFadcNumberOfChannels",
			     "GetFadcNumberOfChannels",
			     "SetFadcModeRegister",
			     "GetFadcModeRegister",
			     "SetFadcFrequency",
			     "GetFadcFrequency",
			     "SetFadcReadMode",
			     "GetFadcReadMode",
			     "SetFadcPostStopLatency",
			     "GetFadcPostStopLatency",
			     "SetFadcPostLatencyPreTrig",
			     "GetFadcPostLatencyPretrig()"};


char *TOS_Command_Generator(const char *text, int state){

    static int list_index, length;
    const char *name;
    
    // if in good state
    if (!state){
        list_index = 0;
        length = strlen(text);
    }

    while ( (name = tosCommands[list_index]) ){
        list_index++;
        if (strncmp (name, text, length) == 0){
            return strdup (name);
        }
    }

    // if no names matched, then return NULL
    return ((char *) NULL);
}

// custom completion function
//static
char **TOS_Command_Completion(const char *text, int start, int end){
    // this prevents appending space to the end of the matching word
    // commented out for now. not too bad? but not all that useful either
    rl_completion_append_character = '\0';

    char **matches = (char **) NULL;
    if (start == 0){
        matches = rl_completion_matches ((char *) text, &TOS_Command_Generator);
    }
    // else rl_bind_key('\t', rl_abort);
    return matches;
}

std::string getUserInput(const char *prompt, 
			 bool numericalInput, 
			 bool allowDefaultOnEmptyInput){
    // char *prompt: given char array is the prompt
    //               presented to the user on input
    // e.g.: prompt == "TOS> " then this will be prepended to the input.
    // for most calls to this function use "" empty string or NULL
    // bool numericalInput: if this flag is set, only allow numerical input
    // bool allowDefaultOnEmptyInput: if this flag is set, allow empty input
    //                                and thus set a default value later

    char *buf;
    while ( true ){
	buf = readline(prompt);

	if (allowDefaultOnEmptyInput == false && buf[0] == '\0'){
	    free(buf);
	    continue;
	}
	else if ((buf != NULL) && 
		 (allowDefaultOnEmptyInput == true) && 
		 (buf[0] == '\0')){
	    // in this case, no EOF encountered, line is empty and we allow
	    // empty lines to set default, simply break
	    break;
	}
	else if (buf != NULL){
	    // buf is non empty and no EOF encountered
	    // create a temporary string to perform proper exception
	    // handling (stoi throws exceptions, atoi does not)
	    std::string tempStr(buf);

	    // if numericalInput is set to true, check whether input is numerical
	    if (numericalInput == true){
		// try to convert tempStr to int, if non numerical, will throw
		// invalid_argument exception
		try{
		    // do not need to use return value, only interested in 
		    // exception 
		    std::stoi(tempStr);
		    // if no exception is thrown, break from while loop
		    break;
		}
		catch (const std::invalid_argument &ia) {
		    // if this is catched, argument non numerical
		    std::cout << "Argument non numerical; try again" 
			      << std::endl;
		}
	    }// end numericalInput
	    else{
		// in this case we accept all input
		break;
	    }
	}//end non empty buffer
    }//end while loop

    // now buf should be non empty
    // initialize a string with the char* and return it
    std::string input(buf);
    if (buf != NULL) free(buf);
    return input;
}


// convenience functions to simply call getUserInput with certain flags
std::string getUserInputNumericalDefault(const char *prompt){      
    return getUserInput(prompt, true, true);
}

std::string getUserInputNumericalNoDefault(const char *prompt){    
    return getUserInput(prompt, true, false);
}

std::string getUserInputNonNumericalDefault(const char *prompt){   
    return getUserInput(prompt, false, true);
}

std::string getUserInputNonNumericalNoDefault(const char *prompt){ 
    return getUserInput(prompt, false, false);
}
