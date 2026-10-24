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



#ifdef HAVE_ZLIB
#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

#include <zlib.h>
#endif

#ifdef __GNUC__
#include <unistd.h>
#endif

#define ZWAD

#ifdef ZWAD
#include <errno.h>
#include "lzf.h"
#endif

#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"

#include "w_wad.h"
#include "z_zone.h"
#include "fastcmp.h"

#include "filesrch.h"

#include "d_main.h"
#include "netcode/d_netfil.h"
#include "netcode/d_clisrv.h"
#include "dehacked.h"
#include "r_defs.h"
#include "r_data.h"
#include "r_textures.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "r_translation.h"
#include "i_time.h"
#include "i_system.h"
#include "i_threads.h"
#include "i_video.h"  
#include "md5.h"
#include "lua_script.h"
#include "lua_hook.h"
#ifdef SCANTHINGS
#include "p_setup.h"  
#endif
#include "m_misc.h"  
#include "g_game.h"  

#ifdef HWRENDER
#include "hardware/hw_main.h"
#include "hardware/hw_glob.h"
#endif

#ifdef _DEBUG
#include "console.h"
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif


typedef struct
{
	const char *name;
	size_t len;
} lumpchecklist_t;




#define LUMPNUMCACHEWAYS 4
#define LUMPNUMCACHESETS 256
#define LUMPNUMCACHESIZE (LUMPNUMCACHEWAYS * LUMPNUMCACHESETS)

typedef struct lumpnum_cache_s
{
	char lumpname[32];
	lumpnum_t lumpnum;
	UINT32 hash;
	boolean longname;
} lumpnum_cache_t;

static lumpnum_cache_t lumpnumcache[LUMPNUMCACHESIZE];
static UINT8 lumpnumcachenext[LUMPNUMCACHESETS];

#define MAPNUMCACHEWAYS 4
#define MAPNUMCACHESETS 32
#define MAPNUMCACHESIZE (MAPNUMCACHEWAYS * MAPNUMCACHESETS)
typedef struct mapnum_cache_s
{
	char name[9];
	lumpnum_t lumpnum;
	UINT32 hash;
	boolean valid;
} mapnum_cache_t;
static mapnum_cache_t mapnumcache[MAPNUMCACHESIZE];
static UINT8 mapnumcachenext[MAPNUMCACHESETS];




UINT16 numwadfiles;  
wadfile_t **wadfiles;  



static UINT8 *lumpheaderbatch;
static size_t lumpheaderbatchsize;
static UINT16 lumpheaderbatchwad = UINT16_MAX;
static I_mutex wadio_mutex;

static void W_EndLumpHeaderBatchUnlocked(void)
{
	if (lumpheaderbatch)
		free(lumpheaderbatch);

	lumpheaderbatch = NULL;
	lumpheaderbatchsize = 0;
	lumpheaderbatchwad = UINT16_MAX;
}

void W_EndLumpHeaderBatch(void)
{
	I_lock_mutex(&wadio_mutex);
	W_EndLumpHeaderBatchUnlocked();
	I_unlock_mutex(wadio_mutex);
}

boolean W_BeginLumpHeaderBatch(UINT16 wad)
{
	FILE *handle;
	long oldposition;
	size_t filesize;
	boolean success = false;

	I_lock_mutex(&wadio_mutex);
	W_EndLumpHeaderBatchUnlocked();

	if (wad >= numwadfiles || wadfiles[wad]->type == RET_FOLDER)
		goto done;

	handle = wadfiles[wad]->handle;
	filesize = wadfiles[wad]->filesize;
	if (!handle || !filesize)
		goto done;

	lumpheaderbatch = malloc(filesize);
	if (!lumpheaderbatch)
		goto done;

	oldposition = ftell(handle);
	if (fseek(handle, 0, SEEK_SET) != 0
		|| fread(lumpheaderbatch, 1, filesize, handle) != filesize)
	{
		clearerr(handle);
		W_EndLumpHeaderBatchUnlocked();
		if (oldposition >= 0)
			fseek(handle, oldposition, SEEK_SET);
		goto done;
	}

	if (oldposition >= 0)
		fseek(handle, oldposition, SEEK_SET);

	lumpheaderbatchsize = filesize;
	lumpheaderbatchwad = wad;
	success = true;

done:
	I_unlock_mutex(wadio_mutex);
	return success;
}






void W_Shutdown(void)
{
	W_EndLumpHeaderBatch();

	while (numwadfiles--)
	{
		wadfile_t *wad = wadfiles[numwadfiles];

		if (wad->handle)
			fclose(wad->handle);
		Z_Free(wad->filename);
		if (wad->path)
			Z_Free(wad->path);
		while (wad->numlumps--)
		{
			if (wad->lumpinfo[wad->numlumps].diskpath)
				Z_Free(wad->lumpinfo[wad->numlumps].diskpath);
			Z_Free(wad->lumpinfo[wad->numlumps].longname);
			Z_Free(wad->lumpinfo[wad->numlumps].fullname);
		}
		M_AATreeFree(wad->startfolders);
		M_AATreeFree(wad->endfolders);
		M_AATreeFree(wad->fullnamelumps);

		Z_Free(wad->lumpinfo);
		Z_Free(wad);
	}

	Z_Free(wadfiles);
}













static char filenamebuf[MAX_WADPATH];






FILE *W_OpenWadFile(const char **filename, boolean useerrors)
{
	FILE *handle;




	if (filenamebuf != *filename)
	{
		strncpy(filenamebuf, *filename, MAX_WADPATH);
		filenamebuf[MAX_WADPATH - 1] = '\0';
		*filename = filenamebuf;
	}


	if ((handle = fopen(*filename, "rb")) == NULL)
	{


		nameonly(filenamebuf);



		if (findfile(filenamebuf, NULL, true))
		{
			if ((handle = fopen(*filename, "rb")) == NULL)
			{
				if (useerrors)
					CONS_Alert(CONS_ERROR, M_GetText("Can't open %s\n"), *filename);
				return NULL;
			}
		}
		else
		{
			if (useerrors)
				CONS_Alert(CONS_ERROR, M_GetText("File %s not found.\n"), *filename);
			return NULL;
		}
	}
	return handle;
}


static void W_LoadDehackedLumpsPK3(UINT16 wadnum, boolean mainfile)
{
	UINT16 posStart, posEnd;

	posStart = W_CheckNumForFullNamePK3("Init.lua", wadnum, 0);
	if (posStart != INT16_MAX)
	{
		LUA_DoLump(wadnum, posStart, true);
	}
	else
	{
		posStart = W_CheckNumForFolderStartPK3("Lua/", wadnum, 0);
		if (posStart != INT16_MAX)
		{
			posEnd = W_CheckNumForFolderEndPK3("Lua/", wadnum, posStart);
			for (; posStart < posEnd; posStart++)
				LUA_DoLump(wadnum, posStart, true);
		}
	}

	posStart = W_CheckNumForFolderStartPK3("SOC/", wadnum, 0);
	if (posStart != INT16_MAX)
	{
		posEnd = W_CheckNumForFolderEndPK3("SOC/", wadnum, posStart);

		for(; posStart < posEnd; posStart++)
		{
			lumpinfo_t *lump_p = &wadfiles[wadnum]->lumpinfo[posStart];
			size_t length = strlen(wadfiles[wadnum]->filename) + 1 + strlen(lump_p->fullname);  
			char *name = malloc(length + 1);
			sprintf(name, "%s|%s", wadfiles[wadnum]->filename, lump_p->fullname);
			name[length] = '\0';
			CONS_Printf(M_GetText("Loading SOC from %s\n"), name);
			DEH_LoadDehackedLumpPwad(wadnum, posStart, mainfile);
			free(name);
		}
	}
}


static void W_LoadDehackedLumps(UINT16 wadnum, boolean mainfile)
{
	UINT16 lump;


	{
		lumpinfo_t *lump_p = wadfiles[wadnum]->lumpinfo;
		for (lump = 0; lump < wadfiles[wadnum]->numlumps; lump++, lump_p++)
			if (memcmp(lump_p->name,"LUA_",4)==0)
				LUA_DoLump(wadnum, lump, true);
	}

	{
		lumpinfo_t *lump_p = wadfiles[wadnum]->lumpinfo;
		for (lump = 0; lump < wadfiles[wadnum]->numlumps; lump++, lump_p++)
			if (memcmp(lump_p->name,"SOC_",4)==0)  
			{	 
				size_t length = strlen(wadfiles[wadnum]->filename) + 1 + strlen(lump_p->fullname);  
				char *name = malloc(length + 1);
				sprintf(name, "%s|%s", wadfiles[wadnum]->filename, lump_p->fullname);
				name[length] = '\0';

				CONS_Printf(M_GetText("Loading SOC from %s\n"), name);
				DEH_LoadDehackedLumpPwad(wadnum, lump, mainfile);
				free(name);
			}
			else if (memcmp(lump_p->name,"MAINCFG",8)==0)  
			{
				CONS_Printf(M_GetText("Loading main config from %s\n"), wadfiles[wadnum]->filename);
				DEH_LoadDehackedLumpPwad(wadnum, lump, mainfile);
			}
			else if (memcmp(lump_p->name,"OBJCTCFG",8)==0)  
			{
				CONS_Printf(M_GetText("Loading object config from %s\n"), wadfiles[wadnum]->filename);
				DEH_LoadDehackedLumpPwad(wadnum, lump, mainfile);
			}
	}

#ifdef SCANTHINGS

	{
		lumpinfo_t *lump_p = wadfiles[wadnum]->lumpinfo;
		for (lump = 0; lump < wadfiles[wadnum]->numlumps; lump++, lump_p++)
		{
			const char *name = lump_p->name;
			if (name[0] == 'M' && name[1] == 'A' && name[2] == 'P' && name[5]=='\0')
			{
				INT16 mapnum = (INT16)M_MapNumber(name[3], name[4]);
				P_ScanThings(mapnum, wadnum, lump + ML_THINGS);
			}
		}
	}
#endif
}

                                                                          
   
                                                                         
                          
   
                                
                                          
                                                                                
    

#ifndef NOMD5
static INT32 W_MakeFileMD5(const char *filename, void *resblock)
{
	FILE *fhandle;

	if ((fhandle = fopen(filename, "rb")) != NULL)
	{
		tic_t t = I_GetTime();
		CONS_Debug(DBG_SETUP, "Making MD5 for %s\n",filename);
		if (md5_stream(fhandle, resblock) == 1)
		{
			fclose(fhandle);
			return 1;
		}
		CONS_Debug(DBG_SETUP, "MD5 calc for %s took %f seconds\n",
			filename, (float)(I_GetTime() - t)/NEWTICRATE);
		fclose(fhandle);
		return 0;
	}
	return 1;
}
#endif


static void W_InvalidateLumpnumCache(void)
{
	memset(lumpnumcache, 0, sizeof (lumpnumcache));
	memset(lumpnumcachenext, 0, sizeof (lumpnumcachenext));
	memset(mapnumcache, 0, sizeof (mapnumcache));
	memset(mapnumcachenext, 0, sizeof (mapnumcachenext));
}

                       
                                                                                                    
   
static restype_t ResourceFileDetect (const char* filename)
{
	if (!stricmp(&filename[strlen(filename) - 4], ".pk3"))
		return RET_PK3;
	if (!stricmp(&filename[strlen(filename) - 4], ".soc"))
		return RET_SOC;
	if (!stricmp(&filename[strlen(filename) - 4], ".lua"))
		return RET_LUA;

	return RET_WAD;
}

                                                    
   
