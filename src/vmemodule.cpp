#include <QtDebug>

#include "vmemodule.h"

typedef union {
    long    l;
    float   f;
    uint16_t  w[2];
    uint8_t   b[4];
} TFloatWord;

CVmeModule::CVmeModule(CVmeController *vmeController, int baseAddress)
{
    // TODO: check if VME controller is initalised or do it

    VmeController = vmeController;

    DeviceClass = 0;
    VendorID    = 0;
    BaseAddress = baseAddress;

    ChannelNumber = -1;

    vfUnit      = vf_volt;
    vfPrecision = 1;
    cfUnit      = cf_milliamp;
    cfPrecision = 3;
}

CVmeModule::~CVmeModule()
{


    // TODO: close VME controller
}

uint16_t CVmeModule::readShort(int address)
{
    return VmeController->readShort(address, ADDRESS_MODIFIER);
}

void CVmeModule::writeShort(int address, uint16_t data)
{
    VmeController->writeShort(address, ADDRESS_MODIFIER, data);
}

uint16_t CVmeModule::readShortBitfield(int address)
{
    return readShort(address);
}

void CVmeModule::writeShortBitfield(int address, uint16_t data)
{
    writeShort(address, data);
}

float CVmeModule::readFloat(int address)
{
    TFloatWord fw = { 0 };

#if (Q_BYTE_ORDER == Q_LITTLE_ENDIAN)
    fw.w[1] = readShort(address + 0); // swap words
    fw.w[0] = readShort(address + 2);
#else
    fw.w[0] = readShort(address + 0);
    fw.w[1] = readShort(address + 2);
#endif

    return fw.f;
}

void CVmeModule::writeFloat(int address, float data)
{
    TFloatWord fw;

    fw.f = data;

#if (Q_BYTE_ORDER == Q_LITTLE_ENDIAN)
    writeShort(address + 0, fw.w[1]);
    writeShort(address + 2, fw.w[0]);
#else
    writeShort(address + 0, fw.w[0]);
    writeShort(address + 2, fw.w[1]);
#endif

}

uint32_t CVmeModule::readLong(int address)
{
    TFloatWord fw = { 0 };

#if (Q_BYTE_ORDER == Q_LITTLE_ENDIAN)
    fw.w[1] = readShort(address + 0); // swap words
    fw.w[0] = readShort(address + 2);
#else
    fw.w[0] = readShort(address + 0);
    fw.w[1] = readShort(address + 2);
#endif

    return fw.l;
}

void CVmeModule::writeLong(int address, uint32_t data)
{
    TFloatWord fw;

    fw.l = data;

#if (Q_BYTE_ORDER == Q_LITTLE_ENDIAN)
    writeShort(address + 0, fw.w[1]);
    writeShort(address + 2, fw.w[0]);
#else
    writeShort(address + 0, fw.w[0]);
    writeShort(address + 2, fw.w[1]);
#endif

}

uint32_t CVmeModule::readLongBitfield(int address)
{
    return readLong(address);
}

void CVmeModule::writeLongBitfield(int address, uint32_t data)
{
    writeLong(address, data);
}

/**
 * Mirrors the Bit positions in a 16 bit word.
 */
uint16_t CVmeModule::mirrorShort(uint16_t data)
{
    uint16_t result = 0;

    for (int i = 16; i; i--) {
	result >>= 1;
	result |= data & 0x8000;
	data <<= 1;
    }

    return result;
}

/**
 * Mirrors the Bit positions in a 32 bit word.
 */
uint32_t CVmeModule::mirrorLong(uint32_t data)
{
    uint32_t result = 0;

    for (int i = 32; i; i--) {
	result >>= 1;
	result |= data & 0x80000000;
	data <<= 1;
    }

    return result;
}


//=============================================================================
// Module Commands
//=============================================================================

