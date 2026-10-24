// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------






#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif

 
#ifdef _MSC_VER


#ifdef HAVE_OPENMPT
#include <stdint.h>
#endif

#define SINT8 signed __int8

typedef long ssize_t;

 
#if (_MSC_VER <= 1200)
	#ifndef DWORD_PTR
		#define DWORD_PTR DWORD
	#endif
	#ifndef PDWORD_PTR
		#define PDWORD_PTR PDWORD
	#endif
#endif
#else
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#define UINT8 uint8_t
#define SINT8 int8_t

#define UINT16 uint16_t
#define INT16 int16_t

#define INT32 int32_t
#define UINT32 uint32_t
#define INT64  int64_t
#define UINT64 uint64_t
#endif

#ifdef __APPLE_CC__
#define DIRECTFULLSCREEN 1
#define DEBUG_LOG
#define NOIPX
#endif

 

#ifdef _MSC_VER

#if (_MSC_VER <= 1800)  
	#define snprintf                _snprintf
#if (_MSC_VER <= 1200)  
	#define vsnprintf               _vsnprintf
#endif
#endif
	#define strncasecmp             strnicmp
	#define strcasecmp              stricmp
#ifndef __cplusplus
	#define inline                  __inline
#endif
#elif defined (__WATCOMC__)
	#include <dos.h>
	#include <sys\types.h>
	#include <direct.h>
	#include <malloc.h>
	#define strncasecmp             strnicmp
	#define strcasecmp              strcmpi
#endif
#if defined (__unix__) || defined (__APPLE__) || defined (UNIXCOMMON)
	#undef stricmp
	#define stricmp(x,y) strcasecmp(x,y)
	#undef strnicmp
	#define strnicmp(x,y,n) strncasecmp(x,y,n)
#endif

char *nongnu_strcasestr(const char *in, const char *what);
#ifndef _GNU_SOURCE
#define strcasestr nongnu_strcasestr
#endif
#define stristr strcasestr

int startswith (const char *base, const char *tag);
int endswith (const char *base, const char *tag);
char *xstrtok(char *line, const char *delims);

#if defined (_WIN32) || defined (__HAIKU__)
#define HAVE_DOSSTR_FUNCS
#endif

#if defined (__APPLE__)
	#define SRB2_HAVE_STRLCPY
#elif defined (__GLIBC_PREREQ)

	#if __GLIBC_PREREQ(2, 38)
		#define SRB2_HAVE_STRLCPY
	#endif
#endif

#ifndef HAVE_DOSSTR_FUNCS
int strupr(char *n);  
int strlwr(char *n);  
#endif

#include <stddef.h>  

#ifndef SRB2_HAVE_STRLCPY
size_t strlcat(char *dst, const char *src, size_t siz);
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif







#define STRBUFCPY(dst,src) strlcpy(dst, src, sizeof dst)

 





#ifndef _WIN32
typedef int32_t boolean;
#else
#define boolean BOOL
#endif

#ifndef __cplusplus
#ifndef _WIN32
enum {false = 0, true = 1};
#else
#define false FALSE
#define true TRUE
#endif
#endif

 

#ifndef INT8_MIN
#define INT8_MIN (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN (-32768)
#endif
#ifndef INT32_MIN
#define INT32_MIN (-2147483647 - 1)
#endif
#ifndef INT64_MIN
#define INT64_MIN  (-9223372036854775807LL - 1)
#endif

#ifndef INT8_MAX
#define INT8_MAX 127
#endif
#ifndef INT16_MAX
#define INT16_MAX 32767
#endif
#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif
#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif

#ifndef UINT8_MAX
#define UINT8_MAX 0xff  
#endif
#ifndef UINT16_MAX
#define UINT16_MAX 0xffff  
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff   
#endif
#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffULL  
#endif

 

