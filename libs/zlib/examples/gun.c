/* gun.c -- simple gunzip to give an example of the use of inflateBack()
 * Copyright (C) 2003, 2005, 2008, 2010, 2012 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
   Version 1.7  12 August 2012  Mark Adler */

                   
                                                               
                                                         
                                                                
                                              
                                                        
                                                          
                                                                
                                            
                                                          
                                                                        
                                                             
                                                         
                                                           
                                                              
                                                           
   

  
                          

                                                                             
                                                                             
                                                                           
                                                                           
                                                                            
                                                                         
                                                                           
                                                                            
                                                                         
                   

                                                                            
                                                                               
                                                                            
        

                                                                       
                                                                          
                                                                              
                                                                          
                                                                    

                                                                             
                                                                             
                             

                                                                            
                                                                            
                                                                           
                           
   

 
#include <stdio.h>           
#include <stdlib.h>          
#include <string.h>          
#include <errno.h>           
#include <fcntl.h>           
#include <unistd.h>          
#include <sys/types.h>
#include <sys/stat.h>        
#include <utime.h>           
#include "zlib.h"            
                             

 
#define local static

 
#define SIZE 32768U          
#define PIECE 16384          

                                                                             
                                          
struct ind {
    int infile;
    unsigned char *inbuf;
};

                                                                        
                                                                           
                                                       
local unsigned in(void *in_desc, z_const unsigned char **buf)
{
    int ret;
    unsigned len;
    unsigned char *next;
    struct ind *me = (struct ind *)in_desc;

    next = me->inbuf;
    *buf = next;
    len = 0;
    do {
        ret = PIECE;
        if ((unsigned)ret > SIZE - len)
            ret = (int)(SIZE - len);
        ret = (int)read(me->infile, next, ret);
        if (ret == -1) {
            len = 0;
            break;
        }
        next += ret;
        len += ret;
    } while (ret != 0 && len < SIZE);
    return len;
}

                                                                               
                                                                            
                                                                            
                                                                              
                                       
struct outd {
    int outfile;
    int check;                   
    unsigned long crc;
    unsigned long total;
};

                                                                              
                                                                           
                                                                             
                                                         
   
local int out(void *out_desc, unsigned char *buf, unsigned len)
{
    int ret;
    struct outd *me = (struct outd *)out_desc;

    if (me->check) {
        me->crc = crc32(me->crc, buf, len);
        me->total += len;
    }
    if (me->outfile != -1)
        do {
            ret = PIECE;
            if ((unsigned)ret > len)
                ret = (int)len;
            ret = (int)write(me->outfile, buf, ret);
            if (ret == -1)
                return 1;
            buf += ret;
            len -= ret;
        } while (len != 0);
    return 0;
}

 
#define NEXT() (have ? 0 : (have = in(indp, &next)), \
                last = have ? (have--, (int)(*next++)) : -1)

                                        
                                                                       
                                                                
unsigned char inbuf[SIZE];               
unsigned char outbuf[SIZE];              
unsigned short prefix[65536];            
unsigned char suffix[65536];             
unsigned char match[65280 + 2];                                             
                                                                

                                                                             
                                                                           
                                                     
#define FLUSHCODE() \
    do { \
        left = 0; \
        rem = 0; \
        if (chunk > have) { \
            chunk -= have; \
            have = 0; \
            if (NEXT() == -1) \
                break; \
            chunk--; \
            if (chunk > have) { \
                chunk = have = 0; \
                break; \
            } \
        } \
        have -= chunk; \
        next += chunk; \
        chunk = 0; \
    } while (0)

                                                                             
                                                                               
                                                                              
                

                                                                              
                                                                             
                                                           
   
