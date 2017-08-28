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

#include <atomic>
#include <set>
#include <string>
#include <list>
#include <map>
#include <stdio.h>

#ifdef __WIN32__
#include <Windows.h> // comes first
#endif

#include <sys/select.h>


// MCP2210
#include "mcp2210/temp_auslese.hpp"

// include readline history library to achieve command completion etc
#include <readline/history.h>
#include "tosCommandCompletion.hpp"


// TOS headers
#include "V1729a.h"
#include "pc.hpp"
#include "hvFadcManager.hpp"
#include "timepix.hpp"
#include "fpga.hpp"
#include "pc.hpp"
#include "tosCommandCompletion.hpp"

#if TESTS==1
//#ifndef _TESTS_INCLUDED
//#define TESTS_INCLUDED 1
#include "../tests/HighLevelFunction_VME_tests/HighLevelFunction_VME_tests.hpp"
//#endif
#endif

//class V1729a;
//class PC;
//class hvFadcManager;
//class Timepix;

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


    // ################################################################################
    // ######################### Private member functions #############################
    // ################################################################################    
    
    unsigned short getPreload();                 //< used to get a number of preload bits

    // ################################################################################
    // ############################### User interface functions #######################
    // ################################################################################
    
    //The main userinterface  
    int UserInterface();                         //< classical version
    void ParseNormalTosCommands(std::string input, int &running);
    void ParseActiveHfmCommands(std::string input);

    int CommandHelp();
    int CommandSpacing();
    void CommandSetPreload();    
    int CommandSetNumChips(bool callSetNumChips = true);
    // SetNumChips is the actual function, which changes the number of chips internally
    // CommandSetNumChips() calls this function after getting user input
    void SetNumChips(unsigned short nChips);

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
    int CommandGetMatrixAsIntAndDump();
    int CommandWriteReadFSR();
  
    //write files containing the values of the DAC control panel
    int CommandSaveFSR();

    // convenience function to set the chip ID offset, which defines the offset
    // for the chip id (for the bitstream of the chip communication; different for different
    // number of chips)
    void CommandSetChipIdOffset();


    // functions to set and print the center chip variable, which decides for which chip
    // to read out the FADC
    void CommandSetCenterChip();
    void CommandPrintCenterChip();

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
    // function to simply load a single fsr file for all chips
    int CommandLoadFSRAll();
    int CommandLoadThreshold();
    int CommandSetDAC();
    int CommandShowFSR();
    // create default chess matrix of 5x5 pixels
    void CommandVarChessMatrixDefault();
    int CommandVarChessMatrix(bool all_chips = false);
    int CommandUniformMatrix();
    // quick implementation to set same uniform matrix for all chips
    int CommandUniformMatrixAllChips();
    int CommandFADCshutter();
    int CommandDACScan();
    int CommandTHLScan();
    // the actual THL function, which is run in a separate thread
    void RunTHLScan(std::set<unsigned short> chip_set,
		    std::pair<int, int> coarse_boundaries,
		    std::pair<int, int> threshold_boundaries,
		    std::string shutter_range,
		    std::string shutter_time,
		    std::atomic_bool *loop_stop);
    
    void CommandSCurve();
    int CommandSCurveOld();
    int Commandi2creset();
    int Commandi2cDAC();
    int Commandi2cADC();
    int CommandTpulse();
    int CommandTestTPulse();
    // the actual function called by CommandTestTPulse (separate thread)
    void runTestPulses();

    int CommandDoTHSopt();
    int CommandThresholdEqNoiseCenter();
    void CommandTOCalib();
    int CommandTOCalibFast();
    void CommandCheckOffsetZeroSuppressed();
    void CommandCheckOffsetFullMatrix();    
    int CommandCalibrate();				
    int CommandSwitchTriggerConnection();
    void DACScanLive(char dac, int val);
    void CommandSpeedTest(std::string ein);
    void CommandSetIP();

    // functions related to HV_FADC object
    void CommandActivateHvFadcManager();
    void CommandAddChannel();
    void CommandRemoveChannel();
    void CommandAddFlexGroup();
    void CommandRemoveFlexGroup();
    void CommandSetChannelValue();

    // function which makes sure user disconnects devices from FADC before starting
    // pedestal calibration run
    void CommandFadcPedestalRun();

    // function to set FADC trigger level in mV
    void CommandFadcTriggerLevel();

    // MCP2210 related functions
    // function to loop over temperature readout (for now just calls temp_loop_main())
    void CommandTempLoopReadout();


    //################################################################################
    //################################ Selection functions ###########################
    //################################################################################
    
    // user input interface to select chips
    std::set<unsigned short> ChipSelection();
    // user input interface function for fastclock, shutter range and time selection
    std::string FastClockSelection();
    std::string ShutterRangeSelection();
    std::string ShutterTimeSelection(int n);
    std::string ShutterTimeSelection(std::string shutter_range);
    int         ShutterRangeToMode(std::string shutter_range);
    std::string TriggerSelection();
    std::string CalibrationSelection();
    std::string PulserSelection();
    std::string CTPRSelection();
    std::list<int> PulseListCreator(std::string pulser, std::string callerFunction);
    std::list<int> GetTOCalibDefaultPulseList();
    std::list<int> GetSCurveDefaultPulseList();
    int PixPerColumnSelection();
    int CoarseAny(std::string boundary);
    std::pair<int, int> CoarseBoundarySelection();    
    int THLAnyBoundary(std::string boundary);
    std::pair<int, int> THLBoundarySelection();
    std::map<std::string, int> ChessMatrixSelection(int chip);
    int FadcTriggerLevelSelection();


    // ################################################################################
    // ############################## other functions #################################
    // ################################################################################
    
    // Some function one can use to print error messages according to the error parameter given
    void ErrorMessages(int err);

    // template function to get matrix parameters
    template <typename Ausgabe> int MatrixParameters(Ausgabe &aus);

    // function to update the chip set references of the PC, FPGA and timepix object
    void UpdateChipSetReference();


#if TESTS==1
    // define the RunTest() function to run custom tests
    void RunTests(bool hardware_connected = false);
    
#endif
    

    //################################################################################
    //############################## Member variables ################################
    //################################################################################    

    //declaration of a pc var - the communication with the fpga is done by this class
    // use a pointer for pc. Create object in console constructor, delete in destructor
    PC *pc;
    // console owns a timepix pointer, which is handed to the PC constructor
    Timepix *_tp;
    
    int ok;              //< some check var to work with pc 
    unsigned short _nbOfChips;      //< the number of chips one wants to use
    int _preload;        //< nb of preload bits - used to fix some bugs

    //vars to cope with the use of the fadc
    //bool _fadcActive;                        //< check var. true if a fadc is used, false otherwise
    
    hvFadcManager* _hvFadcManager;
    bool _hvFadcManagerActive;

    // member variable for general command prompt for input methods
    const char *_prompt;
    std::atomic_bool _loop_stop;
    std::atomic_bool _temp_check_loop_stop;

    // a set of active chips, which we are currently working with
    std::set<unsigned short> _chip_set;

    //HighLevelFunction_VME* _fadcFunctions;   //< some additional functions for the use of the fadc

};

#endif
