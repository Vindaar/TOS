/**********************************************************************/
/*                                                           fpga.cpp */
/*  TOS - Timepix Operating Software                                  */
/*                                                                    */
/*                                                         20.07.2009 */
/*                                                    Christian Kahra */
/*                                     chrkahra@students.uni-mainz.de */
/*                                        Institut fuer Physik - ETAP */
/*                              Johannes-Gutenberg Universitaet Mainz */
/**********************************************************************/

#include "fpga.hpp"
//#define DEBUG 2



//C~tor
FPGA::FPGA():
    ErrInfo(0), 
    ok(1), 
    TriggerConnectionIsTLU(0), 
    _fadcBit(0),
    _fadcFlag(false), 
    Mode(0),
    IncomingLength(0), 
    OutgoingLength(0), 
    PacketQueueSize(1),
    SoftwareCounter(0), 
    FPGACounter(0),
    // in case of a windows machine, we need signed chars, whereas
    // for linux unsigned chars are needed. The macro differentiates
    // both platforms
#ifdef __WIN32__
    PackQueueReceive( new std::vector<std::vector<char> >(PQueue*8, std::vector<char>(PLen+18)) )
#else
    PackQueueReceive( new std::vector<std::vector<unsigned char> >(PQueue*8, std::vector<unsigned char>(PLen+18)))
#endif //endif ifdef __WIN32__
{
    std::cout<<"constructing FPGA"<<std::endl;

#if DEBUG==2
    std::cout<<"Enter FPGA::FPGA()"<<std::endl;	
#endif //endif if DEBUG==2

    int err_code;
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //Port = 0x1776;// for SRS //strcpy(ip,IP_ADRESSE); 0x200 also possible (special UDP port)
    Port = 0xAFFE; // for V6
    ip=IP_ADRESSE;

    FD_ZERO(&readfd);
    FD_SET(sock,&readfd);

#ifdef __WIN32__
    //FD_SET(0,&readfd); //0=stdin -> Lesen von der Konsole Geht unter WIN nicht mit select() muss trotzdem rein
#else
    FD_SET(0,&readfd); //0=stdin -> Lesen von der Konsole
#endif

    timeout.tv_sec =5; 
    timeout.tv_usec =10000;
    sckadd.sin_family = AF_INET; 
    sckadd.sin_port = htons(Port);

    sckadd.sin_addr.s_addr = INADDR_ANY;
    err_code=bind(sock, (struct sockaddr*)&sckadd, sizeof(sckadd));
  
    if(err_code!=0)
    {
	ok=0;
	ErrInfo=91;
	//throw CExceptions(err_code, "Fehler in bind");
    }
	
    sckadd.sin_addr.s_addr = inet_addr(ip.c_str());
    err_code=connect(sock, (struct sockaddr*)&sckadd, sizeof(sckadd));

    if(err_code!=0)
    {
	ok=0;
	ErrInfo=92;
	//throw CExceptions(err_code, "Fehler in connect");
    }
    //enlarge socket buffers: When you set SO_RCVBUF with setsockopt, the kernel allocates twice the
    //memory you set

    int sock_buf_size;
    int i=sizeof(sock_buf_size);

    // in the following lines of code, we need (char*)&sock_buf_size for windows and 
    // &sock_buf_size for linux. In order to not have the whole code twice with only 
    // such a small change, we define a macro:
    // TODO: probably nicer to put the macro into header file?
#ifdef __WIN32__
    #define SOCKET_BUFFER_SIZE_POINTER          (char*) &sock_buf_size
#else
    #define SOCKET_BUFFER_SIZE_POINTER          &sock_buf_size
#endif

    int nbytes; 
    // socket receive buffer
    nbytes = getsockopt(sock, 
			SOL_SOCKET, 
			SO_RCVBUF, 
			SOCKET_BUFFER_SIZE_POINTER, 
			(socklen_t *)&i);
    std::cout << "socket receiver buffer size was " 
	      << sock_buf_size 
	      << ", success (0=yes):" 
	      << nbytes 
	      << "\n" << std::endl;
    sock_buf_size = 200*sock_buf_size;

    nbytes = setsockopt(sock, 
			SOL_SOCKET, 
			SO_RCVBUF, 
			SOCKET_BUFFER_SIZE_POINTER, 
			sizeof(sock_buf_size));
    i=sizeof(sock_buf_size);

    nbytes = getsockopt(sock, 
			SOL_SOCKET, 
			SO_RCVBUF, 
			SOCKET_BUFFER_SIZE_POINTER, 
			(socklen_t *)&i);
    std::cout << "socket receiver buffer size changed to " 
	      << sock_buf_size 
	      << ", success (0=yes):" 
	      << nbytes 
	      << "\n" << std::endl;

    // socket send buffer
    nbytes = getsockopt(sock, 
			SOL_SOCKET, 
			SO_SNDBUF, 
			SOCKET_BUFFER_SIZE_POINTER, 
			(socklen_t *)&i);
    std::cout << "socket sender buffer size was " 
	      << sock_buf_size 
	      << ", success (0=yes):" 
	      << nbytes 
	      << "\n" << std::endl;
    sock_buf_size = 200*sock_buf_size;

    nbytes = setsockopt(sock, 
			SOL_SOCKET, 
			SO_SNDBUF, 
			SOCKET_BUFFER_SIZE_POINTER, 
			sizeof(sock_buf_size));
    i=sizeof(sock_buf_size);

    nbytes = getsockopt(sock, 
			SOL_SOCKET, 
			SO_SNDBUF, 
			SOCKET_BUFFER_SIZE_POINTER, 
			(socklen_t *)&i);
    // ?? the following line printed "socket receiver buffer size changed to" previously
    //    assume (input SO_SNDBUF to setsockopt) that sender buffer size was meant. Changed
    // TODO: check if correct
    std::cout << "socket sender buffer size changed to " 
	      << sock_buf_size 
	      << ", success (0=yes):" 
	      << nbytes 
	      << "\n" << std::endl;

    // TODO: understand why same function is being called again with same arguments?!
    nbytes = getsockopt(sock, 
			SOL_SOCKET, 
			SO_SNDBUF, 
			SOCKET_BUFFER_SIZE_POINTER, 
			(socklen_t *)&i);
    // TODO: should also be sender instead of receiver?
    std::cout << "socket receiver buffer size changed to " 
	      << sock_buf_size 
	      <<", success (0=yes):" 
	      << nbytes 
	      << "\n" << std::endl;
}


