#ifndef V1729a_VME_H
#define V1729a_VME_H 1


#include "DeviceVME.h"
#include "InterfaceVME.h"


//PROJECT FILES
#include "V1729a.h"
#include "FADCConstants_V1729a.h"
#include "vmusb.h"


// STD C++
#include <cstdlib>
#include <cmath>
#include <vector>
#include <utility>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>


/** Abstrac class for the readout of of the V1729a
 *  FlashADC from CAEN. This class only defines a common 
 *  interface for the readout via VME or GPIB. 
 *  Reference to the manual allwayes refers to the V1729a 
 *  user manual by CAEN.
 *
 *  The implementations of the derived classes are not 
 *  expected to validate type and format of the input
 *  values for its methods. They are also only intended
 *  to allow access to the different registers and 
 *  commands of the V1729a ADC, the shall not provide any 
 *  higher level functions. Higher level mehtods (like 
 *  pedestal substraction, data realignment, readout 
 *  schemes and so on should be defined in a different class
 *  which uses this FADC class only to access the device.
 *
 *  For details on the implementation of the derived classes
 *  for access by VME or GPIB refer to the documentation for
 *  classes itself.
 *
 *  @Author Thorsten Krautscheid
 *  (changes by Alexander Deisting to implement the V1729a)
 **/



class V1729a_VME : public V1729a, public vme::DeviceVME{


 public:

  /// C'tor
    V1729a_VME(CVmUsb *Controller, int sAddress = 1 );


  /// D'tor
  ~V1729a_VME(){};




  /***********  General funcions  ***********/


  /** Resets the FADC and returns it to idle state. This
   *  command does not change any settings like channel
   *  mask or columns to read.
   */
  void reset() throw();


  /// Prints all parameter settings of the FADC to console and file
  void printSettings() throw();


  /// Starts the acquisition of data
  void startAcquisition() throw();


  /** Sets the readout frequency of the device ( 2 GHz = 1, 1 GHz = 2 )
   * see manual p. 31 for more values
   **/
  void setFrequency( const unsigned short& frequency ) throw();


  /// Returns the current value of frequency
  unsigned int getFrequency() throw();


  /** Changes the read mode for additional speed in the readout (see
   *  manual p. 34). Maybe a source of noise.
   */
  virtual void setReadMode( const unsigned short& mode ) throw();


  /// Get read mode (0 is default for normal readout )
  virtual unsigned int getReadMode() throw();

  
  /// retruns the FPGA Version 
  unsigned int getFPGAVersion() throw();
 


  /***********  Trigger settings  ***********/



  /** Sets the trigger-treshhold pre-load register of all the DACs to
   * the given value. (range from -1V to 1V) (In VME: 000 (0) to FFF 
   * (4095)) (see p31. manual.)
   **/
  /*
    WARNING: For some reason the setTriggerType and the setModeRegister
    command overwrites the value of the trigger threshold register
   */
  void setTriggerThresholdDACAll(const unsigned int threshold) throw();

  
  /**Get the Trigger Threshold DAC for all channels. This command only returns
   * the value of the pre-load register and not the value the DAC is set ti.
   **/
  unsigned int getTriggerThresholdDACAll() throw();
  

  /** Like the setTriggerThresholdDACAll function, but only for a given
    * channel. - Therfore chNb refers to the number of the 
    * channel one wants to set the register to the given value. 0 = channel 1,
    * 1 = channel 2, ....
    **/
  /*
    WARNING: See setTriggerThresholdDACAll & also this command works like the
    setTriggerThresholdDACAll command - till now it's not possible to set the 
    channels individually
   */
  void setTriggerThresholdDACPerChannel(const unsigned short chNb, const unsigned int threshold) throw();

 
  /** Reads trigger threshold - chNb refers to the number of the 
   * channel one wants to get the threshold off. 0 = channel 1,
   * 1 = channel 2, ....
   **/
  unsigned int getTriggerThresholdDACPerChannel(const unsigned short chNb) throw();


