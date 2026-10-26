// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_clisrv.c
/// \brief SRB2 Network game communication and protocol, all OS independent parts.

#include <time.h>
#ifdef __GNUC__
#include <unistd.h> //for unlink
#endif

#include "../i_time.h"
#include "i_net.h"
#include "../i_system.h"
#include "../i_video.h"
#include "d_net.h"
#include "../d_main.h"
#include "../g_game.h"
#include "../st_stuff.h"
#include "../hu_stuff.h"
#include "../keys.h"
#include "../m_menu.h"
#include "../console.h"
#include "d_netfil.h"
#include "../byteptr.h"
#include "../p_saveg.h"
#include "../z_zone.h"
#include "../p_local.h"
#include "../m_misc.h"
#include "../am_map.h"
#include "../m_random.h"
#include "mserv.h"
#include "../y_inter.h"
#include "../r_local.h"
#include "../r_skins.h"
#include "../m_argv.h"
#include "../p_setup.h"
#include "../lzf.h"
#include "../lua_script.h"
#include "../lua_hook.h"
#include "../lua_libs.h"
#include "../md5.h"
#include "../m_perfstats.h"
#include "../w_wad.h"
#include "server_connection.h"
#include "client_connection.h"
#include "tic_command.h"
#include "net_command.h"
#include "gamestate.h"
#include "commands.h"
#include "protocol.h"

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// Server:
//   maketic is the tic that hasn't had control made for it yet
//   nettics is the tic for each node
//   firstticstosend is the lowest value of nettics
// Client:
//   neededtic is the tic needed by the client to run the game
//   firstticstosend is used to optimize a condition
// Normally maketic >= gametic > 0

boolean server = true; // true or false but !server == client
boolean serverrunning = false;
INT32 serverplayer = 0;
char motd[254], server_context[8]; // Message of the Day, Unique Context (even without Mumble support)

netnode_t netnodes[MAXNETNODES];

// Server specific vars
UINT8 playernode[MAXPLAYERS];

UINT16 pingmeasurecount = 1;
UINT32 realpingtable[MAXPLAYERS]; //the base table of ping where an average will be sent to everyone.
UINT32 playerpingtable[MAXPLAYERS]; //table of player latency values.
static INT32 pingtimeout[MAXPLAYERS];
tic_t servermaxping = 800; // server's max ping. Defaults to 800

tic_t maketic;

INT16 consistancy[BACKUPTICS];

typedef struct
{
	boolean valid;
	tic_t tic;
	tic_t leveltic;
	UINT16 aggregate;
	UINT32 playerfold;
	UINT32 rngseed;
	boolean usesrng;
	struct
	{
		boolean ingame;
		boolean hasmobj;
		fixed_t x, y, z;
		fixed_t momx, momy, momz;
		INT32 shield;
		INT32 pflags;
		INT32 rings;
		UINT8 skin;
		UINT8 playerstate;
		UINT8 charability;
		UINT32 fold_after;
	} player[MAXPLAYERS];
} consistency_snapshot_t;

static consistency_snapshot_t consistency_snapshots[BACKUPTICS];
static UINT32 consistency_report_serial;
static UINT32 joinstate_report_serial;

// true when a player is connecting or disconnecting so that the gameplay has stopped in its tracks
boolean hu_stopped = false;

UINT8 (*adminpassmd5)[16];
UINT32 adminpasscount = 0;

tic_t neededtic;
SINT8 servernode = 0; // the number of the server node

boolean acceptnewnode = true;

UINT16 software_MAXPACKETLENGTH;

static tic_t gametime = 0;

static CV_PossibleValue_t netticbuffer_cons_t[] = {{0, "MIN"}, {3, "MAX"}, {0, NULL}};
consvar_t cv_netticbuffer = CVAR_INIT ("netticbuffer", "1", CV_SAVE, netticbuffer_cons_t, NULL);

static CV_PossibleValue_t netlowlatency_cons_t[] = {
	{0, "Off"}, {1, "On"}, {2, "Adaptive"}, {3, "Maximum"}, {0, NULL}};
consvar_t cv_netlowlatency = CVAR_INIT ("netlowlatency", "Adaptive", CV_SAVE,
	netlowlatency_cons_t, NULL);

static tic_t netlatency_last_stall;
static tic_t netlatency_recovery_until;
static tic_t netlatency_stable_since;
static UINT32 netlatency_stalls;
static UINT32 netlatency_recoveries;
static boolean hosthealth_watch;
static tic_t hosthealth_next_report;

#define JOINSTATE_REPAIR_GRACE (3*TICRATE)
#define JOINSTATE_REPAIR_DELAY (2*TICRATE)
#define JOINSTATE_REPAIR_ATTEMPTS 5

static tic_t joinstate_first_bad;
static tic_t joinstate_next_action;
static UINT8 joinstate_mapping_attempts;
static UINT8 joinstate_config_attempts;
static boolean joinstate_failure_reported;

static void D_PrintHostHealth(void);
static void Command_HostHealth(void);
static void Command_RepairJoin(void);
static void D_ClientJoinStateTicker(void);

static void D_ResetJoinStateRepair(void)
{
	joinstate_first_bad = 0;
	joinstate_next_action = 0;
	joinstate_mapping_attempts = 0;
	joinstate_config_attempts = 0;
	joinstate_failure_reported = false;
}

static boolean D_LocalPlayerMapped(void)
{
	return addedtogame
		&& consoleplayer >= 0 && consoleplayer < MAXPLAYERS
		&& playeringame[consoleplayer];
}

static const char *D_LocalJoinStateProblem(void)
{
	player_t *player;

	if (!addedtogame)
		return "missing-local-player-assignment";
	if (consoleplayer < 0 || consoleplayer >= MAXPLAYERS)
		return "invalid-console-player-slot";
	if (!playeringame[consoleplayer])
		return "console-player-slot-not-in-game";

	player = &players[consoleplayer];
	if (player->skin >= numskins)
		return "invalid-player-skin";
	if (player->skincolor == SKINCOLOR_NONE)
		return "missing-player-color-config";
	if (player->skincolor >= numskincolors)
		return "invalid-player-color";
	if (player->mo && player->mo->player != player)
		return "player-object-owner-mismatch";

	return NULL;
}

static boolean D_NodeIsLoopback(INT32 node)
{
	const char *address;

	if (node == 0)
		return true;
	if (!I_GetNodeAddress || node < 0 || node >= MAXNETNODES)
		return false;
	address = I_GetNodeAddress(node);
	if (!address)
		return false;

	/* I_GetNodeAddress includes the UDP port. Cover native IPv4, native IPv6,
	 * and IPv4-mapped IPv6 without accepting ordinary LAN addresses. */
	return !strnicmp(address, "127.", 4)
		|| !strnicmp(address, "[::1]", 5)
		|| !strnicmp(address, "::1", 3)
		|| !strnicmp(address, "[::ffff:127.", 12)
		|| !strnicmp(address, "::ffff:127.", 11);
}

static INT32 D_EffectiveNetTicBuffer(void)
{
	tic_t now;

	if (cv_netlowlatency.value == 0)
		return cv_netticbuffer.value;
	/* Maximum is an explicit request for zero added safety-buffer tics.  A
	 * normal zero-buffer catch-up can look like starvation on the following
	 * frame, so applying the recovery timer here made Maximum spend nearly all
	 * of its time at one buffered tic and appear to do nothing. */
	if (cv_netlowlatency.value == 3)
		return 0;

	now = I_GetTime();
	if (netlatency_recovery_until > now)
		return 1;

	/* On normally runs at zero buffered tics, but retains the safety recovery
	 * below so a jittery connection cannot repeatedly starve the game loop. */
	if (cv_netlowlatency.value == 1)
		return 0;

	/* Adaptive begins with one buffered tic, then reaches zero after half a
	 * stable second.  A detected stall still gets the longer recovery window. */
	if (!netlatency_stable_since)
		netlatency_stable_since = now ? now : 1;
	return now < netlatency_stable_since + TICRATE/2 ? 1 : 0;
}

static void D_RecordNetTicStall(void)
{
	tic_t now;

	if (cv_netlowlatency.value == 0)
		return;

	now = I_GetTime();
	netlatency_stalls++;

	netlatency_last_stall = now;
	netlatency_stable_since = now;
	/* Maximum deliberately accepts visible jitter instead of silently adding
	 * the tic of latency the user asked to remove. Keep the stall statistic,
	 * but do not arm a recovery buffer in this mode. */
	if (cv_netlowlatency.value == 3)
		return;

	/* A short safety buffer stops the safer zero-buffer modes from repeatedly
	 * starving after actual network jitter. */
	if (netlatency_recovery_until <= now)
		netlatency_recoveries++;
	netlatency_recovery_until = now + 2*TICRATE;
}

static CV_PossibleValue_t resynchattempts_cons_t[] = {{1, "MIN"}, {20, "MAX"}, {0, "No"}, {0, NULL}};
consvar_t cv_resynchattempts = CVAR_INIT ("resynchattempts", "10", CV_SAVE|CV_NETVAR, resynchattempts_cons_t, NULL);

consvar_t cv_blamecfail = CVAR_INIT ("blamecfail", "Off", CV_SAVE|CV_NETVAR, CV_OnOff, NULL);

static CV_PossibleValue_t playbackspeed_cons_t[] = {{1, "MIN"}, {10, "MAX"}, {0, NULL}};
consvar_t cv_playbackspeed = CVAR_INIT ("playbackspeed", "1", 0, playbackspeed_cons_t, NULL);

consvar_t cv_dedicatedidletime = CVAR_INIT ("dedicatedidletime", "10", CV_SAVE|CV_NETVAR, CV_Unsigned, NULL);

static CV_PossibleValue_t idleaction_cons_t[] = {{1, "Kick"}, {2, "Spectate"}, {0, NULL}};
consvar_t cv_idleaction = CVAR_INIT ("idleaction", "Spectate", CV_SAVE|CV_NETVAR, idleaction_cons_t, NULL);
consvar_t cv_idletime = CVAR_INIT ("idletime", "3", CV_SAVE|CV_NETVAR, CV_Unsigned, NULL);

consvar_t cv_httpsource = CVAR_INIT ("http_source", "", CV_SAVE, NULL, NULL);

void ResetNode(INT32 node)
{
	memset(&netnodes[node], 0, sizeof(*netnodes));
	D_ResetConsistencyTracker(node);
	netnodes[node].player = -1;
	netnodes[node].player2 = -1;
}

void CL_Reset(void)
{
	D_ResetJoinStateRepair();
	netlatency_last_stall = 0;
	netlatency_recovery_until = 0;
	netlatency_stable_since = 0;
	netlatency_stalls = 0;
	netlatency_recoveries = 0;

	if (metalrecording)
		G_StopMetalRecording(false);
	if (metalplayback)
		G_StopMetalDemo();
	if (demorecording)
		G_CheckDemoStatus();

	// reset client/server code
	DEBFILE(va("\n-=-=-=-=-=-=-= Client reset =-=-=-=-=-=-=-\n\n"));

	if (servernode > 0 && servernode < MAXNETNODES)
	{
		netnodes[(UINT8)servernode].ingame = false;
		Net_CloseConnection(servernode);
	}
	D_CloseConnection(); // netgame = false
	multiplayer = false;
	servernode = 0;
	server = true;
	doomcom->numnodes = 1;
	doomcom->numslots = 1;
	SV_StopServer();
	SV_ResetServer();

	// make sure we don't leave any fileneeded gunk over from a failed join
	FreeFileNeeded();
	fileneedednum = 0;

	firstconnectattempttime = 0;
	serverisfull = false;
	connectiontimeout = (tic_t)cv_nettimeout.value; //reset this temporary hack

	filedownload.remaining = 0;
	filedownload.http_failed = false;
	filedownload.http_running = false;
	filedownload.http_source[0] = '\0';

	// D_StartTitle should get done now, but the calling function will handle it
}

//
// CL_ClearPlayer
//
// Clears the player data so that a future client can use this slot
//
void CL_ClearPlayer(INT32 playernum)
{
	if (players[playernum].mo)
		P_RemoveMobj(players[playernum].mo);
	memset(&players[playernum], 0, sizeof (player_t));
	memset(playeraddress[playernum], 0, sizeof(*playeraddress));
}

