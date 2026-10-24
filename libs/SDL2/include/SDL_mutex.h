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

#ifndef SDL_mutex_h_
#define SDL_mutex_h_

   
                     
  
                                                           
   

#include "SDL_stdinc.h"
#include "SDL_error.h"

 
                                                   
                                                                           
   
#if defined(SDL_THREAD_SAFETY_ANALYSIS) && \
    defined(__clang__) && (!defined(SWIG))
#define SDL_THREAD_ANNOTATION_ATTRIBUTE__(x)   __attribute__((x))
#else
#define SDL_THREAD_ANNOTATION_ATTRIBUTE__(x)    
#endif

#define SDL_CAPABILITY(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(capability(x))

#define SDL_SCOPED_CAPABILITY \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

#define SDL_GUARDED_BY(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define SDL_PT_GUARDED_BY(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))

#define SDL_ACQUIRED_BEFORE(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(x))

#define SDL_ACQUIRED_AFTER(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(x))

#define SDL_REQUIRES(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(x))

#define SDL_REQUIRES_SHARED(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(x))

#define SDL_ACQUIRE(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(x))

#define SDL_ACQUIRE_SHARED(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(x))

#define SDL_RELEASE(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(release_capability(x))

#define SDL_RELEASE_SHARED(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(x))

#define SDL_RELEASE_GENERIC(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(release_generic_capability(x))

#define SDL_TRY_ACQUIRE(x, y) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(x, y))

#define SDL_TRY_ACQUIRE_SHARED(x, y) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(x, y))

#define SDL_EXCLUDES(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(x))

#define SDL_ASSERT_CAPABILITY(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(x))

#define SDL_ASSERT_SHARED_CAPABILITY(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(x))

#define SDL_RETURN_CAPABILITY(x) \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))

#define SDL_NO_THREAD_SAFETY_ANALYSIS \
  SDL_THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)

 


#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

   
                                                                  
                     
   
#define SDL_MUTEX_TIMEDOUT  1

   
                                                                  
   
#define SDL_MUTEX_MAXWAIT   (~(Uint32)0)


   
                         
   
 

 
struct SDL_mutex;
typedef struct SDL_mutex SDL_mutex;

   
                      
  
                                                           
  
                                                                        
                                                                              
  
                             
  
                                                                       
                                                
  
                                                     
  
                       
                    
                       
                      
   
extern DECLSPEC SDL_mutex *SDLCALL SDL_CreateMutex(void);

   
                  
  
                                                                             
                                                                             
                                                                              
  
                                                                             
                                                                              
                                                                      
  
                                 
                             
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_LockMutex(SDL_mutex * mutex) SDL_ACQUIRE(mutex);
#define SDL_mutexP(m)   SDL_LockMutex(m)

   
                                        
  
                                                                           
                                                         
  
                                                                          
                                                                       
  
                                        
                                                                            
                             
  
                                                     
  
                      
                       
                    
                      
   
extern DECLSPEC int SDLCALL SDL_TryLockMutex(SDL_mutex * mutex) SDL_TRY_ACQUIRE(0, mutex);

   
                    
  
                                                                             
                                                                              
                                                                      
  
                                                                           
                                                      
  
                                                                  
  
                                    
                              
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_UnlockMutex(SDL_mutex * mutex) SDL_RELEASE(mutex);
#define SDL_mutexV(m)   SDL_UnlockMutex(m)

   
                                                  
  
                                                                              
                                                                            
                                                                              
                                                                            
                   
  
                                    
  
                                                     
  
                      
                    
                       
                      
   
extern DECLSPEC void SDLCALL SDL_DestroyMutex(SDL_mutex * mutex);

  


   
                             
   
 

 
struct SDL_semaphore;
typedef struct SDL_semaphore SDL_sem;

   
                      
  
                                                                          
                                                                        
                                                                             
                                                                              
                                                                   
  
                                                           
                                                                            
                        
  
                                                     
  
                           
                  
                     
                   
                  
                         
   
extern DECLSPEC SDL_sem *SDLCALL SDL_CreateSemaphore(Uint32 initial_value);

   
                       
  
                                                                       
                 
  
                                      
  
                                                     
  
                          
                  
                     
                   
                  
                         
   
extern DECLSPEC void SDLCALL SDL_DestroySemaphore(SDL_sem * sem);

   
                                                                      
  
                                                                       
                                                                           
                                                                              
                   
  
                                                                              
                                 
  
                                   
                                                                  
                                                
  
                                                     
  
                          
                           
                  
                     
                   
                  
                         
   
extern DECLSPEC int SDLCALL SDL_SemWait(SDL_sem * sem);

   
                                                                       
  
                                                                         
                                                                              
                                                                        
                              
  
                                      
                                                                          
                                                                           
                                 
  
                                                     
  
                          
                           
                  
                   
                  
                         
   
extern DECLSPEC int SDLCALL SDL_SemTryWait(SDL_sem * sem);

   
                                                                      
  
                                                                       
                                                                         
                                                                     
                                                               
  
                                      
                                                            
                                                                             
                                                                              
                                                     
  
                                                     
  
                          
                           
                  
                     
                   
                  
   
extern DECLSPEC int SDLCALL SDL_SemWaitTimeout(SDL_sem *sem, Uint32 timeout);

   
                                                                     
  
                                        
                                                                  
                                                
  
                                                     
  
                          
                           
                     
                   
                  
                         
   
extern DECLSPEC int SDLCALL SDL_SemPost(SDL_sem * sem);

   
                                        
  
                                    
                                               
  
                                                     
  
                          
   
extern DECLSPEC Uint32 SDLCALL SDL_SemValue(SDL_sem * sem);

  


   
                                      
   
 

 
struct SDL_cond;
typedef struct SDL_cond SDL_cond;

   
                               
  
                                                                            
                                 
  
                                                     
  
                        
                     
                   
                          
                      
   
extern DECLSPEC SDL_cond *SDLCALL SDL_CreateCond(void);

   
                                
  
                                                
  
                                                     
  
                        
                     
                   
                          
                     
   
extern DECLSPEC void SDLCALL SDL_DestroyCond(SDL_cond * cond);

   
                                                                         
  
                                               
                                                                  
                                                
  
                                                     
  
                        
                   
                          
                     
                      
   
extern DECLSPEC int SDLCALL SDL_CondSignal(SDL_cond * cond);

   
                                                                  
  
                                               
                                                                  
                                                
  
                                                     
  
                     
                   
                          
                     
                      
   
extern DECLSPEC int SDLCALL SDL_CondBroadcast(SDL_cond * cond);

   
                                               
  
                                                                              
                                                                         
                                                                              
                        
  
                                                         
  
                                                                          
                                      
  
                                                
                                                          
                                                                           
                                                
  
                                                     
  
                        
                     
                          
                     
                      
   
extern DECLSPEC int SDLCALL SDL_CondWait(SDL_cond * cond, SDL_mutex * mutex);

   
                                                                            
  
                                                                              
                                                                         
                                                                              
                                                                        
           
  
                                                         
  
                                                
                                                          
                                                                              
                                 
                                                                            
                                                                             
                                                                            
  
                                                     
  
                        
                     
                   
                     
                      
   
extern DECLSPEC int SDLCALL SDL_CondWaitTimeout(SDL_cond * cond,
                                                SDL_mutex * mutex, Uint32 ms);

  


 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
