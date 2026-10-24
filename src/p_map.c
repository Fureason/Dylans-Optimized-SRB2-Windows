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
#include "m_bbox.h"
#include "m_random.h"
#include "p_local.h"
#include "p_setup.h"  
#include "r_fps.h"
#include "r_state.h"
#include "r_main.h"
#include "r_sky.h"
#include "s_sound.h"
#include "w_wad.h"

#include "r_splats.h"

#include "p_slopes.h"

#include "z_zone.h"

#include "lua_hook.h"

#include "m_perfstats.h"  


#define MAXTRYMOVE (32*FRACUNIT)

fixed_t tmbbox[4];
mobj_t *tmthing;
static INT32 tmflags;
fixed_t tmx;
fixed_t tmy;

static precipmobj_t *tmprecipthing;
static fixed_t preciptmbbox[4];



boolean floatok;

fixed_t tmfloorz, tmceilingz;
static fixed_t tmdropoffz, tmdrpoffceilz;  
mobj_t *tmfloorthing;  
mobj_t *tmhitthing;  
ffloor_t *tmfloorrover, *tmceilingrover;
pslope_t *tmfloorslope, *tmceilingslope;



line_t *ceilingline;



line_t *blockingline;

msecnode_t *sector_list = NULL;
mprecipsecnode_t *precipsector_list = NULL;
camera_t *mapcampointer;








static boolean P_TeleportMove(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z)
{


	P_UnsetThingPosition(thing);


	if (sector_list)
	{
		P_DelSeclist(sector_list);
		sector_list = NULL;
	}

	thing->x = x;
	thing->y = y;
	thing->z = z;

	P_SetThingPosition(thing);

	P_CheckPosition(thing, thing->x, thing->y);

	if (P_MobjWasRemoved(thing))
		return true;

	thing->floorz = tmfloorz;
	thing->ceilingz = tmceilingz;
	thing->floorrover = tmfloorrover;
	thing->ceilingrover = tmceilingrover;

	return true;
}



boolean P_SetOrigin(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z)
{
	boolean result = P_TeleportMove(thing, x, y, z);

	if (result == true)
	{
		thing->old_x = thing->x;
		thing->old_y = thing->y;
		thing->old_z = thing->z;
	}

	return result;
}




boolean P_MoveOrigin(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z)
{
	return P_TeleportMove(thing, x, y, z);
}
























boolean P_DoSpring(mobj_t *spring, mobj_t *object)
{
	fixed_t vertispeed = spring->info->mass;
	fixed_t horizspeed = spring->info->damage;
	boolean final = false;
	UINT8 strong = 0;


	if (object->eflags & MFE_SPRUNG)
		return false;


	if (object->player && object->player->spectator)
		return false;


	if (!spring->health || !object->health)
		return false;

	if (object->player)
	{
		if (spring->info->painchance == 3)
			;
		else if (object->player->powers[pw_strong] & STR_SPRING)
			strong = 1;
	}

	if (spring->info->painchance == -1)  
	{




		angle_t horizangle, vertiangle;

		if (!vertispeed)
			return false;

		if (object->player && object->player->homing)  
		{
			horizangle = 0;
			vertiangle = ((object->eflags & MFE_VERTICALFLIP) ? ANGLE_270 : ANGLE_90) >> ANGLETOFINESHIFT;
			object->player->pflags &= ~PF_THOKKED;
			if (spring->eflags & MFE_VERTICALFLIP)
				object->z = spring->z - object->height - 1;
			else
				object->z = spring->z + spring->height + 1;
		}
		else
		{
			horizangle = R_PointToAngle2(spring->x, spring->y, object->x, object->y);
			vertiangle = (R_PointToAngle2(
							0,
							spring->z + spring->height/2,
							FixedHypot(object->x - spring->x, object->y - spring->y),
							object->z + object->height/2)
								>> ANGLETOFINESHIFT) & FINEMASK;
		}

		if (spring->spawnpoint && spring->spawnpoint->angle > 0)
			vertispeed = (spring->spawnpoint->angle<<(FRACBITS-1))/5;
		vertispeed = FixedMul(vertispeed, FixedMul(object->scale, spring->scale));

		if (object->player)
		{
			fixed_t playervelocity;

			if (strong)
				vertispeed <<= 1;

			if (!(object->player->pflags & PF_THOKKED) && !(object->player->homing)
			&& ((playervelocity = FixedDiv(9*FixedHypot(object->player->speed, object->momz), 10<<FRACBITS)) > vertispeed))
				vertispeed = playervelocity;

			if (object->player->powers[pw_carry] == CR_NIGHTSMODE)  
			{
				angle_t nightsangle = 0;

				if (object->player->bumpertime > (TICRATE/2)-5)
					return false;

				if ((object->player->pflags & PF_TRANSFERTOCLOSEST) && object->player->axis1 && object->player->axis2)
				{
					nightsangle = R_PointToAngle2(object->player->axis1->x, object->player->axis1->y, object->player->axis2->x, object->player->axis2->y);
					nightsangle += ANGLE_90;
				}
				else if (object->target)
				{
					if (object->target->flags2 & MF2_AMBUSH)
						nightsangle = R_PointToAngle2(object->target->x, object->target->y, object->x, object->y);
					else
						nightsangle = R_PointToAngle2(object->x, object->y, object->target->x, object->target->y);
				}

				object->player->flyangle = AngleFixed(R_PointToAngle2(
					0,
					spring->z + spring->height/2,
					FixedMul(
						FINESINE(((nightsangle - horizangle) >> ANGLETOFINESHIFT) & FINEMASK),
						FixedHypot(object->x - spring->x, object->y - spring->y)),
					object->z + object->height/2))>>FRACBITS;

				object->player->bumpertime = TICRATE/2;
			}
			else
			{
				INT32 pflags = object->player->pflags & (PF_JUMPED|PF_NOJUMPDAMAGE|PF_SPINNING|PF_THOKKED|PF_BOUNCING|PF_CANCARRY);  
				UINT8 secondjump = object->player->secondjump;
				UINT16 tailsfly = object->player->powers[pw_tailsfly];
				if (object->player->pflags & PF_GLIDING)
					P_SetMobjState(object, S_PLAY_FALL);
				P_ResetPlayer(object->player);
				object->player->pflags |= pflags;
				object->player->secondjump = secondjump;
				object->player->powers[pw_tailsfly] = tailsfly;
			}
		}

		if (!P_IsObjectOnGround(object))  
			object->momz = FixedMul(vertispeed, FINESINE(vertiangle));
		P_InstaThrust(object, horizangle, FixedMul(vertispeed, FINECOSINE(vertiangle)));

		object->eflags |= MFE_SPRUNG;  

		goto springstate;
	}


	if (!vertispeed && !horizspeed)
		return false;

	object->standingslope = NULL;  

	if (spring->eflags & MFE_VERTICALFLIP)
		vertispeed *= -1;

	if (object->player && (object->player->powers[pw_carry] == CR_NIGHTSMODE))
	{
		 
		return false;
	}

	if (strong)
	{
		if (horizspeed)
			horizspeed = FixedMul(horizspeed, (4*FRACUNIT)/3);
		if (vertispeed)
			vertispeed = FixedMul(vertispeed, (6*FRACUNIT)/5);  
	}

	object->eflags |= MFE_SPRUNG;  
	spring->flags &= ~(MF_SPRING|MF_SPECIAL);  

	if (spring->info->painchance != 2)
	{
		if (object->player)
		{
			object->player->pflags &= ~PF_APPLYAUTOBRAKE;
#ifndef SPRINGSPIN
			object->player->powers[pw_justsprung] = 5;
			if (horizspeed)
				object->player->powers[pw_noautobrake] = ((horizspeed*TICRATE)>>(FRACBITS+3))/9;  
			else if (P_MobjFlip(object) == P_MobjFlip(spring))
				object->player->powers[pw_justsprung] |= (1<<15);
#else
			object->player->powers[pw_justsprung] = 15;
			if (horizspeed)
				object->player->powers[pw_noautobrake] = ((horizspeed*TICRATE)>>(FRACBITS+3))/9;  
			else
			{
				if (abs(object->player->rmomx) > object->scale || abs(object->player->rmomy) > object->scale)
					object->player->drawangle = R_PointToAngle2(0, 0, object->player->rmomx, object->player->rmomy);
				if (P_MobjFlip(object) == P_MobjFlip(spring))
					object->player->powers[pw_justsprung] |= (1<<15);
			}
#endif
		}

		if ((horizspeed && vertispeed) || (object->player && object->player->homing))  
		{
			object->momx = object->momy = 0;
			P_TryMove(object, spring->x, spring->y, true);
		}

		if (vertispeed > 0)
			object->z = spring->z + spring->height + 1;
		else if (vertispeed < 0)
			object->z = spring->z - object->height - 1;
		else
		{
			fixed_t offx, offy;

			object->momx = object->momy = 0;


			offx = P_ReturnThrustX(spring, spring->angle, (spring->radius + object->radius + 1) * 2);
			offy = P_ReturnThrustY(spring, spring->angle, (spring->radius + object->radius + 1) * 2);


			if (offx > (spring->radius + object->radius + 1))
				offx = spring->radius + object->radius + 1;
			else if (offx < -(spring->radius + object->radius + 1))
				offx = -(spring->radius + object->radius + 1);

			if (offy > (spring->radius + object->radius + 1))
				offy = spring->radius + object->radius + 1;
			else if (offy < -(spring->radius + object->radius + 1))
				offy = -(spring->radius + object->radius + 1);


			P_TryMove(object, spring->x + offx, spring->y + offy, true);

			if ((spring->info->painchance == 3))
			{
				object->z = spring->z;
				if (spring->eflags & MFE_VERTICALFLIP)
					object->z -= object->height;
				object->momz = 0;
			}
		}
	}

	if (vertispeed)
		object->momz = FixedMul(vertispeed,FixedSqrt(FixedMul(object->scale, spring->scale)));

	if (horizspeed)
		P_InstaThrustEvenIn2D(object, spring->angle, FixedMul(horizspeed,FixedSqrt(FixedMul(object->scale, spring->scale))));


	spring->flags |= (spring->info->flags & (MF_SPRING|MF_SPECIAL));

	if (object->player)
	{
		INT32 pflags;
		UINT8 secondjump;

		if (spring->flags & MF_ENEMY)  
			P_SetTarget(&spring->target, object);

		if (horizspeed)
		{
			object->angle = object->player->drawangle = spring->angle;

			if (!demoplayback || P_ControlStyle(object->player) == CS_LMAOGALOG)
				P_SetPlayerAngle(object->player, spring->angle);
		}

		if (object->player->pflags & PF_GLIDING)
			P_SetMobjState(object, S_PLAY_FALL);
		if ((spring->info->painchance == 3))
		{
			if (!(pflags = (object->player->pflags & PF_SPINNING)) &&
				(((object->player->charability2 == CA2_SPINDASH) && (object->player->cmd.buttons & BT_SPIN))
				|| (spring->flags2 & MF2_AMBUSH)))
			{
				pflags = PF_SPINNING;
				P_SetMobjState(object, S_PLAY_ROLL);
				S_StartSound(object, sfx_spin);
			}
			else
				P_SetMobjState(object, S_PLAY_ROLL);
		}
		else
		{
			boolean wasSpindashing = object->player->dashspeed > 0 && (object->player->charability2 == CA2_SPINDASH);

			pflags = object->player->pflags & (PF_STARTJUMP | PF_JUMPED | PF_NOJUMPDAMAGE | PF_SPINNING | PF_BOUNCING);  

			if (wasSpindashing)  
				P_SetMobjState(object, S_PLAY_ROLL);

			if (object->player->charability == CA_GLIDEANDCLIMB && object->player->skidtime && (pflags & PF_JUMPED))
			{
				object->player->skidtime = 0;  
				pflags &= ~PF_JUMPED;
			}
		}
		secondjump = object->player->secondjump;
		P_ResetPlayer(object->player);

		if (spring->info->painchance == 1)  
		{
			object->player->pflags |= P_GetJumpFlags(object->player);
			P_SetMobjState(object, S_PLAY_JUMP);
		}
		else if ((spring->info->painchance == 2) || ((spring->info->painchance != 3) && (pflags & PF_BOUNCING)))  
		{
			object->player->pflags |= (pflags &~ PF_STARTJUMP);
			object->player->secondjump = secondjump;
		}
		else if (!vertispeed)
		{
			if (pflags & (PF_JUMPED|PF_SPINNING))
			{
				object->player->pflags |= pflags;
				object->player->secondjump = secondjump;
			}
			else if (object->player->dashmode >= DASHMODE_THRESHOLD)
				P_SetMobjState(object, S_PLAY_DASH);
			else if (P_IsObjectOnGround(object))
				P_SetMobjState(object, (horizspeed >= FixedMul(object->player->runspeed, object->scale)) ? S_PLAY_RUN : S_PLAY_WALK);
			else
				P_SetMobjState(object, (object->momz > 0) ? S_PLAY_SPRING : S_PLAY_FALL);
		}
		else if (P_MobjFlip(object)*vertispeed > 0)
			P_SetMobjState(object, S_PLAY_SPRING);
		else
			P_SetMobjState(object, S_PLAY_FALL);
	}
	else if (horizspeed
		&& object->tracer
		&& object->tracer->player
		&& object->tracer->player->powers[pw_carry] != CR_NONE
		&& object->tracer->tracer == object
		&& (!demoplayback || P_ControlStyle(object->tracer->player) == CS_LMAOGALOG))
			P_SetPlayerAngle(object->tracer->player, spring->angle);

	object->standingslope = NULL;  

	final = true;

springstate:
	if ((statenum_t)(spring->state-states) < spring->info->raisestate)
	{
		P_SetMobjState(spring, spring->info->raisestate);
		if (object->player && spring->reactiontime && !(spring->info->flags & MF_ENEMY))
		{
			if (object->player->powers[pw_carry] != CR_NIGHTSMODE)  
				P_SetMobjState(P_SpawnMobj(spring->x, spring->y, spring->z + (spring->height/2), MT_SCORE), mobjinfo[MT_SCORE].spawnstate+11);
			P_AddPlayerScore(object->player, 10);
			spring->reactiontime--;
		}

		if (strong)
		{
			if (object->player->charability == CA_TWINSPIN || object->player->charability2 == CA2_MELEE)
				P_TwinSpinRejuvenate(object->player, (object->player->charability == CA_TWINSPIN ? object->player->thokitem : object->player->revitem));
			S_StartSound(object, sfx_sprong);  
		}
	}

	return final;
}

static void P_DoFan(mobj_t *fan, mobj_t *object)
{
	player_t *p = object->player;  
	fixed_t zdist;  
	fixed_t speed = fan->info->mass;  
	SINT8 flipval = P_MobjFlip(fan);  

	if (p && object->state == &states[object->info->painstate])  
		return;


	if (fan->eflags & MFE_VERTICALFLIP)
	{
		if (object->z > fan->z + fan->height)
			return;
		zdist = (fan->z + fan->height) - (object->z + object->height);
	}
	else
	{
		if (object->z + object->height < fan->z)
			return;
		zdist = object->z - fan->z;
	}

	object->standingslope = NULL;  

	switch (fan->type)
	{
		case MT_FAN:  
			if (zdist > (fan->health << FRACBITS))  
				break;
			if (flipval*object->momz >= FixedMul(speed, fan->scale))  
				break;
			if (p && (p->climbing || p->pflags & PF_GLIDING))  
				break;

			object->momz += flipval*FixedMul(speed/4, fan->scale);


			if (flipval*object->momz > FixedMul(speed, fan->scale))
				object->momz = flipval*FixedMul(speed, fan->scale);

			if (p && !p->powers[pw_tailsfly] && !p->powers[pw_carry])  
			{
				P_ResetPlayer(p);
				P_SetMobjState(object, S_PLAY_FALL);
				P_SetTarget(&object->tracer, fan);
				p->powers[pw_carry] = CR_FAN;
			}
			break;
		default:
			break;
	}
}

static void P_DoPterabyteCarry(player_t *player, mobj_t *ptera)
{
	if (player->powers[pw_carry] && player->powers[pw_carry] != CR_ROLLOUT)
		return;
	if (player->powers[pw_ignorelatch] & (1<<15))
		return;
	if (ptera->extravalue1 != 1)
		return;  
	if (ptera->target != player->mo)
		return;  

	if (player->spectator)
		return;

	if ((player->mo->eflags & MFE_VERTICALFLIP) != (ptera->eflags & MFE_VERTICALFLIP))
		return;  

	if (ptera->eflags & MFE_VERTICALFLIP)
	{
		if (ptera->ceilingz - (ptera->z + ptera->height) < player->mo->height - FixedMul(2*FRACUNIT, player->mo->scale))
			return;
	}
	else if (ptera->z - ptera->floorz < player->mo->height - FixedMul(2*FRACUNIT, player->mo->scale))
		return;  

	P_ResetPlayer(player);
	P_SetTarget(&player->mo->tracer, ptera);
	player->pflags &= ~PF_APPLYAUTOBRAKE;
	player->powers[pw_carry] = CR_PTERABYTE;
	S_StartSound(player->mo, sfx_s3k4a);
	P_UnsetThingPosition(player->mo);
	player->mo->x = ptera->x;
	player->mo->y = ptera->y;
	P_SetThingPosition(player->mo);
	ptera->movefactor = 3*TICRATE;  
	ptera->watertop = ptera->waterbottom = ptera->cusval = 0;
}

static void P_DoTailsCarry(player_t *sonic, player_t *tails)
{
	INT32 p;
	fixed_t zdist;  

	if (tails->powers[pw_carry])
		return;
	if (sonic->powers[pw_carry])
		return;

	if (tails->spectator)
		return;
	if (sonic->spectator)
		return;

	if (!(tails->pflags & PF_CANCARRY))
		return;

	if ((sonic->mo->eflags & MFE_VERTICALFLIP) != (tails->mo->eflags & MFE_VERTICALFLIP))
		return;  

	if (tails->mo->eflags & MFE_VERTICALFLIP)
	{
		if (tails->mo->ceilingz - (tails->mo->z + tails->mo->height) < sonic->mo->height-FixedMul(2*FRACUNIT, sonic->mo->scale))
			return;
	}
	else if (tails->mo->z - tails->mo->floorz < sonic->mo->height-FixedMul(2*FRACUNIT, sonic->mo->scale))
		return;  


	for (p = 0; p < MAXPLAYERS; p++)
		if (playeringame[p] && players[p].mo
		&& players[p].powers[pw_carry] == CR_PLAYER && players[p].mo->tracer == tails->mo)
			return;



	  
                               
                                                        
                                                                                      
                           
                                                                
            

	if (tails->mo->eflags & MFE_VERTICALFLIP)
		zdist = (sonic->mo->z + sonic->mo->height) - (tails->mo->z + tails->mo->height);
	else
		zdist = tails->mo->z - sonic->mo->z;

	if (zdist <= sonic->mo->height + sonic->mo->scale  
		&& zdist > sonic->mo->height*2/3
		&& P_MobjFlip(tails->mo)*sonic->mo->momz <= 0
		&& !(sonic->powers[pw_ignorelatch] & (1<<15)))
	{
		if (sonic-players == consoleplayer && botingame)
			CV_SetValue(&cv_analog[1], false);
		P_ResetPlayer(sonic);
		P_SetTarget(&sonic->mo->tracer, tails->mo);
		sonic->powers[pw_carry] = CR_PLAYER;
		S_StartSound(sonic->mo, sfx_s3k4a);
		P_UnsetThingPosition(sonic->mo);
		sonic->mo->x = tails->mo->x;
		sonic->mo->y = tails->mo->y;
		P_SetThingPosition(sonic->mo);
	}
	else {
		if (sonic-players == consoleplayer && botingame)
			CV_SetValue(&cv_analog[1], true);
		P_SetTarget(&sonic->mo->tracer, NULL);
		sonic->powers[pw_carry] = CR_NONE;
	}
}


