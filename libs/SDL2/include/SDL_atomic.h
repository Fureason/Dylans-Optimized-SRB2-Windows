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

   
                     
  
                     
  
             
                                                                          
                                                                         
                                                                          
                     
  
                                           
                    
                      
                      
                      
  
                              
                              
  
                                                                    
                                     
                                                                     
  
                                              
                                                     
                       
  
                                                                
                                                               
                                                                      
                                                                    
                               
  
                                                                            
   

#ifndef SDL_atomic_h_
#define SDL_atomic_h_

#include "SDL_stdinc.h"
#include "SDL_platform.h"

#include "begin_code.h"

 
#ifdef __cplusplus
extern "C" {
#endif

   
                       
  
                                                                   
                                                                    
                                                                  
                                                                
                                                                
  
                                                     
  
                
                                                               
                                                               
   
 

typedef int SDL_SpinLock;

   
                                                             
  
                                                                            
                                                          
  
                                           
                                                                            
                 
  
                                                     
  
                     
                       
   
extern DECLSPEC SDL_bool SDLCALL SDL_AtomicTryLock(SDL_SpinLock *lock);

   
                                                      
  
                                                                            
                                                          
  
                                           
  
                                                     
  
                        
                       
   
extern DECLSPEC void SDLCALL SDL_AtomicLock(SDL_SpinLock *lock);

   
                                         
  
                              
  
                                                                            
                                                          
  
                                           
  
                                                     
  
                     
                        
   
extern DECLSPEC void SDLCALL SDL_AtomicUnlock(SDL_SpinLock *lock);

  


   
                                                             
                                                                  
   
#if defined(_MSC_VER) && (_MSC_VER > 1200) && !defined(__clang__)
void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadWriteBarrier)
#define SDL_CompilerBarrier()   _ReadWriteBarrier()
#elif (defined(__GNUC__) && !defined(__EMSCRIPTEN__)) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x5120))
 
#define SDL_CompilerBarrier()   __asm__ __volatile__ ("" : : : "memory")
#elif defined(__WATCOMC__)
extern __inline void SDL_CompilerBarrier(void);
#pragma aux SDL_CompilerBarrier = "" parm [] modify exact [];
#else
#define SDL_CompilerBarrier()   \
{ SDL_SpinLock _tmp = 0; SDL_AtomicLock(&_tmp); SDL_AtomicUnlock(&_tmp); }
#endif

   
                                                                      
                                                                            
  
                                                                             
                                                                         
                                                                  
                                                                       
                                                                            
                                                                              
                  
  
                                                                         
                                                                          
                 
  
                                                                         
                                                             
  
                                                     
   
extern DECLSPEC void SDLCALL SDL_MemoryBarrierReleaseFunction(void);
extern DECLSPEC void SDLCALL SDL_MemoryBarrierAcquireFunction(void);

#if defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))
#define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("lwsync" : : : "memory")
#define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("lwsync" : : : "memory")
#elif defined(__GNUC__) && defined(__aarch64__)
#define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("dmb ish" : : : "memory")
#define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("dmb ish" : : : "memory")
#elif defined(__GNUC__) && defined(__arm__)
#if 0  
                    
                                                                                                    

                                                                                                  
                                   
  
typedef void (*SDL_KernelMemoryBarrierFunc)();
#define SDL_MemoryBarrierRelease()	((SDL_KernelMemoryBarrierFunc)0xffff0fa0)()
#define SDL_MemoryBarrierAcquire()	((SDL_KernelMemoryBarrierFunc)0xffff0fa0)()
#elif 0  
#include <sys/cpuinline.h>

#define SDL_MemoryBarrierRelease()   __cpu_membarrier()
#define SDL_MemoryBarrierAcquire()   __cpu_membarrier()
#else
#if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__) || defined(__ARM_ARCH_8A__)
#define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("dmb ish" : : : "memory")
#define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("dmb ish" : : : "memory")
#elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6T2__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_5TE__)
#ifdef __thumb__
 
