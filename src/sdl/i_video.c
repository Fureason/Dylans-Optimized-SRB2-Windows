
// Emacs style mode select   -*- C++ -*-
// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2014-2023 by Sonic Team Junior.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------



#include <stdlib.h>

#include <signal.h>

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef HAVE_SDL
#define _MATH_DEFINES_DEFINED
#include "SDL.h"

#ifdef _MSC_VER
#include <windows.h>
#pragma warning(default : 4214 4244)
#endif

#ifdef HAVE_TTF
#include "i_ttf.h"
#endif

#ifdef HAVE_IMAGE
#include "SDL_image.h"
#elif defined (__unix__) || (!defined(__APPLE__) && defined (UNIXCOMMON))  
#define LOAD_XPM  
#include "IMG_xpm.c"  
#define HAVE_IMAGE  
#endif

#ifdef HAVE_IMAGE
#include "SDL_icon.xpm"
#endif

#include "../doomdef.h"

#ifdef _WIN32
#include <windows.h>
#include "SDL_system.h"
#include "SDL_syswm.h"
#endif

#include "../doomstat.h"
#include "../i_system.h"
#include "../i_time.h"
#include "../v_video.h"
#include "../m_argv.h"
#include "../m_menu.h"
#include "../m_misc.h"
#include "../d_main.h"
#include "../s_sound.h"
#include "../i_joy.h"
#include "../st_stuff.h"
#include "../hu_stuff.h"
#include "../g_game.h"
#include "../netcode/d_clisrv.h"
#include "../i_video.h"
#include "../console.h"
#include "../command.h"
#include "../r_main.h"
#include "../r_fps.h"
#include "../screen.h"
#include "../lua_script.h"
#include "../lua_libs.h"
#include "../lua_hook.h"
#include "sdlmain.h"
#ifdef HWRENDER
#include "../hardware/hw_main.h"
#include "../hardware/hw_drv.h"

#include "hwsym_sdl.h"
#include "ogl_sdl.h"
#endif


#define MAXWINMODES (22)

          
  
static INT32 numVidModes = -1;

          
  
static char vidModeName[33][32];  

rendermode_t rendermode = render_soft;
rendermode_t chosenrendermode = render_none;  

static void VidWaitChanged(void);
static void WindowedStyleChanged(void);
static UINT32 VID_GetRefreshRate(void);
static UINT32 refresh_rate;

static CV_PossibleValue_t vsync_cons_t[] = {
	{0, "Off"},
	{1, "On"},
	{0, NULL}
};


consvar_t cv_vidwait = CVAR_INIT ("vid_wait", "On", CV_SAVE | CV_CALL, vsync_cons_t, VidWaitChanged);
static CV_PossibleValue_t windowedstyle_cons_t[] = {
	{0, "Normal"},
	{1, "Borderless Desktop"},
	{0, NULL}
};
                                                                         
                                                                           
                                                                         
static consvar_t cv_windowedstyle = CVAR_INIT ("windowedstyle", "Normal", CV_CALL | CV_NOSHOWHELP,
	windowedstyle_cons_t, WindowedStyleChanged);
static consvar_t cv_stretch = CVAR_INIT ("stretch", "Off", CV_SAVE|CV_NOSHOWHELP, CV_OnOff, NULL);
static consvar_t cv_alwaysgrabmouse = CVAR_INIT ("alwaysgrabmouse", "Off", CV_SAVE, CV_OnOff, NULL);

UINT8 graphics_started = 0;  


boolean allow_fullscreen = false;
static SDL_bool disable_fullscreen = SDL_FALSE;
#define USE_FULLSCREEN (disable_fullscreen||!allow_fullscreen)?0:cv_fullscreen.value
static SDL_bool disable_mouse = SDL_FALSE;
#define USE_MOUSEINPUT (!disable_mouse && cv_usemouse.value && havefocus)
#define MOUSE_MENU false  
#define MOUSEBUTTONS_MAX MOUSEBUTTONS


static      INT32          firstEntry = 0;


static      INT32        mousemovex = 0, mousemovey = 0;


static      SDL_Surface *vidSurface = NULL;
static      SDL_Surface *bufSurface = NULL;
static      SDL_Surface *icoSurface = NULL;
static      size_t        vid_buffer_capacity = 0;
static      SDL_Color    localPalette[256];
#if 0
static      SDL_Rect   **modeList = NULL;
static       Uint8       BitsPerPixel = 16;
#endif
Uint16      realwidth = BASEVIDWIDTH;
Uint16      realheight = BASEVIDHEIGHT;
static       SDL_bool    mousegrabok = SDL_TRUE;
static       SDL_bool    wrapmouseok = SDL_FALSE;
#define HalfWarpMouse(x,y) if (wrapmouseok) SDL_WarpMouseInWindow(window, (Uint16)(x/2),(Uint16)(y/2))
static       SDL_bool    videoblitok = SDL_FALSE;
static       SDL_bool    exposevideo = SDL_FALSE;
static       SDL_bool    usesdl2soft = SDL_FALSE;
static       SDL_bool    window_needs_first_present = SDL_FALSE;
#ifdef HWRENDER
static       SDL_bool    software_gl_presenter = SDL_FALSE;
#endif


SDL_Window   *window;
SDL_Renderer *renderer;
static SDL_Texture  *texture;
static SDL_bool      havefocus = SDL_TRUE;
static const char *fallback_resolution_name = "Fallback";

#ifdef _WIN32
#define SRB2_SIZEMOVE_TIMER_ID 0x53524232u
static boolean windows_sizemove_active;
static precise_t windows_sizemove_started;
static precise_t windows_sizemove_finished;
static HWND windows_sizemove_window;

                                                                          
                                                                            
                                                                        
                                                                              
              
static void SDLCALL Impl_WindowsMessageHook(void *userdata, void *nativewindow,
	unsigned int message, Uint64 wparam, Sint64 lparam)
{
	HWND hwnd = (HWND)nativewindow;
	(void)userdata;
	(void)lparam;

	if (message == WM_ENTERSIZEMOVE)
	{
		windows_sizemove_active = true;
		windows_sizemove_started = I_GetPreciseTime();
		windows_sizemove_finished = 0;
		windows_sizemove_window = hwnd;
		SetTimer(hwnd, SRB2_SIZEMOVE_TIMER_ID, 100, NULL);
	}
	else if (message == WM_TIMER && windows_sizemove_active
		&& wparam == SRB2_SIZEMOVE_TIMER_ID)
	{
		if (netgame || serverrunning)
			NetKeepAlive();
	}
	else if (message == WM_EXITSIZEMOVE)
	{
		if (windows_sizemove_window)
			KillTimer(windows_sizemove_window, SRB2_SIZEMOVE_TIMER_ID);
		windows_sizemove_finished = I_GetPreciseTime();
		windows_sizemove_active = false;
		windows_sizemove_window = NULL;
		if (netgame || serverrunning)
			NetKeepAlive();
	}
}

boolean I_WindowManagerBlockedDuring(precise_t started, precise_t finished)
{
	precise_t blocked_finished;

	if (!windows_sizemove_started)
		return false;
	blocked_finished = windows_sizemove_active
		? I_GetPreciseTime() : windows_sizemove_finished;
	return windows_sizemove_started <= finished && blocked_finished >= started;
}
#else
boolean I_WindowManagerBlockedDuring(precise_t started, precise_t finished)
{
	(void)started;
	(void)finished;
	return false;
}
#endif


static INT32 windowedModes[MAXWINMODES][2] =
{
	{2560,1440},  
	{1920,1200},  
	{1920,1080},  
	{1680,1050},  
	{1600,1200},  
	{1600,1000},  
	{1600, 900},  
	{1536, 864},  
	{1366, 768},  
	{1440, 900},  
	{1280,1024},  
	{1280, 960},  
	{1280, 800},  
	{1280, 720},  
	{1152, 864},  
	{1024, 768},  
	{ 960, 600},  
	{ 800, 600},  
	{ 640, 480},  
	{ 640, 400},  
	{ 320, 240},  
	{ 320, 200},  
};

static void Impl_VideoSetupSDLBuffer(void);
static void Impl_VideoSetupBuffer(void);
static void Impl_DestroySoftwareContext(void);
static SDL_bool Impl_CreateWindow(SDL_bool fullscreen);

static void Impl_SetWindowIcon(void);

static void Impl_ApplyWindowedStyle(INT32 width, INT32 height, SDL_bool reposition)
{
	const int display = SDL_GetWindowDisplayIndex(window);

	SDL_SetWindowBordered(window, SDL_TRUE);
	SDL_SetWindowSize(window, width, height);
	if (reposition)
	{
		SDL_SetWindowPosition(window,
			SDL_WINDOWPOS_CENTERED_DISPLAY(display),
			SDL_WINDOWPOS_CENTERED_DISPLAY(display));
	}
}

