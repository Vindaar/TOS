#include "mcp2210/temp_auslese.hpp"

void write_byte_to_register(hid_device *handle, byte w_addr, byte data){
    // writes a byte data to address w_addr of the currently selected (!)
    // slave

    chip_select(handle, true);
    
    byte conf_ar[64];
    conf_ar[0] = w_addr;
    conf_ar[1] = data;
    
    // SPISendReceive(handle, &w_addr, 1, 0);
    // SPISendReceive(handle, &data, 1, 0);
    SPISendReceive(handle, conf_ar, 2, 0);

    chip_select(handle, false);
}

byte read_register(hid_device *handle, byte r_addr){
    /* read_register(hid_device*, byte) accepts the register address to be read
       and returns the contents of the register to r_addr for the currently
       selected (!) slave
    */
    chip_select(handle, true);

    SPIDataTransferStatusDef status_def;
    // 7 is number of bytes per SPI packet - 1
    status_def = SPISendReceive(handle, &r_addr, 1, 7);
    byte value = get_value_from_status_def(status_def);

    chip_select(handle, false);    
    return value;
}

void write_fault_thresholds(hid_device *handle){
    // writes the fault registers for both devices

    for(int i = 0; i < 2; i++){
        // Writing High Fault Threshold MSB
        write_byte_to_register(handle, WRITE_HIGH_FAULT_THRESHOLD_MSB, 0xFF);
        // Writing High Fault Threshold LSB
        write_byte_to_register(handle, WRITE_HIGH_FAULT_THRESHOLD_LSB, 0xFF);
        // Writing Low Fault Threshold MSB
        write_byte_to_register(handle, WRITE_LOW_FAULT_THRESHOLD_MSB,  0x00);
        // Writing Low Fault Threshold LSB
        write_byte_to_register(handle, WRITE_LOW_FAULT_THRESHOLD_MSB,  0x00);
	// change active slave
	get_current_active_slave(true);
    }	
    return;
}

void check_fault_register(byte fault){
    // Fault(byte) function requires the contents of the fault bit to be provided.
    // It checks for the bits that are set and provides the faulty
    // bit information on the serial console.

    std::cout << fault << std::endl;;
    // temporary variable created:
    //     Purpose is to find out which error bit is set in the fault register
    byte temp = 0;
    // Logic ANDing fault register contents with 0b10000000 to detect for D7 error bit
    temp = fault & 0x80;
    if(temp > 0) {
	std::cout << "Bit D7 is Set. It's Possible your RTD device is disconnected from "
		  << "RTD+ or RTD-. Please verify your connection and High Fault Threshold Value"
		  << std::endl;
    }
    temp = fault & 0x40;
    if(temp > 0) {
	std::cout << "Bit D6 is Set. It's Possible your RTD+ and RTD- is shorted. Please verify your connection and your Low Fault Threshold Value." << std::endl;
    }
    temp = fault & 0x20;
    if(temp > 0){
	std::cout << "Bit D5 is Set. Vref- is greater than 0.85 * Vbias" << std::endl;
    }
    temp = fault & 0x10;
    if(temp > 0){
	std::cout << "Bit D4 is Set. Please refer to data sheet for more information" << std::endl;
    }
    temp = fault &0x08;
    if(temp > 0){
	std::cout << "Bit D3 is Set. Please refer to data sheet for more information" << std::endl;
    }
    temp = fault &0x04;
    if(temp > 0){
	std::cout << "Bit D2 is Set. Please refer to data sheet for more information" << std::endl;
    }
}

byte get_value_from_status_def(SPIDataTransferStatusDef status_def){
    // returns a useful value from the status def
    unsigned char ch[60];
    int nbytes = status_def.NumberOfBytesReceived;

    for(int i = 0; i < nbytes; i++){
        ch[i] = status_def.DataReceived[i];
    }
    int val;
    val = static_cast<byte>(ch[0]);

#if DEBUG==1
    print_spi_transfer_data(status_def);
#endif

    return val;    
}


int chip_select(hid_device *handle, bool activate){
    // (de-)activates the chip select for currently selected (!) slave
    int active_slave;
    int r = 0;
    GPPinDef gp;

    // get current active slave without changing it
    active_slave = get_current_active_slave(false);
    gp = get_gpio_settings();
    if(activate){
	gp.GP[active_slave].GPIOOutput = GPIO_VALUE_LOW;
    }
    else{
	gp.GP[active_slave].GPIOOutput = GPIO_VALUE_HIGH;
    }
    r = SetGPIOPinVal(handle, gp);
    
    return r;
}

