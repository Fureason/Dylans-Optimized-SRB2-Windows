// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2006      by Graue.
// Copyright (C) 2006-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------


















#include "doomdef.h"
#include "doomstat.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "i_system.h"  
#include "i_video.h"  
#include "z_zone.h"
#include "m_misc.h"  
#include "lua_script.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"  
#endif

#ifdef HAVE_VALGRIND
#include "valgrind.h"
static boolean Z_calloc = false;
#include "memcheck.h"
#endif

#define ZONEID 0xa441d13d

#ifdef ZDEBUG

#endif

typedef struct memblock_s
{
	void **user;
	INT32 tag;  
	UINT32 id;  

	size_t size;  
	size_t realsize;  

#ifdef ZDEBUG
	const char *ownerfile;
	INT32 ownerline;
#endif

	struct memblock_s *next, *prev;
} memblock_t;

#define MEMORY(x) (void *)((uintptr_t)(x) + sizeof(memblock_t))
#define MEMBLOCK(x) (memblock_t *)((uintptr_t)(x) - sizeof(memblock_t))

                                                                            
                                                                               
                                                                                 
#ifdef PARANOIA
#define ZONE_TAIL_GUARD_SIZE 32U
#define ZONE_TAIL_GUARD_BYTE 0xA7U

static void Z_SetTailGuard(memblock_t *block)
{
	memset((UINT8 *)MEMORY(block) + block->realsize,
		ZONE_TAIL_GUARD_BYTE, ZONE_TAIL_GUARD_SIZE);
}

static void Z_CheckTailGuard(memblock_t *block, const char *operation)
{
	const UINT8 *guard = (const UINT8 *)MEMORY(block) + block->realsize;
	size_t i;

	for (i = 0; i < ZONE_TAIL_GUARD_SIZE; i++)
		if (guard[i] != ZONE_TAIL_GUARD_BYTE)
			I_Error("%s: zone tail guard damaged at byte %s (tag %d, size %s"
#ifdef ZDEBUG
				", allocated by %s:%d"
#endif
				")", operation, sizeu1(i), block->tag, sizeu2(block->realsize)
#ifdef ZDEBUG
				, block->ownerfile, block->ownerline
#endif
				);
}
#else
#define ZONE_TAIL_GUARD_SIZE 0U
#endif


static memblock_t head;




static void Command_Memfree_f(void);
#ifdef ZDEBUG
static void Command_Memdump_f(void);
#endif





                            
                         
   
                                                          
    
void Z_Init(void)
{
	size_t total, memfree;

	memset(&head, 0x00, sizeof(head));

	head.next = head.prev = &head;

	memfree = I_GetFreeMem(&total)>>20;
	CONS_Printf("System memory: %sMB - Free: %sMB\n", sizeu1(total>>20), sizeu2(memfree));


	COM_AddCommand("memfree", Command_Memfree_f, COM_LUA);

#ifdef ZDEBUG
	COM_AddCommand("memdump", Command_Memdump_f, COM_LUA);
#endif
}






                           
   
                                             
                                                                      
                  
    
