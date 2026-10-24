/* pngcp.c
 *
 * Copyright (c) 2016 John Cunningham Bowler
 *
 * Last changed in libpng 1.6.24 [August 4, 2016]
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * This is an example of copying a PNG without changes using the png_read_png
 * and png_write_png interfaces.  A considerable number of options are provided
 * to manipulate the compression of the PNG data and other compressed chunks.
 *
 * For a more extensive example that uses the transforms see
 * contrib/libtests/pngimage.c in the libpng distribution.
 */
#include "pnglibconf.h"  

#ifdef PNG_PNGCP_TIMING_SUPPORTED
              
     
                                                                             
                                                                           
                                                   
     
                                                                                
                                                                      
     
                                                                              
                                
      
#  define _POSIX_C_SOURCE 199309L
#else  
#  define _POSIX_SOURCE 1
#endif

#if defined(HAVE_CONFIG_H) && !defined(PNG_NO_CONFIG_H)
#  include <config.h>
#endif

#include <stdio.h>

                                                                              
                                 
   
#ifdef PNG_FREESTANDING_TESTS
#  include <png.h>
#else
#  include "../../png.h"
#endif

#if PNG_LIBPNG_VER < 10700
    
#  ifdef PNG_INFO_IMAGE_SUPPORTED
#     ifdef PNG_SEQUENTIAL_READ_SUPPORTED
#        define PNG_READ_PNG_SUPPORTED
#     endif  
#     ifdef PNG_WRITE_SUPPORTED
#        define PNG_WRITE_PNG_SUPPORTED
#     endif  
#  endif  
#endif  

#if (defined(PNG_READ_PNG_SUPPORTED)) && (defined(PNG_WRITE_PNG_SUPPORTED))
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>

#include <unistd.h>
#include <sys/stat.h>

#include <zlib.h>

#ifndef PNG_SETJMP_SUPPORTED
#  include <setjmp.h>  
#endif

#ifdef __cplusplus
#  define voidcast(type, value) static_cast<type>(value)
#else
#  define voidcast(type, value) (value)
#endif  

#ifdef __GNUC__
                                                                             
                                                                          
                                                          
      
#  define gv volatile
#else
#  define gv
#endif

                                                                               
                                                                              
                                                                              
                                                                        
                                                                               
                                                                                
            
   
#ifdef PNG_PNGCP_TIMING_SUPPORTED
#  include <time.h>    
#  ifndef CLOCK_PROCESS_CPUTIME_ID
       
#     undef PNG_PNGCP_TIMING_SUPPORTED
#  endif
#endif  

 

 
typedef enum
{
   VERBOSE,         
   INFORMATION,
   WARNINGS,        
   LIBPNG_WARNING,
   APP_WARNING,
   ERRORS,          
   APP_FAIL,        
   LIBPNG_ERROR,    
   LIBPNG_BUG,      
   APP_ERROR,       
   QUIET,           
   USER_ERROR,      
   INTERNAL_ERROR
} error_level;
#define LEVEL_MASK      0xf    

#define STRICT          0x010  
#define LOG             0x020  
#define CONTINUE        0x040  
#define SIZES           0x080  
#define SEARCH          0x100  
#define NOWRITE         0x200  
#ifdef PNG_CHECK_FOR_INVALID_INDEX_SUPPORTED
#  define IGNORE_INDEX  0x400  
#  ifdef PNG_GET_PALETTE_MAX_SUPPORTED
#     define FIX_INDEX  0x800  
#  endif  
#endif  
#define OPTION     0x80000000  
#define LIST       0x80000001  

                                                                            
                                                                              
                                                                    
   
#define RESULT_STRICT(r)   (((r) & ~((1U<<WARNINGS)-1)) == 0)
#define RESULT_RELAXED(r)  (((r) & ~((1U<<ERRORS)-1)) == 0)

 
static const char range_lo[] = "low";
static const char range_hi[] = "high";
static const char all[] = "all";
#define RANGE(lo,hi) { range_lo, lo }, { range_hi, hi }
typedef struct value_list
{
   const char *name;   
   int         value;  
}  value_list;

static const value_list
#ifdef PNG_SW_COMPRESS_png_level
vl_compression[] =
{
                                                                           
                                                                               
                      
      
   { "low-memory",      PNG_COMPRESSION_LOW_MEMORY },
   { "high-speed",      PNG_COMPRESSION_HIGH_SPEED },
   { "high-read-speed", PNG_COMPRESSION_HIGH_READ_SPEED },
   { "low",             PNG_COMPRESSION_LOW },
   { "medium",          PNG_COMPRESSION_MEDIUM },
   { "old",             PNG_COMPRESSION_COMPAT },
   { "high",            PNG_COMPRESSION_HIGH },
   { all, 0 }
},
#endif  

#if defined(PNG_WRITE_CUSTOMIZE_COMPRESSION_SUPPORTED) ||\
    defined(PNG_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED)
vl_strategy[] =
{
    
   { "huffman", Z_HUFFMAN_ONLY },
   { "RLE", Z_RLE },
   { "fixed", Z_FIXED },  
   { "filtered", Z_FILTERED },
   { "default", Z_DEFAULT_STRATEGY },
   { all, 0 }
},
#ifdef PNG_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED
vl_windowBits_text[] =
{
   { "default", MAX_WBITS  },
   { "minimum", 8 },
   RANGE(8, MAX_WBITS ),
   { all, 0 }
},
#endif  
vl_level[] =
{
   { "default", Z_DEFAULT_COMPRESSION   },
   { "none", Z_NO_COMPRESSION },
   { "speed", Z_BEST_SPEED },
   { "best", Z_BEST_COMPRESSION },
   { "0", Z_NO_COMPRESSION },
   RANGE(1, 9),  
   { all, 0 }
},
vl_memLevel[] =
{
   { "max", MAX_MEM_LEVEL },  
   { "1", 1 },  
   { "default", 8 },  
   { "2", 2 },
   { "3", 3 },
   { "4", 4 },
   { "5", 5 },  
   RANGE(6, MAX_MEM_LEVEL ),  
   { all, 0 }
},
#endif  
#ifdef PNG_WRITE_FILTER_SUPPORTED
vl_filter[] =
{
   { all,      PNG_ALL_FILTERS   },
   { "off",    PNG_NO_FILTERS    },
   { "none",   PNG_FILTER_NONE   },
   { "sub",    PNG_FILTER_SUB    },
   { "up",     PNG_FILTER_UP     },
   { "avg",    PNG_FILTER_AVG    },
   { "paeth",  PNG_FILTER_PAETH  }
},
#endif  
#ifdef PNG_PNGCP_TIMING_SUPPORTED
#  define PNGCP_TIME_READ  1
#  define PNGCP_TIME_WRITE 2
vl_time[] =
{
   { "both",  PNGCP_TIME_READ+PNGCP_TIME_WRITE },
   { "off",   0 },
   { "read",  PNGCP_TIME_READ },
   { "write", PNGCP_TIME_WRITE }
},
#endif  
vl_IDAT_size[] =  
{
   { "default", 0x7FFFFFFF },
   { "minimal", 1 },
   RANGE(1, 0x7FFFFFFF)
},
#ifndef PNG_SW_IDAT_size
    
#  define png_set_IDAT_size(p,v) png_set_compression_buffer_size(p, v)
#endif  
#define SL 8  
vl_log_depth[] = { { "on", 1 }, { "off", 0 }, RANGE(0, SL) },
vl_on_off[] = { { "on", 1 }, { "off", 0 } };

