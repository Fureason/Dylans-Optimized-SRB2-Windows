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



#include "dehacked.h"
#include "doomdef.h"
#include "doomstat.h"
#include "m_random.h"
#include "p_local.h"
#include "p_slopes.h"
#include "r_fps.h"
#include "r_state.h"
#include "s_sound.h"
#include "z_zone.h"
#include "g_game.h"
#include "r_main.h"








result_e T_MovePlane(sector_t *sector, fixed_t speed, fixed_t dest, boolean crush,
	boolean ceiling, INT32 direction)
{
	fixed_t lastpos;
	fixed_t destheight;  
	sector->moved = true;

	if (ceiling)
	{
		lastpos = sector->ceilingheight;

		switch (direction)
		{
			case -1:


				destheight = (dest > sector->floorheight) ? dest : sector->floorheight;
				if (sector->ceilingheight - speed < destheight)
				{
					sector->ceilingheight = destheight;
					if (P_CheckSector(sector, crush))
					{
						sector->ceilingheight = lastpos;
						P_CheckSector(sector, crush);
					}
					return pastdest;
				}
				else
				{

					sector->ceilingheight -= speed;
					if (P_CheckSector(sector, crush))
					{
						sector->ceilingheight = lastpos;
						P_CheckSector(sector, crush);
						return crushed;
					}
				}
				break;

			case 1:

				if (sector->ceilingheight + speed > dest)
				{
					sector->ceilingheight = dest;
					if (P_CheckSector(sector, crush) && sector->numattached)
					{
						sector->ceilingheight = lastpos;
						P_CheckSector(sector, crush);
					}
					return pastdest;
				}
				else
				{
					sector->ceilingheight += speed;
					if (P_CheckSector(sector, crush) && sector->numattached)
					{
						sector->ceilingheight = lastpos;
						P_CheckSector(sector, crush);
						return crushed;
					}
				}
				break;
		}
	}
	else
	{
		lastpos = sector->floorheight;

		switch (direction)
		{
			case -1:

				if (sector->floorheight - speed < dest)
				{
					sector->floorheight = dest;
					if (P_CheckSector(sector, crush) && sector->numattached)
					{
						sector->floorheight = lastpos;
						P_CheckSector(sector, crush);
					}
					return pastdest;
				}
				else
				{
					sector->floorheight -= speed;
					if (P_CheckSector(sector, crush) && sector->numattached)
					{
						sector->floorheight = lastpos;
						P_CheckSector(sector, crush);
						return crushed;
					}
				}
				break;

			case 1:


				destheight = (dest < sector->ceilingheight) ? dest : sector->ceilingheight;
				if (sector->floorheight + speed > destheight)
				{
					sector->floorheight = destheight;
					if (P_CheckSector(sector, crush))
					{
						sector->floorheight = lastpos;
						P_CheckSector(sector, crush);
					}
					return pastdest;
				}
				else
				{

					sector->floorheight += speed;
					if (P_CheckSector(sector, crush))
					{
						sector->floorheight = lastpos;
						P_CheckSector(sector, crush);
						return crushed;
					}
				}
				break;
		}
	}

	return planeok;
}




void T_MoveFloor(floormove_t *movefloor)
{
	result_e res = 0;
	boolean remove = false;

	if (movefloor->delaytimer)
	{
		movefloor->delaytimer--;
		return;
	}

	res = T_MovePlane(movefloor->sector,
	                  movefloor->speed,
	                  movefloor->floordestheight,
	                  movefloor->crush, false, movefloor->direction);

	if (movefloor->type == bounceFloor)
	{
		const fixed_t origspeed = FixedDiv(movefloor->origspeed,(ELEVATORSPEED/2));
		const fixed_t fs = abs(movefloor->sector->floorheight - lines[movefloor->sourceline].frontsector->floorheight);
		const fixed_t bs = abs(movefloor->sector->floorheight - lines[movefloor->sourceline].backsector->floorheight);
		if (fs < bs)
			movefloor->speed = FixedDiv(fs,25*FRACUNIT) + FRACUNIT/4;
		else
			movefloor->speed = FixedDiv(bs,25*FRACUNIT) + FRACUNIT/4;

		movefloor->speed = FixedMul(movefloor->speed,origspeed);
	}

	if (res == pastdest)
	{
		switch (movefloor->type)
		{
			case moveFloorByFrontSector:
				if (movefloor->tag)  
					P_LinedefExecute(movefloor->tag, NULL, NULL);
				 
			case instantMoveFloorByFrontSector:
				if (movefloor->texture > -1)  
					movefloor->sector->floorpic = movefloor->texture;
				remove = true;
				break;
			case bounceFloor:  
			case bounceFloorCrush:  
				if (movefloor->floordestheight == lines[movefloor->sourceline].frontsector->floorheight)
				{
					movefloor->floordestheight = lines[movefloor->sourceline].backsector->floorheight;
					movefloor->origspeed = lines[movefloor->sourceline].args[3] << (FRACBITS - 2);  
				}
				else
				{
					movefloor->floordestheight = lines[movefloor->sourceline].frontsector->floorheight;
					movefloor->origspeed = lines[movefloor->sourceline].args[2] << (FRACBITS - 2);  
				}
				if (movefloor->type == bounceFloorCrush)
					movefloor->speed = movefloor->origspeed;
				movefloor->direction = (movefloor->floordestheight < movefloor->sector->floorheight) ? -1 : 1;
				movefloor->delaytimer = movefloor->delay;
				remove = false;
				break;
			case crushFloorOnce:
				if (movefloor->direction == 1)
				{
					movefloor->floordestheight = lines[movefloor->sourceline].frontsector->floorheight;
					movefloor->direction = -1;
					movefloor->speed = lines[movefloor->sourceline].args[3] << (FRACBITS - 2);
					movefloor->sector->soundorg.z = movefloor->sector->floorheight;
					S_StartSound(&movefloor->sector->soundorg, sfx_pstop);
					remove = false;
				}
				else
					remove = true;
				break;
			default:
				remove = true;
				break;
		}
	}

	if (remove)
	{
		movefloor->sector->floordata = NULL;  
		movefloor->sector->floorspeed = 0;
		P_RemoveThinker(&movefloor->thinker);
	}
	else
		movefloor->sector->floorspeed = movefloor->speed*movefloor->direction;

	P_RecalcPrecipInSector(movefloor->sector);
}














