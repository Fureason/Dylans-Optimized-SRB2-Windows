// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------





#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef __GNUC__
#include <unistd.h>
#endif

#ifndef NO_IPV6
	#define HAVE_IPV6
#endif

#ifdef _WIN32
	#define USE_WINSOCK
	#if defined (_WIN64) || defined (HAVE_IPV6)
		#define USE_WINSOCK2
	#else  
		#define USE_WINSOCK1
	#endif
#endif  

#ifdef USE_WINSOCK2
	#include <ws2tcpip.h>
	#include <iphlpapi.h>
#endif

#ifdef _WIN32
	#include <shellapi.h>
#endif

#include "../doomdef.h"
#include "../z_zone.h"

#ifdef USE_WINSOCK1
	#include <winsock.h>
#else
	#ifndef USE_WINSOCK
		#include <arpa/inet.h>
		#ifdef __APPLE_CC__
			#ifndef _BSD_SOCKLEN_T_
				#define _BSD_SOCKLEN_T_
			#endif  
		#endif  
		#include <sys/socket.h>
		#include <netinet/in.h>
		#include <netdb.h>
		#include <sys/ioctl.h>
	#endif  

	#include <errno.h>
	#include <time.h>

	#if defined (__unix__) || defined (__APPLE__) || defined (UNIXCOMMON)
		#include <sys/time.h>
	#endif  
#endif

#ifdef USE_WINSOCK

	#undef errno

	#define errno h_errno  
	#ifdef EWOULDBLOCK
	#undef EWOULDBLOCK
	#endif
	#define EWOULDBLOCK WSAEWOULDBLOCK
	#ifdef EMSGSIZE
	#undef EMSGSIZE
	#endif
	#define EMSGSIZE WSAEMSGSIZE
	#ifdef ECONNREFUSED
	#undef ECONNREFUSED
	#endif
	#define ECONNREFUSED WSAECONNREFUSED
	#ifdef ETIMEDOUT
	#undef ETIMEDOUT
	#endif
	#define ETIMEDOUT WSAETIMEDOUT
	#ifdef EHOSTUNREACH
	#undef EHOSTUNREACH
	#endif
	#define EHOSTUNREACH WSAEHOSTUNREACH
	#ifdef ENETUNREACH
	#undef ENETUNREACH
	#endif
	#define ENETUNREACH WSAENETUNREACH
	#ifndef IOC_VENDOR
	#define IOC_VENDOR 0x18000000
	#endif
	#ifndef _WSAIOW
	#define _WSAIOW(x,y) (IOC_IN|(x)|(y))
	#endif
	#ifndef SIO_UDP_CONNRESET
	#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
	#endif
	#ifndef AI_ADDRCONFIG
	#define AI_ADDRCONFIG 0x00000400
	#endif
	#ifndef STATUS_INVALID_PARAMETER
	#define STATUS_INVALID_PARAMETER 0xC000000D
	#endif
#endif  

typedef union
{
	struct sockaddr     any;
	struct sockaddr_in  ip4;
#ifdef HAVE_IPV6
	struct sockaddr_in6 ip6;
#endif
} mysockaddr_t;

	#ifdef HAVE_MINIUPNPC
	#include "miniupnpc/miniupnpc.h"
	#include "miniupnpc/upnpcommands.h"
	#include "miniupnpc/upnperrors.h"
	static boolean UPNP_support;
	#endif  

#include "../i_system.h"
#include "../i_time.h"
#include "i_net.h"
#include "d_net.h"
#include "d_netfil.h"
#include "i_tcp.h"
#include "mserv.h"
#include "../m_argv.h"
#include "../i_threads.h"

#include "../doomstat.h"


#ifdef USE_WINSOCK

	#define ioctl ioctlsocket
	#define close closesocket
#endif

#include "i_addrinfo.h"
#define DEFAULTPORT "5029"

#ifdef USE_WINSOCK
	typedef SOCKET SOCKET_TYPE;
	#define ERRSOCKET (SOCKET_ERROR)
#else
	#if defined (__unix__) || defined (__APPLE__) || defined (__HAIKU__)
		typedef int SOCKET_TYPE;
	#else
		typedef unsigned long SOCKET_TYPE;
	#endif
	#define ERRSOCKET (-1)
#endif

#define IPV6_MULTICAST_ADDRESS "ff15::57e1:1a12"


#ifdef USE_WINSOCK1
	typedef int socklen_t;
#endif
static SOCKET_TYPE mysockets[MAXNETNODES+1] = {ERRSOCKET};
static size_t mysocketses = 0;
static int myfamily[MAXNETNODES+1] = {0};
static SOCKET_TYPE nodesocket[MAXNETNODES+1] = {ERRSOCKET};
static mysockaddr_t clientaddress[MAXNETNODES+1];
static mysockaddr_t broadcastaddress[MAXNETNODES+1];
static size_t broadcastaddresses = 0;
static boolean nodeconnected[MAXNETNODES+1];
static mysockaddr_t *banned;
static UINT8 *bannedmask;

static size_t numbans = 0;
static boolean SOCK_bannednode[MAXNETNODES+1];  
static boolean init_tcp_driver = false;



static boolean badaddresswarned[MAXNETNODES+1];
static int lastsenderror[MAXNETNODES+1];
static tic_t lastsenderrorreport[MAXNETNODES+1];

static const char *serverport_name = DEFAULTPORT;
static const char *clientport_name; 

#ifdef USE_WINSOCK

static char *get_WSAErrorStr(int e)
{
	static char buf[256];  

	buf[0] = '\0';

	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		(DWORD)e,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)buf,
		sizeof (buf),
		NULL);

	if (!buf[0])  
		sprintf(buf, "Unknown error");

	return buf;
}
#undef strerror
#define strerror get_WSAErrorStr
#endif

static int SOCK_GetLastErrorCode(void)
{
#ifdef USE_WINSOCK
	return WSAGetLastError();
#else
	return errno;
#endif
}

#ifdef USE_WINSOCK2
#define inet_ntop inet_ntopA
#define HAVE_NTOP
static const char* inet_ntopA(short af, const void *cp, char *buf, socklen_t len)
{
	DWORD Dlen = len, AFlen = 0;
	SOCKADDR_STORAGE any;
	LPSOCKADDR anyp = (LPSOCKADDR)&any;
	LPSOCKADDR_IN any4 = (LPSOCKADDR_IN)&any;
	LPSOCKADDR_IN6 any6 = (LPSOCKADDR_IN6)&any;

	if (!buf)
	{
		WSASetLastError(STATUS_INVALID_PARAMETER);
		return NULL;
	}

	if (af != AF_INET && af != AF_INET6)
	{
		WSASetLastError(WSAEAFNOSUPPORT);
		return NULL;
	}

	ZeroMemory(&any, sizeof(SOCKADDR_STORAGE));
	any.ss_family = af;

	switch (af)
	{
		case AF_INET:
		{
			CopyMemory(&any4->sin_addr, cp, sizeof(IN_ADDR));
			AFlen = sizeof(SOCKADDR_IN);
			break;
		}
		case AF_INET6:
		{
			CopyMemory(&any6->sin6_addr, cp, sizeof(IN6_ADDR));
			AFlen = sizeof(SOCKADDR_IN6);
			break;
		}
	}

	if (WSAAddressToStringA(anyp, AFlen, NULL, buf, &Dlen) == SOCKET_ERROR)
		return NULL;
	return buf;
}
#elif !defined (USE_WINSOCK1)
#define HAVE_NTOP
#endif

#ifdef HAVE_MINIUPNPC  
static void I_ShutdownUPnP(void);
static void I_InitUPnP(void);
static void I_RequestUPnPMapping(const char *port);
static void I_UPnPWorker(void *userdata);
static I_mutex upnp_mutex;
static I_cond upnp_worker_cond;
static struct UPNPUrls urls;
static struct IGDdatas data;
static char lanaddr[64];
static boolean upnp_discovery_started;
static boolean upnp_ready;
static boolean upnp_mapping_requested;
static boolean upnp_mapping_succeeded;
static boolean upnp_mapping_failed;
                                                                             
                                                                              
                                                                              
static boolean upnp_mapping_owned;
static boolean upnp_discovery_failed;
static boolean upnp_exit_registered;
static boolean upnp_worker_running;
static boolean upnp_shutdown_requested;
static UINT8 upnp_pending_actions;
static UINT8 upnp_mapping_method;
static char upnp_mapping_port[16] = DEFAULTPORT;
static char upnp_remove_port[16] = DEFAULTPORT;
static tic_t upnp_retry_after;
static tic_t upnp_retry_delay = 30*TICRATE;
static tic_t upnp_renew_after;
static tic_t upnp_validate_after;
static UINT8 pcp_nonce[12];
static boolean pcp_nonce_valid;

#define EASYHOST_LEASE_SECONDS (2U*60U*60U)
#define EASYHOST_VALIDATE_SECONDS (5U*60U)

enum
{
	UPNP_ACTION_DISCOVER = 1,
	UPNP_ACTION_MAP = 1<<1,
	UPNP_ACTION_REMOVE = 1<<2,
	UPNP_ACTION_VALIDATE = 1<<3
};

enum
{
	UPNP_METHOD_NONE = 0,
	UPNP_METHOD_UPNP,
	UPNP_METHOD_NATPMP,
	UPNP_METHOD_EXISTING_UPNP,
	UPNP_METHOD_PCP
};

typedef enum
{
	UPNP_ENTRY_ERROR = -1,
	UPNP_ENTRY_MISSING = 0,
	UPNP_ENTRY_OURS,
	UPNP_ENTRY_CONFLICT
} upnp_entry_state_t;

                                                                           
                                                                             
                                                                              
                                                          
