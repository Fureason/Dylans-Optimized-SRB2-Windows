 
                   
                                                             
                               
                                                      
                                                          

#ifndef UPNPREPLYPARSE_H_INCLUDED
#define UPNPREPLYPARSE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct NameValue {
	struct NameValue * l_next;
	char name[64];
	char value[128];
};

struct NameValueParserData {
	struct NameValue * l_head;
	char curelt[64];
	char * portListing;
	int portListingLength;
	int topelt;
	const char * cdata;
	int cdatalen;
};

 
void
ParseNameValue(const char * buffer, int bufsize,
               struct NameValueParserData * data);

 
void
ClearNameValueList(struct NameValueParserData * pdata);

 
char *
GetValueFromNameValueList(struct NameValueParserData * pdata,
                          const char * Name);

#if 0
 
char *
GetValueFromNameValueListIgnoreNS(struct NameValueParserData * pdata,
                                  const char * Name);
#endif

 
#ifdef DEBUG
void
DisplayNameValueList(char * buffer, int bufsize);
#endif

#ifdef __cplusplus
}
#endif

#endif