#ifdef PNG_WRITE_CUSTOMIZE_COMPRESSION_SUPPORTED
static value_list
vl_windowBits_IDAT[] =
{
   { "default", MAX_WBITS },
   { "small", 9 },
   RANGE(8, MAX_WBITS),  
   { all, 0 }
};
#endif  

typedef struct option
{
   const char       *name;          
   png_uint_32       opt;           
   png_byte          search;        
   png_byte          value_count;   
   const value_list *values;        
}  option;

static const option options[] =
{
    
#  define S(n,v) { #n, v, 0, 2, vl_on_off },
   S(verbose,  VERBOSE)
   S(warnings, WARNINGS)
   S(errors,   ERRORS)
   S(quiet,    QUIET)
   S(strict,   STRICT)
   S(log,      LOG)
   S(continue, CONTINUE)
   S(sizes,    SIZES)
   S(search,   SEARCH)
   S(nowrite,  NOWRITE)
#  ifdef IGNORE_INDEX
      S(ignore-palette-index, IGNORE_INDEX)
#  endif  
#  ifdef FIX_INDEX
      S(fix-palette-index, FIX_INDEX)
#  endif  
#  undef S

    
#  define VLNAME(name) vl_ ## name
#  define VLSIZE(name) voidcast(png_byte,\
                           (sizeof VLNAME(name))/(sizeof VLNAME(name)[0]))
#  define VL(oname, name, type, search)\
   { oname, type, search, VLSIZE(name), VLNAME(name) },
#  define VLO(oname, name, search) VL(oname, name, OPTION, search)

#  ifdef PNG_WRITE_CUSTOMIZE_COMPRESSION_SUPPORTED
#     define VLCIDAT(name) VLO(#name, name, 1 )
#     ifdef PNG_SW_COMPRESS_level
#        define VLCiCCP(name) VLO("ICC-profile-" #name, name, 0 )
#     else
#        define VLCiCCP(name)
#     endif
#  else
#     define VLCIDAT(name)
#     define VLCiCCP(name)
#  endif  

#  ifdef PNG_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED
#     define VLCzTXt(name) VLO("text-" #name, name, 0 )
#  else
#     define VLCzTXt(name)
#  endif  

#  define VLC(name) VLCIDAT(name) VLCiCCP(name) VLCzTXt(name)

#  ifdef PNG_SW_COMPRESS_png_level
                                                                              
                                        
         
      VLO("compression", compression, 0)
      VLO("text-compression", compression, 0)
      VLO("ICC-profile-compression", compression, 0)
#  endif  
   VLC(strategy)
   VLO("windowBits", windowBits_IDAT, 1)
#  ifdef PNG_SW_COMPRESS_windowBits
      VLO("ICC-profile-windowBits", windowBits_text , 0)
#  endif
   VLO("text-windowBits", windowBits_text, 0)
   VLC(level)
   VLC(memLevel)
   VLO("IDAT-size", IDAT_size, 0)
   VLO("log-depth", log_depth, 0)

#  undef VLO

    
#  define VLL(name, search) VL(#name, name, LIST, search)
#ifdef PNG_WRITE_FILTER_SUPPORTED
   VLL(filter, 0)
#endif  
#ifdef PNG_PNGCP_TIMING_SUPPORTED
   VLL(time, 0)
#endif  
#  undef VLL
#  undef VL
};

#ifdef __cplusplus
   static const size_t option_count((sizeof options)/(sizeof options[0]));
#else  
#  define option_count ((sizeof options)/(sizeof options[0]))
#endif  

static const char *
cts(int ct)
{
   switch (ct)
   {
      case PNG_COLOR_TYPE_PALETTE:     return "P";
      case PNG_COLOR_TYPE_GRAY:        return "G";
      case PNG_COLOR_TYPE_GRAY_ALPHA:  return "GA";
      case PNG_COLOR_TYPE_RGB:         return "RGB";
      case PNG_COLOR_TYPE_RGB_ALPHA:   return "RGBA";
      default:                         return "INVALID";
   }
}

struct display
{
   jmp_buf          error_return;       
   unsigned int     errset;             

   const char      *operation;          
   const char      *filename;           
   const char      *output_file;        

    
   FILE            *fp;

                                                                          
            
      
   png_alloc_size_t read_size;
   png_structp      read_pp;
   png_infop        ip;
#  if PNG_LIBPNG_VER < 10700 && defined PNG_TEXT_SUPPORTED
      png_textp     text_ptr;  
      int           num_text;
      int           text_stashed;
#  endif  

#  ifdef PNG_PNGCP_TIMING_SUPPORTED
      struct timespec   read_time;
      struct timespec   read_time_total;
      struct timespec   write_time;
      struct timespec   write_time_total;
#  endif  

    
#  define MAX_SIZE ((png_alloc_size_t)(-1))
   png_alloc_size_t write_size;
   png_alloc_size_t best_size;
   png_structp      write_pp;

    
   png_alloc_size_t size;
   png_uint_32      w;
   png_uint_32      h;
   int              bpp;
   png_byte         ct;
   int              no_warnings;        
   int              min_windowBits;     

    
   png_uint_32      results;              
   png_uint_32      options;              
   png_byte         entry[option_count];                                     
                                                                                
                                                                     
   int              value[option_count];  

    
                                                                        
               
      
   unsigned int     csp;                
   unsigned int     nsp;                

                                                                                
                  
      
   unsigned int     tsp;                                                    
                                                                                
                                                                                
                                          
   int              opt_string_start;                                     
                                                                              
                                                                               
                                          
   struct stack
   {
      png_alloc_size_t best_size;       
      png_alloc_size_t lo_size;
      png_alloc_size_t hi_size;
      int              lo, hi;          
      int              best_val;        
      int              opt_string_end;  
      png_byte         opt;             
      png_byte         entry;           
      png_byte         end;             
   }                stack[SL];          
   char             curr[32*SL];        
   char             best[32*SL];        

   char             namebuf[FILENAME_MAX];  
};

static void
display_init(struct display *dp)
                                                                      
                                                                            
                          
      
{
   memset(dp, 0, sizeof *dp);
   dp->operation = "internal error";
   dp->filename = "command line";
   dp->output_file = "no output file";
   dp->options = WARNINGS;  
   dp->fp = NULL;
   dp->read_pp = NULL;
   dp->ip = NULL;
   dp->write_pp = NULL;
   dp->min_windowBits = -1;  
#  if PNG_LIBPNG_VER < 10700 && defined PNG_TEXT_SUPPORTED
      dp->text_ptr = NULL;
      dp->num_text = 0;
      dp->text_stashed = 0;
#  endif  
}

static void
display_clean_read(struct display *dp)
{
   if (dp->read_pp != NULL)
      png_destroy_read_struct(&dp->read_pp, NULL, NULL);

   if (dp->fp != NULL)
   {
      FILE *fp = dp->fp;
      dp->fp = NULL;
      (void)fclose(fp);
   }
}

static void
display_clean_write(struct display *dp)
{
   if (dp->fp != NULL)
   {
      FILE *fp = dp->fp;
      dp->fp = NULL;
      (void)fclose(fp);
   }

   if (dp->write_pp != NULL)
      png_destroy_write_struct(&dp->write_pp, dp->tsp > 0 ? NULL : &dp->ip);
}

