// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __G_STATE__
#define __G_STATE__

#include "doomtype.h"


typedef enum
{
	GS_NULL = 0,         


	GS_LEVEL,            
	GS_INTERMISSION,     
	GS_CONTINUING,       

	GS_TITLESCREEN,      
	GS_TIMEATTACK,       

	GS_CREDITS,          
	GS_EVALUATION,       
	GS_GAMEEND,          


	GS_INTRO,            
	GS_ENDING,           
	GS_CUTSCENE,         


	GS_DEDICATEDSERVER,  
	GS_WAITINGPLAYERS    
} gamestate_t;

typedef enum
{
	ga_nothing,
	ga_completed,
	ga_worlddone,
	ga_startcont,
	ga_continued,
} gameaction_t;

extern gamestate_t gamestate;
extern UINT8 titlemapinaction;
extern UINT8 ultimatemode;
extern gameaction_t gameaction;

extern INT32 pickedchar;

extern boolean botingame;
extern UINT8 botskin;
extern UINT16 botcolor;

#endif  