#ifdef ZDEBUG
void Z_Free2(void *ptr, const char *file, INT32 line)
#else
void Z_Free(void *ptr)
#endif
{
	memblock_t *block;

	if (ptr == NULL)
		return;

#ifdef ZDEBUG2
	CONS_Debug(DBG_MEMORY, "Z_Free %s:%d\n", file, line);
#endif

	block = MEMBLOCK(ptr);
#ifdef PARANOIA
	if (block->id != ZONEID)
#ifdef ZDEBUG
		I_Error("Z_Free at %s:%d: wrong id", file, line);
#else
		I_Error("Z_Free: wrong id");
#endif
	Z_CheckTailGuard(block, "Z_Free");
#endif

#ifdef ZDEBUG

	CONS_Debug(DBG_MEMORY, "Z_Free at %s:%d\n", file, line);
#endif


	if (block->tag != PU_LUA)
		LUA_InvalidateUserdata(ptr);





	if (block->user != NULL)
		*block->user = NULL;

#ifdef VALGRIND_DESTROY_MEMPOOL
	VALGRIND_DESTROY_MEMPOOL(block);
#endif
	block->prev->next = block->next;
	block->next->prev = block->prev;
	free(block);
}

                                         
   
                                                           
                                              
    
static void *xm(size_t size)
{
	const size_t padedsize = size+sizeof (size_t);
	void *p;

	if (padedsize < size) 
		I_Error("You are allocating memory too large!");
	p = malloc(padedsize);

	if (p == NULL)
	{

		Z_FreeTags(PU_PURGELEVEL, INT32_MAX);
		p = malloc(padedsize);

		if (p == NULL)
		{
			I_Error("Out of memory allocating %s bytes", sizeu1(size));
		}
	}

	return p;
}

                               
                                                                                     
   
                                                           
                         
                                                                       
                                                         
                                                                                   
                                                                                    
                                                                                    
                                     
    
#ifdef ZDEBUG
void *Z_Malloc2(size_t size, INT32 tag, void *user, INT32 alignbits,
	const char *file, INT32 line)
#else
void *Z_MallocAlign(size_t size, INT32 tag, void *user, INT32 alignbits)
#endif
{
	memblock_t *block;
	void *ptr;
	(void)(alignbits);  

#ifdef ZDEBUG2
	CONS_Debug(DBG_MEMORY, "Z_Malloc %s:%d\n", file, line);
#endif

	block = xm(sizeof (memblock_t) + size + ZONE_TAIL_GUARD_SIZE);
	ptr = MEMORY(block);
	I_Assert((intptr_t)ptr % sizeof (void *) == 0);

#ifdef HAVE_VALGRIND
	Z_calloc = false;
#endif

	block->next = head.next;
	block->prev = &head;
	head.next = block;
	block->next->prev = block;

	block->tag = tag;
	block->user = NULL;
#ifdef ZDEBUG
	block->ownerline = line;
	block->ownerfile = file;
#endif
	block->size = sizeof (memblock_t) + size;
	block->realsize = size;

#ifdef PARANOIA
	Z_SetTailGuard(block);
#endif

#ifdef VALGRIND_CREATE_MEMPOOL
	VALGRIND_CREATE_MEMPOOL(block, size, Z_calloc);
#endif

	block->id = ZONEID;

	if (user != NULL)
	{
		block->user = user;
		*(void **)user = ptr;
	}
	else if (tag >= PU_PURGELEVEL)
		I_Error("Z_Malloc: attempted to allocate purgable block "
			"(size %s) with no user", sizeu1(size));

	return ptr;
}

                               
                                                                                     
                                                                  
   
                                                           
                         
                                                                       
                                                         
                                                                                   
                                                                                    
                                                                                    
                                     
    
#ifdef ZDEBUG
void *Z_Calloc2(size_t size, INT32 tag, void *user, INT32 alignbits, const char *file, INT32 line)
#else
void *Z_CallocAlign(size_t size, INT32 tag, void *user, INT32 alignbits)
#endif
{
#ifdef VALGRIND_MEMPOOL_ALLOC
	Z_calloc = true;
#endif
#ifdef ZDEBUG
	return memset(Z_Malloc2    (size, tag, user, alignbits, file, line), 0, size);
#else
	return memset(Z_MallocAlign(size, tag, user, alignbits            ), 0, size);
#endif
}

                                
                                                  
   
                                             
                                                                      
                                                                                   
                                                   
                                                                       
                             
                                                                         
                                                                                                
                                                                                    
                                                                                 
                                                                                     
                                    
    
