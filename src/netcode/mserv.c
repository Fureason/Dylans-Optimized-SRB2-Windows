// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
// Copyright (C) 2020-2023 by James R.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  mserv.c
/// \brief Commands used to communicate with the master server

#if !defined (UNDER_CE)
#include <time.h>
#endif

#include "../doomstat.h"
#include "../doomdef.h"
#include "../command.h"
#include "../m_argv.h"
#include "../i_threads.h"
#include "../i_system.h"
#include "../i_time.h"
#include "mserv.h"
#include "i_tcp.h"
#include "client_connection.h"
#include "d_clisrv.h"
#include "i_net.h"
#include "../g_game.h"
#include "../m_menu.h"
#include "../z_zone.h"

#ifdef MASTERSERVER

static int     MSId;
static int     MSRegisteredId = -1;

static boolean MSRegistered;
static boolean MSInProgress;
static boolean MSUpdateAgain;
static UINT8   MSNoticeFlags;

#define MSNOTICE_REGISTERED 0x01
#define MSNOTICE_UNLISTED   0x02

static time_t  MSLastPing;
static time_t  MSLastReply;
static INT32   MSResponseCode = -1;

static time_t host_public_query_time;
static tic_t host_status_due;
#ifdef HAVE_THREADS
static UINT32 host_status_session;
#endif

#ifdef HAVE_THREADS
static I_mutex MSMutex;
static I_cond  MSCond;

#  define Lock_state()   I_lock_mutex  (&MSMutex)
#  define Unlock_state() I_unlock_mutex (MSMutex)
#else/*HAVE_THREADS*/
#  define Lock_state()
#  define Unlock_state()
#endif/*HAVE_THREADS*/

static void Command_Listserv_f(void);
static void Command_HostStatus_f(void);

#endif/*MASTERSERVER*/

static boolean ServerName_CanChange (const char*);

static void Update_parameters (void);

static void MasterServer_OnChange(void);
static void RoomId_OnChange(void);

static CV_PossibleValue_t masterserver_update_rate_cons_t[] = {
	{2,  "MIN"},
	{60, "MAX"},
	{0,NULL}
};

#define DEFAULT_IPV6_SETTING "Off"




consvar_t cv_ipv6 = CVAR_INIT ("ipv6", DEFAULT_IPV6_SETTING, CV_SAVE, CV_OnOff, NULL);




static consvar_t cv_ipv6_default_off_applied = CVAR_INIT (
	"ipv6_default_off_applied", "Off", CV_SAVE, CV_OnOff, NULL);

void Net_ApplyIPv6Default(void)
{
	if (!cv_ipv6_default_off_applied.value)
	{
		CV_StealthSetValue(&cv_ipv6, 0);
		CV_StealthSetValue(&cv_ipv6_default_off_applied, 1);
		CONS_Printf("IPv6 networking now defaults to Off; it can be enabled in Server Options.\n");
	}
}

boolean Net_UseIPv6(void)
{
#ifdef NO_IPV6
	return false;
#else
	return cv_ipv6.value && !M_CheckParm("-noipv6");
#endif
}

consvar_t cv_masterserver = CVAR_INIT ("masterserver", "https://ds.ms.srb2.org/MS/0", CV_SAVE|CV_CALL, NULL, MasterServer_OnChange);
consvar_t cv_servername = CVAR_INIT_WITH_CALLBACKS ("servername", "SRB2 server", CV_SAVE|CV_NETVAR|CV_CALL|CV_NOINIT|CV_ALLOWLUA, NULL, Update_parameters, ServerName_CanChange);

consvar_t cv_masterserver_update_rate = CVAR_INIT ("masterserver_update_rate", "15", CV_SAVE|CV_CALL|CV_NOINIT, masterserver_update_rate_cons_t, Update_parameters);
CV_PossibleValue_t cv_masterserver_room_values[] = {{-1, "MIN"}, {999999999, "MAX"}, {0, NULL}};
consvar_t cv_masterserver_room_id = CVAR_INIT ("masterserver_room_id", "-1", CV_CALL, cv_masterserver_room_values, RoomId_OnChange);

