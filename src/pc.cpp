/**********************************************************************/
/*                                                             pc.cpp */
/*  TOS - Timepix Operating Software                                  */
/*                                                                    */
/*                                                         20.07.2009 */
/*                                                    Christian Kahra */
/*                                     chrkahra@students.uni-mainz.de */
/*                                        Institut fuer Physik - ETAP */
/*                              Johannes-Gutenberg Universitaet Mainz */
/**********************************************************************/

#include "pc.hpp"




PC::PC():_useHvFadc(false),
         _hvFadcObj(NULL),
         BufferSize(80),
         Vbuffer( BufferSize, std::vector<std::vector<int>* >( 8))
{
#if DEBUG == 2
    std::cout<<"Enter PC::PC()"<<std::endl;	
#endif

    ok=fpga.okay();
    int i,loop;
    int lfsr, linear;
	
    for(i=0;i<16384;++i){LFSR_LookUpTable[i]=0;}
    lfsr=0x3FFF;

    for(loop=0; loop<11811; loop++)
    {
	LFSR_LookUpTable[lfsr]=loop;
	linear=0; 
	if((lfsr & 1)!=((lfsr>>13)&1)){linear=1;}
	lfsr= ((lfsr<<1) + linear) & 0x3FFF;
    }

    char puffer[256];
    ok=(getcwd(puffer,sizeof(puffer))!=NULL);

#if DEBUG==2
    if(ok){std::cout<<"Current working directory is: "<<puffer<<std::endl;}
    else{std::cout<<"getcwd funktioniert nicht"<<std::endl;}
#endif
  
    TOSPathName=puffer;
  
    DataPathName="data";
    DataFileName="data.txt";
    Data2FileName="data2.txt";
    Data3FileName="data3.txt";
    Data4FileName="data4.txt";
    Data5FileName="data5.txt";
    Data6FileName="data6.txt";
    Data7FileName="data7.txt";
    Data8FileName="data8.txt";
  
    RunFileName="run.txt";
  
    FSRFileName="fsr.txt";
    FSR2FileName="fsr2.txt";
    FSR3FileName="fsr3.txt";
    FSR4FileName="fsr4.txt";
    FSR5FileName="fsr5.txt";
    FSR6FileName="fsr6.txt";
    FSR7FileName="fsr7.txt";
    FSR8FileName="fsr8.txt";
  
    MatrixFileName="matrix.txt";
    Matrix2FileName="matrix2.txt";
    Matrix3FileName="matrix3.txt";
    Matrix4FileName="matrix4.txt";
    Matrix5FileName="matrix5.txt";
    Matrix6FileName="matrix6.txt";
    Matrix7FileName="matrix7.txt";
    Matrix8FileName="matrix8.txt";
        
    ThresholdFileName="threshold.txt";
    Threshold2FileName="threshold2.txt";
    Threshold3FileName="threshold3.txt";
    Threshold4FileName="threshold4.txt";
    Threshold5FileName="threshold5.txt";
    Threshold6FileName="threshold6.txt";
    Threshold7FileName="threshold7.txt";
    Threshold8FileName="threshold8.txt";
  
    MaskFileName="mask.txt";
    Mask2FileName="mask2.txt";
    Mask3FileName="mask3.txt";
    Mask4FileName="mask4.txt";
    Mask5FileName="mask5.txt";
    Mask6FileName="mask6.txt";
    Mask7FileName="mask7.txt";
    Mask8FileName="mask8.txt";
  
    DACScanFileName="DACScan.txt";
  
    TOTCalibFileName = "TOTCalib.txt"; 
    TOTCalib2FileName = "TOTCalib2.txt"; 
    TOTCalib3FileName = "TOTCalib3.txt"; 
    TOTCalib3FileName = "TOTCalib3.txt"; 
    TOTCalib4FileName = "TOTCalib4.txt"; 
    TOTCalib5FileName = "TOTCalib5.txt"; 
    TOTCalib6FileName = "TOTCalib6.txt"; 
    TOTCalib7FileName = "TOTCalib7.txt"; 
    TOTCalib8FileName = "TOTCalib8.txt";
  
    TOACalibFileName = "TOACalib.txt"; 
    TOACalib2FileName = "TOACalib2.txt"; 
    TOACalib3FileName = "TOTAalib3.txt"; 
    TOACalib3FileName = "TOACalib3.txt";
    TOACalib4FileName = "TOACalib4.txt";
    TOACalib5FileName = "TOACalib5.txt";
    TOACalib6FileName = "TOACalib6.txt"; 
    TOACalib7FileName = "TOACalib7.txt"; 
    TOACalib8FileName = "TOACalib8.txt";

    MeasuringCounter=0;

    DataInBuffer = 0;
}




// void PC::initFADC(V1729a* fadc, HighLevelFunction_VME* fadcFunctions, bool useFadc)
// {
// #if DEBUG == 2
//     std::cout << "Enter: PC::initFADC()" << std::endl;
// #endif

//     _useFADC = useFadc;

//     if(useFadc)
//     {
// 	_fadc = fadc;
// 	_fadcFunctions = fadcFunctions;
//     }
//     else
//     {
// 	_fadc = NULL;
// 	_fadcFunctions = NULL;
//     }
  
//     return;
// }


void PC::initHV_FADC(HV_FADC_Obj* hvFadcObj, bool useHvFadc)
{
#if DEBUG == 2
    std::cout << "Enter: PC::initHV_FADC()" << std::endl;
#endif

    _useHvFadc = useHvFadc;

    if(_useHvFadc)
    {
	_hvFadcObj = hvFadcObj;
    }
    else
    {
	_hvFadcObj = NULL;
    }
  
    return;
}



int PC::okay(){
#if DEBUG==2
	std::cout<<"Enter PC::okay()"<<std::endl;	
#endif
	return ok;
}

int PC::DoReadOut(const char* filename[9]){
#if DEBUG==2
	std::cout<<"Enter PC::DoReadOut()"<<std::endl;	
#endif
	int result;
	//int data[256][256];
	std::vector<std::vector<std::vector<int> > > *VecData = new std::vector<std::vector<std::vector<int> > >(9, std::vector < std::vector<int> >(256, std::vector<int>(256,0)));
	int x,y;
	;
	//result=fpga.SerialReadOut(data);
	result=fpga.SerialReadOut(VecData);
	if(result==300){
		FILE* f[8];
		for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
			f[chip]=fopen(filename[chip],"w"); if(f[chip]==NULL) {std::cout<<"(PC::DoReadOut) Dateifehler"<<std::endl; return -1;}
#if PERFORMANCE==1
		fwrite(pix,sizeof(int),256*256,f);
#else
			result=0;
			for(y=0;y<256;y++){
				for(x=0;x<255;x++){
					fprintf(f[chip],"%i ",LFSR_LookUpTable[(*VecData)[chip][y][x]]);//data[y][x]);
					if(((*VecData)[chip][y][x]!=16383)&&((*VecData)[chip][y][x]!=0)){++result;}
				}
				fprintf(f[chip],"%i\n",LFSR_LookUpTable[(*VecData)[chip][y][255]]);//data[y][255]);//
				if(((*VecData)[chip][y][255]!=16383)&&((*VecData)[chip][y][255]!=0)){++result;}
			}
#endif
			fclose(f[chip]);
		}
	}
	else return (-result);
	delete VecData;
	return result;
}

int PC::DoReadOut2(const char* filename, unsigned short chip){
#if DEBUG==2
	std::cout<<"Enter PC::DoReadOut()"<<std::endl;
#endif
	int hits;
	std::vector<int> *data = new std::vector<int>((12288+1),0); //+1: Entry 0 of Vector contains NumHits
	hits=fpga.DataFPGAPC(data,chip);
	//if(hits>1){//need more than 2 hits (specified for Christophs setup with 1 noise pixel)
		FILE* f=fopen(filename,"w"); if(f==NULL) {std::cout<<"(PC::DoReadOut2) Dateifehler"<<std::endl; return -1;}
#if PERFORMANCE==1
		fwrite(pix,sizeof(int),256*256,f);
#else
		 std::cout << "single readout"<<std::endl;

		for(int i=0; i<hits*3; i=i+3){
			//std::cout << "hit " << i/3<<": x:"<<data[i]<<" , y:"<<data[i+1]<<" , value: "<<data[i+2]<<std::endl;
			fprintf(f, "%d %d %d \n",(*data)[i+1],(*data)[i+1+1],(*data)[i+2+1]);
		}
#endif
		fclose(f);

	delete data;
	return hits;
}


int PC::DoDACScan(int DACstoScan,unsigned short chip) {
#if DEBUG==2
	std::cout<<"Enter PC::DACScan()"<<std::endl;	
#endif
	int value = 0;
	int Bits = 0;
	unsigned short channel;
	if (chip == 1) {channel=5;}
	if (chip == 2) {channel=3;}
	if (chip == 3) {channel=7;}
	if (chip == 4) {channel=1;}
	if (chip == 5) {channel=6;}
	if (chip == 6) {channel=0;}
	if (chip == 7) {channel=4;}
	if (chip == 8) {channel=2;}
	std::cout<<"DACstoScan "<<DACstoScan<<std::endl;
	std::fstream f;
	f.open(DACScanFileName.c_str(),std::fstream::out);
	for(unsigned int dac=0;dac<14;dac++){
		fpga.GeneralReset();
		usleep(8000);
		if (dac==0) {Bits=255;}
		if (dac==1) {Bits=255;}
		if (dac==2) {Bits=255;}
		if (dac==3) {Bits=255;}
		if (dac==4) {Bits=255;}
		if (dac==5) {Bits=255;}
		if (dac==6) {Bits=1023;}
		if (dac==7) {Bits=255;}
		if (dac==8) {Bits=255;}
		if (dac==9) {Bits=255;}
		if (dac==10) {Bits=255;}
		if (dac==11) {Bits=255;}
		if (dac==12) {Bits=255;}
		if (dac==13) {Bits=15;}
		fpga.tp.SetDAC(0,chip,128); //All DACs to middle possition
		fpga.tp.SetDAC(1,chip,128);
		fpga.tp.SetDAC(2,chip,128);
		fpga.tp.SetDAC(3,chip,127);
		fpga.tp.SetDAC(4,chip,128);
		fpga.tp.SetDAC(5,chip,128);
		fpga.tp.SetDAC(6,chip,512);
		fpga.tp.SetDAC(7,chip,128);
		fpga.tp.SetDAC(8,chip,128);
		fpga.tp.SetDAC(9,chip,128);
		fpga.tp.SetDAC(10,chip,128);
		fpga.tp.SetDAC(11,chip,128);
		fpga.tp.SetDAC(12,chip,128);
		fpga.tp.SetDAC(13,chip,7);
		fpga.tp.SetDAC(15,chip,1); //Sense DAC
		if (dac==13){
			fpga.tp.SetDAC(17,chip,6);
		}
		else {
			fpga.tp.SetDAC(17,chip,fpga.tp.GetDACCode(dac));
		}
		fpga.WriteReadFSR();
		usleep(8000 );
		std::cout<<"dac "<<dac<<" 1<<dac "<< (DACstoScan&(1<<dac)) <<" DAC Code: "<<fpga.tp.GetDACCode(dac) <<std::endl;
		f << fpga.tp.GetDACName(dac) << std::endl;
		if(DACstoScan&(1<<dac)){

			for(int i=0; i<= Bits; i++){
				fpga.tp.SetDAC(dac,chip,i);
				fpga.WriteReadFSR();
				usleep(50000); // just for readablity of voltage value at osci
				value=fpga.i2cADC(channel);
				f << i << "\t" << value << std::endl;
				std::cout<<"DAC "<<dac<<" "<< i <<std::endl;
			}
		}
	}
	f.close();
	return 0;
}


int PC::DoTHLScan(unsigned short chip,unsigned short coarselow, unsigned short coarsehigh){
	fpga.tp.LoadFSRFromFile(GetFSRFileName(chip),chip);
	for(unsigned int coarse=coarselow;coarse<=coarsehigh;coarse++){
		fpga.GeneralReset();
		usleep(10000);
		fpga.SetMatrix();
		usleep(60000 );
		int data[9][256][256];
		fpga.SerialReadOut(data);
		fpga.tp.SetDAC(13,chip,coarse);
		for(unsigned int thl=0;thl<1024;thl++){
			fpga.tp.SetDAC(6,chip,thl);
			fpga.WriteReadFSR();
			usleep(10000 );
			fpga.CountingTime(255);
			std::vector<int> *data = new std::vector<int>((12288+1),0); //+1: Entry 0 of Vector contains NumHits
			int hits = 0;
			int result=0;
			fpga.DataChipFPGA(result);
			hits = fpga.DataFPGAPC(data,chip);
			usleep(20000);
			std::cout<<"Coarse: "<<coarse<<" THL: "<< thl <<std::endl;
			delete data;
		}
	}
	return 0;
}


