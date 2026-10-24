/* blast.h -- interface for blast.c
  Copyright (C) 2003, 2012, 2013 Mark Adler
  version 1.3, 24 Aug 2013

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler    madler@alumni.caltech.edu
 */


  
                                                                            
                                                                           
                                                                           
                                                                       
                                                                        
  
                                                                        
                                                                       
                                         
   


typedef unsigned (*blast_in)(void *how, unsigned char **buf);
typedef int (*blast_out)(void *how, unsigned char *buf, unsigned len);
                                                                           
                                          
   


int blast(blast_in infun, void *inhow, blast_out outfun, void *outhow,
          unsigned *left, unsigned char **in);
                                                                            
                                                                            
                                                                              
                                                                              
                                                   
  
                                                                             
                                                                          
                                                                             
                                                                               
                                                                             
  
                                                                            
                                                                             
  
                                                                               
                                                                             
                                                                             
                                                                             
           
  
                                                                              
                                                                              
                                                           
  
                        
  
                                                         
                                                     
                                 
                                     
                                    
                                 
  
                                                                               
                                                                            
   
