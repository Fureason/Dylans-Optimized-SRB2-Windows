// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include <math.h>

#include "../doomstat.h"

#ifdef HWRENDER
#include "hw_glob.h"
#include "hw_light.h"
#include "hw_drv.h"
#include "hw_batching.h"
#include "hw_md2.h"
#include "hw_clip.h"

#include "../i_video.h"
#include "../v_video.h"
#include "../p_local.h"
#include "../p_setup.h"
#include "../r_fps.h"
#include "../r_local.h"
#include "../r_patch.h"
#include "../r_picformats.h"
#include "../r_textures.h"
#include "../r_bsp.h"
#include "../netcode/d_clisrv.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../r_splats.h"
#include "../g_game.h"
#include "../st_stuff.h"
#include "../i_system.h"
#include "../m_cheat.h"
#include "../f_finale.h"
#include "../r_things.h"  
#include "../r_translation.h"
#include "../d_main.h"
#include "../p_slopes.h"
#include "../p_spec.h"




struct hwdriver_s hwdriver;





static void HWR_AddSprites(sector_t *sec);
static void HWR_ProjectSprite(mobj_t *thing);
static void HWR_ProjectPrecipitationSprite(precipmobj_t *thing);
static void HWR_ProjectBoundingBox(mobj_t *thing);

void HWR_AddTransparentFloor(levelflat_t *levelflat, extrasubsector_t *xsub, boolean isceiling, fixed_t fixedheight, INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, boolean fogplane, boolean chromakeyed, extracolormap_t *planecolormap);
void HWR_AddTransparentPolyobjectFloor(levelflat_t *levelflat, polyobj_t *polysector, boolean isceiling, fixed_t fixedheight,
                             INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, extracolormap_t *planecolormap);

static boolean drawsky = true;
static boolean hwr_portal_pass = false;
static boolean hwr_portal_background_ready = false;
static line_t *hwr_portal_source_line = NULL;
static line_t *hwr_portal_destination_line = NULL;
static line_t *hwr_nested_portal_source_line = NULL;
static sector_t *hwr_portal_cull_sector = NULL;
static fixed_t hwr_bsp_viewx, hwr_bsp_viewy;
static line_t **hwr_line_portals = NULL;
static line_t **hwr_line_portal_destinations = NULL;
static UINT8 *hwr_line_portal_sight_cache = NULL;
static boolean hwr_line_portal_sight_view_valid;
static tic_t hwr_line_portal_sight_tic;
static fixed_t hwr_line_portal_sight_x, hwr_line_portal_sight_y,
	hwr_line_portal_sight_z;
static angle_t hwr_line_portal_sight_angle, hwr_line_portal_sight_aiming;
static INT32 hwr_line_portal_sight_viewnumber;
static size_t hwr_line_portal_count = 0;
static boolean hwr_line_portal_cache_ready = false;
static line_t **hwr_visible_main_portals = NULL;
static size_t hwr_visible_main_portal_count = 0;
static boolean hwr_collect_main_portals = false;
extern consvar_t cv_glvisuportals;

static boolean HWR_QueueMainPortal(line_t *line, angle_t visiblestart,
	angle_t visibleend);
static boolean HWR_FindLinePortalDestination(line_t *start, line_t **result);

static inline boolean HWR_VisuportalsActive(void)
{
	return cv_glvisuportals.value && !hwr_portal_pass;
}

static inline boolean HWR_PortalSurfaceActive(void)
{
	return HWR_VisuportalsActive() && hwr_portal_background_ready;
}

static double HWR_FixedDelta(fixed_t a, fixed_t b)
{
	return (double)((INT64)a - (INT64)b) / FRACUNIT;
}

                                                                            
                                                                           
                                                              
static boolean HWR_PointBehindPortalLine(fixed_t x, fixed_t y, line_t *line)
{
	const double dx = HWR_FixedDelta(line->v2->x, line->v1->x);
	const double dy = HWR_FixedDelta(line->v2->y, line->v1->y);
	const double px = HWR_FixedDelta(x, line->v1->x);
	const double py = HWR_FixedDelta(y, line->v1->y);

	return py * dx - dy * px > 0.000001;
}

static double HWR_FloatPortalLineSide(float x, float y, line_t *line)
{
	const double dx = HWR_FixedDelta(line->v2->x, line->v1->x);
	const double dy = HWR_FixedDelta(line->v2->y, line->v1->y);
	const double px = (double)x - FixedToFloat(line->v1->x);
	const double py = (double)y - FixedToFloat(line->v1->y);

	return py * dx - dy * px;
}

                                                                       
                                                                            
                                                                        
                                                              
static boolean HWR_PointHiddenByPortalLine(fixed_t x, fixed_t y, line_t *line)
{
	double rx, ry, sx, sy, qx, qy, denominator, rayfrac, linefrac;

	if (!line || P_PointOnLineSide(viewx, viewy, line) != 0
	|| P_PointOnLineSide(x, y, line) == 0)
		return false;

	rx = HWR_FixedDelta(x, viewx);
	ry = HWR_FixedDelta(y, viewy);
	sx = HWR_FixedDelta(line->v2->x, line->v1->x);
	sy = HWR_FixedDelta(line->v2->y, line->v1->y);
	qx = HWR_FixedDelta(line->v1->x, viewx);
	qy = HWR_FixedDelta(line->v1->y, viewy);
	denominator = rx * sy - ry * sx;
	if (fabs(denominator) < 0.000001)
		return false;

	rayfrac = (qx * sy - qy * sx) / denominator;
	linefrac = (qx * ry - qy * rx) / denominator;
	return rayfrac > 0.0 && rayfrac < 1.0
		&& linefrac >= 0.0 && linefrac <= 1.0;
}

static boolean HWR_PointHiddenByLinePortal(fixed_t x, fixed_t y)
{
	size_t i;

	                                                                      
                                                                          
	if (hwr_portal_pass && hwr_portal_destination_line
	&& HWR_PointBehindPortalLine(x, y, hwr_portal_destination_line))
		return true;

	if (!HWR_PortalSurfaceActive())
		return false;
	for (i = 0; i < hwr_visible_main_portal_count; i++)
		if (HWR_PointHiddenByPortalLine(x, y, hwr_visible_main_portals[i]))
			return true;
	return false;
}

static boolean HWR_IsVisibleMainPortal(line_t *line)
{
	size_t i;

	for (i = 0; i < hwr_visible_main_portal_count; i++)
		if (hwr_visible_main_portals[i] == line)
			return true;
	return false;
}

static inline boolean HWR_FloorPortalSurfaceActive(sector_t *sector)
{
	(void)sector;
	return false;
}

static inline boolean HWR_CeilingPortalSurfaceActive(sector_t *sector)
{
	(void)sector;
	return false;
}





static seg_t *gl_curline;
static side_t *gl_sidedef;
static line_t *gl_linedef;
static sector_t *gl_frontsector;
static sector_t *gl_backsector;


ps_metric_t ps_hw_skyboxtime = {0};
ps_metric_t ps_hw_portaltime = {0};
ps_metric_t ps_hw_numportals = {0};
ps_metric_t ps_hw_nodesorttime = {0};
ps_metric_t ps_hw_nodedrawtime = {0};
ps_metric_t ps_hw_spritesorttime = {0};
ps_metric_t ps_hw_spritedrawtime = {0};
ps_metric_t ps_hw_textureuploadtime = {0};
ps_metric_t ps_hw_textureuploads = {0};
ps_metric_t ps_hw_batchedsprites = {0};


ps_metric_t ps_hw_numpolys = {0};
ps_metric_t ps_hw_numverts = {0};
ps_metric_t ps_hw_numcalls = {0};
ps_metric_t ps_hw_numshaders = {0};
ps_metric_t ps_hw_numtextures = {0};
ps_metric_t ps_hw_numpolyflags = {0};
ps_metric_t ps_hw_numcolors = {0};
ps_metric_t ps_hw_batchsorttime = {0};
ps_metric_t ps_hw_batchdrawtime = {0};

boolean gl_init = false;
boolean gl_maploaded = false;
boolean gl_sessioncommandsadded = false;

boolean gl_shadersavailable = false;


static boolean gl_palette_rendering_state = false;





FTransform atransform;

static float gl_viewx, gl_viewy, gl_viewz;
float gl_viewsin, gl_viewcos;


static float gl_viewludsin, gl_viewludcos;
static float gl_fovlud;

static angle_t gl_aimingangle;
static void HWR_SetTransformAiming(FTransform *trans, player_t *player, boolean skybox);

                                                                           
                                                                             
                                                                             
                                                                            
                                                                               
                           
static float HWR_AngleToDegrees(angle_t angle)
{
	return (float)((double)angle * (360.0 / 4294967296.0));
}

                                                                              
                                                                            
                                                                              
#define HWR_ANGLE_CACHE_SIZE 4096
typedef struct
{
	boolean valid;
	fixed_t pointx, pointy;
	fixed_t eyex, eyey;
	angle_t angle;
} hwranglecache_t;
static hwranglecache_t hwr_angle_cache[HWR_ANGLE_CACHE_SIZE];

static angle_t HWR_PointToAngleCached(fixed_t x, fixed_t y)
{
	UINT32 hash = (UINT32)x * 0x9e3779b1u;
	hwranglecache_t *entry;

	hash ^= (UINT32)y * 0x85ebca6bu;
	hash ^= (UINT32)viewx * 0xc2b2ae35u;
	hash ^= (UINT32)viewy * 0x27d4eb2fu;
	hash ^= hash >> 16;
	entry = &hwr_angle_cache[hash & (HWR_ANGLE_CACHE_SIZE - 1)];
	if (entry->valid && entry->pointx == x && entry->pointy == y
	&& entry->eyex == viewx && entry->eyey == viewy)
		return entry->angle;

	entry->valid = true;
	entry->pointx = x;
	entry->pointy = y;
	entry->eyex = viewx;
	entry->eyey = viewy;
	entry->angle = R_PointToAngle64(x, y);
	return entry->angle;
}






boolean HWR_UseShader(void)
{
	return (cv_glshaders.value && gl_shadersavailable);
}

static boolean HWR_IsWireframeMode(void)
{
	return (cv_glwireframe.value && cv_debug);
}

void HWR_Lighting(FSurfaceInfo *Surface, INT32 light_level, extracolormap_t *colormap)
{
	RGBA_t poly_color, tint_color, fade_color;

	poly_color.rgba = 0xFFFFFFFF;
	tint_color.rgba = (colormap != NULL) ? (UINT32)colormap->rgba : 0x00000000;
	fade_color.rgba = (colormap != NULL) ? (UINT32)colormap->fadergba : 0xFF000000;



	light_level = min(max(light_level, 0), 255);


	if (!HWR_UseShader())
	{

		float tint_alpha, fade_alpha;
		float red, green, blue;

		red = (float)poly_color.s.red;
		green = (float)poly_color.s.green;
		blue = (float)poly_color.s.blue;


		tint_alpha = (float)(sqrt((float)tint_color.s.alpha / 10.2) * 48) / 255.0f;





		fade_alpha = (float)(sqrt(255-light_level) * 12) / 255.0f;


		tint_alpha = min(max(tint_alpha, 0.0f), 1.0f);
		fade_alpha = min(max(fade_alpha, 0.0f), 1.0f);

		red = (tint_color.s.red * tint_alpha) + (red * (1.0f - tint_alpha));
		green = (tint_color.s.green * tint_alpha) + (green * (1.0f - tint_alpha));
		blue = (tint_color.s.blue * tint_alpha) + (blue * (1.0f - tint_alpha));

		red = (fade_color.s.red * fade_alpha) + (red * (1.0f - fade_alpha));
		green = (fade_color.s.green * fade_alpha) + (green * (1.0f - fade_alpha));
		blue = (fade_color.s.blue * fade_alpha) + (blue * (1.0f - fade_alpha));

		poly_color.s.red = (UINT8)red;
		poly_color.s.green = (UINT8)green;
		poly_color.s.blue = (UINT8)blue;
	}

	V_CubeApply(&tint_color.s.red, &tint_color.s.green, &tint_color.s.blue);
	V_CubeApply(&fade_color.s.red, &fade_color.s.green, &fade_color.s.blue);
	Surface->PolyColor.rgba = poly_color.rgba;
	Surface->TintColor.rgba = tint_color.rgba;
	Surface->FadeColor.rgba = fade_color.rgba;
	Surface->LightInfo.light_level = light_level;
	Surface->LightInfo.fade_start = (colormap != NULL) ? colormap->fadestart : 0;
	Surface->LightInfo.fade_end = (colormap != NULL) ? colormap->fadeend : 31;

	if (HWR_ShouldUsePaletteRendering())
		Surface->LightTableId = HWR_GetLightTableID(colormap);
	else
		Surface->LightTableId = 0;
}

UINT8 HWR_FogBlockAlpha(INT32 light, extracolormap_t *colormap)  
{
	RGBA_t realcolor, surfcolor;
	INT32 alpha;

	realcolor.rgba = (colormap != NULL) ? colormap->rgba : 0x00000000;

	if (cv_glshaders.value && gl_shadersavailable)
	{
		surfcolor.s.alpha = (255 - light);
	}
	else
	{
		light = light - (255 - light);


		if (light < 0)
			light = 0;
		else if (light > 255)
			light = 255;

		alpha = (realcolor.s.alpha*255)/25;


		surfcolor.s.alpha = (alpha*light) / (2*256) + 255-light;
	}

	return surfcolor.s.alpha;
}

static FUINT HWR_CalcWallLight(FUINT lightnum, fixed_t v1x, fixed_t v1y, fixed_t v2x, fixed_t v2y)
{
	INT16 finallight = lightnum;

	if (cv_glfakecontrast.value != 0)
	{
		const UINT8 contrast = 8;
		fixed_t extralight = 0;

		if (cv_glfakecontrast.value == 2)  
		{
			extralight = (-(contrast<<FRACBITS) +
			FixedDiv(AngleFixed(R_PointToAngle2(0, 0,
				abs(v1x - v2x),
				abs(v1y - v2y))), 90<<FRACBITS)
			* (contrast * 2)) >> FRACBITS;
		}
		else
		{
			if (v1y == v2y)
				extralight = -contrast;
			else if (v1x == v2x)
				extralight = contrast;
		}

		if (extralight != 0)
		{
			finallight += extralight;

			if (finallight < 0)
				finallight = 0;
			if (finallight > 255)
				finallight = 255;
		}
	}

	return (FUINT)finallight;
}

static FUINT HWR_CalcSlopeLight(FUINT lightnum, angle_t dir, fixed_t delta)
{
	INT16 finallight = lightnum;

	if (cv_glfakecontrast.value != 0 && cv_glslopecontrast.value != 0)
	{
		const UINT8 contrast = 8;
		fixed_t extralight = 0;

		if (cv_glfakecontrast.value == 2)  
		{
			fixed_t dirmul = abs(FixedDiv(AngleFixed(dir) - (180<<FRACBITS), 180<<FRACBITS));

			extralight = -(contrast<<FRACBITS) + (dirmul * (contrast * 2));

			extralight = FixedMul(extralight, delta*4) >> FRACBITS;
		}
		else
		{
			dir = ((dir + ANGLE_45) / ANGLE_90) * ANGLE_90;

			if (dir == ANGLE_180)
				extralight = -contrast;
			else if (dir == 0)
				extralight = contrast;

			if (delta >= FRACUNIT/2)
				extralight *= 2;
		}

		if (extralight != 0)
		{
			finallight += extralight;

			if (finallight < 0)
				finallight = 0;
			if (finallight > 255)
				finallight = 255;
		}
	}

	return (FUINT)finallight;
}

static UINT8 HWR_SideLightLevel(side_t *side, INT16 base_lightlevel)
{
	return max(0, min(255, side->light +
		((side->lightabsolute) ? 0 : base_lightlevel)));
}

                                       
                                                                   
 
                                         
                                                                                
 

                                                                   
 
                                         
                                                                                
 

                                                                      
 
                                            
                                                                                   
 
  

static UINT8 HWR_FloorLightLevel(sector_t *sector, INT16 base_lightlevel)
{
	return max(0, min(255, sector->floorlightlevel +
		((sector->floorlightabsolute) ? 0 : base_lightlevel)));
}

static UINT8 HWR_CeilingLightLevel(sector_t *sector, INT16 base_lightlevel)
{
	return max(0, min(255, sector->ceilinglightlevel +
		((sector->ceilinglightabsolute) ? 0 : base_lightlevel)));
}




#if 0  
static double HWR_PlaneVertexCross(const FOutVector *a, const FOutVector *b,
	const FOutVector *c)
{
	return ((double)b->x - a->x) * ((double)c->z - a->z)
		- ((double)b->z - a->z) * ((double)c->x - a->x);
}

static boolean HWR_PointStrictlyInsidePlaneTriangle(const FOutVector *point,
	const FOutVector *a, const FOutVector *b, const FOutVector *c,
	double orientation)
{
	const double epsilon = 1.0E-9;
	return orientation * HWR_PlaneVertexCross(a, b, point) > epsilon
		&& orientation * HWR_PlaneVertexCross(b, c, point) > epsilon
		&& orientation * HWR_PlaneVertexCross(c, a, point) > epsilon;
}

                                                                          
                                                                            
                                                                         
                                                                           
                                                                            
                                                                          
static void HWR_ProcessTriangulatedPlane(FSurfaceInfo *surf,
	const FOutVector *vertices, size_t numvertices, FBITFIELD polyflags,
	INT32 shader)
{
	static size_t *indices = NULL;
	static size_t numallocedindices = 0;
	size_t count = 0;
	size_t i;
	double area = 0.0;
	double orientation;
	FOutVector triangle[3];

	if (numvertices < 3)
		return;

	if (!indices || numvertices > numallocedindices)
	{
		numallocedindices = numvertices;
		Z_Free(indices);
		Z_Malloc(numallocedindices * sizeof (*indices), PU_LEVEL, &indices);
	}

	                                                                          
                                                                    
	for (i = 0; i < numvertices; i++)
	{
		if (count)
		{
			const FOutVector *previous = &vertices[indices[count - 1]];
			const double dx = (double)vertices[i].x - previous->x;
			const double dz = (double)vertices[i].z - previous->z;
			if (dx*dx + dz*dz <= 1.0E-12)
				continue;
		}
		indices[count++] = i;
	}
	if (count > 1)
	{
		const FOutVector *first = &vertices[indices[0]];
		const FOutVector *last = &vertices[indices[count - 1]];
		const double dx = (double)first->x - last->x;
		const double dz = (double)first->z - last->z;
		if (dx*dx + dz*dz <= 1.0E-12)
			count--;
	}
	if (count < 3)
		return;

	for (i = 0; i < count; i++)
	{
		const FOutVector *a = &vertices[indices[i]];
		const FOutVector *b = &vertices[indices[(i + 1) % count]];
		area += (double)a->x * b->z - (double)b->x * a->z;
	}
	if (fabs(area) <= 1.0E-8)
		return;
	orientation = area > 0.0 ? 1.0 : -1.0;

	while (count > 3)
	{
		boolean clipped = false;

		for (i = 0; i < count; i++)
		{
			const size_t previous = (i + count - 1) % count;
			const size_t next = (i + 1) % count;
			const FOutVector *a = &vertices[indices[previous]];
			const FOutVector *b = &vertices[indices[i]];
			const FOutVector *c = &vertices[indices[next]];
			const double cross = HWR_PlaneVertexCross(a, b, c);
			boolean containsvertex = false;
			size_t j;

			                                                                 
                                                                  
			if (fabs(cross) <= 1.0E-8)
			{
				memmove(&indices[i], &indices[i + 1],
					(count - i - 1) * sizeof (*indices));
				count--;
				clipped = true;
				break;
			}
			if (orientation * cross < 0.0)
				continue;

			for (j = 0; j < count; j++)
			{
				if (j == previous || j == i || j == next)
					continue;
				if (HWR_PointStrictlyInsidePlaneTriangle(
					&vertices[indices[j]], a, b, c, orientation))
				{
					containsvertex = true;
					break;
				}
			}
			if (containsvertex)
				continue;

			triangle[0] = *a;
			triangle[1] = *b;
			triangle[2] = *c;
			HWR_ProcessPolygon(surf, triangle, 3, polyflags, shader, false);
			memmove(&indices[i], &indices[i + 1],
				(count - i - 1) * sizeof (*indices));
			count--;
			clipped = true;
			break;
		}

		                                                                    
                                                                    
                                                               
		if (!clipped)
			return;
	}

	if (count == 3
	&& orientation * HWR_PlaneVertexCross(&vertices[indices[0]],
		&vertices[indices[1]], &vertices[indices[2]]) > 1.0E-8)
	{
		triangle[0] = vertices[indices[0]];
		triangle[1] = vertices[indices[1]];
		triangle[2] = vertices[indices[2]];
		HWR_ProcessPolygon(surf, triangle, 3, polyflags, shader, false);
	}
}
#endif

static void HWR_RenderPlane(subsector_t *subsector, extrasubsector_t *xsub, boolean isceiling, fixed_t fixedheight, FBITFIELD PolyFlags, INT32 lightlevel, levelflat_t *levelflat, sector_t *FOFsector, UINT8 alpha, extracolormap_t *planecolormap)
{
	FSurfaceInfo Surf;
	FOutVector *v3d;
	polyvertex_t *pv;
	pslope_t *slope = NULL;
	INT32 shader = SHADER_NONE;

	size_t nrPlaneVerts;
	INT32 i;

	float height;  
	float anglef = 0.0f;
	float fflatwidth = 64.0f, fflatheight = 64.0f;
	float xscale = 1.0f, yscale = 1.0f;

	float tempxsow, tempytow;
	float scrollx = 0.0f, scrolly = 0.0f;
	angle_t angle = 0;

	static FOutVector *planeVerts = NULL;
	static FOutVector *portalPlaneVerts = NULL;
	static UINT16 numAllocedPlaneVerts = 0;
	static UINT16 numAllocedPortalPlaneVerts = 0;
	FOutVector *drawPlaneVerts;

	if (!r_renderfloors)
		return;


	if (!xsub->planepoly)
		return;

	pv = xsub->planepoly->pts;
	nrPlaneVerts = xsub->planepoly->numpts;

	if (nrPlaneVerts < 3)  
		return;


	if (FOFsector)
	{
		if (FOFsector->f_slope && !isceiling)
			slope = FOFsector->f_slope;
		else if (FOFsector->c_slope && isceiling)
			slope = FOFsector->c_slope;
	}
	else
	{
		if (gl_frontsector->f_slope && !isceiling)
			slope = gl_frontsector->f_slope;
		else if (gl_frontsector->c_slope && isceiling)
			slope = gl_frontsector->c_slope;
	}

	height = FixedToFloat(fixedheight);


	if (!planeVerts || nrPlaneVerts > numAllocedPlaneVerts)
	{
		numAllocedPlaneVerts = (UINT16)nrPlaneVerts;
		Z_Free(planeVerts);
		Z_Malloc(numAllocedPlaneVerts * sizeof (FOutVector), PU_LEVEL, &planeVerts);
	}


	if (levelflat != NULL)
	{
		texture_t *texture = textures[R_GetTextureNumForFlat(levelflat)];
		fflatwidth = texture->width;
		fflatheight = texture->height;
	}
	else  
		HWR_SetCurrentTexture(NULL);


	if (FOFsector != NULL)
	{
		if (!isceiling)  
		{
			xscale = FixedToFloat(FOFsector->floorxscale);
			yscale = FixedToFloat(FOFsector->flooryscale);
			scrollx = FixedToFloat(FOFsector->floorxoffset) / fflatwidth;
			scrolly = FixedToFloat(FOFsector->flooryoffset) / fflatheight;
			angle = FOFsector->floorangle;
		}
		else  
		{
			xscale = FixedToFloat(FOFsector->ceilingxscale);
			yscale = FixedToFloat(FOFsector->ceilingyscale);
			scrollx = FixedToFloat(FOFsector->ceilingxoffset) / fflatwidth;
			scrolly = FixedToFloat(FOFsector->ceilingyoffset) / fflatheight;
			angle = FOFsector->ceilingangle;
		}
	}
	else if (gl_frontsector)
	{
		if (!isceiling)  
		{
			xscale = FixedToFloat(gl_frontsector->floorxscale);
			yscale = FixedToFloat(gl_frontsector->flooryscale);
			scrollx = FixedToFloat(gl_frontsector->floorxoffset) / fflatwidth;
			scrolly = FixedToFloat(gl_frontsector->flooryoffset) / fflatheight;
			angle = gl_frontsector->floorangle;
		}
		else  
		{
			xscale = FixedToFloat(gl_frontsector->ceilingxscale);
			yscale = FixedToFloat(gl_frontsector->ceilingyscale);
			scrollx = FixedToFloat(gl_frontsector->ceilingxoffset) / fflatwidth;
			scrolly = FixedToFloat(gl_frontsector->ceilingyoffset) / fflatheight;
			angle = gl_frontsector->ceilingangle;
		}
	}

	anglef = ANG2RAD(InvAngle(angle));

	                                                                      
                                                                        
                                                                          
                                                                             
	if (angle)
	{
		tempxsow = scrollx * fflatwidth;
		tempytow = scrolly * fflatheight;
		scrollx = ((tempxsow * cos(anglef)) - (tempytow * sin(anglef))) / fflatwidth;
		scrolly = ((tempxsow * sin(anglef)) + (tempytow * cos(anglef))) / fflatheight;
	}

#define SETUP3DVERT(vert, vx, vy) {\
		                                                                     \
                                                                   \
		vert->s = (vx);\
		vert->t = -(vy);\
		\
		if (angle)  \
		{\
			tempxsow = vert->s;\
			tempytow = vert->t;\
			vert->s = (tempxsow * cos(anglef)) - (tempytow * sin(anglef));\
			vert->t = (tempxsow * sin(anglef)) + (tempytow * cos(anglef));\
		}\
\
		 \
		vert->s = ((vert->s * xscale) / fflatwidth) + scrollx;\
		vert->t = ((vert->t * yscale) / fflatheight) + scrolly;\
\
		if (slope)\
		{\
			fixedheight = P_GetSlopeZAt(slope, FloatToFixed((vx)), FloatToFixed((vy)));\
			height = FixedToFloat(fixedheight);\
		}\
\
		vert->x = (vx);\
		vert->y = height;\
		vert->z = (vy);\
}

	for (i = 0, v3d = planeVerts; i < (INT32)nrPlaneVerts; i++,v3d++,pv++)
		SETUP3DVERT(v3d, pv->x, pv->y);
	drawPlaneVerts = planeVerts;

	                                                                           
                                                                         
                                                                        
                                                                           
	if (hwr_portal_pass && hwr_portal_destination_line)
	{
		size_t outcount = 0, previous = nrPlaneVerts - 1;

		if (!portalPlaneVerts || nrPlaneVerts + 1 > numAllocedPortalPlaneVerts)
		{
			numAllocedPortalPlaneVerts = (UINT16)(nrPlaneVerts + 1);
			Z_Free(portalPlaneVerts);
			Z_Malloc(numAllocedPortalPlaneVerts * sizeof (*portalPlaneVerts),
				PU_LEVEL, &portalPlaneVerts);
		}
		for (i = 0; i < (INT32)nrPlaneVerts; previous = (size_t)i, i++)
		{
			const FOutVector *first = &planeVerts[previous];
			const FOutVector *second = &planeVerts[i];
			const double firstside = HWR_FloatPortalLineSide(first->x, first->z,
				hwr_portal_destination_line);
			const double secondside = HWR_FloatPortalLineSide(second->x, second->z,
				hwr_portal_destination_line);
			const boolean firstinside = firstside <= 0.000001;
			const boolean secondinside = secondside <= 0.000001;

			if (firstinside != secondinside)
			{
				const float fraction = (float)(firstside / (firstside - secondside));
				FOutVector *intersection = &portalPlaneVerts[outcount++];
				intersection->x = first->x + (second->x - first->x) * fraction;
				intersection->y = first->y + (second->y - first->y) * fraction;
				intersection->z = first->z + (second->z - first->z) * fraction;
				intersection->s = first->s + (second->s - first->s) * fraction;
				intersection->t = first->t + (second->t - first->t) * fraction;
			}
			if (secondinside)
				portalPlaneVerts[outcount++] = *second;
		}
		if (outcount < 3)
			return;
		nrPlaneVerts = outcount;
		drawPlaneVerts = portalPlaneVerts;
	}

	if (slope)
		lightlevel = HWR_CalcSlopeLight(lightlevel, R_PointToAngle2(0, 0, slope->normal.x, slope->normal.y), abs(slope->zdelta));

	HWR_Lighting(&Surf, lightlevel, planecolormap);

	if (PolyFlags & (PF_Translucent|PF_Fog|PF_Additive|PF_Subtractive|PF_ReverseSubtract|PF_Multiplicative|PF_Environment))
	{
		Surf.PolyColor.s.alpha = (UINT8)alpha;
		PolyFlags |= PF_Modulated;
	}
	else
		PolyFlags |= PF_Masked|PF_Modulated;

	if (HWR_UseShader())
	{
		if (PolyFlags & PF_Fog)
			shader = SHADER_FOG;
		else if (PolyFlags & PF_Ripple)
			shader = SHADER_WATER;
		else
			shader = SHADER_FLOOR;

		PolyFlags |= PF_ColorMapped;
	}

	HWR_ProcessPolygon(&Surf, drawPlaneVerts, nrPlaneVerts, PolyFlags, shader, false);

	if (subsector)
	{

		FOutVector horizonpts[6];
		float dist, vx, vy;
		float x1, y1, xd, yd;
		UINT8 numplanes, j;
		vertex_t v;  

		const float renderdist = 27000.0f;  
		const float farrenderdist = 32768.0f;  

		seg_t *line = &segs[subsector->firstline];

		for (i = 0; i < subsector->numlines; i++, line++)
		{
			if (!line->glseg && line->linedef->special == SPECIAL_HORIZON_LINE && R_PointOnSegSide(viewx, viewy, line) == 0)
			{
				P_ClosestPointOnLine(viewx, viewy, line->linedef, &v);
				dist = FIXED_TO_FLOAT(R_PointToDist(v.x, v.y));

				if (line->pv1)
				{
					x1 = ((polyvertex_t *)line->pv1)->x;
					y1 = ((polyvertex_t *)line->pv1)->y;
				}
				else
				{
					x1 = FIXED_TO_FLOAT(line->v1->x);
					y1 = FIXED_TO_FLOAT(line->v1->y);
				}
				if (line->pv2)
				{
					xd = ((polyvertex_t *)line->pv2)->x - x1;
					yd = ((polyvertex_t *)line->pv2)->y - y1;
				}
				else
				{
					xd = FIXED_TO_FLOAT(line->v2->x) - x1;
					yd = FIXED_TO_FLOAT(line->v2->y) - y1;
				}


				                                                            
                                                                   
                                                 
				if (dist < 0.001f)
					dist = 0.001f;
				dist = hypotf(xd, yd) / dist / 16.0f;
				if (dist > 100.0f)
					numplanes = 100;
				else
					numplanes = (UINT8)dist + 1;

				for (j = 0; j < numplanes; j++)
				{

					vx = x1 + xd * j / numplanes;
					vy = y1 + yd * j / numplanes;
					SETUP3DVERT((&horizonpts[1]), vx, vy);

					dist = hypotf(vx - gl_viewx, vy - gl_viewy);
					if (dist < 0.001f)
						dist = 0.001f;
					vx = (vx - gl_viewx) * renderdist / dist + gl_viewx;
					vy = (vy - gl_viewy) * renderdist / dist + gl_viewy;
					SETUP3DVERT((&horizonpts[0]), vx, vy);


					vx = x1 + xd * (j+1) / numplanes;
					vy = y1 + yd * (j+1) / numplanes;
					SETUP3DVERT((&horizonpts[2]), vx, vy);

					dist = hypotf(vx - gl_viewx, vy - gl_viewy);
					if (dist < 0.001f)
						dist = 0.001f;
					vx = (vx - gl_viewx) * renderdist / dist + gl_viewx;
					vy = (vy - gl_viewy) * renderdist / dist + gl_viewy;
					SETUP3DVERT((&horizonpts[3]), vx, vy);


					vx = (horizonpts[0].x - gl_viewx) * farrenderdist / renderdist + gl_viewx;
					vy = (horizonpts[0].z - gl_viewy) * farrenderdist / renderdist + gl_viewy;
					SETUP3DVERT((&horizonpts[5]), vx, vy);
					horizonpts[5].y = gl_viewz;

					vx = (horizonpts[3].x - gl_viewx) * farrenderdist / renderdist + gl_viewx;
					vy = (horizonpts[3].z - gl_viewy) * farrenderdist / renderdist + gl_viewy;
					SETUP3DVERT((&horizonpts[4]), vx, vy);
					horizonpts[4].y = gl_viewz;


					HWR_ProcessPolygon(&Surf, horizonpts, 6, PolyFlags, shader, true);
				}
			}
		}
	}

#ifdef ALAM_LIGHTING

	HWR_PlaneLighting(planeVerts, nrPlaneVerts);
#endif
}

FBITFIELD HWR_GetBlendModeFlag(INT32 style)
{
	switch (style)
	{
		case AST_TRANSLUCENT:
			return PF_Translucent;
		case AST_ADD:
			return PF_Additive;
		case AST_SUBTRACT:
			return PF_Subtractive;
		case AST_REVERSESUBTRACT:
			return PF_ReverseSubtract;
		case AST_MODULATE:
			return PF_Multiplicative;
		default:
			return PF_Masked;
	}
}

UINT8 HWR_GetTranstableAlpha(INT32 transtablenum)
{
	transtablenum = max(min(transtablenum, tr_trans90), 0);

	switch (transtablenum)
	{
		case 0          : return 0xff;
		case tr_trans10 : return 0xe6;
		case tr_trans20 : return 0xcc;
		case tr_trans30 : return 0xb3;
		case tr_trans40 : return 0x99;
		case tr_trans50 : return 0x80;
		case tr_trans60 : return 0x66;
		case tr_trans70 : return 0x4c;
		case tr_trans80 : return 0x33;
		case tr_trans90 : return 0x19;
	}

	return 0xff;
}