static INT16 ms_RoomId = -1;

#if defined (MASTERSERVER) && defined (HAVE_THREADS)
int           ms_QueryId;
I_mutex       ms_QueryId_mutex;

msg_server_t *ms_ServerList;
I_mutex       ms_ServerList_mutex;
#endif

UINT16 current_port = 0;

// Room list is an external variable now.
// Avoiding having to get info ten thousand times...
msg_rooms_t room_list[NUM_LIST_ROOMS+1]; // +1 for easy test

/** Adds variables and commands relating to the master server.
  *
  * \sa cv_masterserver, cv_servername,
  *     Command_Listserv_f
  */
void AddMServCommands(void)
{
	CV_RegisterVar(&cv_masterserver);
	CV_RegisterVar(&cv_masterserver_update_rate);
	CV_RegisterVar(&cv_masterserver_room_id);
	CV_RegisterVar(&cv_masterserver_timeout);
	CV_RegisterVar(&cv_masterserver_debug);
	CV_RegisterVar(&cv_masterserver_token);
	CV_RegisterVar(&cv_ipv6);
	CV_RegisterVar(&cv_ipv6_default_off_applied);
	CV_RegisterVar(&cv_servername);
#ifdef MASTERSERVER
	COM_AddCommand("listserv", Command_Listserv_f, 0);
	COM_AddCommand("hoststatus", Command_HostStatus_f, COM_LUA);
	COM_AddCommand("masterserver_update", Update_parameters, COM_LUA); // allows people to updates manually in case you were delisted by accident
#endif
}

#ifdef MASTERSERVER

static void WarnGUI (void)
{
#ifdef HAVE_THREADS
	I_lock_mutex(&m_menu_mutex);
#endif
	M_StartMessage(M_GetText("There was a problem connecting to\nthe Master Server\n\nCheck the console for details.\n"), NULL, MM_NOTHING);
#ifdef HAVE_THREADS
	I_unlock_mutex(m_menu_mutex);
#endif
}

#define NUM_LIST_SERVER MAXSERVERLIST
msg_server_t *GetShortServersList(INT32 room, int id)
{
	msg_server_t *server_list;

	// +1 for easy test
	server_list = malloc(( NUM_LIST_SERVER + 1 ) * sizeof *server_list);

	if (HMS_fetch_servers(server_list, room, id))
		return server_list;
	else
	{
		free(server_list);
		WarnGUI();
		return NULL;
	}
}

INT32 GetRoomsList(boolean hosting, int id)
{
	if (HMS_fetch_rooms( ! hosting, id))
		return 1;
	else
	{
		boolean current = true;
#ifdef HAVE_THREADS


		I_lock_mutex(&ms_QueryId_mutex);
		current = (id == ms_QueryId);
		I_unlock_mutex(ms_QueryId_mutex);
#endif
		if (current)
			WarnGUI();
		return -1;
	}
}

#ifdef UPDATE_ALERT
char *GetMODVersion(int id)
{
	static boolean checked_current_version;
	char *buffer;
	int c;
	boolean current_version;




	if (checked_current_version)
		return NULL;

	buffer = malloc(16);

	c = HMS_compare_mod_version(buffer, 16, id);
	current_version = (c < 0);

#ifdef HAVE_THREADS
	I_lock_mutex(&ms_QueryId_mutex);
	{
		if (id != ms_QueryId)
			c = -1;
	}
	I_unlock_mutex(ms_QueryId_mutex);
#endif

	if (c > 0)
		return buffer;
	else
	{
		if (current_version)
			checked_current_version = true;
		free(buffer);

		if (! c)
			WarnGUI();

		return NULL;
	}
}

// Console only version of the above (used before game init)
void GetMODVersion_Console(void)
{
	char buffer[16];

	if (HMS_compare_mod_version(buffer, sizeof buffer, -1) > 0)
		I_Error(UPDATE_ALERT_STRING_CONSOLE, VERSIONSTRING, buffer);
}
#endif

