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

   
                      
  
                                               
  
                                             
                             
                            
                           
                        
  
                                                                      
  
                                                                          
                                                                         
                                                              
  
                                                                    
                                                                     
                                                                    
                                
  
                                                        
                                                                             
   

#ifndef SDL_render_h_
#define SDL_render_h_

#include "SDL_stdinc.h"
#include "SDL_rect.h"
#include "SDL_video.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

   
                                               
   
typedef enum
{
    SDL_RENDERER_SOFTWARE = 0x00000001,          
    SDL_RENDERER_ACCELERATED = 0x00000002,                                     
                                                                    
    SDL_RENDERER_PRESENTVSYNC = 0x00000004,                                 
                                                                             
    SDL_RENDERER_TARGETTEXTURE = 0x00000008                               
                                                                            
} SDL_RendererFlags;

   
                                                                 
   
typedef struct SDL_RendererInfo
{
    const char *name;            
    Uint32 flags;                
    Uint32 num_texture_formats;  
    Uint32 texture_formats[16];  
    int max_texture_width;       
    int max_texture_height;      
} SDL_RendererInfo;

   
                    
   
typedef struct SDL_Vertex
{
    SDL_FPoint position;         
    SDL_Color  color;            
    SDL_FPoint tex_coord;        
} SDL_Vertex;

   
                                  
   
typedef enum
{
    SDL_ScaleModeNearest,  
    SDL_ScaleModeLinear,   
    SDL_ScaleModeBest      
} SDL_ScaleMode;

   
                                            
   
typedef enum
{
    SDL_TEXTUREACCESS_STATIC,     
    SDL_TEXTUREACCESS_STREAMING,  
    SDL_TEXTUREACCESS_TARGET      
} SDL_TextureAccess;

   
                                                           
   
typedef enum
{
    SDL_TEXTUREMODULATE_NONE = 0x00000000,      
    SDL_TEXTUREMODULATE_COLOR = 0x00000001,     
    SDL_TEXTUREMODULATE_ALPHA = 0x00000002      
} SDL_TextureModulate;

   
                                      
   
typedef enum
{
    SDL_FLIP_NONE = 0x00000000,      
    SDL_FLIP_HORIZONTAL = 0x00000001,     
    SDL_FLIP_VERTICAL = 0x00000002      
} SDL_RendererFlip;

   
                                           
   
struct SDL_Renderer;
typedef struct SDL_Renderer SDL_Renderer;

   
                                                            
   
struct SDL_Texture;
typedef struct SDL_Texture SDL_Texture;

 

   
                                                                            
  
                                                                      
                                                                           
                                                                  
  
                                                                
  
                                                                              
                                                
  
                                                     
  
                         
                              
   
extern DECLSPEC int SDLCALL SDL_GetNumRenderDrivers(void);

   
                                                                         
  
                                                                  
                                                                             
                                   
                                                                  
                                                
  
                                                     
  
                         
                              
   
extern DECLSPEC int SDLCALL SDL_GetRenderDriverInfo(int index,
                                                    SDL_RendererInfo * info);

   
                                        
  
                                       
                                         
                                                               
                                          
                                                                   
                                                                       
                                                                      
                        
  
                                                     
  
                         
                       
   
extern DECLSPEC int SDLCALL SDL_CreateWindowAndRenderer(
                                int width, int height, Uint32 window_flags,
                                SDL_Window **window, SDL_Renderer **renderer);


   
                                              
  
                                                        
                                                                         
                                                                       
                                                                 
                                                                         
                                                
  
                                                     
  
                                 
                          
                              
                          
   
extern DECLSPEC SDL_Renderer * SDLCALL SDL_CreateRenderer(SDL_Window * window,
                                               int index, Uint32 flags);

   
                                                        
  
                                                          
                                                                           
                                                                       
                                                              
  
                                                                          
                                   
                                                                         
                                                
  
                                                     
  
                         
                               
                          
   
