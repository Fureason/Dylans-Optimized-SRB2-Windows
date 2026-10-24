 
                   
                                                             
                               
                                                      
                                                          
#include <string.h>
#include <stdlib.h>
#ifdef DEBUG
#include <stdio.h>
#endif  
#include "portlistingparse.h"
#include "minixml.h"

#if defined(__HAIKU__)
 
#define atoui atoui2
#endif 

 
static const struct {
	const portMappingElt code;
	const char * const str;
} elements[] = {
	{ PortMappingEntry, "PortMappingEntry"},
	{ NewRemoteHost, "NewRemoteHost"},
	{ NewExternalPort, "NewExternalPort"},
	{ NewProtocol, "NewProtocol"},
	{ NewInternalPort, "NewInternalPort"},
	{ NewInternalClient, "NewInternalClient"},
	{ NewEnabled, "NewEnabled"},
	{ NewDescription, "NewDescription"},
	{ NewLeaseTime, "NewLeaseTime"},
	{ PortMappingEltNone, NULL}
};

 
static UNSIGNED_INTEGER
atoui(const char * p, int l)
{
	UNSIGNED_INTEGER r = 0;
	while(l > 0 && *p)
	{
		if(*p >= '0' && *p <= '9')
			r = r*10 + (*p - '0');
		else
			break;
		p++;
		l--;
	}
	return r;
}

 
static void
startelt(void * d, const char * name, int l)
{
	int i;
	struct PortMappingParserData * pdata = (struct PortMappingParserData *)d;
	pdata->curelt = PortMappingEltNone;
	for(i = 0; elements[i].str; i++)
	{
		if(strlen(elements[i].str) == (size_t)l && memcmp(name, elements[i].str, l) == 0)
		{
			pdata->curelt = elements[i].code;
			break;
		}
	}
	if(pdata->curelt == PortMappingEntry)
	{
		struct PortMapping * pm;
		pm = calloc(1, sizeof(struct PortMapping));
		if(pm == NULL)
		{
			 
#ifdef DEBUG
			fprintf(stderr, "%s: error allocating memory",
			        "startelt");
#endif  
			return;
		}
		pm->l_next = pdata->l_head;	 
		pdata->l_head = pm;
	}
}

 
static void
endelt(void * d, const char * name, int l)
{
	struct PortMappingParserData * pdata = (struct PortMappingParserData *)d;
	(void)name;
	(void)l;
	pdata->curelt = PortMappingEltNone;
}

 
static void
data(void * d, const char * data, int l)
{
	struct PortMapping * pm;
	struct PortMappingParserData * pdata = (struct PortMappingParserData *)d;
	pm = pdata->l_head;
	if(!pm)
		return;
	if(l > 63)
		l = 63;
	switch(pdata->curelt)
	{
	case NewRemoteHost:
		memcpy(pm->remoteHost, data, l);
		pm->remoteHost[l] = '\0';
		break;
	case NewExternalPort:
		pm->externalPort = (unsigned short)atoui(data, l);
		break;
	case NewProtocol:
		if(l > 3)
			l = 3;
		memcpy(pm->protocol, data, l);
		pm->protocol[l] = '\0';
		break;
	case NewInternalPort:
		pm->internalPort = (unsigned short)atoui(data, l);
		break;
	case NewInternalClient:
		memcpy(pm->internalClient, data, l);
		pm->internalClient[l] = '\0';
		break;
	case NewEnabled:
		pm->enabled = (unsigned char)atoui(data, l);
		break;
	case NewDescription:
		memcpy(pm->description, data, l);
		pm->description[l] = '\0';
		break;
	case NewLeaseTime:
		pm->leaseTime = atoui(data, l);
		break;
	default:
		break;
	}
}


                                                           
   
void
ParsePortListing(const char * buffer, int bufsize,
                 struct PortMappingParserData * pdata)
{
	struct xmlparser parser;

	memset(pdata, 0, sizeof(struct PortMappingParserData));
	 
	parser.xmlstart = buffer;
	parser.xmlsize = bufsize;
	parser.data = pdata;
	parser.starteltfunc = startelt;
	parser.endeltfunc = endelt;
	parser.datafunc = data;
	parser.attfunc = 0;
	parsexml(&parser);
}

void
FreePortListing(struct PortMappingParserData * pdata)
{
	struct PortMapping * pm;
	while((pm = pdata->l_head) != NULL)
	{
		 
		pdata->l_head = pm->l_next;
		free(pm);
	}
}

