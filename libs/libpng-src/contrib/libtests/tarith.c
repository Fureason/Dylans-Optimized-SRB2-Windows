
/* tarith.c
 *
 * Copyright (c) 2011-2013 John Cunningham Bowler
 *
 * Last changed in libpng 1.6.0 [February 14, 2013]
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * Test internal arithmetic functions of libpng.
 *
 * This code must be linked against a math library (-lm), but does not require
 * libpng or zlib to work.  Because it includes the complete source of 'png.c'
 * it tests the code with whatever compiler options are used to build it.
 * Changing these options can substantially change the errors in the
 * calculations that the compiler chooses!
 */
#define _POSIX_SOURCE 1
#define _ISOC99_SOURCE 1

                                                                        
                              
   
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "../../pngpriv.h"

#define png_error png_warning

void png_warning(png_const_structrp png_ptr, png_const_charp msg)
{
   fprintf(stderr, "validation: %s\n", msg);
}

#define png_fixed_error png_fixed_warning

void png_fixed_warning(png_const_structrp png_ptr, png_const_charp msg)
{
   fprintf(stderr, "overflow in: %s\n", msg);
}

#define png_set_error_fn(pp, ep, efp, wfp) ((void)0)
#define png_malloc(pp, s) malloc(s)
#define png_malloc_warn(pp, s) malloc(s)
#define png_malloc_base(pp, s) malloc(s)
#define png_calloc(pp, s) calloc(1, (s))
#define png_free(pp, s) free(s)

#define png_safecat(b, sb, pos, str) (pos)
#define png_format_number(start, end, format, number) (start)

#define crc32(crc, pp, s) (crc)
#define inflateReset(zs) Z_OK

#define png_create_struct(type) (0)
#define png_destroy_struct(pp) ((void)0)
#define png_create_struct_2(type, m, mm) (0)
#define png_destroy_struct_2(pp, f, mm) ((void)0)

#undef PNG_SIMPLIFIED_READ_SUPPORTED
#undef PNG_SIMPLIFIED_WRITE_SUPPORTED
#undef PNG_USER_MEM_SUPPORTED

#include "../../png.c"

 
static int verbose = 0;

