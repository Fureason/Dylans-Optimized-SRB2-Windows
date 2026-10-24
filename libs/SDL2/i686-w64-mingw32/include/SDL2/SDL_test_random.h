/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

   
                           
  
                                        
  
                                                                           
   

  

                                                                  
                                            

                                                                 
                   

  

#ifndef SDL_test_random_h_
#define SDL_test_random_h_

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

 

  
                                                           
   
#define SDLTest_RandomInt(c)        ((int)SDLTest_Random(c))

  
                                                           
   
  typedef struct {
    unsigned int a;
    unsigned int x;
    unsigned int c;
    unsigned int ah;
    unsigned int al;
  } SDLTest_RandomContext;


 

   
                                                                
  
                                                                       
                                                            
  
                                                      
                                                              
                                                              
  
   
 void SDLTest_RandomInit(SDLTest_RandomContext * rndContext, unsigned int xi,
                  unsigned int ci);

   
                                                                           
  
                                                      
  
   
 void SDLTest_RandomInitTime(SDLTest_RandomContext *rndContext);


   
                                                                           
  
                                                                      
                               
  
                                                      
  
                                                     
  
   
 unsigned int SDLTest_Random(SDLTest_RandomContext *rndContext);


 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
