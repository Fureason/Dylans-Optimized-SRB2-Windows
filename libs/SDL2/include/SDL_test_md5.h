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

   
                        
  
                                        
  
                                                                           
   

  
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
  

  
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
  

#ifndef SDL_test_md5_h_
#define SDL_test_md5_h_

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

 

 
  typedef unsigned long int MD5UINT4;

 
  typedef struct {
    MD5UINT4  i[2];      
    MD5UINT4  buf[4];        
    unsigned char in[64];    
    unsigned char digest[16];    
  } SDLTest_Md5Context;

 

   
                                
  
                                                       
  
                                                            
                                                             
                                    
   
 void SDLTest_Md5Init(SDLTest_Md5Context * mdContext);


   
                                                 
  
                                                      
                                                       
                                                      
  
                                                                   
                                                                     
                                                       
  

 void SDLTest_Md5Update(SDLTest_Md5Context * mdContext, unsigned char *inBuf,
                 unsigned int inLen);


   
                                     
  
                                                   
  
                                                                   
                                                                         
                                                        
  

 void SDLTest_Md5Final(SDLTest_Md5Context * mdContext);


 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
