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



#ifndef __G_DEMO__
#define __G_DEMO__

#include "doomdef.h"
#include "doomstat.h"
#include "d_event.h"






extern boolean demoplayback, titledemo, demorecording, timingdemo;
extern tic_t demostarttime;
extern UINT16 demoversion;

typedef enum
{
	DFILE_OVERRIDE_NONE = 0,  
	DFILE_OVERRIDE_LOAD,  
	DFILE_OVERRIDE_SKIP,  
} demo_file_override_e;

extern demo_file_override_e demofileoverride;
extern consvar_t cv_resyncdemo;


extern boolean singledemo;
extern boolean demo_start;
extern boolean demo_forwardmove_rng;
extern boolean demosynced;

extern mobj_t *metalplayback;


void G_RecordDemo(const char *name);
void G_RecordMetal(void);
void G_BeginRecording(void);
void G_BeginMetal(void);


void G_SetDemoTime(UINT32 ptime, UINT32 pscore, UINT16 prings);
UINT8 G_CmpDemoTime(char *oldname, char *newname);

typedef enum
{
	GHC_NORMAL = 0,
	GHC_SUPER,
	GHC_FIREFLOWER,
	GHC_INVINCIBLE,
	GHC_NIGHTSSKIN,  
	GHC_RETURNSKIN  
} ghostcolor_t;


typedef enum
{
	DFILE_ERROR_NONE = 0,  
	DFILE_ERROR_NOTLOADED,  
	DFILE_ERROR_OUTOFORDER,  
	DFILE_ERROR_INCOMPLETEOUTOFORDER,  
	DFILE_ERROR_CANNOTLOAD,  
	DFILE_ERROR_EXTRAFILES,  
	DFILE_ERROR_NOTDEMO = UINT8_MAX,  
} demo_file_error_e;


void G_ReadDemoTiccmd(ticcmd_t *cmd, INT32 playernum);
void G_WriteDemoTiccmd(ticcmd_t *cmd, INT32 playernum);
void G_GhostAddThok(void);
void G_GhostAddSpin(void);
void G_GhostAddRev(void);
void G_GhostAddColor(ghostcolor_t color);
void G_GhostAddFlip(void);
void G_GhostAddScale(fixed_t scale);
void G_GhostAddHit(mobj_t *victim);
void G_WriteGhostTic(mobj_t *ghost);
void G_ConsGhostTic(void);
void G_GhostTicker(void);
void G_ReadMetalTic(mobj_t *metal);
void G_WriteMetalTic(mobj_t *metal);
void G_SaveMetal(UINT8 **buffer);
void G_LoadMetal(UINT8 **buffer);

void G_DeferedPlayDemo(const char *demo);
void G_DoPlayDemo(char *defdemoname);
void G_TimeDemo(const char *name);
void G_AddGhost(char *defdemoname);
void G_FreeGhosts(void);
void G_DoPlayMetal(void);
void G_DoneLevelLoad(void);
void G_StopMetalDemo(void);
ATTRNORETURN void FUNCNORETURN G_StopMetalRecording(boolean kill);
void G_StopDemo(void);
boolean G_CheckDemoStatus(void);
INT32 G_ConvertOldFrameFlags(INT32 frame);
UINT8 G_CheckDemoForError(char *defdemoname);

#endif  
