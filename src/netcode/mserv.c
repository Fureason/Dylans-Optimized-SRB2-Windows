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



#if !defined (UNDER_CE)
#include <time.h>
#endif

#include "../doomstat.h"
#include "../doomdef.h"
#include "../command.h"
#include "../m_argv.h"
#include "../i_threads.h"
#include "../i_system.h"
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

#ifdef HAVE_THREADS
static I_mutex MSMutex;
static I_cond  MSCond;

#  define Lock_state()   I_lock_mutex  (&MSMutex)
#  define Unlock_state() I_unlock_mutex (MSMutex)
#else 
#  define Lock_state()
#  define Unlock_state()
#endif 

static void Command_Listserv_f(void);
static void Command_HostStatus_f(void);

#endif 

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



msg_rooms_t room_list[NUM_LIST_ROOMS+1];  

                                                              
   
                                       
                          
    
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
	COM_AddCommand("masterserver_update", Update_parameters, COM_LUA);  
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


void GetMODVersion_Console(void)
{
	char buffer[16];

	if (HMS_compare_mod_version(buffer, sizeof buffer, -1) > 0)
		I_Error(UPDATE_ALERT_STRING_CONSOLE, VERSIONSTRING, buffer);
}
#endif

                                                     
    
static void Command_Listserv_f(void)
{
	CONS_Printf(M_GetText("Retrieving server list...\n"));

	{
		HMS_list_servers();
	}
}

typedef struct
{
	char name[MAXSERVERNAME];
	INT32 room;
	UINT16 port;
	boolean require_port;
} host_status_query_t;

static boolean Host_address_is_public(const char *address)
{
	unsigned int a, b, c, d;
	const char *ipv6 = address;

	if (!address || !address[0])
		return false;
	if (sscanf(address, "%u.%u.%u.%u", &a, &b, &c, &d) == 4
	&& a <= 255 && b <= 255 && c <= 255 && d <= 255)
	{
		if (a == 0 || a == 10 || a == 127 || a >= 224)
			return false;
		if (a == 169 && b == 254)
			return false;
		if (a == 172 && b >= 16 && b <= 31)
			return false;
		if (a == 192 && b == 168)
			return false;
		return true;
	}
	if (*ipv6 == '[')
		ipv6++;
	if (!strnicmp(ipv6, "::1", 3) || !strnicmp(ipv6, "fe80:", 5)
	|| !strnicmp(ipv6, "fc", 2) || !strnicmp(ipv6, "fd", 2))
		return false;
	return strchr(ipv6, ':') != NULL;
}

static INT32 Host_public_connection_count(void)
{
	boolean seen[MAXNETNODES];
	INT32 count = 0;
	INT32 i;

	memset(seen, 0, sizeof seen);
	if (!I_GetNodeAddress)
		return 0;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		const UINT8 node = playernode[i];
		const char *address;
		if (!playeringame[i] || node == UINT8_MAX || node == 0
		|| node >= MAXNETNODES || seen[node])
			continue;
		seen[node] = true;
		address = I_GetNodeAddress(node);
		if (Host_address_is_public(address))
			count++;
	}
	return count;
}

#ifdef HAVE_THREADS
static void Host_status_thread(host_status_query_t *query)
{
	msg_server_t *list;
	INT32 i;
	INT32 matches = 0;

	list = calloc(MAXSERVERLIST + 1, sizeof *list);
	if (!list)
	{
		CONS_Alert(CONS_ERROR, "HOST STATUS: Not enough memory to check the public list.\n");
		free(query);
		return;
	}

	                                                                          
                                        
	if (!HMS_fetch_servers(list, query->room, -1))
	{
		CONS_Alert(CONS_WARNING,
			"HOST STATUS: The master-server list could not be checked. Try again in a few seconds.\n");
		free(list);
		free(query);
		return;
	}

	for (i = 0; i < MAXSERVERLIST && list[i].header.buffer[0]; i++)
	{
		if (stricmp(list[i].name, query->name) != 0)
			continue;
		if (query->require_port && atoi(list[i].port) != query->port)
			continue;

		CONS_Printf("\x83HOST STATUS: REGISTERED in the master directory as \"%s\" at %s:%s (room %d).\n",
			list[i].name, list[i].ip, list[i].port, list[i].room);
		matches++;
	}

	if (!matches)
	{
		CONS_Alert(CONS_WARNING,
			"HOST STATUS: NOT LISTED as \"%s\" on UDP %u in room %d. Registration may still be starting; wait a few seconds and run hoststatus again.\n",
			query->name, (unsigned)query->port, query->room);
	}
	else
	{
		CONS_Printf("HOST STATUS: Master-directory registration is confirmed. This is not proof that unsolicited UDP reaches the PC; an outside player provides that proof. The website may take about two minutes to publish the server.\n");
	}

	free(list);
	free(query);
}
#endif

