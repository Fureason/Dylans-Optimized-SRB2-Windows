/*---------------------------------------------------------------------------

   rpng2 - progressive-model PNG display program                 readpng2.c

  ---------------------------------------------------------------------------

      Copyright (c) 1998-2015 Greg Roelofs.  All rights reserved.

      This software is provided "as is," without warranty of any kind,
      express or implied.  In no event shall the author or contributors
      be held liable for any damages arising in any way from the use of
      this software.

      The contents of this file are DUAL-LICENSED.  You may modify and/or
      redistribute this software according to the terms of one of the
      following two licenses (at your option):


      LICENSE 1 ("BSD-like with advertising clause"):

      Permission is granted to anyone to use this software for any purpose,
      including commercial applications, and to alter it and redistribute
      it freely, subject to the following restrictions:

      1. Redistributions of source code must retain the above copyright
         notice, disclaimer, and this list of conditions.
      2. Redistributions in binary form must reproduce the above copyright
         notice, disclaimer, and this list of conditions in the documenta-
         tion and/or other materials provided with the distribution.
      3. All advertising materials mentioning features or use of this
         software must display the following acknowledgment:

            This product includes software developed by Greg Roelofs
            and contributors for the book, "PNG: The Definitive Guide,"
            published by O'Reilly and Associates.


      LICENSE 2 (GNU GPL v2 or later):

      This program is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 2 of the License, or
      (at your option) any later version.

      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program; if not, write to the Free Software Foundation,
      Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  ---------------------------------------------------------------------------

   Changelog:
     2015-11-12 - Check return value of png_get_bKGD() (Glenn R-P)
     2017-04-22 - Guard against integer overflow (Glenn R-P)

  ---------------------------------------------------------------------------*/


#include <stdlib.h>      
#include <setjmp.h>

#include <zlib.h>
#include "png.h"         
#include "readpng2.h"    


 

static void readpng2_info_callback(png_structp png_ptr, png_infop info_ptr);
static void readpng2_row_callback(png_structp png_ptr, png_bytep new_row,
                                 png_uint_32 row_num, int pass);
static void readpng2_end_callback(png_structp png_ptr, png_infop info_ptr);
static void readpng2_error_handler(png_structp png_ptr, png_const_charp msg);
static void readpng2_warning_handler(png_structp png_ptr, png_const_charp msg);




void readpng2_version_info(void)
{
    fprintf(stderr, "   Compiled with libpng %s; using libpng %s\n",
      PNG_LIBPNG_VER_STRING, png_libpng_ver);

    fprintf(stderr, "   and with zlib %s; using zlib %s.\n",
      ZLIB_VERSION, zlib_version);
}




int readpng2_check_sig(uch *sig, int num)
{
    return !png_sig_cmp(sig, 0, num);
}




 

int readpng2_init(mainprog_info *mainprog_ptr)
{
    png_structp  png_ptr;        
    png_infop  info_ptr;


     

    png_ptr = png_create_read_struct(png_get_libpng_ver(NULL), mainprog_ptr,
      readpng2_error_handler, readpng2_warning_handler);
    if (!png_ptr)
        return 4;    

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return 4;    
    }


                                                                          
                                                                        
                                                             


                                                                         
                                                                         
                                                                         
                                                                   

    if (setjmp(mainprog_ptr->jmpbuf)) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return 2;
    }


#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
                                                                             
                                                                               
                                                                          
    {
                                                                          
                                                                       
           
        static const png_byte chunks_to_process[] = {
            98,  75,  71,  68, '\0',   
           103,  65,  77,  65, '\0',   
           115,  82,  71,  66, '\0',   
           };

        
       png_set_keep_unknown_chunks(png_ptr, -1  ,
          NULL, -1);

        
       png_set_keep_unknown_chunks(png_ptr,
          0  , chunks_to_process,
          sizeof(chunks_to_process)/5);
    }
#endif  


                                                                      
                                            

    png_set_progressive_read_fn(png_ptr, mainprog_ptr,
      readpng2_info_callback, readpng2_row_callback, readpng2_end_callback);


     

    mainprog_ptr->png_ptr = png_ptr;
    mainprog_ptr->info_ptr = info_ptr;


     

    return 0;
}




 

int readpng2_decode_data(mainprog_info *mainprog_ptr, uch *rawbuf, ulg length)
{
    png_structp png_ptr = (png_structp)mainprog_ptr->png_ptr;
    png_infop info_ptr = (png_infop)mainprog_ptr->info_ptr;


                                                                         
                         

    if (setjmp(mainprog_ptr->jmpbuf)) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        mainprog_ptr->png_ptr = NULL;
        mainprog_ptr->info_ptr = NULL;
        return 2;
    }


     

    png_process_data(png_ptr, info_ptr, rawbuf, length);

    return 0;
}




