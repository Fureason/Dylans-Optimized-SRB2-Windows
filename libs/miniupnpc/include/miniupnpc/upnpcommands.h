 
/* Miniupnp project : http://miniupnp.free.fr/
 * Author : Thomas Bernard
 * Copyright (c) 2005-2018 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided within this distribution */
#ifndef UPNPCOMMANDS_H_INCLUDED
#define UPNPCOMMANDS_H_INCLUDED

#include "miniupnpc_declspec.h"
#include "miniupnpctypes.h"

 
#define UPNPCOMMAND_SUCCESS (0)
#define UPNPCOMMAND_UNKNOWN_ERROR (-1)
#define UPNPCOMMAND_INVALID_ARGS (-2)
#define UPNPCOMMAND_HTTP_ERROR (-3)
#define UPNPCOMMAND_INVALID_RESPONSE (-4)
#define UPNPCOMMAND_MEM_ALLOC_ERROR (-5)

#ifdef __cplusplus
extern "C" {
#endif

struct PortMappingParserData;

MINIUPNP_LIBSPEC UNSIGNED_INTEGER
UPNP_GetTotalBytesSent(const char * controlURL,
					const char * servicetype);

MINIUPNP_LIBSPEC UNSIGNED_INTEGER
UPNP_GetTotalBytesReceived(const char * controlURL,
						const char * servicetype);

MINIUPNP_LIBSPEC UNSIGNED_INTEGER
UPNP_GetTotalPacketsSent(const char * controlURL,
					const char * servicetype);

MINIUPNP_LIBSPEC UNSIGNED_INTEGER
UPNP_GetTotalPacketsReceived(const char * controlURL,
					const char * servicetype);

                       
                                               
                  
                                                                           
                          
MINIUPNP_LIBSPEC int
UPNP_GetStatusInfo(const char * controlURL,
			       const char * servicetype,
				   char * status,
				   unsigned int * uptime,
                   char * lastconnerror);

                               
                                                   
                  
                                                                           
                          
MINIUPNP_LIBSPEC int
UPNP_GetConnectionTypeInfo(const char * controlURL,
                           const char * servicetype,
						   char * connectionType);

                                                                  
                                                          
                                      
  
                  
              
                                                                  
  
                         
                                                                      
                                                                          
MINIUPNP_LIBSPEC int
UPNP_GetExternalIPAddress(const char * controlURL,
                          const char * servicetype,
                          char * extIpAdd);

                                 
                                                          
  
                  
                                                                           
                           
MINIUPNP_LIBSPEC int
UPNP_GetLinkLayerMaxBitRates(const char* controlURL,
							const char* servicetype,
							unsigned int * bitrateDown,
							unsigned int * bitrateUp);

                        
                                                          
                                                           
  
                  
              
                                                                   
  
                                                    
                                                          
                                                                      
                                                                       
                                                                              
                                                             
                                                                    
                                                
                                                                              
                                                                          
                                                                           
                                                                 
                                                
                                                                          
                                                          
                                                                     
                                                                              
                                                                             
                                                                         
                                                                         
                                                   
                                                                          
                                                                           
                                                                              
   
MINIUPNP_LIBSPEC int
UPNP_AddPortMapping(const char * controlURL, const char * servicetype,
		    const char * extPort,
		    const char * inPort,
		    const char * inClient,
		    const char * desc,
		    const char * proto,
		    const char * remoteHost,
		    const char * leaseDuration);

                           
                                                          
                                                           
  
                  
              
                                                                   
  
                                                    
                                                          
                                                                      
                                                                       
                                                                              
                                                             
                                                                    
                                                
                                                                              
                                                                         
                                                   
                                                                          
                                                                           
                                                                              
   
MINIUPNP_LIBSPEC int
UPNP_AddAnyPortMapping(const char * controlURL, const char * servicetype,
		       const char * extPort,
		       const char * inPort,
		       const char * inClient,
		       const char * desc,
		       const char * proto,
		       const char * remoteHost,
		       const char * leaseDuration,
		       char * reservedPort);

                           
                                                                  
                                                           
                  
              
                                                                     
  
                                                       
                                                                      
                                                                          
                                                                 
                                                                              
MINIUPNP_LIBSPEC int
UPNP_DeletePortMapping(const char * controlURL, const char * servicetype,
		       const char * extPort, const char * proto,
		       const char * remoteHost);

                                
                                                                  
                                                           
                  
              
                                                                     
  
                                                       
                                                                          
                                                                 
                                                                      
                                                  
                                                                                         
MINIUPNP_LIBSPEC int
UPNP_DeletePortMappingRange(const char * controlURL, const char * servicetype,
        		    const char * extPortStart, const char * extPortEnd,
        		    const char * proto,
        		    const char * manage);

                                       
                                  
MINIUPNP_LIBSPEC int
UPNP_GetPortMappingNumberOfEntries(const char * controlURL,
                                   const char * servicetype,
                                   unsigned int * numEntries);

                                     
                                        
           
                
              
                   
                             
                          
                        
                          
                                 
  
                 
                                                                           
                        
  
                                                                  
                                                                      
                                                                       
                                                                          
                                                                 
                                                                            
   
MINIUPNP_LIBSPEC int
UPNP_GetSpecificPortMappingEntry(const char * controlURL,
                                 const char * servicetype,
                                 const char * extPort,
                                 const char * proto,
                                 const char * remoteHost,
                                 char * intClient,
                                 char * intPort,
                                 char * desc,
                                 char * enabled,
                                 char * leaseDuration);

                                    
           
              
                          
                             
                          
                           
                        
                          
                         
                            
  
                 
                                                                           
                        
  
                              
                                                                      
                                                                          
                                                                 
                                                                              
   
MINIUPNP_LIBSPEC int
UPNP_GetGenericPortMappingEntry(const char * controlURL,
                                const char * servicetype,
								const char * index,
								char * extPort,
								char * intClient,
								char * intPort,
								char * protocol,
								char * desc,
								char * enabled,
								char * rHost,
								char * duration);

                                                        
  
  
                              
                            
                                                                             
                                                                          
                                           
   
MINIUPNP_LIBSPEC int
UPNP_GetListOfPortMappings(const char * controlURL,
                           const char * servicetype,
                           const char * startPort,
                           const char * endPort,
                           const char * protocol,
                           const char * numberOfPorts,
                           struct PortMappingParserData * data);

 
MINIUPNP_LIBSPEC int
UPNP_GetFirewallStatus(const char * controlURL,
				const char * servicetype,
				int * firewallEnabled,
				int * inboundPinholeAllowed);

MINIUPNP_LIBSPEC int
UPNP_GetOutboundPinholeTimeout(const char * controlURL, const char * servicetype,
                    const char * remoteHost,
                    const char * remotePort,
                    const char * intClient,
                    const char * intPort,
                    const char * proto,
                    int * opTimeout);

MINIUPNP_LIBSPEC int
UPNP_AddPinhole(const char * controlURL, const char * servicetype,
                    const char * remoteHost,
                    const char * remotePort,
                    const char * intClient,
                    const char * intPort,
                    const char * proto,
                    const char * leaseTime,
                    char * uniqueID);

MINIUPNP_LIBSPEC int
UPNP_UpdatePinhole(const char * controlURL, const char * servicetype,
                    const char * uniqueID,
                    const char * leaseTime);

MINIUPNP_LIBSPEC int
UPNP_DeletePinhole(const char * controlURL, const char * servicetype, const char * uniqueID);

MINIUPNP_LIBSPEC int
UPNP_CheckPinholeWorking(const char * controlURL, const char * servicetype,
                                 const char * uniqueID, int * isWorking);

MINIUPNP_LIBSPEC int
UPNP_GetPinholePackets(const char * controlURL, const char * servicetype,
                                 const char * uniqueID, int * packets);

#ifdef __cplusplus
}
#endif

#endif

