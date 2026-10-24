// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "tic_command.h"
#include "d_clisrv.h"
#include "d_netfil.h"
#include "net_command.h"
#include "client_connection.h"
#include "gamestate.h"
#include "i_net.h"
#include "../d_main.h"
#include "../g_game.h"
#include "../i_system.h"
#include "../i_time.h"
#include "../byteptr.h"
#include "../doomstat.h"
#include "../doomtype.h"

tic_t firstticstosend;  
tic_t tictoclear = 0;  
ticcmd_t localcmds;
ticcmd_t localcmds2;
boolean cl_packetmissed;
ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];

typedef struct
{
	tic_t lasttic;
	tic_t lastfailuretime;
	UINT8 samples;
	UINT8 resyncs;
} consistency_mismatch_tracker_t;

                                                                           
                                                                            
                                                                 
static consistency_mismatch_tracker_t consistency_mismatches[MAXNETNODES];
#define CONSISTENCY_MISMATCH_CONFIRMATIONS 2
#define CONSISTENCY_ATTEMPT_RESET_TIME (5*60*TICRATE)

typedef struct
{
	INT16 delta;
	tic_t lasttime;
	UINT8 nodes;
	boolean seen[MAXNETNODES];
} consistency_cluster_t;

                                                                            
                                                                             
                                                                       
static consistency_cluster_t consistency_cluster;

                                                                           
                                                                             
                                                                              
static SINT8 NearbyConsistencyMatch(tic_t tic, INT16 value)
{
	INT32 delta;
	for (delta = -2; delta <= 2; delta++)
	{
		tic_t candidate;
		if (!delta || (delta < 0 && tic < (tic_t)-delta))
			continue;
		candidate = (tic_t)((INT64)tic + delta);
		if (candidate > gametic || candidate + BACKUPTICS - 1 <= gametic)
			continue;
		if (consistancy[candidate % BACKUPTICS] == value)
			return (SINT8)delta;
	}
	return 0;
}

static inline void *G_DcpyTiccmd(void* dest, const ticcmd_t* src, const size_t n)
{
	const size_t d = n / sizeof(ticcmd_t);
	const size_t r = n % sizeof(ticcmd_t);
	UINT8 *ret = dest;

	if (r)
		M_Memcpy(dest, src, n);
	else if (d)
		G_MoveTiccmd(dest, src, d);
	return ret+n;
}

static inline void *G_ScpyTiccmd(ticcmd_t* dest, void* src, const size_t n)
{
	const size_t d = n / sizeof(ticcmd_t);
	const size_t r = n % sizeof(ticcmd_t);
	UINT8 *ret = src;

	if (r)
		M_Memcpy(dest, src, n);
	else if (d)
		G_MoveTiccmd(dest, src, d);
	return ret+n;
}

                                                                             
   
                                               
                                              
                              
   
    
tic_t ExpandTics(INT32 low, INT32 node)
{
	INT32 delta;

	delta = low - (netnodes[node].tic & UINT8_MAX);

	if (delta >= -64 && delta <= 64)
		return (netnodes[node].tic & ~UINT8_MAX) + low;
	else if (delta > 64)
		return (netnodes[node].tic & ~UINT8_MAX) - 256 + low;
	else  
		return (netnodes[node].tic & ~UINT8_MAX) + 256 + low;
}

void D_Clearticcmd(tic_t tic)
{
	D_FreeTextcmd(tic);

	for (INT32 i = 0; i < MAXPLAYERS; i++)
		netcmds[tic%BACKUPTICS][i].angleturn = 0;

	DEBFILE(va("clear tic %5u (%2u)\n", tic, tic%BACKUPTICS));
}

void D_ResetTiccmds(void)
{
	memset(&localcmds, 0, sizeof(ticcmd_t));
	memset(&localcmds2, 0, sizeof(ticcmd_t));
	memset(consistency_mismatches, 0, sizeof(consistency_mismatches));
	memset(&consistency_cluster, 0, sizeof(consistency_cluster));


	for (INT32 i = 0; i < TEXTCMD_HASH_SIZE; i++)
		while (textcmds[i])
			D_Clearticcmd(textcmds[i]->tic);
}