static void P_SlapStick(mobj_t *fang, mobj_t *pole)
{
	fixed_t momx1, momx2, momy1, momy2;

#define dist 3
	momx1 = pole->momx/dist;
	momy1 = pole->momy/dist;
	momx2 = fang->momx/dist;
	momy2 = fang->momy/dist;

	pole->tracer->tracer->momx = momx1 + (dist-1)*momx2;
	pole->tracer->tracer->momy = momy1 + (dist-1)*momy2;
	fang->momx = (dist-1)*momx1 + momx2;
	fang->momy = (dist-1)*momy1 + momy2;
#undef dist

	P_SetObjectMomZ(pole->tracer->tracer, 6*FRACUNIT, false);
	pole->tracer->tracer->flags &= ~(MF_NOGRAVITY|MF_NOCLIP);
	pole->tracer->tracer->movedir = ANGLE_67h;
	if ((R_PointToAngle(fang->x - pole->tracer->tracer->x, fang->y - pole->tracer->tracer->y) - pole->angle) > ANGLE_180)
		pole->tracer->tracer->movedir = InvAngle(pole->tracer->movedir);

	P_SetObjectMomZ(fang, 14*FRACUNIT, false);
	fang->flags |= MF_NOGRAVITY|MF_NOCLIP;
	P_SetMobjState(fang, fang->info->xdeathstate);

	pole->tracer->tracer->tics = pole->tracer->tics = pole->tics = fang->tics;

	var1 = var2 = 0;
	A_Scream(pole->tracer->tracer);
	S_StartSound(fang, sfx_altdi1);

	P_SetTarget(&pole->tracer->tracer, NULL);
	P_SetMobjState(pole->tracer, pole->info->xdeathstate);
	P_SetTarget(&pole->tracer, NULL);
	P_SetMobjState(pole, pole->info->deathstate);
}

static void P_PlayerBarrelCollide(mobj_t *toucher, mobj_t *barrel)
{
	if (toucher->momz < 0)
	{
		if (toucher->z + toucher->momz > barrel->z + barrel->height)
			return;
	}
	else
	{
		if (toucher->z > barrel->z + barrel->height)
			return;
	}

	if (toucher->momz > 0)
	{
		if (toucher->z + toucher->height + toucher->momz < barrel->z)
			return;
	}
	else
	{
		if (toucher->z + toucher->height < barrel->z)
			return;
	}

	if (P_PlayerCanDamage(toucher->player, barrel))
		P_DamageMobj(barrel, toucher, toucher, 1, 0);
}

enum
{
	CHECKTHING_NOCOLLIDE,
	CHECKTHING_COLLIDE,
	CHECKTHING_DONE,
	CHECKTHING_IGNORE
};

