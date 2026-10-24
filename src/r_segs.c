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
#include "r_local.h"
#include "r_sky.h"

#include "r_portal.h"

#include "w_wad.h"
#include "z_zone.h"
#include "p_slopes.h"
#include "taglist.h"




static boolean segtextured;
static boolean markfloor;  
static boolean markceiling;

static boolean maskedtexture;
static INT32 toptexture, bottomtexture, midtexture;
static INT32 numthicksides, numbackffloors;

angle_t rw_normalangle;

angle_t rw_angle1;
fixed_t rw_distance;




static INT32 rw_x, rw_stopx;
static angle_t rw_centerangle;
static fixed_t rw_offset, rw_offsetx;
static fixed_t rw_offset_top, rw_offset_mid, rw_offset_bottom;
static fixed_t rw_scale, rw_scalestep;
static fixed_t rw_midtexturemid, rw_toptexturemid, rw_bottomtexturemid;
static INT32 worldtop, worldbottom, worldhigh, worldlow;
static INT32 worldtopslope, worldbottomslope, worldhighslope, worldlowslope;  
static fixed_t rw_toptextureslide, rw_midtextureslide, rw_bottomtextureslide;  
static fixed_t rw_midtextureback, rw_midtexturebackslide;  
static fixed_t rw_midtexturescalex, rw_midtexturescaley;
static fixed_t rw_toptexturescalex, rw_toptexturescaley;
static fixed_t rw_bottomtexturescalex, rw_bottomtexturescaley;
static fixed_t rw_invmidtexturescalex, rw_invtoptexturescalex, rw_invbottomtexturescalex;


static boolean rw_floormarked = false;
static boolean rw_ceilingmarked = false;

static INT32 *rw_silhouette = NULL;
static fixed_t *rw_tsilheight = NULL;
static fixed_t *rw_bsilheight = NULL;

static fixed_t pixhigh, pixlow, pixhighstep, pixlowstep;
static fixed_t topfrac, topstep;
static fixed_t bottomfrac, bottomstep;

static lighttable_t **walllights;
static fixed_t *maskedtexturecol = NULL;
static fixed_t *maskedtextureheight = NULL;
                                                                             
                                                                            
                                      
static INT16 maskedopeningtop[MAXVIDWIDTH];
static INT16 maskedopeningbottom[MAXVIDWIDTH];

                                                                            
                                                                          
                                                                            
                                            
static fixed_t R_InterpolatedFineSine(angle_t angle)
{
	const UINT32 index = angle >> ANGLETOFINESHIFT;
	const UINT32 nextindex = (index + 1U) & FINEMASK;
	const UINT32 fraction = angle & ((1U << ANGLETOFINESHIFT) - 1U);
	const fixed_t first = FINESINE(index);
	const fixed_t second = FINESINE(nextindex);

	return first + (fixed_t)(((INT64)second - first) * fraction
		>> ANGLETOFINESHIFT);
}

static fixed_t *thicksidecol = NULL;
static fixed_t *invscale = NULL;

static boolean texcoltables;


static size_t numopenings;
static INT16 *openings, *lastopening;

static size_t texturecolumntablesize;
static fixed_t *texturecolumntable, *curtexturecolumntable;

void R_ClearSegTables(void)
{
	lastopening = openings;
	curtexturecolumntable = texturecolumntable;
}

transnum_t R_GetLinedefTransTable(fixed_t alpha)
{
	return (20*(FRACUNIT - alpha - 1) + FRACUNIT) >> (FRACBITS+1);
}

static UINT8 R_SideLightLevel(side_t *side, INT16 base_lightlevel)
{
	return max(0, min(255, side->light +
		((side->lightabsolute) ? 0 : base_lightlevel)));
}

                                       
                                                                 
 
                                         
                                                                              
 

                                                                 
 
                                         
                                                                              
 

                                                                    
 
                                            
                                                                                 
 
  






static void R_Render2sidedMultiPatchColumn(column_t *column, unsigned lengthcol)
{
	INT32 topscreen, bottomscreen;

	post_t *post = &column->posts[0];
	if (!post->length)
		return;

	topscreen = sprtopscreen;
	bottomscreen = topscreen + spryscale * lengthcol;

	dc_yl = (sprtopscreen+FRACUNIT-1)>>FRACBITS;
	dc_yh = (bottomscreen-1)>>FRACBITS;

	if (windowtop != INT32_MAX && windowbottom != INT32_MAX)
	{
		dc_yl = ((windowtop + FRACUNIT)>>FRACBITS);
		dc_yh = (windowbottom - 1)>>FRACBITS;
	}

	if (dc_yh >= mfloorclip[dc_x])
		dc_yh =  mfloorclip[dc_x] - 1;
	if (dc_yl <= mceilingclip[dc_x])
		dc_yl =  mceilingclip[dc_x] + 1;

	if (dc_yl >= vid.height || dc_yh < 0)
		return;

	if (dc_yl <= dc_yh && dc_yh < vid.height && dc_yh > 0)
	{
		dc_source = column->pixels + post->data_offset;
		dc_postlength = post->length;

		if (colfunc == colfuncs[BASEDRAWFUNC])
			(colfuncs[COLDRAWFUNC_TWOSMULTIPATCH])();
		else if (colfunc == colfuncs[COLDRAWFUNC_FUZZY])
			(colfuncs[COLDRAWFUNC_TWOSMULTIPATCHTRANS])();
		else
			colfunc();
	}
}

static void R_RenderFlipped2sidedMultiPatchColumn(column_t *column, unsigned lengthcol)
{
	INT32 topscreen, bottomscreen;

	void (*localcolfunc)(void);

	post_t *post = &column->posts[0];
	if (!post->length)
		return;

	topscreen = sprtopscreen;
	bottomscreen = topscreen + spryscale * lengthcol;

	dc_yl = (sprtopscreen+FRACUNIT-1)>>FRACBITS;
	dc_yh = (bottomscreen-1)>>FRACBITS;

	if (windowtop != INT32_MAX && windowbottom != INT32_MAX)
	{
		dc_yl = ((windowtop + FRACUNIT)>>FRACBITS);
		dc_yh = (windowbottom - 1)>>FRACBITS;
	}

	if (dc_yh >= mfloorclip[dc_x])
		dc_yh =  mfloorclip[dc_x] - 1;
	if (dc_yl <= mceilingclip[dc_x])
		dc_yl =  mceilingclip[dc_x] + 1;

	if (dc_yl >= vid.height || dc_yh < 0)
		return;

	if (dc_yl <= dc_yh && dc_yh < vid.height && dc_yh > 0)
	{
		dc_postlength = post->length;

		if (colfunc == colfuncs[BASEDRAWFUNC])
			localcolfunc = colfuncs[COLDRAWFUNC_TWOSMULTIPATCH];
		else if (colfunc == colfuncs[COLDRAWFUNC_FUZZY])
			localcolfunc = colfuncs[COLDRAWFUNC_TWOSMULTIPATCHTRANS];
		else
			localcolfunc = colfunc;

		R_DrawFlippedPost(column->pixels + post->data_offset, post->length, localcolfunc);
	}
}

