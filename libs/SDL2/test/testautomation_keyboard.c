   
                      
   

#include <stdio.h>
#include <limits.h>

#include "SDL_config.h"
#include "SDL.h"
#include "SDL_test.h"

 

 

   
                                                                                
  
                                                  
   
int keyboard_getKeyboardState(void *arg)
{
    int numkeys;
    Uint8 *state;

     
    state = (Uint8 *)SDL_GetKeyboardState(NULL);
    SDLTest_AssertPass("Call to SDL_GetKeyboardState(NULL)");
    SDLTest_AssertCheck(state != NULL, "Validate that return value from SDL_GetKeyboardState is not NULL");

     
    numkeys = -1;
    state = (Uint8 *)SDL_GetKeyboardState(&numkeys);
    SDLTest_AssertPass("Call to SDL_GetKeyboardState(&numkeys)");
    SDLTest_AssertCheck(state != NULL, "Validate that return value from SDL_GetKeyboardState is not NULL");
    SDLTest_AssertCheck(numkeys >= 0, "Validate that value of numkeys is >= 0, got: %i", numkeys);

    return TEST_COMPLETED;
}

   
                                            
  
                                                  
   
int keyboard_getKeyboardFocus(void *arg)
{
     
    SDL_GetKeyboardFocus();
    SDLTest_AssertPass("Call to SDL_GetKeyboardFocus()");

    return TEST_COMPLETED;
}

   
                                                                               
  
                                                
   
int keyboard_getKeyFromName(void *arg)
{
    SDL_Keycode result;

     
    result = SDL_GetKeyFromName("A");
    SDLTest_AssertPass("Call to SDL_GetKeyFromName(known/single)");
    SDLTest_AssertCheck(result == SDLK_a, "Verify result from call, expected: %i, got: %" SDL_PRIs32, SDLK_a, result);

     
    result = SDL_GetKeyFromName("F1");
    SDLTest_AssertPass("Call to SDL_GetKeyFromName(known/double)");
    SDLTest_AssertCheck(result == SDLK_F1, "Verify result from call, expected: %i, got: %" SDL_PRIs32, SDLK_F1, result);

     
    result = SDL_GetKeyFromName("End");
    SDLTest_AssertPass("Call to SDL_GetKeyFromName(known/triple)");
    SDLTest_AssertCheck(result == SDLK_END, "Verify result from call, expected: %i, got: %" SDL_PRIs32, SDLK_END, result);

     
    result = SDL_GetKeyFromName("Find");
    SDLTest_AssertPass("Call to SDL_GetKeyFromName(known/quad)");
    SDLTest_AssertCheck(result == SDLK_FIND, "Verify result from call, expected: %i, got: %" SDL_PRIs32, SDLK_FIND, result);

     
    result = SDL_GetKeyFromName("AudioStop");
    SDLTest_AssertPass("Call to SDL_GetKeyFromName(known/multi)");
    SDLTest_AssertCheck(result == SDLK_AUDIOSTOP, "Verify result from call, expected: %i, got: %" SDL_PRIs32, SDLK_AUDIOSTOP, result);

     
    result = SDL_GetKeyFromName("NotThere");
    SDLTest_AssertPass("Call to SDL_GetKeyFromName(unknown)");
    SDLTest_AssertCheck(result == SDLK_UNKNOWN, "Verify result from call is UNKNOWN, expected: %i, got: %" SDL_PRIs32, SDLK_UNKNOWN, result);

     
    result = SDL_GetKeyFromName(NULL);
    SDLTest_AssertPass("Call to SDL_GetKeyFromName(NULL)");
    SDLTest_AssertCheck(result == SDLK_UNKNOWN, "Verify result from call is UNKNOWN, expected: %i, got: %" SDL_PRIs32, SDLK_UNKNOWN, result);

    return TEST_COMPLETED;
}

  
                                                               
   
void _checkInvalidScancodeError()
{
    const char *expectedError = "Parameter 'scancode' is invalid";
    const char *error;
    error = SDL_GetError();
    SDLTest_AssertPass("Call to SDL_GetError()");
    SDLTest_AssertCheck(error != NULL, "Validate that error message was not NULL");
    if (error != NULL) {
        SDLTest_AssertCheck(SDL_strcmp(error, expectedError) == 0,
                            "Validate error message, expected: '%s', got: '%s'", expectedError, error);
        SDL_ClearError();
        SDLTest_AssertPass("Call to SDL_ClearError()");
    }
}

   
                                              
  
                                                    
   
int keyboard_getKeyFromScancode(void *arg)
{
    SDL_Keycode result;

     
    result = SDL_GetKeyFromScancode(SDL_SCANCODE_A);
    SDLTest_AssertPass("Call to SDL_GetKeyFromScancode(valid)");
    SDLTest_AssertCheck(result == SDLK_a, "Verify result from call, expected: %i, got: %" SDL_PRIs32, SDLK_a, result);

     
    result = SDL_GetKeyFromScancode(0);
    SDLTest_AssertPass("Call to SDL_GetKeyFromScancode(0)");
    SDLTest_AssertCheck(result == SDLK_UNKNOWN, "Verify result from call is UNKNOWN, expected: %i, got: %" SDL_PRIs32, SDLK_UNKNOWN, result);

     
    SDL_ClearError();
    SDLTest_AssertPass("Call to SDL_ClearError()");

     
    result = SDL_GetKeyFromScancode(-999);
    SDLTest_AssertPass("Call to SDL_GetKeyFromScancode(-999)");
    SDLTest_AssertCheck(result == SDLK_UNKNOWN, "Verify result from call is UNKNOWN, expected: %i, got: %" SDL_PRIs32, SDLK_UNKNOWN, result);
    _checkInvalidScancodeError();

     
    result = SDL_GetKeyFromScancode(999);
    SDLTest_AssertPass("Call to SDL_GetKeyFromScancode(999)");
    SDLTest_AssertCheck(result == SDLK_UNKNOWN, "Verify result from call is UNKNOWN, expected: %i, got: %" SDL_PRIs32, SDLK_UNKNOWN, result);
    _checkInvalidScancodeError();

    return TEST_COMPLETED;
}

   
                                      
  
                                            
   
int keyboard_getKeyName(void *arg)
{
    const char *result;
    const char *expected;

     
    expected = "3";
    result = (char *)SDL_GetKeyName(SDLK_3);
    SDLTest_AssertPass("Call to SDL_GetKeyName()");
    SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
    SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: %s, got: %s", expected, result);

     
    expected = "F1";
    result = (char *)SDL_GetKeyName(SDLK_F1);
    SDLTest_AssertPass("Call to SDL_GetKeyName()");
    SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
    SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: %s, got: %s", expected, result);

     
    expected = "Cut";
    result = (char *)SDL_GetKeyName(SDLK_CUT);
    SDLTest_AssertPass("Call to SDL_GetKeyName()");
    SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
    SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: %s, got: %s", expected, result);

     
    expected = "Down";
    result = (char *)SDL_GetKeyName(SDLK_DOWN);
    SDLTest_AssertPass("Call to SDL_GetKeyName()");
    SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
    SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: %s, got: %s", expected, result);

     
    expected = "BrightnessUp";
    result = (char *)SDL_GetKeyName(SDLK_BRIGHTNESSUP);
    SDLTest_AssertPass("Call to SDL_GetKeyName()");
    SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
    SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: %s, got: %s", expected, result);

     
    expected = "Keypad MemStore";
    result = (char *)SDL_GetKeyName(SDLK_KP_MEMSTORE);
    SDLTest_AssertPass("Call to SDL_GetKeyName()");
    SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
    SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: %s, got: %s", expected, result);

    return TEST_COMPLETED;
}

   
                                            
  
                                                 
   
int keyboard_getScancodeNameNegative(void *arg)
{
    SDL_Scancode scancode;
    const char *result;
    const char *expected = "";

     
    SDL_ClearError();
    SDLTest_AssertPass("Call to SDL_ClearError()");

     
    scancode = (SDL_Scancode)SDL_NUM_SCANCODES;
    result = (char *)SDL_GetScancodeName(scancode);
    SDLTest_AssertPass("Call to SDL_GetScancodeName(%d/large)", scancode);
    SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
    SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: '%s', got: '%s'", expected, result);
    _checkInvalidScancodeError();

    return TEST_COMPLETED;
}

   
                                       
  
                                            
   
int keyboard_getKeyNameNegative(void *arg)
{
    SDL_Keycode keycode;
    const char *result;
    const char *expected = "";

     
    keycode = SDLK_UNKNOWN;
    result = (char *)SDL_GetKeyName(keycode);
    SDLTest_AssertPass("Call to SDL_GetKeyName(%" SDL_PRIs32 "/unknown)", keycode);
    SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
    SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: '%s', got: '%s'", expected, result);

     
    SDL_ClearError();
    SDLTest_AssertPass("Call to SDL_ClearError()");

     
    keycode = (SDL_Keycode)SDLTest_RandomIntegerInRange(-255, -1);
    result = (char *)SDL_GetKeyName(keycode);
    SDLTest_AssertPass("Call to SDL_GetKeyName(%" SDL_PRIs32 "/negative)", keycode);
    SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
    SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: '%s', got: '%s'", expected, result);
    _checkInvalidScancodeError();

    SDL_ClearError();
    SDLTest_AssertPass("Call to SDL_ClearError()");

    return TEST_COMPLETED;
}

   
                                                           
  
                                             
                                             
   
