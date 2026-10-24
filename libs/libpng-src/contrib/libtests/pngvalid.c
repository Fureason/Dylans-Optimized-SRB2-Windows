
/* pngvalid.c - validate libpng by constructing then reading png files.
 *
 * Last changed in libpng 1.6.31 [July 27, 2017]
 * Copyright (c) 2014-2017 John Cunningham Bowler
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * NOTES:
 *   This is a C program that is intended to be linked against libpng.  It
 *   generates bitmaps internally, stores them as PNG files (using the
 *   sequential write code) then reads them back (using the sequential
 *   read code) and validates that the result has the correct data.
 *
 *   The program can be modified and extended to test the correctness of
 *   transformations performed by libpng.
 */

#define _POSIX_SOURCE 1
#define _ISOC99_SOURCE 1  
#define _GNU_SOURCE 1  
#define _BSD_SOURCE 1  

#include <signal.h>
#include <stdio.h>

#if defined(HAVE_CONFIG_H) && !defined(PNG_NO_CONFIG_H)
#  include <config.h>
#endif

#ifdef HAVE_FEENABLEEXCEPT  
#  include <fenv.h>
#endif

#ifndef FE_DIVBYZERO
#  define FE_DIVBYZERO 0
#endif
#ifndef FE_INVALID
#  define FE_INVALID 0
#endif
#ifndef FE_OVERFLOW
#  define FE_OVERFLOW 0
#endif

                                                                              
                                 
   
#ifdef PNG_FREESTANDING_TESTS
#  include <png.h>
#else
#  include "../../png.h"
#endif

#ifdef PNG_ZLIB_HEADER
#  include PNG_ZLIB_HEADER
#else
#  include <zlib.h>    
#endif

                                                                                
                                                                                
   
#if PNG_LIBPNG_VER >= 10601 && defined(HAVE_CONFIG_H)
#  define SKIP 77
#else
#  define SKIP 0
#endif

                                                                               
   
#if defined(PNG_WRITE_SUPPORTED) &&\
   (defined(PNG_FIXED_POINT_SUPPORTED) || defined(PNG_FLOATING_POINT_SUPPORTED))

#if PNG_LIBPNG_VER < 10500
 
typedef png_byte *png_const_bytep;

 
#define PNG_INTERLACE_ADAM7_PASSES 7
#define PNG_PASS_START_ROW(pass) (((1U&~(pass))<<(3-((pass)>>1)))&7)
#define PNG_PASS_START_COL(pass) (((1U& (pass))<<(3-(((pass)+1)>>1)))&7)
#define PNG_PASS_ROW_SHIFT(pass) ((pass)>2?(8-(pass))>>1:3)
#define PNG_PASS_COL_SHIFT(pass) ((pass)>1?(7-(pass))>>1:3)
#define PNG_PASS_ROWS(height, pass) (((height)+(((1<<PNG_PASS_ROW_SHIFT(pass))\
   -1)-PNG_PASS_START_ROW(pass)))>>PNG_PASS_ROW_SHIFT(pass))
#define PNG_PASS_COLS(width, pass) (((width)+(((1<<PNG_PASS_COL_SHIFT(pass))\
   -1)-PNG_PASS_START_COL(pass)))>>PNG_PASS_COL_SHIFT(pass))
#define PNG_ROW_FROM_PASS_ROW(yIn, pass) \
   (((yIn)<<PNG_PASS_ROW_SHIFT(pass))+PNG_PASS_START_ROW(pass))
#define PNG_COL_FROM_PASS_COL(xIn, pass) \
   (((xIn)<<PNG_PASS_COL_SHIFT(pass))+PNG_PASS_START_COL(pass))
#define PNG_PASS_MASK(pass,off) ( \
   ((0x110145AFU>>(((7-(off))-(pass))<<2)) & 0xFU) | \
   ((0x01145AF0U>>(((7-(off))-(pass))<<2)) & 0xF0U))
#define PNG_ROW_IN_INTERLACE_PASS(y, pass) \
   ((PNG_PASS_MASK(pass,0) >> ((y)&7)) & 1)
#define PNG_COL_IN_INTERLACE_PASS(x, pass) \
   ((PNG_PASS_MASK(pass,1) >> ((x)&7)) & 1)

 
#define PNG_WRITE_16BIT_SUPPORTED
#define PNG_READ_16BIT_SUPPORTED

 
#define PNG_FP_1 100000
#define PNG_GAMMA_THRESHOLD_FIXED\
   ((png_fixed_point)(PNG_GAMMA_THRESHOLD * PNG_FP_1))
#endif

#if PNG_LIBPNG_VER < 10600
                                                                              
                                        
      
#  define png_const_structp png_structp
#endif

#ifndef RELEASE_BUILD
    
#  define RELEASE_BUILD (PNG_LIBPNG_BUILD_BASE_TYPE >= PNG_LIBPNG_BUILD_RC)
#endif
#if RELEASE_BUILD
#   define debugonly(something)
#else  
#   define debugonly(something) something
#endif  

#include <float.h>   
#include <stdlib.h>  
#include <string.h>  
#include <math.h>    

 
#define CHUNK(a,b,c,d) (((a)<<24)+((b)<<16)+((c)<<8)+(d))
#define CHUNK_IHDR CHUNK(73,72,68,82)
#define CHUNK_PLTE CHUNK(80,76,84,69)
#define CHUNK_IDAT CHUNK(73,68,65,84)
#define CHUNK_IEND CHUNK(73,69,78,68)
#define CHUNK_cHRM CHUNK(99,72,82,77)
#define CHUNK_gAMA CHUNK(103,65,77,65)
#define CHUNK_sBIT CHUNK(115,66,73,84)
#define CHUNK_sRGB CHUNK(115,82,71,66)

                                                                                
                                                  
   
#ifndef UNUSED
#  if defined(__GNUC__) || defined(_MSC_VER)
#     define UNUSED(param) (void)param;
#  else
#     define UNUSED(param)
#  endif
#endif

 
#ifdef PNG_FREESTANDING_TESTS
#  include <cexcept.h>
#else
#  include "../visupng/cexcept.h"
#endif

#ifdef __cplusplus
#  define this not_the_cpp_this
#  define new not_the_cpp_new
#  define voidcast(type, value) static_cast<type>(value)
#else
#  define voidcast(type, value) (value)
#endif  

struct png_store;
define_exception_type(struct png_store*);

                                                                            
                                                
   
#define anon_context(ps) struct exception_context *the_exception_context = \
   &(ps)->exception_context
#define context(ps,fault) anon_context(ps); png_store *fault

                                                                              
                                                                           
                                                                              
                           
   
#define ARRAY_SIZE(a) ((unsigned int)((sizeof (a))/(sizeof (a)[0])))

                                                                              
                                                                              
                                                                               
  
                                                   
   
#define is_gnu(major,minor)\
   (defined __GNUC__) && __GNUC__ == (major) && __GNUC_MINOR__ == (minor)
#define is_gnu_patch(major,minor,patch)\
   is_gnu(major,minor) && __GNUC_PATCHLEVEL__ == 0
 
#ifdef __GNUC__
   const void * volatile make_volatile_for_gnu;
#  define gnu_volatile(x) make_volatile_for_gnu = &x;
#else  
#  define gnu_volatile(x)
#endif  

 
                                                                        
                                                                         
                                                                     
                                                                           
                                                    
   
static size_t safecat(char *buffer, size_t bufsize, size_t pos,
   const char *cat)
{
   while (pos < bufsize && cat != NULL && *cat != 0)
      buffer[pos++] = *cat++;

   if (pos >= bufsize)
      pos = bufsize-1;

   buffer[pos] = 0;
   return pos;
}

static size_t safecatn(char *buffer, size_t bufsize, size_t pos, int n)
{
   char number[64];
   sprintf(number, "%d", n);
   return safecat(buffer, bufsize, pos, number);
}

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
static size_t safecatd(char *buffer, size_t bufsize, size_t pos, double d,
    int precision)
{
   char number[64];
   sprintf(number, "%.*f", precision, d);
   return safecat(buffer, bufsize, pos, number);
}
#endif

static const char invalid[] = "invalid";
static const char sep[] = ": ";

static const char *colour_types[8] =
{
   "grayscale", invalid, "truecolour", "indexed-colour",
   "grayscale with alpha", invalid, "truecolour with alpha", invalid
};

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
 
static png_fixed_point
fix(double d)
{
   d = floor(d * PNG_FP_1 + .5);
   return (png_fixed_point)d;
}
#endif  

                                                                         
                                                                        
                                                                          
                                                                            
                                                                        
               
   
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

static void
make_four_random_bytes(png_uint_32* seed, png_bytep bytes)
{
   make_random_bytes(seed, bytes, 4);
}

#if defined PNG_READ_SUPPORTED || defined PNG_WRITE_tRNS_SUPPORTED ||\
    defined PNG_WRITE_FILTER_SUPPORTED
static void
randomize(void *pv, size_t size)
{
   static png_uint_32 random_seed[2] = {0x56789abc, 0xd};
   make_random_bytes(random_seed, pv, size);
}

#define R8(this) randomize(&(this), sizeof (this))

#ifdef PNG_READ_SUPPORTED
static png_byte
random_byte(void)
{
   unsigned char b1[1];
   randomize(b1, sizeof b1);
   return b1[0];
}
#endif  

static png_uint_16
random_u16(void)
{
   unsigned char b2[2];
   randomize(b2, sizeof b2);
   return png_get_uint_16(b2);
}

#if defined PNG_READ_RGB_TO_GRAY_SUPPORTED ||\
    defined PNG_READ_FILLER_SUPPORTED
static png_uint_32
random_u32(void)
{
   unsigned char b4[4];
   randomize(b4, sizeof b4);
   return png_get_uint_32(b4);
}
#endif  

#endif  

#if defined PNG_READ_TRANSFORMS_SUPPORTED ||\
    defined PNG_WRITE_FILTER_SUPPORTED
static unsigned int
random_mod(unsigned int max)
{
   return random_u16() % max;  
}
#endif  

#if (defined PNG_READ_RGB_TO_GRAY_SUPPORTED) ||\
    (defined PNG_READ_FILLER_SUPPORTED)
static int
random_choice(void)
{
   return random_byte() & 1;
}
#endif  

                                                                            
                                                                         
                                                                                
                                                                             
                                            
   
#define FILEID(col, depth, palette, interlace, width, height, do_interlace) \
   ((png_uint_32)((col) + ((depth)<<3) + ((palette)<<8) + ((interlace)<<13) + \
    (((do_interlace)!=0)<<15) + ((width)<<16) + ((height)<<24)))

#define COL_FROM_ID(id) ((png_byte)((id)& 0x7U))
#define DEPTH_FROM_ID(id) ((png_byte)(((id) >> 3) & 0x1fU))
#define PALETTE_FROM_ID(id) (((id) >> 8) & 0x1f)
#define INTERLACE_FROM_ID(id) ((png_byte)(((id) >> 13) & 0x3))
#define DO_INTERLACE_FROM_ID(id) ((int)(((id)>>15) & 1))
#define WIDTH_FROM_ID(id) (((id)>>16) & 0xff)
#define HEIGHT_FROM_ID(id) (((id)>>24) & 0xff)

 
static size_t
standard_name(char *buffer, size_t bufsize, size_t pos, png_byte colour_type,
    int bit_depth, unsigned int npalette, int interlace_type,
    png_uint_32 w, png_uint_32 h, int do_interlace)
{
   pos = safecat(buffer, bufsize, pos, colour_types[colour_type]);
   if (colour_type == 3)  
   {
      pos = safecat(buffer, bufsize, pos, "[");
      pos = safecatn(buffer, bufsize, pos, npalette);
      pos = safecat(buffer, bufsize, pos, "]");
   }

   else if (npalette != 0)
      pos = safecat(buffer, bufsize, pos, "+tRNS");

   pos = safecat(buffer, bufsize, pos, " ");
   pos = safecatn(buffer, bufsize, pos, bit_depth);
   pos = safecat(buffer, bufsize, pos, " bit");

   if (interlace_type != PNG_INTERLACE_NONE)
   {
      pos = safecat(buffer, bufsize, pos, " interlaced");
      if (do_interlace)
         pos = safecat(buffer, bufsize, pos, "(pngvalid)");
      else
         pos = safecat(buffer, bufsize, pos, "(libpng)");
   }

   if (w > 0 || h > 0)
   {
      pos = safecat(buffer, bufsize, pos, " ");
      pos = safecatn(buffer, bufsize, pos, w);
      pos = safecat(buffer, bufsize, pos, "x");
      pos = safecatn(buffer, bufsize, pos, h);
   }

   return pos;
}

static size_t
standard_name_from_id(char *buffer, size_t bufsize, size_t pos, png_uint_32 id)
{
   return standard_name(buffer, bufsize, pos, COL_FROM_ID(id),
      DEPTH_FROM_ID(id), PALETTE_FROM_ID(id), INTERLACE_FROM_ID(id),
      WIDTH_FROM_ID(id), HEIGHT_FROM_ID(id), DO_INTERLACE_FROM_ID(id));
}

                                                                                
                                                                               
                         
   
#ifdef PNG_WRITE_16BIT_SUPPORTED
#  define WRITE_BDHI 4
#  ifdef PNG_READ_16BIT_SUPPORTED
#     define READ_BDHI 4
#     define DO_16BIT
#  endif
#else
#  define WRITE_BDHI 3
#endif
#ifndef DO_16BIT
#  define READ_BDHI 3
#endif

                                                                         
                                                        
   
#define PALETTE_COUNT(bit_depth) ((bit_depth) > 4 ? 1U : 16U)

static int
next_format(png_bytep colour_type, png_bytep bit_depth,
   unsigned int* palette_number, int low_depth_gray, int tRNS)
{
   if (*bit_depth == 0)
   {
      *colour_type = 0;
      if (low_depth_gray)
         *bit_depth = 1;
      else
         *bit_depth = 8;
      *palette_number = 0;
      return 1;
   }

   if  (*colour_type < 4 )
   {
                                                                     
                                                     
         
      unsigned int pn = ++*palette_number;
      png_byte ct = *colour_type;

      if (((ct == 0  || ct  == 2) && tRNS && pn < 2) ||
          (ct == 3  && pn < PALETTE_COUNT(*bit_depth)))
         return 1;

       
      *palette_number = 0;
   }

   *bit_depth = (png_byte)(*bit_depth << 1);

    
   if (*bit_depth <= 8
#ifdef DO_16BIT
         || (*colour_type != 3 && *bit_depth <= 16)
#endif
      )
      return 1;

    
   switch (*colour_type)
   {
      case 0:
         *colour_type = 2;
         *bit_depth = 8;
         return 1;

      case 2:
         *colour_type = 3;
         *bit_depth = 1;
         return 1;

      case 3:
         *colour_type = 4;
         *bit_depth = 8;
         return 1;

      case 4:
         *colour_type = 6;
         *bit_depth = 8;
         return 1;

      default:
         return 0;
   }
}

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
static unsigned int
sample(png_const_bytep row, png_byte colour_type, png_byte bit_depth,
    png_uint_32 x, unsigned int sample_index, int swap16, int littleendian)
{
   png_uint_32 bit_index, result;

    
   x *= bit_depth;
   bit_index = x;

   if ((colour_type & 1) == 0)  
   {
      if (colour_type & 2)
         bit_index *= 3;

      if (colour_type & 4)
         bit_index += x;  

       
      if (colour_type & (2+4))
         bit_index += sample_index * bit_depth;
   }

    
   row += bit_index >> 3;
   result = *row;

   if (bit_depth == 8)
      return result;

   else if (bit_depth > 8)
   {
      if (swap16)
         return (*++row << 8) + result;
      else
         return (result << 8) + *++row;
   }

                                                                     
                                                                   
                                       
      
   bit_index &= 7;

   if (!littleendian)
      bit_index = 8-bit_index-bit_depth;

   return (result >> bit_index) & ((1U<<bit_depth)-1);
}
#endif  

                                                                     
                                                                        
                                                                      
                                                                       
                                                                 
  
                                                 
   
static void
pixel_copy(png_bytep toBuffer, png_uint_32 toIndex,
   png_const_bytep fromBuffer, png_uint_32 fromIndex, unsigned int pixelSize,
   int littleendian)
{
                                                                      
                                      
      
   toIndex *= pixelSize;
   fromIndex *= pixelSize;
   if (pixelSize < 8)  
   {
       
      unsigned int destMask = ((1U<<pixelSize)-1) <<
         (littleendian ? toIndex&7 : 8-pixelSize-(toIndex&7));
       
      unsigned int destByte = toBuffer[toIndex >> 3] & ~destMask;
      unsigned int sourceByte = fromBuffer[fromIndex >> 3];

       
      fromIndex &= 7;
      if (littleendian)
      {
         if (fromIndex > 0) sourceByte >>= fromIndex;
         if ((toIndex & 7) > 0) sourceByte <<= toIndex & 7;
      }

      else
      {
         if (fromIndex > 0) sourceByte <<= fromIndex;
         if ((toIndex & 7) > 0) sourceByte >>= toIndex & 7;
      }

      toBuffer[toIndex >> 3] = (png_byte)(destByte | (sourceByte & destMask));
   }
   else  
      memmove(toBuffer+(toIndex>>3), fromBuffer+(fromIndex>>3), pixelSize>>3);
}

#ifdef PNG_READ_SUPPORTED
                                                                       
                    
   
static void
row_copy(png_bytep toBuffer, png_const_bytep fromBuffer, unsigned int bitWidth,
      int littleendian)
{
   memcpy(toBuffer, fromBuffer, bitWidth >> 3);

   if ((bitWidth & 7) != 0)
   {
      unsigned int mask;

      toBuffer += bitWidth >> 3;
      fromBuffer += bitWidth >> 3;
      if (littleendian)
         mask = 0xff << (bitWidth & 7);
      else
         mask = 0xff >> (bitWidth & 7);
      *toBuffer = (png_byte)((*toBuffer & mask) | (*fromBuffer & ~mask));
   }
}

                                                                      
                 
   
static int
pixel_cmp(png_const_bytep pa, png_const_bytep pb, png_uint_32 bit_width)
{
#if PNG_LIBPNG_VER < 10506
   if (memcmp(pa, pb, bit_width>>3) == 0)
   {
      png_uint_32 p;

      if ((bit_width & 7) == 0) return 0;

       
      p = pa[bit_width >> 3];
      p ^= pb[bit_width >> 3];

      if (p == 0) return 0;

                                                                    
                                                   
         
      bit_width &= 7;
      p >>= 8-bit_width;

      if (p == 0) return 0;
   }
#else
                                                                              
               
      
   if (memcmp(pa, pb, (bit_width+7)>>3) == 0)
      return 0;
#endif

    
   {
      png_uint_32 where = 0;

      while (pa[where] == pb[where]) ++where;
      return 1+where;
   }
}
#endif  

 
                                                                     
                                                                   
                             
   
#define STORE_BUFFER_SIZE 500  
typedef struct png_store_buffer
{
   struct png_store_buffer*  prev;     
   png_byte                  buffer[STORE_BUFFER_SIZE];
} png_store_buffer;

#define FILE_NAME_SIZE 64

typedef struct store_palette_entry  
{
   png_byte red;
   png_byte green;
   png_byte blue;
   png_byte alpha;
} store_palette_entry, store_palette[256];

typedef struct png_store_file
{
   struct png_store_file*  next;       
   char                    name[FILE_NAME_SIZE];
   unsigned int            IDAT_bits;  
   png_uint_32             IDAT_size;  
   png_uint_32             id;         
   size_t                  datacount;  
   png_store_buffer        data;       
   int                     npalette;   
   store_palette_entry*    palette;    
} png_store_file;

                                                                               
             
   
typedef struct store_pool
{
   struct png_store    *store;    
   struct store_memory *list;     
   png_byte             mark[4];  

    
   png_alloc_size_t     max;      
   png_alloc_size_t     current;  
   png_alloc_size_t     limit;    
   png_alloc_size_t     total;    

    
   png_alloc_size_t     max_max;
   png_alloc_size_t     max_limit;
   png_alloc_size_t     max_total;
} store_pool;

typedef struct png_store
{
                                                                   
                                                                   
                                                        
      
   struct exception_context
                      exception_context;

   unsigned int       verbose :1;
   unsigned int       treat_warnings_as_errors :1;
   unsigned int       expect_error :1;
   unsigned int       expect_warning :1;
   unsigned int       saw_warning :1;
   unsigned int       speed :1;
   unsigned int       progressive :1;  
   unsigned int       validated :1;    
   int                nerrors;
   int                nwarnings;
   int                noptions;        
   struct {
      unsigned char   option;          
      unsigned char   setting;         
   }                  options[16];
   char               test[128];       
   char               error[256];

    
   png_uint_32        chunklen;  
   png_uint_32        chunktype; 
   png_uint_32        chunkpos;  
   png_uint_32        IDAT_size; 
   unsigned int       IDAT_bits; 

    
   png_structp        pread;     
   png_infop          piread;
   png_store_file*    current;   
   png_store_buffer*  next;      
   size_t             readpos;   
   png_byte*          image;     
   size_t             cb_image;  
   size_t             cb_row;    
   uLong              IDAT_crc;
   png_uint_32        IDAT_len;  
   png_uint_32        IDAT_pos;  
   png_uint_32        image_h;   
   store_pool         read_memory_pool;

    
   png_store_file*    saved;
   png_structp        pwrite;    
   png_infop          piwrite;
   size_t             writepos;  
   char               wname[FILE_NAME_SIZE];
   png_store_buffer   new;       
   store_pool         write_memory_pool;
   store_palette_entry* palette;
   int                  npalette;
} png_store;

 
static void
store_pool_mark(png_bytep mark)
{
   static png_uint_32 store_seed[2] = { 0x12345678, 1};

   make_four_random_bytes(store_seed, mark);
}

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
                                                                            
            
   
static png_uint_32
random_32(void)
{

   for (;;)
   {
      png_byte mark[4];
      png_uint_32 result;

      store_pool_mark(mark);
      result = png_get_uint_32(mark);

      if (result != 0)
         return result;
   }
}
#endif  

static void
store_pool_init(png_store *ps, store_pool *pool)
{
   memset(pool, 0, sizeof *pool);

   pool->store = ps;
   pool->list = NULL;
   pool->max = pool->current = pool->limit = pool->total = 0;
   pool->max_max = pool->max_limit = pool->max_total = 0;
   store_pool_mark(pool->mark);
}

static void
store_init(png_store* ps)
{
   memset(ps, 0, sizeof *ps);
   init_exception_context(&ps->exception_context);
   store_pool_init(ps, &ps->read_memory_pool);
   store_pool_init(ps, &ps->write_memory_pool);
   ps->verbose = 0;
   ps->treat_warnings_as_errors = 0;
   ps->expect_error = 0;
   ps->expect_warning = 0;
   ps->saw_warning = 0;
   ps->speed = 0;
   ps->progressive = 0;
   ps->validated = 0;
   ps->nerrors = ps->nwarnings = 0;
   ps->pread = NULL;
   ps->piread = NULL;
   ps->saved = ps->current = NULL;
   ps->next = NULL;
   ps->readpos = 0;
   ps->image = NULL;
   ps->cb_image = 0;
   ps->cb_row = 0;
   ps->image_h = 0;
   ps->pwrite = NULL;
   ps->piwrite = NULL;
   ps->writepos = 0;
   ps->chunkpos = 8;
   ps->chunktype = 0;
   ps->chunklen = 16;
   ps->IDAT_size = 0;
   ps->IDAT_bits = 0;
   ps->new.prev = NULL;
   ps->palette = NULL;
   ps->npalette = 0;
   ps->noptions = 0;
}

static void
store_freebuffer(png_store_buffer* psb)
{
   if (psb->prev)
   {
      store_freebuffer(psb->prev);
      free(psb->prev);
      psb->prev = NULL;
   }
}

static void
store_freenew(png_store *ps)
{
   store_freebuffer(&ps->new);
   ps->writepos = 0;
   ps->chunkpos = 8;
   ps->chunktype = 0;
   ps->chunklen = 16;
   ps->IDAT_size = 0;
   ps->IDAT_bits = 0;
   if (ps->palette != NULL)
   {
      free(ps->palette);
      ps->palette = NULL;
      ps->npalette = 0;
   }
}

static void
store_storenew(png_store *ps)
{
   png_store_buffer *pb;

   pb = voidcast(png_store_buffer*, malloc(sizeof *pb));

   if (pb == NULL)
      png_error(ps->pwrite, "store new: OOM");

   *pb = ps->new;
   ps->new.prev = pb;
   ps->writepos = 0;
}

static void
store_freefile(png_store_file **ppf)
{
   if (*ppf != NULL)
   {
      store_freefile(&(*ppf)->next);

      store_freebuffer(&(*ppf)->data);
      (*ppf)->datacount = 0;
      if ((*ppf)->palette != NULL)
      {
         free((*ppf)->palette);
         (*ppf)->palette = NULL;
         (*ppf)->npalette = 0;
      }
      free(*ppf);
      *ppf = NULL;
   }
}

static unsigned int
bits_of(png_uint_32 num)
{
    
   unsigned int b = 0;

   if (num & 0xffff0000U)  b += 16U, num >>= 16;
   if (num & 0xff00U)      b += 8U, num >>= 8;
   if (num & 0xf0U)        b += 4U, num >>= 4;
   if (num & 0xcU)         b += 2U, num >>= 2;
   if (num & 0x2U)         ++b, num >>= 1;
   if (num)                ++b;

   return b;  
}

                                                                            
                                                                              
   
static void
store_storefile(png_store *ps, png_uint_32 id)
{
   png_store_file *pf;

   if (ps->chunkpos != 0U || ps->chunktype != 0U || ps->chunklen != 0U ||
       ps->IDAT_size == 0)
      png_error(ps->pwrite, "storefile: incomplete write");

   pf = voidcast(png_store_file*, malloc(sizeof *pf));
   if (pf == NULL)
      png_error(ps->pwrite, "storefile: OOM");
   safecat(pf->name, sizeof pf->name, 0, ps->wname);
   pf->id = id;
   pf->data = ps->new;
   pf->datacount = ps->writepos;
   pf->IDAT_size = ps->IDAT_size;
   pf->IDAT_bits = bits_of(ps->IDAT_size);
    
   if (pf->IDAT_bits == 0U)
      png_error(ps->pwrite, "storefile: 0 sized IDAT");
   ps->new.prev = NULL;
   ps->writepos = 0;
   ps->chunkpos = 8;
   ps->chunktype = 0;
   ps->chunklen = 16;
   ps->IDAT_size = 0;
   pf->palette = ps->palette;
   pf->npalette = ps->npalette;
   ps->palette = 0;
   ps->npalette = 0;

    
   pf->next = ps->saved;
   ps->saved = pf;
}

 
static size_t
store_message(png_store *ps, png_const_structp pp, char *buffer, size_t bufsize,
   size_t pos, const char *msg)
{
   if (pp != NULL && pp == ps->pread)
   {
       
      pos = safecat(buffer, bufsize, pos, "read: ");

      if (ps->current != NULL)
      {
         pos = safecat(buffer, bufsize, pos, ps->current->name);
         pos = safecat(buffer, bufsize, pos, sep);
      }
   }

   else if (pp != NULL && pp == ps->pwrite)
   {
       
      pos = safecat(buffer, bufsize, pos, "write: ");
      pos = safecat(buffer, bufsize, pos, ps->wname);
      pos = safecat(buffer, bufsize, pos, sep);
   }

   else
   {
       
      pos = safecat(buffer, bufsize, pos, "pngvalid: ");
   }

   if (ps->test[0] != 0)
   {
      pos = safecat(buffer, bufsize, pos, ps->test);
      pos = safecat(buffer, bufsize, pos, sep);
   }
   pos = safecat(buffer, bufsize, pos, msg);
   return pos;
}

 
static void
store_verbose(png_store *ps, png_const_structp pp, png_const_charp prefix,
   png_const_charp message)
{
   char buffer[512];

   if (prefix)
      fputs(prefix, stderr);

   (void)store_message(ps, pp, buffer, sizeof buffer, 0, message);
   fputs(buffer, stderr);
   fputc('\n', stderr);
}

 
static void
store_log(png_store* ps, png_const_structp pp, png_const_charp message,
   int is_error)
{
                                                                                
                                                                              
                                                   
      
   if (is_error ? (ps->nerrors)++ == 0 :
       (ps->nwarnings)++ == 0 && ps->nerrors == 0)
      store_message(ps, pp, ps->error, sizeof ps->error, 0, message);

   if (ps->verbose)
      store_verbose(ps, pp, is_error ? "error: " : "warning: ", message);
}

#ifdef PNG_READ_SUPPORTED
 
static void
internal_error(png_store *ps, png_const_charp message)
{
   store_log(ps, NULL, message, 1  );

    
   {
      struct exception_context *the_exception_context = &ps->exception_context;
      Throw ps;
   }
}
#endif  

 
static void PNGCBAPI
store_error(png_structp ppIn, png_const_charp message)  
{
   png_const_structp pp = ppIn;
   png_store *ps = voidcast(png_store*, png_get_error_ptr(pp));

   if (!ps->expect_error)
      store_log(ps, pp, message, 1  );

    
   {
      struct exception_context *the_exception_context = &ps->exception_context;
      Throw ps;
   }
}

static void PNGCBAPI
store_warning(png_structp ppIn, png_const_charp message)
{
   png_const_structp pp = ppIn;
   png_store *ps = voidcast(png_store*, png_get_error_ptr(pp));

   if (!ps->expect_warning)
      store_log(ps, pp, message, 0  );
   else
      ps->saw_warning = 1;
}

                                                                             
                                                           
   
 
static png_bytep
store_image_row(const png_store* ps, png_const_structp pp, int nImage,
   png_uint_32 y)
{
   size_t coffset = (nImage * ps->image_h + y) * (ps->cb_row + 5) + 2;

   if (ps->image == NULL)
      png_error(pp, "no allocated image");

   if (coffset + ps->cb_row + 3 > ps->cb_image)
      png_error(pp, "image too small");

   return ps->image + coffset;
}

static void
store_image_free(png_store *ps, png_const_structp pp)
{
   if (ps->image != NULL)
   {
      png_bytep image = ps->image;

      if (image[-1] != 0xed || image[ps->cb_image] != 0xfe)
      {
         if (pp != NULL)
            png_error(pp, "png_store image overwrite (1)");
         else
            store_log(ps, NULL, "png_store image overwrite (2)", 1);
      }

      ps->image = NULL;
      ps->cb_image = 0;
      --image;
      free(image);
   }
}

static void
store_ensure_image(png_store *ps, png_const_structp pp, int nImages,
   size_t cbRow, png_uint_32 cRows)
{
   size_t cb = nImages * cRows * (cbRow + 5);

   if (ps->cb_image < cb)
   {
      png_bytep image;

      store_image_free(ps, pp);

       
      image = voidcast(png_bytep, malloc(cb+2));
      if (image == NULL)
      {
          
         if (pp == NULL)
            return;

         png_error(pp, "OOM allocating image buffer");
      }

       
      ++image;
      image[-1] = 0xed;
      image[cb] = 0xfe;

      ps->image = image;
      ps->cb_image = cb;
   }

                                                                              
                                                                       
                                                                                
      
   ps->cb_row = cbRow;
   ps->image_h = cRows;

                                                                           
                                                                               
                                                                        
                                                                               
                                                        
      
   memset(ps->image, 178, cb);

    
   while (--nImages >= 0)
   {
      png_uint_32 y;

      for (y=0; y<cRows; ++y)
      {
         png_bytep row = store_image_row(ps, pp, nImages, y);

          
         row[-2] = 190;
         row[-1] = 239;
         row[cbRow] = 222;
         row[cbRow+1] = 173;
         row[cbRow+2] = 17;
      }
   }
}

#ifdef PNG_READ_SUPPORTED
static void
store_image_check(const png_store* ps, png_const_structp pp, int iImage)
{
   png_const_bytep image = ps->image;

   if (image[-1] != 0xed || image[ps->cb_image] != 0xfe)
      png_error(pp, "image overwrite");
   else
   {
      size_t cbRow = ps->cb_row;
      png_uint_32 rows = ps->image_h;

      image += iImage * (cbRow+5) * ps->image_h;

      image += 2;  

      for (; rows > 0; --rows)
      {
         if (image[-2] != 190 || image[-1] != 239)
            png_error(pp, "row start overwritten");

         if (image[cbRow] != 222 || image[cbRow+1] != 173 ||
            image[cbRow+2] != 17)
            png_error(pp, "row end overwritten");

         image += cbRow+5;
      }
   }
}
#endif  

static int
valid_chunktype(png_uint_32 chunktype)
{
                                                                              
                           
      
   unsigned int i;

   for (i=0; i<4; ++i)
   {
      unsigned int c = chunktype & 0xffU;

      if (!((c >= 65U && c <= 90U) || (c >= 97U && c <= 122U)))
         return 0;

      chunktype >>= 8;
   }

   return 1;  
}

static void PNGCBAPI
store_write(png_structp ppIn, png_bytep pb, size_t st)
{
   png_const_structp pp = ppIn;
   png_store *ps = voidcast(png_store*, png_get_io_ptr(pp));
   size_t writepos = ps->writepos;
   png_uint_32 chunkpos = ps->chunkpos;
   png_uint_32 chunktype = ps->chunktype;
   png_uint_32 chunklen = ps->chunklen;

   if (ps->pwrite != pp)
      png_error(pp, "store state damaged");

                                                                              
                                                                         
                                       
      
   if (st > 0x7fffffffU)
      png_error(pp, "unexpected write size");

                                                                                
                                                                           
                                                       
      
   while (st > 0)
   {
      if (writepos >= STORE_BUFFER_SIZE)
         store_storenew(ps), writepos = 0;

      if (chunkpos < 4)
      {
         png_byte b = *pb++;
         --st;
         chunklen = (chunklen << 8) + b;
         ps->new.buffer[writepos++] = b;
         ++chunkpos;
      }

      else if (chunkpos < 8)
      {
         png_byte b = *pb++;
         --st;
         chunktype = (chunktype << 8) + b;
         ps->new.buffer[writepos++] = b;

         if (++chunkpos == 8)
         {
            chunklen &= 0xffffffffU;
            if (chunklen > 0x7fffffffU)
               png_error(pp, "chunk length too great");

            chunktype &= 0xffffffffU;
            if (chunktype == CHUNK_IDAT)
            {
               if (chunklen > ~ps->IDAT_size)
                  png_error(pp, "pngvalid internal image too large");

               ps->IDAT_size += chunklen;
            }

            else if (!valid_chunktype(chunktype))
               png_error(pp, "invalid chunk type");

            chunklen += 12;  
         }
      }

      else  
      {
         size_t cb = st;

         if (cb > STORE_BUFFER_SIZE - writepos)
            cb = STORE_BUFFER_SIZE - writepos;

         if (cb  > chunklen - chunkpos )
            cb = (size_t) (chunklen - chunkpos);

         memcpy(ps->new.buffer + writepos, pb, cb);
         chunkpos += (png_uint_32) cb;
         pb += cb;
         writepos += cb;
         st -= cb;

         if (chunkpos >= chunklen)  
            chunkpos = chunktype = chunklen = 0;
      }
   }  

   ps->writepos = writepos;
   ps->chunkpos = chunkpos;
   ps->chunktype = chunktype;
   ps->chunklen = chunklen;
}

static void PNGCBAPI
store_flush(png_structp ppIn)
{
   UNUSED(ppIn)  
}

#ifdef PNG_READ_SUPPORTED
static size_t
store_read_buffer_size(png_store *ps)
{
    
   if (ps->next != &ps->current->data)
      return STORE_BUFFER_SIZE;

   return ps->current->datacount;
}

 
static size_t
store_read_buffer_avail(png_store *ps)
{
   if (ps->current != NULL && ps->next != NULL)
   {
      png_store_buffer *next = &ps->current->data;
      size_t cbAvail = ps->current->datacount;

      while (next != ps->next && next != NULL)
      {
         next = next->prev;
         cbAvail += STORE_BUFFER_SIZE;
      }

      if (next != ps->next)
         png_error(ps->pread, "buffer read error");

      if (cbAvail > ps->readpos)
         return cbAvail - ps->readpos;
   }

   return 0;
}

static int
store_read_buffer_next(png_store *ps)
{
   png_store_buffer *pbOld = ps->next;
   png_store_buffer *pbNew = &ps->current->data;
   if (pbOld != pbNew)
   {
      while (pbNew != NULL && pbNew->prev != pbOld)
         pbNew = pbNew->prev;

      if (pbNew != NULL)
      {
         ps->next = pbNew;
         ps->readpos = 0;
         return 1;
      }

      png_error(ps->pread, "buffer lost");
   }

   return 0;  
}

                                                                          
                                                                         
   
static void
store_read_imp(png_store *ps, png_bytep pb, size_t st)
{
   if (ps->current == NULL || ps->next == NULL)
      png_error(ps->pread, "store state damaged");

   while (st > 0)
   {
      size_t cbAvail = store_read_buffer_size(ps) - ps->readpos;

      if (cbAvail > 0)
      {
         if (cbAvail > st) cbAvail = st;
         memcpy(pb, ps->next->buffer + ps->readpos, cbAvail);
         st -= cbAvail;
         pb += cbAvail;
         ps->readpos += cbAvail;
      }

      else if (!store_read_buffer_next(ps))
         png_error(ps->pread, "read beyond end of file");
   }
}