void R_RenderMaskedSegRange(drawseg_t *ds, INT32 x1, INT32 x2)
{
	size_t pindex;
	column_t *col;
	INT32 lightnum, texnum, i;
	fixed_t height, realbot;
	lightlist_t *light;
	r_lightlist_t *rlight;
	void (*colfunc_2s)(column_t *, unsigned);
	line_t *ldef;
	sector_t *front, *back;
	INT32 times, repeats;
	INT64 overflow_test;
	INT32 range;
	UINT8 vertflip;
	unsigned lengthcol;
	fixed_t wall_scaley;
	fixed_t scalestep;
	fixed_t scale1;





	curline = ds->curline;

	frontsector = curline->frontsector;
	backsector = curline->backsector;
	sidedef = curline->sidedef;
	texnum = R_GetTextureNum(sidedef->midtexture);
	windowbottom = windowtop = sprbotscreen = INT32_MAX;

	ldef = curline->linedef;
	if (!ldef->alpha)
		return;

	if (ldef->blendmode == AST_FOG)
	{
		colfunc = colfuncs[COLDRAWFUNC_FOG];
		windowtop = frontsector->ceilingheight;
		windowbottom = frontsector->floorheight;
	}
	else if (ldef->blendmode)
	{
		if (ldef->alpha == NUMTRANSMAPS || ldef->blendmode == AST_MODULATE)
			dc_transmap = R_GetBlendTable(ldef->blendmode, 0);
		else
			dc_transmap = R_GetBlendTable(ldef->blendmode, R_GetLinedefTransTable(ldef->alpha));
		colfunc = colfuncs[COLDRAWFUNC_FUZZY];
	}
	else if (ldef->alpha > 0 && ldef->alpha < FRACUNIT)
	{
		dc_transmap = R_GetTranslucencyTable(R_GetLinedefTransTable(ldef->alpha));
		colfunc = colfuncs[COLDRAWFUNC_FUZZY];
	}
	else
		colfunc = colfuncs[BASEDRAWFUNC];

	if (curline->polyseg && curline->polyseg->translucency > 0)
	{
		if (curline->polyseg->translucency >= NUMTRANSMAPS)
			return;

		dc_transmap = R_GetTranslucencyTable(curline->polyseg->translucency);
		colfunc = colfuncs[COLDRAWFUNC_FUZZY];
	}

	vertflip = textures[texnum]->flip & 2;

	wall_scaley = sidedef->scaley_mid;
	if (wall_scaley < 0)
	{
		wall_scaley = -wall_scaley;
		vertflip = !vertflip;
	}

	scalestep = FixedDiv(ds->scalestep, wall_scaley);
	scale1 = FixedDiv(ds->scale1, wall_scaley);

	range = max(ds->x2-ds->x1, 1);
	rw_scalestep = scalestep;
	spryscale = scale1 + (x1 - ds->x1)*rw_scalestep;


	R_CheckTextureCache(texnum);



	if (!textures[texnum]->transparency)
	{
		if (vertflip)  
			colfunc_2s = R_RenderFlipped2sidedMultiPatchColumn;
		else
			colfunc_2s = R_Render2sidedMultiPatchColumn;
	}
	else if (vertflip)  
		colfunc_2s = R_DrawFlippedMaskedColumn;
	else
		colfunc_2s = R_DrawMaskedColumn;  

	lengthcol = textures[texnum]->height;


	dc_numlights = 0;
	if (frontsector->numlights)
	{
		dc_numlights = frontsector->numlights;
		if (dc_numlights > dc_maxlights)
		{
			dc_maxlights = dc_numlights;
			dc_lightlist = Z_Realloc(dc_lightlist, sizeof (*dc_lightlist) * dc_maxlights, PU_STATIC, NULL);
		}

		for (i = 0; i < dc_numlights; i++)
		{
			fixed_t leftheight, rightheight;
			light = &frontsector->lightlist[i];
			rlight = &dc_lightlist[i];
			leftheight  = P_GetLightZAt(light, ds-> leftpos.x, ds-> leftpos.y);
			rightheight = P_GetLightZAt(light, ds->rightpos.x, ds->rightpos.y);

			leftheight  -= viewz;
			rightheight -= viewz;

			rlight->height     = (centeryfrac) - FixedMul(leftheight , ds->scale1);
			rlight->heightstep = (centeryfrac) - FixedMul(rightheight, ds->scale2);
			rlight->heightstep = (rlight->heightstep-rlight->height)/(range);


			rlight->startheight = rlight->height;  
			rlight->lightlevel = *light->lightlevel;
			rlight->extra_colormap = *light->extra_colormap;
			rlight->flags = light->flags;

			if ((colfunc != colfuncs[COLDRAWFUNC_FUZZY])
				|| (rlight->flags & FOF_FOG)
				|| (rlight->extra_colormap && (rlight->extra_colormap->flags & CMF_FOG)))
				lightnum = R_SideLightLevel(curline->sidedef, rlight->lightlevel) >> LIGHTSEGSHIFT;
			else
				lightnum = LIGHTLEVELS - 1;

			if (rlight->extra_colormap && (rlight->extra_colormap->flags & CMF_FOG))
				;
			else if (curline->v1->y == curline->v2->y)
				lightnum--;
			else if (curline->v1->x == curline->v2->x)
				lightnum++;

			rlight->lightnum = lightnum;
		}
	}
	else
	{
		if ((colfunc != colfuncs[COLDRAWFUNC_FUZZY])
			|| (frontsector->extra_colormap && (frontsector->extra_colormap->flags & CMF_FOG)))
			lightnum = R_SideLightLevel(curline->sidedef, frontsector->lightlevel) >> LIGHTSEGSHIFT;
		else
			lightnum = LIGHTLEVELS - 1;

		if (colfunc == colfuncs[COLDRAWFUNC_FOG]
			|| (frontsector->extra_colormap && (frontsector->extra_colormap->flags & CMF_FOG)))
			;
		else if (curline->v1->y == curline->v2->y)
			lightnum--;
		else if (curline->v1->x == curline->v2->x)
			lightnum++;

		if (lightnum < 0)
			walllights = scalelight[0];
		else if (lightnum >= LIGHTLEVELS)
			walllights = scalelight[LIGHTLEVELS - 1];
		else
			walllights = scalelight[lightnum];
	}

	maskedtexturecol = ds->maskedtexturecol;

	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	if (frontsector->heightsec != -1)
		front = &sectors[frontsector->heightsec];
	else
		front = frontsector;

	if (backsector->heightsec != -1)
		back = &sectors[backsector->heightsec];
	else
		back = backsector;

	                                                                       
                                                                        
                                                                           
	if (udmf && !sidedef->repeatcnt && (ldef->flags & ML_WRAPMIDTEX))
	{
		sector_t *clipfront = frontsector;
		sector_t *clipback = backsector;
		fixed_t lefttop, righttop, leftbottom, rightbottom;
		INT64 lefttopscreen, righttopscreen;
		INT64 leftbottomscreen, rightbottomscreen;
		const INT32 cliprange = max(ds->x2 - ds->x1, 1);

		if (curline->polyseg)
			clipfront = clipback = back;

		lefttop = min(P_GetZAt(clipfront->c_slope, ds->leftpos.x,
			ds->leftpos.y, clipfront->ceilingheight),
			P_GetZAt(clipback->c_slope, ds->leftpos.x,
			ds->leftpos.y, clipback->ceilingheight)) - viewz;
		righttop = min(P_GetZAt(clipfront->c_slope, ds->rightpos.x,
			ds->rightpos.y, clipfront->ceilingheight),
			P_GetZAt(clipback->c_slope, ds->rightpos.x,
			ds->rightpos.y, clipback->ceilingheight)) - viewz;
		leftbottom = max(P_GetZAt(clipfront->f_slope, ds->leftpos.x,
			ds->leftpos.y, clipfront->floorheight),
			P_GetZAt(clipback->f_slope, ds->leftpos.x,
			ds->leftpos.y, clipback->floorheight)) - viewz;
		rightbottom = max(P_GetZAt(clipfront->f_slope, ds->rightpos.x,
			ds->rightpos.y, clipfront->floorheight),
			P_GetZAt(clipback->f_slope, ds->rightpos.x,
			ds->rightpos.y, clipback->floorheight)) - viewz;

		lefttopscreen = (INT64)centeryfrac
			- (((INT64)lefttop * ds->scale1) >> FRACBITS);
		righttopscreen = (INT64)centeryfrac
			- (((INT64)righttop * ds->scale2) >> FRACBITS);
		leftbottomscreen = (INT64)centeryfrac
			- (((INT64)leftbottom * ds->scale1) >> FRACBITS);
		rightbottomscreen = (INT64)centeryfrac
			- (((INT64)rightbottom * ds->scale2) >> FRACBITS);

		for (i = x1; i <= x2; i++)
		{
			const INT32 offset = i - ds->x1;
			const INT64 topscreen = lefttopscreen
				+ ((righttopscreen - lefttopscreen) * offset) / cliprange;
			const INT64 bottomscreen = leftbottomscreen
				+ ((rightbottomscreen - leftbottomscreen) * offset) / cliprange;
			INT64 topclip = ((topscreen + FRACUNIT - 1) >> FRACBITS) - 1;
			INT64 bottomclip = (bottomscreen + FRACUNIT - 1) >> FRACBITS;

			if (topclip < -1)
				topclip = -1;
			else if (topclip > viewheight)
				topclip = viewheight;
			if (bottomclip < -1)
				bottomclip = -1;
			else if (bottomclip > viewheight)
				bottomclip = viewheight;

			maskedopeningtop[i] = max(ds->sprtopclip[i], (INT16)topclip);
			maskedopeningbottom[i] = min(ds->sprbottomclip[i], (INT16)bottomclip);
		}

		mceilingclip = maskedopeningtop;
		mfloorclip = maskedopeningbottom;
	}

	if (sidedef->repeatcnt)
		repeats = 1 + sidedef->repeatcnt;
	else if (ldef->flags & ML_WRAPMIDTEX)
	{
		fixed_t high, low;

		if (front->ceilingheight > back->ceilingheight)
			high = back->ceilingheight;
		else
			high = front->ceilingheight;

		if (front->floorheight > back->floorheight)
			low = front->floorheight;
		else
			low = back->floorheight;

		if (udmf)
		{
			const fixed_t textureoffset = sidedef->rowoffset + sidedef->offsety_mid;
			const fixed_t tileheight = FixedDiv(textureheight[texnum], wall_scaley);
			fixed_t filloffset = 0;

			repeats = (high - low) / tileheight;
			if ((high - low) % tileheight)
				repeats++;  

			                                                                     
                                                                         
                                                                       
                                                                        
                                                                        
                                                             
			if (ldef->flags & ML_MIDPEG)
			{
				if (textureoffset < 0)
					filloffset = -textureoffset;
			}
			else if (textureoffset > 0)
				filloffset = textureoffset;

			if (filloffset)
			{
				repeats += filloffset / textureheight[texnum];
				if (filloffset % textureheight[texnum])
					repeats++;
			}
		}
		else
		{
			                                                                   
                                                                         
                                                                   
			repeats = (high - low)/textureheight[texnum];
			if ((high-low)%textureheight[texnum])
				repeats++;
		}
	}
	else
		repeats = 1;

	for (times = 0; times < repeats; times++)
	{
		if (times > 0)
		{
			rw_scalestep = scalestep;
			spryscale = scale1 + (x1 - ds->x1)*rw_scalestep;


			for (i = 0; i < dc_numlights; i++)
			{
				rlight = &dc_lightlist[i];
				rlight->height = rlight->startheight;
			}
		}

		dc_texheight = textureheight[texnum]>>FRACBITS;


		for (dc_x = x1; dc_x <= x2; dc_x++)
		{
			dc_texturemid = ds->maskedtextureheight[dc_x];

			if (curline->linedef->flags & ML_MIDPEG)
				dc_texturemid += (textureheight[texnum])*times + textureheight[texnum];
			else
				dc_texturemid -= (textureheight[texnum])*times;


			overflow_test = (INT64)centeryfrac - (((INT64)dc_texturemid*spryscale)>>FRACBITS);
			if (overflow_test < 0) overflow_test = -overflow_test;
			if ((UINT64)overflow_test&0xFFFFFFFF80000000ULL)
			{

				if (dc_numlights)
				{
					for (i = 0; i < dc_numlights; i++)
					{
						rlight = &dc_lightlist[i];
						rlight->height += rlight->heightstep;
					}
				}
				spryscale += rw_scalestep;
				continue;
			}


			if (dc_numlights)
			{
				lighttable_t **xwalllights;

				sprtopscreen = windowtop = (centeryfrac - FixedMul(dc_texturemid, spryscale));

				realbot = FixedMul(textureheight[texnum], spryscale) + sprtopscreen;
				dc_iscale = FixedMul(ds->invscale[dc_x], wall_scaley);

				windowbottom = realbot;


				col = R_GetColumn(texnum, maskedtexturecol[dc_x] >> FRACBITS);

				for (i = 0; i < dc_numlights; i++)
				{
					rlight = &dc_lightlist[i];

					if ((rlight->flags & FOF_NOSHADE))
						continue;

					if (rlight->lightnum < 0)
						xwalllights = scalelight[0];
					else if (rlight->lightnum >= LIGHTLEVELS)
						xwalllights = scalelight[LIGHTLEVELS-1];
					else
						xwalllights = scalelight[rlight->lightnum];

					pindex = FixedMul(spryscale, LIGHTRESOLUTIONFIX)>>LIGHTSCALESHIFT;

					if (pindex >= MAXLIGHTSCALE)
						pindex = MAXLIGHTSCALE - 1;

					if (rlight->extra_colormap)
						rlight->rcolormap = rlight->extra_colormap->colormap + (xwalllights[pindex] - colormaps);
					else
						rlight->rcolormap = xwalllights[pindex];

					height = rlight->height;
					rlight->height += rlight->heightstep;

					if (height <= windowtop)
					{
						dc_colormap = rlight->rcolormap;
						continue;
					}

					windowbottom = height;
					if (windowbottom >= realbot)
					{
						windowbottom = realbot;
						colfunc_2s(col, lengthcol);
						for (i++; i < dc_numlights; i++)
						{
							rlight = &dc_lightlist[i];
							rlight->height += rlight->heightstep;
						}

						continue;
					}
					colfunc_2s(col, lengthcol);
					windowtop = windowbottom + 1;
					dc_colormap = rlight->rcolormap;
				}
				windowbottom = realbot;
				if (windowtop < windowbottom)
					colfunc_2s(col, lengthcol);

				spryscale += rw_scalestep;
				continue;
			}


			pindex = FixedMul(spryscale, LIGHTRESOLUTIONFIX)>>LIGHTSCALESHIFT;

			if (pindex >= MAXLIGHTSCALE)
				pindex = MAXLIGHTSCALE - 1;

			dc_colormap = walllights[pindex];

			if (frontsector->extra_colormap)
				dc_colormap = frontsector->extra_colormap->colormap + (dc_colormap - colormaps);

			sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
			dc_iscale = FixedMul(ds->invscale[dc_x], wall_scaley);


			col = R_GetColumn(texnum, maskedtexturecol[dc_x] >> FRACBITS);
			colfunc_2s(col, lengthcol);

			spryscale += rw_scalestep;
		}
	}
	colfunc = colfuncs[BASEDRAWFUNC];
}


static fixed_t repeatscroll = 0;

static void R_DrawRepeatMaskedColumn(column_t *col, unsigned lengthcol)
{
	fixed_t topscreen = sprtopscreen;
	fixed_t bottomscreen = sprbotscreen;

	fixed_t texheight = dc_texheight*spryscale;

	fixed_t scroll = -repeatscroll;
	if (scroll < 0)
	{
		scroll = -FixedMul((abs(scroll) % (dc_texheight*FRACUNIT)), spryscale);
		bottomscreen += texheight;  
	}
	else if (scroll)
	{
		scroll = FixedMul(scroll % (dc_texheight*FRACUNIT), spryscale);
		topscreen -= texheight;  
	}

	while (topscreen < bottomscreen)
	{
		sprtopscreen = topscreen + scroll;

		R_DrawMaskedColumn(col, lengthcol);

		if ((INT64)sprtopscreen + (INT64)dc_texheight*spryscale > (INT64)INT32_MAX)  
			break;

		topscreen += texheight;
	}

	sprtopscreen = sprbotscreen;
}

static void R_DrawRepeatFlippedMaskedColumn(column_t *col, unsigned lengthcol)
{
	fixed_t topscreen = sprtopscreen;
	fixed_t bottomscreen = sprbotscreen;

	fixed_t texheight = dc_texheight*spryscale;

	fixed_t scroll = -repeatscroll;
	if (scroll < 0)
	{
		scroll = -FixedMul((abs(scroll) % (dc_texheight*FRACUNIT)), spryscale);
		bottomscreen += texheight;  
	}
	else if (scroll)
	{
		scroll = FixedMul(scroll % (dc_texheight*FRACUNIT), spryscale);
		topscreen -= texheight;  
	}

	while (topscreen < bottomscreen)
	{
		sprtopscreen = topscreen + scroll;
		sprbotscreen = bottomscreen + scroll;

		R_DrawFlippedMaskedColumn(col, lengthcol);

		if ((INT64)sprtopscreen + (INT64)dc_texheight*spryscale > (INT64)INT32_MAX)  
			break;

		topscreen += texheight;
	}

	sprtopscreen = sprbotscreen;
}


static boolean R_IsFFloorTranslucent(visffloor_t *pfloor)
{
	if (pfloor->polyobj)
		return (pfloor->polyobj->translucency > 0);


	if (pfloor->ffloor != NULL)
		return (pfloor->ffloor->fofflags & (FOF_TRANSLUCENT|FOF_FOG));

	return false;
}

static fixed_t R_GetSlopeTextureSlide(pslope_t *slope, angle_t lineangle)
{
	return FixedMul(slope->zdelta, FINECOSINE((lineangle-slope->xydirection)>>ANGLETOFINESHIFT));
}





static fixed_t ffloortexturecolumn[MAXVIDWIDTH];

