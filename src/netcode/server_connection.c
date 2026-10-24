// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "server_connection.h"
#include "i_net.h"
#include "d_clisrv.h"
#include "d_netfil.h"
#include "mserv.h"
#include "net_command.h"
#include "gamestate.h"
#include "../byteptr.h"
#include "../g_game.h"
#include "../g_state.h"
#include "../i_time.h"
#include "../p_setup.h"
#include "../p_tick.h"
#include "../command.h"
#include "../doomstat.h"



tic_t jointimeout = (10*TICRATE);



tic_t joindelay = 0;



char playeraddress[MAXPLAYERS][64];

consvar_t cv_showjoinaddress = CVAR_INIT ("showjoinaddress", "Off", CV_SAVE|CV_NETVAR, CV_OnOff, NULL);

consvar_t cv_allownewplayer = CVAR_INIT ("allowjoin", "On", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, CV_OnOff, NULL);

static CV_PossibleValue_t maxplayers_cons_t[] = {{2, "MIN"}, {32, "MAX"}, {0, NULL}};
consvar_t cv_maxplayers = CVAR_INIT ("maxplayers", "8", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, maxplayers_cons_t, NULL);

static CV_PossibleValue_t joindelay_cons_t[] = {{1, "MIN"}, {3600, "MAX"}, {0, "Off"}, {0, NULL}};
consvar_t cv_joindelay = CVAR_INIT ("joindelay", "10", CV_SAVE|CV_NETVAR, joindelay_cons_t, NULL);

static CV_PossibleValue_t rejointimeout_cons_t[] = {{1, "MIN"}, {60 * FRACUNIT, "MAX"}, {0, "Off"}, {0, NULL}};
consvar_t cv_rejointimeout = CVAR_INIT ("rejointimeout", "2", CV_SAVE|CV_NETVAR|CV_FLOAT, rejointimeout_cons_t, NULL);

                                                                          
                                                                               
                                                                          
                                                                          
                                                                         
                                    
static tic_t assignmentreplaytime[MAXNETNODES];
static tic_t playerconfigrepairnext[MAXPLAYERS];
static UINT8 playerconfigrepairattempts[MAXPLAYERS];
static boolean playerconfigrepairfailed[MAXPLAYERS];

static INT32 FindRejoinerNum(SINT8 node)
{
	char addressbuffer[64];
	const char *nodeaddress;
	const char *strippednodeaddress;


	if (!I_GetNodeAddress)
		return -1;
	nodeaddress = I_GetNodeAddress(node);
	if (!nodeaddress)
		return -1;


	strcpy(addressbuffer, nodeaddress);
	strippednodeaddress = I_NetSplitAddress(addressbuffer, NULL);
	if (!strnicmp(strippednodeaddress, "::ffff:", 7))
		strippednodeaddress += 7;


	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && playeraddress[i][0] && playernode[i] == UINT8_MAX)
		{
			const char *savedaddress = playeraddress[i];
			if (!strnicmp(savedaddress, "::ffff:", 7))
				savedaddress += 7;
			if (!stricmp(savedaddress, strippednodeaddress))
				return i;
		}
	}

	return -1;
}

static INT32 FindRejoinerNumByName(const char *name)
{
	if (!name || !name[0])
		return -1;

	for (INT32 i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && players[i].quittime && !IsPlayerAdmin(i)
		&& playernode[i] == UINT8_MAX
		&& !strcasecmp(player_names[i], name))
			return i;

	return -1;
}

static UINT8
GetRefuseReason (INT32 node)
{
	if (!node || FindRejoinerNum(node) != -1)
		return 0;
	else if (bannednode && bannednode[node])
		return REFUSE_BANNED;
	else if (!cv_allownewplayer.value)
		return REFUSE_JOINS_DISABLED;
	else if (D_NumPlayers() >= cv_maxplayers.value)
		return REFUSE_SLOTS_FULL;
	else
		return 0;
}