// Xcmd XD_ADDPLAYER
static void Got_AddPlayer(UINT8 **p, INT32 playernum)
{
	INT16 node, newplayernum;
	boolean splitscreenplayer;
	boolean rejoined;
	boolean activeassignment;
	boolean localassignmentmissing;
	player_t *newplayer;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		// protect against hacked/buggy client
		CONS_Alert(CONS_WARNING, M_GetText("Illegal add player command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	node = READUINT8(*p);
	newplayernum = READUINT8(*p);
	splitscreenplayer = newplayernum & 0x80;
	newplayernum &= ~0x80;

	rejoined = playeringame[newplayernum];
	/* Replaying an assignment for an already-connected player is a repair, not
	 * a leave/rejoin. Preserve their timers and suppress duplicate join noise.
	 * A genuine reconnect has a nonzero quittime and keeps the old behaviour. */
	activeassignment = rejoined && !players[newplayernum].quittime;
	localassignmentmissing = (node == mynode && !addedtogame);

	if (!rejoined)
	{
		// Clear player before joining, lest some things get set incorrectly
		// HACK: don't do this for splitscreen, it relies on preset values
		if (!splitscreen && !botingame)
			CL_ClearPlayer(newplayernum);
		playeringame[newplayernum] = true;
		G_AddPlayer(newplayernum);
		if (newplayernum+1 > doomcom->numslots)
			doomcom->numslots = (INT16)(newplayernum+1);

		if (server && I_GetNodeAddress)
		{
			char addressbuffer[64];
			const char *address = I_GetNodeAddress(node);
			if (address) // MI: fix msvcrt.dll!_mbscat crash?
			{
				strcpy(addressbuffer, address);
				strcpy(playeraddress[newplayernum],
						I_NetSplitAddress(addressbuffer, NULL));
			}
		}
	}

	newplayer = &players[newplayernum];

	if (!activeassignment)
	{
		newplayer->jointime = 0;
		newplayer->lastinputtime = 0;
	}
	newplayer->quittime = 0;

	READSTRINGN(*p, player_names[newplayernum], MAXPLAYERNAME);

	// the server is creating my player
	if (node == mynode)
	{
		playernode[newplayernum] = 0; // for information only
		/* A replay for a healthy local player should be invisible: do not reset
		 * the camera, HUD, view angle, or display slots. Only an actual rejoin or
		 * a client that never received its assignment needs that setup again. */
		if (!activeassignment || localassignmentmissing)
		{
			if (!splitscreenplayer)
			{
				consoleplayer = newplayernum;
				displayplayer = newplayernum;
				secondarydisplayplayer = newplayernum;
				DEBFILE("spawning me\n");
				ticcmd_oldangleturn[0] = newplayer->oldrelangleturn;
			}
			else
			{
				secondarydisplayplayer = newplayernum;
				DEBFILE("spawning my brother\n");
				if (botingame)
					newplayer->bot = 1;
				ticcmd_oldangleturn[1] = newplayer->oldrelangleturn;
			}
			P_ForceLocalAngle(newplayer, (angle_t)(newplayer->angleturn << 16));
		}
		if (rejoined)
			D_SendPlayerConfigForced();
		else
			D_SendPlayerConfig();
		addedtogame = true;

		if (rejoined && (!activeassignment || localassignmentmissing))
		{
			if (newplayer->mo)
			{
				newplayer->viewheight = 41*newplayer->height/48;

				if (newplayer->mo->eflags & MFE_VERTICALFLIP)
					newplayer->viewz = newplayer->mo->z + newplayer->mo->height - newplayer->viewheight;
				else
					newplayer->viewz = newplayer->mo->z + newplayer->viewheight;
			}

			// wake up the status bar
			ST_Start();
			// wake up the heads up text
			HU_Start();

			if (camera.chase && !splitscreenplayer)
				P_ResetCamera(newplayer, &camera);
			if (camera2.chase && splitscreenplayer)
				P_ResetCamera(newplayer, &camera2);
		}
	}

	if (netgame && !activeassignment)
	{
		char joinmsg[256];

		if (rejoined)
			strcpy(joinmsg, M_GetText("\x82*%s has rejoined the game (player %d)"));
		else
			strcpy(joinmsg, M_GetText("\x82*%s has joined the game (player %d)"));
		strcpy(joinmsg, va(joinmsg, player_names[newplayernum], newplayernum));

		// Merge join notification + IP to avoid clogging console/chat
		if (server && cv_showjoinaddress.value && I_GetNodeAddress)
		{
			const char *address = I_GetNodeAddress(node);
			if (address)
				strcat(joinmsg, va(" (%s)", address));
		}

		/* Keep joins visible in the mini-chat, but reserve the radio notification
		 * sound for messages written by a person. A system join is not chat. */
		HU_AddChatText(joinmsg, false);
	}

	if (server && multiplayer && !activeassignment && motd[0] != '\0')
		COM_BufAddText(va("sayto %d %s\n", newplayernum, motd));

	if (!rejoined)
		LUA_HookInt(newplayernum, HOOK(PlayerJoin));
}

static void UnlinkPlayerFromNode(INT32 playernum)
{
	INT32 node = playernode[playernum];

	if (node == UINT8_MAX)
		return;

	playernode[playernum] = UINT8_MAX;

	netnodes[node].numplayers--;
	if (netnodes[node].numplayers <= 0)
	{
		netnodes[node].ingame = false;
		Net_CloseConnection(node);
		ResetNode(node);
	}
}

static void PT_ClientQuit(SINT8 node, INT32 netconsole)
{
	if (client)
		return;

	if (netnodes[node].ingame && netconsole != -1 && playeringame[netconsole])
		SendKicksForNode(node, KICK_MSG_PLAYER_QUIT | KICK_MSG_KEEP_BODY);

	Net_CloseConnection(node);
	netnodes[node].ingame = false;
	netnodes[node].player = -1;
	netnodes[node].player2 = -1;
}

static void Got_KickCmd(UINT8 **p, INT32 playernum)
{
	INT32 pnum, msg;
	char buf[3 + MAX_REASONLENGTH];
	char *reason = buf;
	kickreason_t kickreason = KR_KICK;
	boolean keepbody;

	pnum = READUINT8(*p);
	msg = READUINT8(*p);
	keepbody = (msg & KICK_MSG_KEEP_BODY) != 0;
	msg &= ~KICK_MSG_KEEP_BODY;

	if (pnum == serverplayer && IsPlayerAdmin(playernum))
	{
		CONS_Printf(M_GetText("Server is being shut down remotely. Goodbye!\n"));

		if (server)
			COM_BufAddText("quit\n");

		return;
	}

	// Is playernum authorized to make this kick?
	if (playernum != serverplayer && !IsPlayerAdmin(playernum)
		&& !(playernode[playernum] != UINT8_MAX && netnodes[playernode[playernum]].numplayers == 2
		&& netnodes[playernode[playernum]].player2 == pnum))
	{
		// We received a kick command from someone who isn't the
		// server or admin, and who isn't in splitscreen removing
		// player 2. Thus, it must be someone with a modified
		// binary, trying to kick someone but without having
		// authorization.

		// We deal with this by changing the kick reason to
		// "consistency failure" and kicking the offending user
		// instead.

		// Note: Splitscreen in netgames is broken because of
		// this. Only the server has any idea of which players
		// are using splitscreen on the same computer, so
		// clients cannot always determine if a kick is
		// legitimate.

		CONS_Alert(CONS_WARNING, M_GetText("Illegal kick command received from %s for player %d\n"), player_names[playernum], pnum);

		// In debug, print a longer message with more details.
		// TODO Callum: Should we translate this?
/*
		CONS_Debug(DBG_NETPLAY,
			"So, you must be asking, why is this an illegal kick?\n"
			"Well, let's take a look at the facts, shall we?\n"
			"\n"
			"playernum (this is the guy who did it), he's %d.\n"
			"pnum (the guy he's trying to kick) is %d.\n"
			"playernum's node is %d.\n"
			"That node has %d players.\n"
			"Player 2 on that node is %d.\n"
			"pnum's node is %d.\n"
			"That node has %d players.\n"
			"Player 2 on that node is %d.\n"
			"\n"
			"If you think this is a bug, please report it, including all of the details above.\n",
				playernum, pnum,
				playernode[playernum], netnodes[playernode[playernum]].numplayers,
				netnodes[playernode[playernum]].player2,
				playernode[pnum], netnodes[playernode[pnum]].numplayers,
				netnodes[playernode[pnum]].player2);
*/
		pnum = playernum;
		msg = KICK_MSG_CON_FAIL;
		keepbody = true;
	}

	//CONS_Printf("\x82%s ", player_names[pnum]);

	switch (msg)
	{
		case KICK_MSG_GO_AWAY:
			if (!players[pnum].quittime)
				HU_AddChatText(va("\x82*%s has been kicked (No reason given)", player_names[pnum]), false);
			kickreason = KR_KICK;
			break;
		case KICK_MSG_PING_HIGH:
			HU_AddChatText(va("\x82*%s left the game (Broke ping limit)", player_names[pnum]), false);
			kickreason = KR_PINGLIMIT;
			break;
		case KICK_MSG_CON_FAIL:
			HU_AddChatText(va("\x82*%s left the game (Synch failure)", player_names[pnum]), false);
			kickreason = KR_SYNCH;

			if (M_CheckParm("-consisdump")) // Helps debugging some problems
			{
				CONS_Printf(M_GetText("Player kicked is #%d, dumping consistency...\n"), pnum);

				for (INT32 i = 0; i < MAXPLAYERS; i++)
				{
					if (!playeringame[i])
						continue;
					CONS_Printf("-------------------------------------\n");
					CONS_Printf("Player %d: %s\n", i, player_names[i]);
					CONS_Printf("Skin: %d\n", players[i].skin);
					CONS_Printf("Color: %d\n", players[i].skincolor);
					CONS_Printf("Speed: %d\n",players[i].speed>>FRACBITS);
					if (players[i].mo)
					{
						if (!players[i].mo->skin)
							CONS_Printf("Mobj skin: NULL!\n");
						else
							CONS_Printf("Mobj skin: %s\n", ((skin_t *)players[i].mo->skin)->name);
						CONS_Printf("Position: %d, %d, %d\n", players[i].mo->x, players[i].mo->y, players[i].mo->z);
						if (!players[i].mo->state)
							CONS_Printf("State: S_NULL\n");
						else
							CONS_Printf("State: %d\n", (statenum_t)(players[i].mo->state-states));
					}
					else
						CONS_Printf("Mobj: NULL\n");
					CONS_Printf("-------------------------------------\n");
				}
			}
			break;
		case KICK_MSG_TIMEOUT:
			HU_AddChatText(va("\x82*%s left the game (Connection timeout)", player_names[pnum]), false);
			#if defined(_WIN32) && !defined(DEDICATED)
			if (server)
				I_ReportRemoteExit(player_names[pnum], playernode[pnum], true,
					playerpingtable[pnum]);
			#endif
			kickreason = KR_TIMEOUT;
			break;
		case KICK_MSG_PLAYER_QUIT:
			if (netgame && !players[pnum].quittime) // not splitscreen/bots or soulless body
				HU_AddChatText(va("\x82*%s left the game", player_names[pnum]), false);
			#if defined(_WIN32) && !defined(DEDICATED)
			if (server)
				I_ReportRemoteExit(player_names[pnum], playernode[pnum], false,
					playerpingtable[pnum]);
			#endif
			kickreason = KR_LEAVE;
			break;
		case KICK_MSG_BANNED:
			HU_AddChatText(va("\x82*%s has been banned (No reason given)", player_names[pnum]), false);
			kickreason = KR_BAN;
			break;
		case KICK_MSG_CUSTOM_KICK:
			READSTRINGN(*p, reason, MAX_REASONLENGTH+1);
			HU_AddChatText(va("\x82*%s has been kicked (%s)", player_names[pnum], reason), false);
			kickreason = KR_KICK;
			break;
		case KICK_MSG_CUSTOM_BAN:
			READSTRINGN(*p, reason, MAX_REASONLENGTH+1);
			HU_AddChatText(va("\x82*%s has been banned (%s)", player_names[pnum], reason), false);
			kickreason = KR_BAN;
			break;
		case KICK_MSG_IDLE:
			HU_AddChatText(va("\x82*%s has left the game (Inactive for too long)", player_names[pnum]), false);
			kickreason = KR_TIMEOUT;
			break;
	}

	// If a verified admin banned someone, the server needs to know about it.
	// If the playernum isn't zero (the server) then the server needs to record the ban.
	if (server && playernum && (msg == KICK_MSG_BANNED || msg == KICK_MSG_CUSTOM_BAN))
	{
		if (I_Ban && !I_Ban(playernode[(INT32)pnum]))
			CONS_Alert(CONS_WARNING, M_GetText("Too many bans! Geez, that's a lot of people you're excluding...\n"));
		else
			Ban_Add(msg == KICK_MSG_CUSTOM_BAN ? reason : NULL);
	}

	if (pnum == consoleplayer)
	{
		LUA_HookBool(false, HOOK(GameQuit));
#ifdef DUMPCONSISTENCY
		if (msg == KICK_MSG_CON_FAIL) SV_SavedGame();
#endif
		D_QuitNetGame();
		CL_Reset();
		D_StartTitle();
		if (msg == KICK_MSG_CON_FAIL)
			M_StartMessage(M_GetText("Server closed connection\n(synch failure)\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_PING_HIGH)
			M_StartMessage(M_GetText("Server closed connection\n(Broke ping limit)\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_IDLE)
			M_StartMessage(M_GetText("Server closed connection\n(Inactive for too long)\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_BANNED)
			M_StartMessage(M_GetText("You have been banned by the server\n\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_CUSTOM_KICK)
			M_StartMessage(va(M_GetText("You have been kicked\n(%s)\nPress ESC\n"), reason), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_CUSTOM_BAN)
			M_StartMessage(va(M_GetText("You have been banned\n(%s)\nPress ESC\n"), reason), NULL, MM_NOTHING);
		else
			M_StartMessage(M_GetText("You have been kicked by the server\n\nPress ESC\n"), NULL, MM_NOTHING);
	}
	else if (keepbody)
	{
		if (server)
			UnlinkPlayerFromNode(pnum);
		players[pnum].quittime = 1;
	}
	else
		CL_RemovePlayer(pnum, kickreason);
}

// If in a special stage, redistribute the player's
// spheres across the remaining players.
// I feel like this shouldn't even be in this file at all, but well.
static void RedistributeSpecialStageSpheres(INT32 playernum)
{
	if (!G_IsSpecialStage(gamemap) || D_NumPlayers() <= 1)
		return;

	INT32 count = D_NumPlayers() - 1;
	INT32 spheres = players[playernum].spheres;
	INT32 rings = players[playernum].rings;

	while (spheres || rings)
	{
		INT32 sincrement = max(spheres / count, 1);
		INT32 rincrement = max(rings / count, 1);

		INT32 n;
		for (INT32 i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || i == playernum)
				continue;

			n = min(spheres, sincrement);
			P_GivePlayerSpheres(&players[i], n);
			spheres -= n;

			n = min(rings, rincrement);
			P_GivePlayerRings(&players[i], n);
			rings -= n;
		}
	}
}

//
// CL_RemovePlayer
//
// Removes a player from the current game
//
void CL_RemovePlayer(INT32 playernum, kickreason_t reason)
{
	// Sanity check: exceptional cases (i.e. c-fails) can cause multiple
	// kick commands to be issued for the same player.
	if (!playeringame[playernum])
		return;

	if (server)
		UnlinkPlayerFromNode(playernum);

	if (gametyperules & GTR_TEAMFLAGS)
		P_PlayerFlagBurst(&players[playernum], false); // Don't take the flag with you!

	RedistributeSpecialStageSpheres(playernum);

	LUA_HookPlayerQuit(&players[playernum], reason); // Lua hook for player quitting

	// don't look through someone's view who isn't there
	if (playernum == displayplayer)
	{
		// Call ViewpointSwitch hooks here.
		// The viewpoint was forcibly changed.
		LUA_HookViewpointSwitch(&players[consoleplayer], &players[consoleplayer], true);
		displayplayer = consoleplayer;
	}

	// Reset player data
	CL_ClearPlayer(playernum);

	// remove avatar of player
	playeringame[playernum] = false;
	while (!playeringame[doomcom->numslots-1] && doomcom->numslots > 1)
		doomcom->numslots--;

	// Reset the name
	sprintf(player_names[playernum], "Player %d", playernum+1);

	player_name_changes[playernum] = 0;

	if (IsPlayerAdmin(playernum))
	{
		RemoveAdminPlayer(playernum); // don't stay admin after you're gone
	}

	LUA_InvalidatePlayer(&players[playernum]);

	if (G_TagGametype()) //Check if you still have a game. Location flexible. =P
		P_CheckSurvivors();
	else if (gametyperules & GTR_RACE)
		P_CheckRacers();
}

//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//
void D_QuitNetGame(void)
{
	mousegrabbedbylua = true;
	I_UpdateMouseGrab();

	if (!netgame || !netbuffer)
		return;

	DEBFILE("===========================================================================\n"
	        "                  Quitting Game, closing connection\n"
	        "===========================================================================\n");

	// abort send/receive of files
	CloseNetFile();
	RemoveAllLuaFileTransfers();
	waitingforluafiletransfer = false;
	waitingforluafilecommand = false;

	if (server)
	{
		netbuffer->packettype = PT_SERVERSHUTDOWN;
		for (INT32 i = 0; i < MAXNETNODES; i++)
			if (netnodes[i].ingame)
				HSendPacket(i, true, 0, 0);
#ifdef MASTERSERVER
		if (serverrunning && cv_masterserver_room_id.value > 0)
			UnregisterServer();
#endif
	}
	else if (servernode > 0 && servernode < MAXNETNODES && netnodes[(UINT8)servernode].ingame)
	{
		netbuffer->packettype = PT_CLIENTQUIT;
		HSendPacket(servernode, true, 0, 0);
	}

	D_CloseConnection();
	ClearAdminPlayers();

	DEBFILE("===========================================================================\n"
	        "                         Log finish\n"
	        "===========================================================================\n");
#ifdef DEBUGFILE
	if (debugfile)
	{
		fclose(debugfile);
		debugfile = NULL;
	}
#endif
}

void CL_HandleTimeout(void)
{
	LUA_HookBool(false, HOOK(GameQuit));
	D_QuitNetGame();
	CL_Reset();
	D_StartTitle();
	M_StartMessage(M_GetText("Server Timeout\n\nPress Esc\n"), NULL, MM_NOTHING);
}

void CL_AddSplitscreenPlayer(void)
{
	if (cl_mode == CL_CONNECTED)
		CL_SendJoin();
}

void CL_RemoveSplitscreenPlayer(void)
{
	if (cl_mode != CL_CONNECTED)
		return;

	SendKick(secondarydisplayplayer, KICK_MSG_PLAYER_QUIT);
}

void SV_ResetServer(void)
{
	// +1 because this command will be executed in com_executebuffer in
	// tryruntic so gametic will be incremented, anyway maketic > gametic
	// is not an issue

	maketic = gametic + 1;
	neededtic = maketic;
	tictoclear = maketic;

	joindelay = 0;
	D_ClearAddonNotices();
	D_ResetAddonAuditState();

	for (INT32 i = 0; i < MAXNETNODES; i++)
		ResetNode(i);

	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		LUA_InvalidatePlayer(&players[i]);
		playeringame[i] = false;
		playernode[i] = UINT8_MAX;
		memset(playeraddress[i], 0, sizeof(*playeraddress));
		sprintf(player_names[i], "Player %d", i + 1);
		adminplayers[i] = -1; // Populate the entire adminplayers array with -1.
	}

	memset(player_name_changes, 0, sizeof player_name_changes);

	mynode = 0;
	cl_packetmissed = false;
	cl_redownloadinggamestate = false;

	if (dedicated)
	{
		netnodes[0].ingame = true;
		serverplayer = 0;
	}
	else
		serverplayer = consoleplayer;

	if (server)
		servernode = 0;

	doomcom->numslots = 0;

	// clear server_context
	memset(server_context, '-', 8);

	CV_RevertNetVars();

	// Ensure synched when creating a new server
	M_CopyGameData(serverGamedata, clientGamedata);

	DEBFILE("\n-=-=-=-=-=-=-= Server Reset =-=-=-=-=-=-=-\n\n");
}

static inline void SV_GenContext(void)
{
	// generate server_context, as exactly 8 bytes of randomly mixed A-Z and a-z
	// (hopefully M_Random is initialized!! if not this will be awfully silly!)
	for (UINT8 i = 0; i < 8; i++)
	{
		const char a = M_RandomKey(26*2);
		if (a < 26) // uppercase
			server_context[i] = 'A'+a;
		else // lowercase
			server_context[i] = 'a'+(a-26);
	}
}

void SV_SpawnServer(void)
{
	if (demoplayback)
		G_StopDemo(); // reset engine parameter
	if (metalplayback)
		G_StopMetalDemo();

	if (!serverrunning)
	{
		CONS_Printf(M_GetText("Starting Server....\n"));
		serverrunning = true;
		SV_ResetServer();
		SV_GenContext();
		if (netgame && I_NetOpenSocket)
		{
			I_NetOpenSocket();
#ifdef MASTERSERVER
			if (cv_masterserver_room_id.value > 0)
				RegisterServer();
#endif
		}

		// non dedicated server just connect to itself
		if (!dedicated)
			CL_ConnectToServer();
		else doomcom->numslots = 1;
	}
}

// called at singleplayer start and stopdemo
void SV_StartSinglePlayerServer(void)
{
	server = true;
	netgame = false;
	multiplayer = false;
	G_SetGametype(GT_COOP);

	// no more tic the game with this settings!
	SV_StopServer();

	if (splitscreen)
		multiplayer = true;
}

void SV_StopServer(void)
{
	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission();
	gamestate = wipegamestate = GS_NULL;

	localtextcmd[0] = 0;
	localtextcmd2[0] = 0;

	for (tic_t i = firstticstosend; i < firstticstosend + BACKUPTICS; i++)
		D_Clearticcmd(i);

	consoleplayer = 0;
	cl_mode = CL_SEARCHING;
	maketic = gametic+1;
	neededtic = maketic;
	serverrunning = false;
}

/** Called when a PT_SERVERSHUTDOWN packet is received
  *
  * \param node The packet sender (should be the server)
  *
  */
static void PT_ServerShutdown(SINT8 node)
{
	if (node != servernode || server || cl_mode == CL_SEARCHING)
		return;

	(void)node;
	LUA_HookBool(false, HOOK(GameQuit));
	D_QuitNetGame();
	CL_Reset();
	D_StartTitle();
	M_StartMessage(M_GetText("Server has shutdown\n\nPress Esc\n"), NULL, MM_NOTHING);
}

static void PT_Login(SINT8 node, INT32 netconsole)
{
	(void)node;

	if (client)
		return;

#ifndef NOMD5
	UINT8 finalmd5[16];/* Well, it's the cool thing to do? */
	UINT32 i;

	if (doomcom->datalength < 16)/* ignore partial sends */
		return;

	if (adminpasscount == 0)
	{
		CONS_Printf(M_GetText("Password from %s failed (no password set).\n"), player_names[netconsole]);
		return;
	}

	for (i = 0; i < adminpasscount; i++)
	{
		// Do the final pass to compare with the sent md5
		D_MD5PasswordPass(adminpassmd5[i], 16, va("PNUM%02d", netconsole), &finalmd5);

		if (!memcmp(netbuffer->u.md5sum, finalmd5, 16))
		{
			CONS_Printf(M_GetText("%s passed authentication.\n"), player_names[netconsole]);
			COM_BufInsertText(va("promote %d\n", netconsole)); // do this immediately
			return;
		}
	}

	CONS_Printf(M_GetText("Password from %s failed.\n"), player_names[netconsole]);
#else
	(void)netconsole;
#endif
}

/** Add a login for HTTP downloads. If the
  * user/password is missing, remove it.
  *
  * \sa Command_list_http_logins
  */
static void Command_set_http_login (void)
{
	HTTP_login  *login;
	HTTP_login **prev_next;

	if (COM_Argc() < 2)
	{
		CONS_Printf(
				"set_http_login <URL> [user:password]: Set or remove a login to "
				"authenticate HTTP downloads.\n"
		);
		return;
	}

	login = CURLGetLogin(COM_Argv(1), &prev_next);

	if (COM_Argc() == 2)
	{
		if (login)
		{
			(*prev_next) = login->next;
			CONS_Printf("Login for '%s' removed.\n", login->url);
			Z_Free(login);
		}
	}
	else
	{
		if (login)
			Z_Free(login->auth);
		else
		{
			login = ZZ_Alloc(sizeof *login);
			login->url = Z_StrDup(COM_Argv(1));
		}

		login->auth = Z_StrDup(COM_Argv(2));

		login->next = curl_logins;
		curl_logins = login;
	}
}

/** List logins for HTTP downloads.
  *
  * \sa Command_set_http_login
  */
static void Command_list_http_logins (void)
{
	HTTP_login *login;

	for (
			login = curl_logins;
			login;
			login = login->next
	){
		CONS_Printf(
				"'%s' -> '%s'\n",
				login->url,
				login->auth
		);
	}
}

static void PT_AskLuaFile(SINT8 node)
{
	if (server && luafiletransfers && luafiletransfers->nodestatus[node] == LFTNS_ASKED)
		AddLuaFileToSendQueue(node, luafiletransfers->realfilename);
}

static void PT_HasLuaFile(SINT8 node)
{
	if (server && luafiletransfers && luafiletransfers->nodestatus[node] == LFTNS_SENDING)
		SV_HandleLuaFileSent(node);
}

static void PT_SendingLuaFile(SINT8 node)
{
	(void)node;

	if (client)
		CL_PrepareDownloadLuaFile();
}

/*
Ping Update except better:
We call this once per second and check for people's pings. If their ping happens to be too high, we increment some timer and kick them out.
If they're not lagging, decrement the timer by 1. Of course, reset all of this if they leave.
*/

static inline void PingUpdate(void)
{
	boolean laggers[MAXPLAYERS];
	UINT8 numlaggers = 0;
	memset(laggers, 0, sizeof(boolean) * MAXPLAYERS);

	netbuffer->packettype = PT_PING;

	//check for ping limit breakage.
	if (cv_maxping.value)
	{
		for (INT32 i = 1; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && !players[i].quittime
			&& (realpingtable[i] / pingmeasurecount > (unsigned)cv_maxping.value))
			{
				if (players[i].jointime > 30 * TICRATE)
					laggers[i] = true;
				numlaggers++;
			}
			else
				pingtimeout[i] = 0;
		}

		//kick lagging players... unless everyone but the server's ping sucks.
		//in that case, it is probably the server's fault.
		if (numlaggers < D_NumPlayers() - 1)
		{
			for (INT32 i = 1; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] && laggers[i])
				{
					pingtimeout[i]++;
					// ok your net has been bad for too long, you deserve to die.
					if (pingtimeout[i] > cv_pingtimeout.value)
					{
						pingtimeout[i] = 0;
						SendKick(i, KICK_MSG_PING_HIGH | KICK_MSG_KEEP_BODY);
					}
				}
				/*
					you aren't lagging,
					but you aren't free yet.
					In case you'll keep spiking,
					we just make the timer go back down. (Very unstable net must still get kicked).
				*/
				else
					pingtimeout[i] = (pingtimeout[i] == 0 ? 0 : pingtimeout[i]-1);
			}
		}
	}

	//make the ping packet and clear server data for next one
	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		netbuffer->u.pingtable[i] = realpingtable[i] / pingmeasurecount;
		//server takes a snapshot of the real ping for display.
		//otherwise, pings fluctuate a lot and would be odd to look at.
		playerpingtable[i] = realpingtable[i] / pingmeasurecount;
		realpingtable[i] = 0; //Reset each as we go.
	}

	// send the server's maxping as last element of our ping table. This is useful to let us know when we're about to get kicked.
	netbuffer->u.pingtable[MAXPLAYERS] = cv_maxping.value;

	//send out our ping packets
	for (INT32 i = 0; i < MAXNETNODES; i++)
		if (netnodes[i].ingame)
			HSendPacket(i, true, 0, sizeof(INT32) * (MAXPLAYERS+1));

	pingmeasurecount = 1; //Reset count
}

static void PT_Ping(SINT8 node, INT32 netconsole)
{
	// Only accept PT_PING from the server.
	if (node != servernode)
	{
		CONS_Alert(CONS_WARNING, M_GetText("%s received from non-host %d\n"), "PT_PING", node);
		if (server)
			SendKick(netconsole, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	//Update client ping table from the server.
	if (client)
	{
		for (INT32 i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
				playerpingtable[i] = (tic_t)netbuffer->u.pingtable[i];

		servermaxping = (tic_t)netbuffer->u.pingtable[MAXPLAYERS];
	}
}

static void PT_BasicKeepAlive(SINT8 node, INT32 netconsole)
{
	if (client)
		return;

	// This should probably still timeout though, as the node should always have a player 1 number
	if (netconsole == -1)
		return;

	// If a client sends this it should mean they are done receiving the savegame
	netnodes[node].sendingsavegame = false;

	// As long as clients send keep alives, the server can keep running, so reset the timeout
	/// \todo Use a separate cvar for that kind of timeout?
	netnodes[node].freezetimeout = I_GetTime() + connectiontimeout;
	return;
}

// Confusing, but this DOESN'T send PT_NODEKEEPALIVE, it sends PT_BASICKEEPALIVE
// Used during wipes to tell the server that a node is still connected
static void CL_SendClientKeepAlive(void)
{
	netbuffer->packettype = PT_BASICKEEPALIVE;

	HSendPacket(servernode, false, 0, 0);
}

static void SV_SendServerKeepAlive(void)
{
	for (INT32 n = 1; n < MAXNETNODES; n++)
	{
		if (netnodes[n].ingame)
		{
			netbuffer->packettype = PT_BASICKEEPALIVE;
			HSendPacket(n, false, 0, 0);
		}
	}
}

/** Handles a packet received from a node that isn't in game
  *
  * \param node The packet sender
  * \todo Choose a better name, as the packet can also come from the server apparently?
  * \sa HandlePacketFromPlayer
  * \sa GetPackets
  *
  */
static void HandlePacketFromAwayNode(SINT8 node)
{
	if (node != servernode)
		DEBFILE(va("Received packet from unknown host %d\n", node));

	switch (netbuffer->packettype)
	{
		case PT_ASKINFOVIAMS   : PT_AskInfoViaMS   (node    ); break;
		case PT_SERVERINFO     : PT_ServerInfo     (node    ); break;
		case PT_TELLFILESNEEDED: PT_TellFilesNeeded(node    ); break;
		case PT_MOREFILESNEEDED: PT_MoreFilesNeeded(node    ); break;
		case PT_ASKINFO        : PT_AskInfo        (node    ); break;
		case PT_SERVERREFUSE   : PT_ServerRefuse   (node    ); break;
		case PT_SERVERCFG      : PT_ServerCFG      (node    ); break;
		case PT_FILEFRAGMENT   : PT_FileFragment   (node, -1); break;
		case PT_FILEACK        : PT_FileAck        (node    ); break;
		case PT_FILERECEIVED   : PT_FileReceived   (node    ); break;
		case PT_REQUESTFILE    : PT_RequestFile    (node    ); break;
		case PT_CLIENTQUIT     : PT_ClientQuit     (node, -1); break;
		case PT_SERVERTICS     : PT_ServerTics     (node, -1); break;
		case PT_CLIENTJOIN     : PT_ClientJoin     (node    ); break;
		case PT_SERVERSHUTDOWN : PT_ServerShutdown (node    ); break;
		case PT_CLIENTCMD      :                               break; // This is not an "unknown packet"
		case PT_PLAYERINFO     :                               break; // This is not an "unknown packet"

		default:
			DEBFILE(va("unknown packet received (%d) from unknown host\n",netbuffer->packettype));
			Net_CloseConnection(node);
	}
}

/** Handles a packet received from a node that is in game
  *
  * \param node The packet sender
  * \todo Choose a better name
  * \sa HandlePacketFromAwayNode
  * \sa GetPackets
  *
  */
static void HandlePacketFromPlayer(SINT8 node)
{
	INT32 netconsole;

	if (dedicated && node == 0)
		netconsole = 0;
	else
		netconsole = netnodes[node].player;

#ifdef PARANOIA
	if (netconsole >= MAXPLAYERS)
		I_Error("bad table nodetoplayer: node %d player %d", doomcom->remotenode, netconsole);
#endif

	switch (netbuffer->packettype)
	{
		// SERVER RECEIVE
		case PT_CLIENTCMD:
		case PT_CLIENT2CMD:
		case PT_CLIENTMIS:
		case PT_CLIENT2MIS:
		case PT_NODEKEEPALIVE:
		case PT_NODEKEEPALIVEMIS:
			PT_ClientCmd(node, netconsole);
			break;
		case PT_BASICKEEPALIVE     : PT_BasicKeepAlive     (node, netconsole); break;
		case PT_TEXTCMD            : PT_TextCmd            (node, netconsole); break;
		case PT_TEXTCMD2           : PT_TextCmd            (node, netconsole); break;
		case PT_LOGIN              : PT_Login              (node, netconsole); break;
		case PT_CLIENTQUIT         : PT_ClientQuit         (node, netconsole); break;
		case PT_CANRECEIVEGAMESTATE: PT_CanReceiveGamestate(node            ); break;
		case PT_ASKLUAFILE         : PT_AskLuaFile         (node            ); break;
		case PT_HASLUAFILE         : PT_HasLuaFile         (node            ); break;
		case PT_RECEIVEDGAMESTATE  : PT_ReceivedGamestate  (node            ); break;
		case PT_SERVERINFO         : PT_ServerInfo         (node            ); break;

		// CLIENT RECEIVE
		case PT_SERVERTICS         : PT_ServerTics         (node, netconsole); break;
		case PT_PING               : PT_Ping               (node, netconsole); break;
		case PT_FILEFRAGMENT       : PT_FileFragment       (node, netconsole); break;
		case PT_FILEACK            : PT_FileAck            (node            ); break;
		case PT_FILERECEIVED       : PT_FileReceived       (node            ); break;
		case PT_WILLRESENDGAMESTATE: PT_WillResendGamestate(node            ); break;
		case PT_SENDINGLUAFILE     : PT_SendingLuaFile     (node            ); break;
		case PT_SERVERSHUTDOWN     : PT_ServerShutdown     (node            ); break;
		case PT_SERVERCFG          :                                           break;
		/* A client retries PT_CLIENTJOIN until its reliable PT_SERVERCFG is
		 * received. Once SV_AddNode marks it in-game, those retries arrive in
		 * this dispatcher rather than HandlePacketFromAwayNode. Do not discard
		 * them: PT_ClientJoin detects the existing node and safely resends the
		 * configuration packet. */
		case PT_CLIENTJOIN         : PT_ClientJoin         (node            ); break;

		default:
			DEBFILE(va("UNKNOWN PACKET TYPE RECEIVED %d from host %d\n",
				netbuffer->packettype, node));
	}
}

/**	Handles all received packets, if any
  *
  * \todo Add details to this description (lol)
  *
  */
void GetPackets(void)
{
	while (HGetPacket())
	{
		SINT8 node = doomcom->remotenode;

		// Packet received from someone already playing
		if (netnodes[node].ingame)
			HandlePacketFromPlayer(node);
		// Packet received from someone not playing
		else
			HandlePacketFromAwayNode(node);
	}
}

boolean TryRunTics(tic_t realtics)
{
	// the machine has lagged but it is not so bad
	if (realtics > TICRATE/7)
	{
		if (server)
			realtics = 1;
		else
			realtics = TICRATE/7;
	}

	if (singletics)
		realtics = 1;

	if (realtics >= 1)
	{
		COM_BufTicker();
		if (mapchangepending)
			D_MapChange(-1, 0, ultimatemode, false, 2, false, fromlevelselect); // finish the map change
	}

	NetUpdate();

	if (demoplayback)
	{
		neededtic = gametic + realtics;
		// start a game after a demo
		maketic += realtics;
		firstticstosend = maketic;
		tictoclear = firstticstosend;
	}

	GetPackets();

#ifdef DEBUGFILE
	if (debugfile && (realtics || neededtic > gametic))
	{
		fprintf(debugfile, "------------ Tryruntic: REAL:%d NEED:%d GAME:%d LOAD: %d\n",
			realtics, neededtic, gametic, debugload);
		debugload = 100000;
	}
#endif

	if (neededtic > gametic)
	{
		if (realtics)
			hu_stopped = false;

		if (advancedemo)
		{
			if (timedemo_quit)
				COM_ImmedExecute("quit");
			else
				D_StartTitle();
		}
		else
			// run the count * tics
			while (neededtic > gametic)
			{
				boolean update_stats = !(paused || P_AutoPause());

				DEBFILE(va("============ Running tic %d (local %d)\n", gametic, localgametic));

				if (update_stats)
				{
					PS_BeginTickProfile();
					PS_START_TIMING(ps_tictime);
				}

				G_Ticker((gametic % NEWTICRATERATIO) == 0);
				ExtraDataTicker();
				gametic++;
				consistancy[gametic%BACKUPTICS] = Consistancy();

				if (update_stats)
				{
					PS_STOP_TIMING(ps_tictime);
				#if defined(_WIN32) && !defined(DEDICATED)
					/* A 35 Hz game tic has a 28.6 ms budget. Logging ordinary
					 * 20-28 ms tics created thousands of false "hitches" and kept
					 * diagnostic profiling hot on object-heavy maps. */
					I_ReportHitch("game tic", I_GetPreciseTime() - ps_tictime.value.p, 30);
				#endif
					PS_UpdateTickStats();
				}

				// Leave a certain amount of tics present in the net buffer as long as we've ran at least one tic this frame.
				if (client && gamestate == GS_LEVEL && leveltime > 3
					&& neededtic <= gametic + D_EffectiveNetTicBuffer())
					break;
			}

		return true;
	}
	else
	{
		if (realtics)
		{
			hu_stopped = true;
			if (client && gamestate == GS_LEVEL && leveltime > 3)
				D_RecordNetTicStall();
		}

		return false;
	}
}

static void UpdatePingTable(void)
{
	if (server)
	{
		if (netgame && !(gametime % 35)) // update once per second.
			PingUpdate();
		// update node latency values so we can take an average later.
		for (INT32 i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && playernode[i] != UINT8_MAX)
			{
				/* Loopback RTT is sub-millisecond. GetLag is quantized to the
				 * 35 Hz simulation clock, which made a local dedicated companion
				 * misleadingly appear as 27 ms despite no internet hop. This changes
				 * reporting and ping-limit decisions only, not lockstep timing. */
				if (!D_NodeIsLoopback(playernode[i]))
					realpingtable[i] += G_TicsToMilliseconds(GetLag(playernode[i]));
			}
		pingmeasurecount++;
	}
}

// Handle idle and disconnected player timers
static void IdleUpdate(void)
{
	INT32 i;
	if (!server || !netgame)
		return;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && playernode[i] != UINT8_MAX && !players[i].quittime && !players[i].spectator && !players[i].bot && gamestate == GS_LEVEL)
		{
			if (players[i].cmd.forwardmove || players[i].cmd.sidemove || players[i].cmd.buttons)
				players[i].lastinputtime = 0;
			else
				players[i].lastinputtime++;

			if (cv_idletime.value && !IsPlayerAdmin(i) && i != serverplayer && !(players[i].pflags & PF_FINISHED) && players[i].lastinputtime > (tic_t)cv_idletime.value * TICRATE * 60)
			{
				players[i].lastinputtime = 0;
				if (cv_idleaction.value == 2 && G_GametypeHasSpectators())
				{
					changeteam_union NetPacket;
					UINT16 usvalue;
					NetPacket.value.l = NetPacket.value.b = 0;
					NetPacket.packet.newteam = 0;
					NetPacket.packet.playernum = i;
					NetPacket.packet.verification = true; // This signals that it's a server change
					usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
					SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
				}
				else if (cv_idleaction.value == 1)
				{
					SendKick(i, KICK_MSG_IDLE | KICK_MSG_KEEP_BODY);
				}
			}
		}
		else
		{
			players[i].lastinputtime = 0;

			if (players[i].quittime && playeringame[i])
			{
				players[i].quittime++;

				if (players[i].quittime == 30 * TICRATE && G_TagGametype())
					P_CheckSurvivors();

				if (server && players[i].quittime >= (tic_t)FixedMul(cv_rejointimeout.value, 60 * TICRATE)
				&& !(players[i].quittime % TICRATE))
				{
					if (D_NumNodes(true) > 0)
						SendKick(i, KICK_MSG_PLAYER_QUIT);
					else // If the server is empty, don't send a NetXCmd - that would wake an idling dedicated server
						CL_RemovePlayer(i, KICK_MSG_PLAYER_QUIT);
				}
			}
		}
	}
}

static void D_ClientJoinStateTicker(void)
{
	const char *problem;
	tic_t now;
	UINT8 attempts;

	if (!client || !netgame || cl_mode != CL_CONNECTED)
	{
		D_ResetJoinStateRepair();
		return;
	}

	problem = D_LocalJoinStateProblem();
	if (!problem)
	{
		attempts = joinstate_mapping_attempts + joinstate_config_attempts;
		if (attempts)
		{
			D_WriteJoinStateReport("client-join-state-recovered", consoleplayer,
				servernode, attempts);
			CONS_Printf("Recovered local player join state after %u repair attempt%s.\n",
				(unsigned)attempts, attempts == 1 ? "" : "s");
		}
		D_ResetJoinStateRepair();
		return;
	}

	now = I_GetTime();
	if (!joinstate_first_bad)
	{
		joinstate_first_bad = now ? now : 1;
		joinstate_next_action = now + JOINSTATE_REPAIR_GRACE;
		return;
	}
	if (now < joinstate_next_action)
		return;

	if (!D_LocalPlayerMapped())
	{
		if (joinstate_mapping_attempts < JOINSTATE_REPAIR_ATTEMPTS)
		{
			if (!joinstate_mapping_attempts)
				D_WriteJoinStateReport(problem, -1, servernode, 0);

			/* Reuse vanilla PT_CLIENTJOIN. Optimized hosts answer by replaying
			 * the existing XD_ADDPLAYER mapping; stock hosts safely ignore the
			 * repair aspect and remain fully compatible. */
			if (CL_SendJoin())
			{
				joinstate_mapping_attempts++;
				joinstate_next_action = now + JOINSTATE_REPAIR_DELAY;
				CONS_Alert(CONS_WARNING,
					"Local player assignment was incomplete; requesting repair (attempt %u/%u).\n",
					(unsigned)joinstate_mapping_attempts,
					(unsigned)JOINSTATE_REPAIR_ATTEMPTS);
			}
			else
				joinstate_next_action = now + TICRATE;
			return;
		}
	}
	else if (joinstate_config_attempts < JOINSTATE_REPAIR_ATTEMPTS)
	{
		if (!Playing())
		{
			joinstate_next_action = now + TICRATE;
			return;
		}
		if (!joinstate_config_attempts)
			D_WriteJoinStateReport(problem, consoleplayer, servernode, 0);

		D_SendPlayerConfigForced();
		joinstate_config_attempts++;
		joinstate_next_action = now + JOINSTATE_REPAIR_DELAY;
		CONS_Alert(CONS_WARNING,
			"Local player appearance/config was incomplete; resending it (attempt %u/%u).\n",
			(unsigned)joinstate_config_attempts,
			(unsigned)JOINSTATE_REPAIR_ATTEMPTS);
		return;
	}

	if (!joinstate_failure_reported)
	{
		joinstate_failure_reported = true;
		attempts = joinstate_mapping_attempts + joinstate_config_attempts;
		D_WriteJoinStateReport("client-join-state-unrecovered",
			D_LocalPlayerMapped() ? consoleplayer : -1, servernode, attempts);
		CONS_Alert(CONS_WARNING,
			"Automatic join-state repair did not finish. The connection was kept open; details were saved in diagnostics/join-state-incidents-v1.txt.\n");
	}
}

static void Command_RepairJoin(void)
{
	boolean sent;

	if (!client || !netgame || cl_mode != CL_CONNECTED)
	{
		CONS_Printf("repairjoin is available after joining a remote server.\n");
		return;
	}

	D_WriteJoinStateReport("manual-join-state-repair",
		D_LocalPlayerMapped() ? consoleplayer : -1, servernode, 0);
	sent = CL_SendJoin();
	if (D_LocalPlayerMapped() && Playing())
		D_SendPlayerConfigForced();

	if (sent)
		CONS_Printf("Requested a player-assignment and appearance repair from the server.\n");
	else
		CONS_Alert(CONS_WARNING, "The join-state repair request could not be sent yet.\n");
}

static void DedicatedIdleUpdate(INT32 *realtics)
{
	const tic_t dedicatedidletime = cv_dedicatedidletime.value * TICRATE;
	static tic_t dedicatedidletimeprev = 0;
	static tic_t dedicatedidle = 0;

	if (!server || !dedicated || gamestate != GS_LEVEL)
		return;

	if (dedicatedidletime > 0)
	{
		INT32 i;

		boolean empty = true;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
			{
				empty = false;
				break;
			}

		if (empty)
		{
			if (leveltime == 2)
			{
				// On next tick...
				dedicatedidle = dedicatedidletime - 1;
			}
			else if (dedicatedidle >= dedicatedidletime)
			{
				if (D_GetExistingTextcmd(gametic, 0) || D_GetExistingTextcmd(gametic + 1, 0))
				{
					CONS_Printf("DEDICATED: Awakening from idle (Netxcmd detected...)\n");
					dedicatedidle = 0;
				}
				else
				{
					(*realtics) = 0;
				}
			}
			else
			{
				dedicatedidle += (*realtics);

				if (dedicatedidle >= dedicatedidletime)
				{
					const char *idlereason = "at round start";
					if (leveltime > 3)
						idlereason = va("for %d seconds", dedicatedidle / TICRATE);

					CONS_Printf("DEDICATED: No players %s, idling...\n", idlereason);
					(*realtics) = 0;
					dedicatedidle = dedicatedidletime;
				}
			}
		}
		else
		{
			if (dedicatedidle >= dedicatedidletime)
			{
				CONS_Printf("DEDICATED: Awakening from idle (Player detected...)\n");
			}
			dedicatedidle = 0;
		}
	}
	else
	{
		if (dedicatedidletimeprev > 0 && dedicatedidle >= dedicatedidletimeprev)
		{
			CONS_Printf("DEDICATED: Awakening from idle (Idle disabled...)\n");
		}
		dedicatedidle = 0;
	}

	dedicatedidletimeprev = dedicatedidletime;
}

// Handle timeouts to prevent definitive freezes from happenning
static void HandleNodeTimeouts(void)
{
	if (server)
	{
		for (INT32 i = 1; i < MAXNETNODES; i++)
			if (netnodes[i].ingame && netnodes[i].freezetimeout < I_GetTime())
				Net_ConnectionTimeout(i);

		// In case the cvar value was lowered
		if (joindelay)
			joindelay = min(joindelay - 1, 3 * (tic_t)cv_joindelay.value * TICRATE);
	}
}

// Keep the network alive while not advancing tics!
void NetKeepAlive(void)
{
	tic_t nowtime;
	INT32 realtics;

	nowtime = I_GetTime();
	realtics = nowtime - gametime;

	// return if there's no time passed since the last call
	if (realtics <= 0) // nothing new to update
		return;

	UpdatePingTable();

	GetPackets();

	IdleUpdate();

#ifdef MASTERSERVER
	MasterClient_Ticker();
#endif

	if (client)
	{
		// send keep alive
		CL_SendClientKeepAlive();
		// No need to check for resynch because we aren't running any tics
	}
	else
	{
		SV_SendServerKeepAlive();
	}

	// No else because no tics are being run and we can't resynch during this

	Net_AckTicker();
	HandleNodeTimeouts();
	FileSendTicker();

	/* A deliberately slow cadence keeps the live host monitor useful without
	 * making the console itself a source of frame hitches on busy servers. */
	if (hosthealth_watch && I_GetTime() >= hosthealth_next_report)
	{
		D_PrintHostHealth();
		hosthealth_next_report = I_GetTime() + 10*TICRATE;
	}
}

void NetUpdate(void)
{
	static tic_t resptime = 0;
	tic_t nowtime;
	INT32 realtics;

	nowtime = I_GetTime();
	realtics = nowtime - gametime;

	if (realtics <= 0) // nothing new to update
		return;

	if (realtics > 5)
	{
		if (server)
			realtics = 1;
		else
			realtics = 5;
	}

	DedicatedIdleUpdate(&realtics);

	gametime = nowtime;

	UpdatePingTable();

	if (client)
		maketic = neededtic;

	Local_Maketic(realtics);

	if (server)
		CL_SendClientCmd(); // send it

	GetPackets(); // get packet from client or from server
	if (server)
		SV_JoinStateTicker();
	else
		D_ClientJoinStateTicker();

	IdleUpdate();

	// The client sends the command after receiving from the server
	// The server sends it before because this is better in single player

#ifdef MASTERSERVER
	MasterClient_Ticker(); // Acking the Master Server
#endif

	if (client)
	{
		// If the client just finished redownloading the game state, load it
		if (cl_redownloadinggamestate && fileneeded[0].status == FS_FOUND)
			CL_ReloadReceivedSavegame();

		CL_SendClientCmd(); // Send tic cmd
		hu_redownloadinggamestate = cl_redownloadinggamestate;
	}
	else
	{
		if (!demoplayback && realtics > 0)
		{
			hu_redownloadinggamestate = false;

			firstticstosend = gametic;
			for (INT32 i = 0; i < MAXNETNODES; i++)
				if (netnodes[i].ingame)
				{
					if (netnodes[i].tic < firstticstosend)
						firstticstosend = netnodes[i].tic;

					if (maketic + realtics >= netnodes[i].tic + BACKUPTICS - TICRATE)
						Net_ConnectionTimeout(i);
				}

			// Don't erase tics not acknowledged
			INT32 counts = realtics;
			if (maketic + counts >= firstticstosend + BACKUPTICS)
				counts = firstticstosend+BACKUPTICS-maketic-1;

			for (INT32 i = 0; i < counts; i++)
				SV_Maketic(); // Create missed tics and increment maketic

			for (; tictoclear < firstticstosend; tictoclear++) // Clear only when acknowledged
				D_Clearticcmd(tictoclear);                    // Clear the maketic the new tic

			SV_SendTics();

			neededtic = maketic; // The server is a client too
		}
	}

	Net_AckTicker();
	HandleNodeTimeouts();

	nowtime /= NEWTICRATERATIO;

	if (nowtime != resptime)
	{
		resptime = nowtime;
#ifdef HAVE_THREADS
		I_lock_mutex(&m_menu_mutex);
#endif
		M_Ticker();
#ifdef HAVE_THREADS
		I_unlock_mutex(m_menu_mutex);
#endif
		CON_Ticker();
	}

	FileSendTicker();
}

static void Command_ConsistencyReport(void)
{
	INT16 current = Consistancy();
	D_WriteConsistencyReport("manual", gametic, -1, current, current);
}

static void Command_NetLatencyStats(void)
{
	const INT32 effective = D_EffectiveNetTicBuffer();
	const INT32 configured = cv_netticbuffer.value;
	const INT32 removed = max(0, configured - effective);
	const double milliseconds = effective * (1000.0 / TICRATE);
	const double savedmilliseconds = removed * (1000.0 / TICRATE);
	const boolean haveping = netgame && consoleplayer >= 0
		&& consoleplayer < MAXPLAYERS && playeringame[consoleplayer];
	const UINT32 pingms = haveping ? playerpingtable[consoleplayer] : 0;
	const boolean loopback = client && D_NodeIsLoopback(servernode);

	CONS_Printf("Network buffer reduction: %s\n", cv_netlowlatency.string);
	CONS_Printf("Configured/effective safety buffer: %d/%d tics\n",
		configured, effective);
	CONS_Printf("Effective buffer delay: about %.1f ms; reduction saves about %.1f ms\n",
		milliseconds, savedmilliseconds);
	if (haveping)
	{
		CONS_Printf("Measured round-trip ping: %u ms\n", pingms);
		if (loopback)
			CONS_Printf("Connection path: local loopback (displayed as 0 ms; simulation still advances at 35 Hz)\n");
		CONS_Printf("Estimated authoritative response floor: at least %.1f ms, plus server tick scheduling\n",
			pingms + milliseconds);
	}
	else
		CONS_Printf("Measured round-trip ping: unavailable (not connected as a player)\n");
	CONS_Printf("Local movement prediction: Off (this lockstep client waits for server-authoritative movement)\n");
	if (cv_netlowlatency.value == 3)
		CONS_Printf("Maximum keeps the added buffer at zero even during jitter; visible stutter is possible.\n");
	if (haveping && pingms >= 100)
		CONS_Printf("High ping is the dominant delay; buffer reduction cannot remove physical network latency.\n");
	CONS_Printf("Network tic stalls: %u; jitter recoveries: %u\n",
		netlatency_stalls, netlatency_recoveries);
}

static void D_PrintHostHealth(void)
{
	INT32 i;
	INT32 players_seen = 0;
	INT32 high_ping = 0;
	INT32 congested = 0;

	CONS_Printf("\n\x82--- Live host health ---\n");
	if (!netgame)
	{
		CONS_Printf("Not connected to or hosting a network game.\n");
		return;
	}

	CONS_Printf("Role: %s%s | receive/send: %d/%d B/s | local packet loss: %.2f%%\n",
		server ? (dedicated ? "dedicated server" : "listen server") : "client",
		I_CompanionServerRunning() ? " controlling local dedicated server" : "",
		getbps, sendbps, lostpercent);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		UINT8 node;
		UINT32 ping;
		UINT16 queued = 0;
		UINT16 retries = 0;
		const char *quality = "good";

		if (!playeringame[i])
			continue;

		players_seen++;
		node = playernode[i];
		ping = playerpingtable[i];
		if (ping >= 250)
		{
			quality = "very high ping";
			high_ping++;
		}
		else if (ping >= 120)
		{
			quality = "high ping";
			high_ping++;
		}

		if (server && node > 0 && node < MAXNETNODES)
		{
			queued = Net_GetReliableBacklog(node, &retries);
			if (queued >= 16 || retries >= 3)
			{
				quality = "congested";
				congested++;
			}
			CONS_Printf("  #%d %-20s ping=%ums node=%u queue=%u retries=%u [%s]\n",
				i, player_names[i], (unsigned)ping, (unsigned)node,
				(unsigned)queued, (unsigned)retries, quality);
		}
		else
			CONS_Printf("  #%d %-20s ping=%ums [%s]\n",
				i, player_names[i], (unsigned)ping, quality);
	}

	CONS_Printf("Summary: %d player%s, %d high-ping, %d congested connection%s.\n",
		players_seen, players_seen == 1 ? "" : "s", high_ping, congested,
		congested == 1 ? "" : "s");
	if (server && !dedicated && players_seen >= 5)
		CONS_Printf("Recommendation: Local Dedicated isolates server simulation from rendering and recording stalls.\n");
	if (lostpercent > 5.0f)
		CONS_Printf("Warning: local packet loss is high; check Wi-Fi, upload saturation, or router queueing.\n");
	if (congested)
		CONS_Printf("Warning: reliable traffic is backing up; add-on transfers are being throttled automatically.\n");
}

static void Command_HostHealth(void)
{
	if (COM_Argc() > 1)
	{
		if (!stricmp(COM_Argv(1), "watch") || !stricmp(COM_Argv(1), "on"))
		{
			hosthealth_watch = true;
			hosthealth_next_report = I_GetTime();
			CONS_Printf("Live host-health reports enabled (every 10 seconds). Use 'hosthealth off' to stop.\n");
			return;
		}
		if (!stricmp(COM_Argv(1), "off"))
		{
			hosthealth_watch = false;
			CONS_Printf("Live host-health reports disabled.\n");
			return;
		}
		CONS_Printf("Usage: hosthealth [watch|off]\n");
		return;
	}

	D_PrintHostHealth();
}

// called one time at init
void D_ClientServerInit(void)
{
	char diagnosticdir[MAX_WADPATH];

	/* Keep the destination visible even before the first incident. Reports used
	 * to appear to vanish simply because the directory was created lazily only
	 * after a hitch/resync occurred in that particular run. */
	snprintf(diagnosticdir, sizeof diagnosticdir,
		"%s" PATHSEP "diagnostics", srb2home);
	diagnosticdir[sizeof diagnosticdir - 1] = '\0';
	I_mkdir(diagnosticdir, 0755);

	DEBFILE(va("- - -== SRB2 v%d.%.2d.%d "VERSIONSTRING" debugfile ==- - -\n",
		VERSION/100, VERSION%100, SUBVERSION));

	COM_AddCommand("getplayernum", Command_GetPlayerNum, COM_LUA);
	COM_AddCommand("kick", Command_Kick, COM_LUA);
	COM_AddCommand("ban", Command_Ban, COM_LUA);
	COM_AddCommand("banip", Command_BanIP, COM_LUA);
	COM_AddCommand("clearbans", Command_ClearBans, COM_LUA);
	COM_AddCommand("showbanlist", Command_ShowBan, COM_LUA);
	COM_AddCommand("reloadbans", Command_ReloadBan, COM_LUA);
	COM_AddCommand("connect", Command_connect, COM_LUA);
	COM_AddCommand("nodes", Command_Nodes, COM_LUA);
	COM_AddCommand("set_http_login", Command_set_http_login, 0);
	COM_AddCommand("list_http_logins", Command_list_http_logins, 0);
	COM_AddCommand("resendgamestate", Command_ResendGamestate, COM_LUA);
	COM_AddCommand("consistencyreport", Command_ConsistencyReport, COM_LUA);
	COM_AddCommand("netlatencystats", Command_NetLatencyStats, COM_LUA);
	COM_AddCommand("hosthealth", Command_HostHealth, COM_LUA);
	COM_AddCommand("repairjoin", Command_RepairJoin, 0);
#ifdef PACKETDROP
	COM_AddCommand("drop", Command_Drop, COM_LUA);
	COM_AddCommand("droprate", Command_Droprate, COM_LUA);
#endif
#ifdef _DEBUG
	COM_AddCommand("numnodes", Command_Numnodes, COM_LUA);
#endif

	RegisterNetXCmd(XD_KICK, Got_KickCmd);
	RegisterNetXCmd(XD_ADDPLAYER, Got_AddPlayer);
#ifdef DUMPCONSISTENCY
	CV_RegisterVar(&cv_dumpconsistency);
#endif
	Ban_Load_File(false);

	gametic = 0;
	localgametic = 0;

	// do not send anything before the real begin
	SV_StopServer();
	SV_ResetServer();
	if (dedicated)
		SV_SpawnServer();
}

SINT8 nametonum(const char *name)
{
	INT32 playernum;

	if (!strcmp(name, "0"))
		return 0;

	playernum = (SINT8)atoi(name);

	if (playernum < 0 || playernum >= MAXPLAYERS)
		return -1;

	if (playernum)
	{
		if (playeringame[playernum])
			return (SINT8)playernum;
		else
			return -1;
	}

	for (INT32 i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && !stricmp(player_names[i], name))
			return (SINT8)i;

	CONS_Printf(M_GetText("There is no player named \"%s\"\n"), name);

	return -1;
}

// Is there a game running?
boolean Playing(void)
{
	return (server && serverrunning) || (client && cl_mode == CL_CONNECTED);
}

/** Returns the number of players playing.
  * \return Number of players. Can be zero if we're running a ::dedicated
  *         server.
  * \author Graue <graue@oceanbase.org>
  */
INT32 D_NumPlayers(void)
{
	INT32 num = 0;
	for (INT32 ix = 0; ix < MAXPLAYERS; ix++)
		if (playeringame[ix])
			num++;
	return num;
}

/** Returns the number of currently-connected nodes in a netgame.
  * Not necessarily equivalent to D_NumPlayers() minus D_NumBots().
  *
  * \param skiphost Skip the server's own node.
  */
INT32 D_NumNodes(boolean skiphost)
{
	INT32 num = 0;
	for (INT32 ix = skiphost ? 1 : 0; ix < MAXNETNODES; ix++)
		if (netnodes[ix].ingame)
			num++;
	return num;
}

/** Similar to the above, but counts only bots.
  * Purpose is to remove bots from both the player count and the
  * max player count on the server view
*/
INT32 D_NumBots(void)
{
	INT32 num = 0, ix;
	for (ix = 0; ix < MAXPLAYERS; ix++)
		if (playeringame[ix] && players[ix].bot)
			num++;
	return num;
}


//
// Consistancy
//
// Note: It is called consistAncy on purpose.
//
static void D_MD5ToText(const UINT8 *md5, char text[33])
{
	static const char hex[] = "0123456789abcdef";
	INT32 i;

	for (i = 0; i < 16; i++)
	{
		text[i * 2] = hex[md5[i] >> 4];
		text[i * 2 + 1] = hex[md5[i] & 15];
	}
	text[32] = '\0';
}

static UINT64 D_ConsistencyFingerprintMix(UINT64 hash, UINT32 value)
{
	/* FNV-1a is used only as a compact diagnostic fingerprint. It never feeds
	 * gameplay or the protocol checksum, so collecting it cannot create a new
	 * source of nondeterminism. */
	hash ^= value;
	return hash * 1099511628211ULL;
}

static void D_WriteLiveStateFingerprint(FILE *report)
{
	UINT32 *typecounts;
	UINT64 *typefolds;
	UINT64 mobjfold = 1469598103934665603ULL;
	UINT64 sectorfold = 1469598103934665603ULL;
	UINT32 mobjcount = 0;
	UINT32 removingcount = 0;
	thinker_t *th;
	size_t i;

	if (!report || gamestate != GS_LEVEL)
		return;

	typecounts = calloc(NUMMOBJTYPES, sizeof (*typecounts));
	typefolds = calloc(NUMMOBJTYPES, sizeof (*typefolds));
	if (!typecounts || !typefolds)
	{
		free(typecounts);
		free(typefolds);
		fprintf(report, "\nlive_state_fingerprint=unavailable (out of memory)\n");
		return;
	}

	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		mobj_t *mo;
		UINT64 objectfold = 1469598103934665603ULL;
		UINT32 state = UINT32_MAX;

		if (th->removing)
		{
			removingcount++;
			continue;
		}
		mo = (mobj_t *)th;
		if ((UINT32)mo->type >= NUMMOBJTYPES)
			continue;
		if (mo->state >= states && mo->state < states + NUMSTATES)
			state = (UINT32)(mo->state - states);

#define MIX_MOBJ_FIELD(v) objectfold = D_ConsistencyFingerprintMix(objectfold, (UINT32)(v))
		MIX_MOBJ_FIELD(mo->mobjnum);
		MIX_MOBJ_FIELD(mo->type);
		MIX_MOBJ_FIELD(state);
		MIX_MOBJ_FIELD(mo->tics);
		MIX_MOBJ_FIELD(mo->x);
		MIX_MOBJ_FIELD(mo->y);
		MIX_MOBJ_FIELD(mo->z);
		MIX_MOBJ_FIELD(mo->momx);
		MIX_MOBJ_FIELD(mo->momy);
		MIX_MOBJ_FIELD(mo->momz);
		MIX_MOBJ_FIELD(mo->angle);
		MIX_MOBJ_FIELD(mo->health);
		MIX_MOBJ_FIELD(mo->flags);
		MIX_MOBJ_FIELD(mo->flags2);
		MIX_MOBJ_FIELD(mo->eflags);
		MIX_MOBJ_FIELD(mo->target && !P_MobjWasRemoved(mo->target)
			? mo->target->mobjnum : 0);
		MIX_MOBJ_FIELD(mo->tracer && !P_MobjWasRemoved(mo->tracer)
			? mo->tracer->mobjnum : 0);
#undef MIX_MOBJ_FIELD

		typecounts[mo->type]++;
		typefolds[mo->type] = D_ConsistencyFingerprintMix(
			typefolds[mo->type] ? typefolds[mo->type] : 1469598103934665603ULL,
			(UINT32)objectfold);
		typefolds[mo->type] = D_ConsistencyFingerprintMix(typefolds[mo->type],
			(UINT32)(objectfold >> 32));
		mobjfold = D_ConsistencyFingerprintMix(mobjfold, (UINT32)objectfold);
		mobjfold = D_ConsistencyFingerprintMix(mobjfold, (UINT32)(objectfold >> 32));
		mobjcount++;
	}

	for (i = 0; i < numsectors; i++)
	{
		sectorfold = D_ConsistencyFingerprintMix(sectorfold, sectors[i].floorheight);
		sectorfold = D_ConsistencyFingerprintMix(sectorfold, sectors[i].ceilingheight);
		sectorfold = D_ConsistencyFingerprintMix(sectorfold, sectors[i].lightlevel);
		sectorfold = D_ConsistencyFingerprintMix(sectorfold, sectors[i].special);
		sectorfold = D_ConsistencyFingerprintMix(sectorfold, sectors[i].crumblestate);
		sectorfold = D_ConsistencyFingerprintMix(sectorfold, sectors[i].floorspeed);
		sectorfold = D_ConsistencyFingerprintMix(sectorfold, sectors[i].ceilspeed);
	}

	fprintf(report,
		"\nlive_state_fingerprint:\n"
		"fingerprint_tic=%u\nprng_seed=%u\ninitial_prng_seed=%u\n"
		"mobj_count=%u\nremoving_mobj_count=%u\nmobj_fold=%llu\n"
		"sector_count=%llu\nsector_fold=%llu\n",
		(unsigned)gametic, (unsigned)P_GetRandSeed(), (unsigned)P_GetInitSeed(),
		(unsigned)mobjcount, (unsigned)removingcount,
		(unsigned long long)mobjfold, (unsigned long long)numsectors,
		(unsigned long long)sectorfold);
	fprintf(report, "mobj_type_fingerprints (compare the same type on host/client):\n");
	for (i = 0; i < NUMMOBJTYPES; i++)
		if (typecounts[i])
			fprintf(report, "type=%u count=%u fold=%llu\n", (unsigned)i,
				(unsigned)typecounts[i], (unsigned long long)typefolds[i]);

	free(typecounts);
	free(typefolds);
}

void D_WriteJoinStateReport(const char *event, INT32 playernum, SINT8 node,
	UINT8 attempt)
{
	char reportdir[MAX_WADPATH];
	char reportpath[MAX_WADPATH * 2];
	time_t now = time(NULL);
	FILE *report;
	INT32 i;
	UINT16 reliable_retries = 0;
	UINT16 reliable_backlog = 0;

	if (!event)
		event = "unknown";

	snprintf(reportdir, sizeof reportdir, "%s" PATHSEP "diagnostics", srb2home);
	reportdir[sizeof reportdir - 1] = '\0';
	I_mkdir(reportdir, 0755);
	snprintf(reportpath, sizeof reportpath,
		"%s" PATHSEP "join-state-incidents-v1.txt", reportdir);
	reportpath[sizeof reportpath - 1] = '\0';

	report = fopen(reportpath, "a");
	if (!report)
	{
		CONS_Alert(CONS_WARNING, "Could not write join-state diagnostics to %s\n",
			reportpath);
		return;
	}

	if (node >= 0 && node < MAXNETNODES)
		reliable_backlog = Net_GetReliableBacklog(node, &reliable_retries);

	fprintf(report, "\n=== join-state incident %u ===\n", ++joinstate_report_serial);
	fprintf(report,
		"event=%s\nreport_time=%llu\nside=%s\nversion=%s\nattempt=%u\n",
		event, (unsigned long long)now, server ? "server" : "client",
		VERSIONSTRING, (unsigned)attempt);
	fprintf(report,
		"map=%s\ngamestate=%d\ngametype=%d\ngametic=%u\nleveltime=%u\n"
		"netgame=%d\nmultiplayer=%d\ncl_mode=%d\naddedtogame=%d\n",
		gamemap > 0 ? G_BuildMapName(gamemap) : "none", gamestate, gametype,
		(unsigned)gametic, (unsigned)leveltime, netgame, multiplayer,
		(int)cl_mode, addedtogame);
	fprintf(report,
		"focus_player=%d\nconsoleplayer=%d\ndisplayplayer=%d\nsecondarydisplayplayer=%d\n"
		"mynode=%u\nservernode=%d\nfocus_node=%d\n",
		playernum, consoleplayer, displayplayer, secondarydisplayplayer,
		(unsigned)mynode, servernode, node);
	fprintf(report,
		"desired_name=%s\ndesired_skin=%s\ndesired_color=%d\n"
		"mapping_repair_attempts=%u\nconfig_repair_attempts=%u\n",
		cv_playername.string ? cv_playername.string : "",
		cv_skin.string ? cv_skin.string : "", cv_playercolor.value,
		(unsigned)joinstate_mapping_attempts,
		(unsigned)joinstate_config_attempts);

	if (node >= 0 && node < MAXNETNODES)
	{
		fprintf(report,
			"node_ingame=%d\nnode_player=%d\nnode_player2=%d\n"
			"node_tic=%u\nnode_supposed_tic=%u\n"
			"node_reliable_backlog=%u\nnode_reliable_max_retries=%u\n",
			netnodes[node].ingame, netnodes[node].player,
			netnodes[node].player2, (unsigned)netnodes[node].tic,
			(unsigned)netnodes[node].supposedtic,
			(unsigned)reliable_backlog, (unsigned)reliable_retries);
	}

	fprintf(report, "players:\n");
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player;
		const char *skinname;
		boolean ownerok;

		if (!playeringame[i] && i != playernum && i != consoleplayer)
			continue;
		player = &players[i];
		skinname = (player->skin < numskins && skins[player->skin])
			? skins[player->skin]->name : "invalid";
		ownerok = !player->mo || player->mo->player == player;
		fprintf(report,
			"player=%d ingame=%d node=%u name=%s skin=%s(%u) color=%u "
			"state=%u spectator=%d bot=%d muted=%d jointime=%u quittime=%u "
			"has_mobj=%d mobj_owner_ok=%d\n",
			i, playeringame[i], (unsigned)playernode[i], player_names[i],
			skinname, (unsigned)player->skin, (unsigned)player->skincolor,
			(unsigned)player->playerstate, player->spectator, player->bot,
			player->muted, (unsigned)player->jointime,
			(unsigned)player->quittime, player->mo != NULL, ownerok);
	}
	fprintf(report,
		"note=Server mute state is recorded for diagnosis only; automatic repair never overrides a legitimate mute.\n");
	fflush(report);
	fclose(report);
}

void D_WriteConsistencyReport(const char *trigger, tic_t tic, SINT8 node,
	INT16 expected, INT16 received)
{
	char reportdir[MAX_WADPATH];
	char reportpath[MAX_WADPATH * 2];
	time_t now = time(NULL);
	FILE *report;
	INT32 playernum = -1;
	INT32 i;
	INT32 local_lua_count = 0;
	INT32 received_match_count = 0;
	tic_t received_match_tics[16];
	consistency_snapshot_t *focus = NULL;
	size_t totalmemory = 0;
	size_t freememory = I_GetFreeMem(&totalmemory);

	if (!trigger)
		trigger = "unknown";
	if (node >= 0 && node < MAXNETNODES)
		playernum = netnodes[node].player;
	for (i = 0; i < numwadfiles; i++)
		if (wadfiles[i] && !wadfiles[i]->important && wadfiles[i]->type == RET_LUA)
			local_lua_count++;

	snprintf(reportdir, sizeof reportdir, "%s" PATHSEP "diagnostics", srb2home);
	reportdir[sizeof reportdir - 1] = '\0';
	I_mkdir(reportdir, 0755);
	snprintf(reportpath, sizeof reportpath,
		"%s" PATHSEP "resync-%s-%llu-tic%u-%u.txt", reportdir, trigger,
		(unsigned long long)now, (unsigned)tic, ++consistency_report_serial);
	reportpath[sizeof reportpath - 1] = '\0';

	report = fopen(reportpath, "w");
	if (!report)
	{
		CONS_Alert(CONS_WARNING, "Could not write resync diagnostics to %s\n", reportpath);
		return;
	}

	fprintf(report, "SRB2 optimized consistency/resync report\n");
	fprintf(report, "trigger=%s\nside=%s\nversion=%s\nprocess_bits=%u\n",
		trigger, server ? "server" : "client", VERSIONSTRING,
		(unsigned)(sizeof(void *) * 8));
	fprintf(report, "report_time=%llu\nmap=%s\ngametype=%d\ngamestate=%d\n",
		(unsigned long long)now, G_BuildMapName(gamemap), gametype, gamestate);
	fprintf(report, "reported_tic=%u\ngametic=%u\nleveltime=%u\nmaketic=%u\nneededtic=%u\n",
		(unsigned)tic, (unsigned)gametic, (unsigned)leveltime,
		(unsigned)maketic, (unsigned)neededtic);
	fprintf(report, "node=%d\nplayer=%d\nplayer_name=%s\nexpected=%hd\nreceived=%hd\n",
		node, playernum,
		(playernum >= 0 && playernum < MAXPLAYERS) ? player_names[playernum] : "unknown",
		expected, received);
	fprintf(report,
		"mismatch_confirmation_policy=2 distinct samples within 1 second\n"
		"reported_tic_age=%u\n",
		(unsigned)(gametic >= tic ? gametic - tic : 0));
	fprintf(report, "likely_cause=%s\n",
		local_lua_count ? "gameplay state diverged; local bypass Lua is relevant only if it changed synchronized state"
		: modifiedgame ? "gameplay add-on/script or engine state diverged between peers"
		: "base-game engine state diverged between peers");
	fprintf(report, "packet_loss_assessment=%s\n",
		(lostpercent > 5.0f || gamelostpercent > 5.0f)
		? "high loss can delay commands/recovery, but does not by itself create a checksum mismatch"
		: "loss is not high enough to explain repeated recovery delays");
	fprintf(report, "renderer=%d\nresolution=%dx%d\nmodifiedgame=%d\nnetgame=%d\nmultiplayer=%d\n",
		rendermode, vid.width, vid.height, modifiedgame, netgame, multiplayer);
	/* Search the complete retained checksum history, not only the small excerpt
	 * printed below. A match on another tic identifies a delayed/mis-associated
	 * sample; no match is stronger evidence of an actual simulation split. */
	for (i = 0; i < BACKUPTICS; i++)
	{
		consistency_snapshot_t *snapshot = &consistency_snapshots[i];
		if (!snapshot->valid || (INT16)snapshot->aggregate != received)
			continue;
		if (received_match_count < (INT32)(sizeof received_match_tics / sizeof received_match_tics[0]))
			received_match_tics[received_match_count] = snapshot->tic;
		received_match_count++;
	}
	fprintf(report, "received_checksum_matches_in_server_history=%d\n", received_match_count);
	for (i = 0; i < min(received_match_count,
		(INT32)(sizeof received_match_tics / sizeof received_match_tics[0])); i++)
	{
		const INT64 delta = (INT64)received_match_tics[i] - (INT64)tic;
		fprintf(report, "received_checksum_match_%d_tic=%u delta_from_reported=%lld\n",
			i, (unsigned)received_match_tics[i], (long long)delta);
	}
	fprintf(report,
		"network_get_bps=%d\nnetwork_send_bps=%d\nsystem_packet_loss_percent=%.3f\n"
		"game_tic_loss_percent=%.3f\nconnected_nodes=%d\njoin_or_leave_pause=%d\n"
		"netlowlatency_mode=%d\nconfigured_tic_buffer=%d\neffective_tic_buffer=%d\n"
		"local_ping_ms=%u\neffective_buffer_delay_ms=%.3f\nclient_movement_prediction=0\n"
		"adaptive_stalls=%u\nadaptive_recoveries=%u\nadaptive_recovery_active=%d\n",
		getbps, sendbps, lostpercent, gamelostpercent, D_NumNodes(false), hu_stopped,
		cv_netlowlatency.value, cv_netticbuffer.value, D_EffectiveNetTicBuffer(),
		(consoleplayer >= 0 && consoleplayer < MAXPLAYERS && playeringame[consoleplayer])
			? (unsigned)playerpingtable[consoleplayer] : 0,
		D_EffectiveNetTicBuffer() * (1000.0 / TICRATE),
		netlatency_stalls, netlatency_recoveries,
		netlatency_recovery_until > I_GetTime());
	fprintf(report, "system_memory_free=%llu\nsystem_memory_total=%llu\n",
		(unsigned long long)freememory, (unsigned long long)totalmemory);
	if (playernum >= 0 && playernum < MAXPLAYERS)
		fprintf(report, "player_ping_tics=%u\n", (unsigned)playerpingtable[playernum]);
	if (node >= 0 && node < MAXNETNODES)
	{
		UINT16 reliable_retries = 0;
		UINT16 reliable_backlog = Net_GetReliableBacklog(node, &reliable_retries);
		fprintf(report,
			"node_ingame=%d\nnode_tic=%u\nnode_supposed_tic=%u\n"
			"node_resending_savegame=%d\nnode_resend_cooldown=%u\n"
			"node_reliable_backlog=%u\nnode_reliable_max_retries=%u\n",
			netnodes[node].ingame, (unsigned)netnodes[node].tic,
			(unsigned)netnodes[node].supposedtic, netnodes[node].resendingsavegame,
			(unsigned)netnodes[node].savegameresendcooldown,
			(unsigned)reliable_backlog, (unsigned)reliable_retries);
	}

	fprintf(report, "\nprobable_cause_guide:\n");
	fprintf(report, "- This report was emitted only after two distinct mismatching samples; a single transient sample is ignored.\n");
	fprintf(report, "- Different or hot-reloaded gameplay add-ons/scripts.\n");
	fprintf(report, "- Bypass Lua is local by design and is only a cause if it directly changes synchronized gameplay state.\n");
	fprintf(report, "- Nondeterministic Lua or an unsynchronized gameplay RNG call.\n");
	fprintf(report, "- An engine/add-on state bug, uninitialized data, or memory corruption.\n");
	fprintf(report, "- Packet loss normally causes delay/retry, not a checksum mismatch by itself.\n");
	fprintf(report, "- High loss plus low node tic availability can explain repeated/slow resync delivery.\n");
	fprintf(report, "- OpenGL versus Software rendering should not alter synchronized gameplay.\n");

	fprintf(report, "\nnode_transport_state:\n");
	for (i = 0; i < MAXNETNODES; i++)
	{
		UINT16 reliable_retries;
		UINT16 reliable_backlog;
		if (!netnodes[i].ingame && i != node)
			continue;
		reliable_backlog = Net_GetReliableBacklog(i, &reliable_retries);
		fprintf(report,
			"node=%d ingame=%d player=%d player2=%d tic=%u supposed=%u resending=%d cooldown=%u reliable_backlog=%u reliable_max_retries=%u\n",
			i, netnodes[i].ingame, netnodes[i].player, netnodes[i].player2,
			(unsigned)netnodes[i].tic, (unsigned)netnodes[i].supposedtic,
			netnodes[i].resendingsavegame, (unsigned)netnodes[i].savegameresendcooldown,
			(unsigned)reliable_backlog, (unsigned)reliable_retries);
	}

	fprintf(report, "\nrecent_local_snapshots (newest first):\n");
	for (i = 0; i < min(BACKUPTICS, 16); i++)
	{
		consistency_snapshot_t *snapshot;
		tic_t snap_tic;

		if (tic < (tic_t)i)
			break;
		snap_tic = tic - (tic_t)i;
		snapshot = &consistency_snapshots[snap_tic % BACKUPTICS];
		if (!snapshot->valid || snapshot->tic != snap_tic)
			continue;
		if (!focus)
			focus = snapshot;
		fprintf(report,
			"tic=%u leveltime=%u checksum=%u playerfold=%u rng_used=%d rng=%u\n",
			(unsigned)snapshot->tic, (unsigned)snapshot->leveltic,
			snapshot->aggregate, snapshot->playerfold,
			snapshot->usesrng, snapshot->rngseed);
	}

	if (focus)
	{
		fprintf(report, "\nfocus_snapshot_players (tic=%u):\n", (unsigned)focus->tic);
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!focus->player[i].ingame)
				continue;
			fprintf(report,
				"player=%d name=%s skin=%s(%u) state=%u ability=%u "
				"has_mobj=%d x=%d y=%d z=%d momx=%d momy=%d momz=%d "
				"rings=%d shield=%d pflags=%d fold_after=%u\n", i, player_names[i],
				(focus->player[i].skin < numskins)
					? skins[focus->player[i].skin]->name : "unknown",
				(unsigned)focus->player[i].skin,
				(unsigned)focus->player[i].playerstate,
				(unsigned)focus->player[i].charability,
				focus->player[i].hasmobj, focus->player[i].x,
				focus->player[i].y, focus->player[i].z,
				focus->player[i].momx, focus->player[i].momy,
				focus->player[i].momz, focus->player[i].rings,
				focus->player[i].shield, focus->player[i].pflags,
				focus->player[i].fold_after);
		}
	}

	D_WriteLiveStateFingerprint(report);

	fprintf(report, "\nloaded_resources=%u:\n", numwadfiles);
	for (i = 0; i < numwadfiles; i++)
	{
		char md5text[33];
		wadfile_t *wad = wadfiles[i];

		if (!wad)
			continue;
		D_MD5ToText(wad->md5sum, md5text);
		fprintf(report, "index=%d type=%d important=%d size=%u md5=%s file=%s\n",
			i, wad->type, wad->important, wad->filesize, md5text,
			wad->filename ? wad->filename : "unknown");
	}

	fprintf(report, "\nlocal_unsynchronized_lua:\n");
	{
		for (i = 0; i < numwadfiles; i++)
		{
			wadfile_t *wad = wadfiles[i];
			if (!wad || wad->important || wad->type != RET_LUA)
				continue;
			fprintf(report, "- %s\n", wad->filename ? wad->filename : "unknown");
		}
		if (!local_lua_count)
			fprintf(report, "none\n");
		else
		{
			fprintf(report,
				"note=These bypass scripts are intentionally local and are not verified "
				"or transferred to peers. Their presence alone does not identify the cause. "
				"A consistency failure is possible only if one directly changes synchronized "
				"gameplay state outside a synchronized command.\n");
		}
	}

	fclose(report);
	CONS_Alert(CONS_WARNING, "Resync diagnostics saved to %s\n", reportpath);
}