  /**loads the preset value (functions above) to the trigger. Then it sets
   * the values of the registers to zero. (at least this seems to be the case)
   **/
  void loadTriggerThresholdDAC() throw();


  /** Returns the distance of the column with the
   * last column pointer to the last column (128).
   * (manual p.33)
   **/
  unsigned int getTriggerRecord() throw();


  /// Send software trigger. 
  void sendSoftwareTrigger() throw();

 
  void setPretrig( const unsigned short& bits ) throw();


  /// Returns the current pretrig.
  unsigned short getPretrig() throw();


  /// Set the number of databits to be recorded AFTER a trigger signal arrives
  void setPosttrig( const unsigned short& bits ) throw();


  /// Returns the current postrig (manual p. 32)
  unsigned int getPosttrig() throw();

  
  void setTriggerType( const unsigned short& type ) throw();


  /// Returns the type of the current trigger
  unsigned short getTriggerType() throw();

  /// Set on which channel one wants to trigger
  void setTriggerChannelSource( const unsigned short& mask ) throw();


  /// Returns the trigger channel mask
  unsigned short getTriggerChannelSource() throw();

  

  /***********  Readout settings  ***********/



  /** Get the mode register. (0 is defualt) See manual p.31 
   * for the values
   **/
  unsigned short getModeRegister() throw();
  
  
  /** Sets the mode register. (0 is defualt) See manual p.31
   * for the values
   **/
  void setModeRegister(const unsigned short& mode) throw();


  void setColToRead( const unsigned short& col ) throw();


  /// Returns the present number of columns to read.
  unsigned short getColToRead() throw();


  void setChannelMask( const unsigned short& mask ) throw();

  
  /// Returns the present channel mask.
  unsigned short getChannelMask() throw();


  /** Sets the number of channels (see p 34 manual). 1 -> 
   * one channel with 10240 samples is read out; 2 -> two channels with
   * 5120 samples are read out; 4 (or any other value) -> four channels 
   * with 2560 samples are read out. 
   **/  
  void setNbOfChannels(const unsigned short& nbOfChannels) throw();


  /// Returns the number of set read-out-channels
  unsigned short getNbOfChannels() throw();


  /// Set post stop latency 
  void setPostStopLatency( const unsigned short& latency ) throw();


  /// Read post stop latency
  unsigned short getPostStopLatency() throw();


  /// Set post latency pretrig
  void setPostLatencyPretrig( const unsigned short& latency ) throw();


  /// Get post latency pretrig
  unsigned short getPostLatencyPretrig() throw();


  void setRAMAddress( const unsigned short& add ) throw();


  /// Get internal address of data in the RAM
  unsigned short getRAMAddress() throw();


  //Get all data from the ram
  std::vector< int > getAllData( const unsigned short& nbChannels) throw();


  //Get all data from the ram (via block mode)
  std::vector< int > getAllDataBlockMode() throw();


  int  getDataAt( const unsigned short& address = 0 ) throw();



  /***********  Interrupt settings  ***********/


  
  /* Clears the interrupt register.  The value in the register is set to
   * 1 if a trigger arrived. The register will also be reset after a 
   * the start of a data acq (maunal p.30)
   **/  
  void releaseInterrupt() throw();


  bool readDeviceInterrupt() throw();


  bool readInterrupt() throw();



  /***********  "high level" functions  ***********/



  ///correct Data for trigger position
  //TODO: up to now, the correction is done in the print data functions
  std::vector<std::vector<int> > correctData(std::vector<int> const& dataVec);


  /**print data to file (in the file the unit of time will be ns and the unit of
   *amplitude will be V
   **/
  void printDataToFile(std::vector<int> const& dataVec, std::string fileName = "rawData");


  void printDataToFile(std::vector<std::vector<int> > const& dataVec, std::string fileName = "Data");



 protected:



}; // end of FADC class definition


#endif
