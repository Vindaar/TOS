/**********************************************************************/
/*                                                            gui.cpp */
/*  TOS - Timepix Operating Software                                  */
/*                                                                    */
/*                                                         08.07.2009 */
/*                                                    Christian Kahra */
/*                                     chrkahra@students.uni-mainz.de */
/*                                        Institut fuer Physik - ETAP */
/*                              Johannes-Gutenberg Universitaet Mainz */
/**********************************************************************/

#include "gui.hpp"

GUI::GUI(){
#if DEBUG==2
	std::cout<<"Enter GUI::GUI()"<<std::endl;	
#endif
	//
}

void GUI::WrapperToDACScanLive(void* PointerToObject, char dac, int i, int value){
#if DEBUG==2
	std::cout<<"Enter GUI::WrapperToDACScanLive()"<<std::endl;	
#endif
	//
}
