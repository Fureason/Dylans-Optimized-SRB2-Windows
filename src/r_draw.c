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
#include "doomstat.h"
#include "r_local.h"
#include "r_translation.h"
#include "i_video.h"
#include "v_video.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"
#include "console.h"  
#include "libdivide.h"  

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif





                    
  
INT32 viewwidth, scaledviewwidth, viewheight, viewwindowx, viewwindowy;

UINT8 *topleft;





lighttable_t *dc_colormap;
INT32 dc_x = 0, dc_yl = 0, dc_yh = 0;

fixed_t dc_iscale, dc_texturemid;
UINT8 *dc_source;




#define NUMTRANSTABLES 9  

UINT8 *transtables;  
UINT8 *blendtables[NUMBLENDMAPS];

                                      
  
UINT8 *dc_transmap;  






                                           
  
UINT8 *dc_translation;

struct r_lightlist_s *dc_lightlist = NULL;
INT32 dc_numlights = 0, dc_maxlights, dc_texheight, dc_postlength;





INT32 ds_y, ds_x1, ds_x2;
lighttable_t *ds_colormap;
lighttable_t *ds_translation;  

fixed_t ds_xfrac, ds_yfrac, ds_xstep, ds_ystep;
INT32 ds_waterofs, ds_bgofs;

UINT16 ds_flatwidth, ds_flatheight;
boolean ds_powersoftwo, ds_solidcolor, ds_fog;

UINT8 *ds_source;  
UINT8 *ds_transmap;  


dvector3_t ds_su, ds_sv, ds_sz, ds_slopelight;
double zeroheight;
float focallengthf;

                              
  

UINT32 nflatxshift, nflatyshift, nflatshiftup, nflatmask;





static colorcache_t **translationtablecache[TT_CACHE_SIZE] = {NULL};

boolean skincolor_modified[MAXSKINCOLORS];

INT32 R_SkinTranslationToCacheIndex(INT32 translation)
{
	switch (translation)
	{
		case TC_DEFAULT:    return DEFAULT_TT_CACHE_INDEX;
		case TC_BOSS:       return BOSS_TT_CACHE_INDEX;
		case TC_METALSONIC: return METALSONIC_TT_CACHE_INDEX;
		case TC_ALLWHITE:   return ALLWHITE_TT_CACHE_INDEX;
		case TC_RAINBOW:    return RAINBOW_TT_CACHE_INDEX;
		case TC_BLINK:      return BLINK_TT_CACHE_INDEX;
		case TC_DASHMODE:   return DASHMODE_TT_CACHE_INDEX;
		default:            return translation;
	}
}

static INT32 CacheIndexToSkin(INT32 index)
{
	switch (index)
	{
		case DEFAULT_TT_CACHE_INDEX:    return TC_DEFAULT;
		case BOSS_TT_CACHE_INDEX:       return TC_BOSS;
		case METALSONIC_TT_CACHE_INDEX: return TC_METALSONIC;
		case ALLWHITE_TT_CACHE_INDEX:   return TC_ALLWHITE;
		case RAINBOW_TT_CACHE_INDEX:    return TC_RAINBOW;
		case BLINK_TT_CACHE_INDEX:      return TC_BLINK;
		case DASHMODE_TT_CACHE_INDEX:   return TC_DASHMODE;
		default:                        return index;
	}
}

CV_PossibleValue_t Color_cons_t[MAXSKINCOLORS+1];

                                                                             
  
void R_InitTranslucencyTables(void)
{

	transtables = Z_MallocAlign(NUMTRANSTABLES*0x10000, PU_STATIC,
		NULL, 16);

	W_ReadLump(W_GetNumForName("TRANS10"), transtables);
	W_ReadLump(W_GetNumForName("TRANS20"), transtables+0x10000);
	W_ReadLump(W_GetNumForName("TRANS30"), transtables+0x20000);
	W_ReadLump(W_GetNumForName("TRANS40"), transtables+0x30000);
	W_ReadLump(W_GetNumForName("TRANS50"), transtables+0x40000);
	W_ReadLump(W_GetNumForName("TRANS60"), transtables+0x50000);
	W_ReadLump(W_GetNumForName("TRANS70"), transtables+0x60000);
	W_ReadLump(W_GetNumForName("TRANS80"), transtables+0x70000);
	W_ReadLump(W_GetNumForName("TRANS90"), transtables+0x80000);

	R_GenerateBlendTables();
}

