// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2006      by James Haley
// Copyright (C) 2006-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------






#include "z_zone.h"

#include "doomstat.h"
#include "g_game.h"
#include "m_bbox.h"
#include "m_queue.h"
#include "p_maputl.h"
#include "p_setup.h"
#include "p_tick.h"
#include "p_local.h"
#include "p_polyobj.h"
#include "r_fps.h"
#include "r_main.h"
#include "r_state.h"
#include "r_defs.h"

  
                             

                                                                  
                                                                       
                                                                    
                                                                    
                                                        

                                                                    
                                                                    
                                                                    
                                                                       
                                                                    
                                                                       
                                                                     
               

                                                                       
                                                                       
                                                                  
                                                                    
                               

                                                                   
                                                                  
                                                                     
                                                                       
                

                                                                    

                                                                        
                                                                        
                                                                      
                                                                    
                                                                        
                                                

                                                                       
                                                                      
                                                                         
                                                                        
                                                                        
                                                                    
                                                  
  





#define BYTEANGLEMUL     (ANGLE_11hh/8)






polyobj_t *PolyObjects;
INT32 numPolyObjects;


polymaplink_t **polyblocklinks;







static polymaplink_t *bmap_freelist;  






FUNCINLINE static ATTRINLINE void Polyobj_bboxAdd(fixed_t *bbox, vertex_t *add)
{
	bbox[BOXTOP]    += add->y;
	bbox[BOXBOTTOM] += add->y;
	bbox[BOXLEFT]   += add->x;
	bbox[BOXRIGHT]  += add->x;
}

FUNCINLINE static ATTRINLINE void Polyobj_bboxSub(fixed_t *bbox, vertex_t *sub)
{
	bbox[BOXTOP]    -= sub->y;
	bbox[BOXBOTTOM] -= sub->y;
	bbox[BOXLEFT]   -= sub->x;
	bbox[BOXRIGHT]  -= sub->x;
}

FUNCINLINE static ATTRINLINE void Polyobj_vecAdd(vertex_t *dst, vertex_t *add)
{
	dst->x += add->x;
	dst->y += add->y;
}

FUNCINLINE static ATTRINLINE void Polyobj_vecSub(vertex_t *dst, vertex_t *sub)
{
	dst->x -= sub->x;
	dst->y -= sub->y;
}

FUNCINLINE static ATTRINLINE void Polyobj_vecSub2(vertex_t *dst, vertex_t *v1, vertex_t *v2)
{
	dst->x = v1->x - v2->x;
	dst->y = v1->y - v2->y;
}

boolean P_PointInsidePolyobj(polyobj_t *po, fixed_t x, fixed_t y)
{
	size_t i;

	for (i = 0; i < po->numLines; i++)
	{
		if (P_PointOnLineSide(x, y, po->lines[i]) == 0)
			return false;
	}

	return true;
}

boolean P_MobjTouchingPolyobj(polyobj_t *po, mobj_t *mo)
{
	fixed_t mbbox[4];
	size_t i;

	mbbox[BOXTOP] = mo->y + mo->radius;
	mbbox[BOXBOTTOM] = mo->y - mo->radius;
	mbbox[BOXRIGHT] = mo->x + mo->radius;
	mbbox[BOXLEFT] = mo->x - mo->radius;

	for (i = 0; i < po->numLines; i++)
	{
		if (P_BoxOnLineSide(mbbox, po->lines[i]) == -1)
			return true;
	}

	return false;
}

boolean P_MobjInsidePolyobj(polyobj_t *po, mobj_t *mo)
{
	fixed_t mbbox[4];
	size_t i;

	mbbox[BOXTOP] = mo->y + mo->radius;
	mbbox[BOXBOTTOM] = mo->y - mo->radius;
	mbbox[BOXRIGHT] = mo->x + mo->radius;
	mbbox[BOXLEFT] = mo->x - mo->radius;

	for (i = 0; i < po->numLines; i++)
	{
		if (P_BoxOnLineSide(mbbox, po->lines[i]) == 0)
			return false;
	}

	return true;
}

boolean P_BBoxInsidePolyobj(polyobj_t *po, fixed_t *bbox)
{
	size_t i;

	for (i = 0; i < po->numLines; i++)
	{
		if (P_BoxOnLineSide(bbox, po->lines[i]) == 0)
			return false;
	}

	return true;
}



static void Polyobj_GetInfo(polyobj_t *po, line_t *line)
{
	po->parent = line->args[1];
	if (po->parent == po->id)  
		po->parent = -1;

	po->translucency = max(min(line->args[2], NUMTRANSMAPS), 0);

	po->flags = POF_SOLID|POF_TESTHEIGHT|POF_RENDERSIDES|POF_RENDERPLANES;

	if (line->args[3] & TMPF_NOINSIDES)
		po->flags |= POF_ONESIDE;

	if (line->args[3] & TMPF_INTANGIBLE)
		po->flags &= ~POF_SOLID;

	if (line->args[3] & TMPF_PUSHABLESTOP)
		po->flags |= POF_PUSHABLESTOP;

	if (line->args[3] & TMPF_INVISIBLEPLANES)
		po->flags &= ~POF_RENDERPLANES;

	                                          
                                 

	if (line->args[3] & TMPF_SPLAT)
		po->flags |= POF_SPLAT;

	if (line->args[3] & TMPF_EXECUTOR)  
		po->flags |= POF_LDEXEC;


	if (line->args[3] & TMPF_CRUSH)
		po->damage = 3;

	po->triggertag = line->args[4];
}








static void Polyobj_addVertex(polyobj_t *po, vertex_t *v)
{
	size_t i;



	for (i = 0; i < po->numVertices; ++i)
	{
		if (po->vertices[i] == v)
			return;
	}


	if (po->numVertices >= po->numVerticesAlloc)
	{
		po->numVerticesAlloc = po->numVerticesAlloc ? po->numVerticesAlloc * 2 : 4;
		po->vertices =
			(vertex_t **)Z_Realloc(po->vertices,
			                       po->numVerticesAlloc * sizeof(vertex_t *),
			                       PU_LEVEL, NULL);
		po->origVerts =
			(vertex_t *)Z_Realloc(po->origVerts,
			                      po->numVerticesAlloc * sizeof(vertex_t),
			                      PU_LEVEL, NULL);

		po->tmpVerts =
			(vertex_t *)Z_Realloc(po->tmpVerts,
			                      po->numVerticesAlloc * sizeof(vertex_t),
			                      PU_LEVEL, NULL);
	}
	po->vertices[po->numVertices] = v;
	po->origVerts[po->numVertices] = *v;
	po->numVertices++;
}





static void Polyobj_addLine(polyobj_t *po, line_t *l)
{
	size_t i;



	for (i = 0; i < po->numLines; ++i)
	{
		if (po->lines[i] == l)
			return;
	}


	if (po->numLines >= po->numLinesAlloc)
	{
		po->numLinesAlloc = po->numLinesAlloc ? po->numLinesAlloc * 2 : 4;
		po->lines = (line_t **)Z_Realloc(po->lines,
										po->numLinesAlloc * sizeof(line_t *),
										PU_LEVEL, NULL);
	}
	l->polyobj = po;
	po->lines[po->numLines++] = l;
}





static void Polyobj_addSeg(polyobj_t *po, seg_t *seg)
{
	if (po->segCount >= po->numSegsAlloc)
	{
		po->numSegsAlloc = po->numSegsAlloc ? po->numSegsAlloc * 2 : 4;
		po->segs = (seg_t **)Z_Realloc(po->segs,
										po->numSegsAlloc * sizeof(seg_t *),
										PU_LEVEL, NULL);
	}

	seg->polyseg = po;

	po->segs[po->segCount++] = seg;





	Polyobj_addVertex(po, seg->v1);
	Polyobj_addVertex(po, seg->v2);
	Polyobj_addLine(po, seg->linedef);
}







