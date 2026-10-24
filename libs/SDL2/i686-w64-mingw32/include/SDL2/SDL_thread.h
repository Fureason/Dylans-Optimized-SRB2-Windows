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

#ifndef SDL_thread_h_
#define SDL_thread_h_

   
                      
  
                                                  
   

#include "SDL_stdinc.h"
#include "SDL_error.h"

 
#include "SDL_atomic.h"
#include "SDL_mutex.h"

#if (defined(__WIN32__) || defined(__GDK__)) && !defined(__WINRT__)
#include <process.h>  
#endif
#if defined(__OS2__)  
#ifndef __EMX__
#include <process.h>
#else
#include <stdlib.h>
#endif
#endif

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

 
struct SDL_Thread;
typedef struct SDL_Thread SDL_Thread;

 
typedef unsigned long SDL_threadID;

 
typedef unsigned int SDL_TLSID;

   
                            
  
                                                                                    
                                                                                   
                                                            
                                                                                    
  
                                                                                               
   
typedef enum {
    SDL_THREAD_PRIORITY_LOW,
    SDL_THREAD_PRIORITY_NORMAL,
    SDL_THREAD_PRIORITY_HIGH,
    SDL_THREAD_PRIORITY_TIME_CRITICAL
} SDL_ThreadPriority;

   
                                             
  
                                                              
                                                                  
   
typedef int (SDLCALL * SDL_ThreadFunction) (void *data);


#if (defined(__WIN32__) || defined(__GDK__)) && !defined(__WINRT__)
   
                      
  
                                                                  
                                                                            
                                                                             
                                                                    
                
  
                                              
  
                                                                         
                                                                           
                                                                               
                                                                  
  
                 
                                                                         
            
   
#define SDL_PASSED_BEGINTHREAD_ENDTHREAD

typedef uintptr_t (__cdecl * pfnSDL_CurrentBeginThread)
                   (void *, unsigned, unsigned (__stdcall *func)(void *),
                    void *  , unsigned, unsigned *  );
typedef void (__cdecl * pfnSDL_CurrentEndThread) (unsigned code);

#ifndef SDL_beginthread
#define SDL_beginthread _beginthreadex
#endif
#ifndef SDL_endthread
#define SDL_endthread _endthreadex
#endif

extern DECLSPEC SDL_Thread *SDLCALL
SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data,
                 pfnSDL_CurrentBeginThread pfnBeginThread,
                 pfnSDL_CurrentEndThread pfnEndThread);

extern DECLSPEC SDL_Thread *SDLCALL
SDL_CreateThreadWithStackSize(SDL_ThreadFunction fn,
                 const char *name, const size_t stacksize, void *data,
                 pfnSDL_CurrentBeginThread pfnBeginThread,
                 pfnSDL_CurrentEndThread pfnEndThread);


#if defined(SDL_CreateThread) && SDL_DYNAMIC_API
#undef SDL_CreateThread
#define SDL_CreateThread(fn, name, data) SDL_CreateThread_REAL(fn, name, data, (pfnSDL_CurrentBeginThread)SDL_beginthread, (pfnSDL_CurrentEndThread)SDL_endthread)
#undef SDL_CreateThreadWithStackSize
#define SDL_CreateThreadWithStackSize(fn, name, stacksize, data) SDL_CreateThreadWithStackSize_REAL(fn, name, stacksize, data, (pfnSDL_CurrentBeginThread)SDL_beginthread, (pfnSDL_CurrentEndThread)SDL_endthread)
#else
#define SDL_CreateThread(fn, name, data) SDL_CreateThread(fn, name, data, (pfnSDL_CurrentBeginThread)SDL_beginthread, (pfnSDL_CurrentEndThread)SDL_endthread)
#define SDL_CreateThreadWithStackSize(fn, name, stacksize, data) SDL_CreateThreadWithStackSize(fn, name, stacksize, data, (pfnSDL_CurrentBeginThread)SDL_beginthread, (pfnSDL_CurrentEndThread)SDL_endthread)
#endif

#elif defined(__OS2__)
  
                                                     
                                                      
   
#define SDL_PASSED_BEGINTHREAD_ENDTHREAD

typedef int (*pfnSDL_CurrentBeginThread)(void (*func)(void *), void *, unsigned, void *  );
typedef void (*pfnSDL_CurrentEndThread)(void);

#ifndef SDL_beginthread
#define SDL_beginthread _beginthread
#endif
#ifndef SDL_endthread
#define SDL_endthread _endthread
#endif

extern DECLSPEC SDL_Thread *SDLCALL
SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data,
                 pfnSDL_CurrentBeginThread pfnBeginThread,
                 pfnSDL_CurrentEndThread pfnEndThread);
extern DECLSPEC SDL_Thread *SDLCALL
SDL_CreateThreadWithStackSize(SDL_ThreadFunction fn, const char *name, const size_t stacksize, void *data,
                 pfnSDL_CurrentBeginThread pfnBeginThread,
                 pfnSDL_CurrentEndThread pfnEndThread);

