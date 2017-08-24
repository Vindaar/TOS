#include "V1729a_VME.h"


// C'tor
V1729a_VME::V1729a_VME( CVmUsb *Controller, int sAddress ): vme::DeviceVME( Controller, VME_OFFSET * sAddress ){}
// : MAX_DATA_POINTS( 2564 ) {  /* 2564 is the maximum number of data points in the RAM (per channel)*/ 




/***********  General funcions  ***********/

// void V1729a_VME::printSettings() throw()
// {
//   std::cout << "(V1729a_VME) print some settings" << std::endl;
// }


inline void V1729a_VME::reset() throw() 
{
  write32( 0x0, RESET, 0x0D );
}


inline void V1729a_VME::startAcquisition() throw() 
{
  //std::cout << "debug SA " << START_ACQUISITION << std::endl;
  write32( 0x0, START_ACQUISITION, 0x0D );
}


// Returns the current readout frequency
unsigned int V1729a_VME::getFrequency() throw() 
{
  return read32( FP_FREQUENCY, 0x0D ) & 63;
}


// Sets the frequncy
void V1729a_VME::setFrequency( const unsigned short& frequency ) throw() 
{
  write32( frequency & 63, FP_FREQUENCY, 0x0D );
}


// FIXME: Change read modes for additional speed in the readout
void V1729a_VME::setReadMode( const unsigned short& mode ) throw() 
{
  write32( mode & 3, FAST_READ_MODES, 0x0D );
}


// FIXME: Get read mode (0 is default for normal readout ) 
unsigned int V1729a_VME::getReadMode() throw() 
{
  return read32( FAST_READ_MODES, 0x0D ) & 3;
}


// Returns the version of the FPGA on the board
unsigned int V1729a_VME::getFPGAVersion() throw() 
{
  return read32( FPGA_VERSION, 0x0D ) & 255;
}



/***********  Trigger settings  ***********/



inline void V1729a_VME::setTriggerThresholdDACAll(const unsigned int threshold) throw(){
#if DEBUG==2    
  std::cout << "[DEBUG] -- setTriggerThresholdDACAll -- adress: " << std::hex 
    << TRIGGER_THRESH_DAC_ALL << std::dec << std::endl;
  std::cout << "[DEBUG] -- setTriggerThresholdDACAll -- value: " << threshold << std::endl;
  std::cout << "writing to ALL " << threshold << std::endl;
#endif
  write32(threshold, TRIGGER_THRESH_DAC_ALL, 0x0D );
}


inline unsigned int V1729a_VME::getTriggerThresholdDACAll() throw(){
    //TODO: 4095 correct? (would refer to FFF)
#if DEBUG==2
    std::cout << "[DEBUG] -- getTriggerThresholdAll -- adress: " << std::hex 
	      << TRIGGER_THRESH_DAC_ALL << std::dec << std::endl;
#endif
    int temp;
    temp = read32(TRIGGER_THRESH_DAC_ALL, 0x0D);
#if DEBUG==2
    std::cout << "read32 for ALL returns " << temp << std::endl;
#endif
    return temp & 4095;
}


//We start from channel 1 and add h100 to get to the other channels
inline void V1729a_VME::setTriggerThresholdDACPerChannel(const unsigned short chNb, 
							 const unsigned int threshold) throw()
{
  std::cout << "[DEBUG] -- setTriggerThresholdDAC -- adress: " << std::hex 
    << TRIGGER_THRESH_DAC_CH1 + 0x100*chNb << std::dec << std::endl;
  std::cout << "[DEBUG] -- setTriggerThresholdDAC -- value: " << threshold << std::endl;
  std::cout << "writing " << threshold << std::endl;
  write32(threshold, TRIGGER_THRESH_DAC_CH1 + 0x100*chNb, 0x0D );
}


//We start from channel 1 and add h100 to get to the other channels
inline unsigned int V1729a_VME::getTriggerThresholdDACPerChannel(const unsigned short chNb) throw(){
  //TODO: 4095 correct? (would refer to FFF)
#if DEBUG==2    
  std::cout << "[DEBUG] -- getTriggerThreshold -- adress: " << std::hex 
    << TRIGGER_THRESH_DAC_CH1 + 0x100*chNb << std::dec << std::endl;
#endif
  int temp;
  temp = read32( TRIGGER_THRESH_DAC_CH1 + 0x100*chNb, 0x0D);
#if DEBUG==2
  std::cout << "read32 returns " << temp << std::endl;
#endif
  return temp & 4095; //& 65535;
}


/* Loads the trigger threshold DAC set before by setTriggerThresholdDAC()
   so that it is actully put to use.
*/
inline void V1729a_VME::loadTriggerThresholdDAC() throw(){
#if DEBUG==2
  std::cout << "[DEBUG] -- loadTriggerThresholdDAC -- adress: " << std::hex 
    << LOAD_TRIGGER_THRESHOLD << std::dec << std::endl;
#endif
  write32( 0x0, LOAD_TRIGGER_THRESHOLD, 0x0D );
}


