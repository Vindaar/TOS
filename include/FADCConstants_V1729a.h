#ifndef V1729a_FADCCONSTANTS_H
#define V1729a_FADCCONSTANTS_H 1

  //{
  //multiplicative numbers for the construction of VME comamnds
  //see p. 25 
  static const unsigned int VME_OFFSET = 0x10000;
  static const unsigned int MULT_SUB_ADRESS = 0x100;

  //V1729A control commands. Compare manual p. 29.
  //(some GPIB signals aren't included)
  //FIXME all are multiplied by 100
  static const unsigned int INTERRUPT = 0x0000;
  static const unsigned int FP_FREQUENCY = 0x0100;
  static const unsigned int FPGA_VERSION = 0x0200;
  static const unsigned int MODE_REGISTER = 0x0300;
  static const unsigned int EV_OF_FPGA_VERSION = 0x0400;
  //5-7 not mentioned
  static const unsigned int RESET = 0x00800;
  static const unsigned int LOAD_TRIGGER_THRESHOLD = 0x0900;
  static const unsigned int TRIGGER_THRESH_DAC_ALL = 0x0A00;
  //0B+0C GPIB/USB stuff
  static const unsigned int RAM_DATA_VME = 0x0D00;
  static const unsigned int RAM_INT_ADD_LSB = 0x0E00;
  static const unsigned int RAM_INT_ADD_MSB = 0x0F00;
  static const unsigned int MAT_CTRL_REGISTER_LSB = 0x1000;
  static const unsigned int MAT_CTRL_REGISTER_MSB = 0x1100;
  //12-16 reserved
  static const unsigned int START_ACQUISITION = 0x01700;
  static const unsigned int PRETRIG_LSB = 0x1800;
  static const unsigned int PRETRIG_MSB = 0x1900;
  static const unsigned int POSTTRIG_LSB = 0x1A00;
  static const unsigned int POSTTRIG_MSB = 0x1B00;
  static const unsigned int SOFTWARE_TRIGGER = 0x1C00;
  static const unsigned int TRIGGER_TYPE = 0x1D00;
  static const unsigned int TRIGGER_CHANNEL_SOURCE = 0x1E00;
  //1F not mentioned
  static const unsigned int TRIGGER_REC = 0x2000;
  static const unsigned int FAST_READ_MODES = 0x2100;
  static const unsigned int NB_OF_COLS_TO_READ = 0x2200;
  static const unsigned int CHANNEL_MASK = 0x2300;
  //24 reserved, 25 not mentioned
  static const unsigned int VALP_CP_REGISTER = 0x2600;
  static const unsigned int VALI_CP_REGISTER = 0x2700;
  static const unsigned int TRIGGER_THRESH_DAC_CH1 = 0x2800;
  static const unsigned int TRIGGER_THRESH_DAC_CH2 = 0x2900;
  static const unsigned int TRIGGER_THRESH_DAC_CH3 = 0x2A00;
  static const unsigned int TRIGGER_THRESH_DAC_CH4 = 0x2B00;
  static const unsigned int EEPROM_WRITE = 0x2C00;
  static const unsigned int EEPROM_POLL = 0x2D00;
  static const unsigned int EEPROM_READ = 0x2E00;
  //2F not mentioned
  static const unsigned int POST_STOP_LATENCY = 0x3000;      
  static const unsigned int POST_LATENCY_PRETRIG = 0x3100;
  //32+33 not mentioned
  static const unsigned int NB_OF_CHANNELS = 0x3400;
  //35-37 not mentioned
  static const unsigned int RATE_REG = 0x3800;
  static const unsigned int TRIG_COUNT_LSB = 0x3900;
  static const unsigned int TRIG_COUNT_MSB = 0x3A00;
  static const unsigned int TRIG_RATE_LSB = 0x3B00;
  static const unsigned int TRIG_RATE_MSB = 0x3C00;
  static const unsigned int TRIG_COUNT_RATE = 0x3D00;
  //FF used, but not sensible here 
  //}

#endif
