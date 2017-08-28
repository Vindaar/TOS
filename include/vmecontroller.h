#ifndef __VMECONTROLLER_H
#define __VMECONTROLLER_H

#include <stddef.h>

#if CYGWIN==1
#include <QBool>
#endif
#include <QString>

class CVmeController
{
public:
    CVmeController();
    virtual ~CVmeController();

    virtual bool initController(int);
    virtual bool closeController(void);
    virtual QString errorString(void);
    virtual QString controllerName(void);
    virtual QString information(void);

    virtual void writeShort(int, int, int, int * = NULL);
    virtual int  readShort(int, int, int * = NULL);
};

#endif // __VMECONTROLLER_H