static size_t
store_read_chunk(png_store *ps, png_bytep pb, size_t max, size_t min)
{
   png_uint_32 chunklen = ps->chunklen;
   png_uint_32 chunktype = ps->chunktype;
   png_uint_32 chunkpos = ps->chunkpos;
   size_t st = max;

   if (st > 0) do
   {
      if (chunkpos >= chunklen)  
      {
         png_byte buffer[8];

          
         store_read_imp(ps, buffer, 8U);
         chunklen = png_get_uint_32(buffer) + 12U;
         chunktype = png_get_uint_32(buffer+4U);
         chunkpos = 0U;  
      }

      if (chunktype == CHUNK_IDAT)
      {
         png_uint_32 IDAT_pos = ps->IDAT_pos;
         png_uint_32 IDAT_len = ps->IDAT_len;
         png_uint_32 IDAT_size = ps->IDAT_size;

          
         if (chunkpos < 8U)
            chunkpos = 8U;

         if (IDAT_pos == IDAT_len)
         {
            png_byte random = random_byte();

                                                                              
                                                                            
                                                                         
                                                                
               
            if (IDAT_len == 0U)  
            {
               switch (random & 3U)
               {
                  case 0U: IDAT_len = 12U; break;  
                  case 1U: IDAT_len = 13U; break;  
                  default: IDAT_len = random_u32();
                           IDAT_len %= IDAT_size;
                           IDAT_len += 13U;  
                           break;
               }
            }

            else if (IDAT_size == 0U)  
            {
                
               if (chunkpos != chunklen-4U)
                  png_error(ps->pread, "internal: IDAT size mismatch");

                                                                         
                                                                      
                                                                             
                          
                  
               if ((random & 3U) == 0U)
                  IDAT_len = 12U;  

               else
               {
                                                                               
                                                       
                     
                  png_byte buffer[4];

                  store_read_imp(ps, buffer, 4U);
                  chunkpos += 4U;
                  ps->IDAT_pos = IDAT_pos;
                  ps->IDAT_len = IDAT_len;
                  ps->IDAT_size = 0U;
                  continue;  
               }
            }

            else
            {
                                                                               
                                            
                  
               IDAT_len = random_u32();
               IDAT_len &= (1U << (1U + random % ps->IDAT_bits)) - 1U;
               if (IDAT_len > IDAT_size)
                  IDAT_len = IDAT_size;
               IDAT_len += 12U;  
            }

            IDAT_pos = 0U;
            ps->IDAT_crc = 0x35af061e;  
         }  

         if (IDAT_pos < 8U)   do
         {
            png_uint_32 b;
            unsigned int shift;

            if (IDAT_pos < 4U)
               b = IDAT_len - 12U;

            else
               b = CHUNK_IDAT;

            shift = 3U & IDAT_pos;
            ++IDAT_pos;

            if (shift < 3U)
               b >>= 8U*(3U-shift);

            *pb++ = 0xffU & b;
         }
         while (--st > 0 && IDAT_pos < 8);

         else if (IDAT_pos < IDAT_len - 4U)  
         {
            if (chunkpos < chunklen-4U)
            {
               uInt avail = (uInt)-1;

               if (avail > (IDAT_len-4U) - IDAT_pos)
                  avail = (uInt) ((IDAT_len-4U) - IDAT_pos);

               if (avail > st)
                  avail = (uInt) st;

               if (avail > (chunklen-4U) - chunkpos)
                  avail = (uInt) ((chunklen-4U) - chunkpos);

               store_read_imp(ps, pb, avail);
               ps->IDAT_crc = crc32(ps->IDAT_crc, pb, avail);
               pb += (size_t) avail;
               st -= (size_t) avail;
               chunkpos += (png_uint_32) avail;
               IDAT_size -= (png_uint_32) avail;
               IDAT_pos += (png_uint_32) avail;
            }

            else  
            {
               png_byte buffer[4];

               store_read_imp(ps, buffer, 4U);
               chunkpos += 4U;
            }
         }

         else   do
         {
            uLong b = ps->IDAT_crc;
            unsigned int shift = (IDAT_len - IDAT_pos);  
            ++IDAT_pos;

            if (shift > 1U)
               b >>= 8U*(shift-1U);

            *pb++ = 0xffU & b;
         }
         while (--st > 0 && IDAT_pos < IDAT_len);

         ps->IDAT_pos = IDAT_pos;
         ps->IDAT_len = IDAT_len;
         ps->IDAT_size = IDAT_size;
      }

      else  
      {
                                                                               
                                              
            
         if (ps->IDAT_len > 0 && ps->IDAT_size > 0)
            png_error(ps->pread, "internal: missing IDAT data");

         if (chunktype == CHUNK_IEND && ps->IDAT_len == 0U)
            png_error(ps->pread, "internal: missing IDAT");

         if (chunkpos < 8U)   do
         {
            png_uint_32 b;
            unsigned int shift;

            if (chunkpos < 4U)
               b = chunklen - 12U;

            else
               b = chunktype;

            shift = 3U & chunkpos;
            ++chunkpos;

            if (shift < 3U)
               b >>= 8U*(3U-shift);

            *pb++ = 0xffU & b;
         }
         while (--st > 0 && chunkpos < 8);

         else  
         {
            size_t avail = st;

            if (avail > chunklen - chunkpos)
               avail = (size_t) (chunklen - chunkpos);

            store_read_imp(ps, pb, avail);
            pb += avail;
            st -= avail;
            chunkpos += (png_uint_32) avail;

                                                                              
                                                                              
               
            if (chunkpos >= chunklen && max-st >= min &&
                     store_read_buffer_avail(ps) == 0)
               break;
         }
      }  
   }
   while (st > 0);

   ps->chunklen = chunklen;
   ps->chunktype = chunktype;
   ps->chunkpos = chunkpos;

   return st;  
}

static void PNGCBAPI
store_read(png_structp ppIn, png_bytep pb, size_t st)
{
   png_const_structp pp = ppIn;
   png_store *ps = voidcast(png_store*, png_get_io_ptr(pp));

   if (ps == NULL || ps->pread != pp)
      png_error(pp, "bad store read call");

   store_read_chunk(ps, pb, st, st);
}

static void
store_progressive_read(png_store *ps, png_structp pp, png_infop pi)
{
   if (ps->pread != pp || ps->current == NULL || ps->next == NULL)
      png_error(pp, "store state damaged (progressive)");

                                                                             
                                                                              
                                                                             
                                                                            
                    
      
   while (store_read_buffer_avail(ps) > 0)
   {
      static png_uint_32 noise = 2;
      size_t cb;
      png_byte buffer[512];

       
      noise = (noise << 9) | ((noise ^ (noise >> (9-5))) & 0x1ff);
      cb = noise & 0x1ff;
      cb -= store_read_chunk(ps, buffer, cb, 1);
      png_process_data(pp, pi, buffer, cb);
   }
}
#endif  

 
static store_palette_entry *
store_write_palette(png_store *ps, int npalette)
{
   if (ps->pwrite == NULL)
      store_log(ps, NULL, "attempt to write palette without write stream", 1);

   if (ps->palette != NULL)
      png_error(ps->pwrite, "multiple store_write_palette calls");

    
   if (npalette > 0)
   {
      ps->palette = voidcast(store_palette_entry*, malloc(npalette *
         sizeof *ps->palette));

      if (ps->palette == NULL)
         png_error(ps->pwrite, "store new palette: OOM");

      ps->npalette = npalette;
   }

   return ps->palette;
}

#ifdef PNG_READ_SUPPORTED
static store_palette_entry *
store_current_palette(png_store *ps, int *npalette)
{
                                                                      
                 
      
   if (ps->current == NULL)
   {
      store_log(ps, ps->pread, "no current stream for palette", 1);
      return NULL;
   }

    
   *npalette = ps->current->npalette;
   return ps->current->palette;
}
#endif  

 
#ifdef PNG_USER_MEM_SUPPORTED
                                                                             
                                                                            
                                                               
   
typedef struct store_memory
{
   store_pool          *pool;     
   struct store_memory *next;     
   png_alloc_size_t     size;     
   png_byte             mark[4];  
} store_memory;

                                                                          
                                                                                
                                                                           
                                                                             
                                                                            
                  
   
static void
store_pool_error(png_store *ps, png_const_structp pp, const char *msg)
{
   if (pp != NULL)
      png_error(pp, msg);

                                                                             
                                                                              
                              
      
   store_log(ps, pp, msg, 1  );
}

static void
store_memory_free(png_const_structp pp, store_pool *pool, store_memory *memory)
{
                                                                            
                                                                                
                                                                 
      
   if (memory->pool != pool)
      store_pool_error(pool->store, pp, "memory corrupted (pool)");

   else if (memcmp(memory->mark, pool->mark, sizeof memory->mark) != 0)
      store_pool_error(pool->store, pp, "memory corrupted (start)");

    
   else
   {
      png_alloc_size_t cb = memory->size;

      if (cb > pool->max)
         store_pool_error(pool->store, pp, "memory corrupted (size)");

      else if (memcmp((png_bytep)(memory+1)+cb, pool->mark, sizeof pool->mark)
         != 0)
         store_pool_error(pool->store, pp, "memory corrupted (end)");

       
      else
         {
         pool->current -= cb;
         free(memory);
         }
   }
}

static void
store_pool_delete(png_store *ps, store_pool *pool)
{
   if (pool->list != NULL)
   {
      fprintf(stderr, "%s: %s %s: memory lost (list follows):\n", ps->test,
         pool == &ps->read_memory_pool ? "read" : "write",
         pool == &ps->read_memory_pool ? (ps->current != NULL ?
            ps->current->name : "unknown file") : ps->wname);
      ++ps->nerrors;

      do
      {
         store_memory *next = pool->list;
         pool->list = next->next;
         next->next = NULL;

         fprintf(stderr, "\t%lu bytes @ %p\n",
             (unsigned long)next->size, (const void*)(next+1));
                                                                         
                      
            
         store_memory_free(NULL, pool, next);
      }
      while (pool->list != NULL);
   }

    
   if (pool->max > pool->max_max) pool->max_max = pool->max;
   pool->max = 0;
   if (pool->current != 0)  
      fprintf(stderr, "%s: %s %s: memory counter mismatch (internal error)\n",
         ps->test, pool == &ps->read_memory_pool ? "read" : "write",
         pool == &ps->read_memory_pool ? (ps->current != NULL ?
            ps->current->name : "unknown file") : ps->wname);
   pool->current = 0;

   if (pool->limit > pool->max_limit)
      pool->max_limit = pool->limit;

   pool->limit = 0;

   if (pool->total > pool->max_total)
      pool->max_total = pool->total;

   pool->total = 0;

    
   store_pool_mark(pool->mark);
}

 
static png_voidp PNGCBAPI
store_malloc(png_structp ppIn, png_alloc_size_t cb)
{
   png_const_structp pp = ppIn;
   store_pool *pool = voidcast(store_pool*, png_get_mem_ptr(pp));
   store_memory *new = voidcast(store_memory*, malloc(cb + (sizeof *new) +
      (sizeof pool->mark)));

   if (new != NULL)
   {
      if (cb > pool->max)
         pool->max = cb;

      pool->current += cb;

      if (pool->current > pool->limit)
         pool->limit = pool->current;

      pool->total += cb;

      new->size = cb;
      memcpy(new->mark, pool->mark, sizeof new->mark);
      memcpy((png_byte*)(new+1) + cb, pool->mark, sizeof pool->mark);
      new->pool = pool;
      new->next = pool->list;
      pool->list = new;
      ++new;
   }

   else
   {
                                                                               
                                                                         
                                                  
        
                                                                             
                     
                                                                         
                                                  
        
                                                                             
              
         
      store_log(pool->store, pp, "out of memory", 1  );
   }

   return new;
}

static void PNGCBAPI
store_free(png_structp ppIn, png_voidp memory)
{
   png_const_structp pp = ppIn;
   store_pool *pool = voidcast(store_pool*, png_get_mem_ptr(pp));
   store_memory *this = voidcast(store_memory*, memory), **test;

                                                                           
                                                                              
                                                                         
                             
      
   if (pp != pool->store->pread && pp != pool->store->pwrite)
      pp = NULL;

                                                                               
                                                                           
      
   --this;
   for (test = &pool->list; *test != this; test = &(*test)->next)
   {
      if (*test == NULL)
      {
         store_pool_error(pool->store, pp, "bad pointer to free");
         return;
      }
   }

    
   *test = this->next;
   this->next = NULL;
   store_memory_free(pp, pool, this);
}
#endif  

 
 
static void
store_write_reset(png_store *ps)
{
   if (ps->pwrite != NULL)
   {
      anon_context(ps);

      Try
         png_destroy_write_struct(&ps->pwrite, &ps->piwrite);

      Catch_anonymous
      {
          
      }

      ps->pwrite = NULL;
      ps->piwrite = NULL;
   }

                                                                         
                                                                               
      
#  ifdef PNG_USER_MEM_SUPPORTED
      store_pool_delete(ps, &ps->write_memory_pool);
#  endif

   store_freenew(ps);
}

                                                                         
                                                                    
                                                                            
                                                                      
   
static png_structp
set_store_for_write(png_store *ps, png_infopp ppi, const char *name)
{
   anon_context(ps);

   Try
   {
      if (ps->pwrite != NULL)
         png_error(ps->pwrite, "write store already in use");

      store_write_reset(ps);
      safecat(ps->wname, sizeof ps->wname, 0, name);

                                                                            
                                                       
         
#     ifdef PNG_USER_MEM_SUPPORTED
         if (!ps->speed)
            ps->pwrite = png_create_write_struct_2(PNG_LIBPNG_VER_STRING,
               ps, store_error, store_warning, &ps->write_memory_pool,
               store_malloc, store_free);

         else
#     endif
         ps->pwrite = png_create_write_struct(PNG_LIBPNG_VER_STRING,
            ps, store_error, store_warning);

      png_set_write_fn(ps->pwrite, ps, store_write, store_flush);

#     ifdef PNG_SET_OPTION_SUPPORTED
         {
            int opt;
            for (opt=0; opt<ps->noptions; ++opt)
               if (png_set_option(ps->pwrite, ps->options[opt].option,
                  ps->options[opt].setting) == PNG_OPTION_INVALID)
                  png_error(ps->pwrite, "png option invalid");
         }
#     endif

      if (ppi != NULL)
         *ppi = ps->piwrite = png_create_info_struct(ps->pwrite);
   }

   Catch_anonymous
      return NULL;

   return ps->pwrite;
}

                                                                              
                                                                          
                                                             
   
static void
store_read_reset(png_store *ps)
{
#  ifdef PNG_READ_SUPPORTED
      if (ps->pread != NULL)
      {
         anon_context(ps);

         Try
            png_destroy_read_struct(&ps->pread, &ps->piread, NULL);

         Catch_anonymous
         {
             
         }

         ps->pread = NULL;
         ps->piread = NULL;
      }
#  endif

#  ifdef PNG_USER_MEM_SUPPORTED
       
      store_pool_delete(ps, &ps->read_memory_pool);
#  endif

   ps->current = NULL;
   ps->next = NULL;
   ps->readpos = 0;
   ps->validated = 0;

   ps->chunkpos = 8;
   ps->chunktype = 0;
   ps->chunklen = 16;
   ps->IDAT_size = 0;
}

#ifdef PNG_READ_SUPPORTED
static void
store_read_set(png_store *ps, png_uint_32 id)
{
   png_store_file *pf = ps->saved;

   while (pf != NULL)
   {
      if (pf->id == id)
      {
         ps->current = pf;
         ps->next = NULL;
         ps->IDAT_size = pf->IDAT_size;
         ps->IDAT_bits = pf->IDAT_bits;  
         ps->IDAT_len = 0;
         ps->IDAT_pos = 0;
         ps->IDAT_crc = 0UL;
         store_read_buffer_next(ps);
         return;
      }

      pf = pf->next;
   }

   {
      size_t pos;
      char msg[FILE_NAME_SIZE+64];

      pos = standard_name_from_id(msg, sizeof msg, 0, id);
      pos = safecat(msg, sizeof msg, pos, ": file not found");
      png_error(ps->pread, msg);
   }
}

                                                                               
                                                                                
                                                                           
                                       
   
static png_structp
set_store_for_read(png_store *ps, png_infopp ppi, png_uint_32 id,
   const char *name)
{
    
   safecat(ps->test, sizeof ps->test, 0, name);

   if (ps->pread != NULL)
      png_error(ps->pread, "read store already in use");

   store_read_reset(ps);

                                                                        
                                                                             
                                                                          
                                                                            
                                                
      
#  ifdef PNG_USER_MEM_SUPPORTED
      if (!ps->speed)
         ps->pread = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, ps,
             store_error, store_warning, &ps->read_memory_pool, store_malloc,
             store_free);

      else
#  endif
   ps->pread = png_create_read_struct(PNG_LIBPNG_VER_STRING, ps, store_error,
      store_warning);

   if (ps->pread == NULL)
   {
      struct exception_context *the_exception_context = &ps->exception_context;

      store_log(ps, NULL, "png_create_read_struct returned NULL (unexpected)",
         1  );

      Throw ps;
   }

#  ifdef PNG_SET_OPTION_SUPPORTED
      {
         int opt;
         for (opt=0; opt<ps->noptions; ++opt)
            if (png_set_option(ps->pread, ps->options[opt].option,
               ps->options[opt].setting) == PNG_OPTION_INVALID)
                  png_error(ps->pread, "png option invalid");
      }
#  endif

   store_read_set(ps, id);

   if (ppi != NULL)
      *ppi = ps->piread = png_create_info_struct(ps->pread);

   return ps->pread;
}
#endif  

                                                                             
                                                       
   
static void
store_delete(png_store *ps)
{
   store_write_reset(ps);
   store_read_reset(ps);
   store_freefile(&ps->saved);
   store_image_free(ps, NULL);
}

 
                                                                              
                                                                             
                                                                               
                                                                     
                                                                         
                                                                             
                               
   
typedef enum modifier_state
{
   modifier_start,                         
   modifier_signature,                     
   modifier_IHDR                           
} modifier_state;

typedef struct CIE_color
{
                                                                           
                                                                               
                                                                              
                                              
      
   double X, Y, Z;
} CIE_color;

typedef struct color_encoding
{
                                                                            
                                                                            
                                                                      
                                                                          
                              
      
   double    gamma;             
   CIE_color red, green, blue;  
} color_encoding;

#ifdef PNG_READ_SUPPORTED
#if defined PNG_READ_TRANSFORMS_SUPPORTED && defined PNG_READ_cHRM_SUPPORTED
static double
chromaticity_x(CIE_color c)
{
   return c.X / (c.X + c.Y + c.Z);
}

static double
chromaticity_y(CIE_color c)
{
   return c.Y / (c.X + c.Y + c.Z);
}

static CIE_color
white_point(const color_encoding *encoding)
{
   CIE_color white;

   white.X = encoding->red.X + encoding->green.X + encoding->blue.X;
   white.Y = encoding->red.Y + encoding->green.Y + encoding->blue.Y;
   white.Z = encoding->red.Z + encoding->green.Z + encoding->blue.Z;

   return white;
}
#endif  

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
static void
normalize_color_encoding(color_encoding *encoding)
{
   const double whiteY = encoding->red.Y + encoding->green.Y +
      encoding->blue.Y;

   if (whiteY != 1)
   {
      encoding->red.X /= whiteY;
      encoding->red.Y /= whiteY;
      encoding->red.Z /= whiteY;
      encoding->green.X /= whiteY;
      encoding->green.Y /= whiteY;
      encoding->green.Z /= whiteY;
      encoding->blue.X /= whiteY;
      encoding->blue.Y /= whiteY;
      encoding->blue.Z /= whiteY;
   }
}
#endif

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
static size_t
safecat_color_encoding(char *buffer, size_t bufsize, size_t pos,
   const color_encoding *e, double encoding_gamma)
{
   if (e != 0)
   {
      if (encoding_gamma != 0)
         pos = safecat(buffer, bufsize, pos, "(");
      pos = safecat(buffer, bufsize, pos, "R(");
      pos = safecatd(buffer, bufsize, pos, e->red.X, 4);
      pos = safecat(buffer, bufsize, pos, ",");
      pos = safecatd(buffer, bufsize, pos, e->red.Y, 4);
      pos = safecat(buffer, bufsize, pos, ",");
      pos = safecatd(buffer, bufsize, pos, e->red.Z, 4);
      pos = safecat(buffer, bufsize, pos, "),G(");
      pos = safecatd(buffer, bufsize, pos, e->green.X, 4);
      pos = safecat(buffer, bufsize, pos, ",");
      pos = safecatd(buffer, bufsize, pos, e->green.Y, 4);
      pos = safecat(buffer, bufsize, pos, ",");
      pos = safecatd(buffer, bufsize, pos, e->green.Z, 4);
      pos = safecat(buffer, bufsize, pos, "),B(");
      pos = safecatd(buffer, bufsize, pos, e->blue.X, 4);
      pos = safecat(buffer, bufsize, pos, ",");
      pos = safecatd(buffer, bufsize, pos, e->blue.Y, 4);
      pos = safecat(buffer, bufsize, pos, ",");
      pos = safecatd(buffer, bufsize, pos, e->blue.Z, 4);
      pos = safecat(buffer, bufsize, pos, ")");
      if (encoding_gamma != 0)
         pos = safecat(buffer, bufsize, pos, ")");
   }

   if (encoding_gamma != 0)
   {
      pos = safecat(buffer, bufsize, pos, "^");
      pos = safecatd(buffer, bufsize, pos, encoding_gamma, 5);
   }

   return pos;
}
#endif  
#endif  

typedef struct png_modifier
{
   png_store               this;              
   struct png_modification *modifications;    

   modifier_state           state;            

    
   png_byte                 bit_depth;        
   png_byte                 colour_type;      

                                                                            
                                  
      
   png_uint_32              pending_len;
   png_uint_32              pending_chunk;

    
   double                   *gammas;
   unsigned int              ngammas;
   unsigned int              ngamma_tests;      
   double                    current_gamma;     
   const color_encoding *encodings;
   unsigned int              nencodings;
   const color_encoding *current_encoding;  
   unsigned int              encoding_counter;  
   int                       encoding_ignored;  

                                                                       
                                                                         
                                                                 
      
   unsigned int              repeat :1;    
   unsigned int              test_uses_encoding :1;

    
   png_byte                 sbitlow;

                                                                                
            
      
   double                   maxout8;   
   double                   maxabs8;   
   double                   maxcalc8;  
   double                   maxpc8;    
   double                   maxout16;  
   double                   maxabs16;  
   double                   maxcalc16; 
   double                   maxcalcG;  
   double                   maxpc16;   

                                                                           
                                                                               
                                                                               
                                    
     
                                                  
      
   double                   limit;     

                                                                    
             
      
   double                   log8;      
   double                   log16;     

    
   double                   error_gray_2;
   double                   error_gray_4;
   double                   error_gray_8;
   double                   error_gray_16;
   double                   error_color_8;
   double                   error_color_16;
   double                   error_indexed;

    
                                                                             
                            
      
   int                      use_update_info;

    
   int                      interlace_type :9;  

    
   unsigned int             test_standard :1;

    
   unsigned int             test_size :1;

    
   unsigned int             test_transform :1;
   unsigned int             test_tRNS :1;  

                                                                         
                                                                              
                                                                              
                                                                            
                                                                      
      
   unsigned int             use_input_precision :1;
   unsigned int             use_input_precision_sbit :1;
   unsigned int             use_input_precision_16to8 :1;

                                                                      
                                          
      
   unsigned int             calculations_use_input_precision :1;

                                                                                
                      
      
   unsigned int             assume_16_bit_calculations :1;

    
   unsigned int             test_gamma_threshold :1;
   unsigned int             test_gamma_transform :1;  
   unsigned int             test_gamma_sbit :1;
   unsigned int             test_gamma_scale16 :1;
   unsigned int             test_gamma_background :1;
   unsigned int             test_gamma_alpha_mode :1;
   unsigned int             test_gamma_expand16 :1;
   unsigned int             test_exhaustive :1;

                                                                             
                                                                               
                                       
      
   unsigned int             test_lbg :1;
   unsigned int             test_lbg_gamma_threshold :1;
   unsigned int             test_lbg_gamma_transform :1;
   unsigned int             test_lbg_gamma_sbit :1;
   unsigned int             test_lbg_gamma_composition :1;

   unsigned int             log :1;    

                                                                                
                                                                             
      
   size_t                   flush;            
   size_t                   buffer_count;     
   size_t                   buffer_position;  
   png_byte                 buffer[1024];
} png_modifier;

                                                                             
                                     
    
static int fail(png_modifier *pm)
{
   return !pm->log && !pm->this.verbose && (pm->this.nerrors > 0 ||
       (pm->this.treat_warnings_as_errors && pm->this.nwarnings > 0));
}

static void
modifier_init(png_modifier *pm)
{
   memset(pm, 0, sizeof *pm);
   store_init(&pm->this);
   pm->modifications = NULL;
   pm->state = modifier_start;
   pm->sbitlow = 1U;
   pm->ngammas = 0;
   pm->ngamma_tests = 0;
   pm->gammas = 0;
   pm->current_gamma = 0;
   pm->encodings = 0;
   pm->nencodings = 0;
   pm->current_encoding = 0;
   pm->encoding_counter = 0;
   pm->encoding_ignored = 0;
   pm->repeat = 0;
   pm->test_uses_encoding = 0;
   pm->maxout8 = pm->maxpc8 = pm->maxabs8 = pm->maxcalc8 = 0;
   pm->maxout16 = pm->maxpc16 = pm->maxabs16 = pm->maxcalc16 = 0;
   pm->maxcalcG = 0;
   pm->limit = 4E-3;
   pm->log8 = pm->log16 = 0;  
   pm->error_gray_2 = pm->error_gray_4 = pm->error_gray_8 = 0;
   pm->error_gray_16 = pm->error_color_8 = pm->error_color_16 = 0;
   pm->error_indexed = 0;
   pm->use_update_info = 0;
   pm->interlace_type = PNG_INTERLACE_NONE;
   pm->test_standard = 0;
   pm->test_size = 0;
   pm->test_transform = 0;
#  ifdef PNG_WRITE_tRNS_SUPPORTED
      pm->test_tRNS = 1;
#  else
      pm->test_tRNS = 0;
#  endif
   pm->use_input_precision = 0;
   pm->use_input_precision_sbit = 0;
   pm->use_input_precision_16to8 = 0;
   pm->calculations_use_input_precision = 0;
   pm->assume_16_bit_calculations = 0;
   pm->test_gamma_threshold = 0;
   pm->test_gamma_transform = 0;
   pm->test_gamma_sbit = 0;
   pm->test_gamma_scale16 = 0;
   pm->test_gamma_background = 0;
   pm->test_gamma_alpha_mode = 0;
   pm->test_gamma_expand16 = 0;
   pm->test_lbg = 1;
   pm->test_lbg_gamma_threshold = 1;
   pm->test_lbg_gamma_transform = 1;
   pm->test_lbg_gamma_sbit = 1;
   pm->test_lbg_gamma_composition = 1;
   pm->test_exhaustive = 0;
   pm->log = 0;

    
}

#ifdef PNG_READ_TRANSFORMS_SUPPORTED

                                                                            
                                                                                
                                                                              
             
   
#define DIGITIZE PNG_LIBPNG_VER < 10700

                                                                             
                                                                          
  
                                                                               
                                                                               
                                                                                
                   
   
#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
#if DIGITIZE
static double digitize(double value, int depth, int do_round)
{
                                                                               
                                                                              
                                                                         
                                                                              
                   
      
   unsigned int digitization_factor = (1U << depth) - 1;

                                                                                
                                                       
      
   if (value <= 0)
      value = 0;

   else if (value >= 1)
      value = 1;

   value *= digitization_factor;
   if (do_round) value += .5;
   return floor(value)/digitization_factor;
}
#endif
#endif  

#ifdef PNG_READ_GAMMA_SUPPORTED
static double abserr(const png_modifier *pm, int in_depth, int out_depth)
{
                                                                              
                       
      
   if (pm->assume_16_bit_calculations ||
      (pm->calculations_use_input_precision ? in_depth : out_depth) == 16)
      return pm->maxabs16;
   else
      return pm->maxabs8;
}

static double calcerr(const png_modifier *pm, int in_depth, int out_depth)
{
                                                                     
                                                   
      
   if ((pm->calculations_use_input_precision ? in_depth : out_depth) == 16)
      return pm->maxcalc16;
   else if (pm->assume_16_bit_calculations)
      return pm->maxcalcG;
   else
      return pm->maxcalc8;
}

static double pcerr(const png_modifier *pm, int in_depth, int out_depth)
{
                                                                               
                                                                     
      
   if (pm->assume_16_bit_calculations ||
      (pm->calculations_use_input_precision ? in_depth : out_depth) == 16)
      return pm->maxpc16 * .01;
   else
      return pm->maxpc8 * .01;
}

                                                                            
                                                                              
                                                                              
                                                                               
                                                                       
  
                                                                          
  
                                                                                
                    
   
static double outerr(const png_modifier *pm, int in_depth, int out_depth)
{
                                                                             
                                                                           
                                                                              
                                   
     
                              
      
   if (out_depth == 2)
      return .73182-.5;

   if (out_depth == 4)
      return .90644-.5;

   if ((pm->calculations_use_input_precision ? in_depth : out_depth) == 16)
      return pm->maxout16;

                                                                             
                        
      
   else if (out_depth == 16)
      return pm->maxout8 * 257;

   else
      return pm->maxout8;
}

                                                                             
                                                                             
                                                          
   
static double outlog(const png_modifier *pm, int in_depth, int out_depth)
{
                                                                                
                                                          
      
   if (out_depth <= 8)
   {
      if (pm->log8 == 0)  
         return 256;

      if (out_depth < 8)
         return pm->log8 / 255 * ((1<<out_depth)-1);

      return pm->log8;
   }

   if ((pm->calculations_use_input_precision ? in_depth : out_depth) == 16)
   {
      if (pm->log16 == 0)
         return 65536;

      return pm->log16;
   }

                                                                             
                        
      
   if (pm->log8 == 0)
      return 65536;

   return pm->log8 * 257;
}

                                                                               
                                                                               
                                                                                
       
   
static int output_quantization_factor(const png_modifier *pm, int in_depth,
   int out_depth)
{
   if (out_depth == 16 && in_depth != 16 &&
      pm->calculations_use_input_precision)
      return 257;
   else
      return 1;
}
#endif  

                                                                                
                                                                              
                                                                        
                                 
  
                                                                                
                                                                       
                                                 
   
typedef struct png_modification
{
   struct png_modification *next;
   png_uint_32              chunk;

    
   int                    (*modify_fn)(struct png_modifier *pm,
                               struct png_modification *me, int add);

                                                                              
                                                                                
                                                 
      
   png_uint_32              add;
   unsigned int             modified :1;      
   unsigned int             added    :1;      
   unsigned int             removed  :1;      
} png_modification;

static void
modification_reset(png_modification *pmm)
{
   if (pmm != NULL)
   {
      pmm->modified = 0;
      pmm->added = 0;
      pmm->removed = 0;
      modification_reset(pmm->next);
   }
}

static void
modification_init(png_modification *pmm)
{
   memset(pmm, 0, sizeof *pmm);
   pmm->next = NULL;
   pmm->chunk = 0;
   pmm->modify_fn = NULL;
   pmm->add = 0;
   modification_reset(pmm);
}

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
static void
modifier_current_encoding(const png_modifier *pm, color_encoding *ce)
{
   if (pm->current_encoding != 0)
      *ce = *pm->current_encoding;

   else
      memset(ce, 0, sizeof *ce);

   ce->gamma = pm->current_gamma;
}
#endif

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
static size_t
safecat_current_encoding(char *buffer, size_t bufsize, size_t pos,
   const png_modifier *pm)
{
   pos = safecat_color_encoding(buffer, bufsize, pos, pm->current_encoding,
      pm->current_gamma);

   if (pm->encoding_ignored)
      pos = safecat(buffer, bufsize, pos, "[overridden]");

   return pos;
}
#endif

                                                                             
      
  
                                         
                                                             
                                                                          
                                                                                
  
                                                                            
                                                                              
                                                                          
                                                                            
               
  
                                                                              
                                                                               
   
static unsigned int
modifier_total_encodings(const png_modifier *pm)
{
   return 1 +                  
      pm->ngammas +            
      pm->nencodings +         
                                                                       
                                                                            
                                                                               
                                           
        
                                                                       
                        
         
      ((pm->bit_depth == 16 || pm->assume_16_bit_calculations) ?
         pm->nencodings : 0);  
}

static void
modifier_encoding_iterate(png_modifier *pm)
{
   if (!pm->repeat &&  
      pm->test_uses_encoding)  
   {
      if (pm->test_exhaustive)
      {
         if (++pm->encoding_counter >= modifier_total_encodings(pm))
            pm->encoding_counter = 0;  
      }

      else
      {
                                                                               
                                                                   
            
         if (pm->encoding_counter == 0)
            pm->encoding_counter = random_mod(modifier_total_encodings(pm)-1)+1;
         else
            pm->encoding_counter = 0;
      }

      if (pm->encoding_counter > 0)
         pm->repeat = 1;
   }

   else if (!pm->repeat)
      pm->encoding_counter = 0;
}

static void
modifier_reset(png_modifier *pm)
{
   store_read_reset(&pm->this);
   pm->limit = 4E-3;
   pm->pending_len = pm->pending_chunk = 0;
   pm->flush = pm->buffer_count = pm->buffer_position = 0;
   pm->modifications = NULL;
   pm->state = modifier_start;
   modifier_encoding_iterate(pm);
                                                               
                                                                            
                                                             
      
   pm->test_uses_encoding = 0;
   pm->current_gamma = 0;
   pm->current_encoding = 0;
   pm->encoding_ignored = 0;
    
   pm->bit_depth = pm->colour_type = 0;
}

                                                                               
                                                                              
                        
   
static void
modifier_set_encoding(png_modifier *pm)
{
                                                                            
                                                                                
           
      
   pm->current_gamma = 0;
   pm->current_encoding = 0;
   pm->encoding_ignored = 0;  

    
   if (pm->encoding_counter > 0)
   {
                                                                              
                                
         
      if (pm->encoding_counter <= pm->ngammas)
         pm->current_gamma = 1/pm->gammas[pm->encoding_counter-1];

      else
      {
         unsigned int i = pm->encoding_counter - pm->ngammas;

         if (i >= pm->nencodings)
         {
            i %= pm->nencodings;
            pm->current_gamma = 1;  
         }

         else
            pm->current_gamma = pm->encodings[i].gamma;

         pm->current_encoding = pm->encodings + i;
      }
   }
}

                                                                               
                                                                            
   
static int
modifier_color_encoding_is_sRGB(const png_modifier *pm)
{
   return pm->current_encoding != 0 && pm->current_encoding == pm->encodings &&
      pm->current_encoding->gamma == pm->current_gamma;
}

static int
modifier_color_encoding_is_set(const png_modifier *pm)
{
   return pm->current_gamma != 0;
}

 
static void
modifier_crc(png_bytep buffer)
{
                                                             
                               
      
   uInt datalen = png_get_uint_32(buffer);
   uLong crc = crc32(0, buffer+4, datalen+4);
                                                                               
      
   png_save_uint_32(buffer+datalen+8, (png_uint_32)crc);
}

static void
modifier_setbuffer(png_modifier *pm)
{
   modifier_crc(pm->buffer);
   pm->buffer_count = png_get_uint_32(pm->buffer)+12;
   pm->buffer_position = 0;
}

                                                                            
                                                                              
              
   
