// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_bsp.c
/// \brief convert SRB2 map

#include "../doomdef.h"
#include "../doomstat.h"
#ifdef HWRENDER
#include "hw_glob.h"
#include "../r_local.h"
#include "../z_zone.h"
#include "../console.h"
#include "../v_video.h"
#include "../m_menu.h"
#include "../i_system.h"
#include "../m_argv.h"
#include "../i_video.h"
#include "../w_wad.h"
#include "../p_setup.h" // levelfadecol
#include <float.h>

// --------------------------------------------------------------------------
// This is global data for planes rendering
// --------------------------------------------------------------------------

extrasubsector_t *extrasubsectors = NULL;

// newsubsectors are subsectors without segs, added for the plane polygons
#define NEWSUBSECTORS 50
static size_t totsubsectors;
size_t addsubsector;

typedef struct
{
	float x, y;
	float dx, dy;
} fdivline_t;

// ==========================================================================
//                                    FLOOR & CEILING CONVEX POLYS GENERATION
// ==========================================================================

//debug counters
static INT32 nobackpoly = 0;
static INT32 skipcut = 0;
static INT32 totalsubsecpolys = 0;
static boolean hwr_closed_node_boundaries;
#define HWR_POLYGON_EPSILON 0.005f

// --------------------------------------------------------------------------
// Polygon fast alloc / free
// --------------------------------------------------------------------------
//hurdler: quick fix for those who wants to play with larger wad

#define ZPLANALLOC
#ifndef ZPLANALLOC
//#define POLYPOOLSIZE 1024000 // may be much over what is needed
/// \todo check out how much is used
static size_t POLYPOOLSIZE = 1024000;

static UINT8 *gl_polypool = NULL;
static UINT8 *gl_ppcurrent;
static size_t gl_ppfree;
#endif

// only between levels, clear poly pool
static void HWR_ClearPolys(void)
{
#ifndef ZPLANALLOC
	gl_ppcurrent = gl_polypool;
	gl_ppfree = POLYPOOLSIZE;
#endif
}

// allocate  pool for fast alloc of polys
void HWR_InitPolyPool(void)
{
#ifndef ZPLANALLOC
	INT32 pnum;

	//hurdler: quick fix for those who wants to play with larger wad
	if ((pnum = M_CheckParm("-polypoolsize")))
		POLYPOOLSIZE = atoi(myargv[pnum+1])*1024; // (in kb)

	CONS_Debug(DBG_RENDER, "HWR_InitPolyPool(): allocating %d bytes\n", POLYPOOLSIZE);
	gl_polypool = malloc(POLYPOOLSIZE);
	if (!gl_polypool)
		I_Error("HWR_InitPolyPool(): couldn't malloc polypool\n");
	HWR_ClearPolys();
#endif
}

void HWR_FreePolyPool(void)
{
#ifndef ZPLANALLOC
	if (gl_polypool)
		free(gl_polypool);
	gl_polypool = NULL;
#endif
}

static poly_t *HWR_AllocPoly(INT32 numpts)
{
	poly_t *p;
	size_t size = sizeof (poly_t) + sizeof (polyvertex_t) * numpts;
#ifdef ZPLANALLOC
	p = Z_Malloc(size, PU_HWRPLANE, NULL);
#else
#ifdef PARANOIA
	if (!gl_polypool)
		I_Error("Used gl_polypool without init!\n");
	if (!gl_ppcurrent)
		I_Error("gl_ppcurrent == NULL!\n");
#endif

	if (gl_ppfree < size)
		I_Error("HWR_AllocPoly(): no more memory %u bytes left, %u bytes needed\n\n%s\n",
		        gl_ppfree, size, "You can try the param -polypoolsize 2048 (or higher if needed)");

	p = (poly_t *)gl_ppcurrent;
	gl_ppcurrent += size;
	gl_ppfree -= size;
#endif
	p->numpts = numpts;
	return p;
}

/// \todo polygons should be freed in reverse order for efficiency,
/// for now don't free because it doesn't free in reverse order
static void HWR_FreePoly(poly_t *poly)
{
#ifdef ZPLANALLOC
	Z_Free(poly);
#else
	const size_t size = sizeof (poly_t) + sizeof (polyvertex_t) * poly->numpts;
	memset(poly, 0x00, size);
	//mempoly -= polysize;
#endif
}


// Return interception along bsp line,
// with the polygon segment
//
static float bspfrac;
static polyvertex_t *fracdivline(fdivline_t *bsp, polyvertex_t *v1,
	polyvertex_t *v2)
{
	static polyvertex_t pt;
	double frac;
	double num;
	double den;
	double v1x,v1y,v1dx,v1dy;
	double v2x,v2y,v2dx,v2dy;

	// a segment of a polygon
	v1x  = v1->x;
	v1y  = v1->y;
	v1dx = v2->x - v1->x;
	v1dy = v2->y - v1->y;

	// the bsp partition line
	v2x  = bsp->x;
	v2y  = bsp->y;
	v2dx = bsp->dx;
	v2dy = bsp->dy;

	den = v2dy*v1dx - v2dx*v1dy;
	if (fabsf((float)den) < 1.0E-36f) // avoid checking exactly for 0.0
		return NULL;       // parallel

	// first check the frac along the polygon segment,
	// (do not accept hit with the extensions)
	num = (v2x - v1x)*v2dy + (v1y - v2y)*v2dx;
	frac = num / den;
	if (frac < 0.0l || frac > 1.0l)
		return NULL;

	// now get the frac along the BSP line
	// which is useful to determine what is left, what is right
	num = (v2x - v1x)*v1dy + (v1y - v2y)*v1dx;
	frac = num / den;
	bspfrac = (float)frac;


	// find the interception point along the partition line
	pt.x = (float)(v2x + v2dx*frac);
	pt.y = (float)(v2y + v2dy*frac);

	return &pt;
}



static boolean SameVertice (polyvertex_t *p1, polyvertex_t *p2)
{
#if 0
	float diff;
	diff = p2->x - p1->x;
	if (diff < -1.5f || diff > 1.5f)
		return false;
	diff = p2->y - p1->y;
	if (diff < -1.5f || diff > 1.5f)
		return false;
#elif 0
	if (p1->x != p2->x)
		return false;
	if (p1->y != p2->y)
		return false;
#elif 0
	if (fabsf( p2->x - p1->x ) > 1.0E-36f )
		return false;
	if (fabsf( p2->y - p1->y ) > 1.0E-36f )
		return false;
#else
#define  DIVLINE_VERTEX_DIFF   HWR_POLYGON_EPSILON
	float ep = DIVLINE_VERTEX_DIFF;
	if (fabsf( p2->x - p1->x ) > ep )
		return false;
	if (fabsf( p2->y - p1->y ) > ep )
		return false;
#endif
	// p1 and p2 are considered the same vertex
	return true;
}