static void
display_clean(struct display *dp)
{
   display_clean_read(dp);
   display_clean_write(dp);
   dp->output_file = NULL;

#  if PNG_LIBPNG_VER < 10700 && defined PNG_TEXT_SUPPORTED
                                                                      
                                                                            
         
      if (dp->text_stashed)
      {
         dp->text_stashed = 0;
         dp->num_text = 0;
         free(dp->text_ptr);
         dp->text_ptr = NULL;
      }
#  endif  

    
   dp->results = 0;  
}

static void
display_destroy(struct display *dp)
{
    
   display_clean(dp);
}

static struct display *
get_dp(png_structp pp)
    
{
   struct display *dp = (struct display*)png_get_error_ptr(pp);

   if (dp == NULL)
   {
      fprintf(stderr, "pngcp: internal error (no display)\n");
      exit(99);  
   }

   return dp;
}

 
#ifdef __GNUC__
#  define VGATTR __attribute__((__format__ (__printf__,3,4)))
                                                                              
                                         
      
#else
#  define VGATTR
#endif
static void VGATTR
display_log(struct display *dp, error_level level, const char *fmt, ...)
                                                                            
                                                      
      
{
   dp->results |= 1U << level;

   if (level > (error_level)(dp->options & LEVEL_MASK))
   {
      const char *lp;
      va_list ap;

      switch (level)
      {
         case INFORMATION:    lp = "information"; break;
         case LIBPNG_WARNING: lp = "warning(libpng)"; break;
         case APP_WARNING:    lp = "warning(pngcp)"; break;
         case APP_FAIL:       lp = "error(continuable)"; break;
         case LIBPNG_ERROR:   lp = "error(libpng)"; break;
         case LIBPNG_BUG:     lp = "bug(libpng)"; break;
         case APP_ERROR:      lp = "error(pngcp)"; break;
         case USER_ERROR:     lp = "error(user)"; break;

         case INTERNAL_ERROR:  
         case VERBOSE: case WARNINGS: case ERRORS: case QUIET:
         default:             lp = "bug(pngcp)"; break;
      }

      fprintf(stderr, "%s: %s: %s",
         dp->filename != NULL ? dp->filename : "<stdin>", lp, dp->operation);

      fprintf(stderr, ": ");

      va_start(ap, fmt);
      vfprintf(stderr, fmt, ap);
      va_end(ap);

      fputc('\n', stderr);
   }
    

    
   if (level > APP_FAIL || (level > ERRORS && !(dp->options & CONTINUE)))
   {
      if (dp->errset)
         longjmp(dp->error_return, level);

      else
         exit(99);
   }
}

#if PNG_LIBPNG_VER < 10700 && defined PNG_TEXT_SUPPORTED
static void
text_stash(struct display *dp)
{
                                                                         
                                                                              
                                                                              
                                                                               
                                                                             
                                                                           
              
     
                                                                               
                                                                                
                                                                            
                                                                               
                                       
     
                                                                          
                                                                               
                           
      
   png_textp chunks = NULL;

   dp->num_text = png_get_text(dp->write_pp, dp->ip, &chunks, NULL);

   if (dp->num_text > 0)
   {
      dp->text_ptr = voidcast(png_textp, malloc(dp->num_text * sizeof *chunks));

      if (dp->text_ptr == NULL)
         display_log(dp, APP_ERROR, "text chunks: stash malloc failed");

      else
         memcpy(dp->text_ptr, chunks, dp->num_text * sizeof *chunks);
   }

   dp->text_stashed = 1;  
}

#define text_stash(dp) if (!dp->text_stashed) text_stash(dp)

static void
text_restore(struct display *dp)
{
    
   if (dp->text_ptr != NULL)
      png_set_text(dp->write_pp, dp->ip, dp->text_ptr, dp->num_text);
}

#define text_restore(dp) if (dp->text_stashed) text_restore(dp)

#else
#define text_stash(dp) ((void)0)
#define text_restore(dp) ((void)0)
#endif  

           
  
                                            
  
              
                                   
                 
                                    
                    
                                         
                             
                                                                      
   
static png_byte
option_index(struct display *dp, const char *opt, size_t len)
                                                                              
                                                                              
                              
      
{
   png_byte j;

   for (j=0; j<option_count; ++j)
      if (strncmp(options[j].name, opt, len) == 0 && options[j].name[len] == 0)
         return j;

                                                                              
                                                                 
      
   display_log(dp, dp->errset ? INTERNAL_ERROR : USER_ERROR,
         "%.*s: unknown option", (int) len, opt);
   abort();  
}

 
#define OPTIND(dp, name) option_index(dp, #name, (sizeof #name)-1)

static int
get_option(struct display *dp, const char *opt, int *value)
{
   png_byte i = option_index(dp, opt, strlen(opt));

   if (dp->entry[i])  
   {
      *value = dp->value[i];
      return 1;
   }

   else
      return 0;
}

static int
set_opt_string_(struct display *dp, unsigned int sp, png_byte opt,
      const char *entry_name)
    
{
   int offset, add;

   if (sp > 0)
      offset = dp->stack[sp-1].opt_string_end;

   else
      offset = dp->opt_string_start;

   if (entry_name == range_lo)
      add = sprintf(dp->curr+offset, " --%s=%d", options[opt].name,
            dp->value[opt]);

   else
      add = sprintf(dp->curr+offset, " --%s=%s", options[opt].name, entry_name);

   if (add < 0)
      display_log(dp, INTERNAL_ERROR, "sprintf failed");

   assert(offset+add < (int) sizeof dp->curr);
   return offset+add;
}

static void
set_opt_string(struct display *dp, unsigned int sp)
    
{
   dp->stack[sp].opt_string_end = set_opt_string_(dp, sp, dp->stack[sp].opt, 
      options[dp->stack[sp].opt].values[dp->stack[sp].entry].name);
}

static void
record_opt(struct display *dp, png_byte opt, const char *entry_name)
                                                                              
                                                                        
                      
      
{
   unsigned int sp = dp->csp;  

   if (sp >= dp->tsp)
   {
                                                                           
                                                                               
                                    
         
      int offset = set_opt_string_(dp, sp, opt, entry_name);

      if (sp > 0)
         dp->stack[sp-1].opt_string_end = offset;

      else
         dp->opt_string_start = offset;
   }

    
}

static int
opt_list_end(struct display *dp, png_byte opt, png_byte entry)
{
   if (options[opt].values[entry].name == range_lo)
      return entry+1U >= options[opt].value_count   ||
         options[opt].values[entry+1U].name != range_hi   ||
         options[opt].values[entry+1U].value <= dp->value[opt]  ;

   else
      return entry+1U >= options[opt].value_count   ||
         options[opt].values[entry+1U].name == all  ;
}

