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
#include "g_game.h"
#include "g_input.h"
#include "r_local.h"
#include "r_splats.h"  
#include "r_sky.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "p_local.h"
#include "keys.h"
#include "i_video.h"
#include "m_menu.h"
#include "am_map.h"
#include "d_main.h"
#include "v_video.h"
#include "p_spec.h"  
#include "p_setup.h"
#include "z_zone.h"
#include "m_random.h"  
#include "r_portal.h"
#include "r_main.h"
#include "i_system.h"  
#include "r_fps.h"  
#include "r_patchrotation.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif


#define FIELDOFVIEW 2048


size_t validcount = 1;

INT32 centerx, centery;

fixed_t centerxfrac, centeryfrac;
fixed_t projection;
fixed_t projectiony;  
fixed_t fovtan;  


size_t framecount;

size_t loopcount;

fixed_t viewx, viewy, viewz;
angle_t viewangle, aimingangle;
fixed_t viewcos, viewsin;
sector_t *viewsector;
player_t *viewplayer;
mobj_t *r_viewmobj;

boolean r_renderwalls;
boolean r_renderfloors;
boolean r_renderthings;

fixed_t rendertimefrac;
fixed_t renderdeltatics;
boolean renderisnewtic;




angle_t clipangle;
angle_t doubleclipangle;





INT32 viewangletox[FINEANGLES/2];




angle_t xtoviewangle[MAXVIDWIDTH+1];

lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t *scalelightfixed[MAXLIGHTSCALE];
lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];


extracolormap_t *extra_colormaps = NULL;


precise_t ps_prevframetime = 0;
ps_metric_t ps_rendercalltime = {0};
ps_metric_t ps_otherrendertime = {0};
ps_metric_t ps_uitime = {0};
ps_metric_t ps_swaptime = {0};

ps_metric_t ps_bsptime = {0};

ps_metric_t ps_sw_spritecliptime = {0};
ps_metric_t ps_sw_portaltime = {0};
ps_metric_t ps_sw_planetime = {0};
ps_metric_t ps_sw_maskedtime = {0};

ps_metric_t ps_numbspcalls = {0};
ps_metric_t ps_numsprites = {0};
ps_metric_t ps_numdrawnodes = {0};
ps_metric_t ps_numpolyobjects = {0};

static CV_PossibleValue_t drawdist_cons_t[] = {
	{256, "256"},	{512, "512"},	{768, "768"},
	{1024, "1024"},	{1536, "1536"},	{2048, "2048"},
	{3072, "3072"},	{4096, "4096"},	{6144, "6144"},
	{8192, "8192"},	{0, "Infinite"},	{0, NULL}};



static CV_PossibleValue_t drawdist_precip_cons_t[] = {
	{256, "256"},	{512, "512"},	{768, "768"},
	{1024, "1024"},	{1536, "1536"},	{2048, "2048"},
	{0, "None"},	{0, NULL}};

static CV_PossibleValue_t fov_cons_t[] = {{MINFOV*FRACUNIT, "MIN"}, {MAXFOV*FRACUNIT, "MAX"}, {0, NULL}};
static CV_PossibleValue_t translucenthud_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
static CV_PossibleValue_t maxportals_cons_t[] = {
	{1, "1"}, {2, "2"}, {0, NULL}};
static CV_PossibleValue_t homremoval_cons_t[] = {{0, "No"}, {1, "Yes"}, {2, "Flash"}, {0, NULL}};
static void MSAA_OnChange(void);

static void R_SetFov(fixed_t playerfov);

static void Fov_OnChange(void);
static void ChaseCam_OnChange(void);
static void ChaseCam2_OnChange(void);
static void FlipCam_OnChange(void);
static void FlipCam2_OnChange(void);
void SendWeaponPref(void);
void SendWeaponPref2(void);

consvar_t cv_tailspickup = CVAR_INIT ("tailspickup", "On", CV_NETVAR|CV_ALLOWLUA, CV_OnOff, NULL);
consvar_t cv_chasecam = CVAR_INIT ("chasecam", "On", CV_CALL, CV_OnOff, ChaseCam_OnChange);
consvar_t cv_chasecam2 = CVAR_INIT ("chasecam2", "On", CV_CALL, CV_OnOff, ChaseCam2_OnChange);
consvar_t cv_flipcam = CVAR_INIT ("flipcam", "No", CV_SAVE|CV_CALL|CV_NOINIT, CV_YesNo, FlipCam_OnChange);
consvar_t cv_flipcam2 = CVAR_INIT ("flipcam2", "No", CV_SAVE|CV_CALL|CV_NOINIT, CV_YesNo, FlipCam2_OnChange);

