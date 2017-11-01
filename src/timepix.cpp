/**********************************************************************/
/*                                                        timepix.cpp */
/*  TOS - Timepix Operating Software                                  */
/*                                                                    */
/*                                                         10.07.2009 */
/*                                                    Christian Kahra */
/*                                     chrkahra@students.uni-mainz.de */
/*                                        Institut fuer Physik - ETAP */
/*                              Johannes-Gutenberg Universitaet Mainz */
/**********************************************************************/

#include "timepix.hpp"

#include <iostream>
#include <fstream>


Timepix::Timepix(unsigned short nbOfChips){
#if DEBUG == 2
    std::cout<<"Enter Timepix::Timepix()"<<std::endl;	
#endif

    // use constructor argument to set the number of chips
    _nbOfChips = nbOfChips;
    
    // set the variables for pixel properties
    _pix_per_dimension = PIXPD;
    _pix_total_num = PIXPD * PIXPD;

    // temporary integer variable for number of chips to set chip ID to a
    // sensible value, if the current number is 'standard'
    switch(_nbOfChips)
    {
	case 1:
	    _chipIdOffset = DEFAULT_CHIP_ID_OFFSET_1_CHIP;
	    break;
	case 3:
	    _chipIdOffset = DEFAULT_CHIP_ID_OFFSET_3_CHIPS;
	    break;
	case 5:
	    _chipIdOffset = DEFAULT_CHIP_ID_OFFSET_5_CHIPS;
	    break;
	case 7:
	    _chipIdOffset = DEFAULT_CHIP_ID_OFFSET_7_CHIPS_AT_CERN;
	    break;
	case 8:
	    _chipIdOffset = DEFAULT_CHIP_ID_OFFSET_8_CHIPS;
	    break;
	default:
	    // in case we have a different number of chips, set it
	    std::cout << "Number of chips neither 1, 7 nor 8.\n"
		      << "Setting ChipIdOffset to default value for 1 chip"
		      << std::endl;
	    _chipIdOffset = DEFAULT_CHIP_ID_OFFSET_1_CHIP;
    }
    std::cout << "Chip ID offset being set to " << _chipIdOffset << std::endl;

    int x,y;
	
    DACCodes[0]=15;	DACNames[0]="IKrum";
    DACCodes[1]=11;	DACNames[1]="Disc";
    DACCodes[2]=7;	DACNames[2]="Preamp";
    DACCodes[3]=3;	DACNames[3]="BuffAnalogA";
    DACCodes[4]=4;	DACNames[4]="BuffAnalogB";
    DACCodes[5]=9;	DACNames[5]="Hist";
    DACCodes[6]=6;	DACNames[6]="THL";
    DACCodes[7]=12;	DACNames[7]="Vcas";
    DACCodes[8]=10;	DACNames[8]="FBK";
    DACCodes[9]=13;     DACNames[9]="GND";
    DACCodes[10]=1;	DACNames[10]="THS";
    DACCodes[11]=2;	DACNames[11]="BiasLVDS";
    DACCodes[12]=14;    DACNames[12]="RefLVDS";
    DACNames[13]="Coarse";
    DACNames[14]="CTPR";
    DACNames[15]="SenseDAC";
    DACNames[16]="ExtDAC";
    DACNames[17]="DACCode";
	
    // LSB: x=0
    SenseDACPos = 255 - 43 - 0;
    ExtDACPos   = 255 - 44 - 0;	
    for(x=0;x<8;x++){
	IKrumBitPos[x]=	   255-(   4+x) +2 ;
	DiscBitPos[x]=	   255-(  12+x) -0;
	PreampBitPos[x]=   255-(  20+x) -0;
	BuffAnalogBBitPos[x]=255-(60+x) -0;
	HistBitPos[x]=	   255-(  86+x) -0;
	VcasBitPos[x]=	   255-( 120+x) -0;
	FBKBitPos[x]=	   255-( 128+x) -0;
	GNDBitPos[x]=	   255-( 136+x) -0;
	THSBitPos[x]=	   255-( 181+x) -0;
	BiasLVDSBitPos[x]= 255-( 227+x) -0;
	RefLVDSBitPos[x]=  255-( 235+x) -0;
    }
    for(x=0;x<4;x++){ 
	BuffAnalogABitPos[x]=	255-( 46 +x)  -0;
	BuffAnalogABitPos[x+4]= 255-( 56 +x)  -0;
	CoarseBitPos[x]=	255-(110 +x)  -0;
    }
    /*IKrumBitPos[0]=   255-(   4)+12; // D2 in FPGA firmware such that clock is slower when setting Ikrum. still not perfect. Ikrum 10 = 5 somehow. DACScan: jump from 3 to 4.
      IKrumBitPos[1]=	   255-(   5)+11;
 IKrumBitPos[2]=	   255-(   6)+10;
      IKrumBitPos[3]=	   255-(   7)+9;
      IKrumBitPos[4]=	   255-(   8)+8;
      IKrumBitPos[5]=	   255-(   9)+7;
      IKrumBitPos[6]=	   255-(   10)+6;
      IKrumBitPos[7]=	   255-(   11)+5; // muss
        
      DiscBitPos[0]=	   255-(  12+0) -0;
      DiscBitPos[1]=	   255-(  12+1) -0;
      DiscBitPos[2]=	   255-(  12+2) -0;
      DiscBitPos[3]=	   255-(  12+3) -0;
      DiscBitPos[4]=	   255-(  12+4) -0;
      DiscBitPos[5]=	   255-(  12+5) -0;
      DiscBitPos[6]=	   255-(  12+6) -0;
      DiscBitPos[7]=	   255-(  12+7) -0;*/

    DACCodePos[0] = 255	- 38 - 0;
    DACCodePos[1] = 255	- 39 - 0;
    DACCodePos[2] = 255	- 41 - 0;
    DACCodePos[3] = 255	- 42 - 0;
	
    for(x=0;x<32;x++){CTPRBitPos[x]  = 255 - ( 144 + x) - 0;}
    for(x=0;x<10;x++){THLBitPos[x]   = 255 - ( 100 + x) - 0;}
    for(x=0;x<24;x++){ChipIDBitPos[x]= 255 - ( _chipIdOffset + x);} 
	
    for (unsigned short i = 0; i < DEFAULT_MAX_NUM_CHIPS; i++){
	ChipID_[i]    = 0;
	ChipLetter[i] = "";
	ChipWaver[i]  = 0;
	ChipNumber[i] = 0;	
	ChipType_[i]  = 0;
    }
    NumberDefPixel=0;
    IsCounting_=0;	
    for (unsigned short chip = 0; chip < 8; chip++){
	for(x = 0; x < 18; x++){
	    DACValue[chip][x]=0;
	}
    }
    for (int i = 0; i < 8; i++){
	for(x = 0; x < 52; x++){
	    FSR[i][x] = 0;
	}
    }
    for (unsigned short chip = 0; chip < 8; chip++){
	for(y = 0; y < 256; y++){
	    for(x = 0; x < 256; x++){
		Mask[chip][y][x] = 0;
		Test[chip][y][x] = 0;
		P0[chip][y][x]   = 0;
		P1[chip][y][x]   = 0;
		ThrH[chip][y][x] = 0;
	    }
	}
    }
	
}

