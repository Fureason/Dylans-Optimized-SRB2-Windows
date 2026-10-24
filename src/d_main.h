// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __D_MAIN__
#define __D_MAIN__

#include "d_event.h"
#include "w_wad.h"    

extern boolean advancedemo;


extern tic_t rendergametic;

extern char srb2home[256];  
extern boolean usehome;  
extern const char *pandf;  
extern char srb2path[256];  
extern UINT8 window_notinfocus;


void D_SRB2Loop(void) FUNCNORETURN;







void D_SRB2Main(void);


void D_PostEvent(const event_t *ev);
void D_ProcessEvents(void);

const char *D_Home(void);

boolean D_IsPathAllowed(const char *path);
boolean D_CheckPathAllowed(const char *path, const char *why);




void D_AdvanceDemo(void);
void D_StartTitle(void);

#endif  