extern DECLSPEC SDL_Renderer * SDLCALL SDL_CreateSoftwareRenderer(SDL_Surface * surface);

   
                                             
  
                                    
                                                                     
                                                
  
                                                     
  
                         
   
extern DECLSPEC SDL_Renderer * SDLCALL SDL_GetRenderer(SDL_Window * window);

   
                                             
  
                                        
                                                                             
                             
  
                                                      
   
extern DECLSPEC SDL_Window * SDLCALL SDL_RenderGetWindow(SDL_Renderer *renderer);

   
                                             
  
                                        
                                                                              
                               
                                                                  
                                                
  
                                                     
  
                         
   
extern DECLSPEC int SDLCALL SDL_GetRendererInfo(SDL_Renderer * renderer,
                                                SDL_RendererInfo * info);

   
                                                        
  
                                                                           
                                                                           
                                                                
  
                                        
                                        
                                         
                                                                  
                                                
  
                                                     
  
                      
   
extern DECLSPEC int SDLCALL SDL_GetRendererOutputSize(SDL_Renderer * renderer,
                                                      int *w, int *h);

   
                                            
  
                                                    
                                                               
  
                                        
                                                                    
                                                                  
                                              
                                               
                                                                            
                                                                          
                                                                        
  
                                                     
  
                                   
                         
                       
                        
   
extern DECLSPEC SDL_Texture * SDLCALL SDL_CreateTexture(SDL_Renderer * renderer,
                                                        Uint32 format,
                                                        int access, int w,
                                                        int h);

   
                                             
  
                                                         
  
                                                        
                              
  
                                                                          
                                                                             
               
  
                                        
                                                                              
                             
                                                                           
                             
  
                                                     
  
                        
                         
                       
   
extern DECLSPEC SDL_Texture * SDLCALL SDL_CreateTextureFromSurface(SDL_Renderer * renderer, SDL_Surface * surface);

   
                                     
  
                                      
                                                                            
                                                                            
                                                                              
                                                                
                                                                          
                                                                            
                                                         
                                                                             
                                                                    
                                                                              
                                                                    
                                                                  
                                                
  
                                                     
  
                        
   
extern DECLSPEC int SDLCALL SDL_QueryTexture(SDL_Texture * texture,
                                             Uint32 * format, int *access,
                                             int *w, int *h);

   
                                                                        
  
                                                                             
                                                                       
                     
  
                                
  
                                                                              
                                        
  
                                       
                                                               
                                                                 
                                                                
                                                                  
                                                
  
                                                     
  
                             
                             
   
extern DECLSPEC int SDLCALL SDL_SetTextureColorMod(SDL_Texture * texture,
                                                   Uint8 r, Uint8 g, Uint8 b);


   
                                                                         
  
                                      
                                                                
                                                                  
                                                                 
                                                                  
                                                
  
                                                     
  
                             
                             
   
extern DECLSPEC int SDLCALL SDL_GetTextureColorMod(SDL_Texture * texture,
                                                   Uint8 * r, Uint8 * g,
                                                   Uint8 * b);

   
                                                                        
  
                                                                            
                                                                             
  
                                
  
                                                                              
                                        
  
                                       
                                                                      
                                                                  
                                                
  
                                                     
  
                             
                             
   
extern DECLSPEC int SDLCALL SDL_SetTextureAlphaMod(SDL_Texture * texture,
                                                   Uint8 alpha);

   
                                                                         
  
                                      
                                                                
                                                                  
                                                
  
                                                     
  
                             
                             
   
extern DECLSPEC int SDLCALL SDL_GetTextureAlphaMod(SDL_Texture * texture,
                                                   Uint8 * alpha);

   
                                                              
  
                                                                           
                                
  
                                       
                                                                 
                                                                  
                                                
  
                                                     
  
                              
                     
   
extern DECLSPEC int SDLCALL SDL_SetTextureBlendMode(SDL_Texture * texture,
                                                    SDL_BlendMode blendMode);

   
                                                       
  
                                      
                                                                      
                                                                  
                                                
  
                                                     
  
                              
   
extern DECLSPEC int SDLCALL SDL_GetTextureBlendMode(SDL_Texture * texture,
                                                    SDL_BlendMode *blendMode);

   
                                                        
  
                                                                            
  
                                        
                                                                 
                                                            
  
                                                      
  
                              
   
