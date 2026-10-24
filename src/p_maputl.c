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

#include "p_local.h"
#include "r_main.h"
#include "r_data.h"
#include "r_textures.h"
#include "p_maputl.h"
#include "p_polyobj.h"
#include "p_slopes.h"
#include "z_zone.h"





fixed_t P_AproxDistance(fixed_t dx, fixed_t dy)
{
	dx = abs(dx);
	dy = abs(dy);
	if (dx < dy)
		return dx + dy - (dx>>1);
	return dx + dy - (dy>>1);
}





void P_ClosestPointOnLine(fixed_t x, fixed_t y, line_t *line, vertex_t *result)
{
	fixed_t startx = line->v1->x;
	fixed_t starty = line->v1->y;
	fixed_t dx = line->dx;
	fixed_t dy = line->dy;


	fixed_t cx, cy;
	fixed_t vx, vy;
	fixed_t magnitude;
	fixed_t t;


	cx = x - startx;
	cy = y - starty;


	vx = dx;
	vy = dy;


	magnitude = R_PointToDist2(line->v2->x, line->v2->y, startx, starty);
	vx = FixedDiv(vx, magnitude);
	vy = FixedDiv(vy, magnitude);

	t = (FixedMul(vx, cx) + FixedMul(vy, cy));


	vx = FixedMul(vx, t);
	vy = FixedMul(vy, t);


	result->x = startx + vx;
	result->y = starty + vy;
	return;
}


void P_ClosestPointOnLine3D(const vector3_t *p, const vector3_t *Line, vector3_t *result)
{
	const vector3_t* v1 = &Line[0];
	const vector3_t* v2 = &Line[1];
	vector3_t c, V, n;
	fixed_t t, d;
	FV3_SubEx(v2, v1, &V);
	FV3_SubEx(p, v1, &c);

	d = R_PointToDist2(0, v2->z, R_PointToDist2(v2->x, v2->y, v1->x, v1->y), v1->z);
	FV3_Copy(&n, &V);
	FV3_Divide(&n, d);

	t = FV3_Dot(&n, &c);


	if (t <= 0)
	{
		FV3_Copy(result, v1);
		return;
	}
	else if (t >= d)
	{
		FV3_Copy(result, v2);
		return;
	}

	FV3_Mul(&n, t);

	FV3_AddEx(v1, &n, result);
	return;
}





INT32 P_PointOnLineSide(fixed_t x, fixed_t y, line_t *line)
{
	const vertex_t *v1 = line->v1;
	fixed_t dx, dy, left, right;

	if (!line->dx)
	{
		if (x <= v1->x)
			return (line->dy > 0);

		return (line->dy < 0);
	}
	if (!line->dy)
	{
		if (y <= v1->y)
			return (line->dx < 0);

		return (line->dx > 0);
	}

	dx = (x - v1->x);
	dy = (y - v1->y);

	left = FixedMul(line->dy>>FRACBITS, dx);
	right = FixedMul(dy, line->dx>>FRACBITS);

	if (right < left)
		return 0;  
	return 1;  
}






INT32 P_BoxOnLineSide(fixed_t *tmbox, line_t *ld)
{
	INT32 p1, p2;

	switch (ld->slopetype)
	{
		case ST_HORIZONTAL:
			p1 = tmbox[BOXTOP] > ld->v1->y;
			p2 = tmbox[BOXBOTTOM] > ld->v1->y;
			if (ld->dx < 0)
			{
				p1 ^= 1;
				p2 ^= 1;
			}
			break;

		case ST_VERTICAL:
			p1 = tmbox[BOXRIGHT] < ld->v1->x;
			p2 = tmbox[BOXLEFT] < ld->v1->x;
			if (ld->dy < 0)
			{
				p1 ^= 1;
				p2 ^= 1;
			}
			break;

		case ST_POSITIVE:
			p1 = P_PointOnLineSide(tmbox[BOXLEFT], tmbox[BOXTOP], ld);
			p2 = P_PointOnLineSide(tmbox[BOXRIGHT], tmbox[BOXBOTTOM], ld);
			break;

		case ST_NEGATIVE:
			p1 = P_PointOnLineSide(tmbox[BOXRIGHT], tmbox[BOXTOP], ld);
			p2 = P_PointOnLineSide(tmbox[BOXLEFT], tmbox[BOXBOTTOM], ld);
			break;

		default:
			I_Error("P_BoxOnLineSide: unknown slopetype %d\n", ld->slopetype);
			return -1;
	}

	if (p1 == p2)
		return p1;
	return -1;
}





static INT32 P_PointOnDivlineSide(fixed_t x, fixed_t y, divline_t *line)
{
	fixed_t dx, dy, left, right;

	if (!line->dx)
	{
		if (x <= line->x)
			return line->dy > 0;

		return line->dy < 0;
	}
	if (!line->dy)
	{
		if (y <= line->y)
			return line->dx < 0;

		return line->dx > 0;
	}

	dx = (x - line->x);
	dy = (y - line->y);


	if ((line->dy ^ line->dx ^ dx ^ dy) & 0x80000000)
	{
		if ((line->dy ^ dx) & 0x80000000)
			return 1;  
		return 0;
	}

	left = FixedMul(line->dy>>8, dx>>8);
	right = FixedMul(dy>>8, line->dx>>8);

	if (right < left)
		return 0;  
	return 1;  
}




