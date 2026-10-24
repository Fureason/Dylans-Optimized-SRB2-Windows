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







#include "dehacked.h"
#include "doomdef.h"
#include "g_game.h"
#include "p_local.h"
#include "p_setup.h"  
#include "r_data.h"
#include "r_fps.h"
#include "r_textures.h"
#include "m_random.h"
#include "p_mobj.h"
#include "i_system.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_main.h"  
#include "r_sky.h"
#include "p_polyobj.h"
#include "p_slopes.h"
#include "hu_stuff.h"
#include "v_video.h"  
#include "m_misc.h"
#include "m_cond.h"  
#include "lua_hook.h"  
#include "f_finale.h"  
#include "r_skins.h"  

#ifdef HW3SOUND
#include "hardware/hw3sound.h"
#endif


#include <errno.h>

mobj_t *skyboxmo[2];  
mobj_t *skyboxviewpnts[16];  
mobj_t *skyboxcenterpnts[16];  

size_t secportalcount;
size_t secportalcapacity;
sectorportal_t *secportals;

                               
                                                                
                                                   
    
typedef struct
{
	INT32 picnum;     
	INT32 basepic;    
	INT32 numpics;    
	tic_t speed;      
} anim_t;

                                
                                                 
   
                                                                               
                                                                             
                               
   
              
    
typedef struct
{
	SINT8 istexture;  
	char endname[9];  
	char startname[9];  
	INT32 speed;  
} ATTRPACK animdef_t;

typedef struct
{
	UINT32 count;
	thinker_t **thinkers;
} thinkerlist_t;

static void P_SpawnScrollers(void);
static void P_SpawnFriction(void);
static void P_SpawnPushers(void);
static void Add_Pusher(pushertype_e type, fixed_t x_mag, fixed_t y_mag, fixed_t z_mag, INT32 affectee, INT32 referrer, INT32 exclusive, INT32 slider);  
static void Add_MasterDisappearer(tic_t appeartime, tic_t disappeartime, tic_t offset, INT32 line, INT32 sourceline);
static void P_ResetFakeFloorFader(ffloor_t *rover, fade_t *data, boolean finalize);
#define P_RemoveFakeFloorFader(l) P_ResetFakeFloorFader(l, NULL, false);
static boolean P_FadeFakeFloor(ffloor_t *rover, INT16 sourcevalue, INT16 destvalue, INT16 speed, boolean ticbased, INT32 *timer,
	boolean doexists, boolean dotranslucent, boolean dolighting, boolean docolormap,
	boolean docollision, boolean doghostfade, boolean exactalpha);
static void P_AddFakeFloorFader(ffloor_t *rover, size_t sectornum, size_t ffloornum,
	INT16 destvalue, INT16 speed, boolean ticbased, boolean relative,
	boolean doexists, boolean dotranslucent, boolean dolighting, boolean docolormap,
	boolean docollision, boolean doghostfade, boolean exactalpha);
static void P_ResetColormapFader(sector_t *sector);
static void Add_ColormapFader(sector_t *sector, extracolormap_t *source_exc, extracolormap_t *dest_exc,
	boolean ticbased, INT32 duration);
static void P_AddBlockThinker(sector_t *sec, line_t *sourceline);
static void P_AddFloatThinker(sector_t *sec, UINT16 tag, line_t *sourceline);

static void P_AddFakeFloorsByLine(size_t line, INT32 alpha, UINT8 blendmode, ffloortype_e ffloorflags, thinkerlist_t *secthinkers);
static void P_ProcessLineSpecial(line_t *line, mobj_t *mo, sector_t *callsec);
static void Add_Friction(INT32 friction, INT32 movefactor, INT32 affectee, INT32 referrer);
static void P_AddPlaneDisplaceThinker(INT32 type, fixed_t speed, INT32 control, INT32 affectee, UINT8 reverse);



static anim_t *lastanim;
static anim_t *anims = NULL;  
static size_t maxanims;






static animdef_t *animdefs = NULL;


void P_ParseANIMDEFSLump(INT32 wadNum, UINT16 lumpnum);
void P_ParseAnimationDefintion(SINT8 istexture);

static boolean P_FindTextureForAnimation(anim_t *anim, animdef_t *animdef)
{
	if (R_CheckTextureNumForName(animdef->startname) == -1)
		return false;

	anim->picnum = R_TextureNumForName(animdef->endname);
	anim->basepic = R_TextureNumForName(animdef->startname);

	return true;
}

static boolean P_FindFlatForAnimation(anim_t *anim, animdef_t *animdef)
{
	if (R_CheckFlatNumForName(animdef->startname) == -1)
		return false;

	anim->picnum = R_CheckFlatNumForName(animdef->endname);
	anim->basepic = R_CheckFlatNumForName(animdef->startname);

	return true;
}

                                        
   
                                                         
                    
   
                                                        
   
                                                                                                                                                             
    
void P_InitPicAnims(void)
{

	INT32 w;  
	size_t i;

	I_Assert(animdefs == NULL);

	maxanims = 0;

	for (w = numwadfiles-1; w >= 0; w--)
	{
		UINT16 animdefsLumpNum;


		animdefsLumpNum = W_CheckNumForNamePwad("ANIMDEFS", w, 0);

		while (animdefsLumpNum != INT16_MAX)
		{
			P_ParseANIMDEFSLump(w, animdefsLumpNum);
			animdefsLumpNum = W_CheckNumForNamePwad("ANIMDEFS", (UINT16)w, animdefsLumpNum + 1);
		}
	}


	animdefs[maxanims].istexture = -1;
	strncpy(animdefs[maxanims].endname, "", 9);
	strncpy(animdefs[maxanims].startname, "", 9);
	animdefs[maxanims].speed = 0;

	if (anims)
		free(anims);

	anims = (anim_t *)malloc(sizeof (*anims)*(maxanims + 1));
	if (!anims)
		I_Error("Not enough free memory for ANIMDEFS data");

	lastanim = anims;
	for (i = 0; animdefs[i].istexture != -1; i++)
	{
		animdef_t *animdef = &animdefs[i];


		if (animdefs[i].istexture)
		{
			if (!P_FindTextureForAnimation(lastanim, animdef))
			{
				if (!P_FindFlatForAnimation(lastanim, animdef))
					continue;
			}
		}

		else
		{
			if (!P_FindFlatForAnimation(lastanim, animdef))
			{
				if (!P_FindTextureForAnimation(lastanim, animdef))
					continue;
			}
		}

		lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;

		if (lastanim->numpics < 2)
		{
			free(anims);
			I_Error("P_InitPicAnims: bad cycle from %s to %s",
				animdef->startname, animdef->endname);
		}

		lastanim->speed = animdef->speed;
		lastanim++;
	}
	R_ClearTextureNumCache(false);



	Z_Free(animdefs);
	animdefs = NULL;
}

                                                                              
                                                                               
                                                                             
                                                                   
void P_ExpandTextureAnimationPresence(UINT8 *present, size_t count)
{
	anim_t *anim;

	if (!present || !anims || !lastanim)
		return;

	for (anim = anims; anim < lastanim; anim++)
	{
		UINT8 variants = 0;
		INT32 i;

		if (anim->basepic < 0 || anim->numpics <= 0
		|| (size_t)anim->basepic >= count
		|| (size_t)anim->numpics > count - (size_t)anim->basepic)
			continue;

		for (i = 0; i < anim->numpics; i++)
			variants |= present[anim->basepic + i];
		if (!variants)
			continue;
		for (i = 0; i < anim->numpics; i++)
			present[anim->basepic + i] |= variants;
	}
}

void P_ParseANIMDEFSLump(INT32 wadNum, UINT16 lumpnum)
{
	char *animdefsLump;
	size_t animdefsLumpLength;
	char *animdefsText;
	char *animdefsToken;
	char *p;




	animdefsLump = (char *)W_CacheLumpNumPwad(wadNum,lumpnum,PU_STATIC);

	if (animdefsLump == NULL) return;

	animdefsLumpLength = W_LumpLengthPwad(wadNum,lumpnum);
	animdefsText = (char *)Z_Malloc((animdefsLumpLength+1)*sizeof(char),PU_STATIC,NULL);

	memmove(animdefsText,animdefsLump,animdefsLumpLength);

	animdefsText[animdefsLumpLength] = '\0';


	Z_Free(animdefsLump);


	p = animdefsText;
	animdefsToken = M_GetToken(p);
	while (animdefsToken != NULL)
	{
		if (stricmp(animdefsToken, "TEXTURE") == 0)
		{
			Z_Free(animdefsToken);
			P_ParseAnimationDefintion(1);
		}
		else if (stricmp(animdefsToken, "FLAT") == 0)
		{
			Z_Free(animdefsToken);
			P_ParseAnimationDefintion(0);
		}
		else if (stricmp(animdefsToken, "OSCILLATE") == 0)
		{

			I_Error("Error parsing ANIMDEFS lump: Animation definitions utilizing \"OSCILLATE\" (the animation plays in reverse when it reaches the end) are not supported by SRB2");
		}
		else
		{
			I_Error("Error parsing ANIMDEFS lump: Expected \"TEXTURE\" or \"FLAT\", got \"%s\"",animdefsToken);
		}

		while (*p != '\0' && *p != '\n') ++p;
		if (*p == '\n') ++p;
		animdefsToken = M_GetToken(p);
	}
	Z_Free(animdefsToken);
	Z_Free((void *)animdefsText);
}

void P_ParseAnimationDefintion(SINT8 istexture)
{
	char *animdefsToken;
	size_t animdefsTokenLength;
	char *endPos;
	INT32 animSpeed;
	size_t i;


	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where start texture/flat name should be");
	}
	if (stricmp(animdefsToken, "OPTIONAL") == 0)
	{



		Z_Free(animdefsToken);
		animdefsToken = M_GetToken(NULL);

		if (animdefsToken == NULL)
		{
			I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where start texture/flat name should be");
		}
		else if (stricmp(animdefsToken, "RANGE") == 0)
		{




			I_Error("Error parsing ANIMDEFS lump: \"OPTIONAL\" is a keyword; you cannot use it as the startname of an animation");
		}
	}
	animdefsTokenLength = strlen(animdefsToken);
	if (animdefsTokenLength>8)
	{
		I_Error("Error parsing ANIMDEFS lump: lump name \"%s\" exceeds 8 characters", animdefsToken);
	}


	for (i = 0; i < maxanims; i++)
		if (animdefs[i].istexture == istexture  
		&& stricmp(animdefsToken, animdefs[i].startname) == 0)
		{




			Z_Free(animdefsToken);
			return;
		}


	if (i == maxanims)
	{

		maxanims++;
		animdefs = (animdef_t *)Z_Realloc(animdefs, sizeof(animdef_t)*(maxanims + 1), PU_STATIC, NULL);
		strncpy(animdefs[i].startname, animdefsToken, sizeof(animdefs[i].startname)-1);
	}


	Z_Free(animdefsToken);


	animdefs[i].istexture = istexture;


	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where \"RANGE\" after \"%s\"'s startname should be", animdefs[i].startname);
	}
	if (stricmp(animdefsToken, "ALLOWDECALS") == 0)
	{

		Z_Free(animdefsToken);
		animdefsToken = M_GetToken(NULL);
	}
	if (stricmp(animdefsToken, "PIC") == 0)
	{

		I_Error("Error parsing ANIMDEFS lump: Animation definitions utilizing \"PIC\" (specific frames instead of a consecutive range) are not supported by SRB2");
	}
	if (stricmp(animdefsToken, "RANGE") != 0)
	{
		I_Error("Error parsing ANIMDEFS lump: Expected \"RANGE\" after \"%s\"'s startname, got \"%s\"", animdefs[i].startname, animdefsToken);
	}
	Z_Free(animdefsToken);


	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where \"%s\"'s end texture/flat name should be", animdefs[i].startname);
	}
	animdefsTokenLength = strlen(animdefsToken);
	if (animdefsTokenLength>8)
	{
		I_Error("Error parsing ANIMDEFS lump: lump name \"%s\" exceeds 8 characters", animdefsToken);
	}
	strncpy(animdefs[i].endname, animdefsToken, 9);
	Z_Free(animdefsToken);


	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where \"%s\"'s \"TICS\" should be", animdefs[i].startname);
	}
	if (stricmp(animdefsToken, "RAND") == 0)
	{

		I_Error("Error parsing ANIMDEFS lump: Animation definitions utilizing \"RAND\" (random duration per frame) are not supported by SRB2");
	}
	if (stricmp(animdefsToken, "TICS") != 0)
	{
		I_Error("Error parsing ANIMDEFS lump: Expected \"TICS\" in animation definition for \"%s\", got \"%s\"", animdefs[i].startname, animdefsToken);
	}
	Z_Free(animdefsToken);


	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where \"%s\"'s animation speed should be", animdefs[i].startname);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	animSpeed = strtol(animdefsToken,&endPos,10);
	if (endPos == animdefsToken  
		|| *endPos != '\0'  
#ifndef AVOID_ERRNO
		|| errno == ERANGE  
#endif
		|| animSpeed < 0)  
	{
		I_Error("Error parsing ANIMDEFS lump: Expected a positive integer for \"%s\"'s animation speed, got \"%s\"", animdefs[i].startname, animdefsToken);
	}
	animdefs[i].speed = animSpeed;
	Z_Free(animdefsToken);
}





#if 0
                                   
   
                                               
                                                             
                                                 
                                                       
                                          
    
static inline side_t *getSide(INT32 currentSector, INT32 line, INT32 side)
{
	return &sides[(sectors[currentSector].lines[line])->sidenum[side]];
}

                                     
   
                                               
                                                             
                                                 
                                                                 
                                        
    
static inline sector_t *getSector(INT32 currentSector, INT32 line, INT32 side)
{
	return sides[(sectors[currentSector].lines[line])->sidenum[side]].sector;
}

                                                  
                                                                               
                                   
   
                             
                                               
                                                      
                                         
    
static inline boolean twoSided(INT32 sector, INT32 line)
{
	return (sectors[sector].lines[line])->sidenum[1] != NO_SIDEDEF;
}
#endif

                                 
   
                                             
                                              
                                                                               
                         
                                    
                             
    
static sector_t *getNextSector(line_t *line, sector_t *sec)
{
	if (line->frontsector == sec)
	{
		if (line->backsector != sec)
			return line->backsector;
		else
			return NULL;
	}
	return line->frontsector;
}

                                           
   
                                  
                                                      
                                                             
                                      
    
fixed_t P_FindLowestFloorSurrounding(sector_t *sec)
{
	size_t i;
	line_t *check;
	sector_t *other;
	fixed_t floorh;

	floorh = sec->floorheight;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check,sec);

		if (!other)
			continue;

		if (other->floorheight < floorh)
			floorh = other->floorheight;
	}
	return floorh;
}

                                            
   
                                  
                                                       
                                                             
                                       
    
fixed_t P_FindHighestFloorSurrounding(sector_t *sec)
{
	size_t i;
	line_t *check;
	sector_t *other;
	fixed_t floorh = -500*FRACUNIT;
	INT32 foundsector = 0;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->floorheight > floorh || !foundsector)
			floorh = other->floorheight;

		if (!foundsector)
			foundsector = 1;
	}
	return floorh;
}

                                                 
   
                                            
                                            
                                                                             
                                     
                                                             
                                
                        
    
fixed_t P_FindNextHighestFloor(sector_t *sec, fixed_t currentheight)
{
	sector_t *other;
	size_t i;
	fixed_t height;

	for (i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i],sec);
		if (other && other->floorheight > currentheight)
		{
			height = other->floorheight;
			while (++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i], sec);
				if (other &&
					other->floorheight < height &&
					other->floorheight > currentheight)
					height = other->floorheight;
			}
			return height;
		}
	}
	return currentheight;
}





                                                
   
                                            
                                            
                                                                            
                                    
                                                             
                               
                        
    
fixed_t P_FindNextLowestFloor(sector_t *sec, fixed_t currentheight)
{
	sector_t *other;
	size_t i;
	fixed_t height;

	for (i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i], sec);
		if (other && other->floorheight < currentheight)
		{
			height = other->floorheight;
			while (++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i], sec);
				if (other &&	other->floorheight > height
					&& other->floorheight < currentheight)
					height = other->floorheight;
			}
			return height;
		}
	}
	return currentheight;
}

#if 0
                                                  
   
                                            
                                            
                                                                              
                                    
                                                                 
                             
                        
    
static fixed_t P_FindNextLowestCeiling(sector_t *sec, fixed_t currentheight)
{
	sector_t *other;
	size_t i;
	fixed_t height;

	for (i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i],sec);
		if (other &&	other->ceilingheight < currentheight)
		{
			height = other->ceilingheight;
			while (++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i],sec);
				if (other &&	other->ceilingheight > height
					&& other->ceilingheight < currentheight)
					height = other->ceilingheight;
			}
			return height;
		}
	}
	return currentheight;
}

                                                   
   
                                            
                                            
                                                                               
                                     
                                                                 
                              
                        
    
static fixed_t P_FindNextHighestCeiling(sector_t *sec, fixed_t currentheight)
{
	sector_t *other;
	size_t i;
	fixed_t height;

	for (i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i], sec);
		if (other && other->ceilingheight > currentheight)
		{
			height = other->ceilingheight;
			while (++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i],sec);
				if (other && other->ceilingheight < height
					&& other->ceilingheight > currentheight)
					height = other->ceilingheight;
			}
			return height;
		}
	}
	return currentheight;
}
#endif





                                             
   
                                  
                                                        
                                                                 
                                    
    
fixed_t P_FindLowestCeilingSurrounding(sector_t *sec)
{
	size_t i;
	line_t *check;
	sector_t *other;
	fixed_t height = 32000*FRACUNIT;  
	INT32 foundsector = 0;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->ceilingheight < height || !foundsector)
			height = other->ceilingheight;

		if (!foundsector)
			foundsector = 1;
	}
	return height;
}

                                              
   
                                  
                                                         
                                                                 
                                     
    
fixed_t P_FindHighestCeilingSurrounding(sector_t *sec)
{
	size_t i;
	line_t *check;
	sector_t *other;
	fixed_t height = 0;
	INT32 foundsector = 0;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->ceilingheight > height || !foundsector)
			height = other->ceilingheight;

		if (!foundsector)
			foundsector = 1;
	}
	return height;
}

#if 0








static fixed_t P_FindShortestTextureAround(INT32 secnum)
{
	fixed_t minsize = 32000<<FRACBITS;
	side_t *side;
	size_t i;
	sector_t *sec= &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided(secnum, i))
		{
			side = getSide(secnum,i,0);
			if (side->bottomtexture > 0)
				if (textureheight[side->bottomtexture] < minsize)
					minsize = textureheight[side->bottomtexture];
			side = getSide(secnum,i,1);
			if (side->bottomtexture > 0)
				if (textureheight[side->bottomtexture] < minsize)
					minsize = textureheight[side->bottomtexture];
		}
	}
	return minsize;
}









static fixed_t P_FindShortestUpperAround(INT32 secnum)
{
	fixed_t minsize = 32000<<FRACBITS;
	side_t *side;
	size_t i;
	sector_t *sec = &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided(secnum, i))
		{
			side = getSide(secnum,i,0);
			if (side->toptexture > 0)
				if (textureheight[side->toptexture] < minsize)
					minsize = textureheight[side->toptexture];
			side = getSide(secnum,i,1);
			if (side->toptexture > 0)
				if (textureheight[side->toptexture] < minsize)
					minsize = textureheight[side->toptexture];
		}
	}
	return minsize;
}












static sector_t *P_FindModelFloorSector(fixed_t floordestheight, INT32 secnum)
{
	size_t i;
	sector_t *sec = &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided(secnum, i))
		{
			if (getSide(secnum,i,0)->sector-sectors == secnum)
				sec = getSector(secnum,i,1);
			else
				sec = getSector(secnum,i,0);

			if (sec->floorheight == floordestheight)
				return sec;
		}
	}
	return NULL;
}










static sector_t *P_FindModelCeilingSector(fixed_t ceildestheight, INT32 secnum)
{
	size_t i;
	sector_t *sec = &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided(secnum, i))
		{
			if (getSide(secnum, i, 0)->sector - sectors == secnum)
				sec = getSector(secnum, i, 1);
			else
				sec = getSector(secnum, i, 0);

			if (sec->ceilingheight == ceildestheight)
				return sec;
		}
	}
	return NULL;
}
#endif


static boolean PolyDoor(line_t *line)
{
	polydoordata_t pdd;

	pdd.polyObjNum = line->args[0];  

	switch(line->special)
	{
		case 480:  
			pdd.doorType = POLY_DOOR_SLIDE;
			pdd.speed    = line->args[1] << (FRACBITS - 3);
			pdd.angle    = R_PointToAngle2(line->v1->x, line->v1->y, line->v2->x, line->v2->y);  
			pdd.distance = line->args[2] << FRACBITS;
			pdd.delay    = line->args[3];  
			break;
		case 481:  
			pdd.doorType = POLY_DOOR_SWING;
			pdd.speed    = line->args[1];  
			pdd.distance = line->args[2];  
			pdd.delay    = line->args[3];  
			break;
		default:
			return 0;  
	}

	return EV_DoPolyDoor(&pdd);
}


static boolean PolyMove(line_t *line)
{
	polymovedata_t pmd;

	pmd.polyObjNum = line->args[0];
	pmd.speed      = line->args[1] << (FRACBITS - 3);
	pmd.angle      = R_PointToAngle2(line->v1->x, line->v1->y, line->v2->x, line->v2->y);
	pmd.distance   = line->args[2] << FRACBITS;

	pmd.overRide = !!line->args[3];  

	return EV_DoPolyObjMove(&pmd);
}

static void PolySetVisibilityTangibility(line_t *line)
{
	INT32 polyObjNum = line->args[0];
	polyobj_t* po;

	if (!(po = Polyobj_GetForNum(polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "PolySetVisibilityTangibility: bad polyobj %d\n", polyObjNum);
		return;
	}


	if (po->isBad)
		return;

	if (line->args[1] == TMPV_VISIBLE)
	{
		po->flags &= ~POF_NOSPECIALS;
		po->flags |= (po->spawnflags & POF_RENDERALL);
	}
	else if (line->args[1] == TMPV_INVISIBLE)
	{
		po->flags |= POF_NOSPECIALS;
		po->flags &= ~POF_RENDERALL;
	}

	if (line->args[2] == TMPT_TANGIBLE)
		po->flags |= POF_SOLID;
	else if (line->args[2] == TMPT_INTANGIBLE)
		po->flags &= ~POF_SOLID;
}


static void PolyTranslucency(line_t *line)
{
	INT32 polyObjNum = line->args[0];
	polyobj_t *po;

	if (!(po = Polyobj_GetForNum(polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "PolyTranslucency: bad polyobj %d\n", polyObjNum);
		return;
	}


	if (po->isBad)
		return;

	if (lines->args[2])  
		po->translucency += line->args[1];
	else
		po->translucency = line->args[1];

	po->translucency = max(min(po->translucency, NUMTRANSMAPS), 0);
}


static boolean PolyFade(line_t *line)
{
	INT32 polyObjNum = line->args[0];
	polyobj_t *po;
	polyfadedata_t pfd;

	if (!(po = Polyobj_GetForNum(polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "PolyFade: bad polyobj %d\n", polyObjNum);
		return 0;
	}


	if (po->isBad)
		return 0;


	if (!(line->args[3] & TMPF_OVERRIDE)
		&& po->thinker
		&& po->thinker->function.acp1 == (actionf_p1)T_PolyObjFade)
	{
		CONS_Debug(DBG_POLYOBJ, "Line type 492 Executor: Fade PolyObject thinker already exists\n");
		return 0;
	}

	pfd.polyObjNum = polyObjNum;

	if (line->args[3] & TMPF_RELATIVE)  
		pfd.destvalue = po->translucency + line->args[1];
	else
		pfd.destvalue = line->args[1];

	pfd.destvalue = max(min(pfd.destvalue, NUMTRANSMAPS), 0);


	if (po->translucency == pfd.destvalue)
		return 1;

	pfd.docollision = !(line->args[3] & TMPF_IGNORECOLLISION);  
	pfd.doghostfade = (line->args[3] & TMPF_GHOSTFADE);         
	pfd.ticbased = (line->args[3] & TMPF_TICBASED);             

	pfd.speed = line->args[2];

	return EV_DoPolyObjFade(&pfd);
}


static boolean PolyWaypoint(line_t *line)
{
	polywaypointdata_t pwd;

	pwd.polyObjNum     = line->args[0];
	pwd.speed          = line->args[1] << (FRACBITS - 3);
	pwd.sequence       = line->args[2];
	pwd.returnbehavior = line->args[3];
	pwd.flags          = line->args[4];

	return EV_DoPolyObjWaypoint(&pwd);
}


static boolean PolyRotate(line_t *line)
{
	polyrotdata_t prd;

	prd.polyObjNum = line->args[0];
	prd.speed      = line->args[1];  
	prd.distance   = abs(line->args[2]);  
	prd.direction  = (line->args[2] < 0) ? -1 : 1;
	prd.flags      = line->args[3];

	return EV_DoPolyObjRotate(&prd);
}


static boolean PolyFlag(line_t *line)
{
	polyflagdata_t pfd;

	pfd.polyObjNum = line->args[0];
	pfd.speed = line->args[1];
	pfd.angle = line->angle >> ANGLETOFINESHIFT;
	pfd.momx = line->args[2];

	return EV_DoPolyObjFlag(&pfd);
}


static boolean PolyDisplace(line_t *line)
{
	polydisplacedata_t pdd;
	fixed_t length = R_PointToDist2(line->v2->x, line->v2->y, line->v1->x, line->v1->y);
	fixed_t speed = line->args[1] << FRACBITS;

	pdd.polyObjNum = line->args[0];

	pdd.controlSector = line->frontsector;
	pdd.dx = FixedMul(FixedDiv(line->dx, length), speed) >> 8;
	pdd.dy = FixedMul(FixedDiv(line->dy, length), speed) >> 8;

	return EV_DoPolyObjDisplace(&pdd);
}



static boolean PolyRotDisplace(line_t *line)
{
	polyrotdisplacedata_t pdd;
	fixed_t anginter, distinter;

	pdd.polyObjNum = line->args[0];
	pdd.controlSector = line->frontsector;


	anginter	= line->args[2] << FRACBITS;
	distinter	= line->args[1] << FRACBITS;
	pdd.rotscale = FixedDiv(anginter, distinter);


	pdd.turnobjs = line->args[3];

	return EV_DoPolyObjRotDisplace(&pdd);
}





void P_RunNightserizeExecutors(mobj_t *actor)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special == 323)
			P_RunTriggerLinedef(&lines[i], actor, NULL);
	}
}




void P_RunDeNightserizeExecutors(mobj_t *actor)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special == 325)
			P_RunTriggerLinedef(&lines[i], actor, NULL);
	}
}




void P_RunNightsLapExecutors(mobj_t *actor)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special == 327)
			P_RunTriggerLinedef(&lines[i], actor, NULL);
	}
}




void P_RunNightsCapsuleTouchExecutors(mobj_t *actor, boolean entering, boolean enoughspheres)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special != 329)
			continue;

		if (!!(lines[i].args[7] & TMI_ENTER) != entering)
			continue;

		if (lines[i].args[6] == TMS_IFENOUGH && !enoughspheres)
			continue;

		if (lines[i].args[6] == TMS_IFNOTENOUGH && enoughspheres)
			continue;

		P_RunTriggerLinedef(&lines[i], actor, NULL);
	}
}

                                                
   
                                     
                                          
                                                                              
                                            
    
INT32 P_FindMinSurroundingLight(sector_t *sector, INT32 max)
{
	size_t i;
	INT32 min = max;
	line_t *line;
	sector_t *check;

	for (i = 0; i < sector->linecount; i++)
	{
		line = sector->lines[i];
		check = getNextSector(line,sector);

		if (!check)
			continue;

		if (check->lightlevel < min)
			min = check->lightlevel;
	}
	return min;
}

void T_ExecutorDelay(executor_t *e)
{
	if (--e->timer <= 0)
	{
		if (e->caller && P_MobjWasRemoved(e->caller))  
			P_SetTarget(&e->caller, NULL);  
		P_ProcessLineSpecial(e->line, e->caller, e->sector);
		P_SetTarget(&e->caller, NULL);  
		P_RemoveThinker(&e->thinker);
	}
}

static void P_AddExecutorDelay(line_t *line, mobj_t *mobj, sector_t *sector)
{
	executor_t *e;
	INT32 delay;

	if (udmf)
		delay = line->executordelay;
	else
	{
		if (!line->backsector)
			I_Error("P_AddExecutorDelay: Line has no backsector!\n");

		delay = (line->backsector->ceilingheight >> FRACBITS) + (line->backsector->floorheight >> FRACBITS);
	}

	e = Z_Calloc(sizeof (*e), PU_LEVSPEC, NULL);

	e->thinker.function.acp1 = (actionf_p1)T_ExecutorDelay;
	e->line = line;
	e->sector = sector;
	e->timer = delay;
	P_SetTarget(&e->caller, mobj);  
	P_AddThinker(THINK_MAIN, &e->thinker);
}

                                                                              
   
                                                                                     
                                                                  
                           
    
