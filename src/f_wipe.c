// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2013-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "f_finale.h"
#include "i_video.h"
#include "v_video.h"

#include "r_state.h"  
#include "r_draw.h"  
#include "p_pspr.h"  
#include "p_local.h"
#include "st_stuff.h"
#include "w_wad.h"
#include "z_zone.h"

#include "i_time.h"
#include "i_system.h"
#include "i_threads.h"
#include "m_menu.h"
#include "console.h"
#include "d_main.h"
#include "g_game.h"
#include "m_misc.h"  

#include "doomstat.h"

#include "lua_hud.h"  

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#if NUMSCREENS < 5
#define NOWIPE  
#endif

typedef struct fademask_s {
	UINT8* mask;
	UINT16 width, height;
	size_t size;
	fixed_t xscale, yscale;
} fademask_t;

UINT8 wipedefs[NUMWIPEDEFS] = {
	99,  

	0,   
	UINT8_MAX,   
	0,   
	0,   
	0,   
	99,  
	0,   
	0,   
	99,  
	0,   
	99,  

	0,   
	0,   
	0,   

	0,   
	0,   
	0,   
	0,   
	0,   
	99,  
	0,   
	0,   
	99,  
	0,   
	99,  

	0,   
	0    
};





boolean WipeInAction = false;
boolean WipeStageTitle = false;
INT32 lastwipetic = 0;

wipestyle_t wipestyle = WIPESTYLE_NORMAL;
wipestyleflags_t wipestyleflags = WSF_CROSSFADE;

#ifndef NOWIPE
static UINT8 *wipe_scr_start;  
static UINT8 *wipe_scr_end;  
static UINT8 *wipe_scr;  
static fixed_t paldiv = 0;

                               
   
                                          
                               
    
static fademask_t *F_GetFadeMask(UINT8 masknum, UINT8 scrnnum) {
	static char lumpname[10] = "FADEmmss";
	static fademask_t fm = {NULL,0,0,0,0,0};
	lumpnum_t lumpnum;
	UINT8 *lump, *mask;
	size_t lsize;
	RGBA_t *pcolor;

	if (masknum > 99 || scrnnum > 99)
		goto freemask;

	sprintf(&lumpname[4], "%.2hu%.2hu", (UINT16)masknum, (UINT16)scrnnum);

	lumpnum = W_CheckNumForName(lumpname);
	if (lumpnum == LUMPERROR)
		goto freemask;

	lump = W_CacheLumpNum(lumpnum, PU_CACHE);
	lsize = W_LumpLength(lumpnum);
	switch (lsize)
	{
		case 256000:  
			fm.width = 640;
			fm.height = 400;
			break;
		case 64000:  
			fm.width = 320;
			fm.height = 200;
			break;
		case 16000:  
			fm.width = 160;
			fm.height = 100;
			break;
		case 4000:  
			fm.width = 80;
			fm.height = 50;
			break;

		default:  
			CONS_Alert(CONS_WARNING, "Fade mask lump %s of incorrect size, ignored\n", lumpname);
		case 0:  
			goto freemask;
	}
	if (lsize != fm.size)
		fm.mask = Z_Realloc(fm.mask, lsize, PU_STATIC, NULL);
	fm.size = lsize;

	mask = fm.mask;

	while (lsize--)
	{

		pcolor = &pMasterPalette[*lump++];
		if (wipestyle == WIPESTYLE_COLORMAP)
			*mask++ = pcolor->s.red / FADECOLORMAPDIV;
		else
			*mask++ = FixedDiv((pcolor->s.red+1)<<FRACBITS, paldiv)>>FRACBITS;
	}

	fm.xscale = FixedDiv(vid.width<<FRACBITS, fm.width<<FRACBITS);
	fm.yscale = FixedDiv(vid.height<<FRACBITS, fm.height<<FRACBITS);
	return &fm;




	freemask:
	if (fm.mask)
	{
		Z_Free(fm.mask);
		fm.mask = NULL;
		fm.size = 0;
	}

	return NULL;
}

                         
    