void P_MakeDivline(line_t *li, divline_t *dl)
{
	dl->x = li->v1->x;
	dl->y = li->v1->y;
	dl->dx = li->dx;
	dl->dy = li->dy;
}






fixed_t P_InterceptVector(divline_t *v2, divline_t *v1)
{
	fixed_t frac, num, den;

	den = FixedMul(v1->dy>>8, v2->dx) - FixedMul(v1->dx>>8, v2->dy);

	if (!den)
		return 0;

	num = FixedMul((v1->x - v2->x)>>8, v1->dy) + FixedMul((v2->y - v1->y)>>8, v1->dx);
	frac = FixedDiv(num, den);

	return frac;
}






fixed_t opentop, openbottom, openrange, lowfloor, highceiling;
pslope_t *opentopslope, *openbottomslope;
ffloor_t *openfloorrover, *openceilingrover;




void P_CameraLineOpening(line_t *linedef)
{
	sector_t *front;
	sector_t *back;
	fixed_t frontfloor, frontceiling, backfloor, backceiling;

	if (linedef->sidenum[1] == NO_SIDEDEF)
	{

		openrange = 0;
		return;
	}

	front = linedef->frontsector;
	back = linedef->backsector;



	if (front->camsec >= 0)
	{

		frontfloor   = P_GetSectorFloorZAt  (&sectors[front->camsec], camera.x, camera.y);
		frontceiling = P_GetSectorCeilingZAt(&sectors[front->camsec], camera.x, camera.y);

	}
	else if (front->heightsec >= 0)
	{

		frontfloor   = P_GetSectorFloorZAt  (&sectors[front->heightsec], camera.x, camera.y);
		frontceiling = P_GetSectorCeilingZAt(&sectors[front->heightsec], camera.x, camera.y);
	}
	else
	{
		frontfloor   = P_CameraGetFloorZ  (mapcampointer, front, tmx, tmy, linedef);
		frontceiling = P_CameraGetCeilingZ(mapcampointer, front, tmx, tmy, linedef);
	}
	if (back->camsec >= 0)
	{

		backfloor   = P_GetSectorFloorZAt  (&sectors[back->camsec], camera.x, camera.y);
		backceiling = P_GetSectorCeilingZAt(&sectors[back->camsec], camera.x, camera.y);
	}
	else if (back->heightsec >= 0)
	{

		backfloor   = P_GetSectorFloorZAt  (&sectors[back->heightsec], camera.x, camera.y);
		backceiling = P_GetSectorCeilingZAt(&sectors[back->heightsec], camera.x, camera.y);
	}
	else
	{
		backfloor = P_CameraGetFloorZ(mapcampointer, back, tmx, tmy, linedef);
		backceiling = P_CameraGetCeilingZ(mapcampointer, back, tmx, tmy, linedef);
	}

	{
		fixed_t thingtop = mapcampointer->z + mapcampointer->height;

		if (frontceiling < backceiling)
		{
			opentop = frontceiling;
			highceiling = backceiling;
		}
		else
		{
			opentop = backceiling;
			highceiling = frontceiling;
		}

		if (frontfloor > backfloor)
		{
			openbottom = frontfloor;
			lowfloor = backfloor;
		}
		else
		{
			openbottom = backfloor;
			lowfloor = frontfloor;
		}


		if (front->ffloors || back->ffloors)
		{
			ffloor_t *rover;
			fixed_t delta1, delta2;


			if (front->ffloors)
				for (rover = front->ffloors; rover; rover = rover->next)
				{
					fixed_t topheight, bottomheight;
					if (!(rover->fofflags & FOF_BLOCKOTHERS) || !(rover->fofflags & FOF_RENDERALL) || !(rover->fofflags & FOF_EXISTS) || (rover->master->frontsector->flags & MSF_NOCLIPCAMERA))
						continue;

					topheight = P_CameraGetFOFTopZ(mapcampointer, front, rover, tmx, tmy, linedef);
					bottomheight = P_CameraGetFOFBottomZ(mapcampointer, front, rover, tmx, tmy, linedef);

					delta1 = abs(mapcampointer->z - (bottomheight + ((topheight - bottomheight)/2)));
					delta2 = abs(thingtop - (bottomheight + ((topheight - bottomheight)/2)));
					if (bottomheight < opentop && delta1 >= delta2)
						opentop = bottomheight;
					else if (bottomheight < highceiling && delta1 >= delta2)
						highceiling = bottomheight;

					if (topheight > openbottom && delta1 < delta2)
						openbottom = topheight;
					else if (topheight > lowfloor && delta1 < delta2)
						lowfloor = topheight;
				}


			if (back->ffloors)
				for (rover = back->ffloors; rover; rover = rover->next)
				{
					fixed_t topheight, bottomheight;
					if (!(rover->fofflags & FOF_BLOCKOTHERS) || !(rover->fofflags & FOF_RENDERALL) || !(rover->fofflags & FOF_EXISTS) || (rover->master->frontsector->flags & MSF_NOCLIPCAMERA))
						continue;

					topheight = P_CameraGetFOFTopZ(mapcampointer, back, rover, tmx, tmy, linedef);
					bottomheight = P_CameraGetFOFBottomZ(mapcampointer, back, rover, tmx, tmy, linedef);

					delta1 = abs(mapcampointer->z - (bottomheight + ((topheight - bottomheight)/2)));
					delta2 = abs(thingtop - (bottomheight + ((topheight - bottomheight)/2)));
					if (bottomheight < opentop && delta1 >= delta2)
						opentop = bottomheight;
					else if (bottomheight < highceiling && delta1 >= delta2)
						highceiling = bottomheight;

					if (topheight > openbottom && delta1 < delta2)
						openbottom = topheight;
					else if (topheight > lowfloor && delta1 < delta2)
						lowfloor = topheight;
				}
		}
		openrange = opentop - openbottom;
		return;
	}
}