extern DECLSPEC int SDLCALL SDL_SetTextureScaleMode(SDL_Texture * texture,
                                                    SDL_ScaleMode scaleMode);

   
                                                        
  
                                       
                                                                    
                                                           
  
                                                      
  
                              
   
extern DECLSPEC int SDLCALL SDL_GetTextureScaleMode(SDL_Texture * texture,
                                                    SDL_ScaleMode *scaleMode);

   
                                                     
  
                                        
                                                             
                                                            
  
                                                      
  
                             
   
extern DECLSPEC int SDLCALL SDL_SetTextureUserData(SDL_Texture * texture,
                                                   void *userdata);

   
                                                           
  
                                       
                                                                             
                     
  
                                                      
  
                             
   
extern DECLSPEC void * SDLCALL SDL_GetTextureUserData(SDL_Texture * texture);

   
                                                          
  
                                                                 
                                                               
  
                                                                             
                       
  
                                                                            
                                                                           
                                                                          
                                                                             
  
                                       
                                                                             
                                           
                                                                
                                                                             
                             
                                                                  
                                                
  
                                                     
  
                        
                      
                        
   
extern DECLSPEC int SDLCALL SDL_UpdateTexture(SDL_Texture * texture,
                                              const SDL_Rect * rect,
                                              const void *pixels, int pitch);

   
                                                                         
        
  
                                                                             
                                                                      
                                                  
  
                                       
                                                                         
                                        
                                                   
                                                                         
                      
                                                   
                                                                         
                      
                                                   
                                                                         
                      
                                                                
                                                
  
                                                     
  
                        
   
extern DECLSPEC int SDLCALL SDL_UpdateYUVTexture(SDL_Texture * texture,
                                                 const SDL_Rect * rect,
                                                 const Uint8 *Yplane, int Ypitch,
                                                 const Uint8 *Uplane, int Upitch,
                                                 const Uint8 *Vplane, int Vpitch);

   
                                                                           
  
                                                                             
                                                                              
                                        
  
                                       
                                                                         
                                         
                                                    
                                                                         
                       
                                                      
                                                                           
                        
                                                           
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_UpdateNVTexture(SDL_Texture * texture,
                                                 const SDL_Rect * rect,
                                                 const Uint8 *Yplane, int Ypitch,
                                                 const Uint8 *UVplane, int UVpitch);

   
                                                                 
  
                                                                              
                                                                           
                                                                    
                     
  
                                                                      
           
  
                                                                        
                                               
                                                                              
                                              
                                                                       
                                                        
                                                                          
                                                       
                                                                             
                                                                       
                                                
  
                                                     
  
                        
   
extern DECLSPEC int SDLCALL SDL_LockTexture(SDL_Texture * texture,
                                            const SDL_Rect * rect,
                                            void **pixels, int *pitch);

   
                                                                            
                       
  
                                                                            
                                 
  
                                                                              
                                                                           
                                                                    
                     
  
                                                                      
           
  
                                                                             
                                                          
  
                                                                        
                                               
                                                                            
                                                      
                                                                        
                             
                                                                              
                                              
  
                                                      
  
                      
                        
   
extern DECLSPEC int SDLCALL SDL_LockTextureToSurface(SDL_Texture *texture,
                                            const SDL_Rect *rect,
                                            SDL_Surface **surface);

   
                                                                      
  
                                                                    
                                                                              
                                                                             
                                                                              
  
                                                                             
                                                        
  
                                                       
  
                                                     
  
                      
   
extern DECLSPEC void SDLCALL SDL_UnlockTexture(SDL_Texture * texture);

   
                                                                   
  
                                                    
                                                      
  
                                                     
  
                          
   
extern DECLSPEC SDL_bool SDLCALL SDL_RenderTargetSupported(SDL_Renderer *renderer);

   
                                                 
  
                                                   
                                                                              
                                
  
                                                                              
                                                                           
                                  
  
                                        
                                                                      
                                                                           
                                              
                                                                  
                                                
  
                                                     
  
                          
   
extern DECLSPEC int SDLCALL SDL_SetRenderTarget(SDL_Renderer *renderer,
                                                SDL_Texture *texture);

   
                                 
  
                                                                              
                               
  
                                        
                                                                            
  
                                                     
  
                          
   
