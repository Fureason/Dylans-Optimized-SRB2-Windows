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



#ifndef __D_PLAYER__
#define __D_PLAYER__





#include "p_pspr.h"



#include "p_mobj.h"




#include "d_ticcmd.h"


typedef enum
{
	SF_SUPER            = 1,     
	SF_NOSUPERSPIN      = 1<<1,  
	SF_NOSPINDASHDUST   = 1<<2,  
	SF_HIRES            = 1<<3,  
	SF_NOSKID           = 1<<4,  
	SF_NOSPEEDADJUST    = 1<<5,  
	SF_RUNONWATER       = 1<<6,  
	SF_NOJUMPSPIN       = 1<<7,  
	SF_NOJUMPDAMAGE     = 1<<8,  
	SF_STOMPDAMAGE      = 1<<9,  
	SF_MARIODAMAGE      = SF_NOJUMPDAMAGE|SF_STOMPDAMAGE,  
	SF_MACHINE          = 1<<10,  
	SF_DASHMODE         = 1<<11,  
	SF_FASTWAIT         = 1<<12,  
	SF_FASTEDGE         = 1<<13,  
	SF_MULTIABILITY     = 1<<14,  
	SF_NONIGHTSROTATION = 1<<15,  
	SF_NONIGHTSSUPER    = 1<<16,  
	SF_NOSUPERSPRITES   = 1<<17,  
	SF_NOSUPERJUMPBOOST = 1<<18,  
	SF_CANBUSTWALLS     = 1<<19,  
	SF_NOSHIELDABILITY  = 1<<20,  


} skinflags_t;


typedef enum
{
	CA_NONE=0,
	CA_THOK,
	CA_FLY,
	CA_GLIDEANDCLIMB,
	CA_HOMINGTHOK,
	CA_SWIM,
	CA_DOUBLEJUMP,
	CA_FLOAT,
	CA_SLOWFALL,
	CA_TELEKINESIS,
	CA_FALLSWITCH,
	CA_JUMPBOOST,
	CA_AIRDRILL,
	CA_JUMPTHOK,
	CA_BOUNCE,
	CA_TWINSPIN
} charability_t;


typedef enum
{
	CA2_NONE=0,
	CA2_SPINDASH,
	CA2_GUNSLINGER,
	CA2_MELEE
} charability2_t;




typedef enum
{

	PST_LIVE,

	PST_DEAD,

	PST_REBORN
} playerstate_t;




typedef enum
{

	PF_FLIPCAM       = 1,  
	PF_ANALOGMODE    = 1<<1,  
	PF_DIRECTIONCHAR = 1<<2,  
	PF_AUTOBRAKE     = 1<<3,  


	PF_GODMODE = 1<<4,
	PF_NOCLIP  = 1<<5,
	PF_INVIS   = 1<<6,


	PF_ATTACKDOWN = 1<<7,
	PF_SPINDOWN   = 1<<8,
	PF_JUMPDOWN   = 1<<9,
	PF_WPNDOWN    = 1<<10,


	PF_STASIS     = 1<<11,  
	PF_JUMPSTASIS = 1<<12,  
	PF_FULLSTASIS = PF_STASIS|PF_JUMPSTASIS,


	PF_APPLYAUTOBRAKE = 1<<13,


	PF_STARTJUMP     = 1<<14,
	PF_JUMPED        = 1<<15,
	PF_NOJUMPDAMAGE  = 1<<16,

	PF_SPINNING      = 1<<17,
	PF_STARTDASH     = 1<<18,

	PF_THOKKED       = 1<<19,
	PF_SHIELDABILITY = 1<<20,
	PF_GLIDING       = 1<<21,
	PF_BOUNCING      = 1<<22,


	PF_SLIDING       = 1<<23,


	PF_TRANSFERTOCLOSEST = 1<<24,
	PF_DRILLING          = 1<<25,


	PF_GAMETYPEOVER = 1<<26,  
	PF_TAGIT        = 1<<27,  

	 
	PF_FORCESTRAFE = 1<<28,  
	PF_CANCARRY    = 1<<29,  
	PF_FINISHED    = 1<<30,  


} pflags_t;

typedef enum
{

	PA_ETC=0,
	PA_IDLE,
	PA_EDGE,
	PA_WALK,
	PA_RUN,
	PA_DASH,
	PA_PAIN,
	PA_ROLL,
	PA_JUMP,
	PA_SPRING,
	PA_FALL,
	PA_ABILITY,
	PA_ABILITY2,
	PA_RIDE
} panim_t;