int keyboard_getSetModState(void *arg)
{
    SDL_Keymod result;
    SDL_Keymod currentState;
    SDL_Keymod newState;
    SDL_Keymod allStates =
        KMOD_NONE |
        KMOD_LSHIFT |
        KMOD_RSHIFT |
        KMOD_LCTRL |
        KMOD_RCTRL |
        KMOD_LALT |
        KMOD_RALT |
        KMOD_LGUI |
        KMOD_RGUI |
        KMOD_NUM |
        KMOD_CAPS |
        KMOD_MODE |
        KMOD_SCROLL;

     
    result = SDL_GetModState();
    SDLTest_AssertPass("Call to SDL_GetModState()");
    SDLTest_AssertCheck(  result <= allStates, "Verify result from call is valid, expected: 0 <= result <= %i, got: %i", allStates, result);
    currentState = result;

     
    newState = SDLTest_RandomIntegerInRange(0, allStates);
    SDL_SetModState(newState);
    SDLTest_AssertPass("Call to SDL_SetModState(%i)", newState);
    result = SDL_GetModState();
    SDLTest_AssertPass("Call to SDL_GetModState()");
    SDLTest_AssertCheck(result == newState, "Verify result from call is valid, expected: %i, got: %i", newState, result);

     
    SDL_SetModState(0);
    SDLTest_AssertPass("Call to SDL_SetModState(0)");
    result = SDL_GetModState();
    SDLTest_AssertPass("Call to SDL_GetModState()");
    SDLTest_AssertCheck(result == 0, "Verify result from call is valid, expected: 0, got: %i", result);

     
    if (currentState != 0) {
        SDL_SetModState(currentState);
        SDLTest_AssertPass("Call to SDL_SetModState(%i)", currentState);
        result = SDL_GetModState();
        SDLTest_AssertPass("Call to SDL_GetModState()");
        SDLTest_AssertCheck(result == currentState, "Verify result from call is valid, expected: %i, got: %i", currentState, result);
    }

    return TEST_COMPLETED;
}

   
                                                                
  
                                                
                                               
   