Timepix::~Timepix(){
    // destructor of timepix class
    // nothing to do here
}

int Timepix::GetFSR(unsigned char* FSR_){
#if DEBUG==2
    std::cout<<"Enter Timepix::GetFSR()"<<std::endl;
#endif
    int i;
    UpdateFSR();
    for (auto chip : _chip_set){
        if (chip == 0){
	    for(i = 18; i < 18 + 34; i++){
		FSR_[i] = FSR[chip][i];
	    }
        }
        else {
	    for(i = 19; i < 18 + 34; i++){
		FSR_[i+(32 * chip)] = FSR[chip][i];
	    }
        }
    }
    return 0;
}

int Timepix::LoadFSRFromFile(std::string filename, unsigned short chip){
#if DEBUG==2
    std::cout<<"Enter Timepix::LoadFSRFromFile()"<<std::endl;	
#endif	
    unsigned int i,in;
    std::string s;
    std::fstream f; f.open(filename,std::fstream::in);
    if(!f.is_open()) {return 0;}
	
    for(i=0;i<18;i++){
	f >> s >> in;
	if( (f.fail()) || (s.compare(DACNames[i])!=0) || (SetDAC(i,chip,in)!=1) ) {return (-(i+1));}
    }
    f.close(); UpdateFSR(); 
    return 1;
}

int Timepix::SaveFSRToFile(std::string filename, unsigned short chip){
#if DEBUG==2
    std::cout<<"Enter Timepix::SaveFSRToFile()"<<std::endl;	
#endif
    int i;
    std::fstream f; 
    f.open(filename,std::fstream::out);
    if(!f.is_open()) {return 0;}
    for(i=0;i<18;i++){ 
	f << DACNames[i] << " " << DACValue[chip][i] << std::endl;
	if(f.fail()) {return 0;}
    }
    f.close(); return 1;
}


int Timepix::SetDAC(unsigned int dac, unsigned short chip, unsigned int value){
#if DEBUG==2
    std::cout<<"Enter Timepix::SetDAC()"<<std::endl;	
#endif
    int err = 1;
	
    if(dac > 17){
	// only 17 DACs. If DAC larger than 17 return error code 63 (illegal DAC number)
	err = 63;
    }
    else if( ((	dac ==  6) && (value < 1024)) ||
	     ((	dac == 13) && (value < 16))   ||
	     ( 	dac == 14) ||
	     (((dac == 15) || (dac == 16)) && (value < 2)) ||
	     (( dac == 17) && (value < 16))   ||
	     ( value < 256) ){
	DACValue[chip][dac] = value;
    }
    else{
	// invalid value for this DAC. return error code 60
	err = 60;
    }
    
    return err;
}

unsigned int Timepix::GetDAC(unsigned int dac, unsigned short chip){
#if DEBUG==2
    std::cout<<"Enter Timepix::GetDAC()"<<std::endl;	
#endif	
    if(dac>17) {return -1;}
    return DACValue[chip][dac];
}

int Timepix::MaskPixel(int x, int y, int m, unsigned short chip){ //not used so far
    if(m>1){return Mask[1][y][x];}
    if(Mask[chip][y][x]==m){return -1;}
    else{Mask[chip][y][x]=m; return m;}
}
int Timepix::GetMask(int y, int x, unsigned short chip){
    return Mask[chip][y][x];
}

