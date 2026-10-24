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
#include "i_system.h"
#include "d_event.h"
#include "netcode/d_net.h"
#include "netcode/net_command.h"
#include "g_demo.h"  
#include "g_game.h"
#include "p_local.h"
#include "r_fps.h"
#include "r_main.h"
#include "s_sound.h"
#include "r_skins.h"
#include "d_think.h"
#include "r_sky.h"
#include "p_setup.h"
#include "m_random.h"
#include "m_misc.h"
#include "i_video.h"
#include "p_slopes.h"
#include "p_spec.h"
#include "r_splats.h"
#include "z_zone.h"
#include "w_wad.h"
#include "hu_stuff.h"

#include "st_stuff.h"
#include "lua_script.h"
#include "lua_hook.h"
#include "b_bot.h"

#include "m_cheat.h"

#include "g_input.h"

#ifdef HW3SOUND
#include "hardware/hw3sound.h"
#endif

#ifdef HWRENDER
#include "hardware/hw_light.h"
#include "hardware/hw_main.h"
#endif

#if 0
static void P_NukeAllPlayers(player_t *player);
#endif





jingle_t jingleinfo[NUMJINGLES] = {

	{""        , false},  
	{""        , false},  
	{""        , false},  
	{"_1up"    , false},
	{"_shoes"  ,  true},
	{"_inv"    , false},
	{"_minv"   , false},
	{"_drown"  , false},
	{"_super"  ,  true},
	{"_gover"  , false},
	{"_ntime"  , false},   
	{"_drown"  , false}    



};






#define MAXBOB (0x10 << FRACBITS)

static boolean onground;





void P_Thrust(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx += FixedMul(move, FINECOSINE(angle));

	if (!(twodlevel || (mo->flags2 & MF2_TWOD)))
		mo->momy += FixedMul(move, FINESINE(angle));
}

void P_ThrustEvenIn2D(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx += FixedMul(move, FINECOSINE(angle));
	mo->momy += FixedMul(move, FINESINE(angle));
}

void P_VectorInstaThrust(fixed_t xa, fixed_t xb, fixed_t xc, fixed_t ya, fixed_t yb, fixed_t yc,
	fixed_t za, fixed_t zb, fixed_t zc, fixed_t momentum, mobj_t *mo)
{
	fixed_t a1, b1, c1, a2, b2, c2, i, j, k;

	a1 = xb - xa;
	b1 = yb - ya;
	c1 = zb - za;
	a2 = xb - xc;
	b2 = yb - yc;
	c2 = zb - zc;
  
                              
                                                                                  
                                                                                  
                                                                                  

                                                                                  
                                                                                  
                                                                                  
  

	i = FixedMul(momentum, FixedMul(b1, c2) - FixedMul(c1, b2));
	j = FixedMul(momentum, FixedMul(c1, a2) - FixedMul(a1, c2));
	k = FixedMul(momentum, FixedMul(a1, b2) - FixedMul(a1, c2));

	mo->momx = i;
	mo->momy = j;
	mo->momz = k;
}







void P_InstaThrust(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx = FixedMul(move, FINECOSINE(angle));

	if (!(twodlevel || (mo->flags2 & MF2_TWOD)))
		mo->momy = FixedMul(move,FINESINE(angle));
}

void P_InstaThrustEvenIn2D(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx = FixedMul(move, FINECOSINE(angle));
	mo->momy = FixedMul(move, FINESINE(angle));
}


fixed_t P_ReturnThrustX(mobj_t *mo, angle_t angle, fixed_t move)
{
	(void)mo;
	angle >>= ANGLETOFINESHIFT;
	return FixedMul(move, FINECOSINE(angle));
}
fixed_t P_ReturnThrustY(mobj_t *mo, angle_t angle, fixed_t move)
{
	(void)mo;
	angle >>= ANGLETOFINESHIFT;
	return FixedMul(move, FINESINE(angle));
}





boolean P_AutoPause(void)
{

	if (netgame || modeattacking || gamestate == GS_TITLESCREEN || (marathonmode && gamestate == GS_INTERMISSION))
		return false;

	return (menuactive || ( window_notinfocus && cv_pauseifunfocused.value ));
}





void P_CalcHeight(player_t *player)
{
	INT32 angle;
	fixed_t bob;
	fixed_t pviewheight;
	mobj_t *mo = player->mo;







	player->bob = FixedMul(cv_movebob.value,
		(FixedMul(player->rmomx,player->rmomx)
		+ FixedMul(player->rmomy,player->rmomy))>>2);

	if (player->bob > FixedMul(cv_movebob.value, FixedMul(MAXBOB, mo->scale)))
		player->bob = FixedMul(cv_movebob.value, FixedMul(MAXBOB, mo->scale));

	if (!P_IsObjectOnGround(mo))
	{
		if (mo->eflags & MFE_VERTICALFLIP)
		{
			player->viewz = mo->z + mo->height - player->viewheight;
			if (player->viewz < mo->floorz + FixedMul(FRACUNIT, mo->scale))
				player->viewz = mo->floorz + FixedMul(FRACUNIT, mo->scale);
		}
		else
		{
			player->viewz = mo->z + player->viewheight;
			if (player->viewz > mo->ceilingz - FixedMul(FRACUNIT, mo->scale))
				player->viewz = mo->ceilingz - FixedMul(FRACUNIT, mo->scale);
		}
		return;
	}

	angle = (FINEANGLES/20*localgametic)&FINEMASK;
	bob = FixedMul(player->bob/2, FINESINE(angle));


	pviewheight = FixedMul(41*player->height/48, mo->scale);  

	if (player->playerstate == PST_LIVE)
	{
		player->viewheight += player->deltaviewheight;

		if (player->viewheight > pviewheight)
		{
			player->viewheight = pviewheight;
			player->deltaviewheight = 0;
		}

		if (player->viewheight < pviewheight/2)
		{
			player->viewheight = pviewheight/2;
			if (player->deltaviewheight <= 0)
				player->deltaviewheight = 1;
		}

		if (player->deltaviewheight)
		{
			player->deltaviewheight += FixedMul(FRACUNIT/4, mo->scale);
			if (!player->deltaviewheight)
				player->deltaviewheight = 1;
		}
	}

	if (player->mo->eflags & MFE_VERTICALFLIP)
		player->viewz = mo->z + mo->height - player->viewheight - bob;
	else
		player->viewz = mo->z + player->viewheight + bob;

	if (player->viewz > mo->ceilingz-FixedMul(4*FRACUNIT, mo->scale))
		player->viewz = mo->ceilingz-FixedMul(4*FRACUNIT, mo->scale);
	if (player->viewz < mo->floorz+FixedMul(4*FRACUNIT, mo->scale))
		player->viewz = mo->floorz+FixedMul(4*FRACUNIT, mo->scale);
}

                                  
                                          
                                                          
                                       
    
boolean P_PlayerMoving(INT32 pnum)
{
	player_t *p = &players[pnum];

	if (!Playing())
		return false;

	if (p->jointime < 5*TICRATE || p->playerstate == PST_DEAD || p->playerstate == PST_REBORN || p->spectator)
		return false;

	return gamestate == GS_LEVEL && p->mo && p->mo->health > 0
		&& (abs(p->rmomx) >= FixedMul(FRACUNIT/2, p->mo->scale)
			|| abs(p->rmomy) >= FixedMul(FRACUNIT/2, p->mo->scale)
			|| abs(p->mo->momz) >= FixedMul(FRACUNIT/2, p->mo->scale)
			|| p->climbing || p->powers[pw_tailsfly]
			|| (p->pflags & PF_JUMPED) || (p->pflags & PF_SPINNING));
}





UINT8 P_GetNextEmerald(void)
{
	if (gamemap >= sstage_start && gamemap <= sstage_end)
		return (UINT8)(gamemap - sstage_start);
	if (gamemap >= smpstage_start || gamemap <= smpstage_end)
		return (UINT8)(gamemap - smpstage_start);
	return 0;
}







void P_GiveEmerald(boolean spawnObj)
{
	UINT8 em = P_GetNextEmerald();

	S_StartSound(NULL, sfx_cgot);  
	emeralds |= (1 << em);
	stagefailed = false;

	if (spawnObj)
	{


		UINT8 i, pnum = ((playeringame[consoleplayer]) && (!players[consoleplayer].spectator) && (players[consoleplayer].mo)) ? consoleplayer : 255;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			mobj_t *emmo;
			if (!playeringame[i])
				continue;
			if (players[i].spectator)
				continue;
			if (!players[i].mo)
				continue;

			emmo = P_SpawnMobjFromMobj(players[i].mo, 0, 0, players[i].mo->height, MT_GOTEMERALD);
			if (!emmo)
				continue;
			P_SetTarget(&emmo->target, players[i].mo);
			P_SetMobjState(emmo, mobjinfo[MT_GOTEMERALD].meleestate + em);


			if (players[i].powers[pw_carry] != CR_NIGHTSMODE)
				players[i].powers[pw_carry] = CR_NONE;

			P_SetTarget(&players[i].mo->tracer, emmo);

			if (pnum == 255)
			{
				pnum = i;
				continue;
			}

			if (i == pnum)
				continue;

			emmo->flags2 |= MF2_DONTDRAW;
		}
	}
}







void P_GiveFinishFlags(player_t *player)
{
	angle_t angle = FixedAngle(player->mo->angle << FRACBITS);
	UINT8 i;

	if (!player->mo)
		return;

	if (!(netgame||multiplayer))
		return;

	for (i = 0; i < 3; i++)
	{
		angle_t fa = (angle >> ANGLETOFINESHIFT) & FINEMASK;
		fixed_t xoffs = FINECOSINE(fa);
		fixed_t yoffs = FINESINE(fa);
		mobj_t* flag = P_SpawnMobjFromMobj(player->mo, xoffs, yoffs, 0, MT_FINISHFLAG);
		if (P_MobjWasRemoved(flag))
			continue;
		flag->angle = angle;
		angle += FixedAngle(120*FRACUNIT);

		P_SetTarget(&flag->target, player->mo);
		P_SetTarget(&flag->dontdrawforviewmobj, player->mo);  
	}
}

#if 0




void P_ResetScore(player_t *player)
{


	player->scoreadd = 0;
}
#endif






UINT8 P_FindLowestMare(void)
{
	thinker_t *th;
	mobj_t *mo2;
	UINT8 mare = UINT8_MAX;

	if (gametyperules & GTR_RACE)
		return 0;



	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type != MT_EGGCAPSULE)
			continue;
		if (mo2->health <= 0)
			continue;

		{
			const UINT8 threshold = (UINT8)mo2->threshold;
			if (mare == 255)
				mare = threshold;
			else if (threshold < mare)
				mare = threshold;
		}
	}

	CONS_Debug(DBG_NIGHTS, "Lowest mare found: %d\n", mare);

	return mare;
}








boolean P_TransferToNextMare(player_t *player)
{
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *closestaxis = NULL;
	INT32 lowestaxisnum = -1;
	UINT8 mare = P_FindLowestMare();
	fixed_t dist1, dist2 = 0;

	if (mare == 255)
		return false;

	CONS_Debug(DBG_NIGHTS, "Mare is %d\n", mare);

	player->mare = mare;
	player->marelap = 0;
	player->marebonuslap = 0;



	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type != MT_AXIS)
			continue;

		if (mo2->threshold != mare)
			continue;

		if (closestaxis == NULL)
		{
			closestaxis = mo2;
			lowestaxisnum = mo2->health;
			dist2 = R_PointToDist2(player->mo->x, player->mo->y, mo2->x, mo2->y) - mo2->radius;
		}
		else if (mo2->health < lowestaxisnum)
		{
			dist1 = R_PointToDist2(player->mo->x, player->mo->y, mo2->x, mo2->y) - mo2->radius;

			if (dist1 < dist2)
			{
				closestaxis = mo2;
				lowestaxisnum = mo2->health;
				dist2 = dist1;
			}
		}
	}

	if (closestaxis == NULL)
		return false;

	P_SetTarget(&player->mo->target, closestaxis);
	return true;
}






static mobj_t *P_FindAxis(INT32 mare, INT32 axisnum)
{
	thinker_t *th;
	mobj_t *mo2;



	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mo2 = (mobj_t *)th;


		if (!(mo2->flags2 & MF2_AXIS))
			return NULL;

		if (mo2->type != MT_AXIS)
			continue;
		if (mo2->health != axisnum)
			continue;
		if (mo2->threshold != mare)
			continue;
		return mo2;
	}

	return NULL;
}






static mobj_t *P_FindAxisTransfer(INT32 mare, INT32 axisnum, mobjtype_t type)
{
	thinker_t *th;
	mobj_t *mo2;



	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mo2 = (mobj_t *)th;


		if (!(mo2->flags2 & MF2_AXIS))
			return NULL;

		if (mo2->type != type)
			continue;
		if (mo2->health != axisnum)
			continue;
		if (mo2->threshold != mare)
			continue;
		return mo2;
	}

	return NULL;
}





void P_TransferToAxis(player_t *player, INT32 axisnum)
{
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *closestaxis;
	INT32 mare = player->mare;
	fixed_t dist1, dist2 = 0;

	CONS_Debug(DBG_NIGHTS, "Transferring to axis %d\nLeveltime: %u...\n", axisnum, leveltime);

	closestaxis = NULL;



	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type != MT_AXIS)
			continue;
		if (mo2->health != axisnum)
			continue;
		if (mo2->threshold != mare)
			continue;

		if (closestaxis == NULL)
		{
			closestaxis = mo2;
			dist2 = R_PointToDist2(player->mo->x, player->mo->y, mo2->x, mo2->y) - mo2->radius;
		}
		else
		{
			dist1 = R_PointToDist2(player->mo->x, player->mo->y, mo2->x, mo2->y) - mo2->radius;

			if (dist1 < dist2)
			{
				closestaxis = mo2;
				dist2 = dist1;
			}
		}
	}

	if (!closestaxis)
	{
		CONS_Debug(DBG_NIGHTS, "ERROR: Specified axis point to transfer to not found!\n%d\n", axisnum);
	}
	else
	{
		CONS_Debug(DBG_NIGHTS, "Transferred to axis %d, mare %d\n", closestaxis->health, closestaxis->threshold);
	}

	P_SetTarget(&player->mo->target, closestaxis);
}






static void P_DeNightserizePlayer(player_t *player)
{
	thinker_t *th;
	mobj_t *mo2;

	player->powers[pw_carry] = CR_NIGHTSFALL;

	player->powers[pw_underwater] = 0;
	player->pflags &= ~(PF_SPINDOWN|PF_JUMPDOWN|PF_ATTACKDOWN|PF_STARTDASH|PF_GLIDING|PF_STARTJUMP|PF_JUMPED|PF_NOJUMPDAMAGE|PF_THOKKED|PF_SPINNING|PF_DRILLING|PF_TRANSFERTOCLOSEST);
	player->secondjump = 0;
	player->homing = 0;
	player->climbing = 0;
	player->mo->fuse = 0;
	player->speed = 0;
	player->marelap = 0;
	player->marebonuslap = 0;
	player->flyangle = 0;
	player->anotherflyangle = 0;
	player->mo->spriteroll = 0;

	P_SetTarget(&player->mo->target, NULL);
	P_SetTarget(&player->axis1, P_SetTarget(&player->axis2, NULL));

	player->mo->flags &= ~MF_NOGRAVITY;

	player->mo->skin = skins[player->skin];
	player->followitem = skins[player->skin]->followitem;
	player->mo->color = P_GetPlayerColor(player);
	G_GhostAddColor(GHC_RETURNSKIN);


	if (player == &players[consoleplayer])
		localaiming = 0;
	else if (player == &players[secondarydisplayplayer])
		localaiming2 = 0;

	P_SetMobjState(player->mo, S_PLAY_FALL);


	if (G_IsSpecialStage(gamemap))
	{
		INT32 i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE)
				players[i].nightstime = 1;  
		player->exiting = 3*TICRATE;



		player->marescore = player->spheres = player->rings = 0;
	}


	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mo2 = (mobj_t *)th;
		if (mo2->type != MT_NIGHTSDRONE)
			continue;

		if (mo2->flags2 & MF2_AMBUSH)
		{
			player->marescore = player->spheres = player->rings = 0;
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_INSTAKILL);


			if ((mapheaderinfo[gamemap-1]->levelflags & LF_MIXNIGHTSCOUNTDOWN)
#ifdef _WIN32
				&& S_MusicType() != MU_MID
#endif
			)
				S_SetMusicPosition(0);
		}

		break;
	}

	if (player->mo->scale != player->oldscale)
		player->mo->destscale = player->oldscale;
	player->oldscale = 0;


	if ((mapheaderinfo[gamemap-1]->levelflags & LF_MIXNIGHTSCOUNTDOWN)
#ifdef _WIN32
		&& S_MusicType() != MU_MID
#endif
	)
	{
		S_StopSoundByNum(sfx_timeup);
		S_StopFadingMusic();
		S_SetInternalMusicVolume(100);




		if (P_FindLowestMare() != UINT8_MAX || G_IsSpecialStage(gamemap))
			S_SetMusicPosition(0);
	}
	else
	{
		music_stack_fadein = 0;  
		P_RestoreMusic(player);
	}

	P_RunDeNightserizeExecutors(player->mo);
}





void P_NightserizePlayer(player_t *player, INT32 nighttime)
{
	UINT8 oldmare, oldmarelap, oldmarebonuslap, newmare;


	if (player->bot)
		return;

	if (player->powers[pw_carry] != CR_NIGHTSMODE)
	{
		player->mo->height = P_GetPlayerHeight(player);  
		player->oldscale = player->mo->scale;

		if (skins[player->skin]->sprites[SPR2_NFLY].numframes == 0)  
		{
			player->mo->skin = skins[DEFAULTNIGHTSSKIN];
			if (!(cv_debug || devparm) && !(netgame || multiplayer || demoplayback))
				player->mo->color = skins[DEFAULTNIGHTSSKIN]->prefcolor;
			player->followitem = skins[DEFAULTNIGHTSSKIN]->followitem;
			G_GhostAddColor(GHC_NIGHTSSKIN);
		}
	}


	newmare = P_FindLowestMare();
	if (mapheaderinfo[gamemap-1]->nightstimer[newmare] > 0)
		nighttime = mapheaderinfo[gamemap-1]->nightstimer[newmare];

	player->pflags &= ~(PF_SPINDOWN|PF_JUMPDOWN|PF_ATTACKDOWN|PF_STARTDASH|PF_GLIDING|PF_JUMPED|PF_NOJUMPDAMAGE|PF_THOKKED|PF_SHIELDABILITY|PF_SPINNING|PF_DRILLING);
	player->homing = 0;
	player->mo->fuse = 0;
	player->speed = 0;
	player->climbing = 0;
	player->secondjump = 0;
	player->flyangle = 0;
	player->anotherflyangle = 0;
	player->mo->spriteroll = 0;

	player->powers[pw_shield] = SH_NONE;
	player->powers[pw_super] = 0;

	player->mo->flags |= MF_NOGRAVITY;

	player->nightstime = player->startedtime = player->lapstartedtime = nighttime*TICRATE;
	player->bonustime = false;


	if (mapheaderinfo[gamemap-1]->levelflags & LF_MIXNIGHTSCOUNTDOWN)
	{
		S_StopSoundByNum(sfx_timeup);
		S_StopFadingMusic();
		S_SetInternalMusicVolume(100);
	}
	else
	{
		music_stack_fadein = 0;  
		P_RestoreMusic(player);
	}

	if (gametyperules & GTR_RACE)
	{
		if (player->drillmeter < 48*20)
			player->drillmeter = 48*20;
	}
	else
	{
		if (player->drillmeter < 40*20)
			player->drillmeter = 40*20;
	}

	oldmare = player->mare;
	oldmarelap = player->marelap;
	oldmarebonuslap = player->marebonuslap;

	if (!P_TransferToNextMare(player))
	{
		INT32 i;
		INT32 total_spheres = 0;
		INT32 total_rings = 0;

		P_SetTarget(&player->mo->target, NULL);

		if (G_IsSpecialStage(gamemap))
		{
			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i] )
				{
					total_spheres += players[i].spheres;
					total_rings += players[i].rings;
				}
		}

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || !players[i].mo || players[i].spectator)
				continue;

			players[i].texttimer = (3 * TICRATE) - 10;
			players[i].textvar = NTV_BONUSTIMEEND;  
			players[i].lastmare = players[i].mare;
			players[i].lastmarelap = players[i].marelap;
			players[i].lastmarebonuslap = players[i].marebonuslap;
			if (G_IsSpecialStage(gamemap))
			{
				players[i].finishedspheres = (INT16)total_spheres;
				players[i].finishedrings = (INT16)total_rings;
				P_AddPlayerScore(player, total_spheres * 50);
			}
			else
			{
				players[i].finishedspheres = (INT16)(players[i].spheres);
				players[i].finishedrings = (INT16)(players[i].rings);
				P_AddPlayerScore(&players[i], (players[i].spheres) * 50);
			}


			player->lastmaretime = leveltime - player->marebegunat;
			G_AddTempNightsRecords(player, players[i].marescore, player->lastmaretime, players[i].mare + 1);


			players[i].totalmarescore += players[i].marescore;
			players[i].lastmarescore = players[i].marescore;
			players[i].marescore = 0;

			players[i].spheres = players[i].rings = 0;
			P_DoPlayerExit(&players[i], true);
		}
	}
	else if (oldmare != player->mare)
	{


		P_AddPlayerScore(player, (player->spheres) * 50);

		player->lastmare = oldmare;
		player->lastmarelap = oldmarelap;
		player->lastmarebonuslap = oldmarebonuslap;
		player->texttimer = 4*TICRATE;
		player->textvar = NTV_BONUSTIMEEND;  
		player->finishedspheres = (INT16)(player->spheres);
		player->finishedrings = (INT16)(player->rings);


		player->lastmaretime = leveltime - player->marebegunat;
		G_AddTempNightsRecords(player, player->marescore, player->lastmaretime, (UINT8)(oldmare + 1));


		player->totalmarescore += player->marescore;
		player->lastmarescore = player->marescore;
		player->marescore = 0;
		player->marebegunat = leveltime;
		player->lapbegunat = leveltime;

		player->spheres = player->rings = 0;
	}
	else
	{
		player->textvar = NTV_NONE;  
		player->texttimer = 40;



		if (timeinmap + 40 < (110 - 70))
			player->texttimer = (UINT8)((110 - 70) - timeinmap);
	}

	if (player->drone && player->drone->scale != player->mo->scale)
		player->mo->destscale = player->drone->scale;


	if (player->mo->target)
	{
		player->angle_pos = R_PointToAngle2(player->mo->target->x, player->mo->target->y, player->mo->x, player->mo->y);
		player->drawangle = player->angle_pos
			+ ((player->mo->target->flags2 & MF2_AMBUSH) ?  
				-ANGLE_90 : ANGLE_90);  
				                                                                          
                                                                                  
	}


	P_RunNightserizeExecutors(player->mo);

	player->powers[pw_carry] = CR_NIGHTSMODE;
	P_SetMobjState(player->mo, S_PLAY_NIGHTS_TRANS1);
}

pflags_t P_GetJumpFlags(player_t *player)
{
	if (player->charflags & SF_NOJUMPDAMAGE)
		return (PF_JUMPED|PF_NOJUMPDAMAGE);
	return PF_JUMPED;
}







boolean P_PlayerInPain(player_t *player)
{
	if (P_MobjWasRemoved(player->mo))
		return false;


	if (!(player->pflags & PF_SLIDING) && player->mo->state == &states[player->mo->info->painstate] && player->powers[pw_flashing])
		return true;

	if (player->mo->state == &states[S_PLAY_STUN])
		return true;

	return false;
}







void P_DoPlayerPain(player_t *player, mobj_t *source, mobj_t *inflictor)
{
	if (player->powers[pw_carry] == CR_ROPEHANG)
		P_SetTarget(&player->mo->tracer, NULL);

	player->powers[pw_strong] = STR_NONE;

	{
		angle_t ang;
		fixed_t fallbackspeed;

		P_ResetPlayer(player);
		P_SetMobjState(player->mo, player->mo->info->painstate);

		if (player->mo->eflags & MFE_VERTICALFLIP)
			player->mo->z--;
		else
			player->mo->z++;

		if (player->mo->eflags & MFE_UNDERWATER)
			P_SetObjectMomZ(player->mo, FixedDiv(10511*FRACUNIT,2600*FRACUNIT), false);
		else
			P_SetObjectMomZ(player->mo, FixedDiv(69*FRACUNIT,10*FRACUNIT), false);

		if (inflictor)
		{
			if (inflictor->type == MT_WALLSPIKE)
				ang = inflictor->angle;
			else
				ang = R_PointToAngle2(inflictor->x-inflictor->momx, inflictor->y - inflictor->momy, player->mo->x - player->mo->momx, player->mo->y - player->mo->momy);



			if ((inflictor->flags2 & MF2_SCATTER) && source)
			{
				fixed_t dist = P_AproxDistance(P_AproxDistance(source->x-player->mo->x, source->y-player->mo->y), source->z-player->mo->z);

				dist = FixedMul(128*FRACUNIT, inflictor->scale) - dist/4;

				if (dist < FixedMul(4*FRACUNIT, inflictor->scale))
					dist = FixedMul(4*FRACUNIT, inflictor->scale);

				fallbackspeed = dist;
			}
			else if (inflictor->flags2 & MF2_EXPLOSION)
			{
				if (inflictor->flags2 & MF2_RAILRING)
					fallbackspeed = FixedMul(38*FRACUNIT, inflictor->scale);  
				else
					fallbackspeed = FixedMul(30*FRACUNIT, inflictor->scale);  
			}
			else if (inflictor->flags2 & MF2_RAILRING)
				fallbackspeed = FixedMul(45*FRACUNIT, inflictor->scale);  
			else
				fallbackspeed = FixedMul(4*FRACUNIT, inflictor->scale);  
		}
		else
		{
			ang = ((player->mo->momx || player->mo->momy) ? R_PointToAngle2(player->mo->momx, player->mo->momy, 0, 0) : player->drawangle);
			fallbackspeed = FixedMul(4*FRACUNIT, player->mo->scale);
		}

		if (player->pflags & PF_DIRECTIONCHAR)
			player->drawangle = ang + ANGLE_180;
		P_InstaThrust(player->mo, ang, fallbackspeed);
	}



	if (((gametyperules & (GTR_TAG|GTR_HIDEFROZEN)) == GTR_TAG)
	&& (!(player->pflags & PF_GAMETYPEOVER) && !(player->pflags & PF_TAGIT)))
	{
		if (player->score >= 50)
			player->score -= 50;
		else
			player->score = 0;
	}

	player->powers[pw_flashing] = flashingtics;

	if (player->timeshit != UINT8_MAX)
		++player->timeshit;
}





void P_ResetPlayer(player_t *player)
{
	player->pflags &= ~(PF_SPINNING|PF_STARTDASH|PF_STARTJUMP|PF_JUMPED|PF_NOJUMPDAMAGE|PF_GLIDING|PF_THOKKED|PF_CANCARRY|PF_SHIELDABILITY|PF_BOUNCING);

	if (player->powers[pw_carry] == CR_ROLLOUT)
	{
		if (player->mo->tracer && !P_MobjWasRemoved(player->mo->tracer))
		{
			player->mo->tracer->flags |= MF_PUSHABLE;


			if (player->mo->tracer->eflags & MFE_VERTICALFLIP)
				player->mo->tracer->momz -= 1;
			else
				player->mo->tracer->momz += 1;

			P_SetTarget(&player->mo->tracer->tracer, NULL);
		}
		P_SetTarget(&player->mo->tracer, NULL);
		player->powers[pw_carry] = CR_NONE;
	}

	if (!(player->powers[pw_carry] == CR_NIGHTSMODE || player->powers[pw_carry] == CR_NIGHTSFALL || player->powers[pw_carry] == CR_BRAKGOOP || player->powers[pw_carry] == CR_MINECART))
		player->powers[pw_carry] = CR_NONE;

	player->secondjump = 0;
	player->glidetime = 0;
	player->homing = 0;
	player->climbing = 0;
	player->powers[pw_tailsfly] = 0;
	player->onconveyor = 0;
	player->skidtime = 0;
	if (player-players == consoleplayer && botingame)
		CV_SetValue(&cv_analog[1], true);
}





boolean P_PlayerCanDamage(player_t *player, mobj_t *thing)
{
	fixed_t bottomheight, topheight;
	boolean allatk = ((player->powers[pw_strong] & STR_PUNCH) && (player->powers[pw_strong] & STR_TAIL) && (player->powers[pw_strong] & STR_STOMP) && (player->powers[pw_strong] & STR_UPPER));

	if (!player->mo || player->spectator || !thing || P_MobjWasRemoved(thing))
		return false;

	{
		UINT8 shouldCollide = LUA_HookPlayerCanDamage(player, thing);
		if (P_MobjWasRemoved(thing))
			return false;  
		if (shouldCollide == 1)
			return true;  
		else if (shouldCollide == 2)
			return false;  
	}


	if (!(thing->flags & MF_MONITOR) && (player->powers[pw_invulnerability] || player->powers[pw_super]))
		return true;


	if ((player->powers[pw_carry] == CR_NIGHTSMODE) && (player->pflags & PF_DRILLING))
		return true;


	if ((player->pflags & PF_JUMPED)
	&& (!(player->pflags & PF_NOJUMPDAMAGE)))
		return true;


	if (player->pflags & PF_SPINNING)
		return true;


	if (((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL || (player->powers[pw_shield] & SH_NOSTACK) == SH_BUBBLEWRAP) && (player->pflags & PF_SHIELDABILITY))
		return true;




	if (allatk || (player->powers[pw_strong] & STR_DASH))
		return true;


	if ((player->powers[pw_strong] & STR_PUNCH)
	&& (player->drawangle - R_PointToAngle2(player->mo->x - player->mo->momx, player->mo->y - player->mo->momy, thing->x, thing->y) +  + ANGLE_90) < ANGLE_180)
		return true;


	if ((player->powers[pw_strong] & STR_TAIL)
	&& (player->drawangle - R_PointToAngle2(player->mo->x - player->mo->momx, player->mo->y - player->mo->momy, thing->x, thing->y) +  + ANGLE_90) >= ANGLE_180)
		return true;


	bottomheight = player->mo->z;
	topheight = player->mo->z + player->mo->height;

	if (player->mo->eflags & MFE_VERTICALFLIP)
	{
		fixed_t swap = bottomheight;
		bottomheight = topheight;
		topheight = swap;
	}

	if (P_MobjFlip(player->mo)*(bottomheight - (thing->z + thing->height/2)) > 0)
	{
		if ((player->charflags & SF_STOMPDAMAGE || player->powers[pw_strong] & STR_STOMP) && (P_MobjFlip(player->mo)*(player->mo->momz - thing->momz) < 0))
			return true;
	}
	else if (P_MobjFlip(player->mo)*(topheight - (thing->z + thing->height/2)) < 0)
	{
		if ((player->powers[pw_strong] & STR_UPPER) && (player->mo->sprite2 != SPR2_SWIM) && (P_MobjFlip(player->mo)*(player->mo->momz - thing->momz) > 0))
			return true;
	}

	return false;
}










void P_GivePlayerRings(player_t *player, INT32 num_rings)
{
	if (!player)
		return;

	if ((player->bot == BOT_2PAI || player->bot == BOT_2PHUMAN) && player->botleader)
		player = player->botleader;

	if (!player->mo)
		return;

	player->rings += num_rings;

	player->totalring += num_rings;


	if (player->rings > 9999)
		player->rings = 9999;
	else if (player->rings < 0)
		player->rings = 0;


	if (!ultimatemode && !modeattacking && !G_IsSpecialStage(gamemap) && G_GametypeUsesLives() && player->lives != INFLIVES)
	{
		INT32 gainlives = 0;

		while (player->xtralife < maxXtraLife && player->rings >= 100 * (player->xtralife+1))
		{
			++gainlives;
			++player->xtralife;
		}

		if (gainlives)
		{
			player->lives += gainlives;
			if (player->lives > 99)
				player->lives = 99;
			else if (player->lives < 1)
				player->lives = 1;

			P_PlayLivesJingle(player);
		}
	}
}

void P_GivePlayerSpheres(player_t *player, INT32 num_spheres)
{
	if (!player)
		return;

	if (player->bot)
		player = &players[consoleplayer];

	if (!player->mo)
		return;

	player->spheres += num_spheres;


	if (player->spheres > 9999)
		player->spheres = 9999;
	else if (player->spheres < 0)
		player->spheres = 0;
}







void P_GivePlayerLives(player_t *player, INT32 numlives)
{
	UINT8 prevlives = player->lives;
	if (!player)
		return;

	if (player->bot)
		player = &players[consoleplayer];

	if (gamestate == GS_LEVEL)
	{
		if (player->lives == INFLIVES || !(gametyperules & GTR_LIVES))
		{
			P_GivePlayerRings(player, 100*numlives);
			return;
		}

		if ((netgame || multiplayer) && G_GametypeUsesCoopLives() && cv_cooplives.value == 0)
		{
			P_GivePlayerRings(player, 100*numlives);
			if (player->lives - prevlives >= numlives)
				goto docooprespawn;

			numlives = (numlives + prevlives - player->lives);
		}
	}
	else if (player->lives == INFLIVES)
		return;

	player->lives += numlives;

	if (player->lives > 99)
		player->lives = 99;
	else if (player->lives < 1)
		player->lives = 1;

docooprespawn:
	if (cv_coopstarposts.value)
		return;
	if (prevlives > 0)
		return;
	if (!player->spectator)
		return;
	P_SpectatorJoinGame(player);
}

void P_GiveCoopLives(player_t *player, INT32 numlives, boolean sound)
{
	if (!((netgame || multiplayer) && G_GametypeUsesCoopLives()))
	{
		P_GivePlayerLives(player, numlives);
		if (sound)
			P_PlayLivesJingle(player);
	}
	else
	{
		INT32 i;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			P_GivePlayerLives(&players[i], numlives);
			if (sound)
				P_PlayLivesJingle(&players[i]);
		}
	}
}





void P_DoSuperTransformation(player_t *player, boolean giverings)
{
	player->powers[pw_super] = 1;
	if (!(mapheaderinfo[gamemap-1]->levelflags & LF_NOSSMUSIC) && P_IsLocalPlayer(player))
		P_PlayJingle(player, JT_SUPER);

	S_StartSound(NULL, sfx_supert);  

	player->mo->momx = player->mo->momy = player->mo->momz = player->cmomx = player->cmomy = player->rmomx = player->rmomy = 0;


	P_SetMobjState(player->mo, S_PLAY_SUPER_TRANS1);

	if (giverings && player->rings < 50)
		player->rings = 50;


	if (!(mapheaderinfo[gamemap-1]->levelflags & LF_NOSSMUSIC))
	{
		player->powers[pw_extralife] = 0;
		player->powers[pw_invulnerability] = 0;
		player->powers[pw_sneakers] = 0;
	}

	if (!G_CoopGametype())
	{
		HU_SetCEchoFlags(0);
		HU_SetCEchoDuration(5);
		HU_DoCEcho(va("%s\\is now super.\\\\\\\\", player_names[player-players]));
	}

	P_PlayerFlagBurst(player, false);
}





void P_DoSuperDetransformation(player_t *player)
{
	player->powers[pw_emeralds] = 0;  
	P_SpawnGhostMobj(player->mo);

	player->powers[pw_super] = 0;


	if ((player->powers[pw_shield] & SH_STACK) == SH_FIREFLOWER)
	{
		player->mo->color = SKINCOLOR_WHITE;
		G_GhostAddColor(GHC_FIREFLOWER);
	}
	else
	{
		player->mo->color = P_GetPlayerColor(player);
		G_GhostAddColor(GHC_NORMAL);
	}

	if (!G_CoopGametype())
		player->powers[pw_flashing] = flashingtics-1;

	if (player->mo->sprite2 & FF_SPR2SUPER)
		P_SetMobjState(player->mo, player->mo->state-states);


	if (!G_CoopGametype())
	{
		S_StartSound(NULL, sfx_s3k66);  
		HU_SetCEchoFlags(0);
		HU_SetCEchoDuration(5);
		HU_DoCEcho(va("%s\\is no longer super.\\\\\\\\", player_names[player-players]));
	}


	if (P_IsLocalPlayer(player))
	{
		music_stack_noposition = true;  
		music_stack_fadeout = MUSICRATE/2;  
	}
	P_RestoreMusic(player);


	P_SpawnShieldOrb(player);
}


void P_AddPlayerScore(player_t *player, UINT32 amount)
{
	UINT32 oldscore;

	if ((player->bot == BOT_2PAI || player->bot == BOT_2PHUMAN) && player->botleader)
		player = player->botleader;


	if (gamestate == GS_LEVEL && mapheaderinfo[gamemap-1]->typeoflevel & TOL_NIGHTS)
	{
		if ((netgame || multiplayer) && G_IsSpecialStage(gamemap))
		{  
			INT32 i;
			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE)
				{
					oldscore = players[i].marescore;


					if (players[i].marescore + amount < MAXSCORE)
						players[i].marescore += amount;
					else
						players[i].marescore = MAXSCORE;



                                                                              
                         
      
                                
                                    
                                  
                                     
         
				}
		}
		else
		{
			oldscore = player->marescore;


			if (player->marescore + amount < MAXSCORE)
				player->marescore += amount;
			else
				player->marescore = MAXSCORE;

			if (!ultimatemode && continuesInSession && G_IsSpecialStage(gamemap)
			&& player->marescore >= 50000 && oldscore < 50000)
			{
				player->continues += 1;
				player->gotcontinue = true;
				if (P_IsLocalPlayer(player))
					S_StartSound(NULL, sfx_s3kac);
			}
		}

		if (G_CoopGametype())
			return;
	}

	oldscore = player->score;


	player->score += amount;
	if (player->score > MAXSCORE)
		player->score = MAXSCORE;

	player->recordscore += amount;
	if (player->recordscore > MAXSCORE)
		player->recordscore = MAXSCORE;


	if (!ultimatemode && !modeattacking && player->score > oldscore && player->score % 50000 < amount && (gametyperules & GTR_LIVES))
	{
		P_GivePlayerLives(player, (player->score/50000) - (oldscore/50000));
		P_PlayLivesJingle(player);
	}


	if ((gametyperules & (GTR_TEAMS|GTR_TEAMFLAGS)) == GTR_TEAMS)
	{
		if (player->ctfteam == 1)
			redscore += amount;
		else if (player->ctfteam == 2)
			bluescore += amount;
	}
}


void P_StealPlayerScore(player_t *player, UINT32 amount)
{
	boolean teams = G_GametypeHasTeams();
	UINT32 stolen = 0;
	int i;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (&players[i] == player
		|| (teams && players[i].ctfteam == player->ctfteam))
			continue;
		if (players[i].score >= amount)
		{
			stolen += amount;
			players[i].score -= amount;
		}
		else
		{
			stolen += players[i].score;
			players[i].score = 0;
		}
	}
	if (stolen > 0)
	{

		if ((gametyperules & (GTR_TEAMS|GTR_TEAMFLAGS)) == GTR_TEAMS)
		{
			if (player->ctfteam == 1)
				bluescore -= amount;
			else if (player->ctfteam == 2)
				redscore -= amount;
		}
		P_AddPlayerScore(player, stolen);
	}
}




void P_PlayLivesJingle(player_t *player)
{
	if (player && !P_IsLocalPlayer(player))
		return;

	if (mariomode)
		S_StartSound(NULL, sfx_marioa);
	else if (use1upSound || cv_1upsound.value)
		S_StartSound(NULL, sfx_oneup);
	else
	{
		P_PlayJingle(player, JT_1UP);
		if (player)
			player->powers[pw_extralife] = extralifetics + 1;
		strlcpy(S_sfx[sfx_None].caption, "One-up", 7);
		S_StartCaption(sfx_None, -1, extralifetics+1);
	}
}

void P_PlayJingle(player_t *player, jingletype_t jingletype)
{
	const char *musname = jingleinfo[jingletype].musname;
	UINT16 musflags = 0;
	boolean looping = jingleinfo[jingletype].looping;

	char newmusic[7];
	strncpy(newmusic, musname, 7);
#ifdef HAVE_LUA_MUSICPLUS
	if(LUAh_MusicJingle(jingletype, newmusic, &musflags, &looping))
		return;
#endif
	newmusic[6] = 0;

	P_PlayJingleMusic(player, newmusic, musflags, looping, jingletype);
}




void P_PlayJingleMusic(player_t *player, const char *musname, UINT16 musflags, boolean looping, UINT16 status)
{

	if (gamestate == GS_LEVEL && player && !P_IsLocalPlayer(player))
		return;

	S_RetainMusic(musname, musflags, looping, 0, status);
	S_StopMusic();
	S_ChangeMusicInternal(musname, looping);
}

boolean P_EvaluateMusicStatus(UINT16 status, const char *musname)
{

	int i;
	boolean result = false;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!P_IsLocalPlayer(&players[i]))
			continue;

		switch(status)
		{
			case JT_1UP:  
				result = (players[i].powers[pw_extralife] > 1);
				break;

			case JT_SHOES:   
				if (players[i].powers[pw_sneakers] > 1 && !players[i].powers[pw_super])
				{


					result = true;
				}
				else
					result = false;
				break;

			case JT_INV:  
			case JT_MINV:  
				if (players[i].powers[pw_invulnerability] > 1)
				{


					result = true;
				}
				else
					result = false;
				break;

			case JT_DROWN:   
				result = (players[i].powers[pw_underwater] && players[i].powers[pw_underwater] <= 11*TICRATE + 1);
				break;

			case JT_SUPER:   
				result = (players[i].powers[pw_super] && !(mapheaderinfo[gamemap-1]->levelflags & LF_NOSSMUSIC));
				break;

			case JT_GOVER:  
				result = (players[i].lives <= 0);
				break;

			case JT_NIGHTSTIMEOUT:  
			case JT_SSTIMEOUT:
				result = (players[i].nightstime && players[i].nightstime <= 10*TICRATE);
				break;

			case JT_OTHER:   
				result = LUA_HookShouldJingleContinue(&players[i], musname);
				break;

			case JT_NONE:    
			case JT_MASTER:  
			default:
				result = true;
		}

		if (result)
			break;
	}

	return result;
 }






void P_RestoreMusic(player_t *player)
{
	if (!P_IsLocalPlayer(player))  
		return;

	S_SpeedMusic(1.0f);





	if (player->powers[pw_extralife] > 1)
		return;


	else if (player->powers[pw_super] && !(mapheaderinfo[gamemap-1]->levelflags & LF_NOSSMUSIC)
		&& !S_RecallMusic(JT_SUPER, false))
		P_PlayJingle(player, JT_SUPER);


	else if (player->powers[pw_invulnerability] > 1 && !player->powers[pw_super])
	{
		strlcpy(S_sfx[sfx_None].caption, "Invincibility", 14);
		S_StartCaption(sfx_None, -1, player->powers[pw_invulnerability]);
		if (!S_RecallMusic(JT_INV, false) && !S_RecallMusic(JT_MINV, false))
			P_PlayJingle(player, (mariomode) ? JT_MINV : JT_INV);
	}


	else if (player->powers[pw_sneakers] > 1 && !player->powers[pw_super])
	{
		strlcpy(S_sfx[sfx_None].caption, "Speed shoes", 12);
		S_StartCaption(sfx_None, -1, player->powers[pw_sneakers]);
		if (mapheaderinfo[gamemap-1]->levelflags & LF_SPEEDMUSIC)
		{
			S_SpeedMusic(1.4f);
			if (!S_RecallMusic(JT_MASTER, true))
				S_ChangeMusicEx(mapmusname, mapmusflags, true, mapmusposition, 0, 0);
		}
		else if (!S_RecallMusic(JT_SHOES, false))
			P_PlayJingle(player, JT_SHOES);
	}


	else if (!S_RecallMusic(JT_NONE, false))  
	{
		CONS_Debug(DBG_BASIC, "Cannot find any music in resume stack!\n");
		S_ChangeMusicEx(mapmusname, mapmusflags, true, mapmusposition, 0, 0);
	}
}







boolean P_IsObjectInGoop(mobj_t *mo)
{
	if (mo->player && mo->player->spectator)
		return false;

	if (mo->flags & MF_NOGRAVITY)
		return false;

	return ((mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER)) == (MFE_UNDERWATER|MFE_GOOWATER));
}








boolean P_IsObjectOnGround(mobj_t *mo)
{
	if (P_IsObjectInGoop(mo) && !(mo->player && mo->player->pflags & PF_BOUNCING))
	{
  
                                                            
                                                      
                         
   
                      
                
   
                                             
   
                                     
                
   
  



		return false;
	}

	if (mo->eflags & MFE_VERTICALFLIP)
	{
		if (mo->z+mo->height >= mo->ceilingz)
			return true;
	}
	else
	{
		if (mo->z <= mo->floorz)
			return true;
	}

	return false;
}







void P_SetObjectMomZ(mobj_t *mo, fixed_t value, boolean relative)
{
	if (mo->eflags & MFE_VERTICALFLIP)
		value = -value;

	if (mo->scale != FRACUNIT)
		value = FixedMul(value, mo->scale);

	if (relative)
		mo->momz += value;
	else
		mo->momz = value;
}







boolean P_IsLocalPlayer(player_t *player)
{
	return ((splitscreen && player == &players[secondarydisplayplayer]) || player == &players[consoleplayer]);
}








void P_SpawnShieldOrb(player_t *player)
{
	mobjtype_t orbtype;
	thinker_t *th;
	mobj_t *shieldobj, *ov;

#ifdef PARANOIA
	if (!player->mo)
		I_Error("P_SpawnShieldOrb: player->mo is NULL!\n");
#endif

	if (LUA_HookPlayer(player, HOOK(ShieldSpawn)))
		return;

	if (player->powers[pw_shield] & SH_FORCE)
		orbtype = MT_FORCE_ORB;
	else switch (player->powers[pw_shield] & SH_NOSTACK)
	{
	case SH_WHIRLWIND:
		orbtype = MT_WHIRLWIND_ORB;
		break;
	case SH_ATTRACT:
		orbtype = MT_ATTRACT_ORB;
		break;
	case SH_ELEMENTAL:
		orbtype = MT_ELEMENTAL_ORB;
		break;
	case SH_ARMAGEDDON:
		orbtype = MT_ARMAGEDDON_ORB;
		break;
	case SH_PITY:
	case SH_PINK:  
		orbtype = MT_PITY_ORB;
		break;
	case SH_FLAMEAURA:
		orbtype = MT_FLAMEAURA_ORB;
		break;
	case SH_BUBBLEWRAP:
		orbtype = MT_BUBBLEWRAP_ORB;
		break;
	case SH_THUNDERCOIN:
		orbtype = MT_THUNDERCOIN_ORB;
		break;
	default:
		return;
	}


	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		shieldobj = (mobj_t *)th;

		if (shieldobj->type == orbtype && shieldobj->target == player->mo)
			P_RemoveMobj(shieldobj);  
	}

	shieldobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, orbtype);
	if (P_MobjWasRemoved(shieldobj))
		return;

	shieldobj->flags2 |= MF2_SHIELD;
	P_SetTarget(&shieldobj->target, player->mo);
	P_SetTarget(&shieldobj->dontdrawforviewmobj, player->mo);  
	if ((player->powers[pw_shield] & SH_NOSTACK) == SH_PINK)
	{
		shieldobj->color = SKINCOLOR_PINK;
		shieldobj->colorized = true;
	}
	else
		shieldobj->color = (UINT16)shieldobj->info->painchance;
	shieldobj->threshold = (player->powers[pw_shield] & SH_FORCE) ? SH_FORCE : (player->powers[pw_shield] & SH_NOSTACK);

	if (shieldobj->info->seestate)
	{
		ov = P_SpawnMobj(shieldobj->x, shieldobj->y, shieldobj->z, MT_OVERLAY);
		if (!P_MobjWasRemoved(ov))
		{
			P_SetTarget(&ov->target, shieldobj);
		P_SetTarget(&ov->dontdrawforviewmobj, player->mo);  
			P_SetMobjState(ov, shieldobj->info->seestate);
			P_SetTarget(&shieldobj->tracer, ov);
		}
	}
	if (shieldobj->info->meleestate)
	{
		ov = P_SpawnMobj(shieldobj->x, shieldobj->y, shieldobj->z, MT_OVERLAY);
		if (!P_MobjWasRemoved(ov))
		{
			P_SetTarget(&ov->target, shieldobj);
		P_SetTarget(&ov->dontdrawforviewmobj, player->mo);  
			P_SetMobjState(ov, shieldobj->info->meleestate);
		}
	}
	if (shieldobj->info->missilestate)
	{
		ov = P_SpawnMobj(shieldobj->x, shieldobj->y, shieldobj->z, MT_OVERLAY);
		if (!P_MobjWasRemoved(ov))
		{
			P_SetTarget(&ov->target, shieldobj);
		P_SetTarget(&ov->dontdrawforviewmobj, player->mo);  
			P_SetMobjState(ov, shieldobj->info->missilestate);
		}
	}
	if (player->powers[pw_shield] & SH_FORCE)
	{

		shieldobj->movecount = (player->powers[pw_shield] & SH_FORCEHP);
		if (shieldobj->movecount < 1)
		{
			if (shieldobj->info->painstate)
				P_SetMobjState(shieldobj,shieldobj->info->painstate);
			else
				shieldobj->flags2 |= MF2_SHADOW;
		}
	}
}








void P_SwitchShield(player_t *player, UINT16 shieldtype)
{
	boolean donthavealready;


	if ((shieldtype == SH_ARMAGEDDON) && (player->powers[pw_shield] & SH_NOSTACK) == SH_ARMAGEDDON)
		P_BlackOw(player);

	donthavealready = (shieldtype & SH_FORCE)
		? (!(player->powers[pw_shield] & SH_FORCE) || (player->powers[pw_shield] & SH_FORCEHP) < (shieldtype & ~SH_FORCE))
		: ((player->powers[pw_shield] & SH_NOSTACK) != shieldtype);

	if (donthavealready)
	{
		boolean stopshieldability = (shieldtype & SH_FORCE)
		? (!(player->powers[pw_shield] & SH_FORCE))
		: true;


		if (stopshieldability && player->pflags & PF_SHIELDABILITY)
		{
			player->pflags &= ~(PF_SPINNING|PF_SHIELDABILITY);  
			player->homing = 0;
		}

		player->powers[pw_shield] = shieldtype|(player->powers[pw_shield] & SH_STACK);
		P_SpawnShieldOrb(player);

		if (shieldtype & SH_PROTECTWATER)
		{
			if (player->powers[pw_underwater] && player->powers[pw_underwater] <= 12*TICRATE + 1)
			{
				player->powers[pw_underwater] = 0;
				P_RestoreMusic(player);
			}
			else
				player->powers[pw_underwater] = 0;

			if (player->powers[pw_spacetime] > 1)
			{
				player->powers[pw_spacetime] = 0;
				P_RestoreMusic(player);
			}
		}
	}
}






void P_SetPower(player_t *player, powertype_t power, UINT16 value)
{
	boolean spawnshield = false;

	if (power == pw_shield && player->powers[pw_shield] != value)
		spawnshield = true;

	player->powers[power] = value;

	if (spawnshield)  
		P_SpawnShieldOrb(player);
}






mobj_t *P_SpawnGhostMobj(mobj_t *mobj)
{
	mobj_t *ghost = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_GHOST);
	if (P_MobjWasRemoved(ghost))
		return NULL;

	P_SetTarget(&ghost->target, mobj);
	P_SetTarget(&ghost->dontdrawforviewmobj, mobj);  

	P_SetScale(ghost, mobj->scale, true);

	if (mobj->eflags & MFE_VERTICALFLIP)
	{
		ghost->eflags |= MFE_VERTICALFLIP;
		ghost->z += mobj->height - ghost->height;
	}

	ghost->color = mobj->color;
	ghost->translation = mobj->translation;
	ghost->colorized = mobj->colorized;  

	ghost->angle = (mobj->player ? mobj->player->drawangle : mobj->angle);
	ghost->roll = mobj->roll;
	ghost->pitch = mobj->pitch;
	ghost->spriteroll = mobj->spriteroll;

	ghost->sprite = mobj->sprite;
	ghost->sprite2 = mobj->sprite2;
	ghost->frame = mobj->frame;
	ghost->tics = -1;
	ghost->frame &= ~FF_TRANSMASK;
	ghost->frame |= tr_trans50<<FF_TRANSSHIFT;

	ghost->renderflags = mobj->renderflags;
	ghost->blendmode = mobj->blendmode;
	ghost->alpha = mobj->alpha;

	ghost->spritexscale = mobj->spritexscale;
	ghost->spriteyscale = mobj->spriteyscale;
	ghost->spritexoffset = mobj->spritexoffset;
	ghost->spriteyoffset = mobj->spriteyoffset;

	ghost->fuse = ghost->info->damage;
	ghost->skin = mobj->skin;
	ghost->standingslope = mobj->standingslope;

	if (mobj->flags2 & MF2_OBJECTFLIP)
		ghost->flags2 |= MF2_OBJECTFLIP;

	if (mobj->player && mobj->player->followmobj)
	{
		mobj_t *ghost2 = P_SpawnGhostMobj(mobj->player->followmobj);
		if (!P_MobjWasRemoved(ghost2))
		{
			P_SetTarget(&ghost2->tracer, ghost);
			P_SetTarget(&ghost->tracer, ghost2);
		P_SetTarget(&ghost2->dontdrawforviewmobj, mobj);  
			ghost2->flags2 |= (mobj->player->followmobj->flags2 & MF2_LINKDRAW);
		}
	}


	ghost->old_x = mobj->old_x2;
	ghost->old_y = mobj->old_y2;
	ghost->old_z = mobj->old_z2;
	ghost->old_angle = (mobj->player ? mobj->player->old_drawangle2 : mobj->old_angle2);
	ghost->old_pitch = mobj->old_pitch2;
	ghost->old_roll = mobj->old_roll2;
	ghost->old_spriteroll = mobj->old_spriteroll2;
	ghost->old_spritexscale = mobj->old_spritexscale2;
	ghost->old_spriteyscale = mobj->old_spriteyscale2;
	ghost->old_spritexoffset = mobj->old_spritexoffset2;
	ghost->old_spriteyoffset = mobj->old_spriteyoffset2;
	ghost->old_scale = mobj->old_scale2;

	return ghost;
}






void P_SpawnThokMobj(player_t *player)
{
	mobj_t *mobj;
	mobjtype_t type = player->thokitem;
	fixed_t zheight;

	if (player->skincolor == 0)
		return;

	if (player->spectator)
		return;

	if (!type)
		return;

	if (type == MT_GHOST)
		mobj = P_SpawnGhostMobj(player->mo);  
	else
	{
		if (player->mo->eflags & MFE_VERTICALFLIP)
			zheight = player->mo->z + player->mo->height + FixedDiv(P_GetPlayerHeight(player) - player->mo->height, 3*FRACUNIT) - FixedMul(mobjinfo[type].height, player->mo->scale);
		else
			zheight = player->mo->z - FixedDiv(P_GetPlayerHeight(player) - player->mo->height, 3*FRACUNIT);

		if (!(player->mo->eflags & MFE_VERTICALFLIP) && zheight < player->mo->floorz && !(mobjinfo[type].flags & MF_NOCLIPHEIGHT))
			zheight = player->mo->floorz;
		else if (player->mo->eflags & MFE_VERTICALFLIP && zheight + FixedMul(mobjinfo[type].height, player->mo->scale) > player->mo->ceilingz && !(mobjinfo[type].flags & MF_NOCLIPHEIGHT))
			zheight = player->mo->ceilingz - FixedMul(mobjinfo[type].height, player->mo->scale);

		mobj = P_SpawnMobj(player->mo->x, player->mo->y, zheight, type);
		if (P_MobjWasRemoved(mobj))
			return;


		mobj->angle = player->drawangle;


		mobj->color = player->mo->color;
		mobj->skin = player->mo->skin;


		if (player->mo->eflags & MFE_VERTICALFLIP)
			mobj->flags2 |= MF2_OBJECTFLIP;
		mobj->eflags |= (player->mo->eflags & MFE_VERTICALFLIP);


		P_SetScale(mobj, player->mo->scale, true);

		if (type == MT_THOK)  
		{
			mobj->frame = FF_TRANS70;
			mobj->fuse = mobj->tics;
		}
	}

	if (!P_MobjWasRemoved(mobj))
		P_SetTarget(&mobj->target, player->mo);  
	G_GhostAddThok();
}






void P_SpawnSpinMobj(player_t *player, mobjtype_t type)
{
	mobj_t *mobj;
	fixed_t zheight;

	if (player->skincolor == 0)
		return;

	if (player->spectator)
		return;

	if (!type)
		return;

	if (type == MT_GHOST)
		mobj = P_SpawnGhostMobj(player->mo);  
	else
	{
		if (player->mo->eflags & MFE_VERTICALFLIP)
			zheight = player->mo->z + player->mo->height + FixedDiv(P_GetPlayerHeight(player) - player->mo->height, 3*FRACUNIT) - FixedMul(mobjinfo[type].height, player->mo->scale);
		else
			zheight = player->mo->z - FixedDiv(P_GetPlayerHeight(player) - player->mo->height, 3*FRACUNIT);

		if (!(player->mo->eflags & MFE_VERTICALFLIP) && zheight < player->mo->floorz && !(mobjinfo[type].flags & MF_NOCLIPHEIGHT))
			zheight = player->mo->floorz;
		else if (player->mo->eflags & MFE_VERTICALFLIP && zheight + FixedMul(mobjinfo[type].height, player->mo->scale) > player->mo->ceilingz && !(mobjinfo[type].flags & MF_NOCLIPHEIGHT))
			zheight = player->mo->ceilingz - FixedMul(mobjinfo[type].height, player->mo->scale);

		mobj = P_SpawnMobj(player->mo->x, player->mo->y, zheight, type);
		if (P_MobjWasRemoved(mobj))
			return;


		mobj->angle = player->drawangle;


		mobj->color = player->mo->color;
		mobj->skin = player->mo->skin;


		if (player->mo->eflags & MFE_VERTICALFLIP)
			mobj->flags2 |= MF2_OBJECTFLIP;
		mobj->eflags |= (player->mo->eflags & MFE_VERTICALFLIP);


		P_SetScale(mobj, player->mo->scale, true);

		if (type == MT_THOK)  
		{
			mobj->frame = FF_TRANS70;
			mobj->fuse = mobj->tics;
		}
	}

	if (!P_MobjWasRemoved(mobj))
		P_SetTarget(&mobj->target, player->mo);  
}

                                             
   
                                                              
                                                                        
   
                                                                          
                                                                   
                                   
   
                                                    
                      
   
    
void P_DoPlayerFinish(player_t *player)
{
	if (player->pflags & PF_FINISHED)
		return;

	player->pflags |= PF_FINISHED;
	P_GiveFinishFlags(player);

	if (netgame)
		CONS_Printf(M_GetText("%s has completed the level.\n"), player_names[player-players]);

	player->powers[pw_underwater] = 0;
	player->powers[pw_spacetime] = 0;
	P_RestoreMusic(player);
}





void P_DoPlayerExit(player_t *player, boolean finishedflag)
{
	if (player->exiting)
		return;

	if (cv_allowexitlevel.value == 0 && !G_PlatformGametype())
		return;
	else if (gametyperules & GTR_RACE)  
	{
		if (!countdown)  
			countdown = (cv_countdowntime.value - 1)*TICRATE + 1;  

		player->exiting = 3*TICRATE;

		if (!countdown2)
			countdown2 = (8 + cv_countdowntime.value)*TICRATE + 1;  

		if (P_CheckRacers())
			player->exiting = (14*TICRATE)/5 + 1;
	}
	else
	{
		player->exiting = (14*TICRATE)/5 + 2;  
		if (finishedflag)
			player->pflags |= PF_FINISHED;  
	}


	if (player->climbing)
	{
		player->climbing = 0;
		player->pflags |= P_GetJumpFlags(player);
		P_SetMobjState(player->mo, S_PLAY_JUMP);
	}
	else if (player->pflags & PF_STARTDASH)
	{
		player->pflags &= ~PF_STARTDASH;
		P_SetMobjState(player->mo, S_PLAY_STND);
	}
	player->powers[pw_underwater] = 0;
	player->powers[pw_spacetime] = 0;
	P_RestoreMusic(player);
}

boolean P_InSpaceSector(mobj_t *mo)  
{
	sector_t *sector = mo->subsector->sector;
	fixed_t topheight, bottomheight;

	if (sector->specialflags & SSF_OUTERSPACE)
		return true;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS))
				continue;

			if (!(rover->master->frontsector->specialflags & SSF_OUTERSPACE))
				continue;
			topheight    = P_GetFFloorTopZAt   (rover, mo->x, mo->y);
			bottomheight = P_GetFFloorBottomZAt(rover, mo->x, mo->y);

			if (mo->z + (mo->height/2) > topheight)
				continue;

			if (mo->z + (mo->height/2) < bottomheight)
				continue;

			return true;
		}
	}

	return false;  
}






boolean P_PlayerHitFloor(player_t *player, boolean dorollstuff)
{
	boolean clipmomz;

	I_Assert(player->mo != NULL);

	if (player->powers[pw_carry] == CR_NIGHTSMODE)
		return true;

	if ((clipmomz = !(P_CheckDeathPitCollide(player->mo))) && player->mo->health && !player->spectator)
	{
		if (dorollstuff)
		{
			if ((player->charability2 == CA2_SPINDASH) && !((player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_THOKKED) && !(player->charability == CA_THOK && player->secondjump)
			&& (player->cmd.buttons & BT_SPIN) && (FixedHypot(player->mo->momx, player->mo->momy) >= (5*player->mo->scale)))
				player->pflags = (player->pflags|PF_SPINNING) & ~PF_THOKKED;
			else if (!(player->pflags & PF_STARTDASH))
				player->pflags &= ~PF_SPINNING;
		}

		if (player->pflags & PF_BOUNCING)
		{
			if (dorollstuff && player->mo->state-states != S_PLAY_BOUNCE_LANDING)
			{
				P_MobjCheckWater(player->mo);
				player->mo->momz *= -1;
				P_DoAbilityBounce(player, true);
				if (player->scoreadd)
					player->scoreadd--;
			}
			else
				player->mo->z += P_MobjFlip(player->mo);
			clipmomz = false;
		}
		else
		{
			P_MobjCheckWater(player->mo);
			if (player->pflags & PF_SPINNING)
			{
				if (!(player->pflags & PF_STARTDASH) && player->panim != PA_ROLL && player->panim != PA_ETC
				&& player->panim != PA_ABILITY && player->panim != PA_ABILITY2)
				{
					P_SetMobjState(player->mo, S_PLAY_ROLL);
					S_StartSound(player->mo, sfx_spin);
				}
			}
			else if (player->pflags & PF_GLIDING)  
			{
				if (dorollstuff)
				{
					player->skidtime = TICRATE;
					P_SetMobjState(player->mo, S_PLAY_GLIDE);
					P_SpawnSkidDust(player, player->mo->radius, true);  
					player->mo->tics = -1;
				}
				else if (!player->skidtime)
					player->pflags &= ~PF_GLIDING;
			}
			else if (player->charability == CA_GLIDEANDCLIMB && player->pflags & PF_THOKKED && !(player->pflags & (PF_JUMPED|PF_SHIELDABILITY))
			&& (player->mo->floorz != player->mo->watertop) && player->mo->state-states == S_PLAY_FALL)
			{
				if (player->mo->state-states != S_PLAY_GLIDE_LANDING)
				{
					P_ResetPlayer(player);
					P_SetMobjState(player->mo, S_PLAY_GLIDE_LANDING);
					player->pflags |= PF_STASIS;
					if (player->speed > FixedMul(player->runspeed, player->mo->scale))
						player->skidtime += player->mo->tics;
					player->mo->momx = ((player->mo->momx - player->cmomx)/2) + player->cmomx;
					player->mo->momy = ((player->mo->momy - player->cmomy)/2) + player->cmomy;
					if (player->powers[pw_super])
					{
						P_Earthquake(player->mo, player->mo, 256*FRACUNIT);
						S_StartSound(player->mo, sfx_s3k49);
					}
					else
						S_StartSound(player->mo, sfx_s3k4c);
				}
			}
			else if (player->charability2 == CA2_MELEE
				&& ((player->panim == PA_ABILITY2) || (player->charability == CA_TWINSPIN && player->panim == PA_ABILITY && player->cmd.buttons & (BT_JUMP|BT_SPIN))))
			{
				if (player->mo->state-states != S_PLAY_MELEE_LANDING)
				{
					mobjtype_t type = player->revitem;
					P_SetMobjState(player->mo, S_PLAY_MELEE_LANDING);
					player->mo->tics = (player->mo->movefactor == FRACUNIT) ? TICRATE/2 : (FixedDiv(35<<(FRACBITS-1), FixedSqrt(player->mo->movefactor)))>>FRACBITS;
					S_StartSound(player->mo, sfx_s3k8b);
					player->pflags |= PF_FULLSTASIS;
					player->powers[pw_strong] = STR_MELEE;


					if (type)
					{
						UINT8 i = 0;
						angle_t throwang = -(2*ANG30);
						fixed_t xo = P_ReturnThrustX(player->mo, player->drawangle, 16*player->mo->scale);
						fixed_t yo = P_ReturnThrustY(player->mo, player->drawangle, 16*player->mo->scale);
						fixed_t zo = 6*player->mo->scale;
						fixed_t mu = FixedMul(player->maxdash, player->mo->scale);
						fixed_t mu2 = FixedHypot(player->mo->momx, player->mo->momy);
						fixed_t ev;
						mobj_t *missile = NULL;
						if (mu2 < mu)
							mu2 = mu;
						ev = (50*FRACUNIT - (mu/25))/50;
						while (i < 5)
						{
							missile = P_SpawnMobjFromMobj(player->mo, xo, yo, zo, type);
							if (!P_MobjWasRemoved(missile))
							{
								P_SetTarget(&missile->target, player->mo);
								missile->angle = throwang + player->drawangle;
								P_Thrust(missile, player->drawangle + ANGLE_90,
									P_ReturnThrustY(missile, throwang, mu));  
								P_Thrust(missile, player->drawangle, mu2);  
								P_SetObjectMomZ(missile, (4 + ((i&1)<<1))*FRACUNIT, true);
								missile->momz += player->mo->pmomz;
								missile->fuse = TICRATE/2;
								missile->extravalue2 = ev;
							}

							i++;
							throwang += ANG30;
						}
						if (mobjinfo[type].seesound && missile)
							S_StartSound(missile, missile->info->seesound);
					}
				}
			}
			else if (player->charability == CA_GLIDEANDCLIMB && (player->mo->state-states == S_PLAY_GLIDE_LANDING))
				;
			else if (player->charability2 == CA2_GUNSLINGER && player->panim == PA_ABILITY2)
				;
			else if (dorollstuff && player->panim != PA_IDLE && player->panim != PA_WALK && player->panim != PA_RUN && player->panim != PA_DASH)
			{
				fixed_t runspd = FixedMul(player->runspeed, player->mo->scale);



				if (player->powers[pw_super])
					runspd = FixedMul(runspd, 5*FRACUNIT/3);

				runspd = FixedMul(runspd, player->mo->movefactor);

				if (maptol & TOL_2D)
					runspd = FixedMul(runspd, 2*FRACUNIT/3);

				if (player->cmomx || player->cmomy)
				{
					if (player->charflags & SF_DASHMODE && player->dashmode >= DASHMODE_THRESHOLD && player->panim != PA_DASH)
						P_SetMobjState(player->mo, S_PLAY_DASH);
					else if (player->speed >= runspd
					&& (player->panim != PA_RUN || player->mo->state-states == S_PLAY_FLOAT_RUN))
						P_SetMobjState(player->mo, S_PLAY_RUN);
					else if ((player->rmomx || player->rmomy)
					&& (player->panim != PA_WALK || player->mo->state-states == S_PLAY_FLOAT))
						P_SetMobjState(player->mo, S_PLAY_WALK);
					else if (!player->rmomx && !player->rmomy && player->panim != PA_IDLE)
						P_SetMobjState(player->mo, S_PLAY_STND);
				}
				else
				{
					if (player->charflags & SF_DASHMODE && player->dashmode >= DASHMODE_THRESHOLD && player->panim != PA_DASH)
						P_SetMobjState(player->mo, S_PLAY_DASH);
					else if (player->speed >= runspd
					&& (player->panim != PA_RUN || player->mo->state-states == S_PLAY_FLOAT_RUN))
						P_SetMobjState(player->mo, S_PLAY_RUN);
					else if ((player->mo->momx || player->mo->momy)
					&& (player->panim != PA_WALK || player->mo->state-states == S_PLAY_FLOAT))
						P_SetMobjState(player->mo, S_PLAY_WALK);
					else if (!player->mo->momx && !player->mo->momy && player->panim != PA_IDLE)
						P_SetMobjState(player->mo, S_PLAY_STND);
				}
			}

			if (!(player->pflags & PF_GLIDING))
				player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE);
			player->pflags &= ~(PF_STARTJUMP|PF_THOKKED|PF_CANCARRY );
			player->secondjump = 0;
			player->glidetime = 0;
			player->climbing = 0;
			player->powers[pw_tailsfly] = 0;

			if (player->pflags & PF_SHIELDABILITY)
			{
				player->pflags &= ~PF_SHIELDABILITY;

				if ((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL)  
				{
					if (player->mo->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER))  
						S_StartSound(player->mo, sfx_s3k4c);
					else  
					{
						S_StartSound(player->mo, sfx_s3k47);
						P_ElementalFire(player, true);
					}
					P_SetObjectMomZ(player->mo,
					(player->mo->eflags & MFE_UNDERWATER)
					? 6*FRACUNIT/5
					: 5*FRACUNIT/2,
					false);
					P_SetMobjState(player->mo, S_PLAY_FALL);
					player->mo->momx = player->mo->momy = 0;
					clipmomz = false;
				}
				else if ((player->powers[pw_shield] & SH_NOSTACK) == SH_BUBBLEWRAP)  
				{
					P_DoBubbleBounce(player);
					clipmomz = false;
				}
			}
		}
	}

	return clipmomz;
}

boolean P_InQuicksand(mobj_t *mo)  
{
	sector_t *sector = mo->subsector->sector;
	fixed_t topheight, bottomheight;

	fixed_t flipoffset = ((mo->eflags & MFE_VERTICALFLIP) ? (mo->height/2) : 0);

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS))
				continue;

			if (!(rover->fofflags & FOF_QUICKSAND))
				continue;

			topheight    = P_GetFFloorTopZAt   (rover, mo->x, mo->y);
			bottomheight = P_GetFFloorBottomZAt(rover, mo->x, mo->y);

			if (mo->z + flipoffset > topheight)
				continue;

			if (mo->z + (mo->height/2) + flipoffset < bottomheight)
				continue;

			return true;
		}
	}

	return false;  
}

boolean P_InJumpFlipSector(mobj_t *mo)  
{
	sector_t *sector = mo->subsector->sector;
	fixed_t topheight, bottomheight;

	if (sector->specialflags & SSF_JUMPFLIP)
		return true;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS))
				continue;

			if (!(rover->master->frontsector->specialflags & SSF_JUMPFLIP))
				continue;
			topheight    = P_GetFFloorTopZAt   (rover, mo->x, mo->y);
			bottomheight = P_GetFFloorBottomZAt(rover, mo->x, mo->y);

			if (mo->z + (mo->height/2) > topheight)
				continue;

			if (mo->z + (mo->height/2) < bottomheight)
				continue;

			return true;
		}
	}

	return false;  
}

static boolean P_PlayerCanBust(player_t *player, ffloor_t *rover)
{
	if (!(rover->fofflags & FOF_EXISTS))
		return false;

	if (!(rover->fofflags & FOF_BUSTUP))
		return false;

	                                                               
                 

	switch (rover->busttype)
	{
	case BT_TOUCH:  
		return true;
	case BT_SPINBUST:  
		if ((player->pflags & PF_SPINNING) && !(player->pflags & PF_STARTDASH))
			return true;

		if ((player->pflags & PF_JUMPED) && !(player->pflags & PF_NOJUMPDAMAGE))
			return true;

		 
	case BT_REGULAR:

		if ((player->pflags & PF_SPINNING) && !(player->pflags & PF_JUMPED))
			return true;


		if (player->charflags & SF_CANBUSTWALLS || player->powers[pw_strong] & (STR_WALL|STR_FLOOR|STR_CEILING|STR_DASH))
			return true;


		if (player->powers[pw_super])
			return true;


		if (player->pflags & PF_DRILLING)
			return true;


		if (metalrecording)
			return true;

		 
	case BT_STRONG:  
		if (player->charflags & SF_CANBUSTWALLS || player->powers[pw_strong] & (STR_WALL|STR_FLOOR|STR_CEILING))
			return true;

		break;
	}
	return false;
}

static void P_CheckBustableBlocks(player_t *player)
{
	msecnode_t *node;
	fixed_t oldx;
	fixed_t oldy;

	if ((netgame || multiplayer) && player->spectator)
		return;

	oldx = player->mo->x;
	oldy = player->mo->y;

	if (!((player->powers[pw_strong] & (STR_FLOOR|STR_CEILING)) && (!(player->powers[pw_strong] & STR_WALL)) && (!(player->charflags & SF_CANBUSTWALLS))))  
	{
		P_UnsetThingPosition(player->mo);
		player->mo->x += player->mo->momx;
		player->mo->y += player->mo->momy;
		P_SetThingPosition(player->mo);
	}

	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		ffloor_t *rover;
		fixed_t topheight, bottomheight;

		if (!node->m_sector)
			break;

		if (!node->m_sector->ffloors)
			continue;

		for (rover = node->m_sector->ffloors; rover; rover = rover->next)
		{
			if (!P_PlayerCanBust(player, rover))
				continue;

			topheight = P_GetFOFTopZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);
			bottomheight = P_GetFOFBottomZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);


			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				if ((player->powers[pw_strong] & STR_FLOOR) && (!(player->powers[pw_strong] & STR_CEILING)) && player->mo->z > topheight)
					continue;

				if ((player->powers[pw_strong] & STR_CEILING) && (!(player->powers[pw_strong] & STR_FLOOR)) && player->mo->z + player->mo->height < bottomheight)
					continue;
			}
			else
			{
				if ((player->powers[pw_strong] & STR_FLOOR) && (!(player->powers[pw_strong] & STR_CEILING)) && player->mo->z < bottomheight)
					continue;

				if ((player->powers[pw_strong] & STR_CEILING) && (!(player->powers[pw_strong] & STR_FLOOR)) && player->mo->z + player->mo->height > topheight)
					continue;
			}
			if (player->powers[pw_strong] & (STR_FLOOR|STR_CEILING))
			{
				topheight -= player->mo->momz;
				bottomheight -= player->mo->momz;
			}


			if (rover->bustflags & FB_ONLYBOTTOM)
			{
				if (player->mo->z + player->mo->momz + player->mo->height < bottomheight)
					continue;

				if (player->mo->z + player->mo->height > bottomheight)
					continue;
			}
			else
			{
				switch (rover->busttype)
				{
				case BT_TOUCH:
					if (player->mo->z + player->mo->momz > topheight)
						continue;

					if (player->mo->z + player->mo->momz + player->mo->height < bottomheight)
						continue;

					break;
				case BT_SPINBUST:
					if (player->mo->z + player->mo->momz > topheight)
						continue;

					if (player->mo->z + player->mo->height < bottomheight)
						continue;

					break;
				default:
					if (player->mo->z >= topheight)
						continue;

					if (player->mo->z + player->mo->height < bottomheight)
						continue;

					break;
				}
			}


			if (((rover->busttype == BT_TOUCH) || (rover->busttype == BT_SPINBUST)) && player->mo->z >= topheight)
				player->mo->momz >>= 1;
			else if (rover->busttype == BT_TOUCH)
			{
				player->mo->momx >>= 1;
				player->mo->momy >>= 1;
			}




			EV_CrumbleChain(NULL, rover);  


			if (rover->bustflags & FB_EXECUTOR)
				P_LinedefExecute(rover->busttag, player->mo, node->m_sector);

			goto bustupdone;
		}
	}
bustupdone:
	if (!((player->powers[pw_strong] & (STR_FLOOR|STR_CEILING)) && (!(player->powers[pw_strong] & STR_WALL)) && (!(player->charflags & SF_CANBUSTWALLS))))
	{
		P_UnsetThingPosition(player->mo);
		player->mo->x = oldx;
		player->mo->y = oldy;
		P_SetThingPosition(player->mo);
	}
}

static void P_CheckBouncySectors(player_t *player)
{
	msecnode_t *node;
	fixed_t oldx;
	fixed_t oldy;
	fixed_t oldz;
	vector3_t momentum;

	oldx = player->mo->x;
	oldy = player->mo->y;
	oldz = player->mo->z;

	P_UnsetThingPosition(player->mo);
	player->mo->x += player->mo->momx;
	player->mo->y += player->mo->momy;
	player->mo->z += player->mo->momz;
	P_SetThingPosition(player->mo);

	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		ffloor_t *rover;

		if (!node->m_sector)
			break;

		if (!node->m_sector->ffloors)
			continue;

		for (rover = node->m_sector->ffloors; rover; rover = rover->next)
		{
			fixed_t topheight, bottomheight;

			if (!(rover->fofflags & FOF_EXISTS))
				continue;  



			if (!udmf && GETSECSPECIAL(rover->master->frontsector->special, 1) == 15)
			{
				rover->fofflags |= FOF_BOUNCY;
				rover->bouncestrength = P_AproxDistance(rover->master->dx, rover->master->dy)/100;
			}

			if (!(rover->fofflags & FOF_BOUNCY))
				continue;

			topheight = P_GetFOFTopZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);
			bottomheight = P_GetFOFBottomZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);

			if (player->mo->z > topheight)
				continue;

			if (player->mo->z + player->mo->height < bottomheight)
				continue;

			if (oldz < P_GetFOFTopZ(player->mo, node->m_sector, rover, oldx, oldy, NULL)
					&& oldz + player->mo->height > P_GetFOFBottomZ(player->mo, node->m_sector, rover, oldx, oldy, NULL))
			{
				player->mo->momx = -FixedMul(player->mo->momx,rover->bouncestrength);
				player->mo->momy = -FixedMul(player->mo->momy,rover->bouncestrength);
			}
			else
			{
				pslope_t *slope = (abs(oldz - topheight) < abs(oldz + player->mo->height - bottomheight)) ? *rover->t_slope : *rover->b_slope;

				momentum.x = player->mo->momx;
				momentum.y = player->mo->momy;
				momentum.z = player->mo->momz*2;

				if (slope)
					P_ReverseQuantizeMomentumToSlope(&momentum, slope);

				momentum.z = -FixedMul(momentum.z,rover->bouncestrength)/2;

				if (abs(momentum.z) < (rover->bouncestrength*2))
					goto bouncydone;

				if (momentum.z > FixedMul(24*FRACUNIT, player->mo->scale))  
					momentum.z = FixedMul(24*FRACUNIT, player->mo->scale);
				else if (momentum.z < -FixedMul(24*FRACUNIT, player->mo->scale))
					momentum.z = -FixedMul(24*FRACUNIT, player->mo->scale);

				if (slope)
					P_QuantizeMomentumToSlope(&momentum, slope);

				player->mo->momx = momentum.x;
				player->mo->momy = momentum.y;
				player->mo->momz = momentum.z;

				if (player->pflags & PF_SPINNING)
				{
					player->pflags |= PF_THOKKED;
				}
			}
			goto bouncydone;
		}
	}
bouncydone:
	P_UnsetThingPosition(player->mo);
	player->mo->x = oldx;
	player->mo->y = oldy;
	player->mo->z = oldz;
	P_SetThingPosition(player->mo);
}

static void P_CheckQuicksand(player_t *player)
{
	ffloor_t *rover;
	fixed_t sinkspeed;
	fixed_t topheight, bottomheight;

	if (!(player->mo->subsector->sector->ffloors && P_MobjFlip(player->mo)*player->mo->momz <= 0))
		return;

	for (rover = player->mo->subsector->sector->ffloors; rover; rover = rover->next)
	{
		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!(rover->fofflags & FOF_QUICKSAND))
			continue;

		topheight    = P_GetFFloorTopZAt   (rover, player->mo->x, player->mo->y);
		bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);

		if (topheight >= player->mo->z && bottomheight < player->mo->z + player->mo->height)
		{
			sinkspeed = FixedDiv(rover->sinkspeed,TICRATE*FRACUNIT);

			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				fixed_t ceilingheight = P_GetCeilingZ(player->mo, player->mo->subsector->sector, player->mo->x, player->mo->y, NULL);

				player->mo->z += sinkspeed;

				if (player->mo->z + player->mo->height >= ceilingheight)
					player->mo->z = ceilingheight - player->mo->height;

				if (player->mo->momz <= 0)
					P_PlayerHitFloor(player, false);
			}
			else
			{
				fixed_t floorheight = P_GetFloorZ(player->mo, player->mo->subsector->sector, player->mo->x, player->mo->y, NULL);

				player->mo->z -= sinkspeed;

				if (player->mo->z <= floorheight)
					player->mo->z = floorheight;

				if (player->mo->momz >= 0)
					P_PlayerHitFloor(player, false);
			}

			player->mo->momx = FixedMul(player->mo->momx, rover->friction);
			player->mo->momy = FixedMul(player->mo->momy, rover->friction);
		}
	}
}






static void P_CheckSneakerAndLivesTimer(player_t *player)
{
	if (player->powers[pw_extralife] == 1)  
		P_RestoreMusic(player);

	if (player->powers[pw_sneakers] == 1)
		P_RestoreMusic(player);
}






static void P_CheckUnderwaterAndSpaceTimer(player_t *player)
{
	tic_t timeleft = (player->powers[pw_spacetime]) ? player->powers[pw_spacetime] : player->powers[pw_underwater];

	if (player->exiting || (player->pflags & PF_FINISHED))
		player->powers[pw_underwater] = player->powers[pw_spacetime] = 0;

	timeleft--;  

	if ((timeleft == 11*TICRATE)  
	 || (timeleft ==  9*TICRATE)  
	 || (timeleft ==  7*TICRATE)  
	 || (timeleft ==  5*TICRATE)  
	 || (timeleft ==  3*TICRATE)  
	 || (timeleft ==  1*TICRATE)  
	) {
		fixed_t height = (player->mo->eflags & MFE_VERTICALFLIP)
		? player->mo->z - FixedMul(8*FRACUNIT + mobjinfo[MT_DROWNNUMBERS].height, FixedMul(player->mo->scale, player->shieldscale))
		: player->mo->z + player->mo->height + FixedMul(8*FRACUNIT, FixedMul(player->mo->scale, player->shieldscale));

		mobj_t *numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, height, MT_DROWNNUMBERS);
		if (!P_MobjWasRemoved(numbermobj))
		{
			timeleft /= (2*TICRATE);  

			if (player->charflags & SF_MACHINE)
			{
				S_StartSound(player->mo, sfx_buzz1);
				timeleft += 6;
			}
			else
				S_StartSound(player->mo, sfx_dwnind);

			if (!P_MobjWasRemoved(numbermobj))
			{
				if (timeleft)  
					P_SetMobjState(numbermobj, numbermobj->info->spawnstate+timeleft);

				P_SetTarget(&numbermobj->target, player->mo);
				P_SetScale(numbermobj, player->mo->scale, true);
				numbermobj->threshold = 40;
			}
		}
	}

	else if (timeleft == 1)
	{
		if ((netgame || multiplayer) && P_IsLocalPlayer(player))
			S_ChangeMusic(mapmusname, mapmusflags, true);

		if (player->powers[pw_spacetime] == 1)
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_SPACEDROWN);
		else
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_DROWNED);
	}

	if (!(player->mo->eflags & MFE_UNDERWATER) && player->powers[pw_underwater])
	{
		if (player->powers[pw_underwater] <= 12*TICRATE + 1)
		{
			player->powers[pw_underwater] = 0;
			P_RestoreMusic(player);
		}
		else
			player->powers[pw_underwater] = 0;
	}

	if (player->powers[pw_spacetime] > 1 && !P_InSpaceSector(player->mo))
		player->powers[pw_spacetime] = 0;


	if (P_IsLocalPlayer(player) && !player->bot)
	{
		if ((player->powers[pw_underwater] == 25*TICRATE + 1)
		|| (player->powers[pw_underwater] == 20*TICRATE + 1)
		|| (player->powers[pw_underwater] == 15*TICRATE + 1))
			S_StartSound(NULL, sfx_wtrdng);

		if (player->powers[pw_underwater] == 11*TICRATE + 1
		&& player == &players[consoleplayer])
		{
			P_PlayJingle(player, JT_DROWN);
		}
	}
}






static void P_CheckInvincibilityTimer(player_t *player)
{
	if (!player->powers[pw_invulnerability])
		return;

	if (mariomode && !player->powers[pw_super])
		player->mo->color = (UINT16)(SKINCOLOR_RUBY + (leveltime % (FIRSTSUPERCOLOR - SKINCOLOR_RUBY)));  
	else if (leveltime % (TICRATE/7) == 0)
	{
		mobj_t *sparkle = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_IVSP);
		if (!P_MobjWasRemoved(sparkle))
			P_SetScale(sparkle, player->mo->scale, true);
	}


	if (player->powers[pw_invulnerability] == 1)
	{
		if (!player->powers[pw_super])
		{
			if (mariomode)
			{
				if ((player->powers[pw_shield] & SH_STACK) == SH_FIREFLOWER)
				{
					player->mo->color = SKINCOLOR_WHITE;
					G_GhostAddColor(GHC_FIREFLOWER);
				}
				else
				{
					player->mo->color = P_GetPlayerColor(player);
					G_GhostAddColor(GHC_NORMAL);
				}
			}


			P_SpawnShieldOrb(player);
		}

		if (!player->powers[pw_super] || (mapheaderinfo[gamemap-1]->levelflags & LF_NOSSMUSIC))
			P_RestoreMusic(player);
	}
}






static void P_DoBubbleBreath(player_t *player)
{
	fixed_t x = player->mo->x;
	fixed_t y = player->mo->y;
	fixed_t z = player->mo->z;
	mobj_t *bubble = NULL;

	if (!(player->mo->eflags & MFE_UNDERWATER) || ((player->powers[pw_shield] & SH_PROTECTWATER) && !(player->powers[pw_carry] == CR_NIGHTSMODE)) || player->spectator)
		return;

	if (player->charflags & SF_MACHINE)
	{
		if (player->powers[pw_underwater] && P_RandomChance((128-(player->powers[pw_underwater]/4))*FRACUNIT/256))
		{
			fixed_t r = player->mo->radius>>FRACBITS;
			x += (P_RandomRange(r, -r)<<FRACBITS);
			y += (P_RandomRange(r, -r)<<FRACBITS);
			z += (P_RandomKey(player->mo->height>>FRACBITS)<<FRACBITS);
			bubble = P_SpawnMobj(x, y, z, MT_WATERZAP);
			if (!P_MobjWasRemoved(bubble))
				S_StartSound(bubble, sfx_beelec);
		}
	}
	else
	{
		if (player->mo->eflags & MFE_VERTICALFLIP)
			z += player->mo->height - FixedDiv(player->mo->height,5*(FRACUNIT/4));
		else
			z += FixedDiv(player->mo->height,5*(FRACUNIT/4));

		if (P_RandomChance(FRACUNIT/16))
			bubble = P_SpawnMobj(x, y, z, MT_SMALLBUBBLE);
		else if (P_RandomChance(3*FRACUNIT/256))
			bubble = P_SpawnMobj(x, y, z, MT_MEDIUMBUBBLE);
	}

	if (bubble)
	{
		bubble->threshold = 42;
		P_SetScale(bubble, player->mo->scale, true);
	}


	if (player->powers[pw_tailsfly] && (leveltime & 1) && player->charability != CA_SWIM)
	{
		fixed_t radius = player->mo->radius;
		angle_t fa = ((leveltime%45)*FINEANGLES/8) & FINEMASK;
		fixed_t stirwaterx = FixedMul(FINECOSINE(fa),radius);
		fixed_t stirwatery = FixedMul(FINESINE(fa),radius);
		fixed_t stirwaterz;

		if (player->mo->eflags & MFE_VERTICALFLIP)
			stirwaterz = player->mo->z + player->mo->height - (4<<FRACBITS);
		else
			stirwaterz = player->mo->z + (4<<FRACBITS);

		bubble = P_SpawnMobj(
			player->mo->x + stirwaterx,
			player->mo->y + stirwatery,
			stirwaterz, MT_SMALLBUBBLE);
		if (!P_MobjWasRemoved(bubble))
			P_SetScale(bubble, player->mo->scale, true);

		bubble = P_SpawnMobj(
			player->mo->x - stirwaterx,
			player->mo->y - stirwatery,
			stirwaterz, MT_SMALLBUBBLE);
		if (!P_MobjWasRemoved(bubble))
			P_SetScale(bubble, player->mo->scale, true);
	}
}






static void P_DoPlayerHeadSigns(player_t *player)
{
	mobj_t *sign = NULL;

	if (G_TagGametype())
	{


		if (player->pflags & PF_TAGIT && (!P_IsLocalPlayer(player) || consoleplayer != displayplayer || splitscreen))
		{
			sign = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_TAG);
			if (!P_MobjWasRemoved(sign))
			{
				sign->x = player->mo->x;
				sign->y = player->mo->y;
				sign->z = player->mo->z;
				sign->old_x = player->mo->old_x;
				sign->old_y = player->mo->old_y;
				sign->old_z = player->mo->old_z;

				if (!(player->mo->eflags & MFE_VERTICALFLIP))
				{
					sign->z += player->mo->height;
					sign->old_z += player->mo->height;
				}
				else
				{
					sign->z -= mobjinfo[MT_TAG].height;
					sign->old_z -= mobjinfo[MT_TAG].height;
				}
			}
		}
	}
	else if ((gametyperules & GTR_TEAMFLAGS) && (player->gotflag & (GF_REDFLAG|GF_BLUEFLAG)))  
	{


		if (!P_IsLocalPlayer(player) || consoleplayer != displayplayer || splitscreen)
		{
			fixed_t zofs;
			boolean player_is_flipped = (player->mo->eflags & MFE_VERTICALFLIP) > 0;

			zofs = player->mo->momz;
			if (player_is_flipped)
			{
				zofs += player->mo->height - P_GetPlayerHeight(player) - mobjinfo[MT_GOTFLAG].height - FixedMul(16 * FRACUNIT, player->mo->scale);
			}
			else
			{
				zofs += P_GetPlayerHeight(player) + FixedMul(16 * FRACUNIT, player->mo->scale);
			}

			sign = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_GOTFLAG);
			if (!P_MobjWasRemoved(sign))
			{
				sign->x = player->mo->x;
				sign->y = player->mo->y;
				sign->z = player->mo->z + zofs;
				sign->old_x = player->mo->old_x;
				sign->old_y = player->mo->old_y;
				sign->old_z = player->mo->old_z + zofs;

				if (player_is_flipped)
				{
					sign->eflags |= MFE_VERTICALFLIP;
				}

				if (player->gotflag & GF_REDFLAG)
					sign->frame = 1|FF_FULLBRIGHT;
				else  
					sign->frame = 2|FF_FULLBRIGHT;
			}
		}

	if (!P_MobjWasRemoved(sign) && splitscreen)  
	{
		if (player == &players[displayplayer])
			sign->drawonlyforplayer = &players[secondarydisplayplayer];
		else
			sign->drawonlyforplayer = &players[displayplayer];

#ifdef QUADS
		if (splitscreen > 1)  
		{
			UINT32 signframe = sign->frame;  
			sign = P_SpawnMobjFromMobj(sign, 0, 0, 0, MT_TAG);
			if (P_MobjWasRemoved(sign))
				return;

			sign->frame = signframe;
			if (player == &players[displayplayer] || player == &players[secondarydisplayplayer])
				sign->drawonlyforplayer = &players[thirddisplayplayer];
			else
				sign->drawonlyforplayer = &players[secondarydisplayplayer];

			if (splitscreen > 2)  
			{
				sign = P_SpawnMobjFromMobj(sign, 0, 0, 0, MT_TAG);
				if (P_MobjWasRemoved(sign))
					return;

				sign->frame = signframe;
				if (player != &players[fourthdisplayplayer])
					sign->drawonlyforplayer = &players[fourthdisplayplayer];
				else
					sign->drawonlyforplayer = &players[thirddisplayplayer];
			}
		}
#endif
	}
	}
}






static void P_DoClimbing(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;
	fixed_t platx;
	fixed_t platy;
	subsector_t *glidesector;
	boolean climb = true;

	platx = P_ReturnThrustX(player->mo, player->mo->angle, player->mo->radius + FixedMul(8*FRACUNIT, player->mo->scale));
	platy = P_ReturnThrustY(player->mo, player->mo->angle, player->mo->radius + FixedMul(8*FRACUNIT, player->mo->scale));

	glidesector = R_PointInSubsectorOrNull(player->mo->x + platx, player->mo->y + platy);

	{
		boolean floorclimb = false;
		boolean thrust = false;
		boolean boostup = false;
		boolean skyclimber = false;
		fixed_t floorheight, ceilingheight;

		if (!glidesector)
			floorclimb = true;
		else
		{
			floorheight   = P_GetSectorFloorZAt  (glidesector->sector, player->mo->x, player->mo->y);
			ceilingheight = P_GetSectorCeilingZAt(glidesector->sector, player->mo->x, player->mo->y);

			if (glidesector->sector->ffloors)
			{
				ffloor_t *rover;
				fixed_t topheight, bottomheight;

				for (rover = glidesector->sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_BLOCKPLAYER) || ((rover->fofflags & FOF_BUSTUP) && (player->charflags & SF_CANBUSTWALLS)))
						continue;

					floorclimb = true;

					topheight    = P_GetFFloorTopZAt   (rover, player->mo->x, player->mo->y);
					bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);


					if (rover->master->frontsector->floorspeed && rover->master->frontsector->ceilspeed == 42)
					{
						if ((!(player->mo->eflags & MFE_VERTICALFLIP) && (bottomheight < player->mo->z+player->mo->height)
							&& (topheight >= player->mo->z + FixedMul(16*FRACUNIT, player->mo->scale)))
						|| ((player->mo->eflags & MFE_VERTICALFLIP) && (topheight > player->mo->z)
							&& (bottomheight <= player->mo->z + player->mo->height - FixedMul(16*FRACUNIT, player->mo->scale))))
						{
							if (cmd->forwardmove != 0)
								player->mo->momz += rover->master->frontsector->floorspeed;
							else
							{
								player->mo->momz = rover->master->frontsector->floorspeed;
								climb = false;
							}
						}
					}


					if (player->mo->eflags & MFE_VERTICALFLIP)
					{

						if ((topheight >= player->mo->z + player->mo->height) && ((player->mo->z + player->mo->height + player->mo->momz) >= topheight))
						{
							fixed_t bottomheight2;
							ffloor_t *roverbelow;
							boolean foundfof = false;
							floorclimb = true;
							boostup = false;


							for (roverbelow = glidesector->sector->ffloors; roverbelow; roverbelow = roverbelow->next)
							{
								if (!(roverbelow->fofflags & FOF_EXISTS) || !(roverbelow->fofflags & FOF_BLOCKPLAYER) || ((rover->fofflags & FOF_BUSTUP) && (player->charflags & SF_CANBUSTWALLS)))
									continue;

								if (roverbelow == rover)
									continue;

								bottomheight2 = P_GetFFloorBottomZAt(roverbelow, player->mo->x, player->mo->y);
								if (bottomheight2 < topheight + FixedMul(16*FRACUNIT, player->mo->scale))
									foundfof = true;
							}

							if (!foundfof)
								player->mo->momz = 0;
						}


						if (topheight <= player->mo->z)
						{
							floorclimb = false;
							boostup = false;
							thrust = false;
						}


						if (bottomheight > player->mo->z + player->mo->height - FixedMul(16*FRACUNIT, player->mo->scale))
						{
							floorclimb = false;
							thrust = true;
							boostup = true;
						}
					}
					else
					{

						if ((bottomheight <= player->mo->z) && ((player->mo->z + player->mo->momz) <= bottomheight))
						{
							fixed_t topheight2;
							ffloor_t *roverbelow;
							boolean foundfof = false;
							floorclimb = true;
							boostup = false;


							for (roverbelow = glidesector->sector->ffloors; roverbelow; roverbelow = roverbelow->next)
							{
								if (!(roverbelow->fofflags & FOF_EXISTS) || !(roverbelow->fofflags & FOF_BLOCKPLAYER) || ((rover->fofflags & FOF_BUSTUP) && (player->charflags & SF_CANBUSTWALLS)))
									continue;

								if (roverbelow == rover)
									continue;

								topheight2 = P_GetFFloorTopZAt(roverbelow, player->mo->x, player->mo->y);
								if (topheight2 > bottomheight - FixedMul(16*FRACUNIT, player->mo->scale))
									foundfof = true;
							}

							if (!foundfof)
								player->mo->momz = 0;
						}


						if (bottomheight >= player->mo->z + player->mo->height)
						{
							floorclimb = false;
							boostup = false;
							thrust = false;
						}


						if (topheight < player->mo->z + FixedMul(16*FRACUNIT, player->mo->scale))
						{
							floorclimb = false;
							thrust = true;
							boostup = true;
						}
					}

					if (floorclimb)
					{
						if (rover->fofflags & FOF_CRUMBLE && !(netgame && player->spectator))
							EV_StartCrumble(rover->master->frontsector, rover, (rover->fofflags & FOF_FLOATBOB), player, rover->alpha, !(rover->fofflags & FOF_NORETURN));
						break;
					}
				}
			}


			if (player->mo->eflags & MFE_VERTICALFLIP)
			{

				if ((floorheight >= player->mo->z + player->mo->height) && ((player->mo->z + player->mo->height + player->mo->momz) >= floorheight))
				{
					boolean foundfof = false;
					floorclimb = true;


					if (glidesector->sector->ffloors)
					{
						fixed_t bottomheight;
						ffloor_t *rover;
						for (rover = glidesector->sector->ffloors; rover; rover = rover->next)
						{
							if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_BLOCKPLAYER) || ((rover->fofflags & FOF_BUSTUP) && (player->charflags & SF_CANBUSTWALLS)))
								continue;

							bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);
							if (bottomheight < floorheight + FixedMul(16*FRACUNIT, player->mo->scale))
							{
								foundfof = true;
								break;
							}
						}
					}

					if (!foundfof)
						player->mo->momz = 0;
				}


				if (!floorclimb && ceilingheight > player->mo->z + player->mo->height - FixedMul(16*FRACUNIT, player->mo->scale)
					&& (glidesector->sector->ceilingpic == skyflatnum || floorheight < (player->mo->z - FixedMul(8*FRACUNIT, player->mo->scale))))
				{
					thrust = true;
					boostup = true;

				}
			}
			else
			{

				if ((ceilingheight <= player->mo->z) && ((player->mo->z + player->mo->momz) <= ceilingheight))
				{
					boolean foundfof = false;
					floorclimb = true;


					if (glidesector->sector->ffloors)
					{
						fixed_t topheight;
						ffloor_t *rover;
						for (rover = glidesector->sector->ffloors; rover; rover = rover->next)
						{
							if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_BLOCKPLAYER) || ((rover->fofflags & FOF_BUSTUP) && (player->charflags & SF_CANBUSTWALLS)))
								continue;

							topheight = P_GetFFloorTopZAt(rover, player->mo->x, player->mo->y);
							if (topheight > ceilingheight - FixedMul(16*FRACUNIT, player->mo->scale))
							{
								foundfof = true;
								break;
							}
						}
					}

					if (!foundfof)
						player->mo->momz = 0;
				}


				if (player->mo->z > ceilingheight - FixedMul(16*FRACUNIT, player->mo->scale))
				{
					floorclimb = true;
					thrust = false;
					boostup = false;
				}


				if (!floorclimb && floorheight < player->mo->z + FixedMul(16*FRACUNIT, player->mo->scale)
					&& (glidesector->sector->ceilingpic == skyflatnum || ceilingheight > (player->mo->z + player->mo->height + FixedMul(8*FRACUNIT, player->mo->scale))))
				{
					thrust = true;
					boostup = true;

				}
			}


			if ((ceilingheight < player->mo->z) && glidesector->sector->ceilingpic == skyflatnum)
			{
				skyclimber = true;
			}


			if ((!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + FixedMul(16*FRACUNIT, player->mo->scale) < floorheight)
				|| ((player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + player->mo->height <= floorheight))
			{
				floorclimb = true;

				if (glidesector->sector->floorspeed)
				{
					if (cmd->forwardmove != 0)
						player->mo->momz += glidesector->sector->floorspeed;
					else
					{
						player->mo->momz = glidesector->sector->floorspeed;
						climb = false;
					}
				}
			}

			else if ((!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z >= ceilingheight)
				|| ((player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + player->mo->height - FixedMul(16*FRACUNIT, player->mo->scale) > ceilingheight))
			{
				floorclimb = true;

				if (glidesector->sector->ceilspeed)
				{
					if (cmd->forwardmove != 0)
						player->mo->momz += glidesector->sector->ceilspeed;
					else
					{
						player->mo->momz = glidesector->sector->ceilspeed;
						climb = false;
					}
				}
			}
		}

		if (player->lastsidehit != -1 && player->lastlinehit != -1)
		{
			thinker_t *think;
			scroll_t *scroller;
			angle_t sideangle;
			fixed_t dx, dy;

			for (think = thlist[THINK_MAIN].next; think != &thlist[THINK_MAIN]; think = think->next)
			{
				if (think->function.acp1 != (actionf_p1)T_Scroll)
					continue;

				scroller = (scroll_t *)think;

				if (scroller->type != sc_side)
					continue;

				if (scroller->affectee != player->lastsidehit)
					continue;

				if (scroller->accel)
				{
					dx = scroller->vdx;
					dy = scroller->vdy;
				}
				else
				{
					dx = scroller->dx;
					dy = scroller->dy;
				}

				if (cmd->forwardmove != 0)
				{
					player->mo->momz += dy;
					climb = true;
				}
				else
				{
					player->mo->momz = dy;
					climb = false;
				}

				sideangle = R_PointToAngle2(lines[player->lastlinehit].v2->x,lines[player->lastlinehit].v2->y,lines[player->lastlinehit].v1->x,lines[player->lastlinehit].v1->y);

				if (cmd->sidemove != 0)
				{
					P_Thrust(player->mo, sideangle, dx);
					climb = true;
				}
				else
				{
					P_InstaThrust(player->mo, sideangle, dx);
					climb = false;
				}
			}
		}

		if (cmd->sidemove != 0 || cmd->forwardmove != 0)
			climb = true;
		else
			climb = false;

		if (player->climbing && climb && (player->mo->momx || player->mo->momy || player->mo->momz)
			&& player->mo->state-states != S_PLAY_CLIMB)
			P_SetMobjState(player->mo, S_PLAY_CLIMB);
		else if ((!(player->mo->momx || player->mo->momy || player->mo->momz) || !climb) && player->mo->state-states != S_PLAY_CLING)
			P_SetMobjState(player->mo, S_PLAY_CLING);

		if (!floorclimb)
		{
			if (boostup)
			{
				P_SetObjectMomZ(player->mo, 2*FRACUNIT, true);
				if (cmd->forwardmove)
					player->mo->momz = 2*player->mo->momz/3;
			}
			if (thrust)
				P_Thrust(player->mo, player->mo->angle, FixedMul(4*FRACUNIT, player->mo->scale));  

			player->climbing = 0;
			player->pflags |= P_GetJumpFlags(player);
			P_SetMobjState(player->mo, S_PLAY_JUMP);
		}

		if (skyclimber)
		{
			player->climbing = 0;
			player->pflags |= P_GetJumpFlags(player);
			P_SetMobjState(player->mo, S_PLAY_JUMP);
		}
	}

	if (cmd->sidemove != 0 || cmd->forwardmove != 0)
		climb = true;
	else
		climb = false;

	if (player->climbing && climb && (player->mo->momx || player->mo->momy || player->mo->momz)
		&& player->mo->state-states != S_PLAY_CLIMB)
		P_SetMobjState(player->mo, S_PLAY_CLIMB);
	else if ((!(player->mo->momx || player->mo->momy || player->mo->momz) || !climb) && player->mo->state-states != S_PLAY_CLING)
		P_SetMobjState(player->mo, S_PLAY_CLING);

	if (cmd->buttons & BT_SPIN && !(player->pflags & PF_JUMPSTASIS))
	{
		player->climbing = 0;
		player->pflags |= P_GetJumpFlags(player);
		P_SetMobjState(player->mo, S_PLAY_JUMP);
		P_SetObjectMomZ(player->mo, 4*FRACUNIT, false);
		P_Thrust(player->mo, player->mo->angle, FixedMul(-4*FRACUNIT, player->mo->scale));
	}

#define CLIMBCONEMAX FixedAngle(90*FRACUNIT)
	if (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG)
	{
		angle_t angdiff = P_GetLocalAngle(player) - player->mo->angle;
		if (angdiff < ANGLE_180 && angdiff > CLIMBCONEMAX)
			P_SetLocalAngle(player, player->mo->angle + CLIMBCONEMAX);
		else if (angdiff > ANGLE_180 && angdiff < InvAngle(CLIMBCONEMAX))
			P_SetLocalAngle(player, player->mo->angle - CLIMBCONEMAX);
	}

	if (player->climbing == 0)
		P_SetMobjState(player->mo, S_PLAY_JUMP);

	if (player->climbing && P_IsObjectOnGround(player->mo))
	{
		P_ResetPlayer(player);
		P_SetMobjState(player->mo, S_PLAY_STND);
	}
}






static mobj_t *teeterer;  
static boolean solidteeter;  
static fixed_t highesttop;  

static boolean couldteeter;
static fixed_t teeterxl, teeterxh;
static fixed_t teeteryl, teeteryh;

static boolean PIT_CheckSolidsTeeter(mobj_t *thing)
{
	fixed_t blockdist;
	fixed_t tiptop = FixedMul(MAXSTEPMOVE, teeterer->scale);
	fixed_t thingtop = thing->z + thing->height;
	fixed_t teeterertop = teeterer->z + teeterer->height;

	if (!teeterer || !thing)
		return true;

	if (!(thing->flags & MF_SOLID))
		return true;

	if (thing->flags & MF_NOCLIP)
		return true;

	if (thing == teeterer)
		return true;

	if (thing->player && cv_tailspickup.value && !(gametyperules & GTR_HIDEFROZEN))
		return true;

	blockdist = teeterer->radius + thing->radius;

	if (abs(thing->x - teeterer->x) >= blockdist || abs(thing->y - teeterer->y) >= blockdist)
		return true;  

	highesttop = INT32_MIN;

	if (teeterer->eflags & MFE_VERTICALFLIP)
	{
		if (thingtop < teeterer->z)
			return true;
		if (thing->z > highesttop)
			return true;
		highesttop = thing->z;
		if (thing->z > teeterertop + tiptop)
		{
			solidteeter = true;
			return true;
		}
	}
	else
	{
		if (thing->z > teeterertop)
			return true;
		if (thingtop < highesttop)
			return true;
		highesttop = thingtop;
		if (thingtop < teeterer->z - tiptop)
		{
			solidteeter = true;
			return true;
		}
	}


	if ((teeterer->eflags & MFE_VERTICALFLIP && thing->z - FixedMul(FRACUNIT, teeterer->scale) == teeterertop)
	|| (!(teeterer->eflags & MFE_VERTICALFLIP) && thingtop + FixedMul(FRACUNIT, teeterer->scale) == teeterer->z))
	{
		fixed_t teeterdist = thing->radius - FixedMul(5*FRACUNIT, teeterer->scale);

		if (abs(teeterer->x - thing->x) > teeterdist || abs(teeterer->y - thing->y) > teeterdist)
		{
			if (couldteeter)  
			{
				if (thing->x - teeterdist < teeterxl)
					teeterxl = thing->x - teeterdist;
				if (thing->x + teeterdist > teeterxh)
					teeterxh = thing->x + teeterdist;
				if (thing->y - teeterdist < teeteryl)
					teeteryl = thing->y - teeterdist;
				if (thing->y + teeterdist > teeteryh)
					teeteryh = thing->y + teeterdist;

				if (teeterer->x < teeterxl)
					return true;
				if (teeterer->x > teeterxh)
					return true;
				if (teeterer->y < teeteryl)
					return true;
				if (teeterer->y > teeteryh)
					return true;

				solidteeter = false;  
				couldteeter = false;  
				return false;
			}
			else
			{


				teeterxl = thing->x - teeterdist;
				teeterxh = thing->x + teeterdist;
				teeteryl = thing->y - teeterdist;
				teeteryh = thing->y + teeterdist;
			}
			couldteeter = true;
			return true;
		}
		solidteeter = false;
		couldteeter = false;
		return false;  
	}
	solidteeter = false;
	return true;  
}






static void P_DoTeeter(player_t *player)
{
	boolean teeter = false;
	boolean roverfloor;  
	fixed_t floorheight, ceilingheight;
	fixed_t topheight, bottomheight;  
	const fixed_t tiptop = FixedMul(MAXSTEPMOVE, player->mo->scale);  

	if (player->mo->standingslope && player->mo->standingslope->zdelta >= (FRACUNIT/2))  
		teeter = true;
	else  
	{
		UINT8 i;
		sector_t *sec;
		fixed_t highestceilingheight = INT32_MIN;
		fixed_t lowestfloorheight = INT32_MAX;

		teeter = false;
		roverfloor = false;
		for (i = 0; i < 4; i++)
		{
			ffloor_t *rover;

#define xsign ((i & 1) ? -1 : 1)  
#define ysign ((i & 2) ? 1 : -1)  
			fixed_t checkx = player->mo->x + (xsign*FixedMul(5*FRACUNIT, player->mo->scale));
			fixed_t checky = player->mo->y + (ysign*FixedMul(5*FRACUNIT, player->mo->scale));
#undef xsign
#undef ysign

			sec = R_PointInSubsector(checkx, checky)->sector;

			ceilingheight = P_GetSectorCeilingZAt(sec, checkx, checky);
			floorheight   = P_GetSectorFloorZAt  (sec, checkx, checky);
			highestceilingheight = (ceilingheight > highestceilingheight) ? ceilingheight : highestceilingheight;
			lowestfloorheight = (floorheight < lowestfloorheight) ? floorheight : lowestfloorheight;

			if (!(sec->ffloors))
				continue;  

			for (rover = sec->ffloors; rover; rover = rover->next)
			{
				if (!(rover->fofflags & FOF_EXISTS)) continue;

				topheight    = P_GetFFloorTopZAt   (rover, player->mo->x, player->mo->y);
				bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);

				if (P_CheckSolidLava(rover))
					;
				else if (!(rover->fofflags & FOF_BLOCKPLAYER || rover->fofflags & FOF_QUICKSAND))
					continue;  

				if (player->mo->eflags & MFE_VERTICALFLIP)
				{
					if (bottomheight > ceilingheight)  
						continue;

					if (bottomheight > player->mo->z + player->mo->height + tiptop
						|| (topheight < player->mo->z
						&& player->mo->z + player->mo->height < ceilingheight - tiptop))
					{
						teeter = true;
						roverfloor = true;
					}
					else
					{
						teeter = false;
						roverfloor = true;
						break;
					}
				}
				else
				{
					if (topheight < floorheight)  
						continue;

					if (topheight < player->mo->z - tiptop
						|| (bottomheight > player->mo->z + player->mo->height
						&& player->mo->z > floorheight + tiptop))
					{
						teeter = true;
						roverfloor = true;
					}
					else
					{
						teeter = false;
						roverfloor = true;
						break;
					}
				}
			}
			break;  
		}

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			if (!teeter && !roverfloor && (highestceilingheight > player->mo->ceilingz + tiptop))
					teeter = true;
		}
		else
		{
			if (!teeter && !roverfloor && (lowestfloorheight < player->mo->floorz - tiptop))
					teeter = true;
		}
	}

	{
		INT32 bx, by, xl, xh, yl, yh;

		yh = (unsigned)(player->mo->y + player->mo->radius - bmaporgy)>>MAPBLOCKSHIFT;
		yl = (unsigned)(player->mo->y - player->mo->radius - bmaporgy)>>MAPBLOCKSHIFT;
		xh = (unsigned)(player->mo->x + player->mo->radius - bmaporgx)>>MAPBLOCKSHIFT;
		xl = (unsigned)(player->mo->x - player->mo->radius - bmaporgx)>>MAPBLOCKSHIFT;

		BMBOUNDFIX(xl, xh, yl, yh);


		validcount++;

		for (by = yl; by <= yh; by++)
			for (bx = xl; bx <= xh; bx++)
			{
				INT32 offset;
				polymaplink_t *plink;  

				if (bx < 0 || by < 0 || bx >= bmapwidth || by >= bmapheight)
					continue;

				offset = by*bmapwidth + bx;


				plink = polyblocklinks[offset];

				while (plink)
				{
					polyobj_t *po = plink->po;

					if (po->validcount != validcount)  
					{
						sector_t *polysec;
						fixed_t polytop, polybottom;

						po->validcount = validcount;

						if (!(po->flags & POF_SOLID))
						{
							plink = (polymaplink_t *)(plink->link.next);
							continue;
						}

						if (!P_MobjInsidePolyobj(po, player->mo))
						{
							plink = (polymaplink_t *)(plink->link.next);
							continue;
						}


						polysec = po->lines[0]->backsector;

						if (po->flags & POF_CLIPPLANES)
						{
							polytop = polysec->ceilingheight;
							polybottom = polysec->floorheight;
						}
						else
						{
							polytop = INT32_MAX;
							polybottom = INT32_MIN;
						}

						if (player->mo->eflags & MFE_VERTICALFLIP)
						{
							if (polybottom > player->mo->ceilingz)  
							{
								plink = (polymaplink_t *)(plink->link.next);
								continue;
							}

							if (polybottom > player->mo->z + player->mo->height + tiptop
							|| (polytop < player->mo->z
							&& player->mo->z + player->mo->height < player->mo->ceilingz - tiptop))
								teeter = true;
							else
							{
								teeter = false;
								break;
							}
						}
						else
						{
							if (polytop < player->mo->floorz)  
							{
								plink = (polymaplink_t *)(plink->link.next);
								continue;
							}

							if (polytop < player->mo->z - tiptop
							|| (polybottom > player->mo->z + player->mo->height
							&& player->mo->z > player->mo->floorz + tiptop))
								teeter = true;
							else
							{
								teeter = false;
								break;
							}
						}
					}
					plink = (polymaplink_t *)(plink->link.next);
				}
			}
		if (teeter)  
		{
			mobj_t *oldtmthing = tmthing;
			teeterer = player->mo;
			P_SetTarget(&tmthing, teeterer);
			teeterxl = teeterxh = player->mo->x;
			teeteryl = teeteryh = player->mo->y;
			couldteeter = false;
			solidteeter = teeter;
			if (!P_DoBlockThingsIterate(xl, yl, xh, yh, PIT_CheckSolidsTeeter))
				goto teeterdone;  
teeterdone:
			teeter = solidteeter;
			P_SetTarget(&tmthing, oldtmthing);  
		}
	}
	if (teeter)
	{
		if (player->panim == PA_IDLE)
			P_SetMobjState(player->mo, S_PLAY_EDGE);
	}
	else if (player->panim == PA_EDGE)
		P_SetMobjState(player->mo, S_PLAY_STND);
}






static void P_SetWeaponDelay(player_t *player, INT32 delay)
{
	player->weapondelay = delay;

	if (player->skin == 2)  
	{


		player->weapondelay *= 2;
		player->weapondelay /= 3;
	}
}







static void P_DrainWeaponAmmo (player_t *player, INT32 power)
{
	player->powers[power]--;

	if (player->rings < 1)
	{
		player->ammoremovalweapon = player->currentweapon;
		player->ammoremovaltimer  = ammoremovaltics;

		if (player->powers[power] > 0)  
		{
			player->powers[power]--;
			player->ammoremoval = 2;
		}
		else
			player->ammoremoval = 1;
	}
	else
		player->rings--;
}






static void P_DoFiring(player_t *player, ticcmd_t *cmd)
{
	INT32 i;
	mobj_t *mo = NULL;

	I_Assert(player != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (!(cmd->buttons & (BT_ATTACK|BT_FIRENORMAL)))
	{


		player->pflags &= ~PF_ATTACKDOWN;
		return;
	}

	if (player->pflags & PF_ATTACKDOWN || player->climbing || (G_TagGametype() && !(player->pflags & PF_TAGIT)))
		return;

	if (((player->powers[pw_shield] & SH_STACK) == SH_FIREFLOWER) && !(player->weapondelay))
	{
		player->pflags |= PF_ATTACKDOWN;
		mo = P_SpawnPlayerMissile(player->mo, MT_FIREBALL, 0);
		if (mo)
			P_InstaThrust(mo, player->mo->angle, ((mo->info->speed>>FRACBITS)*player->mo->scale) + player->speed);
		S_StartSound(player->mo, sfx_mario7);
		P_SetWeaponDelay(player, TICRATE);  
		return;
	}

	if (!G_RingSlingerGametype() || player->weapondelay)
		return;

	player->pflags |= PF_ATTACKDOWN;

	if (cmd->buttons & BT_FIRENORMAL)  
		goto firenormal;  

	else if (player->currentweapon == WEP_BOUNCE && player->powers[pw_bouncering])
	{
		P_DrainWeaponAmmo(player, pw_bouncering);
		P_SetWeaponDelay(player, TICRATE/4);

		mo = P_SpawnPlayerMissile(player->mo, MT_THROWNBOUNCE, MF2_BOUNCERING);

		if (mo)
			mo->fuse = 3*TICRATE;  
	}

	else if (player->currentweapon == WEP_RAIL && player->powers[pw_railring])
	{
		P_DrainWeaponAmmo(player, pw_railring);
		P_SetWeaponDelay(player, (3*TICRATE)/2);

		mo = P_SpawnPlayerMissile(player->mo, MT_REDRING, MF2_RAILRING|MF2_DONTDRAW);


		S_StartSound(player->mo, sfx_rail1);
	}

	else if (player->currentweapon == WEP_AUTO && player->powers[pw_automaticring])
	{
		P_DrainWeaponAmmo(player, pw_automaticring);
		player->pflags &= ~PF_ATTACKDOWN;
		P_SetWeaponDelay(player, 2);

		mo = P_SpawnPlayerMissile(player->mo, MT_THROWNAUTOMATIC, MF2_AUTOMATIC);
	}

	else if (player->currentweapon == WEP_EXPLODE && player->powers[pw_explosionring])
	{
		P_DrainWeaponAmmo(player, pw_explosionring);
		P_SetWeaponDelay(player, (3*TICRATE)/2);

		mo = P_SpawnPlayerMissile(player->mo, MT_THROWNEXPLOSION, MF2_EXPLOSION);
	}

	else if (player->currentweapon == WEP_GRENADE && player->powers[pw_grenadering])
	{
		P_DrainWeaponAmmo(player, pw_grenadering);
		P_SetWeaponDelay(player, TICRATE/3);

		mo = P_SpawnPlayerMissile(player->mo, MT_THROWNGRENADE, MF2_EXPLOSION);

		if (mo)
		{

			mo->fuse = mo->info->reactiontime;
		}
	}


	else if (player->currentweapon == WEP_SCATTER && player->powers[pw_scatterring])
	{
		fixed_t oldz = player->mo->z;
		angle_t shotangle = player->mo->angle;
		angle_t oldaiming = player->aiming;

		P_DrainWeaponAmmo(player, pw_scatterring);
		P_SetWeaponDelay(player, (2*TICRATE)/3);


		mo = P_SpawnPlayerMissile(player->mo, MT_THROWNSCATTER, MF2_SCATTER);
		if (mo)
			shotangle = R_PointToAngle2(player->mo->x, player->mo->y, mo->x, mo->y);


		mo = P_SPMAngle(player->mo, MT_THROWNSCATTER, shotangle-ANG2, true, MF2_SCATTER);


		mo = P_SPMAngle(player->mo, MT_THROWNSCATTER, shotangle+ANG2, true, MF2_SCATTER);


		player->mo->z += FixedMul(12*FRACUNIT, player->mo->scale);
		player->aiming += ANG1;
		mo = P_SPMAngle(player->mo, MT_THROWNSCATTER, shotangle, true, MF2_SCATTER);


		player->mo->z -= FixedMul(24*FRACUNIT, player->mo->scale);
		player->aiming -= ANG2;
		mo = P_SPMAngle(player->mo, MT_THROWNSCATTER, shotangle, true, MF2_SCATTER);

		player->mo->z = oldz;
		player->aiming = oldaiming;
		return;
	}

	else
	{
firenormal:





		if (player->currentweapon == 0
		&& player->powers[pw_infinityring])
		{
			P_SetWeaponDelay(player, TICRATE/4);

			mo = P_SpawnPlayerMissile(player->mo, MT_THROWNINFINITY, 0);

			player->powers[pw_infinityring]--;
		}

		else
		{
			if (player->rings <= 0)
				return;
			P_SetWeaponDelay(player, TICRATE/4);

			mo = P_SpawnPlayerMissile(player->mo, MT_REDRING, 0);

			if (mo)
				P_ColorTeamMissile(mo, player);

			player->rings--;
		}
	}

	if (mo)
	{
		if (mo->flags & MF_MISSILE && mo->flags2 & MF2_RAILRING)
		{
			const boolean nblockmap = !(mo->flags & MF_NOBLOCKMAP);
			for (i = 0; i < 256; i++)
			{
				if (nblockmap)
				{
					P_UnsetThingPosition(mo);
					mo->flags |= MF_NOBLOCKMAP;
					P_SetThingPosition(mo);
				}

				if (i&1)
					P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

				if (P_RailThinker(mo))
					break;  
			}


			S_StartSound(mo, sfx_rail2);
		}
	}
}






static void P_DoSuperStuff(player_t *player)
{
	mobj_t *spark;
	ticcmd_t *cmd = &player->cmd;
	if (player->mo->state >= &states[S_PLAY_SUPER_TRANS1]
	&& player->mo->state < &states[S_PLAY_SUPER_TRANS6])
		return;  

	if (player->powers[pw_carry] == CR_NIGHTSMODE)
		return;  

	if (player->powers[pw_super])
	{

		if (!(ALL7EMERALDS(emeralds) && player->charflags & SF_SUPER))
		{
			player->powers[pw_super] = 0;
			P_SetMobjState(player->mo, S_PLAY_STND);
			if (P_IsLocalPlayer(player))
			{
				music_stack_noposition = true;  
				music_stack_fadeout = MUSICRATE/2;  
			}
			P_RestoreMusic(player);
			P_SpawnShieldOrb(player);


			if ((player->powers[pw_shield] & SH_STACK) == SH_FIREFLOWER)
			{
				player->mo->color = SKINCOLOR_WHITE;
				G_GhostAddColor(GHC_FIREFLOWER);
			}
			else
			{
				player->mo->color = P_GetPlayerColor(player);
				G_GhostAddColor(GHC_NORMAL);
			}

			if (!G_CoopGametype())
			{
				HU_SetCEchoFlags(0);
				HU_SetCEchoDuration(5);
				HU_DoCEcho(va("%s\\is no longer super.\\\\\\\\", player_names[player-players]));
			}
			return;
		}

		player->mo->color = (player->pflags & PF_GODMODE && cv_debug == 0)
		? (SKINCOLOR_SUPERSILVER1 + 5*(((signed)leveltime >> 1) % 7))  
		: skins[player->skin]->supercolor + abs( ( (player->powers[pw_super] >> 1) % 9) - 4);  

		G_GhostAddColor(GHC_SUPER);

		if (player->mo->state == &states[S_PLAY_SUPER_TRANS6])  
			return;


		if ((leveltime % TICRATE == 0) && !(player->exiting))
			player->rings--;

		if ((cmd->forwardmove != 0 || cmd->sidemove != 0 || player->powers[pw_carry])
		&& !(leveltime % TICRATE) && (player->mo->momx || player->mo->momy))
		{
			spark = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_SUPERSPARK);
			if (!P_MobjWasRemoved(spark))
				P_SetScale(spark, player->mo->scale, true);
		}


		if (player->rings <= 0 || player->exiting)
		{
			P_DoSuperDetransformation(player);
		}
	}
}






boolean P_SuperReady(player_t *player)
{
	if (!player->powers[pw_super]
	&& !player->powers[pw_invulnerability]
	&& !player->powers[pw_tailsfly]
	&& (player->charflags & SF_SUPER)
	&& (player->pflags & PF_JUMPED)
	&& !(player->powers[pw_shield] & SH_NOSTACK)
	&& !(maptol & TOL_NIGHTS)
	&& ALL7EMERALDS(emeralds)
	&& (player->rings >= 50))
		return true;

	return false;
}






void P_DoJump(player_t *player, boolean soundandstate, boolean allowflip)
{
	fixed_t factor;
	const fixed_t dist6 = FixedMul(FixedDiv(player->speed, player->mo->scale), player->actionspd)/20;

	if (player->pflags & PF_JUMPSTASIS)
		return;

	if (!player->jumpfactor)
		return;

	if (player->climbing)
	{

		if (player->powers[pw_super])
			player->mo->momz = 5*FRACUNIT;
		else if (player->mo->eflags & MFE_UNDERWATER)
			player->mo->momz = 2*FRACUNIT;
		else
			player->mo->momz = 15*(FRACUNIT/4);

		player->drawangle = player->mo->angle = player->mo->angle - ANGLE_180;  

		if (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG)
			P_SetPlayerAngle(player, player->mo->angle);

		player->climbing = 0;  
		P_InstaThrust(player->mo, player->mo->angle, FixedMul(6*FRACUNIT, player->mo->scale));  
	}

	else if (P_InQuicksand(player->mo))
	{
		if (player->mo->ceilingz-player->mo->floorz <= player->mo->height-1)
			return;
		player->mo->momz += (39*(FRACUNIT/4))>>1;
		if (player->mo->momz >= 6*FRACUNIT)
			player->mo->momz = 6*FRACUNIT;  
		else if (player->mo->momz < 0)  
			player->mo->momz = (39*(FRACUNIT/4))>>1;  
	}
	else if (!(player->pflags & PF_JUMPED))  
	{
		if (player->mo->ceilingz-player->mo->floorz <= player->mo->height-1)
			return;

		if (player->powers[pw_carry] == CR_PTERABYTE)
		{
			S_StartSound(player->mo, sfx_s3kd7s);
			player->mo->tracer->cusval += 10;  
			player->mo->tracer->watertop = P_RandomRange(-player->mo->tracer->cusval, player->mo->tracer->cusval) << (FRACBITS - 1);
			player->mo->tracer->waterbottom = P_RandomRange(-player->mo->tracer->cusval, player->mo->tracer->cusval) << (FRACBITS - 1);
			player->mo->tracer->cvmem = P_RandomRange(-player->mo->tracer->cusval, player->mo->tracer->cusval) << (FRACBITS - 1);
			return;
		}


		if (player->powers[pw_carry] == CR_PLAYER)
		{
			player->mo->momz = 9*FRACUNIT;
			player->powers[pw_carry] = CR_NONE;
			P_SetTarget(&player->mo->tracer, NULL);
			if (player-players == consoleplayer && botingame)
				CV_SetValue(&cv_analog[1], true);
		}
		else if (player->powers[pw_carry] == CR_GENERIC)
		{
			player->mo->momz = 9*FRACUNIT;
			player->powers[pw_carry] = CR_NONE;
			if (!(player->mo->tracer->flags & MF_MISSILE))  
				P_SetTarget(&player->mo->tracer->target, NULL);
			P_SetTarget(&player->mo->tracer, NULL);
		}
		else if (player->powers[pw_carry] == CR_ROPEHANG)
		{
			player->mo->momz = 12*FRACUNIT;
			player->powers[pw_carry] = CR_NONE;
			P_SetTarget(&player->mo->tracer, NULL);
		}
		else if (player->powers[pw_carry] == CR_ROLLOUT)
		{
			player->mo->momz = 9*FRACUNIT;
			if (player->mo->tracer)
			{
				if (P_MobjFlip(player->mo->tracer)*player->mo->tracer->momz > 0)
					player->mo->momz += player->mo->tracer->momz;
				if (!P_IsObjectOnGround(player->mo->tracer))
					P_SetObjectMomZ(player->mo->tracer, -9*FRACUNIT, true);


				if (player->mo->tracer->eflags & MFE_VERTICALFLIP)
					player->mo->tracer->momz -= 1;
				else
					player->mo->tracer->momz += 1;

				player->mo->tracer->flags |= MF_PUSHABLE;
				P_SetTarget(&player->mo->tracer->tracer, NULL);
			}
			player->powers[pw_carry] = CR_NONE;
			P_SetTarget(&player->mo->tracer, NULL);
		}
		else if (player->mo->eflags & MFE_GOOWATER)
		{
			player->mo->momz = 7*FRACUNIT;
			if (player->charability == CA_JUMPBOOST && onground)
			{
				if (player->charflags & SF_MULTIABILITY)
					player->mo->momz += FixedMul(FRACUNIT/4, dist6);
				else
					player->mo->momz += FixedMul(FRACUNIT/8, dist6);
			}
		}
		else if (maptol & TOL_NIGHTS)
			player->mo->momz = 18*FRACUNIT;
		else if (player->powers[pw_super] && !(player->charflags & SF_NOSUPERJUMPBOOST))
		{
			player->mo->momz = 13*FRACUNIT;


			if (player->charability == CA_JUMPBOOST)
			{
				if (player->charflags & SF_MULTIABILITY)
					player->mo->momz += FixedMul(FRACUNIT/4, dist6);
				else
					player->mo->momz += FixedMul(FRACUNIT/8, dist6);
			}
		}
		else
		{
			player->mo->momz = 39*(FRACUNIT/4);  
			if (player->charability == CA_JUMPBOOST && onground)
			{
				if (player->charflags & SF_MULTIABILITY)
					player->mo->momz += FixedMul(FRACUNIT/4, dist6);
				else
					player->mo->momz += FixedMul(FRACUNIT/8, dist6);
			}
		}


		if (player->mo->eflags & MFE_UNDERWATER)
			player->mo->momz = FixedMul(player->mo->momz, FixedDiv(117*FRACUNIT, 200*FRACUNIT));

		player->pflags |= PF_STARTJUMP;
	}

	factor = player->jumpfactor;

	if (twodlevel || (player->mo->flags2 & MF2_TWOD))
		factor += player->jumpfactor / 10;

	if (player->charflags & SF_MULTIABILITY && player->charability == CA_DOUBLEJUMP && (player->actionspd >> FRACBITS) != -1)
		factor -= max(0, player->secondjump * player->jumpfactor / ((player->actionspd >> FRACBITS) + 1));  




	P_SetObjectMomZ(player->mo, FixedMul(factor, player->mo->momz), false);  


	if (player->mo->eflags & MFE_VERTICALFLIP)
	{
		player->mo->z--;
		if (player->mo->pmomz < 0)
			player->mo->momz += player->mo->pmomz;  
		player->mo->pmomz = 0;
	}
	else
	{
		player->mo->z++;
		if (player->mo->pmomz > 0)
			player->mo->momz += player->mo->pmomz;  
		player->mo->pmomz = 0;
	}
	player->mo->eflags &= ~MFE_APPLYPMOMZ;

	player->pflags |= P_GetJumpFlags(player);;

	if (player->charflags & SF_NOJUMPDAMAGE)
		player->pflags &= ~PF_SPINNING;

	if (allowflip && P_InJumpFlipSector(player->mo))  
	{
		player->mo->flags2 ^= MF2_OBJECTFLIP;
		S_StartSound(player->mo, sfx_s3k73);  
	}

	if (soundandstate)
	{
		if (!player->spectator)
			S_StartSound(player->mo, sfx_jump);  

		P_SetMobjState(player->mo, S_PLAY_JUMP);
	}
}

void P_DoSpinDashDust(player_t *player)
{
	UINT32 i;
	mobj_t *particle;
	INT32 prandom[3];
	for (i = 0; i <= (leveltime%7)/2; i++) {  
		particle = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_SPINDUST);
		if (P_MobjWasRemoved(particle))
			return;

		if (player->mo->eflags & (MFE_TOUCHWATER|MFE_UNDERWATER))  
			P_SetMobjState(particle, S_SPINDUST_BUBBLE1);
		else if (player->powers[pw_shield] == SH_ELEMENTAL)
			P_SetMobjState(particle, S_SPINDUST_FIRE1);

		P_SetTarget(&particle->target, player->mo);
		P_SetScale(particle, (2*player->mo->scale)/3, true);
		if (player->mo->eflags & MFE_VERTICALFLIP)  
			particle->z = player->mo->z + player->mo->height - particle->height;
		prandom[0] = P_RandomFixed()<<2;  
		prandom[1] = P_RandomRange(-30, 30);  
		prandom[2] = P_RandomFixed()<<3;  
		P_SetObjectMomZ(particle, player->dashspeed/50 + prandom[0], false);
		P_InstaThrust(particle,
				player->drawangle + (prandom[1]*ANG1),
				-FixedMul(player->dashspeed/12 + FRACUNIT + prandom[2], player->mo->scale));
		P_TryMove(particle, particle->x+particle->momx, particle->y+particle->momy, true);
	}
}






static void P_DoSpinAbility(player_t *player, ticcmd_t *cmd)
{
	boolean canstand = true;  
	if (player->pflags & PF_STASIS
		&& (player->pflags & PF_JUMPSTASIS || player->mo->state-states != S_PLAY_GLIDE_LANDING))
		return;

	if (cmd->buttons & BT_SPIN)
	{
		if (LUA_HookPlayer(player, HOOK(SpinSpecial)))
			return;
	}

	canstand = (!player->mo->standingslope || (player->mo->standingslope->flags & SL_NOPHYSICS) || abs(player->mo->standingslope->zdelta) < FRACUNIT/2);




	if (!(player->pflags & PF_SLIDING) && !player->exiting && !P_PlayerInPain(player))
	{
		switch (player->charability2)
		{
			case CA2_SPINDASH:  

				if ((cmd->buttons & BT_SPIN) && (player->speed < FixedMul(5<<FRACBITS, player->mo->scale) || player->mo->state - states == S_PLAY_GLIDE_LANDING)
					&& !player->mo->momz && onground && !(player->pflags & (PF_SPINDOWN|PF_SPINNING))
						&& canstand)
				{
					player->mo->momx = player->cmomx;
					player->mo->momy = player->cmomy;
					player->pflags |= (PF_SPINDOWN|PF_STARTDASH|PF_SPINNING);
					player->dashspeed = player->mindash;
					P_SetMobjState(player->mo, S_PLAY_SPINDASH);
					if (!player->spectator)
						S_StartSound(player->mo, sfx_spndsh);  
				}

				else if ((cmd->buttons & BT_SPIN) && (player->pflags & PF_STARTDASH))
				{
					if (player->speed >= 5*player->mo->scale)
					{
						player->pflags &= ~PF_STARTDASH;
						P_SetMobjState(player->mo, S_PLAY_ROLL);
						S_StartSound(player->mo, sfx_spin);
						break;
					}
					if (player->dashspeed < player->mindash)
						player->dashspeed = player->mindash;

					if (player->dashspeed > player->maxdash)
						player->dashspeed = player->maxdash;

					if (player->dashspeed < player->maxdash && player->mindash != player->maxdash)
					{
#define chargecalculation (6*(player->dashspeed - player->mindash))/(player->maxdash - player->mindash)
						fixed_t soundcalculation = chargecalculation;
						player->dashspeed += FRACUNIT;
						if (!player->spectator && soundcalculation != chargecalculation)
							S_StartSound(player->mo, sfx_spndsh);  
#undef chargecalculation
					}
					if (player->revitem && !(leveltime % 5))  
					{
						P_SpawnSpinMobj(player, player->revitem);
						G_GhostAddRev();
					}
				}



				else if ((cmd->buttons & BT_SPIN || ((twodlevel || (player->mo->flags2 & MF2_TWOD)) && cmd->forwardmove < -20))
					&& !player->climbing && !player->mo->momz && onground && (player->speed > FixedMul(5<<FRACBITS, player->mo->scale)
						|| !canstand) && !(player->pflags & (PF_SPINDOWN|PF_SPINNING)))
				{
					player->pflags |= (PF_SPINDOWN|PF_SPINNING);
					P_SetMobjState(player->mo, S_PLAY_ROLL);
					if (!player->spectator)
						S_StartSound(player->mo, sfx_spin);
				}

				else if (onground && !(player->pflags & PF_SPINDOWN) && (player->pflags & PF_STARTDASH) && (player->pflags & PF_SPINNING))
				{
					player->pflags &= ~PF_STARTDASH;
					if (player->powers[pw_carry] == CR_BRAKGOOP)
						player->dashspeed = 0;

					if (!((gametyperules & GTR_RACE) && leveltime < 4*TICRATE))
					{
						if (player->dashspeed)
						{
							P_SetMobjState(player->mo, S_PLAY_ROLL);
							P_InstaThrust(player->mo, player->mo->angle, (player->speed = FixedMul(player->dashspeed, player->mo->scale)));  
						}
						else
						{
							P_SetMobjState(player->mo, S_PLAY_STND);
							player->pflags &= ~PF_SPINNING;
						}

						if (!player->spectator)
							S_StartSound(player->mo, sfx_zoom);
					}

					player->dashspeed = 0;
				}
				break;
			case CA2_GUNSLINGER:
				if (!player->mo->momz && onground && !player->weapondelay && canstand)
				{
					if (player->speed > FixedMul(10<<FRACBITS, player->mo->scale))
					{}
					else
					{
						mobj_t *lockon = P_LookForEnemies(player, false, true);
						if (lockon)
						{
							if (P_IsLocalPlayer(player))  
							{
								mobj_t *visual = P_SpawnMobj(lockon->x, lockon->y, lockon->z, MT_LOCKON);  
								if (!P_MobjWasRemoved(visual))
									P_SetTarget(&visual->target, lockon);
								visual->drawonlyforplayer = player;  
							}
						}
						if ((cmd->buttons & BT_SPIN) && !(player->pflags & PF_SPINDOWN))
						{
							mobj_t *bullet;

							P_SetMobjState(player->mo, S_PLAY_FIRE);

#define zpos(posmo) (posmo->z + (posmo->height - mobjinfo[player->revitem].height)/2)
							if (lockon)
							{
								player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, lockon->x, lockon->y);
								bullet = P_SpawnPointMissile(player->mo, lockon->x, lockon->y, zpos(lockon), player->revitem, player->mo->x, player->mo->y, zpos(player->mo));
								if (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG)
									P_SetPlayerAngle(player, player->mo->angle);
							}
							else
							{
								bullet = P_SpawnPointMissile(player->mo, player->mo->x + P_ReturnThrustX(NULL, player->mo->angle, FRACUNIT), player->mo->y + P_ReturnThrustY(NULL, player->mo->angle, FRACUNIT), zpos(player->mo), player->revitem, player->mo->x, player->mo->y, zpos(player->mo));
								if (bullet)
								{
									bullet->flags &= ~MF_NOGRAVITY;
									bullet->momx >>= 1;
									bullet->momy >>= 1;
								}
							}
							player->drawangle = player->mo->angle;
#undef zpos

							player->mo->momx >>= 1;
							player->mo->momy >>= 1;
							player->pflags |= PF_SPINDOWN;
							P_SetWeaponDelay(player, TICRATE/2);
						}
					}
				}
				break;
			case CA2_MELEE:  
				if (player->panim != PA_ABILITY2 && (cmd->buttons & BT_SPIN)
				&& !player->mo->momz && onground && !(player->pflags & PF_SPINDOWN)
				&& canstand)
				{
					P_ResetPlayer(player);
					player->pflags |= PF_THOKKED;
#if 0
					if ((player->charability == CA_TWINSPIN) && (player->speed > FixedMul(player->runspeed, player->mo->scale)))
					{
						P_DoJump(player, false, false);
						player->pflags &= ~PF_STARTJUMP;
						player->mo->momz = FixedMul(player->mo->momz, 3*FRACUNIT/2);  
						P_SetMobjState(player->mo, S_PLAY_TWINSPIN);
						S_StartSound(player->mo, sfx_s3k8b);
					}
					else
#endif
					{
						player->mo->z += P_MobjFlip(player->mo);
						P_SetObjectMomZ(player->mo, player->mindash, false);
						if (P_MobjFlip(player->mo)*player->mo->pmomz > 0)
							player->mo->momz += player->mo->pmomz;  
						else
							player->mo->pmomz = 0;
						if (player->mo->eflags & MFE_UNDERWATER)
							player->mo->momz >>= 1;
#if 0
						if (FixedMul(player->speed, FINECOSINE(((player->mo->angle - R_PointToAngle2(0, 0, player->rmomx, player->rmomy)) >> ANGLETOFINESHIFT) & FINEMASK)) < FixedMul(player->maxdash, player->mo->scale))
#else
						if (player->speed < FixedMul(player->maxdash, player->mo->scale))
#endif
						{
							if (player->panim == PA_IDLE)
								player->drawangle = player->mo->angle;
							P_InstaThrust(player->mo, player->drawangle, FixedMul(player->maxdash, player->mo->scale));
						}
						player->mo->momx += player->cmomx;
						player->mo->momy += player->cmomy;
						P_SetMobjState(player->mo, S_PLAY_MELEE);
						player->powers[pw_strong] = STR_MELEE;
						S_StartSound(player->mo, sfx_s3k42);
					}
					player->pflags |= PF_SPINDOWN;
				}
				break;
		}
	}






	if (onground && player->pflags & PF_SPINNING && !(player->pflags & PF_STARTDASH)
		&& player->speed < 5*player->mo->scale && canstand)
	{
		if ((player->mo->subsector->sector->specialflags & SSF_FORCESPIN) || (player->mo->ceilingz - player->mo->floorz < P_GetPlayerHeight(player)))
			P_InstaThrust(player->mo, player->mo->angle, 10*player->mo->scale);
		else
		{
			player->skidtime = 0;
			player->pflags &= ~PF_SPINNING;
			P_SetMobjState(player->mo, S_PLAY_STND);
			player->mo->momx = player->cmomx;
			player->mo->momy = player->cmomy;
		}
	}

	if (onground && player->pflags & PF_STARTDASH)
	{

		if (!(player->charflags & SF_NOSPINDASHDUST) && !(player->mo->eflags & MFE_GOOWATER))
			P_DoSpinDashDust(player);
	}
}






void P_DoJumpShield(player_t *player)
{
	boolean electric = ((player->powers[pw_shield] & SH_PROTECTELECTRIC) == SH_PROTECTELECTRIC);

	if (player->pflags & PF_THOKKED)
		return;

	player->pflags &= ~PF_JUMPED;
	P_DoJump(player, false, true);
	player->secondjump = 0;
	player->pflags |= PF_THOKKED|PF_SHIELDABILITY;
	player->pflags &= ~(PF_STARTJUMP|PF_SPINNING|PF_BOUNCING);
	if (electric)
	{
		mobj_t *spark;
		INT32 i;
#define numangles 6
#define limitangle (360/numangles)
		const angle_t travelangle = player->mo->angle + P_RandomRange(-limitangle, limitangle)*ANG1;
		for (i = 0; i < numangles; i++)
		{
			spark = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_THUNDERCOIN_SPARK);
			if (!P_MobjWasRemoved(spark))
			{
				P_InstaThrust(spark, travelangle + i*(ANGLE_MAX/numangles), FixedMul(4*FRACUNIT, spark->scale));
				if (i % 2)
					P_SetObjectMomZ(spark, -4*FRACUNIT, false);
				spark->fuse = 18;
			}
		}
#undef limitangle
#undef numangles
		player->pflags &= ~PF_NOJUMPDAMAGE;
		P_SetMobjState(player->mo, S_PLAY_ROLL);
		S_StartSound(player->mo, sfx_s3k45);
	}
	else
	{
		player->pflags |= PF_NOJUMPDAMAGE;
		P_SetMobjState(player->mo, S_PLAY_FALL);
		S_StartSound(player->mo, sfx_wdjump);
	}
}






void P_DoBubbleBounce(player_t *player)
{
	player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE|PF_SHIELDABILITY);
	S_StartSound(player->mo, sfx_s3k44);
	P_MobjCheckWater(player->mo);
	P_DoJump(player, false, false);
	if (player->charflags & SF_NOJUMPSPIN)
		P_SetMobjState(player->mo, S_PLAY_FALL);
	else
		P_SetMobjState(player->mo, S_PLAY_ROLL);
	player->pflags |= PF_THOKKED;
	player->pflags &= ~PF_STARTJUMP;
	player->secondjump = UINT8_MAX;
	player->mo->momz = FixedMul(player->mo->momz, 11*FRACUNIT/8);
}






void P_DoAbilityBounce(player_t *player, boolean changemomz)
{
	if (player->mo->state-states == S_PLAY_BOUNCE_LANDING)
		return;

	if (changemomz)
	{
		fixed_t prevmomz = player->mo->momz, minmomz;

		if (P_MobjFlip(player->mo)*prevmomz < 0)
			prevmomz = 0;
		else if (player->mo->eflags & MFE_UNDERWATER)
			prevmomz /= 2;

		P_DoJump(player, false, false);
		player->pflags &= ~(PF_STARTJUMP|PF_JUMPED);
		minmomz = FixedMul(player->mo->momz, 3*FRACUNIT/2);

		if (player->mo->eflags & MFE_VERTICALFLIP)  
			player->mo->momz = min(minmomz, (minmomz + prevmomz)/2);
		else
			player->mo->momz = max(minmomz, (minmomz + prevmomz)/2);
	}

	S_StartSound(player->mo, sfx_boingf);
	P_SetMobjState(player->mo, S_PLAY_BOUNCE_LANDING);
	player->pflags |= PF_BOUNCING|PF_THOKKED;
}






void P_TwinSpinRejuvenate(player_t *player, mobjtype_t type)
{
	fixed_t actionspd;
	angle_t movang, ang, fa;
	fixed_t v, h;
	UINT8 i;

	if (!player->mo || !type)
		return;

	actionspd = FixedMul(player->actionspd, player->mo->scale);

	fa = (R_PointToAngle2(0, 0, player->mo->momz, FixedHypot(player->mo->momx, player->mo->momy))>>ANGLETOFINESHIFT) & FINEMASK;
	movang = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
	ang = 0;

	v = FixedMul(actionspd, FINESINE(fa));
	h = actionspd - FixedMul(actionspd, FINECOSINE(fa));


	for (i = 0; i <= 7; i++)
	{
		fixed_t side = actionspd - FixedMul(h, abs(FINESINE((ang>>ANGLETOFINESHIFT) & FINEMASK)));
		fixed_t xo = P_ReturnThrustX(NULL, ang + movang, side);
		fixed_t yo = P_ReturnThrustY(NULL, ang + movang, side);
		fixed_t zo = -FixedMul(FINECOSINE(((ang>>ANGLETOFINESHIFT) & FINEMASK)), v);
		mobj_t *missile = P_SpawnMobjFromMobj(player->mo,
			xo,
			yo,
			player->mo->height/2 + zo,
			type);
		if (!P_MobjWasRemoved(missile))
		{
			P_SetTarget(&missile->target, player->mo);
			P_SetScale(missile, missile->destscale/2, true);
			missile->angle = ang + movang;
			missile->fuse = TICRATE/2;
			missile->extravalue2 = (99*FRACUNIT)/100;
			missile->momx = xo;
			missile->momy = yo;
			missile->momz = zo;
		}

		ang += ANGLE_45;
	}

	player->pflags &= ~PF_THOKKED;
}







void P_Telekinesis(player_t *player, fixed_t thrust, fixed_t range)
{
	thinker_t *th;
	mobj_t *mo2;
	fixed_t dist = 0;
	angle_t an;

	if (player->powers[pw_super])  
		range *= 2;

	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2 == player->mo)
			continue;

		if (!((mo2->flags & MF_SHOOTABLE && mo2->flags & MF_ENEMY) || mo2->type == MT_EGGGUARD || mo2->player))
			continue;

		dist = P_AproxDistance(P_AproxDistance(player->mo->x-mo2->x, player->mo->y-mo2->y), player->mo->z-mo2->z);

		if (range < dist)
			continue;

		if (!P_CheckSight(player->mo, mo2))
			continue;  

		an = R_PointToAngle2(player->mo->x, player->mo->y, mo2->x, mo2->y);

		if (mo2->health > 0)
		{
			P_Thrust(mo2, an, thrust);

			if (mo2->type == MT_GOLDBUZZ || mo2->type == MT_REDBUZZ)
				mo2->tics += 8;
		}
	}

	P_SpawnThokMobj(player);
	player->pflags |= PF_THOKKED;
}

static void P_DoTwinSpin(player_t *player)
{
	player->pflags &= ~(PF_NOJUMPDAMAGE|PF_SPINNING);
	player->pflags |= P_GetJumpFlags(player) | PF_THOKKED;
	S_StartSound(player->mo, sfx_s3k42);
	player->mo->frame = 0;
	P_SetMobjState(player->mo, S_PLAY_TWINSPIN);
	player->powers[pw_strong] = STR_TWINSPIN;
}





static boolean P_PlayerShieldThink(player_t *player, ticcmd_t *cmd, mobj_t *lockonthok, mobj_t *visual)
{
	mobj_t *lockonshield = NULL;

	if ((player->powers[pw_shield] & SH_NOSTACK) && !player->powers[pw_super] && !(player->pflags & PF_SPINDOWN)
		&& ((!(player->pflags & PF_THOKKED) || (((player->powers[pw_shield] & SH_NOSTACK) == SH_BUBBLEWRAP || (player->powers[pw_shield] & SH_NOSTACK) == SH_ATTRACT) && player->secondjump == UINT8_MAX) )))  
	{
		if ((player->powers[pw_shield] & SH_NOSTACK) == SH_ATTRACT && !(player->charflags & SF_NOSHIELDABILITY))
		{
			if ((lockonshield = P_LookForEnemies(player, false, false)))
			{
				if (P_IsLocalPlayer(player))  
				{
					boolean dovis = true;
					if (lockonshield == lockonthok)
					{
						if (leveltime & 2)
							dovis = false;
						else if (visual)
							P_RemoveMobj(visual);
					}
					if (dovis)
					{
						visual = P_SpawnMobj(lockonshield->x, lockonshield->y, lockonshield->z, MT_LOCKON);  
						if (!P_MobjWasRemoved(visual))
						{
							P_SetTarget(&visual->target, lockonshield);
						visual->drawonlyforplayer = player;  
							P_SetMobjStateNF(visual, visual->info->spawnstate+1);
						}
					}
				}
			}
		}
		if ((!(player->charflags & SF_NOSHIELDABILITY)) && (cmd->buttons & BT_SPIN && !LUA_HookPlayer(player, HOOK(ShieldSpecial))))  
		{

			if ((player->powers[pw_shield] & ~(SH_FORCEHP|SH_STACK)) == SH_FORCE)
			{
				player->pflags |= PF_THOKKED|PF_SHIELDABILITY;
				player->pflags &= ~PF_SPINNING;
				player->mo->momx = player->mo->momy = player->mo->momz = 0;
				S_StartSound(player->mo, sfx_ngskid);
			}
			else
			{
				switch (player->powers[pw_shield] & SH_NOSTACK)
				{

					case SH_WHIRLWIND:
					case SH_THUNDERCOIN:
						P_DoJumpShield(player);
						break;

					case SH_ARMAGEDDON:
						player->pflags |= PF_THOKKED|PF_SHIELDABILITY;
						player->pflags &= ~PF_SPINNING;
						P_BlackOw(player);
						break;

					case SH_ATTRACT:
						player->pflags |= PF_THOKKED|PF_SHIELDABILITY;
						player->pflags &= ~PF_SPINNING;
						player->homing = 2;
						P_SetTarget(&player->mo->target, P_SetTarget(&player->mo->tracer, lockonshield));
						if (lockonshield)
							{
								player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, lockonshield->x, lockonshield->y);
									player->pflags &= ~PF_NOJUMPDAMAGE;
									P_SetMobjState(player->mo, S_PLAY_ROLL);
									S_StartSound(player->mo, sfx_s3k40);
									player->homing = 3*TICRATE;
							}
							else
								S_StartSound(player->mo, sfx_s3ka6);
							break;

						case SH_ELEMENTAL:
						case SH_BUBBLEWRAP:
							{
								boolean elem = ((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL);
								player->pflags |= PF_THOKKED|PF_SHIELDABILITY;
								player->pflags &= ~PF_SPINNING;
								if (elem)
								{
									player->mo->momx = player->mo->momy = 0;
									S_StartSound(player->mo, sfx_s3k43);
								}
								else
								{
									player->mo->momx -= (player->mo->momx/3);
									player->mo->momy -= (player->mo->momy/3);
									player->pflags &= ~PF_NOJUMPDAMAGE;
									P_SetMobjState(player->mo, S_PLAY_ROLL);
									S_StartSound(player->mo, sfx_s3k44);
								}
								player->secondjump = 0;
								P_SetObjectMomZ(player->mo, -24*FRACUNIT, false);
								break;
							}

						case SH_FLAMEAURA:
							player->pflags |= PF_THOKKED|PF_SHIELDABILITY;
							P_Thrust(player->mo, player->mo->angle, FixedMul(30*FRACUNIT - FixedSqrt(FixedDiv(player->speed, player->mo->scale)), player->mo->scale));
							player->drawangle = player->mo->angle;
							player->pflags &= ~(PF_NOJUMPDAMAGE|PF_SPINNING);
							P_SetMobjState(player->mo, S_PLAY_ROLL);
							S_StartSound(player->mo, sfx_s3k43);
						default:
							break;
				}
			}
		}
		return player->pflags & PF_SHIELDABILITY;
	}
	return false;
}






static void P_DoJumpStuff(player_t *player, ticcmd_t *cmd)
{
	mobj_t *lockonthok = NULL, *visual = NULL;

	if (player->pflags & PF_JUMPSTASIS)
		return;

	if ((player->charability == CA_HOMINGTHOK) && !player->homing && (player->pflags & PF_JUMPED) && (!(player->pflags & PF_THOKKED) || (player->charflags & SF_MULTIABILITY)) && (lockonthok = P_LookForEnemies(player, true, false)))
	{
		if (P_IsLocalPlayer(player))  
		{
			visual = P_SpawnMobj(lockonthok->x, lockonthok->y, lockonthok->z, MT_LOCKON);  
			if (!P_MobjWasRemoved(visual))
				P_SetTarget(&visual->target, lockonthok);
			visual->drawonlyforplayer = player;  
		}
	}






	if ((player->pflags & PF_JUMPED) && !player->exiting && !P_PlayerInPain(player))
	{
		if (onground || player->climbing || player->powers[pw_carry])
			;
		else if ((gametyperules & GTR_TEAMFLAGS) && player->gotflag)
			;
		else if (player->pflags & (PF_GLIDING|PF_SLIDING|PF_SHIELDABILITY))  
			;
		else if (P_PlayerShieldThink(player, cmd, lockonthok, visual))
			;
		else if ((cmd->buttons & BT_SPIN))
		{
			if (!(player->pflags & PF_SPINDOWN) && P_SuperReady(player))
			{


				P_DoSuperTransformation(player, false);
			}
			else if (!LUA_HookPlayer(player, HOOK(JumpSpinSpecial)))
				switch (player->charability)
				{
					case CA_THOK:
						if (player->powers[pw_super])  
						{
							if ((player->speed > 5*player->mo->scale)  
							&& (P_MobjFlip(player->mo)*player->mo->momz <= 0))
							{
								if (player->panim != PA_RUN && player->panim != PA_WALK)
								{
									if (player->speed >= FixedMul(player->runspeed, player->mo->scale))
										P_SetMobjState(player->mo, S_PLAY_FLOAT_RUN);
									else
										P_SetMobjState(player->mo, S_PLAY_FLOAT);
								}

								player->mo->momz = 0;
								player->pflags &= ~(PF_STARTJUMP|PF_SPINNING);
								player->secondjump = 1;
							}
						}
						break;
					case CA_TELEKINESIS:
						if (!(player->pflags & (PF_THOKKED|PF_SPINDOWN)) || (player->charflags & SF_MULTIABILITY))
						{
							P_Telekinesis(player,
								-FixedMul(player->actionspd, player->mo->scale),  
								FixedMul(384*FRACUNIT, player->mo->scale));
						}
						break;
					case CA_TWINSPIN:
						if ((player->charability2 == CA2_MELEE) && (!(player->pflags & (PF_THOKKED|PF_SPINDOWN)) || player->charflags & SF_MULTIABILITY))
							P_DoTwinSpin(player);
						break;
					default:
						break;
				}
		}
	}

	if (player->charability == CA_AIRDRILL)
	{
		if (player->pflags & PF_JUMPED)
		{
			if (cmd->buttons & BT_SPIN && player->secondjump < 42)  
				player->secondjump++;

			if (player->flyangle > 0 && player->pflags & PF_THOKKED)
			{
				player->flyangle--;

				P_SetObjectMomZ(player->mo, ((player->flyangle-24 - player->secondjump*3)*((player->actionspd>>FRACBITS)/12 + 1)<<FRACBITS)/7, false);

				P_SpawnThokMobj(player);

				if ((player->mo->eflags & MFE_UNDERWATER))
					P_InstaThrust(player->mo, player->mo->angle, FixedMul(player->normalspeed, player->mo->scale)*(80-player->flyangle - (player->actionspd>>FRACBITS)/2)/80);
				else
					P_InstaThrust(player->mo, player->mo->angle, ((FixedMul(player->normalspeed - player->actionspd/4, player->mo->scale))*2)/3);

				player->drawangle = player->mo->angle;
			}
		}
	}






	if (cmd->buttons & BT_JUMP && !player->exiting && !P_PlayerInPain(player))
	{
		if (LUA_HookPlayer(player, HOOK(JumpSpecial)))
			;

		else if (player->pflags & PF_JUMPDOWN)
			;

		else if (P_InQuicksand(player->mo))
		{
			P_DoJump(player, true, false);
			player->secondjump = 0;
			player->pflags &= ~PF_THOKKED;
		}
		else if (player->powers[pw_carry] == CR_MACESPIN && player->mo->tracer)
		{
			player->powers[pw_carry] = CR_NONE;
			P_SetTarget(&player->mo->tracer, NULL);
			player->powers[pw_flashing] = TICRATE/4;
		}

		else if (onground || player->climbing || player->powers[pw_carry])
		{
			P_DoJump(player, true, true);
			player->secondjump = 0;
			player->pflags &= ~PF_THOKKED;
		}
		else if (player->pflags & PF_SLIDING || ((gametyperules & GTR_TEAMFLAGS) && player->gotflag) || player->pflags & PF_SHIELDABILITY)
			;
		                                
   
                                               
                                         
                                          
     
		else if (player->pflags & PF_JUMPED)
		{
			if (!LUA_HookPlayer(player, HOOK(AbilitySpecial)))
			switch (player->charability)
			{
				case CA_THOK:
				case CA_HOMINGTHOK:
				case CA_JUMPTHOK:  


					if (!(player->pflags & PF_THOKKED) || (player->charflags & SF_MULTIABILITY))
					{

						fixed_t actionspd = player->actionspd;

						if (player->charflags & SF_DASHMODE)
							actionspd = max(player->actionspd, FixedDiv(player->speed, player->mo->scale));

						if (player->mo->eflags & MFE_UNDERWATER)
							actionspd >>= 1;

						if ((player->charability == CA_JUMPTHOK) && !(player->pflags & PF_THOKKED))
						{
							player->pflags &= ~PF_JUMPED;
							P_DoJump(player, false, true);
						}

						P_InstaThrust(player->mo, player->mo->angle, FixedMul(actionspd, player->mo->scale));

						if (maptol & TOL_2D)
						{
							player->mo->momx /= 2;
							player->mo->momy /= 2;
						}
						if (player->charability == CA_HOMINGTHOK)
						{
							player->mo->momx /= 2;
							player->mo->momy /= 2;
						}

						if (player->charability == CA_HOMINGTHOK)
						{
							P_SetTarget(&player->mo->target, P_SetTarget(&player->mo->tracer, lockonthok));
							if (lockonthok)
							{
								P_SetMobjState(player->mo, S_PLAY_ROLL);
								player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, lockonthok->x, lockonthok->y);
								player->homing = 3*TICRATE;
							}
							else
							{
								P_SetMobjState(player->mo, S_PLAY_FALL);
								player->pflags &= ~PF_JUMPED;
								player->mo->height = P_GetPlayerHeight(player);
							}
							player->pflags &= ~PF_NOJUMPDAMAGE;
						}

						player->drawangle = player->mo->angle;

						if (player->mo->info->attacksound && !player->spectator)
							S_StartSound(player->mo, player->mo->info->attacksound);  

						P_SpawnThokMobj(player);

						player->pflags &= ~(PF_SPINNING|PF_STARTDASH);
						player->pflags |= PF_THOKKED;



						                    
       
                                                                                                                                                            
                                                     
                                                                                                                                                                          
                                                    
         
					}
					break;

				case CA_FLY:
				case CA_SWIM:


					if (player->charability == CA_SWIM && !(player->mo->eflags & MFE_UNDERWATER))
						;  
					else if (!(player->pflags & PF_THOKKED) && !(player->powers[pw_tailsfly]))
					{
						P_SetMobjState(player->mo, S_PLAY_FLY);  

						player->powers[pw_tailsfly] = tailsflytics + 1;  

						player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE|PF_SPINNING|PF_STARTDASH);
						if (player->bot == BOT_2PAI)
							player->pflags |= PF_THOKKED;
						else
							player->pflags |= (PF_THOKKED|PF_CANCARRY);
						player->powers[pw_strong] = STR_FLY;
					}
					break;
				case CA_GLIDEANDCLIMB:

					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
					{
						fixed_t glidespeed = FixedMul(player->actionspd, player->mo->scale);
						fixed_t playerspeed = player->speed;

						if (player->mo->eflags & MFE_UNDERWATER)
						{
							glidespeed >>= 1;
							playerspeed = 2*playerspeed/3;
							if (!(player->powers[pw_super] || player->powers[pw_sneakers]))
							{
								player->mo->momx = (2*(player->mo->momx - player->cmomx)/3) + player->cmomx;
								player->mo->momy = (2*(player->mo->momy - player->cmomy)/3) + player->cmomy;
							}
						}

						player->pflags |= PF_GLIDING|PF_THOKKED;
						player->glidetime = 0;

						P_SetMobjState(player->mo, S_PLAY_GLIDE);
						if (playerspeed < glidespeed)
							P_Thrust(player->mo, player->mo->angle, glidespeed - playerspeed);
						player->pflags &= ~(PF_JUMPED|PF_SPINNING|PF_STARTDASH);
						player->powers[pw_strong] = STR_GLIDE;
					}
					break;
				case CA_DOUBLEJUMP:  
					if (!(player->pflags & PF_THOKKED) || ((player->charflags & SF_MULTIABILITY) && (player->secondjump < (player->actionspd >> FRACBITS))))
					{
						player->pflags |= PF_THOKKED;
						player->pflags &= ~(PF_JUMPED|PF_SPINNING);
						P_DoJump(player, true, true);
						player->secondjump++;
					}
					break;
				case CA_FLOAT:  
				case CA_SLOWFALL:  
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
					{
						if (player->charflags & SF_DASHMODE && player->dashmode >= DASHMODE_THRESHOLD)
							P_SetMobjState(player->mo, S_PLAY_DASH);
						else if (player->speed >= FixedMul(player->runspeed, player->mo->scale))
							P_SetMobjState(player->mo, S_PLAY_FLOAT_RUN);
						else
							P_SetMobjState(player->mo, S_PLAY_FLOAT);
						player->pflags |= PF_THOKKED;
						player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE|PF_SPINNING);
						player->secondjump = 1;
					}
					break;
				case CA_TELEKINESIS:
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
					{
						P_Telekinesis(player,
							FixedMul(player->actionspd, player->mo->scale),  
							FixedMul(384*FRACUNIT, player->mo->scale));
						player->pflags &= ~PF_SPINNING;
					}
					break;
				case CA_FALLSWITCH:
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
					{
						player->mo->momz = -player->mo->momz;
						P_SpawnThokMobj(player);
						player->pflags |= PF_THOKKED;
						player->pflags &= ~PF_SPINNING;
					}
					break;
				case CA_AIRDRILL:
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
					{
						player->flyangle = 56 + (60-(player->actionspd>>FRACBITS))/3;
						player->pflags |= PF_THOKKED;
						player->pflags &= ~PF_SPINNING;
						S_StartSound(player->mo, sfx_spndsh);
					}
					break;
				case CA_BOUNCE:
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
					{
						P_SetMobjState(player->mo, S_PLAY_BOUNCE);
						player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE|PF_SPINNING);
						player->pflags |= PF_THOKKED|PF_BOUNCING;
						player->powers[pw_strong] = STR_BOUNCE;
						player->mo->momx >>= 1;
						player->mo->momy >>= 1;
						player->mo->momz >>= 1;
					}
					break;
				case CA_TWINSPIN:
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
						P_DoTwinSpin(player);
					break;
				default:
					break;
			}
		}
		else if (player->pflags & PF_THOKKED)
		{
			if (!LUA_HookPlayer(player, HOOK(AbilitySpecial)))
				switch (player->charability)
				{
					case CA_FLY:
					case CA_SWIM:  
						if (player->charability == CA_SWIM && !(player->mo->eflags & MFE_UNDERWATER))
							;  
						else if (player->powers[pw_tailsfly])  
						{
							player->fly1 = 20;

							if (player->charability == CA_SWIM)
								player->fly1 /= 2;


							if (player->speed > FixedMul(8*FRACUNIT, player->mo->scale) && player->speed > FixedMul(player->normalspeed>>1, player->mo->scale))
								P_Thrust(player->mo, R_PointToAngle2(0,0,player->mo->momx,player->mo->momy), FixedMul(-4*FRACUNIT, player->mo->scale));
						}
						break;
					default:
						break;
				}
		}
		else if ((!(player->charflags & SF_NOSHIELDABILITY)) && ((player->powers[pw_shield] & SH_NOSTACK) == SH_WHIRLWIND && !player->powers[pw_super] && !LUA_HookPlayer(player, HOOK(ShieldSpecial))))
			P_DoJumpShield(player);
	}


	if ((player->powers[pw_shield] & SH_NOSTACK) == SH_ATTRACT  
	&& player->pflags & PF_SHIELDABILITY)
	{
		if (player->homing && player->mo->tracer)
		{
			if (!P_HomingAttack(player->mo, player->mo->tracer))
			{
				player->pflags &= ~PF_SHIELDABILITY;
				player->secondjump = UINT8_MAX;
				P_SetObjectMomZ(player->mo, 6*FRACUNIT, false);
				if (player->mo->eflags & MFE_UNDERWATER)
					player->mo->momz = FixedMul(player->mo->momz, FRACUNIT/3);
				player->homing = 0;
			}
		}


		if (!(player->pflags & PF_JUMPED))
			player->homing = 0;
	}
	else if (player->charability == CA_HOMINGTHOK)  
	{

		if (player->homing && player->mo->tracer)
		{
			P_SpawnThokMobj(player);


			if (!P_HomingAttack(player->mo, player->mo->tracer))
			{
				if (player->mo->eflags & MFE_UNDERWATER)
					P_SetObjectMomZ(player->mo, FixedDiv(457*FRACUNIT,72*FRACUNIT), false);
				else
					P_SetObjectMomZ(player->mo, 10*FRACUNIT, false);

				player->mo->momx = player->mo->momy = player->homing = 0;

				if (player->mo->tracer->flags2 & MF2_FRET)
					P_InstaThrust(player->mo, player->mo->angle, -(player->speed>>3));

				if (!(player->mo->tracer->flags & MF_BOSS))
					player->pflags &= ~PF_THOKKED;

				P_SetMobjState(player->mo, S_PLAY_SPRING);
				player->pflags |= PF_NOJUMPDAMAGE;
			}
		}


		if (!(player->pflags & PF_JUMPED))
			player->homing = 0;
	}
	else
		player->homing = 0;

	if (cmd->buttons & BT_JUMP)
	{
		player->pflags |= PF_JUMPDOWN;

		if ((!(gametyperules & GTR_TEAMFLAGS) || !player->gotflag) && !player->exiting)
		{
			if (player->secondjump == 1 && player->charability != CA_DOUBLEJUMP && player->charability != CA_THOK)
			{
				fixed_t potentialmomz;
				if (player->charability == CA_SLOWFALL)
					potentialmomz = FixedMul(gravity, -4*player->mo->scale);
				else
					potentialmomz = ((player->speed < 10*player->mo->scale)
					? (player->speed - 10*player->mo->scale)/5
					: 0);
				if (P_MobjFlip(player->mo)*player->mo->momz < potentialmomz)
					player->mo->momz = P_MobjFlip(player->mo)*potentialmomz;
				player->pflags &= ~PF_SPINNING;
			}
		}
	}
	else  
	{
		player->pflags &= ~PF_JUMPDOWN;


		if (player->secondjump == 1 && player->charability != CA_DOUBLEJUMP && player->charability != CA_AIRDRILL && player->charability != CA_THOK)
		{
			if (player->charflags & SF_MULTIABILITY)
			{
				player->pflags |= (PF_JUMPED|PF_NOJUMPDAMAGE);
				player->secondjump = 0;
			}
			else
				player->secondjump = 2;

			P_SetMobjState(player->mo, S_PLAY_FALL);
		}


		if (((player->pflags & (PF_JUMPED|PF_STARTJUMP)) == (PF_JUMPED|PF_STARTJUMP)) && (P_MobjFlip(player->mo)*player->mo->momz > 0))
		{
			player->mo->momz >>= 1;
			player->pflags &= ~PF_STARTJUMP;
		}
	}
}










INT32 P_GetPlayerControlDirection(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;
	angle_t controllerdirection, controlplayerdirection;
	angle_t dangle;
	fixed_t tempx = 0, tempy = 0;
	angle_t tempangle, origtempangle;

	if (!cmd->forwardmove && !cmd->sidemove)
		return 0;

	if (!player->mo->momx && !player->mo->momy)
		return 0;

	if (twodlevel || player->mo->flags2 & MF2_TWOD)
	{
		if (!cmd->sidemove)
			return 0;
		if (!player->mo->momx)
			return 0;
		origtempangle = tempangle = 0;  
		controlplayerdirection = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
	}
	else
	{
		if (player->awayviewtics)
			origtempangle = tempangle = player->awayviewmobj->angle;
		else if (P_ControlStyle(player) & CS_LMAOGALOG)
			origtempangle = tempangle = (cmd->angleturn << 16);
		else
			origtempangle = tempangle = player->mo->angle;

		controlplayerdirection = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
	}



	tempangle >>= ANGLETOFINESHIFT;
	if (!(twodlevel || player->mo->flags2 & MF2_TWOD))  
	{
		tempx += FixedMul(cmd->forwardmove*FRACUNIT,FINECOSINE(tempangle));
		tempy += FixedMul(cmd->forwardmove*FRACUNIT,FINESINE(tempangle));

		tempangle = origtempangle-ANGLE_90;
		tempangle >>= ANGLETOFINESHIFT;
	}
	tempx += FixedMul(cmd->sidemove*FRACUNIT,FINECOSINE(tempangle));
	tempy += FixedMul(cmd->sidemove*FRACUNIT,FINESINE(tempangle));

	controllerdirection = R_PointToAngle2(0, 0, tempx, tempy);

	dangle = controllerdirection - controlplayerdirection;

	if (dangle > ANGLE_180)  
		dangle = InvAngle(dangle);

	if (dangle > ANGLE_90)
		return 2;  
	else
		return 1;  
}

static boolean P_ShouldResetConveyorMomentum(player_t *player)
{
	switch (player->onconveyor)
	{
		case 1:
			return false;
		case 2:  
			return !(player->mo->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER));
		case 3:
		default:
			return true;
		case 4:  
			return !P_IsObjectOnGround(player->mo);
	}
}


static void P_2dMovement(player_t *player)
{
	ticcmd_t *cmd;
	INT32 topspeed, acceleration, thrustfactor;
	fixed_t movepushforward = 0;
	angle_t movepushangle = 0;
	fixed_t normalspd = FixedMul(player->normalspeed, player->mo->scale);

	cmd = &player->cmd;

	if (player->exiting || player->pflags & PF_STASIS)
	{
		cmd->forwardmove = cmd->sidemove = 0;
		if (player->pflags & PF_GLIDING)
		{
			if (!player->skidtime)
				player->pflags &= ~PF_GLIDING;
			else if (player->exiting)
			{
				player->pflags &= ~PF_GLIDING;
				P_SetMobjState(player->mo, S_PLAY_WALK);
				player->skidtime = 0;
			}
		}
		if (player->pflags & PF_BOUNCING)
			player->pflags &= ~PF_BOUNCING;
		if (player->pflags & PF_SPINNING && !player->exiting)
		{
			player->pflags &= ~PF_SPINNING;
			P_SetMobjState(player->mo, S_PLAY_STND);
		}
	}

	if (P_ShouldResetConveyorMomentum(player))
		player->cmomx = player->cmomy = 0;

	player->rmomx = player->mo->momx - player->cmomx;
	player->rmomy = player->mo->momy - player->cmomy;


	player->speed = abs(player->rmomx);

	if (player->pflags & PF_GLIDING)
	{

		if (player->mo->angle < ANGLE_180 && player->mo->angle > ANGLE_90)
			player->mo->angle = ANGLE_180;
		else if (player->mo->angle < ANGLE_90 && player->mo->angle > 0)
			player->mo->angle = 0;

		if (cmd->sidemove > 0 && player->mo->angle != 0 && player->mo->angle >= ANGLE_180)
			player->mo->angle += 1280<<FRACBITS;
		else if (cmd->sidemove < 0 && player->mo->angle != ANGLE_180 && (player->mo->angle > ANGLE_180 || player->mo->angle == 0))
			player->mo->angle -= 1280<<FRACBITS;
		else if (cmd->sidemove == 0)
		{
			if (player->mo->angle >= ANGLE_270)
				player->mo->angle += 1280<<FRACBITS;
			else if (player->mo->angle < ANGLE_270 && player->mo->angle > ANGLE_180)
				player->mo->angle -= 1280<<FRACBITS;
		}
	}
	else if (cmd->sidemove && !(player->climbing) && !P_PlayerInPain(player))
	{
		if (cmd->sidemove > 0)
			player->mo->angle = 0;
		else if (cmd->sidemove < 0)
			player->mo->angle = ANGLE_180;
	}

	P_SetPlayerAngle(player, player->mo->angle);

	if (player->pflags & PF_GLIDING)
		movepushangle = player->mo->angle;
	else
	{
		if (cmd->sidemove > 0)
			movepushangle = 0;
		else if (cmd->sidemove < 0)
			movepushangle = ANGLE_180;
		else
			movepushangle = player->mo->angle;
	}


	onground = P_IsObjectOnGround(player->mo);

	player->aiming = cmd->aiming<<FRACBITS;


	if (maptol & TOL_2D)
		normalspd = FixedMul(normalspd, 2*FRACUNIT/3);

	if (player->powers[pw_super] || player->powers[pw_sneakers])
	{
		thrustfactor = player->thrustfactor*2;
		acceleration = player->accelstart/2 + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration/2;

		if (player->powers[pw_tailsfly])
			topspeed = normalspd;
		else if (player->mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER) && !(player->pflags & PF_SLIDING))
		{
			topspeed = normalspd;
			acceleration = 2*acceleration/3;
		}
		else
			topspeed = normalspd * 2;
	}
	else
	{
		thrustfactor = player->thrustfactor;
		acceleration = player->accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration;

		if (player->powers[pw_tailsfly])
			topspeed = normalspd/2;
		else if (player->mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER) && !(player->pflags & PF_SLIDING))
		{
			topspeed = normalspd/2;
			acceleration = 2*acceleration/3;
		}
		else
			topspeed = normalspd;
	}


	if (player->climbing)
	{
		if (cmd->forwardmove != 0)
			P_SetObjectMomZ(player->mo, FixedDiv(cmd->forwardmove*FRACUNIT, player->powers[pw_super] ? 5*FRACUNIT : 15*FRACUNIT>>1), false);  

		player->mo->momx = 0;
	}
	else if (cmd->sidemove != 0 && !(player->pflags & PF_GLIDING || player->exiting
		|| (P_PlayerInPain(player) && !onground)))
	{
		movepushforward = abs(cmd->sidemove) * (thrustfactor * acceleration);


		if (!onground)
			movepushforward >>= 1;  


		if ((player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_SPINNING)
		{
			if (!(player->pflags & PF_STARTDASH))
				movepushforward = movepushforward/48;
			else
				movepushforward = 0;
		}

		movepushforward = FixedMul(movepushforward, player->mo->scale);
		if (player->rmomx < topspeed && cmd->sidemove > 0)  
			P_Thrust(player->mo, movepushangle, movepushforward);
		else if (player->rmomx > -topspeed && cmd->sidemove < 0)
			P_Thrust(player->mo, movepushangle, movepushforward);
	}
	player->mo->friction = ORIG_FRICTION;  
}


static void P_3dMovement(player_t *player)
{
	ticcmd_t *cmd;
	angle_t movepushangle, movepushsideangle;  
	INT32 topspeed, acceleration, thrustfactor;
	fixed_t movepushforward = 0, movepushside = 0;
	INT32 mforward = 0, mbackward = 0;
	angle_t dangle;  
	fixed_t normalspd = FixedMul(player->normalspeed, player->mo->scale);
	controlstyle_e controlstyle;
	boolean spin = ((onground = P_IsObjectOnGround(player->mo)) && (player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_SPINNING && (player->rmomx || player->rmomy) && !(player->pflags & PF_STARTDASH));
	fixed_t oldMagnitude, newMagnitude;
	vector3_t totalthrust;

	totalthrust.x = totalthrust.y = 0;  
	totalthrust.z = FRACUNIT*P_MobjFlip(player->mo)/3;  


	oldMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);

	controlstyle = P_ControlStyle(player);

	cmd = &player->cmd;

	if (player->exiting || player->pflags & PF_STASIS)
	{
		cmd->forwardmove = cmd->sidemove = 0;
		if (player->pflags & PF_GLIDING)
		{
			if (!player->skidtime)
				player->pflags &= ~PF_GLIDING;
			else if (player->exiting)
			{
				player->pflags &= ~PF_GLIDING;
				P_SetMobjState(player->mo, S_PLAY_WALK);
				player->skidtime = 0;
			}
		}
		if (player->pflags & PF_BOUNCING)
			player->pflags &= ~PF_BOUNCING;
		if (player->pflags & PF_SPINNING && !player->exiting)
		{
			player->pflags &= ~PF_SPINNING;
			P_SetMobjState(player->mo, S_PLAY_STND);
		}
	}

	if (controlstyle & CS_LMAOGALOG)
	{
		movepushangle = (cmd->angleturn<<16  );
	}
	else
	{
		movepushangle = player->mo->angle;
	}
	movepushsideangle = movepushangle-ANGLE_90;

	if (P_ShouldResetConveyorMomentum(player))
		player->cmomx = player->cmomy = 0;

	player->rmomx = player->mo->momx - player->cmomx;
	player->rmomy = player->mo->momy - player->cmomy;


	player->speed = P_AproxDistance(player->rmomx, player->rmomy);




	dangle = R_PointToAngle2(0,0, player->rmomx, player->rmomy) - (cmd->angleturn<<16);
	if (dangle > ANGLE_180)  
		dangle = InvAngle(dangle);


	if (dangle <= ANGLE_45)  
		mforward = 1;  
	else if (dangle >= ANGLE_135)  
		mbackward = 1;  




	if (player->pflags & PF_SLIDING)
		cmd->forwardmove = 0;
	else if (onground && player->mo->state == states+S_PLAY_PAIN)
		P_SetMobjState(player->mo, S_PLAY_WALK);

	player->aiming = cmd->aiming<<FRACBITS;


	if (player->pflags & PF_SLIDING)
	{
		normalspd = FixedMul(36<<FRACBITS, player->mo->scale);
		thrustfactor = 5;
		acceleration = 96 + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * 40;
		topspeed = normalspd;
	}
	else
	{
		if (player->powers[pw_super] || player->powers[pw_sneakers])
		{
			topspeed = 5 * normalspd / 3;  
			thrustfactor = player->thrustfactor*2;
			acceleration = player->accelstart/2 + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration/2;
		}
		else
		{
			topspeed = normalspd;
			thrustfactor = player->thrustfactor;
			acceleration = player->accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration;
		}

		if (player->powers[pw_tailsfly])
			topspeed >>= 1;
		else if (player->mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER))
		{
			topspeed >>= 1;
			acceleration = 2*acceleration/3;
		}
	}

	if (spin)  
	{
		const fixed_t ns = FixedDiv(549*ORIG_FRICTION,500*FRACUNIT);  
		topspeed = FixedMul(oldMagnitude, ns);
	}


	if (player->powers[pw_tailsfly])
	{
		thrustfactor = player->thrustfactor*2;
		acceleration = player->accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration;
	}
	else
	{
		if (player->pflags & PF_BOUNCING)
		{
			if (player->mo->state-states == S_PLAY_BOUNCE_LANDING)
			{
				thrustfactor = player->thrustfactor*8;
				acceleration = player->accelstart/8 + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration/8;
			}
			else
			{
				thrustfactor = (3*player->thrustfactor)/4;
				acceleration = player->accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration;
			}
		}

		if (player->mo->movefactor != FRACUNIT)  
			acceleration = FixedMul(acceleration<<FRACBITS, player->mo->movefactor)>>FRACBITS;
	}


	if (player->climbing)
	{
		if (cmd->forwardmove)
		{
			if (player->mo->eflags & MFE_UNDERWATER)
				P_SetObjectMomZ(player->mo, FixedDiv(cmd->forwardmove*FRACUNIT, player->powers[pw_super] ? 20*FRACUNIT/3 : 10*FRACUNIT), false);  
			else
				P_SetObjectMomZ(player->mo, FixedDiv(cmd->forwardmove*FRACUNIT, player->powers[pw_super] ? 5*FRACUNIT : 15*FRACUNIT>>1), false);  
		}
	}
	else if (!(controlstyle == CS_LMAOGALOG)
		&& cmd->forwardmove != 0 && !(player->pflags & PF_GLIDING || player->exiting
		|| (P_PlayerInPain(player) && !onground)))
	{
		movepushforward = cmd->forwardmove * (thrustfactor * acceleration);


		if ((player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_SPINNING)
		{
			if ((mforward && cmd->forwardmove > 0) || (mbackward && cmd->forwardmove < 0)
			|| (player->pflags & PF_STARTDASH))
				movepushforward = 0;
			else if (onground)
				movepushforward >>= 4;
			else
				movepushforward >>= 3;
		}

		else if (!onground)
			movepushforward >>= 2;  

		movepushforward = FixedMul(movepushforward, player->mo->scale);

		totalthrust.x += P_ReturnThrustX(player->mo, movepushangle, movepushforward);
		totalthrust.y += P_ReturnThrustY(player->mo, movepushangle, movepushforward);
	}

	if (player->climbing)
	{
		if (player->mo->eflags & MFE_UNDERWATER)
			P_InstaThrust(player->mo, player->mo->angle-ANGLE_90, FixedDiv(cmd->sidemove*player->mo->scale, player->powers[pw_super] ? 20*FRACUNIT/3 : 10*FRACUNIT));  
		else
			P_InstaThrust(player->mo, player->mo->angle-ANGLE_90, FixedDiv(cmd->sidemove*player->mo->scale, player->powers[pw_super] ? 5*FRACUNIT : 15*FRACUNIT>>1));  
	}

	else if (controlstyle == CS_LMAOGALOG)
	{
		if (!(player->pflags & PF_GLIDING || player->exiting || P_PlayerInPain(player)))
		{
			angle_t controldirection;




			controldirection = R_PointToAngle2(0, 0, cmd->forwardmove*FRACUNIT, -cmd->sidemove*FRACUNIT)+movepushangle;

			movepushforward = FixedHypot(cmd->sidemove, cmd->forwardmove) * (thrustfactor * acceleration);


			if ((player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_SPINNING)
			{
				if ((mforward && cmd->forwardmove > 0) || (mbackward && cmd->forwardmove < 0)
				|| (player->pflags & PF_STARTDASH))
					movepushforward = 0;
				else if (onground)
					movepushforward >>= 4;
				else
					movepushforward >>= 3;
			}

			else if (!onground)
				movepushforward >>= 2;  

			movepushsideangle = controldirection;

			movepushforward = FixedMul(movepushforward, player->mo->scale);

			totalthrust.x += P_ReturnThrustX(player->mo, controldirection, movepushforward);
			totalthrust.y += P_ReturnThrustY(player->mo, controldirection, movepushforward);
		}
	}
	else if (cmd->sidemove && !(player->pflags & PF_GLIDING) && !player->exiting && !P_PlayerInPain(player))
	{
		movepushside = cmd->sidemove * (thrustfactor * acceleration);


		if (!onground)
		{
			movepushside >>= 2;  

			if (((player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_SPINNING) || (player->powers[pw_tailsfly] && player->speed > topspeed))
				movepushside >>= 2;
		}

		else if ((player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_SPINNING)
		{
			if (player->pflags & PF_STARTDASH)
				movepushside = 0;
			else if (onground)
				movepushside >>= 4;
			else
				movepushside >>= 3;
		}


		movepushside = FixedMul(movepushside, player->mo->scale);

		totalthrust.x += P_ReturnThrustX(player->mo, movepushsideangle, movepushside);
		totalthrust.y += P_ReturnThrustY(player->mo, movepushsideangle, movepushside);
	}

	if ((totalthrust.x || totalthrust.y)
		&& player->mo->standingslope && (!(player->mo->standingslope->flags & SL_NOPHYSICS)) && abs(player->mo->standingslope->zdelta) > FRACUNIT/2) {


		angle_t thrustangle = R_PointToAngle2(0, 0, totalthrust.x, totalthrust.y)-player->mo->standingslope->xydirection;

		if (player->mo->standingslope->zdelta < 0) {  
			if (thrustangle < ANGLE_90 || thrustangle > ANGLE_270) {
				P_QuantizeMomentumToSlope(&totalthrust, player->mo->standingslope);
			}
		} else {  
			if (thrustangle > ANGLE_90 && thrustangle < ANGLE_270) {
				P_QuantizeMomentumToSlope(&totalthrust, player->mo->standingslope);
			}
		}
	}

	player->mo->momx += totalthrust.x;
	player->mo->momy += totalthrust.y;











	newMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);
	if (newMagnitude > topspeed)
	{
		fixed_t tempmomx, tempmomy;
		if (oldMagnitude > topspeed && !spin)
		{
			if (newMagnitude > oldMagnitude)
			{
				tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), oldMagnitude);
				tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), oldMagnitude);
				player->mo->momx = tempmomx + player->cmomx;
				player->mo->momy = tempmomy + player->cmomy;
			}

		}
		else
		{
			tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), topspeed);
			tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), topspeed);
			player->mo->momx = tempmomx + player->cmomx;
			player->mo->momy = tempmomy + player->cmomy;
		}
	}
	player->mo->friction = ORIG_FRICTION;  
}






static void P_SpectatorMovement(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;

	player->mo->angle = (cmd->angleturn<<16  );

	ticruned++;
	if (!(cmd->angleturn & TICCMD_RECEIVED))
		ticmiss++;

	if (cmd->buttons & BT_JUMP)
		player->mo->z += FRACUNIT*16;
	else if (cmd->buttons & BT_SPIN)
		player->mo->z -= FRACUNIT*16;

	if (player->mo->z > player->mo->ceilingz - player->mo->height)
		player->mo->z = player->mo->ceilingz - player->mo->height;
	if (player->mo->z < player->mo->floorz)
		player->mo->z = player->mo->floorz;



	player->aiming = cmd->aiming<<FRACBITS;

	player->mo->momx = player->mo->momy = player->mo->momz = 0;
	if (cmd->forwardmove != 0)
	{
		P_Thrust(player->mo, player->mo->angle, cmd->forwardmove*(FRACUNIT/2));


		player->mo->momz += FixedMul(cmd->forwardmove*(FRACUNIT/2), AIMINGTOSLOPE(player->aiming));
	}
	if (cmd->sidemove != 0)
	{
		P_Thrust(player->mo, player->mo->angle-ANGLE_90, cmd->sidemove*(FRACUNIT/2));
	}
}








static void P_ShootLine(mobj_t *source, mobj_t *dest, fixed_t height)
{
	mobj_t *mo;
	INT32 i;
	fixed_t temp;
	INT32 speed, seesound;

	temp = dest->z;
	dest->z = height;

	seesound = mobjinfo[MT_REDRING].seesound;
	speed = mobjinfo[MT_REDRING].speed;
	mobjinfo[MT_REDRING].seesound = sfx_None;
	mobjinfo[MT_REDRING].speed = 20*FRACUNIT;

	mo = P_SpawnXYZMissile(source, dest, MT_REDRING, source->x, source->y, height);

	dest->z = temp;
	if (mo)
	{
		mo->flags2 |= MF2_RAILRING;
		mo->flags2 |= MF2_DONTDRAW;
		mo->flags |= MF_NOCLIPHEIGHT;
		mo->flags |= MF_NOCLIP;
		mo->flags &= ~MF_MISSILE;
		mo->fuse = 3;
	}

	for (i = 0; i < 32; i++)
	{
		if (mo)
		{
			if (!(mo->flags & MF_NOBLOCKMAP))
			{
				P_UnsetThingPosition(mo);
				mo->flags |= MF_NOBLOCKMAP;
				P_SetThingPosition(mo);
			}
			if (i&1)
				P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

			P_UnsetThingPosition(mo);
			mo->x += mo->momx;
			mo->y += mo->momy;
			mo->z += mo->momz;
			P_SetThingPosition(mo);
		}
		else
		{
			mobjinfo[MT_REDRING].seesound = seesound;
			mobjinfo[MT_REDRING].speed = speed;
			return;
		}
	}
	mobjinfo[MT_REDRING].seesound = seesound;
	mobjinfo[MT_REDRING].speed = speed;
}

#define MAXDRILLSPEED 14000
#define MAXNORMALSPEED 6250  

static void P_NightsTransferPoints(player_t *player, fixed_t xspeed, fixed_t radius)
{
	if (player->pflags & PF_TRANSFERTOCLOSEST)
	{
		const angle_t fa = R_PointToAngle2(player->axis1->x, player->axis1->y, player->axis2->x, player->axis2->y);
		P_InstaThrust(player->mo, fa, xspeed/10);
	}
	else
	{
		const angle_t fa = player->angle_pos>>ANGLETOFINESHIFT;
		const angle_t faold = player->old_angle_pos>>ANGLETOFINESHIFT;
		player->mo->momx = FixedMul(FINECOSINE(fa),radius) - FixedMul(FINECOSINE(faold),radius);
		player->mo->momy = FixedMul(FINESINE(fa),radius) - FixedMul(FINESINE(faold),radius);
	}

	if (player->exiting)
		return;

	if (!P_CheckMove(player->mo,
				player->mo->x + player->mo->momx,
				player->mo->y + player->mo->momy, true))
	{
		return;
	}

	{
		const INT32 sequence = player->mo->target->threshold;
		mobj_t *transfer1 = NULL;
		mobj_t *transfer2 = NULL;
		mobj_t *axis;
		mobj_t *mo2;
		thinker_t *th;
		line_t transfer1line;
		line_t transfer2line;
		boolean transfer1last = false;
		boolean transfer2last = false;
		vertex_t vertices[4];
		fixed_t truexspeed = xspeed*(!(player->pflags & PF_TRANSFERTOCLOSEST) && player->mo->target->flags2 & MF2_AMBUSH ? -1 : 1);


		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->removing)
				continue;

			mo2 = (mobj_t *)th;


			if (!(mo2->flags2 & MF2_AXIS))
				break;
			if (!(mo2->type == MT_AXISTRANSFER || mo2->type == MT_AXISTRANSFERLINE))
				continue;
			if (mo2->threshold != sequence)
				continue;

			if (player->pflags & PF_TRANSFERTOCLOSEST)
			{
				if (mo2->health == player->axis1->health)
					transfer1 = mo2;
				else if (mo2->health == player->axis2->health)
					transfer2 = mo2;
			}
			else
			{
				if (mo2->health == player->mo->target->health)
					transfer1 = mo2;
				else if (mo2->health == player->mo->target->health + 1)
					transfer2 = mo2;
			}
		}




		if (!transfer1)
		{
			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->removing)
					continue;

				mo2 = (mobj_t *)th;


				if (!(mo2->flags2 & MF2_AXIS))
					break;
				if (!(mo2->type == MT_AXISTRANSFER || mo2->type == MT_AXISTRANSFERLINE))
					continue;
				if (mo2->threshold != sequence)
					continue;

				if (!transfer1)
				{
					transfer1 = mo2;
					transfer1last = true;
				}
				else if (mo2->health > transfer1->health)
				{
					transfer1 = mo2;
					transfer1last = true;
				}
			}
		}
		if (!transfer2)
		{
			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->removing)
					continue;

				mo2 = (mobj_t *)th;


				if (!(mo2->flags2 & MF2_AXIS))
					break;
				if (!(mo2->type == MT_AXISTRANSFER || mo2->type == MT_AXISTRANSFERLINE))
					continue;
				if (mo2->threshold != sequence)
					continue;

				if (!transfer2)
				{
					transfer2 = mo2;
					transfer2last = true;
				}
				else if (mo2->health > transfer2->health)
				{
					transfer2 = mo2;
					transfer2last = true;
				}
			}
		}

		if (!(transfer1 && transfer2))  
			I_Error("Mare does not form a complete circuit!\n");

		transfer1line.v1 = &vertices[0];
		transfer1line.v2 = &vertices[1];
		transfer2line.v1 = &vertices[2];
		transfer2line.v2 = &vertices[3];

		if (cv_debug && (leveltime % TICRATE == 0))
		{
			CONS_Debug(DBG_NIGHTS, "Transfer1 : %d\n", transfer1->health);
			CONS_Debug(DBG_NIGHTS, "Transfer2 : %d\n", transfer2->health);
		}








		if (P_AproxDistance(transfer1->x - player->mo->x, transfer1->y - player->mo->y)>>FRACBITS
			< P_AproxDistance(transfer2->x - player->mo->x, transfer2->y - player->mo->y)>>FRACBITS)
		{


			if (transfer1->type == MT_AXISTRANSFERLINE)
			{
				if (transfer1last)
					axis = P_FindAxis(transfer1->threshold, transfer1->health-2);
				else if (player->pflags & PF_TRANSFERTOCLOSEST)
					axis = P_FindAxis(transfer1->threshold, transfer1->health-1);
				else
					axis = P_FindAxis(transfer1->threshold, transfer1->health);

				if (!axis)
				{
					CONS_Debug(DBG_NIGHTS, "Unable to find an axis - error code #1\n");
					return;
				}



				transfer1line.v1->x = axis->x;
				transfer1line.v1->y = axis->y;

				transfer1line.v2->x = transfer1->x;
				transfer1line.v2->y = transfer1->y;

				if (cv_debug & DBG_NIGHTS)
					P_ShootLine(axis, transfer1, player->mo->z);



				transfer1line.dx = transfer1line.v2->x - transfer1line.v1->x;
				transfer1line.dy = transfer1line.v2->y - transfer1line.v1->y;

				if (P_PointOnLineSide(player->mo->x, player->mo->y, &transfer1line)
						!= P_PointOnLineSide(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, &transfer1line)
						&& truexspeed < 0)
				{
					if (cv_debug & DBG_NIGHTS)
					{
						HU_SetCEchoDuration(1);
						HU_DoCEcho("transfer!");
						HU_SetCEchoDuration(5);
						S_StartSound(NULL, sfx_strpst);
					}
					if (player->pflags & PF_TRANSFERTOCLOSEST)
					{
						player->pflags &= ~PF_TRANSFERTOCLOSEST;
						P_TransferToAxis(player, transfer1->health - 1);
					}
					else
					{
						player->pflags |= PF_TRANSFERTOCLOSEST;
						P_SetTarget(&player->axis2, transfer1);
						P_SetTarget(&player->axis1, P_FindAxisTransfer(transfer1->threshold, transfer1->health-1, MT_AXISTRANSFERLINE)); 
					}
				}
			}
			else
			{

				if (transfer1last)
					axis = P_FindAxis(transfer1->threshold, 1);
				else
					axis = P_FindAxis(transfer1->threshold, transfer1->health);

				if (!axis)
				{
					CONS_Debug(DBG_NIGHTS, "Unable to find an axis - error code #2\n");
					return;
				}



				transfer1line.v1->x = axis->x;
				transfer1line.v1->y = axis->y;

				if (cv_debug & DBG_NIGHTS)
					P_ShootLine(transfer1, P_FindAxis(transfer1->threshold, transfer1->health-1), player->mo->z);





				transfer1line.v2->x = transfer1->x;
				transfer1line.v2->y = transfer1->y;

				transfer1line.dx = transfer1line.v2->x - transfer1line.v1->x;
				transfer1line.dy = transfer1line.v2->y - transfer1line.v1->y;

				if (P_PointOnLineSide(player->mo->x, player->mo->y, &transfer1line)
					!= P_PointOnLineSide(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, &transfer1line)
						&& truexspeed < 0)
				{
					if (cv_debug & DBG_NIGHTS)
					{
						HU_SetCEchoDuration(1);
						HU_DoCEcho("transfer!");
						HU_SetCEchoDuration(5);
						S_StartSound(NULL, sfx_strpst);
					}
					if (player->mo->target->health < transfer1->health)
					{


						if (transfer1last)
							P_TransferToAxis(player, transfer1->health - 1);
						else
							P_TransferToAxis(player, transfer1->health);
					}
					else if (player->mo->target->health >= transfer1->health)
					{


						P_TransferToAxis(player, transfer1->health - 1);
					}
				}
			}
		}
		else
		{

			if (transfer2->type == MT_AXISTRANSFERLINE)
			{
				if (transfer2last)
					axis = P_FindAxis(transfer2->threshold, 1);
				else if (player->pflags & PF_TRANSFERTOCLOSEST)
					axis = P_FindAxis(transfer2->threshold, transfer2->health);
				else
					axis = P_FindAxis(transfer2->threshold, transfer2->health - 1);

				if (!axis)
					axis = P_FindAxis(transfer2->threshold, 1);

				if (!axis)
				{
					CONS_Debug(DBG_NIGHTS, "Unable to find an axis - error code #3\n");
					return;
				}



				transfer2line.v1->x = axis->x;
				transfer2line.v1->y = axis->y;

				transfer2line.v2->x = transfer2->x;
				transfer2line.v2->y = transfer2->y;



				if (cv_debug & DBG_NIGHTS)
					P_ShootLine(axis, transfer2, player->mo->z);

				transfer2line.dx = transfer2line.v2->x - transfer2line.v1->x;
				transfer2line.dy = transfer2line.v2->y - transfer2line.v1->y;

				if (P_PointOnLineSide(player->mo->x, player->mo->y, &transfer2line)
						!= P_PointOnLineSide(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, &transfer2line)
						&& truexspeed > 0)
				{
					if (cv_debug & DBG_NIGHTS)
					{
						HU_SetCEchoDuration(1);
						HU_DoCEcho("transfer!");
						HU_SetCEchoDuration(5);
						S_StartSound(NULL, sfx_strpst);
					}
					if (player->pflags & PF_TRANSFERTOCLOSEST)
					{
						player->pflags &= ~PF_TRANSFERTOCLOSEST;

						if (!P_FindAxis(transfer2->threshold, transfer2->health))
							transfer2last = true;

						if (transfer2last)
							P_TransferToAxis(player, 1);
						else
							P_TransferToAxis(player, transfer2->health);
					}
					else
					{
						player->pflags |= PF_TRANSFERTOCLOSEST;
						P_SetTarget(&player->axis1, transfer2);
						P_SetTarget(&player->axis2, P_FindAxisTransfer(transfer2->threshold, transfer2->health+1, MT_AXISTRANSFERLINE)); 
					}
				}
			}
			else
			{

				if (transfer2last)
					axis = P_FindAxis(transfer2->threshold, 1);
				else
					axis = P_FindAxis(transfer2->threshold, transfer2->health);

				if (!axis)
					axis = P_FindAxis(transfer2->threshold, 1);

				if (!axis)
				{
					CONS_Debug(DBG_NIGHTS, "Unable to find an axis - error code #4\n");
					return;
				}



				transfer2line.v1->x = axis->x;
				transfer2line.v1->y = axis->y;

				if (cv_debug & DBG_NIGHTS)
					P_ShootLine(transfer2, P_FindAxis(transfer2->threshold, transfer2->health-1), player->mo->z);





				transfer2line.v2->x = transfer2->x;
				transfer2line.v2->y = transfer2->y;

				transfer2line.dx = transfer2line.v2->x - transfer2line.v1->x;
				transfer2line.dy = transfer2line.v2->y - transfer2line.v1->y;

				if (P_PointOnLineSide(player->mo->x, player->mo->y, &transfer2line)
					!= P_PointOnLineSide(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, &transfer2line)
						&& truexspeed > 0)
				{
					if (cv_debug & DBG_NIGHTS)
					{
						HU_SetCEchoDuration(1);
						HU_DoCEcho("transfer!");
						HU_SetCEchoDuration(5);
						S_StartSound(NULL, sfx_strpst);
					}
					if (player->mo->target->health < transfer2->health)
					{
						if (!P_FindAxis(transfer2->threshold, transfer2->health))
							transfer2last = true;

						if (transfer2last)
							P_TransferToAxis(player, 1);
						else
							P_TransferToAxis(player, transfer2->health);
					}
					else if (player->mo->target->health >= transfer2->health)
						P_TransferToAxis(player, transfer2->health - 1);
				}
			}
		}
	}
}






static void P_DoNiGHTSCapsule(player_t *player)
{
	INT32 i, spherecount, totalduration, popduration, deductinterval, deductquantity, sphereresult, firstpoptic;
	INT32 tictimer = ++player->capsule->extravalue2;

	if (abs(player->mo->x-player->capsule->x) <= 3*FRACUNIT)
	{
		P_UnsetThingPosition(player->mo);
		player->mo->x = player->capsule->x;
		P_SetThingPosition(player->mo);
		player->mo->momx = 0;
	}

	if (abs(player->mo->y-player->capsule->y) <= 3*FRACUNIT)
	{
		P_UnsetThingPosition(player->mo);
		player->mo->y = player->capsule->y;
		P_SetThingPosition(player->mo);
		player->mo->momy = 0;
	}

	if (abs(player->mo->z - (player->capsule->z+(player->capsule->height/3))) <= 3*FRACUNIT)
	{
		player->mo->z = player->capsule->z+(player->capsule->height/3);
		player->mo->momz = 0;
	}

	if (player->mo->x > player->capsule->x)
		player->mo->momx = -3*FRACUNIT;
	else if (player->mo->x < player->capsule->x)
		player->mo->momx = 3*FRACUNIT;

	if (player->mo->y > player->capsule->y)
		player->mo->momy = -3*FRACUNIT;
	else if (player->mo->y < player->capsule->y)
		player->mo->momy = 3*FRACUNIT;

	if (player->mo->z > player->capsule->z+(player->capsule->height/3))
		player->mo->momz = -3*FRACUNIT;
	else if (player->mo->z < player->capsule->z+(player->capsule->height/3))
		player->mo->momz = 3*FRACUNIT;

	if (player->powers[pw_carry] == CR_NIGHTSMODE)
	{
		if (player->mo->momx || player->mo->momy || player->mo->momz)
		{
			if (player->mo->state != &states[S_PLAY_NIGHTS_PULL])
				P_SetMobjState(player->mo, S_PLAY_NIGHTS_PULL);
		}
		else if (player->mo->state != &states[S_PLAY_NIGHTS_ATTACK])
		{
			S_StartSound(player->mo, sfx_spin);
			P_SetMobjState(player->mo, S_PLAY_NIGHTS_ATTACK);
		}
	}
	else
	{
		if (!(player->pflags & PF_JUMPED) && !(player->pflags & PF_SPINNING))
			player->pflags |= PF_JUMPED;
		if (player->panim != PA_ROLL)
			P_SetMobjState(player->mo, S_PLAY_ROLL);
	}

	if (!(player->charflags & SF_NONIGHTSROTATION))
	{
		if ((player->mo->state == &states[S_PLAY_NIGHTS_PULL])
		&& (player->mo->sprite2 == SPR2_NPUL))
			player->mo->spriteroll -= ANG30;
		else
			player->mo->spriteroll = 0;
	}

	if (G_IsSpecialStage(gamemap))
	{  
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && (&players[i] != player) && players[i].spheres > 0)
			{
				player->spheres += players[i].spheres;
				players[i].spheres = 0;
			}
	}

	if (player->capsule->extravalue2 <= 0 && player->capsule->health > 0)
		P_RunNightsCapsuleTouchExecutors(player->mo, true, player->spheres >= player->capsule->health);  


	if (player->mo->x == player->capsule->x
		&& player->mo->y == player->capsule->y
		&& player->mo->z == player->capsule->z+(player->capsule->height/3))
	{
		if (player->capsule->lastlook < 0)
		{





			spherecount = min(player->spheres, player->capsule->health);
			totalduration = min(40 + spherecount, 60);

			popduration = player->capsule->extravalue1 = max(totalduration - tictimer, 1);
			deductinterval = player->capsule->cvmem = max(FixedFloor(FixedDiv(popduration, spherecount))/FRACUNIT, 1);
			deductquantity = player->capsule->cusval = max(FixedRound(FixedDiv(spherecount, popduration))/FRACUNIT, 1);
			sphereresult = player->capsule->movecount = player->capsule->health - spherecount;
			firstpoptic = player->capsule->lastlook = tictimer;
		}
		else
		{
			popduration = player->capsule->extravalue1;
			deductinterval = player->capsule->cvmem;
			deductquantity = player->capsule->cusval;
			sphereresult = player->capsule->movecount;
			firstpoptic = player->capsule->lastlook;
		}

		if (tictimer - firstpoptic < popduration)
		{
			if (!((tictimer - firstpoptic) % deductinterval))
			{

				if (player->capsule->health <= sphereresult && player->spheres > 0 && player->capsule->health > 0)
					sphereresult = max(sphereresult - player->spheres, 0);

				if (player->capsule->health > sphereresult && player->spheres > 0)
				{



					deductquantity = min(deductquantity, player->capsule->health - sphereresult);

					player->spheres -= deductquantity;
					player->capsule->health -= deductquantity;

					if (player->spheres < 0)  
						player->spheres = 0;



				}
			}


			if (!((tictimer - firstpoptic) % 2))
			{
				mobj_t *explodemo = P_SpawnMobj(player->capsule->x + ((P_SignedRandom()/2)<<FRACBITS),
					player->capsule->y + ((P_SignedRandom()/2)<<FRACBITS),
					player->capsule->z + (player->capsule->height/2) + ((P_SignedRandom()/2)<<FRACBITS),
					MT_SONIC3KBOSSEXPLODE);
				if (!P_MobjWasRemoved(explodemo))
					S_StartSound(explodemo,sfx_s3kb4);
			}
		}
		else
		{
			if (player->spheres != 0 && player->capsule->health > 0)
			{
				if (player->spheres < player->capsule->health)
				{
					player->capsule->health -= player->spheres;
					player->spheres = 0;
				}
				else
				{
					player->spheres -= player->capsule->health;
					player->capsule->health = 0;
				}
			}

			if (player->capsule->health <= 0)
			{
				player->capsule->flags &= ~MF_NOGRAVITY;
				player->capsule->momz = 5*FRACUNIT;
				player->capsule->reactiontime = 0;
				tictimer = -1;

				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && !player->exiting && players[i].mare == player->mare)
					{
						players[i].bonustime = true;
						players[i].texttimer = 4*TICRATE;
						players[i].textvar = NTV_BONUSTIMESTART;  
						players[i].finishedtime = players[i].nightstime;
						if (!G_IsSpecialStage(gamemap))
							P_AddPlayerScore(&players[i], (players[i].finishedtime/TICRATE) * 100);
						P_FlashPal(&players[i], PAL_WHITE, 8);
					}

				if (G_IsSpecialStage(gamemap))
				{
					tic_t lowest_time;

					                                  
      
                                                                                               
                

                                                                                                                            
                                                
                                                                    
        

					if (player->powers[pw_carry] == CR_NIGHTSMODE)
					{

						UINT8 em = P_GetNextEmerald();

						mobj_t *emmo = P_SpawnMobjFromMobj(player->mo, 0, 0, player->mo->height, MT_GOTEMERALD);
						if (!P_MobjWasRemoved(emmo))
						{
							emmo->health = em;  
							P_SetTarget(&emmo->target, player->mo);
							P_SetMobjState(emmo, mobjinfo[MT_GOTEMERALD].meleestate + em);
							P_SetTarget(&player->mo->tracer, emmo);
						}
					}




					lowest_time = player->finishedtime;
					for (i = 0; i < MAXPLAYERS; i++)
						if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE)
							if (players[i].finishedtime < lowest_time)
								lowest_time = players[i].finishedtime;
					P_AddPlayerScore(player, (lowest_time/TICRATE) * 100);
				}
				else
				{
					                          
      
                                                                                                  
                                             
                                         
                                                       
        
					mobj_t *idya = P_SpawnMobjFromMobj(player->mo, 0, 0, player->mo->height, MT_GOTEMERALD);
					if (!P_MobjWasRemoved(idya))
					{
						idya->extravalue2 = player->mare/5;
						idya->health = player->mare + 1;  
						P_SetTarget(&idya->target, player->mo);
						P_SetMobjState(idya, mobjinfo[MT_GOTEMERALD].missilestate + ((player->mare + 1) % 5));

						if (player->mo->tracer)
						{
							P_SetTarget(&idya->hnext, player->mo->tracer);
							idya->extravalue1 = (angle_t)(player->mo->tracer->extravalue1 - 72*ANG1);
							if (idya->extravalue1 > player->mo->tracer->extravalue1)
								idya->extravalue1 -= (72*ANG1)/idya->extravalue1;
						}
						P_SetTarget(&player->mo->tracer, idya);
					}
				}
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].mare == player->mare)
						P_SetTarget(&players[i].capsule, NULL);  
				S_StartScreamSound(player->mo, sfx_ngdone);
				P_SwitchSpheresBonusMode(true);
				P_RunNightsCapsuleTouchExecutors(player->mo, false, true);  
			}
			else
			{
				S_StartScreamSound(player->mo, sfx_lose);
				player->texttimer = 4*TICRATE;
				player->textvar = NTV_GETMORESPHERES;  
				player->capsule->reactiontime = 0;
				player->capsule->extravalue1 = player->capsule->cvmem =\
				 player->capsule->cusval = player->capsule->movecount =\
				 player->capsule->lastlook = player->capsule->extravalue2 = -1;
				P_RunNightsCapsuleTouchExecutors(player->mo, false, false);  
			}
		}
	}
	else if (player->capsule->lastlook > -1)


		player->capsule->lastlook = player->capsule->extravalue2 = -1;
}






static void P_MoveNiGHTSToDrone(player_t *player)
{
	boolean flip, topaligned, middlealigned, bottomoffsetted;
	fixed_t droneboxmandiff, zofs;

	if (!player->drone)
		return;

	flip = player->drone->flags2 & MF2_OBJECTFLIP;
	topaligned = (player->drone->flags & MF_SLIDEME) && !(player->drone->flags & MF_GRENADEBOUNCE);
	middlealigned = (player->drone->flags & MF_GRENADEBOUNCE) && !(player->drone->flags & MF_SLIDEME);
	bottomoffsetted = !(player->drone->flags & MF_SLIDEME) && !(player->drone->flags & MF_GRENADEBOUNCE);
	droneboxmandiff = max(player->drone->height - player->mo->height, 0);

	if (!flip)
	{
		if (topaligned)
			zofs = droneboxmandiff;
		else if (middlealigned)
			zofs = droneboxmandiff / 2;
		else if (bottomoffsetted)
			zofs = FixedMul(24*FRACUNIT, player->drone->scale);
		else
			zofs = 0;
	}
	else
	{
		if (topaligned)
			zofs = 0;
		else if (middlealigned)
			zofs = droneboxmandiff / 2;
		else if (bottomoffsetted)
			zofs = droneboxmandiff - FixedMul(24*FRACUNIT, player->drone->scale);
		else
			zofs = droneboxmandiff;
	}

	player->mo->momx = player->mo->momy = player->mo->momz = 0;
	P_MoveOrigin(player->mo, player->drone->x, player->drone->y, player->drone->z + zofs);
	P_SetTarget(&player->drone, NULL);
}






static void P_NiGHTSMovement(player_t *player)
{
	fixed_t drillamt = 0;
	boolean still = false, moved = false, backwardaxis = false, firstdrill;
	INT16 newangle = 0;
	fixed_t xspeed, yspeed;
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *closestaxis = NULL;
	fixed_t newx, newy, radius;
	angle_t movingangle;
	ticcmd_t *cmd = &player->cmd;
	INT32 thrustfactor;
	INT32 i;
	statenum_t flystate;
	UINT16 visangle;
	angle_t rollangle = 0;

	player->pflags &= ~PF_DRILLING;

	firstdrill = false;

	if (player->drillmeter > 96*20)
		player->drillmeter = 96*20;

	if (player->drilldelay)
		player->drilldelay--;

	if (!(cmd->buttons & BT_JUMP))
	{

		if (player->drillmeter <= 0)
			player->drillmeter = TICRATE/10;
	}

	if (!(player->powers[pw_carry] == CR_NIGHTSMODE))
	{
		P_DeNightserizePlayer(player);
		return;
	}

	if (G_IsSpecialStage(gamemap))
	{
		boolean capsule = false;

		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i]  
			&& (players[i].capsule && players[i].capsule->reactiontime))
				capsule = true;
		if (!capsule
		&& !(player->mo->state >= &states[S_PLAY_NIGHTS_TRANS1]
			&& player->mo->state <= &states[S_PLAY_NIGHTS_TRANS6])
		&& !player->exiting)
			player->nightstime--;
	}
	else if (!(gametyperules & GTR_RACE)
	&& !(player->mo->state >= &states[S_PLAY_NIGHTS_TRANS1]
			&& player->mo->state <= &states[S_PLAY_NIGHTS_TRANS6])
	&& !(player->capsule && player->capsule->reactiontime)
	&& !player->exiting)
		player->nightstime--;

	if (!player->nightstime)
	{
		P_DeNightserizePlayer(player);
		S_StartScreamSound(player->mo, sfx_s3k66);
		return;
	}
	else if (P_IsLocalPlayer(player) && player->nightstime == 10*TICRATE)
	{
		if (mapheaderinfo[gamemap-1]->levelflags & LF_MIXNIGHTSCOUNTDOWN)
		{
			S_FadeMusic(0, 10*MUSICRATE);
			S_StartSound(NULL, sfx_timeup);  
		}
		else
			P_PlayJingle(player, ((maptol & TOL_NIGHTS) && !G_IsSpecialStage(gamemap)) ? JT_NIGHTSTIMEOUT : JT_SSTIMEOUT);
	}

	if (player->mo->z < player->mo->floorz)
		player->mo->z = player->mo->floorz;

	if (player->mo->z+player->mo->height > player->mo->ceilingz)
		player->mo->z = player->mo->ceilingz - player->mo->height;

	newx = P_ReturnThrustX(player->mo, player->mo->angle, 3*FRACUNIT)+player->mo->x;
	newy = P_ReturnThrustY(player->mo, player->mo->angle, 3*FRACUNIT)+player->mo->y;

	if (!player->mo->target)
	{
		fixed_t dist1, dist2 = 0;



		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->removing)
				continue;

			mo2 = (mobj_t *)th;

			if (mo2->type != MT_AXIS)
				continue;
			if (mo2->threshold != player->mare)
				continue;

			if (closestaxis == NULL)
			{
				closestaxis = mo2;
				dist2 = R_PointToDist2(newx, newy, mo2->x, mo2->y) - mo2->radius;
			}
			else
			{
				dist1 = R_PointToDist2(newx, newy, mo2->x, mo2->y) - mo2->radius;

				if (dist1 < dist2)
				{
					closestaxis = mo2;
					dist2 = dist1;
				}
			}
		}
		P_SetTarget(&player->mo->target, closestaxis);
	}

	if (!player->mo->target)  
	{
		CONS_Debug(DBG_NIGHTS, "No axis points found!\n");
		return;
	}



	if (player->mo->target->flags2 & MF2_AMBUSH)
		backwardaxis = true;

	player->angle_pos = R_PointToAngle2(player->mo->target->x, player->mo->target->y, player->mo->x, player->mo->y);

	player->old_angle_pos = player->angle_pos;

	radius = player->mo->target->radius;

	player->mo->flags |= MF_NOGRAVITY;

	if (player->mo->eflags & MFE_VERTICALFLIP)
		cmd->forwardmove = (SINT8)(-cmd->forwardmove);

	if (!(player->pflags & PF_TRANSFERTOCLOSEST))
	{
		fixed_t realdist = R_PointToDist2(player->mo->x, player->mo->y, player->mo->target->x, player->mo->target->y);

		if (realdist>>FRACBITS != radius>>FRACBITS)
		{
			CONS_Debug(DBG_NIGHTS, "Aligning player with axis\n");
			P_UnsetThingPosition(player->mo);
			if (realdist == 0)  
			{
				const angle_t fa = player->old_angle_pos>>ANGLETOFINESHIFT;
				player->mo->x = player->mo->target->x + FixedMul(FINECOSINE(fa), radius);
				player->mo->y = player->mo->target->y + FixedMul(FINESINE(fa), radius);
			}
			else
			{
				player->mo->x = player->mo->target->x + FixedMul(FixedDiv(player->mo->x - player->mo->target->x, realdist), radius);
				player->mo->y = player->mo->target->y + FixedMul(FixedDiv(player->mo->y - player->mo->target->y, realdist), radius);
			}
			P_SetThingPosition(player->mo);
		}
	}


	if (player->powers[pw_flashing] > (2*flashingtics)/3)
	{
		{
			const angle_t fa = (FixedAngle(player->flyangle*FRACUNIT)>>ANGLETOFINESHIFT) & FINEMASK;
			const fixed_t speed = FixedDiv(player->speed*FRACUNIT,50*FRACUNIT);

			xspeed = FixedMul(FINECOSINE(fa),speed);
			yspeed = FixedMul(FINESINE(fa),speed);
		}

		if (!(player->pflags & PF_TRANSFERTOCLOSEST))
		{
			xspeed = FixedMul(xspeed, FixedDiv(1024*FRACUNIT, player->mo->target->radius));

			if (backwardaxis)
				xspeed *= -1;

			player->angle_pos += FixedAngleC(FixedDiv(xspeed,5*FRACUNIT),40*FRACUNIT);
		}

		player->mo->momz = 0;

		P_NightsTransferPoints(player, xspeed, radius);
		return;
	}

	if (player->mo->state >= &states[S_PLAY_NIGHTS_TRANS1]
		&& player->mo->state <= &states[S_PLAY_NIGHTS_TRANS6])
	{
		player->mo->momx = player->mo->momy = player->mo->momz = 0;
		player->mo->spriteroll = 0;
		return;
	}

	if (player->exiting > 0)  
	{
		player->mo->momx = player->mo->momy = 0;

		if (!(gametyperules & GTR_RACE))
			P_SetObjectMomZ(player->mo, FRACUNIT/2, (P_MobjFlip(player->mo)*player->mo->momz >= 0));
		else
			player->mo->momz = 0;

#if 0 
		if (!(player->charflags & SF_NONIGHTSROTATION) && player->mo->momz)
		{
			if (player->mo->state != &states[S_PLAY_NIGHTS_DRILL])
				P_SetMobjState(player->mo, S_PLAY_NIGHTS_DRILL);
			player->mo->spriteroll = ANGLE_90;
		}
		else
#endif
		{
			if (player->mo->state != &states[S_PLAY_NIGHTS_FLOAT])
				P_SetMobjState(player->mo, S_PLAY_NIGHTS_FLOAT);
			player->drawangle += ANGLE_22h;
		}

		player->mo->flags |= MF_NOCLIPHEIGHT;
		return;
	}


	if (leveltime & 1)
	{
		mobj_t *firstmobj;
		mobj_t *secondmobj;
		fixed_t spawndist = FixedMul(16*FRACUNIT, player->mo->scale);
		fixed_t z = player->mo->z + player->mo->height/2;

		if (player->mo->eflags & MFE_VERTICALFLIP)
			z -= FixedMul(mobjinfo[MT_NIGHTSPARKLE].height, player->mo->scale);

		firstmobj = P_SpawnMobj(player->mo->x + P_ReturnThrustX(player->mo, player->mo->angle+ANGLE_90, spawndist), player->mo->y + P_ReturnThrustY(player->mo, player->mo->angle+ANGLE_90, spawndist), z, MT_NIGHTSPARKLE);
		if (!P_MobjWasRemoved(firstmobj))
		{
			P_SetTarget(&firstmobj->target, player->mo);
			P_SetScale(firstmobj, player->mo->scale, true);

			if (player->powers[pw_nights_superloop])
				P_SetMobjState(firstmobj, mobjinfo[MT_NIGHTSPARKLE].seestate);
		}
		secondmobj = P_SpawnMobj(player->mo->x + P_ReturnThrustX(player->mo, player->mo->angle-ANGLE_90, spawndist), player->mo->y + P_ReturnThrustY(player->mo, player->mo->angle-ANGLE_90, spawndist), z, MT_NIGHTSPARKLE);
		if (!P_MobjWasRemoved(secondmobj))
		{
			P_SetTarget(&secondmobj->target, player->mo);
			P_SetScale(secondmobj, player->mo->scale, true);

			if (player->powers[pw_nights_superloop])
				P_SetMobjState(secondmobj, mobjinfo[MT_NIGHTSPARKLE].seestate);
		}
	}



	{
		mobj_t *helpermobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height/2, MT_NIGHTSLOOPHELPER);
		if (!P_MobjWasRemoved(helpermobj))
		{
			helpermobj->fuse = player->mo->fuse = leveltime;
			P_SetTarget(&helpermobj->target, player->mo);
			P_SetScale(helpermobj, player->mo->scale, false);
		}
	}

	if (player->bumpertime)
	{
		player->pflags |= (PF_STARTJUMP|PF_DRILLING);
		newangle = (INT16)player->flyangle;
	}
	else if (cmd->buttons & BT_JUMP && player->drillmeter && player->drilldelay == 0)
	{
		if (!(player->pflags & PF_STARTJUMP))
			firstdrill = true;

		player->pflags |= (PF_STARTJUMP|PF_DRILLING);
	}
	else
	{
		player->pflags &= ~PF_STARTJUMP;

		if (cmd->sidemove != 0)
			moved = true;

		if (player->drillmeter & 1)
			player->drillmeter++;  
	}

	if (cmd->forwardmove != 0)
		moved = true;

	if (!player->bumpertime)
	{
		if (moved)
		{
			if (player->pflags & PF_DRILLING)
				drillamt += 100*FRACUNIT;
			else
			{
				const fixed_t fforward = abs(cmd->forwardmove)*FRACUNIT;
				const fixed_t fside = abs(cmd->sidemove)*FRACUNIT;
				const fixed_t dist = FixedHypot(fforward, fside);

				drillamt += dist > 50*FRACUNIT ? 50*FRACUNIT : dist;


				drillamt = FixedMul(drillamt, 5*FRACUNIT/2);
			}
			if ((player->pflags & PF_DRILLING && player->speed < MAXDRILLSPEED) || player->speed < MAXNORMALSPEED)
				player->speed += FixedInt(drillamt);
		}

		if (player->pflags & PF_DRILLING)
		{
			if (player->speed < MAXDRILLSPEED)
				player->speed += 150;

			if (--player->drillmeter == 0)
				player->drilldelay = TICRATE*2;
		}

		if (player->speed < 0)
			player->speed = 0;

		if (!cmd->forwardmove)
		{
			if (cmd->sidemove > 0)
				newangle = 0;
			else if (cmd->sidemove < 0)
				newangle = 180;
		}
		else if (!cmd->sidemove)
		{
			if (cmd->forwardmove > 0)
				newangle = 90;
			else if (cmd->forwardmove < 0)
				newangle = 270;
		}
		else  
		{
			newangle = (INT16)FixedInt(AngleFixed(R_PointToAngle2(0,0, cmd->sidemove*FRACUNIT, cmd->forwardmove*FRACUNIT)));

			if (cmd->forwardmove == -36 && cmd->sidemove == 35 && !(demoplayback && demoversion < 0x0011))
				newangle = 315;  
		}

		newangle -= player->viewrollangle / ANG1;

		if (newangle < 0 && moved)
			newangle = (INT16)(360+newangle);

		if (moved && player->flyangle != newangle)
		{

			INT32 anglediff = ((newangle - player->flyangle) + 360) % 360;  


			if (player->pflags & PF_DRILLING)
				thrustfactor = 2;
			else
			{
				thrustfactor = 8;
				if (player->speed > 12000)  
					player->speed -= 60;
			}


			if (anglediff <= thrustfactor || anglediff >= (360-thrustfactor))
				player->flyangle = newangle;
			else if (anglediff <= 180)
				player->flyangle += thrustfactor;
			else
				player->flyangle -= thrustfactor;

			player->flyangle = (player->flyangle + 360) % 360;  
		}
	}

	if (player->speed == 0 && cmd->forwardmove == 0 && (cmd->sidemove == 0 || (demoplayback && demoversion < 0x0011)))
		still = true;


	if (!player->bumpertime
	 && ((cmd->buttons & (BT_CAMLEFT|BT_CAMRIGHT)) == (BT_CAMLEFT|BT_CAMRIGHT)
	  || (cmd->buttons & BT_SPIN)))
	{
		if (!(player->pflags & PF_STARTDASH))
			S_StartSound(player->mo, sfx_ngskid);



		if (player->speed > 8000)
			player->speed -= 2000;
		else if (player->speed > 1000)
			player->speed -= (player->speed/4);
		else {
			player->speed -= 60;
			if (player->speed < 0)
			{
				player->speed = 0;
				still = true;
			}
		}

		player->pflags |= PF_STARTDASH;
	}
	else
		player->pflags &= ~PF_STARTDASH;

	{
		const angle_t fa = (FixedAngle(player->flyangle*FRACUNIT)>>ANGLETOFINESHIFT) & FINEMASK;
		const fixed_t speed = FixedDiv(player->speed*FRACUNIT,50*FRACUNIT);
		xspeed = FixedMul(FINECOSINE(fa),speed);
		yspeed = FixedMul(FINESINE(fa),speed);
	}

	if (!(player->pflags & PF_TRANSFERTOCLOSEST))
	{
		xspeed = FixedMul(xspeed, FixedDiv(1024*FRACUNIT, player->mo->target->radius));

		if (backwardaxis)
			xspeed *= -1;

		player->angle_pos += FixedAngleC(FixedDiv(xspeed,5*FRACUNIT),40*FRACUNIT);
	}

	P_NightsTransferPoints(player, xspeed, radius);

	if (still)
		player->mo->momz = -FRACUNIT;
	else
		player->mo->momz = yspeed/11;

	if (player->mo->momz > 20*FRACUNIT)
		player->mo->momz = 20*FRACUNIT;
	else if (player->mo->momz < -20*FRACUNIT)
		player->mo->momz = -20*FRACUNIT;


	if (((!(player->mo->eflags & MFE_VERTICALFLIP)
		&& player->mo->z + P_GetPlayerHeight(player) >= player->mo->watertop && player->mo->z <= player->mo->watertop)
	|| (player->mo->eflags & MFE_VERTICALFLIP
		&& player->mo->z + player->mo->height - P_GetPlayerHeight(player) <= player->mo->waterbottom && player->mo->z + player->mo->height >= player->mo->waterbottom))
	&& player->speed > 9000 && leveltime % (TICRATE/7) == 0 && !player->spectator)
	{
		mobjtype_t splishtype = (player->mo->eflags & MFE_TOUCHLAVA) ? MT_LAVASPLISH : MT_SPLISH;
		mobj_t *water = P_SpawnMobj(player->mo->x, player->mo->y,
			((player->mo->eflags & MFE_VERTICALFLIP) ? player->mo->waterbottom - FixedMul(mobjinfo[splishtype].height, player->mo->scale) : player->mo->watertop), splishtype);
		if (!P_MobjWasRemoved(water))
		{
			if (player->mo->eflags & MFE_GOOWATER)
				S_StartSound(water, sfx_ghit);
			else if (player->mo->eflags & MFE_TOUCHLAVA)
				S_StartSound(water, sfx_splash);
			else
				S_StartSound(water, sfx_wslap);
			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				water->flags2 |= MF2_OBJECTFLIP;
				water->eflags |= MFE_VERTICALFLIP;
			}
			P_SetScale(water, player->mo->scale, true);
		}
	}

	if (player->mo->momx || player->mo->momy)
	{
		player->mo->angle = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
		player->drawangle = player->mo->angle;
	}

	if (still)
	{
		player->anotherflyangle = 0;
		movingangle = 0;
	}
	else
	{
		INT32 neg = 1;



		if (backwardaxis && (player->old_angle_pos > ANG350 && player->angle_pos < ANG10))
			neg = -1;
		else if (backwardaxis ^ (player->old_angle_pos < ANG10 && player->angle_pos > ANG350))
			neg = -1;
		else if (player->angle_pos > player->old_angle_pos)
			neg = -1;

		movingangle = R_PointToAngle2(0, 0, neg*R_PointToDist2(player->mo->momx, player->mo->momy, 0, 0), player->mo->momz);
		player->anotherflyangle = (movingangle >> ANGLETOFINESHIFT) * 360/FINEANGLES;
	}




	if (still)
		flystate = (P_IsObjectOnGround(player->mo)) ? S_PLAY_NIGHTS_STAND : S_PLAY_NIGHTS_FLOAT;
	else
	{
		flystate = (player->pflags & PF_DRILLING) ? S_PLAY_NIGHTS_DRILL : S_PLAY_NIGHTS_FLY;
		if (player->charflags & SF_NONIGHTSROTATION)
		{
#if 0
			visangle = ((player->anotherflyangle + 7) % 360)/15;
			if (visangle > 18)  
				visangle = 30 - visangle;
			else if (visangle > 12)  
				visangle -= 6;
			else if (visangle > 6)  
				visangle = 12 - visangle;

			if (player->mo->eflags & MFE_VERTICALFLIP && visangle)  
			{
				if (visangle > 6)
					visangle -= 6;  
				else
					visangle += 6;  
			}

			flystate += (visangle*2);  
#endif
		}
		else
		{
			angle_t a = R_PointToAngle(player->mo->x, player->mo->y) - player->mo->angle;
			visangle = (player->flyangle % 360);

			if (player->flyangle >= 90 && player->flyangle <= 270)
			{
				if (player->flyangle == 270 && (a < ANGLE_180))
					;
				else if (player->flyangle == 90 && (a < ANGLE_180))
					;
				else
					visangle += 180;
			}

			if (player->mo->eflags & MFE_VERTICALFLIP)  
				visangle *= -1;

			rollangle = FixedAngle(visangle<<FRACBITS);
		}
	}

	if (player->mo->state != &states[flystate])
		P_SetMobjState(player->mo, flystate);

	if (player->charflags & SF_NONIGHTSROTATION)
		player->mo->spriteroll = 0;
	else
		player->mo->spriteroll = rollangle;

	P_SetPlayerAngle(player, player->mo->angle);


	if ((player->mo->ceilingz - player->mo->floorz < player->mo->height)
		&& !(player->mo->flags & MF_NOCLIP))
	{

		if (player->powers[pw_flashing] == 1)
			player->powers[pw_flashing] = 3;
		else
			P_DamageMobj(player->mo, NULL, NULL, 1, 0);
	}

	if (movingangle >= ANGLE_90 && movingangle <= ANGLE_180)
		movingangle = movingangle - ANGLE_180;
	else if (movingangle >= ANGLE_180 && movingangle <= ANGLE_270)
		movingangle = movingangle - ANGLE_180;
	else if (movingangle >= ANGLE_270)
		movingangle = InvAngle(movingangle);

	if (player == &players[consoleplayer])
		localaiming = movingangle;
	else if (player == &players[secondarydisplayplayer])
		localaiming2 = movingangle;

	if ((player->pflags & PF_DRILLING) && !player->bumpertime)
	{
		if (firstdrill)
		{
			S_StartSound(player->mo, sfx_drill1);
			player->drilltimer = 32;
		}
		else if (player->drilltimer == 32)
		{

			player->drilltimer = 31;
			player->drillmeter -= TICRATE/2;
			if (player->drillmeter <= 0)
				player->drillmeter = TICRATE/10;
		}
		else if (--player->drilltimer == 11)

			player->drillmeter += TICRATE/2;
		else if (player->drilltimer <= 0)
		{
			player->drilltimer = 10;
			S_StartSound(player->mo, sfx_drill2);
		}
	}



}


#if 0
static void P_PlayerDropWeapon(player_t *player)
{
	mobj_t *mo = NULL;

	if (player->powers[pw_homingring])
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+(60*FRACUNIT), MT_HOMINGRING);
		player->powers[pw_homingring] = 0;
	}
	else if (player->powers[pw_railring])
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+(60*FRACUNIT), MT_RAILRING);
		player->powers[pw_railring] = 0;
	}
	else if (player->powers[pw_automaticring])
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+(60*FRACUNIT), MT_AUTOMATICRING);
		player->powers[pw_automaticring] = 0;
	}
	else if (player->powers[pw_explosionring])
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+(60*FRACUNIT), MT_EXPLOSIONRING);
		player->powers[pw_explosionring] = 0;
	}
	else if (player->powers[pw_scatterring])
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+(60*FRACUNIT), MT_SCATTERRING);
		player->powers[pw_scatterring] = 0;
	}
	else if (player->powers[pw_grenadering])
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+(60*FRACUNIT), MT_GRENADERING);
		player->powers[pw_grenadering] = 0;
	}

	if (mo)
	{
		player->rings--;
		P_InstaThrust(mo, player->mo->angle-ANGLE_180, 8*FRACUNIT);
		P_SetObjectMomZ(mo, 4*FRACUNIT, false);
		mo->flags2 |= MF2_DONTRESPAWN;
		mo->flags &= ~MF_NOGRAVITY;
		mo->flags &= ~MF_NOCLIPHEIGHT;
		mo->fuse = 12*TICRATE;
	}
}
#endif

void P_BlackOw(player_t *player)
{
	INT32 i;
	S_StartSound (player->mo, sfx_bkpoof);  

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && P_AproxDistance(player->mo->x - players[i].mo->x,
			player->mo->y - players[i].mo->y) < 1536*FRACUNIT)
			P_FlashPal(&players[i], PAL_NUKE, 10);

	P_NukeEnemies(player->mo, player->mo, 1536*FRACUNIT);  
	player->powers[pw_shield] = player->powers[pw_shield] & SH_STACK;
}

void P_ElementalFire(player_t *player, boolean cropcircle)
{
	fixed_t newx;
	fixed_t newy;
	fixed_t ground;
	mobj_t *flame;
	angle_t travelangle;
	INT32 i;

	I_Assert(player != NULL);
	I_Assert(player->mo != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (player->mo->eflags & MFE_VERTICALFLIP)
		ground = player->mo->ceilingz - FixedMul(mobjinfo[MT_SPINFIRE].height, player->mo->scale);
	else
		ground = player->mo->floorz;

	if (cropcircle)
	{
		ground += P_MobjFlip(player->mo);
#define numangles 8
#define limitangle (180/numangles)
		travelangle = player->mo->angle + P_RandomRange(-limitangle, limitangle)*ANG1;
		for (i = 0; i < numangles; i++)
		{
			flame = P_SpawnMobj(player->mo->x, player->mo->y, ground, MT_SPINFIRE);
			if (P_MobjWasRemoved(flame))
				continue;
			flame->flags &= ~MF_NOGRAVITY;
			P_SetTarget(&flame->target, player->mo);
			flame->angle = travelangle + i*(ANGLE_MAX/numangles);
			flame->fuse = TICRATE*7;  
			P_SetScale(flame, player->mo->scale, true);
			if (!(player->mo->flags2 & MF2_OBJECTFLIP) != !(player->powers[pw_gravityboots]))  
				flame->flags2 |= MF2_OBJECTFLIP;
			flame->eflags = (flame->eflags & ~MFE_VERTICALFLIP)|(player->mo->eflags & MFE_VERTICALFLIP);
			P_InstaThrust(flame, flame->angle, FixedMul(3*FRACUNIT, flame->scale));
			P_SetObjectMomZ(flame, 3*FRACUNIT, false);
			if (!(gametyperules & GTR_FRIENDLY))
			{
				P_SetMobjState(flame, S_TEAM_SPINFIRE1);
				flame->color = player->mo->color;
			}
		}
#undef limitangle
#undef numangles
	}
	else
	{
		travelangle = R_PointToAngle2(0, 0, player->rmomx, player->rmomy);
		for (i = 0; i < 2; i++)
		{

			newx = player->mo->x + P_ReturnThrustX(player->mo, (travelangle + ((i&1) ? -1 : 1)*ANGLE_135), FixedMul(24*FRACUNIT, player->mo->scale));
			newy = player->mo->y + P_ReturnThrustY(player->mo, (travelangle + ((i&1) ? -1 : 1)*ANGLE_135), FixedMul(24*FRACUNIT, player->mo->scale));

			if (player->mo->standingslope)
			{
				ground = P_GetSlopeZAt(player->mo->standingslope, newx, newy);
				if (player->mo->eflags & MFE_VERTICALFLIP)
					ground -= FixedMul(mobjinfo[MT_SPINFIRE].height, player->mo->scale);
			}

			flame = P_SpawnMobj(newx, newy, ground, MT_SPINFIRE);
			if (P_MobjWasRemoved(flame))
				continue;
			P_SetTarget(&flame->target, player->mo);
			flame->angle = travelangle;
			flame->fuse = TICRATE*6;
			P_SetScale(flame, player->mo->scale, true);
			if (!(player->mo->flags2 & MF2_OBJECTFLIP) != !(player->powers[pw_gravityboots]))  
				flame->flags2 |= MF2_OBJECTFLIP;
			flame->eflags = (flame->eflags & ~MFE_VERTICALFLIP)|(player->mo->eflags & MFE_VERTICALFLIP);
			if (!(gametyperules & GTR_FRIENDLY))
			{
				P_SetMobjState(flame, S_TEAM_SPINFIRE1);
				flame->color = player->mo->color;
			}

			flame->momx = 8;  
			P_XYMovement(flame);
			if (P_MobjWasRemoved(flame))
				continue;

			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				if (flame->z + flame->height < flame->ceilingz)
					P_RemoveMobj(flame);
			}
			else if (flame->z > flame->floorz)
				P_RemoveMobj(flame);
		}
	}
}






void P_SpawnSkidDust(player_t *player, fixed_t radius, boolean sound)
{
	mobj_t *mo = player->mo;
	mobj_t *particle;

	particle = P_SpawnMobjFromMobj(mo, 0, 0, 0, MT_SPINDUST);
	if (P_MobjWasRemoved(particle))
		return;
	if (radius >>= FRACBITS)
	{
		P_UnsetThingPosition(particle);
		particle->x += P_RandomRange(-radius, radius) << FRACBITS;
		particle->y += P_RandomRange(-radius, radius) << FRACBITS;
		P_SetThingPosition(particle);
	}
	particle->tics = 10;

	P_SetScale(particle, (2*mo->scale)/3, true);
	P_SetObjectMomZ(particle, FRACUNIT, false);

	if (mo->eflags & (MFE_TOUCHWATER|MFE_UNDERWATER))  
		P_SetMobjState(particle, S_SPINDUST_BUBBLE1);
	else if (player->powers[pw_shield] == SH_ELEMENTAL)
		P_SetMobjState(particle, S_SPINDUST_FIRE1);

	if (sound)
		S_StartSound(mo, sfx_s3k7e);  
}

static void P_SkidStuff(player_t *player)
{
	fixed_t pmx = player->rmomx + player->cmomx;
	fixed_t pmy = player->rmomy + player->cmomy;


	if (player->pflags & PF_GLIDING && player->skidtime)
	{

		if (!onground)
		{
			player->skidtime = 0;
			player->pflags &= ~(PF_GLIDING|PF_JUMPED|PF_NOJUMPDAMAGE);
			player->pflags |= PF_THOKKED;
			P_SetMobjState(player->mo, S_PLAY_FALL);
		}

		else if (player->glidetime > 15 || !(player->cmd.buttons & BT_JUMP))
		{
			P_ResetPlayer(player);
			P_SetMobjState(player->mo, S_PLAY_GLIDE_LANDING);
			player->pflags |= PF_STASIS;
			if (player->speed > FixedMul(player->runspeed, player->mo->scale))
				player->skidtime += player->mo->tics;
			player->mo->momx = ((player->mo->momx - player->cmomx)/2) + player->cmomx;
			player->mo->momy = ((player->mo->momy - player->cmomy)/2) + player->cmomy;
		}

		else if (player->skidtime == 1)
			player->skidtime = 3*TICRATE+1;

		else if (!(player->skidtime % 3) && !(player->charflags & SF_NOSKID))
		{
			P_SpawnSkidDust(player, player->mo->radius, true);
		}
	}

	else if (onground && !(player->mo->eflags & MFE_GOOWATER) && !(player->pflags & (PF_JUMPED|PF_SPINNING|PF_SLIDING)) && !(player->charflags & SF_NOSKID))
	{
		if (player->skidtime)
		{

			if (!(player->skidtime % 3))
			{
				if (player->mo->state-states == S_PLAY_GLIDE_LANDING)
					P_SpawnSkidDust(player, player->mo->radius, true);
				else
					P_SpawnSkidDust(player, 0, false);
			}
		}
		else if (P_AproxDistance(pmx, pmy) >= FixedMul(player->runspeed/2, player->mo->scale)  
		&& (player->mo->momx != pmx || player->mo->momy != pmy)  
		&& P_GetPlayerControlDirection(player) == 2)  
		{  
			angle_t mang = R_PointToAngle2(0,0,pmx,pmy);  
			angle_t pang = R_PointToAngle2(pmx,pmy,player->mo->momx,player->mo->momy);  
			angle_t dang = mang - pang;  

			if (dang > ANGLE_180)  
				dang = InvAngle(dang);


			if (dang > ANGLE_157h)
			{
				if (player->mo->state-states != S_PLAY_SKID)
					P_SetMobjState(player->mo, S_PLAY_SKID);
				player->mo->tics = player->skidtime = (player->mo->movefactor == FRACUNIT) ? TICRATE/2 : (FixedDiv(35<<(FRACBITS-1), FixedSqrt(player->mo->movefactor)))>>FRACBITS;
				S_StartSound(player->mo, sfx_skid);
			}
		}
	}
	else {
		if (player->skidtime) {
			player->skidtime = 0;
			S_StopSound(player->mo);
		}
	}
}



void P_MovePlayer(player_t *player)
{
	ticcmd_t *cmd;
	INT32 i;

	fixed_t runspd;

	if (player->mo->state >= &states[S_PLAY_SUPER_TRANS1] && player->mo->state <= &states[S_PLAY_SUPER_TRANS6])
	{
		player->mo->momx = player->mo->momy = player->mo->momz = 0;
		return;
	}

	cmd = &player->cmd;
	runspd = FixedMul(player->runspeed, player->mo->scale);



	if (player->powers[pw_super])
		runspd = FixedMul(runspd, 5*FRACUNIT/3);


	runspd = FixedMul(runspd, player->mo->movefactor);


	if ((player->powers[pw_carry] == CR_BRAKGOOP)
	|| (player->pflags & PF_GLIDING && player->skidtime)
	|| (player->charability2 == CA2_GUNSLINGER && player->panim == PA_ABILITY2)
	|| (player->charability2 == CA2_MELEE && player->mo->state-states == S_PLAY_MELEE_LANDING))
		player->pflags |= PF_FULLSTASIS;
	else if (player->powers[pw_nocontrol])
	{
		player->pflags |= PF_STASIS;
		if (!(player->powers[pw_nocontrol] & (1<<15)))
			player->pflags |= PF_JUMPSTASIS;
	}

	if (player->charability == CA_GLIDEANDCLIMB && player->mo->state-states == S_PLAY_GLIDE_LANDING)
	{
		player->pflags |= PF_STASIS;
	}



	if (!player->spectator && G_TagGametype())
	{


		boolean forcestasis = false;


		if (leveltime < hidetime * TICRATE)
		{
			if (player->pflags & PF_TAGIT)
				forcestasis = true;
		}
		else if (gametyperules & GTR_HIDEFROZEN)
		{
			if (!(player->pflags & PF_TAGIT))
			{
				forcestasis = true;
				if (player->pflags & PF_GAMETYPEOVER)  
					player->powers[pw_flashing] = 5;
			}
		}

		if (forcestasis)
		{
			player->pflags |= PF_FULLSTASIS;

			if (player->powers[pw_underwater] <= 12*TICRATE + 1)
				P_RestoreMusic(player);
			player->powers[pw_underwater] = player->powers[pw_spacetime] = 0;
		}
	}

	if (player->spectator)
	{
		player->mo->eflags &= ~MFE_VERTICALFLIP;  
		P_SpectatorMovement(player);
		return;
	}


	else if (maptol & TOL_NIGHTS)
	{
		if (P_PlayerFullbright(player))
		{
			player->mo->color = ((skin_t *)player->mo->skin)->supercolor
				+ ((player->nightstime == player->startedtime)
					? 4
					: abs((((signed)leveltime >> 1) % 9) - 4));  
			G_GhostAddColor(GHC_SUPER);
		}

		if (!player->capsule && !player->bonustime)
		{
			thinker_t *th;
			mobj_t *mo2;

			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->removing)
					continue;

				mo2 = (mobj_t *)th;

				if (mo2->type == MT_EGGCAPSULE
					&& mo2->threshold == player->mare)
					P_SetTarget(&player->capsule, mo2);
			}
		}
		else if (player->capsule && player->capsule->reactiontime > 0 && player == &players[player->capsule->reactiontime-1])
		{
			P_DoNiGHTSCapsule(player);
			return;
		}


		if (player->drone)
		{
			P_MoveNiGHTSToDrone(player);
			return;
		}


		if (player->powers[pw_carry] == CR_NIGHTSMODE)
		{
			P_NiGHTSMovement(player);

			if (CheckForBustableBlocks)
				P_CheckBustableBlocks(player);
			if (CheckForBouncySector)
				P_CheckBouncySectors(player);
			if (CheckForQuicksand)
				P_CheckQuicksand(player);
			return;
		}
		else if (player->powers[pw_carry] == CR_NIGHTSFALL && P_IsObjectOnGround(player->mo))
		{
			if (G_IsSpecialStage(gamemap))
			{
				if (player == &players[displayplayer])  
					S_StartSound(NULL, sfx_s3k6a);
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i])
						players[i].exiting = (14*TICRATE)/5 + 1;
			}
			else {

				P_DamageMobj(player->mo, NULL, NULL, 1, 0);


				player->marescore = player->spheres =\
				 player->rings = 0;
			}
			player->powers[pw_carry] = CR_NONE;
		}
	}





	if (twodlevel || player->mo->flags2 & MF2_TWOD)  
		P_2dMovement(player);
	else
	{
		if (!player->climbing)
		{
			switch (P_ControlStyle(player))
			{
			case CS_LEGACY:
			case CS_STANDARD:
				player->mo->angle = (cmd->angleturn<<16  );
				break;

			case CS_LMAOGALOG:
				break;  

			case CS_SIMPLE:
				if (cmd->forwardmove || cmd->sidemove)
				{
					angle_t controlangle = R_PointToAngle2(0, 0, cmd->forwardmove << FRACBITS, -cmd->sidemove << FRACBITS);
					player->mo->angle = (cmd->angleturn<<16  ) + controlangle;
				}
				else
				{
					angle_t drawangleoffset = (player->powers[pw_carry] == CR_ROLLOUT) ? ANGLE_180 : 0;
					player->mo->angle = player->drawangle + drawangleoffset;
				}

				break;
			}
		}

		ticruned++;
		if ((cmd->angleturn & TICCMD_RECEIVED) == 0)
			ticmiss++;

		P_3dMovement(player);
	}

	if (maptol & TOL_2D)
		runspd = FixedMul(runspd, 2*FRACUNIT/3);

	P_SkidStuff(player);





	if ((cmd->forwardmove != 0 || cmd->sidemove != 0) || (player->powers[pw_super] && !onground))
	{

		if (player->charflags & SF_DASHMODE && player->dashmode >= DASHMODE_THRESHOLD && player->panim == PA_RUN && !player->skidtime && (onground || ((player->charability == CA_FLOAT || player->charability == CA_SLOWFALL) && player->secondjump == 1) || player->powers[pw_super]))
			P_SetMobjState (player->mo, S_PLAY_DASH);


		else if (player->speed >= runspd && player->panim == PA_WALK && !player->skidtime
		&& (onground || ((player->charability == CA_FLOAT || player->charability == CA_SLOWFALL) && player->secondjump == 1) || player->powers[pw_super]))
		{
			if (!onground)
				P_SetMobjState(player->mo, S_PLAY_FLOAT_RUN);
			else
				P_SetMobjState(player->mo, S_PLAY_RUN);
		}


		else if ((((player->charability == CA_FLOAT || player->charability == CA_SLOWFALL) && player->secondjump == 1) || player->powers[pw_super]) && player->panim == PA_IDLE && !onground)
			P_SetMobjState (player->mo, S_PLAY_FLOAT);


		else if ((player->rmomx || player->rmomy) && player->panim == PA_IDLE)
			P_SetMobjState (player->mo, S_PLAY_WALK);
	}



	if (player->charflags & SF_DASHMODE && player->panim == PA_DASH && player->dashmode < DASHMODE_THRESHOLD)
		P_SetMobjState(player->mo, S_PLAY_RUN);



	if (player->panim == PA_RUN && player->speed < runspd)
	{
		if (!onground && (((player->charability == CA_FLOAT || player->charability == CA_SLOWFALL) && player->secondjump == 1) || player->powers[pw_super]))
			P_SetMobjState(player->mo, S_PLAY_FLOAT);
		else
			P_SetMobjState(player->mo, S_PLAY_WALK);
	}


	if (onground)
	{
		if (player->mo->state-states == S_PLAY_FLOAT)
			P_SetMobjState(player->mo, S_PLAY_WALK);
		else if (player->mo->state-states == S_PLAY_FLOAT_RUN)
			P_SetMobjState(player->mo, S_PLAY_RUN);
	}


	if ((player->panim == PA_SPRING && P_MobjFlip(player->mo)*player->mo->momz < 0)
		|| ((((player->charflags & SF_NOJUMPSPIN) && (player->pflags & PF_JUMPED) && player->panim == PA_JUMP))
			&& (P_MobjFlip(player->mo)*player->mo->momz < 0)))
		P_SetMobjState(player->mo, S_PLAY_FALL);

	else if (onground && (player->panim == PA_SPRING || player->panim == PA_FALL || player->panim == PA_RIDE || player->panim == PA_JUMP) && !player->mo->momz)
		P_SetMobjState(player->mo, S_PLAY_STND);


	if (!player->mo->momx && !player->mo->momy && !player->mo->momz && player->panim == PA_WALK)
		P_SetMobjState(player->mo, S_PLAY_STND);






	if (onground && player->pflags & PF_JUMPED && !(player->pflags & PF_GLIDING)
	&& P_MobjFlip(player->mo)*player->mo->momz < 0)
	{
		player->pflags &= ~(PF_STARTJUMP|PF_JUMPED|PF_NOJUMPDAMAGE|PF_THOKKED|PF_SHIELDABILITY);
		player->secondjump = 0;
		P_SetMobjState(player->mo, S_PLAY_STND);
	}

	if ((!(player->charability == CA_GLIDEANDCLIMB) || player->gotflag)  
		&& (player->pflags & PF_GLIDING || player->climbing))
	{
		if (onground)
			P_SetMobjState(player->mo, S_PLAY_WALK);
		else
		{
			player->pflags |= P_GetJumpFlags(player);
			P_SetMobjState(player->mo, S_PLAY_JUMP);
		}
		player->pflags &= ~PF_GLIDING;
		player->glidetime = 0;
		player->climbing = 0;
	}

	if ((!(player->charability == CA_BOUNCE) || player->gotflag)  
		&& (player->pflags & PF_BOUNCING))
	{
		if (onground)
			P_SetMobjState(player->mo, S_PLAY_WALK);
		else
		{
			player->pflags |= P_GetJumpFlags(player);
			P_SetMobjState(player->mo, S_PLAY_JUMP);
		}
		player->pflags &= ~PF_BOUNCING;
	}



	if (player->pflags & PF_GLIDING)
	{
		mobj_t *mo = player->mo;  
		fixed_t glidespeed = player->actionspd;
		fixed_t momx = mo->momx - player->cmomx, momy = mo->momy - player->cmomy;
		angle_t angle, moveangle = R_PointToAngle2(0, 0, momx, momy);
		boolean swimming = mo->state - states == S_PLAY_SWIM;
		boolean in2d = mo->flags2 & MF2_TWOD || twodlevel;

		if (player->powers[pw_super] || player->powers[pw_sneakers])
			glidespeed *= 2;

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			if (player->mo->momz > FixedMul(2*FRACUNIT, player->mo->scale))
				player->mo->momz -= FixedMul(3*(FRACUNIT/4), player->mo->scale);
		}
		else
		{
			if (player->mo->momz < FixedMul(-2*FRACUNIT, player->mo->scale))
				player->mo->momz += FixedMul(3*(FRACUNIT/4), player->mo->scale);
		}


		if ((P_ControlStyle(player) & CS_LMAOGALOG) || in2d)
			angle = mo->angle;
		else if (swimming)
			angle = mo->angle + R_PointToAngle2(0, 0, cmd->forwardmove<<FRACBITS, -cmd->sidemove<<FRACBITS);
		else
			angle = mo->angle - FixedAngle(cmd->sidemove * FRACUNIT);

		if (!player->skidtime)  
		{
			angle_t anglediff = angle - moveangle;
			fixed_t accelfactor = 4*FRACUNIT - 3*FINECOSINE((anglediff >> ANGLETOFINESHIFT) & FINEMASK);
			fixed_t speed, scale = mo->scale;

			if (in2d)
			{
				if (mo->eflags & MFE_UNDERWATER)
					speed = FixedMul((glidespeed>>1) + player->glidetime*750, scale);
				else
					speed = FixedMul(glidespeed + player->glidetime*1500, scale);
				P_InstaThrust(mo, angle, speed);
			}
			else if (swimming)
			{
				fixed_t minspeed;

				if (anglediff > ANGLE_180)
					anglediff = InvAngle(InvAngle(anglediff) >> 3);
				else
					anglediff = anglediff >> 3;

				minspeed = FixedMul((glidespeed>>1) + player->glidetime*750, scale);  
				speed = FixedHypot(momx, momy) - abs(P_ReturnThrustY(mo, anglediff, mo->scale));

				if (speed < minspeed)
				{
					momx += P_ReturnThrustX(mo, angle, FixedMul(accelfactor, scale));
					momy += P_ReturnThrustY(mo, angle, FixedMul(accelfactor, scale));
					speed = FixedHypot(momx, momy);  
				}

				mo->momx = P_ReturnThrustX(mo, moveangle + anglediff, speed) + player->cmomx;
				mo->momy = P_ReturnThrustY(mo, moveangle + anglediff, speed) + player->cmomy;
			}
			else
			{
				fixed_t newMagnitude, oldMagnitude = R_PointToDist2(momx, momy, 0, 0);

				if (mo->eflags & MFE_UNDERWATER)
					speed = FixedMul((glidespeed>>1) + player->glidetime*750, scale);
				else
					speed = FixedMul(glidespeed + player->glidetime*1500, scale);

				P_Thrust(mo, angle, FixedMul(accelfactor, scale));

				newMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);
				if (newMagnitude > speed)
				{
					fixed_t tempmomx, tempmomy;
					if (oldMagnitude > speed)
					{
						if (newMagnitude > oldMagnitude)
						{
							tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), oldMagnitude);
							tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), oldMagnitude);
							player->mo->momx = tempmomx + player->cmomx;
							player->mo->momy = tempmomy + player->cmomy;
						}

					}
					else
					{
						tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), speed);
						tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), speed);
						player->mo->momx = tempmomx + player->cmomx;
						player->mo->momy = tempmomy + player->cmomy;
					}
				}
			}
		}

		player->glidetime++;

		if (!(player->pflags & PF_JUMPDOWN))  
		{
			P_ResetPlayer(player);  
			if (onground)
				P_SetMobjState(player->mo, S_PLAY_WALK);
			else if (player->charflags & SF_MULTIABILITY)
			{
				player->pflags |= P_GetJumpFlags(player);
				P_SetMobjState(player->mo, S_PLAY_JUMP);
			}
			else
			{
				player->pflags |= PF_THOKKED;
				player->mo->momx >>= 1;
				player->mo->momy >>= 1;
				P_SetMobjState(player->mo, S_PLAY_FALL);
			}
		}
	}
	else if (player->climbing)  
	{

		if (!player->cmd.forwardmove)
			player->mo->momz = 0;
	}
	else if (player->pflags & PF_BOUNCING)
	{
		if (!(player->pflags & PF_JUMPDOWN))  
		{
			P_ResetPlayer(player);  
			player->pflags |= PF_THOKKED;
			if (onground)
				P_SetMobjState(player->mo, S_PLAY_WALK);
			else if (player->charflags & SF_MULTIABILITY)
			{
				player->pflags |= P_GetJumpFlags(player);
				P_SetMobjState(player->mo, S_PLAY_JUMP);
			}
			else
			{
				player->mo->momx >>= 1;
				player->mo->momy >>= 1;
				player->mo->momz >>= 1;
				P_SetMobjState(player->mo, S_PLAY_FALL);
			}
		}
	}
	else if (player->mo->state-states == S_PLAY_BOUNCE)
		P_SetMobjState(player->mo, S_PLAY_FALL);


	if (!player->climbing
	&& ((!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + player->mo->height >= player->mo->watertop && player->mo->z <= player->mo->watertop)
		|| (player->mo->eflags & MFE_VERTICALFLIP && player->mo->z + player->mo->height >= player->mo->waterbottom && player->mo->z <= player->mo->waterbottom))
	&& (player->speed > runspd || (player->pflags & PF_STARTDASH))
	&& leveltime % (TICRATE/7) == 0 && player->mo->momz == 0 && !(player->pflags & PF_SLIDING) && !player->spectator)
	{
		mobjtype_t splishtype = (player->mo->eflags & MFE_TOUCHLAVA) ? MT_LAVASPLISH : MT_SPLISH;
		mobj_t *water = P_SpawnMobj(player->mo->x - P_ReturnThrustX(NULL, player->mo->angle, player->mo->radius), player->mo->y - P_ReturnThrustY(NULL, player->mo->angle, player->mo->radius),
			((player->mo->eflags & MFE_VERTICALFLIP) ? player->mo->waterbottom - FixedMul(mobjinfo[splishtype].height, player->mo->scale) : player->mo->watertop), splishtype);
		if (!P_MobjWasRemoved(water))
		{
			if (player->mo->eflags & MFE_GOOWATER)
				S_StartSound(water, sfx_ghit);
			else if (player->mo->eflags & MFE_TOUCHLAVA)
				S_StartSound(water, sfx_splash);
			else
				S_StartSound(water, sfx_wslap);
			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				water->flags2 |= MF2_OBJECTFLIP;
				water->eflags |= MFE_VERTICALFLIP;
			}
			P_SetScale(water, player->mo->scale, true);
		}
	}


	if ((player->mo->eflags & MFE_TOUCHWATER) && !(player->mo->eflags & MFE_UNDERWATER) && !player->spectator)
	{
		if (P_RandomChance(FRACUNIT/2) && leveltime % TICRATE == 0)
			S_StartSound(player->mo, sfx_floush);
	}





	if (!(player->charability == CA_FLY || player->charability == CA_SWIM))  
	{
		if (player->powers[pw_tailsfly]
		|| player->mo->state-states == S_PLAY_FLY_TIRED)
		{
			if (onground)
				P_SetMobjState(player->mo, S_PLAY_WALK);
			else
			{
				player->pflags |= P_GetJumpFlags(player);
				P_SetMobjState(player->mo, S_PLAY_JUMP);
			}
		}
		player->powers[pw_tailsfly] = 0;
	}

	if (player->gotflag && player->powers[pw_tailsfly])
		player->powers[pw_tailsfly] = 1;


	if (player->panim != PA_ABILITY)
		player->powers[pw_tailsfly] = 0;

	if (player->charability == CA_FLY || (player->charability == CA_SWIM && player->mo->eflags & MFE_UNDERWATER))
	{

		if (player->powers[pw_tailsfly])
		{
			const fixed_t actionspd = player->actionspd/100;

			if (player->charflags & SF_MULTIABILITY)
			{

				if (cmd->buttons & BT_JUMP && !(player->pflags & PF_STASIS) && !player->exiting)
					P_SetObjectMomZ(player->mo, actionspd/4, true);
			}
			else
			{

				if (player->fly1)
				{
					if (P_MobjFlip(player->mo)*player->mo->momz < FixedMul(5*actionspd, player->mo->scale))
						P_SetObjectMomZ(player->mo, actionspd/2, true);

					P_SetMobjState(player->mo, player->mo->state->nextstate);

					player->fly1--;
				}
			}


			if (player->charability == CA_FLY
				&& (player->pflags & PF_CANCARRY)
				&& !(player->mo->eflags & MFE_UNDERWATER)
				&& leveltime % 10 == 0
				&& !player->spectator)
				S_StartSound(player->mo, sfx_putput);


			if (cmd->buttons & BT_SPIN && !(player->pflags & PF_STASIS) && !player->exiting && !(player->mo->eflags & MFE_GOOWATER))
				if (P_MobjFlip(player->mo)*player->mo->momz > -FixedMul(5*actionspd, player->mo->scale))
				{
					if (player->fly1 > 2)
						player->fly1 = 2;
					P_SetObjectMomZ(player->mo, -actionspd/2, true);
				}

		}
		else
		{

			if (player->panim == PA_ABILITY && player->mo->state-states != S_PLAY_FLY_TIRED)
				P_SetMobjState(player->mo, S_PLAY_FLY_TIRED);

			if (player->charability == CA_FLY && (leveltime % 10 == 0)
				&& player->mo->state-states == S_PLAY_FLY_TIRED
				&& !(player->mo->eflags & MFE_UNDERWATER)
				&& !player->spectator)
				S_StartSound(player->mo, sfx_pudpud);
		}
	}






	if ((onground || player->climbing) && ((player->pflags & (PF_STARTDASH|PF_SPINNING)) != PF_SPINNING) && !(player->pflags & PF_JUMPED) && player->powers[pw_invulnerability] <= 1)
		P_ResetScore(player);


	if (player->pflags & PF_SPINNING && P_AproxDistance(player->speed, player->mo->momz) > FixedMul(15<<FRACBITS, player->mo->scale) && !(player->pflags & PF_JUMPED))
	{
		P_SpawnSpinMobj(player, player->spinitem);
		G_GhostAddSpin();
	}







	if (!onground && player->powers[pw_carry] != CR_NIGHTSMODE)
	{
		player->pflags &= ~PF_STARTDASH;
		player->dashspeed = 0;
	}

	if ((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL
	&& (player->pflags & PF_SPINNING) && player->speed > FixedMul(4<<FRACBITS, player->mo->scale) && onground && (leveltime & 1)
	&& !(player->mo->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)))
		P_ElementalFire(player, false);

	P_DoSpinAbility(player, cmd);


	P_DoJumpStuff(player, cmd);


	if (!(player->pflags & PF_SPINNING) && player->powers[pw_carry] != CR_NIGHTSMODE)
		player->pflags &= ~PF_STARTDASH;






	if (P_ControlStyle(player) == CS_LMAOGALOG
		&& (cmd->forwardmove != 0 || cmd->sidemove != 0) && !player->climbing && !twodlevel && !(player->mo->flags2 & MF2_TWOD))
	{


		if (player->speed < FixedMul(5*FRACUNIT, player->mo->scale) || player->pflags & PF_GLIDING || !onground)
		{
			angle_t tempangle;

			tempangle = (cmd->angleturn << 16);

#ifdef REDSANALOG  
			tempangle += R_PointToAngle2(0, 0, cmd->forwardmove*FRACUNIT, -cmd->sidemove*FRACUNIT);
			{
				fixed_t tweenvalue = max(abs(cmd->forwardmove), abs(cmd->sidemove));

				if (tweenvalue < 10 && (cmd->buttons & (BT_CAMLEFT|BT_CAMRIGHT)) == (BT_CAMLEFT|BT_CAMRIGHT)) {
					tempangle = (cmd->angleturn << 16);
					tweenvalue = 16;
				}

				tweenvalue *= tweenvalue*tweenvalue*1536;




				tempangle -= player->mo->angle;

				if (tempangle < ANGLE_180 && tempangle > tweenvalue)
					player->mo->angle += tweenvalue;
				else if (tempangle >= ANGLE_180 && InvAngle(tempangle) > tweenvalue)
					player->mo->angle -= tweenvalue;
				else
					player->mo->angle += tempangle;
			}
#else

			player->mo->angle = R_PointToAngle2(0, 0, cmd->forwardmove*FRACUNIT, -cmd->sidemove*FRACUNIT)+tempangle;
#endif
		}

		else if (player->panim == PA_WALK || player->panim == PA_RUN || player->panim == PA_DASH || player->panim == PA_ROLL || player->panim == PA_JUMP
		|| (player->panim == PA_ABILITY && player->mo->state-states == S_PLAY_GLIDE))
			player->mo->angle = R_PointToAngle2(0, 0, player->rmomx, player->rmomy);


		P_SetPlayerAngle(player, player->mo->angle);
	}

	if (player->climbing == 1)
		P_DoClimbing(player);

	if (player->climbing > 1)
	{
		P_InstaThrust(player->mo, player->mo->angle, FixedMul(4*FRACUNIT, player->mo->scale));  
		player->climbing--;
	}


	if (player->panim == PA_EDGE
	&& (player->mo->momx || player->mo->momy || player->mo->momz))
		P_SetMobjState(player->mo, S_PLAY_STND);


	if (!(player->mo->momz || player->mo->momx || player->mo->momy) && !(player->mo->eflags & MFE_GOOWATER)
	&& player->panim == PA_IDLE && !(player->powers[pw_carry]))
		P_DoTeeter(player);


	if (G_GametypeHasTeams() && (cmd->buttons & BT_TOSSFLAG) && !(player->powers[pw_super]) && !(player->tossdelay))
	{
		if (!(player->gotflag & (GF_REDFLAG|GF_BLUEFLAG)))
			P_PlayerEmeraldBurst(player, true);  
		else
			P_PlayerFlagBurst(player, true);
	}


	if (!player->exiting)
		P_DoFiring(player, cmd);

	{
		boolean atspinheight = false;
		fixed_t oldheight = player->mo->height;
		fixed_t luaheight = LUA_HookPlayerHeight(player);

		if (luaheight != -1)
		{
			player->mo->height = luaheight;
			if (luaheight <= P_GetPlayerSpinHeight(player))
				atspinheight = true;  
		}

		else if (P_PlayerShouldUseSpinHeight(player))
		{
			player->mo->height = P_GetPlayerSpinHeight(player);
			atspinheight = true;
		}
		else if (player->powers[pw_carry] == CR_PLAYER || player->powers[pw_carry] == CR_PTERABYTE)  
			player->mo->height = FixedDiv(P_GetPlayerHeight(player), FixedDiv(14*FRACUNIT,10*FRACUNIT));
		else
			player->mo->height = P_GetPlayerHeight(player);

		if (player->mo->eflags & MFE_VERTICALFLIP && player->mo->height != oldheight)  
		{
			player->mo->z     -= player->mo->height - oldheight;
			player->mo->old_z -= player->mo->height - oldheight;  
		}


		if ((player->mo->ceilingz - player->mo->floorz < player->mo->height)
			&& !(player->mo->flags & MF_NOCLIP))
		{
			if (!atspinheight)
			{
				player->pflags |= PF_SPINNING;
				P_SetMobjState(player->mo, S_PLAY_ROLL);
			}
			else if (player->mo->ceilingz - player->mo->floorz < player->mo->height)
			{
				if ((netgame || multiplayer) && player->spectator)
					P_DamageMobj(player->mo, NULL, NULL, 1, DMG_SPECTATOR);  
				else
					P_DamageMobj(player->mo, NULL, NULL, 1, DMG_CRUSHED);

				if (player->playerstate == PST_DEAD)
					return;
			}
		}
	}

	if (cv_fovchange.value)
	{
		fixed_t speed;
		const fixed_t runnyspeed = 20*FRACUNIT;

		speed = R_PointToDist2(player->rmomx, player->rmomy, 0, 0);

		if (speed > player->normalspeed-5*FRACUNIT)
			speed = player->normalspeed-5*FRACUNIT;

		if (speed >= runnyspeed)
			player->fovadd = speed-runnyspeed;
		else
			player->fovadd = 0;

		if (player->fovadd < 0)
			player->fovadd = 0;
	}
	else
		player->fovadd = 0;




	if (CheckForBustableBlocks)
		P_CheckBustableBlocks(player);


	if (CheckForBouncySector)
		P_CheckBouncySectors(player);


	if (CheckForQuicksand)
		P_CheckQuicksand(player);

	if (P_IsObjectOnGround(player->mo))
		player->mo->pmomz = 0;
}

static void P_DoZoomTube(player_t *player)
{
	fixed_t speed;
	mobj_t *waypoint = NULL;
	fixed_t dist;
	boolean reverse;

	player->mo->height = P_GetPlayerSpinHeight(player);

	if (player->speed > 0)
		reverse = false;
	else
		reverse = true;

	player->powers[pw_flashing] = 1;

	speed = abs(player->speed);


	dist = P_AproxDistance(P_AproxDistance(player->mo->tracer->x - player->mo->x, player->mo->tracer->y - player->mo->y), player->mo->tracer->z - player->mo->z);

	if (dist < 1)
		dist = 1;

	player->mo->momx = FixedMul(FixedDiv(player->mo->tracer->x - player->mo->x, dist), (speed));
	player->mo->momy = FixedMul(FixedDiv(player->mo->tracer->y - player->mo->y, dist), (speed));
	player->mo->momz = FixedMul(FixedDiv(player->mo->tracer->z - player->mo->z, dist), (speed));





	if (speed > dist)
	{
		speed -= dist;

		P_UnsetThingPosition(player->mo);
		player->mo->x = player->mo->tracer->x;
		player->mo->y = player->mo->tracer->y;
		player->mo->z = player->mo->tracer->z;
		P_SetThingPosition(player->mo);


		player->mo->floorz = player->mo->subsector->sector->floorheight;
		player->mo->ceilingz = player->mo->subsector->sector->ceilingheight;

		CONS_Debug(DBG_GAMELOGIC, "Looking for next waypoint...\n");


		waypoint = reverse ? P_GetPreviousWaypoint(player->mo->tracer, false) : P_GetNextWaypoint(player->mo->tracer, false);

		if (waypoint)
		{
			CONS_Debug(DBG_GAMELOGIC, "Found waypoint (sequence %d, number %d).\n", waypoint->threshold, waypoint->health);

			P_SetTarget(&player->mo->tracer, waypoint);




			dist = P_AproxDistance(P_AproxDistance(player->mo->tracer->x - player->mo->x, player->mo->tracer->y - player->mo->y), player->mo->tracer->z - player->mo->z);

			if (dist < 1)
				dist = 1;

			player->mo->momx = FixedMul(FixedDiv(player->mo->tracer->x - player->mo->x, dist), (speed));
			player->mo->momy = FixedMul(FixedDiv(player->mo->tracer->y - player->mo->y, dist), (speed));
			player->mo->momz = FixedMul(FixedDiv(player->mo->tracer->z - player->mo->z, dist), (speed));
		}
		else
		{
			P_SetTarget(&player->mo->tracer, NULL);  
			player->powers[pw_carry] = CR_NONE;

			CONS_Debug(DBG_GAMELOGIC, "Next waypoint not found, releasing from track...\n");
		}
	}


	if (player->mo->tracer)
	{
		player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, player->mo->tracer->x, player->mo->tracer->y);
		P_SetPlayerAngle(player, player->mo->angle);
	}
}







static void P_DoRopeHang(player_t *player)
{
	INT32 sequence;
	fixed_t speed;
	mobj_t *waypoint = NULL;
	fixed_t dist;
	fixed_t playerz;

	player->mo->height = P_GetPlayerHeight(player);


	if (!(leveltime & 7) && player->speed)
		S_StartSound(player->mo, sfx_s3k55);

	playerz = player->mo->z + player->mo->height;

	speed = abs(player->speed);

	sequence = player->mo->tracer->threshold;


	dist = P_AproxDistance(P_AproxDistance(player->mo->tracer->x - player->mo->x, player->mo->tracer->y - player->mo->y), player->mo->tracer->z - playerz);

	if (dist < 1)
		dist = 1;

	player->mo->momx = FixedMul(FixedDiv(player->mo->tracer->x - player->mo->x, dist), (speed));
	player->mo->momy = FixedMul(FixedDiv(player->mo->tracer->y - player->mo->y, dist), (speed));
	player->mo->momz = FixedMul(FixedDiv(player->mo->tracer->z - playerz, dist), (speed));

	if (player->cmd.buttons & BT_SPIN && !(player->pflags & PF_STASIS))  
	{
		player->pflags |= (P_GetJumpFlags(player)|PF_SPINDOWN);
		P_SetMobjState(player->mo, S_PLAY_JUMP);

		P_SetTarget(&player->mo->tracer, NULL);
		player->powers[pw_carry] = CR_NONE;

		return;
	}

	if (player->mo->state-states != S_PLAY_RIDE)
		P_SetMobjState(player->mo, S_PLAY_RIDE);


	if (!speed)
		return;





	if (speed > dist)
	{
		speed -= dist;

		P_UnsetThingPosition(player->mo);
		player->mo->x = player->mo->tracer->x;
		player->mo->y = player->mo->tracer->y;
		player->mo->z = player->mo->tracer->z - player->mo->height;
		playerz = player->mo->tracer->z;
		P_SetThingPosition(player->mo);

		CONS_Debug(DBG_GAMELOGIC, "Looking for next waypoint...\n");


		waypoint = P_GetNextWaypoint(player->mo->tracer, false);

		if (!(player->mo->tracer->flags & MF_SLIDEME) && !waypoint)
		{
			CONS_Debug(DBG_GAMELOGIC, "Next waypoint not found, wrapping to start...\n");


			waypoint = P_GetFirstWaypoint(sequence);
		}

		if (waypoint)
		{
			CONS_Debug(DBG_GAMELOGIC, "Found waypoint (sequence %d, number %d).\n", waypoint->threshold, waypoint->health);

			P_SetTarget(&player->mo->tracer, waypoint);



			dist = P_AproxDistance(P_AproxDistance(player->mo->tracer->x - player->mo->x, player->mo->tracer->y - player->mo->y), player->mo->tracer->z - playerz);

			if (dist < 1)
				dist = 1;

			player->mo->momx = FixedMul(FixedDiv(player->mo->tracer->x - player->mo->x, dist), (speed));
			player->mo->momy = FixedMul(FixedDiv(player->mo->tracer->y - player->mo->y, dist), (speed));
			player->mo->momz = FixedMul(FixedDiv(player->mo->tracer->z - playerz, dist), (speed));
		}
		else
		{
			if (player->mo->tracer->flags & MF_SLIDEME)
			{
				player->pflags |= P_GetJumpFlags(player);
				P_SetMobjState(player->mo, S_PLAY_JUMP);
			}

			P_SetTarget(&player->mo->tracer, NULL);
			player->powers[pw_carry] = CR_NONE;

			CONS_Debug(DBG_GAMELOGIC, "Next waypoint not found!\n");
		}
	}
}

#if 0






static void P_NukeAllPlayers(player_t *player)
{
	mobj_t *mo;
	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;
		if (players[i].spectator)
			continue;
		if (!players[i].mo)
			continue;
		if (players[i].mo == player->mo)
			continue;
		if (players[i].mo->health <= 0)
			continue;

		P_DamageMobj(players[i].mo, player->mo, player->mo, 1, 0);
	}

	CONS_Printf(M_GetText("%s caused a world of pain.\n"), player_names[player-players]);

	return;
}
#endif





void P_NukeEnemies(mobj_t *inflictor, mobj_t *source, fixed_t radius)
{
	const fixed_t ns = 60 << FRACBITS;
	mobj_t *mo;
	angle_t fa;
	thinker_t *think;
	INT32 i;

	for (i = 0; i < 16; i++)
	{
		fa = (i*(FINEANGLES/16));
		mo = P_SpawnMobj(inflictor->x, inflictor->y, inflictor->z, MT_SUPERSPARK);
		if (!P_MobjWasRemoved(mo))
		{
			mo->momx = FixedMul(FINESINE(fa),ns);
			mo->momy = FixedMul(FINECOSINE(fa),ns);
		}
	}

	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->removing)
			continue;

		mo = (mobj_t *)think;

		if (!(mo->flags & MF_SHOOTABLE) && !(mo->type == MT_EGGGUARD || mo->type == MT_MINUS))
			continue;

		if (mo->flags & MF_MONITOR)
			continue;  

		if (!G_RingSlingerGametype() && mo->type == MT_PLAYER)
			continue;  

		if (abs(inflictor->x - mo->x) > radius || abs(inflictor->y - mo->y) > radius || abs(inflictor->z - mo->z) > radius)
			continue;  

		if (P_AproxDistance(P_AproxDistance(inflictor->x - mo->x, inflictor->y - mo->y), inflictor->z - mo->z) > radius)
			continue;

		if (mo->type == MT_MINUS && !(mo->flags & (MF_SPECIAL|MF_SHOOTABLE)))
			mo->flags = (mo->flags & ~MF_NOCLIPTHING)|MF_SPECIAL|MF_SHOOTABLE;

		if (mo->type == MT_EGGGUARD && mo->tracer)  
		{
			P_KillMobj(mo->tracer, inflictor, source, DMG_NUKE);
			P_KillMobj(mo, inflictor, source, DMG_NUKE);
		}

		if (mo->flags & MF_BOSS || mo->type == MT_PLAYER)  
			P_DamageMobj(mo, inflictor, source, 1, DMG_NUKE);
		else
			P_DamageMobj(mo, inflictor, source, 1000, DMG_NUKE);
	}
}





void P_Earthquake(mobj_t *inflictor, mobj_t *source, fixed_t radius)
{
	const fixed_t scaledradius = FixedMul(radius, inflictor->scale);
	const fixed_t ns = scaledradius/12;
	mobj_t *mo;
	angle_t fa;
	INT32 i;
	boolean grounded = P_IsObjectOnGround(inflictor);

	for (i = 0; i < 16; i++)
	{
		fa = (i*(FINEANGLES/16));
		mo = P_SpawnMobjFromMobj(inflictor, 0, 0, 0, MT_SUPERSPARK);
		if (!P_MobjWasRemoved(mo))
		{
			if (grounded)
			{
				mo->momx = FixedMul(FINESINE(fa),ns);
				mo->momy = FixedMul(FINECOSINE(fa),ns);
			}
			else
			{
				P_InstaThrust(mo, inflictor->angle + ANGLE_90, FixedMul(FINECOSINE(fa),ns));
				mo->momz = FixedMul(FINESINE(fa),ns);
			}
		}
	}

	if (inflictor->player && P_IsLocalPlayer(inflictor->player))
	{
		quake.epicenter = NULL;
		quake.intensity = 8*inflictor->scale;
		quake.time = 8;
		quake.radius = scaledradius;
	}

	P_RadiusAttack(inflictor, source, radius, 0, false);
}








mobj_t *P_LookForFocusTarget(player_t *player, mobj_t *exclude, SINT8 direction, UINT8 lockonflags)
{
	mobj_t *mo;
	thinker_t *think;
	mobj_t *closestmo = NULL;
	const fixed_t maxdist = 2560*player->mo->scale;
	const angle_t span = ANGLE_45;
	fixed_t dist, closestdist = 0;
	angle_t dangle, closestdangle = 0;

	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->removing)
			continue;

		mo = (mobj_t *)think;

		if (mo->flags & MF_NOCLIPTHING)
			continue;

		if (mo == player->mo || mo == exclude)
			continue;

		if (mo->health <= 0)  
			continue;

		switch (mo->type)
		{
		case MT_PLAYER:  
		case MT_DETON:
			continue;  

		case MT_FAKEMOBILE:
			if (!(lockonflags & LOCK_BOSS))
				continue;
			break;

		case MT_EGGSHIELD:
			if (!(lockonflags & LOCK_ENEMY))
				continue;
			break;

		case MT_EGGSTATUE:
			if (tutorialmode)
				break;  
			 
		default:

			if ((lockonflags & LOCK_BOSS) && (mo->flags & MF_BOSS))  
				break;

			if ((lockonflags & LOCK_ENEMY) && (!((mo->flags & (MF_ENEMY|MF_SHOOTABLE)) == (MF_ENEMY|MF_SHOOTABLE)) != !(mo->flags2 & MF2_INVERTAIMABLE)))  
				break;

			if ((lockonflags & LOCK_INTERESTS) && (mo->flags & (MF_PUSHABLE|MF_MONITOR)))  
				break;

			continue;  
		}

		{
			fixed_t zdist = (player->mo->z + player->mo->height/2) - (mo->z + mo->height/2);
			dist = P_AproxDistance(player->mo->x-mo->x, player->mo->y-mo->y);

			if (abs(zdist) > dist)
				continue;  

			dist = P_AproxDistance(dist, zdist);
			if (dist > maxdist)
				continue;  
		}

		if ((twodlevel || player->mo->flags2 & MF2_TWOD)
		&& abs(player->mo->y-mo->y) > player->mo->radius)
			continue;  

		dangle = R_PointToAngle2(player->mo->x, player->mo->y, mo->x, mo->y) - (
			!exclude ? player->mo->angle : R_PointToAngle2(player->mo->x, player->mo->y, exclude->x, exclude->y)
		);

		if (direction)
		{
			if (direction == 1 && dangle > ANGLE_180)
				continue;  
			if (direction == -1 && dangle < ANGLE_180)
				continue;  
		}

		if (dangle > ANGLE_180)
			dangle = InvAngle(dangle);

		if (dangle > span)
			continue;  


		dist = FixedDiv(dist, FINECOSINE(dangle>>ANGLETOFINESHIFT)*3);

		if (closestmo && (exclude ? (dangle > closestdangle) : (dist > closestdist)))
			continue;

		if (!P_CheckSight(player->mo, mo))
			continue;  

		closestmo = mo;
		closestdist = dist;
		closestdangle = dangle;
	}

	return closestmo;
}









mobj_t *P_LookForEnemies(player_t *player, boolean nonenemies, boolean bullet)
{
	mobj_t *mo;
	thinker_t *think;
	mobj_t *closestmo = NULL;
	const fixed_t maxdist = FixedMul((bullet ? RING_DIST*2 : RING_DIST), player->mo->scale);
	const angle_t span = (bullet ? ANG30 : ANGLE_90);
	fixed_t dist, closestdist = 0;
	const mobjflag_t nonenemiesdisregard = (bullet ? 0 : MF_MONITOR)|MF_SPRING;

	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->removing)
			continue;

		mo = (mobj_t *)think;

		if (mo->flags & MF_NOCLIPTHING)
			continue;

		if (mo->health <= 0)  
			continue;

		if (!((mo->flags & (MF_ENEMY|MF_BOSS|MF_MONITOR) && (mo->flags & MF_SHOOTABLE)) || (mo->flags & MF_SPRING)) == !(mo->flags2 & MF2_INVERTAIMABLE))  
			continue;  

		if (mo == player->mo)
			continue;

		if (mo->flags2 & MF2_FRET)
			continue;

		if (!nonenemies && mo->flags & nonenemiesdisregard)
			continue;

		if (!bullet && mo->type == MT_DETON)  
			continue;

		{
			fixed_t zdist = (player->mo->z + player->mo->height/2) - (mo->z + mo->height/2);
			dist = R_PointToDist2(0, 0, player->mo->x-mo->x, player->mo->y-mo->y);
			if (bullet)
			{
				if ((R_PointToAngle2(0, 0, dist, zdist) + span) > span*2)
					continue;  
			}
			else  
			{
				if (player->mo->eflags & MFE_VERTICALFLIP)
				{
					if (mo->z+mo->height < player->mo->z+player->mo->height-FixedMul(MAXSTEPMOVE, player->mo->scale))
							continue;
				}
				else if (mo->z > player->mo->z+FixedMul(MAXSTEPMOVE, player->mo->scale))
					continue;
			}

			dist = R_PointToDist2(0, 0, dist, zdist);
			if (dist > maxdist)
				continue;  
		}

		if ((twodlevel || player->mo->flags2 & MF2_TWOD)
		&& abs(player->mo->y-mo->y) > player->mo->radius)
			continue;  

		if (mo->type == MT_PLAYER)  
			continue;

		if (closestmo && dist > closestdist)
			continue;

		if ((R_PointToAngle2(player->mo->x + P_ReturnThrustX(player->mo, player->mo->angle, player->mo->radius), player->mo->y + P_ReturnThrustY(player->mo, player->mo->angle, player->mo->radius), mo->x, mo->y) - player->mo->angle + span) > span*2)
			continue;  

		if (!P_CheckSight(player->mo, mo))
			continue;  

		closestmo = mo;
		closestdist = dist;
	}

	return closestmo;
}

boolean P_HomingAttack(mobj_t *source, mobj_t *enemy)  
{
	fixed_t zdist;
	fixed_t dist;
	fixed_t ns = 0;

	if (!enemy)
		return false;

	if (enemy->flags & MF_NOCLIPTHING)
		return false;

	if (enemy->health <= 0)  
		return false;

	if (source->player && (!((enemy->flags & (MF_ENEMY|MF_BOSS|MF_MONITOR) && (enemy->flags & MF_SHOOTABLE)) || (enemy->flags & MF_SPRING)) == !(enemy->flags2 & MF2_INVERTAIMABLE)))  
		return false;

	if (enemy->flags2 & MF2_FRET)
		return false;


	source->angle = R_PointToAngle2(source->x, source->y, enemy->x, enemy->y);
	if (source->player)
	{
		source->player->drawangle = source->angle;
		if (!demoplayback || P_ControlStyle(source->player) == CS_LMAOGALOG)
			P_SetPlayerAngle(source->player, source->angle);
	}


	zdist = ((P_MobjFlip(source) == -1) ? (enemy->z + enemy->height) - (source->z + source->height) : (enemy->z - source->z));
	dist = P_AproxDistance(P_AproxDistance(enemy->x - source->x, enemy->y - source->y), zdist);

	if (dist < 1)
		dist = 1;

	if (source->type == MT_DETON && enemy->player)  
		ns = FixedDiv(FixedMul(enemy->player->normalspeed, enemy->scale), FixedDiv(20*FRACUNIT,17*FRACUNIT));
	else if (source->type != MT_PLAYER)
	{
		if (source->threshold == 32000)
			ns = FixedMul(source->info->speed/2, source->scale);
		else
			ns = FixedMul(source->info->speed, source->scale);
	}
	else if (source->player)
	{
		if (source->player->charability == CA_HOMINGTHOK && !(source->player->pflags & PF_SHIELDABILITY))
			ns = FixedDiv(FixedMul(source->player->actionspd, source->scale), 3*FRACUNIT/2);
		else
			ns = FixedMul(45*FRACUNIT, source->scale);
	}

	source->momx = FixedMul(FixedDiv(enemy->x - source->x, dist), ns);
	source->momy = FixedMul(FixedDiv(enemy->y - source->y, dist), ns);
	source->momz = FixedMul(FixedDiv(zdist, dist), ns);

	return true;
}


void P_FindEmerald(void)
{
	thinker_t *th;
	mobj_t *mo2;

	hunt1 = hunt2 = hunt3 = NULL;



	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mo2 = (mobj_t *)th;
		if (mo2->type == MT_EMERHUNT)
		{
			if (!hunt1)
				hunt1 = mo2;
			else if (!hunt2)
				hunt2 = mo2;
			else if (!hunt3)
				hunt3 = mo2;
		}
	}
	return;
}






boolean P_GetLives(player_t *player)
{
	INT32 i, maxlivesplayer = -1, livescheck = 1;
	if (!(netgame || multiplayer)
	|| !G_GametypeUsesCoopLives()
	|| (player->lives == INFLIVES))
		return true;

	if (cv_cooplives.value == 0)  
	{
		if (player->lives < 1)
			player->lives = 1;
		return true;
	}

	if ((cv_cooplives.value == 2 || cv_cooplives.value == 1) && player->lives > 0)
		return true;

	if (cv_cooplives.value == 1)
		return false;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (players[i].lives > livescheck)
		{
			maxlivesplayer = i;
			livescheck = players[i].lives;
		}
	}
	if (maxlivesplayer != -1 && &players[maxlivesplayer] != player)
	{
		if (cv_cooplives.value == 2 && (P_IsLocalPlayer(player) || P_IsLocalPlayer(&players[maxlivesplayer])))
			S_StartSound(NULL, sfx_jshard);  
		if (players[maxlivesplayer].lives != INFLIVES)
			players[maxlivesplayer].lives--;
		player->lives++;
		if (player->lives < 1)
			player->lives = 1;
		return true;
	}
	return (player->lives > 0);
}






static void P_ConsiderAllGone(void)
{
	INT32 i, lastdeadplayer = -1, deadtimercheck = INT32_MAX;

	if (countdown2)
		return;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (players[i].playerstate != PST_DEAD && !players[i].spectator && players[i].mo && players[i].mo->health)
			break;

		if (players[i].spectator)
		{
			if (lastdeadplayer == -1)
				lastdeadplayer = i;
		}
		else if (players[i].lives > 0)
		{
			lastdeadplayer = i;
			if (players[i].deadtimer < deadtimercheck)
				deadtimercheck = players[i].deadtimer;
		}
	}

	if (i == MAXPLAYERS && lastdeadplayer != -1 && deadtimercheck > 2*TICRATE)  
	{

		players[lastdeadplayer].outofcoop = true;
		players[lastdeadplayer].playerstate = PST_REBORN;
	}
}

void P_RestoreMultiMusic(player_t *player)
{
	if (netgame)
	{
		if (P_IsLocalPlayer(player))
			S_ChangeMusic(mapmusname, mapmusflags, true);
	}
	else if (multiplayer)  
	{


		if (player == &players[displayplayer] && players[secondarydisplayplayer].lives > 0)
			P_RestoreMusic(&players[secondarydisplayplayer]);
		else if (player == &players[secondarydisplayplayer] && players[displayplayer].lives > 0)
			P_RestoreMusic(&players[displayplayer]);
	}
}







static void P_DeathThink(player_t *player)
{
	INT32 j = MAXPLAYERS;

	ticcmd_t *cmd = &player->cmd;
	player->deltaviewheight = 0;

	if (player->deadtimer < INT32_MAX)
		player->deadtimer++;

	if (player->bot == BOT_2PAI || player->bot == BOT_2PHUMAN)  
		goto notrealplayer;


	if (!(netgame || multiplayer) && player->lives <= 0 && player == &players[consoleplayer])  
	{
		if (player->deadtimer > (3*TICRATE) && (cmd->buttons & BT_SPIN || cmd->buttons & BT_JUMP) && (!continuesInSession || player->continues > 0))
			G_UseContinue();
		else if (player->deadtimer >= gameovertics)
			G_UseContinue();  
	}

	if ((cv_cooplives.value != 1)
	&& (G_GametypeUsesCoopLives())
	&& (netgame || multiplayer)
	&& (player->lives <= 0))
	{
		for (j = 0; j < MAXPLAYERS; j++)
		{
			if (!playeringame[j])
				continue;

			if (players[j].lives > 1)
				break;
		}
	}


	if (player->deadtimer > 30*TICRATE && !G_PlatformGametype())
		player->playerstate = PST_REBORN;
	else if ((player->lives > 0 || j != MAXPLAYERS) && !(!(netgame || multiplayer) && G_IsSpecialStage(gamemap)))  
	{
		if (G_GametypeUsesCoopStarposts() && (netgame || multiplayer) && cv_coopstarposts.value == 2)
		{
			P_ConsiderAllGone();
			if ((player->deadtimer > TICRATE<<1) || ((cmd->buttons & BT_JUMP) && (player->deadtimer > TICRATE)))
			{

				player->outofcoop = true;
				player->playerstate = PST_REBORN;
			}
		}
		else
		{

			if (cmd->buttons & BT_JUMP)
			{

				if ((gametyperules & GTR_SPECTATORS) && player->spectator)
					player->playerstate = PST_REBORN;
				else
				{

					INT32 respawndelay = TICRATE;


					if (gametyperules & GTR_RESPAWNDELAY)
						respawndelay = (cv_respawntime.value*TICRATE);



					if (player->deadtimer > respawndelay)
						player->playerstate = PST_REBORN;
				}
			}


			if (!(netgame || multiplayer) && player->deadtimer > TICRATE<<1)
				player->playerstate = PST_REBORN;
		}
	}
	else if ((netgame || multiplayer) && player->deadtimer >= 8*TICRATE)
	{
		INT32 i, deadtimercheck = INT32_MAX;


		if (G_CompetitionGametype())
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;
				if (!players[i].exiting && players[i].lives)
					break;
				if (players[i].deadtimer < deadtimercheck)
					deadtimercheck = players[i].deadtimer;
			}

			if (i == MAXPLAYERS && deadtimercheck == 8*TICRATE)
			{

				if (!countdown2 || countdown2 > 1*TICRATE)
					countdown2 = 1*TICRATE;
			}
		}

	}

	if (G_CoopGametype() && (multiplayer || netgame) && (player->lives <= 0) && (player->deadtimer >= 8*TICRATE || ((cmd->buttons & BT_JUMP) && (player->deadtimer > TICRATE))))
	{

		player->outofcoop = true;
		player->playerstate = PST_REBORN;
	}

	if ((gametyperules & GTR_RACE) || (G_CoopGametype() && (multiplayer || netgame)))
	{

		if (!(countdown2 && !countdown) && !player->exiting && !(player->pflags & PF_GAMETYPEOVER) && !stoppedclock)
		{
			if (gametyperules & GTR_RACE)
			{
				if (leveltime >= 4*TICRATE)
					player->realtime = leveltime - 4*TICRATE;
				else
					player->realtime = 0;
			}
			else
				player->realtime = leveltime;
		}


		if (!G_CoopGametype() && player->lives <= 0 && player->deadtimer == gameovertics)
			P_RestoreMultiMusic(player);
	}

notrealplayer:

	if (!player->mo)
		return;

	P_CalcHeight(player);
}





camera_t camera, camera2;  

static void CV_CamRotate_OnChange(void)
{
	if (cv_cam_rotate.value < 0)
		CV_SetValue(&cv_cam_rotate, cv_cam_rotate.value + 360);
	else if (cv_cam_rotate.value > 359)
		CV_SetValue(&cv_cam_rotate, cv_cam_rotate.value % 360);
}

static void CV_CamRotate2_OnChange(void)
{
	if (cv_cam2_rotate.value < 0)
		CV_SetValue(&cv_cam2_rotate, cv_cam2_rotate.value + 360);
	else if (cv_cam2_rotate.value > 359)
		CV_SetValue(&cv_cam2_rotate, cv_cam2_rotate.value % 360);
}

static CV_PossibleValue_t CV_CamSpeed[] = {{0, "MIN"}, {1*FRACUNIT, "MAX"}, {0, NULL}};
static CV_PossibleValue_t rotation_cons_t[] = {{1, "MIN"}, {25, "MAX"}, {0, NULL}};
static CV_PossibleValue_t CV_CamRotate[] = {{-720, "MIN"}, {720, "MAX"}, {0, NULL}};
static CV_PossibleValue_t multiplier_cons_t[] = {{0, "MIN"}, {3*FRACUNIT, "MAX"}, {0, NULL}};
static CV_PossibleValue_t campos_cons_t[] = { {INT32_MIN, "MIN"}, {INT32_MAX, "MAX"}, {0, NULL} };

consvar_t cv_cam_dist = CVAR_INIT ("cam_curdist", "160", CV_FLOAT|CV_ALLOWLUA, campos_cons_t, NULL);
consvar_t cv_cam_height = CVAR_INIT ("cam_curheight", "25", CV_FLOAT|CV_ALLOWLUA, campos_cons_t, NULL);
consvar_t cv_cam_still = CVAR_INIT ("cam_still", "Off", CV_ALLOWLUA, CV_OnOff, NULL);
consvar_t cv_cam_speed = CVAR_INIT ("cam_speed", "0.3", CV_FLOAT|CV_SAVE|CV_ALLOWLUA, CV_CamSpeed, NULL);
consvar_t cv_cam_rotate = CVAR_INIT ("cam_rotate", "0", CV_CALL|CV_NOINIT|CV_ALLOWLUA, CV_CamRotate, CV_CamRotate_OnChange);
consvar_t cv_cam_rotspeed = CVAR_INIT ("cam_rotspeed", "10", CV_SAVE|CV_ALLOWLUA, rotation_cons_t, NULL);
consvar_t cv_cam_turnmultiplier = CVAR_INIT ("cam_turnmultiplier", "0.75", CV_FLOAT|CV_SAVE|CV_ALLOWLUA, multiplier_cons_t, NULL);
consvar_t cv_cam_orbit = CVAR_INIT ("cam_orbit", "Off", CV_SAVE|CV_ALLOWLUA, CV_OnOff, NULL);
consvar_t cv_cam_adjust = CVAR_INIT ("cam_adjust", "On", CV_SAVE|CV_ALLOWLUA, CV_OnOff, NULL);
consvar_t cv_cam2_dist = CVAR_INIT ("cam2_curdist", "160", CV_FLOAT|CV_ALLOWLUA, campos_cons_t, NULL);
consvar_t cv_cam2_height = CVAR_INIT ("cam2_curheight", "25", CV_FLOAT|CV_ALLOWLUA, campos_cons_t, NULL);
consvar_t cv_cam2_still = CVAR_INIT ("cam2_still", "Off", CV_ALLOWLUA, CV_OnOff, NULL);
consvar_t cv_cam2_speed = CVAR_INIT ("cam2_speed", "0.3", CV_FLOAT|CV_SAVE|CV_ALLOWLUA, CV_CamSpeed, NULL);
consvar_t cv_cam2_rotate = CVAR_INIT ("cam2_rotate", "0", CV_CALL|CV_NOINIT|CV_ALLOWLUA, CV_CamRotate, CV_CamRotate2_OnChange);
consvar_t cv_cam2_rotspeed = CVAR_INIT ("cam2_rotspeed", "10", CV_SAVE|CV_ALLOWLUA, rotation_cons_t, NULL);
consvar_t cv_cam2_turnmultiplier = CVAR_INIT ("cam2_turnmultiplier", "0.75", CV_FLOAT|CV_SAVE|CV_ALLOWLUA, multiplier_cons_t, NULL);
consvar_t cv_cam2_orbit = CVAR_INIT ("cam2_orbit", "Off", CV_SAVE|CV_ALLOWLUA, CV_OnOff, NULL);
consvar_t cv_cam2_adjust = CVAR_INIT ("cam2_adjust", "On", CV_SAVE|CV_ALLOWLUA, CV_OnOff, NULL);


consvar_t cv_cam_savedist[2][2] = {
	{  
		CVAR_INIT ("cam_dist", "192", CV_FLOAT|CV_SAVE|CV_CALL|CV_ALLOWLUA, campos_cons_t, CV_UpdateCamDist),
		CVAR_INIT ("cam2_dist", "192", CV_FLOAT|CV_SAVE|CV_CALL|CV_ALLOWLUA, campos_cons_t, CV_UpdateCam2Dist),
	},
	{  
		CVAR_INIT ("cam_simpledist", "256", CV_FLOAT|CV_SAVE|CV_CALL|CV_ALLOWLUA, campos_cons_t, CV_UpdateCamDist),
		CVAR_INIT ("cam2_simpledist", "256", CV_FLOAT|CV_SAVE|CV_CALL|CV_ALLOWLUA, campos_cons_t, CV_UpdateCam2Dist),

	}
};
consvar_t cv_cam_saveheight[2][2] = {
	{  
		CVAR_INIT ("cam_height", "40", CV_FLOAT|CV_SAVE|CV_CALL|CV_ALLOWLUA, campos_cons_t, CV_UpdateCamDist),
		CVAR_INIT ("cam2_height", "40", CV_FLOAT|CV_SAVE|CV_CALL|CV_ALLOWLUA, campos_cons_t, CV_UpdateCam2Dist),
	},
	{  
		CVAR_INIT ("cam_simpleheight", "60", CV_FLOAT|CV_SAVE|CV_CALL|CV_ALLOWLUA, campos_cons_t, CV_UpdateCamDist),
		CVAR_INIT ("cam2_simpleheight", "60", CV_FLOAT|CV_SAVE|CV_CALL|CV_ALLOWLUA, campos_cons_t, CV_UpdateCam2Dist),

	}
};

void CV_UpdateCamDist(void)
{
	CV_Set(&cv_cam_dist, va("%f", FIXED_TO_FLOAT(cv_cam_savedist[cv_useranalog[0].value][0].value)));
	CV_Set(&cv_cam_height, va("%f", FIXED_TO_FLOAT(cv_cam_saveheight[cv_useranalog[0].value][0].value)));
}

void CV_UpdateCam2Dist(void)
{
	CV_Set(&cv_cam2_dist, va("%f", FIXED_TO_FLOAT(cv_cam_savedist[cv_useranalog[1].value][1].value)));
	CV_Set(&cv_cam2_height, va("%f", FIXED_TO_FLOAT(cv_cam_saveheight[cv_useranalog[1].value][1].value)));
}

fixed_t t_cam_dist = -42;
fixed_t t_cam_height = -42;
fixed_t t_cam_rotate = -42;
fixed_t t_cam2_dist = -42;
fixed_t t_cam2_height = -42;
fixed_t t_cam2_rotate = -42;

#define MAXCAMERADIST 140*FRACUNIT  

void P_ResetCamera(player_t *player, camera_t *thiscam)
{
	tic_t tries = 0;
	fixed_t x, y, z;

	if (!player->mo)
		return;

	if (thiscam->chase && player->mo->health <= 0)
		return;

	thiscam->chase = true;
	x = player->mo->x - P_ReturnThrustX(player->mo, thiscam->angle, player->mo->radius);
	y = player->mo->y - P_ReturnThrustY(player->mo, thiscam->angle, player->mo->radius);
	if (player->mo->eflags & MFE_VERTICALFLIP)
		z = player->mo->z + player->mo->height - (41*player->height/48) - 16*FRACUNIT;
	else
		z = player->mo->z + (41*player->height/48);


	thiscam->x = x;
	thiscam->y = y;
	thiscam->z = z;
	thiscam->reset = true;

	if ((thiscam == &camera && G_ControlStyle(1) == CS_SIMPLE)
	|| (thiscam == &camera2 && G_ControlStyle(2) == CS_SIMPLE))
	{
		thiscam->angle = P_GetLocalAngle(player);
		thiscam->aiming = (thiscam == &camera) ? localaiming : localaiming2;
	}
	else if (!(thiscam == &camera && (cv_cam_still.value || cv_analog[0].value))
	&& !(thiscam == &camera2 && (cv_cam2_still.value || cv_analog[1].value)))
	{
		thiscam->angle = player->mo->angle;
		thiscam->aiming = 0;
	}
	thiscam->relativex = 0;

	thiscam->subsector = R_PointInSubsector(thiscam->x,thiscam->y);

	thiscam->radius = 20*FRACUNIT;
	thiscam->height = 16*FRACUNIT;

	while (!P_MoveChaseCamera(player,thiscam,true) && ++tries < 2*TICRATE);
}

boolean P_MoveChaseCamera(player_t *player, camera_t *thiscam, boolean resetcalled)
{
	angle_t angle = 0, focusangle = 0, focusaiming = 0;
	fixed_t x, y, z, dist, distxy, distz, checkdist, viewpointx, viewpointy, camspeed, camdist, camheight, pviewheight, slopez = 0;
	INT32 camrotate;
	boolean camstill, cameranoclip, camorbit;
	mobj_t *mo, *sign = NULL;
	subsector_t *newsubsec;
	fixed_t f1, f2;

	static fixed_t camsideshift[2] = {0, 0};
	fixed_t shiftx = 0, shifty = 0;


	if (!player || !player->mo)
		return true;

	mo = player->mo;

	if (player->playerstate == PST_REBORN)
	{
		P_CalcChasePostImg(player, thiscam);
		return true;
	}

	if (player->exiting)
	{
		if (mo->target && mo->target->type == MT_SIGN && mo->target->spawnpoint
		&& !((gametyperules & GTR_FRIENDLY) && (netgame || multiplayer) && cv_exitmove.value)
		&& !(twodlevel || (mo->flags2 & MF2_TWOD)))
			sign = mo->target;
		else if ((player->powers[pw_carry] == CR_NIGHTSMODE)
		&& !(player->mo->state >= &states[S_PLAY_NIGHTS_TRANS1]
		&& player->mo->state <= &states[S_PLAY_NIGHTS_TRANS6]))
		{
			P_CalcChasePostImg(player, thiscam);
			return true;
		}
	}

	cameranoclip = (sign || player->powers[pw_carry] == CR_NIGHTSMODE || player->pflags & PF_NOCLIP) || (mo->flags & (MF_NOCLIP|MF_NOCLIPHEIGHT));  

	if (!(player->climbing || (player->powers[pw_carry] == CR_NIGHTSMODE) || player->playerstate == PST_DEAD || tutorialmode))
	{
		if (player->spectator)  
			return true;

		if (!cv_chasecam.value && thiscam == &camera)
			return true;

		if (!cv_chasecam2.value && thiscam == &camera2)
			return true;
	}

	if (!thiscam->chase && !resetcalled)
	{
		if (player == &players[consoleplayer])
			focusangle = localangle;
		else if (player == &players[secondarydisplayplayer])
			focusangle = localangle2;
		else
			focusangle = mo->angle;
		if (thiscam == &camera)
			camrotate = cv_cam_rotate.value;
		else if (thiscam == &camera2)
			camrotate = cv_cam2_rotate.value;
		else
			camrotate = 0;
		thiscam->angle = focusangle + FixedAngle(camrotate*FRACUNIT);
		P_ResetCamera(player, thiscam);
		return true;
	}

	thiscam->radius = FixedMul(20*FRACUNIT, mo->scale);
	thiscam->height = FixedMul(16*FRACUNIT, mo->scale);






	if (player->powers[pw_carry] == CR_NIGHTSMODE)
	{
		focusangle = mo->angle;
		focusaiming = 0;
	}
	else if (sign)
	{
		focusangle = FixedAngle(sign->spawnpoint->angle << FRACBITS) + ANGLE_180;
		focusaiming = 0;
	}
	else if (player == &players[consoleplayer])
	{
		focusangle = localangle;
		focusaiming = localaiming;
	}
	else if (player == &players[secondarydisplayplayer])
	{
		focusangle = localangle2;
		focusaiming = localaiming2;
	}
	else
	{
		focusangle = player->cmd.angleturn << 16;
		focusaiming = player->aiming;
	}

	if (P_CameraThinker(player, thiscam, resetcalled))
		return true;

	if (thiscam == &camera)
	{
		camspeed = cv_cam_speed.value;
		camstill = cv_cam_still.value;
		camorbit = cv_cam_orbit.value;
		camrotate = cv_cam_rotate.value;
		camdist = FixedMul(cv_cam_dist.value, mo->scale);
		camheight = FixedMul(cv_cam_height.value, mo->scale);
	}
	else  
	{
		camspeed = cv_cam2_speed.value;
		camstill = cv_cam2_still.value;
		camorbit = cv_cam2_orbit.value;
		camrotate = cv_cam2_rotate.value;
		camdist = FixedMul(cv_cam2_dist.value, mo->scale);
		camheight = FixedMul(cv_cam2_height.value, mo->scale);
	}

	                                                                           
                                                                              
                                                                            
                                                                             
                                                           
	if (resetcalled)
		camspeed = FRACUNIT;

	if (!(twodlevel || (mo->flags2 & MF2_TWOD)) && !(player->powers[pw_carry] == CR_NIGHTSMODE))
		camheight = FixedMul(camheight, player->camerascale);

#ifdef REDSANALOG
	if (P_ControlStyle(player) == CS_LMAOGALOG && (player->cmd.buttons & (BT_CAMLEFT|BT_CAMRIGHT)) == (BT_CAMLEFT|BT_CAMRIGHT)) {
		camstill = true;

		if (camspeed < 4*FRACUNIT/5)
			camspeed = 4*FRACUNIT/5;
	}
#endif  

	if (mo->eflags & MFE_VERTICALFLIP)
		camheight += thiscam->height;

	if (twodlevel || (mo->flags2 & MF2_TWOD))
		angle = ANGLE_90;
	else if (camstill || resetcalled || player->playerstate == PST_DEAD)
		angle = thiscam->angle;
	else if (player->powers[pw_carry] == CR_NIGHTSMODE)  
	{
		if ((player->pflags & PF_TRANSFERTOCLOSEST) && player->axis1 && player->axis2)
		{
			angle = R_PointToAngle2(player->axis1->x, player->axis1->y, player->axis2->x, player->axis2->y);
			angle += ANGLE_90;
		}
		else if (mo->target)
		{
			if (mo->target->flags2 & MF2_AMBUSH)
				angle = R_PointToAngle2(mo->target->x, mo->target->y, mo->x, mo->y);
			else
				angle = R_PointToAngle2(mo->x, mo->y, mo->target->x, mo->target->y);
		}
	}
	else if (P_ControlStyle(player) == CS_LMAOGALOG && !sign)  
		angle = R_PointToAngle2(thiscam->x, thiscam->y, mo->x, mo->y);
	else if (demoplayback)
	{
		angle = focusangle;
		focusangle = R_PointToAngle2(thiscam->x, thiscam->y, mo->x, mo->y);
		if (player == &players[consoleplayer])
		{
			if (focusangle >= localangle)
				P_ForceLocalAngle(player, localangle + (abs((signed)(focusangle - localangle))>>5));
			else
				P_ForceLocalAngle(player, localangle - (abs((signed)(focusangle - localangle))>>5));
		}
	}
	else
		angle = focusangle + FixedAngle(camrotate*FRACUNIT);

	if (!resetcalled && (cv_analog[0].value || demoplayback) && ((thiscam == &camera && t_cam_rotate != -42) || (thiscam == &camera2
		&& t_cam2_rotate != -42)))
	{
		angle = FixedAngle(camrotate*FRACUNIT);
		thiscam->angle = angle;
	}

	if ((((thiscam == &camera) && cv_analog[0].value) || ((thiscam != &camera) && cv_analog[1].value) || demoplayback) && !sign && !objectplacing && !(twodlevel || (mo->flags2 & MF2_TWOD)) && (player->powers[pw_carry] != CR_NIGHTSMODE) && displayplayer == consoleplayer)
	{
#ifdef REDSANALOG
		if ((player->cmd.buttons & (BT_CAMLEFT|BT_CAMRIGHT)) == (BT_CAMLEFT|BT_CAMRIGHT)); else
#endif
		if (player->cmd.buttons & BT_CAMRIGHT)
		{
			if (thiscam == &camera)
				angle -= FixedAngle(cv_cam_rotspeed.value*FRACUNIT);
			else
				angle -= FixedAngle(cv_cam2_rotspeed.value*FRACUNIT);
		}
		else if (player->cmd.buttons & BT_CAMLEFT)
		{
			if (thiscam == &camera)
				angle += FixedAngle(cv_cam_rotspeed.value*FRACUNIT);
			else
				angle += FixedAngle(cv_cam2_rotspeed.value*FRACUNIT);
		}
	}

	if (G_ControlStyle((thiscam == &camera) ? 1 : 2) == CS_SIMPLE && !sign)
	{

		UINT8 forplayer = (thiscam == &camera) ? 0 : 1;
		fixed_t shift = FixedMul(FINESINE((player->mo->angle - angle) >> ANGLETOFINESHIFT), cv_cam_shiftfacing[forplayer].value);

		if (player->powers[pw_carry] == CR_NIGHTSMODE)
		{
			fixed_t cos = FINECOSINE((angle_t) (player->flyangle * ANG1)>>ANGLETOFINESHIFT);
			shift = FixedMul(shift, min(FRACUNIT, player->speed*abs(cos)/6000));
			shift += FixedMul(camsideshift[forplayer] - shift, FRACUNIT-(camspeed>>2));
		}
		else if (ticcmd_centerviewdown[(thiscam == &camera) ? 0 : 1])
			shift = FixedMul(camsideshift[forplayer], FRACUNIT-camspeed);
		else
			shift += FixedMul(camsideshift[forplayer] - shift, FRACUNIT-(camspeed>>3));
		camsideshift[forplayer] = shift;

		shift = FixedMul(shift, camdist);
		shiftx = -FixedMul(FINESINE(angle>>ANGLETOFINESHIFT), shift);
		shifty = FixedMul(FINECOSINE(angle>>ANGLETOFINESHIFT), shift);
	}


	if (twodlevel || (mo->flags2 & MF2_TWOD))
		dist = 480<<FRACBITS;
	else if (player->powers[pw_carry] == CR_NIGHTSMODE
		|| ((maptol & TOL_NIGHTS) && player->capsule && player->capsule->reactiontime > 0 && player == &players[player->capsule->reactiontime-1]))
		dist = 320<<FRACBITS;
	else
	{
		dist = camdist;

		if (sign)  
		{
			camheight = mo->scale << 7;
			camspeed = FRACUNIT/12;
		}
		else if (P_ControlStyle(player) == CS_LMAOGALOG)  
		{
			dist = FixedMul(dist, 6*FRACUNIT/5);
			camheight = FixedMul(camheight, 6*FRACUNIT/5);
		}

		if (player->climbing || player->exiting || player->playerstate == PST_DEAD || (player->powers[pw_carry] == CR_ROPEHANG || player->powers[pw_carry] == CR_GENERIC || player->powers[pw_carry] == CR_MACESPIN))
			dist <<= 1;
	}

	if (!sign && !(twodlevel || (mo->flags2 & MF2_TWOD)) && !(player->powers[pw_carry] == CR_NIGHTSMODE))
		dist = FixedMul(dist, player->camerascale);

	checkdist = dist;

	if (checkdist < 128*FRACUNIT)
		checkdist = 128*FRACUNIT;

	if (!(twodlevel || (mo->flags2 & MF2_TWOD)) && !(player->powers[pw_carry] == CR_NIGHTSMODE))  
	{
		if (!resetcalled && ((thiscam == &camera && cv_cam_adjust.value) || (thiscam == &camera2 && cv_cam2_adjust.value)))
		{
			if (!(mo->eflags & MFE_JUSTHITFLOOR) && (P_IsObjectOnGround(mo))  
			&& thiscam->ceilingz - thiscam->floorz >= P_GetPlayerHeight(player))  
			{
				if (mo->eflags & MFE_VERTICALFLIP)  
				{

					slopez += min(thiscam->ceilingz - mo->z, 0);  
				}
				else  
				{

					slopez += max(thiscam->floorz - mo->z - mo->height, 0);  
				}
			}
		}
	}

	if (camorbit)  
	{
#ifdef HWRENDER
		if (rendermode == render_opengl && !cv_glshearing.value)
			distxy = FixedMul(dist, FINECOSINE((focusaiming>>ANGLETOFINESHIFT) & FINEMASK));
		else
#endif
			distxy = dist;
		distz = -FixedMul(dist, FINESINE((focusaiming>>ANGLETOFINESHIFT) & FINEMASK)) + slopez;
	}
	else
	{
		distxy = dist;
		distz = slopez;
	}

	if (sign)
	{
		x = sign->x - FixedMul(FINECOSINE((angle>>ANGLETOFINESHIFT) & FINEMASK), distxy);
		y = sign->y - FixedMul(FINESINE((angle>>ANGLETOFINESHIFT) & FINEMASK), distxy);
	}
	else
	{
		x = mo->x - FixedMul(FINECOSINE((angle>>ANGLETOFINESHIFT) & FINEMASK), distxy);
		y = mo->y - FixedMul(FINESINE((angle>>ANGLETOFINESHIFT) & FINEMASK), distxy);
	}

#if 0
	if (twodlevel || (mo->flags2 & MF2_TWOD))
	{

		if (abs(mo->momx) > 10)
		{

			if (mo->momx > 0)
			{
				if (thiscam->relativex < MAXCAMERADIST)
					thiscam->relativex += 4*FRACUNIT;
			}
			else if (mo->momx < 0)
			{
				if (thiscam->relativex > -MAXCAMERADIST)
					thiscam->relativex -= 4*FRACUNIT;
			}
		}
		else  
		{
			if (thiscam->relativex > 0)
				thiscam->relativex -= 4*FRACUNIT;
			else if (thiscam->relativex < 0)
				thiscam->relativex += 4*FRACUNIT;
		}


		x += thiscam->relativex;
		y += mo->momy << 1;
	}
#endif  

	pviewheight = FixedMul(41*player->height/48, mo->scale);

	if (sign)
	{
		if (sign->eflags & MFE_VERTICALFLIP)
			z = sign->ceilingz - pviewheight - camheight;
		else
			z = sign->floorz + pviewheight + camheight;
	}
	else
	{
		if (mo->eflags & MFE_VERTICALFLIP)
			z = mo->z + mo->height - pviewheight - camheight + distz;
		else
			z = mo->z + pviewheight + camheight + distz;
	}


	newsubsec = R_PointInSubsectorOrNull(((mo->x>>FRACBITS) + (thiscam->x>>FRACBITS))<<(FRACBITS-1), ((mo->y>>FRACBITS) + (thiscam->y>>FRACBITS))<<(FRACBITS-1));

	if (!newsubsec)
		newsubsec = thiscam->subsector;

	if (newsubsec)
	{
		fixed_t myfloorz, myceilingz;
		fixed_t midz = thiscam->z + (thiscam->z - mo->z)/2;
		fixed_t midx = ((mo->x>>FRACBITS) + (thiscam->x>>FRACBITS))<<(FRACBITS-1);
		fixed_t midy = ((mo->y>>FRACBITS) + (thiscam->y>>FRACBITS))<<(FRACBITS-1);



		if (newsubsec->sector->camsec >= 0)
		{
			myfloorz = sectors[newsubsec->sector->camsec].floorheight;
			myceilingz = sectors[newsubsec->sector->camsec].ceilingheight;
		}
		else if (newsubsec->sector->heightsec >= 0)
		{
			myfloorz = sectors[newsubsec->sector->heightsec].floorheight;
			myceilingz = sectors[newsubsec->sector->heightsec].ceilingheight;
		}
		else
		{
			myfloorz = P_CameraGetFloorZ(thiscam, newsubsec->sector, midx, midy, NULL);
			myceilingz = P_CameraGetCeilingZ(thiscam, newsubsec->sector, midx, midy, NULL);
		}


		if (newsubsec->sector->ffloors)
		{
			ffloor_t *rover;
			fixed_t delta1, delta2;
			INT32 thingtop = midz + thiscam->height;

			for (rover = newsubsec->sector->ffloors; rover; rover = rover->next)
			{
				fixed_t topheight, bottomheight;
				if (!(rover->fofflags & FOF_BLOCKOTHERS) || !(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_RENDERALL) || (rover->master->frontsector->flags & MSF_NOCLIPCAMERA))
					continue;

				topheight = P_CameraGetFOFTopZ(thiscam, newsubsec->sector, rover, midx, midy, NULL);
				bottomheight = P_CameraGetFOFBottomZ(thiscam, newsubsec->sector, rover, midx, midy, NULL);

				delta1 = midz - (bottomheight
					+ ((topheight - bottomheight)/2));
				delta2 = thingtop - (bottomheight
					+ ((topheight - bottomheight)/2));
				if (topheight > myfloorz && abs(delta1) < abs(delta2))
					myfloorz = topheight;
				if (bottomheight < myceilingz && abs(delta1) >= abs(delta2))
					myceilingz = bottomheight;
			}
		}


	{
		INT32 xl, xh, yl, yh, bx, by;
		validcount++;

		xl = (unsigned)(tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
		xh = (unsigned)(tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
		yl = (unsigned)(tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
		yh = (unsigned)(tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

		BMBOUNDFIX(xl, xh, yl, yh);

		for (by = yl; by <= yh; by++)
			for (bx = xl; bx <= xh; bx++)
			{
				INT32 offset;
				polymaplink_t *plink;  

				if (bx < 0 || by < 0 || bx >= bmapwidth || by >= bmapheight)
					continue;

				offset = by*bmapwidth + bx;


				plink = polyblocklinks[offset];

				while (plink)
				{
					polyobj_t *po = plink->po;

					if (po->validcount != validcount)  
					{
						sector_t *polysec;
						fixed_t delta1, delta2, thingtop;
						fixed_t polytop, polybottom;

						po->validcount = validcount;

						if (!P_PointInsidePolyobj(po, x, y) || !(po->flags & POF_SOLID))
						{
							plink = (polymaplink_t *)(plink->link.next);
							continue;
						}


						polysec = po->lines[0]->backsector;

						if (polysec->flags & MSF_NOCLIPCAMERA)
						{  
							plink = (polymaplink_t *)(plink->link.next);
							continue;
						}

						if (po->flags & POF_CLIPPLANES)
						{
							polytop = polysec->ceilingheight;
							polybottom = polysec->floorheight;
						}
						else
						{
							polytop = INT32_MAX;
							polybottom = INT32_MIN;
						}

						thingtop = midz + thiscam->height;
						delta1 = midz - (polybottom + ((polytop - polybottom)/2));
						delta2 = thingtop - (polybottom + ((polytop - polybottom)/2));

						if (polytop > myfloorz && abs(delta1) < abs(delta2))
							myfloorz = polytop;

						if (polybottom < myceilingz && abs(delta1) >= abs(delta2))
							myceilingz = polybottom;
					}
					plink = (polymaplink_t *)(plink->link.next);
				}
			}
	}


		if (myceilingz <= myfloorz + thiscam->height && !resetcalled && !cameranoclip)
		{
			P_ResetCamera(player, thiscam);
			return true;
		}


		if (myceilingz != myfloorz
			&& myceilingz - thiscam->height < z)
		{
              
                                     
    
                                   
                
    
  
			z = myceilingz - thiscam->height-FixedMul(11*FRACUNIT, mo->scale);

		}


		if (newsubsec->sector->ffloors && !cameranoclip)
		{
			ffloor_t *rover;

			for (rover = newsubsec->sector->ffloors; rover; rover = rover->next)
			{
				fixed_t topheight, bottomheight;
				if ((rover->fofflags & FOF_BLOCKOTHERS) && (rover->fofflags & FOF_RENDERALL) && (rover->fofflags & FOF_EXISTS) && !(rover->master->frontsector->flags & MSF_NOCLIPCAMERA))
				{
					topheight = P_CameraGetFOFTopZ(thiscam, newsubsec->sector, rover, midx, midy, NULL);
					bottomheight = P_CameraGetFOFBottomZ(thiscam, newsubsec->sector, rover, midx, midy, NULL);

					if (bottomheight - thiscam->height < z
						&& midz < bottomheight)
						z = bottomheight - thiscam->height-FixedMul(11*FRACUNIT, mo->scale);

					else if (topheight + thiscam->height > z
						&& midz > topheight)
						z = topheight;

					if ((mo->z >= topheight && midz < bottomheight)
						|| ((mo->z < bottomheight && mo->z+mo->height < topheight) && midz >= topheight))
					{

						if (!resetcalled)
							P_ResetCamera(player, thiscam);
						return true;
					}
				}
			}
		}
	}

	if (mo->type == MT_EGGTRAP)
		z = mo->z + 128*FRACUNIT + pviewheight + camheight;

	if (thiscam->z < thiscam->floorz && !cameranoclip)
		thiscam->z = thiscam->floorz;



	dist = FixedMul(64 << FRACBITS, mo->scale);
	if (sign)
	{
		viewpointx = sign->x + FixedMul(FINECOSINE((angle>>ANGLETOFINESHIFT) & FINEMASK), dist);
		viewpointy = sign->y + FixedMul(FINESINE((angle>>ANGLETOFINESHIFT) & FINEMASK), dist);
	}
	else
	{
		viewpointx = mo->x + shiftx + FixedMul(FINECOSINE((angle>>ANGLETOFINESHIFT) & FINEMASK), dist);
		viewpointy = mo->y + shifty + FixedMul(FINESINE((angle>>ANGLETOFINESHIFT) & FINEMASK), dist);
	}

	if (!camstill && !resetcalled && !paused)
		thiscam->angle = R_PointToAngle2(thiscam->x, thiscam->y, viewpointx, viewpointy);

  
                                          
                         
  

	                                                         
  
                                                                                                                     
                                              
   
                    
                                   
               
   
    

	                                                                           
                                                                          
                                                                             
                                                                            
                                                                         
                                                                         
                                       
	if (!resetcalled && !sign && camspeed > 0 && camspeed < FRACUNIT/5)
	{
		fixed_t chaseerror = P_AproxDistance(x - thiscam->x, y - thiscam->y);

		if ((INT64)chaseerror > (INT64)checkdist * 4)
			camspeed = 3*FRACUNIT/4;
		else if ((INT64)chaseerror > (INT64)checkdist * 2)
			camspeed = FRACUNIT/2;
	}

	if (twodlevel || (mo->flags2 & MF2_TWOD))
	{
		thiscam->momx = x-thiscam->x;
		thiscam->momy = y-thiscam->y;
		thiscam->momz = z-thiscam->z;
	}
	else
	{
		thiscam->momx = FixedMul(x - thiscam->x, camspeed);
		thiscam->momy = FixedMul(y - thiscam->y, camspeed);

		if (thiscam->subsector->sector->damagetype == SD_DEATHPITTILT
			&& thiscam->z < thiscam->subsector->sector->floorheight + 256*FRACUNIT
			&& FixedMul(z - thiscam->z, camspeed) < 0)
		{
			thiscam->momz = 0;  
		}
		else
			thiscam->momz = FixedMul(z - thiscam->z, camspeed);

		thiscam->momx += FixedMul(shiftx, camspeed);
		thiscam->momy += FixedMul(shifty, camspeed);
	}


	f1 = viewpointx-thiscam->x;
	f2 = viewpointy-thiscam->y;
	dist = FixedHypot(f1, f2);

	if (mo->eflags & MFE_VERTICALFLIP)
		angle = R_PointToAngle2(0, thiscam->z + thiscam->height, dist, (sign ? sign->ceilingz : mo->z + mo->height) - P_GetPlayerHeight(player));
	else
		angle = R_PointToAngle2(0, thiscam->z, dist, (sign ? sign->floorz : mo->z) + P_GetPlayerHeight(player));
	if (player->playerstate != PST_DEAD)
		angle += (focusaiming < ANGLE_180 ? focusaiming/2 : InvAngle(InvAngle(focusaiming)/2));  

	if (twodlevel || (mo->flags2 & MF2_TWOD) || !camstill)  
	{
		G_ClipAimingPitch((INT32 *)&angle);
		if (resetcalled)
			thiscam->aiming = angle;
		else
		{
			dist = thiscam->aiming - angle;
			thiscam->aiming -= (dist>>3);
		}
	}


	if (!(multiplayer || netgame) && !splitscreen)
	{
		fixed_t vx = thiscam->x, vy = thiscam->y;
		fixed_t vz = thiscam->z + thiscam->height / 2;
		if (player->awayviewtics && player->awayviewmobj != NULL && !P_MobjWasRemoved(player->awayviewmobj))		 
		{
			vx = player->awayviewmobj->x;
			vy = player->awayviewmobj->y;
			vz = player->awayviewmobj->z + player->awayviewmobj->height / 2;
		}

		 
		if (P_AproxDistance(P_AproxDistance(vx - mo->x, vy - mo->y),
					vz - ( mo->z + mo->height / 2 )) < FixedMul(48*FRACUNIT, mo->scale))
			mo->flags2 |= MF2_SHADOW;
		else
			mo->flags2 &= ~MF2_SHADOW;
	}
	else
		mo->flags2 &= ~MF2_SHADOW;

                                                                                      
  
                                              
                    

                                                 
                                                                                                  
                               
                                                                                                         
                              
  
        if (!resetcalled && (player->playerstate == PST_DEAD || player->playerstate == PST_REBORN))
	{

		thiscam->momz = 0;


		if (!(mo->eflags & MFE_VERTICALFLIP) && thiscam->aiming < ANGLE_337h && thiscam->aiming > ANGLE_180)
			thiscam->aiming = ANGLE_337h;
		else if (mo->eflags & MFE_VERTICALFLIP && thiscam->aiming > ANGLE_22h && thiscam->aiming < ANGLE_180)
			thiscam->aiming = ANGLE_22h;
	}

	                                                                         
                                                                               
	return (x + shiftx == thiscam->x && y + shifty == thiscam->y
		&& z == thiscam->z && angle == thiscam->aiming);

}

boolean P_SpectatorJoinGame(player_t *player)
{
	if (!G_CoopGametype() && !cv_allowteamchange.value)
	{
		if (P_IsLocalPlayer(player))
			CONS_Printf(M_GetText("Server does not allow team change.\n"));
		player->powers[pw_flashing] += 2*TICRATE;  
	}




	else if (G_GametypeHasTeams())
	{
		INT32 changeto = 0;
		INT32 z, numplayersred = 0, numplayersblue = 0;


		for (z = 0; z < MAXPLAYERS; ++z)
			if (playeringame[z])
			{
				if (players[z].ctfteam == 1)
					++numplayersred;
				else if (players[z].ctfteam == 2)
					++numplayersblue;
			}


		if (numplayersblue > numplayersred)
			changeto = 1;
		else if (numplayersred > numplayersblue)
			changeto = 2;
		else if (bluescore > redscore)
			changeto = 1;
		else if (redscore > bluescore)
			changeto = 2;
		else
			changeto = (P_RandomFixed() & 1) + 1;

		if (!LUA_HookTeamSwitch(player, changeto, true, false, false))
			return false;

		if (player->mo)
		{
			P_RemoveMobj(player->mo);
			player->mo = NULL;
		}
		player->spectator = false;
		player->ctfteam = changeto;
		player->playerstate = PST_REBORN;


		if (P_IsLocalPlayer(player) && displayplayer != consoleplayer)
		{


			LUA_HookViewpointSwitch(player, &players[consoleplayer], true);
			displayplayer = consoleplayer;
		}

		if (changeto == 1)
			CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[player-players], '\x85', M_GetText("Red team"), '\x80');
		else if (changeto == 2)
			CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[player-players], '\x84', M_GetText("Blue team"), '\x80');

		return true;  
	}

	else
	{


		if (!((gametyperules & GTR_HIDEFROZEN) && leveltime > (hidetime * TICRATE)))
		{
			if (!LUA_HookTeamSwitch(player, 3, true, false, false))
				return false;
			if (player->mo)
			{
				P_RemoveMobj(player->mo);
				player->mo = NULL;
			}
			player->spectator = player->outofcoop = false;
			player->playerstate = PST_REBORN;

			if ((gametyperules & (GTR_TAG|GTR_HIDEFROZEN)) == GTR_TAG)
			{

				if (leveltime > (hidetime * TICRATE))
				{
					CONS_Printf(M_GetText("%s is now IT!\n"), player_names[player-players]);  
					player->pflags |= PF_TAGIT;
				}

				P_CheckSurvivors();
			}


			if (P_IsLocalPlayer(player) && displayplayer != consoleplayer)
			{


				LUA_HookViewpointSwitch(player, &players[consoleplayer], true);
				displayplayer = consoleplayer;
			}

			if (!G_CoopGametype())
				CONS_Printf(M_GetText("%s entered the game.\n"), player_names[player-players]);
			return true;  
		}
		else
		{
			if (P_IsLocalPlayer(player))
				CONS_Printf(M_GetText("You must wait until next round to enter the game.\n"));
			player->powers[pw_flashing] += 2*TICRATE;  
		}
	}
	return false;
}


static void P_CalcPostImg(player_t *player)
{
	sector_t *sector = player->mo->subsector->sector;
	postimg_t *type;
	INT32 *param;
	fixed_t pviewheight;

	if (player->mo->eflags & MFE_VERTICALFLIP)
		pviewheight = player->mo->z + player->mo->height - player->viewheight;
	else
		pviewheight = player->mo->z + player->viewheight;

	if (player->awayviewtics && player->awayviewmobj && !P_MobjWasRemoved(player->awayviewmobj))
	{
		sector = player->awayviewmobj->subsector->sector;
		pviewheight = player->awayviewmobj->z + 20*FRACUNIT;
	}

	if (splitscreen && player == &players[secondarydisplayplayer])
	{
		type = &postimgtype2;
		param = &postimgparam2;
	}
	else
	{
		type = &postimgtype;
		param = &postimgparam;
	}


	if (sector->flags & MSF_HEATWAVE)
		*type = postimg_heat;
	else if (sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t topheight;
		fixed_t bottomheight;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS))
				continue;

			topheight    = P_GetFFloorTopZAt   (rover, player->mo->x, player->mo->y);
			bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);

			if (pviewheight >= topheight || pviewheight <= bottomheight)
				continue;

			if (rover->master->frontsector->flags & MSF_HEATWAVE)
			{
				*type = postimg_heat;
				break;
			}
		}
	}


	if (sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t topheight;
		fixed_t bottomheight;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_SWIMMABLE) || rover->fofflags & FOF_BLOCKPLAYER)
				continue;

			topheight    = P_GetFFloorTopZAt   (rover, player->mo->x, player->mo->y);
			bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);

			if (pviewheight >= topheight || pviewheight <= bottomheight)
				continue;

			*type = postimg_water;
		}
	}

	if (player->mo->eflags & MFE_VERTICALFLIP)
		*type = postimg_flip;

#if 1
	(void)param;
#else

	if (player->speed > (35<<FRACBITS))
	{
		*type = postimg_motion;
		*param = (player->speed - 32)/4;

		if (*param > 5)
			*param = 5;
	}
#endif
}

void P_DoPityCheck(player_t *player)
{

	if (player->spectator
		|| !(gametyperules & GTR_PITYSHIELD))
		return;


	if ((player->pity >= 3 || player->pity < 0) && player->powers[pw_shield] == SH_NONE)
	{
		P_SwitchShield(player, SH_PITY);

		if (player->pity > 0)
			S_StartSound(player->mo, mobjinfo[MT_PITY_ICON].seesound);

		player->pity = 0;
	}
}


static sector_t *P_GetMinecartSector(fixed_t x, fixed_t y, fixed_t z, fixed_t *nz)
{
	sector_t *sec = R_PointInSubsector(x, y)->sector;

	if ((sec->ceilingheight - sec->floorheight) < 64*FRACUNIT)
		return NULL;

	if (sec->ffloors)
	{
		ffloor_t *rover;
		for (rover = sec->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & (FOF_EXISTS|FOF_BLOCKOTHERS)))
				continue;

			*nz = P_GetFFloorTopZAt(rover, x, y);
			if (abs(z - *nz) <= 56*FRACUNIT)
			{
				sec = &sectors[rover->secnum];
				return sec;
			}
		}

	}

	*nz = P_GetSectorFloorZAt(sec, x, y);
	if (abs(z - *nz) > 56*FRACUNIT)
		return NULL;

	return sec;
}

static INT32 P_GetMinecartSpecialLine(sector_t *sec)
{
	INT32 line = -1;
	size_t i;

	if (!sec)
		return line;

	for (i = 0; i < sec->tags.count; i++)
		if (sec->tags.tags[i] != 0)
			line = Tag_FindLineSpecial(16, sec->tags.tags[i]);


	for (i = 0; i < sec->linecount; i++)
	{
		line_t *li = sec->lines[i];
		if (Tag_Find(&li->tags, 0) && li->special == 16 && li->frontsector == sec)
			line = li - lines;
	}

	return line;
}


static mobj_t *P_GetAxis(INT32 num)
{
	thinker_t *th;
	mobj_t *mobj;

	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mobj = (mobj_t *)th;


		if (!(mobj->flags2 & MF2_AXIS))
			break;


		if (mobj->health != num)
			continue;

		return mobj;
	}

	CONS_Alert(CONS_WARNING, "P_GetAxis: Track segment %d is missing!\n", num);
	return NULL;
}


static void P_GetAxisPosition(fixed_t x, fixed_t y, mobj_t *amo, fixed_t *newx, fixed_t *newy, angle_t *targetangle, angle_t *grind)
{
	fixed_t ax = amo->x;
	fixed_t ay = amo->y;
	angle_t ang;
	angle_t gr = 0;

	if (amo->type == MT_AXISTRANSFERLINE)
	{
		ang = amo->angle;

		if (ang == ANGLE_90 || ang == ANGLE_270)  
			x = ax;
		else if (ang == 0 || ang == ANGLE_180)  
			y = ay;
		else  
		{
			fixed_t distance = R_PointToDist2(ax, ay, x, y);
			angle_t fad = ((R_PointToAngle2(ax, ay, x, y) - ang) >> ANGLETOFINESHIFT) & FINEMASK;
			fixed_t cosine = FINECOSINE(fad);
			angle_t fa = (ang >> ANGLETOFINESHIFT) & FINEMASK;
			distance = FixedMul(distance, cosine);
			x = ax + FixedMul(distance, FINECOSINE(fa));
			y = ay + FixedMul(distance, FINESINE(fa));
		}
	}
	else  
	{
		fixed_t rad = amo->radius;
		fixed_t distfactor = FixedDiv(rad, R_PointToDist2(ax, ay, x, y));

		gr = R_PointToAngle2(ax, ay, x, y);
		ang = gr + ANGLE_90;
		x = ax + FixedMul(x - ax, distfactor);
		y = ay + FixedMul(y - ay, distfactor);
	}

	*newx = x;
	*newy = y;
	*targetangle = ang;
	*grind = gr;
}

static void P_SpawnSparks(mobj_t *mo, angle_t maindir)
{
	angle_t fa = (mo->angle >> ANGLETOFINESHIFT) & FINEMASK;
	fixed_t c = FixedMul(FINECOSINE(fa), mo->radius);
	fixed_t s = FixedMul(FINESINE(fa), mo->radius);
	mobj_t *spark;
	SINT8 b1 = (leveltime & 1) ? 1 : -1;
	SINT8 b2 = (leveltime & 2) ? 1 : -1;
	fixed_t r1 = FRACUNIT*P_RandomRange(-1, 1);
	fixed_t r2 = FRACUNIT*P_RandomRange(-1, 1);
	fixed_t r3 = FRACUNIT*P_RandomRange(-1, 1);
	fixed_t fm = (maindir >> ANGLETOFINESHIFT) & FINEMASK;

	spark = P_SpawnMobj(mo->x - b2*s + b1*c, mo->y + b2*c + b1*s, mo->z, MT_MINECARTSPARK);
	if (P_MobjWasRemoved(spark))
		return;
	spark->momx = mo->momx + r1 + 8*FINECOSINE(fm);
	spark->momy = mo->momy + r2 + 8*FINESINE(fm);
	spark->momz = mo->momz + r3;

	P_Thrust(spark, R_PointToAngle2(mo->x, mo->y, spark->x, spark->y), 8*FRACUNIT);
	P_SetScale(spark, FRACUNIT/4, true);
	spark->fuse = TICRATE/3;
}


static mobj_t *P_LookForRails(mobj_t* mobj, fixed_t c, fixed_t s, angle_t targetangle, fixed_t xcom, fixed_t ycom)
{
	INT16 interval = 16;
	INT16 fwooffset = FixedHypot(mobj->momx, mobj->momy) >> FRACBITS;
	fixed_t x = mobj->x;
	fixed_t y = mobj->y;
	fixed_t z = mobj->z;
	UINT8 i;

	for (i = 4; i <= 10; i++)
	{
		fixed_t nz;
		INT32 lline;

		x += interval*xcom*i + fwooffset*c*i;
		y += interval*ycom*i + fwooffset*s*i;

		lline = P_GetMinecartSpecialLine(P_GetMinecartSector(x, y, z, &nz));
		if (lline != -1)
		{
			fixed_t nx, ny;
			angle_t nang, dummy, angdiff;
			mobj_t *mark;
			mobj_t *snax = P_GetAxis(lines[lline].args[0]);
			if (!snax)
				return NULL;
			P_GetAxisPosition(x, y, snax, &nx, &ny, &nang, &dummy);
			angdiff = ((nang - targetangle) + ANG10/2) & ~ANGLE_180;

			if (angdiff < ANG10)
			{
				mark = P_SpawnMobj(nx, ny, nz, (mobjtype_t)mobj->info->raisestate);
				return mark;
			}
		}
	}
	return NULL;
}

static void P_ParabolicMove(mobj_t *mo, fixed_t x, fixed_t y, fixed_t z, fixed_t g, fixed_t speed)
{
	fixed_t dx = x - mo->x;
	fixed_t dy = y - mo->y;
	fixed_t dz = z - mo->z;
	fixed_t dh = P_AproxDistance(dx, dy);
	fixed_t c = FixedDiv(dx, dh);
	fixed_t s = FixedDiv(dy, dh);
	fixed_t fixConst = FixedDiv(speed, g);

	mo->momx = FixedMul(c, speed);
	mo->momy = FixedMul(s, speed);
	mo->momz = FixedDiv(dh, 2*fixConst) + FixedDiv(dz, FixedDiv(dh, fixConst/2));
}

static void P_MinecartThink(player_t *player)
{
	mobj_t *minecart = player->mo->tracer;
	angle_t fa;

	if (!minecart || P_MobjWasRemoved(minecart) || !minecart->health)
	{

		P_KillMobj(player->mo, NULL, NULL, 0);
		return;
	}


#define MINECARTCONEMAX FixedAngle(20*FRACUNIT)
	{
		angle_t angdiff = player->mo->angle - minecart->angle;
		if (angdiff < ANGLE_180 && angdiff > MINECARTCONEMAX)
			player->mo->angle = minecart->angle + MINECARTCONEMAX;
		else if (angdiff > ANGLE_180 && angdiff < InvAngle(MINECARTCONEMAX))
			player->mo->angle = minecart->angle - MINECARTCONEMAX;

		if (angdiff + minecart->angle != player->mo->angle && (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG))
		{
			angdiff = P_GetLocalAngle(player) - minecart->angle;
			if (angdiff < ANGLE_180 && angdiff > MINECARTCONEMAX)
				P_SetLocalAngle(player, minecart->angle + MINECARTCONEMAX);
			else if (angdiff > ANGLE_180 && angdiff < InvAngle(MINECARTCONEMAX))
				P_SetLocalAngle(player, minecart->angle - MINECARTCONEMAX);


		}
	}


	if (!(player->cmd.buttons & BT_JUMP))
		player->pflags &= ~PF_JUMPDOWN;


	P_HandleMinecartSegments(minecart);


	minecart->friction = FRACUNIT;

	fa = (minecart->angle >> ANGLETOFINESHIFT) & FINEMASK;
	if (!P_TryMove(minecart, minecart->x + FINECOSINE(fa), minecart->y + FINESINE(fa), true))
	{
		if (!P_MobjWasRemoved(minecart))
			P_KillMobj(minecart, NULL, NULL, 0);
		return;
	}

	if (P_IsObjectOnGround(minecart))
	{
		sector_t *sec;
		INT32 lnum = -1;
		fixed_t dummy;


		if (minecart->eflags & MFE_JUSTHITFLOOR)
		{
			S_StopSound(minecart);
			S_StartSound(minecart, sfx_s3k96);
		}

		sec = P_GetMinecartSector(minecart->x, minecart->y, minecart->z, &dummy);

		if (sec)
			lnum = P_GetMinecartSpecialLine(sec);


		if (sec && lnum != -1)
		{
			mobj_t *axis = P_GetAxis(lines[lnum].args[0]);
			fixed_t newx, newy;
			angle_t targetangle, grind;
			angle_t prevangle, angdiff;
			mobj_t *detleft = NULL;
			mobj_t *detright = NULL;
			mobj_t *sidelock = NULL;
			boolean jumped = false;
			fixed_t currentSpeed;

			if (!axis)
			{
				P_KillMobj(minecart, NULL, NULL, 0);
				return;
			}

			minecart->movefactor = 0;
			P_ResetScore(player);

			P_GetAxisPosition(minecart->x, minecart->y, axis, &newx, &newy, &targetangle, &grind);
			if (axis->type != MT_AXISTRANSFERLINE)
				P_SpawnSparks(minecart, grind);
			P_TryMove(minecart, newx, newy, true);


			prevangle = minecart->angle;
			angdiff = targetangle - minecart->angle;
			if (angdiff < ANGLE_90 + ANG2 || angdiff > ANGLE_270 - ANG2)
				minecart->angle = targetangle;
			else
				minecart->angle = targetangle + ANGLE_180;
			angdiff = (minecart->angle - prevangle);
			if (angdiff && (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG))   
			{
				player->mo->angle += angdiff;
				P_SetPlayerAngle(player, (angle_t)(player->angleturn << 16) + angdiff);
			}


			if (minecart->flags2 & MF2_AMBUSH)
			{
				angle_t fa2 = (minecart->angle >> ANGLETOFINESHIFT) & FINEMASK;
				fixed_t c = FINECOSINE(fa2);
				fixed_t s = FINESINE(fa2);

				detleft = P_LookForRails(minecart, c, s, targetangle, -s, c);
				detright = P_LookForRails(minecart, c, s, targetangle, s, -c);
			}


			currentSpeed = FixedHypot(minecart->momx, minecart->momy);
			angdiff = R_PointToAngle2(0, 0, minecart->momx, minecart->momy) - minecart->angle;
			if (angdiff > ANGLE_90 && angdiff < ANGLE_270)
				currentSpeed *= -1;


			if (detleft && player->cmd.sidemove < 0)
				sidelock = detleft;
			else if (detright && player->cmd.sidemove > 0)
				sidelock = detright;





			if (sidelock || ((player->cmd.buttons & BT_JUMP) && !(player->pflags & PF_JUMPDOWN)))
			{
				player->pflags |= PF_JUMPDOWN;

				if (minecart->eflags & MFE_ONGROUND)
					minecart->eflags &= ~MFE_ONGROUND;
				minecart->z += P_MobjFlip(minecart);
				if (sidelock)
					P_ParabolicMove(minecart, sidelock->x, sidelock->y, sidelock->z, gravity, max(currentSpeed, 10 * FRACUNIT));
				else
					minecart->momz = 10 * FRACUNIT;

				S_StartSound(minecart, sfx_s3k51);
				jumped = true;
			}

			if (!jumped)
			{

				if (currentSpeed < minecart->info->speed)
					currentSpeed += FRACUNIT/4;

				if (minecart->standingslope)
				{
					fixed_t fa2 = (minecart->angle >> ANGLETOFINESHIFT) & FINEMASK;
					fixed_t front = P_GetSlopeZAt(minecart->standingslope, minecart->x, minecart->y);
					fixed_t back = P_GetSlopeZAt(minecart->standingslope, minecart->x - FINECOSINE(fa2), minecart->y - FINESINE(fa2));

					if (abs(front - back) < 3*FRACUNIT)
						currentSpeed += (back - front)/3;
				}


				P_InstaThrust(minecart, minecart->angle, currentSpeed);


				minecart->movecount += abs(currentSpeed);
				if (minecart->movecount > 128*FRACUNIT)
				{
					minecart->movecount %= 128*FRACUNIT;
					S_StartSound(minecart, minecart->info->activesound);
				}
			}




			if (detleft)
			{
				if (P_IsLocalPlayer(player))
				{
					detleft->old_x = detleft->x - (minecart->old_x - minecart->old_x2);
					detleft->old_y = detleft->y - (minecart->old_y - minecart->old_y2);
					detleft->old_z = detleft->z - (minecart->old_z - minecart->old_z2);
					detleft->drawonlyforplayer = player;  
				}
				else  
					P_RemoveMobj(detleft);  
			}
			if (detright)
			{
				if (P_IsLocalPlayer(player))
				{
					detright->old_x = detright->x - (minecart->old_x - minecart->old_x2);
					detright->old_y = detright->y - (minecart->old_y - minecart->old_y2);
					detright->old_z = detright->z - (minecart->old_z - minecart->old_z2);
					detright->drawonlyforplayer = player;
				}
				else
					P_RemoveMobj(detright);
			}
		}
		else
		{
			minecart->movefactor++;
			if ((P_IsObjectOnGround(minecart) && minecart->movefactor >= 5)  
			|| (abs(minecart->momx) < minecart->scale/2 && abs(minecart->momy) < minecart->scale/2))  
			{
				P_KillMobj(minecart, NULL, NULL, 0);
				return;
			}
		}
	}

	if (player->mo->state-states != S_PLAY_STND)
	{
		P_SetMobjState(player->mo, S_PLAY_STND);
		player->mo->tics = -1;
	}


	P_MoveOrigin(player->mo, minecart->x - minecart->momx, minecart->y - minecart->momy, minecart->z + max(minecart->momz, 0) + 8*FRACUNIT);
	if (player->powers[pw_carry] != CR_MINECART)
		return;
	player->mo->momx = player->mo->momy = player->mo->momz = 0;
	P_TryMove(player->mo, player->mo->x + minecart->momx, player->mo->y + minecart->momy, true);

	if (player->powers[pw_flashing] == flashingtics)
		player->powers[pw_flashing]--;
}


void P_DoTailsOverlay(player_t *player, mobj_t *tails)
{

	boolean smilesonground = P_IsObjectOnGround(player->mo);
	angle_t horizangle = player->drawangle;
	fixed_t zoffs = 0;
	fixed_t backwards = -1*FRACUNIT;
	boolean doswim = (player->panim == PA_ABILITY && (player->mo->eflags & MFE_UNDERWATER));
	boolean doroll = (player->panim == PA_ROLL || (player->panim == PA_JUMP && !(player->charflags & SF_NOJUMPSPIN)) || doswim);
	angle_t rollangle = 0;
	boolean panimchange;
	INT32 ticnum = 0;
	statenum_t chosenstate;

	if (!tails->skin)
	{
		tails->skin = player->mo->skin;
		P_SetMobjState(tails, S_TAILSOVERLAY_STAND);
		tails->movecount = -1;
	}

	panimchange = (tails->movecount != (INT32)player->panim);


	if (doroll)
	{
		fixed_t testval, zdist;
		if (player->speed < FRACUNIT)
			testval = FRACUNIT;
		else
		{
			testval = (FixedMul(player->speed, FINECOSINE((horizangle - R_PointToAngle2(0, 0, player->rmomx, player->rmomy)) >> ANGLETOFINESHIFT)));
			if (testval < FRACUNIT)
				testval = FRACUNIT;
		}

		if (doswim)
			zdist = player->mo->momz<<1;
		else if (smilesonground && !player->mo->reactiontime)
			zdist = (player->mo->z - tails->threshold);
		else
			zdist = player->mo->momz;

		rollangle = R_PointToAngle2(0, 0, testval, -P_MobjFlip(player->mo)*zdist);

		if (!doswim)
		{
			zoffs = 3*FRACUNIT + 12*FINESINE(rollangle >> ANGLETOFINESHIFT);
			backwards = -12*FINECOSINE(rollangle >> ANGLETOFINESHIFT);
		}
	}
	else if (player->panim == PA_RUN)
		backwards = -5*FRACUNIT;
	else if (player->panim == PA_SPRING || player->panim == PA_JUMP)
	{
		zoffs += 4*FRACUNIT;
		backwards /= 2;
	}
	else if (player->panim == PA_PAIN)
		backwards /= 16;
	else if (player->mo->state-states == S_PLAY_GASP)
	{
		backwards /= 16;
		zoffs += 12*FRACUNIT;
	}
	else if (player->mo->state-states == S_PLAY_EDGE)
	{
		backwards /= 16;
		zoffs = 3*FRACUNIT;
	}
	else if (player->panim == PA_ABILITY2)
	{
		zoffs = -7*FRACUNIT;
		backwards = -9*FRACUNIT;
	}
	else if (player->panim == PA_ABILITY)
		backwards = -5*FRACUNIT;


	if (doroll)
	{
		statenum_t add = ((rollangle > ANGLE_180) ? 2 : 0);
		if (add)
			rollangle = InvAngle(rollangle);
		rollangle += ANG15;  
		if (rollangle > ANG60)
			chosenstate = S_TAILSOVERLAY_PLUS60DEGREES + add;
		else if (rollangle > ANG30)
			chosenstate = S_TAILSOVERLAY_PLUS30DEGREES + add;
		else
			chosenstate = S_TAILSOVERLAY_0DEGREES;
	}
	else if (player->panim == PA_SPRING || player->panim == PA_JUMP)
		chosenstate = S_TAILSOVERLAY_MINUS60DEGREES;
	else if (player->panim == PA_FALL || player->mo->state-states == S_PLAY_RIDE)
		chosenstate = S_TAILSOVERLAY_PLUS60DEGREES;
	else if (player->panim == PA_PAIN)
		chosenstate = S_TAILSOVERLAY_PAIN;
	else if (player->mo->state-states == S_PLAY_GASP)
		chosenstate = S_TAILSOVERLAY_GASP;
	else if (player->mo->state-states == S_PLAY_EDGE)
		chosenstate = S_TAILSOVERLAY_EDGE;
	else if (player->panim == PA_DASH)
		chosenstate = S_TAILSOVERLAY_DASH;
	else if (player->panim == PA_RUN)
		chosenstate = S_TAILSOVERLAY_RUN;
	else if (player->panim == PA_WALK)
	{
		if (!smilesonground || player->mo->state-states == S_PLAY_SKID)
			chosenstate = S_TAILSOVERLAY_PLUS30DEGREES;
		else if (player->speed >= FixedMul(player->runspeed/2, player->mo->scale))
			chosenstate = S_TAILSOVERLAY_0DEGREES;
		else
			chosenstate = S_TAILSOVERLAY_MINUS30DEGREES;
	}
	else if (player->mo->sprite2 == SPR2_FLY)
		chosenstate = S_TAILSOVERLAY_FLY;
	else if (player->mo->sprite2 == SPR2_SWIM)
		chosenstate = S_TAILSOVERLAY_FLY;
	else if (player->mo->sprite2 == SPR2_TIRE)
		chosenstate = S_TAILSOVERLAY_TIRE;
	else if (player->panim == PA_ABILITY2)
		chosenstate = S_TAILSOVERLAY_PLUS30DEGREES;
	else if (player->panim == PA_IDLE)
		chosenstate = S_TAILSOVERLAY_STAND;
	else
		chosenstate = S_INVISIBLE;


	if (panimchange)
	{
		tails->sprite2 = -1;
		P_SetMobjState(tails, chosenstate);
	}
	else
	{
		if (tails->state != &states[chosenstate])
		{
			if (states[chosenstate].sprite == SPR_PLAY)
				tails->sprite2 = P_GetSkinSprite2(((skin_t *)tails->skin), P_GetStateSprite2(&states[chosenstate]), player);
			P_SetMobjState(tails, chosenstate);
		}
	}

#if 0
	if (player->fly1 != 0 && player->powers[pw_tailsfly] != 0 && !smilesonground)
		P_SetMobjState(tails, chosenstate);
#endif


	if (player->panim == PA_SPRING || player->panim == PA_FALL || player->mo->state-states == S_PLAY_RIDE)
	{
		if (FixedDiv(abs(player->mo->momz), player->mo->scale) < 20<<FRACBITS)
			ticnum = 2;
		else
			ticnum = 1;
	}
	else if (player->panim == PA_PAIN)
		ticnum = 2;
	else if (player->mo->state-states == S_PLAY_GASP)
		tails->tics = -1;
	else if (player->mo->sprite2 == SPR2_TIRE)
		ticnum = (doswim ? 2 : 4);
	else if (player->panim != PA_IDLE)
		ticnum = player->mo->tics;

	if (ticnum && tails->tics > ticnum)
		tails->tics = ticnum;


	tails->color = player->mo->color;
	tails->threshold = player->mo->z;
	tails->movecount = player->panim;
	tails->angle = horizangle;
	P_SetScale(tails, player->mo->scale, false);
	tails->destscale = player->mo->destscale;
	tails->old_scale = player->mo->old_scale;
	tails->radius = player->mo->radius;
	tails->height = player->mo->height;
	zoffs = FixedMul(zoffs, tails->scale);

	if (player->mo->eflags & MFE_VERTICALFLIP)
	{
		tails->eflags |= MFE_VERTICALFLIP;
		tails->flags2 |= MF2_OBJECTFLIP;
		zoffs = player->mo->height - tails->height - zoffs;
	}
	else
	{
		tails->eflags &= ~MFE_VERTICALFLIP;
		tails->flags2 &= ~MF2_OBJECTFLIP;
	}

	P_UnsetThingPosition(tails);
	tails->x = player->mo->x + P_ReturnThrustX(tails, tails->angle, FixedMul(backwards, tails->scale));
	tails->y = player->mo->y + P_ReturnThrustY(tails, tails->angle, FixedMul(backwards, tails->scale));
	tails->z = player->mo->z + zoffs;
	P_SetThingPosition(tails);

	if (player->mo->flags2 & MF2_SHADOW)
		tails->flags2 |= MF2_SHADOW;
	else
		tails->flags2 &= ~MF2_SHADOW;
}


void P_DoMetalJetFume(player_t *player, mobj_t *fume)
{
	static const UINT8 FUME_SKINCOLORS[] =
	{
		SKINCOLOR_ICY,
		SKINCOLOR_SKY,
		SKINCOLOR_CYAN,
		SKINCOLOR_WAVE,
		SKINCOLOR_TEAL,
		SKINCOLOR_AQUA,
		SKINCOLOR_SEAFOAM,
		SKINCOLOR_MINT,
		SKINCOLOR_PERIDOT,
		SKINCOLOR_LIME,
		SKINCOLOR_YELLOW,
		SKINCOLOR_SANDY,
		SKINCOLOR_GOLD,
		SKINCOLOR_APRICOT,
		SKINCOLOR_SUNSET
	};
	mobj_t *mo = player->mo;
	angle_t angle = player->drawangle;
	fixed_t dist;
	fixed_t heightoffset = ((mo->eflags & MFE_VERTICALFLIP) ? mo->height - (P_GetPlayerHeight(player) >> 1) : (P_GetPlayerHeight(player) >> 1));
	panim_t panim = player->panim;
	tic_t dashmode = min(player->dashmode, DASHMODE_MAX);
	boolean underwater = mo->eflags & MFE_UNDERWATER;
	statenum_t stat = fume->state-states;
	boolean resetinterp = false;

	if (panim != PA_WALK && panim != PA_RUN && panim != PA_DASH)  
	{
		if (stat != fume->info->spawnstate)
			P_SetMobjState(fume, fume->info->spawnstate);
		return;
	}

	if (player->skidtime)  
		angle += ANGLE_90;

	if (underwater)  
	{
		fume->movedir	+= FixedAngle(FixedDiv(2 * player->speed, 3 * mo->scale));
		fume->movefactor += player->speed;

		if (fume->movefactor > FixedDiv(2 * player->normalspeed, 3 * mo->scale))
		{
			INT16 i;
			fixed_t radiusV = 4*FRACUNIT;
			fixed_t radiusX = P_ReturnThrustX(mo, angle, -mo->radius >> (panim == PA_WALK ? 1 : 0));
			fixed_t radiusY = P_ReturnThrustY(mo, angle, -mo->radius >> (panim == PA_WALK ? 1 : 0));
			fixed_t factorX = P_ReturnThrustX(mo, angle + ANGLE_90, mo->scale);
			fixed_t factorY = P_ReturnThrustY(mo, angle + ANGLE_90, mo->scale);
			fixed_t offsetH, offsetV, x, y, z;

			for (i = -1; i < 2; i += 2)
			{
				mobj_t *bubble;
				offsetH = i*P_ReturnThrustX(fume, fume->movedir, radiusV);
				offsetV = i*P_ReturnThrustY(fume, fume->movedir, radiusV);
				x = mo->x + radiusX + FixedMul(offsetH, factorX);
				y = mo->y + radiusY + FixedMul(offsetH, factorY);
				z = mo->z + heightoffset + offsetV;
				bubble = P_SpawnMobj(x, y, z, MT_SMALLBUBBLE);
				P_SetScale(bubble, mo->scale/2, true);
				bubble->destscale = mo->scale;
				bubble->scalespeed = FixedMul(bubble->scalespeed, mo->scale);
				P_SetTarget(&bubble->dontdrawforviewmobj, mo);  
			}

			fume->movefactor = 0;
		}

		if (panim == PA_WALK)
		{
			if (stat != fume->info->spawnstate)
			{
				fume->threshold = 0;
				P_SetMobjState(fume, fume->info->spawnstate);
			}
			return;
		}
	}

	if (stat == fume->info->spawnstate)  
	{
		P_SetMobjState(fume, (stat = fume->info->seestate));
		P_SetScale(fume, mo->scale, false);
		resetinterp = true;
	}

	if (dashmode > DASHMODE_THRESHOLD && stat != fume->info->seestate)  
	{
		fume->destscale = mo->scale;
		fume->flags2 ^= MF2_DONTDRAW;
		fume->flags2 |= mo->flags2 & MF2_DONTDRAW;
	}
	else  
	{
		if (dashmode == DASHMODE_THRESHOLD && dashmode > (tic_t)fume->movecount)  
		{
			P_SetMobjState(fume, (stat = fume->info->seestate));
			P_SetScale(fume, 2*mo->scale, true);
		}
		fume->flags2 = (fume->flags2 & ~MF2_DONTDRAW) | (mo->flags2 & MF2_DONTDRAW);
		fume->destscale = (mo->scale + FixedDiv(player->speed, player->normalspeed)) / (underwater ? 6 : 3);
		fume->color = FUME_SKINCOLORS[(dashmode * sizeof(FUME_SKINCOLORS)) / (DASHMODE_MAX + 1)];

		if (underwater)
		{
			fume->frame = (fume->frame & FF_FRAMEMASK) | FF_ANIMATE | (P_RandomRange(0, 9) * FF_TRANS10);
			fume->threshold = 1;
		}
		else if (fume->threshold)
		{
			fume->frame = (fume->frame & FF_FRAMEMASK) | fume->state->frame;
			fume->threshold = 0;
		}
	}

	fume->movecount = dashmode;  
	fume->flags2 = (fume->flags2 & ~MF2_OBJECTFLIP) | (mo->flags2 & MF2_OBJECTFLIP);  
	fume->eflags = (fume->eflags & ~MFE_VERTICALFLIP) | (mo->eflags & MFE_VERTICALFLIP);  


	dist = -mo->radius - FixedMul(fume->info->radius, fume->destscale - mo->scale/3);

	P_UnsetThingPosition(fume);
	fume->x = mo->x + P_ReturnThrustX(fume, angle, dist);
	fume->y = mo->y + P_ReturnThrustY(fume, angle, dist);
	fume->z = mo->z + heightoffset - (fume->height >> 1);
	P_SetThingPosition(fume);
	if (resetinterp) R_ResetMobjInterpolationState(fume);


	if (player->normalspeed >= skins[player->skin]->normalspeed*2)
	{
		mobj_t *ghost = P_SpawnGhostMobj(fume);
		if (!P_MobjWasRemoved(ghost))
			P_SetTarget(&ghost->dontdrawforviewmobj, mo);  
	}
}


void P_DoFollowMobj(player_t *player, mobj_t *followmobj)
{
	if (LUA_HookFollowMobj(player, followmobj) || P_MobjWasRemoved(followmobj))
		{;}
	else
	{
		switch (followmobj->type)
		{
			case MT_TAILSOVERLAY:  
				P_DoTailsOverlay(player, followmobj);
				break;
			case MT_METALJETFUME:
				P_DoMetalJetFume(player, followmobj);
				break;
			default:
				var1 = 1;
				var2 = 0;
				A_CapeChase(followmobj);
				break;
		}
	}
}





void P_PlayerThink(player_t *player)
{
	ticcmd_t *cmd;
	const size_t playeri = (size_t)(player - players);

#ifdef PARANOIA
	if (!player->mo)
		I_Error("p_playerthink: players[%s].mo == NULL", sizeu1(playeri));
#endif


	player->blocked = false;


	if (player->mo->health <= 0 && player->playerstate == PST_LIVE)  
	{
		CONS_Debug(DBG_GAMELOGIC, "P_PlayerThink: Player %s in PST_LIVE with 0 health. (\"Zombie bug\")\n", sizeu1(playeri));
		player->playerstate = PST_DEAD;
	}

	if (player->bot == BOT_2PAI || player->bot == BOT_2PHUMAN)
	{
		if (player->playerstate == PST_LIVE || player->playerstate == PST_DEAD)
		{
			if (B_CheckRespawn(player))
				player->playerstate = PST_REBORN;
			else
				B_HandleFlightIndicator(player);
		}
		if (player->playerstate == PST_REBORN)
		{
			LUA_HookPlayer(player, HOOK(PlayerThink));
			return;
		}
	}

	if (netgame && player == &players[displayplayer] && !(leveltime % (TICRATE/5)))
	{
		seenplayer = NULL;

		if (cv_seenames.value && cv_allowseenames.value &&
			!(G_TagGametype() && (player->pflags & PF_TAGIT)))
		{
			mobj_t *mo = P_SpawnNameFinder(player->mo, MT_NAMECHECK);

			if (mo)
			{
				short int i;
				mo->flags |= MF_NOCLIPHEIGHT;
				for (i = 0; i < 32; i++)
				{



					if (P_RailThinker(mo))
						break;  
				}
			}
		}
	}

	if (player->awayviewmobj && P_MobjWasRemoved(player->awayviewmobj))
	{
		P_SetTarget(&player->awayviewmobj, NULL);  
		player->awayviewtics = 1;  
	}

	if (player->awayviewtics && player->awayviewtics != -1)
	{
		player->awayviewtics--;
		if (!(player->awayviewtics))
		{
			if (player == &players[displayplayer])
				P_ResetCamera(player, &camera);  
			else if (splitscreen && player == &players[secondarydisplayplayer])
				P_ResetCamera(player, &camera2);   
		}
	}

	if (player->flashcount)
		player->flashcount--;


	if (player->pflags & PF_NOCLIP)
		player->mo->flags |= MF_NOCLIP;
	else
		player->mo->flags &= ~MF_NOCLIP;

	cmd = &player->cmd;

	if (demoplayback && demo_forwardmove_rng)
	{

		if (cmd->forwardmove)
			P_RandomFixed();
	}

#ifdef PARANOIA
	if (player->playerstate == PST_REBORN)
		I_Error("player %s is in PST_REBORN\n", sizeu1(playeri));
#endif

	if (gametyperules & GTR_RACE)
	{
		INT32 i;


		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				if (!players[i].exiting && players[i].lives > 0)
					break;
			}
		}

		if (i == MAXPLAYERS && player->exiting == 3*TICRATE)  
			player->exiting = (14*TICRATE)/5 + 1;



		if (countdown == 11*TICRATE - 1 && P_IsLocalPlayer(player))
			P_PlayJingle(player, JT_DROWN);



		else if (countdown == 1 && !player->exiting && player->lives > 0)
		{
			if (netgame && player->mo->health > 0)
				CONS_Printf(M_GetText("%s ran out of time.\n"), player_names[player-players]);

			player->pflags |= PF_GAMETYPEOVER;

			if (player->powers[pw_carry] == CR_NIGHTSMODE)
			{
				P_DeNightserizePlayer(player);
				S_StartScreamSound(player->mo, sfx_s3k66);
			}

			player->lives = 2;  
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_INSTAKILL);
			player->lives = 0;

			if (player->playerstate == PST_DEAD)
			{
				LUA_HookPlayer(player, HOOK(PlayerThink));
				return;
			}
		}
	}



	if (player->exiting > 1 && player->exiting < 3*TICRATE)
		player->exiting--;

	if (player->exiting && countdown2)
		player->exiting = 5;



#if 0


	if (!exitfadestarted &&
		player->exiting > 0 && player->exiting <= 1*TICRATE &&
		(!multiplayer || G_CoopGametype() ? !mapheaderinfo[gamemap-1]->musinterfadeout : true) &&

		((gametyperules & GTR_RACE) ? countdown2 == 0 : true) &&  
		player->lives > 0 &&  
		P_IsLocalPlayer(player))
	{
		if (cv_playersforexit.value)
		{
			INT32 i;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator || players[i].bot)
					continue;
				if (players[i].lives <= 0)
					continue;

				if (!players[i].exiting || players[i].exiting > 1*TICRATE)
					break;
			}

			if (i == MAXPLAYERS)
			{
				exitfadestarted = true;
				S_FadeOutStopMusic(1*MUSICRATE);
			}
		}
		else
		{
			exitfadestarted = true;
			S_FadeOutStopMusic(1*MUSICRATE);
		}
	}
#endif

	if (player->exiting == 2 || countdown2 == 2)
	{
		UINT8 numneeded = (G_IsSpecialStage(gamemap) ? 4 : cv_playersforexit.value);
		if (numneeded)  
		{
			INT32 i, total = 0, exiting = 0;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator || players[i].bot)
					continue;
				if (players[i].quittime > 30 * TICRATE)
					continue;
				if (players[i].lives <= 0)
					continue;

				total++;
				if (players[i].exiting && players[i].exiting < 4)
					exiting++;
			}

			if (!total || ((4*exiting)/total) >= numneeded)
			{
				if (server)
					SendNetXCmd(XD_EXITLEVEL, NULL, 0);
			}
			else
				player->exiting = 3;
		}
		else
		{
			if (server)
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);
		}
	}

	if (player->pflags & PF_FINISHED)
	{
		if (((gametyperules & GTR_FRIENDLY) && cv_exitmove.value) && !G_EnoughPlayersFinished())
			player->exiting = 0;
		else
			P_DoPlayerExit(player, false);
	}


	P_MobjCheckWater(player->mo);

#ifndef SECTORSPECIALSAFTERTHINK
	if (player->onconveyor != 1 || !P_IsObjectOnGround(player->mo))
		player->onconveyor = 0;


	if (!player->spectator)
		P_PlayerInSpecialSector(player);
	else if (
#else
	if (player->spectator &&
#endif
	G_GametypeUsesCoopStarposts() && (netgame || multiplayer) && cv_coopstarposts.value == 2)
		P_ConsiderAllGone();

	if (player->playerstate == PST_DEAD)
	{
		player->mo->flags2 &= ~MF2_SHADOW;
		P_DeathThink(player);
		LUA_HookPlayer(player, HOOK(PlayerThink));
		return;
	}


	if (player->spectator)
	{
		if (!(gametyperules & GTR_CAMPAIGN))
			player->score = player->recordscore = 0;
	}
	else if ((netgame || multiplayer) && player->lives <= 0 && !G_CoopGametype())
	{


		player->lives = cv_startinglives.value;
	}

	if ((gametyperules & GTR_RACE) && leveltime < 4*TICRATE)
	{
		cmd->buttons &= BT_SPIN;  
		cmd->forwardmove = 0;
		cmd->sidemove = 0;
	}


	if (!player->exiting && !(player->pflags & PF_FINISHED) && !stoppedclock)
	{
		if (gametyperules & GTR_RACE)
		{
			if (leveltime >= 4*TICRATE)
				player->realtime = leveltime - 4*TICRATE;
			else
				player->realtime = 0;
		}
		else
			player->realtime = leveltime;
	}

	if (player->spectator && cmd->buttons & BT_ATTACK && !player->powers[pw_flashing] && G_GametypeHasSpectators())
	{
		if (P_SpectatorJoinGame(player))
		{
			LUA_HookPlayer(player, HOOK(PlayerThink));
			return;  
		}
	}


	if (!objectplacing && !((netgame || multiplayer) && player->spectator)
	&& maptol & TOL_NIGHTS && (player->powers[pw_carry] != CR_NIGHTSMODE || player->powers[pw_nights_helper]))
	{
		thinker_t *th;
		mobj_t *mo2;
		fixed_t x = player->mo->x;
		fixed_t y = player->mo->y;
		fixed_t z = player->mo->z;

		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->removing)
				continue;

			mo2 = (mobj_t *)th;

			if (!(mo2->type == MT_RING || mo2->type == MT_COIN
				|| mo2->type == MT_BLUESPHERE  
				|| mo2->type == MT_NIGHTSCHIP || mo2->type == MT_NIGHTSSTAR))
				continue;

			if (mo2->flags2 & MF2_NIGHTSPULL)
				continue;

			if (P_AproxDistance(P_AproxDistance(mo2->x - x, mo2->y - y), mo2->z - z) > FixedMul(128*FRACUNIT, player->mo->scale))
				continue;


			mo2->flags |= MF_NOCLIP|MF_NOCLIPHEIGHT;
			mo2->flags2 |= MF2_NIGHTSPULL;


			mo2->movefactor = 0;  
			P_SetTarget(&mo2->tracer, player->mo);
		}
	}

	if (player->linktimer && !player->powers[pw_nights_linkfreeze])
	{
		if (--player->linktimer <= 0)  
			player->linkcount = 0;
	}




	if (player->mo->reactiontime)
		player->mo->reactiontime--;
	else if (player->powers[pw_carry] == CR_MINECART)
	{
		if (P_ControlStyle(player) != CS_LMAOGALOG)
			player->mo->angle = (cmd->angleturn << 16  );

		ticruned++;
		if ((cmd->angleturn & TICCMD_RECEIVED) == 0)
			ticmiss++;

		P_MinecartThink(player);
	}
	else if (player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT && (player->powers[pw_carry] == CR_ROPEHANG || player->powers[pw_carry] == CR_ZOOMTUBE))
	{
		if (player->powers[pw_carry] == CR_ROPEHANG)
		{
			if (P_ControlStyle(player) != CS_LMAOGALOG)
				player->mo->angle = (cmd->angleturn<<16  );

			ticruned++;
			if ((cmd->angleturn & TICCMD_RECEIVED) == 0)
				ticmiss++;

			P_DoRopeHang(player);
			P_DoJumpStuff(player, &player->cmd);
		}
		else  
		{
			P_DoZoomTube(player);
			if (!(player->panim == PA_ROLL))
				P_SetMobjState(player->mo, S_PLAY_ROLL);
		}
		player->rmomx = player->rmomy = 0;  
		P_ResetScore(player);
	}
	else
	{
		if (player->bumpertime == TICRATE/2 && player->mo->hnext)
		{




			P_MoveOrigin(player->mo, player->mo->hnext->x, player->mo->hnext->y
				, player->mo->hnext->z + FixedMul(player->mo->hnext->height/4, player->mo->hnext->scale));
			P_SetTarget(&player->mo->hnext, NULL);
		}
		P_MovePlayer(player);
	}

	if (!player->mo)
	{
		LUA_HookPlayer(player, HOOK(PlayerThink));
		return;  
	}


	{
		angle_t oldang = player->drawangle, diff = 0;
		UINT8 factor = 0;


		if (player->exiting  
		|| player->powers[pw_carry] == CR_NIGHTSMODE)
			;
		else if (!(player->pflags & PF_DIRECTIONCHAR)
		|| (player->climbing)  
		|| (twodlevel || player->mo->flags2 & MF2_TWOD)  
		|| G_RingSlingerGametype())  
			player->drawangle = player->mo->angle;
		else if (P_PlayerInPain(player))
			;
		else if (player->powers[pw_justsprung])  
		{
#ifdef SPRINGSPIN
			if (player->powers[pw_justsprung] & (1<<15))
				player->drawangle += (player->powers[pw_justsprung] & ~(1<<15))*(ANG2+ANG1);
#endif
		}
		else if (player->powers[pw_carry] && player->mo->tracer)  
		{
			switch (player->powers[pw_carry])
			{
				case CR_PLAYER:
					if (player->mo->tracer->player)
					{
						player->drawangle = player->mo->tracer->player->drawangle;
						break;
					}
					 
				case CR_MINECART:
				case CR_GENERIC:
				case CR_PTERABYTE:
					player->drawangle = player->mo->tracer->angle;
					break;
				case CR_ROLLOUT:
					if (cmd->forwardmove || cmd->sidemove)  
					{  
						diff = (((player->cmd.angleturn<<16) + R_PointToAngle2(0, 0, -cmd->forwardmove<<FRACBITS, cmd->sidemove<<FRACBITS)) - player->drawangle);
						factor = 4;
					}
					break;
				case CR_DUSTDEVIL:
					player->drawangle += ANG20;
					break;
				case CR_FAN:
					if (player->pflags & PF_ANALOGMODE)  
						player->drawangle = player->mo->angle;
					break;
				                                                                           
                       
				case CR_ROPEHANG:
					if (player->mo->momx || player->mo->momy)
					{
						player->drawangle = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
						break;
					}
					 
				default:
					player->drawangle = player->mo->angle;
					break;
			}
		}
		else if ((player->skidtime > (TICRATE/2 - 2) || ((player->pflags & (PF_SPINNING|PF_STARTDASH)) == PF_SPINNING)) && (abs(player->rmomx) > 5*player->mo->scale || abs(player->rmomy) > 5*player->mo->scale))  
			player->drawangle = R_PointToAngle2(0, 0, player->rmomx, player->rmomy);
		else if (((player->charability2 == CA2_GUNSLINGER || player->charability2 == CA2_MELEE) && player->panim == PA_ABILITY2) || player->pflags & PF_STASIS || player->skidtime)
			;
		else
		{
			if (player->pflags & PF_GLIDING)
			{
				if (player->speed < player->mo->scale)
					diff = player->mo->angle - player->drawangle;
				else
					diff = (R_PointToAngle2(0, 0, player->rmomx, player->rmomy) - player->drawangle);
				factor = 4;
			}
			else if (player->pflags & PF_SLIDING)
			{
#if 0  
				if (player->mo->eflags & MFE_TOUCHWATER || player->powers[pw_flashing] > (flashingtics/4)*3)
				{
					diff = (player->mo->angle - player->drawangle);
					factor = 16;
				}
				else
				{
					diff = factor = 0;
					player->drawangle += ANGLE_22h;
				}
#else
				diff = (player->mo->angle - player->drawangle);
				factor = 16;
#endif
			}
			else if (player->pflags & PF_STARTDASH)
			{
				diff = (player->mo->angle - player->drawangle);
				factor = 4;
			}
			else if (cmd->forwardmove || cmd->sidemove)  
			{
				diff = ((player->mo->angle + ((player->pflags & PF_ANALOGMODE) ? 0 : R_PointToAngle2(0, 0, cmd->forwardmove<<FRACBITS, -cmd->sidemove<<FRACBITS))) - player->drawangle);
				factor = 4;
			}
			else if (player->rmomx || player->rmomy)
				diff = factor = 0;
			else
			{
				diff = (player->mo->angle - player->drawangle);
				factor = 8;
			}
		}

		if (diff)
		{
			if (diff > ANGLE_180)
				diff = InvAngle(InvAngle(diff)/factor);
			else
				diff /= factor;
			player->drawangle += diff;
		}


		if (player->panim == PA_IDLE)
		{
			diff = player->drawangle - oldang;
			if (diff > ANGLE_180)
				diff = InvAngle(diff);
			if (diff > ANG10/2)
			{
				statenum_t stat = player->mo->state-states;
				if (stat == S_PLAY_WAIT)
					P_SetMobjState(player->mo, S_PLAY_STND);
				else if (stat == S_PLAY_STND && player->mo->tics != -1)
					player->mo->tics++;
			}
		}


		{
			boolean currentlyonground = P_IsObjectOnGround(player->mo);

			if (player->powers[pw_noautobrake])
				;
			else if (!player->powers[pw_carry] && !player->powers[pw_nocontrol]
			&& ((player->pflags & (PF_AUTOBRAKE|PF_APPLYAUTOBRAKE|PF_STASIS)) == (PF_AUTOBRAKE|PF_APPLYAUTOBRAKE))
			&& !(cmd->forwardmove || cmd->sidemove)
			&& (player->rmomx || player->rmomy)
			&& (!player->capsule || (player->capsule->reactiontime != (player-players)+1)))
			{
				fixed_t acceleration = (player->accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration) * player->thrustfactor * 20;
				angle_t moveAngle = R_PointToAngle2(0, 0, player->rmomx, player->rmomy);

				if (!currentlyonground)
					acceleration /= 2;

				else if (!player->skidtime && !(player->mo->eflags & MFE_GOOWATER) && !(player->pflags & (PF_JUMPED|PF_SPINNING|PF_SLIDING)) && !(player->charflags & SF_NOSKID) && P_AproxDistance(player->mo->momx, player->mo->momy) >= FixedMul(player->runspeed, player->mo->scale))  
				{
					if (player->mo->state-states != S_PLAY_SKID)
						P_SetMobjState(player->mo, S_PLAY_SKID);
					player->mo->tics = player->skidtime = (player->mo->movefactor == FRACUNIT) ? TICRATE/2 : (FixedDiv(35<<(FRACBITS-1), FixedSqrt(player->mo->movefactor)))>>FRACBITS;

					if (P_IsLocalPlayer(player))  
						S_StartSound(player->mo, sfx_skid);
				}

				if (player->mo->movefactor != FRACUNIT)  
					acceleration = FixedMul(acceleration<<FRACBITS, player->mo->movefactor)>>FRACBITS;

				P_Thrust(player->mo, moveAngle, FixedMul(-acceleration, player->mo->scale));
			}

			if (!(player->pflags & PF_AUTOBRAKE)
			|| player->powers[pw_carry]
			|| player->panim == PA_SPRING
			|| player->panim == PA_PAIN
			|| !player->mo->health
			|| player->climbing
			|| player->pflags & (PF_SPINNING|PF_SLIDING)
			|| player->bumpertime)
				player->pflags &= ~PF_APPLYAUTOBRAKE;
			else if (currentlyonground || player->powers[pw_tailsfly])
				player->pflags |= PF_APPLYAUTOBRAKE;
		}
	}

	player->mo->movefactor = FRACUNIT;  




	player->pflags &= ~PF_FULLSTASIS;

	if (player->onconveyor == 1)
		player->onconveyor = 3;
	else if (player->onconveyor == 3)
		player->cmomy = player->cmomx = 0;

	P_DoSuperStuff(player);
	P_CheckSneakerAndLivesTimer(player);
	P_DoBubbleBreath(player);  
	P_CheckUnderwaterAndSpaceTimer(player);  
	P_CheckInvincibilityTimer(player);  

#if 1

	if ((player->powers[pw_super] || player->powers[pw_sneakers]) && (player->speed + abs(player->mo->momz)) > FixedMul(20*FRACUNIT,player->mo->scale))
	{
		mobj_t *gmobj = P_SpawnGhostMobj(player->mo);
		if (!P_MobjWasRemoved(gmobj))
		{
			gmobj->fuse = 2;
			if (gmobj->tracer)
				gmobj->tracer->fuse = 2;
			if (leveltime & 1)
			{
				gmobj->frame &= ~FF_TRANSMASK;
				gmobj->frame |= tr_trans70<<FF_TRANSSHIFT;
				if (gmobj->tracer)
				{
					gmobj->tracer->frame &= ~FF_TRANSMASK;
					gmobj->tracer->frame |= tr_trans70<<FF_TRANSSHIFT;
				}
			}
		}
	}
#endif


	if (player->powers[pw_carry] != CR_NIGHTSMODE)
	{
		if (cmd->buttons & BT_SPIN)
			player->pflags |= PF_SPINDOWN;
		else
			player->pflags &= ~PF_SPINDOWN;
	}


	if (player->quittime && player->powers[pw_flashing] < flashingtics - 1
	&& !(G_TagGametype() && !(player->pflags & PF_TAGIT)) && !player->gotflag)
		player->powers[pw_flashing] = flashingtics - 1;




	if (player->ammoremovaltimer)
	{
		if (--player->ammoremovaltimer == 0)
			player->ammoremoval = 0;
	}


	if (player->powers[pw_sneakers] && player->powers[pw_sneakers] < UINT16_MAX)
		player->powers[pw_sneakers]--;

	if (player->powers[pw_invulnerability] && player->powers[pw_invulnerability] < UINT16_MAX)
		player->powers[pw_invulnerability]--;

	if (player->powers[pw_flashing] && player->powers[pw_flashing] < UINT16_MAX && ((player->powers[pw_carry] == CR_NIGHTSMODE) || player->powers[pw_flashing] < flashingtics))
		player->powers[pw_flashing]--;

	if (player->powers[pw_tailsfly] && player->powers[pw_tailsfly] < UINT16_MAX && player->charability != CA_SWIM)  
		player->powers[pw_tailsfly]--;

	if (player->powers[pw_pushing] && player->powers[pw_pushing] < UINT16_MAX)
		player->powers[pw_pushing]--;

	if (player->powers[pw_justsprung] & ((1<<15)-1) && player->powers[pw_justsprung] < UINT16_MAX)
		player->powers[pw_justsprung]--;
	else
		player->powers[pw_justsprung] = 0;

	if (player->powers[pw_noautobrake] && player->powers[pw_noautobrake] < UINT16_MAX)
		player->powers[pw_noautobrake]--;

	if (player->powers[pw_underwater] && (player->pflags & PF_GODMODE || (player->powers[pw_shield] & SH_PROTECTWATER)))
	{
		if (player->powers[pw_underwater] <= 12*TICRATE+1)
		{
			player->powers[pw_underwater] = 0;
			P_RestoreMusic(player);  
		}
		else
			player->powers[pw_underwater] = 0;
	}
	else if (player->powers[pw_underwater] && !(maptol & TOL_NIGHTS) && !((netgame || multiplayer) && (player->spectator || player->quittime)))  
		player->powers[pw_underwater]--;

	if (player->powers[pw_spacetime] && (player->pflags & PF_GODMODE || (player->powers[pw_shield] & SH_PROTECTWATER)))
		player->powers[pw_spacetime] = 0;
	else if (player->powers[pw_spacetime] && !(maptol & TOL_NIGHTS) && !((netgame || multiplayer) && (player->spectator || player->quittime)))  
		player->powers[pw_spacetime]--;

	if (player->powers[pw_gravityboots] && player->powers[pw_gravityboots] < UINT16_MAX)
		player->powers[pw_gravityboots]--;

	if (player->powers[pw_extralife] && player->powers[pw_extralife] < UINT16_MAX)
		player->powers[pw_extralife]--;

	if (player->powers[pw_nights_linkfreeze] && player->powers[pw_nights_linkfreeze] < UINT16_MAX)
		player->powers[pw_nights_linkfreeze]--;

	if (player->powers[pw_nights_superloop] && player->powers[pw_nights_superloop] < UINT16_MAX)
		player->powers[pw_nights_superloop]--;

	if (player->powers[pw_nights_helper] && player->powers[pw_nights_helper] < UINT16_MAX)
		player->powers[pw_nights_helper]--;

	if (player->powers[pw_nocontrol] & ((1<<15)-1) && player->powers[pw_nocontrol] < UINT16_MAX)
		player->powers[pw_nocontrol]--;
	else
		player->powers[pw_nocontrol] = 0;

	if (player->powers[pw_ignorelatch] & ((1<<15)-1) && player->powers[pw_ignorelatch] < UINT16_MAX)
		player->powers[pw_ignorelatch]--;
	else
		player->powers[pw_ignorelatch] = 0;

	if (player->powers[pw_strong] & STR_ANIM)
	{
		if (!(player->stronganim))
			player->stronganim = player->panim;
		else if (player->panim != player->stronganim)
			player->powers[pw_strong] = STR_NONE;
	}
	else if (player->stronganim)
		player->stronganim = 0;


	if (player->powers[pw_super]
	&& (player->mo->state < &states[S_PLAY_SUPER_TRANS1]
	|| player->mo->state > &states[S_PLAY_SUPER_TRANS6]))
		player->powers[pw_super]++;

	if (player->powers[pw_carry] == CR_BRAKGOOP)
	{
		if (!player->powers[pw_flashing])
		{
			if (player->mo->state != &states[S_PLAY_STND])
				P_SetMobjState(player->mo, S_PLAY_STND);
			else
				player->mo->tics = 2;
		}
		else
			P_SetTarget(&player->mo->tracer, NULL);

		if (!player->mo->tracer)
			player->powers[pw_carry] = CR_NONE;
	}

	if (player->bumpertime)
		player->bumpertime--;

	if (player->skidtime)
		player->skidtime--;

	if (player->weapondelay)
		player->weapondelay--;

	if (player->tossdelay)
		player->tossdelay--;

	if (player->homing)
		player->homing--;

	if (player->texttimer)
	{
		--player->texttimer;
		if (!player->texttimer && !player->exiting && (player->textvar == NTV_NONE || player->textvar == NTV_BONUSTIMEEND))
		{
			player->texttimer = 4*TICRATE;
			player->textvar = NTV_GETSPHERES;  

			if (!P_MobjWasRemoved(player->capsule) && player->capsule->health != player->capsule->spawnpoint->args[1])
				player->textvar = NTV_GETMORESPHERES;  
		}
	}

	if (player->losstime && !player->powers[pw_flashing])
		player->losstime--;


	if (player->powers[pw_flashing] > 0 && player->powers[pw_flashing] < flashingtics && (leveltime & 1) && player->playerstate == PST_LIVE)
		player->mo->flags2 |= MF2_DONTDRAW;
	else
		player->mo->flags2 &= ~MF2_DONTDRAW;

	player->pflags &= ~PF_SLIDING;

#define dashmode player->dashmode

	if ((player->charflags & SF_DASHMODE) && !player->gotflag && !player->powers[pw_carry] && !player->exiting && !(maptol & TOL_NIGHTS) && !metalrecording)  
	{
		tic_t prevdashmode = dashmode;
		boolean totallyradical = player->speed >= FixedMul(player->runspeed, player->mo->scale);
		boolean floating = (player->secondjump == 1);

		if ((totallyradical && !floating) || (player->pflags & PF_STARTDASH))
		{
			if (dashmode < DASHMODE_MAX)
				dashmode++;  
			if (dashmode == DASHMODE_THRESHOLD)  
				S_StartSound(player->mo, (player->charflags & SF_MACHINE) ? sfx_kc4d : sfx_cdfm40);  
		}
		else if ((!totallyradical || !floating) && !(player->pflags & PF_SPINNING))
		{
			if (dashmode > 3)
			{
				dashmode -= 3;  
				if ((dashmode+3) >= DASHMODE_THRESHOLD && dashmode < DASHMODE_THRESHOLD)
					S_StartSound(player->mo, sfx_kc65);
			}
			else
				dashmode = 0;
		}

		if (dashmode < DASHMODE_THRESHOLD)  
		{
			if (prevdashmode >= DASHMODE_THRESHOLD)
			{
				player->normalspeed = skins[player->skin]->normalspeed;  
				player->jumpfactor = skins[player->skin]->jumpfactor;
				if (player->powers[pw_strong] & STR_DASH)
					player->powers[pw_strong] = STR_NONE;
			}
		}
		else if (P_IsObjectOnGround(player->mo))  
		{
			if (player->normalspeed < skins[player->skin]->normalspeed*2)  
				player->normalspeed += FRACUNIT/5;  

			if (player->jumpfactor < FixedMul(skins[player->skin]->jumpfactor, 5*FRACUNIT/4))  
				player->jumpfactor += FRACUNIT/300;

			if ((player->charflags & SF_MACHINE) && (!(player->powers[pw_strong] == STR_METAL)))
					player->powers[pw_strong] = STR_METAL;
		}

		if (player->normalspeed >= skins[player->skin]->normalspeed*2)
		{
			mobj_t *ghost = P_SpawnGhostMobj(player->mo);  
			if (!P_MobjWasRemoved(ghost))
			{
				ghost->fuse = 2;  
				if (ghost->tracer && !P_MobjWasRemoved(ghost->tracer))
					ghost->tracer->fuse = ghost->fuse;
			}
		}
	}
	else if (dashmode)
	{
		if (dashmode >= DASHMODE_THRESHOLD)  
		{
			player->normalspeed = skins[player->skin]->normalspeed;
			player->jumpfactor = skins[player->skin]->jumpfactor;
			S_StartSound(player->mo, sfx_kc65);
			if (player->powers[pw_strong] & STR_DASH)
				player->powers[pw_strong] = STR_NONE;
		}
		dashmode = 0;
	}
#undef dashmode

	LUA_HookPlayer(player, HOOK(PlayerThink));

  
                        
  
            
                       
                              
   
                                              
                     
                                            
                            
    
                                              
                                                            
                         
     
                                       
           
     
    

                                  
    
                                                
                                        
                             
     
                                                             
            
     

                      
                                                      
    
   

                         
    
}


static boolean P_MobjAboveLava(mobj_t *mobj)
{
	sector_t *sector = mobj->subsector->sector;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_SWIMMABLE))
				continue;

			if (rover->master->frontsector->damagetype != SD_FIRE && rover->master->frontsector->damagetype != SD_LAVA)
				continue;

			if (mobj->eflags & MFE_VERTICALFLIP)
			{
				if (*rover->bottomheight <= mobj->ceilingz && *rover->bottomheight >= mobj->z)
					return true;
			}
			else
			{
				if (*rover->topheight >= mobj->floorz && *rover->topheight <= mobj->z)
					return true;
			}
		}
	}

	return false;
}






void P_PlayerAfterThink(player_t *player)
{
	ticcmd_t *cmd;
	INT32 oldweapon = player->currentweapon;
	camera_t *thiscam = NULL;  

#ifdef PARANOIA
	if (!player->mo)
	{
		const size_t playeri = (size_t)(player - players);
		I_Error("P_PlayerAfterThink: players[%s].mo == NULL", sizeu1(playeri));
	}
#endif

	cmd = &player->cmd;

#ifdef SECTORSPECIALSAFTERTHINK
	if (player->onconveyor != 1 || !P_IsObjectOnGround(player->mo))
		player->onconveyor = 0;


	if (!player->spectator)
		P_PlayerInSpecialSector(player);
#endif

	if (splitscreen && player == &players[secondarydisplayplayer])
		thiscam = &camera2;
	else if (player == &players[displayplayer])
		thiscam = &camera;

	if (player->playerstate == PST_DEAD)
	{


		{
			if (thiscam && thiscam->chase)
				P_MoveChaseCamera(player, thiscam, false);
		}
		if (player->followmobj)
		{
			P_RemoveMobj(player->followmobj);
			P_SetTarget(&player->followmobj, NULL);
		}
		return;
	}

	if (player->powers[pw_carry] == CR_NIGHTSMODE)
	{
		player->powers[pw_gravityboots] = 0;

	}

	if (!(player->pflags & PF_WPNDOWN))
	{
		if (cmd->buttons & BT_WEAPONNEXT)
		{
			player->currentweapon++;
			player->currentweapon %= NUM_WEAPONS;
			player->pflags |= PF_WPNDOWN;
		}

		if (cmd->buttons & BT_WEAPONPREV)
		{
			player->currentweapon--;
			if (player->currentweapon < 0)
				player->currentweapon = NUM_WEAPONS - 1;
			player->pflags |= PF_WPNDOWN;

			if (player->currentweapon == WEP_RAIL && (!(player->ringweapons & RW_RAIL) || !player->powers[pw_railring]))
				player->currentweapon--;
			if (player->currentweapon == WEP_EXPLODE && (!(player->ringweapons & RW_EXPLODE) || !player->powers[pw_explosionring]))
				player->currentweapon--;
			if (player->currentweapon == WEP_GRENADE && (!(player->ringweapons & RW_GRENADE) || !player->powers[pw_grenadering]))
				player->currentweapon--;
			if (player->currentweapon == WEP_SCATTER && (!(player->ringweapons & RW_SCATTER) || !player->powers[pw_scatterring]))
				player->currentweapon--;
			if (player->currentweapon == WEP_BOUNCE && (!(player->ringweapons & RW_BOUNCE) || !player->powers[pw_bouncering]))
				player->currentweapon--;
			if (player->currentweapon == WEP_AUTO && (!(player->ringweapons & RW_AUTO) || !player->powers[pw_automaticring]))
				player->currentweapon = 0;
		}

		if (cmd->buttons & BT_WEAPONMASK)
		{

			INT32 weapon = (cmd->buttons & BT_WEAPONMASK);

			switch (weapon)
			{
			case 1:  
				player->currentweapon = 0;
				player->pflags |= PF_WPNDOWN;
				break;
			case 2:  
				if ((player->ringweapons & RW_AUTO) && player->powers[pw_automaticring])
				{
					player->currentweapon = WEP_AUTO;
					player->pflags |= PF_WPNDOWN;
				}
				break;
			case 3:  
				if ((player->ringweapons & RW_BOUNCE) && player->powers[pw_bouncering])
				{
					player->currentweapon = WEP_BOUNCE;
					player->pflags |= PF_WPNDOWN;
				}
				break;
			case 4:  
				if ((player->ringweapons & RW_SCATTER) && player->powers[pw_scatterring])
				{
					player->currentweapon = WEP_SCATTER;
					player->pflags |= PF_WPNDOWN;
				}
				break;
			case 5:  
				if ((player->ringweapons & RW_GRENADE) && player->powers[pw_grenadering])
				{
					player->currentweapon = WEP_GRENADE;
					player->pflags |= PF_WPNDOWN;
				}
				break;
			case 6:  
				if ((player->ringweapons & RW_EXPLODE) && player->powers[pw_explosionring])
				{
					player->currentweapon = WEP_EXPLODE;
					player->pflags |= PF_WPNDOWN;
				}
				break;
			case 7:  
				if ((player->ringweapons & RW_RAIL) && player->powers[pw_railring])
				{
					player->currentweapon = WEP_RAIL;
					player->pflags |= PF_WPNDOWN;
				}
				break;
			}
		}
	}

	if (!(cmd->buttons & (BT_WEAPONNEXT|BT_WEAPONPREV|BT_WEAPONMASK)))
		player->pflags &= ~PF_WPNDOWN;


	if (player->currentweapon == WEP_AUTO && (!(player->ringweapons & RW_AUTO) || !player->powers[pw_automaticring]))
		player->currentweapon++;
	if (player->currentweapon == WEP_BOUNCE && (!(player->ringweapons & RW_BOUNCE) || !player->powers[pw_bouncering]))
		player->currentweapon++;
	if (player->currentweapon == WEP_SCATTER && (!(player->ringweapons & RW_SCATTER) || !player->powers[pw_scatterring]))
		player->currentweapon++;
	if (player->currentweapon == WEP_GRENADE && (!(player->ringweapons & RW_GRENADE) || !player->powers[pw_grenadering]))
		player->currentweapon++;
	if (player->currentweapon == WEP_EXPLODE && (!(player->ringweapons & RW_EXPLODE) || !player->powers[pw_explosionring]))
		player->currentweapon++;
	if (player->currentweapon == WEP_RAIL && (!(player->ringweapons & RW_RAIL) || !player->powers[pw_railring]))
		player->currentweapon = 0;

	if (P_IsLocalPlayer(player) && (player->pflags & PF_WPNDOWN) && player->currentweapon != oldweapon)
		S_StartSound(NULL, sfx_wepchg);

	if ((player->pflags & PF_SLIDING) && ((player->pflags & (PF_JUMPED|PF_NOJUMPDAMAGE)) != PF_JUMPED))
		P_SetMobjState(player->mo, player->mo->info->painstate);

	                                                                                    
                                         
        
	if (player->mo->tracer)
	{
		switch (player->powers[pw_carry])
		{
			case CR_PLAYER:  
			{
				mobj_t *tails = player->mo->tracer;
				player->mo->height = FixedDiv(P_GetPlayerHeight(player), FixedDiv(14*FRACUNIT,10*FRACUNIT));

				if (tails->player && !(tails->player->pflags & PF_CANCARRY))
					player->powers[pw_carry] = CR_NONE;

				if (player->mo->eflags & MFE_VERTICALFLIP)
				{
					if ((tails->z + tails->height + player->mo->height + FixedMul(FRACUNIT, player->mo->scale)) <= tails->ceilingz
						&& (tails->eflags & MFE_VERTICALFLIP))  
						player->mo->z = tails->z + tails->height + 12*player->mo->scale;
					else
						player->powers[pw_carry] = CR_NONE;
				}
				else
				{
					if ((tails->z - player->mo->height - FixedMul(FRACUNIT, player->mo->scale)) >= tails->floorz
						&& !(tails->eflags & MFE_VERTICALFLIP))  
						player->mo->z = tails->z - player->mo->height - 12*player->mo->scale;
					else
						player->powers[pw_carry] = CR_NONE;
				}

				if (tails->health <= 0)
					player->powers[pw_carry] = CR_NONE;
				else
				{
					if (tails->player)
						P_TryMove(player->mo, tails->x + P_ReturnThrustX(tails, tails->player->drawangle, 4*tails->scale), tails->y + P_ReturnThrustY(tails, tails->player->drawangle, 4*tails->scale), true);
					else
						P_TryMove(player->mo, tails->x + P_ReturnThrustX(tails, tails->angle, 4*tails->scale), tails->y + P_ReturnThrustY(tails, tails->angle, 4*tails->scale), true);
					player->mo->momx = tails->momx;
					player->mo->momy = tails->momy;
					player->mo->momz = tails->momz;
				}

				if (G_CoopGametype() && tails->player && tails->player->bot != BOT_2PAI)
				{
					player->mo->angle = tails->angle;

					if (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG)
						P_SetPlayerAngle(player, player->mo->angle);
				}

				if (P_AproxDistance(player->mo->x - tails->x, player->mo->y - tails->y) > tails->radius)
					player->powers[pw_carry] = CR_NONE;

				if (player->powers[pw_carry] == CR_PLAYER)
				{
					if (player->mo->state-states != S_PLAY_RIDE)
						P_SetMobjState(player->mo, S_PLAY_RIDE);
					if (tails->player && (tails->skin && ((skin_t *)(tails->skin))->sprites[SPR2_SWIM].numframes) && (tails->eflags & MFE_UNDERWATER))
						tails->player->powers[pw_tailsfly] = 0;
				}
				else if (player->powers[pw_carry] == CR_NONE)
					P_SetTarget(&player->mo->tracer, NULL);

				if (player-players == consoleplayer && botingame)
					CV_SetValue(&cv_analog[1], (player->powers[pw_carry] != CR_PLAYER));
				break;
			}
			case CR_GENERIC:  
			{
				mobj_t *item = player->mo->tracer;

				P_UnsetThingPosition(player->mo);
				player->mo->x = item->x;
				player->mo->y = item->y;
				if (player->mo->eflags & MFE_VERTICALFLIP)
					player->mo->z = item->z + item->height - FixedDiv(player->mo->height, 3*FRACUNIT);
				else
					player->mo->z = item->z - FixedDiv(player->mo->height, 3*FRACUNIT/2);
				player->mo->momx = player->mo->momy = player->mo->momz = 0;
				P_SetThingPosition(player->mo);
				if (player->mo->state-states != S_PLAY_RIDE)
					P_SetMobjState(player->mo, S_PLAY_RIDE);


				if (item->type == MT_BLACKEGGMAN_MISSILE)
				{
					if (cmd->forwardmove > 0)
						item->momz += FixedMul(FRACUNIT/4, item->scale);
					else if (cmd->forwardmove < 0)
						item->momz -= FixedMul(FRACUNIT/4, item->scale);

					item->angle = player->mo->angle;
					P_InstaThrust(item, item->angle, FixedMul(item->info->speed, item->scale));

					if (player->mo->z <= player->mo->floorz || item->health <= 0)
					{
						player->powers[pw_carry] = CR_NONE;
						P_SetTarget(&player->mo->tracer, NULL);
					}
				}
				break;
			}
			case CR_MACESPIN:  
			{
				mobj_t *chain;
				mobj_t *macecenter;
				if (!player->mo->tracer->tracer)  
					break;
				chain = player->mo->tracer;
				macecenter = player->mo->tracer->tracer;

				player->mo->height = P_GetPlayerSpinHeight(player);

				player->mo->momx = (chain->x - player->mo->x)*2;
				player->mo->momy = (chain->y - player->mo->y)*2;
				player->mo->momz = (chain->z - (player->mo->height-chain->height/2) - player->mo->z)*2;
				P_MoveOrigin(player->mo, chain->x, chain->y, chain->z - (player->mo->height-chain->height/2));
				if (!player->powers[pw_flashing])  
				{
					player->pflags |= PF_JUMPED;
					player->pflags &= ~PF_NOJUMPDAMAGE;
					player->secondjump = 0;
					player->pflags &= ~PF_THOKKED;

					if ((macecenter->flags & MF_SLIDEME)  
					&& !(twodlevel || player->mo->flags2 & MF2_TWOD))  
					{
						macecenter->angle += cmd->sidemove<<ANGLETOFINESHIFT;
						player->mo->angle += cmd->sidemove<<ANGLETOFINESHIFT;  

						if (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG)
							P_SetPlayerAngle(player, player->mo->angle);
					}
				}
				break;
			}
			case CR_DUSTDEVIL:
			{
				mobj_t *mo = player->mo, *dustdevil = player->mo->tracer;

				if (abs(mo->x - dustdevil->x) > dustdevil->radius || abs(mo->y - dustdevil->y) > dustdevil->radius)
				{
					P_SetTarget(&player->mo->tracer, NULL);
					player->powers[pw_carry] = CR_NONE;
					break;
				}

				break;
			}
			case CR_FAN:
			{
				fixed_t zdist;
				mobj_t *mo = player->mo, *fan = player->mo->tracer;

				if (!(player->pflags & PF_JUMPSTASIS))
					player->pflags |= PF_JUMPSTASIS;

				if (fan->eflags & MFE_VERTICALFLIP)
					zdist = (mo->z + mo->height) - (fan->z + fan->height);
				else
					zdist = mo->z - fan->z;

				if ((fan->type != MT_FAN && !P_PlayerTouchingSectorSpecialFlag(player, SSF_FAN))
				|| (fan->type == MT_FAN && (abs(mo->x - fan->x) > fan->radius || abs(mo->y - fan->y) > fan->radius || zdist > (fan->health << FRACBITS))))
				{
					P_SetTarget(&player->mo->tracer, NULL);
					player->pflags &= ~PF_JUMPSTASIS;
					player->powers[pw_carry] = CR_NONE;
					break;
				}
				break;
			}
			case CR_ROLLOUT:
			{
				mobj_t *mo = player->mo, *rock = player->mo->tracer;
				UINT8 walktics = mo->state->tics - P_GetPlayerControlDirection(player);

				if (!rock || P_MobjWasRemoved(rock))
				{
					P_SetTarget(&player->mo->tracer, NULL);
					player->powers[pw_carry] = CR_NONE;
					break;
				}

				if (player->cmd.forwardmove || player->cmd.sidemove)
				{
					rock->flags2 |= MF2_STRONGBOX;  
					if (twodlevel || (mo->flags2 & MF2_TWOD))
						rock->movedir = mo->angle;
					else
						rock->movedir = (player->cmd.angleturn << FRACBITS) + R_PointToAngle2(0, 0, player->cmd.forwardmove << FRACBITS, -player->cmd.sidemove << FRACBITS);
					P_Thrust(rock, rock->movedir, rock->scale >> 1);
				}
				else
					rock->flags2 &= ~MF2_STRONGBOX;

				mo->momx = rock->momx;
				mo->momy = rock->momy;
				mo->momz = 0;

				if (player->panim == PA_IDLE && (mo->momx || mo->momy))
				{
					P_SetMobjState(player->mo, S_PLAY_WALK);
				}

				if (player->panim == PA_WALK && mo->tics > walktics)
				{
					mo->tics = walktics;
				}

				P_MoveOrigin(player->mo, rock->x, rock->y, rock->z + ((mo->eflags & MFE_VERTICALFLIP) ? -mo->height : rock->height));
				break;
			}
			case CR_PTERABYTE:  
			{

#define DROPTHRESHOLD TICRATE

				mobj_t *ptera = player->mo->tracer;
				mobj_t *spawnpoint = ptera->tracer->tracer;
				player->mo->height = FixedDiv(P_GetPlayerHeight(player), FixedDiv(14 * FRACUNIT, 10 * FRACUNIT));

				if (ptera->health <= 0)
					goto dropoff;

				if (P_MobjAboveLava(ptera) && ptera->movefactor < DROPTHRESHOLD)
					goto dropoff;

				if (player->mo->eflags & MFE_VERTICALFLIP)
				{
					if ((ptera->z + ptera->height + player->mo->height + FixedMul(FRACUNIT, player->mo->scale)) <= ptera->ceilingz
						&& (ptera->eflags & MFE_VERTICALFLIP))  
						player->mo->z = ptera->z + ptera->height + FixedMul(FRACUNIT, player->mo->scale);

					if (ptera->ceilingz - ptera->z > spawnpoint->ceilingz - spawnpoint->z + 512*FRACUNIT && ptera->movefactor < DROPTHRESHOLD)
						goto dropoff;
				}
				else
				{
					if ((ptera->z - player->mo->height - FixedMul(FRACUNIT, player->mo->scale)) >= ptera->floorz
						&& !(ptera->eflags & MFE_VERTICALFLIP))  
						player->mo->z = ptera->z - player->mo->height - FixedMul(FRACUNIT, player->mo->scale);

					if (ptera->z - ptera->floorz > spawnpoint->z - spawnpoint->floorz + 512 * FRACUNIT && ptera->movefactor < DROPTHRESHOLD)
						goto dropoff;
				}

				ptera->movefactor--;
				if (!ptera->movefactor)
					goto dropoff;

				if (ptera->cusval >= 50)  
				{
					player->powers[pw_carry] = CR_NONE;
					P_SetTarget(&player->mo->tracer, NULL);
					P_KillMobj(ptera, player->mo, player->mo, 0);
					P_SetObjectMomZ(player->mo, 12*FRACUNIT, false);
					player->pflags |= PF_APPLYAUTOBRAKE|PF_JUMPED|PF_THOKKED;
					P_SetMobjState(player->mo, S_PLAY_ROLL);
					break;
				}

				if (ptera->cusval)
					ptera->cusval--;

				P_TryMove(player->mo, ptera->x + ptera->watertop, ptera->y + ptera->waterbottom, true);
				player->mo->z += ptera->cvmem;
				player->mo->momx = ptera->momx;
				player->mo->momy = ptera->momy;
				player->mo->momz = ptera->momz;

				if (P_AproxDistance(player->mo->x - ptera->x - ptera->watertop, player->mo->y - ptera->y - ptera->waterbottom) > ptera->radius)
					goto dropoff;

				ptera->watertop >>= 1;
				ptera->waterbottom >>= 1;
				ptera->cvmem >>= 1;

				if (player->mo->state-states != S_PLAY_FALL)
					P_SetMobjState(player->mo, S_PLAY_FALL);
				break;

			dropoff:
				player->powers[pw_carry] = CR_NONE;
				P_SetTarget(&player->mo->tracer, NULL);
				ptera->movefactor = TICRATE;
				ptera->extravalue1 |= 4;
				break;
			}
			default:
				break;
		}
	}

	if (thiscam)
	{
		if (!thiscam->chase)  
		{
			P_CalcHeight(player);
			P_CalcPostImg(player);
		}
		else
		{

			player->deltaviewheight = 0;
			player->viewheight = FixedMul(41*player->height/48, player->mo->scale);
			if (player->mo->eflags & MFE_VERTICALFLIP)
				player->viewz = player->mo->z + player->mo->height - player->viewheight;
			else
				player->viewz = player->mo->z + player->viewheight;
			if (server || addedtogame)
				P_MoveChaseCamera(player, thiscam, false);  
		}
	}


	if ((netgame || multiplayer) && player->spectator)
	{
		player->mo->flags2 |= MF2_DONTDRAW;
		player->mo->flags |= MF_NOGRAVITY;
	}

	if (player->powers[pw_dye])
	{
		player->mo->colorized = true;
		player->mo->color = player->powers[pw_dye];
	}

	if (player->followmobj && (player->spectator || player->mo->health <= 0 || player->followmobj->type != player->followitem))
	{
		P_RemoveMobj(player->followmobj);
		P_SetTarget(&player->followmobj, NULL);
	}

	if (!player->spectator && player->mo->health && player->followitem)
	{
		if (!player->followmobj || P_MobjWasRemoved(player->followmobj))
		{
			P_SetTarget(&player->followmobj, P_SpawnMobjFromMobj(player->mo, 0, 0, 0, player->followitem));
			if (!P_MobjWasRemoved(player->followmobj))
			{
				P_SetTarget(&player->followmobj->tracer, player->mo);
				switch (player->followmobj->type)
				{
					case MT_METALJETFUME:
						player->followmobj->colorized = true;
						break;
					default:
						player->followmobj->flags2 |= MF2_LINKDRAW;
						break;
				}
			}
		}

		if (player->followmobj)
			P_DoFollowMobj(player, player->followmobj);
	}

	P_DoPlayerHeadSigns(player);  
}

void P_SetPlayerAngle(player_t *player, angle_t angle)
{
	INT16 delta = (INT16)(angle >> 16) - player->angleturn;

	P_ForceLocalAngle(player, P_GetLocalAngle(player) + (delta << 16));
	player->angleturn += delta;
}

void P_SetLocalAngle(player_t *player, angle_t angle)
{
	INT16 delta = (INT16)((angle - P_GetLocalAngle(player)) >> 16);

	P_ForceLocalAngle(player, P_GetLocalAngle(player) + (angle_t)(delta << 16));

	if (player == &players[consoleplayer])
		ticcmd_oldangleturn[0] += delta;
	else if (player == &players[secondarydisplayplayer])
		ticcmd_oldangleturn[1] += delta;
}

angle_t P_GetLocalAngle(player_t *player)
{
	if (player == &players[consoleplayer])
		return localangle;
	else if (player == &players[secondarydisplayplayer])
		return localangle2;
	else
		return 0;
}

void P_ForceLocalAngle(player_t *player, angle_t angle)
{
	angle = angle & ~UINT16_MAX;

	if (player == &players[consoleplayer])
		localangle = angle;
	else if (player == &players[secondarydisplayplayer])
		localangle2 = angle;
}

boolean P_PlayerFullbright(player_t *player)
{
	return (player->powers[pw_super]
		|| ((player->powers[pw_carry] == CR_NIGHTSMODE && (player->charflags & (SF_SUPER|SF_NONIGHTSSUPER)) == SF_SUPER)  
		&& (player->exiting
			|| !(player->mo->state >= &states[S_PLAY_NIGHTS_TRANS1]
			&& player->mo->state < &states[S_PLAY_NIGHTS_TRANS6]))));  
}

#define JUMPCURLED(player) ((player->pflags & PF_JUMPED)\
	&& (!(player->charflags & SF_NOJUMPSPIN))\
	&& (player->panim == PA_JUMP || player->panim == PA_ROLL))\


boolean P_PlayerCanEnterSpinGaps(player_t *player)
{
	UINT8 canEnter = LUA_HookPlayerCanEnterSpinGaps(player);
	if (canEnter == 1)
		return true;
	else if (canEnter == 2)
		return false;

	return ((player->pflags & (PF_SPINNING|PF_SLIDING|PF_GLIDING))  
		|| (player->charability == CA_GLIDEANDCLIMB && player->mo->state-states == S_PLAY_GLIDE_LANDING)  
		|| ((player->charflags & (SF_DASHMODE|SF_MACHINE)) == (SF_DASHMODE|SF_MACHINE)
			&& player->dashmode >= DASHMODE_THRESHOLD && player->mo->state-states == S_PLAY_DASH)  
		|| JUMPCURLED(player));  
}


boolean P_PlayerShouldUseSpinHeight(player_t *player)
{
	return ((player->pflags & (PF_SPINNING|PF_SLIDING|PF_GLIDING))
		|| (player->mo->state == &states[player->mo->info->painstate])
		|| (player->panim == PA_ROLL)
		|| ((player->powers[pw_tailsfly] || (player->charability == CA_FLY && player->mo->state-states == S_PLAY_FLY_TIRED))
			&& !(player->charflags & SF_NOJUMPSPIN))
		|| (player->charability == CA_GLIDEANDCLIMB && player->mo->state-states == S_PLAY_GLIDE_LANDING)
		|| ((player->charflags & (SF_DASHMODE|SF_MACHINE)) == (SF_DASHMODE|SF_MACHINE)
			&& player->dashmode >= DASHMODE_THRESHOLD && player->mo->state-states == S_PLAY_DASH)
		|| JUMPCURLED(player));
}

UINT16 P_GetPlayerColor(player_t *player)
{
	if (G_GametypeHasTeams() && player->ctfteam)
	{
		if (player->ctfteam == 1)
			return skincolor_redteam;
		else if (player->ctfteam == 2)
			return skincolor_blueteam;
	}

	return player->skincolor;
}