void F_WipeStageTitle(void)
{

	if ((WipeStageTitle && st_overlay)
	&& (wipestyle == WIPESTYLE_COLORMAP)
	&& G_IsTitleCardAvailable())
	{
		ST_runTitleCard();
		ST_drawWipeTitleCard();
	}
}

               
   
                                    
    
static void F_DoWipe(fademask_t *fademask)
{
















	{

		UINT8       *w = wipe_scr;
		const UINT8 *s = wipe_scr_start;
		const UINT8 *e = wipe_scr_end;


		UINT8       *w_base = w;
		const UINT8 *s_base = s;
		const UINT8 *e_base = e;


		UINT8       *transtbl;
		const UINT8 *mask    = fademask->mask;
		const UINT8 *maskend = mask + fademask->size;


		UINT32 draw_linestart, draw_rowstart;
		UINT32 draw_lineend,   draw_rowend;
		UINT32 draw_linestogo, draw_rowstogo;


		UINT16* scrxpos = (UINT16*)malloc((fademask->width + 1)  * sizeof(UINT16));
		UINT16* scrypos = (UINT16*)malloc((fademask->height + 1) * sizeof(UINT16));
		UINT16 maskx, masky;
		UINT32 relativepos;



		scrxpos[0] = 0;
		for (relativepos = 0, maskx = 1; maskx < fademask->width; ++maskx)
			scrxpos[maskx] = (relativepos += fademask->xscale)>>FRACBITS;
		scrxpos[fademask->width] = vid.width;

		scrypos[0] = 0;
		for (relativepos = 0, masky = 1; masky < fademask->height; ++masky)
			scrypos[masky] = (relativepos += fademask->yscale)>>FRACBITS;
		scrypos[fademask->height] = vid.height;


		maskx = masky = 0;
		do
		{
			draw_rowstart = scrxpos[maskx];
			draw_rowend   = scrxpos[maskx + 1];
			draw_linestart = scrypos[masky];
			draw_lineend   = scrypos[masky + 1];

			relativepos = (draw_linestart * vid.width) + draw_rowstart;
			draw_linestogo = draw_lineend - draw_linestart;

			if (*mask == 0)
			{

				while (draw_linestogo--)
				{
					M_Memcpy(w_base+relativepos, s_base+relativepos, draw_rowend-draw_rowstart);
					relativepos += vid.width;
				}
			}
			else if (*mask >= 10)
			{

				while (draw_linestogo--)
				{
					M_Memcpy(w_base+relativepos, e_base+relativepos, draw_rowend-draw_rowstart);
					relativepos += vid.width;
				}
			}
			else
			{

				transtbl = R_GetTranslucencyTable((9 - *mask) + 1);


				while (draw_linestogo--)
				{
					w = w_base + relativepos;
					s = s_base + relativepos;
					e = e_base + relativepos;
					draw_rowstogo = draw_rowend - draw_rowstart;

					while (draw_rowstogo--)
						*w++ = transtbl[ ( *e++ << 8 ) + *s++ ];

					relativepos += vid.width;
				}

			}

			if (++maskx >= fademask->width)
				++masky, maskx = 0;
		} while (++mask < maskend);

		free(scrxpos);
		free(scrypos);
	}
}