typedef enum
{
	SH_NONE = 0,


	SH_PROTECTFIRE = 0x400,
	SH_PROTECTWATER = 0x800,
	SH_PROTECTELECTRIC = 0x1000,
	SH_PROTECTSPIKE = 0x2000,  



	SH_PITY = 1,  
	SH_WHIRLWIND,
	SH_ARMAGEDDON,
	SH_PINK,  


	SH_ATTRACT = SH_PITY|SH_PROTECTELECTRIC,
	SH_ELEMENTAL = SH_PITY|SH_PROTECTFIRE|SH_PROTECTWATER,


	SH_FLAMEAURA = SH_PITY|SH_PROTECTFIRE,
	SH_BUBBLEWRAP = SH_PITY|SH_PROTECTWATER,
	SH_THUNDERCOIN = SH_WHIRLWIND|SH_PROTECTELECTRIC,


	SH_FORCE = 0x100,
	SH_FORCEHP = 0xFF,  


	SH_FIREFLOWER = 0x200,

	SH_STACK = SH_FIREFLOWER,  
	SH_NOSTACK = ~SH_STACK
} shieldtype_t;  

typedef enum
{
	CR_NONE = 0,

	CR_GENERIC,

	CR_PLAYER,

	CR_NIGHTSMODE,
	CR_NIGHTSFALL,

	CR_BRAKGOOP,

	CR_ZOOMTUBE,
	CR_ROPEHANG,
	CR_MACESPIN,
	CR_MINECART,
	CR_ROLLOUT,
	CR_PTERABYTE,
	CR_DUSTDEVIL,
	CR_FAN
} carrytype_t;  

typedef enum
{
	STR_NONE = 0,  


	STR_ANIM = 0x1,  
	STR_PUNCH = 0x2,  
	STR_TAIL = 0x4,  
	STR_STOMP = 0x8,  
	STR_UPPER = 0x10,  
	STR_GUARD = 0x20,  
	STR_HEAVY = 0x40,  
	STR_DASH = 0x80,  


	STR_WALL = 0x100,  
	STR_FLOOR = 0x200,
	STR_CEILING = 0x400,
	STR_SPRING = 0x800,  
	STR_SPIKE = 0x1000,  


	STR_ATTACK = STR_PUNCH|STR_TAIL|STR_STOMP|STR_UPPER,
	STR_BUST = STR_WALL|STR_FLOOR|STR_CEILING,
	STR_FLY = STR_ANIM|STR_UPPER,
	STR_GLIDE = STR_ANIM|STR_PUNCH,
	STR_TWINSPIN = STR_ANIM|STR_ATTACK|STR_BUST|STR_SPRING|STR_SPIKE,
	STR_MELEE = STR_ANIM|STR_PUNCH|STR_HEAVY|STR_WALL|STR_FLOOR|STR_SPRING|STR_SPIKE,
	STR_BOUNCE = STR_ANIM|STR_STOMP|STR_FLOOR,
	STR_METAL = STR_DASH|STR_SPIKE
} strongtype_t;  


typedef enum
{
	pw_invulnerability,
	pw_sneakers,
	pw_flashing,
	pw_shield,
	pw_carry,
	pw_tailsfly,  
	pw_underwater,  
	pw_spacetime,  
	pw_extralife,  
	pw_pushing,
	pw_justsprung,
	pw_noautobrake,

	pw_super,  
	pw_gravityboots,  


	pw_infinityring,
	pw_automaticring,
	pw_bouncering,
	pw_scatterring,
	pw_grenadering,
	pw_explosionring,
	pw_railring,


	pw_emeralds,  


	pw_nights_superloop,
	pw_nights_helper,
	pw_nights_linkfreeze,

	pw_nocontrol,  

	pw_dye,  

	pw_justlaunched,  

	pw_ignorelatch,  

	pw_strong,  

	NUMPOWERS
} powertype_t;

#define WEP_AUTO    1
#define WEP_BOUNCE  2
#define WEP_SCATTER 3
#define WEP_GRENADE 4
#define WEP_EXPLODE 5
#define WEP_RAIL    6
#define NUM_WEAPONS 7

typedef enum
{
	RW_AUTO    =  1,
	RW_BOUNCE  =  2,
	RW_SCATTER =  4,
	RW_GRENADE =  8,
	RW_EXPLODE = 16,
	RW_RAIL    = 32
} ringweapons_t;


typedef enum
{
	BOT_NONE = 0,
	BOT_2PAI,
	BOT_2PHUMAN,
	BOT_MPAI
} bottype_t;


typedef enum
{
	AI_STANDBY = 0,
	AI_FOLLOW,
	AI_CATCHUP,
	AI_THINKFLY,
	AI_FLYSTANDBY,
	AI_FLYCARRY,
	AI_SPINFOLLOW
} aistatetype_t;


typedef enum
{
	NTV_NONE = 0,
	NTV_GETSPHERES,
	NTV_GETMORESPHERES,
	NTV_BONUSTIMESTART,
	NTV_BONUSTIMEEND,
} nightstextvar_t;







typedef struct botmem_s
{
	boolean lastForward;
	boolean lastBlocked;
	boolean blocked;
	UINT8 catchup_tics;
	UINT8 thinkstate;
} botmem_t;