static void Command_HostStatus_f(void)
{
	boolean registered;
	boolean registering;
	const boolean companion = I_CompanionServerRunning();
	const UINT16 hostport = companion ? M_CompanionServerPort() : current_port;
	host_status_query_t *query;

	if (COM_Argc() > 1)
	{
		CONS_Printf("Usage: hoststatus\n");
		return;
	}

	Lock_state();
	registered = MSRegistered;
	registering = MSInProgress;
	Unlock_state();

	CONS_Printf("\n\x82--- Hosting status ---\n");
	CONS_Printf("Windows Network Discovery/file sharing: NOT REQUIRED for SRB2 internet hosting.\n");
	if (serverrunning || companion)
	{
		const INT32 publicconnections = Host_public_connection_count();
		CONS_Printf("Host process: ACTIVE (%s server)\n",
			companion ? "local dedicated companion" :
			(dedicated ? "dedicated" : "listen"));
		CONS_Printf("UDP socket: listening on port %u\n", (unsigned)hostport);
		if (publicconnections > 0)
			CONS_Printf("Internet reachability: PROVEN by %d active public-Internet connection%s.\n",
				publicconnections, publicconnections == 1 ? "" : "s");
		else
			CONS_Printf("Internet reachability: UNVERIFIED. No outside player is connected right now; a master listing or router mapping alone is not a port test.\n");
	}
	else
	{
		CONS_Printf("Host process: NOT ACTIVE. This client is not currently hosting.\n");
		return;
	}

	if (cv_masterserver_room_id.value < 0)
	{
		CONS_Printf("Public listing: OFF (Unlisted/LAN mode). Players need a direct address or LAN discovery.\n");
		return;
	}

	CONS_Printf("Master registration: %s\n",
		registered ? "accepted" : (registering ? "in progress" : "not accepted yet"));

#ifdef HAVE_THREADS
	query = calloc(1, sizeof *query);
	if (!query)
	{
		CONS_Alert(CONS_ERROR, "HOST STATUS: Not enough memory to start the visibility check.\n");
		return;
	}
	strlcpy(query->name, cv_servername.string, sizeof query->name);
	query->room = cv_masterserver_room_id.value;
	query->port = hostport;
	query->require_port = query->port != 0;
	CONS_Printf("Master directory: checking registration for \"%s\" on UDP %u...\n",
		query->name, (unsigned)query->port);
	I_spawn_thread("host-status", (I_thread_fn)Host_status_thread, query);
#else
	CONS_Printf("Public listing: live verification needs thread support in this build.\n");
#endif
}

static void
Finish_registration (void)
{
	int registered;

	registered = HMS_register();

	Lock_state();
	{
		MSRegistered = registered;
		MSRegisteredId = MSId;
		if (registered)
			MSNoticeFlags |= MSNOTICE_REGISTERED;

		time(&MSLastPing);
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
		MSUpdateAgain = false; 
	}
	Unlock_state();

	if (registered)
	{
		if (HMS_update())
		{
			Lock_state();
			{
				time(&MSLastPing);
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
		 
		while (*id == MSId && MSRegistered)
			I_hold_cond(&MSCond, MSMutex);

		same = ( *id == MSId ); 
	}
	Unlock_state();

	if (same) 
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
#endif 

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
#endif 
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
#endif 
}

static boolean
Online (void)
{
	return ( serverrunning && cv_masterserver_room_id.value > 0 );
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
		CONS_Printf("Master server registration successful. Run hoststatus to verify public reachability.\n");
	if (notices & MSNOTICE_UNLISTED)
		CONS_Printf("Server deregistration request successfully sent.\n");
}

void MasterClient_Ticker(void)
{
#ifdef MASTERSERVER
	Flush_masterserver_notices();
	SendPingToMasterServer();
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

#endif 

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

			if (delayed) 
				MSUpdateAgain = true;
			else
				registered = MSRegistered;
		}
		Unlock_state();

		if (! delayed && registered)
			UpdateServer();
	}
#endif 
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
#endif 
}
