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



#include "r_portal.h"
#include "r_plane.h"
#include "r_main.h"
#include "doomstat.h"
#include "p_spec.h"  
#include "p_slopes.h"  
#include "p_local.h"
#include "z_zone.h"
#include "r_things.h"
#include "r_sky.h"

UINT8 portalrender;			 


portal_t *portal_base, *portal_cap;

line_t *portalclipline;
sector_t *portalcullsector;
INT32 portalclipstart, portalclipend;

boolean portalline;  

void Portal_InitList (void)
{
	portalrender = 0;
	portal_base = portal_cap = NULL;
}

                                                              
  
                                                           
                                                                   
                      
   
void Portal_ClipRange (portal_t* portal)
{
	INT32 start	= portal->start;
	INT32 end	= portal->end;
	size_t columns;

	if (end <= start)
		return;
	columns = (size_t)(end - start);

	memcpy(portal->ceilingclip, &ceilingclip[start], columns * sizeof(*ceilingclip));
	memcpy(portal->floorclip, &floorclip[start], columns * sizeof(*floorclip));
	memcpy(portal->frontscale, &frontscale[start], columns * sizeof(*frontscale));
}

                                            
   
void Portal_ClipApply (const portal_t* portal)
{
	INT32 i;
	INT32 start	= portal->start;
	INT32 end	= portal->end;
	size_t columns;

	if (end <= start)
		return;
	columns = (size_t)(end - start);

	memcpy(&ceilingclip[start], portal->ceilingclip, columns * sizeof(*ceilingclip));
	memcpy(&floorclip[start], portal->floorclip, columns * sizeof(*floorclip));
	memcpy(&frontscale[start], portal->frontscale, columns * sizeof(*frontscale));


	for (i = 0; i < start; i++)
	{
		floorclip[i] = -1;
		ceilingclip[i] = (INT16)viewheight;
	}
	for (i = end; i < vid.width; i++)
	{
		floorclip[i] = -1;
		ceilingclip[i] = (INT16)viewheight;
	}
}

static portal_t* Portal_Add (const INT16 x1, const INT16 x2)
{
	portal_t *portal		= Z_Calloc(sizeof(portal_t), PU_LEVEL, NULL);
	INT16 *ceilingclipsave	= Z_Malloc(sizeof(INT16)*(x2-x1 + 1), PU_LEVEL, NULL);
	INT16 *floorclipsave	= Z_Malloc(sizeof(INT16)*(x2-x1 + 1), PU_LEVEL, NULL);
	fixed_t *frontscalesave	= Z_Malloc(sizeof(fixed_t)*(x2-x1 + 1), PU_LEVEL, NULL);


	if (!portal_base)
	{
		portal_base	= portal;
		portal_cap	= portal;
	}
	else
	{
		portal_cap->next = portal;
		portal_cap = portal;
	}
	portal->clipline = -1;


	portal->ceilingclip	= ceilingclipsave;
	portal->floorclip	= floorclipsave;
	portal->frontscale	= frontscalesave;
	portal->start	= x1;
	portal->end		= x2;


	portal->pass = portalrender+1;

	return portal;
}

void Portal_Remove (portal_t* portal)
{
	portalcullsector = NULL;
	portal_base = portal->next;
	Z_Free(portal->ceilingclip);
	Z_Free(portal->floorclip);
	Z_Free(portal->frontscale);
	Z_Free(portal);
}