// split a _CONVEX_ polygon in two convex polygons
// outputs:
//   frontpoly : polygon on right side of bsp line
//   backpoly  : polygon on left side
//
static void SplitPoly (fdivline_t *bsp,         //splitting parametric line
                       poly_t *poly,            //the convex poly we split
                       poly_t **frontpoly,      //return one poly here
                       poly_t **backpoly)       //return the other here
{
	INT32      i,j;
	polyvertex_t *pv;

	INT32          ps = -1,pe = -1;
	INT32          nptfront,nptback;
	polyvertex_t vs = {0,0,0};
	polyvertex_t ve = {0,0,0};
	polyvertex_t lastpv = {0,0,0};
	float        fracs = 0.0f,frace = 0.0f; //used to tell which poly is on
	                                        // the front side of the bsp partition line
	INT32         psonline = 0, peonline = 0;

	for (i = 0; i < poly->numpts; i++)
	{
		j = i + 1;
		if (j == poly->numpts) j = 0;

		// start & end points
		pv = fracdivline(bsp, &poly->pts[i], &poly->pts[j]);

		if (pv == NULL)
			continue;

		if (ps < 0)
		{
			// first point
			ps = i;
			vs = *pv;
			fracs = bspfrac;
		}
		else
		{
			//the partition line traverse a junction between two segments
			// or the two points are so close, they can be considered as one
			// thus, don't accept, since split 2 must be another vertex
			if (SameVertice(pv, &lastpv))
			{
				if (pe < 0)
				{
					ps = i;
					psonline = 1;
				}
				else
				{
					pe = i;
					peonline = 1;
				}
			}
			else
			{
				if (pe < 0)
				{
					pe = i;
					ve = *pv;
					frace = bspfrac;
				}
				else
				{
				// a frac, not same vertice as last one
				// we already got pt2 so pt 2 is not on the line,
				// so we probably got back to the start point
				// which is on the line
					if (SameVertice(pv, &vs))
						psonline = 1;
					break;
				}
			}
		}

		// remember last point intercept to detect identical points
		lastpv = *pv;
	}

	// no split: the partition line is either parallel and
	// aligned with one of the poly segments, or the line is totally
	// out of the polygon and doesn't traverse it (happens if the bsp
	// is fooled by some trick where the sidedefs don't point to
	// the right sectors)
	if (ps < 0)
	{
		//I_Error("SplitPoly: did not split polygon (%d %d)\n"
		//        "debugpos %d",ps,pe,debugpos);

		// this eventually happens with 'broken' BSP's that accept
		// linedefs where each side point the same sector, that is:
		// the deep water effect with the original Doom

		/// \todo make sure front poly is to front of partition line?

		*frontpoly = poly;
		*backpoly = NULL;
		return;
	}

	if (pe < 0)
	{
		//I_Error("SplitPoly: only one point for split line (%d %d)", ps, pe);
		*frontpoly = poly;
		*backpoly = NULL;
		return;
	}
	if (pe <= ps)
		I_Error("SplitPoly: invalid splitting line (%d %d)", ps, pe);

	// number of points on each side, _not_ counting those
	// that may lie just one the line
	nptback  = pe - ps - peonline;
	nptfront = poly->numpts - peonline - psonline - nptback;

	if (nptback > 0)
		*backpoly = HWR_AllocPoly(2 + nptback);
	else
		*backpoly = NULL;
	if (nptfront > 0)
		*frontpoly = HWR_AllocPoly(2 + nptfront);
	else
		*frontpoly = NULL;

	// generate FRONT poly
	if (*frontpoly)
	{
		pv = (*frontpoly)->pts;
		*pv++ = vs;
		*pv++ = ve;
		i = pe;
		do
		{
			if (++i == poly->numpts)
				i = 0;
			*pv++ = poly->pts[i];
		} while (i != ps && --nptfront);
	}

	// generate BACK poly
	if (*backpoly)
	{
		pv = (*backpoly)->pts;
		*pv++ = ve;
		*pv++ = vs;
		i = ps;
		do
		{
			if (++i == poly->numpts)
				i = 0;
			*pv++ = poly->pts[i];
		} while (i != pe && --nptback);
	}

	// make sure frontpoly is the one on the 'right' side
	// of the partition line
	if (fracs > frace)
	{
		poly_t *swappoly;
		swappoly = *backpoly;
		*backpoly = *frontpoly;
		*frontpoly = swappoly;
	}

	HWR_FreePoly (poly);
}


// use each seg of the poly as a partition line, keep only the
// part of the convex poly to the front of the seg (that is,
// the part inside the sector), the part behind the seg, is
// the void space and is cut out
//
static poly_t *CutOutSubsecPoly(seg_t *lseg, INT32 count, poly_t *poly)
{
	INT32 i, j;

	polyvertex_t *pv;

	INT32 nump = 0, ps, pe;
	polyvertex_t vs = {0, 0, 0}, ve = {0, 0, 0},
		p1 = {0, 0, 0}, p2 = {0, 0, 0};
	float fracs = 0.0f;

	fdivline_t cutseg; // x, y, dx, dy as start of node_t struct

	poly_t *temppoly;

	// for each seg of the subsector
	for (; count--; lseg++)
	{
		line_t *line = lseg->linedef;

		if (lseg->glseg)
			continue;

		//x,y,dx,dy (like a divline)
		p1.x = FIXED_TO_FLOAT(lseg->side ? line->v2->x : line->v1->x);
		p1.y = FIXED_TO_FLOAT(lseg->side ? line->v2->y : line->v1->y);
		p2.x = FIXED_TO_FLOAT(lseg->side ? line->v1->x : line->v2->x);
		p2.y = FIXED_TO_FLOAT(lseg->side ? line->v1->y : line->v2->y);

		cutseg.x = p1.x;
		cutseg.y = p1.y;
		cutseg.dx = p2.x - p1.x;
		cutseg.dy = p2.y - p1.y;

		// see if it cuts the convex poly
		ps = -1;
		pe = -1;
		for (i = 0; i < poly->numpts; i++)
		{
			j = i + 1;
			if (j == poly->numpts)
				j = 0;

			pv = fracdivline(&cutseg, &poly->pts[i], &poly->pts[j]);

			if (pv == NULL)
				continue;

			if (ps < 0)
			{
				ps = i;
				vs = *pv;
				fracs = bspfrac;
			}
			else
			{
				//frac 1 on previous segment,
				//     0 on the next,
				//the split line goes through one of the convex poly
				// vertices, happens quite often since the convex
				// poly is already adjacent to the subsector segs
				// on most borders
				if (SameVertice(pv, &vs))
					continue;

				if (fracs <= bspfrac)
				{
					nump = 2 + poly->numpts - (i-ps);
					pe = ps;
					ps = i;
					ve = *pv;
				}
				else
				{
					nump = 2 + (i-ps);
					pe = i;
					ve = vs;
					vs = *pv;
				}
				//found 2nd point
				break;
			}
		}

		// there was a split
		if (ps >= 0)
		{
			//need 2 points
			if (pe >= 0)
			{
				// generate FRONT poly
				temppoly = HWR_AllocPoly(nump);
				pv = temppoly->pts;
				*pv++ = vs;
				*pv++ = ve;
				do
				{
					if (++ps == poly->numpts)
						ps = 0;
					*pv++ = poly->pts[ps];
				} while (ps != pe);
				HWR_FreePoly(poly);
				poly = temppoly;
			}
			//hmmm... maybe we should NOT accept this, but this happens
			// only when the cut is not needed it seems (when the cut
			// line is aligned to one of the borders of the poly, and
			// only some times..)
			else
				skipcut++;
			//    I_Error("CutOutPoly: only one point for split line (%d %d) %d", ps, pe, debugpos);
		}
	}
	return poly;
}