/** Gets a list of game servers. Called from console.
  */
static void Command_Listserv_f(void)
{
	CONS_Printf(M_GetText("Retrieving server list...\n"));

	{
		HMS_list_servers();
	}
}

void MS_ResetHostProbe(void)
{
	host_public_query_time = 0;
	host_status_due = dedicated ? 0 : I_GetTime() + 12 * TICRATE;
#ifdef HAVE_THREADS
	host_status_session++;
#endif
}

void MS_RecordHostProbe(INT32 node)
{
	if (I_IsPublicNodeAddress(node))
		host_public_query_time = time(NULL);
}

static INT32 Host_public_connection_count(void)
{
	boolean seen[MAXNETNODES] = {false};
	INT32 count = 0;

	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		const UINT8 node = playernode[i];
		if (!playeringame[i] || node == 0 || node >= MAXNETNODES || seen[node])
			continue;

		seen[node] = true;
		if (I_IsPublicNodeAddress(node))
			count++;
	}
	return count;
}

static void Host_collect_status(i_host_status_t *status)
{
	status->port = I_GetListeningServerPort();
	status->room = cv_masterserver_room_id.value;
	status->router_status = I_GetUPnPStatus();
	strlcpy(status->router_method, I_GetPortMappingMethod(), sizeof status->router_method);
	status->public_players = Host_public_connection_count();
	status->public_query_time = (INT64)host_public_query_time;

	Lock_state();
	status->master_pending = MSRegisteredId != MSId || MSInProgress;
	status->master_registered = MSRegisteredId == MSId && MSRegistered;
	status->master_response = MSRegisteredId == MSId ? MSResponseCode : -1;
	status->master_reply_time = (INT64)MSLastReply;
	Unlock_state();
}

#ifdef HAVE_THREADS
typedef struct
{
	char name[MAXSERVERNAME];
	INT32 room;
	UINT16 port;
	UINT32 session;
	boolean companion;
	boolean replied;
	msg_server_t listings[MAXSERVERLIST + 1];
} host_status_query_t;

static boolean host_status_check_running;
static host_status_query_t *host_status_completed;

static void Host_status_thread(host_status_query_t *query)
{

	query->replied = HMS_fetch_servers(query->listings, query->room, -1) != NULL;

	Lock_state();
	host_status_completed = query;
	Unlock_state();
}

static void Flush_host_status_check(void)
{
	host_status_query_t *query;
	UINT16 port;
	INT32 matches = 0;

	Lock_state();
	query = host_status_completed;
	host_status_completed = NULL;
	Unlock_state();
	if (!query)
		return;

	host_status_check_running = false;
	port = query->companion ? M_CompanionServerPort() : I_GetListeningServerPort();
	if (port == query->port && query->session == host_status_session
	&& stricmp(query->name, cv_servername.string) == 0)
	{
		if (query->replied)
		{
			CONS_Printf("The master directory replied.\n");
			for (INT32 i = 0; i < MAXSERVERLIST && query->listings[i].header.buffer[0]; i++)
			{
				const msg_server_t *entry = &query->listings[i];
				if (entry->room != query->room || atoi(entry->port) != query->port
				|| stricmp(entry->name, query->name) != 0)
					continue;

				if (strchr(entry->ip, ':'))
					CONS_Printf("Directory match for this name and port [%s]:%s\n", entry->ip, entry->port);
				else
					CONS_Printf("Directory match for this name and port %s:%s\n", entry->ip, entry->port);
				matches++;
			}
			if (!matches)
				CONS_Printf("The directory has no entry with this name and port yet.\n");
		}
		else
			CONS_Printf("Could not get a server list from the master directory. Run hoststatus to retry.\n");
	}

	free(query);
}
#endif