int PC::DoSCurveScan_meanChip(unsigned short voltage,int time, unsigned short startTHL[9], unsigned short stopTHL[9], unsigned short offset){
	for (unsigned short chip = 1;chip<= fpga.tp.GetNumChips() ;chip++){
		fpga.tp.LoadFSRFromFile(GetFSRFileName(chip),chip);
		unsigned short pix_per_row = 8;
		unsigned short int i = 0;
		if (voltage == 0) { i= 8;}
		for (unsigned int volt=0; volt<=i;volt++){
			int meancounts[1024] = {0};
			int meanstepsum[1024] = {0};
			if (voltage == 0) {
				int myint = 0;
				if (volt == 0) { myint = 350;}
				else if (volt == 1) { myint = 370;}
				else if (volt == 2) { myint = 375;}
				else if (volt == 3) { myint = 380;}
				else if (volt == 4) { myint = 385;}
				else if (volt == 5) { myint = 390;}
				else if (volt == 6) { myint = 400;}
				else if (volt == 7) { myint = 410;}
				else if (volt == 8) { myint = 450;}
				else {myint = 350;}
				fpga.i2cDAC(myint,2);
				fpga.i2cDAC(350,3);
				fpga.tpulse(1000,10);
			}
			for (unsigned int step=0; step<(256/pix_per_row);step=step+8){ //must go from 0 to <(256/pix_per_row), put 1 for just 1 run
				std::cout<<"first step"<<std::endl;
				fpga.GeneralReset();
				usleep(1000);
				for (unsigned short c = 1;c<= fpga.tp.GetNumChips() ;c++){
					fpga.tp.LoadFSRFromFile(GetFSRFileName(c),c);
					fpga.tp.UniformMatrix(0,0,0,0,0,c); //0,0: Medipix modus
					fpga.tp.LoadThresholdFromFile(GetThresholdFileName(c),c);
					fpga.tp.SetDAC(14,c,0);
				}
				unsigned int CTPR = 1<<offset;
				fpga.tp.SetDAC(14,chip,CTPR);
				fpga.tp.Spacing_row(step,pix_per_row,chip);
				fpga.tp.Spacing_row_TPulse(step,pix_per_row,chip);
				fpga.SetMatrix();
				usleep(2000 );
				int data[9][256][256];
				fpga.SerialReadOut(data);
				fpga.EnableTPulse(1);
				fpga.tp.SetDAC(13,chip,7);
				int meancounts_per_step[1024] = {0};
				for(unsigned int thl=startTHL[chip];thl<=stopTHL[chip];thl++){
					fpga.tp.SetDAC(6,chip,thl);
					fpga.WriteReadFSR();
					usleep(400);
					fpga.CountingTime_long(time);
					int result=0;
					std::vector<std::vector<std::vector<int> > > *VecData = new std::vector<std::vector<std::vector<int> > >(9, std::vector < std::vector<int> >(256, std::vector<int>(256,0)));
					int x,y;
					result=fpga.SerialReadOut(VecData);
					if(result==300){
						int hit_counter = 0;
						int hitsum = 0;
						for(y=0;y<256;y++){
							for(x=0+offset;x<256;x=x+32){
								if (LFSR_LookUpTable[(*VecData)[chip][y][x]] >= 0 and LFSR_LookUpTable[(*VecData)[chip][y][x]]!=11810 and (fpga.tp.GetMask(y,x,chip))==1){
									hit_counter +=1;
									hitsum +=LFSR_LookUpTable[(*VecData)[chip][y][x]];
								}
							}
						}
						if (hit_counter != 0){
							meancounts_per_step[thl] = hitsum/hit_counter;
						}
						else {
							meancounts_per_step[thl] = 0;
						}
						//std::cout<<"hit_counter "<<hit_counter<<std::endl;
						//std::cout<<"hitsum "<<hitsum<<std::endl;
					}
					delete VecData;
					std::cout<<"Chip "<<chip<<" of "<<fpga.tp.GetNumChips()<<" step "<<step/8+1<<" of "<<(256/pix_per_row)/8<<" THL: "<< thl <<" meancounts "<<meancounts_per_step[thl]<<std::endl;

				} //end thl loop
				for(unsigned int thl=startTHL[chip];thl<=stopTHL[chip];thl++){
					meanstepsum[thl]+=meancounts_per_step[thl];
					//std::cout<<"meanstepsum[thl] "<<meanstepsum[thl]<<" for thl " <<thl<<std::endl;
				}
				fpga.EnableTPulse(0);
			} //end step loop
			for(unsigned int thl=startTHL[chip];thl<=stopTHL[chip];thl++){
				meancounts[thl] = meanstepsum[thl]/(256/pix_per_row)*8; //put 1 for just 1 run
			}
			std::ostringstream sstream1;
			sstream1<<"chip_"<<chip;
			PathName=DataPathName+"/"; PathName+=sstream1.str();
#ifdef __WIN32__
			mkdir(PathName.c_str());
#else
			mkdir(PathName.c_str(),0755);
#endif
			std::string filename;
			std::ostringstream sstream;
			if (voltage == 0) {
				if (volt == 0) {sstream<<"voltage_0.txt";}
				else if (volt == 1) {sstream<<"voltage_20.txt";}
				else if (volt == 2) {sstream<<"voltage_25.txt";}
				else if (volt == 3) {sstream<<"voltage_30.txt";}
				else if (volt == 4) {sstream<<"voltage_35.txt";}
				else if (volt == 5) {sstream<<"voltage_40.txt";}
				else if (volt == 6) {sstream<<"voltage_50.txt";}
				else if (volt == 7) {sstream<<"voltage_60.txt";}
				else if (volt == 8) {sstream<<"voltage_100.txt";}
				else {sstream<<"voltage_0.txt";}
			}
			else sstream<<"voltage_"<<voltage<<".txt";
			filename=PathName+"/"; filename+=sstream.str();
			const char* filename_= filename.c_str();
			std::fstream f;

			f.open(filename_,std::fstream::out);
			if(f.is_open()){
				for(int thl=stopTHL[chip];thl>=startTHL[chip];thl-=1){
					f<<thl<<"\t"<<meancounts[thl]<<std::endl;
				}
				f.close();
			}
		}
	}
	return 0;
}


int PC::DoSCurveScan(unsigned short chip,unsigned short coarselow, unsigned short coarsehigh, int time){
	fpga.tp.LoadFSRFromFile(GetFSRFileName(chip),chip);
	unsigned short pix_per_row = 8;
	for(unsigned int coarse=coarselow;coarse<=coarsehigh;coarse++){
		for (unsigned int step=0; step<(256/pix_per_row);step++){ //must go from 0 to <(256/pix_per_row)
			fpga.GeneralReset();
			usleep(1000);
			for (unsigned short c = 1;c<= fpga.tp.GetNumChips() ;c++){
				fpga.tp.LoadFSRFromFile(GetFSRFileName(c),c);
				fpga.tp.UniformMatrix(0,0,0,0,0,c); //0,0: Medipix modus
				fpga.tp.LoadThresholdFromFile(GetThresholdFileName(c),c);
				fpga.tp.SetDAC(14,c,0);
			}
			fpga.tp.SetDAC(14,chip,4294967295);
			fpga.tp.Spacing_row(step,pix_per_row,chip);
			fpga.tp.Spacing_row_TPulse(step,pix_per_row,chip);
			fpga.SetMatrix();
			usleep(2000 );
			int data[9][256][256];
			fpga.SerialReadOut(data);
			fpga.EnableTPulse(1);
			fpga.tp.SetDAC(13,chip,coarse);
			for(unsigned int thl=0;thl<1024;thl++){
				fpga.tp.SetDAC(6,chip,thl);
				fpga.WriteReadFSR();
				usleep(400 );
				int ChipID = fpga.tp.GetChipIDsilent(chip);
				fpga.CountingTime_long(time);
				//usleep(20000);
				int result=0;
				int hits = 0;
				std::vector<std::vector<std::vector<int> > > *VecData = new std::vector<std::vector<std::vector<int> > >(9, std::vector < std::vector<int> >(256, std::vector<int>(256,0)));
				int x,y;
				result=fpga.SerialReadOut(VecData);
				if(result==300){
#ifdef __WIN32__
				        mkdir(DataPathName.c_str());
#else
					mkdir(DataPathName.c_str(),0755);
#endif
					std::ostringstream sstream1;
					sstream1<<ChipID<<"_SCurve_"<<chip<<"_coarse_"<<coarse;
					PathName=DataPathName+"/"; PathName+=sstream1.str();
#ifdef __WIN32__
					mkdir(PathName.c_str());
#else
					mkdir(PathName.c_str(),0755);
#endif
					std::string filename;
					std::ostringstream sstream;
					sstream<<thl<<"_step_"<<step<<".txt";
					filename=PathName+"/"; filename+=sstream.str();
					const char* filename_= filename.c_str();
					std::fstream f;
					f.open(filename_,std::fstream::out);
					if(f.is_open()){
						for(y=0;y<256;y++){
							for(x=0;x<256;++x){
								f<<LFSR_LookUpTable[(*VecData)[chip][y][x]]<<" ";
								if(((*VecData)[chip][y][x]!=16383)&&((*VecData)[chip][y][x]!=0)){++hits;}
							}
							f<<std::endl;
						}
					}

				}
				delete VecData;
				//usleep(20000);
				std::cout<<"Step "<<step<<" of "<<(256/pix_per_row)<<" Coarse: "<<coarse<<" THL: "<< thl <<" hits: "<<hits<<std::endl;
			} //end thl loop
			fpga.EnableTPulse(0);
		} //end step loop
	}
	return 0;
}


void PC::DACScanHistogram(void* PointerToObject, char dac, int bit, int val){
#if DEBUG==2
	std::cout<<"Enter PC::DACScanHistogram()"<<std::endl;	
#endif
	if(bit>0)dac=-1;
	// TODO: Alex code has the next line not commented
	//       Tobys code: line is commented
	//       commented for now. Need to take a look
	// Console::WrapperToDACScanLive(PointerToObject, dac, val);
}


int PC::THscan(unsigned int coarse, int thl, int array_pos, short ths, unsigned int step, unsigned short pix_per_row, short ***p3DArray, int sum[256][256], int hit_counter[256][256], short thp, unsigned short chp){
	if(thl%100 == 0) std::cout<<"Thp="<<thp<<" (16=Eq); coarse:"<<coarse<<" , thl:"<<thl<<std::endl; //commented in
	fpga.GeneralReset();
	//for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
		fpga.tp.LoadFSRFromFile(GetFSRFileName(chp),chp);
		fpga.tp.SetDAC(6,chp,thl);
		if (ths!=0) {fpga.tp.SetDAC(10,chp,ths);}
		fpga.tp.SetDAC(13,chp,coarse);
	//}
	fpga.WriteReadFSR();
	usleep(400);
	fpga.CountingTime_long(10);
	int result = 0;

	/*std::string filename[9]= {""};
	const char* f[8];
	for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
		filename[chip]=GetDataPathName();
		filename[chip]+="/";
		filename[chip]+=GetDataFileName(chip);
		f[chip] = filename[chip].c_str();
	}
	DoReadOut(f);*/ //for writing to file

	//std::vector<std::vector<std::vector<int> > > *VecData = new std::vector<std::vector<std::vector<int> > >(9, std::vector < std::vector<int> >(256, std::vector<int>(256,0)));
	//fpga.SerialReadOut(VecData);

	int pix_tempdata2[256][256] = {0};
	fpga.DataChipFPGA(result);
	fpga.DataFPGAPC(pix_tempdata2,chp); //!!!only one chip!!!
	for(short y=step;y<256;y+=(256/pix_per_row)){
		for(short x=0;x<256;x++){
			if(pix_tempdata2[y][x]>=20 and pix_tempdata2[y][x]!=11810){
				//if (pix_tempdata2[y][x]>=200) {std::cout << "hits for thl " << thl <<" :" << pix_tempdata2[y][x] << std::endl;}
				p3DArray[y][x][array_pos] = pix_tempdata2[y][x];
			//if(LFSR_LookUpTable[(*VecData)[chp][y][x]]>=20 and LFSR_LookUpTable[(*VecData)[chp][y][x]]!=11810){
				//p3DArray[y][x][array_pos] = LFSR_LookUpTable[(*VecData)[chp][y][x]];
				sum[y][x]+=p3DArray[y][x][array_pos]*(array_pos);
				hit_counter[y][x]+=p3DArray[y][x][array_pos];
			}
			else{
				p3DArray[y][x][array_pos] = 0;
				sum[y][x]+=0;
				hit_counter[y][x]+=0;
			}
		}
	}
	//delete VecData;
	return 0;
}


