// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __DEH_TABLES_H__
#define __DEH_TABLES_H__

#include "doomdef.h"  
#include "d_think.h"  
#include "info.h"  
#include "lua_script.h"



extern char *FREE_STATES[NUMSTATEFREESLOTS];
extern char *FREE_MOBJS[NUMMOBJFREESLOTS];
extern char *FREE_SKINCOLORS[NUMCOLORFREESLOTS];
extern bitarray_t used_spr[BIT_ARRAY_SIZE(NUMSPRITEFREESLOTS)];  

#define initfreeslots() {\
	memset(FREE_STATES, 0, sizeof(FREE_STATES));\
	memset(FREE_MOBJS, 0, sizeof(FREE_MOBJS));\
	memset(FREE_SKINCOLORS, 0, sizeof(FREE_SKINCOLORS));\
	memset(used_spr, 0, sizeof(used_spr));\
	memset(actionsoverridden, LUA_REFNIL, sizeof(actionsoverridden));\
}

struct flickytypes_s {
	const char *name;
	const mobjtype_t type;
};

#define MAXFLICKIES 64

                                                           
    
typedef struct
{
	actionf_t action;  
	const char *name;  
} actionpointer_t;

struct int_const_s {
	const char *n;

	lua_Integer v;
};

extern const char NIGHTSGRADE_LIST[];
extern struct flickytypes_s FLICKYTYPES[];
extern actionpointer_t actionpointers[];  
extern const char *const STATE_LIST[];
extern const char *const MOBJTYPE_LIST[];
extern const char *const MOBJFLAG_LIST[];
extern const char *const MOBJFLAG2_LIST[];  
extern const char *const MOBJEFLAG_LIST[];
extern const char *const MAPTHINGFLAG_LIST[];
extern const char *const PLAYERFLAG_LIST[];
extern const char *const GAMETYPERULE_LIST[];
extern const char *const ML_LIST[];  
extern const char *const MSF_LIST[];  
extern const char *const SSF_LIST[];  
extern const char *const SD_LIST[];  
extern const char *const TO_LIST[];  
extern const char *COLOR_ENUMS[];
extern const char *const POWERS_LIST[];
extern const char *const HUDITEMS_LIST[];
extern const char *const MENUTYPES_LIST[];

extern struct int_const_s const INT_CONST[];


void DEH_TableCheck(void);

#endif
