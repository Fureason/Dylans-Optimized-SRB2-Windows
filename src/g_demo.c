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
#include "console.h"
#include "d_main.h"
#include "d_player.h"
#include "netcode/d_clisrv.h"
#include "p_setup.h"
#include "i_time.h"
#include "i_system.h"
#include "m_random.h"
#include "p_local.h"
#include "r_draw.h"
#include "r_main.h"
#include "g_game.h"
#include "g_demo.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_argv.h"
#include "hu_stuff.h"
#include "z_zone.h"
#include "i_video.h"
#include "byteptr.h"
#include "i_joy.h"
#include "r_local.h"
#include "r_skins.h"
#include "y_inter.h"
#include "v_video.h"
#include "lua_hook.h"
#include "md5.h"  
#include "netcode/d_netfil.h"  

boolean timingdemo;  
boolean nodrawers;  
boolean noblit;  
tic_t demostarttime;  

static char demoname[64];
boolean demorecording;
boolean demoplayback;
boolean titledemo;  
demo_file_override_e demofileoverride;
static UINT8 *demobuffer = NULL;
static UINT8 *demo_p, *demotime_p;
static UINT8 *demoend;
static UINT8 demoflags;
UINT16 demoversion;
boolean singledemo;  
boolean demo_start;  
boolean demo_forwardmove_rng;  
boolean demosynced = true;  

boolean metalrecording;  
mobj_t *metalplayback;
static UINT8 *metalbuffer = NULL;
static UINT8 *metal_p;
static UINT16 metalversion;

consvar_t cv_resyncdemo = CVAR_INIT("resyncdemo", "On", 0, CV_OnOff, NULL);


static struct {
	UINT8 flags;  


	UINT16 color, lastcolor;


	fixed_t scale, lastscale;


	UINT16 hits;
	mobj_t **hitlist;
} ghostext;



typedef struct demoghost {
	UINT8 checksum[16];
	UINT8 *buffer, *p, fadein;
	UINT16 color;
	UINT16 version;
	mobj_t oldmo, *mo;
	struct demoghost *next;
} demoghost;
demoghost *ghosts = NULL;





#define DEMOVERSION 0x0011
#define DEMOHEADER  "\xF0" "SRB2Replay" "\x0F"

#define DF_GHOST        0x01  
#define DF_RECORDATTACK 0x02  
#define DF_NIGHTSATTACK 0x04  
#define DF_ATTACKMASK   0x06  
#define DF_ATTACKSHIFT  1


#define ZT_FWD     0x01
#define ZT_SIDE    0x02
#define ZT_ANGLE   0x04
#define ZT_BUTTONS 0x08
#define ZT_AIMING  0x10
#define ZT_LATENCY 0x20
#define DEMOMARKER 0x80  
#define METALDEATH 0x44
#define METALSNICE 0x69

static ticcmd_t oldcmd;


#define GZT_XYZ    0x01
#define GZT_MOMXY  0x02
#define GZT_MOMZ   0x04
#define GZT_ANGLE  0x08
#define GZT_FRAME  0x10  
#define GZT_SPR2   0x20  
#define GZT_EXTRA  0x40
#define GZT_FOLLOW 0x80  


#define EZT_THOK   0x01  
#define EZT_SPIN   0x02  
#define EZT_REV    0x03  
#define EZT_THOKMASK 0x03
#define EZT_COLOR  0x04  
#define EZT_FLIP   0x08  
#define EZT_SCALE  0x10  
#define EZT_HIT    0x20  
#define EZT_SPRITE 0x40  
#define EZT_HEIGHT 0x80  


#define FZT_SPAWNED 0x01  
#define FZT_SKIN 0x02  
#define FZT_LINKDRAW 0x04  
#define FZT_COLORIZED 0x08  
#define FZT_SCALE 0x10  


static mobj_t oldmetal, oldghost;

void G_SaveMetal(UINT8 **buffer)
{
	I_Assert(buffer != NULL && *buffer != NULL);

	WRITEUINT32(*buffer, metal_p - metalbuffer);
}

void G_LoadMetal(UINT8 **buffer)
{
	I_Assert(buffer != NULL && *buffer != NULL);

	G_DoPlayMetal();
	metal_p = metalbuffer + READUINT32(*buffer);
}


void G_ReadDemoTiccmd(ticcmd_t *cmd, INT32 playernum)
{
	UINT8 ziptic;

	if (!demo_p || !demo_start)
		return;
	ziptic = READUINT8(demo_p);

	if (ziptic & ZT_FWD)
		oldcmd.forwardmove = READSINT8(demo_p);
	if (ziptic & ZT_SIDE)
		oldcmd.sidemove = READSINT8(demo_p);
	if (ziptic & ZT_ANGLE)
		oldcmd.angleturn = READINT16(demo_p);
	if (ziptic & ZT_BUTTONS)
		oldcmd.buttons = (oldcmd.buttons & (BT_CAMLEFT|BT_CAMRIGHT)) | (READUINT16(demo_p) & ~(BT_CAMLEFT|BT_CAMRIGHT));
	if (ziptic & ZT_AIMING)
		oldcmd.aiming = READINT16(demo_p);
	if (ziptic & ZT_LATENCY)
		oldcmd.latency = READUINT8(demo_p);

	G_CopyTiccmd(cmd, &oldcmd, 1);
	players[playernum].angleturn = cmd->angleturn;

	if (!(demoflags & DF_GHOST) && *demo_p == DEMOMARKER)
	{

		G_CheckDemoStatus();
		return;
	}
}

void G_WriteDemoTiccmd(ticcmd_t *cmd, INT32 playernum)
{
	char ziptic = 0;
	UINT8 *ziptic_p;
	(void)playernum;

	if (!demo_p)
		return;
	ziptic_p = demo_p++;  

	if (cmd->forwardmove != oldcmd.forwardmove)
	{
		WRITEUINT8(demo_p,cmd->forwardmove);
		oldcmd.forwardmove = cmd->forwardmove;
		ziptic |= ZT_FWD;
	}

	if (cmd->sidemove != oldcmd.sidemove)
	{
		WRITEUINT8(demo_p,cmd->sidemove);
		oldcmd.sidemove = cmd->sidemove;
		ziptic |= ZT_SIDE;
	}

	if (cmd->angleturn != oldcmd.angleturn)
	{
		WRITEINT16(demo_p,cmd->angleturn);
		oldcmd.angleturn = cmd->angleturn;
		ziptic |= ZT_ANGLE;
	}

	if (cmd->buttons != oldcmd.buttons)
	{
		WRITEUINT16(demo_p,cmd->buttons);
		oldcmd.buttons = cmd->buttons;
		ziptic |= ZT_BUTTONS;
	}

	if (cmd->aiming != oldcmd.aiming)
	{
		WRITEINT16(demo_p,cmd->aiming);
		oldcmd.aiming = cmd->aiming;
		ziptic |= ZT_AIMING;
	}

	if (cmd->latency != oldcmd.latency)
	{
		WRITEUINT8(demo_p,cmd->latency);
		oldcmd.latency = cmd->latency;
		ziptic |= ZT_LATENCY;
	}

	*ziptic_p = ziptic;



	if (!(demoflags & DF_GHOST) && ziptic_p > demoend - 9)
	{
		G_CheckDemoStatus();  
		return;
	}
}

void G_GhostAddThok(void)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	ghostext.flags = (ghostext.flags & ~EZT_THOKMASK) | EZT_THOK;
}

void G_GhostAddSpin(void)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	ghostext.flags = (ghostext.flags & ~EZT_THOKMASK) | EZT_SPIN;
}

void G_GhostAddRev(void)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	ghostext.flags = (ghostext.flags & ~EZT_THOKMASK) | EZT_REV;
}

void G_GhostAddFlip(void)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	ghostext.flags |= EZT_FLIP;
}

void G_GhostAddColor(ghostcolor_t color)
{
	if (!demorecording || !(demoflags & DF_GHOST))
		return;
	if (ghostext.lastcolor == (UINT16)color)
	{
		ghostext.flags &= ~EZT_COLOR;
		return;
	}
	ghostext.flags |= EZT_COLOR;
	ghostext.color = (UINT16)color;
}

void G_GhostAddScale(fixed_t scale)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	if (ghostext.lastscale == scale)
	{
		ghostext.flags &= ~EZT_SCALE;
		return;
	}
	ghostext.flags |= EZT_SCALE;
	ghostext.scale = scale;
}

void G_GhostAddHit(mobj_t *victim)
{
	if (!demorecording || !(demoflags & DF_GHOST))
		return;
	ghostext.flags |= EZT_HIT;
	ghostext.hits++;
	ghostext.hitlist = Z_Realloc(ghostext.hitlist, ghostext.hits * sizeof(mobj_t *), PU_LEVEL, NULL);
	ghostext.hitlist[ghostext.hits-1] = victim;
}

