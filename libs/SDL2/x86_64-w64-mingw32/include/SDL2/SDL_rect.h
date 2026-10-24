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

   
                    
  
                                                                 
   

#ifndef SDL_rect_h_
#define SDL_rect_h_

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_pixels.h"
#include "SDL_rwops.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

   
                                               
  
                        
                      
   
typedef struct SDL_Point
{
    int x;
    int y;
} SDL_Point;

   
                                                      
  
                         
                       
   
typedef struct SDL_FPoint
{
    float x;
    float y;
} SDL_FPoint;


   
                                                            
  
                    
                     
                          
                        
                               
                    
                        
   
typedef struct SDL_Rect
{
    int x, y;
    int w, h;
} SDL_Rect;


   
                                                                   
  
                     
                      
                             
                           
                         
                                
                     
                         
                       
   
typedef struct SDL_FRect
{
    float x;
    float y;
    float w;
    float h;
} SDL_FRect;


   
                                                    
   
SDL_FORCE_INLINE SDL_bool SDL_PointInRect(const SDL_Point *p, const SDL_Rect *r)
{
    return ( (p->x >= r->x) && (p->x < (r->x + r->w)) &&
             (p->y >= r->y) && (p->y < (r->y + r->h)) ) ? SDL_TRUE : SDL_FALSE;
}

   
                                             
   
SDL_FORCE_INLINE SDL_bool SDL_RectEmpty(const SDL_Rect *r)
{
    return ((!r) || (r->w <= 0) || (r->h <= 0)) ? SDL_TRUE : SDL_FALSE;
}

   
                                                
   
SDL_FORCE_INLINE SDL_bool SDL_RectEquals(const SDL_Rect *a, const SDL_Rect *b)
{
    return (a && b && (a->x == b->x) && (a->y == b->y) &&
            (a->w == b->w) && (a->h == b->h)) ? SDL_TRUE : SDL_FALSE;
}

   
                                              
  
                                                                
  
                                                                  
                                                                   
                                                                      
  
                                                     
  
                        
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasIntersection(const SDL_Rect * A,
                                                     const SDL_Rect * B);

   
                                                
  
                                                                
  
                                                                  
                                                                   
                                                                         
                                       
                                                                      
  
                                                     
  
                          
   
extern DECLSPEC SDL_bool SDLCALL SDL_IntersectRect(const SDL_Rect * A,
                                                   const SDL_Rect * B,
                                                   SDL_Rect * result);

   
                                         
  
                                                                  
                                                                   
                                                                             
                            
  
                                                     
   
extern DECLSPEC void SDLCALL SDL_UnionRect(const SDL_Rect * A,
                                           const SDL_Rect * B,
                                           SDL_Rect * result);

   
                                                           
  
                                                                              
              
  
                                                                           
                         
                                                              
                                                                          
                                                                           
                          
                                                                        
                                                          
  
                                                     
   
extern DECLSPEC SDL_bool SDLCALL SDL_EnclosePoints(const SDL_Point * points,
                                                   int count,
                                                   const SDL_Rect * clip,
                                                   SDL_Rect * result);

   
                                                              
  
                                                                              
                                                                          
                                                                           
                                                                         
                                                                   
  
                                                                            
                                                               
                                                               
                                                             
                                                             
                                                                      
  
                                                     
   
extern DECLSPEC SDL_bool SDLCALL SDL_IntersectRectAndLine(const SDL_Rect *
                                                          rect, int *X1,
                                                          int *Y1, int *X2,
                                                          int *Y2);


 

   
                                                    
   
SDL_FORCE_INLINE SDL_bool SDL_PointInFRect(const SDL_FPoint *p, const SDL_FRect *r)
{
    return ( (p->x >= r->x) && (p->x < (r->x + r->w)) &&
             (p->y >= r->y) && (p->y < (r->y + r->h)) ) ? SDL_TRUE : SDL_FALSE;
}

   
                                             
   
SDL_FORCE_INLINE SDL_bool SDL_FRectEmpty(const SDL_FRect *r)
{
    return ((!r) || (r->w <= 0.0f) || (r->h <= 0.0f)) ? SDL_TRUE : SDL_FALSE;
}

   
                                                                           
  
                                                      
   
SDL_FORCE_INLINE SDL_bool SDL_FRectEqualsEpsilon(const SDL_FRect *a, const SDL_FRect *b, const float epsilon)
{
    return (a && b && ((a == b) ||
            ((SDL_fabsf(a->x - b->x) <= epsilon) &&
            (SDL_fabsf(a->y - b->y) <= epsilon) &&
            (SDL_fabsf(a->w - b->w) <= epsilon) &&
            (SDL_fabsf(a->h - b->h) <= epsilon))))
            ? SDL_TRUE : SDL_FALSE;
}

   
                                                                         
  
                                                      
   
SDL_FORCE_INLINE SDL_bool SDL_FRectEquals(const SDL_FRect *a, const SDL_FRect *b)
{
    return SDL_FRectEqualsEpsilon(a, b, SDL_FLT_EPSILON);
}

   
                                                                   
  
                                                                
  
                                                                   
                                                                    
                                                                      
  
                                                      
  
                        
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasIntersectionF(const SDL_FRect * A,
                                                      const SDL_FRect * B);

   
                                                                     
  
                                                                
  
                                                                   
                                                                    
                                                                          
                                       
                                                                      
  
                                                      
  
                           
   
extern DECLSPEC SDL_bool SDLCALL SDL_IntersectFRect(const SDL_FRect * A,
                                                    const SDL_FRect * B,
                                                    SDL_FRect * result);

   
                                                              
  
                                                                   
                                                                    
                                                                              
                            
  
                                                      
   
extern DECLSPEC void SDLCALL SDL_UnionFRect(const SDL_FRect * A,
                                            const SDL_FRect * B,
                                            SDL_FRect * result);

   
                                                                     
             
  
                                                                              
              
  
                                                                            
                         
                                                              
                                                                           
                                                                            
                          
                                                                        
                                                          
  
                                                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_EncloseFPoints(const SDL_FPoint * points,
                                                    int count,
                                                    const SDL_FRect * clip,
                                                    SDL_FRect * result);

   
                                                                        
             
  
                                                                              
                                                                          
                                                                           
                                                                         
                                                                   
  
                                                                             
                                                               
                                                               
                                                             
                                                             
                                                                      
  
                                                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_IntersectFRectAndLine(const SDL_FRect *
                                                           rect, float *X1,
                                                           float *Y1, float *X2,
                                                           float *Y2);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
