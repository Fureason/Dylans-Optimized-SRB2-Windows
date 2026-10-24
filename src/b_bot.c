// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2007-2016 by John "JTE" Muniz.
// Copyright (C) 2011-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "doomdef.h"
#include "d_player.h"
#include "g_game.h"
#include "r_main.h"
#include "p_local.h"
#include "b_bot.h"
#include "lua_hook.h"
#include "i_system.h"  

void B_UpdateBotleader(player_t *player)
{
	UINT32 i;
	fixed_t dist;
	fixed_t neardist = INT32_MAX;
	player_t *nearplayer = NULL;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (players[i].bot || players[i].playerstate != PST_LIVE || players[i].spectator || !players[i].mo)
			continue;

		if (!player->botleader)
		{
			player->botleader = &players[i];  
			return;
		}

		if (!player->mo)
			return;


		dist = R_PointToDist2(player->mo->x, player->mo->y, players[i].mo->x, players[i].mo->y);
		if (neardist > dist)
		{
			neardist = dist;
			nearplayer = &players[i];
		}
	}

	player->botleader = nearplayer;
}

static inline void B_ResetAI(botmem_t *mem)
{
	mem->thinkstate = AI_FOLLOW;
	mem->catchup_tics = 0;
}

static void B_BuildTailsTiccmd(mobj_t *sonic, mobj_t *tails, ticcmd_t *cmd)
{
	boolean forward=false, backward=false, left=false, right=false, jump=false, spin=false;

	player_t *player = sonic->player, *bot = tails->player;
	ticcmd_t *pcmd = &player->cmd;
	botmem_t *mem = &bot->botmem;
	boolean water = (tails->eflags & MFE_UNDERWATER);
	SINT8 flip = P_MobjFlip(tails);
	boolean _2d = (tails->flags2 & MF2_TWOD) || twodlevel;
	fixed_t scale = tails->scale;
	boolean jump_last = (bot->lastbuttons & BT_JUMP);
	boolean spin_last = (bot->lastbuttons & BT_SPIN);

	fixed_t dist = P_AproxDistance(sonic->x - tails->x, sonic->y - tails->y);
	fixed_t zdist = flip * (sonic->z - tails->z);
	angle_t ang = sonic->angle;
	fixed_t pmom = P_AproxDistance(sonic->momx, sonic->momy);
	fixed_t bmom = P_AproxDistance(tails->momx, tails->momy);
	fixed_t followmax = 128 * 8 * scale;  
	fixed_t followthres = 92 * scale;  
	fixed_t followmin = 32 * scale;
	fixed_t comfortheight = 96 * scale;
	fixed_t touchdist = 24 * scale;
	boolean stalled = (bmom < scale >> 1) && dist > followthres;  
	boolean samepos = (sonic->x == tails->x && sonic->y == tails->y);
	boolean blocked = bot->blocked;

	if (!samepos)
		ang = R_PointToAngle2(tails->x, tails->y, sonic->x, sonic->y);


	if (LUA_HookBotAI(sonic, tails, cmd))
		return;


	if (!sonic || sonic->health <= 0)
	{
		mem->thinkstate = AI_STANDBY;
		return;
	}
	else if (mem->thinkstate == AI_STANDBY)
		mem->thinkstate = AI_FOLLOW;

	if (tails->player->powers[pw_carry] == CR_MACESPIN || tails->player->powers[pw_carry] == CR_GENERIC)
	{
		boolean isrelevant = (sonic->player->powers[pw_carry] == CR_MACESPIN || sonic->player->powers[pw_carry] == CR_GENERIC);
		if (sonic->player->cmd.buttons & BT_JUMP && (sonic->player->pflags & PF_JUMPED) && isrelevant)
			cmd->buttons |= BT_JUMP;
		if (isrelevant)
		{
			cmd->forwardmove = sonic->player->cmd.forwardmove;
			cmd->angleturn = abs((signed)(tails->angle - sonic->angle))>>16;
			if (sonic->angle < tails->angle)
				cmd->angleturn = -cmd->angleturn;
		} else if (dist > FixedMul(512*FRACUNIT, tails->scale))
			cmd->buttons |= BT_JUMP;
		return;
	}




	if (water)
	{
		followmin = 0;
		followthres = 16*scale;
		followmax >>= 1;
		if (mem->thinkstate == AI_THINKFLY)
			mem->thinkstate = AI_FOLLOW;
	}


	if (mem->thinkstate == AI_SPINFOLLOW)
	{
		mem->catchup_tics = 0;
	}
	else if (dist > followmax || zdist > comfortheight || stalled)
	{
		mem->catchup_tics = min(mem->catchup_tics + 2, 70);
		if (mem->catchup_tics >= 70)
			mem->thinkstate = AI_CATCHUP;
	}
	else
	{
		mem->catchup_tics = max(mem->catchup_tics - 1, 0);
		if (mem->thinkstate == AI_CATCHUP)
			mem->thinkstate = AI_FOLLOW;
	}



	if (bot->pflags & (PF_SPINNING|PF_STARTDASH))
	{
		cmd->angleturn = (sonic->angle) >> 16;  
	}
	else if (mem->thinkstate == AI_FLYCARRY)
	{
		cmd->angleturn = sonic->player->cmd.angleturn;
	}
	else
	{
		cmd->angleturn = (ang) >> 16;  
	}




	if (player->exiting && mem->thinkstate == AI_THINKFLY)
		mem->thinkstate = AI_FOLLOW;
	else
	{

		if (mem->thinkstate == AI_THINKFLY && player->pflags & PF_JUMPED)
		{
			if (!jump_last)
			{
				jump = true;
				mem->thinkstate = AI_FLYSTANDBY;
				bot->pflags |= PF_CANCARRY;
			}
		}



		if (!(water || pmom || bmom)
			&& (dist < touchdist && !samepos)
			&& !(pcmd->forwardmove || pcmd->sidemove || player->dashspeed)
			&& P_IsObjectOnGround(sonic) && P_IsObjectOnGround(tails)
			&& !(player->pflags & PF_STASIS)
			&& !(player->exiting)
			&& bot->charability == CA_FLY)
				mem->thinkstate = AI_THINKFLY;
		else if (mem->thinkstate == AI_THINKFLY)
			mem->thinkstate = AI_FOLLOW;


		if (player->powers[pw_carry] == CR_PLAYER && sonic->tracer == tails)
		{
			mem->thinkstate = AI_FLYCARRY;
		}


		if (mem->thinkstate == AI_FLYSTANDBY)
		{
			if (zdist < -64*scale || (flip * tails->momz) > scale)  
				spin = true;
			else if (!jump_last)
				jump = true;


			if (dist > followthres || player->dashspeed)
				mem->thinkstate = AI_FOLLOW;
		}

		else if (mem->thinkstate == AI_FLYCARRY)
		{
			cmd->forwardmove = pcmd->forwardmove;
			cmd->sidemove = pcmd->sidemove;
			if (pcmd->buttons & BT_SPIN)
			{
				spin = true;
				jump = false;
			}
			else if (!jump_last)
				jump = true;

			if (player->powers[pw_carry] != CR_PLAYER)
			{
				mem->thinkstate = AI_FOLLOW;
			}
		}
	}

	if (P_IsObjectOnGround(tails) && !(pcmd->buttons & BT_JUMP) && (mem->thinkstate == AI_FLYSTANDBY || mem->thinkstate == AI_FLYCARRY))
		mem->thinkstate = AI_FOLLOW;



	if (!(player->pflags & (PF_SPINNING|PF_STARTDASH)) && mem->thinkstate == AI_SPINFOLLOW)
		mem->thinkstate = AI_FOLLOW;
	else if ((mem->thinkstate == AI_FOLLOW || mem->thinkstate == AI_SPINFOLLOW)
		&& bot->charability2 == CA2_SPINDASH)
	{
		if (!_2d)
		{

			if (player->dashspeed)
			{
				if (dist < followthres && dist > touchdist)  
				{
					cmd->angleturn = (ang) >> 16;  
					cmd->forwardmove = 50;
					mem->thinkstate = AI_SPINFOLLOW;
				}
				else if (dist < touchdist)
				{
					if (!bmom && (!(bot->pflags & PF_SPINNING) || (bot->dashspeed && bot->pflags & PF_SPINNING)))
					{
						cmd->angleturn = (sonic->angle) >> 16;  
						spin = true;
					}
					mem->thinkstate = AI_SPINFOLLOW;
				}
				else
					mem->thinkstate = AI_FOLLOW;
			}

			else if (player->dashspeed == bot->dashspeed && player->pflags & PF_SPINNING)
			{
				if (bot->pflags & PF_SPINNING || !spin_last)
				{
					spin = true;
					cmd->angleturn = (sonic->angle) >> 16;  
					cmd->forwardmove = MAXPLMOVE;
					mem->thinkstate = AI_SPINFOLLOW;
				}
				else
					mem->thinkstate = AI_FOLLOW;
			}
		}

		else
		{
			if (((player->dashspeed && !bmom) || (player->dashspeed == bot->dashspeed && (player->pflags & PF_SPINNING)))
				&& ((bot->pflags & PF_SPINNING) || !spin_last))
			{
				spin = true;
				mem->thinkstate = AI_SPINFOLLOW;
			}
			else
				mem->thinkstate = AI_FOLLOW;
		}
	}



	if (mem->thinkstate == AI_FOLLOW || mem->thinkstate == AI_CATCHUP)
	{

		if (mem->thinkstate == AI_CATCHUP || dist > followthres)
		{
			if (!_2d)
				cmd->forwardmove = MAXPLMOVE;
			else if (sonic->x > tails->x)
				cmd->sidemove = MAXPLMOVE;
			else
				cmd->sidemove = -MAXPLMOVE;
		}

		else if (dist > followmin && abs(zdist) < 192*scale)
		{
			if (!_2d)
				cmd->forwardmove = FixedHypot(pcmd->forwardmove, pcmd->sidemove);
			else
				cmd->sidemove = pcmd->sidemove;
		}

		else if (dist < followmin)
		{

			cmd->angleturn = (sonic->angle) >> 16;  
			cmd->forwardmove = 8 * pcmd->forwardmove / 10;
			cmd->sidemove = 8 * pcmd->sidemove / 10;
		}
	}



	if (mem->thinkstate == AI_FOLLOW || mem->thinkstate == AI_CATCHUP || (mem->thinkstate == AI_SPINFOLLOW && player->pflags & PF_JUMPED))
	{

		if (bot->charability == CA_FLY && bot->pflags & PF_THOKKED)
		{
			cmd->forwardmove = min(MAXPLMOVE, (dist/scale)>>3);
			if (zdist < -64*scale)
				spin = true;
			else if (zdist > 0 && !jump_last)
				jump = true;
		}


		if (tails->eflags & MFE_JUSTHITFLOOR)
			jump = false;

		else if (!jump_last && !(bot->pflags & PF_JUMPED)  
			&& ((zdist > 32*scale && player->pflags & PF_JUMPED)  
				|| (zdist > 64*scale && mem->thinkstate == AI_CATCHUP)  
				|| (stalled && mem->catchup_tics > 20 && bot->powers[pw_carry] == CR_NONE)

				|| (bot->pflags & PF_SPINNING && !(bot->pflags & PF_JUMPED))))  
					jump = true;

		else if (bot->pflags & PF_JUMPED && jump_last && tails->momz*flip > 0 && (zdist > 0 || mem->thinkstate == AI_CATCHUP))
			jump = true;

		else if (bot->pflags & PF_JUMPED && mem->thinkstate == AI_CATCHUP && !jump_last && bot->charability == CA_FLY)
			jump = true;
	}







	B_KeysToTiccmd(tails, cmd, forward, backward, left, right, false, false, jump, spin);


	mem->lastForward = forward;
	mem->lastBlocked = blocked;
}

