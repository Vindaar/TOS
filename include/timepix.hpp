/**********************************************************************/
/*                                                        timepix.hpp */
/*  TOS - Timepix Operating Software                                  */
/*                                                                    */
/*                                                         10.07.2009 */
/*                                                    Christian Kahra */
/*                                     chrkahra@students.uni-mainz.de */
/*                                        Institut fuer Physik - ETAP */
/*                              Johannes-Gutenberg Universitaet Mainz */
/**********************************************************************/

#include "header.hpp"
#include <string>


// some macros for the default chip ID offsets (simply 'what works usually')
#define DEFAULT_CHIP_ID_OFFSET_1_CHIP               188
#define DEFAULT_CHIP_ID_OFFSET_3_CHIPS              189
#define DEFAULT_CHIP_ID_OFFSET_5_CHIPS              190
#define DEFAULT_CHIP_ID_OFFSET_7_CHIPS              192
#define DEFAULT_CHIP_ID_OFFSET_7_CHIPS_AT_CERN      191
#define DEFAULT_CHIP_ID_OFFSET_8_CHIPS              193
// define some macros for the size of arrays
#define DEFAULT_FSR_SIZE                            52
#define DEFAULT_NUM_DACS                            18
#define DEFAULT_NUM_DAC_CODES                       13
#define DEFAULT_MAX_NUM_CHIPS                       9


class Timepix{
public:
    Timepix(int nbOfChips);
    int GetFSR(unsigned char* FSR_);
    int ChipID(unsigned char* ReplyPacket,unsigned short chip);
    int PackMatrix(std::vector<std::vector<unsigned char> > *PackQueue);
    int SaveFSRToFile(std::string filename, unsigned short chip);
    int LoadFSRFromFile(std::string filename, unsigned short chip);
    int SaveThresholdToFile(std::string filename, unsigned short chip);
    int LoadThresholdFromFile(std::string filename, unsigned short chip);
    int SaveMatrixToFile(std::string filename, unsigned short Chip);
    int LoadMatrixFromFile(std::string filename, unsigned short Chip);
    int UniformMatrix(unsigned char P0, unsigned char P1, unsigned char Mask, unsigned char Test, unsigned char ThrH, unsigned short Chip);
    int VarChessMatrix(int sl,int wl,int sp0,int sp1,int smask,int stest,int sth,int wp0,int wp1,int wmask,int wtest,int wth, unsigned short Chip);
		
    bool SetChipID(int id, std::string IDLetter,int IDNumber, int IDWaver, int chipType, unsigned short chip);
    int GetChipID(unsigned short chip);
    int GetChipIDsilent(unsigned short chip);
    int SetDAC(unsigned int dac, unsigned short chip, unsigned int value);
    unsigned int GetDAC(unsigned int dac, unsigned short chip);
    int MaskPixel(int x, int y, int m, unsigned short chip);
    int GetMask(int y, int x, unsigned short chip);
    int Spacing (unsigned int space, unsigned int step, unsigned short chip);
    int Spacing_row (unsigned int step, unsigned int pix_per_row, unsigned short chip);
    int Spacing_row_TPulse (unsigned int step, unsigned int pix_per_row, unsigned short chip);
    int SpacingCalib (unsigned int stepCTPR, unsigned int stepRow, unsigned int pix_per_row, unsigned short chip);
		
    void SetFSRBit(int bit, bool b);
		
    int IsCounting();	
    int SetCounting(int counting);
    std::string GetDACName(unsigned int dac);
    int GetDACCode(unsigned int dac);
    int SetNumChips(unsigned short Chips, unsigned short preload);
    int SetOption(unsigned short option);
    unsigned short GetNumChips();
    unsigned short GetPreload();
    unsigned short GetOption();
    int SetI2C(unsigned int i2c_val);
    unsigned int GetI2C();
    int SetADCresult(unsigned short ADC_ChAlert, unsigned short ADC_result);
    int SetADCresult(int i2cresult);
    int GetI2cResult();
    unsigned short GetADCresult();
    unsigned short GetADCchannel();
    unsigned short GetADCalert();
    void SetFADCshutter(unsigned short FADCshutter);
    unsigned short GetFADCshutter();
    void SetExtraByte(unsigned short ExtraByte);
    unsigned short GetExtraByte();
    void SetFADCtriggered(unsigned short FADCtriggered);
    unsigned short GetFADCtriggered();

    // helper functions
    std::string GetChipName(unsigned short chip);
    int GetPixelsPerDimension();
    int GetTotalNumPixels();




    // function corresponding to _chipIDOffset to set array ChipIDBitPos
    // with a new offset
    void SetChipIDOffset(int ChipIdOffset);    

private:
    void UpdateFSR();
		
    unsigned char FSR[DEFAULT_MAX_NUM_CHIPS][DEFAULT_FSR_SIZE];
		
    // DAC-Werte
    unsigned int DACValue[DEFAULT_MAX_NUM_CHIPS][DEFAULT_NUM_DACS];
    std::string DACNames[DEFAULT_NUM_DACS];
    int DACCodes[DEFAULT_NUM_DAC_CODES];
    int ChipID_[DEFAULT_MAX_NUM_CHIPS];
    std::string ChipLetter[DEFAULT_MAX_NUM_CHIPS];
    int ChipNumber[DEFAULT_MAX_NUM_CHIPS];
    int ChipWaver[DEFAULT_MAX_NUM_CHIPS];
    int ChipType_[DEFAULT_MAX_NUM_CHIPS];
    int NumberDefPixel;
    int **DefPixel[256][256];
    int IsCounting_;

    // chip ID offset (offset for bitstream of chip ID in header of communication)
    int _chipIdOffset;
		
    // Bitpositionen im FSR
    int IKrumBitPos[8], DiscBitPos[8], PreampBitPos[8], BuffAnalogABitPos[8], BuffAnalogBBitPos[8], HistBitPos[8];
    int THLBitPos[10], CoarseBitPos[4];
    int VcasBitPos[8], FBKBitPos[8], GNDBitPos[8], CTPRBitPos[32], THSBitPos[8], BiasLVDSBitPos[8], RefLVDSBitPos[8], ChipIDBitPos[24];
    int SenseDACPos, ExtDACPos, DACCodePos[4];
		
    // Setup der einzelnen Pixel
    unsigned char P0[DEFAULT_MAX_NUM_CHIPS][256][256];
    unsigned char P1[DEFAULT_MAX_NUM_CHIPS][256][256]; 
    unsigned char Mask[DEFAULT_MAX_NUM_CHIPS][256][256]; 
    unsigned char Test[DEFAULT_MAX_NUM_CHIPS][256][256]; 
    unsigned char ThrH[DEFAULT_MAX_NUM_CHIPS][256][256];
    unsigned short NumChips;
    unsigned short Preload_global;
    unsigned short Option_global;
    unsigned int I2C_global;
    int i2c_result_global;
    unsigned short ADC_channel_global;
    unsigned short ADC_alert_global;
    unsigned short ADC_result_global;
    unsigned short FADCshutter_global;
    unsigned short FADCtriggered_global;
    unsigned short ExtraByte_global;

    // helpful member variables
    int _pix_per_dimension;
    int _pix_total_num;

};
