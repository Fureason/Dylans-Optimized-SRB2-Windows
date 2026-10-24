
#if 0  

/* example.c - an example of using libpng
 *
 * Maintained 2018 Cosmin Truta
 * Maintained 1998-2016 Glenn Randers-Pehrson
 * Maintained 1996-1997 Andreas Dilger
 * Written 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * To the extent possible under law, the authors have waived
 * all copyright and related or neighboring rights to this file.
 * This work is published from: United States, Canada.
 */

/* This is an example of how to use libpng to read and write PNG files.
 * The file libpng-manual.txt is much more verbose then this.  If you have
 * not read it, do so first.  This was designed to be a starting point of an
 * implementation.  This is not officially part of libpng, is hereby placed
 * in the public domain, and therefore does not require a copyright notice.
 *
 * This file does not currently compile, because it is missing certain
 * parts, like allocating memory to hold an image.  You will have to
 * supply these parts to get it to compile.  For an example of a minimal
 * working PNG reader/writer, see pngtest.c, included in this distribution;
 * see also the programs in the contrib directory.
 */

                                                                           
                                                                          
                                                                          
                      
  
                                                                          
                                                                              
                                                                           
                                  
   

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <png.h>
#include <zlib.h>

int main(int argc, const char **argv)
{
   if (argc == 3)
   {
      png_image image;  

       
      memset(&image, 0, (sizeof image));
      image.version = PNG_IMAGE_VERSION;

       
      if (png_image_begin_read_from_file(&image, argv[1]) != 0)
      {
         png_bytep buffer;

                                                                             
                                                                               
                              
            
         image.format = PNG_FORMAT_RGBA;

                                                                            
                                                                             
                                                 
            
         buffer = malloc(PNG_IMAGE_SIZE(image));

                                                                         
                                                                               
                                                                              
                                                                            
                                                                              
                                                                             
                                               
           
                                                                              
                                                                            
                                                                       
                                                                        
                                                                              
                 
           
                                                                         
                                                                           
                                                                      
                                                          
                                                                            
                                                                               
                                                                           
           
                                             
           
                                                              
            
         if (buffer != NULL &&
            png_image_finish_read(&image, NULL , buffer,
                0 , NULL ) != 0)
         {
                                                                            
                                                                               
                                                                              
                                   
               
            if (png_image_write_to_file(&image, argv[2], 0 ,
                buffer, 0 , NULL ) != 0)
            {
                
               exit(0);
            }
         }
         else
         {
                                                                               
                                                                           
                                                                           
                              
               
            if (buffer == NULL)
               png_image_free(&image);
            else
               free(buffer);
      }

                                                                            
                                                      
         
      fprintf(stderr, "pngtopng: error: %s\n", image.message);
      exit(1);
   }

   fprintf(stderr, "pngtopng: usage: pngtopng input-file output-file\n");
   exit(2);
}

                                                                            
                                                                             
                                                                               
                                                                           
                                                                              
              
  
                                                                             
                                                                              
                                                          
  
                                                                            
                                                                               
                                      
  
                                                                      
                                                                          
                                                                               
              
  
                                                                       
                                                                              
                                                                           
                                                                               
                                                                              
                                                                             
                                                                         
                                                                               
                                                                           
                                                                               
               
  
                                                                       
                                                                              
                                                             
  
                                                                              
                                                                               
                 
  
                                                                              
                                                                             
                                                                      
                                                                           
                                                                       
                                 
  
                                                                             
                                                                               
                  
  
                                                                         
                                                                             
                                                                        
                                                                               
                                                                         
   

                                                                               
                                                                               
                                                                         
              
  
                                                                          
                                                                              
                                                                            
                                                                            
                                                                      
                                
  
                                                                               
                                                                          
                                           
   
#include <png.h>

                                                                       
                                                                             
                                                                          
                                      
    

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->png_jmpbuf)
#endif

                                                                           
                                                             
  
                                                                               
                                                                   
  
                                                                       
                                                                       
                                                                      
                                                                      
                                                                        
                                                                      
                                                                       
                                           
  
                                                                       
                                                                       
                                                                    
                                                     
   