extern DECLSPEC SDL_Texture * SDLCALL SDL_GetRenderTarget(SDL_Renderer *renderer);

   
                                                     
  
                                                                             
                                                                    
                                                                           
                                                                         
  
                                                                          
                                                                        
                                                                        
                                          
  
                                                                           
                                                                   
  
                                                                  
                                               
                                                
                                                                  
                                                
  
                                                     
  
                               
   
extern DECLSPEC int SDLCALL SDL_RenderSetLogicalSize(SDL_Renderer * renderer, int w, int h);

   
                                                   
  
                                                                           
                                                                             
                                                                           
                    
  
                                                                             
                                                        
  
                                      
                                              
                                               
  
                                                     
  
                               
   
extern DECLSPEC void SDLCALL SDL_RenderGetLogicalSize(SDL_Renderer * renderer, int *w, int *h);

   
                                                                            
  
                                                                            
                                                                             
                                              
  
                                                                       
                                                                    
                                                                  
                                                
  
                                                     
  
                                
                               
   
extern DECLSPEC int SDLCALL SDL_RenderSetIntegerScale(SDL_Renderer * renderer,
                                                      SDL_bool enable);

   
                                                                              
  
                                                                            
                                                                            
                                                              
  
                                                     
  
                                
   
extern DECLSPEC SDL_bool SDLCALL SDL_RenderGetIntegerScale(SDL_Renderer * renderer);

   
                                                            
  
                                                                           
               
  
                                        
                                                                            
                                                       
                                                                  
                                                
  
                                                     
  
                            
   
extern DECLSPEC int SDLCALL SDL_RenderSetViewport(SDL_Renderer * renderer,
                                                  const SDL_Rect * rect);

   
                                               
  
                                        
                                                                            
  
                                                     
  
                            
   
extern DECLSPEC void SDLCALL SDL_RenderGetViewport(SDL_Renderer * renderer,
                                                   SDL_Rect * rect);

   
                                                                
  
                                                                           
                      
                                                                            
                                                        
                                                                  
                                                
  
                                                     
  
                            
                              
   
extern DECLSPEC int SDLCALL SDL_RenderSetClipRect(SDL_Renderer * renderer,
                                                  const SDL_Rect * rect);

   
                                                 
  
                                                                            
                          
                                                                             
                                                            
  
                                                     
  
                              
                            
   
extern DECLSPEC void SDLCALL SDL_RenderGetClipRect(SDL_Renderer * renderer,
                                                   SDL_Rect * rect);

   
                                                         
  
                                                                       
                                                                     
                                                
  
                                                     
  
                            
                            
   
extern DECLSPEC SDL_bool SDLCALL SDL_RenderIsClipEnabled(SDL_Renderer * renderer);


   
                                                             
  
                                                                            
                                                                              
                            
  
                                                                              
                                                                            
                           
  
                                      
                                              
                                            
                                                                  
                                                
  
                                                     
  
                         
                               
   
extern DECLSPEC int SDLCALL SDL_RenderSetScale(SDL_Renderer * renderer,
                                               float scaleX, float scaleY);

   
                                                
  
                                                                          
                                                                       
                                                                     
  
                                                     
  
                         
   
extern DECLSPEC void SDLCALL SDL_RenderGetScale(SDL_Renderer * renderer,
                                               float *scaleX, float *scaleY);

   
                                                                              
                   
  
                                                                              
                                
  
                                                                            
                             
                                                     
                                                     
                                                                   
                                                                   
  
                                                      
  
                         
                         
                               
                               
   
extern DECLSPEC void SDLCALL SDL_RenderWindowToLogical(SDL_Renderer * renderer, 
                                                            int windowX, int windowY, 
                                                            float *logicalX, float *logicalY);
                                                  

   
                                                                            
                     
  
                                                                              
                                
  
                                                                           
                             
                                           
                                           
                                                                             
                                                                             
  
                                                      
  
                         
                         
                               
                               
   
extern DECLSPEC void SDLCALL SDL_RenderLogicalToWindow(SDL_Renderer * renderer, 
                                                            float logicalX, float logicalY,
                                                            int *windowX, int *windowY);

   
                                                                    
  
                                                                              
                     
  
                                        
                                                              
                                                                
                                                               
                                                                         
                                                                       
                                                 
                                                                  
                                                
  
                                                     
  
                             
                      
                         
                          
                          
                           
                         
                          
                         
                          
   