float get_temp(hid_device *handle, int RTD_Resistance, int Reference_Resistor){
    
    byte lsb_rtd;
    byte msb_rtd;
    lsb_rtd = read_register(handle, READ_LSB);
    msb_rtd = read_register(handle, READ_MSB);
    float temp;
    temp = calc_temperature(lsb_rtd, msb_rtd, Reference_Resistor, RTD_Resistance);
    
    return temp;
}

int set_spi_transfer_settings(hid_device *handle){

    SPITransferSettingsDef spi_set;
    spi_set = get_default_spi_transfer_settings();
    
    // and transfer
    int r = 0;
    r = SetSPITransferSettings(handle, spi_set);
#if DEBUG==1
    std::cout << "return of set spi settings: " << r << std::endl;
    print_spi_transfer_settings(handle, spi_set);
#endif
    return r;
}

void set_gpio_pins(hid_device *handle){
    // DEPRECATED FUNCTION
    // default values are set at the beginning using set_chip_settings
    // chip select is then either done manually using chip_select()
    // or the PIN_DESIGNATION == CHIP_SELECT plus the Idle and Active
    // chip select options of the spi_transfer_settings
    int r = 0;
    GPPinDef gp;

    gp = get_gpio_settings();

    // write GPIOPinValue
    r  = SetGPIOPinVal(handle, gp);
    r  *= SetGPIOPinDirection(handle, gp);
    std::cout << "setting GPIO pin direction error code : " << r << std::endl;
    gp = GetGPIOPinDirection(handle);
    gp = GetGPIOPinValue(handle);

    print_gpio_pins(gp.GP);

    return;
}

void set_chip_settings(hid_device *handle){
    int r = 0;
    ChipSettingsDef settings_def;

    settings_def = GetChipSettings(handle);
    std::cout << "RemoteWakeUpEnabled : " << settings_def.RemoteWakeUpEnabled << "\n"
	      << "DedicatedFunctionInterruptPinMode : " << settings_def.DedicatedFunctionInterruptPinMode << "\n"
	      << "SPIBusReleaseMode : " << settings_def.SPIBusReleaseMode << "\n"
	      << "NVRamChipParamAccessControl : " << settings_def.NVRamChipParamAccessControl
	      << std::endl;

    GPPinDef gp;
    gp = get_gpio_settings();

    for(int i = 0; i < 9; i++){
	settings_def.GP[i] = gp.GP[i];
    }
    
    r = SetChipSettings(handle, settings_def);
    std::cout << "SetChipSettings error code : " << r << std::endl;

}

void debug_spi_via_config(hid_device *handle){
    // function to debug the slaves by writing configurations and
    // checking whether they are correctly written    

    byte dummy;
    dummy = 0x00;
    std::cout << "Now try to set configuration and read it back" << std::endl;
    
    byte config;
    config = READ_CONFIGURATION;
    //config = std::pow(2,7) + std::pow(2,5) + std::pow(2,3) + std::pow(2,1);
    config = std::pow(2,7) + std::pow(2,1) + std::pow(2,0);
    //config = std::pow(2,7) + std::pow(2,0);
    std::bitset<8> conf_binary(config);
    std::cout << "Config to write " << conf_binary << std::endl;

    int active_slave = 0;
    for(int i = 0; i < 2; i++){
	active_slave = get_current_active_slave(false);

	std::cout << "Setting default config for slave " << active_slave << std::endl;
        // now set config
        byte conf_address;
        conf_address = CONFIGURATION;//0xff;
        std::bitset<8> conf_addr_binary(conf_address);
        std::cout << "Config address " << conf_addr_binary << std::endl;

        write_byte_to_register(handle, conf_address, config);
        std::cout << "\n\n\n sleeping before reading..." << std::endl;

        int val;
        val = read_register(handle, dummy);
        if(val != 0){
	    std::cout << "reading non successful, error code: " << val << std::endl;
        }
        simple_sleep(250);

	active_slave = get_current_active_slave(true);
    }

    return;
}