static void Polyobj_findSegs(polyobj_t *po, seg_t *seg)
{
	fixed_t startx, starty;
	size_t i;
	size_t s;

	Polyobj_addSeg(po, seg);

	if (!(po->flags & POF_ONESIDE))
	{

		for (s = 0;  s < numsegs; s++)
		{
			size_t r;

			if (segs[s].glseg)
				continue;

			if (segs[s].linedef != seg->linedef)
				continue;

			if (segs[s].side != 1)
				continue;

			for (r = 0; r < po->segCount; r++)
			{
				if (po->segs[r] == &segs[s])
					break;
			}

			if (r != po->segCount)
				continue;

			segs[s].dontrenderme = true;

			Polyobj_addSeg(po, &segs[s]);
		}
	}


	startx = seg->v1->x;
	starty = seg->v1->y;


newseg:



	if (seg->v2->x == startx && seg->v2->y == starty)
		return;



	for (i = 0; i < numsegs; ++i)
	{
		size_t q;

		if (segs[i].glseg)
			continue;
		if (segs[i].side != 0)  
			continue;
		if (segs[i].v1->x != seg->v2->x)
			continue;
		if (segs[i].v1->y != seg->v2->y)
			continue;


		for (q = 0; q < po->segCount; q++)
		{
			if (po->segs[q] == &segs[i])
				break;
		}

		if (q != po->segCount)
			continue;


		Polyobj_addSeg(po, &segs[i]);
		seg = &segs[i];

		if (!(po->flags & POF_ONESIDE))
		{

			for (q = 0; q < numsegs; q++)
			{
				size_t r;

				if (segs[q].glseg)
					continue;
				if (segs[q].linedef != segs[i].linedef)
					continue;
				if (segs[q].side != 1)
					continue;

				for (r = 0; r < po->segCount; r++)
				{
					if (po->segs[r] == &segs[q])
						break;
				}

				if (r != po->segCount)
					continue;

				segs[q].dontrenderme = true;
				Polyobj_addSeg(po, &segs[q]);
			}
		}

		goto newseg;
	}



	po->isBad = true;
	CONS_Debug(DBG_POLYOBJ, "Polyobject %d is not closed\n", po->id);
}



static void Polyobj_spawnPolyObj(INT32 num, mobj_t *spawnSpot, INT32 id)
{
	size_t i;
	polyobj_t *po = &PolyObjects[num];


	if (po->segCount)
	{
		CONS_Debug(DBG_POLYOBJ, "Polyobj %d has more than one spawn spot", po->id);
		return;
	}

	po->id = id;



	po->thrust = FRACUNIT;
	po->spawnflags = po->flags = 0;




	for (i = 0; i < numsegs; ++i)
	{
		seg_t *seg = &segs[i];

		if (seg->glseg)
			continue;

		if (seg->side != 0)  
			continue;

		if (seg->linedef->special != POLYOBJ_START_LINE)
			continue;

		if (seg->linedef->args[0] != po->id)
			continue;

		Polyobj_GetInfo(po, seg->linedef);  


		po->spawnflags = po->flags;
		po->spawntrans = po->translucency;

		Polyobj_findSegs(po, seg);
		break;
	}

	CONS_Debug(DBG_POLYOBJ, "PO ID: %d; Num verts: %s\n", po->id, sizeu1(po->numVertices));


	if (po->isBad)
		return;


	if (po->segCount == 0)
	{
		po->isBad = true;
		CONS_Debug(DBG_POLYOBJ, "Polyobject %d is empty\n", po->id);
		return;
	}


	po->spawnSpot.x = spawnSpot->x;
	po->spawnSpot.y = spawnSpot->y;


	if (Polyobj_GetForNum(po->id))
	{

		po->isBad = true;
		CONS_Debug(DBG_POLYOBJ, "Polyobject id conflict: %d\n", id);
	}
	else
	{
		INT32 hashkey = po->id % numPolyObjects;
		po->next = PolyObjects[hashkey].first;
		PolyObjects[hashkey].first = num;
	}
}

static void Polyobj_attachToSubsec(polyobj_t *po);



static void Polyobj_moveToSpawnSpot(mapthing_t *anchor)
{
	polyobj_t *po;
	vertex_t  dist, sspot;
	size_t i;
	mtag_t tag = Tag_FGet(&anchor->tags);

	if (!(po = Polyobj_GetForNum(tag)))
	{
		CONS_Debug(DBG_POLYOBJ, "Bad polyobject %d for anchor point\n", tag);
		return;
	}


	if (po->isBad)
		return;


	if (po->attached)
	{
		CONS_Debug(DBG_POLYOBJ, "Polyobj %d has more than one anchor\n", po->id);
		return;
	}

	sspot.x = po->spawnSpot.x;
	sspot.y = po->spawnSpot.y;


	dist.x = (anchor->x << FRACBITS) - sspot.x;
	dist.y = (anchor->y << FRACBITS) - sspot.y;


	for (i = 0; i < po->numLines; ++i)
		Polyobj_bboxSub(po->lines[i]->bbox, &dist);


	for (i = 0; i < po->numVertices; ++i)
	{
		Polyobj_vecSub(po->vertices[i], &dist);

		Polyobj_vecSub2(&(po->origVerts[i]), po->vertices[i], &sspot);
	}


	Polyobj_attachToSubsec(po);
}


static void Polyobj_attachToSubsec(polyobj_t *po)
{
	subsector_t  *ss;
	fixed_t center_x = 0, center_y = 0;
	fixed_t numVertices;
	size_t i;


	if (po->isBad)
		return;

	numVertices = (fixed_t)(po->numVertices*FRACUNIT);

	for (i = 0; i < po->numVertices; ++i)
	{
		center_x += FixedDiv(po->vertices[i]->x, numVertices);
		center_y += FixedDiv(po->vertices[i]->y, numVertices);
	}

	po->centerPt.x = center_x;
	po->centerPt.y = center_y;

	ss = R_PointInSubsector(po->centerPt.x, po->centerPt.y);

	M_DLListInsert(&po->link, (mdllistitem_t **)(void *)(&ss->polyList));

#ifdef R_LINKEDPORTALS

	po->spawnSpot.groupid = ss->sector->groupid;
#endif

	po->attached = true;
}


static void Polyobj_removeFromSubsec(polyobj_t *po)
{
	if (po->attached)
	{
		M_DLListRemove(&po->link);
		po->attached = false;
	}
}




static polymaplink_t *Polyobj_getLink(void)
{
	polymaplink_t *l;

	if (bmap_freelist)
	{
		l = bmap_freelist;
		bmap_freelist = (polymaplink_t *)(l->link.next);
	}
	else
	{
		l = Z_Malloc(sizeof(*l), PU_LEVEL, NULL);
		memset(l, 0, sizeof(*l));
	}

	return l;
}


static void Polyobj_putLink(polymaplink_t *l)
{
	memset(l, 0, sizeof(*l));
	l->link.next = (mdllistitem_t *)bmap_freelist;
	bmap_freelist = l;
}





static void Polyobj_linkToBlockmap(polyobj_t *po)
{
	fixed_t *blockbox = po->blockbox;
	size_t i;
	fixed_t x, y;


	if (po->isBad || po->linked)
		return;


	blockbox[BOXLEFT]   = blockbox[BOXRIGHT] = po->vertices[0]->x;
	blockbox[BOXBOTTOM] = blockbox[BOXTOP]   = po->vertices[0]->y;


	for (i = 1; i < po->numVertices; ++i)
		M_AddToBox(blockbox, po->vertices[i]->x, po->vertices[i]->y);


	blockbox[BOXRIGHT]  = (unsigned)(blockbox[BOXRIGHT]  - bmaporgx) >> MAPBLOCKSHIFT;
	blockbox[BOXLEFT]   = (unsigned)(blockbox[BOXLEFT]   - bmaporgx) >> MAPBLOCKSHIFT;
	blockbox[BOXTOP]    = (unsigned)(blockbox[BOXTOP]    - bmaporgy) >> MAPBLOCKSHIFT;
	blockbox[BOXBOTTOM] = (unsigned)(blockbox[BOXBOTTOM] - bmaporgy) >> MAPBLOCKSHIFT;


	for (y = blockbox[BOXBOTTOM]; y <= blockbox[BOXTOP]; ++y)
	{
		for (x = blockbox[BOXLEFT]; x <= blockbox[BOXRIGHT]; ++x)
		{
			if (!(x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight))
			{
				polymaplink_t  *l = Polyobj_getLink();

				l->po = po;

				M_DLListInsert(&l->link,
							(mdllistitem_t **)(&polyblocklinks[y*bmapwidth + x]));
			}
		}
	}

	po->linked = true;
}



