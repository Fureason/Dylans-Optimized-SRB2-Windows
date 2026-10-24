/*- pngpixel
 *
 * COPYRIGHT: Written by John Cunningham Bowler, 2011.
 * To the extent possible under law, the author has waived all copyright and
 * related or neighboring rights to this work.  This work is published from:
 * United States.
 *
 * Read a single pixel value from a PNG file.
 *
 * This code illustrates basic 'by-row' reading of a PNG file using libpng.
 * Rows are read until a particular pixel is found; the value of this pixel is
 * then printed on stdout.
 *
 * The code illustrates how to do this on interlaced as well as non-interlaced
 * images.  Normally you would call png_set_interlace_handling() to have libpng
 * deal with the interlace for you, but that obliges you to buffer half of the
 * image to assemble the interlaced rows.  In this code
 * png_set_interlace_handling() is not called and, instead, the code handles the
 * interlace passes directly looking for the required pixel.
 */
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>  

                                                                             
                                                            
   
#include "../../png.h"

#if defined(PNG_READ_SUPPORTED) && defined(PNG_SEQUENTIAL_READ_SUPPORTED)

 
static unsigned int
component(png_const_bytep row, png_uint_32 x, unsigned int c,
   unsigned int bit_depth, unsigned int channels)
{
                                                                                
                                                                               
                                                                                
           
      
   png_uint_32 bit_offset_hi = bit_depth * ((x >> 6) * channels);
   png_uint_32 bit_offset_lo = bit_depth * ((x & 0x3f) * channels + c);

   row = (png_const_bytep)(((const png_byte (*)[8])row) + bit_offset_hi);
   row += bit_offset_lo >> 3;
   bit_offset_lo &= 0x07;

                                                                            
                                                                                
                
      
   switch (bit_depth)
   {
      case 1: return (row[0] >> (7-bit_offset_lo)) & 0x01;
      case 2: return (row[0] >> (6-bit_offset_lo)) & 0x03;
      case 4: return (row[0] >> (4-bit_offset_lo)) & 0x0f;
      case 8: return row[0];
      case 16: return (row[0] << 8) + row[1];
      default:
                                                                              
                          
            
         fprintf(stderr, "pngpixel: invalid bit depth %u\n", bit_depth);
         exit(1);
   }
}

                                                                              
                                                           
   
static void
print_pixel(png_structp png_ptr, png_infop info_ptr, png_const_bytep row,
   png_uint_32 x)
{
   unsigned int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

   switch (png_get_color_type(png_ptr, info_ptr))
   {
      case PNG_COLOR_TYPE_GRAY:
         printf("GRAY %u\n", component(row, x, 0, bit_depth, 1));
         return;

                                                                          
                                                                        
                                                             
         
      case PNG_COLOR_TYPE_PALETTE:
         {
            int index = component(row, x, 0, bit_depth, 1);
            png_colorp palette = NULL;
            int num_palette = 0;

            if ((png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette) &
               PNG_INFO_PLTE) && num_palette > 0 && palette != NULL)
            {
               png_bytep trans_alpha = NULL;
               int num_trans = 0;
               if ((png_get_tRNS(png_ptr, info_ptr, &trans_alpha, &num_trans,
                  NULL) & PNG_INFO_tRNS) && num_trans > 0 &&
                  trans_alpha != NULL)
                  printf("INDEXED %u = %d %d %d %d\n", index,
                     palette[index].red, palette[index].green,
                     palette[index].blue,
                     index < num_trans ? trans_alpha[index] : 255);

               else  
                  printf("INDEXED %u = %d %d %d\n", index,
                     palette[index].red, palette[index].green,
                     palette[index].blue);
            }

            else
               printf("INDEXED %u = invalid index\n", index);
         }
         return;

      case PNG_COLOR_TYPE_RGB:
         printf("RGB %u %u %u\n", component(row, x, 0, bit_depth, 3),
            component(row, x, 1, bit_depth, 3),
            component(row, x, 2, bit_depth, 3));
         return;

      case PNG_COLOR_TYPE_GRAY_ALPHA:
         printf("GRAY+ALPHA %u %u\n", component(row, x, 0, bit_depth, 2),
            component(row, x, 1, bit_depth, 2));
         return;

      case PNG_COLOR_TYPE_RGB_ALPHA:
         printf("RGBA %u %u %u %u\n", component(row, x, 0, bit_depth, 4),
            component(row, x, 1, bit_depth, 4),
            component(row, x, 2, bit_depth, 4),
            component(row, x, 3, bit_depth, 4));
         return;

      default:
         png_error(png_ptr, "pngpixel: invalid color type");
   }
}