#ifdef ZDEBUG
void *Z_Realloc2(void *ptr, size_t size, INT32 tag, void *user, INT32 alignbits, const char *file, INT32 line)
#else
void *Z_ReallocAlign(void *ptr, size_t size, INT32 tag, void *user, INT32 alignbits)
#endif
{
	void *rez;
	memblock_t *block;
	size_t copysize;

#ifdef ZDEBUG2
	CONS_Debug(DBG_MEMORY, "Z_Realloc %s:%d\n", file, line);
#endif

	if (!size)
	{
		Z_Free(ptr);
		return NULL;
	}

	if (!ptr)
	{
#ifdef ZDEBUG
		return Z_Calloc2(size, tag, user, alignbits, file , line);
#else
		return Z_CallocAlign(size, tag, user, alignbits);
#endif
	}

	block = MEMBLOCK(ptr);
#ifdef PARANOIA
	if (block->id != ZONEID)
#ifdef ZDEBUG
		I_Error("Z_ReallocAlign at %s:%d: wrong id", file, line);
#else
		I_Error("Z_ReallocAlign: wrong id");
#endif
	Z_CheckTailGuard(block, "Z_ReallocAlign");
#endif

	if (block == NULL)
		return NULL;

#ifdef ZDEBUG

	DEBFILE(va("Z_Realloc at %s:%d\n", file, line));
	rez = Z_Malloc2(size, tag, user, alignbits, file, line);
#else
	rez = Z_MallocAlign(size, tag, user, alignbits);
#endif

	if (size < block->realsize)
		copysize = size;
	else
		copysize = block->realsize;

	M_Memcpy(rez, ptr, copysize);

#ifdef ZDEBUG
	Z_Free2(ptr, file, line);
#else
	Z_Free(ptr);
#endif



	if (user)
		*((void**)user) = rez;

	if (size > copysize)
		memset((char*)rez+copysize, 0x00, size-copysize);

	return rez;
}

                                             
   
                                             
                                               
    
void Z_FreeTags(INT32 lowtag, INT32 hightag)
{
	memblock_t *block, *next;

	Z_CheckHeap(420);
	for (block = head.next; block != &head; block = next)
	{
		next = block->next;  
		if (block->tag >= lowtag && block->tag <= hightag)
			Z_Free(MEMORY(block));
	}
}

                                                        
   
                                             
                                               
                                          
    
void Z_IterateTags(INT32 lowtag, INT32 hightag, boolean (*iterfunc)(void *))
{
	memblock_t *block;

	if (!iterfunc)
		I_Error("Z_IterateTags: no iterator function was given");

	for (block = head.next; block != &head; )
	{
		if (block->tag >= lowtag && block->tag <= hightag)
		{
			void *mem = MEMORY(block);
			boolean free = iterfunc(mem);

			                                                                   
                                                                      
                                                                       
                                                                        
                                   
			if (free)
			{
				memblock_t *next = block->next;
				Z_Free(mem);
				block = next;
			}
			else
				block = block->next;
		}
		else
			block = block->next;
	}
}






#define CLEANUPCOUNT 2000


