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
#include "hvFadcManager.hpp"
#include <functional>

#include <iostream>
#include <cmath>


//C~tor
FPGA::FPGA(Timepix *tp_pointer_from_parent):
    ErrInfo(0),
    _hvFadcManager(NULL),
    _fadcBit(0),
    _fadcFlag(false),
    _fadcShutterCountOn(DEFAULT_FADC_SHUTTER_COUNT_ON),
    ok(1),
    TriggerConnectionIsTLU(0),
    Mode(0),
    IncomingLength(0),
    OutgoingLength(0), 
    PacketQueueSize(1),
    SoftwareCounter(0),
    FPGACounter(0),
    // TODO: make sure PQueue * 8 contains 8 because of a max number of 8 chips? If so,
    // change definition? However, what to do in case someone wants to increase number of
    // chips later. Need to increase size of PackQueueReceive upon call of SetNumChips()!
    PackQueueReceive( new std::vector<std::vector<unsigned char> >(PQueue*8, std::vector<unsigned char>(PLen+18)))
{
    std::cout<<"constructing FPGA"<<std::endl;

#if DEBUG==2
    std::cout<<"Enter FPGA::FPGA()"<<std::endl;	
#endif //endif if DEBUG==2
    // and now set the timepix object pointer the FPGA object owns to the one
    // we hand to it via the constructor
    tp = tp_pointer_from_parent;

    int err_code;
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //Port = 0x1776;// for SRS //strcpy(ip,IP_ADRESSE); 0x200 also possible (special UDP port)
    Port = 0xAFFE; // for V6
    ip=IP_ADRESSE;

    FD_ZERO(&readfd);
    FD_SET(sock, &readfd);

    // TODO:: UNDERSTAND why we should want to set FD_SET to stdin?! Causes super weird
    // behaviour in case one presses ENTER :/
    // #ifdef __WIN32__
    //
    //    FD_SET(0,&readfd);
    //    0=stdin -> Lesen von der Konsole Geht unter WIN nicht mit select() muss trotzdem rein
    // #else
    //     FD_SET(0,&readfd);
    //     0=stdin -> Lesen von der Konsole
    // #endif

    _timeout.tv_sec   = 5; 
    _timeout.tv_usec  = 10000;
    sckadd.sin_family = AF_INET; 
    sckadd.sin_port   = htons(Port);

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

    // now initialize packet buffer abd queue to 0
    for(int i = 0; i < PLen+18; i++){
	PacketBuffer[i] = 0;
	for(int j = 0; j < PQueue; j++){
	    PacketQueue[j][i] = 0;
	}
    }
    

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

void FPGA::initHV_FADC(hvFadcManager* hvFadcManager){
    // this function hands the pointer to the hvFadcManager object to the
    // FPGA object so that it can access its functions
    _hvFadcManager = hvFadcManager;    
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
    int result = 0;
    result = system(cmd.c_str());
    std::cout<<"\nShow new ARP-Table: arp -a"<<std::endl;
    result = system("arp -a");
    if (result == -1){
	std::cout << "MakeARPEntry() Error: Could not send shell command." << std::endl;
    }
    
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
    // this function simply starts counting and leaves the shutter open
    // until we manually close it again
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
    // we set timeout to 0 as to say that we do not want a timeout
    int timeout = 0;
    err_code=Communication(PacketBuffer, PacketQueue[0], timeout);
    if((err_code==0)||(err_code==3)){tp->SetCounting(1);}
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
    if((err_code==0)||(err_code==3)){tp->SetCounting(0);}
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
    // TODO: check if this is reasonable here!!!
    int timeout = 256*46*time / 40;
    err_code=Communication(PacketBuffer,PacketQueue[0], timeout);
    if((err_code==0)||(err_code==3)){tp->SetCounting(0);}
    return 20+err_code;
}

int FPGA::CountingTime(std::string shutter_time, std::string shutter_range){ 
    // this function is a wrapper around the actual CountingTime function 
    // used to hand the CountingTime function a string containing the
    // shutter range as a string:
    // "standard" : mode = 0
    // "long"     : mode = 1
    // "verylong" : mode = 2

    // call the FPGA ShutterRangeToMode function to convert shutter_range
    // to mode, and then call actual CountingTime function
    int mode;
    mode = ShutterRangeToMode(shutter_range);
    int time;
    time = std::stoi(shutter_time);

    int result;
    result = CountingTime(time, mode);
    return result;
}

int FPGA::CountingTime(int shutter_time, std::string shutter_range){ 
    // this function is a another wrapper around the actual CountingTime function 
    // used to hand the CountingTime function a string containing the
    // shutter range as a string:
    // "standard" : mode = 0
    // "long"     : mode = 1
    // "verylong" : mode = 2

    // call the FPGA ShutterRangeToMode function to convert shutter_range
    // to mode, and then call actual CountingTime function
    int mode;
    mode = ShutterRangeToMode(shutter_range);
    int result;
    result = CountingTime(shutter_time, mode);
    return result;
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
    // using time and modeSelector we can calculate a good timeout value
    int timeout = std::pow(256, modeSelector)*46*time / 40;
    err_code = Communication(PacketBuffer, PacketQueue[0], timeout);

    if (tp->GetFADCshutter()==1)
    {
	int fadcTriggerInLastFrame = _hvFadcManager->GetFadcTriggerInLastFrame();
	std::cout << "FADC trigger at " << fadcTriggerInLastFrame << " clock cycles."<< std::endl;
	// now also read out scintillator counters
	std::pair<unsigned short, unsigned short> scint_counter_pair;
	unsigned short scint1_counter = 0;
	unsigned short scint2_counter = 0;
	scint_counter_pair = _hvFadcManager->GetScintillatorCounters();
	scint1_counter = scint_counter_pair.first;
	scint2_counter = scint_counter_pair.second;
	
	std::cout << "Scintillator triggered "
		  << scint1_counter
		  << "\t" << scint2_counter
		  << " clock cycles before FADC triggered."
		  << std::endl;
    }

    return 20+err_code;
}

bool FPGA::checkIfModeIsCounting(int mode){
    // small helper function which simply checks, if the given mode argument
    // is part of a set of defined values, which correspond to the CountingTime function
    // for different shutter lengths
    // values are also shown in CountingTime function itself
    std::set<int> countingModes = {0x14, 0x1E, 0x1F, 0x13, 0x15, 0x16};
    if( countingModes.find(mode) != countingModes.end() ){
	return true;
    }
    else{
	return false;
    }
}

int FPGA::SetMatrix(){
#if DEBUG==2
    std::cout<<"Enter FPGA::SetMatrix()"<<std::endl;	
#endif

    int err_code;
    std::vector<std::vector<unsigned char> > *PackQueue = new std::vector<std::vector<unsigned char> >(PQueue*tp->GetNumChips(), std::vector<unsigned char>(PLen+18));
    //tp->PackMatrix(PacketQueue);
    tp->PackMatrix(PackQueue);

    //M0=0; M1=1; Enable_IN=0; Shutter=1; Reset=1; 	//ModL= 26 = 0x1a
    // Start Set Matrix 0x0A
    Mode = 0x0A;
    OutgoingLength=18+PLen; 
    IncomingLength=18; 
    PacketQueueSize=PQueue*tp->GetNumChips();
	
    err_code=Communication(&((*PackQueue)[0][0]),PacketBuffer);
    if(err_code>0)return 40+err_code;
    // Forward Set Matrix 0x0B
    Mode = 0x0B; 
    OutgoingLength=18+PLen;
	
    for(int p=1;p<PacketQueueSize;++p){
	//usleep(100);
	if(p==PacketQueueSize-1){
	    OutgoingLength=18+((256*256*14+8+256)*tp->GetNumChips())/8%PLen;
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
    tp->GetFSR(PacketBuffer);
#if DEBUG==3
    for(int i = 18; i < 18 + (32*tp->GetNumChips()) + 2; i++){
	printf("%i ",PacketBuffer[i]);printf("\n");
    }
#endif
    //M0=1; M1=0; Enable_IN=0; Shutter=1; Reset=1; 	//ModL= 25 = 0x19
    // Set DAC write fsr
    Mode = 0x0D;
    // changed to 34*tp->GetNumChips() (in Alex code was 32*...)
    // 34 corresponds to: 256 bit (32 byte) + 8 bit (1 byte) preload and 1 more byte to process
    // TODO: + 2 why? not sure, somehow needed?
    OutgoingLength=18+(34*tp->GetNumChips()) + 2; 
    // changed to 34*tp->GetNumChips() + 2 (in Alex code was 33*tp->GetNumChips())
    IncomingLength=18+(34*tp->GetNumChips()) + 2; 
    PacketQueueSize=1;
    err_code=Communication(PacketBuffer, PacketQueue[0]);
    if(err_code>0){
	return 50+err_code;
    }
    for (auto chip : _chip_set){
	if (err_code>0) {
	    err_code = err_code;
	}
	else {
	    err_code = tp->ChipID(PacketQueue[0],chip);
	}
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
    tp->SetI2C(i2c_val);
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
    tp->SetI2C(i2c_val);
    // Readout ADC 0x0E
    Mode = 0x0E;
    OutgoingLength=18; 
    IncomingLength=18+400; 
    PacketQueueSize=1;
    err_code = Communication(PacketBuffer,PacketQueue[0]);
    if (err_code != 0){
	std::cout << "i2cADC(): Error in communication function." << std::endl;
	return -1;
    }
    double ADCmV = 2.0818*tp->GetADCresult()+56.15;
    std::cout<< "ADC conversion result of channel " 
	     << tp->GetADCchannel() 
	     << " is " 
	     << tp->GetADCresult() 
	     << " which is " 
	     << ADCmV 
	     << " mV with alert flag "
	     << tp->GetADCalert() 
	     << std::endl;

    return ADCmV;
}


int FPGA::tpulse(unsigned short Npulses, unsigned short div500kHz){
    /* 
       unsigned short Npulses: number of pulses to be created
       unsigned short div500kHz: input is divided by 500 and used as frequency
                                 in kHz
				 NOTE: explanation probably wrong! INSTEAD:
				 div500kHz * 500kHz is resulting frequency
				 NOTE2: Oscilloscope:
				     div500kHz = 10 results in: 1 period of 20us
				     -> f = div500kHz * 5e3 Hz
     */
    int err_code;
    unsigned int i2c_val = (Npulses<<8) | div500kHz;
    tp->SetI2C(i2c_val);
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
    if (err_code != 0){
	std::cout << "EnableTPulse(): Error in communication function." << std::endl;
	return -1;
    }
    return 20+err_code;	
}


int FPGA::EnableFADCshutter(unsigned short FADCshutter)
{
#if DEBUG==2
    std::cout<<"Enter FPGA::EnableTPulse()"<<std::endl; 
#endif
    int err_code;
    tp->SetFADCshutter(FADCshutter);
    if(FADCshutter){
	// FADC trigger closes shutter on
	// Mode = 0x20 == 32
	Mode = 0x20;
	tp->SetI2C(_fadcShutterCountOn);
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
    if(FADCshutter){
	// in case we're enabling the FADC to close the shutter
	// set i2c back to 0
	tp->SetI2C(0);
	// set fadc trigger clock cycle back to zero
	_hvFadcManager->SetFadcTriggerInLastFrame(0);
	// also set scintillator counters back to zero
	_hvFadcManager->SetScintillatorCounters(0, 0);
    }
    return 20+err_code;
}


/* FPGA::Communication
   returns error-code
   0: allright, a nice and inspiring conversation with a complete handshake
   1: error in function select() - no valid file-descriptors
   2: _timeout in select() - FPGA is not answering in time
   3: received packet has wrong packet-number
*/

int FPGA::Communication(unsigned char* SendBuffer, unsigned char* RecvBuffer, int timeout)
{
#if DEBUG==2
    std::cout<<"Enter FPGA::Communication()"<<std::endl;	
#endif
    // the timeout integer can be handed to the function to set the timeout. Given in mu seconds
    int err_code;
    SendBuffer[0]=SoftwareCounter; (SoftwareCounter<255) ? ++SoftwareCounter : SoftwareCounter=0;
    SendBuffer[1]=FPGACounter;
    SendBuffer[2]=IncomingLength/256;
    SendBuffer[3]=IncomingLength%256;
    SendBuffer[4]=Mode;
    SendBuffer[7]=tp->GetNumChips();
    SendBuffer[9]=tp->GetPreload();
    SendBuffer[10]=tp->GetOption();
    SendBuffer[14]=tp->GetI2C()/65536;
    SendBuffer[15]=tp->GetI2C()/256;
    SendBuffer[16]=tp->GetI2C()%256;
    //SendBuffer[5]=M0 + 2 * M1 + 4 * Enable_IN + 8 * Shutter + 16 * Reset + 32 * Enable_Test;
    //usleep(3000);

#if DEBUG!=1
    sendWrapper(sock, SendBuffer, OutgoingLength, 0);
#else
    int RecvBytes;
    RecvBytes = sendWrapper(sock,SendBuffer,OutgoingLength,0);
    std::cout << "Paket gesendet "<<RecvBytes<< std::endl;
#endif

    //quitusleep(3000);
    testfd = readfd;
    

    // check if the timeout integer was set to 0 (meaning no timeout or not)
    // set the timeout to timeout. If no answer was given until then, either chip defect 
    // or (more likely) no chip is connected
    // done by calling set timeout function
    bool timeout_flag;
    timeout_flag = SetTimeout(timeout);
    if (timeout_flag == true){
	//std::cout << "using timeout! " << std::endl;
	err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, &_timeout);
    }
    else{
	err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, 0);
    }

#ifdef __WIN32__
    // check latest reported error status for last Windows Sockets operation that failed
    int error;
    error=WSAGetLastError();
#endif

//#if DEBUG==1
    if (err_code < 0)  std::cout << "Fehler in select" << std::endl;
    if (err_code == 0) std::cout << "Communication(): Timeout in select" << std::endl; 
//#endif
    // check if _timeout expired, before chip answered. in that case,
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
#if DEBUG!=1
    recvWrapper(sock, RecvBuffer, PLen + 18, 0);
#else
    //std::cout << "recv() returned errno: " << errno << std::endl;
    //usleep(3000);
    RecvBytes = recvWrapper(sock, RecvBuffer, PLen + 18, 0);
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
    _fadcBit      = RecvBuffer[10];

    // get number of clock cycles between last scintillator signal and FADC
    // trigger
    // two unsigned shorts (at least 16 bit) for the counters for scintillator
    // 1 and 2. 
    unsigned short scint1_counter = 0;
    unsigned short scint2_counter = 0;

    // get bytes 11 to 13, which contain both counters, each as 12-bit word
    // shift MSBs for scintillator 1 counter 4 bit to the left (since we get the 4 LSBs
    // from byte 12)
    scint1_counter  = RecvBuffer[11] << 4;
    // now add the result of the bitwise & operation with 240 / F0 / 11110000
    // then shift the result 4 bits to the right, to get the 4 MSBs from RecvBuffer[12]
    // as the LSBs for the scintillator counter
    scint1_counter += (RecvBuffer[12] & 0xF0) >> 4;
    // scintillator counter 2 same in revers
    // take bitwise & of RecvBuffer[12] with 0xF to get 4 LSBs from byte 12
    // and shift 8 bits to the left, to make the the 8 MSBs of the 12-bit word
    scint2_counter  = (RecvBuffer[12] & 0xF) << 8;
    // now add byte 13, to get the 8 LSBs of the 12-bit word. Put & 0xFF there to
    // show intention
    scint2_counter += RecvBuffer[13] & 0xFF;

    // now read the values for (potentially) the clock cycle at which the FADC triggered
    ADC_ChAlert=RecvBuffer[14];
    ADC_result+=RecvBuffer[15] << 8;
    ADC_result+=RecvBuffer[16];
    ExtraByte = RecvBuffer[17];
    tp->SetFADCtriggered(FADCtriggered);
    tp->SetExtraByte(ExtraByte);
    tp->SetADCresult(ADC_ChAlert, ADC_result);

    // now use both counters to set timepix variables. First set FADC bit, if triggered
    // and if that is the case also write scintillator counts
    // we only want to actually use the scint1,2 and fadc values, if the mode with which
    // the communcation function was called, is actually one of those, which correspond to
    // the CountingTime() function (FADC readout is only implemented there!)
    bool isCountingMode = false;
    isCountingMode = checkIfModeIsCounting(Mode);

    // TODO: build something, which does not check for NULL pointer
    if( (_hvFadcManager != NULL) &&
	(_fadcBit == 1) &&
	(isCountingMode == true) ){
	// if fadc bit was set and Communication was called from CountingTime
	// we set the clock cycle at which the FADC triggered and the clock cycles
	// between last scintillator event and FADC trigger
	_fadcFlag = true;
	int fadcTriggerInLastFrame = (tp->GetI2cResult() << 8) + tp->GetExtraByte();
	_hvFadcManager->SetFadcTriggerInLastFrame(fadcTriggerInLastFrame);
	_hvFadcManager->SetScintillatorCounters(scint1_counter, scint2_counter);
    }
      
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


int FPGA::Communication2(unsigned char* SendBuffer, unsigned char* RecvBuffer, int HitsMode, unsigned short chip, int timeout){
    // return codes:
    // err_code == 0: all good
    // err_code == 1: error in select
    // err_code == 2: timeout in select
    // err_code == 3: wrong number of packets

#if DEBUG==2
    std::cout<<"Enter FPGA::Communication2()"<<std::endl;
#endif
    int err_code;
    int Hits;

    SendBuffer[0]=SoftwareCounter; (SoftwareCounter<255) ? ++SoftwareCounter : SoftwareCounter=0;
    SendBuffer[1]=FPGACounter;
    SendBuffer[2]=IncomingLength/256;
    SendBuffer[3]=IncomingLength%256;
    SendBuffer[4]=Mode;
    SendBuffer[7]=tp->GetNumChips();
    // need to hand chip + 1, because firmware seems to expect numbering starting from 1
    SendBuffer[8]= chip + 1;
    SendBuffer[9]=tp->GetPreload();
    SendBuffer[10]=tp->GetOption();
    SendBuffer[14]=tp->GetI2C()/65536;
    SendBuffer[15]=tp->GetI2C()/256;
    SendBuffer[16]=tp->GetI2C()%256;
    //SendBuffer[5]=M0 + 2 * M1 + 4 * Enable_IN + 8 * Shutter + 16 * Reset + 32 * Enable_Test;
    //usleep(3000);
#if DEBUG!=1
    sendWrapper(sock,SendBuffer,OutgoingLength,0);
#else
    int RecvBytes;
    RecvBytes = sendWrapper(sock,SendBuffer,OutgoingLength,0);
    std::cout << "Paket gesendet "<<RecvBytes<< std::endl;
#endif
    //usleep(3000);
    testfd=readfd;

    // check if the timeout integer was set to 0 (meaning no timeout or not)
    // set the timeout to timeout. If no answer was given until then, either chip defect 
    // or (more likely) no chip is connected
    // done by calling set timeout function
    bool timeout_flag;
    timeout_flag = SetTimeout(timeout);
    if (timeout_flag == true){
	err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, &_timeout);
    }
    else{
	err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, 0);
    }
#if DEBUG==1
    if (err_code<0) std::cout << "Fehler in select" << std::endl;
    if (err_code==0) std::cout << "Communication2(): Timeout in select" << std::endl;
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


#if DEBUG!=1
    recvWrapper(sock,RecvBuffer,PLen+18,0);
#else
    RecvBytes=recvWrapper(sock,RecvBuffer,PLen+18,0);
    //usleep(3000);
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
    if((_fadcBit == 1) && !_fadcFlag){
	_fadcFlag = true;
    }

  
    // check if the timeout integer was set to 0 (meaning no timeout or not)
    // set the timeout to timeout. If no answer was given until then, either chip defect 
    // or (more likely) no chip is connected
    // done by calling set timeout function
    if (HitsMode==2) {
	if (timeout_flag == true){
	    err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, &_timeout);
	}
	else{
	    err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, 0);
	}
#if DEBUG==1
	if (err_code<0) std::cout << "Fehler in select" << std::endl;
	if (err_code==0) std::cout << "Communication2(): Timeout in select" << std::endl;
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


#if DEBUG!=1
	recvWrapper(sock,RecvBuffer,PLen+18,0);
#else
	std::cout << "Timeout " << _timeout.tv_usec << " sec used" << std::endl;
	//++SoftwareCounter;
	RecvBytes=recvWrapper(sock,RecvBuffer,PLen+18,0);
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
#if DEBUG==1
    int Storage = RecvBuffer[13];
    std::cout << "Number of Hits: " << Hits << "" << std::endl;
    std::cout << "Communication Info: Number of Hits: " << Hits 
	      << " , HitsMode: "<< HitsMode 
	      << " , FPGA storage used: " << Storage
	      << " , " <<  std::endl;
#endif

    if (HitsMode==1) {
	return Hits;
    }
    else{
	return err_code;
    }
}

int FPGA::CommunicationReadSend(unsigned char* SendBuffer, unsigned char* RecvBuffer, int HitsMode, unsigned short chip, int timeout)
{

#if DEBUG==2
    std::cout<<"Enter FPGA::Communication()"<<std::endl;
#endif
    int err_code;
    int Hits;

    SendBuffer[0]=SoftwareCounter; (SoftwareCounter<255) ? ++SoftwareCounter : SoftwareCounter=0;
    SendBuffer[1]=FPGACounter;
    SendBuffer[2]=IncomingLength/256;
    SendBuffer[3]=IncomingLength%256;
    SendBuffer[4]=Mode;
    SendBuffer[7]=tp->GetNumChips();
    // need to hand chip + 1, because firmware seems to expect numbering starting from 1
    SendBuffer[8]=chip + 1;
    SendBuffer[9]=tp->GetPreload();
    SendBuffer[10]=tp->GetOption();
    SendBuffer[14]=tp->GetI2C()/65536;
    SendBuffer[15]=tp->GetI2C()/256;
    SendBuffer[16]=tp->GetI2C()%256;
    //SendBuffer[5]=M0 + 2 * M1 + 4 * Enable_IN + 8 * Shutter + 16 * Reset + 32 * Enable_Test;
    //usleep(3000);
#if DEBUG!=1
    sendWrapper(sock,SendBuffer,OutgoingLength,0);
#else
    int RecvBytes;
    RecvBytes=sendWrapper(sock,SendBuffer,OutgoingLength,0);
    std::cout << "Paket gesendet " << RecvBytes << std::endl;
#endif
    testfd=readfd;

    // check if the timeout integer was set to 0 (meaning no timeout or not)
    // set the timeout to timeout. If no answer was given until then, either chip defect 
    // or (more likely) no chip is connected
    // done by calling set timeout function
    bool timeout_flag;
    timeout_flag = SetTimeout(timeout);
    if (timeout_flag == true){
	err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, &_timeout);
    }
    else{
	err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, 0);
    }
