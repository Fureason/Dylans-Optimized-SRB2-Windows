// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#include <stdio.h>
#include <sys/stat.h>

#include <time.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <utime.h>
#endif

#ifdef __GNUC__
#include <unistd.h>
#include <limits.h>
#elif defined (_WIN32)
#include <sys/utime.h>
#endif

#include "../doomdef.h"
#include "../doomstat.h"
#include "../d_main.h"
#include "../g_game.h"
#include "../i_time.h"
#include "i_net.h"
#include "../i_system.h"
#ifdef HAVE_THREADS
#include "../i_threads.h"
#endif
#include "../m_argv.h"
#include "d_net.h"
#include "../w_wad.h"
#include "d_netfil.h"
#include "net_command.h"
#include "../z_zone.h"
#include "../byteptr.h"
#include "../p_setup.h"
#include "../m_misc.h"
#include "../m_menu.h"
#include "../md5.h"
#include "../filesrch.h"

#include <errno.h>


static boolean AddFileToSendQueue(INT32 node, UINT8 fileid);


typedef struct filetx_s
{
	INT32 ram;
	union {
		char *filename;  
		char *ram;  
	} id;
	UINT32 size;  
	UINT8 fileid;
	INT32 node;  
	struct filetx_s *next;  
} filetx_t;


typedef struct filetran_s
{
	filetx_t *txlist;  
	UINT8 iteration;
	UINT8 ackediteration;
	UINT32 position;  
	boolean *ackedfragments;
	UINT32 ackedsize;
	FILE *currentfile;  
	tic_t dontsenduntil;
} filetran_t;
static filetran_t transfer[MAXNETNODES];





INT32 fileneedednum;  
fileneeded_t *fileneeded;  
static tic_t lasttimeackpacketsent = 0;
static I_mutex downloadmutex;
char downloaddir[512] = "DOWNLOAD";


typedef struct
{
	char filename[MAX_WADPATH];
	UINT8 md5sum[16];
	boolean *receivedfragments;
	UINT32 fragmentsize;
	UINT32 currentsize;
} pauseddownload_t;
static pauseddownload_t *pauseddownload = NULL;

file_download_t filedownload;

static CURL *http_handle;
static CURLM *multi_handle;
static UINT32 curl_dlnow;
static UINT32 curl_dltotal;
static time_t curl_starttime;
static int curl_runninghandles = 0;
static UINT32 curl_origfilesize;
static UINT32 curl_origtotalfilesize;
static char *curl_realname = NULL;
static fileneeded_t *curl_curfile = NULL;
HTTP_login *curl_logins;

luafiletransfer_t *luafiletransfers = NULL;
boolean waitingforluafiletransfer = false;
boolean waitingforluafilecommand = false;
char luafiledir[256 + 16] = "luafiles";


static CV_PossibleValue_t maxsend_cons_t[] = {{0, "MIN"}, {204800, "MAX"}, {0, NULL}};
consvar_t cv_maxsend = CVAR_INIT ("maxsend", "4096", CV_SAVE|CV_NETVAR, maxsend_cons_t, NULL);
consvar_t cv_autosendaddons = CVAR_INIT ("autosendaddons", "On", CV_SAVE, CV_OnOff, NULL);

consvar_t cv_noticedownload = CVAR_INIT ("noticedownload", "Off", CV_SAVE|CV_NETVAR, CV_OnOff, NULL);


