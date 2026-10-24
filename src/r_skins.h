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



#ifndef __R_SKINS__
#define __R_SKINS__

#include "info.h"
#include "sounds.h"
#include "d_player.h"  
#include "r_patch.h"
#include "r_picformats.h"  
#include "r_defs.h"  


#define SKINNAMESIZE 16

#define DEFAULTSKIN "sonic"
#define DEFAULTSKIN2 "tails"  
#define DEFAULTNIGHTSSKIN 0


typedef struct
{
	char name[SKINNAMESIZE+1];  
	UINT8 skinnum;
	UINT16 wadnum;
	skinflags_t flags;

	char realname[SKINNAMESIZE+1];  
	char hudname[SKINNAMESIZE+1];  
	char supername[SKINNAMESIZE+7];  

	UINT8 ability;  
	UINT8 ability2;  
	INT32 thokitem;
	INT32 spinitem;
	INT32 revitem;
	INT32 followitem;
	fixed_t actionspd;
	fixed_t mindash;
	fixed_t maxdash;

	fixed_t normalspeed;  
	fixed_t runspeed;  

	UINT8 thrustfactor;  
	UINT8 accelstart;  
	UINT8 acceleration;  

	fixed_t jumpfactor;  

	fixed_t radius;  
	fixed_t height;
	fixed_t spinheight;

	fixed_t shieldscale;  
	fixed_t camerascale;


	UINT8 starttranscolor;
	UINT16 prefcolor;
	UINT16 supercolor;
	UINT16 prefoppositecolor;  
	UINT16 natkcolor;  

	fixed_t highresscale;  
	UINT8 contspeed;  
	UINT8 contangle;  


	sfxenum_t soundsid[NUMSKINSOUNDS];  

	spritedef_t sprites[NUMPLAYERSPRITES];
	spriteinfo_t sprinfo[NUMPLAYERSPRITES];


	struct {
		spritedef_t sprites[NUMPLAYERSPRITES];
		spriteinfo_t sprinfo[NUMPLAYERSPRITES];
	} super;


	spritedef_t sprites_compat[NUMPLAYERSPRITES * 2];
} skin_t;


extern INT32 numskins;
extern skin_t **skins;


void R_InitSkins(void);

INT32 GetPlayerDefaultSkin(INT32 playernum);
void SetPlayerSkin(INT32 playernum,const char *skinname);
void SetPlayerSkinByNum(INT32 playernum,INT32 skinnum);  
boolean R_SkinUsable(INT32 playernum, INT32 skinnum);
UINT32 R_GetSkinAvailabilities(void);
INT32 R_SkinAvailable(const char *name);
INT32 R_GetForcedSkin(INT32 playernum);
void R_AddSkins(UINT16 wadnum, boolean mainfile);
void R_PatchSkins(UINT16 wadnum, boolean mainfile);

UINT16 P_GetStateSprite2(state_t *state);
UINT16 P_GetSprite2StateFrame(state_t *state);
UINT16 P_GetSkinSprite2(skin_t *skin, UINT16 spr2, player_t *player);
UINT16 P_ApplySuperFlagToSprite2(UINT16 spr2, mobj_t *mobj);
spritedef_t *P_GetSkinSpritedef(skin_t *skin, UINT16 spr2);
spriteinfo_t *P_GetSkinSpriteInfo(skin_t *skin, UINT16 spr2);
boolean P_IsValidSprite2(skin_t *skin, UINT16 spr2);
boolean P_IsStateSprite2Super(state_t *state);

void R_RefreshSprite2(void);

#endif  
