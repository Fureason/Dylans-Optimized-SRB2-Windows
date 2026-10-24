/*-
 * pngstest.c
 *
 * Last changed in libpng 1.6.31 [July 27, 2017]
 * Copyright (c) 2013-2017 John Cunningham Bowler
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * Test for the PNG 'simplified' APIs.
 */
#define _ISOC90_SOURCE 1
#define MALLOC_CHECK_ 2 

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#if defined(HAVE_CONFIG_H) && !defined(PNG_NO_CONFIG_H)
#  include <config.h>
#endif

                                                                              
                                 
   
#ifdef PNG_FREESTANDING_TESTS
#  include <png.h>
#else
#  include "../../png.h"
#endif

                                                                                
                                                                                
   
#if PNG_LIBPNG_VER >= 10601 && defined(HAVE_CONFIG_H)
#  define SKIP 77
#else
#  define SKIP 0
#endif

#ifdef PNG_SIMPLIFIED_READ_SUPPORTED  
#include "../tools/sRGB.h"

               
  
                                                                               
                                                                            
                                                                          
             
   
#define ALLOW_UNUSED_GPC 0
                                                                               
                                                                             
                                                                               
                                                                              
      

 
#ifdef __cplusplus
#  define voidcast(type, value) static_cast<type>(value)
#  define aligncastconst(type, value) \
      static_cast<type>(static_cast<const void*>(value))
#else
#  define voidcast(type, value) (value)
#  define aligncastconst(type, value) ((const void*)(value))
#endif  

                                                                            
                                                                               
                            
   
static char tmpf[23] = "TMP";

                                                                         
                                                                        
                                                                          
                                                                            
                                  
   
static void
make_random_bytes(png_uint_32* seed, void* pv, size_t size)
{
   png_uint_32 u0 = seed[0], u1 = seed[1];
   png_bytep bytes = voidcast(png_bytep, pv);

                                                                             
                                                               
      
   size_t i;
   for (i=0; i<size; ++i)
   {
       
      png_uint_32 u = ((u0 >> (20-8)) ^ ((u1 << 7) | (u0 >> (32-7)))) & 0xff;
      u1 <<= 8;
      u1 |= u0 >> 24;
      u0 <<= 8;
      u0 |= u;
      *bytes++ = (png_byte)u;
   }

   seed[0] = u0;
   seed[1] = u1;
}

static png_uint_32 color_seed[2];

static void
reseed(void)
{
   color_seed[0] = 0x12345678U;
   color_seed[1] = 0x9abcdefU;
}

static void
random_color(png_colorp color)
{
   make_random_bytes(color_seed, color, sizeof *color);
}

                                                                               
                                                     
   
static double
closestinteger(double x)
{
   return floor(x + .5);
}

 
static png_byte
u8d(double d)
{
   d = closestinteger(d);
   return (png_byte)d;
}

static png_uint_16
u16d(double d)
{
   d = closestinteger(d);
   return (png_uint_16)d;
}

                                                                           
                                                                                
   
static double sRGB_to_d[256];
static double g22_to_d[256];

static void
init_sRGB_to_d(void)
{
   int i;

   sRGB_to_d[0] = 0;
   for (i=1; i<255; ++i)
      sRGB_to_d[i] = linear_from_sRGB(i/255.);
   sRGB_to_d[255] = 1;

   g22_to_d[0] = 0;
   for (i=1; i<255; ++i)
      g22_to_d[i] = pow(i/255., 1/.45455);
   g22_to_d[255] = 1;
}

static png_byte
sRGB(double linear  )
{
   return u8d(255 * sRGB_from_linear(linear));
}

static png_byte
isRGB(int fixed_linear)
{
   return sRGB(fixed_linear / 65535.);
}

#if 0  
static png_byte
unpremultiply(int component, int alpha)
{
   if (alpha <= component)
      return 255;  

   else if (alpha >= 65535)
      return isRGB(component);

   else
      return sRGB((double)component / alpha);
}
#endif

static png_uint_16
ilinear(int fixed_srgb)
{
   return u16d(65535 * sRGB_to_d[fixed_srgb]);
}

static png_uint_16
ilineara(int fixed_srgb, int alpha)
{
   return u16d((257 * alpha) * sRGB_to_d[fixed_srgb]);
}

static png_uint_16
ilinear_g22(int fixed_srgb)
{
   return u16d(65535 * g22_to_d[fixed_srgb]);
}

#if ALLOW_UNUSED_GPC
static png_uint_16
ilineara_g22(int fixed_srgb, int alpha)
{
   return u16d((257 * alpha) * g22_to_d[fixed_srgb]);
}
#endif

static double
YfromRGBint(int ir, int ig, int ib)
{
   double r = ir;
   double g = ig;
   double b = ib;
   return YfromRGB(r, g, b);
}

#if 0  
                                                                            
                                                                                
   
static int
power_law_error8(int value)
{
   if (value > 0 && value < 255)
   {
      double vd = value / 255.;
      double e = fabs(
         pow(sRGB_to_d[value], 1/2.2) - sRGB_from_linear(pow(vd, 2.2)));

       
      e = 1+floor(255 * e);
      return (int)e;
   }

   return 0;
}

static int error_in_sRGB_roundtrip = 56;  
static int
power_law_error16(int value)
{
   if (value > 0 && value < 65535)
   {
                                                                       
                                          
         
      double vd = value / 65535.;
      double e = fabs(
         pow(sRGB_from_linear(vd), 2.2) - linear_from_sRGB(pow(vd, 1/2.2)));

       
      e = error_in_sRGB_roundtrip+floor(65535 * e);
      return (int)e;
   }

   return 0;
}

static int
compare_8bit(int v1, int v2, int error_limit, int multiple_algorithms)
{
   int e = abs(v1-v2);
   int ev1, ev2;

   if (e <= error_limit)
      return 1;

   if (!multiple_algorithms)
      return 0;

   ev1 = power_law_error8(v1);
   if (e <= ev1)
      return 1;

   ev2 = power_law_error8(v2);
   if (e <= ev2)
      return 1;

   return 0;
}

static int
compare_16bit(int v1, int v2, int error_limit, int multiple_algorithms)
{
   int e = abs(v1-v2);
   int ev1, ev2;

   if (e <= error_limit)
      return 1;

                                                                        
                                                                               
                                           
      
   if (!multiple_algorithms)
      return 0;

   ev1 = power_law_error16(v1);
   if (e <= ev1)
      return 1;

   ev2 = power_law_error16(v2);
   if (e <= ev2)
      return 1;

   return 0;
}
#endif  

#define USE_FILE 1        
#define USE_STDIO 2       
#define STRICT 4          
#define VERBOSE 8
#define KEEP_TMPFILES 16  
#define KEEP_GOING 32
#define ACCUMULATE 64
#define FAST_WRITE 128
#define sRGB_16BIT 256
#define NO_RESEED  512    
#define GBG_ERROR 1024                                                    
                                              

static void
print_opts(png_uint_32 opts)
{
   if (opts & USE_FILE)
      printf(" --file");
   if (opts & USE_STDIO)
      printf(" --stdio");
   if (!(opts & STRICT))
      printf(" --nostrict");
   if (opts & VERBOSE)
      printf(" --verbose");
   if (opts & KEEP_TMPFILES)
      printf(" --preserve");
   if (opts & KEEP_GOING)
      printf(" --keep-going");
   if (opts & ACCUMULATE)
      printf(" --accumulate");
   if (!(opts & FAST_WRITE))  
      printf(" --slow");
   if (opts & sRGB_16BIT)
      printf(" --sRGB-16bit");
   if (opts & NO_RESEED)
      printf(" --noreseed");
#if PNG_LIBPNG_VER < 10700  
   if (opts & GBG_ERROR)
      printf(" --fault-gbg-warning");
#endif
}

#define FORMAT_NO_CHANGE 0x80000000  

                                                                                
            
   
#define FORMAT_COUNT 64
#define FORMAT_MASK 0x3f
static const char * const format_names[FORMAT_COUNT] =
{
   "sRGB-gray",
   "sRGB-gray+alpha",
   "sRGB-rgb",
   "sRGB-rgb+alpha",
   "linear-gray",
   "linear-gray+alpha",
   "linear-rgb",
   "linear-rgb+alpha",

   "color-mapped-sRGB-gray",
   "color-mapped-sRGB-gray+alpha",
   "color-mapped-sRGB-rgb",
   "color-mapped-sRGB-rgb+alpha",
   "color-mapped-linear-gray",
   "color-mapped-linear-gray+alpha",
   "color-mapped-linear-rgb",
   "color-mapped-linear-rgb+alpha",

   "sRGB-gray",
   "sRGB-gray+alpha",
   "sRGB-bgr",
   "sRGB-bgr+alpha",
   "linear-gray",
   "linear-gray+alpha",
   "linear-bgr",
   "linear-bgr+alpha",

   "color-mapped-sRGB-gray",
   "color-mapped-sRGB-gray+alpha",
   "color-mapped-sRGB-bgr",
   "color-mapped-sRGB-bgr+alpha",
   "color-mapped-linear-gray",
   "color-mapped-linear-gray+alpha",
   "color-mapped-linear-bgr",
   "color-mapped-linear-bgr+alpha",

   "sRGB-gray",
   "alpha+sRGB-gray",
   "sRGB-rgb",
   "alpha+sRGB-rgb",
   "linear-gray",
   "alpha+linear-gray",
   "linear-rgb",
   "alpha+linear-rgb",

   "color-mapped-sRGB-gray",
   "color-mapped-alpha+sRGB-gray",
   "color-mapped-sRGB-rgb",
   "color-mapped-alpha+sRGB-rgb",
   "color-mapped-linear-gray",
   "color-mapped-alpha+linear-gray",
   "color-mapped-linear-rgb",
   "color-mapped-alpha+linear-rgb",

   "sRGB-gray",
   "alpha+sRGB-gray",
   "sRGB-bgr",
   "alpha+sRGB-bgr",
   "linear-gray",
   "alpha+linear-gray",
   "linear-bgr",
   "alpha+linear-bgr",

   "color-mapped-sRGB-gray",
   "color-mapped-alpha+sRGB-gray",
   "color-mapped-sRGB-bgr",
   "color-mapped-alpha+sRGB-bgr",
   "color-mapped-linear-gray",
   "color-mapped-alpha+linear-gray",
   "color-mapped-linear-bgr",
   "color-mapped-alpha+linear-bgr",
};

 
static png_uint_32
formatof(const char *arg)
{
   char *ep;
   unsigned long format = strtoul(arg, &ep, 0);

   if (ep > arg && *ep == 0 && format < FORMAT_COUNT)
      return (png_uint_32)format;

   else for (format=0; format < FORMAT_COUNT; ++format)
   {
      if (strcmp(format_names[format], arg) == 0)
         return (png_uint_32)format;
   }

   fprintf(stderr, "pngstest: format name '%s' invalid\n", arg);
   return FORMAT_COUNT;
}

 
#define FORMAT_SET_COUNT (FORMAT_COUNT / 32)
typedef struct
{
   png_uint_32 bits[FORMAT_SET_COUNT];
}
format_list;