static lumpinfo_t* ResGetLumpsStandalone (FILE* handle, UINT16* numlumps, const char* lumpname)
{
	lumpinfo_t* lumpinfo = Z_Calloc(sizeof (*lumpinfo), PU_STATIC, NULL);
	lumpinfo->position = 0;
	fseek(handle, 0, SEEK_END);
	lumpinfo->size = ftell(handle);
	fseek(handle, 0, SEEK_SET);
	strcpy(lumpinfo->name, lumpname);
	lumpinfo->hash = quickncasehash(lumpname, 8);


	lumpinfo->longname = Z_Malloc(9 * sizeof(char), PU_STATIC, NULL);
	strcpy(lumpinfo->longname, lumpname);
	lumpinfo->longname[8] = '\0';


	lumpinfo->fullname = Z_Malloc(9 * sizeof(char), PU_STATIC, NULL);
	strcpy(lumpinfo->fullname, lumpname);
	lumpinfo->fullname[8] = '\0';

	*numlumps = 1;
	return lumpinfo;
}

                                             
   
static lumpinfo_t* ResGetLumpsWad (FILE* handle, UINT16* nlmp, const char* filename)
{
	UINT16 numlumps = *nlmp;
	lumpinfo_t* lumpinfo;
	size_t i;
	INT32 compressed = 0;

	wadinfo_t header;
	lumpinfo_t *lump_p;
	filelump_t *fileinfo;
	void *fileinfov;


	if (fread(&header, 1, sizeof header, handle) < sizeof header)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Can't read wad header because %s\n"), M_FileError(handle));
		return NULL;
	}

	if (memcmp(header.identification, "ZWAD", 4) == 0)
		compressed = 1;
	else if (memcmp(header.identification, "IWAD", 4) != 0
		&& memcmp(header.identification, "PWAD", 4) != 0
		&& memcmp(header.identification, "SDLL", 4) != 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Invalid WAD header\n"));
		return NULL;
	}

	header.numlumps = LONG(header.numlumps);
	header.infotableofs = LONG(header.infotableofs);


	i = header.numlumps * sizeof (*fileinfo);
	fileinfov = fileinfo = malloc(i);
	if (fseek(handle, header.infotableofs, SEEK_SET) == -1
		|| fread(fileinfo, 1, i, handle) < i)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Corrupt wadfile directory (%s)\n"), M_FileError(handle));
		free(fileinfov);
		return NULL;
	}

	numlumps = header.numlumps;


	lump_p = lumpinfo = Z_Malloc(numlumps * sizeof (*lumpinfo), PU_STATIC, NULL);
	for (i = 0; i < numlumps; i++, lump_p++, fileinfo++)
	{
		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = lump_p->disksize = LONG(fileinfo->size);
		lump_p->diskpath = NULL;
		if (compressed)  
		{
			UINT32 realsize = 0;
			if (fseek(handle, lump_p->position, SEEK_SET)
				== -1 || fread(&realsize, 1, sizeof realsize,
				handle) < sizeof realsize)
			{
				I_Error("corrupt compressed file: %s; maybe %s",  
					filename, M_FileError(handle));
			}
			realsize = LONG(realsize);
			if (realsize != 0)
			{
				lump_p->size = realsize;
				lump_p->compression = CM_LZF;
			}
			else
			{
				lump_p->size -= 4;
				lump_p->compression = CM_NOCOMPRESSION;
			}

			lump_p->position += 4;
			lump_p->disksize -= 4;
		}
		else
			lump_p->compression = CM_NOCOMPRESSION;
		memset(lump_p->name, 0x00, 9);
		strncpy(lump_p->name, fileinfo->name, 8);
		lump_p->hash = quickncasehash(lump_p->name, 8);


		lump_p->longname = Z_Malloc(9 * sizeof(char), PU_STATIC, NULL);
		strncpy(lump_p->longname, fileinfo->name, 8);
		lump_p->longname[8] = '\0';


		lump_p->fullname = Z_Malloc(9 * sizeof(char), PU_STATIC, NULL);
		strncpy(lump_p->fullname, fileinfo->name, 8);
		lump_p->fullname[8] = '\0';
	}
	free(fileinfov);
	*nlmp = numlumps;
	return lumpinfo;
}

                                       
   
static boolean ResFindSignature (FILE* handle, char endPat[], UINT32 startpos)
{
	char *s;
	int c;

	fseek(handle, startpos, SEEK_SET);
	s = endPat;
	while((c = fgetc(handle)) != EOF)
	{
		if (*s != c && s > endPat)  
			s = endPat;  
		if (*s == c)
		{
			s++;
			if (*s == 0x00)  
			{
				return true;
			}
		}
	}
	return false;
}

#if defined(_MSC_VER)
#pragma pack(1)
#endif
typedef struct zend_s
{
	char signature[4];
	UINT16 diskpos;
	UINT16 cdirdisk;
	UINT16 diskentries;
	UINT16 entries;
	UINT32 cdirsize;
	UINT32 cdiroffset;
	UINT16 commentlen;
} ATTRPACK zend_t;

typedef struct zentry_s
{
	char signature[4];
	UINT16 version;
	UINT16 versionneeded;
	UINT16 flags;
	UINT16 compression;
	UINT16 modtime;
	UINT16 moddate;
	UINT32 CRC32;
	UINT32 compsize;
	UINT32 size;
	UINT16 namelen;
	UINT16 xtralen;
	UINT16 commlen;
	UINT16 diskstart;
	UINT16 attrint;
	UINT32 attrext;
	UINT32 offset;
} ATTRPACK zentry_t;

typedef struct zlentry_s
{
	char signature[4];
	UINT16 versionneeded;
	UINT16 flags;
	UINT16 compression;
	UINT16 modtime;
	UINT16 moddate;
	UINT32 CRC32;
	UINT32 compsize;
	UINT32 size;
	UINT16 namelen;
	UINT16 xtralen;
} ATTRPACK zlentry_t;
#if defined(_MSC_VER)
#pragma pack()
#endif

                                               
   
static lumpinfo_t* ResGetLumpsZip (FILE* handle, UINT16* nlmp)
{
    zend_t zend;
    zentry_t zentry;
    zlentry_t zlentry;

	UINT16 numlumps = *nlmp;
	lumpinfo_t* lumpinfo;
	lumpinfo_t *lump_p;
	size_t i;

	char pat_central[] = {0x50, 0x4b, 0x01, 0x02, 0x00};
	char pat_end[] = {0x50, 0x4b, 0x05, 0x06, 0x00};



	fseek(handle, 0, SEEK_END);
	if (!ResFindSignature(handle, pat_end, max(0, ftell(handle) - (22 + 65536))))
	{
		CONS_Alert(CONS_ERROR, "Missing central directory\n");
		return NULL;
	}

	fseek(handle, -4, SEEK_CUR);
	if (fread(&zend, 1, sizeof zend, handle) < sizeof zend)
	{
		CONS_Alert(CONS_ERROR, "Corrupt central directory (%s)\n", M_FileError(handle));
		return NULL;
	}
	numlumps = zend.entries;

	lump_p = lumpinfo = Z_Malloc(numlumps * sizeof (*lumpinfo), PU_STATIC, NULL);

	fseek(handle, zend.cdiroffset, SEEK_SET);
	for (i = 0; i < numlumps; i++, lump_p++)
	{
		char* fullname;
		char* trimname;
		char* dotpos;

		if (fread(&zentry, 1, sizeof(zentry_t), handle) < sizeof(zentry_t))
		{
			CONS_Alert(CONS_ERROR, "Failed to read central directory (%s)\n", M_FileError(handle));
			Z_Free(lumpinfo);
			return NULL;
		}
		if (memcmp(zentry.signature, pat_central, 4))
		{
			CONS_Alert(CONS_ERROR, "Central directory is corrupt\n");
			Z_Free(lumpinfo);
			return NULL;
		}

		lump_p->position = zentry.offset;  
		lump_p->disksize = zentry.compsize;
		lump_p->diskpath = NULL;
		lump_p->size = zentry.size;

		fullname = malloc(zentry.namelen + 1);
		if (fgets(fullname, zentry.namelen + 1, handle) != fullname)
		{
			CONS_Alert(CONS_ERROR, "Unable to read lumpname (%s)\n", M_FileError(handle));
			Z_Free(lumpinfo);
			free(fullname);
			return NULL;
		}


		if ((trimname = strrchr(fullname, '/')) != 0)
			trimname++;
		else
			trimname = fullname;  

		if ((dotpos = strrchr(trimname, '.')) == 0)
			dotpos = fullname + strlen(fullname);  

		memset(lump_p->name, '\0', 9);  
		strncpy(lump_p->name, trimname, min(8, dotpos - trimname));
		lump_p->hash = quickncasehash(lump_p->name, 8);

		lump_p->longname = Z_Calloc(dotpos - trimname + 1, PU_STATIC, NULL);
		strlcpy(lump_p->longname, trimname, dotpos - trimname + 1);

		lump_p->fullname = Z_Calloc(zentry.namelen + 1, PU_STATIC, NULL);
		strncpy(lump_p->fullname, fullname, zentry.namelen);

		switch(zentry.compression)
		{
		case 0:
			lump_p->compression = CM_NOCOMPRESSION;
			break;
#ifdef HAVE_ZLIB
		case 8:
			lump_p->compression = CM_DEFLATE;
			break;
#endif
		case 14:
			lump_p->compression = CM_LZF;
			break;
		default:
			CONS_Alert(CONS_WARNING, "%s: Unsupported compression method\n", fullname);
			lump_p->compression = CM_UNSUPPORTED;
			break;
		}

		free(fullname);


		if (fseek(handle, zentry.xtralen + zentry.commlen, SEEK_CUR) != 0)
		{
			CONS_Alert(CONS_ERROR, "Central directory is corrupt\n");
			Z_Free(lumpinfo);
			return NULL;
		}
	}


	for (i = 0, lump_p = lumpinfo; i < numlumps; i++, lump_p++)
	{

		if ((fseek(handle, lump_p->position, SEEK_SET) != 0) || (fread(&zlentry, 1, sizeof(zlentry_t), handle) < sizeof(zlentry_t)))
		{
			CONS_Alert(CONS_ERROR, "Local headers for lump %s are corrupt\n", lump_p->fullname);
			Z_Free(lumpinfo);
			return NULL;
		}

		lump_p->position += sizeof(zlentry_t) + zlentry.namelen + zlentry.xtralen;
	}

	*nlmp = numlumps;
	return lumpinfo;
}

static INT32 CheckPathsNotEqual(const char *path1, const char *path2)
{
	INT32 stat = samepaths(path1, path2);

	if (stat == 1)
		return 0;
	else if (stat < 0)
		return -1;

	return 1;
}


INT32 W_IsPathToFolderValid(const char *path)
{
	INT32 stat;


	const char *p = path + (strlen(path) - 1);
	while (*p == '\\' || *p == '/' || *p == ':')
	{
		p--;
		if (p < path)
			return 0;
	}


	stat = pathisdirectory(path);
	if (stat == 0)
		return 0;
	else if (stat < 0)
	{

		if (direrror == ENOENT)
			return 0;

		return -1;
	}


	stat = CheckPathsNotEqual(path, srb2home);
	if (stat != 1)
		return stat;


	stat = CheckPathsNotEqual(path, srb2path);
	if (stat != 1)
		return stat;

	stat = CheckPathsNotEqual(path, ".");
	if (stat != 1)
		return stat;

	return 1;
}