void T_MoveElevator(elevator_t *elevator)
{
	result_e res1 = 0, res2 = 0, res = 0;
	boolean dontupdate = false;
	fixed_t oldfloor, oldceiling;

	if (elevator->delaytimer)
	{
		elevator->delaytimer--;
		return;
	}

	if (elevator->direction < 0)  
	{
		if (elevator->type == elevateContinuous)
		{
			const fixed_t origspeed = FixedDiv(elevator->origspeed,(ELEVATORSPEED/2));
			const fixed_t wh = abs(elevator->sector->floorheight - elevator->floorwasheight);
			const fixed_t dh = abs(elevator->sector->floorheight - elevator->floordestheight);


			if (wh < dh)
				elevator->speed = FixedDiv(wh,25*FRACUNIT) + FRACUNIT/4;
			else
				elevator->speed = FixedDiv(dh,25*FRACUNIT) + FRACUNIT/4;

			if (elevator->origspeed)
			{
				elevator->speed = FixedMul(elevator->speed,origspeed);
				if (elevator->speed > elevator->origspeed)
					elevator->speed = (elevator->origspeed);
				if (elevator->speed < 1)
					elevator->speed = 1;
			}
			else
			{
				if (elevator->speed > 3*FRACUNIT)
					elevator->speed = 3*FRACUNIT;
				if (elevator->speed < 1)
					elevator->speed = 1;
			}
		}

		oldfloor = elevator->sector->floorheight;
		oldceiling = elevator->sector->ceilingheight;

		res1 = T_MovePlane              
		(
			elevator->sector,
			elevator->speed,
			elevator->ceilingdestheight,
			elevator->distance,
			true,                           
			elevator->direction
		);

		res2 = T_MovePlane
		(
			elevator->sector,
			elevator->speed,
			elevator->floordestheight,
			elevator->distance,
			false,                         
			elevator->direction
		);

		if (elevator->distance && (res1 == crushed || res2 == crushed))
		{
			res = crushed;
			elevator->sector->floorheight = oldfloor;
			elevator->sector->ceilingheight = oldceiling;
		}
		else
			res = res1;
	}
	else  
	{
		if (elevator->type == elevateContinuous)
		{
			const fixed_t origspeed = FixedDiv(elevator->origspeed,(ELEVATORSPEED/2));
			const fixed_t wc = abs(elevator->sector->ceilingheight - elevator->ceilingwasheight);
			const fixed_t dc = abs(elevator->sector->ceilingheight - elevator->ceilingdestheight);

			if (wc < dc)
				elevator->speed = FixedDiv(wc,25*FRACUNIT) + FRACUNIT/4;
			else
				elevator->speed = FixedDiv(dc,25*FRACUNIT) + FRACUNIT/4;

			if (elevator->origspeed)
			{
				elevator->speed = FixedMul(elevator->speed,origspeed);
				if (elevator->speed > elevator->origspeed)
					elevator->speed = (elevator->origspeed);
				if (elevator->speed < 1)
					elevator->speed = 1;
			}
			else
			{
				if (elevator->speed > 3*FRACUNIT)
					elevator->speed = 3*FRACUNIT;
				if (elevator->speed < 1)
					elevator->speed = 1;
			}
		}

		oldfloor = elevator->sector->floorheight;
		oldceiling = elevator->sector->ceilingheight;

		res1 = T_MovePlane              
		(
			elevator->sector,
			elevator->speed,
			elevator->floordestheight,
			elevator->distance,
			false,                           
			elevator->direction
		);

		if (res1 != crushed)
		{
			res2 = T_MovePlane
			(
				elevator->sector,
				elevator->speed,
				elevator->ceilingdestheight,
				elevator->distance,
				true,                         
				elevator->direction
			);
		}

		if (elevator->distance && (res1 == crushed || res2 == crushed))
		{
			res = crushed;
			elevator->sector->floorheight = oldfloor;
			elevator->sector->ceilingheight = oldceiling;
		}
		else
			res = res1;
	}
  
                         
                    
                                                        
  
	if (res == pastdest || res == crushed)             
	{
		if (elevator->type == elevateContinuous)
		{
			if (elevator->direction > 0)
			{
				elevator->high = 1;
				elevator->low = 0;
				elevator->direction = -1;

				if (elevator->origspeed)
					elevator->speed = elevator->origspeed;
				else
					elevator->speed = 3*FRACUNIT;

				elevator->floorwasheight = elevator->floordestheight;
				elevator->ceilingwasheight = elevator->ceilingdestheight;

				if (elevator->low)
				{
					elevator->floordestheight =
						P_FindNextHighestFloor(elevator->sector, elevator->sector->floorheight);
					elevator->ceilingdestheight =
						elevator->floordestheight + elevator->sector->ceilingheight - elevator->sector->floorheight;
				}
				else
				{
					elevator->floordestheight =
						P_FindNextLowestFloor(elevator->sector,elevator->sector->floorheight);
					elevator->ceilingdestheight =
						elevator->floordestheight + elevator->sector->ceilingheight - elevator->sector->floorheight;
				}

			}
			else
			{
				elevator->high = 0;
				elevator->low = 1;
				elevator->direction = 1;

				if (elevator->origspeed)
					elevator->speed = elevator->origspeed;
				else
					elevator->speed = 3*FRACUNIT;

				elevator->floorwasheight = elevator->floordestheight;
				elevator->ceilingwasheight = elevator->ceilingdestheight;

				if (elevator->low)
				{
					elevator->floordestheight =
						P_FindNextHighestFloor(elevator->sector, elevator->sector->floorheight);
					elevator->ceilingdestheight =
						elevator->floordestheight + elevator->sector->ceilingheight - elevator->sector->floorheight;
				}
				else
				{
					elevator->floordestheight =
						P_FindNextLowestFloor(elevator->sector,elevator->sector->floorheight);
					elevator->ceilingdestheight =
						elevator->floordestheight + elevator->sector->ceilingheight - elevator->sector->floorheight;
				}

			}
			elevator->delaytimer = elevator->delay;
		}
		else
		{
			elevator->sector->floordata = NULL;      
			elevator->sector->ceilingdata = NULL;    
			elevator->sector->ceilspeed = 0;
			elevator->sector->floorspeed = 0;
			P_RemoveThinker(&elevator->thinker);     
			dontupdate = true;
		}


	}
	if (!dontupdate)
	{
		elevator->sector->floorspeed = elevator->speed*elevator->direction;
		elevator->sector->ceilspeed = 42;
	}
	else
	{
		elevator->sector->floorspeed = 0;
		elevator->sector->ceilspeed = 0;
		elevator->sector->floordata = NULL;
		elevator->sector->ceilingdata = NULL;
	}
}











void T_ContinuousFalling(continuousfall_t *faller)
{
	faller->sector->ceilingheight += faller->speed*faller->direction;
	faller->sector->floorheight += faller->speed*faller->direction;

	P_CheckSector(faller->sector, false);

	if ((faller->direction == -1 && faller->sector->ceilingheight <= faller->destheight)
		|| (faller->direction == 1 && faller->sector->floorheight >= faller->destheight))
	{
		faller->sector->ceilingheight = faller->ceilingstartheight;
		faller->sector->floorheight = faller->floorstartheight;

		R_ClearLevelInterpolatorState(&faller->thinker);
	}

	P_CheckSector(faller->sector, false);  

	faller->sector->floorspeed = faller->speed*faller->direction;
	faller->sector->ceilspeed = 42;
	faller->sector->moved = true;
}





static fixed_t P_SectorCheckWater(sector_t *analyzesector,
	sector_t *elevatorsec)
{
	fixed_t watertop;


	watertop = analyzesector->floorheight - 512*FRACUNIT;


	if (analyzesector->ffloors)
	{
		ffloor_t *rover;

		for (rover = analyzesector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_SWIMMABLE) || rover->fofflags & FOF_SOLID)
				continue;


			if ((elevatorsec->ceilingheight + elevatorsec->floorheight)>>1 < *rover->bottomheight)
				continue;


			if (*rover->topheight < analyzesector->floorheight + abs((elevatorsec->ceilingheight - elevatorsec->floorheight)>>1))
				continue;

			if (*rover->topheight > watertop)  
				watertop = *rover->topheight;
		}
	}

	return watertop;
}






