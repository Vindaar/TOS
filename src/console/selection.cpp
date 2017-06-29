/* this file contains all functions, which selections based on user input */

#include "console.hpp"

std::set<unsigned short> Console::ChipSelection(){
    // this function provides the user input interface to select a 
    // set of chips based on all current active chips
    std::set<unsigned short> chip_set;
    std::string input;
    std::set<std::string> allowedStrings;

    std::cout << "Select which chips to use.\n"
	      << "the following chips are currently connected:" << std::endl;
    for (auto chip : _chip_set){
	std::cout << "#" 
		  << chip 
		  << " : " 
		  << pc->fpga->tp->GetChipName(chip) 
		  << std::endl;
    }
    std::cout << "type # of chip from above to select.\n"
	      << "select one after another\n"
	      << "type : { all } to select all chips" << std::endl;
    
    // now create the allowed strings for the input based on the number of chips
    // the loop goes from 0 to < num chips
    for (auto chip : _chip_set){
	allowedStrings.insert(std::to_string(chip));
    }
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
	if(input == "quit") return std::set<unsigned short>();
	else if( input == "all" ){
	    // in case the user wants to use all chips, we set chip_set to
	    // console member _chip_set, set input to "done" and 
	    // set chip_set to the temp set (temp set used to prevent problems, if 
	    // user first selects some chips individually and then all)
	    input = "done";
	    chip_set = _chip_set;
	}
	else if (input != "done"){
	    chip_set.insert(std::stoi(input));
	}
    } while (input != "done");

    return chip_set;
}

