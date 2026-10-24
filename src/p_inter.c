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
#include "am_map.h"
#include "g_game.h"
#include "m_random.h"
#include "p_local.h"
#include "s_sound.h"
#include "r_main.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "lua_hook.h"
#include "m_cond.h"  
#include "p_setup.h"
#include "m_cheat.h"  
#include "m_misc.h"
#include "v_video.h"  
#include "f_finale.h"
#include "netcode/net_command.h"


#define CTFTEAMCODE(pl) pl->ctfteam ? (pl->ctfteam == 1 ? "\x85" : "\x84") : ""
#define CTFTEAMENDCODE(pl) pl->ctfteam ? "\x80" : ""

void P_ForceFeed(const player_t *player, INT32 attack, INT32 fade, tic_t duration, INT32 period)
{
	BasicFF_t Basicfeed;
	if (!player)
		return;
	Basicfeed.Duration = (UINT32)(duration * (100L/TICRATE));
	Basicfeed.ForceX = Basicfeed.ForceY = 1;
	Basicfeed.Gain = 25000;
	Basicfeed.Magnitude = period*10;
	Basicfeed.player = player;

	P_RampConstant(&Basicfeed, attack, fade);
}

void P_ForceConstant(const BasicFF_t *FFInfo)
{
	JoyFF_t ConstantQuake;
	if (!FFInfo || !FFInfo->player)
		return;
	ConstantQuake.ForceX    = FFInfo->ForceX;
	ConstantQuake.ForceY    = FFInfo->ForceY;
	ConstantQuake.Duration  = FFInfo->Duration;
	ConstantQuake.Gain      = FFInfo->Gain;
	ConstantQuake.Magnitude = FFInfo->Magnitude;
	if (FFInfo->player == &players[consoleplayer])
		I_Tactile(ConstantForce, &ConstantQuake);
	else if (splitscreen && FFInfo->player == &players[secondarydisplayplayer])
		I_Tactile2(ConstantForce, &ConstantQuake);
}
void P_RampConstant(const BasicFF_t *FFInfo, INT32 Start, INT32 End)
{
	JoyFF_t RampQuake;
	if (!FFInfo || !FFInfo->player)
		return;
	RampQuake.ForceX    = FFInfo->ForceX;
	RampQuake.ForceY    = FFInfo->ForceY;
	RampQuake.Duration  = FFInfo->Duration;
	RampQuake.Gain      = FFInfo->Gain;
	RampQuake.Magnitude = FFInfo->Magnitude;
	RampQuake.Start     = Start;
	RampQuake.End       = End;
	if (FFInfo->player == &players[consoleplayer])
		I_Tactile(ConstantForce, &RampQuake);
	else if (splitscreen && FFInfo->player == &players[secondarydisplayplayer])
		I_Tactile2(ConstantForce, &RampQuake);
}






                                                  
                                                                              
                                                                              
                                                                          
   
                                                           
    
void P_ClearStarPost(INT32 postnum)
{
	thinker_t *th;
	mobj_t *mo2;


	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type != MT_STARPOST)
			continue;

		if (mo2->health > postnum)
			continue;

		P_SetMobjState(mo2, mo2->info->seestate);
	}
	return;
}






void P_ResetStarposts(void)
{

	thinker_t *th;
	mobj_t *post;

	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		post = (mobj_t *)th;

		if (post->type != MT_STARPOST)
			continue;

		P_SetMobjState(post, post->info->spawnstate);
	}
}






boolean P_CanPickupItem(player_t *player, boolean weapon)
{
	if (!player->mo || player->mo->health <= 0)
		return false;

	if (player->bot && player->bot != BOT_MPAI)
	{
		if (weapon)
			return false;
		return P_CanPickupItem(&players[consoleplayer], true);  
	}

	if (player->powers[pw_flashing] > (flashingtics/4)*3 && player->powers[pw_flashing] < UINT16_MAX)
		return false;

	return true;
}

boolean P_CanPickupEmblem(player_t *player, INT32 emblemID)
{
	emblem_t *emblem = NULL;

	if (emblemID < 0 || emblemID >= numemblems)
	{

		return false;
	}

	emblem = &emblemlocations[emblemID];

	if (demoplayback)
	{

		return false;
	}

	if (player->bot && player->bot != BOT_MPAI)
	{

		return false;
	}

	if (emblem->type == ET_SKIN)
	{
		INT32 skinnum = M_EmblemSkinNum(emblem);

		if (player->skin != skinnum)
		{

			return false;
		}
	}

	return true;
}

boolean P_EmblemWasCollected(INT32 emblemID)
{
	if (emblemID < 0 || emblemID >= numemblems)
	{

		return true;
	}

	if (shareEmblems && !serverGamedata->collected[emblemID])
	{


		return false;
	}

	return clientGamedata->collected[emblemID];
}







void P_DoNightsScore(player_t *player)
{
	mobj_t *dummymo;

	if (player->exiting)
		return;  

	dummymo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+player->mo->height/2, MT_NIGHTSCORE);
	if (player->bot && player->bot != BOT_MPAI)
		player = &players[consoleplayer];

	if (G_IsSpecialStage(gamemap))  
	{
		INT32 i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
			{
				if (++players[i].linkcount > players[i].maxlink)
					players[i].maxlink = players[i].linkcount;
				players[i].linktimer = nightslinktics;
			}
	}
	else  
	{
		if (++player->linkcount > player->maxlink)
			player->maxlink = player->linkcount;
		player->linktimer = nightslinktics;
	}

	if (P_MobjWasRemoved(dummymo))
		return;


	P_AddPlayerScore(player, min(player->linkcount,10)*(player->bonustime ? 20 : 10));
	P_SetMobjState(dummymo, (player->bonustime ? dummymo->info->xdeathstate : dummymo->info->spawnstate) + min(player->linkcount,10)-1);


	dummymo->momz = FRACUNIT;
	dummymo->fuse = 3*TICRATE;
	dummymo->scalespeed = FRACUNIT/25;
	dummymo->destscale = 2*FRACUNIT;


	dummymo->extravalue1 = player->linkcount-1;
	dummymo->extravalue2 = ((player->linkcount-1 >= 300) ? (player->linkcount-1 >= 600) ? 2 : 1 : 0);
}






void P_DoMatchSuper(player_t *player)
{
	UINT16 match_emeralds = player->powers[pw_emeralds];
	boolean doteams = false;
	int i;


	if (G_GametypeHasTeams())
	{
		doteams = true;
		for (i = 0; i < MAXPLAYERS; i++)
			if (players[i].ctfteam == player->ctfteam)
				match_emeralds |= players[i].powers[pw_emeralds];
	}

	if (!ALL7EMERALDS(match_emeralds))
		return;


	emeraldspawndelay = invulntics + 1;
	player->powers[pw_emeralds] = 0;
	player->powers[pw_invulnerability] = emeraldspawndelay;
	player->powers[pw_sneakers] = emeraldspawndelay;
	if (P_IsLocalPlayer(player) && !player->powers[pw_super])
	{
		S_StopMusic();
		if (mariomode)
			G_GhostAddColor(GHC_INVINCIBLE);
		strlcpy(S_sfx[sfx_None].caption, "Invincibility", 14);
		S_StartCaption(sfx_None, -1, player->powers[pw_invulnerability]);
		S_ChangeMusicInternal((mariomode) ? "_minv" : "_inv", false);
	}


	P_StealPlayerScore(player, 50);



	if (doteams)
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].ctfteam == player->ctfteam
			&& players[i].powers[pw_emeralds] != 0)
			{
				players[i].powers[pw_emeralds] = 0;
				player->powers[pw_invulnerability] = invulntics + 1;
				player->powers[pw_sneakers] = player->powers[pw_invulnerability];
				if (P_IsLocalPlayer(player) && !player->powers[pw_super])
				{
					S_StopMusic();
					if (mariomode)
						G_GhostAddColor(GHC_INVINCIBLE);
					strlcpy(S_sfx[sfx_None].caption, "Invincibility", 14);
					S_StartCaption(sfx_None, -1, player->powers[pw_invulnerability]);
					S_ChangeMusicInternal((mariomode) ? "_minv" : "_inv", false);
				}
			}
}

                                                                   
                                                                         
                                                
   
                                                                             
         
   
                                         
                                         
                                                                            
                                             
    
