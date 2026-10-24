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
#include "p_local.h"
#include "r_state.h"
#include "z_zone.h"
#include "m_random.h"
#include "netcode/d_netcmd.h"

                                                    
   
                                                    
    
void P_RemoveLighting(sector_t *sector)
{
	if (sector->lightingdata)
	{



		P_RemoveThinker(&((thinkerdata_t *)sector->lightingdata)->thinker);
		sector->lightingdata = NULL;
	}
}





                                      
   
                                                  
                                    
    
void T_FireFlicker(fireflicker_t *flick)
{
	INT16 amount;

	if (--flick->count)
		return;

	amount = (INT16)((UINT8)(P_RandomByte() & 3) * 16);

	if (flick->sector->lightlevel - amount < flick->minlight)
		flick->sector->lightlevel = flick->minlight;
	else
		flick->sector->lightlevel = flick->maxlight - amount;

	flick->count = flick->resetcount;
}

                                                         
   
                                                  
                                                                 
                                           
                                                                    
                     
    
fireflicker_t *P_SpawnAdjustableFireFlicker(sector_t *sector, INT16 lighta, INT16 lightb, INT32 length)
{
	fireflicker_t *flick;

	P_RemoveLighting(sector);  
	flick = Z_Calloc(sizeof (*flick), PU_LEVSPEC, NULL);

	P_AddThinker(THINK_MAIN, &flick->thinker);

	flick->thinker.function.acp1 = (actionf_p1)T_FireFlicker;
	flick->sector = sector;
	flick->maxlight = max(lighta, lightb);
	flick->minlight = min(lighta, lightb);
	flick->count = flick->resetcount = length/4;
	sector->lightingdata = flick;


	if (!flick->resetcount)
		flick->resetcount = 1;
	if (flick->minlight == flick->maxlight)
	{
		if (flick->minlight > 0)
			flick->minlight--;
		if (flick->maxlight < 255)
			flick->maxlight++;
	}


	sector->lightlevel = max(flick->minlight, min(flick->maxlight, sector->lightlevel));

	return flick;
}





                                                        
   
                                             
                             
    
void T_LightningFlash(lightflash_t *flash)
{
	flash->sector->lightlevel -= 4;

	if (flash->sector->lightlevel <= flash->minlight)
	{
		flash->sector->lightlevel = (INT16)flash->minlight;
		P_RemoveLighting(flash->sector);
	}
}

                                      
   
                                     
                        
    
void P_SpawnLightningFlash(sector_t *sector)
{
	INT32 minlight;
	lightflash_t *flash;

	minlight = sector->lightlevel;

	if (sector->lightingdata)
	{
		if (((lightflash_t *)sector->lightingdata)->thinker.function.acp1
			== (actionf_p1)T_LightningFlash)
		{


			minlight = ((lightflash_t *)sector->lightingdata)->minlight;
		}

		P_RemoveThinker(&((thinkerdata_t *)sector->lightingdata)->thinker);
	}

	sector->lightingdata = NULL;

	flash = Z_Calloc(sizeof (*flash), PU_LEVSPEC, NULL);

	P_AddThinker(THINK_MAIN, &flash->thinker);

	flash->thinker.function.acp1 = (actionf_p1)T_LightningFlash;
	flash->sector = sector;
	flash->maxlight = 255;
	flash->minlight = minlight;
	sector->lightlevel = (INT16)flash->maxlight;

	sector->lightingdata = flash;
}





                                               
   
                                                
                                    
    
void T_StrobeFlash(strobe_t *flash)
{
	if (--flash->count)
		return;

	if (flash->sector->lightlevel == flash->minlight)
	{
		flash->sector->lightlevel = flash->maxlight;
		flash->count = flash->brighttime;
	}
	else
	{
		flash->sector->lightlevel = flash->minlight;
		flash->count = flash->darktime;
	}
}

                                                         
   
                                                   
                                                                  
                                            
                                                            
                                                              
                                                              
                                                               
                                                        
                                                               
                                                            
                                                 
                     
    
strobe_t *P_SpawnAdjustableStrobeFlash(sector_t *sector, INT16 lighta, INT16 lightb, INT32 darktime, INT32 brighttime, boolean inSync)
{
	strobe_t *flash;

	P_RemoveLighting(sector);  
	flash = Z_Calloc(sizeof (*flash), PU_LEVSPEC, NULL);

	P_AddThinker(THINK_MAIN, &flash->thinker);

	flash->sector = sector;
	flash->darktime = darktime;
	flash->brighttime = brighttime;
	flash->thinker.function.acp1 = (actionf_p1)T_StrobeFlash;
	flash->maxlight = max(lighta, lightb);
	flash->minlight = min(lighta, lightb);

	if (flash->minlight == flash->maxlight)
		flash->minlight = 0;

	if (!inSync)
		flash->count = (P_RandomByte() & 7) + 1;
	else
		flash->count = 1;


	sector->lightlevel = max(flash->minlight, min(flash->maxlight, sector->lightlevel));

	sector->lightingdata = flash;
	return flash;
}

                                       
   
                                              
                                     
    
void T_Glow(glow_t *g)
{
	switch (g->direction)
	{
		case -1:

			g->sector->lightlevel -= g->speed;
			if (g->sector->lightlevel <= g->minlight)
			{
				g->sector->lightlevel += g->speed;
				g->direction = 1;
			}
			break;

		case 1:

			g->sector->lightlevel += g->speed;
			if (g->sector->lightlevel >= g->maxlight)
			{
				g->sector->lightlevel -= g->speed;
				g->direction = -1;
			}
			break;
	}
}

                                                          
   
                                                  
                                                                 
                                           
                                             
              
    