int validation_ascii_to_fp(int count, int argc, char **argv)
{
   int    showall = 0;
   double max_error=2;       
   double max_error_abs=17;  
   double max = 0;
   double max_abs = 0;
   double test = 0;  
   int    precision = 5;
   int    nonfinite = 0;
   int    finite = 0;
   int    ok = 0;
   int    failcount = 0;
   int    minorarith = 0;

   while (--argc > 0)
      if (strcmp(*++argv, "-a") == 0)
         showall = 1;
      else if (strcmp(*argv, "-e") == 0 && argc > 0)
      {
         --argc;
         max_error = atof(*++argv);
      }
      else if (strcmp(*argv, "-E") == 0 && argc > 0)
      {
         --argc;
         max_error_abs = atof(*++argv);
      }
      else
      {
         fprintf(stderr, "unknown argument %s\n", *argv);
         return 1;
      }

   do
   {
      size_t index;
      int state, failed = 0;
      char buffer[64];

      if (isfinite(test))
         ++finite;
      else
         ++nonfinite;

      if (verbose)
         fprintf(stderr, "%.*g %d\n", DBL_DIG, test, precision);

       
      memset(buffer, 71, sizeof buffer);

      png_ascii_from_fp(0, buffer, precision+10, test, precision);

                                                                  
                                          
         
      if (buffer[precision+7] != 71)
      {
         fprintf(stderr, "%g[%d] -> '%s'[%lu] buffer overflow\n", test,
            precision, buffer, (unsigned long)strlen(buffer));
         failed = 1;
      }

                                                                   
                             
         
      state = 0;
      index = 0;
      if (!isfinite(test))
      {
          
         if (test >= 0 && strcmp(buffer, "inf") ||
             test <  0 && strcmp(buffer, "-inf"))
         {
            fprintf(stderr, "%g[%d] -> '%s' but expected 'inf'\n", test,
               precision, buffer);
            failed = 1;
         }
      }
      else if (!png_check_fp_number(buffer, precision+10, &state, &index) ||
          buffer[index] != 0)
      {
         fprintf(stderr, "%g[%d] -> '%s' but has bad format ('%c')\n", test,
         precision, buffer, buffer[index]);
         failed = 1;
      }
      else if (PNG_FP_IS_NEGATIVE(state) && !(test < 0))
      {
         fprintf(stderr, "%g[%d] -> '%s' but negative value not so reported\n",
            test, precision, buffer);
         failed = 1;
         assert(!PNG_FP_IS_ZERO(state));
         assert(!PNG_FP_IS_POSITIVE(state));
      }
      else if (PNG_FP_IS_ZERO(state) && !(test == 0))
      {
         fprintf(stderr, "%g[%d] -> '%s' but zero value not so reported\n",
            test, precision, buffer);
         failed = 1;
         assert(!PNG_FP_IS_NEGATIVE(state));
         assert(!PNG_FP_IS_POSITIVE(state));
      }
      else if (PNG_FP_IS_POSITIVE(state) && !(test > 0))
      {
         fprintf(stderr, "%g[%d] -> '%s' but positive value not so reported\n",
            test, precision, buffer);
         failed = 1;
         assert(!PNG_FP_IS_NEGATIVE(state));
         assert(!PNG_FP_IS_ZERO(state));
      }
      else
      {
          
         double out = atof(buffer);
         double change = fabs((out - test)/test);
         double allow = .5/pow(10,
            (precision >= DBL_DIG) ? DBL_DIG-1 : precision-1);

                                                                       
                                                                   
                                                                       
                                                                      
                                                                        
                                                                  
            
         if (change >= allow && (isfinite(out) ||
             fabs(test/DBL_MAX) <= 1-allow))
         {
            double percent = (precision >= DBL_DIG) ? max_error_abs : max_error;
            double allowp = (change-allow)*100/allow;

            if (precision >= DBL_DIG)
            {
               if (max_abs < allowp) max_abs = allowp;
            }

            else
            {
               if (max < allowp) max = allowp;
            }

            if (showall || allowp >= percent)
            {
               fprintf(stderr,
                  "%.*g[%d] -> '%s' -> %.*g number changed (%g > %g (%d%%))\n",
                  DBL_DIG, test, precision, buffer, DBL_DIG, out, change, allow,
                  (int)round(allowp));
               failed = 1;
            }
            else
               ++minorarith;
         }
      }

      if (failed)
         ++failcount;
      else
         ++ok;

skip:
       
      precision = rand();
      if (precision & 1) test = -test;
      precision >>= 1;

       
      if (test == 0 || !isfinite(test))
         test = precision+1;
      else
      {
          
         int exponent = precision % (DBL_MAX_EXP - DBL_MIN_EXP) + DBL_MIN_EXP;
         int tmp;
         test = frexp(test * rand(), &tmp);
         test = ldexp(test, exponent);
         precision >>= 8;  
      }

                                                                    
                                                           
         
      precision = (precision & 0x1f) + 1;
   }
   while (--count);

   printf("Tested %d finite values, %d non-finite, %d OK (%d failed) %d minor "
      "arithmetic errors\n", finite, nonfinite, ok, failcount, minorarith);
   printf(" Error with >=%d digit precision %.2f%%\n", DBL_DIG, max_abs);
   printf(" Error with < %d digit precision %.2f%%\n", DBL_DIG, max);

   return 0;
}

                                                                            
                 
  
                                                               
  
                                                       
   
typedef enum checkfp_state
{
   start, fraction, exponent, states
} checkfp_state;

 
static const char none[] = "";
static const char hexdigits[16] = "0123456789ABCDEF";

static const struct
{
   const char *start;  
   const char *end;    
   const char *tests;  
}
state_characters[states] =
{
     { "+-.", ".eE", "+-.e*0369" },
     { none, "eE",  "+-.E#0147" },
     { "+-", none,  "+-.eE^0258" }
};

typedef struct
{
   char number[1024];   
   int  limit;          
   int  verbose;        
   int  ctimes;         
   int  cmillions;      
   int  cinvalid;       
   int  cnoaccept;      
}
checkfp_command;

typedef struct
{
   int           cnumber;           
   checkfp_state check_state;       
   int           at_start;          
   int           cdigits_in_state;  
   int           limit;             
   int           state;             
   int           is_negative;       
   int           is_zero;           
   int           number_was_valid;  
}
checkfp_control;

static int check_all_characters(checkfp_command *co, checkfp_control c);

static int check_some_characters(checkfp_command *co, checkfp_control c,
   const char *tests);

