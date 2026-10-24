// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "doomdef.h"
#include "g_game.h"
#include "i_system.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "p_local.h"
#include "m_misc.h"
#include "r_data.h"
#include "r_textures.h"
#include "r_translation.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_setup.h"  
#include "v_video.h"  
#include "f_finale.h"  
#include "byteptr.h"
#include "dehacked.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"  
#endif









size_t numspritelumps, max_spritelumps;


sprcache_t *spritecachedinfo;

lighttable_t *colormaps;
lighttable_t *fadecolormap;


size_t flatmemory, spritememory, texturememory;



UINT32 ASTBlendPixel(RGBA_t background, RGBA_t foreground, int style, UINT8 alpha)
{
	RGBA_t output;
	INT16 fullalpha = (alpha - (0xFF - foreground.s.alpha));
	if (style == AST_TRANSLUCENT)
	{
		if (fullalpha <= 0)
			output.rgba = background.rgba;
		else
		{

			if (fullalpha >= 0xFF)
				fullalpha = 0xFF;
			alpha = (UINT8)fullalpha;



			if (!background.s.alpha)
			{

				if (alpha == 0xFF)
					output.rgba = foreground.rgba;
				else
					output.rgba = 0;
			}
			else
			{
				UINT8 beta = (0xFF - alpha);
				output.s.red = ((background.s.red * beta) + (foreground.s.red * alpha)) / 0xFF;
				output.s.green = ((background.s.green * beta) + (foreground.s.green * alpha)) / 0xFF;
				output.s.blue = ((background.s.blue * beta) + (foreground.s.blue * alpha)) / 0xFF;
				output.s.alpha = 0xFF;
			}
		}
		return output.rgba;
	}
#define clamp(c) max(min(c, 0xFF), 0x00);
	else
	{
		float falpha = ((float)alpha / 256.0f);
		float fr = ((float)foreground.s.red * falpha);
		float fg = ((float)foreground.s.green * falpha);
		float fb = ((float)foreground.s.blue * falpha);
		if (style == AST_ADD)
		{
			output.s.red = clamp((int)(background.s.red + fr));
			output.s.green = clamp((int)(background.s.green + fg));
			output.s.blue = clamp((int)(background.s.blue + fb));
		}
		else if (style == AST_SUBTRACT)
		{
			output.s.red = clamp((int)(background.s.red - fr));
			output.s.green = clamp((int)(background.s.green - fg));
			output.s.blue = clamp((int)(background.s.blue - fb));
		}
		else if (style == AST_REVERSESUBTRACT)
		{
			output.s.red = clamp((int)((-background.s.red) + fr));
			output.s.green = clamp((int)((-background.s.green) + fg));
			output.s.blue = clamp((int)((-background.s.blue) + fb));
		}
		else if (style == AST_MODULATE)
		{
			fr = ((float)foreground.s.red / 256.0f);
			fg = ((float)foreground.s.green / 256.0f);
			fb = ((float)foreground.s.blue / 256.0f);
			output.s.red = clamp((int)(background.s.red * fr));
			output.s.green = clamp((int)(background.s.green * fg));
			output.s.blue = clamp((int)(background.s.blue * fb));
		}

		else if (style == AST_COPY)
			output.rgba = foreground.rgba;

		output.s.alpha = 0xFF;
		return output.rgba;
	}
#undef clamp
	return 0;
}

INT32 ASTTextureBlendingThreshold[2] = {255/11, (10*255/11)};


UINT32 ASTBlendTexturePixel(RGBA_t background, RGBA_t foreground, int style, UINT8 alpha)
{

	if (style == AST_TRANSLUCENT)
	{

		if (alpha <= ASTTextureBlendingThreshold[1])
		{

			if (alpha < ASTTextureBlendingThreshold[0])
				return background.rgba;

			return ASTBlendPixel(background, foreground, style, alpha);
		}
		else  
			return foreground.rgba;
	}
	else
		return ASTBlendPixel(background, foreground, style, alpha);
}



UINT8 ASTBlendPaletteIndexes(UINT8 background, UINT8 foreground, int style, UINT8 alpha)
{

	if (style == AST_TRANSLUCENT)
	{

		if (alpha <= ASTTextureBlendingThreshold[1])
		{
			UINT8 *mytransmap;
			INT32 trans;


			if (alpha < ASTTextureBlendingThreshold[0])
				return background;


			trans = (8*(alpha) + 255/8)/(255 - 255/11);
			mytransmap = R_GetTranslucencyTable(trans + 1);
			if (background != 0xFF)
				return *(mytransmap + (background<<8) + foreground);
		}
		else  
			return foreground;
	}

	else if (style == AST_COPY)
		return foreground;


	else if (style != AST_TRANSLUCENT)
	{
		RGBA_t texel;
		RGBA_t bg = V_GetMasterColor(background);
		RGBA_t fg = V_GetMasterColor(foreground);
		texel.rgba = ASTBlendPixel(bg, fg, style, alpha);
		return NearestColor(texel.s.red, texel.s.green, texel.s.blue);
	}


	return background;
}

#ifdef EXTRACOLORMAPLUMPS
static lumplist_t *colormaplumps = NULL;  
static size_t numcolormaplumps = 0;

static inline lumpnum_t R_CheckNumForNameList(const char *name, lumplist_t *list, size_t listsize)
{
	size_t i;
	UINT16 lump;

	for (i = listsize - 1; i < INT16_MAX; i--)
	{
		lump = W_CheckNumForNamePwad(name, list[i].wadfile, list[i].firstlump);
		if (lump == INT16_MAX || lump > (list[i].firstlump + list[i].numlumps))
			continue;
		else
			return (list[i].wadfile<<16)+lump;
	}
	return LUMPERROR;
}

static void R_InitExtraColormaps(void)
{
	lumpnum_t startnum, endnum;
	UINT16 cfile, clump;
	static size_t maxcolormaplumps = 16;

	for (cfile = clump = 0; cfile < numwadfiles; cfile++, clump = 0)
	{
		startnum = W_CheckNumForNamePwad("C_START", cfile, clump);
		if (startnum == INT16_MAX)
			continue;

		endnum = W_CheckNumForNamePwad("C_END", cfile, clump);

		if (endnum == INT16_MAX)
			I_Error("R_InitExtraColormaps: C_START without C_END\n");





		if (numcolormaplumps >= maxcolormaplumps)
			maxcolormaplumps *= 2;
		colormaplumps = Z_Realloc(colormaplumps,
			sizeof (*colormaplumps) * maxcolormaplumps, PU_STATIC, NULL);
		colormaplumps[numcolormaplumps].wadfile = cfile;
		colormaplumps[numcolormaplumps].firstlump = startnum+1;
		colormaplumps[numcolormaplumps].numlumps = endnum - (startnum + 1);
		numcolormaplumps++;
	}
	CONS_Printf(M_GetText("Number of Extra Colormaps: %s\n"), sizeu1(numcolormaplumps));
}
#endif










static void R_InitSpriteLumps(void)
{
	numspritelumps = 0;
	max_spritelumps = 8192;

	Z_Malloc(max_spritelumps*sizeof(*spritecachedinfo), PU_STATIC, &spritecachedinfo);
}





static void R_CreateFadeColormaps(void)
{
	UINT8 px, fade;
	RGBA_t rgba;
	INT32 r, g, b;
	size_t len, i;

	len = (256 * FADECOLORMAPROWS);
	fadecolormap = Z_MallocAlign(len*2, PU_STATIC, NULL, 8);
	for (i = 0; i < len*2; i++)
		fadecolormap[i] = (i%256);


	{
		lumpnum_t lump = W_CheckNumForName("FADECMAP");
		lumpnum_t wlump = W_CheckNumForName("FADEWMAP");


		if (lump != LUMPERROR)
			W_ReadLumpHeader(lump, fadecolormap, len, 0U);

		if (wlump != LUMPERROR)
			W_ReadLumpHeader(wlump, fadecolormap+len, len, 0U);


		if (lump != LUMPERROR && wlump == LUMPERROR)
			goto makewhite;

		else if (lump == LUMPERROR && wlump != LUMPERROR)
			goto makeblack;

		else if (lump != LUMPERROR && wlump != LUMPERROR)
			return;
	}

#define GETCOLOR \
	px = colormaps[i%256]; \
	fade = (i/256) * (256 / FADECOLORMAPROWS); \
	rgba = V_GetMasterColor(px);


	makeblack:
	for (i = 0; i < len; i++)
	{

		GETCOLOR;


		r = rgba.s.red - FADEREDFACTOR*fade/10;
		g = rgba.s.green - FADEGREENFACTOR*fade/10;
		b = rgba.s.blue - FADEBLUEFACTOR*fade/10;


		if (r < 0) r = 0;
		if (g < 0) g = 0;
		if (b < 0) b = 0;


		fadecolormap[i] = NearestColor(r,g,b);
	}


	makewhite:
	for (i = len; i < len*2; i++)
	{

		GETCOLOR;


		r = rgba.s.red + FADEREDFACTOR*fade/10;
		g = rgba.s.green + FADEGREENFACTOR*fade/10;
		b = rgba.s.blue + FADEBLUEFACTOR*fade/10;


		if (r > 255) r = 255;
		if (g > 255) g = 255;
		if (b > 255) b = 255;


		fadecolormap[i] = NearestColor(r,g,b);
	}
#undef GETCOLOR
}