void T_BounceCheese(bouncecheese_t *bouncer)
{
	fixed_t sectorheight;
	fixed_t halfheight;
	fixed_t waterheight;
	fixed_t floorheight;
	sector_t *actionsector;
	boolean remove;
	INT32 i;

	if (bouncer->sector->crumblestate == CRUMBLE_RESTORE || bouncer->sector->crumblestate == CRUMBLE_WAIT
		|| bouncer->sector->crumblestate == CRUMBLE_ACTIVATED)  
	{
		bouncer->sector->ceilingdata = NULL;
		bouncer->sector->ceilspeed = 0;
		bouncer->sector->floordata = NULL;
		bouncer->sector->floorspeed = 0;
		P_RemoveThinker(&bouncer->thinker);  
		return;
	}


	TAG_ITER_SECTORS(bouncer->sourceline->args[0], i)
	{
		actionsector = &sectors[i];
		actionsector->moved = true;

		sectorheight = abs(bouncer->sector->ceilingheight - bouncer->sector->floorheight);
		halfheight = sectorheight/2;

		waterheight = P_SectorCheckWater(actionsector, bouncer->sector);  

		floorheight = P_FloorzAtPos(actionsector->soundorg.x, actionsector->soundorg.y, bouncer->sector->floorheight, sectorheight);

		remove = false;


		if (waterheight > bouncer->sector->ceilingheight - halfheight && bouncer->sector->ceilingheight >= actionsector->ceilingheight)  
		{
			bouncer->sector->ceilingheight = actionsector->ceilingheight;
			bouncer->sector->floorheight = actionsector->ceilingheight - sectorheight;
			remove = true;
		}

		else if (waterheight < bouncer->sector->floorheight + halfheight && bouncer->sector->floorheight <= floorheight)
		{
			bouncer->sector->ceilingheight = floorheight + sectorheight;
			bouncer->sector->floorheight = floorheight;
			remove = true;
		}
		else
		{
			bouncer->ceilingwasheight = waterheight + halfheight;
			bouncer->floorwasheight = waterheight - halfheight;
		}

		if (remove)
		{
			T_MovePlane(bouncer->sector, 0, bouncer->sector->ceilingheight, false, true, -1);  
			T_MovePlane(bouncer->sector, 0, bouncer->sector->floorheight, false, false, -1);  
			P_RecalcPrecipInSector(actionsector);
			bouncer->sector->ceilingdata = NULL;
			bouncer->sector->floordata = NULL;
			bouncer->sector->floorspeed = 0;
			bouncer->sector->ceilspeed = 0;
			bouncer->sector->moved = true;
			P_RemoveThinker(&bouncer->thinker);  
			return;
		}

		if (bouncer->speed >= 0)  
		{
			T_MovePlane(bouncer->sector, bouncer->speed/2, bouncer->sector->floorheight - 70*FRACUNIT,
				false, false, -1);  
			T_MovePlane(bouncer->sector, bouncer->speed/2, bouncer->sector->ceilingheight -
				70*FRACUNIT, false, true, -1);  
		}
		else
		{
			T_MovePlane(bouncer->sector, bouncer->speed/2, bouncer->sector->ceilingheight -
				70*FRACUNIT, false, true, -1);  
			T_MovePlane(bouncer->sector, bouncer->speed/2, bouncer->sector->floorheight - 70*FRACUNIT,
				false, false, -1);  
		}

		bouncer->sector->floorspeed = -bouncer->speed/2;
		bouncer->sector->ceilspeed = 42;

		if ((bouncer->sector->ceilingheight < bouncer->ceilingwasheight && !bouncer->low)  
			|| (bouncer->sector->ceilingheight > bouncer->ceilingwasheight && bouncer->low))  
		{
			if (abs(bouncer->speed) < 6*FRACUNIT)
				bouncer->speed -= bouncer->speed/3;
			else
				bouncer->speed -= bouncer->speed/2;

			bouncer->low = !bouncer->low;
			if (abs(bouncer->speed) > 6*FRACUNIT)
			{
				mobj_t *mp = (void *)&actionsector->soundorg;
				actionsector->soundorg.z = bouncer->sector->floorheight;
				S_StartSound(mp, sfx_splash);
			}
		}

		if (bouncer->sector->ceilingheight < bouncer->ceilingwasheight)  
		{
			bouncer->speed -= bouncer->distance;
		}
		else if (bouncer->sector->ceilingheight > bouncer->ceilingwasheight)  
		{
			bouncer->speed += gravity;
		}

		if (abs(bouncer->speed) < 2*FRACUNIT
		&& abs(bouncer->sector->ceilingheight-bouncer->ceilingwasheight) < FRACUNIT/4)
		{
			bouncer->sector->floorheight = bouncer->floorwasheight;
			bouncer->sector->ceilingheight = bouncer->ceilingwasheight;
			T_MovePlane(bouncer->sector, 0, bouncer->sector->ceilingheight, false, true, -1);  
			T_MovePlane(bouncer->sector, 0, bouncer->sector->floorheight, false, false, -1);  
			bouncer->sector->ceilingdata = NULL;
			bouncer->sector->floordata = NULL;
			bouncer->sector->floorspeed = 0;
			bouncer->sector->ceilspeed = 0;
			bouncer->sector->moved = true;
			P_RemoveThinker(&bouncer->thinker);     
		}

		if (bouncer->distance > 0)
			bouncer->distance--;

		if (actionsector)
			P_RecalcPrecipInSector(actionsector);
	}
}





void T_StartCrumble(crumble_t *crumble)
{
	ffloor_t *rover;
	sector_t *sector;
	INT32 i;
	mtag_t tag = crumble->sourceline->args[0];



	if ((((crumble->flags & CF_REVERSE) && crumble->direction == -1)
		|| (!(crumble->flags & CF_REVERSE) && crumble->direction == 1))
		&& !(crumble->flags & CF_RETURN))
	{
		crumble->sector->ceilspeed = 0;
		crumble->sector->floorspeed = 0;
		return;
	}

	if (crumble->timer != 0)
	{
		if (crumble->timer > 0)  
		{
			if (--crumble->timer <= 0)
				crumble->timer = -15*TICRATE;  
			else
			{

				crumble->sector->ceilspeed = 0;
				crumble->sector->floorspeed = 0;
				return;
			}
		}
		else if (++crumble->timer == 0)  
		{
			TAG_ITER_SECTORS(tag, i)
			{
				sector = &sectors[i];

				for (rover = sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->fofflags & FOF_CRUMBLE))
						continue;

					if (!(rover->fofflags & FOF_FLOATBOB))
						continue;

					if (rover->master != crumble->sourceline)
						continue;

					rover->alpha = crumble->origalpha;

					if (rover->alpha == 0xff)
						rover->fofflags &= ~FOF_TRANSLUCENT;
				}
			}


			if (crumble->flags & CF_REVERSE)
				crumble->direction = -1;
			else
				crumble->direction = 1;

			crumble->sector->ceilspeed = 0;
			crumble->sector->floorspeed = 0;
			return;
		}


		if (crumble->timer > -224 && (leveltime % ((abs(crumble->timer)/8) + 1) == 0))
		{
			TAG_ITER_SECTORS(tag, i)
			{
				sector = &sectors[i];

				for (rover = sector->ffloors; rover; rover = rover->next)
				{
					if (rover->fofflags & FOF_NORETURN)
						continue;

					if (!(rover->fofflags & FOF_CRUMBLE))
						continue;

					if (!(rover->fofflags & FOF_FLOATBOB))
						continue;

					if (rover->master != crumble->sourceline)
						continue;

					if (rover->alpha == crumble->origalpha)
					{
						rover->fofflags |= FOF_TRANSLUCENT;
						rover->alpha = 0x00;
					}
					else
					{
						rover->alpha = crumble->origalpha;

						if (rover->alpha == 0xff)
							rover->fofflags &= ~FOF_TRANSLUCENT;
					}
				}
			}
		}




		if (crumble->timer < 0 && crumble->timer > -3)
			crumble->sector->crumblestate = CRUMBLE_RESTORE;  
	}

	if ((!(crumble->flags & CF_REVERSE) && crumble->direction == -1)
		|| ((crumble->flags & CF_REVERSE) && crumble->direction == 1))  
	{
		crumble->sector->crumblestate = CRUMBLE_FALL;  


		if (!(crumble->flags & CF_FLOATBOB))
		{
			crumble->speed += gravity;  

			if ((!(crumble->flags & CF_REVERSE) && crumble->sector->ceilingheight >= -16384*FRACUNIT)
				|| ((crumble->flags & CF_REVERSE) && crumble->sector->ceilingheight <= 16384*FRACUNIT))
			{
				T_MovePlane              
				(
				  crumble->sector,
				  crumble->speed,
				  crumble->sector->ceilingheight + crumble->direction*crumble->speed*2,
				  false,
				  true,  
				  crumble->direction
				);

				T_MovePlane
				(
				  crumble->sector,
				  crumble->speed,
				  crumble->sector->floorheight + crumble->direction*crumble->speed*2,
				  false,
				  false,  
				  crumble->direction
				);

				crumble->sector->ceilspeed = 42;
				crumble->sector->floorspeed = crumble->speed*crumble->direction;
			}
		}
	}
	else  
	{
		crumble->sector->crumblestate = CRUMBLE_WAIT;
		crumble->sector->ceilingheight = crumble->ceilingwasheight;
		crumble->sector->floorheight = crumble->floorwasheight;
		crumble->sector->floordata = NULL;
		crumble->sector->ceilingdata = NULL;
		crumble->sector->ceilspeed = 0;
		crumble->sector->floorspeed = 0;
		crumble->sector->moved = true;
		P_RemoveThinker(&crumble->thinker);
	}

	TAG_ITER_SECTORS(tag, i)
	{
		sector = &sectors[i];
		sector->moved = true;
		P_RecalcPrecipInSector(sector);
	}
}






