// this file contains the implementations of the tos command completion functions
// all possible commands have to be added to the (so far) const char* array in
// the header file
// They are based on the GNU readline library:
// https://cnswww.cns.cwru.edu/php/chet/readline/rltop.html

#include "tosCommandCompletion.hpp"
#include <cstring>


namespace fs = boost::filesystem;


const std::set<std::string> get_tos_commands(){
    // function which returns all tos commands
    const std::set<std::string> tosCommandsSet {"GeneralReset", 
						"1", 
						"UserInterface", 
						"Counting", 
						"2", 
						"CountingStop", 
						"2s", 
						"CountingLong", 
						"CountingTrigger", 
						"2t", 
						"CountingTime", 
						"2z", 
						"2f", 
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
						"DACScan", 
						"THLScan", 
						"SCurve", 
						"i2creset", 
						"i2cDAC", 
						"i2cADC", 
						"tpulse",
						"TestTPulse",
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
						"LoadFSRAll",
						"lfa",
						"SetDAC", 
						"ShowFSR",
						"ChessMatrix",
						"ChessMatrixAll",
						"DefaultChessMatrix",
						"GetMatrixAndDump",
						"UniformMatrix", 
						"um",
						"UniformMatrixAllChips",
						"uma",
						"SaveMatrix", 
						"LoadMatrix", 
						"Trigger", 
						"SetIP", 
						"ShowIP", 
						"MakeARP", 
						"quit", 
						"help", 
						"spacing",
						"SetPreload",
						"SetNumChips",
						"SetOption",
						"CheckOffset",
						"CheckOffsetZeroSuppressed",
						"CheckOffsetFullMatrix",						
						"Calibrate", 
						"SetChipIDOffset",
						"EnableFastClock", 
						"DisableFastClock",
// MCP2210 temperature readaout commands
						"TempLoopReadout"};

    return tosCommandsSet;
}

const std::set<std::string> get_hfm_commands(){
    // HV and FADC commands
    const std::set<std::string> HFMCommandsSet =  {"InitHFM",
						   "InitHV_FADC",
						   "RampChannels",
						   "ShutdownHFM",
						   "ActivateHFM",
						   "CheckHVModuleIsGood",
						   "ConnectModule",
						   "TurnChannelOnOff",
						   "ClearChannelEventStatus",
						   "PrintChannelStatus",
						   "PrintChannelEventStatus",
						   "PrintModuleStatus",
						   "PrintModuleEventStatus",
						   "AddChannel",
						   "RemoveChannel",
						   "AddFlexGroup",
						   "RemoveFlexGroup",
						   "PrintActiveChannels",
						   "SetChannelValue",
// FADC commands
						   "PrintFADCSettings",
						   "ResetFADC",
						   "SetFadcSettings",
						   "StartFadcPedestalRun",
						   "StartFadcAcquisition",
						   "StartFadcAcq",
						   "SetFadcTriggerLevel",
						   "EnableFADCshutter", 
						   "DisableFADCshutter", 
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
						   "GetFadcPostLatencyPretrig()",
						   "SetFadcSleepAcqTime",
						   "SetFadcSleepTriggerTime",
						   "SetCenterChip",
						   "PrintCenterChip"};
    return HFMCommandsSet;
}

const char** create_tos_commands(){
    // this function builds the list of TOS commands as the readline
    // function demands (const char** array)

    // first get possible commands
    static const std::set<std::string> tosCommandsSet = get_tos_commands();
    static const std::set<std::string> HFMCommandsSet = get_hfm_commands();
    
    const short tos_cmd_size = tosCommandsSet.size();
    const short hfm_cmd_size = HFMCommandsSet.size();
    static const short elements = tos_cmd_size + hfm_cmd_size;

    // allocate memory for elements + 1 char*. + 1 to store NULL
    // pointer as last element
    const char** cmds = new const char* [elements + 1];
    int index = 0;
    // fill the array
    for (auto &cmd : tosCommandsSet){
	cmds[index] = cmd.c_str();
	index++;
    }
    for (auto &cmd : HFMCommandsSet){
	cmds[index] = cmd.c_str();
	index++;
    }

    // now NULL pointer to properly handle last call to
    // TOS_Command_Generator (while break condition)
    cmds[index] = NULL;
    
    return cmds;
}