static void Host_print_status(const i_host_status_t *status)
{
	const INT64 now = (INT64)time(NULL);
	const boolean unlisted = status->room <= 0;

	CONS_Printf("UDP %u is listening locally.\n", (unsigned)status->port);
	switch (status->router_status)
	{
		case I_UPNP_MAPPED:
			CONS_Printf("The router reports a UDP mapping via %s.\n", status->router_method);
			break;
		case I_UPNP_FAILED:
			CONS_Printf("Automatic router mapping failed. A manual forwarding rule can still work.\n");
			break;
		case I_UPNP_UNAVAILABLE:
		case I_UPNP_DISABLED:
			CONS_Printf("Automatic router mapping is off or unavailable.\n");
			break;
		default:
			CONS_Printf("Automatic router mapping is still checking.\n");
			break;
	}

	if (status->public_players > 0)
		CONS_Printf("Internet reachability verified by %d connected public connection%s.\n",
			status->public_players, status->public_players == 1 ? "" : "s");
	else if (status->public_query_time > 0 && now >= status->public_query_time
		&& now - status->public_query_time <= 120)
		CONS_Printf("A public SRB2 browser query reached UDP %u %lld seconds ago.\n",
			(unsigned)status->port, (long long)(now - status->public_query_time));
	else if (unlisted)
		CONS_Printf("Internet reachability is unverified. Test a direct join from another network.\n");
	else
		CONS_Printf("Internet reachability is unverified. Have someone on a different network look up or join the server, then run hoststatus again.\n");

	if (unlisted)
		CONS_Printf("This server is unlisted and hidden from the public server list.\n"
			"Players can join directly using your public IP address and port.\n");
	else if (status->master_registered)
	{
		const INT64 age = max(now - status->master_reply_time, 0);
		CONS_Printf("The master server accepted the host's registration or update (HTTP %d, %lld seconds ago).\n",
			status->master_response, (long long)age);
	}
	else if (status->master_pending || status->master_response < 0)
		CONS_Printf("Waiting for the master server's registration reply.\n");
	else if (status->master_response > 0)
		CONS_Printf("The master server replied with HTTP %d. Registration was unsuccessful.\n",
			status->master_response);
	else
		CONS_Printf("The registration request received no HTTP reply from the master server.\n");
}

static void Command_HostStatus_f(void)
{
	i_host_status_t status;
	const boolean companion = I_CompanionServerRunning();

	if (COM_Argc() > 1)
	{
		CONS_Printf("Usage: hoststatus\n");
		return;
	}

	CONS_Printf("\n\x82--- Hosting status ---\n");
	if (companion)
	{
#if defined(_WIN32) && defined(HAVE_SDL)
		if (!I_ReadCompanionHostStatus(&status))
#endif
		{
			CONS_Printf("Waiting for the local dedicated server to report its status.\n");
			return;
		}
	}
	else
	{
		if (!serverrunning || !server || !netgame)
		{
			CONS_Printf("This client is not currently hosting a network game.\n");
			return;
		}
		Host_collect_status(&status);
		I_ValidateUPnPMapping();
	}

	if (!status.port)
	{
		CONS_Printf("The host has no listening UDP socket.\n");
		return;
	}

	Host_print_status(&status);
	if (status.room <= 0)
		return;

#ifdef HAVE_THREADS
	if (host_status_check_running)
		CONS_Printf("The directory check is already running.\n");
	else
	{
		host_status_query_t *query = calloc(1, sizeof *query);
		if (!query)
			return;

		strlcpy(query->name, cv_servername.string, sizeof query->name);
		query->room = status.room;
		query->port = status.port;
		query->session = host_status_session;
		query->companion = companion;
		host_status_check_running = true;
		CONS_Printf("Checking the master directory for the server name and UDP port...\n");
		I_spawn_thread("host-status", (I_thread_fn)Host_status_thread, query);
	}
#endif
}

static void
Finish_registration (void)
{
	INT32 response;
	const int registered = HMS_register(&response);

	Lock_state();
	{
		MSRegistered = registered;
		MSRegisteredId = MSId;
		MSResponseCode = response;
		time(&MSLastPing);
		if (registered)
		{
			MSNoticeFlags |= MSNOTICE_REGISTERED;
			MSLastReply = MSLastPing;
		}
	}
	Unlock_state();
}