static colorlookup_t transtab_lut;

static void BlendTab_Translucent(UINT8 *table, int style, UINT8 blendamt)
{
	INT16 bg, fg;

	if (table == NULL)
		I_Error("BlendTab_Translucent: input table was NULL!");

	for (bg = 0; bg < 0xFF; bg++)
	{
		for (fg = 0; fg < 0xFF; fg++)
		{
			RGBA_t backrgba = V_GetMasterColor(bg);
			RGBA_t frontrgba = V_GetMasterColor(fg);
			RGBA_t result;

			result.rgba = ASTBlendPixel(backrgba, frontrgba, style, 0xFF);
			result.rgba = ASTBlendPixel(result, frontrgba, AST_TRANSLUCENT, blendamt);

			table[((bg * 0x100) + fg)] = GetColorLUT(&transtab_lut, result.s.red, result.s.green, result.s.blue);
		}
	}
}

static void BlendTab_Subtractive(UINT8 *table, int style, UINT8 blendamt)
{
	INT16 bg, fg;

	if (table == NULL)
		I_Error("BlendTab_Subtractive: input table was NULL!");

	if (blendamt == 0xFF)
	{
		memset(table, GetColorLUT(&transtab_lut, 0, 0, 0), 0x10000);
		return;
	}

	for (bg = 0; bg < 0xFF; bg++)
	{
		for (fg = 0; fg < 0xFF; fg++)
		{
			RGBA_t backrgba = V_GetMasterColor(bg);
			RGBA_t frontrgba = V_GetMasterColor(fg);
			RGBA_t result;

			result.rgba = ASTBlendPixel(backrgba, frontrgba, style, 0xFF);
			result.s.red = max(0, result.s.red - blendamt);
			result.s.green = max(0, result.s.green - blendamt);
			result.s.blue = max(0, result.s.blue - blendamt);




			table[((bg * 0x100) + fg)] = GetColorLUT(&transtab_lut, result.s.red, result.s.green, result.s.blue);
		}
	}
}

static void BlendTab_Modulative(UINT8 *table)
{
	INT16 bg, fg;

	if (table == NULL)
		I_Error("BlendTab_Modulative: input table was NULL!");

	for (bg = 0; bg < 0xFF; bg++)
	{
		for (fg = 0; fg < 0xFF; fg++)
		{
			RGBA_t backrgba = V_GetMasterColor(bg);
			RGBA_t frontrgba = V_GetMasterColor(fg);
			RGBA_t result;
			result.rgba = ASTBlendPixel(backrgba, frontrgba, AST_MODULATE, 0);
			table[((bg * 0x100) + fg)] = GetColorLUT(&transtab_lut, result.s.red, result.s.green, result.s.blue);
		}
	}
}

static INT32 BlendTab_Count[NUMBLENDMAPS] =
{
	NUMTRANSTABLES+1,  
	NUMTRANSTABLES+1,  
	NUMTRANSTABLES+1,  
	1                  
};

static INT32 BlendTab_FromStyle[] =
{
	0,                         
	0,                         
	blendtab_add,              
	blendtab_subtract,         
	blendtab_reversesubtract,  
	blendtab_modulate,         
	0                          
};

static void BlendTab_GenerateMaps(INT32 tab, INT32 style, void (*genfunc)(UINT8 *, int, UINT8))
{
	INT32 i = 0, num = BlendTab_Count[tab];
	const float amtmul = (256.0f / (float)(NUMTRANSTABLES + 1));
	for (; i < num; i++)
	{
		const size_t offs = (0x10000 * i);
		const UINT16 alpha = min(amtmul * i, 0xFF);
		genfunc(blendtables[tab] + offs, style, alpha);
	}
}