char *TOS_Command_Generator(const char *text, int state){
    // inputs:
    //     const char *text: the string to be checked
    //     int state: a variable to allow for initialization of this
    //                function.
    //                - state == 0 on first call
    //                - state  > 0 on every call afterwards
    //       last state value is custom:
    //                - state == -1 in case last call to this happened
    //                  and we wish to delete the tosCommands array

    
    // define two static variables. One to store the set
    // of commands and a bool to check whether the command
    // set was initialized yet. 
    static const char **tosCommands;
    static bool tosCommandsCreated = false;
    if (state >= 0){
        if (!tosCommandsCreated){
	    tosCommands = create_tos_commands();
	    tosCommandsCreated = true;
        }

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
    else if(state == -1){
	//std::cout << "deleting TOS commands array" << std::endl;
	//delete[] tosCommands;
	return ((char *) NULL);
    }

    // this should never be reached in principle
    return ((char *) NULL);
}

// custom completion function
//static
// NOTE S.Schmidt: last variable has no name, since we do not need it here.
//     However, needed for call to readline function, which expectes 3 args
//     Would otherwise cause unused variable warning.
char **TOS_Command_Completion(const char *text, int start, int){
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

int check_if_string_allowed(std::string tempStr, 
			    std::set<std::string> *allowedStrings){
    // this function is used to check whether the input string is element of
    // the set of allowed strings
    // returns 0 if string allowed
    // returns 1 if string not allowed
    if (allowedStrings == NULL){
	// in this case we accept the input, because our only
	// constraint is a non empty string
	return 0;
    }
    else if ( (allowedStrings != NULL) &&
	      (allowedStrings->find(tempStr) != allowedStrings->end()) ){
	// allowedStrings is not NULL and tempStr is element of allowedStrings
	// thus we can break from the while loop
	return 0;
    }
    // if we have not left the function yet allowedStrings is not NULL, 
    // but string also not in set. Thus output message
    std::cout << "Argument not in set of allowed strings; try again"
	      << std::endl;
    return 1;
}

std::string getUserInput(const char *prompt, 
			 bool numericalInput, 
			 bool allowDefaultOnEmptyInput,
			 std::set<std::string> *allowedStrings){
    // char *prompt: given char array is the prompt
    //               presented to the user on input
    // e.g.: prompt == "TOS> " then this will be prepended to the input.
    // for most calls to this function use "" empty string or NULL
    // bool numericalInput: if this flag is set, only allow numerical input
    // bool allowDefaultOnEmptyInput: if this flag is set, allow empty input
    //                                and thus set a default value later
    // this function provides a general 'quit' command, to leave the function
    // call, if desired. IMPORTANT: this function can only return a string, 
    // that means the function which calls getUserInput, needs to deal with
    // returning to the userInterface
    // TODO: is there maybe a smarter way to do this?

    char *buf;
    // add the user input to the history
    // add_history( buf );
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
	    int stringAllowed;

	    // first of all add this command to the history
	    add_history(buf);


	    // the first thing to check, is whether the input is a quit command
	    if (tempStr == "quit") break;

	    // if numericalInput is set to true, check whether input is numerical
	    if (numericalInput == true){
		// try to convert tempStr to int, if non numerical, will throw
		// invalid_argument exception
		try{
		    // do not need to use return value, only interested in 
		    // exception
		    std::stoi(tempStr);
		    // if no exception is thrown, check if we supply allowed values for 
		    // the input. If so, check if input is in set of allowed inputs
		    stringAllowed = check_if_string_allowed(tempStr, allowedStrings);
		    if (stringAllowed == 0) break;
		}
		catch (const std::invalid_argument &ia) {
		    // if this is catched, argument non numerical
		    std::cout << "Argument non numerical; try again" 
			      << std::endl;
		}
		catch (const std::out_of_range &ia) {
		    // if this is catched, numerical input exceeds size 
		    // of an integer
		    std::cout << "Numerical input exceeds size of an integer; try again"
			      << std::endl;
		}
	    }// end numericalInput
	    else{
		// in this case we want a string input. check if allowedStrings is
		// (not) NULL, and if element is in set of allowedStrings
		stringAllowed = check_if_string_allowed(tempStr, allowedStrings);
		if (stringAllowed == 0) break;
	    }
	}//end non empty buffer
    }//end while loop

    // now buf should be non empty
    // initialize a string with the char* and return it
    std::string input(buf);
    if (buf != NULL) free(buf);
    return input;
}