static void Polyobj_removeFromBlockmap(polyobj_t *po)
{
	polymaplink_t *rover;
	fixed_t *blockbox = po->blockbox;
	INT32 x, y;


	if (!po->linked)
		return;


	for (y = blockbox[BOXBOTTOM]; y <= blockbox[BOXTOP]; ++y)
	{
		for (x = blockbox[BOXLEFT]; x <= blockbox[BOXRIGHT]; ++x)
		{
			if (!(x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight))
			{
				rover = polyblocklinks[y * bmapwidth + x];

				while (rover && rover->po != po)
					rover = (polymaplink_t *)(rover->link.next);


				if (!rover)
					continue;


				M_DLListRemove(&rover->link);
				Polyobj_putLink(rover);
			}
		}
	}

	po->linked = false;
}






static inline boolean Polyobj_untouched(line_t *ld, mobj_t *mo)
{
	fixed_t x, y, ptmbbox[4];

	return
		(ptmbbox[BOXRIGHT]  = (x = mo->x) + mo->radius) <= ld->bbox[BOXLEFT]   ||
		(ptmbbox[BOXLEFT]   =           x - mo->radius) >= ld->bbox[BOXRIGHT]  ||
		(ptmbbox[BOXTOP]    = (y = mo->y) + mo->radius) <= ld->bbox[BOXBOTTOM] ||
		(ptmbbox[BOXBOTTOM] =           y - mo->radius) >= ld->bbox[BOXTOP]    ||
		P_BoxOnLineSide(ptmbbox, ld) != -1;
}




static void Polyobj_pushThing(polyobj_t *po, line_t *line, mobj_t *mo)
{
	angle_t lineangle;
	fixed_t momx, momy;
	vertex_t closest;


	lineangle = line->angle - ANGLE_90;
	lineangle >>= ANGLETOFINESHIFT;
	momx = FixedMul(po->thrust, FINECOSINE(lineangle));
	momy = FixedMul(po->thrust, FINESINE(lineangle));
	mo->momx += momx;
	mo->momy += momy;


	P_UnsetThingPosition(mo);
	P_ClosestPointOnLine(mo->x, mo->y, line, &closest);
	mo->x = closest.x + FixedMul(mo->radius, FINECOSINE(lineangle));
	mo->y = closest.y + FixedMul(mo->radius, FINESINE(lineangle));
	mo->x += momx;
	mo->y += momy;
	P_SetThingPosition(mo);


	if (po->damage && (mo->flags & MF_SHOOTABLE))
	{
		P_CheckPosition(mo, mo->x + momx, mo->y + momy);
		mo->floorz = tmfloorz;
		mo->ceilingz = tmceilingz;
		mo->floorrover = tmfloorrover;
		mo->ceilingrover = tmceilingrover;
	}
}


static void Polyobj_slideThing(mobj_t *mo, fixed_t dx, fixed_t dy)
{
	if (mo->player) {  
		fixed_t cdx, cdy;

		cdx = FixedMul(dx, FRACUNIT-CARRYFACTOR);
		cdy = FixedMul(dy, FRACUNIT-CARRYFACTOR);

		if (mo->player->onconveyor == 1)
		{
			mo->momx += cdx;
			mo->momy += cdy;


			mo->player->cmomx += cdx;
			mo->player->cmomy += cdy;
		}
		else
		{
			if (mo->player->onconveyor == 3)
			{
				mo->momx += cdx - mo->player->cmomx;
				mo->momy += cdy - mo->player->cmomy;
			}

			mo->player->cmomx = cdx;
			mo->player->cmomy = cdy;
		}

		dx = FixedMul(dx, FRACUNIT - mo->friction);
		dy = FixedMul(dy, FRACUNIT - mo->friction);

		if (mo->player->pflags & PF_SPINNING && (mo->player->rmomx || mo->player->rmomy) && !(mo->player->pflags & PF_STARTDASH)) {
#define SPINMULT 5184  
			dx = FixedMul(dx, SPINMULT);
			dy = FixedMul(dy, SPINMULT);
#undef SPINMULT
		}

		mo->momx += dx;
		mo->momy += dy;

		mo->player->onconveyor = 1;
	} else
		P_TryMove(mo, mo->x+dx, mo->y+dy, true);
}


static void Polyobj_carryThings(polyobj_t *po, fixed_t dx, fixed_t dy)
{
	static INT32 pomovecount = 0;
	INT32 x, y;

	pomovecount++;

	if (!(po->flags & POF_SOLID))
		return;

	for (y = po->blockbox[BOXBOTTOM]; y <= po->blockbox[BOXTOP]; ++y)
	{
		for (x = po->blockbox[BOXLEFT]; x <= po->blockbox[BOXRIGHT]; ++x)
		{
			mobj_t *mo;
			blocknode_t *block;
			blocknode_t *next = NULL;

			if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
				continue;

			for (block = blocklinks[y * bmapwidth + x]; block != NULL; block = next)
			{
				mo = block->mobj;
				next = block->mnext;

				if (mo->lastlook == pomovecount)
					continue;

				mo->lastlook = pomovecount;


				if (mo->flags & MF_NOGRAVITY)
					continue;


				if (mo->flags & MF_NOCLIP)
					continue;

				if ((mo->eflags & MFE_VERTICALFLIP) && mo->z + mo->height != po->lines[0]->backsector->floorheight)
					continue;

				if (!(mo->eflags & MFE_VERTICALFLIP) && mo->z != po->lines[0]->backsector->ceilingheight)
					continue;

				if (!P_MobjInsidePolyobj(po, mo))
					continue;

				Polyobj_slideThing(mo, dx, dy);
			}
		}
	}
}



static INT32 Polyobj_clipThings(polyobj_t *po, line_t *line)
{
	INT32 hitflags = 0;
	fixed_t linebox[4];
	INT32 x, y;

	if (!(po->flags & POF_SOLID))
		return hitflags;


	linebox[BOXLEFT]   = (unsigned)(line->bbox[BOXLEFT]   - bmaporgx) >> MAPBLOCKSHIFT;
	linebox[BOXRIGHT]  = (unsigned)(line->bbox[BOXRIGHT]  - bmaporgx) >> MAPBLOCKSHIFT;
	linebox[BOXBOTTOM] = (unsigned)(line->bbox[BOXBOTTOM] - bmaporgy) >> MAPBLOCKSHIFT;
	linebox[BOXTOP]    = (unsigned)(line->bbox[BOXTOP]    - bmaporgy) >> MAPBLOCKSHIFT;


	for (y = linebox[BOXBOTTOM]; y <= linebox[BOXTOP]; ++y)
	{
		for (x = linebox[BOXLEFT]; x <= linebox[BOXRIGHT]; ++x)
		{
			if (!(x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight))
			{
				mobj_t *mo = NULL;
				blocknode_t *block = blocklinks[y * bmapwidth + x];
				blocknode_t *next = NULL;

				for (; block != NULL; block = next)
				{
					next = block->mnext;
					mo = block->mobj;


					if (mo->flags & MF_NOGRAVITY)
						continue;


					if (mo->flags & MF_NOCLIP)
						continue;

					if (mo->z + mo->height <= line->backsector->floorheight)
						continue;

					if (mo->z >= line->backsector->ceilingheight)
						continue;

					if (Polyobj_untouched(line, mo))
						continue;

					if (mo->flags & MF_PUSHABLE && (po->flags & POF_PUSHABLESTOP))
						hitflags |= 2;
					else
						Polyobj_pushThing(po, line, mo);

					if (mo->player && (po->lines[0]->backsector->flags & MSF_TRIGGERSPECIAL_TOUCH) && !(po->flags & POF_NOSPECIALS))
						P_ProcessSpecialSector(mo->player, mo->subsector->sector, po->lines[0]->backsector);

					hitflags |= 1;
				}
			}  
		}  
	}  

	return hitflags;
}