FBITFIELD HWR_SurfaceBlend(INT32 style, INT32 transtablenum, FSurfaceInfo *pSurf)
{
	if (!transtablenum || style <= AST_COPY || style >= AST_OVERLAY)
	{
		pSurf->PolyColor.s.alpha = 0xff;
		return PF_Masked;
	}

	pSurf->PolyColor.s.alpha = HWR_GetTranstableAlpha(transtablenum);
	return HWR_GetBlendModeFlag(style);
}

FBITFIELD HWR_TranstableToAlpha(INT32 transtablenum, FSurfaceInfo *pSurf)
{
	if (!transtablenum)
	{
		pSurf->PolyColor.s.alpha = 0x00;
		return PF_Masked;
	}

	pSurf->PolyColor.s.alpha = HWR_GetTranstableAlpha(transtablenum);
	return PF_Translucent;
}

static void HWR_AddTransparentWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, INT32 texnum, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap);





  
                       
      
      
      
      
  




static void HWR_ProjectWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, FBITFIELD blendmode, INT32 lightlevel, extracolormap_t *wallcolormap)
{
	INT32 shader = SHADER_NONE;

	if (!r_renderwalls)
		return;

	HWR_Lighting(pSurf, lightlevel, wallcolormap);

	if (HWR_UseShader())
	{
		shader = SHADER_WALL;
		blendmode |= PF_ColorMapped;
	}

	HWR_ProcessPolygon(pSurf, wallVerts, 4, blendmode|PF_Modulated|PF_Occlude, shader, false);
}







static void HWR_SplitWall(sector_t *sector, FOutVector *wallVerts, INT32 texnum, FSurfaceInfo* Surf, INT32 cutflag, ffloor_t *pfloor, FBITFIELD polyflags)
{
	float realtop, realbot, top, bot;
	float pegt, pegb, pegmul;
	float height = 0.0f, bheight = 0.0f;

	float endrealtop, endrealbot, endtop, endbot;
	float endpegt, endpegb, endpegmul;
	float endheight = 0.0f, endbheight = 0.0f;

	float diff;

	fixed_t v1x = FloatToFixed(wallVerts[0].x);
	fixed_t v1y = FloatToFixed(wallVerts[0].z);
	fixed_t v2x = FloatToFixed(wallVerts[1].x);
	fixed_t v2y = FloatToFixed(wallVerts[1].z);

	FUINT lightnum = HWR_SideLightLevel(gl_sidedef, sector->lightlevel);
	const UINT8 alpha = Surf->PolyColor.s.alpha;
	lightnum = HWR_CalcWallLight(lightnum, v1x, v1y, v2x, v2y);
	extracolormap_t *colormap = NULL;

	if (!r_renderwalls)
		return;

	realtop = top = wallVerts[3].y;
	realbot = bot = wallVerts[0].y;
	diff = top - bot;

	pegt = wallVerts[3].t;
	pegb = wallVerts[0].t;




	if (fpclassify(diff) == FP_ZERO)
		pegmul = 0.0;
	else
		pegmul = (pegb - pegt) / diff;

	endrealtop = endtop = wallVerts[2].y;
	endrealbot = endbot = wallVerts[1].y;
	diff = endtop - endbot;

	endpegt = wallVerts[2].t;
	endpegb = wallVerts[1].t;

	if (fpclassify(diff) == FP_ZERO)
		endpegmul = 0.0;
	else
		endpegmul = (endpegb - endpegt) / diff;

	for (INT32 i = 0; i < sector->numlights; i++)
	{
		if (endtop < endrealbot && top < realbot)
			return;

		lightlist_t *list = sector->lightlist;

		if (!(list[i].flags & FOF_NOSHADE))
		{
			if (pfloor && (pfloor->fofflags & FOF_FOG))
			{
				lightnum = HWR_SideLightLevel(gl_sidedef, pfloor->master->frontsector->lightlevel);
				colormap = pfloor->master->frontsector->extra_colormap;
				lightnum = colormap ? lightnum : HWR_CalcWallLight(lightnum, v1x, v1y, v2x, v2y);
			}
			else
			{
				lightnum = HWR_SideLightLevel(gl_sidedef, *list[i].lightlevel);
				colormap = *list[i].extra_colormap;
				lightnum = colormap ? lightnum : HWR_CalcWallLight(lightnum, v1x, v1y, v2x, v2y);
			}
		}

		boolean solid = false;

		if ((sector->lightlist[i].flags & FOF_CUTSOLIDS) && !(cutflag & FOF_EXTRA))
			solid = true;
		else if ((sector->lightlist[i].flags & FOF_CUTEXTRA) && (cutflag & FOF_EXTRA))
		{
			if (sector->lightlist[i].flags & FOF_EXTRA)
			{
				if ((sector->lightlist[i].flags & (FOF_FOG|FOF_SWIMMABLE)) == (cutflag & (FOF_FOG|FOF_SWIMMABLE)))  
					solid = true;
			}
			else
				solid = true;
		}
		else
			solid = false;

		height = FixedToFloat(P_GetLightZAt(&list[i], v1x, v1y));
		endheight = FixedToFloat(P_GetLightZAt(&list[i], v2x, v2y));
		if (solid)
		{
			bheight = FixedToFloat(P_GetFFloorBottomZAt(list[i].caster, v1x, v1y));
			endbheight = FixedToFloat(P_GetFFloorBottomZAt(list[i].caster, v2x, v2y));
		}

		if (endheight >= endtop && height >= top)
		{
			if (solid && top > bheight)
				top = bheight;
			if (solid && endtop > endbheight)
				endtop = endbheight;
		}

		if (i + 1 < sector->numlights)
		{
			bheight = FixedToFloat(P_GetLightZAt(&list[i+1], v1x, v1y));
			endbheight = FixedToFloat(P_GetLightZAt(&list[i+1], v2x, v2y));
		}
		else
		{
			bheight = realbot;
			endbheight = endrealbot;
		}



		bot = min(max(bheight, realbot), top);
		endbot = min(max(endbheight, endrealbot), endtop);

		Surf->PolyColor.s.alpha = alpha;

		wallVerts[3].t = pegt + ((realtop - top) * pegmul);
		wallVerts[2].t = endpegt + ((endrealtop - endtop) * endpegmul);
		wallVerts[0].t = pegt + ((realtop - bot) * pegmul);
		wallVerts[1].t = endpegt + ((endrealtop - endbot) * endpegmul);


		wallVerts[3].y = top;
		wallVerts[2].y = endtop;
		wallVerts[0].y = bot;
		wallVerts[1].y = endbot;

		if (cutflag & FOF_FOG)
			HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Fog|PF_NoTexture|polyflags, true, lightnum, colormap);
		else if (polyflags & (PF_Translucent|PF_Additive|PF_Subtractive|PF_ReverseSubtract|PF_Multiplicative|PF_Environment))
			HWR_AddTransparentWall(wallVerts, Surf, texnum, polyflags, false, lightnum, colormap);
		else
			HWR_ProjectWall(wallVerts, Surf, PF_Masked|polyflags, lightnum, colormap);

		top = bot;
		endtop = endbot;
	}

	bot = realbot;
	endbot = endrealbot;
	if (endtop <= endrealbot && top <= realbot)
		return;

	Surf->PolyColor.s.alpha = alpha;

	wallVerts[3].t = pegt + ((realtop - top) * pegmul);
	wallVerts[2].t = endpegt + ((endrealtop - endtop) * endpegmul);
	wallVerts[0].t = pegt + ((realtop - bot) * pegmul);
	wallVerts[1].t = endpegt + ((endrealtop - endbot) * endpegmul);


	wallVerts[3].y = top;
	wallVerts[2].y = endtop;
	wallVerts[0].y = bot;
	wallVerts[1].y = endbot;

	if (cutflag & FOF_FOG)
		HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Fog|PF_NoTexture|polyflags, true, lightnum, colormap);
	else if (polyflags & (PF_Translucent|PF_Additive|PF_Subtractive|PF_ReverseSubtract|PF_Multiplicative|PF_Environment))
		HWR_AddTransparentWall(wallVerts, Surf, texnum, polyflags, false, lightnum, colormap);
	else
		HWR_ProjectWall(wallVerts, Surf, PF_Masked|polyflags, lightnum, colormap);
}



static void HWR_DrawSkyWall(FOutVector *wallVerts, FSurfaceInfo *Surf)
{
	HWR_SetCurrentTexture(NULL);

	wallVerts[3].t = wallVerts[2].t = 0;
	wallVerts[0].t = wallVerts[1].t = 0;
	wallVerts[0].s = wallVerts[3].s = 0;
	wallVerts[2].s = wallVerts[1].s = 0;

	HWR_ProjectWall(wallVerts, Surf, PF_Invisible|PF_NoTexture, 255, NULL);



}


static boolean HWR_BlendMidtextureSurface(FSurfaceInfo *pSurf)
{
	FUINT blendmode = PF_Masked;

	pSurf->PolyColor.s.alpha = 0xFF;

	if (!gl_curline->polyseg)
	{
		if (gl_linedef->blendmode && gl_linedef->blendmode != AST_FOG)
		{
			if (gl_linedef->alpha >= 0 && gl_linedef->alpha < FRACUNIT)
				blendmode = HWR_SurfaceBlend(gl_linedef->blendmode, R_GetLinedefTransTable(gl_linedef->alpha), pSurf);
			else
				blendmode = HWR_GetBlendModeFlag(gl_linedef->blendmode);
		}
		else if (gl_linedef->alpha >= 0 && gl_linedef->alpha < FRACUNIT)
			blendmode = HWR_TranstableToAlpha(R_GetLinedefTransTable(gl_linedef->alpha), pSurf);
	}
	else if (gl_curline->polyseg->translucency > 0)
	{

		if (gl_curline->polyseg->translucency >= NUMTRANSMAPS)  
			return false;
		else
			blendmode = HWR_TranstableToAlpha(gl_curline->polyseg->translucency, pSurf);
	}

	if (blendmode != PF_Masked && pSurf->PolyColor.s.alpha == 0x00)
		return false;

	pSurf->PolyFlags = blendmode;

	return true;
}

                                                                               
                                                                              
                                                                              
                                                                           
                                                    
static void HWR_WallTextureCoords(double cliplow, double cliphigh,
	fixed_t scalex, fixed_t offset, float texturescale,
	boolean protectwrap, float *left, float *right)
{
	const double scale = (double)scalex / (double)FRACUNIT;
	const double signedoffset = (scalex < 0) ? -(double)offset : (double)offset;
	                                                                       
                                                                            
                                                                              
                                                       
	double low = (cliplow * scale + signedoffset) * (double)texturescale;
	double high = (cliphigh * scale + signedoffset) * (double)texturescale;
	const double repeat = floor(low);

	low -= repeat;
	high -= repeat;
	*left = (float)low;
	                                                                      
                                                                            
                                                                              
                                                                          
                                                                            
                                   
	*right = protectwrap ? nextafterf((float)high, *left) : (float)high;
}

static void HWR_RenderMidtexture(INT32 gl_midtexture, double cliplow, double cliphigh, fixed_t worldtop, fixed_t worldbottom, fixed_t worldhigh, fixed_t worldlow, fixed_t worldtopslope, fixed_t worldbottomslope, fixed_t worldhighslope, fixed_t worldlowslope, UINT32 lightnum, FOutVector *inWallVerts)
{
	sector_t *front, *back;
	FOutVector wallVerts[4];

	FSurfaceInfo Surf;
	Surf.PolyColor.s.alpha = 255;


	if (!HWR_BlendMidtextureSurface(&Surf))
		return;

	if (gl_linedef->frontsector->heightsec != -1)
		front = &sectors[gl_linedef->frontsector->heightsec];
	else
		front = gl_linedef->frontsector;

	if (gl_linedef->backsector->heightsec != -1)
		back = &sectors[gl_linedef->backsector->heightsec];
	else
		back = gl_linedef->backsector;

	fixed_t texheight = FixedDiv(textureheight[gl_midtexture], abs(gl_sidedef->scaley_mid));
	INT32 repeats;

	if (gl_sidedef->repeatcnt)
		repeats = 1 + gl_sidedef->repeatcnt;
	else if (gl_linedef->flags & ML_WRAPMIDTEX)
	{
		fixed_t high, low;
		const fixed_t textureoffset = gl_sidedef->rowoffset + gl_sidedef->offsety_mid;

		if (front->ceilingheight > back->ceilingheight)
			high = back->ceilingheight;
		else
			high = front->ceilingheight;

		if (front->floorheight > back->floorheight)
			low = front->floorheight;
		else
			low = back->floorheight;

		repeats = (high - low) / texheight;
		if ((high - low) % texheight)
			repeats++;  

		if (udmf)
		{
			fixed_t filloffset = 0;

			                                                                     
                                                                       
                                                                 
			if (gl_linedef->flags & ML_MIDPEG)
			{
				if (textureoffset < 0)
					filloffset = -textureoffset;
			}
			else if (textureoffset > 0)
				filloffset = textureoffset;

			if (filloffset)
			{
				repeats += filloffset / textureheight[gl_midtexture];
				if (filloffset % textureheight[gl_midtexture])
					repeats++;
			}
		}

	}
	else
		repeats = 1;

	GLMapTexture_t *grTex = HWR_GetTexture(gl_midtexture, true);
	float yscale = FixedToFloat(gl_sidedef->scaley_mid);
	float sleft, sright;
	HWR_WallTextureCoords(cliplow, cliphigh, gl_sidedef->scalex_mid,
		gl_sidedef->textureoffset + gl_sidedef->offsetx_mid,
		grTex->scaleX, true, &sleft, &sright);






	fixed_t popentop, popenbottom, polytop, polybottom, lowcut, highcut;
	fixed_t popentopslope, popenbottomslope, polytopslope, polybottomslope, lowcutslope, highcutslope;




	if (gl_curline->polyseg)
	{

		popentop = popentopslope = back->ceilingheight;
		popenbottom = popenbottomslope = back->floorheight;
	}
	else
	{
		popentop = min(worldtop, worldhigh);
		popenbottom = max(worldbottom, worldlow);
		popentopslope = min(worldtopslope, worldhighslope);
		popenbottomslope = max(worldbottomslope, worldlowslope);
	}


	fixed_t midtexheight = texheight * repeats;

	fixed_t rowoffset = FixedDiv(gl_sidedef->rowoffset + gl_sidedef->offsety_mid,
		abs(gl_sidedef->scaley_mid));


	if (gl_linedef->flags & ML_NOSKEW)
	{

		if (gl_linedef->flags & ML_MIDPEG)
		{
			polybottom = max(gl_frontsector->floorheight, gl_backsector->floorheight) + rowoffset;
			polytop = polybottom + midtexheight;
		}

		else
		{
			polytop = min(gl_frontsector->ceilingheight, gl_backsector->ceilingheight) + rowoffset;
			polybottom = polytop - midtexheight;
		}


		polytopslope = polytop;
		polybottomslope = polybottom;
	}

	else if (gl_linedef->flags & ML_MIDPEG)
	{
		polybottom = popenbottom + rowoffset;
		polytop = polybottom + midtexheight;
		polybottomslope = popenbottomslope + rowoffset;
		polytopslope = polybottomslope + midtexheight;
	}

	else
	{
		polytop = popentop + rowoffset;
		polybottom = polytop - midtexheight;
		polytopslope = popentopslope + rowoffset;
		polybottomslope = polytopslope - midtexheight;
	}


	if (gl_curline->polyseg)
	{
		lowcut = polybottom;
		highcut = polytop;
		lowcutslope = polybottomslope;
		highcutslope = polytopslope;
	}
	else
	{
		lowcut = popenbottom;
		highcut = popentop;
		lowcutslope = popenbottomslope;
		highcutslope = popentopslope;
	}

	fixed_t h = min(highcut, polytop);
	fixed_t l = max(polybottom, lowcut);
	fixed_t hS = min(highcutslope, polytopslope);
	fixed_t lS = max(polybottomslope, lowcutslope);


	fixed_t texturevpeg, texturevpegslope;

	if (gl_linedef->flags & ML_MIDPEG)
	{
		texturevpeg = midtexheight - h + polybottom;
		texturevpegslope = midtexheight - hS + polybottomslope;
	}
	else
	{
		texturevpeg = polytop - h;
		texturevpegslope = polytopslope - hS;
	}

	memcpy(wallVerts, inWallVerts, sizeof(wallVerts));


	wallVerts[3].t = texturevpeg * yscale * grTex->scaleY;
	wallVerts[0].t = (h - l + texturevpeg) * yscale * grTex->scaleY;
	wallVerts[0].s = wallVerts[3].s = sleft;


	wallVerts[2].t = texturevpegslope * yscale * grTex->scaleY;
	wallVerts[1].t = (hS - lS + texturevpegslope) * yscale * grTex->scaleY;
	wallVerts[2].s = wallVerts[1].s = sright;




	wallVerts[3].y = FIXED_TO_FLOAT(h);
	wallVerts[0].y = FIXED_TO_FLOAT(l);
	wallVerts[2].y = FIXED_TO_FLOAT(hS);
	wallVerts[1].y = FIXED_TO_FLOAT(lS);


	FUINT blendmode = Surf.PolyFlags;



	blendmode |= PF_Decal;

	extracolormap_t *colormap = gl_frontsector->extra_colormap;

	if (gl_frontsector->numlights)
	{
		if (!(blendmode & PF_Masked))
			HWR_SplitWall(gl_frontsector, wallVerts, gl_midtexture, &Surf, FOF_TRANSLUCENT, NULL, blendmode);
		else
			HWR_SplitWall(gl_frontsector, wallVerts, gl_midtexture, &Surf, FOF_CUTLEVEL, NULL, blendmode);
	}
	else if (!(blendmode & PF_Masked))
		HWR_AddTransparentWall(wallVerts, &Surf, gl_midtexture, blendmode, false, lightnum, colormap);
	else
		HWR_ProjectWall(wallVerts, &Surf, blendmode, lightnum, colormap);
}


static void HWR_ProcessSeg(void)
{
	FOutVector wallVerts[4];
	v2d_t vs, ve;  
	float segstart = 0.0f, segend = 1.0f;

	fixed_t worldtop, worldbottom;
	fixed_t worldhigh = 0, worldlow = 0;
	fixed_t worldtopslope, worldbottomslope;
	fixed_t worldhighslope = 0, worldlowslope = 0;
	fixed_t v1x, v1y, v2x, v2y;

	fixed_t h, l;  
	fixed_t hS, lS;
	float yscale;

	gl_sidedef = gl_curline->sidedef;
	gl_linedef = gl_curline->linedef;

	if (gl_curline->pv1)
	{
		vs.x = ((polyvertex_t *)gl_curline->pv1)->x;
		vs.y = ((polyvertex_t *)gl_curline->pv1)->y;
	}
	else
	{
		vs.x = FIXED_TO_FLOAT(gl_curline->v1->x);
		vs.y = FIXED_TO_FLOAT(gl_curline->v1->y);
	}
	if (gl_curline->pv2)
	{
		ve.x = ((polyvertex_t *)gl_curline->pv2)->x;
		ve.y = ((polyvertex_t *)gl_curline->pv2)->y;
	}
	else
	{
		ve.x = FIXED_TO_FLOAT(gl_curline->v2->x);
		ve.y = FIXED_TO_FLOAT(gl_curline->v2->y);
	}

	                                                                           
                                                                            
                                                                                
	if (hwr_portal_pass && hwr_portal_destination_line)
	{
		const double firstside = HWR_FloatPortalLineSide(vs.x, vs.y,
			hwr_portal_destination_line);
		const double secondside = HWR_FloatPortalLineSide(ve.x, ve.y,
			hwr_portal_destination_line);
		const double epsilon = 0.000001;

		if (firstside > epsilon && secondside > epsilon)
			return;
		if ((firstside > epsilon) != (secondside > epsilon))
		{
			const float fraction = (float)(firstside / (firstside - secondside));
			const float ix = vs.x + (ve.x - vs.x) * fraction;
			const float iy = vs.y + (ve.y - vs.y) * fraction;

			if (firstside > epsilon)
			{
				vs.x = ix;
				vs.y = iy;
				segstart = fraction;
			}
			else
			{
				ve.x = ix;
				ve.y = iy;
				segend = fraction;
			}
		}
	}

	v1x = FLOAT_TO_FIXED(vs.x);
	v1y = FLOAT_TO_FIXED(vs.y);
	v2x = FLOAT_TO_FIXED(ve.x);
	v2y = FLOAT_TO_FIXED(ve.y);

#define SLOPEPARAMS(slope, end1, end2, normalheight) \
	end1 = P_GetZAt(slope, v1x, v1y, normalheight); \
	end2 = P_GetZAt(slope, v2x, v2y, normalheight);

	SLOPEPARAMS(gl_frontsector->c_slope, worldtop,    worldtopslope,    gl_frontsector->ceilingheight)
	SLOPEPARAMS(gl_frontsector->f_slope, worldbottom, worldbottomslope, gl_frontsector->floorheight)








	wallVerts[0].x = wallVerts[3].x = vs.x;
	wallVerts[0].z = wallVerts[3].z = vs.y;
	wallVerts[2].x = wallVerts[1].x = ve.x;
	wallVerts[2].z = wallVerts[1].z = ve.y;


	                                                                           
                                                                             
                                                                        
	double cliplow = (double)gl_curline->offset
		+ ((double)gl_curline->flength * FRACUNIT) * (double)segstart;
	double cliphigh = (double)gl_curline->offset
		+ ((double)gl_curline->flength * FRACUNIT) * (double)segend;

	FUINT lightnum = HWR_SideLightLevel(gl_sidedef, gl_frontsector->lightlevel);
	extracolormap_t *colormap = gl_frontsector->extra_colormap;
	lightnum = colormap ? lightnum : HWR_CalcWallLight(lightnum, vs.x, vs.y, ve.x, ve.y);

	FSurfaceInfo Surf;
	Surf.PolyColor.s.alpha = 255;

	INT32 gl_midtexture = R_GetTextureNum(gl_sidedef->midtexture);
	GLMapTexture_t *grTex = NULL;


	if (gl_backsector)
	{
		INT32 gl_toptexture = 0, gl_bottomtexture = 0;

		fixed_t texturevpeg;

		SLOPEPARAMS(gl_backsector->c_slope, worldhigh, worldhighslope, gl_backsector->ceilingheight)
		SLOPEPARAMS(gl_backsector->f_slope, worldlow,  worldlowslope,  gl_backsector->floorheight)



		if (gl_frontsector->ceilingpic == skyflatnum
			&& gl_backsector->ceilingpic  == skyflatnum
			&& !(HWR_CeilingPortalSurfaceActive(gl_frontsector)
				|| HWR_CeilingPortalSurfaceActive(gl_backsector)))
		{
			bothceilingssky = true;
		}


		if (gl_frontsector->floorpic == skyflatnum
			&& gl_backsector->floorpic == skyflatnum
			&& !(HWR_FloorPortalSurfaceActive(gl_frontsector)
				|| HWR_FloorPortalSurfaceActive(gl_backsector)))
		{
			bothfloorssky = true;
		}

		if (!bothceilingssky)
			gl_toptexture = R_GetTextureNum(gl_sidedef->toptexture);
		if (!bothfloorssky)
			gl_bottomtexture = R_GetTextureNum(gl_sidedef->bottomtexture);


		if ((worldhighslope < worldtopslope || worldhigh < worldtop) && gl_toptexture)
		{
			grTex = HWR_GetTexture(gl_toptexture, false);
			yscale = FixedToFloat(abs(gl_sidedef->scaley_top));

			fixed_t offsetx_top = gl_sidedef->textureoffset + gl_sidedef->offsetx_top;
			float sleft, sright;
			HWR_WallTextureCoords(cliplow, cliphigh, gl_sidedef->scalex_top,
				offsetx_top, grTex->scaleX, false, &sleft, &sright);

			fixed_t texheight = textureheight[gl_toptexture];
			fixed_t texheightscaled = FixedDiv(texheight, abs(gl_sidedef->scaley_top));



			if (gl_linedef->flags & ML_DONTPEGTOP)
				texturevpeg = 0;
			else if (gl_linedef->flags & ML_SKEWTD)
				texturevpeg = worldhigh + texheight - worldtop;
			else
				texturevpeg = gl_backsector->ceilingheight + texheightscaled - gl_frontsector->ceilingheight;

			texturevpeg *= yscale;

			if (gl_sidedef->scaley_top < 0)
				texturevpeg -= gl_sidedef->rowoffset + gl_sidedef->offsety_top;
			else
				texturevpeg += gl_sidedef->rowoffset + gl_sidedef->offsety_top;

			                                                                     
                                                                     
                                                                          
                                                                       
			texturevpeg %= texheight;

			wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
			wallVerts[0].t = wallVerts[1].t = (texturevpeg + (gl_frontsector->ceilingheight - gl_backsector->ceilingheight) * yscale) * grTex->scaleY;
			wallVerts[0].s = wallVerts[3].s = sleft;
			wallVerts[2].s = wallVerts[1].s = sright;


			if (!(gl_linedef->flags & ML_SKEWTD))
			{

				wallVerts[3].t -= (worldtop - gl_frontsector->ceilingheight) * yscale * grTex->scaleY;
				wallVerts[2].t -= (worldtopslope - gl_frontsector->ceilingheight) * yscale * grTex->scaleY;
				wallVerts[0].t -= (worldhigh - gl_backsector->ceilingheight) * yscale * grTex->scaleY;
				wallVerts[1].t -= (worldhighslope - gl_backsector->ceilingheight) * yscale * grTex->scaleY;
			}
			else if (gl_linedef->flags & ML_DONTPEGTOP)
			{

				wallVerts[0].t = (texturevpeg + (worldtop - worldhigh) * yscale) * grTex->scaleY;
				wallVerts[1].t = (texturevpeg + (worldtopslope - worldhighslope) * yscale) * grTex->scaleY;
			}
			else
			{

				wallVerts[0].t = wallVerts[1].t = (texturevpeg + (worldtop - worldhigh) * yscale) * grTex->scaleY;
				wallVerts[3].t = wallVerts[0].t - (worldtop - worldhigh) * yscale * grTex->scaleY;
				wallVerts[2].t = wallVerts[1].t - (worldtopslope - worldhighslope) * yscale * grTex->scaleY;
			}

			if (gl_sidedef->scaley_top < 0)
			{
				wallVerts[0].t = -wallVerts[0].t;
				wallVerts[1].t = -wallVerts[1].t;
				wallVerts[2].t = -wallVerts[2].t;
				wallVerts[3].t = -wallVerts[3].t;
			}


			wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = FIXED_TO_FLOAT(worldhigh);
			wallVerts[2].y = FIXED_TO_FLOAT(worldtopslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldhighslope);

			if (gl_frontsector->numlights)
				HWR_SplitWall(gl_frontsector, wallVerts, gl_toptexture, &Surf, FOF_CUTLEVEL, NULL, 0);
			else if (grTex->mipmap.flags & TF_TRANSPARENT)
				HWR_AddTransparentWall(wallVerts, &Surf, gl_toptexture, PF_Environment, false, lightnum, colormap);
			else
				HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
		}


		if ((worldlowslope > worldbottomslope || worldlow > worldbottom) && gl_bottomtexture)
		{
			grTex = HWR_GetTexture(gl_bottomtexture, false);
			yscale = FixedToFloat(abs(gl_sidedef->scaley_bottom));

			fixed_t offsetx_bottom = gl_sidedef->textureoffset + gl_sidedef->offsetx_bottom;
			float sleft, sright;
			HWR_WallTextureCoords(cliplow, cliphigh, gl_sidedef->scalex_bottom,
				offsetx_bottom, grTex->scaleX, false, &sleft, &sright);


			if (!(gl_linedef->flags & ML_DONTPEGBOTTOM))
				texturevpeg = 0;
			else if (gl_linedef->flags & ML_SKEWTD)
				texturevpeg = worldbottom - worldlow;
			else
				texturevpeg = gl_frontsector->floorheight - gl_backsector->floorheight;

			texturevpeg *= yscale;

			if (gl_sidedef->scaley_bottom < 0)
				texturevpeg -= gl_sidedef->rowoffset + gl_sidedef->offsety_bottom;
			else
				texturevpeg += gl_sidedef->rowoffset + gl_sidedef->offsety_bottom;

			 
			texturevpeg %= textureheight[gl_bottomtexture];

			wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
			wallVerts[0].t = wallVerts[1].t = (texturevpeg + (gl_backsector->floorheight - gl_frontsector->floorheight) * yscale) * grTex->scaleY;
			wallVerts[0].s = wallVerts[3].s = sleft;
			wallVerts[2].s = wallVerts[1].s = sright;


			if (!(gl_linedef->flags & ML_SKEWTD))
			{

				wallVerts[0].t -= (worldbottom - gl_frontsector->floorheight) * yscale * grTex->scaleY;
				wallVerts[1].t -= (worldbottomslope - gl_frontsector->floorheight) * yscale * grTex->scaleY;
				wallVerts[3].t -= (worldlow - gl_backsector->floorheight) * yscale * grTex->scaleY;
				wallVerts[2].t -= (worldlowslope - gl_backsector->floorheight) * yscale * grTex->scaleY;
			}
			else if (gl_linedef->flags & ML_DONTPEGBOTTOM)
			{

				wallVerts[0].t = wallVerts[1].t = (texturevpeg + (worldlow - worldbottom) * yscale) * grTex->scaleY;
				wallVerts[2].t = wallVerts[1].t - (worldlowslope - worldbottomslope) * yscale * grTex->scaleY;
			}
			else
			{

				wallVerts[0].t = (texturevpeg + (worldlow - worldbottom) * yscale) * grTex->scaleY;
				wallVerts[1].t = (texturevpeg + (worldlowslope - worldbottomslope) * yscale) * grTex->scaleY;
			}

			if (gl_sidedef->scaley_bottom < 0)
			{
				wallVerts[0].t = -wallVerts[0].t;
				wallVerts[1].t = -wallVerts[1].t;
				wallVerts[2].t = -wallVerts[2].t;
				wallVerts[3].t = -wallVerts[3].t;
			}


			wallVerts[3].y = FIXED_TO_FLOAT(worldlow);
			wallVerts[0].y = FIXED_TO_FLOAT(worldbottom);
			wallVerts[2].y = FIXED_TO_FLOAT(worldlowslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldbottomslope);

			if (gl_frontsector->numlights)
				HWR_SplitWall(gl_frontsector, wallVerts, gl_bottomtexture, &Surf, FOF_CUTLEVEL, NULL, 0);
			else if (grTex->mipmap.flags & TF_TRANSPARENT)
				HWR_AddTransparentWall(wallVerts, &Surf, gl_bottomtexture, PF_Environment, false, lightnum, colormap);
			else
				HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
		}


		if (gl_midtexture)
			HWR_RenderMidtexture(gl_midtexture, cliplow, cliphigh, worldtop, worldbottom, worldhigh, worldlow, worldtopslope, worldbottomslope, worldhighslope, worldlowslope, lightnum, wallVerts);

		if (!gl_curline->polyseg)  
		{


			if (gl_frontsector->ceilingpic == skyflatnum
				&& !HWR_CeilingPortalSurfaceActive(gl_frontsector)
				&& gl_backsector->ceilingpic != skyflatnum)  
			{
				wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(INT32_MAX);  
				wallVerts[0].y = FIXED_TO_FLOAT(worldtop);
				wallVerts[1].y = FIXED_TO_FLOAT(worldtopslope);
				HWR_DrawSkyWall(wallVerts, &Surf);
			}

			if (gl_frontsector->floorpic == skyflatnum
				&& !HWR_FloorPortalSurfaceActive(gl_frontsector)
				&& gl_backsector->floorpic != skyflatnum)  
			{
				wallVerts[3].y = FIXED_TO_FLOAT(worldbottom);
				wallVerts[2].y = FIXED_TO_FLOAT(worldbottomslope);
				wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(INT32_MIN);  
				HWR_DrawSkyWall(wallVerts, &Surf);
			}
		}
	}
	else
	{

		if (gl_midtexture && gl_linedef->special != SPECIAL_HORIZON_LINE)  
		{
			grTex = HWR_GetTexture(gl_midtexture, false);
			yscale = FixedToFloat(gl_sidedef->scaley_mid);
			float sleft, sright;
			HWR_WallTextureCoords(cliplow, cliphigh, gl_sidedef->scalex_mid,
				gl_sidedef->textureoffset + gl_sidedef->offsetx_mid,
				grTex->scaleX, false, &sleft, &sright);

			fixed_t texturevpeg;
			const fixed_t yscale_fixed = gl_sidedef->scaley_mid;
			const fixed_t texheight = textureheight[gl_sidedef->midtexture];


			if ((gl_linedef->flags & (ML_DONTPEGBOTTOM|ML_NOSKEW)) == (ML_DONTPEGBOTTOM|ML_NOSKEW))
				texturevpeg = FixedMul(gl_frontsector->floorheight - gl_frontsector->ceilingheight, yscale_fixed);
			else if (gl_linedef->flags & ML_DONTPEGBOTTOM)
				texturevpeg = FixedMul(worldbottom - worldtop, yscale_fixed);
			else

				texturevpeg = (yscale_fixed < 0) ? texheight : 0;

			                                                                      
                                                                     
                                                                     
			if ((gl_linedef->flags & ML_DONTPEGBOTTOM) && yscale_fixed > 0)
				texturevpeg += texheight;

			if (yscale_fixed < 0)
				texturevpeg -= gl_sidedef->rowoffset + gl_sidedef->offsety_mid;
			else
				texturevpeg += gl_sidedef->rowoffset + gl_sidedef->offsety_mid;

			wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
			                                                                       
                                                                          
                                       
			wallVerts[0].t = wallVerts[1].t = (texturevpeg
				+ FixedMul(gl_frontsector->ceilingheight - gl_frontsector->floorheight,
					yscale_fixed)) * grTex->scaleY;
			wallVerts[0].s = wallVerts[3].s = sleft;
			wallVerts[2].s = wallVerts[1].s = sright;


			if (gl_linedef->flags & ML_NOSKEW) {
				wallVerts[3].t += (gl_frontsector->ceilingheight - worldtop) * yscale * grTex->scaleY;
				wallVerts[2].t += (gl_frontsector->ceilingheight - worldtopslope) * yscale * grTex->scaleY;
				wallVerts[0].t += (gl_frontsector->floorheight - worldbottom) * yscale * grTex->scaleY;
				wallVerts[1].t += (gl_frontsector->floorheight - worldbottomslope) * yscale * grTex->scaleY;
			} else if (gl_linedef->flags & ML_DONTPEGBOTTOM) {
				wallVerts[3].t = wallVerts[0].t + ((worldbottom - worldtop) * yscale) * grTex->scaleY;
				wallVerts[2].t = wallVerts[1].t + ((worldbottomslope - worldtopslope) * yscale) * grTex->scaleY;
			} else {
				wallVerts[0].t = wallVerts[3].t - ((worldbottom - worldtop) * yscale) * grTex->scaleY;
				wallVerts[1].t = wallVerts[2].t - ((worldbottomslope - worldtopslope) * yscale) * grTex->scaleY;
			}


			wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = FIXED_TO_FLOAT(worldbottom);
			wallVerts[2].y = FIXED_TO_FLOAT(worldtopslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldbottomslope);


			if (gl_frontsector->numlights)
				HWR_SplitWall(gl_frontsector, wallVerts, gl_midtexture, &Surf, FOF_CUTLEVEL, NULL, 0);
			else
			{
				if (grTex->mipmap.flags & TF_TRANSPARENT)
					HWR_AddTransparentWall(wallVerts, &Surf, gl_midtexture, PF_Environment, false, lightnum, colormap);
				else
					HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
			}
		}

		if (!gl_curline->polyseg)
		{
			if (gl_frontsector->ceilingpic == skyflatnum
				&& !HWR_CeilingPortalSurfaceActive(gl_frontsector))  
			{
				wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(INT32_MAX);  
				wallVerts[0].y = FIXED_TO_FLOAT(worldtop);
				wallVerts[1].y = FIXED_TO_FLOAT(worldtopslope);
				HWR_DrawSkyWall(wallVerts, &Surf);
			}
			if (gl_frontsector->floorpic == skyflatnum
				&& !HWR_FloorPortalSurfaceActive(gl_frontsector))
			{
				wallVerts[3].y = FIXED_TO_FLOAT(worldbottom);
				wallVerts[2].y = FIXED_TO_FLOAT(worldbottomslope);
				wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(INT32_MIN);  
				HWR_DrawSkyWall(wallVerts, &Surf);
			}
		}
	}


	if (gl_backsector && !Tag_Compare(&gl_frontsector->tags, &gl_backsector->tags) && (gl_backsector->ffloors || gl_frontsector->ffloors))
	{
		ffloor_t * rover;
		fixed_t    highcut = 0, lowcut = 0;
		fixed_t lowcutslope, highcutslope;


		fixed_t high1, highslope1, low1, lowslope1;

		fixed_t texturehpeg = gl_sidedef->textureoffset + gl_sidedef->offsetx_mid;

		INT32 texnum;

		lowcut = max(worldbottom, worldlow);
		highcut = min(worldtop, worldhigh);
		lowcutslope = max(worldbottomslope, worldlowslope);
		highcutslope = min(worldtopslope, worldhighslope);

		if (gl_backsector->ffloors)
		{
			for (rover = gl_backsector->ffloors; rover; rover = rover->next)
			{
				boolean bothsides = false;

				ffloor_t * r2;
				for (r2 = gl_frontsector->ffloors; r2; r2 = r2->next)
					if (rover->master == r2->master)
					{
						bothsides = true;
						break;
					}

				if (bothsides) continue;

				if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_RENDERSIDES))
					continue;
				if (!(rover->fofflags & FOF_ALLSIDES) && rover->fofflags & FOF_INVERTSIDES)
					continue;

				SLOPEPARAMS(*rover->t_slope, high1, highslope1, *rover->topheight)
				SLOPEPARAMS(*rover->b_slope, low1,  lowslope1,  *rover->bottomheight)

				if ((high1 < lowcut && highslope1 < lowcutslope) || (low1 > highcut && lowslope1 > highcutslope))
					continue;

				side_t *side = R_GetFFloorSide(gl_curline->linedef, rover, gl_backsector);

				boolean do_texture_skew;
				boolean dont_peg_bottom;

				if (rover->master->flags & ML_TFERLINE)
				{
					line_t *newline = R_GetFFloorLine(gl_curline->linedef, rover, gl_backsector);
					do_texture_skew = newline->flags & ML_SKEWTD;
					dont_peg_bottom = newline->flags & ML_DONTPEGBOTTOM;
				}
				else
				{
					do_texture_skew = rover->master->flags & ML_SKEWTD;
					dont_peg_bottom = gl_curline->linedef->flags & ML_DONTPEGBOTTOM;
				}

				texnum = R_GetTextureNum(side->midtexture);

				h  = P_GetFFloorTopZAt   (rover, v1x, v1y);
				hS = P_GetFFloorTopZAt   (rover, v2x, v2y);
				l  = P_GetFFloorBottomZAt(rover, v1x, v1y);
				lS = P_GetFFloorBottomZAt(rover, v2x, v2y);

				if (h >= highcut && hS >= highcutslope)
				{
					h = highcut;
					hS = highcutslope;
				}
				if (l <= lowcut && lS <= lowcutslope)
				{
					l = lowcut;
					lS = lowcutslope;
				}


				wallVerts[3].y = FIXED_TO_FLOAT(h);
				wallVerts[2].y = FIXED_TO_FLOAT(hS);
				wallVerts[0].y = FIXED_TO_FLOAT(l);
				wallVerts[1].y = FIXED_TO_FLOAT(lS);

				if (rover->fofflags & FOF_FOG)
				{
					wallVerts[3].t = wallVerts[2].t = 0;
					wallVerts[0].t = wallVerts[1].t = 0;
					wallVerts[0].s = wallVerts[3].s = 0;
					wallVerts[2].s = wallVerts[1].s = 0;
				}
				else
				{



					fixed_t texturevpeg = side->rowoffset + side->offsety_mid;

					grTex = HWR_GetTexture(texnum, true);
					yscale = FixedToFloat(side->scaley_mid);
					float sleft, sright;
					HWR_WallTextureCoords(cliplow, cliphigh, side->scalex_mid,
						texturehpeg + side->offsetx_mid,
						grTex->scaleX, true, &sleft, &sright);

					if (!do_texture_skew)  
					{
						if (dont_peg_bottom)
							texturevpeg -= (*rover->topheight - *rover->bottomheight) * yscale;

						wallVerts[3].t = (((*rover->topheight - h) * yscale) + texturevpeg) * grTex->scaleY;
						wallVerts[2].t = (((*rover->topheight - hS) * yscale) + texturevpeg) * grTex->scaleY;
						wallVerts[0].t = (((*rover->topheight - l) * yscale) + texturevpeg) * grTex->scaleY;
						wallVerts[1].t = (((*rover->topheight - lS) * yscale) + texturevpeg) * grTex->scaleY;
					}
					else
					{
						if (!dont_peg_bottom)  
						{
							wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
							wallVerts[0].t = (((h - l) * yscale) + texturevpeg) * grTex->scaleY;
							wallVerts[1].t = (((hS - lS) * yscale) + texturevpeg) * grTex->scaleY;
						}
						else  
						{
							wallVerts[0].t = wallVerts[1].t = texturevpeg * grTex->scaleY;
							wallVerts[3].t = wallVerts[0].t - ((h - l) * yscale) * grTex->scaleY;
							wallVerts[2].t = wallVerts[1].t - ((hS - lS) * yscale) * grTex->scaleY;
						}
					}

					wallVerts[0].s = wallVerts[3].s = sleft;
					wallVerts[2].s = wallVerts[1].s = sright;
				}

				FBITFIELD blendmode;

				if (rover->fofflags & FOF_FOG)
				{
					blendmode = PF_Fog|PF_NoTexture;

					lightnum = HWR_SideLightLevel(gl_sidedef, rover->master->frontsector->lightlevel);
					colormap = rover->master->frontsector->extra_colormap;
					lightnum = colormap ? lightnum : HWR_CalcWallLight(lightnum, vs.x, vs.y, ve.x, ve.y);

					Surf.PolyColor.s.alpha = HWR_FogBlockAlpha(HWR_SideLightLevel(gl_sidedef, rover->master->frontsector->lightlevel), rover->master->frontsector->extra_colormap);

					if (gl_frontsector->numlights)
						HWR_SplitWall(gl_frontsector, wallVerts, 0, &Surf, rover->fofflags, rover, blendmode);
					else
						HWR_AddTransparentWall(wallVerts, &Surf, 0, blendmode, true, lightnum, colormap);
				}
				else
				{
					blendmode = PF_Masked;

					if ((rover->fofflags & FOF_TRANSLUCENT && !((rover->fofflags & FOF_SPLAT) && rover->alpha >= 255)) || rover->blend)
					{
						blendmode = rover->blend ? HWR_GetBlendModeFlag(rover->blend) : PF_Translucent;
						Surf.PolyColor.s.alpha = max(0, min(rover->alpha, 255));
					}

					if (gl_frontsector->numlights)
						HWR_SplitWall(gl_frontsector, wallVerts, texnum, &Surf, rover->fofflags, rover, blendmode);
					else
					{
						if (blendmode != PF_Masked)
							HWR_AddTransparentWall(wallVerts, &Surf, texnum, blendmode, false, lightnum, colormap);
						else
							HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
					}
				}
			}
		}

		if (gl_frontsector->ffloors)  
		{
			for (rover = gl_frontsector->ffloors; rover; rover = rover->next)
			{
				boolean bothsides = false;

				ffloor_t * r2;
				for (r2 = gl_backsector->ffloors; r2; r2 = r2->next)
					if (rover->master == r2->master)
					{
						bothsides = true;
						break;
					}

				if (bothsides) continue;

				if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_RENDERSIDES))
					continue;
				if (!(rover->fofflags & FOF_ALLSIDES || rover->fofflags & FOF_INVERTSIDES))
					continue;

				SLOPEPARAMS(*rover->t_slope, high1, highslope1, *rover->topheight)
				SLOPEPARAMS(*rover->b_slope, low1,  lowslope1,  *rover->bottomheight)

				if ((high1 < lowcut && highslope1 < lowcutslope) || (low1 > highcut && lowslope1 > highcutslope))
					continue;

				side_t *side = R_GetFFloorSide(gl_curline->linedef, rover, gl_backsector);

				boolean do_texture_skew;
				boolean dont_peg_bottom;

				if (rover->master->flags & ML_TFERLINE)
				{
					line_t *newline = R_GetFFloorLine(gl_curline->linedef, rover, gl_backsector);
					do_texture_skew = newline->flags & ML_SKEWTD;
					dont_peg_bottom = newline->flags & ML_DONTPEGBOTTOM;
				}
				else
				{
					do_texture_skew = rover->master->flags & ML_SKEWTD;
					dont_peg_bottom = gl_curline->linedef->flags & ML_DONTPEGBOTTOM;
				}

				texnum = R_GetTextureNum(side->midtexture);

				h  = P_GetFFloorTopZAt   (rover, v1x, v1y);
				hS = P_GetFFloorTopZAt   (rover, v2x, v2y);
				l  = P_GetFFloorBottomZAt(rover, v1x, v1y);
				lS = P_GetFFloorBottomZAt(rover, v2x, v2y);

				if (h >= highcut && hS >= highcutslope)
				{
					h = highcut;
					hS = highcutslope;
				}
				if (l <= lowcut && lS <= lowcutslope)
				{
					l = lowcut;
					lS = lowcutslope;
				}




				wallVerts[3].y = FIXED_TO_FLOAT(h);
				wallVerts[2].y = FIXED_TO_FLOAT(hS);
				wallVerts[0].y = FIXED_TO_FLOAT(l);
				wallVerts[1].y = FIXED_TO_FLOAT(lS);
				if (rover->fofflags & FOF_FOG)
				{
					wallVerts[3].t = wallVerts[2].t = 0;
					wallVerts[0].t = wallVerts[1].t = 0;
					wallVerts[0].s = wallVerts[3].s = 0;
					wallVerts[2].s = wallVerts[1].s = 0;
				}
				else
				{



					fixed_t texturevpeg = side->rowoffset + side->offsety_mid;

					grTex = HWR_GetTexture(texnum, true);
					yscale = FixedToFloat(side->scaley_mid);
					float sleft, sright;
					HWR_WallTextureCoords(cliplow, cliphigh, side->scalex_mid,
						texturehpeg + side->offsetx_mid,
						grTex->scaleX, true, &sleft, &sright);

					if (!do_texture_skew)  
					{
						if (dont_peg_bottom)
							texturevpeg -= (*rover->topheight - *rover->bottomheight) * yscale;

						wallVerts[3].t = (((*rover->topheight - h) * yscale) + texturevpeg) * grTex->scaleY;
						wallVerts[2].t = (((*rover->topheight - hS) * yscale) + texturevpeg) * grTex->scaleY;
						wallVerts[0].t = (((*rover->topheight - l) * yscale) + texturevpeg) * grTex->scaleY;
						wallVerts[1].t = (((*rover->topheight - lS) * yscale) + texturevpeg) * grTex->scaleY;
					}
					else
					{
						if (!dont_peg_bottom)  
						{
							wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
							wallVerts[0].t = (((h - l) * yscale) + texturevpeg) * grTex->scaleY;
							wallVerts[1].t = (((hS - lS) * yscale) + texturevpeg) * grTex->scaleY;
						}
						else  
						{
							wallVerts[0].t = wallVerts[1].t = texturevpeg * grTex->scaleY;
							wallVerts[3].t = wallVerts[0].t - ((h - l) * yscale) * grTex->scaleY;
							wallVerts[2].t = wallVerts[1].t - ((hS - lS) * yscale) * grTex->scaleY;
						}
					}

					wallVerts[0].s = wallVerts[3].s = sleft;
					wallVerts[2].s = wallVerts[1].s = sright;
				}

				FBITFIELD blendmode;

				if (rover->fofflags & FOF_FOG)
				{
					blendmode = PF_Fog|PF_NoTexture;

					lightnum = HWR_SideLightLevel(gl_sidedef, rover->master->frontsector->lightlevel);
					colormap = rover->master->frontsector->extra_colormap;
					lightnum = colormap ? lightnum : HWR_CalcWallLight(lightnum, vs.x, vs.y, ve.x, ve.y);

					Surf.PolyColor.s.alpha = HWR_FogBlockAlpha(rover->master->frontsector->lightlevel, rover->master->frontsector->extra_colormap);

					if (gl_backsector->numlights)
						HWR_SplitWall(gl_backsector, wallVerts, 0, &Surf, rover->fofflags, rover, blendmode);
					else
						HWR_AddTransparentWall(wallVerts, &Surf, 0, blendmode, true, lightnum, colormap);
				}
				else
				{
					blendmode = PF_Masked;

					if ((rover->fofflags & FOF_TRANSLUCENT && !((rover->fofflags & FOF_SPLAT) && rover->alpha >= 255)) || rover->blend)
					{
						blendmode = rover->blend ? HWR_GetBlendModeFlag(rover->blend) : PF_Translucent;
						Surf.PolyColor.s.alpha = max(0, min(rover->alpha, 255));
					}

					if (gl_backsector->numlights)
						HWR_SplitWall(gl_backsector, wallVerts, texnum, &Surf, rover->fofflags, rover, blendmode);
					else
					{
						if (blendmode != PF_Masked)
							HWR_AddTransparentWall(wallVerts, &Surf, texnum, blendmode, false, lightnum, colormap);
						else
							HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
					}
				}
			}
		}
	}