void B_BuildTiccmd(player_t *player, ticcmd_t *cmd)
{
	G_CopyTiccmd(cmd, I_BaseTiccmd(), 1);  


	if (!player->mo)
		return;

	if (player->playerstate == PST_DEAD)
	{
		if (B_CheckRespawn(player))
			cmd->buttons |= BT_JUMP;
		return;
	}


	if (!dedicated)
		CV_SetValue(&cv_analog[1], false);


	if (LUA_HookTiccmd(player, cmd, HOOK(BotTiccmd)))
		return;


	B_UpdateBotleader(player);
	if (!player->botleader)
		return;



	B_BuildTailsTiccmd(player->botleader->mo, player->mo, cmd);
}

void B_KeysToTiccmd(mobj_t *mo, ticcmd_t *cmd, boolean forward, boolean backward, boolean left, boolean right, boolean strafeleft, boolean straferight, boolean jump, boolean spin)
{
	player_t *player = mo->player;

	if (player->botleader && player->botleader->powers[pw_carry] && player->botleader->powers[pw_carry] != CR_PLAYER)
		return;

	if (twodlevel || mo->flags2 & MF2_TWOD) {
		if (player->botleader->climbing
		|| mo->player->pflags & PF_GLIDING) {


			if (forward)
				cmd->forwardmove += MAXPLMOVE<<FRACBITS>>16;
			if (backward)
				cmd->forwardmove -= MAXPLMOVE<<FRACBITS>>16;
			if (left || strafeleft)
				cmd->sidemove -= MAXPLMOVE<<FRACBITS>>16;
			if (right || straferight)
				cmd->sidemove += MAXPLMOVE<<FRACBITS>>16;
		} else {

			if (left || right)
				backward = true;
			left = right = false;
			if (forward) {
				if (mo->angle < ANGLE_90 || mo->angle > ANGLE_270)
					right = true;
				else
					left = true;
			} else if (backward) {
				if (mo->angle < ANGLE_90 || mo->angle > ANGLE_270)
					left = true;
				else
					right = true;
			}
			if (left || strafeleft)
				cmd->sidemove -= MAXPLMOVE<<FRACBITS>>16;
			if (right || straferight)
				cmd->sidemove += MAXPLMOVE<<FRACBITS>>16;
		}
	} else {
		angle_t angle;
		if (forward)
			cmd->forwardmove += MAXPLMOVE<<FRACBITS>>16;
		if (backward)
			cmd->forwardmove -= MAXPLMOVE<<FRACBITS>>16;
		if (left)
			cmd->angleturn += 1280;
		if (right)
			cmd->angleturn -= 1280;
		if (strafeleft)
			cmd->sidemove -= MAXPLMOVE<<FRACBITS>>16;
		if (straferight)
			cmd->sidemove += MAXPLMOVE<<FRACBITS>>16;


		angle = R_PointToAngle2(0, 0, cmd->sidemove << FRACBITS, cmd->forwardmove << FRACBITS);
		{
			INT32 maxforward = abs(P_ReturnThrustY(NULL, angle, MAXPLMOVE));
			INT32 maxside = abs(P_ReturnThrustX(NULL, angle, MAXPLMOVE));
			cmd->forwardmove = max(min(cmd->forwardmove, maxforward), -maxforward);
			cmd->sidemove = max(min(cmd->sidemove, maxside), -maxside);
		}
	}
	if (jump)
		cmd->buttons |= BT_JUMP;
	if (spin)
		cmd->buttons |= BT_SPIN;
}