int CVmeModule::GetModuleDeviceClass(void)
{
    uint32_t tmp;

    if (VendorID == ISEG_VENDOR_ID) { // iseg
	tmp = readLong(BaseAddress + VdsChannelNumberOFFSET);
	DeviceClass   = (uint16_t)tmp;

	if (DeviceClass == V12C0) {
	    ChannelNumber = 0;
	    for (int chn = 0; chn < 16; chn++)
		if (tmp & (1 << ((uint32_t)chn + 16)))
		    ChannelNumber++;

	    ChannelOFFSET[0] = VhsChannel00OFFSET;
	    ChannelOFFSET[1] = VhsChannel01OFFSET;
	    ChannelOFFSET[2] = VhsChannel02OFFSET;
	    ChannelOFFSET[3] = VhsChannel03OFFSET;
	    ChannelOFFSET[4] = VhsChannel04OFFSET;
	    ChannelOFFSET[5] = VhsChannel05OFFSET;
	    ChannelOFFSET[6] = VhsChannel06OFFSET;
	    ChannelOFFSET[7] = VhsChannel07OFFSET;
	    ChannelOFFSET[8] = VhsChannel08OFFSET;
	    ChannelOFFSET[9] = VhsChannel09OFFSET;
	    ChannelOFFSET[10] = VhsChannel10OFFSET;
	    ChannelOFFSET[11] = VhsChannel11OFFSET;

	    FlexGroupOFFSET[0]=VhsFlexGroup00OFFSET;
	    FlexGroupOFFSET[1]=VhsFlexGroup01OFFSET;
	    FlexGroupOFFSET[2]=VhsFlexGroup02OFFSET;
	    FlexGroupOFFSET[3]=VhsFlexGroup03OFFSET;
	    FlexGroupOFFSET[4]=VhsFlexGroup04OFFSET;
	    FlexGroupOFFSET[5]=VhsFlexGroup05OFFSET;
	    FlexGroupOFFSET[6]=VhsFlexGroup06OFFSET;
	    FlexGroupOFFSET[7]=VhsFlexGroup07OFFSET;
	    FlexGroupOFFSET[8]=VhsFlexGroup08OFFSET;
	    FlexGroupOFFSET[9]=VhsFlexGroup09OFFSET;
	    FlexGroupOFFSET[10]=VhsFlexGroup10OFFSET;
	    FlexGroupOFFSET[11]=VhsFlexGroup11OFFSET;
	    FlexGroupOFFSET[12]=VhsFlexGroup12OFFSET;
	    FlexGroupOFFSET[13]=VhsFlexGroup13OFFSET;
	    FlexGroupOFFSET[14]=VhsFlexGroup14OFFSET;
	    FlexGroupOFFSET[15]=VhsFlexGroup15OFFSET;
	    FlexGroupOFFSET[16]=VhsFlexGroup16OFFSET;
	    FlexGroupOFFSET[17]=VhsFlexGroup17OFFSET;
	    FlexGroupOFFSET[18]=VhsFlexGroup18OFFSET;
	    FlexGroupOFFSET[19]=VhsFlexGroup19OFFSET;
	    FlexGroupOFFSET[20]=VhsFlexGroup20OFFSET;
	    FlexGroupOFFSET[21]=VhsFlexGroup21OFFSET;
	    FlexGroupOFFSET[22]=VhsFlexGroup22OFFSET;
	    FlexGroupOFFSET[23]=VhsFlexGroup23OFFSET;
	    FlexGroupOFFSET[24]=VhsFlexGroup24OFFSET;
	    FlexGroupOFFSET[25]=VhsFlexGroup25OFFSET;
	    FlexGroupOFFSET[26]=VhsFlexGroup26OFFSET;
	    FlexGroupOFFSET[27]=VhsFlexGroup27OFFSET;
	    FlexGroupOFFSET[28]=VhsFlexGroup28OFFSET;
	    FlexGroupOFFSET[29]=VhsFlexGroup29OFFSET;
	    FlexGroupOFFSET[30]=VhsFlexGroup30OFFSET;
	    FlexGroupOFFSET[31]=VhsFlexGroup31OFFSET;

	    VoltageMaxSetOFFSET[0]=VhsVoltageMaxSet00OFFSET;
	    VoltageMaxSetOFFSET[1]=VhsVoltageMaxSet01OFFSET;
	    VoltageMaxSetOFFSET[2]=VhsVoltageMaxSet02OFFSET;
	    VoltageMaxSetOFFSET[3]=VhsVoltageMaxSet03OFFSET;
	    VoltageMaxSetOFFSET[4]=VhsVoltageMaxSet04OFFSET;
	    VoltageMaxSetOFFSET[5]=VhsVoltageMaxSet05OFFSET;
	    VoltageMaxSetOFFSET[6]=VhsVoltageMaxSet06OFFSET;
	    VoltageMaxSetOFFSET[7]=VhsVoltageMaxSet07OFFSET;
	    VoltageMaxSetOFFSET[8]=VhsVoltageMaxSet08OFFSET;
	    VoltageMaxSetOFFSET[9]=VhsVoltageMaxSet09OFFSET;
	    VoltageMaxSetOFFSET[10]=VhsVoltageMaxSet10OFFSET;
	    VoltageMaxSetOFFSET[11]=VhsVoltageMaxSet11OFFSET;

	    CurrentMaxSetOFFSET[0]=VhsCurrentMaxSet00OFFSET;
	    CurrentMaxSetOFFSET[1]=VhsCurrentMaxSet01OFFSET;
	    CurrentMaxSetOFFSET[2]=VhsCurrentMaxSet02OFFSET;
	    CurrentMaxSetOFFSET[3]=VhsCurrentMaxSet03OFFSET;
	    CurrentMaxSetOFFSET[4]=VhsCurrentMaxSet04OFFSET;
	    CurrentMaxSetOFFSET[5]=VhsCurrentMaxSet05OFFSET;
	    CurrentMaxSetOFFSET[6]=VhsCurrentMaxSet06OFFSET;
	    CurrentMaxSetOFFSET[7]=VhsCurrentMaxSet07OFFSET;
	    CurrentMaxSetOFFSET[8]=VhsCurrentMaxSet08OFFSET;
	    CurrentMaxSetOFFSET[9]=VhsCurrentMaxSet09OFFSET;
	    CurrentMaxSetOFFSET[10]=VhsCurrentMaxSet10OFFSET;
	    CurrentMaxSetOFFSET[11]=VhsCurrentMaxSet11OFFSET;

	} else if (DeviceClass == V24D0) {
	    ChannelNumber = tmp >> 16;

	    ChannelOFFSET[0] = VdsChannel00OFFSET;
	    ChannelOFFSET[1] = VdsChannel01OFFSET;
	    ChannelOFFSET[2] = VdsChannel02OFFSET;
	    ChannelOFFSET[3] = VdsChannel03OFFSET;
	    ChannelOFFSET[4] = VdsChannel04OFFSET;
	    ChannelOFFSET[5] = VdsChannel05OFFSET;
	    ChannelOFFSET[6] = VdsChannel06OFFSET;
	    ChannelOFFSET[7] = VdsChannel07OFFSET;
	    ChannelOFFSET[8] = VdsChannel08OFFSET;
	    ChannelOFFSET[9] = VdsChannel09OFFSET;
	    ChannelOFFSET[10] = VdsChannel10OFFSET;
	    ChannelOFFSET[11] = VdsChannel11OFFSET;
	    ChannelOFFSET[12] = VdsChannel12OFFSET;
	    ChannelOFFSET[13] = VdsChannel13OFFSET;
	    ChannelOFFSET[14] = VdsChannel14OFFSET;
	    ChannelOFFSET[15] = VdsChannel15OFFSET;
	    ChannelOFFSET[16] = VdsChannel16OFFSET;
	    ChannelOFFSET[17] = VdsChannel17OFFSET;
	    ChannelOFFSET[18] = VdsChannel18OFFSET;
	    ChannelOFFSET[19] = VdsChannel19OFFSET;
	    ChannelOFFSET[20] = VdsChannel20OFFSET;
	    ChannelOFFSET[21] = VdsChannel21OFFSET;
	    ChannelOFFSET[22] = VdsChannel22OFFSET;
	    ChannelOFFSET[23] = VdsChannel23OFFSET;

	    FlexGroupOFFSET[0]=VdsFlexGroup00OFFSET;
	    FlexGroupOFFSET[1]=VdsFlexGroup01OFFSET;
	    FlexGroupOFFSET[2]=VdsFlexGroup02OFFSET;
	    FlexGroupOFFSET[3]=VdsFlexGroup03OFFSET;
	    FlexGroupOFFSET[4]=VdsFlexGroup04OFFSET;
	    FlexGroupOFFSET[5]=VdsFlexGroup05OFFSET;
	    FlexGroupOFFSET[6]=VdsFlexGroup06OFFSET;
	    FlexGroupOFFSET[7]=VdsFlexGroup07OFFSET;
	    FlexGroupOFFSET[8]=VdsFlexGroup08OFFSET;
	    FlexGroupOFFSET[9]=VdsFlexGroup09OFFSET;
	    FlexGroupOFFSET[10]=VdsFlexGroup10OFFSET;
	    FlexGroupOFFSET[11]=VdsFlexGroup11OFFSET;
	    FlexGroupOFFSET[12]=VdsFlexGroup12OFFSET;
	    FlexGroupOFFSET[13]=VdsFlexGroup13OFFSET;
	    FlexGroupOFFSET[14]=VdsFlexGroup14OFFSET;
	    FlexGroupOFFSET[15]=VdsFlexGroup15OFFSET;
	    FlexGroupOFFSET[16]=VdsFlexGroup16OFFSET;
	    FlexGroupOFFSET[17]=VdsFlexGroup17OFFSET;
	    FlexGroupOFFSET[18]=VdsFlexGroup18OFFSET;
	    FlexGroupOFFSET[19]=VdsFlexGroup19OFFSET;
	    FlexGroupOFFSET[20]=VdsFlexGroup20OFFSET;
	    FlexGroupOFFSET[21]=VdsFlexGroup21OFFSET;
	    FlexGroupOFFSET[22]=VdsFlexGroup22OFFSET;
	    FlexGroupOFFSET[23]=VdsFlexGroup23OFFSET;
	    FlexGroupOFFSET[24]=VdsFlexGroup24OFFSET;
	    FlexGroupOFFSET[25]=VdsFlexGroup25OFFSET;
	    FlexGroupOFFSET[26]=VdsFlexGroup26OFFSET;
	    FlexGroupOFFSET[27]=VdsFlexGroup27OFFSET;
	    FlexGroupOFFSET[28]=VdsFlexGroup28OFFSET;
	    FlexGroupOFFSET[29]=VdsFlexGroup29OFFSET;
	    FlexGroupOFFSET[30]=VdsFlexGroup30OFFSET;
	    FlexGroupOFFSET[31]=VdsFlexGroup31OFFSET;

	    VoltageMaxSetOFFSET[0]=VdsVoltageMaxSet00OFFSET;
	    VoltageMaxSetOFFSET[1]=VdsVoltageMaxSet01OFFSET;
	    VoltageMaxSetOFFSET[2]=VdsVoltageMaxSet02OFFSET;
	    VoltageMaxSetOFFSET[3]=VdsVoltageMaxSet03OFFSET;
	    VoltageMaxSetOFFSET[4]=VdsVoltageMaxSet04OFFSET;
	    VoltageMaxSetOFFSET[5]=VdsVoltageMaxSet05OFFSET;
	    VoltageMaxSetOFFSET[6]=VdsVoltageMaxSet06OFFSET;
	    VoltageMaxSetOFFSET[7]=VdsVoltageMaxSet07OFFSET;
	    VoltageMaxSetOFFSET[8]=VdsVoltageMaxSet08OFFSET;
	    VoltageMaxSetOFFSET[9]=VdsVoltageMaxSet09OFFSET;
	    VoltageMaxSetOFFSET[10]=VdsVoltageMaxSet10OFFSET;
	    VoltageMaxSetOFFSET[11]=VdsVoltageMaxSet11OFFSET;
	    VoltageMaxSetOFFSET[12]=VdsVoltageMaxSet12OFFSET;
	    VoltageMaxSetOFFSET[13]=VdsVoltageMaxSet13OFFSET;
	    VoltageMaxSetOFFSET[14]=VdsVoltageMaxSet14OFFSET;
	    VoltageMaxSetOFFSET[15]=VdsVoltageMaxSet15OFFSET;
	    VoltageMaxSetOFFSET[16]=VdsVoltageMaxSet16OFFSET;
	    VoltageMaxSetOFFSET[17]=VdsVoltageMaxSet17OFFSET;
	    VoltageMaxSetOFFSET[18]=VdsVoltageMaxSet18OFFSET;
	    VoltageMaxSetOFFSET[19]=VdsVoltageMaxSet19OFFSET;
	    VoltageMaxSetOFFSET[20]=VdsVoltageMaxSet20OFFSET;
	    VoltageMaxSetOFFSET[21]=VdsVoltageMaxSet21OFFSET;
	    VoltageMaxSetOFFSET[22]=VdsVoltageMaxSet22OFFSET;
	    VoltageMaxSetOFFSET[23]=VdsVoltageMaxSet23OFFSET;

	    CurrentMaxSetOFFSET[0]=VdsCurrentMaxSet00OFFSET;
	    CurrentMaxSetOFFSET[1]=VdsCurrentMaxSet01OFFSET;
	    CurrentMaxSetOFFSET[2]=VdsCurrentMaxSet02OFFSET;
	    CurrentMaxSetOFFSET[3]=VdsCurrentMaxSet03OFFSET;
	    CurrentMaxSetOFFSET[4]=VdsCurrentMaxSet04OFFSET;
	    CurrentMaxSetOFFSET[5]=VdsCurrentMaxSet05OFFSET;
	    CurrentMaxSetOFFSET[6]=VdsCurrentMaxSet06OFFSET;
	    CurrentMaxSetOFFSET[7]=VdsCurrentMaxSet07OFFSET;
	    CurrentMaxSetOFFSET[8]=VdsCurrentMaxSet08OFFSET;
	    CurrentMaxSetOFFSET[9]=VdsCurrentMaxSet09OFFSET;
	    CurrentMaxSetOFFSET[10]=VdsCurrentMaxSet10OFFSET;
	    CurrentMaxSetOFFSET[11]=VdsCurrentMaxSet11OFFSET;
	    CurrentMaxSetOFFSET[12]=VdsCurrentMaxSet12OFFSET;
	    CurrentMaxSetOFFSET[13]=VdsCurrentMaxSet13OFFSET;
	    CurrentMaxSetOFFSET[14]=VdsCurrentMaxSet14OFFSET;
	    CurrentMaxSetOFFSET[15]=VdsCurrentMaxSet15OFFSET;
	    CurrentMaxSetOFFSET[16]=VdsCurrentMaxSet16OFFSET;
	    CurrentMaxSetOFFSET[17]=VdsCurrentMaxSet17OFFSET;
	    CurrentMaxSetOFFSET[18]=VdsCurrentMaxSet18OFFSET;
	    CurrentMaxSetOFFSET[19]=VdsCurrentMaxSet19OFFSET;
	    CurrentMaxSetOFFSET[20]=VdsCurrentMaxSet20OFFSET;
	    CurrentMaxSetOFFSET[21]=VdsCurrentMaxSet21OFFSET;
	    CurrentMaxSetOFFSET[22]=VdsCurrentMaxSet22OFFSET;
	    CurrentMaxSetOFFSET[23]=VdsCurrentMaxSet23OFFSET;
	}

	return DeviceClass;

    } else {
	VendorID = readLong(BaseAddress + VhsVendorIdOFFSET);
	return 0;
    }
}