void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher, boolean heightcheck)
{
	player_t *player;
	INT32 i;
	UINT8 elementalpierce;

	if (objectplacing)
		return;

	I_Assert(special != NULL);
	I_Assert(toucher != NULL);



	if (toucher->health <= 0)
		return;
	if (special->health <= 0)
		return;

	if (heightcheck)
	{
		if (special->type == MT_FLINGEMERALD)  
		{  
			if (toucher->z > (special->z + special->height))
				return;
			if (special->z - special->height > (toucher->z + toucher->height))
				return;
		}
		else
		{
			if (toucher->momz < 0) {
				if (toucher->z + toucher->momz > special->z + special->height)
					return;
			} else if (toucher->z > special->z + special->height)
				return;
			if (toucher->momz > 0) {
				if (toucher->z + toucher->height + toucher->momz < special->z)
					return;
			} else if (toucher->z + toucher->height < special->z)
				return;
		}
	}


	if (special->flags & (MF_ENEMY | MF_BOSS) && special->flags2 & MF2_FRET)
		return;

	player = toucher->player;

	if (player)
	{
		if (player->spectator)
			return;


		if (LUA_HookTouchSpecial(special, toucher) || P_MobjWasRemoved(special))
			return;
	}

	if (player || (toucher->flags & MF_PUSHABLE))  
	{
		if (special->type == MT_STEAM)
		{
			if (player && player->mo->state == &states[player->mo->info->painstate])  
				return;

			fixed_t speed = special->info->mass;  
			SINT8 flipval = P_MobjFlip(special);  

			if (special->state != &states[S_STEAM1])  
				return;

			toucher->eflags |= MFE_SPRUNG;
			toucher->momz = flipval * FixedMul(speed, FixedSqrt(FixedMul(special->scale, toucher->scale)));  

			if (player)
			{
				P_ResetPlayer(player);
				if (player->panim != PA_FALL)
					P_SetMobjState(toucher, S_PLAY_FALL);
			}

			return;  
		}
	}

	if (!player)  
		return;


	elementalpierce = (((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL || (player->powers[pw_shield] & SH_NOSTACK) == SH_BUBBLEWRAP) && (player->pflags & PF_SHIELDABILITY)
	? (((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL) ? 1 : 2)
	: 0);

	if ((special->flags & (MF_ENEMY|MF_BOSS)) && !(special->flags & MF_MISSILE))
	{




		switch (special->type)
		{
			case MT_BLACKEGGMAN:
			{
				P_DamageMobj(toucher, special, special, 1, 0);  
				return;
			}
			 case MT_BIGMINE:
			{
				special->momx = toucher->momx/3;
				special->momy = toucher->momy/3;
				special->momz = toucher->momz/3;
				toucher->momx /= -8;
				toucher->momy /= -8;
				toucher->momz /= -8;
				special->flags &= ~MF_SPECIAL;
				if (special->info->activesound)
					S_StartSound(special, special->info->activesound);
				P_SetTarget(&special->tracer, toucher);
				player->homing = 0;
				return;
			}
			case MT_GSNAPPER:
				if (!elementalpierce
				&& toucher->z < special->z + special->height
				&& toucher->z + toucher->height > special->z
				&& P_DamageMobj(toucher, special, special, 1, DMG_SPIKE))
					return;  
				break;

			case MT_SPINCUSHION:
				if (P_MobjFlip(toucher)*(toucher->z - (special->z + special->height/2)) > 0)
				{
					if (player->pflags & PF_BOUNCING)
					{
						toucher->momz = -toucher->momz;
						P_DoAbilityBounce(player, false);
						return;
					}
					else if (P_DamageMobj(toucher, special, special, 1, DMG_SPIKE))
						return;  
				}
				break;
			case MT_FANG:
				if (!player->powers[pw_flashing]
				&& !(player->charability == CA_TWINSPIN && player->panim == PA_ABILITY)
				&& !(player->charability2 == CA2_MELEE && player->panim == PA_ABILITY2))
				{
					if ((special->state == &states[S_FANG_BOUNCE3]
					  || special->state == &states[S_FANG_BOUNCE4]
					  || special->state == &states[S_FANG_PINCHBOUNCE3]
					  || special->state == &states[S_FANG_PINCHBOUNCE4])
					&& P_MobjFlip(special)*((special->z + special->height/2) - (toucher->z + toucher->height/2)) > (toucher->height/2))
					{
						P_DamageMobj(toucher, special, special, 1, 0);
						P_SetTarget(&special->tracer, toucher);

						if (special->state == &states[S_FANG_PINCHBOUNCE3]
						 || special->state == &states[S_FANG_PINCHBOUNCE4])
							P_SetMobjState(special, S_FANG_PINCHPATHINGSTART2);
						else
						{
							var1 = var2 = 4;
							A_Boss5ExtraRepeat(special);
							P_SetMobjState(special, S_FANG_PATHINGCONT2);  
						}
						if (special->eflags & MFE_VERTICALFLIP)
							special->z = toucher->z - special->height;
						else
							special->z = toucher->z + toucher->height;
						return;
					}
				}
				break;
			case MT_PYREFLY:
				if (special->extravalue2 == 2 && P_DamageMobj(player->mo, special, special, 1, DMG_FIRE))
					return;
			default:
				break;
		}

		if (P_PlayerCanDamage(player, special))  
		{
			if (special->type == MT_PTERABYTE && special->target == player->mo && special->extravalue1 == 1)
				return;  

			if ((P_MobjFlip(toucher)*toucher->momz < 0) && (elementalpierce != 1) && (!(player->powers[pw_strong] & STR_HEAVY)))
			{
				fixed_t setmomz = -toucher->momz;  

				if (elementalpierce == 2)  
					P_DoBubbleBounce(player);
				toucher->momz = setmomz;
				if (elementalpierce == 2)  
				{
					boolean underwater = toucher->eflags & MFE_UNDERWATER;

					if (underwater)
						toucher->momz /= 2;
					toucher->momz -= (toucher->momz/(underwater ? 8 : 4));  
				}
			}
			if (player->pflags & PF_BOUNCING)
				P_DoAbilityBounce(player, false);
			if (special->info->spawnhealth > 1)  
			{
				toucher->momx = -toucher->momx;
				toucher->momy = -toucher->momy;
				if (player->charability == CA_FLY && player->panim == PA_ABILITY)
					toucher->momz = -toucher->momz/2;
				else if (player->pflags & PF_GLIDING && !P_IsObjectOnGround(toucher))
				{
					player->pflags &= ~(PF_GLIDING|PF_JUMPED|PF_NOJUMPDAMAGE);
					P_SetMobjState(toucher, S_PLAY_FALL);
					toucher->momz += P_MobjFlip(toucher) * (player->speed >> 3);
					toucher->momx = 7*toucher->momx>>3;
					toucher->momy = 7*toucher->momy>>3;
				}
				else if ((player->powers[pw_strong] & STR_DASH) && player->panim == PA_DASH)
					P_DoPlayerPain(player, special, special);
			}
			P_DamageMobj(special, toucher, toucher, 1, 0);
			if (player->charability == CA_TWINSPIN && player->panim == PA_ABILITY)
				P_TwinSpinRejuvenate(player, player->thokitem);
		}
		else
		{
			if (special->type == MT_PTERABYTE && special->target == player->mo)
				return;  

			P_DamageMobj(toucher, special, special, 1, 0);
		}

		return;
	}
	else if (special->flags & MF_FIRE)
	{
		P_DamageMobj(toucher, special, special, 1, DMG_FIRE);
		return;
	}
	else
	{

	switch (special->type)
	{



		case MT_REDTEAMRING:
			if (player->ctfteam != 1)
				return;
			 
		case MT_BLUETEAMRING:  
			if (special->type == MT_BLUETEAMRING && player->ctfteam != 2)
				return;
			 
		case MT_RING:
		case MT_FLINGRING:
		case MT_COIN:
		case MT_FLINGCOIN:
		case MT_NIGHTSSTAR:
			if (!(P_CanPickupItem(player, false)) && !(special->flags2 & MF2_NIGHTSPULL))
				return;

			special->momx = special->momy = special->momz = 0;
			P_GivePlayerRings(player, 1);

			if ((maptol & TOL_NIGHTS) && special->type != MT_FLINGRING && special->type != MT_FLINGCOIN)
				P_DoNightsScore(player);
			break;
		case MT_BLUESPHERE:
		case MT_FLINGBLUESPHERE:
		case MT_NIGHTSCHIP:
		case MT_FLINGNIGHTSCHIP:
			if (!(P_CanPickupItem(player, false)) && !(special->flags2 & MF2_NIGHTSPULL))
				return;

			special->momx = special->momy = special->momz = 0;
			P_GivePlayerSpheres(player, 1);

			if (special->type == MT_BLUESPHERE || special->type == MT_FLINGBLUESPHERE)
			{
				special->destscale = ((player->powers[pw_carry] == CR_NIGHTSMODE) ? 4 : 2)*special->scale;
				if (states[special->info->deathstate].tics > 0)
					special->scalespeed = FixedDiv(FixedDiv(special->destscale, special->scale), states[special->info->deathstate].tics<<FRACBITS);
				else
					special->scalespeed = 4*FRACUNIT/5;
			}

			if (maptol & TOL_NIGHTS)
				P_DoNightsScore(player);
			break;
		case MT_BOMBSPHERE:
			if (!(P_CanPickupItem(player, false)) && !(special->flags2 & MF2_NIGHTSPULL))
				return;

			special->momx = special->momy = special->momz = 0;
			P_DamageMobj(toucher, special, special, 1, 0);
			break;
		case MT_AUTOPICKUP:
		case MT_BOUNCEPICKUP:
		case MT_SCATTERPICKUP:
		case MT_GRENADEPICKUP:
		case MT_EXPLODEPICKUP:
		case MT_RAILPICKUP:
			if (!(P_CanPickupItem(player, true)))
				return;


			if (special->info->mass >= pw_infinityring && special->info->mass <= pw_railring)
			{
				INT32 pindex = special->info->mass - (INT32)pw_infinityring;

				player->powers[special->info->mass] += (UINT16)special->reactiontime;
				player->ringweapons |= 1 << (pindex-1);

				if (player->powers[special->info->mass] > rw_maximums[pindex])
					player->powers[special->info->mass] = rw_maximums[pindex];
			}
			break;


		case MT_INFINITYRING:
		case MT_AUTOMATICRING:
		case MT_BOUNCERING:
		case MT_SCATTERRING:
		case MT_GRENADERING:
		case MT_EXPLOSIONRING:
		case MT_RAILRING:
			if (!(P_CanPickupItem(player, true)))
				return;

			if (special->info->mass >= pw_infinityring && special->info->mass <= pw_railring)
			{
				INT32 pindex = special->info->mass - (INT32)pw_infinityring;

				player->powers[special->info->mass] += (UINT16)special->health;
				if (player->powers[special->info->mass] > rw_maximums[pindex])
					player->powers[special->info->mass] = rw_maximums[pindex];
			}
			break;





		case MT_TOKEN:
			if (player->bot && player->bot != BOT_MPAI)
				return;

			P_AddPlayerScore(player, 1000);

			if (!(gametyperules & GTR_SPECIALSTAGES) || modeattacking)  
			{
				S_StartSound(toucher, sfx_chchng);
				break;
			}

			tokenlist += special->health;

			if (ALL7EMERALDS(emeralds))  
			{
				if (continuesInSession)
				{
					player->continues += 1;
					player->gotcontinue = true;
					if (P_IsLocalPlayer(player))
						S_StartSound(NULL, sfx_s3kac);
					else
						S_StartSound(toucher, sfx_chchng);
				}
				else
				{
					P_GiveCoopLives(player, 1, true);  
					S_StartSound(toucher, sfx_chchng);
				}
			}
			else
			{
				token++;
				S_StartSound(toucher, sfx_token);
			}

			break;


		case MT_EMERHUNT:
			if (player->bot && player->bot != BOT_MPAI)
				return;

			if (hunt1 == special)
				hunt1 = NULL;
			else if (hunt2 == special)
				hunt2 = NULL;
			else if (hunt3 == special)
				hunt3 = NULL;

			if (!hunt1 && !hunt2 && !hunt3)
			{
				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (!playeringame[i] || players[i].spectator)
						continue;

					players[i].exiting = (14*TICRATE)/5 + 1;
				}

			}
			break;


		case MT_EMERALD1:
		case MT_EMERALD2:
		case MT_EMERALD3:
		case MT_EMERALD4:
		case MT_EMERALD5:
		case MT_EMERALD6:
		case MT_EMERALD7:
			if (player->bot && player->bot != BOT_MPAI)
				return;

			if (special->threshold)
			{
				player->powers[pw_emeralds] |= special->info->speed;
				P_DoMatchSuper(player);
			}
			else
			{
				emeralds |= special->info->speed;
				stagefailed = false;
			}

			if (special->target && special->target->type == MT_EMERALDSPAWN)
			{
				if (special->target->target)
					P_SetTarget(&special->target->target, NULL);

				special->target->threshold = 0;

				P_SetTarget(&special->target, NULL);
			}
			break;


		case MT_FLINGEMERALD:
			if (!(P_CanPickupItem(player, true)) || player->tossdelay)
				return;

			player->powers[pw_emeralds] |= special->threshold;
			P_DoMatchSuper(player);
			break;


		case MT_EMBLEM:
			{
				const boolean toucherIsServer = ((player - players) == serverplayer);
				const boolean consoleIsServer = (consoleplayer == serverplayer);
				boolean prevCollected = false;

				if ((special->flags2 & MF2_NIGHTSPULL)
					&& (toucher == special->tracer))
				{


					P_SetTarget(&special->tracer, NULL);
					special->flags2 &= ~MF2_NIGHTSPULL;
					special->movefactor = 0;
					special->momx = special->momy = special->momz = 0;
				}

				if (!P_CanPickupEmblem(player, special->health - 1))
				{
					return;
				}

				prevCollected = P_EmblemWasCollected(special->health - 1);

				if (toucherIsServer || shareEmblems)
				{
					serverGamedata->collected[special->health-1] = true;
					M_SilentUpdateUnlockablesAndEmblems(serverGamedata);
				}

				if (P_IsLocalPlayer(player) || (consoleIsServer && shareEmblems))
				{
					clientGamedata->collected[special->health-1] = true;
					M_UpdateUnlockablesAndExtraEmblems(clientGamedata);
					if (!prevCollected)  
						G_SaveGameData(clientGamedata);
				}

				if (netgame)
				{


					mobj_t *spark = P_SpawnMobjFromMobj(special, 0, 0, 0, MT_SPARK);

					if (prevCollected == false && P_EmblemWasCollected(special->health - 1) == true)
					{

						S_StartSound((shareEmblems ? NULL : special), special->info->deathsound);
					}
					else
					{

						spark->flags2 |= MF2_DONTDRAW;
					}

					return;
				}
				else
				{
					if (prevCollected == false && P_EmblemWasCollected(special->health - 1) == true)
					{

						break;
					}

					return;
				}
			}


		case MT_REDFLAG:
		case MT_BLUEFLAG:
			if (player->bot && player->bot != BOT_MPAI)
				return;
			if (player->powers[pw_flashing] || player->tossdelay)
				return;
			if (!special->spawnpoint)
				return;
			if (special->fuse == 1)
				return;


			{
				UINT8 flagteam = (special->type == MT_REDFLAG) ? 1 : 2;
				sectorspecialflags_t specialflag = (special->type == MT_REDFLAG) ? SSF_REDTEAMBASE : SSF_BLUETEAMBASE;
				const char *flagtext;
				char flagcolor;
				char plname[MAXPLAYERNAME+4];

				if (special->type == MT_REDFLAG)
				{
					flagtext = M_GetText("Red flag");
					flagcolor = '\x85';
				}
				else
				{
					flagtext = M_GetText("Blue flag");
					flagcolor = '\x84';
				}
				snprintf(plname, sizeof(plname), "%s%s%s",
						 CTFTEAMCODE(player),
						 player_names[player - players],
						 CTFTEAMENDCODE(player));

				if (player->ctfteam == flagteam)  
				{


					if (special->x>>FRACBITS != special->spawnpoint->x
					    || special->y>>FRACBITS != special->spawnpoint->y)
					{
						special->fuse = 1;
						special->flags2 |= MF2_JUSTATTACKED;

						if (!P_PlayerTouchingSectorSpecialFlag(player, specialflag))
						{
							CONS_Printf(M_GetText("%s returned the %c%s%c to base.\n"), plname, flagcolor, flagtext, 0x80);




						}
					}
				}
				else if (player->ctfteam)  
				{
					UINT16 flagflag   = (special->type == MT_REDFLAG) ? GF_REDFLAG : GF_BLUEFLAG;
					mobj_t **flagmobj = (special->type == MT_REDFLAG) ? &redflag : &blueflag;

					if (player->powers[pw_super])
						return;

					player->gotflag |= flagflag;
					CONS_Printf(M_GetText("%s picked up the %c%s%c!\n"), plname, flagcolor, flagtext, 0x80);
					(*flagmobj) = NULL;

					break;
				}
			}
			return;




		case MT_NIGHTSDRONE:
			{
				boolean spec = G_IsSpecialStage(gamemap);
				boolean cangiveemmy = false;
				if (player->bot && player->bot != BOT_MPAI)
					return;
				if (player->exiting)
					return;
				if (player->bonustime)
				{
					if (spec)  
					{

						if (toucher->tracer
						&& toucher->tracer->type == MT_GOTEMERALD)
						{}
						else  
						{
							for (i = 0; i < MAXPLAYERS; i++)
								if (playeringame[i] && players[i].playerstate == PST_LIVE
								&& players[i].mo->tracer
								&& players[i].mo->tracer->type == MT_GOTEMERALD)
									return;

						}
						cangiveemmy = true;

					}
					else
						S_StartSound(toucher, special->info->activesound);
				}
				else  
				{
					if (player->powers[pw_carry] == CR_NIGHTSMODE)
						return;

					S_StartSound(toucher, sfx_supert);
				}
				P_SwitchSpheresBonusMode(false);
				if (!(netgame || multiplayer) && !(player->powers[pw_carry] == CR_NIGHTSMODE))
					P_SetTarget(&special->tracer, toucher);
				P_SetTarget(&player->drone, special);  
				P_NightserizePlayer(player, special->health);  
				if (!spec)
				{
					if (toucher->tracer)  
					{
						mobj_t *orbittarget = special->target ? special->target : special;
						mobj_t *hnext = orbittarget->hnext, *anchorpoint = NULL, *anchorpoint2 = NULL;
						mobj_t *mo2;
						thinker_t *th;




						for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
						{
							if (th->removing)
								continue;

							mo2 = (mobj_t *)th;

							if (mo2->type != MT_IDEYAANCHOR)
								continue;

							if (mo2->health == toucher->tracer->health)  
								anchorpoint = mo2;
							else if (toucher->tracer->hnext && mo2->health == toucher->tracer->hnext->health)
								anchorpoint2 = mo2;

							if ((!toucher->tracer->hnext && anchorpoint)
								|| (toucher->tracer->hnext && anchorpoint && anchorpoint2))
								break;
						}

						if (anchorpoint)
						{
							toucher->tracer->flags |= MF_GRENADEBOUNCE;  
							toucher->tracer->threshold = 8 << 20;  
						}

						if (anchorpoint2)
						{
							toucher->tracer->hnext->flags |= MF_GRENADEBOUNCE;  
							toucher->tracer->hnext->threshold = 8 << 20;  
						}

						P_SetTarget(&orbittarget->hnext, toucher->tracer);
						if (!orbittarget->hnext->hnext)
							P_SetTarget(&orbittarget->hnext->hnext, hnext);  
						else
							P_SetTarget(&orbittarget->hnext->hnext->target, anchorpoint2 ? anchorpoint2 : orbittarget);
						P_SetTarget(&orbittarget->hnext->target, anchorpoint ? anchorpoint : orbittarget);
						P_SetTarget(&toucher->tracer, NULL);

						if (hnext)
						{
							orbittarget->hnext->extravalue1 = (angle_t)(hnext->extravalue1 - 72*ANG1);
							if (orbittarget->hnext->extravalue1 > hnext->extravalue1)
								orbittarget->hnext->extravalue1 -= (72*ANG1)/orbittarget->hnext->extravalue1;
						}
					}
					if (player->exiting)  
					{
						mobj_t *hnext = special->target ? special->target : special;  
						while ((hnext = hnext->hnext))
						{
							hnext->flags &= ~MF_GRENADEBOUNCE;
							hnext->threshold = 0;
							P_SetTarget(&hnext->target, toucher);
						}
					}
					return;
				}

				if (!cangiveemmy)
					return;

				if (player->exiting)
					P_GiveEmerald(false);
				else if (player->mo->tracer && player->mare)
				{
					P_KillMobj(toucher->tracer, NULL, NULL, 0);  
					S_StartSound(NULL, sfx_ghosty);
					special->flags2 |= MF2_DONTDRAW;
				}

				return;
			}
		case MT_NIGHTSLOOPHELPER:

			if (special->fuse < toucher->fuse - TICRATE)
			{
				thinker_t *th;
				mobj_t *mo2;
				INT32 count;
				fixed_t x,y,z, gatherradius;
				angle_t d;
				statenum_t sparklestate = S_NULL;

				if (special->target != toucher)  
					return;

				x = special->x>>FRACBITS;
				y = special->y>>FRACBITS;
				z = special->z>>FRACBITS;
				count = 1;


				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->removing)
						continue;

					mo2 = (mobj_t *)th;

					if (mo2 == special)
						continue;


					if (mo2->target != toucher)
						continue;

					if (mo2->type == MT_NIGHTSPARKLE)
						mo2->tics = 1;
					else if (mo2->type == MT_NIGHTSLOOPHELPER)
					{
						if (mo2->fuse >= special->fuse)
						{
							count++;
							x += mo2->x>>FRACBITS;
							y += mo2->y>>FRACBITS;
							z += mo2->z>>FRACBITS;
						}
						P_RemoveMobj(mo2);
					}
				}
				x = (x/count)<<FRACBITS;
				y = (y/count)<<FRACBITS;
				z = (z/count)<<FRACBITS;
				gatherradius = P_AproxDistance(P_AproxDistance(special->x - x, special->y - y), special->z - z);
				P_RemoveMobj(special);

				if (player->powers[pw_nights_superloop])
				{
					gatherradius *= 2;
					sparklestate = mobjinfo[MT_NIGHTSPARKLE].seestate;
				}

				if (gatherradius < 30*FRACUNIT)  
					return;

				for (d = 0; d < 16; d++)
					P_SpawnParaloop(x, y, z, gatherradius, 16, MT_NIGHTSPARKLE, sparklestate, d*ANGLE_22h, false);

				S_StartSound(toucher, sfx_prloop);



				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->removing)
						continue;

					mo2 = (mobj_t *)th;

					if (P_AproxDistance(P_AproxDistance(mo2->x - x, mo2->y - y), mo2->z - z) > gatherradius)
						continue;

					if (mo2->flags & MF_SHOOTABLE)
					{
						P_DamageMobj(mo2, toucher, toucher, 1, 0);
						continue;
					}



					if (mo2->flags & MF_NIGHTSITEM)
					{

						if ((mo2->flags2 & MF2_STRONGBOX) && !player->bonustime)
							continue;

						if (!(mo2->flags & MF_SPECIAL) && mo2->health)
						{
							mo2->flags2 &= ~MF2_DONTDRAW;
							mo2->flags |= MF_SPECIAL;
							mo2->flags &= ~MF_NIGHTSITEM;
							S_StartSound(toucher, sfx_hidden);
							continue;
						}
					}

					if (!(mo2->type == MT_RING || mo2->type == MT_COIN
						|| mo2->type == MT_BLUESPHERE || mo2->type == MT_BOMBSPHERE
						|| mo2->type == MT_NIGHTSCHIP || mo2->type == MT_NIGHTSSTAR
						|| ((mo2->type == MT_EMBLEM) && (mo2->reactiontime & GE_NIGHTSPULL) && P_CanPickupEmblem(player, mo2->health - 1) && !P_EmblemWasCollected(mo2->health - 1))))
						continue;


					mo2->flags |= MF_NOCLIP|MF_NOCLIPHEIGHT;
					mo2->flags2 |= MF2_NIGHTSPULL;


					mo2->movefactor = 0;  
					P_SetTarget(&mo2->tracer, toucher);
				}
			}
			return;
		case MT_EGGCAPSULE:
			if (player->bot && player->bot != BOT_MPAI)
				return;


			if (player->powers[pw_carry] == CR_NIGHTSMODE && !toucher->target)
				return;

			if (toucher->tracer && toucher->tracer->health > 0)
				return;  

			if (player->mare != special->threshold)  
				return;

			if (special->reactiontime > 0)  
				return;

			if (G_IsSpecialStage(gamemap) && !player->exiting)
			{  
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && (&players[i] != player) && players[i].spheres > 0)
					{
						player->spheres += players[i].spheres;
						players[i].spheres = 0;
					}
			}

			if (player->spheres <= 0 || player->exiting)
				return;


			P_SetTarget(&player->capsule, special);
			special->reactiontime = (player-players)+1;
			P_SetTarget(&special->target, NULL);


			player->texttimer = 0;
			return;
		case MT_NIGHTSBUMPER:

			if (player->exiting)
				return;

			if (player->bumpertime <= (TICRATE/2)-5)
			{
				S_StartSound(toucher, special->info->seesound);
				if (player->powers[pw_carry] == CR_NIGHTSMODE)
				{
					player->bumpertime = TICRATE/2;
					if (special->threshold > 0)
						player->flyangle = (special->threshold*30)-1;
					else
						player->flyangle = special->threshold;

					player->speed = FixedMul(special->info->speed, special->scale);
					P_SetTarget(&player->mo->hnext, special);  
				}
				else  
				{
					angle_t fa;
					fixed_t xspeed, yspeed;
					const fixed_t speed = FixedMul(FixedDiv(special->info->speed*FRACUNIT,75*FRACUNIT), FixedSqrt(FixedMul(toucher->scale,special->scale)));

					player->bumpertime = TICRATE/2;

					P_UnsetThingPosition(toucher);
					toucher->x = special->x;
					toucher->y = special->y;
					P_SetThingPosition(toucher);
					toucher->z = special->z+(special->height/4);

					if (special->threshold > 0)
						fa = (FixedAngle(((special->threshold*30)-1)*FRACUNIT)>>ANGLETOFINESHIFT) & FINEMASK;
					else
						fa = 0;

					xspeed = FixedMul(FINECOSINE(fa),speed);
					yspeed = FixedMul(FINESINE(fa),speed);

					P_InstaThrust(toucher, special->angle, xspeed/10);
					toucher->momz = yspeed/11;

					toucher->angle = special->angle;

					P_SetPlayerAngle(player, toucher->angle);

					P_ResetPlayer(player);

					P_SetMobjState(toucher, S_PLAY_FALL);
				}
			}
			return;
		case MT_NIGHTSSUPERLOOP:
			if ((player->bot && player->bot != BOT_MPAI) || !(player->powers[pw_carry] == CR_NIGHTSMODE))
				return;
			if (!G_IsSpecialStage(gamemap))
				player->powers[pw_nights_superloop] = (UINT16)special->info->speed;
			else
			{
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE)
						players[i].powers[pw_nights_superloop] = (UINT16)special->info->speed;
				if (special->info->deathsound != sfx_None)
					S_StartSound(NULL, special->info->deathsound);
			}


			if (player == &players[displayplayer] || G_IsSpecialStage(gamemap))
			{
				HU_SetCEchoFlags(V_AUTOFADEOUT);
				HU_SetCEchoDuration(4);
				HU_DoCEcho(M_GetText("\\\\\\\\\\\\\\\\Super Paraloop"));
			}
			break;
		case MT_NIGHTSDRILLREFILL:
			if ((player->bot && player->bot != BOT_MPAI) || !(player->powers[pw_carry] == CR_NIGHTSMODE))
				return;
			if (!G_IsSpecialStage(gamemap))
				player->drillmeter = special->info->speed;
			else
			{
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE)
						players[i].drillmeter = special->info->speed;
				if (special->info->deathsound != sfx_None)
					S_StartSound(NULL, special->info->deathsound);
			}


			if (player == &players[displayplayer] || G_IsSpecialStage(gamemap))
			{
				HU_SetCEchoFlags(V_AUTOFADEOUT);
				HU_SetCEchoDuration(4);
				HU_DoCEcho(M_GetText("\\\\\\\\\\\\\\\\Drill Refill"));
			}
			break;
		case MT_NIGHTSHELPER:
			if ((player->bot && player->bot != BOT_MPAI) || !(player->powers[pw_carry] == CR_NIGHTSMODE))
				return;
			if (!G_IsSpecialStage(gamemap))
			{

				mobj_t *flickyobj = P_SpawnMobj(toucher->x, toucher->y, toucher->z + toucher->info->height, MT_NIGHTOPIANHELPER);
				if (!P_MobjWasRemoved(flickyobj))
					P_SetTarget(&flickyobj->target, toucher);

				player->powers[pw_nights_helper] = (UINT16)special->info->speed;
			}
			else
			{
				mobj_t *flickyobj;
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].mo && players[i].powers[pw_carry] == CR_NIGHTSMODE) {
						players[i].powers[pw_nights_helper] = (UINT16)special->info->speed;
						flickyobj = P_SpawnMobj(players[i].mo->x, players[i].mo->y, players[i].mo->z + players[i].mo->info->height, MT_NIGHTOPIANHELPER);
						if (!P_MobjWasRemoved(flickyobj))
							P_SetTarget(&flickyobj->target, players[i].mo);
					}
				if (special->info->deathsound != sfx_None)
					S_StartSound(NULL, special->info->deathsound);
			}


			if (player == &players[displayplayer] || G_IsSpecialStage(gamemap))
			{
				HU_SetCEchoFlags(V_AUTOFADEOUT);
				HU_SetCEchoDuration(4);
				HU_DoCEcho(M_GetText("\\\\\\\\\\\\\\\\Nightopian Helper"));
			}
			break;
		case MT_NIGHTSEXTRATIME:
			if ((player->bot && player->bot != BOT_MPAI) || !(player->powers[pw_carry] == CR_NIGHTSMODE || (G_IsSpecialStage(gamemap) && !(maptol & TOL_NIGHTS))))
				return;
			if (!G_IsSpecialStage(gamemap))
			{
				player->nightstime += special->info->speed;
				player->startedtime += special->info->speed;
				player->lapstartedtime += special->info->speed;
				P_RestoreMusic(player);
			}
			else
			{
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && (player->powers[pw_carry] == CR_NIGHTSMODE || (G_IsSpecialStage(gamemap) && !(maptol & TOL_NIGHTS))))
					{
						players[i].nightstime += special->info->speed;
						players[i].startedtime += special->info->speed;
						players[i].lapstartedtime += special->info->speed;
						P_RestoreMusic(&players[i]);
					}
				if (special->info->deathsound != sfx_None)
					S_StartSound(NULL, special->info->deathsound);
			}


			if (player == &players[displayplayer] || G_IsSpecialStage(gamemap))
			{
				HU_SetCEchoFlags(V_AUTOFADEOUT);
				HU_SetCEchoDuration(4);
				HU_DoCEcho(M_GetText("\\\\\\\\\\\\\\\\Extra Time"));
			}
			break;
		case MT_NIGHTSLINKFREEZE:
			if ((player->bot && player->bot != BOT_MPAI) || !(player->powers[pw_carry] == CR_NIGHTSMODE))
				return;
			if (!G_IsSpecialStage(gamemap))
			{
				player->powers[pw_nights_linkfreeze] = (UINT16)special->info->speed;
				player->linktimer = nightslinktics;
			}
			else
			{
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE)
					{
						players[i].powers[pw_nights_linkfreeze] += (UINT16)special->info->speed;
						players[i].linktimer = nightslinktics;
					}
				if (special->info->deathsound != sfx_None)
					S_StartSound(NULL, special->info->deathsound);
			}


			if (player == &players[displayplayer] || G_IsSpecialStage(gamemap))
			{
				HU_SetCEchoFlags(V_AUTOFADEOUT);
				HU_SetCEchoDuration(4);
				HU_DoCEcho(M_GetText("\\\\\\\\\\\\\\\\Link Freeze"));
			}
			break;
		case MT_HOOPCOLLIDE:

			for (; special->hprev != NULL; special = special->hprev);  
			i = 0;
			for (; special->type == MT_HOOP; special = special->hnext)
			{
				if (!P_MobjWasRemoved(special->target))
				{
					special->fuse = 11;
					special->movedir = i;
					special->extravalue1 = special->target->extravalue1;
					special->extravalue2 = special->target->extravalue2;
					special->target->threshold = 4242;
				}
				i++;
			}

			{
				mobj_t *hnext;
				for (; special != NULL; special = hnext)
				{
					hnext = special->hnext;
					P_RemoveMobj(special);
				}
			}

			P_DoNightsScore(player);



			if (G_IsSpecialStage(gamemap))
			{
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE)
						players[i].drillmeter += TICRATE/2;
			}
			else if (player->bot && player->bot != BOT_MPAI)
				players[consoleplayer].drillmeter += TICRATE/2;
			else
				player->drillmeter += TICRATE/2;


			if (player->linkcount <= 5)
				S_StartSound(toucher, sfx_hoop1);
			else if (player->linkcount <= 10)
				S_StartSound(toucher, sfx_hoop2);
			else
				S_StartSound(toucher, sfx_hoop3);
			return;




		case MT_SHELL:
			{
				boolean bounceon = ((P_MobjFlip(toucher)*(toucher->z - (special->z + special->height/2)) > 0) && (P_MobjFlip(toucher)*toucher->momz < 0));
				if (special->threshold == TICRATE)  
				{
					if (bounceon)
					{

						special->momx = special->momy = 0;
						S_StartSound(toucher, sfx_mario2);
						P_SetTarget(&special->target, NULL);
						special->threshold = TICRATE - 1;
						toucher->momz = -toucher->momz;
					}
					else  
						P_DamageMobj(toucher, special, special->target, 1, 0);
				}
				else if (special->threshold == 0)
				{

					special->movedir = ((special->movedir == 1) ? -1 : 1);
					P_InstaThrust(special, toucher->angle, (special->info->speed*special->scale));
					S_StartSound(toucher, sfx_mario2);
					P_SetTarget(&special->target, toucher);
					special->threshold = (3*TICRATE)/2;
					if (bounceon)
						toucher->momz = -toucher->momz;
				}
			}
			return;
		case MT_AXE:
			{
				thinker_t  *th;
				mobj_t *mo2;

				if (player->bot && player->bot != BOT_MPAI)
					return;

				if (special->spawnpoint)
					EV_DoElevator(special->spawnpoint->args[0], NULL, bridgeFall);


				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->removing)
						continue;

					mo2 = (mobj_t *)th;

					if (mo2->type != MT_KOOPA)
						continue;

					mo2->momz = 5*FRACUNIT;
					break;
				}
			}
			break;
		case MT_LETTER:
		{
			if (special->health && !player->bot)
			{
				F_StartTextPrompt(199, 0, toucher, 0, true, false);
				special->health = 0;
				if (ultimatemode && player->continues < 99)
					player->continues++;
			}
			return;
		}
		case MT_FIREFLOWER:
			if (player->bot && player->bot != BOT_MPAI)
				return;

			S_StartSound(toucher, sfx_mario3);

			player->powers[pw_shield] = (player->powers[pw_shield] & SH_NOSTACK)|SH_FIREFLOWER;

			if (!(player->powers[pw_super] || (mariomode && player->powers[pw_invulnerability])))
			{
				player->mo->color = SKINCOLOR_WHITE;
				G_GhostAddColor(GHC_FIREFLOWER);
			}

			break;




		case MT_STARPOST:
			P_TouchStarPost(special, player, special->spawnpoint && special->spawnpoint->args[1]);
			return;

		case MT_FAKEMOBILE:
			{
				fixed_t touchx, touchy, touchspeed;
				angle_t angle;

				if (P_AproxDistance(toucher->x-special->x, toucher->y-special->y) >
					P_AproxDistance((toucher->x-toucher->momx)-special->x, (toucher->y-toucher->momy)-special->y))
				{
					touchx = toucher->x + toucher->momx;
					touchy = toucher->y + toucher->momy;
				}
				else
				{
					touchx = toucher->x;
					touchy = toucher->y;
				}

				angle = R_PointToAngle2(special->x, special->y, touchx, touchy);
				touchspeed = P_AproxDistance(toucher->momx, toucher->momy);

				toucher->momx = P_ReturnThrustX(special, angle, touchspeed);
				toucher->momy = P_ReturnThrustY(special, angle, touchspeed);
				toucher->momz = -toucher->momz;
				if (player->pflags & PF_GLIDING && !P_IsObjectOnGround(toucher))
				{
					player->pflags &= ~(PF_GLIDING|PF_JUMPED|PF_NOJUMPDAMAGE);
					P_SetMobjState(toucher, S_PLAY_FALL);
					toucher->momz += P_MobjFlip(toucher) * (player->speed >> 3);
					toucher->momx = 7*toucher->momx>>3;
					toucher->momy = 7*toucher->momy>>3;
				}
				player->homing = 0;


				S_StartSound(toucher, special->info->painsound);
			}
			return;

		case MT_BLACKEGGMAN_GOOPFIRE:
			if (!player->powers[pw_flashing] && !(player->powers[pw_ignorelatch] & (1<<15)))
			{
				toucher->momx = 0;
				toucher->momy = 0;

				if (toucher->momz != 0)
					special->momz = toucher->momz;

				player->powers[pw_carry] = CR_BRAKGOOP;
				P_SetTarget(&toucher->tracer, special);

				P_ResetPlayer(player);

				if (special->target && special->target->state == &states[S_BLACKEGG_SHOOT1])
				{
					if (special->target->health <= 2 && P_RandomChance(FRACUNIT/2))
						P_SetMobjState(special->target, special->target->info->missilestate);
					else
						P_SetMobjState(special->target, special->target->info->raisestate);
				}
			}
			return;
		case MT_EGGSHIELD:
			{
				angle_t angle = R_PointToAngle2(special->x, special->y, toucher->x, toucher->y) - special->angle;
				fixed_t touchspeed = P_AproxDistance(toucher->momx, toucher->momy);
				if (touchspeed < special->scale)
					touchspeed = special->scale;


				if (!(angle > ANGLE_90 && angle < ANGLE_270))
				{
					toucher->momx = P_ReturnThrustX(special, special->angle, touchspeed);
					toucher->momy = P_ReturnThrustY(special, special->angle, touchspeed);
					toucher->momz = -toucher->momz;
					if (player->pflags & PF_GLIDING && !P_IsObjectOnGround(toucher))
					{
						player->pflags &= ~(PF_GLIDING|PF_JUMPED|PF_NOJUMPDAMAGE);
						P_SetMobjState(toucher, S_PLAY_FALL);
						toucher->momz += P_MobjFlip(toucher) * (player->speed >> 3);
						toucher->momx = 7*toucher->momx>>3;
						toucher->momy = 7*toucher->momy>>3;
					}
					player->homing = 0;


					S_StartSound(toucher, special->info->painsound);


					if (special->target)
						special->target->extravalue1 = -special->target->info->speed;
				}
				else
				{

					toucher->momx = -toucher->momx/2;
					toucher->momy = -toucher->momy/2;
					toucher->momz = -toucher->momz;
					break;
				}
			}
			return;

		case MT_EGGROBO1:
			if (special->state == &states[special->info->deathstate])
				return;
			if (P_PlayerInPain(player))
				return;

			P_SetMobjState(special, special->info->meleestate);
			special->angle = special->movedir;
			special->momx = special->momy = 0;


			P_SetMobjState(toucher, S_PLAY_STUN);
			player->pflags &= ~PF_APPLYAUTOBRAKE;
			P_ResetPlayer(player);
			player->drawangle = special->angle + ANGLE_180;
			P_InstaThrust(toucher, special->angle, FixedMul(3*special->info->speed, special->scale/2));
			toucher->z += P_MobjFlip(toucher);
			if (toucher->eflags & MFE_UNDERWATER)  
				P_SetObjectMomZ(toucher, FixedDiv(10511*FRACUNIT,2600*FRACUNIT), false);
			else
				P_SetObjectMomZ(toucher, FixedDiv(69*FRACUNIT,10*FRACUNIT), false);
			if (P_IsLocalPlayer(player))
			{
				quake.intensity = 9*FRACUNIT;
				quake.time = TICRATE/2;
				quake.epicenter = NULL;
			}