int keyboard_startStopTextInput(void *arg)
{
     
    SDL_StartTextInput();
    SDLTest_AssertPass("Call to SDL_StartTextInput()");
    SDL_StopTextInput();
    SDLTest_AssertPass("Call to SDL_StopTextInput()");

     
    SDL_StartTextInput();
    SDLTest_AssertPass("Call to SDL_StartTextInput()");

     
    SDL_StartTextInput();
    SDLTest_AssertPass("Call to SDL_StartTextInput()");

     
    SDL_StopTextInput();
    SDLTest_AssertPass("Call to SDL_StopTextInput()");
    SDL_StopTextInput();
    SDLTest_AssertPass("Call to SDL_StopTextInput()");

    return TEST_COMPLETED;
}

 
void _testSetTextInputRect(SDL_Rect refRect)
{
    SDL_Rect testRect;

    testRect = refRect;
    SDL_SetTextInputRect(&testRect);
    SDLTest_AssertPass("Call to SDL_SetTextInputRect with refRect(x:%i,y:%i,w:%i,h:%i)", refRect.x, refRect.y, refRect.w, refRect.h);
    SDLTest_AssertCheck(
        (refRect.x == testRect.x) && (refRect.y == testRect.y) && (refRect.w == testRect.w) && (refRect.h == testRect.h),
        "Check that input data was not modified, expected: x:%i,y:%i,w:%i,h:%i, got: x:%i,y:%i,w:%i,h:%i",
        refRect.x, refRect.y, refRect.w, refRect.h,
        testRect.x, testRect.y, testRect.w, testRect.h);
}

   
                                            
  
                                                  
   
int keyboard_setTextInputRect(void *arg)
{
    SDL_Rect refRect;

     
    refRect.x = SDLTest_RandomIntegerInRange(1, 50);
    refRect.y = SDLTest_RandomIntegerInRange(1, 50);
    refRect.w = SDLTest_RandomIntegerInRange(10, 50);
    refRect.h = SDLTest_RandomIntegerInRange(10, 50);
    _testSetTextInputRect(refRect);

     
    refRect.x = 0;
    refRect.y = 0;
    refRect.w = SDLTest_RandomIntegerInRange(10, 50);
    refRect.h = SDLTest_RandomIntegerInRange(10, 50);
    _testSetTextInputRect(refRect);

     
    refRect.x = SDLTest_RandomIntegerInRange(10, 50);
    refRect.y = SDLTest_RandomIntegerInRange(10, 50);
    refRect.w = 1;
    refRect.h = 1;
    _testSetTextInputRect(refRect);

     
    refRect.x = 1;
    refRect.y = 1;
    refRect.w = 1;
    refRect.h = 0;
    _testSetTextInputRect(refRect);

     
    refRect.x = 1;
    refRect.y = 1;
    refRect.w = 0;
    refRect.h = 1;
    _testSetTextInputRect(refRect);

     
    refRect.x = 1;
    refRect.y = 1;
    refRect.w = 0;
    refRect.h = 0;
    _testSetTextInputRect(refRect);

     
    refRect.x = 0;
    refRect.y = 0;
    refRect.w = 0;
    refRect.h = 0;
    _testSetTextInputRect(refRect);

     
    refRect.x = SDLTest_RandomIntegerInRange(-200, -100);
    refRect.y = SDLTest_RandomIntegerInRange(-200, -100);
    refRect.w = 50;
    refRect.h = 50;
    _testSetTextInputRect(refRect);

     
    refRect.x = SDLTest_RandomIntegerInRange(1, 50);
    refRect.y = SDLTest_RandomIntegerInRange(1, 50);
    refRect.w = 5000;
    refRect.h = 5000;
    _testSetTextInputRect(refRect);

     
    SDL_SetTextInputRect(NULL);
    SDLTest_AssertPass("Call to SDL_SetTextInputRect(NULL)");

    return TEST_COMPLETED;
}

   
                                                              
  
                                                  
   
int keyboard_setTextInputRectNegative(void *arg)
{
     
#if SDL_VIDEO_DRIVER_WINDOWS || SDL_VIDEO_DRIVER_ANDROID || SDL_VIDEO_DRIVER_COCOA
    const char *expectedError = "Parameter 'rect' is invalid";
    const char *error;

    SDL_ClearError();
    SDLTest_AssertPass("Call to SDL_ClearError()");
#endif

     
    SDL_SetTextInputRect(NULL);
    SDLTest_AssertPass("Call to SDL_SetTextInputRect(NULL)");

     
#if SDL_VIDEO_DRIVER_WINDOWS || SDL_VIDEO_DRIVER_ANDROID || SDL_VIDEO_DRIVER_COCOA
    error = SDL_GetError();
    SDLTest_AssertPass("Call to SDL_GetError()");
    SDLTest_AssertCheck(error != NULL, "Validate that error message was not NULL");
    if (error != NULL) {
        SDLTest_AssertCheck(SDL_strcmp(error, expectedError) == 0,
                            "Validate error message, expected: '%s', got: '%s'", expectedError, error);
    }

    SDL_ClearError();
    SDLTest_AssertPass("Call to SDL_ClearError()");
#endif

    return TEST_COMPLETED;
}

   
                                              
  
                                                    
                                         
   
