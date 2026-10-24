// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



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
#include "../p_setup.h"  





extrasubsector_t *extrasubsectors = NULL;


#define NEWSUBSECTORS 50
static size_t totsubsectors;
size_t addsubsector;

typedef struct
{
	float x, y;
	float dx, dy;
} fdivline_t;






static INT32 nobackpoly = 0;
static INT32 skipcut = 0;
static INT32 totalsubsecpolys = 0;






#define ZPLANALLOC
#ifndef ZPLANALLOC


static size_t POLYPOOLSIZE = 1024000;

static UINT8 *gl_polypool = NULL;
static UINT8 *gl_ppcurrent;
static size_t gl_ppfree;
#endif


static void HWR_ClearPolys(void)
{
#ifndef ZPLANALLOC
	gl_ppcurrent = gl_polypool;
	gl_ppfree = POLYPOOLSIZE;
#endif
}


void HWR_InitPolyPool(void)
{
#ifndef ZPLANALLOC
	INT32 pnum;


	if ((pnum = M_CheckParm("-polypoolsize")))
		POLYPOOLSIZE = atoi(myargv[pnum+1])*1024;  

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

static polyvertex_t *HWR_AllocVertex(void)
{
	polyvertex_t *p;
	size_t size = sizeof (polyvertex_t);
#ifdef ZPLANALLOC
	p = Z_Malloc(size, PU_HWRPLANE, NULL);
#else
	if (gl_ppfree < size)
		I_Error("HWR_AllocVertex(): no more memory %u bytes left, %u bytes needed\n\n%s\n",
		        gl_ppfree, size, "You can try the param -polypoolsize 2048 (or higher if needed)");

	p = (polyvertex_t *)gl_ppcurrent;
	gl_ppcurrent += size;
	gl_ppfree -= size;
#endif
	return p;
}



static void HWR_FreePoly(poly_t *poly)
{
#ifdef ZPLANALLOC
	Z_Free(poly);
#else
	const size_t size = sizeof (poly_t) + sizeof (polyvertex_t) * poly->numpts;
	memset(poly, 0x00, size);

#endif
}





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


	v1x  = v1->x;
	v1y  = v1->y;
	v1dx = v2->x - v1->x;
	v1dy = v2->y - v1->y;


	v2x  = bsp->x;
	v2y  = bsp->y;
	v2dx = bsp->dx;
	v2dy = bsp->dy;

	den = v2dy*v1dx - v2dx*v1dy;
	if (fabsf((float)den) < 1.0E-36f)  
		return NULL;        



	num = (v2x - v1x)*v2dy + (v1y - v2y)*v2dx;
	frac = num / den;
	if (frac < 0.0l || frac > 1.0l)
		return NULL;



	num = (v2x - v1x)*v1dy + (v1y - v2y)*v1dx;
	frac = num / den;
	bspfrac = (float)frac;



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
#define  DIVLINE_VERTEX_DIFF   0.45f
	float ep = DIVLINE_VERTEX_DIFF;
	if (fabsf( p2->x - p1->x ) > ep )
		return false;
	if (fabsf( p2->y - p1->y ) > ep )
		return false;
#endif

	return true;
}







static void SplitPoly (fdivline_t *bsp,          
                       poly_t *poly,             
                       poly_t **frontpoly,       
                       poly_t **backpoly)        
{
	INT32      i,j;
	polyvertex_t *pv;

	INT32          ps = -1,pe = -1;
	INT32          nptfront,nptback;
	polyvertex_t vs = {0,0,0};
	polyvertex_t ve = {0,0,0};
	polyvertex_t lastpv = {0,0,0};
	float        fracs = 0.0f,frace = 0.0f;  

	INT32         psonline = 0, peonline = 0;

	for (i = 0; i < poly->numpts; i++)
	{
		j = i + 1;
		if (j == poly->numpts) j = 0;


		pv = fracdivline(bsp, &poly->pts[i], &poly->pts[j]);

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




					if (SameVertice(pv, &vs))
						psonline = 1;
					break;
				}
			}
		}


		lastpv = *pv;
	}






	if (ps < 0)
	{









		*frontpoly = poly;
		*backpoly = NULL;
		return;
	}

	if (pe < 0)
	{

		*frontpoly = poly;
		*backpoly = NULL;
		return;
	}
	if (pe <= ps)
		I_Error("SplitPoly: invalid splitting line (%d %d)", ps, pe);



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



	if (fracs > frace)
	{
		poly_t *swappoly;
		swappoly = *backpoly;
		*backpoly = *frontpoly;
		*frontpoly = swappoly;
	}

	HWR_FreePoly (poly);
}