static void
modifier_read_imp(png_modifier *pm, png_bytep pb, size_t st)
{
   while (st > 0)
   {
      size_t cb;
      png_uint_32 len, chunk;
      png_modification *mod;

      if (pm->buffer_position >= pm->buffer_count) switch (pm->state)
      {
         static png_byte sign[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
         case modifier_start:
            store_read_chunk(&pm->this, pm->buffer, 8, 8);  
            pm->buffer_count = 8;
            pm->buffer_position = 0;

            if (memcmp(pm->buffer, sign, 8) != 0)
               png_error(pm->this.pread, "invalid PNG file signature");
            pm->state = modifier_signature;
            break;

         case modifier_signature:
            store_read_chunk(&pm->this, pm->buffer, 13+12, 13+12);  
            pm->buffer_count = 13+12;
            pm->buffer_position = 0;

            if (png_get_uint_32(pm->buffer) != 13 ||
                png_get_uint_32(pm->buffer+4) != CHUNK_IHDR)
               png_error(pm->this.pread, "invalid IHDR");

             
            mod = pm->modifications;
            while (mod != NULL)
            {
               if (mod->chunk == CHUNK_IHDR && mod->modify_fn &&
                   (*mod->modify_fn)(pm, mod, 0))
                  {
                  mod->modified = 1;
                  modifier_setbuffer(pm);
                  }

                
               mod = mod->next;
            }

             
            pm->bit_depth = pm->buffer[8+8];
            pm->colour_type = pm->buffer[8+8+1];

            pm->state = modifier_IHDR;
            pm->flush = 0;
            break;

         case modifier_IHDR:
         default:
                                                                         
                                                                        
                                               
               
            if ((cb = pm->flush) > 0)
            {
               if (cb > st) cb = st;
               pm->flush -= cb;
               store_read_chunk(&pm->this, pb, cb, cb);
               pb += cb;
               st -= cb;
               if (st == 0) return;
            }

                                                                         
                     
               
            if (pm->pending_chunk != 0)
            {
               png_save_uint_32(pm->buffer, pm->pending_len);
               png_save_uint_32(pm->buffer+4, pm->pending_chunk);
               pm->pending_len = 0;
               pm->pending_chunk = 0;
            }
            else
               store_read_chunk(&pm->this, pm->buffer, 8, 8);

            pm->buffer_count = 8;
            pm->buffer_position = 0;

             
            len = png_get_uint_32(pm->buffer);
            chunk = png_get_uint_32(pm->buffer+4);

                                                                       
                     
               
            if (chunk == CHUNK_PLTE || chunk == CHUNK_IDAT ||
                chunk == CHUNK_IEND)
            {
               mod = pm->modifications;

               while (mod != NULL)
               {
                  if ((mod->add == chunk ||
                      (mod->add == CHUNK_PLTE && chunk == CHUNK_IDAT)) &&
                      mod->modify_fn != NULL && !mod->modified && !mod->added)
                  {
                                                                              
                                   
                        
                     mod->added = 1;

                     if ((*mod->modify_fn)(pm, mod, 1  ))
                     {
                         
                        if (pm->buffer_count > 0)
                           modifier_setbuffer(pm);

                        else
                           {
                           pm->buffer_position = 0;
                           mod->removed = 1;
                           }

                                                                                
                                                                   
                           
                        pm->pending_len = len;
                        pm->pending_chunk = chunk;
                        break;  
                     }
                  }

                  mod = mod->next;
               }

                                                                              
                                                                           
                             
                  
               if (mod != NULL)
                  break;  
            }

                                                                             
                                                                               
                                    
               
            if (len+12 <= sizeof pm->buffer)
            {
               size_t s = len+12-pm->buffer_count;
               store_read_chunk(&pm->this, pm->buffer+pm->buffer_count, s, s);
               pm->buffer_count = len+12;

                
               mod = pm->modifications;
               while (mod != NULL)
               {
                  if (mod->chunk == chunk)
                  {
                     if (mod->modify_fn == NULL)
                     {
                         
                        pm->buffer_count = pm->buffer_position = 0;
                        mod->removed = 1;
                        break;  
                     }

                     else if ((*mod->modify_fn)(pm, mod, 0))
                     {
                        mod->modified = 1;
                         
                        if (pm->buffer_count == 0)
                        {
                           pm->buffer_position = 0;
                           break;
                        }
                        modifier_setbuffer(pm);
                     }
                  }

                  mod = mod->next;
               }
            }

            else
               pm->flush = len+12 - pm->buffer_count;  

             
            break;
      }

                                                                 
                                                
         
      cb = pm->buffer_count - pm->buffer_position;

      if (cb > st)
         cb = st;

      memcpy(pb, pm->buffer + pm->buffer_position, cb);
      st -= cb;
      pb += cb;
      pm->buffer_position += cb;
   }
}

 
static void PNGCBAPI
modifier_read(png_structp ppIn, png_bytep pb, size_t st)
{
   png_const_structp pp = ppIn;
   png_modifier *pm = voidcast(png_modifier*, png_get_io_ptr(pp));

   if (pm == NULL || pm->this.pread != pp)
      png_error(pp, "bad modifier_read call");

   modifier_read_imp(pm, pb, st);
}

                                                                             
                       
   
static void
modifier_progressive_read(png_modifier *pm, png_structp pp, png_infop pi)
{
   if (pm->this.pread != pp || pm->this.current == NULL ||
       pm->this.next == NULL)
      png_error(pp, "store state damaged (progressive)");

                                                                             
                                                                              
                                                                             
                                                                            
                    
      
   for (;;)
   {
      static png_uint_32 noise = 1;
      size_t cb, cbAvail;
      png_byte buffer[512];

       
      noise = (noise << 9) | ((noise ^ (noise >> (9-5))) & 0x1ff);
      cb = noise & 0x1ff;

                                                                               
                                                                               
                                                                             
                                         
         
      cbAvail = store_read_buffer_avail(&pm->this);
      if (pm->buffer_count > pm->buffer_position)
         cbAvail += pm->buffer_count - pm->buffer_position;

      if (cb > cbAvail)
      {
          
         if (cbAvail == 0)
            break;

         cb = cbAvail;
      }

      modifier_read_imp(pm, buffer, cb);
      png_process_data(pp, pi, buffer, cb);
   }

                                                                           
            
      
   if (pm->buffer_count > pm->buffer_position ||
       pm->this.next != &pm->this.current->data ||
       pm->this.readpos < pm->this.current->datacount)
      png_error(pp, "progressive read implementation error");
}

 
static png_structp
set_modifier_for_read(png_modifier *pm, png_infopp ppi, png_uint_32 id,
    const char *name)
{
                                                                            
                                                                          
                          
      
   pm->state = modifier_start;
   pm->bit_depth = 0;
   pm->colour_type = 255;

   pm->pending_len = 0;
   pm->pending_chunk = 0;
   pm->flush = 0;
   pm->buffer_count = 0;
   pm->buffer_position = 0;

   return set_store_for_read(&pm->this, ppi, id, name);
}


 
                                                                             
                                                                             
                        
   
typedef struct gama_modification
{
   png_modification this;
   png_fixed_point  gamma;
} gama_modification;

static int
gama_modify(png_modifier *pm, png_modification *me, int add)
{
   UNUSED(add)
    
   png_save_uint_32(pm->buffer, 4);
   png_save_uint_32(pm->buffer+4, CHUNK_gAMA);
   png_save_uint_32(pm->buffer+8, ((gama_modification*)me)->gamma);
   return 1;
}

static void
gama_modification_init(gama_modification *me, png_modifier *pm, double gammad)
{
   double g;

   modification_init(&me->this);
   me->this.chunk = CHUNK_gAMA;
   me->this.modify_fn = gama_modify;
   me->this.add = CHUNK_PLTE;
   g = fix(gammad);
   me->gamma = (png_fixed_point)g;
   me->this.next = pm->modifications;
   pm->modifications = &me->this;
}

typedef struct chrm_modification
{
   png_modification          this;
   const color_encoding *encoding;
   png_fixed_point           wx, wy, rx, ry, gx, gy, bx, by;
} chrm_modification;

static int
chrm_modify(png_modifier *pm, png_modification *me, int add)
{
   UNUSED(add)
    
   png_save_uint_32(pm->buffer   , 32);
   png_save_uint_32(pm->buffer+ 4, CHUNK_cHRM);
   png_save_uint_32(pm->buffer+ 8, ((chrm_modification*)me)->wx);
   png_save_uint_32(pm->buffer+12, ((chrm_modification*)me)->wy);
   png_save_uint_32(pm->buffer+16, ((chrm_modification*)me)->rx);
   png_save_uint_32(pm->buffer+20, ((chrm_modification*)me)->ry);
   png_save_uint_32(pm->buffer+24, ((chrm_modification*)me)->gx);
   png_save_uint_32(pm->buffer+28, ((chrm_modification*)me)->gy);
   png_save_uint_32(pm->buffer+32, ((chrm_modification*)me)->bx);
   png_save_uint_32(pm->buffer+36, ((chrm_modification*)me)->by);
   return 1;
}

static void
chrm_modification_init(chrm_modification *me, png_modifier *pm,
   const color_encoding *encoding)
{
   CIE_color white = white_point(encoding);

    
   me->encoding = encoding;

    
   me->wx = fix(chromaticity_x(white));
   me->wy = fix(chromaticity_y(white));

   me->rx = fix(chromaticity_x(encoding->red));
   me->ry = fix(chromaticity_y(encoding->red));
   me->gx = fix(chromaticity_x(encoding->green));
   me->gy = fix(chromaticity_y(encoding->green));
   me->bx = fix(chromaticity_x(encoding->blue));
   me->by = fix(chromaticity_y(encoding->blue));

   modification_init(&me->this);
   me->this.chunk = CHUNK_cHRM;
   me->this.modify_fn = chrm_modify;
   me->this.add = CHUNK_PLTE;
   me->this.next = pm->modifications;
   pm->modifications = &me->this;
}

typedef struct srgb_modification
{
   png_modification this;
   png_byte         intent;
} srgb_modification;

static int
srgb_modify(png_modifier *pm, png_modification *me, int add)
{
   UNUSED(add)
    
   png_save_uint_32(pm->buffer, 1);
   png_save_uint_32(pm->buffer+4, CHUNK_sRGB);
   pm->buffer[8] = ((srgb_modification*)me)->intent;
   return 1;
}

static void
srgb_modification_init(srgb_modification *me, png_modifier *pm, png_byte intent)
{
   modification_init(&me->this);
   me->this.chunk = CHUNK_sBIT;

   if (intent <= 3)  
   {
      me->this.modify_fn = srgb_modify;
      me->this.add = CHUNK_PLTE;
      me->intent = intent;
   }

   else
   {
      me->this.modify_fn = 0;
      me->this.add = 0;
      me->intent = 0;
   }

   me->this.next = pm->modifications;
   pm->modifications = &me->this;
}

#ifdef PNG_READ_GAMMA_SUPPORTED
typedef struct sbit_modification
{
   png_modification this;
   png_byte         sbit;
} sbit_modification;

static int
sbit_modify(png_modifier *pm, png_modification *me, int add)
{
   png_byte sbit = ((sbit_modification*)me)->sbit;
   if (pm->bit_depth > sbit)
   {
      int cb = 0;
      switch (pm->colour_type)
      {
         case 0:
            cb = 1;
            break;

         case 2:
         case 3:
            cb = 3;
            break;

         case 4:
            cb = 2;
            break;

         case 6:
            cb = 4;
            break;

         default:
            png_error(pm->this.pread,
               "unexpected colour type in sBIT modification");
      }

      png_save_uint_32(pm->buffer, cb);
      png_save_uint_32(pm->buffer+4, CHUNK_sBIT);

      while (cb > 0)
         (pm->buffer+8)[--cb] = sbit;

      return 1;
   }
   else if (!add)
   {
       
      pm->buffer_count = pm->buffer_position = 0;
      return 1;
   }
   else
      return 0;  
}

static void
sbit_modification_init(sbit_modification *me, png_modifier *pm, png_byte sbit)
{
   modification_init(&me->this);
   me->this.chunk = CHUNK_sBIT;
   me->this.modify_fn = sbit_modify;
   me->this.add = CHUNK_PLTE;
   me->sbit = sbit;
   me->this.next = pm->modifications;
   pm->modifications = &me->this;
}
#endif  
#endif  

 
 
                                                                             
                                                                               
                                                                                
                                                                          
                                                                               
                                                                             
                                                     
  
                                                                            
                                      
  
                                                                               
                                                                               
                                                                         
  
                                                                             
                                                                            
                                                                              
                                                                             
  
                                                                                
                                                                               
                                                                               
                                  
  
                                                                           
                                                                                
                                                                              
                                                                           
                               
  
                                                                              
                                                                    
                                                                          
                                                                               
        
   

                                                                              
                                                                               
                                                                              
                                                                             
                                                                                
                                                                                
            
  
                                                                              
                                                                             
                                                                                
                                 
   
static store_palette_entry *
make_standard_palette(png_store* ps, int npalette, int do_tRNS)
{
   static png_uint_32 palette_seed[2] = { 0x87654321, 9 };

   int i = 0;
   png_byte values[256][4];

                                                                              
      
   for (; i<8; ++i)
   {
      values[i][1] = (png_byte)((i&1) ? 255U : 0U);
      values[i][2] = (png_byte)((i&2) ? 255U : 0U);
      values[i][3] = (png_byte)((i&4) ? 255U : 0U);
   }

    
   {
      int j = 0;
      png_byte random_bytes[4];
      png_byte need[256];

      need[0] = 0;  
      memset(need+1, 1, (sizeof need)-2);  
      need[255] = 0;  

      while (i<70)
      {
         png_byte b;

         if (j==0)
         {
            make_four_random_bytes(palette_seed, random_bytes);
            j = 4;
         }

         b = random_bytes[--j];
         if (need[b])
         {
            values[i][1] = b;
            values[i][2] = b;
            values[i++][3] = b;
         }
      }
   }

                                                                               
                                                                          
                                                                              
             
      
   for (; i<256; ++i)
      make_four_random_bytes(palette_seed, values[i]);

                                                                               
                                             
      
   {
      store_palette_entry *palette;
      png_byte selector[4];

      make_four_random_bytes(palette_seed, selector);

      if (do_tRNS)
         for (i=0; i<256; ++i)
            values[i][0] = (png_byte)(i ^ selector[0]);

      else
         for (i=0; i<256; ++i)
            values[i][0] = 255;  

                                                                        
                                                                             
                                                                              
                                                                            
                                                                            
                                                                        
                                                                                
                                                                            
         
      palette = store_write_palette(ps, npalette);

      for (i=0; i<npalette; ++i)
      {
         palette[i].alpha = values[i ^ selector[1]][0];
         palette[i].red   = values[i ^ selector[1]][1];
         palette[i].green = values[i ^ selector[1]][2];
         palette[i].blue  = values[i ^ selector[1]][3];
      }

      return palette;
   }
}

                                                                           
                                                       
   
 
static void
init_standard_palette(png_store *ps, png_structp pp, png_infop pi, int npalette,
   int do_tRNS)
{
   store_palette_entry *ppal = make_standard_palette(ps, npalette, do_tRNS);

   {
      int i;
      png_color palette[256];

       
      for (i=0; i<npalette; ++i)
      {
         palette[i].red = ppal[i].red;
         palette[i].green = ppal[i].green;
         palette[i].blue = ppal[i].blue;
      }

       
      for (; i<256; ++i)
         palette[i].red = palette[i].green = palette[i].blue = 42;

      png_set_PLTE(pp, pi, palette, npalette);
   }

   if (do_tRNS)
   {
      int i, j;
      png_byte tRNS[256];

       
      for (i=j=0; i<npalette; ++i)
         if ((tRNS[i] = ppal[i].alpha) < 255)
            j = i+1;

       
      for (; i<256; ++i)
         tRNS[i] = 24;

#ifdef PNG_WRITE_tRNS_SUPPORTED
      if (j > 0)
         png_set_tRNS(pp, pi, tRNS, j, 0 );
#endif
   }
}

#ifdef PNG_WRITE_tRNS_SUPPORTED
static void
set_random_tRNS(png_structp pp, png_infop pi, png_byte colour_type,
   int bit_depth)
{
                                                                           
                                                                              
                                                                             
                                                              
      
   png_color_16 tRNS;
   png_uint_16 mask = (png_uint_16)((1U << bit_depth)-1);

   R8(tRNS);  

   if (colour_type & 2 )
   {
      if (bit_depth == 8)
      {
         tRNS.red = random_u16();
         tRNS.green = random_u16();
         tRNS.blue = tRNS.red ^ tRNS.green;
         tRNS.red &= mask;
         tRNS.green &= mask;
         tRNS.blue &= mask;
      }

      else  
      {
         tRNS.red = random_u16();
         tRNS.green = (png_uint_16)(tRNS.red * 257);
         tRNS.blue = (png_uint_16)(tRNS.green * 17);
      }
   }

   else
   {
      tRNS.gray = random_u16();
      tRNS.gray &= mask;
   }

   png_set_tRNS(pp, pi, NULL, 0, &tRNS);
}
#endif

                                                                             
                                                                      
   
static int
npasses_from_interlace_type(png_const_structp pp, int interlace_type)
{
   switch (interlace_type)
   {
   default:
      png_error(pp, "invalid interlace type");

   case PNG_INTERLACE_NONE:
      return 1;

   case PNG_INTERLACE_ADAM7:
      return PNG_INTERLACE_ADAM7_PASSES;
   }
}

static unsigned int
bit_size(png_const_structp pp, png_byte colour_type, png_byte bit_depth)
{
   switch (colour_type)
   {
      default: png_error(pp, "invalid color type");

      case 0:  return bit_depth;

      case 2:  return 3*bit_depth;

      case 3:  return bit_depth;

      case 4:  return 2*bit_depth;

      case 6:  return 4*bit_depth;
   }
}

#define TRANSFORM_WIDTH  128U
#define TRANSFORM_ROWMAX (TRANSFORM_WIDTH*8U)
#define SIZE_ROWMAX (16*8U)  
#define STANDARD_ROWMAX TRANSFORM_ROWMAX  
#define SIZE_HEIGHTMAX 16  

static size_t
transform_rowsize(png_const_structp pp, png_byte colour_type,
   png_byte bit_depth)
{
   return (TRANSFORM_WIDTH * bit_size(pp, colour_type, bit_depth)) / 8;
}

                                                                              
                                   
   
#define transform_width(pp, colour_type, bit_depth) TRANSFORM_WIDTH

static png_uint_32
transform_height(png_const_structp pp, png_byte colour_type, png_byte bit_depth)
{
   switch (bit_size(pp, colour_type, bit_depth))
   {
      case 1:
      case 2:
      case 4:
         return 1;    

      case 8:
         return 2;    

      case 16:
         return 512;  

      case 24:
      case 32:
         return 512;  

      case 48:
      case 64:
         return 2048; 
#        define TRANSFORM_HEIGHTMAX 2048

      default:
         return 0;    
   }
}

#ifdef PNG_READ_SUPPORTED
                                                                      
                                
   
static png_uint_32
standard_width(png_const_structp pp, png_uint_32 id)
{
   png_uint_32 width = WIDTH_FROM_ID(id);
   UNUSED(pp)

   if (width == 0)
      width = transform_width(pp, COL_FROM_ID(id), DEPTH_FROM_ID(id));

   return width;
}

static png_uint_32
standard_height(png_const_structp pp, png_uint_32 id)
{
   png_uint_32 height = HEIGHT_FROM_ID(id);

   if (height == 0)
      height = transform_height(pp, COL_FROM_ID(id), DEPTH_FROM_ID(id));

   return height;
}

static png_uint_32
standard_rowsize(png_const_structp pp, png_uint_32 id)
{
   png_uint_32 width = standard_width(pp, id);

    
   width *= bit_size(pp, COL_FROM_ID(id), DEPTH_FROM_ID(id));
   return (width + 7) / 8;
}
#endif  

static void
transform_row(png_const_structp pp, png_byte buffer[TRANSFORM_ROWMAX],
   png_byte colour_type, png_byte bit_depth, png_uint_32 y)
{
   png_uint_32 v = y << 7;
   png_uint_32 i = 0;

   switch (bit_size(pp, colour_type, bit_depth))
   {
      case 1:
         while (i<128/8) buffer[i] = (png_byte)(v & 0xff), v += 17, ++i;
         return;

      case 2:
         while (i<128/4) buffer[i] = (png_byte)(v & 0xff), v += 33, ++i;
         return;

      case 4:
         while (i<128/2) buffer[i] = (png_byte)(v & 0xff), v += 65, ++i;
         return;

      case 8:
          
         while (i<128) buffer[i] = (png_byte)(v & 0xff), ++v, ++i;
         return;

      case 16:
                                                                              
                                                 
            
         while (i<128)
         {
            buffer[2*i] = (png_byte)((v>>8) & 0xff);
            buffer[2*i+1] = (png_byte)(v & 0xff);
            ++v;
            ++i;
         }

         return;

      case 24:
          
         while (i<128)
         {
             
            buffer[3*i+0] = (png_byte)((v >> 8) & 0xff);
            buffer[3*i+1] = (png_byte)(v & 0xff);
            buffer[3*i+2] = (png_byte)(((v >> 8) ^ v) & 0xff);
            ++v;
            ++i;
         }

         return;

      case 32:
          
         while (i<128)
         {
            buffer[4*i+0] = (png_byte)((v >> 8) & 0xff);
            buffer[4*i+1] = (png_byte)(v & 0xff);
            buffer[4*i+2] = (png_byte)((v >> 8) & 0xff);
            buffer[4*i+3] = (png_byte)(v & 0xff);
            ++v;
            ++i;
         }

         return;

      case 48:
                                                                               
                                                                    
            
         while (i<128)
         {
            png_uint_32 t = v++;
            buffer[6*i+0] = (png_byte)((t >> 8) & 0xff);
            buffer[6*i+1] = (png_byte)(t & 0xff);
            t *= 257;
            buffer[6*i+2] = (png_byte)((t >> 8) & 0xff);
            buffer[6*i+3] = (png_byte)(t & 0xff);
            t *= 17;
            buffer[6*i+4] = (png_byte)((t >> 8) & 0xff);
            buffer[6*i+5] = (png_byte)(t & 0xff);
            ++i;
         }

         return;

      case 64:
          
         while (i<128)
         {
            png_uint_32 t = v++;
            buffer[8*i+0] = (png_byte)((t >> 8) & 0xff);
            buffer[8*i+1] = (png_byte)(t & 0xff);
            buffer[8*i+4] = (png_byte)((t >> 8) & 0xff);
            buffer[8*i+5] = (png_byte)(t & 0xff);
            t *= 257;
            buffer[8*i+2] = (png_byte)((t >> 8) & 0xff);
            buffer[8*i+3] = (png_byte)(t & 0xff);
            buffer[8*i+6] = (png_byte)((t >> 8) & 0xff);
            buffer[8*i+7] = (png_byte)(t & 0xff);
            ++i;
         }
         return;

      default:
         break;
   }

   png_error(pp, "internal error");
}

                                                                              
                                   
   
#define DEPTH(bd) ((png_byte)(1U << (bd)))

                                                                       
                                                                                
                        
   
#ifdef PNG_WRITE_INTERLACING_SUPPORTED
#  define INTERLACE_LAST PNG_INTERLACE_LAST
#  define check_interlace_type(type) ((void)(type))
#  define set_write_interlace_handling(pp,type) png_set_interlace_handling(pp)
#  define do_own_interlace 0
#elif PNG_LIBPNG_VER < 10700
#  define set_write_interlace_handling(pp,type) (1)
static void
check_interlace_type(int const interlace_type)
{
                                                                             
                                                                           
                                                    
      
   if (interlace_type != PNG_INTERLACE_NONE)
   {
                                                                             
                   
         
      fprintf(stderr, "pngvalid: no interlace support\n");
      exit(99);
   }
}
#  define INTERLACE_LAST (PNG_INTERLACE_NONE+1)
#  define do_own_interlace 0
#else  
#  define set_write_interlace_handling(pp,type)\
      npasses_from_interlace_type(pp,type)
#  define check_interlace_type(type) ((void)(type))
#  define INTERLACE_LAST PNG_INTERLACE_LAST
#  define do_own_interlace 1
#endif  

#if PNG_LIBPNG_VER >= 10700 || defined PNG_WRITE_INTERLACING_SUPPORTED
#   define CAN_WRITE_INTERLACE 1
#else
#   define CAN_WRITE_INTERLACE 0
#endif

                                                                              
                                        
   
#ifdef PNG_READ_INTERLACING_SUPPORTED
#  define do_read_interlace 0
#else  
#  define do_read_interlace 1
#endif
                                                                       
                                          
   
static void
interlace_row(png_bytep buffer, png_const_bytep imageRow,
   unsigned int pixel_size, png_uint_32 w, int pass, int littleendian)
{
   png_uint_32 xin, xout, xstep;

                                                                            
                                                                              
                                                                            
     
                                                                           
                                        
      
   xin = PNG_PASS_START_COL(pass);
   xstep = 1U<<PNG_PASS_COL_SHIFT(pass);

   for (xout=0; xin<w; xin+=xstep)
   {
      pixel_copy(buffer, xout, imageRow, xin, pixel_size, littleendian);
      ++xout;
   }
}

#ifdef PNG_READ_SUPPORTED
static void
deinterlace_row(png_bytep buffer, png_const_bytep row,
   unsigned int pixel_size, png_uint_32 w, int pass, int littleendian)
{
                                                                             
                                                                             
                                                                           
                          
      
   png_uint_32 xin, xout, xstep;

   xout = PNG_PASS_START_COL(pass);
   xstep = 1U<<PNG_PASS_COL_SHIFT(pass);

   for (xin=0; xout<w; xout+=xstep)
   {
      pixel_copy(buffer, xout, row, xin, pixel_size, littleendian);
      ++xin;
   }
}
#endif  

                                                                      
                                                                       
                                                                         
                                                                       
                                                                
   
#ifdef PNG_WRITE_FILTER_SUPPORTED
static void
choose_random_filter(png_structp pp, int start)
{
                                                                           
                                                
      
   int filters = PNG_ALL_FILTERS & random_mod(256U);

    
   if (filters != 0)
   {
      if (start && filters < PNG_FILTER_UP)
         filters |= PNG_FILTER_UP;

      png_set_filter(pp, 0 , filters);
   }
}
#else  
#  define choose_random_filter(pp, start) ((void)0)
#endif  

static void
make_transform_image(png_store* const ps, png_byte const colour_type,
    png_byte const bit_depth, unsigned int palette_number,
    int interlace_type, png_const_charp name)
{
   context(ps, fault);

   check_interlace_type(interlace_type);

   Try
   {
      png_infop pi;
      png_structp pp = set_store_for_write(ps, &pi, name);
      png_uint_32 h, w;

                                                                              
                                                                                
               
         
      if (pp == NULL)
         Throw ps;

      w = transform_width(pp, colour_type, bit_depth);
      h = transform_height(pp, colour_type, bit_depth);

      png_set_IHDR(pp, pi, w, h, bit_depth, colour_type, interlace_type,
         PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

#ifdef PNG_TEXT_SUPPORTED
#  if defined(PNG_READ_zTXt_SUPPORTED) && defined(PNG_WRITE_zTXt_SUPPORTED)
#     define TEXT_COMPRESSION PNG_TEXT_COMPRESSION_zTXt
#  else
#     define TEXT_COMPRESSION PNG_TEXT_COMPRESSION_NONE
#  endif
      {
         static char key[] = "image name";  
         size_t pos;
         png_text text;
         char copy[FILE_NAME_SIZE];

                                                                                
                                             
            
         text.compression = TEXT_COMPRESSION;
         text.key = key;
          
         pos = safecat(copy, sizeof copy, 0, ps->wname);
         text.text = copy;
         text.text_length = pos;
         text.itxt_length = 0;
         text.lang = 0;
         text.lang_key = 0;

         png_set_text(pp, pi, &text, 1);
      }
#endif

      if (colour_type == 3)  
         init_standard_palette(ps, pp, pi, 1U << bit_depth, 1 );

#     ifdef PNG_WRITE_tRNS_SUPPORTED
         else if (palette_number)
            set_random_tRNS(pp, pi, colour_type, bit_depth);
#     endif

      png_write_info(pp, pi);

      if (png_get_rowbytes(pp, pi) !=
          transform_rowsize(pp, colour_type, bit_depth))
         png_error(pp, "transform row size incorrect");

      else
      {
                                                                           
                                                                               
                                                    
            
         int npasses = set_write_interlace_handling(pp, interlace_type);
         int pass;

         if (npasses != npasses_from_interlace_type(pp, interlace_type))
            png_error(pp, "write: png_set_interlace_handling failed");

         for (pass=0; pass<npasses; ++pass)
         {
            png_uint_32 y;

                                                                             
                                                                   
               
            for (y=0; y<h; ++y)
            {
               png_byte buffer[TRANSFORM_ROWMAX];

               transform_row(pp, buffer, colour_type, bit_depth, y);

#              if do_own_interlace
                                                                                
                                                                         
                     
                  if (interlace_type == PNG_INTERLACE_ADAM7)
                  {
                                                                               
                                                                                
                                                                           
                                                                                
                                                                          
                        
                     if (PNG_ROW_IN_INTERLACE_PASS(y, pass) &&
                         PNG_PASS_COLS(w, pass) > 0)
                        interlace_row(buffer, buffer,
                              bit_size(pp, colour_type, bit_depth), w, pass,
                              0 );
                     else
                        continue;
                  }
#              endif  

               choose_random_filter(pp, pass == 0 && y == 0);
               png_write_row(pp, buffer);
            }
         }
      }

#ifdef PNG_TEXT_SUPPORTED
      {
         static char key[] = "end marker";
         static char comment[] = "end";
         png_text text;

                                                                                
                                             
            
         text.compression = TEXT_COMPRESSION;
         text.key = key;
         text.text = comment;
         text.text_length = (sizeof comment)-1;
         text.itxt_length = 0;
         text.lang = 0;
         text.lang_key = 0;

         png_set_text(pp, pi, &text, 1);
      }
#endif

      png_write_end(pp, pi);

       
      store_storefile(ps, FILEID(colour_type, bit_depth, palette_number,
         interlace_type, 0, 0, 0));

      store_write_reset(ps);
   }

   Catch(fault)
   {
                                                                                
                                                                                
                                              
         
      store_write_reset(fault);
   }
}

static void
make_transform_images(png_modifier *pm)
{
   png_byte colour_type = 0;
   png_byte bit_depth = 0;
   unsigned int palette_number = 0;

    
   safecat(pm->this.test, sizeof pm->this.test, 0, "make standard images");

                                                                          
                                                                             
                                                           
      
   while (next_format(&colour_type, &bit_depth, &palette_number, 1, 1))
   {
      int interlace_type;

      for (interlace_type = PNG_INTERLACE_NONE;
           interlace_type < INTERLACE_LAST; ++interlace_type)
      {
         char name[FILE_NAME_SIZE];

         standard_name(name, sizeof name, 0, colour_type, bit_depth,
            palette_number, interlace_type, 0, 0, do_own_interlace);
         make_transform_image(&pm->this, colour_type, bit_depth, palette_number,
            interlace_type, name);
      }
   }
}

                                                                        
                                          
   
static void
size_row(png_byte buffer[SIZE_ROWMAX], png_uint_32 bit_width, png_uint_32 y)
{
    
   y = ((y & 1) << 7) + ((y & 2) << 6) + ((y & 4) << 5) + ((y & 8) << 4);
    
   y ^= 0xA5;

   while (bit_width >= 8)
      *buffer++ = (png_byte)y++, bit_width -= 8;

                                                                           
                       
      
   if (bit_width > 0)
   {
      png_uint_32 mask = (1U<<(8-bit_width))-1;
      *buffer = (png_byte)((*buffer & mask) | (y & ~mask));
   }
}

static void
make_size_image(png_store* const ps, png_byte const colour_type,
    png_byte const bit_depth, int const interlace_type,
    png_uint_32 const w, png_uint_32 const h,
    int const do_interlace)
{
   context(ps, fault);

   check_interlace_type(interlace_type);

   Try
   {
      png_infop pi;
      png_structp pp;
      unsigned int pixel_size;

       
      char name[FILE_NAME_SIZE];
      png_uint_32 id = FILEID(colour_type, bit_depth, 0 ,
         interlace_type, w, h, do_interlace);

      standard_name_from_id(name, sizeof name, 0, id);
      pp = set_store_for_write(ps, &pi, name);

                                                                              
                                                                                
               
         
      if (pp == NULL)
         Throw ps;

      png_set_IHDR(pp, pi, w, h, bit_depth, colour_type, interlace_type,
         PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

#ifdef PNG_TEXT_SUPPORTED
      {
         static char key[] = "image name";  
         size_t pos;
         png_text text;
         char copy[FILE_NAME_SIZE];

                                                                                
                                             
            
         text.compression = TEXT_COMPRESSION;
         text.key = key;
          
         pos = safecat(copy, sizeof copy, 0, ps->wname);
         text.text = copy;
         text.text_length = pos;
         text.itxt_length = 0;
         text.lang = 0;
         text.lang_key = 0;

         png_set_text(pp, pi, &text, 1);
      }
#endif

      if (colour_type == 3)  
         init_standard_palette(ps, pp, pi, 1U << bit_depth, 0 );

      png_write_info(pp, pi);

                                                                              
                                                                            
                                                       
         
      pixel_size = bit_size(pp, colour_type, bit_depth);
      if (png_get_rowbytes(pp, pi) != ((w * pixel_size) + 7) / 8)
         png_error(pp, "size row size incorrect");

      else
      {
         int npasses = npasses_from_interlace_type(pp, interlace_type);
         png_uint_32 y;
         int pass;
         png_byte image[16][SIZE_ROWMAX];

                                                                            
                                          
            
         memset(image, 0xff, sizeof image);

         if (!do_interlace &&
             npasses != set_write_interlace_handling(pp, interlace_type))
            png_error(pp, "write: png_set_interlace_handling failed");

          
         for (y=0; y<h; ++y)
            size_row(image[y], w * pixel_size, y);

         for (pass=0; pass<npasses; ++pass)
         {
             
            png_uint_32 wPass = PNG_PASS_COLS(w, pass);

                                                                           
                                                                             
                                      
               
            for (y=0; y<h; ++y)
            {
               png_const_bytep row = image[y];
               png_byte tempRow[SIZE_ROWMAX];

                                                                  
                                                                   
                           
                  
               if (do_interlace && interlace_type == PNG_INTERLACE_ADAM7)
               {
                                                                            
                                                                             
                                                                        
                                                                             
                                                                       
                     
                  if (PNG_ROW_IN_INTERLACE_PASS(y, pass) && wPass > 0)
                  {
                                                                           
                                               
                        
                     memset(tempRow, 0xff, sizeof tempRow);
                     interlace_row(tempRow, row, pixel_size, w, pass,
                           0 );
                     row = tempRow;
                  }
                  else
                     continue;
               }

#           ifdef PNG_WRITE_FILTER_SUPPORTED
                                                                            
                                                                             
                                                                              
                                                                             
                       
                 
                                                                               
                                                                             
                                                                  
                  
               {
                  int filters = 8 << random_mod(PNG_FILTER_VALUE_LAST);

                  if (pass == 0 && y == 0 &&
                      (filters < PNG_FILTER_UP || w == 1U))
                     filters |= PNG_FILTER_UP;

                  png_set_filter(pp, 0 , filters);
               }
#           endif

               png_write_row(pp, row);
            }
         }
      }

#ifdef PNG_TEXT_SUPPORTED
      {
         static char key[] = "end marker";
         static char comment[] = "end";
         png_text text;

                                                                                
                                             
            
         text.compression = TEXT_COMPRESSION;
         text.key = key;
         text.text = comment;
         text.text_length = (sizeof comment)-1;
         text.itxt_length = 0;
         text.lang = 0;
         text.lang_key = 0;

         png_set_text(pp, pi, &text, 1);
      }
#endif

      png_write_end(pp, pi);

       
      store_storefile(ps, id);

      store_write_reset(ps);
   }

   Catch(fault)
   {
                                                                                
                                                                                
                                              
         
      store_write_reset(fault);
   }
}

static void
make_size(png_store* const ps, png_byte const colour_type, int bdlo,
    int const bdhi)
{
   for (; bdlo <= bdhi; ++bdlo)
   {
      png_uint_32 width;

      for (width = 1; width <= 16; ++width)
      {
         png_uint_32 height;

         for (height = 1; height <= 16; ++height)
         {
                                                                            
                                                                          
                               
               
            make_size_image(ps, colour_type, DEPTH(bdlo), PNG_INTERLACE_NONE,
               width, height, 0);
            make_size_image(ps, colour_type, DEPTH(bdlo), PNG_INTERLACE_NONE,
               width, height, 1);
#        ifdef PNG_WRITE_INTERLACING_SUPPORTED
            make_size_image(ps, colour_type, DEPTH(bdlo), PNG_INTERLACE_ADAM7,
               width, height, 0);
#        endif
#        if CAN_WRITE_INTERLACE
                                                                               
                                                         
               
            make_size_image(ps, colour_type, DEPTH(bdlo), PNG_INTERLACE_ADAM7,
               width, height, 1);
#        endif
         }
      }
   }
}

static void
make_size_images(png_store *ps)
{
    
   safecat(ps->test, sizeof ps->test, 0, "make size images");

                                                              
      
   make_size(ps, 0, 0, WRITE_BDHI);
   make_size(ps, 2, 3, WRITE_BDHI);
   make_size(ps, 3, 0, 3  );
   make_size(ps, 4, 3, WRITE_BDHI);
   make_size(ps, 6, 3, WRITE_BDHI);
}

#ifdef PNG_READ_SUPPORTED
 
static void
standard_row(png_const_structp pp, png_byte std[STANDARD_ROWMAX],
   png_uint_32 id, png_uint_32 y)
{
   if (WIDTH_FROM_ID(id) == 0)
      transform_row(pp, std, COL_FROM_ID(id), DEPTH_FROM_ID(id), y);
   else
      size_row(std, WIDTH_FROM_ID(id) * bit_size(pp, COL_FROM_ID(id),
         DEPTH_FROM_ID(id)), y);
}
#endif  

 
                                                                             
                                                                           
                             
   
                                                               
                                                  
   
#ifdef PNG_WARNINGS_SUPPORTED
static void
sBIT0_error_fn(png_structp pp, png_infop pi)
{
    
   png_color_8 bad;
   bad.red = bad.green = bad.blue = bad.gray = bad.alpha = 0;
   png_set_sBIT(pp, pi, &bad);
}

static void
sBIT_error_fn(png_structp pp, png_infop pi)
{
   png_byte bit_depth;
   png_color_8 bad;

   if (png_get_color_type(pp, pi) == PNG_COLOR_TYPE_PALETTE)
      bit_depth = 8;

   else
      bit_depth = png_get_bit_depth(pp, pi);

    
   bad.red = bad.green = bad.blue = bad.gray = bad.alpha =
      (png_byte)(bit_depth+1);
   png_set_sBIT(pp, pi, &bad);
}

static const struct
{
   void          (*fn)(png_structp, png_infop);
   const char *msg;
   unsigned int    warning :1;  
} error_test[] =
    {
        
       { sBIT0_error_fn, "sBIT(0): failed to detect error",
         PNG_LIBPNG_VER < 10700 },

       { sBIT_error_fn, "sBIT(too big): failed to detect error",
         PNG_LIBPNG_VER < 10700 },
    };

static void
make_error(png_store* const ps, png_byte const colour_type,
    png_byte bit_depth, int interlace_type, int test, png_const_charp name)
{
   context(ps, fault);

   check_interlace_type(interlace_type);

   Try
   {
      png_infop pi;
      png_structp pp = set_store_for_write(ps, &pi, name);
      png_uint_32 w, h;
      gnu_volatile(pp)

      if (pp == NULL)
         Throw ps;

      w = transform_width(pp, colour_type, bit_depth);
      gnu_volatile(w)
      h = transform_height(pp, colour_type, bit_depth);
      gnu_volatile(h)
      png_set_IHDR(pp, pi, w, h, bit_depth, colour_type, interlace_type,
            PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

      if (colour_type == 3)  
         init_standard_palette(ps, pp, pi, 1U << bit_depth, 0 );

                                                                         
                                                             
         
#     define exception__prev exception_prev_1
#     define exception__env exception_env_1
      Try
      {
         gnu_volatile(exception__prev)

          
         ps->expect_error = !error_test[test].warning;
         ps->expect_warning = error_test[test].warning;
         ps->saw_warning = 0;
         error_test[test].fn(pp, pi);

          
         png_write_info(pp, pi);

          
         if (ps->expect_warning && ps->saw_warning)
            Throw ps;

                                                                            
                                                                       
            
         store_log(ps, pp, error_test[test].msg, 1  );
      }

      Catch (fault)
      {  
      }
#undef exception__prev
#undef exception__env

       
      ps->expect_warning = 0;

      if (ps->expect_error)
         ps->expect_error = 0;

      else
      {
                                                                              
                                                                           
                                                                                
                                              
            
         if (png_get_rowbytes(pp, pi) !=
             transform_rowsize(pp, colour_type, bit_depth))
            png_error(pp, "row size incorrect");

         else
         {
            int npasses = set_write_interlace_handling(pp, interlace_type);
            int pass;

            if (npasses != npasses_from_interlace_type(pp, interlace_type))
               png_error(pp, "write: png_set_interlace_handling failed");

            for (pass=0; pass<npasses; ++pass)
            {
               png_uint_32 y;

               for (y=0; y<h; ++y)
               {
                  png_byte buffer[TRANSFORM_ROWMAX];

                  transform_row(pp, buffer, colour_type, bit_depth, y);

#                 if do_own_interlace
                                                                            
                                                                            
                              
                        
                     if (interlace_type == PNG_INTERLACE_ADAM7)
                     {
                                                                           
                                                                                
                                                                             
                                                                               
                                                                              
                                        
                           
                        if (PNG_ROW_IN_INTERLACE_PASS(y, pass) &&
                            PNG_PASS_COLS(w, pass) > 0)
                           interlace_row(buffer, buffer,
                                 bit_size(pp, colour_type, bit_depth), w, pass,
                                 0 );
                        else
                           continue;
                     }
#                 endif  

                  png_write_row(pp, buffer);
               }
            }
         }  

         png_write_end(pp, pi);
      }

       
      store_write_reset(ps);
   }

   Catch(fault)
   {
      store_write_reset(fault);
   }
}

static int
make_errors(png_modifier* const pm, png_byte const colour_type,
    int bdlo, int const bdhi)
{
   for (; bdlo <= bdhi; ++bdlo)
   {
      int interlace_type;

      for (interlace_type = PNG_INTERLACE_NONE;
           interlace_type < INTERLACE_LAST; ++interlace_type)
      {
         unsigned int test;
         char name[FILE_NAME_SIZE];

         standard_name(name, sizeof name, 0, colour_type, 1<<bdlo, 0,
            interlace_type, 0, 0, do_own_interlace);

         for (test=0; test<ARRAY_SIZE(error_test); ++test)
         {
            make_error(&pm->this, colour_type, DEPTH(bdlo), interlace_type,
               test, name);

            if (fail(pm))
               return 0;
         }
      }
   }

   return 1;  
}
#endif  

static void
perform_error_test(png_modifier *pm)
{
#ifdef PNG_WARNINGS_SUPPORTED  
    
   safecat(pm->this.test, sizeof pm->this.test, 0, "error test");

   if (!make_errors(pm, 0, 0, WRITE_BDHI))
      return;

   if (!make_errors(pm, 2, 3, WRITE_BDHI))
      return;

   if (!make_errors(pm, 3, 0, 3))
      return;

   if (!make_errors(pm, 4, 3, WRITE_BDHI))
      return;

   if (!make_errors(pm, 6, 3, WRITE_BDHI))
      return;
#else
   UNUSED(pm)
#endif
}

                                                                            
                                                                          
   
static void
perform_formatting_test(png_store *ps)
{
#ifdef PNG_TIME_RFC1123_SUPPORTED
                                                                                
                                      
      
   context(ps, fault);

   Try
   {
      png_const_charp correct = "29 Aug 2079 13:53:60 +0000";
      png_const_charp result;
#     if PNG_LIBPNG_VER >= 10600
         char timestring[29];
#     endif
      png_structp pp;
      png_time pt;

      pp = set_store_for_write(ps, NULL, "libpng formatting test");

      if (pp == NULL)
         Throw ps;


       
      pt.year = 2079;
      pt.month = 8;
      pt.day = 29;
      pt.hour = 13;
      pt.minute = 53;
      pt.second = 60;  

#     if PNG_LIBPNG_VER < 10600
         result = png_convert_to_rfc1123(pp, &pt);
#     else
         if (png_convert_to_rfc1123_buffer(timestring, &pt))
            result = timestring;

         else
            result = NULL;
#     endif

      if (result == NULL)
         png_error(pp, "png_convert_to_rfc1123 failed");

      if (strcmp(result, correct) != 0)
      {
         size_t pos = 0;
         char msg[128];

         pos = safecat(msg, sizeof msg, pos, "png_convert_to_rfc1123(");
         pos = safecat(msg, sizeof msg, pos, correct);
         pos = safecat(msg, sizeof msg, pos, ") returned: '");
         pos = safecat(msg, sizeof msg, pos, result);
         pos = safecat(msg, sizeof msg, pos, "'");

         png_error(pp, msg);
      }

      store_write_reset(ps);
   }

   Catch(fault)
   {
      store_write_reset(fault);
   }
#else
   UNUSED(ps)
#endif
}

#ifdef PNG_READ_SUPPORTED
                                                                              
                                                                               
                                                                                
                                                                            
                                                      
  
                                                                      
                                                                                
                                                                               
                                                                           
                                                                            
                                                                            
                                                                            
                                                                               
                                                       
  
                                                                      
                                                                               
                                                                        
                                                                              
                                                                           
                                                                          
                    
   
typedef struct standard_display
{
   png_store*  ps;              
   png_byte    colour_type;
   png_byte    bit_depth;
   png_byte    red_sBIT;        
   png_byte    green_sBIT;
   png_byte    blue_sBIT;
   png_byte    alpha_sBIT;
   png_byte    interlace_type;
   png_byte    filler;          
   png_uint_32 id;              
   png_uint_32 w;               
   png_uint_32 h;               
   int         npasses;         
   png_uint_32 pixel_size;      
   png_uint_32 bit_width;       
   size_t      cbRow;           
   int         do_interlace;    
   int         littleendian;    
   int         is_transparent;  
   int         has_tRNS;        
   int         speed;           
   int         use_update_info; 
   struct
   {
      png_uint_16 red;
      png_uint_16 green;
      png_uint_16 blue;
   }           transparent;     
   int         npalette;        
   store_palette
               palette;
} standard_display;

static void
standard_display_init(standard_display *dp, png_store* ps, png_uint_32 id,
   int do_interlace, int use_update_info)
{
   memset(dp, 0, sizeof *dp);

   dp->ps = ps;
   dp->colour_type = COL_FROM_ID(id);
   dp->bit_depth = DEPTH_FROM_ID(id);
   if (dp->bit_depth < 1 || dp->bit_depth > 16)
      internal_error(ps, "internal: bad bit depth");
   if (dp->colour_type == 3)
      dp->red_sBIT = dp->blue_sBIT = dp->green_sBIT = dp->alpha_sBIT = 8;
   else
      dp->red_sBIT = dp->blue_sBIT = dp->green_sBIT = dp->alpha_sBIT =
         dp->bit_depth;
   dp->interlace_type = INTERLACE_FROM_ID(id);
   check_interlace_type(dp->interlace_type);
   dp->id = id;
    
   dp->w = 0;
   dp->h = 0;
   dp->npasses = 0;
   dp->pixel_size = 0;
   dp->bit_width = 0;
   dp->cbRow = 0;
   dp->do_interlace = do_interlace;
   dp->littleendian = 0;
   dp->is_transparent = 0;
   dp->speed = ps->speed;
   dp->use_update_info = use_update_info;
   dp->npalette = 0;
    
   memset(&dp->transparent, 0, sizeof dp->transparent);
    
   memset(dp->palette, 0xff, sizeof dp->palette);
}

                                                                              
                                                             
   
static void
standard_palette_init(standard_display *dp)
{
   store_palette_entry *palette = store_current_palette(dp->ps, &dp->npalette);

    
   if (dp->npalette > 0)
   {
      int i = dp->npalette;
      memcpy(dp->palette, palette, i * sizeof *palette);

       
      while (--i >= 0)
         if (palette[i].alpha < 255)
            break;

#     ifdef __GNUC__
          
         if (i >= 0)
            dp->is_transparent = 1;
         else
            dp->is_transparent = 0;
#     else
         dp->is_transparent = (i >= 0);
#     endif
   }
}

                                                                    
                                                                            
                                                                                
   
static int
read_palette(store_palette palette, int *npalette, png_const_structp pp,
   png_infop pi)
{
   png_colorp pal;
   png_bytep trans_alpha;
   int num;

   pal = 0;
   *npalette = -1;

   if (png_get_PLTE(pp, pi, &pal, npalette) & PNG_INFO_PLTE)
   {
      int i = *npalette;

      if (i <= 0 || i > 256)
         png_error(pp, "validate: invalid PLTE count");

      while (--i >= 0)
      {
         palette[i].red = pal[i].red;
         palette[i].green = pal[i].green;
         palette[i].blue = pal[i].blue;
      }

                                                                        
                                                            
         
      memset(palette + *npalette, 126, (256-*npalette) * sizeof *palette);
   }

   else  
   {
      if (*npalette != (-1))
         png_error(pp, "validate: invalid PLTE result");
       
      *npalette = 0;
      memset(palette, 113, sizeof (store_palette));
   }

   trans_alpha = 0;
   num = 2;  
   if ((png_get_tRNS(pp, pi, &trans_alpha, &num, 0) & PNG_INFO_tRNS) != 0 &&
      (trans_alpha != NULL || num != 1 ) &&
                                                                                
                                                                               
                                                                                
                                       
         
      !(trans_alpha != NULL && num == 0))  
   {
      int i;

                                                                    
                                                                        
                                                                          
                                                                        
                                                                    
         
      if (trans_alpha == NULL || num <= 0 || num > 256 || num > *npalette)
         png_error(pp, "validate: unexpected png_get_tRNS (palette) result");

      for (i=0; i<num; ++i)
         palette[i].alpha = trans_alpha[i];

      for (num=*npalette; i<num; ++i)
         palette[i].alpha = 255;

      for (; i<256; ++i)
         palette[i].alpha = 33;  

      return 1;  
   }

   else
   {
       
      int i;

      for (i=0, num=*npalette; i<num; ++i)
         palette[i].alpha = 255;

      for (; i<256; ++i)
         palette[i].alpha = 55;  

      return 0;  
   }
}

                                                                     
                       
   
static void
standard_palette_validate(standard_display *dp, png_const_structp pp,
   png_infop pi)
{
   int npalette;
   store_palette palette;

   if (read_palette(palette, &npalette, pp, pi) != dp->is_transparent)
      png_error(pp, "validate: palette transparency changed");

   if (npalette != dp->npalette)
   {
      size_t pos = 0;
      char msg[64];

      pos = safecat(msg, sizeof msg, pos, "validate: palette size changed: ");
      pos = safecatn(msg, sizeof msg, pos, dp->npalette);
      pos = safecat(msg, sizeof msg, pos, " -> ");
      pos = safecatn(msg, sizeof msg, pos, npalette);
      png_error(pp, msg);
   }

   {
      int i = npalette;  

      while (--i >= 0)
         if (palette[i].red != dp->palette[i].red ||
            palette[i].green != dp->palette[i].green ||
            palette[i].blue != dp->palette[i].blue ||
            palette[i].alpha != dp->palette[i].alpha)
            png_error(pp, "validate: PLTE or tRNS chunk changed");
   }
}

                                                                        
                                                                         
                                                                       
  
                                                                               
                                                                               
                                                                            
   
static void
standard_info_part1(standard_display *dp, png_structp pp, png_infop pi)
{
   if (png_get_bit_depth(pp, pi) != dp->bit_depth)
      png_error(pp, "validate: bit depth changed");

   if (png_get_color_type(pp, pi) != dp->colour_type)
      png_error(pp, "validate: color type changed");

   if (png_get_filter_type(pp, pi) != PNG_FILTER_TYPE_BASE)
      png_error(pp, "validate: filter type changed");

   if (png_get_interlace_type(pp, pi) != dp->interlace_type)
      png_error(pp, "validate: interlacing changed");

   if (png_get_compression_type(pp, pi) != PNG_COMPRESSION_TYPE_BASE)
      png_error(pp, "validate: compression type changed");

   dp->w = png_get_image_width(pp, pi);

   if (dp->w != standard_width(pp, dp->id))
      png_error(pp, "validate: image width changed");

   dp->h = png_get_image_height(pp, pi);

   if (dp->h != standard_height(pp, dp->id))
      png_error(pp, "validate: image height changed");

                                                                                
                       
      
   {
      png_color_8p sBIT = 0;

      if (png_get_sBIT(pp, pi, &sBIT) & PNG_INFO_sBIT)
      {
         int sBIT_invalid = 0;

         if (sBIT == 0)
            png_error(pp, "validate: unexpected png_get_sBIT result");

         if (dp->colour_type & PNG_COLOR_MASK_COLOR)
         {
            if (sBIT->red == 0 || sBIT->red > dp->bit_depth)
               sBIT_invalid = 1;
            else
               dp->red_sBIT = sBIT->red;

            if (sBIT->green == 0 || sBIT->green > dp->bit_depth)
               sBIT_invalid = 1;
            else
               dp->green_sBIT = sBIT->green;

            if (sBIT->blue == 0 || sBIT->blue > dp->bit_depth)
               sBIT_invalid = 1;
            else
               dp->blue_sBIT = sBIT->blue;
         }

         else  
         {
            if (sBIT->gray == 0 || sBIT->gray > dp->bit_depth)
               sBIT_invalid = 1;
            else
               dp->blue_sBIT = dp->green_sBIT = dp->red_sBIT = sBIT->gray;
         }

                                                                            
                 
            
         if (dp->colour_type & PNG_COLOR_MASK_ALPHA)
         {
            if (sBIT->alpha == 0 || sBIT->alpha > dp->bit_depth)
               sBIT_invalid = 1;
            else
               dp->alpha_sBIT = sBIT->alpha;
         }

         if (sBIT_invalid)
            png_error(pp, "validate: sBIT value out of range");
      }
   }

                                                                               
                                                                              
                                                                                
                           
      
   if (png_get_rowbytes(pp, pi) != standard_rowsize(pp, dp->id))
      png_error(pp, "validate: row size changed");

                                                                            
             
      
   standard_palette_validate(dp, pp, pi);

                                                                       
                                                                               
                            
      
   {
      png_color_16p trans_color = 0;

      if (png_get_tRNS(pp, pi, 0, 0, &trans_color) & PNG_INFO_tRNS)
      {
         if (trans_color == 0)
            png_error(pp, "validate: unexpected png_get_tRNS (color) result");

         switch (dp->colour_type)
         {
         case 0:
            dp->transparent.red = dp->transparent.green = dp->transparent.blue =
               trans_color->gray;
            dp->has_tRNS = 1;
            break;

         case 2:
            dp->transparent.red = trans_color->red;
            dp->transparent.green = trans_color->green;
            dp->transparent.blue = trans_color->blue;
            dp->has_tRNS = 1;
            break;

         case 3:
                                                                      
                     
               
            png_error(pp, "validate: unexpected png_get_tRNS result");
            break;

         default:
            png_error(pp, "validate: invalid tRNS chunk with alpha image");
         }
      }
   }

                                                                       
                                                                          
                                                                 
      
   dp->npasses = npasses_from_interlace_type(pp, dp->interlace_type);
   if (!dp->do_interlace)
   {
#     ifdef PNG_READ_INTERLACING_SUPPORTED
         if (dp->npasses != png_set_interlace_handling(pp))
            png_error(pp, "validate: file changed interlace type");
#     else  
                                                                               
                       
            
         if (dp->npasses > 1)
            png_error(pp, "validate: no libpng interlace support");
#     endif  
   }

                                                                               
            
      
}

                                                                               
                                                                               
         
   
static void
standard_info_part2(standard_display *dp, png_const_structp pp,
    png_const_infop pi, int nImages)
{
    
   {
      png_byte ct = png_get_color_type(pp, pi);
      png_byte bd = png_get_bit_depth(pp, pi);

      if (bd >= 8 && (ct == PNG_COLOR_TYPE_RGB || ct == PNG_COLOR_TYPE_GRAY) &&
          dp->filler)
          ct |= 4;  

      dp->pixel_size = bit_size(pp, ct, bd);
   }
   dp->bit_width = png_get_image_width(pp, pi) * dp->pixel_size;
   dp->cbRow = png_get_rowbytes(pp, pi);

    
   if (dp->cbRow != (dp->bit_width+7)/8)
      png_error(pp, "bad png_get_rowbytes calculation");

    
   store_ensure_image(dp->ps, pp, nImages, dp->cbRow, dp->h);
}

static void
standard_info_imp(standard_display *dp, png_structp pp, png_infop pi,
    int nImages)
{
                                                                        
                                                
      
   standard_info_part1(dp, pp, pi);

                                                                           
                                                      
      
   if (dp->use_update_info)
   {
       
      int i = dp->use_update_info;
      while (i-- > 0)
         png_read_update_info(pp, pi);
   }

   else
      png_start_read_image(pp);

                                                                                
                                    
      
   standard_info_part2(dp, pp, pi, nImages);
}

static void PNGCBAPI
standard_info(png_structp pp, png_infop pi)
{
   standard_display *dp = voidcast(standard_display*,
      png_get_progressive_ptr(pp));

                                                                              
            
      
   standard_info_imp(dp, pp, pi, 1  );
}

static void PNGCBAPI
progressive_row(png_structp ppIn, png_bytep new_row, png_uint_32 y, int pass)
{
   png_const_structp pp = ppIn;
   const standard_display *dp = voidcast(standard_display*,
      png_get_progressive_ptr(pp));

                                                                          
                                                                           
                                                                             
                                                        
      
   if (new_row != NULL)
   {
      png_bytep row;

                                                                       
                                   
         
      if (dp->do_interlace && dp->interlace_type == PNG_INTERLACE_ADAM7)
      {
#ifdef PNG_USER_TRANSFORM_INFO_SUPPORTED
          
         if (y != png_get_current_row_number(pp))
            png_error(pp, "png_get_current_row_number is broken");

         if (pass != png_get_current_pass_number(pp))
            png_error(pp, "png_get_current_pass_number is broken");
#endif  

         y = PNG_ROW_FROM_PASS_ROW(y, pass);
      }

       
      if (y >= dp->h)
         png_error(pp, "invalid y to progressive row callback");

      row = store_image_row(dp->ps, pp, 0, y);

       
#ifdef PNG_READ_INTERLACING_SUPPORTED
      if (dp->do_interlace)
#endif  
      {
         if (dp->interlace_type == PNG_INTERLACE_ADAM7)
            deinterlace_row(row, new_row, dp->pixel_size, dp->w, pass,
                  dp->littleendian);
         else
            row_copy(row, new_row, dp->pixel_size * dp->w, dp->littleendian);
      }
#ifdef PNG_READ_INTERLACING_SUPPORTED
      else
         png_progressive_combine_row(pp, row, new_row);
#endif  
   }

   else if (dp->interlace_type == PNG_INTERLACE_ADAM7 &&
       PNG_ROW_IN_INTERLACE_PASS(y, pass) &&
       PNG_PASS_COLS(dp->w, pass) > 0)
      png_error(pp, "missing row in progressive de-interlacing");
}

static void
sequential_row(standard_display *dp, png_structp pp, png_infop pi,
    int iImage, int iDisplay)
{
   int npasses = dp->npasses;
   int do_interlace = dp->do_interlace &&
      dp->interlace_type == PNG_INTERLACE_ADAM7;
   png_uint_32 height = standard_height(pp, dp->id);
   png_uint_32 width = standard_width(pp, dp->id);
   const png_store* ps = dp->ps;
   int pass;

   for (pass=0; pass<npasses; ++pass)
   {
      png_uint_32 y;
      png_uint_32 wPass = PNG_PASS_COLS(width, pass);

      for (y=0; y<height; ++y)
      {
         if (do_interlace)
         {
                                                                     
                                                              
               
            if (wPass > 0 && PNG_ROW_IN_INTERLACE_PASS(y, pass))
            {
                                                                            
                                                  
                  
               png_byte row[STANDARD_ROWMAX], display[STANDARD_ROWMAX];

                                                                              
                                                                               
                                                                                
                                                                            
                                                              
                  
               memset(row, 0xc5, sizeof row);
               memset(display, 0x5c, sizeof display);

               png_read_row(pp, row, display);

               if (iImage >= 0)
                  deinterlace_row(store_image_row(ps, pp, iImage, y), row,
                     dp->pixel_size, dp->w, pass, dp->littleendian);

               if (iDisplay >= 0)
                  deinterlace_row(store_image_row(ps, pp, iDisplay, y), display,
                     dp->pixel_size, dp->w, pass, dp->littleendian);
            }
         }
         else
            png_read_row(pp,
               iImage >= 0 ? store_image_row(ps, pp, iImage, y) : NULL,
               iDisplay >= 0 ? store_image_row(ps, pp, iDisplay, y) : NULL);
      }
   }

                                                                              
                                                                           
      
   png_read_end(pp, pi);
}

#ifdef PNG_TEXT_SUPPORTED
static void
standard_check_text(png_const_structp pp, png_const_textp tp,
   png_const_charp keyword, png_const_charp text)
{
   char msg[1024];
   size_t pos = safecat(msg, sizeof msg, 0, "text: ");
   size_t ok;

   pos = safecat(msg, sizeof msg, pos, keyword);
   pos = safecat(msg, sizeof msg, pos, ": ");
   ok = pos;

   if (tp->compression != TEXT_COMPRESSION)
   {
      char buf[64];

      sprintf(buf, "compression [%d->%d], ", TEXT_COMPRESSION,
         tp->compression);
      pos = safecat(msg, sizeof msg, pos, buf);
   }

   if (tp->key == NULL || strcmp(tp->key, keyword) != 0)
   {
      pos = safecat(msg, sizeof msg, pos, "keyword \"");
      if (tp->key != NULL)
      {
         pos = safecat(msg, sizeof msg, pos, tp->key);
         pos = safecat(msg, sizeof msg, pos, "\", ");
      }

      else
         pos = safecat(msg, sizeof msg, pos, "null, ");
   }

   if (tp->text == NULL)
      pos = safecat(msg, sizeof msg, pos, "text lost, ");

   else
   {
      if (tp->text_length != strlen(text))
      {
         char buf[64];
         sprintf(buf, "text length changed[%lu->%lu], ",
            (unsigned long)strlen(text), (unsigned long)tp->text_length);
         pos = safecat(msg, sizeof msg, pos, buf);
      }

      if (strcmp(tp->text, text) != 0)
      {
         pos = safecat(msg, sizeof msg, pos, "text becomes \"");
         pos = safecat(msg, sizeof msg, pos, tp->text);
         pos = safecat(msg, sizeof msg, pos, "\" (was \"");
         pos = safecat(msg, sizeof msg, pos, text);
         pos = safecat(msg, sizeof msg, pos, "\"), ");
      }
   }

   if (tp->itxt_length != 0)
      pos = safecat(msg, sizeof msg, pos, "iTXt length set, ");

   if (tp->lang != NULL)
   {
      pos = safecat(msg, sizeof msg, pos, "iTXt language \"");
      pos = safecat(msg, sizeof msg, pos, tp->lang);
      pos = safecat(msg, sizeof msg, pos, "\", ");
   }

   if (tp->lang_key != NULL)
   {
      pos = safecat(msg, sizeof msg, pos, "iTXt keyword \"");
      pos = safecat(msg, sizeof msg, pos, tp->lang_key);
      pos = safecat(msg, sizeof msg, pos, "\", ");
   }

   if (pos > ok)
   {
      msg[pos-2] = '\0';  
      png_error(pp, msg);
   }
}

static void
standard_text_validate(standard_display *dp, png_const_structp pp,
   png_infop pi, int check_end)
{
   png_textp tp = NULL;
   png_uint_32 num_text = png_get_text(pp, pi, &tp, NULL);

   if (num_text == 2 && tp != NULL)
   {
      standard_check_text(pp, tp, "image name", dp->ps->current->name);

                                                                            
                                                                           
                                                        
         
      if (check_end)
         standard_check_text(pp, tp+1, "end marker", "end");
   }

   else
   {
      char msg[64];

      sprintf(msg, "expected two text items, got %lu",
         (unsigned long)num_text);
      png_error(pp, msg);
   }
}
#else
#  define standard_text_validate(dp,pp,pi,check_end) ((void)0)
#endif

static void
standard_row_validate(standard_display *dp, png_const_structp pp,
   int iImage, int iDisplay, png_uint_32 y)
{
   int where;
   png_byte std[STANDARD_ROWMAX];

                                                                           
                    
      
   memset(std, 178, sizeof std);
   standard_row(pp, std, dp->id, y);

                                                                             
                                                                               
                                                                      
                                                                             
      
   if (iImage >= 0 &&
      (where = pixel_cmp(std, store_image_row(dp->ps, pp, iImage, y),
            dp->bit_width)) != 0)
   {
      char msg[64];
      sprintf(msg, "PNG image row[%lu][%d] changed from %.2x to %.2x",
         (unsigned long)y, where-1, std[where-1],
         store_image_row(dp->ps, pp, iImage, y)[where-1]);
      png_error(pp, msg);
   }

   if (iDisplay >= 0 &&
      (where = pixel_cmp(std, store_image_row(dp->ps, pp, iDisplay, y),
         dp->bit_width)) != 0)
   {
      char msg[64];
      sprintf(msg, "display row[%lu][%d] changed from %.2x to %.2x",
         (unsigned long)y, where-1, std[where-1],
         store_image_row(dp->ps, pp, iDisplay, y)[where-1]);
      png_error(pp, msg);
   }
}

static void
standard_image_validate(standard_display *dp, png_const_structp pp, int iImage,
    int iDisplay)
{
   png_uint_32 y;

   if (iImage >= 0)
      store_image_check(dp->ps, pp, iImage);

   if (iDisplay >= 0)
      store_image_check(dp->ps, pp, iDisplay);

   for (y=0; y<dp->h; ++y)
      standard_row_validate(dp, pp, iImage, iDisplay, y);

    
   dp->ps->validated = 1;
}

static void PNGCBAPI
standard_end(png_structp ppIn, png_infop pi)
{
   png_const_structp pp = ppIn;
   standard_display *dp = voidcast(standard_display*,
      png_get_progressive_ptr(pp));

   UNUSED(pi)

                                                                            
                        
      
   standard_text_validate(dp, pp, pi,
      PNG_LIBPNG_VER >= 10518 );
   standard_image_validate(dp, pp, 0, -1);
}

 
static void
standard_test(png_store* const psIn, png_uint_32 const id,
   int do_interlace, int use_update_info)
{
   standard_display d;
   context(psIn, fault);

                                                                          
                                                                  
      
   standard_display_init(&d, psIn, id, do_interlace, use_update_info);

                                                                        
                                            
      
   Try
   {
      png_structp pp;
      png_infop pi;

                                                                               
                                                     
         
      pp = set_store_for_read(d.ps, &pi, d.id,
         d.do_interlace ?  (d.ps->progressive ?
            "pngvalid progressive deinterlacer" :
            "pngvalid sequential deinterlacer") : (d.ps->progressive ?
               "progressive reader" : "sequential reader"));

       
      standard_palette_init(&d);

       
      if (d.ps->progressive)
      {
         png_set_progressive_read_fn(pp, &d, standard_info, progressive_row,
            standard_end);

          
         store_progressive_read(d.ps, pp, pi);
      }
      else
      {
          
         png_set_read_fn(pp, d.ps, store_read);

          
         png_read_info(pp, pi);

                                                                          
                               
            
         standard_info_imp(&d, pp, pi, 2  );

                                                                               
                                                                             
                   
            
         {
            sequential_row(&d, pp, pi, 0, 1);

                                                                             
                                
               
            if (!d.speed)
            {
               standard_text_validate(&d, pp, pi, 1 );
               standard_image_validate(&d, pp, 0, 1);
            }
            else
               d.ps->validated = 1;
         }
      }

       
      if (!d.ps->validated)
         png_error(pp, "image read failed silently");

       
   }

   Catch(fault)
      d.ps = fault;  

    
   store_read_reset(d.ps);
}

static int
test_standard(png_modifier* const pm, png_byte const colour_type,
    int bdlo, int const bdhi)
{
   for (; bdlo <= bdhi; ++bdlo)
   {
      int interlace_type;

      for (interlace_type = PNG_INTERLACE_NONE;
           interlace_type < INTERLACE_LAST; ++interlace_type)
      {
         standard_test(&pm->this, FILEID(colour_type, DEPTH(bdlo), 0 ,
            interlace_type, 0, 0, 0), do_read_interlace, pm->use_update_info);

         if (fail(pm))
            return 0;
      }
   }

   return 1;  
}

static void
perform_standard_test(png_modifier *pm)
{
                                                                            
                                                                     
      
   if (!test_standard(pm, 0, 0, READ_BDHI))
      return;

   if (!test_standard(pm, 2, 3, READ_BDHI))
      return;

   if (!test_standard(pm, 3, 0, 3))
      return;

   if (!test_standard(pm, 4, 3, READ_BDHI))
      return;

   if (!test_standard(pm, 6, 3, READ_BDHI))
      return;
}


 
static int
test_size(png_modifier* const pm, png_byte const colour_type,
    int bdlo, int const bdhi)
{
                                        
     
                                                               
                                                                    
                                                                       
                          
      
   static const png_byte hinc[] = {1, 3, 11, 1, 5};
   static const png_byte winc[] = {1, 9, 5, 7, 1};
   int save_bdlo = bdlo;

   for (; bdlo <= bdhi; ++bdlo)
   {
      png_uint_32 h, w;

      for (h=1; h<=16; h+=hinc[bdlo]) for (w=1; w<=16; w+=winc[bdlo])
      {
                                                                   
                                                                      
                                                                             
                        
            
         standard_test(&pm->this, FILEID(colour_type, DEPTH(bdlo), 0 ,
            PNG_INTERLACE_NONE, w, h, 0), 0 ,
            pm->use_update_info);

         if (fail(pm))
            return 0;

         standard_test(&pm->this, FILEID(colour_type, DEPTH(bdlo), 0 ,
            PNG_INTERLACE_NONE, w, h, 1), 0 ,
            pm->use_update_info);

         if (fail(pm))
            return 0;

                                                                      
                                                                        
                                                            
            
         standard_test(&pm->this, FILEID(colour_type, DEPTH(bdlo), 0 ,
            PNG_INTERLACE_NONE, w, h, 0), 1 ,
            pm->use_update_info);

         if (fail(pm))
            return 0;

#     if CAN_WRITE_INTERLACE
          
         standard_test(&pm->this, FILEID(colour_type, DEPTH(bdlo), 0 ,
            PNG_INTERLACE_ADAM7, w, h, 1), 1 ,
            pm->use_update_info);

         if (fail(pm))
            return 0;
#     endif
      }
   }

                                                                            
                                      
      
   for (bdlo = save_bdlo; bdlo <= bdhi; ++bdlo)
   {
      png_uint_32 h, w;

      for (h=1; h<=16; h+=hinc[bdlo]) for (w=1; w<=16; w+=winc[bdlo])
      {
#     ifdef PNG_READ_INTERLACING_SUPPORTED
                                                                         
                                                                            
                                                                                
                            
            
#     if CAN_WRITE_INTERLACE
         standard_test(&pm->this, FILEID(colour_type, DEPTH(bdlo), 0 ,
            PNG_INTERLACE_ADAM7, w, h, 1), 0 ,
            pm->use_update_info);

         if (fail(pm))
            return 0;
#     endif
#     endif  

#     ifdef PNG_WRITE_INTERLACING_SUPPORTED
          
         standard_test(&pm->this, FILEID(colour_type, DEPTH(bdlo), 0 ,
            PNG_INTERLACE_ADAM7, w, h, 0), 1 ,
            pm->use_update_info);

         if (fail(pm))
            return 0;
#     endif

#     ifdef PNG_READ_INTERLACING_SUPPORTED
#     ifdef PNG_WRITE_INTERLACING_SUPPORTED
          
         standard_test(&pm->this, FILEID(colour_type, DEPTH(bdlo), 0 ,
            PNG_INTERLACE_ADAM7, w, h, 0), 0 ,
            pm->use_update_info);

         if (fail(pm))
            return 0;
#     endif
#     endif  
      }
   }

   return 1;  
}

static void
perform_size_test(png_modifier *pm)
{
                                                                            
                                                                     
      
   if (!test_size(pm, 0, 0, READ_BDHI))
      return;

   if (!test_size(pm, 2, 3, READ_BDHI))
      return;

                                                                          
                                     
      
#if 0
   if (!test_size(pm, 3, 0, 3))
      return;
#endif

   if (!test_size(pm, 4, 3, READ_BDHI))
      return;

   if (!test_size(pm, 6, 3, READ_BDHI))
      return;
}


 
#ifdef PNG_READ_TRANSFORMS_SUPPORTED
                                                                              
                                                                       
                                                                               
                                
   
typedef struct image_pixel
{
                                                                  
                    
      
   unsigned int red, green, blue, alpha;  
   unsigned int palette_index;            
   png_byte     colour_type;              
   png_byte     bit_depth;                
   png_byte     sample_depth;             
   unsigned int have_tRNS :1;             
   unsigned int swap_rgb :1;              
   unsigned int alpha_first :1;           
   unsigned int alpha_inverted :1;        
   unsigned int mono_inverted :1;         
   unsigned int swap16 :1;                
   unsigned int littleendian :1;          
   unsigned int sig_bits :1;              

                                                                             
                                                                             
                                                                              
                                                                               
                                                                                
                                                                   
     
                                                                              
                                                                               
      
   double   redf, greenf, bluef, alphaf;
   double   rede, greene, bluee, alphae;
   png_byte red_sBIT, green_sBIT, blue_sBIT, alpha_sBIT;
} image_pixel;

 
static void
image_pixel_setf(image_pixel *this, unsigned int rMax, unsigned int gMax,
        unsigned int bMax, unsigned int aMax)
{
   this->redf = this->red / (double)rMax;
   this->greenf = this->green / (double)gMax;
   this->bluef = this->blue / (double)bMax;
   this->alphaf = this->alpha / (double)aMax;

   if (this->red < rMax)
      this->rede = this->redf * DBL_EPSILON;
   else
      this->rede = 0;
   if (this->green < gMax)
      this->greene = this->greenf * DBL_EPSILON;
   else
      this->greene = 0;
   if (this->blue < bMax)
      this->bluee = this->bluef * DBL_EPSILON;
   else
      this->bluee = 0;
   if (this->alpha < aMax)
      this->alphae = this->alphaf * DBL_EPSILON;
   else
      this->alphae = 0;
}

                                                                           
                                                                            
         
   
static void
image_pixel_init(image_pixel *this, png_const_bytep row, png_byte colour_type,
    png_byte bit_depth, png_uint_32 x, store_palette palette,
    const image_pixel *format  )
{
   png_byte sample_depth =
      (png_byte)(colour_type == PNG_COLOR_TYPE_PALETTE ? 8 : bit_depth);
   unsigned int max = (1U<<sample_depth)-1;
   int swap16 = (format != 0 && format->swap16);
   int littleendian = (format != 0 && format->littleendian);
   int sig_bits = (format != 0 && format->sig_bits);

                                                                               
                                                                                
                 
      
   this->palette_index = this->red = this->green = this->blue =
      sample(row, colour_type, bit_depth, x, 0, swap16, littleendian);
   this->alpha = max;
   this->red_sBIT = this->green_sBIT = this->blue_sBIT = this->alpha_sBIT =
      sample_depth;

    
   if (colour_type == 3)  
   {
       
      if (palette != 0)
      {
         unsigned int i = this->palette_index;

         this->red = palette[i].red;
         this->green = palette[i].green;
         this->blue = palette[i].blue;
         this->alpha = palette[i].alpha;
      }
   }

   else  
   {
      unsigned int i = 0;

      if ((colour_type & 4) != 0 && format != 0 && format->alpha_first)
      {
         this->alpha = this->red;
          
         this->palette_index = this->red = this->green = this->blue =
            sample(row, colour_type, bit_depth, x, 1, swap16, littleendian);
         i = 1;
      }

      if (colour_type & 2)
      {
          
         this->green = sample(row, colour_type, bit_depth, x, ++i, swap16,
                 littleendian);

         if (format != 0 && format->swap_rgb)  
             this->red = sample(row, colour_type, bit_depth, x, ++i, swap16,
                     littleendian);
         else
             this->blue = sample(row, colour_type, bit_depth, x, ++i, swap16,
                     littleendian);
      }

      else   if (format != 0 && format->mono_inverted)
         this->red = this->green = this->blue = this->red ^ max;

      if ((colour_type & 4) != 0)  
      {
         if (format == 0 || !format->alpha_first)
             this->alpha = sample(row, colour_type, bit_depth, x, ++i, swap16,
                     littleendian);

         if (format != 0 && format->alpha_inverted)
            this->alpha ^= max;
      }
   }

                                                                         
                                                                              
                           
      
   image_pixel_setf(this,
      sig_bits ? (1U << format->red_sBIT)-1 : max,
      sig_bits ? (1U << format->green_sBIT)-1 : max,
      sig_bits ? (1U << format->blue_sBIT)-1 : max,
      sig_bits ? (1U << format->alpha_sBIT)-1 : max);

                                                                        
                             
      
   this->colour_type = colour_type;
   this->bit_depth = bit_depth;
   this->sample_depth = sample_depth;
   this->have_tRNS = 0;
   this->swap_rgb = 0;
   this->alpha_first = 0;
   this->alpha_inverted = 0;
   this->mono_inverted = 0;
   this->swap16 = 0;
   this->littleendian = 0;
   this->sig_bits = 0;
}

#if defined PNG_READ_EXPAND_SUPPORTED || defined PNG_READ_GRAY_TO_RGB_SUPPORTED\
   || defined PNG_READ_EXPAND_SUPPORTED || defined PNG_READ_EXPAND_16_SUPPORTED\
   || defined PNG_READ_BACKGROUND_SUPPORTED
                                                                               
                                                                             
                                                                             
                                                                     
                                                                                
                                                    
   
static void
image_pixel_convert_PLTE(image_pixel *this)
{
   if (this->colour_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (this->have_tRNS)
      {
         this->colour_type = PNG_COLOR_TYPE_RGB_ALPHA;
         this->have_tRNS = 0;
      }
      else
         this->colour_type = PNG_COLOR_TYPE_RGB;

                                                                                
                                                                  
         
      this->bit_depth = 8;
   }
}

                                                                              
                                                                           
                                                                            
                                                                               
                                                                          
                
   
static void
image_pixel_add_alpha(image_pixel *this, const standard_display *display,
   int for_background)
{
   if (this->colour_type == PNG_COLOR_TYPE_PALETTE)
      image_pixel_convert_PLTE(this);

   if ((this->colour_type & PNG_COLOR_MASK_ALPHA) == 0)
   {
      if (this->colour_type == PNG_COLOR_TYPE_GRAY)
      {
#        if PNG_LIBPNG_VER < 10700
            if (!for_background && this->bit_depth < 8)
               this->bit_depth = this->sample_depth = 8;
#        endif

         if (this->have_tRNS)
         {
                                                                              
                                                  
               
#           if PNG_LIBPNG_VER >= 10700
               if (!for_background && this->bit_depth < 8)
                  this->bit_depth = this->sample_depth = 8;
#           endif

            this->have_tRNS = 0;

                                                                        
                                              
               
            if (this->red == display->transparent.red)
               this->alphaf = 0;
            else
               this->alphaf = 1;
         }
         else
            this->alphaf = 1;

         this->colour_type = PNG_COLOR_TYPE_GRAY_ALPHA;
      }

      else if (this->colour_type == PNG_COLOR_TYPE_RGB)
      {
         if (this->have_tRNS)
         {
            this->have_tRNS = 0;

                                                                               
                     
               
            if (this->red == display->transparent.red &&
               this->green == display->transparent.green &&
               this->blue == display->transparent.blue)
               this->alphaf = 0;
            else
               this->alphaf = 1;
         }
         else
            this->alphaf = 1;

         this->colour_type = PNG_COLOR_TYPE_RGB_ALPHA;
      }

                                                                         
                                                                                
         
      this->alphae = 0;
      this->alpha_sBIT = display->alpha_sBIT;
   }
}
#endif  

struct transform_display;
typedef struct image_transform
{
    
   const char *name;

    
   int enable;

    
   struct image_transform *const list;

                                                                               
            
      
   unsigned int global_use;

    
   unsigned int local_use;

                                                                           
                                                              
      
   const struct image_transform *next;

                                                                         
                                          
     
                                                                             
                                                                   
      
   void (*ini)(const struct image_transform *this,
      struct transform_display *that);

                                                                         
      
   void (*set)(const struct image_transform *this,
      struct transform_display *that, png_structp pp, png_infop pi);

                                                                             
                                                                         
                                                                          
                                                                              
                                              
      
   void (*mod)(const struct image_transform *this, image_pixel *that,
      png_const_structp pp, const struct transform_display *display);

                                                                        
                                                                       
                                                                          
                       
      
   int (*add)(struct image_transform *this,
      const struct image_transform **that, png_byte colour_type,
      png_byte bit_depth);
} image_transform;

typedef struct transform_display
{
   standard_display this;

    
   png_modifier*              pm;
   const image_transform* transform_list;
   unsigned int max_gamma_8;

    
   png_byte output_colour_type;
   png_byte output_bit_depth;
   png_byte unpacked;

    
   gama_modification gama_mod;
   chrm_modification chrm_mod;
   srgb_modification srgb_mod;
} transform_display;

 
static void
transform_set_encoding(transform_display *this)
{
                                                                               
                                
      
   png_modifier *pm = this->pm;

   modifier_set_encoding(pm);

   if (modifier_color_encoding_is_set(pm))
   {
      if (modifier_color_encoding_is_sRGB(pm))
         srgb_modification_init(&this->srgb_mod, pm, PNG_sRGB_INTENT_ABSOLUTE);

      else
      {
          
         gama_modification_init(&this->gama_mod, pm, pm->current_gamma);

         if (pm->current_encoding != 0)
            chrm_modification_init(&this->chrm_mod, pm, pm->current_encoding);
      }
   }
}

 
static void
image_transform_ini_end(const image_transform *this,
   transform_display *that)
{
   UNUSED(this)
   UNUSED(that)
}

static void
image_transform_set_end(const image_transform *this,
   transform_display *that, png_structp pp, png_infop pi)
{
   UNUSED(this)
   UNUSED(that)
   UNUSED(pp)
   UNUSED(pi)
}

                                                                           
                                                               
   
static unsigned int
sample_scale(double sample_value, unsigned int scale)
{
   sample_value = floor(sample_value * scale + .5);

    
   if (!(sample_value > 0))
      sample_value = 0;
   else if (sample_value > scale)
      sample_value = scale;

   return (unsigned int)sample_value;
}

static void
image_transform_mod_end(const image_transform *this, image_pixel *that,
    png_const_structp pp, const transform_display *display)
{
   unsigned int scale = (1U<<that->sample_depth)-1;
   int sig_bits = that->sig_bits;

   UNUSED(this)
   UNUSED(pp)
   UNUSED(display)

                                                                              
                                               
     
                                                                         
                                                                     
                                                                              
      
   that->red = sample_scale(that->redf, scale);

                                                                               
                                                                            
                                                                     
      
   if (sig_bits && that->red_sBIT < that->sample_depth)
      that->red >>= that->sample_depth - that->red_sBIT;

                                                                            
                                                                      
                                                                               
                                                                                
                                                                             
                                                                               
                                                                                
                                                                               
                                                                             
      
   that->rede += 1./(2*((1U<<that->red_sBIT)-1));

   if (that->colour_type & PNG_COLOR_MASK_COLOR)
   {
      that->green = sample_scale(that->greenf, scale);
      if (sig_bits && that->green_sBIT < that->sample_depth)
         that->green >>= that->sample_depth - that->green_sBIT;

      that->blue = sample_scale(that->bluef, scale);
      if (sig_bits && that->blue_sBIT < that->sample_depth)
         that->blue >>= that->sample_depth - that->blue_sBIT;

      that->greene += 1./(2*((1U<<that->green_sBIT)-1));
      that->bluee += 1./(2*((1U<<that->blue_sBIT)-1));
   }
   else
   {
      that->blue = that->green = that->red;
      that->bluef = that->greenf = that->redf;
      that->bluee = that->greene = that->rede;
   }

   if ((that->colour_type & PNG_COLOR_MASK_ALPHA) ||
      that->colour_type == PNG_COLOR_TYPE_PALETTE)
   {
      that->alpha = sample_scale(that->alphaf, scale);
      that->alphae += 1./(2*((1U<<that->alpha_sBIT)-1));
   }
   else
   {
      that->alpha = scale;  
      that->alphaf = 1;     
      that->alphae = 0;     
   }

   if (sig_bits && that->alpha_sBIT < that->sample_depth)
      that->alpha >>= that->sample_depth - that->alpha_sBIT;
}

 
static image_transform image_transform_end =
{
   "(end)",  
   1,  
   0,  
   0,  
   0,  
   0,  
   image_transform_ini_end,
   image_transform_set_end,
   image_transform_mod_end,
   0  
};

                                                                            
        
   
static void
transform_display_init(transform_display *dp, png_modifier *pm, png_uint_32 id,
    const image_transform *transform_list)
{
   memset(dp, 0, sizeof *dp);

    
   standard_display_init(&dp->this, &pm->this, id, do_read_interlace,
      pm->use_update_info);

    
   dp->pm = pm;
   dp->transform_list = transform_list;
   dp->max_gamma_8 = 16;

    
   dp->output_colour_type = 255;  
   dp->output_bit_depth = 255;   
   dp->unpacked = 0;  
}

static void
transform_info_imp(transform_display *dp, png_structp pp, png_infop pi)
{
    
   standard_info_part1(&dp->this, pp, pi);

    
   dp->transform_list->set(dp->transform_list, dp, pp, pi);

    
   {
      int i = dp->this.use_update_info;
       
      do
         png_read_update_info(pp, pi);
      while (--i > 0);
   }

    
   standard_info_part2(&dp->this, pp, pi, 1 );

    
   dp->output_colour_type = png_get_color_type(pp, pi);
   dp->output_bit_depth = png_get_bit_depth(pp, pi);

                                                                               
                                         
      
   if (dp->output_bit_depth >= 8 &&
       (dp->output_colour_type == PNG_COLOR_TYPE_RGB ||
        dp->output_colour_type == PNG_COLOR_TYPE_GRAY) && dp->this.filler)
       dp->output_colour_type |= 4;

                                                                               
                                                                           
                    
      
   switch (dp->output_colour_type)
   {
   case PNG_COLOR_TYPE_PALETTE:
      if (dp->output_bit_depth > 8) goto error;
       
   case PNG_COLOR_TYPE_GRAY:
      if (dp->output_bit_depth == 1 || dp->output_bit_depth == 2 ||
         dp->output_bit_depth == 4)
         break;
       
   default:
      if (dp->output_bit_depth == 8 || dp->output_bit_depth == 16)
         break;
       
   error:
      {
         char message[128];
         size_t pos;

         pos = safecat(message, sizeof message, 0,
            "invalid final bit depth: colour type(");
         pos = safecatn(message, sizeof message, pos, dp->output_colour_type);
         pos = safecat(message, sizeof message, pos, ") with bit depth: ");
         pos = safecatn(message, sizeof message, pos, dp->output_bit_depth);

         png_error(pp, message);
      }
   }

                                                                            
                                                                                
                                 
      
   {
      image_pixel test_pixel;

      memset(&test_pixel, 0, sizeof test_pixel);
      test_pixel.colour_type = dp->this.colour_type;  
      test_pixel.bit_depth = dp->this.bit_depth;
      if (test_pixel.colour_type == PNG_COLOR_TYPE_PALETTE)
         test_pixel.sample_depth = 8;
      else
         test_pixel.sample_depth = test_pixel.bit_depth;
                                                                      
                              
         
      test_pixel.have_tRNS = dp->this.is_transparent != 0;
      test_pixel.red_sBIT = test_pixel.green_sBIT = test_pixel.blue_sBIT =
         test_pixel.alpha_sBIT = test_pixel.sample_depth;

      dp->transform_list->mod(dp->transform_list, &test_pixel, pp, dp);

      if (test_pixel.colour_type != dp->output_colour_type)
      {
         char message[128];
         size_t pos = safecat(message, sizeof message, 0, "colour type ");

         pos = safecatn(message, sizeof message, pos, dp->output_colour_type);
         pos = safecat(message, sizeof message, pos, " expected ");
         pos = safecatn(message, sizeof message, pos, test_pixel.colour_type);

         png_error(pp, message);
      }

      if (test_pixel.bit_depth != dp->output_bit_depth)
      {
         char message[128];
         size_t pos = safecat(message, sizeof message, 0, "bit depth ");

         pos = safecatn(message, sizeof message, pos, dp->output_bit_depth);
         pos = safecat(message, sizeof message, pos, " expected ");
         pos = safecatn(message, sizeof message, pos, test_pixel.bit_depth);

         png_error(pp, message);
      }

                                                                              
         
      if (test_pixel.colour_type == PNG_COLOR_TYPE_PALETTE &&
          test_pixel.sample_depth != 8)  
         png_error(pp, "pngvalid: internal: palette sample depth not 8");
      else if (dp->unpacked && test_pixel.bit_depth != 8)
         png_error(pp, "pngvalid: internal: bad unpacked pixel depth");
      else if (!dp->unpacked && test_pixel.colour_type != PNG_COLOR_TYPE_PALETTE
              && test_pixel.bit_depth != test_pixel.sample_depth)
      {
         char message[128];
         size_t pos = safecat(message, sizeof message, 0,
            "internal: sample depth ");

                                                                               
                                                                             
                     
            
         pos = safecatn(message, sizeof message, pos, test_pixel.sample_depth);
         pos = safecat(message, sizeof message, pos, " expected ");
         pos = safecatn(message, sizeof message, pos, test_pixel.bit_depth);

         png_error(pp, message);
      }
      else if (test_pixel.bit_depth != dp->output_bit_depth)
      {
                                                                             
                                            
            
         char message[128];
         size_t pos = safecat(message, sizeof message, 0,
            "internal: bit depth ");

         pos = safecatn(message, sizeof message, pos, dp->output_bit_depth);
         pos = safecat(message, sizeof message, pos, " expected ");
         pos = safecatn(message, sizeof message, pos, test_pixel.bit_depth);

         png_error(pp, message);
      }
   }
}

static void PNGCBAPI
transform_info(png_structp pp, png_infop pi)
{
   transform_info_imp(voidcast(transform_display*, png_get_progressive_ptr(pp)),
      pp, pi);
}

static void
transform_range_check(png_const_structp pp, unsigned int r, unsigned int g,
   unsigned int b, unsigned int a, unsigned int in_digitized, double in,
   unsigned int out, png_byte sample_depth, double err, double limit,
   const char *name, double digitization_error)
{
                                                                             
                                                                              
                                                                              
                                                                            
                                         
      
   unsigned int max = (1U<<sample_depth)-1;
   double in_min = ceil((in-err)*max - digitization_error);
   double in_max = floor((in+err)*max + digitization_error);
   if (debugonly(err > limit ||) !(out >= in_min && out <= in_max))
   {
      char message[256];
      size_t pos;

      pos = safecat(message, sizeof message, 0, name);
      pos = safecat(message, sizeof message, pos, " output value error: rgba(");
      pos = safecatn(message, sizeof message, pos, r);
      pos = safecat(message, sizeof message, pos, ",");
      pos = safecatn(message, sizeof message, pos, g);
      pos = safecat(message, sizeof message, pos, ",");
      pos = safecatn(message, sizeof message, pos, b);
      pos = safecat(message, sizeof message, pos, ",");
      pos = safecatn(message, sizeof message, pos, a);
      pos = safecat(message, sizeof message, pos, "): ");
      pos = safecatn(message, sizeof message, pos, out);
      pos = safecat(message, sizeof message, pos, " expected: ");
      pos = safecatn(message, sizeof message, pos, in_digitized);
      pos = safecat(message, sizeof message, pos, " (");
      pos = safecatd(message, sizeof message, pos, (in-err)*max, 3);
      pos = safecat(message, sizeof message, pos, "..");
      pos = safecatd(message, sizeof message, pos, (in+err)*max, 3);
      pos = safecat(message, sizeof message, pos, ")");

      png_error(pp, message);
   }

   UNUSED(limit)
}

static void
transform_image_validate(transform_display *dp, png_const_structp pp,
   png_infop pi)
{
    
   const png_store* const ps = dp->this.ps;
   png_byte in_ct = dp->this.colour_type;
   png_byte in_bd = dp->this.bit_depth;
   png_uint_32 w = dp->this.w;
   png_uint_32 h = dp->this.h;
   png_byte out_ct = dp->output_colour_type;
   png_byte out_bd = dp->output_bit_depth;
   png_byte sample_depth =
      (png_byte)(out_ct == PNG_COLOR_TYPE_PALETTE ? 8 : out_bd);
   png_byte red_sBIT = dp->this.red_sBIT;
   png_byte green_sBIT = dp->this.green_sBIT;
   png_byte blue_sBIT = dp->this.blue_sBIT;
   png_byte alpha_sBIT = dp->this.alpha_sBIT;
   int have_tRNS = dp->this.is_transparent;
   double digitization_error;

   store_palette out_palette;
   png_uint_32 y;

   UNUSED(pi)

    
   store_image_check(dp->this.ps, pp, 0);

                                                                            
                                                                
      
   if (out_ct == PNG_COLOR_TYPE_PALETTE)
   {
                                                                             
                  
         
      int npalette = (-1);

      (void)read_palette(out_palette, &npalette, pp, pi);
      if (npalette != dp->this.npalette)
         png_error(pp, "unexpected change in palette size");

      digitization_error = .5;
   }
   else
   {
      png_byte in_sample_depth;

      memset(out_palette, 0x5e, sizeof out_palette);

                                                                               
                                                                                
                                      
         
      if (in_ct == PNG_COLOR_TYPE_PALETTE || in_bd < 16)
         in_sample_depth = 8;
      else
         in_sample_depth = in_bd;

      if (sample_depth != 16 || in_sample_depth > 8 ||
         !dp->pm->calculations_use_input_precision)
         digitization_error = .5;

                                                                          
                                                                               
         
      else
         digitization_error = .5 * 257;
   }

   for (y=0; y<h; ++y)
   {
      png_const_bytep const pRow = store_image_row(ps, pp, 0, y);
      png_uint_32 x;

       
      png_byte std[STANDARD_ROWMAX];

      transform_row(pp, std, in_ct, in_bd, y);

                                                                               
                                      
         
      for (x=0; x<w; ++x)
      {
         image_pixel in_pixel, out_pixel;
         unsigned int r, g, b, a;

          
         image_pixel_init(&in_pixel, std, in_ct, in_bd, x, dp->this.palette,
                 NULL);

         in_pixel.red_sBIT = red_sBIT;
         in_pixel.green_sBIT = green_sBIT;
         in_pixel.blue_sBIT = blue_sBIT;
         in_pixel.alpha_sBIT = alpha_sBIT;
         in_pixel.have_tRNS = have_tRNS != 0;

          
         r = in_pixel.red;
         g = in_pixel.green;
         b = in_pixel.blue;
         a = in_pixel.alpha;

                                                                           
                                                                        
                                       
            
         dp->transform_list->mod(dp->transform_list, &in_pixel, pp, dp);

                                                                         
                                                                          
                                                                      
            
         image_pixel_init(&out_pixel, pRow, out_ct, out_bd, x, out_palette,
                 &in_pixel);

                                                                              
                    
            
         if (in_ct == PNG_COLOR_TYPE_PALETTE &&
            out_ct == PNG_COLOR_TYPE_PALETTE)
         {
            if (in_pixel.palette_index != out_pixel.palette_index)
               png_error(pp, "unexpected transformed palette index");
         }

                                                                                
                                                        
            
         if (in_pixel.red != out_pixel.red)
            transform_range_check(pp, r, g, b, a, in_pixel.red, in_pixel.redf,
               out_pixel.red, sample_depth, in_pixel.rede,
               dp->pm->limit + 1./(2*((1U<<in_pixel.red_sBIT)-1)), "red/gray",
               digitization_error);

         if ((out_ct & PNG_COLOR_MASK_COLOR) != 0 &&
            in_pixel.green != out_pixel.green)
            transform_range_check(pp, r, g, b, a, in_pixel.green,
               in_pixel.greenf, out_pixel.green, sample_depth, in_pixel.greene,
               dp->pm->limit + 1./(2*((1U<<in_pixel.green_sBIT)-1)), "green",
               digitization_error);

         if ((out_ct & PNG_COLOR_MASK_COLOR) != 0 &&
            in_pixel.blue != out_pixel.blue)
            transform_range_check(pp, r, g, b, a, in_pixel.blue, in_pixel.bluef,
               out_pixel.blue, sample_depth, in_pixel.bluee,
               dp->pm->limit + 1./(2*((1U<<in_pixel.blue_sBIT)-1)), "blue",
               digitization_error);

         if ((out_ct & PNG_COLOR_MASK_ALPHA) != 0 &&
            in_pixel.alpha != out_pixel.alpha)
            transform_range_check(pp, r, g, b, a, in_pixel.alpha,
               in_pixel.alphaf, out_pixel.alpha, sample_depth, in_pixel.alphae,
               dp->pm->limit + 1./(2*((1U<<in_pixel.alpha_sBIT)-1)), "alpha",
               digitization_error);
      }  
   }  

    
   dp->this.ps->validated = 1;
}

static void PNGCBAPI
transform_end(png_structp ppIn, png_infop pi)
{
   png_const_structp pp = ppIn;
   transform_display *dp = voidcast(transform_display*,
      png_get_progressive_ptr(pp));

   if (!dp->this.speed)
      transform_image_validate(dp, pp, pi);
   else
      dp->this.ps->validated = 1;
}

 
static void
transform_test(png_modifier *pmIn, png_uint_32 idIn,
    const image_transform* transform_listIn, const char * const name)
{
   transform_display d;
   context(&pmIn->this, fault);

   transform_display_init(&d, pmIn, idIn, transform_listIn);

   Try
   {
      size_t pos = 0;
      png_structp pp;
      png_infop pi;
      char full_name[256];

                                                                         
                       
         
      transform_set_encoding(&d);

       
      d.transform_list->ini(d.transform_list, &d);

       
      pos = safecat(full_name, sizeof full_name, pos, name);
      pos = safecat_current_encoding(full_name, sizeof full_name, pos, d.pm);

       
      pp = set_modifier_for_read(d.pm, &pi, d.this.id, full_name);
      standard_palette_init(&d.this);

#     if 0
          
         {
            char buffer[256];

            (void)store_message(&d.pm->this, pp, buffer, sizeof buffer, 0,
               "running test");

            fprintf(stderr, "%s\n", buffer);
         }
#     endif

       
      if (d.pm->this.progressive)
      {
          
         png_set_progressive_read_fn(pp, &d, transform_info, progressive_row,
            transform_end);

          
         modifier_progressive_read(d.pm, pp, pi);
      }
      else
      {
          
         png_set_read_fn(pp, d.pm, modifier_read);

          
         png_read_info(pp, pi);

          
         transform_info_imp(&d, pp, pi);

         sequential_row(&d.this, pp, pi, -1, 0);

         if (!d.this.speed)
            transform_image_validate(&d, pp, pi);
         else
            d.this.ps->validated = 1;
      }

      modifier_reset(d.pm);
   }

   Catch(fault)
   {
      modifier_reset(voidcast(png_modifier*,(void*)fault));
   }
}

 
#define ITSTRUCT(name) image_transform_##name
#define ITDATA(name) image_transform_data_##name
#define image_transform_ini image_transform_default_ini
#define IT(name)\
static image_transform ITSTRUCT(name) =\
{\
   #name,\
   1,  \
   &PT,  \
   0,  \
   0,  \
   0,  \
   image_transform_ini,\
   image_transform_png_set_##name##_set,\
   image_transform_png_set_##name##_mod,\
   image_transform_png_set_##name##_add\
}
#define PT ITSTRUCT(end)  

 
extern void image_transform_default_ini(const image_transform *this,
   transform_display *that);  

void  
image_transform_default_ini(const image_transform *this,
    transform_display *that)
{
   this->next->ini(this->next, that);
}

#ifdef PNG_READ_BACKGROUND_SUPPORTED
static int
image_transform_default_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(colour_type)
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

   return 1;
}
#endif

#ifdef PNG_READ_EXPAND_SUPPORTED
 
static void
image_transform_png_set_palette_to_rgb_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_palette_to_rgb(pp);
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_palette_to_rgb_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if (that->colour_type == PNG_COLOR_TYPE_PALETTE)
      image_pixel_convert_PLTE(that);

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_palette_to_rgb_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

   return colour_type == PNG_COLOR_TYPE_PALETTE;
}

IT(palette_to_rgb);
#undef PT
#define PT ITSTRUCT(palette_to_rgb)
#endif  

#ifdef PNG_READ_EXPAND_SUPPORTED
 
static void
image_transform_png_set_tRNS_to_alpha_set(const image_transform *this,
   transform_display *that, png_structp pp, png_infop pi)
{
   png_set_tRNS_to_alpha(pp);

                                                                        
                                             
      
   if (that->this.has_tRNS)
      that->this.is_transparent = 1;

   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_tRNS_to_alpha_mod(const image_transform *this,
   image_pixel *that, png_const_structp pp,
   const transform_display *display)
{
#if PNG_LIBPNG_VER < 10700
    
   if (that->colour_type == PNG_COLOR_TYPE_PALETTE)
      image_pixel_convert_PLTE(that);
#endif

                                                                             
                                  
      
   if (that->have_tRNS)
#     if PNG_LIBPNG_VER >= 10700
         if (that->colour_type != PNG_COLOR_TYPE_PALETTE &&
             (that->colour_type & PNG_COLOR_MASK_ALPHA) == 0)
#     endif
      image_pixel_add_alpha(that, &display->this, 0 );

#if PNG_LIBPNG_VER < 10700
    
   else
   {
      if (that->bit_depth < 8)
         that->bit_depth =8;
      if (that->sample_depth < 8)
         that->sample_depth = 8;
   }
#endif

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_tRNS_to_alpha_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

                                                                            
                                                                        
                                                                           
                                    
      
   return
#  if PNG_LIBPNG_VER >= 10700
      colour_type != PNG_COLOR_TYPE_PALETTE &&
#  endif
   (colour_type & PNG_COLOR_MASK_ALPHA) == 0;
}

IT(tRNS_to_alpha);
#undef PT
#define PT ITSTRUCT(tRNS_to_alpha)
#endif  

#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED
 
static void
image_transform_png_set_gray_to_rgb_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_gray_to_rgb(pp);
    
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_gray_to_rgb_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
                                                                             
                                                                          
                                                                           
                                          
      
   if ((that->colour_type & PNG_COLOR_MASK_COLOR) == 0 && that->have_tRNS)
      image_pixel_add_alpha(that, &display->this, 0 );

    
   if (that->colour_type == PNG_COLOR_TYPE_GRAY)
   {
       
      if (that->bit_depth < 8)
         that->sample_depth = that->bit_depth = 8;

                                                                                
                                                                      
         
      that->colour_type = PNG_COLOR_TYPE_RGB;
   }

   else if (that->colour_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      that->colour_type = PNG_COLOR_TYPE_RGB_ALPHA;

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_gray_to_rgb_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

   return (colour_type & PNG_COLOR_MASK_COLOR) == 0;
}

IT(gray_to_rgb);
#undef PT
#define PT ITSTRUCT(gray_to_rgb)
#endif  

#ifdef PNG_READ_EXPAND_SUPPORTED
 
static void
image_transform_png_set_expand_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_expand(pp);

   if (that->this.has_tRNS)
      that->this.is_transparent = 1;

   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_expand_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
    
   if (that->colour_type == PNG_COLOR_TYPE_PALETTE)
      image_pixel_convert_PLTE(that);
   else if (that->bit_depth < 8)  
      that->sample_depth = that->bit_depth = 8;

   if (that->have_tRNS)
      image_pixel_add_alpha(that, &display->this, 0 );

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_expand_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

                                                                           
                                  
      
   return (colour_type & PNG_COLOR_MASK_ALPHA) == 0;
}

IT(expand);
#undef PT
#define PT ITSTRUCT(expand)
#endif  

#ifdef PNG_READ_EXPAND_SUPPORTED
                                 
                                                   
   
static void
image_transform_png_set_expand_gray_1_2_4_to_8_set(
    const image_transform *this, transform_display *that, png_structp pp,
    png_infop pi)
{
   png_set_expand_gray_1_2_4_to_8(pp);
    
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_expand_gray_1_2_4_to_8_mod(
    const image_transform *this, image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
#if PNG_LIBPNG_VER < 10700
   image_transform_png_set_expand_mod(this, that, pp, display);
#else
    
   if (that->colour_type == PNG_COLOR_TYPE_GRAY &&
       that->bit_depth < 8)
      that->sample_depth = that->bit_depth = 8;

   this->next->mod(this->next, that, pp, display);
#endif  
}

static int
image_transform_png_set_expand_gray_1_2_4_to_8_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
#if PNG_LIBPNG_VER < 10700
   return image_transform_png_set_expand_add(this, that, colour_type,
      bit_depth);
#else
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

                                                                               
                  
      
   return colour_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8;
#endif  
}

IT(expand_gray_1_2_4_to_8);
#undef PT
#define PT ITSTRUCT(expand_gray_1_2_4_to_8)
#endif  

#ifdef PNG_READ_EXPAND_16_SUPPORTED
 
static void
image_transform_png_set_expand_16_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_expand_16(pp);

    
#  if PNG_LIBPNG_VER < 10700
      if (that->this.has_tRNS)
         that->this.is_transparent = 1;
#  endif

   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_expand_16_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
                                                                          
                                 
      
   if (that->colour_type == PNG_COLOR_TYPE_PALETTE)
      image_pixel_convert_PLTE(that);

   if (that->have_tRNS)
      image_pixel_add_alpha(that, &display->this, 0 );

   if (that->bit_depth < 16)
      that->sample_depth = that->bit_depth = 16;

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_expand_16_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(colour_type)

   this->next = *that;
   *that = this;

    
   return bit_depth < 16;
}

IT(expand_16);
#undef PT
#define PT ITSTRUCT(expand_16)
#endif  

#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED   
 
static void
image_transform_png_set_scale_16_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_scale_16(pp);
#  if PNG_LIBPNG_VER < 10700
       
      that->max_gamma_8 = PNG_MAX_GAMMA_8;
#  endif
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_scale_16_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if (that->bit_depth == 16)
   {
      that->sample_depth = that->bit_depth = 8;
      if (that->red_sBIT > 8) that->red_sBIT = 8;
      if (that->green_sBIT > 8) that->green_sBIT = 8;
      if (that->blue_sBIT > 8) that->blue_sBIT = 8;
      if (that->alpha_sBIT > 8) that->alpha_sBIT = 8;
   }

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_scale_16_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(colour_type)

   this->next = *that;
   *that = this;

   return bit_depth > 8;
}

