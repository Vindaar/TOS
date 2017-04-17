#include "mcp2210/temp_helpers.hpp"

void simple_sleep(int time_ms = 250){
    std::chrono::milliseconds timespan(time_ms); // or whatever
    std::this_thread::sleep_for(timespan);
}

const std::string get_temp_log_file_path(std::string pathName){
    // returns the path for the temperature
    // log file based on the run path name

    std::string filename;
    std::string filepath;
    filename = "/temp_log.txt";
    
    filepath = pathName + filename;

    std::cout << "Printing temperature log to " << filepath << std::endl;
    return filepath;
}

int get_current_active_slave(bool change){
    // change the currently active slave and return
    // that slave

    // initialized once
    static int active_slave = 0;
    if(active_slave == 0){
	// upon first call we return the active slave
	active_slave = ACTIVE_SLAVE;
    }
    else if( (active_slave == ACTIVE_SLAVE) &&
	     (change == true) ){
	active_slave = INACTIVE_SLAVE;
    }
    else if( change == true ){
	active_slave = ACTIVE_SLAVE;
    }
    return active_slave;
}

void print_gpio_pins(GPPin GP[9]){
    /* prints the values, designations and directions of all GPs of the
       module */
    for (int i = 0; i < 9; i++){
        std::cout << "Pin number : " << i << "\n"
                  << "    direction: " << GP[i].GPIODirection << "\n"
                  << "    output (0 / 1 <-> low / high) : "  << GP[i].GPIOOutput << "\n"
                  << "    pin designation (0 / 1 / 2 <-> GPIO / CS / DFP : " 
		  << GP[i].PinDesignation
		  << std::endl;
    }
    return;
}

void print_gpio_pins(hid_device *handle){
    /* overloaded function, first gets the pin values and direction, then prints */
    ChipSettingsDef settings_def;
    settings_def = GetChipSettings(handle);
    for (int i = 0; i < 9; i++){
        std::cout << "Pin number : " << i << "\n"
                  << "    direction: " << settings_def.GP[i].GPIODirection << "\n"
                  << "    output (0 / 1 <-> low / high) : "  << settings_def.GP[i].GPIOOutput << "\n"
                  << "    pin designation (0 / 1 / 2 <-> GPIO / CS / DFP : " 
		  << settings_def.GP[i].PinDesignation
		  << std::endl;
    }
}

void print_spi_transfer_data(SPIDataTransferStatusDef status_def){
    /* prints debug output for a SPI transfer based on a status_def */
    std::cout << "Error code : "   << status_def.ErrorCode
	      << std::endl;
    std::cout << "EngineStatus : " << std::hex << status_def.SPIEngineStatus
	      << std::endl;
    int nbytes = status_def.NumberOfBytesReceived; 
    std::cout << "nbytes : " << nbytes << std::endl;
    std::cout << "data   : " << std::flush;
    unsigned char ch[64];
    for(int i = 0; i < nbytes; i++){
	std::cout << static_cast<int>(status_def.DataReceived[i]) << std::flush;
        ch[i] = status_def.DataReceived[i];
    }
    std::cout << std::endl;

    std::cout << std::endl;
    for(int i = 0; i < nbytes; i++){
	std::cout << static_cast<int>(status_def.DataReceived[i]) << std::flush;
	std::bitset<8> data_binary(ch[i]);
	std::cout << "binary data byte : " << i << " " << data_binary << std::endl;
    }
    int val = static_cast<byte>(ch[0]);
    std::bitset<8> value_bin(val);
    std::cout << "value " << value_bin
	      << " hex: " << std::hex << static_cast<int>(val)
	      << std::endl;
    std::cout << std::endl;
    return;
}

void print_spi_transfer_settings(hid_device *handle, SPITransferSettingsDef spi_set){
    /* debug function, which prints the current and new spi transfer
       settings from an spi_set struct
    */
    spi_set = GetSPITransferSettings(handle);
    std::cout << "SPI transfer settings after setting:" << std::endl;
    std::cout << "spi bitrate : " << spi_set.BitRate << std::endl;
    std::cout << "spi mode : " << spi_set.SPIMode << std::endl;
    std::cout << "bytes per transfer : " << spi_set.BytesPerSPITransfer << std::endl;

    return;
}



double calc_temperature(byte lsb_rtd, byte msb_rtd, float ref_resistor, float RTD_resistance){
    /* calculates the temperature using Callender-Van-Dusen equation from 
       LSB and MSB of the RTD register
       inputs: 
       byte lsb_rtd: least significant bytes of RTD reading
       byte msb_rtd: most        "       "       "     "
       float ref_resistor: reference resistor on board
       float RTD_resistance: resistance of PT<resistance> at 0 degree
    */
    
    // parameters for equation
    static float a = 0.00390830;
    static float b = -0.0000005775;
    // c not used, because we use approximation without T^3 dependence (fine for temps
    // 0 < T < 850 deg C; see MAX31865 datasheet)
    // static float c = -0.00000000000418301;

    double temperature = 0;

    // Combining RTD_MSB and RTD_LSB to protray decimal value.
    // Removing MSB and LSB during shifting / ANDing
    float RTD = ((msb_rtd << 7) + ( (lsb_rtd & 0xFE) >> 1));
    // Conversion of ADC RTD code to resistance 
    float R = (RTD * ref_resistor)/32768;


    /*Callendar-Van Dusen equation is used for temperature linearization. 
      Coefficient of equations are as follows:
      R(T) = R0(1 + aT + bT^2 + c(T - 100)T^3)
      Equation from : MAX31865 data shett
    */

    // Conversion of RTD resistance to Temperature
    temperature = -RTD_resistance*a +
	sqrt(RTD_resistance * RTD_resistance * a * a
	     - 4 * RTD_resistance * b * (RTD_resistance-R));
    // final conversion
    temperature /= 2 * RTD_resistance * b;

    return temperature;
}

