/**********************************************************************/
/*                                                           fpga.hpp */
/*  TOS - Timepix Operating Software                                  */
/*                                                                    */
/*                                                         20.07.2009 */
/*                                                    Christian Kahra */
/*                                     chrkahra@students.uni-mainz.de */
/*                                        Institut fuer Physik - ETAP */
/*                              Johannes-Gutenberg Universitaet Mainz */
/**********************************************************************/
#ifndef _FPGA_HPP
#define _FPGA_HPP

#include "timepix.hpp"
#include "frame.hpp"
#include "protocol_constants.hpp"

#ifdef __WIN32__
    #include <winsock2.h>
#else
    #include <netinet/in.h>
#endif

#include <sys/select.h>

#include <vector>
#include <string>
#include <set>

class hvFadcManager;


// set a default socket buffer size, which is 'big enough'
// this number is the maximum allowed size for my kernel (3.13.0-85-generic)
// given in /proc/sys/net/core/rmem_default
#define DEFAULT_SOCKET_BUFFER_SIZE             212992
// scaling factor by which the timeout is scaled (10 % longer than time we have to wait)
#if TESTS==0
#define DEFAULT_TIMEOUT_SCALING_FACTOR         2//1.1
// offset, which is added to the shutter length * default_scaling factor for timeout
// given in micro seconds
#define DEFAULT_TIMEOUT_OFFSET                 4e6
#else
// in case we run (hardware less) test functions, we probably don't want to wait for the
// select to timeout
#define DEFAULT_TIMEOUT_SCALING_FACTOR         0//1.1
#define DEFAULT_TIMEOUT_OFFSET                 0
#endif
// default number of clock cycles the shutter remains open, after the FADC triggers
// (200 clock cycles at 40 MHz == 5 mu s)
#define DEFAULT_FADC_SHUTTER_COUNT_ON          200

class FPGA{


public:

    //C'tor
    FPGA(Timepix *tp_pointer);
    //D'tor
    ~FPGA();

    // function to hand the hvFadcManager pointer to the FPGA object
    void initHV_FADC(hvFadcManager* hvFadcManager);

    Timepix *tp;
    int ErrInfo;
    int okay();
    int GeneralReset(); 	//err_code=1x
    int Counting();		//err_code=2x
    int CountingStop();
    int CountingTrigger(int time);
    int CountingTime(std::string shutter_time, std::string shutter_range);
    int CountingTime(int shutter_time, std::string shutter_range);
    int CountingTime(int time, int modeSelector);
    // helper function to check, if a given TOF mode is part of CountingTime
    bool checkIfModeIsCounting(int mode);

    // ReadoutFadcBit: Reads out the 16th(?) fpga bit - 1 if a trigger arrived as the shutter was open, 0 otherwise
    int ReadoutFadcBit();
    // ReadoutFadcFlag: Reads out the FADC flag. 1 if the flag is set, zero otherwise
    int ReadoutFadcFlag();
    // ClearFadcFlag: Sets the FADC flab back to false
    void ClearFadcFlag();

    template <typename Ausgabe> int SerialReadOut(Ausgabe aus);	//err_code=3xx
    template <typename Ausgabe> int SerialReadOutReadSend(Ausgabe aus, unsigned short chip);
    //template <typename Ausgabe> int DataChipFPGA(Ausgabe aus);
    int DataChipFPGA();    
    template <typename Ausgabe> int DataFPGAPC(Ausgabe aus, unsigned short chip);
		
    int SetMatrix();	  //err_code=4xx

    int WriteReadFSR();   //err_code=5xx
    int i2creset();
    int i2cDAC(unsigned short Umv, unsigned short DACchannel);
    int i2cADC(unsigned short channel);
    int tpulse(unsigned short Npulses, unsigned short div500kHz);

    int EnableTPulse(int tpulse);
    int EnableFADCshutter(unsigned short FADCshutter);

    int SwitchTriggerConnection(int connection);
    void SetIP(std::string ip_);
    std::string ShowIP();
    void MakeARPEntry();
    void UseFastClock(bool use);

    bool SetTimeout(int timeout);

    int ShutterRangeToMode(std::string shutter_range);

    // function to hand a const reference to the _chip_set member variable of the
    // console class, such that always an up-to-date chip set is available to
    // the sub classes
    void SetChipSet(const std::set<unsigned short> &chip_set);

private:

    // pointer to hvFadcManager object
    hvFadcManager *_hvFadcManager;
    
