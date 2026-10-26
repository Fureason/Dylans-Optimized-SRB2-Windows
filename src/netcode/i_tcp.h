// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_tcp.h
/// \brief TCP driver, sockets.

#ifndef __I_TCP__
#define __I_TCP__

extern UINT16 current_port;

/**	\brief	The I_InitTcpNetwork function

	\return	true if going or in a netgame, else it's false
*/
boolean I_InitTcpNetwork(void);

/**	\brief	The I_InitTcpNetwork function

	\return	true if TCP/IP stack was initilized, else it's false
*/
boolean I_InitTcpDriver(void);
void I_ShutdownTcpDriver(void);
/** Enable or disable automatic UPnP mapping before a server socket opens. */
void I_ConfigureUPnP(boolean enable);
/** Retry router discovery/mapping for an already-open hosting socket. */
boolean I_RetryUPnP(void);

typedef enum
{
	I_UPNP_UNAVAILABLE = 0,
	I_UPNP_DISABLED,
	I_UPNP_DISCOVERING,
	I_UPNP_READY,
	I_UPNP_MAPPING,
	I_UPNP_MAPPED,
	I_UPNP_FAILED
} i_upnp_status_t;

/** Return the current automatic port-mapping state for host diagnostics. */
i_upnp_status_t I_GetUPnPStatus(void);
/** Queue a router-entry refresh so hoststatus cannot report a stale mapping. */
void I_ValidateUPnPMapping(void);
/** Name of the successful mapping protocol, or "none". */
const char *I_GetPortMappingMethod(void);
/** Queue a Windows firewall rule check; return false if it could not start. */
boolean I_RequestWindowsFirewallRule(const char *port);
#endif
