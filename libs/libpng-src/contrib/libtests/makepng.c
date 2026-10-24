 
#define _ISOC99_SOURCE
/* Copyright: */
#define COPYRIGHT "\251 2013,2015 John Cunningham Bowler"
/*
 * Last changed in libpng 1.6.20 [November 24, 2015]
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * Make a test PNG image.  The arguments are as follows:
 *
 *    makepng [--sRGB|--linear|--1.8] [--tRNS] [--nofilters] \
 *       color-type bit-depth [file-name]
 *
 * The color-type may be numeric (and must match the numbers used by the PNG
 * specification) or one of the format names listed below.  The bit-depth is the
 * component bit depth, or the pixel bit-depth for a color-mapped image.
 *
 * Without any options no color-space information is written, with the options
 * an sRGB or the appropriate gAMA chunk is written.  "1.8" refers to the
 * display system used on older Apple computers to correct for high ambient
 * light levels in the viewing environment; it applies a transform of
 * approximately value^(1/1.45) to the color values and so a gAMA chunk of 65909
 * is written (1.45/2.2).
 *
 * The image data is generated internally.  Unless --color is given the images
 * used are as follows:
 *
 * 1 channel: a square image with a diamond, the least luminous colors are on
 *    the edge of the image, the most luminous in the center.
 *
 * 2 channels: the color channel increases in luminosity from top to bottom, the
 *    alpha channel increases in opacity from left to right.
 *
 * 3 channels: linear combinations of, from the top-left corner clockwise,
 *    black, green, white, red.
 *
 * 4 channels: linear combinations of, from the top-left corner clockwise,
 *    transparent, red, green, blue.
 *
 * For color-mapped images a four channel color-map is used and if --tRNS is
 * given the PNG file has a tRNS chunk, as follows:
 *
 * 1-bit: entry 0 is transparent-red, entry 1 is opaque-white
 * 2-bit: entry 0: transparent-green
 *        entry 1: 40%-red
 *        entry 2: 80%-blue
 *        entry 3: opaque-white
 * 4-bit: the 16 combinations of the 2-bit case
 * 8-bit: the 256 combinations of the 4-bit case
 *
 * The palette always has 2^bit-depth entries and the tRNS chunk one fewer.  The
 * image is the 1-channel diamond, but using palette index, not luminosity.
 *
 * For formats other than color-mapped ones if --tRNS is specified a tRNS chunk
 * is generated with all channels equal to the low bits of 0x0101.
 *
 * Image size is determined by the final pixel depth in bits, i.e. channels x
 * bit-depth, as follows:
 *
 * 8 bits or less:    64x64
 * 16 bits:           256x256
 * More than 16 bits: 1024x1024
 *
 * Row filtering is the libpng default but may be turned off (the 'none' filter
 * is used on every row) with the --nofilters option.
 *
 * The images are not interlaced.
 *
 * If file-name is given then the PNG is written to that file, else it is
 * written to stdout.  Notice that stdout is not supported on systems where, by
 * default, it assumes text output; this program makes no attempt to change the
 * text mode of stdout!
 *
 *    makepng --color=<color> ...
 *
 * If --color is given then the whole image has that color, color-mapped images
 * will have exactly one palette entry and all image files with be 16x16 in
 * size.  The color value is 1 to 4 decimal numbers as appropriate for the color
 * type.
 *
 *    makepng --small ...
 *
 * If --small is given the images are no larger than required to include every
 * possible pixel value for the format.
 *
 * For formats with pixels 8 bits or fewer in size the images consist of a
 * single row with 2^pixel-depth pixels, one of every possible value.
 *
 * For formats with 16-bit pixels a 256x256 image is generated containing every
 * possible pixel value.
 *
 * For larger pixel sizes a 256x256 image is generated where the first row
 * consists of each pixel that has identical byte values throughout the pixel
 * followed by rows where the byte values differ within the pixel.
 *
 * In all cases the pixel values are arranged in such a way that the SUB and UP
 * filters give byte sequences for maximal zlib compression.  By default (if
 * --nofilters is not given) the SUB filter is used on the first row and the UP
 * filter on all following rows.
 *
 * The --small option is meant to provide good test-case coverage, however the
 * images are not easy to examine visually.  Without the --small option the
 * images contain identical color values; the pixel values are adjusted
 * according to the gamma encoding with no gamma encoding being interpreted as
 * sRGB.
 *
 * LICENSING
 * =========
 *
 * This code is copyright of the authors, see the COPYRIGHT define above.  The
 * code is licensed as above, using the libpng license.  The code generates
 * images which are solely the product of the code; the options choose which of
 * the many possibilities to generate.  The images that result (but not the code
 * which generates them) are licensed as defined here:
 *
 * IMPORTANT: the COPYRIGHT #define must contain ISO-Latin-1 characters, the
 * IMAGE_LICENSING #define must contain UTF-8 characters.  The 'copyright'
 * symbol 0xA9U (\251) in ISO-Latin-1 encoding and 0xC20xA9 (\302\251) in UTF-8.
 */
#define IMAGE_LICENSING "Dedicated to the public domain per Creative Commons "\
    "license \"CC0 1.0\"; https://creativecommons.org/publicdomain/zero/1.0/"

#include <stddef.h>  
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>

#if defined(HAVE_CONFIG_H) && !defined(PNG_NO_CONFIG_H)
#  include <config.h>
#endif

                                                                              
                                 
   
#ifdef PNG_FREESTANDING_TESTS
#  include <png.h>
#else
#  include "../../png.h"
#endif

#include <zlib.h>

                                                                            
               
   
static unsigned int
flooru(double d)
{
   d = floor(d);
   return (unsigned int)d;
}

static png_byte
floorb(double d)
{
   d = floor(d);
   return (png_byte)d;
}

                                                                                
                     
   
typedef struct chunk_insert
{
   struct chunk_insert *next;
   void               (*insert)(png_structp, png_infop, int, png_charpp);
   int                  nparams;
   png_charp            parameters[1];
} chunk_insert;