int Timepix::Spacing(unsigned int space, unsigned int step, unsigned short chip){ //when using step must go from 0 to <space*space
    int y,x = 0;
    for(y=0;y<256;++y){
	for(x=0;x<256;++x){
	    if (x%space==step%space and y%space==step/space){
		Mask[chip][y][x] = 1;
	    }
	    else {
		Mask[chip][y][x] = 0;
	    }

	}
    }
    return true;
}

int Timepix::SpacingCalib (unsigned int stepCTPR, unsigned int stepRow, unsigned int pix_per_row, unsigned short chip){ //only for TOcalib, matrix was set to all test, mask = 0
    int y,x = 0;
    unsigned int delta_pix = 256/pix_per_row;
    for(y=stepRow;y<256;y=y+delta_pix){
	for(x=stepCTPR;x<256;x=x+32){
	    Mask[chip][y][x] = 1;
	    Test[chip][y][x] = 1;
	}
    }
    return true;
}


int Timepix::Spacing_row (unsigned int step,
			  unsigned int pix_per_row,
			  unsigned short chip){
    int val = 0;
    val = Spacing_row(step, pix_per_row, chip, 0);
    return val;
}

int Timepix::Spacing_row (unsigned int step,
			  unsigned int pix_per_row,
			  unsigned short chip,
			  unsigned int spacing_offset){ //step from 0 to <(256/pix_per_col), y are the row
    int y,x = 0;

    unsigned int delta_pix = 256/pix_per_row;

    //Mask[chip][32][32] = 1;
    
    for(y = 0; y < 256; ++y){
	//for(x=0 + spacing_offset; x < 252 ;++x){n
	for(x=0 + spacing_offset; x < 256 ;++x){
	    // std::cout << "printing y, delta_pix, step, mod "
	    // 	      << y << ", " << delta_pix << ", " << step << ", "
	    // 	      << y % delta_pix << std::endl;
	    if ((y % delta_pix == step) ){//&&
		//(x % delta_pix == step)){
		//Mask[chip][y][x+4] = 1;
		Mask[chip][y][x] = 1;
		// std::cout << "pixel y: " << y << " x: " << x << " not masked \t "
		// 	  << spacing_offset
		// 	  << "\t" << x << std::endl;
	    }
	    else {
		//Mask[chip][y][x+4] = 0;
		Mask[chip][y][x] = 0;
	    }
	}
    }
    //std::cout << "printing step, delta_pix " << step << ", " << delta_pix << std::endl;

    return true;
}

int Timepix::Spacing_row_TPulse (unsigned int step,
				 unsigned int pix_per_row,
				 unsigned short chip){

    int val;
    val = Spacing_row_TPulse(step, pix_per_row, chip, 0);
    return val;
}


int Timepix::Spacing_row_TPulse (unsigned int step,
				 unsigned int pix_per_row,
				 unsigned short chip,
				 unsigned int spacing_offset){
    //step from 0 to <(256/pix_per_col), y are the row
    int y,x = 0;
    unsigned int delta_pix = 256/pix_per_row;

    //Test[chip][32][32] = 1;
    for(y=0;y<256;++y){
	for(x = 0 + spacing_offset; x < 256; ++x){
	    if (y % delta_pix == step){// &&
		//(x % delta_pix == step){//){
		Test[chip][y][x] = 1; // one seems to be on. not 0 as explained in Timepix manual
		//std::cout<<"pixel y: "<<y<<" x: "<<x<<" test pulse on"<<std::endl;
	    }
	    else {
		Test[chip][y][x] = 0;
	    }
	}
    }
    return true;
}

int Timepix::IsCounting(){
#if DEBUG==2
    std::cout<<"Enter Timepix::IsCounting()"<<std::endl;	
#endif
    return IsCounting_;
}

int Timepix::SetCounting(int counting){
#if DEBUG==2
    std::cout<<"Enter Timepix::SetCounting()"<<std::endl;	
#endif
    IsCounting_=(counting!=0);
    return IsCounting_;
}

std::string Timepix::GetDACName(unsigned int dac){
#if DEBUG==2
    std::cout<<"Enter Timepix::GetDACName()"<<std::endl;	
#endif	
    if(dac>17){return std::string("failure");}
    return DACNames[dac];
}

int Timepix::GetDACCode(unsigned int dac){
    if(dac>12){return -1;}
    return DACCodes[dac];
}

/* UpdateFSR
// - Die Wertigkeit eines Bits (2^x) wird �ber bitweisen Verschiebungsoperator berechnet 2^x = (1<<x)
//   Alternative dazu w�re Potenzieren per iterativer Multiplikation (for-Schleifen-Konstrukt) - eventuell ausgelagert in eigene Fkt
// - Zur �berpr�fung ob das (2^x)-te Bit gesetzt ist gibt es zum Bitweisen-Und (&) keine "sch�ne" Alternative
*/

