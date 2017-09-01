#include "hvTempMonitor.hpp"

void WriteTempsIntoRunFolder(hvFadcManager *hfm, std::atomic_bool &loop_continue, std::string path_name){
    /* this function provides the functionality of the init_and_log_temps()
       function, which was previously called in PC::runFADC() to create
       a temperatue log file for each run in the run folder.
       Here the HFM's Temps _currentTemps member variable is used, which 
       is being set by the BackgroundTempLoop() function.
       The PC::runFADC() then calls this function before starting the run.

       Inputs:
       hvFadcManager *hfm: pointer to the HFM object, which stores the current
                           temperature values
       std::atomic_bool loop_continue: variable used to stop this thread when
                           the run is being stopped.
       std::string path_name: name for the run folder, in which the log should 
                            be written

       Function reads the _currentTemps every second, uses currentDateTime() to
       get a TOS syntax date string and dumps date, temps rows into a temp_log.txt
       file in a run folder
    */

    std::string date = "";

    // get the full filepath for the temp_log.txt, open and write header
    std::string filepath = get_temp_log_file_path(path_name);

    std::fstream outfile;
    outfile.open(filepath, std::fstream::out);
    outfile << "# Temperature log file" << "\n"
	    << "# Temp_IMB \t Temp_Septem \t DateTime" << std::endl;
    outfile.close();

    Temps temps;

    while(loop_continue == true){

	temps = hfm->GetCurrentTemps();
	int temp_imb = temps.first;
	int temp_septem = temps.second;

	// get current date and time
	date = currentDateTime();

	// now output temps to file
	outfile << temp_imb << "\t"
		<< temp_septem << "\t"
		<< date << std::endl;

	// wait 1000 milliseconds, one row per second is more than enough, esp.
	// given that temps are only updated every ~5 s currently.
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    // close outfile
    outfile.close();
}