static upnp_entry_state_t I_UPnP_entry(const char *addr, const char *port,
	char *mappedaddr, size_t mappedaddrsize, char *mappedport,
	size_t mappedportsize)
{
	char intclient[64] = "";
	char intport[16] = "";
	char desc[80] = "";
	char enabled[8] = "";
	char duration[16] = "";
	int result;

	result = UPNP_GetSpecificPortMappingEntry(urls.controlURL,
		data.first.servicetype, port, "UDP", NULL, intclient, intport,
		desc, enabled, duration);
	if (result == 714)
		return UPNP_ENTRY_MISSING;
	if (result != UPNPCOMMAND_SUCCESS)
		return UPNP_ENTRY_ERROR;

	if (mappedaddr && mappedaddrsize)
		strlcpy(mappedaddr, intclient, mappedaddrsize);
	if (mappedport && mappedportsize)
		strlcpy(mappedport, intport, mappedportsize);

	                                                                         
                                                                       
	if (enabled[0] && !atoi(enabled))
		return UPNP_ENTRY_MISSING;
	if (!strcmp(intclient, addr) && atoi(intport) == atoi(port))
		return UPNP_ENTRY_OURS;
	return UPNP_ENTRY_CONFLICT;
}

#if defined(_WIN32) && defined(USE_WINSOCK2)
static UINT16 I_PortMapRead16(const UINT8 *value)
{
	return (UINT16)(((UINT16)value[0] << 8) | value[1]);
}

static UINT32 I_PortMapRead32(const UINT8 *value)
{
	return ((UINT32)value[0] << 24) | ((UINT32)value[1] << 16)
		| ((UINT32)value[2] << 8) | value[3];
}

static void I_PortMapWrite16(UINT8 *value, UINT16 number)
{
	value[0] = (UINT8)(number >> 8);
	value[1] = (UINT8)number;
}

static void I_PortMapWrite32(UINT8 *value, UINT32 number)
{
	value[0] = (UINT8)(number >> 24);
	value[1] = (UINT8)(number >> 16);
	value[2] = (UINT8)(number >> 8);
	value[3] = (UINT8)number;
}

static boolean I_PortMapGateway(struct sockaddr_in *gateway)
{
	MIB_IPFORWARDROW route;

	memset(&route, 0, sizeof route);
	if (GetBestRoute(inet_addr("1.1.1.1"), 0, &route) != NO_ERROR
	|| !route.dwForwardNextHop)
		return false;

	memset(gateway, 0, sizeof *gateway);
	gateway->sin_family = AF_INET;
	gateway->sin_port = htons(5351);
	gateway->sin_addr.s_addr = route.dwForwardNextHop;
	return true;
}

                                                                         
                                                                           
                                                                          
static boolean I_PCPMap(const char *port, UINT32 lifetime,
	UINT32 *granted_lifetime)
{
	struct sockaddr_in gateway;
	struct sockaddr_in local;
	SOCKET_TYPE sock;
	UINT8 request[64] = {0};
	UINT8 response[128];
	const UINT16 internalport = (UINT16)atoi(port);
	const int requestsize = lifetime ? 64 : 60;
	DWORD timeout = 700;
	socklen_t locallen = sizeof local;
	INT32 attempt;
	int received = -1;
	UINT8 result = 0xff;
	UINT32 response_lifetime = 0;

	if (!internalport || !I_PortMapGateway(&gateway))
		return false;
	if (!pcp_nonce_valid)
	{
		if (I_GetRandomBytes((char *)pcp_nonce, sizeof pcp_nonce)
			!= sizeof pcp_nonce)
			return false;
		pcp_nonce_valid = true;
	}

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
		return false;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout,
		sizeof timeout);
	if (connect(sock, (const struct sockaddr *)&gateway, sizeof gateway)
		== SOCKET_ERROR
	|| getsockname(sock, (struct sockaddr *)&local, &locallen) == SOCKET_ERROR)
	{
		close(sock);
		return false;
	}

	request[0] = 2;  
	request[1] = 1;  
	I_PortMapWrite32(&request[4], lifetime);
	 
	request[18] = request[19] = 0xff;
	memcpy(&request[20], &local.sin_addr.s_addr, sizeof local.sin_addr.s_addr);
	memcpy(&request[24], pcp_nonce, sizeof pcp_nonce);
	request[36] = IPPROTO_UDP;
	I_PortMapWrite16(&request[40], internalport);
	I_PortMapWrite16(&request[42], internalport);
	request[54] = request[55] = 0xff;  
	if (lifetime)
		request[60] = 2;  

	for (attempt = 0; attempt < 3; attempt++)
	{
		if (send(sock, (const char *)request, requestsize, 0) == requestsize)
			received = recv(sock, (char *)response, sizeof response, 0);
		if (received >= 60 && response[0] == 2 && response[1] == 0x81
		&& !memcmp(&response[24], pcp_nonce, sizeof pcp_nonce)
		&& response[36] == IPPROTO_UDP
		&& I_PortMapRead16(&response[40]) == internalport)
		{
			result = response[3];
			break;
		}
	}
	close(sock);
	if (result == 0)
		response_lifetime = I_PortMapRead32(&response[4]);

	if (result != 0
	|| (lifetime && (I_PortMapRead16(&response[42]) != internalport
		|| !response_lifetime)))
	{
		if (result != 0xff)
			I_OutputMsg("Easy Host PCP request for UDP port %u was rejected (result %u).\n",
				(unsigned)internalport, (unsigned)result);
		return false;
	}

	if (granted_lifetime)
		*granted_lifetime = response_lifetime;
	I_OutputMsg(lifetime
		? "Easy Host mapped UDP port %u through PCP.\n"
		: "Easy Host removed PCP mapping for UDP port %u.\n",
		(unsigned)internalport);
	return true;
}

static boolean I_NATPMPMap(const char *port, UINT32 lifetime,
	UINT32 *granted_lifetime)
{
	struct sockaddr_in gateway;
	SOCKET_TYPE sock;
	UINT8 request[12] = {0};
	UINT8 response[32];
	const UINT16 internalport = (UINT16)atoi(port);
	DWORD timeout = 700;
	INT32 attempt;
	int received = -1;
	UINT32 response_lifetime;

	if (!internalport || !I_PortMapGateway(&gateway))
		return false;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
		return false;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof timeout);

	request[1] = 1;  
	I_PortMapWrite16(&request[4], internalport);
	I_PortMapWrite16(&request[6], internalport);
	I_PortMapWrite32(&request[8], lifetime);

	for (attempt = 0; attempt < 3; attempt++)
	{
		if (sendto(sock, (const char *)request, sizeof request, 0,
			(const struct sockaddr *)&gateway, sizeof gateway) == sizeof request)
			received = recvfrom(sock, (char *)response, sizeof response, 0, NULL, NULL);
		if (received >= 16)
			break;
	}
	close(sock);

	if (received < 16 || response[0] != 0 || response[1] != 129
	|| response[2] != 0 || response[3] != 0
	|| I_PortMapRead16(&response[8]) != internalport
	|| (lifetime && (I_PortMapRead16(&response[10]) != internalport
		|| !I_PortMapRead32(&response[12]))))
		return false;

	response_lifetime = I_PortMapRead32(&response[12]);
	if (granted_lifetime)
		*granted_lifetime = response_lifetime;
	I_OutputMsg(lifetime
		? "Easy Host mapped UDP port %u through NAT-PMP fallback.\n"
		: "Easy Host removed NAT-PMP mapping for UDP port %u.\n",
		(unsigned)internalport);
	return true;
}
#else
static boolean I_PCPMap(const char *port, UINT32 lifetime,
	UINT32 *granted_lifetime)
{
	(void)port;
	(void)lifetime;
	(void)granted_lifetime;
	return false;
}

static boolean I_NATPMPMap(const char *port, UINT32 lifetime,
	UINT32 *granted_lifetime)
{
	(void)port;
	(void)lifetime;
	(void)granted_lifetime;
	return false;
}
#endif

typedef enum
{
	PORTMAP_FAILED = 0,
	PORTMAP_SUCCEEDED,
	PORTMAP_CONFLICT
} portmap_result_t;

static void I_UPnPQueue(UINT8 actions)
{
	boolean spawn_worker = false;

	if (!actions)
		return;
	I_lock_mutex(&upnp_mutex);
	if (upnp_shutdown_requested && !(actions & UPNP_ACTION_REMOVE))
	{
		I_unlock_mutex(upnp_mutex);
		return;
	}
	upnp_pending_actions |= actions;
	if (!upnp_worker_running)
	{
		upnp_worker_running = true;
		spawn_worker = true;
	}
	I_unlock_mutex(upnp_mutex);

	if (spawn_worker)
		I_spawn_thread("easyhost-portmap", (I_thread_fn)I_UPnPWorker, NULL);
}

static void I_InitUPnP(void)
{
	boolean start_discovery = false;

	I_lock_mutex(&upnp_mutex);
	if (UPNP_support && !upnp_shutdown_requested
	&& !upnp_ready && !upnp_discovery_started)
	{
		upnp_discovery_started = true;
		upnp_discovery_failed = false;
		start_discovery = true;
	}
	I_unlock_mutex(upnp_mutex);

	if (start_discovery)
		I_UPnPQueue(UPNP_ACTION_DISCOVER);
}

static void I_RequestUPnPMapping(const char *port)
{
	boolean ready;
	UINT8 actions = UPNP_ACTION_MAP;

	if (!port || !port[0])
		port = DEFAULTPORT;
	I_lock_mutex(&upnp_mutex);
	if (!UPNP_support || upnp_shutdown_requested)
	{
		I_unlock_mutex(upnp_mutex);
		return;
	}
	strlcpy(upnp_mapping_port, port, sizeof upnp_mapping_port);
	upnp_mapping_requested = true;
	upnp_mapping_failed = false;
	ready = upnp_ready;
	if (!ready)
	{
		upnp_discovery_started = true;
		upnp_discovery_failed = false;
		actions |= UPNP_ACTION_DISCOVER;
	}
	I_unlock_mutex(upnp_mutex);
	I_UPnPQueue(actions);
}

