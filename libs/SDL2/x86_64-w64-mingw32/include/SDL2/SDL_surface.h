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

   
                       
  
                                                                      
   

#ifndef SDL_surface_h_
#define SDL_surface_h_

#include "SDL_stdinc.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_blendmode.h"
#include "SDL_rwops.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

   
                       
  
                                                                  
  
             
                                
   
 
#define SDL_SWSURFACE       0            
#define SDL_PREALLOC        0x00000001   
#define SDL_RLEACCEL        0x00000002   
#define SDL_DONTFREE        0x00000004   
#define SDL_SIMD_ALIGNED    0x00000008   
  

   
                                                                      
   
#define SDL_MUSTLOCK(S) (((S)->flags & SDL_RLEACCEL) != 0)

typedef struct SDL_BlitMap SDL_BlitMap;   

   
                                                           
  
                                                                              
                                                                          
   
typedef struct SDL_Surface
{
    Uint32 flags;                
    SDL_PixelFormat *format;     
    int w, h;                    
    int pitch;                   
    void *pixels;                

     
    void *userdata;              

     
    int locked;                  

     
    void *list_blitmap;          

     
    SDL_Rect clip_rect;          

     
    SDL_BlitMap *map;            

     
    int refcount;                
} SDL_Surface;

   
                                                                   
   
typedef int (SDLCALL *SDL_blit) (struct SDL_Surface * src, SDL_Rect * srcrect,
                                 struct SDL_Surface * dst, SDL_Rect * dstrect);

   
                                                             
   
typedef enum
{
    SDL_YUV_CONVERSION_JPEG,         
    SDL_YUV_CONVERSION_BT601,        
    SDL_YUV_CONVERSION_BT709,        
    SDL_YUV_CONVERSION_AUTOMATIC     
} SDL_YUV_CONVERSION_MODE;

   
                              
  
                                                                            
                                                                       
                         
  
                                                                             
                                                                        
                                                                            
                                                  
  
         
                                          
      
  
                                                              
  
                                                                          
  
         
                                                        
      
  
                                                                           
                         
  
                                                           
                                        
                                          
                                                
                                           
                                             
                                            
                                             
                                                                              
                                                     
  
                                                     
  
                               
                                     
                      
   
extern DECLSPEC SDL_Surface *SDLCALL SDL_CreateRGBSurface
    (Uint32 flags, int width, int height, int depth,
     Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);


 

   
                                                           
  
                                                                            
                                                                          
                            
  
                                                           
                                        
                                          
                                                
                                                                            
                                                                              
                                                     
  
                                                     
  
                           
                               
                      
   
extern DECLSPEC SDL_Surface *SDLCALL SDL_CreateRGBSurfaceWithFormat
    (Uint32 flags, int width, int height, int depth, Uint32 format);

   
                                                       
  
                                                                            
                                                                         
                                                  
  
                                                                              
                                                            
  
                                                 
                                        
                                          
                                                
                                                 
                                           
                                             
                                            
                                             
                                                                              
                                                     
  
                                                     
  
                           
                                     
                      
   
extern DECLSPEC SDL_Surface *SDLCALL SDL_CreateRGBSurfaceFrom(void *pixels,
                                                              int width,
                                                              int height,
                                                              int depth,
                                                              int pitch,
                                                              Uint32 Rmask,
                                                              Uint32 Gmask,
                                                              Uint32 Bmask,
                                                              Uint32 Amask);

 

   
                                                                            
              
  
                                                                        
                                                                           
                                   
  
                                                                              
                                                            
  
                                                 
                                        
                                          
                                                
                                                 
                                                                            
                                                                              
                                                     
  
                                                     
  
                               
                                     
                      
   
extern DECLSPEC SDL_Surface *SDLCALL SDL_CreateRGBSurfaceWithFormatFrom
    (void *pixels, int width, int height, int depth, int pitch, Uint32 format);

   
                       
  
                                            
  
                                          
  
                                                     
  
                           
                               
                  
                     
   
extern DECLSPEC void SDLCALL SDL_FreeSurface(SDL_Surface * surface);

   
                                     
  
                                                     
  
                                                     
                                                  
                                                                  
                                                
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_SetSurfacePalette(SDL_Surface * surface,
                                                  SDL_Palette * palette);

   
                                                      
  
                                                                             
                                                                    
                                                                             
                                     
  
                                                                            
                                                                           
                                         
  
                                                        
                                                                  
                                                
  
                                                     
  
                   
                        
   
extern DECLSPEC int SDLCALL SDL_LockSurface(SDL_Surface * surface);

   
                                                         
  
                                                          
  
                                                     
  
                      
   
extern DECLSPEC void SDLCALL SDL_UnlockSurface(SDL_Surface * surface);

   
                                                    
  
                                                                            
                           
  
                                                                         
                                                                           
                                                                     
  
                                             
                                                               
                                                                            
                                                            
  
                                                     
  
                      
                     
                  
                     
   
