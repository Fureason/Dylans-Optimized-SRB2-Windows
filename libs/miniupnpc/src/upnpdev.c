 
/* Project : miniupnp
 * Web : http://miniupnp.free.fr/
 * Author : Thomas BERNARD
 * copyright (c) 2005-2015 Thomas Bernard
 * This software is subjet to the conditions detailed in the
 * provided LICENSE file. */
#include <stdlib.h>
#include "upnpdev.h"

                                      
                                                      
void freeUPNPDevlist(struct UPNPDev * devlist)
{
	struct UPNPDev * next;
	while(devlist)
	{
		next = devlist->pNext;
		free(devlist);
		devlist = next;
	}
}

