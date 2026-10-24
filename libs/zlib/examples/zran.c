/* zran.c -- example of deflate stream indexing and random access
 * Copyright (C) 2005, 2012, 2018, 2023 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 * Version 1.4  13 Apr 2023  Mark Adler */

                   
                                
                                                
                                                            
                                                                        
                                                                      
                                                                         
                                                                     
                                                                        
                                                                       
                                                                 
   







































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "zlib.h"
#include "zran.h"

#define WINSIZE 32768U       
#define CHUNK 16384          


void deflate_index_free(struct deflate_index *index) {
    if (index != NULL) {
        free(index->list);
        free(index);
    }
}





static struct deflate_index *add_point(struct deflate_index *index, int bits,
                                       off_t in, off_t out, unsigned left,
                                       unsigned char *window) {
    if (index == NULL) {

        index = malloc(sizeof(struct deflate_index));
        if (index == NULL)
            return NULL;
        index->have = 0;
        index->mode = 8;
        index->list = malloc(sizeof(point_t) * index->mode);
        if (index->list == NULL) {
            free(index);
            return NULL;
        }
    }

    else if (index->have == index->mode) {

        index->mode <<= 1;
        point_t *next = realloc(index->list, sizeof(point_t) * index->mode);
        if (next == NULL) {
            deflate_index_free(index);
            return NULL;
        }
        index->list = next;
    }


    point_t *next = (point_t *)(index->list) + index->have++;
    if (index->have < 0) {

        deflate_index_free(index);
        return NULL;
    }
    next->out = out;
    next->in = in;
    next->bits = bits;
    if (left)
        memcpy(next->window, window + WINSIZE - left, left);
    if (left < WINSIZE)
        memcpy(next->window + left, window, WINSIZE - left);


    return index;
}


#define RAW -15
#define ZLIB 15
#define GZIP 31


int deflate_index_build(FILE *in, off_t span, struct deflate_index **built) {

    z_stream strm = {0};         
    unsigned char buf[CHUNK];    
    unsigned char win[WINSIZE] = {0};    
    off_t totin = 0;             
    off_t totout = 0;            
    int mode = 0;                


    int ret;                     
    off_t last;                  
    struct deflate_index *index = NULL;      
    do {

        if (strm.avail_in == 0) {
            strm.avail_in = fread(buf, 1, sizeof(buf), in);
            totin += strm.avail_in;
            strm.next_in = buf;
            if (strm.avail_in < sizeof(buf) && ferror(in)) {
                ret = Z_ERRNO;
                break;
            }

            if (mode == 0) {





                mode = strm.avail_in == 0 ? RAW :        
                       (strm.next_in[0] & 0xf) == 8 ? ZLIB :
                       strm.next_in[0] == 0x1f ? GZIP :
                         RAW;
                ret = inflateInit2(&strm, mode);
                if (ret != Z_OK)
                    break;
            }
        }



        if (strm.avail_out == 0) {
            strm.avail_out = sizeof(win);
            strm.next_out = win;
        }

        if (mode == RAW && index == NULL)



            strm.data_type = 0x80;
        else {

            unsigned before = strm.avail_out;
            ret = inflate(&strm, Z_BLOCK);
            totout += before - strm.avail_out;
        }

        if ((strm.data_type & 0xc0) == 0x80 &&
            (index == NULL || totout - last >= span)) {





            index = add_point(index, strm.data_type & 7, totin - strm.avail_in,
                              totout, strm.avail_out, win);
            if (index == NULL) {
                ret = Z_MEM_ERROR;
                break;
            }
            last = totout;
        }

        if (ret == Z_STREAM_END && mode == GZIP &&
            (strm.avail_in || ungetc(getc(in), in) != EOF))



            ret = inflateReset2(&strm, GZIP);



    } while (ret == Z_OK);
    inflateEnd(&strm);

    if (ret != Z_STREAM_END) {


        deflate_index_free(index);
        return ret == Z_NEED_DICT ? Z_DATA_ERROR : ret;
    }


    index->mode = mode;
    index->length = totout;
    point_t *list = realloc(index->list, sizeof(point_t) * index->have);
    if (list == NULL) {


        deflate_index_free(index);
        return Z_MEM_ERROR;
    }
    index->list = list;
    *built = index;
    return index->have;
}

#ifdef NOPRIME



#  define INFLATEPRIME inflatePreface






static inline void append_bits(unsigned value, int bits,
                               unsigned char *in, int *have) {
    in += *have >> 3;            
    int k = *have & 7;           
    *have += bits;
    if (k)
        *in |= value << k;       
    else
        *in = value;
    k = 8 - k;                   
    while (bits > k) {
        value >>= k;             
        bits -= k;
        k = 8;                   
        *++in = value;
    }
}







static int inflatePreface(z_stream *strm, int bits, int value) {

    if (strm == Z_NULL || bits < 0 || bits > 16)
        return Z_STREAM_ERROR;
    if (bits == 0)
        return Z_OK;
    value &= (2 << (bits - 1)) - 1;



    static const unsigned char dyn[] = {
        4, 0xe0, 0x81, 8, 0, 0, 0, 0, 0x20, 0xa8, 0xab, 0x1f
    };
    const int dynlen = 95;           



    unsigned char in[(dynlen + 3 * 10 + 16 + 7) / 8];
    int have = 0;
    if (bits & 1) {



        memcpy(in, dyn, sizeof(dyn));
        have = dynlen;
    }
    while ((have + bits) & 7)


        append_bits(2, 10, in, &have);


    append_bits(value, bits, in, &have);









    strm->avail_in = have >> 3;
    strm->next_in = in;
    strm->avail_out = 0;
    strm->next_out = in;                 
    return inflate(strm, Z_NO_FLUSH);
}

