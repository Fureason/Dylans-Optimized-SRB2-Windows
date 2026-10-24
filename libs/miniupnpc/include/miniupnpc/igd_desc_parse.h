 
/* Project : miniupnp
 * http://miniupnp.free.fr/
 * Author : Thomas Bernard
 * Copyright (c) 2005-2014 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution.
 * */
#ifndef IGD_DESC_PARSE_H_INCLUDED
#define IGD_DESC_PARSE_H_INCLUDED

                                                       
                                              
#define MINIUPNPC_URL_MAXSIZE (128)
struct IGDdatas_service {
	char controlurl[MINIUPNPC_URL_MAXSIZE];
	char eventsuburl[MINIUPNPC_URL_MAXSIZE];
	char scpdurl[MINIUPNPC_URL_MAXSIZE];
	char servicetype[MINIUPNPC_URL_MAXSIZE];
	 
};

struct IGDdatas {
	char cureltname[MINIUPNPC_URL_MAXSIZE];
	char urlbase[MINIUPNPC_URL_MAXSIZE];
	char presentationurl[MINIUPNPC_URL_MAXSIZE];
	int level;
	 
	 
	struct IGDdatas_service CIF;
	                                                   
                                                        
	struct IGDdatas_service first;
	 
	struct IGDdatas_service second;
	 
	struct IGDdatas_service IPv6FC;
	 
	struct IGDdatas_service tmp;
};

void IGDstartelt(void *, const char *, int);
void IGDendelt(void *, const char *, int);
void IGDdata(void *, const char *, int);
#ifdef DEBUG
void printIGD(struct IGDdatas *);
#endif  

#endif  