    int _fadcBit;                 //< set to 1 if a signal was detected on the trigger in input/ 0 otherwise
    bool _fadcFlag;               //< true if _fadcBit was set to 1 since the flag was cleared the last time
    unsigned int _fadcShutterCountOn;
    int ok;
    int TriggerConnectionIsTLU;
    int sock, Port; 
    //char ip[16];
    std::string ip;
    /* Filedeskriptoren fuer select() - readfd merkt sich von wo gelesen werden soll, 
     * testfd testet ob dort ein Paket angekommen ist
     */
    fd_set readfd, testfd; 	 
    struct timeval _timeout; 	//< Zeitvariable fuer Timeout bei Kommunikation mit FPGA
    struct sockaddr_in sckadd;	//< in netinet/in.h definierte Struktur fuer IP-Adressen
		
    //		unsigned char M0,M1,Enable_IN,Shutter,Reset,Enable_Test;  // Modus(=Control)-Bits
    unsigned char Mode;

    /* Modus, zu erwartende Packetlaenge, ausgehende Packetlaenge, Anzahl zu sendende 
     * Packete, Paketzaehler der Software und des FPGA
     */
    int IncomingLength;
    int OutgoingLength;
    int PacketQueueSize; 
    int SoftwareCounter; 
    int FPGACounter; 
    int HitsMode;


    unsigned char PacketBuffer[PLen+18]; //Buffer fuer send, bzw bei SetMatrix fuer receive
    unsigned char PacketQueue[PQueue][PLen+18]; //Buffer fuer receive (alle Pakete bei ReadOut), bzw fuer send (alle Pakete) bei SetMatrix
    std::vector<std::vector<unsigned char> > *PackQueueReceive;// = new std::vector<std::vector<unsigned char> >(PQueue, std::vector<unsigned char>(PLen+18));
    int calcSizeOfLastPackage(int nChips, int p_len);
    int Communication( unsigned char* Bsend, unsigned char* Brecv, int timeout = 2e5); 	                                 //err_code=x
    int Communication2(unsigned char* Bsend, unsigned char* Brecv, int numHits, unsigned short chip, int timeout = 2e5);        //err_code=x
    int CommunicationReadSend(unsigned char* Bsend, unsigned char* Brecv, int numHits, unsigned short chip, int timeout = 2e5); //err_code=x
    // the SaveData function without argument is a default to call, if one does not
    // care about the return values
    // TODO: think about whether to rather change SerialReadOut function to
    // not call a SaveData function in case of an empty argument
    // int SaveData();
    int SaveData(std::string filename);						//err_code=x
    int SaveData(int pix[8][256][256]);
    int SaveData(std::vector<std::vector<std::vector<int> > > *VecData);
    int SaveData(FrameArray<int> *pixel_data, int NumHits);
    int SaveData(int pix[256][256], int NumHits);
    int SaveData(std::vector<int> *pHitArray ,int NumHits);
    // SaveData functions for a map of Frames and single FrameArrays
    int SaveData(std::map<int, Frame> *frame_map);
    // called from function above
    int SaveData(FrameArray<int> *pixel_data, unsigned short chip = 0);
    int SaveData(int **pix);
    int SaveData(int hit_x_y_val[12288], int NumHits );
    bool _usefastclock;		

    // reference to the chip_set of the console object
    std::set<unsigned short> _chip_set;
};

template <typename Ausgabe> int FPGA::SerialReadOut(Ausgabe aus)
{
#if DEBUG==2
    std::cout<<"Enter FPGA::SerialReadOut()"<<std::endl;	
#endif

    int p;
    int err_code;
    int nChips = tp->GetNumChips();
	
    //M0=0; M1=0; Enable_IN=0; Shutter=1; Reset=1;	 //ModL= 24 = 0x18
    // Start Serial Readout 0x04
    Mode = 0x04;
    OutgoingLength = 18;
    IncomingLength = 18 + PLen;
    PacketQueueSize=PQueue*tp->GetNumChips();
    err_code=Communication(PacketBuffer,&((*PackQueueReceive)[0][0]));
  
    if(err_code>0)return 300+err_code;
  
    // Forward Serial Readout 0x05
    Mode = 0x05;
    for(p=1; p<PacketQueueSize; ++p)
    {
	if(p==PacketQueueSize-1)
	{
            // only 8 bit preload instead of 10 doesnt matter here, 
	    // as postload is enough and not completely transmitted
	    // use function to calculate the size of the last package
	    IncomingLength = calcSizeOfLastPackage(nChips, PLen);
	    // End Serial Readout 0x06
	    Mode = 0x06;
	}

	//usleep(10000);
	//Mode=6;
	err_code=Communication(PacketBuffer,&((*PackQueueReceive)[p][0]));
	if(err_code>0) return 300+err_code;
    }
    //if(aus!=NULL){err_code=SaveData(aus);}
    err_code=SaveData(aus);
    return 300+10*err_code;
}