static void R_InitColormaps(void)
{
	size_t len;
	lumpnum_t lump;


	lump = W_GetNumForName("COLORMAP");
	len = W_LumpLength(lump);
	colormaps = Z_MallocAlign(len, PU_STATIC, NULL, 8);
	W_ReadLump(lump, colormaps);


	R_CreateFadeColormaps();


	R_ClearColormaps();
#ifdef EXTRACOLORMAPLUMPS
	R_InitExtraColormaps();
#endif
}

void R_ReInitColormaps(UINT16 num)
{
	char colormap[9] = "COLORMAP";
	lumpnum_t lump;
	const lumpnum_t basecolormaplump = W_GetNumForName(colormap);
	if (num > 0 && num <= 10000)
		snprintf(colormap, 8, "CLM%04u", num-1);


	lump = W_GetNumForName(colormap);
	if (lump == LUMPERROR)
		lump = basecolormaplump;
	else
	{
		if (W_LumpLength(lump) != W_LumpLength(basecolormaplump))
		{
			CONS_Alert(CONS_WARNING, "%s lump size does not match COLORMAP, results may be unexpected.\n", colormap);
		}
	}

	W_ReadLumpHeader(lump, colormaps, W_LumpLength(basecolormaplump), 0U);
	if (fadecolormap)
		Z_Free(fadecolormap);
	R_CreateFadeColormaps();


	R_ClearColormaps();
}






void R_ClearColormaps(void)
{

	extra_colormaps = R_CreateDefaultColormap(true);
}





extracolormap_t *R_CreateDefaultColormap(boolean lighttable)
{
	extracolormap_t *exc = Z_Calloc(sizeof (*exc), PU_LEVEL, NULL);
	exc->fadestart = 0;
	exc->fadeend = 31;
	exc->flags = 0;
	exc->rgba = 0;
	exc->fadergba = 0xFF000000;
	exc->colormap = lighttable ? R_CreateLightTable(exc) : NULL;
#ifdef EXTRACOLORMAPLUMPS
	exc->lump = LUMPERROR;
	exc->lumpname[0] = 0;
#endif
	exc->next = exc->prev = NULL;
	return exc;
}




extracolormap_t *R_GetDefaultColormap(void)
{
#ifdef COLORMAPREVERSELIST
	extracolormap_t *exc;
#endif

	if (!extra_colormaps)
		return (extra_colormaps = R_CreateDefaultColormap(true));

#ifdef COLORMAPREVERSELIST
	for (exc = extra_colormaps; exc->next; exc = exc->next);
	return exc;
#else
	return extra_colormaps;
#endif
}





extracolormap_t *R_CopyColormap(extracolormap_t *extra_colormap, boolean lighttable)
{
	extracolormap_t *exc = Z_Calloc(sizeof (*exc), PU_LEVEL, NULL);

	if (!extra_colormap)
		extra_colormap = R_GetDefaultColormap();

	*exc = *extra_colormap;
	exc->next = exc->prev = NULL;

#ifdef EXTRACOLORMAPLUMPS
	strncpy(exc->lumpname, extra_colormap->lumpname, 9);

	if (exc->lump != LUMPERROR && lighttable)
	{

		exc->colormap = Z_MallocAlign(W_LumpLength(lump), PU_LEVEL, NULL, 16);
		W_ReadLump(lump, exc->colormap);
	}
	else
#endif
	if (lighttable)
		exc->colormap = R_CreateLightTable(exc);
	else
		exc->colormap = NULL;

	return exc;
}







void R_AddColormapToList(extracolormap_t *extra_colormap)
{
#ifndef COLORMAPREVERSELIST
	extracolormap_t *exc;
#endif

	if (!extra_colormaps)
	{
		extra_colormaps = extra_colormap;
		extra_colormap->next = 0;
		extra_colormap->prev = 0;
		return;
	}

#ifdef COLORMAPREVERSELIST
	extra_colormaps->prev = extra_colormap;
	extra_colormap->next = extra_colormaps;
	extra_colormaps = extra_colormap;
	extra_colormap->prev = 0;
#else
	for (exc = extra_colormaps; exc->next; exc = exc->next);

	exc->next = extra_colormap;
	extra_colormap->prev = exc;
	extra_colormap->next = 0;
#endif
}




#ifdef EXTRACOLORMAPLUMPS
boolean R_CheckDefaultColormapByValues(boolean checkrgba, boolean checkfadergba, boolean checkparams,
	INT32 rgba, INT32 fadergba, UINT8 fadestart, UINT8 fadeend, UINT8 flags, lumpnum_t lump)
#else
boolean R_CheckDefaultColormapByValues(boolean checkrgba, boolean checkfadergba, boolean checkparams,
	INT32 rgba, INT32 fadergba, UINT8 fadestart, UINT8 fadeend, UINT8 flags)
#endif
{
	return (
		(!checkparams ? true :
			(fadestart == 0
				&& fadeend == 31
				&& !flags)
			)
		&& (!checkrgba ? true : rgba == 0)
		&& (!checkfadergba ? true : (unsigned)fadergba == 0xFF000000)
#ifdef EXTRACOLORMAPLUMPS
		&& lump == LUMPERROR
		&& extra_colormap->lumpname[0] == 0
#endif
		);
}

boolean R_CheckDefaultColormap(extracolormap_t *extra_colormap, boolean checkrgba, boolean checkfadergba, boolean checkparams)
{
	if (!extra_colormap)
		return true;

#ifdef EXTRACOLORMAPLUMPS
	return R_CheckDefaultColormapByValues(checkrgba, checkfadergba, checkparams, extra_colormap->rgba, extra_colormap->fadergba, extra_colormap->fadestart, extra_colormap->fadeend, extra_colormap->flags, extra_colormap->lump);
#else
	return R_CheckDefaultColormapByValues(checkrgba, checkfadergba, checkparams, extra_colormap->rgba, extra_colormap->fadergba, extra_colormap->fadestart, extra_colormap->fadeend, extra_colormap->flags);
#endif
}

boolean R_CheckEqualColormaps(extracolormap_t *exc_a, extracolormap_t *exc_b, boolean checkrgba, boolean checkfadergba, boolean checkparams)
{


	if (!exc_a)
		exc_a = R_GetDefaultColormap();
	if (!exc_b)
		exc_b = R_GetDefaultColormap();

	if (exc_a == exc_b)
		return true;

	return (
		(!checkparams ? true :
			(exc_a->fadestart == exc_b->fadestart
				&& exc_a->fadeend == exc_b->fadeend
				&& exc_a->flags == exc_b->flags)
			)
		&& (!checkrgba ? true : exc_a->rgba == exc_b->rgba)
		&& (!checkfadergba ? true : exc_a->fadergba == exc_b->fadergba)
#ifdef EXTRACOLORMAPLUMPS
		&& exc_a->lump == exc_b->lump
		&& !strncmp(exc_a->lumpname, exc_b->lumpname, 9)
#endif
		);
}





