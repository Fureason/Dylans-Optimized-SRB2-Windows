/* gzjoin -- command to join gzip files into one gzip file

  Copyright (C) 2004, 2005, 2012 Mark Adler, all rights reserved
  version 1.2, 14 Aug 2012

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler    madler@alumni.caltech.edu
 */

  
                  
  
                                       
                                                                 
                                                    
   

  
                                                                           
                                                                    
                                                                              
                                                                             
                                                                             
                                                                               
                                                            

                                                                           
                                                                            
                                                

                                                                              
                                                                            
                                                                         
                                                                             

                                                                          
                                                                            
                                       
   

#include <stdio.h>       
#include <stdlib.h>      
#include <fcntl.h>       
#include <unistd.h>      
#include "zlib.h"
     

#define local static

 
local int bail(char *why1, char *why2)
{
    fprintf(stderr, "gzjoin error: %s%s, output incomplete\n", why1, why2);
    exit(1);
    return 0;
}

 

#define CHUNK 32768          

 
typedef struct {
    char *name;              
    int fd;                  
    unsigned left;           
    unsigned char *next;     
    unsigned char *buf;      
} bin;

 
local void bclose(bin *in)
{
    if (in != NULL) {
        if (in->fd != -1)
            close(in->fd);
        if (in->buf != NULL)
            free(in->buf);
        free(in);
    }
}

                                                                           
             
local bin *bopen(char *name)
{
    bin *in;

    in = malloc(sizeof(bin));
    if (in == NULL)
        return NULL;
    in->buf = malloc(CHUNK);
    in->fd = open(name, O_RDONLY, 0);
    if (in->buf == NULL || in->fd == -1) {
        bclose(in);
        return NULL;
    }
    in->left = 0;
    in->next = in->buf;
    in->name = name;
    return in;
}

                                                                          
                                               
local int bload(bin *in)
{
    long len;

    if (in == NULL)
        return -1;
    if (in->left != 0)
        return 0;
    in->next = in->buf;
    do {
        len = (long)read(in->fd, in->buf + in->left, CHUNK - in->left);
        if (len < 0)
            return -1;
        in->left += (unsigned)len;
    } while (len != 0 && in->left < CHUNK);
    return len == 0 ? 1 : 0;
}

 
#define bget(in) (in->left ? 0 : bload(in), \
                  in->left ? (in->left--, *(in->next)++) : \
                    bail("unexpected end of file on ", in->name))

 