int main(int argc, const char **argv)
{
                                                                            
                                                                            
                                                                              
                                                                       
                           
      
   volatile int result = 1 ;

   if (argc == 4)
   {
      long x = atol(argv[1]);
      long y = atol(argv[2]);
      FILE *f = fopen(argv[3], "rb");
      volatile png_bytep row = NULL;

      if (f != NULL)
      {
                                                                         
                                                                           
                                                                             
                                                                          
                                                        
            
         png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
            NULL, NULL, NULL);

         if (png_ptr != NULL)
         {
            png_infop info_ptr = png_create_info_struct(png_ptr);

            if (info_ptr != NULL)
            {
                                                                               
                       
                  

                
               if (setjmp(png_jmpbuf(png_ptr)) == 0)
               {
                  png_uint_32 width, height;
                  int bit_depth, color_type, interlace_method,
                     compression_method, filter_method;
                  png_bytep row_tmp;

                                                                               
                                                                          
                                                                              
                                                                               
                                                                          
                     
                  png_init_io(png_ptr, f);

                                                                             
                                                               
                     
                  png_read_info(png_ptr, info_ptr);

                                                                             
                                                                          
                                                                          
                                                      
                     
                  row = png_malloc(png_ptr, png_get_rowbytes(png_ptr,
                     info_ptr));

                                                                                
                                                                           
                     
                  row_tmp = row;

                                                                                
                                                                              
                                                                             
                     
                  if (png_get_IHDR(png_ptr, info_ptr, &width, &height,
                     &bit_depth, &color_type, &interlace_method,
                     &compression_method, &filter_method))
                  {
                     int passes, pass;

                                                                        
                                                                      
                                                                      
                                  
                        
                     switch (interlace_method)
                     {
                        case PNG_INTERLACE_NONE:
                           passes = 1;
                           break;

                        case PNG_INTERLACE_ADAM7:
                           passes = PNG_INTERLACE_ADAM7_PASSES;
                           break;

                        default:
                           png_error(png_ptr, "pngpixel: unknown interlace");
                     }

                      
                     png_start_read_image(png_ptr);

                     for (pass=0; pass<passes; ++pass)
                     {
                        png_uint_32 ystart, xstart, ystep, xstep;
                        png_uint_32 py;

                        if (interlace_method == PNG_INTERLACE_ADAM7)
                        {
                                                                           
                                                                       
                                                                       
                                                                            
                                                                           
                                         
                              
                           if (PNG_PASS_COLS(width, pass) == 0)
                              continue;

                                                                       
                                                                             
                                       
                              
                           xstart = PNG_PASS_START_COL(pass);
                           ystart = PNG_PASS_START_ROW(pass);
                           xstep = PNG_PASS_COL_OFFSET(pass);
                           ystep = PNG_PASS_ROW_OFFSET(pass);
                        }

                        else
                        {
                           ystart = xstart = 0;
                           ystep = xstep = 1;
                        }

                                                                          
                                                                       
                                              
                           
                        for (py = ystart; py < height; py += ystep)
                        {
                           png_uint_32 px, ppx;

                                                                           
                                                                          
                                                                              
                                                                       
                                                                             
                                                                          
                                                                         
                                                                          
                                           
                              
                           png_read_row(png_ptr, row_tmp, NULL);

                                                                            
                                                                            
                                                    
                              
                           if (y == py) for (px = xstart, ppx = 0;
                              px < width; px += xstep, ++ppx) if (x == px)
                           {
                                                                           
                                        
                                 
                              print_pixel(png_ptr, info_ptr, row_tmp, ppx);

                                                                        
                                                                     
                                 
                              goto pass_loop_end;
                           }  
                        }  
                     }  

                      
                  pass_loop_end:
                     row = NULL;
                     png_free(png_ptr, row_tmp);
                  }

                  else
                     png_error(png_ptr, "pngpixel: png_get_IHDR failed");

               }

               else
               {
                                                                           
                                                                             
                                            
                     
                  if (row != NULL)
                  {
                                                                               
                                                                               
                                                                              
                                 
                        
                     png_bytep row_tmp = row;
                     row = NULL;
                     png_free(png_ptr, row_tmp);
                  }
               }

               png_destroy_info_struct(png_ptr, &info_ptr);
            }

            else
               fprintf(stderr, "pngpixel: out of memory allocating png_info\n");

            png_destroy_read_struct(&png_ptr, NULL, NULL);
         }

         else
            fprintf(stderr, "pngpixel: out of memory allocating png_struct\n");
      }

      else
         fprintf(stderr, "pngpixel: %s: could not open file\n", argv[3]);
   }

   else
       
      fprintf(stderr, "pngpixel: usage: pngpixel x y png-file\n");

   return result;
}
#endif  
