
/* intel_init.c - SSE2 optimized filter functions
 *
 * Copyright (c) 2018 Cosmin Truta
 * Copyright (c) 2016-2017 Glenn Randers-Pehrson
 * Written by Mike Klein and Matt Sarett, Google, Inc.
 * Derived from arm/arm_init.c
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 */

#include "../pngpriv.h"

#ifdef PNG_READ_SUPPORTED
#if PNG_INTEL_SSE_IMPLEMENTATION > 0

void
png_init_filter_functions_sse2(png_structp pp, unsigned int bpp)
{
                                                                              
                          
                                                                            
                                                                       
                                                             
                                                                           
                                                                          
     
   png_debug(1, "in png_init_filter_functions_sse2");
   if (bpp == 3)
   {
      pp->read_filter[PNG_FILTER_VALUE_SUB-1] = png_read_filter_row_sub3_sse2;
      pp->read_filter[PNG_FILTER_VALUE_AVG-1] = png_read_filter_row_avg3_sse2;
      pp->read_filter[PNG_FILTER_VALUE_PAETH-1] =
         png_read_filter_row_paeth3_sse2;
   }
   else if (bpp == 4)
   {
      pp->read_filter[PNG_FILTER_VALUE_SUB-1] = png_read_filter_row_sub4_sse2;
      pp->read_filter[PNG_FILTER_VALUE_AVG-1] = png_read_filter_row_avg4_sse2;
      pp->read_filter[PNG_FILTER_VALUE_PAETH-1] =
          png_read_filter_row_paeth4_sse2;
   }

                                                                
                    
      
}

#endif  
#endif  