static int check_one_character(checkfp_command *co, checkfp_control c, int ch)
{
                                                                           
      
   size_t index = 0;
   const char test = (char)ch;
   int number_is_valid = png_check_fp_number(&test, 1, &c.state, &index);
   int character_accepted = (index == 1);

   if (c.check_state != exponent && isdigit(ch) && ch != '0')
      c.is_zero = 0;

   if (c.check_state == start && c.at_start && ch == '-')
      c.is_negative = 1;

   if (isprint(ch))
      co->number[c.cnumber++] = (char)ch;
   else
   {
      co->number[c.cnumber++] = '<';
      co->number[c.cnumber++] = hexdigits[(ch >> 4) & 0xf];
      co->number[c.cnumber++] = hexdigits[ch & 0xf];
      co->number[c.cnumber++] = '>';
   }
   co->number[c.cnumber] = 0;

   if (co->verbose > 1)
      fprintf(stderr, "%s\n", co->number);

   if (++(co->ctimes) == 1000000)
   {
      if (co->verbose == 1)
         fputc('.', stderr);
      co->ctimes = 0;
      ++(co->cmillions);
   }

   if (!number_is_valid)
      ++(co->cinvalid);

   if (!character_accepted)
      ++(co->cnoaccept);

    
   if (index != 0 && index != 1)
   {
      fprintf(stderr, "%s: read beyond end of string (%lu)\n", co->number,
         (unsigned long)index);
      return 0;
   }

                                                                        
                                       
      
   if (PNG_FP_IS_NEGATIVE(c.state) !=
      (number_is_valid && !c.is_zero && c.is_negative))
   {
      fprintf(stderr, "%s: negative when it is not\n", co->number);
      return 0;
   }

   if (PNG_FP_IS_ZERO(c.state) != (number_is_valid && c.is_zero))
   {
      fprintf(stderr, "%s: zero when it is not\n", co->number);
      return 0;
   }

   if (PNG_FP_IS_POSITIVE(c.state) !=
      (number_is_valid && !c.is_zero && !c.is_negative))
   {
      fprintf(stderr, "%s: positive when it is not\n", co->number);
      return 0;
   }

    
   if (isdigit(ch))
   {
      if (!character_accepted)
      {
         fprintf(stderr, "%s: digit '%c' not accepted\n", co->number, ch);
         return 0;
      }

      if (!number_is_valid)
      {
         fprintf(stderr, "%s: saw a digit (%c) but number not valid\n",
            co->number, ch);
         return 0;
      }

      ++c.cdigits_in_state;
      c.at_start = 0;
      c.number_was_valid = 1;

                                                                       
                                                                              
                     
         
      if (c.cdigits_in_state < 1)
         return check_all_characters(co, c);

      else
         return check_some_characters(co, c,
            state_characters[c.check_state].tests);
   }

    
   else if (((ch == '+' || ch == '-') && c.check_state != fraction &&
               c.at_start) ||
            (ch == '.' && c.check_state == start) ||
            ((ch == 'e' || ch == 'E') && c.number_was_valid &&
               c.check_state != exponent))
   {
      if (!character_accepted)
      {
         fprintf(stderr, "%s: character '%c' not accepted\n", co->number, ch);
         return 0;
      }

       
      if (number_is_valid && (c.check_state != start || ch != '.'))
      {
         fprintf(stderr, "%s: saw a non-digit (%c) but number valid\n",
            co->number, ch);
         return 0;
      }

      c.number_was_valid = number_is_valid;

                                                                             
                                                                              
                          
         
      if (c.check_state == start && ch == '.')
      {
         c.check_state = fraction;
         c.at_start = !number_is_valid;
         c.cdigits_in_state = 0;
         c.limit = co->limit;
         return check_all_characters(co, c);
      }

      else if (c.check_state < exponent && (ch == 'e' || ch == 'E'))
      {
         c.check_state = exponent;
         c.at_start = 1;
         c.cdigits_in_state = 0;
         c.limit = co->limit;
         return check_all_characters(co, c);
      }

       
      else
      {
         if (ch != '-' && ch != '+')
         {
            fprintf(stderr, "checkfp: internal error (1)\n");
            return 0;
         }

         c.at_start = 0;
         return check_all_characters(co, c);
      }
   }

    
   else
   {
      if (character_accepted)
      {
         fprintf(stderr, "%s: character '%c' [0x%.2x] accepted\n", co->number,
            ch, ch);
         return 0;
      }

      if (number_is_valid != c.number_was_valid)
      {
         fprintf(stderr,
            "%s: character '%c' [0x%.2x] changed number validity\n", co->number,
            ch, ch);
         return 0;
      }

                                                                              
                            
         
   }

    
   return 1;
}