local int lunpipe(unsigned have, z_const unsigned char *next, struct ind *indp,
                  int outfile, z_stream *strm)
{
    int last;                    
    unsigned chunk;              
    int left;                    
    unsigned rem;                
    int bits;                    
    unsigned code;               
    unsigned mask;               
    int max;                     
    unsigned flags;              
    unsigned end;                
    unsigned temp;               
    unsigned prev;               
    unsigned final;              
    unsigned stack;              
    unsigned outcnt;             
    struct outd outd;            
    unsigned char *p;

     
    outd.outfile = outfile;
    outd.check = 0;

     
    flags = NEXT();
    if (last == -1)
        return Z_BUF_ERROR;
    if (flags & 0x60) {
        strm->msg = (char *)"unknown lzw flags set";
        return Z_DATA_ERROR;
    }
    max = flags & 0x1f;
    if (max < 9 || max > 16) {
        strm->msg = (char *)"lzw bits out of range";
        return Z_DATA_ERROR;
    }
    if (max == 9)                            
        max = 10;
    flags &= 0x80;                           

     
    bits = 9;
    mask = 0x1ff;
    end = flags ? 256 : 255;

                                                                              
                                                        
    if (NEXT() == -1)                        
        return Z_OK;
    final = prev = (unsigned)last;           
    if (NEXT() == -1)                        
        return Z_BUF_ERROR;
    if (last & 1) {                          
        strm->msg = (char *)"invalid lzw code";
        return Z_DATA_ERROR;
    }
    rem = (unsigned)last >> 1;               
    left = 7;
    chunk = bits - 2;                        
    outbuf[0] = (unsigned char)final;        
    outcnt = 1;

     
    stack = 0;
    for (;;) {
         
        if (end >= mask && bits < max) {
            FLUSHCODE();
            bits++;
            mask <<= 1;
            mask++;
        }

         
        if (chunk == 0)                      
            chunk = bits;
        code = rem;                          
        if (NEXT() == -1) {                  
             
            if (outcnt && out(&outd, outbuf, outcnt)) {
                strm->next_in = outbuf;      
                return Z_BUF_ERROR;
            }
            return Z_OK;
        }
        code += (unsigned)last << left;      
        left += 8;
        chunk--;
        if (bits > left) {                   
            if (NEXT() == -1)                
                return Z_BUF_ERROR;
            code += (unsigned)last << left;  
            left += 8;
            chunk--;
        }
        code &= mask;                        
        left -= bits;                        
        rem = (unsigned)last >> (8 - left);  

         
        if (code == 256 && flags) {
            FLUSHCODE();
            bits = 9;                        
            mask = 0x1ff;
            end = 255;                       
            continue;                        
        }

         
        temp = code;                         
        if (code > end) {
                                                                             
                                                                          
                                                                               
                                                                              
                                                                               
                                                                             
                                                                           
                                                                     
            if (code != end + 1 || prev > end) {
                strm->msg = (char *)"invalid lzw code";
                return Z_DATA_ERROR;
            }
            match[stack++] = (unsigned char)final;
            code = prev;
        }

         
        p = match + stack;
        while (code >= 256) {
            *p++ = suffix[code];
            code = prefix[code];
        }
        stack = p - match;
        match[stack++] = (unsigned char)code;
        final = code;

         
        if (end < mask) {
            end++;
            prefix[end] = (unsigned short)prev;
            suffix[end] = (unsigned char)final;
        }

         
        prev = temp;

         
        while (stack > SIZE - outcnt) {
            while (outcnt < SIZE)
                outbuf[outcnt++] = match[--stack];
            if (out(&outd, outbuf, outcnt)) {
                strm->next_in = outbuf;  
                return Z_BUF_ERROR;
            }
            outcnt = 0;
        }
        p = match + stack;
        do {
            outbuf[outcnt++] = *--p;
        } while (p > match);
        stack = 0;

                                                                            
                                                                             
                               
    }
}

                                                                               
                                                                               
                                                                              
                                                                              
                                  

                                                                       
                                                                          
                                                                              
                                                                              
                                       
   
