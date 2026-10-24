/* gzappend -- command to append to a gzip file

  Copyright (C) 2003, 2012 Mark Adler, all rights reserved
  version 1.2, 11 Oct 2012

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

/*
 * Change history:
 *
 * 1.0  19 Oct 2003     - First version
 * 1.1   4 Nov 2003     - Expand and clarify some comments and notes
 *                      - Add version and copyright to help
 *                      - Send help to stdout instead of stderr
 *                      - Add some preemptive typecasts
 *                      - Add L to constants in lseek() calls
 *                      - Remove some debugging information in error messages
 *                      - Use new data_type definition for zlib 1.2.1
 *                      - Simplify and unify file operations
 *                      - Finish off gzip file in gztack()
 *                      - Use deflatePrime() instead of adding empty blocks
 *                      - Keep gzip file clean on appended file read errors
 *                      - Use in-place rotate instead of auxiliary buffer
 *                        (Why you ask?  Because it was fun to write!)
 * 1.2  11 Oct 2012     - Fix for proper z_const usage
 *                      - Check for input buffer malloc failure
 */

  
                                                                           
                                                                             
                                                                              
                                                                          

                                                                              
                                                                               
                                                                            
                                                                            
                                                                            
                                                                             
                                            

                                                                           
                                                                               
                                                                             
                                                                             
                                                        

                                                                               
                                                                               
                                                                            

                                                                           
                                                                      
                                                                            
                                                                              
                                                                         
                                                                           
                                                                            
                                   
   

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "zlib.h"

#define local static
#define LGCHUNK 14
#define CHUNK (1U << LGCHUNK)
#define DSIZE 32768U

 
local void bye(char *msg1, char *msg2)
{
    fprintf(stderr, "gzappend error: %s%s\n", msg1, msg2);
    exit(1);
}

                                                                          
                                                                         
                                         
local unsigned gcd(unsigned a, unsigned b)
{
    unsigned c;

    while (a && b)
        if (a > b) {
            c = b;
            while (a - c >= c)
                c <<= 1;
            a -= c;
        }
        else {
            c = a;
            while (b - c >= c)
                c <<= 1;
            b -= c;
        }
    return a + b;
}

 
local void rotate(unsigned char *list, unsigned len, unsigned rot)
{
    unsigned char tmp;
    unsigned cycles;
    unsigned char *start, *last, *to, *from;

     
    if (len < 2) return;
    if (rot >= len) rot %= len;
    if (rot == 0) return;

     
    last = list + (len - 1);

     
    if (rot == 1) {
        tmp = *list;
        memmove(list, list + 1, len - 1);
        *last = tmp;
        return;
    }

     
    if (rot == len - 1) {
        tmp = *last;
        memmove(list + 1, list, len - 1);
        *list = tmp;
        return;
    }

     
    cycles = gcd(len, rot);              
    do {
        start = from = list + cycles;    
        tmp = *from;                     
        for (;;) {
            to = from;                   
            from += rot;                 
            if (from > last) from -= len;    
            if (from == start) break;    
            *to = *from;                 
        }
        *to = tmp;                       
    } while (--cycles);
}

 
typedef struct {
    int fd;                      
    int size;                    
    unsigned left;               
    unsigned char *buf;          
    z_const unsigned char *next;     
    char *name;                  
} file;

 
local int readin(file *in)
{
    int len;

    len = read(in->fd, in->buf, 1 << in->size);
    if (len == -1) bye("error reading ", in->name);
    in->left = (unsigned)len;
    in->next = in->buf;
    return len;
}

 
local int readmore(file *in)
{
    if (readin(in) == 0) bye("unexpected end of ", in->name);
    return 0;
}

#define read1(in) (in->left == 0 ? readmore(in) : 0, \
                   in->left--, *(in->next)++)

 
