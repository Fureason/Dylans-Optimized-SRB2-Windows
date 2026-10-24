// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "r_defs.h"
#include "d_player.h"
#include "s_sound.h"
#include "d_event.h"
#include "lua_hudlib_drawlist.h"
#include "command.h"

extern consvar_t cv_luahitchlog;

  
                                                                             
                                                                              
                                                                          
              
  

#define MOBJ_HOOK_LIST(X) \
	X (MobjSpawn), \
	X (MobjCollide), \
	X (MobjLineCollide), \
	X (MobjMoveCollide), \
	X (TouchSpecial), \
	X (MobjFuse), \
	X (MobjThinker), \
	X (BossThinker), \
	X (ShouldDamage), \
	X (MobjDamage), \
	X (MobjDeath), \
	X (BossDeath), \
	X (MobjRemoved), \
	X (BotRespawn), \
	X (MobjMoveBlocked), \
	X (MapThingSpawn), \
	X (FollowMobj), \
	X (HurtMsg), \

#define HOOK_LIST(X) \
	X (NetVars), \
	X (MapChange), \
	X (MapLoad),\
	X (PlayerJoin), \
	X (PreThinkFrame) ,\
	X (ThinkFrame), \
	X (PostThinkFrame), \
	X (JumpSpecial), \
	X (AbilitySpecial), \
	X (SpinSpecial), \
	X (JumpSpinSpecial), \
	X (BotTiccmd), \
	X (PlayerMsg), \
	X (PlayerSpawn), \
	X (ShieldSpawn), \
	X (ShieldSpecial), \
	X (PlayerCanDamage), \
	X (PlayerQuit),\
	X (IntermissionThinker), \
	X (TeamSwitch), \
	X (ViewpointSwitch), \
	X (SeenPlayer), \
	X (PlayerThink), \
	X (GameQuit),\
	X (PlayerCmd), \
	X (MusicChange),\
	X (PlayerHeight), \
	X (PlayerCanEnterSpinGaps),\
	X (AddonLoaded),\
	X (KeyDown),\
	X (KeyUp),\

#define STRING_HOOK_LIST(X) \
	X (BotAI), \
	X (LinedefExecute),\
	X (ShouldJingleContinue), \

#define HUD_HOOK_LIST(X) \
	X (game),\
	X (scores), \
	X (title), \
	X (titlecard),\
	X (intermission),\
	X (continue),\
	X (playersetup),\

  
                                                                            
                                                                         
                                                                          
                                                                            
                                     
  

#define   MOBJ_HOOK(name)   mobjhook_ ## name
#define        HOOK(name)       hook_ ## name
#define    HUD_HOOK(name)    hudhook_ ## name
#define STRING_HOOK(name) stringhook_ ## name

#define ENUM(X) enum { X ## _LIST (X)  X(MAX) }

ENUM   (MOBJ_HOOK);
ENUM        (HOOK);
ENUM    (HUD_HOOK);
ENUM (STRING_HOOK);

#undef ENUM

 
#define LUA_HOOK(type) LUA_HookVoid(HOOK(type))
#define LUA_HUDHOOK(type,drawlist) LUA_HookHUD(HUD_HOOK(type),(drawlist))

extern boolean hook_cmd_running;

void LUA_HookVoid(int hook);
void LUA_HookHUD(int hook, huddrawlist_h drawlist);
int LUA_HookCharacterHUD
(
	int hook, huddrawlist_h drawlist, player_t *player,
	fixed_t x, fixed_t y, fixed_t scale,
	INT32 skinIndex, UINT8 sprite2, UINT8 frame, UINT8 rotation, skincolornum_t color,
	INT32 ticker, boolean mode
);

int  LUA_HookMobj(mobj_t *, int hook);
boolean LUA_HookMobjAvailable(mobjtype_t type, int hook);
int  LUA_Hook2Mobj(mobj_t *, mobj_t *, int hook);
void LUA_HookInt(INT32 integer, int hook);
void LUA_HookBool(boolean value, int hook);
int  LUA_HookPlayer(player_t *, int hook);
int  LUA_HookTiccmd(player_t *, ticcmd_t *, int hook);
int  LUA_HookKey(event_t *event, int hook);  

void LUA_HookPreThinkFrame(void);
void LUA_HookThinkFrame(void);
void LUA_HookPostThinkFrame(void);
int  LUA_HookMobjLineCollide(mobj_t *, line_t *);
int  LUA_HookTouchSpecial(mobj_t *special, mobj_t *toucher);
int  LUA_HookShouldDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype);
int  LUA_HookMobjDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype);
int  LUA_HookMobjDeath(mobj_t *target, mobj_t *inflictor, mobj_t *source, UINT8 damagetype);
int  LUA_HookMobjMoveBlocked(mobj_t *, mobj_t *, line_t *);
int  LUA_HookBotAI(mobj_t *sonic, mobj_t *tails, ticcmd_t *cmd);
void LUA_HookLinedefExecute(line_t *, mobj_t *, sector_t *);
int  LUA_HookPlayerMsg(int source, int target, int flags, char *msg);
int  LUA_HookHurtMsg(player_t *, mobj_t *inflictor, mobj_t *source, UINT8 damagetype);
int  LUA_HookMapThingSpawn(mobj_t *, mapthing_t *);
int  LUA_HookFollowMobj(player_t *, mobj_t *);
int  LUA_HookPlayerCanDamage(player_t *, mobj_t *);
void LUA_HookPlayerQuit(player_t *, kickreason_t);
int  LUA_HookTeamSwitch(player_t *, int newteam, boolean fromspectators, boolean tryingautobalance, boolean tryingscramble);
int  LUA_HookViewpointSwitch(player_t *player, player_t *newdisplayplayer, boolean forced);
int  LUA_HookSeenPlayer(player_t *player, player_t *seenfriend);
int  LUA_HookShouldJingleContinue(player_t *, const char *musname);
int  LUA_HookPlayerCmd(player_t *, ticcmd_t *);
int  LUA_HookMusicChange(const char *oldname, struct MusicChange *);
fixed_t LUA_HookPlayerHeight(player_t *player);
int  LUA_HookPlayerCanEnterSpinGaps(player_t *player);
