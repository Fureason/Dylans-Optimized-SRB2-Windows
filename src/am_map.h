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



#ifndef __AMMAP_H__
#define __AMMAP_H__

#include "d_event.h"

typedef struct
{
	INT32 x, y;
} fpoint_t;

typedef struct
{
	fpoint_t a, b;
} fline_t;

extern boolean am_recalc;  
extern boolean automapactive;  


boolean AM_Responder(event_t *ev);


void AM_Ticker(void);


void AM_Drawer(void);


void AM_Start(void);


void AM_Stop(void);

#endif