#undef SLOPEPARAMS

}





static boolean checkforemptylines = true;



static boolean CheckClip(seg_t * seg, sector_t * afrontsector, sector_t * abacksector)
{
	fixed_t frontf1,frontf2, frontc1, frontc2;  
	fixed_t backf1, backf2, backc1, backc2;  



	if (afrontsector->f_slope || afrontsector->c_slope || abacksector->f_slope || abacksector->c_slope)
	{
		fixed_t v1x, v1y, v2x, v2y;  
		if (gl_curline->pv1)
		{
			v1x = FLOAT_TO_FIXED(((polyvertex_t *)gl_curline->pv1)->x);
			v1y = FLOAT_TO_FIXED(((polyvertex_t *)gl_curline->pv1)->y);
		}
		else
		{
			v1x = gl_curline->v1->x;
			v1y = gl_curline->v1->y;
		}
		if (gl_curline->pv2)
		{
			v2x = FLOAT_TO_FIXED(((polyvertex_t *)gl_curline->pv2)->x);
			v2y = FLOAT_TO_FIXED(((polyvertex_t *)gl_curline->pv2)->y);
		}
		else
		{
			v2x = gl_curline->v2->x;
			v2y = gl_curline->v2->y;
		}
#define SLOPEPARAMS(slope, end1, end2, normalheight) \
		end1 = P_GetZAt(slope, v1x, v1y, normalheight); \
		end2 = P_GetZAt(slope, v2x, v2y, normalheight);

		SLOPEPARAMS(afrontsector->f_slope, frontf1, frontf2, afrontsector->  floorheight)
		SLOPEPARAMS(afrontsector->c_slope, frontc1, frontc2, afrontsector->ceilingheight)
		SLOPEPARAMS( abacksector->f_slope,  backf1,  backf2,  abacksector->  floorheight)
		SLOPEPARAMS( abacksector->c_slope,  backc1,  backc2,  abacksector->ceilingheight)
#undef SLOPEPARAMS
	}
	else
	{
		frontf1 = frontf2 = afrontsector->  floorheight;
		frontc1 = frontc2 = afrontsector->ceilingheight;
		backf1  =  backf2 =  abacksector->  floorheight;
		backc1  =  backc2 =  abacksector->ceilingheight;
	}


	if (!bothceilingssky && !bothfloorssky)
	{

		if ((backc1 <= frontf1 && backc2 <= frontf2)
			|| (backf1 >= frontc1 && backf2 >= frontc2))
		{
			checkforemptylines = false;
			return true;
		}

		if (backc1 <= backf1 && backc2 <= backf2)
		{

			if (((backc1 >= frontc1 && backc2 >= frontc2) || seg->sidedef->toptexture)
			&& ((backf1 <= frontf1 && backf2 <= frontf2) || seg->sidedef->bottomtexture))
			{
				checkforemptylines = false;
				return true;
			}
		}
	}

	if (!bothceilingssky) {
		if (backc1 != frontc1 || backc2 != frontc2)
		{
			checkforemptylines = false;
			return false;
		}
	}

	if (!bothfloorssky) {
		if (backf1 != frontf1 || backf2 != frontf2)
		{
			checkforemptylines = false;
			return false;
		}
	}

	return false;
}






static void HWR_AddLine(seg_t * line)
{
	angle_t angle1, angle2;
	boolean queued_main_portal = false;


	static sector_t tempsec;

	fixed_t v1x, v1y, v2x, v2y;  
	if (line->polyseg && !(line->polyseg->flags & POF_RENDERSIDES))
		return;

	gl_curline = line;

	if (gl_curline->pv1)
	{
		v1x = FLOAT_TO_FIXED(((polyvertex_t *)gl_curline->pv1)->x);
		v1y = FLOAT_TO_FIXED(((polyvertex_t *)gl_curline->pv1)->y);
	}
	else
	{
		v1x = gl_curline->v1->x;
		v1y = gl_curline->v1->y;
	}
	if (gl_curline->pv2)
	{
		v2x = FLOAT_TO_FIXED(((polyvertex_t *)gl_curline->pv2)->x);
		v2y = FLOAT_TO_FIXED(((polyvertex_t *)gl_curline->pv2)->y);
	}
	else
	{
		v2x = gl_curline->v2->x;
		v2y = gl_curline->v2->y;
	}


	angle1 = HWR_PointToAngleCached(v1x, v1y);
	angle2 = HWR_PointToAngleCached(v2x, v2y);


	if (angle2 - angle1 < ANGLE_180)
		return;

	                                                                        
                                                                        
                                                                           
                                                                           
                                                                           
	if (hwr_collect_main_portals && line->side == 0
	&& line->linedef->special == 40)
		queued_main_portal = HWR_QueueMainPortal(line->linedef, angle2, angle1);



	if (!gld_clipper_SafeCheckRange(angle2, angle1))
    {
		return;
    }

	                                                                         
                                                                         
                                                                          
                                                                         
	if (hwr_portal_pass && line->linedef == hwr_portal_destination_line)
		return;

	                                                                           
                                                                           
                                                                         
                                                                           
                                                                            
	if (hwr_portal_pass && line->side == 0
	&& line->linedef == hwr_nested_portal_source_line)
	{
		gld_clipper_SafeAddClipRange(angle2, angle1);
		return;
	}

	                                                                            
                                                                            
                                                                         
                                                                        
                                                                       
                                                                         
                                           
	if (queued_main_portal)
	{
		gld_clipper_SafeAddClipRange(angle2, angle1);
		return;
	}

	                                                                       
                                                                            
                                                                  
	if (HWR_PortalSurfaceActive() && line->side == 0
	&& HWR_IsVisibleMainPortal(line->linedef))
	{
		gld_clipper_SafeAddClipRange(angle2, angle1);
		return;
	}

	checkforemptylines = true;

	gl_backsector = line->backsector;
	bothceilingssky = bothfloorssky = false;

	if (!line->backsector)
    {
		gld_clipper_SafeAddClipRange(angle2, angle1);
    }
    else
    {
		gl_backsector = R_FakeFlat(gl_backsector, &tempsec, NULL, NULL, true);

		if (gl_backsector->ceilingpic == skyflatnum && gl_frontsector->ceilingpic == skyflatnum
		&& !(HWR_CeilingPortalSurfaceActive(gl_backsector)
			|| HWR_CeilingPortalSurfaceActive(gl_frontsector)))
			bothceilingssky = true;

		if (gl_backsector->floorpic == skyflatnum && gl_frontsector->floorpic == skyflatnum
		&& !(HWR_FloorPortalSurfaceActive(gl_backsector)
			|| HWR_FloorPortalSurfaceActive(gl_frontsector)))
			bothfloorssky = true;

		if (bothceilingssky && bothfloorssky)  
		{
			if (!line->polyseg &&
				!line->sidedef->midtexture
				&& ((!gl_frontsector->ffloors && !gl_backsector->ffloors)
					|| Tag_Compare(&gl_frontsector->tags, &gl_backsector->tags)))
				return;  

			HWR_ProcessSeg();  
			return;
		}

		if (CheckClip(line, gl_frontsector, gl_backsector))
		{
			gld_clipper_SafeAddClipRange(angle2, angle1);
			checkforemptylines = false;
		}




		if (checkforemptylines && R_IsEmptyLine(line, gl_frontsector, gl_backsector))
			return;
    }

	HWR_ProcessSeg();  
}








static boolean HWR_CheckBBox(fixed_t *bspcoord)
{
	const angle_t bboxmargin = (angle_t)1 << ANGLETOFINESHIFT;
	INT32 boxpos;
	fixed_t px1, py1, px2, py2;
	angle_t angle1, angle2;
	const fixed_t checkx = hwr_portal_pass ? hwr_bsp_viewx : viewx;
	const fixed_t checky = hwr_portal_pass ? hwr_bsp_viewy : viewy;



	if (checkx <= bspcoord[BOXLEFT])
		boxpos = 0;
	else if (checkx < bspcoord[BOXRIGHT])
		boxpos = 1;
	else
		boxpos = 2;

	if (checky >= bspcoord[BOXTOP])
		boxpos |= 0;
	else if (checky > bspcoord[BOXBOTTOM])
		boxpos |= 1<<2;
	else
		boxpos |= 2<<2;

	if (boxpos == 5)
		return true;

	px1 = bspcoord[checkcoord[boxpos][0]];
	py1 = bspcoord[checkcoord[boxpos][1]];
	px2 = bspcoord[checkcoord[boxpos][2]];
	py2 = bspcoord[checkcoord[boxpos][3]];

	if (hwr_portal_pass)
	{
		const fixed_t savedx = viewx, savedy = viewy;
		viewx = checkx;
		viewy = checky;
		angle1 = HWR_PointToAngleCached(px1, py1);
		angle2 = HWR_PointToAngleCached(px2, py2);
		viewx = savedx;
		viewy = savedy;
	}
	else
	{
		angle1 = HWR_PointToAngleCached(px1, py1);
		angle2 = HWR_PointToAngleCached(px2, py2);
	}
	if (gld_clipper_SafeCheckRange(angle2, angle1))
		return true;

	                                                                      
                                                                          
                                                                           
                                                                        
	return gld_clipper_SafeCheckRange(angle2 - bboxmargin,
		angle1 + bboxmargin);
}








static inline void HWR_AddPolyObjectSegs(void)
{
	size_t i, j;


	for (i = 0; i < numpolys; ++i)
	{

		for (j = 0; j < po_ptrs[i]->segCount; ++j)
			HWR_AddLine(po_ptrs[i]->segs[j]);
	}
}

static void HWR_RenderPolyObjectPlane(polyobj_t *polysector, boolean isceiling, fixed_t fixedheight,
									FBITFIELD blendmode, UINT8 lightlevel, levelflat_t *levelflat, sector_t *FOFsector,
									UINT8 alpha, extracolormap_t *planecolormap)
{
	FSurfaceInfo Surf;
	FOutVector *v3d;
	INT32 shader = SHADER_NONE;

	size_t nrPlaneVerts = polysector->numVertices;
	INT32 i;

	float height = FIXED_TO_FLOAT(fixedheight);  
	float fflatwidth = 64.0f, fflatheight = 64.0f;
	float xscale = 1.0f, yscale = 1.0f;

	float scrollx = 0.0f, scrolly = 0.0f;
	float tempxsow, tempytow, anglef = 0.0f;
	angle_t angle = 0;

	static FOutVector *planeVerts = NULL;
	static UINT16 numAllocedPlaneVerts = 0;

	if (!r_renderfloors || nrPlaneVerts < 3)    
		return;


	if (!planeVerts || nrPlaneVerts > numAllocedPlaneVerts)
	{
		numAllocedPlaneVerts = (UINT16)nrPlaneVerts;
		Z_Free(planeVerts);
		Z_Malloc(numAllocedPlaneVerts * sizeof (FOutVector), PU_LEVEL, &planeVerts);
	}


	if (levelflat != NULL)
	{
		texture_t *texture = textures[R_GetTextureNumForFlat(levelflat)];
		fflatwidth = texture->width;
		fflatheight = texture->height;
	}
	else  
		HWR_SetCurrentTexture(NULL);


	v3d = planeVerts;

	if (FOFsector != NULL)
	{
		if (!isceiling)  
		{
			xscale = FixedToFloat(FOFsector->floorxscale);
			yscale = FixedToFloat(FOFsector->flooryscale);
			scrollx = FixedToFloat(FOFsector->floorxoffset) / fflatwidth;
			scrolly = FixedToFloat(FOFsector->flooryoffset) / fflatheight;
			angle = FOFsector->floorangle;
		}
		else  
		{
			xscale = FixedToFloat(FOFsector->ceilingxscale);
			yscale = FixedToFloat(FOFsector->ceilingyscale);
			scrollx = FixedToFloat(FOFsector->ceilingxoffset) / fflatwidth;
			scrolly = FixedToFloat(FOFsector->ceilingyoffset) / fflatheight;
			angle = FOFsector->ceilingangle;
		}
	}
	else if (gl_frontsector)
	{
		if (!isceiling)  
		{
			xscale = FixedToFloat(gl_frontsector->floorxscale);
			yscale = FixedToFloat(gl_frontsector->flooryscale);
			scrollx = FixedToFloat(gl_frontsector->floorxoffset) / fflatwidth;
			scrolly = FixedToFloat(gl_frontsector->flooryoffset) / fflatheight;
			angle = gl_frontsector->floorangle;
		}
		else  
		{
			xscale = FixedToFloat(gl_frontsector->ceilingxscale);
			yscale = FixedToFloat(gl_frontsector->ceilingyscale);
			scrollx = FixedToFloat(gl_frontsector->ceilingxoffset) / fflatwidth;
			scrolly = FixedToFloat(gl_frontsector->ceilingyoffset) / fflatheight;
			angle = gl_frontsector->ceilingangle;
		}
	}

	anglef = ANG2RAD(InvAngle(angle));
	if (angle)
	{
		tempxsow = scrollx * fflatwidth;
		tempytow = scrolly * fflatheight;
		scrollx = ((tempxsow * cos(anglef)) - (tempytow * sin(anglef))) / fflatwidth;
		scrolly = ((tempxsow * sin(anglef)) + (tempytow * cos(anglef))) / fflatheight;
	}

	for (i = 0; i < (INT32)nrPlaneVerts; i++,v3d++)
	{


		v3d->s = FixedToFloat(polysector->origVerts[i].x);
		v3d->t = -FixedToFloat(polysector->origVerts[i].y);


		if (angle)  
		{
			tempxsow = v3d->s;
			tempytow = v3d->t;

			v3d->s = (tempxsow * cos(anglef)) - (tempytow * sin(anglef));
			v3d->t = (tempxsow * sin(anglef)) + (tempytow * cos(anglef));
		}

		v3d->s = ((v3d->s * xscale) / fflatwidth) + scrollx;
		v3d->t = ((v3d->t * yscale) / fflatheight) + scrolly;

		v3d->x = FIXED_TO_FLOAT(polysector->vertices[i]->x);
		v3d->y = height;
		v3d->z = FIXED_TO_FLOAT(polysector->vertices[i]->y);
	}

	HWR_Lighting(&Surf, lightlevel, planecolormap);

	if (blendmode & PF_Translucent)
	{
		Surf.PolyColor.s.alpha = (UINT8)alpha;
		blendmode |= PF_Modulated|PF_Occlude;
	}
	else
		blendmode |= PF_Masked|PF_Modulated;

	if (HWR_UseShader())
	{
		shader = SHADER_FLOOR;
		blendmode |= PF_ColorMapped;
	}

	HWR_ProcessPolygon(&Surf, planeVerts, nrPlaneVerts, blendmode, shader, false);
}

static void HWR_AddPolyObjectPlanes(void)
{
	size_t i;
	sector_t *polyobjsector;
	INT32 light = 0;




	for (i  = 0; i < numpolys; i++)
	{
		polyobjsector = po_ptrs[i]->lines[0]->backsector;  

		if (!(po_ptrs[i]->flags & POF_RENDERPLANES))  
			continue;

		if (po_ptrs[i]->translucency >= NUMTRANSMAPS)
			continue;

		if (polyobjsector->floorheight <= gl_frontsector->ceilingheight
			&& polyobjsector->floorheight >= gl_frontsector->floorheight
			&& (viewz < polyobjsector->floorheight))
		{
			light = R_GetPlaneLight(gl_frontsector, polyobjsector->floorheight, true);
			if (po_ptrs[i]->translucency > 0)
			{
				FSurfaceInfo Surf;
				FBITFIELD blendmode;
				memset(&Surf, 0x00, sizeof(Surf));
				blendmode = HWR_TranstableToAlpha(po_ptrs[i]->translucency, &Surf);
				HWR_AddTransparentPolyobjectFloor(&levelflats[polyobjsector->floorpic], po_ptrs[i], false, polyobjsector->floorheight,
													(light == -1 ? gl_frontsector->lightlevel : *gl_frontsector->lightlist[light].lightlevel), Surf.PolyColor.s.alpha, polyobjsector, blendmode, (light == -1 ? gl_frontsector->extra_colormap : *gl_frontsector->lightlist[light].extra_colormap));
			}
			else
			{
				HWR_GetLevelFlat(&levelflats[polyobjsector->floorpic], false);
				HWR_RenderPolyObjectPlane(po_ptrs[i], false, polyobjsector->floorheight, PF_Occlude,
										(light == -1 ? gl_frontsector->lightlevel : *gl_frontsector->lightlist[light].lightlevel), &levelflats[polyobjsector->floorpic],
										polyobjsector, 255, (light == -1 ? gl_frontsector->extra_colormap : *gl_frontsector->lightlist[light].extra_colormap));
			}
		}

		if (polyobjsector->ceilingheight >= gl_frontsector->floorheight
			&& polyobjsector->ceilingheight <= gl_frontsector->ceilingheight
			&& (viewz > polyobjsector->ceilingheight))
		{
			light = R_GetPlaneLight(gl_frontsector, polyobjsector->ceilingheight, true);
			if (po_ptrs[i]->translucency > 0)
			{
				FSurfaceInfo Surf;
				FBITFIELD blendmode;
				memset(&Surf, 0x00, sizeof(Surf));
				blendmode = HWR_TranstableToAlpha(po_ptrs[i]->translucency, &Surf);
				HWR_AddTransparentPolyobjectFloor(&levelflats[polyobjsector->ceilingpic], po_ptrs[i], true, polyobjsector->ceilingheight,
				                                  (light == -1 ? gl_frontsector->lightlevel : *gl_frontsector->lightlist[light].lightlevel), Surf.PolyColor.s.alpha, polyobjsector, blendmode, (light == -1 ? gl_frontsector->extra_colormap : *gl_frontsector->lightlist[light].extra_colormap));
			}
			else
			{
				HWR_GetLevelFlat(&levelflats[polyobjsector->ceilingpic], false);
				HWR_RenderPolyObjectPlane(po_ptrs[i], true, polyobjsector->ceilingheight, PF_Occlude,
				                          (light == -1 ? gl_frontsector->lightlevel : *gl_frontsector->lightlist[light].lightlevel), &levelflats[polyobjsector->ceilingpic],
				                          polyobjsector, 255, (light == -1 ? gl_frontsector->extra_colormap : *gl_frontsector->lightlist[light].extra_colormap));
			}
		}
	}
}

static FBITFIELD HWR_RippleBlend(sector_t *sector, ffloor_t *rover, boolean ceiling)
{
	(void)sector;
	(void)ceiling;
	return   (rover->fofflags & FOF_RIPPLE) ? PF_Ripple : 0;
}





static boolean HWR_DoCulling(line_t *cullheight, line_t *viewcullheight, float vz, float bottomh, float toph)
{
	float cullplane;

	if (!cullheight)
		return false;

	cullplane = FIXED_TO_FLOAT(cullheight->frontsector->floorheight);
	if (cullheight->args[1])  
	{
		if (!viewcullheight)
			return false;


		if (viewcullheight->frontsector == cullheight->frontsector)
		{

			if (vz > cullplane && toph < cullplane)  
				return true;

			if (bottomh > cullplane && vz <= cullplane)  
				return true;
		}
	}
	else  
	{
		if (vz > cullplane && toph < cullplane)  
			return true;

		if (bottomh > cullplane && vz <= cullplane)  
			return true;
	}

	return false;
}







