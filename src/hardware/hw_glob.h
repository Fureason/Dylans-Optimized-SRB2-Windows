// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef _HWR_GLOB_H_
#define _HWR_GLOB_H_

#include "hw_defs.h"
#include "hw_main.h"
#include "../m_misc.h"
#include "../p_setup.h"


#define ORIGINAL_ASPECT (320.0f/200.0f)









typedef struct
{
	float x;
	float y;
	float z;
} polyvertex_t;

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#endif


typedef struct
{
	INT32 numpts;
	polyvertex_t pts[0];
} poly_t;

#ifdef _MSC_VER
#pragma warning(default :  4200)
#endif


typedef struct
{
	poly_t *planepoly;   
} extrasubsector_t;



typedef struct gl_vissprite_s
{
	float x1, x2;
	float z1, z2;
	float gz, gzt;

	float tz;
	float tracertz;  

	float scale;
	float shadowheight, shadowscale;

	float spritexscale, spriteyscale;
	float spritexoffset, spriteyoffset;

	UINT32 renderflags;
	UINT8 rotateflags;

	boolean flip, vflip;
	boolean precip;  
	boolean bbox;
	boolean rotated;
	                                                                           
                                                               
	boolean sortblended;
	boolean sorttracerblended;
	UINT8 translucency;        

	angle_t angle;  


	UINT8 *colormap;
	INT32 dispoffset;  

	patch_t *gpatch;
	mobj_t *mobj;  
} gl_vissprite_t;




extern extrasubsector_t *extrasubsectors;
extern size_t addsubsector;

void HWR_InitPolyPool(void);
void HWR_FreePolyPool(void);

void HWR_FreeExtraSubsectors(void);




RGBA_t *HWR_GetTexturePalette(void);

void HWR_InitMapTextures(void);
void HWR_LoadMapTextures(size_t pnumtextures);
boolean HWR_MapTexturesAreCurrent(size_t pnumtextures);
void HWR_FreeMapTextures(void);

patch_t *HWR_GetCachedGLPatchPwad(UINT16 wad, UINT16 lump);
patch_t *HWR_GetCachedGLPatch(lumpnum_t lumpnum);

void HWR_GetPatch(patch_t *patch);
void HWR_GetMappedPatch(patch_t *patch, const UINT8 *colormap);
void HWR_GetSpritePatch(patch_t *patch);
void HWR_GetMappedSpritePatch(patch_t *patch, const UINT8 *colormap);
void HWR_GetFadeMask(lumpnum_t fademasklumpnum);

GLMapTexture_t *HWR_GetTexture(INT32 tex, boolean chromakeyed);
void HWR_GetLevelFlat(levelflat_t *levelflat, boolean chromakeyed);
void HWR_GetRawFlat(lumpnum_t flatlumpnum);

void HWR_FreeTexture(patch_t *patch);
void HWR_FreeTextureData(patch_t *patch);
void HWR_FreeTextureColormaps(patch_t *patch);
void HWR_ClearAllTextures(void);
void HWR_ClearLevelTextures(void);
void HWR_FreeColormapCache(void);
void HWR_UnlockCachedPatch(GLPatch_t *gpatch);

void HWR_SetPalette(RGBA_t *palette);
void HWR_SetMapPalette(void);
UINT32 HWR_CreateLightTable(UINT8 *lighttable, RGBA_t *hw_lighttable);
void HWR_UpdateLightTable(UINT32 id, UINT8 *lighttable, RGBA_t *hw_lighttable);
UINT32 HWR_GetLightTableID(extracolormap_t *colormap);
void HWR_ClearLightTables(void);





extern INT32 patchformat;
extern INT32 textureformat;




boolean HWR_InitShaders(void);
void HWR_CompileShaders(void);

int HWR_GetShaderFromTarget(int shader_target);

void HWR_LoadAllCustomShaders(void);
void HWR_LoadCustomShadersFromFile(UINT16 wadnum, boolean PK3);
const char *HWR_GetShaderName(INT32 shader);

extern customshaderxlat_t shaderxlat[];

#endif  
