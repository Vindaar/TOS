#include "waitconditions.hpp"
// #include "pc.hpp"
// #include "hvFadcManager.hpp"
// #include "fpga.hpp"

#include <QWaitCondition>

#include <iostream>
#include <fstream>


/* Here the Consumer and Producer Threads for the multi-threaded readout are implemented
 *
 */


//Producer Thread
void Producer::run()
{
#if DEBUG == 2
    std::cout << "Producer::run()" << std::endl;
#endif

    int i = 0;
    int result;
    time_t start = time(NULL);

    /* Since the FPGA::SerialReadOutReadSend function moves one event from the chip to the
     * fpga and the event from fpga to the pc, one has to wait on event if there was a 
     * signal at the fadc to assign the right chip event to the right fadc event. This
     * var is used as flag to ensure this.
     */
    //up to now: it doesen't - see the "To FIX.." comments
    // bool fadcReadoutNextEvent = false;

    // check whether we're running with the FADC, if so, enable the closing of
    // the shutter based on a trigger by the FADC
    if(parent->_useHvFadc == true){
	parent->fpga->EnableFADCshutter(1);
    }
  
    //open shutter for a given time
    while(parent->IsRunning())
    {
	bool fadcReadout = false;                    //< helper var to trigger the fadc readout

	// TODO: change for usage with HV_FADC_Obj
	//start measurement at the fadc
	if((parent->_useHvFadc) && !(parent->_hvFadcManager == NULL)){
	    // before we start the acquisition, reset the i2c value of the timepix
	    // object (FADC trigger at clock cycle calculated from it)
	    parent->fpga->tp->SetI2C(0);
	    // reset extra byte
	    parent->fpga->tp->SetExtraByte(0);
	    // and reset the calculated fadc trigger at clock cycle
	    // setting FADC trigger to -1 in case it did not trigger
	    parent->_hvFadcManager->SetFadcTriggerInLastFrame(-1);
	    // and the scintillator counters
	    parent->_hvFadcManager->SetScintillatorCounters(0, 0);
	    
	    parent->mutexVBuffer.lock();               
	    (parent->_hvFadcManager)->F_StartAcquisition();       //< start acq
	    parent->mutexVBuffer.unlock();             
	}

	// in case we use an external trigger, we call CountingTrigger()
	if (parent->_useExternalTrigger == true){
	    // set fpga->UseFastClock to the value of _useFastClock
	    parent->fpga->UseFastClock(parent->_useFastClock);
	    result = parent->fpga->CountingTrigger(parent->shutterTime);
	    // after counting, deactivate fast clock variable again
	    parent->fpga->UseFastClock(false);
	    if(result!=20){
		(parent->RunIsRunning)=false;
	    }
	}
	// else we call CountingTime()
	else{
	    // set fpga->UseFastClock to the value of _useFastClock
	    parent->fpga->UseFastClock(parent->_useFastClock);
	    result = parent->fpga->CountingTime(parent->shutterTime, parent->shutter_mode);
	    // after counting, deactivate fast clock variable again
	    parent->fpga->UseFastClock(false);
	    if(result!=20){
		(parent->RunIsRunning)=false;
	    }
	}
   
	parent->mutexVBuffer.lock();               
	//If buffer is full, wait
	if (parent->DataInBuffer == parent->BufferSize){
	    parent->bufferNotFull.wait(&(parent->mutexVBuffer));
	}
	parent->mutexVBuffer.unlock();             


	//check if simultaneous FADC and chip readout is activated
	if(parent->_useHvFadc){
	    parent->mutexVBuffer.lock();             
	    //if there was a trigger from the fadc: stop data taking and readout the chip and fadc event
	    if((parent->fpga->ReadoutFadcFlag()) == 1){
		fadcReadout = true;
		parent->fpga->ClearFadcFlag();
	    }
	    parent->mutexVBuffer.unlock();

	}
	
	// now call the first function for the readout
	parent->fpga->DataChipFPGA(result);
    
	//Producer filling the VBuffer (or a readout vec) for the readout
	for (unsigned short chip = 0; chip < parent->fpga->tp->GetNumChips(); chip++){
	    std::vector<int> *dataVec  = new std::vector<int>(12288+1,0);

	    #if DEBUG==2
	    std::cout << "Producer run " 
		      << i 
		      << " adding bufferentry: " 
		      << (i % parent->BufferSize) 
		      << " chip: " 
		      << chip 
		      << std::endl;
	    #endif

	    //chip readout
	    //To FIX the readout problem
	    // TODO: understand this
	    //if(parent->_useHvFadc){
	    parent->fpga->DataFPGAPC(dataVec, chip);

	    // NOTE: The following was from the time, when one was still using a single Chip
	    // the SerialReadOutReadSend function is not properly implemented in the Virtex Firmware for 
	    // multiple chips!
	    //}
	    // else{
	    // 	parent->fpga->SerialReadOutReadSend(dataVec, chip + 1);
	    // }
	    #if DEBUG==2
	    std::cout << "Producer NumHits chip: " << chip
		      << " " << dataVec->at(0) 
		      << std::endl;
	    #endif

	    
	    // now also add information of this frame to the runMap, so that
	    // we can write it to the file for the frame
	    parent->_runMap["useHvFadc"]   = parent->_useHvFadc;
	    parent->_runMap["fadcReadout"] = fadcReadout;

	    //send events without fadc signal to the consumer or ...
	    //To FIX the readout problem
	    //if(!fadcReadoutNextEvent) (parent->Vbuffer)[(i % parent->BufferSize)][chip] = dataVec;
	    // start by writing the data we read from the chip to the VBuffer
	    (parent->Vbuffer)[(i % parent->BufferSize)][chip] = dataVec;

	    if ( (parent->_useHvFadc == true) &&
		 (fadcReadout == true) &&
		 (chip == parent->_center_chip) ){
		// in case we're using the FADC and we're currently on the center chip,
		// we're going to read out the FADC
        
		parent->mutexVBuffer.lock();

		//fill params map
		std::map<std::string, int> fadcParams;
		fadcParams = parent->_hvFadcManager->GetFadcParameterMap();
		// add current event number to params map
		fadcParams["eventNumber"] = i;
		// now also read out the clock cycle at which the FADC triggered in the last
		// frame and add it tot he fadcParams map
		fadcParams["fadcTriggerClock"] = parent->_hvFadcManager->GetFadcTriggerInLastFrame();
		// and get the corresponding clock values between scintillator events and this
		// clock cycle
		std::pair<unsigned short, unsigned short> scint_pair;
		scint_pair = parent->_hvFadcManager->GetScintillatorCounters();
		fadcParams["scint1ClockInt"] = scint_pair.first;
		fadcParams["scint2ClockInt"] = scint_pair.second;
		
		//get nb of channels
		unsigned short channels = 4;

		if( (fadcParams["NumChannels"] !=1 ) && (fadcParams["NumChannels"] !=2) ){
		    channels = parent->_hvFadcManager->FADC_Functions->getNumberOfActiveChannels(fadcParams["channelMask"]);
		}

		//get fadc data
		std::vector<int> fadcData = (parent->_hvFadcManager)->F_GetAllData(channels);
		std::cout << "saving data to parent pointer. will probably fail " << std::endl;
		// set the data we read from the FADC as the values for our fadc pointer
		parent->_fadcData   = fadcData;
		// and set the parameter map to the parameter map pointer
		parent->_fadcParams = fadcParams;
                   
		//parent->readoutFadc(parent->PathName, fadcParams, dataVec, fadcData);
		// set the VBuffer for this chip to the data vector
		parent->mutexVBuffer.unlock();

		//bugfix
		// std::vector<int> *tmpVec  = new std::vector<int>(12288+1,0);
		// (parent->Vbuffer)[(i % parent->BufferSize)][chip] = tmpVec;                 
	    }

	    //if there was an fadc event: remeber to read out chip and fadc at the next event: 
	    // if(fadcReadout) fadcReadoutNextEvent = true;
	    // else fadcReadoutNextEvent = false;

	}//for (unsigned short chip = 0;..

	parent->mutexVBuffer.lock();               
	++(parent->DataInBuffer);                  
	parent->mutexVBuffer.unlock();             

	//since there is new stuff in the buffer: wake up the the other threads
	parent->bufferNotEmpty.wakeAll();

	//send software trigger (to stop the measurement if there wasn't an event at the fadc)
	if((parent->_useHvFadc) && !(parent->_hvFadcManager == NULL))
	{
	    std::cout << "sending software trigger to FADC" << std::endl;
	    parent->mutexVBuffer.lock();
	    (parent->_hvFadcManager)->F_SendSoftwareTrigger();
	    parent->mutexVBuffer.unlock();
	}

	// increase event number
	i++;
    
	//check if enough frames are recorded or if reason to stop run early
	if ((parent->_useHvFadc) && 
	    !(parent->_hvFadcManager == NULL) &&
	    (i % 10 == 0)){
	    // TODO: change the frequency of this. do not want hardcoded, but rather
	    //       based on time (input given in HFO_settings.ini)
	    int hvGood = 0;
	    bool tempsGood = false;
	    parent->mutexVBuffer.lock();
	    // TODO: deprecated. Call different function!
	    hvGood = (parent->_hvFadcManager)->H_CheckHVModuleIsGood(true);
	    parent->mutexVBuffer.unlock();

	    // hand the current temperatures to a function def. in helper_functions.cpp
	    // to check if we should stop the run based on temps
	    tempsGood = (parent->_hvFadcManager)->CheckIfTempsGood(_temps);

	    if (hvGood == -1 || tempsGood == false){
	    	// this means something is wrong with HV
	    	// - call a function to report about problem (dump to file)
	    	// - stop the run with error message
		if(hvGood == -1){
		    std::cout << "ERROR: HV module in non-good state. Stopping run!" << std::endl;
		}
		if(tempsGood == false){
		    std::cout << "ERROR: temperatures out of bounds. Stopping run!" << std::endl;
		}
	    	parent->mutexVBuffer.lock();
	    	(parent->_hvFadcManager)->H_DumpErrorLogToFile(i, _temps->first, _temps->second);
	    	parent->mutexVBuffer.unlock();
	    	parent->StopRun();
	    }
	}

	if (parent->runtimeFrames == 1)
	{
	    if (i == parent->runtime)
	    {
		parent->StopRun();
	    }
	}
	if (parent->runtimeFrames == 0)
	{
	    std::cout << "Time running: " << difftime(time(NULL),start) 
		      << " seconds" << std::endl;
	    if ((difftime(time(NULL),start)) >= parent->runtime){
		parent->StopRun();
	    }
	}

    }//end of while(parent->IsRunning())

    // check whether we're running with the FADC, if so, disable the closing of
    // the shutter based on a trigger by the FADC
    if(parent->_useHvFadc == true){
	parent->fpga->EnableFADCshutter(0);
    }

    parent->DataAcqRunning = false;
    #if DEBUG==2
    std::cout << "Producer while loop ended"<< std::endl;
    #endif
}