std::string Console::CTPRSelection(){
    // this function is called to provide the user input interface
    // to select, which CTPR value is to be used in the following
    // function (which columns will receive test pulses).
    // function returns:
    //     <number as string> corresponding to 0 - 31 as an allowed CTPR value
    //     "quit" to quit the calling function

    std::string input;
    std::set<std::string> allowedCTPRStrings;
    for(int l = 0; l < 32; l++) allowedCTPRStrings.insert(std::to_string(l));

    std::cout << "Please select a CTPR value (columns to receive test pulses)\n"
	      << "choose a value between 0 and 31. Default is 0."
	      << std::endl;
    input = getUserInputNonNumericalDefault(_prompt, &allowedCTPRStrings);
    
    return input;
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

std::list<int> Console::GetTOCalibDefaultPulseList(){
    // returns the default values for the test pulses
    // for the TOCalib function

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

    return defaultList;
}

std::list<int> Console::GetSCurveDefaultPulseList(){
    // returns the default values for the test pulses
    // for the SCurve function
    
    std::list<int> defaultList;
    defaultList.push_back(0);
    defaultList.push_back(20);
    defaultList.push_back(25);
    defaultList.push_back(30);
    defaultList.push_back(35);
    defaultList.push_back(40);
    defaultList.push_back(50);
    defaultList.push_back(60);
    defaultList.push_back(100);
    return defaultList;
}

std::list<int> Console::PulseListCreator(std::string pulser, std::string callerFunction){
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
	if( input == "quit" ) return std::list<int>(-1);
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

	if (callerFunction == "TOCalib"){
	    defaultList = GetTOCalibDefaultPulseList();
	}
	else if (callerFunction == "SCurve"){
	    defaultList = GetSCurveDefaultPulseList();
	}

	
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


// TODO: think about refactoring Coarse and THL functions into
// a general function to select an upper and lower bound for both
// either done by:
//     - have generic function, which also receives strings as arguments
//       which describe whether coarse or thl
//       however, requires that one also hands the specific ranges for
//       thl or coarse (unless only these two hardcoded)
//     - or simply define a generic function, which is used to build
//       functions for each, where we hand the generic function the
//       ranges and names and the final function is returned
//     C++: how to handle function pointers etc.?
int Console::CoarseAny(std::string boundary){
    // handles the user interface of a seleection of an upper / lower
    // 'coarse' value
    std::string input;

    std::cout << "Choose a " << boundary << " coarse value\n"
	      << "range: 0 - 15"
	      << std::endl;
    std::set<std::string> allowedValues;
    for(int i = 0; i < 15; i++) allowedValues.insert(std::to_string(i));

    input = getUserInputNumericalNoDefault(_prompt, allowedValues);
    if(input == "quit") return -1;

    return std::stoi(input);
}

std::pair<int, int> Console::CoarseBoundarySelection(){
    // this function handles user input in regards to the selection of
    // upper and lower coarse values
    // call CoarseAny for both boundaries 
    // and combine both to a pair, which is returned
    int lower_bound;
    int upper_bound;

    lower_bound = CoarseAny("lower");
    if (lower_bound == -1) return std::make_pair(0, 0);
    upper_bound = CoarseAny("upper");
    if (upper_bound == -1) return std::make_pair(0, 0);

    //std::pair<std::string, std::string> threshold_boundary_pair;
    auto coarse_boundary_pair = std::make_pair(lower_bound, upper_bound);

    return coarse_boundary_pair;
}

int Console::THLAnyBoundary(std::string boundary){
    // handles the user interface of a selection of an upper / lower THL boundary
    std::string input;

    std::cout << "Choose a " << boundary << " THL bound\n"
	      << "range: 0 - 1023"
	      << std::endl;

    std::set<std::string> allowedValues;
    for(int i = 0; i < 1024; i++) allowedValues.insert(std::to_string(i));

    input = getUserInputNumericalNoDefault(_prompt, allowedValues);
    if(input == "quit") return -1;

    return std::stoi(input);
}

std::pair<int, int> Console::THLBoundarySelection(){
    // this function handles user input in regards to the selection of
    // upper and lower THL thresholds
    // call THLUpperBoundarySelection()
    // call THLLowerBoundarySelection()
    // and combine both to a pair, which is returned

    int lower_bound;
    int upper_bound;

    lower_bound = THLAnyBoundary("lower");
    if (lower_bound == -1) return std::make_pair(0, 0);
    upper_bound = THLAnyBoundary("upper");
    if (upper_bound == -1) return std::make_pair(0, 0);

    //std::pair<std::string, std::string> threshold_boundary_pair;
    auto threshold_boundary_pair = std::make_pair(lower_bound, upper_bound);

    return threshold_boundary_pair;
}


std::map<std::string, int> Console::ChessMatrixSelection(int chip){
    // this function handles user input in regards to a creation of a chess matrix
    // asks user for number of columns, rows for the fields and each bit
    // to be set for the pixel
    // inputs:
    //     int chip: the chip for which to set this chess matrix
    //               chip == 0 means for all

    // create an empty map, which is used to return in case user wants to quit
    //   (checked in calling function, whether returned map is empty)
    std::map<std::string, int> empty_return_map;
    // and the map to be returned
    std::map<std::string, int> return_map;
    // return value for matrix parameters
    int err = 0;


    std::string input;
    std::cout << "Chip Number " << chip << std::endl;
    std::cout << "\t Number cols per field=" << std::flush;
    std::set<std::string> allowedValues; 
    for( int l=1; l < 256; l++) allowedValues.insert(std::to_string(l));
    
    input = getUserInputNumericalNoDefault(_prompt, &allowedValues);
    if (input == "quit") return empty_return_map;
    return_map["length"] = std::stoi(input);
    std::cout << "\t Number row per field=" << std::flush;
    input = getUserInputNumericalNoDefault(_prompt, &allowedValues);
    if (input == "quit") return empty_return_map;
    return_map["width"]  = std::stoi(input);

    std::vector<int> black_parameters;
    std::cout << "\nFirst enter the black matrix parameters: " << std::endl;    
    err = MatrixParameters(black_parameters);
    if (err == -1) return empty_return_map;
    // NOTE: not exactly nice to add each element individually.. How else to
    //       do it, since we want strings as keys?
    return_map["black_p0"]   = black_parameters[0];
    return_map["black_p1"]   = black_parameters[1];
    return_map["black_mask"] = black_parameters[2];
    return_map["black_test"] = black_parameters[3];
    return_map["black_thr"]  = black_parameters[4];
    std::vector<int> white_parameters;
    std::cout << "\nNow enter the white matrix parameters: " << std::endl;    
    err = MatrixParameters(white_parameters);
    if (err == -1) return empty_return_map;
    return_map["white_p0"]   = white_parameters[0];
    return_map["white_p1"]   = white_parameters[1];
    return_map["white_mask"] = white_parameters[2];
    return_map["white_test"] = white_parameters[3];
    return_map["white_thr"]  = white_parameters[4];
    
    return return_map;
}

//Function for getting user inputs with matrix parameters
template <typename Ausgabe> int Console::MatrixParameters(Ausgabe &aus){
    // variables for getUserInput
    std::string input;
    const char *prompt;
    std::set<std::string> allowedParameters; //Array of allowed strings for P0, P1, mask and test
    for( int l=0; l<=1; l++) allowedParameters.insert(std::to_string(l));
    std::set<std::string> allowedThresholds; //Array of allowed strings for threshold
    for( int l=0; l<=15; l++) allowedThresholds.insert(std::to_string(l));
    
    //ask the user for the matrix parameters
    prompt = "\t P0=> ";
    input = getUserInputNumericalNoDefault(prompt, &allowedParameters);
    if (input == "quit") return -1;
    aus.push_back(std::stoi(input));
    prompt = "\t P1=> ";
    input = getUserInputNumericalNoDefault(prompt, &allowedParameters);
    if (input == "quit") return -1;
    aus.push_back(std::stoi(input));
    prompt = "\t Mask=> ";
    input = getUserInputNumericalNoDefault(prompt, &allowedParameters);
    if (input == "quit") return -1;
    aus.push_back(std::stoi(input));
    prompt = "\t Test=> ";
    input = getUserInputNumericalNoDefault(prompt, &allowedParameters);
    if (input == "quit") return -1;
    aus.push_back(std::stoi(input));
    prompt = "\t Threshold=> ";
    input = getUserInputNumericalNoDefault(prompt, &allowedThresholds);
    if (input == "quit") return -1;
    aus.push_back(std::stoi(input));
    return 0;
}