static poly_t *CutOutSubsecPoly(seg_t *lseg, INT32 count, poly_t *poly)
{
	INT32 i, j;

	polyvertex_t *pv;

	INT32 nump = 0, ps, pe;
	polyvertex_t vs = {0, 0, 0}, ve = {0, 0, 0},
		p1 = {0, 0, 0}, p2 = {0, 0, 0};
	float fracs = 0.0f;

	fdivline_t cutseg;  

	poly_t *temppoly;


	for (; count--; lseg++)
	{
		line_t *line = lseg->linedef;

		if (lseg->glseg)
			continue;


		p1.x = FIXED_TO_FLOAT(lseg->side ? line->v2->x : line->v1->x);
		p1.y = FIXED_TO_FLOAT(lseg->side ? line->v2->y : line->v1->y);
		p2.x = FIXED_TO_FLOAT(lseg->side ? line->v1->x : line->v2->x);
		p2.y = FIXED_TO_FLOAT(lseg->side ? line->v1->y : line->v2->y);

		cutseg.x = p1.x;
		cutseg.y = p1.y;
		cutseg.dx = p2.x - p1.x;
		cutseg.dy = p2.y - p1.y;


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

				break;
			}
		}


		if (ps >= 0)
		{

			if (pe >= 0)
			{

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




			else
				skipcut++;

		}
	}
	return poly;
}






static inline void HWR_SubsecPoly(INT32 num, poly_t *poly)
{
	INT16 count;
	subsector_t *sub;
	seg_t *lseg;

	sub = &subsectors[num];
	count = sub->numlines;
	lseg = &segs[sub->firstline];

	if (poly)
	{
		poly = CutOutSubsecPoly (lseg,count,poly);
		totalsubsecpolys++;

		extrasubsectors[num].planepoly = poly;
	}
}



static inline void SearchDivline(node_t *bsp, fdivline_t *divline)
{
	divline->x = FIXED_TO_FLOAT(bsp->x);
	divline->y = FIXED_TO_FLOAT(bsp->y);
	divline->dx = FIXED_TO_FLOAT(bsp->dx);
	divline->dy = FIXED_TO_FLOAT(bsp->dy);
}

#ifdef HWR_LOADING_SCREEN

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
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);  

	M_DrawTextBox(x-58, y-8, 13, 1);
	V_DrawString(x-50, y, V_YELLOWMAP, "Loading...");
	V_DrawRightAlignedString(x+50, y, V_YELLOWMAP, s);


	V_DrawCenteredString(BASEVIDWIDTH/2, 40, V_YELLOWMAP, "OPENGL MODE IS INCOMPLETE AND MAY");
	V_DrawCenteredString(BASEVIDWIDTH/2, 50, V_YELLOWMAP, "NOT DISPLAY SOME SURFACES.");
	V_DrawCenteredString(BASEVIDWIDTH/2, 70, V_YELLOWMAP, "USE AT SONIC'S RISK.");

	I_UpdateNoVsync();
}
#endif