void T_MarioBlock(mariothink_t *block)
{
	INT32 i;

	T_MovePlane
	(
	  block->sector,
	  block->speed,
	  block->sector->ceilingheight + 70*FRACUNIT * block->direction,
	  false,
	  true,  
	  block->direction
	);

	T_MovePlane
	(
	  block->sector,
	  block->speed,
	  block->sector->floorheight + 70*FRACUNIT * block->direction,
	  false,
	  false,  
	  block->direction
	);

	if (block->sector->ceilingheight >= block->ceilingstartheight + 32*FRACUNIT)  
		block->direction *= -1;
	else if (block->sector->ceilingheight <= block->ceilingstartheight)
	{
		block->sector->ceilingheight = block->ceilingstartheight;
		block->sector->floorheight = block->floorstartheight;
		P_RemoveThinker(&block->thinker);
		block->sector->floordata = NULL;
		block->sector->ceilingdata = NULL;
		block->sector->floorspeed = 0;
		block->sector->ceilspeed = 0;
		block->direction = 0;
	}
	TAG_ITER_SECTORS((INT16)block->tag, i)
		P_RecalcPrecipInSector(&sectors[i]);
}

void T_FloatSector(floatthink_t *floater)
{
	fixed_t cheeseheight;
	fixed_t waterheight;
	sector_t *actionsector;
	INT32 secnum;



	if ((secnum = Tag_Iterate_Sectors((INT16)floater->tag, 0)) < 0)
		return;
	actionsector = &sectors[secnum];

	cheeseheight = (floater->sector->ceilingheight + floater->sector->floorheight)>>1;


	waterheight = P_SectorCheckWater(actionsector, floater->sector);  

	if (waterheight == cheeseheight)  
		return;

	if (floater->sector->floorheight == actionsector->floorheight && waterheight < cheeseheight)  
		return;

	if (floater->sector->ceilingheight == actionsector->ceilingheight && waterheight > cheeseheight)  
		return;


	if (floater->sector->crumblestate == CRUMBLE_NONE || floater->sector->crumblestate >= CRUMBLE_FALL )
		EV_BounceSector(floater->sector, FRACUNIT, floater->sourceline);
}

static mobj_t *SearchMarioNode(msecnode_t *node)
{
	mobj_t *thing = NULL;
	for (; node; node = node->m_thinglist_next)
	{

		switch (node->m_thing->type)
		{
		case MT_NULL:
		case MT_UNKNOWN:
		case MT_TAILSOVERLAY:
		case MT_THOK:
		case MT_GHOST:
		case MT_OVERLAY:
		case MT_EMERALDSPAWN:
		case MT_ELEMENTAL_ORB:
		case MT_ATTRACT_ORB:
		case MT_FORCE_ORB:
		case MT_ARMAGEDDON_ORB:
		case MT_WHIRLWIND_ORB:
		case MT_PITY_ORB:
		case MT_FLAMEAURA_ORB:
		case MT_BUBBLEWRAP_ORB:
		case MT_THUNDERCOIN_ORB:
		case MT_IVSP:
		case MT_SUPERSPARK:
		case MT_BOXSPARKLE:
		case MT_RAIN:
		case MT_SNOWFLAKE:
		case MT_SPLISH:
		case MT_LAVASPLISH:
		case MT_SMOKE:
		case MT_SMALLBUBBLE:
		case MT_MEDIUMBUBBLE:
		case MT_TFOG:
		case MT_SEED:
		case MT_PARTICLE:
		case MT_SCORE:
		case MT_DROWNNUMBERS:
		case MT_GOTEMERALD:
		case MT_LOCKON:
		case MT_TAG:
		case MT_GOTFLAG:
		case MT_HOOP:
		case MT_HOOPCOLLIDE:
		case MT_NIGHTSCORE:
		case MT_NAMECHECK:  
			continue;
		default:
			break;
		}

		if (node->m_thing->health == 0  
		|| (!(node->m_thing->flags & MF_MONITOR) && (mobjinfo[node->m_thing->type].flags & MF_MONITOR)))  
			continue;

		if (!thing  
		|| node->m_thing->x < thing->x  
		|| node->m_thing->y < thing->y)  
			thing = node->m_thing;
	}
	return thing;
}

void T_MarioBlockChecker(mariocheck_t *block)
{
	line_t *masterline = block->sourceline;
	if (SearchMarioNode(block->sector->touching_thinglist))
	{
		sides[masterline->sidenum[0]].midtexture = sides[masterline->sidenum[0]].bottomtexture;  
		if (masterline->backsector)
			block->sector->ceilingpic = block->sector->floorpic = masterline->backsector->ceilingpic;  
	}
	else
	{
		sides[masterline->sidenum[0]].midtexture = sides[masterline->sidenum[0]].toptexture;
		if (masterline->backsector)
			block->sector->ceilingpic = block->sector->floorpic = masterline->backsector->floorpic;  
	}
}



