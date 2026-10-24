#ifndef __FASTCMP_H__
#define __FASTCMP_H__



FUNCINLINE static ATTRINLINE boolean fasticmp(const char *s, const char *c)
{
	for (; *s && toupper(*s) == toupper(*c); s++, c++) ;
	return (*s == *c);  
}


FUNCINLINE static ATTRINLINE boolean fastcmp(const char *s, const char *c)
{
	for (; *s && *s == *c; s++, c++) ;
	return (*s == *c);  
}



FUNCINLINE static ATTRINLINE boolean fastncmp(const char *s, const char *c, UINT16 l)
{
	for (; *s && *s == *c && --l; s++, c++) ;
	return !l;  
}

#endif