static boolean P_CheckNightsTriggerLine(line_t *triggerline, mobj_t *actor)
{
	INT16 specialtype = triggerline->special;
	size_t i;

	UINT8 inputmare = max(0, min(255, triggerline->args[1]));
	UINT8 inputlap = max(0, min(255, triggerline->args[2]));

	textmapcomparison_t marecomp = triggerline->args[3];
	textmapcomparison_t lapcomp = triggerline->args[4];
	textmapnightsplayer_t checkplayer = triggerline->args[5];

	boolean lapfrombonustime;

	boolean donomares = (specialtype == 323) && (triggerline->args[7] & TMN_LEVELCOMPLETION);  

	UINT8 currentmare = UINT8_MAX;
	UINT8 currentlap = UINT8_MAX;


	switch (specialtype)
	{
		case 323:
			lapfrombonustime = !!(triggerline->args[7] & TMN_BONUSLAPS);
			break;
		case 325:
			lapfrombonustime = !!(triggerline->args[7]);
			break;
		case 327:
			lapfrombonustime = !!(triggerline->args[6]);
			break;
		case 329:
			lapfrombonustime = !!(triggerline->args[7] & TMI_BONUSLAPS);
			break;
		default:
			lapfrombonustime = false;
			break;
	}


	if (specialtype == 323)
	{

		if (donomares && P_FindLowestMare() != UINT8_MAX)
			return false;


		if (!donomares && P_FindLowestMare() == UINT8_MAX)
			return false;


		if (triggerline->args[6] == TMN_FROMNONIGHTS)
		{
			if (!actor->player)
				return false;
			else if (actor->player->powers[pw_carry] == CR_NIGHTSMODE)
				return false;
		}

		else if (triggerline->args[6] == TMN_FROMNIGHTS)
		{
			if (!actor->player)
				return false;
			else if (actor->player->powers[pw_carry] != CR_NIGHTSMODE)
				return false;
		}
	}


	if (checkplayer != TMNP_TRIGGERER
		|| (specialtype == 325 && triggerline->args[6] != TMD_ALWAYS))
	{
		UINT8 playersarenights = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			UINT8 lap;
			if (!playeringame[i] || players[i].spectator)
				continue;


			if (specialtype == 325 && triggerline->args[6] == TMD_NOBODYNIGHTS
				&& players[i].powers[pw_carry] == CR_NIGHTSMODE)
				return false;


			if (specialtype == 325 && triggerline->args[6] == TMD_SOMEBODYNIGHTS
				&& players[i].powers[pw_carry] == CR_NIGHTSMODE)
				playersarenights++;

			lap = lapfrombonustime ? players[i].marebonuslap : players[i].marelap;


			if (checkplayer == TMNP_FASTEST)
			{
				if (players[i].mare > currentmare || currentmare == UINT8_MAX)
				{
					currentmare = players[i].mare;
					currentlap = UINT8_MAX;
				}
				if (players[i].mare == currentmare
					&& (lap > currentlap || currentlap == UINT8_MAX))
					currentlap = lap;
			}

			else if (checkplayer == TMNP_SLOWEST)
			{
				if (players[i].mare < currentmare || currentmare == UINT8_MAX)
				{
					currentmare = players[i].mare;
					currentlap = UINT8_MAX;
				}
				if (players[i].mare == currentmare
					&& (lap < currentlap || currentlap == UINT8_MAX))
					currentlap = lap;
			}
		}


		if (specialtype == 325 && triggerline->args[6] == TMD_SOMEBODYNIGHTS
			&& playersarenights < 1)
			return false;
	}

	else if (checkplayer == TMNP_TRIGGERER)
	{
		if (!actor->player)
			return false;
		currentmare = actor->player->mare;
		currentlap = lapfrombonustime ? actor->player->marebonuslap : actor->player->marelap;
	}

	if (lapfrombonustime && !currentlap)
		return false;  


	if (!donomares  
		&& ((marecomp == TMC_LTE && currentmare > inputmare)
		|| (marecomp == TMC_GTE && currentmare < inputmare)
		|| (marecomp == TMC_EQUAL && currentmare != inputmare)
		|| (lapcomp == TMC_LTE && currentlap > inputlap)
		|| (lapcomp == TMC_GTE && currentlap < inputlap)
		|| (lapcomp == TMC_EQUAL && currentlap != inputlap))
		)
		return false;

	return true;
}

static boolean P_CheckPlayerMareOld(line_t *triggerline)
{
	UINT8 mare;
	INT32 targetmare = P_AproxDistance(triggerline->dx, triggerline->dy) >> FRACBITS;

	if (!(maptol & TOL_NIGHTS))
		return false;

	mare = P_FindLowestMare();

	if (triggerline->flags & ML_NOCLIMB)
		return mare <= targetmare;

	if (triggerline->flags & ML_BLOCKMONSTERS)
		return mare >= targetmare;

	return mare == targetmare;
}

static boolean P_CheckPlayerMare(line_t *triggerline)
{
	UINT8 mare;
	INT32 targetmare = triggerline->args[1];

	if (!(maptol & TOL_NIGHTS))
		return false;

	mare = P_FindLowestMare();

	switch (triggerline->args[2])
	{
		case TMC_EQUAL:
		default:
			return mare == targetmare;
		case TMC_GTE:
			return mare >= targetmare;
		case TMC_LTE:
			return mare <= targetmare;
	}
}

static boolean P_CheckPlayerRings(line_t *triggerline, mobj_t *actor)
{
	INT32 rings = 0;
	INT32 targetrings = triggerline->args[1];
	size_t i;


	if (triggerline->args[3])
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if (!players[i].mo || ((maptol & TOL_NIGHTS) ? players[i].spheres : players[i].rings) <= 0)
				continue;

			rings += (maptol & TOL_NIGHTS) ? players[i].spheres : players[i].rings;
		}
	}
	else
	{
		if (!(actor && actor->player))
			return false;  

		rings = (maptol & TOL_NIGHTS) ? actor->player->spheres : actor->player->rings;
	}

	switch (triggerline->args[2])
	{
		case TMC_EQUAL:
		default:
			return rings == targetrings;
		case TMC_GTE:
			return rings >= targetrings;
		case TMC_LTE:
			return rings <= targetrings;
	}
}

static boolean P_CheckPushables(line_t *triggerline, sector_t *caller)
{
	msecnode_t *node;
	mobj_t *mo;
	INT32 numpushables = 0;
	INT32 targetpushables = triggerline->args[1];

	if (!caller)
		return false;  


	for (node = caller->touching_thinglist; node; node = node->m_thinglist_next)
	{
		mo = node->m_thing;
		if ((mo->flags & MF_PUSHABLE) || ((mo->info->flags & MF_PUSHABLE) && mo->fuse))
			numpushables++;
	}

	switch (triggerline->args[2])
	{
		case TMC_EQUAL:
		default:
			return numpushables == targetpushables;
		case TMC_GTE:
			return numpushables >= targetpushables;
		case TMC_LTE:
			return numpushables <= targetpushables;
	}
}

static boolean P_CheckEmeralds(INT32 checktype, UINT16 target)
{
	switch (checktype)
	{
		case TMF_HASALL:
		default:
			return (emeralds & target) == target;
		case TMF_HASANY:
			return !!(emeralds & target);
		case TMF_HASEXACTLY:
			return emeralds == target;
		case TMF_DOESNTHAVEALL:
			return (emeralds & target) != target;
		case TMF_DOESNTHAVEANY:
			return !(emeralds & target);
	}
}

static void P_ActivateLinedefExecutor(line_t *line, mobj_t *actor, sector_t *caller)
{
	if (line->special < 400 || line->special >= 500)
		return;

	if (line->executordelay)
		P_AddExecutorDelay(line, actor, caller);
	else
		P_ProcessLineSpecial(line, actor, caller);
}

static boolean P_ActivateLinedefExecutorsInSector(line_t *triggerline, mobj_t *actor, sector_t *caller)
{
	sector_t *ctlsector = triggerline->frontsector;
	size_t sectori = (size_t)(ctlsector - sectors);
	size_t linecnt = ctlsector->linecount;
	size_t i;

	if (!udmf && triggerline->flags & ML_WRAPMIDTEX)  
	{
		for (i = 0; i < linecnt; i++)
			P_ActivateLinedefExecutor(ctlsector->lines[i], actor, caller);
	}
	else  
	{
		boolean backwards = false;
		size_t j, masterlineindex = (size_t)-1;

		for (i = 0; i < linecnt; i++)
			if (ctlsector->lines[i] == triggerline)
			{
				masterlineindex = i;
				break;
			}

#ifdef PARANOIA
		if (masterlineindex == (size_t)-1)
		{
			const size_t li = (size_t)(ctlsector->lines[i] - lines);
			I_Error("Line %s isn't linked into its front sector", sizeu1(li));
		}
#endif


		for (;;)
		{
			if (backwards)  
			{
				for (j = 0; j < linecnt; j++)
				{
					if (i == j)
						continue;
					if (ctlsector->lines[i]->v1 == ctlsector->lines[j]->v2)
					{
						i = j;
						break;
					}
					if (ctlsector->lines[i]->v1 == ctlsector->lines[j]->v1)
					{
						i = j;
						backwards = false;
						break;
					}
				}
				if (j == linecnt)
				{
					const size_t vertexei = (size_t)(ctlsector->lines[i]->v1 - vertexes);
					CONS_Debug(DBG_GAMELOGIC, "Warning: Sector %s is not closed at vertex %s (%d, %d)\n",
						sizeu1(sectori), sizeu2(vertexei), ctlsector->lines[i]->v1->x, ctlsector->lines[i]->v1->y);
					return false;  
				}
			}
			else  
			{
				for (j = 0; j < linecnt; j++)
				{
					if (i == j)
						continue;
					if (ctlsector->lines[i]->v2 == ctlsector->lines[j]->v1)
					{
						i = j;
						break;
					}
					if (ctlsector->lines[i]->v2 == ctlsector->lines[j]->v2)
					{
						i = j;
						backwards = true;
						break;
					}
				}
				if (j == linecnt)
				{
					const size_t vertexei = (size_t)(ctlsector->lines[i]->v1 - vertexes);
					CONS_Debug(DBG_GAMELOGIC, "Warning: Sector %s is not closed at vertex %s (%d, %d)\n",
						sizeu1(sectori), sizeu2(vertexei), ctlsector->lines[i]->v2->x, ctlsector->lines[i]->v2->y);
					return false;  
				}
			}

			if (i == masterlineindex)
				break;

			P_ActivateLinedefExecutor(ctlsector->lines[i], actor, caller);
		}
	}

	return true;
}

                                                                    
                                                                                                    
   
                                                                                     
                                                                  
                                                                      
                                              
    
void P_RunTriggerLinedef(line_t *triggerline, mobj_t *actor, sector_t *caller)
{
	INT16 specialtype = triggerline->special;





	if (caller && !udmf)
	{
		if (caller->triggerer == TO_PLAYEREMERALDS)
		{
			if (!(ALL7EMERALDS(emeralds)))
				return;
		}
		else if (caller->triggerer == TO_PLAYERNIGHTS)
		{
			if (!P_CheckPlayerMareOld(triggerline))
				return;
		}
	}

	switch (specialtype)
	{
		case 303:
			if (!P_CheckPlayerRings(triggerline, actor))
				return;
			break;
		case 305:
			if (!(actor && actor->player && actor->player->charability == triggerline->args[1]))
				return;
			break;
		case 309:

			if (!(actor && actor->player))
				return;
			if (actor->player->ctfteam != ((triggerline->args[1] == TMT_RED) ? 1 : 2))
				return;
			break;
		case 314:
			if (!P_CheckPushables(triggerline, caller))
				return;
			break;
		case 317:
			{  
				INT32 trigid = triggerline->args[1];

				if (trigid < 0 || trigid > 31)  
				{
					CONS_Debug(DBG_GAMELOGIC, "Unlockable trigger (sidedef %u): bad trigger ID %d\n", triggerline->sidenum[0], trigid);
					return;
				}
				else if (!(unlocktriggers & (1 << trigid)))
					return;
			}
			break;
		case 319:
			{  
				INT32 unlockid = triggerline->args[1];

				if (unlockid <= 0 || unlockid > MAXUNLOCKABLES)  
				{
					CONS_Debug(DBG_GAMELOGIC, "Unlockable check (sidedef %u): bad unlockable ID %d\n", triggerline->sidenum[0], unlockid);
					return;
				}
				else if (!(serverGamedata->unlocked[unlockid-1]))
					return;
			}
			break;
		case 321:

			if (triggerline->callcount > 0)
			{
				if (--triggerline->callcount > 0)
					return;
			}
			break;
		case 323:  
		case 325:  
		case 327:  
		case 329:  
			if (!P_CheckNightsTriggerLine(triggerline, actor))
				return;
			break;
		case 331:
			if (!(actor && actor->player))
				return;
			if (!triggerline->stringargs[0])
				return;
			if (!(stricmp(triggerline->stringargs[0], skins[actor->player->skin]->name) == 0) ^ !!(triggerline->args[1]))
				return;
			break;
		case 334:  
			{
				INT32 triggercolor = triggerline->stringargs[0] ? get_number(triggerline->stringargs[0]) : SKINCOLOR_NONE;
				UINT16 color = (actor->player ? actor->player->powers[pw_dye] : actor->color);

				if (!!(triggerline->args[1]) ^ (triggercolor != color))
					return;
			}
			break;
		case 337:  
			if (!P_CheckEmeralds(triggerline->args[2], (UINT16)triggerline->args[1]))
				return;
			break;
		case 340:  
			if (!P_CheckPlayerMare(triggerline))
				return;
			break;
		case 343:  
			if (triggerline->args[1] == TMG_TEMPREVERSE && (!(actor->flags2 & MF2_OBJECTFLIP) != !(actor->player->powers[pw_gravityboots])))
				return;
			if ((triggerline->args[1] == TMG_NORMAL) != !(actor->eflags & MFE_VERTICALFLIP))
				return;
			break;
		default:
			break;
	}





	if (!P_ActivateLinedefExecutorsInSector(triggerline, actor, caller))
		return;


	if (specialtype == 321 && triggerline->args[2])
		triggerline->callcount = triggerline->args[1];
	else
	{

		if (specialtype == 313   
			|| specialtype == 321  
			|| specialtype == 399)  
			triggerline->special = 0;
		else if ((specialtype == 323  
			|| specialtype == 325  
			|| specialtype == 327  
			|| specialtype == 329)  
			&& triggerline->args[0])
			triggerline->special = 0;
		else if ((specialtype == 300  
			|| specialtype == 303  
			|| specialtype == 305  
			|| specialtype == 308  
			|| specialtype == 309  
			|| specialtype == 314  
			|| specialtype == 317  
			|| specialtype == 319  
			|| specialtype == 331  
			|| specialtype == 334  
			|| specialtype == 337  
			|| specialtype == 343)  
			&& triggerline->args[0] == TMT_ONCE)
			triggerline->special = 0;
	}
}

                            
                     
                                                     
                                                              
                                                                               
                                                                 
   
                                                  
                                                                  
                                                                      
                                                 
                                       
    
void P_LinedefExecute(INT16 tag, mobj_t *actor, sector_t *caller)
{
	INT32 masterline;

	CONS_Debug(DBG_GAMELOGIC, "P_LinedefExecute: Executing trigger linedefs of tag %d\n", tag);

	I_Assert(!actor || !P_MobjWasRemoved(actor));  

	TAG_ITER_LINES(tag, masterline)
	{
		if (lines[masterline].special < 300
			|| lines[masterline].special > 399)
			continue;


		if (lines[masterline].special == 313  || lines[masterline].special == 399)
			continue;


		if ((lines[masterline].special == 300  
			|| lines[masterline].special == 303  
			|| lines[masterline].special == 305  
			|| lines[masterline].special == 308  
			|| lines[masterline].special == 309  
			|| lines[masterline].special == 314  
			|| lines[masterline].special == 317  
			|| lines[masterline].special == 319  
			|| lines[masterline].special == 331  
			|| lines[masterline].special == 334  
			|| lines[masterline].special == 337  
			|| lines[masterline].special == 343)  
			&& lines[masterline].args[0] > TMT_EACHTIMEMASK)
			continue;

		if (lines[masterline].special == 321 && lines[masterline].args[0] > TMXT_EACHTIMEMASK)  
			continue;

		P_RunTriggerLinedef(&lines[masterline], actor, caller);  
	}
}

static void P_PlaySFX(INT32 sfxnum, mobj_t *mo, sector_t *callsec, INT16 tag, textmapsoundsource_t source, textmapsoundlistener_t listener)
{
	if (sfxnum == sfx_None)
		return;  

	if (sfxnum < sfx_None || sfxnum >= NUMSFX)
	{
		CONS_Debug(DBG_GAMELOGIC, "Line type 414 Executor: sfx number %d is invalid!\n", sfxnum);
		return;
	}


	switch (listener)
	{
		case TMSL_TRIGGERER:  
			if (!mo)
				return;

			if (!mo->player)
				return;

			if (mo->player != &players[displayplayer] && mo->player != &players[secondarydisplayplayer])
				return;

			break;
		case TMSL_TAGGEDSECTOR:  
		{
			UINT8 i = 0;
			mobj_t *camobj = players[displayplayer].mo;
			ffloor_t *rover;
			boolean foundit = false;

			for (i = 0; i < 2; camobj = players[secondarydisplayplayer].mo, i++)
			{
				if (!camobj)
					continue;

				if (foundit || Tag_Find(&camobj->subsector->sector->tags, tag))
				{
					foundit = true;
					break;
				}


				for (rover = camobj->subsector->sector->ffloors; rover; rover = rover->next)
				{
					if (!Tag_Find(&rover->master->frontsector->tags, tag))
						continue;

					if (camobj->z > P_GetSpecialTopZ(camobj, sectors + rover->secnum, camobj->subsector->sector))
						continue;

					if (camobj->z + camobj->height < P_GetSpecialBottomZ(camobj, sectors + rover->secnum, camobj->subsector->sector))
						continue;

					foundit = true;
					break;
				}
			}

			if (!foundit)
				return;

			break;
		}
		case TMSL_EVERYONE:  
		default:
			break;
	}


	switch (source)
	{
		case TMSS_TRIGGERMOBJ:  
			if (mo)
				S_StartSound(mo, sfxnum);
			break;
		case TMSS_TRIGGERSECTOR:  
			if (callsec)
				S_StartSound(&callsec->soundorg, sfxnum);
			else if (mo)
				S_StartSound(&mo->subsector->sector->soundorg, sfxnum);
			break;
		case TMSS_NOWHERE:  
			S_StartSound(NULL, sfxnum);
			break;
		case TMSS_TAGGEDSECTOR:  
		{
			INT32 secnum;

			TAG_ITER_SECTORS(tag, secnum)
				S_StartSound(&sectors[secnum].soundorg, sfxnum);
			break;
		}
		default:
			break;
	}
}

static boolean is_rain_type (INT32 weathernum)
{
	switch (weathernum)
	{
		case PRECIP_SNOW:
		case PRECIP_RAIN:
		case PRECIP_STORM:
		case PRECIP_STORM_NOSTRIKES:
		case PRECIP_BLANK:
			return true;

		default:
			return false;
	}
}






void P_SwitchWeather(INT32 weathernum)
{
	boolean purge = true;

	if (weathernum == curWeather)
		return;

	if (is_rain_type(weathernum) &&
			is_rain_type(curWeather))
		purge = false;

	if (purge)
	{
		thinker_t *think;
		precipmobj_t *precipmobj;

		for (think = thlist[THINK_PRECIP].next; think != &thlist[THINK_PRECIP]; think = think->next)
		{
			if (think->function.acp1 != (actionf_p1)P_NullPrecipThinker)
				continue;  

			precipmobj = (precipmobj_t *)think;

			P_RemovePrecipMobj(precipmobj);
		}
	}
	else  
	{
		thinker_t *think;
		precipmobj_t *precipmobj;
		state_t *st;

		for (think = thlist[THINK_PRECIP].next; think != &thlist[THINK_PRECIP]; think = think->next)
		{
			if (think->function.acp1 != (actionf_p1)P_NullPrecipThinker)
				continue;  
			precipmobj = (precipmobj_t *)think;

			if (weathernum == PRECIP_RAIN || weathernum == PRECIP_STORM || weathernum == PRECIP_STORM_NOSTRIKES)  
			{
				precipmobj->flags = mobjinfo[MT_RAIN].flags;
				st = &states[mobjinfo[MT_RAIN].spawnstate];
				precipmobj->state = st;
				precipmobj->tics = st->tics;
				precipmobj->sprite = st->sprite;
				precipmobj->frame = st->frame;
				precipmobj->momz = mobjinfo[MT_RAIN].speed;

				precipmobj->precipflags &= ~PCF_INVISIBLE;

				precipmobj->precipflags |= PCF_RAIN;

			}
			else if (weathernum == PRECIP_SNOW)  
			{
				INT32 z;

				precipmobj->flags = mobjinfo[MT_SNOWFLAKE].flags;
				z = M_RandomByte();

				if (z < 64)
					z = 2;
				else if (z < 144)
					z = 1;
				else
					z = 0;

				st = &states[mobjinfo[MT_SNOWFLAKE].spawnstate+z];
				precipmobj->state = st;
				precipmobj->tics = st->tics;
				precipmobj->sprite = st->sprite;
				precipmobj->frame = st->frame;
				precipmobj->momz = mobjinfo[MT_SNOWFLAKE].speed;

				precipmobj->precipflags &= ~(PCF_INVISIBLE|PCF_RAIN);


			}
			else  
			{


				precipmobj->precipflags |= PCF_INVISIBLE;
			}
		}
	}

	switch (weathernum)
	{
		case PRECIP_SNOW:  
			curWeather = PRECIP_SNOW;

			if (purge)
				P_SpawnPrecipitation();

			break;
		case PRECIP_RAIN:  
		{
			curWeather = PRECIP_RAIN;

			if (purge)
				P_SpawnPrecipitation();

			break;
		}
		case PRECIP_STORM:  
		{
			curWeather = PRECIP_STORM;

			if (purge)
				P_SpawnPrecipitation();

			break;
		}
		case PRECIP_STORM_NOSTRIKES:  
		{
			curWeather = PRECIP_STORM_NOSTRIKES;

			if (purge)
				P_SpawnPrecipitation();

			break;
		}
		case PRECIP_STORM_NORAIN:  
			curWeather = PRECIP_STORM_NORAIN;

			break;
		case PRECIP_BLANK:  
			curWeather = PRECIP_BLANK;

			if (purge)
				P_SpawnPrecipitation();

			break;
		default:
			curWeather = PRECIP_NONE;
			break;
	}
}

                   
   
                                        
                                      
                                                            
                      
    
static mobj_t *P_GetObjectTypeInSectorNum(mobjtype_t type, size_t s)
{
	sector_t *sec = sectors + s;
	mobj_t *thing = sec->thinglist;

	while (thing)
	{
		if (thing->type == type)
			return thing;
		thing = thing->snext;
	}
	return NULL;
}

static mobj_t* P_FindObjectTypeFromTag(mobjtype_t type, mtag_t tag)
{
	if (udmf)
	{
		INT32 mtnum;
		mobj_t *mo;

		TAG_ITER_THINGS(tag, mtnum)
		{
			mo = mapthings[mtnum].mobj;

			if (!mo)
				continue;

			if (mo->type != type)
				continue;

			return mo;
		}

		return NULL;
	}
	else
	{
		INT32 secnum;

		if ((secnum = Tag_Iterate_Sectors(tag, 0)) < 0)
			return NULL;

		return P_GetObjectTypeInSectorNum(type, secnum);
	}
}

                                                      
   
                                                    
                                                                         
                                                                          
                                                                      
                                                                           
                                                                  
                                                                            
                                       
                                           
                        
                                       
    
