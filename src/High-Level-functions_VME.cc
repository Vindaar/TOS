//TODO: Find a better name!
#include "High-Level-functions_VME.h"



// C'tor
HighLevelFunction_VME::HighLevelFunction_VME(V1729a* dev):
    _currentDevice(dev),
    _nb(100),
    _triggerThresholdRegister(5,0)
//:_currentDevice(dev){}
{
  //copy device pointer
  //_currentDevice = dev;

  //set "try" var
  //_nb = 10;
  
  //get the trigger threshold of the channels of fadc 
  _triggerThresholdRegister[0] = _currentDevice->getTriggerThresholdDACAll();
  _triggerThresholdRegister[1] = _currentDevice->getTriggerThresholdDACPerChannel(0);
  _triggerThresholdRegister[2] = _currentDevice->getTriggerThresholdDACPerChannel(1);
  _triggerThresholdRegister[3] = _currentDevice->getTriggerThresholdDACPerChannel(2);
  _triggerThresholdRegister[4] = _currentDevice->getTriggerThresholdDACPerChannel(3);
}


/// D'tor
HighLevelFunction_VME::~HighLevelFunction_VME()
{
  //before leaving the device we set the preload register of the threshold DACs to
  //the values of the vector (usefull since a lot of commands overwrite the registers)
  _currentDevice->setTriggerThresholdDACAll(_triggerThresholdRegister[0]);
  _currentDevice->setTriggerThresholdDACPerChannel(0,_triggerThresholdRegister[1]);
  _currentDevice->setTriggerThresholdDACPerChannel(1,_triggerThresholdRegister[2]);
  _currentDevice->setTriggerThresholdDACPerChannel(2,_triggerThresholdRegister[3]);
  _currentDevice->setTriggerThresholdDACPerChannel(3,_triggerThresholdRegister[4]);


  //lose the  fadc
  _currentDevice = NULL;
}



/*********** General Functions ***********/



//TODO: Decide which settings one wants to display
void HighLevelFunction_VME::printSettings()
{
  std::cout << std::endl << "Current Settings of the V1729a FADC:" << std::endl << std::endl;
  std::cout << "Frequency:      " << _currentDevice->getFrequency()     << " default:  1/2 (2/1Ghz)" << std::endl << std::endl;
  std::cout << "Read Mode:      " << _currentDevice->getReadMode()      << " default:  0" << std::endl;
  std::cout << "Mode Register:  " << _currentDevice->getModeRegister()  << " default:  0" << std::endl;
  std::cout << "Channel Mask:   " << _currentDevice->getChannelMask()   << " default: 15" << std::endl;
  std::cout << "NB of Channels: " << _currentDevice->getNbOfChannels()  << " default:  0" << std::endl << std::endl;
  std::cout << "Trigger Type:   " << _currentDevice->getTriggerType()   << " default:  3" << std::endl; 
  std::cout << "Trigger Channel Sources: " << _currentDevice->getTriggerChannelSource() << " default ?" << std::endl << std::endl;
  std::cout << "PreTrig:        " << _currentDevice->getPretrig()       << " default: 15000 - 85 seems sufficent" << std::endl;
  std::cout << "PostTrig:       " << _currentDevice->getPosttrig()      << " default: 64" << std::endl << std::endl;
  std::cout << "FPGA Version:   " << _currentDevice->getFPGAVersion()   << std::endl << std::endl;

  return;
}//end of printSettings



/*********** Set Functions ***********/


int HighLevelFunction_VME::setFrequencyH(const unsigned short& frequency)
{
    int timeout = _nb;
    unsigned short currentFreq = -1;

    while( (timeout > 0) &&
	   (currentFreq != frequency) ){
	// first set currentType to the currently set value on the FADC
	currentFreq = _currentDevice->getFrequency();
	// now set the new type on the FADC
	_currentDevice->setFrequency(frequency);

	// now wait a short while
	sleepModule();
	timeout--;
    }
    if (timeout < 1){
	std::cout << "Could not set frequency on the FADC!" << std::endl;
	std::cout << "wanted to set " << frequency << " is " << currentFreq << std::endl;
	return -1;
    }
    else{
	std::cout << "timeout value in setFrequencyH " << timeout << std::endl;
	std::cout << "wanted to set " << frequency << " is " << currentFreq << std::endl;
	return 0;
    }





  //   for(int iTry = 1; iTry <= _nb; iTry++)
  // {
  //   _currentDevice->setFrequency(frequency);
  //   if( _currentDevice->getFrequency() == ((unsigned int)frequency) )
  //   {
  //     return 0;
  //   }
  // }

  // std::cout << "[Error] -- setFrequencyVME: setFrequency failed! " 
  //   << _currentDevice->getFrequency() << std::endl;
  
  // return 1;  
}