static void
Finish_update (void)
{
	int registered;
	int done;

	Lock_state();
	{
		registered = MSRegistered;
		MSUpdateAgain = false;/* this will happen anyway */
	}
	Unlock_state();

	if (registered)
	{
		INT32 response;
		if (HMS_update(&response))
		{
			Lock_state();
			{
				time(&MSLastPing);
				MSLastReply = MSLastPing;
				MSResponseCode = response;
				MSRegistered = true;
			}
			Unlock_state();

			if (cv_masterserver_debug.value)
				CONS_Printf("Updated master server listing.\n");
		}
		else
			Finish_registration();
	}
	else
		Finish_registration();

	Lock_state();
	{
		done = ! MSUpdateAgain;

		if (done)
			MSInProgress = false;
	}
	Unlock_state();

	if (! done)
		Finish_update();
}

static void
Finish_unlist (void)
{
	int registered;

	Lock_state();
	{
		registered = MSRegistered;
	}
	Unlock_state();

	if (registered)
	{
		if (HMS_unlist())
		{
			Lock_state();
			MSNoticeFlags |= MSNOTICE_UNLISTED;
			Unlock_state();
		}

		Lock_state();
		{
			MSRegistered = false;
		}
		Unlock_state();

#ifdef HAVE_THREADS
		I_wake_all_cond(&MSCond);
#endif
	}

	Lock_state();
	{
		if (MSId == MSRegisteredId)
			MSId++;
	}
	Unlock_state();
}

#ifdef HAVE_THREADS
static int *
Server_id (void)
{
	int *id;
	id = malloc(sizeof *id);
	Lock_state();
	{
		*id = MSId;
	}
	Unlock_state();
	return id;
}

static int *
New_server_id (void)
{
	int *id;
	id = malloc(sizeof *id);
	Lock_state();
	{
		*id = ++MSId;
		I_wake_all_cond(&MSCond);
	}
	Unlock_state();
	return id;
}

static void
Register_server_thread (int *id)
{
	int same;

	Lock_state();
	{
		/* wait for previous unlist to finish */
		while (*id == MSId && MSRegistered)
			I_hold_cond(&MSCond, MSMutex);

		same = ( *id == MSId );/* it could have been a while */
	}
	Unlock_state();

	if (same)/* it could have been a while */
		Finish_registration();

	free(id);
}

static void
Update_server_thread (int *id)
{
	int same;

	Lock_state();
	{
		same = ( *id == MSRegisteredId );
	}
	Unlock_state();

	if (same)
		Finish_update();

	free(id);
}

static void
Unlist_server_thread (int *id)
{
	int same;

	Lock_state();
	{
		same = ( *id == MSRegisteredId );
	}
	Unlock_state();

	if (same)
		Finish_unlist();

	free(id);
}

static void
Change_masterserver_thread (char *api)
{
	Lock_state();
	{
		while (MSRegistered)
			I_hold_cond(&MSCond, MSMutex);
	}
	Unlock_state();

	HMS_set_api(api);
}
#endif/*HAVE_THREADS*/

void RegisterServer(void)
{
#ifdef MASTERSERVER
	CONS_Printf("Registering this server on the master server...\n");
#ifdef HAVE_THREADS
	I_spawn_thread(
			"register-server",
			(I_thread_fn)Register_server_thread,
			New_server_id()
	);
#else
	Finish_registration();
#endif
#endif/*MASTERSERVER*/
}

static void UpdateServer(void)
{
#ifdef HAVE_THREADS
	I_spawn_thread(
			"update-server",
			(I_thread_fn)Update_server_thread,
			Server_id()
	);
#else
	Finish_update();
#endif
}

void UnregisterServer(void)
{
#ifdef MASTERSERVER
#ifdef HAVE_THREADS
	I_spawn_thread(
			"unlist-server",
			(I_thread_fn)Unlist_server_thread,
			Server_id()
	);
#else
	Finish_unlist();
#endif
#endif/*MASTERSERVER*/
}