static void format_init(format_list *pf)
{
   int i;
   for (i=0; i<FORMAT_SET_COUNT; ++i)
      pf->bits[i] = 0;  
}

#if 0  
static void format_clear(format_list *pf)
{
   int i;
   for (i=0; i<FORMAT_SET_COUNT; ++i)
      pf->bits[i] = 0;
}
#endif

static int format_is_initial(format_list *pf)
{
   int i;
   for (i=0; i<FORMAT_SET_COUNT; ++i)
      if (pf->bits[i] != 0)
         return 0;

   return 1;
}

static int format_set(format_list *pf, png_uint_32 format)
{
   if (format < FORMAT_COUNT)
      return pf->bits[format >> 5] |= ((png_uint_32)1) << (format & 31);

   return 0;
}

#if 0  
static int format_unset(format_list *pf, png_uint_32 format)
{
   if (format < FORMAT_COUNT)
      return pf->bits[format >> 5] &= ~((png_uint_32)1) << (format & 31);

   return 0;
}
#endif

static int format_isset(format_list *pf, png_uint_32 format)
{
   return format < FORMAT_COUNT &&
      (pf->bits[format >> 5] & (((png_uint_32)1) << (format & 31))) != 0;
}

static void format_default(format_list *pf, int redundant)
{
   if (redundant)
   {
      int i;

       
      for (i=0; i<FORMAT_SET_COUNT; ++i)
         pf->bits[i] = ~(png_uint_32)0;
   }

   else
   {
      png_uint_32 f;

      for (f=0; f<FORMAT_COUNT; ++f)
      {
          
#        ifdef PNG_FORMAT_BGR_SUPPORTED
             
            if ((f & PNG_FORMAT_FLAG_COLOR) == 0 &&
               (f & PNG_FORMAT_FLAG_BGR) != 0)
#        else
            if ((f & 0x10U ) != 0)
#        endif
            continue;

          
#        ifdef PNG_FORMAT_AFIRST_SUPPORTED
            if ((f & PNG_FORMAT_FLAG_ALPHA) == 0 &&
               (f & PNG_FORMAT_FLAG_AFIRST) != 0)
#        else
            if ((f & 0x20U ) != 0)
#        endif
            continue;

         format_set(pf, f);
      }
   }
}

 
                                                                            
                                                              
   
typedef struct
{
   png_image   image;
   png_uint_32 opts;
   const char *file_name;
   int         stride_extra;
   FILE       *input_file;
   png_voidp   input_memory;
   size_t      input_memory_size;
   png_bytep   buffer;
   ptrdiff_t   stride;
   size_t      bufsize;
   size_t      allocsize;
   char        tmpfile_name[32];
   png_uint_16 colormap[256*4];
}
Image;

 
static void
newimage(Image *image)
{
   memset(image, 0, sizeof *image);
}

                                                                                
   
static void
resetimage(Image *image)
{
   if (image->input_file != NULL)
      rewind(image->input_file);
}

                                                                              
           
   
static void
freebuffer(Image *image)
{
   if (image->buffer) free(image->buffer);
   image->buffer = NULL;
   image->bufsize = 0;
   image->allocsize = 0;
}

                                                                               
             
   
static void
freeimage(Image *image)
{
   freebuffer(image);
   png_image_free(&image->image);

   if (image->input_file != NULL)
   {
      fclose(image->input_file);
      image->input_file = NULL;
   }

   if (image->input_memory != NULL)
   {
      free(image->input_memory);
      image->input_memory = NULL;
      image->input_memory_size = 0;
   }

   if (image->tmpfile_name[0] != 0 && (image->opts & KEEP_TMPFILES) == 0)
   {
      (void)remove(image->tmpfile_name);
      image->tmpfile_name[0] = 0;
   }
}

                                                                                
                                                   
   
static void initimage(Image *image, png_uint_32 opts, const char *file_name,
   int stride_extra)
{
   freeimage(image);
   memset(&image->image, 0, sizeof image->image);
   image->opts = opts;
   image->file_name = file_name;
   image->stride_extra = stride_extra;
}

                                                                               
                    
   
#define BUFFER_INIT8 73
static void
allocbuffer(Image *image)
{
   size_t size = PNG_IMAGE_BUFFER_SIZE(image->image, image->stride);

   if (size+32 > image->bufsize)
   {
      freebuffer(image);
      image->buffer = voidcast(png_bytep, malloc(size+32));
      if (image->buffer == NULL)
      {
         fflush(stdout);
         fprintf(stderr,
            "simpletest: out of memory allocating %lu(+32) byte buffer\n",
            (unsigned long)size);
         exit(1);
      }
      image->bufsize = size+32;
   }

   memset(image->buffer, 95, image->bufsize);
   memset(image->buffer+16, BUFFER_INIT8, size);
   image->allocsize = size;
}

 
static int
check16(png_const_bytep bp, int b)
{
   int i = 16;

   do
      if (*bp != b) return 1;
   while (--i);

   return 0;
}

 
static void
checkbuffer(Image *image, const char *arg)
{
   if (check16(image->buffer, 95))
   {
      fflush(stdout);
      fprintf(stderr, "%s: overwrite at start of image buffer\n", arg);
      exit(1);
   }

   if (check16(image->buffer+16+image->allocsize, 95))
   {
      fflush(stdout);
      fprintf(stderr, "%s: overwrite at end of image buffer\n", arg);
      exit(1);
   }
}

 
                                                                              
   
static int
logerror(Image *image, const char *a1, const char *a2, const char *a3)
{
   fflush(stdout);
   if (image->image.warning_or_error)
      fprintf(stderr, "%s%s%s: %s\n", a1, a2, a3, image->image.message);

   else
      fprintf(stderr, "%s%s%s\n", a1, a2, a3);

   if (image->image.opaque != NULL)
   {
      fprintf(stderr, "%s: image opaque pointer non-NULL on error\n",
         image->file_name);
      png_image_free(&image->image);
   }

   return 0;
}

                                                                         
                  
   
static int
logclose(Image *image, FILE *f, const char *name, const char *operation)
{
   int e = errno;

   fclose(f);
   return logerror(image, name, operation, strerror(e));
}

                                                                               
                                       
   
static int
checkopaque(Image *image)
{
   if (image->image.opaque != NULL)
   {
      png_image_free(&image->image);
      return logerror(image, image->file_name, ": opaque not NULL", "");
   }

                                                                          
                                      
      
   else if (image->image.warning_or_error != 0 &&
            (strcmp(image->image.message,
               "libpng does not support gamma+background+rgb_to_gray") == 0 ?
                  (image->opts & GBG_ERROR) != 0 : (image->opts & STRICT) != 0))
      return logerror(image, image->file_name, (image->opts & GBG_ERROR) != 0 ?
                      " --fault-gbg-warning" : " --strict", "");

   else
      return 1;
}

 
                                                                              
                                                  
   
typedef struct
{
                                                                                
                                                                    
      
   int         r, g, b, a;
} Pixel;

typedef struct
{
                                                                            
                                                                             
                                            
      
   int ir, ig, ib;
   double dr, dg, db;  
} Background;

 
#define BASE_FORMATS\
   (PNG_FORMAT_FLAG_ALPHA|PNG_FORMAT_FLAG_COLOR|PNG_FORMAT_FLAG_LINEAR)

                                                                             
                                                            
   
static void
gp_g8(Pixel *p, png_const_voidp pb)
{
   png_const_bytep pp = voidcast(png_const_bytep, pb);

   p->r = p->g = p->b = pp[0];
   p->a = 255;
}

static void
gp_ga8(Pixel *p, png_const_voidp pb)
{
   png_const_bytep pp = voidcast(png_const_bytep, pb);

   p->r = p->g = p->b = pp[0];
   p->a = pp[1];
}

#ifdef PNG_FORMAT_AFIRST_SUPPORTED
static void
gp_ag8(Pixel *p, png_const_voidp pb)
{
   png_const_bytep pp = voidcast(png_const_bytep, pb);

   p->r = p->g = p->b = pp[1];
   p->a = pp[0];
}
#endif

static void
gp_rgb8(Pixel *p, png_const_voidp pb)
{
   png_const_bytep pp = voidcast(png_const_bytep, pb);

   p->r = pp[0];
   p->g = pp[1];
   p->b = pp[2];
   p->a = 255;
}

#ifdef PNG_FORMAT_BGR_SUPPORTED
static void
gp_bgr8(Pixel *p, png_const_voidp pb)
{
   png_const_bytep pp = voidcast(png_const_bytep, pb);

   p->r = pp[2];
   p->g = pp[1];
   p->b = pp[0];
   p->a = 255;
}
#endif

static void
gp_rgba8(Pixel *p, png_const_voidp pb)
{
   png_const_bytep pp = voidcast(png_const_bytep, pb);

   p->r = pp[0];
   p->g = pp[1];
   p->b = pp[2];
   p->a = pp[3];
}

#ifdef PNG_FORMAT_BGR_SUPPORTED
static void
gp_bgra8(Pixel *p, png_const_voidp pb)
{
   png_const_bytep pp = voidcast(png_const_bytep, pb);

   p->r = pp[2];
   p->g = pp[1];
   p->b = pp[0];
   p->a = pp[3];
}
#endif

#ifdef PNG_FORMAT_AFIRST_SUPPORTED
static void
gp_argb8(Pixel *p, png_const_voidp pb)
{
   png_const_bytep pp = voidcast(png_const_bytep, pb);

   p->r = pp[1];
   p->g = pp[2];
   p->b = pp[3];
   p->a = pp[0];
}
#endif

#if defined(PNG_FORMAT_AFIRST_SUPPORTED) && defined(PNG_FORMAT_BGR_SUPPORTED)
static void
gp_abgr8(Pixel *p, png_const_voidp pb)
{
   png_const_bytep pp = voidcast(png_const_bytep, pb);

   p->r = pp[3];
   p->g = pp[2];
   p->b = pp[1];
   p->a = pp[0];
}
#endif

static void
gp_g16(Pixel *p, png_const_voidp pb)
{
   png_const_uint_16p pp = voidcast(png_const_uint_16p, pb);

   p->r = p->g = p->b = pp[0];
   p->a = 65535;
}

static void
gp_ga16(Pixel *p, png_const_voidp pb)
{
   png_const_uint_16p pp = voidcast(png_const_uint_16p, pb);

   p->r = p->g = p->b = pp[0];
   p->a = pp[1];
}

#ifdef PNG_FORMAT_AFIRST_SUPPORTED
static void
gp_ag16(Pixel *p, png_const_voidp pb)
{
   png_const_uint_16p pp = voidcast(png_const_uint_16p, pb);

   p->r = p->g = p->b = pp[1];
   p->a = pp[0];
}
#endif

static void
gp_rgb16(Pixel *p, png_const_voidp pb)
{
   png_const_uint_16p pp = voidcast(png_const_uint_16p, pb);

   p->r = pp[0];
   p->g = pp[1];
   p->b = pp[2];
   p->a = 65535;
}