inline unsigned int V1729a_VME::getTriggerRecord() throw() 
{
  return read32( TRIGGER_REC, 0x0D ) & 255;
}



// Send software trigger. fault.
inline void V1729a_VME::sendSoftwareTrigger() throw() 
{
  write32( 0x0, SOFTWARE_TRIGGER, 0x0D );
}


// Set the number of databits to be recorded BEFORE a trigger signal arrives
void V1729a_VME::setPretrig( const unsigned short& bits ) throw() {

	  int data = (bits >> 8) & 255;
	  write32( data, PRETRIG_MSB, 0x0D );

	  data = bits & 255;
	  write32( data, PRETRIG_LSB, 0x0D );
}


// Returns the current pretrig
unsigned short V1729a_VME::getPretrig() throw() {

	int pretrig = read32( PRETRIG_MSB, 0x0D ) & 255;

	pretrig = pretrig << 8;

	pretrig += ( read32( PRETRIG_LSB, 0x0D ) & 255 );

	return pretrig;
}


// Set the number of databits to be recorded AFTER a trigger signal arrives
void V1729a_VME::setPosttrig( const unsigned short& bits ) throw() {

	int data = (bits >> 8) & 255;
	write32( data, POSTTRIG_MSB, 0x0D );

	data = bits & 255;
	write32( data, POSTTRIG_LSB, 0x0D );
}


// Returns the current postrig by reading of and combinig the 
// posstrig lsb and msb
unsigned int V1729a_VME::getPosttrig() throw() 
{
  int posttrig = read32( POSTTRIG_MSB, 0x0D ) & 255;
  posttrig = posttrig << 8;
  posttrig += ( read32( POSTTRIG_LSB, 0x0D ) & 255 );

  return posttrig;
}


//TODO: test some changes see p.32
inline void V1729a_VME::setTriggerType( const unsigned short& type ) throw() 
{
  write32( type & 255, TRIGGER_TYPE, 0x0D );
  //write32( 3, 0x1D00, 0x0D );
}


// Returns the type of the current trigger
inline unsigned short V1729a_VME::getTriggerType() throw() 
{
  return read32( TRIGGER_TYPE, 0x0D ) & 255;
  //return read32( 0x1D00, 0x0D ) & 255;
}


// Set mask on channels for trigger acceptence
inline void V1729a_VME::setTriggerChannelSource( const unsigned short& mask ) throw() 
{
  write32( mask & 15, TRIGGER_CHANNEL_SOURCE, 0x0D );
}



// Returns the trigger channel mask 
inline unsigned short V1729a_VME::getTriggerChannelSource() throw() 
{
  return read32( TRIGGER_CHANNEL_SOURCE, 0x0D ) & 15;
}




/***********  READOUT SETTINGS  ***********/


// FIXME:
inline unsigned short V1729a_VME::getModeRegister() throw()
{
  return read32( MODE_REGISTER, 0x0D ) & 7;
}


//FIXME:
inline void V1729a_VME::setModeRegister(const unsigned short& mode) throw()
{
  write32( mode & 7, MODE_REGISTER, 0x0D );
}


// Set the number of columns to read
inline void V1729a_VME::setColToRead( const unsigned short& col ) throw() {

	write32( col & 255, NB_OF_COLS_TO_READ, 0x0D );
}


// Returns the number of columns to read
inline unsigned short V1729a_VME::getColToRead() throw() 
{
  return read32( NB_OF_COLS_TO_READ, 0x0D ) & 255;
}


// Set channel mask for the readout 
inline void V1729a_VME::setChannelMask( const unsigned short& mask ) throw() 
{
  write32( mask & 15, CHANNEL_MASK, 0x0D );
}


// Get readout channel mask
inline unsigned short V1729a_VME::getChannelMask() throw() 
{
  return read32( CHANNEL_MASK, 0x0D ) & 15;
}


void V1729a_VME::setNbOfChannels(const unsigned short& nbOfChannels) throw()
{
  write32( nbOfChannels & 3, NB_OF_CHANNELS, 0x0D );
}


/// Returns the number of set read-out-channels
inline unsigned short V1729a_VME::getNbOfChannels() throw()
{
  return read32(NB_OF_CHANNELS , 0x0D ) & 3;
}


// Set post stop latency 
void V1729a_VME::setPostStopLatency( const unsigned short& latency ) throw() 
{ 
  write32( latency & 255, POST_STOP_LATENCY, 0x0D );
}


// Read post stop latency
unsigned short V1729a_VME::getPostStopLatency() throw() { 

  return read32( POST_STOP_LATENCY, 0x0D ) & 255;
}


