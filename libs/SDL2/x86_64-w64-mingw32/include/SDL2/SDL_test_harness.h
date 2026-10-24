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

   
                            
  
                                        
  
                                                                           
   

  
                                                                             

                                                                            
  

#ifndef SDL_test_h_arness_h
#define SDL_test_h_arness_h

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif


 
#define TEST_ENABLED  1
#define TEST_DISABLED 0

 
#define TEST_ABORTED        -1
#define TEST_STARTED         0
#define TEST_COMPLETED       1
#define TEST_SKIPPED         2

 
#define TEST_RESULT_PASSED              0
#define TEST_RESULT_FAILED              1
#define TEST_RESULT_NO_ASSERT           2
#define TEST_RESULT_SKIPPED             3
#define TEST_RESULT_SETUP_FAILURE       4

 
typedef void (*SDLTest_TestCaseSetUpFp)(void *arg);

 
typedef int (*SDLTest_TestCaseFp)(void *arg);

 
typedef void  (*SDLTest_TestCaseTearDownFp)(void *arg);

   
                                              
   
typedef struct SDLTest_TestCaseReference {
     
    SDLTest_TestCaseFp testCase;
     
    const char *name;
     
    const char *description;
     
    int enabled;
} SDLTest_TestCaseReference;

   
                                                              
   
typedef struct SDLTest_TestSuiteReference {
     
    const char *name;
     
    SDLTest_TestCaseSetUpFp testSetUp;
     
    const SDLTest_TestCaseReference **testCases;
     
    SDLTest_TestCaseTearDownFp testTearDown;
} SDLTest_TestSuiteReference;


   
                                                                                                                               
  
                                                                   
  
                                                          
  
                                     
   
char *SDLTest_GenerateRunSeed(const int length);

   
                                                                          
  
                                                     
                                                                                    
                                                                                      
                                                                     
                                                                    
  
                                                                                
   
int SDLTest_RunSuites(SDLTest_TestSuiteReference *testSuites[], const char *userRunSeed, Uint64 userExecKey, const char *filter, int testIterations);


 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
