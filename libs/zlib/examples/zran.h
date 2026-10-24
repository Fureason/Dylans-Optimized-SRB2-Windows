/* zran.h -- example of deflated stream indexing and random access
 * Copyright (C) 2005, 2012, 2018, 2023 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 * Version 1.3  18 Feb 2023  Mark Adler */

#include <stdio.h>
#include "zlib.h"


typedef struct point {
    off_t out;           
    off_t in;            
    int bits;            
    unsigned char window[32768];     
} point_t;


struct deflate_index {
    int have;            
    int mode;            
    off_t length;        
    point_t *list;       
};










int deflate_index_build(FILE *in, off_t span, struct deflate_index **built);












ptrdiff_t deflate_index_extract(FILE *in, struct deflate_index *index,
                                off_t offset, unsigned char *buf, size_t len);


void deflate_index_free(struct deflate_index *index);
