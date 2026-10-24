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



#ifndef __STSTUFF_H__
#define __STSTUFF_H__

#include "doomtype.h"
#include "d_event.h"
#include "d_player.h"
#include "r_defs.h"






void ST_Ticker(boolean run);


void ST_Drawer(void);


void ST_Start(void);


void ST_Init(void);


void ST_changeDemoView(void);

void ST_UnloadGraphics(void);
void ST_LoadGraphics(void);


void ST_LoadFaceGraphics(INT32 skinnum);
void ST_ReloadSkinFaceGraphics(void);

void ST_doPaletteStuff(void);


void ST_startTitleCard(void);
void ST_stopTitleCard(void);
void ST_runTitleCard(void);
void ST_drawTitleCard(void);
void ST_preLevelTitleCardDrawer(void);
void ST_drawWipeTitleCard(void);

extern tic_t lt_ticker, lt_lasttic;
extern tic_t lt_exitticker, lt_endtime;


boolean ST_SameTeam(player_t *a, player_t *b);





extern boolean st_overlay;  
extern INT32 st_palette;  
extern INT32 st_translucency;

extern lumpnum_t st_borderpatchnum;

extern patch_t *tallnum[10];
extern patch_t *sboscore;
extern patch_t *sbotime;
extern patch_t *sbocolon;
extern patch_t *sboperiod;
extern patch_t **faceprefix;  
extern patch_t **superprefix;  
extern patch_t *livesback;
extern patch_t *stlivex;
extern patch_t *ngradeletters[7];

                                                      
    
typedef struct
{
	INT32 x, y, f;
} hudinfo_t;

typedef enum
{
	HUD_LIVES,
	HUD_INPUT,

	HUD_RINGS,
	HUD_RINGSNUM,
	HUD_RINGSNUMTICS,

	HUD_SCORE,
	HUD_SCORENUM,

	HUD_TIME,
	HUD_MINUTES,
	HUD_TIMECOLON,
	HUD_SECONDS,
	HUD_TIMETICCOLON,
	HUD_TICS,

	HUD_SS_TOTALRINGS,

	HUD_GETRINGS,
	HUD_GETRINGSNUM,
	HUD_TIMELEFT,
	HUD_TIMELEFTNUM,
	HUD_TIMEUP,
	HUD_HUNTPICS,
	HUD_POWERUPS,

	NUMHUDITEMS
} hudnum_t;

extern hudinfo_t hudinfo[NUMHUDITEMS];

extern UINT16 objectsdrawn;

#define NUMLINKCOLORS 12
extern skincolornum_t linkColor[3][NUMLINKCOLORS];

#endif