// At this point, the poly should be convex and the exact
// layout of the subsector, it is not always the case,
// so continue to cut off the poly into smaller parts with
// each seg of the subsector.
//
static inline void HWR_SubsecPoly(INT32 num, poly_t *poly)
{
	INT16 count;
	subsector_t *sub;
	seg_t *lseg;
	INT32 i;
	boolean closed = true;
	INT32 orientation = 0;

	sub = &subsectors[num];
	count = sub->numlines;
	lseg = &segs[sub->firstline];

	if (poly)
	{




		if (count < 3 || !hwr_closed_node_boundaries)
			closed = false;
		for (i = 0; closed && i < count; i++)
		{
			const vertex_t *a = lseg[i].v1, *b = lseg[i].v2;
			const vertex_t *c = lseg[(i+1)%count].v2;
			const double cross = ((double)b->x-a->x)*((double)c->y-b->y)
				- ((double)b->y-a->y)*((double)c->x-b->x);
			const vertex_t *next = lseg[(i+1)%count].v1;
			if (lseg[i].polyseg || b->x != next->x || b->y != next->y)
				closed = false;
			if (cross < 0.0 || cross > 0.0)
			{
				const INT32 direction = cross < 0.0 ? -1 : 1;
				if (orientation && direction != orientation)
					closed = false;
				orientation = direction;
			}
		}
		if (closed && orientation)
		{
			poly_t *exact = HWR_AllocPoly(count);
			for (i = 0; i < count; i++)
			{
				exact->pts[i].x = FIXED_TO_FLOAT(lseg[i].v1->x);
				exact->pts[i].y = FIXED_TO_FLOAT(lseg[i].v1->y);
				exact->pts[i].z = 0.0f;
			}
			HWR_FreePoly(poly);
			poly = exact;
		}
		else
			poly = CutOutSubsecPoly (lseg,count,poly);
		totalsubsecpolys++;
		//extra data for this subsector
		extrasubsectors[num].planepoly = poly;
	}
}

// the bsp divline have not enouth presition
// search for the segs source of this divline
static inline void SearchDivline(node_t *bsp, fdivline_t *divline)
{
	divline->x = FIXED_TO_FLOAT(bsp->x);
	divline->y = FIXED_TO_FLOAT(bsp->y);
	divline->dx = FIXED_TO_FLOAT(bsp->dx);
	divline->dy = FIXED_TO_FLOAT(bsp->dy);
}

#ifdef HWR_LOADING_SCREEN
//Hurdler: implement a loading status
static size_t ls_count = 0;
static UINT8 ls_percent = 0;

static void loading_status(void)
{
	char s[16];
	int x, y;

	I_OsPolling();
	CON_Drawer();
	sprintf(s, "%d%%", (++ls_percent)<<1);
	x = BASEVIDWIDTH/2;
	y = BASEVIDHEIGHT/2;
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31); // Black background to match fade in effect
	//V_DrawPatchFill(W_CachePatchName("SRB2BACK",PU_CACHE)); // SRB2 background, ehhh too bright.
	M_DrawTextBox(x-58, y-8, 13, 1);
	V_DrawString(x-50, y, V_YELLOWMAP, "Loading...");
	V_DrawRightAlignedString(x+50, y, V_YELLOWMAP, s);

	// Is this really necessary at this point..?
	V_DrawCenteredString(BASEVIDWIDTH/2, 40, V_YELLOWMAP, "OPENGL MODE IS INCOMPLETE AND MAY");
	V_DrawCenteredString(BASEVIDWIDTH/2, 50, V_YELLOWMAP, "NOT DISPLAY SOME SURFACES.");
	V_DrawCenteredString(BASEVIDWIDTH/2, 70, V_YELLOWMAP, "USE AT SONIC'S RISK.");

	I_UpdateNoVsync();
}
#endif

