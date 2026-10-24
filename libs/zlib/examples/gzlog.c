/*
 * gzlog.c
 * Copyright (C) 2004, 2008, 2012, 2016, 2019 Mark Adler, all rights reserved
 * For conditions of distribution and use, see copyright notice in gzlog.h
 * version 2.3, 25 May 2019
 */

  
                                                                              
                                                                            
                                                                               
                                                                           
                             

                                                                             
                                                                             
                                                                           
                                                                          
                                                                               
                                                  

                                                                            
                                                                               
                                                                          
   

  
                      

                          
                                            
                                                             
                                                                      
                                                                       
                                                                
                                                                            

                       
                                                                    
                                                             
                                                                    
                                                      
                 
                                           

                                                                               
                                                                               
                                                                             
                                                                              
                                                                           

                                                                             
                                                                       
                                                                            
                                                                             
                                                                               
                                                    

                                                                               
                                                                            
                                                                       
                                                                               
                                                                             
        

                                                                              
                                                                               
                                                                           
                                                                              
                                                                              
                                                                               
             

                                                                               
                                                                      
                                                                         
                                                                          
                                                                               
                                                                       
                                                         

                                                                            
                                                                        
                                                                      
                                                                            
                                                                            
                                                                         
                                                                           
                                                                           
                                                                           
                                         

                                                                             
                                                                        
                                                                       
                                                                          
                                                                              
                                                                            
                                                                         
                                                                             
                                                                              
                                                                            
                                                                              
                                                                             
                                                                             
                                                                               
           

                          
                                                                               
                                                                        
                                                                           
                                                                           
                                                                         
                                                                             
                                                                         
                                                                         
                                                                             
                                                                               
                                                                            
                                                                               
                                         
                                                                            
                                                                             
                                                                               
                                                    
                                                                              
                                                                         
                                                                              
                            
                                                                              
                                                                             
                                        
                                                                              
                                                                          
                                                                            
                                                                            
                                                                              
                                                                               
                                                                              
                                                                             
                                                                          
                                                                             
                                                                     
                                                                          
                                                                               
                                                                              
                                                                         
                                                                            
                               

                  
                                                                               
                                                                            
                                                                          
                                 
                                                                              
                                                                            
                                                         
                                                                          
                        
                              

                    
                                                                        
                                                
                                                                       
                                                                            
                                                                             
                                      
                                                                           
                                                                        
             
                                                                             
                                                                            
                                                                            
                                                                           
                                                                    
                                                                      
                             

                                   
                                                                               
                                                                          
                                                                    
                                                                          
                                                                            
                                                                          
                                                                             
                        
                                                                        
                                                                              
                                                                        
                                                                              
                                                                             
                                                                      
                                                                             
                                                                           
                                                                         
                                                       
                                                                       
                                              
                                                                           
                                                                              
                       
                             
                                    
                                                            

                      
                                                                               
                                                                             
                                                                           
                                                                              
                                                                              
                                                         
                     
                                     
                       
                                       
                      
                                       
                                                    
   

#include <sys/types.h>
#include <stdio.h>       
#include <stdlib.h>      
#include <string.h>      
#include <fcntl.h>       
#include <unistd.h>      
                         
#include <errno.h>       
#include <time.h>        
#include <sys/stat.h>    
#include <sys/time.h>    
#include "zlib.h"        

#include "gzlog.h"       

#define local static
typedef unsigned int uint;
typedef unsigned long ulong;

 
#ifdef GZLOG_DEBUG
    #include <setjmp.h>          
    jmp_buf gzlog_jump;          
    int gzlog_bail = 0;          
    int gzlog_count = -1;        
#   define BAIL(n) do { if (n == gzlog_bail && gzlog_count-- == 0) \
                            longjmp(gzlog_jump, gzlog_bail); } while (0)