static char *CheckConcatFolderPath(const char *startpath, const char *path)
{
	if (concatpaths(path, startpath) == 1)
	{
		char *fn;

		if (startpath)
		{
			size_t len = strlen(startpath) + strlen(path) + strlen(PATHSEP) + 1;
			fn = ZZ_Alloc(len);
			snprintf(fn, len, "%s" PATHSEP "%s", startpath, path);
		}
		else
			fn = Z_StrDup(path);

		return fn;
	}

	return NULL;
}



char *W_GetFullFolderPath(const char *path)
{

	char *fn = CheckConcatFolderPath(NULL, path);
	if (fn)
		return fn;

#define checkpath(startpath) \
	fn = CheckConcatFolderPath(startpath, path); \
	if (fn) \
		return fn

	checkpath(srb2home);  
	checkpath(srb2path);  
	checkpath(".");  

#undef checkpath

	return NULL;
}


static lumpinfo_t *ResGetLumpsFolder(const char *path, UINT16 *nlmp, UINT16 *nfolders)
{
	return getdirectoryfiles(path, nlmp, nfolders);
}

static UINT16 W_InitFileError (const char *filename, boolean exitworthy)
{
	if (exitworthy)
	{
#ifdef _DEBUG
		CONS_Error(va("%s was not found or not valid.\nCheck the log for more details.\n", filename));
#else
		I_Error("%s was not found or not valid.\nCheck the log for more details.\n", filename);
#endif
	}
	else
		CONS_Printf(M_GetText("Errors occurred while loading %s; not added.\n"), filename);
	return INT16_MAX;
}

static void W_ReadFileShaders(wadfile_t *wadfile)
{
#ifdef HWRENDER
	if (rendermode == render_opengl && (vid.glstate == VID_GL_LIBRARY_LOADED))
		HWR_LoadCustomShadersFromFile(numwadfiles - 1, W_FileHasFolders(wadfile));
#else
	(void)wadfile;
#endif
}

static void W_LoadTrnslateLumps(UINT16 w)
{
	UINT16 lump = W_CheckNumForNamePwad("TRNSLATE", w, 0);
	while (lump != INT16_MAX)
	{
		R_ParseTrnslate(w, lump);
		lump = W_CheckNumForNamePwad("TRNSLATE", (UINT16)w, lump + 1);
	}
}












UINT16 W_InitFileVerified(const char *filename, boolean mainfile, boolean startup,
	int verification, const UINT8 *knownmd5)
{
	FILE *handle;
	lumpinfo_t *lumpinfo = NULL;
	wadfile_t *wadfile;
	restype_t type;
	UINT16 numlumps = 0;
#ifndef NOMD5
	size_t i;
#endif
	UINT8 md5sum[16];
	int important;

	if (!(refreshdirmenu & REFRESHDIR_ADDFILE))
		refreshdirmenu = REFRESHDIR_NORMAL|REFRESHDIR_ADDFILE;  

	if (refreshdirname)
		Z_Free(refreshdirname);
	if (dirmenu)
	{
		refreshdirname = Z_StrDup(filename);
		nameonly(refreshdirname);
	}
	else
		refreshdirname = NULL;




	if (numwadfiles >= MAX_WADFILES)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Maximum wad files reached\n"));
		refreshdirmenu |= REFRESHDIR_MAX;
		return W_InitFileError(filename, startup);
	}


	if ((handle = W_OpenWadFile(&filename, true)) == NULL)
		return W_InitFileError(filename, startup);





	if (mainfile)
		important = false;
	else
	{
		important = (verification == W_VERIFY_UNKNOWN)
			? W_VerifyNMUSlumps(filename, startup)
			: verification;

		if (important == -1)
		{
			fclose(handle);
			return INT16_MAX;
		}

		important = !important;
	}

#ifndef NOMD5





	if (knownmd5)
		M_Memcpy(md5sum, knownmd5, sizeof(md5sum));
	else if (mainfile || !D_GetCachedFileMD5(filename, md5sum))
		W_MakeFileMD5(filename, md5sum);

	for (i = 0; i < numwadfiles; i++)
	{
		if (wadfiles[i]->type == RET_FOLDER)
			continue;

		if (!memcmp(wadfiles[i]->md5sum, md5sum, 16))
		{
			CONS_Alert(CONS_ERROR, M_GetText("%s is already loaded\n"), filename);
			if (handle)
				fclose(handle);
			return W_InitFileError(filename, false);
		}
	}
#endif

	switch(type = ResourceFileDetect(filename))
	{
	case RET_SOC:
		lumpinfo = ResGetLumpsStandalone(handle, &numlumps, "OBJCTCFG");
		break;
	case RET_LUA:
		lumpinfo = ResGetLumpsStandalone(handle, &numlumps, "LUA_INIT");
		break;
	case RET_PK3:
		lumpinfo = ResGetLumpsZip(handle, &numlumps);
		break;
	case RET_WAD:
		lumpinfo = ResGetLumpsWad(handle, &numlumps, filename);
		break;
	default:
		CONS_Alert(CONS_ERROR, "Unsupported file format\n");
	}

	if (lumpinfo == NULL)
	{
		fclose(handle);
		return W_InitFileError(filename, startup);
	}

	if (important && !mainfile)
	{

		modifiedgame = true;  
	}




	wadfile = Z_Malloc(sizeof (*wadfile), PU_STATIC, NULL);
	wadfile->filename = Z_StrDup(filename);
	wadfile->path = NULL;
	wadfile->type = type;
	wadfile->handle = handle;
	wadfile->numlumps = numlumps;
	wadfile->foldercount = 0;
	wadfile->lumpinfo = lumpinfo;
	wadfile->important = important && !mainfile;
	fseek(handle, 0, SEEK_END);
	wadfile->filesize = (unsigned)ftell(handle);
	wadfile->type = type;
	wadfile->startfolders = M_AATreeAlloc(0);
	wadfile->endfolders = M_AATreeAlloc(0);
	wadfile->fullnamelumps = M_AATreeAlloc(0);


	M_Memcpy(&wadfile->md5sum, &md5sum, 16);




	Z_Calloc(numlumps * sizeof (*wadfile->lumpcache), PU_STATIC, &wadfile->lumpcache);
	Z_Calloc(numlumps * sizeof (*wadfile->patchcache), PU_STATIC, &wadfile->patchcache);




	CONS_Printf(M_GetText("Added file %s (%u lumps)\n"), filename, numlumps);
	wadfiles = Z_Realloc(wadfiles, sizeof(wadfile_t *) * (numwadfiles + 1), PU_STATIC, NULL);
	wadfiles[numwadfiles] = wadfile;
	numwadfiles++;  


	W_ReadFileShaders(wadfile);


	W_LoadTrnslateLumps(numwadfiles - 1);


	switch (wadfile->type)
	{
	case RET_WAD:
		W_LoadDehackedLumps(numwadfiles - 1, mainfile);
		break;
	case RET_PK3:
		W_LoadDehackedLumpsPK3(numwadfiles - 1, mainfile);
		break;
	case RET_SOC:
		CONS_Printf(M_GetText("Loading SOC from %s\n"), wadfile->filename);
		DEH_LoadDehackedLumpPwad(numwadfiles - 1, 0, mainfile);
		break;
	case RET_LUA:
		LUA_DoLump(numwadfiles - 1, 0, true);
		break;
	default:
		break;
	}

	lua_lumploading++;
	LUA_HookVoid(HOOK(AddonLoaded));
	lua_lumploading--;

	W_InvalidateLumpnumCache();
	return wadfile->numlumps;
}

UINT16 W_InitFile(const char *filename, boolean mainfile, boolean startup)
{
	return W_InitFileVerified(filename, mainfile, startup, W_VERIFY_UNKNOWN, NULL);
}




UINT16 W_InitFolder(const char *path, boolean mainfile, boolean startup)
{
	lumpinfo_t *lumpinfo = NULL;
	wadfile_t *wadfile;
	UINT16 numlumps = 0;
	UINT16 foldercount;
	size_t i;
	char *fn, *fullpath;
	const char *p;
	int important;
	INT32 stat;

	if (!(refreshdirmenu & REFRESHDIR_ADDFILE))
		refreshdirmenu = REFRESHDIR_NORMAL|REFRESHDIR_ADDFILE;  

	if (refreshdirname)
		Z_Free(refreshdirname);
	if (dirmenu)
		refreshdirname = Z_StrDup(path);
	else
		refreshdirname = NULL;

	if (numwadfiles >= MAX_WADFILES)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Maximum wad files reached\n"));
		refreshdirmenu |= REFRESHDIR_MAX;
		return W_InitFileError(path, startup);
	}

	important = 1;  


	p = path + (strlen(path) - 1);

	while (*p == '\\' || *p == '/' || *p == ':')
	{
		p--;
		if (p < path)
		{
			CONS_Alert(CONS_ERROR, M_GetText("Path %s is invalid\n"), path);
			return W_InitFileError(path, startup);
		}
	}
	p++;


	i = (p - path) + 1;
	fn = ZZ_Alloc(i);
	strlcpy(fn, path, i);


	if (M_IsStringEmpty(fn))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Folder name is empty\n"));
		Z_Free(fn);

		if (startup)
			return W_InitFileError("A folder", true);
		else
			return W_InitFileError("a folder", false);
	}


	stat = W_IsPathToFolderValid(fn);

	if (stat != 1)
	{
		if (stat == 0)
			CONS_Alert(CONS_ERROR, M_GetText("Path %s is invalid\n"), fn);
		else if (stat < 0)
		{
#ifndef AVOID_ERRNO
			CONS_Alert(CONS_ERROR, M_GetText("Could not stat %s: %s\n"), fn, strerror(direrror));
#else
			CONS_Alert(CONS_ERROR, M_GetText("Could not stat %s\n"), fn);
#endif
		}

		Z_Free(fn);
		return W_InitFileError(path, startup);
	}


	fullpath = W_GetFullFolderPath(fn);
	if (fullpath == NULL)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Path %s is invalid\n"), fn);
		Z_Free(fn);
		return W_InitFileError(path, startup);
	}


	for (i = 0; i < numwadfiles; i++)
	{
		if (wadfiles[i]->type != RET_FOLDER)
			continue;

		if (samepaths(wadfiles[i]->path, fullpath) > 0)
		{
			CONS_Alert(CONS_ERROR, M_GetText("%s is already loaded\n"), path);
			Z_Free(fn);
			Z_Free(fullpath);
			return W_InitFileError(path, false);
		}
	}

	lumpinfo = ResGetLumpsFolder(fullpath, &numlumps, &foldercount);

	if (lumpinfo == NULL)
	{
		if (!numlumps)
			CONS_Alert(CONS_ERROR, M_GetText("Folder %s is empty\n"), path);
		else if (numlumps == UINT16_MAX)
			CONS_Alert(CONS_ERROR, M_GetText("Folder %s contains too many files\n"), path);
		else
			CONS_Alert(CONS_ERROR, M_GetText("Unknown error enumerating files from folder %s\n"), path);

		Z_Free(fn);
		Z_Free(fullpath);

		return W_InitFileError(path, startup);
	}

	if (important && !mainfile)
		G_SetGameModified(true);

	wadfile = Z_Malloc(sizeof (*wadfile), PU_STATIC, NULL);
	wadfile->filename = fn;
	wadfile->path = fullpath;
	wadfile->type = RET_FOLDER;
	wadfile->handle = NULL;
	wadfile->numlumps = numlumps;
	wadfile->foldercount = foldercount;
	wadfile->lumpinfo = lumpinfo;
	wadfile->important = important;
	wadfile->filesize = 0;
	wadfile->startfolders = M_AATreeAlloc(0);
	wadfile->endfolders = M_AATreeAlloc(0);
	wadfile->fullnamelumps = M_AATreeAlloc(0);

	for (i = 0; i < numlumps; i++)
		wadfile->filesize += lumpinfo[i].disksize;

	memset(wadfile->md5sum, 0x00, 16);  

	Z_Calloc(numlumps * sizeof (*wadfile->lumpcache), PU_STATIC, &wadfile->lumpcache);
	Z_Calloc(numlumps * sizeof (*wadfile->patchcache), PU_STATIC, &wadfile->patchcache);

	CONS_Printf(M_GetText("Added folder %s (%u files, %u folders)\n"), fn, numlumps, foldercount);
	wadfiles = Z_Realloc(wadfiles, sizeof(wadfile_t *) * (numwadfiles + 1), PU_STATIC, NULL);
	wadfiles[numwadfiles] = wadfile;
	numwadfiles++;

	W_ReadFileShaders(wadfile);
	W_LoadTrnslateLumps(numwadfiles - 1);
	W_LoadDehackedLumpsPK3(numwadfiles - 1, mainfile);

	lua_lumploading++;
	LUA_HookVoid(HOOK(AddonLoaded));
	lua_lumploading--;

	W_InvalidateLumpnumCache();

	return wadfile->numlumps;
}

                                    
                                                                         
   
                                                                               
                                                                         
                                                          
   
                                                            
    
void W_InitMultipleFiles(addfilelist_t *list)
{
	size_t i = 0;

	for (; i < list->numfiles; i++)
	{
		const char *fn = list->files[i];
		char pathsep = fn[strlen(fn) - 1];
		boolean mainfile = (numwadfiles < mainwads);



		if (pathsep == '\\' || pathsep == '/')
			W_InitFolder(fn, mainfile, true);
		else
			W_InitFile(fn, mainfile, true);
	}
}

                                     
                                                        
    
static boolean TestValidLump(UINT16 wad, UINT16 lump)
{
	I_Assert(wad < numwadfiles);
	if (!wadfiles[wad])  
		return false;

	I_Assert(lump < wadfiles[wad]->numlumps);
	if (lump >= wadfiles[wad]->numlumps)  
		return false;

	return true;
}