void R_GenerateBlendTables(void)
{
	INT32 i;

	for (i = 0; i < NUMBLENDMAPS; i++)
		blendtables[i] = Z_MallocAlign(BlendTab_Count[i] * 0x10000, PU_STATIC, NULL, 16);

	InitColorLUT(&transtab_lut, pMasterPalette, false);


	BlendTab_GenerateMaps(blendtab_add, AST_ADD, BlendTab_Translucent);


#if 1
	BlendTab_GenerateMaps(blendtab_subtract, AST_SUBTRACT, BlendTab_Subtractive);
#else
	BlendTab_GenerateMaps(blendtab_subtract, AST_SUBTRACT, BlendTab_Translucent);
#endif


	BlendTab_GenerateMaps(blendtab_reversesubtract, AST_REVERSESUBTRACT, BlendTab_Translucent);


	BlendTab_Modulative(blendtables[blendtab_modulate]);
}

#define ClipBlendLevel(style, trans) max(min((trans), BlendTab_Count[BlendTab_FromStyle[style]]-1), 0)
#define ClipTransLevel(trans) max(min((trans), NUMTRANSMAPS-2), 0)

UINT8 *R_GetTranslucencyTable(INT32 alphalevel)
{
	return transtables + (ClipTransLevel(alphalevel-1) << FF_TRANSSHIFT);
}

UINT8 *R_GetBlendTable(int style, INT32 alphalevel)
{
	size_t offs;

	if (style <= AST_COPY || style >= AST_OVERLAY)
		return NULL;

	offs = (ClipBlendLevel(style, alphalevel) << FF_TRANSSHIFT);



	switch (style)
	{
		case AST_ADD:
			return blendtables[blendtab_add] + offs;
		case AST_SUBTRACT:
			return blendtables[blendtab_subtract] + offs;
		case AST_REVERSESUBTRACT:
			return blendtables[blendtab_reversesubtract] + offs;
		case AST_MODULATE:
			return blendtables[blendtab_modulate];
		default:
			break;
	}


	if (--alphalevel >= 0)
		return transtables + (ClipTransLevel(alphalevel) << FF_TRANSSHIFT);
	else
		return NULL;
}

boolean R_BlendLevelVisible(INT32 blendmode, INT32 alphalevel)
{
	if (blendmode <= AST_COPY || blendmode == AST_SUBTRACT || blendmode == AST_MODULATE || blendmode >= AST_OVERLAY)
		return true;

	return (alphalevel < BlendTab_Count[BlendTab_FromStyle[blendmode]]);
}







#define SETBRIGHTNESS(brightness,r,g,b) \
	brightness = (UINT8)(((1063*((UINT16)r)/5000) + (3576*((UINT16)g)/5000) + (361*((UINT16)b)/5000)) / 3)

                                                                                                                                        

                                          
                                    
  
static void R_RainbowColormap(UINT8 *dest_colormap, UINT16 skincolor)
{
	INT32 i;
	RGBA_t color;
	UINT8 brightness;
	INT32 j;
	UINT8 colorbrightnesses[COLORRAMPSIZE];
	UINT16 brightdif;
	INT32 temp;


	for (i = 0; i < COLORRAMPSIZE; i++)
	{
		color = V_GetColor(skincolors[skincolor].ramp[i]);
		SETBRIGHTNESS(colorbrightnesses[i], color.s.red, color.s.green, color.s.blue);
	}


	for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
	{
		if (i == 0 || i == 31)  
		{
			dest_colormap[i] = (UINT8)i;
			continue;
		}
		color = V_GetColor(i);
		SETBRIGHTNESS(brightness, color.s.red, color.s.green, color.s.blue);
		brightdif = 256;
		for (j = 0; j < COLORRAMPSIZE; j++)
		{
			temp = abs((INT16)brightness - (INT16)colorbrightnesses[j]);
			if (temp < brightdif)
			{
				brightdif = (UINT16)temp;
				dest_colormap[i] = skincolors[skincolor].ramp[j];
			}
		}
	}
}

#undef SETBRIGHTNESS

                                            

                                            
                                        
                                         
                                                          

             
  
