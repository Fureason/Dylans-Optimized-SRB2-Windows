// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __R_PLANE__
#define __R_PLANE__

#include "screen.h"  
#include "r_data.h"
#include "r_textures.h"
#include "p_polyobj.h"

#define VISPLANEHASHBITS 9
#define VISPLANEHASHMASK ((1<<VISPLANEHASHBITS)-1)

#define MAXVISPLANES ((1<<VISPLANEHASHBITS)+1)





typedef struct visplane_s
{
	struct visplane_s *next;

	fixed_t height;
	fixed_t viewx, viewy, viewz;
	angle_t viewangle;
	angle_t plangle;
	INT32 picnum;
	INT32 lightlevel;
	INT32 minx, maxx;


	extracolormap_t *extra_colormap;


	UINT16 padtopstart, top[MAXVIDWIDTH], padtopend;
	UINT16 padbottomstart, bottom[MAXVIDWIDTH], padbottomend;
	INT32 high, low;  

	INT64 xoffs, yoffs;  
	fixed_t xscale, yscale;

	sector_t *sector;
	struct ffloor_s *ffloor;
	polyobj_t *polyobj;
	pslope_t *slope;
	sectorportal_t *portalsector;
} visplane_t;

extern visplane_t *visplanes[MAXVISPLANES];
extern visplane_t *floorplane;
extern visplane_t *ceilingplane;


extern INT16 floorclip[MAXVIDWIDTH], ceilingclip[MAXVIDWIDTH];
extern fixed_t frontscale[MAXVIDWIDTH], yslopetab[MAXVIDHEIGHT*16];

extern fixed_t *yslope;
extern lighttable_t **planezlight;

void R_ClearPlanes(void);
void R_ClearFFloorClips (void);

void R_DrawPlanes(void);
visplane_t *R_FindPlane(sector_t *sector, fixed_t height, INT32 picnum, INT32 lightlevel,
	fixed_t xoff, fixed_t yoff, fixed_t xscale, fixed_t yscale, angle_t plangle,
	extracolormap_t *planecolormap, ffloor_t *ffloor, polyobj_t *polyobj, pslope_t *slope, sectorportal_t *portalsector);
visplane_t *R_CheckPlane(visplane_t *pl, INT32 start, INT32 stop);
void R_ExpandPlane(visplane_t *pl, INT32 start, INT32 stop);
void R_PlaneBounds(visplane_t *plane);


void R_DrawSinglePlane(visplane_t *pl);


void R_SetSlopePlane(pslope_t *slope, fixed_t xpos, fixed_t ypos, fixed_t zpos, fixed_t xoff, fixed_t yoff, angle_t angle, angle_t plangle);
void R_SetScaledSlopePlane(pslope_t *slope, fixed_t xpos, fixed_t ypos, fixed_t zpos, fixed_t xs, fixed_t ys, INT64 xoff, INT64 yoff, angle_t angle, angle_t plangle);

typedef struct planemgr_s
{
	visplane_t *plane;
	fixed_t height;
	fixed_t f_pos;  
	fixed_t b_pos;  
	fixed_t f_frac, f_step;
	fixed_t b_frac, b_step;
	INT16 f_clip[MAXVIDWIDTH];
	INT16 c_clip[MAXVIDWIDTH];


	fixed_t f_pos_slope;
	fixed_t b_pos_slope;

	struct pslope_s *slope;

	struct ffloor_s *ffloor;
	polyobj_t *polyobj;
} visffloor_t;

extern visffloor_t ffloor[MAXFFLOORS];
extern INT32 numffloors;

void Portal_AddSkyboxPortals (void);
#endif
