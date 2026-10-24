// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------




#define RPC_NO_WINDOWS_H
#include <windows.h>

#ifdef BUGTRAP

BOOL InitBugTrap(void);
void ShutdownBugTrap(void);
BOOL IsBugTrapLoaded(void);

#endif



LONG WINAPI RecordExceptionInfo(PEXCEPTION_POINTERS data );

extern LPTOP_LEVEL_EXCEPTION_FILTER prevExceptionFilter;

#ifdef __MINGW32__

#include <excpt.h>

#ifndef TRYLEVEL_NONE

#ifndef __MINGW64__
#define NO_SEH_MINGW  
#endif
#ifndef GetExceptionInformation
void *__cdecl _exception_info(void);
#define GetExceptionInformation (struct _EXCEPTION_POINTERS *)_exception_info
#endif  


#ifndef __try
#define __try
#endif  



#ifndef __except
#define __except(x) if (0)
#endif  

#endif  

#endif  
