/*
 * InterfaceVME.cc
 *
 *  Created on: Oct 30, 2012
 *      Author: Thorsten Krautscheid
 */

#include "InterfaceVME.h"


// STD C++
#include <cstdlib>
#include <iostream>
#include <iomanip>




namespace vme {





InterfaceVME::InterfaceVME( int devNr ):MAX_USB_DEV(32),
	 	                        m_usbDev(NULL),
	 	                        m_deviceVec()
{
  xxusb_device_type dev[MAX_USB_DEV];
  int nDevices = xxusb_devices_find( dev );

  std::cout << "(InterfaceVME) Found " << nDevices << " devices" << std::endl;

  m_deviceVec.assign( &dev[0], &dev[0] + nDevices );

  if( devNr >= 0 && devNr < nDevices )
  {
    std::cout << "(InterfaceVME) Calling init for device number: " << devNr << std::endl;
    init( devNr );
    std::cout << "...done" << std::endl;
  }
  else{ 
    std::cout << "(InterfaceVME) Exit with failure!" << std::endl;
    exit( EXIT_FAILURE ); 
  }
} // end of InterfaceVME::InterfaceVME()





InterfaceVME::~InterfaceVME()
{
  if( m_usbDev > 0 )
  {
    xxusb_device_close( m_usbDev );
  }
} // end of InterfaceVME::~InterfaceVME()





void InterfaceVME::write32( int data, int address, int addressModifier )
{
  if( m_usbDev < 0 )
  {
    return;
  }

  //int result = VME_write_32( m_usbDev, addressModifier, address, data );
  VME_write_32( m_usbDev, addressModifier, address, data );
} // end of void InterfaceVME::write32( int data, int address, int addressModifier )





void InterfaceVME::write16( int data, int address, int addressModifier ){

	if( m_usbDev < 0 ){

		return;
	}

	//int result = VME_write_16( m_usbDev, addressModifier, address, data );
	VME_write_16( m_usbDev, addressModifier, address, data );

} // end of void InterfaceVME::write32( int data, int address, int addressModifier )





int InterfaceVME::read32( int address, int addressModifier )
{
  if( m_usbDev < 0 )
  {
    return -1;
  }

  long int data = 0;

  //int result = VME_read_32( m_usbDev, addressModifier, address, &data );
  VME_read_32( m_usbDev, addressModifier, address, &data );

  return data;
} // end of int InterfaceVME::read32( int address, int addressModifier )





int InterfaceVME::read16( int address, int addressModifier ){

	if( m_usbDev < 0 ){

		return -1;
	}

	long int data = 0;

	//int result = VME_read_16( m_usbDev, addressModifier, address, &data );
	VME_read_16( m_usbDev, addressModifier, address, &data );


	return data;

} // end of int InterfaceVME::read32( int address, int addressModifier )





std::vector<int> InterfaceVME::readBlock32( int addr, int nWords, int addrMod)
{
  if( m_usbDev < 0 )
  {
    return std::vector<int>();
  }

  long tempData[nWords];

  int result = VME_BLT_read_32( m_usbDev, addrMod, nWords, addr, tempData );

  std::cout << "Result: " << result << std::endl;


  std::vector<int> data;
  data.assign( &tempData[0], &tempData[0] + nWords );

  unsigned iData = 0;

  while( iData < data.size() )
  {
    int word = data[iData];

    int LSB = word & 8191;
    int MSB = (word >> 16) & 8191;

    std::cout << "3: " << std::setw(4) << MSB << "\t 2: " << std::setw(5) << LSB ;

    ++iData;

    word = data[iData];

    LSB = word & 8191;
    MSB = (word >> 16) & 8191;

    std::cout << "\t 1: " << std::setw(5) << MSB << "\t 0: " << std::setw(5) << LSB << std::endl;

    ++iData;
  }

  return data;
}





bool InterfaceVME::isIrq(){

	long int stack[14] = { 0 };

	xxusb_stack_read( m_usbDev, 0x33, &stack[0] );

	for( int iStack = 0; iStack < 14; ++iStack ){

		std::cout << "(isIrq) IRQ Stack: " << stack[iStack] << std::endl;

	}

	return false;

} // end of bool InterfaceVME::isIrg()





void InterfaceVME::init( int devNr ){

		m_usbDev = xxusb_device_open( m_deviceVec[devNr].usbdev );

		m_serialNumber = QString( m_deviceVec[devNr].SerialString );


} // end of void InterfaceVME::init( int devNr )





} // end of  namespace vme