void G_WriteGhostTic(mobj_t *ghost)
{
	char ziptic = 0;
	UINT8 *ziptic_p;
	UINT32 i;
	fixed_t height;

	if (!demo_p)
		return;
	if (!(demoflags & DF_GHOST))
		return;  

	ziptic_p = demo_p++;  

	#define MAXMOM (0xFFFF<<8)


	if (abs(ghost->x-oldghost.x) > MAXMOM
	|| abs(ghost->y-oldghost.y) > MAXMOM
	|| abs(ghost->z-oldghost.z) > MAXMOM)
	{
		oldghost.x = ghost->x;
		oldghost.y = ghost->y;
		oldghost.z = ghost->z;
		ziptic |= GZT_XYZ;
		WRITEFIXED(demo_p,oldghost.x);
		WRITEFIXED(demo_p,oldghost.y);
		WRITEFIXED(demo_p,oldghost.z);
	}
	else
	{

		fixed_t momx = ghost->x-oldghost.x;
		fixed_t momy = ghost->y-oldghost.y;
		if (momx != oldghost.momx
		|| momy != oldghost.momy)
		{
			oldghost.momx = momx;
			oldghost.momy = momy;
			ziptic |= GZT_MOMXY;
			WRITEFIXED(demo_p,momx);
			WRITEFIXED(demo_p,momy);
		}
		momx = ghost->z-oldghost.z;
		if (momx != oldghost.momz)
		{
			oldghost.momz = momx;
			ziptic |= GZT_MOMZ;
			WRITEFIXED(demo_p,momx);
		}


		oldghost.x += oldghost.momx;
		oldghost.y += oldghost.momy;
		oldghost.z += oldghost.momz;
	}

	#undef MAXMOM




	if (ghost->player && ghost->player->drawangle>>24 != oldghost.angle)
	{
		oldghost.angle = ghost->player->drawangle>>24;
		ziptic |= GZT_ANGLE;
		WRITEUINT8(demo_p,oldghost.angle);
	}


	if ((ghost->frame & FF_FRAMEMASK) != oldghost.frame)
	{
		oldghost.frame = (ghost->frame & FF_FRAMEMASK);
		ziptic |= GZT_FRAME;
		WRITEUINT8(demo_p,oldghost.frame);
	}

	if (ghost->sprite == SPR_PLAY
	&& ghost->sprite2 != oldghost.sprite2)
	{
		oldghost.sprite2 = ghost->sprite2;
		ziptic |= GZT_SPR2;
		WRITEUINT16(demo_p,oldghost.sprite2);
	}


	if (ghost->sprite != oldghost.sprite)
	{
		oldghost.sprite = ghost->sprite;
		ghostext.flags |= EZT_SPRITE;
	}

	if ((height = FixedDiv(ghost->height, ghost->scale)) != oldghost.height)
	{
		oldghost.height = height;
		ghostext.flags |= EZT_HEIGHT;
	}

	if (ghostext.flags)
	{
		ziptic |= GZT_EXTRA;

		if (ghostext.color == ghostext.lastcolor)
			ghostext.flags &= ~EZT_COLOR;
		if (ghostext.scale == ghostext.lastscale)
			ghostext.flags &= ~EZT_SCALE;

		WRITEUINT8(demo_p,ghostext.flags);
		if (ghostext.flags & EZT_COLOR)
		{
			WRITEUINT16(demo_p,ghostext.color);
			ghostext.lastcolor = ghostext.color;
		}
		if (ghostext.flags & EZT_SCALE)
		{
			WRITEFIXED(demo_p,ghostext.scale);
			ghostext.lastscale = ghostext.scale;
		}
		if (ghostext.flags & EZT_HIT)
		{
			WRITEUINT16(demo_p,ghostext.hits);
			for (i = 0; i < ghostext.hits; i++)
			{
				mobj_t *mo = ghostext.hitlist[i];

				WRITEUINT32(demo_p,mo->type);
				WRITEUINT16(demo_p,(UINT16)mo->health);
				WRITEFIXED(demo_p,mo->x);
				WRITEFIXED(demo_p,mo->y);
				WRITEFIXED(demo_p,mo->z);
				WRITEANGLE(demo_p,mo->angle);
			}
			Z_Free(ghostext.hitlist);
			ghostext.hits = 0;
			ghostext.hitlist = NULL;
		}
		if (ghostext.flags & EZT_SPRITE)
			WRITEUINT16(demo_p,oldghost.sprite);
		if (ghostext.flags & EZT_HEIGHT)
		{
			WRITEFIXED(demo_p, height);
		}
		ghostext.flags = 0;
	}

	if (ghost->player && ghost->player->followmobj && !(ghost->player->followmobj->sprite == SPR_NULL || (ghost->player->followmobj->flags2 & MF2_DONTDRAW)))  
	{
		fixed_t temp;
		UINT8 *followtic_p = demo_p++;
		UINT8 followtic = 0;

		ziptic |= GZT_FOLLOW;

		if (ghost->player->followmobj->skin)
			followtic |= FZT_SKIN;

		if (!(oldghost.flags2 & MF2_AMBUSH))
		{
			followtic |= FZT_SPAWNED;
			WRITEINT16(demo_p,ghost->player->followmobj->info->height>>FRACBITS);
			if (ghost->player->followmobj->flags2 & MF2_LINKDRAW)
				followtic |= FZT_LINKDRAW;
			if (ghost->player->followmobj->colorized)
				followtic |= FZT_COLORIZED;
			if (followtic & FZT_SKIN)
				WRITEUINT8(demo_p,(UINT8)(((skin_t *)ghost->player->followmobj->skin)->skinnum));
			oldghost.flags2 |= MF2_AMBUSH;
		}

		if (ghost->player->followmobj->scale != ghost->scale)
		{
			followtic |= FZT_SCALE;
			WRITEFIXED(demo_p,ghost->player->followmobj->scale);
		}

		temp = ghost->player->followmobj->x-ghost->x;
		WRITEFIXED(demo_p,temp);
		temp = ghost->player->followmobj->y-ghost->y;
		WRITEFIXED(demo_p,temp);
		temp = ghost->player->followmobj->z-ghost->z;
		WRITEFIXED(demo_p,temp);
		if (followtic & FZT_SKIN)
			WRITEUINT16(demo_p,ghost->player->followmobj->sprite2);
		WRITEUINT16(demo_p,ghost->player->followmobj->sprite);
		WRITEUINT8(demo_p,(ghost->player->followmobj->frame & FF_FRAMEMASK));
		WRITEUINT16(demo_p,ghost->player->followmobj->color);

		*followtic_p = followtic;
	}
	else
		oldghost.flags2 &= ~MF2_AMBUSH;

	*ziptic_p = ziptic;



	if (demo_p >= demoend - (13 + 9 + 9))
	{
		G_CheckDemoStatus();  
		return;
	}
}



void G_ConsGhostTic(void)
{
	UINT8 ziptic;
	UINT16 px,py,pz,gx,gy,gz;
	mobj_t *testmo;

	if (!demo_p || !demo_start)
		return;
	if (!(demoflags & DF_GHOST))
		return;  

	testmo = players[0].mo;

	if (P_MobjWasRemoved(testmo))
		return;  


	ziptic = READUINT8(demo_p);
	if (ziptic & GZT_XYZ)
	{
		oldghost.x = READFIXED(demo_p);
		oldghost.y = READFIXED(demo_p);
		oldghost.z = READFIXED(demo_p);
	}
	else
	{
		if (ziptic & GZT_MOMXY)
		{
			oldghost.momx = (demoversion < 0x000e) ? READINT16(demo_p)<<8 : READFIXED(demo_p);
			oldghost.momy = (demoversion < 0x000e) ? READINT16(demo_p)<<8 : READFIXED(demo_p);
		}
		if (ziptic & GZT_MOMZ)
			oldghost.momz = (demoversion < 0x000e) ? READINT16(demo_p)<<8 : READFIXED(demo_p);
		oldghost.x += oldghost.momx;
		oldghost.y += oldghost.momy;
		oldghost.z += oldghost.momz;
	}
	if (ziptic & GZT_ANGLE)
		demo_p++;
	if (ziptic & GZT_FRAME)
		demo_p++;
	if (ziptic & GZT_SPR2)
		demo_p += (demoversion < 0x0011) ? sizeof(UINT8) : sizeof(UINT16);

	if (ziptic & GZT_EXTRA)
	{  
		UINT8 xziptic = READUINT8(demo_p);
		if (xziptic & EZT_COLOR)
			demo_p += (demoversion==0x000c) ? 1 : sizeof(UINT16);
		if (xziptic & EZT_SCALE)
			demo_p += sizeof(fixed_t);
		if (xziptic & EZT_HIT)
		{  
			UINT16 i, count = READUINT16(demo_p);
			thinker_t *th;
			mobj_t *mobj;

			UINT32 type;
			UINT16 health;
			fixed_t x;
			fixed_t y;
			fixed_t z;

			for (i = 0; i < count; i++)
			{

				type = READUINT32(demo_p);
				health = READUINT16(demo_p);
				x = READFIXED(demo_p);
				y = READFIXED(demo_p);
				z = READFIXED(demo_p);
				demo_p += sizeof(angle_t);  

				mobj = NULL;
				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->removing)
						continue;
					mobj = (mobj_t *)th;
					if (mobj->type == (mobjtype_t)type && mobj->x == x && mobj->y == y && mobj->z == z)
						break;
				}
				if (th != &thlist[THINK_MOBJ] && mobj->health != health)  
				{
					if (demosynced)
						CONS_Alert(CONS_WARNING, M_GetText("Demo playback has desynced!\n"));
					demosynced = false;
					P_DamageMobj(mobj, players[0].mo, players[0].mo, 1, 0);
				}
			}
		}
		if (xziptic & EZT_SPRITE)
			demo_p += sizeof(UINT16);
		if (xziptic & EZT_HEIGHT)
			demo_p += (demoversion < 0x000e) ? sizeof(INT16) : sizeof(fixed_t);
	}

	if (ziptic & GZT_FOLLOW)
	{  
		UINT8 followtic = READUINT8(demo_p);
		if (followtic & FZT_SPAWNED)
		{
			demo_p += sizeof(INT16);
			if (followtic & FZT_SKIN)
				demo_p++;
		}
		if (followtic & FZT_SCALE)
			demo_p += sizeof(fixed_t);

		demo_p += (demoversion < 0x000e) ? sizeof(INT16) * 3 : sizeof(fixed_t) * 3;
		if (followtic & FZT_SKIN)
			demo_p += (demoversion < 0x0011) ? sizeof(UINT8) : sizeof(UINT16);
		demo_p += sizeof(UINT16);
		demo_p++;
		demo_p += (demoversion==0x000c) ? 1 : sizeof(UINT16);
	}


	px = testmo->x>>FRACBITS;
	py = testmo->y>>FRACBITS;
	pz = testmo->z>>FRACBITS;
	gx = oldghost.x>>FRACBITS;
	gy = oldghost.y>>FRACBITS;
	gz = oldghost.z>>FRACBITS;

	if (px != gx || py != gy || pz != gz)
	{
		if (demosynced)
			CONS_Alert(CONS_WARNING, M_GetText("Demo playback has desynced!\n"));
		demosynced = false;

		if (cv_resyncdemo.value)
		{
			P_UnsetThingPosition(testmo);
			testmo->x = oldghost.x;
			testmo->y = oldghost.y;
			P_SetThingPosition(testmo);
			testmo->z = oldghost.z;
		}
	}

	if (*demo_p == DEMOMARKER)
	{

		G_CheckDemoStatus();
		return;
	}
}

