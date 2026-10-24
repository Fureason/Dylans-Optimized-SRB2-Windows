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

   
                     
  
                                          
   

#ifndef SDL_error_h_
#define SDL_error_h_

#include "SDL_stdinc.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

 


   
                                                    
  
                                                                              
  
                                                                              
                                         
  
       
                    
                                                                        
    
      
  
                                                    
                                                                             
                 
                      
  
                                                     
  
                     
                   
   
extern DECLSPEC int SDLCALL SDL_SetError(SDL_PRINTF_FORMAT_STRING const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(1);

   
                                                                       
          
  
                                                                             
                                   
  
                                                                             
                                                                              
                                                                         
                                                                            
                                               
  
                                                                             
                                                                        
                  
  
                                                                          
                                                          
  
                                                                           
               
  
                                                                              
                                                                              
                                                                             
                                                                          
                                                                    
                                              
  
                                                     
  
                     
                   
   
extern DECLSPEC const char *SDLCALL SDL_GetError(void);

   
                                                                  
  
                                                                              
                                                         
  
                                                                              
                                   
                                                                          
                                                            
  
                                                      
  
                   
   
extern DECLSPEC char * SDLCALL SDL_GetErrorMsg(char *errstr, int maxlen);

   
                                                    
  
                                                     
  
                   
                   
   
extern DECLSPEC void SDLCALL SDL_ClearError(void);

   
                                  
  
             
                                                       
   
 
#define SDL_OutOfMemory()   SDL_Error(SDL_ENOMEM)
#define SDL_Unsupported()   SDL_Error(SDL_UNSUPPORTED)
#define SDL_InvalidParamError(param)    SDL_SetError("Parameter '%s' is invalid", (param))
typedef enum
{
    SDL_ENOMEM,
    SDL_EFREAD,
    SDL_EFWRITE,
    SDL_EFSEEK,
    SDL_UNSUPPORTED,
    SDL_LASTERROR
} SDL_errorcode;
 
extern DECLSPEC int SDLCALL SDL_Error(SDL_errorcode code);
  

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
