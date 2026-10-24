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

   
                       
  
                                  
   

#ifndef SDL_cpuinfo_h_
#define SDL_cpuinfo_h_

#include "SDL_stdinc.h"

 
 
#if defined(_MSC_VER) && (_MSC_VER >= 1500) && (defined(_M_IX86) || defined(_M_X64))
#ifdef __clang__
                                                                            
                                                                                              

#ifndef __PRFCHWINTRIN_H
#define __PRFCHWINTRIN_H

static __inline__ void __attribute__((__always_inline__, __nodebug__))
_m_prefetch(void *__P)
{
  __builtin_prefetch (__P, 0, 3  );
}

#endif  
#endif  
#include <intrin.h>
#ifndef _WIN64
#ifndef __MMX__
#define __MMX__
#endif
#ifndef __3dNOW__
#define __3dNOW__
#endif
#endif
#ifndef __SSE__
#define __SSE__
#endif
#ifndef __SSE2__
#define __SSE2__
#endif
#ifndef __SSE3__
#define __SSE3__
#endif
#elif defined(__MINGW64_VERSION_MAJOR)
#include <intrin.h>
#if !defined(SDL_DISABLE_ARM_NEON_H) && defined(__ARM_NEON)
#  include <arm_neon.h>
#endif
#else
 
#if defined(HAVE_ALTIVEC_H) && defined(__ALTIVEC__) && !defined(__APPLE_ALTIVEC__) && defined(SDL_ENABLE_ALTIVEC_H)
#include <altivec.h>
#endif
#if !defined(SDL_DISABLE_ARM_NEON_H)
#  if defined(__ARM_NEON)
#    include <arm_neon.h>
#  elif defined(__WINDOWS__) || defined(__WINRT__) || defined(__GDK__)
 
#    if defined(_M_ARM)
#      include <armintr.h>
#      include <arm_neon.h>
#      define __ARM_NEON 1  
#    endif
#    if defined (_M_ARM64)
#      include <arm64intr.h>
#      include <arm64_neon.h>
#      define __ARM_NEON 1  
#      define __ARM_ARCH 8
#    endif
#  endif
#endif
#endif  

#if defined(__3dNOW__) && !defined(SDL_DISABLE_MM3DNOW_H)
#include <mm3dnow.h>
#endif
#if defined(__loongarch_sx) && !defined(SDL_DISABLE_LSX_H)
#include <lsxintrin.h>
#define __LSX__
#endif
#if defined(__loongarch_asx) && !defined(SDL_DISABLE_LASX_H)
#include <lasxintrin.h>
#define __LASX__
#endif
#if defined(HAVE_IMMINTRIN_H) && !defined(SDL_DISABLE_IMMINTRIN_H)
#include <immintrin.h>
#else
#if defined(__MMX__) && !defined(SDL_DISABLE_MMINTRIN_H)
#include <mmintrin.h>
#endif
#if defined(__SSE__) && !defined(SDL_DISABLE_XMMINTRIN_H)
#include <xmmintrin.h>
#endif
#if defined(__SSE2__) && !defined(SDL_DISABLE_EMMINTRIN_H)
#include <emmintrin.h>
#endif
#if defined(__SSE3__) && !defined(SDL_DISABLE_PMMINTRIN_H)
#include <pmmintrin.h>
#endif
#endif  

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

                                                           
                                                 
                                                            
                                                   
   
#define SDL_CACHELINE_SIZE  128

   
                                         
  
                                                                       
                                                                            
                                                          
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_GetCPUCount(void);

   
                                               
  
                                                                          
                  
  
                                                        
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_GetCPUCacheLineSize(void);

   
                                                       
  
                                                                              
  
                                                                              
  
                                                     
  
                   
                     
                 
                  
                 
                 
                  
                  
                   
                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasRDTSC(void);

   
                                                  
  
                                                                          
        
  
                                                                         
  
                                                     
  
                   
                 
                  
                 
                   
                 
                  
                  
                   
                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasAltiVec(void);

   
                                              
  
                                                                              
  
                                                                     
  
                                                     
  
                   
                     
                 
                  
                   
                 
                  
                  
                   
                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasMMX(void);

   
                                                 
  
                                                                            
  
                                                                        
  
                                                     
  
                     
                 
                  
                 
                   
                 
                  
                  
                   
                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_Has3DNow(void);

   
                                              
  
                                                                              
  
                                                                     
  
                                                     
  
                   
                     
                 
                  
                 
                   
                  
                  
                   
                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE(void);

   
                                               
  
                                                                              
  
                                                                      
  
                                                     
  
                   
                     
                 
                  
                 
                   
                 
                  
                   
                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE2(void);

   
                                               
  
                                                                              
  
                                                                      
  
                                                     
  
                   
                     
                 
                  
                 
                   
                 
                  
                   
                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE3(void);

   
                                                 
  
                                                                              
  
                                                                        
  
                                                     
  
                   
                     
                 
                  
                 
                   
                 
                  
                  
                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE41(void);

   
                                                 
  
                                                                              
  
                                                                        
  
                                                     
  
                   
                     
                 
                  
                 
                   
                 
                  
                  
                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE42(void);

   
                                              
  
                                                                              
  
                                                                     
  
                                                     
  
                   
                     
                  
                 
                   
                 
                  
                  
                   
                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasAVX(void);

   
                                               
  
                                                                              
  
                                                                      
  
                                                     
  
                   
                     
                 
                 
                   
                 
                  
                  
                   
                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasAVX2(void);

   
                                                                
  
                                                                              
  
                                                                          
  
                                                     
  
                 
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasAVX512F(void);

   
                                                           
  
                                                                         
  
                                                                            
  
                                                                          
  
                                                      
  
                  
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasARMSIMD(void);

   
                                                          
  
                                                                            
  
                                                                          
  
                                                     
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasNEON(void);

   
                                                               
  
                                                                            
        
  
                                                                          
                
  
                                                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasLSX(void);

   
                                                                
  
                                                                            
        
  
                                                                           
                
  
                                                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasLASX(void);

   
                                                  
  
                                                              
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_GetSystemRAM(void);

   
                                                               
  
                                                                          
                                                                              
                                                                          
                                                                         
                                                                      
                                                                            
                                                                             
                    
  
                                                                   
                         
  
                                                      
   
extern DECLSPEC size_t SDLCALL SDL_SIMDGetAlignment(void);

   
                                          
  
                                                                          
                                                                             
                                          
  
                                                                              
                                                                              
                                                                       
                                                                          
                    
  
                                                                             
                 
  
                                                                              
                                                                              
                                                                           
                                                                         
                                                                            
                
  
                                                                             
                                                                             
                                  
  
                                                                        
                                                                  
                                                                          
  
                                                      
  
                           
                      
                   
   
extern DECLSPEC void * SDLCALL SDL_SIMDAlloc(const size_t len);

   
                                                
  
                                                                      
                                                                      
                                    
  
                                                                         
                                                                       
                                                        
                                                                         
                                                                            
                                                                              
                     
                                                                            
  
                                                      
  
                           
                    
                   
   
extern DECLSPEC void * SDLCALL SDL_SIMDRealloc(void *mem, const size_t len);

   
                                                
  
                                                                      
                                                                          
                                                     
  
                                                
  
                                                                            
                                                                          
                                                                             
                                             
  
                                                                             
                                                
  
                                                      
  
                    
                      
   
extern DECLSPEC void SDLCALL SDL_SIMDFree(void *ptr);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