consvar_t cv_shadow = CVAR_INIT ("shadow", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_glmsaa = CVAR_INIT ("gl_msaa", "On", CV_SAVE|CV_CALL, CV_OnOff, MSAA_OnChange);
consvar_t cv_skybox = CVAR_INIT ("skybox", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_allowmlook = CVAR_INIT ("allowmlook", "Yes", CV_NETVAR|CV_ALLOWLUA, CV_YesNo, NULL);
consvar_t cv_showhud = CVAR_INIT ("showhud", "Yes", CV_CALL|CV_ALLOWLUA,  CV_YesNo, R_SetViewSize);
consvar_t cv_translucenthud = CVAR_INIT ("translucenthud", "10", CV_SAVE, translucenthud_cons_t, NULL);

consvar_t cv_translucency = CVAR_INIT ("translucency", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_drawdist = CVAR_INIT ("drawdist", "Infinite", CV_SAVE, drawdist_cons_t, NULL);
consvar_t cv_drawdist_nights = CVAR_INIT ("drawdist_nights", "2048", CV_SAVE, drawdist_cons_t, NULL);
consvar_t cv_drawdist_precip = CVAR_INIT ("drawdist_precip", "1024", CV_SAVE, drawdist_precip_cons_t, NULL);
consvar_t cv_fov = CVAR_INIT ("fov", "90", CV_SAVE|CV_FLOAT|CV_CALL, fov_cons_t, Fov_OnChange);
consvar_t cv_fovchange = CVAR_INIT ("fovchange", "Off", CV_SAVE, CV_OnOff, NULL);

static void MSAA_OnChange(void)
{
#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_SetMultisample(cv_glmsaa.value != 0);
#endif
}

boolean R_SkyboxEnabled(void)
{
#ifdef HWRENDER
	                                                                         
                                                                            
                                                                           
                                                                          
                                   
	return cv_skybox.value || cv_glskydome.value;
#else
	return cv_skybox.value;
#endif
}
consvar_t cv_maxportals = CVAR_INIT ("maxportals", "2", CV_SAVE, maxportals_cons_t, NULL);

consvar_t cv_renderview = CVAR_INIT ("renderview", "On", 0, CV_OnOff, NULL);
consvar_t cv_renderwalls = CVAR_INIT ("r_renderwalls", "On", 0, CV_OnOff, NULL);
consvar_t cv_renderfloors = CVAR_INIT ("r_renderfloors", "On", 0, CV_OnOff, NULL);
consvar_t cv_renderthings = CVAR_INIT ("r_renderthings", "On", 0, CV_OnOff, NULL);
consvar_t cv_ffloorclip = CVAR_INIT ("r_ffloorclip", "On", 0, CV_OnOff, NULL);
consvar_t cv_spriteclip = CVAR_INIT ("r_spriteclip", "On", 0, CV_OnOff, NULL);

consvar_t cv_homremoval = CVAR_INIT ("homremoval", "No", CV_SAVE, homremoval_cons_t, NULL);

consvar_t cv_renderstats = CVAR_INIT ("renderstats", "Off", 0, CV_OnOff, NULL);

void SplitScreen_OnChange(void)
{
	if (!cv_debug && netgame)
	{
		if (splitscreen)
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Splitscreen not supported in netplay, sorry!\n"));
			splitscreen = false;
		}
		return;
	}


	R_ExecuteSetViewSize();

	if (!demoplayback && !botingame)
	{
		if (splitscreen)
			CL_AddSplitscreenPlayer();
		else
			CL_RemoveSplitscreenPlayer();

		if (server && !netgame)
			multiplayer = splitscreen;
	}
	else
	{
		INT32 i;
		secondarydisplayplayer = consoleplayer;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && i != consoleplayer)
			{
				secondarydisplayplayer = i;
				break;
			}
	}
}
static void Fov_OnChange(void)
{
	R_SetViewSize();
}

static void ChaseCam_OnChange(void)
{
	if (!cv_chasecam.value || !cv_useranalog[0].value)
		CV_SetValue(&cv_analog[0], 0);
	else
		CV_SetValue(&cv_analog[0], 1);
}

static void ChaseCam2_OnChange(void)
{
	if (botingame)
		return;
	if (!cv_chasecam2.value || !cv_useranalog[1].value)
		CV_SetValue(&cv_analog[1], 0);
	else
		CV_SetValue(&cv_analog[1], 1);
}

static void FlipCam_OnChange(void)
{
	SendWeaponPref();
}

static void FlipCam2_OnChange(void)
{
	SendWeaponPref2();
}









INT32 R_PointOnSide(fixed_t x, fixed_t y, node_t *restrict node)
{
	if (!node->dx)
		return x <= node->x ? node->dy > 0 : node->dy < 0;

	if (!node->dy)
		return y <= node->y ? node->dx < 0 : node->dx > 0;

	fixed_t dx = (x >> 1) - (node->x >> 1);
	fixed_t dy = (y >> 1) - (node->y >> 1);



	INT32 mask = (node->dy ^ node->dx ^ dx ^ dy) >> 31;
	return (mask & ((node->dy ^ dx) < 0)) |   
		(~mask & (FixedMul(dy, node->dx>>FRACBITS) >= FixedMul(node->dy>>FRACBITS, dx)));
}


INT32 R_PointOnSegSide(fixed_t x, fixed_t y, seg_t *line)
{
	fixed_t lx = line->v1->x;
	fixed_t ly = line->v1->y;
	fixed_t ldx = line->v2->x - lx;
	fixed_t ldy = line->v2->y - ly;

	if (!ldx)
		return x <= lx ? ldy > 0 : ldy < 0;

	if (!ldy)
		return y <= ly ? ldx < 0 : ldx > 0;

	fixed_t dx = (x >> 1) - (lx >> 1);
	fixed_t dy = (y >> 1) - (ly >> 1);


	if ((ldy ^ ldx ^ dx ^ dy) < 0)
		return (ldy ^ dx) < 0;           
	return FixedMul(dy, ldx>>FRACBITS) >= FixedMul(ldy>>FRACBITS, dx);
}














angle_t R_PointToAngle(fixed_t x, fixed_t y)
{
	return (y -= viewy, (x -= viewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDiv(y,x)] :                           
		ANGLE_90-tantoangle[SlopeDiv(x,y)] :                            
		x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                    
		ANGLE_270+tantoangle[SlopeDiv(x,y)] :                           
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDiv(y,x)] :   
		ANGLE_90 + tantoangle[SlopeDiv(x,y)] :                          
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDiv(y,x)] :     
		ANGLE_270-tantoangle[SlopeDiv(x,y)] :                           
		0;
}


angle_t R_PointToAngle64(INT64 x, INT64 y)
{
	return (y -= viewy, (x -= viewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDivEx(y,x)] :                             
		ANGLE_90-tantoangle[SlopeDivEx(x,y)] :                                
		x > (y = -y) ? 0-tantoangle[SlopeDivEx(y,x)] :                     
		ANGLE_270+tantoangle[SlopeDivEx(x,y)] :                               
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDivEx(y,x)] :   
		ANGLE_90 + tantoangle[SlopeDivEx(x,y)] :                              
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDivEx(y,x)] :     
		ANGLE_270-tantoangle[SlopeDivEx(x,y)] :                               
		0;
}