static void
push_opt(struct display *dp, unsigned int sp, png_byte opt, int search)
                                                                        
                                                                             
                                  
      
{
   png_byte entry;
   const char *entry_name;

   assert(sp == dp->tsp && sp < SL);

                                                                               
                                          
      
   entry = options[opt].value_count;
   assert(entry > 0U);

   do
   {
      entry_name = options[opt].values[--entry].name;
      if (entry_name == range_lo)
         break;
   }
   while (entry > 0U);

   dp->tsp = sp+1U;
   dp->stack[sp].best_size =
      dp->stack[sp].lo_size =
      dp->stack[sp].hi_size = MAX_SIZE;

   if (search && entry_name == range_lo)  
   {
      dp->stack[sp].lo = options[opt].values[entry].value;
       
      assert(entry+1 < options[opt].value_count &&
             options[opt].values[entry+1].name == range_hi);
      dp->stack[sp].hi = options[opt].values[entry+1].value;
   }

   else
   {
       
      dp->stack[sp].lo = INT_MAX;
      dp->stack[sp].hi = INT_MIN;  
   }

   dp->stack[sp].opt = opt;
   dp->stack[sp].entry = entry;
   dp->stack[sp].best_val = dp->value[opt] = options[opt].values[entry].value;

   set_opt_string(dp, sp);

                                                                               
                          
      
   if (opt_list_end(dp, opt, entry))
   {
      dp->stack[sp].end = 1;
                                                                    
                           
         
      if (opt != dp->min_windowBits)
         display_log(dp, APP_WARNING, "%s: only testing one value",
               options[opt].name);
   }

   else
   {
      dp->stack[sp].end = 0;
      dp->nsp = dp->tsp;
   }

                                                                            
                                                                               
                              
      
   text_stash(dp);
}

static void
next_opt(struct display *dp, unsigned int sp)
                                                                                
                                                                                
                                                                              
                                                                             
                                   
      
{
   int search = 0;
   png_byte entry, opt;
   const char *entry_name;

    
   assert(sp+1U == dp->tsp && !dp->stack[sp].end);

   opt = dp->stack[sp].opt;
   entry = dp->stack[sp].entry;
   assert(entry+1U < options[opt].value_count);
   entry_name = options[opt].values[entry].name;
   assert(entry_name != NULL);

                                                                              
                                                      
      
   if (entry_name == range_lo)  
   {
                                                                                
                                                                               
                                 
         
      if (dp->stack[sp].lo > dp->stack[sp].hi)
         dp->value[opt]++;

      else
      {
          
         png_alloc_size_t best_size = dp->stack[sp].best_size;
         int lo = dp->stack[sp].lo;
         int hi = dp->stack[sp].hi;
         int val = dp->value[opt];

         search = 1;  
         assert(best_size < MAX_SIZE);

         if (val == lo)
         {
             
            dp->stack[sp].lo_size = best_size;
            assert(hi > val);

            if (hi == val+1)  
               dp->stack[sp].end = 1;

            val = hi;
         }

         else if (val == hi)
         {
            dp->stack[sp].hi_size = best_size;
            assert(val > lo+1);  

            if (val == lo+2)  
               dp->stack[sp].end = 1;

            val = (lo + val)/2;
         }

         else
         {
            png_alloc_size_t lo_size = dp->stack[sp].lo_size;
            png_alloc_size_t hi_size = dp->stack[sp].hi_size;

             
            assert(lo_size < MAX_SIZE && hi_size < MAX_SIZE);

                                                                                
                                             
               
            if (val < lo)  
            {
                
               dp->stack[sp].lo = val;
               dp->stack[sp].lo_size = best_size;
               val = lo;
               best_size = lo_size;
               lo = dp->stack[sp].lo;
               lo_size = dp->stack[sp].lo_size;
            }

            else if (val > hi)  
            {
                
               dp->stack[sp].hi = val;
               dp->stack[sp].hi_size = best_size;
               val = hi;
               best_size = hi_size;
               hi = dp->stack[sp].hi;
               hi_size = dp->stack[sp].hi_size;
            }

             
            assert(lo < val && val < hi);

                                                                              
                                        
               
            if (hi == lo+3)
            {
                                                                                
                                         
                  
               val = lo + ((val == lo+1) ? 2 : 1);
               assert(lo < val && val < hi);
               dp->stack[sp].end = 1;
            }

            else
            {
                                                                                
                                                                 
                 
                                                                       
                                                                     
                  
               assert(hi > lo+3);

               if (lo_size <= best_size && best_size <= hi_size)
               {
                                                                             
                                                                               
                                                                                
                                                      
                     
                  if (val == lo+1)
                  {
                     ++val;
                     dp->stack[sp].end = 1;
                  }

                  else
                  {
                     dp->stack[sp].hi = hi = val;
                     dp->stack[sp].hi_size = best_size;
                     val = (lo + val) / 2;
                  }
               }

               else if (lo_size >= best_size && best_size >= hi_size)
               {
                                                                              
                                                                            
                          
                     
                  if (val == hi-1)
                  {
                     --val;
                     dp->stack[sp].end = 1;
                  }

                  else
                  {
                     dp->stack[sp].lo = lo = val;
                     dp->stack[sp].lo_size = best_size;
                     val = (val + hi) / 2;
                  }
               }

                                                                               
                                                                                
                                                                               
                      
                 
                                                                               
                                                                             
                                                                             
                                                                              
                                
                 
                                                                           
                                                                                
                                   
                  
               else if (lo_size <= hi_size)  
               {
                  dp->stack[sp].hi = val;
                  dp->stack[sp].hi_size = best_size;
                  val = --hi;
               }

               else  
               {
                  dp->stack[sp].lo = val;
                  dp->stack[sp].lo_size = best_size;
                  val = ++lo;
               }

                                                                              
                            
                  
               assert(hi > lo+1);
               if (hi <= lo+2)
                  dp->stack[sp].end = 1;
            }
         }

         assert(val != dp->stack[sp].best_val);  
         dp->value[opt] = val;
         dp->stack[sp].best_size = MAX_SIZE;
      }
   }

   else
   {
       
      dp->value[opt] = options[opt].values[++entry].value;
      dp->stack[sp].entry = entry;
   }

   set_opt_string(dp, sp);

   if (!search && opt_list_end(dp, opt, entry))  
      dp->stack[sp].end = 1;

   else if (!dp->stack[sp].end)  
      dp->nsp = dp->tsp;
}

static int
compare_option(const struct display *dp, unsigned int sp)
{
   int opt = dp->stack[sp].opt;

                                                                       
                                                 
      
   if (dp->stack[sp].best_val < dp->value[opt])
      return -1;

                                                                          
                                                                       
      
   else if (dp->stack[sp].best_val > dp->value[opt])
   {
      if (dp->stack[sp].lo <= dp->stack[sp].hi  )
         return 1;

      else
         return -1;
   }

   else
      return 0;  
}

static int
advance_opt(struct display *dp, png_byte opt, int search)
{
   unsigned int sp = dp->csp++;  

   assert(sp >= dp->nsp);  

                                                                          
                                                                       
                          
     
                                  
     
                                                                        
                                                                         
                                                                        
                
     
                                                                       
                                                                        
                                        
     
                                                                     
                                                                         
                                           
      
   if (sp >= dp->tsp)
   {
      push_opt(dp, sp, opt, search);  
      return 1;  
   }

   else
   {
      int ret = 0;  

                                                                              
                                                                           
                                                                          
                         
         
      assert(dp->write_size > 0U && dp->write_size < MAX_SIZE);

      if (dp->stack[sp].best_size > dp->write_size ||
          (dp->stack[sp].best_size == dp->write_size &&
           compare_option(dp, sp) > 0))
      {
         dp->stack[sp].best_size = dp->write_size;
         dp->stack[sp].best_val = dp->value[opt];
      }

      if (sp+1U >= dp->tsp)
      {
         next_opt(dp, sp);
         ret = 1;  
      }

      else if (!dp->stack[sp].end)  
         dp->nsp = sp+1U;

      return ret;  
   }
}

