######################################################################
# Automatically generated by qmake (2.01a) Do. Okt 14 10:55:30 2010
######################################################################


TEMPLATE = app
TARGET = TOS
DEPENDPATH += . src
INCLUDEPATH += . include
QMAKE_CXXFLAGS += -std=c++14 -o0 -g -Wall -W -pedantic -Wl,--no-as-needed
OBJECTS_DIR = output/tmp
MOC_DIR = output/tmp
DESTDIR = output/bin
DEFINES += "PERFORMANCE=0"
DEFINES += "DEBUG=0"

# Input
HEADERS += include/caseHeader.h \ 
           include/console.hpp \ 
           include/fpga.hpp \ 
           include/pc.hpp \ 
           include/timepix.hpp \
           include/waitconditions.hpp \
           include/V1729a_VME.h \
           include/V1729a.h \
           include/vmemodule.h \
           include/vmecontroller.h \
           include/vmusb.h \ 
           include/DeviceVME.h \
           include/High-Level-functions_VME.h \
           include/FADCConstants_V1729a.h \
           include/const.h \
           include/tosCommandCompletion.hpp \
           include/networkWrapper.hpp \
           include/hvFadcManager.hpp \
           include/hvInterface/hvModule.hpp \
           include/hvInterface/hvChannel.hpp \
           include/hvInterface/hvFlexGroup.hpp \
           include/hvInterface/hvSetGroup.hpp \
           include/hvInterface/hvMonitorGroup.hpp \
           include/hvInterface/hvStatusGroup.hpp \
           include/frame.hpp \
           include/mcp2210/hidapi.h \
           include/mcp2210/mcp2210.h \
           include/mcp2210/temp_auslese.hpp \
           include/mcp2210/temp_defaults.hpp \
           include/mcp2210/temp_helpers.hpp \
           include/helper_functions.hpp 
           

SOURCES += src/caseFunctions.cc \
           src/console.cpp \
           src/fpga.cpp \ 
           src/pc.cpp \
           src/timepix.cpp \
           src/tos.cpp \
           src/waitconditions.cpp \
           src/DeviceVME.cc \
           src/V1729a_VME.cc \
           src/vmecontroller.cpp \
           src/vmusb.cpp \
           src/vmemodule.cpp \
           src/High-Level-functions_VME.cc \
           src/tosCommandCompletion.cpp \
           src/networkWrapper.cpp \
           src/hvFadcManager.cpp \
           src/hvInterface/hvModule.cpp \
           src/hvInterface/hvChannel.cpp \
           src/hvInterface/hvFlexGroup.cpp \
           src/hvInterface/hvSetGroup.cpp \
           src/hvInterface/hvMonitorGroup.cpp \
           src/hvInterface/hvStatusGroup.cpp \
           src/frame.cpp \
           src/mcp2210/mcp2210.cpp \
           src/mcp2210/hid.c \
           src/mcp2210/temp_auslese.cpp \
           src/mcp2210/temp_defaults.cpp \
           src/mcp2210/temp_helpers.cpp \
           src/helper_functions.cpp
           

LIBS += -Wl,--no-as-needed \
        -Wl,--rpath=/usr/local/lib \
        -Wl,--rpath=/usr/lib \
        -lboost_system -lboost_filesystem \
        -lreadline \
        -ludev


# external QT headers
INCLUDEPATH += /usr/include/qt4/QtCore 

 # documentation target
dox.target = doc
dox.commands = doxygen doc/TOS.doxyfile;
dox.CONFIG = phony
dox.depends =

QMAKE_EXTRA_TARGETS += dox

# win32 and unix variables should be decideded automatically by qmake!
# just calling qmake-qt4 TOS.pro should suffice
windows|win32{
    message(Generating Makefile for windows)

    # for windows add readline library, which needs to be put into 
    # include folder
    HEADERS += external/readline/readline/readline.h

    LIBS += -lws2_32 -Wl,-Bdynamic -lreadline


    QMAKE_LFLAGS_WINDOWS += -Wl,--stack,10000000
    QMAKE_LFLAGS    += -static -static-libgcc
    CONFIG += staticlib
    INCLUDEPATH += $$PWD/../../../../Qt/Tools/mingw491_32/i686-w64-mingw32/lib
    DEPENDPATH += $$PWD/../../../../Qt/Tools/mingw491_32/i686-w64-mingw32/lib
    INCLUDEPATH += -I"C:\Program Files (x86)\GnuWin32\lib" \
                    C:/local/boost_1_61_0/ \
                    external/ \
                    external/readline/ \
                   -I./external/readline/lib
    DEPENDPATH += "C:\Program Files (x86)\GnuWin32\lib" \
#                  "C:\local\boost_1_61_0\boost" \
# readline5.dll unfortunately still not linked dynamically
                  external/readline/bin
}

linux|unix{
    message(Generating Makefile for linux)
}
