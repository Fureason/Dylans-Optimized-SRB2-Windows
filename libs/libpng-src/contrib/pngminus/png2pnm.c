/*
 *  png2pnm.c --- conversion from PNG-file to PGM/PPM-file
 *  copyright (C) 1999-2019 by Willem van Schaik <willem at schaik dot com>
 *
 *  This software is released under the MIT license. For conditions of
 *  distribution and use, see the LICENSE file part of this package.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#ifndef BOOL
#define BOOL unsigned char
#endif
#ifndef TRUE
#define TRUE (BOOL) 1
#endif
#ifndef FALSE
#define FALSE (BOOL) 0
#endif

 
#ifndef PNG_DEBUG
#define PNG_DEBUG 0
#endif

#include "png.h"

 

int main (int argc, char *argv[]);
void usage ();
BOOL png2pnm (FILE *png_file, FILE *pnm_file, FILE *alpha_file,
              BOOL raw, BOOL alpha);

  
        
   

int main (int argc, char *argv[])
{
  FILE *fp_rd = stdin;
  FILE *fp_wr = stdout;
  FILE *fp_al = NULL;
  BOOL raw = TRUE;
  BOOL alpha = FALSE;
  int argi;

  for (argi = 1; argi < argc; argi++)
  {
    if (argv[argi][0] == '-')
    {
      switch (argv[argi][1])
      {
        case 'n':
          raw = FALSE;
          break;
        case 'r':
          raw = TRUE;
          break;
        case 'a':
          alpha = TRUE;
          argi++;
          if ((fp_al = fopen (argv[argi], "wb")) == NULL)
          {
            fprintf (stderr, "PNM2PNG\n");
            fprintf (stderr, "Error:  cannot create alpha-channel file %s\n",
                     argv[argi]);
            exit (1);
          }
          break;
        case 'h':
        case '?':
          usage ();
          exit (0);
          break;
        default:
          fprintf (stderr, "PNG2PNM\n");
          fprintf (stderr, "Error:  unknown option %s\n", argv[argi]);
          usage ();
          exit (1);
          break;
      }  
    }
    else if (fp_rd == stdin)
    {
      if ((fp_rd = fopen (argv[argi], "rb")) == NULL)
      {
        fprintf (stderr, "PNG2PNM\n");
        fprintf (stderr, "Error:  file %s does not exist\n", argv[argi]);
        exit (1);
      }
    }
    else if (fp_wr == stdout)
    {
      if ((fp_wr = fopen (argv[argi], "wb")) == NULL)
      {
        fprintf (stderr, "PNG2PNM\n");
        fprintf (stderr, "Error:  cannot create file %s\n", argv[argi]);
        exit (1);
      }
    }
    else
    {
      fprintf (stderr, "PNG2PNM\n");
      fprintf (stderr, "Error:  too many parameters\n");
      usage ();
      exit (1);
    }
  }  

#if defined(O_BINARY) && (O_BINARY != 0)
   
  if (fp_rd == stdin)
    setmode (fileno (stdin), O_BINARY);
  if ((raw) && (fp_wr == stdout))
    setmode (fileno (stdout), O_BINARY);
#endif

   
  if (png2pnm (fp_rd, fp_wr, fp_al, raw, alpha) == FALSE)
  {
    fprintf (stderr, "PNG2PNM\n");
    fprintf (stderr, "Error:  unsuccessful conversion of PNG-image\n");
    exit (1);
  }

   
  fclose (fp_rd);
   
  fclose (fp_wr);
   
  if (alpha)
    fclose (fp_al);

  return 0;
}

  
         
   

void usage ()
{
  fprintf (stderr, "PNG2PNM\n");
  fprintf (stderr, "   by Willem van Schaik, 1999\n");
  fprintf (stderr, "Usage:  png2pnm [options] <file>.png [<file>.pnm]\n");
  fprintf (stderr, "   or:  ... | png2pnm [options]\n");
  fprintf (stderr, "Options:\n");
  fprintf (stderr,
      "   -r[aw]   write pnm-file in binary format (P4/P5/P6) (default)\n");
  fprintf (stderr, "   -n[oraw] write pnm-file in ascii format (P1/P2/P3)\n");
  fprintf (stderr,
      "   -a[lpha] <file>.pgm write PNG alpha channel as pgm-file\n");
  fprintf (stderr, "   -h | -?  print this help-information\n");
}

  
           
   

BOOL png2pnm (FILE *png_file, FILE *pnm_file, FILE *alpha_file,
              BOOL raw, BOOL alpha)
{
  png_struct    *png_ptr = NULL;
  png_info      *info_ptr = NULL;
  png_byte      buf[8];
  png_byte      *png_pixels = NULL;
  png_byte      **row_pointers = NULL;
  png_byte      *pix_ptr = NULL;
  png_uint_32   row_bytes;

  png_uint_32   width;
  png_uint_32   height;
  int           bit_depth;
  int           channels;
  int           color_type;
  int           alpha_present;
  int           row, col;
  int           ret;
  int           i;
  long          dep_16;

   
  ret = fread (buf, 1, 8, png_file);
  if (ret != 8)
    return FALSE;

  ret = png_sig_cmp (buf, 0, 8);
  if (ret != 0)
    return FALSE;

   

  png_ptr = png_create_read_struct (png_get_libpng_ver(NULL),
                                    NULL, NULL, NULL);
  if (!png_ptr)
    return FALSE;  

  info_ptr = png_create_info_struct (png_ptr);
  if (!info_ptr)
  {
    png_destroy_read_struct (&png_ptr, NULL, NULL);
    return FALSE;  
  }

  if (setjmp (png_jmpbuf (png_ptr)))
  {
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    return FALSE;
  }

   
  png_init_io (png_ptr, png_file);
  png_set_sig_bytes (png_ptr, 8);  

   
  png_read_info (png_ptr, info_ptr);

   
  png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                NULL, NULL, NULL);

   

   
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_expand (png_ptr);
   
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand (png_ptr);
   
  if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_expand (png_ptr);

#ifdef NJET
   
  if (bit_depth == 16)
    png_set_strip_16 (png_ptr);
   
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb (png_ptr);
   
  if (png_get_gAMA (png_ptr, info_ptr, &file_gamma))
    png_set_gamma (png_ptr, (double) 2.2, file_gamma);
#endif

                                                                        
                                                            

  png_read_update_info (png_ptr, info_ptr);

   
  png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                NULL, NULL, NULL);

   
  if (bit_depth == 16)
  {
    raw = FALSE;
#if defined(O_BINARY) && (O_BINARY != 0)
    setmode (fileno (pnm_file), O_BINARY);
#endif
  }

   
  if (color_type == PNG_COLOR_TYPE_GRAY)
    channels = 1;
  else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    channels = 2;
  else if (color_type == PNG_COLOR_TYPE_RGB)
    channels = 3;
  else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    channels = 4;
  else
    channels = 0;  
  alpha_present = (channels - 1) % 2;

   
  if (alpha && !alpha_present)
  {
    fprintf (stderr, "PNG2PNM\n");
    fprintf (stderr, "Error:  PNG-file doesn't contain alpha channel\n");
    exit (1);
  }

   
  row_bytes = png_get_rowbytes (png_ptr, info_ptr);

  if ((row_bytes == 0) ||
      ((size_t) height > (size_t) (-1) / (size_t) row_bytes))
  {
     
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    return FALSE;
  }
  if ((png_pixels = (png_byte *)
       malloc ((size_t) row_bytes * (size_t) height)) == NULL)
  {
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    return FALSE;
  }

  if ((row_pointers = (png_byte **)
       malloc ((size_t) height * sizeof (png_byte *))) == NULL)
  {
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    free (png_pixels);
    return FALSE;
  }

   
  for (i = 0; i < ((int) height); i++)
    row_pointers[i] = png_pixels + i * row_bytes;

   
  png_read_image (png_ptr, row_pointers);

   
  png_read_end (png_ptr, info_ptr);

   
  png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

   

  if ((color_type == PNG_COLOR_TYPE_GRAY) ||
      (color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
  {
    fprintf (pnm_file, "%s\n", (raw) ? "P5" : "P2");
    fprintf (pnm_file, "%d %d\n", (int) width, (int) height);
    fprintf (pnm_file, "%ld\n", ((1L << (int) bit_depth) - 1L));
  }
  else if ((color_type == PNG_COLOR_TYPE_RGB) ||
           (color_type == PNG_COLOR_TYPE_RGB_ALPHA))
  {
    fprintf (pnm_file, "%s\n", (raw) ? "P6" : "P3");
    fprintf (pnm_file, "%d %d\n", (int) width, (int) height);
    fprintf (pnm_file, "%ld\n", ((1L << (int) bit_depth) - 1L));
  }

   

  if ((alpha) &&
      ((color_type == PNG_COLOR_TYPE_GRAY_ALPHA) ||
       (color_type == PNG_COLOR_TYPE_RGB_ALPHA)))
  {
    fprintf (alpha_file, "%s\n", (raw) ? "P5" : "P2");
    fprintf (alpha_file, "%d %d\n", (int) width, (int) height);
    fprintf (alpha_file, "%ld\n", ((1L << (int) bit_depth) - 1L));
  }

   
  pix_ptr = png_pixels;

  for (row = 0; row < (int) height; row++)
  {
    for (col = 0; col < (int) width; col++)
    {
      for (i = 0; i < (channels - alpha_present); i++)
      {
        if (raw)
        {
          fputc ((int) *pix_ptr++, pnm_file);
        }
        else
        {
          if (bit_depth == 16)
          {
            dep_16 = (long) *pix_ptr++;
            fprintf (pnm_file, "%ld ", (dep_16 << 8) + ((long) *pix_ptr++));
          }
          else
          {
            fprintf (pnm_file, "%ld ", (long) *pix_ptr++);
          }
        }
      }
      if (alpha_present)
      {
        if (!alpha)
        {
          pix_ptr++;  
          if (bit_depth == 16)
            pix_ptr++;
        }
        else  
        {
          if (raw)
          {
            fputc ((int) *pix_ptr++, alpha_file);
          }
          else
          {
            if (bit_depth == 16)
            {
              dep_16 = (long) *pix_ptr++;
              fprintf (alpha_file, "%ld ", (dep_16 << 8) + (long) *pix_ptr++);
            }
            else
            {
              fprintf (alpha_file, "%ld ", (long) *pix_ptr++);
            }
          }
        }
      }  

      if (!raw)
        if (col % 4 == 3)
          fprintf (pnm_file, "\n");
    }  

    if (!raw)
      if (col % 4 != 0)
        fprintf (pnm_file, "\n");
  }  

  if (row_pointers != NULL)
    free (row_pointers);
  if (png_pixels != NULL)
    free (png_pixels);

  return TRUE;

}  