extern DECLSPEC SDL_Surface *SDLCALL SDL_LoadBMP_RW(SDL_RWops * src,
                                                    int freesrc);

   
                              
  
                     
   
#define SDL_LoadBMP(file)   SDL_LoadBMP_RW(SDL_RWFromFile(file, "rb"), 1)

   
                                                              
  
                                                                            
                                                                          
                                                                           
                                                                              
                 
  
                                                                            
                                      
                                                                  
                                                                  
                                                
  
                                                     
  
                     
                  
   
extern DECLSPEC int SDLCALL SDL_SaveBMP_RW
    (SDL_Surface * surface, SDL_RWops * dst, int freedst);

   
                             
  
                      
   
#define SDL_SaveBMP(surface, file) \
        SDL_SaveBMP_RW(surface, SDL_RWFromFile(file, "wb"), 1)

   
                                               
  
                                                                             
                                                                   
  
                                                       
                                                                
                                                                  
                                                
  
                                                     
  
                      
                      
                        
   
extern DECLSPEC int SDLCALL SDL_SetSurfaceRLE(SDL_Surface * surface,
                                              int flag);

   
                                             
  
                                                                      
  
                                                    
                                                                        
  
                                                      
  
                        
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasSurfaceRLE(SDL_Surface * surface);

   
                                                      
  
                                                                             
                                                                              
                                                      
  
                                                                   
                
  
                                                                            
                                                                              
  
                                                     
                                                                           
                                   
                                                                  
                                                
  
                                                     
  
                      
                      
   
extern DECLSPEC int SDLCALL SDL_SetColorKey(SDL_Surface * surface,
                                            int flag, Uint32 key);

   
                                              
  
                                                                      
  
                                                    
                                                                        
  
                                                     
  
                      
                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasColorKey(SDL_Surface * surface);

   
                                                       
  
                                                                              
                
  
                                                                          
  
                                                    
                                                            
                                                                  
                                                
  
                                                     
  
                      
                      
   
extern DECLSPEC int SDLCALL SDL_GetColorKey(SDL_Surface * surface,
                                            Uint32 * key);

   
                                                                 
  
                                                                            
                                                                       
                     
  
                                
  
                                                     
                                                               
                                                                 
                                                                
                                                                  
                                                
  
                                                     
  
                             
                             
   
extern DECLSPEC int SDLCALL SDL_SetSurfaceColorMod(SDL_Surface * surface,
                                                   Uint8 r, Uint8 g, Uint8 b);


   
                                                                  
  
                                                    
                                                                
                                                                  
                                                                 
                                                                  
                                                
  
                                                     
  
                             
                             
   
extern DECLSPEC int SDLCALL SDL_GetSurfaceColorMod(SDL_Surface * surface,
                                                   Uint8 * r, Uint8 * g,
                                                   Uint8 * b);

   
                                                         
  
                                                                           
                                                                             
  
                                
  
                                                     
                                                               
                                                                  
                                                
  
                                                     
  
                             
                             
   
extern DECLSPEC int SDLCALL SDL_SetSurfaceAlphaMod(SDL_Surface * surface,
                                                   Uint8 alpha);

   
                                                          
  
                                                    
                                                                
                                                                  
                                                
  
                                                     
  
                             
                             
   
extern DECLSPEC int SDLCALL SDL_GetSurfaceAlphaMod(SDL_Surface * surface,
                                                   Uint8 * alpha);

   
                                               
  
                                                                              
                                                                      
                        
  
                                                     
                                                              
                                                                  
                                                
  
                                                     
  
                              
   
extern DECLSPEC int SDLCALL SDL_SetSurfaceBlendMode(SDL_Surface * surface,
                                                    SDL_BlendMode blendMode);

   
                                               
  
                                                    
                                                                      
                                                                  
                                                
  
                                                     
  
                              
   
extern DECLSPEC int SDLCALL SDL_GetSurfaceBlendMode(SDL_Surface * surface,
                                                    SDL_BlendMode *blendMode);

   
                                            
  
                                                                             
                           
  
                                                                           
                        
  
                                                         
                                                                             
                                       
                                                                       
                                                           
  
                                                     
  
                      
                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_SetClipRect(SDL_Surface * surface,
                                                 const SDL_Rect * rect);

   
                                            
  
                                                                             
                           
  
                                                                          
                         
                                                                              
                          
  
                                                     
  
                      
                      
   
extern DECLSPEC void SDLCALL SDL_GetClipRect(SDL_Surface * surface,
                                             SDL_Rect * rect);

  
                                                           
  
                                                               
  
                                           
                                                                              
                             
   
extern DECLSPEC SDL_Surface *SDLCALL SDL_DuplicateSurface(SDL_Surface * surface);

   
                                                                     
  
                                                                              
                                                                             
                                                                         
                                    
  
                                                           
                                                                             
                 
                                                                      
                                           
                                                                              
                                                     
  
                                                     
  
                      
                               
                           
   
