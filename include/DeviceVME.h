/*
 * DeviceVME.h
 *
 *  Created on: Oct 30, 2012
 *      Author: Thorsten Krautscheid
 */

#ifndef DEVICEVME_H_
#define DEVICEVME_H_


// STD C++
#include <memory>
#include <vector>

// project files
#include "vmusb.h"

namespace vme {
// Forward declarations (namespace vme)
    class InterfaceVME;
    class DeviceVME {

      public:
	/// C'tor
	DeviceVME( CVmUsb *Controller, int baseAddr = 0x10000 );
	/// D'tor
	virtual ~DeviceVME();

      protected:
	// --- Protected methods --- //

	int read32( int addrOffset, int addrMod = 0x0D );
	int read16( int addrOffset, int addrMod );

	void write32( int data, int addrOffset, int addrMod = 0x0D );
	void write16( int data, int addrOffset, int addrMod );

	std::vector<int> readBlock32( int addrOffset, int nWords, int addrMod = 0x0F );

	// --- Protected member variables --- //

	const int BASE_ADDR;
	
	CVmUsb *m_Controller;

	//std::auto_ptr<InterfaceVME> m_interface;
    }; // end of class DeviceVME

} // end of namespace vme

#endif // DEVICEVME_H_
