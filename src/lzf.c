// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2000-2005 by Marc Alexander Lehmann <schmorp@schmorp.de>
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



 
 

  
                                 
   

#ifndef LZFP_h
#define LZFP_h

#include "lzf.h"
#include "doomdef.h"

  
                                                     
                                                      
                                                 
                                                     
                                                         
                                                
   
#ifndef HLOG
# define HLOG 15
#endif

  
                                                                            
                                                                     
                                                                       
   

#ifndef VERY_FAST
# define VERY_FAST 1
#endif

  
                                                                          
                                                       
                                                                           
                                                                 
                                       
   
#ifndef ULTRA_FAST
# define ULTRA_FAST 0
#endif

  
                                                                       
   
#ifndef STRICT_ALIGN
#if !(defined(__i386) || defined (__amd64)) || defined (__clang__)
#define STRICT_ALIGN 1
#else
#define STRICT_ALIGN 0
#endif
#endif

  
                                       
                                                             
   
#ifndef USE_MEMCPY
#ifdef _MSC_VER
# define USE_MEMCPY 0
#else
# define USE_MEMCPY 1
#endif
#endif

  
                                                                    
                                        
   
#ifndef INIT_HTAB
# define INIT_HTAB 1
#endif

  
                                                                        
                                                                    
                              
   
#ifndef AVOID_ERRNO
# define AVOID_ERRNO 0
#endif

  
                                                                    
                                                                
                                            
   
#ifndef LZF_STATE_ARG
# define LZF_STATE_ARG 0
#endif

  
                                                                  
                                                                 
                                                                 
                                 
                                                          
                                                               
   
#ifndef CHECK_INPUT
# define CHECK_INPUT 1
#endif

 
 

typedef unsigned char u8;

typedef const u8 *LZF_STATE[1 << (HLOG)];

#if !STRICT_ALIGN
 
# include <limits.h>
# if USHRT_MAX == 65535
    typedef unsigned short u16;
# elif UINT_MAX == 65535
    typedef unsigned int u16;
# else
#  undef STRICT_ALIGN
#  define STRICT_ALIGN 1
# endif
#endif

#if ULTRA_FAST
# if defined(VERY_FAST)
#  undef VERY_FAST
# endif
#endif

#if USE_MEMCPY || INIT_HTAB
# ifdef __cplusplus
#  include <cstring>
# else
#  include <string.h>
# endif
#endif

#endif


  
                                     
   

#if AVOID_ERRNO
# define SET_ERRNO(n)
#else
# include <errno.h>
# define SET_ERRNO(n) errno = (n)
#endif

size_t
lzf_decompress (const void *const in_data,  size_t in_len,
                void             *out_data, size_t out_len)
{
	u8 const *ip = (const u8 *)in_data;
	u8       *op = (u8 *)out_data;
	u8 const *const in_end  = ip + in_len;
	u8       *const out_end = op + out_len;

	do
	{
		unsigned int ctrl = *ip++;

		if (ctrl < (1 << 5))  
		{
			ctrl++;

			if (op + ctrl > out_end)
			{
				SET_ERRNO (E2BIG);
				return 0;
			}

#if CHECK_INPUT
			if (ip + ctrl > in_end)
			{
				SET_ERRNO (EINVAL);
				return 0;
			}
#endif

#if USE_MEMCPY
			M_Memcpy (op, ip, ctrl);
			op += ctrl;
			ip += ctrl;
#else
			do
				*op++ = *ip++;
			while (--ctrl);
#endif
		}
		else  
		{
			unsigned int len = ctrl >> 5;

			u8 *ref = op - ((ctrl & 0x1f) << 8) - 1;

#if CHECK_INPUT
			if (ip >= in_end)
			{
				SET_ERRNO (EINVAL);
				return 0;
			}
#endif
			if (len == 7)
			{
				len += *ip++;
#if CHECK_INPUT
				if (ip >= in_end)
				{
					SET_ERRNO (EINVAL);
					return 0;
				}
#endif
			}

			ref -= *ip++;

			if (op + len + 2 > out_end)
			{
				SET_ERRNO (E2BIG);
				return 0;
			}

			if (ref < (u8 *)out_data)
			{
				SET_ERRNO (EINVAL);
				return 0;
			}

			*op++ = *ref++;
			*op++ = *ref++;

			do
				*op++ = *ref++;
			while (--len);
		}
	}
	while (ip < in_end);

	return op - (u8 *)out_data;
}


   
                                      
   

#define HSIZE (1 << (HLOG))

  
                                             
                                                      
                                                           
              
   