#if 0  
			toucher->angle = special->angle;
			P_SetPlayerAngle(player, toucher->angle);
#endif

			S_StartSound(toucher, special->info->attacksound);  

			return;

		case MT_BIGTUMBLEWEED:
		case MT_LITTLETUMBLEWEED:
			if (toucher->momx || toucher->momy)
			{
				special->momx = toucher->momx;
				special->momy = toucher->momy;
				special->momz = P_AproxDistance(toucher->momx, toucher->momy)/4;

				if (toucher->momz > 0)
					special->momz += toucher->momz/8;

				P_SetMobjState(special, special->info->seestate);
			}
			return;
		case MT_SMALLGRABCHAIN:
		case MT_BIGGRABCHAIN:
			{
				boolean macespin = false;
				if (P_MobjFlip(toucher)*toucher->momz > 0
					|| (player->powers[pw_carry]))
					return;

				if (toucher->z > special->z + special->height/2)
					return;

				if (toucher->z + toucher->height/2 < special->z)
					return;

				if (player->powers[pw_flashing])
					return;

				if (special->tracer && !(special->tracer->flags2 & MF2_STRONGBOX))
					macespin = true;

				if (macespin ? (player->powers[pw_ignorelatch] & (1<<15)) : (player->powers[pw_ignorelatch]))
					return;

				if (special->movefactor && special->tracer && special->tracer->angle != ANGLE_90 && special->tracer->angle != ANGLE_270)
				{  
					angle_t ang = R_PointToAngle2(special->x, special->y, toucher->x, toucher->y) - special->tracer->angle;
					if ((special->movefactor > 0) == (special->tracer->angle > ANGLE_90 && special->tracer->angle < ANGLE_270))
						ang += ANGLE_180;
					if (ang < ANGLE_180)
						return;  
				}

				P_ResetPlayer(player);
				P_SetTarget(&toucher->tracer, special);

				if (macespin)
				{
					player->powers[pw_carry] = CR_MACESPIN;
					S_StartSound(toucher, sfx_spin);
					P_SetMobjState(toucher, S_PLAY_ROLL);
				}
				else
					player->powers[pw_carry] = CR_GENERIC;


				player->pflags |= PF_JUMPSTASIS;

				return;
			}
		case MT_EGGMOBILE2_POGO:

			if (!special->target || !special->target->health)
				return;

			if (special->target->momz < 0)
			{
				P_DamageMobj(toucher, special, special->target, 1, 0);

				special->target->momx = special->target->momy = 0;
				special->target->momz = 0;
				special->target->flags |= MF_NOGRAVITY;
				P_SetMobjState(special->target, special->info->raisestate);
				S_StartSound(special->target, special->info->activesound);
				P_RemoveMobj(special);
			}
			return;

		case MT_EXTRALARGEBUBBLE:
			if (player->powers[pw_shield] & SH_PROTECTWATER)
				return;
			if (maptol & TOL_NIGHTS)
				return;
			if (mariomode)
				return;
			if (special->state-states != S_EXTRALARGEBUBBLE)
				return;  
			else if (toucher->eflags & MFE_VERTICALFLIP)
			{
				if (special->z+special->height < toucher->z
					|| special->z+special->height > toucher->z + (toucher->height*2/3))
					return;  
			}
			else if (special->z < toucher->z
				|| special->z > toucher->z + (toucher->height*2/3))
				return;  


			if ((!player->bot || player->bot == BOT_MPAI) && (player->powers[pw_underwater] && player->powers[pw_underwater] <= 12*TICRATE + 1))
			{
				player->powers[pw_underwater] = underwatertics + 1;
				P_RestoreMusic(player);
			}

			if (player->powers[pw_underwater] < underwatertics + 1)
				player->powers[pw_underwater] = underwatertics + 1;

			if (!player->climbing)
			{
				if (player->bot && player->bot != BOT_MPAI && toucher->state-states != S_PLAY_GASP)
					S_StartSound(toucher, special->info->deathsound);  
				P_SetMobjState(toucher, S_PLAY_GASP);
				P_ResetPlayer(player);
			}

			toucher->momx = toucher->momy = toucher->momz = 0;

			if (player->bot && player->bot != BOT_MPAI)
				return;
			else
				break;

		case MT_WATERDROP:
			if (special->state == &states[special->info->spawnstate])
			{
				special->z = toucher->z+toucher->height-FixedMul(8*FRACUNIT, special->scale);
				special->momz = 0;
				special->flags |= MF_NOGRAVITY;
				P_SetMobjState (special, special->info->deathstate);
				S_StartSound (special, special->info->deathsound+(P_RandomKey(special->info->mass)));
			}
			return;

		case MT_CANARIVORE_GAS:

			special->flags |= MF_NOGRAVITY|MF_NOCLIPHEIGHT;
			P_UnsetThingPosition(special);
			special->x = toucher->x - toucher->momx/2;
			special->y = toucher->y - toucher->momy/2;
			special->z = toucher->z - toucher->momz/2;
			P_SetThingPosition(special);
			toucher->momx = FixedMul(toucher->momx, 50*FRACUNIT/51);
			toucher->momy = FixedMul(toucher->momy, 50*FRACUNIT/51);
			special->momx = toucher->momx;
			special->momy = toucher->momy;
			special->momz = toucher->momz;
			return;

		case MT_MINECARTSPAWNER:
			if (!player->bot && player->bot != BOT_MPAI && special->fuse <= TICRATE && player->powers[pw_carry] != CR_MINECART && !(player->powers[pw_ignorelatch] & (1<<15)))
			{
				mobj_t *mcart = P_SpawnMobj(special->x, special->y, special->z, MT_MINECART);
				if (!P_MobjWasRemoved(mcart))
				{
					P_SetTarget(&mcart->target, toucher);
					mcart->angle = toucher->angle = player->drawangle = special->angle;
					mcart->friction = FRACUNIT;
				}

				P_ResetPlayer(player);
				player->pflags |= PF_JUMPDOWN;
				player->powers[pw_carry] = CR_MINECART;
				player->pflags &= ~PF_APPLYAUTOBRAKE;
				P_SetTarget(&toucher->tracer, mcart);
				toucher->momx = toucher->momy = toucher->momz = 0;

				special->fuse = 3*TICRATE;
				special->flags2 |= MF2_DONTDRAW;
			}
			return;

		case MT_MINECARTEND:
			if (player->powers[pw_carry] == CR_MINECART && toucher->tracer && !P_MobjWasRemoved(toucher->tracer) && toucher->tracer->health)
			{
				fixed_t maxz = max(toucher->z, special->z + 35*special->scale);

				toucher->momx = toucher->tracer->momx/2;
				toucher->momy = toucher->tracer->momy/2;
				toucher->momz = toucher->tracer->momz + P_AproxDistance(toucher->tracer->momx, toucher->tracer->momy)/2;
				P_ResetPlayer(player);
				player->pflags &= ~PF_APPLYAUTOBRAKE;
				P_SetMobjState(toucher, S_PLAY_FALL);
				P_SetTarget(&toucher->tracer->target, NULL);
				P_KillMobj(toucher->tracer, toucher, special, 0);
				P_SetTarget(&toucher->tracer, NULL);
				player->powers[pw_carry] = CR_NONE;
				P_UnsetThingPosition(toucher);
				toucher->x = special->x;
				toucher->y = special->y;
				toucher->z = maxz;
				P_SetThingPosition(toucher);
			}
			return;

		case MT_MINECARTSWITCHPOINT:
			if (player->powers[pw_carry] == CR_MINECART && toucher->tracer && !P_MobjWasRemoved(toucher->tracer) && toucher->tracer->health)
			{
				mobjflag2_t destambush = special->flags2 & MF2_AMBUSH;
				angle_t angdiff = toucher->tracer->angle - special->angle;
				if (angdiff > ANGLE_90 && angdiff < ANGLE_270)
					destambush ^= MF2_AMBUSH;
				toucher->tracer->flags2 = (toucher->tracer->flags2 & ~MF2_AMBUSH) | destambush;
			}
			return;

		default:  
			if (player->bot && player->bot != BOT_MPAI)
				return;
			P_SetTarget(&special->target, toucher);
			break;
		}
	}

	S_StartSound(toucher, special->info->deathsound);  
	P_KillMobj(special, NULL, toucher, 0);
	special->shadowscale = 0;
}

                                                  
   
                                        
                                                               
                                                                                                                        
    
