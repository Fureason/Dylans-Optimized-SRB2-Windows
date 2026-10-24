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

   
                       
  
                                                
   

#ifndef SDL_version_h_
#define SDL_version_h_

#include "SDL_stdinc.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

   
                                               
  
                                                                   
                                                                  
                                                                      
                                                                      
             
  
                  
                     
   
typedef struct SDL_version
{
    Uint8 major;         
    Uint8 minor;         
    Uint8 patch;         
} SDL_version;

                                                         
  
#define SDL_MAJOR_VERSION   2
#define SDL_MINOR_VERSION   28
#define SDL_PATCHLEVEL      5

   
                                                               
  
                                                                      
                                                                      
                                                                            
                                                                         
                                                                 
                  
  
                                                            
  
                  
                     
   
#define SDL_VERSION(x)                          \
{                                   \
    (x)->major = SDL_MAJOR_VERSION;                 \
    (x)->minor = SDL_MINOR_VERSION;                 \
    (x)->patch = SDL_PATCHLEVEL;                    \
}

 
#if SDL_MAJOR_VERSION < 3
   
                                                              
             
                     
                
  
                                                                    
  
                                                                   
                                                                
                                           
                                                
   
#define SDL_VERSIONNUM(X, Y, Z)                     \
    ((X)*1000 + (Y)*100 + (Z))

   
                                                                 
  
                                                                   
                                                                
                                                
  
                                                               
   
#define SDL_COMPILEDVERSION \
    SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL)
#endif  

   
                                                                         
   
#define SDL_VERSION_ATLEAST(X, Y, Z) \
    ((SDL_MAJOR_VERSION >= X) && \
     (SDL_MAJOR_VERSION > X || SDL_MINOR_VERSION >= Y) && \
     (SDL_MAJOR_VERSION > X || SDL_MINOR_VERSION > Y || SDL_PATCHLEVEL >= Z))

   
                                                              
  
                                                                              
                                                                        
                                                                            
                                            
  
                                                                          
  
                                                                             
  
                                                     
  
                      
   
extern DECLSPEC void SDLCALL SDL_GetVersion(SDL_version * ver);

   
                                                                    
  
                                                                        
                                                                        
                         
  
                                                                           
                                                                            
                                                             
  
                                                                             
                               
  
                                                                           
                                   
  
                                                                            
                                                                  
  
                                                                           
                                   
  
                                                     
  
                     
   
extern DECLSPEC const char *SDLCALL SDL_GetRevision(void);

   
                                 
  
                                                                          
                                                                              
                                                                            
                                                                            
                                                                  
  
                                                                          
          
  
                                                                            
                                      
  
                                                 
  
                                                     
  
                      
   
extern SDL_DEPRECATED DECLSPEC int SDLCALL SDL_GetRevisionNumber(void);


 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