#if DEBUG==1
    if (err_code<0) std::cout << "Fehler in select" << std::endl;
    if (err_code==0) std::cout << "CommunicationReadSend(): Timeout in select" << std::endl;
#endif
    if(err_code<0){return 1;} else if(err_code==0){return 2;} else{err_code=0;}

#if DEBUG!=1
    recvWrapper(sock,RecvBuffer,PLen+18,0);
#else
    RecvBytes=recvWrapper(sock,RecvBuffer,PLen+18,0);
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
    // TODO: take this out, if all works
    // no need to set timeout here again
    //_timeout.tv_sec = 0;
    //_timeout.tv_usec = 50000;
    if (chip == tp->GetNumChips() - 1){
	if (HitsMode==2 or Hits*4 <PLen) {
	    err_code=select(FD_SETSIZE, &testfd, (fd_set*)0, (fd_set*)0, &_timeout);//&timeout);
#if DEBUG==1
	    std::cout << "Timeout " << timeout << " mu s used" << std::endl;
	    //std::cout << "Timeout 50 ms used of chip " <<chip << std::endl;
#endif
	    //++SoftwareCounter;
	    
	    // NOTE: removed __WIN32__ ifdef with RecvBuffer as array
            recvWrapper(sock,SendBuffer,PLen+18,0);  
	    
	    #if DEBUG==1
	    std::cout << "Packet indicating end of data transfer from Chip to "
		      << "FPGA received" << std::endl;
	    #endif

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
    int Storage = RecvBuffer[13];
    std::cout << "Number of Hits: " << Hits << ""<< std::endl;
    std::cout << "Communication Info: Number of Hits: " << Hits 
	      << " on chip " << chip
	      << " , HitsMode: " << HitsMode 
	      << " , FPGA storage used: " << Storage 
	      << " , " <<  std::endl;
#endif
    if (HitsMode==1) {
	return Hits;
    }
    else return err_code;
}
	


int FPGA::SaveData(std::string filename){
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

    FILE* f=fopen(filename.c_str(),"w"); if(f==NULL) return 1;
	
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


int FPGA::SaveData(int pix[8][256][256]){
#if DEBUG==2
    std::cout<<"Enter FPGA::SaveData(array)"<<std::endl;	
#endif
    //
    int x,y,b;
    int aktBit;

    for (auto chip : _chip_set){
	for(y=0;y<256;++y){
	    for(x=0;x<256;++x){
		pix[chip][y][x]=0;
	    }
	}
    }
    for (auto chip : _chip_set){
	for(y=0;y<256;++y){
	    for(b=0;b<14;++b){
		for(x=0;x<256;++x)
		{
		    // add preload at the end; might need to change the preload
		    // you apply on program start since if statement included 
		    // different values for adding than actual preload value 
		    // before
		    // NOTE: preload removed, not needed anymore, handled in TOF. Remove this comment
		    // at some point!
		    aktBit=y*256*14+b*256+(255-x)+8*tp->GetNumChips()+((256*256*14)+256)*chip;
		    // +8*tp->GetNumChips() for preload. For xinlinx board: additionally +1!, Octoboard also +1 single chip,3 for 8 chips
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

    for (auto chip : _chip_set){
	for(y=0;y<256;++y){
	    for(b=0;b<14;++b){
		for(x=0;x<256;++x)
		{
		    // add preload at the end; might need to change the preload
		    // you apply on program start since if statement included 
		    // different values for adding than actual preload value 
		    // before
		    // NOTE: preload removed, not needed anymore, handled in TOF. Remove this comment
		    // at some point!		    
		    aktBit = y*256*14 + b*256 + (255-x) + 8*tp->GetNumChips() + ((256*256*14) + 256)*chip;
		    // +8*tp->GetNumChips() for preload. For xinlinx board: additionally +1!, Octoboard also +1 single chip,3 for 8 chips
		    if ((((*PackQueueReceive)[(aktBit/8)/PLen][18+((aktBit/8)%PLen)]) & 1<<(7-(aktBit%8)))>0){
			(*VecData)[chip][y][x]+=1<<(13-b);
		    }
		}
	    }
	}
    }
    return 0;
}

int FPGA::SaveData(FrameArray<int> *pixel_data, int NumHits){
    // this function receives a pointer to a FrameArray and saves the data, which
    // was send from the chip to this array

    // TODO: NOTE: for some reason, in these functions (all which also get a hits value)
    // the array is NOT initialized to zero, before the data is written. This is confusing:
    // if I do not read back the whole frame from the chip, part of the array
    // (all that is NOT read from the chip), could be set to anything. Thus, one would
    // need to make sure that one only reads the pixels from the array, which are written
    // during this call of the function
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
	Hits = NumHits;
    }
    int PacketLength = PLen;
    int Packets = ( ( (Hits * 4) + PLen - 1) / PLen);
    int	LastPacketLength = 18 + (Hits * 4 % PLen);
    if ( ( ( Hits * 4) % PLen) == 0) {
	LastPacketLength = 18 + PLen;
    }
    if (Hits * 4 == PLen) {
	Packets = 1;
    }
    if (Hits !=0 ){
	for (byte=18; byte <= PacketLength + 18; byte = byte + 4){
	    // get x, y and value from the received package
	    int x   = (*PackQueueReceive) [packet][byte];
	    int y   = (*PackQueueReceive) [packet][byte + 1];
	    int val = ((*PackQueueReceive)[packet][byte + 2] << 8) + (*PackQueueReceive)[packet][byte + 3];
	    // and set the array to the corresponding value
	    (*pixel_data)[x][y] = val;
	    //std::cout << "pixel  " << y <<" "<< x << " has "<< val<<" hits"<< std::endl;
	    if (packet == Packets - 1){
		PacketLength = LastPacketLength - 18 - 4;
	    }
	    if (byte + 4 == PLen + 18 or byte + 5 == PLen + 18 or byte + 6 == PLen + 18 or byte + 7 == PLen + 18 ) {
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


int FPGA::SaveData(int pix[256][256], int NumHits){
#if DEBUG==2
    std::cout<<"Enter FPGA::SaveData(int pix[256][256], int NumHits)"<<std::endl;
#endif
    int byte;
    int packet = 0;
    int Hits;
    if (NumHits > 4096){
	Hits = 4096;
    }
    else {
	Hits = NumHits;
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
	for (byte=18; byte<=PacketLenght+18;byte=byte+4){
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
	    if (byte+4==PLen+18 or byte+5==PLen+18 or byte+6==PLen+18 or byte+7==PLen+18 ){
		if (packet < Packets-1){
		    packet++;
		    byte=14;
		}
	    }
	}
    }
    #if DEBUG==2
    std::cout << "Save Data: Number of Hits: " << NumHits << ""<< std::endl;
    #endif
    return 0;
}


int FPGA::SaveData(int hit_x_y_val[12288], int NumHits){
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

// int FPGA::SaveData(){
//     // the empty SaveData function is being called, if we want to read out the chips
//     // (e.g. after setting a matrix in a function) without caring about the contents,
//     // which we read back. We will create a temporary object to store the data in
//     // and then simply call another function
//     int nChips = tp->GetNumChips();
//     std::map<int, Frame> frame_map;
//     for (int i = 1; i <= nChips; i++){
// 	Frame frame;
// 	frame_map.insert(std::pair<int, Frame>(i, frame));
//     }
//     int result = 0;
//     result = SaveData(&frame_map);
//     return result;
// }

int FPGA::SaveData(std::map<int, Frame> *frame_map){

    for (auto &frame_pair : *frame_map){
    	FrameArray<int> pixel_data;
	unsigned short chip = frame_pair.first;
    	SaveData(&pixel_data, chip);
    	frame_pair.second.SetFrame(std::ref(pixel_data));
    }

    return 0;
}

int FPGA::SaveData(FrameArray<int> *pixel_data, unsigned short chip){
    // this function receives a pixel_data data array of type FrameArray (defined in
    // frame.hpp), initializes it to 0 and reads the data from the PackQueueReceive
    // array. The result is saved in the pixel_data array

    // NOTE: maybe x and y need to be exchanged!!!
#if DEBUG==2
    std::cout<<"Enter FPGA::SaveData(array)"<<std::endl;
#endif
    //
    int b;
    int aktBit;
    unsigned short nChips = tp->GetNumChips();

    // initialize the whole array to zero to be sure
    // NOTE: this is done to make sure, even if the user hands a FrameArray, which
    //       was used before that the data makes sense. Important, because data will
    //       be set in pixel_data by ADDING individual bits!
    for(std::size_t y = 0; y < pixel_data->size(); ++y){
	for(std::size_t x = 0; x < (*pixel_data)[x].size(); ++x){
	    (*pixel_data)[x][y] = 0;
	}
    }

    // some helper variables for access and determining whether to add bit or not
    unsigned short current_byte = 0;
    unsigned short check_bit = 0;
    unsigned short bit_value = 0;
    int package = 0;
    int pos_in_package = 0;
    
    for(std::size_t y = 0; y < pixel_data->size(); ++y){
	// the main thing to know to read this data:
	//     the data is read column wise from x = 255 down to
	//     x = 0. However, each single bit for each x = .. value
	//     is done one after another, meaning we need to iterate over
	//     every x = 255 -> 0 value for each bit of the 14 bit register,
	//     which is contained in a single pixel.
	//     The individual rows are done one after the other in this manner.
	for(b = 0; b < 14; ++b){
	    for(std::size_t x = 0; x < (*pixel_data)[x].size(); ++x){
		// aktBit stands for current bit. It is the total counter for bits of the
		// WHOLE data stream for all chips and all pixels simply counting 'up'
		// Calculated as follows:
		//     y * 256 * 14   == the offset for each row
		//     b * 256        == the offset for how many bits we're in
		//                       the creation of the values for a single row
		//                       (x = 256 -> 0 one bit at a time!)
		//     (255 - x)      == go over columns from 255 to 0
		//     8 * nChips     == a preload offset (not the preload in the TOS)
		//                       where we get one single byte for each chip
		//                       in the data stream
		//     ((256*256*14) + 256) * chip == the offset we need to skip whole chips
		//     preload        == the preload which is added to move the data column wise
		//                       (for synchronization). each value of preload shifts
		//                       the whole frame from x -> x + 1
		//                       does not change final result, due to postload of 256 bit
		aktBit = y*256*14 + b*256 + (255-x) +	\
		    8 * nChips +			\
		    ((256*256*14) + 256) * chip;
		// NOTE:  + preload;
		// was removed, because it's handled in TOF now. Remove the references here
		// at some point, once made sure everything works as expected!
		// use current bit to calculate which package the data should be in
		//     aktBit / 8      == the byte which our current bit should be in
		//     / PLen (PLen == 1400, size of network data package - 18 bytes of header)
		//                     == determine the network package to look in
		package = (aktBit / 8) / PLen;
		// use current bit to calculate the correct byte in the current network package
		//     18              == the header of each network package, skipped
		//     aktBit / 8      == the byte which our current bit should be in
		//     % PLen          == modulo to take into account that there are many
		//                        network packages (need byte in single package and
		//                        current bit counts number of bits in ALL packages)
		pos_in_package = 18 + ((aktBit / 8) % PLen);

		// get the current byte
		current_byte   = (*PackQueueReceive)[package][pos_in_package];
		// now check whether current bit in package queue (current byte) is 0 or 1
		check_bit = 1 << (7 - (aktBit % 8));
		// get current bit in byte by doing bitwise AND operation
		// and if the bit is non zero, turn bit_value into 1, 0 otherwise
		bit_value = (current_byte & check_bit) > 0;
		// add this value bitshifted by b bits to left
		(*pixel_data)[x][y] += bit_value << (13 - b);
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

bool FPGA::SetTimeout(int timeout){
    // this function is used to set the timeout for the communication functions to
    // something reasonable
    // the return statement returns whether we have a timeout (true) or not (false)
    bool timeout_flag;
    if (timeout == 0){
	timeout_flag = false;
    }
    else{
	_timeout.tv_sec = 0;
	_timeout.tv_usec = (int) timeout*DEFAULT_TIMEOUT_SCALING_FACTOR + DEFAULT_TIMEOUT_OFFSET;
	timeout_flag =  true;
    }
    return timeout_flag;
}


int FPGA::ShutterRangeToMode(std::string shutter_range){
    // this function converts the shutter range (string of type) to 
    // the mode n
    int n = 0;
    
    if( shutter_range == "standard" ){
	n = 0;
    }
    else if( shutter_range == "long" ){
	n = 1;
    }
    else if( shutter_range == "verylong" ){
	n = 2;
    }

    return n;
}

void FPGA::SetChipSet(const std::set<unsigned short> &chip_set){
    // function to hand a const reference to the _chip_set member variable of the
    // console class, such that always an up-to-date chip set is available to
    // the sub classes

    _chip_set = chip_set;
    // and hand chip set reference further to Timepix
    tp->SetChipSet(chip_set);
}


int FPGA::calcSizeOfLastPackage(int nChips, int p_len){
    // function to calculate the size of the last package to be received in case
    // of full matrix readout (SerialReadOut)
    // inputs:
    // int nChips: number of active chips
    // int p_len: the package length in bytes to be used (either PLen == 1400 or
    // PLenUseful == 1399 in case of 'last byte empty')
    int result = 0;
    int header_size = 18;
    // + 8 + 256 due to post load
    int pix_data_in_bits = PIXPD * PIXPD * BIT_PER_PIX;
    result = header_size + ((pix_data_in_bits + 8 + 256) * nChips) / 8 % p_len;
    return result;
}