extern DECLSPEC int SDLCALL SDL_SetRenderDrawColor(SDL_Renderer * renderer,
                                           Uint8 r, Uint8 g, Uint8 b,
                                           Uint8 a);

   
                                                                    
  
                                        
                                                                      
                            
                                                                        
                            
                                                                       
                            
                                                                        
                                                              
                                                                  
                                                
  
                                                     
  
                             
   
extern DECLSPEC int SDLCALL SDL_GetRenderDrawColor(SDL_Renderer * renderer,
                                           Uint8 * r, Uint8 * g, Uint8 * b,
                                           Uint8 * a);

   
                                                                  
  
                                                                            
  
                                        
                                                         
                                                                  
                                                
  
                                                     
  
                                 
                         
                          
                          
                           
                         
                          
                         
                          
   
extern DECLSPEC int SDLCALL SDL_SetRenderDrawBlendMode(SDL_Renderer * renderer,
                                                       SDL_BlendMode blendMode);

   
                                                  
  
                                        
                                                                      
                                                                  
                                                
  
                                                     
  
                                 
   
extern DECLSPEC int SDLCALL SDL_GetRenderDrawBlendMode(SDL_Renderer * renderer,
                                                       SDL_BlendMode *blendMode);

   
                                                             
  
                                                                              
                      
  
                                        
                                                                  
                                                
  
                                                     
  
                             
   
extern DECLSPEC int SDLCALL SDL_RenderClear(SDL_Renderer * renderer);

   
                                                
  
                                                                            
                                      
  
                                        
                                         
                                         
                                                                  
                                                
  
                                                     
  
                         
                          
                           
                         
                          
                         
                          
                        
                                 
                             
   
extern DECLSPEC int SDLCALL SDL_RenderDrawPoint(SDL_Renderer * renderer,
                                                int x, int y);

   
                                                        
  
                                        
                                                                              
                     
                                            
                                                                  
                                                
  
                                                     
  
                         
                          
                          
                         
                          
                         
                          
                        
                                 
                             
   
extern DECLSPEC int SDLCALL SDL_RenderDrawPoints(SDL_Renderer * renderer,
                                                 const SDL_Point * points,
                                                 int count);

   
                                               
  
                                                                              
                                                                        
  
                                        
                                                
                                                
                                              
                                              
                                                                  
                                                
  
                                                     
  
                          
                          
                           
                         
                          
                         
                          
                        
                                 
                             
   
extern DECLSPEC int SDLCALL SDL_RenderDrawLine(SDL_Renderer * renderer,
                                               int x1, int y1, int x2, int y2);

   
                                                                    
  
                                        
                                                                           
                          
                                                           
                                                                  
                                                
  
                                                     
  
                         
                          
                           
                         
                          
                         
                          
                        
                                 
                             
   
extern DECLSPEC int SDLCALL SDL_RenderDrawLines(SDL_Renderer * renderer,
                                                const SDL_Point * points,
                                                int count);

   
                                                    
  
                                        
                                                                           
                                                          
                                                                  
                                                
  
                                                     
  
                         
                          
                          
                           
                          
                         
                          
                        
                                 
                             
   
extern DECLSPEC int SDLCALL SDL_RenderDrawRect(SDL_Renderer * renderer,
                                               const SDL_Rect * rect);

   
                                                                  
  
                                        
                                                                              
                        
                                        
                                                                  
                                                
  
                                                     
  
                         
                          
                          
                           
                         
                         
                          
                        
                                 
                             
   
extern DECLSPEC int SDLCALL SDL_RenderDrawRects(SDL_Renderer * renderer,
                                                const SDL_Rect * rects,
                                                int count);

   
                                                                           
  
                                                                        
                                                                     
                                                    
  
                                        
                                                                            
                                                   
                                                                  
                                                
  
                                                     
  
                         
                          
                          
                           
                         
                          
                          
                        
                                 
                             
   