#define PNG_BYTES_TO_CHECK 4
int check_if_png(char *file_name, FILE **fp)
{
   char buf[PNG_BYTES_TO_CHECK];

    
   if ((*fp = fopen(file_name, "rb")) == NULL)
      return 0;

    
   if (fread(buf, 1, PNG_BYTES_TO_CHECK, *fp) != PNG_BYTES_TO_CHECK)
      return 0;

                                                                  
                                          
      
   return(!png_sig_cmp(buf, 0, PNG_BYTES_TO_CHECK));
}

                                                                     
                                                                        
                                                                      
                                                                     
                                                             
   
#ifdef open_file  
void read_png(char *file_name)  
{
   png_structp png_ptr;
   png_infop info_ptr;
   int sig_read = 0;
   png_uint_32 width, height;
   int bit_depth, color_type, interlace_type;
   FILE *fp;

   if ((fp = fopen(file_name, "rb")) == NULL)
      return (ERROR);

#else no_open_file  
void read_png(FILE *fp, int sig_read)  
{
   png_structp png_ptr;
   png_infop info_ptr;
   png_uint_32 width, height;
   int bit_depth, color_type, interlace_type;
#endif no_open_file  

                                                                         
                                                                            
                                                                            
                                                                          
                                                                       
      
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
       png_voidp user_error_ptr, user_error_fn, user_warning_fn);

   if (png_ptr == NULL)
   {
      fclose(fp);
      return (ERROR);
   }

    
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      return (ERROR);
   }

                                                                            
                                                                          
                                                                             
      
   if (setjmp(png_jmpbuf(png_ptr)))
   {
       
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      fclose(fp);
       
      return (ERROR);
   }

    
#ifdef streams  
    
   png_init_io(png_ptr, fp);

#else no_streams  
                                                                     
                                    
      
   png_set_read_fn(png_ptr, (void *)user_io_ptr, user_read_fn);
    
#endif no_streams  

    
   png_set_sig_bytes(png_ptr, sig_read);

#ifdef hilevel
                                                                    
                                                                    
                                                                   
                                                              
                                                                
                                                     
      
   png_read_png(png_ptr, info_ptr, png_transforms, NULL);

#else
    

                                                                          
                                                                   
      
   png_read_info(png_ptr, info_ptr);

   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
       &interlace_type, NULL, NULL);

                                                                        
                                                                   
                                                                     
                             
      

                                                                    
                                                                         
               
      
#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
   png_set_scale_16(png_ptr);
#else
   png_set_strip_16(png_ptr);