static void HWR_Subsector(size_t num)
{
	INT16 count;
	seg_t *line;
	subsector_t *sub;
	static sector_t tempsec;  
	INT32 floorlightlevel;
	INT32 ceilinglightlevel;
	INT32 locFloorHeight, locCeilingHeight;
	INT32 cullFloorHeight, cullCeilingHeight;
	INT32 light = 0;
	extracolormap_t *floorcolormap;
	extracolormap_t *ceilingcolormap;
	ffloor_t *rover;

#ifdef PARANOIA  
	if (num >= addsubsector)
		I_Error("HWR_Subsector: ss %s with numss = %s, addss = %s\n",
			sizeu1(num), sizeu2(numsubsectors), sizeu3(addsubsector));

	                           
                                                 
              
                           
#endif

	if (num < numsubsectors)
	{

		sub = &subsectors[num];

		gl_frontsector = sub->sector;

		count = sub->numlines;

		line = &segs[sub->firstline];
	}
	else
	{

		sub = &subsectors[0];
		gl_frontsector = sub->sector;
		count = 0;
		line = NULL;
	}


	gl_frontsector = R_FakeFlat(gl_frontsector, &tempsec, &floorlightlevel, &ceilinglightlevel, false);

	floorcolormap = ceilingcolormap = gl_frontsector->extra_colormap;

	cullFloorHeight   = P_GetSectorFloorZAt  (gl_frontsector, viewx, viewy);
	cullCeilingHeight = P_GetSectorCeilingZAt(gl_frontsector, viewx, viewy);
	locFloorHeight    = P_GetSectorFloorZAt  (gl_frontsector, gl_frontsector->soundorg.x, gl_frontsector->soundorg.y);
	locCeilingHeight  = P_GetSectorCeilingZAt(gl_frontsector, gl_frontsector->soundorg.x, gl_frontsector->soundorg.y);

	R_CheckSectorLightLists(sub->sector, gl_frontsector, &floorlightlevel, &ceilinglightlevel, &floorcolormap, &ceilingcolormap);

	sub->sector->extra_colormap = gl_frontsector->extra_colormap;



	if (cullFloorHeight < viewz)
	{
		if (gl_frontsector->floorpic != skyflatnum
		&& !HWR_FloorPortalSurfaceActive(gl_frontsector))
		{
			if (sub->validcount != validcount)
			{
				HWR_GetLevelFlat(&levelflats[gl_frontsector->floorpic], false);
				HWR_RenderPlane(sub, &extrasubsectors[num], false,

					locFloorHeight == cullFloorHeight ? locFloorHeight : gl_frontsector->floorheight,

					PF_Occlude, floorlightlevel, &levelflats[gl_frontsector->floorpic], NULL, 255, floorcolormap);
			}
		}
	}

	if (cullCeilingHeight > viewz)
	{
		if (gl_frontsector->ceilingpic != skyflatnum
		&& !HWR_CeilingPortalSurfaceActive(gl_frontsector))
		{
			if (sub->validcount != validcount)
			{
				HWR_GetLevelFlat(&levelflats[gl_frontsector->ceilingpic], false);
				HWR_RenderPlane(sub, &extrasubsectors[num], true,

					locCeilingHeight == cullCeilingHeight ? locCeilingHeight : gl_frontsector->ceilingheight,

					PF_Occlude, ceilinglightlevel, &levelflats[gl_frontsector->ceilingpic], NULL, 255, ceilingcolormap);
			}
		}
	}


	if (gl_frontsector->ceilingpic == skyflatnum || gl_frontsector->floorpic == skyflatnum
	|| HWR_FloorPortalSurfaceActive(gl_frontsector)
	|| HWR_CeilingPortalSurfaceActive(gl_frontsector))
		drawsky = true;

	if (gl_frontsector->ffloors)
	{

		for (rover = gl_frontsector->ffloors;
			rover; rover = rover->next)
		{
			fixed_t bottomCullHeight, topCullHeight, centerHeight;

			if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_RENDERPLANES))
				continue;
			if (sub->validcount == validcount)
				continue;


			bottomCullHeight = P_GetFFloorBottomZAt(rover, viewx, viewy);
			topCullHeight = P_GetFFloorTopZAt(rover, viewx, viewy);

			if (gl_frontsector->cullheight)
			{
				if (HWR_DoCulling(gl_frontsector->cullheight, viewsector->cullheight, gl_viewz, FIXED_TO_FLOAT(*rover->bottomheight), FIXED_TO_FLOAT(*rover->topheight)))
					continue;
			}


			centerHeight = P_GetFFloorBottomZAt(rover, gl_frontsector->soundorg.x, gl_frontsector->soundorg.y);

			if (centerHeight <= locCeilingHeight &&
			    centerHeight >= locFloorHeight &&
			    ((viewz < bottomCullHeight && (rover->fofflags & FOF_BOTHPLANES || !(rover->fofflags & FOF_INVERTPLANES))) ||
			     (viewz > bottomCullHeight && (rover->fofflags & FOF_BOTHPLANES || rover->fofflags & FOF_INVERTPLANES))))
			{
				if (rover->fofflags & FOF_FOG)
				{
					UINT8 alpha;

					light = R_GetPlaneLight(gl_frontsector, centerHeight, viewz < bottomCullHeight ? true : false);
					alpha = HWR_FogBlockAlpha(HWR_FloorLightLevel(rover->master->frontsector, *gl_frontsector->lightlist[light].lightlevel), rover->master->frontsector->extra_colormap);

					HWR_AddTransparentFloor(0,
					                       &extrasubsectors[num],
										   false,
					                       *rover->bottomheight,
					                       HWR_FloorLightLevel(rover->master->frontsector, *gl_frontsector->lightlist[light].lightlevel),
					                       alpha, rover->master->frontsector, PF_Fog|PF_NoTexture,
										   true, false, rover->master->frontsector->extra_colormap);
				}
				else if ((rover->fofflags & FOF_TRANSLUCENT && !((rover->fofflags & FOF_SPLAT) && rover->alpha >= 255)) || rover->blend)  
				{
					light = R_GetPlaneLight(gl_frontsector, centerHeight, viewz < bottomCullHeight ? true : false);

					HWR_AddTransparentFloor(&levelflats[*rover->bottompic],
					                       &extrasubsectors[num],
										   false,
					                       *rover->bottomheight,
					                       HWR_FloorLightLevel(rover->master->frontsector, *gl_frontsector->lightlist[light].lightlevel),
					                       max(0, min(rover->alpha, 255)), rover->master->frontsector,
					                       HWR_RippleBlend(gl_frontsector, rover, false) | (rover->blend ? HWR_GetBlendModeFlag(rover->blend) : PF_Translucent),
					                       false, rover->fofflags & FOF_SPLAT, *gl_frontsector->lightlist[light].extra_colormap);
				}
				else
				{
					HWR_GetLevelFlat(&levelflats[*rover->bottompic], rover->fofflags & FOF_SPLAT);
					light = R_GetPlaneLight(gl_frontsector, centerHeight, viewz < bottomCullHeight ? true : false);
					HWR_RenderPlane(sub, &extrasubsectors[num], false, *rover->bottomheight, HWR_RippleBlend(gl_frontsector, rover, false)|PF_Occlude,
					                HWR_FloorLightLevel(rover->master->frontsector, *gl_frontsector->lightlist[light].lightlevel),
					                &levelflats[*rover->bottompic], rover->master->frontsector, 255, *gl_frontsector->lightlist[light].extra_colormap);
				}
			}


			centerHeight = P_GetFFloorTopZAt(rover, gl_frontsector->soundorg.x, gl_frontsector->soundorg.y);

			if (centerHeight >= locFloorHeight &&
			    centerHeight <= locCeilingHeight &&
			    ((viewz > topCullHeight && (rover->fofflags & FOF_BOTHPLANES || !(rover->fofflags & FOF_INVERTPLANES))) ||
			     (viewz < topCullHeight && (rover->fofflags & FOF_BOTHPLANES || rover->fofflags & FOF_INVERTPLANES))))
			{
				if (rover->fofflags & FOF_FOG)
				{
					UINT8 alpha;

					light = R_GetPlaneLight(gl_frontsector, centerHeight, viewz < topCullHeight ? true : false);
					alpha = HWR_FogBlockAlpha(HWR_CeilingLightLevel(rover->master->frontsector, *gl_frontsector->lightlist[light].lightlevel), rover->master->frontsector->extra_colormap);

					HWR_AddTransparentFloor(0,
					                       &extrasubsectors[num],
										   true,
					                       *rover->topheight,
					                       HWR_CeilingLightLevel(rover->master->frontsector, *gl_frontsector->lightlist[light].lightlevel),
					                       alpha, rover->master->frontsector, PF_Fog|PF_NoTexture,
										   true, false, rover->master->frontsector->extra_colormap);
				}
				else if ((rover->fofflags & FOF_TRANSLUCENT && !((rover->fofflags & FOF_SPLAT) && rover->alpha >= 255)) || rover->blend)
				{
					light = R_GetPlaneLight(gl_frontsector, centerHeight, viewz < topCullHeight ? true : false);

					HWR_AddTransparentFloor(&levelflats[*rover->toppic],
					                        &extrasubsectors[num],
											true,
					                        *rover->topheight,
					                        HWR_CeilingLightLevel(rover->master->frontsector, *gl_frontsector->lightlist[light].lightlevel),
					                        max(0, min(rover->alpha, 255)), rover->master->frontsector,
					                        HWR_RippleBlend(gl_frontsector, rover, false) | (rover->blend ? HWR_GetBlendModeFlag(rover->blend) : PF_Translucent),
					                        false, rover->fofflags & FOF_SPLAT, *gl_frontsector->lightlist[light].extra_colormap);
				}
				else
				{
					HWR_GetLevelFlat(&levelflats[*rover->toppic], rover->fofflags & FOF_SPLAT);
					light = R_GetPlaneLight(gl_frontsector, centerHeight, viewz < topCullHeight ? true : false);
					HWR_RenderPlane(sub, &extrasubsectors[num], true, *rover->topheight, HWR_RippleBlend(gl_frontsector, rover, false)|PF_Occlude,
					                  HWR_CeilingLightLevel(rover->master->frontsector, *gl_frontsector->lightlist[light].lightlevel),
					                  &levelflats[*rover->toppic], rover->master->frontsector, 255, *gl_frontsector->lightlist[light].extra_colormap);
				}
			}
		}
	}


	if (sub->polyList)
	{
		polyobj_t *po = sub->polyList;

		numpolys = 0;


		while (po)
		{
			++numpolys;
			po = (polyobj_t *)(po->link.next);
		}


		ps_numpolyobjects.value.i += numpolys;


		R_SortPolyObjects(sub);


		HWR_AddPolyObjectSegs();

		if (sub->validcount != validcount)  
		{

			HWR_AddPolyObjectPlanes();
		}
	}





	if (line)
	{


		HWR_AddSprites(gl_frontsector);







		sub->sector->validcount = validcount; 

		while (count--)
		{

			if (!line->glseg && !line->polyseg)  
				HWR_AddLine(line);
			line++;
		}
	}

	sub->validcount = validcount;
}







fixed_t *hwbbox;

static void HWR_RenderBSPNode(INT32 bspnum)
{
	node_t *bsp;
	INT32 side;

	ps_numbspcalls.value.i++;

	while (!(bspnum & NF_SUBSECTOR))
	{
		bsp = &nodes[bspnum];
		side = R_PointOnSide(hwr_portal_pass ? hwr_bsp_viewx : viewx,
			hwr_portal_pass ? hwr_bsp_viewy : viewy, bsp);


		hwbbox = bsp->bbox[side];


		HWR_RenderBSPNode(bsp->children[side]);


		                                                                        
                                                                       
                                                                       
		if (!(hwr_portal_pass && hwr_portal_cull_sector)
		&& !HWR_CheckBBox(bsp->bbox[side^1]))
			return;

		bspnum = bsp->children[side^1];
	}

	bspnum = (bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR);

	                                                                      
                                                                          
                                                                         
                                                                           
                                                                            
	if (hwr_portal_pass && hwr_portal_destination_line)
	{
		subsector_t *sub = &subsectors[bspnum];
		seg_t *seg = &segs[sub->firstline];
		INT16 count = sub->numlines;
		boolean entirely_behind = count > 0;

		while (count--)
		{
			if (!HWR_PointBehindPortalLine(seg->v1->x, seg->v1->y,
				hwr_portal_destination_line))
			{
				entirely_behind = false;
				break;
			}
			seg++;
		}

		if (entirely_behind)
			return;

		if (hwr_portal_cull_sector)
		{
			if (sub->sector != hwr_portal_cull_sector)
				return;
			hwr_portal_cull_sector = NULL;
		}
	}

	HWR_Subsector(bspnum);
}








static UINT32 gl_visspritecount;
static gl_vissprite_t *gl_visspritechunks[MAXVISSPRITES >> VISSPRITECHUNKBITS] = {NULL};





static void HWR_ClearSprites(void)
{
	gl_visspritecount = 0;
}




static gl_vissprite_t gl_overflowsprite;

static gl_vissprite_t *HWR_GetVisSprite(UINT32 num)
{
		UINT32 chunk = num >> VISSPRITECHUNKBITS;


		if (!gl_visspritechunks[chunk])
			Z_Malloc(sizeof(gl_vissprite_t) * VISSPRITESPERCHUNK, PU_LEVEL, &gl_visspritechunks[chunk]);

		return gl_visspritechunks[chunk] + (num & VISSPRITEINDEXMASK);
}

static gl_vissprite_t *HWR_NewVisSprite(void)
{
	if (gl_visspritecount == MAXVISSPRITES)
		return &gl_overflowsprite;

	return HWR_GetVisSprite(gl_visspritecount++);
}







typedef struct
{
	FOutVector verts[4];
	gl_vissprite_t *spr;
} zbuffersprite_t;


zbuffersprite_t linkdrawlist[MAXVISSPRITES];
UINT32 linkdrawcount = 0;

                                                                            
                                                                         
                                                                              
                                                                          
                                                                            
                                                                      
static zbuffersprite_t portal_linkdrawlist[MAXVISSPRITES];
static UINT32 portal_linkdrawcount = 0;


static void HWR_LinkDrawHackAdd(FOutVector *verts, gl_vissprite_t *spr)
{
	if (linkdrawcount < MAXVISSPRITES)
	{
		memcpy(linkdrawlist[linkdrawcount].verts, verts, sizeof(FOutVector) * 4);
		linkdrawlist[linkdrawcount].spr = spr;
		linkdrawcount++;
	}
}

static void HWR_PortalLinkDrawAdd(FOutVector *verts, gl_vissprite_t *spr)
{
	if (!HWR_PortalSurfaceActive() || !spr->mobj
	|| !(spr->mobj->flags2 & MF2_LINKDRAW) || !spr->mobj->tracer)
		return;

	if (portal_linkdrawcount < MAXVISSPRITES)
	{
		memcpy(portal_linkdrawlist[portal_linkdrawcount].verts, verts,
			sizeof(FOutVector) * 4);
		portal_linkdrawlist[portal_linkdrawcount].spr = spr;
		portal_linkdrawcount++;
	}
}

static void HWR_PortalLinkDrawFinish(void)
{
	UINT32 i;
	FSurfaceInfo surf;

	if (!portal_linkdrawcount)
		return;

	surf.PolyColor.rgba = 0xFFFFFFFF;
	surf.TintColor.rgba = 0xFFFFFFFF;
	surf.FadeColor.rgba = 0xFFFFFFFF;
	surf.LightInfo.light_level = 0;
	surf.LightInfo.fade_start = 0;
	surf.LightInfo.fade_end = 31;

	for (i = 0; i < portal_linkdrawcount; i++)
	{
		HWR_GetSpritePatch(portal_linkdrawlist[i].spr->gpatch);
		HWR_ProcessPolygon(&surf, portal_linkdrawlist[i].verts, 4,
			PF_Translucent|PF_Occlude|PF_Invisible, 0, false);
	}

	portal_linkdrawcount = 0;
	HWD.pfnSetBlend(PF_Translucent|PF_Occlude|PF_Masked);
}


static void HWR_LinkDrawHackFinish(void)
{
	UINT32 i;
	FSurfaceInfo surf;
	surf.PolyColor.rgba = 0xFFFFFFFF;
	surf.TintColor.rgba = 0xFFFFFFFF;
	surf.FadeColor.rgba = 0xFFFFFFFF;
	surf.LightInfo.light_level = 0;
	surf.LightInfo.fade_start = 0;
	surf.LightInfo.fade_end = 31;
	for (i = 0; i < linkdrawcount; i++)
	{

		HWR_GetSpritePatch(linkdrawlist[i].spr->gpatch);
		HWR_ProcessPolygon(&surf, linkdrawlist[i].verts, 4, PF_Translucent|PF_Occlude|PF_Invisible, 0, false);
	}

	linkdrawcount = 0;
}

static void HWR_DrawDropShadow(mobj_t *thing, fixed_t scale)
{
	patch_t *gpatch;
	FOutVector shadowVerts[4];
	FSurfaceInfo sSurf;
	float fscale; float fx; float fy; float offset;
	extracolormap_t *colormap = NULL;
	                                                                           
                                                                           
                                                                          
                                                                       
                 
	FBITFIELD blendmode = PF_Translucent|PF_Modulated|PF_Decal;
	INT32 shader = SHADER_NONE;
	UINT8 i;
	INT32 heightsec, phs;
	SINT8 flip = P_MobjFlip(thing);

	INT32 light;
	fixed_t scalemul;
	UINT16 alpha;
	fixed_t floordiff;
	fixed_t groundz;
	fixed_t slopez;
	pslope_t *groundslope;


	interpmobjstate_t interp = {0};

	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(thing, FRACUNIT, &interp);
	}

	groundz = R_GetShadowZ(thing, &groundslope);

	heightsec = thing->subsector->sector->heightsec;
	if (viewplayer->mo && viewplayer->mo->subsector)
		phs = viewplayer->mo->subsector->sector->heightsec;
	else
		phs = -1;

	if (heightsec != -1 && phs != -1)  
	{
		if (gl_viewz < FIXED_TO_FLOAT(sectors[phs].floorheight) ?
		thing->z >= sectors[heightsec].floorheight :
		thing->z < sectors[heightsec].floorheight)
			return;
		if (gl_viewz > FIXED_TO_FLOAT(sectors[phs].ceilingheight) ?
		thing->z < sectors[heightsec].ceilingheight && gl_viewz >= FIXED_TO_FLOAT(sectors[heightsec].ceilingheight) :
		thing->z >= sectors[heightsec].ceilingheight)
			return;
	}

	floordiff = abs((flip < 0 ? interp.height : 0) + interp.z - groundz);

	alpha = floordiff / (4*FRACUNIT) + 75;
	if (alpha >= 255) return;
	alpha = 255 - alpha;

	gpatch = (patch_t *)W_CachePatchName("DSHADOW", PU_SPRITE);
	if (!(gpatch && ((GLPatch_t *)gpatch->hardware)->mipmap->format)) return;
	HWR_GetSpritePatch(gpatch);

	scalemul = FixedMul(FRACUNIT - floordiff/640, scale);
	scalemul = FixedMul(scalemul, (interp.radius*2) / gpatch->height);

	fscale = FIXED_TO_FLOAT(scalemul);
	fx = FIXED_TO_FLOAT(interp.x);
	fy = FIXED_TO_FLOAT(interp.y);






	if (thing && fabsf(fscale - 1.0f) > 1.0E-36f)
		offset = ((gpatch->height)/2) * fscale;
	else
		offset = (float)((gpatch->height)/2);

	shadowVerts[2].x = shadowVerts[3].x = fx + offset;
	shadowVerts[1].x = shadowVerts[0].x = fx - offset;
	shadowVerts[1].z = shadowVerts[2].z = fy - offset;
	shadowVerts[0].z = shadowVerts[3].z = fy + offset;

	for (i = 0; i < 4; i++)
	{
		float oldx = shadowVerts[i].x;
		float oldy = shadowVerts[i].z;
		shadowVerts[i].x = fx + ((oldx - fx) * gl_viewcos) - ((oldy - fy) * gl_viewsin);
		shadowVerts[i].z = fy + ((oldx - fx) * gl_viewsin) + ((oldy - fy) * gl_viewcos);
	}

	if (groundslope)
	{
		for (i = 0; i < 4; i++)
		{
			slopez = P_GetSlopeZAt(groundslope, FLOAT_TO_FIXED(shadowVerts[i].x), FLOAT_TO_FIXED(shadowVerts[i].z));
			shadowVerts[i].y = FIXED_TO_FLOAT(slopez) + flip * 0.05f;
		}
	}
	else
	{
		for (i = 0; i < 4; i++)
			shadowVerts[i].y = FIXED_TO_FLOAT(groundz) + flip * 0.05f;
	}

	shadowVerts[0].s = shadowVerts[3].s = 0;
	shadowVerts[2].s = shadowVerts[1].s = ((GLPatch_t *)gpatch->hardware)->max_s;

	shadowVerts[3].t = shadowVerts[2].t = 0;
	shadowVerts[0].t = shadowVerts[1].t = ((GLPatch_t *)gpatch->hardware)->max_t;

	if (!(thing->renderflags & RF_NOCOLORMAPS))
	{
		if (thing->subsector->sector->numlights)
		{

			light = R_GetPlaneLight(thing->subsector->sector, groundz, false);

			if (*thing->subsector->sector->lightlist[light].extra_colormap)
				colormap = *thing->subsector->sector->lightlist[light].extra_colormap;
		}
		else if (thing->subsector->sector->extra_colormap)
			colormap = thing->subsector->sector->extra_colormap;
	}

	HWR_Lighting(&sSurf, 0, colormap);
	sSurf.PolyColor.s.alpha = FixedMul(thing->alpha, alpha);

	if (HWR_UseShader())
	{
		shader = SHADER_SPRITE;
		blendmode |= PF_ColorMapped;
	}

	HWR_ProcessPolygon(&sSurf, shadowVerts, 4, blendmode, shader, false);
}


static void HWR_RotateSpritePolyToAim(gl_vissprite_t *spr, FOutVector *wallVerts, const boolean precip)
{
	if (cv_glspritebillboarding.value
		&& spr && spr->mobj && !R_ThingIsPaperSprite(spr->mobj)
		&& wallVerts)
	{

		interpmobjstate_t interp = {0};
		float basey, lowy;


		if (R_UsingFrameInterpolation() && !paused)
		{
			if (precip)
			{
				R_InterpolatePrecipMobjState((precipmobj_t *)spr->mobj, rendertimefrac, &interp);
			}
			else
			{
				R_InterpolateMobjState(spr->mobj, rendertimefrac, &interp);
			}
		}
		else
		{
			if (precip)
			{
				R_InterpolatePrecipMobjState((precipmobj_t *)spr->mobj, FRACUNIT, &interp);
			}
			else
			{
				R_InterpolateMobjState(spr->mobj, FRACUNIT, &interp);
			}
		}

		if (P_MobjFlip(spr->mobj) == -1)
		{
			basey = FIXED_TO_FLOAT(interp.z + interp.height);
		}
		else
		{
			basey = FIXED_TO_FLOAT(interp.z);
		}
		lowy = wallVerts[0].y;





		wallVerts[2].y = wallVerts[3].y = (spr->gzt - basey) * gl_viewludsin + basey;
		wallVerts[0].y = wallVerts[1].y = (lowy - basey) * gl_viewludsin + basey;

		wallVerts[3].x += ((spr->gzt - basey) * gl_viewludcos) * gl_viewcos;
		wallVerts[2].x += ((spr->gzt - basey) * gl_viewludcos) * gl_viewcos;

		wallVerts[0].x += ((lowy - basey) * gl_viewludcos) * gl_viewcos;
		wallVerts[1].x += ((lowy - basey) * gl_viewludcos) * gl_viewcos;

		wallVerts[3].z += ((spr->gzt - basey) * gl_viewludcos) * gl_viewsin;
		wallVerts[2].z += ((spr->gzt - basey) * gl_viewludcos) * gl_viewsin;

		wallVerts[0].z += ((lowy - basey) * gl_viewludcos) * gl_viewsin;
		wallVerts[1].z += ((lowy - basey) * gl_viewludcos) * gl_viewsin;
	}
}

static INT32 HWR_GetSpriteBlendMode(const mobj_t *mobj)
{
	if (mobj->frame & FF_BLENDMASK)
		return ((mobj->frame & FF_BLENDMASK) >> FF_BLENDSHIFT) + 1;

	return mobj->blendmode;
}

static fixed_t HWR_GetSpriteAlpha(const mobj_t *mobj)
{
	if ((mobj->flags2 & MF2_LINKDRAW) && mobj->tracer)
		return mobj->tracer->alpha;

	return mobj->alpha;
}

static boolean HWR_SpriteUsesBlending(const mobj_t *mobj)
{
	const INT32 blendmode = HWR_GetSpriteBlendMode(mobj);

	return (mobj->flags2 & MF2_SHADOW)
		|| (mobj->frame & FF_TRANSMASK)
		|| HWR_GetSpriteAlpha(mobj) < FRACUNIT
		|| (blendmode > AST_TRANSLUCENT && blendmode < AST_OVERLAY);
}

