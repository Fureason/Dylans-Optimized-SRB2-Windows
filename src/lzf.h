// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2000-2005 by Marc Alexander Lehmann <schmorp@schmorp.de>
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef LZF_H
#define LZF_H
#include <stdio.h>

                                                                        
  
                                                                 
                          
  
                                                
  
                                                                        

#define LZF_VERSION 0x0105  

  
                                                               
                                                                   
                    
  
                                                               
                                                                   
                                                                  
                                                                      
                                   
  
                                                                       
                                                                        
                                                                      
                                                                    
                                                        
  
                                       
  
                                                                    
                                                                       
               
  
   
size_t
lzf_compress (const void *const in_data,  size_t in_len,
              void             *out_data, size_t out_len);

  
                                                                   
                                                                        
                                                                    
  
                                                                    
                                                                        
                                                                  
            
  
                                                                         
                          
  
                                                               
   
size_t
lzf_decompress (const void *const in_data,  size_t in_len,
                void             *out_data, size_t out_len);

#endif

