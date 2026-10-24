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

   
                     
  
                                                            
   

#ifndef SDL_syswm_h_
#define SDL_syswm_h_

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_version.h"

   
                      
  
                                                                            
                                                                           
                                                                            
                                            
   
struct SDL_SysWMinfo;

#if !defined(SDL_PROTOTYPES_ONLY)

#if defined(SDL_VIDEO_DRIVER_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX    
#define NOMINMAX
#endif
#include <windows.h>
#endif

#if defined(SDL_VIDEO_DRIVER_WINRT)
#include <Inspectable.h>
#endif

 
#if defined(SDL_VIDEO_DRIVER_X11)
#if defined(__APPLE__) && defined(__MACH__)
 
#define Cursor X11Cursor
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#if defined(__APPLE__) && defined(__MACH__)
 
#undef Cursor
#endif

#endif  

#if defined(SDL_VIDEO_DRIVER_DIRECTFB)
#include <directfb.h>
#endif

#if defined(SDL_VIDEO_DRIVER_COCOA)
#ifdef __OBJC__
@class NSWindow;
#else
typedef struct _NSWindow NSWindow;
#endif
#endif

#if defined(SDL_VIDEO_DRIVER_UIKIT)
#ifdef __OBJC__
#include <UIKit/UIKit.h>
#else
typedef struct _UIWindow UIWindow;
typedef struct _UIViewController UIViewController;
#endif
typedef Uint32 GLuint;
#endif

#if defined(SDL_VIDEO_VULKAN) || defined(SDL_VIDEO_METAL)
#define SDL_METALVIEW_TAG 255
#endif

#if defined(SDL_VIDEO_DRIVER_ANDROID)
typedef struct ANativeWindow ANativeWindow;
typedef void *EGLSurface;
#endif

#if defined(SDL_VIDEO_DRIVER_VIVANTE)
#include "SDL_egl.h"
#endif

#if defined(SDL_VIDEO_DRIVER_OS2)
#define INCL_WIN
#include <os2.h>
#endif
#endif  

#if defined(SDL_VIDEO_DRIVER_KMSDRM)
struct gbm_device;
#endif


#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(SDL_PROTOTYPES_ONLY)
   
                                                        
   
typedef enum
{
    SDL_SYSWM_UNKNOWN,
    SDL_SYSWM_WINDOWS,
    SDL_SYSWM_X11,
    SDL_SYSWM_DIRECTFB,
    SDL_SYSWM_COCOA,
    SDL_SYSWM_UIKIT,
    SDL_SYSWM_WAYLAND,
    SDL_SYSWM_MIR,   
    SDL_SYSWM_WINRT,
    SDL_SYSWM_ANDROID,
    SDL_SYSWM_VIVANTE,
    SDL_SYSWM_OS2,
    SDL_SYSWM_HAIKU,
    SDL_SYSWM_KMSDRM,
    SDL_SYSWM_RISCOS
} SDL_SYSWM_TYPE;

   
                               
   
struct SDL_SysWMmsg
{
    SDL_version version;
    SDL_SYSWM_TYPE subsystem;
    union
    {
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
        struct {
            HWND hwnd;                   
            UINT msg;                    
            WPARAM wParam;               
            LPARAM lParam;               
        } win;
#endif
#if defined(SDL_VIDEO_DRIVER_X11)
        struct {
            XEvent event;
        } x11;
#endif
#if defined(SDL_VIDEO_DRIVER_DIRECTFB)
        struct {
            DFBEvent event;
        } dfb;
#endif
#if defined(SDL_VIDEO_DRIVER_COCOA)
        struct
        {
                                                                                       
                                                                   
               
