/**********************************************************************/
/*                                                         header.hpp */
/*  TOS - Timepix Operating Software                                  */
/*                                                                    */
/*                                                         20.07.2009 */
/*                                                    Christian Kahra */
/*                                     chrkahra@students.uni-mainz.de */
/*                                        Institut fuer Physik - ETAP */
/*                              Johannes-Gutenberg Universitaet Mainz */
/**********************************************************************/

//y,x: Matrix as in Timepix manual. Data stream : first bit from pixel (y= 0, x= 255)

//y	255
//	...
//	...
//	...
//	...
//	1
//	0
//	x=0, 1,..., 255


//  Debug-Levels
/*
	DEBUG=1: Status-Meldungen auf der Konsole
	DEBUG=2: Messaging Function-Entries
*/
	#define DEBUG 0


//  Performance
/*
	PERFORMANCE=0: Daten-Datei ist Human-Readable
	PERFORMANCE=1: Daten werden lediglich gedumpt - OHNE LFSR-Konvertierung
*/
	#define PERFORMANCE 0

// Talkativness
/*
	TALKATIVNESS=0: almost now std::cout
	TALKATIVNESS=1: more std::cout
 */

	#define TALKATIVNESS 1

//Header
	#include <iostream>
	#include <fstream>
	#include <sstream>
	#include <string>
	#include <sys/stat.h>
	#include <sys/types.h>
    //#include <sys/socket.h>
	#include <sys/time.h>
    //#include <netinet/in.h>
    //#include <arpa/inet.h>
	#include <unistd.h>
	#include <cstdlib>
	#include <ctime>
	#include <cmath>
	#include <errno.h>
	#include <vector>
	#include <stdio.h>
	#include <iomanip>
	//#include <exception>
// TODO: check which packages are only needed for select(), recv() and send()
//       which are reimplemented with a wrapper in networkWrapper.hpp
#ifdef __WIN32__
# include <winsock2.h>
# include <ws2tcpip.h>
# include <fcntl.h>
# include <io.h>
# include <windows.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif

/*
#ifdef __unix__ || __APPLE__
	#include <unistd.h>
#elif __WIN32__ || _MS_DOS_
	#include <dir.h>
#endif
*/

/*
2009-07-10	Umstieg auf private IP-Adresse 10.93.130.2
2009-07-13	Hinweis auf Nicht-Konvertierung der Daten im Performance=1 Modus
2009-07-20	include: <ctime> fuer Datum-Zeitangaben im Ordnernamen
		Wiedereinfuehrung der Unix/Windows-Weiche
		include: <dir.h>
*/