static CV_PossibleValue_t downloadspeed_cons_t[] = {{1, "MIN"}, {300, "MAX"}, {0, NULL}};
consvar_t cv_downloadspeed = CVAR_INIT ("downloadspeed", "16", CV_SAVE|CV_NETVAR, downloadspeed_cons_t, NULL);
consvar_t cv_autodownloadspeed = CVAR_INIT ("autodownloadspeed", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_persistentmd5 = CVAR_INIT ("persistent_addon_cache", "On", CV_SAVE, CV_OnOff, NULL);

static UINT32 MaxSendBytes(void)
{




	const UINT32 maxsendkb = cv_autosendaddons.value ? 204800 : (UINT32)cv_maxsend.value;
	return maxsendkb * 1024;
}

static UINT16 GetWadNumFromFileNeededId(UINT8 id)
{
	for (UINT16 wadnum = mainwads; wadnum < numwadfiles; wadnum++)
	{
		if (!wadfiles[wadnum]->important)
			continue;
		if (id == 0)
			return wadnum;
		id--;
	}

	return UINT16_MAX;
}

enum
{
	WILLSEND_YES,
	WILLSEND_NO,
	WILLSEND_TOOLARGE
};

                                                                      
   
                                                                       
                                                                               
   
    
UINT8 *PutFileNeeded(UINT16 firstfile)
{
	UINT8 count = 0;
	UINT8 *p_start = netbuffer->packettype == PT_MOREFILESNEEDED ? netbuffer->u.filesneededcfg.files : netbuffer->u.serverinfo.fileneeded;
	UINT8 *p = p_start;
	char wadfilename[MAX_WADPATH] = "";
	UINT8 filestatus, folder;

	for (size_t i = mainwads; i < numwadfiles; i++)  
	{

		if (!wadfiles[i]->important)
			continue;

		if (firstfile)
		{  
			firstfile--;
			continue;
		}

		nameonly(strcpy(wadfilename, wadfiles[i]->filename));


		if (p + 1 + 4 + min(strlen(wadfilename) + 1, MAX_WADPATH) + 16 > p_start + MAXFILENEEDED)
		{

			if (netbuffer->packettype == PT_MOREFILESNEEDED)
				netbuffer->u.filesneededcfg.more = 1;
			else
				netbuffer->u.serverinfo.flags |= SV_LOTSOFADDONS;
			break;
		}

		filestatus = 1;  
		folder = wadfiles[i]->type == RET_FOLDER;

		if (!folder)
		{

			if (!cv_downloading.value)
				filestatus += (WILLSEND_NO << 4);  
			else if (wadfiles[i]->filesize <= MaxSendBytes())
				filestatus += (WILLSEND_YES << 4);  


		}

		WRITEUINT8(p, filestatus);
		WRITEUINT8(p, folder);

		count++;
		WRITEUINT32(p, wadfiles[i]->filesize);
		WRITESTRINGN(p, wadfilename, MAX_WADPATH);
		WRITEMEM(p, wadfiles[i]->md5sum, 16);
	}

	if (netbuffer->packettype == PT_MOREFILESNEEDED)
		netbuffer->u.filesneededcfg.num = count;
	else
		netbuffer->u.serverinfo.fileneedednum = count;

	return p;
}

void AllocFileNeeded(INT32 size)
{
	if (fileneeded == NULL)
		fileneeded = Z_Calloc(sizeof(fileneeded_t) * size, PU_STATIC, NULL);
	else
		fileneeded = Z_Realloc(fileneeded, sizeof(fileneeded_t) * size, PU_STATIC, NULL);
}

void FreeFileNeeded(void)
{
	Z_Free(fileneeded);
	fileneeded = NULL;
}

                                                                         
   
                                                                           
                                                                             
   
    
void D_ParseFileneeded(INT32 fileneedednum_parm, UINT8 *fileneededstr, UINT16 firstfile)
{
	UINT8 *p;
	UINT8 filestatus;

	fileneedednum = firstfile + fileneedednum_parm;
	p = (UINT8 *)fileneededstr;

	AllocFileNeeded(fileneedednum);

	for (INT32 i = firstfile; i < fileneedednum; i++)
	{
		fileneeded[i].type = FILENEEDED_WAD;
		fileneeded[i].status = FS_NOTCHECKED;  
		fileneeded[i].justdownloaded = false;
		filestatus = READUINT8(p);  
		fileneeded[i].folder = READUINT8(p);  
		fileneeded[i].willsend = (UINT8)(filestatus >> 4);
		fileneeded[i].totalsize = READUINT32(p);  
		fileneeded[i].file = NULL;  
		fileneeded[i].failed = FDOWNLOAD_FAIL_NONE;
		READSTRINGN(p, fileneeded[i].filename, MAX_WADPATH);  
		READMEM(p, fileneeded[i].md5sum, 16);  
	}
}

void CL_PrepareDownloadSaveGame(const char *tmpsave)
{
	filedownload.current = -1;

	FreeFileNeeded();
	AllocFileNeeded(1);

	fileneedednum = 1;
	fileneeded[0].type = FILENEEDED_SAVEGAME;
	fileneeded[0].status = FS_REQUESTED;
	fileneeded[0].justdownloaded = false;
	fileneeded[0].totalsize = UINT32_MAX;
	fileneeded[0].file = NULL;
	memset(fileneeded[0].md5sum, 0, 16);
	strcpy(fileneeded[0].filename, tmpsave);
}

                                                              
                                                  
   
                                                
                                                 
   
    
UINT8 CL_CheckDownloadable(boolean direct)
{
	UINT8 dlstatus = DLSTATUS_OK;
	boolean at_least_one = false;

	for (UINT8 i = 0; i < fileneedednum; i++)
		if (fileneeded[i].status != FS_FOUND && fileneeded[i].status != FS_OPEN)
		{
			if (fileneeded[i].folder)
			{
				dlstatus = DLSTATUS_FOLDER;
				break;
			}

			if (!direct || fileneeded[i].willsend == WILLSEND_YES) {
				at_least_one = true;
				continue;
			}

			if (fileneeded[i].willsend == WILLSEND_TOOLARGE)
				dlstatus = DLSTATUS_TOOLARGE;
			else  
				dlstatus = DLSTATUS_WONTSEND;
		}


	if (direct && !dlstatus && M_CheckParm("-nodownload"))
		dlstatus = DLSTATUS_NODOWNLOAD;

	return dlstatus;
}

                                                         
   
                                                          
                                               
   
    
static boolean CL_CanResumeDownload(fileneeded_t *file)
{
	return pauseddownload
		&& !strcmp(pauseddownload->filename, file->filename)  
		&& !memcmp(pauseddownload->md5sum, file->md5sum, 16)  
		&& pauseddownload->fragmentsize == file->fragmentsize;  
}

void CL_AbortDownloadResume(void)
{
	if (!pauseddownload)
		return;

	free(pauseddownload->receivedfragments);
	remove(pauseddownload->filename);
	free(pauseddownload);
	pauseddownload = NULL;
}

                                                                       
                  
   
                                                    
                                       
   
    
boolean CL_SendFileRequest(void)
{
	char *p;
	INT64 totalfreespaceneeded = 0, availablefreespace;

#ifdef PARANOIA
	if (M_CheckParm("-nodownload"))
	{
		CONS_Printf("Attempted to download files in -nodownload mode");
		return false;
	}
#endif

	for (INT32 i = 0; i < fileneedednum; i++)
	{
		if (fileneeded[i].status != FS_FOUND && fileneeded[i].status != FS_OPEN
			&& (fileneeded[i].willsend == WILLSEND_TOOLARGE || fileneeded[i].willsend == WILLSEND_NO || fileneeded[i].folder))
		{
			CONS_Printf("Attempted to download files that were not sendable");
			return false;
		}

		if (fileneeded[i].status == FS_NOTFOUND || fileneeded[i].status == FS_MD5SUMBAD)
		{

			totalfreespaceneeded += fileneeded[i].totalsize;
		}
	}

	I_GetDiskFreeSpace(&availablefreespace);
	if (totalfreespaceneeded > availablefreespace)
	{
		CONS_Printf("To play on this server you must download %s KB,\n"
			"but you have only %s KB free space on your device\n",
			sizeu1((size_t)(totalfreespaceneeded>>10)), sizeu2((size_t)(availablefreespace>>10)));
		return false;
	}

	netbuffer->packettype = PT_REQUESTFILE;
	p = (char *)netbuffer->u.textcmd;
	for (INT32 i = 0; i < fileneedednum; i++)
		if (fileneeded[i].status == FS_NOTFOUND || fileneeded[i].status == FS_MD5SUMBAD)
		{
			WRITEUINT8(p, i);  


			nameonly(fileneeded[i].filename);
			strcatbf(fileneeded[i].filename, downloaddir, "/");

			fileneeded[i].status = FS_REQUESTED;
		}

	WRITEUINT8(p, 0xFF);

	if (!HSendPacket(servernode, true, 0, p - (char *)netbuffer->u.textcmd))
	{
		CONS_Printf("Could not send download request packet to server\n");
		return false;
	}


	I_mkdir(downloaddir, 0755);

	return true;
}


void PT_RequestFile(SINT8 node)
{
	UINT8 *p = netbuffer->u.textcmd;

	if (client || !cv_downloading.value)
	{
		Net_CloseConnection(node);  
		return;
	}

	while (p < netbuffer->u.textcmd + MAXTEXTCMD-1)  
	{
		UINT8 id = READUINT8(p);
		if (id == 0xFF)
			break;

		if (!AddFileToSendQueue(node, id))
		{
			SV_AbortSendFiles(node);
			Net_CloseConnection(node);  
			return;  
		}
	}

	return;  
}

                                                                   
   
                                       
                                
                                                                                        
                                           
                                                 
   
    
INT32 CL_CheckFiles(void)
{
	INT32 i, j;
	char wadfilename[MAX_WADPATH];
	size_t filestoload = 0;
	boolean downloadrequired = false;
	UINT8 fileschecked = 0;
	const precise_t checkstarted = I_GetPreciseTime();
	precise_t checkbudget = (precise_t)(I_GetPrecisePrecision() / 250);  

	if (!checkbudget)
		checkbudget = 1;





	if (modifiedgame)
	{
		CONS_Debug(DBG_NETPLAY, "game is modified; only doing basic checks\n");
		for (i = 0, j = mainwads; i < fileneedednum || j < numwadfiles;)
		{
			if (j < numwadfiles && !wadfiles[j]->important)
			{

				++j;
				continue;
			}


			if (i >= fileneedednum)
			{
				CONS_Alert(CONS_ERROR,
					"Add-on list mismatch at position %d: client=%s, server=%s.\n",
					i + 1,
					j < numwadfiles ? wadfiles[j]->filename : "<end of list>",
					i < fileneedednum ? fileneeded[i].filename : "<end of list>");
				return 2;
			}



			if (j >= numwadfiles)
			{
				for (; i < fileneedednum; i++)
				{
					if (fileneeded[i].status == FS_NOTCHECKED)
					{
						if (fileneeded[i].folder)
							fileneeded[i].status = FS_NOTFOUND;
						else
							fileneeded[i].status = findfile(fileneeded[i].filename,
								fileneeded[i].md5sum, true);
					}

					if (fileneeded[i].status == FS_NOTFOUND
					|| fileneeded[i].status == FS_MD5SUMBAD
					|| fileneeded[i].status == FS_FALLBACK)
						downloadrequired = true;
					if (fileneeded[i].status != FS_OPEN)
						filestoload++;
				}

				if (numwadfiles + filestoload > MAX_WADFILES)
					return 3;
				return downloadrequired ? 0 : 1;
			}


			if (memcmp(wadfiles[j]->md5sum, fileneeded[i].md5sum, 16))
			{
				CONS_Alert(CONS_ERROR,
					"Add-on checksum/order mismatch at position %d: client=%s, server=%s.\n",
					i + 1, wadfiles[j]->filename, fileneeded[i].filename);
				return 2;
			}


			CONS_Debug(DBG_NETPLAY, "'%s' accounted for\n", fileneeded[i].filename);
			fileneeded[i].status = FS_OPEN;
			++i;
			++j;
		}
		return 1;
	}

	for (i = 0; i < fileneedednum; i++)
	{
		if (fileneeded[i].status == FS_NOTFOUND || fileneeded[i].status == FS_MD5SUMBAD || fileneeded[i].status == FS_FALLBACK)
			downloadrequired = true;

		if (fileneeded[i].status != FS_OPEN)
			filestoload++;

		if (fileneeded[i].status != FS_NOTCHECKED)  
			continue;

		CONS_Debug(DBG_NETPLAY, "searching for '%s' ", fileneeded[i].filename);

		if (fileneeded[i].folder)
		{
			fileneeded[i].status = FS_NOTFOUND;
		}
		else
		{
			boolean alreadyloaded = false;


			for (j = mainwads; j < numwadfiles; j++)
			{
				nameonly(strcpy(wadfilename, wadfiles[j]->filename));
				if (!stricmp(wadfilename, fileneeded[i].filename) &&
					!memcmp(wadfiles[j]->md5sum, fileneeded[i].md5sum, 16))
				{
					CONS_Debug(DBG_NETPLAY, "already loaded\n");
					fileneeded[i].status = FS_OPEN;
					alreadyloaded = true;
					break;
				}
			}

			if (!alreadyloaded)
				fileneeded[i].status = findfile(fileneeded[i].filename, fileneeded[i].md5sum, true);
		}

		CONS_Debug(DBG_NETPLAY, "found %d\n", fileneeded[i].status);
		fileschecked++;
		if (fileschecked >= 16 || I_GetPreciseTime() - checkstarted >= checkbudget)
			return 4;
	}


	if (numwadfiles+filestoload > MAX_WADFILES)
		return 3;
	else if (downloadrequired)
		return 0;  
	else
		return 1;  
}


boolean CL_LoadServerFiles(void)
{
	UINT8 filesloaded = 0;
	const precise_t loadstarted = I_GetPreciseTime();
	precise_t loadbudget = (precise_t)(I_GetPrecisePrecision() / 125);  

	if (M_CheckParm("-nofiles"))
		return true;
	if (!loadbudget)
		loadbudget = 1;

	for (INT32 i = 0; i < fileneedednum; i++)
	{
		if (fileneeded[i].status == FS_OPEN)
			continue;  
		else if (fileneeded[i].status == FS_FOUND)
		{
			if (fileneeded[i].folder)
				P_AddFolder(fileneeded[i].filename);
			else
			{
				const int verification = W_VerifyNMUSlumps(
					fileneeded[i].filename, false);
				if (verification == -1 || !P_AddWadFileVerified(
					fileneeded[i].filename, false, verification,
					fileneeded[i].md5sum))
					I_Error("Could not load server file %s",
						fileneeded[i].filename);
			}
			G_SetGameModified(true);
			fileneeded[i].status = FS_OPEN;
			filesloaded++;
			                                                            
                                                                    
                                                              
			if (filesloaded >= 8 || I_GetPreciseTime() - loadstarted >= loadbudget)
				return false;
		}
		else if (fileneeded[i].status == FS_MD5SUMBAD)
			I_Error("Wrong version of file %s", fileneeded[i].filename);
		else
		{
			const char *s;
			switch(fileneeded[i].status)
			{
			case FS_NOTFOUND:
				s = "FS_NOTFOUND";
				break;
			case FS_REQUESTED:
				s = "FS_REQUESTED";
				break;
			case FS_DOWNLOADING:
				s = "FS_DOWNLOADING";
				break;
			default:
				s = "unknown";
				break;
			}
			I_Error("Try to load file \"%s\" with status of %d (%s)\n", fileneeded[i].filename,
				fileneeded[i].status, s);
		}
	}
	return true;
}

void AddLuaFileTransfer(const char *filename, const char *mode)
{
	luafiletransfer_t **prevnext;  
	luafiletransfer_t *filetransfer;
	static INT32 id;


	prevnext = &luafiletransfers;
	while (*prevnext)
		prevnext = &((*prevnext)->next);


	filetransfer = calloc(1, sizeof(luafiletransfer_t));
	if (!filetransfer)
		I_Error("AddLuaFileTransfer: Out of memory\n");
	*prevnext = filetransfer;
	filetransfer->next = NULL;


	filetransfer->filename = strdup(filename);
	if (!filetransfer->filename)
		I_Error("AddLuaFileTransfer: Out of memory\n");


	if (server)
		filetransfer->realfilename = strdup(va("%s" PATHSEP "%s",
												luafiledir, filename));
	else
		filetransfer->realfilename = strdup(va("%s" PATHSEP "client" PATHSEP "$$$%d%d.tmp",
												luafiledir, rand(), rand()));
	if (!filetransfer->realfilename)
		I_Error("AddLuaFileTransfer: Out of memory\n");

	strlcpy(filetransfer->mode, mode, sizeof(filetransfer->mode));


	if (server && filetransfer == luafiletransfers)
		SV_PrepareSendLuaFile();
	else
		filetransfer->ongoing = false;


	filetransfer->id = id;
	StoreLuaFileCallback(id);
	id++;

	if (waitingforluafiletransfer)
	{
		waitingforluafiletransfer = false;
		CL_PrepareDownloadLuaFile();
	}
}

static void SV_PrepareSendLuaFileToNextNode(void)
{
	UINT8 success = 1;


	for (INT32 i = 1; i < MAXNETNODES; i++)
		if (luafiletransfers->nodestatus[i] == LFTNS_WAITING)  
		{

			netbuffer->packettype = PT_SENDINGLUAFILE;
			if (!HSendPacket(i, true, 0, 0))
				I_Error("Failed to send a PT_SENDINGLUAFILE packet\n");  

			luafiletransfers->nodestatus[i] = LFTNS_ASKED;
			luafiletransfers->nodetimeouts[i] = I_GetTime() + 30 * TICRATE;

			return;
		}



	SendNetXCmd(XD_LUAFILE, &success, 1);
}

void SV_PrepareSendLuaFile(void)
{
	char *binfilename;

	luafiletransfers->ongoing = true;


	for (INT32 i = 0; i < MAXNETNODES; i++)
		luafiletransfers->nodestatus[i] = (netnodes[i].ingame ? LFTNS_WAITING : LFTNS_NONE);

	if (FIL_ReadFileOK(luafiletransfers->realfilename))
	{

		if (!strchr(luafiletransfers->mode, 'b'))
		{
			binfilename = strdup(va("%s" PATHSEP "$$$%d%d.tmp",
				luafiledir, rand(), rand()));
			if (!binfilename)
				I_Error("SV_PrepareSendLuaFile: Out of memory\n");

			if (!FIL_ConvertTextFileToBinary(luafiletransfers->realfilename, binfilename))
				I_Error("SV_PrepareSendLuaFile: Failed to convert file newlines\n");


			free(luafiletransfers->realfilename);
			luafiletransfers->realfilename = binfilename;
		}

		SV_PrepareSendLuaFileToNextNode();
	}
	else
	{

		UINT8 success = 0;
		SendNetXCmd(XD_LUAFILE, &success, 1);
	}
}

void SV_HandleLuaFileSent(UINT8 node)
{
	luafiletransfers->nodestatus[node] = LFTNS_SENT;
	                                                                         
                                                                        
                                                                               
	if (node < MAXNETNODES)
		netnodes[node].savegameresendcooldown = I_GetTime() + 3*TICRATE;
	SV_PrepareSendLuaFileToNextNode();
}

void RemoveLuaFileTransfer(void)
{
	luafiletransfer_t *filetransfer = luafiletransfers;


	if (server && !strchr(filetransfer->mode, 'b'))
		remove(filetransfer->realfilename);

	RemoveLuaFileCallback(filetransfer->id);

	luafiletransfers = filetransfer->next;

	free(filetransfer->filename);
	free(filetransfer->realfilename);
	free(filetransfer);
}

void RemoveAllLuaFileTransfers(void)
{
	while (luafiletransfers)
		RemoveLuaFileTransfer();
}

void SV_AbortLuaFileTransfer(INT32 node)
{
	if (luafiletransfers)
	{
		if (luafiletransfers->nodestatus[node] == LFTNS_ASKED
			|| luafiletransfers->nodestatus[node] == LFTNS_SENDING)
		{
			SV_PrepareSendLuaFileToNextNode();
		}
		luafiletransfers->nodestatus[node] = LFTNS_NONE;
	}
}

void CL_PrepareDownloadLuaFile(void)
{


	if (!luafiletransfers)
	{
		waitingforluafiletransfer = true;
		return;
	}

	if (luafiletransfers->ongoing)
	{
		waitingforluafilecommand = true;
		return;
	}


	netbuffer->packettype = PT_ASKLUAFILE;
	HSendPacket(servernode, true, 0, 0);

	FreeFileNeeded();
	AllocFileNeeded(1);

	fileneedednum = 1;
	fileneeded[0].type = FILENEEDED_LUAFILE;
	fileneeded[0].status = FS_REQUESTED;
	fileneeded[0].justdownloaded = false;
	fileneeded[0].totalsize = UINT32_MAX;
	fileneeded[0].file = NULL;
	memset(fileneeded[0].md5sum, 0, 16);
	strcpy(fileneeded[0].filename, luafiletransfers->realfilename);


	MakePathDirs(fileneeded[0].filename);

	luafiletransfers->ongoing = true;
}



static INT32 filestosend = 0;

                                           
   
                                            
                                    
                                                                  
                         
                             
   
    
static boolean AddFileToSendQueue(INT32 node, UINT8 fileid)
{
	filetx_t **q;  
	filetx_t *p;  
	UINT16 wadnum;


	q = &transfer[node].txlist;
	while (*q)
		q = &((*q)->next);


	p = *q = (filetx_t *)malloc(sizeof (filetx_t));
	if (!p)
		I_Error("AddFileToSendQueue: No more memory\n");


	memset(p, 0, sizeof (filetx_t));


	p->id.filename = (char *)malloc(MAX_WADPATH);
	if (!p->id.filename)
		I_Error("AddFileToSendQueue: No more memory\n");


	wadnum = GetWadNumFromFileNeededId(fileid);


	if (wadnum == UINT16_MAX)
	{
		DEBFILE(va("fileneeded %d not found in wadfiles\n", fileid));




		DEBFILE(va("Client %d request fileneeded %d: not found\n", node, fileid));
		free(p->id.filename);
		free(p);
		*q = NULL;
		return false;  
	}


	strlcpy(p->id.filename, wadfiles[wadnum]->filename, MAX_WADPATH);


	if (wadfiles[wadnum]->filesize > MaxSendBytes())
	{


		DEBFILE(va("Client %d request %s: file too big, not sending\n", node, p->id.filename));
		free(p->id.filename);
		free(p);
		*q = NULL;
		return false;  
	}

	if (cv_noticedownload.value)
		CONS_Printf("Sending file \"%s\" to node %d (%s)\n", p->id.filename, node, I_GetNodeAddress(node));

	DEBFILE(va("Sending file %s (id=%d) to %d\n", p->id.filename, fileid, node));
	p->ram = SF_FILE;  
	p->fileid = fileid;
	p->next = NULL;  
	filestosend++;
	return true;
}

                                                   
   
                                                    
                                        
                                              
                                                                  
                                                                  
                          
                             
   
    
void AddRamToSendQueue(INT32 node, void *data, size_t size, freemethod_t freemethod, UINT8 fileid)
{
	filetx_t **q;  
	filetx_t *p;  


	q = &transfer[node].txlist;
	while (*q)
		q = &((*q)->next);


	p = *q = (filetx_t *)malloc(sizeof (filetx_t));
	if (!p)
		I_Error("AddRamToSendQueue: No more memory\n");


	memset(p, 0, sizeof (filetx_t));

	p->ram = freemethod;  
	p->id.ram = data;
	p->size = (UINT32)size;
	p->fileid = fileid;
	p->next = NULL;  

	DEBFILE(va("Sending ram %p(size:%u) to %d (id=%u)\n",p->id.ram,p->size,node,fileid));

	filestosend++;
}

                                                            
   
                                            
                                    
                          
                         
   
    
boolean AddLuaFileToSendQueue(INT32 node, const char *filename)
{
	filetx_t **q;  
	filetx_t *p;  

	luafiletransfers->nodestatus[node] = LFTNS_SENDING;


	q = &transfer[node].txlist;
	while (*q)
		q = &((*q)->next);


	p = *q = (filetx_t *)malloc(sizeof (filetx_t));
	if (!p)
		I_Error("AddLuaFileToSendQueue: No more memory\n");


	memset(p, 0, sizeof (filetx_t));


	p->id.filename = (char *)malloc(MAX_WADPATH);  
	if (!p->id.filename)
		I_Error("AddLuaFileToSendQueue: No more memory\n");


	strlcpy(p->id.filename, filename, MAX_WADPATH);  


	DEBFILE(va("Sending Lua file %s to %d\n", filename, node));
	p->ram = SF_FILE;  
	p->next = NULL;  
	filestosend++;
	return true;
}

                                                                                
                                                                                    
   
                               
   
    
static void SV_EndFileSend(INT32 node)
{
	filetx_t *p = transfer[node].txlist;



	switch (p->ram)
	{
		case SF_FILE:  
			if (cv_noticedownload.value)
				CONS_Printf("Ending file transfer for node %d\n", node);
			if (transfer[node].currentfile)
				fclose(transfer[node].currentfile);
			free(p->id.filename);
			break;
		case SF_Z_RAM:  
			Z_Free(p->id.ram);
			break;
		case SF_RAM:  
			free(p->id.ram);
		case SF_NOFREERAM:  
			break;
	}


	transfer[node].txlist = p->next;
	free(p);


	transfer[node].currentfile = NULL;
	if (transfer[node].ackedfragments)
		free(transfer[node].ackedfragments);
	transfer[node].ackedfragments = NULL;

	filestosend--;
}

#define FILEFRAGMENTSIZE (software_MAXPACKETLENGTH - (FILETXHEADER + BASEPACKETSIZE))

static size_t D_FileTransferPayloadSize(void)
{
	const size_t headersize = FILETXHEADER + BASEPACKETSIZE;
	const size_t packetlength = software_MAXPACKETLENGTH
		? software_MAXPACKETLENGTH : INETPACKETLENGTH;

	return packetlength > headersize ? packetlength - headersize : 0;
}

double D_FileTransferMBPerSecond(INT32 packetspertic)
{
	const size_t payloadsize = D_FileTransferPayloadSize();

	if (packetspertic <= 0 || !payloadsize)
		return 0.0;

	return ((double)packetspertic * (double)payloadsize
		* (double)TICRATE) / 1000000.0;
}

INT32 D_FileTransferPacketsForMBPerSecond(double megabytespersecond)
{
	const size_t payloadsize = D_FileTransferPayloadSize();
	INT32 packetspertic;

	if (megabytespersecond <= 0.0 || !payloadsize)
		return downloadspeed_cons_t[0].value;

	packetspertic = (INT32)((megabytespersecond * 1000000.0
		/ ((double)payloadsize * (double)TICRATE)) + 0.5);

	return max(downloadspeed_cons_t[0].value,
		min(downloadspeed_cons_t[1].value, packetspertic));
}

                                                                           
                                                                             
                                                                              
static INT32 D_AutomaticFileTransferPackets(INT32 limit, boolean transferactive)
{
	static boolean active;
	static INT32 packets;
	static tic_t nextadjust;
	UINT16 worstqueued = 0, worstretries = 0;
	INT32 i;

	if (!cv_autodownloadspeed.value || !transferactive)
	{
		active = false;
		return limit;
	}

	if (!active)
	{
		packets = min(limit, D_FileTransferPacketsForMBPerSecond(0.8));
		nextadjust = I_GetTime() + TICRATE;
		active = true;
	}

	packets = max(downloadspeed_cons_t[0].value, min(limit, packets));
	if (I_GetTime() < nextadjust)
		return packets;

	nextadjust = I_GetTime() + TICRATE;
	for (i = 1; i < MAXNETNODES; i++)
	{
		UINT16 retries;
		UINT16 queued;

		if (!transfer[i].txlist)
			continue;

		queued = Net_GetReliableBacklog(i, &retries);
		worstqueued = max(worstqueued, queued);
		worstretries = max(worstretries, retries);
	}

	if (lostpercent >= 5.0f || worstqueued >= 16 || worstretries >= 3)
		packets = max(downloadspeed_cons_t[0].value, packets / 2);
	else if (lostpercent >= 2.0f || worstqueued >= 8 || worstretries >= 1)
		packets = max(downloadspeed_cons_t[0].value, (packets * 3) / 4);
	else if (lostpercent < 0.5f && worstqueued < 4 && worstretries == 0)
		packets = min(limit, packets + max(1, packets / 8));

	return packets;
}

                             
   
    
void FileSendTicker(void)
{
	static INT32 currentnode = 0;
	const precise_t sendstarted = I_GetPreciseTime();
	const precise_t listenbudget = I_GetPrecisePrecision() / 1000;  
	filetx_pak *p;
	size_t fragmentsize;
	filetx_t *f;
	INT32 packetsent, ram, i, j;



	if (luafiletransfers)
	{
		for (i = 1; i < MAXNETNODES; i++)
		{
			luafiletransfernodestatus_t status = luafiletransfers->nodestatus[i];

			if (status != LFTNS_NONE && status != LFTNS_WAITING && status != LFTNS_SENT
				&& I_GetTime() > luafiletransfers->nodetimeouts[i])
			{
				Net_ConnectionTimeout(i);
			}
		}
	}

	if (!filestosend)  
	{
		D_AutomaticFileTransferPackets(cv_downloadspeed.value, false);
		return;
	}

	packetsent = D_AutomaticFileTransferPackets(cv_downloadspeed.value, true);

	netbuffer->packettype = PT_FILEFRAGMENT;

	while (packetsent-- && filestosend != 0)
	{
		                                                                        
                                                                            
                                                                             
                                 
		if (server && !dedicated && gamestate == GS_LEVEL && listenbudget
		&& I_GetPreciseTime() - sendstarted >= listenbudget)
			break;

		for (i = currentnode, j = 0; j < MAXNETNODES;
			i = (i+1) % MAXNETNODES, j++)
		{
			if (transfer[i].txlist)
				break;
		}

		if (j >= MAXNETNODES)
			I_Error("filestosend=%d but no file to send found\n", filestosend);

		currentnode = (i+1) % MAXNETNODES;
		f = transfer[i].txlist;
		ram = f->ram;

		                                                                       
                                                                           
                                                                        
                                                                          
                                  
		{
			UINT16 retries;
			UINT16 queued = Net_GetReliableBacklog(i, &retries);
			if (queued >= 16 || retries >= 3)
			{
				transfer[i].dontsenduntil = max(transfer[i].dontsenduntil,
					I_GetTime() + max(1, TICRATE / 4));
				continue;
			}
		}


		if (!transfer[i].currentfile)
		{
			if (!ram)  
			{
				long filesize;

				transfer[i].currentfile =
					fopen(f->id.filename, "rb");

				if (!transfer[i].currentfile)
					I_Error("File %s does not exist",
						f->id.filename);

				fseek(transfer[i].currentfile, 0, SEEK_END);
				filesize = ftell(transfer[i].currentfile);



				if (filesize >= LONG_MAX)
					I_Error("filesize of %s is too large", f->id.filename);
				if (filesize == -1)
					I_Error("Error getting filesize of %s", f->id.filename);

				f->size = (UINT32)filesize;
				fseek(transfer[i].currentfile, 0, SEEK_SET);
			}
			else  
				transfer[i].currentfile = (FILE *)1;  

			transfer[i].iteration = 1;
			transfer[i].ackediteration = 0;
			transfer[i].position = 0;
			transfer[i].ackedsize = 0;

			transfer[i].ackedfragments = calloc(f->size / FILEFRAGMENTSIZE + 1, sizeof(*transfer[i].ackedfragments));
			if (!transfer[i].ackedfragments)
				I_Error("FileSendTicker: No more memory\n");

			transfer[i].dontsenduntil = 0;
		}




		if (I_GetTime() < transfer[i].dontsenduntil)
			continue;


		while (transfer[i].ackedfragments[transfer[i].position / FILEFRAGMENTSIZE])
		{
			transfer[i].position += FILEFRAGMENTSIZE;
			if (transfer[i].position >= f->size)
			{
				if (transfer[i].ackediteration < transfer[i].iteration)
					transfer[i].dontsenduntil = I_GetTime() + TICRATE / 2;

				transfer[i].position = 0;
				transfer[i].iteration++;
			}
		}


		p = &netbuffer->u.filetxpak;
		fragmentsize = FILEFRAGMENTSIZE;
		if (f->size-transfer[i].position < fragmentsize)
			fragmentsize = f->size-transfer[i].position;
		if (ram)
			M_Memcpy(p->data, &f->id.ram[transfer[i].position], fragmentsize);
		else
		{
			fseek(transfer[i].currentfile, transfer[i].position, SEEK_SET);

			if (fread(p->data, 1, fragmentsize, transfer[i].currentfile) != fragmentsize)
				I_Error("FileSendTicker: can't read %s byte on %s at %d because %s", sizeu1(fragmentsize), f->id.filename, transfer[i].position, M_FileError(transfer[i].currentfile));
		}
		p->iteration = transfer[i].iteration;
		p->position = LONG(transfer[i].position);
		p->fileid = f->fileid;
		p->filesize = LONG(f->size);
		p->size = SHORT((UINT16)FILEFRAGMENTSIZE);


		if (HSendPacket(i, false, 0, FILETXHEADER + fragmentsize))  
		{  
			transfer[i].position = (UINT32)(transfer[i].position + fragmentsize);
			if (transfer[i].position >= f->size)
			{
				if (transfer[i].ackediteration < transfer[i].iteration)
					transfer[i].dontsenduntil = I_GetTime() + TICRATE / 2;

				transfer[i].position = 0;
				transfer[i].iteration++;
			}
		}
		else
		{  

			break;
		}
	}
}

void PT_FileAck(SINT8 node)
{
	fileack_pak *packet = &netbuffer->u.fileack;
	filetran_t *trans = &transfer[node];

	if (client)
		return;


	if (!(trans->txlist && packet->fileid == trans->txlist->fileid))
		return;

	if (packet->numsegments * sizeof(*packet->segments) != doomcom->datalength - BASEPACKETSIZE - sizeof(*packet))
	{
		Net_CloseConnection(node);
		return;
	}

	if (packet->iteration > trans->ackediteration)
	{
		trans->ackediteration = packet->iteration;
		if (trans->ackediteration >= trans->iteration - 1)
			trans->dontsenduntil = 0;
	}

	for (INT32 i = 0; i < packet->numsegments; i++)
	{
		fileacksegment_t *segment = &packet->segments[i];

		for (INT32 j = 0; j < 32; j++)
			if (LONG(segment->acks) & (1 << j))
			{
				if (LONG(segment->start) * FILEFRAGMENTSIZE >= trans->txlist->size)
				{
					Net_CloseConnection(node);
					return;
				}

				if (!trans->ackedfragments[LONG(segment->start) + j])
				{
					trans->ackedfragments[LONG(segment->start) + j] = true;
					trans->ackedsize += FILEFRAGMENTSIZE;


					if (trans->ackedsize == trans->txlist->size)
					{
						SV_EndFileSend(node);
						return;
					}
				}
			}
	}
}

void PT_FileReceived(SINT8 node)
{
	filetx_t *trans = transfer[node].txlist;

	if (server && trans && netbuffer->u.filereceived == trans->fileid)
		SV_EndFileSend(node);
}

static void SendAckPacket(fileack_pak *packet, UINT8 fileid)
{
	size_t packetsize;

	packetsize = sizeof(*packet) + packet->numsegments * sizeof(*packet->segments);


	packet->fileid = fileid;
	for (INT32 i = 0; i < packet->numsegments; i++)
	{
		packet->segments[i].start = LONG(packet->segments[i].start);
		packet->segments[i].acks = LONG(packet->segments[i].acks);
	}


	netbuffer->packettype = PT_FILEACK;
	M_Memcpy(&netbuffer->u.fileack, packet, packetsize);
	HSendPacket(servernode, false, 0, packetsize);


	memset(packet, 0, sizeof(*packet) + 512);
}

static void AddFragmentToAckPacket(fileack_pak *packet, UINT8 iteration, UINT32 fragmentpos, UINT8 fileid)
{
	fileacksegment_t *segment = &packet->segments[packet->numsegments - 1];

	packet->iteration = max(packet->iteration, iteration);

    if (packet->numsegments == 0
		|| fragmentpos < segment->start
		|| fragmentpos - segment->start >= 32)
	{

		if ((packet->numsegments + 1) * sizeof(*segment) > 512)
			SendAckPacket(packet, fileid);

		packet->numsegments++;
		segment = &packet->segments[packet->numsegments - 1];
		segment->start = fragmentpos;
	}


	segment->acks |= 1 << (fragmentpos - segment->start);
}

void FileReceiveTicker(void)
{
	for (INT32 i = 0; i < fileneedednum; i++)
	{
		fileneeded_t *file = &fileneeded[i];

		if (file->status == FS_DOWNLOADING)
		{
			if (lasttimeackpacketsent - I_GetTime() > TICRATE / 2)
				SendAckPacket(file->ackpacket, i);



			if (file->ackresendposition != UINT32_MAX && file->status == FS_DOWNLOADING)
			{

				for (INT32 j = 0; j < 2048; j++)
				{
					if (file->receivedfragments[file->ackresendposition])
						AddFragmentToAckPacket(file->ackpacket, file->iteration, file->ackresendposition, i);

					file->ackresendposition++;
					if (file->ackresendposition * file->fragmentsize >= file->totalsize)
					{
						file->ackresendposition = UINT32_MAX;
						break;
					}
				}
			}
		}
	}
}

static void OpenNewFileForDownload(fileneeded_t *file, const char *filename)
{
	file->file = fopen(filename, "wb");
	if (!file->file)
		I_Error("Can't create file %s: %s", filename, strerror(errno));

	file->currentsize = 0;
	file->totalsize = LONG(netbuffer->u.filetxpak.filesize);
	file->ackresendposition = UINT32_MAX;  

	file->receivedfragments = calloc(file->totalsize / file->fragmentsize + 1, sizeof(*file->receivedfragments));
	if (!file->receivedfragments)
		I_Error("FileSendTicker: No more memory\n");
}

void PT_FileFragment(SINT8 node, INT32 netconsole)
{
	if (netnodes[node].ingame)
	{

		if (node != servernode)
		{
			CONS_Alert(CONS_WARNING, M_GetText("%s received from non-host %d\n"), "PT_FILEFRAGMENT", node);
			if (server)
				SendKick(netconsole, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
			return;
		}
		if (server)
			return;
	}
	else if (server || node != servernode)
	{
		Net_CloseConnection(node);
		return;
	}

	INT32 filenum = netbuffer->u.filetxpak.fileid;
	fileneeded_t *file = &fileneeded[filenum];
	UINT32 fragmentpos = LONG(netbuffer->u.filetxpak.position);
	UINT16 fragmentsize = SHORT(netbuffer->u.filetxpak.size);
	UINT16 boundedfragmentsize = doomcom->datalength - BASEPACKETSIZE - sizeof(netbuffer->u.filetxpak);
	char *filename;

	if (!file)
		return;

	filename = va("%s", file->filename);
	nameonly(filename);

	if (!(strcmp(filename, "srb2.pk3")
		&& strcmp(filename, "zones.pk3")
		&& strcmp(filename, "characters.pk3")
		&& strcmp(filename, "patch.pk3")
		&& strcmp(filename, "music.pk3")
		))
		I_Error("Tried to download \"%s\"", filename);

	if (filenum >= fileneedednum)
	{
		DEBFILE(va("fileframent not needed %d>%d\n", filenum, fileneedednum));

		return;
	}

	if (file->status == FS_REQUESTED)
	{
		if (file->file)
			I_Error("PT_FileFragment: already open file\n");

		file->status = FS_DOWNLOADING;
		file->fragmentsize = fragmentsize;
		file->iteration = 0;

		file->ackpacket = calloc(1, sizeof(*file->ackpacket) + 512);
		if (!file->ackpacket)
			I_Error("FileSendTicker: No more memory\n");

		if (CL_CanResumeDownload(file))
		{
			file->file = fopen(file->filename, "r+b");
			if (!file->file)
			{
				CONS_Alert(CONS_ERROR, "Couldn't reopen file %s: %s\n", file->filename, strerror(errno));

				free(pauseddownload->receivedfragments);

				CONS_Printf("Restarting download of addon \"%s\"...\n", filename);

				OpenNewFileForDownload(file, file->filename);
			}
			else
			{
				CONS_Printf("Resuming download of addon \"%s\"...\n", filename);
				file->currentsize = pauseddownload->currentsize;
				file->receivedfragments = pauseddownload->receivedfragments;
				file->ackresendposition = 0;
			}

			free(pauseddownload);
			pauseddownload = NULL;
		}
		else
		{
			char backup_filename[MAX_WADPATH+14];  
			char filename_noext[MAX_WADPATH];
			char *extension;
			UINT32 n = 0;
			CL_AbortDownloadResume();
			strcpy(backup_filename, filename);
			strcpy(filename_noext, filename);
			extension = (char*)FIL_GetExtension(filename_noext);
			*(extension++) = '\0';

			for (;;) {
				FILE *tempfp;
				tempfp = fopen(backup_filename, "r");
				if (tempfp)
					fclose(tempfp);
				else
					break;
				sprintf(backup_filename, "%s_old%u.%s", filename_noext, n++, extension);
			}

			if (n) {
				if (rename(filename, backup_filename) == -1)
					I_Error("Error %d renaming %s to %s", errno, filename, backup_filename);
				else
					CONS_Printf("\x82NOTICE:\x80 %s already exists; old file saved as %s\n", filename, backup_filename);
			}
			OpenNewFileForDownload(file, file->filename);
			CONS_Printf("Downloading addon \"%s\" from the server...\n", filename);
		}

		lasttimeackpacketsent = I_GetTime();
	}

	if (file->status == FS_DOWNLOADING)
	{
		if (fragmentpos >= file->totalsize)
			I_Error("Invalid file fragment\n");

		file->iteration = max(file->iteration, netbuffer->u.filetxpak.iteration);

		if (!file->receivedfragments[fragmentpos / fragmentsize])  
		{
			file->receivedfragments[fragmentpos / fragmentsize] = true;


			fseek(file->file, fragmentpos, SEEK_SET);
			if (fragmentsize && fwrite(netbuffer->u.filetxpak.data, boundedfragmentsize, 1, file->file) != 1)
				I_Error("Can't write to %s: %s\n",file->filename, M_FileError(file->file));
			file->currentsize += boundedfragmentsize;

			AddFragmentToAckPacket(file->ackpacket, file->iteration, fragmentpos / fragmentsize, filenum);


			if (file->currentsize == file->totalsize)
			{
				fclose(file->file);
				file->file = NULL;
				free(file->receivedfragments);
				free(file->ackpacket);
				file->status = FS_FOUND;
				file->justdownloaded = true;


				netbuffer->packettype = PT_FILERECEIVED;
				netbuffer->u.filereceived = filenum;
				HSendPacket(servernode, true, 0, 1);

				if (luafiletransfers)
				{

					netbuffer->packettype = PT_HASLUAFILE;
					HSendPacket(servernode, true, 0, 0);
					FreeFileNeeded();
				}
				else
				{
					filedownload.completednum++;
					filedownload.completedsize += file->totalsize;
					filedownload.remaining--;
				}

				CONS_Printf(M_GetText("Finished download of \"%s\"\n"), filename);
			}
		}
		else  
		{


			AddFragmentToAckPacket(file->ackpacket, file->iteration, fragmentpos / fragmentsize, filenum);
		}
	}
	else if (!file->justdownloaded)
	{
		const char *s;
		switch(file->status)
		{
		case FS_NOTFOUND:
			s = "FS_NOTFOUND";
			break;
		case FS_FOUND:
			s = "FS_FOUND";
			break;
		case FS_OPEN:
			s = "FS_OPEN";
			break;
		case FS_MD5SUMBAD:
			s = "FS_MD5SUMBAD";
			break;
		default:
			s = "unknown";
			break;
		}
		I_Error("Received a file not requested (file id: %d, file status: %s)\n", filenum, s);
	}

	filedownload.current = filenum;
}

                                                 
  
                                    
                                                 
  
   
boolean SendingFile(INT32 node)
{
	return transfer[node].txlist != NULL;
}

                                        
   
                               
                      
   
    
void SV_AbortSendFiles(INT32 node)
{
	while (transfer[node].txlist)
		SV_EndFileSend(node);
}

void CloseNetFile(void)
{

	for (INT32 i = 0; i < MAXNETNODES; i++)
		SV_AbortSendFiles(i);


	if (fileneeded)
	{
		for (INT32 i = 0; i < fileneedednum; i++)
			if (fileneeded[i].status == FS_DOWNLOADING && fileneeded[i].file)
			{
				fclose(fileneeded[i].file);
				free(fileneeded[i].ackpacket);

				if (!pauseddownload && (fileneeded[i].type == FILENEEDED_WAD || i != 0))  
				{

					pauseddownload = malloc(sizeof(*pauseddownload));
					if (!pauseddownload)
						I_Error("CloseNetFile: No more memory\n");

					strcpy(pauseddownload->filename, fileneeded[i].filename);
					memcpy(pauseddownload->md5sum, fileneeded[i].md5sum, 16);
					pauseddownload->currentsize = fileneeded[i].currentsize;
					pauseddownload->receivedfragments = fileneeded[i].receivedfragments;
					pauseddownload->fragmentsize = fileneeded[i].fragmentsize;
				}
				else
				{

					free(fileneeded[i].receivedfragments);
					remove(fileneeded[i].filename);
				}
			}
	}

	FreeFileNeeded();
}

void Command_Downloads_f(void)
{
	for (INT32 node = 0; node < MAXNETNODES; node++)
		if (transfer[node].txlist
		&& transfer[node].txlist->ram == SF_FILE)  
		{
			const char *name = transfer[node].txlist->id.filename;
			UINT32 position = transfer[node].ackedsize;
			UINT32 size = transfer[node].txlist->size;
			char ratecolor;


			if (!size)
				size = 1;

			name = &name[strlen(name) - nameonlylength(name)];
			switch (4 * (position - 1) / size)
			{
				case 0: ratecolor = '\x85'; break;
				case 1: ratecolor = '\x87'; break;
				case 2: ratecolor = '\x82'; break;
				case 3: ratecolor = '\x83'; break;
				default: ratecolor = '\x80';
			}

			CONS_Printf("%2d  %c%s  ", node, ratecolor, name);  
			CONS_Printf("\x80%uK\x84/\x80%uK ", position / 1024, size / 1024);  
			CONS_Printf("\x80(%c%u%%\x80)  ", ratecolor, (UINT32)(100.0 * position / size));  
			CONS_Printf("%s\n", I_GetNodeAddress(node));  
		}
}

static size_t curlwrite_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return fwrite(ptr, size, nmemb, stream);
}

static int curlprogress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	time_t elapsed;

	(void)clientp;
	(void)ultotal;
	(void)ulnow;  
	curl_dlnow = (UINT32)dlnow;
	curl_dltotal = (UINT32)dltotal;
	elapsed = time(NULL) - curl_starttime;
	getbytes = elapsed > 0 ? ((double)dlnow) / elapsed : 0.0;
	return 0;
}

