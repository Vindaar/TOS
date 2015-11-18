#############################################################################
# Makefile for building: bin/TOS
# Generated by qmake (2.01a) (Qt 4.8.6) on: Di. Nov 17 15:48:08 2015
# Project:  TOS.pro
# Template: app
# Command: /usr/bin/qmake-qt4 -o Makefile TOS.pro
#############################################################################

####### Compiler, tools and options

CC            = gcc
CXX           = g++
DEFINES       = -DQT_NO_DEBUG -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED
CFLAGS        = -m64 -pipe -O2 -Wall -W -D_REENTRANT $(DEFINES)
CXXFLAGS      = -m64 -pipe -std=c++11 -o0 -g -Wall -W -pedantic -O2 -Wall -W -D_REENTRANT $(DEFINES)
INCPATH       = -I/usr/share/qt4/mkspecs/linux-g++-64 -I. -I/usr/include/qt4/QtCore -I/usr/include/qt4/QtGui -I/usr/include/qt4 -I. -Iinclude -I../VME/src -I../VME/lib -I../FadcVMEReadout/src -I../FadcVMEReadout/lib -Itmp
LINK          = g++
LFLAGS        = -m64 -Wl,-O1
LIBS          = $(SUBLIBS)  -L/home/schmidt/HV_FADC/VME/lib -L/home/schmidt/HV_FADC/FadcVMEReadout/lib -L/usr/lib/x86_64-linux-gnu -Wl,--no-as-needed -Wl,--rpath=/usr/local/lib -Wl,--rpath=/home/schmidt/HV_FADC/VME/lib -Wl,--rpath=/home/schmidt/HV_FADC/FadcVMEReadout/lib -Wl,--rpath=/usr/lib -lQtGui -lQtCore -lpthread 
AR            = ar cqs
RANLIB        = 
QMAKE         = /usr/bin/qmake-qt4
TAR           = tar -cf
COMPRESS      = gzip -9f
COPY          = cp -f
SED           = sed
COPY_FILE     = $(COPY)
COPY_DIR      = $(COPY) -r
STRIP         = strip
INSTALL_FILE  = install -m 644 -p
INSTALL_DIR   = $(COPY_DIR)
INSTALL_PROGRAM = install -m 755 -p
DEL_FILE      = rm -f
SYMLINK       = ln -f -s
DEL_DIR       = rmdir
MOVE          = mv -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p

####### Output directory

OBJECTS_DIR   = tmp/

####### Files

SOURCES       = src/caseFunctions.cc \
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
OBJECTS       = tmp/caseFunctions.o \
		tmp/console.o \
		tmp/fpga.o \
		tmp/gui.o \
		tmp/pc.o \
		tmp/timepix.o \
		tmp/tos.o \
		tmp/waitconditions.o \
		tmp/HV_FADC_Obj.o \
		tmp/DeviceVME.o \
		tmp/V1729a_VME.o \
		tmp/vmecontroller.o \
		tmp/vmusb.o \
		tmp/vmemodule.o \
		tmp/High-Level-functions_VME.o
DIST          = /usr/share/qt4/mkspecs/common/unix.conf \
		/usr/share/qt4/mkspecs/common/linux.conf \
		/usr/share/qt4/mkspecs/common/gcc-base.conf \
		/usr/share/qt4/mkspecs/common/gcc-base-unix.conf \
		/usr/share/qt4/mkspecs/common/g++-base.conf \
		/usr/share/qt4/mkspecs/common/g++-unix.conf \
		/usr/share/qt4/mkspecs/qconfig.pri \
		/usr/share/qt4/mkspecs/features/qt_functions.prf \
		/usr/share/qt4/mkspecs/features/qt_config.prf \
		/usr/share/qt4/mkspecs/features/exclusive_builds.prf \
		/usr/share/qt4/mkspecs/features/default_pre.prf \
		/usr/share/qt4/mkspecs/features/release.prf \
		/usr/share/qt4/mkspecs/features/default_post.prf \
		/usr/share/qt4/mkspecs/features/shared.prf \
		/usr/share/qt4/mkspecs/features/unix/gdb_dwarf_index.prf \
		/usr/share/qt4/mkspecs/features/warn_on.prf \
		/usr/share/qt4/mkspecs/features/qt.prf \
		/usr/share/qt4/mkspecs/features/unix/thread.prf \
		/usr/share/qt4/mkspecs/features/moc.prf \
		/usr/share/qt4/mkspecs/features/resources.prf \
		/usr/share/qt4/mkspecs/features/uic.prf \
		/usr/share/qt4/mkspecs/features/yacc.prf \
		/usr/share/qt4/mkspecs/features/lex.prf \
		/usr/share/qt4/mkspecs/features/include_source_dir.prf \
		TOS.pro
