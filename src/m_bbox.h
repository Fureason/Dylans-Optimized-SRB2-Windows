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



#ifndef __M_BBOX__
#define __M_BBOX__

#include "m_fixed.h"

                                          
  

enum
{
	BOXTOP,  
	BOXBOTTOM,  
	BOXLEFT,  
	BOXRIGHT  
};  


void M_ClearBox(fixed_t *box);

void M_AddToBox(fixed_t *box, fixed_t x, fixed_t y);
boolean M_PointInBox(fixed_t *box, fixed_t x, fixed_t y);
boolean M_CircleTouchBox(fixed_t *box, fixed_t circlex, fixed_t circley, fixed_t circleradius);

#endif