static unsigned int
channels_of_type(int color_type)
{
   if (color_type & PNG_COLOR_MASK_PALETTE)
      return 1;

   else
   {
      int channels = 1;

      if (color_type & PNG_COLOR_MASK_COLOR)
         channels = 3;

      if (color_type & PNG_COLOR_MASK_ALPHA)
         return channels + 1;

      else
         return channels;
   }
}

static unsigned int
pixel_depth_of_type(int color_type, int bit_depth)
{
   return channels_of_type(color_type) * bit_depth;
}

static unsigned int
image_size_of_type(int color_type, int bit_depth, unsigned int *colors,
   int small)
{
   if (*colors)
      return 16;

   else
   {
      int pixel_depth = pixel_depth_of_type(color_type, bit_depth);

      if (small)
      {
         if (pixel_depth <= 8)  
            return 1 << pixel_depth;

         else
            return 256;
      }

      else if (pixel_depth < 8)
         return 64;

      else if (pixel_depth > 16)
         return 1024;

      else
         return 256;
   }
}

static void
set_color(png_colorp color, png_bytep trans, unsigned int red,
   unsigned int green, unsigned int blue, unsigned int alpha,
   png_const_bytep gamma_table)
{
   color->red = gamma_table[red];
   color->green = gamma_table[green];
   color->blue = gamma_table[blue];
   *trans = (png_byte)alpha;
}

static int
generate_palette(png_colorp palette, png_bytep trans, int bit_depth,
   png_const_bytep gamma_table, unsigned int *colors)
{
     
                                                                
                                       
                             
                              
                                  
                                                  
                                                   
      
   switch (colors[0])
   {
      default:
         fprintf(stderr, "makepng: --colors=...: invalid count %u\n",
            colors[0]);
         exit(1);

      case 1:
         set_color(palette+0, trans+0, colors[1], colors[1], colors[1], 255,
            gamma_table);
         return 1;

      case 2:
         set_color(palette+0, trans+0, colors[1], colors[1], colors[1],
            colors[2], gamma_table);
         return 1;

      case 3:
         set_color(palette+0, trans+0, colors[1], colors[2], colors[3], 255,
            gamma_table);
         return 1;

      case 4:
         set_color(palette+0, trans+0, colors[1], colors[2], colors[3],
            colors[4], gamma_table);
         return 1;

      case 0:
         if (bit_depth == 1)
         {
            set_color(palette+0, trans+0, 255, 0, 0, 0, gamma_table);
            set_color(palette+1, trans+1, 255, 255, 255, 255, gamma_table);
            return 2;
         }

         else
         {
            unsigned int size = 1U << (bit_depth/2);  
            unsigned int x, y;
            volatile unsigned int ip = 0;

            for (x=0; x<size; ++x) for (y=0; y<size; ++y)
            {
               ip = x + (size * y);

                                                                               
                  
#              define interp(pos, c1, c2) ((pos * c1) + ((size-pos) * c2))
#              define xyinterp(x, y, c1, c2, c3, c4) (((size * size / 2) +\
                  (interp(x, c1, c2) * y + (size-y) * interp(x, c3, c4))) /\
                  (size*size))

               set_color(palette+ip, trans+ip,
                   
                  xyinterp(x, y,   0, 255,   0, 255),
                  xyinterp(x, y, 255,   0,   0, 255),
                  xyinterp(x, y,   0,   0, 255, 255),
                   
                  xyinterp(x, y,   0, 102, 204, 255),
                  gamma_table);
            }

            return ip+1;
         }
   }
}

static void
set_value(png_bytep row, size_t rowbytes, png_uint_32 x, unsigned int bit_depth,
   png_uint_32 value, png_const_bytep gamma_table, double conv)
{
   unsigned int mask = (1U << bit_depth)-1;

   x *= bit_depth;   

   if (value <= mask)
   {
      png_uint_32 offset = x >> 3;

      if (offset < rowbytes && (bit_depth < 16 || offset+1 < rowbytes))
      {
         row += offset;

         switch (bit_depth)
         {
            case 1:
            case 2:
            case 4:
                
               {
                  unsigned int shift = (8 - bit_depth) - (x & 0x7U);

                  mask <<= shift;
                  value = (value << shift) & mask;
                  *row = (png_byte)((*row & ~mask) | value);
               }
               return;

            default:
               fprintf(stderr, "makepng: bad bit depth (internal error)\n");
               exit(1);

            case 16:
               value = flooru(65535*pow(value/65535.,conv)+.5);
               *row++ = (png_byte)(value >> 8);
               *row = (png_byte)value;
               return;

            case 8:
               *row = gamma_table[value];
               return;
         }
      }

      else
      {
         fprintf(stderr, "makepng: row buffer overflow (internal error)\n");
         exit(1);
      }
   }

   else
   {
      fprintf(stderr, "makepng: component overflow (internal error)\n");
      exit(1);
   }
}