static void P_ProcessLineSpecial(line_t *line, mobj_t *mo, sector_t *callsec)
{
	INT32 secnum = -1;
	mobj_t *bot = NULL;

	I_Assert(!mo || !P_MobjWasRemoved(mo));  

	if (mo && mo->player && botingame)
		bot = players[secondarydisplayplayer].mo;


	switch (line->special)
	{
		case 400:  
			if (line->args[1] != TMP_CEILING)
				EV_DoFloor(line->args[0], line, instantMoveFloorByFrontSector);
			if (line->args[1] != TMP_FLOOR)
				EV_DoCeiling(line->args[0], line, instantMoveCeilingByFrontSector);
			break;

		case 402:  
			{
				INT16 newlightlevel;
				INT16 newfloorlightlevel, newceilinglightlevel;
				boolean newfloorlightabsolute, newceilinglightabsolute;
				INT32 newfloorlightsec, newceilinglightsec;

				newlightlevel = line->frontsector->lightlevel;
				newfloorlightlevel = line->frontsector->floorlightlevel;
				newfloorlightabsolute = line->frontsector->floorlightabsolute;
				newceilinglightlevel = line->frontsector->ceilinglightlevel;
				newceilinglightabsolute = line->frontsector->ceilinglightabsolute;
				newfloorlightsec = line->frontsector->floorlightsec;
				newceilinglightsec = line->frontsector->ceilinglightsec;

				TAG_ITER_SECTORS(line->args[0], secnum)
				{
					if (sectors[secnum].lightingdata)
					{

						P_RemoveThinker(&((thinkerdata_t *)sectors[secnum].lightingdata)->thinker);
						sectors[secnum].lightingdata = NULL;
					}

					if (!(line->args[1] & TMLC_NOSECTOR))
						sectors[secnum].lightlevel = newlightlevel;
					if (!(line->args[1] & TMLC_NOFLOOR))
					{
						sectors[secnum].floorlightlevel = newfloorlightlevel;
						sectors[secnum].floorlightabsolute = newfloorlightabsolute;
						sectors[secnum].floorlightsec = newfloorlightsec;
					}
					if (!(line->args[1] & TMLC_NOCEILING))
					{
						sectors[secnum].ceilinglightlevel = newceilinglightlevel;
						sectors[secnum].ceilinglightabsolute = newceilinglightabsolute;
						sectors[secnum].ceilinglightsec = newceilinglightsec;
					}
				}
			}
			break;

		case 403:  
			if (line->args[1] != TMP_CEILING)
				EV_DoFloor(line->args[0], line, moveFloorByFrontSector);
			if (line->args[1] != TMP_FLOOR)
				EV_DoCeiling(line->args[0], line, moveCeilingByFrontSector);
			break;

		case 405:  
			if (line->args[1] != TMP_CEILING)
				EV_DoFloor(line->args[0], line, moveFloorByDistance);
			if (line->args[1] != TMP_FLOOR)
				EV_DoCeiling(line->args[0], line, moveCeilingByDistance);
			break;

		case 408:  
		{
			TAG_ITER_SECTORS(line->args[0], secnum)
			{
				if (line->args[1] != TMP_CEILING)
					sectors[secnum].floorpic = line->frontsector->floorpic;
				if (line->args[1] != TMP_FLOOR)
					sectors[secnum].ceilingpic = line->frontsector->ceilingpic;
			}
			break;
		}

		case 409:  

		{
			mtag_t newtag = line->args[1];

			TAG_ITER_SECTORS(line->args[0], secnum)
			{
				switch (line->args[2])
				{
					case TMT_ADD:
						Tag_SectorAdd(secnum, newtag);
						break;
					case TMT_REMOVE:
						Tag_SectorRemove(secnum, newtag);
						break;
					case TMT_REPLACEFIRST:
					default:
						Tag_SectorFSet(secnum, newtag);
						break;
					case TMT_TRIGGERTAG:
						sectors[secnum].triggertag = newtag;
						break;
				}
			}
			break;
		}

		case 410:  
		{
			mtag_t newtag = line->args[1];
			secnum = (UINT32)(line->frontsector - sectors);

			switch (line->args[2])
			{
				case TMT_ADD:
					Tag_SectorAdd(secnum, newtag);
					break;
				case TMT_REMOVE:
					Tag_SectorRemove(secnum, newtag);
					break;
				case TMT_REPLACEFIRST:
				default:
					Tag_SectorFSet(secnum, newtag);
					break;
				case TMT_TRIGGERTAG:
					sectors[secnum].triggertag = newtag;
					break;
			}
			break;
		}

		case 411:  
			TAG_ITER_SECTORS(line->args[0], secnum)
			{
				if (sectors[secnum].floordata)
				{
					if (sectors[secnum].floordata == sectors[secnum].ceilingdata)  
					{
						P_RemoveThinker(&((elevator_t *)sectors[secnum].floordata)->thinker);
						sectors[secnum].floordata = sectors[secnum].ceilingdata = NULL;
						sectors[secnum].floorspeed = sectors[secnum].ceilspeed = 0;
					}
					else  
					{
						P_RemoveThinker(&((floormove_t *)sectors[secnum].floordata)->thinker);
						sectors[secnum].floordata = NULL;
						sectors[secnum].floorspeed = 0;
					}
				}

				if (sectors[secnum].ceilingdata)  
				{
					P_RemoveThinker(&((ceiling_t *)sectors[secnum].ceilingdata)->thinker);
					sectors[secnum].ceilingdata = NULL;
					sectors[secnum].ceilspeed = 0;
				}
			}
			break;

		case 412:  
			{
				mobj_t *dest;

				if (!mo)  
					return;

				if (line->args[1] & TMT_RELATIVE)  
				{
					fixed_t x, y, z;

					x = line->args[2] << FRACBITS;
					y = line->args[3] << FRACBITS;
					z = line->args[4] << FRACBITS;

					P_SetOrigin(mo, mo->x + x, mo->y + y, mo->z + z);

					if (mo->player)
					{
						if (bot)  
							P_SetOrigin(bot, bot->x + x, bot->y + y, bot->z + z);
						if (splitscreen && mo->player == &players[secondarydisplayplayer] && camera2.chase)
						{
							camera2.reset = true;
							camera2.x += x;
							camera2.y += y;
							camera2.z += z;
							camera2.subsector = R_PointInSubsector(camera2.x, camera2.y);
						}
						else if (camera.chase && mo->player == &players[displayplayer])
						{
							camera.reset = true;
							camera.x += x;
							camera.y += y;
							camera.z += z;
							camera.subsector = R_PointInSubsector(camera.x, camera.y);
						}
					}
				}
				else
				{
					angle_t angle;
					boolean silent, keepmomentum;

					dest = P_FindObjectTypeFromTag(MT_TELEPORTMAN, line->args[0]);
					if (!dest)
						return;

					angle = (line->args[1] & TMT_KEEPANGLE) ? mo->angle : dest->angle;
					silent = !!(line->args[1] & TMT_SILENT);
					keepmomentum = !!(line->args[1] & TMT_KEEPMOMENTUM);

					if (bot)
						P_Teleport(bot, dest->x, dest->y, dest->z, angle, !silent, keepmomentum);
					P_Teleport(mo, dest->x, dest->y, dest->z, angle, !silent, keepmomentum);
					if (!silent)
						S_StartSound(dest, sfx_mixup);  
				}
			}
			break;

		case 413:  

			if ((line->args[0] & TMM_ALLPLAYERS) || (mo && mo->player && P_IsLocalPlayer(mo->player)) || titlemapinaction)
			{
				boolean musicsame = (!line->stringargs[0] || !line->stringargs[0][0] || !strnicmp(line->stringargs[0], S_MusicName(), 7));
				UINT16 tracknum = (UINT16)max(line->args[6], 0);
				INT32 position = (INT32)max(line->args[1], 0);
				UINT32 prefadems = (UINT32)max(line->args[2], 0);
				UINT32 postfadems = (UINT32)max(line->args[3], 0);
				UINT8 fadetarget = (UINT8)max(line->args[4], 0);
				INT16 fadesource = (INT16)max(line->args[5], -1);


				if (line->args[0] & TMM_OFFSET)
				{

					if (position < 0 && S_GetMusicLength() &&
						S_GetMusicPosition() > S_GetMusicLoopPoint() &&
						S_GetMusicPosition() + position < S_GetMusicLoopPoint())
						position = max(S_GetMusicLength() - (S_GetMusicLoopPoint() - (S_GetMusicPosition() + position)), 0);
					else
						position = max(S_GetMusicPosition() + position, 0);
				}


				if ((line->args[0] & TMM_FADE) && fadetarget && musicsame)
				{


					if (!fadesource)
						fadesource = -1;

					if (!postfadems)
						S_SetInternalMusicVolume(fadetarget);
					else
						S_FadeMusicFromVolume(fadetarget, fadesource, postfadems);

					if (position)
						S_SetMusicPosition(position);
				}

				else
				{
					if (!line->stringargs[0] || !strcmp(line->stringargs[0], "-"))
						strcpy(mapmusname, "");
					else
					{
						strncpy(mapmusname, line->stringargs[0], 7);
						mapmusname[6] = 0;
					}

					mapmusflags = tracknum & MUSIC_TRACKMASK;
					if (!(line->args[0] & TMM_NORELOAD))
						mapmusflags |= MUSIC_RELOADRESET;
					if (line->args[0] & TMM_FORCERESET)
						mapmusflags |= MUSIC_FORCERESET;

					mapmusposition = position;

					S_ChangeMusicEx(mapmusname, mapmusflags, !(line->args[0] & TMM_NOLOOP), position,
						!(line->args[0] & TMM_FADE) ? prefadems : 0,
						!(line->args[0] & TMM_FADE) ? postfadems : 0);

					if ((line->args[0] & TMM_FADE) && fadetarget)
					{
						if (!postfadems)
							S_SetInternalMusicVolume(fadetarget);
						else
							S_FadeMusicFromVolume(fadetarget, fadesource, postfadems);
					}
				}



			}
			break;

		case 414:  
			P_PlaySFX(line->stringargs[0] ? get_number(line->stringargs[0]) : sfx_None, mo, callsec, line->args[2], line->args[0], line->args[1]);
			break;

		case 415:  
			if (cv_runscripts.value)
			{
				lumpnum_t lumpnum = W_CheckNumForName(line->stringargs[0]);

				if (lumpnum == LUMPERROR || W_LumpLength(lumpnum) == 0)
					CONS_Debug(DBG_SETUP, "Line type 415 Executor: script lump %s not found/not valid.\n", line->stringargs[0]);
				else
				{
					void *lump = W_CacheLumpNum(lumpnum, PU_CACHE);
					size_t len = W_LumpLength(lumpnum);
					char *text = Z_Malloc(len + 1, PU_CACHE, NULL);
					memcpy(text, lump, len);
					text[len] = '\0';
					COM_BufInsertTextEx(text, COM_LUA);
					Z_Free(text);
				}
			}
			break;

		case 416:  
			TAG_ITER_SECTORS(line->args[0], secnum)
				P_SpawnAdjustableFireFlicker(&sectors[secnum], line->args[2],
					line->args[3] ? sectors[secnum].lightlevel : line->args[4], line->args[1]);
			break;

		case 417:  
			TAG_ITER_SECTORS(line->args[0], secnum)
				P_SpawnAdjustableGlowingLight(&sectors[secnum], line->args[2],
					line->args[3] ? sectors[secnum].lightlevel : line->args[4], line->args[1]);
			break;

		case 418:  
			TAG_ITER_SECTORS(line->args[0], secnum)
				P_SpawnAdjustableStrobeFlash(&sectors[secnum], line->args[3],
					(line->args[4] & TMB_USETARGET) ? sectors[secnum].lightlevel : line->args[5],
					line->args[1], line->args[2], line->args[4] & TMB_SYNC);
			break;

		case 420:  
			P_FadeLight(line->args[0], line->args[1], line->args[2], line->args[3] & TMF_TICBASED, line->args[3] & TMF_OVERRIDE, line->args[3] & TMF_RELATIVE);
			break;

		case 421:  
			TAG_ITER_SECTORS(line->args[0], secnum)
				if (sectors[secnum].lightingdata)
				{
					P_RemoveThinker(&((thinkerdata_t *)sectors[secnum].lightingdata)->thinker);
					sectors[secnum].lightingdata = NULL;
				}
			break;

		case 422:  
			{
				mobj_t *altview;
				INT32 aim;

				if ((!mo || !mo->player) && !titlemapinaction)  
					return;

				altview = P_FindObjectTypeFromTag(MT_ALTVIEWMAN, line->args[0]);
				if (!altview || !altview->spawnpoint)
					return;



				if (titlemapinaction)
					titlemapcameraref = altview;
				else if (!mo->player->awayviewtics || mo->player->awayviewmobj != altview) {
					P_SetTarget(&mo->player->awayviewmobj, altview);

					if (mo->player == &players[displayplayer])
						P_ResetCamera(mo->player, &camera);  
					else if (splitscreen && mo->player == &players[secondarydisplayplayer])
						P_ResetCamera(mo->player, &camera2);   
				}

				aim = udmf ? altview->spawnpoint->pitch : line->args[2];
				aim = (aim + 360) % 360;
				aim *= (ANGLE_90>>8);
				aim /= 90;
				aim <<= 8;
				if (titlemapinaction)
					titlemapcameraref->cusval = (angle_t)aim;
				else {
					mo->player->awayviewaiming = (angle_t)aim;
					mo->player->awayviewtics = line->args[1];
				}
			}
			break;

		case 423:  
			if ((mo && mo->player && P_IsLocalPlayer(mo->player)) || line->args[1])
				P_SetupLevelSky(line->args[0], line->args[1]);
			break;

		case 424:  
			if (line->args[1])
			{
				globalweather = (UINT8)(line->args[0]);
				P_SwitchWeather(globalweather);
			}
			else if (mo && mo->player && P_IsLocalPlayer(mo->player))
				P_SwitchWeather(line->args[0]);
			break;

		case 425:  
			if (mo && !mo->player)
			{
				statenum_t state = line->stringargs[0] ? get_number(line->stringargs[0]) : S_NULL;
				if (state >= 0 && state < NUMSTATES)
					P_SetMobjState(mo, state);
			}
			break;

		case 426:  
			if (!mo)
				return;

			if (line->args[0])
			{
				P_UnsetThingPosition(mo);
				mo->x = mo->subsector->sector->soundorg.x;
				mo->y = mo->subsector->sector->soundorg.y;
				mo->z = mo->floorz;
				P_SetThingPosition(mo);
			}

			mo->momx = mo->momy = mo->momz = 1;
			mo->pmomz = 0;

			if (mo->player)
			{
				mo->player->rmomx = mo->player->rmomy = 1;
				mo->player->cmomx = mo->player->cmomy = 0;
				P_ResetPlayer(mo->player);
				P_SetMobjState(mo, S_PLAY_STND);


				if (bot) {
					if (line->args[0])
						P_SetOrigin(bot, mo->x, mo->y, mo->z);
					bot->momx = bot->momy = bot->momz = 1;
					bot->pmomz = 0;
					bot->player->rmomx = bot->player->rmomy = 1;
					bot->player->cmomx = bot->player->cmomy = 0;
					P_ResetPlayer(bot->player);
					P_SetMobjState(bot, S_PLAY_STND);
				}
			}
			break;

		case 427:  
			if (mo && mo->player)
				P_AddPlayerScore(mo->player, line->args[0]);
			break;

		case 428:  
			EV_DoElevator(line->args[0], line, elevateContinuous);
			break;

		case 429:  
			if (line->args[1] == TMP_FLOOR)
				EV_DoFloor(line->args[0], line, crushFloorOnce);
			else if (line->args[1] == TMP_CEILING)
				EV_DoCrush(line->args[0], line, crushCeilOnce);
			else
				EV_DoCrush(line->args[0], line, crushBothOnce);
			break;

		case 432:  
			if (mo && mo->player)
			{
				if (line->args[0])
					mo->flags2 &= ~MF2_TWOD;
				else
					mo->flags2 |= MF2_TWOD;



				if (bot && (bot->flags2 & MF2_TWOD) != (mo->flags2 & MF2_TWOD)) {
					bot->flags2 = (bot->flags2 & ~MF2_TWOD) | (mo->flags2 & MF2_TWOD);
					P_SetOrigin(bot, mo->x, mo->y, mo->z);
				}
			}
			break;

		case 433:  
			if (line->args[1])
				mo->flags2 ^= MF2_OBJECTFLIP;
			else if (line->args[0])
				mo->flags2 &= ~MF2_OBJECTFLIP;
			else
				mo->flags2 |= MF2_OBJECTFLIP;
			if (line->args[2])
				mo->eflags |= MFE_VERTICALFLIP;
			if (bot)
				bot->flags2 = (bot->flags2 & ~MF2_OBJECTFLIP) | (mo->flags2 & MF2_OBJECTFLIP);
			break;

		case 434:  
			if (mo && mo->player)
			{
				powertype_t power = line->stringargs[0] ? get_number(line->stringargs[0]) : 0;
				INT32 value = line->stringargs[1] ? get_number(line->stringargs[1]) : 0;
				if (value == -1)  
					value = UINT16_MAX;

				P_SetPower(mo->player, power, value);

				if (bot)
					P_SetPower(bot->player, power, value);
			}
			break;

		case 435:  
			{
				scroll_t *scroller;
				thinker_t *th;

				fixed_t length = R_PointToDist2(line->v2->x, line->v2->y, line->v1->x, line->v1->y);
				fixed_t speed = line->args[1] << FRACBITS;
				fixed_t dx = FixedMul(FixedMul(FixedDiv(line->dx, length), speed) >> SCROLL_SHIFT, CARRYFACTOR);
				fixed_t dy = FixedMul(FixedMul(FixedDiv(line->dy, length), speed) >> SCROLL_SHIFT, CARRYFACTOR);

				for (th = thlist[THINK_MAIN].next; th != &thlist[THINK_MAIN]; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)T_Scroll)
						continue;

					scroller = (scroll_t *)th;
					if (!Tag_Find(&sectors[scroller->affectee].tags, line->args[0]))
						continue;

					scroller->dx = dx;
					scroller->dy = dy;
				}
			}
			break;

		case 436:  
			{
				INT16 sectag = (INT16)(line->args[0]);
				INT16 foftag = (INT16)(line->args[1]);
				sector_t *sec;  
				ffloor_t *rover;  
				boolean foundrover = false;  

				TAG_ITER_SECTORS(sectag, secnum)
				{
					sec = sectors + secnum;

					if (!sec->ffloors)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 436 Executor: Target sector #%d has no FOFs.\n", secnum);
						return;
					}

					for (rover = sec->ffloors; rover; rover = rover->next)
					{
						if (Tag_Find(&rover->master->frontsector->tags, foftag))
						{
							foundrover = true;

							EV_CrumbleChain(sec, rover);
						}
					}

					if (!foundrover)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 436 Executor: Can't find a FOF control sector with tag %d\n", foftag);
						return;
					}
				}
			}
			break;

		case 437:  
			if (mo && mo->player)
			{
				UINT16 fractime = (UINT16)(line->args[0]);
				if (fractime < 1)
					fractime = 1;  
				if (line->args[1])
					fractime |= 1<<15;  
				mo->player->powers[pw_nocontrol] = fractime;
				if (bot)
					bot->player->powers[pw_nocontrol] = fractime;
			}
			break;

		case 438:  
			if (mo)
			{
				mo->destscale = FixedDiv(line->args[0]<<FRACBITS, 100<<FRACBITS);
				if (mo->destscale < FRACUNIT/100)
					mo->destscale = FRACUNIT/100;
				if (mo->player && bot)
					bot->destscale = mo->destscale;
			}
			break;

		case 439:  
			{
				size_t linenum;
				side_t *setfront = &sides[line->sidenum[0]];
				side_t *setback = (line->args[3] && line->sidenum[1] != NO_SIDEDEF) ? &sides[line->sidenum[1]] : setfront;
				side_t *this;
				boolean always = !(line->args[2]);  

				for (linenum = 0; linenum < numlines; linenum++)
				{
					if (lines[linenum].special == 439)
						continue;  

					if (!Tag_Find(&lines[linenum].tags, line->args[0]))
						continue;  


					if (line->args[1] != TMSD_BACK)
					{
						this = &sides[lines[linenum].sidenum[0]];
						if (always || this->toptexture) this->toptexture = setfront->toptexture;
						if (always || this->midtexture) this->midtexture = setfront->midtexture;
						if (always || this->bottomtexture) this->bottomtexture = setfront->bottomtexture;
					}


					if (line->args[1] != TMSD_FRONT && lines[linenum].sidenum[1] != NO_SIDEDEF)
					{
						this = &sides[lines[linenum].sidenum[1]];
						if (always || this->toptexture) this->toptexture = setback->toptexture;
						if (always || this->midtexture) this->midtexture = setback->midtexture;
						if (always || this->bottomtexture) this->bottomtexture = setback->bottomtexture;
					}
				}
			}
			break;

		case 440:  
			if (!metalrecording && !metalplayback)
				G_DoPlayMetal();
			break;

		case 441:  
			{
				INT32 trigid = line->args[0];

				if (trigid < 0 || trigid > 31)  
					CONS_Debug(DBG_GAMELOGIC, "Unlockable trigger (sidedef %u): bad trigger ID %d\n", line->sidenum[0], trigid);
				else
				{
					unlocktriggers |= 1 << trigid;


					M_SilentUpdateUnlockablesAndEmblems(serverGamedata);

					if (M_UpdateUnlockablesAndExtraEmblems(clientGamedata))
					{
						S_StartSound(NULL, sfx_s3k68);
						G_SaveGameData(clientGamedata);  
					}
				}
			}


			line->special = 0;
			break;

		case 442:  
		{
			const mobjtype_t type = line->stringargs[0] ? get_number(line->stringargs[0]) : MT_NULL;
			statenum_t state = NUMSTATES;
			mobj_t *thing;

			if (type < 0 || type >= NUMMOBJTYPES)
				break;

			if (!line->args[1])
			{
				state = line->stringargs[1] ? get_number(line->stringargs[1]) : S_NULL;

				if (state < 0 || state >= NUMSTATES)
					break;
			}

			TAG_ITER_SECTORS(line->args[0], secnum)
			{
				boolean tryagain;
				do {
					tryagain = false;
					for (thing = sectors[secnum].thinglist; thing; thing = thing->snext)
					{
						if (thing->type != type)
							continue;

						if (!P_SetMobjState(thing, line->args[1] ? thing->state->nextstate : state))
						{  
							tryagain = true;  
							break;
						}
					}
				} while (tryagain);
			}
			break;
		}

		case 443:  
			if (line->stringargs[0])
				LUA_HookLinedefExecute(line, mo, callsec);
			else
				CONS_Alert(CONS_WARNING, "Linedef %s is missing the hook name of the Lua function to call! (This should be given in stringarg0)\n", sizeu1(line-lines));
			break;

		case 444:  
		{
			quake.intensity = line->args[1] << FRACBITS;
			quake.radius = line->args[2] << FRACBITS;
			quake.time = line->args[0];

			quake.epicenter = NULL;  


			if (!quake.intensity)
				quake.intensity = 8<<FRACBITS;
			if (!quake.radius)
				quake.radius = 512<<FRACBITS;
			break;
		}

		case 445:  
			{
				INT16 sectag = (INT16)(line->args[0]);
				INT16 foftag = (INT16)(line->args[1]);
				sector_t *sec;  
				ffloor_t *rover;  
				boolean foundrover = false;  
				ffloortype_e oldflags;  

				TAG_ITER_SECTORS(sectag, secnum)
				{
					sec = sectors + secnum;

					if (!sec->ffloors)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 445 Executor: Target sector #%d has no FOFs.\n", secnum);
						return;
					}

					for (rover = sec->ffloors; rover; rover = rover->next)
					{
						if (Tag_Find(&rover->master->frontsector->tags, foftag))
						{
							foundrover = true;

							oldflags = rover->fofflags;


							if (line->args[2])
								rover->fofflags |= FOF_EXISTS;
							else
								rover->fofflags &= ~FOF_EXISTS;


							if (rover->fofflags != oldflags)
							{
								sec->moved = true;
								P_RecalcPrecipInSector(sec);
							}
						}
					}

					if (!foundrover)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 445 Executor: Can't find a FOF control sector with tag %d\n", foftag);
						return;
					}
				}
			}
			break;

		case 446:  
			{
				INT16 sectag = (INT16)(line->args[0]);
				INT16 foftag = (INT16)(line->args[1]);
				sector_t *sec;  
				ffloor_t *rover;  
				boolean foundrover = false;  
				player_t *player = NULL;  
				boolean respawn = true;  

				if (mo)  
					player = mo->player;

				if (line->args[2] & TMFR_NORETURN)  
					respawn = false;

				TAG_ITER_SECTORS(sectag, secnum)
				{
					sec = sectors + secnum;

					if (!sec->ffloors)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 446 Executor: Target sector #%d has no FOFs.\n", secnum);
						return;
					}

					for (rover = sec->ffloors; rover; rover = rover->next)
					{
						if (Tag_Find(&rover->master->frontsector->tags, foftag))
						{
							foundrover = true;

							if (line->args[2] & TMFR_CHECKFLAG)  
								respawn = !(rover->fofflags & FOF_NORETURN) ^ !!(line->args[2] & TMFR_NORETURN);  

							EV_StartCrumble(rover->master->frontsector, rover, (rover->fofflags & FOF_FLOATBOB), player, rover->alpha, respawn);
						}
					}

					if (!foundrover)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 446 Executor: Can't find a FOF control sector with tag %d\n", foftag);
						return;
					}
				}
			}
			break;

		case 447:  




		{
			extracolormap_t *source;
			if (!udmf)
				source = sides[line->sidenum[0]].colormap_data;
			else
			{
				if (!line->args[1])
					source = line->frontsector->extra_colormap;
				else
				{
					INT32 sourcesec = Tag_Iterate_Sectors(line->args[1], 0);
					if (sourcesec == -1)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 447 Executor: Can't find sector with source colormap (tag %d)!\n", line->args[1]);
						return;
					}
					source = sectors[sourcesec].extra_colormap;
				}
			}
			TAG_ITER_SECTORS(line->args[0], secnum)
			{
				if (sectors[secnum].colormap_protected)
					continue;

				P_ResetColormapFader(&sectors[secnum]);

				if (line->args[2] & TMCF_RELATIVE)
				{
					extracolormap_t *target = (!udmf && (line->flags & ML_TFERLINE) && line->sidenum[1] != NO_SIDEDEF) ?
						sides[line->sidenum[1]].colormap_data : sectors[secnum].extra_colormap;  

						extracolormap_t *exc = R_AddColormaps(
							target,
							source,
							line->args[2] & TMCF_SUBLIGHTR,
							line->args[2] & TMCF_SUBLIGHTG,
							line->args[2] & TMCF_SUBLIGHTB,
							line->args[2] & TMCF_SUBLIGHTA,
							line->args[2] & TMCF_SUBFADER,
							line->args[2] & TMCF_SUBFADEG,
							line->args[2] & TMCF_SUBFADEB,
							line->args[2] & TMCF_SUBFADEA,
							line->args[2] & TMCF_SUBFADESTART,
							line->args[2] & TMCF_SUBFADEEND,
							line->args[2] & TMCF_IGNOREFLAGS,
							false);

					if (!(sectors[secnum].extra_colormap = R_GetColormapFromList(exc)))
					{
						exc->colormap = R_CreateLightTable(exc);
						R_AddColormapToList(exc);
						sectors[secnum].extra_colormap = exc;
					}
					else
						Z_Free(exc);
				}
				else
					sectors[secnum].extra_colormap = source;
			}
			break;
		}
		case 448:  
			if ((mo && mo->player && P_IsLocalPlayer(mo->player)) || line->args[3])
			{
				INT32 viewid = line->args[0];
				INT32 centerid = line->args[1];


				if (line->args[2] != TMS_CENTERPOINT)
				{
					if (viewid >= 0 && viewid < 16)
						skyboxmo[0] = skyboxviewpnts[viewid];
					else
						skyboxmo[0] = NULL;
				}


				if (line->args[2] != TMS_VIEWPOINT)
				{
					if (centerid >= 0 && centerid < 16)
						skyboxmo[1] = skyboxcenterpnts[centerid];
					else
						skyboxmo[1] = NULL;
				}

				CONS_Debug(DBG_GAMELOGIC, "Line type 448 Executor: viewid = %d, centerid = %d, viewpoint? = %s, centerpoint? = %s\n",
						viewid,
						centerid,
						((line->args[2] == TMS_CENTERPOINT) ? "no" : "yes"),
						((line->args[2] == TMS_VIEWPOINT) ? "no" : "yes"));
			}
			break;

		case 449:  
		{
			INT32 bossid = line->args[0];
			if (bossid & ~15)  
			{
				CONS_Alert(CONS_WARNING,
					M_GetText("Boss enable linedef has an invalid boss ID (%d).\nConsider changing it or removing it entirely.\n"),
					bossid);
				break;
			}
			if (line->args[1])
			{
				bossdisabled |= (1<<bossid);
				CONS_Debug(DBG_GAMELOGIC, "Line type 449 Executor: bossid disabled = %d", bossid);
			}
			else
			{
				bossdisabled &= ~(1<<bossid);
				CONS_Debug(DBG_GAMELOGIC, "Line type 449 Executor: bossid enabled = %d", bossid);
			}
			break;
		}

		case 450:  
			P_LinedefExecute(line->args[0], mo, NULL);
			break;

		case 451:  
		{
			INT32 rvalue1 = line->args[0];
			INT32 rvalue2 = line->args[1];
			INT32 result;

			if (rvalue1 <= rvalue2)
				result = P_RandomRange(rvalue1, rvalue2);
			else
				result = P_RandomRange(rvalue2, rvalue1);

			P_LinedefExecute((INT16)result, mo, NULL);
			break;
		}

		case 452:  
		{
			INT16 destvalue = (INT16)(line->args[2]);
			INT16 sectag = (INT16)(line->args[0]);
			INT16 foftag = (INT16)(line->args[1]);
			sector_t *sec;  
			ffloor_t *rover;  
			boolean foundrover = false;  

			TAG_ITER_SECTORS(sectag, secnum)
			{
				sec = sectors + secnum;

				if (!sec->ffloors)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 452 Executor: Target sector #%d has no FOFs.\n", secnum);
					return;
				}

				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if (Tag_Find(&rover->master->frontsector->tags, foftag))
					{
						foundrover = true;



						if (!(line->args[3] & TMST_DONTDOTRANSLUCENT) &&       
							(rover->spawnflags & FOF_NOSHADE) &&  
							!(rover->spawnflags & FOF_RENDERSIDES) &&
							!(rover->spawnflags & FOF_RENDERPLANES) &&
							!(rover->fofflags & FOF_RENDERALL))
							rover->alpha = 0;

						P_RemoveFakeFloorFader(rover);
						P_FadeFakeFloor(rover,
							rover->alpha,
							max(0, min(255, (line->args[3] & TMST_RELATIVE) ? rover->alpha + destvalue : destvalue)),
							0,                                          
							false, NULL,                                
							false,                                      
							!(line->args[3] & TMST_DONTDOTRANSLUCENT),  
							false,                                      
							false,                                      
							false,                                      
							false,                                      
							true);                                      
					}
				}

				if (!foundrover)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 452 Executor: Can't find a FOF control sector with tag %d\n", foftag);
					return;
				}
			}
			break;
		}

		case 453:  
		{
			INT16 destvalue = (INT16)(line->args[2]);
			INT16 speed = (INT16)(line->args[3]);
			INT16 sectag = (INT16)(line->args[0]);
			INT16 foftag = (INT16)(line->args[1]);
			sector_t *sec;  
			ffloor_t *rover;  
			boolean foundrover = false;  
			size_t j = 0;  

			TAG_ITER_SECTORS(sectag, secnum)
			{
				sec = sectors + secnum;

				if (!sec->ffloors)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 453 Executor: Target sector #%d has no FOFs.\n", secnum);
					return;
				}

				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if (Tag_Find(&rover->master->frontsector->tags, foftag))
					{
						foundrover = true;


						if (!(line->args[4] & TMFT_OVERRIDE)
							&& rover->fadingdata)

						{
							CONS_Debug(DBG_GAMELOGIC, "Line type 453 Executor: Fade FOF thinker already exists, timer: %d\n", ((fade_t*)rover->fadingdata)->timer);
							continue;
						}

						if (speed > 0)
							P_AddFakeFloorFader(rover, secnum, j,
								destvalue,
								speed,
								(line->args[4] & TMFT_TICBASED),            
								(line->args[4] & TMFT_RELATIVE),            
								!(line->args[4] & TMFT_DONTDOEXISTS),       
								!(line->args[4] & TMFT_DONTDOTRANSLUCENT),  
								!(line->args[4] & TMFT_DONTDOLIGHTING),     
								!(line->args[4] & TMFT_DONTDOCOLORMAP),     
								!(line->args[4] & TMFT_IGNORECOLLISION),    
								(line->args[4] & TMFT_GHOSTFADE),           
								(line->args[4] & TMFT_USEEXACTALPHA));      
						else
						{


							if (!(line->args[4] & TMFT_DONTDOTRANSLUCENT) &&       
								(rover->spawnflags & FOF_NOSHADE) &&  
								!(rover->spawnflags & FOF_RENDERSIDES) &&
								!(rover->spawnflags & FOF_RENDERPLANES) &&
								!(rover->fofflags & FOF_RENDERALL))
								rover->alpha = 0;

							P_RemoveFakeFloorFader(rover);
							P_FadeFakeFloor(rover,
								rover->alpha,
								max(0, min(255, (line->args[4] & TMFT_RELATIVE) ? rover->alpha + destvalue : destvalue)),
								0,                                          
								false, NULL,                                
								!(line->args[4] & TMFT_DONTDOEXISTS),       
								!(line->args[4] & TMFT_DONTDOTRANSLUCENT),  
								!(line->args[4] & TMFT_DONTDOLIGHTING),     
								!(line->args[4] & TMFT_DONTDOCOLORMAP),     
								!(line->args[4] & TMFT_IGNORECOLLISION),    
								(line->args[4] & TMFT_GHOSTFADE),           
								(line->args[4] & TMFT_USEEXACTALPHA));      
						}
					}
					j++;
				}

				if (!foundrover)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 453 Executor: Can't find a FOF control sector with tag %d\n", foftag);
					return;
				}
			}
			break;
		}

		case 454:  
		{
			INT16 sectag = (INT16)(line->args[0]);
			INT16 foftag = (INT16)(line->args[1]);
			sector_t *sec;  
			ffloor_t *rover;  
			boolean foundrover = false;  

			TAG_ITER_SECTORS(sectag, secnum)
			{
				sec = sectors + secnum;

				if (!sec->ffloors)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 454 Executor: Target sector #%d has no FOFs.\n", secnum);
					return;
				}

				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if (Tag_Find(&rover->master->frontsector->tags, foftag))
					{
						foundrover = true;

						P_ResetFakeFloorFader(rover, NULL,
							!(line->args[2]));  
					}
				}

				if (!foundrover)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 454 Executor: Can't find a FOF control sector with tag %d\n", foftag);
					return;
				}
			}
			break;
		}

		case 455:  
		{
			extracolormap_t *dest;
			if (!udmf)
				dest = sides[line->sidenum[0]].colormap_data;
			else
			{
				if (!line->args[1])
					dest = line->frontsector->extra_colormap;
				else
				{
					INT32 destsec = Tag_Iterate_Sectors(line->args[1], 0);
					if (destsec == -1)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 455 Executor: Can't find sector with destination colormap (tag %d)!\n", line->args[1]);
						return;
					}
					dest = sectors[destsec].extra_colormap;
				}
			}

			TAG_ITER_SECTORS(line->args[0], secnum)
			{
				extracolormap_t *source_exc, *dest_exc, *exc;

				if (sectors[secnum].colormap_protected)
					continue;


				if (!(line->args[3] & TMCF_OVERRIDE)
					&& sectors[secnum].fadecolormapdata)

				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 455 Executor: Fade color thinker already exists, timer: %d\n", ((fadecolormap_t*)sectors[secnum].fadecolormapdata)->timer);
					continue;
				}

				if (!udmf && (line->flags & ML_TFERLINE))  
					sectors[secnum].extra_colormap = (line->sidenum[1] != NO_SIDEDEF) ?
					sides[line->sidenum[1]].colormap_data : NULL;

				exc = sectors[secnum].extra_colormap;

				if (!(line->args[3] & TMCF_FROMBLACK)  
					&& !R_CheckDefaultColormap(dest, true, false, false)
					&& R_CheckDefaultColormap(exc, true, false, false))
				{
					exc = R_CopyColormap(exc, false);
					exc->rgba = R_GetRgbaRGB(dest->rgba) + R_PutRgbaA(R_GetRgbaA(exc->rgba));


					if (!(source_exc = R_GetColormapFromList(exc)))
					{
						exc->colormap = R_CreateLightTable(exc);
						R_AddColormapToList(exc);
						source_exc = exc;
					}
					else
						Z_Free(exc);

					sectors[secnum].extra_colormap = source_exc;
				}
				else
					source_exc = exc ? exc : R_GetDefaultColormap();

				if (line->args[3] & TMCF_RELATIVE)
				{
					exc = R_AddColormaps(
						source_exc,
						dest,
						line->args[3] & TMCF_SUBLIGHTR,
						line->args[3] & TMCF_SUBLIGHTG,
						line->args[3] & TMCF_SUBLIGHTB,
						line->args[3] & TMCF_SUBLIGHTA,
						line->args[3] & TMCF_SUBFADER,
						line->args[3] & TMCF_SUBFADEG,
						line->args[3] & TMCF_SUBFADEB,
						line->args[3] & TMCF_SUBFADEA,
						line->args[3] & TMCF_SUBFADESTART,
						line->args[3] & TMCF_SUBFADEEND,
						line->args[3] & TMCF_IGNOREFLAGS,
						false);
				}
				else
					exc = R_CopyColormap(dest, false);

				if (!(dest_exc = R_GetColormapFromList(exc)))
				{
					exc->colormap = R_CreateLightTable(exc);
					R_AddColormapToList(exc);
					dest_exc = exc;
				}
				else
					Z_Free(exc);

				Add_ColormapFader(&sectors[secnum], source_exc, dest_exc, true,  
					line->args[2]);
			}
			break;
		}
		case 456:  
			TAG_ITER_SECTORS(line->args[0], secnum)
				P_ResetColormapFader(&sectors[secnum]);
			break;

		case 457:  
			if (mo)
			{
				INT32 failureangle = FixedAngle((min(max(abs(line->args[1]), 0), 360))*FRACUNIT);
				INT32 failuredelay = abs(line->args[2]);
				INT32 failureexectag = line->args[3];
				boolean persist = !!(line->args[4]);
				mobj_t *anchormo;

				anchormo = P_FindObjectTypeFromTag(MT_ANGLEMAN, line->args[0]);
				if (!anchormo)
					return;

				mo->eflags |= MFE_TRACERANGLE;
				P_SetTarget(&mo->tracer, anchormo);
				mo->lastlook = persist;  
				mo->extravalue1 = failureangle;  
				mo->extravalue2 = failureexectag;  
				mo->cusval = mo->cvmem = failuredelay;  
			}
			break;

		case 458:  
			if (mo && (mo->eflags & MFE_TRACERANGLE))
			{
				mo->eflags &= ~MFE_TRACERANGLE;
				P_SetTarget(&mo->tracer, NULL);
				mo->lastlook = mo->cvmem = mo->cusval = mo->extravalue1 = mo->extravalue2 = 0;
			}
			break;

		case 459:  

			if (mo && mo->player && P_IsLocalPlayer(mo->player) && (!bot || bot != mo))
			{
				INT32 promptnum = max(0, line->args[0] - 1);
				INT32 pagenum = max(0, line->args[1] - 1);
				INT32 postexectag = abs(line->args[3]);

				boolean closetextprompt = (line->args[2] & TMP_CLOSE);

				boolean runpostexec = (line->args[2] & TMP_RUNPOSTEXEC);
				boolean blockcontrols = !(line->args[2] & TMP_KEEPCONTROLS);
				boolean freezerealtime = !(line->args[2] & TMP_KEEPREALTIME);

				boolean callbynamedtag = (line->args[2] & TMP_CALLBYNAME);

				if (closetextprompt)
					F_EndTextPrompt(false, false);
				else
				{
					if (callbynamedtag && line->stringargs[0] && line->stringargs[0][0])
						F_GetPromptPageByNamedTag(line->stringargs[0], &promptnum, &pagenum);
					F_StartTextPrompt(promptnum, pagenum, mo, runpostexec ? postexectag : 0, blockcontrols, freezerealtime);
				}
			}
			break;

		case 460:  
			{
				INT16 rings = line->args[0];
				INT32 delay = line->args[1];
				if (mo && mo->player)
				{
					if (delay <= 0 || !(leveltime % delay))
						P_GivePlayerRings(mo->player, rings);
				}
			}
			break;

		case 461:  
			{
				const mobjtype_t type = line->stringargs[0] ? get_number(line->stringargs[0]) : MT_NULL;
				mobj_t *mobj;

				fixed_t x, y, z;

				if (line->args[4])  
				{
					x = P_RandomRange(line->args[0], line->args[5])<<FRACBITS;
					y = P_RandomRange(line->args[1], line->args[6])<<FRACBITS;
					z = P_RandomRange(line->args[2], line->args[7])<<FRACBITS;
				}
				else
				{
					x = line->args[0] << FRACBITS;
					y = line->args[1] << FRACBITS;
					z = line->args[2] << FRACBITS;
				}

				mobj = P_SpawnMobj(x, y, z, type);
				if (mobj)
				{
					mobj->angle = FixedAngle(line->args[3] << FRACBITS);
					CONS_Debug(DBG_GAMELOGIC, "Linedef Type %d - Spawn Object: %d spawned at (%d, %d, %d)\n", line->special, mobj->type, mobj->x>>FRACBITS, mobj->y>>FRACBITS, mobj->z>>FRACBITS);  
				}
				else
					CONS_Alert(CONS_ERROR,"Linedef Type %d - Spawn Object: Object did not spawn!\n", line->special);
			}
			break;

		case 462:  
			if (G_PlatformGametype())
			{
				stoppedclock = true;
				CONS_Debug(DBG_GAMELOGIC, "Clock stopped!\n");
				if (modeattacking)
				{
					UINT8 i;
					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i])
							continue;
						P_DoPlayerExit(&players[i], true);
					}
				}
			}
			break;

		case 463:  
			{
				if (mo)
				{
					INT32 color = line->stringargs[0] ? get_number(line->stringargs[0]) : SKINCOLOR_NONE;

					if (color < 0 || color >= numskincolors)
						return;

					var1 = 0;
					var2 = color;
					A_Dye(mo);
				}
			}
			break;

		case 464:  
			{
				INT32 mtnum;
				mobj_t *mo2;



				TAG_ITER_THINGS(line->args[0], mtnum)
				{
					mo2 = mapthings[mtnum].mobj;

					if (!mo2)
						continue;

					if (mo2->type != MT_EGGTRAP)
						continue;

					if (mo2->thinker.removing)
						continue;

					P_KillMobj(mo2, NULL, mo, 0);
				}

				if (!(line->args[1]))
				{
					INT32 i;


					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i])
							continue;
						P_DoPlayerExit(&players[i], true);
					}
				}
			}
			break;

		case 465:  
			{
				INT32 linenum;

				if (!udmf)
					break;

				TAG_ITER_LINES(line->args[0], linenum)
				{
					if (line->args[2])
						lines[linenum].executordelay += line->args[1];
					else
						lines[linenum].executordelay = line->args[1];
				}
			}
			break;

		case 466:  
			{
				if (line->args[0])
				{
					stagefailed = false;
					CONS_Debug(DBG_GAMELOGIC, "Stage can be completed successfully!\n");
				}
				else
				{
					stagefailed = true;
					CONS_Debug(DBG_GAMELOGIC, "Stage will end in failure...\n");
				}
			}
			break;

		case 467:  
			TAG_ITER_SECTORS(line->args[0], secnum)
			{
				if (sectors[secnum].lightingdata)
				{

					P_RemoveThinker(&((thinkerdata_t *)sectors[secnum].lightingdata)->thinker);
					sectors[secnum].lightingdata = NULL;
				}

				if (line->args[2] == TML_FLOOR)
				{
					if (line->args[3])
						sectors[secnum].floorlightlevel += line->args[1];
					else
						sectors[secnum].floorlightlevel = line->args[1];
				}
				else if (line->args[2] == TML_CEILING)
				{
					if (line->args[3])
						sectors[secnum].ceilinglightlevel += line->args[1];
					else
						sectors[secnum].ceilinglightlevel = line->args[1];
				}
				else
				{
					if (line->args[3])
						sectors[secnum].lightlevel += line->args[1];
					else
						sectors[secnum].lightlevel = line->args[1];
					sectors[secnum].lightlevel = max(0, min(255, sectors[secnum].lightlevel));
				}
			}
			break;

		case 468:  
		{
			INT32 linenum;

			if (!udmf)
				break;

			if (line->args[1] < 0 || line->args[1] >= NUMLINEARGS)
			{
				CONS_Debug(DBG_GAMELOGIC, "Linedef type 468: Invalid linedef arg %d\n", line->args[1]);
				break;
			}

			TAG_ITER_LINES(line->args[0], linenum)
			{
				if (line->args[3])
					lines[linenum].args[line->args[1]] += line->args[2];
				else
					lines[linenum].args[line->args[1]] = line->args[2];
			}
		}
		break;

		case 469:  
		{
			fixed_t gravityvalue;

			if (!udmf)
				break;

			if (!line->stringargs[0])
				break;

			gravityvalue = FloatToFixed(atof(line->stringargs[0]));

			TAG_ITER_SECTORS(line->args[0], secnum)
			{
				if (line->args[1])
					sectors[secnum].gravity = FixedMul(sectors[secnum].gravity, gravityvalue);
				else
					sectors[secnum].gravity = gravityvalue;

				if (line->args[2] == TMF_ADD)
					sectors[secnum].flags |= MSF_GRAVITYFLIP;
				else if (line->args[2] == TMF_REMOVE)
					sectors[secnum].flags &= ~MSF_GRAVITYFLIP;

				if (line->args[3])
					sectors[secnum].specialflags |= SSF_GRAVITYOVERRIDE;
			}
		}
		break;

		case 480:  
		case 481:  
			PolyDoor(line);
			break;
		case 482:  
			PolyMove(line);
			break;
		case 484:  
			PolyRotate(line);
			break;
		case 488:  
			PolyWaypoint(line);
			break;
		case 489:
			PolySetVisibilityTangibility(line);
			break;
		case 491:
			PolyTranslucency(line);
			break;
		case 492:
			PolyFade(line);
			break;

		default:
			break;
	}
}