#else
#   define BAIL(n)
#endif

 
#define PATIENCE 300

 
#define MAX_STORE 16

                                                                         
                                                                         
                                                                       
#define TRIGGER 1024

 
#define DICT 32768U

 
#define NO_OP 0
#define APPEND_OP 1
#define COMPRESS_OP 2
#define REPLACE_OP 3

 
#define PULL2(p) ((p)[0]+((uint)((p)[1])<<8))
#define PULL4(p) (PULL2(p)+((ulong)PULL2(p+2)<<16))
#define PULL8(p) (PULL4(p)+((off_t)PULL4(p+4)<<32))

 
#define PUT2(p,a) do {(p)[0]=a;(p)[1]=(a)>>8;} while(0)
#define PUT4(p,a) do {PUT2(p,a);PUT2(p+2,a>>16);} while(0)
#define PUT8(p,a) do {PUT4(p,a);PUT4(p+4,a>>32);} while(0)

 
#define LOGID "\106\035\172"     
struct log {
    char id[4];      
    int fd;          
    char *path;      
    char *end;       
    off_t first;     
    int back;        
    uint stored;     
    off_t last;      
    ulong ccrc;      
    ulong clen;      
    ulong tcrc;      
    ulong tlen;      
    time_t lock;     
};

 
local unsigned char log_gzhead[] = {
    0x1f, 0x8b,                  
    8,                           
    4,                           
    0, 0, 0, 0,                  
    0, 0xff,                     
    39, 0, 'a', 'p', 35, 0       
                                 
};

#define HEAD sizeof(log_gzhead)      

 
local unsigned char log_gzext[] = {
    52, 0, 0, 0, 0, 0, 0, 0,     
    52, 0, 0, 0, 0, 0, 0, 0,     
    0, 0, 0, 0, 0, 0, 0, 0,      
    0, 0, 0, 0, 0, 0, 0, 0,      
    0, 0,                        
    5                            
};

#define EXTRA sizeof(log_gzext)      

 
local unsigned char log_gzbody[] = {
    1, 0, 0, 0xff, 0xff,         
    0, 0, 0, 0,                  
    0, 0, 0, 0                   
};

#define BODY sizeof(log_gzbody)

                                                                            
                                                                            
                                                                         
                                                                           
                                                                              
                                                                               
                                                                             
                                                                        
                          
local int log_lock(struct log *log)
{
    int fd;
    struct stat st;

    strcpy(log->end, ".lock");
    while ((fd = open(log->path, O_CREAT | O_EXCL, 0644)) < 0) {
        if (errno != EEXIST)
            return -1;
        if (stat(log->path, &st) == 0 && time(NULL) - st.st_mtime > PATIENCE) {
            unlink(log->path);
            continue;
        }
        sleep(2);        
    }
    close(fd);
    if (stat(log->path, &st) == 0)
        log->lock = st.st_mtime;
    return 0;
}

                                                                              
                                                                             
                                    
local void log_touch(struct log *log)
{
    struct stat st;

    strcpy(log->end, ".lock");
    utimes(log->path, NULL);
    if (stat(log->path, &st) == 0)
        log->lock = st.st_mtime;
}

                                                                           
                                                                    
