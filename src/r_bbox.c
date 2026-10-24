// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C)      2022 by Kart Krew.
// Copyright (C) 1999-2022 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "doomdef.h"
#include "command.h"
#include "r_local.h"
#include "screen.h"  
#include "v_video.h"  

enum {
	RENDERHITBOX_OFF,
	RENDERHITBOX_TANGIBLE,
	RENDERHITBOX_ALL,
	RENDERHITBOX_INTANGIBLE,
	RENDERHITBOX_RINGS,
};

static CV_PossibleValue_t renderhitbox_cons_t[] = {
	{RENDERHITBOX_OFF, "Off"},
	{RENDERHITBOX_TANGIBLE, "Tangible"},
	{RENDERHITBOX_ALL, "All"},
	{RENDERHITBOX_INTANGIBLE, "Intangible"},
	{RENDERHITBOX_RINGS, "Rings"},
	{0}};



consvar_t cv_renderhitbox = CVAR_INIT ("renderhitbox", "Off", CV_CHEAT, renderhitbox_cons_t, NULL);
consvar_t cv_renderhitboxinterpolation = CVAR_INIT ("renderhitbox_interpolation", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_renderhitboxgldepth = CVAR_INIT ("renderhitbox_gldepth", "Off", CV_SAVE, CV_OnOff, NULL);

struct bbox_col {
	INT32 x;
	INT32 y;
	INT32 h;
};

struct bbox_config {
	fixed_t height;
	fixed_t tz;
	struct bbox_col col[4];
	UINT8 color;
};

static inline void
raster_bbox_seg
(		INT32 x,
		fixed_t y,
		fixed_t h,
		UINT8 pixel)
{
	y /= FRACUNIT;

	if (y < 0)
		y = 0;

	h = y + (FixedCeil(abs(h)) / FRACUNIT);

	if (h >= viewheight)
		h = viewheight;

	while (y < h)
	{
		topleft[x + y * vid.width] = pixel;
		y++;
	}
}

static void
draw_bbox_col
(		struct bbox_config * bb,
		size_t p,
		fixed_t tx,
		fixed_t ty)
{
	struct bbox_col *col = &bb->col[p];

	fixed_t xscale, yscale;

	if (ty < FRACUNIT)  
		ty = FRACUNIT;

	xscale = FixedDiv(projection, ty);
	yscale = FixedDiv(projectiony, ty);

	col->x = (centerxfrac + FixedMul(tx, xscale)) / FRACUNIT;
	col->y = (centeryfrac - FixedMul(bb->tz, yscale));
	col->h = FixedMul(bb->height, yscale);


	V_DrawFill(
			viewwindowx + col->x,
			viewwindowy + col->y / FRACUNIT, 1,
			col->h / FRACUNIT, V_NOSCALESTART | bb->color);
}

static void
draw_bbox_row
(		struct bbox_config * bb,
		size_t p1,
		size_t p2)
{
	struct bbox_col
		*a = &bb->col[p1],
		*b = &bb->col[p2];

	INT32 x1, x2;  
	INT32 dx;  

	fixed_t y1, y2;  
	fixed_t s1, s2;  

	if (a->x > b->x)
	{
		struct bbox_col *c = a;
		a = b;
		b = c;
	}

	x1 = a->x;
	x2 = b->x;

	if (x1 == x2 || x1 >= viewwidth || x2 < 0)
		return;

	dx = x2 - x1;

	y1 = a->y;
	y2 = b->y;
	s1 = (y2 - y1) / dx;

	y2 = y1 + a->h;
	s2 = ((b->y + b->h) - y2) / dx;



	if (s1 == 0)
		s1 = 1;

	if (s2 == 0)
		s2 = 1;

	if (x1 < 0)
	{
		y1 -= x1 * s1;
		y2 -= x1 * s2;
		x1 = 0;
	}

	if (x2 >= viewwidth)
		x2 = viewwidth - 1;

	while (x1 < x2)
	{
		raster_bbox_seg(x1, y1, s1, bb->color);
		raster_bbox_seg(x1, y2, s2, bb->color);

		y1 += s1;
		y2 += s2;

		x1++;
	}
}

UINT8 R_GetBoundingBoxColor(mobj_t *thing)
{
	UINT32 flags = thing->flags;

	if (thing->player)
		return 255;  

	if (flags & (MF_NOCLIPTHING))
		return 7;  

	if (flags & (MF_BOSS|MF_ENEMY))
		return 35;  

	if (flags & (MF_MISSILE|MF_PAIN))
		return 54;  

	if (flags & (MF_SPECIAL|MF_MONITOR))
		return 73;  

	if (flags & MF_PUSHABLE)
		return 112;  

	if (flags & (MF_SPRING))
		return 181;  

	if (flags & (MF_NOCLIP))
		return 152;  

	return 0;  
}

void R_DrawThingBoundingBox(vissprite_t *vis)
{

	fixed_t rs = vis->scale;
	fixed_t rc = vis->xscale;


	fixed_t tx = vis->gx;
	fixed_t ty = vis->gy;

	struct bbox_config bb = {
		.height = vis->thingheight,
		.tz = vis->texturemid,
		.color = R_GetBoundingBoxColor(vis->mobj),
	};







	draw_bbox_col(&bb, 0, tx, ty);  
	draw_bbox_col(&bb, 1, tx - rc, ty + rs);  



	tx += rs;
	ty += rc;

	draw_bbox_col(&bb, 2, tx, ty);  
	draw_bbox_col(&bb, 3, tx - rc, ty + rs);  



	draw_bbox_row(&bb, 0, 1);
	draw_bbox_row(&bb, 1, 3);
	draw_bbox_row(&bb, 3, 2);
	draw_bbox_row(&bb, 2, 0);
}

static boolean is_tangible (mobj_t *thing)
{

	if (thing->flags & (MF_NOCLIPTHING))
	{
		return false;
	}


	if ((thing->flags & (MF_SPECIAL|MF_SOLID|MF_SHOOTABLE
					|MF_PUSHABLE|MF_BOSS|MF_MISSILE|MF_SPRING
					|MF_BOUNCE|MF_MONITOR|MF_FIRE|MF_ENEMY
					|MF_PAIN|MF_STICKY
					|MF_GRENADEBOUNCE)) == 0U)
	{
		return false;
	}

	return true;
}

boolean R_ThingBoundingBoxVisible(mobj_t *thing)
{
	INT32 cvmode = cv_renderhitbox.value;
	boolean ring = false;

	switch (thing->type)
	{
		default:



			if (thing != r_viewmobj)
			{
				break;
			}


		case MT_SKYBOX:



			return false;

		case MT_RING:
		case MT_BLUESPHERE:
		case MT_NIGHTSSTAR:
		case MT_NIGHTSCHIP:
		case MT_COIN:



			ring = true;
			break;
	}

	switch (cvmode)
	{
		case RENDERHITBOX_OFF:
			return false;

		case RENDERHITBOX_ALL:
			return true;

		case RENDERHITBOX_INTANGIBLE:
			return !is_tangible(thing);

		case RENDERHITBOX_TANGIBLE:
			return !ring && is_tangible(thing);

		case RENDERHITBOX_RINGS:
			return ring;

		default:
			return false;
	}
}