angle_t R_PointToAngle2(fixed_t pviewx, fixed_t pviewy, fixed_t x, fixed_t y)
{
	return (y -= pviewy, (x -= pviewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDiv(y,x)] :                           
		ANGLE_90-tantoangle[SlopeDiv(x,y)] :                            
		x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                    
		ANGLE_270+tantoangle[SlopeDiv(x,y)] :                           
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDiv(y,x)] :   
		ANGLE_90 + tantoangle[SlopeDiv(x,y)] :                          
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDiv(y,x)] :     
		ANGLE_270-tantoangle[SlopeDiv(x,y)] :                           
		0;
}

fixed_t R_PointToDist2(fixed_t px2, fixed_t py2, fixed_t px1, fixed_t py1)
{
	angle_t angle;
	ufixed_t dx, dy, dist;

	dx = abs(px1 - px2);
	dy = abs(py1 - py2);

	if (dy > dx)
	{
		fixed_t temp;

		temp = dx;
		dx = dy;
		dy = temp;
	}
	if (!dy)
		return dx;

	angle = (tantoangle[FixedDiv(dy, dx)>>DBITS] + ANGLE_90) >> ANGLETOFINESHIFT;


	dist = FixedDiv(dx, FINESINE(angle));

	return dist;
}


fixed_t R_PointToDist(fixed_t x, fixed_t y)
{
	return R_PointToDist2(viewx, viewy, x, y);
}

line_t *R_GetFFloorLine(const line_t *line, const ffloor_t *pfloor, const sector_t *sector)
{
	if (pfloor->master->flags & ML_TFERLINE)
	{
		size_t linenum = min((size_t)(line - sector->lines[0]), pfloor->master->frontsector->linecount);
		return pfloor->master->frontsector->lines[0] + linenum;
	}
	else
		return pfloor->master;
}

side_t *R_GetFFloorSide(const line_t *line, const ffloor_t *pfloor, const sector_t *sector)
{
	if (pfloor->master->flags & ML_TFERLINE)
	{
		line_t *newline = R_GetFFloorLine(line, pfloor, sector);
		return &sides[newline->sidenum[0]];
	}
	else
		return &sides[pfloor->master->sidenum[0]];
}






boolean R_DoCulling(line_t *cullheight, line_t *viewcullheight, fixed_t vz, fixed_t bottomh, fixed_t toph)
{
	fixed_t cullplane;

	if (!cullheight)
		return false;

	cullplane = cullheight->frontsector->floorheight;
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




static void R_InitTextureMapping(void)
{
	INT32 i;
	INT32 x;
	INT32 t;
	fixed_t focallength;







	focallength = FixedDiv(projection,
		FINETANGENT(FINEANGLES/4+FIELDOFVIEW/2));

	focallengthf = FIXED_TO_FLOAT(focallength);

	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (FINETANGENT(i) > fovtan*2)
			t = -1;
		else if (FINETANGENT(i) < -fovtan*2)
			t = viewwidth+1;
		else
		{
			t = FixedMul(FINETANGENT(i), focallength);
			t = (centerxfrac - t+FRACUNIT-1)>>FRACBITS;

			if (t < -1)
				t = -1;
			else if (t > viewwidth+1)
				t = viewwidth+1;
		}
		viewangletox[i] = t;
	}





	for (x = 0; x <= viewwidth;x++)
	{
		i = 0;
		while (viewangletox[i] > x)
			i++;
		xtoviewangle[x] = (i<<ANGLETOFINESHIFT) - ANGLE_90;
	}


	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (viewangletox[i] == -1)
			viewangletox[i] = 0;
		else if (viewangletox[i] == viewwidth+1)
			viewangletox[i]  = viewwidth;
	}

	clipangle = xtoviewangle[0];
	doubleclipangle = clipangle*2;
}








#define DISTMAP 2