uint32_t CVmeModule::GetModulePlacedChannels(void)
{
    if (DeviceClass == V12C0) {
	return readShort(BaseAddress + VhsPlacedChannelsOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLong(BaseAddress + VdsPlacedChannelOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

int CVmeModule::GetModuleSerialNumber(void)
{
    if (DeviceClass == V12C0) {
	return readLong(BaseAddress + VhsSerialNumberOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLong(BaseAddress + VdsSerialNumberOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

uint32_t CVmeModule::GetModuleFirmwareRelease(void)
{
    if (DeviceClass == V12C0) {
	return readLong(BaseAddress + VhsFirmwareReleaseOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLong(BaseAddress + VdsFirmwareReleaseOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

float CVmeModule::GetModuleTemperature(void)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + VhsTemperatureOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + VdsTemperatureOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

float CVmeModule::GetModuleSupplyP5(void)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + VhsSupplyP5OFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + VdsSupplyP5OFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

float CVmeModule::GetModuleSupplyP12(void)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + VhsSupplyP12OFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + VdsSupplyP12OFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

float CVmeModule::GetModuleSupplyN12(void)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + VhsSupplyN12OFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + VdsSupplyN12OFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

uint32_t CVmeModule::GetModuleAdcSamplesPerSecond(void)
{
    if (DeviceClass == V12C0) {
	return readShort(BaseAddress + VhsADCSamplesPerSecondOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLong(BaseAddress + VdsADCSamplesPerSecondOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetModuleAdcSamplesPerSecond(uint32_t sps)
{
    if (DeviceClass == V12C0) {
	return writeShort(BaseAddress + VhsADCSamplesPerSecondOFFSET, sps);
    } else if (DeviceClass == V24D0) {
	return writeLong(BaseAddress + VdsADCSamplesPerSecondOFFSET, sps);
    } else {
	GetModuleDeviceClass();
    }
}

uint32_t CVmeModule::GetModuleDigitalFilter(void)
{
    if (DeviceClass == V12C0) {
	return readShort(BaseAddress + VhsDigitalFilterOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLong(BaseAddress + VdsDigitalFilterOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetModuleDigitalFilter(uint32_t filter)
{
    if (DeviceClass == V12C0) {
	return writeShort(BaseAddress + VhsDigitalFilterOFFSET, filter);
    } else if (DeviceClass == V24D0) {
	return writeLong(BaseAddress + VdsDigitalFilterOFFSET, filter);
    } else {
	GetModuleDeviceClass();
    }
}

float CVmeModule::GetModuleVoltageLimit(void)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + VhsVoltageMaxOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + VdsVoltageMaxOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

float CVmeModule::GetModuleCurrentLimit(void)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + VhsCurrentMaxOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + VdsCurrentMaxOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}


uint32_t CVmeModule::GetModuleStatus(void)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + VhsModuleStatusOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + VdsModuleStatusOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

uint32_t CVmeModule::GetModuleEventStatus(void)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + VhsModuleEventStatusOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + VdsModuleEventStatusOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetModuleEventStatus(uint32_t status)
{
    if (DeviceClass == V12C0) {
	writeShortBitfield(BaseAddress + VhsModuleEventStatusOFFSET, status);
    } else if (DeviceClass == V24D0) {
	writeLongBitfield(BaseAddress + VdsModuleEventStatusOFFSET, status);
    } else {
	GetModuleDeviceClass();
    }
}

uint32_t CVmeModule::GetModuleEventMask(void)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + VhsModuleEventMaskOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + VdsModuleEventMaskOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetModuleEventMask(uint32_t mask)
{
    if (DeviceClass == V12C0) {
	writeShortBitfield(BaseAddress + VhsModuleEventMaskOFFSET, mask);
    } else if (DeviceClass == V24D0) {
	writeLongBitfield(BaseAddress + VdsModuleEventMaskOFFSET, mask);
    } else {
	GetModuleDeviceClass();
    }
}

uint32_t CVmeModule::GetModuleControl(void)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + VhsModuleControlOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + VdsModuleControlOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetModuleControl(uint32_t control)
{
    if (DeviceClass == V12C0) {
	writeShortBitfield(BaseAddress + VhsModuleControlOFFSET, control);
    } else if (DeviceClass == V24D0) {
	writeLongBitfield(BaseAddress + VdsModuleControlOFFSET, control);
    } else {
	GetModuleDeviceClass();
    }
}

float CVmeModule::GetModuleVoltageRampSpeed(void)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + VhsVoltageRampSpeedOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + VdsVoltageRampSpeedOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetModuleVoltageRampSpeed(float vramp)
{
    if (DeviceClass == V12C0) {
	return writeFloat(BaseAddress + VhsVoltageRampSpeedOFFSET, vramp);
    } else if (DeviceClass == V24D0) {
	return writeFloat(BaseAddress + VdsVoltageRampSpeedOFFSET, vramp);
    } else {
	GetModuleDeviceClass();
    }
}

float CVmeModule::GetModuleCurrentRampSpeed(void)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + VhsCurrentRampSpeedOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + VdsCurrentRampSpeedOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetModuleCurrentRampSpeed(float iramp)
{
    if (DeviceClass == V12C0) {
	return writeFloat(BaseAddress + VhsCurrentRampSpeedOFFSET, iramp);
    } else if (DeviceClass == V24D0) {
	return writeFloat(BaseAddress + VdsCurrentRampSpeedOFFSET, iramp);
    } else {
	GetModuleDeviceClass();
    }
}

bool CVmeModule::GetModuleKillEnable(void)
{
    ModuleControlSTRUCT control;

    control.Word = GetModuleControl();
    return control.Bit.SetKillEnable;
}

void CVmeModule::SetModuleKillEnable(bool enable)
{
    ModuleControlSTRUCT control;

    control.Word = GetModuleControl();
    control.Bit.SetKillEnable = enable;

    SetModuleControl(control.Word);
}

void CVmeModule::SetAllChannelsOn(void)
{
    if (DeviceClass == V12C0) {
	writeLong(BaseAddress + VhsSetOnOffAllChannelsOFFSET, 1);
    } else if (DeviceClass == V24D0) {
	writeLong(BaseAddress + VdsSetOnOffAllChannelsOFFSET, 1);
    } else {
	GetModuleDeviceClass();
    }
}

void CVmeModule::SetAllChannelsOff(void)
{
    if (DeviceClass == V12C0) {
	writeLong(BaseAddress + VhsSetOnOffAllChannelsOFFSET, 0);
    } else if (DeviceClass == V24D0) {
	writeLong(BaseAddress + VdsSetOnOffAllChannelsOFFSET, 0);
    } else {
	GetModuleDeviceClass();
    }
}

void CVmeModule::SetAllChannelsVoltageSet(float vset)
{
    if (DeviceClass == V12C0) {
	writeFloat(BaseAddress + VhsSetVoltageAllChannelsOFFSET, vset);
    } else if (DeviceClass == V24D0) {
	writeFloat(BaseAddress + VdsSetVoltageAllChannelsOFFSET, vset);
    } else {
	GetModuleDeviceClass();
    }
}

void CVmeModule::SetAllChannelsCurrentSet(float iset)
{
    if (DeviceClass == V12C0) {
	writeFloat(BaseAddress + VhsSetCurrentAllChannelsOFFSET, iset);
    } else if (DeviceClass == V24D0) {
	writeFloat(BaseAddress + VdsSetCurrentAllChannelsOFFSET, iset);
    } else {
	GetModuleDeviceClass();
    }
}

uint32_t CVmeModule::GetModuleRestartTime(void)
{
    if (DeviceClass == V12C0) {
	return readShort(BaseAddress + VhsRestartTimeOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLong(BaseAddress + VdsRestartTimeOFFSET);
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

void CVmeModule::SetModuleRestartTime(uint32_t restartTime)
{
    if (DeviceClass == V12C0) {
	writeShort(BaseAddress + VhsRestartTimeOFFSET, restartTime);
    } else if (DeviceClass == V24D0) {
	writeLong(BaseAddress + VdsRestartTimeOFFSET, restartTime);
    } else {
	GetModuleDeviceClass();
    }
}

uint32_t CVmeModule::GetModuleEventChannelStatus(void)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + VhsModuleEventChannelStatusOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + VdsModuleEventChannelStatusOFFSET);
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

void CVmeModule::ClearModuleEventChannelStatus(void)
{
    if (DeviceClass == V12C0) {
	writeShort(BaseAddress + VhsModuleEventChannelStatusOFFSET, -1);
    } else if (DeviceClass == V24D0) {
	writeLong(BaseAddress + VdsModuleEventChannelStatusOFFSET, -1);
    } else {
	GetModuleDeviceClass();
    }
}

uint32_t CVmeModule::GetModuleEventChannelMask(void)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + VhsModuleEventChannelMaskOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + VdsModuleEventChannelMaskOFFSET);
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

void CVmeModule::SetModuleEventChannelMask(uint32_t mask)
{
    if (DeviceClass == V12C0) {
	writeShortBitfield(BaseAddress + VhsModuleEventChannelMaskOFFSET, mask);
    } else if (DeviceClass == V24D0) {
	writeLongBitfield(BaseAddress + VdsModuleEventChannelMaskOFFSET, mask);
    } else {
	GetModuleDeviceClass();
    }
}

uint32_t CVmeModule::GetModuleEventGroupStatus(void)
{
    if (DeviceClass == V12C0) {
	return readLongBitfield(BaseAddress + VhsModuleEventGroupStatusOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + VdsModuleEventGroupStatusOFFSET);
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

uint32_t CVmeModule::GetModuleEventGroupMask(void)
{
    if (DeviceClass == V12C0) {
	return readLongBitfield(BaseAddress + VhsModuleEventGroupMaskOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + VdsModuleEventGroupMaskOFFSET);
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

void CVmeModule::SetModuleEventGroupMask(uint32_t mask)
{
    if (DeviceClass == V12C0) {
	writeLongBitfield(BaseAddress + VhsModuleEventGroupMaskOFFSET, mask);
    } else if (DeviceClass == V24D0) {
	writeLongBitfield(BaseAddress + VdsModuleEventGroupMaskOFFSET, mask);
    } else {
	GetModuleDeviceClass();
    }
}

void CVmeModule::ClearModuleEventGroupStatus(void)
{
    if (DeviceClass == V12C0) {
	writeLong(BaseAddress + VhsModuleEventGroupStatusOFFSET, -1);
    } else if (DeviceClass == V24D0) {
	writeLong(BaseAddress + VdsModuleEventGroupStatusOFFSET, -1);
    } else {
	GetModuleDeviceClass();
    }
}

void CVmeModule::DoClear(void)
{
    ModuleControlSTRUCT control;

    control.Word = GetModuleControl();
    control.Bit.DoClear = 1;
    SetModuleControl(control.Word);
}

void CVmeModule::SetModuleBaseAddress(uint16_t ba)
{
    BaseAddress = ba;
    VendorID    = 0; // force re-scan of module at next access
}

//=============================================================================
// Special Commands
//=============================================================================

void CVmeModule::SetModuleSpecialControlCommand(uint32_t command)
{
    if (DeviceClass == V12C0) {
	writeShort(BaseAddress + VhsSpecialControlCommandOFFSET, command);
    } else if (DeviceClass == V24D0) {
	writeLong(BaseAddress + VdsSpecialControlCommandOFFSET, command);
    } else {
	GetModuleDeviceClass();
    }

    if (
	(command == SPECIALCONTROL_COMMAND_ENDSPECIAL) ||
	(command == SPECIALCONTROL_COMMAND_RESTART)
	) {
	VendorID = 0; // Force re-scan of all module data
    }

}

uint32_t CVmeModule::GetModuleSpecialControlCommand(void)
{
    if (DeviceClass == V12C0) {
	return readShort(BaseAddress + VhsSpecialControlCommandOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLong(BaseAddress + VdsSpecialControlCommandOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

uint32_t CVmeModule::GetModuleSpecialControlStatus(void)
{
    if (DeviceClass == V12C0) {
	return readShort(BaseAddress + VhsSpecialControlStatusOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLong(BaseAddress + VdsSpecialControlStatusOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SendHexLine(QByteArray record)
{
    int    i;
    char   buf[10];
    uint32_t val;

    if (DeviceClass == V24D0) {

	while ( (record.length() % 4) != 0) {
	    record += '0';
	}

	for (i = 0; i < record.length(); i += 4) {
	    strcpy(buf, record.mid(i, 4).constData());
	    val = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3] << 0);
	    writeLong(BaseAddress + ChannelOFFSET[0] + i, val);
	}

	writeLong(BaseAddress + VdsSpecialControlCommandOFFSET, SPECIALCONTROL_COMMAND_DATASENT);
    } else if (DeviceClass == V12C0) {

	for (i = 0; i < record.length(); i += 2) {
	    strcpy(buf, record.mid(i, 2).constData());
	    val = (buf[0] << 8) | (buf[1] << 0);
	    writeShort(BaseAddress + ChannelOFFSET[0] + i, val);
	}

	writeShort(BaseAddress + VhsSpecialControlCommandOFFSET, SPECIALCONTROL_COMMAND_DATASENT);
    } else {
	GetModuleDeviceClass();
    }

    //qDebug() << "Data sent:" << record << "\n";
}

void CVmeModule::ProgramModuleBaseAddress(uint16_t address)
{
    if (DeviceClass == V12C0) {
	writeShort(BaseAddress + VhsNewBaseAddressOFFSET, address);
	writeShort(BaseAddress + VhsNewBaseAddressXorOFFSET, address ^ 0xFFFF);

    } else if (DeviceClass == V24D0) {
	writeShort(BaseAddress + VdsNewBaseAddressOFFSET, address);
	writeShort(BaseAddress + VdsNewBaseAddressXorOFFSET, address ^ 0xFFFF);

    } else {
	GetModuleDeviceClass();
    }
}

uint16_t CVmeModule::VerifyModuleBaseAddress(void)
{
    uint16_t newAddress = -1;
    uint16_t temp;

    if (DeviceClass == V12C0) {
	newAddress = readShort(BaseAddress + VhsNewBaseAddressAcceptedOFFSET);
	temp = readShort(BaseAddress + VhsNewBaseAddressOFFSET);
	if (newAddress != temp) {
	    newAddress = -1;
	}

    } else if (DeviceClass == V24D0) {
	newAddress = readShort(BaseAddress + VdsNewBaseAddressAcceptedOFFSET);
	temp = readShort(BaseAddress + VdsNewBaseAddressOFFSET);
	if (newAddress != temp) {
	    newAddress = -1;
	}

    } else {
	GetModuleDeviceClass();
    }

    return newAddress;
}

//=============================================================================
// Channel Commands
//=============================================================================

uint32_t CVmeModule::GetChannelStatus(int channel)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + ChannelOFFSET[channel] + VhsChannelStatusOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + ChannelOFFSET[channel] + VdsChannelStatusOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

uint32_t CVmeModule::GetChannelEventStatus(int channel)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + ChannelOFFSET[channel] + VhsChannelEventStatusOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + ChannelOFFSET[channel] + VdsChannelEventStatusOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetChannelEventStatus(int channel, uint32_t status)
{
    if (DeviceClass == V12C0) {
	writeShortBitfield(BaseAddress + ChannelOFFSET[channel] + VhsChannelEventStatusOFFSET, status);
    } else if (DeviceClass == V24D0) {
	writeLongBitfield(BaseAddress + ChannelOFFSET[channel] + VdsChannelEventStatusOFFSET, status);
    } else {
	GetModuleDeviceClass();
    }
}

uint32_t CVmeModule::GetChannelEventMask(int channel)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + ChannelOFFSET[channel] + VhsChannelEventMaskOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + ChannelOFFSET[channel] + VdsChannelEventMaskOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetChannelEventMask(int channel, uint32_t mask)
{
    if (DeviceClass == V12C0) {
	writeShortBitfield(BaseAddress + ChannelOFFSET[channel] + VhsChannelEventMaskOFFSET, mask);
    } else if (DeviceClass == V24D0) {
	writeLongBitfield(BaseAddress + ChannelOFFSET[channel] + VdsChannelEventMaskOFFSET, mask);
    } else {
	GetModuleDeviceClass();
    }
}

uint32_t CVmeModule::GetChannelControl(int channel)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + ChannelOFFSET[channel] + VhsChannelControlOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + ChannelOFFSET[channel] + VdsChannelControlOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetChannelControl(int channel, uint32_t control)
{
    if (DeviceClass == V12C0) {
	writeShortBitfield(BaseAddress + ChannelOFFSET[channel] + VhsChannelControlOFFSET, control);
    } else if (DeviceClass == V24D0) {
	writeLongBitfield(BaseAddress + ChannelOFFSET[channel] + VdsChannelControlOFFSET, control);
    } else {
	GetModuleDeviceClass();
    }
}

float CVmeModule::GetChannelVoltageNominal(int channel)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelVoltageNominalOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelVoltageNominalOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

float CVmeModule::GetChannelCurrentNominal(int channel)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelCurrentNominalOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelCurrentNominalOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

float CVmeModule::GetChannelVoltageSet(int channel)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelVoltageSetOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelVoltageSetOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetChannelVoltageSet(int channel, float vset)
{
    if (DeviceClass == V12C0) {
	writeFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelVoltageSetOFFSET, vset);
    } else if (DeviceClass == V24D0) {
	writeFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelVoltageSetOFFSET, vset);
    } else {
	GetModuleDeviceClass();
    }
}

float CVmeModule::GetChannelVoltageMeasure(int channel)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelVoltageMeasureOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelVoltageMeasureOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

float CVmeModule::GetChannelCurrentSet(int channel)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelCurrentSetOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelCurrentSetOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetChannelCurrentSet(int channel, float iset)
{
    if (DeviceClass == V12C0) {
	writeFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelCurrentSetOFFSET, iset);
    } else if (DeviceClass == V24D0) {
	writeFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelCurrentSetOFFSET, iset);
    } else {
	GetModuleDeviceClass();
    }
}

float CVmeModule::GetChannelCurrentMeasure(int channel)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelCurrentMeasureOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelCurrentMeasureOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

float CVmeModule::GetChannelVoltageIlkMaxSet(int channel)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelVoltageIlkMaxSetOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelVoltageIlkMaxSetOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetChannelVoltageIlkMaxSet(int channel, float ilkmax)
{
    if (DeviceClass == V12C0) {
	return writeFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelVoltageIlkMaxSetOFFSET, ilkmax);
    } else if (DeviceClass == V24D0) {
	return writeFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelVoltageIlkMaxSetOFFSET, ilkmax);
    } else {
	GetModuleDeviceClass();
    }
}

float CVmeModule::GetChannelVoltageIlkMinSet(int channel)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelVoltageIlkMinSetOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelVoltageIlkMinSetOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetChannelVoltageIlkMinSet(int channel, float ilkmin)
{
    if (DeviceClass == V12C0) {
	return writeFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelVoltageIlkMinSetOFFSET, ilkmin);
    } else if (DeviceClass == V24D0) {
	return writeFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelVoltageIlkMinSetOFFSET, ilkmin);
    } else {
	GetModuleDeviceClass();
    }
}

float CVmeModule::GetChannelCurrentIlkMaxSet(int channel)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelCurrentIlkMaxSetOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelCurrentIlkMaxSetOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetChannelCurrentIlkMaxSet(int channel, float ilkmax)
{
    if (DeviceClass == V12C0) {
	return writeFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelCurrentIlkMaxSetOFFSET, ilkmax);
    } else if (DeviceClass == V24D0) {
	return writeFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelCurrentIlkMaxSetOFFSET, ilkmax);
    } else {
	GetModuleDeviceClass();
    }
}

float CVmeModule::GetChannelCurrentIlkMinSet(int channel)
{
    if (DeviceClass == V12C0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelCurrentIlkMinSetOFFSET);
    } else if (DeviceClass == V24D0) {
	return readFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelCurrentIlkMinSetOFFSET);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetChannelCurrentIlkMinSet(int channel, float ilkmin)
{
    if (DeviceClass == V12C0) {
	return writeFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelCurrentIlkMinSetOFFSET, ilkmin);
    } else if (DeviceClass == V24D0) {
	return writeFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelCurrentIlkMinSetOFFSET, ilkmin);
    } else {
	GetModuleDeviceClass();
    }
}

/**
 * Returns Voltage as formated String according to VoltageFormat.
 * Example: 1234.45
 */
QString CVmeModule::FormatVoltage(double v)
{
    if (vfUnit == vf_kilovolt) {
	v /= 1000.0;
    }

    return QString("%1") .arg(v, 0, 'f', vfPrecision);
}

/**
 * Returns the Voltage unit according to VoltageFormat.
 * Example: V
 */
QString CVmeModule::GetVoltageUnit(void)
{
    QString unit = "V";

    if (vfUnit == vf_kilovolt) {
	unit = "kV";
    }

    return unit;
}

/**
 * Returns Voltage as formated String according to VoltageFormat.
 * Example: 1234.45 V
 */
QString CVmeModule::FormatVoltageUnit(double v)
{
    return QString("%1 %2") .arg(FormatVoltage(v)) .arg(GetVoltageUnit());
}

/**
 * Returns Current as formated String according to CurrentFormat.
 * Example: 1.234
 */
QString CVmeModule::FormatCurrent(double i)
{
    if (cfUnit == cf_nanoamp) {
	i *= 1E9;
    } else if (cfUnit == cf_microamp) {
	i *= 1E6;
    } else if (cfUnit == cf_milliamp) {
	i *= 1E3;
    }

    return QString("%1") .arg(i, 0, 'f', cfPrecision);
}

/**
 * Returns the Current unit according to CurrentFormat.
 * Example: mA
 */
QString CVmeModule::GetCurrentUnit(void)
{
    QString unit = "A";

    if (cfUnit == cf_nanoamp) {
	unit = "nA";
    } else if (cfUnit == cf_microamp) {
	unit = "uA";
    } else if (cfUnit == cf_milliamp) {
	unit = "mA";
    }

    return unit;
}

/**
 * Returns Current as formated String according to CurrentFormat.
 * Example: 1.234 mA
 */
QString CVmeModule::FormatCurrentUnit(double i)
{
    return QString("%1 %2") .arg(FormatCurrent(i)) .arg(GetCurrentUnit());
}

void CVmeModule::SetModuleEmergencyOff(void)
{
    if (DeviceClass == V12C0) {
	writeLong(BaseAddress + VhsSetEmergencyAllChannelsOFFSET, 1);
    } else if (DeviceClass == V24D0) {
	writeLong(BaseAddress + VdsSetEmergencyAllChannelsOFFSET, 1);
    } else {
	GetModuleDeviceClass();
    }
}

void CVmeModule::ClearModuleEmergencyOff(void)
{
    if (DeviceClass == V12C0) {
	writeLong(BaseAddress + VhsSetEmergencyAllChannelsOFFSET, 0);
    } else if (DeviceClass == V24D0) {
	writeLong(BaseAddress + VdsSetEmergencyAllChannelsOFFSET, 0);
    } else {
	GetModuleDeviceClass();
    }
}

void CVmeModule::SetChannelOn(int channel)
{
    ChControlSTRUCT control;

    control.Word = GetChannelControl(channel);
    control.Bit.setON = 1;
    SetChannelControl(channel, control.Word);
}

void CVmeModule::SetChannelOff(int channel)
{
    ChControlSTRUCT control;

    control.Word = GetChannelControl(channel);
    control.Bit.setON = 0;
    SetChannelControl(channel, control.Word);
}

void CVmeModule::SetChannelEmergencyOff(int channel)
{
    ChControlSTRUCT control;

    control.Word = GetChannelControl(channel);
    control.Bit.setEmergency = 1;
    SetChannelControl(channel, control.Word);
}

void CVmeModule::ClearChannelEmergencyOff(int channel)
{
    ChControlSTRUCT control;

    control.Word = GetChannelControl(channel);
    control.Bit.setEmergency = 0;
    SetChannelControl(channel, control.Word);
}

int CVmeModule::GetModuleChannelNumber(void)
{
    return ChannelNumber;
}

uint32_t CVmeModule::GetModuleIlkOutStatus(void)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + VhsModuleIlkOutStatusOFFSET);
    } else if (DeviceClass == V24D0) {
	return 0; // FIXME
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

uint32_t CVmeModule::GetModuleIlkOutControl(void)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + VhsModuleIlkOutControlOFFSET);
    } else if (DeviceClass == V24D0) {
	return 0; /// FIXME
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

void CVmeModule::SetModuleIlkOutControl(uint32_t control)
{
    if (DeviceClass == V12C0) {
	writeShortBitfield(BaseAddress + VhsModuleIlkOutControlOFFSET, control);
    } else if (DeviceClass == V24D0) {
	/// FIXME
    } else {
	GetModuleDeviceClass();
    }
}

uint32_t CVmeModule::GetModuleIlkOutCount(void)
{
    if (DeviceClass == V12C0) {
	return readShort(BaseAddress + VhsModuleIlkCountOFFSET);
    } else if (DeviceClass == V24D0) {
	return 0; // FIXME
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

uint32_t CVmeModule::GetModuleIlkOutLastTrigger(void)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + VhsModuleIlkLastTriggerOFFSET);
    } else if (DeviceClass == V24D0) {
	return 0; // FIXME
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

uint32_t CVmeModule::GetModuleIlkOutChnActualActive(void)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + VhsModuleIlkChnActualActiveOFFSET);
    } else if (DeviceClass == V24D0) {
	return 0; // FIXME
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

uint32_t CVmeModule::GetModuleIlkOutChnEverTriggered(void)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + VhsModuleIlkChnEverTriggeredOFFSET);
    } else if (DeviceClass == V24D0) {
	return 0; // FIXME
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

uint32_t CVmeModule::GetModuleFlexGroupMemberList(int group)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + FlexGroupOFFSET[group] + VhsFlexGroupMemberOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + FlexGroupOFFSET[group] + VdsFlexGroupMember2OFFSET);
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

void CVmeModule::SetModuleFlexGroupMemberList(int group, uint32_t member)
{
    if (DeviceClass == V12C0) {
	writeShortBitfield(BaseAddress + FlexGroupOFFSET[group] + VhsFlexGroupMemberOFFSET, member);
    } else if (DeviceClass == V24D0) {
	writeLongBitfield(BaseAddress + FlexGroupOFFSET[group] + VdsFlexGroupMember2OFFSET, member);
    } else {
	GetModuleDeviceClass();
    }
}

uint32_t CVmeModule::GetModuleFlexGroupType(int group)
{
    if (DeviceClass == V12C0) {
	return readShortBitfield(BaseAddress + FlexGroupOFFSET[group] + VhsFlexGroupTypeOFFSET);
    } else if (DeviceClass == V24D0) {
	return readLongBitfield(BaseAddress + FlexGroupOFFSET[group] + VdsFlexGroupType2OFFSET);
    } else {
	GetModuleDeviceClass(); return 0;
    }
}

void CVmeModule::SetModuleFlexGroupType(int group, uint32_t type)
{
    if (DeviceClass == V12C0) {
	writeShortBitfield(BaseAddress + FlexGroupOFFSET[group] + VhsFlexGroupTypeOFFSET, type);
    } else if (DeviceClass == V24D0) {
	writeLongBitfield(BaseAddress + FlexGroupOFFSET[group] + VdsFlexGroupType2OFFSET, type);
    } else {
	GetModuleDeviceClass();
    }
}

float CVmeModule::GetChannelVoltageHardwareNominal(int channel)
{
    if ( (DeviceClass == V12C0) || (DeviceClass == V24D0) ) {
	return readFloat(BaseAddress + VoltageMaxSetOFFSET[channel]);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetChannelVoltageNominal(int channel, float maxset)
{
    if (DeviceClass == V12C0) {
	return writeFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelVoltageNominalOFFSET, maxset);
    } else if (DeviceClass == V24D0) {
	return writeFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelVoltageNominalOFFSET, maxset);
    } else {
	GetModuleDeviceClass();
    }
}

float CVmeModule::GetChannelCurrentHardwareNominal(int channel)
{
    if ( (DeviceClass == V12C0) || (DeviceClass == V24D0) ) {
	return readFloat(BaseAddress + CurrentMaxSetOFFSET[channel]);
    } else {
	GetModuleDeviceClass();
	return 0;
    }
}

void CVmeModule::SetChannelCurrentNominal(int channel, float maxset)
{
    if (DeviceClass == V12C0) {
	return writeFloat(BaseAddress + ChannelOFFSET[channel] + VhsChannelCurrentNominalOFFSET, maxset);
    } else if (DeviceClass == V24D0) {
	return writeFloat(BaseAddress + ChannelOFFSET[channel] + VdsChannelCurrentNominalOFFSET, maxset);
    } else {
	GetModuleDeviceClass();
    }
}
