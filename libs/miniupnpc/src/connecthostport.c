 
/* vim: tabstop=4 shiftwidth=4 noexpandtab
 * Project : miniupnp
 * Author : Thomas Bernard
 * Copyright (c) 2010-2020 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution. */

                                       
                                                                  
#ifdef NO_GETADDRINFO
#define USE_GETHOSTBYNAME
#endif

#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#define MAXHOSTNAMELEN 64
#include "win32_snprintf.h"
#define herror
#define socklen_t int
#else  
#include <unistd.h>
#include <sys/types.h>
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
#include <sys/time.h>
#endif  
#include <sys/param.h>
#include <sys/select.h>
#include <errno.h>
#define closesocket close
#include <netdb.h>
#include <netinet/in.h>
                                                                     
                               
#define MINIUPNPC_IGNORE_EINTR
#include <sys/socket.h>
#include <sys/select.h>
#endif  

#if defined(__amigaos__) || defined(__amigaos4__)
#define herror(A) printf("%s\n", A)
#endif

#include "connecthostport.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

                    
                                                       
                            
SOCKET connecthostport(const char * host, unsigned short port,
                       unsigned int scope_id)
{
	SOCKET s;
	int n;
#ifdef USE_GETHOSTBYNAME
	struct sockaddr_in dest;
	struct hostent *hp;
#else  
	char tmp_host[MAXHOSTNAMELEN+1];
	char port_str[8];
	struct addrinfo *ai, *p;
	struct addrinfo hints;
#endif  
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
	struct timeval timeout;
#endif  

#ifdef USE_GETHOSTBYNAME
	hp = gethostbyname(host);
	if(hp == NULL)
	{
		herror(host);
		return INVALID_SOCKET;
	}
	memcpy(&dest.sin_addr, hp->h_addr, sizeof(dest.sin_addr));
	memset(dest.sin_zero, 0, sizeof(dest.sin_zero));
	s = socket(PF_INET, SOCK_STREAM, 0);
	if(ISINVALID(s))
	{
		PRINT_SOCKET_ERROR("socket");
		return INVALID_SOCKET;
	}
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
	 
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0)
	{
		PRINT_SOCKET_ERROR("setsockopt SO_RCVTIMEO");
	}
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	if(setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)) < 0)
	{
		PRINT_SOCKET_ERROR("setsockopt SO_SNDTIMEO");
	}
#endif  
	dest.sin_family = AF_INET;
	dest.sin_port = htons(port);
	n = connect(s, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
#ifdef MINIUPNPC_IGNORE_EINTR
	                                                                    
                                                                   
                                            
	while(n < 0 && (errno == EINTR || errno == EINPROGRESS))
	{
		socklen_t len;
		fd_set wset;
		int err;
		FD_ZERO(&wset);
		FD_SET(s, &wset);
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		n = select(s + 1, NULL, &wset, NULL, &timeout);
#else
		n = select(s + 1, NULL, &wset, NULL, NULL);
#endif
		if(n == -1 && errno == EINTR)
			continue;
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
		if(n == 0) {
			errno = ETIMEDOUT;
			n = -1;
			break;
		}
#endif
		 
		 
		len = sizeof(err);
		if(getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
			PRINT_SOCKET_ERROR("getsockopt");
			closesocket(s);
			return INVALID_SOCKET;
		}
		if(err != 0) {
			errno = err;
			n = -1;
		}
	}
#endif  
	if(n<0)
	{
		PRINT_SOCKET_ERROR("connect");
		closesocket(s);
		return INVALID_SOCKET;
	}
#else  
	 
	memset(&hints, 0, sizeof(hints));
	 
#ifdef AI_NUMERICSERV
	hints.ai_flags = AI_NUMERICSERV;
#endif
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;  
	 
	snprintf(port_str, sizeof(port_str), "%hu", port);
	if(host[0] == '[')
	{
		 
		int i, j;
		for(i = 0, j = 1; host[j] && (host[j] != ']') && i < MAXHOSTNAMELEN; i++, j++)
		{
			tmp_host[i] = host[j];
			if(0 == strncmp(host+j, "%25", 3))	 
				j+=2;							 
		}
		tmp_host[i] = '\0';
	}
	else
	{
		strncpy(tmp_host, host, MAXHOSTNAMELEN);
	}
	tmp_host[MAXHOSTNAMELEN] = '\0';
	n = getaddrinfo(tmp_host, port_str, &hints, &ai);
	if(n != 0)
	{
#ifdef _WIN32
		fprintf(stderr, "getaddrinfo() error : %d\n", n);
#else
		fprintf(stderr, "getaddrinfo() error : %s\n", gai_strerror(n));
#endif
		return INVALID_SOCKET;
	}
	s = INVALID_SOCKET;
	for(p = ai; p; p = p->ai_next)
	{
		if(!ISINVALID(s))
			closesocket(s);
#ifdef DEBUG
		printf("ai_family=%d ai_socktype=%d ai_protocol=%d (PF_INET=%d, PF_INET6=%d)\n",
		       p->ai_family, p->ai_socktype, p->ai_protocol, PF_INET, PF_INET6);
#endif
		s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(ISINVALID(s))
			continue;
		if(p->ai_addr->sa_family == AF_INET6 && scope_id > 0) {
			struct sockaddr_in6 * addr6 = (struct sockaddr_in6 *)p->ai_addr;
			addr6->sin6_scope_id = scope_id;
		}
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
		 
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0)
		{
			PRINT_SOCKET_ERROR("setsockopt");
		}
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		if(setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)) < 0)
		{
			PRINT_SOCKET_ERROR("setsockopt");
		}
#endif  
		n = connect(s, p->ai_addr, MSC_CAST_INT p->ai_addrlen);
#ifdef MINIUPNPC_IGNORE_EINTR
		                                                                    
                                                                    
                                             
		while(n < 0 && (errno == EINTR || errno == EINPROGRESS))
		{
			socklen_t len;
			fd_set wset;
			int err;
			FD_ZERO(&wset);
			FD_SET(s, &wset);
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
			timeout.tv_sec = 3;
			timeout.tv_usec = 0;
			n = select(s + 1, NULL, &wset, NULL, &timeout);
#else
			n = select(s + 1, NULL, &wset, NULL, NULL);
#endif
			if(n == -1 && errno == EINTR)
				continue;
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
			if(n == 0) {
				errno = ETIMEDOUT;
				n = -1;
				break;
			}
#endif
			 
			 
			len = sizeof(err);
			if(getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
				PRINT_SOCKET_ERROR("getsockopt");
				closesocket(s);
				freeaddrinfo(ai);
				return INVALID_SOCKET;
			}
			if(err != 0) {
				errno = err;
				n = -1;
			}
		}
#endif  
		if(n >= 0)	 
			break;
	}
	freeaddrinfo(ai);
	if(ISINVALID(s))
	{
		PRINT_SOCKET_ERROR("socket");
		return INVALID_SOCKET;
	}
	if(n < 0)
	{
		PRINT_SOCKET_ERROR("connect");
		closesocket(s);
		return INVALID_SOCKET;
	}
#endif  
	return s;
}
