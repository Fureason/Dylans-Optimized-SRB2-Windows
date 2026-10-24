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



#ifndef __D_EVENT__
#define __D_EVENT__

#include "doomtype.h"
#include "g_state.h"


typedef enum
{
	ev_keydown,
	ev_keyup,
	ev_text,
	ev_console,
	ev_mouse,
	ev_joystick,
	ev_mouse2,
	ev_joystick2,
} evtype_t;


typedef struct
{
	evtype_t type;
	INT32 key;  
	INT32 x;  
	INT32 y;  
	boolean repeated;  
} event_t;




#define MAXEVENTS 128

extern event_t events[MAXEVENTS];
extern INT32 eventhead, eventtail;

#endif
