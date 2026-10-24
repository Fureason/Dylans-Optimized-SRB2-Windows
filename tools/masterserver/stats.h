// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//


#ifndef _STATS_H_
#define _STATS_H_
#ifndef _IPCS_H_
#error "You forget to include ipcs.h"
#endif
#include <time.h>

#define NUMLASTSERVERS 6
#define SERVERMOTDTEXT "No Clans, advertising websites, Chat, Hangout, Hacked, or Role Playing Servers Allowed"




class CServerStats
{
	time_t uptime;  
	char motd[2048];  
	msg_server_t last_server[NUMLASTSERVERS];  
	time_t last_time[NUMLASTSERVERS];  
	char version[32];  

public:
	int num_connections;
	int num_http_con;
	int num_text_con;
	int num_RSS92_con;
	int num_RSS10_con;
	int num_servers;
	int num_add;
	int num_removal;
	int num_retrieval;
	int num_autoremoval;
	int num_badconnection;

	CServerStats();
	~CServerStats();
	const char *getUptime();
	int getHours();
	int getDays();
	const char *getMotd();
	const char *getLastServers();
	const char *getVersion();
	void putMotd(char *);
	void putLastServer(msg_server_t *);
};





#endif