#ifdef EXTRACOLORMAPLUMPS
extracolormap_t *R_GetColormapFromListByValues(INT32 rgba, INT32 fadergba, UINT8 fadestart, UINT8 fadeend, UINT8 flags, lumpnum_t lump)
#else
extracolormap_t *R_GetColormapFromListByValues(INT32 rgba, INT32 fadergba, UINT8 fadestart, UINT8 fadeend, UINT8 flags)
#endif
{
	extracolormap_t *exc;
	UINT32 dbg_i = 0;

	for (exc = extra_colormaps; exc; exc = exc->next)
	{
		if (rgba == exc->rgba
			&& fadergba == exc->fadergba
			&& fadestart == exc->fadestart
			&& fadeend == exc->fadeend
			&& flags == exc->flags
#ifdef EXTRACOLORMAPLUMPS
			&& (lump != LUMPERROR && lump == exc->lump)
#endif
		)
		{
			CONS_Debug(DBG_RENDER, "Found Colormap %d: rgba(%d,%d,%d,%d) fadergba(%d,%d,%d,%d)\n",
				dbg_i, R_GetRgbaR(rgba), R_GetRgbaG(rgba), R_GetRgbaB(rgba), R_GetRgbaA(rgba),
				R_GetRgbaR(fadergba), R_GetRgbaG(fadergba), R_GetRgbaB(fadergba), R_GetRgbaA(fadergba));
			return exc;
		}
		dbg_i++;
	}
	return NULL;
}

extracolormap_t *R_GetColormapFromList(extracolormap_t *extra_colormap)
{
#ifdef EXTRACOLORMAPLUMPS
	return R_GetColormapFromListByValues(extra_colormap->rgba, extra_colormap->fadergba, extra_colormap->fadestart, extra_colormap->fadeend, extra_colormap->flags, extra_colormap->lump);
#else
	return R_GetColormapFromListByValues(extra_colormap->rgba, extra_colormap->fadergba, extra_colormap->fadestart, extra_colormap->fadeend, extra_colormap->flags);
#endif
}

#ifdef EXTRACOLORMAPLUMPS
extracolormap_t *R_ColormapForName(char *name)
{
	lumpnum_t lump;
	extracolormap_t *exc;

	lump = R_CheckNumForNameList(name, colormaplumps, numcolormaplumps);
	if (lump == LUMPERROR)
		I_Error("R_ColormapForName: Cannot find colormap lump %.8s\n", name);

	exc = R_GetColormapFromListByValues(0, 0xFF000000, 0, 31, 0, lump);
	if (exc)
		return exc;

	exc = Z_Calloc(sizeof (*exc), PU_LEVEL, NULL);

	exc->lump = lump;
	strncpy(exc->lumpname, name, 9);
	exc->lumpname[8] = 0;


	exc->colormap = Z_MallocAlign(W_LumpLength(lump), PU_LEVEL, NULL, 16);
	W_ReadLump(lump, exc->colormap);



	exc->fadestart = 0;
	exc->fadeend = 31;
	exc->flags = 0;
	exc->rgba = 0;
	exc->fadergba = 0xFF000000;

	R_AddColormapToList(exc);

	return exc;
}
#endif









static double deltas[256][3], map[256][3];

static colorlookup_t lighttable_lut;

static UINT8 LightTableNearest(UINT8 r, UINT8 g, UINT8 b)
{
	return NearestColor(r, g, b);
}

static UINT8 LightTableNearest_LUT(UINT8 r, UINT8 g, UINT8 b)
{
	return GetColorLUT(&lighttable_lut, r, g, b);
}

lighttable_t *R_CreateLightTable(extracolormap_t *extra_colormap)
{
	extra_colormap->colormap = Z_MallocAlign((256 * 34) + 10, PU_LEVEL, NULL, 8);
	R_GenerateLightTable(extra_colormap, false);
	return extra_colormap->colormap;
}

void R_GenerateLightTable(extracolormap_t *extra_colormap, boolean uselookup)
{
	double cmaskr, cmaskg, cmaskb, cdestr, cdestg, cdestb;
	double maskamt = 0, othermask = 0;

	UINT8 cr = R_GetRgbaR(extra_colormap->rgba),
		cg = R_GetRgbaG(extra_colormap->rgba),
		cb = R_GetRgbaB(extra_colormap->rgba),
		ca = R_GetRgbaA(extra_colormap->rgba),
		cfr = R_GetRgbaR(extra_colormap->fadergba),
		cfg = R_GetRgbaG(extra_colormap->fadergba),
		cfb = R_GetRgbaB(extra_colormap->fadergba);


	UINT8 fadestart = extra_colormap->fadestart,
		fadedist = extra_colormap->fadeend - extra_colormap->fadestart;

	size_t i;




	cmaskr = cr;
	cmaskg = cg;
	cmaskb = cb;

	maskamt = (double)(ca/255.0l);
	othermask = 1 - maskamt;
	maskamt /= 0xff;

	cmaskr *= maskamt;
	cmaskg *= maskamt;
	cmaskb *= maskamt;




	cdestr = cfr;
	cdestg = cfg;
	cdestb = cfb;













	{
		double r, g, b, cbrightness;
		int p;
		char *colormap_p;

		UINT8 (*NearestColorFunc)(UINT8, UINT8, UINT8);

		if (uselookup)
		{
			InitColorLUT(&lighttable_lut, pMasterPalette, false);
			NearestColorFunc = LightTableNearest_LUT;
		}
		else
			NearestColorFunc = LightTableNearest;






		for (i = 0; i < 256; i++)
		{
			r = pMasterPalette[i].s.red;
			g = pMasterPalette[i].s.green;
			b = pMasterPalette[i].s.blue;
			cbrightness = sqrt((r*r) + (g*g) + (b*b));

			map[i][0] = (cbrightness * cmaskr) + (r * othermask);
			if (map[i][0] > 255.0l)
				map[i][0] = 255.0l;
			deltas[i][0] = (map[i][0] - cdestr) / (double)fadedist;

			map[i][1] = (cbrightness * cmaskg) + (g * othermask);
			if (map[i][1] > 255.0l)
				map[i][1] = 255.0l;
			deltas[i][1] = (map[i][1] - cdestg) / (double)fadedist;

			map[i][2] = (cbrightness * cmaskb) + (b * othermask);
			if (map[i][2] > 255.0l)
				map[i][2] = 255.0l;
			deltas[i][2] = (map[i][2] - cdestb) / (double)fadedist;
		}



		colormap_p = (char *)extra_colormap->colormap;



		for (p = 0; p < 34; p++)
		{
			for (i = 0; i < 256; i++)
			{
				*colormap_p = NearestColorFunc((UINT8)M_RoundUp(map[i][0]),
					(UINT8)M_RoundUp(map[i][1]),
					(UINT8)M_RoundUp(map[i][2]));
				colormap_p++;

				if ((UINT32)p < fadestart)
					continue;
#define ABS2(x) ((x) < 0 ? -(x) : (x))
				if (ABS2(map[i][0] - cdestr) > ABS2(deltas[i][0]))
					map[i][0] -= deltas[i][0];
				else
					map[i][0] = cdestr;

				if (ABS2(map[i][1] - cdestg) > ABS2(deltas[i][1]))
					map[i][1] -= deltas[i][1];
				else
					map[i][1] = cdestg;

				if (ABS2(map[i][2] - cdestb) > ABS2(deltas[i][1]))
					map[i][2] -= deltas[i][2];
				else
					map[i][2] = cdestb;
#undef ABS2
			}
		}
	}
}

void R_UpdateLightTable(extracolormap_t *extra_colormap, boolean uselookup)
{
	R_GenerateLightTable(extra_colormap, uselookup);

#ifdef HWRENDER
	extra_colormap->gl_lighttable.needs_update = true;
#endif
}

