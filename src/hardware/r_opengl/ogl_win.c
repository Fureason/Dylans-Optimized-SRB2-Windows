// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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









#if defined (_WIN32)


#define RPC_NO_WINDOWS_H
#include <windows.h>
#include <time.h>
#undef GETTEXT
#include "r_opengl.h"






#ifdef DEBUG_TO_FILE
static unsigned long nb_frames = 0;
static clock_t my_clock;
FILE *gllogstream;
#endif

static  HDC     hDC           = NULL;        
static  HGLRC   hGLRC         = NULL;        
static  HWND    hWnd          = NULL;
static  BOOL    WasFullScreen = FALSE;
static void UnSetRes(void);

#ifdef USE_WGL_SWAP
PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT = NULL;
#endif

PFNglClear pglClear;
PFNglGetIntegerv pglGetIntegerv;
PFNglGetString pglGetString;

#define MAX_VIDEO_MODES   32
static  vmode_t     video_modes[MAX_VIDEO_MODES];
INT32     oglflags = 0;










BOOL WINAPI DllMain(HINSTANCE hinstDLL,  
                    DWORD fdwReason,     
                    LPVOID lpvReserved)  
{

	UNREFERENCED_PARAMETER(lpvReserved);
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:


#ifdef DEBUG_TO_FILE
			gllogstream = fopen("ogllog.txt", "wt");
			if (gllogstream == NULL)
				return FALSE;
#endif
			DisableThreadLibraryCalls(hinstDLL);
			break;

		case DLL_THREAD_ATTACH:

			break;

		case DLL_THREAD_DETACH:

			break;

		case DLL_PROCESS_DETACH:

#ifdef DEBUG_TO_FILE
			if (gllogstream)
			{
				fclose(gllogstream);
				gllogstream  = NULL;
			}
#endif
			break;
	}

	return TRUE;   
}

#ifdef STATIC_OPENGL
#define pwglGetProcAddress wglGetProcAddress;
#define pwglCreateContext wglCreateContext;
#define pwglDeleteContext wglDeleteContext;
#define pwglMakeCurrent wglMakeCurrent;
#else
static HMODULE OGL32;
typedef void *(WINAPI *PFNwglGetProcAddress) (const char *);
static PFNwglGetProcAddress pwglGetProcAddress;
typedef HGLRC (WINAPI *PFNwglCreateContext) (HDC hdc);
static PFNwglCreateContext pwglCreateContext;
typedef BOOL (WINAPI *PFNwglDeleteContext) (HGLRC hglrc);
static PFNwglDeleteContext pwglDeleteContext;
typedef BOOL (WINAPI *PFNwglMakeCurrent) (HDC hdc, HGLRC hglrc);
static PFNwglMakeCurrent pwglMakeCurrent;
#endif

#ifndef STATIC_OPENGL
void *GetGLFunc(const char *proc)
{
	void *func = NULL;
	if (pwglGetProcAddress)
		func = pwglGetProcAddress(proc);
	if (!func)
		func = GetProcAddress(OGL32, proc);
	return func;
}
#endif

boolean LoadGL(void)
{
#ifndef STATIC_OPENGL
	OGL32 = LoadLibrary("OPENGL32.DLL");

	if (!OGL32)
		return 0;

	pwglGetProcAddress = GetGLFunc("wglGetProcAddress");
	pwglCreateContext = GetGLFunc("wglCreateContext");
	pwglDeleteContext = GetGLFunc("wglDeleteContext");
	pwglMakeCurrent = GetGLFunc("wglMakeCurrent");
#endif
	return SetupGLfunc();
}










