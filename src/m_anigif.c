// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2013-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 2013      by "Ninji".
// Copyright (C) 2013-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------





#include "m_anigif.h"
#include "d_main.h"
#include "z_zone.h"
#include "v_video.h"
#include "i_video.h"
#include "i_system.h"  
#include "m_misc.h"
#include "st_stuff.h"  
#include "doomstat.h"  

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif


#include "byteptr.h"

CV_PossibleValue_t gif_dynamicdelay_cons_t[] = {
	{0, "Off"},
	{1, "On"},
	{2, "Accurate, experimental"},
{0, NULL}};

consvar_t cv_gif_optimize = CVAR_INIT ("gif_optimize", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_gif_downscale =  CVAR_INIT ("gif_downscale", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_gif_dynamicdelay = CVAR_INIT ("gif_dynamicdelay", "On", CV_SAVE, gif_dynamicdelay_cons_t, NULL);
consvar_t cv_gif_localcolortable =  CVAR_INIT ("gif_localcolortable", "On", CV_SAVE, CV_OnOff, NULL);
static CV_PossibleValue_t gif_maxfps_cons_t[] = {{5, "MIN"}, {60, "MAX"}, {0, NULL}};
consvar_t cv_gif_maxfps = CVAR_INIT ("gif_maxfps", "30", CV_SAVE, gif_maxfps_cons_t, NULL);

#ifdef HAVE_ANIGIF
static boolean gif_optimize = false;  
static boolean gif_downscale = false;  
static UINT8 gif_dynamicdelay = (UINT8)0;  


static boolean gif_localcolortable = false;
static boolean gif_colorprofile = false;
static RGBA_t *gif_headerpalette = NULL;
static RGBA_t *gif_framepalette = NULL;

static FILE *gif_out = NULL;
static INT32 gif_frames = 0;
static precise_t gif_prevframetime = 0;
static UINT64 gif_delayremainder = 0;
static precise_t gif_prevcapturetime = 0;
static UINT32 gif_delayus = 0;  
static UINT8 gif_writeover = 0;













static UINT8 GIF_optimizecmprow(const UINT8 *dst, const UINT8 *src, INT32 row,
	INT32 *last, INT32 *left, INT32 *right)
{
	const UINT8 *dp = dst + (vid.width * row);
	const UINT8 *sp = src + (vid.width * row);
	const UINT8 *dtmp, *stmp;
	UINT8 doleft = 1, doright = 1;
	INT32 i = 0;

	if (!memcmp(sp, dp, vid.width))
		return 0;  

	*last = row;


	i = 0;
	if (*left == 0)  
		doleft = 0;
	else if (*left > 0)  
	{
		if (!memcmp(sp, dp, *left))
			doleft = 0;  
	}
	while (doleft)
	{
		dtmp = dp + i;
		stmp = sp + i;
		if (*dtmp != *stmp)
		{
			doleft = 0;
			*left = i;
		}
		++i;
	}


	i = vid.width - 1;
	if (*right == vid.width - 1)  
		doright = 0;
	else if (*right >= 0)  
	{
		dtmp = dp + *right + 1;
		stmp = sp + *right + 1;
		if (!memcmp(stmp, dtmp, vid.width - (*right + 1)))
			doright = 0;  
	}
	while (doright)
	{
		dtmp = dp + i;
		stmp = sp + i;
		if (*dtmp != *stmp)
		{
			doright = 0;
			*right = i;
		}
		--i;
	}
	return 1;
}











static void GIF_optimizeregion(const UINT8 *dst, const UINT8 *src,
	INT32 *x, INT32 *y, INT32 *w, INT32 *h)
{
	INT32 st = 0, sb = vid.height - 1;  
	INT32 firstchg_t = -1, firstchg_b = -1;  
	INT32 lastchg_t = -1, lastchg_b = -1;  
	INT32 lmpix = -1, rmpix = -1;  
	UINT8 stopt = 0, stopb = 0;

	while ((!stopt || !stopb) && st < sb)
	{
		if (!stopt)
		{
			if (GIF_optimizecmprow(dst, src, st++, &lastchg_t, &lmpix, &rmpix)
			 && lmpix == 0 && rmpix == vid.width - 1)
				stopt = 1;
			if (firstchg_t < 0 && lastchg_t >= 0)
				firstchg_t = lastchg_t;
		}
		if (!stopb)
		{
			if (GIF_optimizecmprow(dst, src, sb--, &lastchg_b, &lmpix, &rmpix)
			 && lmpix == 0 && rmpix == vid.width - 1)
				stopb = 1;
			if (firstchg_b < 0 && lastchg_b >= 0)
				firstchg_b = lastchg_b;
		}
	}

	if (lmpix < 0)  
	{



		*x = *y = 0;
		*w = *h = 1;
		return;
	}

	*x = lmpix;
	*y = (firstchg_t < 0 && lastchg_b >= 0) ? lastchg_b : firstchg_t;
	*w = rmpix + 1;
	*h = ((firstchg_b < 0 && lastchg_t >= 0) ? lastchg_t : firstchg_b) + 1;

	*w -= *x;
	*h -= *y;
}





static UINT8 *gifbwr_buf = NULL;
static UINT8 *gifbwr_cur;
static UINT8 gifbwr_bufsize = 0;

static UINT32 gifbwr_bits_buf = 0;
static INT32 gifbwr_bits_num = 0;
static UINT8 gifbwr_bits_min = 9;





static void GIF_bwrflush(void)
{
	if (gifbwr_bits_num > 0)  
	{
		WRITEUINT8(gifbwr_cur, (UINT8)(gifbwr_bits_buf&0xFF));
		++gifbwr_bufsize;
	}
	gifbwr_bits_buf = gifbwr_bits_num = 0;
}






static void GIF_bwrwrite(UINT32 idata)
{
	gifbwr_bits_buf |= (idata << gifbwr_bits_num);
	gifbwr_bits_num += gifbwr_bits_min;
	while (gifbwr_bits_num >= 8)
	{
		WRITEUINT8(gifbwr_cur, (UINT8)(gifbwr_bits_buf&0xFF));
		gifbwr_bits_buf >>= 8;
		gifbwr_bits_num -= 8;
		++gifbwr_bufsize;
	}
}





static UINT8 *scrbuf_pos;
static UINT8 *scrbuf_linebegin;
static UINT8 *scrbuf_lineend;
static UINT8 *scrbuf_writeend;
static INT16 scrbuf_downscaleamt = 1;





#define GIFLZW_TABLECLR  0x100
#define GIFLZW_DATAEND   0x101
#define GIFLZW_DICTSTART 0x102
#define GIFLZW_MAXCODE 4096

static UINT16 giflzw_workingCode;
static UINT16 giflzw_nextCodeToAssign;
static UINT32 *giflzw_hashTable = NULL;  





static void GIF_prepareLZW(void)
{
	gifbwr_bits_min = 9;
	giflzw_nextCodeToAssign = GIFLZW_DICTSTART;

	if (!giflzw_hashTable)
		giflzw_hashTable = Z_Malloc(16384*sizeof(UINT32), PU_STATIC, NULL);
	memset(giflzw_hashTable, 0, 16384*sizeof(UINT32));
}





static char GIF_searchHash(UINT32 key, UINT32 *pOutput)
{
	UINT32 entry, position = (key >> 6) & 0x3FFF;

	while (giflzw_hashTable[position] != 0)
	{
		entry = giflzw_hashTable[position];
		if ((entry >> 12) == key)
		{
			*pOutput = (entry & 0xFFF);
			return 1;
		}

		position = (position + 1) & 0x3FFF;
	}

	return 0;
}





static void GIF_addHash(UINT32 key, UINT32 value)
{
	UINT32 position = (key >> 6) & 0x3FFF;

	for (;;)
	{
		if (giflzw_hashTable[position] == 0)
		{
			giflzw_hashTable[position] = (key << 12) | (value & 0xFFF);
			return;
		}

		position = (position + 1) & 0x3FFF;
	}
}






static void GIF_feedByte(UINT8 pbyte)
{
	UINT32 key, hashOutput = 0;


	if (giflzw_workingCode == UINT16_MAX)
	{
		giflzw_workingCode = pbyte;
		return;
	}



	key = (giflzw_workingCode << 8) | pbyte;

	if (0 == GIF_searchHash(key, &hashOutput))
	{




		if (giflzw_nextCodeToAssign > (1 << gifbwr_bits_min))
			++gifbwr_bits_min;  

		GIF_bwrwrite(giflzw_workingCode);
		GIF_addHash(key, giflzw_nextCodeToAssign);
		++giflzw_nextCodeToAssign;



		giflzw_workingCode = pbyte;
		return;
	}


	giflzw_workingCode = hashOutput;
}





static void GIF_lzw(void)
{
	while (scrbuf_pos <= scrbuf_writeend)
	{
		GIF_feedByte(*scrbuf_pos);
		if (giflzw_nextCodeToAssign >= GIFLZW_MAXCODE)
		{
			GIF_bwrwrite(GIFLZW_TABLECLR);
			GIF_prepareLZW();
		}
		if ((scrbuf_pos += scrbuf_downscaleamt) >= scrbuf_lineend)
		{
			scrbuf_lineend += (vid.width * scrbuf_downscaleamt);
			scrbuf_linebegin += (vid.width * scrbuf_downscaleamt);
			scrbuf_pos = scrbuf_linebegin;
		}

		if (gifbwr_bufsize >= 248)
			break;
	}
	if (scrbuf_pos > scrbuf_writeend)
	{




		if (giflzw_nextCodeToAssign++ > (1 << gifbwr_bits_min))
			++gifbwr_bits_min;  
		GIF_bwrwrite(giflzw_workingCode);





		if (giflzw_nextCodeToAssign++ > (1 << gifbwr_bits_min))
			++gifbwr_bits_min;  
		GIF_bwrwrite(GIFLZW_DATAEND);


		GIF_bwrflush();
		gif_writeover = 1;
	}
}





const UINT8 gifhead_base[6] = {0x47,0x49,0x46,0x38,0x39,0x61};  
const UINT8 gifhead_nsid[19] = {0x21,0xFF,0x0B,  
	0x4E,0x45,0x54,0x53,0x43,0x41,0x50,0x45,0x32,0x2E,0x30,  
	0x03,0x01,0xFF,0xFF,0x00};  






static RGBA_t *GIF_getpalette(size_t palnum)
{

#ifdef HWRENDER
	if (rendermode == render_opengl)
		return pLocalPalette;
	else
#endif
		return (gif_colorprofile ? &pLocalPalette[palnum*256] : &pMasterPalette[palnum*256]);
}






static UINT8 *GIF_palwrite(UINT8 *p, RGBA_t *pal)
{
	INT32 i;
	for (i = 0; i < 256; i++)
	{
		WRITEUINT8(p, pal[i].s.red);
		WRITEUINT8(p, pal[i].s.green);
		WRITEUINT8(p, pal[i].s.blue);
	}
	return p;
}





static void GIF_headwrite(void)
{
	UINT8 *gifhead = Z_Malloc(800, PU_STATIC, NULL);
	UINT8 *p = gifhead;
	UINT16 rwidth, rheight;

	if (!gif_out)
		return;

	WRITEMEM(p, gifhead_base, sizeof(gifhead_base));


	if (gif_downscale)
	{
		scrbuf_downscaleamt = vid.dup;
		rwidth = (vid.width / scrbuf_downscaleamt);
		rheight = (vid.height / scrbuf_downscaleamt);
	}
	else
	{
		scrbuf_downscaleamt = 1;
		rwidth = vid.width;
		rheight = vid.height;
	}

	WRITEUINT16(p, rwidth);
	WRITEUINT16(p, rheight);


	WRITEUINT8(p, 0xF7);  
	WRITEUINT8(p, 0x00);
	WRITEUINT8(p, 0x00);


	p = GIF_palwrite(p, gif_headerpalette);


	WRITEMEM(p, gifhead_nsid, sizeof(gifhead_nsid));


	fwrite(gifhead, 1, 800, gif_out);
	Z_Free(gifhead);
}





const UINT8 gifframe_gchead[4] = {0x21,0xF9,0x04,0x04};  

static UINT8 *gifframe_data = NULL;
static size_t gifframe_size = 8192;





#ifdef HWRENDER
static colorlookup_t gif_colorlookup;

static void GIF_rgbconvert(UINT8 *linear, UINT8 *scr)
{
	UINT8 r, g, b;
	size_t src = 0, dest = 0;
	size_t size = (vid.width * vid.height * 3);

	InitColorLUT(&gif_colorlookup, (gif_localcolortable) ? gif_framepalette : gif_headerpalette, true);

	while (src < size)
	{
		r = (UINT8)linear[src];
		g = (UINT8)linear[src + 1];
		b = (UINT8)linear[src + 2];
		scr[dest] = GetColorLUTDirect(&gif_colorlookup, r, g, b);
		src += (3 * scrbuf_downscaleamt);
		dest += scrbuf_downscaleamt;
	}
}
#endif





static void GIF_framewrite(void)
{
	UINT8 *p;
	UINT8 *movie_screen = screens[2];
	INT32 blitx, blity, blitw, blith;
	boolean palchanged;

	if (!gifframe_data)
		gifframe_data = Z_Malloc(gifframe_size, PU_STATIC, NULL);
	p = gifframe_data;

	if (!gif_out)
		return;


	if (gif_localcolortable)
	{
		gif_framepalette = GIF_getpalette(max(st_palette, 0));
		palchanged = memcmp(gif_headerpalette, gif_framepalette, sizeof(RGBA_t) * 256);
	}
	else
		palchanged = false;



	if (gif_optimize && gif_frames > 0 && (!palchanged))
	{

		UINT8 *cur_screen = screens[0];
		GIF_optimizeregion(cur_screen, movie_screen, &blitx, &blity, &blitw, &blith);


		if (rendermode == render_soft)
			I_ReadScreen(movie_screen);
#ifdef HWRENDER
		else if (rendermode == render_opengl)
		{
			UINT8 *linear = HWR_GetScreenshot();
			GIF_rgbconvert(linear, movie_screen);
			free(linear);
		}
#endif
	}
	else
	{
		blitx = blity = 0;
		blitw = vid.width;
		blith = vid.height;

#ifdef HWRENDER

		if (rendermode == render_opengl)
		{
			UINT8 *linear = HWR_GetScreenshot();
			GIF_rgbconvert(linear, screens[0]);
			free(linear);
		}
#endif



		if (gif_frames == 0 && rendermode == render_soft)
			I_ReadScreen(movie_screen);

		movie_screen = screens[0];
	}


	{
		UINT16 delay = 0;
		INT32 startline;

		if (gif_dynamicdelay ==(UINT8) 2 && !singletics)
		{

			UINT16 mingifdelay = 10;  
			gif_delayus += (I_GetPreciseTime() - gif_prevframetime) / (I_GetPrecisePrecision() / 1000000);  

			if (gif_delayus/1000 >= mingifdelay)  
			{
				int frames = (gif_delayus/1000) / mingifdelay;  
				delay = frames;  
				gif_delayus -= frames*(mingifdelay*1000);  
			}
		}
		else if (gif_dynamicdelay ==(UINT8) 1 && !singletics)
		{
			const UINT64 precision = I_GetPrecisePrecision();
			const UINT64 elapsed = I_GetPreciseTime() - gif_prevframetime;
			const UINT64 scaled = elapsed * 100ULL + gif_delayremainder;

			                                                                       
                                                                          
                                                                       
			if (precision)
			{
				delay = (UINT16)max(1ULL, scaled / precision);
				gif_delayremainder = scaled % precision;
			}
			else
				delay = 1;
		}
		else
		{

			int d1 = (int)((100.0f/max(cv_gif_maxfps.value, 1))*(gif_frames+1));
			int d2 = (int)((100.0f/max(cv_gif_maxfps.value, 1))*(gif_frames));
			delay = d1-d2;
			if (!delay)
				delay = 1;
		}

		WRITEMEM(p, gifframe_gchead, 4);

		WRITEUINT16(p, delay);
		WRITEUINT8(p, 0);
		WRITEUINT8(p, 0);  

		if (scrbuf_downscaleamt > 1)
		{

			blitx -= (blitx % scrbuf_downscaleamt);
			blity -= (blity % scrbuf_downscaleamt);
			blitw = ((blitw + (scrbuf_downscaleamt - 1)) / scrbuf_downscaleamt) * scrbuf_downscaleamt;
			blith = ((blith + (scrbuf_downscaleamt - 1)) / scrbuf_downscaleamt) * scrbuf_downscaleamt;
		}

		WRITEUINT8(p, 0x2C);
		WRITEUINT16(p, (UINT16)(blitx / scrbuf_downscaleamt));
		WRITEUINT16(p, (UINT16)(blity / scrbuf_downscaleamt));
		WRITEUINT16(p, (UINT16)(blitw / scrbuf_downscaleamt));
		WRITEUINT16(p, (UINT16)(blith / scrbuf_downscaleamt));

		if (!gif_localcolortable)
			WRITEUINT8(p, 0);  
		else
		{
			if (palchanged)
			{

				WRITEUINT8(p, 0x87);  
				p = GIF_palwrite(p, gif_framepalette);
			}
			else
				WRITEUINT8(p, 0);  
		}

		scrbuf_pos = movie_screen + blitx + (blity * vid.width);
		scrbuf_writeend = scrbuf_pos + (blitw - 1) + ((blith - 1) * vid.width);

		if (!gifbwr_buf)
			gifbwr_buf = Z_Malloc(256, PU_STATIC, NULL);
		gifbwr_cur = gifbwr_buf;

		GIF_prepareLZW();
		giflzw_workingCode = UINT16_MAX;
		WRITEUINT8(p, gifbwr_bits_min - 1);

		startline = (scrbuf_pos - movie_screen) / vid.width;
		scrbuf_linebegin = movie_screen + (startline * vid.width) + blitx;
		scrbuf_lineend = scrbuf_linebegin + blitw;


		GIF_bwrwrite(GIFLZW_TABLECLR);

		gif_writeover = 0;
		while (!gif_writeover)
		{
			GIF_lzw();  

			if ((size_t)(p - gifframe_data) + gifbwr_bufsize + 1 >= gifframe_size)
			{
				INT32 temppos = p - gifframe_data;
				gifframe_data = Z_Realloc(gifframe_data, (gifframe_size *= 2), PU_STATIC, NULL);
				p = gifframe_data + temppos;  
			}


			gifbwr_cur = gifbwr_buf;
			WRITEUINT8(p, gifbwr_bufsize);
			WRITEMEM(p, gifbwr_cur, gifbwr_bufsize);

			gifbwr_bufsize = 0;
			gifbwr_cur = gifbwr_buf;
		}
		WRITEUINT8(p, 0);  
	}
	fwrite(gifframe_data, 1, (p - gifframe_data), gif_out);
	++gif_frames;
}











INT32 GIF_open(const char *filename)
{
	gif_out = fopen(filename, "wb");
	if (!gif_out)
		return 0;

	gif_optimize = (!!cv_gif_optimize.value);
	gif_downscale = (!!cv_gif_downscale.value);
	gif_dynamicdelay = (UINT8)cv_gif_dynamicdelay.value;
	gif_localcolortable = (!!cv_gif_localcolortable.value);
	gif_colorprofile = (!!cv_screenshot_colorprofile.value);
	gif_headerpalette = GIF_getpalette(0);

	GIF_headwrite();
	gif_frames = 0;
	gif_prevframetime = I_GetPreciseTime();
	gif_delayremainder = 0;
	gif_prevcapturetime = 0;
	gif_delayus = 0;
	return 1;
}





void GIF_frame(void)
{
	precise_t now = I_GetPreciseTime();
	UINT64 interval = I_GetPrecisePrecision() / (UINT32)max(cv_gif_maxfps.value, 1);




	if (gif_prevcapturetime && (now - gif_prevcapturetime) < interval)
		return;
	gif_prevcapturetime = now;
	GIF_framewrite();
	gif_prevframetime = now;
}





INT32 GIF_close(void)
{
	if (!gif_out)
		return 0;


	fwrite(";", 1, 1, gif_out);
	fclose(gif_out);
	gif_out = NULL;

	if (gifbwr_buf)
		Z_Free(gifbwr_buf);
	gifbwr_buf = gifbwr_cur = NULL;

	if (gifframe_data)
		Z_Free(gifframe_data);
	gifframe_data = NULL;

	if (giflzw_hashTable)
		Z_Free(giflzw_hashTable);
	giflzw_hashTable = NULL;

	CONS_Printf(M_GetText("Animated gif closed; wrote %d frames\n"), gif_frames);
	return 1;
}
#endif  
