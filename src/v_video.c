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
#include "r_local.h"
#include "p_local.h"  
#include "g_game.h"  
#include "v_video.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "f_finale.h"
#include "r_draw.h"
#include "console.h"

#include "i_video.h"  
#include "z_zone.h"
#include "m_misc.h"
#include "m_random.h"
#include "doomstat.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"
#endif


UINT8 *screens[5];






static CV_PossibleValue_t ticrate_cons_t[] = {{0, "No"}, {1, "Full"}, {2, "Compact"}, {0, NULL}};
consvar_t cv_ticrate = CVAR_INIT ("showfps", "No", CV_SAVE, ticrate_cons_t, NULL);

static void CV_palette_OnChange(void);

static CV_PossibleValue_t gamma_cons_t[] = {{-15, "MIN"}, {5, "MAX"}, {0, NULL}};
consvar_t cv_globalgamma = CVAR_INIT ("gamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);

static CV_PossibleValue_t saturation_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
consvar_t cv_globalsaturation = CVAR_INIT ("saturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);

#define huecoloursteps 4

static CV_PossibleValue_t hue_cons_t[] = {{0, "MIN"}, {(huecoloursteps*6)-1, "MAX"}, {0, NULL}};
consvar_t cv_rhue = CVAR_INIT ("rhue",  "0", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_yhue = CVAR_INIT ("yhue",  "4", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_ghue = CVAR_INIT ("ghue",  "8", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_chue = CVAR_INIT ("chue", "12", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_bhue = CVAR_INIT ("bhue", "16", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_mhue = CVAR_INIT ("mhue", "20", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);

consvar_t cv_rgamma = CVAR_INIT ("rgamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_ygamma = CVAR_INIT ("ygamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_ggamma = CVAR_INIT ("ggamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_cgamma = CVAR_INIT ("cgamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_bgamma = CVAR_INIT ("bgamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_mgamma = CVAR_INIT ("mgamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);

consvar_t cv_rsaturation = CVAR_INIT ("rsaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_ysaturation = CVAR_INIT ("ysaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_gsaturation = CVAR_INIT ("gsaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_csaturation = CVAR_INIT ("csaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_bsaturation = CVAR_INIT ("bsaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_msaturation = CVAR_INIT ("msaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);

static CV_PossibleValue_t constextsize_cons_t[] = {
	{V_NOSCALEPATCH, "Small"}, {V_SMALLSCALEPATCH, "Medium"}, {V_MEDSCALEPATCH, "Large"}, {0, "Huge"},
	{0, NULL}};
static void CV_constextsize_OnChange(void);
consvar_t cv_constextsize = CVAR_INIT ("con_textsize", "Medium", CV_SAVE|CV_CALL, constextsize_cons_t, CV_constextsize_OnChange);


RGBA_t *pLocalPalette = NULL;
RGBA_t *pMasterPalette = NULL;

  
                                                                                   
                                                                 
                                                      
              
  
float Cubepal[2][2][2][3];
boolean Cubeapply = false;


static boolean InitCube(void)
{
	boolean apply = false;
	UINT8 q;
	float working[2][2][2][3] =  
	{
		{
			{
				{0.0, 0.0, 0.0},  
				{0.0, 0.0, 1.0}   
			},
			{
				{0.0, 1.0, 0.0},  
				{0.0, 1.0, 1.0}   
			}
		},
		{
			{
				{1.0, 0.0, 0.0},  
				{1.0, 0.0, 1.0}   
			},
			{
				{1.0, 1.0, 0.0},  
				{1.0, 1.0, 1.0}   
			}
		}
	};
	float desatur[3];  
	float globalgammamul, globalgammaoffs;
	boolean doinggamma;

#define diffcons(cv) (cv.value != atoi(cv.defaultvalue))

	doinggamma = diffcons(cv_globalgamma);

#define gammascale 8
	globalgammamul = (cv_globalgamma.value ? ((255 - (gammascale*abs(cv_globalgamma.value)))/255.0) : 1.0);
	globalgammaoffs = ((cv_globalgamma.value > 0) ? ((gammascale*cv_globalgamma.value)/255.0) : 0.0);
	desatur[0] = desatur[1] = desatur[2] = globalgammaoffs + (0.33*globalgammamul);

	if (doinggamma
		|| diffcons(cv_rhue)
		|| diffcons(cv_yhue)
		|| diffcons(cv_ghue)
		|| diffcons(cv_chue)
		|| diffcons(cv_bhue)
		|| diffcons(cv_mhue)
		|| diffcons(cv_rgamma)
		|| diffcons(cv_ygamma)
		|| diffcons(cv_ggamma)
		|| diffcons(cv_cgamma)
		|| diffcons(cv_bgamma)
		|| diffcons(cv_mgamma))  
	{
		float mod, tempgammamul, tempgammaoffs;

		apply = true;

		working[0][0][0][0] = working[0][0][0][1] = working[0][0][0][2] = globalgammaoffs;
		working[1][1][1][0] = working[1][1][1][1] = working[1][1][1][2] = globalgammaoffs+globalgammamul;

#define dohue(hue, gamma, loc) \
		tempgammamul = (gamma ? ((255 - (gammascale*abs(gamma)))/255.0)*globalgammamul : globalgammamul);\
		tempgammaoffs = ((gamma > 0) ? ((gammascale*gamma)/255.0) + globalgammaoffs : globalgammaoffs);\
		mod = ((hue % huecoloursteps)*(tempgammamul)/huecoloursteps);\
		switch (hue/huecoloursteps)\
		{\
			case 0:\
			default:\
				loc[0] = tempgammaoffs+tempgammamul;\
				loc[1] = tempgammaoffs+mod;\
				loc[2] = tempgammaoffs;\
				break;\
			case 1:\
				loc[0] = tempgammaoffs+tempgammamul-mod;\
				loc[1] = tempgammaoffs+tempgammamul;\
				loc[2] = tempgammaoffs;\
				break;\
			case 2:\
				loc[0] = tempgammaoffs;\
				loc[1] = tempgammaoffs+tempgammamul;\
				loc[2] = tempgammaoffs+mod;\
				break;\
			case 3:\
				loc[0] = tempgammaoffs;\
				loc[1] = tempgammaoffs+tempgammamul-mod;\
				loc[2] = tempgammaoffs+tempgammamul;\
				break;\
			case 4:\
				loc[0] = tempgammaoffs+mod;\
				loc[1] = tempgammaoffs;\
				loc[2] = tempgammaoffs+tempgammamul;\
				break;\
			case 5:\
				loc[0] = tempgammaoffs+tempgammamul;\
				loc[1] = tempgammaoffs;\
				loc[2] = tempgammaoffs+tempgammamul-mod;\
				break;\
		}
		dohue(cv_rhue.value, cv_rgamma.value, working[1][0][0]);
		dohue(cv_yhue.value, cv_ygamma.value, working[1][1][0]);
		dohue(cv_ghue.value, cv_ggamma.value, working[0][1][0]);
		dohue(cv_chue.value, cv_cgamma.value, working[0][1][1]);
		dohue(cv_bhue.value, cv_bgamma.value, working[0][0][1]);
		dohue(cv_mhue.value, cv_mgamma.value, working[1][0][1]);
#undef dohue
	}

#define dosaturation(a, e) a = ((1 - work)*e + work*a)
#define docvsat(cv_sat, hue, gamma, r, g, b) \
	if diffcons(cv_sat)\
	{\
		float work, mod, tempgammamul, tempgammaoffs;\
		apply = true;\
		work = (cv_sat.value/10.0);\
		mod = ((hue % huecoloursteps)*(1.0)/huecoloursteps);\
		if (hue & huecoloursteps)\
			mod = 2-mod;\
		else\
			mod += 1;\
		tempgammamul = (gamma ? ((255 - (gammascale*abs(gamma)))/255.0)*globalgammamul : globalgammamul);\
		tempgammaoffs = ((gamma > 0) ? ((gammascale*gamma)/255.0) + globalgammaoffs : globalgammaoffs);\
		for (q = 0; q < 3; q++)\
			dosaturation(working[r][g][b][q], (tempgammaoffs+(desatur[q]*mod*tempgammamul)));\
	}

	docvsat(cv_rsaturation, cv_rhue.value, cv_rgamma.value, 1, 0, 0);
	docvsat(cv_ysaturation, cv_yhue.value, cv_ygamma.value, 1, 1, 0);
	docvsat(cv_gsaturation, cv_ghue.value, cv_ggamma.value, 0, 1, 0);
	docvsat(cv_csaturation, cv_chue.value, cv_cgamma.value, 0, 1, 1);
	docvsat(cv_bsaturation, cv_bhue.value, cv_bgamma.value, 0, 0, 1);
	docvsat(cv_msaturation, cv_mhue.value, cv_mgamma.value, 1, 0, 1);

#undef gammascale

	if diffcons(cv_globalsaturation)
	{
		float work = (cv_globalsaturation.value/10.0);

		apply = true;

		for (q = 0; q < 3; q++)
		{
			dosaturation(working[1][0][0][q], desatur[q]);
			dosaturation(working[0][1][0][q], desatur[q]);
			dosaturation(working[0][0][1][q], desatur[q]);

			dosaturation(working[1][1][0][q], 2*desatur[q]);
			dosaturation(working[0][1][1][q], 2*desatur[q]);
			dosaturation(working[1][0][1][q], 2*desatur[q]);
		}
	}

#undef dosaturation

#undef diffcons

	if (!apply)
		return false;

#define dowork(i, j, k, l) \
	if (working[i][j][k][l] > 1.0)\
		working[i][j][k][l] = 1.0;\
	else if (working[i][j][k][l] < 0.0)\
		working[i][j][k][l] = 0.0;\
	Cubepal[i][j][k][l] = working[i][j][k][l]
	for (q = 0; q < 3; q++)
	{
		dowork(0, 0, 0, q);
		dowork(1, 0, 0, q);
		dowork(0, 1, 0, q);
		dowork(1, 1, 0, q);
		dowork(0, 0, 1, q);
		dowork(1, 0, 1, q);
		dowork(0, 1, 1, q);
		dowork(1, 1, 1, q);
	}
#undef dowork

	return true;
}

#ifdef BACKWARDSCOMPATCORRECTION
  
                                                                          
                                                                          
                                                                           
                                                                
              
                                                     
  
const UINT8 correctiontable[256] =
	{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
	17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
	33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
	49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,
	65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,
	81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,
	97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,
	113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
	144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
	160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
	176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
	192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
	208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
	224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
	240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};
#endif


static void LoadPalette(const char *lumpname)
{
	lumpnum_t lumpnum = W_GetNumForName(lumpname);
	size_t i, palsize = W_LumpLength(lumpnum)/3;
	UINT8 *pal;

	Cubeapply = InitCube();

	Z_Free(pLocalPalette);
	Z_Free(pMasterPalette);

	pLocalPalette = Z_Malloc(sizeof (*pLocalPalette)*palsize, PU_STATIC, NULL);
	pMasterPalette = Z_Malloc(sizeof (*pMasterPalette)*palsize, PU_STATIC, NULL);

	pal = W_CacheLumpNum(lumpnum, PU_CACHE);
	for (i = 0; i < palsize; i++)
	{
#ifdef BACKWARDSCOMPATCORRECTION
		pMasterPalette[i].s.red = pLocalPalette[i].s.red = correctiontable[*pal++];
		pMasterPalette[i].s.green = pLocalPalette[i].s.green = correctiontable[*pal++];
		pMasterPalette[i].s.blue = pLocalPalette[i].s.blue = correctiontable[*pal++];
#else
		pMasterPalette[i].s.red = pLocalPalette[i].s.red = *pal++;
		pMasterPalette[i].s.green = pLocalPalette[i].s.green = *pal++;
		pMasterPalette[i].s.blue = pLocalPalette[i].s.blue = *pal++;
#endif
		pMasterPalette[i].s.alpha = pLocalPalette[i].s.alpha = 0xFF;


		if (Cubeapply)
			V_CubeApply(&pLocalPalette[i].s.red, &pLocalPalette[i].s.green, &pLocalPalette[i].s.blue);
	}
}

void V_CubeApply(UINT8 *red, UINT8 *green, UINT8 *blue)
{
	float working[4][3];
	float linear;
	UINT8 q;

	if (!Cubeapply)
		return;

	linear = (*red/255.0);
#define dolerp(e1, e2) ((1 - linear)*e1 + linear*e2)
	for (q = 0; q < 3; q++)
	{
		working[0][q] = dolerp(Cubepal[0][0][0][q], Cubepal[1][0][0][q]);
		working[1][q] = dolerp(Cubepal[0][1][0][q], Cubepal[1][1][0][q]);
		working[2][q] = dolerp(Cubepal[0][0][1][q], Cubepal[1][0][1][q]);
		working[3][q] = dolerp(Cubepal[0][1][1][q], Cubepal[1][1][1][q]);
	}
	linear = (*green/255.0);
	for (q = 0; q < 3; q++)
	{
		working[0][q] = dolerp(working[0][q], working[1][q]);
		working[1][q] = dolerp(working[2][q], working[3][q]);
	}
	linear = (*blue/255.0);
	for (q = 0; q < 3; q++)
	{
		working[0][q] = 255*dolerp(working[0][q], working[1][q]);
		if (working[0][q] > 255.0)
			working[0][q] = 255.0;
		else if (working[0][q]  < 0.0)
			working[0][q] = 0.0;
	}
#undef dolerp

	*red = (UINT8)(working[0][0]);
	*green = (UINT8)(working[0][1]);
	*blue = (UINT8)(working[0][2]);
}

const char *R_GetPalname(UINT16 num)
{
	static char palname[8+1];
	char newpal[9] = "PLAYPAL\0";

	if (num > 0 && num <= 10000)
		snprintf(newpal, 8, "PAL%04u", num-1);

	strncpy(palname, newpal, sizeof(palname)-1);
	palname[8] = 0;
	return palname;
}

const char *GetPalette(void)
{
	if (gamestate == GS_LEVEL)
		return R_GetPalname(mapheaderinfo[gamemap-1]->palette);
	return "PLAYPAL";
}

static void LoadMapPalette(void)
{
	LoadPalette(GetPalette());
}





void V_SetPalette(INT32 palettenum)
{
	if (!pLocalPalette)
		LoadMapPalette();

#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_SetPalette(&pLocalPalette[palettenum*256]);
#if defined (__unix__) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	else
#endif
#endif
	if (rendermode != render_none)
		I_SetPalette(&pLocalPalette[palettenum*256]);
}

void V_SetPaletteLump(const char *pal)
{
	LoadPalette(pal);
#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_SetPalette(pLocalPalette);
#if defined (__unix__) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	else
#endif
#endif
	if (rendermode != render_none)
		I_SetPalette(pLocalPalette);
}

static void CV_palette_OnChange(void)
{

	LoadMapPalette();
	V_SetPalette(0);
}

static void CV_constextsize_OnChange(void)
{
	if (!con_refresh)
		con_recalc = true;
}





void VID_BlitLinearScreen(const UINT8 *srcptr, UINT8 *destptr, INT32 width, INT32 height, size_t srcrowbytes,
	size_t destrowbytes)
{
	if (srcrowbytes == destrowbytes)
		M_Memcpy(destptr, srcptr, srcrowbytes * height);
	else
	{
		while (height--)
		{
			M_Memcpy(destptr, srcptr, width);

			destptr += destrowbytes;
			srcptr += srcrowbytes;
		}
	}
}

static UINT8 hudplusalpha[11]  = { 10,  8,  6,  4,  2,  0,  0,  0,  0,  0,  0};
static UINT8 hudminusalpha[11] = { 10,  9,  9,  8,  8,  7,  7,  6,  6,  5,  5};

static const UINT8 *v_colormap = NULL;
static const UINT8 *v_translevel = NULL;

static inline UINT8 standardpdraw(const UINT8 *dest, const UINT8 *source, fixed_t ofs)
{
	(void)dest; return source[ofs>>FRACBITS];
}
static inline UINT8 mappedpdraw(const UINT8 *dest, const UINT8 *source, fixed_t ofs)
{
	(void)dest; return *(v_colormap + source[ofs>>FRACBITS]);
}
static inline UINT8 translucentpdraw(const UINT8 *dest, const UINT8 *source, fixed_t ofs)
{
	return *(v_translevel + ((source[ofs>>FRACBITS]<<8)&0xff00) + (*dest&0xff));
}
static inline UINT8 transmappedpdraw(const UINT8 *dest, const UINT8 *source, fixed_t ofs)
{
	return *(v_translevel + (((*(v_colormap + source[ofs>>FRACBITS]))<<8)&0xff00) + (*dest&0xff));
}


void V_DrawStretchyFixedPatch(fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 scrn, patch_t *patch, const UINT8 *colormap)
{
	UINT8 (*patchdrawfunc)(const UINT8*, const UINT8*, fixed_t);
	UINT32 alphalevel = ((scrn & V_ALPHAMASK) >> V_ALPHASHIFT);
	UINT32 blendmode = ((scrn & V_BLENDMASK) >> V_BLENDSHIFT);

	fixed_t col, ofs, colfrac, rowfrac, fdup, vdup;
	INT32 dup;
	column_t *column;
	UINT8 *desttop, *dest, *deststart, *destend;
	const UINT8 *source, *deststop;
	fixed_t pwidth;  
	fixed_t offx = 0;  

	UINT8 perplayershuffle = 0;

	if (rendermode == render_none)
		return;

#ifdef HWRENDER

	if (rendermode == render_opengl)
	{
		HWR_DrawStretchyFixedPatch(patch, x, y, pscale, vscale, scrn, colormap);
		return;
	}
#endif

	patchdrawfunc = standardpdraw;

	v_translevel = NULL;
	if (alphalevel || blendmode)
	{
		if (alphalevel == 10)  
			alphalevel = hudminusalpha[st_translucency];
		else if (alphalevel == 11)  
			alphalevel = 10 - st_translucency;
		else if (alphalevel == 12)  
			alphalevel = hudplusalpha[st_translucency];

		if (alphalevel >= 10)
			return;  

		if (alphalevel || blendmode)
		{
			v_translevel = R_GetBlendTable(blendmode+1, alphalevel);
			patchdrawfunc = translucentpdraw;
		}
	}

	v_colormap = NULL;
	if (colormap)
	{
		v_colormap = colormap;
		patchdrawfunc = (v_translevel) ? transmappedpdraw : mappedpdraw;
	}

	dup = vid.dup;
	if (scrn & V_SCALEPATCHMASK) switch (scrn & V_SCALEPATCHMASK)
	{
		case V_NOSCALEPATCH:
			dup = 1;
			break;
		case V_SMALLSCALEPATCH:
			dup = vid.smalldup;
			break;
		case V_MEDSCALEPATCH:
			dup = vid.meddup;
			break;
	}

	fdup = vdup = pscale * dup;
	if (vscale != pscale)
		vdup = vscale * dup;
	colfrac = FixedDiv(FRACUNIT, fdup);
	rowfrac = FixedDiv(FRACUNIT, vdup);

	{
		fixed_t offsetx = 0, offsety = 0;


		if (scrn & V_FLIP)
			offsetx = FixedMul((patch->width - patch->leftoffset)<<FRACBITS, pscale) + 1;
		else
			offsetx = FixedMul(patch->leftoffset<<FRACBITS, pscale);


		offsety = FixedMul(patch->topoffset<<FRACBITS, vscale);


		x -= offsetx;
		y -= offsety;
	}

	if (splitscreen && (scrn & V_PERPLAYER))
	{
		fixed_t adjusty = ((scrn & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)<<(FRACBITS-1);
		vdup >>= 1;
		rowfrac <<= 1;
		y >>= 1;
#ifdef QUADS
		if (splitscreen > 1)  
		{
			fixed_t adjustx = ((scrn & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)<<(FRACBITS-1));
			fdup >>= 1;
			colfrac <<= 1;
			x >>= 1;
			if (stplyr == &players[displayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(scrn & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				scrn &= ~V_SNAPTOBOTTOM|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(scrn & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				scrn &= ~V_SNAPTOBOTTOM|V_SNAPTOLEFT;
			}
			else if (stplyr == &players[thirddisplayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(scrn & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				y += adjusty;
				scrn &= ~V_SNAPTOTOP|V_SNAPTORIGHT;
			}
			else  
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(scrn & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				y += adjusty;
				scrn &= ~V_SNAPTOTOP|V_SNAPTOLEFT;
			}
		}
		else
#endif

		{
			if (stplyr == &players[displayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle = 1;
				scrn &= ~V_SNAPTOBOTTOM;
			}
			else  
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle = 2;
				y += adjusty;
				scrn &= ~V_SNAPTOTOP;
			}
		}
	}

	desttop = screens[scrn&V_PARAMMASK];

	if (!desttop)
		return;

	deststop = desttop + vid.rowbytes * vid.height;

	if (scrn & V_NOSCALESTART)
	{
		x >>= FRACBITS;
		y >>= FRACBITS;
		desttop += (y*vid.width) + x;
	}
	else
	{
		x *= dup;
		y *= dup;
		x >>= FRACBITS;
		y >>= FRACBITS;


		if (!(scrn & V_SCALEPATCHMASK))
		{

			if (!v_translevel && x == 0 && patch->width == BASEVIDWIDTH && y == 0 && patch->height == BASEVIDHEIGHT)
			{
				column = &patch->columns[0];
				if (column->num_posts && !column->posts[0].topdelta)
				{
					source = column->pixels;
					V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, source[0]);
				}
			}

			if (vid.width != BASEVIDWIDTH * dup)
			{


				if (scrn & V_SNAPTORIGHT)
					x += (vid.width - (BASEVIDWIDTH * dup));
				else if (!(scrn & V_SNAPTOLEFT))
					x += (vid.width - (BASEVIDWIDTH * dup)) / 2;
				if (perplayershuffle & 4)
					x -= (vid.width - (BASEVIDWIDTH * dup)) / 4;
				else if (perplayershuffle & 8)
					x += (vid.width - (BASEVIDWIDTH * dup)) / 4;
			}
			if (vid.height != BASEVIDHEIGHT * dup)
			{

				if (scrn & V_SNAPTOBOTTOM)
					y += (vid.height - (BASEVIDHEIGHT * dup));
				else if (!(scrn & V_SNAPTOTOP))
					y += (vid.height - (BASEVIDHEIGHT * dup)) / 2;
				if (perplayershuffle & 1)
					y -= (vid.height - (BASEVIDHEIGHT * dup)) / 4;
				else if (perplayershuffle & 2)
					y += (vid.height - (BASEVIDHEIGHT * dup)) / 4;
			}
		}

		desttop += (y*vid.width) + x;
	}

	if (pscale != FRACUNIT)  
	{
		pwidth = patch->width<<FRACBITS;
		pwidth = FixedMul(pwidth, pscale);
		pwidth *= dup;
		pwidth >>= FRACBITS;
	}
	else
		pwidth = patch->width * dup;

	deststart = desttop;
	destend = desttop + pwidth;

	for (col = 0; (col>>FRACBITS) < patch->width; col += colfrac, ++offx, desttop++)
	{
		if (scrn & V_FLIP)  
		{
			if (x+pwidth-offx < 0)  
				break;
			if (x+pwidth-offx >= vid.width)  
				continue;
		}
		else
		{
			if (x+offx < 0)  
				continue;
			if (x+offx >= vid.width)  
				break;
		}

		column = &patch->columns[col>>FRACBITS];

		for (unsigned i = 0; i < column->num_posts; i++)
		{
			post_t *post = &column->posts[i];
			source = column->pixels + post->data_offset;
			dest = desttop;
			if (scrn & V_FLIP)
				dest = deststart + (destend - desttop);
			dest += FixedInt(FixedMul(post->topdelta<<FRACBITS,vdup))*vid.width;

			for (ofs = 0; dest < deststop && (size_t)(ofs>>FRACBITS) < post->length; ofs += rowfrac)
			{
				if (dest >= screens[scrn&V_PARAMMASK])  
					*dest = patchdrawfunc(dest, source, ofs);
				dest += vid.width;
			}
		}
	}
}


void V_DrawCroppedPatch(fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 scrn, patch_t *patch, const UINT8 *colormap, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h)
{
	UINT8 (*patchdrawfunc)(const UINT8*, const UINT8*, fixed_t);
	UINT32 alphalevel = ((scrn & V_ALPHAMASK) >> V_ALPHASHIFT);
	UINT32 blendmode = ((scrn & V_BLENDMASK) >> V_BLENDSHIFT);


	fixed_t col, ofs, colfrac, rowfrac, fdup, vdup;
	INT32 dup;
	column_t *column;
	UINT8 *desttop, *dest;
	const UINT8 *source, *deststop;

	UINT8 perplayershuffle = 0;

	if (rendermode == render_none)
		return;

#ifdef HWRENDER

	if (rendermode == render_opengl)
	{
		HWR_DrawCroppedPatch(patch,x,y,pscale,vscale,scrn,colormap,sx,sy,w,h);
		return;
	}
#endif

	patchdrawfunc = standardpdraw;

	v_translevel = NULL;
	if (alphalevel || blendmode)
	{
		if (alphalevel == 10)  
			alphalevel = hudminusalpha[st_translucency];
		else if (alphalevel == 11)  
			alphalevel = 10 - st_translucency;
		else if (alphalevel == 12)  
			alphalevel = hudplusalpha[st_translucency];

		if (alphalevel >= 10)
			return;  

		if (alphalevel || blendmode)
		{
			v_translevel = R_GetBlendTable(blendmode+1, alphalevel);
			patchdrawfunc = translucentpdraw;
		}
	}

	v_colormap = NULL;
	if (colormap)
	{
		v_colormap = colormap;
		patchdrawfunc = (v_translevel) ? transmappedpdraw : mappedpdraw;
	}

	dup = vid.dup;
	if (scrn & V_SCALEPATCHMASK) switch (scrn & V_SCALEPATCHMASK)
	{
		case V_NOSCALEPATCH:
			dup = 1;
			break;
		case V_SMALLSCALEPATCH:
			dup = vid.smalldup;
			break;
		case V_MEDSCALEPATCH:
			dup = vid.meddup;
			break;
	}

	fdup = vdup = pscale * dup;
	if (vscale != pscale)
		vdup = vscale * dup;
	colfrac = FixedDiv(FRACUNIT, fdup);
	rowfrac = FixedDiv(FRACUNIT, vdup);

	x -= FixedMul(patch->leftoffset<<FRACBITS, pscale);
	y -= FixedMul(patch->topoffset<<FRACBITS, vscale);

	if (splitscreen && (scrn & V_PERPLAYER))
	{
		fixed_t adjusty = ((scrn & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)<<(FRACBITS-1);
		vdup >>= 1;
		rowfrac <<= 1;
		y >>= 1;
#ifdef QUADS
		if (splitscreen > 1)  
		{
			fixed_t adjustx = ((scrn & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)<<(FRACBITS-1));
			fdup >>= 1;
			colfrac <<= 1;
			x >>= 1;
			if (stplyr == &players[displayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(scrn & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				scrn &= ~V_SNAPTOBOTTOM|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(scrn & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				scrn &= ~V_SNAPTOBOTTOM|V_SNAPTOLEFT;
			}
			else if (stplyr == &players[thirddisplayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(scrn & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				y += adjusty;
				scrn &= ~V_SNAPTOTOP|V_SNAPTORIGHT;
			}
			else  
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(scrn & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				y += adjusty;
				scrn &= ~V_SNAPTOTOP|V_SNAPTOLEFT;
			}
		}
		else
#endif

		{
			if (stplyr == &players[displayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				scrn &= ~V_SNAPTOBOTTOM;
			}
			else  
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				y += adjusty;
				scrn &= ~V_SNAPTOTOP;
			}
		}
	}

	desttop = screens[scrn&V_PARAMMASK];

	if (!desttop)
		return;

	deststop = desttop + vid.rowbytes * vid.height;

	if (scrn & V_NOSCALESTART)
	{
		x >>= FRACBITS;
		y >>= FRACBITS;
		desttop += (y*vid.width) + x;
	}
	else
	{
		x *= dup;
		y *= dup;
		x >>= FRACBITS;
		y >>= FRACBITS;


		if (!(scrn & V_SCALEPATCHMASK))
		{



			if (vid.width != BASEVIDWIDTH * dup)
			{


				if (scrn & V_SNAPTORIGHT)
					x += (vid.width - (BASEVIDWIDTH * dup));
				else if (!(scrn & V_SNAPTOLEFT))
					x += (vid.width - (BASEVIDWIDTH * dup)) / 2;
				if (perplayershuffle & 4)
					x -= (vid.width - (BASEVIDWIDTH * dup)) / 4;
				else if (perplayershuffle & 8)
					x += (vid.width - (BASEVIDWIDTH * dup)) / 4;
			}
			if (vid.height != BASEVIDHEIGHT * dup)
			{

				if (scrn & V_SNAPTOBOTTOM)
					y += (vid.height - (BASEVIDHEIGHT * dup));
				else if (!(scrn & V_SNAPTOTOP))
					y += (vid.height - (BASEVIDHEIGHT * dup)) / 2;
				if (perplayershuffle & 1)
					y -= (vid.height - (BASEVIDHEIGHT * dup)) / 4;
				else if (perplayershuffle & 2)
					y += (vid.height - (BASEVIDHEIGHT * dup)) / 4;
			}
		}

		desttop += (y*vid.width) + x;
	}


	if (splitscreen && (scrn & V_PERPLAYER))
	{
#ifdef QUADS
		if (splitscreen > 1)  
		{
			#error Auto-cropping doesnt take quadscreen into account! Fix it!


		}
		else
#endif

		{
			if (stplyr == &players[displayplayer])  
			{

				deststop -= vid.rowbytes * (vid.height>>1);
			}
			else  
			{
				if (y < (vid.height>>1))  
				{
					sy += ((vid.height>>1) - y) * rowfrac;  
					h -= ((vid.height>>1) - y) * rowfrac;  
					desttop += ((vid.height>>1) - y) * vid.width;  
				}
			}
		}
	}

	for (col = sx; (col>>FRACBITS) < patch->width && (col - sx) < w; col += colfrac, ++x, desttop++)
	{
		if (x < 0)  
			continue;
		if (x >= vid.width)  
			break;

		column = &patch->columns[col>>FRACBITS];

		for (unsigned i = 0; i < column->num_posts; i++)
		{
			post_t *post = &column->posts[i];
			INT32 topdelta = post->topdelta;
			source = column->pixels + post->data_offset;
			dest = desttop;
			if ((topdelta<<FRACBITS)-sy > 0)
			{
				dest += FixedInt(FixedMul((topdelta<<FRACBITS)-sy,vdup))*vid.width;
				ofs = 0;
			}
			else
				ofs = sy-(topdelta<<FRACBITS);

			for (; dest < deststop && (size_t)(ofs>>FRACBITS) < post->length && ((ofs - sy) + (topdelta<<FRACBITS)) < h; ofs += rowfrac)
			{
				if (dest >= screens[scrn&V_PARAMMASK])  
					*dest = patchdrawfunc(dest, source, ofs);
				dest += vid.width;
			}
		}
	}
}





void V_DrawContinueIcon(INT32 x, INT32 y, INT32 flags, INT32 skinnum, UINT16 skincolor)
{
	if (skinnum >= 0 && skinnum < numskins && skins[skinnum]->sprites[SPR2_XTRA].numframes > XTRA_CONTINUE)
	{
		spritedef_t *sprdef = &skins[skinnum]->sprites[SPR2_XTRA];
		spriteframe_t *sprframe = &sprdef->spriteframes[XTRA_CONTINUE];
		patch_t *patch = W_CachePatchNum(sprframe->lumppat[0], PU_PATCH);
		const UINT8 *colormap = R_GetTranslationColormap(skinnum, skincolor, GTC_CACHE);

		V_DrawMappedPatch(x, y, flags, patch, colormap);
	}
	else
		V_DrawScaledPatch(x - 10, y - 14, flags, W_CachePatchName("CONTINS", PU_PATCH));
}





void V_DrawBlock(INT32 x, INT32 y, INT32 scrn, INT32 width, INT32 height, const UINT8 *src)
{
	UINT8 *dest;
	const UINT8 *deststop;

#ifdef RANGECHECK
	if (x < 0 || x + width > vid.width || y < 0 || y + height > vid.height || (unsigned)scrn > 4)
		I_Error("Bad V_DrawBlock");
#endif

	dest = screens[scrn] + y*vid.width + x;
	deststop = screens[scrn] + vid.rowbytes * vid.height;

	while (height--)
	{
		M_Memcpy(dest, src, width);

		src += width;
		dest += vid.width;
		if (dest > deststop)
			return;
	}
}




void V_DrawFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c)
{
	UINT8 *dest;
	const UINT8 *deststop;
	UINT32 alphalevel = ((c & V_ALPHAMASK) >> V_ALPHASHIFT);
	UINT32 blendmode = ((c & V_BLENDMASK) >> V_BLENDSHIFT);

	UINT8 perplayershuffle = 0;

	if (rendermode == render_none)
		return;

	v_translevel = NULL;
	if (alphalevel || blendmode)
	{
		if (alphalevel == 10)  
			alphalevel = hudminusalpha[st_translucency];
		else if (alphalevel == 11)  
			alphalevel = 10 - st_translucency;
		else if (alphalevel == 12)  
			alphalevel = hudplusalpha[st_translucency];

		if (alphalevel >= 10)
			return;  

		if (alphalevel || blendmode)
			v_translevel = R_GetBlendTable(blendmode+1, alphalevel);
	}


#ifdef HWRENDER

	if (rendermode == render_opengl)
	{
		HWR_DrawFill(x, y, w, h, c);
		return;
	}
#endif



	if (splitscreen && (c & V_PERPLAYER))
	{
		fixed_t adjusty = ((c & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)>>1;
		h >>= 1;
		y >>= 1;
#ifdef QUADS
		if (splitscreen > 1)  
		{
			fixed_t adjustx = ((c & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)>>1;
			w >>= 1;
			x >>= 1;
			if (stplyr == &players[displayplayer])
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(c & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				c &= ~V_SNAPTOBOTTOM|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(c & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				c &= ~V_SNAPTOBOTTOM|V_SNAPTOLEFT;
			}
			else if (stplyr == &players[thirddisplayplayer])
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(c & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				y += adjusty;
				c &= ~V_SNAPTOTOP|V_SNAPTORIGHT;
			}
			else  
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(c & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				y += adjusty;
				c &= ~V_SNAPTOTOP|V_SNAPTOLEFT;
			}
		}
		else
#endif

		{
			if (stplyr == &players[displayplayer])
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				c &= ~V_SNAPTOBOTTOM;
			}
			else  
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				y += adjusty;
				c &= ~V_SNAPTOTOP;
			}
		}
	}

	if (!(c & V_NOSCALESTART))
	{
		if (x == 0 && y == 0 && w == BASEVIDWIDTH && h == BASEVIDHEIGHT)
		{  
			memset(screens[0], (c&255), vid.width * vid.height * vid.bpp);
			return;
		}

		x *= vid.dup;
		y *= vid.dup;
		w *= vid.dup;
		h *= vid.dup;


		if (vid.width != BASEVIDWIDTH * vid.dup)
		{


			if (c & V_SNAPTORIGHT)
				x += (vid.width - (BASEVIDWIDTH * vid.dup));
			else if (!(c & V_SNAPTOLEFT))
				x += (vid.width - (BASEVIDWIDTH * vid.dup)) / 2;
			if (perplayershuffle & 4)
				x -= (vid.width - (BASEVIDWIDTH * vid.dup)) / 4;
			else if (perplayershuffle & 8)
				x += (vid.width - (BASEVIDWIDTH * vid.dup)) / 4;
		}
		if (vid.height != BASEVIDHEIGHT * vid.dup)
		{

			if (c & V_SNAPTOBOTTOM)
				y += (vid.height - (BASEVIDHEIGHT * vid.dup));
			else if (!(c & V_SNAPTOTOP))
				y += (vid.height - (BASEVIDHEIGHT * vid.dup)) / 2;
			if (perplayershuffle & 1)
				y -= (vid.height - (BASEVIDHEIGHT * vid.dup)) / 4;
			else if (perplayershuffle & 2)
				y += (vid.height - (BASEVIDHEIGHT * vid.dup)) / 4;
		}
	}

	if (x >= vid.width || y >= vid.height)
		return;  
	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (y < 0)
	{
		h += y;
		y = 0;
	}

	if (w <= 0 || h <= 0)
		return;  
	if (x + w > vid.width)
		w = vid.width - x;
	if (y + h > vid.height)
		h = vid.height - y;

	dest = screens[0] + y*vid.width + x;
	deststop = screens[0] + vid.rowbytes * vid.height;

	c &= 255;


	if (alphalevel)
	{
		v_translevel += c<<8;
		for (;(--h >= 0) && dest < deststop; dest += vid.width)
		{
			for (x = 0; x < w; x++)
				dest[x] = v_translevel[dest[x]];
		}
	}
	else
	{
		for (;(--h >= 0) && dest < deststop; dest += vid.width)
			memset(dest, c, w * vid.bpp);
	}
}

#ifdef HWRENDER

static UINT32 V_GetHWConsBackColor(void)
{
	UINT8 r, g, b;
	switch (cons_backcolor.value)
	{
		case 0:		r = 0xff; g = 0xff; b = 0xff;	break; 	 
		case 1:		r = 0x80; g = 0x80; b = 0x80;	break; 	 
		case 2:		r = 0xde; g = 0xb8; b = 0x87;	break;	 
		case 3:		r = 0x40; g = 0x20; b = 0x10;	break; 	 
		case 4:		r = 0xfa; g = 0x80; b = 0x72;	break; 	 
		case 5:		r = 0xff; g = 0x69; b = 0xb4;	break; 	 
		case 6:		r = 0xff; g = 0x00; b = 0x00;	break; 	 
		case 7:		r = 0xff; g = 0xd6; b = 0x83;	break;	 
		case 8:		r = 0xff; g = 0x80; b = 0x00;	break; 	 
		case 9:		r = 0xda; g = 0xa5; b = 0x20;	break; 	 
		case 10:	r = 0x80; g = 0x80; b = 0x00;	break; 	 
		case 11:	r = 0x00; g = 0xff; b = 0x00;	break; 	 
		case 12:	r = 0x00; g = 0x80; b = 0x00;	break; 	 
		case 13:	r = 0x40; g = 0x80; b = 0xff;	break; 	 
		case 14:	r = 0x46; g = 0x82; b = 0xb4;	break; 	 
		case 15:	r = 0x1e; g = 0x90; b = 0xff;	break;	 
		case 16:	r = 0x00; g = 0x00; b = 0xff;	break; 	 
		case 17:	r = 0xff; g = 0x00; b = 0xff;	break; 	 
		case 18:	r = 0xee; g = 0x82; b = 0xee;	break; 	 

		default:	r = 0x00; g = 0x80; b = 0x00;	break;
	}
	V_CubeApply(&r, &g, &b);
	return (r << 24) | (g << 16) | (b << 8);
}
#endif





void V_DrawFillConsoleMap(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c)
{
	UINT8 *dest;
	const UINT8 *deststop;
	INT32 u;
	UINT8 *fadetable;
	UINT32 alphalevel = 0;
	UINT8 perplayershuffle = 0;

	if (rendermode == render_none)
		return;

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		UINT32 hwcolor = V_GetHWConsBackColor();
		HWR_DrawConsoleFill(x, y, w, h, c, hwcolor);	 
		return;
	}
#endif

	if ((alphalevel = ((c & V_ALPHAMASK) >> V_ALPHASHIFT)))
	{
		if (alphalevel == 10)  
			alphalevel = hudminusalpha[st_translucency];
		else if (alphalevel == 11)  
			alphalevel = 10 - st_translucency;
		else if (alphalevel == 12)  
			alphalevel = hudplusalpha[st_translucency];

		if (alphalevel >= 10)
			return;  
	}

	if (splitscreen && (c & V_PERPLAYER))
	{
		fixed_t adjusty = ((c & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)>>1;
		h >>= 1;
		y >>= 1;
#ifdef QUADS
		if (splitscreen > 1)  
		{
			fixed_t adjustx = ((c & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)>>1;
			w >>= 1;
			x >>= 1;
			if (stplyr == &players[displayplayer])
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(c & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				c &= ~V_SNAPTOBOTTOM|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(c & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				c &= ~V_SNAPTOBOTTOM|V_SNAPTOLEFT;
			}
			else if (stplyr == &players[thirddisplayplayer])
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(c & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				y += adjusty;
				c &= ~V_SNAPTOTOP|V_SNAPTORIGHT;
			}
			else  
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(c & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				y += adjusty;
				c &= ~V_SNAPTOTOP|V_SNAPTOLEFT;
			}
		}
		else
#endif

		{
			if (stplyr == &players[displayplayer])
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				c &= ~V_SNAPTOBOTTOM;
			}
			else  
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				y += adjusty;
				c &= ~V_SNAPTOTOP;
			}
		}
	}

	if (!(c & V_NOSCALESTART))
	{
		x *= vid.dup;
		y *= vid.dup;
		w *= vid.dup;
		h *= vid.dup;


		if (vid.width != BASEVIDWIDTH * vid.dup)
		{


			if (c & V_SNAPTORIGHT)
				x += (vid.width - (BASEVIDWIDTH * vid.dup));
			else if (!(c & V_SNAPTOLEFT))
				x += (vid.width - (BASEVIDWIDTH * vid.dup)) / 2;
			if (perplayershuffle & 4)
				x -= (vid.width - (BASEVIDWIDTH * vid.dup)) / 4;
			else if (perplayershuffle & 8)
				x += (vid.width - (BASEVIDWIDTH * vid.dup)) / 4;
		}
		if (vid.height != BASEVIDHEIGHT * vid.dup)
		{

			if (c & V_SNAPTOBOTTOM)
				y += (vid.height - (BASEVIDHEIGHT * vid.dup));
			else if (!(c & V_SNAPTOTOP))
				y += (vid.height - (BASEVIDHEIGHT * vid.dup)) / 2;
			if (perplayershuffle & 1)
				y -= (vid.height - (BASEVIDHEIGHT * vid.dup)) / 4;
			else if (perplayershuffle & 2)
				y += (vid.height - (BASEVIDHEIGHT * vid.dup)) / 4;
		}
	}

	if (x >= vid.width || y >= vid.height)
		return;  
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}

	if (w <= 0 || h <= 0)
		return;  
	if (x + w > vid.width)
		w = vid.width-x;
	if (y + h > vid.height)
		h = vid.height-y;

	dest = screens[0] + y*vid.width + x;
	deststop = screens[0] + vid.rowbytes * vid.height;

	c &= 255;


	if (alphalevel)
	{
		fadetable = R_GetTranslucencyTable(alphalevel) + (c*256);
		for (;(--h >= 0) && dest < deststop; dest += vid.width)
		{
			u = 0;
			while (u < w)
			{
				dest[u] = fadetable[consolebgmap[dest[u]]];
				u++;
			}
		}
	}
	else
	{
		for (;(--h >= 0) && dest < deststop; dest += vid.width)
		{
			u = 0;
			while (u < w)
			{
				dest[u] = consolebgmap[dest[u]];
				u++;
			}
		}
	}
}









void V_DrawFadeFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c, UINT16 color, UINT8 strength)
{
	UINT8 *dest;
	const UINT8 *deststop;
	INT32 u;
	UINT8 *fadetable;
	UINT8 perplayershuffle = 0;

	if (rendermode == render_none)
		return;

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{

		HWR_DrawFadeFill(x, y, w, h, c, color, strength);  
		return;
	}
#endif

	if (splitscreen && (c & V_PERPLAYER))
	{
		fixed_t adjusty = ((c & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)>>1;
		h >>= 1;
		y >>= 1;
#ifdef QUADS
		if (splitscreen > 1)  
		{
			fixed_t adjustx = ((c & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)>>1;
			w >>= 1;
			x >>= 1;
			if (stplyr == &players[displayplayer])
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(c & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				c &= ~V_SNAPTOBOTTOM|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(c & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				c &= ~V_SNAPTOBOTTOM|V_SNAPTOLEFT;
			}
			else if (stplyr == &players[thirddisplayplayer])
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(c & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				y += adjusty;
				c &= ~V_SNAPTOTOP|V_SNAPTORIGHT;
			}
			else  
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(c & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				y += adjusty;
				c &= ~V_SNAPTOTOP|V_SNAPTOLEFT;
			}
		}
		else
#endif

		{
			if (stplyr == &players[displayplayer])
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				c &= ~V_SNAPTOBOTTOM;
			}
			else  
			{
				if (!(c & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				y += adjusty;
				c &= ~V_SNAPTOTOP;
			}
		}
	}

	if (!(c & V_NOSCALESTART))
	{
		x *= vid.dup;
		y *= vid.dup;
		w *= vid.dup;
		h *= vid.dup;


		if (vid.width != BASEVIDWIDTH * vid.dup)
		{


			if (c & V_SNAPTORIGHT)
				x += (vid.width - (BASEVIDWIDTH * vid.dup));
			else if (!(c & V_SNAPTOLEFT))
				x += (vid.width - (BASEVIDWIDTH * vid.dup)) / 2;
			if (perplayershuffle & 4)
				x -= (vid.width - (BASEVIDWIDTH * vid.dup)) / 4;
			else if (perplayershuffle & 8)
				x += (vid.width - (BASEVIDWIDTH * vid.dup)) / 4;
		}
		if (vid.height != BASEVIDHEIGHT * vid.dup)
		{

			if (c & V_SNAPTOBOTTOM)
				y += (vid.height - (BASEVIDHEIGHT * vid.dup));
			else if (!(c & V_SNAPTOTOP))
				y += (vid.height - (BASEVIDHEIGHT * vid.dup)) / 2;
			if (perplayershuffle & 1)
				y -= (vid.height - (BASEVIDHEIGHT * vid.dup)) / 4;
			else if (perplayershuffle & 2)
				y += (vid.height - (BASEVIDHEIGHT * vid.dup)) / 4;
		}
	}

	if (x >= vid.width || y >= vid.height)
		return;  
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}

	if (w <= 0 || h <= 0)
		return;  
	if (x + w > vid.width)
		w = vid.width-x;
	if (y + h > vid.height)
		h = vid.height-y;

	dest = screens[0] + y*vid.width + x;
	deststop = screens[0] + vid.rowbytes * vid.height;

	c &= 255;

	fadetable = ((color & 0xFF00)  
		? ((UINT8 *)colormaps + strength*256)  
		: ((UINT8 *)R_GetTranslucencyTable((9-strength)+1) + color*256));  
	for (;(--h >= 0) && dest < deststop; dest += vid.width)
	{
		u = 0;
		while (u < w)
		{
			dest[u] = fadetable[dest[u]];
			u++;
		}
	}
}




void V_DrawFlatFill(INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatnum)
{
	INT32 u, v;
	fixed_t dx, dy, xfrac, yfrac;
	const UINT8 *src, *deststop;
	UINT8 *flat, *dest;
	size_t lflatsize, flatshift;

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_DrawFlatFill(x, y, w, h, flatnum);
		return;
	}
#endif

	lflatsize = R_GetFlatSize(W_LumpLength(flatnum));
	flatshift = R_GetFlatBits(lflatsize);

	flat = W_CacheLumpNum(flatnum, PU_CACHE);

	dest = screens[0] + y*vid.dup*vid.width + x*vid.dup;
	deststop = screens[0] + vid.rowbytes * vid.height;


	if (vid.width != BASEVIDWIDTH * vid.dup)
	{


		dest += (vid.width - (BASEVIDWIDTH * vid.dup)) / 2;
	}
	if (vid.height != BASEVIDHEIGHT * vid.dup)
	{

		dest += (vid.height - (BASEVIDHEIGHT * vid.dup)) * vid.width / 2;
	}

	w *= vid.dup;
	h *= vid.dup;

	dx = FixedDiv(FRACUNIT, vid.dup<<(FRACBITS-2));
	dy = FixedDiv(FRACUNIT, vid.dup<<(FRACBITS-2));

	yfrac = 0;
	for (v = 0; v < h; v++, dest += vid.width)
	{
		xfrac = 0;
		src = flat + (((yfrac>>FRACBITS) & (lflatsize - 1)) << flatshift);
		for (u = 0; u < w; u++)
		{
			if (&dest[u] > deststop)
				return;
			dest[u] = src[(xfrac>>FRACBITS)&(lflatsize-1)];
			xfrac += dx;
		}
		yfrac += dy;
	}
}




void V_DrawPatchFill(patch_t *pat)
{
	INT32 x, y, pw = pat->width * vid.dup, ph = pat->height * vid.dup;

	for (x = 0; x < vid.width; x += pw)
	{
		for (y = 0; y < vid.height; y += ph)
			V_DrawScaledPatch(x, y, V_NOSCALESTART, pat);
	}
}










void V_DrawFadeScreen(UINT16 color, UINT8 strength)
{
#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_FadeScreenMenuBack(color, strength);
		return;
	}
#endif

	{
		const UINT8 *fadetable = ((color & 0xFF00)  
		? ((UINT8 *)(((color & 0x0F00) == 0x0A00) ? fadecolormap  
		: (((color & 0x0F00) == 0x0B00) ? fadecolormap + (256 * FADECOLORMAPROWS)  
		: colormaps)) + strength*256)  
		: ((UINT8 *)R_GetTranslucencyTable((9-strength)+1) + color*256));  
		const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;
		UINT8 *buf = screens[0];



		for (; buf < deststop; ++buf)
			*buf = fadetable[*buf];
	}
}


void V_DrawFadeConsBack(INT32 plines)
{
	UINT8 *deststop, *buf;

#ifdef HWRENDER  
	if (rendermode == render_opengl)
	{
		UINT32 hwcolor = V_GetHWConsBackColor();
		HWR_DrawConsoleBack(hwcolor, plines);
		return;
	}
#endif



	deststop = screens[0] + vid.rowbytes * min(plines, vid.height);
	for (buf = screens[0]; buf < deststop; ++buf)
		*buf = consolebgmap[*buf];
}


void V_DrawPromptBack(INT32 boxheight, INT32 color)
{
	UINT8 *deststop, *buf;

	if (color >= 256 && color < 512)
	{
		if (boxheight < 0)
			boxheight = -boxheight;
		else  
			boxheight = ((boxheight * 4) + (boxheight/2)*5);
		V_DrawFill((BASEVIDWIDTH-(vid.width/vid.dup))/2, BASEVIDHEIGHT-boxheight, (vid.width/vid.dup),boxheight, (color-256)|V_SNAPTOBOTTOM);
		return;
	}

	boxheight *= vid.dup;

	if (color == INT32_MAX)
		color = cons_backcolor.value;

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		UINT32 hwcolor;
		switch (color)
		{
			case 0:		hwcolor = 0xffffff00;	break; 	 
			case 1:		hwcolor = 0x00000000;	break; 	 
			case 2:		hwcolor = 0xdeb88700;	break;	 
			case 3:		hwcolor = 0x40201000;	break; 	 
			case 4:		hwcolor = 0xfa807200;	break; 	 
			case 5:		hwcolor = 0xff69b400;	break; 	 
			case 6:		hwcolor = 0xff000000;	break; 	 
			case 7:		hwcolor = 0xffd68300;	break;	 
			case 8:		hwcolor = 0xff800000;	break; 	 
			case 9:		hwcolor = 0xdaa52000;	break; 	 
			case 10:	hwcolor = 0x80800000;	break; 	 
			case 11:	hwcolor = 0x00ff0000;	break; 	 
			case 12:	hwcolor = 0x00800000;	break; 	 
			case 13:	hwcolor = 0x4080ff00;	break; 	 
			case 14:	hwcolor = 0x4682b400;	break; 	 
			case 15:	hwcolor = 0x1e90ff00;	break;	 
			case 16:	hwcolor = 0x0000ff00;	break; 	 
			case 17:	hwcolor = 0xff00ff00;	break; 	 
			case 18:	hwcolor = 0xee82ee00;	break; 	 

			default:	hwcolor = 0x00800000;	break;
		}
		HWR_DrawTutorialBack(hwcolor, boxheight);
		return;
	}
#endif

	CON_SetupBackColormapEx(color, true);



	buf = deststop = screens[0] + vid.rowbytes * vid.height;
	if (boxheight < 0)
		buf += vid.rowbytes * boxheight;
	else  
		buf -= vid.rowbytes * ((boxheight * 4) + (boxheight/2)*5);
	for (; buf < deststop; ++buf)
		*buf = promptbgmap[*buf];
}



UINT8 *V_GetStringColormap(INT32 colorflags)
{
	switch ((colorflags & V_CHARCOLORMASK) >> V_CHARCOLORSHIFT)
	{
	case  1:  
		return magentamap;
	case  2:  
		return yellowmap;
	case  3:  
		return lgreenmap;
	case  4:  
		return bluemap;
	case  5:  
		return redmap;
	case  6:  
		return graymap;
	case  7:  
		return orangemap;
	case  8:  
		return skymap;
	case  9:  
		return purplemap;
	case 10:  
		return aquamap;
	case 11:  
		return peridotmap;
	case 12:  
		return azuremap;
	case 13:  
		return brownmap;
	case 14:  
		return rosymap;
	case 15:  
		return invertmap;
	default:  
		return NULL;
	}
}



void V_DrawFontCharacter(INT32 x, INT32 y, INT32 c, boolean lowercaseallowed, fixed_t scale, UINT8 *colormap, fontdef_t font)
{
	INT32 w, flags;
	const UINT8 *color = colormap ? colormap : V_GetStringColormap(c);

	flags = c & ~(V_CHARCOLORMASK | V_PARAMMASK);
	c &= 0x7f;
	c = (lowercaseallowed ? c : toupper(c)) - FONTSTART;
	if (c < 0 || c >= FONTSIZE || !font.chars[c])
		return;

	w = FixedMul(font.chars[c]->width / 2, scale);	 
	if (x + w > vid.width)
		return;

	V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, scale, flags, font.chars[c], color);
}



char *V_FontWordWrap(INT32 x, INT32 w, INT32 option, fixed_t scale, const char *string, fontdef_t font)
{
	int c;
	size_t slen, chw, i, lastusablespace = 0;
	char *newstring = Z_StrDup(string);
	INT32 spacewidth = font.spacewidth, charwidth = 0;

	slen = strlen(string);

	if (w == 0)
		w = BASEVIDWIDTH;
	w -= x;
	x = 0;

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = font.charwidth;
			 
		case V_OLDSPACING:
			charwidth = font.charwidth;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 6;
		default:
			break;
	}

	for (i = 0; i < slen; ++i)
	{
		c = newstring[i];
		if ((UINT8)c & 0x80)  
			continue;

		if (c == '\n')
		{
			x = 0;
			lastusablespace = 0;
			continue;
		}

		c = (option & V_ALLOWLOWERCASE ? c : toupper(c)) - FONTSTART;
		if (c < 0 || c >= FONTSIZE || !font.chars[c])
		{
			chw = spacewidth;
			lastusablespace = i;
		}
		else
			chw = (charwidth ? charwidth : font.chars[c]->width);

		x +=  FixedMul(chw, scale);

		if (lastusablespace != 0 && x > w)
		{
			newstring[lastusablespace] = '\n';
			i = lastusablespace;
			lastusablespace = x = 0;
		}
	}
	return newstring;
}



void V_DrawFontString(INT32 x, INT32 y, INT32 option, fixed_t pscale, fixed_t vscale, const char *string, fontdef_t font)
{
	V_DrawFontStringAtFixed((fixed_t)x<<FRACBITS, (fixed_t)y<<FRACBITS, option, pscale, vscale, string, font);
}

void V_DrawAlignedFontString(INT32 x, INT32 y, INT32 option, fixed_t pscale, fixed_t vscale, const char *string, fontdef_t font, enum string_align align)
{
	V_DrawAlignedFontStringAtFixed((fixed_t)x<<FRACBITS, (fixed_t)y<<FRACBITS, option, pscale, vscale, string, font, align);
}



void V_DrawFontStringAtFixed(fixed_t x, fixed_t y, INT32 option, fixed_t pscale, fixed_t vscale, const char *string, fontdef_t font)
{
	fixed_t cx = x, cy = y;
	INT32 w, c, dupx, dupy, scrwidth, center = 0, left = 0;
	const char *ch = string;
	INT32 charflags = (option & V_CHARCOLORMASK);
	INT32 spacewidth = font.spacewidth, charwidth = 0;

	INT32 lowercase = (option & V_ALLOWLOWERCASE);
	option &= ~V_FLIP;  

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dup<<FRACBITS;
		dupy = vid.dup<<FRACBITS;
		scrwidth = vid.width;
	}
	else
	{
		dupx = pscale;
		dupy = vscale;
		scrwidth = FixedDiv(vid.width<<FRACBITS, vid.dup);
		left = (scrwidth - (BASEVIDWIDTH << FRACBITS))/2;
		scrwidth -= left;
	}

	if (option & V_NOSCALEPATCH)
		scrwidth *= vid.dup;

	switch (option & V_SPACINGMASK)  
	{
		case V_MONOSPACE:
			spacewidth = font.charwidth;
			 
		case V_OLDSPACING:
			charwidth = font.charwidth;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 6;
		default:
			break;
	}

	for (;;ch++)
	{
		if (!*ch)
			break;
		if (*ch & 0x80)  
		{

			if (!(option & V_CHARCOLORMASK))
				charflags = ((*ch & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
			continue;
		}
		if (*ch == '\n')
		{
			cx = x;
			cy += FixedMul(((option & V_RETURN8) ? 8 : font.linespacing)<<FRACBITS, dupy);
			continue;
		}

		c = (lowercase ? *ch : toupper(*ch)) - FONTSTART;
		if (c < 0 || c >= FONTSIZE || !font.chars[c])
		{
			cx += FixedMul((spacewidth<<FRACBITS), dupx);
			continue;
		}

		if (charwidth)
		{
			w = FixedMul((charwidth<<FRACBITS), dupx);
			center = w/2 - FixedMul(font.chars[c]->width<<FRACBITS, (dupx/2));
		}
		else
			w = FixedMul(font.chars[c]->width<<FRACBITS, dupx);

		if ((cx>>FRACBITS) > scrwidth)
			continue;
		if (cx+left + w < 0)  
		{
			cx += w;
			continue;
		}

		V_DrawStretchyFixedPatch(cx + center, cy, pscale, vscale, option, font.chars[c], V_GetStringColormap(charflags));

		cx += w + (font.kerning<<FRACBITS);
	}
}

void V_DrawAlignedFontStringAtFixed(fixed_t x, fixed_t y, INT32 option, fixed_t pscale, fixed_t vscale, const char *string, fontdef_t font, enum string_align align)
{
	char *text = strdup(string);
	char* line = xstrtok(text, "\n");
	fixed_t lx = x, ly = y;

	while (line)
	{
		switch(align)
		{
			case alignleft:
				lx = x;
				break;
			case aligncenter:
				lx = x - (V_FontStringWidth(line, option, font)*pscale) / 2;
				break;
			case alignright:
				lx = x - (V_FontStringWidth(line, option, font)*pscale);
				break;
		}

		V_DrawFontStringAtFixed(lx, ly, option, pscale, vscale, line, font);

		ly += FixedMul(((option & V_RETURN8) ? 8 : font.linespacing)<<FRACBITS, vscale);

		line = xstrtok(NULL, "\n");
	}
}


void V_DrawTallNum(INT32 x, INT32 y, INT32 flags, INT32 num)
{
	INT32 w = tallnum[0]->width;
	boolean neg;

	if (flags & (V_NOSCALESTART|V_NOSCALEPATCH))
		w *= vid.dup;

	if ((neg = num < 0))
		num = -num;


	do
	{
		x -= w;
		V_DrawScaledPatch(x, y, flags, tallnum[num % 10]);
		num /= 10;
	} while (num);


	if (neg)
		V_DrawScaledPatch(x - w, y, flags, tallminus);  
}



void V_DrawPaddedTallNum(INT32 x, INT32 y, INT32 flags, INT32 num, INT32 digits)
{
	INT32 w = tallnum[0]->width;

	if (flags & (V_NOSCALESTART|V_NOSCALEPATCH))
		w *= vid.dup;

	if (num < 0)
		num = -num;


	do
	{
		x -= w;
		V_DrawScaledPatch(x, y, flags, tallnum[num % 10]);
		num /= 10;
	} while (--digits);
}


void V_DrawLevelActNum(INT32 x, INT32 y, INT32 flags, UINT8 num)
{
	if (num > 99)
		return;  

	while (num > 0)
	{
		if (num > 9)  
			V_DrawScaledPatch(x + (V_LevelActNumWidth(num) - V_LevelActNumWidth(num%10)), y, flags, ttlnum[num%10]);
		else
			V_DrawScaledPatch(x, y, flags, ttlnum[num]);
		num = num/10;
	}
}


INT16 V_LevelActNumWidth(UINT8 num)
{
	INT16 result = 0;

	if (num == 0)
		result = ttlnum[num]->width;

	while (num > 0 && num <= 99)
	{
		result = result + ttlnum[num%10]->width;
		num = num/10;
	}

	return result;
}



static void V_DrawNameTagLine(INT32 x, INT32 y, INT32 option, fixed_t scale, UINT8 *basecolormap, UINT8 *outlinecolormap, const char *string)
{
	fixed_t cx, cy, w;
	INT32 c, dup, scrwidth, left = 0;
	const char *ch = string;

	if (option & V_CENTERNAMETAG)
		x -= FixedInt(FixedMul((V_NameTagWidth(string)/2)*FRACUNIT, scale));
	option &= ~V_CENTERNAMETAG;  

	cx = x<<FRACBITS;
	cy = y<<FRACBITS;

	if (option & V_NOSCALESTART)
	{
		dup = vid.dup;
		scrwidth = vid.width;
	}
	else
	{
		dup = 1;
		scrwidth = vid.width/vid.dup;
		left = (scrwidth - BASEVIDWIDTH)/2;
		scrwidth -= left;
	}

	if (option & V_NOSCALEPATCH)
		scrwidth *= vid.dup;

	for (;;ch++)
	{
		if (!*ch)
			break;
		if (*ch == '\n')
		{
			cx = x<<FRACBITS;
			cy += FixedMul((ntb_font.linespacing * dup)*FRACUNIT, scale);
			continue;
		}

		c = toupper(*ch) - FONTSTART;
		if (c < 0 || c >= FONTSIZE || !ntb_font.chars[c] || !nto_font.chars[c])
		{
			cx += FixedMul((ntb_font.spacewidth * dup)*FRACUNIT, scale);
			continue;
		}

		w = FixedMul(((ntb_font.chars[c]->width)+ntb_font.kerning * dup) * FRACUNIT, scale);

		if (FixedInt(cx) > scrwidth)
			continue;
		if (cx+(left*FRACUNIT) + w < 0)  
		{
			cx += w;
			continue;
		}

		V_DrawFixedPatch(cx, cy, scale, option, nto_font.chars[c], outlinecolormap);
		V_DrawFixedPatch(cx, cy, scale, option, ntb_font.chars[c], basecolormap);

		cx += w;
	}
}


void V_DrawNameTag(INT32 x, INT32 y, INT32 option, fixed_t scale, UINT8 *basecolormap, UINT8 *outlinecolormap, const char *string)
{
	const char *text = string;
	const char *first_token = text;
	char *last_token = strchr(text, '\n');
	const INT32 lbreakheight = 21;
	INT32 ntlines;

	if (option & V_CENTERNAMETAG)
	{
		ntlines = V_CountNameTagLines(string);
		y -= FixedInt(FixedMul(((lbreakheight/2) * (ntlines-1))*FRACUNIT, scale));
	}



	if (!last_token)
		V_DrawNameTagLine(x, y, option, scale, basecolormap, outlinecolormap, string);

	else
	{
		char *str = NULL;
		INT32 len;
		while (true)
		{

			if (last_token)
			{
				size_t shift = 0;

				if (str)
					Z_Free(str);

				len = (last_token-first_token)+1;
				str = ZZ_Alloc(len);

				strncpy(str, first_token, len-1);
				str[len-1] = '\0';


				if ((strlen(str) >= 2) && (string[0] == '\n') && (string[1] != '\n'))
					shift++;

				V_DrawNameTagLine(x, y, option, scale, basecolormap, outlinecolormap, str+shift);
			}

			else
			{


				if ((strlen(first_token) >= 2) && (first_token[0] == '\n') && (first_token[1] != '\n'))
					first_token++;

				V_DrawNameTagLine(x, y, option, scale, basecolormap, outlinecolormap, first_token);
				break;
			}


			y += FixedInt(FixedMul(lbreakheight*FRACUNIT, scale));
			if ((last_token-text)+1 >= (signed)strlen(text))
				last_token = NULL;
			else
			{
				first_token = last_token;
				last_token = strchr(first_token+1, '\n');
			}
		}

		if (str)
			Z_Free(str);
	}
}


INT32 V_CountNameTagLines(const char *string)
{
	INT32 ntlines = 1;
	const char *text = string;
	const char *first_token = text;
	char *last_token = strchr(text, '\n');


	if (!last_token)
		return ntlines;

	else
	{
		while (true)
		{
			if (last_token)
				ntlines++;

			else
				break;


			if ((last_token-text)+1 >= (signed)strlen(text))
				last_token = NULL;
			else
			{
				first_token = last_token;
				last_token = strchr(first_token+1, '\n');
			}
		}
	}
	return ntlines;
}



INT32 V_FontStringWidth(const char *string, INT32 option, fontdef_t font)
{
	INT32 c, w = 0, wline = 0;
	INT32 spacewidth = font.spacewidth, charwidth = 0;

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = font.charwidth;
			 
		case V_OLDSPACING:
			charwidth = font.charwidth;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 6;
		default:
			break;
	}

	for (size_t i = 0; i < strlen(string); i++)
	{
		if (string[i] == '\n')
		{
			if (wline < w) wline = w;
			w = 0;
			continue;
		}
		if (string[i] & 0x80)
			continue;

		c = (option & V_ALLOWLOWERCASE ? string[i] : toupper(string[i])) - FONTSTART;
		if (c < 0 || c >= FONTSIZE || !font.chars[c])
			w += spacewidth;
		else
			w += (charwidth ? charwidth : (font.chars[c]->width)) + font.kerning;
	}
	w = max(wline, w);

	if (option & (V_NOSCALESTART|V_NOSCALEPATCH))
		w *= vid.dup;

	return w;
}



INT32 V_FontStringHeight(const char *string, INT32 option, fontdef_t font)
{
	INT32 c, h = 0, result = 0;

	for (size_t i = 0; i < strlen(string); i++)
	{
		c = (option & V_ALLOWLOWERCASE ? string[i] : toupper(string[i])) - FONTSTART;
		if (c < 0 || c >= FONTSIZE || !font.chars[c])
		{
			if (string[i] == '\n')
			{
				result += (option & V_RETURN8) ? 8 : font.linespacing;
				h = 0;
			}
			continue;
		}

		if (font.chars[c]->height > h)
			h = font.chars[c]->height;
	}

	return result + h;
}

boolean *heatshifter = NULL;
INT32 lastheight = 0;
INT32 heatindex[2] = { 0, 0 };






#include "p_local.h"
void V_DoPostProcessor(INT32 view, postimg_t type, INT32 param)
{
#if NUMSCREENS < 5

	(void)view;
	(void)type;
	(void)param;
#else
	INT32 height, yoffset;

#ifdef HWRENDER
	if (rendermode != render_soft)
		return;
#endif

	if (view < 0 || view >= 2 || (view == 1 && !splitscreen))
		return;

	if (splitscreen)
		height = vid.height/2;
	else
		height = vid.height;

	if (view == 1)
		yoffset = vid.height/2;
	else
		yoffset = 0;

	if (type == postimg_water)
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y;

		angle_t disStart = (((leveltime-1)*128) + (rendertimefrac / (FRACUNIT/128))) & FINEMASK;
		INT32 newpix;
		INT32 sine;


		for (y = yoffset; y < yoffset+height; y++)
		{
			sine = (FINESINE(disStart)*5)>>FRACBITS;
			newpix = abs(sine);

			if (sine < 0)
			{
				M_Memcpy(&tmpscr[y*vid.width+newpix], &srcscr[y*vid.width], vid.width-newpix);


				while (newpix)
				{
					tmpscr[y*vid.width+newpix] = srcscr[y*vid.width];
					newpix--;
				}
			}
			else
			{
				M_Memcpy(&tmpscr[y*vid.width+0], &srcscr[y*vid.width+sine], vid.width-newpix);


				while (newpix)
				{
					tmpscr[y*vid.width+vid.width-newpix] = srcscr[y*vid.width+(vid.width-1)];
					newpix--;
				}
			}

  
                   
                                          
    
                        

                   
                
                                 
                          

                                                                                                                                  
      
			disStart += 22; 
			disStart &= FINEMASK;  
		}

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset, screens[0]+vid.width*vid.bpp*yoffset,
				vid.width*vid.bpp, height, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_motion)  
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 x, y;


		UINT8 *transme = R_GetTranslucencyTable(param);

		for (y = yoffset; y < yoffset+height; y++)
		{
			for (x = 0; x < vid.width; x++)
			{
				tmpscr[y*vid.width + x]
					=     colormaps[*(transme     + (srcscr   [y*vid.width+x ] <<8) + (tmpscr[y*vid.width+x]))];
			}
		}
		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset, screens[0]+vid.width*vid.bpp*yoffset,
				vid.width*vid.bpp, height, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_flip)  
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y, y2;

		for (y = yoffset, y2 = yoffset+height - 1; y < yoffset+height; y++, y2--)
			M_Memcpy(&tmpscr[y2*vid.width], &srcscr[y*vid.width], vid.width);

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset, screens[0]+vid.width*vid.bpp*yoffset,
				vid.width*vid.bpp, height, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_heat)  
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y;


		if (heatshifter == NULL || lastheight != height)
		{
			if (heatshifter)
				Z_Free(heatshifter);

			heatshifter = Z_Calloc(height * sizeof(boolean), PU_STATIC, NULL);

			for (y = 0; y < height; y++)
			{
				if (M_RandomChance(FRACUNIT/8))  
					heatshifter[y] = true;
			}

			heatindex[0] = heatindex[1] = 0;
			lastheight = height;
		}

		for (y = yoffset; y < yoffset+height; y++)
		{
			if (heatshifter[heatindex[view]++])
			{

				tmpscr[y*vid.width] = srcscr[y*vid.width];
				M_Memcpy(&tmpscr[y*vid.width+vid.dup], &srcscr[y*vid.width], vid.width-vid.dup);
			}
			else
				M_Memcpy(&tmpscr[y*vid.width], &srcscr[y*vid.width], vid.width);

			heatindex[view] %= height;
		}

		if (renderisnewtic)  
		{
			heatindex[view]++;
			heatindex[view] %= vid.height;
		}

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset, screens[0]+vid.width*vid.bpp*yoffset,
				vid.width*vid.bpp, height, vid.width*vid.bpp, vid.width);
	}
#endif
}


void InitColorLUT(colorlookup_t *lut, RGBA_t *palette, boolean makecolors)
{
	size_t palsize = (sizeof(RGBA_t) * 256);

	if (!lut->init || memcmp(lut->palette, palette, palsize))
	{
		INT32 i;

		lut->init = true;
		memcpy(lut->palette, palette, palsize);

		for (i = 0; i < 0xFFFF; i++)
			lut->table[i] = 0xFFFF;

		if (makecolors)
		{
			UINT8 r, g, b;

			for (r = 0; r < 0xFF; r++)
			for (g = 0; g < 0xFF; g++)
			for (b = 0; b < 0xFF; b++)
			{
				i = CLUTINDEX(r, g, b);
				if (lut->table[i] == 0xFFFF)
					lut->table[i] = NearestPaletteColor(r, g, b, palette);
			}
		}
	}
}

UINT8 GetColorLUT(colorlookup_t *lut, UINT8 r, UINT8 g, UINT8 b)
{
	INT32 i = CLUTINDEX(r, g, b);
	if (lut->table[i] == 0xFFFF)
		lut->table[i] = NearestPaletteColor(r, g, b, lut->palette);
	return lut->table[i];
}

UINT8 GetColorLUTDirect(colorlookup_t *lut, UINT8 r, UINT8 g, UINT8 b)
{
	INT32 i = CLUTINDEX(r, g, b);
	return lut->table[i];
}





void V_Init(void)
{
	INT32 i;
	UINT8 *base = vid.buffer;
	const INT32 screensize = vid.rowbytes * vid.height;

	LoadMapPalette();

	for (i = 0; i < NUMSCREENS; i++)
		screens[i] = NULL;


	if (base)
	{
		for (i = 0; i < NUMSCREENS; i++)
			screens[i] = base + i*screensize;
	}

	if (vid.direct)
		screens[0] = vid.direct;

#ifdef DEBUG
	CONS_Debug(DBG_RENDER, "V_Init done:\n");
	for (i = 0; i < NUMSCREENS; i++)
		CONS_Debug(DBG_RENDER, " screens[%d] = %x\n", i, screens[i]);
#endif
}

void V_Recalc(void)
{




	if (((vid.width*FRACUNIT) / BASEVIDWIDTH) < ((vid.height*FRACUNIT) / BASEVIDHEIGHT))
	{
		vid.dup = vid.width / BASEVIDWIDTH;
		vid.fdup = (vid.width*FRACUNIT) / BASEVIDWIDTH;
	}
	else
	{
		vid.dup = vid.height / BASEVIDHEIGHT;
		vid.fdup = (vid.height*FRACUNIT) / BASEVIDHEIGHT;
	}

	vid.meddup = (UINT8)(vid.dup >> 1) + 1;
	vid.smalldup = (UINT8)(vid.dup / 3) + 1;
#ifdef HWRENDER
	vid.fmeddup = vid.meddup*FRACUNIT;
	vid.fsmalldup = vid.smalldup*FRACUNIT;
#endif
}
