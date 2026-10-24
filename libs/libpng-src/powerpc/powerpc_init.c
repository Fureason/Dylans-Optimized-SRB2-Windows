
/* powerpc_init.c - POWERPC optimised filter functions
 *
 * Copyright (c) 2018 Cosmin Truta
 * Copyright (c) 2017 Glenn Randers-Pehrson
 * Written by Vadim Barkov, 2017.
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 */

                                                                              
          
   
#define _POSIX_SOURCE 1

#include <stdio.h>
#include "../pngpriv.h"

#ifdef PNG_READ_SUPPORTED

#if PNG_POWERPC_VSX_OPT > 0
#ifdef PNG_POWERPC_VSX_CHECK_SUPPORTED  
                                                                        
                                                                                   
                                                                       
                                                                              
               
  
                                                                                 
                                                                              
                                                                                
                                                                       
                                            
   
#ifndef PNG_POWERPC_VSX_FILE
#  ifdef __linux__
#     define  PNG_POWERPC_VSX_FILE "contrib/powerpc-vsx/linux_aux.c"
#  endif
#endif

#ifdef PNG_POWERPC_VSX_FILE

#include <signal.h>  
static int png_have_vsx(png_structp png_ptr);
#include PNG_POWERPC_VSX_FILE

#else   
#  error "PNG_POWERPC_VSX_FILE undefined: no support for run-time POWERPC VSX checks"
#endif  
#endif  

void
png_init_filter_functions_vsx(png_structp pp, unsigned int bpp)
{
                                                                          
                                                                            
                                                                              
                                                                      
      

#ifdef PNG_POWERPC_VSX_API_SUPPORTED
   switch ((pp->options >> PNG_POWERPC_VSX) & 3)
   {
      case PNG_OPTION_UNSET:
                                                                        
                                                                            
                                                                          
                    
            
#endif  
#ifdef PNG_POWERPC_VSX_CHECK_SUPPORTED
         {
            static volatile sig_atomic_t no_vsx = -1;  

            if (no_vsx < 0)
               no_vsx = !png_have_vsx(pp);

            if (no_vsx)
               return;
         }
#ifdef PNG_POWERPC_VSX_API_SUPPORTED
         break;
#endif
#endif  

#ifdef PNG_POWERPC_VSX_API_SUPPORTED
      default:  
         return;

      case PNG_OPTION_ON:
          
         break;
   }
#endif

                                                                            
                                                                          
                                         
     
                                                 
     
                                                                                
                                                                             
                                                                        
                               
      
   pp->read_filter[PNG_FILTER_VALUE_UP-1] = png_read_filter_row_up_vsx;

   if (bpp == 3)
   {
      pp->read_filter[PNG_FILTER_VALUE_SUB-1] = png_read_filter_row_sub3_vsx;
      pp->read_filter[PNG_FILTER_VALUE_AVG-1] = png_read_filter_row_avg3_vsx;
      pp->read_filter[PNG_FILTER_VALUE_PAETH-1] = png_read_filter_row_paeth3_vsx;
   }

   else if (bpp == 4)
   {
      pp->read_filter[PNG_FILTER_VALUE_SUB-1] = png_read_filter_row_sub4_vsx;
      pp->read_filter[PNG_FILTER_VALUE_AVG-1] = png_read_filter_row_avg4_vsx;
      pp->read_filter[PNG_FILTER_VALUE_PAETH-1] = png_read_filter_row_paeth4_vsx;
   }
}
#endif  
#endif  