static int
getallopts_(struct display *dp, png_byte opt, int *value, int record)
                                                                                
      
{
   if (dp->entry[opt])  
   {
                                                                                
                                                                               
                                                               
         
      const char *entry_name = options[opt].values[dp->entry[opt]-1].name;

      if (entry_name == all)
         (void)advance_opt(dp, opt, 0 );

      else if (record)
         record_opt(dp, opt, entry_name);

      *value = dp->value[opt];
      return 1;  
   }

   else
      return 0;  
}

static int
getallopts(struct display *dp, const char *opt_str, int *value)
{
   return getallopts_(dp, option_index(dp, opt_str, strlen(opt_str)), value, 0);
}

static int
getsearchopts(struct display *dp, const char *opt_str, int *value)
    
{
   png_byte istrat;
   png_byte opt = option_index(dp, opt_str, strlen(opt_str));
   int record = options[opt].search;
   const char *entry_name;

                                                                           
                                              
      
   if (getallopts_(dp, opt, value, record))
      return 1;

   else if (!record)  
      return 0;  

    
   istrat = OPTIND(dp, strategy);
   entry_name = range_lo;  

   if (opt == istrat)  
      (void)advance_opt(dp, opt, 0 ), record=0;

   else if (opt == OPTIND(dp, level))
   {
       
      if (dp->value[istrat] == Z_RLE || dp->value[istrat] == Z_HUFFMAN_ONLY)
         dp->value[opt] = 1;

      else  
         (void)advance_opt(dp, opt, 1 ), record=0;
   }

   else if (opt == OPTIND(dp, windowBits))
   {
                                                                            
                                                                              
                                                                               
                                   
         
      if (dp->value[istrat] == Z_HUFFMAN_ONLY)
         dp->value[opt] = 8;

      else if (dp->value[istrat] == Z_RLE)
         dp->value[opt] = 9;

      else  
         (void)advance_opt(dp, opt, 1 ), record=0;
   }

   else if (opt == OPTIND(dp, memLevel))
   {
#     if 0
         (void)advance_opt(dp, opt, 0 ), record=0;
#     else
         dp->value[opt] = MAX_MEM_LEVEL;
#     endif
   }

   else  
      assert(0=="reached");

   if (record)
      record_opt(dp, opt, entry_name);

    
   *value = dp->value[opt];
   return 1;
}

static int
find_val(struct display *dp, png_byte opt, const char *str, size_t len)
                                                                            
                                                                               
            
      
{
   int rlo = INT_MAX, rhi = INT_MIN;
   png_byte j, irange = 0;

   for (j=1U; j<=options[opt].value_count; ++j)
   {
      if (strncmp(options[opt].values[j-1U].name, str, len) == 0 &&
          options[opt].values[j-1U].name[len] == 0)
      {
         dp->entry[opt] = j;
         return options[opt].values[j-1U].value;
      }
      else if (options[opt].values[j-1U].name == range_lo)
         rlo = options[opt].values[j-1U].value, irange = j;
      else if (options[opt].values[j-1U].name == range_hi)
         rhi = options[opt].values[j-1U].value;
   }

    
   if (irange > 0)
   {
      char *ep = NULL;
      long l = strtol(str, &ep, 0);

      if (ep == str+len && l >= rlo && l <= rhi)
      {
         dp->entry[opt] = irange;  
         return (int) l;
      }
   }

   display_log(dp, dp->errset ? INTERNAL_ERROR : USER_ERROR,
         "%s: unknown value setting '%.*s'", options[opt].name,
         (int) len, str);
   abort();  
}

static int
opt_check(struct display *dp, const char *arg)
{
   assert(dp->errset == 0);

   if (arg != NULL && arg[0] == '-' && arg[1] == '-')
   {
      int i = 0, negate = (strncmp(arg+2, "no-", 3) == 0), val;
      png_byte j;

      if (negate)
         arg += 5;  

      else
         arg += 2;  

       
      while (arg[i] != 0 && arg[i] != '=') ++i;

                                                                                
                             
         
      j = option_index(dp, arg, i);

       
      if (arg[i] == 0)  
      {
         val = options[j].values[negate].value;
         dp->entry[j] = (png_byte) (negate + 1U);
      }

      else
      {
         const char *list = arg + (i+1);

                                                                           
                                         
            
         if (options[j].opt != LIST)
         {
             
            val = find_val(dp, j, list, strlen(list));

            if (negate)
            {
               if (options[j].opt < OPTION)
                  val = !val;

               else
               {
                  display_log(dp, USER_ERROR,
                        "%.*s: option=arg cannot be negated", i, arg);
                  abort();  
               }
            }
         }

         else  
         {
                                                                             
                                                                              
                                                                              
                                    
               
            if (negate)
               val = options[j].values[0].value;

            else
               val = 0;

            while (*list != 0)  
            {
                                                                       
                            
                  
               int v, iv;

               iv = 0;  
               while (list[++iv] != 0 && list[iv] != ',') {}

               v = find_val(dp, j, list, iv);

               if (negate)
                  val &= ~v;

               else
                  val |= v;

               list += iv;
               if (*list != 0)
                  ++list;  
            }
         }
      }

       
      dp->value[j] = val;

      if (options[j].opt < LEVEL_MASK)
      {
          
         if (val)  
            dp->options = (dp->options & ~LEVEL_MASK) | options[j].opt;

         else
            display_log(dp, USER_ERROR,
      "%.*s: messages cannot be turned off individually; set a message level",
                  i, arg);
      }

      else if (options[j].opt < OPTION)
      {
         if (val)
            dp->options |= options[j].opt;

         else
            dp->options &= ~options[j].opt;
      }

      return 1;  
   }

   else
      return 0;  
}

#ifdef PNG_PNGCP_TIMING_SUPPORTED
static void
set_timer(struct display *dp, struct timespec *timer)
{
    
   if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, timer))
   {
      display_log(dp, APP_ERROR,
            "CLOCK_PROCESS_CPUTIME_ID: %s: timing disabled\n", strerror(errno));
      dp->value[OPTIND(dp,time)] = 0;  
   }
}

static void
start_timer(struct display *dp, int what)
{
   if ((dp->value[OPTIND(dp,time)] & what) != 0)
      set_timer(dp, what == PNGCP_TIME_READ ? &dp->read_time : &dp->write_time);
}

static void
end_timer(struct display *dp, int what)
{
   if ((dp->value[OPTIND(dp,time)] & what) != 0)
   {
      struct timespec t, tmp;

      set_timer(dp, &t);

      if (what == PNGCP_TIME_READ)
         tmp = dp->read_time;

      else
         tmp = dp->write_time;

      t.tv_sec -= tmp.tv_sec;
      t.tv_nsec -= tmp.tv_nsec;

      if (t.tv_nsec < 0)
      {
         --(t.tv_sec);
         t.tv_nsec += 1000000000L;
      }

      if (what == PNGCP_TIME_READ)
         dp->read_time = t, tmp = dp->read_time_total;

      else
         dp->write_time = t, tmp = dp->write_time_total;

      tmp.tv_sec += t.tv_sec;
      tmp.tv_nsec += t.tv_nsec;

      if (tmp.tv_nsec >= 1000000000L)
      {
         ++(tmp.tv_sec);
         tmp.tv_nsec -= 1000000000L;
      }

      if (what == PNGCP_TIME_READ)
         dp->read_time_total = tmp;

      else
         dp->write_time_total = tmp;
   }
}

