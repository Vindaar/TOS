// this header contains the functions, which return the default values
// for the GPIO settings as well as all macros
// TODO: extend this to either use these defaults or read from a
// config file

#include "mcp2210.h"
#include <iostream>
#include <bitset>

#define MCP2210_ID_VENDOR      	      	     1240 // 0x4D8
#define MCP2210_ID_PRODUCT     	      	     222  // 0xDE

#define GPIO_PIN_DESIGNATION_GPIO            0x00
#define GPIO_PIN_DESIGNATION_CHIP_SELECT     0x01
#define GPIO_PIN_DESIGNATION_DED_FN_PIN      0x02

#define GPIO_VALUE_LOW                       0
#define GPIO_VALUE_HIGH                      1

#define READ_MSB                             0x01
#define READ_LSB                             0x02

#define ACTIVE_SLAVE                         5
#define INACTIVE_SLAVE                       7

// define all registers

//Registers defined in Table 1 on page 12 of the data sheet
#define CONFIGURATION   		     0b10000000 //0x80H                          
#define READ_CONFIGURATION   		     0b00000000 //0x00H
#define WRITE_HIGH_FAULT_THRESHOLD_MSB       0b10000011 //0x83H
#define WRITE_HIGH_FAULT_THRESHOLD_LSB       0b10000100 //0x84H
#define READ_HIGH_FAULT_THRESHOLD_MSB        0b00000011 //0x03H
#define READ_HIGH_FAULT_THRESHOLD_LSB        0b00000100 //0x04H
#define WRITE_LOW_FAULT_THRESHOLD_MSB        0b10000101 //0x85H
#define WRITE_LOW_FAULT_THRESHOLD_LSB        0b10000110 //0x86H
#define READ_LOW_FAULT_THRESHOLD_MSB         0b00000101 //0x05H
#define READ_LOW_FAULT_THRESHOLD_LSB         0b00000110 //0x06H
#define FAULT_STATUS   			     0b00000111 //0x07H


GPPinDef get_gpio_settings();
SPITransferSettingsDef get_default_spi_transfer_settings();