void Timepix::UpdateFSR(){
#if DEBUG==2
    std::cout<<"Enter Timepix::UpdateFSR()"<<std::endl;	
    // output dacs for debugging
    for(int j = 0; j < GetNumChips(); j++){
    	for(int i=0;i<18;++i){
    	    std::cout<<DACNames[j][i]<<DACValue[j][i]<<std::endl;
    	}
    }
#endif
    for (int i = 0; i < 8; i++){
	for(int j = 0; j < 18 + 34; ++j){
	    FSR[i][j]=0;
	}
    }
    for (auto chip : _chip_set){
	FSR[chip][18]=0xff;
	//FSR[chip][19]=0x80;
	for(int i = 0; i < 8; ++i){
	    if( DACValue[chip][0] & ( 1 << i ) ) //pr�fe ob (2^x)-te Bit gesetzt ist durch Bitweises-Und mit einer "bin�ren 1 mit i angeh�ngten Nullen"
		FSR[chip][18+1+IKrumBitPos[i]/8] += ( 1<<(7-IKrumBitPos[i]%8) ); // addiere zum Byte die Wertigkeit des aktuellen FSR-Bits
	    if( DACValue[chip][1] & (1<<i) ) FSR[chip][18+1+ DiscBitPos[i]/8]	 += 1<< (7-DiscBitPos[i]%8);
	    if( DACValue[chip][2] & (1<<i) ) FSR[chip][18+1+ PreampBitPos[i]/8]	 += 1<< (7-PreampBitPos[i]%8);
	    if( DACValue[chip][3] & (1<<i) ) FSR[chip][18+1+ BuffAnalogABitPos[i]/8] += 1<< (7-BuffAnalogABitPos[i]%8);
	    if( DACValue[chip][4] & (1<<i) ) FSR[chip][18+1+ BuffAnalogBBitPos[i]/8] += 1<< (7-BuffAnalogBBitPos[i]%8);
	    if( DACValue[chip][5] & (1<<i) ) FSR[chip][18+1+ HistBitPos[i]/8]	 += 1<< (7-HistBitPos[i]%8);
	    if( DACValue[chip][7] & (1<<i) ) FSR[chip][18+1+ VcasBitPos[i]/8]	 += 1<< (7-VcasBitPos[i]%8);
	    if( DACValue[chip][8] & (1<<i) ) FSR[chip][18+1+ FBKBitPos[i]/8]	 += 1<< (7-FBKBitPos[i]%8);
	    if( DACValue[chip][9] & (1<<i) ) FSR[chip][18+1+ GNDBitPos[i]/8]	 += 1<< (7-GNDBitPos[i]%8);
	    if( DACValue[chip][10]& (1<<i) ) FSR[chip][18+1+ THSBitPos[i]/8]	 += 1<< (7-THSBitPos[i]%8);
	    if( DACValue[chip][11]& (1<<i) ) FSR[chip][18+1+ BiasLVDSBitPos[i]/8]	 += 1<< (7-BiasLVDSBitPos[i]%8);
	    if( DACValue[chip][12]& (1<<i) ) FSR[chip][18+1+ RefLVDSBitPos[i]/8] 	 += 1<< (7-RefLVDSBitPos[i]%8);
	}
	if( DACValue[chip][15] ) FSR[chip][18+1+ SenseDACPos/8] += 1<< (7-SenseDACPos%8);
	if( DACValue[chip][16] ) FSR[chip][18+1+ ExtDACPos/8] 	+= 1<< (7-ExtDACPos%8);
	for(int i = 0; i < 10; ++i){
	    if( DACValue[chip][6] & (1 << i) ) FSR[chip][18+1+THLBitPos[i]/8] += 1<<(7-THLBitPos[i]%8);
	}
	//FSR[18+1+ DACCodePos[3] /8] = 3 ;
	for(int i = 0; i < 4; ++i){
	    if( DACValue[chip][13] & (1 << i) ) FSR[chip][18+1+CoarseBitPos[i]/8] += 1<<(7-CoarseBitPos[i]%8);
	    if( DACValue[chip][17] & (1 << i) ) FSR[chip][18+1+ DACCodePos[i]/8]  += 1<<(7-DACCodePos[i]%8);
	}
			
	//FSR[18-2+ DACCodePos[1] /8] = 1 ;
	for(int i = 0; i < 32; ++i){
	    if(DACValue[chip][14] & (1<<i) ) FSR[chip][18+1+CTPRBitPos[i]/8] += 1<<(7-CTPRBitPos[i]%8);
	}
    }
}

