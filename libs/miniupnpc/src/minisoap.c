 
/* vim: tabstop=4 shiftwidth=4 noexpandtab
 * Project : miniupnp
 * Author : Thomas Bernard
 * Copyright (c) 2005-2020 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution.
 *
 * Minimal SOAP implementation for UPnP protocol.
 */
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <winsock2.h>
#include "win32_snprintf.h"
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include "minisoap.h"
#include "miniupnpcstrings.h"

 
#include <stdlib.h>

                                                         
                                          
static int
httpWrite(SOCKET fd, const char * body, int bodysize,
          const char * headers, int headerssize)
{
	int n = 0;
	 
	                
                                   
	                                                      
                                                      
	char * p;
	 
	p = malloc(headerssize+bodysize);
	if(!p)
	  return -1;
	memcpy(p, headers, headerssize);
	memcpy(p+headerssize, body, bodysize);
	 
	n = send(fd, p, headerssize+bodysize, 0);
	if(n<0) {
	  PRINT_SOCKET_ERROR("send");
	}
	 
	 
#if 0
#ifdef _WIN32
	if(shutdown(fd, SD_SEND)<0) {
#else
	if(shutdown(fd, SHUT_WR)<0)	{  
#endif
		PRINT_SOCKET_ERROR("shutdown");
	}
#endif
	free(p);
	return n;
}

 
int soapPostSubmit(SOCKET fd,
                   const char * url,
				   const char * host,
				   unsigned short port,
				   const char * action,
				   const char * body,
				   const char * httpversion)
{
	char headerbuf[512];
	int headerssize;
	char portstr[8];
	int bodysize = (int)strlen(body);
	                                                
                                                           
                                     
                                                                       
                                                                       
                         
     
	portstr[0] = '\0';
	if(port != 80)
		snprintf(portstr, sizeof(portstr), ":%hu", port);
	headerssize = snprintf(headerbuf, sizeof(headerbuf),
                       "POST %s HTTP/%s\r\n"
	                   "Host: %s%s\r\n"
					   "User-Agent: " OS_STRING ", " UPNP_VERSION_STRING ", MiniUPnPc/" MINIUPNPC_VERSION_STRING "\r\n"
	                   "Content-Length: %d\r\n"
					   "Content-Type: text/xml\r\n"
					   "SOAPAction: \"%s\"\r\n"
					   "Connection: Close\r\n"
					   "Cache-Control: no-cache\r\n"	 
					   "Pragma: no-cache\r\n"
					   "\r\n",
					   url, httpversion, host, portstr, bodysize, action);
	if ((unsigned int)headerssize >= sizeof(headerbuf))
		return -1;
#ifdef DEBUG
	                                                      
                               
   
	printf("SOAP request : POST %s HTTP/%s - Host: %s%s\n",
	        url, httpversion, host, portstr);
	printf("SOAPAction: \"%s\" - Content-Length: %d\n", action, bodysize);
	printf("Headers :\n%s", headerbuf);
	printf("Body :\n%s\n", body);
#endif
	return httpWrite(fd, body, bodysize, headerbuf, headerssize);
}