//D~tor
FPGA::~FPGA(){

    if( PackQueueReceive != NULL ){
	delete PackQueueReceive;}
    PackQueueReceive = NULL;
}


void FPGA::SetIP(std::string ip_){
    ip=ip_;
}


std::string FPGA::ShowIP(){
    return ip;
}


void FPGA::MakeARPEntry(){
    std::string cmd;
    cmd="sudo arp -s ";
    cmd+=ip;
    cmd+=" ";
    cmd+=MAC_ADDR;
    std::cout<<cmd<<std::endl;
    system(cmd.c_str());
    std::cout<<"\nShow new ARP-Table: arp -a"<<std::endl;
    system("arp -a");
}


int FPGA::okay(){
#if DEBUG==2
    std::cout<<"Enter FPGA::okay()"<<std::endl;	
#endif
    return ok;
}


int FPGA::GeneralReset(){
#if DEBUG==2
    std::cout<<"Enter FPGA::GeneralReset()"<<std::endl;	
#endif
    int err_code;
    //M0=1; M1=1; Enable_IN=1; Shutter=1; Reset=0; 	//ModL= 15 = 0x0f
    Mode=1;
    OutgoingLength=18; 
    IncomingLength=18; 
    PacketQueueSize=1; 
    err_code = Communication(PacketBuffer,PacketQueue[0]);
    return 10+err_code;
}


int FPGA::Counting(){
#if DEBUG==2
    std::cout<<"Enter FPGA::Counting()"<<std::endl;	
#endif
    int err_code;
    //M0=1; M1=1; Enable_IN=1; Shutter=0; Reset=1; 	//ModL= 23 = 0x17
    Mode=2;
    OutgoingLength=18; 
    IncomingLength=18; 
    PacketQueueSize=1;
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    if((err_code==0)||(err_code==3)){tp.SetCounting(1);}
    return 20+err_code;
}


int FPGA::CountingStop(){
#if DEBUG==2
    std::cout<<"Enter FPGA::CountingStop()"<<std::endl;	
#endif
    int err_code;
    //M0=1; M1=1; Enable_IN=0; Shutter=0; Reset=1; 	//ModL= 23 = 0x17
    Mode=3;
    OutgoingLength=18; 
    IncomingLength=18; 
    PacketQueueSize=1;  
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    if((err_code==0)||(err_code==3)){tp.SetCounting(0);}
    return 20+err_code;
}

// << "CountingTrigger <shutter-time>             = 2t <shutter-time>: "
// << "open shutter after external trigger for t[µs] = 46* <shutter-time>/40 \n\t"

int FPGA::CountingTrigger(int time){
#if DEBUG==2
    std::cout<<"Enter FPGA::CountingTrigger()"<<std::endl;	
#endif
    int err_code;
    //M0=1; M1=1; Enable_IN=0; Shutter=0; Reset=1; 	//ModL= 23 = 0x17
    if (usefastclock){
	if( TriggerConnectionIsTLU ){ Mode=29; }
	else{ Mode=29; }
    }
    else{
	if( TriggerConnectionIsTLU ){ Mode=23; }
	else{ Mode=23; }
    }
    OutgoingLength=18; 
    IncomingLength=18; 
    PacketQueueSize=1;  
    PacketBuffer[6]=time;
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    if((err_code==0)||(err_code==3)){tp.SetCounting(0);}
    return 20+err_code;
}


int FPGA::CountingTrigger_fast(int time){
#if DEBUG==2
    std::cout<<"Enter FPGA::CountingTrigger()"<<std::endl;
#endif
    int err_code;
    //M0=1; M1=1; Enable_IN=0; Shutter=0; Reset=1; 	//ModL= 23 = 0x17
    if(TriggerConnectionIsTLU){Mode=24;}
    else{Mode=24;}
    OutgoingLength=18; 
    IncomingLength=18; 
    PacketQueueSize=1;
    PacketBuffer[6]=time;
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    if((err_code==0)||(err_code==3)){tp.SetCounting(0);}
    return 20+err_code;
}