// poly : the convex polygon that encloses all child subsectors
static void WalkBSPNode(INT32 bspnum, poly_t *poly, UINT16 *leafnode, fixed_t *bbox)
{
	node_t *bsp;
	poly_t *backpoly, *frontpoly;
	fdivline_t fdivline;
	polyvertex_t *pt;
	INT32 i;

	// Found a subsector?
	if (bspnum & NF_SUBSECTOR)
	{
		if (bspnum == -1)
		{
			// BP: i think this code is useless and wrong because
			// - bspnum==-1 happens only when numsubsectors == 0
			// - it can't happens in bsp recursive call since bspnum is a INT32 and children is UINT16
			// - the BSP is complet !! (there just can have subsector without segs) (i am not sure of this point)

			// do we have a valid polygon ?
			if (poly && poly->numpts > 2)
			{
				CONS_Debug(DBG_RENDER, "Adding a new subsector\n");
				if (addsubsector == numsubsectors + NEWSUBSECTORS)
					I_Error("WalkBSPNode: not enough addsubsectors\n");
				else if (addsubsector > 0x7fff)
					I_Error("WalkBSPNode: addsubsector > 0x7fff\n");
				*leafnode = (UINT16)((UINT16)addsubsector | NF_SUBSECTOR);
				extrasubsectors[addsubsector].planepoly = poly;
				addsubsector++;
			}

			//add subsectors without segs here?
			//HWR_SubsecPoly(0, NULL);
		}
		else
		{
			HWR_SubsecPoly(bspnum & ~NF_SUBSECTOR, poly);

			//Hurdler: implement a loading status
#ifdef HWR_LOADING_SCREEN
			if (ls_count-- <= 0)
			{
				ls_count = numsubsectors/50;
				loading_status();
			}
#endif
		}
		M_ClearBox(bbox);
		poly = extrasubsectors[bspnum & ~NF_SUBSECTOR].planepoly;

		for (i = 0, pt = poly->pts; i < poly->numpts; i++,pt++)
			M_AddToBox(bbox, FLOAT_TO_FIXED(pt->x), FLOAT_TO_FIXED(pt->y));

		return;
	}

	bsp = &nodes[bspnum];
	SearchDivline(bsp, &fdivline);
	SplitPoly(&fdivline, poly, &frontpoly, &backpoly);
	poly = NULL;

	//debug
	if (!backpoly)
		nobackpoly++;

	// Recursively divide front space.
	if (frontpoly)
	{
		WalkBSPNode(bsp->children[0], frontpoly, &bsp->children[0],bsp->bbox[0]);

		// copy child bbox
		M_Memcpy(bbox, bsp->bbox[0], 4*sizeof (fixed_t));
	}
	else
		I_Error("WalkBSPNode: no front poly?");

	// Recursively divide back space.
	if (backpoly)
	{
		// Correct back bbox to include floor/ceiling convex polygon
		WalkBSPNode(bsp->children[1], backpoly, &bsp->children[1], bsp->bbox[1]);

		// enlarge bbox with second child
		M_AddToBox(bbox, bsp->bbox[1][BOXLEFT  ],
		                 bsp->bbox[1][BOXTOP   ]);
		M_AddToBox(bbox, bsp->bbox[1][BOXRIGHT ],
		                 bsp->bbox[1][BOXBOTTOM]);
	}
}

// FIXME: use Z_Malloc() STATIC ?
void HWR_FreeExtraSubsectors(void)
{
	if (extrasubsectors)
		free(extrasubsectors);
	extrasubsectors = NULL;
}

#define MAXDIST HWR_POLYGON_EPSILON
// BP: can't move vertex: DON'T change polygon geometry! (convex)
//#define MOVEVERTEX
static boolean PointInSeg(polyvertex_t *a,polyvertex_t *v1,polyvertex_t *v2)
{
	register float ax,ay,bx,by,cx,cy,d,norm;
	register polyvertex_t *p;

	// check bbox of the seg first
	if (v1->x > v2->x)
	{
		p = v1;
		v1 = v2;
		v2 = p;
	}

	if (a->x < v1->x-MAXDIST || a->x > v2->x+MAXDIST)
		return false;

	if (v1->y > v2->y)
	{
		p = v1;
		v1 = v2;
		v2 = p;
	}
	if (a->y < v1->y-MAXDIST || a->y > v2->y+MAXDIST)
		return false;

	// v1 = origine
	ax= v2->x-v1->x;
	ay= v2->y-v1->y;
	norm = (float)hypot(ax, ay);
	ax /= norm;
	ay /= norm;
	bx = a->x-v1->x;
	by = a->y-v1->y;
	//d = a.b
	d =ax*bx+ay*by;
	// bound of the seg
	if (d < 0 || d > norm)
		return false;
	//c = d.1a-b
	cx = ax*d-bx;
	cy = ay*d-by;
#ifdef MOVEVERTEX
	if (cx*cx+cy*cy <= MAXDIST*MAXDIST)
	{
		// ajust a little the point position
		a->x = ax*d+v1->x;
		a->y = ay*d+v1->y;
		// anyway the correction is not enouth
		return true;
	}
	return false;
#else
	return cx*cx+cy*cy <= MAXDIST*MAXDIST;
#endif
}

static INT32 numsplitpoly;




static void HWR_SnapPlaneCorners(void)
{
	size_t s;
	for (s = 0; s < numsubsectors; s++)
	{
		poly_t *poly = extrasubsectors[s].planepoly;
		const seg_t *first = &segs[subsectors[s].firstline];
		INT32 p, i;
		if (!poly)
			continue;
		for (p = 0; p < poly->numpts; p++)
		{
			polyvertex_t *point = &poly->pts[p];
			const float magnitude = max(1.0f, max(fabsf(point->x), fabsf(point->y)));
			const float epsilon = min(0.005f, 2.0f*FLT_EPSILON*magnitude);
			double nearest = (double)epsilon*epsilon;
			const vertex_t *best = NULL;
			for (i = 0; i < subsectors[s].numlines; i++)
			{
				const line_t *line = first[i].linedef;
				const vertex_t *ends[4] = {first[i].v1, first[i].v2,
					line ? line->v1 : NULL, line ? line->v2 : NULL};
				INT32 j;
				for (j = 0; j < 4; j++)
				{
					double dx, dy, distance;
					if (!ends[j])
						continue;
					dx = (double)point->x-FIXED_TO_FLOAT(ends[j]->x);
					dy = (double)point->y-FIXED_TO_FLOAT(ends[j]->y);
					distance = dx*dx+dy*dy;
					if (distance <= nearest)
					{
						nearest = distance;
						best = ends[j];
					}
				}
			}
			if (best)
			{
				point->x = FIXED_TO_FLOAT(best->x);
				point->y = FIXED_TO_FLOAT(best->y);
			}
		}
	}
}

static void SearchSegInBSP(INT32 bspnum,polyvertex_t *p,poly_t *poly)
{
	poly_t  *q;
	INT32     j,k;

	if (bspnum & NF_SUBSECTOR)
	{
		if (bspnum != -1)
		{
			bspnum &= ~NF_SUBSECTOR;
			q = extrasubsectors[bspnum].planepoly;
			if (poly == q || !q)
				return;



			for (j = 0; j < q->numpts; j++)
				if (SameVertice(p, &q->pts[j]))
					return;
			for (j = 0; j < q->numpts; j++)
			{
				k = j+1;
				if (k == q->numpts) k = 0;
				if (!SameVertice(p, &q->pts[j])
					&& !SameVertice(p, &q->pts[k])
					&& PointInSeg(p, &q->pts[j],
						&q->pts[k]))
				{
					poly_t *newpoly = HWR_AllocPoly(q->numpts+1);
					INT32 n;

					for (n = 0; n <= j; n++)
						newpoly->pts[n] = q->pts[n];
					newpoly->pts[k] = *p;
					for (n = k+1; n < newpoly->numpts; n++)
						newpoly->pts[n] = q->pts[n-1];
					numsplitpoly++;
					extrasubsectors[bspnum].planepoly =
						newpoly;
					HWR_FreePoly(q);
					return;
				}
			}
		}
		return;
	}

	if ((FIXED_TO_FLOAT(nodes[bspnum].bbox[0][BOXBOTTOM])-MAXDIST <= p->y) &&
	    (FIXED_TO_FLOAT(nodes[bspnum].bbox[0][BOXTOP   ])+MAXDIST >= p->y) &&
	    (FIXED_TO_FLOAT(nodes[bspnum].bbox[0][BOXLEFT  ])-MAXDIST <= p->x) &&
	    (FIXED_TO_FLOAT(nodes[bspnum].bbox[0][BOXRIGHT ])+MAXDIST >= p->x))
		SearchSegInBSP(nodes[bspnum].children[0],p,poly);

	if ((FIXED_TO_FLOAT(nodes[bspnum].bbox[1][BOXBOTTOM])-MAXDIST <= p->y) &&
	    (FIXED_TO_FLOAT(nodes[bspnum].bbox[1][BOXTOP   ])+MAXDIST >= p->y) &&
	    (FIXED_TO_FLOAT(nodes[bspnum].bbox[1][BOXLEFT  ])-MAXDIST <= p->x) &&
	    (FIXED_TO_FLOAT(nodes[bspnum].bbox[1][BOXRIGHT ])+MAXDIST >= p->x))
		SearchSegInBSP(nodes[bspnum].children[1],p,poly);
}

