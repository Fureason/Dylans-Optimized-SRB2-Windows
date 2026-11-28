// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
//
//-----------------------------------------------------------------------------
/// \file
/// \brief SDL specific part of the OpenGL API for SRB2

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef HAVE_SDL
#define _MATH_DEFINES_DEFINED

#include "SDL.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "sdlmain.h"

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#include "../doomdef.h"

#ifdef HWRENDER
#include "../hardware/r_opengl/r_opengl.h"
#include "../hardware/hw_main.h"
#include "ogl_sdl.h"
#include "../i_system.h"
#include "../i_video.h"
#include "hwsym_sdl.h"
#include "../m_argv.h"

#ifdef DEBUG_TO_FILE
#include <stdarg.h>
#if defined (_WIN32) && !defined (__CYGWIN__)
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef USE_WGL_SWAP
PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT = NULL;
#else
typedef int (*PFNGLXSWAPINTERVALPROC) (int);
PFNGLXSWAPINTERVALPROC glXSwapIntervalSGIEXT = NULL;
#endif

#ifndef STATIC_OPENGL
PFNglClear pglClear;
PFNglGetIntegerv pglGetIntegerv;
PFNglGetString pglGetString;
#endif

/**	\brief SDL video display surface
*/
INT32 oglflags = 0;
SDL_GLContext sdlglcontext = 0;

#if defined(_WIN32)
typedef HRESULT (WINAPI *dwm_flush_fn)(void);
static dwm_flush_fn windowed_dwm_flush;
static boolean windowed_compositor_pacing;
static UINT16 windowed_dwm_cooldown;
static UINT8 windowed_dwm_slow_score;

static void OglSdlLoadDwmFlush(void)
{
	static boolean attempted;
	HMODULE dwmapi;
	FARPROC proc;

	if (attempted)
		return;
	attempted = true;
	dwmapi = LoadLibraryA("dwmapi.dll");
	if (!dwmapi)
		return;
	proc = GetProcAddress(dwmapi, "DwmFlush");
	memcpy(&windowed_dwm_flush, &proc, sizeof(windowed_dwm_flush));
}
#endif

boolean OglSdlCompositorProvidesPacing(void)
{
#if defined(_WIN32)



	return windowed_compositor_pacing && !windowed_dwm_cooldown;
#else
	return false;
#endif
}

void OglSdlSetVSync(INT32 mode, boolean report)
{
	static INT32 lastreportedmode = -99;
	static INT32 lastreportedinterval = -99;
	static boolean lastreportedcompositor = false;
	INT32 requested = mode ? 1 : 0;
	INT32 actual;
	boolean compositorpacing = false;

#if defined(_WIN32)




	if (mode && window
	&& !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN))
	{
		requested = 0;
		compositorpacing = true;
		OglSdlLoadDwmFlush();
	}
#endif

	SDL_GL_SetSwapInterval(requested);

	actual = SDL_GL_GetSwapInterval();
	compositorpacing = compositorpacing && actual == 0;
#if defined(_WIN32)



	windowed_compositor_pacing = compositorpacing && windowed_dwm_flush != NULL;
	windowed_dwm_cooldown = 0;
	windowed_dwm_slow_score = 0;
	compositorpacing = windowed_compositor_pacing;
#endif
	if (report && (mode != lastreportedmode || actual != lastreportedinterval
	|| compositorpacing != lastreportedcompositor))
	{
		if (compositorpacing)
			CONS_Printf("VSync: On via Windows compositor pacing (swap interval 0)\n");
		else
			CONS_Printf("VSync: %s (swap interval %d)\n", mode ? "On" : "Off", actual);
		lastreportedmode = mode;
		lastreportedinterval = actual;
		lastreportedcompositor = compositorpacing;
	}
}

void *GetGLFunc(const char *proc)
{
	return SDL_GL_GetProcAddress(proc);
}

