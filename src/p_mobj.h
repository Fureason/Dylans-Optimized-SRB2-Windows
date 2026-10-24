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



#ifndef __P_MOBJ__
#define __P_MOBJ__


#include "tables.h"
#include "m_fixed.h"


#include "d_think.h"


#include "doomdata.h"



#include "info.h"




































































typedef enum
{

	MF_SPECIAL          = 1,

	MF_SOLID            = 1<<1,

	MF_SHOOTABLE        = 1<<2,

	MF_NOSECTOR         = 1<<3,

	MF_NOBLOCKMAP       = 1<<4,

	MF_PAPERCOLLISION            = 1<<5,

	MF_PUSHABLE         = 1<<6,

	MF_BOSS             = 1<<7,

	MF_SPAWNCEILING     = 1<<8,


	MF_NOGRAVITY        = 1<<9,

	MF_AMBIENT          = 1<<10,

	MF_SLIDEME          = 1<<11,

	MF_NOCLIP           = 1<<12,

	MF_FLOAT            = 1<<13,

	MF_BOXICON          = 1<<14,


	MF_MISSILE          = 1<<15,

	MF_SPRING           = 1<<16,

	MF_BOUNCE           = 1<<17,

	MF_MONITOR          = 1<<18,

	MF_NOTHINK          = 1<<19,

	MF_FIRE             = 1<<20,

	MF_NOCLIPHEIGHT     = 1<<21,

	MF_ENEMY            = 1<<22,

	MF_SCENERY          = 1<<23,

	MF_PAIN             = 1<<24,

	MF_STICKY           = 1<<25,

	MF_NIGHTSITEM       = 1<<26,

	MF_NOCLIPTHING      = 1<<27,

	MF_GRENADEBOUNCE    = 1<<28,

	MF_RUNSPAWNFUNC     = 1<<29,

} mobjflag_t;

typedef enum
{
	MF2_AXIS           = 1,      
	MF2_TWOD           = 1<<1,   
	MF2_DONTRESPAWN    = 1<<2,   
	MF2_DONTDRAW       = 1<<3,   
	MF2_AUTOMATIC      = 1<<4,   
	MF2_RAILRING       = 1<<5,   
	MF2_BOUNCERING     = 1<<6,   
	MF2_EXPLOSION      = 1<<7,   
	MF2_SCATTER        = 1<<8,   
	MF2_BEYONDTHEGRAVE = 1<<9,   
	MF2_SLIDEPUSH      = 1<<10,  
	MF2_CLASSICPUSH    = 1<<11,  
	MF2_INVERTAIMABLE  = 1<<12,  
	MF2_INFLOAT        = 1<<13,  
	MF2_DEBRIS         = 1<<14,  
	MF2_NIGHTSPULL     = 1<<15,  
	MF2_JUSTATTACKED   = 1<<16,  
	MF2_FIRING         = 1<<17,  
	MF2_SUPERFIRE      = 1<<18,  
	MF2_SHADOW         = 1<<19,  
	MF2_STRONGBOX      = 1<<20,  
	MF2_OBJECTFLIP     = 1<<21,  
	MF2_SKULLFLY       = 1<<22,  
	MF2_FRET           = 1<<23,  
	MF2_BOSSNOTRAP     = 1<<24,  
	MF2_BOSSFLEE       = 1<<25,  
	MF2_BOSSDEAD       = 1<<26,  
	MF2_AMBUSH         = 1<<27,  
	MF2_LINKDRAW       = 1<<28,  
	MF2_SHIELD         = 1<<29,  
	MF2_SPLAT          = 1<<30,  

} mobjflag2_t;

typedef enum
{
	DI_NODIR = -1,
	DI_EAST = 0,
	DI_NORTHEAST = 1,
	DI_NORTH = 2,
	DI_NORTHWEST = 3,
	DI_WEST = 4,
	DI_SOUTHWEST = 5,
	DI_SOUTH = 6,
	DI_SOUTHEAST = 7,
	NUMDIRS = 8,
} dirtype_t;