boolean Polyobj_moveXY(polyobj_t *po, fixed_t x, fixed_t y, boolean checkmobjs)
{
	size_t i;
	vertex_t vec;
	INT32 hitflags = 0;

	vec.x = x;
	vec.y = y;


	if (po->isBad)
		return false;


	for (i = 0; i < po->numVertices; ++i)
		Polyobj_vecAdd(po->vertices[i], &vec);


	for (i = 0; i < po->numLines; ++i)
		Polyobj_bboxAdd(po->lines[i]->bbox, &vec);

	if (checkmobjs)
	{

		for (i = 0; i < po->numLines; ++i)
			hitflags |= Polyobj_clipThings(po, po->lines[i]);
	}

	if (hitflags & 2)
	{

		for (i = 0; i < po->numVertices; ++i)
			Polyobj_vecSub(po->vertices[i], &vec);


		for (i = 0; i < po->numLines; ++i)
			Polyobj_bboxSub(po->lines[i]->bbox, &vec);
	}
	else
	{

		po->spawnSpot.x += vec.x;
		po->spawnSpot.y += vec.y;

		if (checkmobjs)
			Polyobj_carryThings(po, x, y);
		Polyobj_removeFromBlockmap(po);  
		Polyobj_removeFromSubsec(po);    
		Polyobj_linkToBlockmap(po);      
		Polyobj_attachToSubsec(po);      
	}

	return !(hitflags & 2);
}





static inline void Polyobj_rotatePoint(vertex_t *v, const vector2_t *c, angle_t ang)
{
	vertex_t tmp = *v;

	v->x = FixedMul(tmp.x, FINECOSINE(ang)) - FixedMul(tmp.y,   FINESINE(ang));
	v->y = FixedMul(tmp.x,   FINESINE(ang)) + FixedMul(tmp.y, FINECOSINE(ang));

	v->x += c->x;
	v->y += c->y;
}



static void Polyobj_rotateLine(line_t *ld)
{
	vertex_t *v1, *v2;

	v1 = ld->v1;
	v2 = ld->v2;


	ld->dx = v2->x - v1->x;
	ld->dy = v2->y - v1->y;

	ld->angle = R_PointToAngle2(0, 0, ld->dx, ld->dy);


	ld->slopetype = !ld->dx ? ST_VERTICAL : !ld->dy ? ST_HORIZONTAL :
			((ld->dy > 0) == (ld->dx > 0)) ? ST_POSITIVE : ST_NEGATIVE;


	if (v1->x < v2->x)
	{
		ld->bbox[BOXLEFT]  = v1->x;
		ld->bbox[BOXRIGHT] = v2->x;
	}
	else
	{
		ld->bbox[BOXLEFT]  = v2->x;
		ld->bbox[BOXRIGHT] = v1->x;
	}

	if (v1->y < v2->y)
	{
		ld->bbox[BOXBOTTOM] = v1->y;
		ld->bbox[BOXTOP]    = v2->y;
	}
	else
	{
		ld->bbox[BOXBOTTOM] = v2->y;
		ld->bbox[BOXTOP]    = v1->y;
	}
}


static void Polyobj_rotateThings(polyobj_t *po, vector2_t origin, angle_t delta, boolean turnplayers, boolean turnothers)
{
	static INT32 pomovecount = 10000;
	INT32 x, y;
	angle_t deltafine = (((po->angle + delta) >> ANGLETOFINESHIFT) - (po->angle >> ANGLETOFINESHIFT)) & FINEMASK;


	pomovecount++;

	if (!(po->flags & POF_SOLID))
		return;

	for (y = po->blockbox[BOXBOTTOM]; y <= po->blockbox[BOXTOP]; ++y)
	{
		for (x = po->blockbox[BOXLEFT]; x <= po->blockbox[BOXRIGHT]; ++x)
		{
			mobj_t *mo;
			blocknode_t *block;
			blocknode_t *next = NULL;

			if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
				continue;

			for (block = blocklinks[y * bmapwidth + x]; block != NULL; block = next)
			{
				mo = block->mobj;
				next = block->mnext;

				if (mo->lastlook == pomovecount)
					continue;

				mo->lastlook = pomovecount;


				if (mo->flags & MF_NOGRAVITY)
					continue;


				if (mo->flags & MF_NOCLIP)
					continue;

				if ((mo->eflags & MFE_VERTICALFLIP) && mo->z + mo->height != po->lines[0]->backsector->floorheight)
					continue;

				if (!(mo->eflags & MFE_VERTICALFLIP) && mo->z != po->lines[0]->backsector->ceilingheight)
					continue;

				if (!P_MobjInsidePolyobj(po, mo))
					continue;

				{
					fixed_t oldxoff, oldyoff, newxoff, newyoff;
					fixed_t c, s;

					c = FINECOSINE(deltafine);
					s = FINESINE(deltafine);

					oldxoff = mo->x-origin.x;
					oldyoff = mo->y-origin.y;

					newxoff = FixedMul(oldxoff, c)-FixedMul(oldyoff, s) - oldxoff;
					newyoff = FixedMul(oldyoff, c)+FixedMul(oldxoff, s) - oldyoff;

					Polyobj_slideThing(mo, newxoff, newyoff);

					if ((turnplayers && mo->player) || (turnothers && !mo->player)) {
						mo->angle += delta;
						if (mo->player)
							P_SetPlayerAngle(mo->player, (angle_t)(mo->player->angleturn << 16) + delta);
					}
				}
			}
		}
	}
}


boolean Polyobj_rotate(polyobj_t *po, angle_t delta, boolean turnplayers, boolean turnothers, boolean checkmobjs)
{
	size_t i;
	angle_t angle;
	vector2_t origin;
	INT32 hitflags = 0;


	if (po->isBad)
		return false;

	angle = (po->angle + delta) >> ANGLETOFINESHIFT;


	origin.x = po->spawnSpot.x;
	origin.y = po->spawnSpot.y;


	for (i = 0; i < po->numVertices; ++i)
	{
		po->tmpVerts[i] = *(po->vertices[i]);


		*(po->vertices[i]) = po->origVerts[i];

		Polyobj_rotatePoint(po->vertices[i], &origin, angle);
	}


	for (i = 0; i < po->numLines; ++i)
		Polyobj_rotateLine(po->lines[i]);

	if (checkmobjs)
	{

		for (i = 0; i < po->numLines; ++i)
			hitflags |= Polyobj_clipThings(po, po->lines[i]);

		Polyobj_rotateThings(po, origin, delta, turnplayers, turnothers);
	}

	if (hitflags & 2)
	{

		for (i = 0; i < po->numVertices; ++i)
			*(po->vertices[i]) = po->tmpVerts[i];


		for (i = 0; i < po->numLines; ++i)
			Polyobj_rotateLine(po->lines[i]);
	}
	else
	{

		for (i = 0; i < po->segCount; ++i)
			po->segs[i]->angle += delta;


		po->angle += delta;

		Polyobj_removeFromBlockmap(po);  
		Polyobj_removeFromSubsec(po);    
		Polyobj_linkToBlockmap(po);      
		Polyobj_attachToSubsec(po);      
	}

	return !(hitflags & 2);
}







polyobj_t *Polyobj_GetForNum(INT32 id)
{
	if (numPolyObjects == 0)
		return NULL;
	INT32 curidx  = PolyObjects[id % numPolyObjects].first;

	while (curidx != numPolyObjects && PolyObjects[curidx].id != id)
		curidx = PolyObjects[curidx].next;

	return curidx == numPolyObjects ? NULL : &PolyObjects[curidx];
}




