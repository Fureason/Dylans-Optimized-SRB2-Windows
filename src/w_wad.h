// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __W_WAD__
#define __W_WAD__

#ifdef HWRENDER
#include "hardware/hw_data.h"
#endif

#include "m_aatree.h"

#ifdef __GNUG__
#pragma interface
#endif



#if defined(_MSC_VER)
#pragma pack(1)
#endif
typedef struct
{
	UINT32 filepos;  
	UINT32 size;  
	char name[8];  
} ATTRPACK filelump_t;
#if defined(_MSC_VER)
#pragma pack()
#endif







typedef struct
{
	char identification[4];  
	UINT32 numlumps;  
	UINT32 infotableofs;  
} wadinfo_t;


typedef enum
{
	CM_NOCOMPRESSION,
#ifdef HAVE_ZLIB
	CM_DEFLATE,
#endif
	CM_LZF,
	CM_UNSUPPORTED
} compmethod;


typedef struct
{
	unsigned long position;  
	unsigned long disksize;  
	char name[9];            
	UINT32 hash;
	char *longname;          
	char *fullname;          
	char *diskpath;          
	size_t size;             
	compmethod compression;  
} lumpinfo_t;





typedef struct {
	char name[9];
	UINT8* data;
	size_t size;
} virtlump_t;

typedef struct {
	size_t numlumps;
	virtlump_t* vlumps;
} virtres_t;

virtres_t* vres_GetMap(lumpnum_t);
void vres_Free(virtres_t*);
virtlump_t* vres_Find(const virtres_t*, const char*);







#ifdef ENFORCE_WAD_LIMIT
#define MAX_WADFILES 2048  
#else
#define MAX_WADFILES UINT16_MAX
#endif

#define MAX_WADPATH 512

#define lumpcache_t void *


typedef enum restype
{
	RET_WAD,
	RET_SOC,
	RET_LUA,
	RET_PK3,
	RET_FOLDER,
	RET_UNKNOWN,
} restype_t;

typedef struct wadfile_s
{
	char *filename, *path;
	restype_t type;
	lumpinfo_t *lumpinfo;
	lumpcache_t *lumpcache;
	lumpcache_t *patchcache;
	aatree_t *startfolders;
	aatree_t *endfolders;
	aatree_t *fullnamelumps;
	UINT16 numlumps;  
	UINT16 foldercount;  
	FILE *handle;
	UINT32 filesize;  
	UINT8 md5sum[16];

	boolean important;  
} wadfile_t;

#define WADFILENUM(lumpnum) (UINT16)((lumpnum)>>16)  
#define LUMPNUM(lumpnum) (UINT16)((lumpnum)&0xFFFF)  

extern UINT16 numwadfiles;
extern wadfile_t **wadfiles;

typedef struct
{
	char **files;
	size_t numfiles;
} addfilelist_t;



void W_Shutdown(void);


FILE *W_OpenWadFile(const char **filename, boolean useerrors);

#define W_VERIFY_UNKNOWN (-2)
UINT16 W_InitFile(const char *filename, boolean mainfile, boolean startup);
UINT16 W_InitFileVerified(const char *filename, boolean mainfile, boolean startup,
	int verification, const UINT8 *knownmd5);

UINT16 W_InitFolder(const char *path, boolean mainfile, boolean startup);


void W_InitMultipleFiles(addfilelist_t *list);

#define W_FileHasFolders(wadfile) ((wadfile)->type == RET_PK3 || (wadfile)->type == RET_FOLDER)

INT32 W_IsPathToFolderValid(const char *path);
char *W_GetFullFolderPath(const char *path);

const char *W_CheckNameForNumPwad(UINT16 wad, UINT16 lump);
const char *W_CheckNameForNum(lumpnum_t lumpnum);

UINT16 W_CheckNumForNamePwad(const char *name, UINT16 wad, UINT16 startlump);  
UINT16 W_CheckNumForLongNamePwad(const char *name, UINT16 wad, UINT16 startlump);

 
UINT16 W_CheckNumForMarkerStartPwad(const char *name, UINT16 wad, UINT16 startlump);