int keyboard_getScancodeFromKey(void *arg)
{
    SDL_Scancode scancode;

     
    scancode = SDL_GetScancodeFromKey(SDLK_4);
    SDLTest_AssertPass("Call to SDL_GetScancodeFromKey(SDLK_4)");
    SDLTest_AssertCheck(scancode == SDL_SCANCODE_4, "Validate return value from SDL_GetScancodeFromKey, expected: %i, got: %i", SDL_SCANCODE_4, scancode);

     
    scancode = SDL_GetScancodeFromKey(SDLK_PLUS);
    SDLTest_AssertPass("Call to SDL_GetScancodeFromKey(SDLK_PLUS)");
    SDLTest_AssertCheck(scancode == 0, "Validate return value from SDL_GetScancodeFromKey, expected: 0, got: %i", scancode);

    return TEST_COMPLETED;
}

   
                                               
  
                                                     
                                         
   
int keyboard_getScancodeFromName(void *arg)
{
    SDL_Scancode scancode;

     
    scancode = SDL_GetScancodeFromName("A");
    SDLTest_AssertPass("Call to SDL_GetScancodeFromName('A')");
    SDLTest_AssertCheck(scancode == SDL_SCANCODE_A, "Validate return value from SDL_GetScancodeFromName, expected: %i, got: %i", SDL_SCANCODE_A, scancode);

     
    scancode = SDL_GetScancodeFromName("4");
    SDLTest_AssertPass("Call to SDL_GetScancodeFromName('4')");
    SDLTest_AssertCheck(scancode == SDL_SCANCODE_4, "Validate return value from SDL_GetScancodeFromName, expected: %i, got: %i", SDL_SCANCODE_4, scancode);

     
    scancode = SDL_GetScancodeFromName("F1");
    SDLTest_AssertPass("Call to SDL_GetScancodeFromName('F1')");
    SDLTest_AssertCheck(scancode == SDL_SCANCODE_F1, "Validate return value from SDL_GetScancodeFromName, expected: %i, got: %i", SDL_SCANCODE_F1, scancode);

     
    scancode = SDL_GetScancodeFromName("End");
    SDLTest_AssertPass("Call to SDL_GetScancodeFromName('End')");
    SDLTest_AssertCheck(scancode == SDL_SCANCODE_END, "Validate return value from SDL_GetScancodeFromName, expected: %i, got: %i", SDL_SCANCODE_END, scancode);

     
    scancode = SDL_GetScancodeFromName("Find");
    SDLTest_AssertPass("Call to SDL_GetScancodeFromName('Find')");
    SDLTest_AssertCheck(scancode == SDL_SCANCODE_FIND, "Validate return value from SDL_GetScancodeFromName, expected: %i, got: %i", SDL_SCANCODE_FIND, scancode);

     
    scancode = SDL_GetScancodeFromName("Backspace");
    SDLTest_AssertPass("Call to SDL_GetScancodeFromName('Backspace')");
    SDLTest_AssertCheck(scancode == SDL_SCANCODE_BACKSPACE, "Validate return value from SDL_GetScancodeFromName, expected: %i, got: %i", SDL_SCANCODE_BACKSPACE, scancode);

     
    scancode = SDL_GetScancodeFromName("Keypad Enter");
    SDLTest_AssertPass("Call to SDL_GetScancodeFromName('Keypad Enter')");
    SDLTest_AssertCheck(scancode == SDL_SCANCODE_KP_ENTER, "Validate return value from SDL_GetScancodeFromName, expected: %i, got: %i", SDL_SCANCODE_KP_ENTER, scancode);

     
    scancode = SDL_GetScancodeFromName("Sleep");
    SDLTest_AssertPass("Call to SDL_GetScancodeFromName('Sleep')");
    SDLTest_AssertCheck(scancode == SDL_SCANCODE_SLEEP, "Validate return value from SDL_GetScancodeFromName, expected: %i, got: %i", SDL_SCANCODE_SLEEP, scancode);

    return TEST_COMPLETED;
}

  
                                                               
   
void _checkInvalidNameError()
{
    const char *expectedError = "Parameter 'name' is invalid";
    const char *error;
    error = SDL_GetError();
    SDLTest_AssertPass("Call to SDL_GetError()");
    SDLTest_AssertCheck(error != NULL, "Validate that error message was not NULL");
    if (error != NULL) {
        SDLTest_AssertCheck(SDL_strcmp(error, expectedError) == 0,
                            "Validate error message, expected: '%s', got: '%s'", expectedError, error);
        SDL_ClearError();
        SDLTest_AssertPass("Call to SDL_ClearError()");
    }
}

   
                                                                 
  
                                                     
                                         
   