int PC::DoTHSopt(unsigned short doTHeq,unsigned short pix_per_row_THeq,unsigned short chp,short ths,short ext_coarse,short max_thl,short min_thl){ //untill now only one chip at same time
	//#if DEBUG==2
		std::cout<<"Enter PC::THSopt()"<<std::endl;
	//#endif
	{
		int x,y,thl;
		unsigned int coarse;
		#define x_length 256
		#define y_length 256
		#define thl_length 1024+2*372 // thl shift of ~372 for one coarse


		short ***p3DArray0;
		// Allocate memory
		p3DArray0 = new short**[x_length];
		for (int i = 0; i < x_length; ++i) {
			p3DArray0[i] = new short*[y_length];
			for (int j = 0; j < y_length; ++j)
			  p3DArray0[i][j] = new short[thl_length];
		}
		short ***p3DArray15;
		// Allocate memory
		p3DArray15 = new short**[x_length];
		for (int i = 0; i < x_length; ++i) {
			p3DArray15[i] = new short*[y_length];
			for (int j = 0; j < y_length; ++j)
			  p3DArray15[i][j] = new short[thl_length];
		}
		std::vector<std::vector<std::vector<int> > > *pix_tempdata = new std::vector<std::vector<std::vector<int> > >(9, std::vector < std::vector<int> >(256, std::vector<int>(256,0)));
		// Assign values
		for(coarse=8;coarse>5;coarse--){
			for(y=0;y<256;++y){
				for(x=0;x<256;++x){
					if (coarse == 6){
						for(thl=0;thl<1024;thl++){
							p3DArray0[y][x][thl] = 0;
							p3DArray15[y][x][thl] = 0;
						}
					}
					else {
						for(thl=1023;thl>=652;thl-=1){
							p3DArray0[y][x][thl+(coarse-6)*372] = 0;
							p3DArray15[y][x][thl+(coarse-6)*372] = 0;
						}
					}

				}
			}
		}
		float mean0[256][256] = {0};
		float mean15[256][256] = {0};
		int sum0[256][256] = {0};
		int sum15[256][256] = {0};
		int hit_counter0[256][256] = {0};
		int hit_counter15[256][256] = {0};
		int mean0entries = 0;
		int mean15entries = 0;
		int summean0 = 0;
		int summean15 = 0;
		int mean0counter = 0;
		int mean15counter = 0;
		float mean0mean = 0;
		float mean15mean = 0;
		float mean0rms = 0;
		float mean15rms = 0;
		float mean0rms_pix = 0;
		float mean15rms_pix = 0;
		float mean0sum = 0;
		float mean15sum = 0;
		float mean0sum_pix = 0;
		float mean15sum_pix = 0;
		float delta0_15 = 0;
		float last_delta0_15 = 0;
		unsigned short last_ths = 0;
		float optdelta = 20; // not initialised as 0 because while loop would not work in this case
		std::fstream f;
		unsigned short pix_per_row = 16;

		while ((delta0_15 < optdelta-2 || delta0_15 > optdelta+2)) {
			for(y=0;y<256;++y){
				for(x=0;x<256;++x){
					hit_counter0[y][x] =0;
					sum0[y][x] = 0;
					hit_counter15[y][x] =0;
					sum15[y][x] = 0;
				}
			}
			mean0entries = 0;
			mean0counter = 0;
			mean0sum = 0;
			summean0 = 0;
			mean0sum_pix = 0;
			mean15entries = 0;
			mean15counter = 0;
			mean15sum = 0;
			summean15 = 0;
			mean15sum_pix = 0;
			for (unsigned int step=0; step<1;step++){ //must go from 0 to <(256/pix_per_row)
				for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
					fpga.tp.UniformMatrix(0,0,0,0,0,chip); //mask all pixels on all chips, otherwise they draw current
				}
				fpga.tp.UniformMatrix(0,0,1,1,0,chp); //0,0: Medipix modus
				fpga.tp.Spacing_row(step,pix_per_row,chp);
				fpga.tp.SaveMatrixToFile(GetMatrixFileName(chp),chp);
				fpga.SetMatrix();
				usleep(2000);
				fpga.SerialReadOut(pix_tempdata);
				//usleep(60000 );
				fpga.GeneralReset();
				for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
					fpga.tp.LoadFSRFromFile(GetFSRFileName(chip),chip);
				}
				fpga.WriteReadFSR();
				fpga.GeneralReset();
				for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
					fpga.tp.UniformMatrix(0,0,0,0,0,chip); //mask all pixels on all chips, otherwise they draw current
				}
				fpga.tp.UniformMatrix(0,0,1,1,0,chp); //0,0: Medipix modus
				fpga.tp.Spacing_row(step,pix_per_row,chp);
				fpga.tp.SaveMatrixToFile(GetMatrixFileName(chp),chp);
				fpga.SetMatrix();
				usleep(2000);
				fpga.SerialReadOut(pix_tempdata);
				//usleep(60000 );
				std::cout<<"Beginne RauschScan thp=0"<<std::endl;//" step "<<step<<" of "<<(256/pix_per_row)-1<<" from spacing"<<std::endl;
				if (ext_coarse == 1) {
					for(coarse=8;coarse>=6;coarse--){
						if (coarse == 6){
							for(thl=1023;thl>=0;thl-=1){
								THscan(coarse,thl,thl,ths,step,pix_per_row,p3DArray0,sum0,hit_counter0,0,chp);//!!!only one chip!!!
							}
						}
						else {
							for(thl=1023;thl>=652;thl-=1){
								THscan(coarse,thl,thl+(coarse-6)*372,ths,step,pix_per_row,p3DArray0,sum0,hit_counter0,0,chp);
							}
						}
					}
				}
				else {
					coarse = 7;
					for(thl=max_thl;thl>=min_thl;thl-=1){
						THscan(coarse,thl,thl,ths,step,pix_per_row,p3DArray0,sum0,hit_counter0,0,chp);
					}
				}

				fpga.GeneralReset();
				//usleep(8000);
				for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
					fpga.tp.UniformMatrix(0,0,0,0,0,chip); //mask all pixels on all chips, otherwise they draw current
				}
				fpga.tp.UniformMatrix(0,0,1,1,15,chp); //0,0: Medipix modus
				fpga.tp.Spacing_row(step,pix_per_row,chp);
				fpga.tp.SaveMatrixToFile(GetMatrixFileName(chp),chp);
				fpga.SetMatrix();
				usleep(2000);
				fpga.SerialReadOut(pix_tempdata);
				//usleep(60000 );
				std::cout<<"Beginne RauschScan thp=15"<<std::endl;//" step "<<step<<" of "<<(256/pix_per_row)-1<<" from spacing"<<std::endl;
				if (ext_coarse == 1) {
					for(coarse=8;coarse>=6;coarse--){
						if (coarse == 6){
							for(thl=1023;thl>=0;thl-=1){
								THscan(coarse,thl,thl,ths,step,pix_per_row,p3DArray15,sum15,hit_counter15,15,chp);
							}
						}
						else {
							for(thl=1023;thl>=652;thl-=1){
								THscan(coarse,thl,thl+(coarse-6)*372,ths,step,pix_per_row,p3DArray15,sum15,hit_counter15,15,chp);
							}
						}
					}
				}
				else {
					coarse = 7;
					for(thl=max_thl;thl>=min_thl;thl-=1){
						THscan(coarse,thl,thl,ths,step,pix_per_row,p3DArray15,sum15,hit_counter15,15,chp);
					}
				}
			}
				for(y=0;y<256;y++){
					for(x=0;x<256;x++){
						if (hit_counter0[y][x]!=0){
							mean0[y][x] = sum0[y][x]/hit_counter0[y][x];
							mean0entries += 1;
							summean0 += mean0[y][x];
						}
						if (hit_counter15[y][x]!=0){
							mean15[y][x] = sum15[y][x]/hit_counter15[y][x];
							mean15entries += 1;
							summean15 += mean15[y][x];
						}
					}
				}
				//for pixel 43,102 only to test
				if (ext_coarse == 1) {
					for(coarse=8;coarse>=6;coarse--){
						if (coarse == 6){
							for(thl=1023;thl>=0;thl-=1){
								mean0sum_pix+=((thl-mean0[102][43])*(thl-mean0[102][43])*p3DArray0[102][43][thl]);
								mean15sum_pix+=((thl-mean15[102][43])*(thl-mean15[102][43])*p3DArray15[102][43][thl]);
							}
						}
						else {
							for(thl=1023;thl>=652;thl-=1){
								mean0sum_pix+=(((thl+(coarse-6)*372)-mean0[102][43])*((thl+(coarse-6)*372)-mean0[102][43])*p3DArray0[102][43][thl+(coarse-6)*372]);
								mean15sum_pix+=(((thl+(coarse-6)*372)-mean15[102][43])*((thl+(coarse-6)*372)-mean15[102][43])*p3DArray15[102][43][thl+(coarse-6)*372]);
							}
						}
					}
				}
				else {
					coarse = 7;
					for(thl=max_thl;thl>=min_thl;thl-=1){
						mean0sum_pix+=((thl-mean0[102][43])*(thl-mean0[102][43])*p3DArray0[102][43][thl]);
						mean15sum_pix+=((thl-mean15[102][43])*(thl-mean15[102][43])*p3DArray15[102][43][thl]);
					}
				}

			f.open("THSoptdist.txt",std::fstream::out);
			if(f.is_open()){
				for(y=0;y<256;y++){
					for(x=0;x<256;x++){
						//if ((fpga.tp.GetMask(y,x))==1){
							f<<y<<"\t"<<x<<"\t"<<mean0[y][x]<<"\t"<<mean15[y][x]<<std::endl;
						//}
					}
				}
				f.close();
			}

			mean0rms_pix=sqrt(mean0sum_pix/(hit_counter0[102][43]-1));
			mean15rms_pix=sqrt(mean15sum_pix/(hit_counter15[102][43]-1));
			std::cout<<"sum0 pixel 43,102:: "<<sum0[102][43]<<std::endl;
			std::cout<<"Total hits pixel 43,102:: "<<hit_counter0[102][43]<<std::endl;
			std::cout<<"mean0 value pixel 43,102: "<<mean0[102][43]<<" rms="<<mean0rms_pix<<std::endl;
			std::cout<<"mean15 value pixel 43,102: "<<mean15[102][43]<<" rms="<<mean15rms_pix<<std::endl;
			if (mean0rms_pix>=75){
				std::cout<<"WARNING: RMS OF MEAN0 FOR TESTPIXEL IS BIGGER THAN 75, SOMETHING MUST BE WRONG, CHECK CHIP POWER SUPPLY. RMS VALUE IS "<<mean0rms_pix<<std::endl;
			}
			if (mean15rms_pix>=75){
				std::cout<<"WARNING: RMS OF MEAN15 FOR TESTPIXEL IS BIGGER THAN 75, SOMETHING MUST BE WRONG, CHECK CHIP POWER SUPPLY. RMS VALUE IS "<<mean15rms_pix<<std::endl;
			}
			//calculate mean of mean0 and mean15 distribution
			if (mean0entries != 0) mean0mean = summean0/mean0entries;
			if (mean15entries != 0) mean15mean = summean15/mean15entries;
			for(y=0;y<256;y++){
				for(x=0;x<256;x++){
					if (mean0[y][x]!=0){
						mean0sum+=(mean0[y][x]-mean0mean)*(mean0[y][x]-mean0mean);
						mean0counter++;
					}
					if (mean15[y][x]!=0){
						mean15sum+=(mean15[y][x]-mean15mean)*(mean15[y][x]-mean15mean);
						mean15counter++;
					}
				}
			}
			mean0rms=sqrt(mean0sum/(mean0counter-1));
			mean15rms=sqrt(mean15sum/(mean15counter-1));
			std::cout<<"for th_eq bit = 0; mean="<<mean0mean<<" rms= "<<mean0rms<<" counter: "<<mean0counter<<std::endl;
			std::cout<<"for th_eq bit = 15; mean="<<mean15mean<<" rms= "<<mean15rms<<" counter: "<<mean15counter<<std::endl;
			delta0_15 = mean15mean - mean0mean;
			optdelta = 3*mean0rms+3*mean15rms;
			std::cout<<"delta between th eq_0 mean and th_15 mean = "<<delta0_15<<" for ths = "<<ths<<std::endl;
			std::cout<<"optimum delta is 3*rms0 + 3*rms15, such that overlap of both curves is in 4th rms. Optimum delta from this run would be: "<<optdelta<<std::endl;
			// calculate ths for next iteration of while loop
			float newths = 0;
			//last_delta0_15 = 67;
			//last_ths = 100;
			if (last_delta0_15 != 0){ // linear regression. ths - delta is linear. y=a*x+b using (x1,y1) current values, (x2,y2) last values => y3= (D(y)/D(x)*x3+y1-(D(y)/D(x)*x1
				newths = ((last_ths - ths)/(last_delta0_15 - delta0_15))*optdelta+ths-(((last_ths - ths)/(last_delta0_15 - delta0_15))*delta0_15);
			}
			else {
				newths = ths/2; // after first iteration of loop, ths is 127 for first iteration or was set by user
			}
			//store ths value and delta0_15 for next iteration of while loop
			last_ths = ths;
			last_delta0_15 = delta0_15;
			if (newths < 0){
				ths = 127;
			}
			else {
				ths = (short)newths;
			}
			std::cout<<"ths for this iteration was "<<last_ths<<" with delta of "<<last_delta0_15<<std::endl;
			std::cout<<"ths for next iteration will be "<<ths<<std::endl;
			std::cout<<"doTHeq= "<<doTHeq<<std::endl;
		} //while loop end ends when delta0_15 = optdelta +-2

		//store optimised ths in fsr.txt
		std::cout<<"\tSaving optimised ths value in "<<GetFSRFileName(chp)<<"\n> "<<std::flush;
		//for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
			fpga.tp.LoadFSRFromFile(GetFSRFileName(chp),chp);
			fpga.tp.SetDAC(10,chp,ths);
			fpga.tp.SaveFSRToFile(GetFSRFileName(chp),chp);
		//}

		std::cout<<"\tTHS optimisation finished\n> "<<std::flush;
		// De-Allocate memory to prevent memory leak
		for (int i = 0; i < x_length; ++i) {
			for (int j = 0; j < y_length; ++j){
				delete [] p3DArray0[i][j];
				delete [] p3DArray15[i][j];
			}
			delete [] p3DArray0[i];
			delete [] p3DArray15[i];
		}
		delete [] p3DArray0;
		delete [] p3DArray15;
		delete pix_tempdata;

	}
	if (doTHeq==1){
		DoThresholdEqCenter(pix_per_row_THeq,chp,ext_coarse,max_thl,min_thl);
	}
	return 0;
}