static void Portal_GetViewpointForLine(portal_t *portal, line_t *start, line_t *dest, angle_t dangle)
{

	fixed_t disttopoint;
	angle_t angtopoint;

	struct {
		fixed_t x, y;
	} dest_c, start_c;


	                                                                        
                                                                            
                                                                            
                                          
	start_c.x = (fixed_t)(((INT64)start->v1->x + start->v2->x) / 2);
	start_c.y = (fixed_t)(((INT64)start->v1->y + start->v2->y) / 2);


	dest_c.x = (fixed_t)(((INT64)dest->v1->x + dest->v2->x) / 2);
	dest_c.y = (fixed_t)(((INT64)dest->v1->y + dest->v2->y) / 2);

	disttopoint = R_PointToDist2(start_c.x, start_c.y, viewx, viewy);
	angtopoint = R_PointToAngle2(start_c.x, start_c.y, viewx, viewy);
	angtopoint += dangle;

	portal->viewx = dest_c.x + FixedMul(FINECOSINE(angtopoint>>ANGLETOFINESHIFT), disttopoint);
	portal->viewy = dest_c.y + FixedMul(FINESINE(angtopoint>>ANGLETOFINESHIFT), disttopoint);
	portal->viewangle = viewangle + dangle;
}

                                                                    
  
                                                     
                                                  

                                                            
                                                         
                                                          
                                       
   
void Portal_Add2Lines (const INT32 line1, const INT32 line2, const INT32 x1, const INT32 x2)
{
	portal_t* portal = Portal_Add(x1, x2);
	line_t* start	= &lines[line1];
	line_t* dest	= &lines[line2];

	angle_t dangle = R_PointToAngle2(0,0,dest->dx,dest->dy) - R_PointToAngle2(start->dx,start->dy,0,0);

	Portal_GetViewpointForLine(portal, start, dest, dangle);

	portal->viewz = viewz + dest->frontsector->floorheight - start->frontsector->floorheight;

	portal->clipline = line2;

	Portal_ClipRange(portal);

	portalline = true;  
}

                                                            
  
                                                               
                                         
   
static void Portal_ClipVisplane (const visplane_t* plane, portal_t* portal)
{
	INT16 start	= portal->start;
	INT16 end	= portal->end;
	INT32 i;

	for (i = 0; i < end - start; i++)
	{

		if (plane->top[i + start] == 65535)
		{
			portal->ceilingclip[i] = -1;
			portal->floorclip[i] = -1;
			continue;
		}
		portal->ceilingclip[i] = plane->top[i + start] - 1;
		portal->floorclip[i] = plane->bottom[i + start] + 1;
		portal->frontscale[i] = INT32_MAX;
	}
}

extern INT32 viewwidth;

static boolean TrimVisplaneBounds (const visplane_t* plane, INT16* start, INT16* end)
{
	*start = plane->minx;
	*end = plane->maxx + 1;



	if (*end > viewwidth)
		*end = viewwidth;

	if (!(*start < *end))
		return true;


	                                                               
   
                                                              
                                                            
               
    

	while (*start < *end && plane->bottom[*start] == 0 && plane->top[*start] == 65535)
	{
		(*start)++;
	}


	                                                                         
                                                                            
                                                                          
                                                                          
	while (*start < *end && plane->bottom[*end - 1] == 0
	&& plane->top[*end - 1] == 65535)
	{
		(*end)--;
	}

	if (!(*start < *end))
		return true;

	return false;
}

static void Portal_GetViewpointForSkybox(portal_t *portal)
{
	portal->viewx = skyboxmo[0]->x;
	portal->viewy = skyboxmo[0]->y;
	portal->viewz = skyboxmo[0]->z;
	portal->viewangle = viewangle + skyboxmo[0]->angle;

	mapheader_t *mh = mapheaderinfo[gamemap-1];


	if (skyboxmo[1])
	{
		fixed_t x = 0, y = 0;
		angle_t ang = skyboxmo[0]->angle>>ANGLETOFINESHIFT;

		if (mh->skybox_scalex > 0)
			x = (viewx - skyboxmo[1]->x) / mh->skybox_scalex;
		else if (mh->skybox_scalex < 0)
			x = (viewx - skyboxmo[1]->x) * -mh->skybox_scalex;

		if (mh->skybox_scaley > 0)
			y = (viewy - skyboxmo[1]->y) / mh->skybox_scaley;
		else if (mh->skybox_scaley < 0)
			y = (viewy - skyboxmo[1]->y) * -mh->skybox_scaley;


		portal->viewx += FixedMul(x,FINECOSINE(ang)) - FixedMul(y,  FINESINE(ang));
		portal->viewy += FixedMul(x,  FINESINE(ang)) + FixedMul(y,FINECOSINE(ang));
	}

	if (mh->skybox_scalez > 0)
		portal->viewz += viewz / mh->skybox_scalez;
	else if (mh->skybox_scalez < 0)
		portal->viewz += viewz * -mh->skybox_scalez;
}

                                              
  
                                                     
                                  
   
static boolean Portal_AddSkybox (const visplane_t* plane)
{
	INT16 start, end;
	portal_t* portal;

	if (TrimVisplaneBounds(plane, &start, &end))
		return false;

	portal = Portal_Add(start, end);

	Portal_ClipVisplane(plane, portal);

	portal->is_skybox = true;

	Portal_GetViewpointForSkybox(portal);

	return true;
}