boolean CURLPrepareFile(const char* url, int dfilenum)
{
	HTTP_login *login;
	char requesturl[MAX_MIRROR_LENGTH + (MAX_WADPATH * 3) + 40];
	char *escapedname;

#ifdef PARANOIA
	if (M_CheckParm("-nodownload"))
		I_Error("Attempted to download files in -nodownload mode");
#endif

	if (!multi_handle)
	{
		curl_global_init(CURL_GLOBAL_ALL);
		multi_handle = curl_multi_init();
	}

	http_handle = curl_easy_init();
	if (http_handle && multi_handle)
	{
		I_mkdir(downloaddir, 0755);

		curl_curfile = &fileneeded[dfilenum];
		curl_realname = curl_curfile->filename;
		nameonly(curl_realname);

		curl_origfilesize = curl_curfile->currentsize;
		curl_origtotalfilesize = curl_curfile->totalsize;

		char md5tmp[33];
		for (INT32 j = 0; j < 16; j++)
			sprintf(&md5tmp[j*2], "%02x", curl_curfile->md5sum[j]);

		escapedname = curl_easy_escape(http_handle, curl_realname, 0);
		if (!escapedname)
		{
			curl_easy_cleanup(http_handle);
			http_handle = NULL;
			filedownload.http_running = false;
			return false;
		}
		snprintf(requesturl, sizeof requesturl, "%s/%s?md5=%s", url, escapedname, md5tmp);
		curl_free(escapedname);
		curl_easy_setopt(http_handle, CURLOPT_URL, requesturl);


#if (LIBCURL_VERSION_MAJOR <= 7) && (LIBCURL_VERSION_MINOR < 85)
		curl_easy_setopt(http_handle, CURLOPT_PROTOCOLS, CURLPROTO_HTTP|CURLPROTO_HTTPS);
		curl_easy_setopt(http_handle, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP|CURLPROTO_HTTPS);
#else
		curl_easy_setopt(http_handle, CURLOPT_PROTOCOLS_STR, "http,https");
		curl_easy_setopt(http_handle, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#endif


		curl_easy_setopt(http_handle, CURLOPT_USERAGENT, va("Sonic Robo Blast 2/%s", VERSIONSTRING));


		login = CURLGetLogin(url, NULL);

		if (login)
		{
			curl_easy_setopt(http_handle, CURLOPT_USERPWD, login->auth);
		}


		curl_easy_setopt(http_handle, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(http_handle, CURLOPT_MAXREDIRS, 5L);

		curl_easy_setopt(http_handle, CURLOPT_FAILONERROR, 1L);
		                                                                       
                                                                         
                                                     
		curl_easy_setopt(http_handle, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(http_handle, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(http_handle, CURLOPT_BUFFERSIZE, 256L * 1024L);
#if LIBCURL_VERSION_NUM >= 0x072100
		curl_easy_setopt(http_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
#endif

		CONS_Printf("Downloading addon \"%s\" from %s\n", curl_realname, url);

		strcatbf(curl_curfile->filename, downloaddir, "/");
		curl_curfile->file = fopen(curl_curfile->filename, "wb");
		curl_easy_setopt(http_handle, CURLOPT_WRITEDATA, curl_curfile->file);
		curl_easy_setopt(http_handle, CURLOPT_WRITEFUNCTION, curlwrite_data);
		curl_easy_setopt(http_handle, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(http_handle, CURLOPT_XFERINFOFUNCTION, curlprogress_callback);

		curl_curfile->status = FS_DOWNLOADING;
		curl_multi_add_handle(multi_handle, http_handle);

		curl_multi_perform(multi_handle, &curl_runninghandles);
		curl_starttime = time(NULL);

		filedownload.current = dfilenum;
		filedownload.http_running = true;

		I_spawn_thread("http-download", (I_thread_fn)CURLGetFile, NULL);

		return true;
	}

	filedownload.http_running = false;

	return false;
}

void CURLAbortFile(void)
{
	filedownload.http_running = false;


	I_lock_mutex(&downloadmutex);
	I_unlock_mutex(downloadmutex);
}

void CURLGetFile(void)
{
	I_lock_mutex(&downloadmutex);
	CURLMcode mc;  
	CURLcode easyres;  
	CURLMsg *m;  
	CURL *e;
	int msgs_left;  
	const char *easy_handle_error;
	boolean running = true;

	while (running && filedownload.http_running)
	{
		if (curl_runninghandles)
		{
			curl_multi_perform(multi_handle, &curl_runninghandles);

			 
			mc = curl_multi_wait(multi_handle, NULL, 0, 1000, NULL);

			if (mc != CURLM_OK)
			{
				CONS_Alert(CONS_WARNING, "curl_multi_wait() failed, code %d.\n", mc);
				continue;
			}
			curl_curfile->currentsize = curl_dlnow;
			curl_curfile->totalsize = curl_dltotal;
		}

		 
		while ((m = curl_multi_info_read(multi_handle, &msgs_left)))
		{
			if (m && (m->msg == CURLMSG_DONE))
			{
				running = false;
				e = m->easy_handle;
				easyres = m->data.result;

				char *filename = Z_StrDup(curl_realname);
				nameonly(filename);

				if (easyres != CURLE_OK)
				{
					long response_code = 0;

					if (easyres == CURLE_HTTP_RETURNED_ERROR)
						curl_easy_getinfo(e, CURLINFO_RESPONSE_CODE, &response_code);

					if (response_code == 404)
						curl_curfile->failed = FDOWNLOAD_FAIL_NOTFOUND;
					else
						curl_curfile->failed = FDOWNLOAD_FAIL_OTHER;

					easy_handle_error = (response_code) ? va("HTTP response code %ld", response_code) : curl_easy_strerror(easyres);
					curl_curfile->status = FS_FALLBACK;
					curl_curfile->currentsize = curl_origfilesize;
					curl_curfile->totalsize = curl_origtotalfilesize;
					filedownload.http_failed = true;
					fclose(curl_curfile->file);
					remove(curl_curfile->filename);
					CONS_Alert(CONS_ERROR, M_GetText("Failed to download addon \"%s\" (%s)\n"), filename, easy_handle_error);
				}
				else
				{
					fclose(curl_curfile->file);

					CONS_Printf(M_GetText("Finished download of \"%s\"\n"), filename);

					if (checkfilemd5(curl_curfile->filename, curl_curfile->md5sum) == FS_MD5SUMBAD)
					{
						CONS_Alert(CONS_WARNING, M_GetText("File \"%s\" does not match the version used by the server\n"), filename);
						curl_curfile->status = FS_FALLBACK;
						curl_curfile->failed = FDOWNLOAD_FAIL_MD5SUMBAD;
						filedownload.http_failed = true;
					}
					else
					{
						filedownload.completednum++;
						filedownload.completedsize += curl_curfile->totalsize;
						curl_curfile->status = FS_FOUND;
					}
				}

				Z_Free(filename);

				curl_curfile->file = NULL;
				filedownload.remaining--;
				curl_multi_remove_handle(multi_handle, e);
				curl_easy_cleanup(e);

				if (!filedownload.remaining)
					break;
			}
		}
	}

	if (!filedownload.remaining || !filedownload.http_running)
	{
		curl_multi_cleanup(multi_handle);
		curl_global_cleanup();
		multi_handle = NULL;
	}
	filedownload.http_running = false;
	I_unlock_mutex(downloadmutex);
}

HTTP_login *
CURLGetLogin (const char *url, HTTP_login ***return_prev_next)
{
	HTTP_login  * login;
	HTTP_login ** prev_next;

	for (
			prev_next = &curl_logins;
			( login = (*prev_next));
			prev_next = &login->next
	){
		if (strcmp(login->url, url) == 0)
		{
			if (return_prev_next)
				(*return_prev_next) = prev_next;

			return login;
		}
	}

	return NULL;
}



void nameonly(char *s)
{
	for (size_t j = strlen(s); j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
		{
			void *ns = &(s[j+1]);
			size_t len = strlen(ns);
#if 0
			M_Memcpy(s, ns, len+1);
#else
			memmove(s, ns, len+1);
#endif
			return;
		}
}


size_t nameonlylength(const char *s)
{
	size_t len = strlen(s);

	for (size_t j = len; j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
			return len - j - 1;

	return len;
}

#ifndef O_BINARY
#define O_BINARY 0
#endif

#if !defined(NOMD5)
#define MD5_CACHE_SLOTS 256
typedef struct
{
	boolean valid;
	char filename[MAX_WADPATH];
	UINT64 size;
	time_t mtime;
	time_t ctime;
	UINT64 quicksig;
	UINT8 md5sum[16];
} md5cacheentry_t;

static md5cacheentry_t md5cache[MD5_CACHE_SLOTS];
static UINT32 md5cache_next;
static boolean md5cache_loaded;
static UINT32 md5cache_hits, md5cache_misses, md5cache_diskentries, md5cache_writes;
static UINT32 md5cache_duplicates, md5cache_stale;
static UINT32 md5cache_pendingwrites;
static boolean md5cache_dirty, md5cache_exitregistered;
#ifdef HAVE_THREADS
static I_mutex md5cache_mutex;
#define MD5_CACHE_LOCK() I_lock_mutex(&md5cache_mutex)
#define MD5_CACHE_UNLOCK() I_unlock_mutex(md5cache_mutex)
#else
#define MD5_CACHE_LOCK() ((void)0)
#define MD5_CACHE_UNLOCK() ((void)0)
#endif

void D_FlushPersistentMD5Cache(void);

static boolean MD5CachePathEqual(const char *left, const char *right)
{
#ifdef _WIN32
	return !stricmp(left, right);
#else
	return !strcmp(left, right);
#endif
}

                                                                           
                                                                               
                                                                              
static void MD5CacheCanonicalPath(const char *source, char *destination,
	size_t destinationsize)
{
	const char *canonical = source;
#ifdef _WIN32
	char fullpath[MAX_WADPATH];
	if (_fullpath(fullpath, source, sizeof fullpath))
		canonical = fullpath;
#elif defined(PATH_MAX)
	char fullpath[PATH_MAX];
	if (realpath(source, fullpath))
		canonical = fullpath;
#endif

	strlcpy(destination, canonical, destinationsize);
#ifdef _WIN32
	for (char *p = destination; *p; p++)
		if (*p == '/')
			*p = '\\';
#endif
}

static void MD5ToText(const UINT8 *md5sum, char text[33])
{
	static const char hex[] = "0123456789abcdef";
	UINT32 i;

	for (i = 0; i < 16; i++)
	{
		text[i * 2] = hex[md5sum[i] >> 4];
		text[i * 2 + 1] = hex[md5sum[i] & 15];
	}
	text[32] = '\0';
}

static boolean TextToMD5(const char *text, UINT8 md5sum[16])
{
	UINT32 i;

	if (strlen(text) != 32)
		return false;
	for (i = 0; i < 16; i++)
	{
		char pair[3] = {text[i * 2], text[i * 2 + 1], '\0'};
		char *end;
		unsigned long value = strtoul(pair, &end, 16);
		if (*end)
			return false;
		md5sum[i] = (UINT8)value;
	}
	return true;
}

static void MD5CacheGetPaths(char *directory, size_t directorysize,
	char *path, size_t pathsize, char *temporary, size_t temporarysize)
{
	snprintf(directory, directorysize, "%s" PATHSEP "cache", srb2home);
	directory[directorysize - 1] = '\0';
	snprintf(path, pathsize, "%s" PATHSEP "addon-md5-v5.txt", directory);
	path[pathsize - 1] = '\0';
	snprintf(temporary, temporarysize, "%s.tmp", path);
	temporary[temporarysize - 1] = '\0';
}

static void LoadPersistentMD5Cache(void)
{
	char directory[MAX_WADPATH * 2], path[MAX_WADPATH * 2], temporary[MAX_WADPATH * 2];
	char line[MAX_WADPATH + 160];
	boolean preexisting[MD5_CACHE_SLOTS] = {false};
	boolean diskseen[MD5_CACHE_SLOTS] = {false};
	FILE *file;
	UINT32 i;

	if (md5cache_loaded || !cv_persistentmd5.value)
		return;
	md5cache_loaded = true;
	                                                                           
                                                                             
                                                                            
	for (i = 0; i < MD5_CACHE_SLOTS; i++)
		preexisting[i] = md5cache[i].valid;
	if (!md5cache_exitregistered)
	{
		I_AddExitFunc(D_FlushPersistentMD5Cache);
		md5cache_exitregistered = true;
	}
	MD5CacheGetPaths(directory, sizeof directory, path, sizeof path, temporary, sizeof temporary);
	(void)directory;
	(void)temporary;
	file = fopen(path, "rb");
	if (!file)
		return;
	if (!fgets(line, sizeof line, file) || strcmp(line, "SRB2MD5CACHE 1\n"))
	{
		fclose(file);
		return;
	}

	while (md5cache_next < MD5_CACHE_SLOTS && fgets(line, sizeof line, file))
	{
		md5cacheentry_t parsed;
		md5cacheentry_t *entry = &parsed;
		struct stat filestat;
		char canonical[MAX_WADPATH];
		char md5text[33];
		char *field = line;
		char *tab;
		char *end;
		unsigned long long value;

		memset(&parsed, 0, sizeof parsed);

#define NEXT_MD5_FIELD() \
		do { tab = strchr(field, '\t'); if (!tab) goto bad_cache_line; *tab = '\0'; } while (0)
		NEXT_MD5_FIELD();
		value = strtoull(field, &end, 10); if (*end) goto bad_cache_line;
		entry->size = (UINT64)value; field = tab + 1;
		NEXT_MD5_FIELD();
		value = strtoull(field, &end, 10); if (*end) goto bad_cache_line;
		entry->mtime = (time_t)value; field = tab + 1;
		NEXT_MD5_FIELD();
		value = strtoull(field, &end, 10); if (*end) goto bad_cache_line;
		entry->ctime = (time_t)value; field = tab + 1;
		NEXT_MD5_FIELD();
		value = strtoull(field, &end, 16); if (*end) goto bad_cache_line;
		entry->quicksig = (UINT64)value; field = tab + 1;
		NEXT_MD5_FIELD();
		if (strlen(field) != 32) goto bad_cache_line;
		strcpy(md5text, field); field = tab + 1;
		field[strcspn(field, "\r\n")] = '\0';
		if (!field[0] || strlen(field) >= sizeof entry->filename || !TextToMD5(md5text, entry->md5sum))
			goto bad_cache_line;
		MD5CacheCanonicalPath(field, canonical, sizeof canonical);
		strcpy(entry->filename, canonical);
		entry->valid = true;
		                                                                      
                                                                         
                                   
		if (stat(entry->filename, &filestat) != 0
		|| entry->size != (UINT64)filestat.st_size
		|| entry->mtime != filestat.st_mtime
		|| entry->ctime != filestat.st_ctime)
		{
			md5cache_stale++;
			md5cache_dirty = true;
			continue;
		}
		for (i = 0; i < md5cache_next; i++)
		{
			if (!md5cache[i].valid
			|| !MD5CachePathEqual(md5cache[i].filename, entry->filename))
				continue;
			if (preexisting[i])
			{
				                                                          
                                                                      
				if (diskseen[i])
				{
					md5cache_duplicates++;
					md5cache_dirty = true;
				}
				diskseen[i] = true;
				goto loaded_cache_line;
			}
			                                                               
                                              
			md5cache[i] = parsed;
			diskseen[i] = true;
			md5cache_duplicates++;
			md5cache_dirty = true;
			goto loaded_cache_line;
		}
		md5cache[md5cache_next] = parsed;
		diskseen[md5cache_next] = true;
		md5cache_next++;
		md5cache_diskentries++;
	loaded_cache_line:
		continue;
bad_cache_line:
		md5cache_stale++;
		md5cache_dirty = true;
#undef NEXT_MD5_FIELD
	}
	fclose(file);
}

static void SavePersistentMD5Cache(void)
{
	char directory[MAX_WADPATH * 2], path[MAX_WADPATH * 2], temporary[MAX_WADPATH * 2];
	FILE *file;
	UINT32 i;

	if (!cv_persistentmd5.value)
		return;
	MD5CacheGetPaths(directory, sizeof directory, path, sizeof path, temporary, sizeof temporary);
	I_mkdir(directory, 0755);
	file = fopen(temporary, "wb");
	if (!file)
		return;
	fputs("SRB2MD5CACHE 1\n", file);
	for (i = 0; i < MD5_CACHE_SLOTS; i++)
	{
		char md5text[33];
		md5cacheentry_t *entry = &md5cache[i];
		if (!entry->valid)
			continue;
		MD5ToText(entry->md5sum, md5text);
		fprintf(file, "%llu\t%llu\t%llu\t%016llx\t%s\t%s\n",
			(unsigned long long)entry->size,
			(unsigned long long)entry->mtime,
			(unsigned long long)entry->ctime,
			(unsigned long long)entry->quicksig, md5text, entry->filename);
	}
	if (fclose(file) != 0)
	{
		remove(temporary);
		return;
	}
	remove(path);
	if (rename(temporary, path) == 0)
	{
		md5cache_writes++;
		md5cache_dirty = false;
		md5cache_pendingwrites = 0;
	}
}

void D_FlushPersistentMD5Cache(void)
{
	MD5_CACHE_LOCK();
	if (md5cache_dirty)
		SavePersistentMD5Cache();
	MD5_CACHE_UNLOCK();
}

static boolean FileQuickSignature(FILE *file, const struct stat *filestat, UINT64 *signature)
{
	static const UINT32 fractions[] = {0, 1, 2, 3, 4};
	UINT8 buffer[4096];
	UINT64 filesize = (UINT64)filestat->st_size;
	UINT64 hash = 1469598103934665603ULL ^ filesize;
	UINT32 i;

	for (i = 0; i < sizeof fractions / sizeof fractions[0]; i++)
	{
		UINT64 offset = filesize > sizeof buffer
			? ((filesize - sizeof buffer) * fractions[i]) / 4 : 0;
		size_t wanted = (size_t)(filesize < (UINT64)sizeof buffer
			? filesize : (UINT64)sizeof buffer);
		size_t got;
		UINT32 j;

#ifdef _WIN32
		if (_fseeki64(file, (INT64)offset, SEEK_SET) != 0)
#else
		if (fseeko(file, (off_t)offset, SEEK_SET) != 0)
#endif
			goto failed;
		got = fread(buffer, 1, wanted, file);
		if (got != wanted)
			goto failed;
		hash ^= offset;
		hash *= 1099511628211ULL;
		for (j = 0; j < got; j++)
		{
			hash ^= buffer[j];
			hash *= 1099511628211ULL;
		}


		if (filesize <= sizeof buffer)
			break;
	}
	clearerr(file);
	rewind(file);
	*signature = hash;
	return true;

failed:
	clearerr(file);
	rewind(file);
	return false;
}

static boolean CachedFileMD5(const char *filename, const struct stat *filestat,
	UINT64 quicksig, UINT8 *md5sum)
{
	boolean found = false;

	MD5_CACHE_LOCK();
	LoadPersistentMD5Cache();
	for (UINT32 i = 0; i < MD5_CACHE_SLOTS; i++)
	{
		if (md5cache[i].valid
		&& md5cache[i].size == (UINT64)filestat->st_size
		&& md5cache[i].mtime == filestat->st_mtime
		&& md5cache[i].ctime == filestat->st_ctime
		&& md5cache[i].quicksig == quicksig
		&& MD5CachePathEqual(md5cache[i].filename, filename))
		{
			M_Memcpy(md5sum, md5cache[i].md5sum, 16);
			found = true;
			md5cache_hits++;
			break;
		}
	}
	if (!found)
		md5cache_misses++;
	MD5_CACHE_UNLOCK();
	return found;
}

                                                                          
                                                                             
                                                             
static boolean FindCachedFileByMD5(const char *requested, const UINT8 *wantedmd5sum,
	char *resolved, size_t resolvedsize)
{
	md5cacheentry_t candidate;
	boolean found = false;
	struct stat filestat;
	FILE *file;
	UINT64 quicksig;
	const char *requestedbase = requested + strlen(requested) - nameonlylength(requested);

	if (!wantedmd5sum)
		return false;

	MD5_CACHE_LOCK();
	LoadPersistentMD5Cache();
	for (UINT32 i = 0; i < MD5_CACHE_SLOTS; i++)
	{
		const char *cachedbase;

		if (!md5cache[i].valid || memcmp(md5cache[i].md5sum, wantedmd5sum, 16))
			continue;
		cachedbase = md5cache[i].filename + strlen(md5cache[i].filename)
			- nameonlylength(md5cache[i].filename);
		if (!MD5CachePathEqual(cachedbase, requestedbase))
			continue;
		candidate = md5cache[i];
		found = true;
		break;
	}
	MD5_CACHE_UNLOCK();

	if (!found || stat(candidate.filename, &filestat) != 0
	|| candidate.size != (UINT64)filestat.st_size
	|| candidate.mtime != filestat.st_mtime
	|| candidate.ctime != filestat.st_ctime)
		return false;

	file = fopen(candidate.filename, "rb");
	if (!file)
		return false;
	found = FileQuickSignature(file, &filestat, &quicksig)
		&& quicksig == candidate.quicksig;
	fclose(file);
	if (!found || strlen(candidate.filename) >= resolvedsize)
		return false;

	strcpy(resolved, candidate.filename);
	MD5_CACHE_LOCK();
	md5cache_hits++;
	MD5_CACHE_UNLOCK();
	return true;
}

static void CacheFileMD5(const char *filename, const struct stat *filestat,
	UINT64 quicksig, const UINT8 *md5sum)
{
	md5cacheentry_t *entry = NULL;
	UINT32 i;

	MD5_CACHE_LOCK();
	LoadPersistentMD5Cache();
	                                                                      
                                                                         
	for (i = 0; i < MD5_CACHE_SLOTS; i++)
		if (md5cache[i].valid
		&& MD5CachePathEqual(md5cache[i].filename, filename))
		{
			entry = &md5cache[i];
			break;
		}
	if (!entry)
		entry = &md5cache[md5cache_next++ % MD5_CACHE_SLOTS];
	entry->valid = true;
	strncpy(entry->filename, filename, sizeof entry->filename - 1);
	entry->filename[sizeof entry->filename - 1] = '\0';
	entry->size = (UINT64)filestat->st_size;
	entry->mtime = filestat->st_mtime;
	entry->ctime = filestat->st_ctime;
	entry->quicksig = quicksig;
	M_Memcpy(entry->md5sum, md5sum, 16);
	md5cache_dirty = true;
	md5cache_pendingwrites++;
	                                                                         
                                                                           
                                                                         
                                                                               
	if (md5cache_pendingwrites >= 32)
		SavePersistentMD5Cache();
	MD5_CACHE_UNLOCK();
}

void D_MD5CacheStats_f(void)
{
	UINT32 active = 0;

	MD5_CACHE_LOCK();
	LoadPersistentMD5Cache();
	for (UINT32 i = 0; i < MD5_CACHE_SLOTS; i++)
		if (md5cache[i].valid)
			active++;
	MD5_CACHE_UNLOCK();
	CONS_Printf("Addon checksum cache: %u hits, %u misses, %u active entries, %u loaded, %u duplicate and %u stale records cleaned, %u disk updates, %u pending\n",
		md5cache_hits, md5cache_misses, active, md5cache_diskentries,
		md5cache_duplicates, md5cache_stale, md5cache_writes,
		md5cache_pendingwrites);
}

void D_ClearMD5Cache_f(void)
{
	char directory[MAX_WADPATH * 2], path[MAX_WADPATH * 2], temporary[MAX_WADPATH * 2];

	MD5CacheGetPaths(directory, sizeof directory, path, sizeof path, temporary, sizeof temporary);
	MD5_CACHE_LOCK();
	memset(md5cache, 0, sizeof md5cache);
	md5cache_next = 0;
	md5cache_loaded = false;
	md5cache_hits = md5cache_misses = md5cache_diskentries = md5cache_writes = 0;
	md5cache_duplicates = md5cache_stale = md5cache_pendingwrites = 0;
	md5cache_dirty = false;
	remove(temporary);
	remove(path);
#ifdef _WIN32
	_rmdir(directory);
#else
	rmdir(directory);
#endif
	MD5_CACHE_UNLOCK();
	CONS_Printf("Addon checksum cache cleared. It will stay in memory only when persistent_addon_cache is Off.\n");
}

boolean D_GetCachedFileMD5(const char *filename, UINT8 md5sum[16])
{
	FILE *fhandle;
	struct stat before, after;
	boolean have_before, have_quicksig;
	UINT64 quicksig = 0, afterquicksig = 0;
	char cachefilename[MAX_WADPATH];

	MD5CacheCanonicalPath(filename, cachefilename, sizeof cachefilename);

	fhandle = fopen(filename, "rb");
	if (!fhandle)
		return false;
	have_before = (stat(filename, &before) == 0);
	have_quicksig = have_before
		&& FileQuickSignature(fhandle, &before, &quicksig);
	if (have_quicksig && CachedFileMD5(cachefilename, &before, quicksig, md5sum))
	{
		fclose(fhandle);
		return true;
	}

	if (md5_stream(fhandle, md5sum) == 1)
	{
		fclose(fhandle);
		return false;
	}
	 
	have_quicksig = have_quicksig
		&& FileQuickSignature(fhandle, &before, &afterquicksig)
		&& quicksig == afterquicksig;
	fclose(fhandle);
	if (have_quicksig && stat(filename, &after) == 0
	&& before.st_size == after.st_size && before.st_mtime == after.st_mtime
	&& before.st_ctime == after.st_ctime)
		CacheFileMD5(cachefilename, &after, quicksig, md5sum);
	return true;
}
#endif

#if defined (NOMD5)
void D_FlushPersistentMD5Cache(void)
{
}

void D_ClearMD5Cache_f(void)
{
	CONS_Printf("This build has no add-on checksum cache.\n");
}

boolean D_GetCachedFileMD5(const char *filename, UINT8 md5sum[16])
{
	(void)filename;
	(void)md5sum;
	return false;
}
#endif

filestatus_t checkfilemd5(char *filename, const UINT8 *wantedmd5sum)
{
#if defined (NOMD5)
	(void)wantedmd5sum;
	(void)filename;
#else
	UINT8 md5sum[16];

	if (!wantedmd5sum)
		return FS_FOUND;
	if (D_GetCachedFileMD5(filename, md5sum))
		return !memcmp(wantedmd5sum, md5sum, 16) ? FS_FOUND : FS_MD5SUMBAD;

	I_Error("Couldn't open %s for md5 check", filename);
#endif
	return FS_FOUND;  
}

                                                                          
                                                                           
                                                                              
                                                                 
static filestatus_t CheckExactAddonPath(char *filename, const char *directory,
	const UINT8 *wantedmd5sum, boolean completepath)
{
	char candidate[1024];
	char basename[MAX_WADPATH];
	FILE *probe;
	filestatus_t status;
	size_t directorylen;

	if (!directory || !directory[0])
		return FS_NOTFOUND;
	strlcpy(basename,
		filename + strlen(filename) - nameonlylength(filename), sizeof basename);
	directorylen = strlen(directory);
	if (snprintf(candidate, sizeof candidate, "%s%s%s", directory,
		(directorylen && directory[directorylen - 1] != '/' &&
		directory[directorylen - 1] != '\\') ? PATHSEP : "", basename)
		>= (INT32)sizeof candidate)
		return FS_NOTFOUND;
	 
	probe = fopen(candidate, "rb");
	if (!probe)
		return FS_NOTFOUND;
	fclose(probe);

	status = checkfilemd5(candidate, wantedmd5sum);
	if (status == FS_FOUND)
	{
		if (completepath)
		{
			if (strlen(candidate) >= MAX_WADPATH)
				return FS_NOTFOUND;
			strcpy(filename, candidate);
		}
		else
			strlcpy(filename, basename, MAX_WADPATH);
	}
	return status;
}




filestatus_t findfile(char *filename, const UINT8 *wantedmd5sum, boolean completepath)
{
	filestatus_t homecheck;  
	boolean badmd5 = false;  

	char override_dir[sizeof(srb2home) + 9];
	strcpy(override_dir, srb2home);
	strcat(override_dir, PATHSEP "OVERRIDE");
	homecheck = filesearch(filename, override_dir, NULL, completepath, 10);
	if (homecheck == FS_FOUND)
		return FS_FOUND;

	                                                                          
                                                                          
                                         
	homecheck = CheckExactAddonPath(filename, downloaddir, wantedmd5sum, completepath);
	if (homecheck == FS_FOUND)
		return FS_FOUND;
	else if (homecheck == FS_MD5SUMBAD)
		badmd5 = true;

#if !defined(NOMD5)
	{
		char cachedpath[MAX_WADPATH];
		if (FindCachedFileByMD5(filename, wantedmd5sum, cachedpath, sizeof cachedpath))
		{
			if (completepath)
				strcpy(filename, cachedpath);
			else
				nameonly(strcpy(filename, cachedpath));
			return FS_FOUND;
		}
	}
#endif


	homecheck = filesearch(filename, srb2home, wantedmd5sum, completepath, 10);

	if (homecheck == FS_FOUND)  
		return FS_FOUND;
	else if (homecheck == FS_MD5SUMBAD)  
		badmd5 = true;



	homecheck = filesearch(filename, srb2path, wantedmd5sum, completepath, 10);

	if (homecheck == FS_FOUND)  
		return FS_FOUND;
	else if (homecheck == FS_MD5SUMBAD)  
		badmd5 = true;



	homecheck = filesearch(filename, ".", wantedmd5sum, completepath, 10);

	if (homecheck != FS_NOTFOUND)  
		return homecheck;  

	return (badmd5 ? FS_MD5SUMBAD : FS_NOTFOUND);  
}





filestatus_t findfolder(const char *path)
{

	if (concatpaths(path, NULL) == 1)
		return FS_FOUND;

#define checkpath(startpath) \
	if (concatpaths(path, startpath) == 1) \
		return FS_FOUND

	checkpath(srb2home);  
	checkpath(srb2path);  
	checkpath(".");  

#undef checkpath

	return FS_NOTFOUND;
}