static unsigned PIT_DoCheckThing(mobj_t *thing)
{
	fixed_t blockdist;


	if (thing == tmthing)
		return CHECKTHING_IGNORE;


	if (!tmthing || !thing || P_MobjWasRemoved(thing))
		return CHECKTHING_IGNORE;

	I_Assert(!P_MobjWasRemoved(tmthing));
	I_Assert(!P_MobjWasRemoved(thing));


	if ((tmthing->player && tmthing->player->spectator)
	|| (thing->player && thing->player->spectator))
		return CHECKTHING_IGNORE;



	if (tmthing->type == MT_NAMECHECK)
	{

		if (!thing->player || !(tmthing->target && tmthing->target->player) || thing->player->spectator || (tmthing->target && thing->player == tmthing->target->player))
			return CHECKTHING_IGNORE;


		blockdist = thing->radius + tmthing->radius;
		if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
			return CHECKTHING_NOCOLLIDE;  

		if (tmthing->z > thing->z + thing->height)
			return CHECKTHING_NOCOLLIDE;  
		if (tmthing->z + tmthing->height < thing->z)
			return CHECKTHING_NOCOLLIDE;  


		if (!LUA_HookSeenPlayer(tmthing->target->player, thing->player))
			return CHECKTHING_DONE;

		seenplayer = thing->player;
		return CHECKTHING_DONE;
	}


	if (tmthing->type == MT_METALSONIC_RACE
	&& (thing->flags & (MF_MISSILE|MF_ENEMY|MF_BOSS)
	|| (thing->type == MT_SPIKE
	|| thing->type == MT_WALLSPIKE)))
	{
		if ((thing->flags & (MF_ENEMY|MF_BOSS)) && (thing->health <= 0 || !(thing->flags & MF_SHOOTABLE)))
			return CHECKTHING_IGNORE;
		blockdist = thing->radius + tmthing->radius;
		if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
			return CHECKTHING_NOCOLLIDE;  

		if (tmthing->z > thing->z + thing->height)
			return CHECKTHING_NOCOLLIDE;  
		if (tmthing->z + tmthing->height < thing->z)
			return CHECKTHING_NOCOLLIDE;  
		if (thing->type == MT_SPIKE
		|| thing->type == MT_WALLSPIKE)
		{
			mobj_t *iter;
			if (thing->flags & MF_SOLID)
				S_StartSound(tmthing, thing->info->deathsound);
			for (iter = thing->subsector->sector->thinglist; iter; iter = iter->snext)
				if (iter->type == thing->type && iter->health > 0 && iter->flags & MF_SOLID && (iter == thing || P_AproxDistance(P_AproxDistance(thing->x - iter->x, thing->y - iter->y), thing->z - iter->z) < 56*thing->scale)) 
					P_KillMobj(iter, tmthing, tmthing, 0);
		}
		else
		{
			thing->health = 0;
			P_KillMobj(thing, tmthing, tmthing, 0);
		}
		return CHECKTHING_COLLIDE;
	}


	if ((tmthing->player) && ((tmthing->player->powers[pw_strong] & STR_SPIKE) && (thing->type == MT_SPIKE || thing->type == MT_WALLSPIKE)))
	{
		mobj_t *iter;
		blockdist = thing->radius + tmthing->radius;
		if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
			return CHECKTHING_NOCOLLIDE;  

		if (tmthing->z > thing->z + thing->height)
			return CHECKTHING_NOCOLLIDE;  
		if (tmthing->z + tmthing->height < thing->z)
			return CHECKTHING_NOCOLLIDE;  

		if (thing->flags & MF_SOLID)
			S_StartSound(tmthing, thing->info->deathsound);
		for (iter = thing->subsector->sector->thinglist; iter; iter = iter->snext)
			if (iter->type == thing->type && iter->health > 0 && iter->flags & MF_SOLID && (iter == thing || P_AproxDistance(P_AproxDistance(thing->x - iter->x, thing->y - iter->y), thing->z - iter->z) < 56*thing->scale)) 
				P_KillMobj(iter, tmthing, tmthing, 0);
		return CHECKTHING_COLLIDE;
	}


	if (thing->type == MT_METALSONIC_BATTLE
	&& (tmthing->flags & MF_MISSILE)
	&& tmthing->target != thing
	&& thing->state == &states[thing->info->spawnstate])
	{
		blockdist = thing->radius + tmthing->radius;

		if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
			return CHECKTHING_NOCOLLIDE;  

		if (tmthing->z > thing->z + thing->height)
			return CHECKTHING_NOCOLLIDE;  
		if (tmthing->z + tmthing->height < thing->z)
			return CHECKTHING_NOCOLLIDE;  

		thing->flags2 |= MF2_CLASSICPUSH;

		return CHECKTHING_COLLIDE;
	}

	if ((thing->flags & MF_NOCLIPTHING) || !(thing->flags & (MF_SOLID|MF_SPECIAL|MF_PAIN|MF_SHOOTABLE|MF_SPRING)))
		return CHECKTHING_IGNORE;


	if (tmthing->player && thing->player && (maptol & TOL_NIGHTS)
		&& ((tmthing->player->powers[pw_carry] == CR_NIGHTSMODE) || (thing->player->powers[pw_carry] == CR_NIGHTSMODE)))
		return CHECKTHING_IGNORE;

	blockdist = thing->radius + tmthing->radius;

	if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
		return CHECKTHING_NOCOLLIDE;  

	if (thing->flags & MF_PAPERCOLLISION)  
	{
		fixed_t cosradius, sinradius;
		vertex_t v1, v2;  
		line_t junk;  

		cosradius = FixedMul(thing->radius, FINECOSINE(thing->angle>>ANGLETOFINESHIFT));
		sinradius = FixedMul(thing->radius, FINESINE(thing->angle>>ANGLETOFINESHIFT));

		v1.x = thing->x - cosradius;
		v1.y = thing->y - sinradius;
		v2.x = thing->x + cosradius;
		v2.y = thing->y + sinradius;

		junk.v1 = &v1;
		junk.v2 = &v2;
		junk.dx = 2*cosradius;  
		junk.dy = 2*sinradius;  

		if (tmthing->flags & MF_PAPERCOLLISION)  
		{
			INT32 check1, check2, check3, check4;
			fixed_t tmcosradius = FixedMul(tmthing->radius, FINECOSINE(tmthing->angle>>ANGLETOFINESHIFT));
			fixed_t tmsinradius = FixedMul(tmthing->radius, FINESINE(tmthing->angle>>ANGLETOFINESHIFT));
			if (abs(thing->x - tmx) >= (abs(tmcosradius) + abs(cosradius)) || abs(thing->y - tmy) >= (abs(tmsinradius) + abs(sinradius)))
				return CHECKTHING_NOCOLLIDE;  
			check1 = P_PointOnLineSide(tmx - tmcosradius, tmy - tmsinradius, &junk);
			check2 = P_PointOnLineSide(tmx + tmcosradius, tmy + tmsinradius, &junk);
			check3 = P_PointOnLineSide(tmx + tmthing->momx - tmcosradius, tmy + tmthing->momy - tmsinradius, &junk);
			check4 = P_PointOnLineSide(tmx + tmthing->momx + tmcosradius, tmy + tmthing->momy + tmsinradius, &junk);
			if ((check1 == check2) && (check2 == check3) && (check3 == check4))
				return CHECKTHING_NOCOLLIDE;  
		}
		else
		{
			if (abs(thing->x - tmx) >= (tmthing->radius + abs(cosradius)) || abs(thing->y - tmy) >= (tmthing->radius + abs(sinradius)))
				return CHECKTHING_NOCOLLIDE;  
			if ((P_PointOnLineSide(tmx - tmthing->radius, tmy - tmthing->radius, &junk)
			== P_PointOnLineSide(tmx + tmthing->radius, tmy + tmthing->radius, &junk))
			&& (P_PointOnLineSide(tmx + tmthing->radius, tmy - tmthing->radius, &junk)
			== P_PointOnLineSide(tmx - tmthing->radius, tmy + tmthing->radius, &junk)))
				return CHECKTHING_NOCOLLIDE;  
		}
	}
	else if (tmthing->flags & MF_PAPERCOLLISION)
	{
		fixed_t tmcosradius, tmsinradius;
		vertex_t v1, v2;  
		line_t junk;  

		tmcosradius = FixedMul(tmthing->radius, FINECOSINE(tmthing->angle>>ANGLETOFINESHIFT));
		tmsinradius = FixedMul(tmthing->radius, FINESINE(tmthing->angle>>ANGLETOFINESHIFT));

		if (abs(thing->x - tmx) >= (thing->radius + abs(tmcosradius)) || abs(thing->y - tmy) >= (thing->radius + abs(tmsinradius)))
			return CHECKTHING_NOCOLLIDE;  

		v1.x = tmx - tmcosradius;
		v1.y = tmy - tmsinradius;
		v2.x = tmx + tmcosradius;
		v2.y = tmy + tmsinradius;

		junk.v1 = &v1;
		junk.v2 = &v2;
		junk.dx = 2*tmcosradius;  
		junk.dy = 2*tmsinradius;  


		if ((P_PointOnLineSide(thing->x - thing->radius, thing->y - thing->radius, &junk)
		== P_PointOnLineSide(thing->x + thing->radius, thing->y + thing->radius, &junk))
		&& (P_PointOnLineSide(thing->x + thing->radius, thing->y - thing->radius, &junk)
		== P_PointOnLineSide(thing->x - thing->radius, thing->y + thing->radius, &junk)))
			return CHECKTHING_NOCOLLIDE;  
	}

	{
		UINT8 shouldCollide = LUA_Hook2Mobj(thing, tmthing, MOBJ_HOOK(MobjCollide));  
		if (P_MobjWasRemoved(tmthing) || P_MobjWasRemoved(thing))
			return CHECKTHING_NOCOLLIDE;  
		if (shouldCollide == 1)
			return CHECKTHING_DONE;  
		else if (shouldCollide == 2)
			return CHECKTHING_NOCOLLIDE;  

		shouldCollide = LUA_Hook2Mobj(tmthing, thing, MOBJ_HOOK(MobjMoveCollide));  
		if (P_MobjWasRemoved(tmthing) || P_MobjWasRemoved(thing))
			return CHECKTHING_NOCOLLIDE;  
		if (shouldCollide == 1)
			return CHECKTHING_DONE;  
		else if (shouldCollide == 2)
			return CHECKTHING_NOCOLLIDE;  
	}

	if (tmthing->type == MT_LAVAFALL_LAVA && (thing->type == MT_RING || thing->type == MT_REDTEAMRING || thing->type == MT_BLUETEAMRING || thing->type == MT_FLINGRING))
	{

		if (tmthing->z > thing->z + thing->height || thing->z > tmthing->z + tmthing->height || !(thing->health))
			return CHECKTHING_NOCOLLIDE;

		P_KillMobj(thing, tmthing, tmthing, DMG_FIRE);
	}

	if (tmthing->type == MT_MINECART)
	{

		if (tmthing->z > thing->z + thing->height || thing->z > tmthing->z + tmthing->height || !(thing->health))
			return CHECKTHING_NOCOLLIDE;

		if (thing->type == MT_TNTBARREL)
			P_KillMobj(thing, tmthing, tmthing->target, 0);
		else if ((thing->flags & MF_MONITOR) || (thing->flags & MF_ENEMY))
		{
			P_KillMobj(thing, tmthing, tmthing->target, 0);
			if (tmthing->momz*P_MobjFlip(tmthing) < 0)
				tmthing->momz = abs(tmthing->momz)*P_MobjFlip(tmthing);
		}
	}

	if (thing->type == MT_SALOONDOOR && tmthing->player)
	{
		mobj_t *ref = (tmthing->player->powers[pw_carry] == CR_MINECART && tmthing->tracer && !P_MobjWasRemoved(tmthing->tracer)) ? tmthing->tracer : tmthing;
		if (((thing->flags2 & MF2_AMBUSH) && (tmthing->z <= thing->z + thing->height) && (tmthing->z + tmthing->height >= thing->z))
			|| ref != tmthing)
		{
			fixed_t dm = min(FixedHypot(ref->momx, ref->momy), 16*FRACUNIT);
			angle_t ang = R_PointToAngle2(0, 0, ref->momx, ref->momy) - thing->angle;
			fixed_t s = FINESINE((ang >> ANGLETOFINESHIFT) & FINEMASK);
			S_StartSound(tmthing, thing->info->activesound);
			thing->extravalue2 += 2*FixedMul(s, dm)/3;
			return CHECKTHING_COLLIDE;
		}
	}

	if (thing->type == MT_SALOONDOORCENTER && tmthing->player)
	{
		if (((thing->flags2 & MF2_AMBUSH) && (tmthing->z <= thing->z + thing->height) && (tmthing->z + tmthing->height >= thing->z))
			|| (tmthing->player->powers[pw_carry] == CR_MINECART && tmthing->tracer && !P_MobjWasRemoved(tmthing->tracer)))
			return CHECKTHING_COLLIDE;
	}

	if (thing->type == MT_ROLLOUTROCK && tmthing->player && tmthing->health)
	{
		if (tmthing->player->powers[pw_carry] == CR_ROLLOUT)
		{
			return CHECKTHING_NOCOLLIDE;
		}
		if ((thing->flags & MF_PUSHABLE)  
			&& (tmthing->player->powers[pw_carry] == CR_NONE)  
			&& !(tmthing->player->powers[pw_ignorelatch] & (1<<15))
			&& (P_MobjFlip(tmthing)*tmthing->momz <= 0)
			&& ((!(tmthing->eflags & MFE_VERTICALFLIP) && abs(thing->z + thing->height - tmthing->z) < (thing->height>>2))
				|| (tmthing->eflags & MFE_VERTICALFLIP && abs(tmthing->z + tmthing->height - thing->z) < (thing->height>>2))))
		{
			thing->flags &= ~MF_PUSHABLE;  
			thing->flags2 &= ~MF2_DONTDRAW;  
			P_SetTarget(&thing->tracer, tmthing);
			P_ResetPlayer(tmthing->player);
			P_SetMobjState(tmthing, S_PLAY_WALK);
			tmthing->player->powers[pw_carry] = CR_ROLLOUT;
			P_SetTarget(&tmthing->tracer, thing);
			if (!P_IsObjectOnGround(thing))
				thing->momz += tmthing->momz;

			return CHECKTHING_COLLIDE;
		}
	}
	else if (tmthing->type == MT_ROLLOUTROCK)
	{
		if (tmthing->z > thing->z + thing->height || thing->z > tmthing->z + tmthing->height || !thing->health)
			return CHECKTHING_NOCOLLIDE;

		if (thing == tmthing->tracer)  
			return CHECKTHING_IGNORE;

		if (thing->flags & MF_SPRING)  
		{
			P_DoSpring(thing, tmthing);
			return CHECKTHING_COLLIDE;
		}
		else if ((thing->flags & (MF_MONITOR|MF_SHOOTABLE)) == (MF_MONITOR|MF_SHOOTABLE) && !(tmthing->flags & MF_PUSHABLE))  
		{
			P_KillMobj(thing, tmthing, tmthing->tracer, 0);
			return CHECKTHING_COLLIDE;
		}

		if (thing->type == tmthing->type  
			&& (tmthing->momx || tmthing->momy || thing->momx || thing->momy))
		{
			fixed_t tempmomx = thing->momx, tempmomy = thing->momy;
			thing->momx = tmthing->momx;
			thing->momy = tmthing->momy;
			tmthing->momx = tempmomx;
			tmthing->momy = tempmomy;
			S_StartSound(thing, thing->info->painsound);
		}
	}

	if (thing->type == MT_PTERABYTE && tmthing->player)
		P_DoPterabyteCarry(tmthing->player, thing);

	if (thing->type == MT_TNTBARREL && tmthing->player)
		P_PlayerBarrelCollide(tmthing, thing);

	if (thing->type == MT_VULTURE && tmthing->type == MT_VULTURE)
	{
		fixed_t dx = thing->x - tmthing->x;
		fixed_t dy = thing->y - tmthing->y;
		fixed_t dz = thing->z - tmthing->z;
		fixed_t dm = FixedHypot(dz, FixedHypot(dx, dy));
		thing->momx += FixedDiv(dx, dm);
		thing->momy += FixedDiv(dy, dm);
		thing->momz += FixedDiv(dz, dm);
	}

	if (tmthing->type == MT_FANG && thing->type == MT_FSGNB)
	{
		if (thing->z > tmthing->z + tmthing->height)
			return CHECKTHING_NOCOLLIDE;  
		if (thing->z + thing->height < tmthing->z)
			return CHECKTHING_NOCOLLIDE;  
		if (!thing->tracer || !thing->tracer->tracer)
			return CHECKTHING_IGNORE;
		P_SlapStick(tmthing, thing);



	}


	if (thing->type == MT_BIGMINE)
	{
		if (tmthing->type == MT_BIGMINE)
		{
			if (!tmthing->momx && !tmthing->momy)
				return CHECKTHING_IGNORE;
			if ((statenum_t)(thing->state-states) >= thing->info->meleestate)
				return CHECKTHING_IGNORE;
			if (thing->z > tmthing->z + tmthing->height)
				return CHECKTHING_NOCOLLIDE;  
			if (thing->z + thing->height < tmthing->z)
				return CHECKTHING_NOCOLLIDE;  

			thing->momx = tmthing->momx/3;
			thing->momy = tmthing->momy/3;
			thing->momz = tmthing->momz/3;
			tmthing->momx /= -8;
			tmthing->momy /= -8;
			tmthing->momz /= -8;
			if (thing->info->activesound)
				S_StartSound(thing, thing->info->activesound);
			P_SetMobjState(thing, thing->info->meleestate);
			P_SetTarget(&thing->tracer, tmthing->tracer);
			return CHECKTHING_COLLIDE;
		}
		else if (tmthing->type == MT_CRUSHCLAW)
		{
			if (tmthing->extravalue1 <= 0)
				return CHECKTHING_IGNORE;
			if ((statenum_t)(thing->state-states) >= thing->info->meleestate)
				return CHECKTHING_IGNORE;
			if (thing->z > tmthing->z + tmthing->height)
				return CHECKTHING_NOCOLLIDE;  
			if (thing->z + thing->height < tmthing->z)
				return CHECKTHING_NOCOLLIDE;  

			thing->momx = P_ReturnThrustX(tmthing, tmthing->angle, 2*tmthing->extravalue1*tmthing->scale/3);
			thing->momy = P_ReturnThrustY(tmthing, tmthing->angle, 2*tmthing->extravalue1*tmthing->scale/3);
			if (thing->info->activesound)
				S_StartSound(thing, thing->info->activesound);
			P_SetMobjState(thing, thing->info->meleestate);
			if (tmthing->tracer)
				P_SetTarget(&thing->tracer, tmthing->tracer->target);
			return CHECKTHING_DONE;
		}
	}


	if (tmthing->type == MT_SPIKE && (thing->flags & MF_SOLID) && (tmthing->flags & MF_SOLID))
	{
		if (thing->z > tmthing->z + tmthing->height)
			return CHECKTHING_NOCOLLIDE;  
		if (thing->z + thing->height < tmthing->z)
			return CHECKTHING_NOCOLLIDE;  

		if (tmthing->eflags & MFE_VERTICALFLIP)
			P_SetOrigin(thing, thing->x, thing->y, tmthing->z - thing->height - FixedMul(FRACUNIT, tmthing->scale));
		else
			P_SetOrigin(thing, thing->x, thing->y, tmthing->z + tmthing->height + FixedMul(FRACUNIT, tmthing->scale));
		if (thing->flags & MF_SHOOTABLE)
			P_DamageMobj(thing, tmthing, tmthing, 1, DMG_SPIKE);
		return CHECKTHING_COLLIDE;
	}

	if (thing->flags & MF_PAIN && tmthing->player)
	{  

		if (thing->z > tmthing->z + tmthing->height)
			return CHECKTHING_NOCOLLIDE;  
		if (thing->z + thing->height < tmthing->z)
			return CHECKTHING_NOCOLLIDE;  
		if (tmthing->flags & MF_SHOOTABLE && thing->health > 0)
		{
			UINT32 damagetype = (thing->info->mass & 0xFF);
			if (!damagetype && thing->flags & MF_FIRE)  
				damagetype = DMG_FIRE;
			if (P_DamageMobj(tmthing, thing, thing, 1, damagetype) && (damagetype = (thing->info->mass>>8)))
				S_StartSound(thing, damagetype);
			return CHECKTHING_COLLIDE;
		}
		return CHECKTHING_NOCOLLIDE;
	}
	else if (tmthing->flags & MF_PAIN && thing->player)
	{  

		if (tmthing->z > thing->z + thing->height)
			return CHECKTHING_NOCOLLIDE;  
		if (tmthing->z + tmthing->height < thing->z)
			return CHECKTHING_NOCOLLIDE;  
		if (thing->flags & MF_SHOOTABLE && tmthing->health > 0)
		{
			UINT32 damagetype = (tmthing->info->mass & 0xFF);
			if (!damagetype && tmthing->flags & MF_FIRE)  
				damagetype = DMG_FIRE;
			if (P_DamageMobj(thing, tmthing, tmthing, 1, damagetype) && (damagetype = (tmthing->info->mass>>8)))
				S_StartSound(tmthing, damagetype);
			return CHECKTHING_COLLIDE;
		}
		return CHECKTHING_NOCOLLIDE;
	}

	if (thing->type == MT_HOOPCOLLIDE && thing->flags & MF_SPECIAL && tmthing->player)
	{
		P_TouchSpecialThing(thing, tmthing, true);
		return CHECKTHING_COLLIDE;
	}


	if (tmthing->flags2 & MF2_SKULLFLY)
	{
		if (tmthing->type == MT_EGGMOBILE)  
			return CHECKTHING_COLLIDE;  
		else
		{

			if (tmthing->z > thing->z + thing->height)
				return CHECKTHING_NOCOLLIDE;  
			if (tmthing->z + tmthing->height < thing->z)
				return CHECKTHING_NOCOLLIDE;  

			tmthing->flags2 &= ~MF2_SKULLFLY;
			tmthing->momx = tmthing->momy = tmthing->momz = 0;
			return CHECKTHING_DONE;  
		}
	}

	if ((thing->type == MT_SPRINGSHELL || thing->type == MT_YELLOWSHELL) && thing->health > 0
	 && (tmthing->player || (tmthing->flags & MF_PUSHABLE)) && tmthing->health > 0)
	{

		fixed_t tmz     = ((thing->eflags & MFE_VERTICALFLIP) ? -(tmthing->z + tmthing->height) : tmthing->z);
		fixed_t tmznext = ((thing->eflags & MFE_VERTICALFLIP) ? -tmthing->momz : tmthing->momz) + tmz;
		fixed_t thzh    = ((thing->eflags & MFE_VERTICALFLIP) ? -thing->z : thing->z + thing->height);
		fixed_t sprarea = FixedMul(8*FRACUNIT, thing->scale) * P_MobjFlip(thing);

		if ((tmznext <= thzh && tmz > thzh) || (tmznext > thzh - sprarea && tmznext < thzh))
		{
			P_DoSpring(thing, tmthing);
			return CHECKTHING_COLLIDE;
		}
		else if (tmz > thzh - sprarea && tmz < thzh)  
			return CHECKTHING_NOCOLLIDE;
	}


	if (tmthing->flags & MF_MISSILE || tmthing->type == MT_SHELL)
	{

		if (tmthing->z > thing->z + thing->height)
			return CHECKTHING_NOCOLLIDE;  
		if (tmthing->z + tmthing->height < thing->z)
			return CHECKTHING_NOCOLLIDE;  

		if (tmthing->type != MT_SHELL && tmthing->target && tmthing->target->type == thing->type)
		{

			if (thing == tmthing->target
				|| (thing->player && tmthing->target->player && (thing->player->bot == BOT_2PAI || thing->player->bot == BOT_2PHUMAN)))
				return CHECKTHING_IGNORE;

			if (thing->type != MT_PLAYER)
			{


				return CHECKTHING_DONE;
			}
		}


		if ((tmthing->type == MT_THROWNBOUNCE && tmthing->fuse > 8*TICRATE) && thing->flags & MF_SOLID)
			return CHECKTHING_IGNORE;


		if (thing->type == MT_BLACKEGGMAN_HELPER)
			return CHECKTHING_IGNORE;


		if (tmthing->type == MT_BLACKEGGMAN_MISSILE
		    && thing->type == MT_BLACKEGGMAN && tmthing->target != thing)
		{

			if (!(thing->state >= &states[S_BLACKEGG_PAIN1] && thing->state <= &states[S_BLACKEGG_PAIN35]))
				P_SetMobjState(thing, thing->info->painstate);
			return CHECKTHING_DONE;
		}

		if (!(thing->flags & MF_SHOOTABLE) && !(thing->type == MT_EGGSHIELD))
		{

			if (thing->flags & MF_SOLID)
				return CHECKTHING_COLLIDE;
			else
				return CHECKTHING_NOCOLLIDE;
		}

		if (tmthing->flags & MF_MISSILE && thing->player && tmthing->target && tmthing->target->player
		&& thing->player->ctfteam == tmthing->target->player->ctfteam
		&& thing->player->powers[pw_carry] == CR_PLAYER && thing->tracer == tmthing->target)
			return CHECKTHING_IGNORE;  

		if (thing->type == MT_EGGSHIELD)
		{
			angle_t angle = (R_PointToAngle2(thing->x, thing->y, tmthing->x - tmthing->momx, tmthing->y - tmthing->momy) - thing->angle) - ANGLE_90;

			if (angle < ANGLE_180)  
				P_KillMobj(thing, tmthing, tmthing, 0);
			return CHECKTHING_DONE;
		}


		if (tmthing->flags & MF_ENEMY)  
			P_DamageMobj(thing, tmthing, tmthing, 1, 0);
		else if (tmthing->type == MT_BLACKEGGMAN_MISSILE && thing->player
			&& (thing->player->pflags & PF_JUMPED)
			&& !thing->player->powers[pw_flashing]
			&& !thing->player->powers[pw_ignorelatch]
			&& thing->tracer != tmthing
			&& tmthing->target != thing)
		{

			thing->player->powers[pw_carry] = CR_GENERIC;
			thing->player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE);
			P_SetTarget(&thing->tracer, tmthing);
			P_SetTarget(&tmthing->target, thing);  
			P_SetTarget(&tmthing->tracer, NULL);  
			tmthing->momz = 0;

			thing->angle = tmthing->angle;

			if (!demoplayback || P_ControlStyle(thing->player) == CS_LMAOGALOG)
				P_SetPlayerAngle(thing->player, thing->angle);

			return CHECKTHING_COLLIDE;
		}
		else if (tmthing->type == MT_BLACKEGGMAN_MISSILE && thing->player && ((thing->player->powers[pw_carry] == CR_GENERIC) || (thing->player->pflags & PF_JUMPED)))
		{

		}
		else if (tmthing->type == MT_BLACKEGGMAN_GOOPFIRE)
		{
			P_UnsetThingPosition(tmthing);
			tmthing->x = thing->x;
			tmthing->y = thing->y;
			P_SetThingPosition(tmthing);
		}
		else if (!(tmthing->type == MT_SHELL && thing->player))  
		{
			UINT8 damagetype = tmthing->info->mass;
			if (!damagetype && tmthing->flags & MF_FIRE)  
				damagetype = DMG_FIRE;
			P_DamageMobj(thing, tmthing, tmthing->target, 1, damagetype);
		}



		if ((tmthing->type == MT_FIREBALL) && (thing->flags & MF_ENEMY))
			P_KillMobj(tmthing, NULL, NULL, 0);


		if (tmthing->type == MT_SHELL)
			return CHECKTHING_COLLIDE;
		else
			return CHECKTHING_DONE;
	}

	if (thing->flags & MF_PUSHABLE && (tmthing->player || tmthing->flags & MF_PUSHABLE)
		&& tmthing->z + tmthing->height > thing->z && tmthing->z < thing->z + thing->height
		&& !(netgame && tmthing->player && tmthing->player->spectator))  
	{
		if (thing->flags2 & MF2_SLIDEPUSH)  
		{
			if (tmthing->momy > 0 && tmthing->momy > FixedMul(4*FRACUNIT, thing->scale) && tmthing->momy > thing->momy)
			{
				thing->momy += FixedMul(PUSHACCEL, thing->scale);
				tmthing->momy -= FixedMul(PUSHACCEL, thing->scale);
			}
			else if (tmthing->momy < 0 && tmthing->momy < FixedMul(-4*FRACUNIT, thing->scale)
				&& tmthing->momy < thing->momy)
			{
				thing->momy -= FixedMul(PUSHACCEL, thing->scale);
				tmthing->momy += FixedMul(PUSHACCEL, thing->scale);
			}
			if (tmthing->momx > 0 && tmthing->momx > FixedMul(4*FRACUNIT, thing->scale)
				&& tmthing->momx > thing->momx)
			{
				thing->momx += FixedMul(PUSHACCEL, thing->scale);
				tmthing->momx -= FixedMul(PUSHACCEL, thing->scale);
			}
			else if (tmthing->momx < 0 && tmthing->momx < FixedMul(-4*FRACUNIT, thing->scale)
				&& tmthing->momx < thing->momx)
			{
				thing->momx -= FixedMul(PUSHACCEL, thing->scale);
				tmthing->momx += FixedMul(PUSHACCEL, thing->scale);
			}

			if (thing->momx > FixedMul(thing->info->speed, thing->scale))
				thing->momx = FixedMul(thing->info->speed, thing->scale);
			else if (thing->momx < -FixedMul(thing->info->speed, thing->scale))
				thing->momx = -FixedMul(thing->info->speed, thing->scale);
			if (thing->momy > FixedMul(thing->info->speed, thing->scale))
				thing->momy = FixedMul(thing->info->speed, thing->scale);
			else if (thing->momy < -FixedMul(thing->info->speed, thing->scale))
				thing->momy = -FixedMul(thing->info->speed, thing->scale);
		}
		else
		{
			if (tmthing->momx > FixedMul(4*FRACUNIT, thing->scale))
				tmthing->momx = FixedMul(4*FRACUNIT, thing->scale);
			else if (tmthing->momx < FixedMul(-4*FRACUNIT, thing->scale))
				tmthing->momx = FixedMul(-4*FRACUNIT, thing->scale);
			if (tmthing->momy > FixedMul(4*FRACUNIT, thing->scale))
				tmthing->momy = FixedMul(4*FRACUNIT, thing->scale);
			else if (tmthing->momy < FixedMul(-4*FRACUNIT, thing->scale))
				tmthing->momy = FixedMul(-4*FRACUNIT, thing->scale);

			thing->momx = tmthing->momx;
			thing->momy = tmthing->momy;
		}

		if (thing->type != MT_GARGOYLE || P_IsObjectOnGround(thing))
			S_StartSound(thing, thing->info->activesound);

		P_SetTarget(&thing->target, tmthing);
	}


	if ((tmthing->type == MT_NIGHTSDRONE || thing->type == MT_NIGHTSDRONE)
	 && (tmthing->player || thing->player))
	{
		mobj_t *droneobj = (tmthing->type == MT_NIGHTSDRONE) ? tmthing : thing;
		player_t *pl = (droneobj == thing) ? tmthing->player : thing->player;





		if ((pl->powers[pw_carry] == CR_NIGHTSMODE) && (INT32)leveltime > droneobj->extravalue2 && (
		   !(pl->flyangle > 90 &&   pl->flyangle < 270)
		^ (droneobj->extravalue1 > 90 && droneobj->extravalue1 < 270)
		))
		{
			pl->marelap++;
			pl->totalmarelap++;
			pl->lapbegunat = leveltime;
			pl->lapstartedtime = pl->nightstime;

			if (pl->bonustime)
			{
				pl->marebonuslap++;
				pl->totalmarebonuslap++;


				P_ReloadRings();
			}

			P_RunNightsLapExecutors(pl->mo);
		}
		droneobj->extravalue1 = pl->flyangle;
		droneobj->extravalue2 = (INT32)leveltime + TICRATE;
	}


	if (thing->flags & MF_SPECIAL && (tmthing->player || (tmthing->flags & MF_PUSHABLE)))  
	{
		P_TouchSpecialThing(thing, tmthing, true);  
		return CHECKTHING_COLLIDE;
	}

	if (tmthing->flags & MF_SPECIAL && (thing->player || (thing->flags & MF_PUSHABLE)))  
	{
		P_TouchSpecialThing(tmthing, thing, true);  
		return CHECKTHING_COLLIDE;
	}



	if (tmthing->type == MT_SPIKE && tmthing->flags & MF_SOLID && thing->player)  
	{
		if (tmthing->eflags & MFE_VERTICALFLIP)
		{
			if (thing->z + thing->height <= tmthing->z + FixedMul(FRACUNIT, tmthing->scale)
			&& thing->z + thing->height + thing->momz  >= tmthing->z + FixedMul(FRACUNIT, tmthing->scale) + tmthing->momz
			&& !(thing->player->charability == CA_BOUNCE && thing->player->panim == PA_ABILITY && thing->eflags & MFE_VERTICALFLIP))
				P_DamageMobj(thing, tmthing, tmthing, 1, DMG_SPIKE);
		}
		else if (thing->z >= tmthing->z + tmthing->height - FixedMul(FRACUNIT, tmthing->scale)
		&& thing->z + thing->momz <= tmthing->z + tmthing->height - FixedMul(FRACUNIT, tmthing->scale) + tmthing->momz
		&& !(thing->player->charability == CA_BOUNCE && thing->player->panim == PA_ABILITY && !(thing->eflags & MFE_VERTICALFLIP)))
			P_DamageMobj(thing, tmthing, tmthing, 1, DMG_SPIKE);
	}
	else if (thing->type == MT_SPIKE && thing->flags & MF_SOLID && tmthing->player)  
	{
		if (thing->eflags & MFE_VERTICALFLIP)
		{
			if (tmthing->z + tmthing->height <= thing->z - FixedMul(FRACUNIT, thing->scale)
			&& tmthing->z + tmthing->height + tmthing->momz >= thing->z - FixedMul(FRACUNIT, thing->scale)
			&& !(tmthing->player->charability == CA_BOUNCE && tmthing->player->panim == PA_ABILITY && tmthing->eflags & MFE_VERTICALFLIP))
				P_DamageMobj(tmthing, thing, thing, 1, DMG_SPIKE);
		}
		else if (tmthing->z >= thing->z + thing->height + FixedMul(FRACUNIT, thing->scale)
		&& tmthing->z + tmthing->momz <= thing->z + thing->height + FixedMul(FRACUNIT, thing->scale)
		&& !(tmthing->player->charability == CA_BOUNCE && tmthing->player->panim == PA_ABILITY && !(tmthing->eflags & MFE_VERTICALFLIP)))
			P_DamageMobj(tmthing, thing, thing, 1, DMG_SPIKE);
	}

	if (tmthing->type == MT_WALLSPIKE && tmthing->flags & MF_SOLID && thing->player)  
	{
		fixed_t bottomz, topz;
		bottomz = tmthing->z;
		topz = tmthing->z + tmthing->height;
		if (tmthing->eflags & MFE_VERTICALFLIP)
			bottomz -= FixedMul(FRACUNIT, tmthing->scale);
		else
			topz += FixedMul(FRACUNIT, tmthing->scale);

		if (thing->z + thing->height > bottomz  
		&&  thing->z < topz)  

			P_DamageMobj(thing, tmthing, tmthing, 1, DMG_SPIKE);
	}
	else if (thing->type == MT_WALLSPIKE && thing->flags & MF_SOLID && tmthing->player)
	{
		fixed_t bottomz, topz;
		angle_t touchangle = R_PointToAngle2(thing->tracer->x, thing->tracer->y, tmthing->x, tmthing->y);

		if (P_PlayerInPain(tmthing->player) && (tmthing->momx || tmthing->momy))
		{
			angle_t playerangle = R_PointToAngle2(0, 0, tmthing->momx, tmthing->momy) - touchangle;
			if (playerangle > ANGLE_180)
				playerangle = InvAngle(playerangle);
			if (playerangle < ANGLE_90)
				return CHECKTHING_IGNORE;  
		}

		bottomz = thing->z;
		topz = thing->z + thing->height;

		if (thing->eflags & MFE_VERTICALFLIP)
			bottomz -= FixedMul(FRACUNIT, thing->scale);
		else
			topz += FixedMul(FRACUNIT, thing->scale);

		if (tmthing->z + tmthing->height > bottomz  
		&&  tmthing->z < topz  
		&& !P_MobjWasRemoved(thing->tracer))  
		{  
			touchangle = thing->angle - touchangle;
			if (touchangle > ANGLE_180)
				touchangle = InvAngle(touchangle);
			if (touchangle <= ANGLE_22h)  
				P_DamageMobj(tmthing, thing, thing, 1, DMG_SPIKE);
		}
	}

	if (thing->flags & MF_PUSHABLE)
	{
		if (tmthing->type == MT_FAN)
			P_DoFan(tmthing, thing);
	}

	if (tmthing->flags & MF_PUSHABLE)
	{
		if (thing->type == MT_FAN)
		{
			P_DoFan(thing, tmthing);
			return CHECKTHING_COLLIDE;
		}
		else if (thing->flags & MF_SPRING)
		{
			if ( thing->z <= tmthing->z + tmthing->height
			&& tmthing->z <= thing->z + thing->height)
				if (P_DoSpring(thing, tmthing))
					return CHECKTHING_DONE;
			return CHECKTHING_COLLIDE;
		}
	}


	if (thing->flags & (MF_ENEMY|MF_BOSS) && tmthing->flags & (MF_ENEMY|MF_BOSS))
	{
		if ((thing->z + thing->height >= tmthing->z)
		&& (tmthing->z + tmthing->height >= thing->z))
			return CHECKTHING_DONE;
	}


	if (tmthing->player && thing->player

		&& !(thing->z + thing->height < tmthing->z || thing->z > tmthing->z + tmthing->height))
	{
		if (G_RingSlingerGametype() && (!G_GametypeHasTeams() || tmthing->player->ctfteam != thing->player->ctfteam))
		{
			if ((tmthing->player->powers[pw_invulnerability] || tmthing->player->powers[pw_super] || (((tmthing->player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL) && (tmthing->player->pflags & PF_SHIELDABILITY)))
				&& !thing->player->powers[pw_super])
				P_DamageMobj(thing, tmthing, tmthing, 1, 0);
			else if ((thing->player->powers[pw_invulnerability] || thing->player->powers[pw_super] || (((thing->player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL) && (thing->player->pflags & PF_SHIELDABILITY)))
				&& !tmthing->player->powers[pw_super])
				P_DamageMobj(tmthing, thing, thing, 1, 0);
		}


		if (G_TagGametype() && cv_touchtag.value &&
			((thing->player->pflags & PF_TAGIT) != (tmthing->player->pflags & PF_TAGIT)))
		{
			if ((tmthing->player->pflags & PF_TAGIT) && !(thing->player->pflags & PF_TAGIT))
				P_DamageMobj(thing, tmthing, tmthing, 1, 0);
			else if ((thing->player->pflags & PF_TAGIT) && !(tmthing->player->pflags & PF_TAGIT))
				P_DamageMobj(tmthing, thing, tmthing, 1, 0);
		}
	}


	if (cv_tailspickup.value && !(gametyperules & GTR_HIDEFROZEN))
	{
		if (tmthing->player && thing->player)
		{
			P_DoTailsCarry(thing->player, tmthing->player);
			return CHECKTHING_COLLIDE;
		}
	}
	else if (thing->player) {
		if (thing->player-players == consoleplayer && botingame)
			CV_SetValue(&cv_analog[1], true);
		if (thing->player->powers[pw_carry] == CR_PLAYER)
		{
			P_SetTarget(&thing->tracer, NULL);
			thing->player->powers[pw_carry] = CR_NONE;
		}
	}

	if (thing->player)
	{

		if (tmthing->eflags & MFE_VERTICALFLIP
		&& (tmthing->z + tmthing->height + tmthing->momz < thing->z
		 || tmthing->z + tmthing->height + tmthing->momz >= thing->z + thing->height))
			;
		else if (!(tmthing->eflags & MFE_VERTICALFLIP)
		&& (tmthing->z + tmthing->momz > thing->z + thing->height
		 || tmthing->z + tmthing->momz <= thing->z))
			;
		else if  (P_IsObjectOnGround(thing)
			&& !P_IsObjectOnGround(tmthing)  
			&& (tmthing->flags & MF_SOLID))
		{
			if (tmthing->flags & (MF_MONITOR|MF_PUSHABLE))
			{

				if (thing != tmthing->target)
					P_DamageMobj(thing, tmthing, tmthing->target, 1, DMG_CRUSHED);

				tmthing->momz = -tmthing->momz/2;  


			}
		}

		if (tmthing->type == MT_FAN)
			P_DoFan(tmthing, thing);
	}

	if (tmthing->player)  
	{
		if (!tmthing->health)
			return CHECKTHING_IGNORE;

		if (thing->type == MT_FAN)
			P_DoFan(thing, tmthing);
		else if (thing->flags & MF_SPRING && tmthing->player->powers[pw_carry] != CR_MINECART)
		{
			if ( thing->z <= tmthing->z + tmthing->height
			&& tmthing->z <= thing->z + thing->height)
				if (P_DoSpring(thing, tmthing))
					return CHECKTHING_DONE;
			return CHECKTHING_COLLIDE;
		}

		else if (thing->flags & MF_MONITOR
		&& !((thing->type == MT_RING_REDBOX && tmthing->player->ctfteam != 1) || (thing->type == MT_RING_BLUEBOX && tmthing->player->ctfteam != 2))
		&& (!(thing->flags & MF_SOLID) || P_PlayerCanDamage(tmthing->player, thing)))
		{
			unsigned collide = CHECKTHING_NOCOLLIDE;

			if (thing->z - thing->scale <= tmthing->z + tmthing->height
			&& thing->z + thing->height + thing->scale >= tmthing->z)
			{
				player_t *player = tmthing->player;

				UINT8 elementalpierce = (((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL || (player->powers[pw_shield] & SH_NOSTACK) == SH_BUBBLEWRAP) && (player->pflags & PF_SHIELDABILITY)
				? (((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL) ? 1 : 2)
				: 0);
				SINT8 flipval = P_MobjFlip(thing);  
				fixed_t *momz = &tmthing->momz;  
				fixed_t *z = &tmthing->z;  

				if (P_DamageMobj(thing, tmthing, tmthing, 1, 0)  
				&& (flipval*(*momz) < 0)  
				&& (elementalpierce != 1) && (!(player->powers[pw_strong] & STR_HEAVY)))  
				{
					fixed_t setmomz = -*momz;  

					if (elementalpierce == 2)  
						P_DoBubbleBounce(player);
					*momz = setmomz;  
					if (player->charability == CA_TWINSPIN && player->panim == PA_ABILITY)
						P_TwinSpinRejuvenate(player, player->thokitem);
					if (elementalpierce == 2)  
					{
						boolean underwater = tmthing->eflags & MFE_UNDERWATER;

						if (underwater)
							*momz /= 2;
						*momz -= (*momz/(underwater ? 8 : 4));  
					}
					collide = CHECKTHING_COLLIDE;
				}
				if (!(elementalpierce == 1 && thing->flags & MF_GRENADEBOUNCE))  
				{
					if (player->pflags & PF_BOUNCING)
						P_DoAbilityBounce(player, false);
					collide = CHECKTHING_DONE;
				}
				else
					*z -= *momz;  
			}

			return collide;
		}
	}


	unsigned collide = CHECKTHING_NOCOLLIDE;

	if ((tmthing->flags & MF_SPRING || tmthing->type == MT_SPIKE || tmthing->type == MT_WALLSPIKE) && (thing->player))
		;  


	else if ((thing->flags & (MF_SOLID|MF_NOCLIP)) == MF_SOLID
		&& (tmthing->flags & (MF_SOLID|MF_NOCLIP)) == MF_SOLID)
	{
		fixed_t topz, tmtopz;

		if (tmthing->eflags & MFE_VERTICALFLIP)
		{

			tmtopz = tmthing->z;

			if (tmtopz > thing->z + thing->height)
			{
				if (thing->z + thing->height > tmfloorz)
				{
					tmfloorz = thing->z + thing->height;
					tmfloorrover = NULL;
					tmfloorslope = NULL;
				}
				return CHECKTHING_COLLIDE;
			}

			topz = thing->z - thing->scale;  





			if (tmthing->player && tmthing->z + tmthing->height > topz
				&& tmthing->z + tmthing->height < tmthing->ceilingz)
			{
				if (thing->flags & MF_GRENADEBOUNCE && (thing->flags & MF_MONITOR || thing->info->flags & MF_MONITOR))  
					return CHECKTHING_DONE;

				tmfloorz = tmceilingz = topz;  
				tmceilingrover = NULL;
				tmceilingslope = NULL;
				tmfloorthing = thing;  

				collide = CHECKTHING_COLLIDE;
			}
			else if (topz < tmceilingz && tmthing->z <= thing->z+thing->height)
			{
				tmceilingz = topz;
				tmceilingrover = NULL;
				tmceilingslope = NULL;
				tmfloorthing = thing;  

				collide = CHECKTHING_COLLIDE;
			}
		}
		else
		{

			tmtopz = tmthing->z + tmthing->height;

			if (tmtopz < thing->z)
			{
				if (thing->z < tmceilingz)
				{
					tmceilingz = thing->z;
					tmceilingrover = NULL;
					tmceilingslope = NULL;
				}
				return CHECKTHING_COLLIDE;
			}

			topz = thing->z + thing->height + thing->scale;  





			if (tmthing->player && tmthing->z < topz
				&& tmthing->z > tmthing->floorz)
			{
				if (thing->flags & MF_GRENADEBOUNCE && (thing->flags & MF_MONITOR || thing->info->flags & MF_MONITOR))  
					return CHECKTHING_DONE;

				tmfloorz = tmceilingz = topz;  
				tmfloorrover = NULL;
				tmfloorslope = NULL;
				tmfloorthing = thing;  

				collide = CHECKTHING_COLLIDE;
			}
			else if (topz > tmfloorz && tmthing->z+tmthing->height >= thing->z)
			{
				tmfloorz = topz;
				tmfloorrover = NULL;
				tmfloorslope = NULL;
				tmfloorthing = thing;  

				collide = CHECKTHING_COLLIDE;
			}
		}
	}

	return collide;
}

static boolean PIT_CheckThing(mobj_t *thing)
{
	return PIT_DoCheckThing(thing) != CHECKTHING_DONE;
}



static boolean PIT_CheckCameraLine(line_t *ld)
{
	if (ld->polyobj && !(ld->polyobj->flags & POF_SOLID))
		return true;

	if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT] || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
		|| tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM] || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
	{
		return true;
	}

	if (P_BoxOnLineSide(tmbbox, ld) != -1)
		return true;













	blockingline = ld;
	if (!ld->backsector)  
	{
		if (P_PointOnLineSide(mapcampointer->x, mapcampointer->y, ld))
			return true;  
		return false;
	}


	P_CameraLineOpening(ld);


	if (opentop < tmceilingz)
	{
		tmceilingz = opentop;
		ceilingline = ld;
	}

	if (openbottom > tmfloorz)
	{
		tmfloorz = openbottom;
	}

	if (highceiling > tmdrpoffceilz)
		tmdrpoffceilz = highceiling;

	if (lowfloor < tmdropoffz)
		tmdropoffz = lowfloor;

	return true;
}





static boolean PIT_CheckLine(line_t *ld)
{
	if (ld->polyobj && !(ld->polyobj->flags & POF_SOLID))
		return true;

	if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT] || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
	|| tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM] || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
		return true;

	if (P_BoxOnLineSide(tmbbox, ld) != -1)
		return true;

	if (tmthing->flags & MF_PAPERCOLLISION)  
	{
		fixed_t cosradius, sinradius;
		cosradius = FixedMul(tmthing->radius, FINECOSINE(tmthing->angle>>ANGLETOFINESHIFT));
		sinradius = FixedMul(tmthing->radius, FINESINE(tmthing->angle>>ANGLETOFINESHIFT));
		if (P_PointOnLineSide(tmx - cosradius, tmy - sinradius, ld)
		== P_PointOnLineSide(tmx + cosradius, tmy + sinradius, ld))
			return true;  
#ifdef PAPER_COLLISIONCORRECTION
		{
			fixed_t dist;
			vertex_t result;
			angle_t langle;
			P_ClosestPointOnLine(tmx, tmy, ld, &result);
			langle = R_PointToAngle2(ld->v1->x, ld->v1->y, ld->v2->x, ld->v2->y);
			langle += ANGLE_90*(P_PointOnLineSide(tmx, tmy, ld) ? -1 : 1);
			dist = abs(FixedMul(tmthing->radius, FINECOSINE((tmthing->angle - langle)>>ANGLETOFINESHIFT)));
			cosradius = FixedMul(dist, FINECOSINE(langle>>ANGLETOFINESHIFT));
			sinradius = FixedMul(dist, FINESINE(langle>>ANGLETOFINESHIFT));
			tmthing->flags |= MF_NOCLIP;
			P_MoveOrigin(tmthing, result.x + cosradius - tmthing->momx, result.y + sinradius - tmthing->momy, tmthing->z);
			tmthing->flags &= ~MF_NOCLIP;
		}
#endif
	}













	blockingline = ld;

	{
		UINT8 shouldCollide = LUA_HookMobjLineCollide(tmthing, blockingline);  
		if (P_MobjWasRemoved(tmthing))
			return true;  
		if (shouldCollide == 1)
			return false;  
		else if (shouldCollide == 2)
			return true;  
	}

	if (!ld->backsector)  
	{
		if (P_PointOnLineSide(tmthing->x, tmthing->y, ld))
			return true;  
		return false;
	}


	if (!(tmthing->flags & MF_MISSILE))
	{
		if (ld->flags & ML_IMPASSIBLE)  
			return false;
		if ((tmthing->flags & (MF_ENEMY|MF_BOSS)) && ld->flags & ML_BLOCKMONSTERS)
			return false;  
	}


	P_LineOpening(ld, tmthing);


	if (opentop < tmceilingz)
	{
		tmceilingz = opentop;
		ceilingline = ld;
		tmceilingrover = openceilingrover;
		tmceilingslope = opentopslope;
	}

	if (openbottom > tmfloorz)
	{
		tmfloorz = openbottom;
		tmfloorrover = openfloorrover;
		tmfloorslope = openbottomslope;
	}

	if (highceiling > tmdrpoffceilz)
		tmdrpoffceilz = highceiling;

	if (lowfloor < tmdropoffz)
		tmdropoffz = lowfloor;

	return true;
}