UINT16 W_CheckNumForFullNamePK3(const char *name, UINT16 wad, UINT16 startlump);
UINT16 W_CheckNumForFolderStartPK3(const char *name, UINT16 wad, UINT16 startlump);
UINT16 W_CheckNumForFolderEndPK3(const char *name, UINT16 wad, UINT16 startlump);
char *W_GetLumpFolderPathPK3(UINT16 wad, UINT16 lump);
char *W_GetLumpFolderNamePK3(UINT16 wad, UINT16 lump);

void W_GetFolderLumpsPwad(const char *name, UINT16 wad, UINT32 **list, UINT16 *list_capacity, UINT16 *numlumps);
void W_GetFolderLumps(const char *name, UINT32 **list, UINT16 *list_capacity, UINT16 *numlumps);
UINT32 W_CountFolderLumpsPwad(const char *name, UINT16 wad);
UINT32 W_CountFolderLumps(const char *name);

lumpnum_t W_CheckNumForMap(const char *name);
lumpnum_t W_CheckNumForName(const char *name);
lumpnum_t W_CheckNumForLongName(const char *name);
lumpnum_t W_GetNumForName(const char *name);  
lumpnum_t W_GetNumForLongName(const char *name);

lumpnum_t W_CheckNumForPatchName(const char *name);
lumpnum_t W_CheckNumForLongPatchName(const char *name);
lumpnum_t W_GetNumForPatchName(const char *name);  
lumpnum_t W_GetNumForLongPatchName(const char *name);

lumpnum_t W_CheckNumForNameInBlock(const char *name, const char *blockstart, const char *blockend);
UINT8 W_LumpExists(const char *name);  

size_t W_LumpLengthPwad(UINT16 wad, UINT16 lump);
size_t W_LumpLength(lumpnum_t lumpnum);

boolean W_IsLumpWad(lumpnum_t lumpnum);  
boolean W_IsLumpFolder(UINT16 wad, UINT16 lump);  

#ifdef HAVE_ZLIB
void zerr(int ret);  
#endif

size_t W_ReadLumpHeaderPwad(UINT16 wad, UINT16 lump, void *dest, size_t size, size_t offset);
size_t W_ReadLumpHeader(lumpnum_t lump, void *dest, size_t size, size_t offest);  
void W_ReadLumpPwad(UINT16 wad, UINT16 lump, void *dest);
void W_ReadLump(lumpnum_t lump, void *dest);
boolean W_BeginLumpHeaderBatch(UINT16 wad);
void W_EndLumpHeaderBatch(void);

void *W_CacheLumpNumPwad(UINT16 wad, UINT16 lump, INT32 tag);
void *W_CacheLumpNum(lumpnum_t lump, INT32 tag);
void *W_CacheLumpNumForce(lumpnum_t lumpnum, INT32 tag);

boolean W_IsLumpCached(lumpnum_t lump, void *ptr);
boolean W_IsPatchCached(lumpnum_t lump, void *ptr);
boolean W_IsPatchCachedPwad(UINT16 wad, UINT16 lump, void *ptr);

void *W_CacheLumpName(const char *name, INT32 tag);
void *W_CachePatchName(const char *name, INT32 tag);
void *W_CachePatchLongName(const char *name, INT32 tag);

void *W_CachePatchNumPwad(UINT16 wad, UINT16 lump, INT32 tag);
void *W_CachePatchNum(lumpnum_t lumpnum, INT32 tag);
void *W_GetCachedPatchNumPwad(UINT16 wad, UINT16 lump);

boolean W_ReadPatchHeaderPwad(UINT16 wadnum, UINT16 lumpnum, INT16 *width, INT16 *height, INT16 *topoffset, INT16 *leftoffset);
boolean W_ReadPatchHeader(lumpnum_t lumpnum, INT16 *width, INT16 *height, INT16 *topoffset, INT16 *leftoffset);

void W_UnlockCachedPatch(void *patch);

void W_VerifyFileMD5(UINT16 wadfilenum, const char *matchmd5);

int W_VerifyNMUSlumps(const char *filename, boolean exit_on_error);

#endif  
