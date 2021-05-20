#include <QDebug>

#include "vmusb.h"

CVmUsb::CVmUsb()
{
    udev = NULL;

    const QString libName = "libxxusb"; // library was called `xx_usb.so` in the past

    lib = new QLibrary(libName);
    lib->load();

    if ( (lib) && (lib->isLoaded()) ) {
	qDebug() << lib->fileName() << "loaded";

	xxusbDevicesFind = (t_xxusbDevicesFind) lib->resolve("xxusb_devices_find");
	qDebug("xxusbDevicesFind = 0x%lX", (long int)xxusbDevicesFind);

	xxusbDeviceOpen = (t_xxusbDeviceOpen) lib->resolve("xxusb_device_open");
	qDebug("xxusbDeviceOpen = 0x%lX", (long int)xxusbDeviceOpen);

	xxusbDeviceClose = (t_xxusbDeviceClose) lib->resolve("xxusb_device_close");
	qDebug("xxusbDeviceClose = 0x%lX", (long int)xxusbDeviceClose);

	vmeRead16 = (t_vmeRead16) lib->resolve("VME_read_16");
	qDebug("vmeRead16 = 0x%lX", (long int)vmeRead16);

	vmeWrite16 = (t_vmeWrite16) lib->resolve("VME_write_16");
	qDebug("vmeWrite16 = 0x%lX", (long int)vmeWrite16);

	vmeRead32 = (t_vmeRead32) lib->resolve("VME_read_32");
	qDebug("vmeRead32 = 0x%lX", (long int)vmeRead32);

	vmeWrite32 = (t_vmeWrite32) lib->resolve("VME_write_32");
	qDebug("vmeWrite32 = 0x%lX", (long int)vmeWrite32);

	vmeBltRead32 = (t_vmeBltRead32) lib->resolve("VME_BLT_read_32");
	qDebug("vmeBltRead32 = 0x%lX", (long int)vmeBltRead32);

	if (!xxusbDevicesFind || !xxusbDeviceOpen || !xxusbDeviceClose || !vmeRead16 || !vmeWrite16 || !vmeRead32 || !vmeWrite32 || !vmeBltRead32) {
	    ErrorString = "Error while loading shared library '" + libName + "'!";
	    lib->unload();
	    delete lib;
	    lib = NULL;

	    /// ERROR!
	}

    } else {
	qDebug() << lib->errorString();
	ErrorString = lib->errorString();
	delete lib;
	lib = NULL;

	/// TODO give the errors back to VmeControl
    }
}

CVmUsb::~CVmUsb()
{
    if ((lib)) {
	lib->unload();
	delete lib;
	lib = NULL;
    }
}

/*QStringList CVmUsb::enumControllers(void)
  {
  QString controllerName;
  QStringList controllerList;
  xxusb_device_type devices[32]; // should be enough

  if ( (libxxusb) && (libxxusb->isLoaded())) {

  // Find XX_USB devices and open the first one found
  int deviceCount = xxusbDevicesFind(devices);

  for (int i = 0; i < deviceCount; i++) {
  controllerName = devices[i].SerialString;
  controllerList << controllerName;
  }
  }

  return controllerList;
  }*/

bool CVmUsb::initController(int controllerNumber)
{
    xxusb_device_type devices[32]; // should be enough

    if ((!lib)) {
	return false;
    }

    int deviceCount = xxusbDevicesFind(devices);

    // no controllers found
    if (controllerNumber >= deviceCount) {
	ErrorString = QString("No VM-USB controller # %1 found!") .arg(controllerNumber);
	return false;
    }

    // Open the device
    udev = xxusbDeviceOpen(devices[controllerNumber].usbdev);

    serialNumber = QString("%s") .arg(devices[controllerNumber].SerialString);

    return true;
}

/*bool CVmUsb::initController(QString controllerName)
  {
  return true;
  }*/

void CVmUsb::writeShort(int vmeAddress, int addressModifier, int data, int *errorCode)
{
    int result = -1;

    if ( (lib) && (udev) ) {
	result = vmeWrite16(udev, addressModifier, vmeAddress, data);
    }

    if ((errorCode)) {
	*errorCode = result;
    }
}

int CVmUsb::readShort(int vmeAddress, int addressModifier, int *errorCode)
{
    long data = 0;
    int  result = -1;

    if ( (lib) && (udev) ) {
	result = vmeRead16(udev, addressModifier, vmeAddress, &data);
    }

    if ((errorCode)) {
	*errorCode = result;
    }

    return data;
}

void CVmUsb::writeLong(int vmeAddress, int addressModifier, int data, int *errorCode)
{
    int result = -1;

    if ( (lib) && (udev) ) {
	result = vmeWrite32(udev, addressModifier, vmeAddress, data);
    }

    if ((errorCode)) {
	*errorCode = result;
    }
}

int CVmUsb::readLong(int vmeAddress, int addressModifier, int *errorCode)
{
    long data = 0;
    int  result = -1;

    if ( (lib) && (udev) ) {
	result = vmeRead32(udev, addressModifier, vmeAddress, &data);
    }

    if ((errorCode)) {
	*errorCode = result;
    }

    return data;
}

std::vector<int> CVmUsb::readBlock32( int addr, int nWords, int addrMod)
{
/* Function taken directly from Interface.h from Thorsten Krautscheid
   uses functions defined in libxxusb.h directly */

    if( udev != 0 )
    {
	return std::vector<int>();
    }

    //long tempData[nWords];
    long *tempData;
    tempData = (long *) malloc(nWords * sizeof(long));

    int result = vmeBltRead32( udev, addrMod, nWords, addr, tempData );

    std::cout << "Result: " << result << std::endl;


    std::vector<int> data;
    // NOTE: make sure & is correct here
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

    free(tempData);
    return data;
}


bool CVmUsb::closeController(void)
{
    if ( (lib) && (udev) ) {
	xxusbDeviceClose(udev);
    }

    return true;
}

QString CVmUsb::errorString(void)
{
    return ErrorString;
}

QString CVmUsb::controllerName(void)
{
    return "Wiener VM-USB";
}

QString CVmUsb::information(void)
{
    QString result;

    if (!lib) {
	result = "Not connected to Wiener XXUSB library!";
    } else {
	result = QString("Connected to VME controller %1") .arg(serialNumber);
    }

    return result;
}