int FPGA::CountingTime(int time, int modeSelector){
    // int time:         integer in range {1, 255} 
    // int modeSelector: integer corresponding to power to which 256 is raised
    //                   in calculation of shutter time
    // shutter time [µs] = (256)^n*46*time/freq[MHz]
#if DEBUG==2
    std::cout<<"Enter FPGA::CountingTrigger()"<<std::endl;	
#endif
    int err_code;
    // choose the correct mode depening on input given in 
    // Console::CommandCountingTime()
    if (usefastclock){
	// standard, Mode = 20 in Hex: 0x14
	if      (modeSelector == 0) Mode = 20; 
	// long,     Mode = 30 in Hex: 0x1E
	else if (modeSelector == 1) Mode = 30;
	// verylong, Mode = 31 in Hex: 0x1F
	else if (modeSelector == 2) Mode = 31;
    }
    else {
	// standard, Mode = 19 in Hex: 0x13
	if      (modeSelector == 0) Mode = 19;
	//long,      Mode = 21 in Hex: 0x15
	else if (modeSelector == 1) Mode = 21;
	//verylong,  Mode = 22 in Hex: 0x16
	else if (modeSelector == 2) Mode = 22;
    }

    OutgoingLength=18; 
    IncomingLength=18; 
    PacketQueueSize=1;
    PacketBuffer[6]=time;
    err_code=Communication(PacketBuffer,PacketQueue[0]);

    if (tp.GetFADCshutter()==1)
    {
	int i2cresult = (tp.GetI2cResult() << 8) + tp.GetExtraByte();
	std::cout << "FADC trigger at " << i2cresult << " clock cycles."<<std::endl;
    }

    return 20+err_code;
}

int FPGA::CountingTime_fast(int time){
#if DEBUG==2
    std::cout<<"Enter FPGA::CountingTrigger()"<<std::endl;
#endif
    int err_code;
    Mode=20;
    OutgoingLength=18; 
    IncomingLength=18; 
    PacketQueueSize=1;
    PacketBuffer[6]=time;
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    return 20+err_code;
}



int FPGA::SetMatrix(){
#if DEBUG==2
    std::cout<<"Enter FPGA::SetMatrix()"<<std::endl;	
#endif

#ifdef __WIN32__
    int err_code;
    std::vector<std::vector<char> > *PackQueue = new std::vector<std::vector<char> >(PQueue*tp.GetNumChips(), std::vector<char>(PLen+18));
    //tp.PackMatrix(PacketQueue);
    tp.PackMatrix(PackQueue);
#else
    int err_code;
    std::vector<std::vector<unsigned char> > *PackQueue = new std::vector<std::vector<unsigned char> >(PQueue*tp.GetNumChips(), std::vector<unsigned char>(PLen+18));
    //tp.PackMatrix(PacketQueue);
    tp.PackMatrix(PackQueue);
#endif

    //M0=0; M1=1; Enable_IN=0; Shutter=1; Reset=1; 	//ModL= 26 = 0x1a
    Mode=10;
    OutgoingLength=18+PLen; 
    IncomingLength=18; 
    PacketQueueSize=PQueue*tp.GetNumChips();
	
    err_code=Communication(&((*PackQueue)[0][0]),PacketBuffer);
    if(err_code>0)return 40+err_code;
    Mode=11; OutgoingLength=18+PLen;
	
    for(int p=1;p<PacketQueueSize;++p){
	//usleep(100);
	if(p==PacketQueueSize-1){
	    OutgoingLength=18+((256*256*14+8+256)*tp.GetNumChips())/8%PLen;
	    Mode=12;
	}
	err_code=Communication(&((*PackQueue)[p][0]),PacketBuffer);
	if(err_code>0)return 400+err_code;
    }
    delete PackQueue;
    return 40;
}


int FPGA::WriteReadFSR(){
#if DEBUG==2
    std::cout<<"Enter FPGA::WriteReadFSR()\n"<<std::flush;
#endif
    int err_code;
    tp.GetFSR(PacketBuffer);
#if DEBUG>0
    int i;
    for(i=18;i<18+(32*tp.GetNumChips())+2;i++)printf("%i ",PacketBuffer[i]);printf("\n");
#endif
    //M0=1; M1=0; Enable_IN=0; Shutter=1; Reset=1; 	//ModL= 25 = 0x19
    Mode=13;
    // changed to 34*tp.GetNumChips() (in Alex code was 32*...)
    OutgoingLength=18+(34*tp.GetNumChips()) + 2; 
    // changed to 34*tp.GetNumChips() + 2 (in Alex code was 33*tp.GetNumChips())
    IncomingLength=18+(34*tp.GetNumChips()) + 2; 
    PacketQueueSize=1;
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    if(err_code>0)return 50+err_code;
    for (unsigned short chip = 1;chip <= tp.GetNumChips() ;chip++){
	if (err_code>0) {err_code = err_code;}
	else err_code=tp.ChipID(PacketQueue[0],chip);
	//std::cout<<"WriteReadFSR:"<<err_code<<std::endl;
    }
    if(err_code>0){
	ErrInfo=err_code; 
	return 59;
    }

    return 50;
}


int FPGA::i2creset()
{
    int err_code;
    Mode= 7;
    OutgoingLength=18; 
    IncomingLength=18+400; 
    PacketQueueSize=1;
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    return 20+err_code;
}


int FPGA::i2cDAC(unsigned short Umv, unsigned short DACchannel)
{
    int err_code;
    unsigned short bitcode1 = 1.84859*Umv-14.89;
    if (Umv == 0) {bitcode1 = 0;}
    std::cout<<"I2C DAC voltage bitcode: "<<bitcode1<<std::endl;
    unsigned int i2c_val = (DACchannel<<12) | bitcode1;
    tp.SetI2C(i2c_val);
    Mode= 9;
    OutgoingLength=18; 
    IncomingLength=18+400; 
    PacketQueueSize=1;
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    return 20+err_code;
}


