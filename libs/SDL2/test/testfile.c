/*
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

 

 
#if defined(_MSC_VER) && !defined(_CRT_NONSTDC_NO_WARNINGS)
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "SDL.h"

 

#ifdef __IPHONEOS__
#define FBASENAME1  "../Documents/sdldata1"  
#define FBASENAME2  "../Documents/sdldata2"      
#else
#define FBASENAME1 "sdldata1"  
#define FBASENAME2 "sdldata2"  
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

static void
cleanup(void)
{
    unlink(FBASENAME1);
    unlink(FBASENAME2);
}

static void
rwops_error_quit(unsigned line, SDL_RWops *rwops)
{
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "testfile.c(%d): failed\n", line);
    if (rwops) {
        rwops->close(rwops);  
    }
    cleanup();
    exit(1);  
}

#define RWOP_ERR_QUIT(x) rwops_error_quit(__LINE__, (x))

int main(int argc, char *argv[])
{
    SDL_RWops *rwops = NULL;
    char test_buf[30];

     
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    cleanup();

     

    rwops = SDL_RWFromFile(NULL, NULL);
    if (rwops) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops = SDL_RWFromFile(NULL, "ab+");
    if (rwops) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops = SDL_RWFromFile(NULL, "sldfkjsldkfj");
    if (rwops) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops = SDL_RWFromFile("something", "");
    if (rwops) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops = SDL_RWFromFile("something", NULL);
    if (rwops) {
        RWOP_ERR_QUIT(rwops);
    }
    SDL_Log("test1 OK\n");

     
                                               
                                                                         

       
    rwops = SDL_RWFromFile(FBASENAME2, "rb");  
    if (rwops) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops = SDL_RWFromFile(FBASENAME2, "rb+");  
    if (rwops) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops = SDL_RWFromFile(FBASENAME2, "wb");
    if (rwops == NULL) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops->close(rwops);
    unlink(FBASENAME2);
    rwops = SDL_RWFromFile(FBASENAME2, "wb+");
    if (rwops == NULL) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops->close(rwops);
    unlink(FBASENAME2);
    rwops = SDL_RWFromFile(FBASENAME2, "ab");
    if (rwops == NULL) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops->close(rwops);
    unlink(FBASENAME2);
    rwops = SDL_RWFromFile(FBASENAME2, "ab+");
    if (rwops == NULL) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops->close(rwops);
    unlink(FBASENAME2);
    SDL_Log("test2 OK\n");

                                                     
                                              
       
    rwops = SDL_RWFromFile(FBASENAME1, "wb");  
    if (rwops == NULL) {
        RWOP_ERR_QUIT(rwops);
    }
    if (1 != rwops->write(rwops, "1234567890", 10, 1)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (10 != rwops->write(rwops, "1234567890", 1, 10)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (7 != rwops->write(rwops, "1234567", 1, 7)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->seek(rwops, 0L, RW_SEEK_SET)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->read(rwops, test_buf, 1, 1)) {
        RWOP_ERR_QUIT(rwops);  
    }

    rwops->close(rwops);

    rwops = SDL_RWFromFile(FBASENAME1, "rb");  
    if (rwops == NULL) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->seek(rwops, 0L, RW_SEEK_SET)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (20 != rwops->seek(rwops, -7, RW_SEEK_END)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (7 != rwops->read(rwops, test_buf, 1, 7)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (SDL_memcmp(test_buf, "1234567", 7) != 0) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->read(rwops, test_buf, 1, 1)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->read(rwops, test_buf, 10, 100)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->seek(rwops, -27, RW_SEEK_CUR)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (2 != rwops->read(rwops, test_buf, 10, 3)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (SDL_memcmp(test_buf, "12345678901234567890", 20) != 0) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->write(rwops, test_buf, 1, 1)) {
        RWOP_ERR_QUIT(rwops);  
    }

    rwops->close(rwops);

     
    rwops = SDL_RWFromFile(FBASENAME1, "wb+");  
    if (rwops == NULL) {
        RWOP_ERR_QUIT(rwops);
    }
    if (1 != rwops->write(rwops, "1234567890", 10, 1)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (10 != rwops->write(rwops, "1234567890", 1, 10)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (7 != rwops->write(rwops, "1234567", 1, 7)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->seek(rwops, 0L, RW_SEEK_SET)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (1 != rwops->read(rwops, test_buf, 1, 1)) {
        RWOP_ERR_QUIT(rwops);  
    }

    if (0 != rwops->seek(rwops, 0L, RW_SEEK_SET)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (20 != rwops->seek(rwops, -7, RW_SEEK_END)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (7 != rwops->read(rwops, test_buf, 1, 7)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (SDL_memcmp(test_buf, "1234567", 7) != 0) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->read(rwops, test_buf, 1, 1)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->read(rwops, test_buf, 10, 100)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->seek(rwops, -27, RW_SEEK_CUR)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (2 != rwops->read(rwops, test_buf, 10, 3)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (SDL_memcmp(test_buf, "12345678901234567890", 20) != 0) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops->close(rwops);
    SDL_Log("test3 OK\n");

     
    rwops = SDL_RWFromFile(FBASENAME1, "rb+");  
    if (rwops == NULL) {
        RWOP_ERR_QUIT(rwops);
    }
    if (1 != rwops->write(rwops, "1234567890", 10, 1)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (10 != rwops->write(rwops, "1234567890", 1, 10)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (7 != rwops->write(rwops, "1234567", 1, 7)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->seek(rwops, 0L, RW_SEEK_SET)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (1 != rwops->read(rwops, test_buf, 1, 1)) {
        RWOP_ERR_QUIT(rwops);  
    }

    if (0 != rwops->seek(rwops, 0L, RW_SEEK_SET)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (20 != rwops->seek(rwops, -7, RW_SEEK_END)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (7 != rwops->read(rwops, test_buf, 1, 7)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (SDL_memcmp(test_buf, "1234567", 7) != 0) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->read(rwops, test_buf, 1, 1)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->read(rwops, test_buf, 10, 100)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->seek(rwops, -27, RW_SEEK_CUR)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (2 != rwops->read(rwops, test_buf, 10, 3)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (SDL_memcmp(test_buf, "12345678901234567890", 20) != 0) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops->close(rwops);
    SDL_Log("test4 OK\n");

     
    rwops = SDL_RWFromFile(FBASENAME1, "ab+");  
    if (rwops == NULL) {
        RWOP_ERR_QUIT(rwops);
    }
    if (1 != rwops->write(rwops, "1234567890", 10, 1)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (10 != rwops->write(rwops, "1234567890", 1, 10)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (7 != rwops->write(rwops, "1234567", 1, 7)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->seek(rwops, 0L, RW_SEEK_SET)) {
        RWOP_ERR_QUIT(rwops);
    }

    if (1 != rwops->read(rwops, test_buf, 1, 1)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->seek(rwops, 0L, RW_SEEK_SET)) {
        RWOP_ERR_QUIT(rwops);
    }

    if (20 + 27 != rwops->seek(rwops, -7, RW_SEEK_END)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (7 != rwops->read(rwops, test_buf, 1, 7)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (SDL_memcmp(test_buf, "1234567", 7) != 0) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->read(rwops, test_buf, 1, 1)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (0 != rwops->read(rwops, test_buf, 10, 100)) {
        RWOP_ERR_QUIT(rwops);
    }

    if (27 != rwops->seek(rwops, -27, RW_SEEK_CUR)) {
        RWOP_ERR_QUIT(rwops);
    }

    if (0 != rwops->seek(rwops, 0L, RW_SEEK_SET)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (3 != rwops->read(rwops, test_buf, 10, 3)) {
        RWOP_ERR_QUIT(rwops);
    }
    if (SDL_memcmp(test_buf, "123456789012345678901234567123", 30) != 0) {
        RWOP_ERR_QUIT(rwops);
    }
    rwops->close(rwops);
    SDL_Log("test5 OK\n");
    cleanup();
    return 0;  
}
