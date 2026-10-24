
/* symbols.c - find all exported symbols
 *
 * Last changed in libpng version 1.6.16 [December 22, 2014]
 * Copyright (c) 2011-2014 Glenn Randers-Pehrson
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 */

                                                           
                                                            
                                                
                                                   
                                                             
   

#define PNG_EXPORTA(ordinal, type, name, args, attributes)\
        PNG_DFN "@" name "@ @@" ordinal "@"
#define PNG_REMOVED(ordinal, type, name, args, attributes)\
        PNG_DFN "; @" name "@ @@" ordinal "@"
#define PNG_EXPORT_LAST_ORDINAL(ordinal)\
        PNG_DFN "; @@" ordinal "@"

                                                                           
               
   
#include "pnglibconf.h.prebuilt"
#include "../png.h"

                                                            
                                                             
                                                         
                                               
  
                                                             
                                               
   
#define PNG_BENIGN_ERRORS_SUPPORTED
#define PNG_ERROR_NUMBERS_SUPPORTED
#define PNG_READ_BIG_ENDIAN_SUPPORTED   
#define PNG_INCH_CONVERSIONS_SUPPORTED
#define PNG_READ_16_TO_8_ACCURATE_SCALE_SUPPORTED
#define PNG_SET_OPTION_SUPPORTED

#undef PNG_H
#include "../png.h"

                                                            
                                                              
                                  
   
#undef  PNG_FLOATING_POINT_SUPPORTED  
#undef  PNG_ERROR_TEXT_SUPPORTED      

#undef PNG_H
#include "../png.h"