void P_TouchStarPost(mobj_t *post, player_t *player, boolean snaptopost)
{
	size_t i;
	mobj_t *toucher = player->mo;
	mobj_t *checkbase = snaptopost ? post : toucher;

	if (player->bot && player->bot != BOT_MPAI)
		return;

	if (circuitmap
		&& post->health - player->starpostnum > 1)
	{

		if (!player->tossdelay)
			S_StartSound(toucher, sfx_lose);
		player->tossdelay = 3;
		return;
	}


	if (post->health > 1365)
	{
		CONS_Debug(DBG_GAMELOGIC, "Bad Starpost Number!\n");
		return;
	}

	if (player->starpostnum >= post->health)
		return;  

	if (cv_coopstarposts.value && G_GametypeUsesCoopStarposts() && (netgame || multiplayer))
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				if (players[i].bot)  
					continue;

				players[i].starposttime = leveltime;
				players[i].starpostx = checkbase->x>>FRACBITS;
				players[i].starposty = checkbase->y>>FRACBITS;
				players[i].starpostz = post->z>>FRACBITS;
				players[i].starpostangle = post->angle;
				players[i].starpostscale = player->mo->destscale;
				if (post->flags2 & MF2_OBJECTFLIP)
				{
					players[i].starpostscale *= -1;
					players[i].starpostz += post->height>>FRACBITS;
				}
				players[i].starpostnum = post->health;

				if (cv_coopstarposts.value == 2 && (players[i].playerstate == PST_DEAD || players[i].spectator) && P_GetLives(&players[i]))
					P_SpectatorJoinGame(&players[i]);  
			}
		}
		S_StartSound(NULL, post->info->painsound);
	}
	else
	{

		player->starposttime = leveltime;
		player->starpostx = checkbase->x>>FRACBITS;
		player->starposty = checkbase->y>>FRACBITS;
		player->starpostz = post->z>>FRACBITS;
		player->starpostangle = post->angle;
		player->starpostscale = player->mo->destscale;
		if (post->flags2 & MF2_OBJECTFLIP)
		{
			player->starpostscale *= -1;
			player->starpostz += post->height>>FRACBITS;
		}
		player->starpostnum = post->health;
		S_StartSound(toucher, post->info->painsound);
	}

	P_ClearStarPost(post->health);


	if (!(netgame && circuitmap && player != &players[consoleplayer]))
	{
		thinker_t *th;
		mobj_t *mo2;

		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->removing)
				continue;

			mo2 = (mobj_t *)th;

			if (mo2->type != MT_STARPOST)
				continue;
			if (mo2->health != post->health)
				continue;

			P_SetMobjState(mo2, mo2->info->painstate);
		}
	}
}

                                                            
   
                                     
                                                         
                                               
                                                                                                               
    
static void P_HitDeathMessages(player_t *player, mobj_t *inflictor, mobj_t *source, UINT8 damagetype)
{
	const char *str = NULL;
	boolean deathonly = false;
	boolean deadsource = false;
	boolean deadtarget = false;

	char targetname[MAXPLAYERNAME+4];
	char sourcename[MAXPLAYERNAME+4];

	if (!(gametyperules & (GTR_RINGSLINGER|GTR_HURTMESSAGES)))
		return;  

	if (!player)
		return;  

	if (!player->mo)
		return;  

	if (player->spectator)
		return;  

	if (!netgame)
		return;  

	if (LUA_HookHurtMsg(player, inflictor, source, damagetype))
		return;

	deadtarget = (player->mo->health <= 0);


	if (!deadtarget && !cv_hazardlog.value)
		return;


	snprintf(targetname, sizeof(targetname), "%s%s%s",
	         CTFTEAMCODE(player),
	         player_names[player - players],
	         CTFTEAMENDCODE(player));

	if (source)
	{

		I_Assert(inflictor != NULL);

		if (source->player)
		{

			snprintf(sourcename, sizeof(sourcename), "%s%s%s",
					 CTFTEAMCODE(source->player),
					 player_names[source->player - players],
					 CTFTEAMENDCODE(source->player));



			if (source->player->playerstate == PST_DEAD && source->player != player &&
			 (inflictor->flags2 & MF2_BEYONDTHEGRAVE))
				deadsource = true;

			if (inflictor->flags & MF_PUSHABLE)
			{
				str = M_GetText("%s%s's playtime with heavy objects %s %s.\n");
			}
			else switch (inflictor->type)
			{
				case MT_PLAYER:
					if (damagetype == DMG_NUKE)  
						str = M_GetText("%s%s's armageddon blast %s %s.\n");
					else if ((inflictor->player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL && (inflictor->player->pflags & PF_SHIELDABILITY))
						str = M_GetText("%s%s's elemental stomp %s %s.\n");
					else if (inflictor->player->powers[pw_invulnerability])
						str = M_GetText("%s%s's invincibility aura %s %s.\n");
					else if (inflictor->player->powers[pw_super])
						str = M_GetText("%s%s's super aura %s %s.\n");
					else
						str = M_GetText("%s%s's tagging hand %s %s.\n");
					break;
				case MT_SPINFIRE:
					str = M_GetText("%s%s's elemental fire trail %s %s.\n");
					break;
				case MT_THROWNBOUNCE:
					str = M_GetText("%s%s's bounce ring %s %s.\n");
					break;
				case MT_THROWNINFINITY:
					str = M_GetText("%s%s's infinity ring %s %s.\n");
					break;
				case MT_THROWNAUTOMATIC:
					str = M_GetText("%s%s's automatic ring %s %s.\n");
					break;
				case MT_THROWNSCATTER:
					str = M_GetText("%s%s's scatter ring %s %s.\n");
					break;

				case MT_THROWNEXPLOSION:
					str = M_GetText("%s%s's explosion ring %s %s.\n");
					break;
				case MT_THROWNGRENADE:
					str = M_GetText("%s%s's grenade ring %s %s.\n");
					break;
				case MT_REDRING:
					if (inflictor->flags2 & MF2_RAILRING)
						str = M_GetText("%s%s's rail ring %s %s.\n");
					else
						str = M_GetText("%s%s's thrown ring %s %s.\n");
					break;
				default:
					str = M_GetText("%s%s %s %s.\n");
					break;
			}

			CONS_Printf(str,
				deadsource ? M_GetText("The late ") : "",
				sourcename,
				deadtarget ? M_GetText("killed") : M_GetText("hit"),
				targetname);
			return;
		}
		else switch (source->type)
		{
			case MT_EGGMAN_ICON:
				str = M_GetText("%s was %s by Eggman's nefarious TV magic.\n");
				break;
			case MT_SPIKE:
			case MT_WALLSPIKE:
				str = M_GetText("%s was %s by spikes.\n");
				break;
			default:
				str = M_GetText("%s was %s by an environmental hazard.\n");
				break;
		}
	}
	else
	{

		switch (damagetype)
		{
			case DMG_WATER:
				str = M_GetText("%s was %s by dangerous water.\n");
				break;
			case DMG_FIRE:
				str = M_GetText("%s was %s by molten lava.\n");
				break;
			case DMG_ELECTRIC:
				str = M_GetText("%s was %s by electricity.\n");
				break;
			case DMG_SPIKE:
				str = M_GetText("%s was %s by spikes.\n");
				break;
			case DMG_DROWNED:
				deathonly = true;
				str = M_GetText("%s drowned.\n");
				break;
			case DMG_CRUSHED:
				deathonly = true;
				str = M_GetText("%s was crushed.\n");
				break;
			case DMG_DEATHPIT:
				if (deadtarget)
				{
					deathonly = true;
					str = M_GetText("%s fell into a bottomless pit.\n");
				}
				break;
			case DMG_SPACEDROWN:
				if (deadtarget)
				{
					deathonly = true;
					str = M_GetText("%s asphyxiated in space.\n");
				}
				break;
			default:
				if (deadtarget)
				{
					deathonly = true;
					str = M_GetText("%s died.\n");
				}
				break;
		}
		if (!str)
			str = M_GetText("%s was %s by an environmental hazard.\n");
	}

	if (!str)  
		return;

	if (deathonly)
	{
		if (!deadtarget)
			return;
		CONS_Printf(str, targetname);
	}
	else
		CONS_Printf(str, targetname, deadtarget ? M_GetText("killed") : M_GetText("hit"));
}

                                                                             
                                                                              
                                                                         
                                                                       
                          
   
                                                         
    
void P_CheckTimeLimit(void)
{
	INT32 i, k;

	if (!cv_timelimit.value)
		return;

	if (!(multiplayer || netgame))
		return;

	if (!(gametyperules & GTR_TIMELIMIT))
		return;

	if (leveltime < timelimitintics)
		return;

	if (gameaction == ga_completed)
		return;



	if (G_TagGametype())
	{
		if (leveltime == (timelimitintics + 1))
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator
				 || (players[i].pflags & PF_GAMETYPEOVER) || (players[i].pflags & PF_TAGIT))
					continue;

				CONS_Printf(M_GetText("%s received double points for surviving the round.\n"), player_names[i]);
				P_AddPlayerScore(&players[i], players[i].score);
			}
		}

		if (server)
			SendNetXCmd(XD_EXITLEVEL, NULL, 0);
	}


	else if ((cv_overtime.value) && (gametyperules & GTR_OVERTIME))
	{
		INT32 playerarray[MAXPLAYERS];
		INT32 tempplayer = 0;
		INT32 spectators = 0;
		INT32 playercount = 0;


		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && players[i].spectator)
				spectators++;
		}

		if ((D_NumPlayers() - spectators) > 1)
		{

			if (gamestate == GS_LEVEL && (leveltime == (timelimitintics + TICRATE)))
				S_StartSound(NULL, sfx_strpst);


			if (!G_GametypeHasTeams())
			{

				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (playeringame[i] && !players[i].spectator)
					{
						playerarray[playercount] = i;
						playercount++;
					}
				}


				for (i = 1; i < playercount; i++)
				{
					for (k = i; k < playercount; k++)
					{
						if (players[playerarray[i-1]].score < players[playerarray[k]].score)
						{
							tempplayer = playerarray[i-1];
							playerarray[i-1] = playerarray[k];
							playerarray[k] = tempplayer;
						}
					}
				}


				if (players[playerarray[0]].score == players[playerarray[1]].score)
					return;
			}
			else
			{

				if (redscore == bluescore)
					return;
			}
		}
		if (server)
			SendNetXCmd(XD_EXITLEVEL, NULL, 0);
	}

	if (server)
		SendNetXCmd(XD_EXITLEVEL, NULL, 0);
}

                                                                               
                                                                        
                          
   
                                                         
    