const char *W_CheckNameForNumPwad(UINT16 wad, UINT16 lump)
{
	if (lump >= wadfiles[wad]->numlumps || !TestValidLump(wad, 0))
		return NULL;

	return wadfiles[wad]->lumpinfo[lump].name;
}

const char *W_CheckNameForNum(lumpnum_t lumpnum)
{
	return W_CheckNameForNumPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum));
}








UINT16 W_CheckNumForNamePwad(const char *name, UINT16 wad, UINT16 startlump)
{
	UINT16 i;
	static char uname[8 + 1];
	UINT32 hash;

	if (!TestValidLump(wad,0))
		return INT16_MAX;

	strlcpy(uname, name, sizeof uname);
	strupr(uname);
	hash = quickncasehash(uname, 8);






	if (startlump < wadfiles[wad]->numlumps)
	{
		lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
		for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
			if (lump_p->hash == hash && !strncmp(lump_p->name, uname, sizeof(uname) - 1))
				return i;
	}


	return INT16_MAX;
}







UINT16 W_CheckNumForLongNamePwad(const char *name, UINT16 wad, UINT16 startlump)
{
	UINT16 i;
	static char uname[256 + 1];

	if (!TestValidLump(wad,0))
		return INT16_MAX;

	strlcpy(uname, name, sizeof uname);
	strupr(uname);






	if (startlump < wadfiles[wad]->numlumps)
	{
		lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
		for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
			if (!strcmp(lump_p->longname, uname))
				return i;
	}


	return INT16_MAX;
}

UINT16
W_CheckNumForMarkerStartPwad (const char *name, UINT16 wad, UINT16 startlump)
{
	UINT16 marker;
	marker = W_CheckNumForNamePwad(name, wad, startlump);
	if (marker != INT16_MAX)
		marker++;  
	return marker;
}


UINT16 W_CheckNumForFolderStartPK3(const char *name, UINT16 wad, UINT16 startlump)
{
	size_t name_length;
	INT32 i;
	lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
	name_length = strlen(name);
	UINT32 hash = quickncasehash(name, name_length);
	void *val = M_AATreeGet(wadfiles[wad]->startfolders, hash);

	if (val != NULL)
		return (uintptr_t)val;

	for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
	{
		if (strnicmp(name, lump_p->fullname, name_length) == 0)
		{
			 
			if (strlen(lump_p->fullname) == name_length)
				i++;
			M_AATreeSet(wadfiles[wad]->startfolders, hash, (void *)(uintptr_t)i);
			return i;
		}
	}
	M_AATreeSet(wadfiles[wad]->startfolders, hash, (void *)INT16_MAX);
	return INT16_MAX;
}




UINT16 W_CheckNumForFolderEndPK3(const char *name, UINT16 wad, UINT16 startlump)
{
	INT32 i;
	lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
	size_t name_length = strlen(name);
	UINT32 hash = quickncasehash(name, name_length);
	void *val = M_AATreeGet(wadfiles[wad]->endfolders, hash);

	if (val != NULL)
		return (uintptr_t)val;

	for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
	{
		if (strnicmp(name, lump_p->fullname, name_length))
			break;
	}
	M_AATreeSet(wadfiles[wad]->endfolders, hash, (void *)(uintptr_t)i);
	return i;
}

char *W_GetLumpFolderPathPK3(UINT16 wad, UINT16 lump)
{
	const char *fullname = wadfiles[wad]->lumpinfo[lump].fullname;

	const char *slash = strrchr(fullname, '/');
	INT32 pathlen = slash ? slash - fullname : 0;

	char *path = Z_Calloc(pathlen + 1, PU_STATIC, NULL);
	strncpy(path, fullname, pathlen);

	return path;
}

char *W_GetLumpFolderNamePK3(UINT16 wad, UINT16 lump)
{
	const char *fullname = wadfiles[wad]->lumpinfo[lump].fullname;
	size_t start, end;

	INT32 i = strlen(fullname);

	i--;
	while (i >= 0 && fullname[i] != '/')
		i--;
	if (i < 0)
		return NULL;
	end = i;

	i--;
	while (i >= 0 && fullname[i] != '/')
		i--;
	if (i < 0)
		return NULL;
	start = i + 1;

	size_t namelen = end - start;
	char *foldername = Z_Calloc(namelen + 1, PU_STATIC, NULL);
	strncpy(foldername, fullname + start, namelen);

	return foldername;
}

void W_GetFolderLumpsPwad(const char *name, UINT16 wad, UINT32 **list, UINT16 *list_capacity, UINT16 *numlumps)
{
	size_t name_length = strlen(name);
	lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo;

	UINT16 capacity = list_capacity ? *list_capacity : 0;
	UINT16 count = *numlumps;

	for (UINT16 i = 0; i < wadfiles[wad]->numlumps; i++, lump_p++)
	{
		if (strnicmp(name, lump_p->fullname, name_length) == 0)
		{
			if (strlen(lump_p->fullname) > name_length
				&& lump_p->longname[0] != '\0')
			{
				if (!capacity || count >= capacity)
				{
					capacity = capacity ? (capacity * 2) : 16;
					*list = Z_Realloc(*list, capacity * sizeof(UINT32), PU_STATIC, NULL);
				}

				(*list)[count] = (wad << 16) + i;
				count++;
			}
		}
	}

	if (list_capacity)
		(*list_capacity) = capacity;
	(*numlumps) = count;
}

void W_GetFolderLumps(const char *name, UINT32 **list, UINT16 *list_capacity, UINT16 *numlumps)
{
	for (UINT16 i = 0; i < numwadfiles; i++)
		W_GetFolderLumpsPwad(name, i, list, list_capacity, numlumps);
}

UINT32 W_CountFolderLumpsPwad(const char *name, UINT16 wad)
{
	size_t name_length = strlen(name);
	lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo;

	UINT32 count = 0;

	for (UINT16 i = 0; i < wadfiles[wad]->numlumps; i++, lump_p++)
	{
		if (strnicmp(name, lump_p->fullname, name_length) == 0)
		{
			if (strlen(lump_p->fullname) > name_length
				&& lump_p->longname[0] != '\0')
				count++;
		}
	}

	return count;
}

UINT32 W_CountFolderLumps(const char *name)
{
	UINT32 count = 0;

	for (UINT16 i = 0; i < numwadfiles; i++)
		count += W_CountFolderLumpsPwad(name, i);

	return count;
}



UINT16 W_CheckNumForFullNamePK3(const char *name, UINT16 wad, UINT16 startlump)
{
	INT32 i;
	const size_t name_length = strlen(name);
	const UINT32 hash = quickncasehash(name, name_length);
	void *cached = NULL;
	lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;

	if (!startlump)
	{
		cached = M_AATreeGet(wadfiles[wad]->fullnamelumps, hash);
		if (cached)
		{
			const UINT16 result = (UINT16)((uintptr_t)cached - 1);
			if (result == INT16_MAX
			|| (result < wadfiles[wad]->numlumps
			&& !strnicmp(name, wadfiles[wad]->lumpinfo[result].fullname, name_length)))
				return result;
			 
		}
	}
	for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
	{
		if (!strnicmp(name, lump_p->fullname, name_length))
		{
			if (!startlump)
				M_AATreeSet(wadfiles[wad]->fullnamelumps, hash,
					(void *)(uintptr_t)(i + 1));
			return i;
		}
	}

	if (!startlump)
		M_AATreeSet(wadfiles[wad]->fullnamelumps, hash,
			(void *)(uintptr_t)(INT16_MAX + 1));
	return INT16_MAX;
}

static lumpnum_t CheckLumpInCache(const char *name, boolean longname)
{
	UINT32 hash = quickncasehash(name, longname ? 32 : 8);
	UINT32 set = hash & (LUMPNUMCACHESETS - 1);
	UINT32 first = set * LUMPNUMCACHEWAYS;
	UINT32 way;

	if (longname)
	{
		for (way = 0; way < LUMPNUMCACHEWAYS; way++)
		{
			lumpnum_cache_t *entry = &lumpnumcache[first + way];
			if (entry->longname && entry->hash == hash
				&& stricmp(entry->lumpname, name) == 0)
			{
				return entry->lumpnum;
			}
		}
	}
	else
	{
		for (way = 0; way < LUMPNUMCACHEWAYS; way++)
		{
			lumpnum_cache_t *entry = &lumpnumcache[first + way];
			if (!entry->longname && entry->hash == hash
				&& entry->lumpname[8] == '\0'
				&& strnicmp(entry->lumpname, name, 8) == 0)
			{
				return entry->lumpnum;
			}
		}
	}

	return LUMPERROR;
}

static void AddLumpToCache(lumpnum_t lumpnum, const char *name, boolean longname)
{
	UINT32 hash;
	UINT32 set;
	UINT32 slot;
	lumpnum_cache_t *entry;

	if (longname && strlen(name) >= 32)
		return;

	hash = quickncasehash(name, longname ? 32 : 8);
	set = hash & (LUMPNUMCACHESETS - 1);
	slot = set * LUMPNUMCACHEWAYS + lumpnumcachenext[set];
	lumpnumcachenext[set] = (lumpnumcachenext[set] + 1) % LUMPNUMCACHEWAYS;
	entry = &lumpnumcache[slot];

	memset(entry->lumpname, '\0', sizeof entry->lumpname);
	if (longname)
	{
		strlcpy(entry->lumpname, name, sizeof entry->lumpname);
	}
	else
	{
		strncpy(entry->lumpname, name, 8);
	}
	entry->hash = hash;
	entry->lumpnum = lumpnum;
	entry->longname = longname;
}





