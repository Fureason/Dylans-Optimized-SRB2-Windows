 
/* Project : miniupnp
 * Website : http://miniupnp.free.fr/ or https://miniupnp.tuxfamily.org/
 * Author : Thomas Bernard
 * Copyright (c) 2005-2020 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#define MAXHOSTNAMELEN 64
#include "win32_snprintf.h"
#define socklen_t int
#ifndef strncasecmp
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#define strncasecmp _memicmp
#else  
#define strncasecmp memicmp
#endif  
#endif  
#else  
#include <unistd.h>
#include <sys/param.h>
#if defined(__amigaos__) && !defined(__amigaos4__)
#define socklen_t int
#else  
#include <sys/select.h>
#endif  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#define closesocket close
#include <strings.h>
#endif  
#ifdef __GNU__
#define MAXHOSTNAMELEN 64
#endif  

#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif  


#include "miniupnpcstrings.h"
#include "miniwget.h"
#include "connecthostport.h"
#include "receivedata.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

  
                                      
                                                        
                                                                
                           
   
void *
getHTTPResponse(SOCKET s, int * size, int * status_code)
{
	char buf[2048];
	int n;
	int endofheaders = 0;
	int chunked = 0;
	int content_length = -1;
	unsigned int chunksize = 0;
	unsigned int bytestocopy = 0;
	 
	char * header_buf;
	unsigned int header_buf_len = 2048;
	unsigned int header_buf_used = 0;
	char * content_buf;
	unsigned int content_buf_len = 2048;
	unsigned int content_buf_used = 0;
	char chunksize_buf[32];
	unsigned int chunksize_buf_index;
#ifdef DEBUG
	char * reason_phrase = NULL;
	int reason_phrase_len = 0;
#endif

	if(status_code) *status_code = -1;
	header_buf = malloc(header_buf_len);
	if(header_buf == NULL)
	{
#ifdef DEBUG
		fprintf(stderr, "%s: Memory allocation error\n", "getHTTPResponse");
#endif  
		*size = -1;
		return NULL;
	}
	content_buf = malloc(content_buf_len);
	if(content_buf == NULL)
	{
		free(header_buf);
#ifdef DEBUG
		fprintf(stderr, "%s: Memory allocation error\n", "getHTTPResponse");
#endif  
		*size = -1;
		return NULL;
	}
	chunksize_buf[0] = '\0';
	chunksize_buf_index = 0;

	while((n = receivedata(s, buf, sizeof(buf), 5000, NULL)) > 0)
	{
		if(endofheaders == 0)
		{
			int i;
			int linestart=0;
			int colon=0;
			int valuestart=0;
			if(header_buf_used + n > header_buf_len) {
				char * tmp = realloc(header_buf, header_buf_used + n);
				if(tmp == NULL) {
					 
					free(header_buf);
					free(content_buf);
					*size = -1;
					return NULL;
				}
				header_buf = tmp;
				header_buf_len = header_buf_used + n;
			}
			memcpy(header_buf + header_buf_used, buf, n);
			header_buf_used += n;
			                                          
                             
			i = 0;
			while(i < ((int)header_buf_used-1) && (endofheaders == 0)) {
				if(header_buf[i] == '\r') {
					i++;
					if(header_buf[i] == '\n') {
						i++;
						if(i < (int)header_buf_used && header_buf[i] == '\r') {
							i++;
							if(i < (int)header_buf_used && header_buf[i] == '\n') {
								endofheaders = i+1;
							}
						}
					}
				} else if(header_buf[i] == '\n') {
					i++;
					if(header_buf[i] == '\n') {
						endofheaders = i+1;
					}
				}
				i++;
			}
			if(endofheaders == 0)
				continue;
			 
			for(i = 0; i < endofheaders - 1; i++) {
				if(linestart > 0 && colon <= linestart && header_buf[i]==':')
				{
					colon = i;
					while(i < (endofheaders-1)
					      && (header_buf[i+1] == ' ' || header_buf[i+1] == '\t'))
						i++;
					valuestart = i + 1;
				}
				 
				else if(header_buf[i]=='\r' || header_buf[i]=='\n')
				{
					if(linestart == 0 && status_code)
					{
						              
                                                             
						int sp;
						for(sp = 0; sp < i - 1; sp++)
							if(header_buf[sp] == ' ')
							{
								if(*status_code < 0)
								{
									if (header_buf[sp+1] >= '1' && header_buf[sp+1] <= '9')
										*status_code = atoi(header_buf + sp + 1);
								}
								else
								{
#ifdef DEBUG
									reason_phrase = header_buf + sp + 1;
									reason_phrase_len = i - sp - 1;
#endif
									break;
								}
							}
#ifdef DEBUG
						printf("HTTP status code = %d, Reason phrase = %.*s\n",
						       *status_code, reason_phrase_len, reason_phrase);
#endif
					}
					else if(colon > linestart && valuestart > colon)
					{
#ifdef DEBUG
						printf("header='%.*s', value='%.*s'\n",
						       colon-linestart, header_buf+linestart,
						       i-valuestart, header_buf+valuestart);
#endif
						if(0==strncasecmp(header_buf+linestart, "content-length", colon-linestart))
						{
							content_length = atoi(header_buf+valuestart);
#ifdef DEBUG
							printf("Content-Length: %d\n", content_length);
#endif
						}
						else if(0==strncasecmp(header_buf+linestart, "transfer-encoding", colon-linestart)
						   && 0==strncasecmp(header_buf+valuestart, "chunked", 7))
						{
#ifdef DEBUG
							printf("chunked transfer-encoding!\n");
#endif
							chunked = 1;
						}
					}
					while((i < (int)header_buf_used) && (header_buf[i]=='\r' || header_buf[i] == '\n'))
						i++;
					linestart = i;
					colon = linestart;
					valuestart = 0;
				}
			}
			 
			n = header_buf_used - endofheaders;
			memcpy(buf, header_buf + endofheaders, n);
			 
		}
		                                      
                                                     
		 
		if(chunked)
		{
			int i = 0;
			while(i < n)
			{
				if(chunksize == 0)
				{
					 
					if(chunksize_buf_index == 0) {
						 
						if(buf[i] == '\r') i++;
						if(i<n && buf[i] == '\n') i++;
					}
					while(i<n && isxdigit(buf[i])
					     && chunksize_buf_index < (sizeof(chunksize_buf)-1))
					{
						chunksize_buf[chunksize_buf_index++] = buf[i];
						chunksize_buf[chunksize_buf_index] = '\0';
						i++;
					}
					while(i<n && buf[i] != '\r' && buf[i] != '\n')
						i++;  
					if(i<n && buf[i] == '\r') i++;
					if(i<n && buf[i] == '\n') {
						unsigned int j;
						for(j = 0; j < chunksize_buf_index; j++) {
						if(chunksize_buf[j] >= '0'
						   && chunksize_buf[j] <= '9')
							chunksize = (chunksize << 4) + (chunksize_buf[j] - '0');
						else
							chunksize = (chunksize << 4) + ((chunksize_buf[j] | 32) - 'a' + 10);
						}
						chunksize_buf[0] = '\0';
						chunksize_buf_index = 0;
						i++;
					} else {
						 
						continue;
					}
#ifdef DEBUG
					printf("chunksize = %u (%x)\n", chunksize, chunksize);
#endif
					if(chunksize == 0)
					{
#ifdef DEBUG
						printf("end of HTTP content - %d %d\n", i, n);
						 
#endif
						goto end_of_stream;
					}
				}
				 
				bytestocopy = (chunksize < (unsigned int)(n - i))?chunksize:(unsigned int)(n - i);
				if((content_buf_used + bytestocopy) > content_buf_len)
				{
					char * tmp;
					if((content_length >= 0) && ((unsigned int)content_length >= (content_buf_used + bytestocopy))) {
						content_buf_len = content_length;
					} else {
						content_buf_len = content_buf_used + bytestocopy;
					}
					tmp = realloc(content_buf, content_buf_len);
					if(tmp == NULL) {
						 
						free(content_buf);
						free(header_buf);
						*size = -1;
						return NULL;
					}
					content_buf = tmp;
				}
				memcpy(content_buf + content_buf_used, buf + i, bytestocopy);
				content_buf_used += bytestocopy;
				i += bytestocopy;
				chunksize -= bytestocopy;
			}
		}
		else
		{
			 
			if(content_length > 0
			   && (content_buf_used + n) > (unsigned int)content_length) {
				 
				n = content_length - content_buf_used;
			}
			if(content_buf_used + n > content_buf_len)
			{
				char * tmp;
				if(content_length >= 0
				   && (unsigned int)content_length >= (content_buf_used + n)) {
					content_buf_len = content_length;
				} else {
					content_buf_len = content_buf_used + n;
				}
				tmp = realloc(content_buf, content_buf_len);
				if(tmp == NULL) {
					 
					free(content_buf);
					free(header_buf);
					*size = -1;
					return NULL;
				}
				content_buf = tmp;
			}
			memcpy(content_buf + content_buf_used, buf, n);
			content_buf_used += n;
		}
		 
		if(content_length > 0 && content_buf_used >= (unsigned int)content_length)
		{
#ifdef DEBUG
			printf("End of HTTP content\n");
#endif
			break;
		}
	}
end_of_stream:
	free(header_buf);
	*size = content_buf_used;
	if(content_buf_used == 0)
	{
		free(content_buf);
		content_buf = NULL;
	}
	return content_buf;
}

                
                   
                                      
static void *
miniwget3(const char * host,
          unsigned short port, const char * path,
          int * size, char * addr_str, int addr_str_len,
          const char * httpversion, unsigned int scope_id,
          int * status_code)
{
	char buf[2048];
	SOCKET s;
	int n;
	int len;
	int sent;
	void * content;

	*size = 0;
	s = connecthostport(host, port, scope_id);
	if(ISINVALID(s))
		return NULL;

	 
	if(addr_str)
	{
		struct sockaddr_storage saddr;
		socklen_t saddrlen;

		saddrlen = sizeof(saddr);
		if(getsockname(s, (struct sockaddr *)&saddr, &saddrlen) < 0)
		{
			perror("getsockname");
		}
		else
		{
#if defined(__amigaos__) && !defined(__amigaos4__)
	                                                                                                
                                                                          
                                                             
                                                                    
   
                                                                       
     
			 
			strncpy(addr_str, inet_ntoa(((struct sockaddr_in *)&saddr)->sin_addr), addr_str_len);
#else
#if 0
			if(saddr.sa_family == AF_INET6) {
				inet_ntop(AF_INET6,
				          &(((struct sockaddr_in6 *)&saddr)->sin6_addr),
				          addr_str, addr_str_len);
			} else {
				inet_ntop(AF_INET,
				          &(((struct sockaddr_in *)&saddr)->sin_addr),
				          addr_str, addr_str_len);
			}
#endif
			                                                             
                                                  
			n = getnameinfo((const struct sockaddr *)&saddr, saddrlen,
			                addr_str, addr_str_len,
			                NULL, 0,
			                NI_NUMERICHOST | NI_NUMERICSERV);
			if(n != 0) {
#ifdef _WIN32
				fprintf(stderr, "getnameinfo() failed : %d\n", n);
#else
				fprintf(stderr, "getnameinfo() failed : %s\n", gai_strerror(n));
#endif
			}
#endif
		}
#ifdef DEBUG
		printf("address miniwget : %s\n", addr_str);
#endif
	}

	len = snprintf(buf, sizeof(buf),
                 "GET %s HTTP/%s\r\n"
			     "Host: %s:%d\r\n"
				 "Connection: Close\r\n"
				 "User-Agent: " OS_STRING ", " UPNP_VERSION_STRING ", MiniUPnPc/" MINIUPNPC_VERSION_STRING "\r\n"

				 "\r\n",
			   path, httpversion, host, port);
	if ((unsigned int)len >= sizeof(buf))
	{
		closesocket(s);
		return NULL;
	}
	sent = 0;
	 
	while(sent < len)
	{
		n = send(s, buf+sent, len-sent, 0);
		if(n < 0)
		{
			perror("send");
			closesocket(s);
			return NULL;
		}
		else
		{
			sent += n;
		}
	}
	content = getHTTPResponse(s, size, status_code);
	closesocket(s);
	return content;
}

                
                                                         
static void *
miniwget2(const char * host,
          unsigned short port, const char * path,
          int * size, char * addr_str, int addr_str_len,
          unsigned int scope_id, int * status_code)
{
	char * respbuffer;

#if 1
	respbuffer = miniwget3(host, port, path, size,
	                       addr_str, addr_str_len, "1.1",
	                       scope_id, status_code);
#else
	respbuffer = miniwget3(host, port, path, size,
	                       addr_str, addr_str_len, "1.0",
	                       scope_id, status_code);
	if (*size == 0)
	{
#ifdef DEBUG
		printf("Retrying with HTTP/1.1\n");
#endif
		free(respbuffer);
		respbuffer = miniwget3(host, port, path, size,
		                       addr_str, addr_str_len, "1.1",
		                       scope_id, status_code);
	}
#endif
	return respbuffer;
}




             
              
                                      
                                                                      
                               
                                                
  
                  
                 
                            
int
parseURL(const char * url,
         char * hostname, unsigned short * port,
         char * * path, unsigned int * scope_id)
{
	char * p1, *p2, *p3;
	if(!url)
		return 0;
	p1 = strstr(url, "://");
	if(!p1)
		return 0;
	p1 += 3;
	if(  (url[0]!='h') || (url[1]!='t')
	   ||(url[2]!='t') || (url[3]!='p'))
		return 0;
	memset(hostname, 0, MAXHOSTNAMELEN + 1);
	if(*p1 == '[')
	{
		 
		char * scope;
		scope = strchr(p1, '%');
		p2 = strchr(p1, ']');
		if(p2 && scope && scope < p2 && scope_id) {
			 
#ifdef IF_NAMESIZE
			char tmp[IF_NAMESIZE];
			int l;
			scope++;
			 
			if(scope[0] == '2' && scope[1] == '5')
				scope += 2;	 
			l = p2 - scope;
			if(l >= IF_NAMESIZE)
				l = IF_NAMESIZE - 1;
			memcpy(tmp, scope, l);
			tmp[l] = '\0';
			*scope_id = if_nametoindex(tmp);
			if(*scope_id == 0) {
				*scope_id = (unsigned int)strtoul(tmp, NULL, 10);
			}
#else
			 
			char tmp[8];
			size_t l;
			scope++;
			 
			if(scope[0] == '2' && scope[1] == '5')
				scope += 2;	 
			l = p2 - scope;
			if(l >= sizeof(tmp))
				l = sizeof(tmp) - 1;
			memcpy(tmp, scope, l);
			tmp[l] = '\0';
			*scope_id = (unsigned int)strtoul(tmp, NULL, 10);
#endif
		}
		p3 = strchr(p1, '/');
		if(p2 && p3)
		{
			p2++;
			strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p2-p1)));
			if(*p2 == ':')
			{
				*port = 0;
				p2++;
				while( (*p2 >= '0') && (*p2 <= '9'))
				{
					*port *= 10;
					*port += (unsigned short)(*p2 - '0');
					p2++;
				}
			}
			else
			{
				*port = 80;
			}
			*path = p3;
			return 1;
		}
	}
	p2 = strchr(p1, ':');
	p3 = strchr(p1, '/');
	if(!p3)
		return 0;
	if(!p2 || (p2>p3))
	{
		strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p3-p1)));
		*port = 80;
	}
	else
	{
		strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p2-p1)));
		*port = 0;
		p2++;
		while( (*p2 >= '0') && (*p2 <= '9'))
		{
			*port *= 10;
			*port += (unsigned short)(*p2 - '0');
			p2++;
		}
	}
	*path = p3;
	return 1;
}

void *
miniwget(const char * url, int * size,
         unsigned int scope_id, int * status_code)
{
	unsigned short port;
	char * path;
	 
	char hostname[MAXHOSTNAMELEN+1];
	*size = 0;
	if(!parseURL(url, hostname, &port, &path, &scope_id))
		return NULL;
#ifdef DEBUG
	printf("parsed url : hostname='%s' port=%hu path='%s' scope_id=%u\n",
	       hostname, port, path, scope_id);
#endif
	return miniwget2(hostname, port, path, size, 0, 0, scope_id, status_code);
}

void *
miniwget_getaddr(const char * url, int * size,
                 char * addr, int addrlen, unsigned int scope_id,
                 int * status_code)
{
	unsigned short port;
	char * path;
	 
	char hostname[MAXHOSTNAMELEN+1];
	*size = 0;
	if(addr)
		addr[0] = '\0';
	if(!parseURL(url, hostname, &port, &path, &scope_id))
		return NULL;
#ifdef DEBUG
	printf("parsed url : hostname='%s' port=%hu path='%s' scope_id=%u\n",
	       hostname, port, path, scope_id);
#endif
	return miniwget2(hostname, port, path, size, addr, addrlen, scope_id, status_code);
}