int HighLevelFunction_VME::setPosttrigH(const unsigned short& bits)
{
    int timeout = _nb;
    unsigned short currentBits = -1;

    while( (timeout > 0) &&
	   (currentBits != bits) ){
	// first set currentBits to the currently set value on the FADC
	currentBits = _currentDevice->getPosttrig();
	// now set the new bits on the FADC
	_currentDevice->setPosttrig(bits);
	
	// now wait a short while
	sleepModule();
	timeout--;
    }
    if (timeout < 1){
	std::cout << "Could not set post trigger on the FADC!" << std::endl;
	std::cout << "wanted to set " << bits << " is " << currentBits << std::endl;
	return -1;
    }
    else{
	std::cout << "timeout value in setPostTrigH " << timeout << std::endl;
	std::cout << "wanted to set " << bits << " is " << currentBits << std::endl;
	return 0;
    }




  //   for(int iTry = 1; iTry <= _nb; iTry++)
  // {
  //   _currentDevice->setPosttrig(bits);
  //   if( _currentDevice->getPosttrig() == ((unsigned int)bits) )
  //   {
  //     return 0;
  //   }
  // }

  // std::cout << "[Error] -- setPosttrigVME: setting posttrig failed! " 
  //   << _currentDevice->getPosttrig() << std::endl;
  
  // return 1;  
}


int HighLevelFunction_VME::setPretrigH(const unsigned short& bits)
{
    int timeout = _nb;
    unsigned short currentBits = -1;

    while( (timeout > 0) &&
	   (currentBits != bits) ){
	// first set currentBits to the currently set value on the FADC
	currentBits = _currentDevice->getPretrig();
	// now set the new bits on the FADC
	_currentDevice->setPretrig(bits);
	
	// now wait a short while
	sleepModule();
	timeout--;
    }
    if (timeout < 1){
	std::cout << "Could not set pre trigger on the FADC!" << std::endl;
	std::cout << "wanted to set " << bits << " is " << currentBits << std::endl;
	return -1;
    }
    else{
	std::cout << "timeout value in setPreTrigH " << timeout << std::endl;
	std::cout << "wanted to set " << bits << " is " << currentBits << std::endl;
	return 0;
    }




  //   for(int iTry = 1; iTry <= _nb; iTry++)
  // {
  //   _currentDevice->setPretrig(bits);
  //   if( _currentDevice->getPretrig() == ((unsigned int)bits) )
  //   {
  //     return 0;
  //   }
  // }

  // std::cout << "[Error] -- setPretrigVME: setting pretrig failed! " 
  //   << _currentDevice->getPretrig() << std::endl;
  
  // return 1;  
}



/*********** Trigger Functions ***********/



int HighLevelFunction_VME::setTriggerTypeH( const unsigned short& type )
{

    int timeout = _nb;
    unsigned short currentType = -1;

    while( (timeout > 0) &&
	   (currentType != type) ){
	// first set currentType to the currently set value on the FADC
	currentType = _currentDevice->getTriggerType();
	// now set the new type on the FADC
	_currentDevice->setTriggerType(type);
	
	// now wait a short while
	sleepModule();
	timeout--;
    }
    if (timeout < 1){
	std::cout << "Could not set trigger type on the FADC!" << std::endl;
	std::cout << "wanted to set " << type << " is " << currentType << std::endl;
	return -1;
    }
    else{
	std::cout << "timeout value in setTriggerTypeH " << timeout << std::endl;
	std::cout << "wanted to set " << type << " is " << currentType << std::endl;
	return 0;
    }
    
    // for(int iTry = 1; iTry <= _nb; iTry++)
    // {
    // 	_currentDevice->setTriggerType(type);
    // 	if( _currentDevice->getTriggerType() == type )
    // 	{
    // 	    return 0;
    // 	}
    // }

    // std::cout << "[Error] -- setTriggerTypeH: setting trigger type failed! " 
    // 	      << _currentDevice->getTriggerType() << std::endl;
  
    // return 1;  
}


