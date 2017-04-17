#include "mcp2210/temp_defaults.hpp"

GPPinDef get_gpio_settings(){
    /* returns the settings we use for the GPIO pins */
    
    GPPinDef gp;

    for (int i = 0; i < 9; i++){
        gp.GP[i].GPIOOutput     = GPIO_VALUE_HIGH;
        gp.GP[i].GPIODirection  = GPIO_DIRECTION_INPUT;
	gp.GP[i].PinDesignation = GPIO_PIN_DESIGNATION_GPIO;
    }
    // set TMesA and TMesB GPIO directions to input (\bar{DRDY} 

    gp.GP[5].GPIODirection  = GPIO_DIRECTION_OUTPUT;
    gp.GP[7].GPIODirection  = GPIO_DIRECTION_OUTPUT;
    //gp.GP[7].GPIOOutput     = GPIO_VALUE_LOW;
    //gp.GP[5].GPIOOutput     = GPIO_VALUE_HIGH;
    //gp.GP[5].PinDesignation = GPIO_PIN_DESIGNATION_CHIP_SELECT;
    //gp.GP[7].PinDesignation = GPIO_PIN_DESIGNATION_CHIP_SELECT;

    return gp;
}


SPITransferSettingsDef get_default_spi_transfer_settings(){
    // returns the default SPI transfer settings to use
    SPITransferSettingsDef spi_set;
    /* NOTE: Explanation of active and idle chip select:
       Bits set in idle chip select will cause:
           0 means low on corresponding pin
	   1   "   high         "
	 during NO activity (sending etc.)
       Bits set in active chip select will cause:
           0 means low on corresponding pin
	   1   "   high         "
	 during ACTIVITY (sending etc.)
    */
    // select GPIO pin 8 for TMesB (\bar{CS} of MAX...) 
    unsigned int chip_select = 0xffff;
    // results in all 1s except bit 5, which is 0
    chip_select = chip_select ^ 1 << 5;
    std::bitset<16> chp_slct(chip_select);
    std::cout << "Chip select : " << chp_slct << std::endl;
    unsigned int idle_chip_select = 0;
    // results in all 0s except bits 5 and 7 are 1
    idle_chip_select = 1 << 5 | 1 << 7;

    // set spi transfer settings
    spi_set.ActiveChipSelectValue   = chip_select;
    spi_set.IdleChipSelectValue     = idle_chip_select;
    // select SPI mode 1 (MAX can use 1 and 3)
    spi_set.SPIMode                 = 1;
    // bitrate set to 3 Mbps
    spi_set.BitRate                 = 3000000;
    // bytes per transfer 2 is enough to write and read from register
    spi_set.BytesPerSPITransfer     = 2;
    // chip select delay 1, works well
    spi_set.CSToDataDelay           = 1;
    // same as CS to data delay
    spi_set.LastDataByteToCSDelay   = 1;
    // subsequent data delay 1 is fine
    spi_set.SubsequentDataByteDelay = 1;

    return spi_set;
}