static void F_DoColormapWipe(fademask_t *fademask, UINT8 *colormap)
{

	{

		UINT8       *w = wipe_scr;
		const UINT8 *s = wipe_scr_start;
		const UINT8 *e = wipe_scr_end;


		UINT8       *w_base = w;
		const UINT8 *s_base = s;
		const UINT8 *e_base = e;


		UINT8       *transtbl;
		const UINT8 *mask    = fademask->mask;
		const UINT8 *maskend = mask + fademask->size;


		UINT32 draw_linestart, draw_rowstart;
		UINT32 draw_lineend,   draw_rowend;
		UINT32 draw_linestogo, draw_rowstogo;


		UINT16* scrxpos = (UINT16*)malloc((fademask->width + 1)  * sizeof(UINT16));
		UINT16* scrypos = (UINT16*)malloc((fademask->height + 1) * sizeof(UINT16));
		UINT16 maskx, masky;
		UINT32 relativepos;



		scrxpos[0] = 0;
		for (relativepos = 0, maskx = 1; maskx < fademask->width; ++maskx)
			scrxpos[maskx] = (relativepos += fademask->xscale)>>FRACBITS;
		scrxpos[fademask->width] = vid.width;

		scrypos[0] = 0;
		for (relativepos = 0, masky = 1; masky < fademask->height; ++masky)
			scrypos[masky] = (relativepos += fademask->yscale)>>FRACBITS;
		scrypos[fademask->height] = vid.height;


		maskx = masky = 0;
		do
		{
			draw_rowstart = scrxpos[maskx];
			draw_rowend   = scrxpos[maskx + 1];
			draw_linestart = scrypos[masky];
			draw_lineend   = scrypos[masky + 1];

			relativepos = (draw_linestart * vid.width) + draw_rowstart;
			draw_linestogo = draw_lineend - draw_linestart;

			if (*mask == 0)
			{

				while (draw_linestogo--)
				{
					M_Memcpy(w_base+relativepos, s_base+relativepos, draw_rowend-draw_rowstart);
					relativepos += vid.width;
				}
			}
			else if (*mask >= FADECOLORMAPROWS)
			{

				while (draw_linestogo--)
				{
					M_Memcpy(w_base+relativepos, e_base+relativepos, draw_rowend-draw_rowstart);
					relativepos += vid.width;
				}
			}
			else
			{
				int nmask = *mask;
				if (wipestyleflags & WSF_FADEIN)
					nmask = (FADECOLORMAPROWS-1) - nmask;

				transtbl = colormap + (nmask * 256);


				while (draw_linestogo--)
				{
					w = w_base + relativepos;
					s = s_base + relativepos;
					e = e_base + relativepos;
					draw_rowstogo = draw_rowend - draw_rowstart;

					while (draw_rowstogo--)
						*w++ = transtbl[*e++];

					relativepos += vid.width;
				}

			}

			if (++maskx >= fademask->width)
				++masky, maskx = 0;
		} while (++mask < maskend);

		free(scrxpos);
		free(scrypos);
	}
}
#endif

                                       
    
void F_WipeStartScreen(void)
{
#ifndef NOWIPE
#ifdef HWRENDER
	if(rendermode != render_soft)
	{
		HWR_StartScreenWipe();
		return;
	}
#endif
	wipe_scr_start = screens[3];
	I_ReadScreen(wipe_scr_start);
#endif
}

                                      
    
void F_WipeEndScreen(void)
{
#ifndef NOWIPE
#ifdef HWRENDER
	if(rendermode != render_soft)
	{
		HWR_EndScreenWipe();
		return;
	}
#endif
	wipe_scr_end = screens[4];
	I_ReadScreen(wipe_scr_end);
	V_DrawBlock(0, 0, 0, vid.width, vid.height, wipe_scr_start);
#endif
}

                                                    
    
boolean F_ShouldColormapFade(void)
{
#ifndef NOWIPE
	if ((wipestyleflags & (WSF_FADEIN|WSF_FADEOUT))  
	&& !(wipestyleflags & WSF_CROSSFADE))  
	{

		return (gamestate == GS_LEVEL
		|| gamestate == GS_TITLESCREEN

		|| gamestate == GS_CONTINUING
		|| gamestate == GS_CREDITS
		|| gamestate == GS_EVALUATION
		|| gamestate == GS_INTRO
		|| gamestate == GS_ENDING

		|| gamestate == GS_TIMEATTACK);
	}
#endif
	return false;
}

                                   
    
