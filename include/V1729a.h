#ifndef V1729a_H
#define V1729a_H 1


// STD C++
#include <vector>
#include <utility>
#include <string>

/** Abstrac class for the readout of of the V1729a
 *  FlashADC from CAEN. This class only defines a common
 *  interface for the readout via VME.
 *  Reference to the manual allwayes refers to the V1729a
 *  user manual by CAEN.
 *
 *  The implementations of the derived classes are not
 *  expected to validate type and format of the input
 *  values for its methods. They are also only intended
 *  to allow access to the different registers and
 *  commands of the V1729a ADC, they shall not provide any
 *  higher level functions. Higher level methods (like
 *  pedestal substraction, data realignment, readout
 *  schemes and so on should be defined in a different class
 *  which uses this FADC class only to access the device.
 *
 *  For details on the implementation of the derived classes
 *  for access by VME refer to the documentation for
 *  classes itself.
 *
 *  @Author Thorsten Krautscheid
 *  (little changes by Alexander Deisting)
 *
 **/




class V1729a{

 public:

  /// C'tor
  V1729a(){};

  /// D'tor
  virtual ~V1729a(){};




  /***********  General funcions  ***********/
  
  /** Prints the actual settings of the FADC to
   * console or to a file
   **/
  //virtual void printSettings() throw() = 0;
  //now in HighLevelFunction


  /** Resets the FADC and returns it to idle state. This 
   *  command does not change any settings like channel
   *  mask or columns to read.
   **/
  virtual void reset() throw() = 0;


  /** Starts an acqusition of the FADC. This will also reset the
   *  the interrupt register to 0.
   */
  virtual void startAcquisition() throw() = 0;


 /// Sets the readout frequency of the device ( 2 GHz = 1, 1 GHz = 2 )
  virtual void setFrequency( const unsigned short& frequency ) throw() = 0;


  /** Returns the current readout frequency of the device: 2 GHz = 1, 1 GHz = 2 ).
   *  These values are the total sampling frequency of the device. However
   *  The clock frequency of the device is only: 100 MHz = 1, 50 MHz = 2.
   */
  virtual unsigned int getFrequency() throw() = 0;


  /** Changes the read mode for additional speed in the readout (see 
   *  manual p. 34). Maybe a source of noise.
   */
  virtual void setReadMode( const unsigned short& mode ) throw() = 0;


  /// Get read mode (0 is default for normal readout )
  virtual unsigned int getReadMode() throw() = 0;


  /** Returns the version of the FPGA on the FADC board. Not really needed
   *  but provided for completeness.
   */
  virtual unsigned int getFPGAVersion() throw() = 0;


  
  /***********  Trigger settings  ***********/




  /// Sets the treshhold of the trigger (range from -0.1V to 1V) (In VME: 000 to FFF) see p 31 manual
  virtual void setTriggerThresholdDACAll(const unsigned int threshold) throw() = 0;


  /// Reads trigger threshold 
  virtual unsigned int getTriggerThresholdDACAll() throw() = 0;


  /// Sets the treshhold of the trigger (range from -0.1V to 1V) (In VME: 000 to FFF) see p 31 manual
  virtual void setTriggerThresholdDACPerChannel(const unsigned short chNb, const unsigned int threshold) throw() = 0;
  

  /// Reads trigger threshold 
  virtual unsigned int getTriggerThresholdDACPerChannel(const unsigned short chNb) throw() = 0;


  /** Loads the the trigger threshold DAC. This means the trigger threshold 
   *  has to be set first and then "applied" by loading it. Just setting it
   *  by using setTriggerThresholdDAC() will have no effect if its not 
   *  loaded afterwords by this method.
   */
  virtual void loadTriggerThresholdDAC() throw() = 0;


  /** Returns the distance between the column pointer at trigger arrival 
   *  and the last columnn (128). Used for temporal alignment of the data.
   *  TODO: Even more precision can be reached by using the VERNIERS for more 
   *  information see manual p. 14.
   */
  virtual unsigned int getTriggerRecord() throw() = 0;
  

  /// Send software trigger. 
  virtual void sendSoftwareTrigger() throw() = 0;


  /* Set the pretrig,which is the amount of time the ADC waits 
   * before the next acquisition. Minimum values depent on the sampling 
   * frequency. For 2 GHz it has to be set at least to 15000 and for 1 GHz 
   * to 75000. Its set per default to 10240.
   */
  virtual void setPretrig( const unsigned short& bits ) throw()  = 0;


  /// Returns the current pretrig.
  virtual unsigned short getPretrig() throw() = 0;


  /// Set the number of databits to be recorded AFTER a trigger signal arrives
  virtual void setPosttrig( const unsigned short& bits ) throw() = 0;


  /// Returns the current postrig
  virtual unsigned int getPosttrig() throw() = 0;


  /** Set the type of the trigger (see V1729 manual p. 34 for the an explanation
   *  of the different trigger types)
   **/
  virtual void setTriggerType( const unsigned short& type ) throw() = 0;


  /// Returns the type of the current trigger
  virtual unsigned short getTriggerType() throw() = 0;


