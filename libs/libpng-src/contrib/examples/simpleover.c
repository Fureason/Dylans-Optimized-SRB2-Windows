/*- simpleover
 *
 * COPYRIGHT: Written by John Cunningham Bowler, 2015.
 * To the extent possible under law, the author has waived all copyright and
 * related or neighboring rights to this work.  This work is published from:
 * United States.
 *
 * Read several PNG files, which should have an alpha channel or transparency
 * information, and composite them together to produce one or more 16-bit linear
 * RGBA intermediates.  This involves doing the correct 'over' composition to
 * combine the alpha channels and corresponding data.
 *
 * Finally read an output (background) PNG using the 24-bit RGB format (the
 * PNG will be composited on green (#00ff00) by default if it has an alpha
 * channel), and apply the intermediate image generated above to specified
 * locations in the image.
 *
 * The command line has the general format:
 *
 *    simpleover <background.png> [output.png]
 *        {--sprite=width,height,name {[--at=x,y] {sprite.png}}}
 *        {--add=name {x,y}}
 *
 * The --sprite and --add options may occur multiple times. They are executed
 * in order.  --add may refer to any sprite already read.
 *
 * This code is intended to show how to composite multiple images together
 * correctly.  Apart from the libpng Simplified API the only work done in here
 * is to combine multiple input PNG images into a single sprite; this involves
 * a Porter-Duff 'over' operation and the input PNG images may, as a result,
 * be regarded as being layered one on top of the other with the first (leftmost
 * on the command line) being at the bottom and the last on the top.
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

                                                                             
                                                                            
                                                                         
                          
   
#include "../../png.h"

#ifdef PNG_SIMPLIFIED_READ_SUPPORTED

#define sprite_name_chars 15
struct sprite {
   FILE         *file;
   png_uint_16p  buffer;
   unsigned int  width;
   unsigned int  height;
   char          name[sprite_name_chars+1];
};

#if 0  
#include <math.h>
#include <stdio.h>

int main(void) {
   double err = 0;
   unsigned int xerr = 0;
   unsigned int r = 32769;
   {
      unsigned int x = 0;

      do {
         unsigned int t = x + (x >> 16)   + r;
         double v = x, errtest;

         if (t < x) {
            fprintf(stderr, "overflow: %u+%u -> %u\n", x, r, t);
            return 1;
         }

         v /= 65535;
         errtest = v;
         t >>= 16;
         errtest -= t;

         if (errtest > err) {
            err = errtest;
            xerr = x;

            if (errtest >= .5) {
               fprintf(stderr, "error: %u/65535 = %f, not %u, error %f\n",
                     x, v, t, errtest);
               return 0;
            }
         }
      } while (++x <= 65535U*65535U);
   }

   printf("error %f @ %u\n", err, xerr);

   return 0;
}
#endif  

static void
sprite_op(const struct sprite *sprite, int x_offset, int y_offset,
   png_imagep image, const png_uint_16 *buffer)
{
                                                                             
                                                                           
                                                    
      


                                                                             
                                    
      
   if ((y_offset < 0 || (unsigned) y_offset < sprite->height) &&
       (x_offset < 0 || (unsigned) x_offset < sprite->width))
   {
      unsigned int y = 0;

      if (y_offset < 0)
         y = -y_offset;  

      do
      {
         unsigned int x = 0;

         if (x_offset < 0)
            x = -x_offset;

         do
         {
             
            const png_uint_16 *in_pixel = buffer + (y * image->width + x)*4;
            png_uint_32 in_alpha = in_pixel[3];

                                                                           
                                                          
               
            if (in_alpha > 0)
            {
               png_uint_16 *out_pixel = sprite->buffer +
                  ((y+y_offset) * sprite->width + (x+x_offset))*4;

                
               in_alpha = 65535-in_alpha;

               if (in_alpha > 0)
               {
                                                                           
                                                                              
                                                                             
                                                                       
                    
                                                       
                    
                                                                            
                                                                               
                                                                              
                     
                  png_uint_32 tmp;

#                 define compose(c)\
                     tmp = out_pixel[c] * in_alpha;\
                     tmp = (tmp + (tmp >> 16) + 32769) >> 16;\
                     out_pixel[c] = tmp + in_pixel[c]

                   
                  compose(0);
                  compose(1);
                  compose(2);
                  compose(3);
               }

               else
                  out_pixel[0] = in_pixel[0],
                  out_pixel[1] = in_pixel[1],
                  out_pixel[2] = in_pixel[2],
                  out_pixel[3] = in_pixel[3];
            }
         }
         while (++x < image->width);
      }
      while (++y < image->height);
   }
}

static int
create_sprite(struct sprite *sprite, int *argc, const char ***argv)
{
                                                                              
                                                                            
                                                                           
                                                                               
      
   while (*argc > 0)
   {
      char tombstone;
      int x = 0, y = 0;

      if ((*argv)[0][0] == '-' && (*argv)[0][1] == '-')
      {
          
         if (sscanf((*argv)[0], "--at=%d,%d%c", &x, &y, &tombstone) != 2)
            break;  

         ++*argv, --*argc;
      }

      else
      {
          
         png_image image;

         image.version = PNG_IMAGE_VERSION;
         image.opaque = NULL;

         if (png_image_begin_read_from_file(&image, (*argv)[0]))
         {
            png_uint_16p buffer;

            image.format = PNG_FORMAT_LINEAR_RGB_ALPHA;

            buffer = malloc(PNG_IMAGE_SIZE(image));

            if (buffer != NULL)
            {
               if (png_image_finish_read(&image, NULL , buffer,
                  0 ,
                  NULL ))
               {
                                                                            
                                                                       
                                                                         
                                                                       
                                               
                     
                  sprite_op(sprite, x, y, &image, buffer);
                  free(buffer);
                  ++*argv, --*argc;
                   
                  continue;
               }

               else
               {
                  free(buffer);
                  fprintf(stderr, "simpleover: read %s: %s\n", (*argv)[0],
                      image.message);
               }
            }

            else
            {
               fprintf(stderr, "simpleover: out of memory: %lu bytes\n",
                  (unsigned long)PNG_IMAGE_SIZE(image));

                                                                              
                                                                               
                                                                         
                  
               png_image_free(&image);
            }
         }

         else
         {
             
            fprintf(stderr, "simpleover: %s: %s\n", (*argv)[0], image.message);
         }

         return 0;  
      }
   }

                                                                          
                                                     
      
   sprite->file = tmpfile();

   if (sprite->file != NULL)
   {
      png_image save;

      memset(&save, 0, sizeof save);
      save.version = PNG_IMAGE_VERSION;
      save.opaque = NULL;
      save.width = sprite->width;
      save.height = sprite->height;
      save.format = PNG_FORMAT_LINEAR_RGB_ALPHA;
      save.flags = PNG_IMAGE_FLAG_FAST;
      save.colormap_entries = 0;

      if (png_image_write_to_stdio(&save, sprite->file, 1 ,
          sprite->buffer, 0 , NULL ))
      {
          
         free(sprite->buffer);
         sprite->buffer = NULL;
         return 1;  
      }

      else
         fprintf(stderr, "simpleover: write sprite %s: %s\n", sprite->name,
            save.message);
   }

   else
      fprintf(stderr, "simpleover: sprite %s: could not allocate tmpfile: %s\n",
         sprite->name, strerror(errno));

   return 0;  
}

static int
add_sprite(png_imagep output, png_bytep out_buf, struct sprite *sprite,
   int *argc, const char ***argv)
{
                                                                              
                                                                              
                                                                      
             
      
   while (*argc > 0)
   {
      char tombstone;
      int x, y;

      if ((*argv)[0][0] == '-' && (*argv)[0][1] == '-')
         return 1;  

      if (sscanf((*argv)[0], "%d,%d%c", &x, &y, &tombstone) == 2)
      {
                                                                      
                     
            
         if (x < 0 || y < 0 ||
             (unsigned) x >= output->width ||
             (unsigned) y >= output->height ||
             sprite->width > output->width-x ||
             sprite->height > output->height-y)
         {
            fprintf(stderr, "simpleover: sprite %s @ (%d,%d) outside image\n",
               sprite->name, x, y);
             
            return 0;  
         }

         else
         {
                                                                         
                                               
               
            png_image in;

            in.version = PNG_IMAGE_VERSION;
            rewind(sprite->file);

            if (png_image_begin_read_from_stdio(&in, sprite->file))
            {
               in.format = PNG_FORMAT_RGB;  

               if (png_image_finish_read(&in, NULL ,
                  out_buf + (y*output->width + x)*3 ,
                  output->width*3 ,
                  NULL ))
               {
                  ++*argv, --*argc;
                  continue;
               }
            }

             
            fprintf(stderr, "simpleover: add sprite %s: %s\n", sprite->name,
                in.message);
            return 0;  
         }
      }

      else
      {
         fprintf(stderr, "simpleover: --add='%s': invalid position %s\n",
               sprite->name, (*argv)[0]);
         return 0;  
      }
   }

   return 1;  
}

static int
simpleover_process(png_imagep output, png_bytep out_buf, int argc,
   const char **argv)
{
   int result = 1;  
#  define csprites 10 
#  define str(a) #a
   int nsprites = 0;
   struct sprite sprites[csprites];

   while (argc > 0)
   {
      result = 0;  

      if (strncmp(argv[0], "--sprite=", 9) == 0)
      {
         char tombstone;

         if (nsprites < csprites)
         {
            int n;

            sprites[nsprites].width = sprites[nsprites].height = 0;
            sprites[nsprites].name[0] = 0;

            n = sscanf(argv[0], "--sprite=%u,%u,%" str(sprite_name_chars) "s%c",
                &sprites[nsprites].width, &sprites[nsprites].height,
                sprites[nsprites].name, &tombstone);

            if ((n == 2 || n == 3) &&
                sprites[nsprites].width > 0 && sprites[nsprites].height > 0)
            {
               size_t buf_size, tmp;

                
               if (sprites[nsprites].name[0] == 0)
                  sprintf(sprites[nsprites].name, "sprite-%d", nsprites+1);

                                                                           
                       
                  
               buf_size = sizeof (png_uint_16 [4]);
               buf_size *= sprites[nsprites].width;
               buf_size *= sprites[nsprites].height;

                
               tmp = buf_size;
               tmp /= sprites[nsprites].width;
               tmp /= sprites[nsprites].height;

               if (tmp == sizeof (png_uint_16 [4]))
               {
                  sprites[nsprites].buffer = malloc(buf_size);
                   
                  memset(sprites[nsprites].buffer, 0, buf_size);

                  if (sprites[nsprites].buffer != NULL)
                  {
                     sprites[nsprites].file = NULL;
                     ++argv, --argc;

                     if (create_sprite(sprites+nsprites++, &argc, &argv))
                     {
                        result = 1;  
                        continue;
                     }

                     break;  
                  }
               }

                
               fprintf(stderr, "simpleover: %s: sprite too large\n", argv[0]);
               break;
            }

            else
            {
               fprintf(stderr, "simpleover: %s: invalid sprite (%u,%u)\n",
                  argv[0], sprites[nsprites].width, sprites[nsprites].height);
               break;
            }
         }

         else
         {
            fprintf(stderr, "simpleover: %s: too many sprites\n", argv[0]);
            break;
         }
      }

      else if (strncmp(argv[0], "--add=", 6) == 0)
      {
         const char *name = argv[0]+6;
         int isprite = nsprites;

         ++argv, --argc;

         while (--isprite >= 0)
         {
            if (strcmp(sprites[isprite].name, name) == 0)
            {
               if (!add_sprite(output, out_buf, sprites+isprite, &argc, &argv))
                  goto out;  

               break;
            }
         }

         if (isprite < 0)  
         {
            fprintf(stderr, "simpleover: --add='%s': sprite not found\n", name);
            break;
         }
      }

      else
      {
         fprintf(stderr, "simpleover: %s: unrecognized operation\n", argv[0]);
         break;
      }

      result = 1;  
   }

    
out:
   while (--nsprites >= 0)
   {
      if (sprites[nsprites].buffer != NULL)
         free(sprites[nsprites].buffer);

      if (sprites[nsprites].file != NULL)
         (void)fclose(sprites[nsprites].file);
   }

   return result;
}

int main(int argc, const char **argv)
{
   int result = 1;  

   if (argc >= 2)
   {
      int argi = 2;
      const char *output = NULL;
      png_image image;

      if (argc > 2 && argv[2][0] != '-' )
      {
         output = argv[2];
         argi = 3;
      }

      image.version = PNG_IMAGE_VERSION;
      image.opaque = NULL;

      if (png_image_begin_read_from_file(&image, argv[1]))
      {
         png_bytep buffer;

         image.format = PNG_FORMAT_RGB;  

         buffer = malloc(PNG_IMAGE_SIZE(image));

         if (buffer != NULL)
         {
            png_color background = {0, 0xff, 0};  

            if (png_image_finish_read(&image, &background, buffer,
               0 , NULL ))
            {
                                                                        
                                                                              
                        
                 
                                                   
                  
               if (simpleover_process(&image, buffer, argc-argi, argv+argi))
               {
                   
                  if ((output != NULL &&
                       png_image_write_to_file(&image, output,
                        0 , buffer, 0 ,
                        NULL )) ||
                      (output == NULL &&
                       png_image_write_to_stdio(&image, stdout,
                        0 , buffer, 0 ,
                        NULL )))
                     result = 0;

                  else
                     fprintf(stderr, "simpleover: write %s: %s\n",
                        output == NULL ? "stdout" : output, image.message);
               }

                
            }

            else
               fprintf(stderr, "simpleover: read %s: %s\n", argv[1],
                   image.message);

            free(buffer);
         }

         else
         {
            fprintf(stderr, "simpleover: out of memory: %lu bytes\n",
               (unsigned long)PNG_IMAGE_SIZE(image));

                                                                             
                                                                             
                                                                  
               
            png_image_free(&image);
         }
      }

      else
      {
          
         fprintf(stderr, "simpleover: %s: %s\n", argv[1], image.message);
      }
   }

   else
   {
       
      fprintf(stderr,
         "simpleover: usage: simpleover background.png [output.png]\n"
         "  Output 'background.png' as a 24-bit RGB PNG file in 'output.png'\n"
         "   or, if not given, stdout.  'background.png' will be composited\n"
         "   on fully saturated green.\n"
         "\n"
         "  Optionally, before output, process additional PNG files:\n"
         "\n"
         "   --sprite=width,height,name {[--at=x,y] {sprite.png}}\n"
         "    Produce a transparent sprite of size (width,height) and with\n"
         "     name 'name'.\n"
         "    For each sprite.png composite it using a Porter-Duff 'Over'\n"
         "     operation at offset (x,y) in the sprite (defaulting to (0,0)).\n"
         "     Input PNGs will be truncated to the area of the sprite.\n"
         "\n"
         "   --add='name' {x,y}\n"
         "    Optionally, before output, composite a sprite, 'name', which\n"
         "     must have been previously produced using --sprite, at each\n"
         "     offset (x,y) in the output image.  Each sprite must fit\n"
         "     completely within the output image.\n"
         "\n"
         "  PNG files are processed in the order they occur on the command\n"
         "  line and thus the first PNG processed appears as the bottommost\n"
         "  in the output image.\n");
   }

   return result;
}
#endif  
