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
#include "networkWrapper.hpp"
//#define DEBUG 2

//C~tor
FPGA::FPGA():
    ErrInfo(0), 
    _fadcBit(0),
    _fadcFlag(false), 
    ok(1), 
    TriggerConnectionIsTLU(0), 
    Mode(0),
    IncomingLength(0), 
    OutgoingLength(0), 
    PacketQueueSize(1),
    SoftwareCounter(0), 
    FPGACounter(0),
    PackQueueReceive( new std::vector<std::vector<unsigned char> >(PQueue*8, std::vector<unsigned char>(PLen+18)))
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

    int sock_recv_buf_size;
    int sock_send_buf_size;
    int i_recv=sizeof(sock_recv_buf_size);
    int i_send=sizeof(sock_send_buf_size);
    
    // TODO: ? instead of performing a sanity check, one could in principle only
    //       check the return value of setsockopt (if 0 returned, all good, else 
    //       -1 returned)

    // now get both the send and receiver socket buffer size
    // and set it to a value, which is 'big enough'
    getsockoptWrapper(sock, 
		      SOL_SOCKET, 
		      SO_RCVBUF, 
		      &sock_recv_buf_size, 
		      (socklen_t *)&i_recv);
    getsockoptWrapper(sock, 
		      SOL_SOCKET, 
		      SO_SNDBUF, 
		      &sock_send_buf_size, 
		      (socklen_t *)&i_send);
    if (sock_recv_buf_size != DEFAULT_SOCKET_BUFFER_SIZE){
	// if sock buffer size is not the wanted value, set value we want
	sock_recv_buf_size = DEFAULT_SOCKET_BUFFER_SIZE;
	sock_send_buf_size = DEFAULT_SOCKET_BUFFER_SIZE;
	setsockoptWrapper(sock, 
			  SOL_SOCKET, 
			  SO_RCVBUF, 
			  &sock_recv_buf_size,
			  sizeof(sock_recv_buf_size));
	setsockoptWrapper(sock, 
			  SOL_SOCKET, 
			  SO_SNDBUF, 
			  &sock_send_buf_size,
			  sizeof(sock_send_buf_size));
	i_recv = sizeof(sock_recv_buf_size);
	i_send = sizeof(sock_send_buf_size);

	// now perform a sanity check, if the size was correctly applied
	// get new sizes
	getsockoptWrapper(sock, 
			  SOL_SOCKET, 
			  SO_RCVBUF, 
			  &sock_recv_buf_size, 
			  (socklen_t *)&i_recv);
	getsockoptWrapper(sock, 
			  SOL_SOCKET, 
			  SO_SNDBUF, 
			  &sock_send_buf_size, 
			  (socklen_t *)&i_send);

	// and check, if the new size is 2 times the wanted size
	// NOTE: the kernel sets the value to 2* the wanted size
	// for 'bookkeeping' reasons 
	// (http://man7.org/linux/man-pages/man7/socket.7.html under SO_RCVBUF)
	// windows does not multiply by 2!
	if( (sock_recv_buf_size == 2*DEFAULT_SOCKET_BUFFER_SIZE) ||
	    (sock_recv_buf_size ==  DEFAULT_SOCKET_BUFFER_SIZE)  ||
	    (sock_send_buf_size == 2*DEFAULT_SOCKET_BUFFER_SIZE) ||
	    (sock_send_buf_size ==   DEFAULT_SOCKET_BUFFER_SIZE) ){
	    std::cout << "Socket buffer sizes set correctly." << std::endl;
	}
	else{
	    std::cout << "WARNING: new socket buffer size could not be set correctly!"
		      << std::endl;
	}
    }

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
    //M0=1; M1=1; Enable_IN=1; Shutter=1; Reset=0; TODO: why is ModL=15 mentioned here?? //ModL= 15 = 0x0f
    // General Reset 0x01
    Mode = 0x01;
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
    //M0=1; M1=1; Enable_IN=1; Shutter=0; Reset=1; TODO: why ModL=23 mentioned here?? //ModL= 23 = 0x17
    // Start Counting 0x02
    Mode = 0x02;
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
    //M0=1; M1=1; Enable_IN=0; Shutter=0; Reset=1; TODO: why ModL=23 mentioned here?? //ModL= 23 = 0x17
    // Stop Counting 0x03
    Mode = 0x03;
    OutgoingLength=18; 
    IncomingLength=18; 
    PacketQueueSize=1;  
    err_code=Communication(PacketBuffer,PacketQueue[0]);
    if((err_code==0)||(err_code==3)){tp.SetCounting(0);}
    return 20+err_code;
}

