// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "command.h"

#if defined (_WIN32) && !defined (__CYGWIN__)
#define RPC_NO_WINDOWS_H
#include <windows.h>
#define DNWH HWND
#else
#define DNWH void *  
#endif


#ifdef NOPOSTPROCESSING
#define NUMSCREENS 2
#else
#define NUMSCREENS 5
#endif






#define MAXVIDWIDTH 2560  
#define MAXVIDHEIGHT 1440  
#define BASEVIDWIDTH 320  
#define BASEVIDHEIGHT 200  


typedef struct viddef_s
{
	INT32 modenum;  

	UINT8 *buffer;  
	size_t rowbytes;  
	INT32 width;  
	INT32 height;
	union {  
		INT32 numpages;  
		INT32 windowed;  
	} u;
	INT32 recalc;  
	UINT8 *direct;  
	INT32 dup;  
	INT32  fdup;  
	INT32 bpp;  

	INT32 baseratio;  


	DNWH WndParent;  
	UINT8 smalldup;  
	UINT8 meddup;  
#ifdef HWRENDER
	INT32  fsmalldup;
	INT32  fmeddup;
	INT32 glstate;
#endif
} viddef_t;

enum
{
	VID_GL_LIBRARY_NOTLOADED  = 0,
	VID_GL_LIBRARY_LOADED     = 1,
	VID_GL_LIBRARY_ERROR      = -1,
};





#define BASEDRAWFUNC 0

enum
{
	COLDRAWFUNC_BASE = BASEDRAWFUNC,
	COLDRAWFUNC_FUZZY,
	COLDRAWFUNC_TRANS,
	COLDRAWFUNC_SHADE,
	COLDRAWFUNC_SHADOWED,
	COLDRAWFUNC_TRANSTRANS,
	COLDRAWFUNC_CLAMPED,
	COLDRAWFUNC_CLAMPEDTRANS,
	COLDRAWFUNC_TWOSMULTIPATCH,
	COLDRAWFUNC_TWOSMULTIPATCHTRANS,
	COLDRAWFUNC_FOG,

	COLDRAWFUNC_MAX
};

extern void (*colfunc)(void);
extern void (*colfuncs[COLDRAWFUNC_MAX])(void);

enum
{
	SPANDRAWFUNC_BASE = BASEDRAWFUNC,
	SPANDRAWFUNC_TRANS,
	SPANDRAWFUNC_TILTED,
	SPANDRAWFUNC_TILTEDTRANS,

	SPANDRAWFUNC_SPLAT,
	SPANDRAWFUNC_TRANSSPLAT,
	SPANDRAWFUNC_TILTEDSPLAT,
	SPANDRAWFUNC_TILTEDTRANSSPLAT,

	SPANDRAWFUNC_SPRITE,
	SPANDRAWFUNC_TRANSSPRITE,
	SPANDRAWFUNC_TILTEDSPRITE,
	SPANDRAWFUNC_TILTEDTRANSSPRITE,

	SPANDRAWFUNC_WATER,
	SPANDRAWFUNC_TILTEDWATER,

	SPANDRAWFUNC_SOLID,
	SPANDRAWFUNC_TRANSSOLID,
	SPANDRAWFUNC_TILTEDSOLID,
	SPANDRAWFUNC_TILTEDTRANSSOLID,
	SPANDRAWFUNC_WATERSOLID,
	SPANDRAWFUNC_TILTEDWATERSOLID,

	SPANDRAWFUNC_FOG,
	SPANDRAWFUNC_TILTEDFOG,

	SPANDRAWFUNC_MAX
};

extern void (*spanfunc)(void);
extern void (*spanfuncs[SPANDRAWFUNC_MAX])(void);
extern void (*spanfuncs_npo2[SPANDRAWFUNC_MAX])(void);




extern viddef_t vid;
extern INT32 setmodeneeded;  
extern UINT8 setrenderneeded;

extern double averageFPS;

void SCR_ChangeRenderer(void);

extern CV_PossibleValue_t cv_renderer_t[];

extern INT32 scr_bpp;

extern consvar_t cv_scr_width, cv_scr_height, cv_scr_width_w, cv_scr_height_w, cv_scr_depth, cv_fullscreen;
extern consvar_t cv_renderer;

extern consvar_t cv_vidwait;
extern consvar_t cv_timescale;


void SCR_Startup(void);


void SCR_SetMode(void);


void SCR_SetDrawFuncs(void);


void SCR_Recalc(void);


void SCR_CheckDefaultMode(void);


void SCR_SetDefaultMode(void);

void SCR_CalculateFPS(void);

FUNCMATH boolean SCR_IsAspectCorrect(INT32 width, INT32 height);


void SCR_DisplayTicRate(void);
void SCR_ClosedCaptions(void);
void SCR_DisplayLocalPing(void);
void SCR_DisplayMarathonInfo(void);
#undef DNWH
#endif  