typedef enum
{

	MFE_ONGROUND			= 1,


	MFE_JUSTHITFLOOR		= 1<<1,


	MFE_TOUCHWATER			= 1<<2,


	MFE_UNDERWATER			= 1<<3,

	MFE_JUSTSTEPPEDDOWN		= 1<<4,

	MFE_VERTICALFLIP		= 1<<5,

	MFE_GOOWATER			= 1<<6,

	MFE_TOUCHLAVA			= 1<<7,

	MFE_PUSHED				= 1<<8,

	MFE_SPRUNG				= 1<<9,

	MFE_APPLYPMOMZ			= 1<<10,


	MFE_TRACERANGLE			= 1<<11,

	MFE_FORCESUPER			= 1<<12,

	MFE_FORCENOSUPER		= 1<<13,

	MFE_REVERSESUPER		= MFE_FORCESUPER|MFE_FORCENOSUPER


} mobjeflag_t;




typedef enum {

	PCF_INVISIBLE = 1,

	PCF_PIT = 2,

	PCF_FOF = 4,

	PCF_MOVINGFOF = 8,

	PCF_RAIN = 16,

	PCF_THUNK = 32,
} precipflag_t;


typedef struct blocknode_s
{
	struct mobj_s *mobj;

	int blockindex;      

	struct blocknode_s **mprev;  
	struct blocknode_s *mnext;   
	struct blocknode_s **bprev;  
	struct blocknode_s *bnext;   
} blocknode_t;


typedef struct mobj_s
{

	thinker_t thinker;


	fixed_t x, y, z;
	fixed_t old_x, old_y, old_z;  
	fixed_t old_x2, old_y2, old_z2;


	struct mobj_s *snext;
	struct mobj_s **sprev;  


	angle_t angle, pitch, roll;  
	angle_t old_angle, old_pitch, old_roll;  
	angle_t old_angle2, old_pitch2, old_roll2;
	angle_t spriteroll, old_spriteroll, old_spriteroll2;
	spritenum_t sprite;  
	UINT32 frame;  
	UINT16 sprite2;  
	UINT16 anim_duration;  

	UINT32 renderflags;  
	INT32 blendmode;  
	fixed_t alpha;  
	fixed_t spritexscale, spriteyscale;
	fixed_t spritexoffset, spriteyoffset;
	fixed_t old_spritexscale, old_spriteyscale, old_spritexscale2, old_spriteyscale2;
	fixed_t old_spritexoffset, old_spriteyoffset, old_spritexoffset2, old_spriteyoffset2;
	struct pslope_s *floorspriteslope;  

	struct msecnode_s *touching_sectorlist;  

	struct subsector_s *subsector;  


	fixed_t floorz;  
	fixed_t ceilingz;  
	struct ffloor_s *floorrover;  
	struct ffloor_s *ceilingrover;  


	fixed_t radius;
	fixed_t height;


	fixed_t momx, momy, momz;
	fixed_t pmomz;  

	INT32 tics;  
	state_t *state;
	UINT32 flags;  
	UINT32 flags2;  
	UINT16 eflags;  

	void *skin;  



	UINT16 color;


	UINT16 translation;

	struct player_s *drawonlyforplayer;  
	struct mobj_s *dontdrawforviewmobj;  



	blocknode_t *blocknode;


	struct mobj_s *hnext;
	struct mobj_s *hprev;

	mobjtype_t type;
	const mobjinfo_t *info;  

	INT32 health;  


	angle_t movedir;  
	INT32 movecount;  

	struct mobj_s *target;  

	INT32 reactiontime;  

	INT32 threshold;  



	struct player_s *player;

	INT32 lastlook;  

	mapthing_t *spawnpoint;  

	struct mobj_s *tracer;  

	fixed_t friction;
	fixed_t movefactor;

	INT32 fuse;  
	fixed_t watertop;  
	fixed_t waterbottom;  

	UINT32 mobjnum;  

	fixed_t scale;
	fixed_t old_scale;  
	fixed_t old_scale2;
	fixed_t destscale;
	fixed_t scalespeed;


	INT32 extravalue1;
	INT32 extravalue2;



	INT32 cusval;
	INT32 cvmem;

	struct pslope_s *standingslope;  

	boolean resetinterp;  
	boolean colorized;  
	boolean mirrored;  
	fixed_t shadowscale;  
	INT32 dispoffset;  


} mobj_t;








