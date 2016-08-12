#include "waitconditions.hpp"
#include "pc.hpp"

#include <chrono>
#include <thread>


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
    bool fadcReadoutNextEvent = false;

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
	if((parent->_useHvFadc) && !(parent->_hvFadcManager == NULL))
	{
	    parent->mutexVBuffer.lock();               
	    (parent->_hvFadcManager)->F_StartAcquisition();       //< start acq
	    parent->mutexVBuffer.unlock();             

	    std::cout << "fadc active" << std::endl;
	}

	// in case we use an external trigger, we call CountingTrigger()
	if (parent->_useExternalTrigger == true){
	    // set fpga->UseFastClock to the value of _useFastClock
	    parent->fpga->UseFastClock(parent->_useFastClock);
	    result = parent->fpga->CountingTrigger(parent->shutterTime);
	    // after counting, deactivate fast clock variable again
	    parent->fpga->UseFastClock(false);
	    if(result!=20){(parent->RunIsRunning)=false;}
	}
	// else we call CountingTime()
	else{
	    // set fpga->UseFastClock to the value of _useFastClock
	    parent->fpga->UseFastClock(parent->_useFastClock);
	    result = parent->fpga->CountingTime(parent->shutterTime, parent->shutter_mode);
	    // after counting, deactivate fast clock variable again
	    parent->fpga->UseFastClock(false);
	    if(result!=20){(parent->RunIsRunning)=false;}	    
	}
   
	parent->mutexVBuffer.lock();               
	//If buffer is full, wait
	if (parent->DataInBuffer == parent->BufferSize){
	    parent->bufferNotFull.wait(&(parent->mutexVBuffer));
	}
	parent->mutexVBuffer.unlock();             


	//check if simultainious FADC and chip readout is activated
	if(parent->_useHvFadc)                   
	{
	    parent->mutexVBuffer.lock();             
	    //if there was a trigger from the fadc: stop data taking and readout the chip and fadc event
	    // if((parent->fpga->ReadoutFadcFlag()) == 1)
	    // {
	    fadcReadout = true;
	    parent->fpga->ClearFadcFlag();
	    //}
	    parent->mutexVBuffer.unlock();

	    // TODO: check what this means?!
	    //To FIX the readout problem
	    parent->fpga->DataChipFPGA(result);
	}
 
    
	//Producer filling the VBuffer (or a readout vec) for the readout
	for (unsigned short chip = 0; chip < parent->fpga->tp->GetNumChips(); chip++)
	{
	    std::vector<int> *dataVec  = new std::vector<int>(12288+1,0);

	    #if DEBUG==2
	    std::cout << "Producer run " 
		      << i 
		      << " adding bufferentry: " 
		      << (i % parent->BufferSize) 
		      << " chip: " 
		      << chip+1 
		      << std::endl;
	    #endif

	    //chip readout
	    //To FIX the readout problem
	    // TODO: understand this
	    if(parent->_useHvFadc){
		parent->fpga->DataFPGAPC(dataVec, chip + 1);
	    }
	    else{
		parent->fpga->SerialReadOutReadSend(dataVec, chip + 1);
	    }
	    #if DEBUG==2
	    std::cout << "Producer NumHits chip: " << chip+1 
		      << " " << dataVec->at(0) 
		      << std::endl;
	    #endif

	    //send events without fadc signal to the consumer or ...
	    //To FIX the readout problem
	    //if(!fadcReadoutNextEvent) (parent->Vbuffer)[(i % parent->BufferSize)][chip] = dataVec;
	    if(!fadcReadout){
		(parent->Vbuffer)[(i % parent->BufferSize)][chip] = dataVec;
	    }
	    //... readout events with fadc count
	    else                                     
	    {
		std::map<std::string, int> fadcParams;           //< vector containing fadc params
        
		parent->mutexVBuffer.lock();

		//fill params vector
		fadcParams = parent->_hvFadcManager->GetFadcParameterMap();
		fadcParams["eventNumber"] = i;
		//get nb of channels
		int channels = 4;

		if( (fadcParams["NumChannels"] !=1 ) && (fadcParams["NumChannels"] !=2) ){
		    if( !(fadcParams["ChannelMask"] & 8) ) channels--;
		    if( !(fadcParams["ChannelMask"] & 4) ) channels--;
		    if( !(fadcParams["ChannelMask"] & 2) ) channels--;
		    if( !(fadcParams["ChannelMask"] & 1) ) channels--;
		}

		//get fadc data
		//TODO/FIXME one wants to use channels instead of 4 as parameter of the next function?
		// TODO: fix this!!!
		std::vector<int> fadcData = (parent->_hvFadcManager)->F_GetAllData(4);
                   
		parent->readoutFadc(parent->PathName, fadcParams, dataVec, fadcData); 
		parent->mutexVBuffer.unlock();

		//bugfix
		std::vector<int> *tmpVec  = new std::vector<int>(12288+1,0);
		(parent->Vbuffer)[(i % parent->BufferSize)][chip] = tmpVec;                 
	    }

	    //if there was an fadc event: remeber to read out chip and fadc at the next event: 
	    if(fadcReadout) fadcReadoutNextEvent = true;
	    else fadcReadoutNextEvent = false;

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
	    (i % 10 == 0))
	{
	    // TODO: change the frequency of this. do not want hardcoded, but rather
	    //       based on time (input given in HFO_settings.ini)
	    int isGood = 0;
	    parent->mutexVBuffer.lock();
	    // TODO: deprecated. Call different function!
	    isGood = (parent->_hvFadcManager)->H_CheckHVModuleIsGood(true);
	    parent->mutexVBuffer.unlock();             
	    if (isGood == -1){
		// this means something is wrong with HV
		// - call a function to report about problem (dump to file)
		// - stop the run with error message
		parent->mutexVBuffer.lock();
		(parent->_hvFadcManager)->H_DumpErrorLogToFile(i);
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
	std::string FileName[9] = {""};
	for (unsigned short chip = 1;chip <= parent->fpga->tp->GetNumChips() ;chip++){
	    sstream << "data" << i << "_" << chip << ".txt";
	    FileName[chip]=parent->PathName+"/"; 
	    FileName[chip]+=sstream.str();
	    sstream.str("");
	}
	//build filename(s)
	struct   timeval  tv;
	struct   timezone tz;
	struct   tm      *tm;
	int hh, mm, ss, us;
	int   ms;

	gettimeofday(&tv, &tz);
	tm = localtime(&tv.tv_sec);
	hh = tm->tm_hour;                          // hours
	mm = tm->tm_min;                           // minutes
	ss = tm->tm_sec;                           // seconds
	us = tv.tv_usec;                           // micro seconds
	ms = tv.tv_usec/1000;                      // mili seconds

	std::string FileName2 = "";
	sstream << "data" << std::setw(6) << std::setfill('0') << i <<"_1_"
		<< std::setw(2) << std::setfill('0') << hh
		<< std::setw(2) << std::setfill('0') << mm
		<< std::setw(2) << std::setfill('0') << ss
		<< std::setw(3) << std::setfill('0') << ms <<".txt";

	std::cout << "Path Name " << parent->PathName << std::endl;
	FileName2 = parent->PathName + "/" + sstream.str(); 

	sstream.str("");
	// next line does not exist in Tobys code anymore
	//sstream.clear();
    

	parent->mutexVBuffer.lock();
	//check if the data buffer isn't empty
	if (parent->DataInBuffer == 0){
	    parent->bufferNotEmpty.wait(&(parent->mutexVBuffer));
	}
	parent->mutexVBuffer.unlock();

	int hits [9] = {0};
	//create output file
	FILE* f2 = fopen(FileName2.c_str(),"w");

	for (unsigned short chip = 0; chip < parent->fpga->tp->GetNumChips(); chip++)
	{
	    int NumHits = ((parent->Vbuffer)[(i % parent->BufferSize)][chip])->at(0);
	    std::cout << "Consumer run " << i 
		      << " Numhits: " << NumHits 
		      << " on chip " << chip+1 
		#if DEBUG==1
		      << " buffer entry: " << i % parent->BufferSize 
		#endif
		      << std::endl;

	    hits[chip+1] = ((((parent->Vbuffer[(i % parent->BufferSize)][chip]))->size()) - 1)/3;

	    #if DEBUG==1
	    if (parent->IsRunning()){
		std::cout << "IsRunning is true, Consumer run " 
			  << i << " hits: " 
			  << hits[chip+1] << std::endl;
	    }
	    else{
		std::cout << "IsRunning is false, Consumer run " 
			  << i << " hits: " 
			  << hits[chip+1] << std::endl;
	    }
	    #endif

	    //if there is a problem with the output file: ...
	    if(f2 == NULL) 
	    {
		//... stop Run
		std::cout << "(Consumer) Dateifehler" << std::endl; 
		(parent->RunIsRunning) = false;
	    }
      
	    //print data to file(s)
	    for (std::vector<int>::iterator it = ((parent->Vbuffer[i % parent->BufferSize][chip])->begin())+1; 
		 it != ((parent->Vbuffer[i % parent->BufferSize][chip])->end()); 
		 it  = it + 3) 
	    {
		if (*(it+2) !=0) fprintf(f2, "%d %d %d \n", *it, *(it+1), *(it+2));
	    }
      
	    //delete recorded data after printing it
	    delete (parent->Vbuffer)[(i % parent->BufferSize)][chip];
	}//close for (unsigned short chip = 0;chip < p...

	fclose(f2);

    
	parent->mutexVBuffer.lock();
	--(parent->DataInBuffer);
	//... wake up all the other threads (in case of pure chip readout) and ..
	parent->bufferNotFull.wakeAll();
	parent->mutexVBuffer.unlock();


	for (unsigned short chip = 0; chip < parent->fpga->tp->GetNumChips(); chip++)
	{
	    if(hits[chip+1]<0){(parent->RunIsRunning)=false;}
	}

	i++;
    }//end of while(parent->DataAcqRunning || (parent->DataInBuffer) != 0)

    fprintf(stderr, "\n");
    std::cout << "Run finished: " << i << " frames recorded \n> " << std::flush;
}
