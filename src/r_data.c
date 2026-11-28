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
/// \file  r_data.c
/// \brief Preparation of data for rendering, generation of lookups, caching, retrieval by name

#include "doomdef.h"

#if defined(__SSE2__) || defined(_M_X64)
#include <emmintrin.h>
#endif
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
#include "p_setup.h" // levelflats
#include "p_spec.h"
#include "v_video.h" // pMasterPalette
#include "f_finale.h" // wipes
#include "byteptr.h"
#include "dehacked.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h" // HWR_ClearLightTables
#endif

//
// Graphics.
// SRB2 graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
//

size_t numspritelumps, max_spritelumps;

// needed for pre rendering
sprcache_t *spritecachedinfo;

lighttable_t *colormaps;
lighttable_t *fadecolormap;

// for debugging/info purposes
size_t flatmemory, spritememory, texturememory;

// Blends two pixels together, using the equation
// that matches the specified alpha style.
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
			// don't go too high
			if (fullalpha >= 0xFF)
				fullalpha = 0xFF;
			alpha = (UINT8)fullalpha;

			// if the background pixel is empty,
			// match software and don't blend anything
			if (!background.s.alpha)
			{
				// ...unless the foreground pixel ISN'T actually translucent.
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
		// just copy the pixel
		else if (style == AST_COPY)
			output.rgba = foreground.rgba;

		output.s.alpha = 0xFF;
		return output.rgba;
	}
#undef clamp
	return 0;
}

INT32 ASTTextureBlendingThreshold[2] = {255/11, (10*255/11)};

// Blends a pixel for a texture patch.
UINT32 ASTBlendTexturePixel(RGBA_t background, RGBA_t foreground, int style, UINT8 alpha)
{
	// Alpha style set to translucent?
	if (style == AST_TRANSLUCENT)
	{
		// Is the alpha small enough for translucency?
		if (alpha <= ASTTextureBlendingThreshold[1])
		{
			// Is the patch way too translucent? Don't blend then.
			if (alpha < ASTTextureBlendingThreshold[0])
				return background.rgba;

			return ASTBlendPixel(background, foreground, style, alpha);
		}
		else // just copy the pixel
			return foreground.rgba;
	}
	else
		return ASTBlendPixel(background, foreground, style, alpha);
}

// Blends two palette indexes for a texture patch, then
// finds the nearest palette index from the blended output.
UINT8 ASTBlendPaletteIndexes(UINT8 background, UINT8 foreground, int style, UINT8 alpha)
{
	// Alpha style set to translucent?
	if (style == AST_TRANSLUCENT)
	{
		// Is the alpha small enough for translucency?
		if (alpha <= ASTTextureBlendingThreshold[1])
		{
			UINT8 *mytransmap;
			INT32 trans;

			// Is the patch way too translucent? Don't blend then.
			if (alpha < ASTTextureBlendingThreshold[0])
				return background;

			// The equation's not exact but it works as intended. I'll call it a day for now.
			trans = (8*(alpha) + 255/8)/(255 - 255/11);
			mytransmap = R_GetTranslucencyTable(trans + 1);
			if (background != 0xFF)
				return *(mytransmap + (background<<8) + foreground);
		}
		else // just copy the pixel
			return foreground;
	}
	// just copy the pixel
	else if (style == AST_COPY)
		return foreground;
	// use ASTBlendPixel for all other blend modes
	// and find the nearest colour in the palette
	else if (style != AST_TRANSLUCENT)
	{
		RGBA_t texel;
		RGBA_t bg = V_GetMasterColor(background);
		RGBA_t fg = V_GetMasterColor(foreground);
		texel.rgba = ASTBlendPixel(bg, fg, style, alpha);
		return NearestColor(texel.s.red, texel.s.green, texel.s.blue);
	}
	// fallback if all above fails, somehow
	// return the background pixel
	return background;
}

#ifdef EXTRACOLORMAPLUMPS
static lumplist_t *colormaplumps = NULL; ///\todo free leak
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

		// This shouldn't be possible when you use the Pwad function, silly
		//if (WADFILENUM(startnum) != WADFILENUM(endnum))
			//I_Error("R_InitExtraColormaps: C_START and C_END in different wad files!\n");

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

//
// R_InitSpriteLumps
// Finds the width and hoffset of all sprites in the wad, so the sprite does not need to be
// cached completely, just for having the header info ready during rendering.
//

//
// allocate sprite lookup tables
//
static void R_InitSpriteLumps(void)
{
	numspritelumps = 0;
	max_spritelumps = 8192;

	Z_Malloc(max_spritelumps*sizeof(*spritecachedinfo), PU_STATIC, &spritecachedinfo);
}

//
// R_CreateFadeColormaps
//

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

	// Load in the light tables, now 64k aligned for smokie...
	{
		lumpnum_t lump = W_CheckNumForName("FADECMAP");
		lumpnum_t wlump = W_CheckNumForName("FADEWMAP");

		// to black
		if (lump != LUMPERROR)
			W_ReadLumpHeader(lump, fadecolormap, len, 0U);
		// to white
		if (wlump != LUMPERROR)
			W_ReadLumpHeader(wlump, fadecolormap+len, len, 0U);

		// missing "to white" colormap lump
		if (lump != LUMPERROR && wlump == LUMPERROR)
			goto makewhite;
		// missing "to black" colormap lump
		else if (lump == LUMPERROR && wlump != LUMPERROR)
			goto makeblack;
		// both lumps found
		else if (lump != LUMPERROR && wlump != LUMPERROR)
			return;
	}

#define GETCOLOR \
	px = colormaps[i%256]; \
	fade = (i/256) * (256 / FADECOLORMAPROWS); \
	rgba = V_GetMasterColor(px);

	// to black
	makeblack:
	for (i = 0; i < len; i++)
	{
		// find pixel and fade amount
		GETCOLOR;

		// subtractive color blending
		r = rgba.s.red - FADEREDFACTOR*fade/10;
		g = rgba.s.green - FADEGREENFACTOR*fade/10;
		b = rgba.s.blue - FADEBLUEFACTOR*fade/10;

		// clamp values
		if (r < 0) r = 0;
		if (g < 0) g = 0;
		if (b < 0) b = 0;

		// find nearest color in palette
		fadecolormap[i] = NearestColor(r,g,b);
	}

	// to white
	makewhite:
	for (i = len; i < len*2; i++)
	{
		// find pixel and fade amount
		GETCOLOR;

		// additive color blending
		r = rgba.s.red + FADEREDFACTOR*fade/10;
		g = rgba.s.green + FADEGREENFACTOR*fade/10;
		b = rgba.s.blue + FADEBLUEFACTOR*fade/10;

		// clamp values
		if (r > 255) r = 255;
		if (g > 255) g = 255;
		if (b > 255) b = 255;

		// find nearest color in palette
		fadecolormap[i] = NearestColor(r,g,b);
	}
#undef GETCOLOR
}