IT(scale_16);
#undef PT
#define PT ITSTRUCT(scale_16)
#endif  

#ifdef PNG_READ_16_TO_8_SUPPORTED  
 
static void
image_transform_png_set_strip_16_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_strip_16(pp);
#  if PNG_LIBPNG_VER < 10700
       
      that->max_gamma_8 = PNG_MAX_GAMMA_8;
#  endif
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_strip_16_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if (that->bit_depth == 16)
   {
      that->sample_depth = that->bit_depth = 8;
      if (that->red_sBIT > 8) that->red_sBIT = 8;
      if (that->green_sBIT > 8) that->green_sBIT = 8;
      if (that->blue_sBIT > 8) that->blue_sBIT = 8;
      if (that->alpha_sBIT > 8) that->alpha_sBIT = 8;

                                                                               
                                                                               
                                          
         
#     ifdef PNG_READ_ACCURATE_SCALE_SUPPORTED
#        if PNG_LIBPNG_VER >= 10504
#           error PNG_READ_ACCURATE_SCALE should not be set
#        endif

                                                                               
                                                                           
                                                                          
                                                                                
            
         {
            const double d = (255-128.5)/65535;
            that->rede += d;
            that->greene += d;
            that->bluee += d;
            that->alphae += d;
         }
#     endif
   }

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_strip_16_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(colour_type)

   this->next = *that;
   *that = this;

   return bit_depth > 8;
}

