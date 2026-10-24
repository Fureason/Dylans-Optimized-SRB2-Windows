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



#ifndef __R_SKY__
#define __R_SKY__

#include "m_fixed.h"

#ifdef __GNUG__
#pragma interface
#endif


#define SKYFLATNAME "F_SKY1"


#define ANGLETOSKYSHIFT 22

extern INT32 skytexture, skytexturemid;
extern fixed_t skyscale;

extern INT32 skyflatnum;
extern INT32 levelskynum;
extern INT32 globallevelskynum;


void R_SetupSkyDraw(void);

void R_SetSkyScale(void);

#endif