//
// R_InitColormaps
//
static void R_InitColormaps(void)
{
	size_t len;
	lumpnum_t lump;

	// Load in the light tables
	lump = W_GetNumForName("COLORMAP");
	len = W_LumpLength(lump);
	colormaps = Z_MallocAlign(len, PU_STATIC, NULL, 8);
	W_ReadLump(lump, colormaps);

	// Make colormap for fades
	R_CreateFadeColormaps();

	// Init Boom colormaps.
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

	// Load in the light tables, now 64k aligned for smokie...
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

	// Init Boom colormaps.
	R_ClearColormaps();
}

//
// R_ClearColormaps
//
// Clears out extra colormaps between levels.
//
void R_ClearColormaps(void)
{
	// Purged by PU_LEVEL, just overwrite the pointer
	extra_colormaps = R_CreateDefaultColormap(true);
}

//
// R_CreateDefaultColormap()
// NOTE: The result colormap is not added to the extra_colormaps chain. You must do that yourself!
//
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

//
// R_GetDefaultColormap()
//
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

//
// R_CopyColormap()
// NOTE: The result colormap is not added to the extra_colormaps chain. You must do that yourself!
//
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
		// aligned on 8 bit for asm code
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

//
// R_AddColormapToList
//
// Sets prev/next chain for extra_colormaps var
// Copypasta from P_AddFFloorToList
//
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

//
// R_CheckDefaultColormapByValues()
//
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
	// Treat NULL as default colormap
	// We need this because what if one exc is a default colormap, and the other is NULL? They're really both equal.
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

//
// R_GetColormapFromListByValues()
// NOTE: Returns NULL if no match is found
//
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

	// aligned on 8 bit for asm code
	exc->colormap = Z_MallocAlign(W_LumpLength(lump), PU_LEVEL, NULL, 16);
	W_ReadLump(lump, exc->colormap);

	// We set all params of the colormap to normal because there
	// is no real way to tell how GL should handle a colormap lump anyway..
	exc->fadestart = 0;
	exc->fadeend = 31;
	exc->flags = 0;
	exc->rgba = 0;
	exc->fadergba = 0xFF000000;

	R_AddColormapToList(exc);

	return exc;
}
#endif

//
// R_CreateColormapFromLinedef
//
// This is a more GL friendly way of doing colormaps: Specify colormap
// data in a special linedef's texture areas and use that to generate
// custom colormaps at runtime. NOTE: For GL mode, we only need to color
// data and not the colormap data.
//
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
//		cfa = R_GetRgbaA(extra_colormap->fadergba); // unused in software

	UINT8 fadestart = extra_colormap->fadestart,
		fadedist = extra_colormap->fadeend - extra_colormap->fadestart;

	size_t i;

	/////////////////////
	// Calc the RGBA mask
	/////////////////////
	cmaskr = cr;
	cmaskg = cg;
	cmaskb = cb;

	maskamt = (double)(ca/255.0l);
	othermask = 1 - maskamt;
	maskamt /= 0xff;

	cmaskr *= maskamt;
	cmaskg *= maskamt;
	cmaskb *= maskamt;

	/////////////////////
	// Calc the RGBA fade mask
	/////////////////////
	cdestr = cfr;
	cdestg = cfg;
	cdestb = cfb;

	// fade alpha unused in software
	// maskamt = (double)(cfa/255.0l);
	// othermask = 1 - maskamt;
	// maskamt /= 0xff;

	// cdestr *= maskamt;
	// cdestg *= maskamt;
	// cdestb *= maskamt;

	/////////////////////
	// This code creates the colormap array used by software renderer
	/////////////////////
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

		// Initialise the map and delta arrays
		// map[i] stores an RGB color (as double) for index i,
		//  which is then converted to SRB2's palette later
		// deltas[i] stores a corresponding fade delta between the RGB color and the final fade color;
		//  map[i]'s values are decremented by after each use
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

		// Now allocate memory for the actual colormap array itself!
		// aligned on 8 bit for asm code
		colormap_p = (char *)extra_colormap->colormap;

		// Calculate the palette index for each palette index, for each light level
		// (as well as the two unused colormap lines we inherited from Doom)
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
	// default values
	UINT8 cr = 0, cg = 0, cb = 0, ca = 0, cfr = 0, cfg = 0, cfb = 0, cfa = 25;
	UINT32 fadestart = 0, fadeend = 31;
	UINT8 flags = 0;
	INT32 rgba = 0, fadergba = 0xFF000000;

#define HEX2INT(x) (UINT32)(x >= '0' && x <= '9' ? x - '0' : x >= 'a' && x <= 'f' ? x - 'a' + 10 : x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
#define ALPHA2INT(x) (x >= 'a' && x <= 'z' ? x - 'a' : x >= 'A' && x <= 'Z' ? x - 'A' : x >= '0' && x <= '9' ? 25 : 0)

	// Get base colormap value
	// First alpha-only, then full value
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
		// For each subsequent value, the value before it must exist
		// If we don't get every value, then set alpha to max
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

	// Get parameters like fadestart, fadeend, and flags
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

	// Get fade (dark) colormap value
	// First alpha-only, then full value
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
		// For each subsequent value, the value before it must exist
		// If we don't get every value, then set alpha to max
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

	// Pack rgba values into combined var
	// OpenGL also uses this instead of lighttables for rendering
	rgba = R_PutRgbaRGBA(cr, cg, cb, ca);
	fadergba = R_PutRgbaRGBA(cfr, cfg, cfb, cfa);

	return R_CreateColormap(rgba, fadergba, fadestart, fadeend, flags);
}

extracolormap_t *R_CreateColormap(INT32 rgba, INT32 fadergba, UINT8 fadestart, UINT8 fadeend, UINT8 flags)
{
	extracolormap_t *extra_colormap;

	// Did we just make a default colormap?
#ifdef EXTRACOLORMAPLUMPS
	if (R_CheckDefaultColormapByValues(true, true, true, rgba, fadergba, fadestart, fadeend, flags, LUMPERROR))
		return NULL;
#else
	if (R_CheckDefaultColormapByValues(true, true, true, rgba, fadergba, fadestart, fadeend, flags))
		return NULL;
#endif

	// Look for existing colormaps
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

	// Having lighttables for alpha-only entries is kind of pointless,
	// but if there happens to be a matching rgba entry that is NOT alpha-only (but has same rgb values),
	// then it needs this lighttable because we share matching entries.
	extra_colormap->colormap = R_CreateLightTable(extra_colormap);

	R_AddColormapToList(extra_colormap);

	return extra_colormap;
}