local int gunpipe(z_stream *strm, int infile, int outfile)
{
    int ret, first, last;
    unsigned have, flags, len;
    z_const unsigned char *next = NULL;
    struct ind ind, *indp;
    struct outd outd;

     
    ind.infile = infile;
    ind.inbuf = inbuf;
    indp = &ind;

     
    have = 0;                                
    first = 1;                               
    strm->next_in = Z_NULL;                  
    for (;;) {
         
        if (NEXT() == -1) {
            ret = Z_OK;
            break;                           
        }
        if (last != 31 || (NEXT() != 139 && last != 157)) {
            strm->msg = (char *)"incorrect header check";
            ret = first ? Z_DATA_ERROR : Z_ERRNO;
            break;                           
        }
        first = 0;                           

         
        if (last == 157) {
            ret = lunpipe(have, next, indp, outfile, strm);
            break;
        }

         
        ret = Z_BUF_ERROR;
        if (NEXT() != 8) {                   
            if (last == -1) break;
            strm->msg = (char *)"unknown compression method";
            ret = Z_DATA_ERROR;
            break;
        }
        flags = NEXT();                      
        NEXT();                              
        NEXT();
        NEXT();
        NEXT();
        NEXT();
        NEXT();
        if (last == -1) break;
        if (flags & 0xe0) {
            strm->msg = (char *)"unknown header flags set";
            ret = Z_DATA_ERROR;
            break;
        }
        if (flags & 4) {                     
            len = NEXT();
            len += (unsigned)(NEXT()) << 8;
            if (last == -1) break;
            while (len > have) {
                len -= have;
                have = 0;
                if (NEXT() == -1) break;
                len--;
            }
            if (last == -1) break;
            have -= len;
            next += len;
        }
        if (flags & 8)                       
            while (NEXT() != 0 && last != -1)
                ;
        if (flags & 16)                      
            while (NEXT() != 0 && last != -1)
                ;
        if (flags & 2) {                     
            NEXT();
            NEXT();
        }
        if (last == -1) break;

         
        outd.outfile = outfile;
        outd.check = 1;
        outd.crc = crc32(0L, Z_NULL, 0);
        outd.total = 0;

         
        strm->next_in = next;
        strm->avail_in = have;
        ret = inflateBack(strm, in, indp, out, &outd);
        if (ret != Z_STREAM_END) break;
        next = strm->next_in;
        have = strm->avail_in;
        strm->next_in = Z_NULL;              

         
        ret = Z_BUF_ERROR;
        if (NEXT() != (int)(outd.crc & 0xff) ||
            NEXT() != (int)((outd.crc >> 8) & 0xff) ||
            NEXT() != (int)((outd.crc >> 16) & 0xff) ||
            NEXT() != (int)((outd.crc >> 24) & 0xff)) {
             
            if (last != -1) {
                strm->msg = (char *)"incorrect data check";
                ret = Z_DATA_ERROR;
            }
            break;
        }
        if (NEXT() != (int)(outd.total & 0xff) ||
            NEXT() != (int)((outd.total >> 8) & 0xff) ||
            NEXT() != (int)((outd.total >> 16) & 0xff) ||
            NEXT() != (int)((outd.total >> 24) & 0xff)) {
             
            if (last != -1) {
                strm->msg = (char *)"incorrect length check";
                ret = Z_DATA_ERROR;
            }
            break;
        }

         
    }

     
    return ret;
}

                                                                             
                                                                               
                                                                           
                                                                     
