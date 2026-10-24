/*
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

 

#include <stdio.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "SDL.h"

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "testutils.h"

static struct
{
    SDL_AudioSpec spec;
    Uint8 *sound;     
    Uint32 soundlen;  
} wave;

 
static void
quit(int rc)
{
    SDL_Quit();
    exit(rc);
}

static int done = 0;
void poked(int sig)
{
    done = 1;
}

void loop()
{
#ifdef __EMSCRIPTEN__
    if (done || (SDL_GetAudioStatus() != SDL_AUDIO_PLAYING)) {
        emscripten_cancel_main_loop();
    } else
#endif
    {
         
        const Uint32 queued = SDL_GetQueuedAudioSize(1);
        SDL_Log("Device has %u bytes queued.\n", (unsigned int)queued);
        if (queued <= 8192) {  
            if (SDL_QueueAudio(1, wave.sound, wave.soundlen) == 0) {
                SDL_Log("Device queued %u more bytes.\n", (unsigned int)wave.soundlen);
            } else {
                SDL_Log("Device FAILED to queue %u more bytes: %s\n", (unsigned int)wave.soundlen, SDL_GetError());
            }
        }
    }
}

int main(int argc, char *argv[])
{
    char *filename = NULL;

     
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

     
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    filename = GetResourceFilename(argc > 1 ? argv[1] : NULL, "sample.wav");

    if (filename == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s\n", SDL_GetError());
        quit(1);
    }

     
    if (SDL_LoadWAV(filename, &wave.spec, &wave.sound, &wave.soundlen) == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load %s: %s\n", filename, SDL_GetError());
        quit(1);
    }

    wave.spec.callback = NULL;  

#if HAVE_SIGNAL_H
     
#ifdef SIGHUP
    (void)signal(SIGHUP, poked);
#endif
    (void)signal(SIGINT, poked);
#ifdef SIGQUIT
    (void)signal(SIGQUIT, poked);
#endif
    (void)signal(SIGTERM, poked);
#endif  

     
    if (SDL_OpenAudio(&wave.spec, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open audio: %s\n", SDL_GetError());
        SDL_FreeWAV(wave.sound);
        quit(2);
    }

     

     
    SDL_PauseAudio(0);

    done = 0;

                                                                       
                                                                      
                                                     

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop, 0, 1);
#else
    while (!done && (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING)) {
        loop();

        SDL_Delay(100);  
    }
#endif

     
    SDL_CloseAudio();
    SDL_FreeWAV(wave.sound);
    SDL_free(filename);
    SDL_Quit();
    return 0;
}

 