void Timepix::SetFSRBit(int bit, bool b){
    //int i,j;
    for (auto chip : _chip_set){
	if( (FSR[chip][18+1+bit/8] & (1<<(7-bit%8))) && b==false ){FSR[chip][18+1+bit/8]-=(1<<(7-bit%8));}
	if( ((FSR[chip][18+1+bit/8] & (1<<(7-bit%8)))==0) && b==true ){FSR[chip][18+1+bit/8]+=(1<<(7-bit%8));}
	std::cout<<"this has been done for all chips!"<<std::endl;
    }
    //for(i=18+1;i<18+34;
}
int Timepix::ChipID(unsigned char* ReplyPacket, unsigned short chip){
#if DEBUG==2
    std::cout<<"Enter Timepix::ChipID()"<<std::endl;
#endif
    int i;
    int ID=0;
    int IDLetterNum = 0;
    int IDNumber = 0;
    int IDWaver = 0;
    int chipType = 0;
    int chipId= 0;
    std::string IDLetter = "";
    chipId=ReplyPacket[24] << 16;
    chipId+=ReplyPacket[25] << 8;
    chipId+=ReplyPacket[26];

    for(i = 0; i < 24; i++){
        if((ReplyPacket[18+(32*(chip))+ (ChipIDBitPos[i]+9*(_nbOfChips - 1))/8] & (1<<(7-(ChipIDBitPos[i]+9*(_nbOfChips - 1))%8)))>0) ID+=1<<i;
    }
    if((ReplyPacket[18+(32*(chip))+ (ChipIDBitPos[0]+9*(_nbOfChips - 1))/8] & (1<<(7-(ChipIDBitPos[0]+9*(_nbOfChips - 1))%8)))>0) chipType+=1<<0;
    for(i = 1; i < 12; i++){
        if((ReplyPacket[18+(32*(chip))+ (ChipIDBitPos[12-i]+9*(_nbOfChips - 1))/8] & (1<<(7-(ChipIDBitPos[12-i]+9*(_nbOfChips - 1))%8)))>0) IDWaver+=1<<(i-1);
    }
    for(i = 12; i < 16; i++){
        if((ReplyPacket[18+(32*(chip))+ (ChipIDBitPos[27-i]+9*(_nbOfChips - 1))/8] & (1<<(7-(ChipIDBitPos[27-i]+9*(_nbOfChips - 1))%8)))>0) IDNumber+=1<<(i-12);
    }
    for(i = 16; i < 21; i++){
        if((ReplyPacket[18+(32*(chip))+ (ChipIDBitPos[36-i]+9*(_nbOfChips - 1))/8] & (1<<(7-(ChipIDBitPos[36-i]+9*(_nbOfChips - 1))%8)))>0) IDLetterNum+=1<<(i-17);
    }

    if (IDLetterNum == 0) IDLetter = "0";
    else if (IDLetterNum == 1) IDLetter = "A";
    else if (IDLetterNum == 2) IDLetter = "B";
    else if (IDLetterNum == 3) IDLetter = "C";
    else if (IDLetterNum == 4) IDLetter = "D";
    else if (IDLetterNum == 5) IDLetter = "E";
    else if (IDLetterNum == 6) IDLetter = "F";
    else if (IDLetterNum == 7) IDLetter = "G";
    else if (IDLetterNum == 8) IDLetter = "H";
    else if (IDLetterNum == 9) IDLetter = "I";
    else if (IDLetterNum == 10) IDLetter = "J";
    else if (IDLetterNum == 11) IDLetter = "K";
    else if (IDLetterNum == 12) IDLetter = "L";
    else if (IDLetterNum == 13) IDLetter = "M";
    else if (IDLetterNum == 14) IDLetter = "N";
    else if (IDLetterNum == 15) IDLetter = "O";
    else IDLetter = "0";


    SetChipID(ID,IDLetter,(12-IDNumber),IDWaver,chipType,chip);
    //std::cout<<"ChipID of chip "<< chip<<" is "<<ID<<". This is "<<IDLetter<<" "<< (12-IDNumber)<<" W" <<IDWaver<<", Chip tpye (1 = Timepix):"<<chipType<<std::endl;
    if(ChipID_[chip]==0){ChipID_[chip]=ID; return 0;}
    else{
        if(ChipID_[chip]==ID){return 0;}
        else{return ID;}
    }
}

bool Timepix::SetChipID(int id, std::string IDLetter, int IDNumber, int IDWaver, int chipType, unsigned short chip){
    if(id<16777216){
	ChipID_[chip]    = id;
	ChipLetter[chip] = IDLetter;
	ChipNumber[chip] = IDNumber;
	ChipWaver[chip]  = IDWaver;
	ChipType_[chip]  = chipType;
	return true;
    }
    else{
	return false;
    }
}

int Timepix::GetChipID(unsigned short chip){
    std::cout << "ChipID of chip " << chip
	      << " is "            << ChipID_[chip]
	      << ". This is "      << ChipLetter[chip]
	      << " "               << ChipNumber[chip]
	      << " W"              << ChipWaver[chip]
	      << ", Chip type (1 = Timepix):"
	      << ChipType_[chip]
	      << std::endl;
    return ChipID_[chip];
}

int Timepix::GetChipIDsilent(unsigned short chip){
    return ChipID_[chip];
}

std::string Timepix::GetChipName(unsigned short chip){
    // this function builds the chip name from the chip letter, chip number and chip wafer
    // NOTE: this function only returns the name that was read from the chip 
    // during the last WriteReadFSR command! If something goes wrong with the chip during
    // operation, this function is NOT (!) a good way to check this!
    
    std::string chipName;
    chipName = ChipLetter[chip] + " " + std::to_string(ChipNumber[chip]) + " W" + std::to_string(ChipWaver[chip]);
    
    return chipName;    
}

int Timepix::GetPixelsPerDimension(){
    // this function returns the member variable _pix_per_dimension,
    // which simply stores the number of pixels the timepix chip has
    // per dimension

    return _pix_per_dimension;
}

int Timepix::GetTotalNumPixels(){
    // this function returns the member variable _pix_total_num, 
    // which simply stores the total number of pixels the timepix chip has
    
    return _pix_total_num;
}

int Timepix::SaveThresholdToFile(std::string filename, unsigned short chip){
#if DEBUG==2
    std::cout<<"Enter Timepix::SaveThresholdToFile()"<<std::endl;	
#endif	
    int x,y;
    FILE* f=fopen(filename.c_str(), "w"); if(f==NULL) return -1;
    for(y=0;y<256;++y){
	for(x=0;x<255;++x){
	    fprintf(f,"%i ",ThrH[chip][y][x]);
	}
	fprintf(f,"%i\n",ThrH[chip][y][255]);
    }
    fclose(f);
    return 1;
}

