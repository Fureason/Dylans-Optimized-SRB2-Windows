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






#ifndef __D_THINK__
#define __D_THINK__

#include "doomdef.h"

#ifdef __GNUG__
#pragma interface
#endif






typedef void (*actionf_v)();
typedef void (*actionf_p1)(void *);

typedef union
{
	actionf_v acv;
	actionf_p1 acp1;
} actionf_t;



typedef actionf_t think_t;


typedef struct thinker_s
{
	struct thinker_s *prev;
	struct thinker_s *next;
	think_t function;



	INT32 references;

	boolean removing;
	boolean cachable;

#ifdef PARANOIA
	INT32 debug_mobjtype;
	tic_t debug_time;
#endif
} thinker_t;

#endif