void P_SetupSignExit(player_t *player)
{
	mobj_t *thing;
	msecnode_t *node = player->mo->subsector->sector->touching_thinglist;  
	thinker_t *think;
	INT32 numfound = 0;

	for (; node; node = node->m_thinglist_next)
	{
		thing = node->m_thing;
		if (thing->type != MT_SIGN)
			continue;

		if (!numfound
			&& !(player->mo->target && player->mo->target->type == MT_SIGN)
			&& !((gametyperules & GTR_FRIENDLY) && (netgame || multiplayer) && cv_exitmove.value))
				P_SetTarget(&player->mo->target, thing);

		if (thing->state != &states[thing->info->spawnstate])
			continue;

		P_SetTarget(&thing->target, player->mo);
		P_SetObjectMomZ(thing, 12*FRACUNIT, false);
		P_SetMobjState(thing, S_SIGNSPIN1);
		if (thing->info->seesound)
			S_StartSound(thing, thing->info->seesound);

		++numfound;
	}

	if (numfound)
		return;



	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->removing)
			continue;

		thing = (mobj_t *)think;
		if (thing->type != MT_SIGN)
			continue;

		if (!numfound
			&& !(player->mo->target && player->mo->target->type == MT_SIGN)
			&& !((gametyperules & GTR_FRIENDLY) && (netgame || multiplayer) && cv_exitmove.value))
				P_SetTarget(&player->mo->target, thing);

		if (thing->state != &states[thing->info->spawnstate])
			continue;

		P_SetTarget(&thing->target, player->mo);
		P_SetObjectMomZ(thing, 12*FRACUNIT, false);
		P_SetMobjState(thing, S_SIGNSPIN1);
		if (thing->info->seesound)
			S_StartSound(thing, thing->info->seesound);

		++numfound;
	}
}






boolean P_IsFlagAtBase(mobjtype_t flag)
{
	thinker_t *think;
	mobj_t *mo;
	sectorspecialflags_t specialflag = (flag == MT_REDFLAG) ? SSF_REDTEAMBASE : SSF_BLUETEAMBASE;

	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->removing)
			continue;

		mo = (mobj_t *)think;

		if (mo->type != flag)
			continue;

		if (mo->subsector->sector->specialflags & specialflag)
			return true;
		else if (mo->subsector->sector->ffloors)  
		{
			ffloor_t *rover;

			for (rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->fofflags & FOF_EXISTS))
					continue;

				if (!(rover->master->frontsector->specialflags & specialflag))
					continue;

				if (!(mo->z <= P_GetSpecialTopZ(mo, sectors + rover->secnum, mo->subsector->sector)
					&& mo->z >= P_GetSpecialBottomZ(mo, sectors + rover->secnum, mo->subsector->sector)))
					continue;

				return true;
			}
		}
	}
	return false;
}

static boolean P_IsMobjTouchingPlane(mobj_t *mo, sector_t *sec, fixed_t floorz, fixed_t ceilingz)
{
	boolean floorallowed = ((sec->flags & MSF_FLIPSPECIAL_FLOOR) && ((sec->flags & MSF_TRIGGERSPECIAL_HEADBUMP) || !(mo->eflags & MFE_VERTICALFLIP)) && (mo->z == floorz));
	boolean ceilingallowed = ((sec->flags & MSF_FLIPSPECIAL_CEILING) && ((sec->flags &  MSF_TRIGGERSPECIAL_HEADBUMP) || (mo->eflags & MFE_VERTICALFLIP)) && (mo->z + mo->height == ceilingz));
	return (floorallowed || ceilingallowed);
}

boolean P_IsMobjTouchingSectorPlane(mobj_t *mo, sector_t *sec)
{
	return P_IsMobjTouchingPlane(mo, sec, P_GetSpecialBottomZ(mo, sec, sec), P_GetSpecialTopZ(mo, sec, sec));
}

boolean P_IsMobjTouching3DFloor(mobj_t *mo, ffloor_t *ffloor, sector_t *sec)
{
	fixed_t topheight = P_GetSpecialTopZ(mo, sectors + ffloor->secnum, sec);
	fixed_t bottomheight = P_GetSpecialBottomZ(mo, sectors + ffloor->secnum, sec);

	if (((ffloor->fofflags & FOF_BLOCKPLAYER) && mo->player)
		|| ((ffloor->fofflags & FOF_BLOCKOTHERS) && !mo->player))
	{

		return P_IsMobjTouchingPlane(mo, ffloor->master->frontsector, topheight, bottomheight);
	}
	else
	{

		return mo->z <= topheight && (mo->z + mo->height) >= bottomheight;
	}
}

boolean P_IsMobjTouchingPolyobj(mobj_t *mo, polyobj_t *po, sector_t *polysec)
{
	if (!(po->flags & POF_TESTHEIGHT))  
		return true;

	if (po->flags & POF_SOLID)
	{

		return P_IsMobjTouchingPlane(mo, polysec, polysec->ceilingheight, polysec->floorheight);
	}
	else
	{

		return mo->z <= polysec->ceilingheight && (mo->z + mo->height) >= polysec->floorheight;
	}
}

static sector_t *P_MobjTouching3DFloorSpecial(mobj_t *mo, sector_t *sector, INT32 section, INT32 number)
{
	ffloor_t *rover;

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if (GETSECSPECIAL(rover->master->frontsector->special, section) != number)
			continue;

		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!P_IsMobjTouching3DFloor(mo, rover, sector))
			continue;


		if (sector == mo->subsector->sector
			|| (rover->master->frontsector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			return rover->master->frontsector;
	}

	return NULL;
}

static sector_t *P_MobjTouching3DFloorSpecialFlag(mobj_t *mo, sector_t *sector, sectorspecialflags_t flag)
{
	ffloor_t *rover;

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if (!(rover->master->frontsector->specialflags & flag))
			continue;

		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!P_IsMobjTouching3DFloor(mo, rover, sector))
			continue;


		if (sector == mo->subsector->sector
			|| (rover->master->frontsector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			return rover->master->frontsector;
	}

	return NULL;
}

static sector_t *P_MobjTouchingPolyobjSpecial(mobj_t *mo, INT32 section, INT32 number)
{
	polyobj_t *po;
	sector_t *polysec;
	boolean touching = false;
	boolean inside = false;

	for (po = mo->subsector->polyList; po; po = (polyobj_t *)(po->link.next))
	{
		if (po->flags & POF_NOSPECIALS)
			continue;

		polysec = po->lines[0]->backsector;

		if (GETSECSPECIAL(polysec->special, section) != number)
			continue;

		touching = (polysec->flags & MSF_TRIGGERSPECIAL_TOUCH) && P_MobjTouchingPolyobj(po, mo);
		inside = P_MobjInsidePolyobj(po, mo);

		if (!(inside || touching))
			continue;

		if (!P_IsMobjTouchingPolyobj(mo, po, polysec))
			continue;

		return polysec;
	}

	return NULL;
}

static sector_t *P_MobjTouchingPolyobjSpecialFlag(mobj_t *mo, sectorspecialflags_t flag)
{
	polyobj_t *po;
	sector_t *polysec;
	boolean touching = false;
	boolean inside = false;

	for (po = mo->subsector->polyList; po; po = (polyobj_t *)(po->link.next))
	{
		if (po->flags & POF_NOSPECIALS)
			continue;

		polysec = po->lines[0]->backsector;

		if (!(polysec->specialflags & flag))
			continue;

		touching = (polysec->flags & MSF_TRIGGERSPECIAL_TOUCH) && P_MobjTouchingPolyobj(po, mo);
		inside = P_MobjInsidePolyobj(po, mo);

		if (!(inside || touching))
			continue;

		if (!P_IsMobjTouchingPolyobj(mo, po, polysec))
			continue;

		return polysec;
	}

	return NULL;
}

sector_t *P_MobjTouchingSectorSpecial(mobj_t *mo, INT32 section, INT32 number)
{
	msecnode_t *node;
	sector_t *result;

	result = P_MobjTouching3DFloorSpecial(mo, mo->subsector->sector, section, number);
	if (result)
		return result;

	result = P_MobjTouchingPolyobjSpecial(mo, section, number);
	if (result)
		return result;

	if (GETSECSPECIAL(mo->subsector->sector->special, section) == number)
		return mo->subsector->sector;

	for (node = mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (node->m_sector == mo->subsector->sector)  
			continue;

		result = P_MobjTouching3DFloorSpecial(mo, node->m_sector, section, number);
		if (result)
			return result;

		if (!(node->m_sector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			continue;

		if (GETSECSPECIAL(node->m_sector->special, section) == number)
			return node->m_sector;
	}

	return NULL;
}




sector_t *P_ThingOnSpecial3DFloor(mobj_t *mo)
{
	ffloor_t *rover;

	for (rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
	{
		if (!rover->master->frontsector->special)
			continue;

		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!P_IsMobjTouching3DFloor(mo, rover, mo->subsector->sector))
			continue;

		return rover->master->frontsector;
	}

	return NULL;
}

sector_t *P_MobjTouchingSectorSpecialFlag(mobj_t *mo, sectorspecialflags_t flag)
{
	msecnode_t *node;
	sector_t *result;

	result = P_MobjTouching3DFloorSpecialFlag(mo, mo->subsector->sector, flag);
	if (result)
		return result;

	result = P_MobjTouchingPolyobjSpecialFlag(mo, flag);
	if (result)
		return result;

	if (mo->subsector->sector->specialflags & flag)
		return mo->subsector->sector;

	for (node = mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (node->m_sector == mo->subsector->sector)  
			continue;

		result = P_MobjTouching3DFloorSpecialFlag(mo, node->m_sector, flag);
		if (result)
			return result;

		if (!(node->m_sector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			continue;

		if (node->m_sector->specialflags & flag)
			return node->m_sector;
	}

	return NULL;
}












sector_t *P_PlayerTouchingSectorSpecial(player_t *player, INT32 section, INT32 number)
{
	if (!player->mo)
		return NULL;

	return P_MobjTouchingSectorSpecial(player->mo, section, number);
}

sector_t *P_PlayerTouchingSectorSpecialFlag(player_t *player, sectorspecialflags_t flag)
{
	if (!player->mo)
		return NULL;

	return P_MobjTouchingSectorSpecialFlag(player->mo, flag);
}

static sector_t *P_CheckPlayer3DFloorTrigger(player_t *player, sector_t *sector, line_t *sourceline)
{
	ffloor_t *rover;

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if (!rover->master->frontsector->triggertag)
			continue;

		if (rover->master->frontsector->triggerer == TO_MOBJ)
			continue;

		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!Tag_Find(&sourceline->tags, rover->master->frontsector->triggertag))
			continue;

		if (!P_IsMobjTouching3DFloor(player->mo, rover, sector))
			continue;


		if (sector == player->mo->subsector->sector
			|| (rover->master->frontsector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			return rover->master->frontsector;
	}

	return NULL;
}

static sector_t *P_CheckPlayerPolyobjTrigger(player_t *player, line_t *sourceline)
{
	polyobj_t *po;
	sector_t *polysec;
	boolean touching = false;
	boolean inside = false;

	for (po = player->mo->subsector->polyList; po; po = (polyobj_t *)(po->link.next))
	{
		if (po->flags & POF_NOSPECIALS)
			continue;

		polysec = po->lines[0]->backsector;

		if (!polysec->triggertag)
			continue;

		if (polysec->triggerer == TO_MOBJ)
			continue;

		if (!Tag_Find(&sourceline->tags, polysec->triggertag))
			continue;

		touching = (polysec->flags & MSF_TRIGGERSPECIAL_TOUCH) && P_MobjTouchingPolyobj(po, player->mo);
		inside = P_MobjInsidePolyobj(po, player->mo);

		if (!(inside || touching))
			continue;

		if (!P_IsMobjTouchingPolyobj(player->mo, po, polysec))
			continue;

		return polysec;
	}

	return NULL;
}

static boolean P_CheckPlayerSectorTrigger(player_t *player, sector_t *sector, line_t *sourceline)
{
	if (!sector->triggertag)
		return false;

	if (sector->triggerer == TO_MOBJ)
		return false;

	if (!Tag_Find(&sourceline->tags, sector->triggertag))
		return false;

	if (!(sector->flags & MSF_TRIGGERLINE_PLANE))
		return true;  

	return P_IsMobjTouchingSectorPlane(player->mo, sector);

}

sector_t *P_FindPlayerTrigger(player_t *player, line_t *sourceline)
{
	sector_t *originalsector;
	sector_t *loopsector;
	msecnode_t *node;
	sector_t *caller;

	if (!player->mo)
		return NULL;

	originalsector = player->mo->subsector->sector;

	caller = P_CheckPlayer3DFloorTrigger(player, originalsector, sourceline);  

	if (caller)
		return caller;


	caller = P_CheckPlayerPolyobjTrigger(player, sourceline);

	if (caller)
		return caller;

	if (P_CheckPlayerSectorTrigger(player, originalsector, sourceline))
		return originalsector;


	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		loopsector = node->m_sector;

		if (loopsector == originalsector)  
			continue;


		caller = P_CheckPlayer3DFloorTrigger(player, loopsector, sourceline);  

		if (caller)
			return caller;

		if (!(loopsector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			continue;

		if (P_CheckPlayerSectorTrigger(player, loopsector, sourceline))
			return loopsector;
	}

	return NULL;
}

boolean P_IsPlayerValid(size_t playernum)
{
	if (!playeringame[playernum])
		return false;

	if (!players[playernum].mo)
		return false;

	if (players[playernum].mo->health <= 0)
		return false;

	if (players[playernum].spectator)
		return false;

	return true;
}

boolean P_CanPlayerTrigger(size_t playernum)
{
	return P_IsPlayerValid(playernum) && !players[playernum].bot;
}


static boolean P_DoAllPlayersTrigger(mtag_t triggertag)
{
	INT32 i;
	line_t dummyline;
	dummyline.tags.count = 1;
	dummyline.tags.tags = &triggertag;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!P_CanPlayerTrigger(i))
			continue;
		if (!P_FindPlayerTrigger(&players[i], &dummyline))
			return false;
	}

	return true;
}

static void P_ProcessEggCapsule(player_t *player, sector_t *sector)
{
	thinker_t *th;
	mobj_t *mo2;
	INT32 i;

	if (player->bot || sector->ceilingdata || sector->floordata)
		return;



	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;
		mo2 = (mobj_t *)th;
		if (mo2->type != MT_EGGTRAP)
			continue;
		P_KillMobj(mo2, NULL, player->mo, 0);
	}


	sector->special = 0;


	EV_DoElevator(LE_CAPSULE0, NULL, elevateDown);


	EV_DoFloor(LE_CAPSULE1, NULL, raiseFloorToNearestFast);

	EV_DoCeiling(LE_CAPSULE2, NULL, lowerToLowestFast);


	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;
		P_DoPlayerExit(&players[i], true);
	}
}

static void P_ProcessSpeedPad(player_t *player, sector_t *sector, sector_t *roversector, mtag_t sectag)
{
	INT32 lineindex = -1;
	angle_t lineangle;
	fixed_t linespeed;
	fixed_t sfxnum;
	size_t i;

	if (player->powers[pw_flashing] != 0 && player->powers[pw_flashing] < TICRATE/2)
		return;


	for (i = 0; i < sector->linecount; i++)
	{
		line_t *li = sector->lines[i];

		if (li->frontsector != sector)
			continue;

		if (li->special != 4)
			continue;

		if (!Tag_Find(&li->tags, 0))
			continue;

		lineindex = li - lines;
		break;
	}


	if (lineindex == -1)
		lineindex = Tag_FindLineSpecial(4, sectag);

	if (lineindex == -1)
	{
		CONS_Debug(DBG_GAMELOGIC, "ERROR: Speed pad missing line special #4.\n");
		return;
	}

	lineangle = R_PointToAngle2(lines[lineindex].v1->x, lines[lineindex].v1->y, lines[lineindex].v2->x, lines[lineindex].v2->y);
	linespeed = lines[lineindex].args[0] << FRACBITS;

	if (linespeed == 0)
	{
		CONS_Debug(DBG_GAMELOGIC, "ERROR: Speed pad (tag %d) at zero speed.\n", sectag);
		return;
	}

	player->mo->angle = player->drawangle = lineangle;

	if (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG)
		P_SetPlayerAngle(player, player->mo->angle);

	if (!(lines[lineindex].args[1] & TMSP_NOTELEPORT))
	{
		P_UnsetThingPosition(player->mo);
		if (roversector)  
		{
			player->mo->x = roversector->soundorg.x;
			player->mo->y = roversector->soundorg.y;
		}
		else
		{
			player->mo->x = sector->soundorg.x;
			player->mo->y = sector->soundorg.y;
		}
		P_SetThingPosition(player->mo);
	}

	P_InstaThrust(player->mo, player->mo->angle, linespeed);

	if (lines[lineindex].args[1] & TMSP_FORCESPIN)  
	{
		if (!(player->pflags & PF_SPINNING))
			player->pflags |= PF_SPINNING;

		P_SetMobjState(player->mo, S_PLAY_ROLL);
	}

	player->powers[pw_flashing] = TICRATE/3;

	sfxnum = lines[lineindex].stringargs[0] ? get_number(lines[lineindex].stringargs[0]) : sfx_spdpad;

	if (!sfxnum)
		sfxnum = sfx_spdpad;

	S_StartSound(player->mo, sfxnum);
}

static void P_ProcessSpecialStagePit(player_t* player)
{
	if (!(gametyperules & GTR_ALLOWEXIT))
		return;

	if (player->bot)
		return;

	if (!G_IsSpecialStage(gamemap))
		return;

	if (maptol & TOL_NIGHTS)
		return;

	if (player->nightstime <= 6)
		return;

	player->nightstime = 6;  
}

static void P_ProcessExitSector(player_t *player, mtag_t sectag)
{
	INT32 lineindex;

	if (!(gametyperules & GTR_ALLOWEXIT))
		return;

	if (player->bot)
		return;

	if (G_IsSpecialStage(gamemap) && !(maptol & TOL_NIGHTS))
		return;


	P_DoPlayerFinish(player);

	P_SetupSignExit(player);

	if (!G_CoopGametype())
		return;





	lineindex = Tag_FindLineSpecial(2, sectag);

	if (lineindex == -1)
	{
		CONS_Debug(DBG_GAMELOGIC, "ERROR: Exit sector missing line special #2.\n");
		return;
	}


	if ((lines[lineindex].args[1] & TMEF_EMERALDCHECK) && ALL7EMERALDS(emeralds))
		nextmapoverride = (INT16)(udmf ? lines[lineindex].args[2] : lines[lineindex].frontsector->ceilingheight>>FRACBITS);
	else
		nextmapoverride = (INT16)(udmf ? lines[lineindex].args[0] : lines[lineindex].frontsector->floorheight>>FRACBITS);

	if (lines[lineindex].args[1] & TMEF_SKIPTALLY)
		skipstats = 1;
}

static void P_ProcessTeamBase(player_t *player, boolean redteam)
{
	mobj_t *mo;

	if (!(gametyperules & GTR_TEAMFLAGS))
		return;

	if (!P_IsObjectOnGround(player->mo))
		return;

	if (player->ctfteam != (redteam ? 1 : 2))
		return;

	if (!(player->gotflag & (redteam ? GF_BLUEFLAG : GF_REDFLAG)))
		return;



	if (!P_IsFlagAtBase(redteam ? MT_REDFLAG : MT_BLUEFLAG))
		return;

	HU_SetCEchoFlags(V_AUTOFADEOUT|V_ALLOWLOWERCASE);
	HU_SetCEchoDuration(5);
	HU_DoCEcho(va(M_GetText("%s%s\200\\CAPTURED THE %s%s FLAG\200.\\\\\\\\"), redteam ? "\205" : "\204", player_names[player-players], redteam ? "\204" : "\205", redteam ? "BLUE" : "RED"));

	if (splitscreen || players[consoleplayer].ctfteam == (redteam ? 1 : 2))
		S_StartSound(NULL, sfx_flgcap);
	else if (players[consoleplayer].ctfteam == (redteam ? 2 : 1))
		S_StartSound(NULL, sfx_lose);

	mo = P_SpawnMobj(player->mo->x,player->mo->y,player->mo->z, redteam ? MT_BLUEFLAG : MT_REDFLAG);
	player->gotflag &= ~(redteam ? GF_BLUEFLAG : GF_REDFLAG);
	mo->flags &= ~MF_SPECIAL;
	mo->fuse = TICRATE;
	mo->spawnpoint = redteam ? bflagpoint : rflagpoint;
	mo->flags2 |= MF2_JUSTATTACKED;
	if (redteam)
		redscore += 1;
	else
		bluescore += 1;
	P_AddPlayerScore(player, 250);
}

static void P_ProcessZoomTube(player_t *player, mtag_t sectag, boolean end)
{
	INT32 sequence;
	fixed_t speed;
	INT32 lineindex;
	mobj_t *waypoint = NULL;
	angle_t an;

	if (player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT && player->powers[pw_carry] == CR_ZOOMTUBE)
		return;

	if (player->powers[pw_ignorelatch] & (1<<15))
		return;


	lineindex = Tag_FindLineSpecial(3, sectag);

	if (lineindex == -1)
	{
		CONS_Debug(DBG_GAMELOGIC, "ERROR: Zoom tube missing line special #3.\n");
		return;
	}


	speed = abs(lines[lineindex].args[0])<<(FRACBITS-3);
	if (end)
		speed *= -1;
	sequence = abs(lines[lineindex].args[1]);

	if (speed == 0)
	{
		CONS_Debug(DBG_GAMELOGIC, "ERROR: Waypoint sequence %d at zero speed.\n", sequence);
		return;
	}

	waypoint = end ? P_GetLastWaypoint(sequence) : P_GetFirstWaypoint(sequence);

	if (!waypoint)
	{
		CONS_Debug(DBG_GAMELOGIC, "ERROR: %s WAYPOINT IN SEQUENCE %d NOT FOUND.\n", end ? "LAST" : "FIRST", sequence);
		return;
	}

	CONS_Debug(DBG_GAMELOGIC, "Waypoint %d found in sequence %d - speed = %d\n", waypoint->health, sequence, speed);

	an = R_PointToAngle2(player->mo->x, player->mo->y, waypoint->x, waypoint->y) - player->mo->angle;

	if (an > ANGLE_90 && an < ANGLE_270 && !(lines[lineindex].args[2]))
		return;  

	P_SetTarget(&player->mo->tracer, waypoint);
	player->powers[pw_carry] = CR_ZOOMTUBE;
	player->speed = speed;
	player->pflags |= PF_SPINNING;
	player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE|PF_GLIDING|PF_BOUNCING|PF_SLIDING|PF_CANCARRY);
	player->climbing = 0;

	if (player->mo->state-states != S_PLAY_ROLL)
	{
		P_SetMobjState(player->mo, S_PLAY_ROLL);
		S_StartSound(player->mo, sfx_spin);
	}
}

static void P_ProcessFinishLine(player_t *player)
{
	if ((gametyperules & (GTR_RACE|GTR_LIVES)) != GTR_RACE)
		return;

	if (player->exiting)
		return;

	if (player->starpostnum == numstarposts)  
	{
		player->laps++;

		if (player->powers[pw_carry] == CR_NIGHTSMODE)
			player->drillmeter += 48*20;

		if (player->laps >= (UINT8)cv_numlaps.value)
			CONS_Printf(M_GetText("%s has finished the race.\n"), player_names[player-players]);
		else if (player->laps == (UINT8)cv_numlaps.value-1)
			CONS_Printf(M_GetText("%s started the \205final lap\200!\n"), player_names[player-players]);
		else
			CONS_Printf(M_GetText("%s started lap %u\n"), player_names[player-players], (UINT32)player->laps+1);


		player->starpostscale = player->starpostangle = player->starposttime = player->starpostnum = 0;
		player->starpostx = player->starposty = player->starpostz = 0;
		P_ResetStarposts();


		S_StartSound(player->mo, sfx_strpst);
	}
	else if (player->starpostnum)
	{

		if (!player->tossdelay)
			S_StartSound(player->mo, sfx_lose);
		player->tossdelay = 3;
	}

	if (player->laps >= (unsigned)cv_numlaps.value)
	{
		if (P_IsLocalPlayer(player))
		{
			HU_SetCEchoFlags(0);
			HU_SetCEchoDuration(5);
			HU_DoCEcho("FINISHED!");
		}

		P_DoPlayerExit(player, true);
	}
}

