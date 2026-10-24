// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1995-1996 by the Free Software Foundation, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------




                                                                       
                                                                  

#ifndef _MD5_H
#define _MD5_H 1

#include <stdio.h>

#if defined (HAVE_LIMITS_H) || (defined (_LIBC) && _LIBC) || defined (_WIN32)
# include <limits.h>
#endif

                                                                     
                                                                   
                                                                       
                                                                       
                                                                        
                               

#ifdef _LIBC
# include <sys/types.h>
typedef u_int32_t md5_uint32;
#else
# if defined __STDC__ && __STDC__
#  define UINT_MAX_32_BITS 4294967295U
# else
#  define UINT_MAX_32_BITS 0xFFFFFFFF
# endif

                                                        
                                                               
                                                               
                                                                    

# ifndef UINT_MAX
#  define UINT_MAX UINT_MAX_32_BITS
# endif

# if UINT_MAX == UINT_MAX_32_BITS
   typedef unsigned int md5_uint32;
# else
#  if USHRT_MAX == UINT_MAX_32_BITS
    typedef unsigned short md5_uint32;
#  else
#   if ULONG_MAX == UINT_MAX_32_BITS
     typedef unsigned long md5_uint32;
#   else
                                                         
                                                
     "Cannot determine unsigned 32-bit data type."
#   endif
#  endif
# endif
#endif

#undef __P
#if defined (__STDC__) && __STDC__
#define	__P(x) x
#else
#define	__P(x) ()
#endif

  
                                                                   
                                               
   
#if 0
                                                        
                              
extern void md5_init_ctx __P ((struct md5_ctx *ctx));

                                                                    
                                                                    
                      
                                                     
extern void md5_process_block __P ((const void *buffer, size_t len,
                                   struct md5_ctx *ctx));

                                                                    
                                                                    
                      
                                                       
extern void md5_process_bytes __P ((const void *buffer, size_t len,
                                    struct md5_ctx *ctx));

                                                                    
                                                                      
                                                                     
                                              

                                                                     
                                   
extern void *md5_finish_ctx __P ((struct md5_ctx *ctx, void *resbuf));


                                                                         
                                                                        
                                                            

                                                                     
                                   
extern void *md5_read_ctx __P ((const struct md5_ctx *ctx, void *resbuf));

#endif

                                                              
                                                                    
                             
int md5_stream __P ((FILE *stream, void *resblock));

                                                                     
                                                                    
                                                                  
              
extern void *md5_buffer __P ((const char *buffer, size_t len, void *resblock));

#endif