static int  
generate_row(png_bytep row, size_t rowbytes, unsigned int y, int color_type,
   int bit_depth, png_const_bytep gamma_table, double conv,
   unsigned int *colors, int small)
{
   int filters = 0;  
   png_uint_32 size_max =
      image_size_of_type(color_type, bit_depth, colors, small)-1;
   png_uint_32 depth_max = (1U << bit_depth)-1;  

   if (colors[0] == 0) if (small)
   {
      unsigned int pixel_depth = pixel_depth_of_type(color_type, bit_depth);

                                                                               
                                                                    
                                               
         
      switch (pixel_depth)
      {
         case 1:
            assert(y == 0 && rowbytes == 1 && size_max == 1);
            row[0] = 0x6CU;  
            filters = PNG_FILTER_NONE;
            break;

         case 2:
            assert(y == 0 && rowbytes == 1 && size_max == 3);
            row[0] = 0x1BU;  
            filters = PNG_FILTER_NONE;
            break;

         case 4:
            assert(y == 0 && rowbytes == 8 && size_max == 15);
            row[0] = 0x01U;
            row[1] = 0x23U;  
            row[2] = 0x45U;
            row[3] = 0x67U;
            row[4] = 0x89U;
            row[5] = 0xABU;
            row[6] = 0xCDU;
            row[7] = 0xEFU;
            filters = PNG_FILTER_SUB;
            break;

         case 8:
                                                                            
                                                                             
                                                                                
                                                                     
               
            assert(y == 0 && rowbytes == 256 && size_max == 255);
            for (;;)
            {
               row[size_max] = 0xFFU & (size_max+1);
               if (size_max == 0)
                  break;
               --size_max;
            }
            filters = PNG_FILTER_SUB;
            break;

         case 16:
                                                                              
                                                                              
                                                                              
                                                                              
                   
              
                                                               
              
                                                    
              
                                                                                
              
                                        
              
                                                                
              
                                                 
              
                                                                 
              
                                        
              
                                                                         
              
                                                                    
                                                                    
               
            assert(rowbytes == 512 && size_max == 255);
            for (;;)
            {
               row[2*size_max  ] = 0xFFU & size_max;
               row[2*size_max+1] = 0xFFU & (size_max+y+1);
               if (size_max == 0)
                  break;
               --size_max;
            }
                                                                               
                                                     
               
            filters = (y == 0 ? PNG_FILTER_SUB+PNG_FILTER_UP : PNG_FILTER_UP);
            break;

         case 24:
         case 32:
         case 48:
         case 64:
                                                                                
                                                                              
                                                                          
                                                                                
                                                                    
               
            assert(size_max == 255 && rowbytes == 256*(pixel_depth>>3));
            pixel_depth >>= 3;  
            while (rowbytes > 0)
            {
               const size_t pixel_index = --rowbytes/pixel_depth;

               if (y == 0)
                  row[rowbytes] = 0xFFU & pixel_index;

               else
               {
                  const size_t byte_offset =
                     rowbytes - pixel_index * pixel_depth;

                  row[rowbytes] =
                     0xFFU & (pixel_index + (byte_offset * 2*y) + 1);
               }
            }
            filters = (y == 0 ? PNG_FILTER_SUB+PNG_FILTER_UP : PNG_FILTER_UP);
            break;

         default:
            assert(0 );
      }
   }

   else switch (channels_of_type(color_type))
   {
                                                                                
                                                                
      
      case 1:
         {
            png_uint_32 x;
            png_uint_32 base = 2*size_max - abs(2*y-size_max);

            for (x=0; x<=size_max; ++x)
            {
               png_uint_32 luma = base - abs(2*x-size_max);

                                                                   
                              
                  
               luma = (luma*depth_max + size_max) / (2*size_max);
               set_value(row, rowbytes, x, bit_depth, luma, gamma_table, conv);
            }
         }
         break;

                                                                               
                                                                   
      
      case 2:
         {
            png_uint_32 alpha = (depth_max * y * 2 + size_max) / (2 * size_max);
            png_uint_32 x;

            for (x=0; x<=size_max; ++x)
            {
               set_value(row, rowbytes, 2*x, bit_depth,
                  (depth_max * x * 2 + size_max) / (2 * size_max), gamma_table,
                  conv);
               set_value(row, rowbytes, 2*x+1, bit_depth, alpha, gamma_table,
                  conv);
            }
         }
         break;

                                                                             
                                  
      
      case 3:
         {
                                                                               
                                                           
                                                                        
                                                                         
               
            png_uint_32 Y = (depth_max * y * 2 + size_max) / (2 * size_max);
            png_uint_32 x;

                                                         
              
                                                  
                                           
                                                 
                                           
               
            for (x=0; x<=size_max; ++x)
            {
               set_value(row, rowbytes, 3*x+0, bit_depth,   Y,
                     gamma_table, conv);
               set_value(row, rowbytes, 3*x+1, bit_depth,  
                  (depth_max * x * 2 + size_max) / (2 * size_max),
                  gamma_table, conv);
               set_value(row, rowbytes, 3*x+2, bit_depth,  
                  (Y * x * 2 + size_max) / (2 * size_max),
                  gamma_table, conv);
            }
         }
         break;

                                                                             
                                       
      
      case 4:
         {
                                                                               
                                                                             
                                                                       
                                                                           
                              
               
            png_uint_32 Y = (depth_max * y * 2 + size_max) / (2 * size_max);
            png_uint_32 x;

                                                         
              
                                                   
                                                   
                                               
                                               
                                                   
               
            for (x=0; x<=size_max; ++x)
            {
               set_value(row, rowbytes, 4*x+0, bit_depth,  
                  ((depth_max-Y) * x * 2 + size_max) / (2 * size_max),
                  gamma_table, conv);
               set_value(row, rowbytes, 4*x+1, bit_depth,  
                  (Y * x * 2 + size_max) / (2 * size_max),
                  gamma_table, conv);
               set_value(row, rowbytes, 4*x+2, bit_depth,  
                  Y - (Y * x * 2 + size_max) / (2 * size_max),
                  gamma_table, conv);
               set_value(row, rowbytes, 4*x+3, bit_depth,  
                  Y + ((depth_max-Y) * x * 2 + size_max) / (2 * size_max),
                  gamma_table, conv);
            }
         }
         break;

      default:
         fprintf(stderr, "makepng: internal bad channel count\n");
         exit(2);
   }

   else if (color_type & PNG_COLOR_MASK_PALETTE)
   {
                                                                               
               
         
      memset(row, 0, rowbytes);
   }

   else if (colors[0] == channels_of_type(color_type))
      switch (channels_of_type(color_type))
      {
         case 1:
            {
               png_uint_32 luma = colors[1];
               png_uint_32 x;

               for (x=0; x<=size_max; ++x)
                  set_value(row, rowbytes, x, bit_depth, luma, gamma_table,
                     conv);
            }
            break;

         case 2:
            {
               png_uint_32 luma = colors[1];
               png_uint_32 alpha = colors[2];
               png_uint_32 x;

               for (x=0; x<size_max; ++x)
               {
                  set_value(row, rowbytes, 2*x, bit_depth, luma, gamma_table,
                     conv);
                  set_value(row, rowbytes, 2*x+1, bit_depth, alpha, gamma_table,
                     conv);
               }
            }
            break;

         case 3:
            {
               png_uint_32 red = colors[1];
               png_uint_32 green = colors[2];
               png_uint_32 blue = colors[3];
               png_uint_32 x;

               for (x=0; x<=size_max; ++x)
               {
                  set_value(row, rowbytes, 3*x+0, bit_depth, red, gamma_table,
                     conv);
                  set_value(row, rowbytes, 3*x+1, bit_depth, green, gamma_table,
                     conv);
                  set_value(row, rowbytes, 3*x+2, bit_depth, blue, gamma_table,
                     conv);
               }
            }
            break;

         case 4:
            {
               png_uint_32 red = colors[1];
               png_uint_32 green = colors[2];
               png_uint_32 blue = colors[3];
               png_uint_32 alpha = colors[4];
               png_uint_32 x;

               for (x=0; x<=size_max; ++x)
               {
                  set_value(row, rowbytes, 4*x+0, bit_depth, red, gamma_table,
                     conv);
                  set_value(row, rowbytes, 4*x+1, bit_depth, green, gamma_table,
                     conv);
                  set_value(row, rowbytes, 4*x+2, bit_depth, blue, gamma_table,
                     conv);
                  set_value(row, rowbytes, 4*x+3, bit_depth, alpha, gamma_table,
                     conv);
               }
            }
         break;

         default:
            fprintf(stderr, "makepng: internal bad channel count\n");
            exit(2);
      }

   else
   {
      fprintf(stderr,
         "makepng: --color: count(%u) does not match channels(%u)\n",
         colors[0], channels_of_type(color_type));
      exit(1);
   }

   return filters;
}