local void copymeta(char *from, char *to)
{
    struct stat was;
    struct utimbuf when;

     
    if (stat(from, &was) != 0 || (was.st_mode & S_IFMT) != S_IFREG)
        return;

     
    (void)chmod(to, was.st_mode & 07777);

     
    (void)chown(to, was.st_uid, was.st_gid);

     
    when.actime = was.st_atime;
    when.modtime = was.st_mtime;
    (void)utime(to, &when);
}

                                                                            
                                                                           
                                                                             
                                                                           
                                                                        
           

                                                                         
                                                       
   
local int gunzip(z_stream *strm, char *inname, char *outname, int test)
{
    int ret;
    int infile, outfile;

     
    if (inname == NULL || *inname == 0) {
        inname = "-";
        infile = 0;      
    }
    else {
        infile = open(inname, O_RDONLY, 0);
        if (infile == -1) {
            fprintf(stderr, "gun cannot open %s\n", inname);
            return 0;
        }
    }
    if (test)
        outfile = -1;
    else if (outname == NULL || *outname == 0) {
        outname = "-";
        outfile = 1;     
    }
    else {
        outfile = open(outname, O_CREAT | O_TRUNC | O_WRONLY, 0666);
        if (outfile == -1) {
            close(infile);
            fprintf(stderr, "gun cannot create %s\n", outname);
            return 0;
        }
    }
    errno = 0;

     
    ret = gunpipe(strm, infile, outfile);
    if (outfile > 2) close(outfile);
    if (infile > 2) close(infile);

     
    switch (ret) {
    case Z_OK:
    case Z_ERRNO:
        if (infile > 2 && outfile > 2) {
            copymeta(inname, outname);           
            unlink(inname);
        }
        if (ret == Z_ERRNO)
            fprintf(stderr, "gun warning: trailing garbage ignored in %s\n",
                    inname);
        break;
    case Z_DATA_ERROR:
        if (outfile > 2) unlink(outname);
        fprintf(stderr, "gun data error on %s: %s\n", inname, strm->msg);
        break;
    case Z_MEM_ERROR:
        if (outfile > 2) unlink(outname);
        fprintf(stderr, "gun out of memory error--aborting\n");
        return 1;
    case Z_BUF_ERROR:
        if (outfile > 2) unlink(outname);
        if (strm->next_in != Z_NULL) {
            fprintf(stderr, "gun write error on %s: %s\n",
                    outname, strerror(errno));
        }
        else if (errno) {
            fprintf(stderr, "gun read error on %s: %s\n",
                    inname, strerror(errno));
        }
        else {
            fprintf(stderr, "gun unexpected end of file on %s\n",
                    inname);
        }
        break;
    default:
        if (outfile > 2) unlink(outname);
        fprintf(stderr, "gun internal error--aborting\n");
        return 1;
    }
    return 0;
}

                                                                           
                                    
int main(int argc, char **argv)
{
    int ret, len, test;
    char *outname;
    unsigned char *window;
    z_stream strm;

     
    window = match;                          
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = inflateBackInit(&strm, 15, window);
    if (ret != Z_OK) {
        fprintf(stderr, "gun out of memory error--aborting\n");
        return 1;
    }

     
    argc--;
    argv++;
    test = 0;
    if (argc && strcmp(*argv, "-h") == 0) {
        fprintf(stderr, "gun 1.6 (17 Jan 2010)\n");
        fprintf(stderr, "Copyright (C) 2003-2010 Mark Adler\n");
        fprintf(stderr, "usage: gun [-t] [file1.gz [file2.Z ...]]\n");
        return 0;
    }
    if (argc && strcmp(*argv, "-t") == 0) {
        test = 1;
        argc--;
        argv++;
    }
    if (argc)
        do {
            if (test)
                outname = NULL;
            else {
                len = (int)strlen(*argv);
                if (strcmp(*argv + len - 3, ".gz") == 0 ||
                    strcmp(*argv + len - 3, "-gz") == 0)
                    len -= 3;
                else if (strcmp(*argv + len - 2, ".z") == 0 ||
                    strcmp(*argv + len - 2, "-z") == 0 ||
                    strcmp(*argv + len - 2, "_z") == 0 ||
                    strcmp(*argv + len - 2, ".Z") == 0)
                    len -= 2;
                else {
                    fprintf(stderr, "gun error: no gz type on %s--skipping\n",
                            *argv);
                    continue;
                }
                outname = malloc(len + 1);
                if (outname == NULL) {
                    fprintf(stderr, "gun out of memory error--aborting\n");
                    ret = 1;
                    break;
                }
                memcpy(outname, *argv, len);
                outname[len] = 0;
            }
            ret = gunzip(&strm, *argv, outname, test);
            if (outname != NULL) free(outname);
            if (ret) break;
        } while (argv++, --argc);
    else
        ret = gunzip(&strm, NULL, NULL, test);

     
    inflateBackEnd(&strm);
    return ret;
}