static void P_ProcessRopeHang(player_t *player, mtag_t sectag)
{
	INT32 sequence;
	fixed_t speed;
	INT32 lineindex;
	mobj_t *waypointmid = NULL;
	mobj_t *waypointhigh = NULL;
	mobj_t *waypointlow = NULL;
	mobj_t *closest = NULL;
	vector3_t p, line[2], resulthigh, resultlow;

	if (player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT && player->powers[pw_carry] == CR_ROPEHANG)
		return;

	if (player->powers[pw_ignorelatch] & (1<<15))
		return;

	if (player->mo->momz > 0)
		return;

	if (player->cmd.buttons & BT_SPIN)
		return;

	if (!(player->pflags & PF_SLIDING) && player->mo->state == &states[player->mo->info->painstate])
		return;

	if (player->exiting)
		return;


	memset(&resultlow, 0x00, sizeof(resultlow));
	memset(&resulthigh, 0x00, sizeof(resulthigh));


	lineindex = Tag_FindLineSpecial(11, sectag);

	if (lineindex == -1)
	{
		CONS_Debug(DBG_GAMELOGIC, "ERROR: Rope hang missing line special #11.\n");
		return;
	}


	speed = abs(lines[lineindex].args[0]) << (FRACBITS - 3);
	sequence = abs(lines[lineindex].args[1]);







	waypointmid = P_GetClosestWaypoint(sequence, player->mo);

	if (!waypointmid)
	{
		CONS_Debug(DBG_GAMELOGIC, "ERROR: WAYPOINT(S) IN SEQUENCE %d NOT FOUND.\n", sequence);
		return;
	}

	waypointlow = P_GetPreviousWaypoint(waypointmid, true);
	waypointhigh = P_GetNextWaypoint(waypointmid, true);

	CONS_Debug(DBG_GAMELOGIC, "WaypointMid: %d; WaypointLow: %d; WaypointHigh: %d\n",
					waypointmid->health, waypointlow ? waypointlow->health : -1, waypointhigh ? waypointhigh->health : -1);





	p.x = player->mo->x;
	p.y = player->mo->y;
	p.z = player->mo->z;


	if (waypointlow)
	{
		line[0].x = waypointmid->x;
		line[0].y = waypointmid->y;
		line[0].z = waypointmid->z;
		line[1].x = waypointlow->x;
		line[1].y = waypointlow->y;
		line[1].z = waypointlow->z;

		P_ClosestPointOnLine3D(&p, line, &resultlow);
	}


	if (waypointhigh)
	{
		line[0].x = waypointmid->x;
		line[0].y = waypointmid->y;
		line[0].z = waypointmid->z;
		line[1].x = waypointhigh->x;
		line[1].y = waypointhigh->y;
		line[1].z = waypointhigh->z;

		P_ClosestPointOnLine3D(&p, line, &resulthigh);
	}



	P_UnsetThingPosition(player->mo);
	P_ResetPlayer(player);
	player->mo->momx = player->mo->momy = player->mo->momz = 0;

	if (lines[lineindex].args[2])  
	{
		mobj_t *highest = P_GetLastWaypoint(sequence);
		highest->flags |= MF_SLIDEME;
	}


	if ((lines[lineindex].args[2]) && waypointmid->health == 0)
	{
		closest = waypointhigh;
		player->mo->x = resulthigh.x;
		player->mo->y = resulthigh.y;
		player->mo->z = resulthigh.z - P_GetPlayerHeight(player);
	}
	else if ((lines[lineindex].args[2]) && waypointmid->health == numwaypoints[sequence] - 1)
	{
		closest = waypointmid;
		player->mo->x = resultlow.x;
		player->mo->y = resultlow.y;
		player->mo->z = resultlow.z - P_GetPlayerHeight(player);
	}
	else
	{
		if (P_AproxDistance(P_AproxDistance(player->mo->x-resultlow.x, player->mo->y-resultlow.y),
				player->mo->z-resultlow.z) < P_AproxDistance(P_AproxDistance(player->mo->x-resulthigh.x,
					player->mo->y-resulthigh.y), player->mo->z-resulthigh.z))
		{

			closest = waypointmid;
			player->mo->x = resultlow.x;
			player->mo->y = resultlow.y;
			player->mo->z = resultlow.z - P_GetPlayerHeight(player);
		}
		else
		{

			closest = waypointhigh;
			player->mo->x = resulthigh.x;
			player->mo->y = resulthigh.y;
			player->mo->z = resulthigh.z - P_GetPlayerHeight(player);
		}
	}

	P_SetTarget(&player->mo->tracer, closest);
	player->powers[pw_carry] = CR_ROPEHANG;
	player->speed = speed;

	S_StartSound(player->mo, sfx_s3k4a);

	player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE|PF_GLIDING|PF_BOUNCING|PF_SLIDING|PF_CANCARRY);
	player->climbing = 0;
	P_SetThingPosition(player->mo);
	P_SetMobjState(player->mo, S_PLAY_RIDE);
}

static boolean P_SectorHasSpecial(sector_t *sec)
{
	if (sec->specialflags)
		return true;

	if (sec->damagetype != SD_NONE)
		return true;

	if (sec->triggertag)
		return true;

	if (sec->special)
		return true;

	return false;
}

static void P_EvaluateSpecialFlags(player_t *player, sector_t *sector, sector_t *roversector, boolean isTouching)
{
	mtag_t sectag = Tag_FGet(&sector->tags);

	if (sector->specialflags & SSF_OUTERSPACE)
	{
		if (!(player->powers[pw_shield] & SH_PROTECTWATER) && !player->powers[pw_spacetime])
			player->powers[pw_spacetime] = spacetimetics + 1;
	}
	if (sector->specialflags & SSF_WINDCURRENT)
		player->onconveyor = 2;
	if (sector->specialflags & SSF_CONVEYOR)
		player->onconveyor = 4;
	if ((sector->specialflags & SSF_SPEEDPAD) && isTouching)
		P_ProcessSpeedPad(player, sector, roversector, sectag);
	if (sector->specialflags & SSF_STARPOSTACTIVATOR)
	{
		mobj_t *post = P_GetObjectTypeInSectorNum(MT_STARPOST, sector - sectors);
		if (post)
			P_TouchStarPost(post, player, false);
	}
	if (sector->specialflags & SSF_EXIT)
		P_ProcessExitSector(player, sectag);
	if ((sector->specialflags & SSF_SPECIALSTAGEPIT) && isTouching)
		P_ProcessSpecialStagePit(player);
	if ((sector->specialflags & SSF_REDTEAMBASE) && isTouching)
		P_ProcessTeamBase(player, true);
	if ((sector->specialflags & SSF_BLUETEAMBASE) && isTouching)
		P_ProcessTeamBase(player, false);
	if (sector->specialflags & SSF_FAN)
	{
		player->mo->momz += mobjinfo[MT_FAN].mass/4;

		if (player->mo->momz > mobjinfo[MT_FAN].mass)
			player->mo->momz = mobjinfo[MT_FAN].mass;

		if (!player->powers[pw_carry])
		{
			P_ResetPlayer(player);
			P_SetMobjState(player->mo, S_PLAY_FALL);
			P_SetTarget(&player->mo->tracer, player->mo);
			player->powers[pw_carry] = CR_FAN;
		}
	}
	if (sector->specialflags & SSF_SUPERTRANSFORM)
	{
		if (player->mo->health > 0 && !player->bot && (player->charflags & SF_SUPER) && !player->powers[pw_super] && ALL7EMERALDS(emeralds))
			P_DoSuperTransformation(player, true);
	}
	if ((sector->specialflags & SSF_FORCESPIN) && isTouching)
	{
		if (!(player->pflags & PF_SPINNING))
		{
			player->pflags |= PF_SPINNING;
			P_SetMobjState(player->mo, S_PLAY_ROLL);
			S_StartAttackSound(player->mo, sfx_spin);

			if (abs(player->rmomx) < FixedMul(5*FRACUNIT, player->mo->scale)
			&& abs(player->rmomy) < FixedMul(5*FRACUNIT, player->mo->scale))
				P_InstaThrust(player->mo, player->mo->angle, FixedMul(10*FRACUNIT, player->mo->scale));
		}
	}
	if (sector->specialflags & SSF_ZOOMTUBESTART)
		P_ProcessZoomTube(player, sectag, false);
	if (sector->specialflags & SSF_ZOOMTUBEEND)
		P_ProcessZoomTube(player, sectag, true);
	if (sector->specialflags & SSF_FINISHLINE)
		P_ProcessFinishLine(player);
	if ((sector->specialflags & SSF_ROPEHANG) && isTouching)
		P_ProcessRopeHang(player, sectag);
}

static void P_EvaluateDamageType(player_t *player, sector_t *sector, boolean isTouching)
{
	switch (sector->damagetype)
	{
		case SD_GENERIC:
			if (isTouching)
				P_DamageMobj(player->mo, NULL, NULL, 1, 0);
			break;
		case SD_WATER:
			if (isTouching && (player->powers[pw_underwater] || player->powers[pw_carry] == CR_NIGHTSMODE))
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_WATER);
			break;
		case SD_FIRE:
		case SD_LAVA:
			if (isTouching)
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_FIRE);
			break;
		case SD_ELECTRIC:
			if (isTouching)
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_ELECTRIC);
			break;
		case SD_SPIKE:
			if (isTouching)
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_SPIKE);
			break;
		case SD_DEATHPITTILT:
		case SD_DEATHPITNOTILT:
			if (!isTouching)
				break;
			if (player->quittime)
				G_MovePlayerToSpawnOrStarpost(player - players);
			else
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_DEATHPIT);
			break;
		case SD_INSTAKILL:
			if (player->quittime)
				G_MovePlayerToSpawnOrStarpost(player - players);
			else
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_INSTAKILL);
			break;
		case SD_SPECIALSTAGE:
			if (!isTouching)
				break;

			if (player->exiting || player->bot)  
				break;

			if (!(player->powers[pw_shield] || player->spheres > 0))  
				break;

			P_SpecialStageDamage(player, NULL, NULL);
			break;
		default:
			break;
	}
}

static void P_EvaluateLinedefExecutorTrigger(player_t *player, sector_t *sector, boolean isTouching)
{
	if (player->bot)
		return;

	if (!sector->triggertag)
		return;

	if (sector->triggerer == TO_MOBJ)
		return;
	else if (sector->triggerer == TO_ALLPLAYERS && !P_DoAllPlayersTrigger(sector->triggertag))
		return;

	if ((sector->flags & MSF_TRIGGERLINE_PLANE) && !isTouching)
		return;

	P_LinedefExecute(sector->triggertag, player->mo, sector);
}

static void P_EvaluateOldSectorSpecial(player_t *player, sector_t *sector, sector_t *roversector, boolean isTouching)
{
	switch (GETSECSPECIAL(sector->special, 1))
	{
		case 9:  
			if (!isTouching)
				break;
			 
		case 10:  
			if (leveltime % (TICRATE/2) == 0 && player->rings > 0)
			{
				player->rings--;
				S_StartSound(player->mo, sfx_antiri);
			}
			break;
	}

	switch (GETSECSPECIAL(sector->special, 2))
	{
		case 9:  
			if (roversector)
				P_ProcessEggCapsule(player, sector);
			break;
	}
}

                                         
   
                                             
                                                
                                                                              
                                                                    
                                                         
    
void P_ProcessSpecialSector(player_t *player, sector_t *sector, sector_t *roversector)
{
	boolean isTouching;

	if (!P_SectorHasSpecial(sector))
		return;


	if (player->spectator)
		return;




	if (player->playerstate != PST_LIVE)
		return;

	isTouching = roversector || P_IsMobjTouchingSectorPlane(player->mo, sector);

	P_EvaluateSpecialFlags(player, sector, roversector, isTouching);
	P_EvaluateDamageType(player, sector, isTouching);
	P_EvaluateLinedefExecutorTrigger(player, sector, isTouching);

	if (!udmf)
		P_EvaluateOldSectorSpecial(player, sector, roversector, isTouching);
}

#define TELEPORTED(mo) (mo->subsector->sector != originalsector)

                                                                              
                         
   
                                  
                                                        
    
static void P_PlayerOnSpecial3DFloor(player_t *player, sector_t *sector)
{
	sector_t *originalsector = player->mo->subsector->sector;
	ffloor_t *rover;

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if (!P_SectorHasSpecial(rover->master->frontsector))
			continue;

		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!P_IsMobjTouching3DFloor(player->mo, rover, sector))
			continue;


		if (sector == player->mo->subsector->sector
			|| (rover->master->frontsector->flags & MSF_TRIGGERSPECIAL_TOUCH))
		{
			P_ProcessSpecialSector(player, rover->master->frontsector, sector);
			if TELEPORTED(player->mo) return;
		}
	}
}

static void P_PlayerOnSpecialPolyobj(player_t *player)
{
	sector_t *originalsector = player->mo->subsector->sector;
	polyobj_t *po;
	sector_t *polysec;
	boolean touching = false;
	boolean inside = false;

	for (po = player->mo->subsector->polyList; po; po = (polyobj_t *)(po->link.next))
	{
		if (po->flags & POF_NOSPECIALS)
			continue;

		polysec = po->lines[0]->backsector;

		if (!P_SectorHasSpecial(polysec))
			continue;

		touching = (polysec->flags & MSF_TRIGGERSPECIAL_TOUCH) && P_MobjTouchingPolyobj(po, player->mo);
		inside = P_MobjInsidePolyobj(po, player->mo);

		if (!(inside || touching))
			continue;

		if (!P_IsMobjTouchingPolyobj(player->mo, po, polysec))
			continue;

		P_ProcessSpecialSector(player, polysec, originalsector);
		if TELEPORTED(player->mo) return;
	}
}

                                                                              
   
                                  
                                                        
    
void P_PlayerInSpecialSector(player_t *player)
{
	sector_t *originalsector;
	sector_t *loopsector;
	msecnode_t *node;

	if (!player->mo)
		return;

	originalsector = player->mo->subsector->sector;

	P_PlayerOnSpecial3DFloor(player, originalsector);  
	if TELEPORTED(player->mo) return;


	P_PlayerOnSpecialPolyobj(player);
	if TELEPORTED(player->mo) return;

	P_ProcessSpecialSector(player, originalsector, NULL);
	if TELEPORTED(player->mo) return;


	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		loopsector = node->m_sector;

		if (loopsector == originalsector)  
			continue;


		P_PlayerOnSpecial3DFloor(player, loopsector);
		if TELEPORTED(player->mo) return;

		if (!(loopsector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			continue;

		P_ProcessSpecialSector(player, loopsector, NULL);
		if TELEPORTED(player->mo) return;
	}
}

static void P_CheckMobj3DFloorTrigger(mobj_t *mo, sector_t *sec)
{
	sector_t *originalsector = mo->subsector->sector;
	ffloor_t *rover;

	for (rover = sec->ffloors; rover; rover = rover->next)
	{
		if (!rover->master->frontsector->triggertag)
			continue;

		if (rover->master->frontsector->triggerer != TO_MOBJ)
			continue;

		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!P_IsMobjTouching3DFloor(mo, rover, sec))
			continue;

		P_LinedefExecute(rover->master->frontsector->triggertag, mo, rover->master->frontsector);
		if TELEPORTED(mo) return;
	}
}

static void P_CheckMobjPolyobjTrigger(mobj_t *mo)
{
	sector_t *originalsector = mo->subsector->sector;
	polyobj_t *po;
	sector_t *polysec;
	boolean touching = false;
	boolean inside = false;

	for (po = mo->subsector->polyList; po; po = (polyobj_t *)(po->link.next))
	{
		if (po->flags & POF_NOSPECIALS)
			continue;

		polysec = po->lines[0]->backsector;

		if (!polysec->triggertag)
			continue;

		if (polysec->triggerer != TO_MOBJ)
			continue;

		touching = (polysec->flags & MSF_TRIGGERSPECIAL_TOUCH) && P_MobjTouchingPolyobj(po, mo);
		inside = P_MobjInsidePolyobj(po, mo);

		if (!(inside || touching))
			continue;

		if (!P_IsMobjTouchingPolyobj(mo, po, polysec))
			continue;

		P_LinedefExecute(polysec->triggertag, mo, polysec);
		if TELEPORTED(mo) return;
	}
}

static void P_CheckMobjSectorTrigger(mobj_t *mo, sector_t *sec)
{
	if (!sec->triggertag)
		return;

	if (sec->triggerer != TO_MOBJ)
		return;

	if ((sec->flags & MSF_TRIGGERLINE_PLANE) && !P_IsMobjTouchingSectorPlane(mo, sec))
		return;

	P_LinedefExecute(sec->triggertag, mo, sec);
}

void P_CheckMobjTrigger(mobj_t *mobj, boolean pushable)
{
	sector_t *originalsector;

	if (!mobj->subsector)
		return;

	originalsector = mobj->subsector->sector;

	if (!pushable && !(originalsector->flags & MSF_TRIGGERLINE_MOBJ))
		return;

	P_CheckMobj3DFloorTrigger(mobj, originalsector);
	if TELEPORTED(mobj)	return;

	P_CheckMobjPolyobjTrigger(mobj);
	if TELEPORTED(mobj)	return;

	P_CheckMobjSectorTrigger(mobj, originalsector);
}

#undef TELEPORTED

                                                                                                  
   
                                           
    
void P_UpdateSpecials(void)
{

	P_CheckTimeLimit();


	P_CheckPointLimit();


	for (anim_t *anim = anims; anim < lastanim; anim++)
	{
		for (INT32 i = 0; i < anim->numpics; i++)
		{
			INT32 pic = anim->basepic + ((leveltime/anim->speed + i) % anim->numpics);
			texturetranslation[anim->basepic+i] = pic;
		}
	}
}





                                                          
   
                                              
                                                                                                              
                       
    
UINT16 P_GetFFloorID(ffloor_t *fflr)
{
	ffloor_t *rover;
	sector_t *sec;
	UINT16 i = 0;

	if (!fflr)
		return UINT16_MAX;

	sec = fflr->target;

	if (!sec->ffloors)
		return UINT16_MAX;
	for (rover = sec->ffloors; rover; rover = rover->next, i++)
		if (rover == fflr)
			return i;
	return UINT16_MAX;
}

                                     
   
                              
                               
                                              
                       
    
static inline ffloor_t *P_GetFFloorBySec(sector_t *sec, sector_t *sec2)
{
	ffloor_t *rover;

	if (!sec->ffloors)
		return NULL;
	for (rover = sec->ffloors; rover; rover = rover->next)
		if (rover->secnum == (size_t)(sec2 - sectors))
			return rover;
	return NULL;
}

                                
   
                             
                                                                                 
                                              
                                       
    
ffloor_t *P_GetFFloorByID(sector_t *sec, UINT16 id)
{
	ffloor_t *rover;
	UINT16 i = 0;

	if (!sec->ffloors)
		return NULL;
	for (rover = sec->ffloors; rover; rover = rover->next)
		if (i++ == id)
			return rover;
	return NULL;
}

                                                                     
   
                                
                                                 
                      
    
static inline void P_AddFFloorToList(sector_t *sec, ffloor_t *fflr)
{
	ffloor_t *rover;

	if (!sec->ffloors)
	{
		sec->ffloors = fflr;
		fflr->next = 0;
		fflr->prev = 0;
		return;
	}

	for (rover = sec->ffloors; rover->next; rover = rover->next);

	rover->next = fflr;
	fflr->prev = rover;
	fflr->next = 0;
}

                   
   
                                     
                                      
                                       
                                           
                                     
                                                      
                                                                               
                                       
                                                           
    
static ffloor_t *P_AddFakeFloor(sector_t *sec, sector_t *sec2, line_t *master, INT32 alpha, UINT8 blendmode, ffloortype_e flags, thinkerlist_t *secthinkers)
{
	ffloor_t *fflr;
	thinker_t *th;
	friction_t *f;
	pusher_t *p;
	size_t sec2num;
	size_t i;

	if (sec == sec2)
		return NULL;  
	if ((fflr = (P_GetFFloorBySec(sec, sec2))))
		return fflr;  

	if (sec2->ceilingheight < sec2->floorheight)
	{
		fixed_t tempceiling = sec2->ceilingheight;

		CONS_Alert(CONS_ERROR, M_GetText("A FOF tagged %d has a top height below its bottom.\n"), master->args[0]);
		sec2->ceilingheight = sec2->floorheight;
		sec2->floorheight = tempceiling;
	}

	if (sec2->numattached == 0)
	{
		sec2->attached = Z_Malloc(sizeof (*sec2->attached) * sec2->maxattached, PU_STATIC, NULL);
		sec2->attachedsolid = Z_Malloc(sizeof (*sec2->attachedsolid) * sec2->maxattached, PU_STATIC, NULL);
		sec2->attached[0] = sec - sectors;
		sec2->numattached = 1;
		sec2->attachedsolid[0] = (flags & FOF_SOLID);
	}
	else
	{
		for (i = 0; i < sec2->numattached; i++)
			if (sec2->attached[i] == (size_t)(sec - sectors))
				return NULL;

		if (sec2->numattached >= sec2->maxattached)
		{
			sec2->maxattached *= 2;
			sec2->attached = Z_Realloc(sec2->attached, sizeof (*sec2->attached) * sec2->maxattached, PU_STATIC, NULL);
			sec2->attachedsolid = Z_Realloc(sec2->attachedsolid, sizeof (*sec2->attachedsolid) * sec2->maxattached, PU_STATIC, NULL);
		}
		sec2->attached[sec2->numattached] = sec - sectors;
		sec2->attachedsolid[sec2->numattached] = (flags & FOF_SOLID);
		sec2->numattached++;
	}


	fflr = Z_Calloc(sizeof (*fflr), PU_LEVEL, NULL);
	fflr->secnum = sec2 - sectors;
	fflr->target = sec;
	fflr->bottomheight = &sec2->floorheight;
	fflr->bottompic = &sec2->floorpic;
	fflr->bottomxoffs = &sec2->floorxoffset;
	fflr->bottomyoffs = &sec2->flooryoffset;
	fflr->bottomxscale = &sec2->floorxscale;
	fflr->bottomyscale = &sec2->flooryscale;
	fflr->bottomangle = &sec2->floorangle;


	fflr->topheight = &sec2->ceilingheight;
	fflr->toppic = &sec2->ceilingpic;
	fflr->toplightlevel = &sec2->lightlevel;
	fflr->topxoffs = &sec2->ceilingxoffset;
	fflr->topyoffs = &sec2->ceilingyoffset;
	fflr->topxscale = &sec2->ceilingxscale;
	fflr->topyscale = &sec2->ceilingyscale;
	fflr->topangle = &sec2->ceilingangle;


	fflr->t_slope = &sec2->c_slope;
	fflr->b_slope = &sec2->f_slope;


	if (sec2->hasslope)
		sec->hasslope = true;

	fflr->spawnflags = fflr->fofflags = flags;
	fflr->master = master;
	fflr->norender = INFTICS;
	fflr->fadingdata = NULL;





	sec2num = sec2 - sectors;


	i = 0;
	th = thlist[THINK_MAIN].next;

	for(;;)
	{
		if(secthinkers)
		{
			if(i < secthinkers[sec2num].count)
				th = secthinkers[sec2num].thinkers[i];
			else break;
		}
		else if (th == &thlist[THINK_MAIN])
			break;


		if(th->function.acp1 == (actionf_p1)T_Friction)
		{
			f = (friction_t *)th;

			if (f->affectee == (INT32)sec2num)
				Add_Friction(f->friction, f->movefactor, (INT32)(sec-sectors), f->affectee);
		}

		else if(th->function.acp1 == (actionf_p1)T_Pusher)
		{
			p = (pusher_t *)th;

			if (p->affectee == (INT32)sec2num)
				Add_Pusher(p->type, p->x_mag, p->y_mag, p->z_mag, (INT32)(sec-sectors), p->affectee, p->exclusive, p->slider);
		}

		if(secthinkers) i++;
		else th = th->next;
	}

	fflr->alpha = max(0, min(0xff, alpha));
	if (fflr->alpha < 0xff || flags & FOF_SPLAT)
	{
		fflr->fofflags |= FOF_TRANSLUCENT;
		fflr->spawnflags = fflr->fofflags;
	}
	fflr->spawnalpha = fflr->alpha;  

	switch (blendmode)
	{
		case TMB_TRANSLUCENT:
		default:
			fflr->blend = AST_COPY;
			break;
		case TMB_ADD:
			fflr->blend = AST_ADD;
			break;
		case TMB_SUBTRACT:
			fflr->blend = AST_SUBTRACT;
			break;
		case TMB_REVERSESUBTRACT:
			fflr->blend = AST_REVERSESUBTRACT;
			break;
		case TMB_MODULATE:
			fflr->blend = AST_MODULATE;
			break;
	}

	if (flags & FOF_QUICKSAND)
		CheckForQuicksand = true;

	if (flags & FOF_BUSTUP)
		CheckForBustableBlocks = true;

	if ((flags & FOF_MARIO))
	{
		if (!(flags & FOF_GOOWATER))  
			P_AddBlockThinker(sec2, master);
		CheckForMarioBlocks = true;
	}

	if ((flags & FOF_CRUMBLE))
		sec2->crumblestate = CRUMBLE_WAIT;

	if ((flags & FOF_FLOATBOB))
	{
		P_AddFloatThinker(sec2, master->args[0], master);
		CheckForFloatBob = true;
	}

	P_AddFFloorToList(sec, fflr);

	return fflr;
}





                         
                                                          
   
                                       
                                      
                                      
                                              
    
static void P_AddFloatThinker(sector_t *sec, UINT16 tag, line_t *sourceline)
{
	floatthink_t *floater;


	floater = Z_Calloc(sizeof (*floater), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &floater->thinker);

	floater->thinker.function.acp1 = (actionf_p1)T_FloatSector;

	floater->sector = sec;
	floater->tag = (INT16)tag;
	floater->sourceline = sourceline;


	R_CreateInterpolator_SectorPlane(&floater->thinker, sec, false);
	R_CreateInterpolator_SectorPlane(&floater->thinker, sec, true);
}

   
                                      
                                        
                                                             
   
                                                                       
                                           
                                          
                                              
                                        
                          
    
static void P_AddPlaneDisplaceThinker(INT32 type, fixed_t speed, INT32 control, INT32 affectee, UINT8 reverse)
{
	planedisplace_t *displace;


	displace = Z_Calloc(sizeof (*displace), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &displace->thinker);

	displace->thinker.function.acp1 = (actionf_p1)T_PlaneDisplace;
	displace->affectee = affectee;
	displace->control = control;
	displace->last_height = sectors[control].floorheight;
	displace->speed = speed;
	displace->type = type;
	displace->reverse = reverse;


	R_CreateInterpolator_SectorPlane(&displace->thinker, &sectors[affectee], false);
}

                                                                               
                                               
                                          
   
                                       
                                      
                                        
                                            
                                              
    
static void P_AddBlockThinker(sector_t *sec, line_t *sourceline)
{
	mariocheck_t *block;


	block = Z_Calloc(sizeof (*block), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &block->thinker);

	block->thinker.function.acp1 = (actionf_p1)T_MarioBlockChecker;
	block->sourceline = sourceline;

	block->sector = sec;
}

                         
                                        
                                       
                                          
                                          
                                      
                  
   
                                       
                                      
                                              
    
static void P_AddRaiseThinker(sector_t *sec, INT16 tag, fixed_t speed, fixed_t ceilingtop, fixed_t ceilingbottom, boolean lower, boolean spindash)
{
	raise_t *raise;

	raise = Z_Calloc(sizeof (*raise), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &raise->thinker);

	raise->thinker.function.acp1 = (actionf_p1)T_RaiseSector;

	raise->tag = tag;
	raise->sector = sec;

	raise->ceilingtop = ceilingtop;
	raise->ceilingbottom = ceilingbottom;

	raise->basespeed = speed >> 2;

	if (lower)
		raise->flags |= RF_REVERSE;
	if (spindash)
		raise->flags |= RF_SPINDASH;


	R_CreateInterpolator_SectorPlane(&raise->thinker, sec, false);
	R_CreateInterpolator_SectorPlane(&raise->thinker, sec, true);
}

static void P_AddAirbob(sector_t *sec, INT16 tag, fixed_t dist, boolean raise, boolean spindash, boolean dynamic)
{
	raise_t *airbob;

	airbob = Z_Calloc(sizeof (*airbob), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &airbob->thinker);

	airbob->thinker.function.acp1 = (actionf_p1)T_RaiseSector;

	airbob->tag = tag;
	airbob->sector = sec;

	airbob->ceilingtop = sec->ceilingheight;
	airbob->ceilingbottom = sec->ceilingheight - dist;

	airbob->basespeed = FRACUNIT;

	if (!raise)
		airbob->flags |= RF_REVERSE;
	if (spindash)
		airbob->flags |= RF_SPINDASH;
	if (dynamic)
		airbob->flags |= RF_DYNAMIC;


	R_CreateInterpolator_SectorPlane(&airbob->thinker, sec, false);
	R_CreateInterpolator_SectorPlane(&airbob->thinker, sec, true);
}

                          
                               
   
                                       
                                       
                                              
    
static inline void P_AddThwompThinker(sector_t *sec, line_t *sourceline, fixed_t crushspeed, fixed_t retractspeed, UINT16 sound)
{
	thwomp_t *thwomp;



	if (sec->floordata || sec->ceilingdata)
		return;


	thwomp = Z_Calloc(sizeof (*thwomp), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &thwomp->thinker);

	thwomp->thinker.function.acp1 = (actionf_p1)T_ThwompSector;


	thwomp->sourceline = sourceline;
	thwomp->sector = sec;
	thwomp->crushspeed = crushspeed;
	thwomp->retractspeed = retractspeed;
	thwomp->direction = 0;
	thwomp->floorstartheight = sec->floorheight;
	thwomp->ceilingstartheight = sec->ceilingheight;
	thwomp->delay = 1;
	thwomp->tag = sourceline->args[0];
	thwomp->sound = sound;

	sec->floordata = thwomp;
	sec->ceilingdata = thwomp;

	sides[sourceline->sidenum[0]].midtexture = sides[sourceline->sidenum[0]].bottomtexture;


	R_CreateInterpolator_SectorPlane(&thwomp->thinker, sec, false);
	R_CreateInterpolator_SectorPlane(&thwomp->thinker, sec, true);
}

                                                                                            
                                           
   
                                        
                                          
                                              
    
static inline void P_AddNoEnemiesThinker(line_t *sourceline)
{
	noenemies_t *nobaddies;


	nobaddies = Z_Calloc(sizeof (*nobaddies), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &nobaddies->thinker);

	nobaddies->thinker.function.acp1 = (actionf_p1)T_NoEnemiesSector;

	nobaddies->sourceline = sourceline;
}

                                                                             
                                                                                 
   
                                        
                                          
                                              
    
static void P_AddEachTimeThinker(line_t *sourceline, boolean triggerOnExit)
{
	eachtime_t *eachtime;


	eachtime = Z_Calloc(sizeof (*eachtime), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &eachtime->thinker);

	eachtime->thinker.function.acp1 = (actionf_p1)T_EachTimeThinker;

	eachtime->sourceline = sourceline;
	eachtime->triggerOnExit = triggerOnExit;
}

                          
   
                                       
                                      
                                                 
                                        
                                              
    
static inline void P_AddCameraScanner(sector_t *sourcesec, sector_t *actionsector, angle_t angle)
{
	elevator_t *elevator;  


	elevator = Z_Calloc(sizeof (*elevator), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &elevator->thinker);

	elevator->thinker.function.acp1 = (actionf_p1)T_CameraScanner;
	elevator->type = elevateBounce;


	elevator->sector = sourcesec;
	elevator->actionsector = actionsector;
	elevator->distance = FixedInt(AngleFixed(angle));
}

                          
   
                                                  
                         
                                              
    