local int log_check(struct log *log)
{
    struct stat st;

    strcpy(log->end, ".lock");
    if (stat(log->path, &st) || st.st_mtime != log->lock)
        return 1;
    log_touch(log);
    return 0;
}

 
local void log_unlock(struct log *log)
{
    if (log_check(log))
        return;
    strcpy(log->end, ".lock");
    unlink(log->path);
    log->lock = 0;
}

                                                                              
                                                                               
                                                                               
                                                                            
                              
local int log_head(struct log *log)
{
    int op;
    unsigned char buf[HEAD + EXTRA];

    if (lseek(log->fd, 0, SEEK_SET) < 0 ||
        read(log->fd, buf, HEAD + EXTRA) != HEAD + EXTRA ||
        memcmp(buf, log_gzhead, HEAD)) {
        return -1;
    }
    log->first = PULL8(buf + HEAD);
    log->last = PULL8(buf + HEAD + 8);
    log->ccrc = PULL4(buf + HEAD + 16);
    log->clen = PULL4(buf + HEAD + 20);
    log->tcrc = PULL4(buf + HEAD + 24);
    log->tlen = PULL4(buf + HEAD + 28);
    log->stored = PULL2(buf + HEAD + 32);
    log->back = 3 + (buf[HEAD + 34] & 7);
    op = (buf[HEAD + 34] >> 3) & 3;
    return op;
}

                                                                               
                                                                             
                                                                          
                                                                            
                                                                     
local int log_mark(struct log *log, int op)
{
    int ret;
    unsigned char ext[EXTRA];

    PUT8(ext, log->first);
    PUT8(ext + 8, log->last);
    PUT4(ext + 16, log->ccrc);
    PUT4(ext + 20, log->clen);
    PUT4(ext + 24, log->tcrc);
    PUT4(ext + 28, log->tlen);
    PUT2(ext + 32, log->stored);
    ext[34] = log->back - 3 + (op << 3);
    fsync(log->fd);
    ret = lseek(log->fd, HEAD, SEEK_SET) < 0 ||
          write(log->fd, ext, EXTRA) != EXTRA ? -1 : 0;
    fsync(log->fd);
    return ret;
}

                                                                               
                                                                           
                                                                             
                                                                              
                                                         
local int log_last(struct log *log, int last)
{
    int back, len, mask;
    unsigned char buf[6];

     
    back = log->last == log->first ? log->back : 8;
    len = back > 8 ? 2 : 1;                  
    mask = 0x80 >> ((back - 1) & 7);         

                                                                            
                                                                           
                                          
    buf[0] = 0;
    if (back != 8 && (lseek(log->fd, log->last - len, SEEK_SET) < 0 ||
                      read(log->fd, buf, 1) != 1))
        return -1;

                                                                        
                                                                          
                                                                         
                                                            
    buf[1] = 0;
    buf[2 - len] = (*buf & (mask - 1)) + (last ? mask : 0);

                                                                        
                                                     
    PUT2(buf + 2, log->stored);
    PUT2(buf + 4, log->stored ^ 0xffff);
    return lseek(log->fd, log->last - len, SEEK_SET) < 0 ||
           write(log->fd, buf + 2 - len, len + 4) != len + 4 ||
           lseek(log->fd, log->stored, SEEK_CUR) < 0 ? -1 : 0;
}

                                                                               
                                                                              
                                                                        
                                                                             
                      
local int log_append(struct log *log, unsigned char *data, size_t len)
{
    uint put;
    off_t end;
    unsigned char buf[8];

                                                                    
                                                                          
               
    if (log_last(log, 1))
        return -1;

                                                                              
                                                              
    while (len) {
         
        put = (MAX_STORE << 10) - log->stored;
        if (put > len)
            put = (uint)len;
        if (put) {
            if (write(log->fd, data, put) != put)
                return -1;
            BAIL(1);
            log->tcrc = crc32(log->tcrc, data, put);
            log->tlen += put;
            log->stored += put;
            data += put;
            len -= put;
        }

         
        if (len) {
             
            if (log_last(log, 0))
                return -1;

             
            log->last += 4 + log->stored + 1;
            log->stored = 0;
        }

         
        if (log_last(log, 1))
            return -1;
        BAIL(2);
    }

                                                                             
                                                                        
    PUT4(buf, log->tcrc);
    PUT4(buf + 4, log->tlen);
    if (write(log->fd, buf, 8) != 8 ||
        (end = lseek(log->fd, 0, SEEK_CUR)) < 0 || ftruncate(log->fd, end))
        return -1;

     
    if (log_mark(log, NO_OP))
        return -1;
    strcpy(log->end, ".add");
    unlink(log->path);           
    return 0;
}

                                                                             
                                                                               
                                                                         
                                                                          
                                                                            
                                                                            
                                              
local int log_replace(struct log *log)
{
    int ret;
    char *dest;

     
    strcpy(log->end, ".add");
    unlink(log->path);          
    BAIL(3);

     
    strcpy(log->end, ".dict");
    dest = malloc(strlen(log->path) + 1);
    if (dest == NULL)
        return -2;
    strcpy(dest, log->path);
    strcpy(log->end, ".temp");
    ret = rename(log->path, dest);
    free(dest);
    if (ret && errno != ENOENT)
        return -1;
    BAIL(4);

     
    return log_mark(log, NO_OP);
}

                                                                       
                                                                            
                                                                         
                                                                    
                                                                          
                                                                           
                                                                         
                                                                           
                                                                         
                         
