/*
 * InterfaceVME.h
 *
 *  Created on: Oct 30, 2012
 *      Author: Thorsten Krautscheid
 */

#ifndef INTERFACEVME_H_
#define INTERFACEVME_H_

// Wiener USB
#ifdef __WIN32__
// TODO: Check if no inclusion of any header is actually correct for Windows.
//       Included at some other place? Or simply not an issue, since library
//       currently not used on the Windows platform?
#else
#include <libxxusb.h>
#endif

// Qt4
#include <QString>

// STD C++
#include <iostream>
#include <vector>




namespace vme {

/*
 *
 */
class InterfaceVME {

public:

	/// C'tor
        InterfaceVME( int devNr ); // former devNr=0

	/// D'tor
	virtual ~InterfaceVME();

	void write32( int data, int address, int addressModifier = 0x0D );
	void write16( int data, int address, int addressModifier );

	int read32( int address, int addressModifier = 0x0D );
	int read16( int address, int addressModifier );

	std::vector<int> readBlock32( int addr, int nWords, int addrMod = 0x0F );

	bool isIrq();

protected:

	// --- Protected methods --- //

	void init( int devNr );

	

	// --- Protected variables --- //

	QString m_serialNumber;

	const int MAX_USB_DEV;

	usb_dev_handle* m_usbDev;

	std::vector<xxusb_device_type> m_deviceVec;


}; // end of class InterfaceVME

} // namespace vme


#endif // INTERFACEVME_H_
