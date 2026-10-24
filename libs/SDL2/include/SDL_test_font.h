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

   
                         
  
                                        
  
                                                                           
   

#ifndef SDL_test_font_h_
#define SDL_test_font_h_

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

 

#define FONT_CHARACTER_SIZE 8
#define FONT_LINE_HEIGHT    (FONT_CHARACTER_SIZE + 2)

   
                                                   
  
                                            
                                                                        
                                                                        
                                   
  
                                         
   
int SDLTest_DrawCharacter(SDL_Renderer *renderer, int x, int y, Uint32 c);

   
                                                         
  
                                                                                               
  
                                            
                                                                     
                                                                     
                                
  
                                         
   
int SDLTest_DrawString(SDL_Renderer *renderer, int x, int y, const char *s);

   
                                               
   
typedef struct SDLTest_TextWindow
{
    SDL_Rect rect;
    int current;
    int numlines;
    char **lines;
} SDLTest_TextWindow;

   
                                                 
  
                                                                     
                                                                     
                                                        
                                                         
  
                                                
  
                                                      
   
SDLTest_TextWindow *SDLTest_TextWindowCreate(int x, int y, int w, int h);

   
                                                  
  
                                                                  
  
                                         
                                                   
  
                                                      
   
void SDLTest_TextWindowDisplay(SDLTest_TextWindow *textwin, SDL_Renderer *renderer);

   
                                                      
  
                                                                                  
                                                                                   
                                                                             
  
                                         
                                             
                                                                                   
  
                                                      
   
void SDLTest_TextWindowAddText(SDLTest_TextWindow *textwin, SDL_PRINTF_FORMAT_STRING const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);

   
                                                      
  
                                                                                  
                                                                                   
                                                                             
  
                                         
                                             
                                                                     
  
                                                      
   
void SDLTest_TextWindowAddTextWithLength(SDLTest_TextWindow *textwin, const char *text, size_t len);

   
                                                            
  
                                         
  
                                                      
   
void SDLTest_TextWindowClear(SDLTest_TextWindow *textwin);

   
                                                                           
  
                                         
  
                                                      
   
void SDLTest_TextWindowDestroy(SDLTest_TextWindow *textwin);

   
                                                           
   
void SDLTest_CleanupTextDrawing(void);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