void D_ResetConsistencyTracker(INT32 node)
{
	if (node >= 0 && node < MAXNETNODES)
		memset(&consistency_mismatches[node], 0,
			sizeof(consistency_mismatches[node]));
}


static void CheckTiccmdHacks(INT32 playernum, const ticcmd_t *cmd)
{
	if (cmd->forwardmove > MAXPLMOVE || cmd->forwardmove < -MAXPLMOVE
		|| cmd->sidemove > MAXPLMOVE || cmd->sidemove < -MAXPLMOVE)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal movement value received from node %d\n"), playernum);
		SendKick(playernum, KICK_MSG_CON_FAIL);
	}
}


static void CheckConsistancy(SINT8 nodenum, tic_t tic)
{
	netnode_t *node = &netnodes[nodenum];
	consistency_mismatch_tracker_t *tracker = &consistency_mismatches[nodenum];
	INT16 neededconsistancy = consistancy[tic%BACKUPTICS];
	INT16 clientconsistancy = SHORT(netbuffer->u.clientpak.consistancy);
	SINT8 nearbydelta;
	UINT8 confirmations;

	if (tic > gametic || tic + BACKUPTICS - 1 <= gametic || gamestate != GS_LEVEL
		|| SV_ResendingSavegameToAnyone() || node->resendingsavegame
		                                                                      
                                                                         
                                                                         
                                                                            
                                                                           
                                                          
		|| luafiletransfers
		|| node->savegameresendcooldown > I_GetTime())
	{
		tracker->samples = 0;
		return;
	}

	if (neededconsistancy == clientconsistancy)
	{
		tracker->samples = 0;
		                                                                   
                                                                      
                                                                           
                                                                         
		if (tracker->resyncs && tracker->lastfailuretime
			&& I_GetTime() - tracker->lastfailuretime >= CONSISTENCY_ATTEMPT_RESET_TIME)
			tracker->resyncs = 0;
		return;
	}

	if (!tracker->samples || tic <= tracker->lasttic
		|| tic - tracker->lasttic > TICRATE)
		tracker->samples = 1;
	else if (tracker->samples < UINT8_MAX)
		tracker->samples++;
	tracker->lasttic = tic;
	tracker->lastfailuretime = I_GetTime();
	nearbydelta = NearbyConsistencyMatch(tic, clientconsistancy);
	confirmations = nearbydelta ? 3 : CONSISTENCY_MISMATCH_CONFIRMATIONS;

	if (tracker->samples < confirmations)
	{
		if (nearbydelta)
		{
			DEBFILE(va("provisional consistency mismatch node %d [%u] %d!=%d "
				"(matches server tic %+d)\n", nodenum, tic, neededconsistancy,
				clientconsistancy, nearbydelta));
		}
		else
		{
			DEBFILE(va("provisional consistency mismatch node %d [%u] %d!=%d\n",
				nodenum, tic, neededconsistancy, clientconsistancy));
		}
		return;
	}

	D_WriteConsistencyReport("server-mismatch", tic, nodenum,
		neededconsistancy, clientconsistancy);
	tracker->samples = 0;
	{
		const tic_t now = I_GetTime();
		const INT16 delta = (INT16)(neededconsistancy - clientconsistancy);

		if (!consistency_cluster.nodes
		|| consistency_cluster.delta != delta
		|| now - consistency_cluster.lasttime > 5*TICRATE)
		{
			memset(&consistency_cluster, 0, sizeof(consistency_cluster));
			consistency_cluster.delta = delta;
		}
		consistency_cluster.lasttime = now;
		if (!consistency_cluster.seen[nodenum])
		{
			consistency_cluster.seen[nodenum] = true;
			if (consistency_cluster.nodes < UINT8_MAX)
				consistency_cluster.nodes++;
		}
		if (consistency_cluster.nodes == 3)
		{
			CONS_Alert(CONS_WARNING, M_GetText(
				"Three clients reported the same consistency offset (%hd). "
				"They agree with one another, so check host-only Lua or a "
				"custom-host gameplay difference rather than packet loss.\n"), delta);
		}
	}

	if (cv_resynchattempts.value
		&& tracker->resyncs < (UINT8)cv_resynchattempts.value)
	{
		tracker->resyncs++;
		CONS_Alert(CONS_WARNING, M_GetText(
			"Resyncing player %d (%s), recovery attempt %u of %d "
			"(tic %u: expected %hd, got %hd).\n"),
			node->player+1, player_names[node->player], tracker->resyncs,
			cv_resynchattempts.value, (unsigned)tic,
			neededconsistancy, clientconsistancy);


		netbuffer->packettype = PT_WILLRESENDGAMESTATE;
		HSendPacket(nodenum, true, 0, 0);

		node->resendingsavegame = true;

		DEBFILE(va("Restoring player %d (synch failure) [%update] %d!=%d\n",
			node->player, tic, neededconsistancy, clientconsistancy));
	}
	else
	{
		if (cv_resynchattempts.value)
			CONS_Alert(CONS_WARNING, M_GetText(
				"Player %d (%s) kept diverging after %u recovery attempt%s; "
				"stopping the resync loop.\n"), node->player+1,
				player_names[node->player], tracker->resyncs,
				tracker->resyncs == 1 ? "" : "s");
		else
			CONS_Alert(CONS_WARNING, M_GetText(
				"Player %d (%s) diverged and automatic resync is disabled.\n"),
				node->player+1, player_names[node->player]);
		SendKick(node->player, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);

		DEBFILE(va("player %d kicked (synch failure) [%u] %d!=%d\n",
			node->player, tic, neededconsistancy, clientconsistancy));
	}
}