// Set post latency pretrig
void V1729a_VME::setPostLatencyPretrig( const unsigned short& latency ) throw() {

	write32( latency & 255, POST_LATENCY_PRETRIG, 0x0D );
}


// Get post latency pretrig
unsigned short V1729a_VME::getPostLatencyPretrig() throw() {

	return read32( POST_LATENCY_PRETRIG, 0x0D ) & 255;
}


// Set internal  address of data in the RAM
void V1729a_VME::setRAMAddress( const unsigned short& add ) throw() {

	int ramAddr = ( add >> 8 ) & 255;

	write32( ramAddr, RAM_INT_ADD_MSB, 0x0D );

	ramAddr = add & 255;

	write32( ramAddr, RAM_INT_ADD_LSB, 0x0D );
}


// Get internal address of data in the RAM
unsigned short V1729a_VME::getRAMAddress() throw() { 

	int ramAddr = read32( RAM_INT_ADD_MSB, 0x0D ) & 255;

	ramAddr = ( ramAddr << 8 );

	ramAddr += ( read32( RAM_INT_ADD_LSB, 0x0D ) & 255 );

	return ramAddr;
}


//get data from the fadc.
//TODO: Something is still buggy for an odd number of channels
std::vector<int> V1729a_VME::getAllData( const unsigned short& nbChannels) throw() 
{
  //the max number of channels would be channels*2563+3
  const int max = 2563 * nbChannels + 3;
  const int reads = max / 2;

  std::vector<int> dataVec( max );

  int data = 0, data1 = 0, data2 = 0;
  for( int iRead = 0; iRead < reads; ++iRead )
  {
    data = read32( RAM_DATA_VME, 0x0D );
    //read16( RAM_DATA_VME, 0x0D );
    //read32( RAM_DATA_VME, 0x0D );

    data1 = data & 16383; 
    dataVec[ (iRead * 2) + 1 ] = data1;
    //iRead ] = data1; 
    // (iRead * 2) + 1 ] = data1;

    data2 = ( data >> 16 ) & 16383;
    dataVec[ (iRead * 2) ] = data2;
  }

  return dataVec;
}


std::vector<int> V1729a_VME::getAllDataBlockMode() throw() 
{
  //find the number of channels
  const int channelMask = (read32( CHANNEL_MASK, 0x0D ) & 15);
  unsigned int channels = 4;

  if( !(channelMask & 8) ) channels--;
  if( !(channelMask & 4) ) channels--;
  if( !(channelMask & 2) ) channels--;
  if( !(channelMask & 1) ) channels--;

  int maxDataPoints = 2563 * channels + 3; //2564*channels;
  int blockReads = maxDataPoints / 64;
  int rest = maxDataPoints % 64;


  std::vector<int>  dataVec( maxDataPoints );
  std::vector<int>::iterator dataIter = dataVec.begin();

  for( int iRead = 0; iRead < blockReads; ++iRead )
  {
    std::vector<int> tmp = readBlock32( RAM_DATA_VME, 64, 0x0F );
    std::vector<int>::iterator tmpIter = tmp.begin();

    while( (tmpIter != tmp.end()) && (dataIter != dataVec.end()) )
    {
      int data = *tmpIter;
    
      *dataIter = ( data >> 16 ) & 8191;
      ++dataIter;
      *dataIter = data & 8191;

      ++dataIter;
      tmpIter++;
    } // end of while( tmpIter != tmp.end() && dataIter != dataVec.end() )
  }// end of for( int iRead = 0; iRead < blockReads; ++iRead )

  //FIXME: reading out the "rest" results in a memory leak
  std::vector<int> tmp = readBlock32( RAM_DATA_VME, rest, 0x0F );
  std::vector<int>::iterator iter = tmp.begin();

  while( iter != tmp.end() && dataIter != dataVec.end() )
  {
    int data = *iter;

    *dataIter = ( data >> 16 ) & 8191;
    ++dataIter;
    *dataIter = data & 8191;

    ++dataIter;
    ++iter;
  }

  return dataVec;
}


int V1729a_VME::getDataAt( const unsigned short& add ) throw(){

  int data = read32( RAM_DATA_VME, 0x0D );

  int tmp = data & 8191;

  tmp = (data >> 16);

  tmp = tmp & 8191;

  return add;
}



/***********  Interrupt settings  ***********/



// Reset interrupt
inline void V1729a_VME::releaseInterrupt() throw() 
{
  //std::cout << "debug INTRPT " << INTERRUPT << std::endl;
  write32( 0x0, INTERRUPT, 0x0D );
}


// Read interrupt
bool V1729a_VME::readDeviceInterrupt() throw() 
{
  //std::cout << "debug INTRPT " << INTERRUPT << std::endl;
  int interrupt = read32( INTERRUPT, 0x0D ) & 1 ;
  
  return (interrupt == 1 ? true : false);
}


bool V1729a_VME::readInterrupt() throw() 
{
  return readDeviceInterrupt();
}
