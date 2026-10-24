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

   
                   
  
                                                       
  
                                                                          
  
                                                     
  
                                                        
                                    
                           
                                              
   

#ifndef SDL_log_h_
#define SDL_log_h_

#include "SDL_stdinc.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif


   
                                                                
  
                                                                     
   
#define SDL_MAX_LOG_MESSAGE 4096

   
                                        
  
                                                                     
                                                                     
                                                                    
                   
   
typedef enum
{
    SDL_LOG_CATEGORY_APPLICATION,
    SDL_LOG_CATEGORY_ERROR,
    SDL_LOG_CATEGORY_ASSERT,
    SDL_LOG_CATEGORY_SYSTEM,
    SDL_LOG_CATEGORY_AUDIO,
    SDL_LOG_CATEGORY_VIDEO,
    SDL_LOG_CATEGORY_RENDER,
    SDL_LOG_CATEGORY_INPUT,
    SDL_LOG_CATEGORY_TEST,

     
    SDL_LOG_CATEGORY_RESERVED1,
    SDL_LOG_CATEGORY_RESERVED2,
    SDL_LOG_CATEGORY_RESERVED3,
    SDL_LOG_CATEGORY_RESERVED4,
    SDL_LOG_CATEGORY_RESERVED5,
    SDL_LOG_CATEGORY_RESERVED6,
    SDL_LOG_CATEGORY_RESERVED7,
    SDL_LOG_CATEGORY_RESERVED8,
    SDL_LOG_CATEGORY_RESERVED9,
    SDL_LOG_CATEGORY_RESERVED10,

                                                              
             
                                                             
                                   
                                   
              
         
       
    SDL_LOG_CATEGORY_CUSTOM
} SDL_LogCategory;

   
                                        
   
typedef enum
{
    SDL_LOG_PRIORITY_VERBOSE = 1,
    SDL_LOG_PRIORITY_DEBUG,
    SDL_LOG_PRIORITY_INFO,
    SDL_LOG_PRIORITY_WARN,
    SDL_LOG_PRIORITY_ERROR,
    SDL_LOG_PRIORITY_CRITICAL,
    SDL_NUM_LOG_PRIORITIES
} SDL_LogPriority;


   
                                          
  
                                                
  
                                                     
  
                         
   
extern DECLSPEC void SDLCALL SDL_LogSetAllPriority(SDL_LogPriority priority);

   
                                                 
  
                                                       
                                                
  
                                                     
  
                         
                            
   
extern DECLSPEC void SDLCALL SDL_LogSetPriority(int category,
                                                SDL_LogPriority priority);

   
                                                 
  
                                        
                                                          
  
                                                     
  
                         
   
extern DECLSPEC SDL_LogPriority SDLCALL SDL_LogGetPriority(int category);

   
                                   
  
                                
  
                                                     
  
                            
                         
   
extern DECLSPEC void SDLCALL SDL_LogResetPriorities(void);

   
                                                                             
  
                                                        
  
                                                                             
                 
  
                                                     
  
                      
                   
                   
                  
                     
                      
                     
                  
   
extern DECLSPEC void SDLCALL SDL_Log(SDL_PRINTF_FORMAT_STRING const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(1);

   
                                               
  
                                              
                                                    
                                                                            
                    
  
                                                     
  
              
                      
                   
                   
                  
                     
                      
                  
   
extern DECLSPEC void SDLCALL SDL_LogVerbose(int category, SDL_PRINTF_FORMAT_STRING const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);

   
                                             
  
                                              
                                                    
                                                                            
                    
  
                                                     
  
              
                      
                   
                  
                     
                      
                     
                  
   
extern DECLSPEC void SDLCALL SDL_LogDebug(int category, SDL_PRINTF_FORMAT_STRING const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);

   
                                            
  
                                              
                                                    
                                                                            
                    
  
                                                     
  
              
                      
                   
                   
                     
                      
                     
                  
   
extern DECLSPEC void SDLCALL SDL_LogInfo(int category, SDL_PRINTF_FORMAT_STRING const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);

   
                                            
  
                                              
                                                    
                                                                            
                    
  
                                                     
  
              
                      
                   
                   
                  
                     
                      
                     
   
extern DECLSPEC void SDLCALL SDL_LogWarn(int category, SDL_PRINTF_FORMAT_STRING const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);

   
                                             
  
                                              
                                                    
                                                                            
                    
  
                                                     
  
              
                      
                   
                  
                     
                      
                     
                  
   
extern DECLSPEC void SDLCALL SDL_LogError(int category, SDL_PRINTF_FORMAT_STRING const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);

   
                                                
  
                                              
                                                    
                                                                            
                    
  
                                                     
  
              
                   
                   
                  
                     
                      
                     
                  
   
extern DECLSPEC void SDLCALL SDL_LogCritical(int category, SDL_PRINTF_FORMAT_STRING const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);

   
                                                          
  
                                              
                                              
                                                    
                                                                            
                    
  
                                                     
  
              
                      
                   
                   
                  
                      
                     
                  
   
extern DECLSPEC void SDLCALL SDL_LogMessage(int category,
                                            SDL_LogPriority priority,
                                            SDL_PRINTF_FORMAT_STRING const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(3);

   
                                                          
  
                                              
                                              
                                                    
                                     
  
                                                     
  
              
                      
                   
                   
                  
                     
                     
                  
   
extern DECLSPEC void SDLCALL SDL_LogMessageV(int category,
                                             SDL_LogPriority priority,
                                             const char *fmt, va_list ap);

   
                                                      
  
                                                                      
  
                                                                              
                                              
                                              
                                          
   
typedef void (SDLCALL *SDL_LogOutputFunction)(void *userdata, int category, SDL_LogPriority priority, const char *message);

   
                                       
  
                                                                          
                           
                                                                         
                             
  
                                                     
  
                               
   
extern DECLSPEC void SDLCALL SDL_LogGetOutputFunction(SDL_LogOutputFunction *callback, void **userdata);

   
                                                                
  
                                                                          
                                                         
  
                                                     
  
                               
   
extern DECLSPEC void SDLCALL SDL_LogSetOutputFunction(SDL_LogOutputFunction callback, void *userdata);


 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
