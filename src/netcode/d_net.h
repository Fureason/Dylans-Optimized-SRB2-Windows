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







#ifndef __D_NET__
#define __D_NET__

#include "../doomtype.h"




#define MAXNETNODES 127
#define BROADCASTADDR MAXNETNODES
#define MAXSPLITSCREENPLAYERS 2  


#define STATLENGTH (TICRATE*2)


extern INT32 ticruned, ticmiss;
extern INT32 getbps, sendbps;
extern float lostpercent, duppercent, gamelostpercent;
extern INT32 packetheaderlength;
boolean Net_GetNetStat(void);
extern INT32 getbytes;
extern INT64 sendbytes;  

typedef struct netnode_s
{
	boolean ingame;  
	tic_t freezetimeout;  

	SINT8 player;
	SINT8 player2;  
	UINT8 numplayers;  

	tic_t tic;  
	tic_t supposedtic;  

	boolean sendingsavegame;  
	boolean resendingsavegame;  
	tic_t savegameresendcooldown;  
} netnode_t;

extern netnode_t netnodes[MAXNETNODES];

extern boolean serverrunning;

void Net_AckTicker(void);
 
void Net_RefreshTimeouts(void);
 
UINT16 Net_GetReliableBacklog(INT32 node, UINT16 *max_retries);
 
boolean Net_HasPendingReliablePacket(INT32 node, UINT8 packettype);
 
UINT16 Net_GetWorstReliableBacklog(INT32 *node, UINT16 *max_retries);


boolean HSendPacket(INT32 node, boolean reliable, UINT8 acknum,
	size_t packetlength);
boolean HGetPacket(void);
void D_SetDoomcom(void);
boolean D_CheckNetGame(void);
void D_CloseConnection(void);
boolean Net_IsNodeIPv6(INT32 node);
void Net_UnAcknowledgePacket(INT32 node);
void Net_CloseConnection(INT32 node);
void Net_ForceCloseConnection(INT32 node);
void Net_ConnectionTimeout(INT32 node);
void Net_AbortPacketType(UINT8 packettype);
void Net_WaitAllAckReceived(UINT32 timeout);

#endif