int SetupPixelFormat(INT32 WantColorBits, INT32 WantStencilBits, INT32 WantDepthBits)
{
	static DWORD iLastPFD = 0;
	INT32 nPixelFormat;
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof (PIXELFORMATDESCRIPTOR),   
		1,                               
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,                   
		32  ,            
		0, 0,                            
		0, 0,                            
		0, 0,                            
		0, 0,                            
		0,                               
		0, 0, 0, 0,                      
		0,                               
		0,                               
		0,                               
		PFD_MAIN_PLANE,                  
		0,                               
		0, 0, 0,                         
	};

	DWORD iPFD = (WantColorBits<<16) | (WantStencilBits<<8) | WantDepthBits;

	pfd.cDepthBits = (BYTE)WantDepthBits;
	pfd.cStencilBits = (BYTE)WantStencilBits;

	if (iLastPFD)
	{
		GL_DBG_Printf("WARNING : SetPixelFormat() called twise not supported by all drivers !\n");
	}


	if (iPFD == iLastPFD)
		return 2;
	else
		iLastPFD = iPFD;

	GL_DBG_Printf("SetupPixelFormat() - %d ColorBits - %d StencilBits - %d DepthBits\n",
	           WantColorBits, WantStencilBits, WantDepthBits);

	nPixelFormat = ChoosePixelFormat(hDC, &pfd);

	if (nPixelFormat == 0)
		GL_DBG_Printf("ChoosePixelFormat() FAILED\n");

	if (SetPixelFormat(hDC, nPixelFormat, &pfd) == 0)
	{
		GL_DBG_Printf("SetPixelFormat() FAILED\n");
		return 0;
	}

	return 1;
}






static INT32 WINAPI SetRes(viddef_t *lvid, vmode_t *pcurrentmode)
{
	LPCSTR renderer;
	BOOL WantFullScreen = !(lvid->u.windowed);   

	UNREFERENCED_PARAMETER(pcurrentmode);
	GL_DBG_Printf ("SetMode(): %dx%d %d bits (%s)\n",
	            lvid->width, lvid->height, lvid->bpp*8,
	            WantFullScreen ? "fullscreen" : "windowed");

	hWnd = lvid->WndParent;



	Flush();     






	if (WasFullScreen)
		ChangeDisplaySettings(NULL, CDS_FULLSCREEN);  


	if (WantFullScreen)
	{
		DEVMODE dm;
		ZeroMemory(&dm, sizeof (dm));
		dm.dmSize       = sizeof (dm);
		dm.dmPelsWidth  = lvid->width;
		dm.dmPelsHeight = lvid->height;
		dm.dmBitsPerPel = lvid->bpp*8;
		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
		if (ChangeDisplaySettings(&dm, CDS_TEST) != DISP_CHANGE_SUCCESSFUL)
			return -2;
		if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) !=DISP_CHANGE_SUCCESSFUL)
			return -3;

		SetWindowLong(hWnd, GWL_STYLE, WS_POPUP|WS_VISIBLE);


		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, lvid->width, lvid->height, SWP_NOACTIVATE|SWP_NOZORDER);
	}
	else
	{
		RECT bounds;
		INT32 x, y;
		INT32 w = lvid->width, h = lvid->height;
		GetWindowRect(hWnd, &bounds);
		bounds.right = bounds.left+w;
		bounds.bottom = bounds.top+h;
		AdjustWindowRectEx(&bounds, GetWindowLong(hWnd, GWL_STYLE), (GetMenu(hWnd) != NULL), 0);
		w = bounds.right-bounds.left;
		h = bounds.bottom-bounds.top;
		x = (GetSystemMetrics(SM_CXSCREEN)-w)/2;
		y = (GetSystemMetrics(SM_CYSCREEN)-h)/2;
		SetWindowPos(hWnd, NULL, x, y, w, h, SWP_NOACTIVATE|SWP_NOZORDER);
	}

	if (!hDC)
		hDC = GetDC(hWnd);
	if (!hDC)
	{
		GL_DBG_Printf("GetDC() FAILED\n");
		return 0;
	}

	{
		INT32 res;


		res = SetupPixelFormat(lvid->bpp*8, 0, 16);

		if (res == 0)
			return 0;
		else if (res == 1)
		{

			if (hGLRC)
				UnSetRes();
			hGLRC = pwglCreateContext(hDC);
			if (!hGLRC)
			{
				GL_DBG_Printf("pwglCreateContext() FAILED\n");
				return 0;
			}
			if (!pwglMakeCurrent(hDC, hGLRC))
			{
				GL_DBG_Printf("wglMakeCurrent() FAILED\n");
				return 0;
			}
		}
	}

	gl_extensions = pglGetString(GL_EXTENSIONS);



	renderer = (LPCSTR)pglGetString(GL_RENDERER);
	GL_DBG_Printf("Vendor     : %s\n", pglGetString(GL_VENDOR));
	GL_DBG_Printf("Renderer   : %s\n", renderer);
	GL_DBG_Printf("Version    : %s\n", pglGetString(GL_VERSION));
	GL_DBG_Printf("Extensions : %s\n", gl_extensions);



	if (strstr(renderer, "810"))   oglflags |= GLF_NOZBUFREAD;
	GL_DBG_Printf("oglflags   : 0x%X\n", oglflags);