static void
print_time(const char *what, struct timespec t)
{
   printf("%s %.2lu.%.9ld", what, (unsigned long)t.tv_sec, t.tv_nsec);
}
#else  
#define start_timer(dp, what) ((void)0)
#define end_timer(dp, what) ((void)0)
#endif  

                                                                       
             
   
static int
checkdir(const char *pathname)
{
   struct stat buf;
   return stat(pathname, &buf) == 0 && S_ISDIR(buf.st_mode);
}

                                                                              
                                                               
  
                         
   
static int
isdir(struct display *dp, const char *pathname)
{
   if (pathname == NULL)
      return 0;  

   else if (pathname[0] == 0)
      return 1;  

   else
   {
      struct stat buf;
      int ret = stat(pathname, &buf);

      if (ret == 0)  
      {
         if (S_ISDIR(buf.st_mode))
            return 1;

          
         if (access(pathname, W_OK) != 0)
            display_log(dp, USER_ERROR, "%s: cannot be written (%s)", pathname,
                  strerror(errno));

         return 0;  
      }

      else  
      {
          
         if (errno != ENOENT)
            display_log(dp, USER_ERROR, "%s: invalid output name (%s)",
                  pathname, strerror(errno));

         return 0;  
      }
   }
}

static void
makename(struct display *dp, const char *dir, const char *infile)
{
    
   dp->namebuf[0] = 0;

   if (dir == NULL || infile == NULL)
      display_log(dp, INTERNAL_ERROR, "NULL name to makename");

   else
   {
      size_t dsize = strlen(dir);

      if (dsize <= (sizeof dp->namebuf)-2)  
      {
         size_t isize = strlen(infile);
         size_t istart = isize-1;

          
         if (infile[istart] == '/')
            display_log(dp, INTERNAL_ERROR, "infile with trailing /");

         memcpy(dp->namebuf, dir, dsize);
         if (dsize > 0 && dp->namebuf[dsize-1] != '/')
            dp->namebuf[dsize++] = '/';

          
         while (istart > 0 && infile[istart-1] != '/')
            --istart;

         isize -= istart;
         infile += istart;

         if (dsize+isize < (sizeof dp->namebuf))  
         {
            memcpy(dp->namebuf+dsize, infile, isize+1);

            if (isdir(dp, dp->namebuf))
               display_log(dp, USER_ERROR, "%s: output file is a directory",
                     dp->namebuf);
         }

         else
         {
            dp->namebuf[dsize] = 0;  
            display_log(dp, USER_ERROR, "%s%s: output file name too long",
                  dp->namebuf, infile);
         }
      }

      else
         display_log(dp, USER_ERROR, "%s: output directory name too long", dir);
   }
}

 
static void PNGCBAPI
display_warning(png_structp pp, png_const_charp warning)
{
   struct display *dp = get_dp(pp);

    
   if (!dp->no_warnings)
      display_log(get_dp(pp), LIBPNG_WARNING, "%s", warning);
}

static void PNGCBAPI
display_error(png_structp pp, png_const_charp error)
{
   struct display *dp = get_dp(pp);

   display_log(dp, LIBPNG_ERROR, "%s", error);
}

static void
display_start_read(struct display *dp, const char *filename)
{
   if (filename != NULL)
   {
      dp->filename = filename;
      dp->fp = fopen(filename, "rb");
   }

   else
   {
      dp->filename = "<stdin>";
      dp->fp = stdin;
   }

   dp->w = dp->h = 0U;
   dp->bpp = 0U;
   dp->size = 0U;
   dp->read_size = 0U;

   if (dp->fp == NULL)
      display_log(dp, USER_ERROR, "file open failed (%s)", strerror(errno));
}

static void PNGCBAPI
read_function(png_structp pp, png_bytep data, size_t size)
{
   struct display *dp = get_dp(pp);

   if (size == 0U || fread(data, size, 1U, dp->fp) == 1U)
      dp->read_size += size;

   else
   {
      if (feof(dp->fp))
         display_log(dp, LIBPNG_ERROR, "PNG file truncated");
      else
         display_log(dp, LIBPNG_ERROR, "PNG file read failed (%s)",
               strerror(errno));
   }
}

static void
read_png(struct display *dp, const char *filename)
{
   display_clean_read(dp);  
   display_start_read(dp, filename);

   dp->read_pp = png_create_read_struct(PNG_LIBPNG_VER_STRING, dp,
      display_error, display_warning);
   if (dp->read_pp == NULL)
      display_log(dp, LIBPNG_ERROR, "failed to create read struct");

#  ifdef PNG_BENIGN_ERRORS_SUPPORTED
      png_set_benign_errors(dp->read_pp, 1 );
#  endif  

#  ifdef FIX_INDEX
      if ((dp->options & FIX_INDEX) != 0)
         png_set_check_for_invalid_index(dp->read_pp, 1 );
#     ifdef IGNORE_INDEX
         else
#     endif  
#  endif  
#  ifdef IGNORE_INDEX
      if ((dp->options & IGNORE_INDEX) != 0)  
         png_set_check_for_invalid_index(dp->read_pp, -1 );
#  endif  

                                                                               
                            
      
   dp->ip = png_create_info_struct(dp->read_pp);
   if (dp->ip == NULL)
      png_error(dp->read_pp, "failed to create info struct");

    
   png_set_read_fn(dp->read_pp, dp, read_function);

#  ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
      png_set_keep_unknown_chunks(dp->read_pp, PNG_HANDLE_CHUNK_ALWAYS, NULL,
            0);
#  endif  

#  ifdef PNG_SET_USER_LIMITS_SUPPORTED
       
      png_set_user_limits(dp->read_pp, 0x7fffffff, 0x7fffffff);
#  endif  

    
   start_timer(dp, PNGCP_TIME_READ);
   png_read_png(dp->read_pp, dp->ip, 0U , NULL );
   end_timer(dp, PNGCP_TIME_READ);
   dp->w = png_get_image_width(dp->read_pp, dp->ip);
   dp->h = png_get_image_height(dp->read_pp, dp->ip);
   dp->ct = png_get_color_type(dp->read_pp, dp->ip);
   dp->bpp = png_get_bit_depth(dp->read_pp, dp->ip) *
             png_get_channels(dp->read_pp, dp->ip);
   {
                                                                               
                                                                                
                 
         
      png_alloc_size_t rb = png_get_rowbytes(dp->read_pp, dp->ip);

      if (rb == 0)
         png_error(dp->read_pp, "invalid row byte count from libpng");

       
      if ((MAX_SIZE-dp->h)/rb < dp->h)
         png_error(dp->read_pp, "image too large");

      dp->size = rb * dp->h + dp->h ;
   }

#ifdef FIX_INDEX
   if (dp->ct == PNG_COLOR_TYPE_PALETTE && (dp->options & FIX_INDEX) != 0)
   {
      int max = png_get_palette_max(dp->read_pp, dp->ip);
      png_colorp palette = NULL;
      int num = -1;

      if (png_get_PLTE(dp->read_pp, dp->ip, &palette, &num) != PNG_INFO_PLTE
          || max < 0 || num <= 0 || palette == NULL)
         display_log(dp, LIBPNG_ERROR, "invalid png_get_PLTE result");

      if (max >= num)
      {
          
         int i;
         png_color newpal[256];

         for (i=0; i<num; ++i)
            newpal[i] = palette[i];

          
         for (; i<=max; ++i)
         {
            newpal[i].red = 0xbe;
            newpal[i].green = 0xad;
            newpal[i].blue = 0xed;
         }

         png_set_PLTE(dp->read_pp, dp->ip, newpal, i);
      }
   }
#endif  

   display_clean_read(dp);
   dp->operation = "none";
}