void P_CheckPointLimit(void)
{
	INT32 i;

	if (!cv_pointlimit.value)
		return;

	if (!(multiplayer || netgame))
		return;

	if (!(gametyperules & GTR_POINTLIMIT))
		return;


	if (G_GametypeHasTeams())
	{

		if ((UINT32)cv_pointlimit.value <= redscore || (UINT32)cv_pointlimit.value <= bluescore)
		{
			if (server)
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);
		}
	}
	else
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if ((UINT32)cv_pointlimit.value <= players[i].score)
			{
				if (server)
					SendNetXCmd(XD_EXITLEVEL, NULL, 0);
				return;
			}
		}
	}
}

                                                                     
                                               
                                                               
void P_CheckSurvivors(void)
{
	INT32 i;
	INT32 survivors = 0;
	INT32 taggers = 0;
	INT32 spectators = 0;
	INT32 survivorarray[MAXPLAYERS];

	if (!D_NumPlayers())  
		return;

	for (i=0; i < MAXPLAYERS; i++)  
	{
		if (playeringame[i])
		{
			if (players[i].spectator)
				spectators++;
			else if ((players[i].pflags & PF_TAGIT) && players[i].quittime < 30 * TICRATE)
				taggers++;
			else if (!(players[i].pflags & PF_GAMETYPEOVER) && players[i].quittime < 30 * TICRATE)
			{
				survivorarray[survivors] = i;
				survivors++;
			}
		}
	}

	if (!taggers)  
	{

		if ((gametyperules & GTR_HIDEFROZEN) && (leveltime >= (hidetime * TICRATE)))
		{
			CONS_Printf(M_GetText("The IT player has left the game.\n"));
			if (server)
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);

			return;
		}

		if (survivors)
		{
			INT32 newtagger = survivorarray[P_RandomKey(survivors)];

			CONS_Printf(M_GetText("%s is now IT!\n"), player_names[newtagger]);  
			players[newtagger].pflags |= PF_TAGIT;

			survivors--;  



			if (!survivors && (D_NumPlayers() - spectators) > 1)
			{
				CONS_Printf(M_GetText("All players have been tagged!\n"));
				if (server)
					SendNetXCmd(XD_EXITLEVEL, NULL, 0);
			}

			return;
		}



		if ((D_NumPlayers() - spectators) > 1)
		{
			CONS_Printf(M_GetText("There are no players able to become IT.\n"));
			if (server)
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);
		}

		return;
	}



	if (!survivors && (D_NumPlayers() - spectators) > 1)
	{
		CONS_Printf(M_GetText("All players have been tagged!\n"));
		if (server)
			SendNetXCmd(XD_EXITLEVEL, NULL, 0);
	}
}


boolean P_CheckRacers(void)
{
	INT32 i;


	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].exiting && players[i].lives > 0)
			break;
	}

	if (i == MAXPLAYERS)  
	{
		countdown = 0;
		countdown2 = 0;
		return true;
	}

	return false;
}

                    
   
                                
                                                                           
                                               
                                                                                                                          
                                      
                    
    
void P_KillMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source, UINT8 damagetype)
{
	mobj_t *mo;

	if (inflictor && (inflictor->type == MT_SHELL || inflictor->type == MT_FIREBALL))
		S_StartScreamSound(target, sfx_mario2);

	if (!(maptol & TOL_NIGHTS) && G_IsSpecialStage(gamemap) && target->player && target->player->nightstime > 6)
		target->player->nightstime = 6;  

	if (target->flags & (MF_ENEMY|MF_BOSS))
		target->momx = target->momy = target->momz = 0;

	if (target->type != MT_PLAYER && !(target->flags & MF_MONITOR))
		target->flags |= MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT;  

	if (target->flags2 & MF2_NIGHTSPULL)
	{
		P_SetTarget(&target->tracer, NULL);
		target->movefactor = 0;  
	}


	target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SPECIAL);
	target->flags2 &= ~(MF2_SKULLFLY|MF2_NIGHTSPULL);
	target->health = 0;  

	if (LUA_HookMobjDeath(target, inflictor, source, damagetype) || P_MobjWasRemoved(target))
		return;


	if (target->player && !target->player->spectator)
	{
		if (metalrecording)  
			G_StopMetalRecording(true);
		if ((gametyperules & GTR_DEATHPENALTY)  
			&& ((target == source) || (source == NULL && inflictor == NULL) || (source && !source->player)))
		{  
			if (target->player->score >= 50)
				target->player->score -= 50;
			else
				target->player->score = 0;
		}

		target->flags2 &= ~MF2_DONTDRAW;
	}


	if (source && source->player)
	{
		if (target->flags & MF_MONITOR)
		{
			P_SetTarget(&target->target, source);
			source->player->numboxes++;

			if (!(target->flags2 & MF2_DONTRESPAWN))
			{
				if (!(netgame || multiplayer))
					target->fuse = atoi(cv_itemrespawntime.defaultvalue)*TICRATE + 2;
				else if (cv_itemrespawn.value)
					target->fuse = cv_itemrespawntime.value*TICRATE + 2;
			}
		}


		{
			INT32 score = 0;

			if (maptol & TOL_NIGHTS)  
			{
				if ((target->flags & MF_ENEMY) && !(target->flags & (MF_MISSILE|MF_BOSS)))
				{
					score = 200;

					if (source->player->bonustime)
						score *= 2;





					if (++source->player->linkcount > source->player->maxlink)
						source->player->maxlink = source->player->linkcount;
					source->player->linktimer = nightslinktics;
				}
			}
			else
			{
				if (target->flags & MF_BOSS)
					score = 1000;
				else if ((target->flags & MF_ENEMY) && !(target->flags & MF_MISSILE) && target->info->spawnhealth)
				{
					UINT8 locscoreadd = source->player->scoreadd + target->info->spawnhealth;
					mobj_t *scoremobj;
					UINT32 scorestate = mobjinfo[MT_SCORE].spawnstate;

					scoremobj = P_SpawnMobj(target->x, target->y, target->z + (target->height / 2), MT_SCORE);


					if (!mariomode) switch (locscoreadd)
					{
						case 1:  score = 100;   break;
						case 2:  score = 200;   scorestate += 1; break;
						case 3:  score = 500;   scorestate += 2; break;
						case 4: case 5: case 6: case 7: case 8: case 9:
						case 10: case 11: case 12: case 13: case 14:
						         score = 1000;  scorestate += 3; break;
						default: score = 10000; scorestate += 4; break;
					}

					else switch (locscoreadd)
					{
						case 1: score = 100;  break;
						case 2: score = 200;  scorestate += 1; break;
						case 3: score = 400;  scorestate += 5; break;
						case 4: score = 800;  scorestate += 6; break;
						case 5: score = 1000; scorestate += 3; break;
						case 6: score = 2000; scorestate += 7; break;
						case 7: score = 4000; scorestate += 8; break;
						case 8: score = 8000; scorestate += 9; break;
						default:  
							if (modeattacking)  
							{  
								score = 10000; scorestate += 4; break;
							}
							P_GivePlayerLives(source->player, 1);
							P_PlayLivesJingle(source->player);
							scorestate += 10;
							break;
					}

					if (!P_MobjWasRemoved(scoremobj))
						P_SetMobjState(scoremobj, scorestate);

					source->player->scoreadd = locscoreadd;
				}
			}

			P_AddPlayerScore(source->player, score);
		}
	}


	if (target->player)
	{
		target->flags &= ~(MF_SOLID|MF_SHOOTABLE);  
		P_UnsetThingPosition(target);
		target->flags |= MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY;
		P_SetThingPosition(target);
		target->standingslope = NULL;
		target->pmomz = 0;

		if (target->player->powers[pw_super])
		{
			target->player->powers[pw_super] = 0;
			if (P_IsLocalPlayer(target->player))
			{
				music_stack_noposition = true;  
				music_stack_fadeout = MUSICRATE/2;  
			}
			P_RestoreMusic(target->player);

			if (!G_CoopGametype())
			{
				HU_SetCEchoFlags(0);
				HU_SetCEchoDuration(5);
				HU_DoCEcho(va("%s\\is no longer super.\\\\\\\\", player_names[target->player-players]));
			}
		}

		target->color = P_GetPlayerColor(target->player);
		target->colorized = false;
		G_GhostAddColor(GHC_NORMAL);

		if ((target->player->lives <= 1) && (netgame || multiplayer) && G_GametypeUsesCoopLives() && (cv_cooplives.value == 0))
			;
		else if ((!target->player->bot || target->player->bot == BOT_MPAI) && !target->player->spectator && (target->player->lives != INFLIVES)
		 && G_GametypeUsesLives())
		{
			if (!(target->player->pflags & PF_FINISHED))
				target->player->lives -= 1;  

			if (target->player->lives <= 0)  
			{
				boolean gameovermus = false;
				if ((netgame || multiplayer) && G_GametypeUsesCoopLives() && (cv_cooplives.value != 1))
				{
					INT32 i;
					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i])
							continue;

						if (players[i].lives > 0)
							break;
					}
					if (i == MAXPLAYERS)
						gameovermus = true;
				}
				else if (P_IsLocalPlayer(target->player))
					gameovermus = true;

				if (gameovermus)  
					S_ChangeMusicEx("_gover", 0, 0, 0, (2*MUSICRATE) - (MUSICRATE/25), 0);  


				if (!(netgame || multiplayer || demoplayback || demorecording || metalrecording || modeattacking) && numgameovers < maxgameovers)
				{
					numgameovers++;
					if (!usedCheats && cursaveslot > 0)
						G_SaveGameOver((UINT32)cursaveslot, (target->player->continues <= 0));
				}
			}
		}
		target->player->playerstate = PST_DEAD;

		if (target->player == &players[consoleplayer])
		{


			if (automapactive)
				AM_Stop();


			localaiming = 0;
		}
		if (target->player == &players[secondarydisplayplayer])
		{

			localaiming2 = 0;
		}


		if (G_TagGametype()
		 && !(target->player->pflags & PF_TAGIT) && (!source || !source->player) && !(target->player->spectator))
		{


			if (leveltime >= hidetime * TICRATE)
			{
				if (!(gametyperules & GTR_HIDEFROZEN)) 
				{
					target->player->pflags |= PF_TAGIT;
					CONS_Printf(M_GetText("%s is now IT!\n"), player_names[target->player-players]);  
					P_CheckSurvivors();
				}
				else
				{
					if (!(target->player->pflags & PF_GAMETYPEOVER))
					{


						INT32 w;

						for (w=0; w < MAXPLAYERS; w++)
						{
							if (players[w].pflags & PF_TAGIT)
								P_AddPlayerScore(&players[w], 100);
						}

						target->player->pflags |= PF_GAMETYPEOVER;
						CONS_Printf(M_GetText("%s was found!\n"), player_names[target->player-players]);
						P_CheckSurvivors();
					}
				}
			}
		}
	}

	if (source && target && target->player && source->player)
		P_PlayVictorySound(source);  


	switch(target->type)
	{
		case MT_BOUNCEPICKUP:
		case MT_RAILPICKUP:
		case MT_AUTOPICKUP:
		case MT_EXPLODEPICKUP:
		case MT_SCATTERPICKUP:
		case MT_GRENADEPICKUP:
			P_SetObjectMomZ(target, FRACUNIT, false);
			target->fuse = target->info->damage;
			break;

		case MT_BUGGLE:
			if (inflictor && inflictor->player  
			&& P_AproxDistance(inflictor->x - target->x, inflictor->y - target->y) <= inflictor->radius + target->radius + FixedMul(8*FRACUNIT, inflictor->scale)  
			&& inflictor->z <= target->z + target->height + FixedMul(8*FRACUNIT, inflictor->scale)
			&& inflictor->z + inflictor->height >= target->z - FixedMul(8*FRACUNIT, inflictor->scale))
				mo = P_SpawnMobj(inflictor->x + inflictor->momx, inflictor->y + inflictor->momy, inflictor->z + (inflictor->height / 2) + inflictor->momz, MT_EXTRALARGEBUBBLE);
			else
				mo = P_SpawnMobj(target->x, target->y, target->z, MT_EXTRALARGEBUBBLE);
			if (P_MobjWasRemoved(mo))
				break;
			P_SetScale(mo, target->scale, true);
			P_SetMobjState(mo, mo->info->raisestate);
			break;

		case MT_YELLOWSHELL:
			P_SpawnMobjFromMobj(target, 0, 0, 0, MT_YELLOWSPRING);
			break;

		case MT_CRAWLACOMMANDER:
			target->momx = target->momy = target->momz = 0;
			break;

		case MT_CRUSHSTACEAN:
			if (target->tracer)
			{
				mobj_t *chain = target->tracer->target, *chainnext;
				while (chain)
				{
					chainnext = chain->target;
					P_RemoveMobj(chain);
					chain = chainnext;
				}
				S_StopSound(target->tracer);
				P_KillMobj(target->tracer, inflictor, source, damagetype);
			}
			break;

		case MT_BANPYURA:
			if (target->tracer)
			{
				S_StopSound(target->tracer);
				P_KillMobj(target->tracer, inflictor, source, damagetype);
			}
			break;

		case MT_EGGSHIELD:
			P_SetObjectMomZ(target, 4*target->scale, false);
			P_InstaThrust(target, target->angle, 3*target->scale);
			target->flags = (target->flags|MF_NOCLIPHEIGHT) & ~MF_NOGRAVITY;
			break;

		case MT_DRAGONBOMBER:
			{
				mobj_t *segment = target;
				while (segment->tracer != NULL)
				{
					P_KillMobj(segment->tracer, NULL, NULL, 0);
					segment = segment->tracer;
				}
				break;
			}

		case MT_EGGMOBILE3:
			{
				mobj_t *mo2;
				thinker_t *th;
				UINT32 i = 0;  


				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->removing)
						continue;

					mo = (mobj_t *)th;
					if (mo->type != (mobjtype_t)target->info->mass)
						continue;
					if (mo->tracer != target)
						continue;

					P_KillMobj(mo, inflictor, source, damagetype);
					mo->destscale = mo->scale/8;
					mo->scalespeed = (mo->scale - mo->destscale)/(2*TICRATE);
					mo->momz = mo->info->speed;
					mo->angle = FixedAngle((P_RandomKey(36)*10)<<FRACBITS);

					mo2 = P_SpawnMobjFromMobj(mo, 0, 0, 0, MT_BOSSJUNK);
					if (!P_MobjWasRemoved(mo2))
					{
						mo2->angle = mo->angle;
						P_SetMobjState(mo2, S_BOSSSEBH2);
					}

					if (++i == 2)  
						break;
					else
						S_StartSound(mo, mo->info->deathsound);  
				}
			}
			break;

		case MT_BIGMINE:
			if (inflictor)
			{
				fixed_t dx = target->x - inflictor->x, dy = target->y - inflictor->y, dz = target->z - inflictor->z;
				fixed_t dm = FixedHypot(dz, FixedHypot(dy, dx));
				target->momx = FixedDiv(FixedDiv(dx, dm), dm)*512;
				target->momy = FixedDiv(FixedDiv(dy, dm), dm)*512;
				target->momz = FixedDiv(FixedDiv(dz, dm), dm)*512;
			}
			if (source)
				P_SetTarget(&target->tracer, source);
			break;

		case MT_BLASTEXECUTOR:
			if (target->spawnpoint)
				P_LinedefExecute(target->spawnpoint->args[0], (source ? source : inflictor), target->subsector->sector);
			break;

		case MT_SPINBOBERT:
			if (target->hnext)
				P_KillMobj(target->hnext, inflictor, source, damagetype);
			if (target->hprev)
				P_KillMobj(target->hprev, inflictor, source, damagetype);
			break;

		case MT_EGGTRAP:

			target->fuse = TICRATE;
			break;

		case MT_MINECART:
			A_Scream(target);
			target->momx = target->momy = target->momz = 0;
			if (target->target && target->target->health)
				P_KillMobj(target->target, target, source, 0);
			break;

		case MT_PLAYER:
			{
				target->fuse = TICRATE*3;  
				target->momx = target->momy = target->momz = 0;

				if (damagetype == DMG_DROWNED)  
				{
					target->movedir = damagetype;  
					if (target->player->charflags & SF_MACHINE)
						S_StartSound(target, sfx_fizzle);
					else
						S_StartSound(target, sfx_drown);

				}
				else
				{
					P_SetObjectMomZ(target, 14*FRACUNIT, false);
					if (damagetype == DMG_SPIKE)  
						S_StartSound(target, sfx_spkdth);
					else
						P_PlayDeathSound(target);
				}
			}
			break;
		case MT_METALSONIC_RACE:
			target->fuse = TICRATE*3;
			target->momx = target->momy = target->momz = 0;
			P_SetObjectMomZ(target, 14*FRACUNIT, false);
			target->flags = (target->flags & ~MF_NOGRAVITY)|(MF_NOCLIP|MF_NOCLIPTHING);
			break;
		default:
			break;
	}


	if (target->type == MT_SPIKE && target->info->deathstate != S_NULL)
	{
		const angle_t ang = ((inflictor) ? inflictor->angle : 0) + ANGLE_90;
		const fixed_t scale = target->scale;
		const fixed_t xoffs = P_ReturnThrustX(target, ang, 8*scale), yoffs = P_ReturnThrustY(target, ang, 8*scale);
		const UINT16 flip = (target->eflags & MFE_VERTICALFLIP);
		mobj_t *chunk;
		fixed_t momz;

		S_StartSound(target, target->info->deathsound);

		if (target->info->xdeathstate != S_NULL)
		{
			momz = 6*scale;
			if (flip)
				momz *= -1;
#define makechunk(angtweak, xmov, ymov) \
			do {\
			chunk = P_SpawnMobjFromMobj(target, 0, 0, 0, MT_SPIKE);\
			if (P_MobjWasRemoved(chunk))\
				break;\
			P_SetMobjState(chunk, target->info->xdeathstate);\
			chunk->health = 0;\
			chunk->angle = angtweak;\
			P_UnsetThingPosition(chunk);\
			chunk->flags = MF_NOCLIP;\
			chunk->x += xmov;\
			chunk->y += ymov;\
			P_SetThingPosition(chunk);\
			P_InstaThrust(chunk,chunk->angle, 4*scale);\
			chunk->momz = momz;\
			} while (0)

			makechunk(ang + ANGLE_180, -xoffs, -yoffs);
			makechunk(ang, xoffs, yoffs);

#undef makechunk
		}

		momz = 7*scale;
		if (flip)
			momz *= -1;

		chunk = P_SpawnMobjFromMobj(target, 0, 0, 0, MT_SPIKE);
		if (!P_MobjWasRemoved(chunk))
		{
			P_SetMobjState(chunk, target->info->deathstate);
			chunk->health = 0;
			chunk->angle = ang + ANGLE_180;
			P_UnsetThingPosition(chunk);
			chunk->flags = MF_NOCLIP;
			chunk->x -= xoffs;
			chunk->y -= yoffs;
			if (flip)
				chunk->z -= 12*scale;
			else
				chunk->z += 12*scale;
			P_SetThingPosition(chunk);
			P_InstaThrust(chunk, chunk->angle, 2*scale);
			chunk->momz = momz;
		}

		P_SetMobjState(target, target->info->deathstate);
		target->health = 0;
		target->angle = ang;
		P_UnsetThingPosition(target);
		target->flags = MF_NOCLIP;
		target->x += xoffs;
		target->y += yoffs;
		if (flip)
			target->z -= 12*scale;
		else
			target->z += 12*scale;
		P_SetThingPosition(target);
		P_InstaThrust(target, target->angle, 2*scale);
		target->momz = momz;
	}
	else if (target->type == MT_WALLSPIKE && target->info->deathstate != S_NULL)
	{
		const angle_t ang = ( target->angle) + ANGLE_90;
		const fixed_t scale = target->scale;
		const fixed_t xoffs = P_ReturnThrustX(target, ang, 8*scale), yoffs = P_ReturnThrustY(target, ang, 8*scale), forwardxoffs = P_ReturnThrustX(target, target->angle, 7*scale), forwardyoffs = P_ReturnThrustY(target, target->angle, 7*scale);
		const UINT16 flip = (target->eflags & MFE_VERTICALFLIP);
		mobj_t *chunk;
		boolean sprflip;

		S_StartSound(target, target->info->deathsound);
		if (!P_MobjWasRemoved(target->tracer))
			P_RemoveMobj(target->tracer);

		if (target->info->xdeathstate != S_NULL)
		{
			sprflip = P_RandomChance(FRACUNIT/2);

#define makechunk(angtweak, xmov, ymov) \
			do {\
				chunk = P_SpawnMobjFromMobj(target, 0, 0, 0, MT_WALLSPIKE);\
				if (P_MobjWasRemoved(chunk))\
					break;\
				P_SetMobjState(chunk, target->info->xdeathstate);\
				chunk->health = 0;\
				chunk->angle = target->angle;\
				P_UnsetThingPosition(chunk);\
				chunk->flags = MF_NOCLIP;\
				chunk->x += xmov - forwardxoffs;\
				chunk->y += ymov - forwardyoffs;\
				P_SetThingPosition(chunk);\
				P_InstaThrust(chunk, angtweak, 4*scale);\
				chunk->momz = P_RandomRange(5, 7)*scale;\
				if (flip)\
					chunk->momz *= -1;\
				if (sprflip)\
					chunk->frame |= FF_VERTICALFLIP;\
			} while (0)

			makechunk(ang + ANGLE_180, -xoffs, -yoffs);
			sprflip = !sprflip;
			makechunk(ang, xoffs, yoffs);

#undef makechunk
		}

		sprflip = P_RandomChance(FRACUNIT/2);

		chunk = P_SpawnMobjFromMobj(target, 0, 0, 0, MT_WALLSPIKE);
		if (!P_MobjWasRemoved(chunk))
		{
			P_SetMobjState(chunk, target->info->deathstate);
			chunk->health = 0;
			chunk->angle = target->angle;
			P_UnsetThingPosition(chunk);
			chunk->flags = MF_NOCLIP;
			chunk->x += forwardxoffs - xoffs;
			chunk->y += forwardyoffs - yoffs;
			P_SetThingPosition(chunk);
			P_InstaThrust(chunk, ang + ANGLE_180, 2*scale);
			chunk->momz = P_RandomRange(5, 7)*scale;
			if (flip)
				chunk->momz *= -1;
			if (sprflip)
				chunk->frame |= FF_VERTICALFLIP;
		}

		P_SetMobjState(target, target->info->deathstate);
		target->health = 0;
		P_UnsetThingPosition(target);
		target->flags = MF_NOCLIP;
		target->x += forwardxoffs + xoffs;
		target->y += forwardyoffs + yoffs;
		P_SetThingPosition(target);
		P_InstaThrust(target, ang, 2*scale);
		target->momz = P_RandomRange(5, 7)*scale;
		if (flip)
			target->momz *= -1;
		if (!sprflip)
			target->frame |= FF_VERTICALFLIP;
	}
	else if (target->player)
	{
		if (damagetype == DMG_DROWNED || damagetype == DMG_SPACEDROWN)
			P_SetMobjState(target, target->info->xdeathstate);
		else
			P_SetMobjState(target, target->info->deathstate);
	}
	else