extern DECLSPEC int SDLCALL SDL_RenderFillRect(SDL_Renderer * renderer,
                                               const SDL_Rect * rect);

   
                                                                          
                 
  
                                        
                                                                              
                         
                                        
                                                                  
                                                
  
                                                     
  
                         
                          
                          
                           
                         
                          
                         
                        
   
extern DECLSPEC int SDLCALL SDL_RenderFillRects(SDL_Renderer * renderer,
                                                const SDL_Rect * rects,
                                                int count);

   
                                                                 
  
                                                                          
                                  
  
                                                                     
                            
  
                                                                     
                            
  
                                        
                                    
                                                                              
                                                                           
                                                                             
                                 
                                                                  
                                                
  
                                                     
  
                       
                             
                              
                             
   
extern DECLSPEC int SDLCALL SDL_RenderCopy(SDL_Renderer * renderer,
                                           SDL_Texture * texture,
                                           const SDL_Rect * srcrect,
                                           const SDL_Rect * dstrect);

   
                                                                        
                         
  
                                                                            
                                                                    
                                
  
                                                                          
                                  
  
                                                                     
                            
  
                                                                     
                            
  
                                        
                                    
                                                                              
                                                                           
                                  
                                                                            
                                                                        
                                                                       
                                                                        
                                                         
                                                                             
                                          
                                                                  
                                                
  
                                                     
  
                     
                             
                              
                             
   
extern DECLSPEC int SDLCALL SDL_RenderCopyEx(SDL_Renderer * renderer,
                                           SDL_Texture * texture,
                                           const SDL_Rect * srcrect,
                                           const SDL_Rect * dstrect,
                                           const double angle,
                                           const SDL_Point *center,
                                           const SDL_RendererFlip flip);


   
                                                                      
  
                                                          
                                          
                                          
                                       
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_RenderDrawPointF(SDL_Renderer * renderer,
                                                 float x, float y);

   
                                                                              
  
                                                                  
                                   
                                            
                                       
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_RenderDrawPointsF(SDL_Renderer * renderer,
                                                  const SDL_FPoint * points,
                                                  int count);

   
                                                                     
  
                                                         
                                                 
                                                 
                                               
                                               
                                       
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_RenderDrawLineF(SDL_Renderer * renderer,
                                                float x1, float y1, float x2, float y2);

   
                                                                      
                      
  
                                                                 
                                           
                                                           
                                       
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_RenderDrawLinesF(SDL_Renderer * renderer,
                                                 const SDL_FPoint * points,
                                                 int count);

   
                                                                          
  
                                                              
                                                                             
                                       
                                       
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_RenderDrawRectF(SDL_Renderer * renderer,
                                                const SDL_FRect * rect);

   
                                                                             
             
  
                                                                      
                                                                
                                         
                                       
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_RenderDrawRectsF(SDL_Renderer * renderer,
                                                 const SDL_FRect * rects,
                                                 int count);

   
                                                                             
                      
  
                                                              
                                                                             
                                
                                       
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_RenderFillRectF(SDL_Renderer * renderer,
                                                const SDL_FRect * rect);

   
                                                                          
                                       
  
                                                                      
                                                                
                                         
                                       
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_RenderFillRectsF(SDL_Renderer * renderer,
                                                 const SDL_FRect * rects,
                                                 int count);

   
                                                                            
             
  
                                                                     
                                     
                                                                           
                          
                                                                         
                                          
                                       
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_RenderCopyF(SDL_Renderer * renderer,
                                            SDL_Texture * texture,
                                            const SDL_Rect * srcrect,
                                            const SDL_FRect * dstrect);

   
                                                                             
                                                
  
                                                                     
                                     
                                                                           
                          
                                                                         
                                          
                                                                            
                                                                        
                                                                       
                                                                        
                                                  
                                                                              
                                          
                                       
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_RenderCopyExF(SDL_Renderer * renderer,
                                            SDL_Texture * texture,
                                            const SDL_Rect * srcrect,
                                            const SDL_FRect * dstrect,
                                            const double angle,
                                            const SDL_FPoint *center,
                                            const SDL_RendererFlip flip);

   
                                                                              
                                                             
                                                                   
  
                                         
                                                    
                            
                                          
                                                                            
                                                                            
                        
                                        
                                                                
  
                                                      
  
                            
                 
   
