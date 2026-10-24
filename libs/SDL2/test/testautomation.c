/*
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "SDL.h"
#include "SDL_test.h"

#include "testautomation_suites.h"

static SDLTest_CommonState *state;

 
static void
quit(int rc)
{
    SDLTest_CommonQuit(state);
    exit(rc);
}

int main(int argc, char *argv[])
{
    int result;
    int testIterations = 1;
    Uint64 userExecKey = 0;
    char *userRunSeed = NULL;
    char *filter = NULL;
    int i, done;
    SDL_Event event;

     
    state = SDLTest_CommonCreateState(argv, SDL_INIT_VIDEO);
    if (state == NULL) {
        return 1;
    }

     
    state->num_windows = 0;

     
    for (i = 1; i < argc;) {
        int consumed;

        consumed = SDLTest_CommonArg(state, i);
        if (consumed == 0) {
            consumed = -1;
            if (SDL_strcasecmp(argv[i], "--iterations") == 0) {
                if (argv[i + 1]) {
                    testIterations = SDL_atoi(argv[i + 1]);
                    if (testIterations < 1) {
                        testIterations = 1;
                    }
                    consumed = 2;
                }
            } else if (SDL_strcasecmp(argv[i], "--execKey") == 0) {
                if (argv[i + 1]) {
                    (void)SDL_sscanf(argv[i + 1], "%" SDL_PRIu64, &userExecKey);
                    consumed = 2;
                }
            } else if (SDL_strcasecmp(argv[i], "--seed") == 0) {
                if (argv[i + 1]) {
                    userRunSeed = SDL_strdup(argv[i + 1]);
                    consumed = 2;
                }
            } else if (SDL_strcasecmp(argv[i], "--filter") == 0) {
                if (argv[i + 1]) {
                    filter = SDL_strdup(argv[i + 1]);
                    consumed = 2;
                }
            }
        }
        if (consumed < 0) {
            static const char *options[] = { "[--iterations #]", "[--execKey #]", "[--seed string]", "[--filter suite_name|test_name]", NULL };
            SDLTest_CommonLogUsage(state, argv[0], options);
            quit(1);
        }

        i += consumed;
    }

     
    if (!SDLTest_CommonInit(state)) {
        quit(2);
    }

     
    for (i = 0; i < state->num_windows; ++i) {
        SDL_Renderer *renderer = state->renderers[i];
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);
    }

     
    result = SDLTest_RunSuites(testSuites, userRunSeed, userExecKey, filter, testIterations);

     
    done = 0;
    for (i = 0; i < 100; i++) {
        while (SDL_PollEvent(&event)) {
            SDLTest_CommonEvent(state, &event, &done);
        }
        SDL_Delay(10);
    }

     
    SDL_free(userRunSeed);
    SDL_free(filter);

     
    quit(result);
    return result;
}

 
