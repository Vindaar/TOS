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
// /*  and Tobias Schiffer (WIN)                                         */
// /*  and Sebastian Schmidt (HV_FADC, readline)                         */
// /*                                                                    */
// /**********************************************************************/

#include "console.hpp"
#include "hvFadcManager.hpp"

#include <QTextStream>
#include <QString>
#include <QCoreApplication>

#include <cstdlib>
#include <string>


#ifdef __WIN32__
const WORD MAX_CONSOLE_LINES = 500;
#endif

int main(int argc, char *argv[])
{
#ifdef __WIN32__
    //Open Console under Windows
    int hConHandle;
    long lStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    FILE *fp;

    // allocate a console for this app
    AllocConsole();

    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
    coninfo.dwSize.Y = MAX_CONSOLE_LINES;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

    // redirect unbuffered STDOUT to the console
    lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

    fp = _fdopen( hConHandle, "w" );

    *stdout = *fp;
    setvbuf( stdout, NULL, _IONBF, 0 );

    // redirect unbuffered STDIN to the console
    lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

    fp = _fdopen( hConHandle, "r" );

    *stdin = *fp;
    setvbuf( stdin, NULL, _IONBF, 0 );

    // redirect unbuffered STDERR to the console
    lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

    fp = _fdopen( hConHandle, "w" );

    *stderr = *fp;
    setvbuf( stderr, NULL, _IONBF, 0 );

    // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
    // point to console as well
    std::ios_base::sync_with_stdio();
#endif


    
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


    bool useHvFadcManager = false;
    std::string iniFilePath;

    //get parameter for the HV_FADC object use
    extern char* optarg;
    int option = 0;
    bool numericalInput = false;
    bool allowDefaultOnEmptyInput = true;


    while( ( option = getopt( argc, argv, "v:n" ) ) != -1 ){
	switch( option ){
	case 'n' :
	    //do not use the FADC
	    std::cout << "Working without hvFadcManager" << std::endl;
	    break;
	case 'v' :
	{
	    //work with a FADC via VME
	    std::cout << "Working with VME" << std::endl;
	    std::cout << "Please give the relative path to a configuration file for "
		      << "the hvFadcManger. If no input is given, the default path:\n"
		      << "../config/HFM_settings.ini \n"
		      << "is used. " << std::endl;
	    
	    iniFilePath = getUserInput("> ", numericalInput, allowDefaultOnEmptyInput);
	    if (iniFilePath == ""){
		iniFilePath = "../config/HFO_settings.ini";
	    }

	    //HV_FADC_Obj *myHV_FADC_Obj = new HV_FADC_Obj(input);
	    useHvFadcManager = true;
	    break;
	}
	default:
	    std::cout << "Invalid command line argument" << std::endl;
	    break;
	} // switch( option )
    } // end of while( ( option = ... )


    //initialise TOS
    //std::cout<<"\nTOS> "<<std::flush;
    QCoreApplication app(argc, argv);
  
    //check if one wants to use the FADC
    if(useHvFadcManager){
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