extracolormap_t *R_CreateColormapFromLinedef(char *p1, char *p2, char *p3)
{

	UINT8 cr = 0, cg = 0, cb = 0, ca = 0, cfr = 0, cfg = 0, cfb = 0, cfa = 25;
	UINT32 fadestart = 0, fadeend = 31;
	UINT8 flags = 0;
	INT32 rgba = 0, fadergba = 0xFF000000;

#define HEX2INT(x) (UINT32)(x >= '0' && x <= '9' ? x - '0' : x >= 'a' && x <= 'f' ? x - 'a' + 10 : x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
#define ALPHA2INT(x) (x >= 'a' && x <= 'z' ? x - 'a' : x >= 'A' && x <= 'Z' ? x - 'A' : x >= '0' && x <= '9' ? 25 : 0)



	if (p1[0] >= 'a' && p1[0] <= 'z' && !p1[1])
		ca = ((p1[0] - 'a') * 102) / 10;
	else if (p1[0] == '#' && p1[1] >= 'a' && p1[1] <= 'z' && !p1[2])
		ca = ((p1[1] - 'a') * 102) / 10;
	else if (p1[0] >= 'A' && p1[0] <= 'Z' && !p1[1])
		ca = ((p1[0] - 'A') * 102) / 10;
	else if (p1[0] == '#' && p1[1] >= 'A' && p1[1] <= 'Z' && !p1[2])
		ca = ((p1[1] - 'A') * 102) / 10;
	else if (p1[0] == '#')
	{


		if (p1[1] && p1[2])
		{
			cr = ((HEX2INT(p1[1]) * 16) + HEX2INT(p1[2]));
			if (p1[3] && p1[4])
			{
				cg = ((HEX2INT(p1[3]) * 16) + HEX2INT(p1[4]));
				if (p1[5] && p1[6])
				{
					cb = ((HEX2INT(p1[5]) * 16) + HEX2INT(p1[6]));

					if (p1[7] >= 'a' && p1[7] <= 'z')
						ca = ((p1[7] - 'a') * 102) / 10;
					else if (p1[7] >= 'A' && p1[7] <= 'Z')
						ca = ((p1[7] - 'A') * 102) / 10;
					else
						ca = 255;
				}
				else
					ca = 255;
			}
			else
				ca = 255;
		}
		else
			ca = 255;
	}

#define NUMFROMCHAR(c) (c >= '0' && c <= '9' ? c - '0' : 0)


	if (p2[0] == '#')
	{
		if (p2[1])
		{
			flags = NUMFROMCHAR(p2[1]);
			if (p2[2] && p2[3])
			{
				fadestart = NUMFROMCHAR(p2[3]) + (NUMFROMCHAR(p2[2]) * 10);
				if (p2[4] && p2[5])
					fadeend = NUMFROMCHAR(p2[5]) + (NUMFROMCHAR(p2[4]) * 10);
			}
		}

		if (fadestart > 30)
			fadestart = 0;
		if (fadeend > 31 || fadeend < 1)
			fadeend = 31;
	}

#undef NUMFROMCHAR



	if (p3[0] >= 'a' && p3[0] <= 'z' && !p3[1])
		cfa = ((p3[0] - 'a') * 102) / 10;
	else if (p3[0] == '#' && p3[1] >= 'a' && p3[1] <= 'z' && !p3[2])
		cfa = ((p3[1] - 'a') * 102) / 10;
	else if (p3[0] >= 'A' && p3[0] <= 'Z' && !p3[1])
		cfa = ((p3[0] - 'A') * 102) / 10;
	else if (p3[0] == '#' && p3[1] >= 'A' && p3[1] <= 'Z' && !p3[2])
		cfa = ((p3[1] - 'A') * 102) / 10;
	else if (p3[0] == '#')
	{


		if (p3[1] && p3[2])
		{
			cfr = ((HEX2INT(p3[1]) * 16) + HEX2INT(p3[2]));
			if (p3[3] && p3[4])
			{
				cfg = ((HEX2INT(p3[3]) * 16) + HEX2INT(p3[4]));
				if (p3[5] && p3[6])
				{
					cfb = ((HEX2INT(p3[5]) * 16) + HEX2INT(p3[6]));

					if (p3[7] >= 'a' && p3[7] <= 'z')
						cfa = ((p3[7] - 'a') * 102) / 10;
					else if (p3[7] >= 'A' && p3[7] <= 'Z')
						cfa = ((p3[7] - 'A') * 102) / 10;
					else
						cfa = 255;
				}
				else
					cfa = 255;
			}
			else
				cfa = 255;
		}
		else
			cfa = 255;
	}
#undef ALPHA2INT
#undef HEX2INT



	rgba = R_PutRgbaRGBA(cr, cg, cb, ca);
	fadergba = R_PutRgbaRGBA(cfr, cfg, cfb, cfa);

	return R_CreateColormap(rgba, fadergba, fadestart, fadeend, flags);
}

extracolormap_t *R_CreateColormap(INT32 rgba, INT32 fadergba, UINT8 fadestart, UINT8 fadeend, UINT8 flags)
{
	extracolormap_t *extra_colormap;


#ifdef EXTRACOLORMAPLUMPS
	if (R_CheckDefaultColormapByValues(true, true, true, rgba, fadergba, fadestart, fadeend, flags, LUMPERROR))
		return NULL;
#else
	if (R_CheckDefaultColormapByValues(true, true, true, rgba, fadergba, fadestart, fadeend, flags))
		return NULL;
#endif


#ifdef EXTRACOLORMAPLUMPS
	extra_colormap = R_GetColormapFromListByValues(rgba, fadergba, fadestart, fadeend, flags, LUMPERROR);
#else
	extra_colormap = R_GetColormapFromListByValues(rgba, fadergba, fadestart, fadeend, flags);
#endif
	if (extra_colormap)
		return extra_colormap;

	CONS_Debug(DBG_RENDER, "Creating Colormap: rgba(%x) fadergba(%x)\n", rgba, fadergba);

	extra_colormap = Z_Calloc(sizeof(*extra_colormap), PU_LEVEL, NULL);

	extra_colormap->fadestart = (UINT16)fadestart;
	extra_colormap->fadeend = (UINT16)fadeend;
	extra_colormap->flags = flags;

	extra_colormap->rgba = rgba;
	extra_colormap->fadergba = fadergba;

#ifdef EXTRACOLORMAPLUMPS
	extra_colormap->lump = LUMPERROR;
	extra_colormap->lumpname[0] = 0;
#endif




	extra_colormap->colormap = R_CreateLightTable(extra_colormap);

	R_AddColormapToList(extra_colormap);

	return extra_colormap;
}





extracolormap_t *R_AddColormaps(extracolormap_t *exc_augend, extracolormap_t *exc_addend,
	boolean subR, boolean subG, boolean subB, boolean subA,
	boolean subFadeR, boolean subFadeG, boolean subFadeB, boolean subFadeA,
	boolean subFadeStart, boolean subFadeEnd, boolean ignoreFlags,
	boolean lighttable)
{
	INT16 red, green, blue, alpha;






	exc_augend = R_CopyColormap(exc_augend, false);
	if(!exc_addend)
		exc_addend = R_GetDefaultColormap();





	red = max(min(
		R_GetRgbaR(exc_augend->rgba)
			+ (subR ? -1 : 1)  
			* R_GetRgbaR(exc_addend->rgba)
		, 255), 0);

	green = max(min(
		R_GetRgbaG(exc_augend->rgba)
			+ (subG ? -1 : 1)  
			* R_GetRgbaG(exc_addend->rgba)
		, 255), 0);

	blue = max(min(
		R_GetRgbaB(exc_augend->rgba)
			+ (subB ? -1 : 1)  
			* R_GetRgbaB(exc_addend->rgba)
		, 255), 0);

	alpha = R_GetRgbaA(exc_addend->rgba);
	alpha = max(min(R_GetRgbaA(exc_augend->rgba) + (subA ? -1 : 1) * alpha, 25), 0);

	exc_augend->rgba = R_PutRgbaRGBA(red, green, blue, alpha);





	red = max(min(
		R_GetRgbaR(exc_augend->fadergba)
			+ (subFadeR ? -1 : 1)  
			* R_GetRgbaR(exc_addend->fadergba)
		, 255), 0);

	green = max(min(
		R_GetRgbaG(exc_augend->fadergba)
			+ (subFadeG ? -1 : 1)  
			* R_GetRgbaG(exc_addend->fadergba)
		, 255), 0);

	blue = max(min(
		R_GetRgbaB(exc_augend->fadergba)
			+ (subFadeB ? -1 : 1)  
			* R_GetRgbaB(exc_addend->fadergba)
		, 255), 0);

	alpha = R_GetRgbaA(exc_addend->fadergba);
	if (alpha == 25 && !R_GetRgbaRGB(exc_addend->fadergba))
		alpha = 0;  
	alpha = max(min(R_GetRgbaA(exc_augend->fadergba) + (subFadeA ? -1 : 1) * alpha, 25), 0);

	exc_augend->fadergba = R_PutRgbaRGBA(red, green, blue, alpha);





	exc_augend->fadestart = max(min(
		exc_augend->fadestart
			+ (subFadeStart ? -1 : 1)  
			* exc_addend->fadestart
		, 31), 0);

	exc_augend->fadeend = max(min(
		exc_augend->fadeend
			+ (subFadeEnd ? -1 : 1)  
			* (exc_addend->fadeend == 31 && !exc_addend->fadestart ? 0 : exc_addend->fadeend)

		, 31), 0);

	if (!ignoreFlags)  
		exc_augend->flags = exc_addend->flags;





	exc_augend->colormap = lighttable ? R_CreateLightTable(exc_augend) : NULL;
	exc_augend->next = exc_augend->prev = NULL;
	return exc_augend;
}