static void HWR_SplitSprite(gl_vissprite_t *spr)
{
	FOutVector wallVerts[4];
	FOutVector baseWallVerts[4];  
	patch_t *gpatch;
	FSurfaceInfo Surf;
	extracolormap_t *colormap = NULL;
	FUINT lightlevel;
	boolean lightset = true;
	FBITFIELD blend = 0;
	FBITFIELD occlusion;
	INT32 shader = SHADER_NONE;
	boolean use_linkdraw_hack = false;
	UINT8 alpha;

	INT32 i;
	float realtop, realbot, top, bot;
	float ttop, tbot, tmult;
	float bheight;
	float realheight, heightmult;
	const sector_t *sector = spr->mobj->subsector->sector;
	const lightlist_t *list = sector->lightlist;
	float endrealtop, endrealbot, endtop, endbot;
	float endbheight;
	float endrealheight;
	fixed_t temp;
	fixed_t v1x, v1y, v2x, v2y;

	gpatch = spr->gpatch;
	if (!gpatch)
		return;




	HWR_GetMappedSpritePatch(gpatch, spr->colormap);

	baseWallVerts[0].x = baseWallVerts[3].x = spr->x1;
	baseWallVerts[2].x = baseWallVerts[1].x = spr->x2;
	baseWallVerts[0].z = baseWallVerts[3].z = spr->z1;
	baseWallVerts[1].z = baseWallVerts[2].z = spr->z2;

	baseWallVerts[2].y = baseWallVerts[3].y = spr->gzt;
	baseWallVerts[0].y = baseWallVerts[1].y = spr->gz;

	v1x = FLOAT_TO_FIXED(spr->x1);
	v1y = FLOAT_TO_FIXED(spr->z1);
	v2x = FLOAT_TO_FIXED(spr->x2);
	v2y = FLOAT_TO_FIXED(spr->z2);

	if (spr->flip)
	{
		baseWallVerts[0].s = baseWallVerts[3].s = ((GLPatch_t *)gpatch->hardware)->max_s;
		baseWallVerts[2].s = baseWallVerts[1].s = 0;
	}
	else
	{
		baseWallVerts[0].s = baseWallVerts[3].s = 0;
		baseWallVerts[2].s = baseWallVerts[1].s = ((GLPatch_t *)gpatch->hardware)->max_s;
	}


	if (spr->vflip)
	{
		baseWallVerts[3].t = baseWallVerts[2].t = ((GLPatch_t *)gpatch->hardware)->max_t;
		baseWallVerts[0].t = baseWallVerts[1].t = 0;
	}
	else
	{
		baseWallVerts[3].t = baseWallVerts[2].t = 0;
		baseWallVerts[0].t = baseWallVerts[1].t = ((GLPatch_t *)gpatch->hardware)->max_t;
	}


	if (spr->dispoffset) {
		float co = -gl_viewcos*(0.05f*spr->dispoffset);
		float si = -gl_viewsin*(0.05f*spr->dispoffset);
		baseWallVerts[0].z = baseWallVerts[3].z = baseWallVerts[0].z+si;
		baseWallVerts[1].z = baseWallVerts[2].z = baseWallVerts[1].z+si;
		baseWallVerts[0].x = baseWallVerts[3].x = baseWallVerts[0].x+co;
		baseWallVerts[1].x = baseWallVerts[2].x = baseWallVerts[1].x+co;
	}


	HWR_RotateSpritePolyToAim(spr, baseWallVerts, false);

	realtop = top = baseWallVerts[3].y;
	realbot = bot = baseWallVerts[0].y;
	ttop = baseWallVerts[3].t;
	tbot = baseWallVerts[0].t;
	tmult = (tbot - ttop) / (top - bot);

	endrealtop = endtop = baseWallVerts[2].y;
	endrealbot = endbot = baseWallVerts[1].y;




	memcpy(wallVerts, baseWallVerts, sizeof(baseWallVerts));

	const boolean linkdraw = (spr->mobj->flags2 & MF2_LINKDRAW) && spr->mobj->tracer;
	fixed_t newalpha = HWR_GetSpriteAlpha(spr->mobj);



	occlusion = (linkdraw || HWR_SpriteUsesBlending(spr->mobj)) ? 0 : PF_Occlude;

	INT32 blendmode = HWR_GetSpriteBlendMode(spr->mobj);

	if (!cv_translucency.value)  
	{
		Surf.PolyColor.s.alpha = 0xFF;
		blend = PF_Translucent|occlusion;
		if (linkdraw) use_linkdraw_hack = true;
	}
	else if (spr->mobj->flags2 & MF2_SHADOW)
	{
		Surf.PolyColor.s.alpha = 0x40;
		blend = HWR_GetBlendModeFlag(blendmode);
	}
	else if (spr->mobj->frame & FF_TRANSMASK)
	{
		INT32 trans = (spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT;
		blend = HWR_SurfaceBlend(blendmode, trans, &Surf);
	}
	else
	{




		Surf.PolyColor.s.alpha = 0xFF;
		blend = ((newalpha < FRACUNIT && (blendmode <= AST_COPY || blendmode >= AST_OVERLAY))
			? PF_Translucent : HWR_GetBlendModeFlag(blendmode)) | occlusion;
		if (linkdraw) use_linkdraw_hack = true;
	}

	Surf.PolyColor.s.alpha = FixedMul(newalpha, Surf.PolyColor.s.alpha);

	if (HWR_UseShader())
	{
		shader = SHADER_SPRITE;
		blend |= PF_ColorMapped;
	}

	alpha = Surf.PolyColor.s.alpha;


	lightlevel = *list[sector->numlights - 1].lightlevel;
	if (!(spr->mobj->renderflags & RF_NOCOLORMAPS))
		colormap = *list[sector->numlights - 1].extra_colormap;

	i = 0;
	temp = FLOAT_TO_FIXED(realtop);

	if (R_ThingIsFullBright(spr->mobj))
		lightlevel = 255;
	else if (R_ThingIsFullDark(spr->mobj))
		lightlevel = 0;
	else
		lightset = false;

	for (i = 1; i < sector->numlights; i++)
	{
		fixed_t h = P_GetLightZAt(&sector->lightlist[i], spr->mobj->x, spr->mobj->y);
		if (h <= temp)
		{
			if (!lightset)
				lightlevel = max(min(*list[i-1].lightlevel, 255), 0);
			if (!(spr->mobj->renderflags & RF_NOCOLORMAPS))
				colormap = *list[i-1].extra_colormap;
			break;
		}
	}

	if (R_ThingIsSemiBright(spr->mobj))
		lightlevel = 128 + (lightlevel>>1);

	for (i = 0; i < sector->numlights; i++)
	{
		if (endtop < endrealbot && top < realbot)
			return;


		if (!(list[i].flags & FOF_NOSHADE) && (list[i].flags & FOF_CUTSPRITES))
		{
			if (!lightset)
				lightlevel = max(min(*list[i].lightlevel, 255), 0);
			if (!(spr->mobj->renderflags & RF_NOCOLORMAPS))
				colormap = *list[i].extra_colormap;
		}

		if (i + 1 < sector->numlights)
		{
			temp = P_GetLightZAt(&list[i+1], v1x, v1y);
			bheight = FIXED_TO_FLOAT(temp);
			temp = P_GetLightZAt(&list[i+1], v2x, v2y);
			endbheight = FIXED_TO_FLOAT(temp);
		}
		else
		{
			bheight = realbot;
			endbheight = endrealbot;
		}

		if (endbheight >= endtop && bheight >= top)
			continue;

		bot = bheight;

		if (bot < realbot)
			bot = realbot;

		endbot = endbheight;

		if (endbot < endrealbot)
			endbot = endrealbot;

		wallVerts[3].t = ttop + ((realtop - top) * tmult);
		wallVerts[2].t = ttop + ((endrealtop - endtop) * tmult);
		wallVerts[0].t = ttop + ((realtop - bot) * tmult);
		wallVerts[1].t = ttop + ((endrealtop - endbot) * tmult);

		wallVerts[3].y = top;
		wallVerts[2].y = endtop;
		wallVerts[0].y = bot;
		wallVerts[1].y = endbot;


		if (cv_glspritebillboarding.value
			&& spr->mobj && !R_ThingIsPaperSprite(spr->mobj))
		{

			realheight = realbot - realtop;
			endrealheight = endrealbot - endrealtop;
			heightmult = (realtop - top) / realheight;
			wallVerts[3].x = baseWallVerts[3].x + (baseWallVerts[3].x - baseWallVerts[0].x) * heightmult;
			wallVerts[3].z = baseWallVerts[3].z + (baseWallVerts[3].z - baseWallVerts[0].z) * heightmult;

			heightmult = (endrealtop - endtop) / endrealheight;
			wallVerts[2].x = baseWallVerts[2].x + (baseWallVerts[2].x - baseWallVerts[1].x) * heightmult;
			wallVerts[2].z = baseWallVerts[2].z + (baseWallVerts[2].z - baseWallVerts[1].z) * heightmult;

			heightmult = (realtop - bot) / realheight;
			wallVerts[0].x = baseWallVerts[3].x + (baseWallVerts[3].x - baseWallVerts[0].x) * heightmult;
			wallVerts[0].z = baseWallVerts[3].z + (baseWallVerts[3].z - baseWallVerts[0].z) * heightmult;

			heightmult = (endrealtop - endbot) / endrealheight;
			wallVerts[1].x = baseWallVerts[2].x + (baseWallVerts[2].x - baseWallVerts[1].x) * heightmult;
			wallVerts[1].z = baseWallVerts[2].z + (baseWallVerts[2].z - baseWallVerts[1].z) * heightmult;
		}

		HWR_Lighting(&Surf, lightlevel, colormap);

		Surf.PolyColor.s.alpha = alpha;

		HWR_ProcessPolygon(&Surf, wallVerts, 4, blend|PF_Modulated, shader, false);
		HWR_PortalLinkDrawAdd(wallVerts, spr);

		if (use_linkdraw_hack)
			HWR_LinkDrawHackAdd(wallVerts, spr);

		top = bot;
		endtop = endbot;
	}

	bot = realbot;
	endbot = endrealbot;
	if (endtop <= endrealbot && top <= realbot)
		return;


	wallVerts[3].t = ttop + ((realtop - top) * tmult);
	wallVerts[2].t = ttop + ((endrealtop - endtop) * tmult);
	wallVerts[0].t = ttop + ((realtop - bot) * tmult);
	wallVerts[1].t = ttop + ((endrealtop - endbot) * tmult);

	wallVerts[3].y = top;
	wallVerts[2].y = endtop;
	wallVerts[0].y = bot;
	wallVerts[1].y = endbot;

	HWR_Lighting(&Surf, lightlevel, colormap);

	Surf.PolyColor.s.alpha = alpha;

	HWR_ProcessPolygon(&Surf, wallVerts, 4, blend|PF_Modulated, shader, false);
	HWR_PortalLinkDrawAdd(wallVerts, spr);

	if (use_linkdraw_hack)
		HWR_LinkDrawHackAdd(wallVerts, spr);
}

static void HWR_DrawBoundingBox(gl_vissprite_t *vis)
{
	FOutVector v[24];
	FSurfaceInfo Surf = {0};
	RGBA_t *palette = HWR_GetTexturePalette();





















	v[ 0].x = v[ 5].x = v[13].x = v[14].x = v[15].x = v[16].x =
		v[18].x = v[19].x = v[20].x = v[21].x = v[22].x = v[23].x = vis->x1;  

	v[ 1].x = v[ 2].x = v[ 3].x = v[ 4].x = v[ 6].x = v[ 7].x =
		v[ 8].x = v[ 9].x = v[10].x = v[11].x = v[12].x = v[17].x = vis->x2;  

	v[ 0].z = v[ 1].z = v[ 2].z = v[ 3].z = v[ 4].z = v[ 5].z =
		v[ 6].z = v[11].z = v[19].z = v[20].z = v[21].z = v[22].z = vis->z1;  

	v[ 7].z = v[ 8].z = v[ 9].z = v[10].z = v[12].z = v[13].z =
		v[14].z = v[15].z = v[16].z = v[17].z = v[18].z = v[23].z = vis->z2;  

	v[ 0].y = v[ 1].y = v[ 2].y = v[ 6].y = v[ 7].y = v[ 8].y =
		v[12].y = v[13].y = v[14].y = v[18].y = v[19].y = v[20].y = vis->gz;  

	v[ 3].y = v[ 4].y = v[ 5].y = v[ 9].y = v[10].y = v[11].y =
		v[15].y = v[16].y = v[17].y = v[21].y = v[22].y = v[23].y = vis->gzt;  

	Surf.PolyColor = palette[R_GetBoundingBoxColor(vis->mobj)];

	HWR_ProcessPolygon(&Surf, v, 24, (cv_renderhitboxgldepth.value ? 0 : PF_NoDepthTest)|PF_Modulated|PF_NoTexture|PF_WireFrame, SHADER_NONE, false);
}






static void HWR_DrawSprite(gl_vissprite_t *spr)
{
	FOutVector wallVerts[4];
	patch_t *gpatch;
	FSurfaceInfo Surf;
	const boolean splat = R_ThingIsFloorSprite(spr->mobj);

	if (!spr->mobj)
		return;

	if (!spr->mobj->subsector)
		return;

	if (spr->mobj->subsector->sector->numlights && !splat)
	{
		HWR_SplitSprite(spr);
		return;
	}







	gpatch = spr->gpatch;
	if (!gpatch)
		return;

#ifdef ALAM_LIGHTING
	if (!(spr->mobj->flags2 & MF2_DEBRIS) && (spr->mobj->sprite != SPR_PLAY ||
	 (spr->mobj->player && spr->mobj->player->powers[pw_super])))
		HWR_DL_AddLight(spr, gpatch);
#endif








	if (splat)
	{
		F2DCoord verts[4];
		F2DCoord rotated[4];

		angle_t angle;
		float ca, sa;
		float w, h;
		float xscale, yscale;
		float xoffset, yoffset;
		float leftoffset, topoffset;
		float zoffset = (P_MobjFlip(spr->mobj) * 0.05f);
		pslope_t *splatslope = NULL;
		INT32 i;

		renderflags_t renderflags = spr->renderflags;

		if (spr->rotateflags & SRF_3D || renderflags & RF_NOSPLATBILLBOARD)
			angle = spr->mobj->angle;
		else
			angle = viewangle;

		if (!spr->rotated)
			angle += spr->mobj->spriteroll;

		angle = -angle;
		angle += ANGLE_90;

		topoffset = spr->spriteyoffset;
		leftoffset = spr->spritexoffset;
		if (spr->flip)
			leftoffset = ((float)gpatch->width - leftoffset);

		xscale = spr->scale * spr->spritexscale;
		yscale = spr->scale * spr->spriteyscale;

		xoffset = leftoffset * xscale;
		yoffset = topoffset * yscale;

		w = (float)gpatch->width * xscale;
		h = (float)gpatch->height * yscale;







		verts[3].x = -xoffset;
		verts[3].y = yoffset;

		verts[2].x = w - xoffset;
		verts[2].y = yoffset;

		verts[1].x = w - xoffset;
		verts[1].y = -h + yoffset;

		verts[0].x = -xoffset;
		verts[0].y = -h + yoffset;

		ca = FIXED_TO_FLOAT(FINECOSINE((-angle)>>ANGLETOFINESHIFT));
		sa = FIXED_TO_FLOAT(FINESINE((-angle)>>ANGLETOFINESHIFT));


		for (i = 0; i < 4; i++)
		{
			rotated[i].x = (verts[i].x * ca) - (verts[i].y * sa);
			rotated[i].y = (verts[i].x * sa) + (verts[i].y * ca);
		}


		for (i = 0; i < 4; i++)
		{
			wallVerts[i].x = rotated[i].x + FIXED_TO_FLOAT(spr->mobj->x);
			wallVerts[i].z = rotated[i].y + FIXED_TO_FLOAT(spr->mobj->y);
		}

		if (renderflags & (RF_SLOPESPLAT | RF_OBJECTSLOPESPLAT))
		{
			pslope_t *standingslope = spr->mobj->standingslope;  


			if (renderflags & RF_SLOPESPLAT)
				splatslope = spr->mobj->floorspriteslope;

			if (standingslope && (renderflags & RF_OBJECTSLOPESPLAT))
				splatslope = standingslope;
		}


		if (splatslope)
		{
			for (i = 0; i < 4; i++)
			{
				fixed_t slopez = P_GetSlopeZAt(splatslope, FLOAT_TO_FIXED(wallVerts[i].x), FLOAT_TO_FIXED(wallVerts[i].z));
				wallVerts[i].y = FIXED_TO_FLOAT(slopez) + zoffset;
			}
		}
		else
		{
			for (i = 0; i < 4; i++)
				wallVerts[i].y = FIXED_TO_FLOAT(spr->gz) + zoffset;
		}
	}
	else
	{

		wallVerts[0].x = wallVerts[3].x = spr->x1;
		wallVerts[2].x = wallVerts[1].x = spr->x2;
		wallVerts[2].y = wallVerts[3].y = spr->gzt;
		wallVerts[0].y = wallVerts[1].y = spr->gz;



		wallVerts[0].z = wallVerts[3].z = spr->z1;
		wallVerts[1].z = wallVerts[2].z = spr->z2;
	}




	HWR_GetMappedSpritePatch(gpatch, spr->colormap);

	if (spr->flip)
	{
		wallVerts[0].s = wallVerts[3].s = ((GLPatch_t *)gpatch->hardware)->max_s;
		wallVerts[2].s = wallVerts[1].s = 0;
	}else{
		wallVerts[0].s = wallVerts[3].s = 0;
		wallVerts[2].s = wallVerts[1].s = ((GLPatch_t *)gpatch->hardware)->max_s;
	}


	if (spr->vflip)
	{
		wallVerts[3].t = wallVerts[2].t = ((GLPatch_t *)gpatch->hardware)->max_t;
		wallVerts[0].t = wallVerts[1].t = 0;
	}else{
		wallVerts[3].t = wallVerts[2].t = 0;
		wallVerts[0].t = wallVerts[1].t = ((GLPatch_t *)gpatch->hardware)->max_t;
	}

	if (!splat)
	{

		if (spr->dispoffset) {
			float co = -gl_viewcos*(0.05f*spr->dispoffset);
			float si = -gl_viewsin*(0.05f*spr->dispoffset);
			wallVerts[0].z = wallVerts[3].z = wallVerts[0].z+si;
			wallVerts[1].z = wallVerts[2].z = wallVerts[1].z+si;
			wallVerts[0].x = wallVerts[3].x = wallVerts[0].x+co;
			wallVerts[1].x = wallVerts[2].x = wallVerts[1].x+co;
		}


		HWR_RotateSpritePolyToAim(spr, wallVerts, false);
	}






	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 0;
		boolean lightset = true;
		extracolormap_t *colormap = NULL;

		if (R_ThingIsFullBright(spr->mobj))
			lightlevel = 255;
		else if (R_ThingIsFullDark(spr->mobj))
			lightlevel = 0;
		else
			lightset = false;

		if (!(spr->mobj->renderflags & RF_NOCOLORMAPS))
			colormap = sector->extra_colormap;

		if (splat && sector->numlights)
		{
			INT32 light = R_GetPlaneLight(sector, spr->mobj->z, false);

			if (!lightset)
				lightlevel = max(min(*sector->lightlist[light].lightlevel, 255), 0);

			if (*sector->lightlist[light].extra_colormap && !(spr->mobj->renderflags & RF_NOCOLORMAPS))
				colormap = *sector->lightlist[light].extra_colormap;
		}
		else if (!lightset)
			lightlevel = max(min(sector->lightlevel, 255), 0);

		if (R_ThingIsSemiBright(spr->mobj))
			lightlevel = 128 + (lightlevel>>1);

		HWR_Lighting(&Surf, lightlevel, colormap);
	}

	{
		INT32 shader = SHADER_NONE;
		FBITFIELD blend = 0;
		FBITFIELD occlusion;
		boolean use_linkdraw_hack = false;
		const boolean linkdraw = (spr->mobj->flags2 & MF2_LINKDRAW) && spr->mobj->tracer;
		fixed_t newalpha = HWR_GetSpriteAlpha(spr->mobj);



		occlusion = (linkdraw || HWR_SpriteUsesBlending(spr->mobj)) ? 0 : PF_Occlude;

		INT32 blendmode = HWR_GetSpriteBlendMode(spr->mobj);

		if (!cv_translucency.value)  
		{
			Surf.PolyColor.s.alpha = 0xFF;
			blend = PF_Translucent|occlusion;
			if (linkdraw) use_linkdraw_hack = true;
		}
		else if (spr->mobj->flags2 & MF2_SHADOW)
		{
			Surf.PolyColor.s.alpha = 0x40;
			blend = HWR_GetBlendModeFlag(blendmode);
		}
		else if (spr->mobj->frame & FF_TRANSMASK)
		{
			INT32 trans = (spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT;
			blend = HWR_SurfaceBlend(blendmode, trans, &Surf);
		}
		else
		{




			Surf.PolyColor.s.alpha = 0xFF;
			blend = ((newalpha < FRACUNIT && (blendmode <= AST_COPY || blendmode >= AST_OVERLAY))
				? PF_Translucent : HWR_GetBlendModeFlag(blendmode)) | occlusion;
			if (linkdraw) use_linkdraw_hack = true;
		}

		Surf.PolyColor.s.alpha = FixedMul(newalpha, Surf.PolyColor.s.alpha);

		if (spr->renderflags & RF_SHADOWEFFECTS)
		{
			INT32 alpha = Surf.PolyColor.s.alpha;
			alpha -= ((INT32)(spr->shadowheight / 4.0f)) + 75;
			if (alpha < 1)
				return;

			Surf.PolyColor.s.alpha = (UINT8)(alpha);
			blend = PF_Translucent|occlusion;
			if (linkdraw) use_linkdraw_hack = true;
		}

		if (HWR_UseShader())
		{
			shader = SHADER_SPRITE;
			blend |= PF_ColorMapped;
		}

		HWR_ProcessPolygon(&Surf, wallVerts, 4, blend|PF_Modulated, shader, false);
		HWR_PortalLinkDrawAdd(wallVerts, spr);

		if (use_linkdraw_hack)
			HWR_LinkDrawHackAdd(wallVerts, spr);
	}
}


static inline void HWR_DrawPrecipitationSprite(gl_vissprite_t *spr)
{
	INT32 shader = SHADER_NONE;
	FBITFIELD blend = 0;
	FOutVector wallVerts[4];
	patch_t *gpatch;
	FSurfaceInfo Surf;

	if (!spr->mobj)
		return;

	if (!spr->mobj->subsector)
		return;


	gpatch = spr->gpatch;
	if (!gpatch)
		return;

	                                                                        
                                                                            
                                                                    
	HWR_GetMappedSpritePatch(gpatch, spr->colormap);







	wallVerts[0].x = wallVerts[3].x = spr->x1;
	wallVerts[2].x = wallVerts[1].x = spr->x2;
	wallVerts[2].y = wallVerts[3].y = spr->gzt;
	wallVerts[0].y = wallVerts[1].y = spr->gz;



	wallVerts[0].z = wallVerts[3].z = spr->z1;
	wallVerts[1].z = wallVerts[2].z = spr->z2;


	HWR_RotateSpritePolyToAim(spr, wallVerts, true);

	wallVerts[0].s = wallVerts[3].s = 0;
	wallVerts[2].s = wallVerts[1].s = ((GLPatch_t *)gpatch->hardware)->max_s;

	wallVerts[3].t = wallVerts[2].t = 0;
	wallVerts[0].t = wallVerts[1].t = ((GLPatch_t *)gpatch->hardware)->max_t;


	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 255;
		extracolormap_t *colormap = sector->extra_colormap;

		if (sector->numlights)
		{

			INT32 light = R_GetPlaneLight(sector, spr->mobj->z + spr->mobj->height, false);

			if (!R_ThingIsFullBright(spr->mobj))
				lightlevel = max(min(*sector->lightlist[light].lightlevel, 255), 0);

			if (*sector->lightlist[light].extra_colormap)
				colormap = *sector->lightlist[light].extra_colormap;
		}
		else
		{
			if (!R_ThingIsFullBright(spr->mobj))
				lightlevel = max(min(sector->lightlevel, 255), 0);

			if (sector->extra_colormap)
				colormap = sector->extra_colormap;
		}

		HWR_Lighting(&Surf, lightlevel, colormap);
	}

	if (spr->mobj->frame & FF_TRANSMASK)
	{
		INT32 trans = (spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT;
		blend = HWR_SurfaceBlend(AST_TRANSLUCENT, trans, &Surf);
	}
	else
	{




		Surf.PolyColor.s.alpha = 0xFF;
		blend = HWR_GetBlendModeFlag(spr->mobj->blendmode)|PF_Occlude;
	}

	if (HWR_UseShader())
	{
		shader = SHADER_SPRITE;
		blend |= PF_ColorMapped;
	}

	HWR_ProcessPolygon(&Surf, wallVerts, 4, blend|PF_Modulated, shader, false);
}




gl_vissprite_t* gl_vsprorder[MAXVISSPRITES];
static gl_vissprite_t* gl_vsprscratch[MAXVISSPRITES];



static int CompareVisSpritePair(gl_vissprite_t *spr1, gl_vissprite_t *spr2)
{
	int idiff;
	float fdiff;
	float tz1, tz2;







	int transparency1;
	int transparency2;

	int linkdraw1;
	int linkdraw2;


	if (spr1->bbox || spr2->bbox)
		return (spr1->bbox - spr2->bbox);


	linkdraw1 = !spr1->precip && (spr1->mobj->flags2 & MF2_LINKDRAW) && spr1->mobj->tracer;
	linkdraw2 = !spr2->precip && (spr2->mobj->flags2 & MF2_LINKDRAW) && spr2->mobj->tracer;




	if ((linkdraw1 && linkdraw2 && spr1->mobj->tracer != spr2->mobj->tracer) || (linkdraw1 ^ linkdraw2))
	{
		if (linkdraw1)
		{
			tz1 = spr1->tracertz;
			transparency1 = spr1->sorttracerblended;
		}
		else
		{
			tz1 = spr1->tz;
			transparency1 = spr1->sortblended;
		}
		if (linkdraw2)
		{
			tz2 = spr2->tracertz;
			transparency2 = spr2->sorttracerblended;
		}
		else
		{
			tz2 = spr2->tz;
			transparency2 = spr2->sortblended;
		}
	}
	else
	{
		tz1 = spr1->tz;
		transparency1 = spr1->sortblended;
		tz2 = spr2->tz;
		transparency2 = spr2->sortblended;
	}



	idiff = transparency1 - transparency2;
	if (idiff != 0) return idiff;

	fdiff = tz2 - tz1;  
	if (fabsf(fdiff) <= 1.0E-4f)
	{
		idiff = spr1->dispoffset - spr2->dispoffset;  
		if (idiff != 0)
			return idiff;

		                                                                     
                                                                         
                                                                    
                        
		if (!spr1->precip && !spr2->precip && spr1->mobj->mobjnum != spr2->mobj->mobjnum)
			return spr1->mobj->mobjnum < spr2->mobj->mobjnum ? -1 : 1;

		return 0;
	}
	else if (fdiff > 0)
		return 1;
	else
		return -1;
}

static void HWR_SortVisSprites(void)
{
	UINT32 i, width;
	gl_vissprite_t **source = gl_vsprorder;
	gl_vissprite_t **destination = gl_vsprscratch;

	for (i = 0; i < gl_visspritecount; i++)
	{
		gl_vsprorder[i] = HWR_GetVisSprite(i);
		gl_vsprorder[i]->sortblended = !gl_vsprorder[i]->precip
			&& HWR_SpriteUsesBlending(gl_vsprorder[i]->mobj);
		gl_vsprorder[i]->sorttracerblended = false;
		if (!gl_vsprorder[i]->precip && gl_vsprorder[i]->mobj
		&& (gl_vsprorder[i]->mobj->flags2 & MF2_LINKDRAW)
		&& gl_vsprorder[i]->mobj->tracer)
			gl_vsprorder[i]->sorttracerblended =
				HWR_SpriteUsesBlending(gl_vsprorder[i]->mobj->tracer);
	}

	                                                                         
                                                                         
                                                       
	for (width = 1; width < gl_visspritecount; width *= 2)
	{
		UINT32 start;
		for (start = 0; start < gl_visspritecount; start += width * 2)
		{
			UINT32 left = start;
			UINT32 left_end = min(start + width, gl_visspritecount);
			UINT32 right = left_end;
			UINT32 right_end = min(start + width * 2, gl_visspritecount);
			UINT32 out = start;

			while (left < left_end && right < right_end)
			{
				if (CompareVisSpritePair(source[left], source[right]) <= 0)
					destination[out++] = source[left++];
				else
					destination[out++] = source[right++];
			}
			while (left < left_end)
				destination[out++] = source[left++];
			while (right < right_end)
				destination[out++] = source[right++];
		}

		{
			gl_vissprite_t **swap = source;
			source = destination;
			destination = swap;
		}

		 
		if (width > gl_visspritecount / 2)
			break;
	}

	if (source != gl_vsprorder)
		memcpy(gl_vsprorder, source, gl_visspritecount * sizeof(*gl_vsprorder));
}



typedef struct
{
	FOutVector    wallVerts[4];
	FSurfaceInfo  Surf;
	INT32         texnum;
	FBITFIELD     blend;
	INT32         drawcount;
	boolean fogwall;
	INT32 lightlevel;
	extracolormap_t *wallcolormap;  
} wallinfo_t;

static wallinfo_t *wallinfo = NULL;
static size_t numwalls = 0;  

void HWR_RenderWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap);

#define MAX_TRANSPARENTWALL 256

typedef struct
{
	extrasubsector_t *xsub;
	boolean isceiling;
	fixed_t fixedheight;
	INT32 lightlevel;
	levelflat_t *levelflat;
	INT32 alpha;
	sector_t *FOFSector;
	FBITFIELD blend;
	boolean fogplane;
	boolean chromakeyed;
	extracolormap_t *planecolormap;
	INT32 drawcount;
} planeinfo_t;

static size_t numplanes = 0;  
static planeinfo_t *planeinfo = NULL;

typedef struct
{
	polyobj_t *polysector;
	boolean isceiling;
	fixed_t fixedheight;
	INT32 lightlevel;
	levelflat_t *levelflat;
	INT32 alpha;
	sector_t *FOFSector;
	FBITFIELD blend;
	extracolormap_t *planecolormap;
	INT32 drawcount;
} polyplaneinfo_t;

static size_t numpolyplanes = 0;  
static polyplaneinfo_t *polyplaneinfo = NULL;
static INT32 drawcount = 0;

                                                                          
                                                                            
                                                                              
                                                                             
                                                                              
                                                                                 
static wallinfo_t *mainportal_savedwalls;
static planeinfo_t *mainportal_savedplanes;
static polyplaneinfo_t *mainportal_savedpolyplanes;
static size_t mainportal_savednumwalls;
static size_t mainportal_savednumplanes;
static size_t mainportal_savednumpolyplanes;
static size_t mainportal_savedwallcapacity;
static size_t mainportal_savedplanecapacity;
static size_t mainportal_savedpolyplanecapacity;
static INT32 mainportal_saveddrawcount;

static void HWR_SaveMainPortalDrawNodes(void)
{
	if (!mainportal_savedwalls)
		mainportal_savedwallcapacity = 0;
	if (!mainportal_savedplanes)
		mainportal_savedplanecapacity = 0;
	if (!mainportal_savedpolyplanes)
		mainportal_savedpolyplanecapacity = 0;

	if (mainportal_savedwallcapacity < numwalls)
	{
		mainportal_savedwallcapacity = numwalls;
		Z_Realloc(mainportal_savedwalls,
			mainportal_savedwallcapacity * sizeof (*mainportal_savedwalls),
			PU_LEVEL, &mainportal_savedwalls);
	}
	if (mainportal_savedplanecapacity < numplanes)
	{
		mainportal_savedplanecapacity = numplanes;
		Z_Realloc(mainportal_savedplanes,
			mainportal_savedplanecapacity * sizeof (*mainportal_savedplanes),
			PU_LEVEL, &mainportal_savedplanes);
	}
	if (mainportal_savedpolyplanecapacity < numpolyplanes)
	{
		mainportal_savedpolyplanecapacity = numpolyplanes;
		Z_Realloc(mainportal_savedpolyplanes,
			mainportal_savedpolyplanecapacity * sizeof (*mainportal_savedpolyplanes),
			PU_LEVEL, &mainportal_savedpolyplanes);
	}

	mainportal_savednumwalls = numwalls;
	mainportal_savednumplanes = numplanes;
	mainportal_savednumpolyplanes = numpolyplanes;
	mainportal_saveddrawcount = drawcount;
	if (numwalls)
		M_Memcpy(mainportal_savedwalls, wallinfo, numwalls * sizeof (*wallinfo));
	if (numplanes)
		M_Memcpy(mainportal_savedplanes, planeinfo, numplanes * sizeof (*planeinfo));
	if (numpolyplanes)
		M_Memcpy(mainportal_savedpolyplanes, polyplaneinfo,
			numpolyplanes * sizeof (*polyplaneinfo));

	numwalls = numplanes = numpolyplanes = 0;
}

static void HWR_RestoreMainPortalDrawNodes(void)
{
	if (mainportal_savednumwalls)
		M_Memcpy(wallinfo, mainportal_savedwalls,
			mainportal_savednumwalls * sizeof (*wallinfo));
	if (mainportal_savednumplanes)
		M_Memcpy(planeinfo, mainportal_savedplanes,
			mainportal_savednumplanes * sizeof (*planeinfo));
	if (mainportal_savednumpolyplanes)
		M_Memcpy(polyplaneinfo, mainportal_savedpolyplanes,
			mainportal_savednumpolyplanes * sizeof (*polyplaneinfo));
	numwalls = mainportal_savednumwalls;
	numplanes = mainportal_savednumplanes;
	numpolyplanes = mainportal_savednumpolyplanes;
	drawcount = mainportal_saveddrawcount;
	mainportal_savednumwalls = 0;
	mainportal_savednumplanes = 0;
	mainportal_savednumpolyplanes = 0;
}


typedef struct gl_drawnode_s
{
	planeinfo_t *plane;
	polyplaneinfo_t *polyplane;
	wallinfo_t *wall;
	gl_vissprite_t *sprite;



} gl_drawnode_t;

#define MAX_TRANSPARENTFLOOR 512


void HWR_AddTransparentFloor(levelflat_t *levelflat, extrasubsector_t *xsub, boolean isceiling, fixed_t fixedheight, INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, boolean fogplane, boolean chromakeyed, extracolormap_t *planecolormap)
{
	static size_t allocedplanes = 0;


	if (!planeinfo)
		allocedplanes = 0;

	if (allocedplanes < numplanes + 1)
	{
		allocedplanes += MAX_TRANSPARENTFLOOR;
		Z_Realloc(planeinfo, allocedplanes * sizeof (*planeinfo), PU_LEVEL, &planeinfo);
	}

	planeinfo[numplanes].isceiling = isceiling;
	planeinfo[numplanes].fixedheight = fixedheight;
	planeinfo[numplanes].lightlevel = (planecolormap && (planecolormap->flags & CMF_FOG)) ? lightlevel : 255;  
	planeinfo[numplanes].levelflat = levelflat;
	planeinfo[numplanes].xsub = xsub;
	planeinfo[numplanes].alpha = alpha;
	planeinfo[numplanes].FOFSector = FOFSector;
	planeinfo[numplanes].blend = blend;
	planeinfo[numplanes].fogplane = fogplane;
	planeinfo[numplanes].chromakeyed = chromakeyed;
	planeinfo[numplanes].planecolormap = planecolormap;
	planeinfo[numplanes].drawcount = drawcount++;

	numplanes++;
}



void HWR_AddTransparentPolyobjectFloor(levelflat_t *levelflat, polyobj_t *polysector, boolean isceiling, fixed_t fixedheight, INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, extracolormap_t *planecolormap)
{
	static size_t allocedpolyplanes = 0;


	if (!polyplaneinfo)
		allocedpolyplanes = 0;

	if (allocedpolyplanes < numpolyplanes + 1)
	{
		allocedpolyplanes += MAX_TRANSPARENTFLOOR;
		Z_Realloc(polyplaneinfo, allocedpolyplanes * sizeof (*polyplaneinfo), PU_LEVEL, &polyplaneinfo);
	}

	polyplaneinfo[numpolyplanes].isceiling = isceiling;
	polyplaneinfo[numpolyplanes].fixedheight = fixedheight;
	polyplaneinfo[numpolyplanes].lightlevel = (planecolormap && (planecolormap->flags & CMF_FOG)) ? lightlevel : 255;   
	polyplaneinfo[numpolyplanes].levelflat = levelflat;
	polyplaneinfo[numpolyplanes].polysector = polysector;
	polyplaneinfo[numpolyplanes].alpha = alpha;
	polyplaneinfo[numpolyplanes].FOFSector = FOFSector;
	polyplaneinfo[numpolyplanes].blend = blend;
	polyplaneinfo[numpolyplanes].planecolormap = planecolormap;
	polyplaneinfo[numpolyplanes].drawcount = drawcount++;
	numpolyplanes++;
}


gl_drawnode_t *sortnode;
size_t *sortindex;
static size_t sortnode_capacity;

static int CompareDrawNodePlanes(const void *p1, const void *p2)
{
	size_t n1 = *(const size_t*)p1;
	size_t n2 = *(const size_t*)p2;
	if (!sortnode[n1].plane) I_Error("CompareDrawNodePlanes: Uh.. This isn't a plane! (n1)");
	if (!sortnode[n2].plane) I_Error("CompareDrawNodePlanes: Uh.. This isn't a plane! (n2)");
	return abs(sortnode[n2].plane->fixedheight - viewz) - abs(sortnode[n1].plane->fixedheight - viewz);
}




static void HWR_CreateDrawNodes(void)
{
	UINT32 i = 0, p = 0;
	const size_t nodecount = numplanes + numpolyplanes + numwalls;
	size_t run_start = 0;
	size_t newcapacity;

	                                                                            
                                                                           
                                                                            
                                                                           
	if (!sortnode || !sortindex)
		sortnode_capacity = 0;
	if (sortnode_capacity < nodecount)
	{
		newcapacity = max(nodecount, sortnode_capacity + sortnode_capacity / 2 + 64);
		Z_Realloc(sortnode, newcapacity * sizeof(*sortnode), PU_LEVEL, &sortnode);
		Z_Realloc(sortindex, newcapacity * sizeof(*sortindex), PU_LEVEL, &sortindex);
		sortnode_capacity = newcapacity;
	}
	memset(sortnode, 0, nodecount * sizeof(*sortnode));

	PS_START_TIMING(ps_hw_nodesorttime);

	for (i = 0; i < numplanes; i++, p++)
	{
		sortnode[p].plane = &planeinfo[i];
		sortindex[nodecount - 1 - planeinfo[i].drawcount] = p;
	}

	for (i = 0; i < numpolyplanes; i++, p++)
	{
		sortnode[p].polyplane = &polyplaneinfo[i];
		sortindex[nodecount - 1 - polyplaneinfo[i].drawcount] = p;
	}

	for (i = 0; i < numwalls; i++, p++)
	{
		sortnode[p].wall = &wallinfo[i];
		sortindex[nodecount - 1 - wallinfo[i].drawcount] = p;
	}

	ps_numdrawnodes.value.i = p;



	                                                                          
                                                                           
                                                                              



	while (run_start < p-1) 
	{

		if (sortnode[sortindex[run_start]].plane)
		{

			size_t run_end; 
			for (i = run_start+1; i < p; i++) 
			{
				if (!sortnode[sortindex[i]].plane) break;
			}
			run_end = i-1;
			if (run_end > run_start) 
			{
				const size_t run_length = run_end - run_start + 1;
				                                                              
                                                                      
                                                                  
                                            
				if (run_length <= 32)
				{
					size_t current;
					for (current = run_start + 1; current <= run_end; current++)
					{
						const size_t value = sortindex[current];
						size_t insert = current;
						while (insert > run_start
						&& CompareDrawNodePlanes(&sortindex[insert - 1], &value) > 0)
						{
							sortindex[insert] = sortindex[insert - 1];
							insert--;
						}
						sortindex[insert] = value;
					}
				}
				else
					qsort(sortindex + run_start, run_length, sizeof(size_t), CompareDrawNodePlanes);
			}
			run_start = run_end + 1; 
		}
		else
		{

			run_start++;
		}
	}

	PS_STOP_TIMING(ps_hw_nodesorttime);

	PS_START_TIMING(ps_hw_nodedrawtime);


	HWD.pfnSetTransform(&atransform);

	for (i = 0; i < p; i++)
	{
		if (sortnode[sortindex[i]].plane)
		{

			gl_frontsector = NULL;

			if (!(sortnode[sortindex[i]].plane->blend & PF_NoTexture))
				HWR_GetLevelFlat(sortnode[sortindex[i]].plane->levelflat, sortnode[sortindex[i]].plane->chromakeyed);
			HWR_RenderPlane(NULL, sortnode[sortindex[i]].plane->xsub, sortnode[sortindex[i]].plane->isceiling, sortnode[sortindex[i]].plane->fixedheight, sortnode[sortindex[i]].plane->blend, sortnode[sortindex[i]].plane->lightlevel,
				sortnode[sortindex[i]].plane->levelflat, sortnode[sortindex[i]].plane->FOFSector, sortnode[sortindex[i]].plane->alpha, sortnode[sortindex[i]].plane->planecolormap);
		}
		else if (sortnode[sortindex[i]].polyplane)
		{

			gl_frontsector = NULL;

			polyobj_t *po = sortnode[sortindex[i]].polyplane->polysector;

			if (!(sortnode[sortindex[i]].polyplane->blend & PF_NoTexture))
				HWR_GetLevelFlat(sortnode[sortindex[i]].polyplane->levelflat, po->flags & POF_SPLAT);
			HWR_RenderPolyObjectPlane(po, sortnode[sortindex[i]].polyplane->isceiling, sortnode[sortindex[i]].polyplane->fixedheight, sortnode[sortindex[i]].polyplane->blend, sortnode[sortindex[i]].polyplane->lightlevel,
				sortnode[sortindex[i]].polyplane->levelflat, sortnode[sortindex[i]].polyplane->FOFSector, sortnode[sortindex[i]].polyplane->alpha, sortnode[sortindex[i]].polyplane->planecolormap);
		}
		else if (sortnode[sortindex[i]].wall)
		{
			if (!(sortnode[sortindex[i]].wall->blend & PF_NoTexture))
				HWR_GetTexture(sortnode[sortindex[i]].wall->texnum, true);
			HWR_RenderWall(sortnode[sortindex[i]].wall->wallVerts, &sortnode[sortindex[i]].wall->Surf, sortnode[sortindex[i]].wall->blend, sortnode[sortindex[i]].wall->fogwall,
				sortnode[sortindex[i]].wall->lightlevel, sortnode[sortindex[i]].wall->wallcolormap);
		}
	}

	PS_STOP_TIMING(ps_hw_nodedrawtime);

	numwalls = 0;
	numplanes = 0;
	numpolyplanes = 0;

}





static boolean hwr_depth_only_sprites = false;

