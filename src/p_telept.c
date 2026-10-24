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



#include "doomdef.h"
#include "g_game.h"
#include "p_local.h"
#include "r_state.h"
#include "s_sound.h"
#include "r_main.h"

                               

                              
                   
                   
                   
                                  

             


  
void P_MixUp(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z, angle_t angle,
			INT16 starpostx, INT16 starposty, INT16 starpostz,
			INT32 starpostnum, tic_t starposttime, angle_t starpostangle,
			fixed_t starpostscale, angle_t drawangle, INT32 flags2)
{
	const INT32 takeflags2 = MF2_TWOD|MF2_OBJECTFLIP;



	P_UnsetThingPosition(thing);


	if (sector_list)
	{
		P_DelSeclist(sector_list);
		sector_list = NULL;
	}

	thing->x = x;
	thing->y = y;
	thing->z = z;

	if (thing->player)
	{
		if (thing->eflags & MFE_VERTICALFLIP)
			thing->player->viewz = thing->z + thing->height - thing->player->viewheight;
		else
			thing->player->viewz = thing->z + thing->player->viewheight;

		if (!thing->tracer)
			thing->reactiontime = TICRATE/2;  


		P_SetPlayerAngle(thing->player, angle);


		if (splitscreen && camera2.chase
			&& thing->player == &players[secondarydisplayplayer])
		{
			P_ResetCamera(thing->player, &camera2);
		}
		else if (camera.chase && thing->player == &players[displayplayer])
			P_ResetCamera(thing->player, &camera);


		thing->player->cmomx = thing->player->cmomy = 0;
		thing->player->rmomx = thing->player->rmomy = 0;
		if (!thing->tracer)
			thing->player->speed = 0;


		thing->player->starpostx = starpostx;
		thing->player->starposty = starposty;
		thing->player->starpostz = starpostz;
		thing->player->starposttime = starposttime;
		thing->player->starpostangle = starpostangle;
		thing->player->starpostscale = starpostscale;
		thing->player->starpostnum = starpostnum;

		thing->player->drawangle = drawangle;


		P_ResetStarposts();
		P_ClearStarPost(starpostnum);

		P_ResetPlayer(thing->player);
		P_SetMobjState(thing, S_PLAY_STND);

		P_FlashPal(thing->player, PAL_MIXUP, 10);
	}

	thing->angle = angle;

	thing->momx = thing->momy = thing->momz = 0;

	thing->flags2 = (thing->flags2 & ~takeflags2) | (flags2 & takeflags2);
}

                                  

                                
                   
                   
                   
                                  

                                        


  
boolean P_Teleport(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z, angle_t angle, boolean flash, boolean dontstopmove)
{
	if (!P_SetOrigin(thing, x, y, z))
		return false;

	if (!dontstopmove)
		thing->momx = thing->momy = thing->momz = 0;
	else  
		P_InstaThrust(thing, angle, FixedHypot(thing->momx, thing->momy));

	if (thing->player)
	{
		if (thing->eflags & MFE_VERTICALFLIP)
			thing->player->viewz = thing->z + thing->height - thing->player->viewheight;
		else
			thing->player->viewz = thing->z + thing->player->viewheight;


		if (!dontstopmove)
		{
			INT32 p;

			for (p = 0; p < MAXPLAYERS; p++)
				if (playeringame[p] && players[p].mo && players[p].powers[pw_carry] == CR_PLAYER && players[p].mo->tracer == thing)
				{
					players[p].powers[pw_carry] = CR_NONE;
					P_SetTarget(&players[p].mo->tracer, NULL);
					break;
				}
			thing->player->cmomx = thing->player->cmomy = 0;
			thing->player->rmomx = thing->player->rmomy = 0;
			thing->player->speed = 0;
			P_ResetPlayer(thing->player);
			P_SetMobjState(thing, S_PLAY_STND);

			thing->reactiontime = TICRATE/2;  
			thing->player->drawangle = angle;
		}
		else
			thing->player->drawangle += (angle - thing->angle);


		P_SetPlayerAngle(thing->player, angle);


		if (splitscreen && camera2.chase && thing->player == &players[secondarydisplayplayer])
			P_ResetCamera(thing->player, &camera2);
		else if (camera.chase && thing->player == &players[displayplayer])
			P_ResetCamera(thing->player, &camera);

		if (flash)
			P_FlashPal(thing->player, PAL_MIXUP, 10);
	}

	thing->angle = angle;

	return true;
}