//
// R_AddColormaps()
// NOTE: The result colormap is not added to the extra_colormaps chain. You must do that yourself!
//
extracolormap_t *R_AddColormaps(extracolormap_t *exc_augend, extracolormap_t *exc_addend,
	boolean subR, boolean subG, boolean subB, boolean subA,
	boolean subFadeR, boolean subFadeG, boolean subFadeB, boolean subFadeA,
	boolean subFadeStart, boolean subFadeEnd, boolean ignoreFlags,
	boolean lighttable)
{
	INT16 red, green, blue, alpha;

	// exc_augend is added (or subtracted) onto by exc_addend
	// In Rennaisance times, the first number was considered the augend, the second number the addend
	// But since the commutative property was discovered, today they're both called addends!
	// So let's be Olde English for a hot second.

	exc_augend = R_CopyColormap(exc_augend, false);
	if(!exc_addend)
		exc_addend = R_GetDefaultColormap();

	///////////////////
	// base rgba
	///////////////////

	red = max(min(
		R_GetRgbaR(exc_augend->rgba)
			+ (subR ? -1 : 1) // subtract R
			* R_GetRgbaR(exc_addend->rgba)
		, 255), 0);

	green = max(min(
		R_GetRgbaG(exc_augend->rgba)
			+ (subG ? -1 : 1) // subtract G
			* R_GetRgbaG(exc_addend->rgba)
		, 255), 0);

	blue = max(min(
		R_GetRgbaB(exc_augend->rgba)
			+ (subB ? -1 : 1) // subtract B
			* R_GetRgbaB(exc_addend->rgba)
		, 255), 0);

	alpha = R_GetRgbaA(exc_addend->rgba);
	alpha = max(min(R_GetRgbaA(exc_augend->rgba) + (subA ? -1 : 1) * alpha, 25), 0);

	exc_augend->rgba = R_PutRgbaRGBA(red, green, blue, alpha);

	///////////////////
	// fade/dark rgba
	///////////////////

	red = max(min(
		R_GetRgbaR(exc_augend->fadergba)
			+ (subFadeR ? -1 : 1) // subtract R
			* R_GetRgbaR(exc_addend->fadergba)
		, 255), 0);

	green = max(min(
		R_GetRgbaG(exc_augend->fadergba)
			+ (subFadeG ? -1 : 1) // subtract G
			* R_GetRgbaG(exc_addend->fadergba)
		, 255), 0);

	blue = max(min(
		R_GetRgbaB(exc_augend->fadergba)
			+ (subFadeB ? -1 : 1) // subtract B
			* R_GetRgbaB(exc_addend->fadergba)
		, 255), 0);

	alpha = R_GetRgbaA(exc_addend->fadergba);
	if (alpha == 25 && !R_GetRgbaRGB(exc_addend->fadergba))
		alpha = 0; // HACK: fadergba A defaults at 25, so don't add anything in this case
	alpha = max(min(R_GetRgbaA(exc_augend->fadergba) + (subFadeA ? -1 : 1) * alpha, 25), 0);

	exc_augend->fadergba = R_PutRgbaRGBA(red, green, blue, alpha);

	///////////////////
	// parameters
	///////////////////

	exc_augend->fadestart = max(min(
		exc_augend->fadestart
			+ (subFadeStart ? -1 : 1) // subtract fadestart
			* exc_addend->fadestart
		, 31), 0);

	exc_augend->fadeend = max(min(
		exc_augend->fadeend
			+ (subFadeEnd ? -1 : 1) // subtract fadeend
			* (exc_addend->fadeend == 31 && !exc_addend->fadestart ? 0 : exc_addend->fadeend)
				// HACK: fadeend defaults to 31, so don't add anything in this case
		, 31), 0);

	if (!ignoreFlags) // overwrite flags with new value
		exc_augend->flags = exc_addend->flags;

	///////////////////
	// put it together
	///////////////////

	exc_augend->colormap = lighttable ? R_CreateLightTable(exc_augend) : NULL;
	exc_augend->next = exc_augend->prev = NULL;
	return exc_augend;
}