#ifdef PNG_FORMAT_BGR_SUPPORTED
static void
gp_bgr16(Pixel *p, png_const_voidp pb)
{
   png_const_uint_16p pp = voidcast(png_const_uint_16p, pb);

   p->r = pp[2];
   p->g = pp[1];
   p->b = pp[0];
   p->a = 65535;
}
#endif

static void
gp_rgba16(Pixel *p, png_const_voidp pb)
{
   png_const_uint_16p pp = voidcast(png_const_uint_16p, pb);

   p->r = pp[0];
   p->g = pp[1];
   p->b = pp[2];
   p->a = pp[3];
}

#ifdef PNG_FORMAT_BGR_SUPPORTED
static void
gp_bgra16(Pixel *p, png_const_voidp pb)
{
   png_const_uint_16p pp = voidcast(png_const_uint_16p, pb);

   p->r = pp[2];
   p->g = pp[1];
   p->b = pp[0];
   p->a = pp[3];
}
#endif

#ifdef PNG_FORMAT_AFIRST_SUPPORTED
static void
gp_argb16(Pixel *p, png_const_voidp pb)
{
   png_const_uint_16p pp = voidcast(png_const_uint_16p, pb);

   p->r = pp[1];
   p->g = pp[2];
   p->b = pp[3];
   p->a = pp[0];
}
#endif

#if defined(PNG_FORMAT_AFIRST_SUPPORTED) && defined(PNG_FORMAT_BGR_SUPPORTED)
static void
gp_abgr16(Pixel *p, png_const_voidp pb)
{
   png_const_uint_16p pp = voidcast(png_const_uint_16p, pb);

   p->r = pp[3];
   p->g = pp[2];
   p->b = pp[1];
   p->a = pp[0];
}
#endif

 
static void (*
get_pixel(png_uint_32 format))(Pixel *p, png_const_voidp pb)
{
                                                                        
                                                                            
                                     
      
   if (format & PNG_FORMAT_FLAG_LINEAR)
   {
      if (format & PNG_FORMAT_FLAG_COLOR)
      {
#        ifdef PNG_FORMAT_BGR_SUPPORTED
            if (format & PNG_FORMAT_FLAG_BGR)
            {
               if (format & PNG_FORMAT_FLAG_ALPHA)
               {
#                 ifdef PNG_FORMAT_AFIRST_SUPPORTED
                     if (format & PNG_FORMAT_FLAG_AFIRST)
                        return gp_abgr16;

                     else
#                 endif
                     return gp_bgra16;
               }

               else
                  return gp_bgr16;
            }

            else
#        endif
         {
            if (format & PNG_FORMAT_FLAG_ALPHA)
            {
#              ifdef PNG_FORMAT_AFIRST_SUPPORTED
                  if (format & PNG_FORMAT_FLAG_AFIRST)
                     return gp_argb16;

                  else
#              endif
                  return gp_rgba16;
            }

            else
               return gp_rgb16;
         }
      }

      else
      {
         if (format & PNG_FORMAT_FLAG_ALPHA)
         {
#           ifdef PNG_FORMAT_AFIRST_SUPPORTED
               if (format & PNG_FORMAT_FLAG_AFIRST)
                  return gp_ag16;

               else
#           endif
               return gp_ga16;
         }

         else
            return gp_g16;
      }
   }

   else
   {
      if (format & PNG_FORMAT_FLAG_COLOR)
      {
#        ifdef PNG_FORMAT_BGR_SUPPORTED
            if (format & PNG_FORMAT_FLAG_BGR)
            {
               if (format & PNG_FORMAT_FLAG_ALPHA)
               {
#                 ifdef PNG_FORMAT_AFIRST_SUPPORTED
                     if (format & PNG_FORMAT_FLAG_AFIRST)
                        return gp_abgr8;

                     else
#                 endif
                     return gp_bgra8;
               }

               else
                  return gp_bgr8;
            }

            else
#        endif
         {
            if (format & PNG_FORMAT_FLAG_ALPHA)
            {
#              ifdef PNG_FORMAT_AFIRST_SUPPORTED
                  if (format & PNG_FORMAT_FLAG_AFIRST)
                     return gp_argb8;

                  else
#              endif
                  return gp_rgba8;
            }

            else
               return gp_rgb8;
         }
      }

      else
      {
         if (format & PNG_FORMAT_FLAG_ALPHA)
         {
#           ifdef PNG_FORMAT_AFIRST_SUPPORTED
               if (format & PNG_FORMAT_FLAG_AFIRST)
                  return gp_ag8;

               else
#           endif
               return gp_ga8;
         }

         else
            return gp_g8;
      }
   }
}

                                                                               
                                                          
  
                                                                       
                                                              
  
                                                                             
  
                                                                         
                
  
                                                          
  
                                                    
                                                    
                                                   
                                                   
                                                   
                                                   
                                                 
                                                 
                                                 
                                                 
  
                   
                                     
                                      
                                                  
                                                                              
  
                    
                                                    
                                                                            
                                    
                                                                          
                                                                           
                                     
  
                    
                                                        
                                                                       
                                                                         
                                                                             
                                                                              
                                             
                                                         
                                               
                                                                       
                                                                               
                                                             
  
                     
                           
                                                                
                                        
   
 
static void
gpc_noop(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;
   out->r = in->r;
   out->g = in->g;
   out->b = in->b;
   out->a = in->a;
}

#if ALLOW_UNUSED_GPC
static void
gpc_nop8(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;
   if (in->a == 0)
      out->r = out->g = out->b = 255;

   else
   {
      out->r = in->r;
      out->g = in->g;
      out->b = in->b;
   }

   out->a = in->a;
}
#endif

#if ALLOW_UNUSED_GPC
static void
gpc_nop6(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;
   if (in->a == 0)
      out->r = out->g = out->b = 65535;

   else
   {
      out->r = in->r;
      out->g = in->g;
      out->b = in->b;
   }

   out->a = in->a;
}
#endif

 
 
static void
gpc_bckg(Pixel *out, const Pixel *in, const Background *back)
{
   if (in->a <= 0)
      out->r = out->g = out->b = back->ig;

   else if (in->a >= 255)
      out->r = out->g = out->b = in->g;

   else
   {
      double a = in->a / 255.;

      out->r = out->g = out->b = sRGB(sRGB_to_d[in->g] * a + back->dg * (1-a));
   }

   out->a = 255;
}

 
static void
gpc_bckc(Pixel *out, const Pixel *in, const Background *back)
{
   if (in->a <= 0)
   {
      out->r = back->ir;
      out->g = back->ig;
      out->b = back->ib;
   }

   else if (in->a >= 255)
   {
      out->r = in->r;
      out->g = in->g;
      out->b = in->b;
   }

   else
   {
      double a = in->a / 255.;

      out->r = sRGB(sRGB_to_d[in->r] * a + back->dr * (1-a));
      out->g = sRGB(sRGB_to_d[in->g] * a + back->dg * (1-a));
      out->b = sRGB(sRGB_to_d[in->b] * a + back->db * (1-a));
   }

   out->a = 255;
}

 
static void
gpc_g8(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   if (in->r == in->g && in->g == in->b)
      out->r = out->g = out->b = in->g;

   else
      out->r = out->g = out->b =
         sRGB(YfromRGB(sRGB_to_d[in->r], sRGB_to_d[in->g], sRGB_to_d[in->b]));

   out->a = in->a;
}

 
static void
gpc_g8b(Pixel *out, const Pixel *in, const Background *back)
{
   if (in->a <= 0)
      out->r = out->g = out->b = back->ig;

   else if (in->a >= 255)
   {
      if (in->r == in->g && in->g == in->b)
         out->r = out->g = out->b = in->g;

      else
         out->r = out->g = out->b = sRGB(YfromRGB(
            sRGB_to_d[in->r], sRGB_to_d[in->g], sRGB_to_d[in->b]));
   }

   else
   {
      double a = in->a/255.;

      out->r = out->g = out->b = sRGB(a * YfromRGB(sRGB_to_d[in->r],
         sRGB_to_d[in->g], sRGB_to_d[in->b]) + back->dg * (1-a));
   }

   out->a = 255;
}

 
 
static void
gpc_lin(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   out->r = ilinear(in->r);

   if (in->g == in->r)
   {
      out->g = out->r;

      if (in->b == in->r)
         out->b = out->r;

      else
         out->b = ilinear(in->b);
   }

   else
   {
      out->g = ilinear(in->g);

      if (in->b == in->r)
         out->b = out->r;

      else if (in->b == in->g)
         out->b = out->g;

      else
         out->b = ilinear(in->b);
   }

   out->a = 65535;
}

 
static void
gpc_pre(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   out->r = ilineara(in->r, in->a);

   if (in->g == in->r)
   {
      out->g = out->r;

      if (in->b == in->r)
         out->b = out->r;

      else
         out->b = ilineara(in->b, in->a);
   }

   else
   {
      out->g = ilineara(in->g, in->a);

      if (in->b == in->r)
         out->b = out->r;

      else if (in->b == in->g)
         out->b = out->g;

      else
         out->b = ilineara(in->b, in->a);
   }

   out->a = in->a * 257;
}

 
static void
gpc_preq(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   out->r = ilineara(in->r, in->a);

   if (in->g == in->r)
   {
      out->g = out->r;

      if (in->b == in->r)
         out->b = out->r;

      else
         out->b = ilineara(in->b, in->a);
   }

   else
   {
      out->g = ilineara(in->g, in->a);

      if (in->b == in->r)
         out->b = out->r;

      else if (in->b == in->g)
         out->b = out->g;

      else
         out->b = ilineara(in->b, in->a);
   }

   out->a = 65535;
}

 
static void
gpc_glin(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   if (in->r == in->g && in->g == in->b)
      out->r = out->g = out->b = ilinear(in->g);

   else
      out->r = out->g = out->b = u16d(65535 *
         YfromRGB(sRGB_to_d[in->r], sRGB_to_d[in->g], sRGB_to_d[in->b]));

   out->a = 65535;
}

 
static void
gpc_gpre(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   if (in->r == in->g && in->g == in->b)
      out->r = out->g = out->b = ilineara(in->g, in->a);

   else
      out->r = out->g = out->b = u16d(in->a * 257 *
         YfromRGB(sRGB_to_d[in->r], sRGB_to_d[in->g], sRGB_to_d[in->b]));

   out->a = 257 * in->a;
}

 
static void
gpc_gprq(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   if (in->r == in->g && in->g == in->b)
      out->r = out->g = out->b = ilineara(in->g, in->a);

   else
      out->r = out->g = out->b = u16d(in->a * 257 *
         YfromRGB(sRGB_to_d[in->r], sRGB_to_d[in->g], sRGB_to_d[in->b]));

   out->a = 65535;
}

 
 
static void
gpc_Lin(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   out->r = ilinear_g22(in->r);

   if (in->g == in->r)
   {
      out->g = out->r;

      if (in->b == in->r)
         out->b = out->r;

      else
         out->b = ilinear_g22(in->b);
   }

   else
   {
      out->g = ilinear_g22(in->g);

      if (in->b == in->r)
         out->b = out->r;

      else if (in->b == in->g)
         out->b = out->g;

      else
         out->b = ilinear_g22(in->b);
   }

   out->a = 65535;
}

