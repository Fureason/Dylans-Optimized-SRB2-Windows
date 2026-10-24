// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "d_event.h"
#include "command.h"
#include "i_threads.h"

void CON_Init(void);

void CON_StartRefresh(void);
void CON_StopRefresh(void);

boolean CON_Responder(event_t *ev);

#ifdef HAVE_THREADS
extern I_mutex con_mutex;
#endif


extern boolean con_recalc;


extern boolean con_startup;


extern boolean con_refresh;


extern INT32 con_destlines;

extern UINT32 con_scalefactor;  

extern consvar_t cons_backcolor;
extern consvar_t cons_backpic;
extern consvar_t cons_lines;

extern UINT8 *yellowmap, *magentamap, *lgreenmap, *bluemap, *graymap, *redmap, *orangemap, *skymap, *purplemap, *aquamap, *peridotmap, *azuremap, *brownmap, *rosymap, *invertmap;


extern UINT8 *consolebgmap;
extern UINT8 *promptbgmap;

void CON_SetupBackColormapEx(INT32 color, boolean prompt);
void CON_SetupBackColormap(void);
void CON_ClearHUD(void);  

void CON_Ticker(void);
void CON_Drawer(void);
void CONS_Error(const char *msg);  


void CON_ToggleOff(void);


boolean CON_Ready(void);

void CON_LogMessage(const char *msg);
