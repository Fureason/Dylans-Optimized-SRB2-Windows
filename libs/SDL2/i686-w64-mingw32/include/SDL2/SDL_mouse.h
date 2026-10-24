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

   
                     
  
                                              
   

#ifndef SDL_mouse_h_
#define SDL_mouse_h_

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_video.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_Cursor SDL_Cursor;    

   
                                                    
   
typedef enum
{
    SDL_SYSTEM_CURSOR_ARROW,      
    SDL_SYSTEM_CURSOR_IBEAM,      
    SDL_SYSTEM_CURSOR_WAIT,       
    SDL_SYSTEM_CURSOR_CROSSHAIR,  
    SDL_SYSTEM_CURSOR_WAITARROW,  
    SDL_SYSTEM_CURSOR_SIZENWSE,   
    SDL_SYSTEM_CURSOR_SIZENESW,   
    SDL_SYSTEM_CURSOR_SIZEWE,     
    SDL_SYSTEM_CURSOR_SIZENS,     
    SDL_SYSTEM_CURSOR_SIZEALL,    
    SDL_SYSTEM_CURSOR_NO,         
    SDL_SYSTEM_CURSOR_HAND,       
    SDL_NUM_SYSTEM_CURSORS
} SDL_SystemCursor;

   
                                                     
   
typedef enum
{
    SDL_MOUSEWHEEL_NORMAL,     
    SDL_MOUSEWHEEL_FLIPPED     
} SDL_MouseWheelDirection;

 

   
                                                  
  
                                        
  
                                                     
   
extern DECLSPEC SDL_Window * SDLCALL SDL_GetMouseFocus(void);

   
                                           
  
                                                                         
                                                                            
                                                                              
                                                                            
                     
  
                                                                         
                        
                                                                         
                        
                                                                
  
                                                     
  
                              
                                
                     
   
extern DECLSPEC Uint32 SDLCALL SDL_GetMouseState(int *x, int *y);

   
                                                                 
  
                                                                           
                                                                              
                                                                              
                                                                         
                                                                           
                                            
  
                                                                             
                                                                             
                                                                           
                                                                        
                                                                              
  
                                                                              
                
                                                                              
                
                                                                           
                                     
  
                                                     
  
                       
   
extern DECLSPEC Uint32 SDLCALL SDL_GetGlobalMouseState(int *x, int *y);

   
                                            
  
                                                                         
                                                                            
                                                                              
                                                                           
                                                                 
  
                                                                             
                                                                             
                                                                 
  
                                                     
  
                        
   
extern DECLSPEC Uint32 SDLCALL SDL_GetRelativeMouseState(int *x, int *y);

   
                                                                 
  
                                                                       
                                                                           
                                                                
  
                                                                            
                                                 
  
                                                                           
                            
                                              
                                              
  
                                                     
  
                          
   
extern DECLSPEC void SDLCALL SDL_WarpMouseInWindow(SDL_Window * window,
                                                   int x, int y);

   
                                                               
  
                                                
  
                                                                       
            
  
                                                                            
                                                 
  
                            
                            
                                                                  
                                                
  
                                                     
  
                            
   
extern DECLSPEC int SDLCALL SDL_WarpMouseGlobal(int x, int y);

   
                           
  
                                                                       
                                                                        
                                                                        
  
                                                     
  
                                                                         
                                                                  
                                                
  
                                                               
  
                                                     
  
                               
   
extern DECLSPEC int SDLCALL SDL_SetRelativeMouseMode(SDL_bool enabled);

   
                                                              
  
                                                                              
                                                                        
                                                                          
                                                                      
                             
  
                                                                          
                                                                             
                                                                           
                                                                              
                                                                            
                                                                             
                                                                            
                 
  
                                                                        
                                                                        
                                                                              
                                                                            
                                          
  
                                                               
                                       
  
                                                                            
                                                                           
                                                                            
                                                                             
                                                                          
                                                                             
                                                        
                                              
  
                                                                     
                                                                             
                        
  
                                                     
  
                              
   
extern DECLSPEC int SDLCALL SDL_CaptureMouse(SDL_bool enabled);

   
                                                
  
                                                                        
  
                                                     
  
                               
   
extern DECLSPEC SDL_bool SDLCALL SDL_GetRelativeMouseMode(void);

   
                                                                            
  
                                                         
  
                                                       
  
                                                                       
  
                          
                          
                                
                                                              
  
                                                                          
  
                                                                    
                                                                            
                                                                             
                                  
  
                                                                      
                                                                 
  
                                                           
                                                          
                                   
                                    
                                                                          
                                                     
                                                                          
                                                     
                                                                            
                                                              
  
                                                     
  
                     
                    
                     
   
extern DECLSPEC SDL_Cursor *SDLCALL SDL_CreateCursor(const Uint8 * data,
                                                     const Uint8 * mask,
                                                     int w, int h, int hot_x,
                                                     int hot_y);

   
                         
  
                                                                        
                                                     
                                                     
                                                                             
                                 
  
                                                     
  
                       
                     
   
extern DECLSPEC SDL_Cursor *SDLCALL SDL_CreateColorCursor(SDL_Surface *surface,
                                                          int hot_x,
                                                          int hot_y);

   
                          
  
                                           
                                                                           
                             
  
                                                     
  
                     
   
extern DECLSPEC SDL_Cursor *SDLCALL SDL_CreateSystemCursor(SDL_SystemCursor id);

   
                         
  
                                                                              
                                                                             
                                                                          
                                  
  
                                        
  
                                                     
  
                       
                    
                     
   
extern DECLSPEC void SDLCALL SDL_SetCursor(SDL_Cursor * cursor);

   
                         
  
                                                                              
                                                                         
  
                                                           
  
                                                     
  
                    
   
extern DECLSPEC SDL_Cursor *SDLCALL SDL_GetCursor(void);

   
                          
  
                                                                          
                 
  
                                                             
  
                                                     
  
                             
   
extern DECLSPEC SDL_Cursor *SDLCALL SDL_GetDefaultCursor(void);

   
                                    
  
                                                                              
                                                       
  
                                   
  
                                                     
  
                            
                       
                             
   
extern DECLSPEC void SDLCALL SDL_FreeCursor(SDL_Cursor * cursor);

   
                                             
  
                                                                              
                                                          
  
                                                                  
                                                                      
  
                                                                           
                                                                            
                                                                        
                                                                       
                                                
  
                                                     
  
                       
                    
   
extern DECLSPEC int SDLCALL SDL_ShowCursor(int toggle);

   
                                                      
  
                                 
                                   
                                  
   
#define SDL_BUTTON(X)       (1 << ((X)-1))
#define SDL_BUTTON_LEFT     1
#define SDL_BUTTON_MIDDLE   2
#define SDL_BUTTON_RIGHT    3
#define SDL_BUTTON_X1       4
#define SDL_BUTTON_X2       5
#define SDL_BUTTON_LMASK    SDL_BUTTON(SDL_BUTTON_LEFT)
#define SDL_BUTTON_MMASK    SDL_BUTTON(SDL_BUTTON_MIDDLE)
#define SDL_BUTTON_RMASK    SDL_BUTTON(SDL_BUTTON_RIGHT)
#define SDL_BUTTON_X1MASK   SDL_BUTTON(SDL_BUTTON_X1)
#define SDL_BUTTON_X2MASK   SDL_BUTTON(SDL_BUTTON_X2)

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