static int check_all_characters(checkfp_command *co, checkfp_control c)
{
   int ch;

   if (c.cnumber+4 < sizeof co->number) for (ch=0; ch<256; ++ch)
   {
      if (!check_one_character(co, c, ch))
         return 0;
   }

   return 1;
}

static int check_some_characters(checkfp_command *co, checkfp_control c,
   const char *tests)
{
   int i;

   --(c.limit);

   if (c.cnumber+4 < sizeof co->number && c.limit >= 0)
   {
      if (c.limit > 0) for (i=0; tests[i]; ++i)
      {
         if (!check_one_character(co, c, tests[i]))
               return 0;
      }

       
      else
         return check_all_characters(co, c);
   }

   return 1;
}

int validation_checkfp(int count, int argc, char **argv)
{
   int result;
   checkfp_command command;
   checkfp_control control;

   command.number[0] = 0;
   command.limit = 3;
   command.verbose = verbose;
   command.ctimes = 0;
   command.cmillions = 0;
   command.cinvalid = 0;
   command.cnoaccept = 0;

   while (--argc > 0)
   {
      ++argv;
      if (argc > 1 && strcmp(*argv, "-l") == 0)
      {
         --argc;
         command.limit = atoi(*++argv);
      }

      else
      {
         fprintf(stderr, "unknown argument %s\n", *argv);
         return 1;
      }
   }

   control.cnumber = 0;
   control.check_state = start;
   control.at_start = 1;
   control.cdigits_in_state = 0;
   control.limit = command.limit;
   control.state = 0;
   control.is_negative = 0;
   control.is_zero = 1;
   control.number_was_valid = 0;

   result = check_all_characters(&command, control);

   printf("checkfp: %s: checked %d,%.3d,%.3d,%.3d strings (%d invalid)\n",
      result ? "pass" : "FAIL", command.cmillions / 1000,
      command.cmillions % 1000, command.ctimes / 1000, command.ctimes % 1000,
      command.cinvalid);

   return result;
}

int validation_muldiv(int count, int argc, char **argv)
{
   int tested = 0;
   int overflow = 0;
   int error = 0;
   int error64 = 0;
   int passed = 0;
   int randbits = 0;
   png_uint_32 randbuffer;
   png_fixed_point a;
   png_int_32 times, div;

   while (--argc > 0)
      {
         fprintf(stderr, "unknown argument %s\n", *++argv);
         return 1;
      }

    
   randbuffer = RAND_MAX;
   while (randbuffer != 0) ++randbits, randbuffer >>= 1;
   printf("Using random number generator that makes %d bits\n", randbits);
   for (div=0; div<32; div += randbits)
      randbuffer = (randbuffer << randbits) ^ rand();

   a = 0;
   times = div = 0;
   do
   {
      png_fixed_point result;
                                                                       
                                                                    
                                     
         
      long long int fp, fpround;
      unsigned long hi, lo;
      int ok;

                                                                     
                                           
         
      {
         long u1, u2;
         int n = 0;
         if (a < 0) u1 = -a, n = 1; else u1 = a;
         if (times < 0) u2 = -times, n = !n; else u2 = times;
         png_64bit_product(u1, u2, &hi, &lo);
         if (n)
         {
             
            lo = ((~lo) + 1) & 0xffffffff;
            hi = ~hi;
            if (lo == 0) ++hi;
         }
      }

      fp = a;
      fp *= times;
      if ((fp & 0xffffffff) != lo || ((fp >> 32) & 0xffffffff) != hi)
      {
         fprintf(stderr, "png_64bit_product %d * %d -> %lx|%.8lx not %llx\n",
            a, times, hi, lo, fp);
         ++error64;
      }

      if (div != 0)
      {
          
         if ((fp < 0) != (div < 0))
           fp -= div/2;
         else
           fp += div/2;

         fp /= div;
         fpround = fp;
          
         ok = fpround <= PNG_UINT_31_MAX &&
              fpround >= -1-(long long int)PNG_UINT_31_MAX;
         if (!ok) ++overflow;
      }
      else
        ok = 0, ++overflow, fpround = fp ;

      if (verbose)
         fprintf(stderr, "TEST %d * %d / %d -> %lld (%s)\n", a, times, div,
            fp, ok ? "ok" : "overflow");

      ++tested;
      if (png_muldiv(&result, a, times, div) != ok)
      {
         ++error;
         if (ok)
             fprintf(stderr, "%d * %d / %d -> overflow (expected %lld)\n", a,
                times, div, fp);
         else
             fprintf(stderr, "%d * %d / %d -> %d (expected overflow %lld)\n", a,
                times, div, result, fp);
      }
      else if (ok && result != fpround)
      {
         ++error;
         fprintf(stderr, "%d * %d / %d -> %d not %lld\n", a, times, div, result,
            fp);
      }
      else
         ++passed;

                                                                            
                        
         
       
      a += times;
      times += div;
      div = randbuffer;
      randbuffer = (randbuffer << randbits) ^ rand();
   }
   while (--count > 0);

   printf("%d tests including %d overflows, %d passed, %d failed (%d 64-bit "
      "errors)\n", tested, overflow, passed, error, error64);
   return 0;
}

                                                                                
              
   
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
#define LN2 .000010576586617430806112933839  
#define L2INV 94548.46219969910586572651     

 
static png_uint_32 png_log8bit(unsigned x)
{
   if (x > 0)
      return (png_uint_32)floor(.5-log(x/255.)*L2INV);

   return 0xffffffff;
}

