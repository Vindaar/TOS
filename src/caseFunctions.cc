/*
 * Functions combining FADC and TOS readout.
 */

#include "caseHeader.h"


void printGeneralCommands(bool fadcActive)
{
  std::cout << std::endl
            << "-------------------------------------------" << std::endl
            << "              General Commands             " << std::endl 
            << std::endl
            << " Print General Commands                ( 0)" << std::endl;
  if(fadcActive){
    std::cout 
            << " Print Commands (FADC)               (7001)" << std::endl
            << " Print Settings (FADC)               (7002)" << std::endl
            << " Go to FADC UserInterface              (49)" << std::endl
            << " Quit FADC                             (99)" << std::endl;
  }
  std::cout << " Print Commands (TOS)                  (50)" << std::endl
    //            << " Print TOS Settings to screen         (510)" << std::endl
            << " Go to classical TOS mode              (98)" << std::endl
            << " Quit                                  (57)" << std::endl 
            << "-------------------------------------------" << std::endl 
            << std::endl;
}//end of printCommands


void printTosCommands(bool fadcActive)
{
  std::cout << std::endl
            << "-------------------------------------------" << std::endl
            << "          Important general stuff:         " << std::endl 
            << std::endl
            << " Print General Commands                ( 0)" << std::endl;
  if(fadcActive){
    std::cout 
            << " Print FADC Commands                 (7001)" << std::endl;
  }
  std::cout << " Print Commands (TOS)                  (50)" << std::endl
    //            << " Print TOS Settings to screen         (510)" << std::endl
    //            << " Print FSR Settings to screen         (511)" << std::endl
            << " Go to classical TOS mode              (98)" << std::endl
            << " Quit TOS                              (57)" << std::endl
            << std::endl
            << std::endl;
  if(fadcActive){
    std::cout << " FADC - TOS Communication                  " << std::endl
              << "     Quit FADC, switch to TOS only     (99)" << std::endl
              << "     Readout FADC Bit                (1005)" << std::endl 
              << "     Read/Clear FADC Flag       (1006/1007)" << std::endl;
      }
 

  std::cout  << "-------------------------------------------" << std::endl
            << "                TOS Commands:              " << std::endl 
            << "-------------------------------------------" << std::endl
            << " General Reset                         (52)" << std::endl
            << " SetNumChips                           (53)" << std::endl
            << " Trigger                               (54)" << std::endl
            << " Abort measurement                     (56)" << std::endl
            << " Help                                  (58)" << std::endl
            << std::endl
            << "                Data taking:               " << std::endl 
            << " Run                                   (55)" << std::endl
            << " ReadOut             3               (1030)" << std::endl
            << "    -ReadOut2        3a              (1031)" << std::endl
            << "   Counting          2               (1020)" << std::endl
            << "    -CountingStop    2s              (1021)" << std::endl
            << "    -CountingLong    2l              (1022)" << std::endl
            << "    -CountingTrigger 2t              (1023)" << std::endl
            << "    -CountingTime    2z              (1024)" << std::endl
            << "      --fast         2f              (1025)" << std::endl
            << "      --long         2l              (1026)" << std::endl
            << "      --very long    2vl             (1027)" << std::endl
            << std::endl
            << "                  Setup:                   " << std::endl
            << "   LoadThreshold                       (67)" << std::endl
            << "   SaveFSR                             (68)" << std::endl
            << "   LoadFSR           lf                (69)" << std::endl
            << "   SetDAC                              (70)" << std::endl
            << "   ChessMatrix                         (71)" << std::endl
            << "   UniformMatrix                       (72)" << std::endl
            << "   SaveMatrix                          (73)" << std::endl
            << "   LoadMatrix                          (74)" << std::endl
            << "   SetMatrix         4               (1040)" << std::endl
            << "   WriteReadFSR      5               (1050)" << std::endl
            << std::endl
            << "                Calibration, etc:          " << std::endl
            << "   EnableTPulse                        (60)" << std::endl
            << "   DisableTPulse                       (61)" << std::endl  
            << "   DACScan                             (62)" << std::endl
            << "   THLScan                             (63)" << std::endl
            << "   SCurve                              (64)" << std::endl
            << "   CurveFast                           (65)" << std::endl                           
    //            << "   ThresholdNoise                      (66)" << std::endl
            << "   THSopt                            (1060)" << std::endl
            << "   ThEqNoiseCenter                   (1070)" << std::endl
            << "   TOCalib                           (1080)" << std::endl
            << "    -TOCalibFast                     (1081)" << std::endl
            << std::endl
            << "                     IP:                   " << std::endl
            << "   SetIP                              (591)" << std::endl
            << "   ShowIP                             (592)" << std::endl
            << "                    Stuff:                 " << std::endl
            << "   MakeARP                             (75)" << std::endl
            << "   spacing                             (76)" << std::endl

            << std::endl;
}//end of printCommands