void G_GhostTicker(void)
{
	demoghost *g,*p;
	for(g = ghosts, p = NULL; g; g = g->next)
	{

		UINT8 ziptic = READUINT8(g->p);
		UINT8 xziptic = 0;
		if (ziptic & ZT_FWD)
			g->p++;
		if (ziptic & ZT_SIDE)
			g->p++;
		if (ziptic & ZT_ANGLE)
			g->p += 2;
		if (ziptic & ZT_BUTTONS)
			g->p += 2;
		if (ziptic & ZT_AIMING)
			g->p += 2;
		if (ziptic & ZT_LATENCY)
			g->p++;


		ziptic = READUINT8(g->p);
		if (ziptic & GZT_XYZ)
		{
			g->oldmo.x = READFIXED(g->p);
			g->oldmo.y = READFIXED(g->p);
			g->oldmo.z = READFIXED(g->p);
		}
		else
		{
			if (ziptic & GZT_MOMXY)
			{
				g->oldmo.momx = (g->version < 0x000e) ? READINT16(g->p)<<8 : READFIXED(g->p);
				g->oldmo.momy = (g->version < 0x000e) ? READINT16(g->p)<<8 : READFIXED(g->p);
			}
			if (ziptic & GZT_MOMZ)
				g->oldmo.momz = (g->version < 0x000e) ? READINT16(g->p)<<8 : READFIXED(g->p);
			g->oldmo.x += g->oldmo.momx;
			g->oldmo.y += g->oldmo.momy;
			g->oldmo.z += g->oldmo.momz;
		}
		if (ziptic & GZT_ANGLE)
			g->mo->angle = READUINT8(g->p)<<24;
		if (ziptic & GZT_FRAME)
			g->oldmo.frame = READUINT8(g->p);
		if (ziptic & GZT_SPR2)
			g->oldmo.sprite2 = (g->version < 0x0011) ? READUINT8(g->p) : READUINT16(g->p);


		P_UnsetThingPosition(g->mo);
		g->mo->x = g->oldmo.x;
		g->mo->y = g->oldmo.y;
		g->mo->z = g->oldmo.z;
		P_SetThingPosition(g->mo);
		g->mo->frame = g->oldmo.frame | tr_trans30<<FF_TRANSSHIFT;
		if (g->fadein)
		{
			g->mo->frame += (((--g->fadein)/6)<<FF_TRANSSHIFT);  
			g->mo->flags2 &= ~MF2_DONTDRAW;
		}
		g->mo->sprite2 = g->oldmo.sprite2;

		if (ziptic & GZT_EXTRA)
		{  
			xziptic = READUINT8(g->p);
			if (xziptic & EZT_COLOR)
			{
				g->color = (g->version==0x000c) ? READUINT8(g->p) : READUINT16(g->p);
				switch(g->color)
				{
				default:
				case GHC_RETURNSKIN:
					g->mo->skin = g->oldmo.skin;
					 
				case GHC_NORMAL:  
					g->mo->color = g->oldmo.color;
					break;

				case GHC_SUPER:
				case GHC_INVINCIBLE:
					break;
				case GHC_FIREFLOWER:  
					g->mo->color = SKINCOLOR_WHITE;
					break;
				case GHC_NIGHTSSKIN:  
					g->mo->skin = skins[DEFAULTNIGHTSSKIN];
					break;
				}
			}
			if (xziptic & EZT_FLIP)
				g->mo->eflags ^= MFE_VERTICALFLIP;
			if (xziptic & EZT_SCALE)
			{
				g->mo->destscale = READFIXED(g->p);
				if (g->mo->destscale != g->mo->scale)
					P_SetScale(g->mo, g->mo->destscale, false);
			}
			if (xziptic & EZT_THOKMASK)
			{  
				mobj_t *mobj;
				UINT32 type = MT_NULL;
				if (g->mo->skin)
				{
					skin_t *skin = (skin_t *)g->mo->skin;
					switch (xziptic & EZT_THOKMASK)
					{
					case EZT_THOK:
						type = skin->thokitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].painchance : (UINT32)skin->thokitem;
						break;
					case EZT_SPIN:
						type = skin->spinitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].damage : (UINT32)skin->spinitem;
						break;
					case EZT_REV:
						type = skin->revitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].raisestate : (UINT32)skin->revitem;
						break;
					}
				}
				if (type != MT_NULL)
				{
					if (type == MT_GHOST)
					{
						mobj = P_SpawnGhostMobj(g->mo);  
						if (!P_MobjWasRemoved(mobj))
							mobj->frame = (mobj->frame & ~FF_FRAMEMASK)|tr_trans60<<FF_TRANSSHIFT;  
					}
					else
					{
						mobj = P_SpawnMobjFromMobj(g->mo, 0, 0, -FixedDiv(FixedMul(g->mo->info->height, g->mo->scale) - g->mo->height,3*FRACUNIT), MT_THOK);
						if (!P_MobjWasRemoved(mobj))
						{
							mobj->sprite = states[mobjinfo[type].spawnstate].sprite;
							mobj->frame = (states[mobjinfo[type].spawnstate].frame & FF_FRAMEMASK) | tr_trans60<<FF_TRANSSHIFT;
							mobj->color = g->mo->color;
							mobj->skin = g->mo->skin;
							P_SetScale(mobj, g->mo->scale, true);

							if (type == MT_THOK)  
							{
								mobj->frame = FF_TRANS80;
								mobj->fuse = mobj->tics;
							}
							mobj->tics = -1;  
						}
					}

					if (!P_MobjWasRemoved(mobj))
					{
						mobj->floorz = mobj->z;
						mobj->ceilingz = mobj->z+mobj->height;
						P_UnsetThingPosition(mobj);
						mobj->flags = MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY;  
						P_SetThingPosition(mobj);
						if (!mobj->fuse)
							mobj->fuse = 8;
						P_SetTarget(&mobj->target, g->mo);
					}
				}
			}
			if (xziptic & EZT_HIT)
			{  
				UINT16 i, count = READUINT16(g->p), health;
				UINT32 type;
				fixed_t x,y,z;
				angle_t angle;
				mobj_t *poof;
				for (i = 0; i < count; i++)
				{

					type = READUINT32(g->p);
					health = READUINT16(g->p);
					x = READFIXED(g->p);
					y = READFIXED(g->p);
					z = READFIXED(g->p);
					angle = READANGLE(g->p);
					if (!(mobjinfo[type].flags & MF_SHOOTABLE)
					|| !(mobjinfo[type].flags & (MF_ENEMY|MF_MONITOR))
					|| health != 0 || i >= 4)  
						continue;
					poof = P_SpawnMobj(x, y, z, MT_GHOST);
					if (P_MobjWasRemoved(poof))
						continue;
					poof->angle = angle;
					poof->flags = MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY;  
					poof->health = 0;
					P_SetMobjStateNF(poof, S_XPLD1);
				}
			}
			if (xziptic & EZT_SPRITE)
				g->mo->sprite = READUINT16(g->p);
			if (xziptic & EZT_HEIGHT)
			{
				fixed_t temp = (g->version < 0x000e) ? READINT16(g->p)<<FRACBITS : READFIXED(g->p);
				g->mo->height = FixedMul(temp, g->mo->scale);
			}
		}


		switch(g->color)
		{
		case GHC_SUPER:  
			if (g->mo->skin)
			{
				skin_t *skin = (skin_t *)g->mo->skin;
				g->mo->color = skin->supercolor;
			}
			else
				g->mo->color = SKINCOLOR_SUPERGOLD1;
			g->mo->color += abs( ( (signed)( (unsigned)leveltime >> 1 ) % 9) - 4);
			break;
		case GHC_INVINCIBLE:  
			g->mo->color = (UINT16)(SKINCOLOR_RUBY + (leveltime % (FIRSTSUPERCOLOR - SKINCOLOR_RUBY)));  
			break;
		default:
			break;
		}

#define follow g->mo->tracer
		if (ziptic & GZT_FOLLOW)
		{  
			UINT8 followtic = READUINT8(g->p);
			fixed_t temp;
			if (followtic & FZT_SPAWNED)
			{
				if (follow)
					P_RemoveMobj(follow);
				P_SetTarget(&follow, P_SpawnMobjFromMobj(g->mo, 0, 0, 0, MT_GHOST));
				if (!P_MobjWasRemoved(follow))
				{
					P_SetTarget(&follow->tracer, g->mo);
					follow->tics = -1;
					temp = READINT16(g->p)<<FRACBITS;
					follow->height = FixedMul(follow->scale, temp);

					if (followtic & FZT_LINKDRAW)
						follow->flags2 |= MF2_LINKDRAW;

					if (followtic & FZT_COLORIZED)
						follow->colorized = true;

					if (followtic & FZT_SKIN)
						follow->skin = skins[READUINT8(g->p)];
				}
			}
			if (follow)
			{
				if (followtic & FZT_SCALE)
					follow->destscale = READFIXED(g->p);
				else
					follow->destscale = g->mo->destscale;
				if (follow->destscale != follow->scale)
					P_SetScale(follow, follow->destscale, false);

				P_UnsetThingPosition(follow);
				temp = (g->version < 0x000e) ? READINT16(g->p)<<8 : READFIXED(g->p);
				follow->x = g->mo->x + temp;
				temp = (g->version < 0x000e) ? READINT16(g->p)<<8 : READFIXED(g->p);
				follow->y = g->mo->y + temp;
				temp = (g->version < 0x000e) ? READINT16(g->p)<<8 : READFIXED(g->p);
				follow->z = g->mo->z + temp;
				P_SetThingPosition(follow);
				if (followtic & FZT_SKIN)
					follow->sprite2 = (g->version < 0x0011) ? READUINT8(g->p) : READUINT16(g->p);
				else
					follow->sprite2 = 0;
				follow->sprite = READUINT16(g->p);
				follow->frame = (READUINT8(g->p)) | (g->mo->frame & FF_TRANSMASK);
				follow->angle = g->mo->angle;
				follow->color = (g->version==0x000c) ? READUINT8(g->p) : READUINT16(g->p);

				if (!(followtic & FZT_SPAWNED))
				{
					if (xziptic & EZT_FLIP)
					{
						follow->flags2 ^= MF2_OBJECTFLIP;
						follow->eflags ^= MFE_VERTICALFLIP;
					}
				}
			}
		}
		else if (follow)
		{
			P_RemoveMobj(follow);
			P_SetTarget(&follow, NULL);
		}

		if (*g->p == DEMOMARKER)
		{
			g->mo->momx = g->mo->momy = g->mo->momz = 0;
#if 1  
			g->mo->colorized = true;
			if (follow)
				follow->colorized = true;
#else  
			g->mo->fuse = TICRATE;
			if (follow)
				follow->fuse = TICRATE;
#endif
			if (p)
				p->next = g->next;
			else
				ghosts = g->next;
			Z_Free(g);
			continue;
		}
		p = g;
#undef follow
	}
}