// search for T-intersection problem
// BP : It can be mush more faster doing this at the same time of the splitpoly
// but we must use a different structure : polygone pointing on segs
// segs pointing on polygone and on vertex (too mush complicated, well not
// realy but i am soo lasy), the methode discibed is also better for segs presition
static INT32 SolveTProblem(void)
{
	poly_t *p;
	INT32 i;
	size_t l;

	if (cv_glsolvetjoin.value == 0)
		return 0;

	CONS_Debug(DBG_RENDER, "Solving T-joins. This may take a while. Please wait...\n");
#ifdef HWR_LOADING_SCREEN
	CON_Drawer(); //let the user know what we are doing
	I_FinishUpdate(); // page flip or blit buffer
#endif

	numsplitpoly = 0;

	for (l = 0; l < addsubsector; l++)
	{
		p = extrasubsectors[l].planepoly;
		if (p)
			for (i = 0; i < p->numpts; i++)
				SearchSegInBSP((INT32)numnodes-1, &p->pts[i], p);
	}
	//CONS_Debug(DBG_RENDER, "numsplitpoly %d\n", numsplitpoly);
	return numsplitpoly;
}

// Give each shared BSP vertex a single OpenGL position.
static void AdjustSegs(void)
{
	const float maxsnap = 0.75f;
	polyvertex_t *mapvertices = Z_Malloc(numvertexes*sizeof (*mapvertices), PU_HWRPLANE, NULL);
	float *snap = Z_Malloc(numvertexes*sizeof (*snap), PU_STATIC, NULL);
	size_t i;

	for (i = 0; i < numvertexes; i++)
	{
		mapvertices[i].x = FIXED_TO_FLOAT(vertexes[i].x);
		mapvertices[i].y = FIXED_TO_FLOAT(vertexes[i].y);
		mapvertices[i].z = 0.0f;
		snap[i] = maxsnap*maxsnap;
	}

	// Real map corners keep their original coordinates.
	for (i = 0; i < numlines; i++)
	{
		snap[lines[i].v1-vertexes] = 0.0f;
		snap[lines[i].v2-vertexes] = 0.0f;
	}

	for (i = 0; i < numsubsectors; i++)
	{
		const subsector_t *sub = &subsectors[i];
		const poly_t *poly = extrasubsectors[i].planepoly;
		for (INT32 j = 0; j < sub->numlines; j++)
		{
			const seg_t *seg = &segs[sub->firstline+j];
			const vertex_t *ends[2] = {seg->v1, seg->v2};
			if (!poly || seg->polyseg)
				continue;
			for (INT32 end = 0; end < 2; end++)
			{
				const size_t index = ends[end]-vertexes;
				if (snap[index] <= 0.0f)
					continue;
				const float x = FIXED_TO_FLOAT(ends[end]->x);
				const float y = FIXED_TO_FLOAT(ends[end]->y);
				for (INT32 p = 0; p < poly->numpts; p++)
				{
					const float dx = poly->pts[p].x - x;
					const float dy = poly->pts[p].y - y;
					const float distance = dx*dx + dy*dy;
					if (distance < snap[index])
					{
						mapvertices[index].x = poly->pts[p].x;
						mapvertices[index].y = poly->pts[p].y;
						snap[index] = distance;
					}
				}
			}
		}
	}

	for (i = 0; i < numsegs; i++)
	{
		seg_t *seg = &segs[i];
		if (seg->polyseg)
			continue;
		polyvertex_t *v1 = &mapvertices[seg->v1-vertexes];
		polyvertex_t *v2 = &mapvertices[seg->v2-vertexes];
		seg->pv1 = v1;
		seg->pv2 = v2;
		seg->flength = (float)hypot(v2->x-v1->x, v2->y-v1->y);
	}
	Z_Free(snap);
}

static double HWR_SectorLineSide(const line_t *line, const sector_t *sector,
	double x, double y)
{
	const double x1 = FIXED_TO_FLOAT(line->v1->x);
	const double y1 = FIXED_TO_FLOAT(line->v1->y);
	const double dx = (double)FIXED_TO_FLOAT(line->v2->x) - x1;
	const double dy = (double)FIXED_TO_FLOAT(line->v2->y) - y1;
	const double side = dx*(y-y1)-dy*(x-x1);
	return line->frontsector == sector ? side : -side;
}





typedef struct hwr_sector_hull_s
{
	size_t count;
	sector_t *floorbackground, *ceilingbackground;
	vertex_t *points[0];
} hwr_sector_hull_t;

static int HWR_CompareHullVertices(const void *first, const void *second)
{
	const vertex_t *a = *(vertex_t * const *)first;
	const vertex_t *b = *(vertex_t * const *)second;
	if (a->x != b->x)
		return a->x < b->x ? -1 : 1;
	return a->y < b->y ? -1 : a->y > b->y;
}

static double HWR_HullVertexCross(const vertex_t *a, const vertex_t *b,
	const vertex_t *c)
{
	return ((double)b->x-a->x)*((double)c->y-a->y)
		- ((double)b->y-a->y)*((double)c->x-a->x);
}

static INT32 HWR_HullComponentRoot(INT32 *parents, INT32 index)
{
	while (parents[index] != index)
	{
		parents[index] = parents[parents[index]];
		index = parents[index];
	}
	return index;
}