void T_ThwompSector(thwomp_t *thwomp)
{
	fixed_t thwompx, thwompy;
	sector_t *actionsector;
	ffloor_t *rover = NULL;
	INT32 secnum;
	fixed_t speed;


	if (--thwomp->delay > 0)
		return;



	if ((secnum = Tag_Iterate_Sectors((INT16)thwomp->tag, 0)) < 0)
		return;

	actionsector = &sectors[secnum];


	for (rover = actionsector->ffloors; rover; rover = rover->next)
	{
		if (rover->master == thwomp->sourceline)
			break;
	}

	if (!rover)
		return;  

	thwompx = actionsector->soundorg.x;
	thwompy = actionsector->soundorg.y;

	if (thwomp->direction == 0)  
	{
		if (rover->fofflags & FOF_EXISTS)
		{
			UINT8 i;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!P_IsPlayerValid(i))
					continue;

				if (players[i].mo->z > thwomp->sector->ceilingheight)
					continue;

				if (P_AproxDistance(thwompx - players[i].mo->x, thwompy - players[i].mo->y) > 96*FRACUNIT)
					continue;

				thwomp->direction = -1;
				break;
			}
		}

		thwomp->sector->ceilspeed = 0;
		thwomp->sector->floorspeed = 0;
	}
	else
	{
		result_e res = 0;

		if (thwomp->direction > 0)  
		{

			sides[thwomp->sourceline->sidenum[0]].midtexture = sides[thwomp->sourceline->sidenum[0]].bottomtexture;

			speed = thwomp->retractspeed;

			res = T_MovePlane
			(
				thwomp->sector,            
				speed,                     
				thwomp->floorstartheight,  
				false,                     
				false,                     
				thwomp->direction          
			);

			if (res == planeok || res == pastdest)
				T_MovePlane
				(
					thwomp->sector,              
					speed,                       
					thwomp->ceilingstartheight,  
					false,                       
					true,                        
					thwomp->direction            
				);

			if (res == pastdest)
				thwomp->direction = 0;  
			}
		else  
		{

			sides[thwomp->sourceline->sidenum[0]].midtexture = sides[thwomp->sourceline->sidenum[0]].toptexture;

			speed = thwomp->crushspeed;

			res = T_MovePlane
			(
				thwomp->sector,    
				speed,             
				P_FloorzAtPos(thwompx, thwompy, thwomp->sector->floorheight,
					thwomp->sector->ceilingheight - thwomp->sector->floorheight),  
				false,               
				false,               
				thwomp->direction  
			);

			if (res == planeok || res == pastdest)
				T_MovePlane
				(
					thwomp->sector,    
					speed,             
					P_FloorzAtPos(thwompx, thwompy, thwomp->sector->floorheight,
						thwomp->sector->ceilingheight
						- (thwomp->sector->floorheight + speed))
						+ (thwomp->sector->ceilingheight
						- (thwomp->sector->floorheight + speed/2)),  
					false,              
					true,               
					thwomp->direction  
				);

			if (res == pastdest)
			{
				if (rover->fofflags & FOF_EXISTS)
					S_StartSound((void *)&actionsector->soundorg, thwomp->sound);

				thwomp->direction = 1;  
				thwomp->delay = TICRATE;  
			}
		}

		thwomp->sector->ceilspeed = 42;
		thwomp->sector->floorspeed = speed*thwomp->direction;
	}

	P_RecalcPrecipInSector(actionsector);
}

static boolean T_SectorHasEnemies(sector_t *sec)
{
	msecnode_t *node = sec->touching_thinglist;  
	mobj_t *mo;
	while (node)
	{
		mo = node->m_thing;

		if ((mo->flags & (MF_ENEMY|MF_BOSS))
			&& mo->health > 0
			&& mo->z < sec->ceilingheight
			&& mo->z + mo->height > sec->floorheight)
			return true;

		node = node->m_thinglist_next;
	}

	return false;
}

static void T_UpdateMobjPlaneZ(sector_t *sec)
{
	msecnode_t *node = sec->touching_thinglist;  
	mobj_t *mo;
	while (node)
	{
		mo = node->m_thing;
		mo->floorz = P_FloorzAtPos(mo->x, mo->y, mo->z, mo->height);
		mo->ceilingz = P_CeilingzAtPos(mo->x, mo->y, mo->z, mo->height);
		node = node->m_thinglist_next;
	}
}







void T_NoEnemiesSector(noenemies_t *nobaddies)
{
	size_t i;
	sector_t *sec = NULL;
	INT32 secnum = -1;
	boolean FOFsector = false;
	mtag_t tag = nobaddies->sourceline->args[0];

	TAG_ITER_SECTORS(tag, secnum)
	{
		sec = &sectors[secnum];

		FOFsector = false;


		for (i = 0; i < sec->linecount; i++)
		{
			INT32 targetsecnum = -1;

			if (sec->lines[i]->special < 100 || sec->lines[i]->special >= 300)
				continue;

			FOFsector = true;

			TAG_ITER_SECTORS(sec->lines[i]->args[0], targetsecnum)
			{
				if (T_SectorHasEnemies(&sectors[targetsecnum]))
					return;
			}
		}

		if (!FOFsector && T_SectorHasEnemies(sec))
			return;
	}

	CONS_Debug(DBG_GAMELOGIC, "Running no-more-enemies exec with tag of %d\n", tag);


	P_RunTriggerLinedef(nobaddies->sourceline, NULL, NULL);
	P_RemoveThinker(&nobaddies->thinker);
}

static boolean P_CheckAllTrigger(eachtime_t *eachtime)
{
	size_t i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (P_CanPlayerTrigger(i) && !eachtime->playersInArea[i])
			return false;
	}

	return true;
}

void T_EachTimeThinker(eachtime_t *eachtime)
{
	size_t i;
	boolean oldPlayersInArea[MAXPLAYERS];
	sector_t *caller[MAXPLAYERS];
	boolean allPlayersChecked = false;
	boolean allPlayersTrigger = false;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		oldPlayersInArea[i] = eachtime->playersInArea[i];
		caller[i] = P_CanPlayerTrigger(i) ? P_FindPlayerTrigger(&players[i], eachtime->sourceline) : NULL;
		eachtime->playersInArea[i] = caller[i] != NULL;
	}


	if (!memcmp(eachtime->playersInArea, oldPlayersInArea, sizeof(boolean)*MAXPLAYERS))
		return;


	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (eachtime->playersInArea[i] == oldPlayersInArea[i])
			continue;


		if (!eachtime->playersInArea[i] && (!eachtime->triggerOnExit || !P_CanPlayerTrigger(i)))
			continue;


		if (caller[i] && caller[i]->triggerer == TO_ALLPLAYERS)
		{
			if (!allPlayersChecked)
			{
				allPlayersChecked = true;
				allPlayersTrigger = P_CheckAllTrigger(eachtime);
			}

			if (!allPlayersTrigger)
				continue;
		}

		CONS_Debug(DBG_GAMELOGIC, "Trying to activate each time executor with tag %d\n", Tag_FGet(&eachtime->sourceline->tags));





		P_RunTriggerLinedef(eachtime->sourceline, players[i].mo, caller[i]);

		if (!eachtime->sourceline->special)  
			P_RemoveThinker(&eachtime->thinker);
	}
}