void B_MoveBlocked(player_t *player)
{
	(void)player;
	player->blocked = true;
}

boolean B_CheckRespawn(player_t *player)
{
	mobj_t *sonic;
	mobj_t *tails = player->mo;


	if (!player->botleader)
		return false;

	sonic = player->botleader->mo;

	if (!sonic || sonic->health <= 0)
		return false;


	{
		UINT8 shouldForce = LUA_Hook2Mobj(sonic, tails, MOBJ_HOOK(BotRespawn));

		if (P_MobjWasRemoved(sonic) || P_MobjWasRemoved(tails))
			return (shouldForce == 1);  

		if (shouldForce == 1)
			return true;
		else if (shouldForce == 2)
			return false;
	}



	if (sonic->player->pflags & (PF_GLIDING|PF_SLIDING|PF_BOUNCING)
	|| (sonic->player->panim != PA_IDLE && sonic->player->panim != PA_WALK)
	|| (sonic->player->powers[pw_carry] && sonic->player->powers[pw_carry] != CR_PLAYER))
		return false;


	if (sonic->eflags & MFE_VERTICALFLIP)
	{
		if (sonic->z - sonic->floorz < (sonic->player->exiting ? 5 : 2)*sonic->height)
			return false;
	}
	else if (sonic->ceilingz - sonic->z < (sonic->player->exiting ? 6 : 3)*sonic->height)
		return false;


	if (player->playerstate == PST_DEAD) {
		if (player->deadtimer > 4*TICRATE)
			return true;
		return false;
	}


	if (!P_CheckSight(sonic, tails) && P_AproxDistance(P_AproxDistance(tails->x-sonic->x, tails->y-sonic->y), tails->z-sonic->z) > FixedMul(1024*FRACUNIT, tails->scale))
		return true;
	return false;
}

