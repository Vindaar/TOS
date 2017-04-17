// some small helper functions for the temp readout

#include "mcp2210.h"
#include "temp_defaults.hpp"

// C++
#include <thread>
#include <chrono>
#include <iostream>
#include <math.h>

void simple_sleep(int time_ms);
const std::string get_temp_log_file_path(std::string pathName);
int get_current_active_slave(bool change);
void print_gpio_pins(GPPin GP[9]);
void print_gpio_pins(hid_device *handle);
void print_spi_transfer_data(SPIDataTransferStatusDef status_def);
void print_spi_transfer_settings(hid_device *handle, SPITransferSettingsDef spi_set);
double calc_temperature(byte lsb_rtd, byte msb_rtd, float ref_resistor, float RTD_resistance);