#ifdef DEBUG_NULL_DEATHSTATE
		P_SetMobjState(target, S_NULL);
#else
		P_SetMobjState(target, target->info->deathstate);
#endif

	                                                                                            
                     
                       
}

static void P_NiGHTSDamage(mobj_t *target, mobj_t *source)
{
	player_t *player = target->player;
	tic_t oldnightstime = player->nightstime;

	(void)source;  

	if (!player->powers[pw_flashing])
	{
		angle_t fa;

		player->angle_pos = player->old_angle_pos;
		player->speed /= 5;
		player->flyangle += 180;  
		player->flyangle %= 360;

		if (gametyperules & GTR_RACE)
			player->drillmeter -= 5*20;
		else
		{
			if (player->nightstime > 5*TICRATE)
				player->nightstime -= 5*TICRATE;
			else
				player->nightstime = 1;
		}

		if (player->pflags & PF_TRANSFERTOCLOSEST)
		{
			target->momx = -target->momx;
			target->momy = -target->momy;
		}
		else
		{
			fa = player->old_angle_pos>>ANGLETOFINESHIFT;

			target->momx = FixedMul(FINECOSINE(fa),target->target->radius);
			target->momy = FixedMul(FINESINE(fa),target->target->radius);
		}

		player->powers[pw_flashing] = flashingtics;
		P_SetMobjState(target, S_PLAY_NIGHTS_STUN);
		S_StartSound(target, sfx_nghurt);

		player->mo->spriteroll = 0;

		if (oldnightstime > 10*TICRATE
			&& player->nightstime < 10*TICRATE)
		{
			if ((mapheaderinfo[gamemap-1]->levelflags & LF_MIXNIGHTSCOUNTDOWN)
#ifdef _WIN32

				&& S_MusicType() != MU_MID
#endif
			)
			{
				S_FadeMusic(0, 10*MUSICRATE);
				S_StartSound(NULL, sfx_timeup);  
			}
			else
				P_PlayJingle(player, ((maptol & TOL_NIGHTS) && !G_IsSpecialStage(gamemap)) ? JT_NIGHTSTIMEOUT : JT_SSTIMEOUT);
		}
	}
}

static boolean P_TagDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{
	player_t *player = target->player;
	(void)damage;  


	if (player->powers[pw_flashing] || player->powers[pw_invulnerability])
		return false;


	if (leveltime <= hidetime * TICRATE)
		return false;


	if ((player->pflags & PF_TAGIT && !((cv_friendlyfire.value || (gametyperules & GTR_FRIENDLYFIRE) || (damagetype & DMG_CANHURTSELF)) &&
		source && source->player && source->player->pflags & PF_TAGIT)))
	{
		if (inflictor->type == MT_LHRT && !(player->powers[pw_shield] & SH_NOSTACK))
		{
			if (player->revitem != MT_LHRT && player->spinitem != MT_LHRT && player->thokitem != MT_LHRT)  
			{
				P_SwitchShield(player, SH_PINK);
				S_StartSound(target, mobjinfo[MT_PITY_ICON].seesound);
			}
		}
		return false;
	}



	if (!(cv_friendlyfire.value || (gametyperules & GTR_FRIENDLYFIRE) || (damagetype & DMG_CANHURTSELF)) && (player->pflags & PF_TAGIT) == (source->player->pflags & PF_TAGIT))
	{
		if (inflictor->type == MT_LHRT && !(player->powers[pw_shield] & SH_NOSTACK))
		{
			if (player->revitem != MT_LHRT && player->spinitem != MT_LHRT && player->thokitem != MT_LHRT)  
			{
				P_SwitchShield(player, SH_PINK);
				S_StartSound(target, mobjinfo[MT_PITY_ICON].seesound);
			}
		}
		else if (!(inflictor->flags & MF_FIRE))
			P_GivePlayerRings(player, 1);
		if (inflictor->flags2 & MF2_BOUNCERING)
			inflictor->fuse = 0;  
		return false;
	}

	if (inflictor->type == MT_LHRT)
		return false;


	if (source->player->pflags & PF_TAGIT && !(player->pflags & PF_TAGIT))
	{
		P_AddPlayerScore(source->player, 100);  
		P_HitDeathMessages(player, inflictor, source, 0);

		if (!(gametyperules & GTR_HIDEFROZEN))  
		{
			player->pflags |= PF_TAGIT;  
			CONS_Printf(M_GetText("%s is now IT!\n"), player_names[player-players]);  
		}
		else
		{
			player->pflags |= PF_GAMETYPEOVER;  
			CONS_Printf(M_GetText("%s was found!\n"), player_names[player-players]);  
		}


		P_CheckSurvivors();
	}

	P_DoPlayerPain(player, source, inflictor);


	if (player->powers[pw_shield])
	{
		P_RemoveShield(player);
		S_StartSound(target, sfx_shldls);
		return true;
	}

	if (player->powers[pw_carry] == CR_NIGHTSFALL)
	{
		if (player->spheres > 0)
		{
			P_PlayRinglossSound(target);
			P_PlayerRingBurst(player, player->spheres);
			player->spheres = 0;
		}
	}
	else if (player->rings > 0)  
	{
		P_PlayRinglossSound(target);
		P_PlayerRingBurst(player, player->rings);
		player->rings = 0;
	}
	else  
	{
		P_PlayDeathSound(target);
		P_PlayVictorySound(source);  
	}
	return true;
}

static boolean P_PlayerHitsPlayer(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{
	player_t *player = target->player;

	if (!(damagetype & DMG_CANHURTSELF))
	{

		if (source == target)
			return false;


		if (!(cv_friendlyfire.value || (gametyperules & GTR_FRIENDLYFIRE)) && (gametyperules & GTR_FRIENDLY))
		{
			if ((gametyperules & GTR_FRIENDLY) && inflictor->type == MT_LHRT && !(player->powers[pw_shield] & SH_NOSTACK))  
			{
				if (player->revitem != MT_LHRT && player->spinitem != MT_LHRT && player->thokitem != MT_LHRT)  
				{
					P_SwitchShield(player, SH_PINK);
					S_StartSound(target, mobjinfo[MT_PITY_ICON].seesound);
				}
			}
			return false;
		}
	}


	if (G_TagGametype())
		return P_TagDamage(target, inflictor, source, damage, damagetype);
	else if (damagetype & DMG_CANHURTSELF)
		return true;
	else if (G_GametypeHasTeams())  
	{


		if (!(cv_friendlyfire.value || (gametyperules & GTR_FRIENDLYFIRE)) && target->player->ctfteam == source->player->ctfteam)
		{
			if (inflictor->type == MT_LHRT && !(player->powers[pw_shield] & SH_NOSTACK))
			{
				if (player->revitem != MT_LHRT && player->spinitem != MT_LHRT && player->thokitem != MT_LHRT)  
				{
					P_SwitchShield(player, SH_PINK);
					S_StartSound(target, mobjinfo[MT_PITY_ICON].seesound);
				}
			}
			else if (!(inflictor->flags & MF_FIRE))
				P_GivePlayerRings(target->player, 1);
			if (inflictor->flags2 & MF2_BOUNCERING)
				inflictor->fuse = 0;  

			return false;
		}
	}

	if (inflictor->type == MT_LHRT)
		return false;


	if (!player->powers[pw_flashing] && !player->powers[pw_invulnerability] && !player->powers[pw_super] && !(player->powers[pw_strong] & STR_GUARD)
	&& source->player->score > player->score)
		player->pity++;

	return true;
}

static void P_KillPlayer(player_t *player, mobj_t *source, INT32 damage)
{
	player->pflags &= ~PF_SLIDING;

	player->powers[pw_carry] = CR_NONE;


	if (source)
	{
		if ((gametyperules & GTR_RINGSLINGER) && !(gametyperules & GTR_TAG))
			P_PlayerRingBurst(player, player->rings);
		if (gametyperules & GTR_POWERSTONES)
			P_PlayerEmeraldBurst(player, false);
	}


	player->powers[pw_shield] = SH_NONE;
	player->mo->color = P_GetPlayerColor(player);


	player->powers[pw_emeralds] = 0;

	P_ForceFeed(player, 40, 10, TICRATE, 40 + min(damage, 100)*2);

	P_ResetPlayer(player);

	if (!player->spectator)
		player->mo->flags2 &= ~MF2_DONTDRAW;

	P_SetMobjState(player->mo, player->mo->info->deathstate);
	if ((gametyperules & GTR_TEAMFLAGS) && (player->gotflag & (GF_REDFLAG|GF_BLUEFLAG)))
	{
		P_PlayerFlagBurst(player, false);
		if (source && source->player)
		{

			if (!G_GametypeHasTeams() || !(source->player->ctfteam == player->ctfteam && source != player->mo))
				P_AddPlayerScore(source->player, 25);
		}
	}
	if (source && source->player && !player->powers[pw_super])  
	{

		if (!G_GametypeHasTeams() || !(source->player->ctfteam == player->ctfteam && source != player->mo))
			P_AddPlayerScore(source->player, 100);
	}


	if (!G_CoopGametype() && player->powers[pw_super])
	{
		S_StartSound(NULL, sfx_s3k66);  
		HU_SetCEchoFlags(0);
		HU_SetCEchoDuration(5);
		HU_DoCEcho(va("%s\\is no longer super.\\\\\\\\", player_names[player-players]));
	}
}

static void P_SuperDamage(player_t *player, mobj_t *inflictor, mobj_t *source, INT32 damage)
{
	fixed_t fallbackspeed;
	angle_t ang;

	P_ForceFeed(player, 40, 10, TICRATE, 40 + min(damage, 100)*2);

	if (player->mo->eflags & MFE_VERTICALFLIP)
		player->mo->z--;
	else
		player->mo->z++;

	if (player->mo->eflags & MFE_UNDERWATER)
		P_SetObjectMomZ(player->mo, FixedDiv(10511*FRACUNIT,2600*FRACUNIT), false);
	else
		P_SetObjectMomZ(player->mo, FixedDiv(69*FRACUNIT,10*FRACUNIT), false);

	ang = R_PointToAngle2(inflictor->x,	inflictor->y, player->mo->x, player->mo->y);



	if (inflictor->flags2 & MF2_SCATTER && source)
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
			fallbackspeed = FixedMul(28*FRACUNIT, inflictor->scale);  
		else
			fallbackspeed = FixedMul(20*FRACUNIT, inflictor->scale);  
	}
	else if (inflictor->flags2 & MF2_RAILRING)
		fallbackspeed = FixedMul(16*FRACUNIT, inflictor->scale);  
	else
		fallbackspeed = FixedMul(4*FRACUNIT, inflictor->scale);  

	P_InstaThrust(player->mo, ang, fallbackspeed);

	P_SetMobjState(player->mo, S_PLAY_STUN);

	P_ResetPlayer(player);

	if (player->timeshit != UINT8_MAX)
		++player->timeshit;
}