int PC::DoThresholdEqCenter(unsigned short pix_per_row, unsigned short chp, short ext_coarse, short max_thl, short min_thl){
//#if DEBUG==2
	std::cout<<"Enter PC::ThresholdEqCenter()"<<std::endl;
//#endif
	int x,y,thl;
	unsigned int coarse;
	#define x_length 256
	#define y_length 256
	#define thl_length 1024+2*372 // thl shift of ~372 for one coarse


	short ***p3DArray0;
	// Allocate memory
	p3DArray0 = new short**[x_length];
	for (int i = 0; i < x_length; ++i) {
		p3DArray0[i] = new short*[y_length];
		for (int j = 0; j < y_length; ++j)
		  p3DArray0[i][j] = new short[thl_length];
	}
	short ***p3DArray15;
	// Allocate memory
	p3DArray15 = new short**[x_length];
	for (int i = 0; i < x_length; ++i) {
		p3DArray15[i] = new short*[y_length];
		for (int j = 0; j < y_length; ++j)
		  p3DArray15[i][j] = new short[thl_length];
	}

	short ***p3DArrayEq;
	// Allocate memory
	p3DArrayEq = new short**[x_length];
	for (int i = 0; i < x_length; ++i) {
		p3DArrayEq[i] = new short*[y_length];
		for (int j = 0; j < y_length; ++j)
		  p3DArrayEq[i][j] = new short[thl_length];
	}
	std::vector<std::vector<std::vector<int> > > *pix_tempdata = new std::vector<std::vector<std::vector<int> > >(9, std::vector < std::vector<int> >(256, std::vector<int>(256,0)));
	// Assign values
	for(coarse=8;coarse>5;coarse--){
		for(y=0;y<256;++y){
			for(x=0;x<256;++x){
				if (coarse == 6){
					for(thl=0;thl<1024;thl++){
						p3DArray0[y][x][thl] = 0;
						p3DArray15[y][x][thl] = 0;
						p3DArrayEq[y][x][thl] = 0;
					}
				}
				else {
					for(thl=1023;thl>=652;thl-=1){
						p3DArray0[y][x][thl+(coarse-6)*372] = 0;
						p3DArray15[y][x][thl+(coarse-6)*372] = 0;
						p3DArrayEq[y][x][thl+(coarse-6)*372] = 0;
					}
				}

			}
		}
	}



	float mean0[256][256] = {0};
	float mean15[256][256] = {0};
	float meanEq[256][256] = {0};
	int sum0[256][256] = {0};
	int sum15[256][256] = {0};
	int sumEq[256][256] = {0};
	int hit_counter0[256][256] = {0};
	int hit_counter15[256][256] = {0};
	int hit_counterEq[256][256] = {0};
	int mean0entries = 0;
	int mean15entries = 0;
	int meanEqentries = 0;
	int summean0 = 0;
	int summean15 = 0;
	int summeanEq = 0;
	int mean0counter = 0;
	int mean15counter = 0;
	int meanEqcounter = 0;
	float mean0mean = 0;
	float mean15mean = 0;
	float meanEqmean = 0;
	float mean0rms = 0;
	float mean15rms = 0;
	float mean0rms_pix = 0;
	float mean15rms_pix = 0;
	float meanEqrms = 0;
	float mean0sum = 0;
	float mean15sum = 0;
	float mean0sum_pix = 0;
	float mean15sum_pix = 0;
	float meanEqsum = 0;
	float delta0_15 = 0;
	float step0_15 = 0;
	short p[256][256] = {0};
	short th_eq_DAC[256][256] = {0};
	std::fstream f;
	/*char TimeName[20]={0}; //if frames should be recorded
	time_t Time_SecondsPassed;
	struct tm * TimeStruct;
	time(&Time_SecondsPassed); TimeStruct=localtime(&Time_SecondsPassed);
	strftime(TimeName,19,"Run%y%m%d_%H-%M-%S",TimeStruct);
	mkdir(DataPathName.c_str(),0755);
	PathName=DataPathName+"/"; PathName+=TimeName;
	mkdir(PathName.c_str(),0755);

	FileName=PathName+"/"; FileName+=FSRFileName;
	fpga.tp.SaveFSRToFile(FileName.c_str());
	FileName=PathName+"/"; FileName+=MatrixFileName;
	fpga.tp.SaveMatrixToFile(FileName.c_str());*/

	for (unsigned int step=0; step<(256/pix_per_row);step++){ //must go from 0 to <(256/pix_per_row)
		for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
			fpga.tp.UniformMatrix(0,0,0,0,0,chip); //mask all pixels on all chips, otherwise they draw current
		}
		fpga.tp.UniformMatrix(0,0,1,1,0,chp); //0,0: Medipix modus
		fpga.tp.Spacing_row(step,pix_per_row,chp);
		fpga.tp.SaveMatrixToFile(GetMatrixFileName(chp),chp);
		fpga.SetMatrix();
		usleep(2000);
		fpga.SerialReadOut(pix_tempdata);
		//usleep(60000 );
		fpga.GeneralReset();
		//usleep(8000);
		for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
			fpga.tp.LoadFSRFromFile(GetFSRFileName(chip),chip);
		}
		fpga.WriteReadFSR();
		//usleep(8000);
		fpga.GeneralReset();
		usleep(2000);
		for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
			fpga.tp.UniformMatrix(0,0,0,0,0,chip); //mask all pixels on all chips, otherwise they draw current
		}
		fpga.tp.UniformMatrix(0,0,1,1,0,chp); //0,0: Medipix modus
		fpga.tp.Spacing_row(step,pix_per_row,chp);
		fpga.tp.SaveMatrixToFile(GetMatrixFileName(chp),chp);
		fpga.SetMatrix();
		usleep(2000);
		fpga.SerialReadOut(pix_tempdata);
		//usleep(60000 );
		std::cout<<"Beginne RauschScan thp=0"<<" step "<<step<<" of "<<(256/pix_per_row)-1<<" from spacing"<<std::endl;
		if (ext_coarse == 1) {
			for(coarse=8;coarse>=6;coarse--){
				//fpga.tp.SetDAC(13,coarse);
				if (coarse == 6){
					for(thl=1023;thl>=0;thl-=1){
						THscan(coarse,thl,thl,0,step,pix_per_row,p3DArray0,sum0,hit_counter0,0,chp);
					}
				}
				else {
					for(thl=1023;thl>=652;thl-=1){
						THscan(coarse,thl,thl+(coarse-6)*372,0,step,pix_per_row,p3DArray0,sum0,hit_counter0,0,chp);
						//std::cout<<"pixel x= 43, y =102, thl= "<<thl+(coarse-6)*372<<" has "<< p3DArray0[y][43][thl+(coarse-6)*372] <<" hits"<<std::endl; //added
					}
				}
			}
		}
		else {
			coarse = 7;
			for(thl=max_thl;thl>=min_thl;thl-=1){
				THscan(coarse,thl,thl,0,step,pix_per_row,p3DArray0,sum0,hit_counter0,0,chp);
			}
		}

		fpga.GeneralReset();
		//usleep(8000);
		for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
			fpga.tp.UniformMatrix(0,0,0,0,0,chip); //mask all pixels on all chips, otherwise they draw current
		}
		fpga.tp.UniformMatrix(0,0,1,1,15,chp); //0,0: Medipix modus
		fpga.tp.Spacing_row(step,pix_per_row,chp);
		fpga.tp.SaveMatrixToFile(GetMatrixFileName(chp),chp);
		fpga.SetMatrix();
		usleep(2000);
		fpga.SerialReadOut(pix_tempdata);
		//usleep(60000 );
		std::cout<<"Beginne RauschScan thp=15"<<" step "<<step<<" of "<<(256/pix_per_row)-1<<" from spacing"<<std::endl;
		if (ext_coarse == 1) {
			for(coarse=8;coarse>=6;coarse--){
				//fpga.tp.SetDAC(13,coarse);
				if (coarse == 6){
					for(thl=1023;thl>=0;thl-=1){
						THscan(coarse,thl,thl,0,step,pix_per_row,p3DArray15,sum15,hit_counter15,15,chp);
						//std::cout<<"sum15 pixel 43,102:: "<<sum15[102][43]<<std::endl;
						//std::cout<<"pixel x= 43, y =102, thl= "<<thl<<" has "<< p3DArray15[y][43][thl] <<" hits"<<std::endl; //added
					}
				}
				else {
					for(thl=1023;thl>=652;thl-=1){
						THscan(coarse,thl,thl+(coarse-6)*372,0,step,pix_per_row,p3DArray15,sum15,hit_counter15,15,chp);
						//std::cout<<"sum15 pixel 43,102:: "<<sum15[102][43]<<std::endl;
						//std::cout<<"pixel x= 43, y =102, thl= "<<thl+(coarse-6)*372<<" has "<< p3DArray15[y][43][thl+(coarse-6)*372] <<" hits"<<std::endl; //added
					}
				}
			}
		}
		else {
			coarse = 7;
			for(thl=max_thl;thl>=min_thl;thl-=1){
				THscan(coarse,thl,thl,0,step,pix_per_row,p3DArray15,sum15,hit_counter15,15,chp);
			}
		}
	}

		f.open("PixelControl.txt",std::fstream::out);
		if(f.is_open()){
			for(coarse=8;coarse>=6;coarse--){
				if (coarse == 6){
					for(thl=1023;thl>=0;thl-=1){
						f<<thl<<"\t"<<p3DArray0[102][43][thl]<<"\t"<<p3DArray0[23][202][thl]<<"\t"<<p3DArray15[102][43][thl]<<"\t"<<p3DArray15[23][202][thl]<<std::endl;
					}
				}
				else{
					for(thl=1023;thl>=651;thl-=1){
						f<<thl+(coarse-6)*372<<"\t"<<p3DArray0[102][43][thl+(coarse-6)*372]<<"\t"<<p3DArray0[23][202][thl+(coarse-6)*372]<<"\t"<<p3DArray15[102][43][thl+(coarse-6)*372]<<"\t"<<p3DArray15[23][202][thl+(coarse-6)*372]<<std::endl;
					}
				}
			}
			f.close();
		}

		for(y=0;y<256;y++){
			for(x=0;x<256;x++){
				if (hit_counter0[y][x]!=0){
					mean0[y][x] = sum0[y][x]/hit_counter0[y][x];
	//				if (mean0[y][x]< 400){ // this is just for Debug
	//					std::cout<<"pixel y: "<< y<<" x: "<<x<<" has mean of "<< mean0[y][x]<<std::endl;
	//				}
					mean0entries += 1;
					summean0 += mean0[y][x];
				}
				if (hit_counter15[y][x]!=0){
					mean15[y][x] = sum15[y][x]/hit_counter15[y][x];
					mean15entries += 1;
					summean15 += mean15[y][x];
				}
			}
		}
		//for pixel 43,102 only to test
		for(coarse=8;coarse>=6;coarse--){
			//fpga.tp.SetDAC(13,coarse);
			if (coarse == 6){
				for(thl=1023;thl>=0;thl-=1){
					mean0sum_pix+=((thl-mean0[102][43])*(thl-mean0[102][43])*p3DArray0[102][43][thl]);
					mean15sum_pix+=((thl-mean15[102][43])*(thl-mean15[102][43])*p3DArray15[102][43][thl]);
				}
			}
			else {
				for(thl=1023;thl>=652;thl-=1){
					mean0sum_pix+=(((thl+(coarse-6)*372)-mean0[102][43])*((thl+(coarse-6)*372)-mean0[102][43])*p3DArray0[102][43][thl+(coarse-6)*372]);
					mean15sum_pix+=(((thl+(coarse-6)*372)-mean15[102][43])*((thl+(coarse-6)*372)-mean15[102][43])*p3DArray15[102][43][thl+(coarse-6)*372]);
				}
			}
		}

	mean0rms_pix=sqrt(mean0sum_pix/(hit_counter0[102][43]-1));
	mean15rms_pix=sqrt(mean15sum_pix/(hit_counter15[102][43]-1));
	std::cout<<"sum0 pixel 43,102:: "<<sum0[102][43]<<std::endl;
	std::cout<<"Total hits pixel 43,102:: "<<hit_counter0[102][43]<<std::endl;
	std::cout<<"mean0 value pixel 43,102: "<<mean0[102][43]<<" rms="<<mean0rms_pix<<std::endl;
	std::cout<<"mean15 value pixel 43,102: "<<mean15[102][43]<<" rms="<<mean15rms_pix<<std::endl;
	if (mean0rms_pix>=75){
		std::cout<<"WARNING: RMS OF MEAN0 FOR TESTPIXEL IS BIGGER THAN 75, SOMETHING MUST BE WRONG, CHECK CHIP POWER SUPPLY. RMS VALUE IS "<<mean0rms_pix<<std::endl;
	}
	if (mean15rms_pix>=75){
		std::cout<<"WARNING: RMS OF MEAN15 FOR TESTPIXEL IS BIGGER THAN 75, SOMETHING MUST BE WRONG, CHECK CHIP POWER SUPPLY. RMS VALUE IS "<<mean15rms_pix<<std::endl;
	}

	//calculate mean of mean0 and mean15 distribution
	if (mean0entries != 0) mean0mean = summean0/mean0entries;
	if (mean15entries != 0) mean15mean = summean15/mean15entries;
	for(y=0;y<256;y++){
		for(x=0;x<256;x++){
			if (mean0[y][x]!=0){
				mean0sum+=(mean0[y][x]-mean0mean)*(mean0[y][x]-mean0mean);
				mean0counter++;
			}
			if (mean15[y][x]!=0){
				mean15sum+=(mean15[y][x]-mean15mean)*(mean15[y][x]-mean15mean);
				mean15counter++;
			}
		}
	}
	mean0rms=sqrt(mean0sum/(mean0counter-1));
	mean15rms=sqrt(mean15sum/(mean15counter-1));

	std::cout<<"for th_eq bit = 0; mean="<<mean0mean<<" rms= "<<mean0rms<<" counter: "<<mean0counter<<std::endl;
	std::cout<<"for th_eq bit = 15; mean="<<mean15mean<<" rms= "<<mean15rms<<" counter: "<<mean15counter<<std::endl;
	delta0_15 = mean15mean - mean0mean;
	step0_15 = delta0_15/16;
	if (step0_15 == 0){
		std::cout<<"step is 0, this will cause an error. Something must be wrong. Step put to 1 instead for this run."<<std::endl;
		step0_15=1;
	}
	std::cout<<"delta between th eq_0 mean and th_15 mean ="<<delta0_15<<std::endl;
	std::cout<<"step size for th_optimisation is "<<step0_15<<" thl"<<std::endl;

	for(y=0;y<256;y++){
		for(x=0;x<256;x++){
			p[y][x] = (mean0[y][x] - mean0mean)/step0_15;
				if (p[y][x]>=8) {th_eq_DAC[y][x] = 0;}
				else if (p[y][x]<=-8) {th_eq_DAC[y][x] = 15;}
				else {th_eq_DAC[y][x] = 8 -p[y][x];}
		}
	}
	std::cout<<"p value for pixel x= 43, y =102 ="<<p[102][43]<<" equal to thl "<<p[102][43]*step0_15<<std::endl;
	std::cout<<"th_eq_DAC for pixel x= 43, y =102 ="<<th_eq_DAC[102][43]<<std::endl;

	// save the final threshold dac mask
	//for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
		f.open(GetThresholdFileName(chp),std::fstream::out);
		if(f.is_open()){
			for(y=0;y<256;y++){
				for(x=0;x<256;x++){
					f<<th_eq_DAC[y][x]<<"\t";
				}
				f<<std::endl;
			}
			f.close();
		}
	//}
	for (unsigned int step=0; step<(256/pix_per_row);step++){ //must go from 0 to <(256/pix_per_row)
		fpga.GeneralReset();
		//usleep(8000);
		//load the threshold dac mask
		for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
			fpga.tp.UniformMatrix(0,0,0,0,0,chip); //mask all pixels on all chips, otherwise they draw current
		}
		fpga.tp.UniformMatrix(0,0,1,1,0,chp);
		fpga.tp.Spacing_row(step,pix_per_row,chp);
		fpga.tp.SaveMatrixToFile(GetMatrixFileName(chp),chp);
		fpga.tp.LoadThresholdFromFile(GetThresholdFileName(chp),chp);
		fpga.tp.SaveMatrixToFile(GetMatrixFileName(chp),chp);
		fpga.SetMatrix();
		usleep(2000);
		fpga.SerialReadOut(pix_tempdata);
		//usleep(60000 );
		std::cout<<"Beginne RauschScan mit equalisierter Matrix, step "<<step<<" of "<<(256/pix_per_row)-1<<" from spacing"<<std::endl;
		if (ext_coarse == 1) {
			for(coarse=8;coarse>=6;coarse--){
				//fpga.tp.SetDAC(13,coarse);
				if (coarse == 6){
					for(thl=1023;thl>=0;thl-=1){
						THscan(coarse,thl,thl,0,step,pix_per_row,p3DArrayEq,sumEq,hit_counterEq,16,chp);
						//std::cout<<"sum15 pixel 43,102:: "<<sum15[102][43]<<std::endl;
						//std::cout<<"pixel x= 43, y =102, thl= "<<thl<<" has "<< p3DArrayEq[102][43][thl] <<" hits"<<std::endl; //added
					}
				}
				else {
					for(thl=1023;thl>=652;thl-=1){
						THscan(coarse,thl,thl+(coarse-6)*372,0,step,pix_per_row,p3DArrayEq,sumEq,hit_counterEq,16,chp);
						//std::cout<<"sum15 pixel 43,102:: "<<sum15[102][43]<<std::endl;
						//std::cout<<"pixel x= 43, y =102, thl= "<<thl+(coarse-6)*372<<" has "<< p3DArrayEq[102][43][thl+(coarse-6)*372] <<" hits"<<std::endl; //added
					}
				}
			}
		}
		else {
			coarse = 7;
			for(thl=max_thl;thl>=min_thl;thl-=1){
				THscan(coarse,thl,thl,0,step,pix_per_row,p3DArrayEq,sumEq,hit_counterEq,16,chp);
			}
		}
	}
		for(y=0;y<256;y++){
			for(x=0;x<256;x++){
				if (hit_counterEq[y][x]>0){
					meanEq[y][x] = sumEq[y][x]/hit_counterEq[y][x];
					meanEqentries += 1;
					summeanEq += meanEq[y][x];
					if (meanEq[y][x]!=0){
						meanEqcounter++;
					}
				}
			}
		}

		f.open("Pixelcontrol2.txt",std::fstream::out);
		if(f.is_open()){
			for(coarse=8;coarse>=6;coarse--){
				if (coarse == 6){
					for(thl=1023;thl>=0;thl-=1){
						//f<<thl<<"\t"<<p3DArray0[y][x][thl]<<"\t"<<p3DArray0[23][202][thl]<<"\t"<<p3DArray15[y][x][thl]<<"\t"<<p3DArray15[23][202][thl]<<std::endl;
						f<<thl<<"\t"<<p3DArray0[102][43][thl]<<"\t"<<p3DArray0[23][202][thl]<<"\t"<<p3DArray15[102][43][thl]<<"\t"<<p3DArray15[23][202][thl]<<"\t"<<p3DArrayEq[102][43][thl]<<"\t"<<p3DArrayEq[23][202][thl]<<std::endl;
					}
				}
				else{
					for(thl=1023;thl>=651;thl-=1){
						//f<<thl+(coarse-6)*372<<"\t"<<p3DArray0[y][x][thl+(coarse-6)*372]<<"\t"<<p3DArray0[23][202][thl+(coarse-6)*372]<<"\t"<<p3DArray15[y][x][thl+(coarse-6)*372]<<"\t"<<p3DArray15[23][202][thl+(coarse-6)*372]<<std::endl;
						f<<thl+(coarse-6)*372<<"\t"<<p3DArray0[102][43][thl+(coarse-6)*372]<<"\t"<<p3DArray0[23][202][thl+(coarse-6)*372]<<"\t"<<p3DArray15[102][43][thl+(coarse-6)*372]<<"\t"<<p3DArray15[23][202][thl+(coarse-6)*372]<<"\t"<<p3DArrayEq[102][43][thl+(coarse-6)*372]<<"\t"<<p3DArrayEq[23][202][thl+(coarse-6)*372]<<std::endl;
					}
				}
			}
			f.close();
		}
		meanEqmean = summeanEq/meanEqcounter;
		for(y=0;y<256;y++){
			for(x=0;x<256;x++){
				if (meanEq[y][x]>0){
					meanEqsum+=(meanEq[y][x]-meanEqmean)*(meanEq[y][x]-meanEqmean);
				}
			}
		}
		meanEqrms=sqrt(meanEqsum/(meanEqcounter-1));
		std::cout<<"For equalisation: meanEq is: "<<meanEqmean<<" rms is: "<<meanEqrms<<" counter: "<<meanEqcounter<<std::endl;

		f.open("ThresholdMeans.txt",std::fstream::out);
		if(f.is_open()){
			for(y=0;y<256;y++){
				for(x=0;x<256;x++){
					//if ((fpga.tp.GetMask(y,x))==1){
						f<<y<<"\t"<<x<<"\t"<<mean0[y][x]<<"\t"<<mean15[y][x]<<"\t"<<th_eq_DAC[y][x]<<"\t"<<meanEq[y][x]<<std::endl;
					//}
				}
			}
			f.close();
		}



	std::cout<<"\tThresholdNoise finished\n> "<<std::flush;
	// De-Allocate memory to prevent memory leak
	for (int i = 0; i < x_length; ++i) {
		for (int j = 0; j < y_length; ++j){
			delete [] p3DArray0[i][j];
			delete [] p3DArray15[i][j];
			delete [] p3DArrayEq[i][j];
		}
		delete [] p3DArray0[i];
		delete [] p3DArray15[i];
		delete [] p3DArrayEq[i];
	}
	delete [] p3DArray0;
	delete [] p3DArray15;
	delete [] p3DArrayEq;
	delete pix_tempdata;
	return 0;
}


