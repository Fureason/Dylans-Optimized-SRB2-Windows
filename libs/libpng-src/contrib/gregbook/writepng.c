/*---------------------------------------------------------------------------

   wpng - simple PNG-writing program                             writepng.c

  ---------------------------------------------------------------------------

      Copyright (c) 1998-2007, 2017 Greg Roelofs.  All rights reserved.

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

  ---------------------------------------------------------------------------*/


#include <stdlib.h>      
#include <zlib.h>

#include "png.h"         
#include "writepng.h"    


 

static void writepng_error_handler(png_structp png_ptr, png_const_charp msg);



void writepng_version_info(void)
{
  fprintf(stderr, "   Compiled with libpng %s; using libpng %s.\n",
    PNG_LIBPNG_VER_STRING, png_libpng_ver);
  fprintf(stderr, "   Compiled with zlib %s; using zlib %s.\n",
    ZLIB_VERSION, zlib_version);
}




                                                                           
                                                            

int writepng_init(mainprog_info *mainprog_ptr)
{
    png_structp  png_ptr;        
    png_infop  info_ptr;
    int color_type, interlace_type;


     

    png_ptr = png_create_write_struct(png_get_libpng_ver(NULL), mainprog_ptr,
      writepng_error_handler, NULL);
    if (!png_ptr)
        return 4;    

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        return 4;    
    }


                                                                         
                                                                         
                                                                         
                                                                     
                                          

    if (setjmp(mainprog_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return 2;
    }


     

    png_init_io(png_ptr, mainprog_ptr->outfile);


                                                                             
                                                                          
                                                                             
                                                                           
                                                                            
                       

    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
  
                                                                         
                                                              
                              
                                              
                                                 
                                           
   


     

    if (mainprog_ptr->pnmtype == 5)
        color_type = PNG_COLOR_TYPE_GRAY;
    else if (mainprog_ptr->pnmtype == 6)
        color_type = PNG_COLOR_TYPE_RGB;
    else if (mainprog_ptr->pnmtype == 8)
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    else {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return 11;
    }

    interlace_type = mainprog_ptr->interlaced? PNG_INTERLACE_ADAM7 :
                                               PNG_INTERLACE_NONE;

    png_set_IHDR(png_ptr, info_ptr, mainprog_ptr->width, mainprog_ptr->height,
      mainprog_ptr->sample_depth, color_type, interlace_type,
      PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    if (mainprog_ptr->gamma > 0.0)
        png_set_gAMA(png_ptr, info_ptr, mainprog_ptr->gamma);

    if (mainprog_ptr->have_bg) {    
        png_color_16  background;

        background.red = mainprog_ptr->bg_red;
        background.green = mainprog_ptr->bg_green;
        background.blue = mainprog_ptr->bg_blue;
        png_set_bKGD(png_ptr, info_ptr, &background);
    }

    if (mainprog_ptr->have_time) {
        png_time  modtime;

        png_convert_from_time_t(&modtime, mainprog_ptr->modtime);
        png_set_tIME(png_ptr, info_ptr, &modtime);
    }

    if (mainprog_ptr->have_text) {
        png_text  text[6];
        int  num_text = 0;

        if (mainprog_ptr->have_text & TEXT_TITLE) {
            text[num_text].compression = PNG_TEXT_COMPRESSION_NONE;
            text[num_text].key = "Title";
            text[num_text].text = mainprog_ptr->title;
            ++num_text;
        }
        if (mainprog_ptr->have_text & TEXT_AUTHOR) {
            text[num_text].compression = PNG_TEXT_COMPRESSION_NONE;
            text[num_text].key = "Author";
            text[num_text].text = mainprog_ptr->author;
            ++num_text;
        }
        if (mainprog_ptr->have_text & TEXT_DESC) {
            text[num_text].compression = PNG_TEXT_COMPRESSION_NONE;
            text[num_text].key = "Description";
            text[num_text].text = mainprog_ptr->desc;
            ++num_text;
        }
        if (mainprog_ptr->have_text & TEXT_COPY) {
            text[num_text].compression = PNG_TEXT_COMPRESSION_NONE;
            text[num_text].key = "Copyright";
            text[num_text].text = mainprog_ptr->copyright;
            ++num_text;
        }
        if (mainprog_ptr->have_text & TEXT_EMAIL) {
            text[num_text].compression = PNG_TEXT_COMPRESSION_NONE;
            text[num_text].key = "E-mail";
            text[num_text].text = mainprog_ptr->email;
            ++num_text;
        }
        if (mainprog_ptr->have_text & TEXT_URL) {
            text[num_text].compression = PNG_TEXT_COMPRESSION_NONE;
            text[num_text].key = "URL";
            text[num_text].text = mainprog_ptr->url;
            ++num_text;
        }
        png_set_text(png_ptr, info_ptr, text, num_text);
    }


     

    png_write_info(png_ptr, info_ptr);


                                                                          
                                                                           
                                                                         
                                                                           
                  


                                                                           
                                                       

    png_set_packing(png_ptr);
 


     

    mainprog_ptr->png_ptr = png_ptr;
    mainprog_ptr->info_ptr = info_ptr;


     

    return 0;
}





 

int writepng_encode_image(mainprog_info *mainprog_ptr)
{
    png_structp png_ptr = (png_structp)mainprog_ptr->png_ptr;
    png_infop info_ptr = (png_infop)mainprog_ptr->info_ptr;


                                                                        
                                     

    if (setjmp(mainprog_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        mainprog_ptr->png_ptr = NULL;
        mainprog_ptr->info_ptr = NULL;
        return 2;
    }


                                                                              
                

    png_write_image(png_ptr, mainprog_ptr->row_pointers);


                                                                            
                                                                          
                                                                              

    png_write_end(png_ptr, NULL);

    return 0;
}





 

int writepng_encode_row(mainprog_info *mainprog_ptr)   
{
    png_structp png_ptr = (png_structp)mainprog_ptr->png_ptr;
    png_infop info_ptr = (png_infop)mainprog_ptr->info_ptr;


                                                                        
                                     

    if (setjmp(mainprog_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        mainprog_ptr->png_ptr = NULL;
        mainprog_ptr->info_ptr = NULL;
        return 2;
    }


     

    png_write_row(png_ptr, mainprog_ptr->image_data);

    return 0;
}





 

int writepng_encode_finish(mainprog_info *mainprog_ptr)    
{
    png_structp png_ptr = (png_structp)mainprog_ptr->png_ptr;
    png_infop info_ptr = (png_infop)mainprog_ptr->info_ptr;


                                                                        
                                     

    if (setjmp(mainprog_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        mainprog_ptr->png_ptr = NULL;
        mainprog_ptr->info_ptr = NULL;
        return 2;
    }


                                                                         
                                                       

    png_write_end(png_ptr, NULL);

    return 0;
}





void writepng_cleanup(mainprog_info *mainprog_ptr)
{
    png_structp png_ptr = (png_structp)mainprog_ptr->png_ptr;
    png_infop info_ptr = (png_infop)mainprog_ptr->info_ptr;

    if (png_ptr && info_ptr)
        png_destroy_write_struct(&png_ptr, &info_ptr);
}





static void writepng_error_handler(png_structp png_ptr, png_const_charp msg)
{
    mainprog_info  *mainprog_ptr;

                                                                        
                                                                          
                                                                      
                                                                        
                                                                     
                                                                     
                                                                          
                       

    fprintf(stderr, "writepng libpng error: %s\n", msg);
    fflush(stderr);

    mainprog_ptr = png_get_error_ptr(png_ptr);
    if (mainprog_ptr == NULL) {          
        fprintf(stderr,
          "writepng severe error:  jmpbuf not recoverable; terminating.\n");
        fflush(stderr);
        exit(99);
    }

                                                                      
                                                                     
                                                                         
                                                                        
                                                                         
                                                                      
       
    longjmp(mainprog_ptr->jmpbuf, 1);
}
