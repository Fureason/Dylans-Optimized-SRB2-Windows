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

   
                          
  
                                        
  
                                                                           
   

  

                                                                                 

  

#ifndef SDL_test_crc32_h_
#define SDL_test_crc32_h_

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif


 

 

#ifndef CrcUint32
 #define CrcUint32  unsigned int
#endif
#ifndef CrcUint8
 #define CrcUint8   unsigned char
#endif

#ifdef ORIGINAL_METHOD
 #define CRC32_POLY 0x04c11db7    
#else
 #define CRC32_POLY 0xEDB88320    
#endif

   
                                                  
   
  typedef struct {
    CrcUint32    crc32_table[256];  
  } SDLTest_Crc32Context;

 

   
                                    
  
                                                                                  
  
                                                       
  
                                 
  
   
 int SDLTest_Crc32Init(SDLTest_Crc32Context * crcContext);


   
                                             
  
                                                        
                                                     
                                                   
                                                                          
  
                                 
  
   
int SDLTest_Crc32Calc(SDLTest_Crc32Context * crcContext, CrcUint8 *inBuf, CrcUint32 inLen, CrcUint32 *crc32);

 
int SDLTest_Crc32CalcStart(SDLTest_Crc32Context * crcContext, CrcUint32 *crc32);
int SDLTest_Crc32CalcEnd(SDLTest_Crc32Context * crcContext, CrcUint32 *crc32);
int SDLTest_Crc32CalcBuffer(SDLTest_Crc32Context * crcContext, CrcUint8 *inBuf, CrcUint32 inLen, CrcUint32 *crc32);


   
                              
  
                                                       
  
                                 
  
  

int SDLTest_Crc32Done(SDLTest_Crc32Context * crcContext);


 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