#if ALLOW_UNUSED_GPC
                                                                                
   
static void
gpc_Pre(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   out->r = ilineara_g22(in->r, in->a);

   if (in->g == in->r)
   {
      out->g = out->r;

      if (in->b == in->r)
         out->b = out->r;

      else
         out->b = ilineara_g22(in->b, in->a);
   }

   else
   {
      out->g = ilineara_g22(in->g, in->a);

      if (in->b == in->r)
         out->b = out->r;

      else if (in->b == in->g)
         out->b = out->g;

      else
         out->b = ilineara_g22(in->b, in->a);
   }

   out->a = in->a * 257;
}
#endif

#if ALLOW_UNUSED_GPC
 
static void
gpc_Preq(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   out->r = ilineara_g22(in->r, in->a);

   if (in->g == in->r)
   {
      out->g = out->r;

      if (in->b == in->r)
         out->b = out->r;

      else
         out->b = ilineara_g22(in->b, in->a);
   }

   else
   {
      out->g = ilineara_g22(in->g, in->a);

      if (in->b == in->r)
         out->b = out->r;

      else if (in->b == in->g)
         out->b = out->g;

      else
         out->b = ilineara_g22(in->b, in->a);
   }

   out->a = 65535;
}
#endif

#if ALLOW_UNUSED_GPC
                                                                                
   
static void
gpc_Glin(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   if (in->r == in->g && in->g == in->b)
      out->r = out->g = out->b = ilinear_g22(in->g);

   else
      out->r = out->g = out->b = u16d(65535 *
         YfromRGB(g22_to_d[in->r], g22_to_d[in->g], g22_to_d[in->b]));

   out->a = 65535;
}
#endif

#if ALLOW_UNUSED_GPC
                                                                           
         
   
static void
gpc_Gpre(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   if (in->r == in->g && in->g == in->b)
      out->r = out->g = out->b = ilineara_g22(in->g, in->a);

   else
      out->r = out->g = out->b = u16d(in->a * 257 *
         YfromRGB(g22_to_d[in->r], g22_to_d[in->g], g22_to_d[in->b]));

   out->a = 257 * in->a;
}
#endif

#if ALLOW_UNUSED_GPC
 
static void
gpc_Gprq(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   if (in->r == in->g && in->g == in->b)
      out->r = out->g = out->b = ilineara_g22(in->g, in->a);

   else
      out->r = out->g = out->b = u16d(in->a * 257 *
         YfromRGB(g22_to_d[in->r], g22_to_d[in->g], g22_to_d[in->b]));

   out->a = 65535;
}
#endif

 
 
static void
gpc_sRGB(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   out->r = isRGB(in->r);

   if (in->g == in->r)
   {
      out->g = out->r;

      if (in->b == in->r)
         out->b = out->r;

      else
         out->b = isRGB(in->b);
   }

   else
   {
      out->g = isRGB(in->g);

      if (in->b == in->r)
         out->b = out->r;

      else if (in->b == in->g)
         out->b = out->g;

      else
         out->b = isRGB(in->b);
   }

   out->a = 255;
}

 
static void
gpc_unpg(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   if (in->a <= 128)
   {
      out->r = out->g = out->b = 255;
      out->a = 0;
   }

   else
   {
      out->r = out->g = out->b = sRGB((double)in->g / in->a);
      out->a = u8d(in->a / 257.);
   }
}

 
static void
gpc_unpc(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   if (in->a <= 128)
   {
      out->r = out->g = out->b = 255;
      out->a = 0;
   }

   else
   {
      out->r = sRGB((double)in->r / in->a);
      out->g = sRGB((double)in->g / in->a);
      out->b = sRGB((double)in->b / in->a);
      out->a = u8d(in->a / 257.);
   }
}

 
static void
gpc_b16g(Pixel *out, const Pixel *in, const Background *back)
{
   if (in->a <= 0)
      out->r = out->g = out->b = back->ig;

   else
   {
      double a = in->a/65535.;
      double a1 = 1-a;

      a /= 65535;
      out->r = out->g = out->b = sRGB(in->g * a + back->dg * a1);
   }

   out->a = 255;
}

 
static void
gpc_b16c(Pixel *out, const Pixel *in, const Background *back)
{
   if (in->a <= 0)
   {
      out->r = back->ir;
      out->g = back->ig;
      out->b = back->ib;
   }

   else
   {
      double a = in->a/65535.;
      double a1 = 1-a;

      a /= 65535;
      out->r = sRGB(in->r * a + back->dr * a1);
      out->g = sRGB(in->g * a + back->dg * a1);
      out->b = sRGB(in->b * a + back->db * a1);
   }

   out->a = 255;
}

 
static void
gpc_sG(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   out->r = out->g = out->b = sRGB(YfromRGBint(in->r, in->g, in->b)/65535);
   out->a = 255;
}

 
static void
gpc_sGp(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   if (in->a <= 128)
   {
      out->r = out->g = out->b = 255;
      out->a = 0;
   }

   else
   {
      out->r = out->g = out->b = sRGB(YfromRGBint(in->r, in->g, in->b)/in->a);
      out->a = u8d(in->a / 257.);
   }
}

 
static void
gpc_sCp(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;

   if (in->a <= 128)
   {
      out->r = out->g = out->b = 255;
      out->a = 0;
   }

   else
   {
      out->r = sRGB((double)in->r / in->a);
      out->g = sRGB((double)in->g / in->a);
      out->b = sRGB((double)in->b / in->a);
      out->a = u8d(in->a / 257.);
   }
}

 
 
static void
gpc_gb16(Pixel *out, const Pixel *in, const Background *back)
{
   if (in->a <= 0)
      out->r = out->g = out->b = back->ig;

   else if (in->a >= 65535)
      out->r = out->g = out->b = isRGB(in->g);

   else
   {
      double a = in->a / 65535.;
      double a1 = 1-a;

      a /= 65535;
      out->r = out->g = out->b = sRGB(in->g * a + back->dg * a1);
   }

   out->a = 255;
}

 
static void
gpc_cb16(Pixel *out, const Pixel *in, const Background *back)
{
   if (in->a <= 0)
   {
      out->r = back->ir;
      out->g = back->ig;
      out->b = back->ib;
   }

   else if (in->a >= 65535)
   {
      out->r = isRGB(in->r);
      out->g = isRGB(in->g);
      out->b = isRGB(in->b);
   }

   else
   {
      double a = in->a / 65535.;
      double a1 = 1-a;

      a /= 65535;
      out->r = sRGB(in->r * a + back->dr * a1);
      out->g = sRGB(in->g * a + back->dg * a1);
      out->b = sRGB(in->b * a + back->db * a1);
   }

   out->a = 255;
}

 
 
static void
gpc_A(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;
   out->r = in->r;
   out->g = in->g;
   out->b = in->b;
   out->a = 65535;
}

 
static void
gpc_g16(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;
   out->r = out->g = out->b = u16d(YfromRGBint(in->r, in->g, in->b));
   out->a = 65535;
}

 
static void
gpc_g16q(Pixel *out, const Pixel *in, const Background *back)
{
   (void)back;
   out->r = out->g = out->b = u16d(YfromRGBint(in->r, in->g, in->b));
   out->a = in->a;
}

#if ALLOW_UNUSED_GPC
 
void (* const gpc_unused[])
   (Pixel *out, const Pixel *in, const Background *back) =
{
   gpc_Pre, gpc_Preq, gpc_Glin, gpc_Gpre, gpc_Gprq, gpc_nop8, gpc_nop6
};
#endif

                                                    
                                                    
                                                   
                                                   
                                                   
                                                   
                                                 
                                                 
                                                 
                                                 
  
                                               
  
                                                                 
   
                                                                         
                          
   
#if PNG_FORMAT_FLAG_ALPHA == 1 && PNG_FORMAT_FLAG_COLOR == 2 &&\
   PNG_FORMAT_FLAG_LINEAR == 4
static void (* const gpc_fn[8 ][8 ])
   (Pixel *out, const Pixel *in, const Background *back) =
{
 
   {gpc_noop,gpc_noop,gpc_noop,gpc_noop, gpc_Lin, gpc_Lin, gpc_Lin, gpc_Lin },
   {gpc_bckg,gpc_noop,gpc_bckc,gpc_noop, gpc_preq,gpc_pre, gpc_preq,gpc_pre },
   {gpc_g8,  gpc_g8,  gpc_noop,gpc_noop, gpc_glin,gpc_glin,gpc_lin, gpc_lin },
   {gpc_g8b, gpc_g8,  gpc_bckc,gpc_noop, gpc_gprq,gpc_gpre,gpc_preq,gpc_pre },
   {gpc_sRGB,gpc_sRGB,gpc_sRGB,gpc_sRGB, gpc_noop,gpc_noop,gpc_noop,gpc_noop},
   {gpc_b16g,gpc_unpg,gpc_b16c,gpc_unpc, gpc_A,   gpc_noop,gpc_A,   gpc_noop},
   {gpc_sG,  gpc_sG,  gpc_sRGB,gpc_sRGB, gpc_g16, gpc_g16, gpc_noop,gpc_noop},
   {gpc_gb16,gpc_sGp, gpc_cb16,gpc_sCp,  gpc_g16, gpc_g16q,gpc_A,   gpc_noop}
};

                                                                                
                                                     
   
static void (* const gpc_fn_colormapped[8 ][8 ])
   (Pixel *out, const Pixel *in, const Background *back) =
{
 
   {gpc_noop,gpc_noop,gpc_noop,gpc_noop, gpc_lin, gpc_lin, gpc_lin, gpc_lin },
   {gpc_bckg,gpc_noop,gpc_bckc,gpc_noop, gpc_preq,gpc_pre, gpc_preq,gpc_pre },
   {gpc_g8,  gpc_g8,  gpc_noop,gpc_noop, gpc_glin,gpc_glin,gpc_lin, gpc_lin },
   {gpc_g8b, gpc_g8,  gpc_bckc,gpc_noop, gpc_gprq,gpc_gpre,gpc_preq,gpc_pre },
   {gpc_sRGB,gpc_sRGB,gpc_sRGB,gpc_sRGB, gpc_noop,gpc_noop,gpc_noop,gpc_noop},
   {gpc_b16g,gpc_unpg,gpc_b16c,gpc_unpc, gpc_A,   gpc_noop,gpc_A,   gpc_noop},
   {gpc_sG,  gpc_sG,  gpc_sRGB,gpc_sRGB, gpc_g16, gpc_g16, gpc_noop,gpc_noop},
   {gpc_gb16,gpc_sGp, gpc_cb16,gpc_sCp,  gpc_g16, gpc_g16q,gpc_A,   gpc_noop}
};

                                                                            
                                                                              
                                                                     
                           
  
                                                                      
                                                                                
                                                                             
                                                                             
                                                       
  
                                                                               
                                                           
  
                                                                             
                                                                              
  
                                                                         
                                                                       
                                                                       
                                                                             
                                                                            
           
  
                                                                             
                                                                       
                                                              
                         
   
#if PNG_FORMAT_FLAG_COLORMAP == 8  
#  include "pngstest-errors.h"  
#endif  
#endif  