void R_RenderThickSideRange(drawseg_t *ds, INT32 x1, INT32 x2, ffloor_t *pfloor)
{
	size_t          pindex;
	column_t *      col;
	INT32             lightnum;
	INT32            texnum;
	sector_t        tempsec;
	INT32             templight;
	INT32             i, p;
	lightlist_t     *light;
	r_lightlist_t   *rlight;
	INT32           range;


	INT64         top_frac, top_step, bottom_frac, bottom_step;

	fixed_t       ffloortextureslide = 0;
	fixed_t       oldtexturecolumn = -1;
	fixed_t       left_top, left_bottom;  
	pslope_t      *skewslope = NULL;
	boolean do_texture_skew;
	boolean dont_peg_bottom;
	fixed_t wall_offsetx;
	fixed_t wall_scalex, wall_scaley;
	UINT8 vertflip;
	unsigned lengthcol;
	boolean fog = false;
	boolean fuzzy = false;

	void (*colfunc_2s) (column_t *, unsigned);






	curline = ds->curline;
	backsector = pfloor->target;
	frontsector = curline->frontsector == pfloor->target ? curline->backsector : curline->frontsector;
	sidedef = R_GetFFloorSide(curline->linedef, pfloor, pfloor->target);

	if (pfloor->master->flags & ML_TFERLINE)
	{
		line_t *newline = R_GetFFloorLine(curline->linedef, pfloor, pfloor->target);
		do_texture_skew = newline->flags & ML_SKEWTD;
		dont_peg_bottom = newline->flags & ML_DONTPEGBOTTOM;
	}
	else
	{
		do_texture_skew = pfloor->master->flags & ML_SKEWTD;
		dont_peg_bottom = curline->linedef->flags & ML_DONTPEGBOTTOM;
	}

	texnum = R_GetTextureNum(sidedef->midtexture);
	vertflip = textures[texnum]->flip & 2;

	if (pfloor->fofflags & FOF_TRANSLUCENT)
	{
		fuzzy = true;



		{
			INT32 trans = (10*((256+12) - pfloor->alpha))/255;
			if (trans >= 10)
				return;  
			if (pfloor->blend)  
				dc_transmap = R_GetBlendTable(pfloor->blend, trans);
			else if (!(dc_transmap = R_GetTranslucencyTable(trans)) || trans == 0)
				fuzzy = false;  
		}
	}
	else if (pfloor->fofflags & FOF_FOG)
	{
		colfunc = colfuncs[COLDRAWFUNC_FOG];
		fog = true;
	}

	range = max(ds->x2-ds->x1, 1);

	dc_numlights = 0;
	if (frontsector->numlights)
	{
		dc_numlights = frontsector->numlights;
		if (dc_numlights > dc_maxlights)
		{
			dc_maxlights = dc_numlights;
			dc_lightlist = Z_Realloc(dc_lightlist, sizeof (*dc_lightlist) * dc_maxlights, PU_STATIC, NULL);
		}

		for (i = p = 0; i < dc_numlights; i++)
		{
			fixed_t leftheight, rightheight;
			fixed_t pfloorleft, pfloorright;
			INT64 overflow_test;
			light = &frontsector->lightlist[i];
			rlight = &dc_lightlist[p];

#define SLOPEPARAMS(slope, end1, end2, normalheight) \
	end1 = P_GetZAt(slope, ds-> leftpos.x, ds-> leftpos.y, normalheight); \
	end2 = P_GetZAt(slope, ds->rightpos.x, ds->rightpos.y, normalheight);

			SLOPEPARAMS(light->slope,     leftheight, rightheight, light->height)
			SLOPEPARAMS(*pfloor->b_slope, pfloorleft, pfloorright, *pfloor->bottomheight)

			if (leftheight < pfloorleft && rightheight < pfloorright)
				continue;

			SLOPEPARAMS(*pfloor->t_slope, pfloorleft, pfloorright, *pfloor->topheight)

			if (leftheight > pfloorleft && rightheight > pfloorright && i+1 < dc_numlights)
			{
				lightlist_t *nextlight = &frontsector->lightlist[i+1];
				if (P_GetZAt(nextlight->slope, ds-> leftpos.x, ds-> leftpos.y, nextlight->height) > pfloorleft
				 && P_GetZAt(nextlight->slope, ds->rightpos.x, ds->rightpos.y, nextlight->height) > pfloorright)
					continue;
			}

			leftheight -= viewz;
			rightheight -= viewz;

#define CLAMPMAX INT32_MAX
#define CLAMPMIN (-INT32_MAX)  

			overflow_test = (INT64)centeryfrac - (((INT64)leftheight*ds->scale1)>>FRACBITS);
			if      (overflow_test > (INT64)CLAMPMAX) rlight->height = CLAMPMAX;
			else if (overflow_test > (INT64)CLAMPMIN) rlight->height = (fixed_t)overflow_test;
			else                                      rlight->height = CLAMPMIN;

			overflow_test = (INT64)centeryfrac - (((INT64)rightheight*ds->scale2)>>FRACBITS);
			if      (overflow_test > (INT64)CLAMPMAX) rlight->heightstep = CLAMPMAX;
			else if (overflow_test > (INT64)CLAMPMIN) rlight->heightstep = (fixed_t)overflow_test;
			else                                      rlight->heightstep = CLAMPMIN;
			rlight->heightstep = (rlight->heightstep-rlight->height)/(range);
			rlight->flags = light->flags;
			if (light->flags & FOF_CUTLEVEL)
			{
				SLOPEPARAMS(*light->caster->b_slope, leftheight, rightheight, *light->caster->bottomheight)
#undef SLOPEPARAMS
				leftheight -= viewz;
				rightheight -= viewz;


				overflow_test = (INT64)centeryfrac - (((INT64)leftheight*ds->scale1)>>FRACBITS);
				if      (overflow_test > (INT64)CLAMPMAX) rlight->botheight = CLAMPMAX;
				else if (overflow_test > (INT64)CLAMPMIN) rlight->botheight = (fixed_t)overflow_test;
				else                                      rlight->botheight = CLAMPMIN;

				overflow_test = (INT64)centeryfrac - (((INT64)rightheight*ds->scale2)>>FRACBITS);
				if      (overflow_test > (INT64)CLAMPMAX) rlight->botheightstep = CLAMPMAX;
				else if (overflow_test > (INT64)CLAMPMIN) rlight->botheightstep = (fixed_t)overflow_test;
				else                                      rlight->botheightstep = CLAMPMIN;
				rlight->botheightstep = (rlight->botheightstep-rlight->botheight)/(range);
			}

			rlight->lightlevel = *light->lightlevel;
			rlight->extra_colormap = *light->extra_colormap;


			if (pfloor->fofflags & FOF_FOG)
				rlight->lightnum = R_SideLightLevel(curline->sidedef, pfloor->master->frontsector->lightlevel) >> LIGHTSEGSHIFT;
			else
				rlight->lightnum = R_SideLightLevel(curline->sidedef, rlight->lightlevel) >> LIGHTSEGSHIFT;

			if (pfloor->fofflags & FOF_FOG || rlight->flags & FOF_FOG || (rlight->extra_colormap && (rlight->extra_colormap->flags & CMF_FOG)))
				;
			else if (curline->v1->y == curline->v2->y)
				rlight->lightnum--;
			else if (curline->v1->x == curline->v2->x)
				rlight->lightnum++;

			p++;
		}

		dc_numlights = p;
	}
	else
	{

		if ((frontsector->extra_colormap && (frontsector->extra_colormap->flags & CMF_FOG)))
			lightnum = R_SideLightLevel(curline->sidedef, frontsector->lightlevel) >> LIGHTSEGSHIFT;
		else if (fog)
			lightnum = R_SideLightLevel(curline->sidedef, pfloor->master->frontsector->lightlevel) >> LIGHTSEGSHIFT;
		else if (fuzzy)
			lightnum = LIGHTLEVELS-1;
		else
			lightnum = R_SideLightLevel(curline->sidedef, R_FakeFlat(frontsector, &tempsec, &templight, &templight, false)->lightlevel) >> LIGHTSEGSHIFT;

		if (pfloor->fofflags & FOF_FOG || (frontsector->extra_colormap && (frontsector->extra_colormap->flags & CMF_FOG)));
			else if (curline->v1->y == curline->v2->y)
			lightnum--;
		else if (curline->v1->x == curline->v2->x)
			lightnum++;

		if (lightnum < 0)
			walllights = scalelight[0];
		else if (lightnum >= LIGHTLEVELS)
			walllights = scalelight[LIGHTLEVELS-1];
		else
			walllights = scalelight[lightnum];
	}

	wall_scalex = FixedDiv(FRACUNIT, sidedef->scalex_mid);
	wall_scaley = sidedef->scaley_mid;
	if (wall_scaley < 0)
	{
		wall_scaley = -wall_scaley;
		vertflip = !vertflip;
	}





	double dwall_scaley = FixedToDouble(wall_scaley);
	double scalestep = FixedToDouble(ds->scalestep) / dwall_scaley;
	double yscale = (FixedToDouble(ds->scale1) + (x1 - ds->x1)*scalestep) / dwall_scaley;

	thicksidecol = ffloortexturecolumn;

	wall_offsetx = ds->offsetx + sidedef->offsetx_mid;

	if (wall_scalex == FRACUNIT)
	{
		for (INT32 x = x1; x <= x2; x++)
			thicksidecol[x] = ds->thicksidecol[x];
	}
	else
	{
		for (INT32 x = x1; x <= x2; x++)
			thicksidecol[x] = FixedDiv(ds->thicksidecol[x], wall_scalex);
	}

	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;
	dc_texheight = textureheight[texnum]>>FRACBITS;


	left_top    = P_GetFFloorTopZAt   (pfloor, ds->leftpos.x, ds->leftpos.y) - viewz;
	left_bottom = P_GetFFloorBottomZAt(pfloor, ds->leftpos.x, ds->leftpos.y) - viewz;

	if (do_texture_skew)
	{
		skewslope = *pfloor->t_slope;  
		dc_texturemid = FixedMul(left_top, wall_scaley);
	}
	else
		dc_texturemid = FixedMul(*pfloor->topheight - viewz, wall_scaley);

	repeatscroll = sidedef->rowoffset + sidedef->offsety_mid;

	if (dont_peg_bottom)
	{
		if (do_texture_skew)
		{
			skewslope = *pfloor->b_slope;  
			dc_texturemid = FixedMul(left_bottom, wall_scaley);
		}
		else
			repeatscroll -= FixedMul(*pfloor->topheight - *pfloor->bottomheight, wall_scaley);
	}

	if (skewslope)
	{
		angle_t lineangle = R_PointToAngle2(curline->v1->x, curline->v1->y, curline->v2->x, curline->v2->y);
		ffloortextureslide = FixedMul(R_GetSlopeTextureSlide(skewslope, lineangle), wall_scaley);
	}

	dc_texturemid += repeatscroll;


	R_CheckTextureCache(texnum);



	if (!textures[texnum]->transparency)
	{
		if (vertflip)  
			colfunc_2s = R_RenderFlipped2sidedMultiPatchColumn;
		else
			colfunc_2s = R_Render2sidedMultiPatchColumn;
	}
	else if (vertflip)  
		colfunc_2s = R_DrawRepeatFlippedMaskedColumn;
	else
		colfunc_2s = R_DrawRepeatMaskedColumn;

	lengthcol = textures[texnum]->height;


	{
		fixed_t right_top, right_bottom;


		right_top    = P_GetFFloorTopZAt   (pfloor, ds->rightpos.x, ds->rightpos.y) - viewz;
		right_bottom = P_GetFFloorBottomZAt(pfloor, ds->rightpos.x, ds->rightpos.y) - viewz;


		top_frac =    (INT64)centeryfrac - (((INT64)left_top     * ds->scale1) >> FRACBITS);
		bottom_frac = (INT64)centeryfrac - (((INT64)left_bottom  * ds->scale1) >> FRACBITS);
		top_step =    (INT64)centeryfrac - (((INT64)right_top    * ds->scale2) >> FRACBITS);
		bottom_step = (INT64)centeryfrac - (((INT64)right_bottom * ds->scale2) >> FRACBITS);

		top_step = (top_step-top_frac)/(range);
		bottom_step = (bottom_step-bottom_frac)/(range);

		top_frac += top_step * (x1 - ds->x1);
		bottom_frac += bottom_step * (x1 - ds->x1);
	}


	for (dc_x = x1; dc_x <= x2; dc_x++)
	{

		if (ffloortextureslide)
		{
			if (oldtexturecolumn != -1)
				dc_texturemid += FixedMul(ffloortextureslide, oldtexturecolumn-ds->thicksidecol[dc_x]);
			oldtexturecolumn = ds->thicksidecol[dc_x];
		}



		if      (top_frac > (INT64)CLAMPMAX) sprtopscreen = windowtop = CLAMPMAX;
		else if (top_frac > (INT64)CLAMPMIN) sprtopscreen = windowtop = (fixed_t)top_frac;
		else                                 sprtopscreen = windowtop = CLAMPMIN;
		if      (bottom_frac > (INT64)CLAMPMAX) sprbotscreen = windowbottom = CLAMPMAX;
		else if (bottom_frac > (INT64)CLAMPMIN) sprbotscreen = windowbottom = (fixed_t)bottom_frac;
		else                                    sprbotscreen = windowbottom = CLAMPMIN;

		fixed_t bottomclip = sprbotscreen;

		top_frac += top_step;
		bottom_frac += bottom_step;


		if (windowbottom < 0 || windowtop > (viewheight << FRACBITS))
		{
			if (dc_numlights)
			{
				for (i = 0; i < dc_numlights; i++)
				{
					rlight = &dc_lightlist[i];
					rlight->height += rlight->heightstep;
					if (rlight->flags & FOF_CUTLEVEL)
						rlight->botheight += rlight->botheightstep;
				}
			}
			yscale += scalestep;
			continue;
		}


		col = R_GetColumn(texnum, ((thicksidecol[dc_x] + wall_offsetx) >> FRACBITS));

		spryscale = DoubleToFixed(yscale);





		if (!fog)
		{
			dc_iscale = 0xffffffffu / (unsigned)spryscale;


			if (col->num_posts == 1 && col->posts[0].topdelta == 0 && col->posts[0].length == (unsigned)dc_texheight)
			{
				if (fuzzy)
					colfunc = colfuncs[COLDRAWFUNC_FUZZY];
				else
					colfunc = colfuncs[BASEDRAWFUNC];
			}
			else
			{

				if (fuzzy)
					colfunc = colfuncs[COLDRAWFUNC_CLAMPEDTRANS];
				else
					colfunc = colfuncs[COLDRAWFUNC_CLAMPED];
			}
		}



		if (dc_numlights)
		{
			lighttable_t **xwalllights;
			fixed_t height;
			fixed_t bheight = 0;
			boolean lighteffect = false;

			for (i = 0; i < dc_numlights; i++)
			{

				rlight = &dc_lightlist[i];
				lighteffect = !(rlight->flags & FOF_NOSHADE);
				if (lighteffect)
				{
					lightnum = rlight->lightnum;

					if (lightnum < 0)
						xwalllights = scalelight[0];
					else if (lightnum >= LIGHTLEVELS)
						xwalllights = scalelight[LIGHTLEVELS-1];
					else
						xwalllights = scalelight[lightnum];

					pindex = FixedMul(spryscale, LIGHTRESOLUTIONFIX)>>LIGHTSCALESHIFT;

					if (pindex >= MAXLIGHTSCALE)
						pindex = MAXLIGHTSCALE-1;

					if (pfloor->fofflags & FOF_FOG)
					{
						if (pfloor->master->frontsector->extra_colormap)
							rlight->rcolormap = pfloor->master->frontsector->extra_colormap->colormap + (xwalllights[pindex] - colormaps);
						else
							rlight->rcolormap = xwalllights[pindex];
					}
					else
					{
						if (rlight->extra_colormap)
							rlight->rcolormap = rlight->extra_colormap->colormap + (xwalllights[pindex] - colormaps);
						else
							rlight->rcolormap = xwalllights[pindex];
					}
				}


				boolean solid = false;

				if (rlight->flags & FOF_CUTSOLIDS && !(pfloor->fofflags & FOF_EXTRA))
					solid = true;
				else if (rlight->flags & FOF_CUTEXTRA && pfloor->fofflags & FOF_EXTRA)
				{
					if (rlight->flags & FOF_EXTRA)
					{


						if ((rlight->flags & (FOF_FOG|FOF_SWIMMABLE)) == (pfloor->fofflags & (FOF_FOG|FOF_SWIMMABLE)))
							solid = true;
					}
					else
						solid = true;
				}
				else
					solid = false;

				height = rlight->height;
				rlight->height += rlight->heightstep;

				if (solid)
				{
					bheight = rlight->botheight - (FRACUNIT >> 1);
					rlight->botheight += rlight->botheightstep;
				}

				if (height <= windowtop)
				{
					if (lighteffect)
						dc_colormap = rlight->rcolormap;
					if (solid && windowtop < bheight)
						sprtopscreen = windowtop = bheight;
					continue;
				}

				sprbotscreen = windowbottom = height;
				if (windowbottom >= bottomclip)
				{
					sprbotscreen = windowbottom = bottomclip;

					colfunc_2s (col, lengthcol);
					for (i++; i < dc_numlights; i++)
					{
						rlight = &dc_lightlist[i];
						rlight->height += rlight->heightstep;
						if (rlight->flags & FOF_CUTLEVEL)
							rlight->botheight += rlight->botheightstep;
					}
					continue;
				}

				colfunc_2s (col, lengthcol);
				if (solid)
					windowtop = bheight;
				else
					windowtop = windowbottom + 1;
				sprtopscreen = windowtop;
				if (lighteffect)
					dc_colormap = rlight->rcolormap;
			}
			sprbotscreen = windowbottom = bottomclip;

			if (windowtop < windowbottom)
				colfunc_2s (col, lengthcol);

			yscale += scalestep;
			continue;
		}


		pindex = FixedMul(spryscale, LIGHTRESOLUTIONFIX)>>LIGHTSCALESHIFT;

		if (pindex >= MAXLIGHTSCALE)
			pindex = MAXLIGHTSCALE - 1;

		dc_colormap = walllights[pindex];

		if (pfloor->fofflags & FOF_FOG && pfloor->master->frontsector->extra_colormap)
			dc_colormap = pfloor->master->frontsector->extra_colormap->colormap + (dc_colormap - colormaps);
		else if (frontsector->extra_colormap)
			dc_colormap = frontsector->extra_colormap->colormap + (dc_colormap - colormaps);


		colfunc_2s (col, lengthcol);
		yscale += scalestep;
	}
	colfunc = colfuncs[BASEDRAWFUNC];

#undef CLAMPMAX
#undef CLAMPMIN
}