IT(strip_16);
#undef PT
#define PT ITSTRUCT(strip_16)
#endif  

#ifdef PNG_READ_STRIP_ALPHA_SUPPORTED
 
static void
image_transform_png_set_strip_alpha_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_strip_alpha(pp);
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_strip_alpha_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if (that->colour_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      that->colour_type = PNG_COLOR_TYPE_GRAY;
   else if (that->colour_type == PNG_COLOR_TYPE_RGB_ALPHA)
      that->colour_type = PNG_COLOR_TYPE_RGB;

   that->have_tRNS = 0;
   that->alphaf = 1;

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_strip_alpha_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

   return (colour_type & PNG_COLOR_MASK_ALPHA) != 0;
}

IT(strip_alpha);
#undef PT
#define PT ITSTRUCT(strip_alpha)
#endif  

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
                                                                             
                                                                              
                            
                             
  
                                                                              
         
  
                 
                 
                 
  
                                                                             
                                                                             
                                                                             
                                   
   
#define data ITDATA(rgb_to_gray)
static struct
{
   double gamma;       

    
#  ifdef PNG_FLOATING_POINT_SUPPORTED
      double red_to_set;
      double green_to_set;
#  else
      png_fixed_point red_to_set;
      png_fixed_point green_to_set;
#  endif

    
   double red_coefficient;
   double green_coefficient;
   double blue_coefficient;

    
   int coefficients_overridden;
} data;

#undef image_transform_ini
#define image_transform_ini image_transform_png_set_rgb_to_gray_ini
static void
image_transform_png_set_rgb_to_gray_ini(const image_transform *this,
    transform_display *that)
{
   png_modifier *pm = that->pm;
   const color_encoding *e = pm->current_encoding;

   UNUSED(this)

    
   pm->test_uses_encoding = 1;

                                                                              
                                        
      
   if (e != 0)
   {
                                                                                
                             
         
      const double whiteY = e->red.Y + e->green.Y + e->blue.Y;

      data.red_coefficient = e->red.Y;
      data.green_coefficient = e->green.Y;
      data.blue_coefficient = e->blue.Y;

      if (whiteY != 1)
      {
         data.red_coefficient /= whiteY;
         data.green_coefficient /= whiteY;
         data.blue_coefficient /= whiteY;
      }
   }

   else
   {
       
#     if PNG_LIBPNG_VER < 10700
         data.red_coefficient = 6968 / 32768.;
         data.green_coefficient = 23434 / 32768.;
         data.blue_coefficient = 2366 / 32768.;
#     else
         data.red_coefficient = .2126;
         data.green_coefficient = .7152;
         data.blue_coefficient = .0722;
#     endif
   }

   data.gamma = pm->current_gamma;

    
   if (data.gamma == 0)
      data.gamma = 1;

                                                                                
                                                                              
                                             
      
   if (pm->test_exhaustive)
   {
                                                                                
                                                                               
                                               
         
      data.coefficients_overridden = !data.coefficients_overridden;
      pm->repeat = data.coefficients_overridden != 0;
   }

   else
      data.coefficients_overridden = random_choice();

   if (data.coefficients_overridden)
   {
                                                                             
                 
         
      png_uint_32 ru;
      double total;

      ru = random_u32();
      data.green_coefficient = total = (ru & 0xffff) / 65535.;
      ru >>= 16;
      data.red_coefficient = (1 - total) * (ru & 0xffff) / 65535.;
      total += data.red_coefficient;
      data.blue_coefficient = 1 - total;

#     ifdef PNG_FLOATING_POINT_SUPPORTED
         data.red_to_set = data.red_coefficient;
         data.green_to_set = data.green_coefficient;
#     else
         data.red_to_set = fix(data.red_coefficient);
         data.green_to_set = fix(data.green_coefficient);
#     endif

       
      pm->encoding_ignored = 1;
   }

   else
   {
      data.red_to_set = -1;
      data.green_to_set = -1;
   }

                                                                             
                                                             
      
   if (data.gamma != 1)  
   {
      if (that->this.bit_depth == 16 || pm->assume_16_bit_calculations)
      {
                                           
           
                                       
           
                                                                            
                                                                         
                                                                
                                                            
            
#        if PNG_LIBPNG_VER < 10700
            if (that->this.bit_depth < 16)
               that->max_gamma_8 = PNG_MAX_GAMMA_8;
#        endif
         that->pm->limit += pow(
            (that->this.bit_depth == 16 || that->max_gamma_8 > 14 ?
               8. :
               6. + (1<<(15-that->max_gamma_8))
            )/65535, data.gamma);
      }

      else
      {
                                                                              
                                                                            
                                                           
           
                                                                             
                                        
           
                                                                                
                                                                        
            
#if DIGITIZE
          that->pm->limit += pow( 2.0/255, data.gamma);
#else
          that->pm->limit += pow( 1.0/255, data.gamma);
#endif
      }
   }

   else
   {
                                                                                
                                                            
         
      if (that->this.bit_depth != 16 && !pm->assume_16_bit_calculations)
         that->pm->limit += 4E-3;
   }
}