int Timepix::LoadThresholdFromFile(std::string filename, unsigned short chip){
#if DEBUG==2
    std::cout<<"Enter Timepix::LoadMatrixFromFile()"<<std::endl;	
#endif
    int x,y;
    int in;
    FILE* f=fopen(filename.c_str(), "r"); if(f==NULL) return -1;
    for(y=0;y<256;++y)for(x=0;x<256;++x){
	    if(fscanf(f,"%i",&in)!=1) return (y*255+x);
	    ThrH[chip][y][x]=in;
	}
    fclose(f);
    return -2;
}

int Timepix::SaveMatrixToFile(std::string filename, unsigned short Chip){
#if DEBUG==2
    std::cout<<"Enter Timepix::SaveMatrixToFile()"<<std::endl;	
#endif
    int out;
    int x,y;
    FILE* f=fopen(filename.c_str(), "w"); if(f==NULL) return -1;
    for(y=0;y<256;++y){
	for(x=0;x<255;++x){
	    out=ThrH[Chip][y][x] + 16*P0[Chip][y][x] + 32*P1[Chip][y][x] + 64*Mask[Chip][y][x] + 128*Test[Chip][y][x];
	    fprintf(f,"%i ",out);
	}
	out=ThrH[Chip][y][255] + 16*P0[Chip][y][255] + 32*P1[Chip][y][255] + 64*Mask[Chip][y][255] + 128*Test[Chip][y][255];
	fprintf(f,"%i\n",out);
    }
    fclose(f);
    return 1;
}

int Timepix::LoadMatrixFromFile(std::string filename, unsigned short chip){
#if DEBUG==2
    std::cout<<"Enter Timepix::LoadMatrixFromFile()"<<std::endl;	
#endif
    int in;
    int x,y;
    FILE* f=fopen(filename.c_str(), "r"); if(f==NULL) return -1;
    for(y=0;y<256;++y){
	for(x=0;x<256;++x){
	    if( fscanf(f, "%i", &in) != 1) return (y*255+x);
	    ThrH[chip][y][x] =    in & 15;
	    P0[chip][y][x]   = ( (in & 16)  > 0); 
	    P1[chip][y][x]   = ( (in & 32)  > 0); 
	    Mask[chip][y][x] = ( (in & 64)  > 0); 
	    Test[chip][y][x] = ( (in & 128) > 0);
	}
    }
    fclose(f);
    return 0;
}

int Timepix::UniformMatrix(unsigned char P0_, unsigned char P1_, unsigned char Mask_, unsigned char Test_, unsigned char ThrH_, unsigned short Chip){
    //std::cout<<"Set values for chip number "<<Chip<<std::endl;
    int x,y,res;
    res=((P0_>1)+(P1_>1)+(Mask_>1)+(Test_>1)+(ThrH_>15));
    if(res>0){return res;}
    for(y=0;y<256;y++){
	for(x=0;x<256;x++){
	    P0[Chip][y][x]=P0_;
	    P1[Chip][y][x]=P1_;
	    Mask[Chip][y][x]=Mask_;
	    Test[Chip][y][x]=Test_;
	    ThrH[Chip][y][x]=ThrH_;
	}
    }
    return 0;
}

int Timepix::VarChessMatrix(int chip, std::map<std::string, int> parameter_map){
    // this function overloads the VarChessMatrix() function by enabling the use
    // of a map, which stores all parameters to be used for a chess matrix
    
    int length;
    int width;
    int black_p0;
    int black_p1;
    int black_mask;
    int black_test;
    int black_thr;
    int white_p0;
    int white_p1;
    int white_mask;
    int white_test;
    int white_thr;
    int err;

    length     = parameter_map["length"];
    width      = parameter_map["width"];
    black_p0   = parameter_map["black_p0"];
    black_p1   = parameter_map["black_p1"];
    black_mask = parameter_map["black_mask"];
    black_test = parameter_map["black_test"];
    black_thr  = parameter_map["black_thr"];
    white_p0   = parameter_map["white_p0"];
    white_p1   = parameter_map["white_p1"];
    white_mask = parameter_map["white_mask"];
    white_test = parameter_map["white_test"];
    white_thr  = parameter_map["white_thr"];

    err = VarChessMatrix(length,
			 width,
			 black_p0,
			 black_p1,
			 black_mask,
			 black_test,
			 black_thr,
			 white_p0,
			 white_p1,
			 white_mask,
			 white_test,
			 white_thr,
			 chip);

    return err;
}

int Timepix::VarChessMatrix(int sl,int wl,int sp0,int sp1,int smask,int stest,int sth,int wp0,int wp1,int wmask,int wtest,int wth, unsigned short Chip){
    int x,y;
    int xnor1, xnor2;
    for(y=0;y<256;y++){
	for(x=0;x<256;x++){
	    //xnor1=0; xnor2=1;
	    xnor1=(x%(2*sl))>=sl;
	    xnor2=(y%(2*wl))>=wl;
	    //if((x==255)&&(y==0)){printf("xnor1=%i,xnor2=%i\n",xnor1,xnor2);}
	    if((xnor1 && xnor2) || (!xnor1 && !xnor2)) {
				
		ThrH[Chip][y][x]=sth;
		P0[Chip][y][x]=sp0;
		P1[Chip][y][x]=sp1;
		Mask[Chip][y][x]=smask;
		Test[Chip][y][x]=stest;
	    }
	    else{
		ThrH[Chip][y][x]=wth;
		P0[Chip][y][x]=wp0;
		P1[Chip][y][x]=wp1;
		Mask[Chip][y][x]=wmask;
		Test[Chip][y][x]=wtest;
	    }
	}
    }
    return 0;
}