boolean P_CheckPosition(mobj_t *thing, fixed_t x, fixed_t y)
{
	INT32 xl, xh, yl, yh, bx, by;
	subsector_t *newsubsec;
	boolean blockval = true;

	ps_checkposition_calls.value.i++;

	I_Assert(thing != NULL);
#ifdef PARANOIA
	if (P_MobjWasRemoved(thing))
		I_Error("Previously-removed Thing of type %u crashes P_CheckPosition!", thing->type);
#endif

	P_SetTarget(&tmthing, thing);
	tmflags = thing->flags;

	tmx = x;
	tmy = y;

	tmbbox[BOXTOP] = y + tmthing->radius;
	tmbbox[BOXBOTTOM] = y - tmthing->radius;
	tmbbox[BOXRIGHT] = x + tmthing->radius;
	tmbbox[BOXLEFT] = x - tmthing->radius;

	newsubsec = R_PointInSubsector(x, y);
	ceilingline = blockingline = NULL;





	tmfloorz = tmdropoffz = P_GetFloorZ(thing, newsubsec->sector, x, y, NULL);  
	tmceilingz = P_GetCeilingZ(thing, newsubsec->sector, x, y, NULL);  
	tmfloorrover = NULL;
	tmceilingrover = NULL;
	tmfloorslope = newsubsec->sector->f_slope;
	tmceilingslope = newsubsec->sector->c_slope;


	if (newsubsec->sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t delta1, delta2;
		INT32 thingtop = thing->z + thing->height;

		for (rover = newsubsec->sector->ffloors; rover; rover = rover->next)
		{
			fixed_t topheight, bottomheight;

			if (!(rover->fofflags & FOF_EXISTS))
				continue;

			topheight = P_GetFOFTopZ(thing, newsubsec->sector, rover, x, y, NULL);
			bottomheight = P_GetFOFBottomZ(thing, newsubsec->sector, rover, x, y, NULL);

			if ((rover->fofflags & (FOF_SWIMMABLE|FOF_GOOWATER)) == (FOF_SWIMMABLE|FOF_GOOWATER) && !(thing->flags & MF_NOGRAVITY))
			{

				fixed_t sinklevel = FixedMul(thing->info->height/6, thing->scale);
				fixed_t minspeed = FixedMul(thing->info->height/9, thing->scale);
				if (thing->z < topheight && bottomheight < thingtop
				&& abs(thing->momz) < minspeed)
				{

					if (!(thing->eflags & MFE_VERTICALFLIP) && thing->z > topheight - sinklevel
					&& thing->momz >= 0 && thing->momz < (minspeed>>2))
						thing->momz += minspeed>>2;
					else if (thing->eflags & MFE_VERTICALFLIP && thingtop < bottomheight + sinklevel
					&& thing->momz <= 0 && thing->momz > -(minspeed>>2))
						thing->momz -= minspeed>>2;


					if (!(thing->eflags & MFE_VERTICALFLIP) && thing->z >= topheight - sinklevel && thing->momz <= 0)
					{
						if (tmfloorz < topheight - sinklevel) {
							tmfloorz = topheight - sinklevel;
							tmfloorrover = rover;
							tmfloorslope = *rover->t_slope;
						}
					}
					else if (thing->eflags & MFE_VERTICALFLIP && thingtop <= bottomheight + sinklevel && thing->momz >= 0)
					{
						if (tmceilingz > bottomheight + sinklevel) {
							tmceilingz = bottomheight + sinklevel;
							tmceilingrover = rover;
							tmceilingslope = *rover->b_slope;
						}
					}
				}
				continue;
			}

			if (thing->player && (P_CheckSolidLava(rover) || P_CanRunOnWater(thing->player, rover)))
				;
			else if (thing->type == MT_SKIM && (rover->fofflags & FOF_SWIMMABLE))
				;
			else if (!((rover->fofflags & FOF_BLOCKPLAYER && thing->player)
			    || (rover->fofflags & FOF_BLOCKOTHERS && !thing->player)
				|| rover->fofflags & FOF_QUICKSAND))
				continue;

			if (rover->fofflags & FOF_QUICKSAND)
			{
				if (!(thing->eflags & MFE_VERTICALFLIP) && thing->z < topheight && bottomheight < thingtop)
				{
					if (tmfloorz < thing->z) {
						tmfloorz = thing->z;
						tmfloorrover = rover;
						tmfloorslope = NULL;
					}
				}
				else if (thing->eflags & MFE_VERTICALFLIP && thing->z < topheight && bottomheight < thingtop)
				{
					if (tmceilingz > thingtop) {
						tmceilingz = thingtop;
						tmceilingrover = rover;
						tmceilingslope = NULL;
					}
				}

				continue;
			}

			delta1 = thing->z - (bottomheight
				+ ((topheight - bottomheight)/2));
			delta2 = thingtop - (bottomheight
				+ ((topheight - bottomheight)/2));

			if (topheight > tmfloorz && abs(delta1) < abs(delta2)
				&& !(rover->fofflags & FOF_REVERSEPLATFORM))
			{
				tmfloorz = tmdropoffz = topheight;
				tmfloorrover = rover;
				tmfloorslope = *rover->t_slope;
			}
			if (bottomheight < tmceilingz && abs(delta1) >= abs(delta2)
				&& !(rover->fofflags & FOF_PLATFORM)
				&& !(thing->type == MT_SKIM && (rover->fofflags & FOF_SWIMMABLE)))
			{
				tmceilingz = tmdrpoffceilz = bottomheight;
				tmceilingrover = rover;
				tmceilingslope = *rover->b_slope;
			}
		}
	}

	xl = (unsigned)(tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);


	{
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
						fixed_t delta1, delta2, thingtop;
						fixed_t polytop, polybottom;

						po->validcount = validcount;

						if (!P_BBoxInsidePolyobj(po, tmbbox)
							|| !(po->flags & POF_SOLID))
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

						thingtop = thing->z + thing->height;
						delta1 = thing->z - (polybottom + ((polytop - polybottom)/2));
						delta2 = thingtop - (polybottom + ((polytop - polybottom)/2));

						if (polytop > tmfloorz && abs(delta1) < abs(delta2)) {
							tmfloorz = tmdropoffz = polytop;
							tmfloorslope = NULL;
							tmfloorrover = NULL;
						}

						if (polybottom < tmceilingz && abs(delta1) >= abs(delta2)) {
							tmceilingz = tmdrpoffceilz = polybottom;
							tmceilingslope = NULL;
							tmceilingrover = NULL;
						}
					}
					plink = (polymaplink_t *)(plink->link.next);
				}
			}
	}


	tmfloorthing = NULL;
	tmhitthing = NULL;

	validcount++;

	if (tmflags & MF_NOCLIP)
		return true;


	if (!(thing->flags & MF_NOCLIPTHING))
	{
		for (bx = xl; bx <= xh; bx++)
			for (by = yl; by <= yh; by++)
			{
				if (!P_BlockThingsIterator(bx, by, PIT_CheckThing))
					blockval = false;
				else
					tmhitthing = tmfloorthing;
				if (P_MobjWasRemoved(tmthing))
					return false;
			}
	}

	validcount++;


	for (bx = xl; bx <= xh; bx++)
		for (by = yl; by <= yh; by++)
			if (!P_BlockLinesIterator(bx, by, PIT_CheckLine))
				blockval = false;

	return blockval;
}