typedef struct
{
    
   Image*       in_image;    
   const Image* out_image;   

                                                                              
                                                                           
              
      
   Background        background_color;
   const Background* background;

    
   int          in_opaque;    
   int          is_palette;   
   int          accumulate;   
   int          output_8bit;  

   void (*in_gp)(Pixel*, png_const_voidp);
   void (*out_gp)(Pixel*, png_const_voidp);

   void (*transform)(Pixel *out, const Pixel *in, const Background *back);
       

   void (*from_linear)(Pixel *out, const Pixel *in, const Background *back);
       

   png_uint_16 error[4];
                                                                             
                                
         

   png_uint_16 *error_ptr;
       
}
Transform;

 
static void
transform_from_formats(Transform *result, Image *in_image,
   const Image *out_image, png_const_colorp background, int via_linear)
{
   png_uint_32 in_format, out_format;
   png_uint_32 in_base, out_base;

   memset(result, 0, sizeof *result);

    
   result->in_image = in_image;
   result->out_image = out_image;

   in_format = in_image->image.format;
   out_format = out_image->image.format;

   if (in_format & PNG_FORMAT_FLAG_LINEAR)
      result->in_opaque = 65535;
   else
      result->in_opaque = 255;

   result->output_8bit = (out_format & PNG_FORMAT_FLAG_LINEAR) == 0;

   result->is_palette = 0;  
   result->accumulate = (in_image->opts & ACCUMULATE) != 0;

    
   result->in_gp = get_pixel(in_format);
   result->out_gp = get_pixel(out_format);

    
   in_format &= BASE_FORMATS | PNG_FORMAT_FLAG_COLORMAP;
   in_base = in_format & BASE_FORMATS;
   out_format &= BASE_FORMATS | PNG_FORMAT_FLAG_COLORMAP;
   out_base = out_format & BASE_FORMATS;

   if (via_linear)
   {
       
      if (out_format & (PNG_FORMAT_FLAG_LINEAR|PNG_FORMAT_FLAG_COLORMAP))
      {
         fprintf(stderr, "internal transform via linear error 0x%x->0x%x\n",
            in_format, out_format);
         exit(1);
      }

      result->transform = gpc_fn[in_base][out_base | PNG_FORMAT_FLAG_LINEAR];
      result->from_linear = gpc_fn[out_base | PNG_FORMAT_FLAG_LINEAR][out_base];
      result->error_ptr = gpc_error_via_linear[in_format][out_format];
   }

   else if (~in_format & out_format & PNG_FORMAT_FLAG_COLORMAP)
   {
                                                                        
                                                                                
                            
         
      result->transform = gpc_fn[in_base][out_base];
      result->from_linear = NULL;
      result->error_ptr = gpc_error_to_colormap[in_base][out_base];
   }

   else
   {
                                                                        
                                                                           
                                                                                
                                                                           
                                                                    
         
      if (in_format & out_format & PNG_FORMAT_FLAG_COLORMAP)
         result->transform = gpc_fn_colormapped[in_base][out_base];
      else
         result->transform = gpc_fn[in_base][out_base];
      result->from_linear = NULL;
      result->error_ptr = gpc_error[in_format][out_format];
   }

                                                                                
                                                                              
                                                              
      
   result->background = NULL;  

    
   if (in_format & ~out_format & PNG_FORMAT_FLAG_ALPHA)
   {
                                                                                
                                                                              
                                                                          
                                                                         
                                                                                
                                                                              
         
      result->background = &result->background_color;

      if (out_format & PNG_FORMAT_FLAG_LINEAR || via_linear)
      {
         if (out_format & PNG_FORMAT_FLAG_COLORMAP)
         {
            result->background_color.ir =
               result->background_color.ig =
               result->background_color.ib = 0;
            result->background_color.dr =
               result->background_color.dg =
               result->background_color.db = 0;
         }

         else
         {
            result->background_color.ir =
               result->background_color.ig =
               result->background_color.ib = BUFFER_INIT8 * 257;
            result->background_color.dr =
               result->background_color.dg =
               result->background_color.db = 0;
         }
      }

      else  
      {
         if (background != NULL)
         {
            if (out_format & PNG_FORMAT_FLAG_COLOR)
            {
               result->background_color.ir = background->red;
               result->background_color.ig = background->green;
               result->background_color.ib = background->blue;
                                                                                
                                 
                  
               result->background_color.dr = sRGB_to_d[background->red];
               result->background_color.dg = sRGB_to_d[background->green];
               result->background_color.db = sRGB_to_d[background->blue];
            }

            else  
            {
               result->background_color.ir =
                  result->background_color.ig =
                  result->background_color.ib = background->green;
                                                                                
                                 
                  
               result->background_color.dr =
                  result->background_color.dg =
                  result->background_color.db = sRGB_to_d[background->green];
            }
         }

         else if ((out_format & PNG_FORMAT_FLAG_COLORMAP) == 0)
         {
            result->background_color.ir =
               result->background_color.ig =
               result->background_color.ib = BUFFER_INIT8;
                                                                             
                              
               
            result->background_color.dr =
               result->background_color.dg =
               result->background_color.db = sRGB_to_d[BUFFER_INIT8];
         }

                                                                         
                                                                            
                                                     
            
         else
            result->background = NULL;
      }
   }

   if (result->background == NULL)
   {
      result->background_color.ir =
         result->background_color.ig =
         result->background_color.ib = -1;  
      result->background_color.dr =
         result->background_color.dg =
         result->background_color.db = 1E30;  
   }


    
   result->error[0] = result->error_ptr[0];
   result->error[1] = result->error_ptr[1];
   result->error[2] = result->error_ptr[2];
   result->error[3] = result->error_ptr[3];
}


                      
  
                    
                                                   
                                                             
                                               
                                                                        
                                                             
                                                  
                                    
  
                                                                               
                                                                              
  
                                                                                
                                                                               
                                                                              
                                                            
  
                                                                       
                                                                              
                                        
  
                                                                                
                                                                             
                                                                  
   
#if 0
static int error_in_libpng_gamma;
static int error_via_linear[256];  

static void
init_error_via_linear(void)
{
   int alpha;

   error_via_linear[0] = 255;  

   for (alpha=1; alpha<=255; ++alpha)
   {
                                                                              
                                                                
                                                                            
                                                               
        
                                                                           
                                                                         
        
                                                    
         
      double v = floor(255*pow(.5  / alpha, 1/2.2)+.1);

      error_via_linear[alpha] = (int)v;
   }

                                                                                
                      
      
   error_in_libpng_gamma = 14;
}
#endif

static void
print_pixel(char string[64], const Pixel *pixel, png_uint_32 format)
{
   switch (format & (PNG_FORMAT_FLAG_ALPHA|PNG_FORMAT_FLAG_COLOR))
   {
      case 0:
         sprintf(string, "%s(%d)", format_names[format], pixel->g);
         break;

      case PNG_FORMAT_FLAG_ALPHA:
         sprintf(string, "%s(%d,%d)", format_names[format], pixel->g,
            pixel->a);
         break;

      case PNG_FORMAT_FLAG_COLOR:
         sprintf(string, "%s(%d,%d,%d)", format_names[format],
            pixel->r, pixel->g, pixel->b);
         break;

      case PNG_FORMAT_FLAG_COLOR|PNG_FORMAT_FLAG_ALPHA:
         sprintf(string, "%s(%d,%d,%d,%d)", format_names[format],
            pixel->r, pixel->g, pixel->b, pixel->a);
         break;

      default:
         sprintf(string, "invalid-format");
         break;
   }
}

static int
logpixel(const Transform *transform, png_uint_32 x, png_uint_32 y,
   const Pixel *in, const Pixel *calc, const Pixel *out, const char *reason)
{
   png_uint_32 in_format = transform->in_image->image.format;
   png_uint_32 out_format = transform->out_image->image.format;

   png_uint_32 back_format = out_format & ~PNG_FORMAT_FLAG_ALPHA;
   const char *via_linear = "";

   char pixel_in[64], pixel_calc[64], pixel_out[64], pixel_loc[64];
   char background_info[100];

   print_pixel(pixel_in, in, in_format);
   print_pixel(pixel_calc, calc, out_format);
   print_pixel(pixel_out, out, out_format);

   if (transform->is_palette)
      sprintf(pixel_loc, "palette: %lu", (unsigned long)y);
   else
      sprintf(pixel_loc, "%lu,%lu", (unsigned long)x, (unsigned long)y);

   if (transform->from_linear != NULL)
   {
      via_linear = " (via linear)";
                                                                              
                                                                       
                
         
      back_format |= PNG_FORMAT_FLAG_LINEAR;
   }

   if (transform->background != NULL)
   {
      Pixel back;
      char pixel_back[64];

      back.r = transform->background->ir;
      back.g = transform->background->ig;
      back.b = transform->background->ib;
      back.a = -1;  

      print_pixel(pixel_back, &back, back_format);
      sprintf(background_info, " on background %s", pixel_back);
   }

   else
      background_info[0] = 0;

   if (transform->in_image->file_name != transform->out_image->file_name)
   {
      char error_buffer[512];
      sprintf(error_buffer,
         "(%s) %s error%s:\n %s%s ->\n       %s\n  not: %s.\n"
         "Use --preserve and examine: ", pixel_loc, reason, via_linear,
         pixel_in, background_info, pixel_out, pixel_calc);
      return logerror(transform->in_image, transform->in_image->file_name,
         error_buffer, transform->out_image->file_name);
   }

   else
   {
      char error_buffer[512];
      sprintf(error_buffer,
         "(%s) %s error%s:\n %s%s ->\n       %s\n  not: %s.\n"
         " The error happened when reading the original file with this format.",
         pixel_loc, reason, via_linear, pixel_in, background_info, pixel_out,
         pixel_calc);
      return logerror(transform->in_image, transform->in_image->file_name,
         error_buffer, "");
   }
}