static void R_GenerateTranslationColormap(UINT8 *dest_colormap, INT32 translation, UINT16 color, INT32 starttranscolor)
{
	INT32 i, skinramplength;
	remaptable_t *tr;


	if (translation < TC_DEFAULT)
	{
		switch (translation)
		{
			case TC_ALLWHITE:
			case TC_DASHMODE:
				tr = R_GetBuiltInTranslation((SINT8)translation);
				if (tr)
				{
					memcpy(dest_colormap, tr->remap, NUM_PALETTE_ENTRIES);
					return;
				}
				break;
			case TC_RAINBOW:
				if (color >= numskincolors)
					I_Error("Invalid skin color #%hu", (UINT16)color);
				else if (color != SKINCOLOR_NONE)
				{
					R_RainbowColormap(dest_colormap, color);
					return;
				}
				break;
			case TC_BLINK:
				if (color >= numskincolors)
					I_Error("Invalid skin color #%hu", (UINT16)color);
				else if (color != SKINCOLOR_NONE)
				{
					memset(dest_colormap, skincolors[color].ramp[3], NUM_PALETTE_ENTRIES * sizeof(UINT8));
					return;
				}
				break;
			default:
				break;
		}

		for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
			dest_colormap[i] = (UINT8)i;


		if (translation == TC_BOSS)
		{
			UINT8 *originalColormap = R_GetTranslationColormap(TC_DEFAULT, (skincolornum_t)color, GTC_CACHE);
			if (starttranscolor >= NUM_PALETTE_ENTRIES)
				I_Error("Invalid startcolor #%d", starttranscolor);
			for (i = 0; i < COLORRAMPSIZE; i++)
			{
				dest_colormap[starttranscolor + i] = originalColormap[starttranscolor + i];
				dest_colormap[31-i] = i;
			}
		}
		else if (translation == TC_METALSONIC)
		{
			for (i = 0; i < 6; i++)
			{
				dest_colormap[skincolors[SKINCOLOR_BLUE].ramp[12-i]] = skincolors[SKINCOLOR_BLUE].ramp[i];
			}
			dest_colormap[159] = dest_colormap[253] = dest_colormap[254] = 0;
			for (i = 0; i < COLORRAMPSIZE; i++)
				dest_colormap[96+i] = dest_colormap[skincolors[SKINCOLOR_COBALT].ramp[i]];
		}
		return;
	}
	else if (color == SKINCOLOR_NONE)
	{
		for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
			dest_colormap[i] = (UINT8)i;
		return;
	}

	if (color >= numskincolors)
		I_Error("Invalid skin color #%hu", (UINT16)color);

	if (starttranscolor >= NUM_PALETTE_ENTRIES)
		I_Error("Invalid startcolor #%d", starttranscolor);


	for (i = 0; i < starttranscolor; i++)
		dest_colormap[i] = (UINT8)i;

	i = starttranscolor + COLORRAMPSIZE;
	if (i < NUM_PALETTE_ENTRIES)
	{
		for (i = (UINT8)i; i < NUM_PALETTE_ENTRIES; i++)
			dest_colormap[i] = (UINT8)i;
		skinramplength = COLORRAMPSIZE;
	}
	else
		skinramplength = i - NUM_PALETTE_ENTRIES;  


	for (i = 0; i < skinramplength; i++)
		dest_colormap[starttranscolor + i] = (UINT8)skincolors[color].ramp[i];
}


                                                           

                                                    
                               
                                            

                                                       
  
UINT8* R_GetTranslationColormap(INT32 skinnum, skincolornum_t color, UINT8 flags)
{
	colorcache_t *ret;
	INT32 index = 0;
	INT32 starttranscolor = DEFAULT_STARTTRANSCOLOR;


	if (skinnum >= numskins)
		I_Error("Invalid skin number %d", skinnum);
	else if (skinnum >= 0)
	{
		index = skins[skinnum]->skinnum;
		starttranscolor = skins[skinnum]->starttranscolor;
	}
	else if (skinnum <= TC_DEFAULT)
	{

		index = R_SkinTranslationToCacheIndex(skinnum);
	}
	else
		I_Error("Invalid translation %d", skinnum);

	if (flags & GTC_CACHE)
	{

		if (!translationtablecache[index])
			translationtablecache[index] = Z_Calloc(MAXSKINCOLORS * sizeof(colorcache_t**), PU_STATIC, NULL);


		ret = translationtablecache[index][color];


		if (skincolor_modified[color])
		{


			skincolor_modified[color] = false;

			for (unsigned i = 0; i < TT_CACHE_SIZE; i++)
			{
				if (translationtablecache[i])
				{
					colorcache_t *cache = translationtablecache[i][color];
					if (cache)
					{
						R_GenerateTranslationColormap(cache->colors, CacheIndexToSkin(i), color, starttranscolor);
						R_UpdateTranslationRemaps(color, i);
					}
				}
			}
		}
	}
	else
		ret = NULL;


	if (!ret)
	{
		ret = Z_Malloc(sizeof(colorcache_t), (flags & GTC_CACHE) ? PU_LEVEL : PU_STATIC, NULL);
		R_GenerateTranslationColormap(ret->colors, skinnum, color, starttranscolor);


		if (flags & GTC_CACHE)
			translationtablecache[index][color] = ret;
	}

	return ret->colors;
}

                                                  

                                                                      
                                                                       
                                                          

             
  