static poly_t *HWR_ClipPlaneToSectorLine(poly_t *poly, const line_t *line,
	const sector_t *sector)
{
	poly_t *clipped;
	INT32 i, previous, count = 0;
	boolean outside = false;
	const double dx = (double)FIXED_TO_FLOAT(line->v2->x) - FIXED_TO_FLOAT(line->v1->x);
	const double dy = (double)FIXED_TO_FLOAT(line->v2->y) - FIXED_TO_FLOAT(line->v1->y);
	const double epsilon = hypot(dx, dy)*0.0001;
	if (line->frontsector == line->backsector || poly->numpts < 3 || epsilon <= 0.0)
		return poly;
	for (i = 0; i < poly->numpts; i++)
		if (HWR_SectorLineSide(line, sector, poly->pts[i].x, poly->pts[i].y) > epsilon)
		{
			outside = true;
			break;
		}
	if (!outside)
		return poly;


	clipped = HWR_AllocPoly(poly->numpts*2);
	previous = poly->numpts-1;
	for (i = 0; i < poly->numpts; previous = i++)
	{
		const polyvertex_t *a = &poly->pts[previous], *b = &poly->pts[i];
		const double sa = HWR_SectorLineSide(line, sector, a->x, a->y);
		const double sb = HWR_SectorLineSide(line, sector, b->x, b->y);
		const boolean ina = sa <= epsilon, inb = sb <= epsilon;
		if (ina != inb)
		{
			const double fraction = (sa-epsilon)/(sa-sb);
			polyvertex_t *point = &clipped->pts[count++];
			point->x = (float)(a->x+((double)b->x-a->x)*fraction);
			point->y = (float)(a->y+((double)b->y-a->y)*fraction);
			point->z = 0.0f;
		}
		if (inb)
			clipped->pts[count++] = *b;
	}
	clipped->numpts = count;
	HWR_FreePoly(poly);
	return clipped;
}



static void HWR_SplitPlaneRegion(poly_t *poly, const line_t *line,
	poly_t **right, poly_t **left)
{
	const double x = FIXED_TO_FLOAT(line->v1->x);
	const double y = FIXED_TO_FLOAT(line->v1->y);
	const double dx = (double)FIXED_TO_FLOAT(line->v2->x)-x;
	const double dy = (double)FIXED_TO_FLOAT(line->v2->y)-y;
	const double epsilon = hypot(dx, dy)*0.00001;
	double low = DBL_MAX, high = -DBL_MAX;
	INT32 i, previous, nr = 0, nl = 0;
	*right = *left = NULL;
	for (i = 0; i < poly->numpts; i++)
	{
		const double side = dx*(poly->pts[i].y-y)-dy*(poly->pts[i].x-x);
		low = min(low, side);
		high = max(high, side);
	}
	if (high <= epsilon)
	{
		*right = poly;
		return;
	}
	if (low >= -epsilon)
	{
		*left = poly;
		return;
	}
	*right = HWR_AllocPoly(poly->numpts*2);
	*left = HWR_AllocPoly(poly->numpts*2);
	previous = poly->numpts-1;
	for (i = 0; i < poly->numpts; previous = i++)
	{
		const polyvertex_t *a = &poly->pts[previous], *b = &poly->pts[i];
		const double sa = dx*(a->y-y)-dy*(a->x-x);
		const double sb = dx*(b->y-y)-dy*(b->x-x);
		if ((sa < 0.0 && sb > 0.0) || (sa > 0.0 && sb < 0.0))
		{
			const double fraction = sa/(sa-sb);
			polyvertex_t point;
			point.x = (float)(a->x+((double)b->x-a->x)*fraction);
			point.y = (float)(a->y+((double)b->y-a->y)*fraction);
			point.z = 0.0f;
			(*right)->pts[nr++] = (*left)->pts[nl++] = point;
		}
		if (sb <= 0.0) (*right)->pts[nr++] = *b;
		if (sb >= 0.0) (*left)->pts[nl++] = *b;
	}
	(*right)->numpts = nr;
	(*left)->numpts = nl;
	HWR_FreePoly(poly);
}

static hwr_plane_region_t *HWR_NewPlaneRegion(poly_t *poly,
	hwr_plane_region_t *next)
{
	hwr_plane_region_t *region = Z_Calloc(sizeof (*region), PU_HWRPLANE, NULL);
	region->xsub.planepoly = poly;
	region->next = next;
	return region;
}




static sector_t *HWR_PlaneRegionSector(double x, double y)
{
	double nearest = DBL_MAX;
	sector_t *sector = NULL;
	size_t i;
	for (i = 0; i < numlines; i++)
	{
		const line_t *line = &lines[i];
		const double x1 = FIXED_TO_FLOAT(line->v1->x);
		const double y1 = FIXED_TO_FLOAT(line->v1->y);
		const double x2 = FIXED_TO_FLOAT(line->v2->x);
		const double y2 = FIXED_TO_FLOAT(line->v2->y);
		double crossing;
		if (line->polyobj || line->frontsector == line->backsector
		|| (y1 > y) == (y2 > y))
			continue;
		crossing = x1+(x2-x1)*(y-y1)/(y2-y1);
		if (crossing >= x && crossing < nearest)
		{
			nearest = crossing;
			sector = y2 > y1 ? line->backsector : line->frontsector;
		}
	}
	return sector;
}