static void I_ReleaseUPnPMapping(void)
{
	boolean release;

	I_lock_mutex(&upnp_mutex);
	release = upnp_mapping_requested || upnp_mapping_owned
		|| upnp_mapping_succeeded;
	strlcpy(upnp_remove_port, upnp_mapping_port, sizeof upnp_remove_port);
	upnp_mapping_requested = false;
	upnp_retry_after = 0;
	upnp_renew_after = 0;
	upnp_validate_after = 0;
	I_unlock_mutex(upnp_mutex);
	if (release)
		I_UPnPQueue(UPNP_ACTION_REMOVE);
}

static boolean I_UPnPDiscoverWorker(void)
{
	const char * const deviceTypes[] = {
		"urn:schemas-upnp-org:device:InternetGatewayDevice:2",
		"urn:schemas-upnp-org:device:InternetGatewayDevice:1",
		0
	};
	struct UPNPDev *devlist = NULL;
	boolean discovered_ready = false;
	int upnp_error = -2;
	int igd_status = 0;
	UINT8 discovery_attempt;
	boolean skip;

	I_lock_mutex(&upnp_mutex);
	skip = upnp_ready || !UPNP_support || upnp_shutdown_requested;
	if (skip)
		upnp_discovery_started = false;
	I_unlock_mutex(upnp_mutex);
	if (skip)
		return false;

	FreeUPNPUrls(&urls);
	memset(&urls, 0, sizeof urls);
	memset(&data, 0, sizeof data);
	memset(lanaddr, 0, sizeof lanaddr);

	I_OutputMsg(M_GetText("Looking for UPnP Internet Gateway Device\n"));
	                                                                         
                                                                            
                                                                        
	for (discovery_attempt = 0; discovery_attempt < 3 && !devlist;
		discovery_attempt++)
	{
		if (discovery_attempt < 2)
			devlist = upnpDiscoverDevices(deviceTypes,
				500 + discovery_attempt * 500, NULL, NULL, 0, false, 2,
				&upnp_error, 0);
		else
			devlist = upnpDiscoverAll(1500, NULL, NULL, 0, false, 2,
				&upnp_error);
		if (!devlist && discovery_attempt < 2)
			I_Sleep(150);
	}

	if (devlist)
	{
		struct UPNPDev *dev = devlist;

		while (dev)
		{
			if (strstr(dev->st, "InternetGatewayDevice"))
				break;
			dev = dev->pNext;
		}
		if (!dev)
			dev = devlist;
		I_OutputMsg(M_GetText("Found UPnP device:\n desc: %s\n st: %s\n"),
			dev->descURL, dev->st);

#if (MINIUPNPC_API_VERSION >= 18)
		igd_status = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof lanaddr,
			NULL, 0);
#else
		igd_status = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof lanaddr);
#endif
		discovered_ready = igd_status > 0 && urls.controlURL
			&& urls.controlURL[0] != '\0' && lanaddr[0] != '\0';
		if (discovered_ready)
			I_OutputMsg(M_GetText("Local LAN IP address: %s\n"), lanaddr);
		freeUPNPDevlist(devlist);
	}
	else
		I_OutputMsg(M_GetText(
			"Easy Host could not discover a UPnP router after 3 attempts (error %d).\n"
			"Windows Network Discovery and a Private network profile are not required. "
			"Easy Host will try PCP and NAT-PMP next. Double NAT/CGNAT may still require a relay or tunnel.\n"),
			upnp_error);

	I_lock_mutex(&upnp_mutex);
	upnp_ready = discovered_ready;
	upnp_discovery_started = false;
	upnp_discovery_failed = !discovered_ready;
	I_unlock_mutex(upnp_mutex);
	return discovered_ready;
}

static void I_UPnPMappingSucceeded(UINT8 method, boolean owned,
	UINT32 granted_lifetime)
{
	UINT32 renew_seconds = 0;

	if ((method == UPNP_METHOD_PCP || method == UPNP_METHOD_NATPMP)
	&& granted_lifetime)
	{
		renew_seconds = granted_lifetime / 2;
		if (renew_seconds < 60)
			renew_seconds = 60;
		if (renew_seconds > 60*60)
			renew_seconds = 60*60;
	}

	I_lock_mutex(&upnp_mutex);
	upnp_mapping_succeeded = true;
	upnp_mapping_failed = false;
	upnp_mapping_owned = owned;
	upnp_mapping_method = method;
	upnp_discovery_failed = false;
	upnp_retry_after = 0;
	upnp_retry_delay = 30*TICRATE;
	upnp_renew_after = renew_seconds
		? I_GetTime() + renew_seconds*TICRATE : 0;
	upnp_validate_after = (method == UPNP_METHOD_UPNP
		|| method == UPNP_METHOD_EXISTING_UPNP)
		? I_GetTime() + EASYHOST_VALIDATE_SECONDS*TICRATE : 0;
	I_unlock_mutex(upnp_mutex);
}

static void I_UPnPMappingFailed(boolean preserve_existing)
{
	I_lock_mutex(&upnp_mutex);
	if (!preserve_existing)
	{
		upnp_mapping_succeeded = false;
		upnp_mapping_owned = false;
		upnp_mapping_method = UPNP_METHOD_NONE;
	}
	upnp_mapping_failed = UPNP_support && upnp_mapping_requested;
	upnp_renew_after = 0;
	upnp_validate_after = 0;
	upnp_retry_after = upnp_mapping_failed
		? I_GetTime() + upnp_retry_delay : 0;
	I_unlock_mutex(upnp_mutex);
}

static portmap_result_t I_UPnPAddWorker(const char *port)
{
	upnp_entry_state_t entrystate;
	char mappedaddr[64] = "";
	char mappedport[16] = "";
	boolean already_owned;
	int result;

	if (!upnp_ready || !urls.controlURL || !urls.controlURL[0])
		return PORTMAP_FAILED;
	entrystate = I_UPnP_entry(lanaddr, port, mappedaddr, sizeof mappedaddr,
		mappedport, sizeof mappedport);
	if (entrystate == UPNP_ENTRY_OURS)
	{
		const int refresh_result = UPNP_AddPortMapping(urls.controlURL,
			data.first.servicetype, port, port, lanaddr, "SRB2 Easy Host",
			"UDP", NULL, NULL);
		I_lock_mutex(&upnp_mutex);
		already_owned = upnp_mapping_owned
			&& upnp_mapping_method == UPNP_METHOD_UPNP;
		I_unlock_mutex(upnp_mutex);
		I_UPnPMappingSucceeded(already_owned ? UPNP_METHOD_UPNP
			: UPNP_METHOD_EXISTING_UPNP, already_owned, 0);
		I_OutputMsg(refresh_result == UPNPCOMMAND_SUCCESS
			? "Easy Host refreshed the existing UDP port %s mapping.\n"
			: "Easy Host is using the existing UDP port %s mapping.\n", port);
		return PORTMAP_SUCCEEDED;
	}
	if (entrystate == UPNP_ENTRY_CONFLICT)
	{
		I_OutputMsg("Easy Host cannot map UDP port %s: the router already sends it to %s:%s. "
			"Close the other host or choose a different server port.\n",
			port, mappedaddr, mappedport);
		return PORTMAP_CONFLICT;
	}

	result = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
		port, port, lanaddr, "SRB2 Easy Host", "UDP", NULL, NULL);
	if (result == UPNPCOMMAND_SUCCESS)
	{
		I_UPnPMappingSucceeded(UPNP_METHOD_UPNP, true, 0);
		I_OutputMsg("Easy Host mapped UDP port %s through UPnP.\n", port);
		return PORTMAP_SUCCEEDED;
	}
	I_OutputMsg("Easy Host could not map UDP port %s through UPnP: %s (%d).\n",
		port, strupnperror(result), result);
	return PORTMAP_FAILED;
}

static void I_UPnPMapWorker(const char *port)
{
	portmap_result_t result = PORTMAP_FAILED;
	UINT8 previous_method;
	boolean previously_mapped;
	boolean ready;
	boolean active;
	boolean pcp_tried = false;
	boolean natpmp_tried = false;
	UINT32 granted_lifetime = 0;

	I_lock_mutex(&upnp_mutex);
	active = UPNP_support && upnp_mapping_requested
		&& !upnp_shutdown_requested;
	previous_method = upnp_mapping_method;
	previously_mapped = upnp_mapping_succeeded;
	ready = upnp_ready;
	I_unlock_mutex(upnp_mutex);
	if (!active)
		return;

	 
	if (previously_mapped && previous_method == UPNP_METHOD_PCP)
	{
		pcp_tried = true;
		if (I_PCPMap(port, EASYHOST_LEASE_SECONDS, &granted_lifetime))
		{
			I_UPnPMappingSucceeded(UPNP_METHOD_PCP, true,
				granted_lifetime);
			return;
		}
	}
	else if (previously_mapped && previous_method == UPNP_METHOD_NATPMP)
	{
		natpmp_tried = true;
		if (I_NATPMPMap(port, EASYHOST_LEASE_SECONDS,
			&granted_lifetime))
		{
			I_UPnPMappingSucceeded(UPNP_METHOD_NATPMP, true,
				granted_lifetime);
			return;
		}
	}

	if (ready)
	{
		result = I_UPnPAddWorker(port);
		if (result == PORTMAP_SUCCEEDED)
			return;
		if (result == PORTMAP_CONFLICT)
		{
			I_UPnPMappingFailed(false);
			return;
		}
	}
	if (!pcp_tried)
	{
		granted_lifetime = 0;
		if (I_PCPMap(port, EASYHOST_LEASE_SECONDS, &granted_lifetime))
		{
			I_UPnPMappingSucceeded(UPNP_METHOD_PCP, true,
				granted_lifetime);
			return;
		}
	}
	if (!natpmp_tried)
	{
		granted_lifetime = 0;
		if (I_NATPMPMap(port, EASYHOST_LEASE_SECONDS,
			&granted_lifetime))
		{
			I_UPnPMappingSucceeded(UPNP_METHOD_NATPMP, true,
				granted_lifetime);
			return;
		}
	}

	I_OutputMsg("Easy Host could not automatically map UDP port %s through UPnP, PCP, or NAT-PMP.\n",
		port);
	I_UPnPMappingFailed(previously_mapped);
}