boolean LoadGL(void)
{
#ifndef STATIC_OPENGL
	const char *OGLLibname = NULL;

	if (M_CheckParm("-OGLlib") && M_IsNextParm())
		OGLLibname = M_GetNextParm();

	if (SDL_GL_LoadLibrary(OGLLibname) != 0)
	{
		CONS_Alert(CONS_ERROR, "Could not load OpenGL Library: %s\n"
					"Falling back to Software mode.\n", SDL_GetError());
		if (!M_CheckParm("-OGLlib"))
			CONS_Printf("If you know what is the OpenGL library's name, use -OGLlib\n");
		return 0;
	}
#endif
	return SetupGLfunc();
}

static GLuint renderTarget;
static GLuint renderTargetBuffers[2];
static PFNGLGENFRAMEBUFFERSPROC genFramebuffers;
static PFNGLDELETEFRAMEBUFFERSPROC deleteFramebuffers;
static PFNGLBINDFRAMEBUFFERPROC bindFramebuffer;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC checkFramebuffer;
static PFNGLGENRENDERBUFFERSPROC genRenderbuffers;
static PFNGLDELETERENDERBUFFERSPROC deleteRenderbuffers;
static PFNGLBINDRENDERBUFFERPROC bindRenderbuffer;
static PFNGLRENDERBUFFERSTORAGEPROC renderbufferStorage;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC framebufferRenderbuffer;
static PFNGLBLITFRAMEBUFFERPROC blitFramebuffer;
static boolean renderTargetCanBlit;

static boolean OglSdlLoadFramebufferFunctions(void)
{
	if (gl_version[0] < '3' && !isExtAvailable("GL_ARB_framebuffer_object", gl_extensions))
		return false;

	genFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)GetGLFunc("glGenFramebuffers");
	deleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)GetGLFunc("glDeleteFramebuffers");
	bindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)GetGLFunc("glBindFramebuffer");
	checkFramebuffer = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)GetGLFunc("glCheckFramebufferStatus");
	genRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)GetGLFunc("glGenRenderbuffers");
	deleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)GetGLFunc("glDeleteRenderbuffers");
	bindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)GetGLFunc("glBindRenderbuffer");
	renderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)GetGLFunc("glRenderbufferStorage");
	framebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)GetGLFunc("glFramebufferRenderbuffer");
	blitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)GetGLFunc("glBlitFramebuffer");
	return genFramebuffers && deleteFramebuffers && bindFramebuffer && checkFramebuffer
		&& genRenderbuffers && deleteRenderbuffers && bindRenderbuffer
		&& renderbufferStorage && framebufferRenderbuffer;
}

void OglSdlReleaseRenderTarget(void)
{
	if (!renderTarget)
		return;

	bindFramebuffer(GL_FRAMEBUFFER, 0);
	deleteFramebuffers(1, &renderTarget);
	deleteRenderbuffers(2, renderTargetBuffers);
	renderTarget = 0;
	renderTargetBuffers[0] = renderTargetBuffers[1] = 0;
	renderTargetCanBlit = false;
}