static png_uint_32 png_log16bit(png_uint_32 x)
{
   if (x > 0)
      return (png_uint_32)floor(.5-log(x/65535.)*L2INV);

   return 0xffffffff;
}

static png_uint_32 png_exp(png_uint_32 x)
{
   return (png_uint_32)floor(.5 + exp(x * -LN2) * 0xffffffffU);
}

static png_byte png_exp8bit(png_uint_32 log)
{
   return (png_byte)floor(.5 + exp(log * -LN2) * 255);
}

static png_uint_16 png_exp16bit(png_uint_32 log)
{
   return (png_uint_16)floor(.5 + exp(log * -LN2) * 65535);
}
#endif  

int validation_gamma(int argc, char **argv)
{
   double gamma[9] = { 2.2, 1.8, 1.52, 1.45, 1., 1/1.45, 1/1.52, 1/1.8, 1/2.2 };
   double maxerr;
   int i, silent=0, onlygamma=0;

    
   while (--argc > 0)
      if (strcmp(*++argv, "-s") == 0)
         silent = 1;
      else if (strcmp(*argv, "-g") == 0)
         onlygamma = 1;
      else
      {
         fprintf(stderr, "unknown argument %s\n", *argv);
         return 1;
      }

   if (!onlygamma)
   {
       
      maxerr = 0;
      for (i=0; i<256; ++i)
      {
         double correct = -log(i/255.)/log(2.)*65536;
         double error = png_log8bit(i) - correct;

         if (i != 0 && fabs(error) > maxerr)
            maxerr = fabs(error);

         if (i == 0 && png_log8bit(i) != 0xffffffff ||
             i != 0 && png_log8bit(i) != floor(correct+.5))
         {
            fprintf(stderr, "8-bit log error: %d: got %u, expected %f\n",
               i, png_log8bit(i), correct);
         }
      }

      if (!silent)
         printf("maximum 8-bit log error = %f\n", maxerr);

      maxerr = 0;
      for (i=0; i<65536; ++i)
      {
         double correct = -log(i/65535.)/log(2.)*65536;
         double error = png_log16bit(i) - correct;

         if (i != 0 && fabs(error) > maxerr)
            maxerr = fabs(error);

         if (i == 0 && png_log16bit(i) != 0xffffffff ||
             i != 0 && png_log16bit(i) != floor(correct+.5))
         {
            if (error > .68)  
            {
               fprintf(stderr, "16-bit log error: %d: got %u, expected %f"
                  " error: %f\n", i, png_log16bit(i), correct, error);
            }
         }
      }

      if (!silent)
         printf("maximum 16-bit log error = %f\n", maxerr);

       
      maxerr = 0;
      for (i=0; i<=0xfffff; ++i)
      {
         double correct = exp(-i/65536. * log(2.)) * (65536. * 65536);
         double error = png_exp(i) - correct;

         if (fabs(error) > maxerr)
            maxerr = fabs(error);
         if (fabs(error) > 1883)  
         {
            fprintf(stderr, "32-bit exp error: %d: got %u, expected %f"
                  " error: %f\n", i, png_exp(i), correct, error);
         }
      }

      if (!silent)
         printf("maximum 32-bit exp error = %f\n", maxerr);

      maxerr = 0;
      for (i=0; i<=0xfffff; ++i)
      {
         double correct = exp(-i/65536. * log(2.)) * 255;
         double error = png_exp8bit(i) - correct;

         if (fabs(error) > maxerr)
            maxerr = fabs(error);
         if (fabs(error) > .50002)  
         {
            fprintf(stderr, "8-bit exp error: %d: got %u, expected %f"
                  " error: %f\n", i, png_exp8bit(i), correct, error);
         }
      }

      if (!silent)
         printf("maximum 8-bit exp error = %f\n", maxerr);

      maxerr = 0;
      for (i=0; i<=0xfffff; ++i)
      {
         double correct = exp(-i/65536. * log(2.)) * 65535;
         double error = png_exp16bit(i) - correct;

         if (fabs(error) > maxerr)
            maxerr = fabs(error);
         if (fabs(error) > .524)  
         {
            fprintf(stderr, "16-bit exp error: %d: got %u, expected %f"
                  " error: %f\n", i, png_exp16bit(i), correct, error);
         }
      }

      if (!silent)
         printf("maximum 16-bit exp error = %f\n", maxerr);
   }  

    
   for (i=0; i<9; ++i)
   {
      unsigned j;
      double g = gamma[i];
      png_fixed_point gfp = floor(g * PNG_FP_1 + .5);

      if (!silent)
         printf("Test gamma %f\n", g);

      maxerr = 0;
      for (j=0; j<256; ++j)
      {
         double correct = pow(j/255., g) * 255;
         png_byte out = png_gamma_8bit_correct(j, gfp);
         double error = out - correct;

         if (fabs(error) > maxerr)
            maxerr = fabs(error);
         if (out != floor(correct+.5))
         {
            fprintf(stderr, "8bit %d ^ %f: got %d expected %f error %f\n",
               j, g, out, correct, error);
         }
      }

      if (!silent)
         printf("gamma %f: maximum 8-bit error %f\n", g, maxerr);

      maxerr = 0;
      for (j=0; j<65536; ++j)
      {
         double correct = pow(j/65535., g) * 65535;
         png_uint_16 out = png_gamma_16bit_correct(j, gfp);
         double error = out - correct;

         if (fabs(error) > maxerr)
            maxerr = fabs(error);
         if (fabs(error) > 1.62)
         {
            fprintf(stderr, "16bit %d ^ %f: got %d expected %f error %f\n",
               j, g, out, correct, error);
         }
      }

      if (!silent)
         printf("gamma %f: maximum 16-bit error %f\n", g, maxerr);
   }

   return 0;
}

 
                                                                     
                                                                    
  
                                                       
                          
                                               
   

                                                                     
             
   
#define COUNT 1000000000

