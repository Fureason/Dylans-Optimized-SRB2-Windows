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

   
                           
  
                                        
  
                                                                           
   

  
  
                                          
  
   

#ifndef SDL_test_assert_h_
#define SDL_test_assert_h_

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

   
                           
   
#define ASSERT_FAIL     0

   
                            
   
#define ASSERT_PASS     1

   
                                                                
  
                                                                                              
                                                                         
   
void SDLTest_Assert(int assertCondition, SDL_PRINTF_FORMAT_STRING const char *assertDescription, ...) SDL_PRINTF_VARARG_FUNC(2);

   
                                                                                                                    
  
                                                                                              
                                                                         
  
                                                                                                   
   
int SDLTest_AssertCheck(int assertCondition, SDL_PRINTF_FORMAT_STRING const char *assertDescription, ...) SDL_PRINTF_VARARG_FUNC(2);

   
                                                                                             
  
                                                                         
   
void SDLTest_AssertPass(SDL_PRINTF_FORMAT_STRING const char *assertDescription, ...) SDL_PRINTF_VARARG_FUNC(1);

   
                                                     
   
void SDLTest_ResetAssertSummary(void);

   
                                                                                               
   
void SDLTest_LogAssertSummary(void);


   
                                                                     
  
                                                                            
   
int SDLTest_AssertSummaryToTestResult(void);

#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