static void PNGCBAPI
makepng_warning(png_structp png_ptr, png_const_charp message)
{
   const char **ep = png_get_error_ptr(png_ptr);
   const char *name;

   if (ep != NULL && *ep != NULL)
      name = *ep;

   else
      name = "makepng";

  fprintf(stderr, "%s: warning: %s\n", name, message);
}

static void PNGCBAPI
makepng_error(png_structp png_ptr, png_const_charp message)
{
   makepng_warning(png_ptr, message);
   png_longjmp(png_ptr, 1);
}

static int  
write_png(const char **name, FILE *fp, int color_type, int bit_depth,
   volatile png_fixed_point gamma, chunk_insert * volatile insert,
   unsigned int filters, unsigned int *colors, int small, int tRNS)
{
   png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
      name, makepng_error, makepng_warning);
   volatile png_infop info_ptr = NULL;
   volatile png_bytep row = NULL;

   if (png_ptr == NULL)
   {
      fprintf(stderr, "makepng: OOM allocating write structure\n");
      return 1;
   }

   if (setjmp(png_jmpbuf(png_ptr)))
   {
      png_structp nv_ptr = png_ptr;
      png_infop nv_info = info_ptr;

      png_ptr = NULL;
      info_ptr = NULL;
      png_destroy_write_struct(&nv_ptr, &nv_info);
      if (row != NULL) free(row);
      return 1;
   }

    
   png_set_benign_errors(png_ptr, 1 );

   /* Max out the text compression level in an attempt to make the license
    * small.   If --small then do the same for the IDAT.
    */
   if (small)
      png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

   png_set_text_compression_level(png_ptr, Z_BEST_COMPRESSION);

   png_init_io(png_ptr, fp);

   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
      png_error(png_ptr, "OOM allocating info structure");

   {
      unsigned int size =
         image_size_of_type(color_type, bit_depth, colors, small);
      unsigned int ysize;
      png_fixed_point real_gamma = 45455;  
      png_byte gamma_table[256];
      double conv;

                                                                                
                                                             
         
      if (small)
      {
         unsigned int pixel_depth =
            pixel_depth_of_type(color_type, bit_depth);

         if (pixel_depth <= 8U)
         {
            assert(size == (1U<<pixel_depth));
            ysize = 1U;
         }

         else
         {
            assert(size == 256U);
            ysize = 256U;
         }
      }

      else
         ysize = size;

                                                                         
                                     
         
      if (gamma == PNG_GAMMA_MAC_18)
         gamma = 65909;

      else if (gamma > 0 && gamma < 1000)
         gamma = PNG_FP_1;

      if (gamma > 0)
         real_gamma = gamma;

      {
         unsigned int i;

         if (real_gamma == 45455) for (i=0; i<256; ++i)
         {
            gamma_table[i] = (png_byte)i;
            conv = 1.;
         }

         else
         {
                                                                      
                                                                     
               
            conv = real_gamma;
            conv /= 45455;

            gamma_table[0] = 0;

            for (i=1; i<255; ++i)
               gamma_table[i] = floorb(pow(i/255.,conv) * 255 + .5);

            gamma_table[255] = 255;
         }
      }

      png_set_IHDR(png_ptr, info_ptr, size, ysize, bit_depth, color_type,
         PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

      if (color_type & PNG_COLOR_MASK_PALETTE)
      {
         int npalette;
         png_color palette[256];
         png_byte trans[256];

         npalette = generate_palette(palette, trans, bit_depth, gamma_table,
            colors);
         png_set_PLTE(png_ptr, info_ptr, palette, npalette);

         if (tRNS)
            png_set_tRNS(png_ptr, info_ptr, trans, npalette-1,
               NULL );

          
         for (npalette=0; npalette<256; ++npalette)
            gamma_table[npalette] = (png_byte)npalette;
      }

      else if (tRNS)
      {
         png_color_16 col;

         col.red = col.green = col.blue = col.gray =
            0x0101U & ((1U<<bit_depth)-1U);
         col.index = 0U;
         png_set_tRNS(png_ptr, info_ptr, NULL , 1U, &col);
      }

      if (gamma == PNG_DEFAULT_sRGB)
         png_set_sRGB(png_ptr, info_ptr, PNG_sRGB_INTENT_ABSOLUTE);

      else if (gamma > 0)  
      {
         png_set_gAMA_fixed(png_ptr, info_ptr, real_gamma);

          
         png_set_cHRM_fixed(png_ptr, info_ptr,
             
              31270, 32900,
              64000, 33000,
              30000, 60000,
              15000,  6000
         );
      }

       
      while (insert != NULL)
      {
         insert->insert(png_ptr, info_ptr, insert->nparams, insert->parameters);
         insert = insert->next;
      }

       
      png_write_info(png_ptr, info_ptr);

       
      png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, filters);

      {
#        ifdef PNG_WRITE_INTERLACING_SUPPORTED
            int passes = png_set_interlace_handling(png_ptr);
#        else  
            int passes = 1;
#        endif  
         int pass;
         size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

         row = malloc(rowbytes);

         if (row == NULL)
            png_error(png_ptr, "OOM allocating row buffer");

         for (pass = 0; pass < passes; ++pass)
         {
            unsigned int y;

            for (y=0; y<ysize; ++y)
            {
               unsigned int row_filters =
                  generate_row(row, rowbytes, y, color_type, bit_depth,
                        gamma_table, conv, colors, small);

               if (row_filters != 0 && filters == PNG_ALL_FILTERS)
                  png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, row_filters);

               png_write_row(png_ptr, row);
            }
         }
      }
   }

    
   png_write_end(png_ptr, info_ptr);

   {
      png_structp nv_ptr = png_ptr;
      png_infop nv_info = info_ptr;

      png_ptr = NULL;
      info_ptr = NULL;
      png_destroy_write_struct(&nv_ptr, &nv_info);
   }
   free(row);
   return 0;
}


