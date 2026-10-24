
/* filter_sse2_intrinsics.c - SSE2 optimized filter functions
 *
 * Copyright (c) 2018 Cosmin Truta
 * Copyright (c) 2016-2017 Glenn Randers-Pehrson
 * Written by Mike Klein and Matt Sarett
 * Derived from arm/filter_neon_intrinsics.c
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 */

#include "../pngpriv.h"

#ifdef PNG_READ_SUPPORTED

#if PNG_INTEL_SSE_IMPLEMENTATION > 0

#include <immintrin.h>

                                                                               
                                
                
                
                                                                         
                                                 
   

static __m128i load4(const void* p) {
   int tmp;
   memcpy(&tmp, p, sizeof(tmp));
   return _mm_cvtsi32_si128(tmp);
}

static void store4(void* p, __m128i v) {
   int tmp = _mm_cvtsi128_si32(v);
   memcpy(p, &tmp, sizeof(int));
}

static __m128i load3(const void* p) {
   png_uint_32 tmp = 0;
   memcpy(&tmp, p, 3);
   return _mm_cvtsi32_si128(tmp);
}

static void store3(void* p, __m128i v) {
   int tmp = _mm_cvtsi128_si32(v);
   memcpy(p, &tmp, 3);
}

void png_read_filter_row_sub3_sse2(png_row_infop row_info, png_bytep row,
   png_const_bytep prev)
{
                                                                  
                                                                               
                                                                            
      
   size_t rb;

   __m128i a, d = _mm_setzero_si128();

   png_debug(1, "in png_read_filter_row_sub3_sse2");

   rb = row_info->rowbytes;
   while (rb >= 4) {
      a = d; d = load4(row);
      d = _mm_add_epi8(d, a);
      store3(row, d);

      row += 3;
      rb  -= 3;
   }
   if (rb > 0) {
      a = d; d = load3(row);
      d = _mm_add_epi8(d, a);
      store3(row, d);

      row += 3;
      rb  -= 3;
   }
   PNG_UNUSED(prev)
}

void png_read_filter_row_sub4_sse2(png_row_infop row_info, png_bytep row,
   png_const_bytep prev)
{
                                                                  
                                                                               
                                                                            
      
   size_t rb;

   __m128i a, d = _mm_setzero_si128();

   png_debug(1, "in png_read_filter_row_sub4_sse2");

   rb = row_info->rowbytes+4;
   while (rb > 4) {
      a = d; d = load4(row);
      d = _mm_add_epi8(d, a);
      store4(row, d);

      row += 4;
      rb  -= 4;
   }
   PNG_UNUSED(prev)
}

void png_read_filter_row_avg3_sse2(png_row_infop row_info, png_bytep row,
   png_const_bytep prev)
{
                                                                               
                                                                     
                                                                       
                                                               
      

   size_t rb;

   const __m128i zero = _mm_setzero_si128();

   __m128i    b;
   __m128i a, d = zero;

   png_debug(1, "in png_read_filter_row_avg3_sse2");
   rb = row_info->rowbytes;
   while (rb >= 4) {
      __m128i avg;
             b = load4(prev);
      a = d; d = load4(row );

       
      avg = _mm_avg_epu8(a,b);
       
      avg = _mm_sub_epi8(avg, _mm_and_si128(_mm_xor_si128(a,b),
                                            _mm_set1_epi8(1)));
      d = _mm_add_epi8(d, avg);
      store3(row, d);

      prev += 3;
      row  += 3;
      rb   -= 3;
   }
   if (rb > 0) {
      __m128i avg;
             b = load3(prev);
      a = d; d = load3(row );

       
      avg = _mm_avg_epu8(a,b);
       
      avg = _mm_sub_epi8(avg, _mm_and_si128(_mm_xor_si128(a,b),
                                            _mm_set1_epi8(1)));

      d = _mm_add_epi8(d, avg);
      store3(row, d);

      prev += 3;
      row  += 3;
      rb   -= 3;
   }
}

void png_read_filter_row_avg4_sse2(png_row_infop row_info, png_bytep row,
   png_const_bytep prev)
{
                                                                               
                                                                     
                                                                       
                                                               
      
   size_t rb;
   const __m128i zero = _mm_setzero_si128();
   __m128i    b;
   __m128i a, d = zero;

   png_debug(1, "in png_read_filter_row_avg4_sse2");

   rb = row_info->rowbytes+4;
   while (rb > 4) {
      __m128i avg;
             b = load4(prev);
      a = d; d = load4(row );

       
      avg = _mm_avg_epu8(a,b);
       
      avg = _mm_sub_epi8(avg, _mm_and_si128(_mm_xor_si128(a,b),
                                            _mm_set1_epi8(1)));

      d = _mm_add_epi8(d, avg);
      store4(row, d);

      prev += 4;
      row  += 4;
      rb   -= 4;
   }
}

 
static __m128i abs_i16(__m128i x) {
#if PNG_INTEL_SSE_IMPLEMENTATION >= 2
   return _mm_abs_epi16(x);
#else
                                            
                                                                  
      
   __m128i is_negative = _mm_cmplt_epi16(x, _mm_setzero_si128());

    
   x = _mm_xor_si128(x, is_negative);

    
   x = _mm_sub_epi16(x, is_negative);
   return x;
#endif
}

 
static __m128i if_then_else(__m128i c, __m128i t, __m128i e) {
#if PNG_INTEL_SSE_IMPLEMENTATION >= 3
   return _mm_blendv_epi8(e,t,c);
#else
   return _mm_or_si128(_mm_and_si128(c, t), _mm_andnot_si128(c, e));
#endif
}