static inline void R_ExpandPlaneY(visplane_t *pl, INT32 x, INT16 top, INT16 bottom)
{


	if (pl->top[x] > top)       pl->top[x] = top;
	if (pl->bottom[x] < bottom) pl->bottom[x] = bottom;
}




static boolean R_FFloorCanClip(visffloor_t *pfloor)
{
	return (cv_ffloorclip.value && !R_IsFFloorTranslucent(pfloor) && !pfloor->polyobj);
}









#define HEIGHTBITS              12
#define HEIGHTUNIT              (1<<HEIGHTBITS)

static void R_DrawRegularWall(UINT8 *source, INT32 height)
{
	dc_source = source;
	dc_texheight = height;
	colfunc();
}

static void R_DrawFlippedWall(UINT8 *source, INT32 height)
{
	dc_texheight = height;
	R_DrawFlippedPost(source, (unsigned)height, colfunc);
}

static void R_DrawNoWall(UINT8 *source, INT32 height)
{
	(void)source;
	(void)height;
}

static void R_RenderSegLoop (void)
{
	angle_t angle;
	fixed_t textureoffset;
	size_t pindex;
	INT32 yl;
	INT32 yh;

	INT32 mid;
	fixed_t texturecolumn = 0;
	fixed_t toptexturecolumn = 0;
	fixed_t bottomtexturecolumn = 0;
	fixed_t oldtexturecolumn = -1;
	fixed_t oldtexturecolumn_top = -1;
	fixed_t oldtexturecolumn_bottom = -1;
	INT32 top;
	INT32 bottom;
	INT32 i;

	fixed_t topscaley = rw_toptexturescaley;
	fixed_t midscaley = rw_midtexturescaley;
	fixed_t bottomscaley = rw_bottomtexturescaley;

	void (*drawtop)(UINT8 *, INT32) = R_DrawRegularWall;
	void (*drawmiddle)(UINT8 *, INT32) = R_DrawRegularWall;
	void (*drawbottom)(UINT8 *, INT32) = R_DrawRegularWall;

	if (dc_numlights)
		colfunc = colfuncs[COLDRAWFUNC_SHADOWED];

	if (toptexture && topscaley < 0)
	{
		topscaley = -topscaley;
		drawtop = R_DrawFlippedWall;
	}
	if (midtexture && midscaley < 0)
	{
		midscaley = -midscaley;
		drawmiddle = R_DrawFlippedWall;
	}
	if (bottomtexture && bottomscaley < 0)
	{
		bottomscaley = -bottomscaley;
		drawbottom = R_DrawFlippedWall;
	}

	if (!r_renderwalls)
	{
		drawtop = R_DrawNoWall;
		drawmiddle = R_DrawNoWall;
		drawbottom = R_DrawNoWall;
	}

	if (midtexture)
		R_CheckTextureCache(midtexture);
	if (toptexture)
		R_CheckTextureCache(toptexture);
	if (bottomtexture)
		R_CheckTextureCache(bottomtexture);

	                                                                        
                                                                             
                                                                           
                                                                         
                                                           
	if (midtexture && textures[midtexture]->fullytransparent)
		drawmiddle = R_DrawNoWall;
	if (toptexture && textures[toptexture]->fullytransparent)
		drawtop = R_DrawNoWall;
	if (bottomtexture && textures[bottomtexture]->fullytransparent)
		drawbottom = R_DrawNoWall;

	if (dc_numlights)
		colfunc = colfuncs[COLDRAWFUNC_SHADOWED];

	for (; rw_x < rw_stopx; rw_x++)
	{

		yl = (topfrac+HEIGHTUNIT-1)>>HEIGHTBITS;

		top = ceilingclip[rw_x]+1;


		if (yl < top)
			yl = top;

		if (markceiling)
		{
#if 0
			bottom = yl-1;

			if (bottom >= floorclip[rw_x])
				bottom = floorclip[rw_x]-1;

			if (top <= bottom)
#else
			bottom = yl > floorclip[rw_x] ? floorclip[rw_x] : yl;

			if (top <= --bottom && ceilingplane)
#endif
				R_ExpandPlaneY(ceilingplane, rw_x, top, bottom);
		}


		yh = bottomfrac>>HEIGHTBITS;

		bottom = floorclip[rw_x]-1;

		if (yh > bottom)
			yh = bottom;

		if (markfloor)
		{
			top = yh < ceilingclip[rw_x] ? ceilingclip[rw_x] : yh;

			if (++top <= bottom && floorplane)
				R_ExpandPlaneY(floorplane, rw_x, top, bottom);
		}

		rw_floormarked = false;
		rw_ceilingmarked = false;

		if (numffloors)
		{
			INT16 fftop, ffbottom;

			firstseg->frontscale[rw_x] = frontscale[rw_x];
			top = ceilingclip[rw_x]+1;  
			bottom = floorclip[rw_x]-1;  

			for (i = 0; i < numffloors; i++)
			{
				if (ffloor[i].polyobj && (!curline->polyseg || ffloor[i].polyobj != curline->polyseg))
					continue;

				if (ffloor[i].height < viewz)
				{
					INT32 top_w = (ffloor[i].f_frac >> HEIGHTBITS) + 1;
					INT32 bottom_w = ffloor[i].f_clip[rw_x];

					if (top_w < top)
						top_w = top;

					if (bottom_w > bottom)
						bottom_w = bottom;


					if (ffloor[i].polyobj && top_w >= bottom_w)
					{
						ffloor[i].plane->top[rw_x] = 0xFFFF;
						ffloor[i].plane->bottom[rw_x] = 0x0000;  
					}
					else
					{
						if (top_w <= bottom_w)
						{
							fftop = (INT16)top_w;
							ffbottom = (INT16)bottom_w;

							ffloor[i].plane->top[rw_x] = fftop;
							ffloor[i].plane->bottom[rw_x] = ffbottom;



							if (ffbottom >= bottom-1 && R_FFloorCanClip(&ffloor[i]) && !curline->polyseg)
							{
								rw_floormarked = true;
								floorclip[rw_x] = fftop;
								if (yh > fftop)
									yh = fftop;

								if (markfloor && floorplane)
									floorplane->top[rw_x] = bottom;

								if (rw_silhouette)
								{
									(*rw_silhouette) |= SIL_BOTTOM;
									(*rw_bsilheight) = INT32_MAX;
								}
							}
						}
					}
				}
				else if (ffloor[i].height > viewz)
				{
					INT32 top_w = ffloor[i].c_clip[rw_x] + 1;
					INT32 bottom_w = (ffloor[i].f_frac >> HEIGHTBITS);

					if (top_w < top)
						top_w = top;

					if (bottom_w > bottom)
						bottom_w = bottom;


					if (ffloor[i].polyobj && top_w >= bottom_w)
					{
						ffloor[i].plane->top[rw_x] = 0xFFFF;
						ffloor[i].plane->bottom[rw_x] = 0x0000;  
					}
					else
					{
						if (top_w <= bottom_w)
						{
							fftop = (INT16)top_w;
							ffbottom = (INT16)bottom_w;

							ffloor[i].plane->top[rw_x] = fftop;
							ffloor[i].plane->bottom[rw_x] = ffbottom;



							if (fftop <= top+1 && R_FFloorCanClip(&ffloor[i]) && !curline->polyseg)
							{
								rw_ceilingmarked = true;
								ceilingclip[rw_x] = ffbottom;
								if (yl < ffbottom)
									yl = ffbottom;

								if (markceiling && ceilingplane)
									ceilingplane->bottom[rw_x] = top;

								if (rw_silhouette)
								{
									(*rw_silhouette) |= SIL_TOP;
									(*rw_tsilheight) = INT32_MIN;
								}
							}
						}
					}
				}
			}
		}



		angle = (rw_centerangle + xtoviewangle[rw_x])>>ANGLETOFINESHIFT;
		textureoffset = rw_offset - FixedMul(FINETANGENT(angle), rw_distance);
		texturecolumn = FixedDiv(textureoffset, rw_invmidtexturescalex);


		if (segtextured)
		{

			pindex = FixedMul(rw_scale, LIGHTRESOLUTIONFIX)>>LIGHTSCALESHIFT;

			if (pindex >=  MAXLIGHTSCALE)
				pindex = MAXLIGHTSCALE-1;

			dc_colormap = walllights[pindex];
			dc_x = rw_x;

			if (frontsector->extra_colormap)
				dc_colormap = frontsector->extra_colormap->colormap + (dc_colormap - colormaps);
		}

		if (dc_numlights)
		{
			lighttable_t **xwalllights;
			for (i = 0; i < dc_numlights; i++)
			{
				INT32 lightnum;
				lightnum = R_SideLightLevel(curline->sidedef, dc_lightlist[i].lightlevel) >> LIGHTSEGSHIFT;

				if (dc_lightlist[i].extra_colormap)
					;
				else if (curline->v1->y == curline->v2->y)
					lightnum--;
				else if (curline->v1->x == curline->v2->x)
					lightnum++;

				if (lightnum < 0)
					xwalllights = scalelight[0];
				else if (lightnum >= LIGHTLEVELS)
					xwalllights = scalelight[LIGHTLEVELS-1];
				else
					xwalllights = scalelight[lightnum];

				pindex = FixedMul(rw_scale, LIGHTRESOLUTIONFIX)>>LIGHTSCALESHIFT;

				if (pindex >=  MAXLIGHTSCALE)
					pindex = MAXLIGHTSCALE-1;

				if (dc_lightlist[i].extra_colormap)
					dc_lightlist[i].rcolormap = dc_lightlist[i].extra_colormap->colormap + (xwalllights[pindex] - colormaps);
				else
					dc_lightlist[i].rcolormap = xwalllights[pindex];
			}
		}

		frontscale[rw_x] = rw_scale;


		if (midtexture)
		{

			if (yl <= yh && yh >= 0 && yl < viewheight)
			{
				fixed_t offset = texturecolumn + rw_offsetx;

				dc_yl = yl;
				dc_yh = yh;
				dc_texturemid = rw_midtexturemid;
				dc_texheight = textureheight[midtexture]>>FRACBITS;
				dc_iscale = FixedMul(0xffffffffu / (unsigned)rw_scale, midscaley);
				drawmiddle(R_GetColumn(midtexture, offset >> FRACBITS)->pixels, dc_texheight);



				if (!rw_ceilingmarked)
					ceilingclip[rw_x] = (INT16)viewheight;
				if (!rw_floormarked)
					floorclip[rw_x] = -1;
			}
			else
			{

				if (markceiling && (!rw_ceilingmarked))
					ceilingclip[rw_x] = (yl >= 0) ? ((yl > viewheight) ? (INT16)viewheight : (INT16)((INT16)yl - 1)) : -1;
				if (markfloor && (!rw_floormarked))
					floorclip[rw_x] = (yh < viewheight) ? ((yh < -1) ? -1 : (INT16)((INT16)yh + 1)) : (INT16)viewheight;
			}
		}
		else
		{
			INT16 topclip = (yl >= 0) ? ((yl > viewheight) ? (INT16)viewheight : (INT16)((INT16)yl - 1)) : -1;
			INT16 bottomclip = (yh < viewheight) ? ((yh < -1) ? -1 : (INT16)((INT16)yh + 1)) : (INT16)viewheight;


			if (toptexture)
			{

				mid = pixhigh>>HEIGHTBITS;
				pixhigh += pixhighstep;

				if (mid >= floorclip[rw_x])
					mid = floorclip[rw_x]-1;

				toptexturecolumn = FixedDiv(textureoffset, rw_invtoptexturescalex);

				if (mid >= yl)  
				{
					if (yl >= viewheight)  
					{
						if (!rw_ceilingmarked)
							ceilingclip[rw_x] = (INT16)viewheight;
					}
					else if (mid >= 0)  
					{
						fixed_t offset = rw_offset_top;
						if (rw_toptexturescalex < 0)
							offset = -offset;
						offset = toptexturecolumn + offset;

						dc_yl = yl;
						dc_yh = mid;
						dc_texturemid = rw_toptexturemid;
						dc_texheight = textureheight[toptexture]>>FRACBITS;
						dc_iscale = FixedMul(0xffffffffu / (unsigned)rw_scale, topscaley);
						drawtop(R_GetColumn(toptexture, offset >> FRACBITS)->pixels, dc_texheight);
						ceilingclip[rw_x] = (INT16)mid;
					}
					else if (!rw_ceilingmarked)  
						ceilingclip[rw_x] = -1;
				}
				else if (!rw_ceilingmarked)
					ceilingclip[rw_x] = topclip;

				if (oldtexturecolumn_top != -1)
					rw_toptexturemid += FixedMul(rw_toptextureslide, oldtexturecolumn_top-textureoffset);
				oldtexturecolumn_top = textureoffset;
			}
			else if (markceiling && (!rw_ceilingmarked))  
				ceilingclip[rw_x] = topclip;

			if (bottomtexture)
			{

				mid = (pixlow+HEIGHTUNIT-1)>>HEIGHTBITS;
				pixlow += pixlowstep;


				if (mid <= ceilingclip[rw_x])
					mid = ceilingclip[rw_x]+1;

				bottomtexturecolumn = FixedDiv(textureoffset, rw_invbottomtexturescalex);

				if (mid <= yh)  
				{
					if (yh < 0)  
					{
						if (!rw_floormarked)
							floorclip[rw_x] = -1;
					}
					else if (mid < viewheight)  
					{
						fixed_t offset = rw_offset_bottom;
						if (rw_bottomtexturescalex < 0)
							offset = -offset;
						offset = bottomtexturecolumn + offset;

						dc_yl = mid;
						dc_yh = yh;
						dc_texturemid = rw_bottomtexturemid;
						dc_texheight = textureheight[bottomtexture]>>FRACBITS;
						dc_iscale = FixedMul(0xffffffffu / (unsigned)rw_scale, bottomscaley);
						drawbottom(R_GetColumn(bottomtexture, offset >> FRACBITS)->pixels, dc_texheight);
						floorclip[rw_x] = (INT16)mid;
					}
					else if (!rw_floormarked)   
						floorclip[rw_x] = (INT16)viewheight;
				}
				else if (!rw_floormarked)
					floorclip[rw_x] = bottomclip;

				if (oldtexturecolumn_bottom != -1)
					rw_bottomtexturemid += FixedMul(rw_bottomtextureslide, oldtexturecolumn_bottom-textureoffset);
				oldtexturecolumn_bottom = textureoffset;
			}
			else if (markfloor && (!rw_floormarked))  
				floorclip[rw_x] = bottomclip;
		}

		if (maskedtexturecol)
			maskedtexturecol[rw_x] = texturecolumn + rw_offsetx;

		if (thicksidecol)
			thicksidecol[rw_x] = textureoffset;

		if (maskedtextureheight)
		{
			if (curline->linedef->flags & ML_MIDPEG)
				maskedtextureheight[rw_x] = max(rw_midtexturemid, rw_midtextureback);
			else
				maskedtextureheight[rw_x] = min(rw_midtexturemid, rw_midtextureback);
		}

		if (midtexture || maskedtextureheight)
		{
			if (oldtexturecolumn != -1)
			{
				INT32 diff = oldtexturecolumn-textureoffset;
				if (rw_invmidtexturescalex < 0)
					diff = -diff;
				rw_midtexturemid += FixedMul(rw_midtextureslide, diff);
				rw_midtextureback += FixedMul(rw_midtexturebackslide, diff);
			}

			oldtexturecolumn = textureoffset;
		}

		if (invscale)
			invscale[rw_x] = 0xffffffffu / (unsigned)rw_scale;

		if (dc_numlights)
		{
			for (i = 0; i < dc_numlights; i++)
			{
				dc_lightlist[i].height += dc_lightlist[i].heightstep;
				if (dc_lightlist[i].flags & FOF_CUTSOLIDS)
					dc_lightlist[i].botheight += dc_lightlist[i].botheightstep;
			}
		}

		for (i = 0; i < numffloors; i++)
		{
			if (curline->polyseg && (ffloor[i].polyobj != curline->polyseg))
				continue;

			ffloor[i].f_frac += ffloor[i].f_step;
		}

		for (i = 0; i < numbackffloors; i++)
		{
			if (curline->polyseg && (ffloor[i].polyobj != curline->polyseg))
				continue;

			ffloor[i].f_clip[rw_x] = ffloor[i].c_clip[rw_x] = (INT16)((ffloor[i].b_frac >> HEIGHTBITS) & 0xFFFF);
			ffloor[i].b_frac += ffloor[i].b_step;
		}

		rw_scale += rw_scalestep;
		topfrac += topstep;
		bottomfrac += bottomstep;
	}
}