void T_LaserFlash(laserthink_t *flash)
{
	msecnode_t *node;
	mobj_t *thing;
	INT32 s;
	ffloor_t *fflr;
	sector_t *sector;
	sector_t *sourcesec = flash->sourceline->frontsector;
	fixed_t top, bottom;

	TAG_ITER_SECTORS(flash->tag, s)
	{
		sector = &sectors[s];
		for (fflr = sector->ffloors; fflr; fflr = fflr->next)
		{
			if (fflr->master != flash->sourceline)
				continue;

			if (!(fflr->fofflags & FOF_EXISTS))
				break;

			if (leveltime & 2)

				fflr->alpha = 0xB0;
			else

				fflr->alpha = 0x90;

			top    = P_GetFFloorTopZAt   (fflr, sector->soundorg.x, sector->soundorg.y);
			bottom = P_GetFFloorBottomZAt(fflr, sector->soundorg.x, sector->soundorg.y);
			sector->soundorg.z = (top + bottom)/2;
			S_StartSound(&sector->soundorg, sfx_laser);


			for (node = sector->touching_thinglist; node && node->m_thing; node = node->m_thinglist_next)
			{
				thing = node->m_thing;

				if (flash->nobosses && thing->flags & MF_BOSS)
					continue;  


				if (thing->health <= 0)
					continue;

				top = P_GetSpecialTopZ(thing, sourcesec, sector);
				bottom = P_GetSpecialBottomZ(thing, sourcesec, sector);

				if (thing->z >= top
				|| thing->z + thing->height <= bottom)
					continue;

				if (thing->flags & MF_SHOOTABLE)
					P_DamageMobj(thing, NULL, NULL, 1, 0);
				else if (thing->type == MT_EGGSHIELD)
					P_KillMobj(thing, NULL, NULL, 0);
			}

			break;
		}
	}
}

static inline void P_AddLaserThinker(INT16 tag, line_t *line, boolean nobosses)
{
	laserthink_t *flash = Z_Calloc(sizeof (*flash), PU_LEVSPEC, NULL);

	P_AddThinker(THINK_MAIN, &flash->thinker);

	flash->thinker.function.acp1 = (actionf_p1)T_LaserFlash;
	flash->tag = tag;
	flash->sourceline = line;
	flash->nobosses = nobosses;
}







static void P_RunLevelLoadExecutors(void)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special == 399)
			P_RunTriggerLinedef(&lines[i], NULL, NULL);
	}
}

                                                                              
                                                             
                                               
   
                       
                          
    
void P_InitSpecials(void)
{

	gravity = mapheaderinfo[gamemap-1]->gravity;


	sstimer = mapheaderinfo[gamemap-1]->sstimer*TICRATE + 6;
	ssspheres = mapheaderinfo[gamemap-1]->ssspheres;

	CheckForBustableBlocks = CheckForBouncySector = CheckForQuicksand = CheckForMarioBlocks = CheckForFloatBob = CheckForReverseGravity = false;


	switch (mapheaderinfo[gamemap-1]->weather)
	{
		case PRECIP_SNOW:  
		case PRECIP_RAIN:  
		case PRECIP_STORM:  
		case PRECIP_STORM_NORAIN:  
		case PRECIP_STORM_NOSTRIKES:  
			curWeather = mapheaderinfo[gamemap-1]->weather;
			break;
		default:  
			curWeather = PRECIP_NONE;
			break;
	}


	globalweather = mapheaderinfo[gamemap-1]->weather;
}

void P_ApplyFlatAlignment(sector_t *sector, angle_t flatangle, fixed_t xoffs, fixed_t yoffs, boolean floor, boolean ceiling)
{
	if (floor)
	{
		sector->floorangle = flatangle;
		sector->floorxoffset += xoffs;
		sector->flooryoffset += yoffs;
	}

	if (ceiling)
	{
		sector->ceilingangle = flatangle;
		sector->ceilingxoffset += xoffs;
		sector->ceilingyoffset += yoffs;
	}

}

static void P_MakeFOFBouncy(line_t *paramline, line_t *masterline)
{
	INT32 s;

	if (masterline->special < 100 || masterline->special >= 300)
		return;

	TAG_ITER_SECTORS(masterline->args[0], s)
	{
		ffloor_t *rover;

		for (rover = sectors[s].ffloors; rover; rover = rover->next)
		{
			if (rover->master != masterline)
				continue;

			rover->fofflags |= FOF_BOUNCY;
			rover->spawnflags |= FOF_BOUNCY;
			rover->bouncestrength = (paramline->args[1]<< FRACBITS)/100;
			CheckForBouncySector = true;
			break;
		}
	}

}

static boolean P_CheckGametypeRules(INT32 checktype, UINT32 target)
{
	switch (checktype)
	{
		case TMF_HASALL:
		default:
			return (gametyperules & target) == target;
		case TMF_HASANY:
			return !!(gametyperules & target);
		case TMF_HASEXACTLY:
			return gametyperules == target;
		case TMF_DOESNTHAVEALL:
			return (gametyperules & target) != target;
		case TMF_DOESNTHAVEANY:
			return !(gametyperules & target);
	}
}

fixed_t P_GetSectorGravityFactor(sector_t *sec)
{
	if (sec->gravityptr)
		return FixedDiv(*sec->gravityptr >> FRACBITS, 1000);
	else
		return sec->gravity;
}

void P_InitSectorPortals(void)
{
	secportalcount = 0;
	secportalcapacity = 0;
	secportals = NULL;
}

UINT32 P_NewSectorPortal(void)
{
	size_t i = secportalcount++;
	if (i == UINT32_MAX)
		I_Error("Too many sector portals");

	if (secportalcapacity == 0 || secportalcount == secportalcapacity)
	{
		secportalcapacity = secportalcapacity ? (secportalcapacity * 2) : 16;
		secportals = Z_Realloc(secportals, secportalcapacity * sizeof(sectorportal_t), PU_LEVEL, NULL);
	}

	secportals[i].type = SECPORTAL_NONE;

	return (UINT32)i;
}

boolean P_IsSectorPortalValid(sectorportal_t *secportal)
{
	if (secportal == NULL)
		return false;

	switch (secportal->type)
	{
	case SECPORTAL_LINE:
	case SECPORTAL_FLOOR:
	case SECPORTAL_CEILING:
		return true;
	case SECPORTAL_OBJECT:
		return secportal->mobj && !P_MobjWasRemoved(secportal->mobj);
	case SECPORTAL_SKYBOX:
		return skyboxmo[0] && !P_MobjWasRemoved(skyboxmo[0]);
	case SECPORTAL_PLANE:
	case SECPORTAL_HORIZON:
		return true;
	default:
		return false;
	}
}

boolean P_SectorHasPortal(sector_t *sector)
{
	return P_SectorHasFloorPortal(sector) || P_SectorHasCeilingPortal(sector);
}

sectorportal_t *P_SectorGetFloorPortal(sector_t *sector)
{
	UINT32 num = sector->portal_floor;
	if (num >= secportalcount)
		return NULL;

	return &secportals[num];
}

sectorportal_t *P_SectorGetCeilingPortal(sector_t *sector)
{
	UINT32 num = sector->portal_ceiling;
	if (num >= secportalcount)
		return NULL;

	return &secportals[num];
}

boolean P_SectorHasFloorPortal(sector_t *sector)
{
	return P_IsSectorPortalValid(P_SectorGetFloorPortal(sector));
}

boolean P_SectorHasCeilingPortal(sector_t *sector)
{
	return P_IsSectorPortalValid(P_SectorGetCeilingPortal(sector));
}

boolean P_CompareSectorPortals(sectorportal_t *a, sectorportal_t *b)
{
	if (a == NULL && b == NULL)
		return true;
	else if (!a || !b)
		return false;
	else if (a->type != b->type)
		return false;

	switch (a->type)
	{
	case SECPORTAL_LINE:
		return a->line.start == b->line.start && a->line.dest == b->line.dest;
	case SECPORTAL_FLOOR:
	case SECPORTAL_CEILING:
		return a->sector == b->sector;
	case SECPORTAL_OBJECT:
		return a->mobj == b->mobj;
	default:
		return true;
	}
}

static mobj_t *P_GetMobjByTag(INT32 tag)
{
	INT32 mtnum = -1;

	TAG_ITER_THINGS(tag, mtnum)
	{
		mobj_t *mo = mapthings[mtnum].mobj;
		if (mo)
			return mo;
	}

	return NULL;
}

static void P_DoPortalCopyFromLine(sector_t *dest_sector, int plane_type, int tag)
{
	INT32 secnum = -1;
	TAG_ITER_SECTORS(tag, secnum)
	{
		sector_t *src_sector = &sectors[secnum];
		if (plane_type == TMP_FLOOR || plane_type == TMP_BOTH)
			dest_sector->portal_floor = src_sector->portal_floor;
		if (plane_type == TMP_CEILING || plane_type == TMP_BOTH)
			dest_sector->portal_ceiling = src_sector->portal_ceiling;
	}
}

static sectorportal_t *P_SectorGetPortalOrCreate(sector_t *sector, UINT32 *num, UINT32 *result, boolean ceiling)
{
	sectorportal_t *secportal = NULL;

	if (*num >= secportalcount)
	{
		*num = P_NewSectorPortal();
		secportal = &secportals[*num];
		secportal->target = sector;
		secportal->ceiling = ceiling;
		*result = *num;
	}
	else
	{
		*result = *num;
		secportal = &secportals[*num];
	}

	return secportal;
}

static sectorportal_t *P_SectorGetFloorPortalOrCreate(sector_t *sector, UINT32 *result)
{
	return P_SectorGetPortalOrCreate(sector, &sector->portal_floor, result, false);
}

static sectorportal_t *P_SectorGetCeilingPortalOrCreate(sector_t *sector, UINT32 *result)
{
	return P_SectorGetPortalOrCreate(sector, &sector->portal_ceiling, result, true);
}

static void P_CopySectorPortalToLines(UINT32 portal_num, int sector_tag)
{
	for (size_t i = 0; i < numlines; i++)
	{
		if (lines[i].special != SPECIAL_SECTOR_SETPORTAL)
			continue;

		if (lines[i].args[1] != TMSECPORTAL_COPY_PORTAL_TO_LINE)
			continue;

		if (lines[i].args[3] != sector_tag)
			continue;

		if (lines[i].args[0] != 0)
		{
			INT32 linenum = -1;
			TAG_ITER_LINES(lines[i].args[0], linenum)
			{
				lines[linenum].secportal = portal_num;
			}
		}
		else
		{

			lines[i].secportal = portal_num;
		}
	}
}

                                                                        
             
   
                                           
                                           
                                                                                   
                                                         
                                                                               
    
void P_SpawnSpecials(boolean fromnetsave)
{
	sector_t *sector;
	size_t i;
	INT32 j;
	thinkerlist_t *secthinkers;
	thinker_t *th;


	(void)fromnetsave;


	bossdisabled = 0;
	stoppedclock = false;


	sector = sectors;
	for (i = 0; i < numsectors; i++, sector++)
	{
		CheckForReverseGravity |= (sector->flags & MSF_GRAVITYFLIP);

		if (sector->specialflags & SSF_FINISHLINE)
		{
			if ((gametyperules & (GTR_RACE|GTR_LIVES)) == GTR_RACE)
				circuitmap = true;
		}

		if (sector->damagetype == SD_SPIKE) {



			sector->flags |= MSF_TRIGGERSPECIAL_TOUCH;
		}



		if (udmf || !sector->special)
			continue;


		switch(GETSECSPECIAL(sector->special, 1))
		{
			case 15:  
				CheckForBouncySector = true;
				break;
		}


		switch(GETSECSPECIAL(sector->special, 2))
		{
			case 10:  
				sstimer = (sector->floorheight>>FRACBITS) * TICRATE + 6;  
				ssspheres = sector->ceilingheight>>FRACBITS;  
				break;

			case 11:  
				gravity = sector->floorheight/1000;
				break;
		}
	}

	P_SpawnScrollers();  
	P_SpawnFriction();   
	P_SpawnPushers();    



	secthinkers = Z_Calloc(numsectors * sizeof(thinkerlist_t), PU_STATIC, NULL);


	for (th = thlist[THINK_MAIN].next; th != &thlist[THINK_MAIN]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)T_Friction)
			secthinkers[((friction_t *)th)->affectee].count++;
		else if (th->function.acp1 == (actionf_p1)T_Pusher)
			secthinkers[((pusher_t *)th)->affectee].count++;
	}



	for (i = 0; i < numsectors; i++)
		if(secthinkers[i].count > 0)
		{
			secthinkers[i].thinkers = Z_Malloc(secthinkers[i].count * sizeof(thinker_t *), PU_STATIC, NULL);
			secthinkers[i].count = 0;
		}


	for (th = thlist[THINK_MAIN].next; th != &thlist[THINK_MAIN]; th = th->next)
	{
		size_t secnum = (size_t)-1;

		if (th->function.acp1 == (actionf_p1)T_Friction)
			secnum = ((friction_t *)th)->affectee;
		else if (th->function.acp1 == (actionf_p1)T_Pusher)
			secnum = ((pusher_t *)th)->affectee;

		if (secnum != (size_t)-1)
			secthinkers[secnum].thinkers[secthinkers[secnum].count++] = th;
	}



	for (i = 0; i < numlines; i++)
	{

		if (netgame || multiplayer)
		{
			if (lines[i].flags & ML_NONET)
			{
				lines[i].special = 0;
				continue;
			}
		}
		else if (lines[i].flags & ML_NETONLY)
		{
			lines[i].special = 0;
			continue;
		}

		switch (lines[i].special)
		{
			INT32 s;
			INT32 l;
			size_t sec;
			ffloortype_e ffloorflags;

			case 1:  
				if (udmf)
					break;

				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(Tag_FGet(&lines[i].tags), s)
				{
					sectors[s].gravityptr = &sectors[sec].floorheight;  

					if (lines[i].flags & ML_NOCLIMB)
						sectors[s].flags |= MSF_GRAVITYFLIP;
					else
						sectors[s].flags &= ~MSF_GRAVITYFLIP;

					if (lines[i].flags & ML_EFFECT6)
						sectors[s].specialflags |= SSF_GRAVITYOVERRIDE;

					CheckForReverseGravity |= (sectors[s].flags & MSF_GRAVITYFLIP);
				}
				break;

			case 5:  
				if (udmf)
					break;

				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(Tag_FGet(&lines[i].tags), s)
					P_AddCameraScanner(&sectors[sec], &sectors[s], R_PointToAngle2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y));
				break;

			case SPECIAL_SECTOR_SETPORTAL:  
			{
				int target_sector_tag = lines[i].args[0];
				int portal_type = lines[i].args[1];
				int plane_type = lines[i].args[2];
				int misc = lines[i].args[3];

				boolean floor, ceiling;
				if (plane_type == TMP_BOTH)
					floor = ceiling = true;
				else
				{
					floor = plane_type == TMP_FLOOR;
					ceiling = plane_type == TMP_CEILING;
				}

				UINT32 portal_num = UINT32_MAX;


				if (portal_type == TMSECPORTAL_PLANE || portal_type == TMSECPORTAL_HORIZON)
				{
					secportaltype_e type = portal_type == TMSECPORTAL_HORIZON ? SECPORTAL_HORIZON : SECPORTAL_PLANE;
					if (floor)
					{
						sectorportal_t *floorportal = P_SectorGetFloorPortalOrCreate(lines[i].frontsector, &portal_num);
						floorportal->type = type;
						floorportal->sector = lines[i].frontsector;
						P_CopySectorPortalToLines(portal_num, target_sector_tag);
					}
					if (ceiling)
					{
						sectorportal_t *ceilportal = P_SectorGetCeilingPortalOrCreate(lines[i].frontsector, &portal_num);
						ceilportal->type = type;
						ceilportal->sector = lines[i].frontsector;
						P_CopySectorPortalToLines(portal_num, target_sector_tag);
					}
					break;
				}

				INT32 s1 = -1;

				TAG_ITER_SECTORS(target_sector_tag, s1)
				{
					sector_t *target_sector = &sectors[s1];


					if (portal_type == TMSECPORTAL_NORMAL)
					{
						INT32 linenum = -1;
						TAG_ITER_LINES(misc, linenum)
						{
							if (lines[linenum].special == SPECIAL_SECTOR_SETPORTAL
							&& lines[linenum].args[0] == target_sector_tag
							&& lines[linenum].args[1] == portal_type
							&& lines[linenum].args[2] == plane_type
							&& lines[linenum].args[3] == 1)
							{
								if (floor)
								{
									sectorportal_t *floorportal = P_SectorGetFloorPortalOrCreate(target_sector, &portal_num);
									floorportal->type = SECPORTAL_LINE;
									floorportal->line.start = &lines[i];
									floorportal->line.dest = &lines[linenum];
									P_CopySectorPortalToLines(portal_num, target_sector_tag);
								}
								if (ceiling)
								{
									sectorportal_t *ceilportal = P_SectorGetCeilingPortalOrCreate(target_sector, &portal_num);
									ceilportal->type = SECPORTAL_LINE;
									ceilportal->line.start = &lines[i];
									ceilportal->line.dest = &lines[linenum];
									P_CopySectorPortalToLines(portal_num, target_sector_tag);
								}
							}
						}
					}

					else if (portal_type == TMSECPORTAL_SKYBOX)
					{
						if (floor)
						{
							sectorportal_t *floorportal = P_SectorGetFloorPortalOrCreate(target_sector, &portal_num);
							floorportal->type = SECPORTAL_SKYBOX;
							P_CopySectorPortalToLines(portal_num, target_sector_tag);
						}
						if (ceiling)
						{
							sectorportal_t *ceilportal = P_SectorGetCeilingPortalOrCreate(target_sector, &portal_num);
							ceilportal->type = SECPORTAL_SKYBOX;
							P_CopySectorPortalToLines(portal_num, target_sector_tag);
						}
					}

					else if (portal_type == TMSECPORTAL_SECTOR)
					{
						INT32 s2 = -1;
						TAG_ITER_SECTORS(misc, s2)  
						{
							sector_t *view_sector = &sectors[s2];
							if (floor)
							{
								sectorportal_t *floorportal = P_SectorGetFloorPortalOrCreate(target_sector, &portal_num);
								floorportal->type = SECPORTAL_CEILING;
								floorportal->sector = view_sector;
								P_CopySectorPortalToLines(portal_num, target_sector_tag);
							}
							if (ceiling)
							{
								sectorportal_t *ceilportal = P_SectorGetCeilingPortalOrCreate(target_sector, &portal_num);
								ceilportal->type = SECPORTAL_FLOOR;
								ceilportal->sector = view_sector;
								P_CopySectorPortalToLines(portal_num, target_sector_tag);
							}
						}
					}

					else if (portal_type == TMSECPORTAL_OBJECT)
					{
						mobj_t *mobj = P_GetMobjByTag(misc);
						if (!mobj)
							break;
						if (floor)
						{
							sectorportal_t *floorportal = P_SectorGetFloorPortalOrCreate(target_sector, &portal_num);
							floorportal->type = SECPORTAL_OBJECT;
							P_SetTarget(&floorportal->mobj, mobj);
							P_CopySectorPortalToLines(portal_num, target_sector_tag);
						}
						if (ceiling)
						{
							sectorportal_t *ceilportal = P_SectorGetCeilingPortalOrCreate(target_sector, &portal_num);
							ceilportal->type = SECPORTAL_OBJECT;
							P_SetTarget(&ceilportal->mobj, mobj);
							P_CopySectorPortalToLines(portal_num, target_sector_tag);
						}
					}
				}
				break;
			}

			case 7:  
			{

				angle_t flatangle = InvAngle(R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y));
				fixed_t xoffs = -lines[i].v1->x;
				fixed_t yoffs = lines[i].v1->y;


				if (lines[i].args[0] == 0)
					P_ApplyFlatAlignment(lines[i].frontsector, flatangle, xoffs, yoffs, lines[i].args[1] != TMP_CEILING, lines[i].args[1] != TMP_FLOOR);
				else
				{
					TAG_ITER_SECTORS(lines[i].args[0], s)
						P_ApplyFlatAlignment(sectors + s, flatangle, xoffs, yoffs, lines[i].args[1] != TMP_CEILING, lines[i].args[1] != TMP_FLOOR);
				}
				break;
			}

			case 8:  
				if (lines[i].frontsector)
					TAG_ITER_SECTORS(lines[i].args[0], s)
						sectors[s].camsec = lines[i].frontsector-sectors;
				break;

			case 10:  
				TAG_ITER_SECTORS(lines[i].args[0], s)
					sectors[s].cullheight = &lines[i];  
				break;

			case 50:  
				if (!udmf)
					EV_DoFloor(lines[i].args[0], &lines[i], instantLower);
				break;

			case 51:  
				if (!udmf)
					EV_DoCeiling(lines[i].args[0], &lines[i], instantRaise);
				break;

			case 52:  
				EV_DoContinuousFall(lines[i].frontsector, lines[i].backsector, lines[i].args[0] << FRACBITS, lines[i].args[1]);
				break;

			case 53:  
				if (lines[i].backsector)
				{
					if (lines[i].args[1] != TMP_CEILING)
						EV_DoFloor(lines[i].args[0], &lines[i], bounceFloor);
					if (lines[i].args[1] != TMP_FLOOR)
						EV_DoCeiling(lines[i].args[0], &lines[i], bounceCeiling);
				}
				break;

			case 56:  
				if (lines[i].backsector)
				{
					if (lines[i].args[1] != TMP_CEILING)
						EV_DoFloor(lines[i].args[0], &lines[i], bounceFloorCrush);
					if (lines[i].args[1] != TMP_FLOOR)
						EV_DoCeiling(lines[i].args[0], &lines[i], bounceCeilingCrush);
				}
				break;

			case 60:  
				EV_DoElevator(lines[i].args[0], &lines[i], elevateContinuous);
				break;

			case 61:  
				EV_DoCrush(lines[i].args[0], &lines[i], lines[i].args[1] ? raiseAndCrush : crushAndRaise);
				break;

			case 63:  
				sec = sides[*lines[i].sidenum].sector-sectors;
				TAG_ITER_SECTORS(lines[i].args[0], s)
					sectors[s].heightsec = (INT32)sec;
				break;

			case 64:  
				if (lines[i].args[0] == 0)  
				{
					TAG_ITER_SECTORS(lines[i].args[1], s)
					{
						for (j = 0; (unsigned)j < sectors[s].linecount; j++)
						{
							if (sectors[s].lines[j]->special < 100 || sectors[s].lines[j]->special >= 300)
								continue;

							Add_MasterDisappearer(abs(lines[i].args[2]), abs(lines[i].args[3]), abs(lines[i].args[4]), (INT32)(sectors[s].lines[j] - lines), (INT32)i);
						}
					}
				}
				else  
				{
					TAG_ITER_LINES(lines[i].args[0], s)
					{
						if (lines[s].special < 100 || lines[s].special >= 300)
							continue;

						if (lines[i].args[1] != 0 && !Tag_Find(&lines[s].frontsector->tags, lines[i].args[1]))
							continue;

						Add_MasterDisappearer(abs(lines[i].args[2]), abs(lines[i].args[3]), abs(lines[i].args[4]), s, (INT32)i);
					}
				}
				break;

			case 66:  
				TAG_ITER_SECTORS(lines[i].args[0], s)
					P_AddPlaneDisplaceThinker(lines[i].args[1], abs(lines[i].args[2])<<8, sides[lines[i].sidenum[0]].sector-sectors, s, lines[i].args[2] < 0);
				break;

			case 70:  
				if (udmf)
				{
					fixed_t destheight = lines[i].args[2] << FRACBITS;
					fixed_t startheight, topheight, bottomheight;

					TAG_ITER_LINES(lines[i].args[0], l)
					{
						if (lines[l].special < 100 || lines[l].special >= 300)
							continue;

						startheight = lines[l].frontsector->ceilingheight;
						topheight = max(startheight, destheight);
						bottomheight = min(startheight, destheight);

						P_AddRaiseThinker(lines[l].frontsector, lines[l].args[0], lines[i].args[1] << FRACBITS, topheight, bottomheight, (destheight < startheight), !!(lines[i].args[3]));
					}
				}
				break;

			case 71:  
				if (udmf)
				{
					TAG_ITER_LINES(lines[i].args[0], l)
					{
						if (lines[l].special < 100 || lines[l].special >= 300)
							continue;

						P_AddAirbob(lines[l].frontsector, lines[l].args[0], lines[i].args[1] << FRACBITS, !!(lines[i].args[2] & TMFB_REVERSE), !!(lines[i].args[2] & TMFB_SPINDASH), !!(lines[i].args[2] & TMFB_DYNAMIC));
					}
				}
				break;

			case 72:  
				if (udmf)
				{
					UINT16 sound = (lines[i].stringargs[0]) ? get_number(lines[i].stringargs[0]) : sfx_thwomp;

					TAG_ITER_LINES(lines[i].args[0], l)
					{
						if (lines[l].special < 100 || lines[l].special >= 300)
							continue;

						P_AddThwompThinker(lines[l].frontsector, &lines[l], lines[i].args[1] << (FRACBITS - 3), lines[i].args[2] << (FRACBITS - 3), sound);
					}
				}
				break;

			case 73:  
				if (udmf)
				{
					TAG_ITER_LINES(lines[i].args[0], l)
					{
						if (lines[l].special < 100 || lines[l].special >= 300)
							continue;

						P_AddLaserThinker(lines[l].args[0], lines + l, !!(lines[i].args[1]));
					}
				}
				break;

			case 100:  
				ffloorflags = FOF_EXISTS|FOF_SOLID|FOF_RENDERALL;


				if (lines[i].args[3] & TMFA_NOPLANES)
					ffloorflags &= ~FOF_RENDERPLANES;
				if (lines[i].args[3] & TMFA_NOSIDES)
					ffloorflags &= ~FOF_RENDERSIDES;
				if (lines[i].args[3] & TMFA_INSIDES)
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_BOTHPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_ALLSIDES;
				}
				if (lines[i].args[3] & TMFA_ONLYINSIDES)
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_INVERTPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_INVERTSIDES;
				}
				if (lines[i].args[3] & TMFA_NOSHADE)
					ffloorflags |= FOF_NOSHADE;
				if (lines[i].args[3] & TMFA_SPLAT)
					ffloorflags |= FOF_SPLAT;


				if (lines[i].args[4] & TMFT_INTANGIBLETOP)
					ffloorflags |= FOF_REVERSEPLATFORM;
				if (lines[i].args[4] & TMFT_INTANGIBLEBOTTOM)
					ffloorflags |= FOF_PLATFORM;
				if (lines[i].args[4] & TMFT_DONTBLOCKPLAYER)
					ffloorflags &= ~FOF_BLOCKPLAYER;
				if (lines[i].args[4] & TMFT_DONTBLOCKOTHERS)
					ffloorflags &= ~FOF_BLOCKOTHERS;


				if (ffloorflags & FOF_RENDERALL)
				{

					if (lines[i].args[1] < 255 || (lines[i].args[3] & TMFA_SPLAT))
						ffloorflags |= FOF_CUTEXTRA|FOF_EXTRA;
					else if (!(lines[i].args[4] & TMFT_VISIBLEFROMINSIDE))
						ffloorflags |= FOF_CUTLEVEL;
				}

				P_AddFakeFloorsByLine(i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
				break;

			case 120:  
				ffloorflags = FOF_EXISTS|FOF_RENDERPLANES|FOF_SWIMMABLE|FOF_BOTHPLANES|FOF_CUTEXTRA|FOF_EXTRA|FOF_CUTSPRITES;
				if (!(lines[i].args[3] & TMFW_NOSIDES))
					ffloorflags |= FOF_RENDERSIDES|FOF_ALLSIDES;
				if (lines[i].args[3] & TMFW_DOUBLESHADOW)
					ffloorflags |= FOF_DOUBLESHADOW;
				if (lines[i].args[3] & TMFW_COLORMAPONLY)
					ffloorflags |= FOF_COLORMAPONLY;
				if (!(lines[i].args[3] & TMFW_NORIPPLE))
					ffloorflags |= FOF_RIPPLE;
				if (lines[i].args[3] & TMFW_GOOWATER)
					ffloorflags |= FOF_GOOWATER;
				if (lines[i].args[3] & TMFW_SPLAT)
					ffloorflags |= FOF_SPLAT;
				P_AddFakeFloorsByLine(i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
				break;

			case 150:  
				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, FOF_EXISTS|FOF_SOLID|FOF_RENDERALL, secthinkers);
				P_AddAirbob(lines[i].frontsector, lines[i].args[0], lines[i].args[1] << FRACBITS, !!(lines[i].args[2] & TMFB_REVERSE), !!(lines[i].args[2] & TMFB_SPINDASH), !!(lines[i].args[2] & TMFB_DYNAMIC));
				break;

			case 160:  
				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, FOF_EXISTS|FOF_SOLID|FOF_RENDERALL|FOF_FLOATBOB, secthinkers);
				break;

			case 170:  
				ffloorflags = FOF_EXISTS|FOF_SOLID|FOF_RENDERALL|FOF_CRUMBLE;


				if (lines[i].args[3] & TMFT_INTANGIBLETOP)
					ffloorflags |= FOF_REVERSEPLATFORM;
				if (lines[i].args[3] & TMFT_INTANGIBLEBOTTOM)
					ffloorflags |= FOF_PLATFORM;
				if (lines[i].args[3] & TMFT_DONTBLOCKPLAYER)
					ffloorflags &= ~FOF_BLOCKPLAYER;
				if (lines[i].args[3] & TMFT_DONTBLOCKOTHERS)
					ffloorflags &= ~FOF_BLOCKOTHERS;


				if (lines[i].args[4] & TMFC_NOSHADE)
					ffloorflags |= FOF_NOSHADE;
				if (lines[i].args[4] & TMFC_NORETURN)
					ffloorflags |= FOF_NORETURN;
				if (lines[i].args[4] & TMFC_FLOATBOB)
					ffloorflags |= FOF_FLOATBOB;
				if (lines[i].args[4] & TMFC_SPLAT)
					ffloorflags |= FOF_SPLAT;


				if (lines[i].args[1] < 255 || (lines[i].args[4] & TMFC_SPLAT))
					ffloorflags |= FOF_CUTEXTRA|FOF_EXTRA;

				else if (lines[i].args[3] & TMFT_VISIBLEFROMINSIDE)
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_BOTHPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_ALLSIDES;
				}
				else
					ffloorflags |= FOF_CUTLEVEL;

				P_AddFakeFloorsByLine(i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
				if (lines[i].args[4] & TMFC_AIRBOB)
					P_AddAirbob(lines[i].frontsector, lines[i].args[0], 16*FRACUNIT, false, false, false);
				break;

			case 190:  
			{
				fixed_t ceilingtop = P_FindHighestCeilingSurrounding(lines[i].frontsector);
				fixed_t ceilingbottom = P_FindLowestCeilingSurrounding(lines[i].frontsector);

				ffloorflags = FOF_EXISTS|FOF_SOLID|FOF_RENDERALL;


				if (lines[i].args[3] & TMFA_NOPLANES)
					ffloorflags &= ~FOF_RENDERPLANES;
				if (lines[i].args[3] & TMFA_NOSIDES)
					ffloorflags &= ~FOF_RENDERSIDES;
				if (lines[i].args[3] & TMFA_INSIDES)
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_BOTHPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_ALLSIDES;
				}
				if (lines[i].args[3] & TMFA_ONLYINSIDES)
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_INVERTPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_INVERTSIDES;
				}
				if (lines[i].args[3] & TMFA_NOSHADE)
					ffloorflags |= FOF_NOSHADE;
				if (lines[i].args[3] & TMFA_SPLAT)
					ffloorflags |= FOF_SPLAT;


				if (lines[i].args[4] & TMFT_INTANGIBLETOP)
					ffloorflags |= FOF_REVERSEPLATFORM;
				if (lines[i].args[4] & TMFT_INTANGIBLEBOTTOM)
					ffloorflags |= FOF_PLATFORM;
				if (lines[i].args[4] & TMFT_DONTBLOCKPLAYER)
					ffloorflags &= ~FOF_BLOCKPLAYER;
				if (lines[i].args[4] & TMFT_DONTBLOCKOTHERS)
					ffloorflags &= ~FOF_BLOCKOTHERS;


				if (ffloorflags & FOF_RENDERALL)
				{

					if (lines[i].args[1] < 255 || (lines[i].args[3] & TMFA_SPLAT))
						ffloorflags |= FOF_CUTEXTRA|FOF_EXTRA;
					else if (!(lines[i].args[4] & TMFT_VISIBLEFROMINSIDE))
						ffloorflags |= FOF_CUTLEVEL;
				}

				P_AddFakeFloorsByLine(i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
				P_AddRaiseThinker(lines[i].frontsector, lines[i].args[0], lines[i].args[5] << FRACBITS, ceilingtop, ceilingbottom, !!(lines[i].args[6] & TMFR_REVERSE), !!(lines[i].args[6] & TMFR_SPINDASH));
				break;
			}
			case 200:  
				ffloorflags = FOF_EXISTS|FOF_CUTSPRITES;
				if (!lines[i].args[1])
					ffloorflags |= FOF_DOUBLESHADOW;
				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, ffloorflags, secthinkers);
				break;

			case 202:  
				ffloorflags = FOF_EXISTS|FOF_RENDERALL|FOF_FOG|FOF_INVERTPLANES|FOF_INVERTSIDES|FOF_CUTEXTRA|FOF_EXTRA|FOF_DOUBLESHADOW|FOF_CUTSPRITES;
				sec = sides[*lines[i].sidenum].sector - sectors;

				if (sectors[sec].extra_colormap)
					sectors[sec].extra_colormap->flags = CMF_FOG;
				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, ffloorflags, secthinkers);
				break;

			case 220:  
				ffloorflags = FOF_EXISTS|FOF_RENDERALL|FOF_CUTEXTRA|FOF_EXTRA|FOF_CUTSPRITES;


				if (lines[i].args[3] & TMFA_NOPLANES)
					ffloorflags &= ~FOF_RENDERPLANES;
				if (lines[i].args[3] & TMFA_NOSIDES)
					ffloorflags &= ~FOF_RENDERSIDES;
				if (!(lines[i].args[3] & TMFA_INSIDES))
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_BOTHPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_ALLSIDES;
				}
				if (lines[i].args[3] & TMFA_ONLYINSIDES)
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_INVERTPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_INVERTSIDES;
				}
				if (lines[i].args[3] & TMFA_NOSHADE)
					ffloorflags |= FOF_NOSHADE;
				if (lines[i].args[3] & TMFA_SPLAT)
					ffloorflags |= FOF_SPLAT;

				P_AddFakeFloorsByLine(i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
				break;

			case 223:  
				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, FOF_EXISTS|FOF_NOSHADE, secthinkers);
				break;

			case 250:  
				ffloorflags = FOF_EXISTS|FOF_SOLID|FOF_RENDERALL|FOF_CUTLEVEL|FOF_MARIO;
				if (lines[i].args[1] & TMFM_BRICK)
					ffloorflags |= FOF_GOOWATER;
				if (lines[i].args[1] & TMFM_INVISIBLE)
					ffloorflags &= ~(FOF_SOLID|FOF_RENDERALL|FOF_CUTLEVEL);

				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, ffloorflags, secthinkers);
				break;

			case 251:  
			{
				UINT16 sound = (lines[i].stringargs[0]) ? get_number(lines[i].stringargs[0]) : sfx_thwomp;
				P_AddThwompThinker(lines[i].frontsector, &lines[i], lines[i].args[1] << (FRACBITS - 3), lines[i].args[2] << (FRACBITS - 3), sound);
				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, FOF_EXISTS|FOF_SOLID|FOF_RENDERALL|FOF_CUTLEVEL, secthinkers);
				break;
			}

			case 254:  
			{
				UINT8 busttype = BT_REGULAR;
				ffloorbustflags_e bustflags = 0;

				ffloorflags = FOF_EXISTS|FOF_BLOCKOTHERS|FOF_RENDERALL|FOF_BUSTUP;


				switch (lines[i].args[3])
				{
					case TMFB_TOUCH:
						busttype = BT_TOUCH;
						break;
					case TMFB_SPIN:
						busttype = BT_SPINBUST;
						break;
					case TMFB_REGULAR:
						busttype = BT_REGULAR;
						break;
					case TMFB_STRONG:
						busttype = BT_STRONG;
						break;
				}


				if (lines[i].args[4] & TMFB_PUSHABLES)
					bustflags |= FB_PUSHABLES;
				if (lines[i].args[4] & TMFB_EXECUTOR)
					bustflags |= FB_EXECUTOR;
				if (lines[i].args[4] & TMFB_ONLYBOTTOM)
					bustflags |= FB_ONLYBOTTOM;
				if (lines[i].args[4] & TMFB_SPLAT)
					ffloorflags |= FOF_SPLAT;

				if (busttype != BT_TOUCH || bustflags & FB_ONLYBOTTOM)
					ffloorflags |= FOF_BLOCKPLAYER;

				TAG_ITER_SECTORS(lines[i].args[0], s)
				{
					ffloor_t *fflr = P_AddFakeFloor(&sectors[s], lines[i].frontsector, lines + i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
					if (!fflr)
						continue;
					fflr->bustflags = bustflags;
					fflr->busttype = busttype;
					fflr->busttag = lines[i].args[5];
				}
				break;
			}
			case 257:  
				ffloorflags = FOF_EXISTS|FOF_QUICKSAND|FOF_RENDERALL|FOF_ALLSIDES|FOF_CUTSPRITES;
				if (!(lines[i].args[1]))
					ffloorflags |= FOF_RIPPLE;

				TAG_ITER_SECTORS(lines[i].args[0], s)
				{
					ffloor_t *fflr = P_AddFakeFloor(&sectors[s], lines[i].frontsector, lines + i, 0xff, TMB_TRANSLUCENT, ffloorflags, secthinkers);
					if (!fflr)
						continue;
					fflr->sinkspeed = abs(lines[i].args[2]) << (FRACBITS - 1);
					fflr->friction = abs(lines[i].args[3]) << (FRACBITS - 6);
				}
				break;

			case 258:  
				ffloorflags = FOF_EXISTS|FOF_RENDERALL|FOF_NOSHADE|FOF_EXTRA|FOF_CUTEXTRA|FOF_TRANSLUCENT;
				P_AddLaserThinker(lines[i].args[0], lines + i, !!(lines[i].args[3] & TMFL_NOBOSSES));
				if (lines[i].args[3] & TMFL_SPLAT)
					ffloorflags |= FOF_SPLAT;
				P_AddFakeFloorsByLine(i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
				break;

			case 259:  
				TAG_ITER_SECTORS(lines[i].args[0], s)
				{
					ffloor_t *fflr = P_AddFakeFloor(&sectors[s], lines[i].frontsector, lines + i, lines[i].args[1], lines[i].args[2], lines[i].args[3], secthinkers);
					if (!fflr)
						continue;
					if (!udmf)  
					{
						if (lines[i].args[3] & FOF_QUICKSAND)
						{
							fflr->sinkspeed = abs(lines[i].dx) >> 1;
							fflr->friction = abs(lines[i].dy) >> 6;
						}
						if (lines[i].args[3] & FOF_BUSTUP)
						{
							switch (lines[i].args[4] % TMFB_ONLYBOTTOM)
							{
								case TMFB_TOUCH:
									fflr->busttype = BT_TOUCH;
									break;
								case TMFB_SPIN:
									fflr->busttype = BT_SPINBUST;
									break;
								case TMFB_REGULAR:
									fflr->busttype = BT_REGULAR;
									break;
								case TMFB_STRONG:
									fflr->busttype = BT_STRONG;
									break;
							}

							if (lines[i].args[4] & TMFB_ONLYBOTTOM)
								fflr->bustflags |= FB_ONLYBOTTOM;
							if (lines[i].flags & ML_MIDSOLID)
								fflr->bustflags |= FB_PUSHABLES;
							if (lines[i].flags & ML_WRAPMIDTEX)
							{
								fflr->bustflags |= FB_EXECUTOR;
								fflr->busttag = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
							}
						}
					}
				}
				break;

			case 300:  
			case 303:  
			case 305:  
			case 314:  
			case 317:  
			case 319:  
			case 331:  
			case 334:  
			case 337:  
			case 343:  
				if (lines[i].args[0] > TMT_EACHTIMEMASK)
					P_AddEachTimeThinker(&lines[i], lines[i].args[0] == TMT_EACHTIMEENTERANDEXIT);
				break;

			case 308:  
				if (!P_CheckGametypeRules(lines[i].args[2], (UINT32)lines[i].args[1]))
				{
					lines[i].special = 0;
					break;
				}
				if (lines[i].args[0] > TMT_EACHTIMEMASK)
					P_AddEachTimeThinker(&lines[i], lines[i].args[0] == TMT_EACHTIMEENTERANDEXIT);
				break;


			case 309:
				if (!(gametyperules & GTR_TEAMFLAGS))
				{
					lines[i].special = 0;
					break;
				}
				if (lines[i].args[0] > TMT_EACHTIMEMASK)
					P_AddEachTimeThinker(&lines[i], lines[i].args[0] == TMT_EACHTIMEENTERANDEXIT);
				break;


			case 313:
				P_AddNoEnemiesThinker(&lines[i]);
				break;


			case 321:
				lines[i].callcount = (lines[i].args[2] && lines[i].args[3] > 0) ? lines[i].args[3] : lines[i].args[1];  
				if (lines[i].args[0] > TMXT_EACHTIMEMASK)  
					P_AddEachTimeThinker(&lines[i], lines[i].args[0] == TMXT_EACHTIMEENTERANDEXIT);
				break;

			case 449:  
			{
				INT32 bossid = lines[i].args[0];
				if (bossid & ~15)  
				{
					CONS_Alert(CONS_WARNING,
						M_GetText("Boss enable linedef has an invalid boss ID (%d).\nConsider changing it or removing it entirely.\n"),
						bossid);
					break;
				}
				if (!(lines[i].args[1]))
				{
					bossdisabled |= (1<<bossid);  
					CONS_Debug(DBG_GAMELOGIC, "Line type 449 spawn effect: bossid disabled = %d", bossid);
				}
				break;
			}

			case 600:  
				sec = sides[*lines[i].sidenum].sector-sectors;
				TAG_ITER_SECTORS(lines[i].args[0], s)
				{
					if (lines[i].args[1] != TMP_CEILING)
						sectors[s].floorlightsec = (INT32)sec;
					if (lines[i].args[1] != TMP_FLOOR)
						sectors[s].ceilinglightsec = (INT32)sec;
				}
				break;

			case 602:  
				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(lines[i].args[0], s)
					P_SpawnAdjustableGlowingLight(&sectors[s], lines[i].args[2],
						lines[i].args[3] ? sectors[s].lightlevel : lines[i].args[4], lines[i].args[1]);
				break;

			case 603:  
				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(lines[i].args[0], s)
					P_SpawnAdjustableFireFlicker(&sectors[s], lines[i].args[2],
						lines[i].args[3] ? sectors[s].lightlevel : lines[i].args[4], lines[i].args[1]);
				break;

			case 604:  
				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(lines[i].args[0], s)
					P_SpawnAdjustableStrobeFlash(&sectors[s], lines[i].args[3],
						(lines[i].args[4] & TMB_USETARGET) ? sectors[s].lightlevel : lines[i].args[5],
						lines[i].args[1], lines[i].args[2], lines[i].args[4] & TMB_SYNC);
				break;

			case 606:  
				TAG_ITER_SECTORS(lines[i].args[0], s)
				{
					extracolormap_t *exc;

					if (sectors[s].colormap_protected)
						continue;

					if (!udmf)
						exc = sides[lines[i].sidenum[0]].colormap_data;
					else
					{
						if (!lines[i].args[1])
							exc = lines[i].frontsector->extra_colormap;
						else
						{
							INT32 sourcesec = Tag_Iterate_Sectors(lines[i].args[1], 0);
							if (sourcesec == -1)
							{
								CONS_Debug(DBG_GAMELOGIC, "Line type 606: Can't find sector with source colormap (tag %d)!\n", lines[i].args[1]);
								return;
							}
							exc = sectors[sourcesec].extra_colormap;
						}
					}
					sectors[s].extra_colormap = sectors[s].spawn_extra_colormap = exc;
				}
				break;

			default:
				break;
		}
	}


	for (i = 0; i < numlines; i++)
	{
		switch (lines[i].special)
		{
			INT32 s;
			INT32 l;

			case 74:  
			{
				UINT8 busttype = BT_REGULAR;
				ffloorbustflags_e bustflags = 0;

				if (!udmf)
					break;

				switch (lines[i].args[1])
				{
					case TMFB_TOUCH:
						busttype = BT_TOUCH;
						break;
					case TMFB_SPIN:
						busttype = BT_SPINBUST;
						break;
					case TMFB_REGULAR:
						busttype = BT_REGULAR;
						break;
					case TMFB_STRONG:
						busttype = BT_STRONG;
						break;
				}

				if (lines[i].args[2] & TMFB_PUSHABLES)
					bustflags |= FB_PUSHABLES;
				if (lines[i].args[2] & TMFB_EXECUTOR)
					bustflags |= FB_EXECUTOR;
				if (lines[i].args[2] & TMFB_ONLYBOTTOM)
					bustflags |= FB_ONLYBOTTOM;

				TAG_ITER_LINES(lines[i].args[0], l)
				{
					if (lines[l].special < 100 || lines[l].special >= 300)
						continue;

					TAG_ITER_SECTORS(lines[l].args[0], s)
					{
						ffloor_t *rover;

						for (rover = sectors[s].ffloors; rover; rover = rover->next)
						{
							if (rover->master != lines + l)
								continue;

							rover->fofflags |= FOF_BUSTUP;
							rover->spawnflags |= FOF_BUSTUP;
							rover->bustflags = bustflags;
							rover->busttype = busttype;
							rover->busttag = lines[i].args[3];
							CheckForBustableBlocks = true;
							break;
						}
					}
				}
				break;
			}

			case 75:  
			{
				if (!udmf)
					break;
				TAG_ITER_LINES(lines[i].args[0], l)
				{
					if (lines[l].special < 100 || lines[l].special >= 300)
						continue;

					TAG_ITER_SECTORS(lines[l].args[0], s)
					{
						ffloor_t *rover;

						for (rover = sectors[s].ffloors; rover; rover = rover->next)
						{
							if (rover->master != lines + l)
								continue;

							rover->fofflags |= FOF_QUICKSAND;
							rover->spawnflags |= FOF_QUICKSAND;
							rover->sinkspeed = abs(lines[i].args[1]) << (FRACBITS - 1);
							rover->friction = abs(lines[i].args[2]) << (FRACBITS - 6);
							CheckForQuicksand = true;
							break;
						}
					}
				}
				break;
			}

			case 76:  
			{
				if (udmf)
				{
					TAG_ITER_LINES(lines[i].args[0], l)
						P_MakeFOFBouncy(lines + i, lines + l);
				}
				else
				{
					TAG_ITER_SECTORS(lines[i].args[0], s)
						for (j = 0; (unsigned)j < sectors[s].linecount; j++)
							P_MakeFOFBouncy(lines + i, sectors[s].lines[j]);
				}
				break;
			}
		}
	}


	for (i = 0; i < numsectors; i++)
		if(secthinkers[i].thinkers)
			Z_Free(secthinkers[i].thinkers);

	Z_Free(secthinkers);


	Polyobj_InitLevel();

	for (i = 0; i < numlines; i++)
	{
		switch (lines[i].special)
		{
			case 30:  
				PolyFlag(&lines[i]);
				break;

			case 31:  
				PolyDisplace(&lines[i]);
				break;

			case 32:  
				PolyRotDisplace(&lines[i]);
				break;
		}
	}


	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special != SPECIAL_SECTOR_SETPORTAL)
			continue;

		int portal_type = lines[i].args[1];
		if (portal_type != TMSECPORTAL_COPIED)
			continue;

		int target_sector_tag = lines[i].args[0];
		int plane_type = lines[i].args[2];
		int tag_to_copy = lines[i].args[3];

		if (plane_type == 3)
			plane_type = TMP_BOTH;

		if (target_sector_tag == 0)
			P_DoPortalCopyFromLine(lines[i].frontsector, plane_type, tag_to_copy);
		else
		{
			INT32 s1 = -1;
			TAG_ITER_SECTORS(target_sector_tag, s1)
				P_DoPortalCopyFromLine(&sectors[s1], plane_type, tag_to_copy);
		}
	}

	if (!fromnetsave)
		P_RunLevelLoadExecutors();
}

                                                                   
   
                                              
                                           
                                     
                                            
                                                                       
                                       
                                       
    
