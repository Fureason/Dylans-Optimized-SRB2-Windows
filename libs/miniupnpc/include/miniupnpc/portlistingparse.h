 
                   
                                                             
                               
                                                      
                                                          
#ifndef PORTLISTINGPARSE_H_INCLUDED
#define PORTLISTINGPARSE_H_INCLUDED

#include "miniupnpc_declspec.h"
 
#include "miniupnpctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

                               
                      
                                                  
                                               
                                      
                                               
                                                            
                                  
                                              
                                        
                       
   
typedef enum { PortMappingEltNone,
       PortMappingEntry, NewRemoteHost,
       NewExternalPort, NewProtocol,
       NewInternalPort, NewInternalClient,
       NewEnabled, NewDescription,
       NewLeaseTime } portMappingElt;

struct PortMapping {
	struct PortMapping * l_next;	 
	UNSIGNED_INTEGER leaseTime;
	unsigned short externalPort;
	unsigned short internalPort;
	char remoteHost[64];
	char internalClient[64];
	char description[64];
	char protocol[4];
	unsigned char enabled;
};

struct PortMappingParserData {
	struct PortMapping * l_head;	 
	portMappingElt curelt;
};

MINIUPNP_LIBSPEC void
ParsePortListing(const char * buffer, int bufsize,
                 struct PortMappingParserData * pdata);

MINIUPNP_LIBSPEC void
FreePortListing(struct PortMappingParserData * pdata);

#ifdef __cplusplus
}
#endif

#endif