int HighLevelFunction_VME::setTriggerChannelSourceH(const unsigned short& mask )
{
    int timeout = _nb;
    unsigned short currentSource = -1;

    while( (timeout > 0) &&
	   (currentSource != mask) ){
	// first set currentBits to the currently set value on the FADC
	currentSource = _currentDevice->getTriggerChannelSource();
	// now set the new bits on the FADC
	_currentDevice->setTriggerChannelSource(mask);
	
	// now wait a short while
	sleepModule();
	timeout--;
    }
    if (timeout < 1){
	std::cout << "Could not set trigger channel source on the FADC!" << std::endl;
	std::cout << "wanted to set " << mask << " is " << currentSource << std::endl;
	return -1;
    }
    else{
	std::cout << "timeout value in setTriggerChannelSourceH " << timeout << std::endl;
	std::cout << "wanted to set " << mask << " is " << currentSource << std::endl;
	return 0;
    }





  //   for(int iTry = 1; iTry <= _nb; iTry++)
  // {
  //   _currentDevice->setTriggerChannelSource(mask);
  //   if( _currentDevice->getTriggerChannelSource() == ((unsigned short)mask) )
  //   {
  //     return 0;
  //   }
  // }

  // std::cout << "[Error] -- setTriggerChannelSourcesH: setting trigger channel sources failed! " 
  //   << _currentDevice->getTriggerChannelSource() << std::endl;
  
  // return 1;  
}


unsigned int HighLevelFunction_VME::setTriggerThresholdRegisterAll(const unsigned int threshold)
{
    //store the threshold in the register vec
    _triggerThresholdRegister[0] = threshold;
    _triggerThresholdRegister[1] = threshold;
    _triggerThresholdRegister[2] = threshold;
    _triggerThresholdRegister[3] = threshold;
    _triggerThresholdRegister[4] = threshold;


    int timeout = _nb;
    unsigned int thres0 = -1;
    unsigned int thres1 = -1;
    unsigned int thres2 = -1;
    unsigned int thres3 = -1;
    while( (timeout > 0) &&
	   ( (thres0 != threshold) ||
	     (thres1 != threshold) ||
	     (thres2 != threshold) ||
	     (thres3 != threshold) ) ){
	//write threshold to the register and...
	_currentDevice->setTriggerThresholdDACAll(threshold);
	// _currentDevice->setTriggerThresholdDACPerChannel(0, threshold);
	// _currentDevice->setTriggerThresholdDACPerChannel(1, threshold);
	// _currentDevice->setTriggerThresholdDACPerChannel(2, threshold);
	// _currentDevice->setTriggerThresholdDACPerChannel(3, threshold);

	// now read the current trigger thresholds
	// thres0 = _currentDevice->getTriggerThresholdDACPerChannel(0);
	// thres1 = _currentDevice->getTriggerThresholdDACPerChannel(1);
	// thres2 = _currentDevice->getTriggerThresholdDACPerChannel(2);
	// thres3 = _currentDevice->getTriggerThresholdDACPerChannel(3);
	thres0 = _currentDevice->getTriggerThresholdDACAll();
	thres1 = thres0;
	thres2 = thres0;
	thres3 = thres0;
		
	//...load it to the DACs
	_currentDevice->loadTriggerThresholdDAC();
	// now wait a short while
	sleepModule();
	timeout--;
    }
    if (timeout < 1){
	std::cout << "Could not set trigger threshold DAC for one or more channels." << std::endl;
	std::cout << "wanted to set "
		  << threshold
		  << " is "
		  << thres0 << "\t"
		  << thres1 << "\t"
		  << thres2 << "\t"
		  << thres3 << "\t"
		  << std::endl;
	return -1;
    }
    else{
	// everything worked well
	std::cout << "timeout value in setTriggerThresholdRegisterAll." << std::endl;
	std::cout << "wanted to set "
		  << threshold
		  << " is "
		  << thres0 << "\t"
		  << thres1 << "\t"
		  << thres2 << "\t"
		  << thres3 << "\t"
		  << std::endl;
	return 0;
    }
  
  
    // if(_currentDevice->getTriggerThresholdDACAll() != 0) return 1;
    // else return 0;
}