local unsigned long bget4(bin *in)
{
    unsigned long val;

    val = bget(in);
    val += (unsigned long)(bget(in)) << 8;
    val += (unsigned long)(bget(in)) << 16;
    val += (unsigned long)(bget(in)) << 24;
    return val;
}

 
local void bskip(bin *in, unsigned skip)
{
     
    if (in == NULL)
        return;

     
    if (skip <= in->left) {
        in->left -= skip;
        in->next += skip;
        return;
    }

     
    skip -= in->left;
    in->left = 0;

     
    if (skip > CHUNK) {
        unsigned left;

        left = skip & (CHUNK - 1);
        if (left == 0) {
                                                                               
                                             
            lseek(in->fd, skip - 1, SEEK_CUR);
            if (read(in->fd, in->buf, 1) != 1)
                bail("unexpected end of file on ", in->name);
            return;
        }

         
        lseek(in->fd, skip - left, SEEK_CUR);
        skip = left;
    }

     
    bload(in);
    if (skip > in->left)
        bail("unexpected end of file on ", in->name);
    in->left -= skip;
    in->next += skip;
}

 

 
local void gzhead(bin *in)
{
    int flags;

     
    if (bget(in) != 0x1f || bget(in) != 0x8b || bget(in) != 8)
        bail(in->name, " is not a valid gzip file");

     
    flags = bget(in);
    if ((flags & 0xe0) != 0)
        bail("unknown reserved bits set in ", in->name);

     
    bskip(in, 6);

     
    if (flags & 4) {
        unsigned len;

        len = bget(in);
        len += (unsigned)(bget(in)) << 8;
        bskip(in, len);
    }

     
    if (flags & 8)
        while (bget(in) != 0)
            ;

     
    if (flags & 16)
        while (bget(in) != 0)
            ;

     
    if (flags & 2)
        bskip(in, 2);
}

 
local void put4(unsigned long val, FILE *out)
{
    putc(val & 0xff, out);
    putc((val >> 8) & 0xff, out);
    putc((val >> 16) & 0xff, out);
    putc((val >> 24) & 0xff, out);
}

 
local void zpull(z_streamp strm, bin *in)
{
    if (in->left == 0)
        bload(in);
    if (in->left == 0)
        bail("unexpected end of file on ", in->name);
    strm->avail_in = in->left;
    strm->next_in = in->next;
}

 
local void gzinit(unsigned long *crc, unsigned long *tot, FILE *out)
{
    fwrite("\x1f\x8b\x08\0\0\0\0\0\0\xff", 1, 10, out);
    *crc = crc32(0L, Z_NULL, 0);
    *tot = 0;
}

                                                                             
                                                                           
                                                                            
                                                                          
                                                                             
                                                                              
                                                                         
local void gzcopy(char *name, int clr, unsigned long *crc, unsigned long *tot,
                  FILE *out)
{
    int ret;                 
    int pos;                 
    int last;                
    bin *in;                 
    unsigned char *start;    
    unsigned char *junk;     
    z_off_t len;             
    z_stream strm;           

     
    in = bopen(name);
    if (in == NULL)
        bail("could not open ", name);
    gzhead(in);

                                                                       
                
    junk = malloc(CHUNK);
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, -15);
    if (junk == NULL || ret != Z_OK)
        bail("out of memory", "");

     
    len = 0;
    zpull(&strm, in);
    start = in->next;
    last = start[0] & 1;
    if (last && clr)
        start[0] &= ~1;
    strm.avail_out = 0;
    for (;;) {
         
        if (strm.avail_in == 0 && strm.avail_out != 0) {
            fwrite(start, 1, strm.next_in - start, out);
            start = in->buf;
            in->left = 0;
            zpull(&strm, in);
        }

         
        strm.avail_out = CHUNK;
        strm.next_out = junk;
        ret = inflate(&strm, Z_BLOCK);
        switch (ret) {
        case Z_MEM_ERROR:
            bail("out of memory", "");
        case Z_DATA_ERROR:
            bail("invalid compressed data in ", in->name);
        }

         
        len += CHUNK - strm.avail_out;

         
        if (strm.data_type & 128) {
             
            if (last)
                break;

             
            pos = strm.data_type & 7;

             
            if (pos != 0) {
                 
                pos = 0x100 >> pos;
                last = strm.next_in[-1] & pos;
                if (last && clr)
                    in->buf[strm.next_in - in->buf - 1] &= ~pos;
            }
            else {
                 
                if (strm.avail_in == 0) {
                     
                    fwrite(start, 1, strm.next_in - start, out);
                    start = in->buf;
                    in->left = 0;
                    zpull(&strm, in);
                }
                last = strm.next_in[0] & 1;
                if (last && clr)
                    in->buf[strm.next_in - in->buf] &= ~1;
            }
        }
    }

     
    in->left = strm.avail_in;
    in->next = in->buf + (strm.next_in - in->buf);

     
    pos = strm.data_type & 7;
    fwrite(start, 1, in->next - start - 1, out);
    last = in->next[-1];
    if (pos == 0 || !clr)
         
        putc(last, out);
    else {
         
        last &= ((0x100 >> pos) - 1);        
        if (pos & 1) {
             
            putc(last, out);
            if (pos == 1)
                putc(0, out);                
            fwrite("\0\0\xff\xff", 1, 4, out);
        }
        else {
             
            switch (pos) {
            case 6:
                putc(last | 8, out);
                last = 0;
            case 4:
                putc(last | 0x20, out);
                last = 0;
            case 2:
                putc(last | 0x80, out);
                putc(0, out);
            }
        }
    }

     
    *crc = crc32_combine(*crc, bget4(in), len);
    *tot += (unsigned long)len;

     
    inflateEnd(&strm);
    free(junk);
    bclose(in);

     
    if (!clr) {
        put4(*crc, out);
        put4(*tot, out);
    }
}

 
int main(int argc, char **argv)
{
    unsigned long crc, tot;      

     
    argc--;
    argv++;

     
    if (argc == 0) {
        fputs("gzjoin usage: gzjoin f1.gz [f2.gz [f3.gz ...]] > fjoin.gz\n",
              stderr);
        return 0;
    }

     
    gzinit(&crc, &tot, stdout);
    while (argc--)
        gzcopy(*argv++, argc, &crc, &tot, stdout);

     
    return 0;
}