void P_RemoveShield(player_t *player)
{
	if (player->powers[pw_shield] & SH_FORCE)
	{  
		if (player->powers[pw_shield] & SH_FORCEHP)
			player->powers[pw_shield]--;
		else
			player->powers[pw_shield] &= SH_STACK;
	}
	else if (player->powers[pw_shield] & SH_NOSTACK)
	{  
		if ((player->powers[pw_shield] & SH_NOSTACK) == SH_ARMAGEDDON)  
		{
			P_BlackOw(player);  
			player->pflags |= PF_JUMPDOWN;
		}
		else
			player->powers[pw_shield] &= SH_STACK;
	}
	else
	{  
		if (((player->powers[pw_shield] & SH_STACK) == SH_FIREFLOWER) && !(player->powers[pw_super] || (mariomode && player->powers[pw_invulnerability])))
		{
			player->mo->color = P_GetPlayerColor(player);
			G_GhostAddColor(GHC_NORMAL);
		}
		player->powers[pw_shield] = SH_NONE;
	}
}

static void P_ShieldDamage(player_t *player, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{

	P_DoPlayerPain(player, source, inflictor);

	P_RemoveShield(player);

	P_ForceFeed(player, 40, 10, TICRATE, 40 + min(damage, 100)*2);

	if (damagetype == DMG_SPIKE)  
		S_StartSound(player->mo, sfx_spkdth);
	else
		S_StartSound (player->mo, sfx_shldls);  

	if ((gametyperules & GTR_TEAMFLAGS) && (player->gotflag & (GF_REDFLAG|GF_BLUEFLAG)))
	{
		P_PlayerFlagBurst(player, false);
		if (source && source->player)
		{

			if (!G_GametypeHasTeams() || !(source->player->ctfteam == player->ctfteam && source != player->mo))
				P_AddPlayerScore(source->player, 25);
		}
	}
	if (source && source->player && !player->powers[pw_super])  
	{

		if (!G_GametypeHasTeams() || !(source->player->ctfteam == player->ctfteam && source != player->mo))
			P_AddPlayerScore(source->player, 50);
	}
}

static void P_RingDamage(player_t *player, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype, boolean dospheres)
{
	P_DoPlayerPain(player, source, inflictor);

	P_ForceFeed(player, 40, 10, TICRATE, 40 + min(damage, 100)*2);

	if (damagetype == DMG_SPIKE)  
		S_StartSound(player->mo, sfx_spkdth);

	if (source && source->player && !player->powers[pw_super])  
	{

		if (!G_GametypeHasTeams() || !(source->player->ctfteam == player->ctfteam && source != player->mo))
			P_AddPlayerScore(source->player, 50);
	}

	if ((gametyperules & GTR_TEAMFLAGS) && (player->gotflag & (GF_REDFLAG|GF_BLUEFLAG)))
	{
		P_PlayerFlagBurst(player, false);
		if (source && source->player)
		{

			if (!G_GametypeHasTeams() || !(source->player->ctfteam == player->ctfteam && source != player->mo))
				P_AddPlayerScore(source->player, 25);
		}
	}


	P_PlayRinglossSound(player->mo);  
	P_PlayerRingBurst(player, damage);

	if (dospheres)
	{
		player->spheres -= damage;
		if (player->spheres < 0)
			player->spheres = 0;
	}
	else
	{
		player->rings -= damage;
		if (player->rings < 0)
			player->rings = 0;
	}
}








void P_SpecialStageDamage(player_t *player, mobj_t *inflictor, mobj_t *source)
{
	tic_t oldnightstime = player->nightstime;

	if (player->powers[pw_invulnerability] || player->powers[pw_flashing] || player->powers[pw_super])
		return;

	if (!cv_friendlyfire.value && source && source->player)
	{
		if (inflictor->type == MT_LHRT && !(player->powers[pw_shield] & SH_NOSTACK))
		{
			if (player->revitem != MT_LHRT && player->spinitem != MT_LHRT && player->thokitem != MT_LHRT)  
			{
				P_SwitchShield(player, SH_PINK);
				S_StartSound(player->mo, mobjinfo[MT_PITY_ICON].seesound);
			}
		}

		if (source->player->ctfteam == player->ctfteam)
			return;
	}

	if (inflictor && inflictor->type == MT_LHRT)
		return;

	if (player->powers[pw_shield] || (player->bot && player->bot != BOT_MPAI))   
	{
		P_RemoveShield(player);
		S_StartSound(player->mo, sfx_shldls);  
	}
	else
	{
		S_StartSound(player->mo, sfx_nghurt);
		if (player->nightstime > 5*TICRATE)
			player->nightstime -= 5*TICRATE;
		else
			player->nightstime = 1;
	}

	P_DoPlayerPain(player, inflictor, source);

	if ((gametyperules & GTR_TEAMFLAGS) && player->gotflag & (GF_REDFLAG|GF_BLUEFLAG))
		P_PlayerFlagBurst(player, false);

	if (oldnightstime > 10*TICRATE
		&& player->nightstime < 10*TICRATE)
	{
		if (mapheaderinfo[gamemap-1]->levelflags & LF_MIXNIGHTSCOUNTDOWN)
		{
			S_FadeMusic(0, 10*MUSICRATE);
			S_StartSound(NULL, sfx_timeup);  
		}
		else
			S_ChangeMusicInternal((((maptol & TOL_NIGHTS) && !G_IsSpecialStage(gamemap)) ? "_ntime" : "_drown"), false);
	}
}

                                                        
                                                         
   
                                               
                                                                          
                                                                        
                                                                      
                                                                           
                                                                              
                                                                       
                                                         
                                                   
                                                                                                  
                                                                 
                                                            
                  
    
boolean P_DamageMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{
	player_t *player;
	boolean force = false;

	if (objectplacing)
		return false;

	if (target->health <= 0)
		return false;


	if (multiplayer)
	{
		if (damagetype != DMG_SPECTATOR && target->player && target->player->spectator)
			return false;

		if (source && source->player && source->player->spectator)
			return false;
	}


	if (!metalrecording)
	{
		UINT8 shouldForce = LUA_HookShouldDamage(target, inflictor, source, damage, damagetype);
		if (P_MobjWasRemoved(target))
			return (shouldForce == 1);  
		if (P_MobjWasRemoved(source))
			source = NULL;
		if (shouldForce == 1)
			force = true;
		else if (shouldForce == 2)
			return false;
	}

	if (!force)
	{
		if (!(target->flags & MF_SHOOTABLE))
			return false;  

		if (target->type == MT_BLACKEGGMAN)
			return false;


		if (target->flags & MF_MONITOR && ((!source || !source->player || (source->player->bot && source->player->bot != BOT_MPAI))
		|| (inflictor && (inflictor->type == MT_REDRING || (inflictor->type >= MT_THROWNBOUNCE && inflictor->type <= MT_THROWNGRENADE)))))
			return false;
	}

	if (target->flags2 & MF2_SKULLFLY)
		target->momx = target->momy = target->momz = 0;

	if (!force)
	{

		if (target->type == MT_RING_REDBOX && !(source->player->ctfteam == 1))
			return false;

		if (target->type == MT_RING_BLUEBOX && !(source->player->ctfteam == 2))
			return false;
	}

	if (target->flags & (MF_ENEMY|MF_BOSS))
	{
		if (!force && target->flags2 & MF2_FRET)  
			return false;

		if (LUA_HookMobjDamage(target, inflictor, source, damage, damagetype) || P_MobjWasRemoved(target))
			return true;

		if (target->health > 1)
			target->flags2 |= MF2_FRET;
	}

	player = target->player;

	if (player)  
	{
		if (!force)
		{
			if (player->exiting)
				return false;

			if (player->pflags & PF_GODMODE)
				return false;

			if ((maptol & TOL_NIGHTS) && target->player->powers[pw_carry] != CR_NIGHTSMODE && target->player->powers[pw_carry] != CR_NIGHTSFALL)
				return false;

			switch (damagetype)
			{
#define DAMAGECASE(type)\
				case DMG_##type:\
					if (player->powers[pw_shield] & SH_PROTECT##type)\
						return false;\
					break
				DAMAGECASE(WATER);
				DAMAGECASE(FIRE);
				DAMAGECASE(ELECTRIC);
				DAMAGECASE(SPIKE);
#undef DAMAGECASE
				default:
					break;
			}
		}

		if (player->powers[pw_carry] == CR_NIGHTSMODE)  
		{
			if (player->powers[pw_flashing])
				return false;
			if (!force)
			{
				if (source == target)
					return false;  
				if (source && source->player && !(cv_friendlyfire.value || (gametyperules & GTR_FRIENDLYFIRE))
				&& ((gametyperules & GTR_FRIENDLY)
				|| (G_GametypeHasTeams() && player->ctfteam == source->player->ctfteam)))
					return false;  
			}
			if (LUA_HookMobjDamage(target, inflictor, source, damage, damagetype))
				return true;
			P_NiGHTSDamage(target, source);  
			return true;
		}

		if (G_IsSpecialStage(gamemap) && !(damagetype & DMG_DEATHMASK))
		{
			if (player->powers[pw_flashing])
				return false;
			if (LUA_HookMobjDamage(target, inflictor, source, damage, damagetype))
				return true;
			P_SpecialStageDamage(player, inflictor, source);
			return true;
		}

		if (!force && inflictor && inflictor->flags & MF_FIRE && !(damagetype && damagetype != DMG_FIRE))
		{
			if (player->powers[pw_shield] & SH_PROTECTFIRE)
				return false;  

			if (G_PlatformGametype() && inflictor && source && source->player)
				return false;  
		}


		if (!force && source && source->player)
		{
			if (!P_PlayerHitsPlayer(target, inflictor, source, damage, damagetype))
				return false;
		}


		if (damagetype & DMG_DEATHMASK)
			P_KillPlayer(player, source, damage);
		else if (metalrecording)
		{
			if (!inflictor)
				inflictor = source;
			if (inflictor && inflictor->flags & MF_ENEMY)
			{  
				P_KillMobj(inflictor, NULL, target, damagetype);
				return false;
			}
			else if (inflictor && inflictor->flags & MF_MISSILE)
				return false;  
			else if (!player->powers[pw_flashing])
			{  
				P_ShieldDamage(player, inflictor, source, damage, damagetype);
				return true;
			}
			return false;
		}
		else if (player->powers[pw_invulnerability] || player->powers[pw_flashing] || player->powers[pw_super] || (player->powers[pw_strong] & STR_GUARD))  
		{
			if (force
			|| (inflictor && inflictor->flags & MF_MISSILE && inflictor->flags2 & MF2_SUPERFIRE))  
			{
				if (!LUA_HookMobjDamage(target, inflictor, source, damage, damagetype))
					P_SuperDamage(player, inflictor, source, damage);
				return true;
			}
			return false;
		}
		else if (LUA_HookMobjDamage(target, inflictor, source, damage, damagetype))
			return true;
		else if (player->powers[pw_shield] || (player->bot && player->bot != BOT_MPAI && !ultimatemode))   
		{
			P_ShieldDamage(player, inflictor, source, damage, damagetype);
			damage = 0;
		}
		else if (player->powers[pw_carry] == CR_NIGHTSFALL)
		{

			damage = player->spheres;
			P_RingDamage(player, inflictor, source, damage, damagetype, true);
			damage = 0;
		}
		else if (player->rings > 0)  
		{
			damage = player->rings;
			P_RingDamage(player, inflictor, source, damage, damagetype, false);
			damage = 0;
		}


		else if (!force && G_GametypeHasTeams()
			&& source && source->player && (source->player->ctfteam == player->ctfteam)
			&& (cv_friendlyfire.value || (gametyperules & GTR_FRIENDLYFIRE)))
		{
			damage = 0;
			P_ShieldDamage(player, inflictor, source, damage, damagetype);
		}
		else  
		{
			damage = 1;
			P_KillPlayer(player, source, damage);
		}

		P_ForceFeed(player, 40, 10, TICRATE, 40 + min(damage, 100)*2);
	}



	if (cv_killingdead.value && (source && source->player) && (inflictor && inflictor->player) && P_RandomChance(5*FRACUNIT/16))
		P_DamageMobj(source, target, target, 1, 0);


	if (damagetype & DMG_DEATHMASK)
		target->health = 0;
	else
		target->health -= damage;

	if (player)
		P_HitDeathMessages(player, inflictor, source, damagetype);

	if (source && source->player && target)
		G_GhostAddHit(target);

	if (target->health <= 0)
	{
		P_KillMobj(target, inflictor, source, damagetype);
		return true;
	}

	if (player)
		P_ResetPlayer(target->player);
	else if ((target->type == MT_EGGMOBILE2)  
	&& (target->health < target->info->damage))  
		P_SetMobjState(target, target->info->meleestate);  
	else
		P_SetMobjState(target, target->info->painstate);

	if (P_MobjWasRemoved(target))
		return false;

	if (target->type == MT_HIVEELEMENTAL)
		target->extravalue1 += 3;

	target->reactiontime = 0;  

	if (source && source != target)
	{


		P_SetTarget(&target->target, source);
	}

	return true;
}

                                     
   
                                                    
                                                                        
                             
                         
    
void P_PlayerRingBurst(player_t *player, INT32 num_rings)
{
	INT32 i;
	mobj_t *mo;
	angle_t fa, va;
	fixed_t ns;
	fixed_t z;
	boolean nightsreplace = ((maptol & TOL_NIGHTS) && !G_IsSpecialStage(gamemap));


	if (!player)
		return;


	if (((maptol & TOL_NIGHTS) && player->spheres <= 0) || (!(maptol & TOL_NIGHTS) && player->rings <= 0))
		num_rings = 0;

	if (num_rings > 32 && player->powers[pw_carry] != CR_NIGHTSFALL)
		num_rings = 32;

	if (player->powers[pw_emeralds])
		P_PlayerEmeraldBurst(player, false);


	P_PlayerWeaponPanelOrAmmoBurst(player);

	if (abs(player->mo->momx) > player->mo->scale || abs(player->mo->momy) > player->mo->scale)
		va = R_PointToAngle2(player->mo->momx, player->mo->momy, 0, 0)>>ANGLETOFINESHIFT;
	else
		va = player->mo->angle>>ANGLETOFINESHIFT;

	for (i = 0; i < num_rings; i++)
	{
		INT32 objType = mobjinfo[MT_RING].reactiontime;
		if (mariomode)
			objType = mobjinfo[MT_COIN].reactiontime;
		else if (player->powers[pw_carry] == CR_NIGHTSFALL)
			objType = mobjinfo[(nightsreplace ? MT_NIGHTSCHIP : MT_BLUESPHERE)].reactiontime;

		z = player->mo->z;
		if (player->mo->eflags & MFE_VERTICALFLIP)
			z += player->mo->height - mobjinfo[objType].height;

		mo = P_SpawnMobj(player->mo->x, player->mo->y, z, objType);
		if (P_MobjWasRemoved(mo))
			continue;

		mo->fuse = 8*TICRATE;
		P_SetTarget(&mo->target, player->mo);

		P_SetScale(mo, player->mo->scale, true);


		fa = ((i*FINEANGLES/16) + va - ((num_rings-1)*FINEANGLES/32)) & FINEMASK;



		if (player->powers[pw_carry] == CR_NIGHTSFALL)
		{
			ns = FixedMul(((i*FRACUNIT)/16)+2*FRACUNIT, mo->scale);
			mo->momx = FixedMul(FINECOSINE(fa),ns);

			if (!(twodlevel || (player->mo->flags2 & MF2_TWOD)))
				mo->momy = FixedMul(FINESINE(fa),ns);

			P_SetObjectMomZ(mo, 8*FRACUNIT, false);
			mo->fuse = 20*TICRATE;  


			P_SetMobjState(mo, (player->bonustime ? mo->info->raisestate : mo->info->spawnstate));
		}
		else
		{
			fixed_t momxy, momz;  

			if (i > 15)
			{
				momxy = 3*FRACUNIT;
				momz = 4*FRACUNIT;
			}
			else
			{
				momxy = 2*FRACUNIT;
				momz = 3*FRACUNIT;
			}

			ns = FixedMul(FixedMul(momxy, FRACUNIT + FixedDiv(player->losstime<<FRACBITS, 10*TICRATE<<FRACBITS)), mo->scale);
			mo->momx = FixedMul(FINECOSINE(fa),ns);

			if (!(twodlevel || (player->mo->flags2 & MF2_TWOD)))
				mo->momy = FixedMul(FINESINE(fa),ns);

			ns = FixedMul(momz, FRACUNIT + FixedDiv(player->losstime<<FRACBITS, 10*TICRATE<<FRACBITS));
			P_SetObjectMomZ(mo, ns, false);

			if (i & 1)
				P_SetObjectMomZ(mo, ns, true);
		}
		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			mo->momz *= -1;
			mo->flags2 |= MF2_OBJECTFLIP;
		}
	}

	player->losstime += 10*TICRATE;

	return;
}

void P_PlayerWeaponPanelBurst(player_t *player)
{
	mobj_t *mo;
	angle_t fa;
	fixed_t ns;
	INT32 i;
	fixed_t z;

	INT32 num_weapons = M_CountBits((UINT32)player->ringweapons, NUM_WEAPONS-1);
	UINT16 ammoamt = 0;

	for (i = 0; i < num_weapons; i++)
	{
		mobjtype_t weptype = 0;
		powertype_t power = 0;

		if (player->ringweapons & RW_BOUNCE)  
		{
			weptype = MT_BOUNCEPICKUP;
			player->ringweapons &= ~RW_BOUNCE;
			power = pw_bouncering;
		}
		else if (player->ringweapons & RW_RAIL)  
		{
			weptype = MT_RAILPICKUP;
			player->ringweapons &= ~RW_RAIL;
			power = pw_railring;
		}
		else if (player->ringweapons & RW_AUTO)  
		{
			weptype = MT_AUTOPICKUP;
			player->ringweapons &= ~RW_AUTO;
			power = pw_automaticring;
		}
		else if (player->ringweapons & RW_EXPLODE)  
		{
			weptype = MT_EXPLODEPICKUP;
			player->ringweapons &= ~RW_EXPLODE;
			power = pw_explosionring;
		}
		else if (player->ringweapons & RW_SCATTER)  
		{
			weptype = MT_SCATTERPICKUP;
			player->ringweapons &= ~RW_SCATTER;
			power = pw_scatterring;
		}
		else if (player->ringweapons & RW_GRENADE)  
		{
			weptype = MT_GRENADEPICKUP;
			player->ringweapons &= ~RW_GRENADE;
			power = pw_grenadering;
		}

		if (!weptype)  
			continue;

		if (player->powers[power] >= mobjinfo[weptype].reactiontime)
			ammoamt = (UINT16)mobjinfo[weptype].reactiontime;
		else
			ammoamt = player->powers[power];

		player->powers[power] -= ammoamt;

		z = player->mo->z;
		if (player->mo->eflags & MFE_VERTICALFLIP)
			z += player->mo->height - mobjinfo[weptype].height;

		mo = P_SpawnMobj(player->mo->x, player->mo->y, z, weptype);
		if (P_MobjWasRemoved(mo))
			continue;

		mo->reactiontime = ammoamt;
		mo->flags2 |= MF2_DONTRESPAWN;
		mo->flags &= ~(MF_NOGRAVITY|MF_NOCLIPHEIGHT);
		P_SetTarget(&mo->target, player->mo);
		mo->fuse = 12*TICRATE;
		P_SetScale(mo, player->mo->scale, true);


		fa = ((i*FINEANGLES/16) + (player->mo->angle>>ANGLETOFINESHIFT)) & FINEMASK;





		ns = FixedMul(3*FRACUNIT, mo->scale);
		mo->momx = FixedMul(FINECOSINE(fa),ns);

		if (!(twodlevel || (player->mo->flags2 & MF2_TWOD)))
			mo->momy = FixedMul(FINESINE(fa),ns);

		P_SetObjectMomZ(mo, 4*FRACUNIT, false);

		if (i & 1)
			P_SetObjectMomZ(mo, 4*FRACUNIT, true);
	}
}

void P_PlayerWeaponAmmoBurst(player_t *player)
{
	mobj_t *mo;
	angle_t fa;
	fixed_t ns;
	INT32 i;
	fixed_t z;

	mobjtype_t weptype = 0;
	powertype_t power = 0;

	for (i = 0;; i++)
	{
		if (player->powers[pw_bouncering])
		{
			weptype = MT_BOUNCERING;
			power = pw_bouncering;
		}
		else if (player->powers[pw_railring])
		{
			weptype = MT_RAILRING;
			power = pw_railring;
		}
		else if (player->powers[pw_infinityring])
		{
			weptype = MT_INFINITYRING;
			power = pw_infinityring;
		}
		else if (player->powers[pw_automaticring])
		{
			weptype = MT_AUTOMATICRING;
			power = pw_automaticring;
		}
		else if (player->powers[pw_explosionring])
		{
			weptype = MT_EXPLOSIONRING;
			power = pw_explosionring;
		}
		else if (player->powers[pw_scatterring])
		{
			weptype = MT_SCATTERRING;
			power = pw_scatterring;
		}
		else if (player->powers[pw_grenadering])
		{
			weptype = MT_GRENADERING;
			power = pw_grenadering;
		}
		else
			break;  

		z = player->mo->z;
		if (player->mo->eflags & MFE_VERTICALFLIP)
			z += player->mo->height - mobjinfo[weptype].height;

		mo = P_SpawnMobj(player->mo->x, player->mo->y, z, weptype);
		if (P_MobjWasRemoved(mo))
			continue;
		mo->health = player->powers[power];
		mo->flags2 |= MF2_DONTRESPAWN;
		mo->flags &= ~(MF_NOGRAVITY|MF_NOCLIPHEIGHT);
		P_SetTarget(&mo->target, player->mo);

		player->powers[power] = 0;
		mo->fuse = 12*TICRATE;

		P_SetScale(mo, player->mo->scale, true);


		fa = ((i*FINEANGLES/16) + (player->mo->angle>>ANGLETOFINESHIFT)) & FINEMASK;


		ns = FixedMul(2*FRACUNIT, mo->scale);
		mo->momx = FixedMul(FINECOSINE(fa), ns);

		if (!(twodlevel || (player->mo->flags2 & MF2_TWOD)))
			mo->momy = FixedMul(FINESINE(fa),ns);

		P_SetObjectMomZ(mo, 3*FRACUNIT, false);

		if (i & 1)
			P_SetObjectMomZ(mo, 3*FRACUNIT, true);
	}
}

void P_PlayerWeaponPanelOrAmmoBurst(player_t *player)
{
	mobj_t *mo;
	angle_t fa;
	fixed_t ns;
	INT32 i = 0;
	fixed_t z;

	#define SETUP_DROP(thingtype) \
		z = player->mo->z; \
		if (player->mo->eflags & MFE_VERTICALFLIP) \
			z += player->mo->height - mobjinfo[thingtype].height; \
		fa = ((i*FINEANGLES/16) + (player->mo->angle>>ANGLETOFINESHIFT)) & FINEMASK; \
		ns = FixedMul(3*FRACUNIT, player->mo->scale); \

	#define DROP_WEAPON(rwflag, pickup, ammo, power) \
	if (player->ringweapons & rwflag) \
	{ \
		player->ringweapons &= ~rwflag; \
		SETUP_DROP(pickup) \
		mo = P_SpawnMobj(player->mo->x, player->mo->y, z, pickup); \
		if (!P_MobjWasRemoved(mo)) \
		{ \
			mo->reactiontime = 0; \
			mo->flags2 |= MF2_DONTRESPAWN; \
			mo->flags &= ~(MF_NOGRAVITY|MF_NOCLIPHEIGHT); \
			P_SetTarget(&mo->target, player->mo); \
			mo->fuse = 12*TICRATE; \
			P_SetScale(mo, player->mo->scale, true); \
			mo->momx = FixedMul(FINECOSINE(fa),ns); \
			if (!(twodlevel || (player->mo->flags2 & MF2_TWOD))) \
				mo->momy = FixedMul(FINESINE(fa),ns); \
			P_SetObjectMomZ(mo, 4*FRACUNIT, false); \
			if (i & 1) \
				P_SetObjectMomZ(mo, 4*FRACUNIT, true); \
			if (player->mo->eflags & MFE_VERTICALFLIP) \
				mo->flags2 |= MF2_OBJECTFLIP; \
		} \
	} \
	else if (player->powers[power] > 0) \
	{ \
		SETUP_DROP(ammo) \
		mo = P_SpawnMobj(player->mo->x, player->mo->y, z, ammo); \
		if (!P_MobjWasRemoved(mo)) \
		{ \
			mo->health = player->powers[power]; \
			mo->flags2 |= MF2_DONTRESPAWN; \
			mo->flags &= ~(MF_NOGRAVITY|MF_NOCLIPHEIGHT); \
			P_SetTarget(&mo->target, player->mo); \
			mo->fuse = 12*TICRATE; \
			P_SetScale(mo, player->mo->scale, true); \
			mo->momx = FixedMul(FINECOSINE(fa),ns); \
			if (!(twodlevel || (player->mo->flags2 & MF2_TWOD))) \
				mo->momy = FixedMul(FINESINE(fa),ns); \
			P_SetObjectMomZ(mo, 3*FRACUNIT, false); \
			if (i & 1) \
				P_SetObjectMomZ(mo, 3*FRACUNIT, true); \
			if (player->mo->eflags & MFE_VERTICALFLIP) \
				mo->flags2 |= MF2_OBJECTFLIP; \
			player->powers[power] = 0; \
		} \
	} \
	++i

	DROP_WEAPON(RW_BOUNCE, MT_BOUNCEPICKUP, MT_BOUNCERING, pw_bouncering);
	DROP_WEAPON(RW_RAIL, MT_RAILPICKUP, MT_RAILRING, pw_railring);
	DROP_WEAPON(RW_AUTO, MT_AUTOPICKUP, MT_AUTOMATICRING, pw_automaticring);
	DROP_WEAPON(RW_EXPLODE, MT_EXPLODEPICKUP, MT_EXPLOSIONRING, pw_explosionring);
	DROP_WEAPON(RW_SCATTER, MT_SCATTERPICKUP, MT_SCATTERRING, pw_scatterring);
	DROP_WEAPON(RW_GRENADE, MT_GRENADEPICKUP, MT_GRENADERING, pw_grenadering);
	DROP_WEAPON(0, 0, MT_INFINITYRING, pw_infinityring);

	#undef DROP_WEAPON
	#undef SETUP_DROP
}






void P_PlayerEmeraldBurst(player_t *player, boolean toss)
{
	INT32 i;
	angle_t fa;
	fixed_t ns;
	fixed_t z = 0, momx = 0, momy = 0;


	if (!player)
		return;


	if (player->powers[pw_emeralds])
	{
		INT32 num_stones = 0;

		if (player->powers[pw_emeralds] & EMERALD1)
			num_stones++;
		if (player->powers[pw_emeralds] & EMERALD2)
			num_stones++;
		if (player->powers[pw_emeralds] & EMERALD3)
			num_stones++;
		if (player->powers[pw_emeralds] & EMERALD4)
			num_stones++;
		if (player->powers[pw_emeralds] & EMERALD5)
			num_stones++;
		if (player->powers[pw_emeralds] & EMERALD6)
			num_stones++;
		if (player->powers[pw_emeralds] & EMERALD7)
			num_stones++;

		for (i = 0; i < num_stones; i++)
		{
			INT32 stoneflag = 0;
			statenum_t statenum = S_CEMG1;
			mobj_t *mo;

			if (player->powers[pw_emeralds] & EMERALD1)
			{
				stoneflag = EMERALD1;
				statenum = S_CEMG1;
			}
			else if (player->powers[pw_emeralds] & EMERALD2)
			{
				stoneflag = EMERALD2;
				statenum = S_CEMG2;
			}
			else if (player->powers[pw_emeralds] & EMERALD3)
			{
				stoneflag = EMERALD3;
				statenum = S_CEMG3;
			}
			else if (player->powers[pw_emeralds] & EMERALD4)
			{
				stoneflag = EMERALD4;
				statenum = S_CEMG4;
			}
			else if (player->powers[pw_emeralds] & EMERALD5)
			{
				stoneflag = EMERALD5;
				statenum = S_CEMG5;
			}
			else if (player->powers[pw_emeralds] & EMERALD6)
			{
				stoneflag = EMERALD6;
				statenum = S_CEMG6;
			}
			else if (player->powers[pw_emeralds] & EMERALD7)
			{
				stoneflag = EMERALD7;
				statenum = S_CEMG7;
			}

			if (!stoneflag)  
				continue;

			player->powers[pw_emeralds] &= ~stoneflag;

			if (toss)
			{
				fa = player->mo->angle>>ANGLETOFINESHIFT;

				z = player->mo->z + player->mo->height;
				if (player->mo->eflags & MFE_VERTICALFLIP)
					z -= mobjinfo[MT_FLINGEMERALD].height + player->mo->height;
				ns = FixedMul(8*FRACUNIT, player->mo->scale);
			}
			else
			{
				fa = ((255 / num_stones) * i) * FINEANGLES/256;

				z = player->mo->z + (player->mo->height / 2);
				if (player->mo->eflags & MFE_VERTICALFLIP)
					z -= mobjinfo[MT_FLINGEMERALD].height;
				ns = FixedMul(4*FRACUNIT, player->mo->scale);
			}

			momx = FixedMul(FINECOSINE(fa), ns);

			if (!(twodlevel || (player->mo->flags2 & MF2_TWOD)))
				momy = FixedMul(FINESINE(fa),ns);
			else
				momy = 0;

			mo = P_SpawnMobj(player->mo->x, player->mo->y, z, MT_FLINGEMERALD);
			if (!P_MobjWasRemoved(mo))
			{
				mo->health = 1;
				mo->threshold = stoneflag;
				mo->flags2 |= (MF2_DONTRESPAWN|MF2_SLIDEPUSH);
				mo->flags &= ~(MF_NOGRAVITY|MF_NOCLIPHEIGHT);
				P_SetTarget(&mo->target, player->mo);
				mo->fuse = 12*TICRATE;
				P_SetMobjState(mo, statenum);

				mo->momx = momx;
				mo->momy = momy;

				P_SetObjectMomZ(mo, 3*FRACUNIT, false);

				if (player->mo->eflags & MFE_VERTICALFLIP)
				{
					mo->momz = -mo->momz;
					mo->flags2 |= MF2_OBJECTFLIP;
				}
			}

			if (toss)
				player->tossdelay = 2*TICRATE;
		}
	}
}

                                                                
   
                                                             
                         
    
void P_PlayerFlagBurst(player_t *player, boolean toss)
{
	mobj_t *flag;
	mobjtype_t type;

	if (!(player->gotflag & (GF_REDFLAG|GF_BLUEFLAG)))
		return;

	if (player->gotflag & GF_REDFLAG)
		type = MT_REDFLAG;
	else
		type = MT_BLUEFLAG;

	flag = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, type);
	if (P_MobjWasRemoved(flag))
		return;

	if (player->mo->eflags & MFE_VERTICALFLIP)
	{
		flag->z += player->mo->height - flag->height;
		flag->flags2 |= MF2_OBJECTFLIP;
	}

	if (toss)
		P_InstaThrust(flag, player->mo->angle, FixedMul(6*FRACUNIT, player->mo->scale));
	else
	{
		angle_t fa = P_RandomByte()*FINEANGLES/256;
		flag->momx = FixedMul(FINECOSINE(fa), FixedMul(6*FRACUNIT, player->mo->scale));
		if (!(twodlevel || (player->mo->flags2 & MF2_TWOD)))
			flag->momy = FixedMul(FINESINE(fa), FixedMul(6*FRACUNIT, player->mo->scale));
	}

	flag->momz = FixedMul(8*FRACUNIT, player->mo->scale);
	if (player->mo->eflags & MFE_VERTICALFLIP)
		flag->momz = -flag->momz;

	if (type == MT_REDFLAG)
		flag->spawnpoint = rflagpoint;
	else
		flag->spawnpoint = bflagpoint;

	flag->fuse = cv_flagtime.value * TICRATE;
	P_SetTarget(&flag->target, player->mo);


	{
		char plname[MAXPLAYERNAME+4];
		const char *flagtext;
		char flagcolor;

		snprintf(plname, sizeof(plname), "%s%s%s",
				 CTFTEAMCODE(player),
				 player_names[player - players],
				 CTFTEAMENDCODE(player));

		if (type == MT_REDFLAG)
		{
			flagtext = M_GetText("Red flag");
			flagcolor = '\x85';
		}
		else
		{
			flagtext = M_GetText("Blue flag");
			flagcolor = '\x84';
		}

		if (toss)
			CONS_Printf(M_GetText("%s tossed the %c%s%c.\n"), plname, flagcolor, flagtext, 0x80);
		else
			CONS_Printf(M_GetText("%s dropped the %c%s%c.\n"), plname, flagcolor, flagtext, 0x80);
	}

	player->gotflag = 0;


	if (type == MT_REDFLAG)
		redflag = flag;
	else
		blueflag = flag;

	if (toss)
		player->tossdelay = 2*TICRATE;

	return;
}
