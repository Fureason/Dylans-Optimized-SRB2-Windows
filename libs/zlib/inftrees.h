/* inftrees.h -- header to use inftrees.c
 * Copyright (C) 1995-2005, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

                                                                 
                                                               
                                                          
   

                                                                 
                                                                    
                                                                
                                                                  
                                                                  
                                                              
                                                                  
                                                                 
                                                              
                                                                  
                                                                   
                                                                
                                                                     
typedef struct {
    unsigned char op;            
    unsigned char bits;          
    unsigned short val;          
} code;

                                       
                      
                                                                      
                                                                   
                           
                           
   

                                                                               
                                                                              
                                                                           
                                                                           
                                                                          
                                                                            
                                                                        
                                                                         
                                                                               
                                                                         
              
#define ENOUGH_LENS 852
#define ENOUGH_DISTS 592
#define ENOUGH (ENOUGH_LENS+ENOUGH_DISTS)

 
typedef enum {
    CODES,
    LENS,
    DISTS
} codetype;

int ZLIB_INTERNAL inflate_table(codetype type, unsigned short FAR *lens,
                                unsigned codes, code FAR * FAR *table,
                                unsigned FAR *bits, unsigned short FAR *work);