static int
cmppixel(Transform *transform, png_const_voidp in, png_const_voidp out,
   png_uint_32 x, png_uint_32 y )
{
   int maxerr;
   png_const_charp errmsg;
   Pixel pixel_in, pixel_calc, pixel_out;

   transform->in_gp(&pixel_in, in);

   if (transform->from_linear == NULL)
      transform->transform(&pixel_calc, &pixel_in, transform->background);

   else
   {
      transform->transform(&pixel_out, &pixel_in, transform->background);
      transform->from_linear(&pixel_calc, &pixel_out, NULL);
   }

   transform->out_gp(&pixel_out, out);

    
   if (pixel_calc.a == pixel_out.a && pixel_calc.r == pixel_out.r &&
      pixel_calc.g == pixel_out.g && pixel_calc.b == pixel_out.b)
      return 1;

                                                                             
                                                                             
                                                     
      
   if (transform->output_8bit && pixel_calc.a == 0 && pixel_out.a == 0)
      return 1;

                                                                                
                                                             
      
   errmsg = NULL;
   {
      int err_a = abs(pixel_calc.a-pixel_out.a);

      if (err_a > transform->error[3])
      {
          
         if (transform->accumulate)
            transform->error[3] = (png_uint_16)err_a;

         else
            errmsg = "alpha";
      }
   }

                                                                              
                                               
      
   if (errmsg == NULL && transform->output_8bit &&
      (pixel_calc.a == 0 || pixel_out.a == 0))
      return 1;

   if (errmsg == NULL)  
   {
      int err_r = abs(pixel_calc.r - pixel_out.r);
      int err_g = abs(pixel_calc.g - pixel_out.g);
      int err_b = abs(pixel_calc.b - pixel_out.b);
      int limit;

      if ((err_r | err_g | err_b) == 0)
         return 1;  

       
      if (pixel_in.a >= transform->in_opaque)
      {
         errmsg = "opaque component";
         limit = 2;  
      }

      else if (pixel_in.a > 0)
      {
         errmsg = "alpha component";
         limit = 1;  
      }

      else
      {
         errmsg = "transparent component (background)";
         limit = 0;  
      }

      maxerr = err_r;
      if (maxerr < err_g) maxerr = err_g;
      if (maxerr < err_b) maxerr = err_b;

      if (maxerr <= transform->error[limit])
         return 1;  

                                                                            
                       
         
      if (transform->accumulate)
      {
         transform->error[limit] = (png_uint_16)maxerr;
         return 1;  
      }
   }

    
   return logpixel(transform, x, y, &pixel_in, &pixel_calc, &pixel_out, errmsg);
}

static png_byte
component_loc(png_byte loc[4], png_uint_32 format)
{
                                                                      
                   
     
                                                                         
                                                                               
                                                   
     
              
                    
                      
                     
      
   png_byte channels;

   if (format & PNG_FORMAT_FLAG_COLOR)
   {
      channels = 3;

      loc[2] = 1;

#     ifdef PNG_FORMAT_BGR_SUPPORTED
         if (format & PNG_FORMAT_FLAG_BGR)
         {
            loc[1] = 2;
            loc[3] = 0;
         }

         else
#     endif
      {
         loc[1] = 0;
         loc[3] = 2;
      }
   }

   else
   {
      channels = 1;
      loc[1] = loc[2] = loc[3] = 0;
   }

   if (format & PNG_FORMAT_FLAG_ALPHA)
   {
#     ifdef PNG_FORMAT_AFIRST_SUPPORTED
         if (format & PNG_FORMAT_FLAG_AFIRST)
         {
            loc[0] = 0;
            ++loc[1];
            ++loc[2];
            ++loc[3];
         }

         else
#     endif
         loc[0] = channels;

      ++channels;
   }

   else
      loc[0] = 4;  

   return channels;
}

                                                                                
                                                           
   
static int
compare_two_images(Image *a, Image *b, int via_linear,
   png_const_colorp background)
{
   ptrdiff_t stridea = a->stride;
   ptrdiff_t strideb = b->stride;
   png_const_bytep rowa = a->buffer+16;
   png_const_bytep rowb = b->buffer+16;
   png_uint_32 width = a->image.width;
   png_uint_32 height = a->image.height;
   png_uint_32 formata = a->image.format;
   png_uint_32 formatb = b->image.format;
   unsigned int a_sample = PNG_IMAGE_SAMPLE_SIZE(formata);
   unsigned int b_sample = PNG_IMAGE_SAMPLE_SIZE(formatb);
   int alpha_added, alpha_removed;
   int bchannels;
   png_uint_32 y;
   Transform tr;
   int btoa[4]={0,0,0,0};

    
   if (width != b->image.width || height != b->image.height)
      return logerror(a, a->file_name, ": width x height changed: ",
         b->file_name);

    
   transform_from_formats(&tr, a, b, background, via_linear);

    
   if (!(formata & PNG_FORMAT_FLAG_COLORMAP) &&
      (formata & PNG_FORMAT_FLAG_LINEAR))
      stridea *= 2;

   if (!(formatb & PNG_FORMAT_FLAG_COLORMAP) &&
      (formatb & PNG_FORMAT_FLAG_LINEAR))
      strideb *= 2;

   if (stridea < 0) rowa += (height-1) * (-stridea);
   if (strideb < 0) rowb += (height-1) * (-strideb);

                                                                             
                                                                         
                                                                              
                                        
      
   if (formata & formatb & PNG_FORMAT_FLAG_COLORMAP)
   {
       
      png_const_bytep ppa, ppb;
      int match;
      png_byte in_use[256], amax = 0, bmax = 0;

      memset(in_use, 0, sizeof in_use);

      ppa = rowa;
      ppb = rowb;

                                                                               
                                                                           
                                                       
         
      for (y=0, match=1; y<height && match; ++y, ppa += stridea, ppb += strideb)
      {
         png_uint_32 x;

         for (x=0; x<width; ++x)
         {
            png_byte bval = ppb[x];
            png_byte aval = ppa[x];

            if (bval > bmax)
               bmax = bval;

            if (bval != aval)
               match = 0;

            in_use[aval] = 1;
            if (aval > amax)
               amax = aval;
         }
      }

       
      if (match)
      {
                                                                             
                                                                          
            
         png_const_bytep a_cmap = (png_const_bytep)a->colormap;
         png_const_bytep b_cmap = (png_const_bytep)b->colormap;
         int result = 1;  

          
         tr.is_palette = 1;

         for (y=0; y<256; ++y, a_cmap += a_sample, b_cmap += b_sample)
            if (in_use[y])
         {
                                                                               
                                                                                
                                                                           
                                                                               
               
            if (y >= a->image.colormap_entries)
            {
               if ((a->opts & ACCUMULATE) == 0)
               {
                  char pindex[9];
                  sprintf(pindex, "%lu[%lu]", (unsigned long)y,
                     (unsigned long)a->image.colormap_entries);
                  logerror(a, a->file_name, ": bad pixel index: ", pindex);
               }
               result = 0;
            }

            else if (y >= b->image.colormap_entries)
            {
               if ((b->opts & ACCUMULATE) == 0)
                  {
                  char pindex[9];
                  sprintf(pindex, "%lu[%lu]", (unsigned long)y,
                     (unsigned long)b->image.colormap_entries);
                  logerror(b, b->file_name, ": bad pixel index: ", pindex);
                  }
               result = 0;
            }

             
            else if (!cmppixel(&tr, a_cmap, b_cmap, 0, y))
               result = 0;
         }

          
         if (a->opts & ACCUMULATE)
         {
            tr.error_ptr[0] = tr.error[0];
            tr.error_ptr[1] = tr.error[1];
            tr.error_ptr[2] = tr.error[2];
            tr.error_ptr[3] = tr.error[3];
            result = 1;  
         }

         return result;
      }

                                                                               
                                                                             
                                                              
         
      else if ((a->opts & ACCUMULATE) == 0)
      {
#        ifdef __GNUC__
#           define BYTE_CHARS 20  
#        else
#           define BYTE_CHARS 3  
#        endif
                                           
                                                               
            
         if (amax >= a->image.colormap_entries)
         {
            char pindex[3+2*BYTE_CHARS];
            sprintf(pindex, "%d[%u]", amax,
               (png_byte) a->image.colormap_entries);
            return logerror(a, a->file_name, ": bad pixel index: ", pindex);
         }

         else if (bmax >= b->image.colormap_entries)
         {
            char pindex[3+2*BYTE_CHARS];
            sprintf(pindex, "%d[%u]", bmax,
               (png_byte) b->image.colormap_entries);
            return logerror(b, b->file_name, ": bad pixel index: ", pindex);
         }
      }
   }

                                                                           
                                                      
     
                                       
                                                                               
                                                         
                                                                           
                                                         
     
                                                                                
                           
     
                                                                          
                                        
      
   if (!(((formata ^ formatb) & PNG_FORMAT_FLAG_LINEAR) |
      (formata & (formatb ^ PNG_FORMAT_FLAG_COLOR) & PNG_FORMAT_FLAG_COLOR)))
   {
       
      png_uint_32 alpha_changed = (formata ^ formatb) & PNG_FORMAT_FLAG_ALPHA;

                                                                           
                                                                  
         
      alpha_removed = (formata & alpha_changed) != 0;

       
      alpha_added = (formatb & alpha_changed) != 0;

                                                                              
                                                                             
                                                                              
                                        
         
      {
         int i;
         png_byte aloc[4];
         png_byte bloc[4];

                                                                         
                                                                               
                                                                             
                                                                              
                                                          
            
         (void)component_loc(aloc, formata);
         bchannels = component_loc(bloc, formatb);

          
         for (i=0; i<4; ++i) if (bloc[i] < 4)
            btoa[bloc[i]] = aloc[i];  

         if (alpha_added)
            alpha_added = bloc[0];  

         else
            alpha_added = 4;  

         if (alpha_removed)
            alpha_removed = aloc[0];  

         else
            alpha_removed = 4;
      }
   }

   else
   {
                                                                               
                   
         
      bchannels = 0;
      alpha_removed = alpha_added = 4;
      btoa[3] = btoa[2] = btoa[1] = btoa[0] = 4;  
   }

   for (y=0; y<height; ++y, rowa += stridea, rowb += strideb)
   {
      png_const_bytep ppa, ppb;
      png_uint_32 x;

      for (x=0, ppa=rowa, ppb=rowb; x<width; ++x)
      {
         png_const_bytep psa, psb;

         if (formata & PNG_FORMAT_FLAG_COLORMAP)
            psa = (png_const_bytep)a->colormap + a_sample * *ppa++;
         else
            psa = ppa, ppa += a_sample;

         if (formatb & PNG_FORMAT_FLAG_COLORMAP)
            psb = (png_const_bytep)b->colormap + b_sample * *ppb++;
         else
            psb = ppb, ppb += b_sample;

          
         if (bchannels)
         {
                                                                          
                                                                     
                                                                               
                                                                             
                              
               
            if (formatb & PNG_FORMAT_FLAG_LINEAR)  
            {
               png_const_uint_16p pua = aligncastconst(png_const_uint_16p, psa);
               png_const_uint_16p pub = aligncastconst(png_const_uint_16p, psb);

               switch (bchannels)
               {
                  case 4:
                     if (pua[btoa[3]] != pub[3]) break;
                      
                  case 3:
                     if (pua[btoa[2]] != pub[2]) break;
                      
                  case 2:
                     if (pua[btoa[1]] != pub[1]) break;
                      
                  case 1:
                     if (pua[btoa[0]] != pub[0]) break;
                     if (alpha_added != 4 && pub[alpha_added] != 65535) break;
                     continue;  
                  default:
                     break;  
               }
            }

            else if (alpha_removed == 4 || psa[alpha_removed] == 255)
            {
               switch (bchannels)
               {
                  case 4:
                     if (psa[btoa[3]] != psb[3]) break;
                      
                  case 3:
                     if (psa[btoa[2]] != psb[2]) break;
                      
                  case 2:
                     if (psa[btoa[1]] != psb[1]) break;
                      
                  case 1:
                     if (psa[btoa[0]] != psb[0]) break;
                     if (alpha_added != 4 && psb[alpha_added] != 255) break;
                     continue;  
                  default:
                     break;  
               }
            }
         }

                                                                               
                  
            
         if (!cmppixel(&tr, psa, psb, x, y) && (a->opts & KEEP_GOING) == 0)
            return 0;  
      }
   }

    
   if (a->opts & ACCUMULATE)
   {
      tr.error_ptr[0] = tr.error[0];
      tr.error_ptr[1] = tr.error[1];
      tr.error_ptr[2] = tr.error[2];
      tr.error_ptr[3] = tr.error[3];
   }

   return 1;
}

                                                                           
                              
   
static int
read_file(Image *image, png_uint_32 format, png_const_colorp background)
{
   memset(&image->image, 0, sizeof image->image);
   image->image.version = PNG_IMAGE_VERSION;

   if (image->input_memory != NULL)
   {
      if (!png_image_begin_read_from_memory(&image->image, image->input_memory,
         image->input_memory_size))
         return logerror(image, "memory init: ", image->file_name, "");
   }

#  ifdef PNG_STDIO_SUPPORTED
      else if (image->input_file != NULL)
      {
         if (!png_image_begin_read_from_stdio(&image->image, image->input_file))
            return logerror(image, "stdio init: ", image->file_name, "");
      }

      else
      {
         if (!png_image_begin_read_from_file(&image->image, image->file_name))
            return logerror(image, "file init: ", image->file_name, "");
      }
#  else
      else
      {
         return logerror(image, "unsupported file/stdio init: ",
            image->file_name, "");
      }
#  endif

    
   if (image->opts & sRGB_16BIT)
      image->image.flags |= PNG_IMAGE_FLAG_16BIT_sRGB;

                                                                         
                                                                         
      
   {
      int result;
      png_uint_32 image_format;

       
      image_format = image->image.format;

      if (image->opts & VERBOSE)
      {
         printf("%s %lu x %lu %s -> %s", image->file_name,
            (unsigned long)image->image.width,
            (unsigned long)image->image.height,
            format_names[image_format & FORMAT_MASK],
            (format & FORMAT_NO_CHANGE) != 0 || image->image.format == format
            ? "no change" : format_names[format & FORMAT_MASK]);

         if (background != NULL)
            printf(" background(%d,%d,%d)\n", background->red,
               background->green, background->blue);
         else
            printf("\n");

         fflush(stdout);
      }

                                                                            
                                                                              
                                                        
         
      if ((format & FORMAT_NO_CHANGE) != 0)
      {
         if ((format & PNG_FORMAT_FLAG_COLORMAP) != 0 &&
            (image_format & PNG_FORMAT_FLAG_COLORMAP) != 0)
            format = (image_format & ~BASE_FORMATS) | (format & BASE_FORMATS);

         else
            format = image_format;
      }

      image->image.format = format;

      image->stride = PNG_IMAGE_ROW_STRIDE(image->image) + image->stride_extra;
      allocbuffer(image);

      result = png_image_finish_read(&image->image, background,
         image->buffer+16, (png_int_32)image->stride, image->colormap);

      checkbuffer(image, image->file_name);

      if (result)
         return checkopaque(image);

      else
         return logerror(image, image->file_name, ": image read failed", "");
   }
}

                                                                     
                                                                           
                                                
   