            int dummy;
             
        } cocoa;
#endif
#if defined(SDL_VIDEO_DRIVER_UIKIT)
        struct
        {
            int dummy;
             
        } uikit;
#endif
#if defined(SDL_VIDEO_DRIVER_VIVANTE)
        struct
        {
            int dummy;
             
        } vivante;
#endif
#if defined(SDL_VIDEO_DRIVER_OS2)
        struct
        {
            BOOL fFrame;                 
            HWND hwnd;                   
            ULONG msg;                   
            MPARAM mp1;                  
            MPARAM mp2;                  
        } os2;
#endif
         
        int dummy;
    } msg;
};

   
                                                    
  
                                                                     
                                                                    
   
struct SDL_SysWMinfo
{
    SDL_version version;
    SDL_SYSWM_TYPE subsystem;
    union
    {
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
        struct
        {
            HWND window;                 
            HDC hdc;                     
            HINSTANCE hinstance;         
        } win;
#endif
#if defined(SDL_VIDEO_DRIVER_WINRT)
        struct
        {
            IInspectable * window;       
        } winrt;
#endif
#if defined(SDL_VIDEO_DRIVER_X11)
        struct
        {
            Display *display;            
            Window window;               
        } x11;
#endif
#if defined(SDL_VIDEO_DRIVER_DIRECTFB)
        struct
        {
            IDirectFB *dfb;              
            IDirectFBWindow *window;     
            IDirectFBSurface *surface;   
        } dfb;
#endif
#if defined(SDL_VIDEO_DRIVER_COCOA)
        struct
        {
#if defined(__OBJC__) && defined(__has_feature)
        #if __has_feature(objc_arc)
            NSWindow __unsafe_unretained *window;  
        #else
            NSWindow *window;                      
        #endif
#else
            NSWindow *window;                      
#endif
        } cocoa;
#endif
#if defined(SDL_VIDEO_DRIVER_UIKIT)
        struct
        {
#if defined(__OBJC__) && defined(__has_feature)
        #if __has_feature(objc_arc)
            UIWindow __unsafe_unretained *window;  
        #else
            UIWindow *window;                      
        #endif
#else
            UIWindow *window;                      
#endif
            GLuint framebuffer;  
            GLuint colorbuffer;  
            GLuint resolveFramebuffer;  
        } uikit;
#endif
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
        struct
        {
            struct wl_display *display;              
            struct wl_surface *surface;              
            void *shell_surface;                     
            struct wl_egl_window *egl_window;        
            struct xdg_surface *xdg_surface;         
            struct xdg_toplevel *xdg_toplevel;       
            struct xdg_popup *xdg_popup;             
            struct xdg_positioner *xdg_positioner;   
        } wl;
#endif
#if defined(SDL_VIDEO_DRIVER_MIR)   
        struct
        {
            void *connection;   
            void *surface;   
        } mir;
#endif

#if defined(SDL_VIDEO_DRIVER_ANDROID)
        struct
        {
            ANativeWindow *window;
            EGLSurface surface;
        } android;
#endif

#if defined(SDL_VIDEO_DRIVER_OS2)
        struct
        {
            HWND hwnd;                   
            HWND hwndFrame;              
        } os2;
#endif

#if defined(SDL_VIDEO_DRIVER_VIVANTE)
        struct
        {
            EGLNativeDisplayType display;
            EGLNativeWindowType window;
        } vivante;
#endif

#if defined(SDL_VIDEO_DRIVER_KMSDRM)
        struct
        {
            int dev_index;                
            int drm_fd;                   
            struct gbm_device *gbm_dev;   
        } kmsdrm;
#endif

         
         
        Uint8 dummy[64];
    } info;
};

#endif  

typedef struct SDL_SysWMinfo SDL_SysWMinfo;


   
                                                  
  
                                                                     
  
                                                                     
                                                                             
                                                            
  
                                                                      
                                                                           
                                                                            
                                                                          
                                                                             
  
                                                     
   
extern DECLSPEC SDL_bool SDLCALL SDL_GetWindowWMInfo(SDL_Window * window,
                                                     SDL_SysWMinfo * info);


 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