  /* Sets the trigger channeel mask for the 4 channels. 1 allows and 0 denies
   * trigger from a channel. Each bit of the 4 bit mask corresponds to a 
   * channel. Bit 0 corresponds to channel 0 and so on.
   */
  virtual void setTriggerChannelSource( const unsigned short& mask ) throw() = 0;


  /// Returns the trigger channel mask
  virtual unsigned short getTriggerChannelSource() throw() = 0;


  

  /***********  Readout settings  ***********/


  /** Get the mode register. (0 is defualt) See manual p.31
   * for the values
   **/  
  virtual unsigned short getModeRegister() throw() = 0;
  

  /** Sets the mode register. (0 is defualt) See manual p.31 
   * for the values
   **/
  virtual void setModeRegister(const unsigned short& mode) throw() = 0;


  /* Set the number of columns to read. Per default the number
   * of columns to read is set to 128. Changing this value
   * will change the format of the data in the RAM. In
   * espacially one has to make sure, that all interesting
   * data is in the first part of the RAM matrix. Read the 
   * manual for instructions on how to do this.
   */
  virtual void setColToRead( const unsigned short& col ) throw() = 0 ;


  /// Returns the present number of columns to read.
  virtual unsigned short getColToRead() throw() = 0;


  /** Set channel mask for the readout. The device has four
   *  input channels which can be masked individually. To 
   *  activate a channel set the corresponding bit to 1, 
   *  to mask it, set the bit to 0. Keep in mind that changing
   *  the channel mask also changes the data format in the RAM
   *  of the ADC.
   **/
  virtual void setChannelMask( const unsigned short& mask ) throw() = 0 ;

  
  /// Returns the present channel mask.
  virtual unsigned short getChannelMask() throw() = 0;

  
  /** Sets the number of channels (see p 34 manual). 1 -> 
   * one channel with 10240 samples is read out; 2 -> two channels with
   * 5120 samples are read out; 4 (or any other value) -> four channels 
   * with 2560 samples are read out. 
   **/  
  virtual void setNbOfChannels(const unsigned short& nbOfChannels) throw() = 0;


  /// Returns the number of set read-out-channels
  virtual unsigned short getNbOfChannels() throw() = 0;

  
  /// Set post stop latency 
  virtual void setPostStopLatency( const unsigned short& latency ) throw() = 0;


  /// Read post stop latency
  virtual unsigned short getPostStopLatency() throw() = 0;


  /// Set post latency pretrig
  virtual void setPostLatencyPretrig( const unsigned short& latency ) throw() = 0;


  /// Get post latency pretrig
  virtual unsigned short getPostLatencyPretrig() throw() = 0;


  /** Set the RAM address pointer on the board to a certain address.
   *  By doing this certain data within the RAM can be accessed directly.
   *  For a complete readout of the RAM this is not necessary and therefore
   *  readDataInRAM() should be used for a complete readout.
   */
  virtual void setRAMAddress( const unsigned short& add ) throw() = 0;


  /// Get internal address of data in the RAM
  virtual unsigned short getRAMAddress() throw() = 0;


  /** Reads all the data stored in the RAM. The output data is TEMPORAL
   *  UNALIGNED and WITHOUT PEDESTAL CORRECTION. This steps have to be
   *  by higher level functions.
   */
  virtual std::vector< int > getAllData( const unsigned short& nbChannels) throw() = 0;


  //Get all data from the ram (via block mode)
  virtual std::vector< int > getAllDataBlockMode() throw() = 0;


  /** Reads data in the RAM at the current location of the RAM address 
   *  which is controlled by setRAMAddress(). 
   */
  virtual int  getDataAt( const unsigned short& address ) throw() = 0;





  /***********  Interrupt settings  ***********/




  /* Reset interrupt. This is done by the startAcquisition
   * command automatically (boardwise). 
   */
  virtual void releaseInterrupt() throw() = 0;


  /** Reads the interrupt register of the ADC. Usage of this 
   *  method is strongly discouraged as it is a source of
   *  noise. Better scan the bus for assertion of the interrupt
   *  line (IRQ3 for VME and SRQ for GPIB). This is method is
   *  less noisy as the device is not accessed, but provides the
   *  same information.
   */
  virtual bool readDeviceInterrupt() throw() = 0;


  /** Reads interrupt on the bus by scaning for assertion of the
   *  SRQ line in case of GPIB and IRQ3 in case of VME access.
   *  This method should be prefered to directly scaning the 
   *  interrupt register on the device as it is less noisy (see
   *  readDeviceInterrupt() or check the manual for more information).
   */ 
  virtual bool readInterrupt() throw() = 0;
 


  /***********  "high level" functions  ***********/



  ///correct Data for trigger position
  //TODO: up to now, the correction is done in the print data functions
  //now in HighLevelFunctions
  //virtual std::vector<std::vector<int> > correctData(std::vector<int> const& dataVec) = 0;


  /** print data to file (in the file the unit of time will be ns and the unit of
   * amplitude will be V
   **/
  //now in HighLevelFunctions
  //virtual void printDataToFile(std::vector<int> const& dataVec, std::string fileName = "rawData") = 0;


  //now in HighLevelFunctions
  //virtual void printDataToFile(std::vector<std::vector<int> > const& dataVec, std::string fileName = "Data") = 0;


}; // end of FADC class definition


#endif