static void readpng2_info_callback(png_structp png_ptr, png_infop info_ptr)
{
    mainprog_info  *mainprog_ptr;
    int  color_type, bit_depth;
    png_uint_32 width, height;
#ifdef PNG_FLOATING_POINT_SUPPORTED
    double  gamma;
#else
    png_fixed_point gamma;
#endif


                                                                            
                                                                           
                                                                             
                                                                             
                  


                                                                            
                                                                           
                                                                             
                                                                              
                                       

    mainprog_ptr = png_get_progressive_ptr(png_ptr);

    if (mainprog_ptr == NULL) {          
        fprintf(stderr,
          "readpng2 error:  main struct not recoverable in info_callback.\n");
        fflush(stderr);
        return;
          
                                                                          
                                                                           
                                                                          
                                                                        
                                                                  
           
    }


     

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
       NULL, NULL, NULL);
    mainprog_ptr->width = (ulg)width;
    mainprog_ptr->height = (ulg)height;


                                                                        
                                                            

    if (mainprog_ptr->need_bgcolor)
    {
        png_color_16p pBackground;

                                                                             
                                                                         
                                                              
        if (png_get_bKGD(png_ptr, info_ptr, &pBackground))
        {

                                                                             
                                                                               
              
           if (bit_depth == 16) {
               mainprog_ptr->bg_red   = pBackground->red   >> 8;
               mainprog_ptr->bg_green = pBackground->green >> 8;
               mainprog_ptr->bg_blue  = pBackground->blue  >> 8;
           } else if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
               if (bit_depth == 1)
                   mainprog_ptr->bg_red = mainprog_ptr->bg_green =
                     mainprog_ptr->bg_blue = pBackground->gray? 255 : 0;
               else if (bit_depth == 2)
                   mainprog_ptr->bg_red = mainprog_ptr->bg_green =
                     mainprog_ptr->bg_blue = (255/3) * pBackground->gray;
               else  
                   mainprog_ptr->bg_red = mainprog_ptr->bg_green =
                     mainprog_ptr->bg_blue = (255/15) * pBackground->gray;
           } else {
               mainprog_ptr->bg_red   = (uch)pBackground->red;
               mainprog_ptr->bg_green = (uch)pBackground->green;
               mainprog_ptr->bg_blue  = (uch)pBackground->blue;
           }
        }
    }


                                                                        
                                                                             
                                                                       
                             

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_expand(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_expand(png_ptr);
#ifdef PNG_READ_16_TO_8_SUPPORTED
    if (bit_depth == 16)
#  ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
        png_set_scale_16(png_ptr);
#  else
        png_set_strip_16(png_ptr);
#  endif
#endif
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);


                                                                             
                                                                         
                                                                           
                                                                              
                                                                          
                                                                           
                                                                               
                                                                           
                                                                          
                                                                            
                                                                          
                                                                           
                                                                        
                                        

#ifdef PNG_FLOATING_POINT_SUPPORTED
    if (png_get_gAMA(png_ptr, info_ptr, &gamma))
        png_set_gamma(png_ptr, mainprog_ptr->display_exponent, gamma);
    else
        png_set_gamma(png_ptr, mainprog_ptr->display_exponent, 0.45455);
#else
    if (png_get_gAMA_fixed(png_ptr, info_ptr, &gamma))
        png_set_gamma_fixed(png_ptr,
            (png_fixed_point)(100000*mainprog_ptr->display_exponent+.5), gamma);
    else
        png_set_gamma_fixed(png_ptr,
            (png_fixed_point)(100000*mainprog_ptr->display_exponent+.5), 45455);
#endif

     

    mainprog_ptr->passes = png_set_interlace_handling(png_ptr);


                                                                             
                                        

    png_read_update_info(png_ptr, info_ptr);

    mainprog_ptr->rowbytes = (int)png_get_rowbytes(png_ptr, info_ptr);
    mainprog_ptr->channels = png_get_channels(png_ptr, info_ptr);


                                                                        
                                                                       
                                                                      
                                                                       

    (*mainprog_ptr->mainprog_init)();


     

    return;
}





static void readpng2_row_callback(png_structp png_ptr, png_bytep new_row,
                                  png_uint_32 row_num, int pass)
{
    mainprog_info  *mainprog_ptr;


                                                                          
                                       

    if (!new_row)
        return;


                                                                          
                                                          

    mainprog_ptr = png_get_progressive_ptr(png_ptr);


     

    mainprog_ptr->pass = pass;


                                                                             
                                                                         
                                              

    png_progressive_combine_row(png_ptr, mainprog_ptr->row_pointers[row_num],
      new_row);


                                                                            
                                    

    (*mainprog_ptr->mainprog_display_row)(row_num);


     

    return;
}





static void readpng2_end_callback(png_structp png_ptr, png_infop info_ptr)
{
    mainprog_info  *mainprog_ptr;


     

    mainprog_ptr = png_get_progressive_ptr(png_ptr);


                                                                        
                                                                          
                                                                         
                                                                         
                                                                  
                                

    (*mainprog_ptr->mainprog_finish_display)();


     

    (void)info_ptr;  

    return;
}





void readpng2_cleanup(mainprog_info *mainprog_ptr)
{
    png_structp png_ptr = (png_structp)mainprog_ptr->png_ptr;
    png_infop info_ptr = (png_infop)mainprog_ptr->info_ptr;

    if (png_ptr && info_ptr)
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    mainprog_ptr->png_ptr = NULL;
    mainprog_ptr->info_ptr = NULL;
}


static void readpng2_warning_handler(png_structp png_ptr, png_const_charp msg)
{
    fprintf(stderr, "readpng2 libpng warning: %s\n", msg);
    fflush(stderr);
    (void)png_ptr;  
}


static void readpng2_error_handler(png_structp png_ptr, png_const_charp msg)
{
    mainprog_info  *mainprog_ptr;

                                                                        
                                                                          
                                                                      
                                                                        
                                                                     
                                                                     
                                                                          
                       

    fprintf(stderr, "readpng2 libpng error: %s\n", msg);
    fflush(stderr);

    mainprog_ptr = png_get_error_ptr(png_ptr);
    if (mainprog_ptr == NULL) {          
        fprintf(stderr,
          "readpng2 severe error:  jmpbuf not recoverable; terminating.\n");
        fflush(stderr);
        exit(99);
    }

                                                                      
                                                                     
                                                                         
                                                                        
                                                                         
                                                                      
       
    longjmp(mainprog_ptr->jmpbuf, 1);
}