int keyboard_getScancodeFromNameNegative(void *arg)
{
    const char *name;
    SDL_Scancode scancode;

     
    SDL_ClearError();
    SDLTest_AssertPass("Call to SDL_ClearError()");

     
    name = SDLTest_RandomAsciiStringOfSize(32);
    SDLTest_Assert(name != NULL, "Check that random name is not NULL");
    if (name == NULL) {
        return TEST_ABORTED;
    }
    scancode = SDL_GetScancodeFromName(name);
    SDLTest_AssertPass("Call to SDL_GetScancodeFromName('%s')", name);
    SDL_free((void *)name);
    SDLTest_AssertCheck(scancode == SDL_SCANCODE_UNKNOWN, "Validate return value from SDL_GetScancodeFromName, expected: %i, got: %i", SDL_SCANCODE_UNKNOWN, scancode);
    _checkInvalidNameError();

     
    name = "";
    scancode = SDL_GetScancodeFromName(name);
    SDLTest_AssertPass("Call to SDL_GetScancodeFromName(NULL)");
    SDLTest_AssertCheck(scancode == SDL_SCANCODE_UNKNOWN, "Validate return value from SDL_GetScancodeFromName, expected: %i, got: %i", SDL_SCANCODE_UNKNOWN, scancode);
    _checkInvalidNameError();

     
    name = NULL;
    scancode = SDL_GetScancodeFromName(name);
    SDLTest_AssertPass("Call to SDL_GetScancodeFromName(NULL)");
    SDLTest_AssertCheck(scancode == SDL_SCANCODE_UNKNOWN, "Validate return value from SDL_GetScancodeFromName, expected: %i, got: %i", SDL_SCANCODE_UNKNOWN, scancode);
    _checkInvalidNameError();

    return TEST_COMPLETED;
}

 

 
static const SDLTest_TestCaseReference keyboardTest1 = {
    (SDLTest_TestCaseFp)keyboard_getKeyboardState, "keyboard_getKeyboardState", "Check call to SDL_GetKeyboardState with and without numkeys reference", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest2 = {
    (SDLTest_TestCaseFp)keyboard_getKeyboardFocus, "keyboard_getKeyboardFocus", "Check call to SDL_GetKeyboardFocus", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest3 = {
    (SDLTest_TestCaseFp)keyboard_getKeyFromName, "keyboard_getKeyFromName", "Check call to SDL_GetKeyFromName for known, unknown and invalid name", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest4 = {
    (SDLTest_TestCaseFp)keyboard_getKeyFromScancode, "keyboard_getKeyFromScancode", "Check call to SDL_GetKeyFromScancode", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest5 = {
    (SDLTest_TestCaseFp)keyboard_getKeyName, "keyboard_getKeyName", "Check call to SDL_GetKeyName", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest6 = {
    (SDLTest_TestCaseFp)keyboard_getSetModState, "keyboard_getSetModState", "Check call to SDL_GetModState and SDL_SetModState", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest7 = {
    (SDLTest_TestCaseFp)keyboard_startStopTextInput, "keyboard_startStopTextInput", "Check call to SDL_StartTextInput and SDL_StopTextInput", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest8 = {
    (SDLTest_TestCaseFp)keyboard_setTextInputRect, "keyboard_setTextInputRect", "Check call to SDL_SetTextInputRect", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest9 = {
    (SDLTest_TestCaseFp)keyboard_setTextInputRectNegative, "keyboard_setTextInputRectNegative", "Check call to SDL_SetTextInputRect with invalid data", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest10 = {
    (SDLTest_TestCaseFp)keyboard_getScancodeFromKey, "keyboard_getScancodeFromKey", "Check call to SDL_GetScancodeFromKey", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest11 = {
    (SDLTest_TestCaseFp)keyboard_getScancodeFromName, "keyboard_getScancodeFromName", "Check call to SDL_GetScancodeFromName", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest12 = {
    (SDLTest_TestCaseFp)keyboard_getScancodeFromNameNegative, "keyboard_getScancodeFromNameNegative", "Check call to SDL_GetScancodeFromName with invalid data", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest13 = {
    (SDLTest_TestCaseFp)keyboard_getKeyNameNegative, "keyboard_getKeyNameNegative", "Check call to SDL_GetKeyName with invalid data", TEST_ENABLED
};

static const SDLTest_TestCaseReference keyboardTest14 = {
    (SDLTest_TestCaseFp)keyboard_getScancodeNameNegative, "keyboard_getScancodeNameNegative", "Check call to SDL_GetScancodeName with invalid data", TEST_ENABLED
};

 
static const SDLTest_TestCaseReference *keyboardTests[] = {
    &keyboardTest1, &keyboardTest2, &keyboardTest3, &keyboardTest4, &keyboardTest5, &keyboardTest6,
    &keyboardTest7, &keyboardTest8, &keyboardTest9, &keyboardTest10, &keyboardTest11, &keyboardTest12,
    &keyboardTest13, &keyboardTest14, NULL
};

 
SDLTest_TestSuiteReference keyboardTestSuite = {
    "Keyboard",
    NULL,
    keyboardTests,
    NULL
};