void G_ReadMetalTic(mobj_t *metal)
{
	UINT8 ziptic;
	UINT8 xziptic = 0;

	if (!metal_p)
		return;

	if (!metal->health)
	{
		G_StopMetalDemo();
		return;
	}

	switch (*metal_p)
	{
		case METALSNICE:
			break;
		case METALDEATH:
			if (metal->tracer)
				P_RemoveMobj(metal->tracer);
			P_KillMobj(metal, NULL, NULL, 0);
			 
		case DEMOMARKER:
		default:

			G_StopMetalDemo();
			return;
	}
	metal_p++;

	ziptic = READUINT8(metal_p);


	if (ziptic & GZT_XYZ)
	{

		oldmetal.x = READFIXED(metal_p);
		oldmetal.y = READFIXED(metal_p);
		oldmetal.z = READFIXED(metal_p);
		P_MoveOrigin(metal, oldmetal.x, oldmetal.y, oldmetal.z);
		oldmetal.x = metal->x;
		oldmetal.y = metal->y;
		oldmetal.z = metal->z;
	}
	else
	{
		if (ziptic & GZT_MOMXY)
		{
			oldmetal.momx = (metalversion < 0x000e) ? READINT16(metal_p)<<8 : READFIXED(metal_p);
			oldmetal.momy = (metalversion < 0x000e) ? READINT16(metal_p)<<8 : READFIXED(metal_p);
		}
		if (ziptic & GZT_MOMZ)
			oldmetal.momz = (metalversion < 0x000e) ? READINT16(metal_p)<<8 : READFIXED(metal_p);
		oldmetal.x += oldmetal.momx;
		oldmetal.y += oldmetal.momy;
		oldmetal.z += oldmetal.momz;
	}
	if (ziptic & GZT_ANGLE)
		metal->angle = READUINT8(metal_p)<<24;
	if (ziptic & GZT_FRAME)
	{
		oldmetal.frame = READUINT32(metal_p);
		if (metalversion < 0x000f)
			oldmetal.frame = G_ConvertOldFrameFlags(oldmetal.frame);
	}
	if (ziptic & GZT_SPR2)
		oldmetal.sprite2 = (metalversion < 0x0011) ? READUINT8(metal_p) : READUINT16(metal_p);



	metal->momx = oldmetal.momx;
	metal->momy = oldmetal.momy;
	metal->momz = oldmetal.momz;
	P_UnsetThingPosition(metal);
	metal->x = oldmetal.x;
	metal->y = oldmetal.y;
	metal->z = oldmetal.z;
	P_SetThingPosition(metal);
	metal->frame = oldmetal.frame;
	metal->sprite2 = oldmetal.sprite2;

	if (ziptic & GZT_EXTRA)
	{  
		xziptic = READUINT8(metal_p);
		if (xziptic & EZT_FLIP)
		{
			metal->eflags ^= MFE_VERTICALFLIP;
			metal->flags2 ^= MF2_OBJECTFLIP;
		}
		if (xziptic & EZT_SCALE)
		{
			metal->destscale = READFIXED(metal_p);
			if (metal->destscale != metal->scale)
				P_SetScale(metal, metal->destscale, false);
		}
		if (xziptic & EZT_THOKMASK)
		{  
			mobj_t *mobj;
			UINT32 type = MT_NULL;
			if (metal->skin)
			{
				skin_t *skin = (skin_t *)metal->skin;
				switch (xziptic & EZT_THOKMASK)
				{
				case EZT_THOK:
					type = skin->thokitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].painchance : (UINT32)skin->thokitem;
					break;
				case EZT_SPIN:
					type = skin->spinitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].damage : (UINT32)skin->spinitem;
					break;
				case EZT_REV:
					type = skin->revitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].raisestate : (UINT32)skin->revitem;
					break;
				}
			}
			if (type != MT_NULL)
			{
				if (type == MT_GHOST)
				{
					mobj = P_SpawnGhostMobj(metal);  
				}
				else
				{
					mobj = P_SpawnMobjFromMobj(metal, 0, 0, -FixedDiv(FixedMul(metal->info->height, metal->scale) - metal->height,3*FRACUNIT), MT_THOK);
					if (!P_MobjWasRemoved(mobj))
					{
						mobj->sprite = states[mobjinfo[type].spawnstate].sprite;
						mobj->frame = states[mobjinfo[type].spawnstate].frame;
						mobj->angle = metal->angle;
						mobj->color = metal->color;
						mobj->skin = metal->skin;
						P_SetScale(mobj, metal->scale, true);

						if (type == MT_THOK)  
						{
							mobj->frame = FF_TRANS70;
							mobj->fuse = mobj->tics;
						}
						mobj->tics = -1;  
					}
				}

				if (!P_MobjWasRemoved(mobj))
				{
					mobj->floorz = mobj->z;
					mobj->ceilingz = mobj->z+mobj->height;
					P_UnsetThingPosition(mobj);
					mobj->flags = MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY;  
					P_SetThingPosition(mobj);
					if (!mobj->fuse)
						mobj->fuse = 8;
					P_SetTarget(&mobj->target, metal);
				}
			}
		}
		if (xziptic & EZT_SPRITE)
			metal->sprite = READUINT16(metal_p);
		if (xziptic & EZT_HEIGHT)
		{
			fixed_t temp = (metalversion < 0x000e) ? READINT16(metal_p)<<FRACBITS : READFIXED(metal_p);
			metal->height = FixedMul(temp, metal->scale);
		}
	}

#define follow metal->tracer
		if (ziptic & GZT_FOLLOW)
		{  
			UINT8 followtic = READUINT8(metal_p);
			fixed_t temp;
			if (followtic & FZT_SPAWNED)
			{
				if (follow)
					P_RemoveMobj(follow);
				P_SetTarget(&follow, P_SpawnMobjFromMobj(metal, 0, 0, 0, MT_GHOST));
				if (!P_MobjWasRemoved(follow))
				{
					P_SetTarget(&follow->tracer, metal);
					follow->tics = -1;
					temp = READINT16(metal_p)<<FRACBITS;
					follow->height = FixedMul(follow->scale, temp);

					if (followtic & FZT_LINKDRAW)
						follow->flags2 |= MF2_LINKDRAW;

					if (followtic & FZT_COLORIZED)
						follow->colorized = true;

					if (followtic & FZT_SKIN)
						follow->skin = skins[READUINT8(metal_p)];
				}
			}
			if (follow)
			{
				if (followtic & FZT_SCALE)
					follow->destscale = READFIXED(metal_p);
				else
					follow->destscale = metal->destscale;
				if (follow->destscale != follow->scale)
					P_SetScale(follow, follow->destscale, false);

				P_UnsetThingPosition(follow);
				temp = (metalversion < 0x000e) ? READINT16(metal_p)<<8 : READFIXED(metal_p);
				follow->x = metal->x + temp;
				temp = (metalversion < 0x000e) ? READINT16(metal_p)<<8 : READFIXED(metal_p);
				follow->y = metal->y + temp;
				temp = (metalversion < 0x000e) ? READINT16(metal_p)<<8 : READFIXED(metal_p);
				follow->z = metal->z + temp;
				P_SetThingPosition(follow);
				if (followtic & FZT_SKIN)
					follow->sprite2 = (metalversion < 0x0011) ? READUINT8(metal_p) : READUINT16(metal_p);
				else
					follow->sprite2 = 0;
				follow->sprite = READUINT16(metal_p);
				follow->frame = READUINT32(metal_p);  
				if (metalversion < 0x000f)
					follow->frame = G_ConvertOldFrameFlags(follow->frame);
				follow->angle = metal->angle;
				follow->color = (metalversion == 0x000c) ? READUINT8(metal_p) : READUINT16(metal_p);

				if (!(followtic & FZT_SPAWNED))
				{
					if (xziptic & EZT_FLIP)
					{
						follow->flags2 ^= MF2_OBJECTFLIP;
						follow->eflags ^= MFE_VERTICALFLIP;
					}
				}
			}
		}
		else if (follow)
		{
			P_RemoveMobj(follow);
			P_SetTarget(&follow, NULL);
		}
#undef follow
}

void G_WriteMetalTic(mobj_t *metal)
{
	UINT8 ziptic = 0;
	UINT8 *ziptic_p;
	fixed_t height;

	if (!demo_p)  
		return;

	WRITEUINT8(demo_p, METALSNICE);
	ziptic_p = demo_p++;  

	#define MAXMOM (0xFFFF<<8)


	if (abs(metal->x-oldmetal.x) > MAXMOM
	|| abs(metal->y-oldmetal.y) > MAXMOM
	|| abs(metal->z-oldmetal.z) > MAXMOM)
	{
		oldmetal.x = metal->x;
		oldmetal.y = metal->y;
		oldmetal.z = metal->z;
		ziptic |= GZT_XYZ;
		WRITEFIXED(demo_p,oldmetal.x);
		WRITEFIXED(demo_p,oldmetal.y);
		WRITEFIXED(demo_p,oldmetal.z);
	}
	else
	{


		fixed_t momx = metal->x-oldmetal.x;
		fixed_t momy = metal->y-oldmetal.y;
		if (momx != oldmetal.momx
		|| momy != oldmetal.momy)
		{
			oldmetal.momx = momx;
			oldmetal.momy = momy;
			ziptic |= GZT_MOMXY;
			WRITEFIXED(demo_p,momx);
			WRITEFIXED(demo_p,momy);
		}
		momx = metal->z-oldmetal.z;
		if (momx != oldmetal.momz)
		{
			oldmetal.momz = momx;
			ziptic |= GZT_MOMZ;
			WRITEFIXED(demo_p,momx);
		}


		oldmetal.x += oldmetal.momx;
		oldmetal.y += oldmetal.momy;
		oldmetal.z += oldmetal.momz;
	}

	#undef MAXMOM




	if (metal->player && metal->player->drawangle>>24 != oldmetal.angle)
	{
		oldmetal.angle = metal->player->drawangle>>24;
		ziptic |= GZT_ANGLE;
		WRITEUINT8(demo_p,oldmetal.angle);
	}


	if ((metal->frame & FF_FRAMEMASK) != oldmetal.frame)
	{
		oldmetal.frame = metal->frame;  
		ziptic |= GZT_FRAME;
		WRITEUINT32(demo_p,oldmetal.frame);
	}

	if (metal->sprite == SPR_PLAY
	&& metal->sprite2 != oldmetal.sprite2)
	{
		oldmetal.sprite2 = metal->sprite2;
		ziptic |= GZT_SPR2;
		WRITEUINT16(demo_p,oldmetal.sprite2);
	}


	if (metal->sprite != oldmetal.sprite)
	{
		oldmetal.sprite = metal->sprite;
		ghostext.flags |= EZT_SPRITE;
	}

	if ((height = FixedDiv(metal->height, metal->scale)) != oldmetal.height)
	{
		oldmetal.height = height;
		ghostext.flags |= EZT_HEIGHT;
	}

	if (ghostext.flags & ~(EZT_COLOR|EZT_HIT))  
	{
		ziptic |= GZT_EXTRA;

		if (ghostext.scale == ghostext.lastscale)
			ghostext.flags &= ~EZT_SCALE;

		WRITEUINT8(demo_p,ghostext.flags);
		if (ghostext.flags & EZT_SCALE)
		{
			WRITEFIXED(demo_p,ghostext.scale);
			ghostext.lastscale = ghostext.scale;
		}
		if (ghostext.flags & EZT_SPRITE)
			WRITEUINT16(demo_p,oldmetal.sprite);
		if (ghostext.flags & EZT_HEIGHT)
		{
			WRITEFIXED(demo_p, height);
		}
		ghostext.flags = 0;
	}

	if (metal->player && metal->player->followmobj && !(metal->player->followmobj->sprite == SPR_NULL || (metal->player->followmobj->flags2 & MF2_DONTDRAW)))
	{
		fixed_t temp;
		UINT8 *followtic_p = demo_p++;
		UINT8 followtic = 0;

		ziptic |= GZT_FOLLOW;

		if (metal->player->followmobj->skin)
			followtic |= FZT_SKIN;

		if (!(oldmetal.flags2 & MF2_AMBUSH))
		{
			followtic |= FZT_SPAWNED;
			WRITEINT16(demo_p,metal->player->followmobj->info->height>>FRACBITS);
			if (metal->player->followmobj->flags2 & MF2_LINKDRAW)
				followtic |= FZT_LINKDRAW;
			if (metal->player->followmobj->colorized)
				followtic |= FZT_COLORIZED;
			if (followtic & FZT_SKIN)
				WRITEUINT8(demo_p,(UINT8)(((skin_t *)metal->player->followmobj->skin)->skinnum));
			oldmetal.flags2 |= MF2_AMBUSH;
		}

		if (metal->player->followmobj->scale != metal->scale)
		{
			followtic |= FZT_SCALE;
			WRITEFIXED(demo_p,metal->player->followmobj->scale);
		}

		temp = metal->player->followmobj->x-metal->x;
		WRITEFIXED(demo_p,temp);
		temp = metal->player->followmobj->y-metal->y;
		WRITEFIXED(demo_p,temp);
		temp = metal->player->followmobj->z-metal->z;
		WRITEFIXED(demo_p,temp);
		if (followtic & FZT_SKIN)
			WRITEUINT16(demo_p,metal->player->followmobj->sprite2);
		WRITEUINT16(demo_p,metal->player->followmobj->sprite);
		WRITEUINT32(demo_p,metal->player->followmobj->frame);  
		WRITEUINT16(demo_p,metal->player->followmobj->color);

		*followtic_p = followtic;
	}
	else
		oldmetal.flags2 &= ~MF2_AMBUSH;

	*ziptic_p = ziptic;



	if (demo_p >= demoend - 32)
	{
		G_StopMetalRecording(false);  
		return;
	}
}




