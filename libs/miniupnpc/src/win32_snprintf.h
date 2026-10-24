                                          
                   
                                                              
                      
                                                      
                                                          

#ifndef WIN32_SNPRINTF_H
#define WIN32_SNPRINTF_H

#ifdef _WIN32

#include <stdio.h>

                            
                               
                            
                               
                                      
                                           
   
#if ( \
	(defined(_MSC_VER) && _MSC_VER < 1900)   || \
	(defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR) && defined(__NO_ISOCEXT))   || \
	(defined(__MINGW64_VERSION_MAJOR) &&   !( \
		(defined (__USE_MINGW_ANSI_STDIO) && __USE_MINGW_ANSI_STDIO != 0))   || \
		(__MINGW64_VERSION_MAJOR >= 6 && defined(_UCRT))   || \
		(__MINGW64_VERSION_MAJOR >= 8 && !defined(__NO_ISOCEXT))   || \
	0) || \
0)

                             
                               
                        
                                     
   
#if ( \
	(defined(_MSC_VER) && _MSC_VER < 1300)   || \
	(defined(__MSVCRT_VERSION__) && __MSVCRT_VERSION__ < 0x700)   || \
0)
#define CHECK_SCPRINTF 0
#define IF_SCPRINTF(expr) 0
#define ELSE_SCPRINTF(expr) expr
#else
#define CHECK_SCPRINTF 1
#define IF_SCPRINTF(expr) expr
#define ELSE_SCPRINTF(expr) 0
#endif

 
#define snprintf(buf, size, fmt, ...) ( \
	(((size) != 0 && (buf) != NULL) ? (   \
		_snprintf((buf), (size), (fmt), __VA_ARGS__),   \
		(((char *)buf)[(size_t)(size)-1] = 0),   \
	0) : 0), \
	(CHECK_SCPRINTF ? IF_SCPRINTF( \
		_scprintf((fmt), __VA_ARGS__)   \
	) : ELSE_SCPRINTF( \
		((size) != 0 && (buf) != NULL) ? \
			strlen((buf))   \
		: \
			1   \
		) \
	) \
)

#endif

#endif  

#endif  