void B_RespawnBot(INT32 playernum)
{
	player_t *player = &players[playernum];
	fixed_t x,y,z;
	mobj_t *sonic;
	mobj_t *tails;

	if (!player->botleader)
		return;

	sonic = player->botleader->mo;
	if (!sonic || sonic->health <= 0)
		return;

	B_ResetAI(&player->botmem);

	player->bot = BOT_2PAI;
	P_SpawnPlayer(playernum);
	tails = player->mo;

	x = sonic->x;
	y = sonic->y;
	if (sonic->eflags & MFE_VERTICALFLIP) {
		tails->eflags |= MFE_VERTICALFLIP;
		z = sonic->z - (512*sonic->scale);
		if (z < sonic->floorz)
			z = sonic->floorz;
	} else {
		z = sonic->z + sonic->height + (512*sonic->scale);
		if (z > sonic->ceilingz - sonic->height)
			z = sonic->ceilingz - sonic->height;
	}

	if (sonic->flags2 & MF2_OBJECTFLIP)
		tails->flags2 |= MF2_OBJECTFLIP;
	if (sonic->flags2 & MF2_TWOD)
		tails->flags2 |= MF2_TWOD;
	if (sonic->eflags & MFE_UNDERWATER)
		tails->eflags |= MFE_UNDERWATER;
	player->powers[pw_underwater] = sonic->player->powers[pw_underwater];
	player->powers[pw_spacetime] = sonic->player->powers[pw_spacetime];
	player->powers[pw_gravityboots] = sonic->player->powers[pw_gravityboots];
	player->powers[pw_nocontrol] = sonic->player->powers[pw_nocontrol];
	player->pflags |= PF_AUTOBRAKE|(sonic->player->pflags & PF_DIRECTIONCHAR);

	P_SetOrigin(tails, x, y, z);
	if (player->charability == CA_FLY)
	{
		P_SetMobjState(tails, S_PLAY_FLY);
		tails->player->powers[pw_tailsfly] = (UINT16)-1;
	}
	else
		P_SetMobjState(tails, S_PLAY_FALL);
	P_SetScale(tails, sonic->scale, false);
	tails->destscale = sonic->destscale;
	tails->old_scale = sonic->old_scale;
}