void HighLevelFunction_VME::getTriggerThresholdRegister()
{
    std::cout << "[Begin Message] -- getTriggerThresholdRegister:" << std::endl
	      << "getTriggerThreshold perChannel:" << std::endl
	      << "Channel 1: " << _triggerThresholdRegister[1] << std::endl
	      << "Channel 2: " << _triggerThresholdRegister[2] << std::endl
	      << "Channel 3: " << _triggerThresholdRegister[3] << std::endl
	      << "Channel 4: " << _triggerThresholdRegister[4] << std::endl
	      << "getTriggerThreshold All: " << _triggerThresholdRegister[0] << std::endl 
	      << "[End Message] -- getTriggerThresholdRegister" << std::endl << std::endl;
  
    return;
}



/*********** Readout Functions ***********/


/* If one plots the data, one has to take care of the right axis
 * title. (Set the time axis according to the frequency set)
 */
std::vector<std::vector<int> > HighLevelFunction_VME::correctData(std::vector<int> const& dataVec)
{
  std::cout << "[DEBUG] -- correctData -- dataVec.size() " << dataVec.size() << std::endl;
  
  //check if one channel (~10k datapoints), two channels (~5k) or four channel readout is set
  if(_currentDevice->getNbOfChannels() != 0) 
    std::cout <<  "[WARNING] -- correctData -- only 4 channel readout implemented!" << std::endl;
  if( (_currentDevice->getFrequency()) > 2)
    std::cout <<  "[WARNING] -- correctData -- correction for trigger position only works with frequencys higher then 0.5GHz" << std::endl;

  //create vector for data output, therefore ....
  std::vector<std::vector<int> > corrDataVec;

  //...get number of channels
  unsigned short channelMask = _currentDevice->getChannelMask();
  unsigned int channels = 4;

  if( !(channelMask & 8) ) channels--;
  if( !(channelMask & 4) ) channels--;
  if( !(channelMask & 2) ) channels--;
  if( !(channelMask & 1) ) channels--;

  std::cout << "[DEBUG] -- correctData -- Number of channels: " << channels << std::endl;

  //...and create one subvector for each channel
  for(unsigned int iCreate = 0; iCreate < channels+1; iCreate++)
  {
    std::vector<int> temp;
    corrDataVec.push_back(temp);
  }

     
  //Get the parameter needed for the time correction of the data
  unsigned int trigger_rec = _currentDevice->getTriggerRecord();
  std::cout << "[DEBUG] -- correctData -- Trigger Record: " << trigger_rec << std::endl;
  
  unsigned int posttrig = _currentDevice->getPosttrig();
  std::cout << "[DEBUG] -- correctData -- posttrig: " << posttrig << std::endl;
     
  unsigned int oldId = 0;

  //Correct data
  for(unsigned int iVector = 3*channels; iVector < 2563*channels; iVector++)
  {
    const int iData = iVector - (3 * channels) + 1;
    int ch = (channels - ( iData % channels ))  - 1; 
    
    //std::cout << iVector << " " << iData << " " << ch << std::endl;
    
    //one finds the correction formula in the manual at p. 14 manual
    //as it turns out there had to be a 1 (or 2) with respect to the given formula
    //to ensure all this works properly
    unsigned int var = 3 - (_currentDevice->getFrequency());
    unsigned int end_cell = 20*(( posttrig + 128 + var - trigger_rec ) % 128);
    unsigned int newId = (2560 + oldId - end_cell) % 2560;
    
    if( ch == (int)( channels - 1 ) ) 
    {
      corrDataVec.at(0).push_back(newId);
      oldId++;
    }
    corrDataVec.at(ch+1).push_back(dataVec[iVector]); //*4.0/16384.0
  }    
 
  return corrDataVec; 
}//end of correctData