UINT8 NearestPaletteColor(UINT8 r, UINT8 g, UINT8 b, RGBA_t *palette)
{
	int dr, dg, db;
	int distortion, bestdistortion = 256 * 256 * 4, bestcolor = 0, i;


	if (palette == NULL)
		palette = pMasterPalette;

	for (i = 0; i < 256; i++)
	{
		dr = r - palette[i].s.red;
		dg = g - palette[i].s.green;
		db = b - palette[i].s.blue;
		distortion = dr*dr + dg*dg + db*db;
		if (distortion < bestdistortion)
		{
			if (!distortion)
				return (UINT8)i;

			bestdistortion = distortion;
			bestcolor = i;
		}
	}

	return (UINT8)bestcolor;
}

#ifdef EXTRACOLORMAPLUMPS
const char *R_NameForColormap(extracolormap_t *extra_colormap)
{
	if (!extra_colormap)
		return "NONE";

	if (extra_colormap->lump == LUMPERROR)
		return "INLEVEL";

	return extra_colormap->lumpname;
}
#endif







void R_InitData(void)
{
	CONS_Printf("R_LoadParsedTranslations()...\n");
	R_LoadParsedTranslations();

	CONS_Printf("R_LoadTextures()...\n");
	R_LoadTextures();

	CONS_Printf("P_InitPicAnims()...\n");
	P_InitPicAnims();

	CONS_Printf("R_InitSprites()...\n");
	R_InitSpriteLumps();
	R_InitSprites();

	CONS_Printf("R_InitColormaps()...\n");
	R_InitColormaps();
}






static void R_PrecacheSpriteLump(lumpnum_t lump)
{
#ifdef HWRENDER
	const precise_t started = I_GetPreciseTime();
	char operation[96];
	patch_t *cachedpatch = W_CachePatchNum(lump, PU_SPRITE);
	if (rendermode == render_opengl && vid.glstate == VID_GL_LIBRARY_LOADED)
	{
		HWR_GetPatch(cachedpatch);
		snprintf(operation, sizeof operation, "OpenGL sprite upload %.8s",
			W_CheckNameForNum(lump));
		I_ReportHitch(operation, started, 10);
	}
#else
	W_CachePatchNum(lump, PU_SPRITE);
#endif
}

static boolean R_PrecacheSpriteLumpBounded(lumpnum_t lump, size_t maxpixels)
{
	INT16 width = 0, height = 0;

	                                                                       
                                                                            
                                                                              
	if (!W_ReadPatchHeader(lump, &width, &height, NULL, NULL)
	|| width <= 0 || height <= 0
	|| (size_t)width * (size_t)height > maxpixels)
		return false;

	R_PrecacheSpriteLump(lump);
	return true;
}

                                                                              
                                                                               
                                                                         
                                                                              
                                                                            
                                                    
static char *levelspritepresent;
static size_t levelspriteprecache_sprite;
static size_t levelspriteprecache_frame;
static size_t levelspriteprecache_angle;
static UINT8 *leveltexturepresent;
static size_t leveltextureprecache_texture;
static INT32 *leveltexturepriority;
static size_t leveltexturepriority_count;
static size_t leveltexturepriority_index;
static UINT8 *levelflatprecachepresent;
static size_t levelflatprecache_flat;
static INT32 *levelflatpriority;
static size_t levelflatpriority_count;
static size_t levelflatpriority_index;
static UINT8 levelprecachecooldown;

#define LEVELTEXTURE_OPAQUE 1
#define LEVELTEXTURE_MASKED 2
#define LEVELGRAPHICS_LOCAL_PRIORITY_MAX 48
                                                                             
                                                                               
                                                                                 
#define LEVELTEXTURE_BACKGROUND_MAX_PIXELS (160u * 1024u)
                                                                              
                                                                           
                                                                              
#define LEVELGRAPHICS_INITIAL_MAX_PIXELS (64u * 1024u * 1024u)
#define LEVELGRAPHICS_INITIAL_MAX_SINGLE_PIXELS (8u * 1024u * 1024u)
#define LEVELGRAPHICS_INITIAL_MAX_MILLISECONDS 900u

typedef struct
{
	INT32 graphic;
	UINT32 references;
} levelgraphicpriority_t;

static int R_CompareLevelGraphicPriority(const void *left, const void *right)
{
	const levelgraphicpriority_t *a = left;
	const levelgraphicpriority_t *b = right;

	if (a->references < b->references)
		return 1;
	if (a->references > b->references)
		return -1;
	return a->graphic - b->graphic;
}

