
#ifndef __SERVER_BROWSER_H__
#define __SERVER_BROWSER_H__

#include "client_connection.h"
#include "../w_wad.h"

#define SB_ADDRESSLEN 96
typedef struct
{
	char address[SB_ADDRESSLEN];
	char displayname[MAXSERVERNAME];
	serverinfo_pak info;
	boolean favorite, known, listed;
	char problem[40];
	tic_t seen, queried, checkuntil;
	size_t filebytes;
} sb_server_t;

typedef struct
{
	char name[MAX_WADPATH];
	UINT32 size;
	UINT8 md5[16], download;
	boolean folder;
} sb_addon_t;

typedef struct
{
	char name[MAXPLAYERNAME+1];
	boolean spectator;
} sb_player_t;

void SB_Init(void);
void SB_Open(void);
void SB_Close(void);
void SB_Refresh(void);
void SB_Ticker(void);
boolean SB_Active(void);
size_t SB_Count(void);
UINT32 SB_Revision(void);
sb_server_t *SB_Server(size_t index);
boolean SB_Online(const sb_server_t *entry);
const char *SB_Status(const sb_server_t *entry);
const char *SB_DisplayName(const sb_server_t *entry);
boolean SB_ValidAddress(const char *address);
boolean SB_Lookup(const char *address);
INT32 SB_LookupIndex(void);
boolean SB_Favorite(size_t index);
INT32 SB_AddFavorite(const char *address);
void SB_Unavailable(SINT8 node, const char *reason);
void SB_Info(SINT8 node, const serverinfo_pak *info, size_t filebytes);
boolean SB_MoreFiles(SINT8 node);
void SB_Select(INT32 index);
size_t SB_AddonCount(void);
const sb_addon_t *SB_Addon(size_t index);
boolean SB_AddonsComplete(void);
boolean SB_AddonsFailed(void);
boolean SB_NeedsCleanSession(void);
void SB_ShowPlayers(boolean show);
void SB_PlayerInfo(SINT8 node);
size_t SB_PlayerCount(void);
const sb_player_t *SB_Player(size_t index);
const char *SB_PlayerStatus(void);

#endif