#else
#  define INFLATEPRIME inflatePrime
#endif


ptrdiff_t deflate_index_extract(FILE *in, struct deflate_index *index,
                                off_t offset, unsigned char *buf, size_t len) {

    if (index == NULL || index->have < 1 || index->list[0].out != 0)
        return Z_STREAM_ERROR;


    if (len == 0 || offset < 0 || offset >= index->length)
        return 0;


    int lo = -1, hi = index->have;
    point_t *point = index->list;
    while (hi - lo > 1) {
        int mid = (lo + hi) >> 1;
        if (offset < point[mid].out)
            hi = mid;
        else
            lo = mid;
    }
    point += lo;


    int ret = fseeko(in, point->in - (point->bits ? 1 : 0), SEEK_SET);
    if (ret == -1)
        return Z_ERRNO;
    int ch = 0;
    if (point->bits && (ch = getc(in)) == EOF)
        return ferror(in) ? Z_ERRNO : Z_BUF_ERROR;
    z_stream strm = {0};
    ret = inflateInit2(&strm, RAW);
    if (ret != Z_OK)
        return ret;
    if (point->bits)
        INFLATEPRIME(&strm, point->bits, ch >> (8 - point->bits));
    inflateSetDictionary(&strm, point->window, WINSIZE);


    unsigned char input[CHUNK];
    unsigned char discard[WINSIZE];
    offset -= point->out;        
    size_t left = len;           
    do {
        if (offset) {

            strm.avail_out = offset < WINSIZE ? (unsigned)offset : WINSIZE;
            strm.next_out = discard;
        }
        else {

            strm.avail_out = left < UINT_MAX ? (unsigned)left : UINT_MAX;
            strm.next_out = buf + len - left;
        }


        if (strm.avail_in == 0) {

            strm.avail_in = fread(input, 1, CHUNK, in);
            if (strm.avail_in < CHUNK && ferror(in)) {
                ret = Z_ERRNO;
                break;
            }
            strm.next_in = input;
        }
        unsigned got = strm.avail_out;
        ret = inflate(&strm, Z_NO_FLUSH);
        got -= strm.avail_out;


        if (offset)
            offset -= got;
        else
            left -= got;



        if (ret == Z_STREAM_END && index->mode == GZIP) {

            unsigned drop = 8;               
            if (strm.avail_in >= drop) {
                strm.avail_in -= drop;
                strm.next_in += drop;
            }
            else {

                drop -= strm.avail_in;
                strm.avail_in = 0;
                do {
                    if (getc(in) == EOF)

                        return ferror(in) ? Z_ERRNO : Z_BUF_ERROR;
                } while (--drop);
            }

            if (strm.avail_in || ungetc(getc(in), in) != EOF) {


                inflateReset2(&strm, GZIP);
                do {
                    if (strm.avail_in == 0) {
                        strm.avail_in = fread(input, 1, CHUNK, in);
                        if (strm.avail_in < CHUNK && ferror(in)) {
                            ret = Z_ERRNO;
                            break;
                        }
                        strm.next_in = input;
                    }
                    strm.avail_out = WINSIZE;
                    strm.next_out = discard;
                    ret = inflate(&strm, Z_BLOCK);   
                } while (ret == Z_OK && (strm.data_type & 0x80) == 0);
                if (ret != Z_OK)
                    break;
                inflateReset2(&strm, RAW);
            }
        }



    } while (ret == Z_OK && left);
    inflateEnd(&strm);


    return ret == Z_OK || ret == Z_STREAM_END ? len - left : ret;
}

#ifdef TEST

#define SPAN 1048576L        
#define LEN 16384            






int main(int argc, char **argv) {

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: zran file.raw [offset]\n");
        return 1;
    }
    FILE *in = fopen(argv[1], "rb");
    if (in == NULL) {
        fprintf(stderr, "zran: could not open %s for reading\n", argv[1]);
        return 1;
    }


    off_t offset = -1;
    if (argc == 3) {
        char *end;
        offset = strtoll(argv[2], &end, 10);
        if (*end || offset < 0) {
            fprintf(stderr, "zran: %s is not a valid offset\n", argv[2]);
            return 1;
        }
    }


    struct deflate_index *index = NULL;
    int len = deflate_index_build(in, SPAN, &index);
    if (len < 0) {
        fclose(in);
        switch (len) {
        case Z_MEM_ERROR:
            fprintf(stderr, "zran: out of memory\n");
            break;
        case Z_BUF_ERROR:
            fprintf(stderr, "zran: %s ended prematurely\n", argv[1]);
            break;
        case Z_DATA_ERROR:
            fprintf(stderr, "zran: compressed data error in %s\n", argv[1]);
            break;
        case Z_ERRNO:
            fprintf(stderr, "zran: read error on %s\n", argv[1]);
            break;
        default:
            fprintf(stderr, "zran: error %d while building index\n", len);
        }
        return 1;
    }
    fprintf(stderr, "zran: built index with %d access points\n", len);


    unsigned char buf[LEN];
    if (offset == -1)
        offset = ((index->length + 1) << 1) / 3;
    ptrdiff_t got = deflate_index_extract(in, index, offset, buf, LEN);
    if (got < 0)
        fprintf(stderr, "zran: extraction failed: %s error\n",
                got == Z_MEM_ERROR ? "out of memory" : "input corrupted");
    else {
        fwrite(buf, 1, got, stdout);
        fprintf(stderr, "zran: extracted %ld bytes at %lld\n", got, offset);
    }


    deflate_index_free(index);
    fclose(in);
    return 0;
}

#endif