int FPGA::i2cADC(unsigned short channel)
{
    int err_code;
    unsigned int i2c_val = channel;
    tp.SetI2C(i2c_val);
    Mode= 14;
    OutgoingLength=18; 
    IncomingLength=18+400; 
    PacketQueueSize=1;
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    double ADCmV = 2.0818*tp.GetADCresult()+56.15;
    std::cout<< "ADC conversion result of channel " 
	     << tp.GetADCchannel() 
	     << " is " 
	     << tp.GetADCresult() 
	     << " which is " 
	     << ADCmV 
	     << " mV with alert flag "
	     << tp.GetADCalert() 
	     << std::endl;

    return ADCmV;
}


int FPGA::tpulse(unsigned short Npulses, unsigned short div500kHz)
{
    int err_code;
    unsigned int i2c_val = (Npulses<<8) | div500kHz;
    tp.SetI2C(i2c_val);
    Mode= 15;
    OutgoingLength=18; 
    IncomingLength=18+400; 
    PacketQueueSize=1;
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    return 20+err_code;
}


int FPGA::EnableTPulse(int tpulse){
#if DEBUG==2
    std::cout<<"Enter FPGA::EnableTPulse()"<<std::endl;	
#endif
    int err_code;
    if(tpulse){Mode=16;}
    else{Mode=17;}
    OutgoingLength=18; 
    IncomingLength=18; 
    PacketQueueSize=1;
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    return 20+err_code;	
}


int FPGA::EnableFADCshutter(unsigned short FADCshutter)
{
#if DEBUG==2
    std::cout<<"Enter FPGA::EnableTPulse()"<<std::endl;
#endif
    int err_code;
    tp.SetFADCshutter(FADCshutter);
    if(FADCshutter){Mode=8;}
    else{Mode=18;}
    OutgoingLength=18; 
    IncomingLength=18; 
    PacketQueueSize=1;
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    return 20+err_code;
}


/* FPGA::Communication
   returns error-code
   0: allright, a nice and inspiring conversation with a complete handshake
   1: error in function select() - no valid file-descriptors
   2: timeout in select() - FPGA is not answering in time
   3: received packet has wrong packet-number
*/

#ifdef __WIN32__
int FPGA::Communication(char* SendBuffer, char* RecvBuffer)
#else
int FPGA::Communication(unsigned char* SendBuffer, unsigned char* RecvBuffer)
#endif
{
#if DEBUG==2
    std::cout<<"Enter FPGA::Communication()"<<std::endl;	
#endif
    int err_code;
    int RecvBytes;
    SendBuffer[0]=SoftwareCounter; (SoftwareCounter<255) ? ++SoftwareCounter : SoftwareCounter=0;
    SendBuffer[1]=FPGACounter;
    SendBuffer[2]=IncomingLength/256;
    SendBuffer[3]=IncomingLength%256;
    SendBuffer[4]=Mode;
    SendBuffer[7]=tp.GetNumChips();
    SendBuffer[9]=tp.GetPreload();
    SendBuffer[10]=tp.GetOption();
    SendBuffer[14]=tp.GetI2C()/65536;
    SendBuffer[15]=tp.GetI2C()/256;
    SendBuffer[16]=tp.GetI2C()%256;
    //SendBuffer[5]=M0 + 2 * M1 + 4 * Enable_IN + 8 * Shutter + 16 * Reset + 32 * Enable_Test;
    //usleep(3000);
    RecvBytes=send(sock,SendBuffer,OutgoingLength,0);
#if DEBUG==1 
    std::cout << "Paket gesendet "<<RecvBytes<< std::endl;
#endif

    //quitusleep(3000);
    testfd=readfd;
    err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, NULL);//&timeout);
#ifdef __WIN32__
    // check latest reported error status for last Windows Sockets operation that failed
    int error;
    std::cout << "error code "<<err_code<< std::endl;
    error=WSAGetLastError();
    std::cout << "error "<<error<< std::endl;
#endif

#if DEBUG==1
    if (err_code<0) std::cout << "Fehler in select" << std::endl;
    if (err_code==0) std::cout << "Timeout in select" << std::endl; 
#endif
    if(err_code<0){return 1;} else if(err_code==0){return 2;} else{err_code=0;}

    RecvBytes=recv(sock,RecvBuffer,PLen+18,0);
    //usleep(3000);
#if DEBUG==1
    std::cout << "Antwort empfangen:" << RecvBytes << "Bytes" << std::endl;
    std::cout << "RecvBuffer[1]:" << RecvBuffer[1] 
	      << ", FPGACounter+1:" 
	      << FPGACounter+1 << std::endl;
#endif
		
    if(RecvBuffer[1]!=(FPGACounter+1)%256){
#if DEBUG==1
	std::cout << "  Antwort hat falsche Paket-Nummer. SOLL=" 
		  << FPGACounter+1 << ", IST=" 
		  << RecvBuffer[1] << std::endl << std::endl; 
#endif
	err_code=3;
    }

    unsigned short ADC_result = 0;
    unsigned short ADC_ChAlert = 0;
    unsigned short FADCtriggered = 0;
    unsigned short ExtraByte = 0;

    // TODO: see if we need to change something for HV_FADC_Obj
    //readout of the FADC bit - set the flag if it equals 1
    FADCtriggered = RecvBuffer[10];
    _fadcBit = RecvBuffer[10];
    if((_fadcBit == 1) && !_fadcFlag) _fadcFlag = true;
    
    ADC_ChAlert=RecvBuffer[14];
    ADC_result+=RecvBuffer[15] << 8;
    ADC_result+=RecvBuffer[16];
    ExtraByte = RecvBuffer[17];
    tp.SetFADCtriggered(FADCtriggered);
    tp.SetExtraByte(ExtraByte);
    tp.SetADCresult(ADC_ChAlert,ADC_result);
      
    return err_code;
}