static size_t
load_file(png_const_charp name, png_bytepp result)
{
   FILE *fp = tmpfile();

   if (fp != NULL)
   {
      FILE *ip = fopen(name, "rb");

      if (ip != NULL)
      {
         size_t total = 0;
         int ch;

         for (;;)
         {
            ch = getc(ip);
            if (ch == EOF) break;
            putc(ch, fp);
            ++total;
         }

         if (ferror(ip))
         {
            perror(name);
            fprintf(stderr, "%s: read error\n", name);
            (void)fclose(ip);
         }

         else
         {
            (void)fclose(ip);

            if (ferror(fp))
            {
               perror("temporary file");
               fprintf(stderr, "temporary file write error\n");
            }

            else
            {
               rewind(fp);

               if (total > 0)
               {
                                                                              
                                                   
                     
                  png_bytep data = malloc((total+3)&~3);

                  if (data != NULL)
                  {
                     size_t new_size = 0;

                     for (;;)
                     {
                        ch = getc(fp);
                        if (ch == EOF) break;
                        data[new_size++] = (png_byte)ch;
                     }

                     if (ferror(fp) || new_size != total)
                     {
                        perror("temporary file");
                        fprintf(stderr, "temporary file read error\n");
                        free(data);
                     }

                     else
                     {
                        (void)fclose(fp);
                        *result = data;
                        return total;
                     }
                  }

                  else
                     fprintf(stderr, "%s: out of memory loading file\n", name);
               }

               else
                  fprintf(stderr, "%s: empty file\n", name);
            }
         }
      }

      else
      {
         perror(name);
         fprintf(stderr, "%s: open failed\n", name);
      }

      fclose(fp);
   }

   else
      fprintf(stderr, "makepng: %s: could not open temporary file\n", name);

   exit(1);
   return 0;
}

static size_t
load_fake(png_charp param, png_bytepp profile)
{
   char *endptr = NULL;
   uint64_t size = strtoull(param, &endptr, 0 );

    
   if (endptr != NULL && *endptr == '*')
   {
      size_t len = strlen(++endptr);
      size_t result = (size_t)size;

      if (len == 0) len = 1;  

       
      if (result == size && (*profile = malloc(result)) != NULL)
      {
         png_bytep out = *profile;

         if (len == 1)
            memset(out, *endptr, result);

         else
         {
            while (size >= len)
            {
               memcpy(out, endptr, len);
               out += len;
               size -= len;
            }
            memcpy(out, endptr, size);
         }

         return result;
      }

      else
      {
         fprintf(stderr, "%s: size exceeds system limits\n", param);
         exit(1);
      }
   }

   return 0;
}

static void
check_param_count(int nparams, int expect)
{
   if (nparams != expect)
   {
      fprintf(stderr, "bad parameter count (internal error)\n");
      exit(1);
   }
}

static void
insert_iCCP(png_structp png_ptr, png_infop info_ptr, int nparams,
   png_charpp params)
{
   png_bytep profile = NULL;
   png_uint_32 proflen = 0;
   int result;

   check_param_count(nparams, 2);

   switch (params[1][0])
   {
      case '<':
         {
            size_t filelen = load_file(params[1]+1, &profile);
            if (filelen > 0xfffffffc)  
            {
               fprintf(stderr, "%s: file too long (%lu) for an ICC profile\n",
                  params[1]+1, (unsigned long)filelen);
               exit(1);
            }

            proflen = (png_uint_32)filelen;
         }
         break;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
         {
            size_t fake_len = load_fake(params[1], &profile);

            if (fake_len > 0)  
            {
               if (fake_len > 0xffffffff)  
               {
                  fprintf(stderr,
                     "%s: fake data too long (%lu) for an ICC profile\n",
                     params[1], (unsigned long)fake_len);
                  exit(1);
               }
               proflen = (png_uint_32)(fake_len & ~3U);
                
               png_save_uint_32(profile, proflen);
               break;
            }
         }

      default:
         fprintf(stderr, "--insert iCCP \"%s\": unrecognized\n", params[1]);
         fprintf(stderr, "  use '<' to read a file: \"<filename\"\n");
         exit(1);
   }

   result = 1;

   if (proflen & 3)
   {
      fprintf(stderr,
         "makepng: --insert iCCP %s: profile length made a multiple of 4\n",
         params[1]);

                                                                                
                                 
         
      while (proflen & 3)
         profile[proflen++] = 0;
   }

   if (profile != NULL && proflen > 3)
   {
      png_uint_32 prof_header = png_get_uint_32(profile);

      if (prof_header != proflen)
      {
         fprintf(stderr, "--insert iCCP %s: profile length field wrong:\n",
            params[1]);
         fprintf(stderr, "  actual %lu, recorded value %lu (corrected)\n",
            (unsigned long)proflen, (unsigned long)prof_header);
         png_save_uint_32(profile, proflen);
      }
   }

   if (result && profile != NULL && proflen >=4)
      png_set_iCCP(png_ptr, info_ptr, params[0], PNG_COMPRESSION_TYPE_BASE,
         profile, proflen);

   if (profile)
      free(profile);

   if (!result)
      exit(1);
}

