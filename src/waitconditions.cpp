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
     * var is uesd as flag to ensure this.
     */
    //up to now: it doesen't - see the "To FIX.." comments
    bool fadcReadoutNextEvent = false;
  
    //open shutter for a given time
    while(parent->IsRunning())
    {
	bool fadcReadout = false;                    //< helper var to trigger the fadc readout

	// TODO: change for usage with HV_FADC_Obj
	//start measurement at the fadc
	if((parent->_useHvFadc) && !(parent->_hvFadcObj == NULL))
	{
	    parent->mutexVBuffer.lock();               
	    (parent->_hvFadcObj)->F_StartAcquisition();       //< start acq
	    parent->mutexVBuffer.unlock();             

	    std::cout << "fadc active" << std::endl;
	}

    
	if (parent->shutter_mode == 0){
	    // Trigger when trigger used
	    result=parent->fpga.CountingTime(parent->shutter, 0);
	    if(result!=20){(parent->RunIsRunning)=false;}
	}
	if (parent->shutter_mode == 1)  {
	    // Trigger when trigger used
	    result=parent->fpga.CountingTrigger(parent->shutter);
	    if(result!=20){(parent->RunIsRunning)=false;}
	}
	if (parent->shutter_mode == 2)  {
	    // Trigger when trigger used
	    parent->fpga.UseFastClock(true);
	    result=parent->fpga.CountingTime(parent->shutter, 0);
	    parent->fpga.UseFastClock(false);
	    if(result!=20){(parent->RunIsRunning)=false;}
	}
	if (parent->shutter_mode == 3) {
	    // Trigger when trigger used
	    parent->fpga.UseFastClock(true);
	    result=parent->fpga.CountingTrigger(parent->shutter);
	    parent->fpga.UseFastClock(false);
	    if(result!=20){(parent->RunIsRunning)=false;}
	}
	if (parent->shutter_mode == 4) {
	    // Trigger when trigger used
	    // calling CountingTime with second argument == 1
	    // corresponds to n = 1, power of 256
	    result=parent->fpga.CountingTime(parent->shutter, 1);
	    if(result!=20){(parent->RunIsRunning)=false;}
	}
	if (parent->shutter_mode == 5) {
	    // Trigger when trigger used
	    // calling CountingTime with second argument == 2
	    // corresponds to n = 2, power of 256
	    result=parent->fpga.CountingTime(parent->shutter, 2);
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
	    if((parent->fpga.ReadoutFadcFlag()) == 1)
	    {
		fadcReadout = true;
		parent->fpga.ClearFadcFlag();
	    }
	    parent->mutexVBuffer.unlock();           

	    // TODO: check what this means?!
	    //To FIX the readout problem
	    parent->fpga.DataChipFPGA(result);
	}
 
    
	//Producer filling the VBuffer (or a readout vec) for the readout
	for (unsigned short chip = 0; chip < parent->fpga.tp.GetNumChips(); chip++)
	{
	    std::vector<int> *dataVec  = new std::vector<int>(12288+1,0);
	    std::cout << "Producer run " 
		      << i 
		      <<" adding bufferentry: " 
		      << (i % parent->BufferSize) 
		      << " chip: " 
		      << chip+1 
		      << std::endl;

	    //chip readout
	    //To FIX the readout problem
	    // TODO: understand this
	    if(parent->_useHvFadc){
		parent->fpga.DataFPGAPC(dataVec,chip+1);
	    }
	    else{
		parent->fpga.SerialReadOutReadSend(dataVec,chip+1);
	    }
	    std::cout << "Producer NumHits chip: " << chip+1 
		      << " " << dataVec->at(0) 
		      << std::endl;

	    //send events without fadc signal to the consumer or ...
	    //To FIX the readout problem
	    //if(!fadcReadoutNextEvent) (parent->Vbuffer)[(i % parent->BufferSize)][chip] = dataVec;
	    if(!fadcReadout){
		(parent->Vbuffer)[(i % parent->BufferSize)][chip] = dataVec;
	    }
	    //... readout events with fadc count
	    else                                     
	    {
		std::vector<int> fadcParams;           //< vector containing fadc params
        
		parent->mutexVBuffer.lock();

		//fill params vector
		fadcParams.push_back((parent->_hvFadcObj)->F_GetNbOfChannels());
		fadcParams.push_back((parent->_hvFadcObj)->F_GetChannelMask());
		fadcParams.push_back((parent->_hvFadcObj)->F_GetPosttrig());
		fadcParams.push_back((parent->_hvFadcObj)->F_GetPretrig());
		fadcParams.push_back((parent->_hvFadcObj)->F_GetTriggerRecord());
		fadcParams.push_back((parent->_hvFadcObj)->F_GetFrequency());
		fadcParams.push_back((parent->_hvFadcObj)->F_GetModeRegister());

		//get nb of channels
		int channels = 4;

		if( (fadcParams.at(0) !=1 ) && (fadcParams.at(0) !=2))
		{
		    if( !(fadcParams.at(1) & 8) ) channels--;
		    if( !(fadcParams.at(1) & 4) ) channels--;
		    if( !(fadcParams.at(1) & 2) ) channels--;
		    if( !(fadcParams.at(1) & 1) ) channels--;
		}	

		//get fadc data
		//TODO/FIXME one wants to use channels instead of 4 as parameter of the next function?
		// TODO: fix this!!!
		std::vector<int> fadcData = (parent->_hvFadcObj)->F_GetAllData(4);
                   
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
	if((parent->_useHvFadc) && !(parent->_hvFadcObj == NULL))
	{
	    parent->mutexVBuffer.lock();
	    (parent->_hvFadcObj)->F_SendSoftwareTrigger();
	    parent->mutexVBuffer.unlock();
	}

	// increase event number
	i++;
    
	//check if enough frames are recorded or if reason to stop run early
	if ((parent->_useHvFadc) && 
	    !(parent->_hvFadcObj == NULL) &&
	    (i % 10 == 0))
	{
	    // TODO: change the frequency of this. do not want hardcoded, but rather
	    //       based on time (input given in HFO_settings.ini)
	    int isGood = 0;
	    parent->mutexVBuffer.lock();
	    isGood = (parent->_hvFadcObj)->H_CheckHVModuleIsGood();
	    parent->mutexVBuffer.unlock();             
	    if (isGood == -1){
		// this means something is wrong with HV
		// - call a function to report about problem (dump to file)
		// - stop the run with error message
		parent->mutexVBuffer.lock();
		(parent->_hvFadcObj)->H_DumpErrorLogToFile(i);
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

    parent->DataAcqRunning = false;
    std::cout << "Producer while loop ended"<< std::endl;
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
	for (unsigned short chip = 1;chip <= parent->fpga.tp.GetNumChips() ;chip++){
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

	for (unsigned short chip = 0; chip < parent->fpga.tp.GetNumChips(); chip++)
	{
	    int NumHits = ((parent->Vbuffer)[(i % parent->BufferSize)][chip])->at(0);
	    std::cout << "Consumer run " << i 
		      << " Numhits: " << NumHits 
		      << " on chip " << chip+1 
		      << " buffer entry: " << i % parent->BufferSize 
		      << std::endl;

	    hits[chip+1] = ((((parent->Vbuffer[(i % parent->BufferSize)][chip]))->size()) - 1)/3;

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
		 it= it + 3) 
	    {
		if (*(it+2) !=0) fprintf(f2, "%d %d %d \n", *it, *(it+1), *(it+2));
	    }
      
	    //delete recorded data after printing it
	    delete (parent->Vbuffer)[(i % parent->BufferSize)][chip];
	    std::cout << "deleted" << std::endl;
	}//close for (unsigned short chip = 0;chip < p...

	fclose(f2);

    
	parent->mutexVBuffer.lock();
	--(parent->DataInBuffer);
	//... wake up all the other threads (in case of pure chip readout) and ..
	parent->bufferNotFull.wakeAll();
	parent->mutexVBuffer.unlock();


	for (unsigned short chip = 0; chip < parent->fpga.tp.GetNumChips(); chip++)
	{
	    if(hits[chip+1]<0){(parent->RunIsRunning)=false;}
	}



	i++;
    }//end of while(parent->DataAcqRunning || (parent->DataInBuffer) != 0)

    fprintf(stderr, "\n");
    std::cout << "Run finished: " << i << " frames recorded \n> " << std::flush;
}
