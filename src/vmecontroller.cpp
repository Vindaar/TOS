#include "vmecontroller.h"

CVmeController::CVmeController()
{
}

CVmeController::~CVmeController()
{
}

bool CVmeController::initController(int)
{
    return true;
}

bool CVmeController::closeController(void)
{
    return true;
}

void CVmeController::writeShort(int, int, int, int *)
{
}

int CVmeController::readShort(int, int, int *)
{
    return 0;
}

QString CVmeController::errorString(void)
{
    return "";
}

QString CVmeController::controllerName(void)
{
    return "";
}

QString CVmeController::information(void)
{
    return "";
}