static void I_UPnPRemoveWorker(const char *port)
{
	UINT8 method;
	boolean owned;
	boolean removed = true;

	I_lock_mutex(&upnp_mutex);
	method = upnp_mapping_method;
	owned = upnp_mapping_owned;
	I_unlock_mutex(upnp_mutex);

	if (owned && method == UPNP_METHOD_PCP)
		removed = I_PCPMap(port, 0, NULL);
	else if (owned && method == UPNP_METHOD_NATPMP)
		removed = I_NATPMPMap(port, 0, NULL);
	else if (owned && method == UPNP_METHOD_UPNP
	&& upnp_ready && urls.controlURL && urls.controlURL[0])
	{
		const upnp_entry_state_t entrystate = I_UPnP_entry(lanaddr, port,
			NULL, 0, NULL, 0);
		if (entrystate == UPNP_ENTRY_OURS)
			removed = UPNP_DeletePortMapping(urls.controlURL,
				data.first.servicetype, port, "UDP", NULL)
				== UPNPCOMMAND_SUCCESS;
	}
	if (owned && !removed)
		I_OutputMsg("Easy Host could not confirm removal of its UDP port %s mapping. Finite leases will expire automatically.\n",
			port);

	I_lock_mutex(&upnp_mutex);
	upnp_mapping_succeeded = false;
	upnp_mapping_failed = false;
	upnp_mapping_owned = false;
	upnp_mapping_method = UPNP_METHOD_NONE;
	upnp_retry_after = 0;
	upnp_renew_after = 0;
	upnp_validate_after = 0;
	I_unlock_mutex(upnp_mutex);
}

static void I_UPnPValidateWorker(const char *port)
{
	upnp_entry_state_t entrystate;
	char mappedaddr[64] = "";
	char mappedport[16] = "";
	UINT8 method;
	boolean mapped;

	I_lock_mutex(&upnp_mutex);
	method = upnp_mapping_method;
	mapped = upnp_mapping_succeeded;
	I_unlock_mutex(upnp_mutex);
	if (!mapped || !upnp_ready
	|| (method != UPNP_METHOD_UPNP
		&& method != UPNP_METHOD_EXISTING_UPNP))
		return;

	entrystate = I_UPnP_entry(lanaddr, port, mappedaddr, sizeof mappedaddr,
		mappedport, sizeof mappedport);
	if (entrystate == UPNP_ENTRY_OURS || entrystate == UPNP_ENTRY_ERROR)
	{
		I_lock_mutex(&upnp_mutex);
		upnp_validate_after = I_GetTime()
			+ EASYHOST_VALIDATE_SECONDS*TICRATE;
		I_unlock_mutex(upnp_mutex);
		return;
	}

	I_lock_mutex(&upnp_mutex);
	upnp_mapping_succeeded = false;
	upnp_mapping_failed = true;
	upnp_mapping_owned = false;
	upnp_mapping_method = UPNP_METHOD_NONE;
	upnp_retry_after = I_GetTime() + 1;
	upnp_validate_after = 0;
	I_unlock_mutex(upnp_mutex);
	if (entrystate == UPNP_ENTRY_CONFLICT)
		I_OutputMsg("Easy Host mapping validation failed: UDP %s now points to %s:%s; automatic recovery will not overwrite it.\n",
			port, mappedaddr, mappedport);
	else
		I_OutputMsg("Easy Host mapping validation failed: the router no longer has the entry. Automatic recovery is scheduled.\n");
}

static void I_UPnPWorker(void *userdata)
{
	(void)userdata;
	for (;;)
	{
		UINT8 actions;
		boolean shutting_down;
		char port[sizeof upnp_mapping_port];
		char removeport[sizeof upnp_remove_port];

		I_lock_mutex(&upnp_mutex);
		actions = upnp_pending_actions;
		upnp_pending_actions = 0;
		shutting_down = upnp_shutdown_requested;
		strlcpy(port, upnp_mapping_port, sizeof port);
		strlcpy(removeport, upnp_remove_port, sizeof removeport);
		if (!actions)
		{
			upnp_worker_running = false;
			I_wake_all_cond(&upnp_worker_cond);
			I_unlock_mutex(upnp_mutex);
			return;
		}
		I_unlock_mutex(upnp_mutex);

		if (actions & UPNP_ACTION_REMOVE)
			I_UPnPRemoveWorker(removeport);
		if (!shutting_down && (actions & UPNP_ACTION_DISCOVER))
			I_UPnPDiscoverWorker();

		I_lock_mutex(&upnp_mutex);
		shutting_down = upnp_shutdown_requested || !UPNP_support;
		I_unlock_mutex(upnp_mutex);
		if (!shutting_down && (actions & UPNP_ACTION_MAP))
			I_UPnPMapWorker(port);
		if (!shutting_down && (actions & UPNP_ACTION_VALIDATE))
			I_UPnPValidateWorker(port);
	}
}

static void I_ShutdownUPnP(void)
{
	boolean spawn_worker = false;

	I_lock_mutex(&upnp_mutex);
	upnp_shutdown_requested = true;
	UPNP_support = false;
	upnp_mapping_requested = false;
	strlcpy(upnp_remove_port, upnp_mapping_port, sizeof upnp_remove_port);
	upnp_pending_actions |= UPNP_ACTION_REMOVE;
	if (!upnp_worker_running)
	{
		upnp_worker_running = true;
		spawn_worker = true;
	}
	I_unlock_mutex(upnp_mutex);
	if (spawn_worker)
		I_spawn_thread("easyhost-shutdown", (I_thread_fn)I_UPnPWorker, NULL);

	                                                                          
                                                                           
	I_lock_mutex(&upnp_mutex);
	while (upnp_worker_running)
		I_hold_cond(&upnp_worker_cond, upnp_mutex);
	FreeUPNPUrls(&urls);
	memset(&urls, 0, sizeof urls);
	memset(&data, 0, sizeof data);
	upnp_ready = false;
	upnp_discovery_started = false;
	upnp_discovery_failed = false;
	upnp_mapping_succeeded = false;
	upnp_mapping_failed = false;
	upnp_mapping_owned = false;
	upnp_mapping_method = UPNP_METHOD_NONE;
	upnp_pending_actions = 0;
	upnp_retry_after = 0;
	upnp_renew_after = 0;
	upnp_validate_after = 0;
	upnp_retry_delay = 30*TICRATE;
	I_unlock_mutex(upnp_mutex);
}
#endif

void I_ConfigureUPnP(boolean enable)
{
#ifdef HAVE_MINIUPNPC
	boolean register_exit = false;

	I_lock_mutex(&upnp_mutex);
	UPNP_support = enable;
	if (enable)
	{
		upnp_shutdown_requested = false;
		if (!upnp_exit_registered)
		{
			upnp_exit_registered = true;
			register_exit = true;
		}
	}
	I_unlock_mutex(upnp_mutex);

	                                                                          
                                                                        
	if (register_exit)
		I_AddExitFunc(I_ShutdownUPnP);
	if (enable)
		I_InitUPnP();
	else
		I_ReleaseUPnPMapping();
#else
	if (enable)
		I_OutputMsg("Automatic port mapping is unavailable in this build.\n");
#endif
}

boolean I_RetryUPnP(void)
{
#ifdef HAVE_MINIUPNPC
	UINT8 actions = UPNP_ACTION_MAP;

	if (!UPNP_support)
		return false;

	I_lock_mutex(&upnp_mutex);
	if (upnp_shutdown_requested)
	{
		I_unlock_mutex(upnp_mutex);
		return false;
	}
	upnp_discovery_failed = false;
	upnp_mapping_failed = false;
	upnp_mapping_requested = true;
	upnp_retry_after = 0;
	upnp_retry_delay = 30*TICRATE;
	if (!upnp_ready)
	{
		upnp_discovery_started = true;
		actions |= UPNP_ACTION_DISCOVER;
	}
	I_unlock_mutex(upnp_mutex);

	I_UPnPQueue(actions);
	return true;
#else
	return false;
#endif
}

static void I_UPnPRetryTicker(void)
{
#ifdef HAVE_MINIUPNPC
	UINT8 actions = 0;
	boolean retrying = false;
	const tic_t now = I_GetTime();

	I_lock_mutex(&upnp_mutex);
	if (UPNP_support && upnp_mapping_requested
	&& (upnp_discovery_failed || upnp_mapping_failed)
	&& upnp_retry_after && now >= upnp_retry_after)
	{
		retrying = true;
		actions |= UPNP_ACTION_MAP;
		if (!upnp_ready)
		{
			upnp_discovery_started = true;
			upnp_discovery_failed = false;
			actions |= UPNP_ACTION_DISCOVER;
		}
		upnp_mapping_failed = false;
		upnp_retry_after = 0;
		upnp_retry_delay = min(upnp_retry_delay * 2, 5*TICRATE*60);
	}
	else if (UPNP_support && upnp_mapping_requested
	&& upnp_mapping_succeeded && upnp_renew_after
	&& now >= upnp_renew_after)
	{
		upnp_renew_after = 0;
		actions |= UPNP_ACTION_MAP;
	}
	else if (UPNP_support && upnp_mapping_requested
	&& upnp_mapping_succeeded && upnp_validate_after
	&& now >= upnp_validate_after)
	{
		upnp_validate_after = 0;
		actions |= UPNP_ACTION_VALIDATE;
	}
	I_unlock_mutex(upnp_mutex);

	if (retrying)
		I_OutputMsg("Easy Host is retrying automatic router discovery/mapping.\n");
	if (actions)
		I_UPnPQueue(actions);
#endif
}

