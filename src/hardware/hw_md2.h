// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------




#ifndef _HW_MD2_H_
#define _HW_MD2_H_

#include "hw_glob.h"
#include "hw_model.h"

#if defined(_MSC_VER)
#pragma pack()
#endif

typedef struct
{
	char        filename[32];
	float       scale;
	float       offset;
	model_t     *model;
	void        *grpatch;
	boolean     notexturefile;  
	void        *blendgrpatch;
	boolean     noblendfile;  
	boolean     found;
	boolean     error;
} md2_t;

extern md2_t md2_models[NUMSPRITES];
extern md2_t *md2_playermodels;
extern size_t md2_numplayermodels;

void HWR_InitModels(void);
void HWR_LoadModels(void);

boolean HWR_DrawModel(gl_vissprite_t *spr);

#define PLAYERMODELPREFIX "PLAYER"

#endif  