static void SDLSetMode(INT32 width, INT32 height, SDL_bool fullscreen, SDL_bool reposition)
{
	static SDL_bool wasfullscreen = SDL_FALSE;
	Uint32 rmask;
	Uint32 gmask;
	Uint32 bmask;
	Uint32 amask;
	int bpp = 16;
	int sw_texture_format = SDL_PIXELFORMAT_ABGR8888;

	realwidth = vid.width;
	realheight = vid.height;

	if (window)
	{
		if (fullscreen)
		{
			wasfullscreen = SDL_TRUE;
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else  
		{
			if (wasfullscreen)
			{
				wasfullscreen = SDL_FALSE;
				SDL_SetWindowFullscreen(window, 0);
			}
			Impl_ApplyWindowedStyle(width, height, reposition);
		}
	}
	else
	{
		Impl_CreateWindow(fullscreen);
		wasfullscreen = fullscreen;
		if (fullscreen)
		{
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else
			Impl_ApplyWindowedStyle(width, height, reposition);
	}

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		OglSdlSurface(vid.width, vid.height);
	}
#endif

	if (rendermode == render_soft)
	{
		if (renderer)
		{
			SDL_RenderClear(renderer);
			SDL_RenderSetLogicalSize(renderer, width, height);
		}

		realwidth = width;
		realheight = height;
		if (texture != NULL)
		{
			SDL_DestroyTexture(texture);
			texture = NULL;
		}

		if (!usesdl2soft)
		{
			sw_texture_format = SDL_PIXELFORMAT_RGB565;
		}
		else
		{
			bpp = 32;
			sw_texture_format = SDL_PIXELFORMAT_RGBA8888;
		}

		if (renderer)
		{
			texture = SDL_CreateTexture(renderer, sw_texture_format, SDL_TEXTUREACCESS_STREAMING, width, height);
			if (texture == NULL)
				I_Error("Failed to create the software screen texture: %s\n", SDL_GetError());
		}


		if (vidSurface != NULL)
		{
			SDL_FreeSurface(vidSurface);
		}
		SDL_PixelFormatEnumToMasks(sw_texture_format, &bpp, &rmask, &gmask, &bmask, &amask);
		vidSurface = SDL_CreateRGBSurface(0, width, height, bpp, rmask, gmask, bmask, amask);
		if (vidSurface == NULL)
			I_Error("Failed to create the software screen surface: %s\n", SDL_GetError());
	}
}

static void VidWaitChanged(void)
{
#ifdef HWRENDER
	if (software_gl_presenter && sdlglcontext != NULL
	&& SDL_GL_GetCurrentContext() == sdlglcontext)
	{
		OglSdlSetVSync(cv_vidwait.value, true);
	}
	else
#endif
	if (renderer && rendermode == render_soft)
	{
#if SDL_VERSION_ATLEAST(2, 0, 18)
		SDL_RenderSetVSync(renderer, cv_vidwait.value ? 1 : 0);
#endif
	}
#ifdef HWRENDER
	else if (rendermode == render_opengl && sdlglcontext != NULL && SDL_GL_GetCurrentContext() == sdlglcontext)
	{
		OglSdlSetVSync(cv_vidwait.value, true);
	}
#endif
}

static void WindowedStyleChanged(void)
{
	                                                                         
                                                   
	if (cv_windowedstyle.value)
		CV_StealthSetValue(&cv_windowedstyle, 0);

	if (!window || USE_FULLSCREEN)
		return;

	Impl_ApplyWindowedStyle(vid.width, vid.height, SDL_TRUE);
#ifdef HWRENDER
	if (sdlglcontext != NULL && SDL_GL_GetCurrentContext() == sdlglcontext)
		OglSdlSetVSync(cv_vidwait.value, true);
#endif
}

static INT32 Impl_SDL_Scancode_To_Keycode(SDL_Scancode code)
{
	if (code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z)
	{

		return code - SDL_SCANCODE_A + 'a';
	}
	if (code >= SDL_SCANCODE_1 && code <= SDL_SCANCODE_9)
	{
		return code - SDL_SCANCODE_1 + '1';
	}
	else if (code == SDL_SCANCODE_0)
	{
		return '0';
	}
	if (code >= SDL_SCANCODE_F1 && code <= SDL_SCANCODE_F10)
	{
		return KEY_F1 + (code - SDL_SCANCODE_F1);
	}
	switch (code)
	{

		case SDL_SCANCODE_F11: return KEY_F11;
		case SDL_SCANCODE_F12: return KEY_F12;

		case SDL_SCANCODE_KP_0: return KEY_KEYPAD0;
		case SDL_SCANCODE_KP_1: return KEY_KEYPAD1;
		case SDL_SCANCODE_KP_2: return KEY_KEYPAD2;
		case SDL_SCANCODE_KP_3: return KEY_KEYPAD3;
		case SDL_SCANCODE_KP_4: return KEY_KEYPAD4;
		case SDL_SCANCODE_KP_5: return KEY_KEYPAD5;
		case SDL_SCANCODE_KP_6: return KEY_KEYPAD6;
		case SDL_SCANCODE_KP_7: return KEY_KEYPAD7;
		case SDL_SCANCODE_KP_8: return KEY_KEYPAD8;
		case SDL_SCANCODE_KP_9: return KEY_KEYPAD9;

		case SDL_SCANCODE_RETURN:         return KEY_ENTER;
		case SDL_SCANCODE_ESCAPE:         return KEY_ESCAPE;
		case SDL_SCANCODE_BACKSPACE:      return KEY_BACKSPACE;
		case SDL_SCANCODE_TAB:            return KEY_TAB;
		case SDL_SCANCODE_SPACE:          return KEY_SPACE;
		case SDL_SCANCODE_MINUS:          return KEY_MINUS;
		case SDL_SCANCODE_EQUALS:         return KEY_EQUALS;
		case SDL_SCANCODE_LEFTBRACKET:    return '[';
		case SDL_SCANCODE_RIGHTBRACKET:   return ']';
		case SDL_SCANCODE_BACKSLASH:      return '\\';
		case SDL_SCANCODE_NONUSHASH:      return '#';
		case SDL_SCANCODE_SEMICOLON:      return ';';
		case SDL_SCANCODE_APOSTROPHE:     return '\'';
		case SDL_SCANCODE_GRAVE:          return '`';
		case SDL_SCANCODE_COMMA:          return ',';
		case SDL_SCANCODE_PERIOD:         return '.';
		case SDL_SCANCODE_SLASH:          return '/';
		case SDL_SCANCODE_CAPSLOCK:       return KEY_CAPSLOCK;
		case SDL_SCANCODE_PRINTSCREEN:    return 0;  
		case SDL_SCANCODE_SCROLLLOCK:     return KEY_SCROLLLOCK;
		case SDL_SCANCODE_PAUSE:          return KEY_PAUSE;
		case SDL_SCANCODE_INSERT:         return KEY_INS;
		case SDL_SCANCODE_HOME:           return KEY_HOME;
		case SDL_SCANCODE_PAGEUP:         return KEY_PGUP;
		case SDL_SCANCODE_DELETE:         return KEY_DEL;
		case SDL_SCANCODE_END:            return KEY_END;
		case SDL_SCANCODE_PAGEDOWN:       return KEY_PGDN;
		case SDL_SCANCODE_RIGHT:          return KEY_RIGHTARROW;
		case SDL_SCANCODE_LEFT:           return KEY_LEFTARROW;
		case SDL_SCANCODE_DOWN:           return KEY_DOWNARROW;
		case SDL_SCANCODE_UP:             return KEY_UPARROW;
		case SDL_SCANCODE_NUMLOCKCLEAR:   return KEY_NUMLOCK;
		case SDL_SCANCODE_KP_DIVIDE:      return KEY_KPADSLASH;
		case SDL_SCANCODE_KP_MULTIPLY:    return '*';  
		case SDL_SCANCODE_KP_MINUS:       return KEY_MINUSPAD;
		case SDL_SCANCODE_KP_PLUS:        return KEY_PLUSPAD;
		case SDL_SCANCODE_KP_ENTER:       return KEY_ENTER;
		case SDL_SCANCODE_KP_PERIOD:      return KEY_KPADDEL;
		case SDL_SCANCODE_NONUSBACKSLASH: return '\\';

		case SDL_SCANCODE_LSHIFT: return KEY_LSHIFT;
		case SDL_SCANCODE_RSHIFT: return KEY_RSHIFT;
		case SDL_SCANCODE_LCTRL:  return KEY_LCTRL;
		case SDL_SCANCODE_RCTRL:  return KEY_RCTRL;
		case SDL_SCANCODE_LALT:   return KEY_LALT;
		case SDL_SCANCODE_RALT:   return KEY_RALT;
		case SDL_SCANCODE_LGUI:   return KEY_LEFTWIN;
		case SDL_SCANCODE_RGUI:   return KEY_RIGHTWIN;
		default:                  break;
	}

	return 0;
}

static boolean ShouldIgnoreMouse(void)
{
	if (cv_alwaysgrabmouse.value)
		return false;
	if (menuactive)
		return !M_MouseNeeded();
	if (paused || con_destlines || chat_on)
		return true;
	if (gamestate != GS_LEVEL && gamestate != GS_INTERMISSION &&
			gamestate != GS_CONTINUING && gamestate != GS_CUTSCENE)
		return true;
	return false;
}

static boolean ShouldGrabMouse(void)
{
	if (cv_alwaysgrabmouse.value)
		return true;
	if (ShouldIgnoreMouse())
		return false;
	if (!mousegrabbedbylua)
		return false;
	return true;
}

static void SDLdoGrabMouse(void)
{
	SDL_ShowCursor(SDL_DISABLE);
	SDL_SetWindowGrab(window, SDL_TRUE);
	if (SDL_SetRelativeMouseMode(SDL_TRUE) == 0)  
		wrapmouseok = SDL_TRUE;  
}

                                                                            
                                                                              
                                                                          
                                    
static void SDLreacquireMouse(void)
{
	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_SetWindowGrab(window, SDL_FALSE);
	SDLdoGrabMouse();
}

static void SDLdoUngrabMouse(void)
{
	SDL_ShowCursor(SDL_ENABLE);
	SDL_SetWindowGrab(window, SDL_FALSE);
	wrapmouseok = SDL_FALSE;
	SDL_SetRelativeMouseMode(SDL_FALSE);
}

void SDLforceUngrabMouse(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO)==SDL_INIT_VIDEO && window != NULL)
		SDLdoUngrabMouse();
}