// << "CountingTrigger <shutter-time>                = 2t <shutter-time>: "
// << "open shutter after external trigger for t[µs] = 46* <shutter-time>/40 \n\t"

int FPGA::CountingTrigger(int time){
    // int time          integer in range {1, 255}
#if DEBUG==2
    std::cout<<"Enter FPGA::CountingTrigger()"<<std::endl;	
#endif
    int err_code;
    //M0=1; M1=1; Enable_IN=0; Shutter=0; Reset=1; 	//ModL= 23 = 0x17
    if (_usefastclock){
	// Start Counting external Trigger LEMO fast clock 0x18
	Mode = 0x18;
    }
    else{
	// Start Counting external Trigger LEMO 0x17
	Mode = 0x17;
    }
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
    if (_usefastclock){
	// standard, Mode = 20 in Hex: 0x14
	if      (modeSelector == 0) Mode = 0x14;
	// long,     Mode = 30 in Hex: 0x1E
	else if (modeSelector == 1) Mode = 0x1E;
	// verylong, Mode = 31 in Hex: 0x1F
	else if (modeSelector == 2) Mode = 0x1F;
    }
    else {
	// standard, Mode = 19 in Hex: 0x13
	if      (modeSelector == 0) Mode = 0x13;
	//long,      Mode = 21 in Hex: 0x15
	else if (modeSelector == 1) Mode = 0x15;
	//verylong,  Mode = 22 in Hex: 0x16
	else if (modeSelector == 2) Mode = 0x16;
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

int FPGA::SetMatrix(){
#if DEBUG==2
    std::cout<<"Enter FPGA::SetMatrix()"<<std::endl;	
#endif

    int err_code;
    std::vector<std::vector<unsigned char> > *PackQueue = new std::vector<std::vector<unsigned char> >(PQueue*tp.GetNumChips(), std::vector<unsigned char>(PLen+18));
    //tp.PackMatrix(PacketQueue);
    tp.PackMatrix(PackQueue);

    //M0=0; M1=1; Enable_IN=0; Shutter=1; Reset=1; 	//ModL= 26 = 0x1a
    // Start Set Matrix 0x0A
    Mode = 0x0A;
    OutgoingLength=18+PLen; 
    IncomingLength=18; 
    PacketQueueSize=PQueue*tp.GetNumChips();
	
    err_code=Communication(&((*PackQueue)[0][0]),PacketBuffer);
    if(err_code>0)return 40+err_code;
    // Forward Set Matrix 0x0B
    Mode = 0x0B; 
    OutgoingLength=18+PLen;
	
    for(int p=1;p<PacketQueueSize;++p){
	//usleep(100);
	if(p==PacketQueueSize-1){
	    OutgoingLength=18+((256*256*14+8+256)*tp.GetNumChips())/8%PLen;
	    // End Set Matrix
	    Mode = 0x0C;
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
    // Set DAC write fsr
    Mode = 0x0D;
    // changed to 34*tp.GetNumChips() (in Alex code was 32*...)
    // 34 corresponds to: 256 bit (32 byte) + 8 bit (1 byte) preload and 1 more byte to process
    // TODO: + 2 why? not sure, somehow needed?
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
    // I²C reset 0x07
    Mode = 0x07;
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
    // I²C DACs
    Mode = 0x09;
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
    // Readout ADC 0x0E
    Mode = 0x0E;
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
    // Testpulses 0x0F
    Mode = 0x0F;
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
    if(tpulse){
	// External Testpulses Enable  0x10
	Mode = 0x10;
    }
    else{
	// External Testpulses Disable 0x11
	Mode = 0x11;
    }
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
    if(FADCshutter){
	// FADC trigger closes shutter on
	// Mode = 0x20 == 32
	Mode = 0x20;
    }
    else{
	// Mode = 0x21 == 33
	// FADC trigger closes shutter off
	Mode = 0x21;
    }
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

int FPGA::Communication(unsigned char* SendBuffer, unsigned char* RecvBuffer)
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
    
    RecvBytes=sendWrapper(sock,SendBuffer,OutgoingLength,0);
#if DEBUG==1 
    std::cout << "Paket gesendet "<<RecvBytes<< std::endl;
#endif

    //quitusleep(3000);
    testfd=readfd;
    
    
    // set the timeout to 20ms. If no answer was given until then, either chip defect 
    // or (more likely) no chip is connected
    timeout.tv_sec = 0;
    timeout.tv_usec = 20000;
    err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, &timeout);

#ifdef __WIN32__
    // check latest reported error status for last Windows Sockets operation that failed
    int error;
    error=WSAGetLastError();
#endif

#if DEBUG==1
    if (err_code<0) std::cout << "Fehler in select" << std::endl;
    if (err_code==0) std::cout << "Timeout in select" << std::endl; 
#endif
    // check if timeout expired, before chip answered. in that case,
    // select returns 0 and we return 2
    if(err_code<0){
	return 1;
    } 
    else if(err_code==0){
	return 2;
    } 
    else{
	err_code=0;
    }

    RecvBytes=recvWrapper(sock,RecvBuffer,PLen+18,0);
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


int FPGA::Communication2(unsigned char* SendBuffer, unsigned char* RecvBuffer, int HitsMode, unsigned short chip)
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
    RecvBytes=sendWrapper(sock,SendBuffer,OutgoingLength,0);
#if DEBUG==1
    std::cout << "Paket gesendet "<<RecvBytes<< std::endl;
#endif
    //usleep(3000);
    testfd=readfd;

    // set the timeout to 20ms. If no answer was given until then, either chip defect 
    // or (more likely) no chip is connected
    timeout.tv_sec = 0;
    timeout.tv_usec = 20000;
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


    RecvBytes=recvWrapper(sock,RecvBuffer,PLen+18,0);
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
	RecvBytes=recvWrapper(sock,RecvBuffer,PLen+18,0);

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

int FPGA::CommunicationReadSend(unsigned char* SendBuffer, unsigned char* RecvBuffer, int HitsMode, unsigned short chip)
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
    RecvBytes=sendWrapper(sock,SendBuffer,OutgoingLength,0);
#if DEBUG==1
    std::cout << "Paket gesendet " << RecvBytes << std::endl;
#endif
    testfd=readfd;

    // set the timeout to 20ms. If no answer was given until then, either chip defect 
    // or (more likely) no chip is connected
    timeout.tv_sec = 0;
    timeout.tv_usec = 20000;
    err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, NULL);//&timeout);
#if DEBUG==1
    if (err_code<0) std::cout << "Fehler in select" << std::endl;
    if (err_code==0) std::cout << "Timeout in select" << std::endl;
#endif
    if(err_code<0){return 1;} else if(err_code==0){return 2;} else{err_code=0;}


    RecvBytes=recvWrapper(sock,RecvBuffer,PLen+18,0);
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
	    
	    // NOTE: removed __WIN32__ ifdef with RecvBuffer as array
            recvWrapper(sock,SendBuffer,PLen+18,0);  
	    
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
	if (use == true) _usefastclock= true;
	else _usefastclock = false;
}