void G_RecordDemo(const char *name)
{
	INT32 maxsize;

	strcpy(demoname, name);
	strcat(demoname, ".lmp");
	maxsize = 1024*1024;
	if (M_CheckParm("-maxdemo") && M_IsNextParm())
		maxsize = atoi(M_GetNextParm()) * 1024;


	demo_p = NULL;
	demobuffer = malloc(maxsize);
	demoend = demobuffer + maxsize;

	demorecording = true;
}

void G_RecordMetal(void)
{
	INT32 maxsize;
	maxsize = 1024*1024;
	if (M_CheckParm("-maxdemo") && M_IsNextParm())
		maxsize = atoi(M_GetNextParm()) * 1024;
	demo_p = NULL;
	demobuffer = malloc(maxsize);
	demoend = demobuffer + maxsize;
	metalrecording = true;
}

void G_BeginRecording(void)
{
	UINT8 i;
	char name[MAXCOLORNAME+1];
	player_t *player = &players[consoleplayer];

	char *filename;
	UINT16 totalfiles;
	UINT8 *m;
	save_t savebuffer;

	if (demo_p)
		return;
	memset(name,0,sizeof(name));

	demo_p = demobuffer;
	demoflags = DF_GHOST|(modeattacking<<DF_ATTACKSHIFT);


	M_Memcpy(demo_p, DEMOHEADER, 12); demo_p += 12;
	WRITEUINT8(demo_p,VERSION);
	WRITEUINT8(demo_p,SUBVERSION);
	WRITEUINT16(demo_p,DEMOVERSION);


	demo_p += 16;


	M_Memcpy(demo_p, "PLAY", 4); demo_p += 4;
	WRITEINT16(demo_p,gamemap);
	M_Memcpy(demo_p, mapmd5, 16); demo_p += 16;

	WRITEUINT8(demo_p,demoflags);


	m = demo_p; 
	demo_p += 2;

	totalfiles = 0;
	for (i = mainwads; ++i < numwadfiles; )
	{
		if (wadfiles[i]->important)
		{
			nameonly(( filename = va("%s", wadfiles[i]->filename) ));
			WRITESTRINGL(demo_p, filename, MAX_WADPATH);
			WRITEMEM(demo_p, wadfiles[i]->md5sum, 16);

			totalfiles++;
		}
	}

	WRITEUINT16(m, totalfiles);

	switch ((demoflags & DF_ATTACKMASK)>>DF_ATTACKSHIFT)
	{
		case ATTACKING_NONE:  
			break;
		case ATTACKING_RECORD:  
			demotime_p = demo_p;
			WRITEUINT32(demo_p,UINT32_MAX);  
			WRITEUINT32(demo_p,0);  
			WRITEUINT16(demo_p,0);  
			break;
		case ATTACKING_NIGHTS:  
			demotime_p = demo_p;
			WRITEUINT32(demo_p,UINT32_MAX);  
			WRITEUINT32(demo_p,0);  
			break;
		default:  
			break;
	}

	WRITEUINT32(demo_p,P_GetInitSeed());


	for (i = 0; i < 16 && cv_playername.string[i]; i++)
		name[i] = cv_playername.string[i];
	for (; i < 16; i++)
		name[i] = '\0';
	M_Memcpy(demo_p,name,16);
	demo_p += 16;


	const char *skinname = skins[players[0].skin]->name;
	for (i = 0; i < 16 && skinname[i]; i++)
		name[i] = skinname[i];
	for (; i < 16; i++)
		name[i] = '\0';
	M_Memcpy(demo_p,name,16);
	demo_p += 16;


	UINT16 skincolor = players[0].skincolor;
	if (skincolor >= numskincolors)
		skincolor = SKINCOLOR_NONE;
	const char *skincolor_name = skincolors[skincolor].name;
	for (i = 0; i < MAXCOLORNAME && skincolor_name[i]; i++)
		name[i] = skincolor_name[i];
	for (; i < MAXCOLORNAME; i++)
		name[i] = '\0';
	M_Memcpy(demo_p,name,MAXCOLORNAME);
	demo_p += MAXCOLORNAME;


	WRITEUINT8(demo_p,player->charability);
	WRITEUINT8(demo_p,player->charability2);
	WRITEFIXED(demo_p,player->actionspd);
	WRITEFIXED(demo_p,player->mindash);
	WRITEFIXED(demo_p,player->maxdash);
	WRITEFIXED(demo_p,player->normalspeed);
	WRITEFIXED(demo_p,player->runspeed);
	WRITEUINT8(demo_p,player->thrustfactor);
	WRITEUINT8(demo_p,player->accelstart);
	WRITEUINT8(demo_p,player->acceleration);
	WRITEFIXED(demo_p,player->height);
	WRITEFIXED(demo_p,player->spinheight);
	WRITEFIXED(demo_p,player->camerascale);
	WRITEFIXED(demo_p,player->shieldscale);



	WRITEFIXED(demo_p, player->jumpfactor);


	WRITEUINT32(demo_p, player->followitem);


	{
		UINT8 buf = 0;
		pflags_t pflags = 0;
		if (cv_flipcam.value)
		{
			buf |= 0x01;
			pflags |= PF_FLIPCAM;
		}
		if (cv_analog[0].value)
		{
			buf |= 0x02;
			pflags |= PF_ANALOGMODE;
		}
		if (cv_directionchar[0].value)
		{
			buf |= 0x04;
			pflags |= PF_DIRECTIONCHAR;
		}
		if (cv_autobrake.value)
		{
			buf |= 0x08;
			pflags |= PF_AUTOBRAKE;
		}
		if (cv_usejoystick.value)
			buf |= 0x10;
		CV_SetValue(&cv_showinputjoy, !!(cv_usejoystick.value));

		WRITEUINT8(demo_p,buf);
		player->pflags = pflags;
	}


	savebuffer.buf = demo_p;
	savebuffer.size = demoend - demo_p;
	savebuffer.pos = 0;
	CV_SaveDemoVars(&savebuffer);
	demo_p = &savebuffer.buf[savebuffer.pos];

	memset(&oldcmd,0,sizeof(oldcmd));
	memset(&oldghost,0,sizeof(oldghost));
	memset(&ghostext,0,sizeof(ghostext));
	ghostext.lastcolor = ghostext.color = GHC_NORMAL;
	ghostext.lastscale = ghostext.scale = FRACUNIT;

	if (player->mo)
	{
		oldghost.x = player->mo->x;
		oldghost.y = player->mo->y;
		oldghost.z = player->mo->z;
		oldghost.angle = player->mo->angle>>24;


		if (player->mo->eflags & MFE_VERTICALFLIP)
			ghostext.flags |= EZT_FLIP;
	}
}

void G_BeginMetal(void)
{
	mobj_t *mo = players[consoleplayer].mo;

#if 0
	if (demo_p)
		return;
#endif

	demo_p = demobuffer;


	M_Memcpy(demo_p, DEMOHEADER, 12); demo_p += 12;
	WRITEUINT8(demo_p,VERSION);
	WRITEUINT8(demo_p,SUBVERSION);
	WRITEUINT16(demo_p,DEMOVERSION);


	demo_p += 16;

	M_Memcpy(demo_p, "METL", 4); demo_p += 4;

	memset(&ghostext,0,sizeof(ghostext));
	ghostext.lastscale = ghostext.scale = FRACUNIT;


	memset(&oldmetal,0,sizeof(oldmetal));
	oldmetal.x = mo->x;
	oldmetal.y = mo->y;
	oldmetal.z = mo->z;
	oldmetal.angle = mo->angle>>24;
}

static void G_LoadDemoExtraFiles(UINT8 **pp, UINT16 this_demo_version)
{
	UINT16 totalfiles;
	char filename[MAX_WADPATH];
	UINT8 md5sum[16];
	filestatus_t ncs = FS_NOTFOUND;
	boolean toomany = false;
	boolean alreadyloaded;
	UINT16 i, j;

	if (this_demo_version < 0x0010)
	{

		return;
	}

	totalfiles = READUINT16((*pp));
	for (i = 0; i < totalfiles; ++i)
	{
		if (toomany)
			SKIPSTRING((*pp));
		else
		{
			strlcpy(filename, (char *)(*pp), sizeof filename);
			SKIPSTRING((*pp));
		}
		READMEM((*pp), md5sum, 16);

		if (!toomany)
		{
			alreadyloaded = false;

			for (j = 0; j < numwadfiles; ++j)
			{
				if (memcmp(md5sum, wadfiles[j]->md5sum, 16) == 0)
				{
					alreadyloaded = true;
					break;
				}
			}

			if (alreadyloaded)
				continue;

			if (numwadfiles >= MAX_WADFILES)
				toomany = true;
			else
				ncs = findfile(filename, md5sum, false);

			if (toomany)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Too many files loaded to add anymore for demo playback\n"));
				if (!CON_Ready())
					M_StartMessage(M_GetText("There are too many files loaded to add this demo's addons.\n\nDemo playback may desync.\n\nPress ESC\n"), NULL, MM_NOTHING);
			}
			else if (ncs != FS_FOUND)
			{
				if (ncs == FS_NOTFOUND)
					CONS_Alert(CONS_NOTICE, M_GetText("You do not have a copy of %s\n"), filename);
				else if (ncs == FS_MD5SUMBAD)
					CONS_Alert(CONS_NOTICE, M_GetText("Checksum mismatch on %s\n"), filename);
				else
					CONS_Alert(CONS_NOTICE, M_GetText("Unknown error finding file %s\n"), filename);

				if (!CON_Ready())
					M_StartMessage(M_GetText("There were errors trying to add this demo's addons. Check the console for more information.\n\nDemo playback may desync.\n\nPress ESC\n"), NULL, MM_NOTHING);
			}
			else
			{
				P_AddWadFile(filename, false);
			}
		}
	}
}

static void G_SkipDemoExtraFiles(UINT8 **pp, UINT16 this_demo_version)
{
	UINT16 totalfiles;
	UINT16 i;

	if (this_demo_version < 0x0010)
	{

		return;
	}

	totalfiles = READUINT16((*pp));
	for (i = 0; i < totalfiles; ++i)
	{
		SKIPSTRING((*pp)); 
		(*pp) += 16; 
	}
}