static boolean OglSdlPrepareRenderTarget(INT32 width, INT32 height)
{
	int drawablewidth, drawableheight;
	GLint sampleBuffers = 0;
	SDL_GL_GetDrawableSize(window, &drawablewidth, &drawableheight);
	OglSdlReleaseRenderTarget();
	const boolean requiresTarget = width > drawablewidth || height > drawableheight;
	if (!OglSdlLoadFramebufferFunctions())
		return !requiresTarget;

	pglGetIntegerv(GL_SAMPLE_BUFFERS, &sampleBuffers);
	const boolean canBlit = blitFramebuffer && !sampleBuffers;
	if (!requiresTarget && !canBlit)
		return true;



	genFramebuffers(1, &renderTarget);
	if (!renderTarget)
		return !requiresTarget;
	bindFramebuffer(GL_FRAMEBUFFER, renderTarget);
	genRenderbuffers(2, renderTargetBuffers);
	bindRenderbuffer(GL_RENDERBUFFER, renderTargetBuffers[0]);
	renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
	framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderTargetBuffers[0]);

	bindRenderbuffer(GL_RENDERBUFFER, renderTargetBuffers[1]);
	renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderTargetBuffers[1]);
	bindRenderbuffer(GL_RENDERBUFFER, 0);

	if (checkFramebuffer(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
	{
		renderTargetCanBlit = canBlit;
		return true;
	}

	OglSdlReleaseRenderTarget();
	return !requiresTarget;
}

/**	\brief	The OglSdlSurface function

	\param	w	width
	\param	h	height
	\param	isFullscreen	if true, go fullscreen

	\return	if true, changed video mode
*/
boolean OglSdlSurface(INT32 w, INT32 h)
{
	INT32 cbpp = cv_scr_depth.value < 16 ? 16 : cv_scr_depth.value;
	static boolean first_init = false;
	static int majorGL = 0, minorGL = 0;

	oglflags = 0;

	if (!first_init)
	{
		gl_version = pglGetString(GL_VERSION);
		gl_renderer = pglGetString(GL_RENDERER);
		gl_extensions = pglGetString(GL_EXTENSIONS);

		GL_DBG_Printf("OpenGL %s\n", gl_version);
		GL_DBG_Printf("GPU: %s\n", gl_renderer);
		GL_DBG_Printf("Extensions: %s\n", gl_extensions);

		if (strcmp((const char*)gl_renderer, "GDI Generic") == 0 &&
			strcmp((const char*)gl_version, "1.1.0") == 0)
		{
			// Oh no... Windows gave us the GDI Generic rasterizer, so something is wrong...
			// The game will crash later on when unsupported OpenGL commands are encountered.
			// Instead of a nondescript crash, show a more informative error message.
			// Also set the renderer variable back to software so the next launch won't
			// repeat this error.
			CV_StealthSet(&cv_renderer, "Software");
			I_Error("OpenGL Error: Failed to access the GPU. Possible reasons include:\n"
					"- GPU vendor has dropped OpenGL support on your GPU and OS. (Old GPU?)\n"
					"- GPU drivers are missing or broken. You may need to update your drivers.");
		}
	}
	first_init = true;

	if (isExtAvailable("GL_EXT_texture_filter_anisotropic", gl_extensions))
		pglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
	else
		maximumAnisotropy = 1;

	if (sscanf((const char*)gl_version, "%d.%d", &majorGL, &minorGL)
		&& (!(majorGL == 1 && minorGL <= 3)))
		supportMipMap = true;
	else
		supportMipMap = false;

	SetupGLFunc4();




	glanisotropicmode_cons_t[2].value = maximumAnisotropy;

	if (!OglSdlPrepareRenderTarget(w, h))
		return false;
#ifndef __ANDROID__
	SDL_SetWindowMinimumSize(window, renderTarget ? BASEVIDWIDTH : w,
		renderTarget ? BASEVIDHEIGHT : h);
#endif

	SetModelView(w, h);
	SetStates();
	pglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	OglSdlSetVSync(cv_vidwait.value, true);

	HWR_Startup();
	textureformatGL = cbpp > 16 ? GL_RGBA : GL_RGB5_A1;

	return true;
}

/**	\brief	The OglSdlFinishUpdate function

	\param	vidwait	wait for video sync

	\return	void
*/
void OglSdlFinishUpdate(INT32 waitvbl)
{
	static INT32 oldwaitvbl = -1;
	int sdlw, sdlh;
	SDL_Rect viewport;
	if (oldwaitvbl != waitvbl)
	{



		oldwaitvbl = waitvbl;
		OglSdlSetVSync(waitvbl, true);
	}

	SDL_GL_GetDrawableSize(window, &sdlw, &sdlh);
	if (sdlw <= 0 || sdlh <= 0)
		return;
	I_GetGameViewport(sdlw, sdlh, &viewport);

	const boolean directBlit = renderTargetCanBlit && !HWR_ShouldUsePaletteRendering();
	const INT32 bottom = sdlh - viewport.y - viewport.h;
	if (directBlit)
	{


		bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		if (viewport.w != sdlw || viewport.h != sdlh)
		{
			FRGBAFloat black = {0, 0, 0, 1};
			ClearBuffer(true, false, &black);
		}
		blitFramebuffer(0, 0, realwidth, realheight,
			viewport.x, bottom, viewport.x + viewport.w, bottom + viewport.h,
			GL_COLOR_BUFFER_BIT, (viewport.w < realwidth || viewport.h < realheight)
			? GL_LINEAR : GL_NEAREST);
	}
	else
	{
		HWR_MakeScreenFinalTexture();
		if (renderTarget)
			bindFramebuffer(GL_FRAMEBUFFER, 0);
		HWR_DrawScreenFinalTexture(viewport.x, bottom, viewport.w, viewport.h);
	}
	SDL_GL_SwapWindow(window);
	if (renderTarget)
		bindFramebuffer(GL_FRAMEBUFFER, renderTarget);
#if defined(_WIN32)
	if (windowed_dwm_cooldown)
		windowed_dwm_cooldown--;
	else if (windowed_compositor_pacing && windowed_dwm_flush)
	{
		const precise_t started = I_GetPreciseTime();
		const HRESULT result = windowed_dwm_flush();
		const precise_t elapsed = I_GetPreciseTime() - started;
		const precise_t precision = I_GetPrecisePrecision();
		const UINT32 refresh = max(30u, I_GetRefreshRate());


		const precise_t slow = (precise_t)(((UINT64)precision * 5) / (2 * refresh));

		if (FAILED(result))
		{


			windowed_compositor_pacing = false;
			CONS_Alert(CONS_WARNING,
				"Windows compositor pacing became unavailable; using engine pacing.\n");
		}
		else if (elapsed > slow)
		{
			windowed_dwm_slow_score = min(4, windowed_dwm_slow_score + 2);
			if (windowed_dwm_slow_score >= 4)
			{



				windowed_dwm_cooldown = 2 * refresh;
				windowed_dwm_slow_score = 0;
			}
		}
		else if (windowed_dwm_slow_score)
			windowed_dwm_slow_score--;
	}
#endif

	GClipRect(0, 0, realwidth, realheight, NZCLIP_PLANE);

	// Sryder:	We need to draw the final screen texture again into the other buffer in the original position so that
	//			effects that want to take the old screen can do so after this
	// Generic2 has the screen image without palette rendering brightness adjustments.
	// Using that here will prevent brightness adjustments being applied twice.
	if (!directBlit)
		DrawScreenTexture(HWD_SCREENTEXTURE_GENERIC2, NULL, 0, true);
}

void OglSdlPresentSoftware(const void *pixels, INT32 width, INT32 height, INT32 waitvbl, boolean rgba)
{
	static INT32 oldwaitvbl = -1;
	int drawablewidth, drawableheight;
	SDL_Rect viewport;

	if (oldwaitvbl != waitvbl)
		OglSdlSetVSync(waitvbl, true);
	oldwaitvbl = waitvbl;

	SDL_GL_GetDrawableSize(window, &drawablewidth, &drawableheight);
	I_GetGameViewport(drawablewidth, drawableheight, &viewport);
	OglRenderSoftwareFrame(pixels, width, height, rgba,
		viewport.x, drawableheight - viewport.y - viewport.h, viewport.w, viewport.h);
	SDL_GL_SwapWindow(window);
}

void OglSdlDestroySoftwarePresenter(void)
{
	OglDestroySoftwareFrame();
}

EXPORT void HWRAPI(OglSdlSetPalette) (RGBA_t *palette)
{
	size_t palsize = (sizeof(RGBA_t) * 256);
	// on a palette change, you have to reload all of the textures
	if (memcmp(&myPaletteData, palette, palsize))
	{
		memcpy(&myPaletteData, palette, palsize);
		Flush();
	}
}

#endif //HWRENDER
#endif //SDL