void T_RaiseSector(raise_t *raise)
{
	msecnode_t *node;
	mobj_t *thing;
	sector_t *sector;
	INT32 i;
	boolean playeronme = false, active = false;
	boolean moveUp;
	fixed_t ceilingdestination, floordestination;
	fixed_t speed, origspeed;
	fixed_t distToNearestEndpoint;
	INT32 direction;
	result_e res = 0;
	mtag_t tag = raise->tag;

	if (raise->sector->crumblestate >= CRUMBLE_FALL || raise->sector->ceilingdata)
		return;

	TAG_ITER_SECTORS(tag, i)
	{
		sector = &sectors[i];


		for (node = sector->touching_thinglist; node; node = node->m_thinglist_next)
		{
			thing = node->m_thing;

			if (!thing->player)
				continue;


			if (thing->player && thing->player->spectator)
				continue;


			if (raise->flags & RF_SPINDASH && !(thing->player->pflags & PF_STARTDASH))
				continue;

			if (!(thing->z == P_GetSpecialTopZ(thing, raise->sector, sector)))
				continue;

			playeronme = true;
			break;
		}
	}

	if (raise->flags & RF_DYNAMIC)  
	{
#define shaketime 10
		if (raise->shaketimer > shaketime)  
		{
			if (playeronme)  
			{
				raise->extraspeed += (FRACUNIT >> 5);
			}
			else  
			{
				raise->extraspeed -= FRACUNIT >> 3;
				if (raise->extraspeed <= 0)  
				{
					raise->extraspeed = 0;
					raise->shaketimer = 0;  
				}
			}
			active = raise->extraspeed > 0;
		}
		else  
		{
			if (playeronme || raise->shaketimer)
			{
				active = true;
				if (++raise->shaketimer > shaketime)
				{
					if (playeronme)
						raise->extraspeed = FRACUNIT >> 5;
					else
						raise->extraspeed = FRACUNIT << 1;
				}
				else
				{
					raise->extraspeed = ((shaketime/2) - raise->shaketimer) << FRACBITS;
					if (raise->extraspeed < -raise->basespeed/2)
						raise->extraspeed = -raise->basespeed/2;
				}
			}
		}
#undef shaketime
	}
	else  
		active = playeronme;

	moveUp = active ^ (raise->flags & RF_REVERSE);
	ceilingdestination = moveUp ? raise->ceilingtop : raise->ceilingbottom;
	floordestination = ceilingdestination - (raise->sector->ceilingheight - raise->sector->floorheight);

	if ((moveUp && raise->sector->ceilingheight >= ceilingdestination)
		|| (!moveUp && raise->sector->ceilingheight <= ceilingdestination))
	{
		raise->sector->floorheight = floordestination;
		raise->sector->ceilingheight = ceilingdestination;
		raise->sector->ceilspeed = 0;
		raise->sector->floorspeed = 0;
		return;
	}
	direction = moveUp ? 1 : -1;

	origspeed = raise->basespeed;
	if (!active)
		origspeed /= 2;


	distToNearestEndpoint = min(raise->sector->ceilingheight - raise->ceilingbottom, raise->ceilingtop - raise->sector->ceilingheight);
	speed = FixedMul(origspeed, FixedDiv(distToNearestEndpoint, (raise->ceilingtop - raise->ceilingbottom) >> 5));

	if (speed <= origspeed/16)
		speed = origspeed/16;
	else if (speed > origspeed)
		speed = origspeed;

	speed += raise->extraspeed;

	res = T_MovePlane
	(
		raise->sector,       
		speed,               
		ceilingdestination,  
		false,               
		true,                
		direction            
	);

	if (res == planeok || res == pastdest)
		T_MovePlane
		(
			raise->sector,     
			speed,             
			floordestination,  
			false,             
			false,             
			direction          
		);

	raise->sector->ceilspeed = 42;
	raise->sector->floorspeed = speed*direction;

	TAG_ITER_SECTORS(tag, i)
		P_RecalcPrecipInSector(&sectors[i]);
}

void T_CameraScanner(elevator_t *elevator)
{

	static tic_t lastleveltime = 32000;  
	static boolean camerascanned, camerascanned2;

	if (leveltime != lastleveltime)  
	{
		camerascanned = camerascanned2 = false;
		lastleveltime = leveltime;
	}

	if (players[displayplayer].mo)
	{
		if (players[displayplayer].mo->subsector->sector == elevator->actionsector)
		{
			if (t_cam_dist == -42)
				t_cam_dist = cv_cam_dist.value;
			if (t_cam_height == -42)
				t_cam_height = cv_cam_height.value;
			if (t_cam_rotate == -42)
				t_cam_rotate = cv_cam_rotate.value;
			CV_SetValue(&cv_cam_height, FixedInt(elevator->sector->floorheight));
			CV_SetValue(&cv_cam_dist, FixedInt(elevator->sector->ceilingheight));
			CV_SetValue(&cv_cam_rotate, elevator->distance);
			camerascanned = true;
		}
		else if (!camerascanned)
		{
			if (t_cam_height != -42 && cv_cam_height.value != t_cam_height)
				CV_Set(&cv_cam_height, va("%f", (double)FIXED_TO_FLOAT(t_cam_height)));
			if (t_cam_dist != -42 && cv_cam_dist.value != t_cam_dist)
				CV_Set(&cv_cam_dist, va("%f", (double)FIXED_TO_FLOAT(t_cam_dist)));
			if (t_cam_rotate != -42 && cv_cam_rotate.value != t_cam_rotate)
				CV_Set(&cv_cam_rotate, va("%f", (double)t_cam_rotate));

			t_cam_dist = t_cam_height = t_cam_rotate = -42;
		}
	}

	if (splitscreen && players[secondarydisplayplayer].mo)
	{
		if (players[secondarydisplayplayer].mo->subsector->sector == elevator->actionsector)
		{
			if (t_cam2_rotate == -42)
				t_cam2_dist = cv_cam2_dist.value;
			if (t_cam2_rotate == -42)
				t_cam2_height = cv_cam2_height.value;
			if (t_cam2_rotate == -42)
				t_cam2_rotate = cv_cam2_rotate.value;
			CV_SetValue(&cv_cam2_height, FixedInt(elevator->sector->floorheight));
			CV_SetValue(&cv_cam2_dist, FixedInt(elevator->sector->ceilingheight));
			CV_SetValue(&cv_cam2_rotate, elevator->distance);
			camerascanned2 = true;
		}
		else if (!camerascanned2)
		{
			if (t_cam2_height != -42 && cv_cam2_height.value != t_cam2_height)
				CV_Set(&cv_cam2_height, va("%f", (double)FIXED_TO_FLOAT(t_cam2_height)));
			if (t_cam2_dist != -42 && cv_cam2_dist.value != t_cam2_dist)
				CV_Set(&cv_cam2_dist, va("%f", (double)FIXED_TO_FLOAT(t_cam2_dist)));
			if (t_cam2_rotate != -42 && cv_cam2_rotate.value != t_cam2_rotate)
				CV_Set(&cv_cam2_rotate, va("%f", (double)t_cam2_rotate));

			t_cam2_dist = t_cam2_height = t_cam2_rotate = -42;
		}
	}
}

void T_PlaneDisplace(planedisplace_t *pd)
{
	sector_t *control, *target;
	INT32 direction;
	fixed_t diff;

	control = &sectors[pd->control];
	target = &sectors[pd->affectee];

	if (control->floorheight == pd->last_height)
		return;  

	direction = (control->floorheight > pd->last_height) ? 1 : -1;
	diff = FixedMul(control->floorheight-pd->last_height, pd->speed);

	if (pd->reverse)  
	{
		direction *= -1;
		diff *= -1;
	}

	if (pd->type == pd_floor || pd->type == pd_both)
		T_MovePlane(target, INT32_MAX/2, target->floorheight+diff, false, false, direction);  
	if (pd->type == pd_ceiling || pd->type == pd_both)
		T_MovePlane(target, INT32_MAX/2, target->ceilingheight+diff, false, true, direction);  

	pd->last_height = control->floorheight;
}










