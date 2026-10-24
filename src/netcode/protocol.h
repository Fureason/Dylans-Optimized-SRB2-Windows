// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __PROTOCOL__
#define __PROTOCOL__

#include "d_net.h"
#include "../d_ticcmd.h"
#include "../doomdef.h"

  
                                                  
                                                   
                                                       
                

                                                  
                               
  
#define PACKETVERSION 5







#define BACKUPTICS 1024
#define MAXTEXTCMD 256




typedef enum
{
	PT_NOTHING,        
	PT_SERVERCFG,      


	PT_CLIENTCMD,      
	PT_CLIENTMIS,      
	PT_CLIENT2CMD,     
	PT_CLIENT2MIS,     
	PT_NODEKEEPALIVE,  
	PT_NODEKEEPALIVEMIS,
	PT_SERVERTICS,     
	PT_SERVERREFUSE,   
	PT_SERVERSHUTDOWN,
	PT_CLIENTQUIT,     

	PT_ASKINFO,        
	PT_SERVERINFO,     
	PT_PLAYERINFO,     
	PT_REQUESTFILE,    
	PT_ASKINFOVIAMS,   


	PT_WILLRESENDGAMESTATE,  
	PT_CANRECEIVEGAMESTATE,  
	PT_RECEIVEDGAMESTATE,    

	PT_SENDINGLUAFILE,  
	PT_ASKLUAFILE,      
	PT_HASLUAFILE,      

	PT_BASICKEEPALIVE,  



	PT_CANFAIL,        



	PT_FILEFRAGMENT = PT_CANFAIL,  
	PT_FILEACK,
	PT_FILERECEIVED,

	PT_TEXTCMD,        
	PT_TEXTCMD2,       
	PT_CLIENTJOIN,     

	PT_LOGIN,          

	PT_TELLFILESNEEDED,  
	PT_MOREFILESNEEDED,  

	PT_PING,           
	NUMPACKETTYPE
} packettype_t;

#if defined(_MSC_VER)
#pragma pack(1)
#endif


typedef struct
{
	UINT8 client_tic;
	UINT8 resendfrom;
	INT16 consistancy;
	ticcmd_t cmd;
} ATTRPACK clientcmd_pak;



typedef struct
{
	UINT8 client_tic;
	UINT8 resendfrom;
	INT16 consistancy;
	ticcmd_t cmd, cmd2;
} ATTRPACK client2cmd_pak;

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#endif


typedef struct
{
	tic_t starttic;
	UINT8 numtics;
	UINT8 numslots;  
	ticcmd_t cmds[45];
} ATTRPACK servertics_pak;

typedef struct
{

	UINT8 serverplayer;
	UINT8 totalslotnum;  

	tic_t gametic;
	UINT8 clientnode;
	UINT8 gamestate;

	UINT8 gametype;
	UINT8 modifiedgame;
	UINT8 usedCheats;

	char server_context[8];  
} ATTRPACK serverconfig_pak;

typedef struct
{
	UINT8 fileid;
	UINT32 filesize;
	UINT8 iteration;
	UINT32 position;
	UINT16 size;
	UINT8 data[0];  
} ATTRPACK filetx_pak;

typedef struct
{
	UINT32 start;
	UINT32 acks;
} ATTRPACK fileacksegment_t;

typedef struct
{
	UINT8 fileid;
	UINT8 iteration;
	UINT8 numsegments;
	fileacksegment_t segments[0];
} ATTRPACK fileack_pak;

#ifdef _MSC_VER
#pragma warning(default : 4200)
#endif

#define MAXAPPLICATION 16

typedef struct
{
	UINT8 modversion;
	char application[MAXAPPLICATION];
	UINT8 localplayers;
	UINT8 mode;
	char names[MAXSPLITSCREENPLAYERS][MAXPLAYERNAME];
} ATTRPACK clientconfig_pak;

