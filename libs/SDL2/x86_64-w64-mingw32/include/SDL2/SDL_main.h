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

#ifndef SDL_main_h_
#define SDL_main_h_

#include "SDL_stdinc.h"

   
                    
  
                                                                  
   

#ifndef SDL_MAIN_HANDLED
#if defined(__WIN32__)
                                                                              
                                       

                                                                     
   
#define SDL_MAIN_AVAILABLE

#elif defined(__WINRT__)
                                                                           
                                                         

                                                                         
                                                 
                                                                               
                                                                            
                                                                        
  
#define SDL_MAIN_NEEDED

#elif defined(__GDK__)
                                                                          

                                                                         
                                                                               
                                                         
  
#define SDL_MAIN_NEEDED

#elif defined(__IPHONEOS__)
                                                                           
                                           

                                                                           
                                                                          
                                                                            

                                                               
   
#define SDL_MAIN_NEEDED

#elif defined(__ANDROID__)
                                                                       
                             

                                                                       
   
#define SDL_MAIN_NEEDED

 
#define SDLMAIN_DECLSPEC    DECLSPEC

#elif defined(__NACL__)
                                                                     
                                                                   
                                   
                                                                   
                                                     
  
#define SDL_MAIN_NEEDED

#elif defined(__PSP__)
                                                                 
                                                                      
                

                                                                
   
#define SDL_MAIN_AVAILABLE

#elif defined(__PS2__)
#define SDL_MAIN_AVAILABLE

#define SDL_PS2_SKIP_IOP_RESET() \
   void reset_IOP(); \
   void reset_IOP() {}

#elif defined(__3DS__)
  
                                                                
              

                                                               
  
#define SDL_MAIN_AVAILABLE

#endif
#endif  

#ifndef SDLMAIN_DECLSPEC
#define SDLMAIN_DECLSPEC
#endif

   
                    
  
                                                                    
                                     
         
                      
              
          
                                    
     
     
            
   

#if defined(SDL_MAIN_NEEDED) || defined(SDL_MAIN_AVAILABLE)
#define main    SDL_main
#endif

#include "begin_code.h"
#ifdef __cplusplus
extern "C" {
#endif

   
                                                       
   
typedef int (*SDL_main_func)(int argc, char *argv[]);
extern SDLMAIN_DECLSPEC int SDL_main(int argc, char *argv[]);


   
                                                                         
         
  
                                                                              
                                                                          
                                                                        
                   
  
                                                     
  
               
   
extern DECLSPEC void SDLCALL SDL_SetMainReady(void);

#if defined(__WIN32__) || defined(__GDK__)

   
                                               
  
                                                                           
                                                                        
                                                                             
                                                                            
                          
  
                                                                            
                                                      
  
                                                                     
                                                                  
                                                                             
                                                                              
                                       
                                                                           
                                                         
                                                                       
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_RegisterApp(const char *name, Uint32 style, void *hInst);

   
                                                                  
  
                                                             
  
                                                                            
                                                        
  
                                                                              
                                                                             
                                                                              
                                       
  
                                                     
   
extern DECLSPEC void SDLCALL SDL_UnregisterApp(void);

#endif  


#ifdef __WINRT__

   
                                                  
  
                                                                     
                                                          
                                                                          
                                            
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_WinRTRunApp(SDL_main_func mainFunction, void * reserved);

#endif  

#if defined(__IPHONEOS__)

   
                                               
  
                                                                        
                                                                        
                                                                     
                                             
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_UIKitRunApp(int argc, char *argv[], SDL_main_func mainFunction);

#endif  

#ifdef __GDK__

   
                                                
  
                                                                     
                                                          
                                                                          
                                            
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_GDKRunApp(SDL_main_func mainFunction, void *reserved);

   
                                                             
  
                                                      
   
extern DECLSPEC void SDLCALL SDL_GDKSuspendComplete(void);

#endif  

#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