template <typename Ausgabe> int FPGA::SerialReadOutReadSend(Ausgabe aus, unsigned short chip)
{
#if DEBUG==2
    std::cout<<"Enter FPGA::SerialReadOut()"<<std::endl;
#endif
 
    int p;
    int err_code;

    //M0=0; M1=0; Enable_IN=0; Shutter=1; Reset=1;	 //ModL= 24 = 0x18
    // Readout Chip zero suppressed output prepared 0x1C
    Mode = 0x1C;

#if DEBUG==1
    std::cout << "reading out data from chip to FPGA and sending last frame meanwhile" << std::endl;
#endif

    OutgoingLength=18; 
    IncomingLength=18+PLen; 
    //changed: IncomingLength=18+PLen -> 18, no data in new Mode 4, only data Chip -> FPGA
    PacketQueueSize=PQueue; 
    //first packet PC-> PFGA: start readout Chip, FPGA->PC: first data packet of last frame
    int NumHits = CommunicationReadSend(PacketBuffer,&((*PackQueueReceive)[0][0]),1,chip); 
    //NumHits = 40002;
    int Hits;

    if (NumHits > 4096)
    {
	Hits = 4096;
    }
    else
    {
	Hits = NumHits;
    }

    // number of Packets: round up division, -1 because first packet allready arrived
    int packets = (((Hits*4)+PLen-1)/PLen)-1; 
    // in case of exactly 350 hits (which fit into a single package!) we set the number of still to
    // come packages to 1, so as to receive another package
    // why? not exactly clear
    if (Hits==PLen/4){packets = 1;}
    #if DEBUG==1
    std::cout << "Number of packets: " << packets << " for chip " << chip << std::endl;
    #endif
    if (packets == 0){
	//IncomingLength=18+PLen;
	//err_code=CommunicationReadSend(PacketBuffer,&((*PackQueueReceive)[0][0]),2);
    }
    else {
	for (p=0;p<packets;++p)
	{
	    #if DEBUG==1
	    std::cout << "packet Number: " << p << ""<< std::endl;
	    #endif
	    if (p == packets-1) 
	    {
		IncomingLength = (18+((Hits)*4)%PLen);
		if ((((Hits)*4)%PLen) == 0) 
		{
		    IncomingLength=18+PLen;
		}
		err_code=CommunicationReadSend(PacketBuffer,&((*PackQueueReceive)[p+1][0]),2,chip);
	    }
	    else err_code=CommunicationReadSend(PacketBuffer,&((*PackQueueReceive)[p+1][0]),0,chip);
	    if(err_code>0) return 300+err_code;
	}
    }
    if(aus!=NULL){
	err_code=SaveData(aus, NumHits);
    }
    //std::cout << "Pointer aus " << aus<<std::endl;
    return Hits;
}

#endif


template <typename Ausgabe> int FPGA::DataFPGAPC(Ausgabe aus, unsigned short chip){
    // return values:
    // a return value of -300 or less corresponds to
    // an error in Communication2!
    // where the diff error - 300 corresponds to the error code from
    // Communication2()
#if DEBUG==2
    std::cout<<"Enter FPGA::SerialReadOut()"<<std::endl;
#endif
    int err_code;

    OutgoingLength=18; PacketQueueSize=PQueue; //changed: IncomingLength=18+PLen -> 18, no data in new Mode 4, only data Chip -> FPGA
    // Send Data 0x1B
    Mode = 0x1B;
#if DEBUG==1
    std::cout << "transmitting data from FPGA" << std::endl;
#endif
    IncomingLength=18+PLen; //added as changed above
    int NumHits=Communication2(PacketBuffer,&((*PackQueueReceive)[0][0]),1,chip); // first packet
    if(NumHits < 0){
	return NumHits;
    }
    int Hits;
    if (NumHits > 4096){
	Hits = 4096;
    }
    else {
	Hits = NumHits;
    }

    // in case of exactly 350 hits (which fit into a single package!) we set the number of still to
    // come packages to 1, so as to receive another package
    // why? not exactly clear
    int packets = (((Hits*4)+PLen-1)/PLen)-1; // number of Packets: round up division, -1 because first packet allready arrived
    if (Hits==PLen/4){packets = 1;}
    //std::cout << "Number of packets: " << packets << ""<< std::endl;
    for (int p=0;p<packets;++p){
	if (p == packets-1) {
	    IncomingLength = (18+((Hits)*4)%PLen);
	    if ((((Hits)*4)%PLen) == 0) {
		IncomingLength=18+PLen;
	    }
	}
	err_code=Communication2(PacketBuffer,&((*PackQueueReceive)[p+1][0]),0,chip);
	if(err_code < 0) return -300+err_code;
    }
    if(aus!=NULL){
	err_code=SaveData(aus, NumHits);
    }
    //std::cout << "Pointer aus " << aus<<std::endl;
    return Hits;
}
