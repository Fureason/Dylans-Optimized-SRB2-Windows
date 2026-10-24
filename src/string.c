// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2006      by Graue.
// Copyright (C) 2006-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------




#include <stddef.h>
#include <string.h>
#include "doomdef.h"

#ifndef SRB2_HAVE_STRLCPY



size_t strlcat(char *dst, const char *src, size_t siz)
{
	size_t dstlen, n = siz;
	char *p = dst;

	dstlen = strlen(dst);
	n -= dstlen;
	p += dstlen;

	while (n > 1 && *src != '\0')
	{
		*p++ = *src++;
		n--;
	}

	if (n >= 1)
		*p = '\0';

	return strlen(src) + dstlen;
}

size_t strlcpy(char *dst, const char *src, size_t siz)
{
	if (siz == 0)
		return strlen(dst);

	dst[0] = '\0';
	return strlcat(dst, src, siz);
}

#endif

#include "strcasestr.c"

int startswith(const char *path, const char *tag)
{
	return !strncmp(path, tag, strlen(tag));
}

int endswith(const char *base, const char *tag)
{
	const size_t base_length = strlen(base);
	const size_t tag_length = strlen(tag);

	if (tag_length > base_length)
		return false;

	return !memcmp(&base[base_length - tag_length], tag, tag_length);
}


char *xstrtok(char *line, const char *delims)
{
	static char *saveline = NULL;
	char *p;

	if(line != NULL)
		saveline = line;


	if(saveline == NULL || *saveline == '\0')
		return NULL;

	p = saveline;  

	saveline += strcspn(saveline, delims);  

	if(*saveline != '\0')  
		*saveline++ = '\0';

	return p;
}
