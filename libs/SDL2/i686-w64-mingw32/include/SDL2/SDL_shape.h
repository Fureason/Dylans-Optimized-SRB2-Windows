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

#ifndef SDL_shape_h_
#define SDL_shape_h_

#include "SDL_stdinc.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_surface.h"
#include "SDL_video.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

                     
  
                                         
   

#define SDL_NONSHAPEABLE_WINDOW -1
#define SDL_INVALID_SHAPE_ARGUMENT -2
#define SDL_WINDOW_LACKS_SHAPE -3

   
                                                                              
             
  
                                                           
                                                                      
                                      
                                                                      
                                      
                                    
                                     
                                                                              
                                                          
                                                                
                                                               
                                                                              
                                                            
                                                                 
  
                                                     
  
                        
   
extern DECLSPEC SDL_Window * SDLCALL SDL_CreateShapedWindow(const char *title,unsigned int x,unsigned int y,unsigned int w,unsigned int h,Uint32 flags);

   
                                                      
  
                                                      
                                                                              
                                          
  
                                                     
  
                             
   
extern DECLSPEC SDL_bool SDLCALL SDL_IsShapedWindow(const SDL_Window *window);

 
typedef enum {
     
    ShapeModeDefault,
     
    ShapeModeBinarizeAlpha,
     
    ShapeModeReverseBinarizeAlpha,
     
    ShapeModeColorKey
} WindowShapeMode;

#define SDL_SHAPEMODEALPHA(mode) (mode == ShapeModeDefault || mode == ShapeModeBinarizeAlpha || mode == ShapeModeReverseBinarizeAlpha)

 
typedef union {
     
    Uint8 binarizationCutoff;
    SDL_Color colorKey;
} SDL_WindowShapeParams;

 
typedef struct SDL_WindowShapeMode {
     
    WindowShapeMode mode;
     
    SDL_WindowShapeParams parameters;
} SDL_WindowShapeMode;

   
                                                   
  
                                                                  
                                                                    
                                                                 
                                                                       
                                                                            
                                               
  
                                                     
  
                          
                              
   
extern DECLSPEC int SDLCALL SDL_SetWindowShape(SDL_Window *window,SDL_Surface *shape,SDL_WindowShapeMode *shape_mode);

   
                                               
  
                                                                        
                                                                            
                                                    
                                                                             
                                                         
                                                                          
                                                                         
                                                      
  
                                                     
  
                          
                         
   
extern DECLSPEC int SDLCALL SDL_GetShapedWindowMode(SDL_Window *window,SDL_WindowShapeMode *shape_mode);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  
