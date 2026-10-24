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



#ifndef __R_PORTAL__
#define __R_PORTAL__

#include "r_data.h"
#include "r_textures.h"
#include "r_plane.h"  

                                               
   
typedef struct portal_s
{
	struct portal_s *next;


	fixed_t viewx;
	fixed_t viewy;
	fixed_t viewz;
	angle_t viewangle;


	boolean is_horizon;
	sector_t *horizon_sector;

	boolean is_skybox;

	mobj_t *viewmobj;

	UINT8 pass;			 
	INT32 clipline;		 


	INT32 start;		 
	INT32 end;			 
	INT16 *ceilingclip;  
	INT16 *floorclip;	 
	fixed_t *frontscale; 
} portal_t;

extern portal_t* portal_base;
extern portal_t* portal_cap;
extern UINT8 portalrender;

extern line_t *portalclipline;
extern sector_t *portalcullsector;
extern INT32 portalclipstart, portalclipend;

void Portal_InitList		(void);
void Portal_Remove			(portal_t* portal);
void Portal_Add2Lines		(const INT32 line1, const INT32 line2, const INT32 x1, const INT32 x2);
void Portal_AddTransferred	(const UINT32 secportalnum, const INT32 x1, const INT32 x2);

void Portal_ClipRange (portal_t* portal);
void Portal_ClipApply (const portal_t* portal);

void Portal_AddPlanePortals (boolean add_skyboxes);
#endif