static void
clear_text(png_text *text, png_charp keyword)
{
   text->compression = -1;  
   text->key = keyword;
   text->text = NULL;
   text->text_length = 0;  
   text->itxt_length = 0;  
   text->lang = NULL;
   text->lang_key = NULL;
}

static void
set_text(png_structp png_ptr, png_infop info_ptr, png_textp text,
   png_charp param)
{
   switch (param[0])
   {
      case '<':
         {
            png_bytep file = NULL;

            text->text_length = load_file(param+1, &file);
            text->text = (png_charp)file;
         }
         break;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
         {
            png_bytep data = NULL;
            size_t fake_len = load_fake(param, &data);

            if (fake_len > 0)  
            {
               text->text_length = fake_len;
               text->text = (png_charp)data;
               break;
            }
         }

      default:
         text->text = param;
         break;
   }

   png_set_text(png_ptr, info_ptr, text, 1);

   if (text->text != param)
      free(text->text);
}

static void
insert_tEXt(png_structp png_ptr, png_infop info_ptr, int nparams,
   png_charpp params)
{
   png_text text;

   check_param_count(nparams, 2);
   clear_text(&text, params[0]);
   set_text(png_ptr, info_ptr, &text, params[1]);
}

static void
insert_zTXt(png_structp png_ptr, png_infop info_ptr, int nparams,
   png_charpp params)
{
   png_text text;

   check_param_count(nparams, 2);
   clear_text(&text, params[0]);
   text.compression = 0;  
   set_text(png_ptr, info_ptr, &text, params[1]);
}

static void
insert_iTXt(png_structp png_ptr, png_infop info_ptr, int nparams,
   png_charpp params)
{
   png_text text;

   check_param_count(nparams, 4);
   clear_text(&text, params[0]);
   text.compression = 2;  
   text.lang = params[1]; 
   text.lang_key = params[2];  
   set_text(png_ptr, info_ptr, &text, params[3]);
}

static void
insert_hIST(png_structp png_ptr, png_infop info_ptr, int nparams,
      png_charpp params)
{
   int i;
   png_uint_16 freq[256];

                                                                               
                                                    
      
   memset(freq, 0, sizeof freq);
   for (i=0; i<nparams; ++i)
   {
      char *endptr = NULL;
      unsigned long int l = strtoul(params[i], &endptr, 0 );

      if (params[i][0] && *endptr == 0 && l <= 65535)
         freq[i] = (png_uint_16)l;

      else
      {
         fprintf(stderr, "hIST[%d]: %s: invalid frequency\n", i, params[i]);
         exit(1);
      }
   }

   png_set_hIST(png_ptr, info_ptr, freq);
}

static png_byte
bval(png_const_structrp png_ptr, png_charp param, unsigned int maxval)
{
   char *endptr = NULL;
   unsigned long int l = strtoul(param, &endptr, 0 );

   if (param[0] && *endptr == 0 && l <= maxval)
      return (png_byte)l;

   else
      png_error(png_ptr, "sBIT: invalid sBIT value");
}

static void
insert_sBIT(png_structp png_ptr, png_infop info_ptr, int nparams,
      png_charpp params)
{
   int ct = png_get_color_type(png_ptr, info_ptr);
   int c = (ct & PNG_COLOR_MASK_COLOR ? 3 : 1) +
      (ct & PNG_COLOR_MASK_ALPHA ? 1 : 0);
   unsigned int maxval =
      ct & PNG_COLOR_MASK_PALETTE ? 8U : png_get_bit_depth(png_ptr, info_ptr);
   png_color_8 sBIT;

   if (nparams != c)
      png_error(png_ptr, "sBIT: incorrect parameter count");

   if (ct & PNG_COLOR_MASK_COLOR)
   {
      sBIT.red = bval(png_ptr, params[0], maxval);
      sBIT.green = bval(png_ptr, params[1], maxval);
      sBIT.blue = bval(png_ptr, params[2], maxval);
      sBIT.gray = 42;
   }

   else
   {
      sBIT.red = sBIT.green = sBIT.blue = 42;
      sBIT.gray = bval(png_ptr, params[0], maxval);
   }

   if (ct & PNG_COLOR_MASK_ALPHA)
      sBIT.alpha = bval(png_ptr, params[nparams-1], maxval);

   else
      sBIT.alpha = 42;

   png_set_sBIT(png_ptr, info_ptr, &sBIT);
}

#if 0
static void
insert_sPLT(png_structp png_ptr, png_infop info_ptr, int nparams, png_charpp params)
{
   fprintf(stderr, "insert sPLT: NYI\n");
}
#endif

static int
find_parameters(png_const_charp what, png_charp param, png_charp *list,
   int nparams)
{
                                                                           
                                                                   
      
   int i;
   for (i=0; *param && i<nparams; ++i)
   {
      list[i] = param;
      while (*++param) if (*param == '\n' || *param == ':')
      {
         *param++ = 0;  
         break;         
      }
   }

   if (*param)
   {
      fprintf(stderr, "--insert %s: too many parameters (%s)\n", what, param);
      exit(1);
   }

   list[i] = NULL;  
   return i;  
}

