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



#ifndef __D_TICCMD__
#define __D_TICCMD__

#include "m_fixed.h"
#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif

#define MAXPREDICTTICS 12


typedef enum
{

	BT_WEAPONMASK = 0x0F,  

	BT_WEAPONNEXT = 1<<4,
	BT_WEAPONPREV = 1<<5,

	BT_ATTACK     = 1<<6,  
	BT_SPIN       = 1<<7,
	BT_CAMLEFT    = 1<<8,  
	BT_CAMRIGHT   = 1<<9,  
	BT_TOSSFLAG   = 1<<10,
	BT_JUMP       = 1<<11,
	BT_FIRENORMAL = 1<<12,  

	BT_CUSTOM1    = 1<<13,
	BT_CUSTOM2    = 1<<14,
	BT_CUSTOM3    = 1<<15,
} buttoncode_t;







#define TICCMD_RECEIVED 1
#define TICCMD_XY 2

#if defined(_MSC_VER)
#pragma pack(1)
#endif

typedef struct
{
	SINT8 forwardmove;  
	SINT8 sidemove;  
	INT16 angleturn;  
	INT16 aiming;  
	UINT16 buttons;
	UINT8 latency;  
} ATTRPACK ticcmd_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

#endif