static void R_BuildLevelGraphicPriorities(void)
{
	sector_t **queue = NULL;
	UINT8 *sectorseen = NULL, *textureseen = NULL, *flatseen = NULL, *depth = NULL;
	UINT32 *textureusage = NULL, *flatusage = NULL;
	levelgraphicpriority_t *texturecandidates = NULL, *flatcandidates = NULL;
	sector_t *start;
	size_t head = 0, tail = 0, i, candidatecount;

	free(leveltexturepriority);
	leveltexturepriority = NULL;
	leveltexturepriority_count = leveltexturepriority_index = 0;
	free(levelflatpriority);
	levelflatpriority = NULL;
	levelflatpriority_count = levelflatpriority_index = 0;

	if ((!leveltexturepresent && !levelflatprecachepresent) || !numsectors
	|| consoleplayer < 0 || consoleplayer >= MAXPLAYERS
	|| !players[consoleplayer].mo || !players[consoleplayer].mo->subsector)
		return;

	start = players[consoleplayer].mo->subsector->sector;
	if (!start)
		return;

	queue = malloc((size_t)numsectors * sizeof (*queue));
	depth = malloc((size_t)numsectors * sizeof (*depth));
	sectorseen = calloc((size_t)numsectors, sizeof (*sectorseen));
	if (numtextures)
	{
		textureseen = calloc((size_t)numtextures, sizeof (*textureseen));
		textureusage = calloc((size_t)numtextures, sizeof (*textureusage));
		texturecandidates = malloc((size_t)numtextures * sizeof (*texturecandidates));
		leveltexturepriority = malloc((size_t)numtextures
			* sizeof (*leveltexturepriority));
	}
	if (numlevelflats)
	{
		flatseen = calloc(numlevelflats, sizeof (*flatseen));
		flatusage = calloc(numlevelflats, sizeof (*flatusage));
		flatcandidates = malloc(numlevelflats * sizeof (*flatcandidates));
		levelflatpriority = malloc(numlevelflats * sizeof (*levelflatpriority));
	}
	if (!queue || !depth || !sectorseen
	|| (numtextures && (!textureseen || !textureusage || !texturecandidates
		|| !leveltexturepriority))
	|| (numlevelflats && (!flatseen || !flatusage || !flatcandidates
		|| !levelflatpriority)))
		goto done;

	                                                                          
                                                                           
                                                                              
	if (leveltexturepresent && skytexture >= 0 && skytexture < numtextures
	&& leveltexturepresent[skytexture])
	{
		textureseen[skytexture] = 1;
		leveltexturepriority[leveltexturepriority_count++] = skytexture;
	}
	sectorseen[start - sectors] = 1;
	queue[tail] = start;
	depth[tail++] = 0;
	while (head < tail
	&& (leveltexturepriority_count < LEVELGRAPHICS_LOCAL_PRIORITY_MAX
		|| levelflatpriority_count < LEVELGRAPHICS_LOCAL_PRIORITY_MAX))
	{
		sector_t *sector = queue[head];
		const UINT8 sectordepth = depth[head++];
		INT32 localflats[2];

		localflats[0] = sector->floorpic;
		localflats[1] = sector->ceilingpic;
		for (i = 0; i < 2; i++)
		{
			const INT32 flat = localflats[i];
			if (levelflatprecachepresent && flat >= 0
			&& (size_t)flat < numlevelflats && levelflatprecachepresent[flat]
			&& !flatseen[flat]
			&& levelflatpriority_count < LEVELGRAPHICS_LOCAL_PRIORITY_MAX)
			{
				flatseen[flat] = 1;
				levelflatpriority[levelflatpriority_count++] = flat;
			}
		}

		for (i = 0; i < sector->linecount; i++)
		{
			line_t *line = sector->lines[i];
			UINT8 sideindex;

			for (sideindex = 0; sideindex < 2; sideindex++)
			{
				side_t *side;
				INT32 candidates[3];
				UINT8 candidate;

				if (line->sidenum[sideindex] == NO_SIDEDEF)
					continue;
				side = &sides[line->sidenum[sideindex]];
				candidates[0] = side->toptexture;
				candidates[1] = side->midtexture;
				candidates[2] = side->bottomtexture;
				for (candidate = 0; candidate < 3; candidate++)
				{
					const INT32 texture = candidates[candidate];
					if (leveltexturepresent && texture >= 0 && texture < numtextures
					&& leveltexturepresent[texture] && !textureseen[texture]
					&& leveltexturepriority_count < LEVELGRAPHICS_LOCAL_PRIORITY_MAX)
					{
						textureseen[texture] = 1;
						leveltexturepriority[leveltexturepriority_count++] = texture;
					}
				}
			}

			if (sectordepth < 2)
			{
				sector_t *next = line->frontsector == sector
					? line->backsector : line->frontsector;
				if (next && next >= sectors && next < sectors + numsectors
				&& !sectorseen[next - sectors] && tail < (size_t)numsectors)
				{
					sectorseen[next - sectors] = 1;
					queue[tail] = next;
					depth[tail++] = sectordepth + 1;
				}
			}
		}
	}

	                                                                           
                                                                           
                                                        
	for (i = 0; i < numsides; i++)
	{
		const INT32 candidates[3] = {sides[i].toptexture,
			sides[i].midtexture, sides[i].bottomtexture};
		UINT8 candidate;
		for (candidate = 0; candidate < 3; candidate++)
		{
			const INT32 texture = candidates[candidate];
			if (leveltexturepresent && texture >= 0 && texture < numtextures
			&& leveltexturepresent[texture])
				textureusage[texture]++;
		}
	}
	candidatecount = 0;
	for (i = 0; leveltexturepresent && i < (size_t)numtextures; i++)
		if (leveltexturepresent[i] && textureusage[i])
		{
			texturecandidates[candidatecount].graphic = (INT32)i;
			texturecandidates[candidatecount++].references = textureusage[i];
		}
	qsort(texturecandidates, candidatecount, sizeof (*texturecandidates),
		R_CompareLevelGraphicPriority);
	for (i = 0; i < candidatecount; i++)
	{
		const INT32 texture = texturecandidates[i].graphic;
		if (!textureseen[texture])
		{
			textureseen[texture] = 1;
			leveltexturepriority[leveltexturepriority_count++] = texture;
		}
	}
	                                                                         
                                                                     
	for (i = 0; leveltexturepresent && i < (size_t)numtextures; i++)
		if (leveltexturepresent[i] && !textureseen[i])
			leveltexturepriority[leveltexturepriority_count++] = (INT32)i;

	for (i = 0; i < (size_t)numsectors; i++)
	{
		const INT32 candidates[2] = {sectors[i].floorpic, sectors[i].ceilingpic};
		UINT8 candidate;
		for (candidate = 0; candidate < 2; candidate++)
		{
			const INT32 flat = candidates[candidate];
			if (levelflatprecachepresent && flat >= 0
			&& (size_t)flat < numlevelflats && levelflatprecachepresent[flat])
				flatusage[flat]++;
		}
	}
	candidatecount = 0;
	for (i = 0; levelflatprecachepresent && i < numlevelflats; i++)
		if (levelflatprecachepresent[i] && flatusage[i])
		{
			flatcandidates[candidatecount].graphic = (INT32)i;
			flatcandidates[candidatecount++].references = flatusage[i];
		}
	qsort(flatcandidates, candidatecount, sizeof (*flatcandidates),
		R_CompareLevelGraphicPriority);
	for (i = 0; i < candidatecount; i++)
	{
		const INT32 flat = flatcandidates[i].graphic;
		if (!flatseen[flat])
		{
			flatseen[flat] = 1;
			levelflatpriority[levelflatpriority_count++] = flat;
		}
	}
	for (i = 0; levelflatprecachepresent && i < numlevelflats; i++)
		if (levelflatprecachepresent[i] && !flatseen[i])
			levelflatpriority[levelflatpriority_count++] = (INT32)i;

done:
	free(queue);
	free(depth);
	free(sectorseen);
	free(textureseen);
	free(flatseen);
	free(textureusage);
	free(flatusage);
	free(texturecandidates);
	free(flatcandidates);
	if (!leveltexturepriority_count)
	{
		free(leveltexturepriority);
		leveltexturepriority = NULL;
	}
	if (!levelflatpriority_count)
	{
		free(levelflatpriority);
		levelflatpriority = NULL;
	}
}

#ifdef HWRENDER
static boolean R_PrewarmLevelTextureBounded(INT32 texture, size_t maxpixels,
	size_t *remaining, boolean retire_oversize)
{
	UINT8 variants;
	INT32 translated;
	texture_t *definition;
	size_t pixels;

	if (!leveltexturepresent || texture < 0 || texture >= numtextures)
		return false;
	variants = leveltexturepresent[texture];
	if (!variants)
		return false;

	translated = texturetranslation[texture];
	if (translated < 0 || translated >= numtextures
	|| !(definition = textures[translated])
	|| definition->width <= 0 || definition->height <= 0)
	{
		leveltexturepresent[texture] = 0;
		return false;
	}

	pixels = (size_t)definition->width * (size_t)definition->height;
	if (pixels > maxpixels || (remaining && pixels > *remaining))
	{
		                                                                         
                                                                            
                                                         
		if (retire_oversize)
			leveltexturepresent[texture] = 0;
		return false;
	}

	if (variants & LEVELTEXTURE_OPAQUE)
	{
		HWR_GetTexture(translated, false);
		leveltexturepresent[texture] &= ~LEVELTEXTURE_OPAQUE;
	}
	else
	{
		HWR_GetTexture(translated, true);
		leveltexturepresent[texture] &= ~LEVELTEXTURE_MASKED;
	}
	if (remaining)
		*remaining -= pixels;
	return true;
}

static boolean R_PrewarmLevelTexture(INT32 texture)
{
	return R_PrewarmLevelTextureBounded(texture,
		LEVELTEXTURE_BACKGROUND_MAX_PIXELS, NULL, true);
}

static boolean R_PrewarmLevelFlatBounded(size_t flat, size_t maxpixels,
	size_t *remaining, boolean retire_oversize)
{
	levelflat_t *levelflat;
	texture_t *definition;
	INT32 translated;
	size_t pixels;
	UINT8 variants;

	if (!levelflatprecachepresent || flat >= numlevelflats)
		return false;
	variants = levelflatprecachepresent[flat];
	if (!variants)
		return false;

	levelflat = &levelflats[flat];
	if (levelflat->type == LEVELFLAT_NONE || levelflat->texture_id < 0
	|| levelflat->texture_id >= numtextures)
	{
		levelflatprecachepresent[flat] = 0;
		return false;
	}

	translated = texturetranslation[levelflat->texture_id];
	if (translated < 0 || translated >= numtextures
	|| !(definition = textures[translated])
	|| definition->width <= 0 || definition->height <= 0)
	{
		levelflatprecachepresent[flat] = 0;
		return false;
	}

	pixels = (size_t)definition->width * (size_t)definition->height;
	if (pixels > maxpixels || (remaining && pixels > *remaining))
	{
		if (retire_oversize)
			levelflatprecachepresent[flat] = 0;
		return false;
	}

	if (variants & LEVELTEXTURE_OPAQUE)
	{
		HWR_GetLevelFlat(levelflat, false);
		levelflatprecachepresent[flat] &= ~LEVELTEXTURE_OPAQUE;
	}
	else
	{
		HWR_GetLevelFlat(levelflat, true);
		levelflatprecachepresent[flat] &= ~LEVELTEXTURE_MASKED;
	}
	if (remaining)
		*remaining -= pixels;
	return true;
}

