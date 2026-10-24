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



#ifndef __R_TEXTURES__
#define __R_TEXTURES__

#include "r_defs.h"
#include "r_state.h"
#include "p_setup.h"  
#include "r_data.h"

#ifdef __GNUG__
#pragma interface
#endif




typedef struct
{

	INT16 originx, originy;
	UINT16 wad, lump;
	UINT8 flip;  
	UINT8 alpha;  
	enum patchalphastyle style;
} texpatch_t;


enum
{
	TEXTURETYPE_UNKNOWN,
	TEXTURETYPE_SINGLEPATCH,
	TEXTURETYPE_COMPOSITE,
	TEXTURETYPE_FLAT
};




typedef struct
{

	char name[8];
	UINT32 hash;
	UINT8 type;  
	boolean transparency;
	boolean fullytransparent;  
	INT16 width, height;
	UINT8 flip;  
	void *flat;  


	INT16 patchcount;
	texpatch_t patches[0];
} texture_t;


extern texture_t **textures;

extern INT32 *texturewidth;
extern fixed_t *textureheight;  

extern column_t **texturecolumns;  
extern UINT8 **texturecache;  


void R_LoadTextures(void);
void R_LoadTexturesPwad(UINT16 wadnum);
void R_FlushTextureCache(void);


UINT8 *R_GenerateTexture(size_t texnum);
UINT8 *R_GetFlatForTexture(size_t texnum);
INT32 R_GetTextureNum(INT32 texnum);
void R_CheckTextureCache(INT32 tex);
void R_ClearTextureNumCache(boolean btell);


column_t *R_GetColumn(fixed_t tex, INT32 col);
void *R_GetFlat(levelflat_t *levelflat);

INT32 R_GetTextureNumForFlat(levelflat_t *levelflat);

boolean R_CheckPowersOfTwo(void);
boolean R_CheckSolidColorFlat(void);

UINT16 R_GetFlatSize(size_t length);
UINT8 R_GetFlatBits(INT32 size);
void R_SetFlatVars(size_t length);


INT32 R_TextureNumForName(const char *name);
INT32 R_CheckTextureNumForName(const char *name);
INT32 R_CheckFlatNumForName(const char *name);


const char *R_CheckTextureNameForNum(INT32 num);
const char *R_TextureNameForNum(INT32 num);

extern INT32 numtextures;

#endif