static void P_AddFakeFloorsByLine(size_t line, INT32 alpha, UINT8 blendmode, ffloortype_e ffloorflags, thinkerlist_t *secthinkers)
{
	INT32 s;
	mtag_t tag = lines[line].args[0];
	size_t sec = sides[*lines[line].sidenum].sector-sectors;

	line_t* li = lines + line;
	TAG_ITER_SECTORS(tag, s)
		P_AddFakeFloor(&sectors[s], &sectors[sec], li, alpha, blendmode, ffloorflags, secthinkers);
}

  
                                            
          
              
                  
                 
  


static void P_DoScrollMove(mobj_t *thing, fixed_t dx, fixed_t dy, INT32 exclusive)
{
	fixed_t fuckaj = 0;  
	if (thing->player)
	{
		if (!(dx | dy))
		{
			thing->player->cmomx = 0;
			thing->player->cmomy = 0;
		}
		else
		{
			thing->player->cmomx += dx;
			thing->player->cmomy += dy;
			thing->player->cmomx = FixedMul(thing->player->cmomx, 0xe800);
			thing->player->cmomy = FixedMul(thing->player->cmomy, 0xe800);
		}
	}

	if (thing->player && (thing->player->pflags & PF_SPINNING) && (thing->player->rmomx || thing->player->rmomy) && !(thing->player->pflags & PF_STARTDASH))
		fuckaj = FixedDiv(549*ORIG_FRICTION,500*FRACUNIT);
	else if (thing->friction != ORIG_FRICTION)
		fuckaj = thing->friction;

	if (fuckaj) {

		dx = FixedDiv(dx, CARRYFACTOR);
		dy = FixedDiv(dy, CARRYFACTOR);

		dx = FixedMul(dx, FRACUNIT-fuckaj);
		dy = FixedMul(dy, FRACUNIT-fuckaj);
	}

	thing->momx += dx;
	thing->momy += dy;

	if (exclusive)
		thing->eflags |= MFE_PUSHED;
}

                                 
                                                                               
                                                                            
                                                                          
                                                                             
                                                                        
                                                                           
                                           
   
                                                                       
                                                                            
                                                                             
                                                
   
                                                 
                                           
                                                                       
                  
                                                        
                             
                                       
    
void T_Scroll(scroll_t *s)
{
	fixed_t dx = s->dx, dy = s->dy;
	boolean is3dblock = false;

	if (s->control != -1)
	{  
		fixed_t height = sectors[s->control].floorheight +
			sectors[s->control].ceilingheight;
		fixed_t delta = height - s->last_height;
		s->last_height = height;
		dx = FixedMul(dx, delta);
		dy = FixedMul(dy, delta);
	}

	if (s->accel)
	{
		s->vdx = dx += s->vdx;
		s->vdy = dy += s->vdy;
	}




	switch (s->type)
	{
		side_t *side;
		sector_t *sec;
		fixed_t height;
		msecnode_t *node;
		mobj_t *thing;
		line_t *line;
		size_t i;
		INT32 sect;
		ffloor_t *rover;

		case sc_side:  
			side = sides + s->affectee;
			side->textureoffset += dx;
			side->rowoffset += dy;
			break;

		case sc_floor:  
			sec = sectors + s->affectee;
			sec->floorxoffset += dx;
			sec->flooryoffset += dy;
			break;

		case sc_ceiling:  
			sec = sectors + s->affectee;
			sec->ceilingxoffset += dx;
			sec->ceilingyoffset += dy;
			break;

		case sc_carry:
			sec = sectors + s->affectee;
			height = sec->floorheight;


			for (i = 0; i < sec->linecount; i++)
			{
				line = sec->lines[i];

				if (line->special < 100 || line->special >= 300)
					is3dblock = false;
				else
					is3dblock = true;

				if (!is3dblock)
					continue;

				TAG_ITER_SECTORS(line->args[0], sect)
				{
					sector_t *psec;
					psec = sectors + sect;


					for (rover = psec->ffloors; rover; rover = rover->next)
					{
						if (rover->master == sec->lines[i])
							break;
					}

					if (!rover)  
						continue;

					if (!(rover->fofflags & FOF_EXISTS))  
						continue;

					for (node = psec->touching_thinglist; node; node = node->m_thinglist_next)
					{
						thing = node->m_thing;

						if (thing->eflags & MFE_PUSHED)  
							continue;

						height = P_GetSpecialBottomZ(thing, sec, psec);

						if (!(thing->flags & MF_NOCLIP))  
						if (!(thing->flags & MF_NOGRAVITY || thing->z+thing->height != height))  
						{


							P_DoScrollMove(thing, dx, dy, s->exclusive);
						}
					}  
				}  
			}

			if (!is3dblock)
			{
				for (node = sec->touching_thinglist; node; node = node->m_thinglist_next)
				{
					thing = node->m_thing;

					if (thing->eflags & MFE_PUSHED)
						continue;

					height = P_GetSpecialBottomZ(thing, sec, sec);

					if (!(thing->flags & MF_NOCLIP) &&
						(!(thing->flags & MF_NOGRAVITY || thing->z > height)))
					{


						P_DoScrollMove(thing, dx, dy, s->exclusive);
					}
				}
			}
			break;

		case sc_carry_ceiling:  
			sec = sectors + s->affectee;
			height = sec->ceilingheight;


			for (i = 0; i < sec->linecount; i++)
			{
				line = sec->lines[i];
				if (line->special < 100 || line->special >= 300)
					is3dblock = false;
				else
					is3dblock = true;

				if (!is3dblock)
					continue;
				TAG_ITER_SECTORS(line->args[0], sect)
				{
					sector_t *psec;
					psec = sectors + sect;


					for (rover = psec->ffloors; rover; rover = rover->next)
					{
						if (rover->master == sec->lines[i])
							break;
					}

					if (!rover)  
						continue;

					if (!(rover->fofflags & FOF_EXISTS))  
						continue;

					for (node = psec->touching_thinglist; node; node = node->m_thinglist_next)
					{
						thing = node->m_thing;

						if (thing->eflags & MFE_PUSHED)
							continue;

						height = P_GetSpecialTopZ(thing, sec, psec);

						if (!(thing->flags & MF_NOCLIP))  
						if (!(thing->flags & MF_NOGRAVITY || thing->z != height)) 
						{


							P_DoScrollMove(thing, dx, dy, s->exclusive);
						}
					}  
				}  
			}

			if (!is3dblock)
			{
				for (node = sec->touching_thinglist; node; node = node->m_thinglist_next)
				{
					thing = node->m_thing;

					if (thing->eflags & MFE_PUSHED)
						continue;

					height = P_GetSpecialTopZ(thing, sec, sec);

					if (!(thing->flags & MF_NOCLIP) &&
						(!(thing->flags & MF_NOGRAVITY || thing->z+thing->height < height)))
					{


						P_DoScrollMove(thing, dx, dy, s->exclusive);
					}
				}
			}
			break;  
	}  
}

static boolean IsSector3DBlock(sector_t* sec)
{
	size_t i;
	for (i = 0; i < sec->linecount; i++)
	{
		if (sec->lines[i]->special >= 100 && sec->lines[i]->special < 300)
			return true;
	}
	return false;
}

                                                    
   
                                                     
                                                             
                                                             
                                                                       
                                                                  
                                                                    
                                                       
                                                    
    
static void Add_Scroller(INT32 type, fixed_t dx, fixed_t dy, INT32 control, INT32 affectee, INT32 accel, INT32 exclusive)
{
	scroll_t *s = Z_Calloc(sizeof *s, PU_LEVSPEC, NULL);
	s->thinker.function.acp1 = (actionf_p1)T_Scroll;
	s->type = type;
	s->dx = dx;
	s->dy = dy;
	s->accel = accel;
	s->exclusive = exclusive;
	s->vdx = s->vdy = 0;
	s->control = control;
	if (s->control != -1)
		s->last_height = sectors[control].floorheight + sectors[control].ceilingheight;
	s->affectee = affectee;
	if (type == sc_carry || type == sc_carry_ceiling)
	{
		sectors[affectee].specialflags |= SSF_CONVEYOR;
		if (IsSector3DBlock(&sectors[affectee]))
		{
			if (type == sc_carry)
				sectors[affectee].flags |= MSF_FLIPSPECIAL_CEILING;
			else
				sectors[affectee].flags |= MSF_FLIPSPECIAL_FLOOR;
		}
	}
	P_AddThinker(THINK_MAIN, &s->thinker);


	switch (type)
	{
		case sc_side:
			R_CreateInterpolator_SideScroll(&s->thinker, &sides[affectee]);
			break;
		case sc_floor:
			R_CreateInterpolator_SectorScroll(&s->thinker, &sectors[affectee], false);
			break;
		case sc_ceiling:
			R_CreateInterpolator_SectorScroll(&s->thinker, &sectors[affectee], true);
			break;
		default:
			break;
	}
}

static void P_SpawnPlaneScroller(line_t *l, fixed_t dx, fixed_t dy, INT32 control, INT32 affectee, INT32 accel, INT32 exclusive)
{
	if (l->args[1] != TMP_CEILING)
	{
		if (l->args[2] != TMS_SCROLLONLY)
			Add_Scroller(sc_carry, FixedMul(dx, CARRYFACTOR), FixedMul(dy, CARRYFACTOR), control, affectee, accel, exclusive);
		if (l->args[2] != TMS_CARRYONLY)
			Add_Scroller(sc_floor, -dx, dy, control, affectee, accel, exclusive);
	}
	if (l->args[1] != TMP_FLOOR)
	{
		if (l->args[2] != TMS_SCROLLONLY)
			Add_Scroller(sc_carry_ceiling, FixedMul(dx, CARRYFACTOR), FixedMul(dy, CARRYFACTOR), control, affectee, accel, exclusive);
		if (l->args[2] != TMS_CARRYONLY)
			Add_Scroller(sc_ceiling, -dx, dy, control, affectee, accel, exclusive);
	}
}

                              
   
                                           
                                                       
    
static void P_SpawnScrollers(void)
{
	size_t i;
	line_t *l = lines;

	for (i = 0; i < numlines; i++, l++)
	{
		INT32 control = -1, accel = 0;  

		if (l->special == 502 || l->special == 510)
		{
			if ((l->args[4] & TMST_TYPEMASK) != TMST_REGULAR)
				control = (INT32)(sides[*l->sidenum].sector - sectors);
			if ((l->args[4] & TMST_TYPEMASK) == TMST_ACCELERATIVE)
				accel = 1;
		}

		switch (l->special)
		{
			register INT32 s;

			case 510:  
			{
				fixed_t length = R_PointToDist2(l->v2->x, l->v2->y, l->v1->x, l->v1->y);
				fixed_t speed = l->args[3] << FRACBITS;
				fixed_t dx = FixedMul(FixedDiv(l->dx, length), speed) >> SCROLL_SHIFT;
				fixed_t dy = FixedMul(FixedDiv(l->dy, length), speed) >> SCROLL_SHIFT;

				if (l->args[0] == 0)
					P_SpawnPlaneScroller(l, dx, dy, control, (INT32)(l->frontsector - sectors), accel, !(l->args[4] & TMST_NONEXCLUSIVE));
				else
				{
					TAG_ITER_SECTORS(l->args[0], s)
						P_SpawnPlaneScroller(l, dx, dy, control, s, accel, !(l->args[4] & TMST_NONEXCLUSIVE));
				}
				break;
			}



			case 502:
			{
				TAG_ITER_LINES(l->args[0], s)
					if (s != (INT32)i)
					{
						if (l->args[1] != TMSD_BACK)
							Add_Scroller(sc_side, l->args[2] << (FRACBITS - SCROLL_SHIFT), l->args[3] << (FRACBITS - SCROLL_SHIFT), control, lines[s].sidenum[0], accel, 0);
						if (l->args[1] != TMSD_FRONT && lines[s].sidenum[1] != NO_SIDEDEF)
							Add_Scroller(sc_side, l->args[2] << (FRACBITS - SCROLL_SHIFT), l->args[3] << (FRACBITS - SCROLL_SHIFT), control, lines[s].sidenum[1], accel, 0);
					}
				break;
			}

			case 500:
				if (l->args[0] != TMSD_BACK)
					Add_Scroller(sc_side, -l->args[1] << FRACBITS, l->args[2] << FRACBITS, -1, l->sidenum[0], accel, 0);
				if (l->args[0] != TMSD_FRONT)
				{
					if (l->sidenum[1] != NO_SIDEDEF)
						Add_Scroller(sc_side, -l->args[1] << FRACBITS, l->args[2] << FRACBITS, -1, l->sidenum[1], accel, 0);
					else
						CONS_Debug(DBG_GAMELOGIC, "Line special 500 (line #%s) missing back side!\n", sizeu1(i));
				}
				break;
		}
	}
}

                                         
   
                                          
                                               
                                             
    