typedef struct precipmobj_s
{

	thinker_t thinker;


	fixed_t x, y, z;
	fixed_t old_x, old_y, old_z;  
	fixed_t old_x2, old_y2, old_z2;


	struct precipmobj_s *snext;
	struct precipmobj_s **sprev;  


	angle_t angle, pitch, roll;  
	angle_t old_angle, old_pitch, old_roll;  
	angle_t old_angle2, old_pitch2, old_roll2;
	angle_t spriteroll, old_spriteroll, old_spriteroll2;
	spritenum_t sprite;  
	UINT32 frame;  
	UINT16 sprite2;  
	UINT16 anim_duration;  

	UINT32 renderflags;  
	INT32 blendmode;  
	fixed_t alpha;  
	fixed_t spritexscale, spriteyscale;
	fixed_t spritexoffset, spriteyoffset;
	fixed_t old_spritexscale, old_spriteyscale, old_spritexscale2, old_spriteyscale2;
	fixed_t old_spritexoffset, old_spriteyoffset, old_spritexoffset2, old_spriteyoffset2;
	struct pslope_s *floorspriteslope;  

	struct mprecipsecnode_s *touching_sectorlist;  

	struct subsector_s *subsector;  


	fixed_t floorz;  
	fixed_t ceilingz;  
	struct ffloor_s *floorrover;  
	struct ffloor_s *ceilingrover;  


	fixed_t radius;  
	fixed_t height;  


	fixed_t momx, momy, momz;
	fixed_t precipflags;  

	INT32 tics;  
	state_t *state;
	INT32 flags;  

	tic_t lastupdatetime;
} precipmobj_t;

typedef struct actioncache_s
{
	struct actioncache_s *next;
	struct actioncache_s *prev;
	struct mobj_s *mobj;
	INT32 statenum;
} actioncache_t;

extern actioncache_t actioncachehead;

void P_InitCachedActions(void);
void P_RunCachedActions(void);
void P_AddCachedAction(mobj_t *mobj, INT32 statenum);


void P_MobjCheckWater(mobj_t *mobj);


void P_SpawnPlayer(INT32 playernum);
void P_MovePlayerToSpawn(INT32 playernum, mapthing_t *mthing);
void P_MovePlayerToStarpost(INT32 playernum);
void P_AfterPlayerSpawn(INT32 playernum);

fixed_t P_GetMobjSpawnHeight(const mobjtype_t mobjtype, const fixed_t x, const fixed_t y, const fixed_t dz, const fixed_t offset, const boolean flip, const fixed_t scale, const boolean absolutez);
fixed_t P_GetMapThingSpawnHeight(const mobjtype_t mobjtype, const mapthing_t* mthing, const fixed_t x, const fixed_t y);

mobj_t *P_SpawnMapThing(mapthing_t *mthing);
void P_SpawnHoop(mapthing_t *mthing);
void P_SetBonusTime(mobj_t *mobj);
void P_SpawnItemPattern(mapthing_t *mthing, boolean bonustime);
void P_SpawnHoopOfSomething(fixed_t x, fixed_t y, fixed_t z, fixed_t radius, INT32 number, mobjtype_t type, angle_t rotangle);
void P_SpawnPrecipitation(void);
void P_SpawnParaloop(fixed_t x, fixed_t y, fixed_t z, fixed_t radius, INT32 number, mobjtype_t type, statenum_t nstate, angle_t rotangle, boolean spawncenter);
void *P_CreateFloorSpriteSlope(mobj_t *mobj);
void P_RemoveFloorSpriteSlope(mobj_t *mobj);
boolean P_BossTargetPlayer(mobj_t *actor, boolean closest);
boolean P_SupermanLook4Players(mobj_t *actor);
void P_DestroyRobots(void);
void P_SnowThinker(precipmobj_t *mobj);
void P_RainThinker(precipmobj_t *mobj);
void P_NullPrecipThinker(precipmobj_t *mobj);
void P_RemovePrecipMobj(precipmobj_t *mobj);
void P_SetScale(mobj_t *mobj, fixed_t newscale, boolean instant);
void P_XYMovement(mobj_t *mo);
void P_RingXYMovement(mobj_t *mo);
void P_SceneryXYMovement(mobj_t *mo);
boolean P_ZMovement(mobj_t *mo);
void P_RingZMovement(mobj_t *mo);
boolean P_SceneryZMovement(mobj_t *mo);
void P_PlayerZMovement(mobj_t *mo);
void P_EmeraldManager(void);

mobj_t *P_FindNewPosition(UINT32 oldposition);

extern INT32 modulothing;

#define MAXHUNTEMERALDS 64
extern mapthing_t *huntemeralds[MAXHUNTEMERALDS];
extern INT32 numhuntemeralds;
extern boolean runemeraldmanager;
extern UINT16 emeraldspawndelay;
extern INT32 numstarposts;
extern UINT16 bossdisabled;
extern boolean stoppedclock;
#endif