static void
image_transform_png_set_rgb_to_gray_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   int error_action = 1;  

#  ifdef PNG_FLOATING_POINT_SUPPORTED
      png_set_rgb_to_gray(pp, error_action, data.red_to_set, data.green_to_set);
#  else
      png_set_rgb_to_gray_fixed(pp, error_action, data.red_to_set,
         data.green_to_set);
#  endif

#  ifdef PNG_READ_cHRM_SUPPORTED
      if (that->pm->current_encoding != 0)
      {
                                                                             
                                                                             
                                                                            
                 
            
#        ifdef PNG_FLOATING_POINT_SUPPORTED
#           define API_function png_get_cHRM_XYZ
#           define API_form "FP"
#           define API_type double
#           define API_cvt(x) (x)
#        else
#           define API_function png_get_cHRM_XYZ_fixed
#           define API_form "fixed"
#           define API_type png_fixed_point
#           define API_cvt(x) ((double)(x)/PNG_FP_1)
#        endif

         API_type rX, gX, bX;
         API_type rY, gY, bY;
         API_type rZ, gZ, bZ;

         if ((API_function(pp, pi, &rX, &rY, &rZ, &gX, &gY, &gZ, &bX, &bY, &bZ)
               & PNG_INFO_cHRM) != 0)
         {
            double maxe;
            const char *el;
            color_encoding e, o;

                                                                            
                                                          
               
            modifier_current_encoding(that->pm, &o);
            normalize_color_encoding(&o);

                                                                             
                                                                       
                          
               
            if (data.red_to_set == -1 && data.green_to_set == -1 &&
               (fabs(o.red.Y - data.red_coefficient) > DBL_EPSILON ||
               fabs(o.green.Y - data.green_coefficient) > DBL_EPSILON ||
               fabs(o.blue.Y - data.blue_coefficient) > DBL_EPSILON))
               png_error(pp, "internal pngvalid cHRM coefficient error");

             
            e.gamma = o.gamma;  
            e.red.X = API_cvt(rX);
            e.red.Y = API_cvt(rY);
            e.red.Z = API_cvt(rZ);
            e.green.X = API_cvt(gX);
            e.green.Y = API_cvt(gY);
            e.green.Z = API_cvt(gZ);
            e.blue.X = API_cvt(bX);
            e.blue.Y = API_cvt(bY);
            e.blue.Z = API_cvt(bZ);

                                                                               
                                                                            
               
            maxe = 0;
            el = "-";  

#           define CHECK(col,x)\
            {\
               double err = fabs(o.col.x - e.col.x);\
               if (err > maxe)\
               {\
                  maxe = err;\
                  el = #col "(" #x ")";\
               }\
            }

            CHECK(red,X)
            CHECK(red,Y)
            CHECK(red,Z)
            CHECK(green,X)
            CHECK(green,Y)
            CHECK(green,Z)
            CHECK(blue,X)
            CHECK(blue,Y)
            CHECK(blue,Z)

                                                                             
                                                                               
                                                                              
               
            if (maxe >= 1E-5)
            {
               size_t pos = 0;
               char buffer[256];

               pos = safecat(buffer, sizeof buffer, pos, API_form);
               pos = safecat(buffer, sizeof buffer, pos, " cHRM ");
               pos = safecat(buffer, sizeof buffer, pos, el);
               pos = safecat(buffer, sizeof buffer, pos, " error: ");
               pos = safecatd(buffer, sizeof buffer, pos, maxe, 7);
               pos = safecat(buffer, sizeof buffer, pos, " ");
                
               pos = safecat_color_encoding(buffer, sizeof buffer, pos, &o, 0);
               pos = safecat(buffer, sizeof buffer, pos, " -> ");
               pos = safecat_color_encoding(buffer, sizeof buffer, pos, &e, 0);

               png_error(pp, buffer);
            }
         }
      }
#  endif  

   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_rgb_to_gray_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if ((that->colour_type & PNG_COLOR_MASK_COLOR) != 0)
   {
      double gray, err;

#     if PNG_LIBPNG_VER < 10700
         if (that->colour_type == PNG_COLOR_TYPE_PALETTE)
            image_pixel_convert_PLTE(that);
#     endif

       
#  if DIGITIZE
      {
         png_modifier *pm = display->pm;
         unsigned int sample_depth = that->sample_depth;
         unsigned int calc_depth = (pm->assume_16_bit_calculations ? 16 :
            sample_depth);
         unsigned int gamma_depth =
            (sample_depth == 16 ?
               display->max_gamma_8 :
               (pm->assume_16_bit_calculations ?
                  display->max_gamma_8 :
                  sample_depth));
         int isgray;
         double r, g, b;
         double rlo, rhi, glo, ghi, blo, bhi, graylo, grayhi;

                                                                               
                                        
           
                                                                              
                                                                               
                                                                       
                                                                                
                                                                   
           
                                                                        
                                                                                
                                                                              
                                                                    
            
#        define DD(v,d,r) (digitize(v*(1-DBL_EPSILON), d, r) * (1-DBL_EPSILON))
#        define DU(v,d,r) (digitize(v*(1+DBL_EPSILON), d, r) * (1+DBL_EPSILON))

         r = rlo = rhi = that->redf;
         rlo -= that->rede;
         rlo = DD(rlo, calc_depth, 1 );
         rhi += that->rede;
         rhi = DU(rhi, calc_depth, 1 );

         g = glo = ghi = that->greenf;
         glo -= that->greene;
         glo = DD(glo, calc_depth, 1 );
         ghi += that->greene;
         ghi = DU(ghi, calc_depth, 1 );

         b = blo = bhi = that->bluef;
         blo -= that->bluee;
         blo = DD(blo, calc_depth, 1 );
         bhi += that->bluee;
         bhi = DU(bhi, calc_depth, 1 );

         isgray = r==g && g==b;

         if (data.gamma != 1)
         {
            const double power = 1/data.gamma;
            const double abse = .5/(sample_depth == 16 ? 65535 : 255);

                                                                               
                                                                              
                                                 
               
            if (gamma_depth != calc_depth)
            {
               rlo = DD(rlo, gamma_depth, 0 );
               rhi = DU(rhi, gamma_depth, 0 );
               glo = DD(glo, gamma_depth, 0 );
               ghi = DU(ghi, gamma_depth, 0 );
               blo = DD(blo, gamma_depth, 0 );
               bhi = DU(bhi, gamma_depth, 0 );
            }

             
            r = pow(r, power);
            rlo = DD(pow(rlo, power)-abse, calc_depth, 1);
            rhi = DU(pow(rhi, power)+abse, calc_depth, 1);

            g = pow(g, power);
            glo = DD(pow(glo, power)-abse, calc_depth, 1);
            ghi = DU(pow(ghi, power)+abse, calc_depth, 1);

            b = pow(b, power);
            blo = DD(pow(blo, power)-abse, calc_depth, 1);
            bhi = DU(pow(bhi, power)+abse, calc_depth, 1);
         }

                                                                            
                                                                              
                                                                                
                                                                              
                                                                           
                                                                           
           
                                                                                
                                                                      
            
         gray = r * data.red_coefficient + g * data.green_coefficient +
            b * data.blue_coefficient;

         {
            int do_round = data.gamma != 1 || calc_depth == 16;
            const double ce = 2. / 32768;

            graylo = DD(rlo * (data.red_coefficient-ce) +
               glo * (data.green_coefficient-ce) +
               blo * (data.blue_coefficient-ce), calc_depth, do_round);
            if (graylo > gray)  
               graylo = gray;

            grayhi = DU(rhi * (data.red_coefficient+ce) +
               ghi * (data.green_coefficient+ce) +
               bhi * (data.blue_coefficient+ce), calc_depth, do_round);
            if (grayhi < gray)
               grayhi = gray;
         }

          
         if (data.gamma != 1)
         {
            const double power = data.gamma;

             
            if (gamma_depth != sample_depth)
            {
               rlo = DD(rlo, gamma_depth, 0 );
               rhi = DU(rhi, gamma_depth, 0 );
               glo = DD(glo, gamma_depth, 0 );
               ghi = DU(ghi, gamma_depth, 0 );
               blo = DD(blo, gamma_depth, 0 );
               bhi = DU(bhi, gamma_depth, 0 );
            }

            gray = pow(gray, power);
            graylo = DD(pow(graylo, power), sample_depth, 1);
            grayhi = DU(pow(grayhi, power), sample_depth, 1);
         }

#        undef DD
#        undef DU

                                            
           
                                                                          
                                                   
            
         if (isgray)
            err = (that->rede + that->greene + that->bluee)/3;

         else
         {
            err = fabs(grayhi-gray);

            if (fabs(gray - graylo) > err)
               err = fabs(graylo-gray);

#if !RELEASE_BUILD
             
            if (err > pm->limit)
            {
               size_t pos = 0;
               char buffer[128];

               pos = safecat(buffer, sizeof buffer, pos, "rgb_to_gray error ");
               pos = safecatd(buffer, sizeof buffer, pos, err, 6);
               pos = safecat(buffer, sizeof buffer, pos, " exceeds limit ");
               pos = safecatd(buffer, sizeof buffer, pos, pm->limit, 6);
               png_warning(pp, buffer);
               pm->limit = err;
            }
#endif  
         }
      }
#  else   
      {
         double r = that->redf;
         double re = that->rede;
         double g = that->greenf;
         double ge = that->greene;
         double b = that->bluef;
         double be = that->bluee;

#        if PNG_LIBPNG_VER < 10700
                                                                           
                                                                           
               
            if (r == g && r == b)
            {
               gray = r;
               err = re;
               if (err < ge) err = ge;
               if (err < be) err = be;
            }

            else
#        endif  
         if (data.gamma == 1)
         {
                                                                               
                                                                            
                                                                                
                                                                        
                                                                          
              
                                                               
              
                                                                       
                                                                            
                                                                             
                                                                                
                                                                        
              
                                                                               
                                             
              
                                                                              
                                                                  
               
            gray = r * data.red_coefficient + g * data.green_coefficient +
               b * data.blue_coefficient;
            err = re * data.red_coefficient + ge * data.green_coefficient +
               be * data.blue_coefficient + 2./32768 + gray * 5 * DBL_EPSILON;
         }

         else
         {
                                                                              
                                                                            
                                                                               
                                                                            
                                               
               
            png_modifier *pm = display->pm;
            double in_qe = (that->sample_depth > 8 ? .5/65535 : .5/255);
            double out_qe = (that->sample_depth > 8 ? .5/65535 :
               (pm->assume_16_bit_calculations ? .5/(1<<display->max_gamma_8) :
               .5/255));
            double rhi, ghi, bhi, grayhi;
            double g1 = 1/data.gamma;

            rhi = r + re + in_qe; if (rhi > 1) rhi = 1;
            r -= re + in_qe; if (r < 0) r = 0;
            ghi = g + ge + in_qe; if (ghi > 1) ghi = 1;
            g -= ge + in_qe; if (g < 0) g = 0;
            bhi = b + be + in_qe; if (bhi > 1) bhi = 1;
            b -= be + in_qe; if (b < 0) b = 0;

            r = pow(r, g1)*(1-DBL_EPSILON); rhi = pow(rhi, g1)*(1+DBL_EPSILON);
            g = pow(g, g1)*(1-DBL_EPSILON); ghi = pow(ghi, g1)*(1+DBL_EPSILON);
            b = pow(b, g1)*(1-DBL_EPSILON); bhi = pow(bhi, g1)*(1+DBL_EPSILON);

                                                                            
                                                                             
                                                                       
               
            gray = r * data.red_coefficient + g * data.green_coefficient +
               b * data.blue_coefficient - 2./32768 - out_qe;
            if (gray <= 0)
               gray = 0;
            else
            {
               gray *= (1 - 6 * DBL_EPSILON);
               gray = pow(gray, data.gamma) * (1-DBL_EPSILON);
            }

            grayhi = rhi * data.red_coefficient + ghi * data.green_coefficient +
               bhi * data.blue_coefficient + 2./32768 + out_qe;
            grayhi *= (1 + 6 * DBL_EPSILON);
            if (grayhi >= 1)
               grayhi = 1;
            else
               grayhi = pow(grayhi, data.gamma) * (1+DBL_EPSILON);

            err = (grayhi - gray) / 2;
            gray = (grayhi + gray) / 2;

            if (err <= in_qe)
               err = gray * DBL_EPSILON;

            else
               err -= in_qe;

#if !RELEASE_BUILD
                                                                        
                                                                      
               
            if (err > pm->limit)
            {
               size_t pos = 0;
               char buffer[128];

               pos = safecat(buffer, sizeof buffer, pos, "rgb_to_gray error ");
               pos = safecatd(buffer, sizeof buffer, pos, err, 6);
               pos = safecat(buffer, sizeof buffer, pos, " exceeds limit ");
               pos = safecatd(buffer, sizeof buffer, pos, pm->limit, 6);
               png_warning(pp, buffer);
               pm->limit = err;
            }
#endif  
         }
      }
#  endif  

      that->bluef = that->greenf = that->redf = gray;
      that->bluee = that->greene = that->rede = err;

       
      if (that->red_sBIT > that->green_sBIT)
         that->red_sBIT = that->green_sBIT;
      if (that->red_sBIT > that->blue_sBIT)
         that->red_sBIT = that->blue_sBIT;
      that->blue_sBIT = that->green_sBIT = that->red_sBIT;

       
      if (that->colour_type == PNG_COLOR_TYPE_RGB)
         that->colour_type = PNG_COLOR_TYPE_GRAY;
      else if (that->colour_type == PNG_COLOR_TYPE_RGB_ALPHA)
         that->colour_type = PNG_COLOR_TYPE_GRAY_ALPHA;
   }

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_rgb_to_gray_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

   return (colour_type & PNG_COLOR_MASK_COLOR) != 0;
}

#undef data
IT(rgb_to_gray);
#undef PT
#define PT ITSTRUCT(rgb_to_gray)
#undef image_transform_ini
#define image_transform_ini image_transform_default_ini
#endif  

#ifdef PNG_READ_BACKGROUND_SUPPORTED
                                                                        
                                                                          
                                                                              
                                                 
                                       
  
                                       
  
#define data ITDATA(background)
static image_pixel data;

static void
image_transform_png_set_background_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_byte colour_type, bit_depth;
   png_byte random_bytes[8];  
   int expand;
   png_color_16 back;

                                                                                
                                                                                
                                                                              
                        
      
   R8(random_bytes);

                                                                                
                                             
      
   colour_type = that->this.colour_type;
   if (colour_type == 3)
   {
      colour_type = PNG_COLOR_TYPE_RGB;
      bit_depth = 8;
      expand = 0;  
   }

   else
   {
      if (that->this.has_tRNS)
         that->this.is_transparent = 1;

      bit_depth = that->this.bit_depth;
      expand = 1;
   }

   image_pixel_init(&data, random_bytes, colour_type,
      bit_depth, 0 , 0 , NULL );

                                                                            
                                          
      
   R8(back);

   if (colour_type & PNG_COLOR_MASK_COLOR)
   {
      back.red = (png_uint_16)data.red;
      back.green = (png_uint_16)data.green;
      back.blue = (png_uint_16)data.blue;
   }

   else
      back.gray = (png_uint_16)data.red;

#ifdef PNG_FLOATING_POINT_SUPPORTED
   png_set_background(pp, &back, PNG_BACKGROUND_GAMMA_FILE, expand, 0);
#else
   png_set_background_fixed(pp, &back, PNG_BACKGROUND_GAMMA_FILE, expand, 0);
#endif

   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_background_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
    
   if (that->have_tRNS && that->colour_type != PNG_COLOR_TYPE_PALETTE)
      image_pixel_add_alpha(that, &display->this, 1 );

    
   if (that->alphaf < 1)
   {
       
      if (that->alphaf <= 0)
      {
         that->redf = data.redf;
         that->greenf = data.greenf;
         that->bluef = data.bluef;

         that->rede = data.rede;
         that->greene = data.greene;
         that->bluee = data.bluee;

         that->red_sBIT= data.red_sBIT;
         that->green_sBIT= data.green_sBIT;
         that->blue_sBIT= data.blue_sBIT;
      }

      else  
      {
         double alf = 1 - that->alphaf;

         that->redf = that->redf * that->alphaf + data.redf * alf;
         that->rede = that->rede * that->alphaf + data.rede * alf +
            DBL_EPSILON;
         that->greenf = that->greenf * that->alphaf + data.greenf * alf;
         that->greene = that->greene * that->alphaf + data.greene * alf +
            DBL_EPSILON;
         that->bluef = that->bluef * that->alphaf + data.bluef * alf;
         that->bluee = that->bluee * that->alphaf + data.bluee * alf +
            DBL_EPSILON;
      }

       
      that->alphaf = 1;
      that->alphae = 0;
   }

   if (that->colour_type == PNG_COLOR_TYPE_RGB_ALPHA)
      that->colour_type = PNG_COLOR_TYPE_RGB;
   else if (that->colour_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      that->colour_type = PNG_COLOR_TYPE_GRAY;
    

   this->next->mod(this->next, that, pp, display);
}

#define image_transform_png_set_background_add image_transform_default_add

#undef data
IT(background);
#undef PT
#define PT ITSTRUCT(background)
#endif  

                                                                     
                                                                          
  
                                   
   
 

                                                                           
                                                                                
                                                                             
                                                                               
                                                                           
                
   
 
#ifdef PNG_READ_INVERT_ALPHA_SUPPORTED
                           
  
                                              
   
static void
image_transform_png_set_invert_alpha_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_invert_alpha(pp);
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_invert_alpha_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if (that->colour_type & 4)
      that->alpha_inverted = 1;

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_invert_alpha_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

    
   return (colour_type & 4) != 0;
}

IT(invert_alpha);
#undef PT
#define PT ITSTRUCT(invert_alpha)

#endif  

 
#ifdef PNG_READ_BGR_SUPPORTED
                                      
  
                                     
  
                                                  
   
static void
image_transform_png_set_bgr_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_bgr(pp);
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_bgr_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if (that->colour_type == PNG_COLOR_TYPE_RGB ||
       that->colour_type == PNG_COLOR_TYPE_RGBA)
       that->swap_rgb = 1;

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_bgr_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

   return colour_type == PNG_COLOR_TYPE_RGB ||
       colour_type == PNG_COLOR_TYPE_RGBA;
}

IT(bgr);
#undef PT
#define PT ITSTRUCT(bgr)

#endif  

 
#ifdef PNG_READ_SWAP_ALPHA_SUPPORTED
                               
  
                                            
  
                                                 
   
static void
image_transform_png_set_swap_alpha_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_swap_alpha(pp);
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_swap_alpha_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if (that->colour_type == PNG_COLOR_TYPE_GA ||
       that->colour_type == PNG_COLOR_TYPE_RGBA)
      that->alpha_first = 1;

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_swap_alpha_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

   return colour_type == PNG_COLOR_TYPE_GA ||
       colour_type == PNG_COLOR_TYPE_RGBA;
}

IT(swap_alpha);
#undef PT
#define PT ITSTRUCT(swap_alpha)

#endif  

 
#ifdef PNG_READ_SWAP_SUPPORTED
                               
  
                                      
   
static void
image_transform_png_set_swap_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_swap(pp);
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_swap_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if (that->bit_depth == 16)
      that->swap16 = 1;

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_swap_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(colour_type)

   this->next = *that;
   *that = this;

   return bit_depth == 16;
}

IT(swap);
#undef PT
#define PT ITSTRUCT(swap)

#endif  

#ifdef PNG_READ_FILLER_SUPPORTED
                                                        
  
                                                                          
  
         
  
                     
                    
   
#define data ITDATA(filler)
static struct
{
   png_uint_32 filler;
   int         flags;
} data;

static void
image_transform_png_set_filler_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
                                                                      
                                                                         
                                                           
      
   data.filler = random_u32();
   data.flags = random_choice();

   png_set_filler(pp, data.filler, data.flags);

                                                                 
                                          
      
   that->this.filler = 1;

   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_filler_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if (that->bit_depth >= 8 &&
       (that->colour_type == PNG_COLOR_TYPE_RGB ||
        that->colour_type == PNG_COLOR_TYPE_GRAY))
   {
      unsigned int max = (1U << that->bit_depth)-1;
      that->alpha = data.filler & max;
      that->alphaf = ((double)that->alpha) / max;
      that->alphae = 0;

                                                                        
                                                                     
                                                                        
                                                                      
                                                         
         
      that->colour_type |= 4;  
      that->alpha_first = data.flags == PNG_FILLER_BEFORE;
   }

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_filler_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   this->next = *that;
   *that = this;

   return bit_depth >= 8 && (colour_type == PNG_COLOR_TYPE_RGB ||
           colour_type == PNG_COLOR_TYPE_GRAY);
}

#undef data
IT(filler);
#undef PT
#define PT ITSTRUCT(filler)

 
 
#define data ITDATA(add_alpha)
static struct
{
   png_uint_32 filler;
   int         flags;
} data;

static void
image_transform_png_set_add_alpha_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
                                                                      
                                                                         
                                                           
      
   data.filler = random_u32();
   data.flags = random_choice();

   png_set_add_alpha(pp, data.filler, data.flags);
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_add_alpha_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if (that->bit_depth >= 8 &&
       (that->colour_type == PNG_COLOR_TYPE_RGB ||
        that->colour_type == PNG_COLOR_TYPE_GRAY))
   {
      unsigned int max = (1U << that->bit_depth)-1;
      that->alpha = data.filler & max;
      that->alphaf = ((double)that->alpha) / max;
      that->alphae = 0;

      that->colour_type |= 4;  
      that->alpha_first = data.flags == PNG_FILLER_BEFORE;
   }

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_add_alpha_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   this->next = *that;
   *that = this;

   return bit_depth >= 8 && (colour_type == PNG_COLOR_TYPE_RGB ||
           colour_type == PNG_COLOR_TYPE_GRAY);
}

#undef data
IT(add_alpha);
#undef PT
#define PT ITSTRUCT(add_alpha)

#endif  

 
#ifdef PNG_READ_PACK_SUPPORTED
                                                      
  
                                         
  
                                                                             
             
   
static void
image_transform_png_set_packing_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_packing(pp);
   that->unpacked = 1;
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_packing_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
                                                                 
                                                                
                                              
      
   if (that->bit_depth < 8)  
      that->bit_depth = 8;

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_packing_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(colour_type)

   this->next = *that;
   *that = this;

    
   return bit_depth < 8;
}

IT(packing);
#undef PT
#define PT ITSTRUCT(packing)

#endif  

 
#ifdef PNG_READ_PACKSWAP_SUPPORTED
                                                                      
                                                          
  
                                          
   
static void
image_transform_png_set_packswap_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_packswap(pp);
   that->this.littleendian = 1;
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_packswap_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if (that->bit_depth < 8)
      that->littleendian = 1;

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_packswap_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(colour_type)

   this->next = *that;
   *that = this;

   return bit_depth < 8;
}

IT(packswap);
#undef PT
#define PT ITSTRUCT(packswap)

#endif  


 
#ifdef PNG_READ_INVERT_MONO_SUPPORTED
                          
  
                                             
   
static void
image_transform_png_set_invert_mono_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_invert_mono(pp);
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_invert_mono_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   if (that->colour_type & 4)
      that->mono_inverted = 1;

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_invert_mono_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

    
   return (colour_type & 2) == 0;
}

IT(invert_mono);
#undef PT
#define PT ITSTRUCT(invert_mono)

#endif  

#ifdef PNG_READ_SHIFT_SUPPORTED
                                                           
  
                                                           
          
   
#define data ITDATA(shift)
static png_color_8 data;

static void
image_transform_png_set_shift_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
                                                                  
                                                                 
                                                                  
                                                                   
                                        
      
   unsigned int depth = that->this.bit_depth;

   data.red = (png_byte) (random_mod(depth)+1);
   data.green = (png_byte) (random_mod(depth)+1);
   data.blue = (png_byte) (random_mod(depth)+1);
   data.gray = (png_byte) (random_mod(depth)+1);
   data.alpha = (png_byte) (random_mod(depth)+1);

   png_set_shift(pp, &data);
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_shift_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
                                                                      
                               
      
   if (that->colour_type != PNG_COLOR_TYPE_PALETTE)
   {
       that->sig_bits = 1;

                                                                    
                                                     
               
          
       if (that->colour_type & 2)  
       {
          that->red_sBIT = data.red;
          that->green_sBIT = data.green;
          that->blue_sBIT = data.blue;
       }

       else  
          that->red_sBIT = that->green_sBIT = that->blue_sBIT = data.gray;

       that->alpha_sBIT = data.alpha;
   }

   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_shift_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   UNUSED(bit_depth)

   this->next = *that;
   *that = this;

   return colour_type != PNG_COLOR_TYPE_PALETTE;
}

IT(shift);
#undef PT
#define PT ITSTRUCT(shift)

#endif  

#ifdef THIS_IS_THE_PROFORMA
static void
image_transform_png_set_@_set(const image_transform *this,
    transform_display *that, png_structp pp, png_infop pi)
{
   png_set_@(pp);
   this->next->set(this->next, that, pp, pi);
}

static void
image_transform_png_set_@_mod(const image_transform *this,
    image_pixel *that, png_const_structp pp,
    const transform_display *display)
{
   this->next->mod(this->next, that, pp, display);
}

static int
image_transform_png_set_@_add(image_transform *this,
    const image_transform **that, png_byte colour_type, png_byte bit_depth)
{
   this->next = *that;
   *that = this;

   return 1;
}

IT(@);
#endif


 
static image_transform *const image_transform_first = &PT;

static void
transform_enable(const char *name)
{
                                                                      
                                           
      
   static int all_disabled = 0;
   int found_it = 0;
   image_transform *list = image_transform_first;

   while (list != &image_transform_end)
   {
      if (strcmp(list->name, name) == 0)
      {
         list->enable = 1;
         found_it = 1;
      }
      else if (!all_disabled)
         list->enable = 0;

      list = list->list;
   }

   all_disabled = 1;

   if (!found_it)
   {
      fprintf(stderr, "pngvalid: --transform-enable=%s: unknown transform\n",
         name);
      exit(99);
   }
}

static void
transform_disable(const char *name)
{
   image_transform *list = image_transform_first;

   while (list != &image_transform_end)
   {
      if (strcmp(list->name, name) == 0)
      {
         list->enable = 0;
         return;
      }

      list = list->list;
   }

   fprintf(stderr, "pngvalid: --transform-disable=%s: unknown transform\n",
      name);
   exit(99);
}

static void
image_transform_reset_count(void)
{
   image_transform *next = image_transform_first;
   int count = 0;

   while (next != &image_transform_end)
   {
      next->local_use = 0;
      next->next = 0;
      next = next->list;
      ++count;
   }

                                                                              
                           
      
   if (count > 32) abort();
}

static int
image_transform_test_counter(png_uint_32 counter, unsigned int max)
{
                                                                           
                                
      
   image_transform *next = image_transform_first;

   while (next != &image_transform_end)
   {
       
      counter >>= 1;

       
      if (max > 1 && next->local_use < max)
         return 1;
      next = next->list;
   }

   return max <= 1 && counter == 0;
}

static png_uint_32
image_transform_add(const image_transform **this, unsigned int max,
   png_uint_32 counter, char *name, size_t sizeof_name, size_t *pos,
   png_byte colour_type, png_byte bit_depth)
{
   for (;;)  
   {
      png_uint_32 mask;
      image_transform *list;

                                                                              
                                                                               
                                                                         
         
      if (counter == 0)  
      {
         image_transform_reset_count();
         if (max <= 1)
            counter = 1;
         else
            counter = random_32();
      }
      else  
      {
         switch (max)
         {
            case 0:  ++counter; break;
            case 1:  counter <<= 1; break;
            default: counter = random_32(); break;
         }
      }

       
      *this = &image_transform_end;
      list = image_transform_first;
      mask = 1;

       
      while (list != &image_transform_end)
      {
         if ((counter & mask) != 0 && list->enable &&
             (max == 0 || list->local_use < max))
         {
             
            if (list->add(list, this, colour_type, bit_depth) || max == 0)
            {
                
               *pos = safecat(name, sizeof_name, *pos, " +");
               *pos = safecat(name, sizeof_name, *pos, list->name);
            }

            else
            {
                
               *this = list->next;
               list->next = 0;

                                                                               
                              
                  
               list->local_use = max;
            }
         }

         mask <<= 1;
         list = list->list;
      }

       
      if (*this != &image_transform_end)
         return counter;

       
      if (!image_transform_test_counter(counter, max))
         return 0;
   }
}

static void
perform_transform_test(png_modifier *pm)
{
   png_byte colour_type = 0;
   png_byte bit_depth = 0;
   unsigned int palette_number = 0;

   while (next_format(&colour_type, &bit_depth, &palette_number, pm->test_lbg,
            pm->test_tRNS))
   {
      png_uint_32 counter = 0;
      size_t base_pos;
      char name[64];

      base_pos = safecat(name, sizeof name, 0, "transform:");

      for (;;)
      {
         size_t pos = base_pos;
         const image_transform *list = 0;

                                                                               
                         
            
         counter = image_transform_add(&list, 1 , counter,
            name, sizeof name, &pos, colour_type, bit_depth);

         if (counter == 0)
            break;

          
         do
         {
            pm->repeat = 0;
            transform_test(pm, FILEID(colour_type, bit_depth, palette_number,
               pm->interlace_type, 0, 0, 0), list, name);

            if (fail(pm))
               return;
         }
         while (pm->repeat);
      }
   }
}
#endif  

 
#ifdef PNG_READ_GAMMA_SUPPORTED
                                                                            
        
   
typedef struct gamma_display
{
   standard_display this;

    
   png_modifier*    pm;
   double           file_gamma;
   double           screen_gamma;
   double           background_gamma;
   png_byte         sbit;
   int              threshold_test;
   int              use_input_precision;
   int              scale16;
   int              expand16;
   int              do_background;
   png_color_16     background_color;

    
   double       maxerrout;
   double       maxerrpc;
   double       maxerrabs;
} gamma_display;

#define ALPHA_MODE_OFFSET 4

static void
gamma_display_init(gamma_display *dp, png_modifier *pm, png_uint_32 id,
    double file_gamma, double screen_gamma, png_byte sbit, int threshold_test,
    int use_input_precision, int scale16, int expand16,
    int do_background, const png_color_16 *pointer_to_the_background_color,
    double background_gamma)
{
    
   standard_display_init(&dp->this, &pm->this, id, do_read_interlace,
      pm->use_update_info);

    
   dp->pm = pm;
   dp->file_gamma = file_gamma;
   dp->screen_gamma = screen_gamma;
   dp->background_gamma = background_gamma;
   dp->sbit = sbit;
   dp->threshold_test = threshold_test;
   dp->use_input_precision = use_input_precision;
   dp->scale16 = scale16;
   dp->expand16 = expand16;
   dp->do_background = do_background;
   if (do_background && pointer_to_the_background_color != 0)
      dp->background_color = *pointer_to_the_background_color;
   else
      memset(&dp->background_color, 0, sizeof dp->background_color);

    
   dp->maxerrout = dp->maxerrpc = dp->maxerrabs = 0;
}

static void
gamma_info_imp(gamma_display *dp, png_structp pp, png_infop pi)
{
    
   standard_info_part1(&dp->this, pp, pi);

                                                                           
                                                                             
                                                                           
                                                            
      
   if (dp->scale16)
#     ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
         png_set_scale_16(pp);
#     else
          
#        ifdef PNG_READ_16_TO_8_SUPPORTED
            png_set_strip_16(pp);
#        else
            png_error(pp, "scale16 (16 to 8 bit conversion) not supported");
#        endif
#     endif

   if (dp->expand16)
#     ifdef PNG_READ_EXPAND_16_SUPPORTED
         png_set_expand_16(pp);
#     else
         png_error(pp, "expand16 (8 to 16 bit conversion) not supported");
#     endif

   if (dp->do_background >= ALPHA_MODE_OFFSET)
   {
#     ifdef PNG_READ_ALPHA_MODE_SUPPORTED
      {
          
         int mode = dp->do_background - ALPHA_MODE_OFFSET;

                                                                        
                                                                                 
                                                                            
            
         const double sg = dp->screen_gamma;
#        ifndef PNG_FLOATING_POINT_SUPPORTED
            png_fixed_point g = fix(sg);
#        endif

#        ifdef PNG_FLOATING_POINT_SUPPORTED
            png_set_alpha_mode(pp, mode, sg);
#        else
            png_set_alpha_mode_fixed(pp, mode, g);
#        endif

                                                                               
                                                                              
                           
            
         if (mode == PNG_ALPHA_STANDARD && sg != 1)
         {
#           ifdef PNG_FLOATING_POINT_SUPPORTED
               png_set_gamma(pp, sg, dp->file_gamma);
#           else
               png_fixed_point f = fix(dp->file_gamma);
               png_set_gamma_fixed(pp, g, f);
#           endif
         }
      }
#     else
         png_error(pp, "alpha mode handling not supported");
#     endif
   }

   else
   {
       
#     ifdef PNG_FLOATING_POINT_SUPPORTED
         png_set_gamma(pp, dp->screen_gamma, dp->file_gamma);
#     else
      {
         png_fixed_point s = fix(dp->screen_gamma);
         png_fixed_point f = fix(dp->file_gamma);
         png_set_gamma_fixed(pp, s, f);
      }
#     endif

      if (dp->do_background)
      {
#     ifdef PNG_READ_BACKGROUND_SUPPORTED
                                                                                
            
         const double bg = dp->background_gamma;
#        ifndef PNG_FLOATING_POINT_SUPPORTED
            png_fixed_point g = fix(bg);
#        endif

#        ifdef PNG_FLOATING_POINT_SUPPORTED
            png_set_background(pp, &dp->background_color, dp->do_background,
               0 , bg);
#        else
            png_set_background_fixed(pp, &dp->background_color,
               dp->do_background, 0 , g);
#        endif
#     else
         png_error(pp, "png_set_background not supported");
#     endif
      }
   }

   {
      int i = dp->this.use_update_info;
       
      do
         png_read_update_info(pp, pi);
      while (--i > 0);
   }

    
   standard_info_part2(&dp->this, pp, pi, 1  );
}

static void PNGCBAPI
gamma_info(png_structp pp, png_infop pi)
{
   gamma_info_imp(voidcast(gamma_display*, png_get_progressive_ptr(pp)), pp,
      pi);
}

                                                                            
                                                                               
                                               
   
typedef struct validate_info
{
   png_const_structp  pp;
   gamma_display *dp;
   png_byte sbit;
   int use_input_precision;
   int do_background;
   int scale16;
   unsigned int sbit_max;
   unsigned int isbit_shift;
   unsigned int outmax;

   double gamma_correction;  
   double file_inverse;      
   double screen_gamma;
   double screen_inverse;    

   double background_red;    
   double background_green;
   double background_blue;

   double maxabs;
   double maxpc;
   double maxcalc;
   double maxout;
   double maxout_total;      
   double outlog;
   int    outquant;
}
validate_info;