static void Add_MasterDisappearer(tic_t appeartime, tic_t disappeartime, tic_t offset, INT32 line, INT32 sourceline)
{
	disappear_t *d = Z_Malloc(sizeof *d, PU_LEVSPEC, NULL);

	d->thinker.function.acp1 = (actionf_p1)T_Disappear;
	d->appeartime = appeartime;
	d->disappeartime = disappeartime;
	d->offset = offset;
	d->affectee = line;
	d->sourceline = sourceline;
	d->exists = true;
	d->timer = 1;

	P_AddThinker(THINK_MAIN, &d->thinker);
}

                                
   
                               
                             
    
void T_Disappear(disappear_t *d)
{
	if (d->offset && !d->exists)
	{
		d->offset--;
		return;
	}

	if (--d->timer <= 0)
	{
		ffloor_t *rover;
		register INT32 s;
		mtag_t afftag = lines[d->affectee].args[0];

		TAG_ITER_SECTORS(afftag, s)
		{
			for (rover = sectors[s].ffloors; rover; rover = rover->next)
			{
				if (rover->master != &lines[d->affectee])
					continue;

				if (d->exists)
					rover->fofflags &= ~FOF_EXISTS;
				else
				{
					rover->fofflags |= FOF_EXISTS;

					if (!(lines[d->sourceline].args[5]))
					{
						sectors[s].soundorg.z = P_GetFFloorTopZAt(rover, sectors[s].soundorg.x, sectors[s].soundorg.y);
						S_StartSound(&sectors[s].soundorg, sfx_appear);
					}
				}
			}
			sectors[s].moved = true;
			P_RecalcPrecipInSector(&sectors[s]);
		}

		if (d->exists)
		{
			d->timer = d->disappeartime;
			d->exists = false;
		}
		else
		{
			d->timer = d->appeartime;
			d->exists = true;
		}
	}
}

                                              
  
                                               
                                                                      
   
static void P_ResetFakeFloorFader(ffloor_t *rover, fade_t *data, boolean finalize)
{
	fade_t *fadingdata = (fade_t *)rover->fadingdata;

	if (fadingdata != data)
	{
		if (fadingdata)
		{
			if (finalize)
				P_FadeFakeFloor(rover,
					fadingdata->sourcevalue,
					fadingdata->alpha >= fadingdata->destvalue ?
						fadingdata->alpha - 1 :  
						fadingdata->alpha + 1,  
					0,
					fadingdata->ticbased,
					&fadingdata->timer,
					fadingdata->doexists,
					fadingdata->dotranslucent,
					fadingdata->dolighting,
					fadingdata->docolormap,
					fadingdata->docollision,
					fadingdata->doghostfade,
					fadingdata->exactalpha);
			rover->alpha = fadingdata->alpha;

			if (fadingdata->dolighting)
				P_RemoveLighting(&sectors[rover->secnum]);

			if (fadingdata->docolormap)
				P_ResetColormapFader(&sectors[rover->secnum]);

			P_RemoveThinker(&fadingdata->thinker);
		}

		rover->fadingdata = data;
	}
}

static boolean P_FadeFakeFloor(ffloor_t *rover, INT16 sourcevalue, INT16 destvalue, INT16 speed, boolean ticbased, INT32 *timer,
	boolean doexists, boolean dotranslucent, boolean dolighting, boolean docolormap,
	boolean docollision, boolean doghostfade, boolean exactalpha)
{
	boolean stillfading = false;
	INT32 alpha;
	fade_t *fadingdata = (fade_t *)rover->fadingdata;
	(void)docolormap;  

	if (rover->master->special == 258)  
		return false;


	if (dotranslucent &&
		(rover->spawnflags & FOF_NOSHADE) &&  
		!(rover->fofflags & FOF_FOG) &&  
		!(rover->spawnflags & FOF_RENDERSIDES) &&
		!(rover->spawnflags & FOF_RENDERPLANES) &&
		!(rover->fofflags & FOF_RENDERALL))
		rover->alpha = 0;

	if (fadingdata)
		alpha = fadingdata->alpha;
	else
		alpha = rover->alpha;


	if (!ticbased && alpha == destvalue)
		return stillfading;
	else if (alpha > destvalue)  
	{

		if (speed < 1 || (!ticbased && alpha - speed <= destvalue + speed) ||
			(ticbased && (--(*timer) <= 0 || alpha <= destvalue)))
		{
			alpha = destvalue;

			if (docollision)
			{
				if (rover->spawnflags & FOF_SOLID)
					rover->fofflags &= ~FOF_SOLID;
				if (rover->spawnflags & FOF_SWIMMABLE)
					rover->fofflags &= ~FOF_SWIMMABLE;
				if (rover->spawnflags & FOF_QUICKSAND)
					rover->fofflags &= ~FOF_QUICKSAND;
				if (rover->spawnflags & FOF_BUSTUP)
					rover->fofflags &= ~FOF_BUSTUP;
				if (rover->spawnflags & FOF_MARIO)
					rover->fofflags &= ~FOF_MARIO;
			}
		}
		else  
		{
			if (!ticbased)
				alpha -= speed;
			else
			{
				INT16 delta = abs(destvalue - sourcevalue);
				fixed_t factor = min(FixedDiv(speed - (*timer), speed), 1*FRACUNIT);
				alpha = max(min(alpha, sourcevalue - (INT16)FixedMul(delta, factor)), destvalue);
			}
			stillfading = true;
		}
	}
	else  
	{

		if (speed < 1 || (!ticbased && alpha + speed >= destvalue - speed) ||
			(ticbased && (--(*timer) <= 0 || alpha >= destvalue)))
		{
			alpha = destvalue;

			if (docollision)
			{
				if (rover->spawnflags & FOF_SOLID)
					rover->fofflags |= FOF_SOLID;
				if (rover->spawnflags & FOF_SWIMMABLE)
					rover->fofflags |= FOF_SWIMMABLE;
				if (rover->spawnflags & FOF_QUICKSAND)
					rover->fofflags |= FOF_QUICKSAND;
				if (rover->spawnflags & FOF_BUSTUP)
					rover->fofflags |= FOF_BUSTUP;
				if (rover->spawnflags & FOF_MARIO)
					rover->fofflags |= FOF_MARIO;
			}
		}
		else  
		{
			if (!ticbased)
				alpha += speed;
			else
			{
				INT16 delta = abs(destvalue - sourcevalue);
				fixed_t factor = min(FixedDiv(speed - (*timer), speed), 1*FRACUNIT);
				alpha = min(max(alpha, sourcevalue + (INT16)FixedMul(delta, factor)), destvalue);
			}
			stillfading = true;
		}
	}


	if (!stillfading)
	{
		if (doexists && !(rover->spawnflags & FOF_BUSTUP))
		{
			if (alpha <= 0)
				rover->fofflags &= ~FOF_EXISTS;
			else
				rover->fofflags |= FOF_EXISTS;


			if (dolighting && !(rover->spawnflags & FOF_NOSHADE) && !(rover->fofflags & FOF_EXISTS))
				rover->target->moved = true;
		}

		if (dotranslucent && !(rover->fofflags & FOF_FOG))
		{
			if (alpha >= 255)
			{
				if (!(rover->fofflags & FOF_CUTSOLIDS) &&
					(rover->spawnflags & FOF_CUTSOLIDS))
				{
					rover->fofflags |= FOF_CUTSOLIDS;
					rover->target->moved = true;
				}

				rover->fofflags &= ~FOF_TRANSLUCENT;
			}
			else
			{
				rover->fofflags |= FOF_TRANSLUCENT;

				if ((rover->fofflags & FOF_CUTSOLIDS) &&
					(rover->spawnflags & FOF_CUTSOLIDS))
				{
					rover->fofflags &= ~FOF_CUTSOLIDS;
					rover->target->moved = true;
				}
			}

			if ((rover->spawnflags & FOF_NOSHADE) &&  
				!(rover->spawnflags & FOF_RENDERSIDES) &&
				!(rover->spawnflags & FOF_RENDERPLANES))
			{
				if (rover->alpha > 1)
					rover->fofflags |= FOF_RENDERALL;
				else
					rover->fofflags &= ~FOF_RENDERALL;
			}
		}
	}
	else
	{
		if (doexists && !(rover->spawnflags & FOF_BUSTUP))
		{

			if (dolighting && !(rover->spawnflags & FOF_NOSHADE) && !(rover->fofflags & FOF_EXISTS))
				rover->target->moved = true;

			rover->fofflags |= FOF_EXISTS;
		}

		if (dotranslucent && !(rover->fofflags & FOF_FOG))
		{
			rover->fofflags |= FOF_TRANSLUCENT;

			if ((rover->fofflags & FOF_CUTSOLIDS) &&
				(rover->spawnflags & FOF_CUTSOLIDS))
			{
				rover->fofflags &= ~FOF_CUTSOLIDS;
				rover->target->moved = true;
			}

			if ((rover->spawnflags & FOF_NOSHADE) &&  
				!(rover->spawnflags & FOF_RENDERSIDES) &&
				!(rover->spawnflags & FOF_RENDERPLANES))
				rover->fofflags |= FOF_RENDERALL;
		}

		if (docollision)
		{
			if (doghostfade)  
			{
				if (rover->spawnflags & FOF_SOLID)
					rover->fofflags &= ~FOF_SOLID;
				if (rover->spawnflags & FOF_SWIMMABLE)
					rover->fofflags &= ~FOF_SWIMMABLE;
				if (rover->spawnflags & FOF_QUICKSAND)
					rover->fofflags &= ~FOF_QUICKSAND;
				if (rover->spawnflags & FOF_BUSTUP)
					rover->fofflags &= ~FOF_BUSTUP;
				if (rover->spawnflags & FOF_MARIO)
					rover->fofflags &= ~FOF_MARIO;
			}
			else  
			{
				if (rover->spawnflags & FOF_SOLID)
					rover->fofflags |= FOF_SOLID;
				if (rover->spawnflags & FOF_SWIMMABLE)
					rover->fofflags |= FOF_SWIMMABLE;
				if (rover->spawnflags & FOF_QUICKSAND)
					rover->fofflags |= FOF_QUICKSAND;
				if (rover->spawnflags & FOF_BUSTUP)
					rover->fofflags |= FOF_BUSTUP;
				if (rover->spawnflags & FOF_MARIO)
					rover->fofflags |= FOF_MARIO;
			}
		}
	}

	if (!(rover->fofflags & FOF_FOG))  
	{
		if (!stillfading || exactalpha)
			rover->alpha = alpha;
		else  
		{
			if (alpha < 12)
				rover->alpha = destvalue < 12 ? destvalue : 0;  
			else if (alpha < 38)
				rover->alpha = destvalue >= 12 && destvalue < 38 ? destvalue : 25;
			else if (alpha < 64)
				rover->alpha = destvalue >= 38 && destvalue < 64 ? destvalue : 51;
			else if (alpha < 89)
				rover->alpha = destvalue >= 64 && destvalue < 89 ? destvalue : 76;
			else if (alpha < 115)
				rover->alpha = destvalue >= 89 && destvalue < 115 ? destvalue : 102;
			else if (alpha < 140)
				rover->alpha = destvalue >= 115 && destvalue < 140 ? destvalue : 128;
			else if (alpha < 166)
				rover->alpha = destvalue >= 140 && destvalue < 166 ? destvalue : 154;
			else if (alpha < 192)
				rover->alpha = destvalue >= 166 && destvalue < 192 ? destvalue : 179;
			else if (alpha < 217)
				rover->alpha = destvalue >= 192 && destvalue < 217 ? destvalue : 204;
			else if (alpha < 243)
				rover->alpha = destvalue >= 217 && destvalue < 243 ? destvalue : 230;
			else  
				rover->alpha = destvalue >= 243 ? destvalue : 255;
		}
	}

	if (fadingdata)
		fadingdata->alpha = alpha;

	return stillfading;
}

                                  
   
                                                     
                                        
                                                         
                                                             
                                         
                                               
                                     
                                               
                                                          
                                                       
    
static void P_AddFakeFloorFader(ffloor_t *rover, size_t sectornum, size_t ffloornum,
	INT16 destvalue, INT16 speed, boolean ticbased, boolean relative,
	boolean doexists, boolean dotranslucent, boolean dolighting, boolean docolormap,
	boolean docollision, boolean doghostfade, boolean exactalpha)
{
	fade_t *d;


	if (dotranslucent &&
		(rover->spawnflags & FOF_NOSHADE) &&  
		!(rover->spawnflags & FOF_RENDERSIDES) &&
		!(rover->spawnflags & FOF_RENDERPLANES) &&
		!(rover->fofflags & FOF_RENDERALL))
		rover->alpha = 0;


	if (rover->alpha == max(0, min(255, relative ? rover->alpha + destvalue : destvalue)))
		return;

	d = Z_Malloc(sizeof *d, PU_LEVSPEC, NULL);

	d->thinker.function.acp1 = (actionf_p1)T_Fade;
	d->rover = rover;
	d->sectornum = (UINT32)sectornum;
	d->ffloornum = (UINT32)ffloornum;

	d->alpha = d->sourcevalue = rover->alpha;
	d->destvalue = max(0, min(255, relative ? rover->alpha + destvalue : destvalue));  

	if (ticbased)
	{
		d->ticbased = true;
		d->timer = d->speed = abs(speed);  
	}
	else
	{
		d->ticbased = false;
		d->speed = max(1, speed);  
		d->timer = -1;
	}

	d->doexists = doexists;
	d->dotranslucent = dotranslucent;
	d->dolighting = dolighting;
	d->docolormap = docolormap;
	d->docollision = docollision;
	d->doghostfade = doghostfade;
	d->exactalpha = exactalpha;


	P_ResetFakeFloorFader(rover, d, false);


	if (dolighting && !(rover->fofflags & FOF_NOSHADE))
	{
		UINT16 lightdelta = abs(sectors[rover->secnum].spawn_lightlevel - rover->target->lightlevel);
		fixed_t alphapercent = min(FixedDiv(d->destvalue, rover->spawnalpha), 1*FRACUNIT);  
		fixed_t adjustedlightdelta = FixedMul(lightdelta, alphapercent);

		if (rover->target->lightlevel >= sectors[rover->secnum].spawn_lightlevel)  
			d->destlightlevel = rover->target->lightlevel - adjustedlightdelta;
		else  
			d->destlightlevel = rover->target->lightlevel + adjustedlightdelta;

		P_FadeLightBySector(&sectors[rover->secnum],
			d->destlightlevel,
			ticbased ? d->timer :
				FixedFloor(FixedDiv(abs(d->destvalue - d->alpha), d->speed))/FRACUNIT,
			true);
	}
	else
		d->destlightlevel = -1;


	if (docolormap && !(rover->fofflags & FOF_NOSHADE) && sectors[rover->secnum].spawn_extra_colormap && !sectors[rover->secnum].colormap_protected)
	{
		extracolormap_t *dest_exc,
			*source_exc = sectors[rover->secnum].extra_colormap ? sectors[rover->secnum].extra_colormap : R_GetDefaultColormap();

		INT32 colordelta = R_GetRgbaA(sectors[rover->secnum].spawn_extra_colormap->rgba);  
		fixed_t alphapercent = min(FixedDiv(d->destvalue, rover->spawnalpha), 1*FRACUNIT);  
		fixed_t adjustedcolordelta = FixedMul(colordelta, alphapercent);
		INT32 coloralpha;

		coloralpha = adjustedcolordelta;

		dest_exc = R_CopyColormap(sectors[rover->secnum].spawn_extra_colormap, false);
		dest_exc->rgba = R_GetRgbaRGB(dest_exc->rgba) + R_PutRgbaA(coloralpha);

		if (!(d->dest_exc = R_GetColormapFromList(dest_exc)))
		{
			dest_exc->colormap = R_CreateLightTable(dest_exc);
			R_AddColormapToList(dest_exc);
			d->dest_exc = dest_exc;
		}
		else
			Z_Free(dest_exc);


		if (!R_CheckDefaultColormap(d->dest_exc, true, false, false)
			&& R_CheckDefaultColormap(source_exc, true, false, false))
		{
			extracolormap_t *exc = R_CopyColormap(source_exc, false);
			exc->rgba = R_GetRgbaRGB(d->dest_exc->rgba) + R_PutRgbaA(R_GetRgbaA(source_exc->rgba));
			exc->fadergba = R_GetRgbaRGB(d->dest_exc->rgba) + R_PutRgbaA(R_GetRgbaA(source_exc->fadergba));

			if (!(source_exc = R_GetColormapFromList(exc)))
			{
				exc->colormap = R_CreateLightTable(exc);
				R_AddColormapToList(exc);
				source_exc = exc;
			}
			else
				Z_Free(exc);
		}

		Add_ColormapFader(&sectors[rover->secnum], source_exc, d->dest_exc, true,
			ticbased ? d->timer :
				FixedFloor(FixedDiv(abs(d->destvalue - d->alpha), d->speed))/FRACUNIT);
	}

	P_AddThinker(THINK_MAIN, &d->thinker);
}

                    
   
                          
                           
    
void T_Fade(fade_t *d)
{
	if (d->rover && !P_FadeFakeFloor(d->rover, d->sourcevalue, d->destvalue, d->speed, d->ticbased, &d->timer,
		d->doexists, d->dotranslucent, d->dolighting, d->docolormap, d->docollision, d->doghostfade, d->exactalpha))
	{

		if (d->dolighting && !(d->rover->fofflags & FOF_NOSHADE) && d->destlightlevel > -1)
			sectors[d->rover->secnum].lightlevel = d->destlightlevel;


		if (d->docolormap && !(d->rover->fofflags & FOF_NOSHADE) && sectors[d->rover->secnum].spawn_extra_colormap)
			sectors[d->rover->secnum].extra_colormap = d->dest_exc;

		P_RemoveFakeFloorFader(d->rover);
	}
}

static void P_ResetColormapFader(sector_t *sector)
{
	if (sector->fadecolormapdata)
	{



		P_RemoveThinker(&((thinkerdata_t *)sector->fadecolormapdata)->thinker);
		sector->fadecolormapdata = NULL;
	}
}

static void Add_ColormapFader(sector_t *sector, extracolormap_t *source_exc, extracolormap_t *dest_exc,
	boolean ticbased, INT32 duration)
{
	fadecolormap_t *d;

	P_ResetColormapFader(sector);


	if (!duration || R_CheckEqualColormaps(source_exc, dest_exc, true, true, true))
	{
		sector->extra_colormap = dest_exc;
		return;
	}

	d = Z_Malloc(sizeof *d, PU_LEVSPEC, NULL);
	d->thinker.function.acp1 = (actionf_p1)T_FadeColormap;
	d->sector = sector;
	d->source_exc = source_exc;
	d->dest_exc = dest_exc;

	if (ticbased)
	{
		d->ticbased = true;
		d->duration = d->timer = duration;
	}
	else
	{
		d->ticbased = false;
		d->timer = 256;
		d->duration = duration;  
	}

	sector->fadecolormapdata = d;
	P_AddThinker(THINK_MAIN, &d->thinker);
}

void T_FadeColormap(fadecolormap_t *d)
{
	if ((d->ticbased && --d->timer <= 0)
		|| (!d->ticbased && (d->timer -= d->duration) <= 0))  
	{
		d->sector->extra_colormap = d->dest_exc;
		P_ResetColormapFader(d->sector);
	}
	else
	{
		extracolormap_t *exc;
		INT32 duration = d->ticbased ? d->duration : 256;
		fixed_t factor = min(FixedDiv(duration - d->timer, duration), 1*FRACUNIT);
		INT16 cr, cg, cb, ca, fadestart, fadeend, flags;
		INT32 rgba, fadergba;


		if (!d->sector->extra_colormap)
			d->sector->extra_colormap = R_GetDefaultColormap();

		if (!d->source_exc)
			d->source_exc = R_GetDefaultColormap();

		if (!d->dest_exc)
			d->dest_exc = R_GetDefaultColormap();






#define APPLYFADE(dest, src, cur) (\
(dest-src < 0) ? \
	max(\
		min(cur,\
			src + (INT16)FixedMul(dest-src, factor)),\
		dest)\
: (dest-src > 0) ? \
	min(\
		max(cur,\
			src + (INT16)FixedMul(dest-src, factor)),\
		dest)\
: \
	dest\
)

		cr = APPLYFADE(R_GetRgbaR(d->dest_exc->rgba), R_GetRgbaR(d->source_exc->rgba), R_GetRgbaR(d->sector->extra_colormap->rgba));
		cg = APPLYFADE(R_GetRgbaG(d->dest_exc->rgba), R_GetRgbaG(d->source_exc->rgba), R_GetRgbaG(d->sector->extra_colormap->rgba));
		cb = APPLYFADE(R_GetRgbaB(d->dest_exc->rgba), R_GetRgbaB(d->source_exc->rgba), R_GetRgbaB(d->sector->extra_colormap->rgba));
		ca = APPLYFADE(R_GetRgbaA(d->dest_exc->rgba), R_GetRgbaA(d->source_exc->rgba), R_GetRgbaA(d->sector->extra_colormap->rgba));

		rgba = R_PutRgbaRGBA(cr, cg, cb, ca);

		cr = APPLYFADE(R_GetRgbaR(d->dest_exc->fadergba), R_GetRgbaR(d->source_exc->fadergba), R_GetRgbaR(d->sector->extra_colormap->fadergba));
		cg = APPLYFADE(R_GetRgbaG(d->dest_exc->fadergba), R_GetRgbaG(d->source_exc->fadergba), R_GetRgbaG(d->sector->extra_colormap->fadergba));
		cb = APPLYFADE(R_GetRgbaB(d->dest_exc->fadergba), R_GetRgbaB(d->source_exc->fadergba), R_GetRgbaB(d->sector->extra_colormap->fadergba));
		ca = APPLYFADE(R_GetRgbaA(d->dest_exc->fadergba), R_GetRgbaA(d->source_exc->fadergba), R_GetRgbaA(d->sector->extra_colormap->fadergba));

		fadergba = R_PutRgbaRGBA(cr, cg, cb, ca);

		fadestart = APPLYFADE(d->dest_exc->fadestart, d->source_exc->fadestart, d->sector->extra_colormap->fadestart);
		fadeend = APPLYFADE(d->dest_exc->fadeend, d->source_exc->fadeend, d->sector->extra_colormap->fadeend);
		flags = abs(factor) > FRACUNIT/2 ? d->dest_exc->flags : d->source_exc->flags;  

#undef APPLYFADE





		if (!(d->sector->extra_colormap = R_GetColormapFromListByValues(rgba, fadergba, fadestart, fadeend, flags)))
		{
			exc = R_CreateDefaultColormap(false);
			exc->fadestart = fadestart;
			exc->fadeend = fadeend;
			exc->flags = flags;
			exc->rgba = rgba;
			exc->fadergba = fadergba;
			exc->colormap = R_CreateLightTable(exc);
			R_AddColormapToList(exc);
			d->sector->extra_colormap = exc;
		}
	}
}

  
                                         
              
            
                
  

                          
   
                                                          
                                       
                                   
                                   
    
static void Add_Friction(INT32 friction, INT32 movefactor, INT32 affectee, INT32 referrer)
{
	friction_t *f = Z_Calloc(sizeof *f, PU_LEVSPEC, NULL);

	f->thinker.function.acp1 = (actionf_p1)T_Friction;
	f->friction = friction;
	f->movefactor = movefactor;
	f->affectee = affectee;

	if (referrer != -1)
	{
		f->roverfriction = true;
		f->referrer = referrer;
	}
	else
		f->roverfriction = false;

	P_AddThinker(THINK_MAIN, &f->thinker);
}

                                               
   
                              
                    
    
void T_Friction(friction_t *f)
{
	sector_t *sec, *referrer = NULL;
	mobj_t *thing;
	msecnode_t *node;

	sec = sectors + f->affectee;


	if (f->roverfriction)
		referrer = sectors + f->referrer;









	node = sec->touching_thinglist;  
	while (node)
	{
		thing = node->m_thing;



		if (!(thing->flags & (MF_NOGRAVITY | MF_NOCLIP)) && thing->z == thing->floorz)
		{
			if (f->roverfriction)
			{
				if (thing->floorz != P_GetSpecialTopZ(thing, referrer, sec))
				{
					node = node->m_thinglist_next;
					continue;
				}

				if ((thing->friction == ORIG_FRICTION)  
					|| (f->friction < thing->friction))
				{
					thing->friction = f->friction;
					if (thing->player)
						thing->movefactor = f->movefactor;
				}
			}
			else if (P_GetSpecialBottomZ(thing, sec, sec) == thing->floorz && (thing->friction == ORIG_FRICTION  
				|| f->friction < thing->friction))
			{
				thing->friction = f->friction;
				if (thing->player)
					thing->movefactor = f->movefactor;
			}
		}
		node = node->m_thinglist_next;
	}
}

                                
   
                                     
    
static void P_SpawnFriction(void)
{
	size_t i;
	sector_t *s = sectors;

	fixed_t friction;  
	INT32 movefactor;  

	for (i = 0; i < numsectors; i++, s++)
	{
		if (s->friction == ORIG_FRICTION)
			continue;

		friction = s->friction;

		if (friction > FRACUNIT)
			friction = FRACUNIT;
		if (friction < 0)
			friction = 0;

		movefactor = FixedDiv(ORIG_FRICTION, friction);
		if (movefactor < FRACUNIT)
			movefactor = 8*movefactor - 7*FRACUNIT;
		else
			movefactor = FRACUNIT;

		Add_Friction(friction, movefactor, (INT32)(s-sectors), -1);

	}
}

  
                                                 
            
               
          
                
               
  

#define PUSH_FACTOR 7

                  
   
                                             
                                
                                
                                
                                  
                                      
                                                
    
static void Add_Pusher(pushertype_e type, fixed_t x_mag, fixed_t y_mag, fixed_t z_mag, INT32 affectee, INT32 referrer, INT32 exclusive, INT32 slider)
{
	pusher_t *p = Z_Calloc(sizeof *p, PU_LEVSPEC, NULL);

	p->thinker.function.acp1 = (actionf_p1)T_Pusher;
	p->type = type;
	p->x_mag = x_mag;
	p->y_mag = y_mag;
	p->z_mag = z_mag;
	p->exclusive = exclusive;
	p->slider = slider;

	if (referrer != -1)
	{
		p->roverpusher = true;
		p->referrer = referrer;
		sectors[referrer].specialflags |= SSF_WINDCURRENT;
	}
	else
	{
		p->roverpusher = false;
		sectors[affectee].specialflags |= SSF_WINDCURRENT;
	}

	p->affectee = affectee;
	P_AddThinker(THINK_MAIN, &p->thinker);
}

                                             
   
                                           
                                           
                                 
    
void T_Pusher(pusher_t *p)
{
	sector_t *sec, *referrer = NULL;
	mobj_t *thing;
	msecnode_t *node;
	fixed_t x_mag, y_mag, z_mag;
	fixed_t xspeed = 0, yspeed = 0, zspeed = 0;
	boolean inFOF;
	boolean touching;
	boolean moved;

	x_mag = p->x_mag >> PUSH_FACTOR;
	y_mag = p->y_mag >> PUSH_FACTOR;
	z_mag = p->z_mag >> PUSH_FACTOR;

	sec = sectors + p->affectee;
	if (p->roverpusher)
		referrer = sectors + p->referrer;




















	node = sec->touching_thinglist;  
	for (; node; node = node->m_thinglist_next)
	{
		thing = node->m_thing;
		if (thing->flags & (MF_NOGRAVITY | MF_NOCLIP)
			&& !(thing->type == MT_SMALLBUBBLE
			|| thing->type == MT_MEDIUMBUBBLE
			|| thing->type == MT_EXTRALARGEBUBBLE))
			continue;

		if (!((thing->flags & MF_PUSHABLE) || ((thing->info->flags & MF_PUSHABLE) && thing->fuse))
			&& !(thing->type == MT_PLAYER
			|| thing->type == MT_SMALLBUBBLE
			|| thing->type == MT_MEDIUMBUBBLE
			|| thing->type == MT_EXTRALARGEBUBBLE
			|| thing->type == MT_LITTLETUMBLEWEED
			|| thing->type == MT_BIGTUMBLEWEED))
			continue;

		if (thing->eflags & MFE_PUSHED)
			continue;

		if (thing->player && thing->player->powers[pw_carry] == CR_ROPEHANG)
			continue;

		if (thing->player && (thing->state == &states[thing->info->painstate]) && (thing->player->powers[pw_flashing] > (flashingtics/4)*3 && thing->player->powers[pw_flashing] <= flashingtics))
			continue;

		inFOF = touching = moved = false;


		if (p->roverpusher)
		{
			fixed_t top, bottom;

			top = P_GetSpecialTopZ(thing, referrer, sec);
			bottom = P_GetSpecialBottomZ(thing, referrer, sec);

			if (thing->eflags & MFE_VERTICALFLIP)
			{
				if (bottom > thing->z + thing->height
					|| top < (thing->z + (thing->height >> 1)))
					continue;

				if (thing->z < bottom)
					touching = true;

				if (thing->z + (thing->height >> 1) > bottom)
					inFOF = true;

			}
			else
			{
				if (top < thing->z || bottom > (thing->z + (thing->height >> 1)))
					continue;
				if (thing->z + thing->height > top)
					touching = true;

				if (thing->z + (thing->height >> 1) < top)
					inFOF = true;
			}
		}
		else  
		{
			if (thing->z == P_GetSpecialBottomZ(thing, sec, sec))
				touching = true;


			else if (udmf || p->type != p_current || z_mag != 0)
				inFOF = true;
		}

		if (!touching && !inFOF)  
			continue;

		if (inFOF || (p->type == p_current && touching))
		{
			xspeed = x_mag;  
			yspeed = y_mag;
			zspeed = z_mag;
			moved = true;
		}
		else if (p->type == p_wind && touching)
		{
			xspeed = x_mag>>1;  
			yspeed = y_mag>>1;
			zspeed = z_mag>>1;
			moved = true;
		}

		thing->momx += xspeed;
		thing->momy += yspeed;
		thing->momz += zspeed;
		if (thing->player)
		{
			thing->player->cmomx += xspeed;
			thing->player->cmomy += yspeed;
			thing->player->cmomx = FixedMul(thing->player->cmomx, ORIG_FRICTION);
			thing->player->cmomy = FixedMul(thing->player->cmomy, ORIG_FRICTION);
		}


		if (thing->type == MT_LITTLETUMBLEWEED || thing->type == MT_BIGTUMBLEWEED)
			thing->momz += P_AproxDistance(xspeed, yspeed) >> 2;

		if (moved)
		{
			if (p->slider && thing->player)
			{
				pflags_t jumped = (thing->player->pflags & (PF_JUMPED|PF_NOJUMPDAMAGE));
				P_ResetPlayer (thing->player);

				if (jumped)
					thing->player->pflags |= jumped;

				thing->player->pflags |= PF_SLIDING;
				thing->angle = R_PointToAngle2(0, 0, xspeed, yspeed);

				if (!demoplayback || P_ControlStyle(thing->player) == CS_LMAOGALOG)
				{
					angle_t angle = thing->player->angleturn << 16;
					if (thing->angle - angle > ANGLE_180)
						P_SetPlayerAngle(thing->player, angle - (angle - thing->angle) / 8);
					else
						P_SetPlayerAngle(thing->player, angle + (thing->angle - angle) / 8);

				}
			}

			if (p->exclusive)
				thing->eflags |= MFE_PUSHED;
		}
	}
}

                   
   
                                   
    
static void P_SpawnPushers(void)
{
	size_t i;
	line_t *l = lines;
	register INT32 s;
	fixed_t length, hspeed, dx, dy;

	for (i = 0; i < numlines; i++, l++)
	{
		if (l->special != 541)
			continue;

		length = R_PointToDist2(l->v2->x, l->v2->y, l->v1->x, l->v1->y);
		hspeed = l->args[1] << FRACBITS;
		dx = FixedMul(FixedDiv(l->dx, length), hspeed);
		dy = FixedMul(FixedDiv(l->dy, length), hspeed);

		if (l->args[0] == 0)
			Add_Pusher(l->args[3], dx, dy, l->args[2] << FRACBITS, (INT32)(l->frontsector - sectors), -1, !(l->args[4] & TMPF_NONEXCLUSIVE), !!(l->args[4] & TMPF_SLIDE));
		else
		{
			TAG_ITER_SECTORS(l->args[0], s)
				Add_Pusher(l->args[3], dx, dy, l->args[2] << FRACBITS, s, -1, !(l->args[4] & TMPF_NONEXCLUSIVE), !!(l->args[4] & TMPF_SLIDE));
		}
	}
}
