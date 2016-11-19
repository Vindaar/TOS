# TOS - Timepix Operating Software

The Timepix Operating Software (TOS) is a C++ project which provides the
interface of the Timepix Operating Firmware (TOF) to a PC.

The current version can always be found at <https://gitweb.physik.uni-bonn.de/cgit/ilc/software/TOS.git/>.


## Installation

TOS in its current form has several dependencies, which need to be met.

A not yet complete list of dependencies includes:
```
libusb-dev \
libreadline6 \
libreadline6-dev \
libboost-dev
```
and the 'libxxusb.h', which needs to be installed manually.
Visit <https://wiki.physik.uni-bonn.de/ilc/private/index.php/Wiener_VM-USB> for more information.
Regarding the boost dependency: It is probably enough to install libboost-dev. If not, one needs to install libboost-all-dev after all.

If all dependencies are met,
```
qmake TOS.pro
```
will create the Makefile for the project from the QT project file. Then a simple
```
make
```
should (hopefully) compile successfully.
Linux and Windows are now both supported by a single project file (TOS.pro). Just call qmake and
it will decide by itself, which configuration it has to use!

NOTE:
On my linux machine, I need to use QT4 in order to create the makefile from the TOS.pro file:
qmake-qt4 TOS.pro
