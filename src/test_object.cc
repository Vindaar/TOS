// This program is used to test
// the basic functionality of the HV_FADC_Obj

#include "HV_FADC_Obj.h"
#include <iostream>
#include <string>
#include <cstdlib>

#include <QString>
#include <QTimer>
#include <QTime>
#include <QObject>

bool update(HV_FADC_Obj *myHV_FADC_Obj){

    int b;
    float c, d;
    std::string input("");
    std::string abort_string("q");

    std::cout << "is connected next" << std::endl;
    std::cin >> input;
    bool a = myHV_FADC_Obj->H_IsConnected();
    std::cout << a << std::endl;

    myHV_FADC_Obj->F_StartAcquisition();
    std::cin >> input;
    myHV_FADC_Obj->F_Reset();
//    int b = 0;
    b = myHV_FADC_Obj->H_GetModuleStatus();
    std::cout << "Status  " << b << std::endl;

//    QString s;
//    // access to HV module currently causes SegFault
    c = myHV_FADC_Obj->H_GetModuleCurrentRampSpeed();
    d = myHV_FADC_Obj->H_GetModuleTemperature();

    std::cout << "Ramp Speed  " << c << std::endl;
    std::cout << "Temperature  " << d << std::endl;
//    int f = myHV_FADC_Obj->HV_module->GetModuleBaseAddress();
//    myHV_FADC_Obj->HV_module->GetModuleSerialNumber();
//    myHV_FADC_Obj->HV_module->GetModuleFirmwareRelease();
//    myHV_FADC_Obj->HV_module->GetModuleChannelNumber();

//    std::cout << f << std::endl;

//    s = myHV_FADC_Obj->Controller.initController(0);
//    std::cout << s.toStdString() << std::endl;
    

//    myHV_FADC_Obj->HV_module->GetModuleDeviceClass();
//    std::cout << "Is connected!!!" << a << std::endl;

//    std::cout << "Channel Number" << myHV_FADC_Obj->HV_module->GetModuleChannelNumber() << std::endl;
    

    if(input.find(abort_string) != std::string::npos){
	return true;
    }
    else{ return false; };
}


int main( int argc, char **argv ){

    // create a new HV_FADC_Obj
    int sAddress_fadc  = 1;
    uint16_t baseAddress_hv = 0x4400;

    HV_FADC_Obj *myHV_FADC_Obj = new HV_FADC_Obj(sAddress_fadc, baseAddress_hv);


    std::cout << "Trying to reset FADC" << std::endl;
    myHV_FADC_Obj->F_Reset();

    myHV_FADC_Obj->FADC_Functions->printSettings();

    myHV_FADC_Obj->F_StartAcquisition();

//    std::cin.ignore();

    myHV_FADC_Obj->F_Reset();


    // perform initialization using InitHFOForTOS()
    QString filepath = "../config/HFO_settings.ini";
    myHV_FADC_Obj->InitHFOForTOS(filepath, true);


    //QTimer* myTimer = new QTimer();
    //myTimer->setInterval(50);
    //connect(myTimer, SIGNAL(timeout()), myHV_FADC_Obj , SLOT(update(myHV_FADC_Obj)));
    
    bool exit = true;
    while(exit == false){
    	exit = update(myHV_FADC_Obj);
    }
    
    delete(myHV_FADC_Obj);
    
    return 0;

}