static INT64 R_CalcSegDist(seg_t* seg, INT64 x2, INT64 y2)
{
	if (!seg->linedef->dy)
		return llabs(y2 - seg->v1->y);
	else if (!seg->linedef->dx)
		return llabs(x2 - seg->v1->x);
	else
	{
		INT64 dx = (seg->v2->x)-(seg->v1->x);
		INT64 dy = (seg->v2->y)-(seg->v1->y);
		INT64 vdx = x2-(seg->v1->x);
		INT64 vdy = y2-(seg->v1->y);
		return ((dy*vdx)-(dx*vdy))/(seg->length);
	}
}


static void R_AllocClippingTables(size_t range)
{
	size_t pos = lastopening - openings;
	size_t need = range * 2;  

	if (pos + need < numopenings)
		return;

	INT16 *oldopenings = openings;
	INT16 *oldlast = lastopening;

	if (numopenings == 0)
		numopenings = 16384;

	numopenings += need;
	openings = Z_Realloc(openings, numopenings * sizeof (*openings), PU_STATIC, NULL);
	lastopening = openings + pos;

	if (oldopenings == NULL)
		return;




	for (drawseg_t *ds = drawsegs; ds < ds_p; ds++)
	{

		if (ds->sprtopclip + ds->x1 >= oldopenings && ds->sprtopclip + ds->x1 <= oldlast)
			ds->sprtopclip = (ds->sprtopclip - oldopenings) + openings;
		if (ds->sprbottomclip + ds->x1 >= oldopenings && ds->sprbottomclip + ds->x1 <= oldlast)
			ds->sprbottomclip = (ds->sprbottomclip - oldopenings) + openings;
	}
}

static void R_AllocTextureColumnTables(size_t range)
{
	size_t pos = curtexturecolumntable - texturecolumntable;
	size_t need = range * 4;

	if (pos + need < texturecolumntablesize)
		return;

	fixed_t *oldtable = texturecolumntable;
	fixed_t *oldlast = curtexturecolumntable;

	if (texturecolumntablesize == 0)
		texturecolumntablesize = 16384;

	texturecolumntablesize += need;
	texturecolumntable = Z_Realloc(texturecolumntable, texturecolumntablesize * sizeof (*texturecolumntable), PU_STATIC, NULL);
	curtexturecolumntable = texturecolumntable + pos;

	if (oldtable == NULL)
		return;

	for (drawseg_t *ds = drawsegs; ds < ds_p; ds++)
	{

#define CHECK(which) \
		if (which + ds->x1 >= oldtable && which + ds->x1 <= oldlast) \
			which = (which - oldtable) + texturecolumntable

		CHECK(ds->maskedtexturecol);
		CHECK(ds->maskedtextureheight);
		CHECK(ds->thicksidecol);
		CHECK(ds->invscale);

#undef CHECK
	}
}










static fixed_t R_ScaleFromGlobalAngle(angle_t visangle)
{
	angle_t anglea = ANGLE_90 + (visangle-viewangle);
	angle_t angleb = ANGLE_90 + (visangle-rw_normalangle);
	fixed_t den = FixedMul(rw_distance, R_InterpolatedFineSine(anglea));

	fixed_t num = FixedMul(projectiony, R_InterpolatedFineSine(angleb));

	if (den > num>>16)
	{
		num = FixedDiv(num, den);
		if (num > 64*FRACUNIT)
			return 64*FRACUNIT;
		if (num < 256)
			return 256;
		return num;
	}
	return 64*FRACUNIT;
}






