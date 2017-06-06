// vmusb.h by ISEG
// modified by Sebastian Schmidt
// added 32bit reads and write

#ifndef __VMUSB_H
#define __VMUSB_H

#include <QLibrary>
#include <QStringList>

#include "vmecontroller.h"
// originally using xxusbdll.h by ISEG
// due to conflicts with libxxusb (which refers to <usb.h>)


#ifdef __WIN32__
#include <xxusbdll.h>
#else
#include <libxxusb.h>
#endif



// C libs
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>

#ifdef Q_OS_LINUX
#define __stdcall
#endif

/* Prototypes of dynamically imported functions from xxusb.dll */

typedef short (__stdcall *t_xxusbDevicesFind)(xxusb_device_type *);
typedef usb_dev_handle* (__stdcall *t_xxusbDeviceOpen)(struct usb_device *dev);
typedef short (__stdcall *t_xxusbDeviceClose)(usb_dev_handle *hDev);
typedef short (__stdcall *t_vmeRead16)(usb_dev_handle *, short, long, long *);
typedef short (__stdcall *t_vmeWrite16)(usb_dev_handle *, short, long, long);
typedef short (__stdcall *t_vmeRead32)(usb_dev_handle *, short, long, long *);
typedef short (__stdcall *t_vmeWrite32)(usb_dev_handle *, short, long, long); 
typedef short (__stdcall *t_vmeBltRead32)(usb_dev_handle *, short, int, long, long *); 

class CVmUsb : public CVmeController
{
public:
    CVmUsb();
    ~CVmUsb();
    //QStringList enumControllers(void);

    virtual QString errorString(void);
    virtual QString controllerName(void);
    virtual QString information(void);

    virtual bool initController(int controllerNumber);
    // functions from libxxusb, which are dynamically linked into the program during runtime
    virtual void writeShort(int vmeAddress, int addressModifier, int data, int *errorCode = NULL);
    virtual int  readShort(int vmeAddress, int addressModifier, int *errorCode = NULL);
    virtual void writeLong(int vmeAddress, int addressModifier, int data, int *errorCode = NULL);
    virtual int  readLong(int vmeAddress, int addressModifier, int *errorCode = NULL);
	
    // this function is not necessary, since we only include the VME_BLT_read_32 function from
    // the library, but do not define a new function, which works on this for convenience
    // used in readBlock32
    // virtual int  vmeBlockRead(int vmeAddress, int addressModifier, int nWords, long Data[]);
    // function specific to FADC
    virtual std::vector<int> readBlock32(int vmeAddress, int nWords, int addrMod = 0x0F);
    virtual bool closeController(void);

private:

    QLibrary *lib;

    usb_dev_handle *udev;

    QString serialNumber;
    QString ErrorString;

    t_xxusbDevicesFind xxusbDevicesFind;
    t_xxusbDeviceOpen xxusbDeviceOpen;
    t_xxusbDeviceClose xxusbDeviceClose;
    t_vmeRead16 vmeRead16;
    t_vmeWrite16 vmeWrite16;
    t_vmeRead32 vmeRead32;
    t_vmeWrite32 vmeWrite32;
    // function to read blocks via VME
    t_vmeBltRead32 vmeBltRead32;
};

#endif // __VMUSB_H
