/* gznorm.c -- normalize a gzip stream
 * Copyright (C) 2018 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 * Version 1.0  7 Oct 2018  Mark Adler */








#include <stdio.h>       

#include <stdlib.h>      
#include <string.h>      
#include <errno.h>       
#include <stdarg.h>      
#include "zlib.h"        




#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define local static



local char *aprintf(char *fmt, ...) {

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len < 0)
        return NULL;


    char *str = malloc(len + 1);
    if (str == NULL)
        return NULL;
    va_start(args, fmt);
    vsnprintf(str, len + 1, fmt, args);
    va_end(args);
    return str;
}




#define BYE(...) \
    do { \
        inflateEnd(&strm); \
        *err = aprintf(__VA_ARGS__); \
        return 1; \
    } while (0)



#define CHUNK 16384


















local int gzip_normalize(FILE *in, FILE *out, char **err) {

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    if (inflateInit2(&strm, 15 + 16) != Z_OK)
        BYE("out of memory");


    enum {               
        BETWEEN,         
        HEAD,            
        BLOCK,           
        TAIL             
    } state = BETWEEN;               
    unsigned long crc = 0;           
    unsigned long len = 0;           
    unsigned long buf = 0;           
    int num = 0;                     



    fwrite("\x1f\x8b\x08\0\0\0\0\0\0\xff", 1, 10, out);



    int more;                        
    do {

        unsigned char *put;          
        int prev;                    


        unsigned long long memb;     
        size_t tail;                 
        unsigned long part;          


        unsigned char dat[CHUNK];
        strm.avail_in = fread(dat, 1, CHUNK, in);
        if (strm.avail_in == 0)
            break;
        more = strm.avail_in == CHUNK;
        strm.next_in = put = dat;


        do {







            do {

                unsigned char scrap[CHUNK];
                strm.avail_out = CHUNK;
                strm.next_out = scrap;
                int ret = inflate(&strm, Z_BLOCK);
                if (ret == Z_MEM_ERROR)
                    BYE("out of memory");
                if (ret == Z_DATA_ERROR)
                    BYE("input invalid: %s", strm.msg);
                if (ret != Z_OK && ret != Z_BUF_ERROR && ret != Z_STREAM_END)
                    BYE("internal error");




                unsigned got = CHUNK - strm.avail_out;
                memb += got;
                if (memb < got)
                    BYE("overflow error");




            } while (strm.avail_out == 0 && (strm.data_type & 0x80) == 0);





            switch (state) {
                case BETWEEN:
                    state = HEAD;



                case HEAD:

                    if (strm.data_type & 0x80) {

                        put = strm.next_in;
                        prev = num;
                        memb = 0;
                        state = BLOCK;
                    }
                    break;

                case BLOCK:

































                    ;                    
                    int bits = strm.data_type & 0x1f;

                    if (prev != -1) {









                        if (num - prev < 3) {
                            buf += (unsigned long)*put++ << num;
                            num += 8;
                        }



                        unsigned long last = (unsigned long)1 << prev;

                        if (((buf >> prev) & 7) == 3) {




                            if (num - prev < 10) {
                                if (put == strm.next_in)


                                    break;
                                buf += (unsigned long)*put++ << num;
                                num += 8;
                            }
                            if (((buf >> prev) & 0x3ff) == 3) {



                                num = prev;
                                buf &= last - 1;     
                            }
                        }
                        else if (((buf >> prev) & 6) == 0) {


                            num = (prev + 10) & ~7;
                            buf &= last - 1;         
                        }


                        buf &= ~last;


                        while (num >= 8) {
                            putc(buf, out);
                            buf >>= 8;
                            num -= 8;
                        }



                        if (put == strm.next_in)
                            bits = 0;
                    }












                    int mix = (strm.data_type & 0x80) && bits;







                    unsigned char *end = strm.next_in - mix;
                    if (put < end) {
                        if (num)


                            do {
                                buf += (unsigned)(*put++) << num;
                                putc(buf, out);
                                buf >>= 8;
                            } while (put < end);
                        else {

                            fwrite(put, 1, end - put, out);
                            put = end;
                        }
                    }


                    if (mix) {

                        buf += (unsigned)(*put++) << num;
                        num += 8;

                        if (strm.data_type & 0x40) {





                            num -= bits;
                            if (num >= 8) {
                                putc(buf, out);
                                num -= 8;
                                buf >>= 8;
                            }


                            buf &= ((unsigned long)1 << num) - 1;


                        }
                        else




                            prev = num - bits;       

                    }





                    else if (strm.data_type & 0x80)


                        prev = num;
                    else

                        prev = -1;


                    if ((strm.data_type & 0xc0) == 0xc0) {





                        tail = 0;
                        part = 0;
                        state = TAIL;
                    }
                    break;

                case TAIL:


                    do {
                        part = (part >> 8) + ((unsigned long)(*put++) << 24);
                        tail++;
                        if (tail == 4) {

                            z_off_t len2 = memb;
                            if (len2 < 0 || (unsigned long long)len2 != memb)
                                BYE("overflow error");
                            crc = crc ? crc32_combine(crc, part, len2) : part;
                            part = 0;
                        }
                        else if (tail == 8) {


                            len += part;




                            if (inflateReset(&strm) != Z_OK)
                                BYE("internal error");
                            state = BETWEEN;
                            break;
                        }
                    } while (put < strm.next_in);
                    break;
            }


        } while (strm.avail_in > 0);


    } while (more);


    inflateEnd(&strm);


    if (state != BETWEEN)
        BYE("input invalid: incomplete gzip stream");



    buf += (unsigned long)3 << num;
    putc(buf, out);
    putc(buf >> 8, out);
    if (num > 6)
        putc(0, out);



    putc(crc, out);
    putc(crc >> 8, out);
    putc(crc >> 16, out);
    putc(crc >> 24, out);
    putc(len, out);
    putc(len >> 8, out);
    putc(len >> 16, out);
    putc(len >> 24, out);
    fflush(out);


    if (ferror(in) || ferror(out))
        BYE("i/o error: %s", strerror(errno));


    *err = NULL;
    return 0;
}


int main(void) {

    SET_BINARY_MODE(stdin);
    SET_BINARY_MODE(stdout);


    char *err;
    int ret = gzip_normalize(stdin, stdout, &err);
    if (ret)
        fprintf(stderr, "gznorm error: %s\n", err);
    free(err);
    return ret;
}