#ifdef __GNUC__  
	#define FUNCNORETURN __attribute__ ((noreturn))

	#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)) && defined (__MINGW32__)  
		#include "inttypes.h"
		#if 0  
			#define FUNCPRINTF __attribute__ ((format(gnu_printf, 1, 2)))
			#define FUNCDEBUG  __attribute__ ((format(gnu_printf, 2, 3)))
			#define FUNCIERROR __attribute__ ((format(gnu_printf, 1, 2),noreturn))
		#elif (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)  
			#define FUNCPRINTF __attribute__ ((format(ms_printf, 1, 2)))
			#define FUNCDEBUG  __attribute__ ((format(ms_printf, 2, 3)))
			#define FUNCIERROR __attribute__ ((format(ms_printf, 1, 2),noreturn))
		#else
			#define FUNCPRINTF __attribute__ ((format(printf, 1, 2)))
			#define FUNCDEBUG  __attribute__ ((format(printf, 2, 3)))
			#define FUNCIERROR __attribute__ ((format(printf, 1, 2),noreturn))
		#endif
	#else
		#define FUNCPRINTF __attribute__ ((format(printf, 1, 2)))
		#define FUNCDEBUG  __attribute__ ((format(printf, 2, 3)))
		#define FUNCIERROR __attribute__ ((format(printf, 1, 2),noreturn))
	#endif

	#ifndef FUNCIERROR
		#define FUNCIERROR __attribute__ ((noreturn))
	#endif

	#define FUNCMATH __attribute__((const))

	#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)  
		#define FUNCDEAD __attribute__ ((deprecated))
		#define FUNCINLINE __attribute__((always_inline))
		#define FUNCNONNULL __attribute__((nonnull))
	#endif

	#define FUNCNOINLINE __attribute__((noinline))

	#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)  
		#ifdef __i386__  
			#define FUNCTARGET(X)  __attribute__ ((__target__ (X)))
		#endif
	#endif

	#if defined (__MINGW32__) && ((__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))  
		#define ATTRPACK __attribute__((packed, gcc_struct))
	#else
		#define ATTRPACK __attribute__((packed))
	#endif

	#define ATTRUNUSED __attribute__((unused))
#elif defined (_MSC_VER)
	#define ATTRNORETURN __declspec(noreturn)
	#define ATTRINLINE __forceinline
	#if _MSC_VER > 1200  
		#define ATTRNOINLINE __declspec(noinline)
	#endif
#endif

#ifndef FUNCPRINTF
#define FUNCPRINTF
#endif
#ifndef FUNCDEBUG
#define FUNCDEBUG
#endif
#ifndef FUNCNORETURN
#define FUNCNORETURN
#endif
#ifndef FUNCIERROR
#define FUNCIERROR
#endif
#ifndef FUNCMATH
#define FUNCMATH
#endif
#ifndef FUNCDEAD
#define FUNCDEAD
#endif
#ifndef FUNCINLINE
#define FUNCINLINE
#endif
#ifndef FUNCNONNULL
#define FUNCNONNULL
#endif
#ifndef FUNCNOINLINE
#define FUNCNOINLINE
#endif
#ifndef FUNCTARGET
#define FUNCTARGET(x)
#endif
#ifndef ATTRPACK
#define ATTRPACK
#endif
#ifndef ATTRUNUSED
#define ATTRUNUSED
#endif
#ifndef ATTRNORETURN
#define ATTRNORETURN
#endif
#ifndef ATTRINLINE
#define ATTRINLINE inline
#endif
#ifndef ATTRNOINLINE
#define ATTRNOINLINE
#endif

 

typedef struct
{
	UINT8 red;
	UINT8 green;
	UINT8 blue;
	UINT8 alpha;
} byteColor_t;

union FColorRGBA
{
	UINT32 rgba;
	byteColor_t s;
} ATTRPACK;
typedef union FColorRGBA RGBA_t;

typedef enum
{
	postimg_none,
	postimg_water,
	postimg_motion,
	postimg_flip,
	postimg_heat
} postimg_t;

typedef UINT32 lumpnum_t;  
#define LUMPERROR UINT32_MAX

typedef UINT32 tic_t;
#define INFTICS UINT32_MAX

#include "endian.h"  

#ifdef SRB2_BIG_ENDIAN
#define UINT2RGBA(a) a
#else
#define UINT2RGBA(a) (UINT32)((a&0xff)<<24)|((a&0xff00)<<8)|((a&0xff0000)>>8)|(((UINT32)a&0xff000000)>>24)
#endif

#define TOSTR(x) #x

 
#define WSTRING2(s) L ## s
#define WSTRING(s) WSTRING2 (s)

  
                                                        
                                                       
                                
  
#define FIELDFROM( type, field, have, want ) \
	(void *)((intptr_t)(field) - offsetof (type, have) + offsetof (type, want))

typedef UINT8 bitarray_t;

#define BIT_ARRAY_SIZE(n) (((n) + 7) >> 3)

static inline int
in_bit_array (const bitarray_t * const array, const int value)
{
	return (array[value >> 3] & (1<<(value & 7)));
}

static inline void
set_bit_array (bitarray_t * const array, const int value)
{
	array[value >> 3] |= (1<<(value & 7));
}

static inline void
unset_bit_array (bitarray_t * const array, const int value)
{
	array[value >> 3] &= ~(1<<(value & 7));
}

typedef UINT64 precise_t;

#ifdef __cplusplus
}  
#endif

#endif  