static UINT8 G_CheckDemoExtraFiles(UINT8 **pp, boolean quick, UINT16 this_demo_version)
{
	UINT16 totalfiles, filesloaded, nmusfilecount;
	char filename[MAX_WADPATH];
	UINT8 md5sum[16];
	boolean toomany = false;
	boolean alreadyloaded;
	UINT16 i, j;
	UINT8 error = DFILE_ERROR_NONE;

	if (this_demo_version < 0x0010)
	{

		return DFILE_ERROR_NONE;
	}

	totalfiles = READUINT16((*pp));
	filesloaded = 0;
	for (i = 0; i < totalfiles; ++i)
	{
		if (toomany)
			SKIPSTRING((*pp));
		else
		{
			strlcpy(filename, (char *)(*pp), sizeof filename);
			SKIPSTRING((*pp));
		}
		READMEM((*pp), md5sum, 16);

		if (!toomany)
		{
			alreadyloaded = false;
			nmusfilecount = 0;

			for (j = 0; j < numwadfiles; ++j)
			{
				if (wadfiles[j]->important && j > mainwads)
					nmusfilecount++;
				else
					continue;

				if (memcmp(md5sum, wadfiles[j]->md5sum, 16) == 0)
				{
					alreadyloaded = true;

					if (i != nmusfilecount-1 && error < DFILE_ERROR_OUTOFORDER)
						error |= DFILE_ERROR_OUTOFORDER;

					break;
				}
			}

			if (alreadyloaded)
			{
				filesloaded++;
				continue;
			}

			if (numwadfiles >= MAX_WADFILES)
				error = DFILE_ERROR_CANNOTLOAD;
			else if (!quick && findfile(filename, md5sum, false) != FS_FOUND)
				error = DFILE_ERROR_CANNOTLOAD;
			else if (error < DFILE_ERROR_INCOMPLETEOUTOFORDER)
				error |= DFILE_ERROR_NOTLOADED;
		} else
			error = DFILE_ERROR_CANNOTLOAD;
	}


	nmusfilecount = 0;

	for (j = 0; j < numwadfiles; ++j)
		if (wadfiles[j]->important && j > mainwads)
			nmusfilecount++;

	if (!error && filesloaded < nmusfilecount)
		error = DFILE_ERROR_EXTRAFILES;

	return error;
}

void G_SetDemoTime(UINT32 ptime, UINT32 pscore, UINT16 prings)
{
	if (!demorecording || !demotime_p)
		return;
	if (demoflags & DF_RECORDATTACK)
	{
		WRITEUINT32(demotime_p, ptime);
		WRITEUINT32(demotime_p, pscore);
		WRITEUINT16(demotime_p, prings);
		demotime_p = NULL;
	}
	else if (demoflags & DF_NIGHTSATTACK)
	{
		WRITEUINT32(demotime_p, ptime);
		WRITEUINT32(demotime_p, pscore);
		demotime_p = NULL;
	}
}





UINT8 G_CmpDemoTime(char *oldname, char *newname)
{
	UINT8 *buffer,*p;
	UINT8 flags;
	UINT32 oldtime, newtime, oldscore, newscore;
	UINT16 oldrings, newrings, oldversion, newversion;
	size_t bufsize ATTRUNUSED;
	UINT8 c;
	UINT8 aflags = 0;


	FIL_DefaultExtension(newname, ".lmp");
	bufsize = FIL_ReadFile(newname, &buffer);
	I_Assert(bufsize != 0);
	p = buffer;


	I_Assert(!memcmp(p, DEMOHEADER, 12));
	p += 12;  
	c = READUINT8(p);  
	I_Assert(c == VERSION);
	c = READUINT8(p);  
	I_Assert(c == SUBVERSION);
	newversion = READUINT16(p);
	I_Assert(newversion == DEMOVERSION);
	p += 16;  
	I_Assert(!memcmp(p, "PLAY", 4));
	p += 4;  
	p += 2;  
	p += 16;  
	flags = READUINT8(p);  
	G_SkipDemoExtraFiles(&p, newversion);
	aflags = flags & (DF_RECORDATTACK|DF_NIGHTSATTACK);
	I_Assert(aflags);
	if (flags & DF_RECORDATTACK)
	{
		newtime = READUINT32(p);
		newscore = READUINT32(p);
		newrings = READUINT16(p);
	}
	else if (flags & DF_NIGHTSATTACK)
	{
		newtime = READUINT32(p);
		newscore = READUINT32(p);
		newrings = 0;
	}
	else  
		return 0;

	Z_Free(buffer);


	FIL_DefaultExtension(oldname, ".lmp");
	if (!FIL_ReadFile(oldname, &buffer))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to read file '%s'.\n"), oldname);
		return UINT8_MAX;
	}
	p = buffer;


	if (memcmp(p, DEMOHEADER, 12))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' invalid format. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	} p += 12;  
	p++;  
	p++;  
	oldversion = READUINT16(p);
	if (oldversion < 0x000c || oldversion > DEMOVERSION)
	{

		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' invalid format. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	}
	p += 16;  
	if (memcmp(p, "PLAY", 4))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' invalid format. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	} p += 4;  
	if (oldversion <= 0x0008)
		p++;  
	else
		p += 2;  
	p += 16;  
	flags = READUINT8(p);
	G_SkipDemoExtraFiles(&p, oldversion);
	if (!(flags & aflags))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' not from same game mode. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	}
	if (flags & DF_RECORDATTACK)
	{
		oldtime = READUINT32(p);
		oldscore = READUINT32(p);
		oldrings = READUINT16(p);
	}
	else if (flags & DF_NIGHTSATTACK)
	{
		oldtime = READUINT32(p);
		oldscore = READUINT32(p);
		oldrings = 0;
	}
	else  
		return UINT8_MAX;

	Z_Free(buffer);

	c = 0;
	if (newtime < oldtime
	|| (newtime == oldtime && (newscore > oldscore || newrings > oldrings)))
		c |= 1;  
	if (newscore > oldscore
	|| (newscore == oldscore && newtime < oldtime))
		c |= 1<<1;  
	if (newrings > oldrings
	|| (newrings == oldrings && newtime < oldtime))
		c |= 1<<2;  
	return c;
}




void G_DeferedPlayDemo(const char *name)
{
	COM_BufAddText("playdemo \"");
	COM_BufAddText(name);
	COM_BufAddText("\"\n");
}