static void
init_validate_info(validate_info *vi, gamma_display *dp, png_const_structp pp,
    int in_depth, int out_depth)
{
   unsigned int outmax = (1U<<out_depth)-1;

   vi->pp = pp;
   vi->dp = dp;

   if (dp->sbit > 0 && dp->sbit < in_depth)
   {
      vi->sbit = dp->sbit;
      vi->isbit_shift = in_depth - dp->sbit;
   }

   else
   {
      vi->sbit = (png_byte)in_depth;
      vi->isbit_shift = 0;
   }

   vi->sbit_max = (1U << vi->sbit)-1;

                                                                         
                                        
      
   vi->screen_gamma = dp->screen_gamma;
   if (fabs(vi->screen_gamma-1) < PNG_GAMMA_THRESHOLD)
      vi->screen_gamma = vi->screen_inverse = 0;
   else
      vi->screen_inverse = 1/vi->screen_gamma;

   vi->use_input_precision = dp->use_input_precision;
   vi->outmax = outmax;
   vi->maxabs = abserr(dp->pm, in_depth, out_depth);
   vi->maxpc = pcerr(dp->pm, in_depth, out_depth);
   vi->maxcalc = calcerr(dp->pm, in_depth, out_depth);
   vi->maxout = outerr(dp->pm, in_depth, out_depth);
   vi->outquant = output_quantization_factor(dp->pm, in_depth, out_depth);
   vi->maxout_total = vi->maxout + vi->outquant * .5;
   vi->outlog = outlog(dp->pm, in_depth, out_depth);

   if ((dp->this.colour_type & PNG_COLOR_MASK_ALPHA) != 0 ||
      (dp->this.colour_type == 3 && dp->this.is_transparent) ||
      ((dp->this.colour_type == 0 || dp->this.colour_type == 2) &&
       dp->this.has_tRNS))
   {
      vi->do_background = dp->do_background;

      if (vi->do_background != 0)
      {
         const double bg_inverse = 1/dp->background_gamma;
         double r, g, b;

          
         r = dp->background_color.red; r /= outmax;
         g = dp->background_color.green; g /= outmax;
         b = dp->background_color.blue; b /= outmax;

#     if 0
                                                                             
            
         if (fabs(bg_inverse-1) >= PNG_GAMMA_THRESHOLD)
#     endif
         {
            r = pow(r, bg_inverse);
            g = pow(g, bg_inverse);
            b = pow(b, bg_inverse);
         }

         vi->background_red = r;
         vi->background_green = g;
         vi->background_blue = b;
      }
   }
   else  
      vi->do_background = 0;

   if (vi->do_background == 0)
      vi->background_red = vi->background_green = vi->background_blue = 0;

   vi->gamma_correction = 1/(dp->file_gamma*dp->screen_gamma);
   if (fabs(vi->gamma_correction-1) < PNG_GAMMA_THRESHOLD)
      vi->gamma_correction = 0;

   vi->file_inverse = 1/dp->file_gamma;
   if (fabs(vi->file_inverse-1) < PNG_GAMMA_THRESHOLD)
      vi->file_inverse = 0;

   vi->scale16 = dp->scale16;
}

                                                                          
                                                                              
                                                                                
                                                                     
   
static double
gamma_component_compose(int do_background, double input_sample, double alpha,
   double background, int *compose)
{
   switch (do_background)
   {
#ifdef PNG_READ_BACKGROUND_SUPPORTED
      case PNG_BACKGROUND_GAMMA_SCREEN:
      case PNG_BACKGROUND_GAMMA_FILE:
      case PNG_BACKGROUND_GAMMA_UNIQUE:
          
         if (alpha < 1)
         {
            if (alpha > 0)
            {
               input_sample = input_sample * alpha + background * (1-alpha);
               if (compose != NULL)
                  *compose = 1;
            }

            else
               input_sample = background;
         }
         break;
#endif

#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
      case ALPHA_MODE_OFFSET + PNG_ALPHA_STANDARD:
      case ALPHA_MODE_OFFSET + PNG_ALPHA_BROKEN:
                                                                             
                                                                           
                                    
            
      case ALPHA_MODE_OFFSET + PNG_ALPHA_OPTIMIZED:
                                                                         
                                                                                
                            
            
         if (alpha < 1)
         {
            if (alpha > 0)
            {
               input_sample *= alpha;
               if (compose != NULL)
                  *compose = 1;
            }

            else
               input_sample = 0;
         }
         break;
#endif

      default:
                                                                         
                                      
            
         UNUSED(alpha)
         UNUSED(background)
         UNUSED(compose)
         break;
   }

   return input_sample;
}

 
static double
gamma_component_validate(const char *name, const validate_info *vi,
    unsigned int id, unsigned int od,
    const double alpha  ,
    const double background  )
{
   unsigned int isbit = id >> vi->isbit_shift;
   unsigned int sbit_max = vi->sbit_max;
   unsigned int outmax = vi->outmax;
   int do_background = vi->do_background;

   double i;

                                                                           
                                                                            
                                                
      
   i = isbit; i /= sbit_max;

                                                                               
                                                                            
                                                                               
      
   if (alpha == 1   || !do_background
#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
      || do_background == ALPHA_MODE_OFFSET + PNG_ALPHA_PNG
#endif
      || (alpha < 0  
#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
      && do_background != ALPHA_MODE_OFFSET + PNG_ALPHA_BROKEN
#endif
      ))
   {
                                                                             
                                                                              
                                                                            
                                        
         
      double encoded_sample = i;
      double encoded_error;

                                                                              
         
      if (alpha >= 0 && vi->gamma_correction > 0)
         encoded_sample = pow(encoded_sample, vi->gamma_correction);
      encoded_sample *= outmax;

      encoded_error = fabs(od-encoded_sample);

      if (encoded_error > vi->dp->maxerrout)
         vi->dp->maxerrout = encoded_error;

      if (encoded_error < vi->maxout_total && encoded_error < vi->outlog)
         return i;
   }

    
                                                                               
                                                                                
                                                                        
                                                                            
                                                                             
                                                                             
      
   {
      double input_sample = i;  
      double output, error, encoded_sample, encoded_error;
      double es_lo, es_hi;
      int compose = 0;            
      int output_is_encoded;      
      int log_max_error = 1;      
      png_const_charp pass = 0;   

                                                                               
                        
         
      if (alpha >= 0)
      {
         int tcompose;

         if (vi->file_inverse > 0)
            input_sample = pow(input_sample, vi->file_inverse);

          
         tcompose = 0;
         input_sample = gamma_component_compose(do_background, input_sample,
            alpha, background, &tcompose);

         if (tcompose)
            compose = 1;
      }

                                                                                
                                            
         
      output = od;
      output /= outmax;

      output_is_encoded = vi->screen_gamma > 0;

      if (alpha < 0)  
      {
#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
         if (do_background != ALPHA_MODE_OFFSET + PNG_ALPHA_BROKEN)
#endif
         {
                                                                             
                                                                          
               
            output_is_encoded = 0;
            log_max_error = 0;
         }
      }

#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
      else  
      {
         if (do_background == ALPHA_MODE_OFFSET + PNG_ALPHA_OPTIMIZED &&
            alpha < 1)  
         {
            if (alpha > 0) log_max_error = 0;
            output_is_encoded = 0;
         }
      }
#endif

      if (output_is_encoded)
         output = pow(output, vi->screen_gamma);

                                                                           
                                                                           
         
      encoded_sample = input_sample;
      if (output_is_encoded)
         encoded_sample = pow(encoded_sample, vi->screen_inverse);
      encoded_sample *= outmax;

      encoded_error = fabs(od-encoded_sample);

                                                                        
                                                           
         
      if (log_max_error && encoded_error > vi->dp->maxerrout)
         vi->dp->maxerrout = encoded_error;

      if (encoded_error < vi->maxout_total)
      {
         if (encoded_error < vi->outlog)
            return i;

                                                                              
                        
            
         pass = "less than maxout:\n";
      }

                                                      
        
                               
                                                                            
                                                                                 
        
                             
                                                                 
                                        
         

                                                                              
                                                  
         
      error = fabs(input_sample-output);

      if (log_max_error && error > vi->dp->maxerrabs)
         vi->dp->maxerrabs = error;

                                                                              
                                                                
         
      if (log_max_error && input_sample > .5)
      {
         double percentage_error = error/input_sample;
         if (percentage_error > vi->dp->maxerrpc)
            vi->dp->maxerrpc = percentage_error;
      }

                                                                             
                                                                              
                                          
        
                                                                        
         
      {
         double tmp = input_sample * vi->maxpc;
         if (tmp < vi->maxabs) tmp = vi->maxabs;
                                                                               
                                                                              
                                                                             
                                                                                
                               
            
         if (compose && tmp < vi->maxcalc) tmp = vi->maxcalc;

                                                                            
                                                                               
            
         es_lo = encoded_sample - vi->maxout;

         if (es_lo > 0 && input_sample-tmp > 0)
         {
            double low_value = input_sample-tmp;
            if (output_is_encoded)
               low_value = pow(low_value, vi->screen_inverse);
            low_value *= outmax;
            if (low_value < es_lo) es_lo = low_value;

             
            es_lo = ceil(es_lo / vi->outquant - .5) * vi->outquant;
         }

         else
            es_lo = 0;

         es_hi = encoded_sample + vi->maxout;

         if (es_hi < outmax && input_sample+tmp < 1)
         {
            double high_value = input_sample+tmp;
            if (output_is_encoded)
               high_value = pow(high_value, vi->screen_inverse);
            high_value *= outmax;
            if (high_value > es_hi) es_hi = high_value;

            es_hi = floor(es_hi / vi->outquant + .5) * vi->outquant;
         }

         else
            es_hi = outmax;
      }

                                                                         
                                                                       
                          
         
      if (od >= es_lo && od <= es_hi)
      {
          
         if (encoded_error < vi->outlog)
            return i;

         if (pass == 0)
            pass = "within digitization limits:\n";
      }

      {
                                                                         
                  
            
         double is_lo, is_hi;

                                                                             
                                                                        
                                           
            
         if (pass == 0 && vi->use_input_precision && vi->dp->sbit)
         {
                                                                               
                                                                             
                                                                                
                                          
              
                                                                             
                                                                             
                                                                           
                                                                              
                                               
               
#           if PNG_LIBPNG_VER < 10700
#              define SBIT_ERROR .5
#           else
#              define SBIT_ERROR 1.
#           endif
            double tmp = (isbit - SBIT_ERROR)/sbit_max;

            if (tmp <= 0)
               tmp = 0;

            else if (alpha >= 0 && vi->file_inverse > 0 && tmp < 1)
               tmp = pow(tmp, vi->file_inverse);

            tmp = gamma_component_compose(do_background, tmp, alpha, background,
               NULL);

            if (output_is_encoded && tmp > 0 && tmp < 1)
               tmp = pow(tmp, vi->screen_inverse);

            is_lo = ceil(outmax * tmp - vi->maxout_total);

            if (is_lo < 0)
               is_lo = 0;

            tmp = (isbit + SBIT_ERROR)/sbit_max;

            if (tmp >= 1)
               tmp = 1;

            else if (alpha >= 0 && vi->file_inverse > 0 && tmp < 1)
               tmp = pow(tmp, vi->file_inverse);

            tmp = gamma_component_compose(do_background, tmp, alpha, background,
               NULL);

            if (output_is_encoded && tmp > 0 && tmp < 1)
               tmp = pow(tmp, vi->screen_inverse);

            is_hi = floor(outmax * tmp + vi->maxout_total);

            if (is_hi > outmax)
               is_hi = outmax;

            if (!(od < is_lo || od > is_hi))
            {
               if (encoded_error < vi->outlog)
                  return i;

               pass = "within input precision limits:\n";
            }

                                                                          
                                                                             
                                                                              
                     
              
                                                                                
                                                 
               
#           ifndef PNG_READ_16_TO_8_ACCURATE_SCALE_SUPPORTED
#              if PNG_LIBPNG_VER < 10504
                                                                             
                                                                                
                                                                                
                                                                              
                                          
                     
                  if (pass == 0 && alpha < 0 && vi->scale16 && vi->sbit > 8 &&
                     vi->sbit + vi->isbit_shift == 16)
                  {
                     tmp = ((id >> 8) - .5)/255;

                     if (tmp > 0)
                     {
                        is_lo = ceil(outmax * tmp - vi->maxout_total);
                        if (is_lo < 0) is_lo = 0;
                     }

                     else
                        is_lo = 0;

                     tmp = ((id >> 8) + .5)/255;

                     if (tmp < 1)
                     {
                        is_hi = floor(outmax * tmp + vi->maxout_total);
                        if (is_hi > outmax) is_hi = outmax;
                     }

                     else
                        is_hi = outmax;

                     if (!(od < is_lo || od > is_hi))
                     {
                        if (encoded_error < vi->outlog)
                           return i;

                        pass = "within 8 bit limits:\n";
                     }
                  }
#              endif
#           endif
         }
         else  
            is_lo = es_lo, is_hi = es_hi;

                                                                             
                                                                                
                                                                          
            
         {
            size_t pos = 0;
                                                                          
                                                          
               
            int precision = (outmax >= 1000 ? 6 : 3);
            int use_input=1, use_background=0, do_compose=0;
            char msg[256];

            if (pass != 0)
               pos = safecat(msg, sizeof msg, pos, "\n\t");

                                                                         
                                                                      
               
            switch (do_background)
            {
#           ifdef PNG_READ_BACKGROUND_SUPPORTED
               case PNG_BACKGROUND_GAMMA_SCREEN:
               case PNG_BACKGROUND_GAMMA_FILE:
               case PNG_BACKGROUND_GAMMA_UNIQUE:
                  use_background = (alpha >= 0 && alpha < 1);
#           endif
#           ifdef PNG_READ_ALPHA_MODE_SUPPORTED
                
               case ALPHA_MODE_OFFSET + PNG_ALPHA_STANDARD:
               case ALPHA_MODE_OFFSET + PNG_ALPHA_BROKEN:
               case ALPHA_MODE_OFFSET + PNG_ALPHA_OPTIMIZED:
#           endif  
               do_compose = (alpha > 0 && alpha < 1);
               use_input = (alpha != 0);
               break;

            default:
               break;
            }

             
            if (compose != do_compose)
               png_error(vi->pp, "internal error (compose)");

             
            pos = safecat(msg, sizeof msg, pos, name);
            pos = safecat(msg, sizeof msg, pos, "(");
            pos = safecatn(msg, sizeof msg, pos, id);
            if (use_input || pass != 0 )
            {
               if (isbit != id)
               {
                   
                  pos = safecat(msg, sizeof msg, pos, ", sbit(");
                  pos = safecatn(msg, sizeof msg, pos, vi->sbit);
                  pos = safecat(msg, sizeof msg, pos, "): ");
                  pos = safecatn(msg, sizeof msg, pos, isbit);
               }
               pos = safecat(msg, sizeof msg, pos, "/");
                
               pos = safecatn(msg, sizeof msg, pos, vi->sbit_max);
            }
            pos = safecat(msg, sizeof msg, pos, ")");

                                                                            
                                                                    
               
            if (compose || pass != 0)
            {
                                                                     
                                                                             
                                                                          
                                                                
                  
               if (use_input || pass != 0)
               {
                  if (vi->file_inverse > 0)
                  {
                     pos = safecat(msg, sizeof msg, pos, "^");
                     pos = safecatd(msg, sizeof msg, pos, vi->file_inverse, 2);
                  }

                  else
                     pos = safecat(msg, sizeof msg, pos, "[linear]");

                  pos = safecat(msg, sizeof msg, pos, "*(alpha)");
                  pos = safecatd(msg, sizeof msg, pos, alpha, precision);
               }

                                                                         
                                                                        
                                                              
                  
               if (use_background)
               {
                  pos = safecat(msg, sizeof msg, pos, use_input ? "+" : " ");
                  pos = safecat(msg, sizeof msg, pos, "(background)");
                  pos = safecatd(msg, sizeof msg, pos, background, precision);
                  pos = safecat(msg, sizeof msg, pos, "*");
                  pos = safecatd(msg, sizeof msg, pos, 1-alpha, precision);
               }
            }

                                                                            
                                                                          
                          
               
            if (compose || alpha < 0 || pass != 0)
            {
               pos = safecat(msg, sizeof msg, pos,
                  pass != 0 ? " =\n\t" : " = ");
               pos = safecatd(msg, sizeof msg, pos, input_sample, precision);
               pos = safecat(msg, sizeof msg, pos, " (libpng: ");
               pos = safecatd(msg, sizeof msg, pos, output, precision);
               pos = safecat(msg, sizeof msg, pos, ")");

                
               if (output_is_encoded)
               {
                  pos = safecat(msg, sizeof msg, pos, " ^");
                  pos = safecatd(msg, sizeof msg, pos, vi->screen_inverse, 2);
                  pos = safecat(msg, sizeof msg, pos, "(to screen) =");
               }

               else
                  pos = safecat(msg, sizeof msg, pos, " [screen is linear] =");
            }

            if ((!compose && alpha >= 0) || pass != 0)
            {
               if (pass != 0)  
                  pos = safecat(msg, sizeof msg, pos, "\n\t[overall:");

                                                                       
                                                                         
                                                                                
                                               
                  
               if (vi->gamma_correction > 0)
               {
                  pos = safecat(msg, sizeof msg, pos, " ^");
                  pos = safecatd(msg, sizeof msg, pos, vi->gamma_correction, 2);
                  pos = safecat(msg, sizeof msg, pos, "(gamma correction) =");
               }

               else
                  pos = safecat(msg, sizeof msg, pos,
                     " [no gamma correction] =");

               if (pass != 0)
                  pos = safecat(msg, sizeof msg, pos, "]");
            }

                                                                           
                             
               
            pos = safecat(msg, sizeof msg, pos, pass != 0 ? "\n\t" : " ");
            pos = safecatd(msg, sizeof msg, pos, is_lo, 1);
            pos = safecat(msg, sizeof msg, pos, " < ");
            pos = safecatd(msg, sizeof msg, pos, encoded_sample, 1);
            pos = safecat(msg, sizeof msg, pos, " (libpng: ");
            pos = safecatn(msg, sizeof msg, pos, od);
            pos = safecat(msg, sizeof msg, pos, ")");
            pos = safecat(msg, sizeof msg, pos, "/");
            pos = safecatn(msg, sizeof msg, pos, outmax);
            pos = safecat(msg, sizeof msg, pos, " < ");
            pos = safecatd(msg, sizeof msg, pos, is_hi, 1);

            if (pass == 0)  
            {
#              ifdef PNG_WARNINGS_SUPPORTED
                  png_warning(vi->pp, msg);
#              else
                  store_warning(vi->pp, msg);
#              endif
            }

            else  
               store_verbose(&vi->dp->pm->this, vi->pp, pass, msg);
         }
      }
   }

   return i;
}

static void
gamma_image_validate(gamma_display *dp, png_const_structp pp,
   png_infop pi)
{
    
   const png_store* const ps = dp->this.ps;
   png_byte in_ct = dp->this.colour_type;
   png_byte in_bd = dp->this.bit_depth;
   png_uint_32 w = dp->this.w;
   png_uint_32 h = dp->this.h;
   const size_t cbRow = dp->this.cbRow;
   png_byte out_ct = png_get_color_type(pp, pi);
   png_byte out_bd = png_get_bit_depth(pp, pi);

                                                                       
                                                                       
                                                                     
     
                                                                 
                                                                          
                                                                         
                                                                    
                                                                          
                          
     
                                                                         
                                                                       
                                                                    
                                                                   
                  
     
                                                                     
                                                                      
                                                                       
              
     
                                                                    
     
                                                                     
                                                                        
                                                                    
                                         
     
                                                                      
                                                             
                                                                    
                                                     
     
                                                                         
                                                                         
                                                                            
                                                  
      
   unsigned int samples_per_pixel = (out_ct & 2U) ? 3U : 1U;
   int processing;
   png_uint_32 y;
   const store_palette_entry *in_palette = dp->this.palette;
   int in_is_transparent = dp->this.is_transparent;
   int process_tRNS;
   int out_npalette = -1;
   int out_is_transparent = 0;  
   store_palette out_palette;
   validate_info vi;

    
   store_image_check(dp->this.ps, pp, 0);

                                                                              
                              
      
   init_validate_info(&vi, dp, pp, in_ct==3?8:in_bd, out_ct==3?8:out_bd);

   processing = (vi.gamma_correction > 0 && !dp->threshold_test)
      || in_bd != out_bd || in_ct != out_ct || vi.do_background;
   process_tRNS = dp->this.has_tRNS && vi.do_background;

                                                                          
                                                                         
                                                                         
                                                                          
                                                                        
                        
      
   if (in_ct == 3 && out_ct == 3)
      processing = 0;

   if (processing && out_ct == 3)
      out_is_transparent = read_palette(out_palette, &out_npalette, pp, pi);

   for (y=0; y<h; ++y)
   {
      png_const_bytep pRow = store_image_row(ps, pp, 0, y);
      png_byte std[STANDARD_ROWMAX];

      transform_row(pp, std, in_ct, in_bd, y);

      if (processing)
      {
         unsigned int x;

         for (x=0; x<w; ++x)
         {
            double alpha = 1;  

             
            unsigned int in_index =
               in_ct == 3 ? sample(std, 3, in_bd, x, 0, 0, 0) : 256;
            unsigned int out_index =
               out_ct == 3 ? sample(std, 3, out_bd, x, 0, 0, 0) : 256;

                                                                            
                                                               
               
            if ((in_ct & PNG_COLOR_MASK_ALPHA) != 0 ||
                (in_ct == 3 && in_is_transparent))
            {
               unsigned int input_alpha = in_ct == 3 ?
                  dp->this.palette[in_index].alpha :
                  sample(std, in_ct, in_bd, x, samples_per_pixel, 0, 0);

               unsigned int output_alpha = 65536  ;

               if (out_ct == 3)
               {
                  if (out_is_transparent)
                     output_alpha = out_palette[out_index].alpha;
               }

               else if ((out_ct & PNG_COLOR_MASK_ALPHA) != 0)
                  output_alpha = sample(pRow, out_ct, out_bd, x,
                     samples_per_pixel, 0, 0);

               if (output_alpha != 65536)
                  alpha = gamma_component_validate("alpha", &vi, input_alpha,
                     output_alpha, -1 , 0 );

               else  
               {
                                                                               
                                                                               
                     
                  alpha = input_alpha >> vi.isbit_shift;
                  alpha /= vi.sbit_max;
               }
            }

            else if (process_tRNS)
            {
                                                                           
                                                                                
                                     
                  
               switch (in_ct)
               {
                  case 0:  
                     if (sample(std, in_ct, in_bd, x, 0, 0, 0) ==
                           dp->this.transparent.red)
                        alpha = 0;
                     break;

                  case 2:  
                     if (sample(std, in_ct, in_bd, x, 0, 0, 0) ==
                           dp->this.transparent.red &&
                         sample(std, in_ct, in_bd, x, 1, 0, 0) ==
                           dp->this.transparent.green &&
                         sample(std, in_ct, in_bd, x, 2, 0, 0) ==
                           dp->this.transparent.blue)
                        alpha = 0;
                     break;

                  default:
                     break;
               }
            }

             
            if ((in_ct & PNG_COLOR_MASK_COLOR) == 0)  
               (void)gamma_component_validate("gray", &vi,
                  sample(std, in_ct, in_bd, x, 0, 0, 0),
                  sample(pRow, out_ct, out_bd, x, 0, 0, 0),
                  alpha , vi.background_red);
            else  
            {
               (void)gamma_component_validate("red", &vi,
                  in_ct == 3 ? in_palette[in_index].red :
                     sample(std, in_ct, in_bd, x, 0, 0, 0),
                  out_ct == 3 ? out_palette[out_index].red :
                     sample(pRow, out_ct, out_bd, x, 0, 0, 0),
                  alpha , vi.background_red);

               (void)gamma_component_validate("green", &vi,
                  in_ct == 3 ? in_palette[in_index].green :
                     sample(std, in_ct, in_bd, x, 1, 0, 0),
                  out_ct == 3 ? out_palette[out_index].green :
                     sample(pRow, out_ct, out_bd, x, 1, 0, 0),
                  alpha , vi.background_green);

               (void)gamma_component_validate("blue", &vi,
                  in_ct == 3 ? in_palette[in_index].blue :
                     sample(std, in_ct, in_bd, x, 2, 0, 0),
                  out_ct == 3 ? out_palette[out_index].blue :
                     sample(pRow, out_ct, out_bd, x, 2, 0, 0),
                  alpha , vi.background_blue);
            }
         }
      }

      else if (memcmp(std, pRow, cbRow) != 0)
      {
         char msg[64];

          
         sprintf(msg, "gamma: below threshold row %lu changed",
            (unsigned long)y);

         png_error(pp, msg);
      }
   }  

   dp->this.ps->validated = 1;
}

static void PNGCBAPI
gamma_end(png_structp ppIn, png_infop pi)
{
   png_const_structp pp = ppIn;
   gamma_display *dp = voidcast(gamma_display*, png_get_progressive_ptr(pp));

   if (!dp->this.speed)
      gamma_image_validate(dp, pp, pi);
   else
      dp->this.ps->validated = 1;
}

                                                     
  
                                               
                                                   
                                                     
   
static void
gamma_test(png_modifier *pmIn, png_byte colour_typeIn,
    png_byte bit_depthIn, int palette_numberIn,
    int interlace_typeIn,
    const double file_gammaIn, const double screen_gammaIn,
    png_byte sbitIn, int threshold_testIn,
    const char *name,
    int use_input_precisionIn, int scale16In,
    int expand16In, int do_backgroundIn,
    const png_color_16 *bkgd_colorIn, double bkgd_gammaIn)
{
   gamma_display d;
   context(&pmIn->this, fault);

   gamma_display_init(&d, pmIn, FILEID(colour_typeIn, bit_depthIn,
      palette_numberIn, interlace_typeIn, 0, 0, 0),
      file_gammaIn, screen_gammaIn, sbitIn,
      threshold_testIn, use_input_precisionIn, scale16In,
      expand16In, do_backgroundIn, bkgd_colorIn, bkgd_gammaIn);

   Try
   {
      png_structp pp;
      png_infop pi;
      gama_modification gama_mod;
      srgb_modification srgb_mod;
      sbit_modification sbit_mod;

       
      d.pm->encoding_counter = 0;
      modifier_set_encoding(d.pm);  
      d.pm->current_gamma = d.file_gamma;

                                                                      
                                                                     
         
      d.pm->modifications = NULL;
      gama_modification_init(&gama_mod, d.pm, d.file_gamma);
      srgb_modification_init(&srgb_mod, d.pm, 127  );
      if (d.sbit > 0)
         sbit_modification_init(&sbit_mod, d.pm, d.sbit);

      modification_reset(d.pm->modifications);

       
      pp = set_modifier_for_read(d.pm, &pi, d.this.id, name);
      standard_palette_init(&d.this);

       
      if (d.pm->this.progressive)
      {
          
         png_set_progressive_read_fn(pp, &d, gamma_info, progressive_row,
            gamma_end);

          
         modifier_progressive_read(d.pm, pp, pi);
      }
      else
      {
          
         png_set_read_fn(pp, d.pm, modifier_read);

          
         png_read_info(pp, pi);

          
         gamma_info_imp(&d, pp, pi);

         sequential_row(&d.this, pp, pi, -1, 0);

         if (!d.this.speed)
            gamma_image_validate(&d, pp, pi);
         else
            d.this.ps->validated = 1;
      }

      modifier_reset(d.pm);

      if (d.pm->log && !d.threshold_test && !d.this.speed)
         fprintf(stderr, "%d bit %s %s: max error %f (%.2g, %2g%%)\n",
            d.this.bit_depth, colour_types[d.this.colour_type], name,
            d.maxerrout, d.maxerrabs, 100*d.maxerrpc);

       
      if (d.this.colour_type == 0 || d.this.colour_type == 4)
      {
         switch (d.this.bit_depth)
         {
         case 1:
            break;

         case 2:
            if (d.maxerrout > d.pm->error_gray_2)
               d.pm->error_gray_2 = d.maxerrout;

            break;

         case 4:
            if (d.maxerrout > d.pm->error_gray_4)
               d.pm->error_gray_4 = d.maxerrout;

            break;

         case 8:
            if (d.maxerrout > d.pm->error_gray_8)
               d.pm->error_gray_8 = d.maxerrout;

            break;

         case 16:
            if (d.maxerrout > d.pm->error_gray_16)
               d.pm->error_gray_16 = d.maxerrout;

            break;

         default:
            png_error(pp, "bad bit depth (internal: 1)");
         }
      }

      else if (d.this.colour_type == 2 || d.this.colour_type == 6)
      {
         switch (d.this.bit_depth)
         {
         case 8:

            if (d.maxerrout > d.pm->error_color_8)
               d.pm->error_color_8 = d.maxerrout;

            break;

         case 16:

            if (d.maxerrout > d.pm->error_color_16)
               d.pm->error_color_16 = d.maxerrout;

            break;

         default:
            png_error(pp, "bad bit depth (internal: 2)");
         }
      }

      else if (d.this.colour_type == 3)
      {
         if (d.maxerrout > d.pm->error_indexed)
            d.pm->error_indexed = d.maxerrout;
      }
   }

   Catch(fault)
      modifier_reset(voidcast(png_modifier*,(void*)fault));
}

static void gamma_threshold_test(png_modifier *pm, png_byte colour_type,
    png_byte bit_depth, int interlace_type, double file_gamma,
    double screen_gamma)
{
   size_t pos = 0;
   char name[64];
   pos = safecat(name, sizeof name, pos, "threshold ");
   pos = safecatd(name, sizeof name, pos, file_gamma, 3);
   pos = safecat(name, sizeof name, pos, "/");
   pos = safecatd(name, sizeof name, pos, screen_gamma, 3);

   (void)gamma_test(pm, colour_type, bit_depth, 0 , interlace_type,
      file_gamma, screen_gamma, 0 , 1 , name,
      0  ,
      0  , 0  , 0  , 0  ,
      0  );
}

static void
perform_gamma_threshold_tests(png_modifier *pm)
{
   png_byte colour_type = 0;
   png_byte bit_depth = 0;
   unsigned int palette_number = 0;

                                                                            
                                                                         
                                                 
     
                                                                           
                   
      
   while (next_format(&colour_type, &bit_depth, &palette_number,
                      pm->test_lbg_gamma_threshold, pm->test_tRNS))
      if (palette_number < 2)
   {
      double test_gamma = 1.0;
      while (test_gamma >= .4)
      {
                                                                            
                                                      
            
         gamma_threshold_test(pm, colour_type, bit_depth, pm->interlace_type,
            test_gamma, 1/test_gamma);
         test_gamma *= .95;
      }

       
      gamma_threshold_test(pm, colour_type, bit_depth, pm->interlace_type,
          .45455, 2.2);

      if (fail(pm))
         return;
   }
}

static void gamma_transform_test(png_modifier *pm,
   png_byte colour_type, png_byte bit_depth,
   int palette_number,
   int interlace_type, const double file_gamma,
   const double screen_gamma, png_byte sbit,
   int use_input_precision, int scale16)
{
   size_t pos = 0;
   char name[64];

   if (sbit != bit_depth && sbit != 0)
   {
      pos = safecat(name, sizeof name, pos, "sbit(");
      pos = safecatn(name, sizeof name, pos, sbit);
      pos = safecat(name, sizeof name, pos, ") ");
   }

   else
      pos = safecat(name, sizeof name, pos, "gamma ");

   if (scale16)
      pos = safecat(name, sizeof name, pos, "16to8 ");

   pos = safecatd(name, sizeof name, pos, file_gamma, 3);
   pos = safecat(name, sizeof name, pos, "->");
   pos = safecatd(name, sizeof name, pos, screen_gamma, 3);

   gamma_test(pm, colour_type, bit_depth, palette_number, interlace_type,
      file_gamma, screen_gamma, sbit, 0, name, use_input_precision,
      scale16, pm->test_gamma_expand16, 0 , 0, 0);
}

static void perform_gamma_transform_tests(png_modifier *pm)
{
   png_byte colour_type = 0;
   png_byte bit_depth = 0;
   unsigned int palette_number = 0;

   while (next_format(&colour_type, &bit_depth, &palette_number,
                      pm->test_lbg_gamma_transform, pm->test_tRNS))
   {
      unsigned int i, j;

      for (i=0; i<pm->ngamma_tests; ++i) for (j=0; j<pm->ngamma_tests; ++j)
         if (i != j)
         {
            gamma_transform_test(pm, colour_type, bit_depth, palette_number,
               pm->interlace_type, 1/pm->gammas[i], pm->gammas[j], 0 ,
               pm->use_input_precision, 0  );

            if (fail(pm))
               return;
         }
   }
}

static void perform_gamma_sbit_tests(png_modifier *pm)
{
   png_byte sbit;

                                                                                
                                                                               
                 
      
   for (sbit=pm->sbitlow; sbit<(1<<READ_BDHI); ++sbit)
   {
      png_byte colour_type = 0, bit_depth = 0;
      unsigned int npalette = 0;

      while (next_format(&colour_type, &bit_depth, &npalette,
                         pm->test_lbg_gamma_sbit, pm->test_tRNS))
         if ((colour_type & PNG_COLOR_MASK_ALPHA) == 0 &&
            ((colour_type == 3 && sbit < 8) ||
            (colour_type != 3 && sbit < bit_depth)))
      {
         unsigned int i;

         for (i=0; i<pm->ngamma_tests; ++i)
         {
            unsigned int j;

            for (j=0; j<pm->ngamma_tests; ++j) if (i != j)
            {
               gamma_transform_test(pm, colour_type, bit_depth, npalette,
                  pm->interlace_type, 1/pm->gammas[i], pm->gammas[j],
                  sbit, pm->use_input_precision_sbit, 0  );

               if (fail(pm))
                  return;
            }
         }
      }
   }
}

                                                                              
                                                                       
                                    
   
#ifdef DO_16BIT
static void perform_gamma_scale16_tests(png_modifier *pm)
{
#  ifndef PNG_MAX_GAMMA_8
#     define PNG_MAX_GAMMA_8 11
#  endif
#  if defined PNG_MAX_GAMMA_8 || PNG_LIBPNG_VER < 10700
#     define SBIT_16_TO_8 PNG_MAX_GAMMA_8
#  else
#     define SBIT_16_TO_8 16
#  endif
                                                                             
                                                                          
                                        
     
                                                                              
                                                                                
                                                                         
      
   unsigned int i, j;
   for (i=0; i<pm->ngamma_tests; ++i)
   {
      for (j=0; j<pm->ngamma_tests; ++j)
      {
         if (i != j &&
             fabs(pm->gammas[j]/pm->gammas[i]-1) >= PNG_GAMMA_THRESHOLD)
         {
            gamma_transform_test(pm, 0, 16, 0, pm->interlace_type,
               1/pm->gammas[i], pm->gammas[j], SBIT_16_TO_8,
               pm->use_input_precision_16to8, 1  );

            if (fail(pm))
               return;

            gamma_transform_test(pm, 2, 16, 0, pm->interlace_type,
               1/pm->gammas[i], pm->gammas[j], SBIT_16_TO_8,
               pm->use_input_precision_16to8, 1  );

            if (fail(pm))
               return;

            gamma_transform_test(pm, 4, 16, 0, pm->interlace_type,
               1/pm->gammas[i], pm->gammas[j], SBIT_16_TO_8,
               pm->use_input_precision_16to8, 1  );

            if (fail(pm))
               return;

            gamma_transform_test(pm, 6, 16, 0, pm->interlace_type,
               1/pm->gammas[i], pm->gammas[j], SBIT_16_TO_8,
               pm->use_input_precision_16to8, 1  );

            if (fail(pm))
               return;
         }
      }
   }
}
#endif  

#if defined(PNG_READ_BACKGROUND_SUPPORTED) ||\
   defined(PNG_READ_ALPHA_MODE_SUPPORTED)
static void gamma_composition_test(png_modifier *pm,
   png_byte colour_type, png_byte bit_depth,
   int palette_number,
   int interlace_type, const double file_gamma,
   const double screen_gamma,
   int use_input_precision, int do_background,
   int expand_16)
{
   size_t pos = 0;
   png_const_charp base;
   double bg;
   char name[128];
   png_color_16 background;

    
   switch (do_background)
   {
      default:
         base = "";
         bg = 4;  
         break;
      case PNG_BACKGROUND_GAMMA_SCREEN:
         base = " bckg(Screen):";
         bg = 1/screen_gamma;
         break;
      case PNG_BACKGROUND_GAMMA_FILE:
         base = " bckg(File):";
         bg = file_gamma;
         break;
      case PNG_BACKGROUND_GAMMA_UNIQUE:
         base = " bckg(Unique):";
                                                                                
                                                                              
                   
            
         bg = (file_gamma + screen_gamma) / 3;
         break;
#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
      case ALPHA_MODE_OFFSET + PNG_ALPHA_PNG:
         base = " alpha(PNG)";
         bg = 4;  
         break;
      case ALPHA_MODE_OFFSET + PNG_ALPHA_STANDARD:
         base = " alpha(Porter-Duff)";
         bg = 4;  
         break;
      case ALPHA_MODE_OFFSET + PNG_ALPHA_OPTIMIZED:
         base = " alpha(Optimized)";
         bg = 4;  
         break;
      case ALPHA_MODE_OFFSET + PNG_ALPHA_BROKEN:
         base = " alpha(Broken)";
         bg = 4;  
         break;
#endif
   }

                                                                              
                                            
      
   if (expand_16 || bit_depth == 16)
   {
      png_uint_32 r = random_32();

      background.red = (png_uint_16)r;
      background.green = (png_uint_16)(r >> 16);
      r = random_32();
      background.blue = (png_uint_16)r;
      background.gray = (png_uint_16)(r >> 16);

                                                                                
                                                                         
                                                                           
                                                                         
                    
         
#     if DIGITIZE
         if (expand_16 && (do_background == PNG_BACKGROUND_GAMMA_UNIQUE ||
                           do_background == PNG_BACKGROUND_GAMMA_FILE) &&
            fabs(bg*screen_gamma-1) > PNG_GAMMA_THRESHOLD)
         {
                                                                              
                                                                             
                                                 
               
            background.red = (png_uint_16)((background.red >> 8) * 257);
            background.green = (png_uint_16)((background.green >> 8) * 257);
            background.blue = (png_uint_16)((background.blue >> 8) * 257);
            background.gray = (png_uint_16)((background.gray >> 8) * 257);
         }
#     endif
   }

   else  
   {
      png_uint_32 r = random_32();

      background.red = (png_byte)r;
      background.green = (png_byte)(r >> 8);
      background.blue = (png_byte)(r >> 16);
      background.gray = (png_byte)(r >> 24);
   }

   background.index = 193;  

   if (!(colour_type & PNG_COLOR_MASK_COLOR))
   {
                                                                     
                                                                           
                                                                              
                                         
         
      if (bit_depth < 8)
         background.gray &= (png_uint_16)((1U << bit_depth)-1);

                                                                            
                                                        
         
      background.red = background.green = background.blue = background.gray;
   }

   pos = safecat(name, sizeof name, pos, "gamma ");
   pos = safecatd(name, sizeof name, pos, file_gamma, 3);
   pos = safecat(name, sizeof name, pos, "->");
   pos = safecatd(name, sizeof name, pos, screen_gamma, 3);

   pos = safecat(name, sizeof name, pos, base);
   if (do_background < ALPHA_MODE_OFFSET)
   {
       
      pos = safecat(name, sizeof name, pos, "(");
                                                                               
         
      if (colour_type & PNG_COLOR_MASK_COLOR)
      {
         pos = safecatn(name, sizeof name, pos, background.red);
         pos = safecat(name, sizeof name, pos, ",");
         pos = safecatn(name, sizeof name, pos, background.green);
         pos = safecat(name, sizeof name, pos, ",");
         pos = safecatn(name, sizeof name, pos, background.blue);
      }
      else
         pos = safecatn(name, sizeof name, pos, background.gray);
      pos = safecat(name, sizeof name, pos, ")^");
      pos = safecatd(name, sizeof name, pos, bg, 3);
   }

   gamma_test(pm, colour_type, bit_depth, palette_number, interlace_type,
      file_gamma, screen_gamma, 0 , 0, name, use_input_precision,
      0 , expand_16, do_background, &background, bg);
}