int FPGA::ReadoutFadcBit(){
#if DEBUG==2
    std::cout << "Enter FPGA::ReadoutFadcBit()"<<std::endl;
#endif
    //std::cout << "Fadc Bit: " << _fadcBit << std::endl;
    return _fadcBit;
}


int FPGA::ReadoutFadcFlag(){
#if DEBUG==2
    std::cout << "Enter FPGA::ReadoutFadcFlag()"<<std::endl;
#endif
    if(_fadcFlag)
    {
    return 1;
}
    else
    {
    return 0;
}
}


void FPGA::ClearFadcFlag(){
#if DEBUG==2
    std::cout << "Enter FPGA::ClearFadcFlag()"<<std::endl;
#endif
    _fadcFlag = false;
    return;
}


#ifdef __WIN32__
int FPGA::Communication2(char* SendBuffer, char* RecvBuffer, int HitsMode, unsigned short chip)
#else
int FPGA::Communication2(unsigned char* SendBuffer, unsigned char* RecvBuffer, int HitsMode, unsigned short chip)
#endif
{

#if DEBUG==2
    std::cout<<"Enter FPGA::Communication2()"<<std::endl;
#endif
    int err_code, RecvBytes, Hits;

    SendBuffer[0]=SoftwareCounter; (SoftwareCounter<255) ? ++SoftwareCounter : SoftwareCounter=0;
    SendBuffer[1]=FPGACounter;
    SendBuffer[2]=IncomingLength/256;
    SendBuffer[3]=IncomingLength%256;
    SendBuffer[4]=Mode;
    SendBuffer[7]=tp.GetNumChips();
    SendBuffer[8]=chip;
    SendBuffer[9]=tp.GetPreload();
    SendBuffer[10]=tp.GetOption();
    SendBuffer[14]=tp.GetI2C()/65536;
    SendBuffer[15]=tp.GetI2C()/256;
    SendBuffer[16]=tp.GetI2C()%256;
    //SendBuffer[5]=M0 + 2 * M1 + 4 * Enable_IN + 8 * Shutter + 16 * Reset + 32 * Enable_Test;
    //usleep(3000);
    RecvBytes=send(sock,SendBuffer,OutgoingLength,0);
#if DEBUG==1
    std::cout << "Paket gesendet "<<RecvBytes<< std::endl;
#endif
    //usleep(3000);
    testfd=readfd;

    err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, NULL);//&timeout);
#if DEBUG==1
    if (err_code<0) std::cout << "Fehler in select" << std::endl;
    if (err_code==0) std::cout << "Timeout in select" << std::endl;
#endif
    if(err_code<0){
	return 1;
    } 
    else if(err_code==0){
	return 2;
    } 
    else{
	err_code=0;
    }


    RecvBytes=recv(sock,RecvBuffer,PLen+18,0);
    //usleep(3000);
#if DEBUG==1
    std::cout << "Antwort empfangen:" << RecvBytes << "Bytes" << std::endl;
    std::cout << "RecvBuffer[1]:" << RecvBuffer[1] << ", FPGACounter+1:" << FPGACounter+1 << std::endl;
#endif

    if(RecvBuffer[1]!=(FPGACounter+1)%256){
#if DEBUG==1
	std::cout << "  Antwort hat falsche Paket-Nummer. SOLL=" << FPGACounter+1 << ", IST=" << RecvBuffer[1] << std::endl << std::endl;
#endif
	err_code=3;
    }


    // TODO: check if need to change something for HV_FADC_Obj
    //readout of the FADC bit - set the flag if it equals 1
    _fadcBit = RecvBuffer[16];
    if((_fadcBit == 1) && !_fadcFlag) _fadcFlag = true;
  
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;
    if (HitsMode==2) {
	err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, &timeout);//&timeout);
#if DEBUG==1
	std::cout << "Timeout 10 sec used" << std::endl;
#endif
	//++SoftwareCounter;
	RecvBytes=recv(sock,RecvBuffer,PLen+18,0);

#if DEBUG==1
	std::cout << "Antwort empfangen:" << RecvBytes << "Bytes" << std::endl;
	std::cout << "RecvBuffer[1]:" << RecvBuffer[1] 
		  << ", FPGACounter+1:" << FPGACounter+1 << std::endl;
#endif

	if(RecvBuffer[1]!=(FPGACounter+1)%256){
#if DEBUG==1
	    std::cout << "  Antwort hat falsche Paket-Nummer. SOLL=" 
		      << FPGACounter+1 
		      << ", IST=" 
		      << RecvBuffer[1] 
		      << std::endl << std::endl;
#endif
	    err_code=3;
	}
	err_code=0;
    }


    FPGACounter=RecvBuffer[1];
    Hits=RecvBuffer[10] << 16;
    Hits+=RecvBuffer[11] << 8;
    Hits+=RecvBuffer[12];
    int Storage = RecvBuffer[13];