static void SV_SendServerInfo(INT32 node, tic_t servertime)
{
	UINT8 *p;

	netbuffer->packettype = PT_SERVERINFO;
	netbuffer->u.serverinfo._255 = 255;
	netbuffer->u.serverinfo.packetversion = PACKETVERSION;
	netbuffer->u.serverinfo.version = VERSION;
	netbuffer->u.serverinfo.subversion = SUBVERSION;
	strncpy(netbuffer->u.serverinfo.application, SRB2APPLICATION,
			sizeof netbuffer->u.serverinfo.application);

	netbuffer->u.serverinfo.time = (tic_t)LONG(servertime);
	netbuffer->u.serverinfo.leveltime = (tic_t)LONG(leveltime);


	netbuffer->u.serverinfo.numberofplayer = (UINT8)D_NumNodes(dedicated);
	netbuffer->u.serverinfo.maxplayer = (UINT8)(cv_maxplayers.value - D_NumBots());

	netbuffer->u.serverinfo.refusereason = GetRefuseReason(node);

	strncpy(netbuffer->u.serverinfo.gametypename, Gametype_Names[gametype],
			sizeof(netbuffer->u.serverinfo.gametypename)-1);
	netbuffer->u.serverinfo.modifiedgame = (UINT8)modifiedgame;
	netbuffer->u.serverinfo.cheatsenabled = CV_CheatsEnabled();
	netbuffer->u.serverinfo.flags = (dedicated ? SV_DEDICATED : 0);
	strncpy(netbuffer->u.serverinfo.servername, cv_servername.string,
		sizeof(netbuffer->u.serverinfo.servername)-1);
	strncpy(netbuffer->u.serverinfo.mapname, G_BuildMapName(gamemap), 7);

	M_Memcpy(netbuffer->u.serverinfo.mapmd5, mapmd5, 16);

	memset(netbuffer->u.serverinfo.maptitle, 0, sizeof netbuffer->u.serverinfo.maptitle);

	memset(netbuffer->u.serverinfo.httpsource, 0, MAX_MIRROR_LENGTH);

	if (mapheaderinfo[gamemap-1] && *mapheaderinfo[gamemap-1]->lvlttl)
	{
		char *read = mapheaderinfo[gamemap-1]->lvlttl, *writ = netbuffer->u.serverinfo.maptitle;
		while (writ < (netbuffer->u.serverinfo.maptitle+32) && *read != '\0')
		{
			if (!(*read & 0x80))
			{
				*writ = toupper(*read);
				writ++;
			}
			read++;
		}
		*writ = '\0';

	}
	else
		strncpy(netbuffer->u.serverinfo.maptitle, "UNKNOWN", 32);

	if (mapheaderinfo[gamemap-1] && !(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
		netbuffer->u.serverinfo.iszone = 1;
	else
		netbuffer->u.serverinfo.iszone = 0;

	if (mapheaderinfo[gamemap-1])
		netbuffer->u.serverinfo.actnum = mapheaderinfo[gamemap-1]->actnum;

	const char *httpurl = cv_httpsource.string;
	                                                                         
                                                                       
                      
	strlcpy(netbuffer->u.serverinfo.httpsource, httpurl,
		sizeof netbuffer->u.serverinfo.httpsource);

	p = PutFileNeeded(0);

	HSendPacket(node, false, 0, p - ((UINT8 *)&netbuffer->u));
}

static void SV_SendPlayerInfo(INT32 node)
{
	netbuffer->packettype = PT_PLAYERINFO;

	for (UINT8 i = 0; i < MAXPLAYERS; i++)
	{
		if (playernode[i] == UINT8_MAX || !netnodes[playernode[i]].ingame)
		{
			netbuffer->u.playerinfo[i].num = 255;  
			continue;
		}

		netbuffer->u.playerinfo[i].num = i;
		memset(netbuffer->u.playerinfo[i].name, 0x00, sizeof(netbuffer->u.playerinfo[i].name));
		memcpy(netbuffer->u.playerinfo[i].name, player_names[i], sizeof(player_names[i]));

		netbuffer->u.playerinfo[i].name[MAXPLAYERNAME] = '\0';



		memset(netbuffer->u.playerinfo[i].address, 0, 4);

		if (G_GametypeHasTeams())
		{
			if (!players[i].ctfteam)
				netbuffer->u.playerinfo[i].team = 255;
			else
				netbuffer->u.playerinfo[i].team = (UINT8)players[i].ctfteam;
		}
		else
		{
			if (players[i].spectator)
				netbuffer->u.playerinfo[i].team = 255;
			else
				netbuffer->u.playerinfo[i].team = 0;
		}

		netbuffer->u.playerinfo[i].score = LONG(players[i].score);
		netbuffer->u.playerinfo[i].timeinserver = SHORT((UINT16)(players[i].jointime / TICRATE));
		netbuffer->u.playerinfo[i].skin = (UINT8)(players[i].skin
#ifdef DEVELOP  
		% 3
#endif
		);


		netbuffer->u.playerinfo[i].data = 0;  

		if (players[i].pflags & PF_TAGIT)
			netbuffer->u.playerinfo[i].data |= 0x20;

		if (players[i].gotflag)
			netbuffer->u.playerinfo[i].data |= 0x40;

		if (players[i].powers[pw_super])
			netbuffer->u.playerinfo[i].data |= 0x80;
	}

	HSendPacket(node, false, 0, sizeof(plrinfo_pak) * MAXPLAYERS);
}

                               
   
                               
                                                    
   
    
static boolean SV_SendServerConfig(INT32 node)
{
	boolean waspacketsent;

	netbuffer->packettype = PT_SERVERCFG;

	netbuffer->u.servercfg.serverplayer = (UINT8)serverplayer;
	netbuffer->u.servercfg.totalslotnum = (UINT8)(doomcom->numslots);
	netbuffer->u.servercfg.gametic = (tic_t)LONG(gametic);
	netbuffer->u.servercfg.clientnode = (UINT8)node;
	netbuffer->u.servercfg.gamestate = (UINT8)gamestate;
	netbuffer->u.servercfg.gametype = (UINT8)gametype;
	netbuffer->u.servercfg.modifiedgame = (UINT8)modifiedgame;
	netbuffer->u.servercfg.usedCheats = (UINT8)usedCheats;

	memcpy(netbuffer->u.servercfg.server_context, server_context, 8);

	{
		const size_t len = sizeof (serverconfig_pak);

#ifdef DEBUGFILE
		if (debugfile)
		{
			fprintf(debugfile, "ServerConfig Packet about to be sent, size of packet:%s to node:%d\n",
				sizeu1(len), node);
		}
#endif

		waspacketsent = HSendPacket(node, true, 0, len);
	}

#ifdef DEBUGFILE
	if (debugfile)
	{
		if (waspacketsent)
		{
			fprintf(debugfile, "ServerConfig Packet was sent\n");
		}
		else
		{
			fprintf(debugfile, "ServerConfig Packet could not be sent right now\n");
		}
	}
#endif

	return waspacketsent;
}


static inline void SV_AddNode(INT32 node)
{
	netnodes[node].tic = gametic;
	netnodes[node].supposedtic = gametic;
	assignmentreplaytime[node] = 0;


	if (node)
		netnodes[node].ingame = true;
}

static void SV_QueuePlayerAssignment(SINT8 node, INT32 playernum,
	boolean splitscreenplayer, const char *name)
{
	UINT8 buf[2 + MAXPLAYERNAME];
	UINT8 *p = buf + 2;

	buf[0] = (UINT8)node;
	buf[1] = (UINT8)playernum;
	if (splitscreenplayer)
		buf[1] |= 0x80;
	WRITESTRINGN(p, name, MAXPLAYERNAME);
	SendNetXCmd(XD_ADDPLAYER, &buf, p - buf);
}

                                                                              
                                                                            
                            
static boolean SV_ReplayPlayerAssignments(SINT8 node)
{
	tic_t now;
	boolean queued = false;
	INT32 playernum;

	if (node <= 0 || node >= MAXNETNODES || !netnodes[node].ingame)
		return false;

	now = I_GetTime();
	if (assignmentreplaytime[node]
	&& now < assignmentreplaytime[node] + 2*TICRATE)
		return false;

	playernum = netnodes[node].player;
	if (playernum >= 0 && playernum < MAXPLAYERS)
	{
		SV_QueuePlayerAssignment(node, playernum, false,
			player_names[playernum]);
		queued = true;
	}

	playernum = netnodes[node].player2;
	if (playernum >= 0 && playernum < MAXPLAYERS)
	{
		SV_QueuePlayerAssignment(node, playernum, true,
			player_names[playernum]);
		queued = true;
	}

	if (queued)
		assignmentreplaytime[node] = now ? now : 1;
	return queued;
}

static void SV_AddPlayer(SINT8 node, const char *name)
{
	INT32 n;
	INT32 newplayernum;
	boolean splitscreenplayer;

	newplayernum = FindRejoinerNum(node);
	if (newplayernum == -1)
	{


		for (newplayernum = dedicated ? 1 : 0; newplayernum < MAXPLAYERS; newplayernum++)
		{
			if (playeringame[newplayernum])
				continue;
			for (n = 0; n < MAXNETNODES; n++)
				if (netnodes[n].player == newplayernum || netnodes[n].player2 == newplayernum)
					break;
			if (n == MAXNETNODES)
				break;
		}
	}



	I_Assert(newplayernum < MAXPLAYERS);

	playernode[newplayernum] = (UINT8)node;
	splitscreenplayer = netnodes[node].numplayers >= 1;
	if (netnodes[node].numplayers < 1)
	{
		netnodes[node].player = newplayernum;
	}
	else
	{
		netnodes[node].player2 = newplayernum;
	}
	netnodes[node].numplayers++;
	playerconfigrepairnext[newplayernum] = 0;
	playerconfigrepairattempts[newplayernum] = 0;
	playerconfigrepairfailed[newplayernum] = false;

	SV_QueuePlayerAssignment(node, newplayernum, splitscreenplayer, name);

	DEBFILE(va("Server added player %d node %d\n", newplayernum, node));
}

void SV_JoinStateTicker(void)
{
	tic_t now;
	INT32 i;

	if (!server || !netgame)
		return;

	now = I_GetTime();
	for (i = 0; i < MAXPLAYERS; i++)
	{
		INT32 node = playernode[i];

		if (!playeringame[i] || players[i].quittime || players[i].bot
		|| node <= 0 || node >= MAXNETNODES || !netnodes[node].ingame)
		{
			playerconfigrepairnext[i] = 0;
			playerconfigrepairattempts[i] = 0;
			playerconfigrepairfailed[i] = false;
			continue;
		}

		                                                                     
                                                                      
                                                                         
		if (players[i].skincolor != SKINCOLOR_NONE
		&& players[i].skincolor < numskincolors)
		{
			if (playerconfigrepairattempts[i])
			{
				D_WriteJoinStateReport("server-player-config-recovered", i,
					(SINT8)node, playerconfigrepairattempts[i]);
				CONS_Printf("Recovered incomplete join state for %s (player %d).\n",
					player_names[i], i);
			}
			playerconfigrepairnext[i] = 0;
			playerconfigrepairattempts[i] = 0;
			playerconfigrepairfailed[i] = false;
			continue;
		}

		 
		if (players[i].jointime < 5*TICRATE)
			continue;
		if (playerconfigrepairnext[i] && now < playerconfigrepairnext[i])
			continue;

		if (playerconfigrepairattempts[i] < 2)
		{
			if (!playerconfigrepairattempts[i])
				D_WriteJoinStateReport("server-missing-player-config", i,
					(SINT8)node, 0);

			if (SV_ReplayPlayerAssignments((SINT8)node))
			{
				playerconfigrepairattempts[i]++;
				playerconfigrepairnext[i] = now + 3*TICRATE;
				CONS_Alert(CONS_WARNING,
					"Repairing incomplete join state for %s (player %d, attempt %u/2).\n",
					player_names[i], i,
					(unsigned)playerconfigrepairattempts[i]);
			}
			else
				playerconfigrepairnext[i] = now + TICRATE;
			continue;
		}

		if (!playerconfigrepairfailed[i])
		{
			playerconfigrepairfailed[i] = true;
			D_WriteJoinStateReport("server-player-config-unrecovered", i,
				(SINT8)node, playerconfigrepairattempts[i]);
			CONS_Alert(CONS_WARNING,
				"Player %s still has incomplete join state; details were saved in diagnostics/join-state-incidents-v1.txt.\n",
				player_names[i]);
		}
	}
}

static void SV_SendRefuse(INT32 node, const char *reason)
{
	strcpy(netbuffer->u.serverrefuse.reason, reason);

	netbuffer->packettype = PT_SERVERREFUSE;
	HSendPacket(node, true, 0, strlen(netbuffer->u.serverrefuse.reason) + 1);
	Net_CloseConnection(node);
}

static const char *
GetRefuseMessage (SINT8 node, INT32 rejoinernum)
{
	clientconfig_pak *cc = &netbuffer->u.clientcfg;

	boolean rejoining = (rejoinernum != -1);

	if (!node) 
		return NULL;

	if (
			cc->modversion != MODVERSION ||
			strncmp(cc->application, SRB2APPLICATION,
				sizeof cc->application)
	){
		return 
			"Incompatible.";
	}
	else if (bannednode && bannednode[node])
	{
		return
			"You have been banned\n"
			"from the server.";
	}
	else if (cc->localplayers != 1)
	{
		return
			"Wrong player count.";
	}

	if (!rejoining)
	{
		if (!cv_allownewplayer.value)
		{
			return
				"The server is not accepting\n"
				"joins for the moment.";
		}
		else if (D_NumPlayers() >= cv_maxplayers.value)
		{
			return va(
					"Maximum players reached: %d",
					cv_maxplayers.value);
		}
	}

	if (luafiletransfers)
	{
		return
			"The serveris broadcasting a file\n"
			"requested by a Lua script.\n"
			"Please wait a bit and then\n"
			"try rejoining.";
	}

	if (netgame)
	{
		const tic_t th = 2 * cv_joindelay.value * TICRATE;

		if (joindelay > th)
		{
			return va(
					"Too many people are connecting.\n"
					"Please wait %d seconds and then\n"
					"try rejoining.",
					(joindelay - th) / TICRATE);
		}
	}

	return NULL;
}

                                                  
   
                                 
   
    
void PT_ClientJoin(SINT8 node)
{
	char names[MAXSPLITSCREENPLAYERS][MAXPLAYERNAME + 1];
	INT32 numplayers = netbuffer->u.clientcfg.localplayers;
	INT32 rejoinernum;

	if (client)
		return;







	if (netnodes[node].ingame)
	{
		const boolean hasplayerslot = (netnodes[node].player >= 0
			|| netnodes[node].player2 >= 0);

		                                                                       
                                                                            
                                                                       
                                               
		if (!hasplayerslot)
		{
			DEBFILE(va("discarding half-connected joining node %d\n", node));
			Net_ForceCloseConnection(node);
			ResetNode(node);
			return;
		}

		                                                                          
                                                                         
                                                                           
                                
		if (Net_HasPendingReliablePacket(node, PT_SERVERCFG))
		{
			DEBFILE(va("server config still pending for joining node %d\n", node));
		}
		else if (SV_SendServerConfig(node))
		{
			DEBFILE(va("resent server config to joining node %d\n", node));
			                                                               
                                                                        
			if (SV_ReplayPlayerAssignments(node))
				DEBFILE(va("replayed player assignment for joining node %d\n", node));
		}
		return;
	}

	if (numplayers < 1 || numplayers > MAXSPLITSCREENPLAYERS)
	{
		SV_SendRefuse(node, "Wrong player count.");
		return;
	}

	for (INT32 i = 0; i < numplayers; i++)
		strlcpy(names[i], netbuffer->u.clientcfg.names[i], MAXPLAYERNAME + 1);

	rejoinernum = FindRejoinerNum(node);
	                                                                       
                                                                         
                                                                            
	if (rejoinernum == -1 && numplayers == 1)
		rejoinernum = FindRejoinerNumByName(names[0]);

	const char *refuse = GetRefuseMessage(node, rejoinernum);
	if (refuse)
	{
		SV_SendRefuse(node, refuse);
		return;
	}

	for (INT32 i = 0; i < numplayers; i++)
	{
		if (!EnsurePlayerNameIsGood(names[i], rejoinernum))
		{
			SV_SendRefuse(node, "Bad player name");
			return;
		}
	}

	SV_AddNode(node);

	if (!SV_SendServerConfig(node))
	{




		ResetNode(node);
		DEBFILE(va("server config busy for node %d; waiting for join retry\n", node));
		return;
	}
	DEBFILE("new node joined\n");

	if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION)
	{
		SV_SendSaveGame(node, false);  
		DEBFILE("send savegame\n");
	}


	for (INT32 i = 0; i < numplayers; i++)
		SV_AddPlayer(node, names[i]);

	joindelay += cv_joindelay.value * TICRATE;
}

void PT_AskInfoViaMS(SINT8 node)
{
	Net_CloseConnection(node);
}

void PT_TellFilesNeeded(SINT8 node)
{
	if (server && serverrunning)
	{
		UINT8 *p;
		INT32 firstfile = netbuffer->u.filesneedednum;

		netbuffer->packettype = PT_MOREFILESNEEDED;
		netbuffer->u.filesneededcfg.first = firstfile;
		netbuffer->u.filesneededcfg.more = 0;

		p = PutFileNeeded(firstfile);

		HSendPacket(node, false, 0, p - ((UINT8 *)&netbuffer->u));
	}
	else  
		Net_CloseConnection(node);
}

void PT_AskInfo(SINT8 node)
{
	if (server && serverrunning)
	{
		SV_SendServerInfo(node, (tic_t)LONG(netbuffer->u.askinfo.time));
		SV_SendPlayerInfo(node);  
	}
	Net_CloseConnection(node);
}