static int
read_one_file(Image *image)
{
   if (!(image->opts & USE_FILE) || (image->opts & USE_STDIO))
   {
       
      FILE *f = fopen(image->file_name, "rb");

      if (f != NULL)
      {
         if (image->opts & USE_FILE)
            image->input_file = f;

         else  
         {
            if (fseek(f, 0, SEEK_END) == 0)
            {
               long int cb = ftell(f);

               if (cb > 0)
               {
#ifndef __COVERITY__
                  if ((unsigned long int)cb <= (size_t)~(size_t)0)
#endif
                  {
                     png_bytep b = voidcast(png_bytep, malloc((size_t)cb));

                     if (b != NULL)
                     {
                        rewind(f);

                        if (fread(b, (size_t)cb, 1, f) == 1)
                        {
                           fclose(f);
                           image->input_memory_size = cb;
                           image->input_memory = b;
                        }

                        else
                        {
                           free(b);
                           return logclose(image, f, image->file_name,
                              ": read failed: ");
                        }
                     }

                     else
                        return logclose(image, f, image->file_name,
                           ": out of memory: ");
                  }

                  else
                     return logclose(image, f, image->file_name,
                        ": file too big for this architecture: ");
                                                                    
                                                                  
                                                                 
                        
               }

               else if (cb == 0)
                  return logclose(image, f, image->file_name,
                     ": zero length: ");

               else
                  return logclose(image, f, image->file_name,
                     ": tell failed: ");
            }

            else
               return logclose(image, f, image->file_name, ": seek failed: ");
         }
      }

      else
         return logerror(image, image->file_name, ": open failed: ",
            strerror(errno));
   }

   return read_file(image, FORMAT_NO_CHANGE, NULL);
}

#ifdef PNG_SIMPLIFIED_WRITE_SUPPORTED
static int
write_one_file(Image *output, Image *image, int convert_to_8bit)
{
   if (image->opts & FAST_WRITE)
      image->image.flags |= PNG_IMAGE_FLAG_FAST;

   if (image->opts & USE_STDIO)
   {
#ifdef PNG_SIMPLIFIED_WRITE_STDIO_SUPPORTED
#ifndef __COVERITY__
      FILE *f = tmpfile();
#else
                                                                     
                                     
        
                                                                        
                                                                             
                                                                     
                                                                      
        
                                                                
                                                                     
                                                                     
                                                               
                                                                       
                                                                              
         
      char tmpfile[] = "pngstest-XXXXXX";
      int filedes;
      FILE *f;
      umask(0177);
      filedes = mkstemp(tmpfile);
      if (filedes < 0)
        f = NULL;
      else
      {
        f = fdopen(filedes,"w+");
                                                                      
                                           
           
        (void) unlink(tmpfile);
      }
#endif

      if (f != NULL)
      {
         if (png_image_write_to_stdio(&image->image, f, convert_to_8bit,
            image->buffer+16, (png_int_32)image->stride, image->colormap))
         {
            if (fflush(f) == 0)
            {
               rewind(f);
               initimage(output, image->opts, "tmpfile", image->stride_extra);
               output->input_file = f;
               if (!checkopaque(image))
                  return 0;
            }

            else
               return logclose(image, f, "tmpfile", ": flush: ");
         }

         else
         {
            fclose(f);
            return logerror(image, "tmpfile", ": write failed", "");
         }
      }

      else
         return logerror(image, "tmpfile", ": open: ", strerror(errno));
#else  
      return logerror(image, "tmpfile", ": open: unsupported", "");
#endif  
   }

   else if (image->opts & USE_FILE)
   {
#ifdef PNG_SIMPLIFIED_WRITE_STDIO_SUPPORTED
      static int counter = 0;
      char name[32];

      sprintf(name, "%s%d.png", tmpf, ++counter);

      if (png_image_write_to_file(&image->image, name, convert_to_8bit,
         image->buffer+16, (png_int_32)image->stride, image->colormap))
      {
         initimage(output, image->opts, output->tmpfile_name,
            image->stride_extra);
          
         strcpy(output->tmpfile_name, name);

         if (!checkopaque(image))
            return 0;
      }

      else
         return logerror(image, name, ": write failed", "");
#else  
      return logerror(image, "stdio", ": open: unsupported", "");
#endif  
   }

   else  
   {
      png_alloc_size_t size;

      if (png_image_write_get_memory_size(image->image, size, convert_to_8bit,
               image->buffer+16, (png_int_32)image->stride, image->colormap))
      {
                                                                             
                                    
            
         if (size > PNG_IMAGE_PNG_SIZE_MAX(image->image))
            return logerror(image, "memory", ": PNG_IMAGE_SIZE_MAX wrong", "");

         initimage(output, image->opts, "memory", image->stride_extra);
         output->input_memory = malloc(size);

         if (output->input_memory != NULL)
         {
            output->input_memory_size = size;

            if (png_image_write_to_memory(&image->image, output->input_memory,
                  &output->input_memory_size, convert_to_8bit, image->buffer+16,
                  (png_int_32)image->stride, image->colormap))
            {
                                                                                
                  
               if (size != output->input_memory_size)
                  return logerror(image, "memory", ": memory size wrong", "");
            }

            else
               return logerror(image, "memory", ": write failed", "");
         }

         else
            return logerror(image, "memory", ": out of memory", "");
      }

      else
         return logerror(image, "memory", ": get size:", "");
   }

                                                                                
                                                                             
                                                                           
                                                                             
                                                                                
                               
      
   if (read_file(output, image->image.format | FORMAT_NO_CHANGE, NULL))
   {
      png_uint_32 original_format = image->image.format;

      if (convert_to_8bit)
         original_format &= ~PNG_FORMAT_FLAG_LINEAR;

      if ((output->image.format & BASE_FORMATS) !=
         (original_format & BASE_FORMATS))
         return logerror(image, image->file_name, ": format changed on read: ",
            output->file_name);

      return compare_two_images(image, output, 0 , NULL);
   }

   else
      return logerror(output, output->tmpfile_name,
         ": read of new file failed", "");
}
#endif

static int
testimage(Image *image, png_uint_32 opts, format_list *pf)
{
   int result;
   Image copy;

    
   checkopaque(image);
   copy = *image;

   copy.opts = opts;
   copy.buffer = NULL;
   copy.bufsize = 0;
   copy.allocsize = 0;

   image->input_file = NULL;
   image->input_memory = NULL;
   image->input_memory_size = 0;
   image->tmpfile_name[0] = 0;

   {
      png_uint_32 counter;
      Image output;

      newimage(&output);

      result = 1;

                                                                            
                                                                              
                                               
         
      for (counter=0; counter<2*FORMAT_COUNT; ++counter)
         if (format_isset(pf, counter >> 1))
      {
         png_uint_32 format = counter >> 1;

         png_color background_color;
         png_colorp background = NULL;

                                                                           
                                                                            
                                                                           
                                                                         
                                     
            
         if ((counter & 1) == 0)
         {
            if ((format & PNG_FORMAT_FLAG_ALPHA) == 0 &&
               (image->image.format & PNG_FORMAT_FLAG_ALPHA) != 0)
            {
                                                                       
                                                          
                  
               random_color(&background_color);
               background = &background_color;

                                                                          
                                                                               
                                        
                  
               if ((format & PNG_FORMAT_FLAG_COLORMAP) != 0 &&
                  (format & PNG_FORMAT_FLAG_LINEAR) == 0)
                  ++counter;
            }

                                                                             
                                                                    
               
            else
               ++counter;
         }
          

         resetimage(&copy);
         copy.opts = opts;  

         result = read_file(&copy, format, background);
         if (!result)
            break;

          
         result = compare_two_images(image, &copy, 0 , background);
         if (!result)
            break;

#        ifdef PNG_SIMPLIFIED_WRITE_SUPPORTED
                                                                              
                                                                          
                      
               
            output.opts = opts;
            result = write_one_file(&output, &copy, 0 );
            if (!result)
               break;

                                                                               
                                                                            
               
            result = compare_two_images(image, &output, 0, background);
            if (!result)
               break;

            if ((format & PNG_FORMAT_FLAG_LINEAR) != 0 &&
               (format & PNG_FORMAT_FLAG_COLORMAP) == 0)
            {
                                                                               
                  
               output.opts = opts;
               result = write_one_file(&output, &copy, 1 );
               if (!result)
                  break;

                                                                              
                                                                              
                                                                  
                 
                                                                               
                                                                           
                                                                           
                             
                  
               result = compare_two_images(image, &output, 1 ,
                  background);
               if (!result)
                  break;
            }
#        endif  
      }

      freeimage(&output);
   }

   freeimage(&copy);

   return result;
}

