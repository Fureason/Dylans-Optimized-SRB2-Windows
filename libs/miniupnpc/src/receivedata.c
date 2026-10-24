 
/* Project : miniupnp
 * Website : http://miniupnp.free.fr/
 * Author : Thomas Bernard
 * Copyright (c) 2011-2021 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution. */

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else  
#include <unistd.h>
#if defined(__amigaos__) && !defined(__amigaos4__)
#define socklen_t int
#else  
#include <sys/select.h>
#endif  
#include <sys/socket.h>
#include <netinet/in.h>
#if !defined(__amigaos__) && !defined(__amigaos4__)
#include <poll.h>
#endif	 
#include <errno.h>
#define MINIUPNPC_IGNORE_EINTR
#endif  

#include "receivedata.h"

int
receivedata(SOCKET socket,
            char * data, int length,
            int timeout, unsigned int * scope_id)
{
#ifdef MINIUPNPC_GET_SRC_ADDR
	struct sockaddr_storage src_addr;
	socklen_t src_addr_len = sizeof(src_addr);
#endif	 
    int n;
#if !defined(_WIN32) && !defined(__amigaos__) && !defined(__amigaos4__)
	 
    struct pollfd fds[1];  
#ifdef MINIUPNPC_IGNORE_EINTR
    do {
#endif	 
        fds[0].fd = socket;
        fds[0].events = POLLIN;
        n = poll(fds, 1, timeout);
#ifdef MINIUPNPC_IGNORE_EINTR
    } while(n < 0 && errno == EINTR);
#endif	 
    if(n < 0) {
        PRINT_SOCKET_ERROR("poll");
        return -1;
    } else if(n == 0) {
		 
        return 0;
    }
#else	 
	 
    fd_set socketSet;
    TIMEVAL timeval;
    FD_ZERO(&socketSet);
    FD_SET(socket, &socketSet);
    timeval.tv_sec = timeout / 1000;
    timeval.tv_usec = (timeout % 1000) * 1000;
    n = select(FD_SETSIZE, &socketSet, NULL, NULL, &timeval);
    if(n < 0) {
        PRINT_SOCKET_ERROR("select");
        return -1;
    } else if(n == 0) {
        return 0;
    }
#endif	 
#ifdef MINIUPNPC_GET_SRC_ADDR
	memset(&src_addr, 0, sizeof(src_addr));
	n = recvfrom(socket, data, length, 0,
	             (struct sockaddr *)&src_addr, &src_addr_len);
#else	 
	n = recv(socket, data, length, 0);
#endif	 
	if(n<0) {
		PRINT_SOCKET_ERROR("recv");
	}
#ifdef MINIUPNPC_GET_SRC_ADDR
	if (src_addr.ss_family == AF_INET6) {
		const struct sockaddr_in6 * src_addr6 = (struct sockaddr_in6 *)&src_addr;
#ifdef DEBUG
		printf("scope_id=%u\n", src_addr6->sin6_scope_id);
#endif	 
		if(scope_id)
			*scope_id = src_addr6->sin6_scope_id;
	} else {
		if(scope_id)
			*scope_id = 0;
	}
#else	 
	if(scope_id)
		*scope_id = 0;
#endif	 
	return n;
}

