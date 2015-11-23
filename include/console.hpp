
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

//TOS and FADC
#include "caseHeader.h"

//FADC stuff
// TODO: replace this by HV_FADC_Obj
#include "V1729a.h"
#include "V1729a_Dummy.h"
#include "V1729a_VME.h"
#include "High-Level-functions_VME.h"

// include readline history library to achieve command completion etc
#include <readline/history.h>
#include "tosCommandCompletion.hpp"

class Console{

public:
    //C'tor
    Console();
    // TODO: replace by HV_FADC_Obj pointer
    Console(V1729a* dev);

    Console(QString iniFilePath);

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
    int UserInterfaceFadc();                     //< new version, applicable to the use of the FADC
    int UserInterfaceNew(bool useFadc = false);  //< new version - like above - but without the FADC

    // Some function one can use to print error messages according to the error parameter given
    void ErrorMessages(int err);

    int CommandHelp();
    int CommandSpacing(std::string ein);
    int CommandSetNumChips();
    int CommandSetOption();
    int CommandRun(bool useFadc = false);
    int CommandCounting(int c);
    int CommandCountingLong();

    int CommandCountingTrigger(std::string ein);
    int CommandCountingTime(std::string ein);
    int CommandCountingTime_fast(std::string ein);
    int CommandCountingTime_long(std::string ein);
    int CommandCountingTime_verylong(std::string ein);
    // same functions for FADC usage
    // slightly different implementation. Uses getInputValue() functions
    // defined in caseFunctions
    int CommandSpacing(unsigned int space);
    int CommandCountingTrigger(unsigned int time);
    int CommandCountingTime(unsigned int time);
    int CommandCountingTime_fast(unsigned int time);
    int CommandCountingTime_long(unsigned int time);
    int CommandCountingTime_verylong(unsigned int time);

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
    int CommandSetDAC(std::string ein);
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
    int CommandSwitchTriggerConnection(std::string ein);
    // same function for FADC usage (see above for explanation)
    int CommandSwitchTriggerConnection(int trigCon);
    void DACScanLive(char dac, int val);

    void CommandSpeedTest(std::string ein);

		
    void CommandSetIP(std::string ein);


    //
    //vars to cope with the use of the fadc
    // TODO: Change to usage of HV_FADC_Obj
    
  
    //V1729a* _fadc;                           //< fadc pointer
    //bool _fadcActive;                        //< check var. true if a fadc is used, false otherwise
    
    HV_FADC_Obj* _hvFadcObj;
    bool _hvFadcObjActive;		       

    //HighLevelFunction_VME* _fadcFunctions;   //< some additional functions for the use of the fadc


    // test
    std::list<std::string> commandList;
};

#endif