i_upnp_status_t I_GetUPnPStatus(void)
{
#ifdef HAVE_MINIUPNPC
	i_upnp_status_t status;

	I_lock_mutex(&upnp_mutex);
	if (!UPNP_support)
		status = I_UPNP_DISABLED;
	else if (upnp_mapping_succeeded)
		status = I_UPNP_MAPPED;
	else if (upnp_mapping_failed || upnp_discovery_failed)
		status = I_UPNP_FAILED;
	else if (upnp_discovery_started && !upnp_ready)
		status = I_UPNP_DISCOVERING;
	else if (upnp_mapping_requested)
		status = I_UPNP_MAPPING;
	else if (upnp_ready)
		status = I_UPNP_READY;
	else
		status = I_UPNP_DISCOVERING;
	I_unlock_mutex(upnp_mutex);
	return status;
#else
	return I_UPNP_UNAVAILABLE;
#endif
}

void I_ValidateUPnPMapping(void)
{
#ifdef HAVE_MINIUPNPC
	boolean validate;

	I_lock_mutex(&upnp_mutex);
	validate = UPNP_support && upnp_mapping_requested
		&& upnp_mapping_succeeded && upnp_ready
		&& (upnp_mapping_method == UPNP_METHOD_UPNP
			|| upnp_mapping_method == UPNP_METHOD_EXISTING_UPNP);
	I_unlock_mutex(upnp_mutex);
	if (validate)
		I_UPnPQueue(UPNP_ACTION_VALIDATE);
#endif
}

const char *I_GetPortMappingMethod(void)
{
#ifdef HAVE_MINIUPNPC
	UINT8 method;
	I_lock_mutex(&upnp_mutex);
	method = upnp_mapping_method;
	I_unlock_mutex(upnp_mutex);
	if (method == UPNP_METHOD_UPNP)
		return "UPnP";
	if (method == UPNP_METHOD_NATPMP)
		return "NAT-PMP";
	if (method == UPNP_METHOD_EXISTING_UPNP)
		return "UPnP (existing mapping)";
	if (method == UPNP_METHOD_PCP)
		return "PCP";
#endif
	return "none";
}

static boolean I_EnsureWindowsFirewallRule(const char *port)
{
#ifdef _WIN32
	char executable[MAX_PATH];
	char rulename[128];
	char parameters[2048];
	unsigned long portvalue;
	UINT32 pathhash = 2166136261U;
	const unsigned char *cursor;
	SHELLEXECUTEINFOA check;
	DWORD exitcode = 1;

	if (!port || !port[0])
		port = DEFAULTPORT;
	portvalue = strtoul(port, NULL, 10);
	if (!portvalue || portvalue > 65535)
		return false;
	if (!GetModuleFileNameA(NULL, executable, sizeof executable))
		return false;
	                                                                         
                                                                       
                                                                         
                                                                   
	for (cursor = (const unsigned char *)executable; *cursor; cursor++)
	{
		unsigned char value = *cursor;
		if (value >= 'A' && value <= 'Z')
			value = (unsigned char)(value + ('a' - 'A'));
		pathhash ^= value;
		pathhash *= 16777619U;
	}
	snprintf(rulename, sizeof rulename,
		"SRB2 Optimized Easy Host UDP %lu %08x", portvalue,
		(unsigned)pathhash);
	rulename[sizeof rulename - 1] = '\0';

	                                                                        
                                                                               
	memset(&check, 0, sizeof check);
	check.cbSize = sizeof check;
	check.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
	check.lpVerb = "open";
	check.lpFile = "netsh.exe";
	snprintf(parameters, sizeof parameters,
		"advfirewall firewall show rule name=\"%s\"", rulename);
	parameters[sizeof parameters - 1] = '\0';
	check.lpParameters = parameters;
	check.nShow = SW_HIDE;
	if (ShellExecuteExA(&check))
	{
		if (WaitForSingleObject(check.hProcess, 5000) == WAIT_OBJECT_0)
			GetExitCodeProcess(check.hProcess, &exitcode);
		CloseHandle(check.hProcess);
		if (exitcode == 0)
			return true;
	}

	                                                                          
                                                                         
                                                                        
	snprintf(parameters, sizeof parameters,
		"advfirewall firewall add rule name=\"%s\" "
		"dir=in action=allow protocol=UDP localport=%lu profile=any "
		"program=\"%s\" enable=yes",
		rulename, portvalue, executable);
	parameters[sizeof parameters - 1] = '\0';
	memset(&check, 0, sizeof check);
	check.cbSize = sizeof check;
	check.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
	check.lpVerb = "runas";
	check.lpFile = "netsh.exe";
	check.lpParameters = parameters;
	check.nShow = SW_HIDE;
	if (!ShellExecuteExA(&check))
		return false;
	                                                                         
                                                                                
	if (WaitForSingleObject(check.hProcess, 15000) != WAIT_OBJECT_0)
	{
		CloseHandle(check.hProcess);
		return false;
	}
	GetExitCodeProcess(check.hProcess, &exitcode);
	CloseHandle(check.hProcess);
	return exitcode == 0;
#else
	(void)port;
	return false;
#endif
}

#ifdef _WIN32
typedef struct
{
	char port[16];
} firewall_request_t;

static void I_WindowsFirewallRuleWorker(void *userdata)
{
	firewall_request_t *request = (firewall_request_t *)userdata;
	const boolean confirmed = I_EnsureWindowsFirewallRule(request->port);

	if (confirmed)
		I_OutputMsg("Easy Dedicated Host: Windows firewall rule is ready for UDP port %s.\n",
			request->port);
	else
		I_OutputMsg(
			"Easy Dedicated Host: the Windows firewall rule was not confirmed for UDP port %s. "
			"The server can keep running, but outside players may be blocked.\n",
			request->port);
	free(request);
}
#endif

boolean I_RequestWindowsFirewallRule(const char *port)
{
#ifdef _WIN32
	firewall_request_t *request;
	unsigned long portvalue;

	if (!port || !port[0])
		port = DEFAULTPORT;
	portvalue = strtoul(port, NULL, 10);
	if (!portvalue || portvalue > 65535)
		return false;
	request = (firewall_request_t *)malloc(sizeof *request);
	if (!request)
		return false;
	snprintf(request->port, sizeof request->port, "%lu", portvalue);
	request->port[sizeof request->port - 1] = '\0';
	I_spawn_thread("easyhost-firewall",
		(I_thread_fn)I_WindowsFirewallRuleWorker, request);
	return true;
#else
	(void)port;
	return false;
#endif
}

static const char *SOCK_AddrToStr(mysockaddr_t *sk)
{
	static char s[64];  

#ifdef HAVE_NTOP
#ifdef HAVE_IPV6
	int v6 = (sk->any.sa_family == AF_INET6);
#else
	int v6 = 0;
#endif
	void *addr;

	if(sk->any.sa_family == AF_INET)
		addr = &sk->ip4.sin_addr;
#ifdef HAVE_IPV6
	else if(sk->any.sa_family == AF_INET6)
		addr = &sk->ip6.sin6_addr;
#endif
	else
		addr = NULL;

	if(addr == NULL)
		sprintf(s, "No address");
	else if(inet_ntop(sk->any.sa_family, addr, &s[v6], sizeof (s) - v6) == NULL)
		sprintf(s, "Unknown family type, error #%u", SOCK_GetLastErrorCode());
#ifdef HAVE_IPV6
	else if(sk->any.sa_family == AF_INET6)
	{
		s[0] = '[';
		strcat(s, "]");

		if (sk->ip6.sin6_port != 0)
			strcat(s, va(":%d", ntohs(sk->ip6.sin6_port)));
	}
#endif
	else if(sk->any.sa_family == AF_INET  && sk->ip4.sin_port  != 0)
		strcat(s, va(":%d", ntohs(sk->ip4.sin_port)));

#else
	if (sk->any.sa_family == AF_INET)
	{
		strcpy(s, inet_ntoa(sk->ip4.sin_addr));
		if (sk->ip4.sin_port != 0) strcat(s, va(":%d", ntohs(sk->ip4.sin_port)));
	}
	else
		sprintf(s, "Unknown type");
#endif
	return s;
}

static const char *SOCK_GetNodeAddress(INT32 node)
{
	if (node < 0 || node > MAXNETNODES)
		return NULL;
	if (node == 0)
		return "self";
	if (!nodeconnected[node])
		return NULL;
	return SOCK_AddrToStr(&clientaddress[node]);
}

static const char *SOCK_GetBanAddress(size_t ban)
{
	if (ban >= numbans)
		return NULL;
	return SOCK_AddrToStr(&banned[ban]);
}

static const char *SOCK_GetBanMask(size_t ban)
{
	static char s[16];  
	if (ban >= numbans)
		return NULL;
	if (sprintf(s,"%d",bannedmask[ban]) > 0)
		return s;
	return NULL;
}

#ifdef HAVE_IPV6
static boolean SOCK_cmpipv6(mysockaddr_t *a, mysockaddr_t *b, UINT8 mask)
{
	UINT8 bitmask;
	I_Assert(mask <= 128);
	if (memcmp(&a->ip6.sin6_addr.s6_addr, &b->ip6.sin6_addr.s6_addr, mask / 8) != 0)
		return false;
	if (mask % 8 == 0)
		return true;
	bitmask = 255 << (mask % 8);
	return (a->ip6.sin6_addr.s6_addr[mask / 8] & bitmask) == (b->ip6.sin6_addr.s6_addr[mask / 8] & bitmask);
}
#endif

