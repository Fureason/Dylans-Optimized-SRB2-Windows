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

#ifndef SDL_timer_h_
#define SDL_timer_h_

   
                     
  
                                                
   

#include "SDL_stdinc.h"
#include "SDL_error.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

   
                                                                   
  
                                                               
  
                                                                          
                                                                            
                                                                      
                                                                           
              
  
                                                                            
                                              
  
                                                     
  
                       
   
extern DECLSPEC Uint32 SDLCALL SDL_GetTicks(void);

   
                                                                   
  
                                                                      
                                                                              
                                                                              
                                                             
  
                                                              
  
       
                                                 
                                       
                                               
    
      
  
                                                                            
                                              
  
                                                      
   
extern DECLSPEC Uint64 SDLCALL SDL_GetTicks64(void);

   
                                                                          
  
                                                                      
                                                                           
                                                                            
                
  
                                                                          
           
  
       
                                               
                                                       
                                               
    
      
  
                                                          
                                                           
                                            
   
#define SDL_TICKS_PASSED(A, B)  ((Sint32)((B) - (A)) <= 0)

   
                                                        
  
                                                 
  
                                                                             
                                                    
                                 
  
                                      
  
                                                     
  
                                  
   
extern DECLSPEC Uint64 SDLCALL SDL_GetPerformanceCounter(void);

   
                                                           
  
                                                 
  
                                                     
  
                                
   
extern DECLSPEC Uint64 SDLCALL SDL_GetPerformanceFrequency(void);

   
                                                            
  
                                                                              
                                                                   
              
  
                                                
  
                                                     
   
extern DECLSPEC void SDLCALL SDL_Delay(Uint32 ms);

   
                                                      
  
                                                                         
                                                                        
                                                                    
                                                                         
   
typedef Uint32 (SDLCALL * SDL_TimerCallback) (Uint32 interval, void *param);

   
                                   
   
typedef int SDL_TimerID;

   
                                             
  
                                                                          
  
                                                                          
                                                                             
                                                                             
            
  
                                            
  
                                                                     
                                                                             
                                                                      
             
  
                                                                          
                                                                       
                                          
  
                                                                         
                                                                            
                                     
                                                      
                                                                            
                        
  
                                                     
  
                      
   
extern DECLSPEC SDL_TimerID SDLCALL SDL_AddTimer(Uint32 interval,
                                                 SDL_TimerCallback callback,
                                                 void *param);

   
                                              
  
                                          
                                                                             
                  
  
                                                     
  
                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_RemoveTimer(SDL_TimerID id);


 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
