/* Fail-proof memory allocation by Graue <graue@oceanbase.org>; public domain */

#include <stdlib.h>
#include <limits.h>
#include "err.h"

 
#ifndef SIZE_T_MAX
#define SIZE_T_MAX ULONG_MAX
#endif

  
                                                                      
                               
  
                                                                     
                                                                      
                                                                    
                               
   

 

  
                                                                      
         

                                                                     
                                                                 
                                                                     
          
   
static size_t sizmul(size_t size, size_t nmemb)
{
	if (SIZE_T_MAX / size <= nmemb)
	{
		errx(1, "allocating %lu objects of size %lu "
			"would overflow", (unsigned long)nmemb,
			(unsigned long)size);
	}
	return size * nmemb;  
}

static void nomem(size_t size, size_t nmemb)
{
	err(1, "cannot allocate %lu objects of size %lu "
		"(total %lu bytes)", (unsigned long)nmemb,
		(unsigned long)size, (unsigned long)(nmemb*size));
}

void *xm(size_t size, size_t nmemb)
{
	const size_t bytes = sizmul(size, nmemb);
	void *p;

	p = malloc(bytes);
	if (p == NULL) nomem(size, nmemb);
	return p;
}

void *xr(void *p, size_t size, size_t nmemb)
{
	const size_t bytes = sizmul(size, nmemb);

	p = realloc(p, bytes);
	if (p == NULL) nomem(size, nmemb);
	return p;
}

 
void *xpnd(void *p, int nit, int *sit, size_t sz)
{
	if (nit < *sit) return p;
	else if (*sit > 0) return xr(p, sz, (*sit *= 2));
	else return xm(sz, (*sit = 10));
}
