// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2004-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



extern boolean usebuffer;

void Y_IntermissionDrawer(void);
void Y_Ticker(void);

void Y_LoadIntermissionData(void);
void Y_StartIntermission(void);
void Y_EndIntermission(void);

void Y_ConsiderScreenBuffer(void);
void Y_CleanupScreenBuffer(void);

void Y_DetermineIntermissionType(void);

typedef enum
{
	int_none,
	int_coop,      
	int_match,     
	int_teammatch, 

	int_ctf,       
	int_spec,      
	int_race,      
	int_comp,      
} intertype_t;
extern intertype_t intertype;
extern intertype_t intermissiontypes[NUMGAMETYPES];