void P_LineOpening(line_t *linedef, mobj_t *mobj)
{
	sector_t *front, *back;

	if (linedef->sidenum[1] == NO_SIDEDEF)
	{

		openrange = 0;
		return;
	}

	front = linedef->frontsector;
	back = linedef->backsector;

	I_Assert(front != NULL);
	I_Assert(back != NULL);

	openfloorrover = openceilingrover = NULL;
	if (linedef->polyobj)
	{

		opentop = INT32_MAX;
		openbottom = INT32_MIN;
		highceiling = INT32_MIN;
		lowfloor = INT32_MAX;
		opentopslope = openbottomslope = NULL;
	}
	else
	{  
		fixed_t frontheight, backheight;

		frontheight = P_GetCeilingZ(mobj, front, tmx, tmy, linedef);
		backheight = P_GetCeilingZ(mobj, back, tmx, tmy, linedef);

		if (frontheight < backheight)
		{
			opentop = frontheight;
			highceiling = backheight;
			opentopslope = front->c_slope;
		}
		else
		{
			opentop = backheight;
			highceiling = frontheight;
			opentopslope = back->c_slope;
		}

		frontheight = P_GetFloorZ(mobj, front, tmx, tmy, linedef);
		backheight = P_GetFloorZ(mobj, back, tmx, tmy, linedef);

		if (frontheight > backheight)
		{
			openbottom = frontheight;
			lowfloor = backheight;
			openbottomslope = front->f_slope;
		}
		else
		{
			openbottom = backheight;
			lowfloor = frontheight;
			openbottomslope = back->f_slope;
		}
	}

	if (mobj)
	{
		fixed_t thingtop = mobj->z + mobj->height;


		if (linedef->flags & ML_MIDSOLID
			&& !linedef->polyobj  
			) {
			side_t *side = &sides[linedef->sidenum[0]];
			fixed_t textop, texbottom, texheight;
			fixed_t texmid, delta1, delta2;
			INT32 texnum = R_GetTextureNum(side->midtexture);  

			if (texnum) {
				fixed_t scaley = abs(side->scaley_mid);
				fixed_t offsetvalue = FixedDiv(side->rowoffset + side->offsety_mid, scaley);
				fixed_t midopentop, midopenbottom;

				if (linedef->flags & ML_NOSKEW)
				{

					midopentop = min(front->ceilingheight, back->ceilingheight);
					midopenbottom = max(front->floorheight, back->floorheight);
				}
				else
				{
					midopentop = opentop;
					midopenbottom = openbottom;
				}


				texheight = FixedDiv(textureheight[texnum], scaley);


#if 0


				if (linedef->polyobj && (linedef->polyobj->flags & POF_TESTHEIGHT)) {
					if (linedef->flags & ML_WRAPMIDTEX && !side->repeatcnt) {  
						texbottom = back->floorheight + offsetvalue;
						textop = back->ceilingheight + offsetvalue;
					} else if (linedef->flags & ML_MIDTEX) {
						texbottom = back->floorheight + offsetvalue;
						textop = texbottom + texheight*(side->repeatcnt+1);
					} else {
						textop = back->ceilingheight + offsetvalue;
						texbottom = textop - texheight*(side->repeatcnt+1);
					}
				} else
#endif
				{
					if (linedef->flags & ML_WRAPMIDTEX && !side->repeatcnt) {  
						texbottom = midopenbottom + offsetvalue;
						textop = midopentop + offsetvalue;
					} else if (linedef->flags & ML_MIDPEG) {
						texbottom = midopenbottom + offsetvalue;
						textop = texbottom + texheight*(side->repeatcnt+1);
					} else {
						textop = midopentop + offsetvalue;
						texbottom = textop - texheight*(side->repeatcnt+1);
					}
				}

				texmid = texbottom+(textop-texbottom)/2;

				delta1 = abs(mobj->z - texmid);
				delta2 = abs(thingtop - texmid);

				if (delta1 > delta2) {  
					if (opentop > texbottom) {
						opentop = texbottom;
						if (linedef->flags & ML_NOSKEW)
							opentopslope = NULL;  
						else
							opentopslope = linedef->midtexslope;
					}
				} else {  
					if (openbottom < textop) {
						openbottom = textop;
						if (linedef->flags & ML_NOSKEW)
							openbottomslope = NULL;  
						else
							openbottomslope = linedef->midtexslope;
					}
				}
			}
		}
		if (linedef->polyobj)
		{

			if (linedef->polyobj->flags & POF_TESTHEIGHT)
			{
				const sector_t *polysec = linedef->backsector;
				fixed_t polytop, polybottom;
				fixed_t delta1, delta2;

				if (linedef->polyobj->flags & POF_CLIPPLANES)
				{
					polytop = polysec->ceilingheight;
					polybottom = polysec->floorheight;
				}
				else
				{
					polytop = INT32_MAX;
					polybottom = INT32_MIN;
				}

				delta1 = abs(mobj->z - (polybottom + ((polytop - polybottom)/2)));
				delta2 = abs(thingtop - (polybottom + ((polytop - polybottom)/2)));

				if (polybottom < opentop && delta1 >= delta2)
					opentop = polybottom;
				else if (polybottom < highceiling && delta1 >= delta2)
					highceiling = polybottom;

				if (polytop > openbottom && delta1 < delta2)
					openbottom = polytop;
				else if (polytop > lowfloor && delta1 < delta2)
					lowfloor = polytop;
			}

		}
		else
		{

			if (front->ffloors || back->ffloors)
			{
				ffloor_t *rover;
				fixed_t delta1, delta2;


				for (rover = front->ffloors; rover; rover = rover->next)
				{
					fixed_t topheight, bottomheight;
					if (!(rover->fofflags & FOF_EXISTS))
						continue;

					if (mobj->player && (P_CheckSolidLava(rover) || P_CanRunOnWater(mobj->player, rover)))
						;
					else if (!((rover->fofflags & FOF_BLOCKPLAYER && mobj->player)
						|| (rover->fofflags & FOF_BLOCKOTHERS && !mobj->player)))
						continue;

					topheight = P_GetFOFTopZ(mobj, front, rover, tmx, tmy, linedef);
					bottomheight = P_GetFOFBottomZ(mobj, front, rover, tmx, tmy, linedef);

					delta1 = abs(mobj->z - (bottomheight + ((topheight - bottomheight)/2)));
					delta2 = abs(thingtop - (bottomheight + ((topheight - bottomheight)/2)));

					if (delta1 >= delta2 && (rover->fofflags & FOF_INTANGIBLEFLATS) != FOF_PLATFORM)  
					{
						if (bottomheight < opentop) {
							opentop = bottomheight;
							opentopslope = *rover->b_slope;
							openceilingrover = rover;
						}
						else if (bottomheight < highceiling)
							highceiling = bottomheight;
					}

					if (delta1 < delta2 && (rover->fofflags & FOF_INTANGIBLEFLATS) != FOF_REVERSEPLATFORM)  
					{
						if (topheight > openbottom) {
							openbottom = topheight;
							openbottomslope = *rover->t_slope;
							openfloorrover = rover;
						}
						else if (topheight > lowfloor)
							lowfloor = topheight;
					}
				}


				for (rover = back->ffloors; rover; rover = rover->next)
				{
					fixed_t topheight, bottomheight;
					if (!(rover->fofflags & FOF_EXISTS))
						continue;

					if (mobj->player && (P_CheckSolidLava(rover) || P_CanRunOnWater(mobj->player, rover)))
						;
					else if (!((rover->fofflags & FOF_BLOCKPLAYER && mobj->player)
						|| (rover->fofflags & FOF_BLOCKOTHERS && !mobj->player)))
						continue;

					topheight = P_GetFOFTopZ(mobj, back, rover, tmx, tmy, linedef);
					bottomheight = P_GetFOFBottomZ(mobj, back, rover, tmx, tmy, linedef);

					delta1 = abs(mobj->z - (bottomheight + ((topheight - bottomheight)/2)));
					delta2 = abs(thingtop - (bottomheight + ((topheight - bottomheight)/2)));

					if (delta1 >= delta2 && (rover->fofflags & FOF_INTANGIBLEFLATS) != FOF_PLATFORM)  
					{
						if (bottomheight < opentop) {
							opentop = bottomheight;
							opentopslope = *rover->b_slope;
							openceilingrover = rover;
						}
						else if (bottomheight < highceiling)
							highceiling = bottomheight;
					}

					if (delta1 < delta2 && (rover->fofflags & FOF_INTANGIBLEFLATS) != FOF_REVERSEPLATFORM)  
					{
						if (topheight > openbottom) {
							openbottom = topheight;
							openbottomslope = *rover->t_slope;
							openfloorrover = rover;
						}
						else if (topheight > lowfloor)
							lowfloor = topheight;
					}
				}
			}
		}
	}

	openrange = opentop - openbottom;
}