extern DECLSPEC SDL_Surface *SDLCALL SDL_ConvertSurface
    (SDL_Surface * src, const SDL_PixelFormat * fmt, Uint32 flags);

   
                                                                          
  
                                                                        
                                                                              
                                                                   
                                                                            
  
                                                           
                                                                      
                                    
                                                                      
                                           
                                                                              
                                                     
  
                                                     
  
                      
                         
                           
   
extern DECLSPEC SDL_Surface *SDLCALL SDL_ConvertSurfaceFormat
    (SDL_Surface * src, Uint32 pixel_format, Uint32 flags);

   
                                                          
  
                                                         
                                                           
                                                                            
                                            
                                                            
                                                                            
                                                           
                                                                 
                                                                  
                                                
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_ConvertPixels(int width, int height,
                                              Uint32 src_format,
                                              const void * src, int src_pitch,
                                              Uint32 dst_format,
                                              void * dst, int dst_pitch);

   
                                              
  
                                                                            
  
                                                                            
  
                                                            
                                                              
                                                                            
                                            
                                                            
                                                                            
                                                                     
                                                                 
                                                                  
                                                
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_PremultiplyAlpha(int width, int height,
                                                 Uint32 src_format,
                                                 const void * src, int src_pitch,
                                                 Uint32 dst_format,
                                                 void * dst, int dst_pitch);

   
                                                            
  
                                                                          
                                                                             
                                                                        
                                        
  
                                                               
                                                                             
                                    
  
                                                                  
                                                                            
                                              
                                      
                                                                  
                                                
  
                                                     
  
                    
   
extern DECLSPEC int SDLCALL SDL_FillRect
    (SDL_Surface * dst, const SDL_Rect * rect, Uint32 color);

   
                                                                    
  
                                                                          
                                                                             
                                                                        
                                        
  
                                                               
                                                                             
                                    
  
                                                                  
                                                                          
                                                     
                                      
                                                                  
                                                
  
                                                     
  
                   
   
extern DECLSPEC int SDLCALL SDL_FillRects
    (SDL_Surface * dst, const SDL_Rect * rects, int count, Uint32 color);

 
   
                                                                            
  
                                                               
                                                                           
                                                                              
                                                                 
  
                                                                  
  
                                                               
  
                                                                          
                           
             
              
                                                           
                                                                          
                                
                                                          
                 
                                                                 
                                                                 
                   

              
                                                           
                                                        
                                                          
                                                                          
           
                                                                 
                         

               
                                                           
                                                                          
                                
                                                          
                                            
                                                                 
                                                                 
                   

             
                                                           
                                                        
                                                          
                 
           
                                                                 
                         
                
  
                                                                     
                                                                      
   
#define SDL_BlitSurface SDL_UpperBlit

   
                                                                          
  
                                                                            
                                                      
  
                                                     
  
                      
   
extern DECLSPEC int SDLCALL SDL_UpperBlit
    (SDL_Surface * src, const SDL_Rect * srcrect,
     SDL_Surface * dst, SDL_Rect * dstrect);

   
                                           
  
                                                                         
                                                                     
  
                                                                           
           
  
                                                         
                                                                         
                                                            
                                                               
                                                                           
                             
                                                                  
                                                
  
                                                     
  
                      
   
extern DECLSPEC int SDLCALL SDL_LowerBlit
    (SDL_Surface * src, SDL_Rect * srcrect,
     SDL_Surface * dst, SDL_Rect * dstrect);


   
                                                                             
          
  
                                       
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_SoftStretch(SDL_Surface * src,
                                            const SDL_Rect * srcrect,
                                            SDL_Surface * dst,
                                            const SDL_Rect * dstrect);

   
                                                                           
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_SoftStretchLinear(SDL_Surface * src,
                                            const SDL_Rect * srcrect,
                                            SDL_Surface * dst,
                                            const SDL_Rect * dstrect);


#define SDL_BlitScaled SDL_UpperBlitScaled

   
                                                          
  
                                                                        
                                                               
  
                                                     
  
                     
   
extern DECLSPEC int SDLCALL SDL_UpperBlitScaled
    (SDL_Surface * src, const SDL_Rect * srcrect,
    SDL_Surface * dst, SDL_Rect * dstrect);

   
                                                  
  
                                                                              
                                                           
  
                                                         
                                                                         
                        
                                                               
                                                                           
                             
                                                                  
                                                
  
                                                     
  
                     
   
extern DECLSPEC int SDLCALL SDL_LowerBlitScaled
    (SDL_Surface * src, SDL_Rect * srcrect,
    SDL_Surface * dst, SDL_Rect * dstrect);

   
                              
  
                                                     
   
extern DECLSPEC void SDLCALL SDL_SetYUVConversionMode(SDL_YUV_CONVERSION_MODE mode);

   
                              
  
                                                     
   
extern DECLSPEC SDL_YUV_CONVERSION_MODE SDLCALL SDL_GetYUVConversionMode(void);

   
                                                                             
                                                                   
  
                                                     
   
extern DECLSPEC SDL_YUV_CONVERSION_MODE SDLCALL SDL_GetYUVConversionModeForResolution(int width, int height);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
