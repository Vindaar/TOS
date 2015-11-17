# Qmake project file to test the HV_FADC_Obj
# written by Sebastian Schmidt
# based on Alexander Deisting FadcVMEReadout.pro

DEPENDPATH  += . src
INCLUDEPATH += . include
QMAKE_CXXFLAGS += -std=c++11 -o0 -g -Wall -W -pedantic -fPIC
OBJECTS_DIR = tmp
MOC_DIR     = tmp
DESTDIR     = bin


HEADERS += include/V1729a_VME.h \
        include/HV_FADC_Obj.h \
        include/vmemodule.h \
        include/vmecontroller.h \
        include/vmusb.h \ 
        include/DeviceVME.h \
        include/High-Level-functions_VME.h \
        include/FADCConstants_V1729a.h \
        include/const.h
SOURCES += src/HV_FADC_Obj.cc \
        src/DeviceVME.cc \
        src/V1729a_VME.cc \
        src/vmecontroller.cpp \
        src/vmusb.cpp \
        src/vmemodule.cpp \
        src/High-Level-functions_VME.cc

#libs
LIBS += -lxx_usb#-Wl,--rpath=/home/tpc/HV_FADC/VME/lib \
        #-Wl,--no-as-needed \
        #-Wl,--rpath=/home/tpc/HV_FADC/FadcVMEReadout/lib \
        #-Wl,--rpath=/home/tpc/HV_FADC/thq_vhs/vmecontrol/vmecontrol-src/lib \
        #-lxx_usb \
        #-lMyVME \
        #-lMyV1729a \
        #-lMyVMEmodule #\

#external libs
QMAKE_LIBDIR += #/home/tpc/HV_FADC/VME/lib \
        #$(LCIO)/lib \
        #/home/tpc/HV_FADC/FadcVMEReadout/lib \
        #/home/tpc/HV_FADC/thq_vhs/vmecontrol/vmecontrol-src/lib


#external headers
INCLUDEPATH += /home/tpc/HV_FADC/FadcVMEReadout/src \
            /usr/include
            #/home/tpc/HV_FADC/VME/src \
            #/home/tpc/HV_FADC/FadcVMEReadout/lib \
            #/home/tpc/HV_FADC/thq_vhs/vmecontrol/vmecontrol-src \
            #/home/tpc/HV_FADC/thq_vhs/vmecontrol/vmecontrol-src/lib \
            #/home/tpc/HV_FADC/VME/lMyVME \

test_object{
        TEMPLATE = app
        SOURCES += src/test_object.cc
        TARGET  = test_object
}