static void
bad_parameter_count(png_const_charp what, int nparams)
{
   fprintf(stderr, "--insert %s: bad parameter count %d\n", what, nparams);
   exit(1);
}

static chunk_insert *
make_insert(png_const_charp what,
   void (*insert)(png_structp, png_infop, int, png_charpp),
   int nparams, png_charpp list)
{
   int i;
   chunk_insert *cip;

   cip = malloc(offsetof(chunk_insert,parameters) +
      nparams * sizeof (png_charp));

   if (cip == NULL)
   {
      fprintf(stderr, "--insert %s: out of memory allocating %d parameters\n",
         what, nparams);
      exit(1);
   }

   cip->next = NULL;
   cip->insert = insert;
   cip->nparams = nparams;
   for (i=0; i<nparams; ++i)
      cip->parameters[i] = list[i];

   return cip;
}

static chunk_insert *
find_insert(png_const_charp what, png_charp param)
{
   png_uint_32 chunk = 0;
   png_charp parameter_list[1024];
   int i, nparams;

    
   for (i=0; i<4; ++i)
   {
      char ch = what[i];

      if ((ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122))
         chunk = (chunk << 8) + what[i];

      else
         break;
   }

   if (i < 4 || what[4] != 0)
   {
      fprintf(stderr, "makepng --insert \"%s\": invalid chunk name\n", what);
      exit(1);
   }

    
   nparams = find_parameters(what, param, parameter_list, 1024);

#  define CHUNK(a,b,c,d) (((a)<<24)+((b)<<16)+((c)<<8)+(d))

   switch (chunk)
   {
      case CHUNK(105,67,67,80):   
         if (nparams == 2)
            return make_insert(what, insert_iCCP, nparams, parameter_list);
         break;

      case CHUNK(116,69,88,116):  
         if (nparams == 2)
            return make_insert(what, insert_tEXt, nparams, parameter_list);
         break;

      case CHUNK(122,84,88,116):  
         if (nparams == 2)
            return make_insert(what, insert_zTXt, nparams, parameter_list);
         break;

      case CHUNK(105,84,88,116):  
         if (nparams == 4)
            return make_insert(what, insert_iTXt, nparams, parameter_list);
         break;

      case CHUNK(104,73,83,84):   
         if (nparams <= 256)
            return make_insert(what, insert_hIST, nparams, parameter_list);
         break;

      case CHUNK(115,66,73,84):  
         if (nparams <= 4)
            return make_insert(what, insert_sBIT, nparams, parameter_list);
         break;

#if 0
      case CHUNK(115,80,76,84):   
         return make_insert(what, insert_sPLT, nparams, parameter_list);
#endif

      default:
         fprintf(stderr, "makepng --insert \"%s\": unrecognized chunk name\n",
            what);
         exit(1);
   }

   bad_parameter_count(what, nparams);
   return NULL;
}

                                                                             
                                              
   
static png_charp
strstash(png_const_charp foo)
{
                                                                             
             
      
   if (foo != NULL)
   {
      png_charp bar = malloc(strlen(foo)+1);
      return strcpy(bar, foo);
   }

   return NULL;
}

static png_charp
strstash_list(const png_const_charp *text)
{
   size_t foo = 0;
   png_charp result, bar;
   const png_const_charp *line = text;

   while (*line != NULL)
      foo += strlen(*line++);

   result = bar = malloc(foo+1);

   line = text;
   while (*line != NULL)
   {
      foo = strlen(*line);
      memcpy(bar, *line++, foo);
      bar += foo;
   }

   *bar = 0;
   return result;
}

/* These are used to insert Copyright and Licence fields, they allow the text to
 * have \n unlike the --insert option.
 */
static chunk_insert *
add_tEXt(const char *key, const png_const_charp *text)
{
   static char what[5] = { 116, 69, 88, 116, 0 };
   png_charp parameter_list[3];

   parameter_list[0] = strstash(key);
   parameter_list[1] = strstash_list(text);
   parameter_list[2] = NULL;

   return make_insert(what, insert_tEXt, 2, parameter_list);
}

static chunk_insert *
add_iTXt(const char *key, const char *language, const char *language_key,
      const png_const_charp *text)
{
   static char what[5] = { 105, 84, 88, 116, 0 };
   png_charp parameter_list[5];

   parameter_list[0] = strstash(key);
   parameter_list[1] = strstash(language);
   parameter_list[2] = strstash(language_key);
   parameter_list[3] = strstash_list(text);
   parameter_list[4] = NULL;

   return make_insert(what, insert_iTXt, 4, parameter_list);
}

                                                                              
                                                                               
                
   
static void
parse_color(char *arg, unsigned int *colors)
{
   unsigned int ncolors = 0;

   while (*arg && ncolors < 4)
   {
      char *ep = arg;

      unsigned long ul = strtoul(arg, &ep, 0);

      if (ul > 65535)
      {
         fprintf(stderr, "makepng --color=...'%s': too big\n", arg);
         exit(1);
      }

      if (ep == arg)
      {
         fprintf(stderr, "makepng --color=...'%s': not a valid color\n", arg);
         exit(1);
      }

      if (*ep) ++ep;  
      arg = ep;

      colors[++ncolors] = (unsigned int)ul;  
   }

   if (*arg)
   {
      fprintf(stderr, "makepng --color=...'%s': too many values\n", arg);
      exit(1);
   }

   *colors = ncolors;
}