static int
test_one_file(const char *file_name, format_list *formats, png_uint_32 opts,
   int stride_extra, int log_pass)
{
   int result;
   Image image;

   if (!(opts & NO_RESEED))
      reseed();  
   newimage(&image);
   initimage(&image, opts, file_name, stride_extra);
   result = read_one_file(&image);
   if (result)
      result = testimage(&image, opts, formats);
   freeimage(&image);

    
   fflush(stderr);

   if (log_pass)
   {
      if (result)
         printf("PASS:");

      else
         printf("FAIL:");

#     ifndef PNG_SIMPLIFIED_WRITE_SUPPORTED
         printf(" (no write)");
#     endif

      print_opts(opts);
      printf(" %s\n", file_name);
       
      fflush(stdout);
   }

   else if (!result)
      exit(1);

   return result;
}

int
main(int argc, char **argv)
{
   png_uint_32 opts = FAST_WRITE | STRICT;
   format_list formats;
   const char *touch = NULL;
   int log_pass = 0;
   int redundant = 0;
   int stride_extra = 0;
   int retval = 0;
   int c;

#if PNG_LIBPNG_VER >= 10700
       
      opts |= GBG_ERROR;
#endif

   init_sRGB_to_d();
#if 0
   init_error_via_linear();
#endif
   format_init(&formats);
   reseed();  

   for (c=1; c<argc; ++c)
   {
      const char *arg = argv[c];

      if (strcmp(arg, "--log") == 0)
         log_pass = 1;
      else if (strcmp(arg, "--fresh") == 0)
      {
         memset(gpc_error, 0, sizeof gpc_error);
         memset(gpc_error_via_linear, 0, sizeof gpc_error_via_linear);
      }
      else if (strcmp(arg, "--file") == 0)
#        ifdef PNG_STDIO_SUPPORTED
            opts |= USE_FILE;
#        else
            return SKIP;  
#        endif
      else if (strcmp(arg, "--memory") == 0)
         opts &= ~USE_FILE;
      else if (strcmp(arg, "--stdio") == 0)
#        ifdef PNG_STDIO_SUPPORTED
            opts |= USE_STDIO;
#        else
            return SKIP;  
#        endif
      else if (strcmp(arg, "--name") == 0)
         opts &= ~USE_STDIO;
      else if (strcmp(arg, "--verbose") == 0)
         opts |= VERBOSE;
      else if (strcmp(arg, "--quiet") == 0)
         opts &= ~VERBOSE;
      else if (strcmp(arg, "--preserve") == 0)
         opts |= KEEP_TMPFILES;
      else if (strcmp(arg, "--nopreserve") == 0)
         opts &= ~KEEP_TMPFILES;
      else if (strcmp(arg, "--keep-going") == 0)
         opts |= KEEP_GOING;
      else if (strcmp(arg, "--fast") == 0)
         opts |= FAST_WRITE;
      else if (strcmp(arg, "--slow") == 0)
         opts &= ~FAST_WRITE;
      else if (strcmp(arg, "--accumulate") == 0)
         opts |= ACCUMULATE;
      else if (strcmp(arg, "--redundant") == 0)
         redundant = 1;
      else if (strcmp(arg, "--stop") == 0)
         opts &= ~KEEP_GOING;
      else if (strcmp(arg, "--strict") == 0)
         opts |= STRICT;
      else if (strcmp(arg, "--nostrict") == 0)
         opts &= ~STRICT;
      else if (strcmp(arg, "--sRGB-16bit") == 0)
         opts |= sRGB_16BIT;
      else if (strcmp(arg, "--linear-16bit") == 0)
         opts &= ~sRGB_16BIT;
      else if (strcmp(arg, "--noreseed") == 0)
         opts |= NO_RESEED;
      else if (strcmp(arg, "--fault-gbg-warning") == 0)
         opts |= GBG_ERROR;
      else if (strcmp(arg, "--tmpfile") == 0)
      {
         if (c+1 < argc)
         {
            if (strlen(argv[++c]) >= sizeof tmpf)
            {
               fflush(stdout);
               fprintf(stderr, "%s: %s is too long for a temp file prefix\n",
                  argv[0], argv[c]);
               exit(99);
            }

             
            strncpy(tmpf, argv[c], sizeof (tmpf)-1);
         }

         else
         {
            fflush(stdout);
            fprintf(stderr, "%s: %s requires a temporary file prefix\n",
               argv[0], arg);
            exit(99);
         }
      }
      else if (strcmp(arg, "--touch") == 0)
      {
         if (c+1 < argc)
            touch = argv[++c];

         else
         {
            fflush(stdout);
            fprintf(stderr, "%s: %s requires a file name argument\n",
               argv[0], arg);
            exit(99);
         }
      }
      else if (arg[0] == '+')
      {
         png_uint_32 format = formatof(arg+1);

         if (format > FORMAT_COUNT)
            exit(99);

         format_set(&formats, format);
      }
      else if (arg[0] == '-' && arg[1] != 0 && (arg[1] != '0' || arg[2] != 0))
      {
         fflush(stdout);
         fprintf(stderr, "%s: unknown option: %s\n", argv[0], arg);
         exit(99);
      }
      else
      {
         if (format_is_initial(&formats))
            format_default(&formats, redundant);

         if (arg[0] == '-')
         {
            int term = (arg[1] == '0' ? 0 : '\n');
            unsigned int ich = 0;

                                                                                
                                             
               
            static char buffer[4096];

            do
            {
               int ch = getchar();

                                                                             
                                                                            
                  
               if (ch == EOF || ch == term || ch == 0)
               {
                  buffer[ich] = 0;

                  if (ich > 0 && !test_one_file(buffer, &formats, opts,
                     stride_extra, log_pass))
                     retval = 1;

                  if (ch == EOF)
                     break;

                  ich = 0;
                  --ich;  
               }

               else
                  buffer[ich] = (char)ch;
            } while (++ich < sizeof buffer);

            if (ich)
            {
               buffer[32] = 0;
               buffer[4095] = 0;
               fprintf(stderr, "%s...%s: file name too long\n", buffer,
                  buffer+(4096-32));
               exit(99);
            }
         }

         else if (!test_one_file(arg, &formats, opts, stride_extra, log_pass))
            retval = 1;
      }
   }

   if (opts & ACCUMULATE)
   {
      unsigned int in;

      printf("/* contrib/libtests/pngstest-errors.h\n");
      printf(" *\n");
      printf(" * BUILT USING:" PNG_HEADER_VERSION_STRING);
      printf(" *\n");
      printf(" * This code is released under the libpng license.\n");
      printf(" * For conditions of distribution and use, see the disclaimer\n");
      printf(" * and license in png.h\n");
      printf(" *\n");
      printf(" * THIS IS A MACHINE GENERATED FILE: do not edit it directly!\n");
      printf(" * Instead run:\n");
      printf(" *\n");
      printf(" *    pngstest --accumulate\n");
      printf(" *\n");
      printf(" * on as many PNG files as possible; at least PNGSuite and\n");
      printf(" * contrib/libtests/testpngs.\n");
      printf(" */\n");

      printf("static png_uint_16 gpc_error[16/*in*/][16/*out*/][4/*a*/] =\n");
      printf("{\n");
      for (in=0; in<16; ++in)
      {
         unsigned int out;
         printf(" { /* input: %s */\n ", format_names[in]);
         for (out=0; out<16; ++out)
         {
            unsigned int alpha;
            printf(" {");
            for (alpha=0; alpha<4; ++alpha)
            {
               printf(" %d", gpc_error[in][out][alpha]);
               if (alpha < 3) putchar(',');
            }
            printf(" }");
            if (out < 15)
            {
               putchar(',');
               if (out % 4 == 3) printf("\n ");
            }
         }
         printf("\n }");

         if (in < 15)
            putchar(',');
         else
            putchar('\n');
      }
      printf("};\n");

      printf("static png_uint_16 gpc_error_via_linear[16][4/*out*/][4] =\n");
      printf("{\n");
      for (in=0; in<16; ++in)
      {
         unsigned int out;
         printf(" { /* input: %s */\n ", format_names[in]);
         for (out=0; out<4; ++out)
         {
            unsigned int alpha;
            printf(" {");
            for (alpha=0; alpha<4; ++alpha)
            {
               printf(" %d", gpc_error_via_linear[in][out][alpha]);
               if (alpha < 3) putchar(',');
            }
            printf(" }");
            if (out < 3)
               putchar(',');
         }
         printf("\n }");

         if (in < 15)
            putchar(',');
         else
            putchar('\n');
      }
      printf("};\n");

      printf("static png_uint_16 gpc_error_to_colormap[8/*i*/][8/*o*/][4] =\n");
      printf("{\n");
      for (in=0; in<8; ++in)
      {
         unsigned int out;
         printf(" { /* input: %s */\n ", format_names[in]);
         for (out=0; out<8; ++out)
         {
            unsigned int alpha;
            printf(" {");
            for (alpha=0; alpha<4; ++alpha)
            {
               printf(" %d", gpc_error_to_colormap[in][out][alpha]);
               if (alpha < 3) putchar(',');
            }
            printf(" }");
            if (out < 7)
            {
               putchar(',');
               if (out % 4 == 3) printf("\n ");
            }
         }
         printf("\n }");

         if (in < 7)
            putchar(',');
         else
            putchar('\n');
      }
      printf("};\n");
      printf("/* END MACHINE GENERATED */\n");
   }

   if (retval == 0 && touch != NULL)
   {
      FILE *fsuccess = fopen(touch, "wt");

      if (fsuccess != NULL)
      {
         int error = 0;
         fprintf(fsuccess, "PNG simple API tests succeeded\n");
         fflush(fsuccess);
         error = ferror(fsuccess);

         if (fclose(fsuccess) || error)
         {
            fflush(stdout);
            fprintf(stderr, "%s: write failed\n", touch);
            exit(99);
         }
      }

      else
      {
         fflush(stdout);
         fprintf(stderr, "%s: open failed\n", touch);
         exit(99);
      }
   }

   return retval;
}

#else  
int main(void)
{
   fprintf(stderr, "pngstest: no read support in libpng, test skipped\n");
    
   return SKIP;
}
#endif  
