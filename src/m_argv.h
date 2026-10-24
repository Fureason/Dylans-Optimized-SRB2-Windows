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



#ifndef __M_ARGV__
#define __M_ARGV__




extern INT32 myargc;
extern char **myargv;
extern boolean myargmalloc;


const char *M_GetUrlProtocolArg(void);


INT32 M_CheckParm(const char *check);


void M_PushSpecialParameters(void);


boolean M_IsNextParm(void);


const char *M_GetNextParm(void);


void M_FindResponseFile(void);

#endif  