static boolean
Online (void)
{
	return I_GetListeningServerPort() && cv_masterserver_room_id.value > 0;
}

static inline void SendPingToMasterServer(void)
{
	int ready;
	time_t now;

	if (Online())
	{
		time(&now);

		Lock_state();
		{
			ready = (
					MSRegisteredId == MSId &&
					! MSInProgress &&
					now >= ( MSLastPing + 60 * cv_masterserver_update_rate.value )
			);

			if (ready)
				MSInProgress = true;
		}
		Unlock_state();

		if (ready)
			UpdateServer();
	}
}

static void Flush_masterserver_notices(void)
{
	UINT8 notices;

	Lock_state();
	{
		notices = MSNoticeFlags;
		MSNoticeFlags = 0;
	}
	Unlock_state();



	if (notices & MSNOTICE_REGISTERED)
		CONS_Printf("Master server registration successful. Run hoststatus for the port and reachability checks.\n");
	if (notices & MSNOTICE_UNLISTED)
		CONS_Printf("Server deregistration request successfully sent.\n");
}

void MasterClient_Ticker(void)
{
#ifdef MASTERSERVER
	Flush_masterserver_notices();
	SendPingToMasterServer();
#ifdef HAVE_THREADS
	Flush_host_status_check();
#endif

	if (host_status_due && I_GetTime() >= host_status_due)
	{
		host_status_due = 0;
		if (I_GetListeningServerPort())
			COM_BufAddText("hoststatus\n");
	}

#if defined(_WIN32) && defined(HAVE_SDL)
	if (dedicated && serverrunning)
	{
		static tic_t next_report;
		const tic_t now = I_GetTime();
		if (now >= next_report)
		{
			i_host_status_t status;
			next_report = now + TICRATE;
			Host_collect_status(&status);
			I_PublishCompanionHostStatus(&status);
		}
	}
#endif
#endif
}

static void
Set_api (const char *api)
{
#ifdef HAVE_THREADS
	I_spawn_thread(
			"change-masterserver",
			(I_thread_fn)Change_masterserver_thread,
			strdup(api)
	);
#else
	HMS_set_api(strdup(api));
#endif
}

#endif/*MASTERSERVER*/

static boolean ServerName_CanChange(const char* newvalue)
{
	if (strlen(newvalue) < MAXSERVERNAME)
		return true;

	CONS_Alert(CONS_NOTICE, "The server name must be shorter than %d characters\n", MAXSERVERNAME);
	return false;
}

static void
Update_parameters (void)
{
#ifdef MASTERSERVER
	int registered = 0;
	int delayed;

	if (Online())
	{
		Lock_state();
		{
			delayed = MSInProgress;

			if (delayed)/* do another update after the current one */
				MSUpdateAgain = true;
			else
				registered = MSRegistered;
		}
		Unlock_state();

		if (! delayed && registered)
			UpdateServer();
	}
#endif/*MASTERSERVER*/
}

static void RoomId_OnChange(void)
{
	if (ms_RoomId != cv_masterserver_room_id.value)
	{
		UnregisterServer();
		ms_RoomId = cv_masterserver_room_id.value;
		if (Online())
			RegisterServer();
	}
}

static void MasterServer_OnChange(void)
{
#ifdef MASTERSERVER
	UnregisterServer();

	/*
	TODO: remove this for v2, it's just a hack
	for those coming in with an old config.
	*/
	if (
			! cv_masterserver.changed &&
			strcmp(cv_masterserver.string, "ms.srb2.org:28900") == 0
	){
		CV_StealthSet(&cv_masterserver, cv_masterserver.defaultvalue);
	}

	if (
			! cv_masterserver.changed &&
			strcmp(cv_masterserver.string, "https://mb.srb2.org/MS/0") == 0
	){
		CV_StealthSet(&cv_masterserver, cv_masterserver.defaultvalue);
	}

	Set_api(cv_masterserver.string);

	if (Online())
		RegisterServer();
#endif/*MASTERSERVER*/
}
