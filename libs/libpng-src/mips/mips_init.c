
/* mips_init.c - MSA optimised filter functions
 *
 * Copyright (c) 2018 Cosmin Truta
 * Copyright (c) 2016 Glenn Randers-Pehrson
 * Written by Mandar Sahastrabuddhe, 2016.
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 */

                                                                              
          
   
#define _POSIX_SOURCE 1

#include <stdio.h>
#include "../pngpriv.h"

#ifdef PNG_READ_SUPPORTED

#if PNG_MIPS_MSA_OPT > 0
#ifdef PNG_MIPS_MSA_CHECK_SUPPORTED  
                                                                        
                                                                                
                                                                       
                                                                              
               
  
                                                                              
                                                                              
                                                                             
                                                                    
                                            
   
#ifndef PNG_MIPS_MSA_FILE
#  ifdef __linux__
#     define PNG_MIPS_MSA_FILE "contrib/mips-msa/linux.c"
#  endif
#endif

#ifdef PNG_MIPS_MSA_FILE

#include <signal.h>  
static int png_have_msa(png_structp png_ptr);
#include PNG_MIPS_MSA_FILE

#else   
#  error "PNG_MIPS_MSA_FILE undefined: no support for run-time MIPS MSA checks"
#endif  
#endif  

#ifndef PNG_ALIGNED_MEMORY_SUPPORTED
#  error "ALIGNED_MEMORY is required; set: -DPNG_ALIGNED_MEMORY_SUPPORTED"
#endif

void
png_init_filter_functions_msa(png_structp pp, unsigned int bpp)
{
                                                                       
                                                                         
                                                                          
                                                                      
      

#ifdef PNG_MIPS_MSA_API_SUPPORTED
   switch ((pp->options >> PNG_MIPS_MSA) & 3)
   {
      case PNG_OPTION_UNSET:
                                                                        
                                                                            
                                                                          
                    
            
#endif  
#ifdef PNG_MIPS_MSA_CHECK_SUPPORTED
         {
            static volatile sig_atomic_t no_msa = -1;  

            if (no_msa < 0)
               no_msa = !png_have_msa(pp);

            if (no_msa)
               return;
         }
#ifdef PNG_MIPS_MSA_API_SUPPORTED
         break;
#endif
#endif  

#ifdef PNG_MIPS_MSA_API_SUPPORTED
      default:  
         return;

      case PNG_OPTION_ON:
          
         break;
   }
#endif

                                                                            
                                                                          
                                         
     
                                                 
     
                                                                                
                                                                             
                                                                        
                               
      
   pp->read_filter[PNG_FILTER_VALUE_UP-1] = png_read_filter_row_up_msa;

   if (bpp == 3)
   {
      pp->read_filter[PNG_FILTER_VALUE_SUB-1] = png_read_filter_row_sub3_msa;
      pp->read_filter[PNG_FILTER_VALUE_AVG-1] = png_read_filter_row_avg3_msa;
      pp->read_filter[PNG_FILTER_VALUE_PAETH-1] = png_read_filter_row_paeth3_msa;
   }

   else if (bpp == 4)
   {
      pp->read_filter[PNG_FILTER_VALUE_SUB-1] = png_read_filter_row_sub4_msa;
      pp->read_filter[PNG_FILTER_VALUE_AVG-1] = png_read_filter_row_avg4_msa;
      pp->read_filter[PNG_FILTER_VALUE_PAETH-1] = png_read_filter_row_paeth4_msa;
   }
}
#endif  
#endif  