static boolean R_PrewarmLevelFlat(size_t flat)
{
	return R_PrewarmLevelFlatBounded(flat,
		LEVELTEXTURE_BACKGROUND_MAX_PIXELS, NULL, true);
}

static void R_PrewarmInitialLevelGraphics(void)
{
	size_t remaining = LEVELGRAPHICS_INITIAL_MAX_PIXELS;
	size_t texturepriority = 0, flatpriority = 0;
	size_t texture = 0, flat = 0;
	const precise_t started = I_GetPreciseTime();
	const precise_t deadline = started
		+ (I_GetPrecisePrecision() * LEVELGRAPHICS_INITIAL_MAX_MILLISECONDS) / 1000u;

#define INITIAL_PREWARM_TIME_LEFT() (I_GetPreciseTime() < deadline)

	                                                                              
                                                                               
                                  
	while ((texturepriority < leveltexturepriority_count
		|| flatpriority < levelflatpriority_count)
	&& remaining && INITIAL_PREWARM_TIME_LEFT())
	{
		if (leveltexturepriority && texturepriority < leveltexturepriority_count)
		{
			const INT32 prioritytexture = leveltexturepriority[texturepriority++];
			while (leveltexturepresent[prioritytexture] && remaining
			&& INITIAL_PREWARM_TIME_LEFT())
				if (!R_PrewarmLevelTextureBounded(prioritytexture,
					LEVELGRAPHICS_INITIAL_MAX_SINGLE_PIXELS, &remaining, false))
					break;
		}
		if (levelflatpriority && flatpriority < levelflatpriority_count
		&& remaining && INITIAL_PREWARM_TIME_LEFT())
		{
			const INT32 priorityflat = levelflatpriority[flatpriority++];
			while (levelflatprecachepresent[priorityflat] && remaining
			&& INITIAL_PREWARM_TIME_LEFT())
				if (!R_PrewarmLevelFlatBounded((size_t)priorityflat,
					LEVELGRAPHICS_INITIAL_MAX_SINGLE_PIXELS, &remaining, false))
					break;
		}
	}

	                                                                               
                                                              
	while ((leveltexturepresent && texture < (size_t)numtextures)
		|| (levelflatprecachepresent && flat < numlevelflats))
	{
		if (!remaining || !INITIAL_PREWARM_TIME_LEFT())
			break;
		if (leveltexturepresent && texture < (size_t)numtextures)
		{
			while (leveltexturepresent[texture] && remaining
			&& INITIAL_PREWARM_TIME_LEFT())
				if (!R_PrewarmLevelTextureBounded((INT32)texture,
					LEVELGRAPHICS_INITIAL_MAX_SINGLE_PIXELS, &remaining, false))
					break;
			texture++;
		}
		if (levelflatprecachepresent && flat < numlevelflats
		&& remaining && INITIAL_PREWARM_TIME_LEFT())
		{
			while (levelflatprecachepresent[flat] && remaining
			&& INITIAL_PREWARM_TIME_LEFT())
				if (!R_PrewarmLevelFlatBounded(flat,
					LEVELGRAPHICS_INITIAL_MAX_SINGLE_PIXELS, &remaining, false))
					break;
			flat++;
		}
	}

#undef INITIAL_PREWARM_TIME_LEFT
}
#endif


void R_ContinueLevelSpritePrecache(void)
{
	const precise_t started = I_GetPreciseTime();
	const precise_t budget = (precise_t)(I_GetPrecisePrecision() / 1000);
	unsigned int warmed = 0;
	boolean hardware = false;

	if (!levelspritepresent && !leveltexturepresent && !levelflatprecachepresent)
		return;
	                                                                         
                                                                            
                                                                         
	if (levelprecachecooldown)
	{
		levelprecachecooldown--;
		return;
	}
	if (rendermode == render_soft)
		hardware = false;
#ifdef HWRENDER
	else if (rendermode == render_opengl && vid.glstate == VID_GL_LIBRARY_LOADED)
		hardware = true;
#endif
	else
	{
		free(levelspritepresent);
		levelspritepresent = NULL;
		free(leveltexturepresent);
		leveltexturepresent = NULL;
		free(levelflatprecachepresent);
		levelflatprecachepresent = NULL;
		free(leveltexturepriority);
		leveltexturepriority = NULL;
		leveltexturepriority_count = leveltexturepriority_index = 0;
		free(levelflatpriority);
		levelflatpriority = NULL;
		levelflatpriority_count = levelflatpriority_index = 0;
		return;
	}

#ifdef HWRENDER
	while (hardware && levelflatpriority
	&& levelflatpriority_index < levelflatpriority_count && warmed < 1)
	{
		const INT32 flat = levelflatpriority[levelflatpriority_index];
		if (R_PrewarmLevelFlat((size_t)flat))
			warmed++;
		if (!levelflatprecachepresent[flat])
			levelflatpriority_index++;
	}
	if (levelflatpriority && levelflatpriority_index >= levelflatpriority_count)
	{
		free(levelflatpriority);
		levelflatpriority = NULL;
		levelflatpriority_count = levelflatpriority_index = 0;
	}

	                                                                            
                                             
	while (hardware && levelflatprecachepresent
	&& levelflatprecache_flat < numlevelflats && warmed < 1)
	{
		if (!levelflatprecachepresent[levelflatprecache_flat])
		{
			levelflatprecache_flat++;
			continue;
		}

		if (R_PrewarmLevelFlat(levelflatprecache_flat))
			warmed++;

		if (!levelflatprecachepresent[levelflatprecache_flat])
			levelflatprecache_flat++;
	}
	if (levelflatprecachepresent && levelflatprecache_flat >= numlevelflats)
	{
		free(levelflatprecachepresent);
		levelflatprecachepresent = NULL;
	}

	while (hardware && leveltexturepriority
	&& leveltexturepriority_index < leveltexturepriority_count && warmed < 1)
	{
		const INT32 texture = leveltexturepriority[leveltexturepriority_index];
		if (R_PrewarmLevelTexture(texture))
			warmed++;
		if (!leveltexturepresent[texture])
			leveltexturepriority_index++;
	}
	if (leveltexturepriority
	&& leveltexturepriority_index >= leveltexturepriority_count)
	{
		free(leveltexturepriority);
		leveltexturepriority = NULL;
		leveltexturepriority_count = leveltexturepriority_index = 0;
	}

	                                                                            
                                                                           
                                                                            
                                                                           
	while (hardware && leveltexturepresent
	&& leveltextureprecache_texture < (size_t)numtextures && warmed < 1)
	{
		UINT8 variants = leveltexturepresent[leveltextureprecache_texture];

		if (!variants)
		{
			leveltextureprecache_texture++;
			continue;
		}

		if (R_PrewarmLevelTexture((INT32)leveltextureprecache_texture))
			warmed++;

		if (!leveltexturepresent[leveltextureprecache_texture])
			leveltextureprecache_texture++;
	}

	if (leveltexturepresent && leveltextureprecache_texture >= (size_t)numtextures)
	{
		free(leveltexturepresent);
		leveltexturepresent = NULL;
	}
#endif

	                                                                            
                                                                              
                                                                               
	while (levelspritepresent && levelspriteprecache_sprite < numsprites
	&& (!hardware || warmed < 1))
	{
		spriteframe_t *sf;
		size_t angles;

		if (!levelspritepresent[levelspriteprecache_sprite]
		|| levelspriteprecache_frame >= sprites[levelspriteprecache_sprite].numframes)
		{
			levelspriteprecache_sprite++;
			levelspriteprecache_frame = 0;
			levelspriteprecache_angle = 0;
			continue;
		}

		sf = &sprites[levelspriteprecache_sprite].spriteframes[levelspriteprecache_frame];
		if (sf->rotate == SRF_SINGLE)
			angles = 1;
		else if (sf->rotate == SRF_2D)
			angles = 2;
		else
			angles = (sf->rotate & SRF_3DGE) ? 16 : 8;

		if (levelspriteprecache_angle < angles)
		{
			size_t angle = levelspriteprecache_angle++;
			if (sf->rotate == SRF_2D)
				angle = angle ? 6 : 2;
			if (R_PrecacheSpriteLumpBounded(sf->lumppat[angle],
				LEVELTEXTURE_BACKGROUND_MAX_PIXELS))
				warmed++;
		}

		if (levelspriteprecache_angle >= angles)
		{
			levelspriteprecache_frame++;
			levelspriteprecache_angle = 0;
		}

		if (I_GetPreciseTime() - started >= budget)
			break;
	}

	if (levelspriteprecache_sprite >= numsprites)
	{
		free(levelspritepresent);
		levelspritepresent = NULL;
	}

	if (warmed)
	{
		const precise_t elapsed = I_GetPreciseTime() - started;
		const precise_t precision = I_GetPrecisePrecision();
		if (elapsed >= precision / 125)  
			levelprecachecooldown = 30;
		else if (elapsed >= precision / 250)  
			levelprecachecooldown = 8;
		else if (elapsed >= precision / 500)  
			levelprecachecooldown = 2;
	}
}

