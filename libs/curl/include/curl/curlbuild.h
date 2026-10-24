 
#ifndef __CURL_CURLBUILD_H
#define __CURL_CURLBUILD_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2009, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

 
 
 

  
          
          
  
                                                                     
                                                     
  
                                                                     
                                                                    
                                                                   
  
                                                                        
                                                           
  
          
          
  
                                                                        
                                                                         
  
                                                                            
                                                                             
                                                                            
                                                                   
  
   

 
 
 

#ifdef CURL_SIZEOF_LONG
#  error "CURL_SIZEOF_LONG shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_SIZEOF_LONG_already_defined
#endif

#ifdef CURL_TYPEOF_CURL_SOCKLEN_T
#  error "CURL_TYPEOF_CURL_SOCKLEN_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_TYPEOF_CURL_SOCKLEN_T_already_defined
#endif

#ifdef CURL_SIZEOF_CURL_SOCKLEN_T
#  error "CURL_SIZEOF_CURL_SOCKLEN_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_SIZEOF_CURL_SOCKLEN_T_already_defined
#endif

#ifdef CURL_TYPEOF_CURL_OFF_T
#  error "CURL_TYPEOF_CURL_OFF_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_TYPEOF_CURL_OFF_T_already_defined
#endif

#ifdef CURL_FORMAT_CURL_OFF_T
#  error "CURL_FORMAT_CURL_OFF_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_FORMAT_CURL_OFF_T_already_defined
#endif

#ifdef CURL_FORMAT_CURL_OFF_TU
#  error "CURL_FORMAT_CURL_OFF_TU shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_FORMAT_CURL_OFF_TU_already_defined
#endif

#ifdef CURL_FORMAT_OFF_T
#  error "CURL_FORMAT_OFF_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_FORMAT_OFF_T_already_defined
#endif

#ifdef CURL_SIZEOF_CURL_OFF_T
#  error "CURL_SIZEOF_CURL_OFF_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_SIZEOF_CURL_OFF_T_already_defined
#endif

#ifdef CURL_SUFFIX_CURL_OFF_T
#  error "CURL_SUFFIX_CURL_OFF_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_SUFFIX_CURL_OFF_T_already_defined
#endif

#ifdef CURL_SUFFIX_CURL_OFF_TU
#  error "CURL_SUFFIX_CURL_OFF_TU shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_SUFFIX_CURL_OFF_TU_already_defined
#endif

 
 
 

 
 
#define CURL_PULL_WS2TCPIP_H 1
#ifdef CURL_PULL_WS2TCPIP_H
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN 1
#  endif
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

 
 
#define CURL_PULL_SYS_TYPES_H 1
#ifdef CURL_PULL_SYS_TYPES_H
#  include <sys/types.h>
#endif

 
 
#define CURL_PULL_STDINT_H 1
#ifdef CURL_PULL_STDINT_H
#  include <stdint.h>
#endif

 
 
#define CURL_PULL_INTTYPES_H 1
#ifdef CURL_PULL_INTTYPES_H
#  include <inttypes.h>
#endif

 
 
 
#ifdef CURL_PULL_SYS_SOCKET_H
#  include <sys/socket.h>
#endif

 
#define CURL_SIZEOF_LONG 4

 
#define CURL_TYPEOF_CURL_SOCKLEN_T socklen_t

 
#define CURL_SIZEOF_CURL_SOCKLEN_T 4

 
typedef CURL_TYPEOF_CURL_SOCKLEN_T curl_socklen_t;

 
#define CURL_TYPEOF_CURL_OFF_T int64_t

 
typedef CURL_TYPEOF_CURL_OFF_T curl_off_t;

 
#define CURL_FORMAT_CURL_OFF_T "I64d"

 
#define CURL_FORMAT_CURL_OFF_TU "I64u"

 
#define CURL_FORMAT_OFF_T "%I64d"

 
#define CURL_SIZEOF_CURL_OFF_T 8

 
#define CURL_SUFFIX_CURL_OFF_T LL

 
#define CURL_SUFFIX_CURL_OFF_TU ULL

#endif  
