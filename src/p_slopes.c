// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2009      by Stephen McGranahan.
// Copyright (C) 2015-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "doomdef.h"
#include "r_defs.h"
#include "r_state.h"
#include "m_bbox.h"
#include "m_vector.h"
#include "z_zone.h"
#include "p_local.h"
#include "p_spec.h"
#include "p_slopes.h"
#include "p_setup.h"
#include "r_main.h"
#include "p_maputl.h"
#include "w_wad.h"
#include "r_fps.h"

pslope_t *slopelist = NULL;
UINT16 slopecount = 0;

static void P_UpdateMidtextureSlopesForSector(sector_t *sector);


void P_CalculateSlopeNormal(pslope_t *slope)
{
	slope->normal.z = FINECOSINE(slope->zangle>>ANGLETOFINESHIFT);
	slope->normal.x = FixedMul(FINESINE(slope->zangle>>ANGLETOFINESHIFT), slope->d.x);
	slope->normal.y = FixedMul(FINESINE(slope->zangle>>ANGLETOFINESHIFT), slope->d.y);
}

static void CalculateNormalDir(pslope_t *slope, dvector3_t *dnormal)
{
	double hyp = hypot(dnormal->x, dnormal->y);

	if (fpclassify(hyp) == FP_NORMAL)
	{
		slope->dnormdir.x = -dnormal->x / hyp;
		slope->dnormdir.y = -dnormal->y / hyp;
		slope->dzdelta = hyp / dnormal->z;
	}
	else
	{
		slope->dnormdir.x = slope->dnormdir.y = 0.0;
		slope->dzdelta = 0.0;
	}
}

void P_CalculateSlopeVectors(pslope_t *slope)
{
	dvector3_t dnormal;

	DVector3_Load(&dnormal, FixedToDouble(slope->normal.x), FixedToDouble(slope->normal.y), FixedToDouble(slope->normal.z));
	DVector3_Load(&slope->dorigin, FixedToDouble(slope->o.x), FixedToDouble(slope->o.y), FixedToDouble(slope->o.z));

	CalculateNormalDir(slope, &dnormal);
}


static void ReconfigureViaVertexes (pslope_t *slope, const vector3_t v1, const vector3_t v2, const vector3_t v3)
{
	vector3_t vec1, vec2;


	FV3_Copy(&slope->o, &v1);


	FV3_SubEx(&v2, &v1, &vec1);
	FV3_SubEx(&v3, &v1, &vec2);


	if (vec1.z == 0 && vec2.z == 0)
	{
		slope->zangle = slope->xydirection = 0;
		slope->zdelta = slope->d.x = slope->d.y = 0;
		slope->normal.x = slope->normal.y = 0;
		slope->normal.z = FRACUNIT;
	}
	else
	{

		fixed_t m = max(
			max(max(abs(vec1.x), abs(vec1.y)), abs(vec1.z)),
			max(max(abs(vec2.x), abs(vec2.y)), abs(vec2.z))
		) >> 5;  

		FV3_Cross(
				FV3_Divide(&vec1, m),
				FV3_Divide(&vec2, m),
				&slope->normal
				);


		m = R_PointToDist2(0, 0, R_PointToDist2(0, 0, slope->normal.x, slope->normal.y), slope->normal.z);


		if (slope->normal.z < 0)
			m = -m;

		FV3_Divide(&slope->normal, m);


		m = FixedHypot(slope->normal.x, slope->normal.y);
		slope->d.x = -FixedDiv(slope->normal.x, m);
		slope->d.y = -FixedDiv(slope->normal.y, m);


		slope->zdelta = FixedDiv(m, slope->normal.z);


		slope->xydirection = R_PointToAngle2(0, 0, slope->d.x, slope->d.y)+ANGLE_180;
		slope->zangle = InvAngle(R_PointToAngle2(0, 0, FRACUNIT, slope->zdelta));
	}

	P_CalculateSlopeVectors(slope);
}