#if defined(SDL_CreateThread) && SDL_DYNAMIC_API
#undef SDL_CreateThread
#define SDL_CreateThread(fn, name, data) SDL_CreateThread_REAL(fn, name, data, (pfnSDL_CurrentBeginThread)SDL_beginthread, (pfnSDL_CurrentEndThread)SDL_endthread)
#undef SDL_CreateThreadWithStackSize
#define SDL_CreateThreadWithStackSize(fn, name, stacksize, data) SDL_CreateThreadWithStackSize_REAL(fn, name, stacksize, data, (pfnSDL_CurrentBeginThread)SDL_beginthread, (pfnSDL_CurrentEndThread)SDL_endthread)
#else
#define SDL_CreateThread(fn, name, data) SDL_CreateThread(fn, name, data, (pfnSDL_CurrentBeginThread)SDL_beginthread, (pfnSDL_CurrentEndThread)SDL_endthread)
#define SDL_CreateThreadWithStackSize(fn, name, stacksize, data) SDL_CreateThreadWithStackSize(fn, name, stacksize, data, (pfnSDL_CurrentBeginThread)SDL_beginthread, (pfnSDL_CurrentEndThread)SDL_endthread)
#endif

#else

   
                                                 
  
                                 
  
       
                                                    
      
  
                                                                      
                                     
                                               
                                                                              
                                                                         
                        
  
                                                     
  
                                    
                     
   
extern DECLSPEC SDL_Thread *SDLCALL
SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data);

   
                                                  
  
                                                                             
                                              
  
                                                                             
                                                                            
                                                                           
                                                                              
                                                                          
                                                                       
                                                                              
  
                                                                            
  
                                                                            
                                                                           
                       
  
                                                                             
                                                                         
                                                                             
                                                                         
                                                                              
                                    
  
                                                                           
                                                                          
            
  
                                                                      
                                     
                                                                             
                                               
                                                                              
                                                                         
                        
  
                                                     
  
                     
   
extern DECLSPEC SDL_Thread *SDLCALL
SDL_CreateThreadWithStackSize(SDL_ThreadFunction fn, const char *name, const size_t stacksize, void *data);

#endif

   
                                                                 
  
                                                                            
                                                                
  
                                    
                                                                           
                                           
  
                                                     
  
                       
   
extern DECLSPEC const char *SDLCALL SDL_GetThreadName(SDL_Thread *thread);

   
                                                    
  
                                                                            
                                                                           
                             
  
                                                                         
          
  
                                         
  
                                                     
  
                      
   
extern DECLSPEC SDL_threadID SDLCALL SDL_ThreadID(void);

   
                                                      
  
                                                                            
                                                                           
                             
  
                                    
                                                                              
                             
  
                                                     
  
                   
   
extern DECLSPEC SDL_threadID SDLCALL SDL_GetThreadID(SDL_Thread * thread);

   
                                           
  
                                                                             
                                                                              
                                                          
  
                                                
                                                                  
                                                
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_SetThreadPriority(SDL_ThreadPriority priority);

   
                               
  
                                                                            
                                                            
  
                                                                          
                                                                            
                                                              
  
                                                                              
                                     
  
                                                               
                                                                             
                         
  
                                                                    
  
                                                                          
             
  
                                                                  
                                                                 
                                                                           
                                                                         
                                         
  
                                                     
  
                       
                       
   
extern DECLSPEC void SDLCALL SDL_WaitThread(SDL_Thread * thread, int *status);

   
                                                      
  
                                                                        
                                                                          
                                                                            
                                                                       
  
                                                                          
                                                                       
  
                                                                            
                                                                          
                                                                        
                                                                          
                         
  
                                                                            
                                                                              
                                                                        
  
                                                                            
                                                                            
  
                                                           
  
                                                                  
                                                                 
  
                                                     
  
                       
                     
   
extern DECLSPEC void SDLCALL SDL_DetachThread(SDL_Thread * thread);

   
                                          
  
                                                                         
                                          
  
                                                                            
  
                                                     
  
                 
                 
   
extern DECLSPEC SDL_TLSID SDLCALL SDL_TLSCreate(void);

   
                                                                            
  
                                        
                                                                              
                                                                            
  
                                                     
  
                    
                 
   
extern DECLSPEC void * SDLCALL SDL_TLSGet(SDL_TLSID id);

   
                                                                            
  
                                              
  
       
                               
      
  
                                                                             
  
                                        
                                                                         
                                                                         
                          
                                                                  
                                                
  
                                                     
  
                    
                 
   
extern DECLSPEC int SDLCALL SDL_TLSSet(SDL_TLSID id, const void *value, void (SDLCALL *destructor)(void*));

   
                                        
  
                                                      
   
extern DECLSPEC void SDLCALL SDL_TLSCleanup(void);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