lumpnum_t W_CheckNumForName(const char *name)
{
	INT32 i;
	lumpnum_t check = INT16_MAX;

	if (!*name)  
		return LUMPERROR;


	lumpnum_t cachenum = CheckLumpInCache(name, false);
	if (cachenum != LUMPERROR)
		return cachenum;


	for (i = numwadfiles - 1; i >= 0; i--)
	{
		check = W_CheckNumForNamePwad(name,(UINT16)i,0);
		if (check != INT16_MAX)
			break;  
	}

	if (check == INT16_MAX) return LUMPERROR;
	else
	{

		lumpnum_t lumpnum = (i << 16) + check;

		AddLumpToCache(lumpnum, name, false);

		return lumpnum;
	}
}







lumpnum_t W_CheckNumForLongName(const char *name)
{
	INT32 i;
	lumpnum_t check = INT16_MAX;

	if (!*name)  
		return LUMPERROR;


	lumpnum_t cachenum = CheckLumpInCache(name, true);
	if (cachenum != LUMPERROR)
		return cachenum;


	for (i = numwadfiles - 1; i >= 0; i--)
	{
		check = W_CheckNumForLongNamePwad(name,(UINT16)i,0);
		if (check != INT16_MAX)
			break;  
	}

	if (check == INT16_MAX) return LUMPERROR;
	else
	{

		lumpnum_t lumpnum = (i << 16) + check;

		AddLumpToCache(lumpnum, name, true);

		return lumpnum;
	}
}



lumpnum_t W_CheckNumForMap(const char *name)
{
	UINT32 hash = quickncasehash(name, 8);
	UINT16 lumpNum, end;
	UINT32 i;
	lumpinfo_t *p;
	const UINT32 set = hash & (MAPNUMCACHESETS - 1);
	const UINT32 first = set * MAPNUMCACHEWAYS;
	lumpnum_t result = LUMPERROR;
	UINT32 way;

	for (way = 0; way < MAPNUMCACHEWAYS; way++)
	{
		mapnum_cache_t *entry = &mapnumcache[first + way];
		if (entry->valid && entry->hash == hash
		&& !strnicmp(entry->name, name, 8))
			return entry->lumpnum;
	}

	for (i = numwadfiles - 1; i < numwadfiles; i--)
	{
		if (wadfiles[i]->type == RET_WAD)
		{
			for (lumpNum = 0; lumpNum < wadfiles[i]->numlumps; lumpNum++)
			{
				p = wadfiles[i]->lumpinfo + lumpNum;
				if (p->hash == hash && !strncmp(name, p->name, 8))
				{
					result = (i<<16) + lumpNum;
					goto done;
				}
			}
		}
		else if (W_FileHasFolders(wadfiles[i]))
		{
			lumpNum = W_CheckNumForFolderStartPK3("maps/", i, 0);
			if (lumpNum != INT16_MAX)
				end = W_CheckNumForFolderEndPK3("maps/", i, lumpNum);
			else
				continue;

			for (; lumpNum < end; lumpNum++)
			{
				p = wadfiles[i]->lumpinfo + lumpNum;
				if (p->hash == hash && !strnicmp(name, p->name, 8))
				{
					const char *extension = strrchr(p->fullname, '.');
					if (!(extension && stricmp(extension, ".wad")))
					{
						result = (i<<16) + lumpNum;
						goto done;
					}
				}
			}
		}
	}

done:
	{
		const UINT32 slot = first + mapnumcachenext[set];
		mapnum_cache_t *entry = &mapnumcache[slot];
		mapnumcachenext[set] = (mapnumcachenext[set] + 1) % MAPNUMCACHEWAYS;
		memset(entry->name, 0, sizeof entry->name);
		strncpy(entry->name, name, 8);
		entry->hash = hash;
		entry->lumpnum = result;
		entry->valid = true;
	}
	return result;
}






lumpnum_t W_GetNumForName(const char *name)
{
	lumpnum_t i;

	i = W_CheckNumForName(name);

	if (i == LUMPERROR)
		I_Error("W_GetNumForName: %s not found!\n", name);

	return i;
}







lumpnum_t W_GetNumForLongName(const char *name)
{
	lumpnum_t i;

	i = W_CheckNumForLongName(name);

	if (i == LUMPERROR)
		I_Error("W_GetNumForLongName: %s not found!\n", name);

	return i;
}




static boolean W_IsProbablyValidPatch(UINT16 wadnum, UINT16 lumpnum)
{
	UINT8 header[MIN_PATCH_LUMP_HEADER_SIZE] = {0};
	size_t lumplen = W_LumpLengthPwad(wadnum, lumpnum);
	softwarepatch_t patch;
	INT16 width, height;
	UINT32 ofs;

	I_StaticAssert(sizeof(header) >= PNG_HEADER_SIZE);

	if (lumplen < MIN_PATCH_LUMP_SIZE)
		return false;

	if (lumplen >= PNG_MIN_SIZE)
	{
		if (W_ReadLumpHeaderPwad(wadnum, lumpnum, header, PNG_HEADER_SIZE, 0)
			!= PNG_HEADER_SIZE)
			return false;
		if (Picture_IsLumpPNG(header, lumplen))
			return true;
	}

	if (W_ReadLumpHeaderPwad(wadnum, lumpnum, header, sizeof(header), 0)
		!= sizeof(header))
		return false;
	memcpy(&patch, header, sizeof(header));

	width = SHORT(patch.width);
	height = SHORT(patch.height);
	if (!VALID_PATCH_LUMP_SIZE(lumplen, width))
		return false;

	if (width <= 0 || height <= 0 || width > MAX_PATCH_DIMENSIONS || height > MAX_PATCH_DIMENSIONS)
		return false;

	ofs = LONG(patch.columnofs[0]);
	if (ofs < FIRST_PATCH_LUMP_COLUMN(width) || (size_t)ofs >= lumplen)
		return false;

	return true;
}




static UINT16 W_CheckNumForPatchNamePwad(const char *name, UINT16 wad, boolean longname)
{
	UINT16 i;
	static char uname[8 + 1] = { 0 };
	UINT32 hash = 0;
	lumpinfo_t *lump_p;

	if (!TestValidLump(wad,0))
		return INT16_MAX;

	if (!longname)
	{
		strlcpy(uname, name, sizeof uname);
		strupr(uname);
		hash = quickncasehash(uname, 8);
	}

	lump_p = wadfiles[wad]->lumpinfo;

	for (i = 0; i < wadfiles[wad]->numlumps; i++, lump_p++)
	{
		if ((!longname && lump_p->hash == hash && !strncmp(lump_p->name, uname, sizeof(uname) - 1))
		|| (longname && stricmp(lump_p->longname, name) == 0))
		{
			if (W_IsProbablyValidPatch(wad, i))
				return i;
		}
	}


	return INT16_MAX;
}





static lumpnum_t W_CheckNumForPatchNameInternal(const char *name, boolean longname)
{
	INT32 i;
	lumpnum_t check = INT16_MAX;

	if (!*name)  
		return LUMPERROR;


	lumpnum_t cachenum = CheckLumpInCache(name, longname);
	if (cachenum != LUMPERROR)
		return cachenum;


	for (i = numwadfiles - 1; i >= 0; i--)
	{
		check = W_CheckNumForPatchNamePwad(name,(UINT16)i,longname);
		if (check != INT16_MAX)
			break;  
	}

	if (check == INT16_MAX) return LUMPERROR;
	else
	{

		lumpnum_t lumpnum = (i << 16) + check;

		AddLumpToCache(lumpnum, name, longname);

		return lumpnum;
	}
}





lumpnum_t W_CheckNumForPatchName(const char *name)
{
	return W_CheckNumForPatchNameInternal(name, false);
}





lumpnum_t W_CheckNumForLongPatchName(const char *name)
{
	return W_CheckNumForPatchNameInternal(name, true);
}






lumpnum_t W_GetNumForPatchName(const char *name)
{
	lumpnum_t i;

	i = W_CheckNumForPatchName(name);

	if (i == LUMPERROR)
		I_Error("W_CheckNumForPatchName: %s not found!\n", name);

	return i;
}




lumpnum_t W_GetNumForLongPatchName(const char *name)
{
	lumpnum_t i;

	i = W_CheckNumForLongPatchName(name);

	if (i == LUMPERROR)
		I_Error("W_GetNumForLongPatchName: %s not found!\n", name);

	return i;
}





lumpnum_t W_CheckNumForNameInBlock(const char *name, const char *blockstart, const char *blockend)
{
	INT32 i;
	lumpnum_t bsid, beid;
	lumpnum_t check = INT16_MAX;


	for (i = numwadfiles - 1; i >= 0; i--)
	{
		if (wadfiles[i]->type == RET_WAD)
		{
			bsid = W_CheckNumForNamePwad(blockstart, (UINT16)i, 0);
			if (bsid == INT16_MAX)
				continue;  
			beid = W_CheckNumForNamePwad(blockend, (UINT16)i, 0);
			if (beid == INT16_MAX)
				continue;  

			check = W_CheckNumForNamePwad(name, (UINT16)i, bsid);
			if (check < beid)
				return (i<<16)+check;  
		}
	}
	return LUMPERROR;
}


#include "fastcmp.h"
UINT8 W_LumpExists(const char *name)
{
	INT32 i,j;
	for (i = numwadfiles - 1; i >= 0; i--)
	{
		lumpinfo_t *lump_p = wadfiles[i]->lumpinfo;
		for (j = 0; j < wadfiles[i]->numlumps; ++j, ++lump_p)
			if (fastcmp(lump_p->longname, name))
				return true;
	}
	return false;
}

size_t W_LumpLengthPwad(UINT16 wad, UINT16 lump)
{
	lumpinfo_t *l;

	if (!TestValidLump(wad, lump))
		return 0;

	l = wadfiles[wad]->lumpinfo + lump;


	if (wadfiles[wad]->type == RET_FOLDER)
	{


		INT32 stat = pathisdirectory(l->diskpath);

		if (stat < 0)
		{
#ifndef AVOID_ERRNO
			if (direrror == ENOENT)
				I_Error("W_LumpLengthPwad: file %s doesn't exist", l->diskpath);
			else
				I_Error("W_LumpLengthPwad: could not stat %s: %s", l->diskpath, strerror(direrror));
#else
			I_Error("W_LumpLengthPwad: could not access %s", l->diskpath);
#endif
		}
		else if (stat == 1)  
			return 0;
		else
		{
			FILE *handle = fopen(l->diskpath, "rb");
			if (handle == NULL)
				I_Error("W_LumpLengthPwad: could not open file %s", l->diskpath);

			fseek(handle, 0, SEEK_END);
			l->size = l->disksize = ftell(handle);
			fclose(handle);
		}
	}

	return l->size;
}

                                                          
   
                                       
                                         
    
size_t W_LumpLength(lumpnum_t lumpnum)
{
	return W_LumpLengthPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum));
}





boolean W_IsLumpWad(lumpnum_t lumpnum)
{
	if (W_FileHasFolders(wadfiles[WADFILENUM(lumpnum)]))
	{
		const char *lumpfullName = (wadfiles[WADFILENUM(lumpnum)]->lumpinfo + LUMPNUM(lumpnum))->fullname;

		if (strlen(lumpfullName) < 4)
			return false;  
		return !strnicmp(lumpfullName + strlen(lumpfullName) - 4, ".wad", 4);
	}

	return false;  
}





