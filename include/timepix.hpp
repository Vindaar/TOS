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

class Timepix{
	public:
		Timepix();
       int GetFSR(unsigned char* FSR_);
       int ChipID(unsigned char* ReplyPacket,unsigned short chip);
       int PackMatrix(std::vector<std::vector<unsigned char> > *PackQueue);
		int SaveFSRToFile(const char* filename, unsigned short chip);
		int LoadFSRFromFile(const char* filename, unsigned short chip);
		int SaveThresholdToFile(const char* filename, unsigned short chip);
		int LoadThresholdFromFile(const char* filename, unsigned short chip);
		int SaveMatrixToFile(const char* filename, unsigned short Chip);
		int LoadMatrixFromFile(const char* filename, unsigned short Chip);
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
		unsigned short SetFADCshutter(unsigned short FADCshutter);
		unsigned short GetFADCshutter();
		unsigned short SetExtraByte(unsigned short ExtraByte);
		unsigned short GetExtraByte();
		unsigned short SetFADCtriggered(unsigned short FADCtriggered);
		unsigned short SetFADCtriggered();

	private:
		void UpdateFSR();
		
		unsigned char FSR[9][52];
		
		// DAC-Werte
		unsigned int DACValue[9][18];
		std::string DACNames[18];
		int DACCodes[13];
		int ChipID_[9];
		std::string ChipLetter[9];
		int ChipNumber[9];
		int ChipWaver[9];
		int ChipType_[9];
		int NumberDefPixel;
		int DefPixel[256][256];
		int IsCounting_;
		
		// Bitpositionen im FSR
		int IKrumBitPos[8], DiscBitPos[8], PreampBitPos[8], BuffAnalogABitPos[8], BuffAnalogBBitPos[8], HistBitPos[8];
		int THLBitPos[10], CoarseBitPos[4];
		int VcasBitPos[8], FBKBitPos[8], GNDBitPos[8], CTPRBitPos[32], THSBitPos[8], BiasLVDSBitPos[8], RefLVDSBitPos[8], ChipIDBitPos[24];
		int SenseDACPos, ExtDACPos, DACCodePos[4];
		
		// Setup der einzelnen Pixel
		unsigned char P0[9][256][256], P1[9][256][256], Mask[9][256][256], Test[9][256][256], ThrH[9][256][256];
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
};