void png_read_filter_row_paeth3_sse2(png_row_infop row_info, png_bytep row,
   png_const_bytep prev)
{
                                                                          
                                                    
                 
                 
                                                                               
              
     
                                                                           
                                                                             
              
                                                                             
               
      
   size_t rb;
   const __m128i zero = _mm_setzero_si128();
   __m128i c, b = zero,
           a, d = zero;

   png_debug(1, "in png_read_filter_row_paeth3_sse2");

   rb = row_info->rowbytes;
   while (rb >= 4) {
                                                                              
                       
         
      __m128i pa,pb,pc,smallest,nearest;
      c = b; b = _mm_unpacklo_epi8(load4(prev), zero);
      a = d; d = _mm_unpacklo_epi8(load4(row ), zero);

       
   
      pa = _mm_sub_epi16(b,c);

       
      pb = _mm_sub_epi16(a,c);

       
      pc = _mm_add_epi16(pa,pb);

      pa = abs_i16(pa);   
      pb = abs_i16(pb);   
      pc = abs_i16(pc);   

      smallest = _mm_min_epi16(pc, _mm_min_epi16(pa, pb));

       
      nearest  = if_then_else(_mm_cmpeq_epi16(smallest, pa), a,
                 if_then_else(_mm_cmpeq_epi16(smallest, pb), b,
                                                             c));

       
      d = _mm_add_epi8(d, nearest);
      store3(row, _mm_packus_epi16(d,d));

      prev += 3;
      row  += 3;
      rb   -= 3;
   }
   if (rb > 0) {
                                                                              
                       
         
      __m128i pa,pb,pc,smallest,nearest;
      c = b; b = _mm_unpacklo_epi8(load3(prev), zero);
      a = d; d = _mm_unpacklo_epi8(load3(row ), zero);

       
      pa = _mm_sub_epi16(b,c);

       
      pb = _mm_sub_epi16(a,c);

       
      pc = _mm_add_epi16(pa,pb);

      pa = abs_i16(pa);   
      pb = abs_i16(pb);   
      pc = abs_i16(pc);   

      smallest = _mm_min_epi16(pc, _mm_min_epi16(pa, pb));

       
      nearest  = if_then_else(_mm_cmpeq_epi16(smallest, pa), a,
                         if_then_else(_mm_cmpeq_epi16(smallest, pb), b,
                                                                     c));

       
      d = _mm_add_epi8(d, nearest);
      store3(row, _mm_packus_epi16(d,d));

      prev += 3;
      row  += 3;
      rb   -= 3;
   }
}

void png_read_filter_row_paeth4_sse2(png_row_infop row_info, png_bytep row,
   png_const_bytep prev)
{
                                                                          
                                                    
                 
                 
                                                                               
              
     
                                                                           
                                                                             
              
                                                                             
               
      
   size_t rb;
   const __m128i zero = _mm_setzero_si128();
   __m128i pa,pb,pc,smallest,nearest;
   __m128i c, b = zero,
           a, d = zero;

   png_debug(1, "in png_read_filter_row_paeth4_sse2");

   rb = row_info->rowbytes+4;
   while (rb > 4) {
                                                                              
                       
         
      c = b; b = _mm_unpacklo_epi8(load4(prev), zero);
      a = d; d = _mm_unpacklo_epi8(load4(row ), zero);

       
      pa = _mm_sub_epi16(b,c);

       
      pb = _mm_sub_epi16(a,c);

       
      pc = _mm_add_epi16(pa,pb);

      pa = abs_i16(pa);   
      pb = abs_i16(pb);   
      pc = abs_i16(pc);   

      smallest = _mm_min_epi16(pc, _mm_min_epi16(pa, pb));

       
      nearest  = if_then_else(_mm_cmpeq_epi16(smallest, pa), a,
                         if_then_else(_mm_cmpeq_epi16(smallest, pb), b,
                                                                     c));

       
      d = _mm_add_epi8(d, nearest);
      store4(row, _mm_packus_epi16(d,d));

      prev += 4;
      row  += 4;
      rb   -= 4;
   }
}

#endif  
#endif  
