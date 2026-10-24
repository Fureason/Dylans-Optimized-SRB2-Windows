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

   
                       
  
                                                
   

#ifndef SDL_gesture_h_
#define SDL_gesture_h_

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_video.h"

#include "SDL_touch.h"


#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

typedef Sint64 SDL_GestureID;

 

   
                                                                              
  
                                                                           
                                                                         
  
                                                                  
                                                                         
  
                                                     
  
                         
   
extern DECLSPEC int SDLCALL SDL_RecordGesture(SDL_TouchID touchId);


   
                                                      
  
                                    
                                                                          
                                                
  
                                                     
  
                              
                             
   
extern DECLSPEC int SDLCALL SDL_SaveAllDollarTemplates(SDL_RWops *dst);

   
                                                   
  
                                
                                    
                                                                      
                        
  
                                                     
  
                              
                                 
   
extern DECLSPEC int SDLCALL SDL_SaveDollarTemplate(SDL_GestureID gestureId,SDL_RWops *dst);


   
                                             
  
                            
                                      
                                                                              
                                                                        
  
                                                     
  
                                 
                             
   
extern DECLSPEC int SDLCALL SDL_LoadDollarTemplates(SDL_TouchID touchId, SDL_RWops *src);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