static inline void R_InitLightTables(void)
{
	INT32 i;
	INT32 j;
	INT32 level;
	INT32 startmapl;
	INT32 scale;



	for (i = 0; i < LIGHTLEVELS; i++)
	{
		startmapl = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTZ; j++)
		{


			scale = FixedDiv((BASEVIDWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
			scale >>= LIGHTSCALESHIFT;
			level = startmapl - scale/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS-1;

			zlight[i][j] = colormaps + level*256;
		}
	}
}




static struct {
	angle_t rollangle;  
#ifdef WOUGHMP_WOUGHMP
	fixed_t fisheye;
#endif

	fixed_t zoomneeded;
	INT32 *scrmap;
	INT32 scrmapsize;

	INT32 x1;  
	INT16 ceilingclip[MAXVIDWIDTH], floorclip[MAXVIDWIDTH];

	boolean use;
} viewmorph = {
	0,
#ifdef WOUGHMP_WOUGHMP
	0,
#endif

	FRACUNIT,
	NULL,
	0,

	0,
	{0}, {0},

	false
};

void R_CheckViewMorph(void)
{
	float zoomfactor, rollcos, rollsin;
	float x1, y1, x2, y2;
	fixed_t temp;
	INT32 end, vx, vy, pos, usedpos;
	INT32 usedx, usedy, halfwidth = vid.width/2, halfheight = vid.height/2;
#ifdef WOUGHMP_WOUGHMP
	float fisheyemap[MAXVIDWIDTH/2 + 1];
#endif

	angle_t rollangle = players[displayplayer].viewrollangle;
#ifdef WOUGHMP_WOUGHMP
	fixed_t fisheye = cv_cam2_turnmultiplier.value;  
#endif

	rollangle >>= ANGLETOFINESHIFT;
	rollangle = ((rollangle+2) & ~3) & FINEMASK;  

#ifdef WOUGHMP_WOUGHMP
	fisheye &= ~0x7FF;  
#endif

	if (rollangle == viewmorph.rollangle &&
#ifdef WOUGHMP_WOUGHMP
		fisheye == viewmorph.fisheye &&
#endif
		viewmorph.scrmapsize == vid.width*vid.height)
		return;  

	viewmorph.rollangle = rollangle;
#ifdef WOUGHMP_WOUGHMP
	viewmorph.fisheye = fisheye;
#endif

	if (viewmorph.rollangle == 0
#ifdef WOUGHMP_WOUGHMP
		 && viewmorph.fisheye == 0
#endif
	 )
	{
		viewmorph.use = false;
		viewmorph.x1 = 0;
		if (viewmorph.zoomneeded != FRACUNIT)
			R_SetViewSize();
		viewmorph.zoomneeded = FRACUNIT;

		return;
	}

	if (viewmorph.scrmapsize != vid.width*vid.height)
	{
		if (viewmorph.scrmap)
			free(viewmorph.scrmap);
		viewmorph.scrmap = malloc(vid.width*vid.height * sizeof(INT32));
		viewmorph.scrmapsize = vid.width*vid.height;
	}

	temp = FINECOSINE(rollangle);
	rollcos = FIXED_TO_FLOAT(temp);
	temp = FINESINE(rollangle);
	rollsin = FIXED_TO_FLOAT(temp);


	x1 = (vid.width*fabsf(rollcos) + vid.height*fabsf(rollsin)) / vid.width;
	y1 = (vid.height*fabsf(rollcos) + vid.width*fabsf(rollsin)) / vid.height;

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
	{
		float f = FIXED_TO_FLOAT(fisheye);
		for (vx = 0; vx <= halfwidth; vx++)
			fisheyemap[vx] = 1.0f / cos(atan(vx * f / halfwidth));

		f = cos(atan(f));
		if (f < 1.0f)
		{
			x1 /= f;
			y1 /= f;
		}
	}
#endif

	temp = max(x1, y1)*FRACUNIT;
	if (temp < FRACUNIT)
		temp = FRACUNIT;
	else
		temp |= 0x3FFF;  



	if (temp != viewmorph.zoomneeded)
	{
		viewmorph.zoomneeded = temp;
		R_SetViewSize();
	}

	zoomfactor = FIXED_TO_FLOAT(viewmorph.zoomneeded);

	end = vid.width * vid.height - 1;

	pos = 0;


	rollcos /= zoomfactor;
	rollsin /= zoomfactor;

	x1 = -(halfwidth * rollcos - halfheight * rollsin);
	y1 = -(halfheight * rollcos + halfwidth * rollsin);

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
		viewmorph.x1 = (INT32)(halfwidth - (halfwidth * fabsf(rollcos) + halfheight * fabsf(rollsin)) * fisheyemap[halfwidth]);
	else
#endif
	viewmorph.x1 = (INT32)(halfwidth - (halfwidth * fabsf(rollcos) + halfheight * fabsf(rollsin)));



	for (vx = 0; vx < vid.width; vx++)
	{
		viewmorph.ceilingclip[vx] = vid.height;
		viewmorph.floorclip[vx] = -1;
	}
	x2 = x1;
	y2 = y1;
	for (vx = 0; vx < vid.width; vx++)
	{
		INT16 xa, ya, xb, yb;
		xa = x2+halfwidth;
		ya = y2+halfheight-1;
		xb = vid.width-1-xa;
		yb = vid.height-1-ya;

		viewmorph.ceilingclip[xa] = min(viewmorph.ceilingclip[xa], ya);
		viewmorph.floorclip[xa] = max(viewmorph.floorclip[xa], ya);
		viewmorph.ceilingclip[xb] = min(viewmorph.ceilingclip[xb], yb);
		viewmorph.floorclip[xb] = max(viewmorph.floorclip[xb], yb);
		x2 += rollcos;
		y2 += rollsin;
	}
	x2 = x1;
	y2 = y1;
	for (vy = 0; vy < vid.height; vy++)
	{
		INT16 xa, ya, xb, yb;
		xa = x2+halfwidth;
		ya = y2+halfheight;
		xb = vid.width-1-xa;
		yb = vid.height-1-ya;

		viewmorph.ceilingclip[xa] = min(viewmorph.ceilingclip[xa], ya);
		viewmorph.floorclip[xa] = max(viewmorph.floorclip[xa], ya);
		viewmorph.ceilingclip[xb] = min(viewmorph.ceilingclip[xb], yb);
		viewmorph.floorclip[xb] = max(viewmorph.floorclip[xb], yb);
		x2 -= rollsin;
		y2 += rollcos;
	}



#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
	{
		for (vy = 0; vy < halfheight; vy++)
		{
			x2 = x1;
			y2 = y1;
			x1 -= rollsin;
			y1 += rollcos;

			for (vx = 0; vx < vid.width; vx++)
			{
				usedx = halfwidth + x2*fisheyemap[(int) floorf(fabsf(y2*zoomfactor))];
				usedy = halfheight + y2*fisheyemap[(int) floorf(fabsf(x2*zoomfactor))];

				usedpos = usedx + usedy*vid.width;

				viewmorph.scrmap[pos] = usedpos;
				viewmorph.scrmap[end-pos] = end-usedpos;

				x2 += rollcos;
				y2 += rollsin;
				pos++;
			}
		}
	}
	else
	{
#endif
	x1 += halfwidth;
	y1 += halfheight;

	for (vy = 0; vy < halfheight; vy++)
	{
		x2 = x1;
		y2 = y1;
		x1 -= rollsin;
		y1 += rollcos;

		for (vx = 0; vx < vid.width; vx++)
		{
			usedx = x2;
			usedy = y2;

			usedpos = usedx + usedy*vid.width;

			viewmorph.scrmap[pos] = usedpos;
			viewmorph.scrmap[end-pos] = end-usedpos;

			x2 += rollcos;
			y2 += rollsin;
			pos++;
		}
	}
#ifdef WOUGHMP_WOUGHMP
	}
#endif

	viewmorph.use = true;
}

void R_ApplyViewMorph(void)
{
	UINT8 *tmpscr = screens[4];
	UINT8 *srcscr = screens[0];
	INT32 p, end = vid.width * vid.height;

	if (!viewmorph.use)
		return;

	if (cv_debug & DBG_VIEWMORPH)
	{
		UINT8 border = 32;
		UINT8 grid = 160;
		INT32 ws = vid.width / 4;
		INT32 hs = vid.width * (vid.height / 4);

		memcpy(tmpscr, srcscr, vid.width*vid.height);
		for (p = 0; p < vid.width; p++)
		{
			tmpscr[viewmorph.scrmap[p]] = border;
			tmpscr[viewmorph.scrmap[p + hs]] = grid;
			tmpscr[viewmorph.scrmap[p + hs*2]] = grid;
			tmpscr[viewmorph.scrmap[p + hs*3]] = grid;
			tmpscr[viewmorph.scrmap[end - 1 - p]] = border;
		}
		for (p = vid.width; p < end; p += vid.width)
		{
			tmpscr[viewmorph.scrmap[p]] = border;
			tmpscr[viewmorph.scrmap[p + ws]] = grid;
			tmpscr[viewmorph.scrmap[p + ws*2]] = grid;
			tmpscr[viewmorph.scrmap[p + ws*3]] = grid;
			tmpscr[viewmorph.scrmap[end - 1 - p]] = border;
		}
	}
	else
		for (p = 0; p < end; p++)
			tmpscr[p] = srcscr[viewmorph.scrmap[p]];

	VID_BlitLinearScreen(tmpscr, screens[0],
			vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.width);
}








boolean setsizeneeded;

void R_SetViewSize(void)
{
	setsizeneeded = true;
}




void R_ExecuteSetViewSize(void)
{
	INT32 i;
	INT32 j;
	INT32 level;
	INT32 startmapl;

	setsizeneeded = false;

	if (rendermode == render_none)
		return;


	st_overlay = cv_showhud.value;

	scaledviewwidth = vid.width;
	viewheight = vid.height;

	if (splitscreen)
		viewheight >>= 1;

	viewwidth = scaledviewwidth;

	centerx = viewwidth/2;
	centery = viewheight/2;
	centerxfrac = centerx<<FRACBITS;
	centeryfrac = centery<<FRACBITS;

	R_SetFov(cv_fov.value);

	R_InitViewBuffer(scaledviewwidth, viewheight);


	for (i = 0; i < viewwidth; i++)
		screenheightarray[i] = (INT16)viewheight;

	memset(scalelight, 0xFF, sizeof(scalelight));


	for (i = 0; i< LIGHTLEVELS; i++)
	{
		startmapl = ((LIGHTLEVELS - 1 - i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTSCALE; j++)
		{
			level = startmapl - j*vid.width/(viewwidth)/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS - 1;

			scalelight[i][j] = colormaps + level*256;
		}
	}


#ifdef HWRENDER
	if (rendermode != render_soft)
		HWR_SetViewSize();
#endif

	am_recalc = true;
}

fixed_t R_GetPlayerFov(player_t *player)
{
	fixed_t fov = cv_fov.value + player->fovadd;
	return max(MINFOV*FRACUNIT, min(fov, MAXFOV*FRACUNIT));
}

static void R_SetFov(fixed_t playerfov)
{
	angle_t fov = FixedAngle(playerfov/2) + ANGLE_90;
	fovtan = FixedMul(FINETANGENT(fov >> ANGLETOFINESHIFT), viewmorph.zoomneeded);
	if (splitscreen == 1)  
		fovtan = 17*fovtan/10;


	INT32 j = viewheight*16;
	for (INT32 i = 0; i < j; i++)
	{
		fixed_t dy = (i - viewheight*8)<<FRACBITS;
		dy = FixedMul(abs(dy), fovtan);
		yslopetab[i] = FixedDiv(centerx*FRACUNIT, dy);
	}

	projection = projectiony = FixedDiv(centerxfrac, fovtan);

	R_InitTextureMapping();


	R_SetSkyScale();
}





void R_Init(void)
{


	R_InitData();

	R_SetViewSize();  



	R_InitLightTables();


	R_InitTranslucencyTables();

	R_InitDrawNodes();

	framecount = 0;
}




boolean R_IsPointInSector(sector_t *sector, fixed_t x, fixed_t y)
{
	size_t i;
	size_t passes = 0;

	for (i = 0; i < sector->linecount; i++)
	{
		line_t *line = sector->lines[i];
		vertex_t *v1, *v2;

		if (line->frontsector == line->backsector)
			continue;

		v1 = line->v1;
		v2 = line->v2;


		if (v1->y > v2->y)
		{
			vertex_t *tmp = v1;
			v1 = v2;
			v2 = tmp;
		}
		else if (v1->y == v2->y)

			continue;

		if (v1->y < y && y <= v2->y)
		{

			fixed_t vx = v1->x + (INT64)(v2->x - v1->x) * (y - v1->y) / (v2->y - v1->y);


			if (vx < x)
				passes++;
		}
	}


	return passes % 2;
}




subsector_t *R_PointInSubsector(fixed_t x, fixed_t y)
{
	size_t nodenum = numnodes-1;

	while (!(nodenum & NF_SUBSECTOR))
		nodenum = nodes[nodenum].children[R_PointOnSide(x, y, nodes+nodenum)];

	return &subsectors[nodenum & ~NF_SUBSECTOR];
}




subsector_t *R_PointInSubsectorOrNull(fixed_t x, fixed_t y)
{
	node_t *node;
	INT32 side, i;
	size_t nodenum;
	subsector_t *ret;
	seg_t *seg;


	if (numnodes == 0)
		return subsectors;

	nodenum = numnodes - 1;

	while (!(nodenum & NF_SUBSECTOR))
	{
		node = &nodes[nodenum];
		side = R_PointOnSide(x, y, node);
		nodenum = node->children[side];
	}

	ret = &subsectors[nodenum & ~NF_SUBSECTOR];
	for (i = 0, seg = &segs[ret->firstline]; i < ret->numlines; i++, seg++)
	{
		if (seg->glseg)
			continue;


		if (P_PointOnLineSide(x, y, seg->linedef) != seg->side)
			return 0;
	}

	return ret;
}




void R_SetupFrame(player_t *player)
{
	camera_t *thiscam;
	boolean chasecam = R_ViewpointHasChasecam(player);
	boolean ispaused = paused || P_AutoPause();

	if (splitscreen && player == &players[secondarydisplayplayer] && player != &players[consoleplayer])
		thiscam = &camera2;
	else
		thiscam = &camera;

	newview->sky = false;

	if (player->awayviewtics)
	{

		r_viewmobj = player->awayviewmobj;  
		I_Assert(r_viewmobj != NULL);
		newview->z = r_viewmobj->z + 20*FRACUNIT;
		newview->aim = player->awayviewaiming;
		newview->angle = r_viewmobj->angle;
	}
	else if (!player->spectator && chasecam)

	{
		r_viewmobj = NULL;
		newview->z = thiscam->z + (thiscam->height>>1);
		newview->aim = thiscam->aiming;
		newview->angle = thiscam->angle;
	}
	else

	{
		newview->z = player->viewz;

		r_viewmobj = player->mo;
		I_Assert(r_viewmobj != NULL);

		newview->aim = player->aiming;
		newview->angle = r_viewmobj->angle;

		if (!demoplayback && player->playerstate != PST_DEAD)
		{
			if (player == &players[consoleplayer])
			{
				newview->angle = localangle;  
				newview->aim = localaiming;
			}
			else if (player == &players[secondarydisplayplayer])
			{
				newview->angle = localangle2;
				newview->aim = localaiming2;
			}
		}
	}

	if (quake.time && !ispaused)
	{
		fixed_t ir = quake.intensity>>1;

		if (quake.epicenter) {

			fixed_t xydist, dist;
			if (P_MobjWasRemoved(r_viewmobj)) {
				xydist = R_PointToDist2(thiscam->x, thiscam->y, quake.epicenter->x, quake.epicenter->y);
				dist = R_PointToDist2(0, thiscam->z, xydist, quake.epicenter->z);
			} else {
				xydist = R_PointToDist2(r_viewmobj->x, r_viewmobj->y, quake.epicenter->x, quake.epicenter->y);
				dist = R_PointToDist2(0, r_viewmobj->z, xydist, quake.epicenter->z);
			}


			if (!quake.radius || dist > quake.radius)
				ir = 0;
			else
				ir = FixedMul(ir, FRACUNIT - FixedDiv(dist, quake.radius));
		}

		quake.x = M_RandomRange(-ir,ir);
		quake.y = M_RandomRange(-ir,ir);
		quake.z = M_RandomRange(-ir,ir);
	}
	else if (!ispaused)
		quake.x = quake.y = quake.z = 0;

	newview->z += quake.z;

	newview->player = player;

	if (chasecam && !player->awayviewtics && !player->spectator)
	{
		newview->x = thiscam->x;
		newview->y = thiscam->y;
		newview->x += quake.x;
		newview->y += quake.y;

		if (thiscam->subsector)
			newview->sector = thiscam->subsector->sector;
		else
			newview->sector = R_PointInSubsector(newview->x, newview->y)->sector;
	}
	else
	{
		newview->x = r_viewmobj->x;
		newview->y = r_viewmobj->y;
		newview->x += quake.x;
		newview->y += quake.y;

		if (!P_MobjWasRemoved(r_viewmobj) && r_viewmobj->subsector)
			newview->sector = r_viewmobj->subsector->sector;
		else
			newview->sector = R_PointInSubsector(newview->x, newview->y)->sector;
	}




	R_InterpolateView(R_UsingFrameInterpolation() ? rendertimefrac : FRACUNIT);
}

void R_SkyboxFrame(player_t *player)
{
	camera_t *thiscam;

	if (splitscreen && player == &players[secondarydisplayplayer]
	&& player != &players[consoleplayer])
	{
		thiscam = &camera2;
		R_SetViewContext(VIEWCONTEXT_SKY2);
	}
	else
	{
		thiscam = &camera;
		R_SetViewContext(VIEWCONTEXT_SKY1);
	}


	newview->sky = true;
	r_viewmobj = skyboxmo[0];
#ifdef PARANOIA
	if (!r_viewmobj)
	{
		const size_t playeri = (size_t)(player - players);
		I_Error("R_SkyboxFrame: r_viewmobj null (player %s)", sizeu1(playeri));
	}
#endif
	if (player->awayviewtics)
	{
		newview->aim = player->awayviewaiming;
		newview->angle = player->awayviewmobj->angle;
	}
	else if (thiscam->chase)
	{
		newview->aim = thiscam->aiming;
		newview->angle = thiscam->angle;
	}
	else
	{
		newview->aim = player->aiming;
		newview->angle = player->mo->angle;
		if (!demoplayback && player->playerstate != PST_DEAD)
		{
			if (player == &players[consoleplayer])
			{
				newview->angle = localangle;  
				newview->aim = localaiming;
			}
			else if (player == &players[secondarydisplayplayer])
			{
				newview->angle = localangle2;
				newview->aim = localaiming2;
			}
		}
	}
	newview->angle += r_viewmobj->angle;

	newview->player = player;

	newview->x = r_viewmobj->x;
	newview->y = r_viewmobj->y;
	newview->z = r_viewmobj->z;  

	if (mapheaderinfo[gamemap-1])
	{
		mapheader_t *mh = mapheaderinfo[gamemap-1];
		vector3_t campos = {0,0,0};  

		if (player->awayviewtics) {
			campos.x = player->awayviewmobj->x;
			campos.y = player->awayviewmobj->y;
			campos.z = player->awayviewmobj->z + 20*FRACUNIT;
		} else if (thiscam->chase) {
			campos.x = thiscam->x;
			campos.y = thiscam->y;
			campos.z = thiscam->z + (thiscam->height>>1);
		} else {
			campos.x = player->mo->x;
			campos.y = player->mo->y;
			campos.z = player->viewz;
		}



		campos.x += quake.x;
		campos.y += quake.y;
		campos.z += quake.z;

		if (skyboxmo[1])  
		{
			fixed_t x = 0, y = 0;
			if (mh->skybox_scalex > 0)
				x = (campos.x - skyboxmo[1]->x) / mh->skybox_scalex;
			else if (mh->skybox_scalex < 0)
				x = (campos.x - skyboxmo[1]->x) * -mh->skybox_scalex;

			if (mh->skybox_scaley > 0)
				y = (campos.y - skyboxmo[1]->y) / mh->skybox_scaley;
			else if (mh->skybox_scaley < 0)
				y = (campos.y - skyboxmo[1]->y) * -mh->skybox_scaley;

			if (r_viewmobj->angle == 0)
			{
				newview->x += x;
				newview->y += y;
			}
			else if (r_viewmobj->angle == ANGLE_90)
			{
				newview->x -= y;
				newview->y += x;
			}
			else if (r_viewmobj->angle == ANGLE_180)
			{
				newview->x -= x;
				newview->y -= y;
			}
			else if (r_viewmobj->angle == ANGLE_270)
			{
				newview->x += y;
				newview->y -= x;
			}
			else
			{
				angle_t ang = r_viewmobj->angle>>ANGLETOFINESHIFT;
				newview->x += FixedMul(x,FINECOSINE(ang)) - FixedMul(y,  FINESINE(ang));
				newview->y += FixedMul(x,  FINESINE(ang)) + FixedMul(y,FINECOSINE(ang));
			}
		}
		if (mh->skybox_scalez > 0)
			newview->z += campos.z / mh->skybox_scalez;
		else if (mh->skybox_scalez < 0)
			newview->z += campos.z * -mh->skybox_scalez;
	}

	if (!P_MobjWasRemoved(r_viewmobj) && r_viewmobj->subsector)
		newview->sector = r_viewmobj->subsector->sector;
	else
		newview->sector = R_PointInSubsector(newview->x, newview->y)->sector;




	R_InterpolateView(R_UsingFrameInterpolation() ? rendertimefrac : FRACUNIT);
}

boolean R_ViewpointHasChasecam(player_t *player)
{
	camera_t *thiscam;
	boolean chasecam = false;
	boolean isplayer2 = (splitscreen && player == &players[secondarydisplayplayer] && player != &players[consoleplayer]);

	if (isplayer2)
	{
		thiscam = &camera2;
		chasecam = (cv_chasecam2.value != 0);
	}
	else
	{
		thiscam = &camera;
		chasecam = (cv_chasecam.value != 0);
	}

	if (player->climbing || (player->powers[pw_carry] == CR_NIGHTSMODE) || player->playerstate == PST_DEAD || gamestate == GS_TITLESCREEN || tutorialmode)
		chasecam = true;  
	else if (player->spectator)  
		chasecam = false;  

	if (chasecam && !thiscam->chase)
	{
		P_ResetCamera(player, thiscam);
		thiscam->chase = true;
	}
	else if (!chasecam && thiscam->chase)
	{
		P_ResetCamera(player, thiscam);
		thiscam->chase = false;
	}

	if (isplayer2)
	{
		R_SetViewContext(VIEWCONTEXT_PLAYER2);
		if (thiscam->reset)
		{
			R_ResetViewInterpolation(2);
			thiscam->reset = false;
		}
	}
	else
	{
		R_SetViewContext(VIEWCONTEXT_PLAYER1);
		if (thiscam->reset)
		{
			R_ResetViewInterpolation(1);
			thiscam->reset = false;
		}
	}

	return chasecam;
}

boolean R_IsViewpointThirdPerson(player_t *player, boolean skybox)
{
	boolean chasecam = R_ViewpointHasChasecam(player);


	if (player->awayviewtics || skybox)
		return chasecam;

	else if (!player->spectator && chasecam)
		return true;


	return false;
}

static void R_PortalFrame(portal_t *portal)
{
	viewx = portal->viewx;
	viewy = portal->viewy;
	viewz = portal->viewz;

	viewangle = portal->viewangle;
	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	if (!P_MobjWasRemoved(portal->viewmobj))
		r_viewmobj = portal->viewmobj;

	portalclipstart = portal->start;
	portalclipend = portal->end;

	if (portal->clipline != -1)
	{
		portalclipline = &lines[portal->clipline];
		portalcullsector = portalclipline->frontsector;
		viewsector = portalclipline->frontsector;
	}
	else
	{
		portalclipline = NULL;
		portalcullsector = NULL;
		viewsector = R_PointInSubsector(viewx, viewy)->sector;
	}
}

static void Mask_Pre (maskcount_t* m)
{
	m->drawsegs[0] = ds_p - drawsegs;
	m->vissprites[0] = visspritecount;
	m->viewx = viewx;
	m->viewy = viewy;
	m->viewz = viewz;
	m->viewsector = viewsector;
}

static void Mask_Post (maskcount_t* m)
{
	m->drawsegs[1] = ds_p - drawsegs;
	m->vissprites[1] = visspritecount;
}











static fixed_t viewfov[2];

void R_RenderPlayerView(player_t *player)
{
	INT32			nummasks	= 1;
	maskcount_t*	masks		= malloc(sizeof(maskcount_t));

	if (cv_homremoval.value && player == &players[displayplayer])  
	{
		if (cv_homremoval.value == 1)
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);  
		else  
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 32+(timeinmap&15));
	}

	fixed_t fov = R_GetPlayerFov(player);

	if (player == &players[displayplayer] && viewfov[0] != fov)
	{
		viewfov[0] = fov;
		R_SetFov(fov);
	}
	else if (player == &players[secondarydisplayplayer] && viewfov[1] != fov)
	{
		viewfov[1] = fov;
		R_SetFov(fov);
	}

	R_SetupFrame(player);
	framecount++;
	validcount++;


	R_ClearPlanes();
	if (viewmorph.use)
	{
		portalclipstart = viewmorph.x1;
		portalclipend = viewwidth-viewmorph.x1-1;
		R_PortalClearClipSegs(portalclipstart, portalclipend);
		memcpy(ceilingclip, viewmorph.ceilingclip, sizeof(INT16)*vid.width);
		memcpy(floorclip, viewmorph.floorclip, sizeof(INT16)*vid.width);
	}
	else
	{
		portalclipstart = 0;
		portalclipend = viewwidth;
		R_ClearClipSegs();
	}
	R_ClearDrawSegs();
	R_ClearSegTables();
	R_ClearSprites();
	Portal_InitList();


	NetUpdate();



	Mask_Pre(&masks[nummasks - 1]);
	curdrawsegs = ds_p;
	ps_numbspcalls.value.i = ps_numpolyobjects.value.i = ps_numdrawnodes.value.i = 0;
	PS_START_TIMING(ps_bsptime);
	R_RenderBSPNode((INT32)numnodes - 1);
	PS_STOP_TIMING(ps_bsptime);
	Mask_Post(&masks[nummasks - 1]);

	PS_START_TIMING(ps_sw_spritecliptime);
	R_ClipSprites(drawsegs, NULL);
	PS_STOP_TIMING(ps_sw_spritecliptime);

	ps_numsprites.value.i = numvisiblesprites;


	Portal_AddPlanePortals(R_SkyboxEnabled());


	PS_START_TIMING(ps_sw_portaltime);
	if (portal_base)
	{
		portal_t *portal;

		for(portal = portal_base; portal; portal = portal_base)
		{
			portalrender = portal->pass;  

			R_ClearFFloorClips();


			R_PortalFrame(portal);




			R_PortalClearClipSegs(portal->start, portal->end);



			Portal_ClipApply(portal);

			validcount++;

			masks = realloc(masks, (++nummasks)*sizeof(maskcount_t));

			Mask_Pre(&masks[nummasks - 1]);
			curdrawsegs = ds_p;

			if (portal->is_horizon)
			{

				R_RenderPortalHorizonLine(portal->horizon_sector);
			}
			else
			{


				R_RenderBSPNode((INT32)numnodes - 1);
			}


			Portal_AddPlanePortals(R_SkyboxEnabled() && !portal->is_skybox);

			Mask_Post(&masks[nummasks - 1]);

			R_ClipSprites(ds_p - (masks[nummasks - 1].drawsegs[1] - masks[nummasks - 1].drawsegs[0]), portal);

			Portal_Remove(portal);
		}
	}
	PS_STOP_TIMING(ps_sw_portaltime);

	PS_START_TIMING(ps_sw_planetime);
	R_DrawPlanes();
	PS_STOP_TIMING(ps_sw_planetime);



	PS_START_TIMING(ps_sw_maskedtime);
	R_DrawMasked(masks, nummasks);
	PS_STOP_TIMING(ps_sw_maskedtime);

	free(masks);
}





