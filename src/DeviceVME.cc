/*
 * DeviceVME.cc
 *
 *  Created on: Oct 30, 2012
 *      Author: Thorsten Krautscheid
 */

#include "DeviceVME.h"

// Project
#include "InterfaceVME.h"
#include "vmusb.h"



namespace vme {





    DeviceVME::DeviceVME( CVmUsb *Controller, int baseAddr )

	:	BASE_ADDR(baseAddr),
	 	m_Controller(Controller)

{} // end of DeviceVME::DeviceVME( int baseAddr, int usbNr )





DeviceVME::~DeviceVME() {

} // end of DeviceVME::~DeviceVME()



int DeviceVME::read32( int addrOffset, int addrMod ){

  //return m_interface.get()->read32( BASE_ADDR + addrOffset, addrMod );
  return m_Controller->readLong( BASE_ADDR + addrOffset, addrMod );

} // end of void DeviceVME::read32( int addrOffset, int addrMod = 0x0D )





int DeviceVME::read16( int addrOffset, int addrMod ){

    return m_Controller->readShort( BASE_ADDR + addrOffset, addrMod );

} // end of void DeviceVME::read16( int addrOffset, int addrMod )





void DeviceVME::write32( int data, int addrOffset, int addrMod ){

    m_Controller->writeLong( BASE_ADDR + addrOffset, addrMod, data );

} // end of int DeviceVME::write32( int data, int addrOffset, int addrMod = 0x0D )





void DeviceVME::write16( int data, int addrOffset, int addrMod ){

    m_Controller->writeShort( BASE_ADDR + addrOffset, addrMod, data );

} // end of int DeviceVME::write16( int data, int addrOffset, int addrMod )





std::vector<int> DeviceVME::readBlock32( int addrOffset, int nWords, int addrMod ){

	return m_Controller->readBlock32( BASE_ADDR + addrOffset, nWords, addrMod );
}





} // end of namespace vme