void I_UpdateMouseGrab(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO) == SDL_INIT_VIDEO && window != NULL
	&& SDL_GetKeyboardFocus() == window
	&& USE_MOUSEINPUT && ShouldGrabMouse())
		SDLdoGrabMouse();
}

void I_SetMouseGrab(boolean grab)
{
	if (grab)
		SDLdoGrabMouse();
	else
		SDLdoUngrabMouse();
}

static void VID_Command_NumModes_f (void)
{
	CONS_Printf(M_GetText("%d video mode(s) available(s)\n"), VID_NumModes());
}


static void SurfaceInfo(const SDL_Surface *infoSurface, const char *SurfaceText)
{
	INT32 vfBPP;

	if (!infoSurface)
		return;

	if (!SurfaceText)
		SurfaceText = M_GetText("Unknown Surface");

	vfBPP = infoSurface->format?infoSurface->format->BitsPerPixel:0;

	CONS_Printf("\x82" "%s\n", SurfaceText);
	CONS_Printf(M_GetText(" %ix%i at %i bit color\n"), infoSurface->w, infoSurface->h, vfBPP);

	if (infoSurface->flags&SDL_PREALLOC)
		CONS_Printf("%s", M_GetText(" Uses preallocated memory\n"));
	else
		CONS_Printf("%s", M_GetText(" Stored in system memory\n"));
	if (infoSurface->flags&SDL_RLEACCEL)
		CONS_Printf("%s", M_GetText(" Colorkey RLE acceleration blit\n"));
}

static void VID_Command_Info_f (void)
{
#if 0
	SDL2STUB();
#else
#if 0
	const SDL_VideoInfo *videoInfo;
	videoInfo = SDL_GetVideoInfo();  
	if (videoInfo)
	{
		CONS_Printf("%s", M_GetText("Video Interface Capabilities:\n"));
		if (videoInfo->hw_available)
			CONS_Printf("%s", M_GetText(" Hardware surfaces\n"));
		if (videoInfo->wm_available)
			CONS_Printf("%s", M_GetText(" Window manager\n"));


		if (videoInfo->blit_hw)
			CONS_Printf("%s", M_GetText(" Accelerated blits HW-2-HW\n"));
		if (videoInfo->blit_hw_CC)
			CONS_Printf("%s", M_GetText(" Accelerated blits HW-2-HW with Colorkey\n"));
		if (videoInfo->wm_available)
			CONS_Printf("%s", M_GetText(" Accelerated blits HW-2-HW with Alpha\n"));
		if (videoInfo->blit_sw)
		{
			CONS_Printf("%s", M_GetText(" Accelerated blits SW-2-HW\n"));
			if (!M_CheckParm("-noblit")) videoblitok = SDL_TRUE;
		}
		if (videoInfo->blit_sw_CC)
			CONS_Printf("%s", M_GetText(" Accelerated blits SW-2-HW with Colorkey\n"));
		if (videoInfo->blit_sw_A)
			CONS_Printf("%s", M_GetText(" Accelerated blits SW-2-HW with Alpha\n"));
		if (videoInfo->blit_fill)
			CONS_Printf("%s", M_GetText(" Accelerated Color filling\n"));

		if (videoInfo->video_mem)
			CONS_Printf(M_GetText(" There is %i KB of video memory\n"), videoInfo->video_mem);
		else
			CONS_Printf("%s", M_GetText(" There no video memory for SDL\n"));

	}
#else
	if (!M_CheckParm("-noblit")) videoblitok = SDL_TRUE;
#endif
	SurfaceInfo(bufSurface, M_GetText("Current Engine Mode"));
	SurfaceInfo(vidSurface, M_GetText("Current Video Mode"));
	CONS_Printf(" Window: %dx%d %s on display %d\n",
		vid.width, vid.height, USE_FULLSCREEN ? "fullscreen" : "windowed",
		window ? SDL_GetWindowDisplayIndex(window) : -1);
	CONS_Printf(" Frame pacing: %.2f FPS current, %u Hz display, %s cap (%u effective)\n",
		averageFPS, refresh_rate, cv_fpscap.string, R_GetFramerateCap());
#ifdef HWRENDER
	if (sdlglcontext != NULL && SDL_GL_GetCurrentContext() == sdlglcontext)
	{
		if (cv_vidwait.value && OglSdlCompositorProvidesPacing())
			CONS_Printf(" VSync: %s, Windows compositor pacing\n",
				cv_vidwait.string);
		else
			CONS_Printf(" VSync: %s, active OpenGL swap interval %d\n",
				cv_vidwait.string, SDL_GL_GetSwapInterval());
	}
	else
#endif
		CONS_Printf(" VSync: %s\n", cv_vidwait.string);
#endif
}

static void VID_Command_ModeList_f(void)
{

	INT32 i = 0;
	CONS_Printf("NOTE: Under SDL2, all modes are supported on all platforms.\n");
	CONS_Printf("Under opengl, fullscreen only supports native desktop resolution.\n");
	CONS_Printf("Under software, the mode is stretched up to desktop resolution.\n");
	for (i = 0; i < MAXWINMODES; i++)
	{
		CONS_Printf("%2d: %dx%d\n", i, windowedModes[i][0], windowedModes[i][1]);
	}

}

static void VID_Command_Mode_f (void)
{
	INT32 modenum;

	if (COM_Argc()!= 2)
	{
		CONS_Printf(M_GetText("vid_mode <modenum> : set video mode, current video mode %i\n"), vid.modenum);
		return;
	}

	modenum = atoi(COM_Argv(1));

	if (modenum >= VID_NumModes())
		CONS_Printf(M_GetText("Video mode not present\n"));
	else
		setmodeneeded = modenum+1;  
}

static inline void SDLJoyRemap(event_t *event)
{
	(void)event;
}

static INT32 SDLJoyAxis(const Sint16 axis, evtype_t which)
{

	INT32 raxis = axis/32;
	if (which == ev_joystick)
	{
		if (Joystick.bGamepadStyle)
		{

			if (raxis < -(JOYAXISRANGE/2))
				raxis = -1;
			else if (raxis > (JOYAXISRANGE/2))
				raxis = 1;
			else
				raxis = 0;
		}
		else
		{
			raxis = JoyInfo.scale!=1?((raxis/JoyInfo.scale)*JoyInfo.scale):raxis;

#ifdef SDL_JDEADZONE
			if (-SDL_JDEADZONE <= raxis && raxis <= SDL_JDEADZONE)
				raxis = 0;
#endif
		}
	}
	else if (which == ev_joystick2)
	{
		if (Joystick2.bGamepadStyle)
		{

			if (raxis < -(JOYAXISRANGE/2))
				raxis = -1;
			else if (raxis > (JOYAXISRANGE/2))
				raxis = 1;
			else raxis = 0;
		}
		else
		{
			raxis = JoyInfo2.scale!=1?((raxis/JoyInfo2.scale)*JoyInfo2.scale):raxis;

#ifdef SDL_JDEADZONE
			if (-SDL_JDEADZONE <= raxis && raxis <= SDL_JDEADZONE)
				raxis = 0;
#endif
		}
	}
	return raxis;
}

