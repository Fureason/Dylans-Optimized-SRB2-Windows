 
/* vim: tabstop=4 shiftwidth=4 noexpandtab
 * Project : miniupnp
 * Web : http://miniupnp.free.fr/ or https://miniupnp.tuxfamily.org/
 * Author : Thomas BERNARD
 * copyright (c) 2005-2021 Thomas Bernard
 * This software is subjet to the conditions detailed in the
 * provided LICENSE file. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
 
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <iphlpapi.h>
#include "win32_snprintf.h"
#define strdup _strdup
#ifndef strncasecmp
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#define strncasecmp _memicmp
#else  
#define strncasecmp memicmp
#endif  
#endif  
#define MAXHOSTNAMELEN 64
#else  
 
#include <unistd.h>
#if defined(__amigaos__) && !defined(__amigaos4__)
 
#define socklen_t int
#else
#include <sys/select.h>
#endif
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#if !defined(__amigaos__) && !defined(__amigaos4__)
#include <poll.h>
#endif
#include <strings.h>
#include <errno.h>
#define closesocket close
#endif  
#ifdef __GNU__
#define MAXHOSTNAMELEN 64
#endif


#include "miniupnpc.h"
#include "minissdpc.h"
#include "miniwget.h"
#include "miniwget_private.h"
#include "minisoap.h"
#include "minixml.h"
#include "upnpcommands.h"
#include "connecthostport.h"
#include "addr_is_reserved.h"

 
#define COMPARE(str, cstr) (0==strncmp(str, cstr, sizeof(cstr) - 1))

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#define SOAPPREFIX "s"
#define SERVICEPREFIX "u"
#define SERVICEPREFIX2 'u'

 
MINIUPNP_LIBSPEC void parserootdesc(const char * buffer, int bufsize, struct IGDdatas * data)
{
	struct xmlparser parser;
	 
	parser.xmlstart = buffer;
	parser.xmlsize = bufsize;
	parser.data = data;
	parser.starteltfunc = IGDstartelt;
	parser.endeltfunc = IGDendelt;
	parser.datafunc = IGDdata;
	parser.attfunc = 0;
	parsexml(&parser);
#ifdef DEBUG
	printIGD(data);
#endif
}

                       
                  
                  
                 
                    
static char *
simpleUPnPcommand2(SOCKET s, const char * url, const char * service,
                   const char * action, struct UPNParg * args,
                   int * bufsize, const char * httpversion)
{
	char hostname[MAXHOSTNAMELEN+1];
	unsigned short port = 0;
	char * path;
	char soapact[128];
	char soapbody[2048];
	int soapbodylen;
	char * buf;
	int n;
	int status_code;

	*bufsize = 0;
	snprintf(soapact, sizeof(soapact), "%s#%s", service, action);
	if(args==NULL)
	{
		soapbodylen = snprintf(soapbody, sizeof(soapbody),
						  "<?xml version=\"1.0\"?>\r\n"
						  "<" SOAPPREFIX ":Envelope "
						  "xmlns:" SOAPPREFIX "=\"http://schemas.xmlsoap.org/soap/envelope/\" "
						  SOAPPREFIX ":encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
						  "<" SOAPPREFIX ":Body>"
						  "<" SERVICEPREFIX ":%s xmlns:" SERVICEPREFIX "=\"%s\">"
						  "</" SERVICEPREFIX ":%s>"
						  "</" SOAPPREFIX ":Body></" SOAPPREFIX ":Envelope>"
						  "\r\n", action, service, action);
		if ((unsigned int)soapbodylen >= sizeof(soapbody))
			return NULL;
	}
	else
	{
		char * p;
		const char * pe, * pv;
		const char * const pend = soapbody + sizeof(soapbody);
		soapbodylen = snprintf(soapbody, sizeof(soapbody),
						"<?xml version=\"1.0\"?>\r\n"
						"<" SOAPPREFIX ":Envelope "
						"xmlns:" SOAPPREFIX "=\"http://schemas.xmlsoap.org/soap/envelope/\" "
						SOAPPREFIX ":encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
						"<" SOAPPREFIX ":Body>"
						"<" SERVICEPREFIX ":%s xmlns:" SERVICEPREFIX "=\"%s\">",
						action, service);
		if ((unsigned int)soapbodylen >= sizeof(soapbody))
			return NULL;
		p = soapbody + soapbodylen;
		while(args->elt)
		{
			if(p >= pend)  
				return NULL;
			*(p++) = '<';

			pe = args->elt;
			while(p < pend && *pe)
				*(p++) = *(pe++);

			if(p >= pend)  
				return NULL;
			*(p++) = '>';

			if((pv = args->val))
			{
				while(p < pend && *pv)
					*(p++) = *(pv++);
			}

			if((p+2) > pend)  
				return NULL;
			*(p++) = '<';
			*(p++) = '/';

			pe = args->elt;
			while(p < pend && *pe)
				*(p++) = *(pe++);

			if(p >= pend)  
				return NULL;
			*(p++) = '>';

			args++;
		}
		if((p+4) > pend)  
			return NULL;
		*(p++) = '<';
		*(p++) = '/';
		*(p++) = SERVICEPREFIX2;
		*(p++) = ':';

		pe = action;
		while(p < pend && *pe)
			*(p++) = *(pe++);

		strncpy(p, "></" SOAPPREFIX ":Body></" SOAPPREFIX ":Envelope>\r\n",
		        pend - p);
		if(soapbody[sizeof(soapbody)-1])  
			return NULL;
	}
	if(!parseURL(url, hostname, &port, &path, NULL)) return NULL;
	if(ISINVALID(s)) {
		s = connecthostport(hostname, port, 0);
		if(ISINVALID(s)) {
			 
			return NULL;
		}
	}

	n = soapPostSubmit(s, path, hostname, port, soapact, soapbody, httpversion);
	if(n<=0) {
#ifdef DEBUG
		printf("Error sending SOAP request\n");
#endif
		closesocket(s);
		return NULL;
	}

	buf = getHTTPResponse(s, bufsize, &status_code);
#ifdef DEBUG
	if(*bufsize > 0 && buf)
	{
		printf("HTTP %d SOAP Response :\n%.*s\n", status_code, *bufsize, buf);
	}
	else
	{
		printf("HTTP %d, empty SOAP response. size=%d\n", status_code, *bufsize);
	}
#endif
	closesocket(s);
	return buf;
}

                      
                  
                  
                 
                       
char *
simpleUPnPcommand(int s, const char * url, const char * service,
                  const char * action, struct UPNParg * args,
                  int * bufsize)
{
	char * buf;

#if 1
	buf = simpleUPnPcommand2((SOCKET)s, url, service, action, args, bufsize, "1.1");
#else
	buf = simpleUPnPcommand2((SOCKET)s, url, service, action, args, bufsize, "1.0");
	if (!buf || *bufsize == 0)
	{
#if DEBUG
	    printf("Error or no result from SOAP request; retrying with HTTP/1.1\n");
#endif
		buf = simpleUPnPcommand2((SOCKET)s, url, service, action, args, bufsize, "1.1");
	}
#endif
	return buf;
}

                          
                                                        
                        
                                                  
                                  
                  
                                                      
                               
MINIUPNP_LIBSPEC struct UPNPDev *
upnpDiscoverDevices(const char * const deviceTypes[],
                    int delay, const char * multicastif,
                    const char * minissdpdsock, int localport,
                    int ipv6, unsigned char ttl,
                    int * error,
                    int searchalltypes)
{
	struct UPNPDev * tmp;
	struct UPNPDev * devlist = 0;
#if !defined(_WIN32) && !defined(__amigaos__) && !defined(__amigaos4__)
	int deviceIndex;
#endif  

	if(error)
		*error = UPNPDISCOVER_UNKNOWN_ERROR;
#if !defined(_WIN32) && !defined(__amigaos__) && !defined(__amigaos4__)
	 
	if(!minissdpdsock)
		minissdpdsock = "/var/run/minissdpd.sock";
	if(minissdpdsock[0] != '\0') {
		for(deviceIndex = 0; deviceTypes[deviceIndex]; deviceIndex++) {
			struct UPNPDev * minissdpd_devlist;
			int only_rootdevice = 1;
			minissdpd_devlist = getDevicesFromMiniSSDPD(deviceTypes[deviceIndex],
			                                            minissdpdsock, 0);
			if(minissdpd_devlist) {
#ifdef DEBUG
				printf("returned by MiniSSDPD: %s\t%s\n",
				       minissdpd_devlist->st, minissdpd_devlist->descURL);
#endif  
				if(!strstr(minissdpd_devlist->st, "rootdevice"))
					only_rootdevice = 0;
				for(tmp = minissdpd_devlist; tmp->pNext != NULL; tmp = tmp->pNext) {
#ifdef DEBUG
					printf("returned by MiniSSDPD: %s\t%s\n",
					       tmp->pNext->st, tmp->pNext->descURL);
#endif  
					if(!strstr(tmp->st, "rootdevice"))
						only_rootdevice = 0;
				}
				tmp->pNext = devlist;
				devlist = minissdpd_devlist;
				if(!searchalltypes && !only_rootdevice)
					break;
			}
		}
	}
	for(tmp = devlist; tmp != NULL; tmp = tmp->pNext) {
		 
		if(!strstr(tmp->st, "rootdevice")) {
			if(error)
				*error = UPNPDISCOVER_SUCCESS;
			return devlist;
		}
	}
#else	 
	(void)minissdpdsock;  
#endif	 

	 
	{
		struct UPNPDev * discovered_devlist;
		discovered_devlist = ssdpDiscoverDevices(deviceTypes, delay, multicastif, localport,
		                                         ipv6, ttl, error, searchalltypes);
		if(devlist == NULL)
			devlist = discovered_devlist;
		else {
			for(tmp = devlist; tmp->pNext != NULL; tmp = tmp->pNext);
			tmp->pNext = discovered_devlist;
		}
	}
	return devlist;
}

 
MINIUPNP_LIBSPEC struct UPNPDev *
upnpDiscover(int delay, const char * multicastif,
             const char * minissdpdsock, int localport,
             int ipv6, unsigned char ttl,
             int * error)
{
	static const char * const deviceList[] = {
#if 0
		"urn:schemas-upnp-org:device:InternetGatewayDevice:2",
		"urn:schemas-upnp-org:service:WANIPConnection:2",
#endif
		"urn:schemas-upnp-org:device:InternetGatewayDevice:1",
		"urn:schemas-upnp-org:service:WANIPConnection:1",
		"urn:schemas-upnp-org:service:WANPPPConnection:1",
		"upnp:rootdevice",
		 
		0
	};
	return upnpDiscoverDevices(deviceList,
	                           delay, multicastif, minissdpdsock, localport,
	                           ipv6, ttl, error, 0);
}

 
MINIUPNP_LIBSPEC struct UPNPDev *
upnpDiscoverAll(int delay, const char * multicastif,
                const char * minissdpdsock, int localport,
                int ipv6, unsigned char ttl,
                int * error)
{
	static const char * const deviceList[] = {
		 
		"ssdp:all",
		0
	};
	return upnpDiscoverDevices(deviceList,
	                           delay, multicastif, minissdpdsock, localport,
	                           ipv6, ttl, error, 0);
}

 
MINIUPNP_LIBSPEC struct UPNPDev *
upnpDiscoverDevice(const char * device, int delay, const char * multicastif,
                const char * minissdpdsock, int localport,
                int ipv6, unsigned char ttl,
                int * error)
{
	const char * const deviceList[] = {
		device,
		0
	};
	return upnpDiscoverDevices(deviceList,
	                           delay, multicastif, minissdpdsock, localport,
	                           ipv6, ttl, error, 0);
}

static char *
build_absolute_url(const char * baseurl, const char * descURL,
                   const char * url, unsigned int scope_id)
{
	size_t l, n;
	char * s;
	const char * base;
	char * p;
#if defined(IF_NAMESIZE) && !defined(_WIN32)
	char ifname[IF_NAMESIZE];
#else  
	char scope_str[8];
#endif	 

	if(  (url[0] == 'h')
	   &&(url[1] == 't')
	   &&(url[2] == 't')
	   &&(url[3] == 'p')
	   &&(url[4] == ':')
	   &&(url[5] == '/')
	   &&(url[6] == '/'))
		return strdup(url);
	base = (baseurl[0] == '\0') ? descURL : baseurl;
	n = strlen(base);
	if(n > 7) {
		p = strchr(base + 7, '/');
		if(p)
			n = p - base;
	}
	l = n + strlen(url) + 1;
	if(url[0] != '/')
		l++;
	if(scope_id != 0) {
#if defined(IF_NAMESIZE) && !defined(_WIN32)
		if(if_indextoname(scope_id, ifname)) {
			l += 3 + strlen(ifname);	 
		}
#else  
		 
		l += 3 + snprintf(scope_str, sizeof(scope_str), "%u", scope_id);
#endif  
	}
	s = malloc(l);
	if(s == NULL) return NULL;
	memcpy(s, base, n);
	if(scope_id != 0) {
		s[n] = '\0';
		if(n > 13 && 0 == memcmp(s, "http://[fe80:", 13)) {
			 
			p = strchr(s, ']');
			if(p) {
				 
#if defined(IF_NAMESIZE) && !defined(_WIN32)
				memmove(p + 3 + strlen(ifname), p, strlen(p) + 1);
				memcpy(p, "%25", 3);
				memcpy(p + 3, ifname, strlen(ifname));
				n += 3 + strlen(ifname);
#else  
				memmove(p + 3 + strlen(scope_str), p, strlen(p) + 1);
				memcpy(p, "%25", 3);
				memcpy(p + 3, scope_str, strlen(scope_str));
				n += 3 + strlen(scope_str);
#endif  
			}
		}
	}
	if(url[0] != '/')
		s[n++] = '/';
	memcpy(s + n, url, l - n);
	return s;
}

                                
   
MINIUPNP_LIBSPEC void
GetUPNPUrls(struct UPNPUrls * urls, struct IGDdatas * data,
            const char * descURL, unsigned int scope_id)
{
	 
	urls->rootdescURL = strdup(descURL);

	 
	urls->ipcondescURL = build_absolute_url(data->urlbase, descURL,
	                                        data->first.scpdurl, scope_id);
	urls->controlURL = build_absolute_url(data->urlbase, descURL,
	                                      data->first.controlurl, scope_id);
	urls->controlURL_CIF = build_absolute_url(data->urlbase, descURL,
	                                          data->CIF.controlurl, scope_id);
	urls->controlURL_6FC = build_absolute_url(data->urlbase, descURL,
	                                          data->IPv6FC.controlurl, scope_id);

#ifdef DEBUG
	printf("urls->ipcondescURL='%s'\n", urls->ipcondescURL);
	printf("urls->controlURL='%s'\n", urls->controlURL);
	printf("urls->controlURL_CIF='%s'\n", urls->controlURL_CIF);
	printf("urls->controlURL_6FC='%s'\n", urls->controlURL_6FC);
#endif
}

MINIUPNP_LIBSPEC void
FreeUPNPUrls(struct UPNPUrls * urls)
{
	if(!urls)
		return;
	free(urls->controlURL);
	urls->controlURL = 0;
	free(urls->ipcondescURL);
	urls->ipcondescURL = 0;
	free(urls->controlURL_CIF);
	urls->controlURL_CIF = 0;
	free(urls->controlURL_6FC);
	urls->controlURL_6FC = 0;
	free(urls->rootdescURL);
	urls->rootdescURL = 0;
}

int
UPNPIGD_IsConnected(struct UPNPUrls * urls, struct IGDdatas * data)
{
	char status[64];
	unsigned int uptime;
	status[0] = '\0';
	UPNP_GetStatusInfo(urls->controlURL, data->first.servicetype,
	                   status, &uptime, NULL);
	if(0 == strcmp("Connected", status))
		return 1;
	else if(0 == strcmp("Up", status))	 
		return 1;
	else
		return 0;
}


                       
                  
                         
                       
                                               
                                                        
                        
                                                                         
  
                                                                     
                                                                           
                         
   
MINIUPNP_LIBSPEC int
UPNP_GetValidIGD(struct UPNPDev * devlist,
                 struct UPNPUrls * urls,
				 struct IGDdatas * data,
				 char * lanaddr, int lanaddrlen)
{
	struct xml_desc {
		char lanaddr[40];
		char * xml;
		int size;
		int is_igd;
	} * desc = NULL;
	struct UPNPDev * dev;
	int ndev = 0;
	int i;
	int state = -1;  
	char extIpAddr[16];
	int status_code = -1;

	if(!devlist)
	{
#ifdef DEBUG
		printf("Empty devlist\n");
#endif
		return 0;
	}
	 
	for(dev = devlist; dev; dev = dev->pNext)
		ndev++;
	 
	desc = calloc(ndev, sizeof(struct xml_desc));
	if(!desc)
		return -1;  
	 
	for(dev = devlist, i = 0; dev; dev = dev->pNext, i++)
	{
		                                               
                                                                      
		desc[i].xml = miniwget_getaddr(dev->descURL, &(desc[i].size),
		                               desc[i].lanaddr, sizeof(desc[i].lanaddr),
		                               dev->scope_id, &status_code);
#ifdef DEBUG
		if(!desc[i].xml)
		{
			printf("error getting XML description %s\n", dev->descURL);
		}
#endif
		if(desc[i].xml)
		{
			memset(data, 0, sizeof(struct IGDdatas));
			memset(urls, 0, sizeof(struct UPNPUrls));
			parserootdesc(desc[i].xml, desc[i].size, data);
			if(COMPARE(data->CIF.servicetype,
			           "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:"))
			{
				desc[i].is_igd = 1;
			}
		}
	}
	 
	for(state = 1; state <= 3; state++)
	{
		for(dev = devlist, i = 0; dev; dev = dev->pNext, i++)
		{
			if(desc[i].xml)
			{
				memset(data, 0, sizeof(struct IGDdatas));
				memset(urls, 0, sizeof(struct UPNPUrls));
				parserootdesc(desc[i].xml, desc[i].size, data);
				if(desc[i].is_igd || state >= 3 )
				{
				  int is_connected;

				  GetUPNPUrls(urls, data, dev->descURL, dev->scope_id);

				   
				  if(state >= 2)
				    goto free_and_return;
				  is_connected = UPNPIGD_IsConnected(urls, data);
#ifdef DEBUG
				  printf("UPNPIGD_IsConnected(%s) = %d\n",
				     urls->controlURL, is_connected);
#endif
				   
				  if(is_connected &&
				     (UPNP_GetExternalIPAddress(urls->controlURL,  data->first.servicetype, extIpAddr) == 0)) {
					if(!addr_is_reserved(extIpAddr))
					  goto free_and_return;
				  }
				  FreeUPNPUrls(urls);
				  if(data->second.servicetype[0] != '\0') {
#ifdef DEBUG
				    printf("We tried %s, now we try %s !\n",
				           data->first.servicetype, data->second.servicetype);
#endif
				     
				    memcpy(&data->tmp, &data->first, sizeof(struct IGDdatas_service));
				    memcpy(&data->first, &data->second, sizeof(struct IGDdatas_service));
				    memcpy(&data->second, &data->tmp, sizeof(struct IGDdatas_service));
				    GetUPNPUrls(urls, data, dev->descURL, dev->scope_id);
				    is_connected = UPNPIGD_IsConnected(urls, data);
#ifdef DEBUG
				    printf("UPNPIGD_IsConnected(%s) = %d\n",
				       urls->controlURL, is_connected);
#endif
				    if(is_connected &&
				       (UPNP_GetExternalIPAddress(urls->controlURL,  data->first.servicetype, extIpAddr) == 0)) {
					  if(!addr_is_reserved(extIpAddr))
					    goto free_and_return;
				    }
				    FreeUPNPUrls(urls);
				  }
				}
				memset(data, 0, sizeof(struct IGDdatas));
			}
		}
	}
	state = 0;
free_and_return:
	if (lanaddr != NULL && state >= 1 && state <= 3 && i < ndev)
		strncpy(lanaddr, desc[i].lanaddr, lanaddrlen);
	for(i = 0; i < ndev; i++)
		free(desc[i].xml);
	free(desc);
	return state;
}

                       
                                            
                 
               
              
int
UPNP_GetIGDFromUrl(const char * rootdescurl,
                   struct UPNPUrls * urls,
                   struct IGDdatas * data,
                   char * lanaddr, int lanaddrlen)
{
	char * descXML;
	int descXMLsize = 0;

	descXML = miniwget_getaddr(rootdescurl, &descXMLsize,
	                           lanaddr, lanaddrlen, 0, NULL);
	if(descXML) {
		memset(data, 0, sizeof(struct IGDdatas));
		memset(urls, 0, sizeof(struct UPNPUrls));
		parserootdesc(descXML, descXMLsize, data);
		free(descXML);
		GetUPNPUrls(urls, data, rootdescurl, 0);
		return 1;
	} else {
		return 0;
	}
}
