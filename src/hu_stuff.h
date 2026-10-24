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



#ifndef __HU_STUFF_H__
#define __HU_STUFF_H__

#include "d_event.h"
#include "w_wad.h"
#include "r_defs.h"




#define FONTSTART '\x16'  
#define FONTEND '~'
#define FONTSIZE (FONTEND - FONTSTART + 1)

#define HU_CROSSHAIRS 3  

extern char *shiftxform;  
extern char english_shiftxform[];

typedef struct
{
	patch_t *chars[FONTSIZE];
	INT32 kerning;
	UINT32 spacewidth;
	UINT32 charwidth;
	UINT32 linespacing;
} fontdef_t;

extern fontdef_t hu_font, tny_font, cred_font, lt_font;
extern fontdef_t ntb_font, nto_font;
extern patch_t *tallnum[10];
extern patch_t *nightsnum[10];
extern patch_t *ttlnum[10];
extern patch_t *tallminus;
extern patch_t *tallinfin;





typedef struct
{
	UINT32 count;
	INT32 num;
	INT32 color;
	INT32 emeralds;
	const char *name;
} playersort_t;




#define HU_MAXMSGLEN 223
#define CHAT_BUFSIZE 64		 
#ifdef NETSPLITSCREEN
#define OLDCHAT (cv_consolechat.value == 1 || dedicated || vid.width < 640)
#else
#define OLDCHAT (cv_consolechat.value == 1 || dedicated || vid.width < 640 || splitscreen)
#endif
#define CHAT_MUTE ((cv_mute.value || players[consoleplayer].muted) && !(server || IsPlayerAdmin(consoleplayer)))	 
#define OLD_MUTE (OLDCHAT && (cv_mute.value || players[consoleplayer].muted) && !(server || IsPlayerAdmin(consoleplayer)))	 


void HU_AddChatText(const char *text, boolean playsound);


extern boolean chat_on;

extern UINT8 spam_tokens[MAXPLAYERS];
extern tic_t spam_tics[MAXPLAYERS];

extern patch_t *emeraldpics[3][8];
extern patch_t *rflagico;
extern patch_t *bflagico;
extern patch_t *rmatcico;
extern patch_t *bmatcico;
extern patch_t *tagico;
extern patch_t *tokenicon;


extern boolean hu_showscores;


void HU_Init(void);

void HU_LoadGraphics(void);
void HU_LoadFontCharacters(fontdef_t *font, const char *prefix);
void HU_SetFontProperties(fontdef_t *font, INT32 kerning, UINT32 spacewidth, UINT32 charwidth, UINT32 linespacing);


void HU_Start(void);

boolean HU_Responder(event_t *ev);
void HU_Ticker(void);
void HU_Drawer(void);
char HU_dequeueChatChar(void);
void HU_clearChatChars(void);
void HU_drawPing(INT32 x, INT32 y, UINT32 ping, boolean notext, INT32 flags);	 
void HU_DrawTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer);
void HU_DrawTeamTabRankings(playersort_t *tab, INT32 whiteplayer);
void HU_DrawDualTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer);
void HU_DrawEmeralds(INT32 x, INT32 y, INT32 pemeralds);

INT32 HU_CreateTeamScoresTbl(playersort_t *tab, UINT32 dmtotals[]);


void HU_ClearCEcho(void);
void HU_SetCEchoDuration(INT32 seconds);
void HU_SetCEchoFlags(INT32 flags);
void HU_DoCEcho(const char *msg);


extern UINT32 hu_demoscore;
extern UINT32 hu_demotime;
extern UINT16 hu_demorings;
#endif