void R_StoreWallRange(INT32 start, INT32 stop)
{
	fixed_t       hyp;
	fixed_t       sineval;
	angle_t       distangle, offsetangle;
	boolean longboi;
	INT32           lightnum;
	INT32           i, p;
	lightlist_t   *light;
	r_lightlist_t *rlight;
	INT32 range;
	vertex_t segleft, segright;
	fixed_t ceilingfrontslide, floorfrontslide, ceilingbackslide, floorbackslide;
	static size_t maxdrawsegs = 0;

	maskedtexturecol = NULL;
	maskedtextureheight = NULL;
	thicksidecol = NULL;
	invscale = NULL;


	memset(&segleft, 0x00, sizeof(segleft));
	memset(&segright, 0x00, sizeof(segright));

	colfunc = colfuncs[BASEDRAWFUNC];

	if (ds_p == drawsegs+maxdrawsegs)
	{
		size_t curpos = curdrawsegs - drawsegs;
		size_t pos = ds_p - drawsegs;
		size_t newmax = maxdrawsegs ? maxdrawsegs*2 : 128;
		if (firstseg)
			firstseg = (drawseg_t *)(firstseg - drawsegs);
		drawsegs = Z_Realloc(drawsegs, newmax*sizeof (*drawsegs), PU_STATIC, NULL);
		ds_p = drawsegs + pos;
		maxdrawsegs = newmax;
		curdrawsegs = drawsegs + curpos;
		if (firstseg)
			firstseg = drawsegs + (size_t)firstseg;
	}

	sidedef = curline->sidedef;
	linedef = curline->linedef;


	rw_normalangle = curline->angle + ANGLE_90;
	offsetangle = abs((INT32)(rw_normalangle-rw_angle1));

	if (offsetangle > ANGLE_90)
		offsetangle = ANGLE_90;

	distangle = ANGLE_90 - offsetangle;
	sineval = R_InterpolatedFineSine(distangle);

	hyp = R_PointToDist(curline->v1->x, curline->v1->y);
	rw_distance = FixedMul(hyp, sineval);
	longboi = (hyp >= INT32_MAX);


	if (longboi)
		rw_distance = (fixed_t)R_CalcSegDist(curline,viewx,viewy);

	ds_p->x1 = rw_x = start;
	ds_p->x2 = stop;
	ds_p->curline = curline;
	rw_stopx = stop+1;


	ds_p->scale1 = rw_scale = R_ScaleFromGlobalAngle(viewangle + xtoviewangle[start]);

	if (stop > start)
	{
		ds_p->scale2 = R_ScaleFromGlobalAngle(viewangle + xtoviewangle[stop]);
		range = stop-start;
	}
	else
	{

#if 0
		if (rw_distance < FRACUNIT/2)
		{
			fixed_t         tr_x,tr_y;
			fixed_t         gxt,gyt;
			CONS_Debug(DBG_RENDER, "TRYING TO FIX THE STRETCHED ETC\n");

			tr_x = curline->v1->x - viewx;
			tr_y = curline->v1->y - viewy;

			gxt = FixedMul(tr_x, viewcos);
			gyt = -FixedMul(tr_y, viewsin);
			ds_p->scale1 = FixedDiv(projection, gxt - gyt);
		}
#endif
		ds_p->scale2 = ds_p->scale1;
		range = 1;
	}

	ds_p->scalestep = rw_scalestep = (ds_p->scale2 - rw_scale) / (range);




	if (frontsector->hasslope || (backsector && backsector->hasslope))  
	{
		angle_t temp;


		temp = xtoviewangle[start]+viewangle;

		{

			double a1, b1, c1, a2, b2, c2, det;  


			a1 = FixedToDouble(curline->v2->y-curline->v1->y);
			b1 = FixedToDouble(curline->v1->x-curline->v2->x);
			c1 = a1*FixedToDouble(curline->v1->x) + b1*FixedToDouble(curline->v1->y);

			a2 = -FixedToDouble(FINESINE(temp>>ANGLETOFINESHIFT));
			b2 = FixedToDouble(FINECOSINE(temp>>ANGLETOFINESHIFT));
			c2 = a2*FixedToDouble(viewx) + b2*FixedToDouble(viewy);

			det = a1*b2 - a2*b1;

			ds_p->leftpos.x = segleft.x = DoubleToFixed((b2*c1 - b1*c2)/det);
			ds_p->leftpos.y = segleft.y = DoubleToFixed((a1*c2 - a2*c1)/det);
		}


		temp = xtoviewangle[stop]+viewangle;

		{

			double a1, b1, c1, a2, b2, c2, det;  


			a1 = FixedToDouble(curline->v2->y-curline->v1->y);
			b1 = FixedToDouble(curline->v1->x-curline->v2->x);
			c1 = a1*FixedToDouble(curline->v1->x) + b1*FixedToDouble(curline->v1->y);

			a2 = -FixedToDouble(FINESINE(temp>>ANGLETOFINESHIFT));
			b2 = FixedToDouble(FINECOSINE(temp>>ANGLETOFINESHIFT));
			c2 = a2*FixedToDouble(viewx) + b2*FixedToDouble(viewy);

			det = a1*b2 - a2*b1;

			ds_p->rightpos.x = segright.x = DoubleToFixed((b2*c1 - b1*c2)/det);
			ds_p->rightpos.y = segright.y = DoubleToFixed((a1*c2 - a2*c1)/det);
		}
	}

#define SLOPEPARAMS(slope, end1, end2, normalheight) \
	end1 = P_GetZAt(slope,  segleft.x,  segleft.y, normalheight); \
	end2 = P_GetZAt(slope, segright.x, segright.y, normalheight);

	SLOPEPARAMS(frontsector->c_slope, worldtop,    worldtopslope,    frontsector->ceilingheight)
	SLOPEPARAMS(frontsector->f_slope, worldbottom, worldbottomslope, frontsector->floorheight)


	worldtop -= viewz;
	worldtopslope -= viewz;
	worldbottom -= viewz;
	worldbottomslope -= viewz;

	midtexture = toptexture = bottomtexture = maskedtexture = 0;
	ds_p->maskedtexturecol = NULL;
	ds_p->maskedtextureheight = NULL;
	ds_p->numthicksides = numthicksides = 0;
	ds_p->thicksidecol = NULL;
	ds_p->invscale = NULL;
	ds_p->tsilheight = 0;

	texcoltables = false;

	numbackffloors = 0;

	for (i = 0; i < MAXFFLOORS; i++)
		ds_p->thicksides[i] = NULL;

	if (numffloors)
	{
		for (i = 0; i < numffloors; i++)
		{
			if (ffloor[i].polyobj && (!ds_p->curline->polyseg || ffloor[i].polyobj != ds_p->curline->polyseg))
				continue;

			ffloor[i].f_pos       = P_GetZAt(ffloor[i].slope, segleft .x, segleft .y, ffloor[i].height) - viewz;
			ffloor[i].f_pos_slope = P_GetZAt(ffloor[i].slope, segright.x, segright.y, ffloor[i].height) - viewz;
		}
	}


	rw_toptextureslide = rw_midtextureslide = rw_bottomtextureslide = 0;
	ceilingfrontslide = floorfrontslide = ceilingbackslide = floorbackslide = 0;

	{
		angle_t lineangle = R_PointToAngle2(curline->v1->x, curline->v1->y, curline->v2->x, curline->v2->y);

		if (frontsector->f_slope)
			floorfrontslide = R_GetSlopeTextureSlide(frontsector->f_slope, lineangle);

		if (frontsector->c_slope)
			ceilingfrontslide = R_GetSlopeTextureSlide(frontsector->c_slope, lineangle);

		if (backsector)
		{
			if (backsector->f_slope)
				floorbackslide = R_GetSlopeTextureSlide(backsector->f_slope, lineangle);

			if (backsector->c_slope)
				ceilingbackslide = R_GetSlopeTextureSlide(backsector->c_slope, lineangle);
		}
	}

	rw_midtexturescalex = sidedef->scalex_mid;
	rw_midtexturescaley = sidedef->scaley_mid;
	rw_invmidtexturescalex = FixedDiv(FRACUNIT, rw_midtexturescalex);

	if (!backsector)
	{
		midtexture = R_GetTextureNum(sidedef->midtexture);


		markfloor = markceiling = true;

		fixed_t rowoffset = sidedef->rowoffset + sidedef->offsety_mid;
		fixed_t texheight = textureheight[midtexture];
		fixed_t scaley = abs(rw_midtexturescaley);

		if (rw_midtexturescaley > 0)
		{
			if (linedef->flags & ML_NOSKEW)
			{
				if (linedef->flags & ML_DONTPEGBOTTOM)
					rw_midtexturemid = FixedMul(frontsector->floorheight - viewz, scaley) + texheight;
				else
					rw_midtexturemid = FixedMul(frontsector->ceilingheight - viewz, scaley);
			}
			else if (linedef->flags & ML_DONTPEGBOTTOM)
			{
				rw_midtexturemid = FixedMul(worldbottom, scaley) + texheight;
				rw_midtextureslide = FixedMul(floorfrontslide, scaley);
			}
			else
			{

				rw_midtexturemid = FixedMul(worldtop, scaley);
				rw_midtextureslide = FixedMul(ceilingfrontslide, scaley);
			}
		}
		else
		{

			rowoffset = -rowoffset;

			if (linedef->flags & ML_NOSKEW)
			{
				if (linedef->flags & ML_DONTPEGBOTTOM)
					rw_midtexturemid = FixedMul(frontsector->floorheight - viewz, scaley);
				else
					rw_midtexturemid = FixedMul(frontsector->ceilingheight - viewz, scaley) + texheight;
			}
			else if (linedef->flags & ML_DONTPEGBOTTOM)
			{
				rw_midtexturemid = FixedMul(worldbottom, scaley);
				rw_midtextureslide = FixedMul(floorfrontslide, scaley);
			}
			else
			{

				rw_midtexturemid = FixedMul(worldtop, scaley) + texheight;
				rw_midtextureslide = FixedMul(ceilingfrontslide, scaley);
			}
		}

		rw_midtexturemid += rowoffset;

		ds_p->silhouette = SIL_BOTH;
		ds_p->sprtopclip = screenheightarray;
		ds_p->sprbottomclip = negonearray;
		ds_p->bsilheight = INT32_MAX;
		ds_p->tsilheight = INT32_MIN;
	}
	else
	{

		SLOPEPARAMS(backsector->c_slope, worldhigh, worldhighslope, backsector->ceilingheight)
		SLOPEPARAMS(backsector->f_slope, worldlow,  worldlowslope,  backsector->floorheight)
		worldhigh -= viewz;
		worldhighslope -= viewz;
		worldlow -= viewz;
		worldlowslope -= viewz;

		ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
		ds_p->silhouette = 0;

		if (!bothfloorssky)
		{
			if (worldbottomslope > worldlowslope || worldbottom > worldlow)
			{
				ds_p->silhouette = SIL_BOTTOM;
				if (P_GetSectorFloorZAt(backsector, viewx, viewy) > viewz)
					ds_p->bsilheight = INT32_MAX;
				else
					ds_p->bsilheight = (frontsector->f_slope ? INT32_MAX : frontsector->floorheight);
			}
			else if (P_GetSectorFloorZAt(backsector, viewx, viewy) > viewz)
			{
				ds_p->silhouette = SIL_BOTTOM;
				ds_p->bsilheight = INT32_MAX;

			}
		}

		if (!bothceilingssky)
		{
			if (worldtopslope < worldhighslope || worldtop < worldhigh)
			{
				ds_p->silhouette |= SIL_TOP;
				if (P_GetSectorCeilingZAt(backsector, viewx, viewy) < viewz)
					ds_p->tsilheight = INT32_MIN;
				else
					ds_p->tsilheight = (frontsector->c_slope ? INT32_MIN : frontsector->ceilingheight);
			}
			else if (P_GetSectorCeilingZAt(backsector, viewx, viewy) < viewz)
			{
				ds_p->silhouette |= SIL_TOP;
				ds_p->tsilheight = INT32_MIN;

			}
		}

		if (!bothceilingssky && !bothfloorssky)
		{
			if (worldhigh <= worldbottom && worldhighslope <= worldbottomslope)
			{
				ds_p->sprbottomclip = negonearray;
				ds_p->bsilheight = INT32_MAX;
				ds_p->silhouette |= SIL_BOTTOM;
			}

			if (worldlow >= worldtop && worldlowslope >= worldtopslope)
			{
				ds_p->sprtopclip = screenheightarray;
				ds_p->tsilheight = INT32_MIN;
				ds_p->silhouette |= SIL_TOP;
			}
		}




		if (!bothceilingssky && !bothfloorssky)
		{
			if (doorclosed || (worldhigh <= worldbottom && worldhighslope <= worldbottomslope))
			{
				ds_p->sprbottomclip = negonearray;
				ds_p->bsilheight = INT32_MAX;
				ds_p->silhouette |= SIL_BOTTOM;
			}
			if (doorclosed || (worldlow >= worldtop && worldlowslope >= worldtopslope))
			{                    
				ds_p->sprtopclip = screenheightarray;
				ds_p->tsilheight = INT32_MIN;
				ds_p->silhouette |= SIL_TOP;
			}
		}

		if (bothfloorssky)
		{


			markfloor = false;
		}
		else if (worldlow != worldbottom
			|| worldlowslope != worldbottomslope
			|| backsector->f_slope != frontsector->f_slope
			|| backsector->floorpic != frontsector->floorpic
			|| backsector->lightlevel != frontsector->lightlevel

			|| backsector->floorxoffset != frontsector->floorxoffset
			|| backsector->flooryoffset != frontsector->flooryoffset
			|| backsector->floorxscale != frontsector->floorxscale
			|| backsector->flooryscale != frontsector->flooryscale
			|| backsector->floorangle != frontsector->floorangle

			|| (frontsector->heightsec != -1 && frontsector->floorpic != skyflatnum)
			|| backsector->floorlightlevel != frontsector->floorlightlevel
			|| backsector->floorlightabsolute != frontsector->floorlightabsolute
			|| backsector->floorlightsec != frontsector->floorlightsec

			|| frontsector->extra_colormap != backsector->extra_colormap
			|| !P_CompareSectorPortals(P_SectorGetFloorPortal(frontsector), P_SectorGetFloorPortal(backsector))
			|| (frontsector->ffloors != backsector->ffloors && !Tag_Compare(&frontsector->tags, &backsector->tags)))
		{
			markfloor = true;
		}
		else
		{

			markfloor = false;
		}

		if (bothceilingssky)
		{



			markceiling = false;
		}
		else if (worldhigh != worldtop
			|| worldhighslope != worldtopslope
			|| backsector->c_slope != frontsector->c_slope
			|| backsector->ceilingpic != frontsector->ceilingpic
			|| backsector->lightlevel != frontsector->lightlevel

			|| backsector->ceilingxoffset != frontsector->ceilingxoffset
			|| backsector->ceilingyoffset != frontsector->ceilingyoffset
			|| backsector->ceilingxscale != frontsector->ceilingxscale
			|| backsector->ceilingyscale != frontsector->ceilingyscale
			|| backsector->ceilingangle != frontsector->ceilingangle

			|| (frontsector->heightsec != -1 && frontsector->ceilingpic != skyflatnum)
			|| backsector->ceilinglightlevel != frontsector->ceilinglightlevel
			|| backsector->ceilinglightabsolute != frontsector->ceilinglightabsolute
			|| backsector->ceilinglightsec != frontsector->ceilinglightsec

			|| frontsector->extra_colormap != backsector->extra_colormap
			|| !P_CompareSectorPortals(P_SectorGetCeilingPortal(frontsector), P_SectorGetCeilingPortal(backsector))
			|| (frontsector->ffloors != backsector->ffloors && !Tag_Compare(&frontsector->tags, &backsector->tags)))
		{
			markceiling = true;
		}
		else
		{

			markceiling = false;
		}

		if (!bothceilingssky && !bothfloorssky)
		{
			if ((worldhigh <= worldbottom && worldhighslope <= worldbottomslope)
			 || (worldlow >= worldtop && worldlowslope >= worldtopslope))
			{

				markceiling = markfloor = true;
			}
		}

		fixed_t toprowoffset = sidedef->rowoffset + sidedef->offsety_top;
		fixed_t botrowoffset = sidedef->rowoffset + sidedef->offsety_bottom;


		if (!bothceilingssky  
			&& (worldhigh < worldtop || worldhighslope < worldtopslope))
		{
			toptexture = R_GetTextureNum(sidedef->toptexture);

			rw_toptexturescalex = sidedef->scalex_top;
			rw_toptexturescaley = sidedef->scaley_top;

			rw_invtoptexturescalex = FixedDiv(FRACUNIT, rw_toptexturescalex);

			if (rw_toptexturescaley < 0)
				toprowoffset = -toprowoffset;

			fixed_t texheight = textureheight[toptexture];


			if (!(linedef->flags & ML_SKEWTD))
			{
				if (linedef->flags & ML_DONTPEGTOP)
					rw_toptexturemid = frontsector->ceilingheight - viewz;
				else
					rw_toptexturemid = backsector->ceilingheight - viewz;
			}
			else if (linedef->flags & ML_DONTPEGTOP)
			{

				rw_toptexturemid = worldtop;
				rw_toptextureslide = FixedMul(ceilingfrontslide, abs(rw_toptexturescaley));
			}
			else
			{
				rw_toptexturemid = worldhigh + texheight;
				rw_toptextureslide = FixedMul(ceilingbackslide, abs(rw_toptexturescaley));
			}

			rw_toptexturemid = FixedMul(rw_toptexturemid, abs(rw_toptexturescaley));
			rw_toptexturemid += toprowoffset;
		}


		if (!bothfloorssky  
			&& (worldlow > worldbottom || worldlowslope > worldbottomslope))  
		{

			bottomtexture = R_GetTextureNum(sidedef->bottomtexture);

			rw_bottomtexturescalex = sidedef->scalex_bottom;
			rw_bottomtexturescaley = sidedef->scaley_bottom;

			rw_invbottomtexturescalex = FixedDiv(FRACUNIT, rw_bottomtexturescalex);

			if (rw_bottomtexturescaley < 0)
				botrowoffset = -botrowoffset;


			if (!(linedef->flags & ML_SKEWTD))
			{
				if (linedef->flags & ML_DONTPEGBOTTOM)
					rw_bottomtexturemid = frontsector->floorheight - viewz;
				else
					rw_bottomtexturemid = backsector->floorheight - viewz;
			}
			else if (linedef->flags & ML_DONTPEGBOTTOM)
			{


				rw_bottomtexturemid = worldbottom;
				rw_bottomtextureslide = FixedMul(floorfrontslide, abs(rw_bottomtexturescaley));
			}
			else
			{

				rw_bottomtexturemid = worldlow;
				rw_bottomtextureslide = FixedMul(floorbackslide, abs(rw_bottomtexturescaley));
			}

			rw_bottomtexturemid = FixedMul(rw_bottomtexturemid, abs(rw_bottomtexturescaley));
			rw_bottomtexturemid += botrowoffset;
		}


		R_AllocTextureColumnTables(rw_stopx - start);

		texcoltables = true;

		if (frontsector && backsector && !Tag_Compare(&frontsector->tags, &backsector->tags) && (backsector->ffloors || frontsector->ffloors))
		{
			ffloor_t *rover;
			ffloor_t *r2;
			fixed_t   lowcut, highcut;
			fixed_t lowcutslope, highcutslope;


			fixed_t high1, highslope1, low1, lowslope1, high2, highslope2, low2, lowslope2;

			maskedtexture = true;

			ds_p->thicksidecol = thicksidecol = curtexturecolumntable - rw_x;
			curtexturecolumntable += rw_stopx - rw_x;

			lowcut = max(worldbottom, worldlow) + viewz;
			highcut = min(worldtop, worldhigh) + viewz;
			lowcutslope = max(worldbottomslope, worldlowslope) + viewz;
			highcutslope = min(worldtopslope, worldhighslope) + viewz;

			if (frontsector->ffloors && backsector->ffloors)
			{
				i = 0;
				for (rover = backsector->ffloors; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->fofflags & FOF_RENDERSIDES) || !(rover->fofflags & FOF_EXISTS))
						continue;
					if (!(rover->fofflags & FOF_ALLSIDES) && rover->fofflags & FOF_INVERTSIDES)
						continue;

					if (rover->norender == leveltime)
						continue;

					SLOPEPARAMS(*rover->t_slope, high1, highslope1, *rover->topheight)
					SLOPEPARAMS(*rover->b_slope, low1,  lowslope1,  *rover->bottomheight)

					if ((high1 < lowcut && highslope1 < lowcutslope) || (low1 > highcut && lowslope1 > highcutslope))
						continue;

					for (r2 = frontsector->ffloors; r2; r2 = r2->next)
					{
						if (r2->master == rover->master)  
							break;

						if (!(r2->fofflags & FOF_EXISTS) || !(r2->fofflags & FOF_RENDERSIDES))
							continue;

						if (r2->norender == leveltime)
							continue;

						if (rover->fofflags & FOF_EXTRA)
						{
							if (!(r2->fofflags & FOF_CUTEXTRA))
								continue;

							if (r2->fofflags & FOF_EXTRA && (r2->fofflags & (FOF_TRANSLUCENT|FOF_FOG)) != (rover->fofflags & (FOF_TRANSLUCENT|FOF_FOG)))
								continue;
						}
						else
						{
							if (!(r2->fofflags & FOF_CUTSOLIDS))
								continue;
						}

						SLOPEPARAMS(*r2->t_slope, high2, highslope2, *r2->topheight)
						SLOPEPARAMS(*r2->b_slope, low2,  lowslope2,  *r2->bottomheight)

						if ((high2 < lowcut || highslope2 < lowcutslope) || (low2 > highcut || lowslope2 > highcutslope))
							continue;
						if ((high1 > high2 || highslope1 > highslope2) || (low1 < low2 || lowslope1 < lowslope2))
							continue;

						break;
					}
					if (r2)
						continue;

					ds_p->thicksides[i] = rover;
					i++;
				}

				for (rover = frontsector->ffloors; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->fofflags & FOF_RENDERSIDES) || !(rover->fofflags & FOF_EXISTS))
						continue;
					if (!(rover->fofflags & FOF_ALLSIDES || rover->fofflags & FOF_INVERTSIDES))
						continue;

					if (rover->norender == leveltime)
						continue;

					SLOPEPARAMS(*rover->t_slope, high1, highslope1, *rover->topheight)
					SLOPEPARAMS(*rover->b_slope, low1,  lowslope1,  *rover->bottomheight)

					if ((high1 < lowcut && highslope1 < lowcutslope) || (low1 > highcut && lowslope1 > highcutslope))
						continue;

					for (r2 = backsector->ffloors; r2; r2 = r2->next)
					{
						if (r2->master == rover->master)  
							break;

						if (!(r2->fofflags & FOF_EXISTS) || !(r2->fofflags & FOF_RENDERSIDES))
							continue;

						if (r2->norender == leveltime)
							continue;

						if (rover->fofflags & FOF_EXTRA)
						{
							if (!(r2->fofflags & FOF_CUTEXTRA))
								continue;

							if (r2->fofflags & FOF_EXTRA && (r2->fofflags & (FOF_TRANSLUCENT|FOF_FOG)) != (rover->fofflags & (FOF_TRANSLUCENT|FOF_FOG)))
								continue;
						}
						else
						{
							if (!(r2->fofflags & FOF_CUTSOLIDS))
								continue;
						}

						SLOPEPARAMS(*r2->t_slope, high2, highslope2, *r2->topheight)
						SLOPEPARAMS(*r2->b_slope, low2,  lowslope2,  *r2->bottomheight)
#undef SLOPEPARAMS
						if ((high2 < lowcut || highslope2 < lowcutslope) || (low2 > highcut || lowslope2 > highcutslope))
							continue;
						if ((high1 > high2 || highslope1 > highslope2) || (low1 < low2 || lowslope1 < lowslope2))
							continue;

						break;
					}
					if (r2)
						continue;

					ds_p->thicksides[i] = rover;
					i++;
				}
			}
			else if (backsector->ffloors)
			{
				for (rover = backsector->ffloors, i = 0; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->fofflags & FOF_RENDERSIDES) || !(rover->fofflags & FOF_EXISTS))
						continue;
					if (!(rover->fofflags & FOF_ALLSIDES) && rover->fofflags & FOF_INVERTSIDES)
						continue;
					if (rover->norender == leveltime)
						continue;


					if (      ((P_GetFFloorTopZAt   (rover, segleft .x, segleft .y)) <= worldbottom      + viewz
					        && (P_GetFFloorTopZAt   (rover, segright.x, segright.y)) <= worldbottomslope + viewz)
					        ||((P_GetFFloorBottomZAt(rover, segleft .x, segleft .y)) >= worldtop         + viewz
					        && (P_GetFFloorBottomZAt(rover, segright.x, segright.y)) >= worldtopslope    + viewz))
						continue;

					ds_p->thicksides[i] = rover;
					i++;
				}
			}
			else if (frontsector->ffloors)
			{
				for (rover = frontsector->ffloors, i = 0; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->fofflags & FOF_RENDERSIDES) || !(rover->fofflags & FOF_EXISTS))
						continue;
					if (!(rover->fofflags & FOF_ALLSIDES || rover->fofflags & FOF_INVERTSIDES))
						continue;
					if (rover->norender == leveltime)
						continue;

					if (      (P_GetFFloorTopZAt   (rover, segleft .x, segleft .y) <= worldbottom      + viewz
					        && P_GetFFloorTopZAt   (rover, segright.x, segright.y) <= worldbottomslope + viewz)
					        ||(P_GetFFloorBottomZAt(rover, segleft .x, segleft .y) >= worldtop         + viewz
					        && P_GetFFloorBottomZAt(rover, segright.x, segright.y) >= worldtopslope    + viewz))
						continue;

					if (      (P_GetFFloorTopZAt   (rover, segleft .x, segleft .y) <= worldlow       + viewz
					        && P_GetFFloorTopZAt   (rover, segright.x, segright.y) <= worldlowslope  + viewz)
					        ||(P_GetFFloorBottomZAt(rover, segleft .x, segleft .y) >= worldhigh      + viewz
					        && P_GetFFloorBottomZAt(rover, segright.x, segright.y) >= worldhighslope + viewz))
						continue;

					ds_p->thicksides[i] = rover;
					i++;
				}
			}

			ds_p->numthicksides = numthicksides = i;
		}


		if (sidedef->midtexture > 0 && sidedef->midtexture < numtextures)
		{
			ds_p->maskedtexturecol = maskedtexturecol = curtexturecolumntable - rw_x;
			curtexturecolumntable += rw_stopx - rw_x;

			ds_p->maskedtextureheight = maskedtextureheight = curtexturecolumntable - rw_x;
			curtexturecolumntable += rw_stopx - rw_x;

			maskedtexture = true;

			if (curline->polyseg)
			{

				rw_midtextureslide = rw_midtexturebackslide = 0;
				if (linedef->flags & ML_MIDPEG)
					rw_midtexturemid = rw_midtextureback = max(curline->frontsector->floorheight, curline->backsector->floorheight) - viewz;
				else
					rw_midtexturemid = rw_midtextureback = min(curline->frontsector->ceilingheight, curline->backsector->ceilingheight) - viewz;
			}
			else
			{

				if (linedef->flags & ML_NOSKEW)
				{

					rw_midtextureslide = rw_midtexturebackslide = 0;
					if (linedef->flags & ML_MIDPEG)
						rw_midtexturemid = rw_midtextureback = max(frontsector->floorheight, backsector->floorheight) - viewz;
					else
						rw_midtexturemid = rw_midtextureback = min(frontsector->ceilingheight, backsector->ceilingheight) - viewz;

				}
				else if (linedef->flags & ML_MIDPEG)
				{
					rw_midtexturemid = worldbottom;
					rw_midtextureslide = floorfrontslide;
					rw_midtextureback = worldlow;
					rw_midtexturebackslide = floorbackslide;
				}
				else
				{
					rw_midtexturemid = worldtop;
					rw_midtextureslide = ceilingfrontslide;
					rw_midtextureback = worldhigh;
					rw_midtexturebackslide = ceilingbackslide;
				}
			}

			rw_midtexturemid = FixedMul(rw_midtexturemid, abs(rw_midtexturescaley));
			rw_midtextureback = FixedMul(rw_midtextureback, abs(rw_midtexturescaley));

			rw_midtextureslide = FixedMul(rw_midtextureslide, abs(rw_midtexturescaley));
			rw_midtexturebackslide = FixedMul(rw_midtexturebackslide, abs(rw_midtexturescaley));

			rw_midtexturemid += sidedef->rowoffset + sidedef->offsety_mid;
			rw_midtextureback += sidedef->rowoffset + sidedef->offsety_mid;
		}
	}


	segtextured = midtexture || toptexture || bottomtexture || maskedtexture || (numthicksides > 0);

	if (segtextured)
	{
		fixed_t sideoffset = sidedef->textureoffset;

		offsetangle = rw_normalangle-rw_angle1;

		if (offsetangle > ANGLE_180)
			offsetangle = -(signed)offsetangle;

		if (offsetangle > ANGLE_90)
			offsetangle = ANGLE_90;

		sineval = R_InterpolatedFineSine(offsetangle);
		rw_offset = FixedMul(hyp, sineval);


		if (longboi)
		{
			INT64 dx = (curline->v2->x)-(curline->v1->x);
			INT64 dy = (curline->v2->y)-(curline->v1->y);
			INT64 vdx = viewx-(curline->v1->x);
			INT64 vdy = viewy-(curline->v1->y);
			rw_offset = ((dx*vdx-dy*vdy))/(curline->length);
		}

		if (rw_normalangle-rw_angle1 < ANGLE_180)
			rw_offset = -rw_offset;

		rw_offset += curline->offset;
		rw_centerangle = ANGLE_90 + viewangle - rw_normalangle;

		rw_offset_top = sideoffset + sidedef->offsetx_top;
		rw_offset_mid = sideoffset + sidedef->offsetx_mid;
		rw_offset_bottom = sideoffset + sidedef->offsetx_bottom;

		rw_offsetx = rw_offset_mid;
		if (rw_midtexturescalex < 0)
			rw_offsetx = -rw_offsetx;

		if (numthicksides)
			ds_p->offsetx = rw_offsetx;





		lightnum = R_SideLightLevel(curline->sidedef, frontsector->lightlevel) >> LIGHTSEGSHIFT;

		if (curline->v1->y == curline->v2->y)
			lightnum--;
		else if (curline->v1->x == curline->v2->x)
			lightnum++;

		if (lightnum < 0)
			walllights = scalelight[0];
		else if (lightnum >= LIGHTLEVELS)
			walllights = scalelight[LIGHTLEVELS - 1];
		else
			walllights = scalelight[lightnum];
	}

	if (maskedtexture)
	{
		ds_p->invscale = invscale = curtexturecolumntable - rw_x;
		curtexturecolumntable += rw_stopx - rw_x;
	}




	if (frontsector->heightsec == -1)
	{
		if (frontsector->floorpic != skyflatnum && P_GetSectorFloorZAt(frontsector, viewx, viewy) >= viewz)
		{

			markfloor = false;
		}

		if (frontsector->ceilingpic != skyflatnum && P_GetSectorCeilingZAt(frontsector, viewx, viewy) <= viewz)
		{

			markceiling = false;
		}
	}


	worldtop >>= 4;
	worldbottom >>= 4;
	worldtopslope >>= 4;
	worldbottomslope >>= 4;

	if (horizonline) {  
		topstep = bottomstep = 0;
		topfrac = bottomfrac = (centeryfrac>>4);
		topfrac++;  
	} else {
		topstep = -FixedMul (rw_scalestep, worldtop);
		topfrac = (centeryfrac>>4) - FixedMul (worldtop, rw_scale);

		bottomstep = -FixedMul (rw_scalestep,worldbottom);
		bottomfrac = (centeryfrac>>4) - FixedMul (worldbottom, rw_scale);

		if (frontsector->c_slope) {
			fixed_t topfracend = (centeryfrac>>4) - FixedMul (worldtopslope, ds_p->scale2);
			topstep = (topfracend-topfrac)/(range);
		}
		if (frontsector->f_slope) {
			fixed_t bottomfracend = (centeryfrac>>4) - FixedMul (worldbottomslope, ds_p->scale2);
			bottomstep = (bottomfracend-bottomfrac)/(range);
		}
	}

	dc_numlights = 0;

	if (frontsector->numlights)
	{
		dc_numlights = frontsector->numlights;
		if (dc_numlights > dc_maxlights)
		{
			dc_maxlights = dc_numlights;
			dc_lightlist = Z_Realloc(dc_lightlist, sizeof (*dc_lightlist) * dc_maxlights, PU_STATIC, NULL);
		}

		for (i = p = 0; i < dc_numlights; i++)
		{
			fixed_t leftheight, rightheight;

			light = &frontsector->lightlist[i];
			rlight = &dc_lightlist[p];

			leftheight  = P_GetLightZAt(light,  segleft.x,  segleft.y);
			rightheight = P_GetLightZAt(light, segright.x, segright.y);

			if (light->slope)

				frontsector->hasslope = true;

			leftheight -= viewz;
			rightheight -= viewz;

			leftheight >>= 4;
			rightheight >>= 4;

			if (i != 0)
			{
				if (leftheight < worldbottom && rightheight < worldbottomslope)
					continue;

				if (leftheight > worldtop && rightheight > worldtopslope && i+1 < dc_numlights && frontsector->lightlist[i+1].height > frontsector->ceilingheight)
					continue;
			}

			rlight->height = (centeryfrac>>4) - FixedMul(leftheight, rw_scale);
			rlight->heightstep = (centeryfrac>>4) - FixedMul(rightheight, ds_p->scale2);
			rlight->heightstep = (rlight->heightstep-rlight->height)/(range);
			rlight->flags = light->flags;

			if (light->caster && light->caster->fofflags & FOF_CUTSOLIDS)
			{
				leftheight  = P_GetFFloorBottomZAt(light->caster,  segleft.x,  segleft.y);
				rightheight = P_GetFFloorBottomZAt(light->caster, segright.x, segright.y);

				if (*light->caster->b_slope)

					frontsector->hasslope = true;

				leftheight  -= viewz;
				rightheight -= viewz;

				leftheight  >>= 4;
				rightheight >>= 4;

				rlight->botheight = (centeryfrac>>4) - FixedMul(leftheight, rw_scale);
				rlight->botheightstep = (centeryfrac>>4) - FixedMul(rightheight, ds_p->scale2);
				rlight->botheightstep = (rlight->botheightstep-rlight->botheight)/(range);

			}

			rlight->lightlevel = *light->lightlevel;
			rlight->extra_colormap = *light->extra_colormap;
			p++;
		}

		dc_numlights = p;
	}

	if (numffloors)
	{
		for (i = 0; i < numffloors; i++)
		{
			ffloor[i].f_pos >>= 4;
			ffloor[i].f_pos_slope >>= 4;
			if (horizonline)  
			{
				ffloor[i].f_step = 0;
				ffloor[i].f_frac = (centeryfrac>>4);
				ffloor[i].f_frac++;  
			}
			else
			{
				ffloor[i].f_frac = (centeryfrac>>4) - FixedMul(ffloor[i].f_pos, rw_scale);
				ffloor[i].f_step = ((centeryfrac>>4) - FixedMul(ffloor[i].f_pos_slope, ds_p->scale2) - ffloor[i].f_frac)/(range);
			}
		}
	}

	if (backsector)
	{
		worldhigh >>= 4;
		worldlow >>= 4;
		worldhighslope >>= 4;
		worldlowslope >>= 4;

		if (toptexture)
		{
			pixhigh = (centeryfrac>>4) - FixedMul (worldhigh, rw_scale);
			pixhighstep = -FixedMul (rw_scalestep,worldhigh);

			if (backsector->c_slope) {
				fixed_t topfracend = (centeryfrac>>4) - FixedMul (worldhighslope, ds_p->scale2);
				pixhighstep = (topfracend-pixhigh)/(range);
			}
		}

		if (bottomtexture)
		{
			pixlow = (centeryfrac>>4) - FixedMul (worldlow, rw_scale);
			pixlowstep = -FixedMul (rw_scalestep,worldlow);
			if (backsector->f_slope) {
				fixed_t bottomfracend = (centeryfrac>>4) - FixedMul (worldlowslope, ds_p->scale2);
				pixlowstep = (bottomfracend-pixlow)/(range);
			}
		}

		{
			ffloor_t * rover;
			fixed_t roverleft, roverright;
			fixed_t planevistest;
			i = 0;

			if (backsector->ffloors)
			{
				for (rover = backsector->ffloors; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_RENDERPLANES))
						continue;
					if (rover->norender == leveltime)
						continue;


					if (*rover->b_slope || *rover->t_slope)
						backsector->hasslope = true;

					roverleft    = P_GetFFloorBottomZAt(rover, segleft .x, segleft .y) - viewz;
					roverright   = P_GetFFloorBottomZAt(rover, segright.x, segright.y) - viewz;
					planevistest = P_GetFFloorBottomZAt(rover, viewx, viewy);

					if ((roverleft>>4 <= worldhigh || roverright>>4 <= worldhighslope) &&
					    (roverleft>>4 >= worldlow || roverright>>4 >= worldlowslope) &&
					    ((viewz < planevistest && (rover->fofflags & FOF_BOTHPLANES || !(rover->fofflags & FOF_INVERTPLANES))) ||
					     (viewz > planevistest && (rover->fofflags & FOF_BOTHPLANES || rover->fofflags & FOF_INVERTPLANES))))
					{

						ffloor[i].b_pos = roverleft;
						ffloor[i].b_pos_slope = roverright;
						ffloor[i].b_pos >>= 4;
						ffloor[i].b_pos_slope >>= 4;
						ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos, rw_scale);
						ffloor[i].b_step = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos_slope, ds_p->scale2);
						ffloor[i].b_step = (ffloor[i].b_step-ffloor[i].b_frac)/(range);
						i++;
					}

					if (i >= MAXFFLOORS)
						break;

					roverleft    = P_GetFFloorTopZAt(rover, segleft .x, segleft .y) - viewz;
					roverright   = P_GetFFloorTopZAt(rover, segright.x, segright.y) - viewz;
					planevistest = P_GetFFloorTopZAt(rover, viewx, viewy);

					if ((roverleft>>4 <= worldhigh || roverright>>4 <= worldhighslope) &&
					    (roverleft>>4 >= worldlow || roverright>>4 >= worldlowslope) &&
					    ((viewz > planevistest && (rover->fofflags & FOF_BOTHPLANES || !(rover->fofflags & FOF_INVERTPLANES))) ||
					     (viewz < planevistest && (rover->fofflags & FOF_BOTHPLANES || rover->fofflags & FOF_INVERTPLANES))))
					{

						ffloor[i].b_pos = roverleft;
						ffloor[i].b_pos_slope = roverright;
						ffloor[i].b_pos >>= 4;
						ffloor[i].b_pos_slope >>= 4;
						ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos, rw_scale);
						ffloor[i].b_step = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos_slope, ds_p->scale2);
						ffloor[i].b_step = (ffloor[i].b_step-ffloor[i].b_frac)/(range);
						i++;
					}
				}
			}
			else if (frontsector && frontsector->ffloors)
			{
				for (rover = frontsector->ffloors; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_RENDERPLANES))
						continue;
					if (rover->norender == leveltime)
						continue;


					if (*rover->b_slope || *rover->t_slope)
						frontsector->hasslope = true;

					roverleft  = P_GetFFloorBottomZAt(rover, segleft .x, segleft .y) - viewz;
					roverright = P_GetFFloorBottomZAt(rover, segright.x, segright.y) - viewz;
					planevistest = P_GetFFloorBottomZAt(rover, viewx, viewy);

					if ((roverleft>>4 <= worldhigh || roverright>>4 <= worldhighslope) &&
					    (roverleft>>4 >= worldlow || roverright>>4 >= worldlowslope) &&
					    ((viewz < planevistest && (rover->fofflags & FOF_BOTHPLANES || !(rover->fofflags & FOF_INVERTPLANES))) ||
					     (viewz > planevistest && (rover->fofflags & FOF_BOTHPLANES || rover->fofflags & FOF_INVERTPLANES))))
					{

						ffloor[i].b_pos = roverleft;
						ffloor[i].b_pos_slope = roverright;
						ffloor[i].b_pos >>= 4;
						ffloor[i].b_pos_slope >>= 4;
						ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos, rw_scale);
						ffloor[i].b_step = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos_slope, ds_p->scale2);
						ffloor[i].b_step = (ffloor[i].b_step-ffloor[i].b_frac)/(range);
						i++;
					}

					if (i >= MAXFFLOORS)
						break;

					roverleft  = P_GetFFloorTopZAt(rover, segleft .x, segleft .y) - viewz;
					roverright = P_GetFFloorTopZAt(rover, segright.x, segright.y) - viewz;
					planevistest = P_GetFFloorTopZAt(rover, viewx, viewy);

					if ((roverleft>>4 <= worldhigh || roverright>>4 <= worldhighslope) &&
					    (roverleft>>4 >= worldlow || roverright>>4 >= worldlowslope) &&
					    ((viewz > planevistest && (rover->fofflags & FOF_BOTHPLANES || !(rover->fofflags & FOF_INVERTPLANES))) ||
					     (viewz < planevistest && (rover->fofflags & FOF_BOTHPLANES || rover->fofflags & FOF_INVERTPLANES))))
					{

						ffloor[i].b_pos = roverleft;
						ffloor[i].b_pos_slope = roverright;
						ffloor[i].b_pos >>= 4;
						ffloor[i].b_pos_slope >>= 4;
						ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos, rw_scale);
						ffloor[i].b_step = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos_slope, ds_p->scale2);
						ffloor[i].b_step = (ffloor[i].b_step-ffloor[i].b_frac)/(range);
						i++;
					}
				}
			}
			if (curline->polyseg && frontsector && (curline->polyseg->flags & POF_RENDERPLANES))
			{
				while (i < numffloors && ffloor[i].polyobj != curline->polyseg) i++;
				if (i < numffloors && backsector->floorheight <= frontsector->ceilingheight &&
					backsector->floorheight >= frontsector->floorheight &&
					(viewz < backsector->floorheight))
				{
					if (ffloor[i].plane->minx > ds_p->x1)
						ffloor[i].plane->minx = ds_p->x1;

					if (ffloor[i].plane->maxx < ds_p->x2)
						ffloor[i].plane->maxx = ds_p->x2;

					ffloor[i].slope = NULL;
					ffloor[i].b_pos = backsector->floorheight;
					ffloor[i].b_pos = (ffloor[i].b_pos - viewz) >> 4;
					ffloor[i].b_step = FixedMul(-rw_scalestep, ffloor[i].b_pos);
					ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos, rw_scale);
					i++;
				}
				if (i < numffloors && backsector->ceilingheight >= frontsector->floorheight &&
					backsector->ceilingheight <= frontsector->ceilingheight &&
					(viewz > backsector->ceilingheight))
				{
					if (ffloor[i].plane->minx > ds_p->x1)
						ffloor[i].plane->minx = ds_p->x1;

					if (ffloor[i].plane->maxx < ds_p->x2)
						ffloor[i].plane->maxx = ds_p->x2;

					ffloor[i].slope = NULL;
					ffloor[i].b_pos = backsector->ceilingheight;
					ffloor[i].b_pos = (ffloor[i].b_pos - viewz) >> 4;
					ffloor[i].b_step = FixedMul(-rw_scalestep, ffloor[i].b_pos);
					ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos, rw_scale);
					i++;
				}
			}

			numbackffloors = i;
		}
	}


	if (markceiling)
	{
		if (ceilingplane)  
			ceilingplane = R_CheckPlane (ceilingplane, rw_x, rw_stopx-1);
		else
			markceiling = false;



		if (curline->polyseg && !curline->sidedef->toptexture)
			markceiling = false;
	}


	if (markfloor)
	{
		if (floorplane)  
			floorplane = R_CheckPlane (floorplane, rw_x, rw_stopx-1);
		else
			markfloor = false;



		if (curline->polyseg && !curline->sidedef->bottomtexture)
			markfloor = false;
	}

	ds_p->numffloorplanes = 0;
	if (numffloors)
	{
		if (!firstseg)
		{
			ds_p->numffloorplanes = numffloors;

			for (i = 0; i < numffloors; i++)
			{
				ds_p->ffloorplanes[i] = ffloor[i].plane =
					R_CheckPlane(ffloor[i].plane, rw_x, rw_stopx - 1);
			}

			firstseg = ds_p;
		}
		else
		{
			for (i = 0; i < numffloors; i++)
				R_ExpandPlane(ffloor[i].plane, rw_x, rw_stopx - 1);
		}

		if (curline->polyseg)
		{
			for (i = 0; i < numffloors; i++)
			{
				if (!ffloor[i].polyobj || ffloor[i].polyobj != curline->polyseg)
					continue;
				if (ffloor[i].plane->minx > rw_x)
					ffloor[i].plane->minx = rw_x;

				if (ffloor[i].plane->maxx < rw_stopx - 1)
					ffloor[i].plane->maxx = rw_stopx - 1;
			}
		}
	}

	rw_silhouette = &(ds_p->silhouette);
	rw_tsilheight = &(ds_p->tsilheight);
	rw_bsilheight = &(ds_p->bsilheight);

	R_RenderSegLoop();
	colfunc = colfuncs[BASEDRAWFUNC];

	if (portalline)  
		ds_p->portalpass = portalrender+1;
	else
		ds_p->portalpass = 0;


	if (maskedtexture || (ds_p->silhouette & (SIL_TOP | SIL_BOTTOM)))
	{
		R_AllocClippingTables(rw_stopx - start);

		if (((ds_p->silhouette & SIL_TOP) || maskedtexture) && !ds_p->sprtopclip)
		{
			M_Memcpy(lastopening, ceilingclip + start, 2*(rw_stopx - start));
			ds_p->sprtopclip = lastopening - start;
			lastopening += rw_stopx - start;
		}

		if (((ds_p->silhouette & SIL_BOTTOM) || maskedtexture) && !ds_p->sprbottomclip)
		{
			M_Memcpy(lastopening, floorclip + start, 2*(rw_stopx - start));
			ds_p->sprbottomclip = lastopening - start;
			lastopening += rw_stopx - start;
		}
	}

	if (maskedtexture && !(ds_p->silhouette & SIL_TOP))
	{
		ds_p->silhouette |= SIL_TOP;
		ds_p->tsilheight = (sidedef->midtexture > 0 && sidedef->midtexture < numtextures) ? INT32_MIN : INT32_MAX;
	}
	if (maskedtexture && !(ds_p->silhouette & SIL_BOTTOM))
	{
		ds_p->silhouette |= SIL_BOTTOM;
		ds_p->bsilheight = (sidedef->midtexture > 0 && sidedef->midtexture < numtextures) ? INT32_MAX : INT32_MIN;
	}
	ds_p++;
}