static boolean SOCK_cmpaddr(mysockaddr_t *a, mysockaddr_t *b, UINT8 mask)
{
	UINT32 bitmask = INADDR_NONE;

	if (a->any.sa_family != b->any.sa_family)
		return false;

	if (mask && mask < 32)
		bitmask = htonl((UINT32)(-1) << (32 - mask));

	if (b->any.sa_family == AF_INET)
		return (a->ip4.sin_addr.s_addr & bitmask) == (b->ip4.sin_addr.s_addr & bitmask)
			&& (b->ip4.sin_port == 0 || (a->ip4.sin_port == b->ip4.sin_port));
#ifdef HAVE_IPV6
	else if (b->any.sa_family == AF_INET6)
		return SOCK_cmpipv6(a, b, mask)
			&& (b->ip6.sin6_port == 0 || (a->ip6.sin6_port == b->ip6.sin6_port));
#endif
	else
		return false;
}



                                                                                
                                                                              
                                                                  
    
static void cleanupnodes(void)
{
	if (!Playing())
		return;


	for (SINT8 j = 1; j < MAXNETNODES; j++)
		if (!(netnodes[j].ingame || SendingFile(j)))
			nodeconnected[j] = false;
}

static SINT8 getfreenode(void)
{
	cleanupnodes();

	for (SINT8 j = 0; j < MAXNETNODES; j++)
		if (!nodeconnected[j])
		{
			nodeconnected[j] = true;
			badaddresswarned[j] = false;
			lastsenderror[j] = 0;
			lastsenderrorreport[j] = 0;
			return j;
		}

	                                                                                  
                                                                                     
                                                                          
                                                                                       
     
	                                                 
                                        
                          
              

	return -1;
}

#ifdef _DEBUG
void Command_Numnodes(void)
{
	INT32 connected = 0;
	INT32 ingame = 0;

	for (INT32 i = 1; i < MAXNETNODES; i++)
	{
		if (!(nodeconnected[i] || netnodes[i].ingame))
			continue;

		if (nodeconnected[i])
			connected++;
		if (netnodes[i].ingame)
			ingame++;

		CONS_Printf("%2d - ", i);
		if (netnodes[i].player != -1)
			CONS_Printf("player %.2d", netnodes[i].player);
		else
			CONS_Printf("         ");
		if (nodeconnected[i])
			CONS_Printf(" - connected");
		else
			CONS_Printf(" -          ");
		if (netnodes[i].ingame)
			CONS_Printf(" - ingame");
		else
			CONS_Printf(" -       ");
		CONS_Printf(" - %s\n", I_GetNodeAddress(i));
	}

	CONS_Printf("\n"
				"Connected: %d\n"
				"Ingame:    %d\n",
				connected, ingame);
}
#endif


static boolean SOCK_Get(void)
{
	size_t i;
	int j;
	ssize_t c;
	mysockaddr_t fromaddress;
	socklen_t fromlen;

	I_UPnPRetryTicker();

	for (size_t n = 0; n < mysocketses; n++)
	{
		fromlen = (socklen_t)sizeof(fromaddress);
		c = recvfrom(mysockets[n], (char *)&doomcom->data, MAXPACKETLENGTH, 0,
			(void *)&fromaddress, &fromlen);
		if (c != ERRSOCKET)
		{

			for (j = 1; j <= MAXNETNODES; j++)  
			{
				if (SOCK_cmpaddr(&fromaddress, &clientaddress[j], 0))
				{
					doomcom->remotenode = (INT16)j;  
					doomcom->datalength = (INT16)c;
					nodesocket[j] = mysockets[n];
					return false;
				}
			}



			j = getfreenode();
			if (j > 0)
			{
				M_Memcpy(&clientaddress[j], &fromaddress, fromlen);
				nodesocket[j] = mysockets[n];
				DEBFILE(va("New node detected: node:%d address:%s\n", j,
						SOCK_GetNodeAddress(j)));
				doomcom->remotenode = (INT16)j;  
				doomcom->datalength = (INT16)c;


				for (i = 0; i < numbans; i++)
				{
					if (SOCK_cmpaddr(&fromaddress, &banned[i], bannedmask[i]))
					{
						SOCK_bannednode[j] = true;
						DEBFILE("This dude has been banned\n");
						break;
					}
				}
				if (i == numbans)
					SOCK_bannednode[j] = false;
				return true;
			}
			else
				DEBFILE("New node detected: No more free slots\n");
		}
	}

	doomcom->remotenode = -1;  
	return false;
}

static inline ssize_t SOCK_SendToAddr(SOCKET_TYPE socket, mysockaddr_t *sockaddr)
{
	socklen_t d4 = (socklen_t)sizeof(struct sockaddr_in);
#ifdef HAVE_IPV6
	socklen_t d6 = (socklen_t)sizeof(struct sockaddr_in6);
#endif
	socklen_t d, da = (socklen_t)sizeof(mysockaddr_t);

	switch (sockaddr->any.sa_family)
	{
		case AF_INET:  d = d4; break;
#ifdef HAVE_IPV6
		case AF_INET6: d = d6; break;
#endif
		default:       d = da; break;
	}

	return sendto(socket, (char *)&doomcom->data, doomcom->datalength, 0, &sockaddr->any, d);
}

#define ALLOWEDERROR(x) ((x) == ECONNREFUSED || (x) == EWOULDBLOCK || (x) == EHOSTUNREACH || (x) == ENETUNREACH)

static void SOCK_Send(void)
{
	ssize_t c = ERRSOCKET;
	INT32 node = doomcom->remotenode;

	if (node < 0 || node > MAXNETNODES)
		return;

	if (!nodeconnected[node])
		return;

	if (node != BROADCASTADDR
		&& clientaddress[node].any.sa_family != AF_INET
#ifdef HAVE_IPV6
		&& clientaddress[node].any.sa_family != AF_INET6
#endif
	)
	{
		if (!badaddresswarned[node])
		{
			CONS_Alert(CONS_WARNING,
				"SOCK_Send: skipped node %d because it has no usable IP address\n",
				node);
			badaddresswarned[node] = true;
		}
		return;
	}

	if (node == BROADCASTADDR)
	{
		for (size_t i = 0; i < mysocketses; i++)
		{
			for (size_t j = 0; j < broadcastaddresses; j++)
			{
				if (myfamily[i] == broadcastaddress[j].any.sa_family)
				{
					c = SOCK_SendToAddr(mysockets[i], &broadcastaddress[j]);
					if (c == ERRSOCKET && !ALLOWEDERROR(SOCK_GetLastErrorCode()))
						break;
				}
			}
		}
	}
	else if (nodesocket[node] == (SOCKET_TYPE)ERRSOCKET)
	{
		for (size_t i = 0; i < mysocketses; i++)
		{
			if (myfamily[i] == clientaddress[node].any.sa_family)
			{
				c = SOCK_SendToAddr(mysockets[i], &clientaddress[node]);
				if (c == ERRSOCKET && !ALLOWEDERROR(SOCK_GetLastErrorCode()))
					break;
			}
		}
	}
	else
	{
		c = SOCK_SendToAddr(nodesocket[node], &clientaddress[node]);
	}

	if (c == ERRSOCKET)
	{
		int e = SOCK_GetLastErrorCode();  
		if (!ALLOWEDERROR(e))
		{
			tic_t now = I_GetTime();
			if (lastsenderror[node] != e
				|| !lastsenderrorreport[node]
				|| now >= lastsenderrorreport[node] + 5*TICRATE)
			{
				CONS_Alert(CONS_WARNING,
					"SOCK_Send: error sending to node %d (%s) #%u: %s; packet dropped\n",
					node, SOCK_GetNodeAddress(node), e, strerror(e));
				lastsenderror[node] = e;
				lastsenderrorreport[node] = now;
			}
		}
	}
}

#undef ALLOWEDERROR

static void SOCK_FreeNodenum(INT32 numnode)
{

	if (!numnode || numnode > MAXNETNODES)
		return;

	DEBFILE(va("Free node %d (%s)\n", numnode, SOCK_GetNodeAddress(numnode)));

	nodeconnected[numnode] = false;
	nodesocket[numnode] = ERRSOCKET;
	badaddresswarned[numnode] = false;
	lastsenderror[numnode] = 0;
	lastsenderrorreport[numnode] = 0;


	memset(&clientaddress[numnode], 0, sizeof (clientaddress[numnode]));
}






static SOCKET_TYPE UDP_Bind(int family, struct sockaddr *addr, socklen_t addrlen)
{
	                                                                           
                                                                           
                                                                            
	const int targetbuffersize = 1024 << 10;
	SOCKET_TYPE s = socket(family, SOCK_DGRAM, IPPROTO_UDP);
	int opt;
	socklen_t opts;
#ifdef FIONBIO
	unsigned long trueval = true;
#endif
	mysockaddr_t straddr;
	socklen_t len = sizeof(straddr);

	if (s == (SOCKET_TYPE)ERRSOCKET)
		return (SOCKET_TYPE)ERRSOCKET;
#ifdef USE_WINSOCK
	{  
#ifdef USE_WINSOCK2
		DWORD dwBytesReturned = 0;
		BOOL bfalse = FALSE;
		WSAIoctl(s, SIO_UDP_CONNRESET, &bfalse, sizeof(bfalse),
		         NULL, 0, &dwBytesReturned, NULL, NULL);
#else
		unsigned long falseval = false;
		ioctl(s, SIO_UDP_CONNRESET, &falseval);
#endif
	}
#endif

	memcpy(&straddr, addr, addrlen);
	I_OutputMsg("Binding to %s\n", SOCK_AddrToStr(&straddr));

	if (family == AF_INET)
	{
		if (straddr.ip4.sin_addr.s_addr == htonl(INADDR_ANY))
		{
			opt = true;
			opts = (socklen_t)sizeof(opt);
			                                                            
                                                               
                                                                    
                        
#ifdef USE_WINSOCK2
			if (serverrunning)
			{
				if (setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
					(char *)&opt, opts))
					CONS_Alert(CONS_WARNING,
						"Could not reserve the server UDP port exclusively\n");
			}
			else
#endif
			if (!serverrunning)
				setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, opts);
		}

		opt = true;
		opts = (socklen_t)sizeof(opt);
		if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&opt, opts))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Could not get broadcast rights\n"));  
		}
	}