void PT_ClientCmd(SINT8 nodenum, INT32 netconsole)
{
	netnode_t *node = &netnodes[nodenum];
	tic_t realend, realstart;

	if (client)
		return;



	realstart = ExpandTics(netbuffer->u.clientpak.client_tic, nodenum);
	realend = ExpandTics(netbuffer->u.clientpak.resendfrom, nodenum);

	if (netbuffer->packettype == PT_CLIENTMIS || netbuffer->packettype == PT_CLIENT2MIS
		|| netbuffer->packettype == PT_NODEKEEPALIVEMIS
		|| node->supposedtic < realend)
	{
		node->supposedtic = realend;
	}

	if (node->tic > realend)
	{
		DEBFILE(va("out of order ticcmd discarded nettics = %u\n", node->tic));
		return;
	}


	node->tic = realend;


	if (netconsole == -1)
		return;



	node->freezetimeout = I_GetTime() + connectiontimeout;



	if (netbuffer->packettype == PT_NODEKEEPALIVE
		|| netbuffer->packettype == PT_NODEKEEPALIVEMIS)
		return;

	                                                                        
                                                                           
                                                                       
                                                                             
                                                                          
                                                          
	tic_t faketic = maketic;
	if ((netcmds[maketic % BACKUPTICS][netconsole].angleturn & TICCMD_RECEIVED)
	&& maketic - firstticstosend < BACKUPTICS - 1)
		faketic++;

	G_MoveTiccmd(&netcmds[faketic%BACKUPTICS][netconsole],
		&netbuffer->u.clientpak.cmd, 1);


	if ((netbuffer->packettype == PT_CLIENT2CMD || netbuffer->packettype == PT_CLIENT2MIS)
		&& node->player2 >= 0)
		G_MoveTiccmd(&netcmds[faketic%BACKUPTICS][(UINT8)node->player2],
			&netbuffer->u.client2pak.cmd2, 1);

	CheckTiccmdHacks(netconsole, &netcmds[faketic%BACKUPTICS][netconsole]);
	if ((netbuffer->packettype == PT_CLIENT2CMD || netbuffer->packettype == PT_CLIENT2MIS)
		&& node->player2 >= 0)
		CheckTiccmdHacks((UINT8)node->player2,
			&netcmds[faketic%BACKUPTICS][(UINT8)node->player2]);
	CheckConsistancy(nodenum, realstart);
}

