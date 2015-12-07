// This header defines the functions used to achieve command completion in
// TOS. They are based on the GNU readline library:
// https://cnswww.cns.cwru.edu/php/chet/readline/rltop.html

#ifndef _TOS_COMMAND_COMPLETION_HPP
#define _TOS_COMMAND_COMPLETION_HPP 1


// include the necessary readline libraries
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <iostream>
#include <stdexcept>




// the following array contains all possible TOS commands
// TODO: change const char array to something nicer
extern const char *tosCommands[];
// Generator function for word completion
char *TOS_Command_Generator (const char *text, int state);

// custom completion function
//static 
char **TOS_Command_Completion( const char *text, int start, int end);

// get user input function
std::string getUserInput(const char *prompt, 
			 bool numericalInput = true, 
			 bool allowDefaultOnEmptyInput = true);

std::string getUserInputNumericalDefault(     const char *prompt);
std::string getUserInputNumericalNoDefault(   const char *prompt);
std::string getUserInputNonNumericalDefault(  const char *prompt);
std::string getUserInputNonNumericalNoDefault(const char *prompt);





#endif