int PC::TOCalibFast(unsigned short pix_per_row, unsigned short shuttertype, unsigned short time, unsigned short TOT, unsigned short internalPulser){
	std::fstream f;
	bool next_voltage = true;
	std::vector<int> voltage;
	std::vector<int> meanTOT1; std::vector<double> stddevTOT1;
	std::vector<int> meanTOT2; std::vector<double> stddevTOT2;
	std::vector<int> meanTOT3; std::vector<double> stddevTOT3;
	std::vector<int> meanTOT4; std::vector<double> stddevTOT4;
	std::vector<int> meanTOT5; std::vector<double> stddevTOT5;
	std::vector<int> meanTOT6; std::vector<double> stddevTOT6;
	std::vector<int> meanTOT7; std::vector<double> stddevTOT7;
	std::vector<int> meanTOT8; std::vector<double> stddevTOT8;

	int i = 0;

	while(next_voltage)
	{
		int myint = 0;
		if (internalPulser == 0) {
			std::cout << "What is the voltage you set at the external pulser in mV?"<<std::endl;
			std::string ein5="";
			std::getline(std::cin,ein5);
			if(ein5==""){myint=0;}
			else if(ein5.find_first_not_of("0123456789 ",0)==ein5.npos){myint=(int) atoi(ein5.data());}
			else{std::cout<<"(Run)\t Non-numerical sign -> abort"<<std::endl; return 0;}
		}
		if (internalPulser == 1) {
			if (i == 0) { myint = 370;}
			else if (i == 1) { myint = 380;}
			else if (i == 2) { myint = 390;}
			else if (i == 3) { myint = 400;}
			else if (i == 4) { myint = 410;}
			else if (i == 5) { myint = 420;}
			else if (i == 6) { myint = 430;}
			else if (i == 7) { myint = 440;}
			else if (i == 8) { myint = 450;}
			else if (i == 9) { myint = 500;}
			else if (i == 10) { myint = 550;}
			else if (i == 11) { myint = 650;}
			else if (i == 12) { myint = 750;}
			else if (i == 13) { myint = 850;}
			else {myint = 350;}
			fpga.i2cDAC(myint,3);
			fpga.i2cDAC(350,2);
			fpga.tpulse(1,50);
		}
		voltage.push_back(myint);
		int meanTOT_[9] = {0};
		double stddevTOT_[9] = {0};
		double sumstddev_iteration[9][4]={0};
		for (unsigned int iteration=0; iteration<4; iteration++){
			if (internalPulser == 1) {
				if (TALKATIVNESS == 0 ){
					std::cout << "Voltage "<<i<<" of 13; iteration "<<iteration<<" of "<< 3<<std::endl;
				}
			}
			if (internalPulser == 0) {
				if (TALKATIVNESS == 0 ) {
					std::cout << "Iteration "<<iteration<<" of "<< 3<<std::endl;
				}
			}
			int meanTOT_ctpr[9] = {0};
			double sumstddev_iteration_ctpr[9][256]={0};
			double sumctpr[9][4]={0};
			for (unsigned int step=0; step<(256/pix_per_row);step++){
				double stddevTOT_iteration_spacing[9][32] = {0};
				int meanTOT_iteration[9] = {0};
				//fpga.GeneralReset();
				for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
					if (TOT == 3) { fpga.tp.UniformMatrix(1,0,0,0,0,chip);} //1,0: TOT modus, Mask and test pulse are set later
					else if (TOT == 4) { fpga.tp.UniformMatrix(1,1,0,0,0,chip);} //1,1: TIME modus
					else {fpga.tp.UniformMatrix(1,TOT,0,0,0,chip);} //1,0: TOT modus
					fpga.tp.Spacing_row(step,pix_per_row,chip);
					fpga.tp.Spacing_row_TPulse(step,pix_per_row,chip);
					//fpga.tp.SpacingCalib(CTPR,step,pix_per_row,chip);
					fpga.tp.LoadThresholdFromFile(GetThresholdFileName(chip),chip);
					fpga.tp.SaveMatrixToFile(GetMatrixFileName(chip),chip);
				}
				fpga.WriteReadFSR();
				int pix_tempdata2[9][256][256]= {0};
				fpga.SetMatrix();
				usleep(2000 );
				fpga.SerialReadOut(pix_tempdata2);
				fpga.EnableTPulse(1);
				for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
					fpga.tp.SetDAC(14,chip,0);
				}
				for (unsigned int CTPR=0;CTPR<32;CTPR++){ //must go from 0 to <(256/pix_per_row)
					for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
						fpga.tp.LoadFSRFromFile(GetFSRFileName(chip),chip);
						unsigned int CTPRval = 1<<CTPR;
						fpga.tp.SetDAC(14,chip,CTPRval);
					}
					fpga.WriteReadFSR();
					fpga.WriteReadFSR();
					usleep(400 );
					int result = 0;
					if (shuttertype==1){
						fpga.CountingTime(time);
					}
					else{
						fpga.CountingTime_long(time);
					}
					int meanTOT_iteration_spacing[9] = {0};
					if (TOT == 3 or TOT == 4) {
						for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
							std::string filename_; //if want to write to file
							std::ostringstream sstream;
							sstream<<"TOTCalib1"<<"_iteration"<<iteration<<"_step"<<step<<"_chip"<<chip<<".txt";
							filename_=DataPathName+"/"+"TOT"+"/"; filename_+=sstream.str();
							fpga.DataChipFPGA(result);
							DoReadOut2(filename_.c_str(),chip);
						}
					}
					else {
						fpga.DataChipFPGA(result);
						for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
							int sumTOT = 0;
							int pix_tempdata[256][256]= {0};
							int hitcounter = 0;
							int hitcounter_real = 0;
							fpga.DataFPGAPC(pix_tempdata,chip);
							for(short y=step;y<256;y+=(256/pix_per_row)){
                                                                for(short x=CTPR;x<256;x+=32){//short x=CTPR;x<256;x+=32
									if((pix_tempdata[y][x])!=0 && (pix_tempdata[y][x])!=11810 && fpga.tp.GetMask(y,x,chip)==1){
										sumTOT+=pix_tempdata[y][x];
										hitcounter_real++;
                                                                                //if (CTPR ==0)std::cout << "y "<<y<<" x "<<x<<" hits "<<pix_tempdata[y][x]<<std::endl;
									}
									else{
										sumTOT+=0;
										hitcounter_real = hitcounter_real;
									}
								}
							}
							//std::cout << "Hits "<<hitcounter<<std::endl;
							if (hitcounter_real == 0 or hitcounter_real == 1 ) {
								hitcounter = 2;
								meanTOT_iteration_spacing[chip] = sumTOT/hitcounter;
							}
							else {
								hitcounter = hitcounter_real;
								meanTOT_iteration_spacing[chip] = sumTOT/hitcounter;
							}
							double sumstddev = 0;
							for(short y=step;y<256;y+=(256/pix_per_row)){
								for(short x=CTPR;x<256;x+=32){
									if (pix_tempdata[y][x] != 0) {
										sumstddev+=(((pix_tempdata[y][x]-meanTOT_iteration_spacing[chip])*(pix_tempdata[y][x]-meanTOT_iteration_spacing[chip]))/(hitcounter-1));
									}
								}
							}
                                                        stddevTOT_iteration_spacing[chip][CTPR]=sqrt(sumstddev);
                                                        if (internalPulser == 1) {
                                                        	if (TALKATIVNESS == 1){
                                                            	std::cout << "Voltage "<<i<<" of 13; step "<<step<<" of "<< (256/pix_per_row)-1<<"; CTPR iteration "<<CTPR<<" of 31;"<<"; frame "<<iteration<<" of 3"<<"; Chip "<<chip<<"; meanTOT: "<<meanTOT_iteration_spacing[chip]<<" stddevTOT: "<<stddevTOT_iteration_spacing[chip][step]<<" with "<<hitcounter_real<<" hits"<<std::endl;
                                                        	}
                                                        }
                                                        if (internalPulser == 0) {
                                                        	if (TALKATIVNESS == 1){
                                                        		std::cout << "Step "<<step<<" of "<< (256/pix_per_row)-1<<"; CTPR iteration "<<CTPR<<" of 31;"<<"; frame "<<iteration<<" of 3"<<"; Chip "<<chip<<"; meanTOT: "<<meanTOT_iteration_spacing[chip]<<" stddevTOT: "<<stddevTOT_iteration_spacing[chip][step]<<" with "<<hitcounter_real<<" hits"<<std::endl;
                                                        	}
                                                        }

						} //chip
					} //TOT
					for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
						meanTOT_iteration[chip] += meanTOT_iteration_spacing[chip];
					}
				} // spacingCTPR
				double sumstd_iteration[9][4] = {0};				
				for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
					for (unsigned int ctpr=0; ctpr<32;ctpr++){
                                                sumstd_iteration[chip][iteration]+=(stddevTOT_iteration_spacing[chip][ctpr]*stddevTOT_iteration_spacing[chip][ctpr])/(32-1);
					}
					meanTOT_ctpr[chip] += meanTOT_iteration[chip]/32;
					sumstddev_iteration_ctpr[chip][step] = sqrt(sumstd_iteration[chip][iteration]);
				}
				fpga.EnableTPulse(0);
			}//spacing row
			for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
				meanTOT_[chip] += meanTOT_ctpr[chip]/(256/pix_per_row);
				for (unsigned int step=0; step<(256/pix_per_row);step++){
					sumctpr[chip][iteration]+=(sumstddev_iteration_ctpr[chip][step]*sumstddev_iteration_ctpr[chip][step])/((256/pix_per_row)-1);
				}
				sumstddev_iteration[chip][iteration] =sqrt(sumctpr[chip][iteration]);
			}
			//sumstddev_iteration[chip][iteration] hier erzeugen aus sumstddev_iteration_ctpr[chip][iteration] der einzelnen ctprs
		} // iterations
		//if (TOT == 0){
			for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
				double sumstd[9]={0};
				for (unsigned int iteration=0; iteration<4; iteration++){
					sumstd[chip]+=(sumstddev_iteration[chip][iteration]*sumstddev_iteration[chip][iteration])/3;
				}
				stddevTOT_[chip] = sqrt(sumstd[chip]);
			}
			meanTOT1.push_back(meanTOT_[1]/4);stddevTOT1.push_back(stddevTOT_[1]);
			if (fpga.tp.GetNumChips() > 1) {meanTOT2.push_back(meanTOT_[2]/4);stddevTOT2.push_back(stddevTOT_[2]);}
			if (fpga.tp.GetNumChips() > 2) {meanTOT3.push_back(meanTOT_[3]/4);stddevTOT3.push_back(stddevTOT_[3]);}
			if (fpga.tp.GetNumChips() > 3) {meanTOT4.push_back(meanTOT_[4]/4);stddevTOT4.push_back(stddevTOT_[4]);}
			if (fpga.tp.GetNumChips() > 4) {meanTOT5.push_back(meanTOT_[5]/4);stddevTOT5.push_back(stddevTOT_[5]);}
			if (fpga.tp.GetNumChips() > 5) {meanTOT6.push_back(meanTOT_[6]/4);stddevTOT6.push_back(stddevTOT_[6]);}
			if (fpga.tp.GetNumChips() > 6) {meanTOT7.push_back(meanTOT_[7]/4);stddevTOT7.push_back(stddevTOT_[7]);}
			if (fpga.tp.GetNumChips() > 7) {meanTOT8.push_back(meanTOT_[8]/4);stddevTOT8.push_back(stddevTOT_[8]);}
		//}
			if (internalPulser == 0) {
				std::cout << "Do you want to record data for another voltage? (0 = no, 1 = yes)"<<std::endl;
				std::string ein4="";
				std::getline(std::cin,ein4);
				if(ein4==""){next_voltage=false;}
				else if(ein4.find_first_not_of("0123456789 ",0)==ein4.npos){next_voltage=(bool) atoi(ein4.data());}
				else{std::cout<<"(Run)\t Non-numerical sign -> abort"<<std::endl; return 0;}
			}
			if (internalPulser == 1) {
				next_voltage=true;
				if (i == 13 ) next_voltage=false;
			}
			i++;
	}
	//if (TOT == 0){
		std::cout << "Vector Voltages size: "<<voltage.size()<<std::endl;
		for (unsigned int j = 0; j < voltage.size(); j++){
			std::cout << "Voltage "<<j<<": "<<voltage[j]<<" with mean TOT (chip 1)"<<meanTOT1[j]<<std::endl;
		}
		for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
			if (TOT == 0) {f.open(GetTOTCalibFileName(chip),std::fstream::out);}
			else f.open(GetTOACalibFileName(chip),std::fstream::out);
			if(f.is_open()){
				for(unsigned int j = 0; j < voltage.size(); j++){
					if (chip == 1){f<<voltage[j]-350<<"\t"<<meanTOT1[j]<<"\t"<<stddevTOT1[j]<<std::endl;}
					else if (chip == 2){f<<voltage[j]-350<<"\t"<<meanTOT2[j]<<"\t"<<stddevTOT2[j]<<std::endl;}
					else if (chip == 3){f<<voltage[j]-350<<"\t"<<meanTOT3[j]<<"\t"<<stddevTOT3[j]<<std::endl;}
					else if (chip == 4){f<<voltage[j]-350<<"\t"<<meanTOT4[j]<<"\t"<<stddevTOT4[j]<<std::endl;}
					else if (chip == 5){f<<voltage[j]-350<<"\t"<<meanTOT5[j]<<"\t"<<stddevTOT5[j]<<std::endl;}
					else if (chip == 6){f<<voltage[j]-350<<"\t"<<meanTOT6[j]<<"\t"<<stddevTOT6[j]<<std::endl;}
					else if (chip == 7){f<<voltage[j]-350<<"\t"<<meanTOT7[j]<<"\t"<<stddevTOT7[j]<<std::endl;}
					else if (chip == 8){f<<voltage[j]-350<<"\t"<<meanTOT8[j]<<"\t"<<stddevTOT8[j]<<std::endl;}
				}
				f.close();
			}
		}
	//}


	return 0;
}