QMAKE_TARGET  = TOS
DESTDIR       = bin/
TARGET        = bin/TOS

first: all
####### Implicit rules

.SUFFIXES: .o .c .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"

####### Build rules

all: Makefile $(TARGET)

$(TARGET):  $(OBJECTS)  
	@$(CHK_DIR_EXISTS) bin/ || $(MKDIR) bin/ 
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)

Makefile: TOS.pro  /usr/share/qt4/mkspecs/linux-g++-64/qmake.conf /usr/share/qt4/mkspecs/common/unix.conf \
		/usr/share/qt4/mkspecs/common/linux.conf \
		/usr/share/qt4/mkspecs/common/gcc-base.conf \
		/usr/share/qt4/mkspecs/common/gcc-base-unix.conf \
		/usr/share/qt4/mkspecs/common/g++-base.conf \
		/usr/share/qt4/mkspecs/common/g++-unix.conf \
		/usr/share/qt4/mkspecs/qconfig.pri \
		/usr/share/qt4/mkspecs/features/qt_functions.prf \
		/usr/share/qt4/mkspecs/features/qt_config.prf \
		/usr/share/qt4/mkspecs/features/exclusive_builds.prf \
		/usr/share/qt4/mkspecs/features/default_pre.prf \
		/usr/share/qt4/mkspecs/features/release.prf \
		/usr/share/qt4/mkspecs/features/default_post.prf \
		/usr/share/qt4/mkspecs/features/shared.prf \
		/usr/share/qt4/mkspecs/features/unix/gdb_dwarf_index.prf \
		/usr/share/qt4/mkspecs/features/warn_on.prf \
		/usr/share/qt4/mkspecs/features/qt.prf \
		/usr/share/qt4/mkspecs/features/unix/thread.prf \
		/usr/share/qt4/mkspecs/features/moc.prf \
		/usr/share/qt4/mkspecs/features/resources.prf \
		/usr/share/qt4/mkspecs/features/uic.prf \
		/usr/share/qt4/mkspecs/features/yacc.prf \
		/usr/share/qt4/mkspecs/features/lex.prf \
		/usr/share/qt4/mkspecs/features/include_source_dir.prf \
		/usr/lib/x86_64-linux-gnu/libQtGui.prl \
		/usr/lib/x86_64-linux-gnu/libQtCore.prl
	$(QMAKE) -o Makefile TOS.pro
/usr/share/qt4/mkspecs/common/unix.conf:
/usr/share/qt4/mkspecs/common/linux.conf:
/usr/share/qt4/mkspecs/common/gcc-base.conf:
/usr/share/qt4/mkspecs/common/gcc-base-unix.conf:
/usr/share/qt4/mkspecs/common/g++-base.conf:
/usr/share/qt4/mkspecs/common/g++-unix.conf:
/usr/share/qt4/mkspecs/qconfig.pri:
/usr/share/qt4/mkspecs/features/qt_functions.prf:
/usr/share/qt4/mkspecs/features/qt_config.prf:
/usr/share/qt4/mkspecs/features/exclusive_builds.prf:
/usr/share/qt4/mkspecs/features/default_pre.prf:
/usr/share/qt4/mkspecs/features/release.prf:
/usr/share/qt4/mkspecs/features/default_post.prf:
/usr/share/qt4/mkspecs/features/shared.prf:
/usr/share/qt4/mkspecs/features/unix/gdb_dwarf_index.prf:
/usr/share/qt4/mkspecs/features/warn_on.prf:
/usr/share/qt4/mkspecs/features/qt.prf:
/usr/share/qt4/mkspecs/features/unix/thread.prf:
/usr/share/qt4/mkspecs/features/moc.prf:
/usr/share/qt4/mkspecs/features/resources.prf:
/usr/share/qt4/mkspecs/features/uic.prf:
/usr/share/qt4/mkspecs/features/yacc.prf:
/usr/share/qt4/mkspecs/features/lex.prf:
/usr/share/qt4/mkspecs/features/include_source_dir.prf:
/usr/lib/x86_64-linux-gnu/libQtGui.prl:
/usr/lib/x86_64-linux-gnu/libQtCore.prl:
qmake:  FORCE
	@$(QMAKE) -o Makefile TOS.pro

