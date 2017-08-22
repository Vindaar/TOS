// some small helper functions for the temp readout

#include "mcp2210.h"
#include "temp_defaults.hpp"

// C++
#include <thread>
#include <chrono>
#include <iostream>
#include <math.h>
#include <atomic>

// define a pair of atomic_ints to store integers of temperatures (IMB and Septem)
// using atomic_ints, because implementations of atomic_floats do not exist.
// (atomics make sure we do not get undefined behaviour in case one thread writes
// to these variables while another reads them at the same time
// As a rule:
//     the .first  of the pair corresponds to IMB
//     the .second of the pair corresponds to Septem
typedef std::pair<std::atomic_int, std::atomic_int> AtomicTemps;

// alternatively we could use the following struct
// define a struct, which only contains
// typedef struct AtomicTemps {
//     std::atomic_int t_imb;
//     std::atomic_int t_sep;
// } AtomicTemps;




void simple_sleep(int time_ms);
const std::string get_temp_log_file_path(std::string pathName);
int get_current_active_slave(bool change);
void print_gpio_pins(GPPin GP[9]);
void print_gpio_pins(hid_device *handle);
void print_spi_transfer_data(SPIDataTransferStatusDef status_def);
void print_spi_transfer_settings(hid_device *handle, SPITransferSettingsDef spi_set);
double calc_temperature(byte lsb_rtd, byte msb_rtd, float ref_resistor, float RTD_resistance);