int PC::TOCalib(){
	unsigned short pix_per_row = 1;
	unsigned short shuttertype = 1;
	unsigned short time = 1;
	unsigned short TOT = 0;
	std::fstream f;
	std::cout << "TOT (0) or TOA (1)? "<<std::endl;
	std::string ein0="";
	std::getline(std::cin,ein0);
	if(ein0==""){TOT=0;}
	else if(ein0.find_first_not_of("0123456789 ",0)==ein0.npos){TOT=(unsigned short) atoi(ein0.data());}
	else{std::cout<<"(Run)\t Non-numerical sign -> abort"<<std::endl; return 0;}
	std::cout<<TOT<<" selected"<<std::endl;
	std::cout << "Hello this is TOT/TOA calibration. You will have to specify spacing and shutter length. I will then ask you a voltage you set on the external pulser and record 4 frames(times pixel per row). Then ask you if you want to do another voltage. "<< std::endl;
	std::cout<<"You have "<< fpga.tp.GetNumChips() <<" chips, all of them will be calibrated at the same time. "<<std::endl;
	std::cout << "For the spacing: How many pixel per row at same time? 1,2,4,8,16 (more is not good!)? "<<std::endl;
	std::string ein="";
	std::getline(std::cin,ein);
	if(ein==""){pix_per_row=1;}
	else if(ein.find_first_not_of("0123456789 ",0)==ein.npos){pix_per_row=(unsigned short) atoi(ein.data());}
	else{std::cout<<"(Run)\t Non-numerical sign -> abort"<<std::endl; return 0;}
	std::cout<<pix_per_row<<" pixel per row at same time"<<std::endl;
	std::cout << "Shutter length: press 1 for short shutter (0-255 clock cycles); press 2 for long shutter (256 - 65280 clock cycles)"<<std::endl;
	std::string ein2="";
	std::getline(std::cin,ein2);
	if(ein2==""){shuttertype=1;}
	else if(ein2.find_first_not_of("0123456789 ",0)==ein2.npos){shuttertype=(unsigned short) atoi(ein2.data());}
	else{std::cout<<"(Run)\t Non-numerical sign -> abort"<<std::endl; return 0;}
	std::cout << "Set shutter length in clock cycles(0 -255) (if you have chosen long shutter this value will be multiplied by 256):"<<std::endl;
	std::string ein3="";
	std::getline(std::cin,ein3);
	if(ein3==""){time=1;}
	else if(ein3.find_first_not_of("0123456789 ",0)==ein3.npos){time=(unsigned short) atoi(ein3.data());}
	else{std::cout<<"(Run)\t Non-numerical sign -> abort"<<std::endl; return 0;}
	std::cout << "Ok, lets start!"<<std::endl;

	bool next_voltage = true;
	std::vector<int> voltage;
	std::vector<int> meanTOT1; std::vector<double> stddevTOT1;
	std::vector<int> meanTOT2; std::vector<double> stddevTOT2;
	std::vector<int> meanTOT3; std::vector<double> stddevTOT3;
	std::vector<int> meanTOT4; std::vector<double> stddevTOT4;
	std::vector<int> meanTOT5; std::vector<double> stddevTOT5;
	std::vector<int> meanTOT6; std::vector<double> stddevTOT6;
	std::vector<int> meanTOT7; std::vector<double> stddevTOT7;
	std::vector<int> meanTOT8; std::vector<double> stddevTOT8;

	int i = 0;

	while(next_voltage)
	{
		int myint = 0;
		std::cout << "What is the voltage you set at the external pulser in mV?"<<std::endl;
		std::string ein5="";
		std::getline(std::cin,ein5);
		if(ein5==""){myint=0;}
		else if(ein5.find_first_not_of("0123456789 ",0)==ein.npos){myint=(int) atoi(ein5.data());}
		else{std::cout<<"(Run)\t Non-numerical sign -> abort"<<std::endl; return 0;}
		voltage.push_back(myint);
		int meanTOT_[9] = {0};
		double stddevTOT_[9] = {0};
		double sumstddev_iteration[9][4]={0};
		for (unsigned int iteration=0; iteration<4; iteration++){
			int meanTOT_ctpr[9] = {0};
			double sumstddev_iteration_ctpr[9][32]={0};
			double sumctpr[9][4]={0};
			for (unsigned int CTPR=0;CTPR<32;CTPR++){
				double stddevTOT_iteration_spacing[9][256] = {0};
				int meanTOT_iteration[9] = {0};
				for (unsigned int step=0; step<(256/pix_per_row);step++){ //must go from 0 to <(256/pix_per_row)
					fpga.GeneralReset();
					for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
						if (TOT == 3) { fpga.tp.UniformMatrix(1,0,0,0,0,chip);} //1,0: TOT modus, Mask and test pulse are set later
						else if (TOT == 4) { fpga.tp.UniformMatrix(1,1,0,0,0,chip);} //1,1: TIME modus
						else {fpga.tp.UniformMatrix(1,TOT,0,0,0,chip);} //1,0: TOT modus
                        	                //fpga.tp.Spacing_row(step,pix_per_row,chip);
						//fpga.tp.Spacing_row_TPulse(step,pix_per_row,chip);
						fpga.tp.SpacingCalib(CTPR,step,pix_per_row,chip);
						fpga.tp.LoadThresholdFromFile(GetThresholdFileName(chip),chip);
						fpga.tp.SaveMatrixToFile(GetMatrixFileName(chip),chip);
						fpga.tp.LoadFSRFromFile(GetFSRFileName(chip),chip);
						unsigned int CTPRval = 1<<CTPR;
						fpga.tp.SetDAC(14,chip,CTPRval);
					}
					fpga.WriteReadFSR();
					fpga.WriteReadFSR();
					int pix_tempdata2[9][256][256]= {0};
					int result = 0;
					fpga.SetMatrix();
					usleep(2000 );
					fpga.SerialReadOut(pix_tempdata2);
					fpga.EnableTPulse(1);
					if (shuttertype==1){
						fpga.CountingTime(time);
					}
					else{
						fpga.CountingTime_long(time);
					}
					int meanTOT_iteration_spacing[9] = {0};
					if (TOT == 3 or TOT == 4) {
						for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
							std::string filename_; //if want to write to file
							std::ostringstream sstream;
							sstream<<"TOTCalib1"<<"_iteration"<<iteration<<"_step"<<step<<"_chip"<<chip<<".txt";
							filename_=DataPathName+"/"+"TOT"+"/"; filename_+=sstream.str();
							fpga.DataChipFPGA(result);
							DoReadOut2(filename_.c_str(),chip);
						}
					}
					else {
						fpga.DataChipFPGA(result);
						for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
							int sumTOT = 0;
							int pix_tempdata[256][256]= {0};
							int hitcounter = 0;
							int hitcounter_real = 0;
							fpga.DataFPGAPC(pix_tempdata,chip);
							for(short y=step;y<256;y+=(256/pix_per_row)){
								for(short x=0;x<256;x++){
									if((pix_tempdata[y][x])!=0 && (pix_tempdata[y][x])!=11810 && fpga.tp.GetMask(y,x,chip)==1){
										sumTOT+=pix_tempdata[y][x];
										//std::cout << "hits chip " << chip << " : "<< pix_tempdata[y][x] << std::endl;
										hitcounter_real++;
									}
									else{
										sumTOT+=0;
										hitcounter_real = hitcounter_real;
									}
								}
							}
							//std::cout << "Hits "<<hitcounter<<std::endl;
							if (hitcounter_real == 0 or hitcounter_real == 1 ) {
								hitcounter = 2;
								meanTOT_iteration_spacing[chip] = sumTOT/hitcounter;
							}
							else {
								hitcounter = hitcounter_real;
								meanTOT_iteration_spacing[chip] = sumTOT/hitcounter;
							}
							double sumstddev = 0;
							for(short y=step;y<256;y+=(256/pix_per_row)){
								for(short x=CTPR;x<256;x+=32){
									if (pix_tempdata[y][x] != 0) {
										sumstddev+=(((pix_tempdata[y][x]-meanTOT_iteration_spacing[chip])*(pix_tempdata[y][x]-meanTOT_iteration_spacing[chip]))/(hitcounter-1));
									}
								}
							}
							stddevTOT_iteration_spacing[chip][step]=sqrt(sumstddev);
							std::cout << "Step "<<step<<" of "<< (256/pix_per_row)-1<<"; CTPR iteration "<<CTPR<<" of 31;"<<"; frame "<<iteration<<" of 3"<<"; Chip "<<chip<<"; meanTOT: "<<meanTOT_iteration_spacing[chip]<<" stddevTOT: "<<stddevTOT_iteration_spacing[chip][step]<<" with "<<hitcounter_real<<" hits"<<std::endl;
						} //chip
					} //TOT
					for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
						meanTOT_iteration[chip] += meanTOT_iteration_spacing[chip];
					}
					fpga.EnableTPulse(0);
				} // spacing
				double sumstd_iteration[9][4] = {0};				
				for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
					for (unsigned int step=0; step<(256/pix_per_row);step++){
						sumstd_iteration[chip][iteration]+=(stddevTOT_iteration_spacing[chip][step]*stddevTOT_iteration_spacing[chip][step])/((256/pix_per_row)-1);
					}
					meanTOT_ctpr[chip] += meanTOT_iteration[chip]/(256/pix_per_row);
					sumstddev_iteration_ctpr[chip][CTPR] = sqrt(sumstd_iteration[chip][iteration]);
				}
			}//spacing CTPR
			for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
				meanTOT_[chip] += meanTOT_ctpr[chip]/32;
				for (unsigned int ctpr=0; ctpr<32;ctpr++){
					sumctpr[chip][iteration]+=(sumstddev_iteration_ctpr[chip][ctpr]*sumstddev_iteration_ctpr[chip][ctpr])/(32-1);
				}
				sumstddev_iteration[chip][iteration] =sqrt(sumctpr[chip][iteration]);
			}
			//sumstddev_iteration[chip][iteration] hier erzeugen aus sumstddev_iteration_ctpr[chip][iteration] der einzelnen ctprs
		} // iterations
		//if (TOT == 0){
			for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
				double sumstd[9]={0};
				for (unsigned int iteration=0; iteration<4; iteration++){
					sumstd[chip]+=(sumstddev_iteration[chip][iteration]*sumstddev_iteration[chip][iteration])/3;
				}
				stddevTOT_[chip] = sqrt(sumstd[chip]);
			}
			meanTOT1.push_back(meanTOT_[1]/4);stddevTOT1.push_back(stddevTOT_[1]);
			if (fpga.tp.GetNumChips() > 1) {meanTOT2.push_back(meanTOT_[2]/4);stddevTOT2.push_back(stddevTOT_[2]);}
			if (fpga.tp.GetNumChips() > 2) {meanTOT3.push_back(meanTOT_[3]/4);stddevTOT3.push_back(stddevTOT_[3]);}
			if (fpga.tp.GetNumChips() > 3) {meanTOT4.push_back(meanTOT_[4]/4);stddevTOT4.push_back(stddevTOT_[4]);}
			if (fpga.tp.GetNumChips() > 4) {meanTOT5.push_back(meanTOT_[5]/4);stddevTOT5.push_back(stddevTOT_[5]);}
			if (fpga.tp.GetNumChips() > 5) {meanTOT6.push_back(meanTOT_[6]/4);stddevTOT6.push_back(stddevTOT_[6]);}
			if (fpga.tp.GetNumChips() > 6) {meanTOT7.push_back(meanTOT_[7]/4);stddevTOT7.push_back(stddevTOT_[7]);}
			if (fpga.tp.GetNumChips() > 7) {meanTOT8.push_back(meanTOT_[8]/4);stddevTOT8.push_back(stddevTOT_[8]);}
		//}

			std::cout << "Do you want to record data for another voltage? (0 = no, 1 = yes)"<<std::endl;
			std::string ein4="";
			std::getline(std::cin,ein4);
			if(ein4==""){next_voltage=false;}
			else if(ein4.find_first_not_of("0123456789 ",0)==ein.npos){next_voltage=(bool) atoi(ein4.data());}
			else{std::cout<<"(Run)\t Non-numerical sign -> abort"<<std::endl; return 0;}
			i++;
	}
	//if (TOT == 0){
		std::cout << "Vector Voltages size: "<<voltage.size()<<std::endl;
		for (unsigned int j = 0; j < voltage.size(); j++){
			std::cout << "Voltage "<<j<<": "<<voltage[j]<<" with mean TOT (chip 1)"<<meanTOT1[j]<<std::endl;
		}
		for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
			if (TOT == 0) {f.open(GetTOTCalibFileName(chip),std::fstream::out);}
			else f.open(GetTOACalibFileName(chip),std::fstream::out);
			if(f.is_open()){
				for(unsigned int j = 0; j < voltage.size(); j++){
					if (chip == 1){f<<voltage[j]<<"\t"<<meanTOT1[j]<<"\t"<<stddevTOT1[j]<<std::endl;}
					else if (chip == 2){f<<voltage[j]<<"\t"<<meanTOT2[j]<<"\t"<<stddevTOT2[j]<<std::endl;}
					else if (chip == 3){f<<voltage[j]<<"\t"<<meanTOT3[j]<<"\t"<<stddevTOT3[j]<<std::endl;}
					else if (chip == 4){f<<voltage[j]<<"\t"<<meanTOT4[j]<<"\t"<<stddevTOT4[j]<<std::endl;}
					else if (chip == 5){f<<voltage[j]<<"\t"<<meanTOT5[j]<<"\t"<<stddevTOT5[j]<<std::endl;}
					else if (chip == 6){f<<voltage[j]<<"\t"<<meanTOT6[j]<<"\t"<<stddevTOT6[j]<<std::endl;}
					else if (chip == 7){f<<voltage[j]<<"\t"<<meanTOT7[j]<<"\t"<<stddevTOT7[j]<<std::endl;}
					else if (chip == 8){f<<voltage[j]<<"\t"<<meanTOT8[j]<<"\t"<<stddevTOT8[j]<<std::endl;}
				}
				f.close();
			}
		}
	//}


	return 0;
}


unsigned short PC::CheckOffset(){
	for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
		fpga.tp.VarChessMatrix(254,254,1,1,1,1,8,0,1,1,1,7,chip);
	}
	unsigned short oldpreload = fpga.tp.GetPreload();
	unsigned short usepreload = 0;
	unsigned short errorsbefore = 512;
	unsigned short  chips = fpga.tp.GetNumChips();
	for (unsigned short preload = 0; preload <= 5; preload ++){
		unsigned short errors = 0;
		for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++){
			fpga.tp.SetNumChips(chips,preload);
			fpga.WriteReadFSR();
			fpga.WriteReadFSR();
			fpga.SetMatrix();
			std::vector<int> *data = new std::vector<int>((12288+1),0); //+1: Entry 0 of Vector contains NumHits
			int result;
			fpga.DataChipFPGA(result);
			int hits=fpga.DataFPGAPC(data,chip);
			for(int i=0; i<hits*3; i=i+3){
				if ( (*data)[i+1] == 255 or (*data)[i+1] == 254) {
					if ((*data)[i+1+1 ] < 254){
						if ((*data)[i+2+1] != 597){
							errors ++;
						}
					}
				}
				if ( (*data)[i+1] < 254 ){
					if ((*data)[i+1+1 ] < 254){
						if ((*data)[i+2+1] != 10001){
							errors ++;
						}
					}
				}
			}
			delete data;
		}
		std::cout << "Preload " << preload <<"; Errors " << errors << std::endl;
		if (errors < errorsbefore){
			usepreload = preload;
		}
		errorsbefore = errors;
	}
	std::cout << "Preload was " << oldpreload  << std::endl;
	if (usepreload != oldpreload){
		std::cout << "WARNING: optimum preload has changed !!! "<< std::endl;
		std::cout << "Preload was " << oldpreload  << std::endl;
		std::cout << "Optimum preload is  " << usepreload << std::endl;
		std::cout << "You can change it using the SetNumChips command" << std::endl;
	}
	fpga.tp.SetNumChips(fpga.tp.GetNumChips(),oldpreload);
	return usepreload;
}