typedef struct player_s
{
	mobj_t *mo;


	ticcmd_t cmd;

	playerstate_t playerstate;


	fixed_t camerascale;
	fixed_t shieldscale;

	fixed_t viewz;

	fixed_t viewheight;

	fixed_t deltaviewheight;

	fixed_t bob;

	angle_t viewrollangle;

	INT16 angleturn;
	INT16 oldrelangleturn;



	angle_t aiming;


	angle_t drawangle;
	angle_t old_drawangle;
	angle_t old_drawangle2;


	INT16 rings;
	INT16 spheres;

	SINT8 pity;  
	INT32 currentweapon;  
	INT32 ringweapons;  

	UINT16 ammoremoval;  
	tic_t  ammoremovaltimer;  
	INT32  ammoremovalweapon;  


	UINT16 powers[NUMPOWERS];



	pflags_t pflags;


	panim_t panim;
	UINT8 stronganim;


	UINT16 flashcount;
	UINT16 flashpal;


	UINT16 skincolor;

	UINT8 skin;
	UINT32 availabilities;

	UINT32 score;  
	UINT32 recordscore;  
	fixed_t dashspeed;  

	fixed_t normalspeed;  
	fixed_t runspeed;  
	UINT8 thrustfactor;  
	UINT8 accelstart;  
	UINT8 acceleration;  


	UINT8 charability;  
	UINT8 charability2;  

	UINT32 charflags;  


	mobjtype_t thokitem;  
	mobjtype_t spinitem;  
	mobjtype_t revitem;  
	mobjtype_t followitem;  
	mobj_t *followmobj;  

	fixed_t actionspd;  
	fixed_t mindash;  
	fixed_t maxdash;  

	fixed_t jumpfactor;  

	fixed_t height;  
	fixed_t spinheight;

	SINT8 lives;  
	SINT8 continues;  

	SINT8 xtralife;  
	UINT8 gotcontinue;  

	fixed_t speed;  
	UINT8 secondjump;  

	UINT8 fly1;  
	UINT8 scoreadd;  
	tic_t glidetime;  
	UINT8 climbing;  
	INT32 deadtimer;  
	tic_t exiting;  

	UINT8 homing;  
	tic_t dashmode;  

	tic_t skidtime;  




	fixed_t cmomx;  
	fixed_t cmomy;  
	fixed_t rmomx;  
	fixed_t rmomy;  




	INT16 numboxes;  
	INT16 totalring;  
	tic_t realtime;  
	UINT8 laps;  




	INT32 ctfteam;  
	UINT16 gotflag;  

	INT32 weapondelay;  
	INT32 tossdelay;    


	INT16 starpostx;
	INT16 starposty;
	INT16 starpostz;
	INT32 starpostnum;  
	tic_t starposttime;  
	angle_t starpostangle;  
	fixed_t starpostscale;  




	angle_t angle_pos;
	angle_t old_angle_pos;

	mobj_t *axis1;
	mobj_t *axis2;
	tic_t bumpertime;  
	INT32 flyangle;
	tic_t drilltimer;
	INT32 linkcount;
	tic_t linktimer;
	INT32 anotherflyangle;
	tic_t nightstime;  
	INT32 drillmeter;
	UINT8 drilldelay;
	boolean bonustime;  
	mobj_t *capsule;  
	mobj_t *drone;  
	fixed_t oldscale;  
	UINT8 mare;  
	UINT8 marelap;  
	UINT8 marebonuslap;  


	tic_t marebegunat;  
	tic_t startedtime;  
	tic_t finishedtime;  
	tic_t lastmaretime;  
	tic_t lapbegunat;  
	tic_t lapstartedtime;  
	INT16 finishedspheres;  
	INT16 finishedrings;  
	UINT32 marescore;  
	UINT32 lastmarescore;  
	UINT32 totalmarescore;  
	UINT8 lastmare;  
	UINT8 lastmarelap;  
	UINT8 lastmarebonuslap;  
	UINT8 totalmarelap;  
	UINT8 totalmarebonuslap;  
	INT32 maxlink;  
	UINT8 texttimer;  
	UINT8 textvar;  

	INT16 lastsidehit, lastlinehit;

	tic_t losstime;
	UINT8 timeshit;  

	INT32 onconveyor;  

	mobj_t *awayviewmobj;
	INT32 awayviewtics;
	angle_t awayviewaiming;  

	boolean spectator;
	boolean outofcoop;
	boolean removing;
	boolean muted;
	UINT8 bot;
	struct player_s *botleader;
	UINT16 lastbuttons;
	botmem_t botmem;
	boolean blocked;

	tic_t jointime;  
	tic_t quittime;  
	tic_t lastinputtime;  
	fixed_t fovadd;  
} player_t;


#define DASHMODE_THRESHOLD (3*TICRATE)
#define DASHMODE_MAX (DASHMODE_THRESHOLD + 3)


#define INFLIVES 0x7F

#endif