static void Impl_HandleWindowEvent(SDL_WindowEvent evt)
{
	static SDL_bool firsttimeonmouse = SDL_TRUE;
	static SDL_bool mousefocus = SDL_TRUE;
	static SDL_bool kbfocus = SDL_TRUE;
	SDL_bool reacquire = SDL_FALSE;

	switch (evt.event)
	{
		case SDL_WINDOWEVENT_ENTER:
			mousefocus = SDL_TRUE;
			break;
		case SDL_WINDOWEVENT_LEAVE:
			mousefocus = SDL_FALSE;
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			kbfocus = SDL_TRUE;
			mousefocus = SDL_TRUE;
			reacquire = SDL_TRUE;
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			kbfocus = SDL_FALSE;
			mousefocus = SDL_FALSE;
			break;
		case SDL_WINDOWEVENT_MAXIMIZED:
			break;
		case SDL_WINDOWEVENT_MOVED:
#if SDL_VERSION_ATLEAST(2, 0, 18)
		case SDL_WINDOWEVENT_DISPLAY_CHANGED:
#endif
		{
			UINT32 newrate = VID_GetRefreshRate();
			if (newrate && newrate != refresh_rate)
			{
				refresh_rate = newrate;
				CONS_Printf("Display refresh rate: %u Hz\n", refresh_rate);
			}
			break;
		}
	}

	if (mousefocus && kbfocus)
	{

		window_notinfocus = false;
		if (!paused)
			S_ResumeAudio();  

		if (!firsttimeonmouse)
		{
			if (cv_usemouse.value) I_StartupMouse();
		}


		if (USE_MOUSEINPUT && ShouldGrabMouse())
		{
			if (reacquire)
				SDLreacquireMouse();
			else
				SDLdoGrabMouse();
		}
	}
	else if (!mousefocus && !kbfocus)
	{

		window_notinfocus = true;
		if (! cv_playmusicifunfocused.value)
			S_PauseAudio();
		if (! cv_playsoundsifunfocused.value)
			S_StopSounds();

		if (!disable_mouse)
		{
			SDLforceUngrabMouse();
		}
		memset(gamekeydown, 0, NUMKEYS);  

		if (MOUSE_MENU)
		{
			SDLdoUngrabMouse();
		}
	}

}

static void Impl_HandleKeyboardEvent(SDL_KeyboardEvent evt, Uint32 type)
{
	event_t event;
	if (type == SDL_KEYUP)
	{
		event.type = ev_keyup;
	}
	else if (type == SDL_KEYDOWN)
	{
		event.type = ev_keydown;
	}
	else
	{
		return;
	}
	event.key = Impl_SDL_Scancode_To_Keycode(evt.keysym.scancode);
	event.repeated = (evt.repeat != 0);
	if (event.key) D_PostEvent(&event);
}

static void Impl_HandleTextEvent(SDL_TextInputEvent evt)
{
	event_t event;
	event.type = ev_text;
	if (evt.text[1] != '\0')
	{

		return;
	}
	event.key = evt.text[0];
	D_PostEvent(&event);
}

static void Impl_HandleMouseMotionEvent(SDL_MouseMotionEvent evt)
{
	static boolean firstmove = true;

	if (USE_MOUSEINPUT)
	{
		if ((SDL_GetMouseFocus() != window && SDL_GetKeyboardFocus() != window) || (!ShouldGrabMouse() && !firstmove))
		{
			SDLdoUngrabMouse();
			firstmove = false;
			return;
		}



		if (SDL_GetRelativeMouseMode())
		{
			if (SDL_GetMouseFocus() == window && SDL_GetKeyboardFocus() == window)
			{
				mousemovex += evt.xrel;
				mousemovey += evt.yrel;
				SDL_SetWindowGrab(window, SDL_TRUE);
			}
			firstmove = false;
			return;
		}



		if ((evt.x == realwidth/2) && (evt.y == realheight/2))
		{
			firstmove = false;
			return;
		}





		if (SDL_GetMouseFocus() == window && SDL_GetKeyboardFocus() == window)
		{
			SDLdoGrabMouse();
		}
	}

	firstmove = false;
}

static void Impl_HandleMouseButtonEvent(SDL_MouseButtonEvent evt, Uint32 type)
{
	event_t event;

	SDL_memset(&event, 0, sizeof(event_t));






	if (SDL_GetMouseFocus() != window
	 || (ShouldIgnoreMouse() && !chat_on && !con_destlines))
		return;


	if (USE_MOUSEINPUT)
	{
		if (type == SDL_MOUSEBUTTONUP)
		{
			event.type = ev_keyup;
		}
		else if (type == SDL_MOUSEBUTTONDOWN)
		{
			event.type = ev_keydown;
		}
		else return;
		if (evt.button == SDL_BUTTON_MIDDLE)
			event.key = KEY_MOUSE1+2;
		else if (evt.button == SDL_BUTTON_RIGHT)
			event.key = KEY_MOUSE1+1;
		else if (evt.button == SDL_BUTTON_LEFT)
			event.key = KEY_MOUSE1;
		else if (evt.button == SDL_BUTTON_X1)
			event.key = KEY_MOUSE1+3;
		else if (evt.button == SDL_BUTTON_X2)
			event.key = KEY_MOUSE1+4;
		if (event.type == ev_keyup || event.type == ev_keydown)
		{
			D_PostEvent(&event);
		}
	}
}

static void Impl_HandleMouseWheelEvent(SDL_MouseWheelEvent evt)
{
	event_t event;

	SDL_memset(&event, 0, sizeof(event_t));

	if (evt.y > 0)
	{
		event.key = KEY_MOUSEWHEELUP;
		event.type = ev_keydown;
	}
	if (evt.y < 0)
	{
		event.key = KEY_MOUSEWHEELDOWN;
		event.type = ev_keydown;
	}
	if (evt.y == 0)
	{
		event.key = 0;
		event.type = ev_keyup;
	}
	if (event.type == ev_keyup || event.type == ev_keydown)
	{
		D_PostEvent(&event);
	}
}

static void Impl_HandleJoystickAxisEvent(SDL_JoyAxisEvent evt)
{
	event_t event;
	SDL_JoystickID joyid[2];


	joyid[0] = SDL_JoystickInstanceID(JoyInfo.dev);
	joyid[1] = SDL_JoystickInstanceID(JoyInfo2.dev);

	evt.axis++;
	event.key = event.x = event.y = INT32_MAX;

	if (evt.which == joyid[0])
	{
		event.type = ev_joystick;
	}
	else if (evt.which == joyid[1])
	{
		event.type = ev_joystick2;
	}
	else return;

	if (evt.axis > JOYAXISSET*2)
		return;

	if (evt.axis%2)
	{
		event.key = evt.axis / 2;
		event.x = SDLJoyAxis(evt.value, event.type);
	}
	else
	{
		evt.axis--;
		event.key = evt.axis / 2;
		event.y = SDLJoyAxis(evt.value, event.type);
	}
	D_PostEvent(&event);
}

#if 0
static void Impl_HandleJoystickHatEvent(SDL_JoyHatEvent evt)
{
	event_t event;
	SDL_JoystickID joyid[2];


	joyid[0] = SDL_JoystickInstanceID(JoyInfo.dev);
	joyid[1] = SDL_JoystickInstanceID(JoyInfo2.dev);

	if (evt.hat >= JOYHATS)
		return;  

	if (evt.which == joyid[0])
	{
		event.key = KEY_HAT1 + (evt.hat*4);
	}
	else if (evt.which == joyid[1])
	{
		event.key = KEY_2HAT1 + (evt.hat*4);
	}
	else return;


}
#endif

static void Impl_HandleJoystickButtonEvent(SDL_JoyButtonEvent evt, Uint32 type)
{
	event_t event;
	SDL_JoystickID joyid[2];


	joyid[0] = SDL_JoystickInstanceID(JoyInfo.dev);
	joyid[1] = SDL_JoystickInstanceID(JoyInfo2.dev);

	if (evt.which == joyid[0])
	{
		event.key = KEY_JOY1;
	}
	else if (evt.which == joyid[1])
	{
		event.key = KEY_2JOY1;
	}
	else return;
	if (type == SDL_JOYBUTTONUP)
	{
		event.type = ev_keyup;
	}
	else if (type == SDL_JOYBUTTONDOWN)
	{
		event.type = ev_keydown;
	}
	else return;
	if (evt.button < JOYBUTTONS)
	{
		event.key += evt.button;
	}
	else return;

	SDLJoyRemap(&event);
	if (event.type != ev_console) D_PostEvent(&event);
}