#endif

                                                                      
                                   
      
   png_set_strip_alpha(png_ptr);

                                                                        
                                                                          
      
   png_set_packing(png_ptr);

                                                                      
                                                    
      
   png_set_packswap(png_ptr);

    
   if (color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png_ptr);

    
   if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      png_set_expand_gray_1_2_4_to_8(png_ptr);

                                                                            
                                                     
      
   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0)
      png_set_tRNS_to_alpha(png_ptr);

                                                                         
                                                                       
                                                                            
                                                                         
                                                                               
      
   png_color_16 my_background, *image_background;

   if (png_get_bKGD(png_ptr, info_ptr, &image_background) != 0)
      png_set_background(png_ptr, image_background,
          PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
   else
      png_set_background(png_ptr, &my_background,
          PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);

                                                             
     
                                                                    
                                                                 
      
   if ( )
      screen_gamma = user-defined screen_gamma;
    
   else if ((gamma_str = getenv("SCREEN_GAMMA")) != NULL)
      screen_gamma = atof(gamma_str);
    
   else
   {
      screen_gamma = PNG_DEFAULT_sRGB;                                 
                                                                
      screen_gamma = PNG_GAMMA_MAC_18 or 1.0;                        
                                                           
   }

                                                                         
                                                                            
                                                                            
                              
      

   int intent;

   if (png_get_sRGB(png_ptr, info_ptr, &intent) != 0)
      png_set_gamma(png_ptr, screen_gamma, PNG_DEFAULT_sRGB);
   else
   {
      double image_gamma;
      if (png_get_gAMA(png_ptr, info_ptr, &image_gamma) != 0)
         png_set_gamma(png_ptr, screen_gamma, image_gamma);
      else
         png_set_gamma(png_ptr, screen_gamma, 0.45455);
   }

#ifdef PNG_READ_QUANTIZE_SUPPORTED
                                                                  
                                                       
      
   if ((color_type & PNG_COLOR_MASK_COLOR) != 0)
   {
      int num_palette;
      png_colorp palette;

       
      if ( )
      {
          
         png_color std_color_cube[MAX_SCREEN_COLORS];
         png_set_quantize(png_ptr, std_color_cube, MAX_SCREEN_COLORS,
             MAX_SCREEN_COLORS, NULL, 0);
      }
       
      else if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette) != 0)
      {
         png_uint_16p histogram = NULL;
         png_get_hIST(png_ptr, info_ptr, &histogram);
         png_set_quantize(png_ptr, palette, num_palette,
             max_screen_colors, histogram, 0);
      }
   }
#endif  

    
   png_set_invert_mono(png_ptr);

                                                                     
                                                                      
                                
      
   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_sBIT) != 0)
   {
      png_color_8p sig_bit_p;
      png_get_sBIT(png_ptr, info_ptr, &sig_bit_p);
      png_set_shift(png_ptr, sig_bit_p);
   }

    
   if ((color_type & PNG_COLOR_MASK_COLOR) != 0)
      png_set_bgr(png_ptr);

    
   png_set_swap_alpha(png_ptr);

    
   png_set_swap(png_ptr);

    
   png_set_filler(png_ptr, 0xffff, PNG_FILLER_AFTER);

#ifdef PNG_READ_INTERLACING_SUPPORTED
                                                                
                                                                 
                                          
      
   number_passes = png_set_interlace_handling(png_ptr);
#else  
   number_passes = 1;
#endif  

                                                                          
                                                                         
                                                                            
      
   png_read_update_info(png_ptr, info_ptr);

    
   png_bytep row_pointers[height];
   for (row = 0; row < height; row++)
      row_pointers[row] = NULL;  
   for (row = 0; row < height; row++)
      row_pointers[row] = png_malloc(png_ptr, png_get_rowbytes(png_ptr,
          info_ptr));

    
#ifdef entire  
   png_read_image(png_ptr, row_pointers);

#else no_entire  
    
   for (pass = 0; pass < number_passes; pass++)
   {
#ifdef single  
      for (y = 0; y < height; y++)
         png_read_rows(png_ptr, &row_pointers[y], NULL, 1);

#else no_single  
      for (y = 0; y < height; y += number_of_rows)
      {
#ifdef sparkle  
         png_read_rows(png_ptr, &row_pointers[y], NULL,
             number_of_rows);
#else no_sparkle  
         png_read_rows(png_ptr, NULL, &row_pointers[y],
             number_of_rows);
#endif no_sparkle  
      }

       
#endif no_single  
   }
#endif no_entire  

    
   png_read_end(png_ptr, info_ptr);
#endif hilevel

    

    
   png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    
   fclose(fp);

    
   return (OK);
}

 