static void
display_start_write(struct display *dp, const char *filename)
{
   assert(dp->fp == NULL);

   if ((dp->options & NOWRITE) != 0)
      dp->output_file = "<no write>";

   else
   {
      if (filename != NULL)
      {
         dp->output_file = filename;
         dp->fp = fopen(filename, "wb");
      }

      else
      {
         dp->output_file = "<stdout>";
         dp->fp = stdout;
      }

      if (dp->fp == NULL)
         display_log(dp, USER_ERROR, "%s: file open failed (%s)",
               dp->output_file, strerror(errno));
   }
}

static void PNGCBAPI
write_function(png_structp pp, png_bytep data, size_t size)
{
   struct display *dp = get_dp(pp);

                                                                            
                                                
      
   if (dp->fp == NULL || fwrite(data, size, 1U, dp->fp) == 1U)
   {
      dp->write_size += size;
      if (dp->write_size < size || dp->write_size == MAX_SIZE)
         png_error(pp, "IDAT size overflow");
   }

   else
      display_log(dp, USER_ERROR, "%s: PNG file write failed (%s)",
            dp->output_file, strerror(errno));
}

                                                                 
  
                                                                              
                                                                             
                                                                               
                                                                          
   
#define SET_COMPRESSION\
   SET(strategy, strategy);\
   SET(windowBits, window_bits);\
   SET(level, level);\
   SET(memLevel, mem_level);

#ifdef PNG_WRITE_CUSTOMIZE_COMPRESSION_SUPPORTED
static void
search_compression(struct display *dp)
{
    
   int val;

#  define SET(name, func) if (getsearchopts(dp, #name, &val))\
      png_set_compression_ ## func(dp->write_pp, val);
   SET_COMPRESSION
#  undef SET
}

static void
set_compression(struct display *dp)
{
   int val;

#  define SET(name, func) if (getallopts(dp, #name, &val))\
      png_set_compression_ ## func(dp->write_pp, val);
   SET_COMPRESSION
#  undef SET
}

#ifdef PNG_SW_COMPRESS_level  
static void
set_ICC_profile_compression(struct display *dp)
{
   int val;

#  define SET(name, func) if (getallopts(dp, "ICC-profile-" #name, &val))\
      png_set_ICC_profile_compression_ ## func(dp->write_pp, val);
   SET_COMPRESSION
#  undef SET
}
#else
#  define set_ICC_profile_compression(dp) ((void)0)
#endif
#else
#  define search_compression(dp) ((void)0)
#  define set_compression(dp) ((void)0)
#  define set_ICC_profile_compression(dp) ((void)0)
#endif  

#ifdef PNG_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED
static void
set_text_compression(struct display *dp)
{
   int val;

#  define SET(name, func) if (getallopts(dp, "text-" #name, &val))\
      png_set_text_compression_ ## func(dp->write_pp, val);
   SET_COMPRESSION
#  undef SET
}
#else
#  define set_text_compression(dp) ((void)0)
#endif  

static void
write_png(struct display *dp, const char *destname)
{
   display_clean_write(dp);  
   display_start_write(dp, destname);

   dp->write_pp = png_create_write_struct(PNG_LIBPNG_VER_STRING, dp,
      display_error, display_warning);

   if (dp->write_pp == NULL)
      display_log(dp, LIBPNG_ERROR, "failed to create write png_struct");

#  ifdef PNG_BENIGN_ERRORS_SUPPORTED
      png_set_benign_errors(dp->write_pp, 1 );
#  endif  

   png_set_write_fn(dp->write_pp, dp, write_function, NULL );

#ifdef IGNORE_INDEX
   if ((dp->options & IGNORE_INDEX) != 0)  
      png_set_check_for_invalid_index(dp->write_pp, -1 );
#endif  

                                                                            
                                                                    
                                                                          
      
   text_restore(dp);

#  ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
      png_set_keep_unknown_chunks(dp->write_pp, PNG_HANDLE_CHUNK_ALWAYS, NULL,
            0);
#  endif  

#  ifdef PNG_SET_USER_LIMITS_SUPPORTED
       
      png_set_user_limits(dp->write_pp, 0x7fffffff, 0x7fffffff);
#  endif

    
    
   dp->tsp = dp->nsp;
   dp->nsp = dp->csp = 0;
#  ifdef PNG_SW_COMPRESS_png_level
      {
         int val;

                                                                              
                                                            
            
         if (getallopts(dp, "compression", &val))
            png_set_compression(dp->write_pp, val);

         if (getallopts(dp, "ICC-profile-compression", &val))
            png_set_ICC_profile_compression(dp->write_pp, val);

         if (getallopts(dp, "text-compression", &val))
            png_set_text_compression(dp->write_pp, val);
      }
#  endif  
   if (dp->options & SEARCH)
      search_compression(dp);
   else
      set_compression(dp);
   set_ICC_profile_compression(dp);
   set_text_compression(dp);

   {
      int val;

       
      if (get_option(dp, "IDAT-size", &val))
         png_set_IDAT_size(dp->write_pp, val);
   }

    
#  ifdef PNG_WRITE_FILTER_SUPPORTED
      {
         int val;

         if (get_option(dp, "filter", &val))
            png_set_filter(dp->write_pp, PNG_FILTER_TYPE_BASE, val);
      }
#  endif  

    
   dp->write_size = 0U;
   start_timer(dp, PNGCP_TIME_WRITE);
   png_write_png(dp->write_pp, dp->ip, 0U , NULL );
   end_timer(dp, PNGCP_TIME_WRITE);

    
   if (dp->fp != NULL)
   {
      FILE *fp = dp->fp;
      dp->fp = NULL;
      if (fclose(fp))
         display_log(dp, APP_ERROR, "%s: write failed (%s)",
               destname == NULL ? "stdout" : destname, strerror(errno));
   }

                                                                         
                                              
      
   display_clean_write(dp);
   dp->operation = "none";
}

static void
set_windowBits_hi(struct display *dp)
{
                                                                             
                                                       
      
   int wb = MAX_WBITS;  
   int i = VLSIZE(windowBits_IDAT);

   while (wb > 8 && dp->size <= 1U<<(wb-1)) --wb;

   while (--i >= 0) if (VLNAME(windowBits_IDAT)[i].name == range_hi) break;

   assert(i > 1);  
   VLNAME(windowBits_IDAT)[i].value = wb;

   assert(VLNAME(windowBits_IDAT)[--i].name == range_lo);
   VLNAME(windowBits_IDAT)[i].value = wb > 8 ? 9 : 8;

                                                                           
                                                                              
            
      
   if (wb == 8)
      dp->min_windowBits = OPTIND(dp, windowBits);
}