// int getUserInput(const char *prompt,
// 		 bool numericalInput, 
// 		 bool allowDefaultOnEmptyInput,
// 		 std::set<std::string> *allowedStrings){
//     // a wrapper around getUserInput to return an int, instead of a string
//     int input;
//     std::string input_str;
//     input_str = getUserInput(prompt, numericalInput, allowDefaultOnEmptyInput, allowedStrings);

//     input = std::stoi(input_str);
//     return input;    
// }

// template<class T> T getUserInputNumericalNoDefault(const char *prompt, std::set<std::string> *allowedStrings){
//     // template function to return any kind of value (typically a type of int or string)
//     return getUserInput(prompt, true, true, allowedStrings);
// }
    

// convenience functions to simply call getUserInput with certain flags
std::string getUserInputNumericalDefault(const char *prompt, std::set<std::string> *allowedStrings){
    return getUserInput(prompt, true, true, allowedStrings);
}

std::string getUserInputNumericalNoDefault(const char *prompt, std::set<std::string> *allowedStrings){    
    return getUserInput(prompt, true, false, allowedStrings);
}

std::string getUserInputNonNumericalDefault(const char *prompt, std::set<std::string> *allowedStrings){   
    return getUserInput(prompt, false, true, allowedStrings);
}

std::string getUserInputNonNumericalNoDefault(const char *prompt, std::set<std::string> *allowedStrings){ 
    return getUserInput(prompt, false, false, allowedStrings);
}

// the following convenience functions allow to hand a set of strings (not a pointer), 
// and have this wrapper function hand the correct pointer to the main function
// these functions are mainly used if one wants to avoid initializing a set 
// of strings in the code manually and hand the pointer. Using these functions, 
// something like the following is possible:
// input = getUserInputNonNumericalNoDefault("> ", {"allowed1", "allowed2", "and so on"});
std::string getUserInputNumericalDefault(const char *prompt, std::set<std::string> allowedStrings){
    return getUserInputNumericalDefault(prompt, &allowedStrings);
}

std::string getUserInputNumericalNoDefault(const char *prompt, std::set<std::string> allowedStrings){    
    return getUserInputNumericalNoDefault(prompt, &allowedStrings);
}

std::string getUserInputNonNumericalDefault(const char *prompt, std::set<std::string> allowedStrings){   
    return getUserInputNonNumericalDefault(prompt, &allowedStrings);
}

std::string getUserInputNonNumericalNoDefault(const char *prompt, std::set<std::string> allowedStrings){ 
    return getUserInputNonNumericalNoDefault(prompt, &allowedStrings);
}

bool getUserInputOrDefaultFile(const char *prompt, const std::string& default_path, std::string& filename) {
    std::string input = getUserInput(prompt, false, true).c_str();

    if (input == "quit") {
        // User aborted the command
        return false;
    } else if (input == "")
	    // Choose the default file
        filename = default_path;
    else 
	    // Choose the user-defined file
        filename = input;

    if (!fs::exists(fs::path(filename))) {
	    std::cout << "File not found: " << filename << std::endl;
        return false;
    }

    return true;
}


// get Input value function used for FADC input.
// TODO: To be taken out at some point...
int getInputValue(){
  // Prompt for input var
  std::cout << "Enter positive integer value: ";

  //Get input var
  int value = 0 ;
  std::cin.exceptions( std::istringstream::failbit );
  
  try{ //Check whether input is of valid type
    std::cin >> value ;
  }catch( std::istream::failure &e){
    value = -1; 
    std::cout << "Invalid input value, value is set to -1" << std::endl;    
    std::cin.clear();
  }
  return value;
}//end of getInputValue