local int log_compress(struct log *log, unsigned char *data, size_t len)
{
    int fd;
    uint got, max;
    ssize_t dict;
    off_t end;
    z_stream strm;
    unsigned char buf[DICT];

     
    if (len) {
         
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8,
                         Z_DEFAULT_STRATEGY) != Z_OK)
            return -2;

         
        strcpy(log->end, ".dict");
        fd = open(log->path, O_RDONLY, 0);
        if (fd >= 0) {
            dict = read(fd, buf, DICT);
            close(fd);
            if (dict < 0) {
                deflateEnd(&strm);
                return -1;
            }
            if (dict)
                deflateSetDictionary(&strm, buf, (uint)dict);
        }
        log_touch(log);

                                                                         
                                                                    
        if (lseek(log->fd, log->first - (log->back > 8 ? 2 : 1),
                SEEK_SET) < 0 ||
            read(log->fd, buf, 1) != 1 || lseek(log->fd, -1, SEEK_CUR) < 0) {
            deflateEnd(&strm);
            return -1;
        }
        deflatePrime(&strm, (8 - log->back) & 7, *buf);

         
        strm.next_in = data;
        max = (((uint)0 - 1) >> 1) + 1;  
        do {
            strm.avail_in = len > max ? max : (uint)len;
            len -= strm.avail_in;
            do {
                strm.avail_out = DICT;
                strm.next_out = buf;
                deflate(&strm, len ? Z_NO_FLUSH : Z_PARTIAL_FLUSH);
                got = DICT - strm.avail_out;
                if (got && write(log->fd, buf, got) != got) {
                    deflateEnd(&strm);
                    return -1;
                }
                log_touch(log);
            } while (strm.avail_out == 0);
        } while (len);
        deflateEnd(&strm);
        BAIL(5);

                                                                               
                                                                             
                                                                          
                                                 
        if ((log->first = lseek(log->fd, -1, SEEK_CUR)) < 0 ||
            read(log->fd, buf, 1) != 1)
            return -1;
        log->first++;
        if (*buf) {
            log->back = 1;
            while ((*buf & ((uint)1 << (8 - log->back++))) == 0)
                ;        
        }
        else
            log->back = 10;

         
        log->ccrc = log->tcrc;
        log->clen = log->tlen;
    }
    else {
         
        log->tcrc = log->ccrc;
        log->tlen = log->clen;
    }

     
    log->last = log->first;
    log->stored = 0;
    PUT4(buf, log->tcrc);
    PUT4(buf + 4, log->tlen);
    if (log_last(log, 1) || write(log->fd, buf, 8) != 8 ||
        (end = lseek(log->fd, 0, SEEK_CUR)) < 0 || ftruncate(log->fd, end))
        return -1;
    BAIL(6);

     
    if (log_mark(log, REPLACE_OP))
        return -1;

     
    return log_replace(log);
}

 
local void log_log(struct log *log, int op, char *record)
{
    time_t now;
    FILE *rec;

    now = time(NULL);
    strcpy(log->end, ".repairs");
    rec = fopen(log->path, "a");
    if (rec == NULL)
        return;
    fprintf(rec, "%.24s %s recovery: %s\n", ctime(&now), op == APPEND_OP ?
            "append" : (op == COMPRESS_OP ? "compress" : "replace"), record);
    fclose(rec);
    return;
}

                                                                              
                                                                            
                                                                             
                         
