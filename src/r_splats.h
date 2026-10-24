// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __R_SPLATS_H__
#define __R_SPLATS_H__

#include "r_defs.h"
#include "r_things.h"





struct rastery_s
{
	fixed_t minx, maxx;  
	fixed_t tx1, ty1;    
	fixed_t tx2, ty2;    
};
extern struct rastery_s *prastertab;  

typedef struct floorsplat_s
{
	UINT16 *pic;
	INT32 width, height;
	fixed_t scale, xscale, yscale;
	angle_t angle;
	pslope_t *slope;

	vector3_t verts[4];  
	fixed_t x, y, z;  
	mobj_t *mobj;  
} floorsplat_t;

void R_DrawFloorSplat(vissprite_t *spr);

#endif  
