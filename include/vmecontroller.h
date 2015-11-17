#ifndef __VMECONTROLLER_H
#define __VMECONTROLLER_H

#include <stddef.h>

#include <QString>

class CVmeController
{
public:
	CVmeController();
	virtual ~CVmeController();

	virtual bool initController(int controllerNumber);
	virtual bool closeController(void);
	virtual QString errorString(void);
	virtual QString controllerName(void);
	virtual QString information(void);

	virtual void writeShort(int vmeAddress, int addressModifier, int data, int *errorCode = NULL);
	virtual int  readShort(int vmeAddress, int addressModifier, int *errorCode = NULL);
};

#endif // __VMECONTROLLER_H