static void Portal_GetViewpointForSecPortal(portal_t *portal, sectorportal_t *secportal)
{
	fixed_t x, y, z;
	angle_t angle;

	sector_t *target = secportal->target;

	fixed_t target_x = target->soundorg.x;
	fixed_t target_y = target->soundorg.y;
	fixed_t target_z;

	if (secportal->ceiling)
		target_z = P_GetSectorCeilingZAt(target, target_x, target_y);
	else
		target_z = P_GetSectorFloorZAt(target, target_x, target_y);

	switch (secportal->type)
	{
	case SECPORTAL_LINE:
		angle = secportal->line.dest->angle - secportal->line.start->angle;
		Portal_GetViewpointForLine(portal, secportal->line.start, secportal->line.dest, angle);
		portal->viewz = viewz;  
		return;
	case SECPORTAL_OBJECT:
		if (P_MobjWasRemoved(secportal->mobj))
			return;
		portal->viewmobj = secportal->mobj;
		x = secportal->mobj->x;
		y = secportal->mobj->y;
		z = secportal->mobj->z;
		angle = secportal->mobj->angle;
		break;
	case SECPORTAL_FLOOR:
		x = secportal->sector->soundorg.x;
		y = secportal->sector->soundorg.y;
		z = P_GetSectorFloorZAt(secportal->sector, x, y);
		angle = 0;
		break;
	case SECPORTAL_CEILING:
		x = secportal->sector->soundorg.x;
		y = secportal->sector->soundorg.y;
		z = P_GetSectorCeilingZAt(secportal->sector, x, y);
		angle = 0;
		break;
	case SECPORTAL_PLANE:
	case SECPORTAL_HORIZON:
		portal->is_horizon = true;
		portal->horizon_sector = secportal->sector;
		x = secportal->sector->soundorg.x;
		y = secportal->sector->soundorg.y;
		if (secportal->type == SECPORTAL_PLANE)
			z = -viewz;
		else
			z = 0;
		angle = 0;
		break;
	default:
		return;
	}

	fixed_t refx = target_x - viewx;
	fixed_t refy = target_y - viewy;
	fixed_t refz = target_z - viewz;


	if (angle != 0)
	{
		fixed_t tr_x = refx, tr_y = refy;
		angle_t ang = angle >> ANGLETOFINESHIFT;
		refx = FixedMul(tr_x, FINECOSINE(ang)) - FixedMul(tr_y, FINESINE(ang));
		refy = FixedMul(tr_x, FINESINE(ang)) + FixedMul(tr_y, FINECOSINE(ang));
	}

	portal->viewx = x - refx;
	portal->viewy = y - refy;
	portal->viewz = z - refz;
	portal->viewangle = angle + viewangle;
}

                                              
   
static boolean Portal_AddSectorPortal (const visplane_t* plane)
{
	INT16 start, end;
	sectorportal_t *secportal = plane->portalsector;


	if (secportal->type == SECPORTAL_SKYBOX)
	{
		if (R_SkyboxEnabled() && skyboxmo[0])
			return Portal_AddSkybox(plane);
		return false;
	}

	if (TrimVisplaneBounds(plane, &start, &end))
		return false;

	portal_t* portal = Portal_Add(start, end);

	Portal_ClipVisplane(plane, portal);

	Portal_GetViewpointForSecPortal(portal, secportal);

	return true;
}

                                        
   
void Portal_AddTransferred (const UINT32 secportalnum, const INT32 x1, const INT32 x2)
{
	if (secportalnum >= secportalcount)
		return;

	sectorportal_t *secportal = &secportals[secportalnum];
	if (!P_IsSectorPortalValid(secportal))
		return;

	portal_t* portal = Portal_Add(x1, x2);

	if (secportal->type == SECPORTAL_SKYBOX)
		Portal_GetViewpointForSkybox(portal);
	else
		Portal_GetViewpointForSecPortal(portal, secportal);

	if (secportal->type == SECPORTAL_LINE)
		portal->clipline = secportal->line.dest - lines;
	else
		portal->clipline = -1;

	Portal_ClipRange(portal);

	portalline = true;
}

                                                                
                                                            
   
void Portal_AddPlanePortals (boolean add_skyboxes)
{
	visplane_t *pl;

	for (INT32 i = 0; i < MAXVISPLANES; i++)
	{
		for (pl = visplanes[i]; pl; pl = pl->next)
		{
			 
			if (pl->minx >= pl->maxx)
				continue;

			boolean added_portal = false;


			if (pl->portalsector && portalrender < R_PortalDepthLimit())
				added_portal = Portal_AddSectorPortal(pl);


			if (!added_portal && pl->picnum == skyflatnum && add_skyboxes && skyboxmo[0])
				added_portal = Portal_AddSkybox(pl);


			if (added_portal)
			{
				pl->minx = 0;
				pl->maxx = -1;
			}
		}
	}
}