void I_GetEvent(void)
{
	SDL_Event evt;




	if (!graphics_started)
	{
		return;
	}

	mousemovex = mousemovey = 0;

	while (SDL_PollEvent(&evt))
	{
		switch (evt.type)
		{
			case SDL_WINDOWEVENT:
				Impl_HandleWindowEvent(evt.window);
				break;
			case SDL_KEYUP:
			case SDL_KEYDOWN:
				Impl_HandleKeyboardEvent(evt.key, evt.type);
				break;
			case SDL_TEXTINPUT:
				Impl_HandleTextEvent(evt.text);
				break;
			case SDL_MOUSEMOTION:

				Impl_HandleMouseMotionEvent(evt.motion);

				break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				Impl_HandleMouseButtonEvent(evt.button, evt.type);
				break;
			case SDL_MOUSEWHEEL:
				Impl_HandleMouseWheelEvent(evt.wheel);
				break;
			case SDL_JOYAXISMOTION:
				Impl_HandleJoystickAxisEvent(evt.jaxis);
				break;
#if 0
			case SDL_JOYHATMOTION:
				Impl_HandleJoystickHatEvent(evt.jhat)
				break;
#endif
			case SDL_JOYBUTTONUP:
			case SDL_JOYBUTTONDOWN:
				Impl_HandleJoystickButtonEvent(evt.jbutton, evt.type);
				break;
			case SDL_JOYDEVICEADDED:
				{
					SDL_Joystick *newjoy = SDL_JoystickOpen(evt.jdevice.which);

					CONS_Debug(DBG_GAMELOGIC, "Joystick device index %d added\n", evt.jdevice.which + 1);







					if (newjoy && (!JoyInfo.dev || !SDL_JoystickGetAttached(JoyInfo.dev))
						&& JoyInfo2.dev != newjoy)  
					{
						cv_usejoystick.value = evt.jdevice.which + 1;

						if (JoyInfo2.dev)
							cv_usejoystick2.value = I_GetJoystickDeviceIndex(JoyInfo2.dev) + 1;
						else if (atoi(cv_usejoystick2.string) != JoyInfo.oldjoy
								&& atoi(cv_usejoystick2.string) != cv_usejoystick.value)
							cv_usejoystick2.value = atoi(cv_usejoystick2.string);
						else if (atoi(cv_usejoystick.string) != JoyInfo.oldjoy
								&& atoi(cv_usejoystick.string) != cv_usejoystick.value)
							cv_usejoystick2.value = atoi(cv_usejoystick.string);
						else  
							cv_usejoystick2.value = 0;
					}
					else if (newjoy && (!JoyInfo2.dev || !SDL_JoystickGetAttached(JoyInfo2.dev))
						&& JoyInfo.dev != newjoy)  
					{
						cv_usejoystick2.value = evt.jdevice.which + 1;

						if (JoyInfo.dev)
							cv_usejoystick.value = I_GetJoystickDeviceIndex(JoyInfo.dev) + 1;
						else if (atoi(cv_usejoystick.string) != JoyInfo2.oldjoy
								&& atoi(cv_usejoystick.string) != cv_usejoystick2.value)
							cv_usejoystick.value = atoi(cv_usejoystick.string);
						else if (atoi(cv_usejoystick2.string) != JoyInfo2.oldjoy
								&& atoi(cv_usejoystick2.string) != cv_usejoystick2.value)
							cv_usejoystick.value = atoi(cv_usejoystick2.string);
						else  
							cv_usejoystick.value = 0;
					}


					if (!strcmp(cv_usejoystick.string, "0") || !cv_usejoystick.value)
						cv_usejoystick.value = 0;
					else if (atoi(cv_usejoystick.string) <= I_NumJoys()  
						     && cv_usejoystick.value)  
						CV_SetValue(&cv_usejoystick, cv_usejoystick.value);

					if (!strcmp(cv_usejoystick2.string, "0") || !cv_usejoystick2.value)
						cv_usejoystick2.value = 0;
					else if (atoi(cv_usejoystick2.string) <= I_NumJoys()  
					         && cv_usejoystick2.value)  
						CV_SetValue(&cv_usejoystick2, cv_usejoystick2.value);





					I_InitJoystick();
					I_InitJoystick2();

					CONS_Debug(DBG_GAMELOGIC, "Joystick1 device index: %d\n", JoyInfo.oldjoy);
					CONS_Debug(DBG_GAMELOGIC, "Joystick2 device index: %d\n", JoyInfo2.oldjoy);


					if (currentMenu == &OP_JoystickSetDef)
						M_SetupJoystickMenu(0);

					if (JoyInfo.dev != newjoy && JoyInfo2.dev != newjoy)
						SDL_JoystickClose(newjoy);
				}
				break;
			case SDL_JOYDEVICEREMOVED:
				if (JoyInfo.dev && !SDL_JoystickGetAttached(JoyInfo.dev))
				{
					CONS_Debug(DBG_GAMELOGIC, "Joystick1 removed, device index: %d\n", JoyInfo.oldjoy);
					I_ShutdownJoystick();
				}

				if (JoyInfo2.dev && !SDL_JoystickGetAttached(JoyInfo2.dev))
				{
					CONS_Debug(DBG_GAMELOGIC, "Joystick2 removed, device index: %d\n", JoyInfo2.oldjoy);
					I_ShutdownJoystick2();
				}




				if (JoyInfo.dev)
					cv_usejoystick.value = JoyInfo.oldjoy = I_GetJoystickDeviceIndex(JoyInfo.dev) + 1;
				else if (atoi(cv_usejoystick.string) != JoyInfo2.oldjoy)
					cv_usejoystick.value = atoi(cv_usejoystick.string);
				else if (atoi(cv_usejoystick2.string) != JoyInfo2.oldjoy)
					cv_usejoystick.value = atoi(cv_usejoystick2.string);
				else  
					cv_usejoystick.value = 0;

				if (JoyInfo2.dev)
					cv_usejoystick2.value = JoyInfo2.oldjoy = I_GetJoystickDeviceIndex(JoyInfo2.dev) + 1;
				else if (atoi(cv_usejoystick2.string) != JoyInfo.oldjoy)
					cv_usejoystick2.value = atoi(cv_usejoystick2.string);
				else if (atoi(cv_usejoystick.string) != JoyInfo.oldjoy)
					cv_usejoystick2.value = atoi(cv_usejoystick.string);
				else  
					cv_usejoystick2.value = 0;


				if (!strcmp(cv_usejoystick.string, "0"))
					cv_usejoystick.value = 0;
				else if (atoi(cv_usejoystick.string) <= I_NumJoys()  
						 && cv_usejoystick.value)  
					CV_SetValue(&cv_usejoystick, cv_usejoystick.value);

				if (!strcmp(cv_usejoystick2.string, "0"))
					cv_usejoystick2.value = 0;
				else if (atoi(cv_usejoystick2.string) <= I_NumJoys()  
						 && cv_usejoystick2.value)  
					CV_SetValue(&cv_usejoystick2, cv_usejoystick2.value);

				CONS_Debug(DBG_GAMELOGIC, "Joystick1 device index: %d\n", JoyInfo.oldjoy);
				CONS_Debug(DBG_GAMELOGIC, "Joystick2 device index: %d\n", JoyInfo2.oldjoy);


				if (currentMenu == &OP_JoystickSetDef)
					M_SetupJoystickMenu(0);
				break;
			case SDL_QUIT:
				LUA_HookBool(true, HOOK(GameQuit));
				I_Quit();
				break;
		}
	}


	if (mousemovex || mousemovey)
	{
		event_t event;
		int wwidth, wheight;
		SDL_GetWindowSize(window, &wwidth, &wheight);

		event.type = ev_mouse;
		event.key = 0;
		event.x = (INT32)lround(mousemovex * ((float)wwidth / (float)realwidth));
		event.y = (INT32)lround(mousemovey * ((float)wheight / (float)realheight));
		D_PostEvent(&event);
	}



	gamekeydown[KEY_MOUSEWHEELDOWN] = gamekeydown[KEY_MOUSEWHEELUP] = 0;
}

void I_StartupMouse(void)
{
	static SDL_bool firsttimeonmouse = SDL_TRUE;

	if (disable_mouse)
		return;

	if (!firsttimeonmouse)
	{
		HalfWarpMouse(realwidth, realheight);  
	}
	else
		firsttimeonmouse = SDL_FALSE;
	if (cv_usemouse.value && ShouldGrabMouse())
		SDLdoGrabMouse();
	else
		SDLdoUngrabMouse();
}




void I_OsPolling(void)
{
	SDL_Keymod mod;

	if (consolevent)
		I_GetConsoleEvents();
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == SDL_INIT_JOYSTICK)
	{
		SDL_JoystickUpdate();
		I_GetJoystickEvents();
		I_GetJoystick2Events();
	}

	I_GetMouseEvents();

	I_GetEvent();

	mod = SDL_GetModState();
	 
	shiftdown = ctrldown = altdown = 0;
	capslock = false;
	if (mod & KMOD_LSHIFT) shiftdown |= 1;
	if (mod & KMOD_RSHIFT) shiftdown |= 2;
	if (mod & KMOD_LCTRL)   ctrldown |= 1;
	if (mod & KMOD_RCTRL)   ctrldown |= 2;
	if (mod & KMOD_LALT)     altdown |= 1;
	if (mod & KMOD_RALT)     altdown |= 2;
	if (mod & KMOD_CAPS) capslock = true;
}