#if 0  
static polyobj_t *Polyobj_GetParent(polyobj_t *po)
{
	return (po && po->parent != -1) ? Polyobj_GetForNum(po->parent) : NULL;
}
#endif



static polyobj_t *Polyobj_GetChild(polyobj_t *po, INT32 *start)
{
	for (; *start < numPolyObjects; (*start)++)
	{
		if (PolyObjects[*start].parent == po->id)
			return &PolyObjects[(*start)++];
	}

	return NULL;
}


typedef struct mobjqitem_s
{
	mqueueitem_t mqitem;
	mobj_t *mo;
} mobjqitem_t;



void Polyobj_InitLevel(void)
{
	thinker_t   *th;
	mqueue_t    spawnqueue;
	mqueue_t    anchorqueue;
	mobjqitem_t *qitem;
	INT32 i, numAnchors = 0;
	mobj_t *mo;

	M_QueueInit(&spawnqueue);
	M_QueueInit(&anchorqueue);




	PolyObjects    = NULL;
	numPolyObjects = 0;
	bmap_freelist  = NULL;



	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mo = (mobj_t *)th;

		if (mo->info->doomednum == POLYOBJ_SPAWN_DOOMEDNUM)
		{
			++numPolyObjects;

			qitem = malloc(sizeof(mobjqitem_t));
			memset(qitem, 0, sizeof(mobjqitem_t));
			qitem->mo = mo;
			M_QueueInsert(&(qitem->mqitem), &spawnqueue);
		}
		else if (mo->info->doomednum == POLYOBJ_ANCHOR_DOOMEDNUM)
		{
			++numAnchors;

			qitem = malloc(sizeof(mobjqitem_t));
			memset(qitem, 0, sizeof(mobjqitem_t));
			qitem->mo = mo;
			M_QueueInsert(&(qitem->mqitem), &anchorqueue);
		}
	}

	if (numPolyObjects)
	{

		PolyObjects = Z_Calloc(numPolyObjects * sizeof(polyobj_t),
													PU_LEVEL, NULL);


		for (i = 0; i < numPolyObjects; ++i)
			PolyObjects[i].first = PolyObjects[i].next = numPolyObjects;


		for (i = 0; i < numPolyObjects; ++i)
		{
			qitem = (mobjqitem_t *)M_QueueIterator(&spawnqueue);

			Polyobj_spawnPolyObj(i, qitem->mo, Tag_FGet(&qitem->mo->spawnpoint->tags));
		}


		for (i = 0; i < numAnchors; ++i)
		{
			qitem = (mobjqitem_t *)M_QueueIterator(&anchorqueue);

			Polyobj_moveToSpawnSpot((qitem->mo->spawnpoint));
		}


		for (i = 0; i < numPolyObjects; ++i)
			Polyobj_linkToBlockmap(&PolyObjects[i]);
	}

#if 0

	printf("DEBUG: numPolyObjects = %d\n", numPolyObjects);
	for (i = 0; i < numPolyObjects; ++i)
	{
		INT32 j;
		polyobj_t *po = &PolyObjects[i];

		printf("polyobj %d:\n", i);
		printf("id = %d, first = %d, next = %d\n", po->id, po->first, po->next);
		printf("segCount = %d, numSegsAlloc = %d\n", po->segCount, po->numSegsAlloc);
		for (j = 0; j < po->segCount; ++j)
			printf("\tseg %d: %p\n", j, po->segs[j]);
		printf("numVertices = %d, numVerticesAlloc = %d\n", po->numVertices, po->numVerticesAlloc);
		for (j = 0; j < po->numVertices; ++j)
		{
			printf("\tvtx %d: (%d, %d) / orig: (%d, %d)\n",
				j, po->vertices[j]->x>>FRACBITS, po->vertices[j]->y>>FRACBITS,
				po->origVerts[j].x>>FRACBITS, po->origVerts[j].y>>FRACBITS);
		}
		printf("numLines = %d, numLinesAlloc = %d\n", po->numLines, po->numLinesAlloc);
		for (j = 0; j < po->numLines; ++j)
			printf("\tline %d: %p\n", j, po->lines[j]);
		printf("spawnSpot = (%d, %d)\n", po->spawnSpot.x >> FRACBITS, po->spawnSpot.y >> FRACBITS);
		printf("centerPt = (%d, %d)\n", po->centerPt.x >> FRACBITS, po->centerPt.y >> FRACBITS);
		printf("attached = %d, linked = %d, validcount = %d, isBad = %d\n",
			po->attached, po->linked, po->validcount, po->isBad);
		printf("blockbox: [%d, %d, %d, %d]\n",
			po->blockbox[BOXLEFT], po->blockbox[BOXRIGHT], po->blockbox[BOXBOTTOM],
			po->blockbox[BOXTOP]);
	}
#endif


	M_QueueFree(&spawnqueue);
	M_QueueFree(&anchorqueue);
}








void Polyobj_MoveOnLoad(polyobj_t *po, angle_t angle, fixed_t x, fixed_t y)
{
	fixed_t dx, dy;


	Polyobj_rotate(po, angle, false, false, false);


	dx = x - po->spawnSpot.x;
	dy = y - po->spawnSpot.y;


	Polyobj_moveXY(po, dx, dy, false);
}




void T_PolyObjRotate(polyrotate_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjRotate: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjRotate: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif


	if (po->thinker == NULL)
	{
		po->thinker = &th->thinker;


		po->thrust = abs(th->speed) >> 8;
		if (po->thrust < FRACUNIT)
			po->thrust = FRACUNIT;
		else if (po->thrust > 4*FRACUNIT)
			po->thrust = 4*FRACUNIT;
	}



	if (Polyobj_rotate(po, th->speed, th->turnobjs & PTF_PLAYERS, th->turnobjs & PTF_OTHERS, true) && th->distance != -1)
	{
		INT32 avel = abs(th->speed);


		th->distance -= avel;


		if (th->distance <= 0)
		{

			if (po->thinker == &th->thinker)
			{
				po->thinker = NULL;
				po->thrust = FRACUNIT;
			}
			P_RemoveThinker(&th->thinker);



		}
		else if (th->distance < avel)
		{


			th->speed = th->speed >= 0 ? th->distance : -th->distance;
		}
	}
}


FUNCINLINE static ATTRINLINE void Polyobj_componentSpeed(INT32 resVel, INT32 angle,
                                            fixed_t *xVel, fixed_t *yVel)
{
	if (angle == 0)
	{
		*xVel = resVel;
		*yVel = 0;
	}
	else if (angle == (INT32)(ANGLE_90>>ANGLETOFINESHIFT))
	{
		*xVel = 0;
		*yVel = resVel;
	}
	else
	{
		*xVel = FixedMul(resVel, FINECOSINE(angle));
		*yVel = FixedMul(resVel,   FINESINE(angle));
	}
}

void T_PolyObjMove(polymove_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjMove: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjMove: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif


	if (po->thinker == NULL)
	{
		po->thinker = &th->thinker;


		po->thrust = abs(th->speed) >> 3;
		if (po->thrust < FRACUNIT)
			po->thrust = FRACUNIT;
		else if (po->thrust > 4*FRACUNIT)
			po->thrust = 4*FRACUNIT;
	}


	if (Polyobj_moveXY(po, th->momx, th->momy, true))
	{
		INT32 avel = abs(th->speed);


		th->distance -= avel;


		if (th->distance <= 0)
		{

			if (po->thinker == &th->thinker)
			{
				po->thinker = NULL;
				po->thrust = FRACUNIT;
			}
			P_RemoveThinker(&th->thinker);



		}
		else if (th->distance < avel)
		{


			th->speed = th->speed >= 0 ? th->distance : -th->distance;
			Polyobj_componentSpeed(th->speed, th->angle, &th->momx, &th->momy);
		}
	}
}