static void HWR_BuildPlaneRegions(extrasubsector_t *xsub, sector_t *sector,
	const hwr_sector_hull_t *hull)
{
	const poly_t *original = xsub->planepoly;
	poly_t *inside = HWR_AllocPoly(original->numpts);
	hwr_plane_region_t *pieces = NULL, *region;
	float cellminx = FLT_MAX, cellminy = FLT_MAX;
	float cellmaxx = -FLT_MAX, cellmaxy = -FLT_MAX;
	size_t i;
	memcpy(inside->pts, original->pts, original->numpts*sizeof (*original->pts));
	for (i = 0; i < (size_t)original->numpts; i++)
	{
		cellminx = min(cellminx, original->pts[i].x); cellmaxx = max(cellmaxx, original->pts[i].x);
		cellminy = min(cellminy, original->pts[i].y); cellmaxy = max(cellmaxy, original->pts[i].y);
	}
	for (i = 0; inside && i < hull->count; i++)
	{
		line_t boundary;
		poly_t *outside;
		memset(&boundary, 0, sizeof boundary);
		boundary.v1 = hull->points[(i+1)%hull->count];
		boundary.v2 = hull->points[i];
		HWR_SplitPlaneRegion(inside, &boundary, &inside, &outside);
		if (outside)
			pieces = HWR_NewPlaneRegion(outside, pieces);
	}
	if (inside) HWR_FreePoly(inside);
	for (i = 0; pieces && i < numlines; i++)
	{
		const line_t *line = &lines[i];
		const float x1 = FIXED_TO_FLOAT(line->v1->x), x2 = FIXED_TO_FLOAT(line->v2->x);
		const float y1 = FIXED_TO_FLOAT(line->v1->y), y2 = FIXED_TO_FLOAT(line->v2->y);
		if (line->polyobj || line->frontsector == line->backsector
		|| cellmaxx < min(x1, x2) || cellminx > max(x1, x2)
		|| cellmaxy < min(y1, y2) || cellminy > max(y1, y2))
			continue;
		for (region = pieces; region; region = region->next)
		{
			poly_t *right, *left, *poly = region->xsub.planepoly;
			float minx = FLT_MAX, miny = FLT_MAX, maxx = -FLT_MAX, maxy = -FLT_MAX;
			INT32 p;
			for (p = 0; p < poly->numpts; p++)
			{
				minx = min(minx, poly->pts[p].x); maxx = max(maxx, poly->pts[p].x);
				miny = min(miny, poly->pts[p].y); maxy = max(maxy, poly->pts[p].y);
			}
			if (maxx < min(x1, x2) || minx > max(x1, x2)
			|| maxy < min(y1, y2) || miny > max(y1, y2))
				continue;
			HWR_SplitPlaneRegion(poly, line, &right, &left);
			region->xsub.planepoly = right ? right : left;
			if (right && left)
			{
				region->next = HWR_NewPlaneRegion(left, region->next);
				region = region->next;
			}
		}
	}
	while (pieces)
	{
		poly_t *poly = pieces->xsub.planepoly;
		double x = 0.0, y = 0.0, area = 0.0;
		INT32 p, previous = poly->numpts-1;
		region = pieces;
		pieces = pieces->next;
		for (p = 0; p < poly->numpts; previous = p++)
		{
			x += poly->pts[p].x; y += poly->pts[p].y;
			area += (double)poly->pts[previous].x*poly->pts[p].y
				-(double)poly->pts[p].x*poly->pts[previous].y;
		}
		if (poly->numpts >= 3 && fabs(area) > 0.0000001)
			region->sector = HWR_PlaneRegionSector(x/poly->numpts, y/poly->numpts);
		if (region->sector && region->sector != sector)
		{
			region->next = xsub->plane_regions;
			xsub->plane_regions = region;
		}
		else
		{
			HWR_FreePoly(poly);
			Z_Free(region);
		}
	}
}

static void HWR_ConstrainSectorPlanes(void)
{
	hwr_sector_hull_t **linehulls = calloc(numlines*2, sizeof (*linehulls));
	hwr_sector_hull_t **allocated = calloc(numlines*2, sizeof (*allocated));
	UINT32 *stamps = calloc(numvertexes, sizeof (*stamps));
	INT32 *owners = calloc(numvertexes, sizeof (*owners));
	size_t i, j, numallocated = 0;
	if (!linehulls || !allocated || !stamps || !owners)
		I_Error("Out of memory checking OpenGL sector boundaries");

	for (i = 0; i < numsectors; i++)
	{
		sector_t *sector = &sectors[i];
		INT32 parents[256];
		double areas[256] = {0};
		boolean moving = false;
		if (sector->linecount < 3 || sector->linecount > 256)
			continue;
		for (j = 0; j < sector->linecount; j++)
			if (sector->lines[j]->polyobj)
				moving = true;
		if (moving)
			continue;
		for (j = 0; j < sector->linecount; j++)
			parents[j] = (INT32)j;



		for (j = 0; j < sector->linecount; j++)
		{
			line_t *line = sector->lines[j];
			vertex_t *ends[2] = {line->v1, line->v2};
			size_t end;
			if (line->frontsector == line->backsector)
				continue;
			for (end = 0; end < 2; end++)
			{
				const size_t index = (size_t)(ends[end]-vertexes);
				if (index >= numvertexes)
					continue;
				if (stamps[index] == i+1)
					parents[HWR_HullComponentRoot(parents, (INT32)j)] =
						HWR_HullComponentRoot(parents, owners[index]);
				else
				{
					stamps[index] = (UINT32)(i+1);
					owners[index] = (INT32)j;
				}
			}
		}
		for (j = 0; j < sector->linecount; j++)
		{
			line_t *line = sector->lines[j];
			double area;
			if (line->frontsector == line->backsector)
				continue;
			area = (double)line->v1->x*line->v2->y
				- (double)line->v2->x*line->v1->y;
			areas[HWR_HullComponentRoot(parents, (INT32)j)] +=
				line->frontsector == sector ? area : -area;
		}
		for (j = 0; j < sector->linecount; j++)
		{
			vertex_t *points[512], *hull[1024];
			size_t k, count = 0, hullcount = 0, lower;
			hwr_sector_hull_t *result;


			if (parents[j] != (INT32)j || areas[j] >= 0.0)
				continue;
			for (k = 0; k < sector->linecount; k++)
			{
				line_t *line = sector->lines[k];
				if (line->frontsector != line->backsector
				&& HWR_HullComponentRoot(parents, (INT32)k) == (INT32)j)
				{
					points[count++] = line->v1;
					points[count++] = line->v2;
				}
			}
			qsort(points, count, sizeof (*points), HWR_CompareHullVertices);
			for (k = 0; k < count; k++)
			{
				if (k && points[k]->x == points[k-1]->x
				&& points[k]->y == points[k-1]->y)
					continue;
				while (hullcount >= 2 && HWR_HullVertexCross(
					hull[hullcount-2], hull[hullcount-1], points[k]) <= 0.0)
					hullcount--;
				hull[hullcount++] = points[k];
			}
			lower = hullcount;
			for (k = count; k-- > 0;)
			{
				if (k+1 < count && points[k]->x == points[k+1]->x
				&& points[k]->y == points[k+1]->y)
					continue;
				while (hullcount > lower && HWR_HullVertexCross(
					hull[hullcount-2], hull[hullcount-1], points[k]) <= 0.0)
					hullcount--;
				hull[hullcount++] = points[k];
			}
			if (hullcount <= 3)
				continue;
			hullcount--;
			result = malloc(sizeof (*result)+hullcount*sizeof (*result->points));
			if (!result)
				I_Error("Out of memory bounding OpenGL sector island");
			result->count = hullcount;
			result->floorbackground = result->ceilingbackground = sector;
			memcpy(result->points, hull, hullcount*sizeof (*hull));
			allocated[numallocated++] = result;
			for (k = 0; k < sector->linecount; k++)
			{
				line_t *line = sector->lines[k];
				if (line->frontsector != line->backsector
				&& HWR_HullComponentRoot(parents, (INT32)k) == (INT32)j)
				{
					sector_t *outside = line->frontsector == sector
						? line->backsector : line->frontsector;
					linehulls[(line-lines)*2+(line->frontsector != sector)] = result;
					if (outside && outside->floorheight < result->floorbackground->floorheight)
						result->floorbackground = outside;
					if (outside && outside->ceilingheight > result->ceilingbackground->ceilingheight)
						result->ceilingbackground = outside;
				}
			}
		}
	}
	for (i = 0; i < numsubsectors; i++)
	{
		sector_t *sector = subsectors[i].sector;
		poly_t *poly = extrasubsectors[i].planepoly;
		const hwr_sector_hull_t *hull = NULL;
		boolean ambiguous = false;
		if (!poly || !sector || subsectors[i].numlines <= 0)
			continue;
		for (j = 0; j < (size_t)subsectors[i].numlines; j++)
		{
			const seg_t *seg = &segs[subsectors[i].firstline+j];
			const hwr_sector_hull_t *candidate;
			if (seg->glseg || !seg->linedef || seg->polyseg)
				continue;
			candidate = linehulls[(seg->linedef-lines)*2
				+(seg->linedef->frontsector != sector)];
			if (candidate && hull && hull != candidate)
				ambiguous = true;
			if (candidate)
				hull = candidate;
		}
		if (!hull || ambiguous)
			continue;



		{
			poly_t *copy = HWR_AllocPoly(poly->numpts);
			memcpy(copy->pts, poly->pts, poly->numpts*sizeof (*poly->pts));
			poly = copy;
		}
		for (j = 0; j < hull->count && poly->numpts >= 3; j++)
		{
			line_t boundary;
			memset(&boundary, 0, sizeof boundary);
			boundary.frontsector = sector;

			boundary.v1 = hull->points[(j+1)%hull->count];
			boundary.v2 = hull->points[j];
			poly = HWR_ClipPlaneToSectorLine(poly, &boundary, sector);
		}
		{
			const poly_t *original = extrasubsectors[i].planepoly;
			boolean different = original->numpts != poly->numpts;
			for (j = 0; !different && j < (size_t)poly->numpts; j++)
				if (memcmp(&original->pts[j].x, &poly->pts[j].x,
					2*sizeof (poly->pts[j].x)))
					different = true;
			if (different)
			{
				extrasubsectors[i].boundedpoly = poly;
				extrasubsectors[i].floorbackground = hull->floorbackground;
				extrasubsectors[i].ceilingbackground = hull->ceilingbackground;
				HWR_BuildPlaneRegions(&extrasubsectors[i], sector, hull);
			}
			else
				HWR_FreePoly(poly);
		}
	}
	for (i = 0; i < numallocated; i++)
		free(allocated[i]);
	free(owners);
	free(stamps);
	free(allocated);
	free(linehulls);
}