int
main(int argc, char **argv)
{
   FILE *fp = stdout;
   const char *file_name = NULL;
   int color_type = 8;  
   int bit_depth = 32;  
   int small = 0;  
   int tRNS = 0;  
   unsigned int colors[5];
   unsigned int filters = PNG_ALL_FILTERS;
   png_fixed_point gamma = 0;  
   chunk_insert *head_insert = NULL;
   chunk_insert **insert_ptr = &head_insert;

   memset(colors, 0, sizeof colors);

   while (--argc > 0)
   {
      char *arg = *++argv;

      if (strcmp(arg, "--small") == 0)
      {
         small = 1;
         continue;
      }

      if (strcmp(arg, "--tRNS") == 0)
      {
         tRNS = 1;
         continue;
      }

      if (strcmp(arg, "--sRGB") == 0)
      {
         gamma = PNG_DEFAULT_sRGB;
         continue;
      }

      if (strcmp(arg, "--linear") == 0)
      {
         gamma = PNG_FP_1;
         continue;
      }

      if (strcmp(arg, "--1.8") == 0)
      {
         gamma = PNG_GAMMA_MAC_18;
         continue;
      }

      if (strcmp(arg, "--nofilters") == 0)
      {
         filters = PNG_FILTER_NONE;
         continue;
      }

      if (strncmp(arg, "--color=", 8) == 0)
      {
          parse_color(arg+8, colors);
          continue;
      }

      if (argc >= 3 && strcmp(arg, "--insert") == 0)
      {
         png_const_charp what = *++argv;
         png_charp param = *++argv;
         chunk_insert *new_insert;

         argc -= 2;

         new_insert = find_insert(what, param);

         if (new_insert != NULL)
         {
            *insert_ptr = new_insert;
            insert_ptr = &new_insert->next;
         }

         continue;
      }

      if (arg[0] == '-')
      {
         fprintf(stderr, "makepng: %s: invalid option\n", arg);
         exit(1);
      }

      if (strcmp(arg, "palette") == 0)
      {
         color_type = PNG_COLOR_TYPE_PALETTE;
         continue;
      }

      if (strncmp(arg, "gray", 4) == 0)
      {
         if (arg[4] == 0)
         {
            color_type = PNG_COLOR_TYPE_GRAY;
            continue;
         }

         else if (strcmp(arg+4, "a") == 0 ||
            strcmp(arg+4, "alpha") == 0 ||
            strcmp(arg+4, "-alpha") == 0)
         {
            color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
            continue;
         }
      }

      if (strncmp(arg, "rgb", 3) == 0)
      {
         if (arg[3] == 0)
         {
            color_type = PNG_COLOR_TYPE_RGB;
            continue;
         }

         else if (strcmp(arg+3, "a") == 0 ||
            strcmp(arg+3, "alpha") == 0 ||
            strcmp(arg+3, "-alpha") == 0)
         {
            color_type = PNG_COLOR_TYPE_RGB_ALPHA;
            continue;
         }
      }

      if (color_type == 8 && isdigit(arg[0]))
      {
         color_type = atoi(arg);
         if (color_type < 0 || color_type > 6 || color_type == 1 ||
            color_type == 5)
         {
            fprintf(stderr, "makepng: %s: not a valid color type\n", arg);
            exit(1);
         }

         continue;
      }

      if (bit_depth == 32 && isdigit(arg[0]))
      {
         bit_depth = atoi(arg);
         if (bit_depth <= 0 || bit_depth > 16 ||
            (bit_depth & -bit_depth) != bit_depth)
         {
            fprintf(stderr, "makepng: %s: not a valid bit depth\n", arg);
            exit(1);
         }

         continue;
      }

      if (argc == 1)  
      {
         fp = fopen(arg, "wb");
         if (fp == NULL)
         {
            fprintf(stderr, "%s: %s: could not open\n", arg, strerror(errno));
            exit(1);
         }

         file_name = arg;
         continue;
      }

      fprintf(stderr, "makepng: %s: unknown argument\n", arg);
      exit(1);
   }  

   if (color_type == 8 || bit_depth == 32)
   {
      fprintf(stderr, "usage: makepng [--small] [--sRGB|--linear|--1.8] "
         "[--color=...] color-type bit-depth [file-name]\n"
         "  Make a test PNG file, by default writes to stdout.\n"
         "  Other options are available, UTSL.\n");
      exit(1);
   }

    
   {
      unsigned int lim = (color_type == PNG_COLOR_TYPE_PALETTE ? 255U :
         (1U<<bit_depth)-1);
      unsigned int i;

      for (i=1; i<=colors[0]; ++i)
         if (colors[i] > lim)
         {
            fprintf(stderr, "makepng: --color=...: %u out of range [0..%u]\n",
               colors[i], lim);
            exit(1);
         }
   }

                                                                                
                     
      
   if (small && colors[0] != 0)
   {
      fprintf(stderr, "makepng: --color --small: only one at a time!\n");
      exit(1);
   }

                                                                           
                       
      
   if (filters == PNG_ALL_FILTERS && !small )
   {
      if ((color_type & PNG_COLOR_MASK_PALETTE) != 0 || bit_depth < 8)
         filters = PNG_FILTER_NONE;

      else if (color_type & PNG_COLOR_MASK_COLOR)  
      {
         if (bit_depth == 8)
            filters &= ~(PNG_FILTER_NONE | PNG_FILTER_AVG);

         else
            filters = PNG_FILTER_SUB | PNG_FILTER_PAETH;
      }

      else  
         filters &= ~PNG_FILTER_NONE;
   }

   /* Insert standard copyright and licence text. */
   {
      static png_const_charp copyright[] =
      {
         COPYRIGHT,  
         NULL
      };
      static png_const_charp licensing[] =
      {
         IMAGE_LICENSING,  
         NULL
      };

      chunk_insert *new_insert;

      new_insert = add_tEXt("Copyright", copyright);
      if (new_insert != NULL)
      {
         *insert_ptr = new_insert;
         insert_ptr = &new_insert->next;
      }

      new_insert = add_iTXt("Licensing", "en", NULL, licensing);
      if (new_insert != NULL)
      {
         *insert_ptr = new_insert;
         insert_ptr = &new_insert->next;
      }
   }

   {
      int ret = write_png(&file_name, fp, color_type, bit_depth, gamma,
         head_insert, filters, colors, small, tRNS);

      if (ret != 0 && file_name != NULL)
         remove(file_name);

      return ret;
   }
}
