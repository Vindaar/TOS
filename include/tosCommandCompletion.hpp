// This header defines the functions used to achieve command completion in
// TOS. They are based on the GNU readline library:
// https://cnswww.cns.cwru.edu/php/chet/readline/rltop.html

#ifndef _TOS_COMMAND_COMPLETION_HPP
#define _TOS_COMMAND_COMPLETION_HPP 1


#include <stdio.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>

#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <set>
#include <string>


// the following array contains all possible TOS commands
// TODO: change const char array to something nicer
// extern const char *tosCommands[];
const std::set<std::string> get_tos_commands();
const std::set<std::string> get_hfm_commands();

// Generator function for word completion
char *TOS_Command_Generator(const char *text, int state);

// custom completion function
//static 
char **TOS_Command_Completion(const char *text, int start, int);

// function which checks if input string is element of set of allowed strings
int check_if_string_allowed(std::string tempStr, 
			    std::set<std::string> *allowedStrings);

// get user input function
std::string getUserInput(const char *prompt,
			 bool numericalInput = true, 
			 bool allowDefaultOnEmptyInput = true,
			 std::set<std::string> *allowedStrings = NULL);

// convenience function which just call getUserInput with certain flags set
std::string getUserInputNumericalDefault(     const char *prompt,
					      std::set<std::string> *allowedStrings = NULL);
std::string getUserInputNumericalNoDefault(   const char *prompt,
					      std::set<std::string> *allowedStrings = NULL);
std::string getUserInputNonNumericalDefault(  const char *prompt,
					      std::set<std::string> *allowedStrings = NULL);
std::string getUserInputNonNumericalNoDefault(const char *prompt,
					      std::set<std::string> *allowedStrings = NULL);

std::string getUserInputNumericalDefault(     const char *prompt, 
					      std::set<std::string> allowedStrings);

std::string getUserInputNumericalNoDefault(   const char *prompt, 
					      std::set<std::string> allowedStrings); 

std::string getUserInputNonNumericalDefault(  const char *prompt, 
					      std::set<std::string> allowedStrings);

std::string getUserInputNonNumericalNoDefault(const char *prompt, 
					      std::set<std::string> allowedStrings);

bool getUserInputOrDefaultFile(const char *prompt, const std::string& default_path, std::string& filename);

// function to get input for FADC functions
int getInputValue();


#endif