static void T_MovePolyObj(polyobj_t *po, fixed_t distx, fixed_t disty, fixed_t distz)
{
	polyobj_t *child;
	INT32 start;

	Polyobj_moveXY(po, distx, disty, true);

	po->lines[0]->backsector->floorheight += distz;
	po->lines[0]->backsector->ceilingheight += distz;



	P_CheckSector(po->lines[0]->backsector, (boolean)(po->damage));

	start = 0;
	while ((child = Polyobj_GetChild(po, &start)))
	{
		if (child->isBad)
			continue;

		Polyobj_moveXY(child, distx, disty, true);

		child->lines[0]->backsector->floorheight += distz;
		child->lines[0]->backsector->ceilingheight += distz;
		P_CheckSector(child->lines[0]->backsector, (boolean)(child->damage));
	}
}

void T_PolyObjWaypoint(polywaypoint_t *th)
{
	mobj_t *target = NULL;
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);
	fixed_t speed = th->speed;

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjWaypoint: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjWaypoint: thinker with invalid id %d removed.", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif


	if (!po->thinker)
		po->thinker = &th->thinker;

	target = waypoints[th->sequence][th->pointnum];

	if (!target)
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjWaypoint: Unable to find target waypoint!\n");
		return;
	}


	while (speed > 0)
	{
		mobj_t *waypoint = NULL;
		fixed_t pox, poy, poz;
		fixed_t distx, disty, distz, dist;


		pox = po->centerPt.x;
		poy = po->centerPt.y;
		poz = (po->lines[0]->backsector->floorheight + po->lines[0]->backsector->ceilingheight)/2;


		distx = target->x - pox;
		disty = target->y - poy;
		distz = target->z - poz;
		dist = P_AproxDistance(P_AproxDistance(distx, disty), distz);

		if (dist < 1)
			dist = 1;


		if (speed < dist)
		{

			fixed_t momx, momy, momz;

			momx = FixedMul(FixedDiv(target->x - pox, dist), speed);
			momy = FixedMul(FixedDiv(target->y - poy, dist), speed);
			momz = FixedMul(FixedDiv(target->z - poz, dist), speed);
			T_MovePolyObj(po, momx, momy, momz);
			return;
		}
		else
		{

			T_MovePolyObj(po, distx, disty, distz);

			if (!th->stophere)
			{
				CONS_Debug(DBG_POLYOBJ, "Looking for next waypoint...\n");
				waypoint = (th->direction == -1) ? P_GetPreviousWaypoint(target, false) : P_GetNextWaypoint(target, false);

				if (!waypoint && th->returnbehavior == PWR_WRAP)  
				{
					if (!th->continuous)
					{
						th->returnbehavior = PWR_STOP;
						th->stophere = true;
					}

					waypoint = (th->direction == -1) ? P_GetLastWaypoint(th->sequence) : P_GetFirstWaypoint(th->sequence);
				}
				else if (!waypoint && th->returnbehavior == PWR_COMEBACK)  
				{
					th->direction = -th->direction;

					if (!th->continuous)
						th->returnbehavior = PWR_STOP;

					waypoint = (th->direction == -1) ? P_GetPreviousWaypoint(target, false) : P_GetNextWaypoint(target, false);
				}
			}

			if (waypoint)
			{
				CONS_Debug(DBG_POLYOBJ, "Found waypoint (sequence %d, number %d).\n", waypoint->threshold, waypoint->health);

				target = waypoint;
				th->pointnum = target->health;


				speed -= dist;
			}
			else
			{
				if (!th->stophere)
					CONS_Debug(DBG_POLYOBJ, "Next waypoint not found!\n");

				if (po->thinker == &th->thinker)
					po->thinker = NULL;

				P_RemoveThinker(&th->thinker);
				return;
			}
		}
	}
}

void T_PolyDoorSlide(polyslidedoor_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyDoorSlide: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyDoorSlide: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif


	if (po->thinker == NULL)
	{
		po->thinker = &th->thinker;


		po->thrust = abs(th->speed) >> 3;
		if (po->thrust < FRACUNIT)
			po->thrust = FRACUNIT;
		else if (po->thrust > 4*FRACUNIT)
			po->thrust = 4*FRACUNIT;
	}


	if (th->delayCount)
	{
		if (--th->delayCount == 0)
		{
			;  
		}
		return;
	}


	if (Polyobj_moveXY(po, th->momx, th->momy, true))
	{
		INT32 avel = abs(th->speed);


		th->distance -= avel;



		if (th->distance <= 0)
		{

			if (!th->closing)
			{
				th->closing = true;


				th->distance = th->initDistance;
				th->speed    = th->initSpeed;


				th->delayCount = th->delay;


				th->angle = th->revAngle;


				Polyobj_componentSpeed(th->speed, th->angle, &th->momx, &th->momy);
			}
			else
			{

				if (po->thinker == &th->thinker)
				{
					po->thinker = NULL;
					po->thrust = FRACUNIT;
				}
				P_RemoveThinker(&th->thinker);

			}

		}
		else if (th->distance < avel)
		{



			th->speed = th->speed >= 0
				? th->distance : -th->distance;
			Polyobj_componentSpeed(th->speed, th->angle,
				&th->momx, &th->momy);
		}
	}
	else if (th->closing && th->distance != th->initDistance)
	{

		th->distance = th->initDistance - th->distance;
		th->speed    = th->initSpeed;
		th->angle    = th->initAngle;
		Polyobj_componentSpeed(th->speed, th->angle,
			&th->momx, &th->momy);
		th->closing  = false;

	}
}

void T_PolyDoorSwing(polyswingdoor_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyDoorSwing: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyDoorSwing: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif


	if (po->thinker == NULL)
	{
		po->thinker = &th->thinker;


		po->thrust = abs(th->speed) >> 3;
		if (po->thrust < FRACUNIT)
			po->thrust = FRACUNIT;
		else if (po->thrust > 4*FRACUNIT)
			po->thrust = 4*FRACUNIT;
	}


	if (th->delayCount)
	{
		if (--th->delayCount == 0)
		{
			;  
		}
		return;
	}



	if (Polyobj_rotate(po, th->speed, false, false, true) && th->distance != -1)
	{
		INT32 avel = abs(th->speed);


		th->distance -= avel;


		if (th->distance <= 0)
		{

			if (!th->closing)
			{
				th->closing = true;


				th->distance =  th->initDistance;
				th->speed    = -th->initSpeed;  


				th->delayCount = th->delay;
			}
			else
			{

				if (po->thinker == &th->thinker)
				{
					po->thinker = NULL;
					po->thrust = FRACUNIT;
				}
				P_RemoveThinker(&th->thinker);

			}

		}
		else if (th->distance < avel)
		{



			th->speed = th->speed >= 0
				? th->distance : -th->distance;
		}
	}
	else if (th->closing && th->distance != th->initDistance)
	{


		th->distance = th->initDistance - th->distance;
		th->speed    = th->initSpeed;
		th->closing  = false;


	}
}


void T_PolyObjDisplace(polydisplace_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);
	fixed_t newheights, delta;
	fixed_t dx, dy;

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjDisplace: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjDisplace: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif


	if (po->thinker == NULL)
	{
		po->thinker = &th->thinker;


		po->thrust = FRACUNIT;
	}

	newheights = th->controlSector->floorheight+th->controlSector->ceilingheight;
	delta = newheights-th->oldHeights;

	if (!delta)
		return;

	dx = FixedMul(th->dx, delta);
	dy = FixedMul(th->dy, delta);

	if (Polyobj_moveXY(po, dx, dy, true))
		th->oldHeights = newheights;
}


void T_PolyObjRotDisplace(polyrotdisplace_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);
	fixed_t newheights, delta;
	fixed_t rotangle;

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjRotDisplace: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjRotDisplace: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif


	if (po->thinker == NULL)
	{
		po->thinker = &th->thinker;


		po->thrust = FRACUNIT;
	}

	newheights = th->controlSector->floorheight+th->controlSector->ceilingheight;
	delta = newheights-th->oldHeights;

	if (!delta)
		return;

	rotangle = FixedMul(th->rotscale, delta);

	if (Polyobj_rotate(po, FixedAngle(rotangle), th->turnobjs & PTF_PLAYERS, th->turnobjs & PTF_OTHERS, true))
		th->oldHeights = newheights;
}