extern DECLSPEC int SDLCALL SDL_RenderGeometry(SDL_Renderer *renderer,
                                               SDL_Texture *texture,
                                               const SDL_Vertex *vertices, int num_vertices,
                                               const int *indices, int num_indices);

   
                                                                              
                                                              
                                                                   
  
                                         
                                                    
                             
                                                                          
                                            
                                                                             
                                                  
                                                                          
                                          
                                                                            
                                                                            
                                        
                                                               
                                                                
  
                                                      
  
                         
                 
   
extern DECLSPEC int SDLCALL SDL_RenderGeometryRaw(SDL_Renderer *renderer,
                                               SDL_Texture *texture,
                                               const float *xy, int xy_stride,
                                               const SDL_Color *color, int color_stride,
                                               const float *uv, int uv_stride,
                                               int num_vertices,
                                               const void *indices, int num_indices, int size_indices);

   
                                                                       
  
                                                                     
                                                                              
                                                         
  
                                                                        
                                                                           
                                                                         
                                                                           
                                                                          
                                                      
  
                                        
                                                                           
                                           
                                                                          
                                                                           
                                                         
                                                   
                                                                  
                                                
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_RenderReadPixels(SDL_Renderer * renderer,
                                                 const SDL_Rect * rect,
                                                 Uint32 format,
                                                 void *pixels, int pitch);

   
                                                                          
  
                                                                        
                                                                          
                                                                              
                                                                             
                    
  
                                                                           
                                                                            
                             
  
                                                                             
                                                                            
                                                                           
                                                                         
         
  
                                        
  
                                                                            
                                                                             
                                                                              
                                   
  
                                                     
  
                      
                         
                          
                          
                           
                         
                          
                         
                          
                                 
                             
   
extern DECLSPEC void SDLCALL SDL_RenderPresent(SDL_Renderer * renderer);

   
                                 
  
                                                                              
                        
  
                                        
  
                                                     
  
                        
                                   
   
extern DECLSPEC void SDLCALL SDL_DestroyTexture(SDL_Texture * texture);

   
                                                                           
  
                                                                             
                                                                   
  
                                        
  
                                                     
  
                         
   
extern DECLSPEC void SDLCALL SDL_DestroyRenderer(SDL_Renderer * renderer);

   
                                                                              
                 
  
                                                                            
                                                                       
                                     
  
                                                                           
                                                                         
                                                                            
                                                                         
                                                                          
                                             
  
                                                                             
                                                                           
                                                          
  
                                                                             
                                                                            
                                               
  
                                        
                                                                  
                                                
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_RenderFlush(SDL_Renderer * renderer);


   
                                                        
  
                                                                            
            
  
                                                                          
                                                                             
                                                                           
                                                                              
                                                                        
                                 
  
                                                                           
                                                                           
                                                                         
                                                               
  
                                                                         
                                                                            
                                   
  
                                                                          
                                                                       
                                                                 
                                                                       
                                                                  
                                                                       
                                                
  
                                                     
  
                         
                           
   
extern DECLSPEC int SDLCALL SDL_GL_BindTexture(SDL_Texture *texture, float *texw, float *texh);

   
                                                            
  
                                                                      
  
                                                                              
                                                                 
  
                                                     
  
                         
                         
   
extern DECLSPEC int SDLCALL SDL_GL_UnbindTexture(SDL_Texture *texture);

   
                                                                 
  
                                                                         
                                                            
  
                                        
                                                                          
                          
  
                                                     
  
                                       
   
extern DECLSPEC void *SDLCALL SDL_RenderGetMetalLayer(SDL_Renderer * renderer);

   
                                                      
  
                                                                         
                                                                          
  
                                                                             
                                                                   
                                                                         
                                                                          
  
                                        
                                                                       
                                                                  
  
                                                     
  
                              
   
extern DECLSPEC void *SDLCALL SDL_RenderGetMetalCommandEncoder(SDL_Renderer * renderer);

   
                                      
  
                                         
                                                                  
                                                      
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_RenderSetVSync(SDL_Renderer* renderer, int vsync);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