local int log_recover(struct log *log, int op)
{
    int fd, ret = 0;
    unsigned char *data = NULL;
    size_t len = 0;
    struct stat st;

     
    log_log(log, op, "start");

     
    if (op == APPEND_OP || op == COMPRESS_OP) {
        strcpy(log->end, ".add");
        if (stat(log->path, &st) == 0 && st.st_size) {
            len = (size_t)(st.st_size);
            if ((off_t)len != st.st_size ||
                    (data = malloc(st.st_size)) == NULL) {
                log_log(log, op, "allocation failure");
                return -2;
            }
            if ((fd = open(log->path, O_RDONLY, 0)) < 0) {
                free(data);
                log_log(log, op, ".add file read failure");
                return -1;
            }
            ret = (size_t)read(fd, data, len) != len;
            close(fd);
            if (ret) {
                free(data);
                log_log(log, op, ".add file read failure");
                return -1;
            }
            log_log(log, op, "loaded .add file");
        }
        else
            log_log(log, op, "missing .add file!");
    }

     
    switch (op) {
    case APPEND_OP:
        ret = log_append(log, data, len);
        break;
    case COMPRESS_OP:
        ret = log_compress(log, data, len);
        break;
    case REPLACE_OP:
        ret = log_replace(log);
    }

     
    log_log(log, op, ret ? "failure" : "complete");

     
    if (data != NULL)
        free(data);
    return ret;
}

 
local void log_close(struct log *log)
{
    if (log->fd >= 0)
        close(log->fd);
    log->fd = -1;
    log_unlock(log);
}

                                                                           
                                                                         
                                                                               
                                                                             
                                                                            
                                                                              
                                                                           
                                                                         
                                           
local int log_open(struct log *log)
{
    int op;

                                                                        
                                                
    if (log->fd >= 0)
        close(log->fd);
    log->fd = -1;

     
    if (log_lock(log) < 0)
        return -1;

     
    strcpy(log->end, ".gz");
    log->fd = open(log->path, O_RDWR | O_CREAT, 0644);
    if (log->fd < 0) {
        log_close(log);
        return -1;
    }

     
    if (lseek(log->fd, 0, SEEK_END) == 0) {
        if (write(log->fd, log_gzhead, HEAD) != HEAD ||
            write(log->fd, log_gzext, EXTRA) != EXTRA ||
            write(log->fd, log_gzbody, BODY) != BODY) {
            log_close(log);
            return -1;
        }
        strcpy(log->end, ".dict");
        unlink(log->path);
    }

     
    if ((op = log_head(log)) < 0) {
        log_close(log);
        return -1;
    }

     
    if (op != NO_OP && log_recover(log, op)) {
        log_close(log);
        return -1;
    }

     
    log_touch(log);
    return 0;
}

 
gzlog *gzlog_open(char *path)
{
    size_t n;
    struct log *log;

     
    if (path == NULL || *path == 0)
        return NULL;

     
    log = malloc(sizeof(struct log));
    if (log == NULL)
        return NULL;
    strcpy(log->id, LOGID);
    log->fd = -1;

     
    n = strlen(path);
    log->path = malloc(n + 9);               
    if (log->path == NULL) {
        free(log);
        return NULL;
    }
    strcpy(log->path, path);
    log->end = log->path + n;

                                                                 
                                      
    if (log_open(log)) {
        free(log->path);
        free(log);
        return NULL;
    }

     
    return log;
}

                                  
               
                                            
                                
                                      
int gzlog_compress(gzlog *logd)
{
    int fd, ret;
    uint block;
    size_t len, next;
    unsigned char *data, buf[5];
    struct log *log = logd;

     
    if (log == NULL || strcmp(log->id, LOGID))
        return -3;

                                                                         
                                                                         
                   
    if (log_check(log) && log_open(log))
        return -1;

     
    len = ((size_t)(log->last - log->first) & ~(((size_t)1 << 10) - 1)) +
          log->stored;
    if ((data = malloc(len)) == NULL)
        return -2;

     
    do {
         
        if (lseek(log->fd, log->first - 1, SEEK_SET) < 0)
            break;
        next = 0;
        while (next < len) {
            if (read(log->fd, buf, 5) != 5)
                break;
            block = PULL2(buf + 1);
            if (next + block > len ||
                read(log->fd, (char *)data + next, block) != block)
                break;
            next += block;
        }
        if (lseek(log->fd, 0, SEEK_CUR) != log->last + 4 + log->stored)
            break;
        log_touch(log);

         
        strcpy(log->end, ".add");
        fd = open(log->path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
            break;
        ret = (size_t)write(fd, data, len) != len;
        if (ret | close(fd))
            break;
        log_touch(log);

         
        strcpy(log->end, ".temp");
        fd = open(log->path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
            break;
        next = DICT > len ? len : DICT;
        ret = (size_t)write(fd, (char *)data + len - next, next) != next;
        if (ret | close(fd))
            break;
        log_touch(log);

         
        log->last = log->first;
        log->stored = 0;
        if (log_mark(log, COMPRESS_OP))
            break;
        BAIL(7);

         
        ret = log_compress(log, data, len);
        free(data);
        return ret;
    } while (0);

     
    free(data);
    return -1;
}

                               
               
                                            
                                
                                      
int gzlog_write(gzlog *logd, void *data, size_t len)
{
    int fd, ret;
    struct log *log = logd;

     
    if (log == NULL || strcmp(log->id, LOGID))
        return -3;
    if (data == NULL || len <= 0)
        return 0;

                                                                         
                                                                         
                   
    if (log_check(log) && log_open(log))
        return -1;

     
    strcpy(log->end, ".add");
    fd = open(log->path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return -1;
    ret = (size_t)write(fd, data, len) != len;
    if (ret | close(fd))
        return -1;
    log_touch(log);

     
    if (log_mark(log, APPEND_OP))
        return -1;
    BAIL(8);

     
    if (log_append(log, data, len))
        return -1;

     
    if (((log->last - log->first) >> 10) + (log->stored >> 10) < TRIGGER)
        return 0;

     
    return gzlog_compress(log);
}

                               
         
                                      
int gzlog_close(gzlog *logd)
{
    struct log *log = logd;

     
    if (log == NULL || strcmp(log->id, LOGID))
        return -3;

     
    log_close(log);

     
    if (log->path != NULL)
        free(log->path);
    strcpy(log->id, "bad");
    free(log);
    return 0;
}
