#ifndef CURLINC_CURLVER_H
#define CURLINC_CURLVER_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

                                                                           
                                                                           

/* This is the global package copyright */
#define LIBCURL_COPYRIGHT "1996 - 2020 Daniel Stenberg, <daniel@haxx.se>."

                                                                           
                   
#define LIBCURL_VERSION "7.69.0"

                                                                         
              
#define LIBCURL_VERSION_MAJOR 7
#define LIBCURL_VERSION_MINOR 69
#define LIBCURL_VERSION_PATCH 0

                                                                              
                                                                          
                             

                 

                                                                         
                                                                            
                                                                         
                         

                                                                              
                                                                       
                                                    

                                                                  
                                                                           
                                           
  
#define LIBCURL_VERSION_NUM 0x074500

  
                                                                          
                                                                          
                                  
  
                                                
  
               
   
#define LIBCURL_TIMESTAMP "2020-03-04"

#define CURL_VERSION_BITS(x,y,z) ((x)<<16|(y)<<8|(z))
#define CURL_AT_LEAST_VERSION(x,y,z) \
  (LIBCURL_VERSION_NUM >= CURL_VERSION_BITS(x, y, z))

#endif  