static boolean HWR_SpriteHasModel(const gl_vissprite_t *spr)
{
	if (!cv_glmodels.value || !spr->mobj)
		return false;

	if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
	{
		const md2_t *model = &md2_playermodels[((skin_t *)spr->mobj->skin)->skinnum];
		return model->found && model->scale >= 0.0f;
	}

	return md2_models[spr->mobj->sprite].found
		&& md2_models[spr->mobj->sprite].scale >= 0.0f;
}

static boolean HWR_SpriteCanBatch(const gl_vissprite_t *spr)
{
	if (!spr || spr->bbox || spr->precip || !spr->mobj || hwr_depth_only_sprites)
		return false;

	                                                                         
                                                                         
                                      
	if (HWR_SpriteHasModel(spr)
	|| spr->sortblended
	|| (spr->mobj->flags2 & MF2_LINKDRAW)
	|| (spr->renderflags & RF_SHADOWEFFECTS)
	|| (spr->mobj->shadowscale && cv_shadow.value))
		return false;

	return true;
}


static void HWR_DrawSprites(void)
{
	UINT32 i;
	UINT32 batchable = 0;
	boolean batched = false;
	boolean skipshadow = false;  
	HWD.pfnSetSpecialState(HWD_SET_MODEL_LIGHTING, cv_glmodellighting.value);

	                                                                          
                                                                             
                                                                     
                                                                       
	if (cv_glbatching.value && !hwr_depth_only_sprites)
	{
		for (i = 0; i < gl_visspritecount; i++)
			if (HWR_SpriteCanBatch(gl_vsprorder[i]))
				batchable++;

		if (batchable >= 16)
		{
			HWR_StartBatching();
			for (i = 0; i < gl_visspritecount; i++)
				if (HWR_SpriteCanBatch(gl_vsprorder[i]))
					HWR_DrawSprite(gl_vsprorder[i]);
			HWR_RenderBatches();
			ps_hw_batchedsprites.value.i += batchable;
			batched = true;
		}
	}

	for (i = 0; i < gl_visspritecount; i++)
	{
		gl_vissprite_t *spr = gl_vsprorder[i];

		if (batched && HWR_SpriteCanBatch(spr))
			continue;

		                                                                    
                                                                     
                                                                   
                                                                        
		if (hwr_depth_only_sprites)
		{
			if (!spr->bbox && !spr->precip)
				HWR_DrawSprite(spr);
			continue;
		}

		if (spr->bbox)
			HWR_DrawBoundingBox(spr);
		else if (spr->precip)
			HWR_DrawPrecipitationSprite(spr);
		else
		{
			if (spr->mobj && spr->mobj->shadowscale && cv_shadow.value && !skipshadow)
			{
				HWR_DrawDropShadow(spr->mobj, spr->mobj->shadowscale);
			}

			if ((spr->mobj->flags2 & MF2_LINKDRAW) && spr->mobj->tracer)
			{





				if (cv_shadow.value && !skipshadow && spr->dispoffset < 0 && spr->mobj->tracer->shadowscale)
				{
					HWR_DrawDropShadow(spr->mobj->tracer, spr->mobj->tracer->shadowscale);
					skipshadow = true;



				}
			}
			else
			{
				skipshadow = false;
			}

			if (spr->mobj && spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
			{
				if (!cv_glmodels.value || !md2_playermodels[((skin_t*)spr->mobj->skin)->skinnum].found || md2_playermodels[((skin_t*)spr->mobj->skin)->skinnum].scale < 0.0f)
					HWR_DrawSprite(spr);
				else
				{
					if (!HWR_DrawModel(spr))
						HWR_DrawSprite(spr);
				}
			}
			else
			{
				if (!cv_glmodels.value || !md2_models[spr->mobj->sprite].found || md2_models[spr->mobj->sprite].scale < 0.0f)
					HWR_DrawSprite(spr);
				else
				{
					if (!HWR_DrawModel(spr))
						HWR_DrawSprite(spr);
				}
			}
		}
	}
	HWD.pfnSetSpecialState(HWD_SET_MODEL_LIGHTING, 0);



	HWR_LinkDrawHackFinish();





	HWD.pfnSetBlend(PF_Translucent|PF_Occlude|PF_Masked);
}





static UINT8 sectorlight;
static void HWR_AddSprites(sector_t *sec)
{
	mobj_t *thing;
	precipmobj_t *precipthing;
	fixed_t limit_dist, hoop_limit_dist;
	const boolean renderhitboxes = (cv_renderhitbox.value != 0);





	if (sec->validcount == validcount)
		return;


	sec->validcount = validcount;


	sectorlight = sec->lightlevel & 0xff;



	limit_dist = (fixed_t)(cv_drawdist.value) << FRACBITS;
	hoop_limit_dist = (fixed_t)(cv_drawdist_nights.value) << FRACBITS;
	for (thing = sec->thinglist; thing; thing = thing->snext)
	{
		const boolean visible = R_ThingVisible(thing);

		if (!visible && !renderhitboxes)
			continue;

		if (R_ThingWithinDist(thing, limit_dist, hoop_limit_dist))
		{
			if (visible && (renderhitboxes || !R_ShouldCullAdaptiveEffect(thing)))
			{
				HWR_ProjectSprite(thing);
			}

			if (renderhitboxes)
				HWR_ProjectBoundingBox(thing);
		}
	}


	if ((limit_dist = (fixed_t)cv_drawdist_precip.value << FRACBITS))
	{
		for (precipthing = sec->preciplist; precipthing; precipthing = precipthing->snext)
		{
			if (R_PrecipThingVisible(precipthing, limit_dist)
			&& !R_ShouldCullAdaptivePrecipitation(precipthing))
				HWR_ProjectPrecipitationSprite(precipthing);
		}
	}
}






static void HWR_ProjectSprite(mobj_t *thing)
{
	gl_vissprite_t *vis;
	float tr_x, tr_y;
	float tz;
	float tracertz = 0.0f;
	float x1, x2;
	float rightsin, rightcos;
	float this_scale, this_xscale, this_yscale;
	float spritexscale, spriteyscale;
	float shadowheight = 1.0f, shadowscale = 1.0f;
	float gz, gzt;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
#ifdef ROTSPRITE
	spriteinfo_t *sprinfo;
#endif
	md2_t *md2;
	size_t lumpoff;
	unsigned rot;
	UINT16 flip;
	boolean vflip = (!(thing->eflags & MFE_VERTICALFLIP) != !R_ThingVerticallyFlipped(thing));
	boolean mirrored = thing->mirrored;
	boolean hflip = (!R_ThingHorizontallyFlipped(thing) != !mirrored);
	skincolornum_t color;
	UINT16 translation;
	INT32 dispoffset;

	angle_t ang;
	INT32 heightsec, phs;
	const boolean splat = R_ThingIsFloorSprite(thing);
	const boolean papersprite = (R_ThingIsPaperSprite(thing) && !splat);
	float z1, z2;

	fixed_t spr_width, spr_height;
	fixed_t spr_offset, spr_topoffset;
#ifdef ROTSPRITE
	patch_t *rotsprite = NULL;
	INT32 rollangle = 0;
	angle_t spriterotangle = 0;
#endif


	interpmobjstate_t interp = {0};

	if (!r_renderthings)
		return;

	if (!thing)
		return;

	if (HWR_PointHiddenByLinePortal(thing->x, thing->y))
		return;

	INT32 blendmode;
	if (thing->frame & FF_BLENDMASK)
		blendmode = ((thing->frame & FF_BLENDMASK) >> FF_BLENDSHIFT) + 1;
	else
		blendmode = thing->blendmode;


	if (thing->frame & FF_TRANSMASK)
	{
		if (!R_BlendLevelVisible(blendmode, (thing->frame & FF_TRANSMASK)>>FF_TRANSSHIFT))
			return;
	}

	dispoffset = thing->dispoffset;


	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(thing, FRACUNIT, &interp);
	}

	if (interp.spritexscale < 1 || interp.spriteyscale < 1)
		return;

	this_scale = FIXED_TO_FLOAT(interp.scale);
	spritexscale = FIXED_TO_FLOAT(interp.spritexscale);
	spriteyscale = FIXED_TO_FLOAT(interp.spriteyscale);


	if (thing->type == MT_OVERLAY)  
		R_ThingOffsetOverlay(thing, &interp.x, &interp.y);
	tr_x = FIXED_TO_FLOAT(interp.x) - gl_viewx;
	tr_y = FIXED_TO_FLOAT(interp.y) - gl_viewy;


	tz = (tr_x * gl_viewcos) + (tr_y * gl_viewsin);


	if (tz < ZCLIP_PLANE && !(papersprite || splat))
	{
		if (cv_glmodels.value)  
		{
			if (thing->skin && thing->sprite == SPR_PLAY)
				md2 = &md2_playermodels[((skin_t *)thing->skin)->skinnum];
			else
				md2 = &md2_models[thing->sprite];

			if (!md2->found || md2->scale < 0.0f)
				return;
		}
		else
			return;
	}


	tr_x = FIXED_TO_FLOAT(interp.x);
	tr_y = FIXED_TO_FLOAT(interp.y);


#ifdef RANGECHECK
	if ((unsigned)thing->sprite >= numsprites)
		I_Error("HWR_ProjectSprite: invalid sprite number %i ", thing->sprite);
#endif

	rot = thing->frame&FF_FRAMEMASK;


	if (thing->skin && thing->sprite == SPR_PLAY)
	{
		sprdef = P_GetSkinSpritedef(thing->skin, thing->sprite2);
#ifdef ROTSPRITE
		sprinfo = P_GetSkinSpriteInfo(thing->skin, thing->sprite2);
#endif
	}
	else
	{
		sprdef = &sprites[thing->sprite];
#ifdef ROTSPRITE
		sprinfo = &spriteinfo[thing->sprite];
#endif
	}

	if (rot >= sprdef->numframes)
	{
		CONS_Alert(CONS_ERROR, M_GetText("HWR_ProjectSprite: invalid sprite frame %s/%s for %s\n"),
			sizeu1(rot), sizeu2(sprdef->numframes), sprnames[thing->sprite]);
		thing->sprite = states[S_UNKNOWN].sprite;
		thing->frame = states[S_UNKNOWN].frame;
		sprdef = &sprites[thing->sprite];
#ifdef ROTSPRITE
		sprinfo = &spriteinfo[thing->sprite];
#endif
		rot = thing->frame&FF_FRAMEMASK;
		thing->state->sprite = thing->sprite;
		thing->state->frame = thing->frame;
	}

	sprframe = &sprdef->spriteframes[rot];

#ifdef PARANOIA
	if (!sprframe)
		I_Error("sprframes NULL for sprite %d\n", thing->sprite);
#endif

	if (splat)
	{
		ang = R_PointToAngle2(0, viewz, 0, interp.z);
	}
	else
	{
		ang = R_PointToAngle (interp.x, interp.y) - interp.angle;
		if (mirrored)
			ang = InvAngle(ang);
	}

	if (sprframe->rotate == SRF_SINGLE)
	{

		rot = 0;                         
		lumpoff = sprframe->lumpid[0];      
		flip = sprframe->flip;  

		if (papersprite && ang < ANGLE_180)
			flip ^= 0xFFFF;
	}
	else
	{

		if ((sprframe->rotate & SRF_RIGHT) && (ang < ANGLE_180))  
			rot = 6;  
		else if ((sprframe->rotate & SRF_LEFT) && (ang >= ANGLE_180))  
			rot = 2;  
		else if (sprframe->rotate & SRF_3DGE)  
		{
			rot = (ang+ANGLE_180+ANGLE_11hh)>>28;
			rot = ((rot & 1)<<3)|(rot>>1);
		}
		else  
			rot = (ang+ANGLE_202h)>>29;


		lumpoff = sprframe->lumpid[rot];
		flip = sprframe->flip & (1<<rot);

		if (papersprite && ang < ANGLE_180)
			flip ^= (1<<rot);
	}

	if (thing->skin && ((skin_t *)thing->skin)->flags & SF_HIRES)
		this_scale *= FIXED_TO_FLOAT(((skin_t *)thing->skin)->highresscale);

	spr_width = spritecachedinfo[lumpoff].width;
	spr_height = spritecachedinfo[lumpoff].height;
	spr_offset = spritecachedinfo[lumpoff].offset;
	spr_topoffset = spritecachedinfo[lumpoff].topoffset;

#ifdef ROTSPRITE
	spriterotangle = R_SpriteRotationAngle(&interp);

	if (spriterotangle != 0
	&& !(splat && !(thing->renderflags & RF_NOSPLATROLLANGLE)))
	{
		if (papersprite)
		{

			rollangle = R_GetRollAngle(InvAngle(spriterotangle));
		}
		else
		{
			rollangle = R_GetRollAngle(spriterotangle);
		}

		rotsprite = Patch_GetRotatedSprite(sprframe, (thing->frame & FF_FRAMEMASK), rot, flip, sprinfo, rollangle);

		if (rotsprite != NULL)
		{
			spr_width = rotsprite->width << FRACBITS;
			spr_height = rotsprite->height << FRACBITS;
			spr_offset = rotsprite->leftoffset << FRACBITS;
			spr_topoffset = rotsprite->topoffset << FRACBITS;
			spr_topoffset += FEETADJUST;


			flip = 0;
		}
	}
#endif

	if (thing->renderflags & RF_ABSOLUTEOFFSETS)
	{
		spr_offset = interp.spritexoffset;
		spr_topoffset = interp.spriteyoffset;
	}
	else
	{
		SINT8 flipoffset = 1;

		if ((thing->renderflags & RF_FLIPOFFSETS) && flip)
			flipoffset = -1;

		spr_offset += interp.spritexoffset * flipoffset;
		spr_topoffset += interp.spriteyoffset * flipoffset;
	}

	if (papersprite)
	{
		rightsin = FIXED_TO_FLOAT(FINESINE(interp.angle >> ANGLETOFINESHIFT));
		rightcos = FIXED_TO_FLOAT(FINECOSINE(interp.angle >> ANGLETOFINESHIFT));
	}
	else
	{
		rightsin = FIXED_TO_FLOAT(FINESINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
		rightcos = FIXED_TO_FLOAT(FINECOSINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	}

	flip = !flip != !hflip;

	if (thing->renderflags & RF_SHADOWEFFECTS)
	{
		mobj_t *caster = thing->target;

		if (caster && !P_MobjWasRemoved(caster))
		{
			interpmobjstate_t casterinterp = { 0 };
			fixed_t groundz;
			fixed_t floordiff;

			if (R_UsingFrameInterpolation() && !paused)
			{
				R_InterpolateMobjState(caster, rendertimefrac, &casterinterp);
			}
			else
			{
				R_InterpolateMobjState(caster, FRACUNIT, &casterinterp);
			}

			groundz = R_GetShadowZ(thing, NULL);
			floordiff = abs(((thing->eflags & MFE_VERTICALFLIP) ? casterinterp.height : 0) + casterinterp.z - groundz);

			shadowheight = FIXED_TO_FLOAT(floordiff);
			shadowscale = FIXED_TO_FLOAT(FixedMul(FRACUNIT - floordiff/640, casterinterp.scale));

			if (splat)
				spritexscale *= shadowscale;
			spriteyscale *= shadowscale;
		}
	}

	this_xscale = spritexscale * this_scale;
	this_yscale = spriteyscale * this_scale;

	if (splat)
	{
		z1 = z2 = tr_y;
		x1 = x2 = tr_x;
		gz = gzt = interp.z;
	}
	else
	{
		if (flip)
		{
			x1 = (FIXED_TO_FLOAT(spr_width - spr_offset) * this_xscale);
			x2 = (FIXED_TO_FLOAT(spr_offset) * this_xscale);
		}
		else
		{
			x1 = (FIXED_TO_FLOAT(spr_offset) * this_xscale);
			x2 = (FIXED_TO_FLOAT(spr_width - spr_offset) * this_xscale);
		}


	  
                  
   
                               
                               

                                 
           
   
   

		z1 = tr_y + x1 * rightsin;
		z2 = tr_y - x2 * rightsin;
		x1 = tr_x + x1 * rightcos;
		x2 = tr_x - x2 * rightcos;

		if (vflip)
		{
			gz = FIXED_TO_FLOAT(interp.z + interp.height) - (FIXED_TO_FLOAT(spr_topoffset) * this_yscale);
			gzt = gz + (FIXED_TO_FLOAT(spr_height) * this_yscale);
		}
		else
		{
			gzt = FIXED_TO_FLOAT(interp.z) + (FIXED_TO_FLOAT(spr_topoffset) * this_yscale);
			gz = gzt - (FIXED_TO_FLOAT(spr_height) * this_yscale);
		}
	}

	if (thing->subsector->sector->cullheight)
	{
		if (HWR_DoCulling(thing->subsector->sector->cullheight, viewsector->cullheight, gl_viewz, gz, gzt))
			return;
	}

	heightsec = thing->subsector->sector->heightsec;
	if (viewplayer->mo && viewplayer->mo->subsector)
		phs = viewplayer->mo->subsector->sector->heightsec;
	else
		phs = -1;

	if (heightsec != -1 && phs != -1)  
	{
		float top = gzt;
		float bottom = FIXED_TO_FLOAT(interp.z);

		if (R_ThingIsFloorSprite(thing))
			top = bottom;

		if (gl_viewz < FIXED_TO_FLOAT(sectors[phs].floorheight) ?
		bottom >= FIXED_TO_FLOAT(sectors[heightsec].floorheight) :
		top < FIXED_TO_FLOAT(sectors[heightsec].floorheight))
			return;
		if (gl_viewz > FIXED_TO_FLOAT(sectors[phs].ceilingheight) ?
		top < FIXED_TO_FLOAT(sectors[heightsec].ceilingheight) && gl_viewz >= FIXED_TO_FLOAT(sectors[heightsec].ceilingheight) :
		bottom >= FIXED_TO_FLOAT(sectors[heightsec].ceilingheight))
			return;
	}

	if ((thing->flags2 & MF2_LINKDRAW) && thing->tracer)
	{
		interpmobjstate_t tracer_interp = { 0 };

		if (! R_ThingVisible(thing->tracer))
			return;

		if (R_UsingFrameInterpolation() && !paused)
		{
			R_InterpolateMobjState(thing->tracer, rendertimefrac, &tracer_interp);
		}
		else
		{
			R_InterpolateMobjState(thing->tracer, FRACUNIT, &tracer_interp);
		}



		if (thing->tracer->type == MT_OVERLAY)  
			R_ThingOffsetOverlay(thing->tracer, &tracer_interp.x, &tracer_interp.y);
		tr_x = FIXED_TO_FLOAT(tracer_interp.x) - gl_viewx;
		tr_y = FIXED_TO_FLOAT(tracer_interp.y) - gl_viewy;


		tracertz = (tr_x * gl_viewcos) + (tr_y * gl_viewsin);






		if (tracertz < ZCLIP_PLANE)
			return;


		if (tz > tracertz)
			dispoffset *= -1;
	}


	vis = HWR_NewVisSprite();
	vis->x1 = x1;
	vis->x2 = x2;
	vis->z1 = z1;
	vis->z2 = z2;

	vis->tz = tz;  
	vis->tracertz = tracertz;

	vis->renderflags = thing->renderflags;
	vis->rotateflags = sprframe->rotate;

	vis->shadowheight = shadowheight;
	vis->shadowscale = shadowscale;
	vis->dispoffset = dispoffset;  
	vis->flip = flip;

	vis->scale = this_scale;
	vis->spritexscale = spritexscale;
	vis->spriteyscale = spriteyscale;
	vis->spritexoffset = FIXED_TO_FLOAT(spr_offset);
	vis->spriteyoffset = FIXED_TO_FLOAT(spr_topoffset);

	vis->rotated = false;

#ifdef ROTSPRITE
	if (rotsprite)
	{
		vis->gpatch = (patch_t *)rotsprite;
		vis->rotated = true;
	}
	else
#endif
		vis->gpatch = (patch_t *)W_CachePatchNum(sprframe->lumppat[rot], PU_SPRITE);

	vis->mobj = thing;

	if ((thing->flags2 & MF2_LINKDRAW) && thing->tracer && thing->color == SKINCOLOR_NONE)
		color = thing->tracer->color;
	else
		color = thing->color;

	if ((thing->flags2 & MF2_LINKDRAW) && thing->tracer && thing->translation == 0)
		translation = thing->tracer->translation;
	else
		translation = thing->translation;


	if ((thing->flags2 & MF2_LINKDRAW) && thing->tracer)
		vis->colormap = R_GetTranslationForThing(thing->tracer, color, translation);
	else
		vis->colormap = R_GetTranslationForThing(thing, color, translation);


	vis->gzt = gzt;
	vis->gz = gz;




	vis->vflip = vflip;

	vis->precip = false;
	vis->bbox = false;

	vis->angle = interp.angle;
}


static void HWR_ProjectPrecipitationSprite(precipmobj_t *thing)
{
	gl_vissprite_t *vis;
	float tr_x, tr_y;
	float tz;
	float x1, x2;
	float z1, z2;
	float rightsin, rightcos;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	size_t lumpoff;
	unsigned rot = 0;
	UINT8 flip;

	if (!thing)
		return;


	if (thing->frame & FF_TRANSMASK)
	{
		if (!R_BlendLevelVisible(thing->blendmode, (thing->frame & FF_TRANSMASK)>>FF_TRANSSHIFT))
			return;
	}


	interpmobjstate_t interp = {0};



	if (!paused && thing->lastupdatetime < gametic)
		R_ResetPrecipitationMobjInterpolationState(thing);


	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolatePrecipMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolatePrecipMobjState(thing, FRACUNIT, &interp);
	}


	tr_x = FIXED_TO_FLOAT(interp.x) - gl_viewx;
	tr_y = FIXED_TO_FLOAT(interp.y) - gl_viewy;


	tz = (tr_x * gl_viewcos) + (tr_y * gl_viewsin);


	if (tz < ZCLIP_PLANE)
		return;

	tr_x = FIXED_TO_FLOAT(interp.x);
	tr_y = FIXED_TO_FLOAT(interp.y);


	if ((unsigned)thing->sprite >= numsprites)
#ifdef RANGECHECK
		I_Error("HWR_ProjectPrecipitationSprite: invalid sprite number %i ",
		        thing->sprite);
#else
		return;
#endif

	sprdef = &sprites[thing->sprite];

	if ((size_t)(thing->frame&FF_FRAMEMASK) >= sprdef->numframes)
#ifdef RANGECHECK
		I_Error("HWR_ProjectPrecipitationSprite: invalid sprite frame %i : %i for %s",
		        thing->sprite, thing->frame, sprnames[thing->sprite]);
#else
		return;
#endif

	sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];


	lumpoff = sprframe->lumpid[0];
	flip = sprframe->flip;  

	rightsin = FIXED_TO_FLOAT(FINESINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	rightcos = FIXED_TO_FLOAT(FINECOSINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	if (flip)
	{
		x1 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].width - spritecachedinfo[lumpoff].offset);
		x2 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].offset);
	}
	else
	{
		x1 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].offset);
		x2 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].width - spritecachedinfo[lumpoff].offset);
	}

	z1 = tr_y + x1 * rightsin;
	z2 = tr_y - x2 * rightsin;
	x1 = tr_x + x1 * rightcos;
	x2 = tr_x - x2 * rightcos;




	vis = HWR_NewVisSprite();
	vis->x1 = x1;
	vis->x2 = x2;
	vis->z1 = z1;
	vis->z2 = z2;
	vis->tz = tz;
	vis->dispoffset = 0;  
	vis->gpatch = (patch_t *)W_CachePatchNum(sprframe->lumppat[rot], PU_SPRITE);
	vis->flip = flip;
	vis->mobj = (mobj_t *)thing;

	vis->colormap = NULL;


	vis->gzt = FIXED_TO_FLOAT(interp.z + spritecachedinfo[lumpoff].topoffset);
	vis->gz = vis->gzt - FIXED_TO_FLOAT(spritecachedinfo[lumpoff].height);

	vis->precip = true;
	vis->bbox = false;


	if (!(paused || P_AutoPause()) && thing->lastupdatetime < gametic)
	{
		if (thing->precipflags & PCF_RAIN)
			P_RainThinker(thing);
		else
			P_SnowThinker(thing);

		thing->lastupdatetime = gametic;
	}
}

static void HWR_ProjectBoundingBox(mobj_t *thing)
{
	gl_vissprite_t *vis;
	float tr_x, tr_y;
	float tz;

	if (!thing)
		return;

	if (!R_ThingBoundingBoxVisible(thing))
		return;


	boolean interpolate = cv_renderhitboxinterpolation.value;
	interpmobjstate_t interp = {0};

	if (R_UsingFrameInterpolation() && !paused && interpolate)
	{
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(thing, FRACUNIT, &interp);
	}


	tr_x = FIXED_TO_FLOAT(interp.x) - gl_viewx;
	tr_y = FIXED_TO_FLOAT(interp.y) - gl_viewy;


	tz = (tr_x * gl_viewcos) + (tr_y * gl_viewsin);


	if (tz < ZCLIP_PLANE)
		return;

	tr_x += gl_viewx;
	tr_y += gl_viewy;

	vis = HWR_NewVisSprite();
	vis->x1 = tr_x - FIXED_TO_FLOAT(interp.radius);
	vis->x2 = tr_x + FIXED_TO_FLOAT(interp.radius);
	vis->z1 = tr_y - FIXED_TO_FLOAT(interp.radius);
	vis->z2 = tr_y + FIXED_TO_FLOAT(interp.radius);
	vis->gz = FIXED_TO_FLOAT(interp.z);
	vis->gzt = vis->gz + FIXED_TO_FLOAT(interp.height);
	vis->mobj = thing;

	vis->precip = false;
	vis->bbox = true;
}





static gl_sky_t gl_sky;

static void HWR_SkyDomeVertex(gl_sky_t *sky, gl_skyvertex_t *vbo, int r, int c, signed char yflip, float delta, boolean foglayer)
{
	const float radians = (float)(M_PIl / 180.0f);
	const float scale = 10000.0f;
	                                                                           
                                                                              
	const float maxSideAngle = 60.0f;

	float topAngle = (c / (float)sky->columns * 360.0f);
	float sideAngle = (maxSideAngle * (sky->rows - r) / sky->rows);
	float height = (float)(sin(sideAngle * radians));
	float realRadius = (float)(scale * cos(sideAngle * radians));
	float x = (float)(realRadius * cos(topAngle * radians));
	float y = (!yflip) ? scale * height : -scale * height;
	float z = (float)(realRadius * sin(topAngle * radians));
	float timesRepeat = (4 * (256.0f / sky->width));
	float texelOffset = (BASEVIDWIDTH / 2.0f)
		* (float)tan(sideAngle * radians);
	if (fpclassify(timesRepeat) == FP_ZERO)
		timesRepeat = 1.0f;

	if (!foglayer)
	{
		vbo->r = 255;
		vbo->g = 255;
		vbo->b = 255;
		vbo->a = (r == 0 ? 0 : 255);


		vbo->u = (-timesRepeat * c / (float)sky->columns);
		                                                                          
                                                                           
                                                                              
		vbo->v = 0.5f + (yflip ? texelOffset : -texelOffset)
			/ max(1.0f, (float)sky->height);
	}
	else
	{
		                                                                        
                                                                     
		vbo->u = 0.0f;
		vbo->v = yflip ? 1.0f : 0.0f;
	}


	vbo->x = x;
	vbo->y = y + delta;
	vbo->z = z;
}


void HWR_ClearSkyDome(void)
{
	gl_sky_t *sky = &gl_sky;

	if (sky->loops)
		free(sky->loops);
	if (sky->data)
		free(sky->data);

	sky->loops = NULL;
	sky->data = NULL;

	sky->vbo = 0;
	sky->rows = sky->columns = 0;
	sky->loopcount = 0;

	sky->detail = 0;
	sky->texture = -1;
	sky->width = sky->height = 0;
	sky->rebuild = true;
}

void HWR_BuildSkyDome(void)
{
	int c, r;
	signed char yflip;
	                                                                          
                                                                          
                                                           
	int row_count = 8;
	int col_count = 64;
	float delta;

	gl_sky_t *sky = &gl_sky;
	gl_skyvertex_t *vertex_p;
	texture_t *texture = textures[texturetranslation[skytexture]];

	sky->detail = 16;
	col_count *= sky->detail;

	if ((sky->columns != col_count) || (sky->rows != row_count))
		HWR_ClearSkyDome();

	sky->columns = col_count;
	sky->rows = row_count;
	sky->vertex_count = 2 * sky->rows * (sky->columns * 2 + 2) + sky->columns * 2;

	if (!sky->loops)
		sky->loops = malloc((sky->rows * 2 + 2) * sizeof(sky->loops[0]));


	if (!sky->data)
		sky->data = malloc(sky->vertex_count * sizeof(sky->data[0]));

	sky->texture = texturetranslation[skytexture];
	sky->width = texture->width;
	sky->height = texture->height;

	vertex_p = &sky->data[0];
	sky->loopcount = 0;

	for (yflip = 0; yflip < 2; yflip++)
	{
		sky->loops[sky->loopcount].mode = HWD_SKYLOOP_FAN;
		sky->loops[sky->loopcount].vertexindex = vertex_p - &sky->data[0];
		sky->loops[sky->loopcount].vertexcount = col_count;
		sky->loops[sky->loopcount].use_texture = false;
		sky->loopcount++;

		delta = 0.0f;

		for (c = 0; c < col_count; c++)
		{
			HWR_SkyDomeVertex(sky, vertex_p, 1, c, yflip, 0.0f, true);
			vertex_p->r = 255;
			vertex_p->g = 255;
			vertex_p->b = 255;
			vertex_p->a = 255;
			vertex_p++;
		}

		delta = (yflip ? 5.0f : -5.0f) / 128.0f;

		for (r = 0; r < row_count; r++)
		{
			sky->loops[sky->loopcount].mode = HWD_SKYLOOP_STRIP;
			sky->loops[sky->loopcount].vertexindex = vertex_p - &sky->data[0];
			sky->loops[sky->loopcount].vertexcount = 2 * col_count + 2;
			sky->loops[sky->loopcount].use_texture = true;
			sky->loopcount++;

			for (c = 0; c <= col_count; c++)
			{
				HWR_SkyDomeVertex(sky, vertex_p++, r + (yflip ? 1 : 0), (c ? c : 0), yflip, delta, false);
				HWR_SkyDomeVertex(sky, vertex_p++, r + (yflip ? 0 : 1), (c ? c : 0), yflip, delta, false);
			}
		}
	}
}

static void HWR_DrawSkyBackground(player_t *player)
{
	if (HWR_IsWireframeMode())
		return;

	HWD.pfnSetBlend(PF_Translucent|PF_NoDepthTest|PF_Modulated);

	HWR_GetTexture(texturetranslation[skytexture], false);

	                                                                             
                                                                              
                                                                                
	{
		FTransform dometransform;

		memcpy(&dometransform, &atransform, sizeof(FTransform));

		dometransform.x = 0.0f;
		dometransform.y = 0.0f;
		dometransform.z = 0.0f;

		HWR_SetTransformAiming(&dometransform, player, false);
		dometransform.angley = HWR_AngleToDegrees(viewangle-ANGLE_270);

		if (gl_sky.texture != texturetranslation[skytexture])
		{
			HWR_ClearSkyDome();
			HWR_BuildSkyDome();
		}

		if (HWR_UseShader())
			HWD.pfnSetShader(HWR_GetShaderFromTarget(SHADER_SKY));
		                                                                        
                                                                            
		HWD.pfnSetBlend(PF_Translucent|PF_NoDepthTest|PF_Modulated
			| (gl_sky.height > 128 ? PF_RemoveYWrap : 0));
		HWD.pfnSetTransform(&dometransform);
		HWD.pfnRenderSkyDome(&gl_sky);
	}
}





static inline void HWR_ClearView(void)
{







	HWD.pfnGClipRect((INT32)viewwindowx,
	                 (INT32)viewwindowy,
	                 (INT32)(viewwindowx + viewwidth),
	                 (INT32)(viewwindowy + viewheight),
	                 ZCLIP_PLANE);
	HWD.pfnClearBuffer(false, true, 0);




}





void HWR_SetViewSize(void)
{
	HWD.pfnFlushScreenTextures();
}



static void HWR_SetTransformAiming(FTransform *trans, player_t *player, boolean skybox)
{


	if (cv_glshearing.value == 1 || (cv_glshearing.value == 2 && R_IsViewpointThirdPerson(player, skybox)))
	{
		fixed_t fixedaiming = AIMINGTODY(aimingangle);
		trans->viewaiming = FIXED_TO_FLOAT(fixedaiming) * ((float)vid.width / vid.height) / ((float)BASEVIDWIDTH / BASEVIDHEIGHT);
		if (splitscreen)
			trans->viewaiming *= 2.125;  
		trans->shearing = true;
		gl_aimingangle = 0;
	}
	else
	{
		trans->shearing = false;
		gl_aimingangle = aimingangle;
	}

	trans->anglex = (float)(gl_aimingangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);
}




static void HWR_SetShaderState(void)
{
	HWD.pfnSetSpecialState(HWD_SET_SHADERS, (INT32)HWR_UseShader());
}

static void HWR_SetupView(player_t *player, INT32 viewnumber, float fpov, boolean skybox)
{
	postimg_t *type;

	if (splitscreen && player == &players[secondarydisplayplayer])
		type = &postimgtype2;
	else
		type = &postimgtype;

	if (!HWR_ShouldUsePaletteRendering())
	{

		player_t *saved_player = stplyr;
		stplyr = player;
		ST_doPaletteStuff();
		stplyr = saved_player;
#ifdef ALAM_LIGHTING
		HWR_SetLights(viewnumber);
#else
		(void)viewnumber;
#endif
	}


	if (skybox)
		R_SkyboxFrame(player);
	else
		R_SetupFrame(player);

	gl_viewx = FixedToFloat(viewx);
	gl_viewy = FixedToFloat(viewy);
	gl_viewz = FixedToFloat(viewz);
	gl_viewsin = FixedToFloat(viewsin);
	gl_viewcos = FixedToFloat(viewcos);



	memset(&atransform, 0x00, sizeof(FTransform));

	HWR_SetTransformAiming(&atransform, player, skybox);
	atransform.angley = HWR_AngleToDegrees(viewangle);

	gl_viewludsin = FixedToFloat(FINECOSINE(gl_aimingangle>>ANGLETOFINESHIFT));
	gl_viewludcos = FixedToFloat(-FINESINE(gl_aimingangle>>ANGLETOFINESHIFT));

	if (*type == postimg_flip)
		atransform.flip = true;
	else
		atransform.flip = false;

	atransform.x      = gl_viewx;
	atransform.y      = gl_viewy;
	atransform.z      = gl_viewz;
	atransform.scalex = 1;
	atransform.scaley = (float)vid.width/vid.height;
	atransform.scalez = 1;

	atransform.fovxangle = fpov;  
	atransform.fovyangle = fpov;  
	if (player->viewrollangle != 0)
	{
		fixed_t rol = AngleFixed(player->viewrollangle);
		atransform.rollangle = FixedToFloat(rol);
		atransform.roll = true;
		atransform.rollx = 1.0f;
		atransform.rollz = 0.0f;
	}
	atransform.splitscreen = splitscreen;

	gl_fovlud = (float)(1.0l/tan((double)(fpov*M_PIl/360l)));
}

