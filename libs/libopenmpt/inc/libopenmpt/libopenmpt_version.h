/*
 * libopenmpt_version.h
 * --------------------
 * Purpose: libopenmpt public interface version
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_VERSION_H
#define LIBOPENMPT_VERSION_H

                          
    
  

 
#define OPENMPT_API_VERSION_MAJOR 0
 
#define OPENMPT_API_VERSION_MINOR 4
 
#define OPENMPT_API_VERSION_PATCH 7
 
#define OPENMPT_API_VERSION_PREREL ""
 
#define OPENMPT_API_VERSION_IS_PREREL 0

                                                              
              
                                                                                       
                  
                                                       
                                                   
                                                                                                             
                   
                     
   
#define OPENMPT_API_VERSION_MAKE(major, minor, patch) (((major)<<24)|((minor)<<16)|((patch)<<0))

 
#define OPENMPT_API_VERSION OPENMPT_API_VERSION_MAKE(OPENMPT_API_VERSION_MAJOR, OPENMPT_API_VERSION_MINOR, OPENMPT_API_VERSION_PATCH)

                                                                            
              
                                                                                       
                  
                                                       
                                                       
                                                                                                                                                   
                   
                     
   
#define OPENMPT_API_VERSION_AT_LEAST(major, minor, patch) (OPENMPT_API_VERSION >= OPENMPT_API_VERSION_MAKE((major), (minor), (patch)))

                                                                          
              
                                                                                       
                  
                                                       
                                                     
                                                                                                                                                
                   
                     
   
#define OPENMPT_API_VERSION_BEFORE(major, minor, patch) (OPENMPT_API_VERSION < OPENMPT_API_VERSION_MAKE((major), (minor), (patch)))

#define OPENMPT_API_VERSION_HELPER_STRINGIZE(x) #x
#define OPENMPT_API_VERSION_STRINGIZE(x) OPENMPT_API_VERSION_HELPER_STRINGIZE(x)
#define OPENMPT_API_VERSION_STRING OPENMPT_API_VERSION_STRINGIZE(OPENMPT_API_VERSION_MAJOR) "." OPENMPT_API_VERSION_STRINGIZE(OPENMPT_API_VERSION_MINOR) "." OPENMPT_API_VERSION_STRINGIZE(OPENMPT_API_VERSION_PATCH) OPENMPT_API_VERSION_PREREL

   
    
  

#endif  
