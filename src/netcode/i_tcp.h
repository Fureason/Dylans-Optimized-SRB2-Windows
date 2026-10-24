// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __I_TCP__
#define __I_TCP__

extern UINT16 current_port;

                                        

                                                       
  
boolean I_InitTcpNetwork(void);

                                        

                                                             
  
boolean I_InitTcpDriver(void);
void I_ShutdownTcpDriver(void);
 
void I_ConfigureUPnP(boolean enable);
 
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

 
i_upnp_status_t I_GetUPnPStatus(void);
 
void I_ValidateUPnPMapping(void);
 
const char *I_GetPortMappingMethod(void);
 
boolean I_RequestWindowsFirewallRule(const char *port);
#endif