static blocknode_t *freeblocks;

static blocknode_t *P_CreateBlockNode(mobj_t *thing, int x, int y)
{
	blocknode_t *block;

	if (freeblocks != NULL)
	{
		block = freeblocks;
		freeblocks = block->bnext;
	}
	else
		block = Z_Malloc(sizeof(blocknode_t), PU_LEVEL, NULL);

	block->blockindex = x + y*bmapwidth;
	block->mobj = thing;
	block->mnext = NULL;
	block->mprev = NULL;
	block->bprev = NULL;
	block->bnext = NULL;

	return block;
}

static void P_ReleaseBlockNode(blocknode_t *node)
{
	node->bnext = freeblocks;
	freeblocks = node;
}












void P_UnsetThingPosition(mobj_t *thing)
{
	I_Assert(thing != NULL);
	I_Assert(!P_MobjWasRemoved(thing));

	if (!(thing->flags & MF_NOSECTOR))
	{
		                                                   
                         
   
                                                                        
                                                                          
    

		mobj_t **sprev = thing->sprev;
		mobj_t  *snext = thing->snext;
		if ((*sprev = snext) != NULL)   
			snext->sprev = sprev;














		sector_list = thing->touching_sectorlist;
		thing->touching_sectorlist = NULL;  
	}

	if (!(thing->flags & MF_NOBLOCKMAP))
	{

		blocknode_t *block = thing->blocknode;

		while (block != NULL)
		{
			if (block->mnext != NULL)
				block->mnext->mprev = block->mprev;
			*(block->mprev) = block->mnext;
			blocknode_t *next = block->bnext;
			P_ReleaseBlockNode(block);
			block = next;
		}

		thing->blocknode = NULL;
	}
}

