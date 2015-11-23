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
                             "Trigger ", 
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
                             "DisableFastClock"};


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
