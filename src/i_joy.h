// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __I_JOY_H__
#define __I_JOY_H__

#include "g_input.h"

   
                                                     

                                                        
                                                   
                                                    
                                            
                                                  
  

#define JOYAXISRANGE 1023


#if (JOYBUTTONS > 64)
"JOYBUTTONS is greater than INT64 bits can hold"
#endif

                               

                                                                                   
                                                                
  

struct JoyType_s
{
	                                                     
                                                                         
	INT32 bJoyNeedPoll;
	                                                  
                                                                           
	INT32 bGamepadStyle;

};
typedef struct JoyType_s JoyType_t;
                        
                                      
  

extern JoyType_t Joystick, Joystick2;

#endif  
