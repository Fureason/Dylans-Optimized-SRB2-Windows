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


#ifdef __GNUC__
#include <unistd.h>
#endif

#ifdef _WIN32

#else
#include <sys/types.h>    
#include <sys/socket.h>   
#endif
#include <stdlib.h>       
#include <signal.h>       
#ifndef _WIN32
#include <netdb.h>        
#endif
#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>     
#endif
#include <string.h>       
#include "ipcs.h"
#include "common.h"

typedef void Sigfunc(int);

#ifndef _WIN32
static void sigHandler(int signr);
static Sigfunc *mySignal(int signo, Sigfunc *func);
#endif

#ifdef _MSC_VER
#pragma warning(disable :  4127)
#endif


#if defined (_WIN32) || defined (__OS2__)

static int inet_aton(const char *hostname, struct in_addr *addr)
{
	return ((addr->s_addr=inet_addr(hostname)) != INADDR_NONE);
}
#endif

  
            
  
CSocket::CSocket()
{
	dbgPrintf(DEFCOL, "Initializing socket... ");
#ifdef _WIN32

	WSADATA winsockdata;
	if (WSAStartup(MAKEWORD(1, 1), &winsockdata))
		conPrintf(RED,"No TCP/IP driver detected");
#else
	signal(SIGPIPE, sigHandler);
#endif
	FD_ZERO(&rset);
	memset(&addr, 0, sizeof (addr));
	addr.sin_family = AF_INET;
	dbgPrintf(DEFCOL, "DONE1.\n");
}

  
             
  
CSocket::~CSocket()
{
	dbgPrintf(DEFCOL, "Freeing socket... ");
#ifdef _WIN32
	WSACleanup();
#endif
	dbgPrintf(DEFCOL, "DONE2.\n");
}

  
          
  
int CSocket::getIP(const char *ip_addr)
{
	struct hostent *host_ent;

	dbgPrintf(DEFCOL, "get IP for %s... ", ip_addr);
	if (!inet_aton(ip_addr, &addr.sin_addr))
	{
		host_ent = gethostbyname(ip_addr);
		if (host_ent == NULL)
			return GETHOSTBYNAME_ERROR;
		memcpy(&addr.sin_addr, host_ent->h_addr_list[0],
			sizeof (struct in_addr));
	}
	dbgPrintf(DEFCOL, "got %s. ", inet_ntoa(addr.sin_addr));
	dbgPrintf(DEFCOL, "DONE3.\n");
	return 0;
}

  
                  
  
CClientSocket::CClientSocket()
{
	dbgPrintf(DEFCOL, "Initializing client socket... ");
	socket_fd = (SOCKET)-1;
	dbgPrintf(DEFCOL, "DONE4.\n");
}

  
                   
  
CClientSocket::~CClientSocket()
{
	dbgPrintf(DEFCOL, "Freeing client socket... ");
	if (socket_fd != (SOCKET)-1)
		close(socket_fd);
	dbgPrintf(DEFCOL, "DONE5.\n");
}

  
            
  
int CClientSocket::connect(const char *ip_addr, const char *str_port)
{
	dbgPrintf(DEFCOL, "Connect to %s %s...\n", ip_addr, str_port);


	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == (SOCKET)-1)
		return SOCKET_ERROR;

	if (getIP(ip_addr) == GETHOSTBYNAME_ERROR)
		return GETHOSTBYNAME_ERROR;
	addr.sin_port = htons(atoi(str_port));

	if (::connect(socket_fd, (struct sockaddr *) &addr, sizeof (addr)) < 0)
		return CONNECT_ERROR;

	FD_SET(socket_fd, &rset);

	dbgPrintf(DEFCOL, "DONE6.\n");

	return 0;
}

  
          
   
int CClientSocket::write(msg_t *msg)
{
	int len;

	if (msg->length < 0)
		msg->length = (INT32)strlen(msg->buffer);
	if (msg->length > PACKET_SIZE)
		return WRITE_ERROR;  
	len = msg->length + HEADER_SIZE;

	msg->id = htonl(msg->id);
	msg->type = htonl(msg->type);
	msg->length = htonl(msg->length);
	msg->room = htonl(msg->room);

	dbgPrintf(DEFCOL, "Write a message %d (%s)... ", len, msg->buffer);

	if (send(socket_fd, (const char *)msg, len, 0) != len)
		return WRITE_ERROR;

	dbgPrintf(DEFCOL, "DONE7.\n");

	return 0;
}

  
         
   
int CClientSocket::read(msg_t *msg)
{
	struct timeval timeout;
	fd_set tset;

	dbgPrintf(DEFCOL, "Waiting a message... ");

	timeout.tv_sec = 60, timeout.tv_usec = 0;
	memcpy(&tset, &rset, sizeof (tset));
	if ((select(255, &tset, NULL, NULL, &timeout)) <= 0)
		return TIMEOUT_ERROR;

	if (!FD_ISSET(socket_fd, &tset))
		return READ_ERROR;

	dbgPrintf(DEFCOL, "Reading a message.\n");

	if (recv(socket_fd, (char *)msg, HEADER_SIZE, 0) != HEADER_SIZE)
		return READ_ERROR;

	msg->id = ntohl(msg->id);
	msg->type = ntohl(msg->type);
	msg->length = ntohl(msg->length);
	msg->room = ntohl(msg->room);

	if (!msg->length)  
		return 0;

	if (msg->length > PACKET_SIZE)
		return READ_ERROR;  

	if (recv(socket_fd, msg->buffer, msg->length, 0) != msg->length)
		return READ_ERROR;

	dbgPrintf(DEFCOL, "DONE8.\n");

	return 0;
}

  
                  
  
CServerSocket::CServerSocket()
{
	size_t id;
	dbgPrintf(DEFCOL, "Initializing server socket... ");
	num_clients = 0;
	accept_fd = udp_fd = (SOCKET)-1;
	for (id = 0; id < MAX_CLIENT; id++)
		client_fd[id] = (SOCKET)-1;
	udp_addr.sin_family = AF_INET;
	dbgPrintf(DEFCOL, "DONE9.\n");
}

  
                   
  
CServerSocket::~CServerSocket()
{
	size_t id;
	dbgPrintf(DEFCOL, "Freeing server socket... ");
	if (udp_fd != (SOCKET)-1)
		close(udp_fd);
	if (accept_fd != (SOCKET)-1)
		close(accept_fd);
	for (id = 0; id < num_clients || id < MAX_CLIENT; id++)
		if (client_fd[id] != (SOCKET)-1)
			close(client_fd[id]);
	dbgPrintf(DEFCOL, "DONE10.\n");
}

  
           
  
int CServerSocket::listen(const char *str_port)
{
	dbgPrintf(DEFCOL, "Listen on %s... ", str_port);

	if ((accept_fd = socket(AF_INET, SOCK_STREAM, 0)) == (SOCKET)-1)
		return SOCKET_ERROR;
	int one = 1; setsockopt(accept_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(int));

	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(atoi(str_port));

	if (bind(accept_fd, (struct sockaddr *) &addr, sizeof (addr)) < 0)
		return BIND_ERROR;

	if (::listen(accept_fd, 5) < 0)
		return LISTEN_ERROR;

	FD_SET(accept_fd, &rset);


	if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == (SOCKET)-1)
		return SOCKET_ERROR;

	udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	udp_addr.sin_port = htons(atoi(str_port)+1);

	if (bind(udp_fd, (struct sockaddr *) &udp_addr, sizeof (udp_addr)) < 0)
	return BIND_ERROR;

	FD_SET(udp_fd, &rset);

	dbgPrintf(DEFCOL, "DONE11.\n");

	return 0;
}

  
                  
   
int CServerSocket::deleteClient(size_t id)
{
	dbgPrintf(DEFCOL, "Deleting (client %u) of %u... ", id+1, num_clients);
	FD_CLR(client_fd[id], &rset);
	close(client_fd[id]);
	if (num_clients != 0)
		num_clients--;
	client_fd[id] = client_fd[num_clients];  
	memmove(&client_addr[id], &client_addr[num_clients], sizeof (client_addr[num_clients]));
	client_fd[num_clients] = (SOCKET)-1;  
	memset(&client_addr[num_clients], 0 , sizeof (client_addr[num_clients]));

	dbgPrintf(DEFCOL, "DONE12.\n");

	return 0;
}

  
             
   
const char *CServerSocket::getUdpIP(void)
{
	return inet_ntoa(udp_in_addr.sin_addr);
}

  
               
   
const char *CServerSocket::getUdpPort(bool offset)
{
	static char buffer[8];
	UINT16 port = htons(udp_in_addr.sin_port);
	if (offset)
		port--;
	snprintf(buffer, sizeof buffer, "%d", port);
	return buffer;
}

  
                                                                               
                                                                           
                                                     
   
int CServerSocket::read(msg_t *msg)
{
	struct timeval timeout;
	fd_set tset;
	UINT32 id;
	socklen_t lg = sizeof (addr);

	timeout.tv_sec = 20, timeout.tv_usec = 0;
	memcpy(&tset, &rset, sizeof (tset));
	if ((select(255, &tset, NULL, NULL, &timeout)) <= 0)
	{
		msg->type = TIMEOUT_MSG;
		return TIMEOUT_ERROR;
	}

	if (FD_ISSET(udp_fd, &tset))
	{
		msg->type = UDP_RECV_MSG;  
		msg->length = recvfrom(udp_fd, msg->buffer, PACKET_SIZE, 0, (struct sockaddr *)&udp_in_addr, &lg);
		return 0;
	}

	if (FD_ISSET(accept_fd, &tset))
	{
		msg->type = ACCEPT_MSG;
		return this->accept();
	}

	id = 0;
	while (!FD_ISSET(client_fd[id], &tset) && id < MAX_CLIENT)
		id++;


	if ( (id >= num_clients) || (id >= MAX_CLIENT) )
	{
		dbgPrintf(DEFCOL, "This should never happen\n");
		return READ_ERROR;
	}

	dbgPrintf(DEFCOL, "Reading a message from (socket %u) for (client %u)... ", client_fd[id], id+1);

	if (recv(client_fd[id], (char *)msg, HEADER_SIZE, 0) != HEADER_SIZE)
	{
		deleteClient(id);  
		return READ_ERROR;
	}


	msg->type = ntohl(msg->type);
	msg->length = ntohl(msg->length);
	msg->room = ntohl(msg->room);

	if ((0 < msg->length) && (msg->length < PACKET_SIZE))
	{
		timeout.tv_sec = 0, timeout.tv_usec = 0;  
		memcpy(&tset, &rset, sizeof (tset));
		if ((select(255, &tset, NULL, NULL, &timeout)) <= 0)
		{
			deleteClient(id);
			return READ_ERROR;
		}

		if (!FD_ISSET(client_fd[id], &tset))
		{
			deleteClient(id);
			return READ_ERROR;
		}
		if (recv(client_fd[id], msg->buffer, msg->length, 0) != msg->length)
		{
			deleteClient(id);
			return READ_ERROR;
		}
	}

	msg->id = id;

	dbgPrintf(DEFCOL, "DONE13.\n");

	return 0;
}

  
           
   
int CServerSocket::write(msg_t *msg)
{
	struct timeval timeout;
	fd_set tset;
	int len;

	dbgPrintf(DEFCOL, "Write a message... ");
	FD_ZERO(&tset);
	  
                                                    
                                                  
                                                    
                                                   
                                           
    
	FD_SET(client_fd[msg->id], &tset);
	timeout.tv_sec = 0, timeout.tv_usec = 0;
	if ((select(255, NULL, &tset, NULL, &timeout)) <= 0)
	{
		deleteClient(msg->id);  
		return TIMEOUT_ERROR;
	}
	if (msg->length < 0)
		msg->length = (INT32)strlen(msg->buffer);
	if (msg->length > PACKET_SIZE)
		return WRITE_ERROR;  
	len = msg->length+HEADER_SIZE;

	msg->type = htonl(msg->type);
	msg->length = htonl(msg->length);
	if (send(client_fd[msg->id], (const char *)msg, len, 0) != len)
	{
		deleteClient(msg->id);
		return WRITE_ERROR;
	}

	dbgPrintf(DEFCOL, "DONE14.\n");

	return 0;
}

  
             
   
int CServerSocket::writeUDP(const char *data, size_t length, const char *ip, UINT16 port)
{
	sockaddr_in sin;
	struct timeval timeout;
	fd_set tset;

	FD_ZERO(&tset);
	FD_SET(udp_fd, &tset);

	if ((select(255, NULL, &tset, NULL, &timeout)) <= 0)
	{
		return TIMEOUT_ERROR;
	}

	sin.sin_family = AF_INET;
	inet_aton(ip, &sin.sin_addr);
	sin.sin_port = htons(port);

	sendto(udp_fd, data, (int)length, 0, (sockaddr*)&sin, sizeof(sin));

	return 0;
}


  
           
   
int CServerSocket::accept()
{
	socklen_t lg = sizeof (addr);

	dbgPrintf(DEFCOL, "Accepting on (socket %u)... ", accept_fd);

	if ((client_fd[num_clients] = ::accept(accept_fd, (struct sockaddr *) &addr, &lg)) == (SOCKET)-1)
	{

		return ACCEPT_ERROR;
	}

	if (num_clients < MAX_CLIENT-1)
	{
		extern FILE *sockfile;

		logPrintf(sockfile, "Accepting on socket %u for client %u (%s:%i)\n",
		client_fd[num_clients], num_clients+1, inet_ntoa(addr.sin_addr), htons(addr.sin_port));
		memcpy(&client_addr[num_clients], &addr, sizeof (addr));
		FD_SET(client_fd[num_clients], &rset);
		num_clients++;
	}
	else  
	{

		close(client_fd[num_clients]);
		client_fd[num_clients] = (SOCKET)-1;
		return ACCEPT_ERROR;
	}



	return 0;
}

  
                
   
const char *CServerSocket::getClientIP(size_t id)
{
	dbgPrintf(DEFCOL, "getClientIP: %u\n", id);
	return inet_ntoa(client_addr[id].sin_addr);
}

  
                  
   
const char *CServerSocket::getClientPort(size_t id)
{
	static char buffer[8];

	dbgPrintf(DEFCOL, "getClientPort: %u\n", id);
	UINT16 port = htons(client_addr[id].sin_port);
	snprintf(buffer, sizeof buffer, "%d", port);
	return buffer;
}

  
             
   
#ifndef _WIN32
static Sigfunc *mySignal(int signo, Sigfunc *func)
{
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
#if defined (SA_RESTART)
	act.sa_flags = SA_RESTART;
#else
	act.sa_flags = 0;
#endif
	if (sigaction(signo, &act, &oact) < 0)
		return SIG_ERR;
	return oact.sa_handler;
}

  
               
   
static void sigHandler(int signr)
{
	mySignal(signr, sigHandler);
	switch (signr)
	{
	case SIGPIPE:
		dbgPrintf(DEFCOL, "Catch SIG_PIPE\n");


		break;
	}
}
#endif
