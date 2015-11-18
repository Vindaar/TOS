// /**********************************************************************/
// /*                                                            tos.cpp */
// /*  TOS - Timepix Operating Software                                  */
// /*  with addtional readout software of a CAEN V1729a FADC             */
// /*  based on:                                                         */
// /*                                                                    */
// /*                                                         20.07.2009 */
// /*                                                    Christian Kahra */
// /*                                     chrkahra@students.uni-mainz.de */
// /*                                        Institut fuer Physik - ETAP */
// /*                              Johannes-Gutenberg Universitaet Mainz */
// /*                                                                    */
// /*  with changes by Michael Lupberger (Time Pix)                      */
// /*  and Alexander Deisting (FADC)                                     */
// /*                                                                    */
// /**********************************************************************/

// //TOS
#include "header.hpp"
#include "console.hpp"


// //STD C++
#include <cstdlib>

// //FADC stuff
#include "V1729a.h"
#include "V1729a_Dummy.h"
#include "V1729a_VME.h"

// // HV_FADC_Obj and related header files
#include "HV_FADC_Obj.h"
#include <QTextStream>
#include <QString>


int main(int argc, char *argv[])
{
    
    std::cout<<"\n\n\t-- Welcome to TOS --\n\nIf you need help, type: help"<<std::endl;

#ifdef __WIN32__
    WSADATA wsaData;
    int err;
    err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        printf("Could not find a usable version of Winsock.dll\n");
        WSACleanup();
        return 1;
    }
    else
        printf("The Winsock 2.2 dll was found okay\n");
#endif


    //just to be sure - initialise FADC
    // TODO: change all this to use HV_FADC_Obj
    // V1729a* myFADC = new V1729a_Dummy;
    bool useHV_FADC_Obj = false;
    QString iniFilePath("../config/HFOSettings.ini");

    //get parameter for the FADC use
    extern char* optarg;
    int option = 0;


    while( ( option = getopt( argc, argv, "v:g:n" ) ) != -1 ){
	switch( option ){
	case 'n' :
	    //do not use the FADC
	    std::cout << "Working without HV_FADC_Obj" << std::endl;
	    break;
	case 'v' :
	{
	    //work with a FADC via VME
	    std::cout << "Working with VME" << std::endl;
	    std::cout << "Please give the relative path to a configuration file for "
		      << "the HV_FADC_Obj. If no input is given, the default path:\n"
		      << "../config/HFOSettings.ini \n"
		      << "is used. " << std::endl;
	    QTextStream qtin(stdin);

	    iniFilePath = qtin.readLine();
	    if(iniFilePath.isEmpty() == true){
		iniFilePath = "../config/HFOSettings.ini";
	    }
	    std::cout << "Using path " << iniFilePath.toStdString() << std::endl;
	    //HV_FADC_Obj *myHV_FADC_Obj = new HV_FADC_Obj(input);
	    useHV_FADC_Obj = true;
	    break;
	}
	case 'g' :
	    // TODO: deprecated
	    //work with a FADC via GPIB - not implemented
	    //delete myFADC;
	    //myFADC = new V1729a_Dummy;
	    std::cout << std::endl << "GPIB is not supported" << std::endl << std::endl;
	    break;
	default:
	    std::cout << "Invalid selection - deleting FADC pointer" << std::endl;
	    break;
	} // switch( option )
    } // end of while( ( option = ... )


    //initialise TOS
    //std::cout<<"\nTOS> "<<std::flush;
    QCoreApplication app(argc, argv);
  
    //check if one wants to use the FADC
    if(useHV_FADC_Obj){
	//std::cout << "WARNING: Still FADC parameters hardcoded in tos.cpp main()" << std::endl;
    
	// TODO: add these to HFOSettings.ini
	// myFADC->setTriggerType(7);
	// myFADC->setTriggerType(7);

	// myFADC->setFrequency(2);
	// myFADC->setPosttrig(80);
	// myFADC->setPretrig(15000);

	//myFADC->setTriggerThresholdRegisterAll(2033);
	//myFADC->printSettings();

	
    
	Console* consoleHV_FADC = new Console(iniFilePath);
  
	if(consoleHV_FADC->okay()){
	    consoleHV_FADC->ConsoleMain();
	}

	delete(consoleHV_FADC);
	
    }
    else{
	Console console;
  
	if(console.okay()){
	    console.ConsoleMain();
        }
    }

    std::cout<<"TOS ends now...\n\tHave a nice day\n\n"<<std::flush;
#ifdef __WIN32__
    WSACleanup();
#endif
    return EXIT_SUCCESS;
}