void printFadcCommands()
{
  std::cout << std::endl
	    << "-------------------------------------------" << std::endl
	    << "          Important general stuff:         " << std::endl 
	    << std::endl
            << "     Print General Commands            ( 0)" << std::endl
            << "     Print Commands TOS                (50)" << std::endl
            << "     Print Commands (FADC)             ( 1)" << std::endl
            << "     Go to classical TOS mode          (98)" << std::endl
            << "     Quit FADC, switch to TOS only     (57)" << std::endl
	    << "     Quit (all)                        (99)" << std::endl
	    << std::endl
	    << std::endl 
	    << "-------------------------------------------" << std::endl
	    << "               FADC Commands: (70..        " << std::endl  
	    << std::endl
	    << "H -- Print Settings                    ..02)" << std::endl
	    << "     Reset FADC                        ..03)" << std::endl
    	    << "     Start aquisition                  ..04)" << std::endl
	    << "     Send software trigger             ..05)" << std::endl
	    << "     Read/Release interrupt         ..06/07)" << std::endl
            << std::endl
	    << "     Set/get trigger threshold all  ..08/09)" << std::endl
            << "H -- Set/get trigger threshold all  ..10/11)" << std::endl
	    << "     Load trigger threshold            ..12)" << std::endl
	    << std::endl
	    << "     Set/Get trigger type           ..13/14)" << std::endl
	    << "     Set/Get trigger channel source ..15/16)" << std::endl
	    << "     Set/Get Posttrig               ..17/18)" << std::endl
	    << "     Set/Get Pretrig                ..19/20)" << std::endl
	    << std::endl
	    << "     Set/Get channel mask           ..21/22)" << std::endl
    	    << "     Set/Get #ch to use             ..23/24)" << std::endl
            << "     Set/Get Mode Register          ..27/28)" << std::endl
	    << std::endl
	    << "     Set/Get frequency              ..29/30)" << std::endl
	    << "     Set/Get read mode              ..31/32)" << std::endl
	    << std::endl
	    << "     Set/Get post stop latency      ..33/34)" << std::endl
	    << "     Set/Get post latency pretrig   ..35/36)" << std::endl
	    << "-------------------------------------------" << std::endl 
            << "     FADC - TOS Communication              " << std::endl
            << "     Readout FADC Bit                (1005)" << std::endl 
            << "     Read/Clear FADC Flag       (1006/1007)" << std::endl

	    << std::endl;
}


int getInputValue()
{

  //Prompt for input var
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


unsigned int getLongInputValue()
{

  //Prompt for input var
  std::cout << "Enter positive integer value: ";

  //Get input var
  unsigned int value = 0 ;
  std::cin.exceptions( std::istringstream::failbit );
  
  try{ //Check whether input is of valid type
    std::cin >> value ;
  }catch( std::istream::failure &e){ 
    std::cout << "Invalid input value, value is set to 0" << std::endl;    
    std::cin.clear();
  }
  return value;
}//end of getInputValue


unsigned short getShortInputValue()
{
  //Prompt for input var
  std::cout << "Enter positive integer value: ";

  //Get input var
  unsigned int value = 0 ;
  std::cin.exceptions( std::istringstream::failbit );
  
  try{ //Check whether input is of valid type
    std::cin >> value ;
  }catch( std::istream::failure &e){ 
    value = 5000;
    std::cout << "Invalid input value, value is set to 5000" << std::endl;    
    std::cin.clear();
  }
  return value;
}//end of getInputValue


//template<typename TYPE> TYPE getAllInputValue(int err)
template<class T> T getAllInputValue(T err)
{

  //Prompt for input var
  std::cout << "Enter positive integer value: ";

  //Get input var
  T value = 0 ;
  std::cin.exceptions( std::istringstream::failbit );
  
  try{ //Check whether input is of valid type
    std::cin >> value ;
  }catch( std::istream::failure &e){ 
    std::cout << "Invalid input value, value is set to " << err << std::endl;
    value = (T) err;    
    std::cin.clear();
  }
  return value;
}//end of getInputValue


int turnString2Int(std::string inputString)
{
  int var = 0;
  std::stringstream newString(inputString); 
  newString >> var;  
  return var;
} 
