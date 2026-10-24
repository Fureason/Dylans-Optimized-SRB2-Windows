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



#ifndef __I_TIME_H__
#define __I_TIME_H__

#include "command.h"
#include "doomtype.h"
#include "m_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct timestate_s {
	tic_t time;
	fixed_t timefrac;
} timestate_t;

extern timestate_t g_time;
extern consvar_t cv_timescale;

                                                                    
  
tic_t I_GetTime(void);

                                      
  
void I_InitializeTime(void);

void I_UpdateTime(fixed_t timescale);

                                                                              
                                                                             
                                                                      
                      
  
void I_SleepDuration(precise_t duration);

#ifdef __cplusplus
}  
#endif

#endif  
