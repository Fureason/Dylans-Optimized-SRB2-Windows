 
                   
                                                             
                   
                                            
  
                                  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "minixml.h"

 
struct event {
	enum { ELTSTART, ELTEND, ATT, CHARDATA } type;
	const char * data;
	int len;
};

struct eventlist {
	int n;
	struct event * events;
};

                            
                                          
int evtlistcmp(struct eventlist * a, struct eventlist * b)
{
	int i;
	struct event * ae, * be;
	if(a->n != b->n)
	{
		printf("event number not matching : %d != %d\n", a->n, b->n);
		 
	}
	for(i=0; i<a->n; i++)
	{
		ae = a->events + i;
		be = b->events + i;
		if(  (ae->type != be->type)
		   ||(ae->len != be->len)
		   ||memcmp(ae->data, be->data, ae->len))
		{
			printf("Found a difference : %d '%.*s' != %d '%.*s'\n",
			       ae->type, ae->len, ae->data,
			       be->type, be->len, be->data);
			return 1;
		}
	}
	return 0;
}

 
static const char xmldata[] =
"<xmlroot>\n"
" <elt1 att1=\"attvalue1\" att2=\"attvalue2\">"
"character data"
"</elt1> \n \t"
"<elt1b/>"
"<elt1>\n<![CDATA[ <html>stuff !\n ]]> \n</elt1>\n"
"<elt2a> \t<elt2b>chardata1</elt2b><elt2b> chardata2 </elt2b></elt2a>"
"</xmlroot>";

static const struct event evtref[] =
{
	{ELTSTART, "xmlroot", 7},
	{ELTSTART, "elt1", 4},
	 
	{CHARDATA, "character data", 14},
	{ELTEND, "elt1", 4},
	{ELTSTART, "elt1b", 5},
	{ELTSTART, "elt1", 4},
	{CHARDATA, " <html>stuff !\n ", 16},
	{ELTEND, "elt1", 4},
	{ELTSTART, "elt2a", 5},
	{ELTSTART, "elt2b", 5},
	{CHARDATA, "chardata1", 9},
	{ELTEND, "elt2b", 5},
	{ELTSTART, "elt2b", 5},
	{CHARDATA, " chardata2 ", 11},
	{ELTEND, "elt2b", 5},
	{ELTEND, "elt2a", 5},
	{ELTEND, "xmlroot", 7}
};

void startelt(void * data, const char * p, int l)
{
	struct eventlist * evtlist = data;
	struct event * evt;
	evt = evtlist->events + evtlist->n;
	 
	evt->type = ELTSTART;
	evt->data = p;
	evt->len = l;
	evtlist->n++;
}

void endelt(void * data, const char * p, int l)
{
	struct eventlist * evtlist = data;
	struct event * evt;
	evt = evtlist->events + evtlist->n;
	 
	evt->type = ELTEND;
	evt->data = p;
	evt->len = l;
	evtlist->n++;
}

void chardata(void * data, const char * p, int l)
{
	struct eventlist * evtlist = data;
	struct event * evt;
	evt = evtlist->events + evtlist->n;
	 
	evt->type = CHARDATA;
	evt->data = p;
	evt->len = l;
	evtlist->n++;
}

int testxmlparser(const char * xml, int size)
{
	int r;
	struct eventlist evtlist;
	struct eventlist evtlistref;
	struct xmlparser parser;
	evtlist.n = 0;
	evtlist.events = malloc(sizeof(struct event)*100);
	if(evtlist.events == NULL)
	{
		fprintf(stderr, "Memory allocation error.\n");
		return -1;
	}
	memset(&parser, 0, sizeof(parser));
	parser.xmlstart = xml;
	parser.xmlsize = size;
	parser.data = &evtlist;
	parser.starteltfunc = startelt;
	parser.endeltfunc = endelt;
	parser.datafunc = chardata;
	parsexml(&parser);
	printf("%d events\n", evtlist.n);
	 
	evtlistref.n = sizeof(evtref)/sizeof(struct event);
	evtlistref.events = (struct event *)evtref;
	r = evtlistcmp(&evtlistref, &evtlist);
	free(evtlist.events);
	return r;
}

int main(int argc, char * * argv)
{
	int r;
	(void)argc; (void)argv;

	r = testxmlparser(xmldata, sizeof(xmldata)-1);
	if(r)
		printf("minixml validation test failed\n");
	return r;
}