#define SDL_MEMORY_BARRIER_USES_FUNCTION
#define SDL_MemoryBarrierRelease()   SDL_MemoryBarrierReleaseFunction()
#define SDL_MemoryBarrierAcquire()   SDL_MemoryBarrierAcquireFunction()
#else
#define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" : : "r"(0) : "memory")
#define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" : : "r"(0) : "memory")
#endif  
#else
#define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("" : : : "memory")
#define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("" : : : "memory")
#endif  
#endif  
#else
#if (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x5120))
 
#include <mbarrier.h>
#define SDL_MemoryBarrierRelease()  __machine_rel_barrier()
#define SDL_MemoryBarrierAcquire()  __machine_acq_barrier()
#else
 
#define SDL_MemoryBarrierRelease()  SDL_CompilerBarrier()
#define SDL_MemoryBarrierAcquire()  SDL_CompilerBarrier()
#endif
#endif

 
#if (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
    #define SDL_CPUPauseInstruction() __asm__ __volatile__("pause\n")   
#elif (defined(__arm__) && defined(__ARM_ARCH) && __ARM_ARCH >= 7) || defined(__aarch64__)
    #define SDL_CPUPauseInstruction() __asm__ __volatile__("yield" ::: "memory")
#elif (defined(__powerpc__) || defined(__powerpc64__))
    #define SDL_CPUPauseInstruction() __asm__ __volatile__("or 27,27,27");
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    #define SDL_CPUPauseInstruction() _mm_pause()   
#elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
    #define SDL_CPUPauseInstruction() __yield()
#elif defined(__WATCOMC__) && defined(__386__)
    extern __inline void SDL_CPUPauseInstruction(void);
    #pragma aux SDL_CPUPauseInstruction = ".686p" ".xmm2" "pause"
#else
    #define SDL_CPUPauseInstruction()
#endif


   
                                                                      
                                                                    
   
typedef struct { int value; } SDL_atomic_t;

   
                                                                         
  
                                                                          
         
  
                                                                
                              
                              
                                                                         
  
                                                     
  
                       
                    
                    
   
extern DECLSPEC SDL_bool SDLCALL SDL_AtomicCAS(SDL_atomic_t *a, int oldval, int newval);

   
                                     
  
                                                    
  
                                                                          
         
  
                                                                
                             
                                                      
  
                                                     
  
                    
   
extern DECLSPEC int SDLCALL SDL_AtomicSet(SDL_atomic_t *a, int v);

   
                                       
  
                                                                          
         
  
                                                 
                                                    
  
                                                     
  
                    
   
extern DECLSPEC int SDLCALL SDL_AtomicGet(SDL_atomic_t *a);

   
                             
  
                                                    
  
                                                                          
         
  
                                                                
                                    
                                                      
  
                                                     
  
                       
                       
   
extern DECLSPEC int SDLCALL SDL_AtomicAdd(SDL_atomic_t *a, int v);

   
                                                                 
   
#ifndef SDL_AtomicIncRef
#define SDL_AtomicIncRef(a)    SDL_AtomicAdd(a, 1)
#endif

   
                                                                 
  
                                                                    
                              
   
#ifndef SDL_AtomicDecRef
#define SDL_AtomicDecRef(a)    (SDL_AtomicAdd(a, -1) == 1)
#endif

   
                                                                
  
                                                                          
         
  
                                  
                                      
                                      
                                                                 
  
                                                     
  
                    
                       
                       
   
extern DECLSPEC SDL_bool SDLCALL SDL_AtomicCASPtr(void **a, void *oldval, void *newval);

   
                                       
  
                                                                          
         
  
                                  
                                     
                                              
  
                                                     
  
                       
                       
   
extern DECLSPEC void* SDLCALL SDL_AtomicSetPtr(void **a, void* v);

   
                                         
  
                                                                          
         
  
                                  
                                           
  
                                                     
  
                       
                       
   
extern DECLSPEC void* SDLCALL SDL_AtomicGetPtr(void **a);

 
#ifdef __cplusplus
}
#endif

#include "close_code.h"

#endif  

 