void B_HandleFlightIndicator(player_t *player)
{
	mobj_t *tails = player->mo;
	botmem_t *mem = &player->botmem;
	boolean shouldExist;

	if (!tails)
		return;

	shouldExist = (mem->thinkstate == AI_THINKFLY) && player->botleader
		&& player->bot == BOT_2PAI && player->playerstate == PST_LIVE;


	if (P_MobjWasRemoved(tails->hnext))
	{

		if (!shouldExist)
			return;


		P_SetTarget(&tails->hnext, P_SpawnMobjFromMobj(tails, 0, 0, 0, MT_OVERLAY));
		if (P_MobjWasRemoved(tails->hnext))
			return;   

		P_SetTarget(&tails->hnext->target, tails);
		P_SetTarget(&tails->hnext->hprev, tails);
		P_SetMobjState(tails->hnext, S_FLIGHTINDICATOR);
	}


	if (tails->hnext->type != MT_OVERLAY || (tails->hnext->state != states+S_FLIGHTINDICATOR))
		return;


	if (!shouldExist)
	{
		P_RemoveMobj(tails->hnext);
		P_SetTarget(&tails->hnext, NULL);
		return;
	}


	tails->hnext->drawonlyforplayer = player->botleader;  
	if (P_IsLocalPlayer(player->botleader))  
		tails->hnext->flags2 &= ~MF2_DONTDRAW;
	else
		tails->hnext->flags2 |= MF2_DONTDRAW;
}