#ifdef HAVE_IPV6
	else if (family == AF_INET6)
	{
		if (memcmp(&straddr.ip6.sin6_addr, &in6addr_any, sizeof(in6addr_any)) == 0)  
		{
			opt = true;
			opts = (socklen_t)sizeof(opt);
#ifdef USE_WINSOCK2
			if (serverrunning)
			{
				if (setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
					(char *)&opt, opts))
					CONS_Alert(CONS_WARNING,
						"Could not reserve the server UDP port exclusively\n");
			}
			else
#endif
			if (!serverrunning)
				setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, opts);
		}
#ifdef IPV6_V6ONLY

		opt = true;
		opts = (socklen_t)sizeof(opt);
		if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&opt, opts))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Could not limit IPv6 bind\n"));  
		}
#endif
	}
#endif

	if (bind(s, addr, addrlen) == ERRSOCKET)
	{
		close(s);
		I_OutputMsg("Binding failed\n");
		return (SOCKET_TYPE)ERRSOCKET;
	}

#ifdef HAVE_IPV6
	if (family == AF_INET6)
	{

		if (memcmp(&straddr.ip6.sin6_addr, &in6addr_any, sizeof(in6addr_any)) == 0)  
		{
			struct ipv6_mreq maddr;

			inet_pton(AF_INET6, IPV6_MULTICAST_ADDRESS, &maddr.ipv6mr_multiaddr);
			maddr.ipv6mr_interface = 0;
			if (setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, (const char *)&maddr, sizeof(maddr)) != 0)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Could not register multicast address\n"));
			}
		}
	}
#endif

#ifdef FIONBIO

	opt = true;
	if (ioctl(s, FIONBIO, &trueval) != 0)
	{
		close(s);
		I_OutputMsg("Seting FIOBIO on failed\n");
		return (SOCKET_TYPE)ERRSOCKET;
	}
#endif

	opts = (socklen_t)sizeof(opt);
	getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&opt, &opts);
	CONS_Printf(M_GetText("Network system buffer: %dKb\n"), opt>>10);

	if (opt < targetbuffersize)
	{
		opt = targetbuffersize;
		opts = (socklen_t)sizeof(opt);
		setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&opt, opts);
		getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&opt, &opts);
		if (opt < 64<<10)
			CONS_Alert(CONS_WARNING, M_GetText("Can't set buffer length to 64k, file transfer will be bad\n"));
		else
			CONS_Printf(M_GetText("Network system buffer set to: %dKb\n"), opt>>10);
	}

	                                                                          
                                                                              
	opts = (socklen_t)sizeof(opt);
	if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&opt, &opts) == 0
	&& opt < targetbuffersize)
	{
		opt = targetbuffersize;
		opts = (socklen_t)sizeof(opt);
		setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&opt, opts);
	}

	if (getsockname(s, &straddr.any, &len) == -1)
		CONS_Alert(CONS_WARNING, M_GetText("Failed to get port number\n"));
	else
	{
		if (family == AF_INET)
			current_port = (UINT16)ntohs(straddr.ip4.sin_port);
#ifdef HAVE_IPV6
		else if (family == AF_INET6)
			current_port = (UINT16)ntohs(straddr.ip6.sin6_port);
#endif
	}

	return s;
}

static boolean UDP_Socket(void)
{
	size_t s;
	struct my_addrinfo *ai, *runp, hints;
	int gaie;
#ifdef HAVE_IPV6
	const INT32 b_ipv6 = Net_UseIPv6();
#endif
	const char *serv;


	for (s = 0; s < mysocketses; s++)
		mysockets[s] = ERRSOCKET;
	for (s = 0; s < MAXNETNODES+1; s++)
		nodesocket[s] = ERRSOCKET;
	s = 0;

	memset(&hints, 0x00, sizeof (hints));
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if (serverrunning)
		serv = serverport_name;
	else
		serv = clientport_name;

	if (M_CheckParm("-bindaddr"))
	{
		while (M_IsNextParm())
		{
			gaie = I_getaddrinfo(M_GetNextParm(), serv, &hints, &ai);
			if (gaie == 0)
			{
				runp = ai;
				while (runp != NULL && s < MAXNETNODES+1)
				{
					mysockets[s] = UDP_Bind(runp->ai_family, runp->ai_addr, (socklen_t)runp->ai_addrlen);
					if (mysockets[s] != (SOCKET_TYPE)ERRSOCKET)
					{
						myfamily[s] = hints.ai_family;
						s++;
					}
					runp = runp->ai_next;
				}
				I_freeaddrinfo(ai);
			}
		}
	}
	else
	{
		gaie = I_getaddrinfo("0.0.0.0", serv, &hints, &ai);
		if (gaie == 0)
		{
			runp = ai;
			while (runp != NULL && s < MAXNETNODES+1)
			{
				mysockets[s] = UDP_Bind(runp->ai_family, runp->ai_addr, (socklen_t)runp->ai_addrlen);
				if (mysockets[s] != (SOCKET_TYPE)ERRSOCKET)
				{
					myfamily[s] = hints.ai_family;
					s++;
				}
				runp = runp->ai_next;
			}
			I_freeaddrinfo(ai);
		}
	}
#ifdef HAVE_IPV6
	if (b_ipv6)
	{
		hints.ai_family = AF_INET6;
		if (M_CheckParm("-bindaddr6"))
		{
			while (M_IsNextParm())
			{
				gaie = I_getaddrinfo(M_GetNextParm(), serv, &hints, &ai);
				if (gaie == 0)
				{
					runp = ai;
					while (runp != NULL && s < MAXNETNODES+1)
					{
						mysockets[s] = UDP_Bind(runp->ai_family, runp->ai_addr, (socklen_t)runp->ai_addrlen);
						if (mysockets[s] != (SOCKET_TYPE)ERRSOCKET)
						{
							myfamily[s] = hints.ai_family;
							s++;
						}
						runp = runp->ai_next;
					}
					I_freeaddrinfo(ai);
				}
			}
		}
		else
		{
			gaie = I_getaddrinfo("::", serv, &hints, &ai);
			if (gaie == 0)
			{
				runp = ai;
				while (runp != NULL && s < MAXNETNODES+1)
				{
					mysockets[s] = UDP_Bind(runp->ai_family, runp->ai_addr, (socklen_t)runp->ai_addrlen);
					if (mysockets[s] != (SOCKET_TYPE)ERRSOCKET)
					{
						myfamily[s] = hints.ai_family;
						s++;
					}
					runp = runp->ai_next;
				}
				I_freeaddrinfo(ai);
			}
		}
	}
#endif

	mysocketses = s;
	if (s == 0)  
		return false;

#ifdef HAVE_MINIUPNPC
	                                                                           
                                                                           
	if (serverrunning)
		I_RequestUPnPMapping(serverport_name);
#endif

	s = 0;


	packetheaderlength = 20 + 8;  

	clientaddress[s].any.sa_family = AF_INET;
	clientaddress[s].ip4.sin_port = htons(0);
	clientaddress[s].ip4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  
	s++;

	s = 0;


	broadcastaddress[s].any.sa_family = AF_INET;
	broadcastaddress[s].ip4.sin_port = htons(atoi(DEFAULTPORT));
	broadcastaddress[s].ip4.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	s++;

#ifdef HAVE_IPV6
	if (b_ipv6)
	{
		broadcastaddress[s].any.sa_family = AF_INET6;
		broadcastaddress[s].ip6.sin6_port = htons(atoi(DEFAULTPORT));
		broadcastaddress[s].ip6.sin6_flowinfo = 0;
		inet_pton(AF_INET6, IPV6_MULTICAST_ADDRESS, &broadcastaddress[s].ip6.sin6_addr);
		broadcastaddress[s].ip6.sin6_scope_id = 0;
		s++;
	}
#endif

	broadcastaddresses = s;

	doomcom->extratics = 1;  

	return true;
}