void P_UnsetPrecipThingPosition(precipmobj_t *thing)
{
	precipmobj_t **sprev = thing->sprev;
	precipmobj_t  *snext = thing->snext;
	if ((*sprev = snext) != NULL)   
		snext->sprev = sprev;

	precipsector_list = thing->touching_sectorlist;
	thing->touching_sectorlist = NULL;  
}







void P_SetThingPosition(mobj_t *thing)
{                                                       
	subsector_t *ss;
	sector_t *oldsec = NULL;
	fixed_t tfloorz, tceilz;

	I_Assert(thing != NULL);
	I_Assert(!P_MobjWasRemoved(thing));

	if (thing->player && thing->z <= thing->floorz && thing->subsector)
		oldsec = thing->subsector->sector;

	ss = thing->subsector = R_PointInSubsector(thing->x, thing->y);

	if (!(thing->flags & MF_NOSECTOR))
	{





		mobj_t **link = &ss->sector->thinglist;
		mobj_t *snext = *link;
		if ((thing->snext = snext) != NULL)
			snext->sprev = &thing->snext;
		thing->sprev = link;
		*link = thing;














		P_CreateSecNodeList(thing,thing->x,thing->y);
		thing->touching_sectorlist = sector_list;  
		sector_list = NULL;  
	}


	if (!(thing->flags & MF_NOBLOCKMAP))
	{

		INT32 x1 = (unsigned)(thing->x - thing->radius - bmaporgx)>>MAPBLOCKSHIFT;
		INT32 y1 = (unsigned)(thing->y - thing->radius - bmaporgy)>>MAPBLOCKSHIFT;
		INT32 x2 = (unsigned)(thing->x + thing->radius - bmaporgx)>>MAPBLOCKSHIFT;
		INT32 y2 = (unsigned)(thing->y + thing->radius - bmaporgy)>>MAPBLOCKSHIFT;

		thing->blocknode = NULL;

		blocknode_t **alink = &thing->blocknode;

		if (!(x1 >= bmapwidth || x2 < 0 || y1 >= bmapheight || y2 < 0))
		{

			x1 = max(0, x1);
			y1 = max(0, y1);
			x2 = min(bmapwidth - 1, x2);
			y2 = min(bmapheight - 1, y2);
			for (int y = y1; y <= y2; ++y)
			{
				for (int x = x1; x <= x2; ++x)
				{
					blocknode_t **link = &blocklinks[y*bmapwidth + x];
					blocknode_t *node = P_CreateBlockNode(thing, x, y);


					if ((node->mnext = *link) != NULL)
					{
						(*link)->mprev = &node->mnext;
					}
					node->mprev = link;
					*link = node;


					node->bprev = alink;
					node->bnext = NULL;
					(*alink) = node;
					alink = &node->bnext;
				}
			}
		}
	}



	if (thing->player && oldsec != NULL && thing->subsector && oldsec != thing->subsector->sector)
	{
		tfloorz = P_GetFloorZ(thing, ss->sector, thing->x, thing->y, NULL);
		tceilz = P_GetCeilingZ(thing, ss->sector, thing->x, thing->y, NULL);

		if (thing->eflags & MFE_VERTICALFLIP)
		{
			if (thing->z + thing->height >= tceilz)
				thing->eflags |= MFE_JUSTSTEPPEDDOWN;
		}
		else if (thing->z <= tfloorz)
			thing->eflags |= MFE_JUSTSTEPPEDDOWN;
	}
}