#ifdef USE_WGL_SWAP
	if (isExtAvailable("WGL_EXT_swap_control",gl_extensions))
		wglSwapIntervalEXT = GetGLFunc("wglSwapIntervalEXT");
	else
		wglSwapIntervalEXT = NULL;
#endif

	if (isExtAvailable("GL_EXT_texture_filter_anisotropic",gl_extensions))
		pglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
	else
		maximumAnisotropy = 0;

	SetupGLFunc13();

	screen_depth = (GLbyte)(lvid->bpp*8);
	if (screen_depth > 16)
		textureformatGL = GL_RGBA;
	else
		textureformatGL = GL_RGB5_A1;

	SetModelView(lvid->width, lvid->height);
	SetStates();

	pglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	lvid->buffer = NULL;     
	lvid->direct = NULL;     

	WasFullScreen = WantFullScreen;

	return 1;                
}





static void UnSetRes(void)
{
	GL_DBG_Printf("UnSetRes()\n");

	pwglMakeCurrent(hDC, NULL);
	pwglDeleteContext(hGLRC);
	hGLRC = NULL;
	if (WasFullScreen)
		ChangeDisplaySettings(NULL, CDS_FULLSCREEN);
}







EXPORT void HWRAPI(GetModeList) (vmode_t** pvidmodes, INT32 *numvidmodes)
{
	INT32  i;

#if 1
	INT32 iMode;
  
              

                                                                                  
                                                 
  
	(*pvidmodes) = &video_modes[0];


	for (i = 0,iMode = 0; iMode < MAX_VIDEO_MODES; i++)
	{
		DEVMODE Tmp;
		ZeroMemory(&Tmp, sizeof (Tmp));
		Tmp.dmSize = sizeof (Tmp);
		if (!EnumDisplaySettings(NULL, i, &Tmp))
			break;


		if (Tmp.dmBitsPerPel == 16 &&
			 (iMode == 0 || !
			  (Tmp.dmPelsWidth == video_modes[iMode-1].width &&
			   Tmp.dmPelsHeight == video_modes[iMode-1].height)
			 )
			)
		{
			video_modes[iMode].pnext = &video_modes[iMode+1];
			video_modes[iMode].windowed = 0;                     
			video_modes[iMode].misc = 0;
			video_modes[iMode].name = malloc(12 * sizeof (CHAR));
			sprintf(video_modes[iMode].name, "%dx%d", (INT32)Tmp.dmPelsWidth, (INT32)Tmp.dmPelsHeight);
			GL_DBG_Printf ("Mode: %s\n", video_modes[iMode].name);
			video_modes[iMode].width = Tmp.dmPelsWidth;
			video_modes[iMode].height = Tmp.dmPelsHeight;
			video_modes[iMode].bytesperpixel = Tmp.dmBitsPerPel/8;
			video_modes[iMode].rowbytes = Tmp.dmPelsWidth * video_modes[iMode].bytesperpixel;
			video_modes[iMode].pextradata = NULL;
			video_modes[iMode].setmode = SetRes;
			iMode++;
		}
	}
	(*numvidmodes) = iMode;
#else



	INT32 res[][2] = {
					{ 320, 200},
					{ 320, 240},
					{ 400, 300},
					{ 512, 384},
					{ 640, 400},
					{ 640, 480},
					{ 800, 600},
					{ 960, 600},
					{1024, 768},
					{1152, 864},
					{1280, 800},
					{1280, 960},
					{1280,1024},
					{1600,1000},
					{1680,1050},
					{1920,1200},
};

	HDC bpphdc;
	INT32 iBitsPerPel;

	GL_DBG_Printf ("HWRAPI GetModeList()\n");

	bpphdc = GetDC(NULL);  
	iBitsPerPel = GetDeviceCaps(bpphdc, BITSPIXEL);

	ReleaseDC(NULL, bpphdc);

	(*pvidmodes) = &video_modes[0];
	(*numvidmodes) = sizeof (res) / sizeof (res[0]);
	for (i = 0; i < (*numvidmodes); i++)
	{
		video_modes[i].pnext = &video_modes[i+1];
		video_modes[i].windowed = 0;  
		video_modes[i].misc = 0;
		video_modes[i].name = malloc(12 * sizeof (CHAR));
		sprintf(video_modes[i].name, "%dx%d", res[i][0], res[i][1]);
		GL_DBG_Printf ("Mode: %s\n", video_modes[i].name);
		video_modes[i].width = res[i][0];
		video_modes[i].height = res[i][1];
		video_modes[i].bytesperpixel = iBitsPerPel/8;
		video_modes[i].rowbytes = res[i][0] * video_modes[i].bytesperpixel;
		video_modes[i].pextradata = NULL;
		video_modes[i].setmode = SetRes;
	}
#endif
	video_modes[(*numvidmodes)-1].pnext = NULL;
}





