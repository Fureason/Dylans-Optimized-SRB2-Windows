// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __D_NETFIL__
#define __D_NETFIL__

#include "d_net.h"
#include "d_clisrv.h"
#include "../w_wad.h"

typedef enum
{
	SF_FILE,
	SF_Z_RAM,
	SF_RAM,
	SF_NOFREERAM
} freemethod_t;

typedef enum
{
	DLSTATUS_OK,
	DLSTATUS_TOOLARGE,
	DLSTATUS_WONTSEND,
	DLSTATUS_NODOWNLOAD,
	DLSTATUS_FOLDER
} dlstatus_t;

typedef enum
{
	FDOWNLOAD_FAIL_NONE,
	FDOWNLOAD_FAIL_NOTFOUND,
	FDOWNLOAD_FAIL_MD5SUMBAD,
	FDOWNLOAD_FAIL_OTHER
} filedownloadfail_t;

typedef enum
{
	FS_NOTCHECKED,
	FS_NOTFOUND,
	FS_FOUND,
	FS_REQUESTED,
	FS_DOWNLOADING,
	FS_OPEN,  
	FS_MD5SUMBAD,
	FS_FALLBACK
} filestatus_t;

typedef enum
{
	FILENEEDED_WAD,
	FILENEEDED_SAVEGAME,
	FILENEEDED_LUAFILE
} fileneededtype_t;

typedef struct
{
	char filename[MAX_WADPATH];
	UINT8 md5sum[16];
	filestatus_t status;  
	UINT8 willsend;  
	UINT8 folder;  
	fileneededtype_t type;
	filedownloadfail_t failed;
	boolean justdownloaded;  


	FILE *file;
	boolean *receivedfragments;
	UINT32 fragmentsize;
	UINT8 iteration;
	fileack_pak *ackpacket;
	UINT32 currentsize;
	UINT32 totalsize;
	UINT32 ackresendposition;  
} fileneeded_t;

#define FILENEEDEDSIZE 23

extern INT32 fileneedednum;
extern fileneeded_t *fileneeded;
extern char downloaddir[512];

typedef struct
{
	INT32 current;
	INT32 remaining;
	INT32 completednum;
	UINT32 completedsize;

	boolean http_failed;
	boolean http_running;

	char http_source[MAX_MIRROR_LENGTH];
} file_download_t;

extern file_download_t filedownload;

typedef struct HTTP_login HTTP_login;

extern struct HTTP_login
{
	char       * url;
	char       * auth;
	HTTP_login * next;
}
*curl_logins;

extern consvar_t cv_maxsend, cv_autosendaddons, cv_noticedownload, cv_downloadspeed, cv_autodownloadspeed, cv_persistentmd5;

                                                                            
                                                                                   
                                                                         
                         
double D_FileTransferMBPerSecond(INT32 packetspertic);
 
INT32 D_FileTransferPacketsForMBPerSecond(double megabytespersecond);

void D_MD5CacheStats_f(void);
void D_ClearMD5Cache_f(void);
 
void D_FlushPersistentMD5Cache(void);

void AllocFileNeeded(INT32 size);
void FreeFileNeeded(void);
UINT8 *PutFileNeeded(UINT16 firstfile);
void D_ParseFileneeded(INT32 fileneedednum_parm, UINT8 *fileneededstr, UINT16 firstfile);
void CL_PrepareDownloadSaveGame(const char *tmpsave);

INT32 CL_CheckFiles(void);
boolean CL_LoadServerFiles(void);
void AddRamToSendQueue(INT32 node, void *data, size_t size, freemethod_t freemethod,
	UINT8 fileid);

void FileSendTicker(void);
void PT_FileAck(SINT8 node);
void PT_FileReceived(SINT8 node);
boolean SendingFile(INT32 node);

void FileReceiveTicker(void);
void PT_FileFragment(SINT8 node, INT32 netconsole);

UINT8 CL_CheckDownloadable(boolean direct);
boolean CL_SendFileRequest(void);
void PT_RequestFile(SINT8 node);

boolean CURLPrepareFile(const char* url, int dfilenum);
void CURLAbortFile(void);
void CURLGetFile(void);
HTTP_login * CURLGetLogin (const char *url, HTTP_login ***return_prev_next);

typedef enum
{
	LFTNS_NONE,     
	LFTNS_WAITING,  
	LFTNS_ASKED,    
	LFTNS_SENDING,  
	LFTNS_SENT      
} luafiletransfernodestatus_t;

typedef struct luafiletransfer_s
{
	char *filename;
	char *realfilename;
	char mode[4];  
	INT32 id;  
	boolean ongoing;
	luafiletransfernodestatus_t nodestatus[MAXNETNODES];
	tic_t nodetimeouts[MAXNETNODES];
	struct luafiletransfer_s *next;
} luafiletransfer_t;

extern luafiletransfer_t *luafiletransfers;
extern boolean waitingforluafiletransfer;
extern boolean waitingforluafilecommand;
extern char luafiledir[256 + 16];

void AddLuaFileTransfer(const char *filename, const char *mode);
void SV_PrepareSendLuaFile(void);
boolean AddLuaFileToSendQueue(INT32 node, const char *filename);
void SV_HandleLuaFileSent(UINT8 node);
void RemoveLuaFileTransfer(void);
void RemoveAllLuaFileTransfers(void);
void SV_AbortLuaFileTransfer(INT32 node);
void CL_PrepareDownloadLuaFile(void);
void Got_LuaFile(UINT8 **cp, INT32 playernum);
void StoreLuaFileCallback(INT32 id);
void RemoveLuaFileCallback(INT32 id);
void MakePathDirs(char *path);

void SV_AbortSendFiles(INT32 node);
void CloseNetFile(void);
void CL_AbortDownloadResume(void);

void Command_Downloads_f(void);

boolean fileexist(char *filename, time_t ptime);


filestatus_t findfile(char *filename, const UINT8 *wantedmd5sum,
	boolean completepath);
filestatus_t checkfilemd5(char *filename, const UINT8 *wantedmd5sum);
                                                                           
                                                         
boolean D_GetCachedFileMD5(const char *filename, UINT8 md5sum[16]);


filestatus_t findfolder(const char *path);

void nameonly(char *s);
size_t nameonlylength(const char *s);

#endif  