void P_SetUnderlayPosition(mobj_t *thing)
{                                                       
	subsector_t *ss;
	mobj_t **link, *lend;
	I_Assert(thing);

	ss = thing->subsector = R_PointInSubsector(thing->x, thing->y);
	link = &ss->sector->thinglist;
	for (lend = *link; lend && lend->snext; lend = lend->snext)
		;
	thing->snext = NULL;
	if (!lend)
	{
		thing->sprev = link;
		*link = thing;
	}
	else
	{
		thing->sprev = &lend->snext;
		lend->snext = thing;
	}

	P_CreateSecNodeList(thing,thing->x,thing->y);
	thing->touching_sectorlist = sector_list;  
	sector_list = NULL;  
}

void P_SetPrecipitationThingPosition(precipmobj_t *thing)
{
	subsector_t *ss = thing->subsector = R_PointInSubsector(thing->x, thing->y);

	precipmobj_t **link = &ss->sector->preciplist;
	precipmobj_t *snext = *link;
	if ((thing->snext = snext) != NULL)
		snext->sprev = &thing->snext;
	thing->sprev = link;
	*link = thing;

	P_CreatePrecipSecNodeList(thing, thing->x, thing->y);
	thing->touching_sectorlist = precipsector_list;  
	precipsector_list = NULL;  
}


















boolean P_BlockLinesIterator(INT32 x, INT32 y, boolean (*func)(line_t *))
{
	INT32 offset;
	const INT32 *list;  
	polymaplink_t *plink;  
	line_t *ld;

	if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
		return true;

	offset = y*bmapwidth + x;


	plink = polyblocklinks[offset];

	while (plink)
	{
		polyobj_t *po = plink->po;

		if (po->validcount != validcount)  
		{
			size_t i;
			po->validcount = validcount;

			for (i = 0; i < po->numLines; ++i)
			{
				if (po->lines[i]->validcount == validcount)  
					continue;
				po->lines[i]->validcount = validcount;
				if (!func(po->lines[i]))
					return false;
			}
		}
		plink = (polymaplink_t *)(plink->link.next);
	}

	offset = *(blockmap + offset);  


	for (list = blockmaplump + offset + 1; *list != -1; list++)
	{
		ld = &lines[*list];

		if (ld->validcount == validcount)
			continue;  

		ld->validcount = validcount;

		if (!func(ld))
			return false;
	}
	return true;  
}




boolean P_BlockThingsIterator(INT32 x, INT32 y, boolean (*func)(mobj_t *))
{
	blocknode_t *block, *next = NULL;

	if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
		return true;


	for (block = blocklinks[y*bmapwidth + x]; block != NULL; block = next)
	{
		next = block->mnext;  

		if (!func(block->mobj))
			return false;

		if (P_MobjWasRemoved(tmthing))  
			return true;
	}

	return true;
}

boolean P_DoBlockThingsIterate(int x1, int y1, int x2, int y2, boolean (*func)(mobj_t *))
{
	boolean status = true;

	for (INT32 bx = x1; bx <= x2; bx++)
		for (INT32 by = y1; by <= y2; by++)
			if (!P_BlockThingsIterator(bx, by, func))
				status = false;

	return status;
}

static bthingit_hash_entry_t *GetHashEntryForIterator(bthingit_t *it, int i)
{
	if (i < NUM_BTHINGIT_FIXEDHASH)
		return &it->fixedhash[i];
	else
		return &it->dynhash[i - NUM_BTHINGIT_FIXEDHASH];
}

static blocknode_t *GetBlockmapBlock(int x, int y)
{
	if (x >= 0 && y >= 0 && x < bmapwidth && y < bmapheight)
	{
		return blocklinks[y*bmapwidth + x];
	}
	else
	{

		return NULL;
	}
}

static bthingit_t *freeiters;

bthingit_t *P_NewBlockThingsIterator(int x1, int y1, int x2, int y2)
{
	bthingit_t *it;
	blocknode_t *block;

	x1 = max(0, x1);
	y1 = max(0, y1);
	x2 = min(bmapwidth - 1, x2);
	y2 = min(bmapheight - 1, y2);

	if (x1 > x2 || y1 > y2)
		return NULL;

	block = GetBlockmapBlock(x1, y1);

	if (freeiters != NULL)
	{
		it = freeiters;
		freeiters = it->freechain;
	}
	else
		it = Z_Calloc(sizeof(bthingit_t), PU_LEVEL, NULL);

	it->x1 = x1;
	it->y1 = y1;
	it->x2 = x2;
	it->y2 = y2;
	it->curx = x1;
	it->cury = y1;
	it->block = block;
	it->freechain = NULL;
	it->numfixedhash = 0;
	it->dynhashcount = 0;

	for (size_t i = 0; i < NUM_BTHINGIT_BUCKETS; i++)
		it->buckets[i] = -1;

	return it;
}