void R_FlushTranslationColormapCache(void)
{
	INT32 i;

	for (i = 0; i < TT_CACHE_SIZE; i++)
		if (translationtablecache[i])
			memset(translationtablecache[i], 0, MAXSKINCOLORS * sizeof(UINT8**));
}

UINT16 R_GetColorByName(const char *name)
{
	UINT16 color = (UINT16)atoi(name);
	if (color > 0 && color < numskincolors)
		return color;
	for (color = 1; color < numskincolors; color++)
		if (!stricmp(skincolors[color].name, name))
			return color;
	return SKINCOLOR_NONE;
}

UINT16 R_GetSuperColorByName(const char *name)
{
	UINT16 i, color = SKINCOLOR_NONE;
	char *realname = Z_Malloc(MAXCOLORNAME+1, PU_STATIC, NULL);
	snprintf(realname, MAXCOLORNAME+1, "Super %s 1", name);
	for (i = 1; i < numskincolors; i++)
		if (!stricmp(skincolors[i].name, realname)) {
			color = i;
			break;
		}
	Z_Free(realname);
	return color;
}











                                        

                                                          
                    

                             
                               

             


  

void R_InitViewBuffer(INT32 width, INT32 height)
{
	INT32 bytesperpixel = vid.bpp;

	if (width > MAXVIDWIDTH)
		width = MAXVIDWIDTH;
	if (height > MAXVIDHEIGHT)
		height = MAXVIDHEIGHT;
	if (bytesperpixel < 1 || bytesperpixel > 4)
		I_Error("R_InitViewBuffer: wrong bytesperpixel value %d\n", bytesperpixel);


	viewwindowx = (vid.width - width) >> 1;


	if (width == vid.width)
		viewwindowy = 0;
	else
		viewwindowy = (vid.height - height) >> 1;
}

                                    

                      

                              
                            

             


  
void R_VideoErase(size_t ofs, INT32 count)
{





	M_Memcpy(screens[0] + ofs, screens[1] + ofs, count);
}



static INT32 tiltlighting[MAXVIDWIDTH];

static void R_CalcTiltedLighting(fixed_t start, fixed_t end)
{


	INT32 left = ds_x1, right = ds_x2;
	fixed_t step = (end-start)/(ds_x2-ds_x1+1);
	INT32 i;




	for (i = left; i <= right; i++) {
		tiltlighting[i] = (start += step) >> FRACBITS;
		if (tiltlighting[i] < 0)
			tiltlighting[i] = 0;
		else if (tiltlighting[i] >= MAXLIGHTSCALE)
			tiltlighting[i] = MAXLIGHTSCALE-1;
	}
}

#define PLANELIGHTFLOAT (BASEVIDWIDTH * BASEVIDWIDTH / vid.width / zeroheight / 21.0f * FIXED_TO_FLOAT(fovtan))


static void R_CalcSlopeLight(void)
{
	float iz = ds_slopelight.z + ds_slopelight.y * (centery - ds_y) + ds_slopelight.x * (ds_x1 - centerx);
	float lightstart = iz * PLANELIGHTFLOAT;
	float lightend = (iz + ds_slopelight.x * (ds_x2 - ds_x1)) * PLANELIGHTFLOAT;
	R_CalcTiltedLighting(FloatToFixed(lightstart), FloatToFixed(lightend));
}







#include "r_draw8.c"
#include "r_draw8_npo2.c"