void PC::Histogramm(int hist[16384], int pix[256][256], int* m, int* s, int* a){
	int x,y;
	int l,r;
	
	//Histogramm
	for(y=0;y<256;++y){
		for(x=0;x<256;++x){
			if((pix[y][x]>=0)&&(pix[y][x]<16384)){hist[pix[y][x]]++;}
		}
	}
	
	//Mittelwert
	*m=0; *a=0;
	for(y=16383;y>=0;--y){
		if(hist[y]>*a){*a=hist[y]; *m=y;}
	}
	
	//Breite
	l=-1; r=-1;
	for(y=1;y<16384;++y){
		if( (l<0) && (hist[y]>*a * 0.6065) ){l=y;}
		if( (r<0) && (hist[16384-y]>*a * 0.6065) ){r=16384-y;}
	}
	std::cout<<"(Histogramm)\tMaximum bei m="<<*m<<" mit a="<<*a<<" Grenzen: "<<l<<","<<r<<std::endl;
	*s=r-l;
	if((l>=0)&&(r>=0)){*m=(l+r)/2;}
	return;
}

void PC::Histogramm(int hist[16384], int pix[256][256], int* m, int* s, int* a, int* sup ){
	int x,y;
	int l,r;
	
	//Histogramm
	for(y=0;y<256;++y){
		for(x=0;x<256;++x){
			if((pix[y][x]>=0)&&(pix[y][x]<16384)){hist[pix[y][x]]++;}
		}
	}
	
	//Mittelwert
	*m=0; *a=0; *sup=-1;
	for(y=16383;y>=0;--y){
		if((*sup<0)&&(hist[y]>2)) {*sup=y;}
		if(hist[y]>*a){*a=hist[y]; *m=y;}
	}
	
	//Breite
	l=-1; r=-1;
	for(y=1;y<16384;++y){
		if( (l<0) && (hist[y]>*a * 0.6065) ){l=y;}
		if( (r<0) && (hist[16384-y]>*a * 0.6065) ){r=16384-y;}
	}
	std::cout<<"(Histogramm)\tMaximum bei m="<<*m<<" mit a="<<*a<<" Grenzen: "<<l<<","<<r<<" sup="<<*sup<<std::endl;
	*s=r-l;
	if((l>=0)&&(r>=0)){*m=(l+r)/2;}
	return;
}


void PC::Histogramm(int hist[16384], int pix[256][256], int* m, int* s, int* a, int* sup, int* inf ){
	int x,y;
	int l,r;
	
	//Histogramm
	for(y=0;y<256;++y){
		for(x=0;x<256;++x){
			if((pix[y][x]>=0)&&(pix[y][x]<16384)){hist[pix[y][x]]++;}
		}
	}
	
	//Mittelwert
	*m=0; *a=0; *sup=-1; *inf=-1;
	for(y=1;y<16384;y++){
		if(*sup<=0){
			if( (*sup==0) && (hist[16384-y]>0) ) {*sup=16384-y;}
			if( (*sup<0) && (hist[16384-y]>=2) ) {*sup=0; }	//bei unserem Timepix-Chip haben 2 Pixel eine sehr hohe Schwelle
		}
		if((*inf<0)&&(hist[y]>0)) {*inf=y;}
		if(hist[y]>*a){*a=hist[y]; *m=y;}
	}
	
	//Breite
	l=-1; r=-1;
	for(y=1;y<16384;++y){
		if( (l<0) && (hist[y]>*a * 0.6065) ){l=y;}
		if( (r<0) && (hist[16384-y]>*a * 0.6065) ){r=16384-y;}
	}
	std::cout<<"(Histogramm)\tMaximum bei m="<<*m<<" mit a="<<*a<<" Grenzen: "<<l<<","<<r<<" inf="<<*inf<<" sup="<<*sup<<std::endl;
	*s=r-l;
	if( (l>=0) && (r>=0) && (*s>7) ){*m=(l+r)/2;}
	return;
}


int PC::DoRun(unsigned short runtimeFrames_, 
	      int runtime_, 
	      int shutter_, 
	      int filter_, 
	      unsigned short shutter_mode_, 
	      unsigned short run_mode_, 
	      bool useHvFadc)
{
#if DEBUG == 2
    std::cout << "Enter: PC::DoRun()" << std::endl;
#endif 

    //check if the fadc readout is not wanted although the fadc was initilised...
    bool tmpFADC = _useHvFadc;
    //... if this happens, set the FADC flag to false and change it after the run to true again
    if(useHvFadc != _useHvFadc) _useHvFadc = false; 
  
    //build local vars
    int result;
    int data[9][256][256];
    char TimeName[20]={0};
    std::ostringstream sstream;
  
    //set global vars
    runtime = runtime_;
    runtimeFrames = runtimeFrames_;
    shutter = shutter_;
    filter = filter_;
    shutter_mode = shutter_mode_;
    run_mode = run_mode_;
  
    //create run directory
    time_t Time_SecondsPassed;
    struct tm * TimeStruct;

    time(&Time_SecondsPassed); TimeStruct=localtime(&Time_SecondsPassed);
    strftime(TimeName,19,"Run%y%m%d_%H-%M-%S",TimeStruct);
#ifdef __WIN32__
    mkdir(DataPathName.c_str());
#else
    mkdir(DataPathName.c_str(),0755);
#endif
    PathName = DataPathName+"/"; PathName+=TimeName;
#ifdef __WIN32__
    mkdir(PathName.c_str());
#else
    mkdir(PathName.c_str(),0755);
#endif
  
    //WriteRunFile();
    for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++)
    {
	sstream<<"_"<<chip;
	FileName=PathName+"/"; FileName+=FSRFileName; FileName+=sstream.str();
	fpga.tp.SaveFSRToFile(FileName.c_str(),chip);
	FileName=PathName+"/"; FileName+=MatrixFileName; FileName+=sstream.str();
	fpga.tp.SaveMatrixToFile(FileName.c_str(),chip);
	sstream.str("");
    }
          
  
    MeasuringCounter = 0;

    //open shutter once
    if (shutter_mode == 0){
	result=fpga.CountingTime(shutter);
	if(result!=20){(RunIsRunning)=false;}
    }
    if (shutter_mode == 1)  {
	result=fpga.CountingTrigger(shutter);
	if(result!=20){(RunIsRunning)=false;}
    }
    if (shutter_mode == 2)  {
	result=fpga.CountingTime_fast(shutter);
	if(result!=20){(RunIsRunning)=false;}
    }
    if (shutter_mode == 3) {
	result=fpga.CountingTrigger_fast(shutter);
	if(result!=20){(RunIsRunning)=false;}
    }
    if (shutter_mode == 4) {
	result=fpga.CountingTime_long(shutter);
	if(result!=20){(RunIsRunning)=false;}
    }
    if (shutter_mode == 5) {
	result=fpga.CountingTime_verylong(shutter);
	if(result!=20){(RunIsRunning)=false;}
    }
  
    result=fpga.DataChipFPGA(result);


    //start run  
    mutexRun.lock();
    RunIsRunning = true;
    mutexRun.unlock();
    start(QThread::NormalPriority);


    //check var for abort
    std::string ein="";
    time_t start = time(NULL);
    int timediff = 0;

    //TODO: Someone - who wrote this - should check the difference between the two if loops...
    if (runtimeFrames == 0)
    {
	do{std::getline(std::cin,ein);if(ein.compare("q")==0){StopRun();}} while(IsRunning()) ;
    }
    if (runtimeFrames == 1)
    {
	do{std::getline(std::cin,ein);if(ein.compare("q")==0){StopRun();}} while(IsRunning()) ;
    }

    //result=fpga.DataFPGAPC(data); something like this should in principle be done (Mode 1b = 27 to read out RAM from FPGA from last run). Not important

    //change the global FADC flag back to true (otherwise it stays at false)
    _useHvFadc = tmpFADC;
    return 0;
}



void PC::StopRun(){
    mutexRun.lock();
    RunIsRunning=false;
    mutexRun.unlock();
}



bool PC::IsRunning()
{
    bool result;
    mutexRun.lock();
    result = RunIsRunning;
    mutexRun.unlock();
    return result;
}



void PC::run()
{
 #if DEBUG == 2
    std::cout << "Enter: PC::run()" << std::endl;
 #endif 
  
    if(_useHvFadc) runFADC();
    else runOTPX();
  
    return;
}



