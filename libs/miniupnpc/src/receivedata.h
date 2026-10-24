 
/* Project: miniupnp
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * Author: Thomas Bernard
 * Copyright (c) 2011-2018 Thomas Bernard
 * This software is subjects to the conditions detailed
 * in the LICENCE file provided within this distribution */
#ifndef RECEIVEDATA_H_INCLUDED
#define RECEIVEDATA_H_INCLUDED

#include "miniupnpc_socketdef.h"

                                        
                                                                        
                                                                      
int receivedata(SOCKET socket,
                char * data, int length,
                int timeout, unsigned int * scope_id);

#endif