#if DEBUG==1
    std::cout << "Number of Hits: " << Hits << "" << std::endl;
    std::cout << "Communication Info: Number of Hits: " << Hits 
	      << " , HitsMode: "<< HitsMode 
	      << " , FPGA storage used: " << Storage
	      << " , " <<  std::endl;
#endif

    if (HitsMode==1) {
	return Hits;
    }
    else return err_code;
}

#ifdef __WIN32__
int FPGA::CommunicationReadSend(char* SendBuffer, char* RecvBuffer, int HitsMode, unsigned short chip)
#else
int FPGA::CommunicationReadSend(unsigned char* SendBuffer, unsigned char* RecvBuffer, int HitsMode, unsigned short chip)
#endif
{

#if DEBUG==2
    std::cout<<"Enter FPGA::Communication()"<<std::endl;
#endif
    int err_code,RecvBytes,Hits;

    SendBuffer[0]=SoftwareCounter; (SoftwareCounter<255) ? ++SoftwareCounter : SoftwareCounter=0;
    SendBuffer[1]=FPGACounter;
    SendBuffer[2]=IncomingLength/256;
    SendBuffer[3]=IncomingLength%256;
    SendBuffer[4]=Mode;
    SendBuffer[7]=tp.GetNumChips();
    SendBuffer[8]=chip;
    SendBuffer[9]=tp.GetPreload();
    SendBuffer[10]=tp.GetOption();
    SendBuffer[14]=tp.GetI2C()/65536;
    SendBuffer[15]=tp.GetI2C()/256;
    SendBuffer[16]=tp.GetI2C()%256;
    //SendBuffer[5]=M0 + 2 * M1 + 4 * Enable_IN + 8 * Shutter + 16 * Reset + 32 * Enable_Test;
    //usleep(3000);
    RecvBytes=send(sock,SendBuffer,OutgoingLength,0);
#if DEBUG==1
    std::cout << "Paket gesendet " << RecvBytes << std::endl;
#endif
    testfd=readfd;

    err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, NULL);//&timeout);
#if DEBUG==1
    if (err_code<0) std::cout << "Fehler in select" << std::endl;
    if (err_code==0) std::cout << "Timeout in select" << std::endl;
#endif
    if(err_code<0){return 1;} else if(err_code==0){return 2;} else{err_code=0;}


    RecvBytes=recv(sock,RecvBuffer,PLen+18,0);
#if DEBUG==1
    std::cout << "Antwort empfangen:" << RecvBytes << "Bytes" << std::endl;
    std::cout << "RecvBuffer[1]:" << RecvBuffer[1] 
	      << ", FPGACounter+1:" << FPGACounter+1 << std::endl;
#endif

    if(RecvBuffer[1]!=(FPGACounter+1)%256){
#if DEBUG==1
	std::cout << "  Antwort hat falsche Paket-Nummer. SOLL=" 
		  << FPGACounter+1 
		  << ", IST=" 
		  << RecvBuffer[1] 
		  << std::endl << std::endl;
#endif
	err_code=3;
    }

    FPGACounter=RecvBuffer[1];
    Hits=RecvBuffer[10] << 16;
    Hits+=RecvBuffer[11] << 8;
    Hits+=RecvBuffer[12];
    int Storage = RecvBuffer[13];

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;
    if (chip == tp.GetNumChips()){
	if (HitsMode==2 or Hits*4 <PLen) {
	    err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, &timeout);//&timeout);
#if DEBUG==1
	    std::cout << "Timeout 50 ms used" << std::endl;
#endif
	    std::cout << "Timeout 50 ms used of chip " <<chip << std::endl;
	    //++SoftwareCounter;
	    
	    // TODO: understand why different variables are given for each platform
#ifdef __WIN32__
            recv(sock,RecvBuffer,PLen+18,0);
#else
            recv(sock,SendBuffer,PLen+18,0);  //??
#endif
	    
	    std::cout << "Packet indicating end of data transfer from Chip to "
		      << "FPGA received" << std::endl;

#if DEBUG==1
	    std::cout << "Antwort empfangen:" << RecvBytes << "Bytes" << std::endl;
	    std::cout << "RecvBuffer[1]:" << RecvBuffer[1] 
		      << ", FPGACounter+1:" << FPGACounter+1 << std::endl;
#endif

	    if(RecvBuffer[1]!=(FPGACounter+1)%256){
#if DEBUG==1
		std::cout << "  Antwort hat falsche Paket-Nummer. SOLL=" 
			  << FPGACounter+1 
			  << ", IST=" 
			  << RecvBuffer[1] 
			  << std::endl << std::endl;
#endif
		err_code=3;
	    }

	    err_code=0;
	}
    }
	

#if DEBUG==1
    std::cout << "Number of Hits: " << Hits << ""<< std::endl;
#endif
    std::cout << "Communication Info: Number of Hits: " << Hits 
	      << " on chip " << chip
	      << " , HitsMode: " << HitsMode 
	      << " , FPGA storage used: " << Storage 
	      << " , " <<  std::endl;
    if (HitsMode==1) {
	return Hits;
    }
    else return err_code;
}
	