mobj_t *P_BlockThingsIteratorNext(bthingit_t *it, boolean centeronly)
{
	for (;;)
	{
		while (it->block != NULL)
		{
			mobj_t *mobj = it->block->mobj;
			blocknode_t *node = it->block;

			it->block = it->block->mnext;


			if (node->bnext == NULL && node->bprev == &mobj->blocknode)
			{

				return mobj;
			}
			else
			{

				if (centeronly)
				{
					fixed_t blockleft = (it->curx * MAPBLOCKUNITS) + bmaporgx;
					fixed_t blockright = blockleft + MAPBLOCKUNITS;
					fixed_t blockbottom = (it->cury * MAPBLOCKUNITS) + bmaporgy;
					fixed_t blocktop = blockbottom + MAPBLOCKUNITS;


					if (mobj->x >= blockleft && mobj->x < blockright &&
						mobj->y >= blockbottom && mobj->y < blocktop)
					{
						return mobj;
					}
				}
				else
				{
					bthingit_hash_entry_t *entry;
					int i;
					size_t hash = ((size_t)mobj >> 3) % NUM_BTHINGIT_BUCKETS;

					for (i = it->buckets[hash]; i >= 0; )
					{
						entry = GetHashEntryForIterator(it, i);
						if (entry->mobj == mobj)
						{

							break;
						}
						i = entry->next;
					}

					if (i < 0)
					{

						if (it->numfixedhash < NUM_BTHINGIT_FIXEDHASH)
						{
							entry = &it->fixedhash[it->numfixedhash];
							entry->next = it->buckets[hash];
							it->buckets[hash] = it->numfixedhash++;
						}
						else
						{
							if (!it->dynhash)
							{
								it->dynhashcapacity = 50;
								Z_Calloc(it->dynhashcapacity * sizeof(*it->dynhash), PU_LEVEL, &it->dynhash);
							}
							if (it->dynhashcount == it->dynhashcapacity)
							{
								it->dynhashcapacity *= 2;
								it->dynhash = Z_Realloc(it->dynhash, it->dynhashcapacity * sizeof(*it->dynhash), PU_LEVEL, &it->dynhash);
							}
							i = (int)it->dynhashcount;
							it->dynhashcount++;
							entry = &it->dynhash[i];
							entry->next = it->buckets[hash];
							it->buckets[hash] = i + NUM_BTHINGIT_FIXEDHASH;
						}

						entry->mobj = mobj;
						return mobj;
					}
				}
			}
		}

		if (++it->curx > it->x2)
		{
			it->curx = it->x1;
			if (++it->cury > it->y2)
				return NULL;
		}

		it->block = GetBlockmapBlock(it->curx, it->cury);
	}

	return NULL;
}

void P_FreeBlockThingsIterator(bthingit_t *it)
{
	if (it)
	{
		it->freechain = freeiters;
		freeiters = it;
	}
}

void P_ClearBlockNodes(void)
{
	freeblocks = NULL;
	freeiters = NULL;
}






static intercept_t *intercepts = NULL;
static intercept_t *intercept_p = NULL;

divline_t trace;
static boolean earlyout;


static void P_CheckIntercepts(void)
{
	static size_t max_intercepts = 0;
	size_t count = intercept_p - intercepts;

	if (max_intercepts <= count)
	{
		if (!max_intercepts)
			max_intercepts = 128;
		else
			max_intercepts *= 2;

		intercepts = Z_Realloc(intercepts, sizeof (*intercepts) * max_intercepts, PU_STATIC, NULL);

		intercept_p = intercepts + count;
	}
}











static boolean PIT_AddLineIntercepts(line_t *ld)
{
	INT32 s1, s2;
	fixed_t frac;
	divline_t dl;


	if (trace.dx > FRACUNIT*16 || trace.dy > FRACUNIT*16
		|| trace.dx < -FRACUNIT*16 || trace.dy < -FRACUNIT*16)
	{



		s1 = P_PointOnDivlineSide(ld->v1->x, ld->v1->y, &trace);
		s2 = P_PointOnDivlineSide(ld->v2->x, ld->v2->y, &trace);
	}
	else
	{
		s1 = P_PointOnLineSide(trace.x, trace.y, ld);
		s2 = P_PointOnLineSide(trace.x+trace.dx, trace.y+trace.dy, ld);
	}

	if (s1 == s2)
		return true;  


	P_MakeDivline(ld, &dl);
	frac = P_InterceptVector(&trace, &dl);

	if (frac < 0)
		return true;  


	if (earlyout && frac < FRACUNIT && !ld->backsector)
		return false;  

	P_CheckIntercepts();

	intercept_p->frac = frac;
	intercept_p->isaline = true;
	intercept_p->d.line = ld;
	intercept_p++;

	return true;  
}