glow_t *P_SpawnAdjustableGlowingLight(sector_t *sector, INT16 lighta, INT16 lightb, INT32 length)
{
	glow_t *g;

	P_RemoveLighting(sector);  
	g = Z_Calloc(sizeof (*g), PU_LEVSPEC, NULL);

	P_AddThinker(THINK_MAIN, &g->thinker);

	g->sector = sector;
	g->minlight = min(lighta, lightb);
	g->maxlight = max(lighta, lightb);
	g->thinker.function.acp1 = (actionf_p1)T_Glow;
	g->direction = 1;
	g->speed = (INT16)(length/4);
	if (g->speed > (g->maxlight - g->minlight)/2)  
		g->speed = (g->maxlight - g->minlight)/2;

	while (g->speed < 1)
	{
		if (g->minlight > 0)
			g->minlight--;
		if (g->maxlight < 255)
			g->maxlight++;

		g->speed = (g->maxlight - g->minlight)/2;
	}


	sector->lightlevel = max(g->minlight, min(g->maxlight, sector->lightlevel));

	sector->lightingdata = g;

	return g;
}

                                                     
          
   
                                  
                                                            
                                                            
                                                                              
                                                  
                                                                           
                   
    
void P_FadeLightBySector(sector_t *sector, INT32 destvalue, INT32 speed, boolean ticbased)
{
	lightlevel_t *ll;

	P_RemoveLighting(sector);  

	if ((ticbased && !speed) || sector->lightlevel == destvalue)  
	{
		sector->lightlevel = destvalue;
		return;
	}

	ll = Z_Calloc(sizeof (*ll), PU_LEVSPEC, NULL);
	ll->thinker.function.acp1 = (actionf_p1)T_LightFade;
	sector->lightingdata = ll;  

	P_AddThinker(THINK_MAIN, &ll->thinker);  

	ll->sector = sector;
	ll->sourcelevel = sector->lightlevel;
	ll->destlevel = destvalue;

	ll->fixedcurlevel = sector->lightlevel<<FRACBITS;

	if (ticbased)
	{

		ll->timer = abs(speed);
		ll->fixedpertic = FixedDiv((destvalue<<FRACBITS) - ll->fixedcurlevel, speed<<FRACBITS);
	}
	else
	{

		ll->timer = abs(FixedDiv((destvalue<<FRACBITS) - ll->fixedcurlevel, speed<<FRACBITS)>>FRACBITS);
		ll->fixedpertic = ll->destlevel < ll->sourcelevel ? -speed<<FRACBITS : speed<<FRACBITS;
	}
}

void P_FadeLight(INT16 tag, INT32 destvalue, INT32 speed, boolean ticbased, boolean force, boolean relative)
{
	INT32 i;
	INT32 realdestvalue;


	TAG_ITER_SECTORS(tag, i)
	{
		if (!force && ticbased  
			&& sectors[i].lightingdata
			&& ((lightlevel_t*)sectors[i].lightingdata)->thinker.function.acp1 == (actionf_p1)T_LightFade)

		{
			CONS_Debug(DBG_GAMELOGIC, "Line type 420 Executor: Fade light thinker already exists, timer: %d\n", ((lightlevel_t*)sectors[i].lightingdata)->timer);
			continue;
		}

		realdestvalue = relative ? max(0, min(255, sectors[i].lightlevel + destvalue)) : destvalue;
		P_FadeLightBySector(&sectors[i], realdestvalue, speed, ticbased);
	}
}

                                                     
   
                                                  
                   
    
void T_LightFade(lightlevel_t *ll)
{
	if (--ll->timer <= 0)
	{
		ll->sector->lightlevel = ll->destlevel;  
		P_RemoveLighting(ll->sector);  
		return;
	}

	ll->fixedcurlevel = ll->fixedcurlevel + ll->fixedpertic;
	ll->sector->lightlevel = (ll->fixedcurlevel)>>FRACBITS;
}
