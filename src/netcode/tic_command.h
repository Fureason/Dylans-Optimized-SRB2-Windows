// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __D_TIC_COMMAND__
#define __D_TIC_COMMAND__

#include "d_clisrv.h"
#include "../d_ticcmd.h"
#include "../doomdef.h"
#include "../doomtype.h"

extern tic_t firstticstosend;  
extern tic_t tictoclear;  

extern ticcmd_t localcmds;
extern ticcmd_t localcmds2;
extern boolean cl_packetmissed;

extern ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];

tic_t ExpandTics(INT32 low, INT32 node);
void D_Clearticcmd(tic_t tic);
void D_ResetTiccmds(void);
void D_ResetConsistencyTracker(INT32 node);

void PT_ClientCmd(SINT8 nodenum, INT32 netconsole);
void PT_ServerTics(SINT8 node, INT32 netconsole);


void CL_SendClientCmd(void);

void SV_SendTics(void);
void Local_Maketic(INT32 realtics);
void SV_Maketic(void);

#endif
