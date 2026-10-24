/* Fake a PNG - just write it out directly.
 *
 * COPYRIGHT: Written by John Cunningham Bowler, 2014.
 * To the extent possible under law, the author has waived all copyright and
 * related or neighboring rights to this work.  This work is published from:
 * United States.
 *
 */

#include <stdio.h>
#include <zlib.h>  

void
put_uLong(uLong val)
{
   putchar(val >> 24);
   putchar(val >> 16);
   putchar(val >>  8);
   putchar(val >>  0);
}

void
put_chunk(const unsigned char *chunk, uInt length)
{
   uLong crc;

   put_uLong(length-4);  

   fwrite(chunk, length, 1, stdout);

   crc = crc32(0, Z_NULL, 0);
   put_uLong(crc32(crc, chunk, length));
}

const unsigned char signature[] =
{
   137, 80, 78, 71, 13, 10, 26, 10
};

const unsigned char IHDR[] =
{
   73, 72, 68, 82,  
   0, 0, 0, 1,  
   0, 0, 0, 1,  
   1,  
   0,  
   0,  
   0,  
   0   
};

const unsigned char unknown[] =
{
   'u', 'n', 'K', 'n'  
};

int
main(void)
{
   fwrite(signature, sizeof signature, 1, stdout);
   put_chunk(IHDR, sizeof IHDR);

   for (;;)
      put_chunk(unknown, sizeof unknown);
}