void EV_DoFloor(mtag_t tag, line_t *line, floor_e floortype)
{
	INT32 firstone = 1;
	INT32 secnum = -1;
	sector_t *sec;
	floormove_t *dofloor;

	TAG_ITER_SECTORS(tag, secnum)
	{
		sec = &sectors[secnum];

		if (sec->floordata)  
			continue;  


		dofloor = Z_Calloc(sizeof (*dofloor), PU_LEVSPEC, NULL);
		P_AddThinker(THINK_MAIN, &dofloor->thinker);


		sec->floordata = dofloor;


		dofloor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
		dofloor->type = floortype;
		dofloor->crush = false;  
		dofloor->sector = sec;
		dofloor->sourceline = (INT32)(line - lines);

		switch (floortype)
		{

			case raiseFloorToNearestFast:
				dofloor->direction = -1;  
				dofloor->speed = FLOORSPEED*4;  
				dofloor->floordestheight = P_FindNextHighestFloor(sec, sec->floorheight);
				break;



			case instantLower:
				dofloor->direction = -1;  
				dofloor->speed = INT32_MAX/2;  
				dofloor->floordestheight = P_FindLowestFloorSurrounding(sec);
				break;

			case instantMoveFloorByFrontSector:
				dofloor->speed = INT32_MAX/2;  
				dofloor->floordestheight = line->frontsector->floorheight;

				if (dofloor->floordestheight >= sec->floorheight)
					dofloor->direction = 1;  
				else
					dofloor->direction = -1;  


				dofloor->texture = line->args[2] ? line->frontsector->floorpic : -1;
				break;

			case moveFloorByFrontSector:
				dofloor->speed = line->args[2] << (FRACBITS - 3);
				dofloor->floordestheight = line->frontsector->floorheight;

				if (dofloor->floordestheight >= sec->floorheight)
					dofloor->direction = 1;  
				else
					dofloor->direction = -1;  



				if (line->args[3])
					dofloor->tag = firstone ? (INT16)line->args[3] : 0;


				dofloor->texture = line->args[4] ? line->frontsector->floorpic : -1;
				break;

			case moveFloorByDistance:
				if (line->args[4])
					dofloor->speed = INT32_MAX/2;  
				else
					dofloor->speed = line->args[3] << (FRACBITS - 3);
				dofloor->floordestheight = sec->floorheight + (line->args[2] << FRACBITS);
				if (dofloor->floordestheight > sec->floorheight)
					dofloor->direction = 1;  
				else
					dofloor->direction = -1;  
				break;



			case bounceFloor:
			case bounceFloorCrush:
				dofloor->speed = line->args[2] << (FRACBITS - 2);  
				dofloor->origspeed = dofloor->speed;
				dofloor->floordestheight = line->frontsector->floorheight;

				if (dofloor->floordestheight >= sec->floorheight)
					dofloor->direction = 1;  
				else
					dofloor->direction = -1;  


				dofloor->delay = line->args[5];
				dofloor->delaytimer = line->args[4];  
				break;

			case crushFloorOnce:
				dofloor->speed = dofloor->origspeed = line->args[2] << (FRACBITS - 2);
				dofloor->floordestheight = line->frontsector->ceilingheight;

				if (dofloor->floordestheight >= sec->floorheight)
					dofloor->direction = 1;  
				else
					dofloor->direction = -1;  
				break;

			default:
				break;
		}

		firstone = 0;


		R_CreateInterpolator_SectorPlane(&dofloor->thinker, sec, false);
	}
}











void EV_DoElevator(mtag_t tag, line_t *line, elevator_e elevtype)
{
	INT32 secnum = -1;
	sector_t *sec;
	elevator_t *elevator;


	TAG_ITER_SECTORS(tag, secnum)
	{
		sec = &sectors[secnum];


		if (sec->floordata || sec->ceilingdata)
			continue;


		elevator = Z_Calloc(sizeof (*elevator), PU_LEVSPEC, NULL);
		P_AddThinker(THINK_MAIN, &elevator->thinker);
		sec->floordata = elevator;
		sec->ceilingdata = elevator;
		elevator->thinker.function.acp1 = (actionf_p1)T_MoveElevator;
		elevator->type = elevtype;
		elevator->sourceline = line;
		elevator->distance = 1;  
		elevator->sector = sec;


		switch (elevtype)
		{

			case elevateDown:
				elevator->direction = -1;
				elevator->speed = ELEVATORSPEED/2;  
				elevator->floordestheight = P_FindNextLowestFloor(sec, sec->floorheight);
				break;


			case elevateUp:
				elevator->direction = 1;
				elevator->speed = ELEVATORSPEED/4;  
				elevator->floordestheight = P_FindNextHighestFloor(sec, sec->floorheight);
				break;


			case elevateHighest:
				elevator->direction = 1;
				elevator->speed = ELEVATORSPEED/4;  
				elevator->floordestheight = P_FindHighestFloorSurrounding(sec);
				break;

			case elevateContinuous:
				elevator->origspeed = line->args[1] << (FRACBITS - 2);
				elevator->speed = elevator->origspeed;

				elevator->low = !line->args[4];  
				if (elevator->low)
				{
					elevator->direction = 1;
					elevator->floordestheight = P_FindNextHighestFloor(sec, sec->floorheight);
				}
				else
				{
					elevator->direction = -1;
					elevator->floordestheight = P_FindNextLowestFloor(sec,sec->floorheight);
				}
				elevator->floorwasheight = elevator->sector->floorheight;
				elevator->ceilingwasheight = elevator->sector->ceilingheight;

				elevator->delay = line->args[3];
				elevator->delaytimer = line->args[2];  
				break;

			case bridgeFall:
				elevator->direction = -1;
				elevator->speed = ELEVATORSPEED*4;  
				elevator->floordestheight = P_FindNextLowestFloor(sec, sec->floorheight);
				break;

			default:
				break;
		}

		elevator->ceilingdestheight = elevator->floordestheight + sec->ceilingheight - sec->floorheight;


		R_CreateInterpolator_SectorPlane(&elevator->thinker, sec, false);
		R_CreateInterpolator_SectorPlane(&elevator->thinker, sec, true);
	}
}

void EV_CrumbleChain(sector_t *sec, ffloor_t *rover)
{
	size_t i, leftmostvertex, rightmostvertex, topmostvertex, bottommostvertex;
	fixed_t leftx, rightx, topy, bottomy, topz, bottomz, widthfactor, heightfactor, a, b, c, spacing;
	mobjtype_t type;
	tic_t lifetime;
	boolean fromcenter;

	sector_t *controlsec = rover->master->frontsector;
	mtag_t tag = Tag_FGet(&controlsec->tags);

	if (sec == NULL)
	{
		if (controlsec->numattached)
		{
			for (i = 0; i < controlsec->numattached; i++)
			{
				sec = &sectors[controlsec->attached[i]];
				if (!sec->ffloors)
					continue;

				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if (rover->master->frontsector == controlsec)
						EV_CrumbleChain(sec, rover);
				}
			}
		}
		return;
	}

	leftmostvertex = rightmostvertex = topmostvertex = bottommostvertex = 0;
	widthfactor = heightfactor = FRACUNIT;
	spacing = (32<<FRACBITS);
	type = MT_ROCKCRUMBLE1;
	lifetime = 3*TICRATE;
	fromcenter = false;

	if (tag != 0)
	{
		INT32 tagline = Tag_FindLineSpecial(14, tag);
		if (tagline != -1)
		{
			if (lines[tagline].stringargs[0])
				type = get_number(lines[tagline].stringargs[0]);
			if (lines[tagline].args[0])
				spacing = lines[tagline].args[0] << FRACBITS;
			if (lines[tagline].args[1])
				lifetime = (lines[tagline].args[1] != -1) ? lines[tagline].args[1] : 0;
			fromcenter = !!lines[tagline].args[2];
		}
	}

	sec->soundorg.z = (controlsec->floorheight + controlsec->ceilingheight)/2;
	S_StartSound(&sec->soundorg, mobjinfo[type].activesound);