// call this routine after the BSP of a Doom wad file is loaded,
// and it will generate all the convex polys for the hardware renderer
void HWR_CreatePlanePolygons(INT32 bspnum)
{
	poly_t *rootp;
	polyvertex_t *rootpv;
	size_t i;
	fixed_t rootbbox[4];

	CONS_Debug(DBG_RENDER, "Creating polygons, please wait...\n");
#ifdef HWR_LOADING_SCREEN
	ls_count = ls_percent = 0; // reset the loading status
	CON_Drawer(); //let the user know what we are doing
	I_FinishUpdate(); // page flip or blit buffer
#endif

	HWR_ClearPolys();
	hwr_closed_node_boundaries = false;
	for (i = 0; i < numsegs; i++)
		if (segs[i].glseg)
		{
			hwr_closed_node_boundaries = true;
			break;
		}

	// find min/max boundaries of map
	//CONS_Debug(DBG_RENDER, "Looking for boundaries of map...\n");
	M_ClearBox(rootbbox);
	for (i = 0;i < numvertexes; i++)
		M_AddToBox(rootbbox, vertexes[i].x, vertexes[i].y);

	//CONS_Debug(DBG_RENDER, "Generating subsector polygons... %d subsectors\n", numsubsectors);

	HWR_FreeExtraSubsectors();
	// allocate extra data for each subsector present in map
	totsubsectors = numsubsectors + NEWSUBSECTORS;
	extrasubsectors = calloc(totsubsectors, sizeof (*extrasubsectors));
	if (extrasubsectors == NULL)
		I_Error("couldn't malloc extrasubsectors totsubsectors %s\n", sizeu1(totsubsectors));

	// allocate table for back to front drawing of subsectors
	/*gl_drawsubsectors = (INT16 *)malloc(sizeof (*gl_drawsubsectors) * totsubsectors);
	if (!gl_drawsubsectors)
		I_Error("couldn't malloc gl_drawsubsectors\n");*/

	// number of the first new subsector that might be added
	addsubsector = numsubsectors;

	// construct the initial convex poly that encloses the full map
	rootp = HWR_AllocPoly(4);
	rootpv = rootp->pts;

	rootpv->x = FIXED_TO_FLOAT(rootbbox[BOXLEFT  ]);
	rootpv->y = FIXED_TO_FLOAT(rootbbox[BOXBOTTOM]);  //lr
	rootpv++;
	rootpv->x = FIXED_TO_FLOAT(rootbbox[BOXLEFT  ]);
	rootpv->y = FIXED_TO_FLOAT(rootbbox[BOXTOP   ]);  //ur
	rootpv++;
	rootpv->x = FIXED_TO_FLOAT(rootbbox[BOXRIGHT ]);
	rootpv->y = FIXED_TO_FLOAT(rootbbox[BOXTOP   ]);  //ul
	rootpv++;
	rootpv->x = FIXED_TO_FLOAT(rootbbox[BOXRIGHT ]);
	rootpv->y = FIXED_TO_FLOAT(rootbbox[BOXBOTTOM]);  //ll
	rootpv++;

	WalkBSPNode(bspnum, rootp, NULL,rootbbox);

	HWR_SnapPlaneCorners();
	SolveTProblem();
	HWR_ConstrainSectorPlanes();
	AdjustSegs();

	//debug debug..
	//if (nobackpoly)
	//    CONS_Debug(DBG_RENDER, "no back polygon %u times\n",nobackpoly);
	//"(should happen only with the deep water trick)"
	//if (skipcut)
	//    CONS_Debug(DBG_RENDER, "%u cuts were skipped because of only one point\n",skipcut);

	//CONS_Debug(DBG_RENDER, "done: %u total subsector convex polygons\n", totalsubsecpolys);
}

#endif //HWRENDER