typedef struct
{
	fixed_t x, y, z;
	angle_t angle;
} hwr_portalview_t;

typedef struct
{
	line_t *source;
	line_t *destination;
	hwr_portalview_t view;
	double distance;
	double aperture_low, aperture_high;
	boolean stencil_ready;
} hwr_mainportal_t;

static hwr_mainportal_t *hwr_main_portals = NULL;
static size_t hwr_main_portal_count = 0;

static boolean HWR_FindLinePortalDestinationUncached(line_t *start, line_t **result)
{
	const mtag_t tag = Tag_FGet(&start->tags);
	INT32 destnum;
	size_t p;

	                                                                       
                                              
	for (p = 0; (destnum = Tag_Iterate_Lines(tag, p)) >= 0; p++)
	{
		line_t *dest = &lines[destnum];
		if (dest == start || dest->special != start->special
		|| Tag_FGet(&dest->tags) != tag || !dest->frontsector)
			continue;
		*result = dest;
		return true;
	}

	return false;
}

static boolean HWR_FindLinePortalDestination(line_t *start, line_t **result)
{
	ptrdiff_t linenum;

	if (!start || !result)
		return false;

	linenum = start - lines;
	if (hwr_line_portal_cache_ready && linenum >= 0
	&& (size_t)linenum < (size_t)numlines)
	{
		*result = hwr_line_portal_destinations[linenum];
		return *result != NULL;
	}

	return HWR_FindLinePortalDestinationUncached(start, result);
}

static boolean HWR_BuildLinePortalView(line_t *start, line_t *dest,
	hwr_portalview_t *result)
{
	fixed_t startx, starty, destx, desty, disttopoint;
	angle_t angle, angtopoint;

	if (!start || !dest || !result || !start->frontsector || !dest->frontsector)
		return false;

	startx = (fixed_t)(((INT64)start->v1->x + start->v2->x) / 2);
	starty = (fixed_t)(((INT64)start->v1->y + start->v2->y) / 2);
	destx = (fixed_t)(((INT64)dest->v1->x + dest->v2->x) / 2);
	desty = (fixed_t)(((INT64)dest->v1->y + dest->v2->y) / 2);
	angle = R_PointToAngle2(0, 0, dest->dx, dest->dy)
		- R_PointToAngle2(start->dx, start->dy, 0, 0);
	disttopoint = R_PointToDist2(startx, starty, viewx, viewy);
	angtopoint = R_PointToAngle2(startx, starty, viewx, viewy) + angle;

	result->x = destx + FixedMul(FINECOSINE(angtopoint>>ANGLETOFINESHIFT), disttopoint);
	result->y = desty + FixedMul(FINESINE(angtopoint>>ANGLETOFINESHIFT), disttopoint);
	result->z = viewz + dest->frontsector->floorheight - start->frontsector->floorheight;
	result->angle = viewangle + angle;
	return true;
}

static void HWR_GetLinePortalApertureRange(line_t *line, double low,
	double high, FOutVector aperture[4])
{
	fixed_t x1, y1, x2, y2;
	fixed_t floor1, floor2, ceiling1, ceiling2;
	const fixed_t seam_pad = FRACUNIT / 64;
	const double linelength = hypot(HWR_FixedDelta(line->dx, 0),
		HWR_FixedDelta(line->dy, 0));
	const double padfraction = linelength > 0.0
		? ((1.0 / 64.0) / linelength) : 0.0;

	                                                                        
                                                                           
	if (low <= 0.0000001)
		low -= padfraction;
	if (high >= 0.9999999)
		high += padfraction;

	memset(aperture, 0, sizeof (*aperture) * 4);
	x1 = line->v1->x + (fixed_t)((double)line->dx * low);
	y1 = line->v1->y + (fixed_t)((double)line->dy * low);
	x2 = line->v1->x + (fixed_t)((double)line->dx * high);
	y2 = line->v1->y + (fixed_t)((double)line->dy * high);
	floor1 = P_GetSectorFloorZAt(line->frontsector, x1, y1);
	floor2 = P_GetSectorFloorZAt(line->frontsector, x2, y2);
	ceiling1 = P_GetSectorCeilingZAt(line->frontsector, x1, y1);
	ceiling2 = P_GetSectorCeilingZAt(line->frontsector, x2, y2);
	floor1 -= seam_pad;
	floor2 -= seam_pad;
	ceiling1 += seam_pad;
	ceiling2 += seam_pad;

	aperture[0].x = aperture[3].x = FixedToFloat(x1);
	aperture[1].x = aperture[2].x = FixedToFloat(x2);
	aperture[0].z = aperture[3].z = FixedToFloat(y1);
	aperture[1].z = aperture[2].z = FixedToFloat(y2);
	aperture[0].y = FixedToFloat(floor1);
	aperture[1].y = FixedToFloat(floor2);
	aperture[2].y = FixedToFloat(ceiling2);
	aperture[3].y = FixedToFloat(ceiling1);
}

                                                                            
                                                                          
                                                                             
                                                                         
static boolean HWR_ClipLinePortalToNearPlane(line_t *line, double *low,
	double *high)
{
	const double mindepth = 0.01;
	const double oldlow = *low;
	const double oldhigh = *high;
	const double dx = HWR_FixedDelta(line->v2->x, line->v1->x);
	const double dy = HWR_FixedDelta(line->v2->y, line->v1->y);
	const double x1 = HWR_FixedDelta(line->v1->x, viewx) + dx * oldlow;
	const double y1 = HWR_FixedDelta(line->v1->y, viewy) + dy * oldlow;
	const double x2 = HWR_FixedDelta(line->v1->x, viewx) + dx * oldhigh;
	const double y2 = HWR_FixedDelta(line->v1->y, viewy) + dy * oldhigh;
	const double forward1 = x1 * gl_viewcos + y1 * gl_viewsin;
	const double forward2 = x2 * gl_viewcos + y2 * gl_viewsin;
	double crossing;

	if (forward1 < mindepth && forward2 < mindepth)
		return false;
	if (forward1 >= mindepth && forward2 >= mindepth)
		return oldhigh - oldlow > 0.0000001;
	if (fabs(forward2 - forward1) < 0.0000001)
		return false;

	crossing = (mindepth - forward1) / (forward2 - forward1);
	crossing = oldlow + (oldhigh - oldlow) * crossing;
	if (forward1 < mindepth)
		*low = crossing;
	else
		*high = crossing;

	return *high - *low > 0.0000001;
}

static boolean HWR_BeginLinePortalStencil(line_t *line, INT32 stencilref,
	boolean clearstencil, double aperture_low, double aperture_high)
{
	FSurfaceInfo surface;
	FOutVector aperture[4];
	boolean visible;

	memset(&surface, 0, sizeof (surface));
	surface.PolyColor.rgba = 0xffffffff;
	visible = HWR_ClipLinePortalToNearPlane(line, &aperture_low,
		&aperture_high);
	HWD.pfnSetTransform(&atransform);
	                                                                          
                                
	HWD.pfnSetSpecialState(HWD_SET_PORTALSTENCIL,
		clearstencil ? 1 : 1000 + stencilref);
	if (!visible)
		return false;

	HWR_GetLinePortalApertureRange(line, aperture_low, aperture_high, aperture);
	                                                                            
                                                                         
                                                                           
	HWD.pfnDrawPolygon(&surface, aperture, 4,
		PF_NoTexture|PF_NoAlphaTest);
	return true;
}

static boolean HWR_BeginNestedLinePortalStencil(line_t *line, INT32 parentref)
{
	FSurfaceInfo surface;
	FOutVector aperture[4];
	double aperture_low = 0.0, aperture_high = 1.0;

	memset(&surface, 0, sizeof (surface));
	surface.PolyColor.rgba = 0xffffffff;
	if (!HWR_ClipLinePortalToNearPlane(line, &aperture_low, &aperture_high))
		return false;
	HWR_GetLinePortalApertureRange(line, aperture_low, aperture_high, aperture);
	HWD.pfnSetTransform(&atransform);
	                                                                           
                                                                             
                                                                
	HWD.pfnSetSpecialState(HWD_SET_PORTALSTENCIL, 2000 + parentref);
	HWD.pfnDrawPolygon(&surface, aperture, 4,
		PF_NoTexture|PF_NoAlphaTest);
	HWD.pfnSetSpecialState(HWD_SET_PORTALSTENCIL, 200 + parentref + 1);
	return true;
}

static boolean HWR_ClipPortalConstraint(double first, double second,
	double *low, double *high)
{
	double crossing;

	if (first < 0.0 && second < 0.0)
		return false;
	if (first >= 0.0 && second >= 0.0)
		return true;

	crossing = -first / (second - first);
	if (first < 0.0)
		*low = max(*low, crossing);
	else
		*high = min(*high, crossing);
	return *low <= *high;
}

                                                                           
                                                                             
                                                                          
                                       
static boolean HWR_ClipLinePortalToView(line_t *line, double *low,
	double *high, double padding)
{
	const double dx1 = HWR_FixedDelta(line->v1->x, viewx);
	const double dy1 = HWR_FixedDelta(line->v1->y, viewy);
	const double dx2 = HWR_FixedDelta(line->v2->x, viewx);
	const double dy2 = HWR_FixedDelta(line->v2->y, viewy);
	const double forward1 = dx1 * gl_viewcos + dy1 * gl_viewsin;
	const double forward2 = dx2 * gl_viewcos + dy2 * gl_viewsin;
	const double side1 = -dx1 * gl_viewsin + dy1 * gl_viewcos;
	const double side2 = -dx2 * gl_viewsin + dy2 * gl_viewcos;
	                                                                         
                                                                           
                                 
	const double tangent = gl_fovlud > 0.0f ? padding / gl_fovlud : padding;

	return HWR_ClipPortalConstraint(forward1 - 0.001,
		forward2 - 0.001, low, high)
		&& HWR_ClipPortalConstraint(tangent * forward1 - side1,
			tangent * forward2 - side2, low, high)
		&& HWR_ClipPortalConstraint(tangent * forward1 + side1,
			tangent * forward2 + side2, low, high)
		&& *high - *low > 0.0000001;
}

                                                                        
                                                                            
                                                                              
                                                                            
                                                                             
                                     
static boolean HWR_LinePortalInParentAperture(line_t *line, line_t *aperture,
	double *visiblelow, double *visiblehigh)
{
	double ax, ay, bx, by, q1x, q1y, q2x, q2y;
	double crossab, low = 0.0, high = 1.0;
	double first1, first2, second1, second2;
	double exitside1, exitside2;

	if (!aperture)
	{
		*visiblelow = low;
		*visiblehigh = high;
		return true;
	}

	ax = HWR_FixedDelta(aperture->v1->x, viewx);
	ay = HWR_FixedDelta(aperture->v1->y, viewy);
	bx = HWR_FixedDelta(aperture->v2->x, viewx);
	by = HWR_FixedDelta(aperture->v2->y, viewy);
	crossab = ax * by - ay * bx;
	if (fabs(crossab) < 0.000001)
		return false;
	if (crossab < 0.0)
	{
		const double swapx = ax, swapy = ay;
		ax = bx;
		ay = by;
		bx = swapx;
		by = swapy;
	}

	q1x = HWR_FixedDelta(line->v1->x, viewx);
	q1y = HWR_FixedDelta(line->v1->y, viewy);
	q2x = HWR_FixedDelta(line->v2->x, viewx);
	q2y = HWR_FixedDelta(line->v2->y, viewy);
	first1 = ax * q1y - ay * q1x;
	first2 = ax * q2y - ay * q2x;
	second1 = q1x * by - q1y * bx;
	second2 = q2x * by - q2y * bx;
	exitside1 = -HWR_FloatPortalLineSide(FixedToFloat(line->v1->x),
		FixedToFloat(line->v1->y), aperture);
	exitside2 = -HWR_FloatPortalLineSide(FixedToFloat(line->v2->x),
		FixedToFloat(line->v2->y), aperture);

	if (!(HWR_ClipPortalConstraint(first1, first2, &low, &high)
		&& HWR_ClipPortalConstraint(second1, second2, &low, &high)
		&& HWR_ClipPortalConstraint(exitside1, exitside2, &low, &high)))
		return false;

	*visiblelow = low;
	*visiblehigh = high;
	return high - low > 0.0000001;
}

                                                                             
                                                                               
                                                                           
                                                                           
                                        
static boolean HWR_LinePortalHasSightUncached(line_t *line, double low,
	double high)
{
	static const double samples[] =
		{0.5, 0.0625, 0.9375, 0.25, 0.75, 0.375, 0.625};
	mobj_t eye, target;
	UINT32 i;

	memset(&eye, 0, sizeof (eye));
	eye.x = viewx;
	eye.y = viewy;
	eye.z = viewz;
	eye.subsector = R_PointInSubsector(eye.x, eye.y);
	eye.thinker.function.acp1 = (actionf_p1)P_MobjThinker;

	for (i = 0; i < sizeof (samples) / sizeof (*samples); i++)
	{
		const double fraction = low + (high - low) * samples[i];
		fixed_t distance;
		fixed_t floorheight, ceilingheight;

		memset(&target, 0, sizeof (target));
		target.x = line->v1->x + (fixed_t)((double)line->dx * fraction);
		target.y = line->v1->y + (fixed_t)((double)line->dy * fraction);
		floorheight = P_GetSectorFloorZAt(line->frontsector, target.x, target.y);
		ceilingheight = P_GetSectorCeilingZAt(line->frontsector, target.x, target.y);
		if (ceilingheight <= floorheight)
			continue;

		distance = P_AproxDistance(viewx - target.x, viewy - target.y);
		if (distance > FRACUNIT/8)
		{
			target.x += FixedMul(FixedDiv(viewx - target.x, distance), FRACUNIT/8);
			target.y += FixedMul(FixedDiv(viewy - target.y, distance), FRACUNIT/8);
		}
		target.z = floorheight;
		target.height = ceilingheight - floorheight;
		target.subsector = R_PointInSubsector(target.x, target.y);
		target.thinker.function.acp1 = (actionf_p1)P_MobjThinker;

		if (P_CheckSight(&eye, &target))
			return true;
	}
	return false;
}

static boolean HWR_LinePortalHasSight(line_t *line, double low, double high)
{
	ptrdiff_t linenum;
	boolean visible;

	if (!line)
		return false;
	linenum = line - lines;
	if (!hwr_line_portal_sight_cache || linenum < 0
	|| (size_t)linenum >= (size_t)numlines)
		return HWR_LinePortalHasSightUncached(line, low, high);

	                                                                          
                                                                              
                                                                       
	if (hwr_line_portal_sight_cache[linenum])
		return hwr_line_portal_sight_cache[linenum] == 2;
	visible = HWR_LinePortalHasSightUncached(line, low, high);
	hwr_line_portal_sight_cache[linenum] = visible ? 2 : 1;
	return visible;
}

                                                                            
                                                                              
                                                                             
                                                                           
                                                                          
                                                                              
                                                                       
static boolean HWR_LinePortalHasSightFromExit(line_t *line, line_t *exit,
	double low, double high)
{
	static const double samples[] =
		{0.5, 0.0625, 0.9375, 0.25, 0.75, 0.375, 0.625};
	UINT32 i;

	if (!exit)
		return HWR_LinePortalHasSight(line, low, high);

	for (i = 0; i < sizeof (samples) / sizeof (*samples); i++)
	{
		const double fraction = low + (high - low) * samples[i];
		mobj_t eye, target;
		fixed_t rawx = line->v1->x
			+ (fixed_t)((double)line->dx * fraction);
		fixed_t rawy = line->v1->y
			+ (fixed_t)((double)line->dy * fraction);
		const double rx = HWR_FixedDelta(rawx, viewx);
		const double ry = HWR_FixedDelta(rawy, viewy);
		const double sx = HWR_FixedDelta(exit->v2->x, exit->v1->x);
		const double sy = HWR_FixedDelta(exit->v2->y, exit->v1->y);
		const double qx = HWR_FixedDelta(exit->v1->x, viewx);
		const double qy = HWR_FixedDelta(exit->v1->y, viewy);
		const double denominator = rx * sy - ry * sx;
		double rayfrac, linefrac;
		fixed_t distance;
		fixed_t floorheight, ceilingheight;

		if (fabs(denominator) < 0.000001)
			continue;
		rayfrac = (qx * sy - qy * sx) / denominator;
		linefrac = (qx * ry - qy * rx) / denominator;
		if (rayfrac < 0.0 || rayfrac > 1.0
		|| linefrac < 0.0 || linefrac > 1.0)
			continue;

		memset(&eye, 0, sizeof (eye));
		eye.x = viewx + (fixed_t)((double)((INT64)rawx - viewx) * rayfrac);
		eye.y = viewy + (fixed_t)((double)((INT64)rawy - viewy) * rayfrac);
		eye.z = viewz;
		distance = P_AproxDistance(rawx - eye.x, rawy - eye.y);
		if (distance > FRACUNIT/8)
		{
			eye.x += FixedMul(FixedDiv(rawx - eye.x, distance), FRACUNIT/8);
			eye.y += FixedMul(FixedDiv(rawy - eye.y, distance), FRACUNIT/8);
		}
		eye.subsector = R_PointInSubsector(eye.x, eye.y);
		eye.thinker.function.acp1 = (actionf_p1)P_MobjThinker;

		memset(&target, 0, sizeof (target));
		target.x = rawx;
		target.y = rawy;
		floorheight = P_GetSectorFloorZAt(line->frontsector, rawx, rawy);
		ceilingheight = P_GetSectorCeilingZAt(line->frontsector, rawx, rawy);
		if (ceilingheight <= floorheight)
			continue;
		distance = P_AproxDistance(eye.x - target.x, eye.y - target.y);
		if (distance > FRACUNIT/8)
		{
			target.x += FixedMul(FixedDiv(eye.x - target.x, distance), FRACUNIT/8);
			target.y += FixedMul(FixedDiv(eye.y - target.y, distance), FRACUNIT/8);
		}
		target.z = floorheight;
		target.height = ceilingheight - floorheight;
		target.subsector = R_PointInSubsector(target.x, target.y);
		target.thinker.function.acp1 = (actionf_p1)P_MobjThinker;

		if (P_CheckSight(&eye, &target))
			return true;
	}
	return false;
}

static double HWR_DistanceToPortalLine(line_t *line)
{
	double ax = FixedToFloat(line->v1->x);
	double ay = FixedToFloat(line->v1->y);
	double bx = FixedToFloat(line->v2->x);
	double by = FixedToFloat(line->v2->y);
	double px = FixedToFloat(viewx), py = FixedToFloat(viewy);
	double dx = bx - ax, dy = by - ay, length2 = dx * dx + dy * dy;
	double fraction = length2 > 0.0 ? ((px - ax) * dx + (py - ay) * dy) / length2 : 0.0;
	double nearx, neary;

	fraction = max(0.0, min(1.0, fraction));
	nearx = ax + fraction * dx;
	neary = ay + fraction * dy;
	dx = px - nearx;
	dy = py - neary;
	return dx * dx + dy * dy;
}

static boolean HWR_QueueMainPortal(line_t *start, angle_t visiblestart,
	angle_t visibleend)
{
	line_t *dest = NULL;
	hwr_portalview_t portalview;
	hwr_mainportal_t portal;
	size_t i, insert;
	double visiblelow = 0.0, visiblehigh = 1.0;
	const boolean reached_from_bsp = visiblestart != visibleend;

	for (i = 0; i < hwr_main_portal_count; i++)
		if (hwr_main_portals[i].source == start)
			return true;
	if (!start || !start->frontsector
	|| P_PointOnLineSide(viewx, viewy, start) != 0
	|| !HWR_ClipLinePortalToView(start, &visiblelow, &visiblehigh, 1.10)
	                                                                        
                                                                          
                                                                                 
	|| (!reached_from_bsp
		&& !HWR_LinePortalHasSight(start, visiblelow, visiblehigh))
	|| !HWR_FindLinePortalDestination(start, &dest)
	|| !HWR_BuildLinePortalView(start, dest, &portalview))
		return false;

	portal.source = start;
	portal.destination = dest;
	portal.view = portalview;
	portal.distance = HWR_DistanceToPortalLine(start);
	                                                                       
                                                                             
                                                                           
                                              
	portal.aperture_low = 0.0;
	portal.aperture_high = 1.0;
	portal.stencil_ready = false;
	insert = hwr_main_portal_count;
	while (insert > 0
	&& hwr_main_portals[insert - 1].distance < portal.distance)
	{
		hwr_main_portals[insert] = hwr_main_portals[insert - 1];
		insert--;
	}
	hwr_main_portals[insert] = portal;
	hwr_main_portal_count++;
	return true;
}

                                                                             
                                                                                
                                                                             
                                                             
static void HWR_PrecollectMainPortals(void)
{
	size_t i;

	for (i = 0; i < hwr_line_portal_count; i++)
	{
		line_t *start = hwr_line_portals[i];
		if (start->special != 40 || !start->frontsector
		|| P_PointOnLineSide(viewx, viewy, start) != 0)
			continue;
		HWR_QueueMainPortal(start, 0, 0);
	}
}

static size_t HWR_FinalizeMainPortalViews(void)
{
	size_t i;
	const size_t stencilcapacity = 255 / (size_t)max(1, R_PortalDepthLimit());

	                                                                       
                                                                         
                                                                          
                                                                        
	if (hwr_main_portal_count > stencilcapacity)
	{
		const size_t discarded = hwr_main_portal_count - stencilcapacity;
		memmove(hwr_main_portals, hwr_main_portals + discarded,
			stencilcapacity * sizeof (*hwr_main_portals));
		hwr_main_portal_count = stencilcapacity;
	}

	for (i = 0; i < hwr_main_portal_count; i++)
		hwr_visible_main_portals[i] = hwr_main_portals[i].source;
	hwr_visible_main_portal_count = hwr_main_portal_count;
	return hwr_main_portal_count;
}

                                                                               
                                                                          
static boolean HWR_FindPortalView(hwr_portalview_t *result)
{
	line_t *bestline = NULL;
	line_t *bestdest = NULL;
	line_t *fallbackline = NULL;
	line_t *fallbackdest = NULL;
	line_t *parentaperture = hwr_portal_pass ? hwr_portal_destination_line : NULL;
	double bestdist = 0.0;
	double fallbackdist = 0.0;
	size_t i;

	hwr_portal_source_line = NULL;
	hwr_portal_destination_line = NULL;

	                                                                    
                                                                     
   
                                                                          
                                                                          
                                                                            
                                                                          
                                                    
	for (i = 0; i < hwr_line_portal_count; i++)
	{
		line_t *start = hwr_line_portals[i];
		line_t *dest = NULL;
		double dist;
		double visiblelow = 0.0, visiblehigh = 1.0;

		if (start->special != 40 || !start->frontsector)
			continue;
		if (P_PointOnLineSide(viewx, viewy, start) != 0)
			continue;
		if (!HWR_LinePortalInParentAperture(start, parentaperture,
			&visiblelow, &visiblehigh))
			continue;
		                                                                       
                                                                          
                                                                           
                                                                           
                                                                         
		if (!HWR_ClipLinePortalToView(start, &visiblelow, &visiblehigh, 1.25))
			continue;
		if (!HWR_FindLinePortalDestination(start, &dest))
			continue;

		dist = HWR_DistanceToPortalLine(start);
		                                                                    
                                                                         
                                                                            
                                                                          
                                                                             
                                                              
		if (!fallbackline || dist < fallbackdist)
		{
			fallbackdist = dist;
			fallbackline = start;
			fallbackdest = dest;
		}
		if (!HWR_LinePortalHasSightFromExit(start, parentaperture,
			visiblelow, visiblehigh))
			continue;
		                                                                     
                                                                          
		if (!bestline || dist < bestdist)
		{
			bestdist = dist;
			bestline = start;
			bestdest = dest;
		}
	}

	if (!bestline)
	{
		bestline = fallbackline;
		bestdest = fallbackdest;
	}
	if (bestline)
	{
		hwr_portal_source_line = bestline;
		hwr_portal_destination_line = bestdest;
		return HWR_BuildLinePortalView(bestline, bestdest, result);
	}
	return false;
}

static void HWR_ApplyPortalView(const hwr_portalview_t *portal)
{
	viewx = portal->x;
	viewy = portal->y;
	viewz = portal->z;
	viewangle = portal->angle;
	viewsin = FINESINE(viewangle >> ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle >> ANGLETOFINESHIFT);
	if (hwr_portal_pass && hwr_portal_destination_line
	&& hwr_portal_destination_line->frontsector)
	{
		viewsector = hwr_portal_destination_line->frontsector;
		hwr_portal_cull_sector = hwr_portal_destination_line->frontsector;
		                                                                    
                                                       
		hwr_bsp_viewx = viewx;
		hwr_bsp_viewy = viewy;
	}
	else
	{
		viewsector = R_PointInSubsector(viewx, viewy)->sector;
		hwr_portal_cull_sector = NULL;
		hwr_bsp_viewx = viewx;
		hwr_bsp_viewy = viewy;
	}

	gl_viewx = FixedToFloat(viewx);
	gl_viewy = FixedToFloat(viewy);
	gl_viewz = FixedToFloat(viewz);
	gl_viewsin = FixedToFloat(viewsin);
	gl_viewcos = FixedToFloat(viewcos);
	atransform.x = gl_viewx;
	atransform.y = gl_viewy;
	atransform.z = gl_viewz;
	atransform.angley = HWR_AngleToDegrees(viewangle);
}

                                                                          
                                                                             
static boolean HWR_BuildPortalSkyboxView(const hwr_portalview_t *portal,
	hwr_portalview_t *result)
{
	mapheader_t *mh;

	if (!portal || !result || !skyboxmo[0] || !R_SkyboxEnabled())
		return false;

	result->x = skyboxmo[0]->x;
	result->y = skyboxmo[0]->y;
	result->z = skyboxmo[0]->z;
	result->angle = portal->angle + skyboxmo[0]->angle;
	mh = mapheaderinfo[gamemap-1];

	if (skyboxmo[1])
	{
		fixed_t x = 0, y = 0;
		const angle_t ang = skyboxmo[0]->angle >> ANGLETOFINESHIFT;

		if (mh->skybox_scalex > 0)
			x = (portal->x - skyboxmo[1]->x) / mh->skybox_scalex;
		else if (mh->skybox_scalex < 0)
			x = (portal->x - skyboxmo[1]->x) * -mh->skybox_scalex;

		if (mh->skybox_scaley > 0)
			y = (portal->y - skyboxmo[1]->y) / mh->skybox_scaley;
		else if (mh->skybox_scaley < 0)
			y = (portal->y - skyboxmo[1]->y) * -mh->skybox_scaley;

		result->x += FixedMul(x, FINECOSINE(ang)) - FixedMul(y, FINESINE(ang));
		result->y += FixedMul(x, FINESINE(ang)) + FixedMul(y, FINECOSINE(ang));
	}

	if (mh->skybox_scalez > 0)
		result->z += portal->z / mh->skybox_scalez;
	else if (mh->skybox_scalez < 0)
		result->z += portal->z * -mh->skybox_scalez;

	return true;
}

                                                                           
                                                                             
                                                                             
                                                       
static boolean HWR_RenderPortalSkybox(INT32 viewnumber, player_t *player,
	const hwr_portalview_t *portal, float fpov)
{
	hwr_portalview_t skyview;
	const boolean saved_drawsky = drawsky;
	line_t *saved_destination_line = hwr_portal_destination_line;

	if (!HWR_BuildPortalSkyboxView(portal, &skyview))
		return false;

	HWR_SetupView(player, viewnumber, fpov, false);
	HWR_ApplyPortalView(&skyview);
	 
	viewsector = R_PointInSubsector(viewx, viewy)->sector;
	hwr_portal_cull_sector = NULL;
	hwr_portal_destination_line = NULL;
	HWR_ClearView();
	HWR_DrawSkyBackground(player);
	drawsky = splitscreen;
	HWR_ClearSprites();
	drawcount = 0;

	if (rendermode == render_opengl)
	{
		const angle_t a1 = gld_FrustumAngle(fpov, gl_aimingangle);
		gld_clipper_Clear();
		gld_clipper_SafeAddClipRange(viewangle + a1, viewangle - a1);
#ifdef HAVE_SPHEREFRUSTRUM
		gld_FrustrumSetup();
#endif
	}

	HWD.pfnSetTransform(&atransform);
	HWR_SetShaderState();
	validcount++;
	if (cv_glbatching.value)
		HWR_StartBatching();
	HWR_RenderBSPNode((INT32)numnodes-1);
	if (cv_glbatching.value)
		HWR_RenderBatches();
	HWR_SortVisSprites();
	HWR_DrawSprites();
	if (numplanes || numpolyplanes || numwalls)
		HWR_CreateDrawNodes();
	HWD.pfnSetTransform(NULL);
	HWD.pfnUnSetShader();
	                                                                         
                                                                 
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, ZCLIP_PLANE);
	drawsky = saved_drawsky;
	hwr_portal_destination_line = saved_destination_line;
	return true;
}

static void HWR_RenderPortalBackground(INT32 viewnumber, player_t *player,
	const hwr_portalview_t *portal, INT32 depth, INT32 stencilref)
{
	const float fpov = FixedToFloat(R_GetPlayerFov(player));
	const boolean saved_drawsky = drawsky;
	line_t *current_source = hwr_portal_source_line;
	line_t *current_destination = hwr_portal_destination_line;
	line_t *current_nested_source = hwr_nested_portal_source_line;
	line_t *nested_source = NULL;
	line_t *nested_destination = NULL;
	hwr_portalview_t nestedview;
	boolean have_nested = false;
	boolean drew_skybox;
	hwr_portal_pass = true;
	HWD.pfnSetSpecialState(HWD_SET_PORTALSTENCIL, 200 + stencilref);

	drew_skybox = HWR_RenderPortalSkybox(viewnumber, player, portal, fpov);
	HWR_SetupView(player, viewnumber, fpov, false);
	HWR_ApplyPortalView(portal);
	HWR_ClearView();
	                                                                  
                                                                           
                                                                            
	if (!drew_skybox)
		HWR_DrawSkyBackground(player);
	drawsky = splitscreen;
	HWR_ClearSprites();
	drawcount = 0;

	                                                                            
                                                                             
                                                                           
                                                                              
	if (depth < R_PortalDepthLimit())
	{
		have_nested = HWR_FindPortalView(&nestedview);
		if (have_nested)
		{
			nested_source = hwr_portal_source_line;
			nested_destination = hwr_portal_destination_line;
		}
		hwr_portal_source_line = current_source;
		hwr_portal_destination_line = current_destination;
	}
	hwr_nested_portal_source_line = nested_source;

	if (rendermode == render_opengl)
	{
		angle_t a1 = gld_FrustumAngle(fpov, gl_aimingangle);
		gld_clipper_Clear();
		gld_clipper_SafeAddClipRange(viewangle + a1, viewangle - a1);
		if (hwr_portal_destination_line)
		{
			                                                                   
                                                                        
                                                                            
			angle_t end1 = R_PointToAngle2(viewx, viewy,
				hwr_portal_destination_line->v1->x,
				hwr_portal_destination_line->v1->y);
			angle_t end2 = R_PointToAngle2(viewx, viewy,
				hwr_portal_destination_line->v2->x,
				hwr_portal_destination_line->v2->y);
			if (end1 - end2 < ANGLE_180)
				gld_clipper_SafeAddClipRange(end1, end2);
			else
				gld_clipper_SafeAddClipRange(end2, end1);
		}
#ifdef HAVE_SPHEREFRUSTRUM
		gld_FrustrumSetup();
#endif
	}

	HWD.pfnSetTransform(&atransform);
	HWR_SetShaderState();
	validcount++;
	if (cv_glbatching.value)
		HWR_StartBatching();
	HWR_RenderBSPNode((INT32)numnodes-1);
	if (cv_glbatching.value)
		HWR_RenderBatches();
	HWR_SortVisSprites();
	HWR_DrawSprites();
	if (numplanes || numpolyplanes || numwalls)
		HWR_CreateDrawNodes();

	                                                                        
                                                                            
                                                                            
                                                             
	if (have_nested && nested_source)
	{
		HWR_SetupView(player, viewnumber, fpov, false);
		HWR_ApplyPortalView(portal);
		hwr_portal_source_line = nested_source;
		hwr_portal_destination_line = nested_destination;
		if (HWR_BeginNestedLinePortalStencil(nested_source, stencilref))
			HWR_RenderPortalBackground(viewnumber, player, &nestedview,
				depth + 1, stencilref + 1);
	}
	hwr_portal_source_line = current_source;
	hwr_portal_destination_line = current_destination;
	hwr_nested_portal_source_line = current_nested_source;
	HWD.pfnSetTransform(NULL);
	HWD.pfnUnSetShader();
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, NZCLIP_PLANE);
	drawsky = saved_drawsky;
	if (depth == 1)
		hwr_portal_pass = false;
	hwr_portal_cull_sector = NULL;
}

                                                                             
                                                                            
                                                                        
                                                                          
                                                                             
                                                             