#ifndef NOWIPE
void F_DecideWipeStyle(void)
{

	wipestyle = WIPESTYLE_NORMAL;


	if (F_ShouldColormapFade())
		wipestyle = WIPESTYLE_COLORMAP;
}
#endif

                                   
                                                    
                       
                                                    
    
boolean F_TryColormapFade(UINT8 wipecolor)
{
#ifndef NOWIPE
	if (F_ShouldColormapFade())
	{
#ifdef HWRENDER
		if (rendermode == render_opengl)
			F_WipeColorFill(wipecolor);
#endif
		return true;
	}
	else
#endif
	{
		F_WipeColorFill(wipecolor);
		return false;
	}
}

                                                  
                                          
    
void F_RunWipe(UINT8 wipetype, boolean drawMenu)
{
#ifdef NOWIPE
	(void)wipetype;
	(void)drawMenu;
#else
	tic_t nowtime;
	UINT8 wipeframe = 0;
	fademask_t *fmask;

	if (!paldiv)
		paldiv = FixedDiv(257<<FRACBITS, 11<<FRACBITS);


	F_DecideWipeStyle();
	WipeInAction = true;
	wipe_scr = screens[0];



	for (;;)
	{

		fmask = F_GetFadeMask(wipetype, wipeframe++);
		if (!fmask)
			break;


		while (!((nowtime = I_GetTime()) - lastwipetic))
		{
			I_Sleep(cv_sleep.value);
			I_UpdateTime(cv_timescale.value);
		}
		lastwipetic = nowtime;


		if (wipestyle == WIPESTYLE_COLORMAP)
		{
#ifdef HWRENDER
			if (rendermode == render_opengl)
			{

				HWR_DoWipe(wipetype, wipeframe-1);
			}
			else
#endif
			{
				UINT8 *colormap = fadecolormap;
				if (wipestyleflags & WSF_TOWHITE)
					colormap += (FADECOLORMAPROWS * 256);
				F_DoColormapWipe(fmask, colormap);
			}


			F_WipeStageTitle();
		}
		else
		{
#ifdef HWRENDER
			if (rendermode == render_opengl)
			{

				HWR_DoWipe(wipetype, wipeframe-1);
			}
			else
#endif
				F_DoWipe(fmask);
		}

		I_OsPolling();
		I_UpdateNoBlit();

		if (drawMenu)
		{
#ifdef HAVE_THREADS
			I_lock_mutex(&m_menu_mutex);
#endif
			M_Drawer();  
#ifdef HAVE_THREADS
			I_unlock_mutex(m_menu_mutex);
#endif
		}

		I_FinishUpdate();  

		if (moviemode)
			M_SaveFrame();

		NetKeepAlive();  
	}

	WipeInAction = false;
	WipeStageTitle = false;
#endif
}

                              
                           
    
tic_t F_GetWipeLength(UINT8 wipetype)
{
#ifdef NOWIPE
	(void)wipetype;
	return 0;
#else
	static char lumpname[10] = "FADEmmss";
	lumpnum_t lumpnum;
	UINT8 wipeframe;

	if (wipetype > 99)
		return 0;

	for (wipeframe = 0; wipeframe < 100; wipeframe++)
	{
		sprintf(&lumpname[4], "%.2hu%.2hu", (UINT16)wipetype, (UINT16)wipeframe);

		lumpnum = W_CheckNumForName(lumpname);
		if (lumpnum == LUMPERROR)
			return --wipeframe;
	}
	return --wipeframe;
#endif
}

                                  
    
boolean F_WipeExists(UINT8 wipetype)
{
#ifdef NOWIPE
	(void)wipetype;
	return false;
#else
	static char lumpname[10] = "FADEmm00";
	lumpnum_t lumpnum;

	if (wipetype > 99)
		return false;

	sprintf(&lumpname[4], "%.2hu00", (UINT16)wipetype);

	lumpnum = W_CheckNumForName(lumpname);
	return !(lumpnum == LUMPERROR);
#endif
}