void R_RegisterEngineStuff(void)
{
	                                                                       
                                                                          
                                                                            
	CV_RegisterVar(&cv_cam_speed);


	if (dedicated)
		return;

	CV_RegisterVar(&cv_homremoval);
	CV_RegisterVar(&cv_translucency);
	CV_RegisterVar(&cv_drawdist);
	CV_RegisterVar(&cv_drawdist_nights);
	CV_RegisterVar(&cv_drawdist_precip);
	CV_RegisterVar(&cv_fovchange);
	CV_RegisterVar(&cv_fov);

	CV_RegisterVar(&cv_chasecam);
	CV_RegisterVar(&cv_chasecam2);

	CV_RegisterVar(&cv_shadow);
	CV_RegisterVar(&cv_glmsaa);
	CV_RegisterVar(&cv_skybox);
	CV_RegisterVar(&cv_renderview);
	CV_RegisterVar(&cv_renderhitboxinterpolation);
	CV_RegisterVar(&cv_renderhitboxgldepth);
	CV_RegisterVar(&cv_renderhitbox);
	CV_RegisterVar(&cv_renderwalls);
	CV_RegisterVar(&cv_renderfloors);
	CV_RegisterVar(&cv_renderthings);
	CV_RegisterVar(&cv_ffloorclip);
	CV_RegisterVar(&cv_spriteclip);

	CV_RegisterVar(&cv_cam_dist);
	CV_RegisterVar(&cv_cam_still);
	CV_RegisterVar(&cv_cam_height);
	CV_RegisterVar(&cv_cam_rotate);
	CV_RegisterVar(&cv_cam_rotspeed);
	CV_RegisterVar(&cv_cam_turnmultiplier);
	CV_RegisterVar(&cv_cam_orbit);
	CV_RegisterVar(&cv_cam_adjust);

	CV_RegisterVar(&cv_cam2_dist);
	CV_RegisterVar(&cv_cam2_still);
	CV_RegisterVar(&cv_cam2_height);
	CV_RegisterVar(&cv_cam2_speed);
	CV_RegisterVar(&cv_cam2_rotate);
	CV_RegisterVar(&cv_cam2_rotspeed);
	CV_RegisterVar(&cv_cam2_turnmultiplier);
	CV_RegisterVar(&cv_cam2_orbit);
	CV_RegisterVar(&cv_cam2_adjust);

	CV_RegisterVar(&cv_cam_savedist[0][0]);
	CV_RegisterVar(&cv_cam_savedist[0][1]);
	CV_RegisterVar(&cv_cam_savedist[1][0]);
	CV_RegisterVar(&cv_cam_savedist[1][1]);

	CV_RegisterVar(&cv_cam_saveheight[0][0]);
	CV_RegisterVar(&cv_cam_saveheight[0][1]);
	CV_RegisterVar(&cv_cam_saveheight[1][0]);
	CV_RegisterVar(&cv_cam_saveheight[1][1]);

	CV_RegisterVar(&cv_showhud);
	CV_RegisterVar(&cv_translucenthud);

	CV_RegisterVar(&cv_maxportals);
	CV_RegisterVar(&cv_rotspritecache);
	COM_AddCommand("rotspritecachestats", R_RotatedPatchCacheStats_f, COM_LUA);


	CV_RegisterVar(&cv_fpscap);
	CV_RegisterVar(&cv_smoothframepacing);
	CV_RegisterVar(&cv_adaptiveeffects);
	COM_AddCommand("perfprofile", PS_DeepProfile_f, COM_LUA);
	COM_AddCommand("perfreport", PS_PerformanceReport_f, COM_LUA);
}
