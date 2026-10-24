// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------





#ifndef __D_NETCMD__
#define __D_NETCMD__

#include "../command.h"


extern consvar_t cv_playername;
extern consvar_t cv_playercolor;
extern consvar_t cv_skin;

extern consvar_t cv_playername2;
extern consvar_t cv_playercolor2;
extern consvar_t cv_skin2;

extern consvar_t cv_defaultplayercolor;
extern consvar_t cv_defaultskin;
extern consvar_t cv_defaultplayercolor2;
extern consvar_t cv_defaultskin2;

extern consvar_t cv_seenames, cv_allowseenames;
extern consvar_t cv_commandwatch_originals;
extern consvar_t cv_usemouse;
extern consvar_t cv_usejoystick;
extern consvar_t cv_usejoystick2;
#ifdef LJOYSTICK
extern consvar_t cv_joyport;
extern consvar_t cv_joyport2;
#endif
extern consvar_t cv_joyscale;
extern consvar_t cv_joyscale2;


extern consvar_t cv_mouse2port;
extern consvar_t cv_usemouse2;
#if defined (__unix__) || defined (__APPLE__) || defined (UNIXCOMMON)
extern consvar_t cv_mouse2opt;
#endif


extern consvar_t cv_itemrespawntime;
extern consvar_t cv_itemrespawn;

extern consvar_t cv_flagtime;

extern consvar_t cv_touchtag;
extern consvar_t cv_hidetime;

extern consvar_t cv_friendlyfire;
extern consvar_t cv_pointlimit;
extern consvar_t cv_timelimit;
extern consvar_t cv_numlaps;
extern consvar_t cv_basenumlaps;
extern UINT32 timelimitintics;
extern consvar_t cv_allowexitlevel;

extern consvar_t cv_hazardlog;

extern consvar_t cv_autobalance;
extern consvar_t cv_teamscramble;
extern consvar_t cv_scrambleonchange;

extern consvar_t cv_netstat;
extern consvar_t cv_nettimeout;

extern consvar_t cv_countdowntime;
extern consvar_t cv_runscripts;
extern consvar_t cv_mute;
extern consvar_t cv_killingdead;
extern consvar_t cv_pause;

extern consvar_t cv_restrictskinchange, cv_allowteamchange, cv_respawntime;

extern consvar_t cv_teleporters, cv_superring, cv_supersneakers, cv_invincibility;
extern consvar_t cv_jumpshield, cv_watershield, cv_ringshield, cv_forceshield, cv_bombshield;
extern consvar_t cv_1up, cv_eggmanbox;
extern consvar_t cv_recycler;

extern consvar_t cv_itemfinder;

extern consvar_t cv_inttime, cv_coopstarposts, cv_cooplives, cv_advancemap, cv_playersforexit, cv_exitmove;
extern consvar_t cv_overtime;
extern consvar_t cv_startinglives;

extern consvar_t cv_gravity, cv_movebob;
extern consvar_t cv_tailspickup;


extern consvar_t cv_rollingdemos;

extern consvar_t cv_ringslinger, cv_soundtest;

extern consvar_t cv_specialrings, cv_powerstones, cv_matchboxes, cv_competitionboxes;

extern consvar_t cv_maxping;
extern consvar_t cv_pingtimeout;
extern consvar_t cv_showping;

extern consvar_t cv_skipmapcheck;

extern consvar_t cv_sleep;

extern consvar_t cv_perfstats;
extern consvar_t cv_ps_samplesize;
extern consvar_t cv_ps_descriptor;

extern char timedemo_name[256];
extern boolean timedemo_csv;
extern char timedemo_csv_id[256];
extern boolean timedemo_quit;

extern consvar_t cv_freedemocamera;

typedef enum
{
	XD_NAMEANDCOLOR = 1,
	XD_WEAPONPREF,   
	XD_KICK,         
	XD_NETVAR,       
	XD_SAY,          
	XD_MAP,          
	XD_EXITLEVEL,    
	XD_ADDFILE,      
	XD_ADDFOLDER,    
	XD_PAUSE,        
	XD_ADDPLAYER,    
	XD_TEAMCHANGE,   
	XD_CLEARSCORES,  
	XD_VERIFIED,     
	XD_RANDOMSEED,   
	XD_RUNSOC,       
	XD_REQADDFILE,   
	XD_REQADDFOLDER, 
	XD_SETMOTD,      
	XD_SUICIDE,      
	XD_DEMOTED,      
	XD_LUACMD,       
	XD_LUAVAR,       
	XD_LUAFILE,      
	XD_MUTEPLAYER,   
	MAXNETXCMD
} netxcmd_t;

extern const char *netxcmdnames[MAXNETXCMD - 1];

#if defined(_MSC_VER)
#pragma pack(1)
#endif

#ifdef _MSC_VER
#pragma warning(disable :  4214)
#endif




typedef struct {
	UINT32 playernum    : 5;   
	UINT32 newteam      : 5;   
	UINT32 verification : 1;   
	UINT32 autobalance  : 1;   
	UINT32 scrambled    : 1;   
} ATTRPACK changeteam_packet_t;

#ifdef _MSC_VER
#pragma warning(default : 4214)
#endif

typedef struct {
	UINT16 l;  
	UINT16 b;  
} ATTRPACK changeteam_value_t;




typedef union {
	changeteam_packet_t packet;
	changeteam_value_t value;
} ATTRPACK changeteam_union;

#if defined(_MSC_VER)
#pragma pack()
#endif


void D_RegisterServerCommands(void);
void D_RegisterClientCommands(void);
void CleanupPlayerName(INT32 playernum, const char *newname);
boolean EnsurePlayerNameIsGood(char *name, INT32 playernum);
void D_SendPlayerConfig(void);
void D_SendPlayerConfigForced(void);
void Command_ExitGame_f(void);
void Command_Retry_f(void);
void D_GameTypeChanged(INT32 lastgametype);  
void D_MapChange(INT32 pmapnum, INT32 pgametype, boolean pultmode, boolean presetplayers, INT32 pdelay, boolean pskipprecutscene, boolean pfromlevelselect);
boolean IsPlayerAdmin(INT32 playernum);
void SetAdminPlayer(INT32 playernum);
void ClearAdminPlayers(void);
void RemoveAdminPlayer(INT32 playernum);
void ItemFinder_OnChange(void);
void D_SetPassword(const char *pw);
void D_ClearPassword(void);
void D_ResetAddonAuditState(void);


UINT8 CanChangeSkin(INT32 playernum);

#endif
