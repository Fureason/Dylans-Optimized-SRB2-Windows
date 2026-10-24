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

   
                        
  
                                                
   

#ifndef SDL_keyboard_h_
#define SDL_keyboard_h_

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_keycode.h"
#include "SDL_video.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

   
                                                        
  
                                                                                            
   
typedef struct SDL_Keysym
{
    SDL_Scancode scancode;       
    SDL_Keycode sym;             
    Uint16 mod;                  
    Uint32 unused;
} SDL_Keysym;

 

   
                                                       
  
                                           
  
                                                     
   
extern DECLSPEC SDL_Window * SDLCALL SDL_GetKeyboardFocus(void);

   
                                                       
  
                                                                         
                                                                             
              
  
                                                                              
                                                                           
                       
  
                                                  
  
                                                                       
                                                                            
                                                                   
                                
  
                                                                       
                  
  
                                                                        
                                                
  
                                                     
  
                     
                        
   
extern DECLSPEC const Uint8 *SDLCALL SDL_GetKeyboardState(int *numkeys);

   
                                  
  
                                                                  
  
                                                      
  
                           
   
extern DECLSPEC void SDLCALL SDL_ResetKeyboard(void);

   
                                                       
  
                                                                          
                                   
  
                                                     
  
                           
                      
   
extern DECLSPEC SDL_Keymod SDLCALL SDL_GetModState(void);

   
                                                       
  
                                                                           
                                                                             
                                                                           
                     
  
                                                                            
               
  
                                                          
  
                                                     
  
                      
   
extern DECLSPEC void SDLCALL SDL_SetModState(SDL_Keymod modstate);

   
                                                                        
                           
  
                               
  
                                                    
                                                                       
  
                                                     
  
                     
                             
   
extern DECLSPEC SDL_Keycode SDLCALL SDL_GetKeyFromScancode(SDL_Scancode scancode);

   
                                                                        
                           
  
                                
  
                                              
                                                                       
  
                                                     
  
                             
                          
   
extern DECLSPEC SDL_Scancode SDLCALL SDL_GetScancodeFromKey(SDL_Keycode key);

   
                                            
  
                                
  
                                                                           
                                                                            
                                                            
                                                                           
                                                             
                                                                            
                                                                          
                         
  
                                                    
                                                                           
                                                                   
  
                                                     
  
                             
                              
   
extern DECLSPEC const char *SDLCALL SDL_GetScancodeName(SDL_Scancode scancode);

   
                                             
  
                                               
                                                                          
                                                                 
  
                                                     
  
                         
                             
                          
   
extern DECLSPEC SDL_Scancode SDLCALL SDL_GetScancodeFromName(const char *name);

   
                                       
  
                                                
  
                                              
                                                                           
                                                                             
                                                                       
                                         
  
                                                     
  
                         
                             
                             
   
extern DECLSPEC const char *SDLCALL SDL_GetKeyName(SDL_Keycode key);

   
                                             
  
                                          
                                                                           
                                                
  
                                                     
  
                             
                     
                              
   
extern DECLSPEC SDL_Keycode SDLCALL SDL_GetKeyFromName(const char *name);

   
                                             
  
                                                                              
                                                                        
                                                                             
                                 
  
                                                                       
  
                                                     
  
                           
                        
   
extern DECLSPEC void SDLCALL SDL_StartTextInput(void);

   
                                                              
  
                                                                     
  
                                                     
  
                         
   
extern DECLSPEC SDL_bool SDLCALL SDL_IsTextInputActive(void);

   
                                        
  
                                                     
  
                         
   
extern DECLSPEC void SDLCALL SDL_StopTextInput(void);

   
                                                                      
  
                                                      
  
                         
                        
   
extern DECLSPEC void SDLCALL SDL_ClearComposition(void);

   
                                                                      
  
                                                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_IsTextInputShown(void);

   
                                                      
  
                                                                           
                                                                           
                                                                     
  
                                                                          
                                                                            
                
  
                                                                           
                                     
  
                                                     
  
                         
   
extern DECLSPEC void SDLCALL SDL_SetTextInputRect(const SDL_Rect *rect);

   
                                                          
  
                                                                        
                             
  
                                                     
  
                         
                                
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasScreenKeyboardSupport(void);

   
                                                               
  
                                                                       
                                                                     
  
                                                     
  
                                   
   
extern DECLSPEC SDL_bool SDLCALL SDL_IsScreenKeyboardShown(SDL_Window *window);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
