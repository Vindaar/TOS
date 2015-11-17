/**********************************************************************/
/*                                                            gui.hpp */
/*  TOS - Timepix Operating Software                                  */
/*                                                                    */
/*                                                         08.07.2009 */
/*                                                    Christian Kahra */
/*                                     chrkahra@students.uni-mainz.de */
/*                                        Institut fuer Physik - ETAP */
/*                              Johannes-Gutenberg Universitaet Mainz */
/**********************************************************************/

#ifndef _GUI_HPP
#define _GUI_HPP 1

#include "header.hpp"
//#include "pc.hpp"


class GUI{
	public:
		GUI();
		static void WrapperToDACScanLive(void* PointerToObject, char dac, int i, int value);
};


#endif