static void ReconfigureViaConstants (pslope_t *slope, const double pa, const double pb, const double pc, const double pd)
{
	fixed_t m;
	fixed_t o = 0;
	double d_o = 0.0;

	fixed_t a = DoubleToFixed(pa), b = DoubleToFixed(pb), c = DoubleToFixed(pc), d = DoubleToFixed(pd);

	if (c)
	{
		d_o = abs(c) <= FRACUNIT ? -(pd * (1.0 / pc)) : -(pd / pc);
		o = abs(c) <= FRACUNIT ? -FixedMul(d, FixedDiv(FRACUNIT, c)) : -FixedDiv(d, c);
	}


	FV3_Load(&slope->o, 0, 0, o);


	vector3_t *normal = &slope->normal;

	FV3_Load(normal, a, b, c);
	FV3_Normalize(normal);


	if (normal->z < 0)
		FV3_Negate(normal);


	m = FixedHypot(normal->x, normal->y);
	slope->d.x = -FixedDiv(normal->x, m);
	slope->d.y = -FixedDiv(normal->y, m);


	slope->zdelta = FixedDiv(m, normal->z);


	slope->xydirection = R_PointToAngle2(0, 0, slope->d.x, slope->d.y)+ANGLE_180;
	slope->zangle = InvAngle(R_PointToAngle2(0, 0, FRACUNIT, slope->zdelta));

	dvector3_t dnormal;

	DVector3_Load(&dnormal, pa, pb, pc);
	DVector3_Normalize(&dnormal);
	if (dnormal.z < 0)
		DVector3_Negate(&dnormal);

	DVector3_Load(&slope->dorigin, 0, 0, d_o);

	CalculateNormalDir(slope, &dnormal);
}


void T_DynamicSlopeLine (dynlineplanethink_t* th)
{
	pslope_t* slope = th->slope;
	line_t* srcline = th->sourceline;

	fixed_t zdelta, oldoz = slope->o.z;

	switch(th->type) {
	case DP_FRONTFLOOR:
		zdelta = srcline->backsector->floorheight - srcline->frontsector->floorheight;
		slope->o.z = srcline->frontsector->floorheight;
		break;

	case DP_FRONTCEIL:
		zdelta = srcline->backsector->ceilingheight - srcline->frontsector->ceilingheight;
		slope->o.z = srcline->frontsector->ceilingheight;
		break;

	case DP_BACKFLOOR:
		zdelta = srcline->frontsector->floorheight - srcline->backsector->floorheight;
		slope->o.z = srcline->backsector->floorheight;
		break;

	case DP_BACKCEIL:
		zdelta = srcline->frontsector->ceilingheight - srcline->backsector->ceilingheight;
		slope->o.z = srcline->backsector->ceilingheight;
		break;

	default:
		return;
	}

	if (slope->zdelta != FixedDiv(zdelta, th->extent) || oldoz != slope->o.z) {
		slope->zdelta = FixedDiv(zdelta, th->extent);
		slope->zangle = R_PointToAngle2(0, 0, th->extent, -zdelta);
		slope->moved = true;
		P_CalculateSlopeNormal(slope);
		P_UpdateMidtextureSlopesForSector(srcline->frontsector);
		if (srcline->backsector)
			P_UpdateMidtextureSlopesForSector(srcline->backsector);
	}
}


void T_DynamicSlopeVert (dynvertexplanethink_t* th)
{
	size_t i;

	for (i = 0; i < 3; i++)
	{
		if (!th->secs[i])
			continue;

		if (th->relative & (1 << i))
			th->vex[i].z = th->origvecheights[i] + (th->secs[i]->floorheight - th->origsecheights[i]);
		else
			th->vex[i].z = th->secs[i]->floorheight;
	}

	ReconfigureViaVertexes(th->slope, th->vex[0], th->vex[1], th->vex[2]);

	for (i = 0; i < 3; i++)
	{
		if (th->secs[i])
			P_UpdateMidtextureSlopesForSector(th->secs[i]);
	}
}