boolean W_IsLumpFolder(UINT16 wad, UINT16 lump)
{
	if (W_FileHasFolders(wadfiles[wad]))
	{
		const char *name = wadfiles[wad]->lumpinfo[lump].fullname;

		return (name[strlen(name)-1] == '/');  
	}

	return false;  
}

#ifdef HAVE_ZLIB
 
void zerr(int ret)
{
    CONS_Printf("zpipe: ");
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            CONS_Printf("error reading stdin\n");
        if (ferror(stdout))
            CONS_Printf("error writing stdout\n");
        break;
    case Z_STREAM_ERROR:
        CONS_Printf("invalid compression level\n");
        break;
    case Z_DATA_ERROR:
        CONS_Printf("invalid or incomplete deflate data\n");
        break;
    case Z_MEM_ERROR:
        CONS_Printf("out of memory\n");
        break;
    case Z_VERSION_ERROR:
        CONS_Printf("zlib version mismatch!\n");
    }
}
#endif

#ifdef NO_PNG_LUMPS
#define Picture_ThrowPNGError(lumpname, wadfilename) I_Error("W_Wad: Lump \"%s\" in file \"%s\" is a .png - please convert to either Doom or Flat (raw) image format.", lumpname, wadfilename)
#endif

                                        
                                                                           
   
                                       
                                         
                                                         
                                        
                                            
                                                     
                                       
    
static size_t W_ReadLumpHeaderPwadUnlocked(UINT16 wad, UINT16 lump, void *dest, size_t size, size_t offset)
{
	size_t lumpsize, bytesread;
	lumpinfo_t *l;
	FILE *handle = NULL;
	UINT8 *batchsource = NULL;

	if (!TestValidLump(wad, lump))
		return 0;

	l = wadfiles[wad]->lumpinfo + lump;

	if (lumpheaderbatch && wad == lumpheaderbatchwad
		&& l->position <= lumpheaderbatchsize
		&& l->disksize <= lumpheaderbatchsize - l->position)
		batchsource = lumpheaderbatch + l->position;


	if (wadfiles[wad]->type == RET_FOLDER)
	{


		INT32 stat = pathisdirectory(l->diskpath);

		if (stat < 0)
		{
#ifndef AVOID_ERRNO
			if (direrror == ENOENT)
				I_Error("W_ReadLumpHeaderPwad: file %s doesn't exist", l->diskpath);
			else
				I_Error("W_ReadLumpHeaderPwad: could not stat %s: %s", l->diskpath, strerror(direrror));
#else
			I_Error("W_ReadLumpHeaderPwad: could not access %s", l->diskpath);
#endif
		}
		else if (stat == 1)  
			return 0;
		else
		{
			handle = fopen(l->diskpath, "rb");
			if (handle == NULL)
				I_Error("W_ReadLumpHeaderPwad: could not open file %s", l->diskpath);


			fseek(handle, 0, SEEK_END);
			l->size = l->disksize = ftell(handle);
		}
	}

	lumpsize = wadfiles[wad]->lumpinfo[lump].size;

	if (!lumpsize || lumpsize<offset)
	{
		if (wadfiles[wad]->type == RET_FOLDER)
			fclose(handle);
		return 0;
	}


	if (!size || size+offset > lumpsize)
		size = lumpsize - offset;



	if (wadfiles[wad]->type != RET_FOLDER)
		handle = wadfiles[wad]->handle;


	switch(wadfiles[wad]->lumpinfo[lump].compression)
	{
	case CM_NOCOMPRESSION:		 
		if (batchsource)
		{
			M_Memcpy(dest, batchsource + offset, size);
			return size;
		}
		if (fseek(handle, (long)(l->position + offset), SEEK_SET) != 0)
		{
			if (wadfiles[wad]->type == RET_FOLDER)
				fclose(handle);
			return 0;
		}
		bytesread = fread(dest, 1, size, handle);
		if (wadfiles[wad]->type == RET_FOLDER)
			fclose(handle);
		return bytesread;
	case CM_LZF:		 
		{
#ifdef ZWAD
			char *rawData;  
			char *decData;  
			size_t retval;  

			rawData = Z_Malloc(l->disksize, PU_STATIC, NULL);
			decData = Z_Malloc(l->size, PU_STATIC, NULL);

			if (batchsource)
				M_Memcpy(rawData, batchsource, l->disksize);
			else if (fseek(handle, (long)l->position, SEEK_SET) != 0
				|| fread(rawData, 1, l->disksize, handle) < l->disksize)
				I_Error("wad %d, lump %d: cannot read compressed data", wad, lump);
			retval = lzf_decompress(rawData, l->disksize, decData, l->size);
#ifndef AVOID_ERRNO
			if (retval == 0)  
			{

				if (errno == E2BIG)
					I_Error("wad %d, lump %d: compressed data too big (bigger than %s)", wad, lump, sizeu1(l->size));
				else if (errno == EINVAL)
					I_Error("wad %d, lump %d: invalid compressed data", wad, lump);
			}

#endif
			if (retval != l->size)
			{
				I_Error("wad %d, lump %d: decompressed to wrong number of bytes (expected %s, got %s)", wad, lump, sizeu1(l->size), sizeu2(retval));
			}

			if (!decData)  
				return 0;
			M_Memcpy(dest, decData + offset, size);
			Z_Free(rawData);
			Z_Free(decData);
			return size;
#else

			return 0;
#endif

		}
#ifdef HAVE_ZLIB
	case CM_DEFLATE:  
		{
			UINT8 *rawData = NULL;
			UINT8 *decData = dest;
			const UINT8 *source = batchsource;
			int zErr = Z_OK;
			z_stream strm;
			unsigned long rawSize = l->disksize;

			unsigned long decSize = size + offset;

			if (!source)
			{
				rawData = Z_Malloc(rawSize, PU_STATIC, NULL);
				if (fseek(handle, (long)l->position, SEEK_SET) != 0
					|| fread(rawData, 1, rawSize, handle) != rawSize)
				{
					CONS_Alert(CONS_ERROR,
						"Could not read compressed lump %s from %s (wad %u, lump %u).\n",
						l->fullname ? l->fullname : l->name,
						wadfiles[wad]->filename, wad, lump);
					Z_Free(rawData);
					if (wadfiles[wad]->type == RET_FOLDER)
						fclose(handle);
					return 0;
				}
				source = rawData;
			}

			if (offset)
				decData = Z_Malloc(decSize, PU_STATIC, NULL);

			memset(&strm, 0, sizeof strm);
			strm.next_in = (Bytef *)source;
			strm.avail_in = rawSize;
			strm.next_out = decData;
			strm.avail_out = decSize;

			zErr = inflateInit2(&strm, -15);
			if (zErr == Z_OK)
			{
				do
				{
					zErr = inflate(&strm, Z_NO_FLUSH);
				}
				while (zErr == Z_OK && strm.total_out < decSize && strm.avail_in);

				if (strm.total_out == decSize
					&& (zErr == Z_OK || zErr == Z_STREAM_END || zErr == Z_BUF_ERROR))
				{
					if (offset)
						M_Memcpy(dest, decData + offset, size);
				}
				else
				{
					CONS_Alert(CONS_ERROR,
						"Could not decompress lump %s from %s (wad %u, lump %u; zlib %d, %lu/%lu bytes).\n",
						l->fullname ? l->fullname : l->name,
						wadfiles[wad]->filename, wad, lump, zErr,
						(unsigned long)strm.total_out, decSize);
					size = 0;
				}

				(void)inflateEnd(&strm);
			}
			else
			{
				CONS_Alert(CONS_ERROR,
					"Could not initialize decompression for lump %s from %s (zlib %d).\n",
					l->fullname ? l->fullname : l->name,
					wadfiles[wad]->filename, zErr);
				size = 0;
			}

			if (rawData)
				Z_Free(rawData);
			if (offset)
				Z_Free(decData);
			if (wadfiles[wad]->type == RET_FOLDER)
				fclose(handle);

			return size;
		}
#endif
	default:
		I_Error("wad %d, lump %d: unsupported compression type!", wad, lump);
	}
	return 0;
}

size_t W_ReadLumpHeaderPwad(UINT16 wad, UINT16 lump, void *dest, size_t size, size_t offset)
{
	size_t bytesread;

	I_lock_mutex(&wadio_mutex);
	bytesread = W_ReadLumpHeaderPwadUnlocked(wad, lump, dest, size, offset);
	I_unlock_mutex(wadio_mutex);

	return bytesread;
}

size_t W_ReadLumpHeader(lumpnum_t lumpnum, void *dest, size_t size, size_t offset)
{
	return W_ReadLumpHeaderPwad(WADFILENUM(lumpnum), LUMPNUM(lumpnum), dest, size, offset);
}

                             
   
                                         
                                                                         
                               
                        
    
void W_ReadLump(lumpnum_t lumpnum, void *dest)
{
	const size_t expected = W_LumpLength(lumpnum);
	if (W_ReadLumpHeaderPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum),dest,0,0) != expected)
		I_Error("W_ReadLump: could not read lump %u:%u (%s)",
			WADFILENUM(lumpnum), LUMPNUM(lumpnum),
			W_CheckNameForNum(lumpnum));
}

void W_ReadLumpPwad(UINT16 wad, UINT16 lump, void *dest)
{
	const size_t expected = W_LumpLengthPwad(wad, lump);
	if (W_ReadLumpHeaderPwad(wad, lump, dest, 0, 0) != expected)
		I_Error("W_ReadLumpPwad: could not read lump %u:%u (%s) from %s",
			wad, lump, W_CheckNameForNumPwad(wad, lump), wadfiles[wad]->filename);
}




void *W_CacheLumpNumPwad(UINT16 wad, UINT16 lump, INT32 tag)
{
	lumpcache_t *lumpcache;

	if (!TestValidLump(wad,lump))
		return NULL;

	lumpcache = wadfiles[wad]->lumpcache;
	if (!lumpcache[lump])
	{
		const size_t expected = W_LumpLengthPwad(wad, lump);
		void *ptr = Z_Malloc(expected, tag, &lumpcache[lump]);
		if (W_ReadLumpHeaderPwad(wad, lump, ptr, 0, 0) != expected)
		{
			Z_Free(ptr);  
			I_Error("W_CacheLumpNumPwad: could not read lump %u:%u (%s) from %s",
				wad, lump, W_CheckNameForNumPwad(wad, lump), wadfiles[wad]->filename);
		}
	}
	else
		Z_ChangeTag(lumpcache[lump], tag);

	return lumpcache[lump];
}

void *W_CacheLumpNum(lumpnum_t lumpnum, INT32 tag)
{
	return W_CacheLumpNumPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum),tag);
}






void *W_CacheLumpNumForce(lumpnum_t lumpnum, INT32 tag)
{
	UINT16 wad, lump;
	void *ptr;

	wad = WADFILENUM(lumpnum);
	lump = LUMPNUM(lumpnum);

	if (!TestValidLump(wad,lump))
		return NULL;

	{
		const size_t expected = W_LumpLengthPwad(wad, lump);
		ptr = Z_Malloc(expected, tag, NULL);
		if (W_ReadLumpHeaderPwad(wad, lump, ptr, 0, 0) != expected)
		{
			Z_Free(ptr);
			I_Error("W_CacheLumpNumForce: could not read lump %u:%u (%s) from %s",
				wad, lump, W_CheckNameForNumPwad(wad, lump), wadfiles[wad]->filename);
		}
	}

	return ptr;
}