static const fixed_t hoopblockdist = 16*FRACUNIT + 8*FRACUNIT;
static const fixed_t hoophalfheight = (56*FRACUNIT)/2;


void P_CheckHoopPosition(mobj_t *hoopthing, fixed_t x, fixed_t y, fixed_t z, fixed_t radius)
{
	INT32 i;

	(void)radius;  
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || !players[i].mo || players[i].spectator)
			continue;

		if (abs(players[i].mo->x - x) >= hoopblockdist ||
			abs(players[i].mo->y - y) >= hoopblockdist ||
			abs((players[i].mo->z+hoophalfheight) - z) >= hoopblockdist)
			continue;  


		P_TouchSpecialThing(hoopthing, players[i].mo, false);
		break;
	}

	return;
}




boolean P_CheckCameraPosition(fixed_t x, fixed_t y, camera_t *thiscam)
{
	INT32 xl, xh, yl, yh, bx, by;
	subsector_t *newsubsec;

	tmx = x;
	tmy = y;

	tmbbox[BOXTOP] = y + thiscam->radius;
	tmbbox[BOXBOTTOM] = y - thiscam->radius;
	tmbbox[BOXRIGHT] = x + thiscam->radius;
	tmbbox[BOXLEFT] = x - thiscam->radius;

	newsubsec = R_PointInSubsector(x, y);
	ceilingline = blockingline = NULL;

	mapcampointer = thiscam;

	if (newsubsec->sector->flags & MSF_NOCLIPCAMERA)
	{  
		tmfloorz = tmdropoffz = thiscam->z;
		tmceilingz = tmdrpoffceilz = thiscam->z + thiscam->height;
		return true;
	}





	tmfloorz = tmdropoffz = P_CameraGetFloorZ(thiscam, newsubsec->sector, x, y, NULL);

	tmceilingz = P_CameraGetCeilingZ(thiscam, newsubsec->sector, x, y, NULL);



	if (newsubsec->sector->heightsec >= 0)
	{
		tmfloorz = tmdropoffz = sectors[newsubsec->sector->heightsec].floorheight;
		tmceilingz = tmdrpoffceilz = sectors[newsubsec->sector->heightsec].ceilingheight;
	}


	if (newsubsec->sector->camsec >= 0)
	{
		tmfloorz = tmdropoffz = sectors[newsubsec->sector->camsec].floorheight;
		tmceilingz = tmdrpoffceilz = sectors[newsubsec->sector->camsec].ceilingheight;
	}


	if (newsubsec->sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t delta1, delta2;
		INT32 thingtop = thiscam->z + thiscam->height;

		for (rover = newsubsec->sector->ffloors; rover; rover = rover->next)
		{
			fixed_t topheight, bottomheight;
			if (!(rover->fofflags & FOF_BLOCKOTHERS) || !(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_RENDERALL) || (rover->master->frontsector->flags & MSF_NOCLIPCAMERA))
				continue;

			topheight = P_CameraGetFOFTopZ(thiscam, newsubsec->sector, rover, x, y, NULL);
			bottomheight = P_CameraGetFOFBottomZ(thiscam, newsubsec->sector, rover, x, y, NULL);

			delta1 = thiscam->z - (bottomheight
				+ ((topheight - bottomheight)/2));
			delta2 = thingtop - (bottomheight
				+ ((topheight - bottomheight)/2));
			if (topheight > tmfloorz && abs(delta1) < abs(delta2))
			{
				tmfloorz = tmdropoffz = topheight;
			}
			if (bottomheight < tmceilingz && abs(delta1) >= abs(delta2))
			{
				tmceilingz = tmdrpoffceilz = bottomheight;
			}
		}
	}

	xl = (unsigned)(tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);


	{
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

						thingtop = thiscam->z + thiscam->height;
						delta1 = thiscam->z - (polybottom + ((polytop - polybottom)/2));
						delta2 = thingtop - (polybottom + ((polytop - polybottom)/2));

						if (polytop > tmfloorz && abs(delta1) < abs(delta2))
							tmfloorz = tmdropoffz = polytop;

						if (polybottom < tmceilingz && abs(delta1) >= abs(delta2))
							tmceilingz = tmdrpoffceilz = polybottom;
					}
					plink = (polymaplink_t *)(plink->link.next);
				}
			}
	}


	for (bx = xl; bx <= xh; bx++)
		for (by = yl; by <= yh; by++)
			if (!P_BlockLinesIterator(bx, by, PIT_CheckCameraLine))
				return false;

	return true;
}


#define MAXCAMERASTEPMOVE MAXSTEPMOVE








boolean P_TryCameraMove(fixed_t x, fixed_t y, camera_t *thiscam)
{
	subsector_t *s = R_PointInSubsector(x, y);
	boolean itsatwodlevel = false;

	floatok = false;

	if (dedicated)  
		return false;

	if (twodlevel
		|| (thiscam == &camera && players[displayplayer].mo && (players[displayplayer].mo->flags2 & MF2_TWOD))
		|| (thiscam == &camera2 && players[secondarydisplayplayer].mo && (players[secondarydisplayplayer].mo->flags2 & MF2_TWOD)))
		itsatwodlevel = true;

	if (!itsatwodlevel && players[displayplayer].mo)
	{
		fixed_t tryx = thiscam->x;
		fixed_t tryy = thiscam->y;

		if ((thiscam == &camera && (players[displayplayer].pflags & PF_NOCLIP || players[displayplayer].powers[pw_carry] == CR_NIGHTSMODE))
		|| (thiscam == &camera2 && (players[secondarydisplayplayer].pflags & PF_NOCLIP || players[secondarydisplayplayer].powers[pw_carry] == CR_NIGHTSMODE)))
		{  
			floatok = true;
			thiscam->floorz = thiscam->z;
			thiscam->ceilingz = thiscam->z + thiscam->height;
			thiscam->x = x;
			thiscam->y = y;
			thiscam->subsector = s;
			return true;
		}

		do {
			if (x-tryx > MAXTRYMOVE)
				tryx += MAXTRYMOVE;
			else if (x-tryx < -MAXTRYMOVE)
				tryx -= MAXTRYMOVE;
			else
				tryx = x;
			if (y-tryy > MAXTRYMOVE)
				tryy += MAXTRYMOVE;
			else if (y-tryy < -MAXTRYMOVE)
				tryy -= MAXTRYMOVE;
			else
				tryy = y;

			if (!P_CheckCameraPosition(tryx, tryy, thiscam))
				return false;  

			if (tmceilingz - tmfloorz < thiscam->height)
				return false;  

			floatok = true;

			if (tmceilingz - thiscam->z < thiscam->height)
			{
				if (s == thiscam->subsector && tmceilingz >= thiscam->z)
				{
					floatok = true;
					thiscam->floorz = tmfloorz;
					thiscam->ceilingz = tmfloorz + thiscam->height;
					thiscam->x = x;
					thiscam->y = y;
					thiscam->subsector = s;
					return true;
				}
				else
					return false;  
			}

			if ((tmfloorz - thiscam->z > MAXCAMERASTEPMOVE))
				return false;  
		} while(tryx != x || tryy != y);
	}
	else
	{
		tmfloorz = P_CameraGetFloorZ(thiscam, thiscam->subsector->sector, x, y, NULL);
		tmceilingz = P_CameraGetCeilingZ(thiscam, thiscam->subsector->sector, x, y, NULL);
	}




	thiscam->floorz = tmfloorz;
	thiscam->ceilingz = tmceilingz;
	thiscam->x = x;
	thiscam->y = y;
	thiscam->subsector = s;

	return true;
}







static mobj_t *stand;
static fixed_t standx, standy;

boolean PIT_PushableMoved(mobj_t *thing)
{
	fixed_t blockdist;

	if (!(thing->flags & MF_SOLID)
		|| (thing->flags & MF_NOGRAVITY))
		return true;  


	if (!(thing->flags & MF_PUSHABLE || thing->player))
		return true;

	if (thing == stand)
		return true;

	blockdist = stand->radius + thing->radius;

	if (abs(thing->x - stand->x) >= blockdist || abs(thing->y - stand->y) >= blockdist)
		return true;  

	if ((!(stand->eflags & MFE_VERTICALFLIP) && thing->z != stand->z + stand->height + FixedMul(FRACUNIT, stand->scale))
	|| ((stand->eflags & MFE_VERTICALFLIP) && thing->z + thing->height != stand->z - FixedMul(FRACUNIT, stand->scale)))
		return true;  

	if (!stand->momx && !stand->momy)
		return true;


	if (thing->player)
	{





		boolean oldfltok = floatok;
		fixed_t oldflrz = tmfloorz;
		fixed_t oldceilz = tmceilingz;
		mobj_t *oldflrthing = tmfloorthing;
		mobj_t *oldthing = tmthing;
		line_t *oldceilline = ceilingline;
		line_t *oldblockline = blockingline;
		ffloor_t *oldflrrover = tmfloorrover;
		ffloor_t *oldceilrover = tmceilingrover;
		pslope_t *oldfslope = tmfloorslope;
		pslope_t *oldcslope = tmceilingslope;


		P_TryMove(thing, thing->x+stand->momx, thing->y+stand->momy, true);


		floatok = oldfltok;
		tmfloorz = oldflrz;
		tmceilingz = oldceilz;
		tmfloorthing = oldflrthing;
		P_SetTarget(&tmthing, oldthing);
		ceilingline = oldceilline;
		blockingline = oldblockline;
		tmfloorrover = oldflrrover;
		tmceilingrover = oldceilrover;
		tmfloorslope = oldfslope;
		tmceilingslope = oldcslope;
		thing->momz = stand->momz;
	}
	else
	{
		thing->momx = stand->momx;
		thing->momy = stand->momy;
		thing->momz = stand->momz;
	}
	return true;
}

static boolean
increment_move
(		mobj_t * thing,
		fixed_t x,
		fixed_t y,
		boolean allowdropoff)
{
	fixed_t tryx = thing->x;
	fixed_t tryy = thing->y;
	fixed_t radius = thing->radius;
	fixed_t thingtop;
	floatok = false;



	radius = max(radius, thing->scale);



	radius = min(radius, 16 * thing->scale);










	do {
		if (thing->flags & MF_NOCLIP) {
			tryx = x;
			tryy = y;
		} else {
			if (x-tryx > radius)
				tryx += radius;
			else if (x-tryx < -radius)
				tryx -= radius;
			else
				tryx = x;
			if (y-tryy > radius)
				tryy += radius;
			else if (y-tryy < -radius)
				tryy -= radius;
			else
				tryy = y;
		}

		if (!P_CheckPosition(thing, tryx, tryy) || P_MobjWasRemoved(thing))
			return false;  

		if (!(thing->flags & MF_NOCLIP))
		{

			fixed_t maxstep = FixedMul(MAXSTEPMOVE, thing->scale);

			if (thing->player)
			{

				if (P_PlayerTouchingSectorSpecialFlag(thing->player, SSF_DOUBLESTEPUP)
				|| (R_PointInSubsector(x, y)->sector->specialflags & SSF_DOUBLESTEPUP))
					maxstep <<= 1;


				if (P_PlayerTouchingSectorSpecialFlag(thing->player, SSF_NOSTEPDOWN)
				|| (R_PointInSubsector(x, y)->sector->specialflags & SSF_NOSTEPDOWN))
					maxstep = 0;



				if (thing->player->panim == PA_SPRING
				&& P_MobjFlip(thing)*thing->momz > FixedMul(FRACUNIT, thing->scale))
					maxstep = 0;
			}
			else if (thing->flags & MF_PUSHABLE)
			{

				if (R_PointInSubsector(x, y)->sector->specialflags & SSF_DOUBLESTEPUP)
					maxstep <<= 1;


				if (R_PointInSubsector(x, y)->sector->specialflags & SSF_NOSTEPDOWN)
					maxstep = 0;
			}

			if (thing->type == MT_SKIM)
				maxstep = 0;

			if (tmceilingz - tmfloorz < thing->height
				|| (thing->player
					&& tmceilingz - tmfloorz < P_GetPlayerHeight(thing->player)
					&& !P_PlayerCanEnterSpinGaps(thing->player)))
			{
				if (tmfloorthing)
					tmhitthing = tmfloorthing;
				return false;  
			}

			floatok = true;

			thingtop = thing->z + thing->height;


			if (thing->z < tmfloorz)
			{
				if (maxstep > 0 && tmfloorz - thing->z <= maxstep)
				{
					thing->z = thing->floorz = tmfloorz;
					thing->floorrover = tmfloorrover;
					thing->eflags |= MFE_JUSTSTEPPEDDOWN;
				}
				else
				{
					return false;  
				}
			}
			else if (tmceilingz < thingtop)
			{
				if (maxstep > 0 && thingtop - tmceilingz <= maxstep)
				{
					thing->z = ( thing->ceilingz = tmceilingz ) - thing->height;
					thing->ceilingrover = tmceilingrover;
					thing->eflags |= MFE_JUSTSTEPPEDDOWN;
				}
				else
				{
					return false;  
				}
			}
			else if (maxstep > 0)  
			{



				if (thingtop == thing->ceilingz && tmceilingz > thingtop && tmceilingz - thingtop <= maxstep)
				{
					thing->z = (thing->ceilingz = tmceilingz) - thing->height;
					thing->ceilingrover = tmceilingrover;
					thing->eflags |= MFE_JUSTSTEPPEDDOWN;
				}
				else if (thing->z == thing->floorz && tmfloorz < thing->z && thing->z - tmfloorz <= maxstep)
				{
					thing->z = thing->floorz = tmfloorz;
					thing->floorrover = tmfloorrover;
					thing->eflags |= MFE_JUSTSTEPPEDDOWN;
				}
			}

			if (!allowdropoff && !(thing->flags & MF_FLOAT) && thing->type != MT_SKIM && !tmfloorthing)
			{
				if (thing->eflags & MFE_VERTICALFLIP)
				{
					if (tmdrpoffceilz - tmceilingz > maxstep)
						return false;
				}
				else if (tmfloorz - tmdropoffz > maxstep)
					return false;  
			}
		}
	} while (tryx != x || tryy != y);

	return true;
}





boolean P_CheckMove(mobj_t *thing, fixed_t x, fixed_t y, boolean allowdropoff)
{
	boolean moveok;
	mobj_t *hack = P_SpawnMobjFromMobj(thing, 0, 0, 0, MT_RAY);
	if (P_MobjWasRemoved(hack))
		return false;

	hack->radius = thing->radius;
	hack->height = thing->height;

	moveok = increment_move(hack, x, y, allowdropoff);
	P_RemoveMobj(hack);

	return moveok;
}