static void WalkBSPNode(INT32 bspnum, poly_t *poly, UINT16 *leafnode, fixed_t *bbox)
{
	node_t *bsp;
	poly_t *backpoly, *frontpoly;
	fdivline_t fdivline;
	polyvertex_t *pt;
	INT32 i;


	if (bspnum & NF_SUBSECTOR)
	{
		if (bspnum == -1)
		{






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



		}
		else
		{
			HWR_SubsecPoly(bspnum & ~NF_SUBSECTOR, poly);


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


	if (!backpoly)
		nobackpoly++;


	if (frontpoly)
	{
		WalkBSPNode(bsp->children[0], frontpoly, &bsp->children[0],bsp->bbox[0]);


		M_Memcpy(bbox, bsp->bbox[0], 4*sizeof (fixed_t));
	}
	else
		I_Error("WalkBSPNode: no front poly?");


	if (backpoly)
	{

		WalkBSPNode(bsp->children[1], backpoly, &bsp->children[1], bsp->bbox[1]);


		M_AddToBox(bbox, bsp->bbox[1][BOXLEFT  ],
		                 bsp->bbox[1][BOXTOP   ]);
		M_AddToBox(bbox, bsp->bbox[1][BOXRIGHT ],
		                 bsp->bbox[1][BOXBOTTOM]);
	}
}


void HWR_FreeExtraSubsectors(void)
{
	if (extrasubsectors)
		free(extrasubsectors);
	extrasubsectors = NULL;
}

#define MAXDIST 1.5f


static boolean PointInSeg(polyvertex_t *a,polyvertex_t *v1,polyvertex_t *v2)
{
	register float ax,ay,bx,by,cx,cy,d,norm;
	register polyvertex_t *p;


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


	ax= v2->x-v1->x;
	ay= v2->y-v1->y;
	norm = (float)hypot(ax, ay);
	ax /= norm;
	ay /= norm;
	bx = a->x-v1->x;
	by = a->y-v1->y;

	d =ax*bx+ay*by;

	if (d < 0 || d > norm)
		return false;

	cx = ax*d-bx;
	cy = ay*d-by;
#ifdef MOVEVERTEX
	if (cx*cx+cy*cy <= MAXDIST*MAXDIST)
	{

		a->x = ax*d+v1->x;
		a->y = ay*d+v1->y;

		return true;
	}
	return false;
#else
	return cx*cx+cy*cy <= MAXDIST*MAXDIST;
#endif
}

static INT32 numsplitpoly;

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






static INT32 SolveTProblem(void)
{
	poly_t *p;
	INT32 i;
	size_t l;

	if (cv_glsolvetjoin.value == 0)
		return 0;

	CONS_Debug(DBG_RENDER, "Solving T-joins. This may take a while. Please wait...\n");
#ifdef HWR_LOADING_SCREEN
	CON_Drawer();  
	I_FinishUpdate();  
#endif

	numsplitpoly = 0;

	for (l = 0; l < addsubsector; l++)
	{
		p = extrasubsectors[l].planepoly;
		if (p)
			for (i = 0; i < p->numpts; i++)
				SearchSegInBSP((INT32)numnodes-1, &p->pts[i], p);
	}

	return numsplitpoly;
}

#define NEARDIST (0.75f)
#define MYMAX    (10000000000000.0f)

                                                                   
                      
                                                                  
                            
   
static void AdjustSegs(void)
{
	size_t i, count;
	INT32 j;
	seg_t *lseg;
	poly_t *p;
	INT32 v1found = 0, v2found = 0;
	float nearv1, nearv2;

	for (i = 0; i < numsubsectors; i++)
	{
		count = subsectors[i].numlines;
		lseg = &segs[subsectors[i].firstline];
		p = extrasubsectors[i].planepoly;


		for (; count--; lseg++)
		{
			float distv1,distv2,tmp;
			nearv1 = nearv2 = MYMAX;



			if (lseg->polyseg)
				continue;

			if (p) {
				for (j = 0; j < p->numpts; j++)
				{
					distv1 = p->pts[j].x - FIXED_TO_FLOAT(lseg->v1->x);
					tmp    = p->pts[j].y - FIXED_TO_FLOAT(lseg->v1->y);
					distv1 = distv1*distv1+tmp*tmp;
					if (distv1 <= nearv1)
					{
						v1found = j;
						nearv1 = distv1;
					}

					distv2 = p->pts[j].x - FIXED_TO_FLOAT(lseg->v2->x);
					tmp    = p->pts[j].y - FIXED_TO_FLOAT(lseg->v2->y);
					distv2 = distv2*distv2+tmp*tmp;
					if (distv2 <= nearv2)
					{
						v2found = j;
						nearv2 = distv2;
					}
				}
			}
			if (p && nearv1 <= NEARDIST*NEARDIST)

				lseg->pv1 = &(p->pts[v1found]);
			else
			{





				polyvertex_t *pv = HWR_AllocVertex();
				pv->x = FIXED_TO_FLOAT(lseg->v1->x);
				pv->y = FIXED_TO_FLOAT(lseg->v1->y);
				lseg->pv1 = pv;
			}
			if (p && nearv2 <= NEARDIST*NEARDIST)
				lseg->pv2 = &(p->pts[v2found]);
			else
			{
				polyvertex_t *pv = HWR_AllocVertex();
				pv->x = FIXED_TO_FLOAT(lseg->v2->x);
				pv->y = FIXED_TO_FLOAT(lseg->v2->y);
				lseg->pv2 = pv;
			}

			                                                                      
                                                                          
                                                                          
                                                                  
			{
				float x,y;
				x = ((polyvertex_t *)lseg->pv2)->x - ((polyvertex_t *)lseg->pv1)->x;
				y = ((polyvertex_t *)lseg->pv2)->y - ((polyvertex_t *)lseg->pv1)->y;
				lseg->flength = (float)hypot(x, y);



			}
		}
	}
}




void HWR_CreatePlanePolygons(INT32 bspnum)
{
	poly_t *rootp;
	polyvertex_t *rootpv;
	size_t i;
	fixed_t rootbbox[4];

	CONS_Debug(DBG_RENDER, "Creating polygons, please wait...\n");
#ifdef HWR_LOADING_SCREEN
	ls_count = ls_percent = 0;  
	CON_Drawer();  
	I_FinishUpdate();  
#endif

	HWR_ClearPolys();



	M_ClearBox(rootbbox);
	for (i = 0;i < numvertexes; i++)
		M_AddToBox(rootbbox, vertexes[i].x, vertexes[i].y);



	HWR_FreeExtraSubsectors();

	totsubsectors = numsubsectors + NEWSUBSECTORS;
	extrasubsectors = calloc(totsubsectors, sizeof (*extrasubsectors));
	if (extrasubsectors == NULL)
		I_Error("couldn't malloc extrasubsectors totsubsectors %s\n", sizeu1(totsubsectors));


	                                                                                   
                        
                                                   


	addsubsector = numsubsectors;


	rootp = HWR_AllocPoly(4);
	rootpv = rootp->pts;

	rootpv->x = FIXED_TO_FLOAT(rootbbox[BOXLEFT  ]);
	rootpv->y = FIXED_TO_FLOAT(rootbbox[BOXBOTTOM]);   
	rootpv++;
	rootpv->x = FIXED_TO_FLOAT(rootbbox[BOXLEFT  ]);
	rootpv->y = FIXED_TO_FLOAT(rootbbox[BOXTOP   ]);   
	rootpv++;
	rootpv->x = FIXED_TO_FLOAT(rootbbox[BOXRIGHT ]);
	rootpv->y = FIXED_TO_FLOAT(rootbbox[BOXTOP   ]);   
	rootpv++;
	rootpv->x = FIXED_TO_FLOAT(rootbbox[BOXRIGHT ]);
	rootpv->y = FIXED_TO_FLOAT(rootbbox[BOXBOTTOM]);   
	rootpv++;

	WalkBSPNode(bspnum, rootp, NULL,rootbbox);

	i = SolveTProblem();

	AdjustSegs();









}

#endif  