static inline void P_AddDynLineSlopeThinker (pslope_t* slope, dynplanetype_t type, line_t* sourceline, fixed_t extent)
{
	dynlineplanethink_t* th = Z_Calloc(sizeof (*th), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_DynamicSlopeLine;
	th->slope = slope;
	th->type = type;
	th->sourceline = sourceline;
	th->extent = extent;
	P_AddThinker(THINK_DYNSLOPE, &th->thinker);


	R_CreateInterpolator_DynSlope(&th->thinker, slope);
}

static inline void P_AddDynVertexSlopeThinker (pslope_t* slope, const INT16 tags[3], const vector3_t vx[3])
{
	dynvertexplanethink_t* th = Z_Calloc(sizeof (*th), PU_LEVSPEC, NULL);
	size_t i;
	INT32 l;
	th->thinker.function.acp1 = (actionf_p1)T_DynamicSlopeVert;
	th->slope = slope;

	for (i = 0; i < 3; i++) {
		l = Tag_FindLineSpecial(799, tags[i]);
		th->secs[i] = (l == -1) ? NULL : lines[l].frontsector;
		th->vex[i] = vx[i];
		th->origsecheights[i] = (l == -1) ? 0 : lines[l].frontsector->floorheight;
		th->origvecheights[i] = vx[i].z;
		if (l != -1 && lines[l].args[0])
			th->relative |= 1<<i;
	}
	P_AddThinker(THINK_DYNSLOPE, &th->thinker);
}


static inline pslope_t* Slope_Add (const UINT8 flags)
{
	pslope_t *ret = Z_Calloc(sizeof(pslope_t), PU_LEVEL, NULL);
	ret->flags = flags;

	ret->next = slopelist;
	slopelist = ret;

	slopecount++;
	ret->id = slopecount;

	return ret;
}


static pslope_t *MakeViaVectors(const vector3_t *o, const vector2_t *d,
                             const fixed_t zdelta, UINT8 flags)
{
	pslope_t *ret = Slope_Add(flags);

	FV3_Copy(&ret->o, o);
	FV2_Copy(&ret->d, d);

	ret->zdelta = zdelta;

	ret->flags = flags;

	return ret;
}


static fixed_t GetExtent(sector_t *sector, line_t *line)
{

	fixed_t fardist = -FRACUNIT;
	size_t i;



	for(i = 0; i < sector->linecount; i++)
	{
		line_t *li = sector->lines[i];
		vertex_t tempv;
		fixed_t dist;


		if(li == line)
			continue;

		P_ClosestPointOnLine(li->v1->x, li->v1->y, line, &tempv);
		dist = R_PointToDist2(tempv.x, tempv.y, li->v1->x, li->v1->y);
		if(dist > fardist)
			fardist = dist;


		P_ClosestPointOnLine(li->v2->x, li->v2->y, line, &tempv);
		dist = R_PointToDist2(tempv.x, tempv.y, li->v2->x, li->v2->y);
		if(dist > fardist)
			fardist = dist;
	}

	return fardist;
}

static boolean P_CopySlope(pslope_t** toslope, pslope_t* fromslope)
{
	if (*toslope || !fromslope)
		return true;

	*toslope = fromslope;
	return true;
}

static void P_UpdateHasSlope(sector_t *sec)
{
	size_t i;

	sec->hasslope = true;


	if (sec->numattached)
		for (i = 0; i < sec->numattached; i++)
			sectors[sec->attached[i]].hasslope = true;
}


static void line_SpawnViaLine(const int linenum, const boolean spawnthinker)
{



	line_t *line = lines + linenum;
	pslope_t *fslope = NULL, *cslope = NULL;
	vector3_t origin, point;
	vector2_t direction;
	fixed_t nx, ny, dz, extent;

	boolean frontfloor = line->args[0] == TMS_FRONT;
	boolean backfloor = line->args[0] == TMS_BACK;
	boolean frontceil = line->args[1] == TMS_FRONT;
	boolean backceil = line->args[1] == TMS_BACK;
	UINT8 flags = 0;  
	if (line->args[2] & TMSL_NOPHYSICS)
		flags |= SL_NOPHYSICS;
	if (line->args[2] & TMSL_DYNAMIC)
		flags |= SL_DYNAMIC;

	if(!frontfloor && !backfloor && !frontceil && !backceil)
	{
		CONS_Printf("line_SpawnViaLine: Unused slope special with nothing to do on line number %i\n", linenum);
		return;
	}

	if(!line->frontsector || !line->backsector)
	{
		CONS_Debug(DBG_SETUP, "line_SpawnViaLine: Slope special used on a line without two sides. (line number %i)\n", linenum);
		return;
	}

	{
		fixed_t len = R_PointToDist2(0, 0, line->dx, line->dy);
		nx = FixedDiv(line->dy, len);
		ny = -FixedDiv(line->dx, len);
	}


	origin.x = line->v1->x + (line->v2->x - line->v1->x)/2;
	origin.y = line->v1->y + (line->v2->y - line->v1->y)/2;


	if(frontfloor || frontceil)
	{
		line->frontsector->hasslope = true;  

		origin.z = line->backsector->floorheight;
		direction.x = nx;
		direction.y = ny;

		extent = GetExtent(line->frontsector, line);

		if(extent < 0)
		{
			CONS_Printf("line_SpawnViaLine failed to get frontsector extent on line number %i\n", linenum);
			return;
		}


		point.x = origin.x + FixedMul(direction.x, extent);
		point.y = origin.y + FixedMul(direction.y, extent);
		direction.x = -direction.x;
		direction.y = -direction.y;



		if(frontfloor)
		{
			point.z = line->frontsector->floorheight;  
			dz = FixedDiv(origin.z - point.z, extent);  



			fslope = line->frontsector->f_slope =
            MakeViaVectors(&point, &direction, dz, flags);






			fslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			fslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(fslope);
			P_CalculateSlopeVectors(fslope);

			if (spawnthinker && (flags & SL_DYNAMIC))
				P_AddDynLineSlopeThinker(fslope, DP_FRONTFLOOR, line, extent);
		}
		if(frontceil)
		{
			origin.z = line->backsector->ceilingheight;
			point.z = line->frontsector->ceilingheight;
			dz = FixedDiv(origin.z - point.z, extent);

			cslope = line->frontsector->c_slope =
            MakeViaVectors(&point, &direction, dz, flags);

			cslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			cslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(cslope);
			P_CalculateSlopeVectors(cslope);

			if (spawnthinker && (flags & SL_DYNAMIC))
				P_AddDynLineSlopeThinker(cslope, DP_FRONTCEIL, line, extent);
		}
	}
	if(backfloor || backceil)
	{
		line->backsector->hasslope = true;  

		origin.z = line->frontsector->floorheight;

		direction.x = -nx;
		direction.y = -ny;

		extent = GetExtent(line->backsector, line);

		if(extent < 0)
		{
			CONS_Printf("line_SpawnViaLine failed to get backsector extent on line number %i\n", linenum);
			return;
		}


		point.x = origin.x + FixedMul(direction.x, extent);
		point.y = origin.y + FixedMul(direction.y, extent);
		direction.x = -direction.x;
		direction.y = -direction.y;

		if(backfloor)
		{
			point.z = line->backsector->floorheight;
			dz = FixedDiv(origin.z - point.z, extent);

			fslope = line->backsector->f_slope =
            MakeViaVectors(&point, &direction, dz, flags);

			fslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			fslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(fslope);
			P_CalculateSlopeVectors(fslope);

			if (spawnthinker && (flags & SL_DYNAMIC))
				P_AddDynLineSlopeThinker(fslope, DP_BACKFLOOR, line, extent);
		}
		if(backceil)
		{
			origin.z = line->frontsector->ceilingheight;
			point.z = line->backsector->ceilingheight;
			dz = FixedDiv(origin.z - point.z, extent);

			cslope = line->backsector->c_slope =
            MakeViaVectors(&point, &direction, dz, flags);

			cslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			cslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(cslope);
			P_CalculateSlopeVectors(cslope);

			if (spawnthinker && (flags & SL_DYNAMIC))
				P_AddDynLineSlopeThinker(cslope, DP_BACKCEIL, line, extent);
		}
	}

	if (line->args[2] & TMSL_COPY)
	{
		if (frontfloor)
			P_CopySlope(&line->backsector->f_slope, line->frontsector->f_slope);
		if (backfloor)
			P_CopySlope(&line->frontsector->f_slope, line->backsector->f_slope);
		if (frontceil)
			P_CopySlope(&line->backsector->c_slope, line->frontsector->c_slope);
		if (backceil)
			P_CopySlope(&line->frontsector->c_slope, line->backsector->c_slope);

		if (backfloor || backceil)
			P_UpdateHasSlope(line->frontsector);
		if (frontfloor || frontceil)
			P_UpdateHasSlope(line->backsector);
	}
}


static pslope_t *MakeViaMapthings(INT16 tag1, INT16 tag2, INT16 tag3, UINT8 flags, const boolean spawnthinker)
{
	size_t i;
	mapthing_t* mt = mapthings;
	mapthing_t* vertices[3] = {0};
	INT16 tags[3] = {tag1, tag2, tag3};

	vector3_t vx[3];
	pslope_t* ret = Slope_Add(flags);


	for (i = 0; i < nummapthings; i++, mt++) {
		if (mt->type != 750)  
			continue;

		if (!vertices[0] && Tag_Find(&mt->tags, tag1))
			vertices[0] = mt;
		else if (!vertices[1] && Tag_Find(&mt->tags, tag2))
			vertices[1] = mt;
		else if (!vertices[2] && Tag_Find(&mt->tags, tag3))
			vertices[2] = mt;
	}


	for (i = 0; i < 3; i++) {
		mt = vertices[i];
		if (!mt)  
			I_Error("MakeViaMapthings: Slope vertex %s (for linedef tag %d) not found!", sizeu1(i), tag1);
		vx[i].x = mt->x << FRACBITS;
		vx[i].y = mt->y << FRACBITS;
		vx[i].z = mt->z << FRACBITS;
		if (!mt->args[0])
			vx[i].z += R_PointInSubsector(vx[i].x, vx[i].y)->sector->floorheight;
	}

	ReconfigureViaVertexes(ret, vx[0], vx[1], vx[2]);

	if (spawnthinker && (flags & SL_DYNAMIC))
		P_AddDynVertexSlopeThinker(ret, tags, vx);

	return ret;
}


static void line_SpawnViaMapthingVertexes(const int linenum, const boolean spawnthinker)
{
	line_t *line = lines + linenum;
	side_t *side;
	pslope_t **slopetoset;
	UINT16 tag1 = line->args[1];
	UINT16 tag2 = line->args[2];
	UINT16 tag3 = line->args[3];
	UINT8 flags = 0;  
	if (line->args[4] & TMSL_NOPHYSICS)
		flags |= SL_NOPHYSICS;
	if (line->args[4] & TMSL_DYNAMIC)
		flags |= SL_DYNAMIC;

	switch(line->args[0])
	{
	case TMSP_FRONTFLOOR:
		slopetoset = &line->frontsector->f_slope;
		side = &sides[line->sidenum[0]];
		break;
	case TMSP_FRONTCEILING:
		slopetoset = &line->frontsector->c_slope;
		side = &sides[line->sidenum[0]];
		break;
	case TMSP_BACKFLOOR:
		slopetoset = &line->backsector->f_slope;
		side = &sides[line->sidenum[1]];
		break;
	case TMSP_BACKCEILING:
		slopetoset = &line->backsector->c_slope;
		side = &sides[line->sidenum[1]];
		break;
	default:
		return;
	}

	*slopetoset = MakeViaMapthings(tag1, tag2, tag3, flags, spawnthinker);

	side->sector->hasslope = true;
}


static void SpawnVertexSlopes(void)
{
	line_t *l1, *l2;
	sector_t* sc;
	vertex_t *v1, *v2, *v3;
	size_t i;
	for (i = 0, sc = sectors; i < numsectors; i++, sc++)
	{

		if (sc->linecount != 3)
			continue;

		l1 = sc->lines[0];
		l2 = sc->lines[1];


		v1 = l1->v1;
		v2 = l1->v2;
		if ((l2->v1 != v1) && (l2->v1 != v2))
			v3 = l2->v1;
		else
			v3 = l2->v2;

		if (v1->floorzset || v2->floorzset || v3->floorzset)
		{
			vector3_t vtx[3] = {
				{v1->x, v1->y, v1->floorzset ? v1->floorz : sc->floorheight},
				{v2->x, v2->y, v2->floorzset ? v2->floorz : sc->floorheight},
				{v3->x, v3->y, v3->floorzset ? v3->floorz : sc->floorheight}};
			pslope_t *slop = Slope_Add(0);
			sc->f_slope = slop;
			sc->hasslope = true;
			ReconfigureViaVertexes(slop, vtx[0], vtx[1], vtx[2]);
		}

		if (v1->ceilingzset || v2->ceilingzset || v3->ceilingzset)
		{
			vector3_t vtx[3] = {
				{v1->x, v1->y, v1->ceilingzset ? v1->ceilingz : sc->ceilingheight},
				{v2->x, v2->y, v2->ceilingzset ? v2->ceilingz : sc->ceilingheight},
				{v3->x, v3->y, v3->ceilingzset ? v3->ceilingz : sc->ceilingheight}};
			pslope_t *slop = Slope_Add(0);
			sc->c_slope = slop;
			sc->hasslope = true;
			ReconfigureViaVertexes(slop, vtx[0], vtx[1], vtx[2]);
		}
	}
}

static boolean P_SetSlopeFromTag(sector_t *sec, INT32 tag, boolean ceiling)
{
	INT32 i;
	pslope_t **secslope = ceiling ? &sec->c_slope : &sec->f_slope;

	if (!tag || *secslope)
		return false;
	TAG_ITER_SECTORS(tag, i)
	{
		pslope_t *srcslope = ceiling ? sectors[i].c_slope : sectors[i].f_slope;
		if (srcslope)
		{
			*secslope = srcslope;
			return true;
		}
	}
	return false;
}






void P_CopySectorSlope(line_t *line)
{
	sector_t *fsec = line->frontsector;
	sector_t *bsec = line->backsector;
	boolean setfront = false;
	boolean setback = false;

	setfront |= P_SetSlopeFromTag(fsec, line->args[0], false);
	setfront |= P_SetSlopeFromTag(fsec, line->args[1], true);
	if (bsec)
	{
		setback |= P_SetSlopeFromTag(bsec, line->args[2], false);
		setback |= P_SetSlopeFromTag(bsec, line->args[3], true);

		if (line->args[4] & TMSC_FRONTTOBACKFLOOR)
			setback |= P_CopySlope(&bsec->f_slope, fsec->f_slope);
		if (line->args[4] & TMSC_BACKTOFRONTFLOOR)
			setfront |= P_CopySlope(&fsec->f_slope, bsec->f_slope);
		if (line->args[4] & TMSC_FRONTTOBACKCEILING)
			setback |= P_CopySlope(&bsec->c_slope, fsec->c_slope);
		if (line->args[4] & TMSC_BACKTOFRONTCEILING)
			setfront |= P_CopySlope(&fsec->c_slope, bsec->c_slope);
	}

	if (setfront)
		P_UpdateHasSlope(fsec);
	if (setback)
		P_UpdateHasSlope(bsec);

	line->special = 0;  
}






pslope_t *P_SlopeById(UINT16 id)
{
	pslope_t *ret;
	for (ret = slopelist; ret && ret->id != id; ret = ret->next);
	return ret;
}


pslope_t *P_MakeSlopeViaEquationConstants(const double a, const double b, const double c, const double d)
{
	pslope_t* ret = Slope_Add(0);

	ReconfigureViaConstants(ret, a, b, c, d);

	return ret;
}

static pslope_t *P_GetReferenceSlopeForMidtexture(line_t *line)
{
	if (line->flags & ML_MIDPEG)
	{

		fixed_t frontheight = P_GetSectorFloorZAt(line->frontsector, line->v1->x, line->v1->y);
		fixed_t backheight = P_GetSectorFloorZAt(line->backsector, line->v1->x, line->v1->y);

		if (frontheight > backheight)
		{
			return line->frontsector->f_slope;
		}
		else
		{
			return line->backsector->f_slope;
		}
	}
	else
	{

		fixed_t frontheight = P_GetSectorCeilingZAt(line->frontsector, line->v1->x, line->v1->y);
		fixed_t backheight = P_GetSectorCeilingZAt(line->backsector, line->v1->x, line->v1->y);

		if (frontheight < backheight)
		{
			return line->frontsector->c_slope;
		}
		else
		{
			return line->backsector->c_slope;
		}
	}

	return NULL;
}


static void P_UpdateSolidMidtextureSlope(line_t *line, pslope_t *ref)
{
	pslope_t *slope = line->midtexslope;

	if (ref == NULL)
		return;


	vector3_t origin;
	origin.x = line->v1->x;
	origin.y = line->v1->y;
	origin.z = P_GetSlopeZAt(ref, origin.x, origin.y);
	FV3_Copy(&slope->o, &origin);


	vector3_t point;
	point.x = line->v2->x;
	point.y = line->v2->y;
	point.z = P_GetSlopeZAt(ref, point.x, point.y);


	fixed_t extent = R_PointToDist2(0, 0, line->dx, line->dy);


	slope->zdelta = FixedDiv(origin.z - point.z, extent);
	slope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
	slope->xydirection = line->angle;


	vector2_t dir;
	dir.x = FixedMul(FINECOSINE(slope->zangle >> ANGLETOFINESHIFT), FINECOSINE((slope->xydirection+ANGLE_180) >> ANGLETOFINESHIFT));
	dir.y = FixedMul(FINECOSINE(slope->zangle >> ANGLETOFINESHIFT), FINESINE((slope->xydirection+ANGLE_180) >> ANGLETOFINESHIFT));
	FV2_Copy(&slope->d, &dir);

	P_CalculateSlopeNormal(slope);


	slope->moved = true;
}


static void P_UpdateMidtextureSlopesForSector(sector_t *sector)
{
	for (size_t i = 0; i < sector->linecount; i++)
	{
		if (sector->lines[i]->midtexslope != NULL)
			P_UpdateSolidMidtextureSlope(sector->lines[i], P_GetReferenceSlopeForMidtexture(sector->lines[i]));
	}
}


static void P_CreateSolidMidtextureSlope(line_t *line)
{
	if (line->backsector == NULL)  
		return;

	if ((line->flags & ML_MIDSOLID) == 0)  
		return;

	pslope_t *ref = P_GetReferenceSlopeForMidtexture(line);
	if (ref)
	{
		line->midtexslope = Slope_Add(ref->flags & SL_NOPHYSICS);

		P_UpdateSolidMidtextureSlope(line, ref);
	}
}


void P_SpawnSlopes(const boolean fromsave) {
	size_t i;


	SpawnVertexSlopes();


	for (i = 0; i < numlines; i++)
	{
		switch (lines[i].special)
		{
			case 700:
				line_SpawnViaLine(i, !fromsave);
				break;

			case 704:
				line_SpawnViaMapthingVertexes(i, !fromsave);
				break;

			default:
				break;
		}
	}




	for (i = 0; i < numlines; i++)
	{
		line_t *line = &lines[i];

		switch (line->special)
		{
			case 720:
				P_CopySectorSlope(line);
			default:
				break;
		}

		P_CreateSolidMidtextureSlope(line);
	}
}


void P_InitSlopes(void)
{
	slopelist = NULL;
	slopecount = 0;
}







fixed_t P_GetSlopeZAt(const pslope_t *slope, fixed_t x, fixed_t y)
{
	fixed_t dist = FixedMul(x - slope->o.x, slope->d.x) / 2 +
	               FixedMul(y - slope->o.y, slope->d.y) / 2;

	return slope->o.z + FixedMul(dist, slope->zdelta) * 2;
}


fixed_t P_GetZAt(const pslope_t *slope, fixed_t x, fixed_t y, fixed_t z)
{
	return slope ? P_GetSlopeZAt(slope, x, y) : z;
}


fixed_t P_GetSectorFloorZAt(const sector_t *sector, fixed_t x, fixed_t y)
{
	return sector->f_slope ? P_GetSlopeZAt(sector->f_slope, x, y) : sector->floorheight;
}


fixed_t P_GetSectorCeilingZAt(const sector_t *sector, fixed_t x, fixed_t y)
{
	return sector->c_slope ? P_GetSlopeZAt(sector->c_slope, x, y) : sector->ceilingheight;
}


fixed_t P_GetFFloorTopZAt(const ffloor_t *ffloor, fixed_t x, fixed_t y)
{
	return *ffloor->t_slope ? P_GetSlopeZAt(*ffloor->t_slope, x, y) : *ffloor->topheight;
}


fixed_t P_GetFFloorBottomZAt(const ffloor_t *ffloor, fixed_t x, fixed_t y)
{
	return *ffloor->b_slope ? P_GetSlopeZAt(*ffloor->b_slope, x, y) : *ffloor->bottomheight;
}


fixed_t P_GetLightZAt(const lightlist_t *light, fixed_t x, fixed_t y)
{
	return light->slope ? P_GetSlopeZAt(light->slope, x, y) : light->height;
}






void P_QuantizeMomentumToSlope(vector3_t *momentum, pslope_t *slope)
{
	vector3_t axis;  

	if (slope->flags & SL_NOPHYSICS)
		return;  

	axis.x = -slope->d.y;
	axis.y = slope->d.x;
	axis.z = 0;

	FV3_Rotate(momentum, &axis, slope->zangle >> ANGLETOFINESHIFT);
}





void P_ReverseQuantizeMomentumToSlope(vector3_t *momentum, pslope_t *slope)
{
	slope->zangle = InvAngle(slope->zangle);
	P_QuantizeMomentumToSlope(momentum, slope);
	slope->zangle = InvAngle(slope->zangle);
}





void P_SlopeLaunch(mobj_t *mo)
{
	if (!(mo->standingslope->flags & SL_NOPHYSICS)  
		&& (mo->standingslope->normal.x != 0
		||  mo->standingslope->normal.y != 0))
	{



		vector3_t slopemom;
		slopemom.x = mo->momx;
		slopemom.y = mo->momy;
		slopemom.z = mo->momz*2;
		P_QuantizeMomentumToSlope(&slopemom, mo->standingslope);

		mo->momx = slopemom.x;
		mo->momy = slopemom.y;
		mo->momz = slopemom.z/2;

	    if (mo->player)
		    mo->player->powers[pw_justlaunched] = 1;
	}


	mo->standingslope = NULL;
}







fixed_t P_GetWallTransferMomZ(mobj_t *mo, pslope_t *slope)
{
	vector3_t slopemom, axis;
	angle_t ang;

	if (slope->flags & SL_NOPHYSICS)
		return 0;



	ang = slope->zangle + ANG15*((slope->zangle > 0) ? 1 : -1);
	if (ang > ANGLE_90 && ang < ANGLE_180)
		ang = ((slope->zangle > 0) ? ANGLE_90 : InvAngle(ANGLE_90));  

	slopemom.x = mo->momx;
	slopemom.y = mo->momy;
	slopemom.z = 3*(mo->momz/2);

	axis.x = -slope->d.y;
	axis.y = slope->d.x;
	axis.z = 0;

	FV3_Rotate(&slopemom, &axis, ang >> ANGLETOFINESHIFT);

	return 2*(slopemom.z/3);
}


void P_HandleSlopeLanding(mobj_t *thing, pslope_t *slope)
{
	vector3_t mom;  
	if (slope->flags & SL_NOPHYSICS || (slope->normal.x == 0 && slope->normal.y == 0)) {  
		if (P_MobjFlip(thing)*(thing->momz) < 0)  
		{
			thing->standingslope = slope;
			P_SetPitchRollFromSlope(thing, slope);

			if (!thing->player || !(thing->player->pflags & PF_BOUNCING))
				thing->momz = -P_MobjFlip(thing);
		}
		return;
	}

	mom.x = thing->momx;
	mom.y = thing->momy;
	mom.z = thing->momz*2;

	P_ReverseQuantizeMomentumToSlope(&mom, slope);

	if (P_MobjFlip(thing)*mom.z < 0) {  
		thing->momx = mom.x;
		thing->momy = mom.y;
		thing->standingslope = slope;
		P_SetPitchRollFromSlope(thing, slope);
		if (!thing->player || !(thing->player->pflags & PF_BOUNCING))
			thing->momz = -P_MobjFlip(thing);
	}
}



void P_ButteredSlope(mobj_t *mo)
{
	fixed_t thrust;

	if (!mo->standingslope)
		return;

	if (mo->standingslope->flags & SL_NOPHYSICS)
		return;  

	if (mo->flags & (MF_NOCLIPHEIGHT|MF_NOGRAVITY))
		return;  

	if (mo->player) {
		if (abs(mo->standingslope->zdelta) < FRACUNIT/4 && !(mo->player->pflags & PF_SPINNING))
			return;  

		if (abs(mo->standingslope->zdelta) < FRACUNIT/2 && !(mo->player->rmomx || mo->player->rmomy))
			return;  
	}

	thrust = FINESINE(mo->standingslope->zangle>>ANGLETOFINESHIFT) * 3 / 2 * (mo->eflags & MFE_VERTICALFLIP ? 1 : -1);

	if (mo->player && (mo->player->pflags & PF_SPINNING)) {
		fixed_t mult = 0;
		if (mo->momx || mo->momy) {
			angle_t angle = R_PointToAngle2(0, 0, mo->momx, mo->momy) - mo->standingslope->xydirection;

			if (P_MobjFlip(mo) * mo->standingslope->zdelta < 0)
				angle ^= ANGLE_180;

			mult = FINECOSINE(angle >> ANGLETOFINESHIFT);
		}

		thrust = FixedMul(thrust, FRACUNIT*2/3 + mult/8);
	}

	if (mo->momx || mo->momy)  
		thrust = FixedMul(thrust, FRACUNIT+P_AproxDistance(mo->momx, mo->momy)/16);



	thrust = FixedMul(thrust, abs(P_GetMobjGravity(mo)));


	thrust = FixedMul(thrust, FixedDiv(mo->friction, ORIG_FRICTION));

	P_Thrust(mo, mo->standingslope->xydirection, thrust);
}