INT16 Consistancy(void)
{
	UINT32 ret = 0;
	consistency_snapshot_t *snapshot = &consistency_snapshots[gametic%BACKUPTICS];
#ifdef MOBJCONSISTANCY
	thinker_t *th;
	mobj_t *mo;
#endif

	memset(snapshot, 0, sizeof (*snapshot));
	snapshot->valid = true;
	snapshot->tic = gametic;
	snapshot->leveltic = leveltime;

	DEBFILE(va("TIC %u ", gametic));

	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		snapshot->player[i].ingame = playeringame[i];
		if (!playeringame[i])
			ret ^= 0xCCCC;
		else
		{
			snapshot->player[i].skin = players[i].skin;
			snapshot->player[i].playerstate = (UINT8)players[i].playerstate;
			snapshot->player[i].charability = players[i].charability;
			snapshot->player[i].pflags = players[i].pflags;
			snapshot->player[i].rings = players[i].rings;
			snapshot->player[i].shield = players[i].powers[pw_shield];
			if (players[i].mo)
			{
				snapshot->player[i].hasmobj = true;
				snapshot->player[i].x = players[i].mo->x;
				snapshot->player[i].y = players[i].mo->y;
				snapshot->player[i].z = players[i].mo->z;
				snapshot->player[i].momx = players[i].mo->momx;
				snapshot->player[i].momy = players[i].mo->momy;
				snapshot->player[i].momz = players[i].mo->momz;
				ret += players[i].mo->x;
				ret -= players[i].mo->y;
				ret += players[i].powers[pw_shield];
				ret *= i+1;
			}
		}
		snapshot->player[i].fold_after = ret;
	}
	snapshot->playerfold = ret;
	// I give up
	// Coop desynching enemies is painful
	if (!G_PlatformGametype())
	{
		snapshot->usesrng = true;
		snapshot->rngseed = P_GetRandSeed();
		ret += snapshot->rngseed;
	}