void I_UpdateNoBlit(void)
{
	if (rendermode == render_none)
		return;
	if (exposevideo)
	{
#ifdef HWRENDER
		if (rendermode == render_opengl)
		{
			OglSdlFinishUpdate(cv_vidwait.value);
		}
		else
#endif
		if (rendermode == render_soft)
		{
			SDL_RenderCopy(renderer, texture, NULL, NULL);
			SDL_RenderPresent(renderer);
		}
	}
	exposevideo = SDL_FALSE;
}





static inline boolean I_SkipFrame(void)
{
#if 1




	return false;
#else
	static boolean skip = false;

	skip = !skip;

	switch (gamestate)
	{
		case GS_LEVEL:
			if (!paused)
				return false;
			 

		case GS_WAITINGPLAYERS:
			return skip;  
		default:
			return false;
	}
#endif
}




static SDL_Rect src_rect = { 0, 0, 0, 0 };

void I_FinishUpdate(void)
{
	if (rendermode == render_none)
		return;  

	SCR_CalculateFPS();

	if (I_SkipFrame())
		return;

	if (marathonmode)
		SCR_DisplayMarathonInfo();


	if (cv_closedcaptioning.value)
		SCR_ClosedCaptions();

	if (cv_ticrate.value)
		SCR_DisplayTicRate();

	if (cv_showping.value && netgame && consoleplayer != serverplayer)
		SCR_DisplayLocalPing();

	if (rendermode == render_soft && screens[0])
	{
		if (!bufSurface)  
		{
			Impl_VideoSetupSDLBuffer();
		}

		if (bufSurface)
		{
			SDL_BlitSurface(bufSurface, &src_rect, vidSurface, &src_rect);
		}

#ifdef HWRENDER
		if (software_gl_presenter)
		{
			OglSdlPresentSoftware(vidSurface->pixels, vid.width, vid.height, cv_vidwait.value);
		}
		else
#endif
		if (renderer && texture)
		{


			SDL_UpdateTexture(texture, &src_rect, vidSurface->pixels, vidSurface->pitch);
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, &src_rect, NULL);
			SDL_RenderPresent(renderer);
		}
	}
#ifdef HWRENDER
	else if (rendermode == render_opengl)
	{

		if (HWR_ShouldUsePaletteRendering())
		{
			HWD.pfnMakeScreenTexture(HWD_SCREENTEXTURE_GENERIC2);
			HWD.pfnSetShader(HWR_GetShaderFromTarget(SHADER_PALETTE_POSTPROCESS));
			HWD.pfnDrawScreenTexture(HWD_SCREENTEXTURE_GENERIC2, NULL, 0);
			HWD.pfnUnSetShader();
		}
		OglSdlFinishUpdate(cv_vidwait.value);
	}
#endif

	if (window_needs_first_present && window)
	{
		SDL_ShowWindow(window);
		SDL_RaiseWindow(window);
		window_needs_first_present = SDL_FALSE;
	}

	exposevideo = SDL_FALSE;
}




void I_UpdateNoVsync(void)
{
	INT32 real_vidwait = cv_vidwait.value;
	cv_vidwait.value = 0;
	I_FinishUpdate();
	cv_vidwait.value = real_vidwait;
}




void I_ReadScreen(UINT8 *scr)
{
	if (rendermode != render_soft)
		I_Error ("I_ReadScreen: called while in non-software mode");
	else
		VID_BlitLinearScreen(screens[0], scr,
			vid.width*vid.bpp, vid.height,
			vid.rowbytes, vid.rowbytes);
}




void I_SetPalette(RGBA_t *palette)
{
	size_t i;
	for (i=0; i<256; i++)
	{
		localPalette[i].r = palette[i].s.red;
		localPalette[i].g = palette[i].s.green;
		localPalette[i].b = palette[i].s.blue;
	}


	if (bufSurface) SDL_SetPaletteColors(bufSurface->format->palette, localPalette, 0, 256);
}


INT32 VID_NumModes(void)
{
	if (USE_FULLSCREEN && numVidModes != -1)
		return numVidModes - firstEntry;
	else
		return MAXWINMODES;
}

const char *VID_GetModeName(INT32 modeNum)
{
#if 0
	if (USE_FULLSCREEN && numVidModes != -1)  
	{
		modeNum += firstEntry;
		if (modeNum >= numVidModes)
			return NULL;

		sprintf(&vidModeName[modeNum][0], "%dx%d",
			modeList[modeNum]->w,
			modeList[modeNum]->h);
	}
	else  
	{
#endif
	if (modeNum == -1)
	{
		return fallback_resolution_name;
	}
		if (modeNum >= MAXWINMODES)
			return NULL;

		sprintf(&vidModeName[modeNum][0], "%dx%d",
			windowedModes[modeNum][0],
			windowedModes[modeNum][1]);

	return &vidModeName[modeNum][0];
}

INT32 VID_GetModeForSize(INT32 w, INT32 h)
{
	int i;
	for (i = 0; i < MAXWINMODES; i++)
	{
		if (windowedModes[i][0] == w && windowedModes[i][1] == h)
		{
			return i;
		}
	}
	return -1;
}

void VID_PrepareModeList(void)
{

	allow_fullscreen = true;
}





static void Impl_DestroySoftwareContext(void)
{
#ifdef HWRENDER
	if (software_gl_presenter && window && sdlglcontext)
	{
		if (SDL_GL_GetCurrentContext() != sdlglcontext)
			SDL_GL_MakeCurrent(window, sdlglcontext);
		if (SDL_GL_GetCurrentContext() == sdlglcontext)
			OglSdlDestroySoftwarePresenter();
	}
	software_gl_presenter = SDL_FALSE;
#endif

	if (texture)
	{
		SDL_DestroyTexture(texture);
		texture = NULL;
	}

	if (renderer)
	{


#if SDL_VERSION_ATLEAST(2, 0, 10)
		SDL_RenderFlush(renderer);
#endif
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}

	if (vidSurface)
	{
		SDL_FreeSurface(vidSurface);
		vidSurface = NULL;
	}

	if (bufSurface)
	{
		SDL_FreeSurface(bufSurface);
		bufSurface = NULL;
	}
}

static SDL_bool Impl_CreateContext(void)
{

#ifdef HWRENDER
	if ((rendermode == render_opengl)
	&& (vid.glstate != VID_GL_LIBRARY_ERROR))
	{
		Impl_DestroySoftwareContext();

		if (!sdlglcontext)
			sdlglcontext = SDL_GL_CreateContext(window);
		if (sdlglcontext == NULL)
		{
			SDL_DestroyWindow(window);
			window = NULL;
			I_Error("Failed to create a GL context: %s\n", SDL_GetError());
		}
		if (SDL_GL_MakeCurrent(window, sdlglcontext) != 0)
			I_Error("Failed to activate the GL context: %s\n", SDL_GetError());
		{
			static INT32 reported_samples = -1;
			static INT32 reported_depth = -1;
			INT32 buffers = 0, samples = 0, depth = 0;
			SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buffers);
			SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &samples);
			SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth);
			if (!buffers)
				samples = 0;
			if (samples != reported_samples)
			{
				CONS_Printf("OpenGL edge antialiasing: %s\n",
					samples ? va("%dx MSAA", samples) : "Off");
				reported_samples = samples;
			}
			if (depth != reported_depth)
			{
				CONS_Printf("OpenGL depth buffer: %d bits\n", depth);
				reported_depth = depth;
			}
		}
	}
	else
#endif
	if (rendermode == render_soft)
	{
		int flags = 0;  
#ifdef HWRENDER





		if (sdlglcontext)
		{
			static SDL_bool reported_gl_software_presenter = SDL_FALSE;

			if (SDL_GL_GetCurrentContext() != sdlglcontext
			&& SDL_GL_MakeCurrent(window, sdlglcontext) != 0)
				I_Error("Failed to activate the GL software presenter: %s\n", SDL_GetError());
			software_gl_presenter = SDL_TRUE;
			if (!reported_gl_software_presenter)
			{
				CONS_Printf("Software presentation: OpenGL framebuffer upload\n");
				reported_gl_software_presenter = SDL_TRUE;
			}
			return SDL_TRUE;
		}
#endif
		if (usesdl2soft)
			flags |= SDL_RENDERER_SOFTWARE;
		else if (cv_vidwait.value)
		{
#if SDL_VERSION_ATLEAST(2, 0, 18)

			flags |= SDL_RENDERER_PRESENTVSYNC;
#else




			CV_StealthSetValue(&cv_vidwait, 0);
#endif
		}

		if (!renderer)
			renderer = SDL_CreateRenderer(window, -1, flags);
		if (renderer == NULL)
		{
			CONS_Printf(M_GetText("Couldn't create rendering context: %s\n"), SDL_GetError());
			return SDL_FALSE;
		}
		SDL_RenderSetLogicalSize(renderer, BASEVIDWIDTH, BASEVIDHEIGHT);
	}
	return SDL_TRUE;
}