boolean P_TryMove(mobj_t *thing, fixed_t x, fixed_t y, boolean allowdropoff)
{
	fixed_t startingonground = P_IsObjectOnGround(thing);


	if (!increment_move(thing, x, y, allowdropoff))
		return false;



	if (thing->flags & MF_PUSHABLE)
	{
		INT32 xl, xh, yl, yh;

		yh = (unsigned)(thing->y + thing->radius - bmaporgy)>>MAPBLOCKSHIFT;
		yl = (unsigned)(thing->y - thing->radius - bmaporgy)>>MAPBLOCKSHIFT;
		xh = (unsigned)(thing->x + thing->radius - bmaporgx)>>MAPBLOCKSHIFT;
		xl = (unsigned)(thing->x - thing->radius - bmaporgx)>>MAPBLOCKSHIFT;

		BMBOUNDFIX(xl, xh, yl, yh);

		stand = thing;
		standx = x;
		standy = y;

		P_DoBlockThingsIterate(xl, yl, xh, yh, PIT_PushableMoved);
	}


	P_UnsetThingPosition(thing);

	thing->floorz = tmfloorz;
	thing->ceilingz = tmceilingz;
	thing->floorrover = tmfloorrover;
	thing->ceilingrover = tmceilingrover;

	if (!(thing->flags & MF_NOCLIPHEIGHT))
	{

		if (thing->z <= tmfloorz && !(thing->eflags & MFE_VERTICALFLIP)) {
			if (!startingonground && tmfloorslope)
				P_HandleSlopeLanding(thing, tmfloorslope);

			if (thing->momz <= 0)
			{
				thing->standingslope = tmfloorslope;
				P_SetPitchRollFromSlope(thing, thing->standingslope);

				if (thing->momz == 0 && thing->player && !startingonground)
					P_PlayerHitFloor(thing->player, true);
			}
		}
		else if (thing->z+thing->height >= tmceilingz && (thing->eflags & MFE_VERTICALFLIP)) {
			if (!startingonground && tmceilingslope)
				P_HandleSlopeLanding(thing, tmceilingslope);

			if (thing->momz >= 0)
			{
				thing->standingslope = tmceilingslope;
				P_SetPitchRollFromSlope(thing, thing->standingslope);

				if (thing->momz == 0 && thing->player && !startingonground)
					P_PlayerHitFloor(thing->player, true);
			}
		}
	}
	else  
		thing->standingslope = NULL;

	thing->x = x;
	thing->y = y;

	if (tmfloorthing)
		thing->eflags &= ~MFE_ONGROUND;  
	else
		thing->eflags |= MFE_ONGROUND;

	P_SetThingPosition(thing);
	return true;
}

boolean P_SceneryTryMove(mobj_t *thing, fixed_t x, fixed_t y)
{
	fixed_t tryx, tryy;
	I_Assert(!P_MobjWasRemoved(thing));

	tryx = thing->x;
	tryy = thing->y;
	do {
		if (x-tryx > MAXTRYMOVE)
			tryx += MAXTRYMOVE;
		else if (x-tryx < -MAXTRYMOVE)
			tryx -= MAXTRYMOVE;
		else
			tryx = x;
		if (y-tryy > MAXTRYMOVE)
			tryy += MAXTRYMOVE;
		else if (y-tryy < -MAXTRYMOVE)
			tryy -= MAXTRYMOVE;
		else
			tryy = y;

		if (!P_CheckPosition(thing, tryx, tryy) || P_MobjWasRemoved(thing))
			return false;  

		if (!(thing->flags & MF_NOCLIP))
		{
			const fixed_t maxstep = MAXSTEPMOVE;

			if (tmceilingz - tmfloorz < thing->height)
				return false;  

			if (tmceilingz - thing->z < thing->height)
				return false;  

			if (tmfloorz - thing->z > maxstep)
				return false;  
		}
	} while(tryx != x || tryy != y);



	P_UnsetThingPosition(thing);

	thing->floorz = tmfloorz;
	thing->ceilingz = tmceilingz;
	thing->floorrover = tmfloorrover;
	thing->ceilingrover = tmceilingrover;
	thing->x = x;
	thing->y = y;

	if (tmfloorthing)
		thing->eflags &= ~MFE_ONGROUND;  
	else
		thing->eflags |= MFE_ONGROUND;

	P_SetThingPosition(thing);
	return true;
}











static boolean P_ThingHeightClip(mobj_t *thing)
{
	boolean floormoved;
	fixed_t oldfloorz = thing->floorz, oldz = thing->z;
	ffloor_t *oldfloorrover = thing->floorrover;
	ffloor_t *oldceilingrover = thing->ceilingrover;
	boolean onfloor = P_IsObjectOnGround(thing); 
	ffloor_t *rover = NULL;
	boolean bouncing;
	boolean hitfloor = false;

	if (thing->flags & MF_NOCLIPHEIGHT)
		return true;

	P_CheckPosition(thing, thing->x, thing->y);

	if (P_MobjWasRemoved(thing))
		return true;

	floormoved = (thing->eflags & MFE_VERTICALFLIP && tmceilingz != thing->ceilingz)
		|| (!(thing->eflags & MFE_VERTICALFLIP) && tmfloorz != thing->floorz);

	thing->floorz = tmfloorz;
	thing->ceilingz = tmceilingz;
	thing->floorrover = tmfloorrover;
	thing->ceilingrover = tmceilingrover;



	if (tmfloorz > oldfloorz+thing->height)
		return true;

	bouncing = thing->player && thing->state-states == S_PLAY_BOUNCE_LANDING && P_IsObjectOnGround(thing);

	if ((onfloor || bouncing) && !(thing->flags & MF_NOGRAVITY) && floormoved)
	{
		rover = (thing->eflags & MFE_VERTICALFLIP) ? oldceilingrover : oldfloorrover;



		if (!rover || ((rover->fofflags & FOF_EXISTS) && (rover->fofflags & FOF_SOLID)))
		{
			hitfloor = bouncing;
			if (!(thing->player) || !(thing->player->pflags & PF_JUMPED || bouncing))
			{
				if (thing->eflags & MFE_VERTICALFLIP)
					thing->pmomz = thing->ceilingz - (thing->z + thing->height);
				else
					thing->pmomz = thing->floorz - thing->z;
				thing->eflags |= MFE_APPLYPMOMZ;
			}

			if (thing->eflags & MFE_VERTICALFLIP)
				thing->z = thing->ceilingz - thing->height;
			else
				thing->z = thing->floorz;
		}
	}
	else if (!tmfloorthing)
	{

		if (thing->eflags & MFE_VERTICALFLIP)
		{
			if (!onfloor && thing->z < tmfloorz)
				thing->z = thing->floorz;
		}
		else if (!onfloor && thing->z + thing->height > tmceilingz)
			thing->z = thing->ceilingz - thing->height;
	}

	if ((P_MobjFlip(thing)*(thing->z - oldz) > 0 || hitfloor) && thing->player)
		P_PlayerHitFloor(thing->player, !onfloor);


	thing->eflags &= ~MFE_ONGROUND;

	if (thing->ceilingz - thing->floorz < thing->height && thing->z >= thing->floorz)




		return false;

	return true;
}





static fixed_t bestslidefrac, secondslidefrac;
static line_t *bestslideline;
static line_t *secondslideline;
static mobj_t *slidemo;
static fixed_t tmxmove, tmymove;




static void P_HitCameraSlideLine(line_t *ld, camera_t *thiscam)
{
	INT32 side;
	angle_t lineangle, moveangle, deltaangle;
	fixed_t movelen, newlen;

	if (ld->slopetype == ST_HORIZONTAL)
	{
		tmymove = 0;
		return;
	}

	if (ld->slopetype == ST_VERTICAL)
	{
		tmxmove = 0;
		return;
	}

	side = P_PointOnLineSide(thiscam->x, thiscam->y, ld);
	lineangle = ld->angle;

	if (side == 1)
		lineangle += ANGLE_180;

	moveangle = R_PointToAngle2(0, 0, tmxmove, tmymove);
	deltaangle = moveangle-lineangle;

	if (deltaangle > ANGLE_180)
		deltaangle += ANGLE_180;

	lineangle >>= ANGLETOFINESHIFT;
	deltaangle >>= ANGLETOFINESHIFT;

	movelen = P_AproxDistance(tmxmove, tmymove);
	newlen = FixedMul(movelen, FINECOSINE(deltaangle));

	tmxmove = FixedMul(newlen, FINECOSINE(lineangle));
	tmymove = FixedMul(newlen, FINESINE(lineangle));
}






static void P_HitSlideLine(line_t *ld)
{
	INT32 side;
	angle_t lineangle, moveangle, deltaangle;
	fixed_t movelen, newlen;

	if (ld->slopetype == ST_HORIZONTAL)
	{
		tmymove = 0;
		return;
	}

	if (ld->slopetype == ST_VERTICAL)
	{
		tmxmove = 0;
		return;
	}

	side = P_PointOnLineSide(slidemo->x, slidemo->y, ld);

	lineangle = ld->angle;

	if (side == 1)
		lineangle += ANGLE_180;

	moveangle = R_PointToAngle2(0, 0, tmxmove, tmymove);
	deltaangle = moveangle-lineangle;

	if (deltaangle > ANGLE_180)
		deltaangle += ANGLE_180;

	lineangle >>= ANGLETOFINESHIFT;
	deltaangle >>= ANGLETOFINESHIFT;

	movelen = R_PointToDist2(0, 0, tmxmove, tmymove);
	newlen = FixedMul(movelen, FINECOSINE(deltaangle));

	tmxmove = FixedMul(newlen, FINECOSINE(lineangle));
	tmymove = FixedMul(newlen, FINESINE(lineangle));
}






static void P_HitBounceLine(line_t *ld)
{
	angle_t lineangle, moveangle, deltaangle;
	fixed_t movelen;

	if (ld->slopetype == ST_HORIZONTAL)
	{
		tmymove = -tmymove;
		return;
	}

	if (ld->slopetype == ST_VERTICAL)
	{
		tmxmove = -tmxmove;
		return;
	}

	lineangle = ld->angle;

	if (lineangle >= ANGLE_180)
		lineangle -= ANGLE_180;

	moveangle = R_PointToAngle2(0, 0, tmxmove, tmymove);
	deltaangle = moveangle + 2*(lineangle - moveangle);

	lineangle >>= ANGLETOFINESHIFT;
	deltaangle >>= ANGLETOFINESHIFT;

	movelen = P_AproxDistance(tmxmove, tmymove);

	tmxmove = FixedMul(movelen, FINECOSINE(deltaangle));
	tmymove = FixedMul(movelen, FINESINE(deltaangle));

	deltaangle = R_PointToAngle2(0, 0, tmxmove, tmymove);
}




static boolean PTR_SlideCameraTraverse(intercept_t *in)
{
	line_t *li;

	I_Assert(in->isaline);

	li = in->d.line;


	if (!li->backsector)
	{
		if (P_PointOnLineSide(mapcampointer->x, mapcampointer->y, li))
			return true;  
		goto isblocking;
	}


	P_CameraLineOpening(li);

	if (openrange < mapcampointer->height)
		goto isblocking;  

	if (opentop - mapcampointer->z < mapcampointer->height)
		goto isblocking;  

	if (openbottom - mapcampointer->z > 0)  
		goto isblocking;  


	return true;



isblocking:
	{
		if (in->frac < bestslidefrac)
		{
			secondslidefrac = bestslidefrac;
			secondslideline = bestslideline;
			bestslidefrac = in->frac;
			bestslideline = li;
		}
	}

	return false;  
}






static boolean P_IsClimbingValid(player_t *player, angle_t angle)
{
	fixed_t platx, platy;
	sector_t *glidesector;
	fixed_t floorz, ceilingz;
	mobj_t *mo = player->mo;
	ffloor_t *rover;

	platx = P_ReturnThrustX(mo, angle, mo->radius + FixedMul(8*FRACUNIT, mo->scale));
	platy = P_ReturnThrustY(mo, angle, mo->radius + FixedMul(8*FRACUNIT, mo->scale));

	glidesector = R_PointInSubsector(mo->x + platx, mo->y + platy)->sector;

	floorz   = P_GetSectorFloorZAt  (glidesector, mo->x, mo->y);
	ceilingz = P_GetSectorCeilingZAt(glidesector, mo->x, mo->y);

	if (glidesector != mo->subsector->sector)
	{
		boolean floorclimb = false;
		fixed_t topheight, bottomheight;

		for (rover = glidesector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_BLOCKPLAYER))
				continue;

			topheight    = P_GetFFloorTopZAt   (rover, mo->x, mo->y);
			bottomheight = P_GetFFloorBottomZAt(rover, mo->x, mo->y);

			floorclimb = true;

			if (mo->eflags & MFE_VERTICALFLIP)
			{
				if ((topheight < mo->z + mo->height) && ((mo->z + mo->height + mo->momz) < topheight))
					floorclimb = true;
				if (topheight < mo->z)  
					floorclimb = false;
				if (bottomheight > mo->z + mo->height - FixedMul(16*FRACUNIT,mo->scale))
					floorclimb = false;
			}
			else
			{
				if ((bottomheight > mo->z) && ((mo->z - mo->momz) > bottomheight))
					floorclimb = true;
				if (bottomheight > mo->z + mo->height)  
					floorclimb = false;
				if (topheight < mo->z + FixedMul(16*FRACUNIT,mo->scale))
					floorclimb = false;
			}

			if (floorclimb)
				break;
		}

		if (mo->eflags & MFE_VERTICALFLIP)
		{
			if ((floorz <= mo->z + mo->height)
				&& ((mo->z + mo->height - mo->momz) <= floorz))
				floorclimb = true;

			if ((floorz > mo->z)
				&& glidesector->floorpic == skyflatnum)
				return false;

			if ((mo->z + mo->height - FixedMul(16*FRACUNIT,mo->scale) > ceilingz)
				|| (mo->z + mo->height <= floorz))
				floorclimb = true;
		}
		else
		{
			if ((ceilingz >= mo->z)
				&& ((mo->z - mo->momz) >= ceilingz))
				floorclimb = true;

			if ((ceilingz < mo->z+mo->height)
				&& glidesector->ceilingpic == skyflatnum)
				return false;

			if ((mo->z + FixedMul(16*FRACUNIT,mo->scale) < floorz)
				|| (mo->z >= ceilingz))
				floorclimb = true;
		}

		if (!floorclimb)
			return false;

		return true;
	}

	return false;
}






boolean P_LineIsBlocking(mobj_t *mo, line_t *li)
{

	if (!li->backsector)
		return !P_PointOnLineSide(mo->x, mo->y, li);

	if (!(mo->flags & MF_MISSILE))
	{
		if (li->flags & ML_IMPASSIBLE)
			return true;

		if ((mo->flags & (MF_ENEMY|MF_BOSS)) && li->flags & ML_BLOCKMONSTERS)
			return true;
	}


	P_LineOpening(li, mo);

	if (openrange < mo->height)
		return true;  

	if (opentop - mo->z < mo->height)
		return true;  

	if (openbottom - mo->z > FixedMul(MAXSTEPMOVE, mo->scale))
		return true;  

	if (mo->player
		&& openrange < P_GetPlayerHeight(mo->player)
		&& !P_PlayerCanEnterSpinGaps(mo->player))
			return true;  

	return false;
}

static void PTR_GlideClimbTraverse(line_t *li)
{
	line_t *checkline = li;
	ffloor_t *rover;
	fixed_t topheight, bottomheight;
	boolean fofline = false;
	sector_t *checksector = (li->backsector && !P_PointOnLineSide(slidemo->x, slidemo->y, li)) ? li->backsector : li->frontsector;

	if (checksector->ffloors)
	{
		for (rover = checksector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_BLOCKPLAYER) || ((rover->fofflags & FOF_BUSTUP) && (slidemo->player->charflags & SF_CANBUSTWALLS)))
				continue;

			topheight    = P_GetFFloorTopZAt   (rover, slidemo->x, slidemo->y);
			bottomheight = P_GetFFloorBottomZAt(rover, slidemo->x, slidemo->y);

			if (topheight < slidemo->z)
				continue;

			if (bottomheight > slidemo->z + slidemo->height)
				continue;


			if (rover->master->flags & ML_TFERLINE)
			{
				size_t linenum = li-checksector->lines[0];
				checkline = rover->master->frontsector->lines[0] + linenum;
				fofline = true;
			}

			break;
		}
	}


	if (!(checkline->flags & ML_NOCLIMB) && checkline->special != SPECIAL_HORIZON_LINE)
	{
		boolean canclimb;
		angle_t climbangle, climbline;
		INT32 whichside = P_PointOnLineSide(slidemo->x, slidemo->y, li);

		climbangle = climbline = R_PointToAngle2(li->v1->x, li->v1->y, li->v2->x, li->v2->y);

		if (whichside)  
			climbline += ANGLE_180;

		climbangle += (ANGLE_90 * (whichside ? -1 : 1));

		canclimb = (li->backsector ? P_IsClimbingValid(slidemo->player, climbangle) : true);

		if (((!slidemo->player->climbing && abs((signed)(slidemo->angle - ANGLE_90 - climbline)) < ANGLE_45)
			|| (slidemo->player->climbing == 1 && abs((signed)(slidemo->angle - climbline)) < ANGLE_135))
			&& canclimb)
		{
			slidemo->angle = climbangle;
			                                                                       
                                                        

			if (!slidemo->player->climbing)
			{
				S_StartSound(slidemo, sfx_s3k4a);
				slidemo->player->climbing = 5;
				if (slidemo->player->powers[pw_super])
				{
					P_Earthquake(slidemo, slidemo, 256*FRACUNIT);
					S_StartSound(slidemo, sfx_s3k49);
				}
			}

			slidemo->player->pflags &= ~(PF_GLIDING|PF_SPINNING|PF_JUMPED|PF_NOJUMPDAMAGE|PF_THOKKED);
			slidemo->player->glidetime = 0;
			slidemo->player->secondjump = 0;

			if (slidemo->player->climbing > 1)
				slidemo->momz = slidemo->momx = slidemo->momy = 0;

			if (fofline)
				whichside = 0;


			slidemo->player->lastsidehit = checkline->sidenum[whichside];
			slidemo->player->lastlinehit = (INT16)(checkline - lines);

			P_Thrust(slidemo, slidemo->angle, FixedMul(5*FRACUNIT, slidemo->scale));
		}
	}
}

static boolean PTR_SlideTraverse(intercept_t *in)
{
	line_t *li;

	I_Assert(in->isaline);

	li = in->d.line;

	if (!P_LineIsBlocking(slidemo, li))
		return true;



	if (li->polyobj && slidemo->player)
	{
		if ((li->polyobj->lines[0]->backsector->flags & MSF_TRIGGERSPECIAL_TOUCH) && !(li->polyobj->flags & POF_NOSPECIALS))
			P_ProcessSpecialSector(slidemo->player, slidemo->subsector->sector, li->polyobj->lines[0]->backsector);
	}

	if (slidemo->player)
	{
		if (slidemo->player->charability == CA_GLIDEANDCLIMB
			&& (slidemo->player->pflags & PF_GLIDING || slidemo->player->climbing))
			PTR_GlideClimbTraverse(li);
		else
		{
			slidemo->player->lastsidehit = li->sidenum[P_PointOnLineSide(slidemo->x, slidemo->y, li)];
			slidemo->player->lastlinehit = (INT16)(li - lines);
		}
	}

	if (in->frac < bestslidefrac && (!slidemo->player || !slidemo->player->climbing))
	{
		secondslidefrac = bestslidefrac;
		secondslideline = bestslideline;
		bestslidefrac = in->frac;
		bestslideline = li;
	}

	return false;  
}






