######################################################################
# Automatically generated by qmake (2.01a) Do. Okt 14 10:55:30 2010
######################################################################

TEMPLATE = app
TARGET = TOS
DEPENDPATH += . src
INCLUDEPATH += . include
QMAKE_CXXFLAGS += -std=c++11 -o0 -g -Wall -W -pedantic
OBJECTS_DIR = tmp
MOC_DIR = tmp
DESTDIR = bin

# Input
HEADERS += include/caseHeader.h \ 
           include/console.hpp \ 
           include/fpga.hpp \ 
           include/gui.hpp \ 
           include/header.hpp \ 
           include/pc.hpp \ 
           include/timepix.hpp \
           include/waitconditions.hpp \
           include/V1729a_VME.h \
           include/HV_FADC_Obj.h \
           include/vmemodule.h \
           include/vmecontroller.h \
           include/vmusb.h \ 
           include/DeviceVME.h \
           include/High-Level-functions_VME.h \
           include/FADCConstants_V1729a.h \
           include/const.h
SOURCES += src/caseFunctions.cc \
           src/console.cpp \
           src/fpga.cpp \ 
           src/gui.cpp \
           src/pc.cpp \
           src/timepix.cpp \
           src/tos.cpp \
           src/waitconditions.cpp \
           src/HV_FADC_Obj.cc \
           src/DeviceVME.cc \
           src/V1729a_VME.cc \
           src/vmecontroller.cpp \
           src/vmusb.cpp \
           src/vmemodule.cpp \
           src/High-Level-functions_VME.cc

LIBS += -Wl,--no-as-needed \
        -Wl,--rpath=/usr/local/lib \
        -Wl,--rpath=/home/schmidt/HV_FADC/VME/lib \
        -Wl,--rpath=/home/schmidt/HV_FADC/FadcVMEReadout/lib \
        -Wl,--rpath=/usr/lib \
        #-lMyVME \
        #-lMyV1729a


#external libs
QMAKE_LIBDIR += /home/schmidt/HV_FADC/VME/lib \
                /home/schmidt/HV_FADC/FadcVMEReadout/lib

#external headers
INCLUDEPATH += /usr/include/qt4/QtCore \
               /home/schmidt/HV_FADC/VME/src \
               /home/schmidt/HV_FADC/VME/lib \
               /home/schmidt/HV_FADC/FadcVMEReadout/src \
               /home/schmidt/HV_FADC/FadcVMEReadout/lib \