int
initialize_png_reader(png_structp *png_ptr, png_infop *info_ptr)
{
                                                                         
                                                                            
                                                                            
                                                                         
                       
      
   *png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
        png_voidp user_error_ptr, user_error_fn, user_warning_fn);
   if (*png_ptr == NULL)
   {
      *info_ptr = NULL;
      return (ERROR);
   }
   *info_ptr = png_create_info_struct(png_ptr);
   if (*info_ptr == NULL)
   {
      png_destroy_read_struct(png_ptr, info_ptr, NULL);
      return (ERROR);
   }
   if (setjmp(png_jmpbuf((*png_ptr))))
   {
      png_destroy_read_struct(png_ptr, info_ptr, NULL);
      return (ERROR);
   }

                                                            
                                           
                                                             
                                                          
                                                     
                                                         
                                                         
                                                            
                                                          
                                                              
                                                    
      
   png_set_progressive_read_fn(*png_ptr, (void *)stream_data,
       info_callback, row_callback, end_callback);
   return (OK);
}

int
process_data(png_structp *png_ptr, png_infop *info_ptr,
    png_bytep buffer, png_uint_32 length)
{
   if (setjmp(png_jmpbuf((*png_ptr))))
   {
       
      png_destroy_read_struct(png_ptr, info_ptr, NULL);
      return (ERROR);
   }

                                                             
                            
                                                             
                                                              
                                                               
                                                             
                                                               
                                                             
                                                            
      
   png_process_data(*png_ptr, *info_ptr, buffer, length);
   return (OK);
}

info_callback(png_structp png_ptr, png_infop info)
{
                                                                     
                                                                      
                                                                  
                                                                  
                                                                 
                                                               
      
}

row_callback(png_structp png_ptr, png_bytep new_row,
    png_uint_32 row_num, int pass)
{
                                                                 
                                                                   
                                                               
     
                                                                      
                                                                      
                                                                      
     
                                                                         
                                                                 
     
                                                   
                                                                        
                  
      

    
   png_bytep old_row = ((png_bytep *)our_data)[row_num];

#ifdef PNG_READ_INTERLACING_SUPPORTED
                                                       
                                         
      
   if (old_row != NULL && new_row != NULL)
      png_progressive_combine_row(png_ptr, old_row, new_row);

                                                                  
                                                                  
                                
     
                                                               
                                                                  
                                                                     
                                                                   
                                                                
                                                        
      
   png_progressive_combine_row(png_ptr, old_row, new_row);

                                                                  
                                                                  
                                                                    
                                                                    
                                                                       
                                  
      
#endif  
}

end_callback(png_structp png_ptr, png_infop info)
{
                                                                 
                                                               
                                                                  
                                                               
                                      
     
                                                                 
                                  
      
}

 
void write_png(char *file_name  )
{
   FILE *fp;
   png_structp png_ptr;
   png_infop info_ptr;
   png_colorp palette;

    
   fp = fopen(file_name, "wb");
   if (fp == NULL)
      return (ERROR);

                                                                         
                                                                            
                                                                            
                                                                          
                                                                   
      
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
       png_voidp user_error_ptr, user_error_fn, user_warning_fn);
   if (png_ptr == NULL)
   {
      fclose(fp);
      return (ERROR);
   }

    
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  NULL);
      return (ERROR);
   }

                                                                       
                                                                     
      
   if (setjmp(png_jmpbuf(png_ptr)))
   {
       
      fclose(fp);
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return (ERROR);
   }

    

#ifdef streams  
    
   png_init_io(png_ptr, fp);

#else no_streams  
                                                                      
                                    
      
   png_set_write_fn(png_ptr, (void *)user_io_ptr, user_write_fn,
       user_IO_flush_function);
    
#endif no_streams  

#ifdef hilevel
                                                               
                                                             
                                                               
      
   png_write_png(png_ptr, info_ptr, png_transforms, NULL);