static inline INT32 Polyobj_AngSpeed(INT32 speed)
{
	return (speed*ANG1)>>3;  
}



boolean EV_DoPolyObjRotate(polyrotdata_t *prdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polyrotate_t *th;
	INT32 start;

	if (!(po = Polyobj_GetForNum(prdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjRotate: bad polyobj %d\n", prdata->polyObjNum);
		return false;
	}


	if (po->isBad)
		return false;


	if (po->thinker && !(prdata->flags & TMPR_OVERRIDE))
		return false;


	th = Z_Malloc(sizeof(polyrotate_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjRotate;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;


	th->polyObjNum = prdata->polyObjNum;


	th->speed = Polyobj_AngSpeed(prdata->speed * prdata->direction);

	if (prdata->flags & TMPR_CONTINUOUS)
		th->distance = -1;
	else if (prdata->distance == 360)
		th->distance = ANGLE_MAX - 1;
	else
		th->distance = FixedAngle(prdata->distance*FRACUNIT);


	po->thrust = abs(th->speed) >> 8;
	if (po->thrust < FRACUNIT)
		po->thrust = FRACUNIT;
	else if (po->thrust > 4*FRACUNIT)
		po->thrust = 4*FRACUNIT;



	oldpo = po;


	R_CreateInterpolator_Polyobj(&th->thinker, po);

	th->turnobjs = 0;
	if (!(prdata->flags & TMPR_DONTROTATEOTHERS))
		th->turnobjs |= PTF_OTHERS;
	if (prdata->flags & TMPR_ROTATEPLAYERS)
		th->turnobjs |= PTF_PLAYERS;


	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		prdata->polyObjNum = po->id;  
		EV_DoPolyObjRotate(prdata);
	}


	return true;
}

boolean EV_DoPolyObjMove(polymovedata_t *pmdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polymove_t *th;
	INT32 start;

	if (!(po = Polyobj_GetForNum(pmdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjMove: bad polyobj %d\n", pmdata->polyObjNum);
		return false;
	}


	if (po->isBad)
		return false;


	if (po->thinker && !pmdata->overRide)
		return false;


	th = Z_Malloc(sizeof(polymove_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjMove;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;


	th->polyObjNum = pmdata->polyObjNum;
	th->distance   = pmdata->distance;
	th->speed      = pmdata->speed;
	th->angle      = pmdata->angle >> ANGLETOFINESHIFT;


	Polyobj_componentSpeed(th->speed, th->angle, &th->momx, &th->momy);


	po->thrust = abs(th->speed) >> 3;
	if (po->thrust < FRACUNIT)
		po->thrust = FRACUNIT;
	else if (po->thrust > 4*FRACUNIT)
		po->thrust = 4*FRACUNIT;



	oldpo = po;


	R_CreateInterpolator_Polyobj(&th->thinker, po);


	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		pmdata->polyObjNum = po->id;  
		EV_DoPolyObjMove(pmdata);
	}


	return true;
}

boolean EV_DoPolyObjWaypoint(polywaypointdata_t *pwdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polywaypoint_t *th;
	mobj_t *first = NULL;
	INT32 start;

	if (!(po = Polyobj_GetForNum(pwdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjWaypoint: bad polyobj %d\n", pwdata->polyObjNum);
		return false;
	}


	if (po->isBad)
		return false;

	if (po->thinker)  
		return false;


	th = Z_Malloc(sizeof(polywaypoint_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjWaypoint;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;


	th->polyObjNum = pwdata->polyObjNum;
	th->speed = pwdata->speed;
	th->sequence = pwdata->sequence;
	th->direction = (pwdata->flags & PWF_REVERSE) ? -1 : 1;

	th->returnbehavior = pwdata->returnbehavior;
	if (pwdata->flags & PWF_LOOP)
		th->continuous = true;
	th->stophere = false;


	first = (th->direction == -1) ? P_GetLastWaypoint(th->sequence) : P_GetFirstWaypoint(th->sequence);

	if (!first)
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjWaypoint: Missing starting waypoint!\n");
		po->thinker = NULL;
		P_RemoveThinker(&th->thinker);
		return false;
	}



	if (th->continuous && P_IsDegeneratedWaypointSequence(th->sequence))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjWaypoint: All waypoints are in the same location!\n");
		th->continuous = false;
	}


	R_CreateInterpolator_Polyobj(&th->thinker, po);


	R_CreateInterpolator_SectorPlane(&th->thinker, po->lines[0]->backsector, false);
	R_CreateInterpolator_SectorPlane(&th->thinker, po->lines[0]->backsector, true);





	start = 0;
	oldpo = po;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		R_CreateInterpolator_Polyobj(&th->thinker, po);
		R_CreateInterpolator_SectorPlane(&th->thinker, po->lines[0]->backsector, false);
		R_CreateInterpolator_SectorPlane(&th->thinker, po->lines[0]->backsector, true);
	}

	th->pointnum = first->health;

	return true;
}

static void Polyobj_doSlideDoor(polyobj_t *po, polydoordata_t *doordata)
{
	polyslidedoor_t *th;
	polyobj_t *oldpo;
	angle_t angtemp;
	INT32 start;


	th = Z_Malloc(sizeof(polyslidedoor_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyDoorSlide;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);


	po->thinker = &th->thinker;


	th->polyObjNum = po->id;
	th->closing = false;
	th->delay = doordata->delay;
	th->delayCount = 0;
	th->distance = th->initDistance = doordata->distance;
	th->speed = th->initSpeed = doordata->speed;



	angtemp       = doordata->angle;
	th->angle     = angtemp >> ANGLETOFINESHIFT;
	th->initAngle = th->angle;
	th->revAngle  = (angtemp + ANGLE_180) >> ANGLETOFINESHIFT;

	Polyobj_componentSpeed(th->speed, th->angle, &th->momx, &th->momy);


	po->thrust = abs(th->speed) >> 3;
	if (po->thrust < FRACUNIT)
		po->thrust = FRACUNIT;
	else if (po->thrust > 4*FRACUNIT)
		po->thrust = 4*FRACUNIT;



	oldpo = po;


	R_CreateInterpolator_Polyobj(&th->thinker, po);


	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
		Polyobj_doSlideDoor(po, doordata);
}

static void Polyobj_doSwingDoor(polyobj_t *po, polydoordata_t *doordata)
{
	polyswingdoor_t *th;
	polyobj_t *oldpo;
	INT32 start;


	th = Z_Malloc(sizeof(polyswingdoor_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyDoorSwing;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);


	po->thinker = &th->thinker;


	th->polyObjNum   = po->id;
	th->closing      = false;
	th->delay        = doordata->delay;
	th->delayCount   = 0;
	th->distance     = th->initDistance = FixedAngle(doordata->distance*FRACUNIT);
	th->speed        = Polyobj_AngSpeed(doordata->speed);
	th->initSpeed    = th->speed;


	po->thrust = abs(th->speed) >> 3;
	if (po->thrust < FRACUNIT)
		po->thrust = FRACUNIT;
	else if (po->thrust > 4*FRACUNIT)
		po->thrust = 4*FRACUNIT;



	oldpo = po;


	R_CreateInterpolator_Polyobj(&th->thinker, po);


	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
		Polyobj_doSwingDoor(po, doordata);
}

boolean EV_DoPolyDoor(polydoordata_t *doordata)
{
	polyobj_t *po;

	if (!(po = Polyobj_GetForNum(doordata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyDoor: bad polyobj %d\n", doordata->polyObjNum);
		return false;
	}



	if (po->isBad || po->thinker)
		return false;

	switch (doordata->doorType)
	{
	case POLY_DOOR_SLIDE:
		Polyobj_doSlideDoor(po, doordata);
		break;
	case POLY_DOOR_SWING:
		Polyobj_doSwingDoor(po, doordata);
		break;
	default:
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyDoor: unknown door type %d", doordata->doorType);
		return false;
	}

	return true;
}

boolean EV_DoPolyObjDisplace(polydisplacedata_t *prdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polydisplace_t *th;
	INT32 start;

	if (!(po = Polyobj_GetForNum(prdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjRotate: bad polyobj %d\n", prdata->polyObjNum);
		return false;
	}


	if (po->isBad)
		return false;


	th = Z_Malloc(sizeof(polydisplace_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjDisplace;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;


	th->polyObjNum = prdata->polyObjNum;

	th->controlSector = prdata->controlSector;
	th->oldHeights = th->controlSector->floorheight+th->controlSector->ceilingheight;

	th->dx = prdata->dx;
	th->dy = prdata->dy;

	oldpo = po;


	R_CreateInterpolator_Polyobj(&th->thinker, po);


	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		prdata->polyObjNum = po->id;  
		EV_DoPolyObjDisplace(prdata);
	}


	return true;
}

boolean EV_DoPolyObjRotDisplace(polyrotdisplacedata_t *prdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polyrotdisplace_t *th;
	INT32 start;

	if (!(po = Polyobj_GetForNum(prdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjRotate: bad polyobj %d\n", prdata->polyObjNum);
		return false;
	}


	if (po->isBad)
		return false;


	th = Z_Malloc(sizeof(polyrotdisplace_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjRotDisplace;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;


	th->polyObjNum = prdata->polyObjNum;

	th->controlSector = prdata->controlSector;
	th->oldHeights = th->controlSector->floorheight+th->controlSector->ceilingheight;

	th->rotscale = prdata->rotscale;
	th->turnobjs = prdata->turnobjs;

	oldpo = po;


	R_CreateInterpolator_Polyobj(&th->thinker, po);


	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		prdata->polyObjNum = po->id;  
		EV_DoPolyObjRotDisplace(prdata);
	}


	return true;
}

void T_PolyObjFlag(polymove_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);
	size_t i;

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjFlag: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjFlag: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif


	if (po->thinker == NULL)
		po->thinker = &th->thinker;


	for (i = 0; i < po->numVertices/2; i++)
	{
		vertex_t vec;
		fixed_t sine = FINESINE(th->distance)*th->momx;

		Polyobj_componentSpeed(sine, th->angle, &vec.x, &vec.y);

		po->vertices[i]->x = po->tmpVerts[i].x;
		po->vertices[i]->y = po->tmpVerts[i].y;

		Polyobj_vecAdd(po->vertices[i], &vec);

		th->distance += th->speed;
		th->distance &= FINEMASK;
	}

	for (i = 0; i < po->numLines; i++)
		Polyobj_rotateLine(po->lines[i]);

	Polyobj_removeFromBlockmap(po);  
	Polyobj_removeFromSubsec(po);    
	Polyobj_linkToBlockmap(po);      
	Polyobj_attachToSubsec(po);      
}

boolean EV_DoPolyObjFlag(polyflagdata_t *pfdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polymove_t *th;
	size_t i;
	INT32 start;
	mtag_t tag = pfdata->polyObjNum;

	if (!(po = Polyobj_GetForNum(tag)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyFlag: bad polyobj %d\n", tag);
		return false;
	}



	if (po->isBad || po->thinker)
		return false;


	if (po->numVertices & 1)
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyFlag: Polyobject has odd # of vertices!\n");
		return false;
	}


	th = Z_Malloc(sizeof(polymove_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjFlag;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;


	th->polyObjNum = tag;
	th->distance   = 0;
	th->speed      = pfdata->speed;
	th->angle      = pfdata->angle;
	th->momx       = pfdata->momx;


	for (i = 0; i < po->numVertices; ++i)
		po->tmpVerts[i] = *(po->vertices[i]);

	oldpo = po;


	R_CreateInterpolator_Polyobj(&th->thinker, po);


	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		pfdata->polyObjNum = po->id;
		EV_DoPolyObjFlag(pfdata);
	}


	return true;
}

void T_PolyObjFade(polyfade_t *th)
{
	boolean stillfading = false;
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjFade: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjFade: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif


	if (po->thinker == NULL)
		po->thinker = &th->thinker;

	stillfading = th->ticbased ? !(--(th->timer) <= 0)
		: !((th->timer -= th->duration) <= 0);

	if (th->timer <= 0)
	{
		po->translucency = max(min(th->destvalue, NUMTRANSMAPS), 0);


		if (po->thinker == &th->thinker)
			po->thinker = NULL;
		P_RemoveThinker(&th->thinker);
	}
	else
	{
		INT16 delta = abs(th->destvalue - th->sourcevalue);
		INT32 duration = th->ticbased ? th->duration
			: abs(FixedMul(FixedDiv(256, NUMTRANSMAPS), NUMTRANSMAPS - th->destvalue)
				- FixedMul(FixedDiv(256, NUMTRANSMAPS), NUMTRANSMAPS - th->sourcevalue));  
		fixed_t factor = min(FixedDiv(duration - th->timer, duration), 1*FRACUNIT);
		if (th->destvalue < th->sourcevalue)
			po->translucency = max(min(po->translucency, th->sourcevalue - (INT16)FixedMul(delta, factor)), th->destvalue);
		else if (th->destvalue > th->sourcevalue)
			po->translucency = min(max(po->translucency, th->sourcevalue + (INT16)FixedMul(delta, factor)), th->destvalue);
	}

	if (!stillfading)
	{

		if (po->translucency >= NUMTRANSMAPS)  
			po->flags &= ~POF_RENDERALL;
		else
			po->flags |= (po->spawnflags & POF_RENDERALL);


		if (th->docollision)
		{
			if (th->destvalue > th->sourcevalue)  
			{
				po->flags &= ~POF_SOLID;
				po->flags |= POF_NOSPECIALS;
			}
			else
			{
				po->flags |= (po->spawnflags & POF_SOLID);
				if (!(po->spawnflags & POF_NOSPECIALS))
					po->flags &= ~POF_NOSPECIALS;
			}
		}
	}
	else
	{
		if (po->translucency >= NUMTRANSMAPS)

			po->translucency = NUMTRANSMAPS-1;

		po->flags |= (po->spawnflags & POF_RENDERALL);


		if (th->docollision)
		{
			if (th->doghostfade)
			{
				po->flags &= ~POF_SOLID;
				po->flags |= POF_NOSPECIALS;
			}
			else
			{
				po->flags |= (po->spawnflags & POF_SOLID);
				if (!(po->spawnflags & POF_NOSPECIALS))
					po->flags &= ~POF_NOSPECIALS;
			}
		}
	}
}

boolean EV_DoPolyObjFade(polyfadedata_t *pfdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polyfade_t *th;
	INT32 start;

	if (!(po = Polyobj_GetForNum(pfdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjFade: bad polyobj %d\n", pfdata->polyObjNum);
		return false;
	}


	if (po->isBad)
		return false;


	if (po->translucency == pfdata->destvalue)
		return true;

	if (po->thinker && po->thinker->function.acp1 == (actionf_p1)T_PolyObjFade)
		P_RemoveThinker(po->thinker);


	th = Z_Malloc(sizeof(polyfade_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjFade;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;


	th->polyObjNum = pfdata->polyObjNum;
	th->sourcevalue = po->translucency;
	th->destvalue = pfdata->destvalue;
	th->docollision = pfdata->docollision;
	th->doghostfade = pfdata->doghostfade;

	if (pfdata->ticbased)
	{
		th->ticbased = true;
		th->timer = th->duration = abs(pfdata->speed);  
	}
	else
	{
		th->ticbased = false;
		th->timer = abs(FixedMul(FixedDiv(256, NUMTRANSMAPS), NUMTRANSMAPS - th->destvalue)
			- FixedMul(FixedDiv(256, NUMTRANSMAPS), NUMTRANSMAPS - th->sourcevalue));  
		th->duration = abs(pfdata->speed);  
	}

	oldpo = po;


	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		pfdata->polyObjNum = po->id;
		EV_DoPolyObjFade(pfdata);
	}


	return true;
}


