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

   
                           
  
                                        
  
                                                                           
   

  

                                                              

  

#ifndef SDL_test_fuzzer_h_
#define SDL_test_fuzzer_h_

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif


  
                                                                   
  


   
        
                                                                           
                                           
   

   
                                    
  
                                                                                                     
  
   
void SDLTest_FuzzerInit(Uint64 execKey);


   
                         
  
                               
   
Uint8 SDLTest_RandomUint8(void);

   
                         
  
                                      
   
Sint8 SDLTest_RandomSint8(void);


   
                          
  
                               
   
Uint16 SDLTest_RandomUint16(void);

   
                          
  
                                      
   
Sint16 SDLTest_RandomSint16(void);


   
                           
  
                               
   
Sint32 SDLTest_RandomSint32(void);


   
                                    
  
                               
   
Uint32 SDLTest_RandomUint32(void);

   
                         
  
                               
   
Uint64 SDLTest_RandomUint64(void);


   
                         
  
                                      
   
Sint64 SDLTest_RandomSint64(void);

   
                                               
   
float SDLTest_RandomUnitFloat(void);

   
                                                
   
double SDLTest_RandomUnitDouble(void);

   
                           
  
   
float SDLTest_RandomFloat(void);

   
                            
  
   
double SDLTest_RandomDouble(void);

   
                                                                         
                                                                         
                                                                               
                                
                                                   
  
                  
                                                                      
                                                             
                                                         
                                                                    
  
                                        
                                        
                                                                                         
  
                                                                                      
   
Uint8 SDLTest_RandomUint8BoundaryValue(Uint8 boundary1, Uint8 boundary2, SDL_bool validDomain);

   
                                                                          
                                                                         
                                                                               
                                
                                                   
  
                  
                                                                       
                                                              
                                                          
                                                                        
  
                                        
                                        
                                                                                         
  
                                                                                      
   
Uint16 SDLTest_RandomUint16BoundaryValue(Uint16 boundary1, Uint16 boundary2, SDL_bool validDomain);

   
                                                                          
                                                                         
                                                                               
                                
                                                   
  
                  
                                                                       
                                                              
                                                          
                                                                                 
  
                                        
                                        
                                                                                         
  
                                                                                      
   
Uint32 SDLTest_RandomUint32BoundaryValue(Uint32 boundary1, Uint32 boundary2, SDL_bool validDomain);

   
                                                                          
                                                                         
                                                                               
                                
                                                   
  
                  
                                                                       
                                                              
                                                          
                                                                                         
  
                                        
                                        
                                                                                         
  
                                                                                      
   
Uint64 SDLTest_RandomUint64BoundaryValue(Uint64 boundary1, Uint64 boundary2, SDL_bool validDomain);

   
                                                                         
                                                                         
                                                                               
                                
                                                   
  
                  
                                                                         
                                                                    
                                                                 
                                                                                                              
  
                                        
                                        
                                                                                         
  
                                                                                              
   
Sint8 SDLTest_RandomSint8BoundaryValue(Sint8 boundary1, Sint8 boundary2, SDL_bool validDomain);


   
                                                                          
                                                                         
                                                                               
                                
                                                   
  
                  
                                                                          
                                                                     
                                                                   
                                                                                                                  
  
                                        
                                        
                                                                                         
  
                                                                                               
   
Sint16 SDLTest_RandomSint16BoundaryValue(Sint16 boundary1, Sint16 boundary2, SDL_bool validDomain);

   
                                                                          
                                                                         
                                                                               
                                
                                                   
  
                  
                                                                          
                                                                     
                                                                   
                                                                                                   
  
                                        
                                        
                                                                                         
  
                                                                                               
   
Sint32 SDLTest_RandomSint32BoundaryValue(Sint32 boundary1, Sint32 boundary2, SDL_bool validDomain);

   
                                                                          
                                                                         
                                                                               
                                
                                                   
  
                  
                                                                          
                                                                     
                                                                   
                                                                                                                 
  
                                        
                                        
                                                                                         
  
                                                                                               
   
Sint64 SDLTest_RandomSint64BoundaryValue(Sint64 boundary1, Sint64 boundary2, SDL_bool validDomain);


   
                                                   
                                             
                                                           
                                                               
  
                                                               
                                                               
  
                                               
   
Sint32 SDLTest_RandomIntegerInRange(Sint32 min, Sint32 max);


   
                                                                  
                                                                  
                                                                 
  
                                                 
  
                                                                                                            
   
char * SDLTest_RandomAsciiString(void);


   
                                                                  
                                                    
                                                      
  
                                                 
  
                                                               
  
                                                                                                               
   
char * SDLTest_RandomAsciiStringWithMaximumLength(int maxLength);


   
                                                          
                                               
                                                      
  
                                                 
  
                                                 
  
                                                                                                          
   
char * SDLTest_RandomAsciiStringOfSize(int size);

   
                                                                    
  
                                 
   
int SDLTest_GetFuzzerInvocationCount(void);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