#ifndef FRST
# define FRST(p) (((p[0]) << 8) | p[1])
# define NEXT(v,p) (((v) << 8) | p[2])
# define IDX(h) ((((h ^ (h << 5)) >> (3*8 - HLOG)) - h*5) & (HSIZE - 1))
#endif
  
                                                                      
                                              
                                                                   
  
                                                  
                                 
   

#if 0
 
# define FRST(p) (p[0] << 5) ^ p[1]
# define NEXT(v,p) ((v) << 5) ^ p[2]
# define IDX(h) ((h) & (HSIZE - 1))
#endif

#define        MAX_LIT        (1 <<  5)
#define        MAX_OFF        (1 << 13)
#define        MAX_REF        ((1 <<  8) + (1 << 3))

  
                    
  
                              
                                
                                           
  
   

size_t
lzf_compress (const void *const in_data,size_t in_len,
              void *out_data, size_t out_len
#if LZF_STATE_ARG
              , LZF_STATE *htab
#endif
              )
{
#if !LZF_STATE_ARG
	LZF_STATE htab;
#endif
	const u8 **hslot;
	const u8 *ip = (const u8 *)in_data;
	      u8 *op = (u8 *)out_data;
	const u8 *in_end  = ip + in_len;
	      u8 *out_end = op + out_len;
	const u8 *ref = NULL;

	unsigned int hval = FRST (ip);
	size_t off;
	int lit = 0;

#if INIT_HTAB
# if USE_MEMCPY
	memset (htab, 0, sizeof (htab));
# else
	for (hslot = htab; hslot < htab + HSIZE; hslot++)
		*hslot++ = ip;
# endif
#endif

	for (;;)
	{
		if (ip < in_end - 2)
		{
			hval = NEXT (hval, ip);
			hslot = htab + IDX (hval);
			ref = *hslot; *hslot = ip;

			if (
#if INIT_HTAB && !USE_MEMCPY
			     ref < ip  
			     &&
#endif
			     (off = ip - ref - 1) < MAX_OFF
			     && ip + 4 < in_end
			     && ref > (const u8 *)in_data
#if STRICT_ALIGN
			     && ref[0] == ip[0]
			     && ref[1] == ip[1]
			     && ref[2] == ip[2]
#else
			     && *(const u16 *)ref == *(const u16 *)ip
			     && ref[2] == ip[2]
#endif
			     )
			{
				 
				unsigned int len = 2;
				size_t maxlen = in_end - ip - len;
				maxlen = maxlen > MAX_REF ? MAX_REF : maxlen;

				if (op + lit + 1 + 3 >= out_end)
					return 0;

				do
					len++;
				while (len < maxlen && ref[len] == ip[len]);

				if (lit)
				{
					*op++ = (u8)(lit - 1);
					lit = -lit;
					do
						*op++ = ip[lit];
					while (++lit);
				}

				len -= 2;
				ip++;

				if (len < 7)
				{
					*op++ = (u8)((off >> 8) + (len << 5));
				}
				else
				{
					*op++ = (u8)((off >> 8) + (  7 << 5));
					*op++ = (u8)(len - 7);
				}

				*op++ = (u8)off;

#if ULTRA_FAST || VERY_FAST
				ip += len;
#if VERY_FAST && !ULTRA_FAST
				--ip;
#endif
				hval = FRST (ip);

				hval = NEXT (hval, ip);
				htab[IDX (hval)] = ip;
				ip++;

#if VERY_FAST && !ULTRA_FAST
				hval = NEXT (hval, ip);
				htab[IDX (hval)] = ip;
				ip++;
#endif
#else
				do
				{
					hval = NEXT (hval, ip);
					htab[IDX (hval)] = ip;
					ip++;
				}
				while (len--);
#endif
				continue;
			}
		}
		else if (ip == in_end)
			break;

		 
		lit++;
		ip++;

		if (lit == MAX_LIT)
		{
			if (op + 1 + MAX_LIT >= out_end)
				return 0;

			*op++ = MAX_LIT - 1;
#if USE_MEMCPY
			M_Memcpy (op, ip - MAX_LIT, MAX_LIT);
			op += MAX_LIT;
			lit = 0;
#else
			lit = -lit;
			do
				*op++ = ip[lit];
			while (++lit);
#endif
		}
	}

	if (lit)
	{
		if (op + lit + 1 >= out_end)
			return 0;

		*op++ = (u8)(lit - 1);
		lit = -lit;
		do
			*op++ = ip[lit];
		while (++lit);
	}

	return op - (u8 *) out_data;
}