static boolean PIT_AddThingIntercepts(mobj_t *thing)
{
	fixed_t px1, py1, px2, py2, frac;
	INT32 s1, s2;
	boolean tracepositive;
	divline_t dl;

	tracepositive = (trace.dx ^ trace.dy) > 0;


	if (tracepositive)
	{
		px1 = thing->x - thing->radius;
		py1 = thing->y + thing->radius;

		px2 = thing->x + thing->radius;
		py2 = thing->y - thing->radius;
	}
	else
	{
		px1 = thing->x - thing->radius;
		py1 = thing->y - thing->radius;

		px2 = thing->x + thing->radius;
		py2 = thing->y + thing->radius;
	}

	s1 = P_PointOnDivlineSide(px1, py1, &trace);
	s2 = P_PointOnDivlineSide(px2, py2, &trace);

	if (s1 == s2)
		return true;  

	dl.x = px1;
	dl.y = py1;
	dl.dx = px2 - px1;
	dl.dy = py2 - py1;

	frac = P_InterceptVector(&trace, &dl);

	if (frac < 0)
		return true;  

	P_CheckIntercepts();

	intercept_p->frac = frac;
	intercept_p->isaline = false;
	intercept_p->d.thing = thing;
	intercept_p++;

	return true;  
}






static boolean P_TraverseIntercepts(traverser_t func, fixed_t maxfrac)
{
	size_t count;
	fixed_t dist;
	intercept_t *scan, *in = NULL;

	count = intercept_p - intercepts;

	while (count--)
	{
		dist = INT32_MAX;
		for (scan = intercepts; scan < intercept_p; scan++)
		{
			if (scan->frac < dist)
			{
				dist = scan->frac;
				in = scan;
			}
		}

		if (dist > maxfrac)
			return true;  

		if (!func(in))
			return false;  

		in->frac = INT32_MAX;
	}

	return true;  
}








boolean P_PathTraverse(fixed_t px1, fixed_t py1, fixed_t px2, fixed_t py2,
	INT32 flags, traverser_t trav)
{
	fixed_t xt1, yt1, xt2, yt2;
	fixed_t xstep, ystep, partial, xintercept, yintercept;
	INT32 mapx, mapy, mapxstep, mapystep, count;

	earlyout = flags & PT_EARLYOUT;

	validcount++;
	intercept_p = intercepts;

	if (((px1 - bmaporgx) & (MAPBLOCKSIZE-1)) == 0)
		px1 += FRACUNIT;  

	if (((py1 - bmaporgy) & (MAPBLOCKSIZE-1)) == 0)
		py1 += FRACUNIT;  

	trace.x = px1;
	trace.y = py1;
	trace.dx = px2 - px1;
	trace.dy = py2 - py1;

	px1 -= bmaporgx;
	py1 -= bmaporgy;
	xt1 = (unsigned)px1>>MAPBLOCKSHIFT;
	yt1 = (unsigned)py1>>MAPBLOCKSHIFT;

	px2 -= bmaporgx;
	py2 -= bmaporgy;
	xt2 = (unsigned)px2>>MAPBLOCKSHIFT;
	yt2 = (unsigned)py2>>MAPBLOCKSHIFT;

	if (xt2 > xt1)
	{
		mapxstep = 1;
		partial = FRACUNIT - ((px1>>MAPBTOFRAC) & FRACMASK);
		ystep = FixedDiv(py2 - py1, abs(px2 - px1));
	}
	else if (xt2 < xt1)
	{
		mapxstep = -1;
		partial = (px1>>MAPBTOFRAC) & FRACMASK;
		ystep = FixedDiv(py2 - py1, abs(px2 - px1));
	}
	else
	{
		mapxstep = 0;
		partial = FRACUNIT;
		ystep = 256*FRACUNIT;
	}

	yintercept = (py1>>MAPBTOFRAC) + FixedMul(partial, ystep);

	if (yt2 > yt1)
	{
		mapystep = 1;
		partial = FRACUNIT - ((py1>>MAPBTOFRAC) & FRACMASK);
		xstep = FixedDiv(px2 - px1, abs(py2 - py1));
	}
	else if (yt2 < yt1)
	{
		mapystep = -1;
		partial = (py1>>MAPBTOFRAC) & FRACMASK;
		xstep = FixedDiv(px2 - px1, abs(py2 - py1));
	}
	else
	{
		mapystep = 0;
		partial = FRACUNIT;
		xstep = 256*FRACUNIT;
	}
	xintercept = (px1>>MAPBTOFRAC) + FixedMul(partial, xstep);




	mapx = xt1;
	mapy = yt1;

	for (count = 0; count < 64; count++)
	{
		if (flags & PT_ADDLINES)
			if (!P_BlockLinesIterator(mapx, mapy, PIT_AddLineIntercepts))
				return false;  

		if (flags & PT_ADDTHINGS)
			if (!P_BlockThingsIterator(mapx, mapy, PIT_AddThingIntercepts))
				return false;  

		if (mapx == xt2 && mapy == yt2)
			break;

		if ((yintercept >> FRACBITS) == mapy)
		{
			yintercept += ystep;
			mapx += mapxstep;
		}
		else if ((xintercept >> FRACBITS) == mapx)
		{
			xintercept += xstep;
			mapy += mapystep;
		}
	}

	return P_TraverseIntercepts(trav, FRACUNIT);
}












  
                                                                
                           
 
                      
              

                             
                                
                               
                              

               
                                                            
                                                             
                                                              
                                                           

                              
                               
                                           
                 
             
 
  
