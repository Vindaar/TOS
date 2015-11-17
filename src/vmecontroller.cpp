#include "vmecontroller.h"

CVmeController::CVmeController()
{
}

CVmeController::~CVmeController()
{
}

bool CVmeController::initController(int controllerNumber)
{
	return true;
}

bool CVmeController::closeController(void)
{
	return true;
}

void CVmeController::writeShort(int vmeAddress, int addressModifier, int data, int *errorCode)
{
}

int CVmeController::readShort(int vmeAddress, int addressModifier, int *errorCode)
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