// Thanks to quake2 source!
// utils3/qdata/images.c
UINT8 NearestPaletteColor(UINT8 r, UINT8 g, UINT8 b, RGBA_t *palette)
{
#if defined(__SSE2__) || defined(_M_X64)
	const __m128i zero = _mm_setzero_si128();
	const __m128i rgbmask = _mm_set_epi16(0, -1, -1, -1, 0, -1, -1, -1);
	const __m128i target = _mm_set_epi16(0, b, g, r, 0, b, g, r);
	__m128i best = _mm_set1_epi32(256 * 256 * 4);
	__m128i indices = zero;
	INT32 distances[4], colors[4];
	INT32 i, bestcolor = 0, bestdistortion = 256 * 256 * 4;

	if (palette == NULL)
		palette = pMasterPalette;


	for (i = 0; i < 256; i += 4)
	{
		const __m128i packed = _mm_loadu_si128((const __m128i *)&palette[i]);
		const __m128i lo = _mm_and_si128(rgbmask,
			_mm_sub_epi16(_mm_unpacklo_epi8(packed, zero), target));
		const __m128i hi = _mm_and_si128(rgbmask,
			_mm_sub_epi16(_mm_unpackhi_epi8(packed, zero), target));
		const __m128i losq = _mm_madd_epi16(lo, lo);
		const __m128i hisq = _mm_madd_epi16(hi, hi);
		const __m128i losum = _mm_add_epi32(losq,
			_mm_shuffle_epi32(losq, _MM_SHUFFLE(2, 3, 0, 1)));
		const __m128i hisum = _mm_add_epi32(hisq,
			_mm_shuffle_epi32(hisq, _MM_SHUFFLE(2, 3, 0, 1)));
		const __m128i distance = _mm_unpacklo_epi64(
			_mm_shuffle_epi32(losum, _MM_SHUFFLE(2, 0, 2, 0)),
			_mm_shuffle_epi32(hisum, _MM_SHUFFLE(2, 0, 2, 0)));
		const __m128i better = _mm_cmpgt_epi32(best, distance);
		const __m128i current = _mm_set_epi32(i+3, i+2, i+1, i);
		best = _mm_or_si128(_mm_and_si128(better, distance),
			_mm_andnot_si128(better, best));
		indices = _mm_or_si128(_mm_and_si128(better, current),
			_mm_andnot_si128(better, indices));
	}
	_mm_storeu_si128((__m128i *)distances, best);
	_mm_storeu_si128((__m128i *)colors, indices);
	for (i = 0; i < 4; i++)

		if (distances[i] < bestdistortion
		|| (distances[i] == bestdistortion && colors[i] < bestcolor))
		{
			bestdistortion = distances[i];
			bestcolor = colors[i];
		}
	return (UINT8)bestcolor;
#else
	int dr, dg, db;
	int distortion, bestdistortion = 256 * 256 * 4, bestcolor = 0, i;

	// Use master palette if none specified
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
#endif
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

//
// R_InitData
//
// Locates all the lumps that will be used by all views
// Must be called after W_Init.
//
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

//
// R_PrecacheLevel
//
// Preloads all relevant graphics for the level.
//
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
static size_t leveltexturewipe_index;
static size_t levelflatwipe_index;
static boolean levelwipeprefertexture;
static UINT8 levelprecachecooldown;
static boolean levelprecacheprefertexture;

#define LEVELGRAPHICS_LOCAL_PRIORITY_MAX 16
#define LEVELGRAPHICS_FREQUENT_PRIORITY_MAX 12
#define LEVELGRAPHICS_FREQUENT_SEARCH_START 64
#define LEVELGRAPHICS_FREQUENT_INSERT_AFTER 54
#define LEVELGRAPHICS_FREQUENT_MAX_PIXELS (512u * 1024u)





#define LEVELGRAPHICS_COSTLY_MIN_PIXELS (256u * 1024u)
#define LEVELGRAPHICS_COSTLY_DIRECT_MAX 16
#define LEVELGRAPHICS_COSTLY_DIRECT_FIRST 5
#define LEVELGRAPHICS_COSTLY_ANIMATION_MAX 8
#define LEVELGRAPHICS_COSTLY_FLAT_MAX 12



#define LEVELTEXTURE_BACKGROUND_MAX_PIXELS (160u * 1024u)



#define LEVELGRAPHICS_INITIAL_MAX_PIXELS (64u * 1024u * 1024u)
#define LEVELGRAPHICS_INITIAL_MAX_SINGLE_PIXELS (8u * 1024u * 1024u)
#define LEVELGRAPHICS_INITIAL_MAX_MILLISECONDS 90u
#define LEVELGRAPHICS_NETGAME_INITIAL_MAX_MILLISECONDS 220u



#define LEVELGRAPHICS_SOFTWARE_INITIAL_MAX_PIXELS (16u * 1024u * 1024u)
#define LEVELGRAPHICS_SOFTWARE_INITIAL_MAX_SINGLE_PIXELS (8u * 1024u * 1024u)
#define LEVELGRAPHICS_SOFTWARE_INITIAL_MAX_MILLISECONDS 120u

typedef struct
{
	INT32 graphic;
	UINT32 references;
} levelgraphicpriority_t;

static UINT32 R_LevelGraphicPriorityScore(INT32 texture, UINT32 references,
	boolean animationframe)
{
	UINT32 score = references;
	if (texture >= 0 && texture < numtextures && textures[texture]
	&& textures[texture]->width > 0 && textures[texture]->height > 0)
	{
		const size_t pixels = (size_t)textures[texture]->width
			* (size_t)textures[texture]->height;




		UINT32 sizebonus = (UINT32)(pixels / 1024u);
		if (sizebonus > 4096u)
			sizebonus = 4096u;
		score += sizebonus;




		if (textures[texture]->patchcount > 1)
		{
			UINT32 compositebonus = (UINT32)(textures[texture]->patchcount - 1)
				* 256u;
			if (compositebonus > 8192u)
				compositebonus = 8192u;
			score += compositebonus;
		}
	}



	if (animationframe)
		score += 48u;
	return score;
}

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

static void R_QueueLocalPrecacheTexture(INT32 texture, UINT8 *seen)
{
	if (leveltexturepresent && texture >= 0 && texture < numtextures
	&& leveltexturepresent[texture] && !seen[texture]
	&& leveltexturepriority_count < LEVELGRAPHICS_LOCAL_PRIORITY_MAX)
	{
		seen[texture] = 1;
		leveltexturepriority[leveltexturepriority_count++] = texture;
	}
}

static void R_QueueLocalPrecacheFlat(INT32 flat, UINT8 *seen)
{
	if (levelflatprecachepresent && flat >= 0 && (size_t)flat < numlevelflats
	&& levelflatprecachepresent[flat] && !seen[flat]
	&& levelflatpriority_count < LEVELGRAPHICS_LOCAL_PRIORITY_MAX)
	{
		seen[flat] = 1;
		levelflatpriority[levelflatpriority_count++] = flat;
	}
}

static sector_t *R_PrecachePortalDestination(sectorportal_t *portal)
{
	if (!P_IsSectorPortalValid(portal))
		return NULL;
	switch (portal->type)
	{
	case SECPORTAL_LINE:
		if (!portal->line.dest)
			return NULL;
		return portal->line.dest->frontsector
			? portal->line.dest->frontsector : portal->line.dest->backsector;
	case SECPORTAL_OBJECT:
		return portal->mobj->subsector ? portal->mobj->subsector->sector : NULL;
	case SECPORTAL_SKYBOX:
		return skyboxmo[0]->subsector ? skyboxmo[0]->subsector->sector : NULL;
	case SECPORTAL_FLOOR:
	case SECPORTAL_CEILING:
		return portal->sector;
	default:
		return NULL;
	}
}

static void R_BuildLevelGraphicPriorities(const UINT8 *animationframes)
{
	sector_t **queue = NULL;
	UINT8 *sectorseen = NULL, *textureseen = NULL, *flatseen = NULL, *depth = NULL;
	UINT32 *textureusage = NULL, *flatusage = NULL;
	levelgraphicpriority_t *texturecandidates = NULL, *flatcandidates = NULL;
	INT32 frequenttextures[LEVELGRAPHICS_FREQUENT_PRIORITY_MAX];
	INT32 costlydirect[LEVELGRAPHICS_COSTLY_DIRECT_MAX];
	INT32 costlyanimation[LEVELGRAPHICS_COSTLY_ANIMATION_MAX];
	INT32 costlyflats[LEVELGRAPHICS_COSTLY_FLAT_MAX];
	sector_t *start;
	size_t head = 0, tail = 0, i, candidatecount, frequentcount = 0;
	size_t costlydirectcount = 0, costlyanimationcount = 0;
	size_t costlyflatcount = 0;
	boolean frequentinserted = false;

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



	if (skyboxmo[0] && skyboxmo[0]->subsector
	&& skyboxmo[0]->subsector->sector >= sectors
	&& skyboxmo[0]->subsector->sector < sectors + numsectors
	&& !sectorseen[skyboxmo[0]->subsector->sector - sectors]
	&& tail < (size_t)numsectors)
	{
		sector_t *skyboxsector = skyboxmo[0]->subsector->sector;
		sectorseen[skyboxsector - sectors] = 1;
		queue[tail] = skyboxsector;
		depth[tail++] = 0;
	}
	while (head < tail
	&& (leveltexturepriority_count < LEVELGRAPHICS_LOCAL_PRIORITY_MAX
		|| levelflatpriority_count < LEVELGRAPHICS_LOCAL_PRIORITY_MAX))
	{
		sector_t *sector = queue[head];
		const UINT8 sectordepth = depth[head++];
		INT32 localflats[2];
		ffloor_t *rover;
		UINT8 plane;



		if (sectordepth < 2)
			for (plane = 0; plane < 2; plane++)
			{
				sector_t *next = R_PrecachePortalDestination(plane
					? P_SectorGetCeilingPortal(sector) : P_SectorGetFloorPortal(sector));
				if (next && next >= sectors && next < sectors + numsectors
				&& !sectorseen[next - sectors] && tail < (size_t)numsectors)
				{
					sectorseen[next - sectors] = 1;
					queue[tail] = next;
					depth[tail++] = sectordepth + 1;
				}
			}

		localflats[0] = sector->floorpic;
		localflats[1] = sector->ceilingpic;
		for (i = 0; i < 2; i++)
			R_QueueLocalPrecacheFlat(localflats[i], flatseen);



		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS))
				continue;
			if (rover->fofflags & FOF_RENDERPLANES)
			{
				if (rover->toppic)
					R_QueueLocalPrecacheFlat(*rover->toppic, flatseen);
				if (rover->bottompic)
					R_QueueLocalPrecacheFlat(*rover->bottompic, flatseen);
			}
			if ((rover->fofflags & FOF_RENDERSIDES) && rover->master
			&& rover->master->sidenum[0] != NO_SIDEDEF
			&& rover->master->sidenum[0] < numsides)
				R_QueueLocalPrecacheTexture(
					sides[rover->master->sidenum[0]].midtexture, textureseen);
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
					R_QueueLocalPrecacheTexture(texture, textureseen);
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
	for (i = 0; i < (size_t)numsectors; i++)
	{
		const INT32 candidates[2] = {sectors[i].floorpic, sectors[i].ceilingpic};
		ffloor_t *rover;
		UINT8 candidate;
		for (candidate = 0; candidate < 2; candidate++)
		{
			const INT32 flat = candidates[candidate];
			if (levelflatprecachepresent && flat >= 0
			&& (size_t)flat < numlevelflats && levelflatprecachepresent[flat])
				flatusage[flat]++;
		}



		for (rover = sectors[i].ffloors; rover; rover = rover->next)
		{
			INT32 roverflats[2];
			if (!(rover->fofflags & FOF_EXISTS)
			|| !(rover->fofflags & FOF_RENDERPLANES))
				continue;
			roverflats[0] = rover->toppic ? *rover->toppic : -1;
			roverflats[1] = rover->bottompic ? *rover->bottompic : -1;
			for (candidate = 0; candidate < 2; candidate++)
				if (levelflatprecachepresent && roverflats[candidate] >= 0
				&& (size_t)roverflats[candidate] < numlevelflats
				&& levelflatprecachepresent[roverflats[candidate]])



					flatusage[roverflats[candidate]] += 8;
		}
	}



	P_ExpandTextureAnimationUsage(textureusage, (size_t)numtextures);



	for (i = 0; leveltexturepresent && i < numlevelflats; i++)
		if (flatusage[i])
		{
			const INT32 flattexture = levelflats[i].texture_id;
			const INT32 walltexture =
				R_CheckTextureNumForName(levelflats[i].name);
			if (flattexture >= 0 && flattexture < numtextures
			&& leveltexturepresent[flattexture])
				textureusage[flattexture] += flatusage[i] * 32u;


			if (walltexture >= 0 && walltexture < numtextures
			&& walltexture != flattexture && leveltexturepresent[walltexture])
				textureusage[walltexture] += flatusage[i] * 32u;
		}
	candidatecount = 0;
	for (i = 0; leveltexturepresent && i < (size_t)numtextures; i++)
		if (leveltexturepresent[i] && textureusage[i])
		{
			texturecandidates[candidatecount].graphic = (INT32)i;
			texturecandidates[candidatecount++].references =
				R_LevelGraphicPriorityScore((INT32)i, textureusage[i],
					animationframes && animationframes[i]);
		}
	qsort(texturecandidates, candidatecount, sizeof (*texturecandidates),
		R_CompareLevelGraphicPriority);



	for (i = 0; i < candidatecount; i++)
	{
		const INT32 texture = texturecandidates[i].graphic;
		size_t pixels;
		if (textureseen[texture] || !textures[texture]
		|| textures[texture]->width <= 0 || textures[texture]->height <= 0)
			continue;
		pixels = (size_t)textures[texture]->width
			* (size_t)textures[texture]->height;
		if (pixels < LEVELGRAPHICS_COSTLY_MIN_PIXELS)
			continue;
		if (animationframes && animationframes[texture])
		{
			if (costlyanimationcount < LEVELGRAPHICS_COSTLY_ANIMATION_MAX)
				costlyanimation[costlyanimationcount++] = texture;
		}
		else if (costlydirectcount < LEVELGRAPHICS_COSTLY_DIRECT_MAX)
			costlydirect[costlydirectcount++] = texture;
	}
	for (i = 0; i < costlydirectcount
	&& i < LEVELGRAPHICS_COSTLY_DIRECT_FIRST; i++)
	{
		textureseen[costlydirect[i]] = 1;
		leveltexturepriority[leveltexturepriority_count++] = costlydirect[i];
	}
	for (i = 0; i < costlyanimationcount; i++)
	{
		textureseen[costlyanimation[i]] = 1;
		leveltexturepriority[leveltexturepriority_count++] = costlyanimation[i];
	}
	for (i = LEVELGRAPHICS_COSTLY_DIRECT_FIRST; i < costlydirectcount; i++)
	{
		textureseen[costlydirect[i]] = 1;
		leveltexturepriority[leveltexturepriority_count++] = costlydirect[i];
	}





	while (frequentcount < LEVELGRAPHICS_FREQUENT_PRIORITY_MAX)
	{
		INT32 besttexture = -1;
		UINT32 bestreferences = 0;
		size_t candidate, selected;
		for (candidate = LEVELGRAPHICS_FREQUENT_SEARCH_START;
		candidate < candidatecount; candidate++)
		{
			const INT32 texture = texturecandidates[candidate].graphic;
			boolean alreadyselected = false;
			for (selected = 0; selected < frequentcount; selected++)
				if (frequenttextures[selected] == texture)
				{
					alreadyselected = true;
					break;
				}
			if (alreadyselected || textureseen[texture] || !textures[texture]
			|| textures[texture]->width <= 0 || textures[texture]->height <= 0
			|| (size_t)textures[texture]->width
				* (size_t)textures[texture]->height
					> LEVELGRAPHICS_FREQUENT_MAX_PIXELS)
				continue;
			if (besttexture < 0 || textureusage[texture] > bestreferences)
			{
				besttexture = texture;
				bestreferences = textureusage[texture];
			}
		}
		if (besttexture < 0)
			break;
		frequenttextures[frequentcount++] = besttexture;
	}
	for (i = 0; i < candidatecount; i++)
	{
		const INT32 texture = texturecandidates[i].graphic;
		if (!frequentinserted
		&& leveltexturepriority_count >= LEVELGRAPHICS_FREQUENT_INSERT_AFTER)
		{
			size_t selected;
			for (selected = 0; selected < frequentcount; selected++)
			{
				const INT32 frequenttexture = frequenttextures[selected];
				if (!textureseen[frequenttexture])
				{
					textureseen[frequenttexture] = 1;
					leveltexturepriority[leveltexturepriority_count++]
						= frequenttexture;
				}
			}
			frequentinserted = true;
		}
		if (!textureseen[texture])
		{
			textureseen[texture] = 1;
			leveltexturepriority[leveltexturepriority_count++] = texture;
		}
	}
	if (!frequentinserted)
		for (i = 0; i < frequentcount; i++)
			if (!textureseen[frequenttextures[i]])
			{
				textureseen[frequenttextures[i]] = 1;
				leveltexturepriority[leveltexturepriority_count++]
					= frequenttextures[i];
			}


	for (i = 0; leveltexturepresent && i < (size_t)numtextures; i++)
		if (leveltexturepresent[i] && !textureseen[i])
			leveltexturepriority[leveltexturepriority_count++] = (INT32)i;
	candidatecount = 0;
	for (i = 0; levelflatprecachepresent && i < numlevelflats; i++)
		if (levelflatprecachepresent[i] && flatusage[i])
		{
			flatcandidates[candidatecount].graphic = (INT32)i;
			flatcandidates[candidatecount++].references =
				R_LevelGraphicPriorityScore(levelflats[i].texture_id,
					flatusage[i], false);
		}
	qsort(flatcandidates, candidatecount, sizeof (*flatcandidates),
		R_CompareLevelGraphicPriority);
	for (i = 0; i < candidatecount
	&& costlyflatcount < LEVELGRAPHICS_COSTLY_FLAT_MAX; i++)
	{
		const INT32 flat = flatcandidates[i].graphic;
		const INT32 texture = levelflats[flat].texture_id;
		if (flatseen[flat] || texture < 0 || texture >= numtextures
		|| !textures[texture] || textures[texture]->width <= 0
		|| textures[texture]->height <= 0
		|| (size_t)textures[texture]->width
			* (size_t)textures[texture]->height
				< LEVELGRAPHICS_COSTLY_MIN_PIXELS)
			continue;
		costlyflats[costlyflatcount++] = flat;
	}
	for (i = 0; i < costlyflatcount; i++)
	{
		flatseen[costlyflats[i]] = 1;
		levelflatpriority[levelflatpriority_count++] = costlyflats[i];
	}
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

static boolean R_PrewarmSoftwareLevelTextureBounded(INT32 texture,
	size_t maxpixels, size_t *remaining, boolean retire_oversize)
{
	texture_t *definition;
	size_t pixels;

	if (!leveltexturepresent || texture < 0 || texture >= numtextures
	|| !leveltexturepresent[texture])
		return false;
	if (!(definition = textures[texture])
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

	if (!texturecache[texture])
		R_GenerateTexture((size_t)texture);
	leveltexturepresent[texture] = 0;
	if (remaining)
		*remaining -= pixels;
	return true;
}

static boolean R_PrewarmSoftwareLevelFlatBounded(size_t flat,
	size_t maxpixels, size_t *remaining, boolean retire_oversize)
{
	levelflat_t *levelflat;
	texture_t *definition;
	INT32 translated;
	size_t pixels;

	if (!levelflatprecachepresent || flat >= numlevelflats
	|| !levelflatprecachepresent[flat])
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

	if (!definition->flat)
		R_GetFlat(levelflat);
	levelflatprecachepresent[flat] = 0;
	if (remaining)
		*remaining -= pixels;
	return true;
}

static boolean R_PrewarmNextSoftwareLevelTexture(size_t maxpixels)
{
	while (leveltexturepriority
	&& leveltexturepriority_index < leveltexturepriority_count)
	{
		const INT32 texture = leveltexturepriority[leveltexturepriority_index];
		if (R_PrewarmSoftwareLevelTextureBounded(texture, maxpixels, NULL, true))
			return true;
		if (!leveltexturepresent || !leveltexturepresent[texture])
			leveltexturepriority_index++;
	}
	if (leveltexturepriority
	&& leveltexturepriority_index >= leveltexturepriority_count)
	{
		free(leveltexturepriority);
		leveltexturepriority = NULL;
		leveltexturepriority_count = leveltexturepriority_index = 0;
	}

	while (leveltexturepresent
	&& leveltextureprecache_texture < (size_t)numtextures)
	{
		if (!leveltexturepresent[leveltextureprecache_texture])
		{
			leveltextureprecache_texture++;
			continue;
		}
		if (R_PrewarmSoftwareLevelTextureBounded(
			(INT32)leveltextureprecache_texture, maxpixels, NULL, true))
			return true;
		if (!leveltexturepresent[leveltextureprecache_texture])
			leveltextureprecache_texture++;
	}
	if (leveltexturepresent
	&& leveltextureprecache_texture >= (size_t)numtextures)
	{
		free(leveltexturepresent);
		leveltexturepresent = NULL;
	}
	return false;
}

static boolean R_PrewarmNextSoftwareLevelFlat(size_t maxpixels)
{
	while (levelflatpriority
	&& levelflatpriority_index < levelflatpriority_count)
	{
		const INT32 flat = levelflatpriority[levelflatpriority_index];
		if (R_PrewarmSoftwareLevelFlatBounded((size_t)flat,
			maxpixels, NULL, true))
			return true;
		if (!levelflatprecachepresent || !levelflatprecachepresent[flat])
			levelflatpriority_index++;
	}
	if (levelflatpriority
	&& levelflatpriority_index >= levelflatpriority_count)
	{
		free(levelflatpriority);
		levelflatpriority = NULL;
		levelflatpriority_count = levelflatpriority_index = 0;
	}

	while (levelflatprecachepresent
	&& levelflatprecache_flat < numlevelflats)
	{
		if (!levelflatprecachepresent[levelflatprecache_flat])
		{
			levelflatprecache_flat++;
			continue;
		}
		if (R_PrewarmSoftwareLevelFlatBounded(levelflatprecache_flat,
			maxpixels, NULL, true))
			return true;
		if (!levelflatprecachepresent[levelflatprecache_flat])
			levelflatprecache_flat++;
	}
	if (levelflatprecachepresent && levelflatprecache_flat >= numlevelflats)
	{
		free(levelflatprecachepresent);
		levelflatprecachepresent = NULL;
	}
	return false;
}

static void R_PrewarmInitialSoftwareLevelGraphics(void)
{
	size_t remaining = LEVELGRAPHICS_SOFTWARE_INITIAL_MAX_PIXELS;
	size_t texturepriority = 0, flatpriority = 0;
	const size_t texturelimit = leveltexturepriority_count;
	const size_t flatlimit = levelflatpriority_count;
	const precise_t started = I_GetPreciseTime();
	const precise_t deadline = started + (I_GetPrecisePrecision()
		* LEVELGRAPHICS_SOFTWARE_INITIAL_MAX_MILLISECONDS) / 1000u;




	while ((texturepriority < texturelimit || flatpriority < flatlimit)
	&& remaining && I_GetPreciseTime() < deadline)
	{
		if (leveltexturepriority && texturepriority < texturelimit)
		{
			const INT32 texture = leveltexturepriority[texturepriority++];
			R_PrewarmSoftwareLevelTextureBounded(texture,
				LEVELGRAPHICS_SOFTWARE_INITIAL_MAX_SINGLE_PIXELS,
				&remaining, false);
		}
		if (levelflatpriority && flatpriority < flatlimit
		&& remaining && I_GetPreciseTime() < deadline)
		{
			const INT32 flat = levelflatpriority[flatpriority++];
			R_PrewarmSoftwareLevelFlatBounded((size_t)flat,
				LEVELGRAPHICS_SOFTWARE_INITIAL_MAX_SINGLE_PIXELS,
				&remaining, false);
		}
	}
}

#ifdef HWRENDER
static boolean R_PrewarmLevelTextureBounded(INT32 texture, size_t maxpixels,
	size_t *remaining, boolean retire_oversize)
{
	INT32 translated;
	texture_t *definition;
	size_t pixels;

	if (!leveltexturepresent || texture < 0 || texture >= numtextures
	|| !leveltexturepresent[texture])
		return false;



	translated = texture;
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

	HWR_GetTexture(translated);
	leveltexturepresent[texture] = 0;
	if (remaining)
		*remaining -= pixels;
	return true;
}

static boolean R_PrewarmLevelFlatBounded(size_t flat, size_t maxpixels,
	size_t *remaining, boolean retire_oversize)
{
	levelflat_t *levelflat;
	texture_t *definition;
	INT32 translated;
	size_t pixels;

	if (!levelflatprecachepresent || flat >= numlevelflats
	|| !levelflatprecachepresent[flat])
		return false;

	levelflat = &levelflats[flat];
	if (levelflat->type == LEVELFLAT_NONE || levelflat->texture_id < 0
	|| levelflat->texture_id >= numtextures)
	{
		levelflatprecachepresent[flat] = 0;
		return false;
	}

	translated = levelflat->texture_id;
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

	HWR_GetLevelFlat(levelflat);
	levelflatprecachepresent[flat] = 0;
	if (remaining)
		*remaining -= pixels;
	return true;
}

static boolean R_PrewarmNextLevelFlat(size_t maxpixels)
{
	while (levelflatpriority
	&& levelflatpriority_index < levelflatpriority_count)
	{
		const INT32 flat = levelflatpriority[levelflatpriority_index];
		if (R_PrewarmLevelFlatBounded((size_t)flat, maxpixels, NULL, true))
			return true;
		if (!levelflatprecachepresent || !levelflatprecachepresent[flat])
			levelflatpriority_index++;
	}
	if (levelflatpriority
	&& levelflatpriority_index >= levelflatpriority_count)
	{
		free(levelflatpriority);
		levelflatpriority = NULL;
		levelflatpriority_count = levelflatpriority_index = 0;
	}

	while (levelflatprecachepresent
	&& levelflatprecache_flat < numlevelflats)
	{
		if (!levelflatprecachepresent[levelflatprecache_flat])
		{
			levelflatprecache_flat++;
			continue;
		}
		if (R_PrewarmLevelFlatBounded(levelflatprecache_flat,
			maxpixels, NULL, true))
			return true;
		if (!levelflatprecachepresent[levelflatprecache_flat])
			levelflatprecache_flat++;
	}
	if (levelflatprecachepresent && levelflatprecache_flat >= numlevelflats)
	{
		free(levelflatprecachepresent);
		levelflatprecachepresent = NULL;
	}
	return false;
}

static boolean R_PrewarmNextLevelTexture(size_t maxpixels)
{
	while (leveltexturepriority
	&& leveltexturepriority_index < leveltexturepriority_count)
	{
		const INT32 texture = leveltexturepriority[leveltexturepriority_index];
		if (R_PrewarmLevelTextureBounded(texture, maxpixels, NULL, true))
			return true;
		if (!leveltexturepresent || !leveltexturepresent[texture])
			leveltexturepriority_index++;
	}
	if (leveltexturepriority
	&& leveltexturepriority_index >= leveltexturepriority_count)
	{
		free(leveltexturepriority);
		leveltexturepriority = NULL;
		leveltexturepriority_count = leveltexturepriority_index = 0;
	}

	while (leveltexturepresent
	&& leveltextureprecache_texture < (size_t)numtextures)
	{
		if (!leveltexturepresent[leveltextureprecache_texture])
		{
			leveltextureprecache_texture++;
			continue;
		}
		if (R_PrewarmLevelTextureBounded((INT32)leveltextureprecache_texture,
			maxpixels, NULL, true))
			return true;
		if (!leveltexturepresent[leveltextureprecache_texture])
			leveltextureprecache_texture++;
	}
	if (leveltexturepresent
	&& leveltextureprecache_texture >= (size_t)numtextures)
	{
		free(leveltexturepresent);
		leveltexturepresent = NULL;
	}
	return false;
}

static boolean R_PrewarmNextWipeTexture(void)
{
	while (leveltexturepriority && leveltexturepresent
	&& leveltexturewipe_index < leveltexturepriority_count)
	{
		const INT32 texture = leveltexturepriority[leveltexturewipe_index];
		if (R_PrewarmLevelTextureBounded(texture, 512u * 1024u, NULL, false))
			return true;


		leveltexturewipe_index++;
	}
	return false;
}

static boolean R_PrewarmNextWipeFlat(void)
{
	while (levelflatpriority && levelflatprecachepresent
	&& levelflatwipe_index < levelflatpriority_count)
	{
		const INT32 flat = levelflatpriority[levelflatwipe_index];
		if (R_PrewarmLevelFlatBounded((size_t)flat, 512u * 1024u, NULL, false))
			return true;
		levelflatwipe_index++;
	}
	return false;
}

static void R_PrewarmInitialLevelGraphics(void)
{
	size_t remaining = LEVELGRAPHICS_INITIAL_MAX_PIXELS;
	size_t texturepriority = 0, flatpriority = 0;
	size_t texture = 0, flat = 0;




	const UINT32 maxmilliseconds = netgame
		? LEVELGRAPHICS_NETGAME_INITIAL_MAX_MILLISECONDS
		: LEVELGRAPHICS_INITIAL_MAX_MILLISECONDS;
	const precise_t started = I_GetPreciseTime();
	const precise_t deadline = started
		+ (I_GetPrecisePrecision() * maxmilliseconds) / 1000u;

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

void R_ContinueLevelWipePrecache(void)
{
#ifdef HWRENDER
	const precise_t started = I_GetPreciseTime();
	const precise_t budget = I_GetPrecisePrecision() / 250;
	UINT8 warmed;

	if (!netgame || rendermode != render_opengl
	|| vid.glstate != VID_GL_LIBRARY_LOADED)
		return;



	for (warmed = 0; warmed < 8 && I_GetPreciseTime() - started < budget; warmed++)
	{
		boolean prepared;
		if (levelwipeprefertexture)
		{
			prepared = R_PrewarmNextWipeTexture();
			if (!prepared)
				prepared = R_PrewarmNextWipeFlat();
		}
		else
		{
			prepared = R_PrewarmNextWipeFlat();
			if (!prepared)
				prepared = R_PrewarmNextWipeTexture();
		}
		levelwipeprefertexture = !levelwipeprefertexture;
		if (!prepared)
			break;
	}
#endif
}


void R_ContinueLevelSpritePrecache(boolean blocking_warmup)
{
	const precise_t started = I_GetPreciseTime();
	const precise_t budget = (precise_t)(I_GetPrecisePrecision() / 1000);




	const size_t backgroundmaxpixels = LEVELTEXTURE_BACKGROUND_MAX_PIXELS;
	unsigned int warmed = 0;
	boolean hardware = false;

	if (!levelspritepresent && !leveltexturepresent && !levelflatprecachepresent)
		return;



	if (levelprecachecooldown && !blocking_warmup)
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






	if (hardware && netgame && !blocking_warmup)
		return;

	if (!hardware)
	{
		const size_t maxpixels = blocking_warmup
			? LEVELGRAPHICS_SOFTWARE_INITIAL_MAX_SINGLE_PIXELS
			: backgroundmaxpixels;
		boolean warmedgraphic = false;

		if (levelprecacheprefertexture)
		{
			warmedgraphic = R_PrewarmNextSoftwareLevelTexture(maxpixels);
			if (!warmedgraphic)
				warmedgraphic = R_PrewarmNextSoftwareLevelFlat(maxpixels);
			levelprecacheprefertexture = false;
		}
		else
		{
			warmedgraphic = R_PrewarmNextSoftwareLevelFlat(maxpixels);
			if (!warmedgraphic)
				warmedgraphic = R_PrewarmNextSoftwareLevelTexture(maxpixels);
			levelprecacheprefertexture = true;
		}
		if (warmedgraphic)
			warmed++;
	}

#ifdef HWRENDER
	if (hardware)
	{
		const size_t maxpixels = blocking_warmup
			? LEVELGRAPHICS_INITIAL_MAX_SINGLE_PIXELS
			: backgroundmaxpixels;
		boolean warmedgraphic = false;



		if (levelprecacheprefertexture)
		{
			warmedgraphic = R_PrewarmNextLevelTexture(maxpixels);
			if (!warmedgraphic)
				warmedgraphic = R_PrewarmNextLevelFlat(maxpixels);
			levelprecacheprefertexture = false;
		}
		else
		{
			warmedgraphic = R_PrewarmNextLevelFlat(maxpixels);
			if (!warmedgraphic)
				warmedgraphic = R_PrewarmNextLevelTexture(maxpixels);
			levelprecacheprefertexture = true;
		}
		if (warmedgraphic)
			warmed++;
	}
#endif




	while (levelspritepresent && levelspriteprecache_sprite < numsprites
	&& warmed < 1)
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
				backgroundmaxpixels))
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

	if (warmed && !blocking_warmup)
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
	UINT8 *textureanimationframes = NULL;
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
	leveltexturewipe_index = levelflatwipe_index = 0;
	levelwipeprefertexture = true;
	leveltexturepriority_count = leveltexturepriority_index = 0;
	levelflatpriority_count = levelflatpriority_index = 0;
	levelprecachecooldown = 0;
	levelprecacheprefertexture = true;






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
			flatmemory = 0;

	//
	// Precache textures.
	//
	// no need to precache all software textures in 3D mode
	// (note they are still used with the reference software view)
	texturepresent = calloc(numtextures, sizeof (*texturepresent));
	if (texturepresent == NULL) I_Error("%s: Out of memory looking up textures", "R_PrecacheLevel");

	for (j = 0; j < numsides; j++)
	{
		// huh, a potential bug here????
		if (sides[j].toptexture >= 0 && sides[j].toptexture < numtextures)
			texturepresent[sides[j].toptexture] = true;
		if (sides[j].midtexture >= 0 && sides[j].midtexture < numtextures)
			texturepresent[sides[j].midtexture] = true;
		if (sides[j].bottomtexture >= 0 && sides[j].bottomtexture < numtextures)
			texturepresent[sides[j].bottomtexture] = true;
	}





	for (j = 0; j < (size_t)numsectors; j++)
	{
		ffloor_t *rover;
		for (rover = sectors[j].ffloors; rover; rover = rover->next)
		{
			size_t targetline;
			if (!rover->master || !(rover->fofflags & FOF_EXISTS)
			|| !(rover->fofflags & FOF_RENDERSIDES)
			|| (rover->fofflags & FOF_FOG))
				continue;

			for (targetline = 0; targetline < sectors[j].linecount; targetline++)
			{
				line_t *source = R_GetFFloorLine(sectors[j].lines[targetline],
					rover, &sectors[j]);
				size_t sideindex;
				INT32 texture;
				if (!source || (sideindex = source->sidenum[0]) == NO_SIDEDEF
				|| sideindex >= numsides)
					continue;
				texture = sides[sideindex].midtexture;
				if (texture >= 0 && texture < numtextures)
					texturepresent[texture] = true;
			}
		}
	}




#ifdef HWRENDER
	if (rendermode == render_opengl && vid.glstate == VID_GL_LIBRARY_LOADED)
	{
		textureanimationframes = malloc((size_t)numtextures
			* sizeof (*textureanimationframes));
		if (numtextures && !textureanimationframes)
			I_Error("%s: Out of memory ranking animated textures", "R_PrecacheLevel");
		memcpy(textureanimationframes, texturepresent,
			(size_t)numtextures * sizeof (*textureanimationframes));
	}
#endif




	P_ExpandTextureAnimationPresence(texturepresent, (size_t)numtextures);
#ifdef HWRENDER
	if (textureanimationframes)
		for (j = 0; j < (size_t)numtextures; j++)
			textureanimationframes[j] = texturepresent[j]
				&& !textureanimationframes[j];
#endif

	// Sky texture is always present.
	// Note that F_SKY1 is the name used to indicate a sky floor/ceiling as a flat,
	// while the sky texture is stored like a wall texture, with a skynum dependent name.
	texturepresent[skytexture] = true;

	if (rendermode == render_soft)
	{
#if defined(_WIN32) && !defined(DEDICATED)
		const precise_t preload_started = I_GetPreciseTime();
#endif
		texturememory = 0;
		leveltexturepresent = texturepresent;
		levelflatprecachepresent = calloc(numlevelflats,
			sizeof (*levelflatprecachepresent));
		if (numlevelflats && !levelflatprecachepresent)
			I_Error("%s: Out of memory looking up level flats", "R_PrecacheLevel");
		for (j = 0; j < numlevelflats; j++)
			if (levelflats[j].type != LEVELFLAT_NONE)
				levelflatprecachepresent[j] = true;
		R_BuildLevelGraphicPriorities(NULL);
		R_PrewarmInitialSoftwareLevelGraphics();
#if defined(_WIN32) && !defined(DEDICATED)
		I_ReportHitch("initial Software level graphics preload",
			preload_started, 25);
#endif
	}
#ifdef HWRENDER
	else
	{
		precise_t preload_started;
		size_t flat;

		leveltexturepresent = texturepresent;

		levelflatprecachepresent = calloc(numlevelflats,
			sizeof (*levelflatprecachepresent));
		if (numlevelflats && !levelflatprecachepresent)
			I_Error("%s: Out of memory looking up level flats", "R_PrecacheLevel");
		for (flat = 0; flat < numlevelflats; flat++)
			if (levelflats[flat].type != LEVELFLAT_NONE)
				levelflatprecachepresent[flat] = true;

		R_BuildLevelGraphicPriorities(textureanimationframes);
		free(textureanimationframes);
		textureanimationframes = NULL;

		preload_started = I_GetPreciseTime();
		R_PrewarmInitialLevelGraphics();
		I_ReportHitch("initial OpenGL level graphics preload",
			preload_started, 25);
	}
#endif
	}

	//
	// Precache sprites.
	//
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
			// see R_InitSprites for more about lumppat,lumpid
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