int FPGA::SaveData(const char* filename){
#if DEBUG==2
    std::cout<<"Enter FPGA::SaveData(file)"<<std::endl;	
#endif
    //
    int x,y,b; // b: for 14 bit counter in each pixel
    int aktBit;
    int pix[256][256];
	
	
    for(y=0;y<256;++y)for(x=0;x<256;++x)pix[y][x]=0;
    std::cout<<"pack matrix"<<std::endl;
    for(y=0;y<256;++y){
	for(b=0;b<14;++b){
	    for(x=0;x<256;++x){
		aktBit=y*256*14+b*256+(255-x)+8;
		if((PacketQueue[(aktBit/8)/PLen][18+((aktBit/8)%PLen)] & 1<<(7-(aktBit%8)))>0){
		    pix[y][x]+=1<<(13-b);
		}
		//else{printf("y=%i, b=%i, x=%i, aktbit=%i\n",y,b,x,aktBit);}
	    }	
	}
    }

    FILE* f=fopen(filename,"w"); if(f==NULL) return 1;
	
#if PERFORMANCE==1
    fwrite(pix,sizeof(int),256*256,f);
#else
    for(y=0;y<256;y++){
	for(x=0;x<255;x++){
	    fprintf(f,"%i ",pix[y][x]);
	} 
	fprintf(f,"%i\n",pix[y][255]);
    }
#endif
    fclose(f);
    return 0;
}


int FPGA::SaveData(int pix[9][256][256]){
#if DEBUG==2
    std::cout<<"Enter FPGA::SaveData(array)"<<std::endl;	
#endif
    //
    int x,y,b;
    int aktBit;
    // get the preload in order to get rid of if else statements within the 
    // 4 nested for loops...
    // now simply add preload to aktBit
    int preload = tp.GetPreload();

    for (unsigned short chip = 1;chip <= 8;chip++){
	for(y=0;y<256;++y){
	    for(x=0;x<256;++x){
		pix[chip][y][x]=0;
	    }
	}
    }
    for (unsigned short chip = 1;chip <= tp.GetNumChips() ;chip++){
	for(y=0;y<256;++y){
	    for(b=0;b<14;++b){
		for(x=0;x<256;++x)
		{
		    // add preload at the end; might need to change the preload
		    // you apply on program start since if statement included 
		    // different values for adding than actual preload value 
		    // before
		    aktBit=y*256*14+b*256+(255-x)+8*tp.GetNumChips()+((256*256*14)+256)*(chip-1) + preload;
		    // +8*tp.GetNumChips() for preload. For xinlinx board: additionally +1!, Octoboard also +1 single chip,3 for 8 chips
		    if((((*PackQueueReceive)[(aktBit/8)/PLen][18+((aktBit/8)%PLen)]) & 1<<(7-(aktBit%8)))>0){
			pix[chip][y][x]+=1<<(13-b);
		    }
		}
	    }
	}
    }
    return 0;
}


int FPGA::SaveData(std::vector<std::vector<std::vector<int> > > *VecData){
#if DEBUG==2
    std::cout<<"Enter FPGA::SaveData(array)"<<std::endl;
#endif
    //
    int x,y,b;
    int aktBit;
    // get the preload in order to get rid of if else statements within the 
    // 4 nested for loops...
    // now simply add preload to aktBit
    int preload = tp.GetPreload();

    //for(y=0;y<256;++y)for(x=0;x<256;++x)pix[y][x]=0;
    for (unsigned short chip = 1;chip <= tp.GetNumChips() ;chip++){
	for(y=0;y<256;++y){
	    for(b=0;b<14;++b){
		for(x=0;x<256;++x)
		{
		    // add preload at the end; might need to change the preload
		    // you apply on program start since if statement included 
		    // different values for adding than actual preload value 
		    // before
		    aktBit=y*256*14+b*256+(255-x)+8*tp.GetNumChips()+((256*256*14)+256)*(chip-1) + preload;
		    // +8*tp.GetNumChips() for preload. For xinlinx board: additionally +1!, Octoboard also +1 single chip,3 for 8 chips
		    if ((((*PackQueueReceive)[(aktBit/8)/PLen][18+((aktBit/8)%PLen)]) & 1<<(7-(aktBit%8)))>0){
			(*VecData)[chip][y][x]+=1<<(13-b);
		    }
		}
	    }
	}
    }
    return 0;
}

int FPGA::SaveData(int pix[256][256], int NumHits){
#if DEBUG==2
    std::cout<<"Enter FPGA::SaveData(array)"<<std::endl;
#endif
    int byte;
    int packet = 0;
    int Hits;
    if (NumHits > 4096){
	Hits = 4096;
    }
    else {
	Hits =NumHits;
    }
    int PacketLenght = PLen;
    int Packets = (((Hits*4)+PLen-1)/PLen);
    int	LastPacketLenght = 18+(Hits*4%PLen);
    if ((((Hits)*4)%PLen) == 0) {
	LastPacketLenght = 18+PLen;
    }
    if (Hits*4 == PLen) {
	Packets = 1;
    }
    if (Hits !=0 ){
	for (byte=18; byte<=PacketLenght+18;byte=byte+4) {
	    int x = (*PackQueueReceive)[packet][byte];
	    int y = (*PackQueueReceive)[packet][byte + 1];
	    int val = ((*PackQueueReceive)[packet][byte + 2] << 8) + (*PackQueueReceive)[packet][byte + 3];
	    pix[y][x]=val;
	    //std::cout << "pixel  " << y <<" "<< x << " has "<< val<<" hits"<< std::endl;
	    if (packet == Packets-1){
		PacketLenght = LastPacketLenght-18-4;
	    }
	    if (byte+4==PLen+18 or byte+5==PLen+18 or byte+6==PLen+18 or byte+7==PLen+18 ) {
		if (packet < Packets-1){
		    packet++;
		    byte=14;
		}
	    }
	}
    }
    //std::cout << "pixel 102 43   has "<< pix[102][43]<<" hits"<< std::endl;
    return 0;

}

