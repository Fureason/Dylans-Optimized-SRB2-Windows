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

   
                           
  
                                        
  
                                                                           
   

 

#ifndef SDL_test_common_h_
#define SDL_test_common_h_

#include "SDL.h"

#if defined(__PSP__)
#define DEFAULT_WINDOW_WIDTH  480
#define DEFAULT_WINDOW_HEIGHT 272
#elif defined(__VITA__)
#define DEFAULT_WINDOW_WIDTH  960
#define DEFAULT_WINDOW_HEIGHT 544
#else
#define DEFAULT_WINDOW_WIDTH  640
#define DEFAULT_WINDOW_HEIGHT 480
#endif

#define VERBOSE_VIDEO   0x00000001
#define VERBOSE_MODES   0x00000002
#define VERBOSE_RENDER  0x00000004
#define VERBOSE_EVENT   0x00000008
#define VERBOSE_AUDIO   0x00000010
#define VERBOSE_MOTION  0x00000020

typedef struct
{
     
    char **argv;
    Uint32 flags;
    Uint32 verbose;

     
    const char *videodriver;
    int display;
    const char *window_title;
    const char *window_icon;
    Uint32 window_flags;
    SDL_bool flash_on_focus_loss;
    int window_x;
    int window_y;
    int window_w;
    int window_h;
    int window_minW;
    int window_minH;
    int window_maxW;
    int window_maxH;
    int logical_w;
    int logical_h;
    float scale;
    int depth;
    int refresh_rate;
    int num_windows;
    SDL_Window **windows;

     
    const char *renderdriver;
    Uint32 render_flags;
    SDL_bool skip_renderer;
    SDL_Renderer **renderers;
    SDL_Texture **targets;

     
    const char *audiodriver;
    SDL_AudioSpec audiospec;

     
    int gl_red_size;
    int gl_green_size;
    int gl_blue_size;
    int gl_alpha_size;
    int gl_buffer_size;
    int gl_depth_size;
    int gl_stencil_size;
    int gl_double_buffer;
    int gl_accum_red_size;
    int gl_accum_green_size;
    int gl_accum_blue_size;
    int gl_accum_alpha_size;
    int gl_stereo;
    int gl_multisamplebuffers;
    int gl_multisamplesamples;
    int gl_retained_backing;
    int gl_accelerated;
    int gl_major_version;
    int gl_minor_version;
    int gl_debug;
    int gl_profile_mask;

     
    SDL_Rect confine;

} SDLTest_CommonState;

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

 

   
                                                                
  
                                               
                                                                                                     
  
                                                  
   
SDLTest_CommonState *SDLTest_CommonCreateState(char **argv, Uint32 flags);

   
                                      
  
                                                                      
                                                               
  
                                                                                                                    
   
int SDLTest_CommonArg(SDLTest_CommonState * state, int index);


   
                                       
  
                                                                           
                                                                         
                                                                           
                                
  
                                                                        
                                                    
                                                                                                                     
   
void SDLTest_CommonLogUsage(SDLTest_CommonState * state, const char *argv0, const char **options);

   
                                          
  
                                                                            
                                                                         
                                                                          
                                                             
  
                                                                      
                                           
   
const char *SDLTest_CommonUsage(SDLTest_CommonState * state);

   
                           
  
                                                                      
  
                                                                 
   
SDL_bool SDLTest_CommonInit(SDLTest_CommonState * state);

   
                                                                            
  
                                                                      
                                            
                                            
  
                                                         
   
SDL_bool SDLTest_CommonDefaultArgs(SDLTest_CommonState * state, const int argc, char **argv);

   
                                                
  
                                                            
                                    
                                           
  
   
void SDLTest_CommonEvent(SDLTest_CommonState * state, SDL_Event * event, int *done);

   
                            
  
                                                            
  
   
void SDLTest_CommonQuit(SDLTest_CommonState * state);

   
                                                                                  
  
                                           
                                                                  
                                                                                
  
   
void SDLTest_CommonDrawWindowInfo(SDL_Renderer * renderer, SDL_Window * window, int * usedHeight);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
