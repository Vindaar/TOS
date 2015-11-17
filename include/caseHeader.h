//std c++
#include <iostream>
#include <sstream>
#include <string>


//function to print all some important commands to screen
void printGeneralCommands(bool fadcActive = false);

//function to print all tos commands to screen
void printTosCommands(bool fadcActive = false);

//function to print all fadc commands to screen
void printFadcCommands();

//Universal input funciton
int getInputValue();

//Universal input function to handel long ints
unsigned int getLongInputValue();

//Universal input function to handel short ints
unsigned short getShortInputValue();

//Universal input function
//template<typename TYPE> TYPE getAllInputValue(int err);
template <class T> T getAllInputValue(T err);

//helper function to work with the old TOS version
int turnString2Int(std::string inputString);