static boolean W_IsLumpCachedPWAD(UINT16 wad, UINT16 lump, void *ptr)
{
	void *lcache;

	if (!TestValidLump(wad, lump))
		return false;

	lcache = wadfiles[wad]->lumpcache[lump];

	if (ptr)
	{
		if (ptr == lcache)
			return true;
	}
	else if (lcache)
		return true;

	return false;
}

boolean W_IsLumpCached(lumpnum_t lumpnum, void *ptr)
{
	return W_IsLumpCachedPWAD(WADFILENUM(lumpnum),LUMPNUM(lumpnum), ptr);
}







boolean W_IsPatchCachedPwad(UINT16 wad, UINT16 lump, void *ptr)
{
	void *lcache;

	if (!TestValidLump(wad, lump))
		return false;

	lcache = wadfiles[wad]->patchcache[lump];

	if (ptr)
	{
		if (ptr == lcache)
			return true;
	}
	else if (lcache)
		return true;

	return false;
}

boolean W_IsPatchCached(lumpnum_t lumpnum, void *ptr)
{
	return W_IsPatchCachedPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum), ptr);
}




void *W_CacheLumpName(const char *name, INT32 tag)
{
	return W_CacheLumpNum(W_GetNumForName(name), tag);
}





                                                                              
                                                                              
                                                                          
            
static patch_t *W_CreateInvalidGraphicPlaceholder(void)
{
	UINT8 pixel = TRANSPARENTPIXEL;
	return (patch_t *)Picture_PatchConvert(PICFMT_FLAT, sizeof pixel, &pixel,
		PICFMT_PATCH, NULL, 1, 1, 0, 0, PICFLAGS_USE_TRANSPARENTPIXEL);
}




static void *W_GetPatchPwad(UINT16 wad, UINT16 lump, INT32 tag)
{
	lumpcache_t *lumpcache = NULL;

	if (!TestValidLump(wad, lump))
		return NULL;

	lumpcache = wadfiles[wad]->patchcache;

	if (!lumpcache[lump])
	{
		size_t len = W_LumpLengthPwad(wad, lump);
		void *ptr, *dest, *lumpdata = Z_Malloc(len, PU_STATIC, NULL);


		if (W_ReadLumpHeaderPwad(wad, lump, lumpdata, 0, 0) != len)
		{
			CONS_Alert(CONS_WARNING,
				"Using a transparent placeholder for unreadable graphic %s from %s.\n",
				W_CheckNameForNumPwad(wad, lump), wadfiles[wad]->filename);
			Z_Free(lumpdata);
			ptr = W_CreateInvalidGraphicPlaceholder();
			if (!ptr)
				return NULL;
			Z_ChangeTag(ptr, tag);
			Z_SetUser(ptr, &lumpcache[lump]);
			return lumpcache[lump];
		}
		ptr = lumpdata;

		if (Picture_IsLumpPNG((UINT8 *)lumpdata, len))
		{
#ifndef NO_PNG_LUMPS
			const size_t pnglen = len;
#ifdef HWRENDER
			{
				RGBA_t *hardware_pixels = NULL;
				patch_t *pngpatch;

				ptr = Picture_PNGConvertWithRGBA((UINT8 *)lumpdata,
					PICFMT_PATCH, NULL, NULL, NULL, NULL, pnglen, &len, 0,
					&hardware_pixels);
				if (!ptr)
				{
					Z_Free(lumpdata);
					ptr = W_CreateInvalidGraphicPlaceholder();
					if (!ptr)
						return NULL;
					Z_ChangeTag(ptr, tag);
					Z_SetUser(ptr, &lumpcache[lump]);
					return lumpcache[lump];
				}
				pngpatch = (patch_t *)ptr;
				pngpatch->hardware_pixels = hardware_pixels;
				Z_ChangeTag(pngpatch->hardware_pixels, PU_PATCH_DATA);

				                                                            
                                                                     
                                                                    
                                                                    
                                                                  
				pngpatch->hardware_palette_pixels = NULL;
			}
#else
			ptr = Picture_PNGConvert((UINT8 *)lumpdata, PICFMT_PATCH,
				NULL, NULL, NULL, NULL, pnglen, &len, 0);
			if (!ptr)
			{
				Z_Free(lumpdata);
				ptr = W_CreateInvalidGraphicPlaceholder();
				if (!ptr)
					return NULL;
				Z_ChangeTag(ptr, tag);
				Z_SetUser(ptr, &lumpcache[lump]);
				return lumpcache[lump];
			}
#endif
			Z_ChangeTag(ptr, tag);
			Z_SetUser(ptr, &lumpcache[lump]);
			Z_Free(lumpdata);
			return lumpcache[lump];
#else
			Picture_ThrowPNGError(W_CheckNameForNumPwad(wad, lump), wadfiles[wad]->filename);
			return NULL;
#endif
		}

		dest = Patch_CreateFromDoomPatch(ptr, len);
		Z_Free(ptr);
		if (dest == NULL)
		{
			CONS_Alert(CONS_WARNING,
				"Using a transparent placeholder for malformed graphic %s from %s.\n",
				W_CheckNameForNumPwad(wad, lump), wadfiles[wad]->filename);
			dest = W_CreateInvalidGraphicPlaceholder();
			if (dest == NULL)
				return NULL;
		}

		Z_ChangeTag(dest, tag);
		Z_SetUser(dest, &lumpcache[lump]);
	}
	else
		Z_ChangeTag(lumpcache[lump], tag);

	return lumpcache[lump];
}

void *W_CachePatchNumPwad(UINT16 wad, UINT16 lump, INT32 tag)
{
	if (!TestValidLump(wad, lump))
		return NULL;

	patch_t *patch = W_GetPatchPwad(wad, lump, tag);

#ifdef HWRENDER
	if (patch != NULL && rendermode == render_opengl)
		Patch_CreateGL(patch);
#endif

	return (void *)patch;
}

void *W_CachePatchNum(lumpnum_t lumpnum, INT32 tag)
{
	return W_CachePatchNumPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum),tag);
}

void *W_GetCachedPatchNumPwad(UINT16 wad, UINT16 lump)
{
	if (!TestValidLump(wad, lump))
		return NULL;

	return wadfiles[wad]->patchcache[lump];
}

boolean W_ReadPatchHeaderPwad(UINT16 wadnum, UINT16 lumpnum, INT16 *width, INT16 *height, INT16 *topoffset, INT16 *leftoffset)
{
	UINT8 header[PNG_DIMENSIONS_HEADER_SIZE];
	size_t headerbytes;

	if (!TestValidLump(wadnum, lumpnum))
		return false;

	headerbytes = W_ReadLumpHeaderPwad(wadnum, lumpnum, header, sizeof header, 0);
	if (headerbytes < sizeof(INT16) * 4)
		return false;

	size_t len = W_LumpLengthPwad(wadnum, lumpnum);

	if (headerbytes >= PNG_DIMENSIONS_HEADER_SIZE && Picture_IsLumpPNG(header, len))
	{
#ifndef NO_PNG_LUMPS
		INT32 pwidth = 0, pheight = 0;
		UINT8 *png;



		if (!topoffset && !leftoffset
			&& Picture_PNGDimensionsFromHeader(header, len, &pwidth, &pheight))
		{
			*width = (INT16)pwidth;
			*height = (INT16)pheight;
			return true;
		}

		png = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);

		if (!Picture_PNGDimensions(png, &pwidth, &pheight, topoffset, leftoffset, len))
		{
			Z_Free(png);
			return false;
		}

		*width = (INT16)pwidth;
		*height = (INT16)pheight;

		Z_Free(png);

		return true;
#else
		Picture_ThrowPNGError(W_CheckNameForNumPwad(wadnum, lumpnum), wadfiles[wadnum]->filename);

		return false;
#endif
	}

	softwarepatch_t patch;
	M_Memcpy(&patch, header, sizeof(INT16) * 4);

	*width = SHORT(patch.width);
	*height = SHORT(patch.height);
	if (topoffset)
		*topoffset = SHORT(patch.topoffset);
	if (leftoffset)
		*leftoffset = SHORT(patch.leftoffset);

	return true;
}

boolean W_ReadPatchHeader(lumpnum_t lumpnum, INT16 *width, INT16 *height, INT16 *topoffset, INT16 *leftoffset)
{
	return W_ReadPatchHeaderPwad(WADFILENUM(lumpnum), LUMPNUM(lumpnum), width, height, topoffset, leftoffset);
}

void W_UnlockCachedPatch(void *patch)
{
	if (!patch)
		return;



#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_UnlockCachedPatch((GLPatch_t *)((patch_t *)patch)->hardware);
	else
#endif
		Z_Unlock(patch);
}

void *W_CachePatchName(const char *name, INT32 tag)
{
	lumpnum_t num;

	num = W_CheckNumForPatchName(name);

	if (num == LUMPERROR)
		return W_CachePatchNum(W_GetNumForPatchName("MISSING"), tag);
	return W_CachePatchNum(num, tag);
}

void *W_CachePatchLongName(const char *name, INT32 tag)
{
	lumpnum_t num;

	num = W_CheckNumForLongPatchName(name);

	if (num == LUMPERROR)
		return W_CachePatchNum(W_GetNumForLongPatchName("MISSING"), tag);
	return W_CachePatchNum(num, tag);
}
#ifndef NOMD5
#define MD5_LEN 16

   
                                                              
   
                                                            
                                                                             
                                       
    
#define MD5_FORMAT \
	"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
static void PrintMD5String(const UINT8 *md5, char *buf)
{
	snprintf(buf, 2*MD5_LEN+1, MD5_FORMAT,
		md5[0], md5[1], md5[2], md5[3],
		md5[4], md5[5], md5[6], md5[7],
		md5[8], md5[9], md5[10], md5[11],
		md5[12], md5[13], md5[14], md5[15]);
}
#endif
                                             
                                                                         
                                                                         
            
   
                                                             
                                                                      
                                     
                                       
    
void W_VerifyFileMD5(UINT16 wadfilenum, const char *matchmd5)
{
#ifdef NOMD5
	(void)wadfilenum;
	(void)matchmd5;
#else
	UINT8 realmd5[MD5_LEN];
	INT32 ix;

	I_Assert(strlen(matchmd5) == 2*MD5_LEN);
	I_Assert(wadfilenum < numwadfiles);


	for (ix = 0; ix < 2*MD5_LEN; ix++)
	{
		INT32 n, c = matchmd5[ix];
		if (isdigit(c))
			n = c - '0';
		else
		{
			I_Assert(isxdigit(c));
			if (isupper(c)) n = c - 'A' + 10;
			else n = c - 'a' + 10;
		}
		if (ix & 1) realmd5[ix>>1] = (UINT8)(realmd5[ix>>1]+n);
		else realmd5[ix>>1] = (UINT8)(n<<4);
	}

	if (memcmp(realmd5, wadfiles[wadfilenum]->md5sum, 16))
	{
		char actualmd5text[2*MD5_LEN+1];
		PrintMD5String(wadfiles[wadfilenum]->md5sum, actualmd5text);
#ifdef _DEBUG
		CONS_Printf
#else
		I_Error
#endif
			(M_GetText("File is old, is corrupt or has been modified:\n%s\nFound MD5: %s\nWanted MD5: %s\n"), wadfiles[wadfilenum]->filename, actualmd5text, matchmd5);
	}
#endif
}