void P_SlideCameraMove(camera_t *thiscam)
{
	fixed_t leadx, leady, trailx, traily, newx, newy;
	INT32 hitcount = 0;
	INT32 retval = 0;

	bestslideline = NULL;

retry:
	if (++hitcount == 3)
		goto stairstep;  


	if (thiscam->momx > 0)
	{
		leadx = thiscam->x + thiscam->radius;
		trailx = thiscam->x - thiscam->radius;
	}
	else
	{
		leadx = thiscam->x - thiscam->radius;
		trailx = thiscam->x + thiscam->radius;
	}

	if (thiscam->momy > 0)
	{
		leady = thiscam->y + thiscam->radius;
		traily = thiscam->y - thiscam->radius;
	}
	else
	{
		leady = thiscam->y - thiscam->radius;
		traily = thiscam->y + thiscam->radius;
	}

	bestslidefrac = FRACUNIT+1;

	mapcampointer = thiscam;

	P_PathTraverse(leadx, leady, leadx + thiscam->momx, leady + thiscam->momy,
		PT_ADDLINES, PTR_SlideCameraTraverse);
	P_PathTraverse(trailx, leady, trailx + thiscam->momx, leady + thiscam->momy,
		PT_ADDLINES, PTR_SlideCameraTraverse);
	P_PathTraverse(leadx, traily, leadx + thiscam->momx, traily + thiscam->momy,
		PT_ADDLINES, PTR_SlideCameraTraverse);


	if (bestslidefrac == FRACUNIT+1)
	{
		retval = P_TryCameraMove(thiscam->x, thiscam->y + thiscam->momy, thiscam);

stairstep:
		if (!retval)  
			P_TryCameraMove(thiscam->x + thiscam->momx, thiscam->y, thiscam);
		return;
	}


	bestslidefrac -= 0x800;
	if (bestslidefrac > 0)
	{
		newx = FixedMul(thiscam->momx, bestslidefrac);
		newy = FixedMul(thiscam->momy, bestslidefrac);

		retval = P_TryCameraMove(thiscam->x + newx, thiscam->y + newy, thiscam);

		if (!retval)
			goto stairstep;
	}



	bestslidefrac = FRACUNIT - (bestslidefrac+0x800);

	if (bestslidefrac > FRACUNIT)
		bestslidefrac = FRACUNIT;

	if (bestslidefrac <= 0)
		return;

	tmxmove = FixedMul(thiscam->momx, bestslidefrac);
	tmymove = FixedMul(thiscam->momy, bestslidefrac);

	P_HitCameraSlideLine(bestslideline, thiscam);  

	thiscam->momx = tmxmove;
	thiscam->momy = tmymove;

	retval = P_TryCameraMove(thiscam->x + tmxmove, thiscam->y + tmymove, thiscam);

	if (!retval)
		goto retry;
}

static void P_CheckLavaWall(mobj_t *mo, sector_t *sec)
{
	ffloor_t *rover;
	fixed_t topheight, bottomheight;

	for (rover = sec->ffloors; rover; rover = rover->next)
	{
		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!(rover->fofflags & FOF_SWIMMABLE))
			continue;

		if (rover->master->frontsector->damagetype != SD_LAVA)
			continue;

		topheight = P_GetFFloorTopZAt(rover, mo->x, mo->y);

		if (mo->eflags & MFE_VERTICALFLIP)
		{
			if (topheight < mo->z - mo->height)
				continue;
		}
		else
		{
			if (topheight < mo->z)
				continue;
		}

		bottomheight = P_GetFFloorBottomZAt(rover, mo->x, mo->y);

		if (mo->eflags & MFE_VERTICALFLIP)
		{
			if (bottomheight > mo->z)
				continue;
		}
		else
		{
			if (bottomheight > mo->z + mo->height)
				continue;
		}

		P_DamageMobj(mo, NULL, NULL, 1, DMG_FIRE);
		return;
	}
}

static inline void P_StairStepSlideMove(mobj_t *mo)
{
	if (!P_TryMove(mo, mo->x, mo->y + mo->momy, true) && !P_MobjWasRemoved(mo))  
		P_TryMove(mo, mo->x + mo->momx, mo->y, true);
}










void P_SlideMove(mobj_t *mo)
{
	fixed_t leadx, leady, trailx, traily, newx, newy;
	INT16 hitcount = 0;
	boolean success = false;

	boolean papercol = false;
	vertex_t v1, v2;  
	static line_t junk;  

	memset(&junk, 0x00, sizeof(junk));

	I_Assert(!P_MobjWasRemoved(mo));

	if (tmhitthing && mo->z + mo->height > tmhitthing->z && mo->z < tmhitthing->z + tmhitthing->height)
	{

		if (tmhitthing->flags & MF_PUSHABLE)
			return;

		if (tmhitthing->flags & MF_PAPERCOLLISION)
		{
			fixed_t cosradius, sinradius, num, den;


			if (mo->momx > 0)
			{
				leadx = mo->x + mo->radius;
				trailx = mo->x - mo->radius;
			}
			else
			{
				leadx = mo->x - mo->radius;
				trailx = mo->x + mo->radius;
			}

			if (mo->momy > 0)
			{
				leady = mo->y + mo->radius;
				traily = mo->y - mo->radius;
			}
			else
			{
				leady = mo->y - mo->radius;
				traily = mo->y + mo->radius;
			}

			papercol = true;
			slidemo = mo;
			bestslideline = &junk;

			cosradius = FixedMul(tmhitthing->radius, FINECOSINE(tmhitthing->angle>>ANGLETOFINESHIFT));
			sinradius = FixedMul(tmhitthing->radius, FINESINE(tmhitthing->angle>>ANGLETOFINESHIFT));

			v1.x = tmhitthing->x - cosradius;
			v1.y = tmhitthing->y - sinradius;
			v2.x = tmhitthing->x + cosradius;
			v2.y = tmhitthing->y + sinradius;


			if (sinradius && mo->y + mo->radius <= min(v1.y, v2.y))
			{
				mo->momy = 0;
				P_TryMove(mo, mo->x + mo->momx, min(v1.y, v2.y) - mo->radius, true);
				return;
			}
			else if (sinradius && mo->y - mo->radius >= max(v1.y, v2.y))
			{
				mo->momy = 0;
				P_TryMove(mo, mo->x + mo->momx, max(v1.y, v2.y) + mo->radius, true);
				return;
			}
			else if (cosradius && mo->x + mo->radius <= min(v1.x, v2.x))
			{
				mo->momx = 0;
				P_TryMove(mo, min(v1.x, v2.x) - mo->radius, mo->y + mo->momy, true);
				return;
			}
			else if (cosradius && mo->x - mo->radius >= max(v1.x, v2.x))
			{
				mo->momx = 0;
				P_TryMove(mo, max(v1.x, v2.x) + mo->radius, mo->y + mo->momy, true);
				return;
			}


			junk.v1 = &v1;
			junk.v2 = &v2;
			junk.dx = 2*cosradius;  
			junk.dy = 2*sinradius;  

			junk.slopetype = !cosradius ? ST_VERTICAL : !sinradius ? ST_HORIZONTAL :
			((sinradius > 0) == (cosradius > 0)) ? ST_POSITIVE : ST_NEGATIVE;

			bestslidefrac = FRACUNIT+1;

			den = FixedMul(junk.dy>>8, mo->momx) - FixedMul(junk.dx>>8, mo->momy);

			if (!den)
				bestslidefrac = 0;
			else
			{
				fixed_t frac;
#define P_PaperTraverse(startx, starty) \
				num = FixedMul((v1.x - leadx)>>8, junk.dy) + FixedMul((leady - v1.y)>>8, junk.dx); \
				frac = FixedDiv(num, den); \
				if (frac < bestslidefrac) \
					bestslidefrac = frac
				P_PaperTraverse(leadx, leady);
				P_PaperTraverse(trailx, leady);
				P_PaperTraverse(leadx, traily);
#undef dowork
			}

			goto papercollision;
		}


		if (mo->y + mo->radius <= tmhitthing->y - tmhitthing->radius)
		{
			mo->momy = 0;
			P_TryMove(mo, mo->x + mo->momx, tmhitthing->y - tmhitthing->radius - mo->radius, true);
		}
		else if (mo->y - mo->radius >= tmhitthing->y + tmhitthing->radius)
		{
			mo->momy = 0;
			P_TryMove(mo, mo->x + mo->momx, tmhitthing->y + tmhitthing->radius + mo->radius, true);
		}
		else if (mo->x + mo->radius <= tmhitthing->x - tmhitthing->radius)
		{
			mo->momx = 0;
			P_TryMove(mo, tmhitthing->x - tmhitthing->radius - mo->radius, mo->y + mo->momy, true);
		}
		else if (mo->x - mo->radius >= tmhitthing->x + tmhitthing->radius)
		{
			mo->momx = 0;
			P_TryMove(mo, tmhitthing->x + tmhitthing->radius + mo->radius, mo->y + mo->momy, true);
		}
		else
			mo->momx = mo->momy = 0;
		return;
	}

	slidemo = mo;
	bestslideline = NULL;

retry:
	if ((++hitcount == 3) || papercol)
	{
		P_StairStepSlideMove(mo);
		return;
	}


	if (mo->momx > 0)
	{
		leadx = mo->x + mo->radius;
		trailx = mo->x - mo->radius;
	}
	else
	{
		leadx = mo->x - mo->radius;
		trailx = mo->x + mo->radius;
	}

	if (mo->momy > 0)
	{
		leady = mo->y + mo->radius;
		traily = mo->y - mo->radius;
	}
	else
	{
		leady = mo->y - mo->radius;
		traily = mo->y + mo->radius;
	}

	bestslidefrac = FRACUNIT+1;

	P_PathTraverse(leadx, leady, leadx + mo->momx, leady + mo->momy,
		PT_ADDLINES, PTR_SlideTraverse);
	P_PathTraverse(trailx, leady, trailx + mo->momx, leady + mo->momy,
		PT_ADDLINES, PTR_SlideTraverse);
	P_PathTraverse(leadx, traily, leadx + mo->momx, traily + mo->momy,
		PT_ADDLINES, PTR_SlideTraverse);

	if (bestslideline && mo->player && bestslideline->sidenum[1] != NO_SIDEDEF)
	{
		sector_t *sec = P_PointOnLineSide(mo->x, mo->y, bestslideline) ? bestslideline->frontsector : bestslideline->backsector;
		P_CheckLavaWall(mo, sec);
	}


	if (bestslideline && bestslideline->flags & ML_BOUNCY)
	{
		P_BounceMove(mo);
		return;
	}

papercollision:

	if (bestslidefrac == FRACUNIT+1)
	{

		P_StairStepSlideMove(mo);
		return;
	}


	bestslidefrac -= 0x800;
	if (bestslidefrac > 0)
	{
		newx = FixedMul(mo->momx, bestslidefrac);
		newy = FixedMul(mo->momy, bestslidefrac);

		if (!P_TryMove(mo, mo->x + newx, mo->y + newy, true))
		{
			if (!P_MobjWasRemoved(mo))
				P_StairStepSlideMove(mo);
			return;
		}
		if (P_MobjWasRemoved(mo))
			return;
	}



	bestslidefrac = FRACUNIT - (bestslidefrac+0x800);

	if (bestslidefrac > FRACUNIT)
		bestslidefrac = FRACUNIT;

	if (bestslidefrac <= 0)
		return;

	tmxmove = FixedMul(mo->momx, bestslidefrac);
	tmymove = FixedMul(mo->momy, bestslidefrac);

	P_HitSlideLine(bestslideline);  

	if ((twodlevel || (mo->flags2 & MF2_TWOD)) && mo->player)
	{
		mo->momx = tmxmove;
		tmymove = 0;
	}
	else
	{
		mo->momx = tmxmove;
		mo->momy = tmymove;
	}

	const fixed_t tmradius = mo->radius > 8 ? mo->radius : 8;

	do {
		if (tmxmove > tmradius) {
			newx = mo->x + tmradius;
			tmxmove -= tmradius;
		} else if (tmxmove < -tmradius) {
			newx = mo->x - tmradius;
			tmxmove += tmradius;
		} else {
			newx = mo->x + tmxmove;
			tmxmove = 0;
		}
		if (tmymove > tmradius) {
			newy = mo->y + tmradius;
			tmymove -= tmradius;
		} else if (tmymove < -tmradius) {
			newy = mo->y - tmradius;
			tmymove += tmradius;
		} else {
			newy = mo->y + tmymove;
			tmymove = 0;
		}
		if (!P_TryMove(mo, newx, newy, true)) {
			if (success || P_MobjWasRemoved(mo))
				return;  
			else
				goto retry;
		}
		if (P_MobjWasRemoved(mo))
			return;
		success = true;
	} while(tmxmove || tmymove);
}






void P_BounceMove(mobj_t *mo)
{
	fixed_t leadx, leady;
	fixed_t trailx, traily;
	fixed_t newx, newy;
	INT32 hitcount;
	fixed_t mmomx = 0, mmomy = 0;

	slidemo = mo;
	hitcount = 0;

	do
	{
		if (++hitcount == 3)
			goto bounceback;  

		if (mo->player)
		{
			mmomx = mo->player->rmomx;
			mmomy = mo->player->rmomy;
		}
		else
		{
			mmomx = mo->momx;
			mmomy = mo->momy;
		}


		if (mo->momx > 0)
		{
			leadx = mo->x + mo->radius;
			trailx = mo->x - mo->radius;
		}
		else
		{
			leadx = mo->x - mo->radius;
			trailx = mo->x + mo->radius;
		}

		if (mo->momy > 0)
		{
			leady = mo->y + mo->radius;
			traily = mo->y - mo->radius;
		}
		else
		{
			leady = mo->y - mo->radius;
			traily = mo->y + mo->radius;
		}

		bestslidefrac = FRACUNIT + 1;

		P_PathTraverse(leadx, leady, leadx + mmomx, leady + mmomy, PT_ADDLINES, PTR_SlideTraverse);
		P_PathTraverse(trailx, leady, trailx + mmomx, leady + mmomy, PT_ADDLINES, PTR_SlideTraverse);
		P_PathTraverse(leadx, traily, leadx + mmomx, traily + mmomy, PT_ADDLINES, PTR_SlideTraverse);


		if (bestslidefrac == FRACUNIT + 1)
		{

bounceback:
			if (P_TryMove(mo, mo->x - mmomx, mo->y - mmomy, true))
			{
				mo->momx *= -1;
				mo->momy *= -1;
				mo->momx = FixedMul(mo->momx, (FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));
				mo->momy = FixedMul(mo->momy, (FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));

				if (mo->player)
				{
					mo->player->cmomx *= -1;
					mo->player->cmomy *= -1;
					mo->player->cmomx = FixedMul(mo->player->cmomx,
						(FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));
					mo->player->cmomy = FixedMul(mo->player->cmomy,
						(FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));
				}
			}
			return;
		}


		bestslidefrac -= 0x800;
		if (bestslidefrac > 0)
		{
			newx = FixedMul(mmomx, bestslidefrac);
			newy = FixedMul(mmomy, bestslidefrac);

			if (!P_TryMove(mo, mo->x + newx, mo->y + newy, true))
			{
				if (P_MobjWasRemoved(mo))
					return;
				goto bounceback;
			}
		}



		bestslidefrac = FRACUNIT - bestslidefrac;

		if (bestslidefrac > FRACUNIT)
			bestslidefrac = FRACUNIT;

		if (bestslidefrac <= 0)
			return;

		if (mo->type == MT_SHELL)
		{
			tmxmove = mmomx;
			tmymove = mmomy;
		}
		else if (mo->type == MT_THROWNBOUNCE)
		{
			tmxmove = FixedMul(mmomx, (FRACUNIT - (FRACUNIT>>6) - (FRACUNIT>>5)));
			tmymove = FixedMul(mmomy, (FRACUNIT - (FRACUNIT>>6) - (FRACUNIT>>5)));
		}
		else if (mo->type == MT_THROWNGRENADE || mo->type == MT_CYBRAKDEMON_NAPALM_BOMB_LARGE)
		{

			tmxmove = FixedDiv(mmomx, 2*FRACUNIT);
			tmymove = FixedDiv(mmomy, 2*FRACUNIT);
		}
		else
		{
			tmxmove = FixedMul(mmomx, (FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));
			tmymove = FixedMul(mmomy, (FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));
		}

		P_HitBounceLine(bestslideline);  

		mo->momx = tmxmove;
		mo->momy = tmymove;

		if (mo->player)
		{
			mo->player->cmomx = tmxmove;
			mo->player->cmomy = tmymove;
		}
	}
	while (!P_TryMove(mo, mo->x + tmxmove, mo->y + tmymove, true) && !P_MobjWasRemoved(mo));
}




static fixed_t bombdamage;
static mobj_t *bombsource;
static mobj_t *bombspot;
static UINT8 bombdamagetype;
static boolean bombsightcheck;






static boolean PIT_RadiusAttack(mobj_t *thing)
{
	fixed_t dx, dy, dz, dist;

	if (thing == bombspot)  
		return true;

	if (bombsource && thing->type == bombsource->type && !(bombdamagetype & DMG_CANHURTSELF))  
		return true;

	if (thing->type == MT_MINUS && !(thing->flags & (MF_SPECIAL|MF_SHOOTABLE)) && !bombsightcheck)
		thing->flags = (thing->flags & ~MF_NOCLIPTHING)|MF_SPECIAL|MF_SHOOTABLE;

	if (thing->type == MT_EGGGUARD && thing->tracer)  
		P_KillMobj(thing->tracer, bombspot, bombsource, bombdamagetype);

	if ((thing->flags & (MF_MONITOR|MF_SHOOTABLE)) != MF_SHOOTABLE)
		return true;

	dx = abs(thing->x - bombspot->x);
	dy = abs(thing->y - bombspot->y);
	dz = abs(thing->z + (thing->height>>1) - bombspot->z);

	dist = P_AproxDistance(P_AproxDistance(dx, dy), dz);
	dist -= thing->radius;

	if (dist < 0)
		dist = 0;

	if (dist >= bombdamage)
		return true;  

	if (thing->floorz > bombspot->z && bombspot->ceilingz < thing->z)
		return true;

	if (thing->ceilingz < bombspot->z && bombspot->floorz > thing->z)
		return true;

	if (!bombsightcheck || P_CheckSight(thing, bombspot))
	{	 
		P_DamageMobj(thing, bombspot, bombsource, 1, bombdamagetype);  
	}

	return true;
}





void P_RadiusAttack(mobj_t *spot, mobj_t *source, fixed_t damagedist, UINT8 damagetype, boolean sightcheck)
{
	INT32 xl, xh, yl, yh;
	fixed_t dist;

	dist = FixedMul(damagedist, spot->scale);

	yh = (unsigned)(spot->y + dist - bmaporgy)>>MAPBLOCKSHIFT;
	yl = (unsigned)(spot->y - dist - bmaporgy)>>MAPBLOCKSHIFT;
	xh = (unsigned)(spot->x + dist - bmaporgx)>>MAPBLOCKSHIFT;
	xl = (unsigned)(spot->x - dist - bmaporgx)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	bombspot = spot;
	bombsource = source;
	bombdamage = FixedMul(damagedist, spot->scale);
	bombdamagetype = damagetype;
	bombsightcheck = sightcheck;

	P_DoBlockThingsIterate(xl, yl, xh, yh, PIT_RadiusAttack);
}


