FrameArray<int> Timepix::GetMatrixAsInts(int chip){
    // this function returns a FrameArray for the matrix currently set
    // such that the values correspond to the integer values, which will be
    // written to the pixels based on their 14bit register
    // inputs:
    //     int chip: the chip for which to return the frame array
    FrameArray<int> matrix;

    int p0   = 0;
    int p1   = 0;
    int mask = 0;
    int test = 0;
    int thr  = 0;
    int pix_value = 0;
    for(int x = 0; x < PIXPD; x++){
	for(int y = 0; y < PIXPD; y++){
	    // careful, all internal arrays are addressed as
	    // [y][x] instead of [x][y] as FrameArray!
	    p0 	 = P0[chip][y][x];
	    p1 	 = P1[chip][y][x];
	    mask = Mask[chip][y][x];
	    test = Test[chip][y][x];
	    thr  = ThrH[chip][y][x];
	    // using the current values get the corresponding integer
	    // value and assign to matrix
	    pix_value = CalcPixelValueFrom14Bit(p0, p1, mask, test, thr);
	    matrix[x][y] = pix_value;
	}
    }
    // and return the matrix
    return matrix;
}

int Timepix::CalcPixelValueFrom14Bit(int p0, int p1, int mask, int test, int thr){
    // this function calculates the value a pixel should have after writing
    // the 14 bit register with certain P0, P1, Mask, Test, Threshold values
    // inputs:
    //     the properties to be set for individual pixels

    // the 14 bit register on the chip looks as follows:
    // bits:   0   1   2   3   4   5   6    7      8      9    10     11     12     13 
    //         x   x   x   x   x   x   p1   mask   thr0   p0   thr2   thr3   thr1   test
    int value = 0;

    // first separate the threshold int into 4 bits (use bool to represent bit)
    // (bool == false <--> bit == 0)
    bool thr0 = false;
    bool thr1 = false;
    bool thr2 = false;
    bool thr3 = false;
    // bitwise AND between threshold int and specific bits
    // done by 1 << n, where n results in bit being compared
    // if both 1, result is true, else false
    thr0 = thr & (1 << 0);
    thr1 = thr & (1 << 1);
    thr2 = thr & (1 << 2);
    thr3 = thr & (1 << 3);
    
    // now add each bit to the return value
    value += p1   << 6;
    value += mask << 7;
    value += thr0 << 8;
    value += p0   << 9;
    value += thr2 << 10;
    value += thr3 << 11;
    value += thr1 << 12;
    value += test << 13;
    
    return value;
}

int Timepix::PackMatrix(std::vector<std::vector<unsigned char> > *PackQueue){
#if DEBUG==2
    std::cout<<"Enter Timepix::PackMatrix()"<<std::endl;
#endif

    int x,y;
    int aktBit; //aktuelles Bit von Matrix 256Spalten * 256Zeilen *14Bit + 8BitPreload

    //std::cout << "Set Matrix to 0"<< std::endl;
    for(y = 0; y < PQueue * _nbOfChips; ++y){
	for(x = 0; x < PLen + 18; ++x){
	    (*PackQueue)[y][x]=0;
	}
    }
    //std::cout << "Matrix set to 0"<< std::endl;
    //for (auto chip : _chip_set){
    for (size_t i = 0; i < _nbOfChips; i++){
	aktBit=0;//(917768*(chip)); preload scheint nicht da zu sein fuer chip 2... daher auch nur 917760 fuer die aktBit
	(*PackQueue)[(aktBit/8)/PLen][18+((aktBit/8)%PLen)]=0xFF; // 0xff = 255 preload
    }

    for (auto chip : _chip_set){
        for(y=0;y<256;++y) {
	    for(x=0;x<256;++x){
		int aktBit_base = y*256*14 + 8 + (255 - x) + (917760 * chip);
		if(Test[chip][y][x])	{
		    // yZeilen * 256Pixel/Zeile * 14Bit/Pixel + 8Bit Preload + aktuelles Bit in Zeile
		    aktBit=aktBit_base;
		    SetPackageByte(aktBit, PackQueue);
		}
		if(2&ThrH[chip][y][x]) {
		    aktBit=256+aktBit_base;
		    SetPackageByte(aktBit, PackQueue);
		}
		if(8&ThrH[chip][y][x]) {
		    aktBit=2*256+aktBit_base;
		    SetPackageByte(aktBit, PackQueue);
		}
		if(4&ThrH[chip][y][x]) {
		    aktBit=3*256+aktBit_base;
		    SetPackageByte(aktBit, PackQueue);
		}
		if(P0[chip][y][x])     {
		    aktBit=4*256+aktBit_base;
		    SetPackageByte(aktBit, PackQueue);
		}
		if(1&ThrH[chip][y][x]) {
		    aktBit=5*256+aktBit_base;
		    SetPackageByte(aktBit, PackQueue);
		}
		if(Mask[chip][y][x])   {
		    aktBit=6*256+aktBit_base;
		    SetPackageByte(aktBit, PackQueue);
		}
		if(P1[chip][y][x])     {
		    aktBit=7*256+aktBit_base;
		    SetPackageByte(aktBit, PackQueue);
		}
	    }
	}
    }
    for (auto chip : _chip_set){
	for(x = 0; x < 32; x++){
	    // NOTE: the following line is completely broken. 
	    (*PackQueue)[PQueue*(chip+1) - 1][18+1+(32 * chip)+((((256*256*14)*(chip + 1))/8)%PLen)+x] = 0xff;
	    // shouldn't it be:
	    // (*PackQueue)[PQueue * chip][18+1+(32 * chip)+((((256*256*14)*chip)/8)%PLen)+x] = 0xff;
	    // based on previous (when chip 0 was still chip 1):
	    // (*PackQueue)[PQueue*chip-1][18+1+(32*(chip-1))+((((256*256*14)*(chip))/8)%PLen)+x] = 0xff;}
	
	}//postload
    }
    return 0;
}