static void HWR_RebuildMainViewDepth(INT32 viewnumber, player_t *player,
	float fpov)
{
	HWR_SetupView(player, viewnumber, fpov, false);
	HWR_ClearView();
	HWD.pfnSetSpecialState(HWD_SET_COLORWRITE, 0);
	HWR_ClearSprites();
	drawcount = 0;
	numwalls = numplanes = numpolyplanes = 0;

	if (rendermode == render_opengl)
	{
		const angle_t a1 = gld_FrustumAngle(fpov, gl_aimingangle);
		gld_clipper_Clear();
		gld_clipper_SafeAddClipRange(viewangle + a1, viewangle - a1);
#ifdef HAVE_SPHEREFRUSTRUM
		gld_FrustrumSetup();
#endif
	}

	HWD.pfnSetTransform(&atransform);
	HWR_SetShaderState();
	validcount++;
	if (cv_glbatching.value)
		HWR_StartBatching();
	HWR_RenderBSPNode((INT32)numnodes-1);
	if (cv_glbatching.value)
		HWR_RenderBatches();
	HWR_SortVisSprites();
	hwr_depth_only_sprites = true;
	HWR_DrawSprites();
	hwr_depth_only_sprites = false;

	                                                                           
                                                 
	numwalls = numplanes = numpolyplanes = 0;
	HWD.pfnSetSpecialState(HWD_SET_COLORWRITE, 1);
}




void HWR_RenderSkyboxView(INT32 viewnumber, player_t *player)
{
	const float fpov = FixedToFloat(R_GetPlayerFov(player));

	HWR_SetupView(player, viewnumber, fpov, true);


	NetUpdate();


	HWR_ClearView();

	if (drawsky)
		HWR_DrawSkyBackground(player);


	drawsky = splitscreen;

	HWR_ClearSprites();

	drawcount = 0;

	if (rendermode == render_opengl)
	{
		angle_t a1 = gld_FrustumAngle(fpov, gl_aimingangle);
		gld_clipper_Clear();
		gld_clipper_SafeAddClipRange(viewangle + a1, viewangle - a1);
#ifdef HAVE_SPHEREFRUSTRUM
		gld_FrustrumSetup();
#endif
	}



	HWD.pfnSetTransform(&atransform);


	HWR_SetShaderState();

	if (HWR_IsWireframeMode())
		HWD.pfnSetSpecialState(HWD_SET_WIREFRAME, 1);

	validcount++;

	if (cv_glbatching.value)
		HWR_StartBatching();

	HWR_RenderBSPNode((INT32)numnodes-1);

	if (cv_glbatching.value)
		HWR_RenderBatches();


	NetUpdate();

#ifdef ALAM_LIGHTING


	HWR_ResetLights();
#endif


	HWR_SortVisSprites();
	HWR_DrawSprites();

#ifdef NEWCORONAS

	HWR_DrawCoronas();
#endif

	if (numplanes || numpolyplanes || numwalls)  
	{
		HWR_CreateDrawNodes();
	}

	if (HWR_IsWireframeMode())
		HWD.pfnSetSpecialState(HWD_SET_WIREFRAME, 0);

	HWD.pfnSetTransform(NULL);
	HWD.pfnUnSetShader();


	NetUpdate();

	                                                                      
                                                                          
                                                                             
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, ZCLIP_PLANE);
}




void HWR_RenderPlayerView(INT32 viewnumber, player_t *player)
{
	const float fpov = FixedToFloat(R_GetPlayerFov(player));

	                                                                          
                                                                        
	const boolean skybox = (skyboxmo[0] && R_SkyboxEnabled());
	boolean portal_background = false;
	size_t portal_index;

	FRGBAFloat ClearColor;

	ClearColor.red = 0.0f;
	ClearColor.green = 0.0f;
	ClearColor.blue = 0.0f;
	ClearColor.alpha = 1.0f;
	hwr_portal_background_ready = false;
	hwr_portal_source_line = NULL;
	hwr_portal_destination_line = NULL;
	hwr_nested_portal_source_line = NULL;
	hwr_portal_cull_sector = NULL;
	hwr_main_portal_count = 0;
	hwr_visible_main_portal_count = 0;
	hwr_collect_main_portals = false;
	portal_linkdrawcount = 0;
	ps_hw_portaltime.value.p = 0;
	ps_hw_numportals.value.i = 0;
	ps_hw_textureuploadtime.value.p = 0;
	ps_hw_textureuploads.value.i = 0;
	ps_hw_batchedsprites.value.i = 0;
	ps_hw_numpolys.value.i = 0;
	ps_hw_numverts.value.i = 0;
	ps_hw_numcalls.value.i = 0;
	ps_hw_numshaders.value.i = 0;
	ps_hw_numtextures.value.i = 0;
	ps_hw_numpolyflags.value.i = 0;
	ps_hw_numcolors.value.i = 0;
	ps_hw_batchsorttime.value.p = 0;
	ps_hw_batchdrawtime.value.p = 0;

	if (cv_glshaders.value)
		HWD.pfnSetShaderInfo(HWD_SHADERINFO_LEVELTIME, (INT32)leveltime);  

	if (viewnumber == 0)  
		HWD.pfnClearBuffer(true, false, &ClearColor);  

	PS_START_TIMING(ps_hw_skyboxtime);
	if (skybox && drawsky)
		HWR_RenderSkyboxView(viewnumber, player);
	PS_STOP_TIMING(ps_hw_skyboxtime);

	                                                                          
                                                                           
                                                                
	HWR_SetupView(player, viewnumber, fpov, false);

	framecount++;  


	NetUpdate();


	HWR_ClearView();  

	if (!skybox && drawsky)
		HWR_DrawSkyBackground(player);


	drawsky = splitscreen;

	HWR_ClearSprites();

	drawcount = 0;

	if (rendermode == render_opengl)
	{
		angle_t a1 = gld_FrustumAngle(fpov, gl_aimingangle);
		gld_clipper_Clear();
		gld_clipper_SafeAddClipRange(viewangle + a1, viewangle - a1);
#ifdef HAVE_SPHEREFRUSTRUM
		gld_FrustrumSetup();
#endif
	}



	HWD.pfnSetTransform(&atransform);


	HWR_SetShaderState();

	if (HWR_IsWireframeMode())
		HWD.pfnSetSpecialState(HWD_SET_WIREFRAME, 1);

	ps_numbspcalls.value.i = 0;
	ps_numpolyobjects.value.i = 0;
	PS_START_TIMING(ps_bsptime);

	validcount++;
	hwr_collect_main_portals = HWR_VisuportalsActive() && !splitscreen
		&& R_PortalDepthLimit() > 0;
	if (hwr_collect_main_portals)
	{
		const boolean same_view = hwr_line_portal_sight_view_valid
			&& hwr_line_portal_sight_tic == gametic
			&& hwr_line_portal_sight_viewnumber == viewnumber
			&& hwr_line_portal_sight_x == viewx
			&& hwr_line_portal_sight_y == viewy
			&& hwr_line_portal_sight_z == viewz
			&& hwr_line_portal_sight_angle == viewangle
			&& hwr_line_portal_sight_aiming == gl_aimingangle;

		if (hwr_line_portal_sight_cache && !same_view)
			memset(hwr_line_portal_sight_cache, 0,
				(size_t)numlines * sizeof (*hwr_line_portal_sight_cache));
		hwr_line_portal_sight_view_valid = true;
		hwr_line_portal_sight_tic = gametic;
		hwr_line_portal_sight_viewnumber = viewnumber;
		hwr_line_portal_sight_x = viewx;
		hwr_line_portal_sight_y = viewy;
		hwr_line_portal_sight_z = viewz;
		hwr_line_portal_sight_angle = viewangle;
		hwr_line_portal_sight_aiming = gl_aimingangle;
		HWR_PrecollectMainPortals();
	}

	if (cv_glbatching.value)
		HWR_StartBatching();

	HWR_RenderBSPNode((INT32)numnodes-1);
	hwr_collect_main_portals = false;
	if (hwr_main_portal_count)
	{
		portal_background = HWR_FinalizeMainPortalViews() > 0;
		hwr_portal_background_ready = portal_background;
	}

	PS_STOP_TIMING(ps_bsptime);

	if (cv_glbatching.value)
		HWR_RenderBatches();


	NetUpdate();

#ifdef ALAM_LIGHTING


	HWR_ResetLights();
#endif


	ps_numsprites.value.i = gl_visspritecount;
	PS_START_TIMING(ps_hw_spritesorttime);
	HWR_SortVisSprites();
	PS_STOP_TIMING(ps_hw_spritesorttime);
	PS_START_TIMING(ps_hw_spritedrawtime);
	HWR_DrawSprites();
	if (portal_background)
		HWR_PortalLinkDrawFinish();
	PS_STOP_TIMING(ps_hw_spritedrawtime);

#ifdef NEWCORONAS

	HWR_DrawCoronas();
#endif

	ps_numdrawnodes.value.i = 0;
	ps_hw_nodesorttime.value.p = 0;
	ps_hw_nodedrawtime.value.p = 0;
	                                                                      
                                                                             
                                                                                
	if (numplanes || numpolyplanes || numwalls)  
	{
		if (portal_background)
			HWR_SaveMainPortalDrawNodes();
		else
			HWR_CreateDrawNodes();
	}

	if (portal_background)
	{
		const INT32 portaldepth = R_PortalDepthLimit();
		ps_hw_numportals.value.i = (INT32)hwr_main_portal_count;
		PS_START_TIMING(ps_hw_portaltime);

		                                                                     
                                                                           
                                                                  
		HWR_SetupView(player, viewnumber, fpov, false);
		for (portal_index = 0; portal_index < hwr_main_portal_count; portal_index++)
		{
			const INT32 stencilref = 1 + (INT32)portal_index * portaldepth;
			hwr_mainportal_t *portal = &hwr_main_portals[portal_index];
			portal->stencil_ready = HWR_BeginLinePortalStencil(portal->source, stencilref,
				portal_index == 0, portal->aperture_low, portal->aperture_high);
		}

		for (portal_index = 0; portal_index < hwr_main_portal_count; portal_index++)
		{
			hwr_mainportal_t *portal = &hwr_main_portals[portal_index];
			const INT32 stencilref = 1 + (INT32)portal_index * portaldepth;
			if (!portal->stencil_ready)
				continue;

			hwr_portal_source_line = portal->source;
			hwr_portal_destination_line = portal->destination;
			HWR_RenderPortalBackground(viewnumber, player, &portal->view, 1,
				stencilref);
		}
		HWD.pfnSetSpecialState(HWD_SET_PORTALSTENCIL, 0);
		hwr_portal_source_line = NULL;
		hwr_portal_destination_line = NULL;
		HWR_SetupView(player, viewnumber, fpov, false);

	}

	if (mainportal_savednumplanes || mainportal_savednumpolyplanes
	|| mainportal_savednumwalls)
		HWR_RebuildMainViewDepth(viewnumber, player, fpov);

	if (portal_background)
		PS_STOP_TIMING(ps_hw_portaltime);

	if (mainportal_savednumplanes || mainportal_savednumpolyplanes
	|| mainportal_savednumwalls)
	{
		HWR_RestoreMainPortalDrawNodes();
		HWR_CreateDrawNodes();
	}

	if (HWR_IsWireframeMode())
		HWD.pfnSetSpecialState(HWD_SET_WIREFRAME, 0);

	HWD.pfnSetTransform(NULL);
	HWD.pfnUnSetShader();

	HWR_DoPostProcessor(player);


	NetUpdate();



	HWD.pfnGClipRect(0, 0, vid.width, vid.height, NZCLIP_PLANE);
	hwr_portal_background_ready = false;
	hwr_portal_source_line = NULL;
	hwr_portal_destination_line = NULL;
	hwr_portal_cull_sector = NULL;
	hwr_main_portal_count = 0;
	hwr_visible_main_portal_count = 0;
	hwr_collect_main_portals = false;
}



boolean HWR_ShouldUsePaletteRendering(void)
{
	return (pMasterPalette != NULL && cv_glpaletterendering.value && HWR_UseShader());
}




static void HWR_TogglePaletteRendering(void)
{

	if (HWR_ShouldUsePaletteRendering())
	{

		if (!gl_palette_rendering_state)
		{
			gl_palette_rendering_state = true;




			textureformat = GL_TEXFMT_AP_88;

			HWR_SetMapPalette();
			HWR_SetPalette(pLocalPalette);




			HWR_LoadMapTextures(numtextures);
		}
	}
	else
	{

		if (gl_palette_rendering_state)
		{
			gl_palette_rendering_state = false;
			textureformat = GL_TEXFMT_RGBA;
			HWR_SetPalette(pLocalPalette);



			HWR_LoadMapTextures(numtextures);
		}
	}
}

void HWR_LoadLevel(void)
{
	size_t i;

#ifdef ALAM_LIGHTING

	HWR_ResetLights();
#endif

	                                                                          
                                                                                
	hwr_line_portals = NULL;
	hwr_line_portal_destinations = NULL;
	hwr_line_portal_sight_cache = NULL;
	hwr_line_portal_sight_view_valid = false;
	hwr_line_portal_count = 0;
	hwr_line_portal_cache_ready = false;
	hwr_visible_main_portals = NULL;
	hwr_visible_main_portal_count = 0;
	hwr_main_portals = NULL;
	hwr_main_portal_count = 0;
	for (i = 0; i < (size_t)numlines; i++)
		if (lines[i].special == 40)
			hwr_line_portal_count++;
	if (hwr_line_portal_count)
	{
		size_t p = 0;
		hwr_line_portals = Z_Malloc(hwr_line_portal_count * sizeof (*hwr_line_portals),
			PU_LEVEL, NULL);
		hwr_line_portal_destinations = Z_Calloc((size_t)numlines
			* sizeof (*hwr_line_portal_destinations), PU_LEVEL, NULL);
		hwr_line_portal_sight_cache = Z_Calloc((size_t)numlines
			* sizeof (*hwr_line_portal_sight_cache), PU_LEVEL, NULL);
		hwr_visible_main_portals = Z_Malloc(hwr_line_portal_count
			* sizeof (*hwr_visible_main_portals), PU_LEVEL, NULL);
		hwr_main_portals = Z_Malloc(hwr_line_portal_count
			* sizeof (*hwr_main_portals), PU_LEVEL, NULL);
		for (i = 0; i < (size_t)numlines; i++)
			if (lines[i].special == 40)
			{
				line_t *destination = NULL;
				hwr_line_portals[p++] = &lines[i];
				if (HWR_FindLinePortalDestinationUncached(&lines[i], &destination))
					hwr_line_portal_destinations[i] = destination;
			}
		hwr_line_portal_cache_ready = true;
	}

	HWR_CreatePlanePolygons((INT32)numnodes - 1);

	HWR_ClearSkyDome();
	HWR_BuildSkyDome();

	if (HWR_ShouldUsePaletteRendering())
		HWR_SetMapPalette();

	gl_maploaded = true;
}





static CV_PossibleValue_t glshaders_cons_t[] = {{0, "Off"}, {1, "On"}, {2, "Ignore custom shaders"}, {0, NULL}};
static CV_PossibleValue_t glmodelinterpolation_cons_t[] = {{0, "Off"}, {1, "Sometimes"}, {2, "Always"}, {0, NULL}};
static CV_PossibleValue_t glfakecontrast_cons_t[] = {{0, "Off"}, {1, "On"}, {2, "Smooth"}, {0, NULL}};
static CV_PossibleValue_t glshearing_cons_t[] = {{0, "Off"}, {1, "On"}, {2, "Third-person"}, {0, NULL}};

static void CV_glfiltermode_OnChange(void);
static void CV_glspritefiltermode_OnChange(void);
static void CV_glanisotropic_OnChange(void);
static void CV_glmodellighting_OnChange(void);
static void CV_glpaletterendering_OnChange(void);
static void CV_glpalettedepth_OnChange(void);
static void CV_glshaders_OnChange(void);
static void CV_gltruecolorpngs_OnChange(void);

static CV_PossibleValue_t glfiltermode_cons_t[]= {{HWD_SET_TEXTUREFILTER_POINTSAMPLED, "Nearest"},
	{HWD_SET_TEXTUREFILTER_BILINEAR, "Bilinear"}, {HWD_SET_TEXTUREFILTER_TRILINEAR, "Trilinear"},
	{HWD_SET_TEXTUREFILTER_MIXED1, "Linear_Nearest"},
	{HWD_SET_TEXTUREFILTER_MIXED2, "Nearest_Linear"},
	{HWD_SET_TEXTUREFILTER_MIXED3, "Nearest_Mipmap"},
	{HWD_SET_TEXTUREFILTER_SHARP, "Sharp"},
	{0, NULL}};
static CV_PossibleValue_t glspritefiltermode_cons_t[] = {
	{HWD_SET_TEXTUREFILTER_POINTSAMPLED, "Crisp"},
	{HWD_SET_TEXTUREFILTER_BILINEAR, "Smooth"},
	{HWD_SET_TEXTUREFILTER_TRILINEAR, "Mipmap"},
	{HWD_SET_TEXTUREFILTER_SHARP, "Sharp"},
	{0, NULL}};
CV_PossibleValue_t glanisotropicmode_cons_t[] = {{0, "None"}, {1, "MIN"}, {16, "MAX"}, {0, NULL}};

consvar_t cv_glshaders = CVAR_INIT ("gr_shaders", "On", CV_SAVE|CV_CALL, glshaders_cons_t, CV_glshaders_OnChange);

#ifdef ALAM_LIGHTING
consvar_t cv_gldynamiclighting = CVAR_INIT ("gr_dynamiclighting", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_glstaticlighting  = CVAR_INIT ("gr_staticlighting", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_glcoronas = CVAR_INIT ("gr_coronas", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_glcoronasize = CVAR_INIT ("gr_coronasize", "1", CV_SAVE|CV_FLOAT, 0, NULL);
#endif

consvar_t cv_glmodels = CVAR_INIT ("gr_models", "Off", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_glmodelinterpolation = CVAR_INIT ("gr_modelinterpolation", "Sometimes", CV_SAVE, glmodelinterpolation_cons_t, NULL);
consvar_t cv_glmodellighting = CVAR_INIT ("gr_modellighting", "Off", CV_SAVE|CV_CALL, CV_OnOff, CV_glmodellighting_OnChange);

consvar_t cv_glshearing = CVAR_INIT ("gr_shearing", "Off", CV_SAVE, glshearing_cons_t, NULL);
consvar_t cv_glspritebillboarding = CVAR_INIT ("gr_spritebillboarding", "Off", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_glskydome = CVAR_INIT ("gr_skydome", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_glfakecontrast = CVAR_INIT ("gr_fakecontrast", "Smooth", CV_SAVE, glfakecontrast_cons_t, NULL);
consvar_t cv_glvisuportals = CVAR_INIT ("gr_visuportals", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_glslopecontrast = CVAR_INIT ("gr_slopecontrast", "Off", CV_SAVE, CV_OnOff, NULL);

consvar_t cv_glfiltermode = CVAR_INIT ("gr_filtermode", "Nearest", CV_SAVE|CV_CALL, glfiltermode_cons_t, CV_glfiltermode_OnChange);
consvar_t cv_glspritefiltermode = CVAR_INIT ("gr_spritefiltermode", "Crisp", CV_SAVE|CV_CALL, glspritefiltermode_cons_t, CV_glspritefiltermode_OnChange);
consvar_t cv_glanisotropicmode = CVAR_INIT ("gr_anisotropicmode", "None", CV_SAVE|CV_CALL, glanisotropicmode_cons_t, CV_glanisotropic_OnChange);

consvar_t cv_glsolvetjoin = CVAR_INIT ("gr_solvetjoin", "On", 0, CV_OnOff, NULL);

consvar_t cv_glbatching = CVAR_INIT ("gr_batching", "On", 0, CV_OnOff, NULL);
consvar_t cv_glretaintextures = CVAR_INIT ("gr_retaintextures", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_gltruecolorpngs = CVAR_INIT ("gr_truecolorpngs", "On", CV_SAVE|CV_CALL,
	CV_OnOff, CV_gltruecolorpngs_OnChange);

static CV_PossibleValue_t glpalettedepth_cons_t[] = {{16, "16 bits"}, {24, "24 bits"}, {0, NULL}};

consvar_t cv_glpaletterendering = CVAR_INIT ("gr_paletterendering", "On", CV_SAVE|CV_CALL, CV_OnOff, CV_glpaletterendering_OnChange);
consvar_t cv_glpalettedepth = CVAR_INIT ("gr_palettedepth", "16 bits", CV_SAVE|CV_CALL, glpalettedepth_cons_t, CV_glpalettedepth_OnChange);

#define ONLY_IF_GL_LOADED if (vid.glstate != VID_GL_LIBRARY_LOADED) return;
consvar_t cv_glwireframe = CVAR_INIT ("gr_wireframe", "Off", 0, CV_OnOff, NULL);

static void CV_glfiltermode_OnChange(void)
{
	ONLY_IF_GL_LOADED
	HWD.pfnSetSpecialState(HWD_SET_TEXTUREFILTERMODE, cv_glfiltermode.value);
}

static void CV_glspritefiltermode_OnChange(void)
{
	ONLY_IF_GL_LOADED
	HWD.pfnSetSpecialState(HWD_SET_SPRITEFILTERMODE, cv_glspritefiltermode.value);
}

static void CV_glanisotropic_OnChange(void)
{
	ONLY_IF_GL_LOADED
	HWD.pfnSetSpecialState(HWD_SET_TEXTUREANISOTROPICMODE, cv_glanisotropicmode.value);
}

static void CV_glmodellighting_OnChange(void)
{
	ONLY_IF_GL_LOADED

	if (gl_shadersavailable)
		HWR_CompileShaders();
}

static void CV_glpaletterendering_OnChange(void)
{
	ONLY_IF_GL_LOADED
	if (gl_shadersavailable)
	{
		HWR_CompileShaders();
		HWR_TogglePaletteRendering();
	}
}

static void CV_glpalettedepth_OnChange(void)
{
	ONLY_IF_GL_LOADED

	if (HWR_ShouldUsePaletteRendering())
		HWR_SetPalette(pLocalPalette);
}

static void CV_glshaders_OnChange(void)
{
	ONLY_IF_GL_LOADED
	HWR_SetShaderState();
	if (cv_glpaletterendering.value)
	{

		HWR_TogglePaletteRendering();
	}
}

static void CV_gltruecolorpngs_OnChange(void)
{
	ONLY_IF_GL_LOADED
	                                                                           
                                                                          
	HWR_ClearAllTextures();
	HWR_LoadMapTextures(numtextures);
	if (cv_gltruecolorpngs.value && HWR_ShouldUsePaletteRendering())
		CONS_Printf("True-color PNGs are loaded, but Palette rendering still reduces the final scene to the SRB2 palette.\n");
}


void HWR_AddCommands(void)
{
#ifdef ALAM_LIGHTING
	CV_RegisterVar(&cv_glstaticlighting);
	CV_RegisterVar(&cv_gldynamiclighting);
	CV_RegisterVar(&cv_glcoronasize);
	CV_RegisterVar(&cv_glcoronas);
#endif

	CV_RegisterVar(&cv_glmodellighting);
	CV_RegisterVar(&cv_glmodelinterpolation);
	CV_RegisterVar(&cv_glmodels);

	CV_RegisterVar(&cv_glskydome);
	CV_RegisterVar(&cv_glspritebillboarding);
	CV_RegisterVar(&cv_glfakecontrast);
	CV_RegisterVar(&cv_glvisuportals);
	CV_RegisterVar(&cv_glslopecontrast);
	CV_RegisterVar(&cv_glshearing);
	CV_RegisterVar(&cv_glshaders);

	CV_RegisterVar(&cv_glfiltermode);
	CV_RegisterVar(&cv_glspritefiltermode);
	CV_RegisterVar(&cv_glanisotropicmode);
	CV_RegisterVar(&cv_glsolvetjoin);

	CV_RegisterVar(&cv_glbatching);
	CV_RegisterVar(&cv_glretaintextures);
	CV_RegisterVar(&cv_gltruecolorpngs);

	CV_RegisterVar(&cv_glpaletterendering);
	CV_RegisterVar(&cv_glpalettedepth);
	CV_RegisterVar(&cv_glwireframe);
}




void HWR_Startup(void)
{
	if (!gl_init)
	{
		CONS_Printf("HWR_Startup()...\n");

		textureformat = patchformat = GL_TEXFMT_RGBA;

		HWR_InitPolyPool();
		HWR_InitMapTextures();
		HWR_InitModels();
#ifdef ALAM_LIGHTING
		HWR_InitLight();
#endif

		gl_shadersavailable = HWR_InitShaders();
		HWR_SetShaderState();
		HWR_LoadAllCustomShaders();
		HWR_TogglePaletteRendering();
	}

	gl_init = true;
}




void HWR_Switch(void)
{

	HWD.pfnSetSpecialState(HWD_SET_MULTISAMPLE, cv_glmsaa.value != 0);
	HWD.pfnSetSpecialState(HWD_SET_TEXTUREFILTERMODE, cv_glfiltermode.value);
	HWD.pfnSetSpecialState(HWD_SET_SPRITEFILTERMODE, cv_glspritefiltermode.value);
	HWD.pfnSetSpecialState(HWD_SET_TEXTUREANISOTROPICMODE, cv_glanisotropicmode.value);


	if (!HWR_MapTexturesAreCurrent(numtextures))
		HWR_LoadMapTextures(numtextures);


	if (!gl_maploaded && (gamestate == GS_LEVEL || (gamestate == GS_TITLESCREEN && titlemapinaction)))
	{
		HWR_ClearAllTextures();
		HWR_LoadLevel();
	}
}

void HWR_SetMultisample(boolean enabled)
{
	HWD.pfnSetSpecialState(HWD_SET_MULTISAMPLE, enabled);
}




void HWR_Shutdown(void)
{
	CONS_Printf("HWR_Shutdown()\n");
	HWR_FreeExtraSubsectors();
	HWR_FreePolyPool();
	HWR_FreeMapTextures();
	HWD.pfnFlushScreenTextures();
}

void transform(float *cx, float *cy, float *cz)
{
	float tr_x,tr_y;

	tr_x = *cx - gl_viewx;
	tr_y = *cz - gl_viewy;



	*cx = (tr_x * gl_viewsin) - (tr_y * gl_viewcos);
	tr_x = (tr_x * gl_viewcos) + (tr_y * gl_viewsin);


	tr_y = *cy - gl_viewz;

	*cy = (tr_x * gl_viewludcos) + (tr_y * gl_viewludsin);
	*cz = (tr_x * gl_viewludsin) - (tr_y * gl_viewludcos);


	*cy *= ORIGINAL_ASPECT * gl_fovlud;
	*cx *= gl_fovlud;
}

void HWR_AddTransparentWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, INT32 texnum, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap)
{
	static size_t allocedwalls = 0;

	if (!r_renderwalls)
		return;


	if (!wallinfo)
		allocedwalls = 0;

	if (allocedwalls < numwalls + 1)
	{
		allocedwalls += MAX_TRANSPARENTWALL;
		Z_Realloc(wallinfo, allocedwalls * sizeof (*wallinfo), PU_LEVEL, &wallinfo);
	}

	M_Memcpy(wallinfo[numwalls].wallVerts, wallVerts, sizeof (wallinfo[numwalls].wallVerts));
	M_Memcpy(&wallinfo[numwalls].Surf, pSurf, sizeof (FSurfaceInfo));
	wallinfo[numwalls].texnum = texnum;
	wallinfo[numwalls].blend = blend;
	wallinfo[numwalls].drawcount = drawcount++;
	wallinfo[numwalls].fogwall = fogwall;
	wallinfo[numwalls].lightlevel = lightlevel;
	wallinfo[numwalls].wallcolormap = wallcolormap;
	numwalls++;
}

void HWR_RenderWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap)
{
	FBITFIELD blendmode = blend;
	UINT8 alpha = pSurf->PolyColor.s.alpha;  

	INT32 shader = SHADER_NONE;

	if (!r_renderwalls)
		return;


	HWR_Lighting(pSurf, lightlevel, wallcolormap);

	pSurf->PolyColor.s.alpha = alpha;  

	if (blend & PF_Environment)
		blendmode |= PF_Occlude;	 

	if (HWR_UseShader())
	{
		if (fogwall)
			shader = SHADER_FOG;
		else
			shader = SHADER_WALL;

		blendmode |= PF_ColorMapped;
	}

	if (fogwall)
		blendmode |= PF_Fog;

	blendmode |= PF_Modulated;	 
	HWR_ProcessPolygon(pSurf, wallVerts, 4, blendmode, shader, false);
}

UINT32 HWR_GetTextureUsed(void)
{
	return HWD.pfnGetTextureUsed();
}

void HWR_DoPostProcessor(player_t *player)
{
	postimg_t *type;

	HWD.pfnUnSetShader();

	if (splitscreen && player == &players[secondarydisplayplayer])
		type = &postimgtype2;
	else
		type = &postimgtype;



	if (player->flashcount && !HWR_ShouldUsePaletteRendering())
	{
		FOutVector      v[4];
		FSurfaceInfo Surf;

		v[0].x = v[2].y = v[3].x = v[3].y = -4.0f;
		v[0].y = v[1].x = v[1].y = v[2].x = 4.0f;
		v[0].z = v[1].z = v[2].z = v[3].z = 4.0f;  


		if (player->flashpal == PAL_NUKE)
		{
			Surf.PolyColor.s.red = 0xff;
			Surf.PolyColor.s.green = Surf.PolyColor.s.blue = 0x7F;  
		}
		else
			Surf.PolyColor.s.red = Surf.PolyColor.s.green = Surf.PolyColor.s.blue = 0xff;

		Surf.PolyColor.s.alpha = 0xc0;  

		HWD.pfnDrawPolygon(&Surf, v, 4, PF_Modulated|PF_Additive|PF_NoTexture|PF_NoDepthTest);
	}


	if(gamestate != GS_INTERMISSION)
		HWD.pfnMakeScreenTexture(HWD_SCREENTEXTURE_GENERIC1);

	if (splitscreen)  
		return;


	if (*type == postimg_water || *type == postimg_heat)
	{

		float v[SCREENVERTS][SCREENVERTS][2];
		float disStart = (leveltime-1) + FIXED_TO_FLOAT(rendertimefrac);

		UINT8 x, y;
		INT32 WAVELENGTH;
		INT32 AMPLITUDE;
		INT32 FREQUENCY;


		if (*type == postimg_water)
		{
			WAVELENGTH = 5;
			AMPLITUDE = 40;
			FREQUENCY = 8;
		}
		else
		{
			WAVELENGTH = 10;
			AMPLITUDE = 60;
			FREQUENCY = 4;
		}

		for (x = 0; x < SCREENVERTS; x++)
		{
			for (y = 0; y < SCREENVERTS; y++)
			{

				v[x][y][0] = (x/((float)(SCREENVERTS-1.0f)/9.0f))-4.5f + (float)sin((disStart+(y*WAVELENGTH))/FREQUENCY)/AMPLITUDE;
				v[x][y][1] = (y/((float)(SCREENVERTS-1.0f)/9.0f))-4.5f;
			}
		}
		HWD.pfnPostImgRedraw(v);


		if(gamestate != GS_INTERMISSION)
			HWD.pfnMakeScreenTexture(HWD_SCREENTEXTURE_GENERIC1);
	}

}

void HWR_StartScreenWipe(void)
{

	HWD.pfnMakeScreenTexture(HWD_SCREENTEXTURE_WIPE_START);
}

void HWR_EndScreenWipe(void)
{

	HWD.pfnMakeScreenTexture(HWD_SCREENTEXTURE_WIPE_END);
}

void HWR_DrawIntermissionBG(void)
{
	HWD.pfnDrawScreenTexture(HWD_SCREENTEXTURE_GENERIC1, NULL, 0);
}




static lumpnum_t wipelumpnum;


static boolean HWR_WipeCheck(UINT8 wipenum, UINT8 scrnnum)
{
	static char lumpname[9] = "FADEmmss";
	size_t lsize;


	if (wipenum > 99 || scrnnum > 99)
		return false;  


	lumpname[4] = '0'+(wipenum/10);
	lumpname[5] = '0'+(wipenum%10);
	lumpname[6] = '0'+(scrnnum/10);
	lumpname[7] = '0'+(scrnnum%10);
	wipelumpnum = W_CheckNumForName(lumpname);


	if (wipelumpnum == LUMPERROR)
		return false;

	lsize = W_LumpLength(wipelumpnum);
	if (!(lsize == 256000 || lsize == 64000 || lsize == 16000 || lsize == 4000))
	{
		CONS_Alert(CONS_WARNING, "Fade mask lump %s of incorrect size, ignored\n", lumpname);
		return false;  
	}

	return true;
}

void HWR_DoWipe(UINT8 wipenum, UINT8 scrnnum)
{
	if (!HWR_WipeCheck(wipenum, scrnnum))
		return;

	HWR_GetFadeMask(wipelumpnum);
	if (wipestyle == WIPESTYLE_COLORMAP && HWR_UseShader())
	{
		FSurfaceInfo surf = {0};
		FBITFIELD polyflags = PF_Modulated|PF_NoDepthTest;

		polyflags |= (wipestyleflags & WSF_TOWHITE) ? PF_Additive : PF_ReverseSubtract;
		surf.PolyColor.s.red = FADEREDFACTOR;
		surf.PolyColor.s.green = FADEGREENFACTOR;
		surf.PolyColor.s.blue = FADEBLUEFACTOR;

		surf.PolyColor.s.alpha = (wipestyleflags & WSF_FADEIN) ? 255 : 0;

		HWD.pfnSetShader(HWR_GetShaderFromTarget(SHADER_UI_TINTED_WIPE));
		HWD.pfnDoScreenWipe(HWD_SCREENTEXTURE_WIPE_START, HWD_SCREENTEXTURE_WIPE_END,
			&surf, polyflags);
		HWD.pfnUnSetShader();
	}
	else
	{
		HWD.pfnDoScreenWipe(HWD_SCREENTEXTURE_WIPE_START, HWD_SCREENTEXTURE_WIPE_END,
			NULL, 0);
	}
}

void HWR_MakeScreenFinalTexture(void)
{
	int tex = HWR_ShouldUsePaletteRendering() ? HWD_SCREENTEXTURE_GENERIC3 : HWD_SCREENTEXTURE_GENERIC2;
	HWD.pfnMakeScreenTexture(tex);
}

void HWR_DrawScreenFinalTexture(int width, int height)
{
	int tex = HWR_ShouldUsePaletteRendering() ? HWD_SCREENTEXTURE_GENERIC3 : HWD_SCREENTEXTURE_GENERIC2;
	HWD.pfnDrawScreenFinalTexture(tex, width, height);
}

#endif  