static void
perform_gamma_composition_tests(png_modifier *pm, int do_background,
   int expand_16)
{
   png_byte colour_type = 0;
   png_byte bit_depth = 0;
   unsigned int palette_number = 0;

                                                                                
              
     
                                                                           
                                                                         
      
   while (next_format(&colour_type, &bit_depth, &palette_number,
                      pm->test_lbg_gamma_composition, pm->test_tRNS))
      if ((colour_type & PNG_COLOR_MASK_ALPHA) != 0
#if 0  
           
          || colour_type == 3
#endif
          || (colour_type != 3 && palette_number != 0))
   {
      unsigned int i, j;

       
      for (i=0; i<pm->ngamma_tests; ++i) for (j=0; j<pm->ngamma_tests; ++j)
      {
         gamma_composition_test(pm, colour_type, bit_depth, palette_number,
            pm->interlace_type, 1/pm->gammas[i], pm->gammas[j],
            pm->use_input_precision, do_background, expand_16);

         if (fail(pm))
            return;
      }
   }
}
#endif  

static void
init_gamma_errors(png_modifier *pm)
{
    
   pm->error_gray_2 = pm->error_gray_4 = pm->error_gray_8 = -1.;
   pm->error_color_8 = -1.;
   pm->error_indexed = -1.;
   pm->error_gray_16 = pm->error_color_16 = -1.;
}

static void
print_one(const char *leader, double err)
{
   if (err != -1.)
      printf(" %s %.5f\n", leader, err);
}

static void
summarize_gamma_errors(png_modifier *pm, png_const_charp who, int low_bit_depth,
   int indexed)
{
   fflush(stderr);

   if (who)
      printf("\nGamma correction with %s:\n", who);

   else
      printf("\nBasic gamma correction:\n");

   if (low_bit_depth)
   {
      print_one(" 2 bit gray: ", pm->error_gray_2);
      print_one(" 4 bit gray: ", pm->error_gray_4);
      print_one(" 8 bit gray: ", pm->error_gray_8);
      print_one(" 8 bit color:", pm->error_color_8);
      if (indexed)
         print_one(" indexed:    ", pm->error_indexed);
   }

   print_one("16 bit gray: ", pm->error_gray_16);
   print_one("16 bit color:", pm->error_color_16);

   fflush(stdout);
}

static void
perform_gamma_test(png_modifier *pm, int summary)
{
    
    
   unsigned int calculations_use_input_precision =
      pm->calculations_use_input_precision;
#  ifdef PNG_READ_BACKGROUND_SUPPORTED
      double maxout8 = pm->maxout8;
#  endif

    
   if (!pm->this.speed && pm->test_gamma_threshold)
   {
      perform_gamma_threshold_tests(pm);

      if (fail(pm))
         return;
   }

    
   if (pm->test_gamma_transform)
   {
      if (summary)
      {
         fflush(stderr);
         printf("Gamma correction error summary\n\n");
         printf("The printed value is the maximum error in the pixel values\n");
         printf("calculated by the libpng gamma correction code.  The error\n");
         printf("is calculated as the difference between the output pixel\n");
         printf("value (always an integer) and the ideal value from the\n");
         printf("libpng specification (typically not an integer).\n\n");

         printf("Expect this value to be less than .5 for 8 bit formats,\n");
         printf("less than 1 for formats with fewer than 8 bits and a small\n");
         printf("number (typically less than 5) for the 16 bit formats.\n");
         printf("For performance reasons the value for 16 bit formats\n");
         printf("increases when the image file includes an sBIT chunk.\n");
         fflush(stdout);
      }

      init_gamma_errors(pm);
                                                                
                                        
         
      if (pm->test_gamma_expand16)
         pm->calculations_use_input_precision = 1;
      perform_gamma_transform_tests(pm);
      if (!calculations_use_input_precision)
         pm->calculations_use_input_precision = 0;

      if (summary)
         summarize_gamma_errors(pm, 0 , 1 , 1 );

      if (fail(pm))
         return;
   }

    
   if (pm->test_gamma_sbit)
   {
      init_gamma_errors(pm);
      perform_gamma_sbit_tests(pm);

      if (summary)
         summarize_gamma_errors(pm, "sBIT", pm->sbitlow < 8U, 1 );

      if (fail(pm))
         return;
   }

#ifdef DO_16BIT  
   if (pm->test_gamma_scale16)
   {
       
      init_gamma_errors(pm);
      perform_gamma_scale16_tests(pm);

      if (summary)
      {
         fflush(stderr);
         printf("\nGamma correction with 16 to 8 bit reduction:\n");
         printf(" 16 bit gray:  %.5f\n", pm->error_gray_16);
         printf(" 16 bit color: %.5f\n", pm->error_color_16);
         fflush(stdout);
      }

      if (fail(pm))
         return;
   }
#endif

#ifdef PNG_READ_BACKGROUND_SUPPORTED
   if (pm->test_gamma_background)
   {
      init_gamma_errors(pm);

                                                                
                                        
         
      if (pm->test_gamma_expand16)
      {
         pm->calculations_use_input_precision = 1;
         pm->maxout8 = .499;  
      }
      perform_gamma_composition_tests(pm, PNG_BACKGROUND_GAMMA_UNIQUE,
         pm->test_gamma_expand16);
      if (!calculations_use_input_precision)
         pm->calculations_use_input_precision = 0;
      pm->maxout8 = maxout8;

      if (summary)
         summarize_gamma_errors(pm, "background", 1, 0 );

      if (fail(pm))
         return;
   }
#endif

#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
   if (pm->test_gamma_alpha_mode)
   {
      int do_background;

      init_gamma_errors(pm);

                                                                
                                        
         
      if (pm->test_gamma_expand16)
         pm->calculations_use_input_precision = 1;
      for (do_background = ALPHA_MODE_OFFSET + PNG_ALPHA_STANDARD;
         do_background <= ALPHA_MODE_OFFSET + PNG_ALPHA_BROKEN && !fail(pm);
         ++do_background)
         perform_gamma_composition_tests(pm, do_background,
            pm->test_gamma_expand16);
      if (!calculations_use_input_precision)
         pm->calculations_use_input_precision = 0;

      if (summary)
         summarize_gamma_errors(pm, "alpha mode", 1, 0 );

      if (fail(pm))
         return;
   }
#endif
}
#endif  
#endif  

 
                                                                        
                                                                       
                                                                       
                                   
   
static const
png_byte adam7[8][8] =
{
   { 1,6,4,6,2,6,4,6 },
   { 7,7,7,7,7,7,7,7 },
   { 5,6,5,6,5,6,5,6 },
   { 7,7,7,7,7,7,7,7 },
   { 3,6,4,6,3,6,4,6 },
   { 7,7,7,7,7,7,7,7 },
   { 5,6,5,6,5,6,5,6 },
   { 7,7,7,7,7,7,7,7 }
};

                                                                       
                                                                            
                                                         
   
static png_uint_32
png_pass_start_row(int pass)
{
   int x, y;
   ++pass;
   for (y=0; y<8; ++y) for (x=0; x<8; ++x) if (adam7[y][x] == pass)
      return y;
   return 0xf;
}

static png_uint_32
png_pass_start_col(int pass)
{
   int x, y;
   ++pass;
   for (x=0; x<8; ++x) for (y=0; y<8; ++y) if (adam7[y][x] == pass)
      return x;
   return 0xf;
}

static int
png_pass_row_shift(int pass)
{
   int x, y, base=(-1), inc=8;
   ++pass;
   for (y=0; y<8; ++y) for (x=0; x<8; ++x) if (adam7[y][x] == pass)
   {
      if (base == (-1))
         base = y;
      else if (base == y)
         {}
      else if (inc == y-base)
         base=y;
      else if (inc == 8)
         inc = y-base, base=y;
      else if (inc != y-base)
         return 0xff;  
   }

   if (base == (-1)) return 0xfe;  

    
   switch (inc)
   {
case 2: return 1;
case 4: return 2;
case 8: return 3;
default: break;
   }

    
   return (inc << 8) + 0xfd;
}

static int
png_pass_col_shift(int pass)
{
   int x, y, base=(-1), inc=8;
   ++pass;
   for (x=0; x<8; ++x) for (y=0; y<8; ++y) if (adam7[y][x] == pass)
   {
      if (base == (-1))
         base = x;
      else if (base == x)
         {}
      else if (inc == x-base)
         base=x;
      else if (inc == 8)
         inc = x-base, base=x;
      else if (inc != x-base)
         return 0xff;  
   }

   if (base == (-1)) return 0xfe;  

    
   switch (inc)
   {
case 1: return 0;  
case 2: return 1;
case 4: return 2;
case 8: return 3;
default: break;
   }

    
   return (inc << 8) + 0xfd;
}

static png_uint_32
png_row_from_pass_row(png_uint_32 yIn, int pass)
{
    
   switch (pass)
   {
case 0: return yIn * 8;
case 1: return yIn * 8;
case 2: return yIn * 8 + 4;
case 3: return yIn * 4;
case 4: return yIn * 4 + 2;
case 5: return yIn * 2;
case 6: return yIn * 2 + 1;
default: break;
   }

   return 0xff;  
}

static png_uint_32
png_col_from_pass_col(png_uint_32 xIn, int pass)
{
    
   switch (pass)
   {
case 0: return xIn * 8;
case 1: return xIn * 8 + 4;
case 2: return xIn * 4;
case 3: return xIn * 4 + 2;
case 4: return xIn * 2;
case 5: return xIn * 2 + 1;
case 6: return xIn;
default: break;
   }

   return 0xff;  
}

static int
png_row_in_interlace_pass(png_uint_32 y, int pass)
{
    
   int x;
   y &= 7;
   ++pass;
   for (x=0; x<8; ++x) if (adam7[y][x] == pass)
      return 1;

   return 0;
}

static int
png_col_in_interlace_pass(png_uint_32 x, int pass)
{
    
   int y;
   x &= 7;
   ++pass;
   for (y=0; y<8; ++y) if (adam7[y][x] == pass)
      return 1;

   return 0;
}

static png_uint_32
png_pass_rows(png_uint_32 height, int pass)
{
   png_uint_32 tiles = height>>3;
   png_uint_32 rows = 0;
   unsigned int x, y;

   height &= 7;
   ++pass;
   for (y=0; y<8; ++y) for (x=0; x<8; ++x) if (adam7[y][x] == pass)
   {
      rows += tiles;
      if (y < height) ++rows;
      break;  
   }

   return rows;
}

static png_uint_32
png_pass_cols(png_uint_32 width, int pass)
{
   png_uint_32 tiles = width>>3;
   png_uint_32 cols = 0;
   unsigned int x, y;

   width &= 7;
   ++pass;
   for (x=0; x<8; ++x) for (y=0; y<8; ++y) if (adam7[y][x] == pass)
   {
      cols += tiles;
      if (x < width) ++cols;
      break;  
   }

   return cols;
}

static void
perform_interlace_macro_validation(void)
{
                                                                   
     
                              
                              
                              
                              
      
   int pass;

   for (pass=0; pass<7; ++pass)
   {
      png_uint_32 m, f, v;

      m = PNG_PASS_START_ROW(pass);
      f = png_pass_start_row(pass);
      if (m != f)
      {
         fprintf(stderr, "PNG_PASS_START_ROW(%d) = %u != %x\n", pass, m, f);
         exit(99);
      }

      m = PNG_PASS_START_COL(pass);
      f = png_pass_start_col(pass);
      if (m != f)
      {
         fprintf(stderr, "PNG_PASS_START_COL(%d) = %u != %x\n", pass, m, f);
         exit(99);
      }

      m = PNG_PASS_ROW_SHIFT(pass);
      f = png_pass_row_shift(pass);
      if (m != f)
      {
         fprintf(stderr, "PNG_PASS_ROW_SHIFT(%d) = %u != %x\n", pass, m, f);
         exit(99);
      }

      m = PNG_PASS_COL_SHIFT(pass);
      f = png_pass_col_shift(pass);
      if (m != f)
      {
         fprintf(stderr, "PNG_PASS_COL_SHIFT(%d) = %u != %x\n", pass, m, f);
         exit(99);
      }

                                                                 
        
                                    
                                   
                                         
                                         
                                           
                                           
         
      for (v=0;;)
      {
                                                                             
                                                                              
                                                                               
                                                                            
                                                                        
                                        
            
          
         if (v < png_pass_rows(0xFFFFFFFFU, pass))
         {
            m = PNG_ROW_FROM_PASS_ROW(v, pass);
            f = png_row_from_pass_row(v, pass);
            if (m != f)
            {
               fprintf(stderr, "PNG_ROW_FROM_PASS_ROW(%u, %d) = %u != %x\n",
                  v, pass, m, f);
               exit(99);
            }
         }

         if (v < png_pass_cols(0xFFFFFFFFU, pass))
         {
            m = PNG_COL_FROM_PASS_COL(v, pass);
            f = png_col_from_pass_col(v, pass);
            if (m != f)
            {
               fprintf(stderr, "PNG_COL_FROM_PASS_COL(%u, %d) = %u != %x\n",
                  v, pass, m, f);
               exit(99);
            }
         }

         m = PNG_ROW_IN_INTERLACE_PASS(v, pass);
         f = png_row_in_interlace_pass(v, pass);
         if (m != f)
         {
            fprintf(stderr, "PNG_ROW_IN_INTERLACE_PASS(%u, %d) = %u != %x\n",
               v, pass, m, f);
            exit(99);
         }

         m = PNG_COL_IN_INTERLACE_PASS(v, pass);
         f = png_col_in_interlace_pass(v, pass);
         if (m != f)
         {
            fprintf(stderr, "PNG_COL_IN_INTERLACE_PASS(%u, %d) = %u != %x\n",
               v, pass, m, f);
            exit(99);
         }

          
         ++v;
         m = PNG_PASS_ROWS(v, pass);
         f = png_pass_rows(v, pass);
         if (m != f)
         {
            fprintf(stderr, "PNG_PASS_ROWS(%u, %d) = %u != %x\n",
               v, pass, m, f);
            exit(99);
         }

         m = PNG_PASS_COLS(v, pass);
         f = png_pass_cols(v, pass);
         if (m != f)
         {
            fprintf(stderr, "PNG_PASS_COLS(%u, %d) = %u != %x\n",
               v, pass, m, f);
            exit(99);
         }

                                                                       
                              
            
         if (v > 1024)
         {
            if (v == PNG_UINT_31_MAX)
               break;

            v = (v << 1) ^ v;
            if (v >= PNG_UINT_31_MAX)
               v = PNG_UINT_31_MAX-1;
         }
      }
   }
}

                                                                            
                                                                              
                                                                               
                                                                             
                                                                             
                                                                               
         
  
                                                                        
                                                         
   
static const color_encoding test_encodings[] =
{
 
  { 1/2.2,
  { 0.412390799265959, 0.212639005871510, 0.019330818715592 },
  { 0.357584339383878, 0.715168678767756, 0.119194779794626 },
  { 0.180480788401834, 0.072192315360734, 0.950532152249660} },
 
  { 1/1.6  ,
  { 0.797760489672303, 0.288071128229293, 0.000000000000000 },
  { 0.135185837175740, 0.711843217810102, 0.000000000000000 },
  { 0.031349349581525, 0.000085653960605, 0.825104602510460} },
 
  { 1/(2+51./256),
  { 0.576669042910131, 0.297344975250536, 0.027031361386412 },
  { 0.185558237906546, 0.627363566255466, 0.070688852535827 },
  { 0.188228646234995, 0.075291458493998, 0.991337536837639} },
 
  { 1/(2+51./256),
  { 0.716500716779386, 0.258728243040113, 0.000000000000000 },
  { 0.101020574397477, 0.724682314948566, 0.051211818965388 },
  { 0.146774385252705, 0.016589442011321, 0.773892783545073} },
 
  { 1.45/2.2,  
  { 0.716500716779386, 0.000000000000000, 0.000000000000000 },
  { 0.101020574397477, 1.000000000000000, 0.051211818965388 },
  { 0.146774385252705, 0.000000000000000, 0.773892783545073} },
};

                 
  
                                                                         
                                                                              
                                                                                
                          
   
static png_modifier pm;

static void signal_handler(int signum)
{

   size_t pos = 0;
   char msg[64];

   pos = safecat(msg, sizeof msg, pos, "caught signal: ");

   switch (signum)
   {
      case SIGABRT:
         pos = safecat(msg, sizeof msg, pos, "abort");
         break;

      case SIGFPE:
         pos = safecat(msg, sizeof msg, pos, "floating point exception");
         break;

      case SIGILL:
         pos = safecat(msg, sizeof msg, pos, "illegal instruction");
         break;

      case SIGINT:
         pos = safecat(msg, sizeof msg, pos, "interrupt");
         break;

      case SIGSEGV:
         pos = safecat(msg, sizeof msg, pos, "invalid memory access");
         break;

      case SIGTERM:
         pos = safecat(msg, sizeof msg, pos, "termination request");
         break;

      default:
         pos = safecat(msg, sizeof msg, pos, "unknown ");
         pos = safecatn(msg, sizeof msg, pos, signum);
         break;
   }

   store_log(&pm.this, NULL , msg, 1 );

                                                                         
                                     
      
   if (signum != SIGTERM)
   {
      struct exception_context *the_exception_context =
         &pm.this.exception_context;

      Throw &pm.this;
   }

   else
      exit(1);
}

 
int main(int argc, char **argv)
{
   int summary = 1;   
   int memstats = 0;  

    
   const char *touch = NULL;

                                                                            
                                              
     
                                                             
      
   static double
      gammas[]={2.2, 1.0, 2.2/1.45, 1.8, 1.5, 2.4, 2.5, 2.62, 2.9};

    
   size_t cp = 0;
   char command[1024];

   anon_context(&pm.this);

   gnu_volatile(summary)
   gnu_volatile(memstats)
   gnu_volatile(touch)

    
   signal(SIGABRT, signal_handler);
   signal(SIGFPE, signal_handler);
   signal(SIGILL, signal_handler);
   signal(SIGINT, signal_handler);
   signal(SIGSEGV, signal_handler);
   signal(SIGTERM, signal_handler);

#ifdef HAVE_FEENABLEEXCEPT
                                                                             
                                                                                
                                                               
      
   feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

   modifier_init(&pm);

                                                                           
                                                                            
                                                                               
                         
      
   store_ensure_image(&pm.this, NULL, 2, TRANSFORM_ROWMAX, TRANSFORM_HEIGHTMAX);

    
   cp = safecat(command, sizeof command, cp, "pngvalid");

    
   pm.this.treat_warnings_as_errors = 1;

                                                                               
                                                                                
                 
      
   pm.assume_16_bit_calculations = PNG_LIBPNG_VER >= 10700;

                                                                           
                                                                  
     
                    
      
   pm.calculations_use_input_precision = 1U;

    
   pm.gammas = gammas;
   pm.ngammas = ARRAY_SIZE(gammas);
   pm.ngamma_tests = 0;  

                                                                       
                                                       
      
#  ifdef PNG_WRITE_tRNS_SUPPORTED
      pm.test_tRNS = 1;
#  endif
   pm.test_lbg = PNG_LIBPNG_VER >= 10600;
   pm.test_lbg_gamma_threshold = 1;
   pm.test_lbg_gamma_transform = PNG_LIBPNG_VER >= 10600;
   pm.test_lbg_gamma_sbit = 1;
   pm.test_lbg_gamma_composition = PNG_LIBPNG_VER >= 10700;

    
   pm.encodings = test_encodings;
   pm.nencodings = ARRAY_SIZE(test_encodings);

#  if PNG_LIBPNG_VER < 10700
      pm.sbitlow = 8U;  
#  else
      pm.sbitlow = 1U;
#  endif

                                                                                
                                                                               
                                                                           
                           
     
                       
      
   pm.use_input_precision_16to8 = 1U;
   pm.use_input_precision_sbit = 1U;  

                                                                   
                                                                          
                                                                               
                                                                           
                                                                             
                                                              
      
   pm.maxout8 = .1;      
   pm.maxabs8 = .00005;  
   pm.maxcalc8 = 1./255;   
   pm.maxpc8 = .499;     
   pm.maxout16 = .499;   
   pm.maxabs16 = .00005; 
   pm.maxcalc16 =1./65535; 
#  if PNG_LIBPNG_VER < 10700
      pm.maxcalcG = 1./((1<<PNG_MAX_GAMMA_8)-1);
#  else
      pm.maxcalcG = 1./((1<<16)-1);
#  endif

                                                                            
                                                                              
                                                                             
                                                                          
                                                                           
                                                                                
                                                              
      
   pm.maxpc16 = .005;    

    
   while (--argc >= 1)
   {
      int catmore = 0;  

       
      cp = safecat(command, sizeof command, cp, " ");
      cp = safecat(command, sizeof command, cp, *++argv);

      if (strcmp(*argv, "-v") == 0)
         pm.this.verbose = 1;

      else if (strcmp(*argv, "-l") == 0)
         pm.log = 1;

      else if (strcmp(*argv, "-q") == 0)
         summary = pm.this.verbose = pm.log = 0;

      else if (strcmp(*argv, "-w") == 0 ||
               strcmp(*argv, "--strict") == 0)
         pm.this.treat_warnings_as_errors = 1;  

      else if (strcmp(*argv, "--nostrict") == 0)
         pm.this.treat_warnings_as_errors = 0;

      else if (strcmp(*argv, "--speed") == 0)
         pm.this.speed = 1, pm.ngamma_tests = pm.ngammas, pm.test_standard = 0,
            summary = 0;

      else if (strcmp(*argv, "--memory") == 0)
         memstats = 1;

      else if (strcmp(*argv, "--size") == 0)
         pm.test_size = 1;

      else if (strcmp(*argv, "--nosize") == 0)
         pm.test_size = 0;

      else if (strcmp(*argv, "--standard") == 0)
         pm.test_standard = 1;

      else if (strcmp(*argv, "--nostandard") == 0)
         pm.test_standard = 0;

      else if (strcmp(*argv, "--transform") == 0)
         pm.test_transform = 1;

      else if (strcmp(*argv, "--notransform") == 0)
         pm.test_transform = 0;

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
      else if (strncmp(*argv, "--transform-disable=",
         sizeof "--transform-disable") == 0)
         {
         pm.test_transform = 1;
         transform_disable(*argv + sizeof "--transform-disable");
         }

      else if (strncmp(*argv, "--transform-enable=",
         sizeof "--transform-enable") == 0)
         {
         pm.test_transform = 1;
         transform_enable(*argv + sizeof "--transform-enable");
         }
#endif  

      else if (strcmp(*argv, "--gamma") == 0)
         {
          
         pm.ngamma_tests = 2U;
         pm.test_gamma_threshold = 1;
         pm.test_gamma_transform = 1;
         pm.test_gamma_sbit = 1;
         pm.test_gamma_scale16 = 1;
         pm.test_gamma_background = 1;  
         pm.test_gamma_alpha_mode = 1;
         }

      else if (strcmp(*argv, "--nogamma") == 0)
         pm.ngamma_tests = 0;

      else if (strcmp(*argv, "--gamma-threshold") == 0)
         pm.ngamma_tests = 2U, pm.test_gamma_threshold = 1;

      else if (strcmp(*argv, "--nogamma-threshold") == 0)
         pm.test_gamma_threshold = 0;

      else if (strcmp(*argv, "--gamma-transform") == 0)
         pm.ngamma_tests = 2U, pm.test_gamma_transform = 1;

      else if (strcmp(*argv, "--nogamma-transform") == 0)
         pm.test_gamma_transform = 0;

      else if (strcmp(*argv, "--gamma-sbit") == 0)
         pm.ngamma_tests = 2U, pm.test_gamma_sbit = 1;

      else if (strcmp(*argv, "--nogamma-sbit") == 0)
         pm.test_gamma_sbit = 0;

      else if (strcmp(*argv, "--gamma-16-to-8") == 0)
         pm.ngamma_tests = 2U, pm.test_gamma_scale16 = 1;

      else if (strcmp(*argv, "--nogamma-16-to-8") == 0)
         pm.test_gamma_scale16 = 0;

      else if (strcmp(*argv, "--gamma-background") == 0)
         pm.ngamma_tests = 2U, pm.test_gamma_background = 1;

      else if (strcmp(*argv, "--nogamma-background") == 0)
         pm.test_gamma_background = 0;

      else if (strcmp(*argv, "--gamma-alpha-mode") == 0)
         pm.ngamma_tests = 2U, pm.test_gamma_alpha_mode = 1;

      else if (strcmp(*argv, "--nogamma-alpha-mode") == 0)
         pm.test_gamma_alpha_mode = 0;

      else if (strcmp(*argv, "--expand16") == 0)
         pm.test_gamma_expand16 = 1;

      else if (strcmp(*argv, "--noexpand16") == 0)
         pm.test_gamma_expand16 = 0;

      else if (strcmp(*argv, "--low-depth-gray") == 0)
         pm.test_lbg = pm.test_lbg_gamma_threshold =
            pm.test_lbg_gamma_transform = pm.test_lbg_gamma_sbit =
            pm.test_lbg_gamma_composition = 1;

      else if (strcmp(*argv, "--nolow-depth-gray") == 0)
         pm.test_lbg = pm.test_lbg_gamma_threshold =
            pm.test_lbg_gamma_transform = pm.test_lbg_gamma_sbit =
            pm.test_lbg_gamma_composition = 0;

#     ifdef PNG_WRITE_tRNS_SUPPORTED
         else if (strcmp(*argv, "--tRNS") == 0)
            pm.test_tRNS = 1;
#     endif

      else if (strcmp(*argv, "--notRNS") == 0)
         pm.test_tRNS = 0;

      else if (strcmp(*argv, "--more-gammas") == 0)
         pm.ngamma_tests = 3U;

      else if (strcmp(*argv, "--all-gammas") == 0)
         pm.ngamma_tests = pm.ngammas;

      else if (strcmp(*argv, "--progressive-read") == 0)
         pm.this.progressive = 1;

      else if (strcmp(*argv, "--use-update-info") == 0)
         ++pm.use_update_info;  

      else if (strcmp(*argv, "--interlace") == 0)
      {
#        if CAN_WRITE_INTERLACE
            pm.interlace_type = PNG_INTERLACE_ADAM7;
#        else  
            fprintf(stderr, "pngvalid: no write interlace support\n");
            return SKIP;
#        endif  
      }

      else if (strcmp(*argv, "--use-input-precision") == 0)
         pm.use_input_precision = 1U;

      else if (strcmp(*argv, "--use-calculation-precision") == 0)
         pm.use_input_precision = 0;

      else if (strcmp(*argv, "--calculations-use-input-precision") == 0)
         pm.calculations_use_input_precision = 1U;

      else if (strcmp(*argv, "--assume-16-bit-calculations") == 0)
         pm.assume_16_bit_calculations = 1U;

      else if (strcmp(*argv, "--calculations-follow-bit-depth") == 0)
         pm.calculations_use_input_precision =
            pm.assume_16_bit_calculations = 0;

      else if (strcmp(*argv, "--exhaustive") == 0)
         pm.test_exhaustive = 1;

      else if (argc > 1 && strcmp(*argv, "--sbitlow") == 0)
         --argc, pm.sbitlow = (png_byte)atoi(*++argv), catmore = 1;

      else if (argc > 1 && strcmp(*argv, "--touch") == 0)
         --argc, touch = *++argv, catmore = 1;

      else if (argc > 1 && strncmp(*argv, "--max", 5) == 0)
      {
         --argc;

         if (strcmp(5+*argv, "abs8") == 0)
            pm.maxabs8 = atof(*++argv);

         else if (strcmp(5+*argv, "abs16") == 0)
            pm.maxabs16 = atof(*++argv);

         else if (strcmp(5+*argv, "calc8") == 0)
            pm.maxcalc8 = atof(*++argv);

         else if (strcmp(5+*argv, "calc16") == 0)
            pm.maxcalc16 = atof(*++argv);

         else if (strcmp(5+*argv, "out8") == 0)
            pm.maxout8 = atof(*++argv);

         else if (strcmp(5+*argv, "out16") == 0)
            pm.maxout16 = atof(*++argv);

         else if (strcmp(5+*argv, "pc8") == 0)
            pm.maxpc8 = atof(*++argv);

         else if (strcmp(5+*argv, "pc16") == 0)
            pm.maxpc16 = atof(*++argv);

         else
         {
            fprintf(stderr, "pngvalid: %s: unknown 'max' option\n", *argv);
            exit(99);
         }

         catmore = 1;
      }

      else if (strcmp(*argv, "--log8") == 0)
         --argc, pm.log8 = atof(*++argv), catmore = 1;

      else if (strcmp(*argv, "--log16") == 0)
         --argc, pm.log16 = atof(*++argv), catmore = 1;

#ifdef PNG_SET_OPTION_SUPPORTED
      else if (strncmp(*argv, "--option=", 9) == 0)
      {
          
         const char *arg = 9+*argv;
         unsigned char option=0, setting=0;

#ifdef PNG_ARM_NEON
         if (strncmp(arg, "arm-neon:", 9) == 0)
            option = PNG_ARM_NEON, arg += 9;

         else
#endif
#ifdef PNG_EXTENSIONS
         if (strncmp(arg, "extensions:", 11) == 0)
            option = PNG_EXTENSIONS, arg += 11;

         else
#endif
#ifdef PNG_MAXIMUM_INFLATE_WINDOW
         if (strncmp(arg, "max-inflate-window:", 19) == 0)
            option = PNG_MAXIMUM_INFLATE_WINDOW, arg += 19;

         else
#endif
         {
            fprintf(stderr, "pngvalid: %s: %s: unknown option\n", *argv, arg);
            exit(99);
         }

         if (strcmp(arg, "off") == 0)
            setting = PNG_OPTION_OFF;

         else if (strcmp(arg, "on") == 0)
            setting = PNG_OPTION_ON;

         else
         {
            fprintf(stderr,
               "pngvalid: %s: %s: unknown setting (use 'on' or 'off')\n",
               *argv, arg);
            exit(99);
         }

         pm.this.options[pm.this.noptions].option = option;
         pm.this.options[pm.this.noptions++].setting = setting;
      }
#endif  

      else
      {
         fprintf(stderr, "pngvalid: %s: unknown argument\n", *argv);
         exit(99);
      }

      if (catmore)  
      {
         cp = safecat(command, sizeof command, cp, " ");
         cp = safecat(command, sizeof command, cp, *argv);
      }
   }

                                                                             
            
      
   if (pm.test_standard == 0 && pm.test_size == 0 && pm.test_transform == 0 &&
      pm.ngamma_tests == 0)
   {
                                                                                
                                                                                
                                                                              
                                                    
         
      pm.test_standard = 1;
      pm.test_size = 1;
      pm.test_transform = 1;
      pm.ngamma_tests = 2U;
   }

   if (pm.ngamma_tests > 0 &&
      pm.test_gamma_threshold == 0 && pm.test_gamma_transform == 0 &&
      pm.test_gamma_sbit == 0 && pm.test_gamma_scale16 == 0 &&
      pm.test_gamma_background == 0 && pm.test_gamma_alpha_mode == 0)
   {
      pm.test_gamma_threshold = 1;
      pm.test_gamma_transform = 1;
      pm.test_gamma_sbit = 1;
      pm.test_gamma_scale16 = 1;
      pm.test_gamma_background = 1;
      pm.test_gamma_alpha_mode = 1;
   }

   else if (pm.ngamma_tests == 0)
   {
       
      pm.test_gamma_threshold = 0;
      pm.test_gamma_transform = 0;
      pm.test_gamma_sbit = 0;
      pm.test_gamma_scale16 = 0;
      pm.test_gamma_background = 0;
      pm.test_gamma_alpha_mode = 0;
   }

   Try
   {
       
      make_transform_images(&pm);

       
      if (pm.test_standard)
      {
         perform_interlace_macro_validation();
         perform_formatting_test(&pm.this);
#        ifdef PNG_READ_SUPPORTED
            perform_standard_test(&pm);
#        endif
         perform_error_test(&pm);
      }

       
      if (pm.test_size)
      {
         make_size_images(&pm.this);
#        ifdef PNG_READ_SUPPORTED
            perform_size_test(&pm);
#        endif
      }

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
       
      if (pm.test_transform)
         perform_transform_test(&pm);
#endif  

#ifdef PNG_READ_GAMMA_SUPPORTED
      if (pm.ngamma_tests > 0)
         perform_gamma_test(&pm, summary);
#endif
   }

   Catch_anonymous
   {
      fprintf(stderr, "pngvalid: test aborted (probably failed in cleanup)\n");
      if (!pm.this.verbose)
      {
         if (pm.this.error[0] != 0)
            fprintf(stderr, "pngvalid: first error: %s\n", pm.this.error);

         fprintf(stderr, "pngvalid: run with -v to see what happened\n");
      }
      exit(1);
   }

   if (summary)
   {
      printf("%s: %s (%s point arithmetic)\n",
         (pm.this.nerrors || (pm.this.treat_warnings_as_errors &&
            pm.this.nwarnings)) ? "FAIL" : "PASS",
         command,
#if defined(PNG_FLOATING_ARITHMETIC_SUPPORTED) || PNG_LIBPNG_VER < 10500
         "floating"
#else
         "fixed"
#endif
         );
   }

   if (memstats)
   {
      printf("Allocated memory statistics (in bytes):\n"
         "\tread  %lu maximum single, %lu peak, %lu total\n"
         "\twrite %lu maximum single, %lu peak, %lu total\n",
         (unsigned long)pm.this.read_memory_pool.max_max,
         (unsigned long)pm.this.read_memory_pool.max_limit,
         (unsigned long)pm.this.read_memory_pool.max_total,
         (unsigned long)pm.this.write_memory_pool.max_max,
         (unsigned long)pm.this.write_memory_pool.max_limit,
         (unsigned long)pm.this.write_memory_pool.max_total);
   }

                                                                             
                                                                         
      
   store_delete(&pm.this);

                                                                    
               
      
   if (pm.this.nerrors || (pm.this.treat_warnings_as_errors &&
       pm.this.nwarnings))
   {
      if (!pm.this.verbose)
         fprintf(stderr, "pngvalid: %s\n", pm.this.error);

      fprintf(stderr, "pngvalid: %d errors, %d warnings\n", pm.this.nerrors,
          pm.this.nwarnings);

      exit(1);
   }

    
   if (touch != NULL)
   {
      FILE *fsuccess = fopen(touch, "wt");

      if (fsuccess != NULL)
      {
         int error = 0;
         fprintf(fsuccess, "PNG validation succeeded\n");
         fflush(fsuccess);
         error = ferror(fsuccess);

         if (fclose(fsuccess) || error)
         {
            fprintf(stderr, "%s: write failed\n", touch);
            exit(1);
         }
      }

      else
      {
         fprintf(stderr, "%s: open failed\n", touch);
         exit(1);
      }
   }

                                                                              
      
   UNUSED(fail)
   return 0;
}
#else  
int main(void)
{
   fprintf(stderr,
      "pngvalid: no low level write support in libpng, all tests skipped\n");
    
   return SKIP;
}
#endif