//If we plot data it's in the order 4,3,2,1 in the columns of data files (we may have to fix that)
void HighLevelFunction_VME::printDataToFile(std::vector<int> const& dataVec, std::string fileName)
{
  //check if one channel (~10k datapoints), two channels (~5k) or four channel readout is set
  if(_currentDevice->getNbOfChannels() != 0) 
    std::cout <<  "[WARNING] -- printDataToFile -- only 4 channel readout implemented!" << std::endl;

  //get number of channels
  const unsigned short channelMask = _currentDevice->getChannelMask();
  //check which readout is activated
  const unsigned short nbOfChannels = _currentDevice->getNbOfChannels();
  
  unsigned int channels = 4;
  
  //set nb of channels for the readout
  if((nbOfChannels != 1) && (nbOfChannels != 2))
  {
    if( !(channelMask & 8) ) channels--;
    if( !(channelMask & 4) ) channels--;
    if( !(channelMask & 2) ) channels--;
    if( !(channelMask & 1) ) channels--;

  }  
  
  std::cout << "[DEBUG] -- printDataToFile -- Number of channels: " << channels << std::endl;


  //check if 12 or 14 bit sampling mode is activated...
  int mode = (_currentDevice->getModeRegister() & 2)/2;
  double countsToVolt = 1.0/4096.0;
  //...and if adjust the conversation factor from counts to volt
  if(mode == 1) countsToVolt = 1.0/8192.0;

  std::cout << "[DEBUG] -- printDataToFile -- 2nd Bit Mode Register: " << mode << std::endl;

 
  //create outputfile
  std::stringstream fileNameStream;
  //fileNameStream << fileName << "-chM" << channelMask;
  fileNameStream << fileName;
  std::ofstream outFile( fileNameStream.str().c_str() );
    

  //write data to output file
  if( outFile.is_open() )
  {
    //write petrig/postrig/etc to file
    outFile << "# nb of channels: " << std::endl << nbOfChannels << std::endl;
    outFile << "# channel mask: " << std::endl << channelMask << std::endl;
    outFile << "# postrig: " << std::endl << _currentDevice->getPosttrig() << std::endl;
    outFile << "# pretrig: " << std::endl << _currentDevice->getPretrig() << std::endl;
    outFile << "# triggerrecord: " << std::endl << _currentDevice->getTriggerRecord() << std::endl;
    outFile << "# frequency: " << std::endl << _currentDevice->getFrequency() << std::endl;

    outFile << "#Data:" << std::endl;

    //skip 'first sample', 'venier' and 'rest baseline' (manual p27)
    for(unsigned int iData = 0; iData < 3*channels; iData++)
    { 
      outFile << "#" << dataVec[iData] << std::endl;
    }
    
    
    //TODO: For odd channels there are two lines    
    for(unsigned int iVector = 3*channels; iVector < 2563*channels; iVector++)
    {
      const int iData = iVector - 3 * channels + 1;
      int ch = channels - ( iData % channels ) -1; 
                  
      //let's transform the units of the FADC to Volt
      outFile << double(dataVec[iVector]*countsToVolt);
                        
      if( ch == (int)( channels -1 ) )
      {
	outFile << std::endl;
      }
      else{
	outFile << "\t";
      }
    }
    
    outFile << std::endl;
        
    for(unsigned int iRest = channels * 2563; iRest < dataVec.size(); iRest++)
    {
      outFile << "# " << dataVec.at(iRest) << std::endl; 
    }
    outFile.close();
  }
  
  std::cout << "[DEBUG] -- printDataToFile -- Data is written to " << fileNameStream.str() << std::endl; 
 
  return;
}//end of printDataToFile