//Comsumer Thread
void Consumer::run()
{
#if DEBUG == 2
    std::cout << "Consumer::run()" << std::endl;
#endif

    std::ostringstream sstream;                  //< var to build filename
    int i = 0;                                   //< counter var to count frames
  
    while(parent->DataAcqRunning || (parent->DataInBuffer) != 0)
    {
	// std::string FileName[9] = {""};
	// for (unsigned short chip = 1;chip <= parent->fpga->tp->GetNumChips() ;chip++){
	//     sstream << "data" << i << "_" << chip << ".txt";
	//     FileName[chip]=parent->PathName+"/"; 
	//     FileName[chip]+=sstream.str();
	//     sstream.str("");
	// }
	// //build filename(s)
	// struct   timeval  tv;
	// struct   timezone tz;
	// struct   tm      *tm;
	// int hh, mm, ss, us;
	// int   ms;

	// gettimeofday(&tv, &tz);
	// tm = localtime(&tv.tv_sec);
	// hh = tm->tm_hour;                          // hours
	// mm = tm->tm_min;                           // minutes
	// ss = tm->tm_sec;                           // seconds
	// us = tv.tv_usec;                           // micro seconds
	// ms = tv.tv_usec/1000;                      // mili seconds

	// std::string FileName2 = "";
	// sstream << "data" << std::setw(6) << std::setfill('0') << i <<"_1_"
	// 	<< std::setw(2) << std::setfill('0') << hh
	// 	<< std::setw(2) << std::setfill('0') << mm
	// 	<< std::setw(2) << std::setfill('0') << ss
	// 	<< std::setw(3) << std::setfill('0') << ms <<".txt";

	// std::cout << "Path Name " << parent->PathName << std::endl;
	// FileName2 = parent->PathName + "/" + sstream.str(); 

	// sstream.str("");

	std::string filePathName;
	// first we build the filename for the output file based on the PathName defined in the
	// PC constructor, false ( no pedestal run ) and i as the event number


	// TODO: understand how this works, if one calls the Run function without activating the
	// HFM. In that case, _hvFadcManager should be pointing to NULL, thus the function 
	// should not be well defined.
	filePathName = parent->_hvFadcManager->buildFileName(parent->PathName, false, i);
	
	parent->mutexVBuffer.lock();
	//check if the data buffer isn't empty
	if (parent->DataInBuffer == 0){
	    parent->bufferNotEmpty.wait(&(parent->mutexVBuffer));
	}
	parent->mutexVBuffer.unlock();


	// get current date and time to output that into event file
	std::string curDateAndTime;
	curDateAndTime = currentDateTime();
	

	// before we write actual data to the file, we write a header for the file
	std::fstream outfile;
	// get the parameters we are going to print into the header from the _runMap
	int runTimeM          	 = boost::any_cast<int>(parent->_runMap["runTime"]); 
	int runTimeFramesM    	 = boost::any_cast<unsigned short>(parent->_runMap["runTimeFrames"]);
	int runNumberM           = boost::any_cast<int>(parent->_runMap["runNumber"]); 
	std::string pathNameM 	 = boost::any_cast<std::string>(parent->_runMap["pathName"]);
	int numChipsM         	 = boost::any_cast<unsigned short>(parent->_runMap["numChips"]);
	int shutterTimeM      	 = boost::any_cast<int>(parent->_runMap["shutterTime"]);
	std::string shutterModeM = boost::any_cast<std::string>(parent->_runMap["shutterMode"]);
	int runModeM             = boost::any_cast<unsigned short>(parent->_runMap["runMode"]);
	bool fastClockM          = boost::any_cast<bool>(parent->_runMap["fastClock"]);
	bool extTriggerM         = boost::any_cast<bool>(parent->_runMap["externalTrigger"]);
	bool useHvFadcM          = boost::any_cast<bool>(parent->_runMap["useHvFadc"]);
	bool fadcReadoutM        = boost::any_cast<bool>(parent->_runMap["fadcReadout"]);
	
	outfile.open(filePathName, std::fstream::out);
	outfile << "## [General]"                            << "\n"
	        << "## runNumber:        " << runNumberM     << "\n"
	        << "## runTime:       	 " << runTimeM       << "\n"
		<< "## runTimeFrames: 	 " << runTimeFramesM << "\n"
		<< "## pathName:         " << pathNameM      << "\n"
		<< "## dateTime:         " << curDateAndTime << "\n"
		<< "## numChips:         " << numChipsM      << "\n"
		<< "## shutterTime:      " << shutterTimeM   << "\n"
		<< "## shutterMode:      " << shutterModeM   << "\n"
		<< "## runMode:          " << runModeM       << "\n"
		<< "## fastClock:        " << fastClockM     << "\n"
		<< "## externalTrigger:  " << extTriggerM    << "\n"
		<< "## [Event]"                              << "\n"
		<< "## eventNumber:      " << i              << "\n"
		<< "## useHvFadc:    	 " << useHvFadcM     << "\n"
		<< "## fadcReadout:    	 " << fadcReadoutM   << "\n"
		<< "## szint1ClockInt:	 " << parent->_fadcParams["scint1ClockInt"] << "\n"
		<< "## szint2ClockInt:	 " << parent->_fadcParams["scint2ClockInt"] << "\n"
		<< "## fadcTriggerClock: " << parent->_fadcParams["fadcTriggerClock"]
		<< std::endl;
	
	int hits [9] = { };
	// we now run over all chips, call the writeChipData function 
	for (unsigned short chip = 0; chip < parent->fpga->tp->GetNumChips(); chip++)
	{
	    int NumHits = ((parent->Vbuffer)[(i % parent->BufferSize)][chip])->at(0);
	    std::cout << "Consumer run " << i 
		      << " Numhits: " << NumHits 
		      << " on chip " << chip 
		#if DEBUG==1
		      << " buffer entry: " << i % parent->BufferSize 
		#endif
		      << std::endl;

	    hits[chip] = ((((parent->Vbuffer[(i % parent->BufferSize)][chip]))->size()) - 1)/3;

	    #if DEBUG==1
	    if (parent->IsRunning()){
	    	std::cout << "IsRunning is true, Consumer run " 
	    		  << i << " hits: " 
	    		  << hits[chip] << std::endl;
	    }
	    else{
	    	std::cout << "IsRunning is false, Consumer run " 
	    		  << i << " hits: " 
	    		  << hits[chip] << std::endl;
	    }
	    #endif

	    // now call the writeChipData function to write the vector of this chip
	    // to file

	    if ( (parent->_useHvFadc == true) &&
		 (fadcReadoutM == true) &&
		 (chip == parent->_center_chip) ){
		// if we're at the center chip, call the readoutFadc function, which internally
		// calls writeChipData and also writes the FADC data to file
		parent->readoutFadc(parent->PathName, parent->_fadcParams, parent->Vbuffer[i % parent->BufferSize][chip], parent->_fadcData);
	    }
	    else{
		// in case we're currently not at the center chip, just call writeChipData
		// or the HFM is not initialized
		parent->writeChipData(filePathName, parent->Vbuffer[i % parent->BufferSize][chip], chip);
	    }

	    // now call readoutFADC to print the FADC 

	    //if there is a problem with the output file: ...
	    // if(f2 == NULL) 
	    // {
	    // 	//... stop Run
	    // 	std::cout << "(Consumer) Dateifehler" << std::endl; 
	    // 	(parent->RunIsRunning) = false;
	    // }
      
	    // //print data to file(s)
	    // for( std::vector<int>::iterator it = ((parent->Vbuffer[i % parent->BufferSize][chip])->begin())+1; 
	    // 	 it != ((parent->Vbuffer[i % parent->BufferSize][chip])->end()); 
	    // 	 it  = it + 3 ) {
	    // 	if (*(it+2) !=0) fprintf(f2, "%d %d %d \n", *it, *(it+1), *(it+2));
	    // }

	    // if we're currently at the center chip, also readout the FADC
      
	    //delete recorded data after printing it
	    delete (parent->Vbuffer)[(i % parent->BufferSize)][chip];
	}//close for (unsigned short chip = 0;chip < p...

	parent->mutexVBuffer.lock();
	--(parent->DataInBuffer);
	//... wake up all the other threads (in case of pure chip readout) and ..
	parent->bufferNotFull.wakeAll();
	parent->mutexVBuffer.unlock();


	for (unsigned short chip = 0; chip < parent->fpga->tp->GetNumChips(); chip++){
	    if(hits[chip]<0){
		(parent->RunIsRunning)=false;
	    }
	}

	i++;
    }//end of while(parent->DataAcqRunning || (parent->DataInBuffer) != 0)

    fprintf(stderr, "\n");
    std::cout << "Run finished: " << i << " frames recorded \n> " << std::flush;
}