void G_DoPlayDemo(char *defdemoname)
{
	UINT8 i;
	lumpnum_t l;
	char skin[17],color[MAXCOLORNAME+1],*n,*pdemoname;
	UINT8 version,subversion,charability,charability2,thrustfactor,accelstart,acceleration;
	pflags_t pflags;
	UINT32 randseed, followitem;
	fixed_t camerascale,shieldscale,actionspd,mindash,maxdash,normalspeed,runspeed,jumpfactor,height,spinheight;
	char msg[1024];

	skin[16] = '\0';
	color[MAXCOLORNAME] = '\0';

	n = defdemoname+strlen(defdemoname);
	while (*n != '/' && *n != '\\' && n != defdemoname)
		n--;
	if (n != defdemoname)
		n++;
	pdemoname = ZZ_Alloc(strlen(n)+1);
	strcpy(pdemoname,n);


	if (FIL_CheckExtension(defdemoname))
	{

		if (!FIL_ReadFile(defdemoname, &demobuffer))
		{
			snprintf(msg, 1024, M_GetText("Failed to read file '%s'.\n"), defdemoname);
			CONS_Alert(CONS_ERROR, "%s", msg);
			gameaction = ga_nothing;
			M_StartMessage(msg, NULL, MM_NOTHING);
			return;
		}
		demo_p = demobuffer;
	}

	else if ((l = W_CheckNumForName(defdemoname)) == LUMPERROR)
	{
		snprintf(msg, 1024, M_GetText("Failed to read lump '%s'.\n"), defdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		gameaction = ga_nothing;
		M_StartMessage(msg, NULL, MM_NOTHING);
		return;
	}
	else  
		demobuffer = demo_p = W_CacheLumpNum(l, PU_STATIC);


	gameaction = ga_nothing;
	demoplayback = true;
	if (memcmp(demo_p, DEMOHEADER, 12))
	{
		snprintf(msg, 1024, M_GetText("%s is not a SRB2 replay file.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demoplayback = false;
		titledemo = false;
		return;
	}
	demo_p += 12;  

	version = READUINT8(demo_p);
	subversion = READUINT8(demo_p);
	demoversion = READUINT16(demo_p);
	demo_forwardmove_rng = (demoversion < 0x0010);
#ifdef OLD22DEMOCOMPAT
	if (demoversion < 0x000c || demoversion > DEMOVERSION)
#else
	if (demoversion < 0x000d || demoversion > DEMOVERSION)
#endif
	{

		snprintf(msg, 1024, M_GetText("%s is an incompatible replay format and cannot be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demoplayback = false;
		titledemo = false;
		return;
	}
	demo_p += 16;  
	if (memcmp(demo_p, "PLAY", 4))
	{
		snprintf(msg, 1024, M_GetText("%s is the wrong type of recording and cannot be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demoplayback = false;
		titledemo = false;
		return;
	}
	demo_p += 4;  
	gamemap = READINT16(demo_p);
	demo_p += 16;  

	demoflags = READUINT8(demo_p);

	if (titledemo)
	{

		G_SkipDemoExtraFiles(&demo_p, demoversion);
	}
	else if (demofileoverride == DFILE_OVERRIDE_LOAD)
	{
		G_LoadDemoExtraFiles(&demo_p, demoversion);
	}
	else if (demofileoverride == DFILE_OVERRIDE_SKIP)
	{
		G_SkipDemoExtraFiles(&demo_p, demoversion);
	}
	else
	{
		UINT8 error = G_CheckDemoExtraFiles(&demo_p, false, demoversion);

		if (error)
		{
			switch (error)
			{
				case DFILE_ERROR_NOTLOADED:
					snprintf(msg, 1024,
						M_GetText("Required files for this demo are not loaded.\n\nUse\n\"playdemo %s -addfiles\"\nto load them and play the demo.\n"),
					pdemoname);
					break;

				case DFILE_ERROR_OUTOFORDER:
					snprintf(msg, 1024,
						M_GetText("Required files for this demo are loaded out of order.\n\nUse\n\"playdemo %s -force\"\nto play the demo anyway.\n"),
					pdemoname);
					break;

				case DFILE_ERROR_INCOMPLETEOUTOFORDER:
					snprintf(msg, 1024,
						M_GetText("Required files for this demo are not loaded, and some are out of order.\n\nUse\n\"playdemo %s -addfiles\"\nto load needed files and play the demo.\n"),
					pdemoname);
					break;

				case DFILE_ERROR_CANNOTLOAD:
					snprintf(msg, 1024,
						M_GetText("Required files for this demo cannot be loaded.\n\nUse\n\"playdemo %s -force\"\nto play the demo anyway.\n"),
					pdemoname);
					break;

				case DFILE_ERROR_EXTRAFILES:
					snprintf(msg, 1024,
						M_GetText("You have additional files loaded beyond the demo's file list.\n\nUse\n\"playdemo %s -force\"\nto play the demo anyway.\n"),
					pdemoname);
					break;
			}

			CONS_Alert(CONS_ERROR, "%s", msg);
			M_StartMessage(msg, NULL, MM_NOTHING);
			Z_Free(pdemoname);
			Z_Free(demobuffer);
			demoplayback = false;
			titledemo = false;
			return;
		}
	}

	modeattacking = (demoflags & DF_ATTACKMASK)>>DF_ATTACKSHIFT;
	CON_ToggleOff();

	hu_demoscore = 0;
	hu_demotime = UINT32_MAX;
	hu_demorings = 0;

	switch (modeattacking)
	{
	case ATTACKING_NONE:  
		break;
	case ATTACKING_RECORD:  
		hu_demotime  = READUINT32(demo_p);
		hu_demoscore = READUINT32(demo_p);
		hu_demorings = READUINT16(demo_p);
		break;
	case ATTACKING_NIGHTS:  
		hu_demotime  = READUINT32(demo_p);
		hu_demoscore = READUINT32(demo_p);
		break;
	default:  
		modeattacking = ATTACKING_NONE;
		break;
	}


	randseed = READUINT32(demo_p);


	M_Memcpy(player_names[0],demo_p,16);
	demo_p += 16;


	M_Memcpy(skin,demo_p,16);
	demo_p += 16;


	M_Memcpy(color, demo_p, (demoversion < 0x000d) ? 16 : MAXCOLORNAME);
	demo_p += (demoversion < 0x000d) ? 16 : MAXCOLORNAME;

	charability = READUINT8(demo_p);
	charability2 = READUINT8(demo_p);
	actionspd = (demoversion < 0x0010) ? (fixed_t)READUINT8(demo_p)<<FRACBITS : READFIXED(demo_p);
	mindash = (demoversion < 0x0010) ? (fixed_t)READUINT8(demo_p)<<FRACBITS : READFIXED(demo_p);
	maxdash = (demoversion < 0x0010) ? (fixed_t)READUINT8(demo_p)<<FRACBITS : READFIXED(demo_p);
	normalspeed = (demoversion < 0x0010) ? (fixed_t)READUINT8(demo_p)<<FRACBITS : READFIXED(demo_p);
	runspeed = (demoversion < 0x0010) ? (fixed_t)READUINT8(demo_p)<<FRACBITS : READFIXED(demo_p);
	thrustfactor = READUINT8(demo_p);
	accelstart = READUINT8(demo_p);
	acceleration = READUINT8(demo_p);
	height = (demoversion < 0x000e) ? (fixed_t)READUINT8(demo_p)<<FRACBITS : READFIXED(demo_p);
	spinheight = (demoversion < 0x000e) ? (fixed_t)READUINT8(demo_p)<<FRACBITS : READFIXED(demo_p);
	camerascale = (demoversion < 0x0010) ? (fixed_t)READUINT8(demo_p)<<FRACBITS : READFIXED(demo_p);
	shieldscale = (demoversion < 0x0010) ? (fixed_t)READUINT8(demo_p)<<FRACBITS : READFIXED(demo_p);
	jumpfactor = READFIXED(demo_p);
	followitem = READUINT32(demo_p);


	{
		UINT8 buf = READUINT8(demo_p);
		pflags = 0;
		if (buf & 0x01)
			pflags |= PF_FLIPCAM;
		if (buf & 0x02)
			pflags |= PF_ANALOGMODE;
		if (buf & 0x04)
			pflags |= PF_DIRECTIONCHAR;
		if (buf & 0x08)
			pflags |= PF_AUTOBRAKE;
		CV_SetValue(&cv_showinputjoy, !!(buf & 0x10));
	}


#ifdef OLD22DEMOCOMPAT
	if (demoversion < 0x000d)
	{
		save_t savebuffer;
		savebuffer.buf = demo_p;
		savebuffer.size = demoend - demo_p;
		savebuffer.pos = 0;
		CV_LoadOldDemoVars(&savebuffer);
		demo_p = &savebuffer.buf[savebuffer.pos];
	}
	else
#endif
	{
		save_t savebuffer;
		savebuffer.buf = demo_p;
		savebuffer.size = demoend - demo_p;
		savebuffer.pos = 0;
		CV_LoadDemoVars(&savebuffer);
		demo_p = &savebuffer.buf[savebuffer.pos];
	}


	if (*demo_p == DEMOMARKER)
	{
		snprintf(msg, 1024, M_GetText("%s contains no data to be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demoplayback = false;
		titledemo = false;
		return;
	}

	Z_Free(pdemoname);

	memset(&oldcmd,0,sizeof(oldcmd));
	memset(&oldghost,0,sizeof(oldghost));

	if (VERSION != version || SUBVERSION != subversion)
		CONS_Alert(CONS_WARNING, M_GetText("Demo version does not match game version. Desyncs may occur.\n"));


	demo_start = false;


	SetPlayerSkin(0, skin);

	LUA_HookInt(gamemap, HOOK(MapChange));
	displayplayer = consoleplayer = 0;
	memset(playeringame,0,sizeof(playeringame));
	playeringame[0] = true;
	P_SetRandSeed(randseed);
	G_InitNew(false, G_BuildMapName(gamemap), true, true, false);


	players[0].skincolor = skins[players[0].skin]->prefcolor;
	for (i = 0; i < numskincolors; i++)
		if (!stricmp(skincolors[i].name,color))
		{
			players[0].skincolor = i;
			break;
		}
	if (players[0].mo)
	{
		players[0].mo->color = players[0].skincolor;
		oldghost.x = players[0].mo->x;
		oldghost.y = players[0].mo->y;
		oldghost.z = players[0].mo->z;
	}




	players[0].camerascale = camerascale;
	players[0].shieldscale = shieldscale;
	players[0].charability = charability;
	players[0].charability2 = charability2;
	players[0].actionspd = actionspd;
	players[0].mindash = mindash;
	players[0].maxdash = maxdash;
	players[0].normalspeed = normalspeed;
	players[0].runspeed = runspeed;
	players[0].thrustfactor = thrustfactor;
	players[0].accelstart = accelstart;
	players[0].acceleration = acceleration;
	players[0].height = height;
	players[0].spinheight = spinheight;
	players[0].jumpfactor = jumpfactor;
	players[0].followitem = followitem;
	players[0].pflags = pflags;

	demo_start = true;
}




UINT8 G_CheckDemoForError(char *defdemoname)
{
	lumpnum_t l;
	char *n,*pdemoname;
	UINT16 our_demo_version;

	if (titledemo)
	{

		return DFILE_ERROR_NONE;
	}

	n = defdemoname+strlen(defdemoname);
	while (*n != '/' && *n != '\\' && n != defdemoname)
		n--;
	if (n != defdemoname)
		n++;
	pdemoname = ZZ_Alloc(strlen(n)+1);
	strcpy(pdemoname,n);


	if (FIL_CheckExtension(defdemoname))
	{

		if (!FIL_ReadFile(defdemoname, &demobuffer))
		{
			return DFILE_ERROR_NOTDEMO;
		}
		demo_p = demobuffer;
	}

	else if ((l = W_CheckNumForName(defdemoname)) == LUMPERROR)
	{
		return DFILE_ERROR_NOTDEMO;
	}
	else  
	{
		demobuffer = demo_p = W_CacheLumpNum(l, PU_STATIC);
	}


	if (memcmp(demo_p, DEMOHEADER, 12))
	{
		return DFILE_ERROR_NOTDEMO;
	}
	demo_p += 12;  

	demo_p++;  
	demo_p++;  
	our_demo_version = READUINT16(demo_p);
#ifdef OLD22DEMOCOMPAT
	if (our_demo_version < 0x000c || our_demo_version > DEMOVERSION)
#else
	if (our_demo_version < 0x000d || our_demo_version > DEMOVERSION)
#endif
	{

		return DFILE_ERROR_NOTDEMO;
	}
	demo_p += 16;  
	if (memcmp(demo_p, "PLAY", 4))
	{
		return DFILE_ERROR_NOTDEMO;
	}
	demo_p += 4;  
	demo_p += 2;  
	demo_p += 16;  

	demo_p++;  

	return G_CheckDemoExtraFiles(&demo_p, true, our_demo_version);
}

void G_AddGhost(char *defdemoname)
{
	INT32 i;
	lumpnum_t l;
	char name[17],skin[17],color[MAXCOLORNAME+1],*n,*pdemoname,md5[16];
	demoghost *gh;
	UINT8 flags, subversion;
	UINT8 *buffer,*p;
	mapthing_t *mthing;
	UINT16 count, ghostversion;

	name[16] = '\0';
	skin[16] = '\0';
	color[16] = '\0';

	n = defdemoname+strlen(defdemoname);
	while (*n != '/' && *n != '\\' && n != defdemoname)
		n--;
	if (n != defdemoname)
		n++;
	pdemoname = ZZ_Alloc(strlen(n)+1);
	strcpy(pdemoname,n);


	if (FIL_CheckExtension(defdemoname))
	{

		if (!FIL_ReadFileTag(defdemoname, &buffer, PU_LEVEL))
		{
			CONS_Alert(CONS_ERROR, M_GetText("Failed to read file '%s'.\n"), defdemoname);
			Z_Free(pdemoname);
			return;
		}
		p = buffer;
	}

	else if ((l = W_CheckNumForName(defdemoname)) == LUMPERROR)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to read lump '%s'.\n"), defdemoname);
		Z_Free(pdemoname);
		return;
	}
	else  
		buffer = p = W_CacheLumpNum(l, PU_LEVEL);


	if (memcmp(p, DEMOHEADER, 12))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Not a SRB2 replay.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	} p += 12;  
	p++;  
	subversion = READUINT8(p);  
	ghostversion = READUINT16(p);
	if (ghostversion < 0x000c || ghostversion > DEMOVERSION)
	{

		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Demo version incompatible.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}
	M_Memcpy(md5, p, 16); p += 16;  
	for (gh = ghosts; gh; gh = gh->next)
		if (!memcmp(md5, gh->checksum, 16))  
		{  
			CONS_Debug(DBG_SETUP, "Rejecting duplicate ghost %s (MD5 was matched)\n", pdemoname);
			Z_Free(pdemoname);
			Z_Free(buffer);
			return;
		}
	if (memcmp(p, "PLAY", 4))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Demo format unacceptable.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	} p += 4;  
	if (ghostversion <= 0x0008)
		p++;  
	else
		p += 2;  
	p += 16;  
	flags = READUINT8(p);
	if (!(flags & DF_GHOST))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: No ghost data in this demo.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}

	G_SkipDemoExtraFiles(&p, ghostversion);  

	switch ((flags & DF_ATTACKMASK)>>DF_ATTACKSHIFT)
	{
	case ATTACKING_NONE:  
		break;
	case ATTACKING_RECORD:  
		p += 10;  
		break;
	case ATTACKING_NIGHTS:  
		p += 8;  
		break;
	default:  
		break;
	}

	p += 4;  


	M_Memcpy(name, p,16);
	p += 16;


	M_Memcpy(skin, p,16);
	p += 16;


	M_Memcpy(color, p, (ghostversion < 0x000d) ? 16 : MAXCOLORNAME);
	p += (ghostversion < 0x000d) ? 16 : MAXCOLORNAME;


	p++;  
	p++;  
	p += (ghostversion < 0x0010) ? 5 : 5 * sizeof(fixed_t);  
	p++;  
	p++;  
	p++;  
	p += (ghostversion < 0x000e) ? 2 : 2 * sizeof(fixed_t);  
	p += (ghostversion < 0x0010) ? 2 : 2 * sizeof(fixed_t);  
	p += 4;  
	p += 4;  

	p++;  


	count = READUINT16(p);
	while (count--)
	{

		if (subversion < 7)
		{
			p += 2;
			SKIPSTRING(p);
			p++;
		}
		else
		{
			SKIPSTRING(p);
			SKIPSTRING(p);
			p++;
		}
	}

	if (*p == DEMOMARKER)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Failed to add ghost %s: Replay is empty.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}

	gh = Z_Calloc(sizeof(demoghost), PU_LEVEL, NULL);
	gh->next = ghosts;
	gh->buffer = buffer;
	M_Memcpy(gh->checksum, md5, 16);
	gh->p = p;

	ghosts = gh;

	gh->version = ghostversion;
	mthing = playerstarts[0];
	I_Assert(mthing);
	{  
		fixed_t z,f,c;
		fixed_t offset = mthing->z << FRACBITS;
		P_SetTarget(&gh->mo, P_SpawnMobj(mthing->x << FRACBITS, mthing->y << FRACBITS, 0, MT_GHOST));
		if (P_MobjWasRemoved(gh->mo))
			return;
		gh->mo->angle = FixedAngle(mthing->angle << FRACBITS);
		f = gh->mo->floorz;
		c = gh->mo->ceilingz - mobjinfo[MT_PLAYER].height;
		if (!!(mthing->args[0]) ^ !!(mthing->options & MTF_OBJECTFLIP))
		{
			z = c - offset;
			if (z < f)
				z = f;
		}
		else
		{
			z = f + offset;
			if (z > c)
				z = c;
		}
		gh->mo->z = z;
	}

	gh->oldmo.x = gh->mo->x;
	gh->oldmo.y = gh->mo->y;
	gh->oldmo.z = gh->mo->z;


	gh->mo->skin = skins[0];
	for (i = 0; i < numskins; i++)
		if (!stricmp(skins[i]->name,skin))
		{
			gh->mo->skin = skins[i];
			break;
		}
	gh->oldmo.skin = gh->mo->skin;


	gh->mo->color = ((skin_t*)gh->mo->skin)->prefcolor;
	for (i = 0; i < numskincolors; i++)
		if (!stricmp(skincolors[i].name,color))
		{
			gh->mo->color = (UINT16)i;
			break;
		}
	gh->oldmo.color = gh->mo->color;

	gh->mo->state = &states[S_PLAY_STND];
	gh->mo->sprite = gh->mo->state->sprite;
	gh->mo->sprite2 = P_GetStateSprite2(gh->mo->state);
	gh->mo->frame = (gh->mo->state->frame & ~FF_FRAMEMASK) | P_GetSprite2StateFrame(gh->mo->state);
	gh->mo->flags2 |= MF2_DONTDRAW;
	gh->fadein = (9-3)*6;  
	gh->mo->tics = -1;

	CONS_Printf(M_GetText("Added ghost %s from %s\n"), name, pdemoname);
	Z_Free(pdemoname);
}


void G_FreeGhosts(void)
{
	while (ghosts)
	{
		demoghost *next = ghosts->next;
		Z_Free(ghosts);
		ghosts = next;
	}
	ghosts = NULL;
}





static INT32 restorecv_vidwait;

void G_TimeDemo(const char *name)
{
	nodrawers = M_CheckParm("-nodraw");
	noblit = M_CheckParm("-noblit");
	restorecv_vidwait = cv_vidwait.value;
	if (cv_vidwait.value)
		CV_Set(&cv_vidwait, "0");
	timingdemo = true;
	singletics = true;
	framecount = 0;
	demostarttime = I_GetTime();
	G_DeferedPlayDemo(name);
}

void G_DoPlayMetal(void)
{
	lumpnum_t l;
	mobj_t *mo = NULL;
	thinker_t *th;


	if ((l = W_CheckNumForName(va("%sMS",G_BuildMapName(gamemap)))) == LUMPERROR)
	{
		CONS_Alert(CONS_WARNING, M_GetText("No bot recording for this map.\n"));
		return;
	}
	else
		metalbuffer = metal_p = W_CacheLumpNum(l, PU_STATIC);


	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mo = (mobj_t *)th;
		if (mo->type != MT_METALSONIC_RACE)
			continue;

		break;
	}
	if (th == &thlist[THINK_MOBJ])
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to find bot entity.\n"));
		Z_Free(metalbuffer);
		return;
	}


    metal_p += 12;  
	metal_p++;  
	metal_p++;  
	metalversion = READUINT16(metal_p);
	if (metalversion < 0x000c || metalversion > DEMOVERSION)
	{

		CONS_Alert(CONS_WARNING, M_GetText("Failed to load bot recording for this map, format version incompatible.\n"));
		Z_Free(metalbuffer);
		return;
	}
	metal_p += 16;  
	if (memcmp(metal_p, "METL", 4))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Failed to load bot recording for this map, wasn't recorded in Metal format.\n"));
		Z_Free(metalbuffer);
		return;
	} metal_p += 4;  


	memset(&oldmetal,0,sizeof(oldmetal));
	oldmetal.x = mo->x;
	oldmetal.y = mo->y;
	oldmetal.z = mo->z;
	metalplayback = mo;
}

void G_DoneLevelLoad(void)
{
	CONS_Printf(M_GetText("Loaded level in %f sec\n"), (double)(I_GetTime() - demostarttime) / TICRATE);
	framecount = 0;
	demostarttime = I_GetTime();
}

  
                   
 
                   
 
                                                                          
                                                        
                   
  


static void WriteDemoChecksum(void)
{
	UINT8 *p = demobuffer+16;  
#ifdef NOMD5
	UINT8 i;
	for (i = 0; i < 16; i++, p++)
		*p = P_RandomByte();  
#else
	md5_buffer((char *)p+16, demo_p - (p+16), p);  
#endif
}


static void G_StopDemoRecording(void)
{
	boolean saved = false;
	if (demo_p)
	{
		WRITEUINT8(demo_p, DEMOMARKER);  
		WriteDemoChecksum();
		saved = FIL_WriteFile(va(pandf, srb2home, demoname), demobuffer, demo_p - demobuffer);  
	}
	free(demobuffer);
	demorecording = false;

	if (modeattacking != ATTACKING_RECORD)
	{
		if (saved)
			CONS_Printf(M_GetText("Demo %s recorded\n"), demoname);
		else
			CONS_Alert(CONS_WARNING, M_GetText("Demo %s not saved\n"), demoname);
	}
}


void G_StopMetalDemo(void)
{


	Z_Free(metalbuffer);
	metalbuffer = NULL;
	metalplayback = NULL;
	metal_p = NULL;
}


ATTRNORETURN void FUNCNORETURN G_StopMetalRecording(boolean kill)
{
	boolean saved = false;
	if (demo_p)
	{
		WRITEUINT8(demo_p, (kill) ? METALDEATH : DEMOMARKER);  
		WriteDemoChecksum();
		sprintf(demoname, "%sMS.LMP", G_BuildMapName(gamemap));
		saved = FIL_WriteFile(va(pandf, srb2home, demoname), demobuffer, demo_p - demobuffer);  
	}
	free(demobuffer);
	metalrecording = false;
	if (saved)
		I_Error("Saved to %s", demoname);
	I_Error("Failed to save demo!");
}


static void G_StopTimingDemo(void)
{
	INT32 demotime;
	double f1, f2;
	demotime = I_GetTime() - demostarttime;
	if (!demotime)
		return;
	G_StopDemo();
	timingdemo = false;
	f1 = (double)demotime;
	f2 = (double)framecount*TICRATE;

	CONS_Printf(M_GetText("timed %u gametics in %d realtics - %u frames\n%f seconds, %f avg fps\n"),
		leveltime,demotime,(UINT32)framecount,f1/TICRATE,f2/f1);


	if (timedemo_csv)
	{
		FILE *f;
		const char *csvpath = va("%s"PATHSEP"%s", srb2home, "timedemo.csv");
		const char *header = "id,demoname,seconds,avgfps,leveltime,demotime,framecount,ticrate,rendermode,vidmode,vidwidth,vidheight,procbits\n";
		const char *rowformat = "\"%s\",\"%s\",%f,%f,%u,%d,%u,%u,%u,%u,%u,%u,%u\n";
		boolean headerrow = !FIL_FileExists(csvpath);
		UINT8 procbits = 0;


		if (sizeof(void*) == 4)
			procbits = 32;
		else if (sizeof(void*) == 8)
			procbits = 64;

		f = fopen(csvpath, "a+");

		if (f)
		{
			if (headerrow)
				fputs(header, f);
			fprintf(f, rowformat,
				timedemo_csv_id,timedemo_name,f1/TICRATE,f2/f1,leveltime,demotime,(UINT32)framecount,TICRATE,rendermode,vid.modenum,vid.width,vid.height,procbits);
			fclose(f);
			CONS_Printf("Timedemo results saved to '%s'\n", csvpath);
		}
		else
		{

			CON_LogMessage(header);
			CONS_Printf(rowformat,
				timedemo_csv_id,timedemo_name,f1/TICRATE,f2/f1,leveltime,demotime,(UINT32)framecount,TICRATE,rendermode,vid.modenum,vid.width,vid.height,procbits);
		}
	}

	if (restorecv_vidwait != cv_vidwait.value)
		CV_SetValue(&cv_vidwait, restorecv_vidwait);
	D_AdvanceDemo();
}



void G_StopDemo(void)
{
	Z_Free(demobuffer);
	demobuffer = NULL;
	demoplayback = false;
	titledemo = false;
	timingdemo = false;
	singletics = false;

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission();  

	G_SetGamestate(GS_NULL);
	wipegamestate = GS_NULL;
	SV_StopServer();
	SV_ResetServer();
}

boolean G_CheckDemoStatus(void)
{
	G_FreeGhosts();



	if (timingdemo)
	{
		G_StopTimingDemo();
		return true;
	}

	if (demoplayback)
	{
		if (singledemo)
			I_Quit();
		G_StopDemo();

		if (modeattacking)
			M_EndModeAttackRun();
		else
			D_AdvanceDemo();

		return true;
	}

	if (demorecording)
	{
		G_StopDemoRecording();
		return true;
	}

	return false;
}


INT32 G_ConvertOldFrameFlags(INT32 frame)
{
	if (frame & 0x01000000)  
	{
		frame &= ~0x01000000;
		frame |= FF_ANIMATE;
	}

	if (frame & 0x02000000)  
	{
		frame &= ~0x02000000;
		frame |= FF_RANDOMANIM;
	}

	if (frame & 0x04000000)  
	{
		frame &= ~0x04000000;
		frame |= FF_GLOBALANIM;
	}

	if (frame & 0x00200000)  
	{
		frame &= ~0x00200000;
		frame |= FF_VERTICALFLIP;
	}

	if (frame & 0x00400000)  
	{
		frame &= ~0x00400000;
		frame |= FF_HORIZONTALFLIP;
	}

	if (frame & 0x00800000)  
	{
		frame &= ~0x00800000;
		frame |= FF_PAPERSPRITE;
	}

	return frame;
}