void loop(hid_device *handle,
	  std::atomic_bool &loop_continue,
	  AtomicTemps &temps,
	  int sleep_time,
	  int rtd_resistance,
	  int ref_resistor){
    // is being called from loop_temp in case of no logging to file

    byte lsb_rtd = read_register(handle, READ_LSB);
    byte fault_test = lsb_rtd & 0x01;
    float temp1;
    float temp2;
    int active_slave;


    while( (fault_test == 0) &&
	   (loop_continue == true) )
    {
	std::cout << std::endl;
	temp1 = get_temp(handle, rtd_resistance, ref_resistor);
	// get current active slave without changing it (false)
	active_slave = get_current_active_slave(false);
	std::cout << "Temperature measured from RTD for " << "\n"
		  << "    slave "
		  <<  active_slave << " is: " << temp1 << std::flush;
	// now change current active chip by calling function with true as argument
	active_slave = get_current_active_slave(true);	    
	temp2 = get_temp(handle, rtd_resistance, ref_resistor);
	std::cout << "    slave "
		  <<  active_slave << " is: " << temp2 << std::endl;
	simple_sleep(sleep_time);
	// now change current active chip by calling function with true as argument
	get_current_active_slave(true);
	lsb_rtd = read_register(handle, READ_LSB); 
	fault_test = lsb_rtd & 0x01;
	std::cout << std::endl;


	// set current values in AtomicTemps reference
	temps.first  = (int) temp1;
	temps.second = (int) temp2;
    }
    if(fault_test != 0){
	std::cout << "Error was detected. The RTD resistance measured is not within the range specified in nthe Threshold Registers." << std::endl;
    }
    else{
	std::cout << "Looping thread was stopped by parent thread. Stopping." << std::endl;
    }

    return;
}

void loop_and_log(hid_device *handle,
		  std::atomic_bool &loop_continue,
		  AtomicTemps &temps,		  
		  int sleep_time,
		  int rtd_resistance,
		  int ref_resistor,
		  std::string path_name){
    // besides printing, this function also logs the data to file
    std::string date = "";
    if(path_name == ""){
	/* in case path_name is empty, we are running from the standalone
	 log temperature command in TOS. In this case save the log file
	 to the TOS/log folder
	*/
	path_name = "./log";
    }
    std::string filepath = get_temp_log_file_path(path_name);

    std::fstream outfile;
    outfile.open(filepath, std::fstream::out);
    outfile << "# Temperature log file" << "\n"
	    << "# Temp_IMB \t Temp_Septem \t DateTime" << std::endl;
    outfile.close();

    byte lsb_rtd = read_register(handle, READ_LSB);
    byte fault_test = lsb_rtd & 0x01;
    float temp1;
    float temp2;
    int active_slave;

    float temp_IMB = 0;
    float temp_septem = 0;

    // the time after which we perform a new temp measurement
    // after sleep_time we average over the number of measurements
    // we took
    int measure_sleep_time = 250;
    int num_measurements = sleep_time / measure_sleep_time;
    int counter = 0;

    std::cout << "meas sleep " << measure_sleep_time << "  "
	      << "sleep time " << sleep_time << "  "
	      << "num meas " << num_measurements << std::endl;

    while( (fault_test == 0) &&
	   (loop_continue == true) )
    {
	if( ((counter % num_measurements) == 0) &&
	    (temp_IMB != 0) &&
	    (temp_septem != 0) ){


	    // re-open the output file and append data
	    outfile.open(filepath, std::fstream::app);

	    // calculate average of temps
	    temp_septem /= num_measurements;
	    temp_IMB    /= num_measurements;

	    // get current date and time
	    date = currentDateTime();

	    // now output temps to file
	    outfile << temp_IMB << "\t"
		    << temp_septem << "\t"
		    << date << std::endl;
	    // close outfile
	    outfile.close();
	    
	    std::cout << "Temperature measured from RTD for " << "\n"
		      << "    slave "
		      << "IMB"  << " is: " << temp_IMB << std::flush;
	    std::cout << "    slave "
		      << "septem" << " is: " << temp_septem << std::endl;

	    // set current values in AtomicTemps reference
	    temps.first  = (int) temp_IMB;
	    temps.second = (int) temp_septem;

	    // and reset counter and temps
	    temp_IMB     = 0;
	    temp_septem  = 0;
	}
	temp1 = get_temp(handle, rtd_resistance, ref_resistor);
	
	// get current active slave without changing it (false)
	active_slave = get_current_active_slave(false);
	// now change current active chip by calling function with true as argument
	active_slave = get_current_active_slave(true);	    
	temp2 = get_temp(handle, rtd_resistance, ref_resistor);
	simple_sleep(measure_sleep_time);
	// now change current active chip by calling function with true as argument
	get_current_active_slave(true);	    
	lsb_rtd = read_register(handle, READ_LSB); 
	fault_test = lsb_rtd & 0x01;

	if(active_slave == 5){
	    temp_septem += temp1;
	    temp_IMB    += temp2;
	}
	else{
	    temp_IMB    += temp1;
	    temp_septem += temp2;
	}
	counter++;
    }

    // close the outfile again
    outfile.close();

    if(fault_test != 0){
	std::cout << "Error was detected. The RTD resistance measured is not within the range specified in nthe Threshold Registers." << std::endl;
    }
    else{
	std::cout << "Looping thread was stopped by parent thread. Stopping." << std::endl;
    }

    return;
}