static boolean PIT_ChangeSector(mobj_t *thing, boolean realcrush, boolean crunch)
{
	mobj_t *killer = NULL;

	boolean immunepushable = ((thing->flags & (MF_PUSHABLE | MF_SHOOTABLE)) == MF_PUSHABLE);

	if (P_ThingHeightClip(thing))
	{

		return true;
	}

	if (!(thing->flags & (MF_SHOOTABLE|MF_PUSHABLE))
	|| thing->flags & MF_NOCLIPHEIGHT)
	{

		return true;
	}



	if (thing->z + thing->height > thing->ceilingz && thing->z <= thing->ceilingz)
	{
		if (immunepushable && thing->z + thing->height > thing->subsector->sector->ceilingheight)
			return false;  


		if (thing->subsector->sector->ffloors && (realcrush || immunepushable))
		{
			ffloor_t *rover;
			fixed_t topheight, bottomheight;
			fixed_t delta1, delta2;
			INT32 thingtop = thing->z + thing->height;

			for (rover = thing->subsector->sector->ffloors; rover; rover = rover->next)
			{
				thinker_t *think;

				if (!(rover->fofflags & FOF_EXISTS))
					continue;
				if (thing->player && !(rover->fofflags & FOF_BLOCKPLAYER))
					continue;
				if (!thing->player && !(rover->fofflags & FOF_BLOCKOTHERS))
					continue;

				topheight = *rover->topheight;
				bottomheight = *rover->bottomheight;

				if (bottomheight > thing->ceilingz)
					continue;

				delta1 = thing->z - (bottomheight + topheight)/2;
				delta2 = thingtop - (bottomheight + topheight)/2;
				if (abs(delta1) < abs(delta2))
					continue;

				if (immunepushable)
					return false;  

				if (!realcrush)
					continue;


				for (think = thlist[THINK_MAIN].next; think != &thlist[THINK_MAIN]; think = think->next)
				{
					crumble_t *crumbler;

					if (think->function.acp1 != (actionf_p1)T_StartCrumble)
						continue;

					crumbler = (crumble_t *)think;

					if (!crumbler->player)
						continue;
					if (!crumbler->player->mo)
						continue;
					if (crumbler->player->mo == thing)
						continue;
					if (crumbler->actionsector != thing->subsector->sector)
						continue;
					if (crumbler->sector != rover->master->frontsector)
						continue;

					killer = crumbler->player->mo;
				}
			}
		}

		if (realcrush)
		{

			if (netgame && thing->player && thing->player->spectator)
				P_DamageMobj(thing, NULL, NULL, 1, DMG_SPECTATOR);  
			else
				P_DamageMobj(thing, killer, killer, 1, DMG_CRUSHED);
			return true;
		}
	}

	if (realcrush && crunch)
		P_DamageMobj(thing, NULL, NULL, 1, 0);


	return true;
}

static boolean P_CheckSectorPolyObjects(sector_t *sector, boolean realcrush, boolean crunch)
{
	size_t i;



	validcount++;

	for (i = 0; i < sector->linecount; i++)
	{
		INT32 x, y;
		polyobj_t *po = sector->lines[i]->polyobj;

		if (!po)
			continue;
		if (po->validcount == validcount)
			continue;  
		if (!(po->flags & POF_SOLID))
			continue;
		if (po->lines[0]->backsector != sector)  
			continue;

		po->validcount = validcount;

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


					if (!P_MobjInsidePolyobj(po, mo))
						continue;

					if (!PIT_ChangeSector(mo, realcrush, crunch) && !realcrush)
						return false;
				}
			}
		}
	}

	return true;
}

static boolean P_CheckTouchingThinglist(sector_t *sector, boolean realcrush, boolean crunch)
{
	msecnode_t *n;

	for (n = sector->touching_thinglist; n; n = n->m_thinglist_next)
		n->visited = false;

	do
	{
		for (n = sector->touching_thinglist; n; n = n->m_thinglist_next)  
		{
			if (n->visited)
				continue;

			n->visited = true;  

			if (n->m_thing->flags & MF_NOBLOCKMAP)  
				continue;

			if (!PIT_ChangeSector(n->m_thing, realcrush, crunch) && !realcrush)  
				return false;

			break;  
		}
	} while (n);  

	return true;
}

static boolean P_CheckSectorFFloors(sector_t *sector, boolean realcrush, boolean crunch)
{
	sector_t *sec;
	size_t i;

	if (!sector->numattached)
		return true;

	for (i = 0; i < sector->numattached; i++)
	{
		sec = &sectors[sector->attached[i]];

		sec->moved = true;

		P_RecalcPrecipInSector(sec);

		if (!sector->attachedsolid[i])
			continue;

		if (!P_CheckTouchingThinglist(sec, realcrush, crunch))
			return false;
	}

	return true;
}

static boolean P_CheckSectorHelper(sector_t *sector, boolean realcrush, boolean crunch)
{
	if (!P_CheckSectorPolyObjects(sector, realcrush, crunch))
		return false;

	if (!P_CheckSectorFFloors(sector, realcrush, crunch))
		return false;


	sector->moved = true;

	return P_CheckTouchingThinglist(sector, realcrush, crunch);
}




boolean P_CheckSector(sector_t *sector, boolean crunch)
{









	if (!P_CheckSectorHelper(sector, false, crunch))
		return true;


	P_CheckSectorHelper(sector, true, crunch);

	return false;
}

  
               
                                                                   
  

static msecnode_t *headsecnode = NULL;
static mprecipsecnode_t *headprecipsecnode = NULL;

void P_Initsecnode(void)
{
	headsecnode = NULL;
	headprecipsecnode = NULL;
}




static msecnode_t *P_GetSecnode(void)
{
	msecnode_t *node;

	if (headsecnode)
	{
		node = headsecnode;
		headsecnode = headsecnode->m_thinglist_next;
	}
	else
		node = Z_Calloc(sizeof (*node), PU_LEVEL, NULL);
	return node;
}

static mprecipsecnode_t *P_GetPrecipSecnode(void)
{
	mprecipsecnode_t *node;

	if (headprecipsecnode)
	{
		node = headprecipsecnode;
		headprecipsecnode = headprecipsecnode->m_thinglist_next;
	}
	else
		node = Z_Calloc(sizeof (*node), PU_LEVEL, NULL);
	return node;
}



static inline void P_PutSecnode(msecnode_t *node)
{
	node->m_thinglist_next = headsecnode;
	headsecnode = node;
}


static inline void P_PutPrecipSecnode(mprecipsecnode_t *node)
{
	node->m_thinglist_next = headprecipsecnode;
	headprecipsecnode = node;
}






static msecnode_t *P_AddSecnode(sector_t *s, mobj_t *thing, msecnode_t *nextnode)
{
	msecnode_t *node;

	node = nextnode;
	while (node)
	{
		if (node->m_sector == s)  
		{
			node->m_thing = thing;  
			return nextnode;
		}
		node = node->m_sectorlist_next;
	}




	node = P_GetSecnode();


	node->visited = 0;

	node->m_sector = s;  
	node->m_thing = thing;  
	node->m_sectorlist_prev = NULL;  
	node->m_sectorlist_next = nextnode;  
	if (nextnode)
		nextnode->m_sectorlist_prev = node;  



	node->m_thinglist_prev = NULL;  
	node->m_thinglist_next = s->touching_thinglist;  
	if (s->touching_thinglist)
		node->m_thinglist_next->m_thinglist_prev = node;
	s->touching_thinglist = node;
	return node;
}


static mprecipsecnode_t *P_AddPrecipSecnode(sector_t *s, precipmobj_t *thing, mprecipsecnode_t *nextnode)
{
	mprecipsecnode_t *node;

	node = nextnode;
	while (node)
	{
		if (node->m_sector == s)  
		{
			node->m_thing = thing;  
			return nextnode;
		}
		node = node->m_sectorlist_next;
	}




	node = P_GetPrecipSecnode();


	node->visited = 0;

	node->m_sector = s;  
	node->m_thing = thing;  
	node->m_sectorlist_prev = NULL;  
	node->m_sectorlist_next = nextnode;  
	if (nextnode)
		nextnode->m_sectorlist_prev = node;  



	node->m_thinglist_prev = NULL;  
	node->m_thinglist_next = s->touching_preciplist;  
	if (s->touching_preciplist)
		node->m_thinglist_next->m_thinglist_prev = node;
	s->touching_preciplist = node;
	return node;
}





static msecnode_t *P_DelSecnode(msecnode_t *node)
{
	msecnode_t *tp;  
	msecnode_t *tn;  
	msecnode_t *sp;  
	msecnode_t *sn;  

	if (!node)
		return NULL;




	tp = node->m_sectorlist_prev;
	tn = node->m_sectorlist_next;
	if (tp)
		tp->m_sectorlist_next = tn;
	if (tn)
		tn->m_sectorlist_prev = tp;




	sp = node->m_thinglist_prev;
	sn = node->m_thinglist_next;
	if (sp)
		sp->m_thinglist_next = sn;
	else
		node->m_sector->touching_thinglist = sn;
	if (sn)
		sn->m_thinglist_prev = sp;



	P_PutSecnode(node);
	return tn;
}


static mprecipsecnode_t *P_DelPrecipSecnode(mprecipsecnode_t *node)
{
	mprecipsecnode_t *tp;  
	mprecipsecnode_t *tn;  
	mprecipsecnode_t *sp;  
	mprecipsecnode_t *sn;  

	if (!node)
		return NULL;




	tp = node->m_sectorlist_prev;
	tn = node->m_sectorlist_next;
	if (tp)
		tp->m_sectorlist_next = tn;
	if (tn)
		tn->m_sectorlist_prev = tp;




	sp = node->m_thinglist_prev;
	sn = node->m_thinglist_next;
	if (sp)
		sp->m_thinglist_next = sn;
	else
		node->m_sector->touching_preciplist = sn;
	if (sn)
		sn->m_thinglist_prev = sp;



	P_PutPrecipSecnode(node);
	return tn;
}


void P_DelSeclist(msecnode_t *node)
{
	while (node)
		node = P_DelSecnode(node);
}


void P_DelPrecipSeclist(mprecipsecnode_t *node)
{
	while (node)
		node = P_DelPrecipSecnode(node);
}







static inline boolean PIT_GetSectors(line_t *ld)
{
	if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT] ||
		tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT] ||
		tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM] ||
		tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
	return true;

	if (P_BoxOnLineSide(tmbbox, ld) != -1)
		return true;

	if (ld->polyobj)  
		return true;








	sector_list = P_AddSecnode(ld->frontsector,tmthing,sector_list);






	if (ld->backsector)
		sector_list = P_AddSecnode(ld->backsector, tmthing, sector_list);

	return true;
}


static inline boolean PIT_GetPrecipSectors(line_t *ld)
{
	if (preciptmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT] ||
		preciptmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT] ||
		preciptmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM] ||
		preciptmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
	return true;

	if (P_BoxOnLineSide(preciptmbbox, ld) != -1)
		return true;

	if (ld->polyobj)  
		return true;








	precipsector_list = P_AddPrecipSecnode(ld->frontsector, tmprecipthing, precipsector_list);






	if (ld->backsector)
		precipsector_list = P_AddPrecipSecnode(ld->backsector, tmprecipthing, precipsector_list);

	return true;
}




void P_CreateSecNodeList(mobj_t *thing, fixed_t x, fixed_t y)
{
	INT32 xl, xh, yl, yh, bx, by;
	msecnode_t *node = sector_list;
	mobj_t *saved_tmthing = tmthing;  
	fixed_t saved_tmx = tmx, saved_tmy = tmy;  






	while (node)
	{
		node->m_thing = NULL;
		node = node->m_sectorlist_next;
	}

	P_SetTarget(&tmthing, thing);
	tmflags = thing->flags;

	tmx = x;
	tmy = y;

	tmbbox[BOXTOP] = y + tmthing->radius;
	tmbbox[BOXBOTTOM] = y - tmthing->radius;
	tmbbox[BOXRIGHT] = x + tmthing->radius;
	tmbbox[BOXLEFT] = x - tmthing->radius;

	validcount++;  

	xl = (unsigned)(tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	for (bx = xl; bx <= xh; bx++)
		for (by = yl; by <= yh; by++)
			P_BlockLinesIterator(bx, by, PIT_GetSectors);


	sector_list = P_AddSecnode(thing->subsector->sector, thing, sector_list);



	node = sector_list;
	while (node)
	{
		if (!node->m_thing)
		{
			if (node == sector_list)
				sector_list = node->m_sectorlist_next;
			node = P_DelSecnode(node);
		}
		else
			node = node->m_sectorlist_next;
	}

	        
                                                                     
                                                        
                                                                       
                                                                  
                                                                  
                                                                       
   
	P_SetTarget(&tmthing, saved_tmthing);

	                                                
                                     
	tmx = saved_tmx, tmy = saved_tmy;

	if (tmthing)
	{
		tmbbox[BOXTOP]  = tmy + tmthing->radius;
		tmbbox[BOXBOTTOM] = tmy - tmthing->radius;
		tmbbox[BOXRIGHT]  = tmx + tmthing->radius;
		tmbbox[BOXLEFT]   = tmx - tmthing->radius;
	}
}


void P_CreatePrecipSecNodeList(precipmobj_t *thing,fixed_t x,fixed_t y)
{
	INT32 xl, xh, yl, yh, bx, by;
	mprecipsecnode_t *node = precipsector_list;
	precipmobj_t *saved_tmthing = tmprecipthing;  






	while (node)
	{
		node->m_thing = NULL;
		node = node->m_sectorlist_next;
	}

	tmprecipthing = thing;

	preciptmbbox[BOXTOP] = y + 2*FRACUNIT;
	preciptmbbox[BOXBOTTOM] = y - 2*FRACUNIT;
	preciptmbbox[BOXRIGHT] = x + 2*FRACUNIT;
	preciptmbbox[BOXLEFT] = x - 2*FRACUNIT;

	validcount++;  

	xl = (unsigned)(preciptmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(preciptmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(preciptmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(preciptmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	for (bx = xl; bx <= xh; bx++)
		for (by = yl; by <= yh; by++)
			P_BlockLinesIterator(bx, by, PIT_GetPrecipSectors);


	precipsector_list = P_AddPrecipSecnode(thing->subsector->sector, thing, precipsector_list);



	node = precipsector_list;
	while (node)
	{
		if (!node->m_thing)
		{
			if (node == precipsector_list)
				precipsector_list = node->m_sectorlist_next;
			node = P_DelPrecipSecnode(node);
		}
		else
			node = node->m_sectorlist_next;
	}

	        
                                                                     
                                                        
                                                                       
                                                                  
                                                                  
                                                                       
   
	tmprecipthing = saved_tmthing;
}

                       
                                                                                                                                                                                                                                            
void P_MapStart(void)
{
	if (tmthing)
	{
		                                                                          
                                                                             
		CONS_Alert(CONS_WARNING, "P_MapStart: cleared stale collision state.\n");
		P_SetTarget(&tmthing, NULL);
	}
}

void P_MapEnd(void)
{
	P_SetTarget(&tmthing, NULL);
}




fixed_t P_FloorzAtPos(fixed_t x, fixed_t y, fixed_t z, fixed_t height)
{
	sector_t *sec = R_PointInSubsector(x, y)->sector;
	fixed_t floorz = P_GetSectorFloorZAt(sec, x, y);


	if (sec->ffloors)
	{
		ffloor_t *rover;
		fixed_t delta1, delta2, thingtop = z + height;

		for (rover = sec->ffloors; rover; rover = rover->next)
		{
			fixed_t topheight, bottomheight;
			if (!(rover->fofflags & FOF_EXISTS))
				continue;

			if ((!(rover->fofflags & FOF_SOLID || rover->fofflags & FOF_QUICKSAND) || (rover->fofflags & FOF_SWIMMABLE)))
				continue;

			topheight    = P_GetFFloorTopZAt   (rover, x, y);
			bottomheight = P_GetFFloorBottomZAt(rover, x, y);

			if (rover->fofflags & FOF_QUICKSAND)
			{
				if (z < topheight && bottomheight < thingtop)
				{
					if (floorz < z)
						floorz = z;
				}
				continue;
			}

			delta1 = z - (bottomheight + ((topheight - bottomheight)/2));
			delta2 = thingtop - (bottomheight + ((topheight - bottomheight)/2));
			if (topheight > floorz && abs(delta1) < abs(delta2))
				floorz = topheight;
		}
	}

	return floorz;
}



fixed_t P_CeilingzAtPos(fixed_t x, fixed_t y, fixed_t z, fixed_t height)
{
	sector_t *sec = R_PointInSubsector(x, y)->sector;
	fixed_t ceilingz = P_GetSectorCeilingZAt(sec, x, y);

	if (sec->ffloors)
	{
		ffloor_t *rover;
		fixed_t delta1, delta2, thingtop = z + height;

		for (rover = sec->ffloors; rover; rover = rover->next)
		{
			fixed_t topheight, bottomheight;
			if (!(rover->fofflags & FOF_EXISTS))
				continue;

			if ((!(rover->fofflags & FOF_SOLID || rover->fofflags & FOF_QUICKSAND) || (rover->fofflags & FOF_SWIMMABLE)))
				continue;

			topheight    = P_GetFFloorTopZAt   (rover, x, y);
			bottomheight = P_GetFFloorBottomZAt(rover, x, y);

			if (rover->fofflags & FOF_QUICKSAND)
			{
				if (thingtop > bottomheight && topheight > z)
				{
					if (ceilingz > thingtop)
						ceilingz = thingtop;
				}
				continue;
			}

			delta1 = z - (bottomheight + ((topheight - bottomheight)/2));
			delta2 = thingtop - (bottomheight + ((topheight - bottomheight)/2));
			if (bottomheight < ceilingz && abs(delta1) > abs(delta2))
				ceilingz = bottomheight;
		}
	}

	return ceilingz;
}

INT32 P_GetSectorLightNumAt(sector_t *sector, fixed_t x, fixed_t y, fixed_t z)
{
	if (!sector->numlights)
		return -1;

	INT32 light = sector->numlights - 1;


	for (INT32 lightnum = 1; lightnum < sector->numlights; lightnum++) {
		fixed_t h = P_GetLightZAt(&sector->lightlist[lightnum], x, y);
		if (h <= z) {
			light = lightnum - 1;
			break;
		}
	}

	return light;
}

INT32 P_GetLightLevelFromSectorAt(sector_t *sector, fixed_t x, fixed_t y, fixed_t z)
{
	if (sector->numlights)
		return *sector->lightlist[P_GetSectorLightNumAt(sector, x, y, z)].lightlevel;
	else
		return sector->lightlevel;
}

INT32 P_GetSectorLightLevelAt(fixed_t x, fixed_t y, fixed_t z)
{
	return P_GetLightLevelFromSectorAt(R_PointInSubsector(x, y)->sector, x, y, z);
}

extracolormap_t *P_GetColormapFromSectorAt(sector_t *sector, fixed_t x, fixed_t y, fixed_t z)
{
	if (sector->numlights)
		return *sector->lightlist[P_GetSectorLightNumAt(sector, x, y, z)].extra_colormap;
	else
		return sector->extra_colormap;
}

extracolormap_t *P_GetSectorColormapAt(fixed_t x, fixed_t y, fixed_t z)
{
	return P_GetColormapFromSectorAt(R_PointInSubsector(x, y)->sector, x, y, z);
}