boolean I_InitTcpDriver(void)
{
	boolean tcp_was_up = init_tcp_driver;
	if (!init_tcp_driver)
	{
#ifdef USE_WINSOCK
#ifdef USE_WINSOCK2
		const WORD VerNeed = MAKEWORD(2,2);
#else
		const WORD VerNeed = MAKEWORD(1,1);
#endif
		WSADATA WSAData;
		const int WSAresult = WSAStartup(VerNeed, &WSAData);
		if (WSAresult != 0)
		{
			LPCSTR WSError = NULL;
			switch (WSAresult)
			{
				case WSASYSNOTREADY:
					WSError = "The underlying network subsystem is not ready for network communication";
					break;
				case WSAEINPROGRESS:
					WSError = "A blocking Windows Sockets 1.1 operation is in progress";
					break;
				case WSAEPROCLIM:
					WSError = "Limit on the number of tasks supported by the Windows Sockets implementation has been reached";
					break;
				case WSAEFAULT:
					WSError = "WSAData is not a valid pointer? What kind of setup do you have?";
					break;
				default:
					WSError = va("Error code %u",WSAresult);
					break;
			}
			if (WSAresult != WSAVERNOTSUPPORTED)
				CONS_Debug(DBG_NETPLAY, "WinSock(TCP/IP) error: %s\n",WSError);
		}
#ifdef USE_WINSOCK2
		if(LOBYTE(WSAData.wVersion) != 2 ||
			HIBYTE(WSAData.wVersion) != 2)
		{
			WSACleanup();
			CONS_Debug(DBG_NETPLAY, "No WinSock(TCP/IP) 2.2 driver detected\n");
		}
#else
		if (LOBYTE(WSAData.wVersion) != 1 ||
			HIBYTE(WSAData.wVersion) != 1)
		{
			WSACleanup();
			CONS_Debug(DBG_NETPLAY, "No WinSock(TCP/IP) 1.1 driver detected\n");
		}
#endif
		CONS_Debug(DBG_NETPLAY, "WinSock description: %s\n",WSAData.szDescription);
		CONS_Debug(DBG_NETPLAY, "WinSock System Status: %s\n",WSAData.szSystemStatus);
#endif
		init_tcp_driver = true;
	}

	if (!tcp_was_up && init_tcp_driver)
	{
		I_AddExitFunc(I_ShutdownTcpDriver);
#ifdef HAVE_MINIUPNPC
		if (M_CheckParm("-useUPnP"))
			I_ConfigureUPnP(true);
		else
			UPNP_support = false;
#endif
	}
	return init_tcp_driver;
}

static void SOCK_CloseSocketOnly(void)
{
	for (size_t i=0; i < mysocketses; i++)
	{
		if (mysockets[i] != (SOCKET_TYPE)ERRSOCKET)
			close(mysockets[i]);
		mysockets[i] = ERRSOCKET;
	}
	mysocketses = 0;
}

static void SOCK_CloseSocket(void)
{
#ifdef HAVE_MINIUPNPC
	                                                                          
                                          
	I_ReleaseUPnPMapping();
#endif
	SOCK_CloseSocketOnly();
}

void I_ShutdownTcpDriver(void)
{
	 
	SOCK_CloseSocketOnly();

	CONS_Printf("I_ShutdownTcpDriver: ");
#ifdef USE_WINSOCK
	WS_addrinfocleanup();
	WSACleanup();
#endif
	CONS_Printf("shut down\n");
	init_tcp_driver = false;
}

static SINT8 SOCK_NetMakeNodewPort(const char *address, const char *port)
{
	SINT8 newnode = -1;
	struct my_addrinfo *ai = NULL, *runp, hints;
	int gaie;
	size_t i;
	boolean foundaddress = false;

	 if (!port || !port[0])
		port = DEFAULTPORT;

	DEBFILE(va("Creating new node: %s@%s\n", address, port));

	memset (&hints, 0x00, sizeof (hints));
	hints.ai_flags = 0;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	gaie = I_getaddrinfo(address, port, &hints, &ai);
	if (gaie == 0)
	{
		newnode = getfreenode();
	}
	if (newnode == -1)
	{
		I_freeaddrinfo(ai);
		return -1;
	}
	else
		runp = ai;

	while (runp != NULL)
	{

		for (i = 0; i < mysocketses; ++i)
		{
			if (runp->ai_addr != NULL
				&& runp->ai_addrlen <= sizeof (clientaddress[newnode])
				&& runp->ai_addr->sa_family == myfamily[i])
			{
				memset(&clientaddress[newnode], 0, sizeof (clientaddress[newnode]));
				memcpy(&clientaddress[newnode], runp->ai_addr, runp->ai_addrlen);
				foundaddress = true;
				break;
			}
		}

		if (foundaddress)
			break;
		runp = runp->ai_next;
	}
	I_freeaddrinfo(ai);

	if (!foundaddress)
	{
		CONS_Debug(DBG_NETPLAY,
			"No compatible IP address for %s:%s; skipping server entry\n",
			address, port);
		SOCK_FreeNodenum(newnode);
		return -1;
	}

	return newnode;
}

static boolean SOCK_OpenSocket(void)
{
	memset(clientaddress, 0, sizeof (clientaddress));
	memset(badaddresswarned, 0, sizeof (badaddresswarned));
	memset(lastsenderror, 0, sizeof (lastsenderror));
	memset(lastsenderrorreport, 0, sizeof (lastsenderrorreport));

	nodeconnected[0] = true;  
	for (size_t i = 1; i < MAXNETNODES; i++)
		nodeconnected[i] = false;
	nodeconnected[BROADCASTADDR] = true;
	I_NetSend = SOCK_Send;
	I_NetGet = SOCK_Get;
	I_NetCloseSocket = SOCK_CloseSocket;
	I_NetFreeNodenum = SOCK_FreeNodenum;
	I_NetMakeNodewPort = SOCK_NetMakeNodewPort;


	SOCK_CloseSocketOnly();
	return UDP_Socket();
}

static boolean SOCK_Ban(INT32 node)
{
	if (node > MAXNETNODES)
		return false;

	banned = Z_Realloc(banned, sizeof(*banned) * (numbans+1), PU_STATIC, NULL);
	bannedmask = Z_Realloc(bannedmask, sizeof(*bannedmask) * (numbans+1), PU_STATIC, NULL);
	M_Memcpy(&banned[numbans], &clientaddress[node], sizeof (mysockaddr_t));
	if (banned[numbans].any.sa_family == AF_INET)
	{
		banned[numbans].ip4.sin_port = 0;
		bannedmask[numbans] = 32;
	}
#ifdef HAVE_IPV6
	else if (banned[numbans].any.sa_family == AF_INET6)
	{
		banned[numbans].ip6.sin6_port = 0;
		bannedmask[numbans] = 64;
	}
#endif
	numbans++;
	return true;
}

static boolean SOCK_SetBanAddress(const char *address, const char *mask)
{
	struct my_addrinfo *ai, *runp, hints;
	int gaie;

	if (!address)
		return false;

	memset(&hints, 0x00, sizeof(hints));
	hints.ai_flags = 0;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	gaie = I_getaddrinfo(address, "0", &hints, &ai);
	if (gaie != 0)
		return false;

	runp = ai;

	while(runp != NULL)
	{
		banned = Z_Realloc(banned, sizeof(*banned) * (numbans+1), PU_STATIC, NULL);
		bannedmask = Z_Realloc(bannedmask, sizeof(*bannedmask) * (numbans+1), PU_STATIC, NULL);
		memcpy(&banned[numbans], runp->ai_addr, runp->ai_addrlen);

		if (mask)
			bannedmask[numbans] = (UINT8)atoi(mask);
#ifdef HAVE_IPV6
		else if (runp->ai_family == AF_INET6)
			bannedmask[numbans] = 64;
#endif
		else
			bannedmask[numbans] = 32;

		if (bannedmask[numbans] > 32 && runp->ai_family == AF_INET)
			bannedmask[numbans] = 32;
#ifdef HAVE_IPV6
		else if (bannedmask[numbans] > 128 && runp->ai_family == AF_INET6)
			bannedmask[numbans] = 64;
#endif
		numbans++;
		runp = runp->ai_next;
	}

	I_freeaddrinfo(ai);

	return true;
}

static void SOCK_ClearBans(void)
{
	numbans = 0;
	Z_Free(banned);
	banned = NULL;
	Z_Free(bannedmask);
	bannedmask = NULL;
}

boolean I_InitTcpNetwork(void)
{
	char serverhostname[255];
	const char *urlparam = NULL;
	boolean ret = false;

	if (!I_InitTcpDriver())
		return false;

	if (M_CheckParm("-port") || M_CheckParm("-serverport"))


	 
	{
		  
                                         
                                           
    
		serverport_name = M_GetNextParm();
	}
	if (M_CheckParm("-clientport"))
		clientport_name = M_GetNextParm();


	if (M_CheckParm("-server") || dedicated)
	{
		server = true;








		if (dedicated)
			doomcom->numnodes = 0;
                            
                                                      
		else
			doomcom->numnodes = 1;

		if (doomcom->numnodes < 0)
			doomcom->numnodes = 0;
		if (doomcom->numnodes > MAXNETNODES)
			doomcom->numnodes = MAXNETNODES;


		servernode = 0;



		hardware_MAXPACKETLENGTH = INETPACKETLENGTH;

		ret = true;
	}
	else if ((urlparam = M_GetUrlProtocolArg()) != NULL || M_CheckParm("-connect"))
	{
		if (urlparam != NULL)
			strlcpy(serverhostname, urlparam, sizeof(serverhostname));
		else if (M_IsNextParm())
			strlcpy(serverhostname, M_GetNextParm(), sizeof(serverhostname));
		else
			serverhostname[0] = 0;  


		if (serverhostname[0])
		{
			COM_BufAddText("connect \"");
			COM_BufAddText(serverhostname);
			COM_BufAddText("\"\n");


			hardware_MAXPACKETLENGTH = INETPACKETLENGTH;
		}
		else
		{

			COM_BufAddText("connect any\n");

			hardware_MAXPACKETLENGTH = MAXPACKETLENGTH;
		}
	}

	I_NetOpenSocket = SOCK_OpenSocket;
	I_Ban = SOCK_Ban;
	I_ClearBans = SOCK_ClearBans;
	I_GetNodeAddress = SOCK_GetNodeAddress;
	I_GetBanAddress = SOCK_GetBanAddress;
	I_GetBanMask = SOCK_GetBanMask;
	I_SetBanAddress = SOCK_SetBanAddress;
	bannednode = SOCK_bannednode;

	return ret;
}

boolean Net_IsNodeIPv6(INT32 node)
{
#ifdef NO_IPV6
	return false;
#else
	return clientaddress[node].any.sa_family == AF_INET6;
#endif
}

#include "i_addrinfo.c"