void HighLevelFunction_VME::printDataToFile(std::vector<std::vector<int> > const& dataVec, std::string fileName)
{
  //check if one channel (~10k datapoints), two channels (~5k) or four channel readout is set
  if(_currentDevice->getNbOfChannels() != 0) 
    std::cout <<  "[WARNING] -- printDataToFile -- only 4 channel readout implemented!" << std::endl;

  //get number of channels
  const unsigned short channelMask = _currentDevice->getChannelMask();
  unsigned int channels = 4;

  if( !(channelMask & 8) ) channels--;
  if( !(channelMask & 4) ) channels--;
  if( !(channelMask & 2) ) channels--;
  if( !(channelMask & 1) ) channels--;

  std::cout << "[DEBUG] -- printDataToFile -- Number of channels: " << channels << std::endl;

  
  //check if 12 or 14 bit sampling mode is activated...
  int mode = (_currentDevice->getModeRegister() & 2)/2;
  double countsToVolt = 1.0/4096.0;
  //...and if adjust the conversation factor from counts to volt
  if(mode == 1) countsToVolt = 1.0/8192.0;

  std::cout << "[DEBUG] -- printDataToFile -- 2nd Bit Mode Register: " << mode << std::endl;
  

  //create outputfiles
  std::stringstream fileNameStream;
  fileNameStream << fileName << "-chM" << channelMask;
  std::ofstream outFile( fileNameStream.str().c_str() );
  //debug stuff
  //std::ofstream debugFile("debug.txt");
  

  //write out corrected data
  if( outFile.is_open() )
  {
    /**TODO: There are still some Problems with the read out - the corrections made in the 
     *following lines can only be considerd as "brutal force" 
     */

    bool modeReg = false; //var to change the correction according to the read mode set
    if( (_currentDevice->getModeRegister() & 2) == 2) modeReg = true;
    bool help = false;    //helper var needed if second readmode bit is set to 0
                    
 
    for(unsigned int iPrint = 0; iPrint < dataVec.at(channels-1).size(); ++iPrint)
    {
      outFile << dataVec.at(0).at(iPrint);

      for(unsigned int iCh = 1; iCh < dataVec.size(); ++iCh)
      { 
        if(!modeReg) //ergo: modeReg @ 12 bit mode
	{       
          /*Works for two/four channels: no correction needed
            For one/three channel/s: move every 2nd vlaue down by one
	   */
          if(channels % 2 == 0) 
	  {
            outFile << "\t" << (double(dataVec.at(iCh).at(iPrint)))*countsToVolt;
	  }
	  else
          {
            if(help) outFile << "\t" << (double(dataVec.at(iCh).at(iPrint))-4096.0)*countsToVolt;
          }
	}//end of if(!modeReg)
	else //ergo: modeReg @ 14 bit mode
	{
	  /*For four/two channels: there is a gap [1:1.5] in mode 3: adding -0.5 to each value about 1Volt
	    For one/three channels there are dual lines everytime the signal gets bigger then 1
           */
	  if((channels % 2) == 0)
	  { 
            if( (double(dataVec.at(iCh).at(iPrint))*countsToVolt) > 1.0)
	    {
              /*this way to correct channels still causes "dual lines"!! (but one line has 
                only a few points.)
                TODO: Understand the "20 feature" (some kind of periodic noise)
	       */ 
              outFile << "\t" << (double(dataVec.at(iCh).at(iPrint)))*countsToVolt - 0.5;
	    }
            else
	    {
              outFile << "\t" << (double(dataVec.at(iCh).at(iPrint)))*countsToVolt;
	    }
	  }//end of if((channels % 2) == 0)
	  else //ergo: odd channels
	  {
            if(double(dataVec.at(iCh).at(iPrint))*countsToVolt > 1.5)
	    {
              /*this way to correct channels still causes "dual lines"!! (but one line has 
                only a few points.)
                TODO: Understand the "20 feature" (some kind of periodic noise)
	       */
              outFile << "\t" << (double(dataVec.at(iCh).at(iPrint)))*countsToVolt - 0.5;
	    }
            else
	    {
              outFile << "\t" << (double(dataVec.at(iCh).at(iPrint)))*countsToVolt;
	    }
	  }//end of else "odd channels"
        }//end of else "14 bit mode"
      }
      
      help = !help;

      outFile << std::endl;
    }
    outFile.close();    
  }
     
  std::cout << "[DEBUG] -- printDataToFile -- Data is written to " << fileNameStream.str() << std::endl;

  return;
}//end of printDataToFile


void HighLevelFunction_VME::sleepModule(){
    // function which calls sleep for this thread for time given by 
    // macro in header

    std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_FADC_SLEEP_TIME));
}