void PC::runFADC()
{
#if DEBUG == 2
    std::cout << "Enter: PC::runFADC()" << std::endl;
#endif 

    time_t start = time(NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

    //zero surpressed readout
    if (run_mode == 0)
    {
	Producer producer(this);
	DataAcqRunning = true;
	Consumer consumer(this);
	producer.start();
	std::cout << "runFADC: Producer start" << std::endl;
	consumer.start();
	//producer.wait();
	//consumer.wait();
	consumer.wait();
	std::cout << "runFADC: consumer ended"<< std::endl;
	std::cout<<"Press ENTER to close run "<<std::flush;
    }
    else{
	std::cout << "only zero surpressed readout implemeted, while using the FADC!" << std::endl;
	std::cout << "abort" << std::endl;
    }


    return;
}



void PC::runOTPX()
{
#if DEBUG == 2
    std::cout << "Enter: PC::runOTPX()" << std::endl;
#endif 

    time_t start = time(NULL);

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

    //zero surpressed readout
    if (run_mode == 0)
    {
	Producer producer(this);
	DataAcqRunning = true;
	Consumer consumer(this);
	producer.start();
	std::cout << "runOTPX: Producer start" << std::endl;
	consumer.start();
	//producer.wait();
	//consumer.wait();
	consumer.wait();
	std::cout << "runOTPX: consumer ended"<< std::endl;
	std::cout<<"Press ENTER to close run "<<std::flush;
    }
    //complete matrix readout
    else 
    {
	while(IsRunning())
	{
	    int result = 0;
	    //std::cout<<"I'm Running"<<std::endl;
	    if (shutter_mode == 0)
	    {
		result=fpga.CountingTime(shutter);         //Trigger when trigger used
		if(result!=20){(RunIsRunning)=false;}
	    }
	    if (shutter_mode == 1)  
	    {
		result=fpga.CountingTrigger(shutter);      //Trigger when trigger used
		if(result!=20){(RunIsRunning)=false;}
	    }
	    if (shutter_mode == 2)  
	    {
		result=fpga.CountingTime_fast(shutter);    //Trigger when trigger used
		if(result!=20){(RunIsRunning)=false;}
	    }
	    if (shutter_mode == 3) 
	    {
		result=fpga.CountingTrigger_fast(shutter); //Trigger when trigger used
		if(result!=20){(RunIsRunning)=false;}
	    }
	    if (shutter_mode == 4) 
	    {
		result=fpga.CountingTime_long(shutter);    //Trigger when trigger used
		if(result!=20){(RunIsRunning)=false;}
	    }
	    if (shutter_mode == 5)
	    {
		result=fpga.CountingTime_verylong(shutter); //Trigger when trigger used
		if(result!=20){(RunIsRunning)=false;}
	    }


	    //vars used to build the filenames
	    struct   timeval  tv;
	    struct   timezone tz;
	    struct   tm      *tm;
	    int hh, mm, ss, us;
	    int   ms;

	    gettimeofday(&tv, &tz);
	    tm = localtime(&tv.tv_sec);
	    hh = tm->tm_hour;        // hours
	    mm = tm->tm_min;         // minutes
	    ss = tm->tm_sec;         // seconds
	    us = tv.tv_usec;         // micro seconds
	    ms = tv.tv_usec/1000;    // mili seconds

	    std::string filename[9]= {""};
	    const char* f[9];
	    std::ostringstream sstream;

	    //build filename(s)
	    for (unsigned short chip = 1;chip <= fpga.tp.GetNumChips() ;chip++)
	    {
		sstream << "data" << std::setw(6) << std::setfill('0') << MeasuringCounter << "_" << chip << "_"
			<< std::setw(2) << std::setfill('0') << hh << std::setw(2) << std::setfill('0') << mm
			<< std::setw(2) << std::setfill('0') << ss
			<< std::setw(3) << std::setfill('0') << ms << ".txt";
		filename[chip]=PathName;
		filename[chip]+="/";
		filename[chip]+=sstream.str();
		std::cout << filename[chip] << std::endl;
		f[chip] = filename[chip].c_str();
		sstream.str("");
		// TODO: next line does not exist in Tobys code.
		sstream.clear();
	    }
	    sstream.str("");


	    //readout
	    result=DoReadOut(f);
	    if(result<0)
	    {
		RunIsRunning=false; 
		pthread_exit(0);
	    }  //return;}
	    //std::cout<<MeasuringCounter<<": "<<result<< " answering Pixel"<<std::endl;
      
	    if((filter)&&(result==0)){remove(FileName.c_str());}
      
	    ++MeasuringCounter;
      
	    if (runtimeFrames == 1)
	    {
		//std::cout << "MeasuringCounter "<<MeasuringCounter<<" runtime "<<runtime<< std::endl;
		if (MeasuringCounter == runtime)
		{ 
		    StopRun(); 
		}
	    }
	    if (runtimeFrames == 0)
	    {
		if ((difftime(time(NULL),start)) >= runtime) StopRun();
	    }
	}//end of: while(IsRunning)
    
   
	std::cout << "runOTPX: Run finshed: " << MeasuringCounter << "\n> "<<std::endl;
	std::cout<<"Press ENTER to close run "<<std::flush;
    }//end of else{ ... (Full Matrix readout)
    return;
}



void PC::readoutFadc(std::string filePath, std::vector<int> fadcParams, std::vector<int> *chipData, std::vector<int>fadcData)
{
 #if DEBUG == 2
 std::cout << "Enter: PC::readoutFADC()" << std::endl;
 #endif 

  //build filename
  std::string fileName;
  std::stringstream buildFileName;

  struct   timeval  tv;
  struct   timezone tz;
  struct   tm      *tm;
  int hh, mm, ss;
  int   ms;

  gettimeofday(&tv, &tz);
  tm = localtime(&tv.tv_sec);
  hh = tm->tm_hour;        // hours
  mm = tm->tm_min;         // minutes
  ss = tm->tm_sec;         // seconds
  ms = tv.tv_usec/1000;    // mili seconds

  //FIXME!!
  buildFileName << "data" << std::setw(6) << std::setfill('0') << "42" <<"_1_"
	   << std::setw(2) << std::setfill('0') << hh
	   << std::setw(2) << std::setfill('0') << mm
	   << std::setw(2) << std::setfill('0') << ss
	   << std::setw(3) << std::setfill('0') << ms <<".txt";
  
  FileName = filePath + "/" + buildFileName.str(); 
  buildFileName.str("");
  buildFileName.clear();

  //readout chip
  if (run_mode == 0)
  {
    int hits [9] = {0};
    //create output file
    FILE* f2 = fopen(FileName.c_str(),"w");

    for (unsigned short chip = 0;chip < fpga.tp.GetNumChips() ;chip++)
    {
      int NumHits = chipData->at(0);
      std::cout << "FADC and Chip Readout Numhits: " << NumHits << " on chip " << chip+1 << std::endl; 
      hits[chip+1] = (((chipData)->size()) - 1)/3;

      //if there is a problem with the output file: ...
      if(f2 == NULL) 
      {
        //... stop Run
        std::cout << "Readout: File error" << std::endl; 
        RunIsRunning = false;
      }
      
      //print data to file(s)
      for (std::vector<int>::iterator it = chipData->begin()+1; it != (chipData->end()); it= it + 3) 
      {
	if (*(it+2) !=0) fprintf(f2, "%d %d %d \n", *it, *(it+1), *(it+2));
      }      
    }//close for (unsigned short chip = 0;chip < p...

    fclose(f2);

    std::cout << "created " << FileName << std::endl;
  
  }
  //TODO:: This should not be possible!!
  else
  { 
    std::cout << "full matrix readout not implemented" << std::endl;
    std::cout << "if this error message appears on your screen, something went really wrong!!" << std::endl;
  }

  //readout FADC 
  //get nb of channels
  int channels = 4;

  if( (fadcParams.at(0) !=1 ) && (fadcParams.at(0) !=2))
  {
    if( !(fadcParams.at(1) & 8) ) channels--;
    if( !(fadcParams.at(1) & 4) ) channels--;
    if( !(fadcParams.at(1) & 2) ) channels--;
    if( !(fadcParams.at(1) & 1) ) channels--;
  }

  //set filenmane and open file 
  buildFileName << FileName << "-fadc";
  std::cout << "outfile " << buildFileName.str() << std::endl;

  std::ofstream outFile( buildFileName.str().c_str() );

  //write data to output file
  if( outFile.is_open() )
  {
    //write petrig/postrig/etc to file
    outFile << "# nb of channels: " << std::endl << fadcParams.at(0) << std::endl;
    outFile << "# channel mask: " << std::endl << fadcParams.at(1) << std::endl;
    outFile << "# postrig: " << std::endl << fadcParams.at(2) << std::endl;
    outFile << "# pretrig: " << std::endl << fadcParams.at(3) << std::endl;
    outFile << "# triggerrecord: " << std::endl << fadcParams.at(4) << std::endl;
    outFile << "# frequency: " << std::endl << fadcParams.at(5) << std::endl;
    outFile << "# sampling mode: " << std::endl << fadcParams.at(6) << std::endl;
    outFile << "#Data:" << std::endl;


    //"skip" 'first sample', 'venier' and 'rest baseline' (manual p27)
    for(unsigned int iData = 0; iData < 3*channels; iData++)
      outFile << "#" << fadcData[iData] << std::endl;
    
    //print the actual data to file
    for(unsigned int iVector = 3*channels; iVector < 2563*channels; iVector++)
      outFile << fadcData[iVector] << std::endl;
          
    //"skip the remaining data points
    for(unsigned int iRest = channels * 2563; iRest < fadcData.size(); iRest++)
      outFile << "# " << fadcData.at(iRest) << std::endl; 
  }//end of if( outFile.is_open() )
  
  //close output file
  outFile.close();
  

  return;
}



bool PC::SetDataPathName(std::string DataPath){
	DataPathName=DataPath;
	return true;
}
bool PC::SetDataFileName(std::string DataFile){
	DataFileName=DataFile;
	return true;
}
bool PC::SetRunFileName(std::string RunFile){
	RunFileName=RunFile;
	return true;
}
bool PC::SetFSRFileName(std::string FSRFile){
	FSRFileName=FSRFile;
	return true;
}
bool PC::SetMatrixFileName(std::string MatrixFile){
	MatrixFileName=MatrixFile;
	return true;
}
bool PC::SetDACScanFileName(std::string DACScanFile){
	DACScanFileName=DACScanFile;
	return true;
}
bool PC::SetThresholdFileName(std::string ThresholdFile){
	ThresholdFileName=ThresholdFile;
	return true;
}
bool PC::SetMaskFileName(std::string MaskFile){
	MaskFileName=MaskFile;
	return true;
}
const char* PC::GetTOSPathName(){
	return TOSPathName.c_str();
}
const char* PC::GetDataPathName(){
	return DataPathName.c_str();
}
const char* PC::GetDataFileName(unsigned short chip){
	if (chip == 1){return DataFileName.c_str();}
	else if (chip == 2){return Data2FileName.c_str();}
	else if (chip == 3){return Data3FileName.c_str();}
	else if (chip == 4){return Data4FileName.c_str();}
	else if (chip == 5){return Data5FileName.c_str();}
	else if (chip == 6){return Data6FileName.c_str();}
	else if (chip == 7){return Data7FileName.c_str();}
	else if (chip == 8){return Data8FileName.c_str();}
	else return false;

}
const char* PC::GetRunFileName(){
	return RunFileName.c_str();
}
const char* PC::GetFSRFileName(unsigned short chip){
	if (chip == 1){return FSRFileName.c_str();}
	else if (chip == 2){return FSR2FileName.c_str();}
	else if (chip == 3){return FSR3FileName.c_str();}
	else if (chip == 4){return FSR4FileName.c_str();}
	else if (chip == 5){return FSR5FileName.c_str();}
	else if (chip == 6){return FSR6FileName.c_str();}
	else if (chip == 7){return FSR7FileName.c_str();}
	else if (chip == 8){return FSR8FileName.c_str();}
	else return false;
}
const char* PC::GetMatrixFileName(unsigned short chip){
	if (chip == 1){return MatrixFileName.c_str();}
	else if (chip == 2){return Matrix2FileName.c_str();}
	else if (chip == 3){return Matrix3FileName.c_str();}
	else if (chip == 4){return Matrix4FileName.c_str();}
	else if (chip == 5){return Matrix5FileName.c_str();}
	else if (chip == 6){return Matrix6FileName.c_str();}
	else if (chip == 7){return Matrix7FileName.c_str();}
	else if (chip == 8){return Matrix8FileName.c_str();}
	else return false;
}
const char* PC::GetDACScanFileName(){
	return DACScanFileName.c_str();
}
const char* PC::GetThresholdFileName(unsigned short chip){
	if (chip == 1){return ThresholdFileName.c_str();}
	else if (chip == 2){return Threshold2FileName.c_str();}
	else if (chip == 3){return Threshold3FileName.c_str();}
	else if (chip == 4){return Threshold4FileName.c_str();}
	else if (chip == 5){return Threshold5FileName.c_str();}
	else if (chip == 6){return Threshold6FileName.c_str();}
	else if (chip == 7){return Threshold7FileName.c_str();}
	else if (chip == 8){return Threshold8FileName.c_str();}
	else return false;
}
const char* PC::GetTOTCalibFileName(unsigned short chip){
	if (chip == 1){return TOTCalibFileName.c_str();}
	else if (chip == 2){return TOTCalib2FileName.c_str();}
	else if (chip == 3){return TOTCalib3FileName.c_str();}
	else if (chip == 4){return TOTCalib4FileName.c_str();}
	else if (chip == 5){return TOTCalib5FileName.c_str();}
	else if (chip == 6){return TOTCalib6FileName.c_str();}
	else if (chip == 7){return TOTCalib7FileName.c_str();}
	else if (chip == 8){return TOTCalib8FileName.c_str();}
	else return false;
}

const char* PC::GetTOACalibFileName(unsigned short chip){
	if (chip == 1){return TOACalibFileName.c_str();}
	else if (chip == 2){return TOACalib2FileName.c_str();}
	else if (chip == 3){return TOACalib3FileName.c_str();}
	else if (chip == 4){return TOACalib4FileName.c_str();}
	else if (chip == 5){return TOACalib5FileName.c_str();}
	else if (chip == 6){return TOACalib6FileName.c_str();}
	else if (chip == 7){return TOACalib7FileName.c_str();}
	else if (chip == 8){return TOACalib8FileName.c_str();}
	else return false;
}

const char* PC::GetMaskFileName(unsigned short chip){
	if (chip == 1){return MaskFileName.c_str();}
	else if (chip == 2){return Mask2FileName.c_str();}
	else if (chip == 3){return Mask3FileName.c_str();}
	else if (chip == 4){return Mask4FileName.c_str();}
	else if (chip == 5){return Mask5FileName.c_str();}
	else if (chip == 6){return Mask6FileName.c_str();}
	else if (chip == 7){return Mask7FileName.c_str();}
	else if (chip == 8){return Mask8FileName.c_str();}
	else return false;
}


void PC::MakeBMP(int arr[256][256]){

  unsigned char buffer[196662];
  //int korr[256][256];
  unsigned char r,g,b;
  int i,x,y;
  FILE* f;
  //std::fstream fs;
  
  /*fs.open("korr.txt",std::fstream::in); if(!fs.is_open()){std::cout<<"Dateifehler"<<std::endl; return;}
  for(y=0;y<256;++y){for(x=0;x<256;++x){
	fs>>korr[y][x];
  }}
  fs.close();*/
  
  for(i=0;i<196662;++i){buffer[i]=0xff;}
  
  buffer[0]=0x42; 	buffer[1]=0x4d;
  buffer[2]=0x36; 	buffer[3]=0x00;		buffer[4]=0x03;		buffer[5]=0x00;
  buffer[6]=0x00; 	buffer[7]=0x00;		buffer[8]=0x00;		buffer[9]=0x00;
  buffer[10]=0x36;	buffer[11]=0x00;	buffer[12]=0x00;	buffer[13]=0x00;
  
  buffer[14]=0x28;	buffer[15]=0x00;	buffer[16]=0x00;	buffer[17]=0x00;
  buffer[18]=0x00;	buffer[19]=0x01;	buffer[20]=0x00;	buffer[21]=0x00;
  buffer[22]=0x00;	buffer[23]=0xff;	buffer[24]=0xff;	buffer[25]=0xff;
  buffer[26]=0x01;	buffer[27]=0x00;
  buffer[28]=0x18;	buffer[29]=0x00;
  buffer[30]=0x00;	buffer[31]=0x00;	buffer[32]=0x00;	buffer[33]=0x00;
  buffer[34]=0x00;	buffer[35]=0x00;	buffer[36]=0x00;	buffer[37]=0x00;
  buffer[38]=0x00;	buffer[39]=0x00;	buffer[40]=0x00;	buffer[41]=0x00;
  buffer[42]=0x00;	buffer[43]=0x00;	buffer[44]=0x00;	buffer[45]=0x00;
  buffer[46]=0x00;	buffer[47]=0x00;	buffer[48]=0x00;	buffer[49]=0x00;
  buffer[50]=0x00;	buffer[51]=0x00;	buffer[52]=0x00;	buffer[53]=0x00;
  
  f=fopen("pix.bmp","w");
  fwrite(buffer,1,54,f);
  
  g=0x0; b=0x0; r=0;
  for(y=0;y<256;++y){for(x=0;x<256;++x){
	if( (arr[y][x]>=7480)  ){r=0xff; g=0x0; b=0x0;}
	else{g=0xff; b=0xff; r=0xff;}
	fwrite(&b,1,1,f); fwrite(&g,1,1,f); fwrite(&r,1,1,f);
  }}
  
  fclose(f);
}


void PC::MakeBMP(int **arr){

  unsigned char buffer[196662];
  //int korr[256][256];
  unsigned char r,g,b;
  int i,x,y;
  FILE* f;
  //std::fstream fs;
  
  /*fs.open("korr.txt",std::fstream::in); if(!fs.is_open()){std::cout<<"Dateifehler"<<std::endl; return;}
  for(y=0;y<256;++y){for(x=0;x<256;++x){
	fs>>korr[y][x];
  }}
  fs.close();*/
  
  for(i=0;i<196662;++i){buffer[i]=0xff;}
  
  buffer[0]=0x42; 	buffer[1]=0x4d;
  buffer[2]=0x36; 	buffer[3]=0x00;		buffer[4]=0x03;		buffer[5]=0x00;
  buffer[6]=0x00; 	buffer[7]=0x00;		buffer[8]=0x00;		buffer[9]=0x00;
  buffer[10]=0x36;	buffer[11]=0x00;	buffer[12]=0x00;	buffer[13]=0x00;
  
  buffer[14]=0x28;	buffer[15]=0x00;	buffer[16]=0x00;	buffer[17]=0x00;
  buffer[18]=0x00;	buffer[19]=0x01;	buffer[20]=0x00;	buffer[21]=0x00;
  buffer[22]=0x00;	buffer[23]=0xff;	buffer[24]=0xff;	buffer[25]=0xff;
  buffer[26]=0x01;	buffer[27]=0x00;
  buffer[28]=0x18;	buffer[29]=0x00;
  buffer[30]=0x00;	buffer[31]=0x00;	buffer[32]=0x00;	buffer[33]=0x00;
  buffer[34]=0x00;	buffer[35]=0x00;	buffer[36]=0x00;	buffer[37]=0x00;
  buffer[38]=0x00;	buffer[39]=0x00;	buffer[40]=0x00;	buffer[41]=0x00;
  buffer[42]=0x00;	buffer[43]=0x00;	buffer[44]=0x00;	buffer[45]=0x00;
  buffer[46]=0x00;	buffer[47]=0x00;	buffer[48]=0x00;	buffer[49]=0x00;
  buffer[50]=0x00;	buffer[51]=0x00;	buffer[52]=0x00;	buffer[53]=0x00;
  
  f=fopen("pix.bmp","w");
  fwrite(buffer,1,54,f);
  
  g=0x0; b=0x0; r=0;
  for(y=0;y<256;++y){for(x=0;x<256;++x){
	if( (arr[y][x]>=7480)  ){r=0xff; g=0x0; b=0x0;}
	else{g=0xff; b=0xff; r=0xff;}
	fwrite(&b,1,1,f); fwrite(&g,1,1,f); fwrite(&r,1,1,f);
  }}
  
  fclose(f);
}

void PC::SpeedTest(int wdh, int freq){ //outdated, not used any more

  int i;
  double hertz;
  timeval start,end;
  int temp[9][256][256];
  int msec;
  unsigned long sec;
  long usec;
  std::fstream f;
  std::stringstream sstr;
  int ChipID;
  
  fpga.GeneralReset(); fpga.GeneralReset(); fpga.GeneralReset();
  fpga.tp.LoadFSRFromFile(GetFSRFileName(1),1);
  fpga.WriteReadFSR(); fpga.WriteReadFSR(); fpga.WriteReadFSR();
  ChipID=fpga.tp.GetChipID(1);
  
  fpga.tp.UniformMatrix(0,0,1,1,7,1);
  fpga.SetMatrix(); fpga.SerialReadOut(temp); fpga.SetMatrix(); fpga.SerialReadOut(temp); fpga.SetMatrix(); fpga.SerialReadOut(temp);
  
  gettimeofday(&start,0);
  
  for(i=0;i<wdh;++i){
	fpga.SerialReadOut(temp);
  }
  
  gettimeofday(&end,0);
  usec= end.tv_usec - start.tv_usec;
  sec= end.tv_sec - start.tv_sec;
  if(usec<0){usec*=-1; sec-=1;}
  msec=usec/1000;
  usec=usec%1000;
  std::cout<<sec<<":"<<msec<<":"<<usec<<std::endl;
  hertz=double(wdh)/(double(sec)+(usec/1000000.));
  std::cout<<hertz<<" Hz"<<std::endl;
  
  sstr.str(""); sstr.clear(); sstr<<"results/"<<freq<<"MHz.txt";
  f.open(sstr.str().c_str(),std::fstream::out); 
  if(!f.is_open()){std::cout<<"Dateifehler"<<std::endl;}
  else{
    f<<"Wiederholungen:\t"<<wdh<<"\nZeit:\t"<<sec<<"."<<msec<<"."<<usec<<"\nFrequenz:\t"<<hertz<<"\nChipID=\t"<<ChipID<<std::endl;
    f.close();
  }
  
  
  fpga.GeneralReset(); fpga.GeneralReset(); fpga.GeneralReset();

}