local void skip(file *in, unsigned n)
{
    unsigned bypass;

    if (n > in->left) {
        n -= in->left;
        bypass = n & ~((1U << in->size) - 1);
        if (bypass) {
            if (lseek(in->fd, (off_t)bypass, SEEK_CUR) == -1)
                bye("seeking ", in->name);
            n -= bypass;
        }
        readmore(in);
        if (n > in->left)
            bye("unexpected end of ", in->name);
    }
    in->left -= n;
    in->next += n;
}

 
unsigned long read4(file *in)
{
    unsigned long val;

    val = read1(in);
    val += (unsigned)read1(in) << 8;
    val += (unsigned long)read1(in) << 16;
    val += (unsigned long)read1(in) << 24;
    return val;
}

 
local void gzheader(file *in)
{
    int flags;
    unsigned n;

    if (read1(in) != 31 || read1(in) != 139) bye(in->name, " not a gzip file");
    if (read1(in) != 8) bye("unknown compression method in", in->name);
    flags = read1(in);
    if (flags & 0xe0) bye("unknown header flags set in", in->name);
    skip(in, 6);
    if (flags & 4) {
        n = read1(in);
        n += (unsigned)(read1(in)) << 8;
        skip(in, n);
    }
    if (flags & 8) while (read1(in) != 0) ;
    if (flags & 16) while (read1(in) != 0) ;
    if (flags & 2) skip(in, 2);
}

                                                                          
                                                                       
                                                                           
                                                           
local int gzscan(char *name, z_stream *strm, int level)
{
    int ret, lastbit, left, full;
    unsigned have;
    unsigned long crc, tot;
    unsigned char *window;
    off_t lastoff, end;
    file gz;

     
    gz.name = name;
    gz.fd = open(name, O_RDWR, 0);
    if (gz.fd == -1) bye("cannot open ", name);
    gz.buf = malloc(CHUNK);
    if (gz.buf == NULL) bye("out of memory", "");
    gz.size = LGCHUNK;
    gz.left = 0;

     
    gzheader(&gz);

     
    window = malloc(DSIZE);
    if (window == NULL) bye("out of memory", "");
    strm->zalloc = Z_NULL;
    strm->zfree = Z_NULL;
    strm->opaque = Z_NULL;
    ret = inflateInit2(strm, -15);
    if (ret != Z_OK) bye("out of memory", " or library mismatch");

     
    lastbit = 0;
    lastoff = lseek(gz.fd, 0L, SEEK_CUR) - gz.left;
    left = 0;
    strm->avail_in = gz.left;
    strm->next_in = gz.next;
    crc = crc32(0L, Z_NULL, 0);
    have = full = 0;
    do {
         
        if (strm->avail_in == 0) {
            readmore(&gz);
            strm->avail_in = gz.left;
            strm->next_in = gz.next;
        }

         
        strm->avail_out = DSIZE - have;
        strm->next_out = window + have;

         
        ret = inflate(strm, Z_BLOCK);
        if (ret == Z_STREAM_ERROR) bye("internal stream error!", "");
        if (ret == Z_MEM_ERROR) bye("out of memory", "");
        if (ret == Z_DATA_ERROR)
            bye("invalid compressed data--format violated in", name);

         
        crc = crc32(crc, window + have, DSIZE - have - strm->avail_out);
        if (strm->avail_out)
            have = DSIZE - strm->avail_out;
        else {
            have = 0;
            full = 1;
        }

         
        if (strm->data_type & 128) {
            if (strm->data_type & 64)
                left = strm->data_type & 0x1f;
            else {
                lastbit = strm->data_type & 0x1f;
                lastoff = lseek(gz.fd, 0L, SEEK_CUR) - strm->avail_in;
            }
        }
    } while (ret != Z_STREAM_END);
    inflateEnd(strm);
    gz.left = strm->avail_in;
    gz.next = strm->next_in;

     
    end = lseek(gz.fd, 0L, SEEK_CUR) - gz.left;

     
    if (crc != read4(&gz))
        bye("invalid compressed data--crc mismatch in ", name);
    tot = strm->total_out;
    if ((tot & 0xffffffffUL) != read4(&gz))
        bye("invalid compressed data--length mismatch in", name);

     
    if (gz.left || readin(&gz))
        fprintf(stderr,
            "gzappend warning: junk at end of gzip file overwritten\n");

     
    lseek(gz.fd, lastoff - (lastbit != 0), SEEK_SET);
    if (read(gz.fd, gz.buf, 1) != 1) bye("reading after seek on ", name);
    *gz.buf = (unsigned char)(*gz.buf ^ (1 << ((8 - lastbit) & 7)));
    lseek(gz.fd, -1L, SEEK_CUR);
    if (write(gz.fd, gz.buf, 1) != 1) bye("writing after seek to ", name);

     
    if (full) {
        rotate(window, DSIZE, have);
        have = DSIZE;
    }

     
    ret = deflateInit2(strm, level, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) bye("out of memory", "");
    deflateSetDictionary(strm, window, have);
    strm->adler = crc;
    strm->total_in = tot;
    if (left) {
        lseek(gz.fd, --end, SEEK_SET);
        if (read(gz.fd, gz.buf, 1) != 1) bye("reading after seek on ", name);
        deflatePrime(strm, 8 - left, *gz.buf);
    }
    lseek(gz.fd, end, SEEK_SET);

     
    free(window);
    free(gz.buf);
    return gz.fd;
}

                                                                          
                                                            