#define SV_DEDICATED    0x40  
#define SV_LOTSOFADDONS 0x20  

enum {
	REFUSE_JOINS_DISABLED = 1,
	REFUSE_SLOTS_FULL,
	REFUSE_BANNED,
};

#define MAXSERVERNAME 32
#define MAXFILENEEDED 915
#define MAX_MIRROR_LENGTH 256


typedef struct
{
	  
                                                                       
                                                                
                        
   
	UINT8 _255;
	UINT8 packetversion;
	char  application[MAXAPPLICATION];
	UINT8 version;
	UINT8 subversion;
	UINT8 numberofplayer;
	UINT8 maxplayer;
	UINT8 refusereason;  
	char gametypename[24];
	UINT8 modifiedgame;
	UINT8 cheatsenabled;
	UINT8 flags;
	UINT8 fileneedednum;
	tic_t time;
	tic_t leveltime;
	char servername[MAXSERVERNAME];
	char mapname[8];
	char maptitle[33];
	unsigned char mapmd5[16];
	UINT8 actnum;
	UINT8 iszone;
	char httpsource[MAX_MIRROR_LENGTH];
	UINT8 fileneeded[MAXFILENEEDED];  
} ATTRPACK serverinfo_pak;

typedef struct
{
	char reason[255];
} ATTRPACK serverrefuse_pak;

typedef struct
{
	UINT8 version;
	tic_t time;  
} ATTRPACK askinfo_pak;

typedef struct
{
	char clientaddr[22];
	tic_t time;  
} ATTRPACK msaskinfo_pak;


typedef struct
{
	UINT8 num;
	char name[MAXPLAYERNAME+1];
	UINT8 address[4];  
	UINT8 team;
	UINT8 skin;
	UINT8 data;  
	UINT32 score;
	UINT16 timeinserver;  
} ATTRPACK plrinfo_pak;


typedef struct
{
	char name[MAXPLAYERNAME+1];
	UINT8 skin;
	UINT16 color;
	UINT32 pflags;
	UINT32 score;
	UINT8 ctfteam;
} ATTRPACK plrconfig_pak;

typedef struct
{
	INT32 first;
	UINT8 num;
	UINT8 more;
	UINT8 files[MAXFILENEEDED];  
} ATTRPACK filesneededconfig_pak;




typedef struct
{
	UINT32 checksum;
	UINT8 ack;  
	UINT8 ackreturn;  

	UINT8 packettype;
	UINT8 reserved;  
	union
	{
		clientcmd_pak clientpak;
		client2cmd_pak client2pak;
		servertics_pak serverpak;
		serverconfig_pak servercfg;
		UINT8 textcmd[MAXTEXTCMD+1];
		filetx_pak filetxpak;
		fileack_pak fileack;
		UINT8 filereceived;
		clientconfig_pak clientcfg;
		UINT8 md5sum[16];
		serverinfo_pak serverinfo;
		serverrefuse_pak serverrefuse;
		askinfo_pak askinfo;
		msaskinfo_pak msaskinfo;
		plrinfo_pak playerinfo[MAXPLAYERS];
		plrconfig_pak playerconfig[MAXPLAYERS];
		INT32 filesneedednum;
		filesneededconfig_pak filesneededcfg;
		UINT32 pingtable[MAXPLAYERS+1];
	} u;  
} ATTRPACK doomdata_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

#define FILETXHEADER        offsetof(filetx_pak, data)

#define KICK_MSG_GO_AWAY     1
#define KICK_MSG_CON_FAIL    2
#define KICK_MSG_PLAYER_QUIT 3
#define KICK_MSG_TIMEOUT     4
#define KICK_MSG_BANNED      5
#define KICK_MSG_PING_HIGH   6
#define KICK_MSG_CUSTOM_KICK 7
#define KICK_MSG_CUSTOM_BAN  8
#define KICK_MSG_IDLE        9
#define KICK_MSG_KEEP_BODY   0x80

#endif
