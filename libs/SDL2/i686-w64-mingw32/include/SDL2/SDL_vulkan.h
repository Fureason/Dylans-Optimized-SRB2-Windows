/*
  Simple DirectMedia Layer
  Copyright (C) 2017, Mark Callow

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

   
                      
  
                                                                         
   

#ifndef SDL_vulkan_h_
#define SDL_vulkan_h_

#include "SDL_video.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

 
#ifdef VULKAN_H_
#define NO_SDL_VULKAN_TYPEDEFS
#endif
#ifndef NO_SDL_VULKAN_TYPEDEFS
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;

#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
#else
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#endif

VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR)

#endif  

typedef VkInstance SDL_vulkanInstance;
typedef VkSurfaceKHR SDL_vulkanSurface;  

   
                                  
  
                                                                         
                                                                     
   
 

   
                                              
  
                                                                        
                                                                          
                                                                           
  
                                                                             
                                                                             
                                                                          
               
  
                                                                              
                                                          
                                                                             
                                                               
  
                                                                    
                                                                              
                                                                          
                                                                          
                                                                   
                                                  
                                                                            
                                                                          
                                                                            
          
  
                                                                           
                                                                          
                   
  
                                                                        
                                                                      
                                                
  
                                                     
  
                                       
                               
   
extern DECLSPEC int SDLCALL SDL_Vulkan_LoadLibrary(const char *path);

   
                                                           
  
                                                                         
                                                            
  
                                                                              
  
                                                     
   
extern DECLSPEC void *SDLCALL SDL_Vulkan_GetVkGetInstanceProcAddr(void);

   
                                                                          
  
                                                     
  
                             
   
extern DECLSPEC void SDLCALL SDL_Vulkan_UnloadLibrary(void);

   
                                                                             
                                 
  
                                                                              
                                                                            
                                                                              
                                                                           
                                                                            
                                                                        
                                                                          
                                                               
  
                                                                          
                                                                    
  
                                                                           
                                                                             
                                                                            
                                          
                                                                         
                                           
                                                    
  
                                                     
  
                               
   
extern DECLSPEC SDL_bool SDLCALL SDL_Vulkan_GetInstanceExtensions(SDL_Window *window,
                                                                  unsigned int *pCount,
                                                                  const char **pNames);

   
                                                  
  
                                                                            
                                                                
                                              
  
                                                                 
                                             
                                                                        
                                 
                                                    
  
                                                     
  
                                       
                                 
   
extern DECLSPEC SDL_bool SDLCALL SDL_Vulkan_CreateSurface(SDL_Window *window,
                                                          VkInstance instance,
                                                          VkSurfaceKHR* surface);

   
                                                                         
  
                                                                            
                                                                             
                                                                      
                                                          
  
                                                                  
                                                                 
                                                                  
  
                                                     
  
                        
                       
                               
   
extern DECLSPEC void SDLCALL SDL_Vulkan_GetDrawableSize(SDL_Window * window,
                                                        int *w, int *h);

  

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  
