/* fitblk.c: example of fitting compressed output to a specified size
   Not copyrighted -- provided to the public domain
   Version 1.1  25 November 2004  Mark Adler */

                   
                                  
                                                           
                                                                
                                                                    
                                                     
                                                         
   

                                                        

                                                                     
                                                                     
                                                                    
                                                                
                                                                    
                                                                    
                                                                    
                                                                  
                                                                  
                                                                   
                                                                    
                                                                      
                                                                    
                                                                  
                                                                    
                                                                   
                                                                 
                                                                    
                                                                  
                                                                    
                       

                                                                    
                                                                 
                                          

                                                                   
                                                                    
                                                                     
                                                                  
                                                                     
                                                                     
                                                                      
                                                                  
                                                                    
                            
   

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "zlib.h"

#define local static

 
local void quit(char *why)
{
    fprintf(stderr, "fitblk abort: %s\n", why);
    exit(1);
}

#define RAWLEN 4096     

                                                                    
                                                                   
                                      
local int partcompress(FILE *in, z_streamp def)
{
    int ret, flush;
    unsigned char raw[RAWLEN];

    flush = Z_NO_FLUSH;
    do {
        def->avail_in = fread(raw, 1, RAWLEN, in);
        if (ferror(in))
            return Z_ERRNO;
        def->next_in = raw;
        if (feof(in))
            flush = Z_FINISH;
        ret = deflate(def, flush);
        assert(ret != Z_STREAM_ERROR);
    } while (def->avail_out != 0 && flush == Z_NO_FLUSH);
    return ret;
}

                                                                     
                                                                 
                                                                  
                                                              
local int recompress(z_streamp inf, z_streamp def)
{
    int ret, flush;
    unsigned char raw[RAWLEN];

    flush = Z_NO_FLUSH;
    do {
         
        inf->avail_out = RAWLEN;
        inf->next_out = raw;
        ret = inflate(inf, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR && ret != Z_DATA_ERROR &&
               ret != Z_NEED_DICT);
        if (ret == Z_MEM_ERROR)
            return ret;

         
        def->avail_in = RAWLEN - inf->avail_out;
        def->next_in = raw;
        if (inf->avail_out != 0)
            flush = Z_FINISH;
        ret = deflate(def, flush);
        assert(ret != Z_STREAM_ERROR);
    } while (ret != Z_STREAM_END && def->avail_out != 0);
    return ret;
}

#define EXCESS 256       
#define MARGIN 8         

 
int main(int argc, char **argv)
{
    int ret;                 
    unsigned size;           
    unsigned have;           
    unsigned char *blk;      
    unsigned char *tmp;      
    z_stream def, inf;       

     
    if (argc != 2)
        quit("need one argument: size of output block");
    ret = strtol(argv[1], argv + 1, 10);
    if (argv[1][0] != 0)
        quit("argument must be a number");
    if (ret < 8)             
        quit("need positive size of 8 or greater");
    size = (unsigned)ret;

     
    blk = malloc(size + EXCESS);
    def.zalloc = Z_NULL;
    def.zfree = Z_NULL;
    def.opaque = Z_NULL;
    ret = deflateInit(&def, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK || blk == NULL)
        quit("out of memory");

     
    def.avail_out = size + EXCESS;
    def.next_out = blk;
    ret = partcompress(stdin, &def);
    if (ret == Z_ERRNO)
        quit("error reading input");

     
    if (ret == Z_STREAM_END && def.avail_out >= EXCESS) {
         
        have = size + EXCESS - def.avail_out;
        if (fwrite(blk, 1, have, stdout) != have || ferror(stdout))
            quit("error writing output");

         
        ret = deflateEnd(&def);
        assert(ret != Z_STREAM_ERROR);
        free(blk);
        fprintf(stderr,
                "%u bytes unused out of %u requested (all input)\n",
                size - have, size);
        return 0;
    }

     
    inf.zalloc = Z_NULL;
    inf.zfree = Z_NULL;
    inf.opaque = Z_NULL;
    inf.avail_in = 0;
    inf.next_in = Z_NULL;
    ret = inflateInit(&inf);
    tmp = malloc(size + EXCESS);
    if (ret != Z_OK || tmp == NULL)
        quit("out of memory");
    ret = deflateReset(&def);
    assert(ret != Z_STREAM_ERROR);

     
    inf.avail_in = size + EXCESS;
    inf.next_in = blk;
    def.avail_out = size + EXCESS;
    def.next_out = tmp;
    ret = recompress(&inf, &def);
    if (ret == Z_MEM_ERROR)
        quit("out of memory");

     
    ret = inflateReset(&inf);
    assert(ret != Z_STREAM_ERROR);
    ret = deflateReset(&def);
    assert(ret != Z_STREAM_ERROR);

     
    inf.avail_in = size - MARGIN;    
    inf.next_in = tmp;
    def.avail_out = size;
    def.next_out = blk;
    ret = recompress(&inf, &def);
    if (ret == Z_MEM_ERROR)
        quit("out of memory");
    assert(ret == Z_STREAM_END);     

     
    have = size - def.avail_out;
    if (fwrite(blk, 1, have, stdout) != have || ferror(stdout))
        quit("error writing output");

     
    free(tmp);
    ret = inflateEnd(&inf);
    assert(ret != Z_STREAM_ERROR);
    ret = deflateEnd(&def);
    assert(ret != Z_STREAM_ERROR);
    free(blk);
    fprintf(stderr,
            "%u bytes unused out of %u requested (%lu input)\n",
            size - have, size, def.total_in);
    return 0;
}