void PT_ServerTics(SINT8 node, INT32 netconsole)
{
	tic_t realend, realstart;
	servertics_pak *packet = &netbuffer->u.serverpak;

	if (!netnodes[node].ingame)
	{

		if (node != servernode)
		{
			DEBFILE(va("unknown packet received (%d) from unknown host\n",netbuffer->packettype));
			Net_CloseConnection(node);
		}
		return;
	}


	if (node != servernode)
	{
		CONS_Alert(CONS_WARNING, M_GetText("%s received from non-host %d\n"), "PT_SERVERTICS", node);
		if (server)
			SendKick(netconsole, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	realstart = packet->starttic;
	realend = realstart + packet->numtics;

	realend = min(realend, gametic + CLIENTBACKUPTICS);
	cl_packetmissed = realstart > neededtic;

	if (realstart <= neededtic && realend > neededtic)
	{
		UINT8 *pak = (UINT8 *)&packet->cmds;
		UINT8 *txtpak = (UINT8 *)&packet->cmds[packet->numslots * packet->numtics];

		for (tic_t i = realstart; i < realend; i++)
		{

			D_Clearticcmd(i);


			pak = G_ScpyTiccmd(netcmds[i%BACKUPTICS], pak,
				packet->numslots*sizeof (ticcmd_t));

			CL_CopyNetCommandsFromServerPacket(i, &txtpak);
		}

		neededtic = realend;
	}
	else
	{
		DEBFILE(va("frame not in bound: %u\n", neededtic));
	}
}


void CL_SendClientCmd(void)
{
	size_t packetsize = 0;
	boolean mis = false;

	netbuffer->packettype = PT_CLIENTCMD;

	if (cl_packetmissed)
	{
		netbuffer->packettype = PT_CLIENTMIS;
		mis = true;
	}

	netbuffer->u.clientpak.resendfrom = (UINT8)(neededtic & UINT8_MAX);
	netbuffer->u.clientpak.client_tic = (UINT8)(gametic & UINT8_MAX);

	if (gamestate == GS_WAITINGPLAYERS)
	{

		netbuffer->packettype = (mis ? PT_NODEKEEPALIVEMIS : PT_NODEKEEPALIVE);
		packetsize = sizeof (clientcmd_pak) - sizeof (ticcmd_t) - sizeof (INT16);
		HSendPacket(servernode, false, 0, packetsize);
	}
	else if (gamestate != GS_NULL && (addedtogame || dedicated))
	{
		packetsize = sizeof (clientcmd_pak);
		G_MoveTiccmd(&netbuffer->u.clientpak.cmd, &localcmds, 1);
		netbuffer->u.clientpak.consistancy = SHORT(consistancy[gametic%BACKUPTICS]);


		if (splitscreen || botingame)
		{
			netbuffer->packettype = (mis ? PT_CLIENT2MIS : PT_CLIENT2CMD);
			packetsize = sizeof (client2cmd_pak);
			G_MoveTiccmd(&netbuffer->u.client2pak.cmd2, &localcmds2, 1);
		}

		HSendPacket(servernode, false, 0, packetsize);
	}

	if (cl_mode == CL_CONNECTED || dedicated)
		CL_SendNetCommands();
}




static tic_t SV_CalculateNumTicsForPacket(SINT8 nodenum, tic_t firsttic, tic_t lasttic)
{
	size_t size = BASESERVERTICSSIZE;

	for (tic_t tic = firsttic; tic < lasttic; tic++)
	{
		size += sizeof (ticcmd_t) * doomcom->numslots;
		size += TotalTextCmdPerTic(tic);

		if (size > software_MAXPACKETLENGTH)
		{
			DEBFILE(va("packet too large (%s) at tic %d (should be from %d to %d)\n",
				sizeu1(size), tic, firsttic, lasttic));
			lasttic = tic;




			if (lasttic == firsttic)
			{
				if (size > MAXPACKETLENGTH)
					I_Error("Too many players: can't send %s data for %d players to node %d\n"
							"Well sorry nobody is perfect....\n",
							sizeu1(size), doomcom->numslots, nodenum);
				else
				{
					lasttic++;  
					DEBFILE("sending it anyway\n");
				}
			}
			break;
		}
	}

	return lasttic - firsttic;
}



void SV_SendTics(void)
{
	tic_t realfirsttic, lasttictosend;




	for (INT32 n = 1; n < MAXNETNODES; n++)
		if (netnodes[n].ingame)
		{
			netnode_t *node = &netnodes[n];


			realfirsttic = node->supposedtic;
			lasttictosend = min(maketic, node->tic + CLIENTBACKUPTICS);

			if (realfirsttic >= lasttictosend)
			{




				DEBFILE(va("Nothing to send node %u mak=%u sup=%u net=%u \n",
					n, maketic, node->supposedtic, node->tic));

				realfirsttic = node->tic;

				if (realfirsttic >= lasttictosend || (I_GetTime() + n)&3)

					continue;

				DEBFILE(va("Sent %d anyway\n", realfirsttic));
			}
			realfirsttic = max(realfirsttic, firstticstosend);

			lasttictosend = realfirsttic + SV_CalculateNumTicsForPacket(n, realfirsttic, lasttictosend);


			netbuffer->packettype = PT_SERVERTICS;
			netbuffer->u.serverpak.starttic = realfirsttic;
			netbuffer->u.serverpak.numtics = (UINT8)(lasttictosend - realfirsttic);
			netbuffer->u.serverpak.numslots = (UINT8)SHORT(doomcom->numslots);


			UINT8 *bufpos = (UINT8 *)&netbuffer->u.serverpak.cmds;
			for (tic_t i = realfirsttic; i < lasttictosend; i++)
				bufpos = G_DcpyTiccmd(bufpos, netcmds[i%BACKUPTICS], doomcom->numslots * sizeof (ticcmd_t));
			for (tic_t i = realfirsttic; i < lasttictosend; i++)
				SV_WriteNetCommandsForTic(i, &bufpos);
			size_t packsize = bufpos - (UINT8 *)&(netbuffer->u);
			HSendPacket(n, false, 0, packsize);


			if (lasttictosend-doomcom->extratics > realfirsttic)
				node->supposedtic = lasttictosend-doomcom->extratics;
			else
				node->supposedtic = lasttictosend;
			node->supposedtic = max(node->supposedtic, node->tic);
		}


	netnodes[0].supposedtic = maketic;
}

void Local_Maketic(INT32 realtics)
{
	I_OsPolling();  
	D_ProcessEvents();  


	if (!dedicated)
		rendergametic = gametic;

	G_BuildTiccmd(&localcmds, realtics, 1);
	if (splitscreen || botingame)
		G_BuildTiccmd(&localcmds2, realtics, 2);

	localcmds.angleturn |= TICCMD_RECEIVED;
	localcmds2.angleturn |= TICCMD_RECEIVED;
}


void SV_Maketic(void)
{
	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;


		if ((netcmds[maketic % BACKUPTICS][i].angleturn & TICCMD_RECEIVED) == 0)
		{
			ticcmd_t *    ticcmd = &netcmds[(maketic    ) % BACKUPTICS][i];
			ticcmd_t *prevticcmd = &netcmds[(maketic - 1) % BACKUPTICS][i];

			if (players[i].quittime)
			{


				memset(ticcmd, 0, sizeof(netcmds[0][0]));
				ticcmd->angleturn = prevticcmd->angleturn | TICCMD_RECEIVED;
				ticcmd->aiming = prevticcmd->aiming;
			}
			else
			{
				DEBFILE(va("MISS tic%4d for player %d\n", maketic, i));

				*ticcmd = *prevticcmd;
				ticcmd->angleturn &= ~TICCMD_RECEIVED;
			}
		}
	}


	maketic++;
}