int main(int argc, char **argv)
{
   int count = COUNT;

   while (argc > 1)
   {
      if (argc > 2 && strcmp(argv[1], "-c") == 0)
      {
         count = atoi(argv[2]);
         argc -= 2;
         argv += 2;
      }

      else if (strcmp(argv[1], "-v") == 0)
      {
         ++verbose;
         --argc;
         ++argv;
      }

      else
         break;
   }

   if (count > 0 && argc > 1)
   {
      if (strcmp(argv[1], "ascii") == 0)
         return validation_ascii_to_fp(count, argc-1, argv+1);
      else if (strcmp(argv[1], "checkfp") == 0)
         return validation_checkfp(count, argc-1, argv+1);
      else if (strcmp(argv[1], "muldiv") == 0)
         return validation_muldiv(count, argc-1, argv+1);
      else if (strcmp(argv[1], "gamma") == 0)
         return validation_gamma(argc-1, argv+1);
   }

    
   fprintf(stderr,
      "usage: tarith [-v] [-c count] {ascii,muldiv,gamma} [args]\n");
   fprintf(stderr, " arguments: ascii [-a (all results)] [-e error%%]\n");
   fprintf(stderr, "            checkfp [-l max-number-chars]\n");
   fprintf(stderr, "            muldiv\n");
   fprintf(stderr, "            gamma -s (silent) -g (only gamma; no log)\n");
   return 1;
}