void loop_temp(hid_device *handle,
	       std::atomic_bool &loop_continue,
	       AtomicTemps &temps,	       
	       int sleep_time,
	       bool log_flag,
	       std::string path_name){
    /* loops until being stopped by parent thread (settings loop_continue to true)
       and prints current temperatures to console
       Additionally updates the temperatures in the AtomicTemps reference
    */
    
    int config;
    config = read_register(handle, 0x00);
    if(config == 0b11000001){
	std::cout << "config properly written" << std::endl;

	write_fault_thresholds(handle);
	
	// Reference Resistor installed on EVM
	int rtd_resistance = 1000;
	// RTD Resistance at 0 Degrees
	int ref_resistor = 3900;

	if(log_flag == true){
	    loop_and_log(handle, loop_continue, temps, sleep_time, rtd_resistance, ref_resistor, path_name);
	}
	else{
	    loop(handle, loop_continue, temps, sleep_time, rtd_resistance, ref_resistor);
	}
    }
    return;
}

int init_and_log_temp(std::atomic_bool &loop_continue, std::string path_name, AtomicTemps &temps){
    /* initializes a MCP2210 device and reads out the temperatures
       inputs: 
           std::atomic_bool *loop_continue: atomic flag to be able to stop the 
	   temperature loop from the calling function.
	   std::string path_name: name of the path to the folder (typically Run 
	   folder), in which the temperature log is to be stored
	   AtomicTemps &temps: reference to an AtomicTemps object, which stores 
	   (rounded to ints) values of the current temperature. This is used 
	   to be able to (roughly) read the current temperatures and be able
	   to shutdown a (e.g.) Run in case the cooling has stopped working 
	   and ramp down the HV.
    */
    
    hid_device *handle;

    // initialize using MCP2210 library
    handle = InitMCP2210();
    if (handle == NULL){
	std::cout << "Warning: Could not find MCP2210 as usb device!" << std::endl;
	return -1;
    }
	
    set_chip_settings(handle);

    int r;
    r = set_spi_transfer_settings(handle);
    if(r != 0){
	std::cout << "Warning: spi settings could not be properly set. "
		  << "Readout might be erroneously." << std::endl;
    }

    byte Fault_Error;
    Fault_Error = read_register(handle, FAULT_STATUS);
    if(Fault_Error == 0){
	for(int i = 0; i < 2; i++){
	    write_byte_to_register(handle, CONFIGURATION, 0b11000001);
	    get_current_active_slave(true);
	}
	bool log_flag = true;
	loop_temp(handle, loop_continue, temps, 5000, log_flag, path_name);
    }
    else{
	check_fault_register(Fault_Error);
    }

    // CancelSPITransfer(handle);
    return 0;
}

int temp_auslese_main(std::atomic_bool &loop_continue, bool log_flag){
    // argument is a pointer to a bool variable from the calling function

    hid_device *handle;

    // initialize using MCP2210 library
    handle = InitMCP2210();
    if (handle == NULL){
	std::cout << "Warning: Could not find MCP2210 as usb device!" << std::endl;
	return -1;
    }
	
    set_chip_settings(handle);

    int r;
    r = set_spi_transfer_settings(handle);
    if(r != 0){
	std::cout << "Warning: spi settings could not be properly set. "
		  << "Readout might be erroneously." << std::endl;
    }
        
    //set_gpio_pins(handle);
	
    print_gpio_pins(handle);

    //debug_spi_via_config(handle);


    AtomicTemps temps;
    temps.first = 10;
    temps.second = 10;

    byte Fault_Error;
    Fault_Error = read_register(handle, FAULT_STATUS);
    if(Fault_Error == 0){
	for(int i = 0; i < 2; i++){
	    write_byte_to_register(handle, CONFIGURATION, 0b11000001);
	    get_current_active_slave(true);
	}
	loop_temp(handle, loop_continue, temps,  250, log_flag, "");
    }
    else{
	check_fault_register(Fault_Error);
    }

    CancelSPITransfer(handle);


    return 0;
}