static int
W_VerifyName (const char *name, lumpchecklist_t *checklist, boolean status)
{
	size_t j;
	for (j = 0; checklist[j].len && checklist[j].name; ++j)
	{
		if (( strncasecmp(name, checklist[j].name,
						checklist[j].len) != false ) == status)
		{
			return true;
		}
	}
	return false;
}

static int
W_VerifyWAD (FILE *fp, lumpchecklist_t *checklist, boolean status)
{
	size_t i;


	wadinfo_t header;
	filelump_t lumpinfo;


	if (fread(&header, 1, sizeof header, fp) == sizeof header
			&& header.numlumps < INT16_MAX
			&& strncmp(header.identification, "ZWAD", 4)
			&& strncmp(header.identification, "IWAD", 4)
			&& strncmp(header.identification, "PWAD", 4)
			&& strncmp(header.identification, "SDLL", 4))
	{
		return true;
	}

	header.numlumps = LONG(header.numlumps);
	header.infotableofs = LONG(header.infotableofs);


	if (fseek(fp, header.infotableofs, SEEK_SET) == -1)
		return true;

	for (i = 0; i < header.numlumps; i++)
	{

		if (fread(&lumpinfo, sizeof (lumpinfo), 1 , fp) != 1)
			return true;

		lumpinfo.filepos = LONG(lumpinfo.filepos);
		lumpinfo.size = LONG(lumpinfo.size);

		if (lumpinfo.size == 0)
			continue;

		if (! W_VerifyName(lumpinfo.name, checklist, status))
			return false;
	}

	return true;
}


static lumpchecklist_t folderblacklist[] =
{
	{"Lua/", 4},
	{"SOC/", 4},
	{"Sprites/", 8},
	{"LongSprites/", 12},
	{"Textures/", 9},
	{"Patches/", 8},
	{"Flats/", 6},
	{"Fades/", 6},
	{NULL, 0},
};

static int
W_VerifyPK3 (FILE *fp, lumpchecklist_t *checklist, boolean status)
{
	int verified = true;

    zend_t zend;
    zentry_t zentry;
    zlentry_t zlentry;

	long file_size; 
	long data_size; 

	long old_position;

	UINT16 numlumps;
	size_t i;

	char pat_central[] = {0x50, 0x4b, 0x01, 0x02, 0x00};
	char pat_end[] = {0x50, 0x4b, 0x05, 0x06, 0x00};

	char lumpname[9];






	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);

	if (!ResFindSignature(fp, pat_end, max(0, ftell(fp) - (22 + 65536))))
		return true;

	fseek(fp, -4, SEEK_CUR);
	if (fread(&zend, 1, sizeof zend, fp) < sizeof zend)
		return true;

	data_size = sizeof zend;

	numlumps = zend.entries;

	fseek(fp, zend.cdiroffset, SEEK_SET);
	for (i = 0; i < numlumps; i++)
	{
		char* fullname;
		char* trimname;
		char* dotpos;

		if (fread(&zentry, 1, sizeof(zentry_t), fp) < sizeof(zentry_t))
			return true;
		if (memcmp(zentry.signature, pat_central, 4))
			return true;

		if (verified == true)
		{
			fullname = malloc(zentry.namelen + 1);
			if (fgets(fullname, zentry.namelen + 1, fp) != fullname)
				return true;


			if ((trimname = strrchr(fullname, '/')) != 0)
				trimname++;
			else
				trimname = fullname;  

			if (*trimname)  
			{
				if ((dotpos = strrchr(trimname, '.')) == 0)
					dotpos = fullname + strlen(fullname);  

				memset(lumpname, '\0', 9);  
				strncpy(lumpname, trimname, min(8, dotpos - trimname));

				if (! W_VerifyName(lumpname, checklist, status))
					verified = false;


				else if (W_VerifyName(fullname, folderblacklist, status))
					verified = false;
			}

			free(fullname);


			if (fseek(fp, zentry.xtralen + zentry.commlen, SEEK_CUR) != 0)
				return true;
		}
		else
		{
			if (fseek(fp, zentry.namelen + zentry.xtralen + zentry.commlen, SEEK_CUR) != 0)
				return true;
		}

		data_size +=
			sizeof zentry + zentry.namelen + zentry.xtralen + zentry.commlen;

		old_position = ftell(fp);

		if (fseek(fp, zentry.offset, SEEK_SET) != 0)
			return true;

		if (fread(&zlentry, 1, sizeof(zlentry_t), fp) < sizeof (zlentry_t))
			return true;

		data_size +=
			sizeof zlentry + zlentry.namelen + zlentry.xtralen + zlentry.compsize;

		fseek(fp, old_position, SEEK_SET);
	}

	if (data_size < file_size)
	{
		const char * error = "ZIP file has holes (%ld extra bytes)\n";
		CONS_Alert(CONS_ERROR, error, (file_size - data_size));
		return -1;
	}
	else if (data_size > file_size)
	{
		const char * error = "Reported size of ZIP file contents exceeds file size (%ld extra bytes)\n";
		CONS_Alert(CONS_ERROR, error, (data_size - file_size));
		return -1;
	}
	else
	{
		return verified;
	}
}



static int W_VerifyFile(const char *filename, lumpchecklist_t *checklist,
	boolean status)
{
	FILE *handle;
	int goodfile = false;

	if (!checklist)
		I_Error("No checklist for %s\n", filename);

	if ((handle = W_OpenWadFile(&filename, false)) == NULL)
		return -1;

	if (stricmp(&filename[strlen(filename) - 4], ".pk3") == 0)
		goodfile = W_VerifyPK3(handle, checklist, status);
	else
	{

		if (stricmp(&filename[strlen(filename) - 4], ".soc")
		&& stricmp(&filename[strlen(filename) - 4], ".lua"))
		{
			goodfile = W_VerifyWAD(handle, checklist, status);
		}
	}
	fclose(handle);
	return goodfile;
}


                                                      
                                                                         
                                                                            
            
   
                                                 
                                                         
                                                                             
                                                                            
                                          
                      
    
int W_VerifyNMUSlumps(const char *filename, boolean exit_on_error)
{


	lumpchecklist_t NMUSlist[] =
	{
		{"D_", 2},  
		{"O_", 2},  
		{"DS", 2},  

		{"ENDOOM", 6},  

		{"PLAYPAL", 7},  
		{"PAL", 3},  
		{"COLORMAP", 8},  
		{"CLM", 3},  
		{"TRANS", 5},  

		{"CONSBACK", 8},  

		{"SAVE", 4},  
		{"BLACXLVL", 8},
		{"GAMEDONE", 8},
		{"CONT", 4},  
		{"STNONEX", 7},  
		{"ULTIMATE", 8},  

		{"SLCT", 4},  
		{"LSSTATIC", 8},  
		{"BLANKLV", 7},  

		{"CRFNT", 5},  
		{"NTFNT", 5},  
		{"NTFNO", 5},  
		{"LTFNT", 5},  
		{"TTL", 3},  
		{"STCFN", 5},  
		{"TNYFN", 5},  

		{"STLIVE", 6},  
		{"CROSHAI", 7},  
		{"INTERSC", 7},  
		{"SPECTILE", 8},  
		{"STT", 3},  
		{"YB_", 3},  
		{"RESULT", 6},  
		{"RACE", 4},  
		{"SRB2BACK", 8},  
		{"M_", 2},  
		{"LT", 2},  
		{"HOMING", 6},  
		{"HOMITM", 6},  

		{"CHARFG", 6},  
		{"CHARBG", 6},
		{"RECATK", 6},  
		{"RECCLOCK", 8},
		{"NTSATK", 6},  
		{"NTSSONC", 7},

		{"SLID", 4},  
		{"CONT", 4},

		{"MINICAPS", 8},  
		{"BLUESTAT", 8},  
		{"BYELSTAT", 8},
		{"ORNGSTAT", 8},
		{"REDSTAT", 7},
		{"YELSTAT", 7},
		{"NBRACKET", 8},
		{"NGHTLINK", 8},
		{"NGT", 3},  
		{"NARROW", 6},
		{"NREDAR", 6},
		{"NSS", 3},
		{"NBON", 4},
		{"NRNG", 4},
		{"NHUD", 4},
		{"CAPS", 4},
		{"DRILL", 5},
		{"GRADE", 5},
		{"MINUS5", 6},
		{"NGRTIMER", 8},  

		{"MUSICDEF", 8},  
		{"SHADERS", 7},  
		{"SH_", 3},  

		{NULL, 0},
	};

	int status = W_VerifyFile(filename, NMUSlist, false);

	if (status == -1)
		W_InitFileError(filename, exit_on_error);

	return status;
}

                                                                    
  
                             
                           
  
   
virtres_t* vres_GetMap(lumpnum_t lumpnum)
{
	UINT32 i;
	virtres_t* vres = NULL;
	virtlump_t* vlumps = NULL;
	size_t numlumps = 0;

	if (W_IsLumpWad(lumpnum))
	{

		UINT8 *wadData = W_CacheLumpNum(lumpnum, PU_LEVEL);
		filelump_t *fileinfo = (filelump_t *)(wadData + ((wadinfo_t *)wadData)->infotableofs);
		numlumps = ((wadinfo_t *)wadData)->numlumps;
		vlumps = Z_Calloc(sizeof(virtlump_t)*numlumps, PU_LEVEL, NULL);


		for (i = 0; i < numlumps; i++)
		{
			vlumps[i].size = (size_t)(((filelump_t *)(fileinfo + i))->size);

			memcpy(vlumps[i].name, (fileinfo + i)->name, 8);
			vlumps[i].name[8] = '\0';
			vlumps[i].data = Z_Malloc(vlumps[i].size, PU_LEVEL, NULL);  
			memcpy(vlumps[i].data, wadData + (fileinfo + i)->filepos, vlumps[i].size);
		}

		Z_Free(wadData);
	}
	else
	{

		lumpnum_t lumppos = lumpnum + 1;
		for (i = LUMPNUM(lumppos); i < wadfiles[WADFILENUM(lumpnum)]->numlumps; i++, lumppos++, numlumps++)
			if (memcmp(W_CheckNameForNum(lumppos), "MAP", 3) == 0)
				break;
		numlumps++;

		vlumps = Z_Calloc(sizeof(virtlump_t)*numlumps, PU_LEVEL, NULL);
		for (i = 0; i < numlumps; i++, lumpnum++)
		{
			vlumps[i].size = W_LumpLength(lumpnum);
			memcpy(vlumps[i].name, W_CheckNameForNum(lumpnum), 8);
			vlumps[i].name[8] = '\0';
			vlumps[i].data = W_CacheLumpNum(lumpnum, PU_LEVEL);
		}
	}
	vres = Z_Malloc(sizeof(virtres_t), PU_LEVEL, NULL);
	vres->vlumps = vlumps;
	vres->numlumps = numlumps;

	return vres;
}

                                                          
  
                          
   
void vres_Free(virtres_t* vres)
{
	while (vres->numlumps--)
		Z_Free(vres->vlumps[vres->numlumps].data);
	Z_Free(vres->vlumps);
	Z_Free(vres);
}

                                                              
   
  
                                            
 
          
                                     
                                            
 
  

                                                    
  
                          
                               
                                                
  
   
virtlump_t* vres_Find(const virtres_t* vres, const char* name)
{
	UINT32 i;
	for (i = 0; i < vres->numlumps; i++)
		if (fastcmp(name, vres->vlumps[i].name))
			return &vres->vlumps[i];
	return NULL;
}