local void gztack(char *name, int gd, z_stream *strm, int last)
{
    int fd, len, ret;
    unsigned left;
    unsigned char *in, *out;

     
    fd = 0;
    if (name != NULL) {
        fd = open(name, O_RDONLY, 0);
        if (fd == -1)
            fprintf(stderr, "gzappend warning: %s not found, skipping ...\n",
                    name);
    }

     
    in = malloc(CHUNK);
    out = malloc(CHUNK);
    if (in == NULL || out == NULL) bye("out of memory", "");

     
    do {
         
        len = read(fd, in, CHUNK);
        if (len == -1) {
            fprintf(stderr,
                    "gzappend warning: error reading %s, skipping rest ...\n",
                    name);
            len = 0;
        }
        strm->avail_in = (unsigned)len;
        strm->next_in = in;
        if (len) strm->adler = crc32(strm->adler, in, (unsigned)len);

         
        do {
            strm->avail_out = CHUNK;
            strm->next_out = out;
            ret = deflate(strm, last && len == 0 ? Z_FINISH : Z_NO_FLUSH);
            left = CHUNK - strm->avail_out;
            while (left) {
                len = write(gd, out + CHUNK - strm->avail_out - left, left);
                if (len == -1) bye("writing gzip file", "");
                left -= (unsigned)len;
            }
        } while (strm->avail_out == 0 && ret != Z_STREAM_END);
    } while (len != 0);

     
    if (last) {
        deflateEnd(strm);
        out[0] = (unsigned char)(strm->adler);
        out[1] = (unsigned char)(strm->adler >> 8);
        out[2] = (unsigned char)(strm->adler >> 16);
        out[3] = (unsigned char)(strm->adler >> 24);
        out[4] = (unsigned char)(strm->total_in);
        out[5] = (unsigned char)(strm->total_in >> 8);
        out[6] = (unsigned char)(strm->total_in >> 16);
        out[7] = (unsigned char)(strm->total_in >> 24);
        len = 8;
        do {
            ret = write(gd, out + 8 - len, len);
            if (ret == -1) bye("writing gzip file", "");
            len -= ret;
        } while (len);
        close(gd);
    }

     
    free(out);
    free(in);
    if (fd > 0) close(fd);
}

                                                                           
                                                                             
                                                                           
                  
int main(int argc, char **argv)
{
    int gd, level;
    z_stream strm;

     
    argc--; argv++;

     
    if (*argv == NULL) {
        printf(
            "gzappend 1.2 (11 Oct 2012) Copyright (C) 2003, 2012 Mark Adler\n"
               );
        printf(
            "usage: gzappend [-level] file.gz [ addthis [ andthis ... ]]\n");
        return 0;
    }

     
    level = Z_DEFAULT_COMPRESSION;
    if (argv[0][0] == '-') {
        if (argv[0][1] < '0' || argv[0][1] > '9' || argv[0][2] != 0)
            bye("invalid compression level", "");
        level = argv[0][1] - '0';
        if (*++argv == NULL) bye("no gzip file name after options", "");
    }

     
    gd = gzscan(*argv++, &strm, level);

     
    if (*argv == NULL)
        gztack(NULL, gd, &strm, 1);
    else
        do {
            gztack(*argv, gd, &strm, argv[1] == NULL);
        } while (*++argv != NULL);
    return 0;
}
