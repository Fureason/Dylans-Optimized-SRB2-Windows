// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __HWR_BATCHING_H__
#define __HWR_BATCHING_H__

#include "hw_defs.h"
#include "hw_data.h"
#include "hw_drv.h"

typedef struct
{
	FSurfaceInfo surf; 
	unsigned int vertsIndex; 
	FUINT numVerts;
	FBITFIELD polyFlags;
	GLMipmap_t *texture;
	int shader;

	boolean horizonSpecial;
	INT32 hash;
} PolygonArrayEntry;

void HWR_StartBatching(void);
void HWR_SetCurrentTexture(GLMipmap_t *texture);
void HWR_ProcessPolygon(FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, int shader, boolean horizonSpecial);
void HWR_RenderBatches(void);

#endif
