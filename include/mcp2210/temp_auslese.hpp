//#include "MCP2210_linux/mcp2210.h"
#include "temp_defaults.hpp"
#include "temp_helpers.hpp"
#include "helper_functions.hpp"
#include <iostream>
#include <fstream>
#include <usb.h>
#include <bitset>
#include <math.h>
#include <vector>
#include <string>
#include <cstddef>
#include <algorithm>
#include <atomic>


/* current compilation flags
   g++ mini_auslese_clean.cpp -Wall -pedantic -o mini_auslese -lusb -IMCP2210_linux -l:MCP2210_linux/build/Debug/GNU-Linux-x86/mcp2210.o -l:MCP2210_linux/build/Debug/GNU-Linux-x86/hid.o -ludev
 */

/* to compile everything from scratch:
   g++ temp_auslese.cpp temp_defaults.cpp temp_helpers.cpp mcp2210.cpp hid.c -Wall -o mini_auslese -lusb -ludev
   
   meaning without a compiled library. probably the way to go for TOS
 */

// main communication functions to read and write single bytes
void write_byte_to_register(hid_device *handle, byte w_addr, byte data);
byte read_register(hid_device *handle, byte r_addr);

// writes fault threshold registers to defaults
void write_fault_thresholds(hid_device *handle);

// checks fault register
void check_fault_register(byte fault);
// get correct value from transfer status def as return value of transfer
byte get_value_from_status_def(SPIDataTransferStatusDef status_def);
// manual chip select function based on currently active slave device
int chip_select(hid_device *handle, bool activate);

// read the current RTD values from the active slave device
float get_temp(hid_device *handle, int RTD_Resistance, int Reference_Resistor);

// set the default settings read from temp_defaults.cpp
int set_spi_transfer_settings(hid_device *handle);
void set_gpio_pins(hid_device *handle);
void set_chip_settings(hid_device *handle);
// deprecated functions, debugging and initialization as USB deviec
void debug_spi_via_config(hid_device *handle);

void loop(hid_device *handle,
	  std::atomic_bool *loop_continue,
	  AtomicTemps &temps,	  
	  int sleep_time,
	  int rtd_resistance,
	  int ref_resistor);

void loop_and_log(hid_device *handle,
		  std::atomic_bool *loop_continue,
		  AtomicTemps &temps,
		  int sleep_time,
		  int rtd_resistance,
		  int ref_resistor,
		  std::string path_name);

void loop_temp(hid_device *handle,
	       std::atomic_bool *loop_continue,
	       AtomicTemps &temps,
	       int sleep_time,
	       std::string path_name);

int init_and_log_temp(std::atomic_bool *loop_continue, std::string path_name, AtomicTemps &temps);
int temp_auslese_main(std::atomic_bool *loop_continue, bool log_flag);