int FPGA::SaveData(std::vector<int> *pHitArray, int NumHits ){
#if DEBUG==2
    std::cout<<"Enter FPGA::SaveData(array)"<<std::endl;
#endif
    //pHitArray->push_back(NumHits);
    (*pHitArray)[0] = NumHits;
    //std::cout << "Hello this is SaveData" <<std::endl;
    int i=0;
    int byte;
    int packet = 0;
    int Hits;
    if (NumHits > 4096){
	Hits = 4096;
    }
    else {
	Hits =NumHits;
    }
    int PacketLenght = PLen;
    int Packets = (((Hits*4)+PLen-1)/PLen);
    int LastPacketLenght = 18+(Hits*4%PLen);
    if ((((Hits)*4)%PLen) == 0) {
	LastPacketLenght = 18+PLen;
    }
    if (Hits*4 == PLen) {
	Packets = 1;
    }
    if (Hits !=0 ){
	for (byte=18; byte<=PacketLenght+18;byte=byte+4) {
	    (*pHitArray)[i+1] = ((*PackQueueReceive)[packet][byte]); //std::cout << "i: " << i<<std::endl;
	    i++;
	    (*pHitArray)[i+1] = ((*PackQueueReceive)[packet][byte + 1]);//std::cout << "i: " << i<<std::endl;
	    i++;
	    (*pHitArray)[i+1] = (((*PackQueueReceive)[packet][byte + 2] << 8) + (*PackQueueReceive)[packet][byte + 3]);//std::cout << "i: " << i<<std::endl;
	    i++;
	    //std::cout << "Hit number: " << (byte+4-18)/4<< ", x: "<< x<< ", y: "<<y<<", value: "<<pix[y][x]<<", Packet:"<< packet<<  std::endl;
	    if (packet == Packets-1){
		PacketLenght = LastPacketLenght-18-4;
	    }
	    if (byte+4==PLen+18 or byte+5==PLen+18 or byte+6==PLen+18 or byte+7==PLen+18 ) {
		if (packet < Packets-1){
		    packet++;
		    byte=14;
		}
	    }
	}
    }
    std::cout << "Save Data: Number of Hits: " << NumHits << ""<< std::endl;
    return 0;
}


int FPGA::SaveData(int hit_x_y_val[12288] ,int NumHits){
#if DEBUG==2
    std::cout<<"Enter FPGA::SaveData(array)"<<std::endl;
#endif
    int i=0;
    int byte;
    int packet = 0;
    int Hits;
    if (NumHits > 4096){
	Hits = 4096;
    }
    else {
	Hits =NumHits;
    }
    int PacketLenght = PLen;
    int Packets = (((Hits*4)+PLen-1)/PLen);
    int LastPacketLenght = 18+(Hits*4%PLen);
    if ((((Hits)*4)%PLen) == 0) {
	LastPacketLenght = 18+PLen;
    }
    if (Hits*4 == PLen) {
	Packets = 1;
    }
    if (Hits !=0 ){
	for (byte=18; byte<=PacketLenght+18;byte=byte+4) {
	    hit_x_y_val[i] = (*PackQueueReceive)[packet][byte]; //std::cout << "i: " << i<<std::endl;
	    i++;
	    hit_x_y_val[i] = (*PackQueueReceive)[packet][byte + 1];//std::cout << "i: " << i<<std::endl;
	    i++;
	    hit_x_y_val[i] = ((*PackQueueReceive)[packet][byte + 2] << 8) + (*PackQueueReceive)[packet][byte + 3];//std::cout << "i: " << i<<std::endl;
	    i++;
	    //std::cout << "Hit number: " << (byte+4-18)/4<< ", x: "<< x<< ", y: "<<y<<", value: "<<pix[y][x]<<", Packet:"<< packet<<  std::endl;
	    if (packet == Packets-1){
		PacketLenght = LastPacketLenght-18-4;
	    }
	    if (byte+4==PLen+18 or byte+5==PLen+18 or byte+6==PLen+18 or byte+7==PLen+18 ) {
		if (packet < Packets-1){
		    packet++;
		    byte=14;
		}
	    }
	}
    }
    return 0;
}

int FPGA::SaveData(int **pix){
#if DEBUG==2
    std::cout<<"Enter FPGA::SaveData(array)"<<std::endl;
#endif
    //
    int x,y,b;
    int aktBit;


    for(y=0;y<256;++y){
	for(x=0;x<256;++x){
	    pix[y][x]=0;
	}
    }

    for(y=0;y<256;++y){
	for(b=0;b<14;++b){
	    for(x=0;x<256;++x){
		aktBit=y*256*14+b*256+(255-x)+8;
		if(((*PackQueueReceive)[(aktBit/8)/PLen][18+((aktBit/8)%PLen)] & 1<<(7-(aktBit%8)))>0){
		    pix[y][x]+=1<<(13-b);
		}
	    }
	}
    }
    return 0;
}

int FPGA::SwitchTriggerConnection(int connection){
#if DEBUG==2
    std::cout<<"Enter FPGA::SwitchTriggerConnection()"<<std::endl;
#endif
    TriggerConnectionIsTLU=(connection>0);
    return TriggerConnectionIsTLU;
}

void FPGA::UseFastClock(bool use){
	if (use == true) usefastclock= true;
	else usefastclock = false;
}