static INT32 nextcleanup = CLEANUPCOUNT;

                                                 
                                                       
                      
   
                                                           
                                                           
                                                                    
                                  
    
void Z_CheckMemCleanup(void)
{
	if (nextcleanup-- == 0)
	{
		nextcleanup = CLEANUPCOUNT;
		Z_FreeTags(PU_PURGELEVEL, INT32_MAX);
	}
}


                                                                    
                   
                                                                      
                                       
    
void Z_CheckHeap(INT32 i)
{
	memblock_t *block;
	UINT32 blocknumon = 0;
	void *given;

	for (block = head.next; block != &head; block = block->next)
	{
		blocknumon++;
		given = MEMORY(block);
#ifdef ZDEBUG2
		CONS_Debug(DBG_MEMORY, "block %u owned by %s:%d\n",
			blocknumon, block->ownerfile, block->ownerline);
#endif
#ifdef VALGRIND_MEMPOOL_EXISTS
		if (!VALGRIND_MEMPOOL_EXISTS(block))
		{
			I_Error("Z_CheckHeap %d: block %u"
#ifdef ZDEBUG
				"(owned by %s:%d)"
#endif
				" should not exist", i, blocknumon
#ifdef ZDEBUG
				, block->ownerfile, block->ownerline
#endif
				);
		}
#endif
		if (block->user != NULL && *(block->user) != given)
		{
			I_Error("Z_CheckHeap %d: block %u"
#ifdef ZDEBUG
				"(owned by %s:%d)"
#endif
				" doesn't have a proper user", i, blocknumon
#ifdef ZDEBUG
				, block->ownerfile, block->ownerline
#endif
				);
		}
		if (block->next->prev != block)
		{
			I_Error("Z_CheckHeap %d: block %u"
#ifdef ZDEBUG
				"(owned by %s:%d)"
#endif
				" lacks proper backlink", i, blocknumon
#ifdef ZDEBUG
				, block->ownerfile, block->ownerline
#endif
				);
		}
		if (block->prev->next != block)
		{
			I_Error("Z_CheckHeap %d: block %u"
#ifdef ZDEBUG
				"(owned by %s:%d)"
#endif
				" lacks proper forward link", i, blocknumon
#ifdef ZDEBUG
				, block->ownerfile, block->ownerline
#endif
				);
		}
		if (block->id != ZONEID)
		{
			I_Error("Z_CheckHeap %d: block %u"
#ifdef ZDEBUG
				"(owned by %s:%d)"
#endif
				" have the wrong ID", i, blocknumon
#ifdef ZDEBUG
				, block->ownerfile, block->ownerline
#endif
				);
		}
#ifdef PARANOIA
		Z_CheckTailGuard(block, "Z_CheckHeap");
#endif
	}
}





                                       
   
                                             
                                                                      
                           
                 
    
#ifdef PARANOIA
void Z_ChangeTag2(void *ptr, INT32 tag, const char *file, INT32 line)
#else
void Z_ChangeTag(void *ptr, INT32 tag)
#endif
{
	memblock_t *block;

	if (ptr == NULL)
		return;

	block = MEMBLOCK(ptr);

#ifdef PARANOIA
	if (block->id != ZONEID) I_Error("Z_ChangeTag at %s:%d: wrong id", file, line);
#endif

	if (tag >= PU_PURGELEVEL && block->user == NULL)
		I_Error("Internal memory management error: "
			"tried to make block purgable but it has no owner");

	block->tag = tag;
}

                                  
   
                                             
                                                                      
                                                     
                   
    
#ifdef PARANOIA
void Z_SetUser2(void *ptr, void **newuser, const char *file, INT32 line)
#else
void Z_SetUser(void *ptr, void **newuser)
#endif
{
	memblock_t *block;

	if (ptr == NULL)
		return;

	block = MEMBLOCK(ptr);

#ifdef PARANOIA
	if (block->id != ZONEID) I_Error("Z_SetUser at %s:%d: wrong id", file, line);
#endif

	if (block->tag >= PU_PURGELEVEL && newuser == NULL)
		I_Error("Internal memory management error: "
			"tried to make block purgable but it has no owner");

	block->user = (void*)newuser;
	*newuser = ptr;
}





                                                    
   
                                             
                                               
                                                                   
                       
    
size_t Z_TagsUsage(INT32 lowtag, INT32 hightag)
{
	size_t cnt = 0;
	memblock_t *rover;

	for (rover = head.next; rover != &head; rover = rover->next)
	{
		if (rover->tag < lowtag || rover->tag > hightag)
			continue;
		cnt += rover->size + sizeof *rover;
	}

	return cnt;
}





                                                         
                                                                         
    
static void Command_Memfree_f(void)
{
	size_t freebytes, totalbytes;

	Z_CheckHeap(-1);
	CONS_Printf("\x82%s", M_GetText("Memory Info\n"));
	CONS_Printf(M_GetText("Total heap used        : %7s KB\n"), sizeu1(Z_TotalUsage()>>10));
	CONS_Printf(M_GetText("Static                 : %7s KB\n"), sizeu1(Z_TagUsage(PU_STATIC)>>10));
	CONS_Printf(M_GetText("Static (sound)         : %7s KB\n"), sizeu1(Z_TagUsage(PU_SOUND)>>10));
	CONS_Printf(M_GetText("Static (music)         : %7s KB\n"), sizeu1(Z_TagUsage(PU_MUSIC)>>10));
	CONS_Printf(M_GetText("Patches                : %7s KB\n"), sizeu1(Z_TagUsage(PU_PATCH)>>10));
	CONS_Printf(M_GetText("Patches (low priority) : %7s KB\n"), sizeu1(Z_TagUsage(PU_PATCH_LOWPRIORITY)>>10));
	CONS_Printf(M_GetText("Patches (rotated)      : %7s KB\n"), sizeu1(Z_TagUsage(PU_PATCH_ROTATED)>>10));
	CONS_Printf(M_GetText("Sprites                : %7s KB\n"), sizeu1(Z_TagUsage(PU_SPRITE)>>10));
	CONS_Printf(M_GetText("HUD graphics           : %7s KB\n"), sizeu1(Z_TagUsage(PU_HUDGFX)>>10));
	CONS_Printf(M_GetText("Locked cache           : %7s KB\n"), sizeu1(Z_TagUsage(PU_CACHE)>>10));
	CONS_Printf(M_GetText("Level                  : %7s KB\n"), sizeu1(Z_TagUsage(PU_LEVEL)>>10));
	CONS_Printf(M_GetText("Special thinker        : %7s KB\n"), sizeu1(Z_TagUsage(PU_LEVSPEC)>>10));
	CONS_Printf(M_GetText("All purgable           : %7s KB\n"),
		sizeu1(Z_TagsUsage(PU_PURGELEVEL, INT32_MAX)>>10));

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		CONS_Printf(M_GetText("Patch info headers     : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRPATCHINFO)>>10));
		CONS_Printf(M_GetText("Cached textures        : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRCACHE)>>10));
		CONS_Printf(M_GetText("Texture colormaps      : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRPATCHCOLMIPMAP)>>10));
		CONS_Printf(M_GetText("Model textures         : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRMODELTEXTURE)>>10));
		CONS_Printf(M_GetText("Light table textures   : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRLIGHTTABLEDATA)>>10));
		CONS_Printf(M_GetText("Plane polygons         : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRPLANE)>>10));
		CONS_Printf(M_GetText("All GPU textures       : %7u KB\n"), HWR_GetTextureUsed()>>10);
	}
#endif

	CONS_Printf("\x82%s", M_GetText("System Memory Info\n"));
	freebytes = I_GetFreeMem(&totalbytes);
	CONS_Printf(M_GetText("    Total physical memory: %s KB\n"), sizeu1(totalbytes>>10));
	CONS_Printf(M_GetText("Available physical memory: %s KB\n"), sizeu1(freebytes>>10));
}

#ifdef ZDEBUG
                                                         
                                                                                          
                                                                                           
                                                        
    
static void Command_Memdump_f(void)
{
	memblock_t *block;
	INT32 mintag = 0, maxtag = INT32_MAX;
	INT32 i;

	if ((i = COM_CheckParm("-min")))
		mintag = atoi(COM_Argv(i + 1));

	if ((i = COM_CheckParm("-max")))
		maxtag = atoi(COM_Argv(i + 1));

	for (block = head.next; block != &head; block = block->next)
		if (block->tag >= mintag && block->tag <= maxtag)
		{
			char *filename = strrchr(block->ownerfile, PATHSEP[0]);
			CONS_Printf("[%3d] %s (%s) bytes @ %s:%d\n", block->tag, sizeu1(block->size), sizeu2(block->realsize), filename ? filename + 1 : block->ownerfile, block->ownerline);
		}
}
#endif

                               
   
                                     
                                                           
    
char *Z_StrDup(const char *s)
{
	return strcpy(ZZ_Alloc(strlen(s) + 1), s);
}
