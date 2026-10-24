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



#ifndef __Z_ZONE__
#define __Z_ZONE__

#include <stdio.h>
#include "doomdef.h"
#include "doomtype.h"

#ifdef __GNUC__  
#if (__GNUC__ > 4) || (__GNUC__ == 4 && (__GNUC_MINOR__ >= 3 || (__GNUC_MINOR__ == 2 && __GNUC_PATCHLEVEL__ >= 5)))
#define FUNCALLOC(X) __attribute__((alloc_size(X)))
#endif  
#endif

#ifndef FUNCALLOC
#define FUNCALLOC(x)
#endif








enum
{

	PU_STATIC                = 1,  
	PU_LUA                   = 2,  
	PU_PERFSTATS             = 3,  

	PU_SOUND                 = 11,  
	PU_MUSIC                 = 12,  

	PU_PATCH                 = 14,  
	PU_PATCH_LOWPRIORITY     = 15,  
	PU_PATCH_ROTATED         = 16,  
	PU_PATCH_DATA            = 17,  
	PU_SPRITE                = 18,  
	PU_HUDGFX                = 19,  

	PU_HWRPATCHINFO          = 21,  
	PU_HWRPATCHCOLMIPMAP     = 22,  
	PU_HWRMODELTEXTURE       = 23,  
	PU_HWRLIGHTTABLEDATA     = 24,  

	PU_HWRCACHE              = 48,  
	PU_CACHE                 = 49,  


	PU_LEVEL                 = 50,  
	PU_LEVSPEC               = 51,  
	PU_HWRPLANE              = 52,  


	PU_PURGELEVEL            = 100,  
	PU_CACHE_UNLOCKED        = 101,  
	PU_HWRCACHE_UNLOCKED     = 102,  


	PU_HWRMODELTEXTURE_UNLOCKED = 103,  
};




void Z_Init(void);









#ifdef ZDEBUG
#define Z_Free(p)                 Z_Free2(p, __FILE__, __LINE__)
#define Z_MallocAlign(s,t,u,a)    Z_Malloc2(s, t, u, a, __FILE__, __LINE__)
#define Z_CallocAlign(s,t,u,a)    Z_Calloc2(s, t, u, a, __FILE__, __LINE__)
#define Z_ReallocAlign(p,s,t,u,a) Z_Realloc2(p,s, t, u, a, __FILE__, __LINE__)
void Z_Free2(void *ptr, const char *file, INT32 line);
void *Z_Malloc2(size_t size, INT32 tag, void *user, INT32 alignbits, const char *file, INT32 line) FUNCALLOC(1);
void *Z_Calloc2(size_t size, INT32 tag, void *user, INT32 alignbits, const char *file, INT32 line) FUNCALLOC(1);
void *Z_Realloc2(void *ptr, size_t size, INT32 tag, void *user, INT32 alignbits, const char *file, INT32 line) FUNCALLOC(2);
#else
void Z_Free(void *ptr);
void *Z_MallocAlign(size_t size, INT32 tag, void *user, INT32 alignbits) FUNCALLOC(1);
void *Z_CallocAlign(size_t size, INT32 tag, void *user, INT32 alignbits) FUNCALLOC(1);
void *Z_ReallocAlign(void *ptr, size_t size, INT32 tag, void *user, INT32 alignbits) FUNCALLOC(2);
#endif


#define Z_Malloc(s,t,u)    Z_MallocAlign(s, t, u, sizeof(void *))
#define Z_Calloc(s,t,u)    Z_CallocAlign(s, t, u, sizeof(void *))
#define Z_Realloc(p,s,t,u) Z_ReallocAlign(p, s, t, u, sizeof(void *))




#define Z_FreeTag(tagnum) Z_FreeTags(tagnum, tagnum)
void Z_FreeTags(INT32 lowtag, INT32 hightag);


#define Z_IterateTag(tagnum, func) Z_IterateTags(tagnum, tagnum, func)
void Z_IterateTags(INT32 lowtag, INT32 hightag, boolean (*iterfunc)(void *));




void Z_CheckMemCleanup(void);
void Z_CheckHeap(INT32 i);






#ifdef PARANOIA
#define Z_ChangeTag(p,t) Z_ChangeTag2(p, t, __FILE__, __LINE__)
#define Z_SetUser(p,u)   Z_SetUser2(p, u, __FILE__, __LINE__)
void Z_ChangeTag2(void *ptr, INT32 tag, const char *file, INT32 line);
void Z_SetUser2(void *ptr, void **newuser, const char *file, INT32 line);
#else
void Z_ChangeTag(void *ptr, INT32 tag);
void Z_SetUser(void *ptr, void **newuser);
#endif







#define Z_TagUsage(tagnum) Z_TagsUsage(tagnum, tagnum)
size_t Z_TagsUsage(INT32 lowtag, INT32 hightag);
#define Z_TotalUsage() Z_TagsUsage(0, INT32_MAX)




char *Z_StrDup(const char *in);
#define Z_Unlock(p) (void)p  

#endif