static INT32 Impl_GetStartupMSAA(void)
{
	INT32 parm = M_CheckParm("-msaa");

	if (parm && M_IsNextParm())
	{
		const char *value = M_GetNextParm();
		INT32 samples = atoi(value);
		return (samples == 2 || samples == 4 || samples == 8 || samples == 16) ? samples : 0;
	}

	                                                                       
                                                                        
                                                                            
                                             
	return 0;
}

void VID_CheckGLLoaded(rendermode_t oldrender)
{
	(void)oldrender;
#ifdef HWRENDER
	if (vid.glstate == VID_GL_LIBRARY_ERROR)  
	{
		CONS_Alert(CONS_ERROR, "OpenGL never loaded\n");
		rendermode = oldrender;
		if (chosenrendermode == render_opengl)  
			rendermode = render_soft;
		if (setrenderneeded)
		{
			CV_StealthSetValue(&cv_renderer, oldrender);
			setrenderneeded = 0;
		}
	}
#endif
}

boolean VID_CheckRenderer(void)
{
	boolean rendererchanged = false;
	boolean contextcreated = false;
#ifdef HWRENDER
	rendermode_t oldrenderer = rendermode;
#endif

	if (dedicated)
		return false;

	if (setrenderneeded)
	{
#if defined(_WIN32) && !defined(DEDICATED)
		I_SetCrashContext(va("switching renderer %d to %d at %dx%d fullscreen=%d",
			rendermode, setrenderneeded, vid.width, vid.height, USE_FULLSCREEN));
#endif
#ifdef HWRENDER


		if (oldrenderer == render_soft && setrenderneeded == render_opengl)
			Impl_DestroySoftwareContext();

#endif

		rendermode = setrenderneeded;
		rendererchanged = true;

#ifdef HWRENDER
		if (rendermode == render_opengl)
		{
			VID_CheckGLLoaded(oldrenderer);



			if (vid.glstate == VID_GL_LIBRARY_NOTLOADED)
			{
				VID_StartupOpenGL();


				if (vid.glstate == VID_GL_LIBRARY_LOADED)
				{
					Impl_DestroySoftwareContext();
					if (!window || !(SDL_GetWindowFlags(window) & SDL_WINDOW_OPENGL))
					{
						if (window)
						{
							SDL_DestroyWindow(window);
							window = NULL;
						}
						Impl_CreateWindow(USE_FULLSCREEN);
					}
					else
						Impl_CreateContext();


					contextcreated = true;
				}
			}
			else if (vid.glstate == VID_GL_LIBRARY_ERROR)
				rendererchanged = false;
		}
#endif

		if (!contextcreated)
			Impl_CreateContext();

		setrenderneeded = 0;
	}

	SDLSetMode(vid.width, vid.height, USE_FULLSCREEN, (setmodeneeded ? SDL_TRUE : SDL_FALSE));
	Impl_VideoSetupBuffer();

	if (rendermode == render_soft)
	{
		if (bufSurface)
		{
			SDL_FreeSurface(bufSurface);
			bufSurface = NULL;
		}

		SCR_SetDrawFuncs();
	}
#ifdef HWRENDER
	else if (rendermode == render_opengl && rendererchanged)
	{
		HWR_Switch();
		V_SetPalette(0);
	}
#endif

#if defined(_WIN32) && !defined(DEDICATED)
	if (rendererchanged)
		I_SetCrashContext(va("renderer %d active at %dx%d fullscreen=%d",
			rendermode, vid.width, vid.height, USE_FULLSCREEN));
#endif

	return rendererchanged;
}

static UINT32 VID_GetRefreshRate(void)
{
	int index = SDL_GetWindowDisplayIndex(window);
	SDL_DisplayMode m;

	if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
	{

		return 0;
	}

	if (SDL_GetCurrentDisplayMode(index, &m) != 0)
	{

		return 0;
	}

	return m.refresh_rate;
}

INT32 VID_SetMode(INT32 modeNum)
{
	SDLdoUngrabMouse();

	vid.recalc = 1;
	vid.bpp = 1;

	if (modeNum < 0)
		modeNum = 0;
	if (modeNum >= MAXWINMODES)
		modeNum = MAXWINMODES-1;

	vid.width = windowedModes[modeNum][0];
	vid.height = windowedModes[modeNum][1];
	vid.modenum = modeNum;


	src_rect.w = vid.width;
	src_rect.h = vid.height;

	refresh_rate = VID_GetRefreshRate();

	VID_CheckRenderer();
	return SDL_TRUE;
}

static SDL_bool Impl_CreateWindow(SDL_bool fullscreen)
{
	int flags = SDL_WINDOW_HIDDEN;
#ifdef HWRENDER
	const INT32 requested_msaa = Impl_GetStartupMSAA();
#endif

	if (rendermode == render_none)  
		return SDL_TRUE;  

	if (window != NULL)
		return SDL_FALSE;

	if (fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

#ifdef HWRENDER
	                                                                        
                                                                           
	flags |= SDL_WINDOW_OPENGL;

	                                                                         
                                                                             
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);


	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	if (requested_msaa > 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, requested_msaa);
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}
#endif


	window = SDL_CreateWindow("SRB2 "VERSIONSTRING, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			realwidth, realheight, flags);

#ifdef HWRENDER
	                                                                           
                                                                      
	if (window == NULL && requested_msaa > 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		window = SDL_CreateWindow("SRB2 "VERSIONSTRING, SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED, realwidth, realheight, flags);
		if (window)
			CONS_Alert(CONS_WARNING,
				"The GPU/driver could not create %dx MSAA; continuing without edge antialiasing.\n",
				requested_msaa);
	}

#endif

#ifdef HWRENDER
	                                                                          
                                                                               
	if (window == NULL && rendermode == render_soft)
	{
		flags &= ~SDL_WINDOW_OPENGL;
		window = SDL_CreateWindow("SRB2 "VERSIONSTRING,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			realwidth, realheight, flags);
	}
#endif

	if (window == NULL)
	{
		CONS_Printf(M_GetText("Couldn't create window: %s\n"), SDL_GetError());
		return SDL_FALSE;
	}



	window_needs_first_present = SDL_TRUE;

	Impl_SetWindowIcon();

	return Impl_CreateContext();
}

  
                                                 
 
                    
  
         
  
                                   
 
  

static void Impl_SetWindowIcon(void)
{
	if (window && icoSurface)
		SDL_SetWindowIcon(window, icoSurface);
}

static void Impl_VideoSetupSDLBuffer(void)
{
	if (bufSurface != NULL)
	{
		SDL_FreeSurface(bufSurface);
		bufSurface = NULL;
	}

	if (vid.bpp == 1)
	{
		bufSurface = SDL_CreateRGBSurfaceFrom(screens[0],vid.width,vid.height,8,
			(int)vid.rowbytes,0x00000000,0x00000000,0x00000000,0x00000000);  
	}
	else if (vid.bpp == 2)  
	{
		bufSurface = SDL_CreateRGBSurfaceFrom(screens[0],vid.width,vid.height,15,
			(int)vid.rowbytes,0x00007C00,0x000003E0,0x0000001F,0x00000000);  
	}
	if (bufSurface)
	{
		SDL_SetPaletteColors(bufSurface->format->palette, localPalette, 0, 256);
	}
	else
	{
		I_Error("%s", M_GetText("No system memory for SDL buffer surface\n"));
	}
}

static void Impl_VideoSetupBuffer(void)
{
	size_t required;
	void *newbuffer;


	vid.rowbytes = vid.width * vid.bpp;
	vid.direct = NULL;
	required = (size_t)NUMSCREENS * (size_t)vid.rowbytes * (size_t)vid.height;




	if (!vid.buffer || required > vid_buffer_capacity)
	{
		newbuffer = calloc(1, required);
		if (!newbuffer)
			I_Error("%s", M_GetText("Not enough memory for video buffer\n"));
		free(vid.buffer);
		vid.buffer = newbuffer;
		vid_buffer_capacity = required;
	}
	else
		memset(vid.buffer, 0, required);

	if (!vid.buffer)
	{
		I_Error("%s", M_GetText("Not enough memory for video buffer\n"));
	}
}