static int
better_options(const struct display *dp)
{
                                                                        
                                                                              
                                                                               
                                                         
     
                                                                             
                                                                              
                                                                             
                     
     
                                                                             
                                                                             
           
      
   unsigned int sp;

   for (sp=0; sp<dp->csp; ++sp)
   {
      int c = compare_option(dp, sp);

      if (c < 0)
         return 0;  

      else if (c > 0)
         return 1;  
   }

   assert(0 && "unreached");
}

static void
print_search_results(struct display *dp)
{
   assert(dp->filename != NULL);
   printf("%s [%ld x %ld %d bpp %s, %lu bytes] %lu -> %lu with '%s'\n",
      dp->filename, (unsigned long)dp->w, (unsigned long)dp->h, dp->bpp,
      cts(dp->ct), (unsigned long)dp->size, (unsigned long)dp->read_size,
      (unsigned long)dp->best_size, dp->best);
   fflush(stdout);
}

static void
log_search(struct display *dp, unsigned int log_depth)
{
    
   if (dp->nsp  <= log_depth)
   {
      print_search_results(dp);
       
      dp->best_size = MAX_SIZE;
   }
}

static void
cp_one_file(struct display *dp, const char *filename, const char *destname)
{
   unsigned int log_depth;

   dp->filename = filename;
   dp->operation = "read";
   dp->no_warnings = 0;

    
   if (filename != NULL && access(filename, R_OK) != 0)
      display_log(dp, USER_ERROR, "%s: invalid file name (%s)",
            filename, strerror(errno));

   read_png(dp, filename);

    
   dp->operation = "write";

    
   set_windowBits_hi(dp);

    
   {
      int val;

      if (get_option(dp, "log-depth", &val) && val >= 0)
         log_depth = (unsigned int) val;

      else
         log_depth = 0U;
   }

   if (destname != NULL)  
   {
      if (isdir(dp, destname))
      {
         makename(dp, destname, filename);
         destname = dp->namebuf;
      }

      else if (access(destname, W_OK) != 0 && errno != ENOENT)
         display_log(dp, USER_ERROR, "%s: invalid output name (%s)", destname,
               strerror(errno));
   }

   dp->nsp = 0;
   dp->curr[0] = 0;  
   dp->opt_string_start = 0;
   dp->best[0] = 0;  
   dp->best_size = MAX_SIZE;
   write_png(dp, destname);

    
   strcpy(dp->best, dp->curr);
   dp->best_size = dp->write_size;

   if (dp->nsp > 0)  
   {
      char *tmpname, tmpbuf[(sizeof dp->namebuf) + 4];
      assert(dp->curr[0] == ' ' && dp->tsp > 0);

       
      log_search(dp, log_depth);
      dp->no_warnings = 1;

       
      if (destname != NULL)
      {
         strcpy(tmpbuf, destname);
         strcat(tmpbuf, ".tmp");  
         tmpname = tmpbuf;
      }

      else
         tmpname = NULL;  

       
      do
      {
         write_png(dp, tmpname);

                                                                           
                              
            
         assert(dp->csp > 0);

         if (dp->write_size < dp->best_size ||
             (dp->write_size == dp->best_size && better_options(dp)))
         {
            if (destname != NULL && rename(tmpname, destname) != 0)
               display_log(dp, APP_ERROR, "rename %s %s failed (%s)", tmpname,
                     destname, strerror(errno));

            strcpy(dp->best, dp->curr);
            dp->best_size = dp->write_size;
         }

         else if (tmpname != NULL && unlink(tmpname) != 0)
            display_log(dp, APP_WARNING, "unlink %s failed (%s)", tmpname,
                  strerror(errno));

         log_search(dp, log_depth);
      }
      while (dp->nsp > 0);

       
      dp->write_size = dp->best_size;
   }
}

static int
cppng(struct display *dp, const char *file, const char *gv dest)
                                                                             
                           
      
{
   int ret = setjmp(dp->error_return);

   if (ret == 0)
   {
      dp->errset = 1;
      cp_one_file(dp, file, dest);
      dp->errset = 0;
      return 0;
   }

   else
   {
      dp->errset = 0;

      if (ret < ERRORS)  
         display_log(dp, INTERNAL_ERROR, "unexpected return code %d", ret);

      return ret;
   }
}

int
main(int argc, char **argv)
{
    
   int option_end;
   struct display d;

   display_init(&d);

   d.operation = "options";
   for (option_end = 1;
        option_end < argc && opt_check(&d, argv[option_end]);
        ++option_end)
   {
   }

                                                                             
                                                     
      
   if (!(d.options & NOWRITE) && option_end+2 < argc && !checkdir(argv[argc-1]))
   {
      fprintf(stderr,
            "pngcp: %s: directory required with more than two arguments\n",
            argv[argc-1]);
      return 99;
   }

   {
      int errors = 0;
      int i = option_end;

                                                                                
         
      d.operation = "files";
      do
      {
         const char *infile = NULL;
         const char *outfile = NULL;
         int ret;

         if (i < argc)
         {
            infile = argv[i++];
            if (!(d.options & NOWRITE) && i < argc)
               outfile = argv[argc-1];
         }

         ret = cppng(&d, infile, outfile);

         if (ret)
         {
            if (ret > QUIET)  
               return 99;

             
         }

         else if (d.best[0] != 0)
         {
                                                                                
                              
               
            if (d.best_size < MAX_SIZE)
               print_search_results(&d);
         }

         else if (d.options & SIZES)
         {
            printf("%s [%ld x %ld %d bpp %s, %lu bytes] %lu -> %lu [0x%lx]\n",
                  infile, (unsigned long)d.w, (unsigned long)d.h, d.bpp,
                  cts(d.ct), (unsigned long)d.size, (unsigned long)d.read_size,
                  (unsigned long)d.write_size, (unsigned long)d.results);
            fflush(stdout);
         }

                                                                               
            
         {
            int pass = (d.options & STRICT) ?
               RESULT_STRICT(d.results) : RESULT_RELAXED(d.results);

            if (!pass)
               ++errors;

            if (d.options & LOG)
            {
               int j;

               printf("%s: pngcp", pass ? "PASS" : "FAIL");

               for (j=1; j<option_end; ++j)
                  printf(" %s", argv[j]);

               if (infile != NULL)
                  printf(" %s", infile);

#              ifdef PNG_PNGCP_TIMING_SUPPORTED
                   
                  if ((d.value[OPTIND(&d,time)] & PNGCP_TIME_READ) != 0)
                     print_time(" read", d.read_time);

                  if ((d.value[OPTIND(&d,time)] & PNGCP_TIME_WRITE) != 0)
                     print_time(" write", d.write_time);
#              endif  

               printf("\n");
               fflush(stdout);
            }
         }

         display_clean(&d);
      }
      while (i+!(d.options & NOWRITE) < argc);
                                                                              
                                                                                
                                               
            

       
      display_destroy(&d);

#     ifdef PNG_PNGCP_TIMING_SUPPORTED
         {
            int output = 0;

            if ((d.value[OPTIND(&d,time)] & PNGCP_TIME_READ) != 0)
               print_time("read", d.read_time_total), output = 1;

            if ((d.value[OPTIND(&d,time)] & PNGCP_TIME_WRITE) != 0)
            {
               if (output) putchar(' ');
               print_time("write", d.write_time_total);
               output = 1;
            }

            if (output) putchar('\n');
         }
#     endif  

      return errors != 0;
   }
}
#else  
int
main(void)
{
   fprintf(stderr, "pngcp: no support for png_read/write_image\n");
   return 77;
}
#endif  