void R_PrecacheLevel(void)
{
	UINT8 *texturepresent;
	char *spritepresent;
	size_t i, j, k;
	lumpnum_t lump;

	thinker_t *th;
	spriteframe_t *sf;

	free(levelspritepresent);
	levelspritepresent = NULL;
	free(leveltexturepresent);
	leveltexturepresent = NULL;
	free(levelflatprecachepresent);
	levelflatprecachepresent = NULL;
	free(leveltexturepriority);
	leveltexturepriority = NULL;
	free(levelflatpriority);
	levelflatpriority = NULL;
	levelspriteprecache_sprite = 0;
	levelspriteprecache_frame = 0;
	levelspriteprecache_angle = 0;
	leveltextureprecache_texture = 0;
	levelflatprecache_flat = 0;
	leveltexturepriority_count = leveltexturepriority_index = 0;
	levelflatpriority_count = levelflatpriority_index = 0;
	levelprecachecooldown = 0;

	                                                                            
                                                                            
                                                                           
                                                                             
                         
	if (rendermode == render_none)
		return;

	if (demoplayback)
		return;

	if (rendermode == render_soft
#ifdef HWRENDER
	|| (rendermode == render_opengl && vid.glstate == VID_GL_LIBRARY_LOADED)
#endif
	)
	{

		if (rendermode == render_soft)
			flatmemory = P_PrecacheLevelFlats();






	texturepresent = calloc(numtextures, sizeof (*texturepresent));
	if (texturepresent == NULL) I_Error("%s: Out of memory looking up textures", "R_PrecacheLevel");

	for (j = 0; j < numsides; j++)
	{
		line_t *line = sides[j].line;


		if (sides[j].toptexture >= 0 && sides[j].toptexture < numtextures)
			texturepresent[sides[j].toptexture] |= LEVELTEXTURE_OPAQUE;
		if (sides[j].midtexture >= 0 && sides[j].midtexture < numtextures)
		{
			                                                                     
                                                                          
                                                                            
                                                                   
			if (line && line->backsector)
				texturepresent[sides[j].midtexture] |= LEVELTEXTURE_MASKED;
			else
				texturepresent[sides[j].midtexture] |= LEVELTEXTURE_OPAQUE;
		}
		if (sides[j].bottomtexture >= 0 && sides[j].bottomtexture < numtextures)
			texturepresent[sides[j].bottomtexture] |= LEVELTEXTURE_OPAQUE;
	}

	                                                                         
                                                                            
                                                                            
                                                                           
                            
	for (j = 0; j < (size_t)numsectors; j++)
	{
		ffloor_t *rover;
		for (rover = sectors[j].ffloors; rover; rover = rover->next)
		{
			line_t *master = rover->master;
			if (!master || !(rover->fofflags & FOF_EXISTS)
			|| !(rover->fofflags & FOF_RENDERSIDES)
			|| (rover->fofflags & FOF_FOG))
				continue;

			if (master->flags & ML_TFERLINE)
			{
				sector_t *control = master->frontsector;
				size_t control_line;
				if (!control)
					continue;
				for (control_line = 0; control_line < control->linecount;
					control_line++)
				{
					line_t *source = control->lines[control_line];
					size_t sideindex;
					INT32 texture;
					if (!source || (sideindex = source->sidenum[0]) == NO_SIDEDEF
					|| sideindex >= numsides)
						continue;
					texture = sides[sideindex].midtexture;
					if (texture >= 0 && texture < numtextures)
						texturepresent[texture] |= LEVELTEXTURE_MASKED;
				}
			}
			else if (master->sidenum[0] != NO_SIDEDEF
			&& master->sidenum[0] < numsides)
			{
				const INT32 texture = sides[master->sidenum[0]].midtexture;
				if (texture >= 0 && texture < numtextures)
					texturepresent[texture] |= LEVELTEXTURE_MASKED;
			}
		}
	}

	                                                                         
                                                                            
                                                                     
	P_ExpandTextureAnimationPresence(texturepresent, (size_t)numtextures);




	texturepresent[skytexture] |= LEVELTEXTURE_OPAQUE;

	if (rendermode == render_soft)
	{
		texturememory = 0;
		for (j = 0; j < (unsigned)numtextures; j++)
		{
			if (!texturepresent[j])
				continue;

			if (!texturecache[j])
				R_GenerateTexture(j);


		}
		free(texturepresent);
	}
#ifdef HWRENDER
	else
	{
		precise_t preload_started;
		size_t flat;
		size_t sector;

		leveltexturepresent = texturepresent;

		levelflatprecachepresent = calloc(numlevelflats,
			sizeof (*levelflatprecachepresent));
		if (numlevelflats && !levelflatprecachepresent)
			I_Error("%s: Out of memory looking up level flats", "R_PrecacheLevel");
		for (flat = 0; flat < numlevelflats; flat++)
			if (levelflats[flat].type != LEVELFLAT_NONE)
				levelflatprecachepresent[flat] |= LEVELTEXTURE_OPAQUE;

		                                                                         
                                                                            
                                       
		for (sector = 0; sector < (size_t)numsectors; sector++)
		{
			ffloor_t *rover;
			for (rover = sectors[sector].ffloors; rover; rover = rover->next)
			{
				if (!(rover->fofflags & FOF_SPLAT))
					continue;
				if (rover->toppic && *rover->toppic >= 0
				&& (size_t)*rover->toppic < numlevelflats)
					levelflatprecachepresent[*rover->toppic] |= LEVELTEXTURE_MASKED;
				if (rover->bottompic && *rover->bottompic >= 0
				&& (size_t)*rover->bottompic < numlevelflats)
					levelflatprecachepresent[*rover->bottompic] |= LEVELTEXTURE_MASKED;
			}
		}
		R_BuildLevelGraphicPriorities();

		preload_started = I_GetPreciseTime();
		R_PrewarmInitialLevelGraphics();
		I_ReportHitch("initial OpenGL level graphics preload",
			preload_started, 25);
	}
#endif
	}




	spritepresent = calloc(numsprites, sizeof (*spritepresent));
	if (spritepresent == NULL) I_Error("%s: Out of memory looking up sprites", "R_PrecacheLevel");

	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		if (th->function.acp1 != (actionf_p1)P_RemoveThinkerDelayed)
			spritepresent[((mobj_t *)th)->sprite] = 1;

	if (rendermode == render_soft
#ifdef HWRENDER
	|| (rendermode == render_opengl && vid.glstate == VID_GL_LIBRARY_LOADED)
#endif
	)
	{
		                                                                          
                                                                             
                                                                        
		levelspritepresent = spritepresent;
		return;
	}

	spritememory = 0;
	for (i = 0; i < numsprites; i++)
	{
		if (!spritepresent[i])
			continue;

		for (j = 0; j < sprites[i].numframes; j++)
		{
			sf = &sprites[i].spriteframes[j];
#define cacheang(a) {\
		lump = sf->lumppat[a];\
		if (devparm)\
			spritememory += W_LumpLength(lump);\
		R_PrecacheSpriteLump(lump);\
	}

			switch (sf->rotate)
			{
				case SRF_SINGLE:
					cacheang(0);
					break;
				case SRF_2D:
					cacheang(2);
					cacheang(6);
					break;
				default:
					k = (sf->rotate & SRF_3DGE ? 16 : 8);
					while (k--)
						cacheang(k);
					break;
			}
#undef cacheang
		}
	}
	free(spritepresent);

	CONS_Debug(DBG_SETUP, "Precache level done:\n"
			"flatmemory:    %s k\n"
			"texturememory: %s k\n"
			"spritememory:  %s k\n", sizeu1(flatmemory>>10), sizeu2(texturememory>>10), sizeu3(spritememory>>10));
}