void Timepix::SetPackageByte(int aktBit, std::vector<std::vector<unsigned char> > *PackQueue){
    // this function sets an individual byte in the package queue
    // inputs:
    //     int aktBit: the current bit (byte?!) we're currently setting
    //     std::vector... *PackQueue: the pointer to the vector object, which stores the
    //                 package. We change this in place!
    
    // [aktBit/(8*PLen)]=Paket des aktuellen Bits,
    // [18+PacketBit/8]=PaketHeader+BytePosition im Paket,
    // (1<<(7-PacketBit%8))=Wertigkeit des aktuellen Bits in seinem Byte (1<<x = 2^x)
    int package = (aktBit/8)/PLen;
    int pos_in_package = 18+((aktBit/8)%PLen);
    (*PackQueue)[package][pos_in_package] += 1<<(7-(aktBit%8));
}

int Timepix::SetPreload(unsigned short preload){
    Preload_global = preload;
    return true;
}

int Timepix::SetNumChips(unsigned short Chips,unsigned short preload){
    _nbOfChips     = Chips;
    Preload_global = preload;
    return true;
}

int Timepix::SetOption(unsigned short option){
    Option_global=option;
    return true;
}

unsigned short Timepix::GetNumChips(){
    return _nbOfChips;
}

FrameArray<bool> Timepix::GetMaskArray(int chip){
    // returns a FrameArray<bool> based on the Mask member array
    FrameArray<bool> mask_array;
    for(std::size_t x = 0; x < PIXPD; x++){
	for(std::size_t y = 0; y < PIXPD; y++){
	    // compare Mask with 0. If Mask == 0, in TOS terms the pixel
	    // will NOT be considered. In our case this means we want
	    // mask_data == True, since we do NOT consider masked data
	    mask_array[x][y]  = Mask[chip][y][x] == 0;
	    if(mask_array[x][y] == true){
		//std::cout << "Pixel x,y " << x << "," << y << "is true" << std::endl;
	    }
	    else{
		// std::cout << "Pixel x,y " << x << "," << y << "is false\n"
		// 	  << "mask x,y "  << static_cast<int>(Mask[chip][y][x])
		// 	  << std::endl;
	    }
	}
    }
    return mask_array;
}

unsigned short Timepix::GetPreload(){
    return Preload_global;
}

unsigned short Timepix::GetOption(){
    return Option_global;
}

int Timepix::SetI2C(unsigned int i2c_val){
    I2C_global=i2c_val;
    return true;
}

unsigned int Timepix::GetI2C(){
    return I2C_global;
}

int Timepix::SetADCresult(unsigned short ADC_ChAlert, unsigned short ADC_result){
    ADC_channel_global = ADC_ChAlert & 7; // gives last 3 bits
    ADC_alert_global = ADC_ChAlert & 8; // gives 4th bit
    ADC_result_global = ADC_result;
    i2c_result_global = ((ADC_ChAlert << 16) + ADC_result);
    return true;
}
int Timepix::GetI2cResult(){
    return i2c_result_global;
}

unsigned short Timepix::GetADCresult(){
    return ADC_result_global;
}
unsigned short Timepix::GetADCchannel(){
    return ADC_channel_global;
}
unsigned short Timepix::GetADCalert(){
    return ADC_alert_global;
}
void Timepix::SetFADCshutter(unsigned short FADCshutter){
    FADCshutter_global = FADCshutter;
}
unsigned short Timepix::GetFADCshutter(){
    return FADCshutter_global;
}
void Timepix::SetExtraByte(unsigned short ExtraByte){
    ExtraByte_global = ExtraByte;
}
unsigned short Timepix::GetExtraByte(){
    return ExtraByte_global;
}
void Timepix::SetFADCtriggered(unsigned short FADCtriggered){
    FADCtriggered_global = FADCtriggered;
}
unsigned short Timepix::GetFADCtriggered(){
    return FADCtriggered_global;
}

void Timepix::SetChipIDOffset(int ChipIdOffset){
    // run over all 24 bits and set ChipIDBitPos array to new value of chip ID Bit pos
    _chipIdOffset = ChipIdOffset;
    
    for(int x = 0; x < 24; x++){
	ChipIDBitPos[x]= 255-(_chipIdOffset + x);
    } 
    // now the array should be set to a new value
}


void Timepix::SetChipSet(const std::set<unsigned short> &chip_set){
    // function to hand a const reference to the _chip_set member variable of the
    // console class, such that always an up-to-date chip set is available to
    // the sub classes

    _chip_set = chip_set;
}