EXPORT void HWRAPI(Shutdown) (void)
{
#ifdef DEBUG_TO_FILE
	long nb_centiemes;

	GL_DBG_Printf ("HWRAPI Shutdown()\n");
	nb_centiemes = ((clock()-my_clock)*100)/CLOCKS_PER_SEC;
	GL_DBG_Printf("Nb frames: %li;  Nb sec: %2.2f  ->  %2.1f fps\n",
					nb_frames, nb_centiemes/100.0f, (100*nb_frames)/(double)nb_centiemes);
#endif

	Flush();


	if (hGLRC)
		UnSetRes();

	if (hDC)
	{
		ReleaseDC(hWnd, hDC);
		hDC = NULL;
	}
	FreeLibrary(OGL32);
	GL_DBG_Printf ("HWRAPI Shutdown(DONE)\n");
}




EXPORT void HWRAPI(FinishUpdate) (INT32 waitvbl)
{
#ifdef USE_WGL_SWAP
	static INT32 oldwaitvbl = 0;
#else
	UNREFERENCED_PARAMETER(waitvbl);
#endif

#ifdef DEBUG_TO_FILE
	if ((++nb_frames)==2)   
		my_clock = clock();
#endif

#ifdef USE_WGL_SWAP
	if (oldwaitvbl != waitvbl && wglSwapIntervalEXT)
		wglSwapIntervalEXT(waitvbl);
	oldwaitvbl = waitvbl;
#endif

	SwapBuffers(hDC);
}







EXPORT void HWRAPI(SetPalette) (RGBA_t *pal)
{
	size_t palsize = (sizeof(RGBA_t) * 256);

	if (memcmp(&myPaletteData, pal, palsize))
	{
		memcpy(&myPaletteData, pal, palsize);
		FlushPaletteDependentTextures();
	}
}

#endif