void I_StartupGraphics(void)
{
	if (dedicated)
	{
		rendermode = render_none;
		return;
	}
	if (graphics_started)
		return;

	COM_AddCommand ("vid_nummodes", VID_Command_NumModes_f, COM_LUA);
	COM_AddCommand ("vid_info", VID_Command_Info_f, COM_LUA);
	COM_AddCommand ("vid_modelist", VID_Command_ModeList_f, COM_LUA);
	COM_AddCommand ("vid_mode", VID_Command_Mode_f, 0);
	CV_RegisterVar (&cv_vidwait);
	CV_RegisterVar (&cv_windowedstyle);
	CV_RegisterVar (&cv_stretch);
	CV_RegisterVar (&cv_alwaysgrabmouse);
	disable_mouse = M_CheckParm("-nomouse");
	disable_fullscreen = M_CheckParm("-win") ? 1 : 0;

	keyboard_started = true;

#if !defined(HAVE_TTF)

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
	{
		CONS_Printf(M_GetText("Couldn't initialize SDL's Video System: %s\n"), SDL_GetError());
		return;
	}
#endif
	{
		const char *vd = SDL_GetCurrentVideoDriver();

		if (vd && (
			strncasecmp(vd, "gcvideo", 8) == 0 ||
			strncasecmp(vd, "fbcon", 6) == 0 ||
			strncasecmp(vd, "wii", 4) == 0 ||
			strncasecmp(vd, "psl1ght", 8) == 0
		))
			framebuffer = SDL_TRUE;
	}



	if (M_CheckParm("-renderer"))
	{
		INT32 i = 0;
		CV_PossibleValue_t *renderer_list = cv_renderer_t;
		const char *modeparm = M_GetNextParm();
		while (renderer_list[i].strvalue)
		{
			if (!stricmp(modeparm, renderer_list[i].strvalue))
			{
				chosenrendermode = renderer_list[i].value;
				break;
			}
			i++;
		}
	}


	else if (M_CheckParm("-software"))
		chosenrendermode = render_soft;

#ifdef HWRENDER

	else if (M_CheckParm("-opengl"))
		chosenrendermode = render_opengl;


	if (M_CheckParm("-nogl"))
	{
		vid.glstate = VID_GL_LIBRARY_ERROR;
		if (chosenrendermode == render_opengl)
			chosenrendermode = render_none;
	}
#endif

	if (chosenrendermode != render_none)
		rendermode = chosenrendermode;

	usesdl2soft = M_CheckParm("-softblit");




#ifdef HWRENDER
	if (rendermode == render_opengl)
		VID_StartupOpenGL();
#endif


#ifdef HAVE_IMAGE
	icoSurface = IMG_ReadXPMFromArray(SDL_icon_xpm);
#endif







	VID_SetMode(VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT));

	vid.width = BASEVIDWIDTH;  
	vid.height = BASEVIDHEIGHT;  
	vid.recalc = true;  
	vid.direct = NULL;  
	vid.bpp = 1;  
	vid.WndParent = NULL;  

#ifdef HAVE_TTF
	I_ShutdownTTF();
#endif

	VID_SetMode(VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT));

	if (M_CheckParm("-nomousegrab"))
		mousegrabok = SDL_FALSE;
#if 0  
	else
	{
		char videodriver[4] = {'S','D','L',0};
		if (!M_CheckParm("-mousegrab") &&
		    *strncpy(videodriver, SDL_GetCurrentVideoDriver(), 4) != '\0' &&
		    strncasecmp("x11",videodriver,4) == 0)
			mousegrabok = SDL_FALSE;  
	}
#endif
	realwidth = (Uint16)vid.width;
	realheight = (Uint16)vid.height;


	SDLdoUngrabMouse();

	SDL_RaiseWindow(window);

	if (mousegrabok && !disable_mouse)
		SDLdoGrabMouse();

#ifdef _WIN32
	SDL_SetWindowsMessageHook(Impl_WindowsMessageHook, NULL);
#endif

	graphics_started = true;
}

void VID_StartupOpenGL(void)
{
#ifdef HWRENDER
	static boolean glstartup = false;
	if (!glstartup)
	{
		CONS_Printf("VID_StartupOpenGL()...\n");
		HWD.pfnInit             = hwSym("Init",NULL);
		HWD.pfnFinishUpdate     = NULL;
		HWD.pfnDraw2DLine       = hwSym("Draw2DLine",NULL);
		HWD.pfnDrawPolygon      = hwSym("DrawPolygon",NULL);
		HWD.pfnDrawIndexedTriangles = hwSym("DrawIndexedTriangles",NULL);
		HWD.pfnRenderSkyDome    = hwSym("RenderSkyDome",NULL);
		HWD.pfnSetBlend         = hwSym("SetBlend",NULL);
		HWD.pfnClearBuffer      = hwSym("ClearBuffer",NULL);
		HWD.pfnSetTexture       = hwSym("SetTexture",NULL);
		HWD.pfnUpdateTexture    = hwSym("UpdateTexture",NULL);
		HWD.pfnDeleteTexture    = hwSym("DeleteTexture",NULL);
		HWD.pfnReadScreenTexture= hwSym("ReadScreenTexture",NULL);
		HWD.pfnGClipRect        = hwSym("GClipRect",NULL);
		HWD.pfnClearMipMapCache = hwSym("ClearMipMapCache",NULL);
		HWD.pfnSetSpecialState  = hwSym("SetSpecialState",NULL);
		HWD.pfnSetTexturePalette= hwSym("SetTexturePalette",NULL);
		HWD.pfnGetTextureUsed   = hwSym("GetTextureUsed",NULL);
		HWD.pfnDrawModel        = hwSym("DrawModel",NULL);
		HWD.pfnCreateModelVBOs  = hwSym("CreateModelVBOs",NULL);
		HWD.pfnSetTransform     = hwSym("SetTransform",NULL);
		HWD.pfnPostImgRedraw    = hwSym("PostImgRedraw",NULL);
		HWD.pfnFlushScreenTextures=hwSym("FlushScreenTextures",NULL);
		HWD.pfnDoScreenWipe     = hwSym("DoScreenWipe",NULL);
		HWD.pfnDrawScreenTexture= hwSym("DrawScreenTexture",NULL);
		HWD.pfnMakeScreenTexture= hwSym("MakeScreenTexture",NULL);
		HWD.pfnDrawScreenFinalTexture=hwSym("DrawScreenFinalTexture",NULL);

		HWD.pfnInitShaders      = hwSym("InitShaders",NULL);
		HWD.pfnLoadShader       = hwSym("LoadShader",NULL);
		HWD.pfnCompileShader    = hwSym("CompileShader",NULL);
		HWD.pfnSetShader        = hwSym("SetShader",NULL);
		HWD.pfnUnSetShader      = hwSym("UnSetShader",NULL);

		HWD.pfnSetShaderInfo    = hwSym("SetShaderInfo",NULL);

		HWD.pfnSetPaletteLookup = hwSym("SetPaletteLookup",NULL);
		HWD.pfnCreateLightTable = hwSym("CreateLightTable",NULL);
		HWD.pfnUpdateLightTable = hwSym("UpdateLightTable",NULL);
		HWD.pfnClearLightTables = hwSym("ClearLightTables",NULL);
		HWD.pfnSetScreenPalette = hwSym("SetScreenPalette",NULL);

		vid.glstate = HWD.pfnInit() ? VID_GL_LIBRARY_LOADED : VID_GL_LIBRARY_ERROR;  

		if (vid.glstate == VID_GL_LIBRARY_ERROR)
		{
			rendermode = render_soft;
			setrenderneeded = 0;
		}
		glstartup = true;
	}
#endif
}

void I_ShutdownGraphics(void)
{
	rendermode = render_none;
#ifdef _WIN32
	SDL_SetWindowsMessageHook(NULL, NULL);
	if (windows_sizemove_window)
		KillTimer(windows_sizemove_window, SRB2_SIZEMOVE_TIMER_ID);
	windows_sizemove_window = NULL;
	windows_sizemove_active = false;
#endif
	if (icoSurface) SDL_FreeSurface(icoSurface);
	icoSurface = NULL;

	I_OutputMsg("I_ShutdownGraphics(): ");


	if (!graphics_started)
	{
		I_OutputMsg("graphics never started\n");
		return;
	}
	graphics_started = false;
	I_OutputMsg("shut down\n");

	Impl_DestroySoftwareContext();
	if (vid.buffer)
	{
		free(vid.buffer);
		vid.buffer = NULL;
		vid_buffer_capacity = 0;
	}

#ifdef HWRENDER
	if (sdlglcontext)
	{
		if (window && SDL_GL_GetCurrentContext() == sdlglcontext)
			SDL_GL_MakeCurrent(window, NULL);
		SDL_GL_DeleteContext(sdlglcontext);
		sdlglcontext = NULL;
	}
#endif
	if (window)
	{
		SDL_DestroyWindow(window);
		window = NULL;
	}
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	framebuffer = SDL_FALSE;
}
#endif

void I_GetCursorPosition(INT32 *x, INT32 *y)
{
	SDL_GetMouseState(x, y);
}

UINT32 I_GetRefreshRate(void)
{





	return refresh_rate;
}

boolean I_VSyncProvidesPacing(void)
{
#ifdef HWRENDER
	if (sdlglcontext != NULL && SDL_GL_GetCurrentContext() == sdlglcontext)
		return SDL_GL_GetSwapInterval() != 0 || OglSdlCompositorProvidesPacing();
#endif
	return renderer != NULL && rendermode == render_soft && cv_vidwait.value != 0;
}

boolean I_CompositorProvidesPacing(void)
{
#ifdef HWRENDER
	return sdlglcontext != NULL && SDL_GL_GetCurrentContext() == sdlglcontext
		&& OglSdlCompositorProvidesPacing();
#else
	return false;
#endif
}