#ifdef MOBJCONSISTANCY
	if (gamestate == GS_LEVEL)
	{
		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->removing)
				continue;

			mo = (mobj_t *)th;

			if (mo->flags & (MF_SPECIAL | MF_SOLID | MF_PUSHABLE | MF_BOSS | MF_MISSILE | MF_SPRING | MF_MONITOR | MF_FIRE | MF_ENEMY | MF_PAIN | MF_STICKY))
			{
				ret -= mo->type;
				ret += mo->x;
				ret -= mo->y;
				ret += mo->z;
				ret -= mo->momx;
				ret += mo->momy;
				ret -= mo->momz;
				ret += mo->angle;
				ret -= mo->flags;
				ret += mo->flags2;
				ret -= mo->eflags;
				if (mo->target)
				{
					ret += mo->target->type;
					ret -= mo->target->x;
					ret += mo->target->y;
					ret -= mo->target->z;
					ret += mo->target->momx;
					ret -= mo->target->momy;
					ret += mo->target->momz;
					ret -= mo->target->angle;
					ret += mo->target->flags;
					ret -= mo->target->flags2;
					ret += mo->target->eflags;
					ret -= mo->target->state - states;
					ret += mo->target->tics;
					ret -= mo->target->sprite;
					ret += mo->target->frame;
				}
				else
					ret ^= 0x3333;
				if (mo->tracer && mo->tracer->type != MT_OVERLAY)
				{
					ret += mo->tracer->type;
					ret -= mo->tracer->x;
					ret += mo->tracer->y;
					ret -= mo->tracer->z;
					ret += mo->tracer->momx;
					ret -= mo->tracer->momy;
					ret += mo->tracer->momz;
					ret -= mo->tracer->angle;
					ret += mo->tracer->flags;
					ret -= mo->tracer->flags2;
					ret += mo->tracer->eflags;
					ret -= mo->tracer->state - states;
					ret += mo->tracer->tics;
					ret -= mo->tracer->sprite;
					ret += mo->tracer->frame;
				}
				else
					ret ^= 0xAAAA;
				ret -= mo->state - states;
				ret += mo->tics;
				ret -= mo->sprite;
				ret += mo->frame;
			}
		}
	}
#endif

	DEBFILE(va("Consistancy = %u\n", (ret & 0xFFFF)));
	snapshot->aggregate = (UINT16)(ret & 0xFFFF);

	return (INT16)(ret & 0xFFFF);
}

tic_t GetLag(INT32 node)
{
	return gametic - netnodes[node].tic;
}

void D_MD5PasswordPass(const UINT8 *buffer, size_t len, const char *salt, void *dest)
{
#ifdef NOMD5
	(void)buffer;
	(void)len;
	(void)salt;
	memset(dest, 0, 16);
#else
	char tmpbuf[256];
	const size_t sl = strlen(salt);

	if (len > 256-sl)
		len = 256-sl;

	memcpy(tmpbuf, buffer, len);
	memmove(&tmpbuf[len], salt, sl);
	//strcpy(&tmpbuf[len], salt);
	len += strlen(salt);
	if (len < 256)
		memset(&tmpbuf[len],0,256-len);

	// Yes, we intentionally md5 the ENTIRE buffer regardless of size...
	md5_buffer(tmpbuf, 256, dest);
#endif
}