dist: 
	@$(CHK_DIR_EXISTS) tmp/TOS1.0.0 || $(MKDIR) tmp/TOS1.0.0 
	$(COPY_FILE) --parents $(SOURCES) $(DIST) tmp/TOS1.0.0/ && $(COPY_FILE) --parents include/caseHeader.h include/console.hpp include/fpga.hpp include/gui.hpp include/header.hpp include/pc.hpp include/timepix.hpp include/waitconditions.hpp include/V1729a_VME.h include/HV_FADC_Obj.h include/vmemodule.h include/vmecontroller.h include/vmusb.h include/DeviceVME.h include/High-Level-functions_VME.h include/FADCConstants_V1729a.h include/const.h tmp/TOS1.0.0/ && $(COPY_FILE) --parents src/caseFunctions.cc src/console.cpp src/fpga.cpp src/gui.cpp src/pc.cpp src/timepix.cpp src/tos.cpp src/waitconditions.cpp src/HV_FADC_Obj.cc src/DeviceVME.cc src/V1729a_VME.cc src/vmecontroller.cpp src/vmusb.cpp src/vmemodule.cpp src/High-Level-functions_VME.cc tmp/TOS1.0.0/ && (cd `dirname tmp/TOS1.0.0` && $(TAR) TOS1.0.0.tar TOS1.0.0 && $(COMPRESS) TOS1.0.0.tar) && $(MOVE) `dirname tmp/TOS1.0.0`/TOS1.0.0.tar.gz . && $(DEL_FILE) -r tmp/TOS1.0.0


clean:compiler_clean 
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core


####### Sub-libraries

distclean: clean
	-$(DEL_FILE) $(TARGET) 
	-$(DEL_FILE) Makefile


check: first

mocclean: compiler_moc_header_clean compiler_moc_source_clean

mocables: compiler_moc_header_make_all compiler_moc_source_make_all

compiler_moc_header_make_all:
compiler_moc_header_clean:
compiler_rcc_make_all:
compiler_rcc_clean:
compiler_image_collection_make_all: qmake_image_collection.cpp
compiler_image_collection_clean:
	-$(DEL_FILE) qmake_image_collection.cpp
compiler_moc_source_make_all:
compiler_moc_source_clean:
compiler_uic_make_all:
compiler_uic_clean:
compiler_yacc_decl_make_all:
compiler_yacc_decl_clean:
compiler_yacc_impl_make_all:
compiler_yacc_impl_clean:
compiler_lex_make_all:
compiler_lex_clean:
compiler_clean: 

####### Compile

tmp/caseFunctions.o: src/caseFunctions.cc 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/caseFunctions.o src/caseFunctions.cc

tmp/console.o: src/console.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/console.o src/console.cpp

tmp/fpga.o: src/fpga.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/fpga.o src/fpga.cpp

tmp/gui.o: src/gui.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/gui.o src/gui.cpp

tmp/pc.o: src/pc.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/pc.o src/pc.cpp

tmp/timepix.o: src/timepix.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/timepix.o src/timepix.cpp

tmp/tos.o: src/tos.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tos.o src/tos.cpp

tmp/waitconditions.o: src/waitconditions.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/waitconditions.o src/waitconditions.cpp

tmp/HV_FADC_Obj.o: src/HV_FADC_Obj.cc 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/HV_FADC_Obj.o src/HV_FADC_Obj.cc

tmp/DeviceVME.o: src/DeviceVME.cc 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/DeviceVME.o src/DeviceVME.cc

tmp/V1729a_VME.o: src/V1729a_VME.cc 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/V1729a_VME.o src/V1729a_VME.cc

tmp/vmecontroller.o: src/vmecontroller.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/vmecontroller.o src/vmecontroller.cpp

tmp/vmusb.o: src/vmusb.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/vmusb.o src/vmusb.cpp

tmp/vmemodule.o: src/vmemodule.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/vmemodule.o src/vmemodule.cpp

tmp/High-Level-functions_VME.o: src/High-Level-functions_VME.cc 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/High-Level-functions_VME.o src/High-Level-functions_VME.cc

####### Install

install:   FORCE

uninstall:   FORCE

FORCE:

