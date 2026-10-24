/* contrib/arm-neon/linux.c
 *
 * Last changed in libpng 1.6.31 [July 27, 2017]
 * Copyright (c) 2014, 2017 Glenn Randers-Pehrson
 * Written by John Bowler, 2014, 2017.
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * SEE contrib/arm-neon/README before reporting bugs
 *
 * STATUS: SUPPORTED
 * BUG REPORTS: png-mng-implement@sourceforge.net
 *
 * png_have_neon implemented for Linux by reading the widely available
 * pseudo-file /proc/cpuinfo.
 *
 * This code is strict ANSI-C and is probably moderately portable; it does
 * however use <stdio.h> and it assumes that /proc/cpuinfo is never localized.
 */
#include <stdio.h>

static int
png_have_neon(png_structp png_ptr)
{
   FILE *f = fopen("/proc/cpuinfo", "rb");

   if (f != NULL)
   {
                                                                                
                                                                                
         
      static const char ch_feature[] = { 70, 69, 65, 84, 85, 82, 69, 83 };
      static const char ch_neon[] = { 78, 69, 79, 78 };

      enum
      {
         StartLine, Feature, Colon, StartTag, Neon, HaveNeon, SkipTag, SkipLine
      }  state;
      int counter;

      for (state=StartLine, counter=0;;)
      {
         int ch = fgetc(f);

         if (ch == EOF)
         {
                                                                           
                                       
               
            fclose(f);
            return 0;
         }

         switch (state)
         {
            case StartLine:
                
               if (ch <= 32)  
                  break;

               counter=0;
               state = Feature;
                

            case Feature:
                
               if ((ch & ~0x20) == ch_feature[counter])
               {
                  if (++counter == (sizeof ch_feature))
                     state = Colon;
                  break;
               }

                
               state = SkipLine;
                

            case SkipLine:
            skipLine:
                
               if (ch != 10 && ch != 13)
                  break;

               state = StartLine;
               break;

            case Colon:
                
               if (ch == 32 || ch == 9)
                  break;

               if (ch == 58)  
               {
                  state = StartTag;
                  break;
               }

                                                                            
                                   
                  
               state = SkipLine;
               goto skipLine;

            case StartTag:
                
               if (ch == 32 || ch == 9)
                  break;

               state = Neon;
               counter = 0;
                

            case Neon:
                
               if ((ch & ~0x20) == ch_neon[counter])
               {
                  if (++counter == (sizeof ch_neon))
                     state = HaveNeon;
                  break;
               }

               state = SkipTag;
                

            case SkipTag:
                
               if (ch == 10 || ch == 13)
                  state = StartLine;

               else if (ch == 32 || ch == 9)
                  state = StartTag;
               break;

            case HaveNeon:
                                                                             
                                                 
                  
               if (ch == 10 || ch == 13 || ch == 32 || ch == 9)
               {
                  fclose(f);
                  return 1;
               }

               state = SkipTag;
               break;

            default:
               png_error(png_ptr, "png_have_neon: internal error (bug)");
         }
      }
   }

#ifdef PNG_WARNINGS_SUPPORTED
   else
      png_warning(png_ptr, "/proc/cpuinfo open failed");
#endif

   return 0;
}