#else
    

                                                                       
                                                                           
                                                                         
                                                                            
                                                                             
                                                                        
                                                                      
               
      
   png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth,
       PNG_COLOR_TYPE_???, PNG_INTERLACE_????,
       PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    
   palette = (png_colorp)png_malloc(png_ptr,
       PNG_MAX_PALETTE_LENGTH * (sizeof (png_color)));
    
   png_set_PLTE(png_ptr, info_ptr, palette, PNG_MAX_PALETTE_LENGTH);
                                                                            
                                                                             
                        
      

    
   png_color_8 sig_bit;

    
   sig_bit.gray = true_bit_depth;

    
   sig_bit.red = true_red_bit_depth;
   sig_bit.green = true_green_bit_depth;
   sig_bit.blue = true_blue_bit_depth;

    
   sig_bit.alpha = true_alpha_bit_depth;

   png_set_sBIT(png_ptr, info_ptr, &sig_bit);

                                                                      
                                           
      
   png_set_gAMA(png_ptr, info_ptr, gamma);

    
   {
      png_text text_ptr[3];

      char key0[] = "Title";
      char text0[] = "Mona Lisa";
      text_ptr[0].key = key0;
      text_ptr[0].text = text0;
      text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
      text_ptr[0].itxt_length = 0;
      text_ptr[0].lang = NULL;
      text_ptr[0].lang_key = NULL;

      char key1[] = "Author";
      char text1[] = "Leonardo DaVinci";
      text_ptr[1].key = key1;
      text_ptr[1].text = text1;
      text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
      text_ptr[1].itxt_length = 0;
      text_ptr[1].lang = NULL;
      text_ptr[1].lang_key = NULL;

      char key2[] = "Description";
      char text2[] = "<long text>";
      text_ptr[2].key = key2;
      text_ptr[2].text = text2;
      text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
      text_ptr[2].itxt_length = 0;
      text_ptr[2].lang = NULL;
      text_ptr[2].lang_key = NULL;

      png_set_text(write_ptr, write_info_ptr, text_ptr, 3);
   }

    

                                                                            
                                                                       
                                                     
      

    
   png_write_info(png_ptr, info_ptr);

                                                                           
                                             
     
                                                              
                         
                                          
     
                                                                           
                                                 
      

                                                                    
                                                           
                                                                       
                 
      

                                                               
                                                     
      

    
   png_set_invert_mono(png_ptr);

                                                          
                                                  
      
   png_set_shift(png_ptr, &sig_bit);

    
   png_set_packing(png_ptr);

    
   png_set_swap_alpha(png_ptr);

                                                                       
                                                                        
      
   png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);

    
   png_set_bgr(png_ptr);

    
   png_set_swap(png_ptr);

    
   png_set_packswap(png_ptr);

    
   if (interlacing != 0)
      number_passes = png_set_interlace_handling(png_ptr);
   else
      number_passes = 1;

                                                                         
                                                                         
                                                                       
      
   png_uint_32 k, height, width;

    

    
   if (height > PNG_SIZE_MAX / (width * bytes_per_pixel))
      png_error(png_ptr, "Image data buffer would be too large");

   png_byte image[height * width * bytes_per_pixel];
   png_bytep row_pointers[height];

   if (height > PNG_UINT_32_MAX / (sizeof (png_bytep)))
      png_error(png_ptr, "Image is too tall to process in memory");

    
   for (k = 0; k < height; k++)
      row_pointers[k] = image + k * width * bytes_per_pixel;

    

#ifdef entire  
   png_write_image(png_ptr, row_pointers);

    

#else no_entire  

                                                                 
                                 
      
   for (pass = 0; pass < number_passes; pass++)
   {
       
      png_write_rows(png_ptr, &row_pointers[first_row], number_of_rows);

       
      for (y = 0; y < height; y++)
         png_write_rows(png_ptr, &row_pointers[y], 1);
   }
#endif no_entire  

                                                                        
                                                                         
                                                                       
                                                                        
      

    
   png_write_end(png_ptr, info_ptr);
#endif hilevel

                                                  
                                                                               
                                                                              
                                                                           
                            
      
   png_free(png_ptr, palette);
   palette = NULL;

                                                                     
                                                                       
                                                          
      
   png_free(png_ptr, trans);
   trans = NULL;

                                                                          
                                                                        
                                                                             
                              
      

    
   png_destroy_write_struct(&png_ptr, &info_ptr);

    
   fclose(fp);

    
   return (OK);
}

#endif  
