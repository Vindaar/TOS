/**********************************************************************/
/*                                                        console.hpp */
/*  TOS - Timepix Operating Software                                  */
/*                                                                    */
/*                                                         20.07.2009 */
/*                                                    Christian Kahra */
/*                                     chrkahra@students.uni-mainz.de */
/*                                        Institut fuer Physik - ETAP */
/*                              Johannes-Gutenberg Universitaet Mainz */
/**********************************************************************/

#ifndef _CONSOLE_HPP
#define _CONSOLE_HPP 1

//project
#include "header.hpp"
#include "pc.hpp"

//c++
#include <string>
#include <sstream>
// set and cmath used for CountingTime()
#include <set>
#include <cmath>

//TOS and FADC
#include "caseHeader.h"

//FADC stuff
#include "High-Level-functions_VME.h"

// include readline history library to achieve command completion etc
#include <readline/history.h>
#include "tosCommandCompletion.hpp"

#define DEFAULT_USER_INPUT_PROMPT                   "> " 



class Console{

public:
    //C'tor
    Console();
    // TODO: replace by HV_FADC_Obj pointer
    Console(V1729a* dev);

    Console(std::string iniFilePath);

    //D'tor
    ~Console();

    void ConsoleMain();  //< calls a UserInterface function 
    int okay();          //< returns value of the ok-var
   
    static void WrapperToDACScanLive(void* PointerToObject, char dac, int val);

private:

    //declaration of a pc var - the communication with the fpga is done by this class
    PC pc;
    
    int ok;              //< some check var to work with pc 
    int _nbOfChips;      //< the number of chips one wants to use
    int _preload;        //< nb of preload bits - used to fix some bugs
  
    unsigned short getPreload();                 //< used to get a number of preload bits
  
    //The main userinterface  
    int UserInterface();                         //< classical version
    // user input interface function for fastclock, shutter range and time selection
    std::string FastClockSelection();
    std::string ShutterRangeSelection();
    std::string ShutterTimeSelection(int n);
    std::string TriggerSelection();

    // Some function one can use to print error messages according to the error parameter given
    void ErrorMessages(int err);

    int CommandHelp();
    int CommandSpacing();
    int CommandSetNumChips();
    // SetNumChips is the actual function, which changes the number of chips internally
    // CommandSetNumChips() calls this function after getting user input
    void SetNumChips(int nChips);

    int CommandSetOption();
    int CommandRun(bool useHvFadc = false);
    int CommandCounting(int c);

    int CommandCountingTrigger();
    int CommandCountingTime();
    // same functions for FADC usage
    // slightly different implementation. Uses getInputValue() functions
    // defined in caseFunctions

    int CommandReadOut();
    int CommandReadOut2();
    int CommandSetMatrix();
    int CommandSaveMatrix();
    int CommandLoadMatrix();
    int CommandWriteReadFSR();
  
    //write files containing the values of the DAC control panel
    int CommandSaveFSR();

    /* ReadoutFpgaExtTriggerBit: Reads out the 16th(?) fpga bit - 1 if a trigger arrived at the 
     * extern-trigger-input as the shutter was open, 0 otherwise
     */
    int ReadoutFpgaExtTriggerBit();
    /* ReadoutFpgaExtShutterFlag: Reads out the fpga extern-trigger-flag variable of the pfga. 1 if the 
     * flag is set, zero otherwise
     */
    int ReadoutFpgaExtTriggerFlag();
    // ClearFpgaExtTriggerFlag: Sets the FADC flab back to false
    void ClearFpgaExtTriggerFlag();
  
    int CommandLoadFSR();	
    int CommandLoadThreshold();
    int CommandSetDAC();
    int CommandShowFSR();
    int CommandVarChessMatrix();
    int CommandUniformMatrix();
    int CommandFADCshutter();
    int CommandDACScan();
    int CommandTHLScan();
    int CommandSCurve();
    int CommandSCurveFast();
    int Commandi2creset();
    int Commandi2cDAC();
    int Commandi2cADC();
    int CommandTpulse();

    int CommandDoTHSopt();
    int CommandThresholdEqNoiseCenter();
    int CommandTOCalib();
    int CommandTOCalibFast();
    int CommandCheckOffset();
    int CommandCalibrate();				
    int CommandSwitchTriggerConnection();
    void DACScanLive(char dac, int val);

    void CommandSpeedTest(std::string ein);

		
    void CommandSetIP();


    // functions related to HV_FADC object
    void CommandActivateHvFadcObj();


    //
    //vars to cope with the use of the fadc
    // TODO: Change to usage of HV_FADC_Obj
    
  
    //V1729a* _fadc;                           //< fadc pointer
    //bool _fadcActive;                        //< check var. true if a fadc is used, false otherwise
    
    HV_FADC_Obj* _hvFadcObj;
    bool _hvFadcObjActive;

    // member variable for general command prompt for input methods
    const char *_prompt;

    //HighLevelFunction_VME* _fadcFunctions;   //< some additional functions for the use of the fadc

};

#endif