#undef controlsec


	for (i = 0; i < sec->linecount; i++)
	{

		if ((sec->lines[i]->v1->x < sec->lines[leftmostvertex]->v1->x))
			leftmostvertex = i;

		if ((sec->lines[i]->v1->x > sec->lines[rightmostvertex]->v1->x))
			rightmostvertex = i;

		if ((sec->lines[i]->v1->y > sec->lines[topmostvertex]->v1->y))
			topmostvertex = i;

		if ((sec->lines[i]->v1->y < sec->lines[bottommostvertex]->v1->y))
			bottommostvertex = i;
	}

	leftx = sec->lines[leftmostvertex]->v1->x+(spacing>>1);
	rightx = sec->lines[rightmostvertex]->v1->x;
	topy = sec->lines[topmostvertex]->v1->y-(spacing>>1);
	bottomy = sec->lines[bottommostvertex]->v1->y;

	topz = *rover->topheight-(spacing>>1);
	bottomz = *rover->bottomheight;

	if (fromcenter)
	{
		widthfactor = (rightx + topy - leftx - bottomy)>>3;
		heightfactor = (topz - *rover->bottomheight)>>2;
	}

	for (a = leftx; a < rightx; a += spacing)
	{
		for (b = topy; b > bottomy; b -= spacing)
		{
			if (R_PointInSubsector(a, b)->sector == sec)
			{
				mobj_t *spawned = NULL;
				if (*rover->t_slope)
					topz = P_GetSlopeZAt(*rover->t_slope, a, b) - (spacing>>1);
				if (*rover->b_slope)
					bottomz = P_GetSlopeZAt(*rover->b_slope, a, b);

				for (c = topz; c > bottomz; c -= spacing)
				{
					spawned = P_SpawnMobj(a, b, c, type);
					if (P_MobjWasRemoved(spawned))
						continue;

					spawned->angle += P_RandomKey(36)*ANG10;  

					if (fromcenter)
					{
						P_InstaThrust(spawned, R_PointToAngle2(sec->soundorg.x, sec->soundorg.y, a, b), FixedDiv(P_AproxDistance(a - sec->soundorg.x, b - sec->soundorg.y), widthfactor));
						P_SetObjectMomZ(spawned, FixedDiv((c - bottomz), heightfactor), false);
					}

					spawned->fuse = lifetime;
				}
			}
		}
	}


	rover->fofflags &= ~FOF_EXISTS;
	rover->master->frontsector->moved = true;
	T_UpdateMobjPlaneZ(sec);  
	P_RecalcPrecipInSector(sec);
}


void EV_BounceSector(sector_t *sec, fixed_t momz, line_t *sourceline)
{
	bouncecheese_t *bouncer;


	if (sec->ceilingdata)  
		return;

	bouncer = Z_Calloc(sizeof (*bouncer), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &bouncer->thinker);
	sec->ceilingdata = bouncer;
	bouncer->thinker.function.acp1 = (actionf_p1)T_BounceCheese;


	bouncer->sourceline = sourceline;
	bouncer->sector = sec;
	bouncer->speed = momz/2;
	bouncer->distance = FRACUNIT;
	bouncer->low = true;


	R_CreateInterpolator_SectorPlane(&bouncer->thinker, sec, false);
	R_CreateInterpolator_SectorPlane(&bouncer->thinker, sec, true);
}


void EV_DoContinuousFall(sector_t *sec, sector_t *backsector, fixed_t spd, boolean backwards)
{
	continuousfall_t *faller;


	if (!backsector)
		backsector = sec;


	faller = Z_Calloc(sizeof (*faller), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &faller->thinker);
	faller->thinker.function.acp1 = (actionf_p1)T_ContinuousFalling;


	faller->sector = sec;
	faller->speed = spd;

	faller->floorstartheight = sec->floorheight;
	faller->ceilingstartheight = sec->ceilingheight;

	faller->destheight = backwards ? backsector->ceilingheight : backsector->floorheight;
	faller->direction = backwards ? 1 : -1;


	R_CreateInterpolator_SectorPlane(&faller->thinker, sec, false);
	R_CreateInterpolator_SectorPlane(&faller->thinker, sec, true);
}


INT32 EV_StartCrumble(sector_t *sec, ffloor_t *rover, boolean floating,
	player_t *player, fixed_t origalpha, boolean crumblereturn)
{
	crumble_t *crumble;
	sector_t *foundsec;
	INT32 i;
	mtag_t tag = rover->master->args[0];


	if (sec->floordata)
		return 0;

	if (sec->crumblestate >= CRUMBLE_ACTIVATED)
		return 0;


	crumble = Z_Calloc(sizeof (*crumble), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &crumble->thinker);
	crumble->thinker.function.acp1 = (actionf_p1)T_StartCrumble;


	crumble->sector = sec;
	crumble->speed = 0;

	if (player && player->mo && (player->mo->eflags & MFE_VERTICALFLIP))
	{
		crumble->direction = 1;  
		crumble->flags |= CF_REVERSE;
	}
	else
		crumble->direction = -1;  

	crumble->floorwasheight = crumble->sector->floorheight;
	crumble->ceilingwasheight = crumble->sector->ceilingheight;
	crumble->timer = TICRATE;
	crumble->player = player;
	crumble->origalpha = origalpha;

	crumble->sourceline = rover->master;

	sec->floordata = crumble;

	if (crumblereturn)
		crumble->flags |= CF_RETURN;
	if (floating)
		crumble->flags |= CF_FLOATBOB;

	crumble->sector->crumblestate = CRUMBLE_ACTIVATED;


	R_CreateInterpolator_SectorPlane(&crumble->thinker, sec, false);
	R_CreateInterpolator_SectorPlane(&crumble->thinker, sec, true);

	TAG_ITER_SECTORS(tag, i)
	{
		foundsec = &sectors[i];

		P_SpawnMobj(foundsec->soundorg.x, foundsec->soundorg.y, crumble->direction == 1 ? crumble->sector->floorheight : crumble->sector->ceilingheight, MT_CRUMBLEOBJ);
	}

	return 1;
}

void EV_MarioBlock(ffloor_t *rover, sector_t *sector, mobj_t *puncher)
{
	sector_t *roversec = rover->master->frontsector;
	fixed_t topheight = *rover->topheight;
	mariothink_t *block;
	mobj_t *thing;
	fixed_t oldx = 0, oldy = 0, oldz = 0;

	I_Assert(puncher != NULL);
	I_Assert(puncher->player != NULL);

	if (roversec->floordata || roversec->ceilingdata)
		return;

	if (!(rover->fofflags & FOF_SOLID))
		rover->fofflags |= (FOF_SOLID|FOF_RENDERALL|FOF_CUTLEVEL);


	thing = SearchMarioNode(roversec->touching_thinglist);

	if (!thing)
		S_StartSound(puncher, sfx_mario1);  
	else  
	{
		const boolean itsamonitor = (thing->flags & MF_MONITOR) == MF_MONITOR;


		block = Z_Calloc(sizeof (*block), PU_LEVSPEC, NULL);
		P_AddThinker(THINK_MAIN, &block->thinker);
		roversec->floordata = block;
		roversec->ceilingdata = block;
		block->thinker.function.acp1 = (actionf_p1)T_MarioBlock;


		block->sector = roversec;
		block->speed = 4*FRACUNIT;
		block->direction = 1;
		block->floorstartheight = block->sector->floorheight;
		block->ceilingstartheight = block->sector->ceilingheight;
		block->tag = (INT16)rover->master->args[0];


		R_CreateInterpolator_SectorPlane(&block->thinker, roversec, false);
		R_CreateInterpolator_SectorPlane(&block->thinker, roversec, true);

		if (itsamonitor)
		{
			oldx = thing->x;
			oldy = thing->y;
			oldz = thing->z;
		}

		P_UnsetThingPosition(thing);
		thing->x = thing->old_x = sector->soundorg.x;
		thing->y = thing->old_y = sector->soundorg.y;
		thing->z = thing->old_z = topheight;
		thing->momz = FixedMul(6*FRACUNIT, thing->scale);
		P_SetThingPosition(thing);
		if (thing->flags & MF_SHOOTABLE)
			P_DamageMobj(thing, puncher, puncher, 1, 0);
		else if (thing->type == MT_RING || thing->type == MT_COIN || thing->type == MT_TOKEN)
		{
			thing->momz = FixedMul(3*FRACUNIT, thing->scale);
			P_TouchSpecialThing(thing, puncher, false);

			S_StartSound(puncher, sfx_mario1);  
		}
		else
		{

			S_StartSound(puncher, sfx_mario9);  
		}

		if (itsamonitor && thing)
		{
			P_UnsetThingPosition(thing);
			thing->x = thing->old_x = oldx;
			thing->y = thing->old_y = oldy;
			thing->z = thing->old_z = oldz;
			thing->momx = 1;
			thing->momy = 1;
			P_SetThingPosition(thing);
		}
	}
}
