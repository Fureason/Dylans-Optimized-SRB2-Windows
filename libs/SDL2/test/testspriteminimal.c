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

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "SDL.h"
#include "testutils.h"

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480
#define NUM_SPRITES   100
#define MAX_SPEED     1

static SDL_Texture *sprite;
static SDL_Rect positions[NUM_SPRITES];
static SDL_Rect velocities[NUM_SPRITES];
static int sprite_w, sprite_h;

SDL_Renderer *renderer;
int done;

 
static void
quit(int rc)
{
    SDL_Quit();
    exit(rc);
}

void MoveSprites()
{
    int i;
    int window_w = WINDOW_WIDTH;
    int window_h = WINDOW_HEIGHT;
    SDL_Rect *position, *velocity;

     
    SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xA0, 0xFF);
    SDL_RenderClear(renderer);

     
    for (i = 0; i < NUM_SPRITES; ++i) {
        position = &positions[i];
        velocity = &velocities[i];
        position->x += velocity->x;
        if ((position->x < 0) || (position->x >= (window_w - sprite_w))) {
            velocity->x = -velocity->x;
            position->x += velocity->x;
        }
        position->y += velocity->y;
        if ((position->y < 0) || (position->y >= (window_h - sprite_h))) {
            velocity->y = -velocity->y;
            position->y += velocity->y;
        }

         
        SDL_RenderCopy(renderer, sprite, NULL, position);
    }

     
    SDL_RenderPresent(renderer);
}

void loop()
{
    SDL_Event event;

     
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN) {
            done = 1;
        }
    }
    MoveSprites();
#ifdef __EMSCRIPTEN__
    if (done) {
        emscripten_cancel_main_loop();
    }
#endif
}

int main(int argc, char *argv[])
{
    SDL_Window *window;
    int i;

     
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer) < 0) {
        quit(2);
    }

    sprite = LoadTexture(renderer, "icon.bmp", SDL_TRUE, &sprite_w, &sprite_h);

    if (sprite == NULL) {
        quit(2);
    }

     
    srand((unsigned int)time(NULL));
    for (i = 0; i < NUM_SPRITES; ++i) {
        positions[i].x = rand() % (WINDOW_WIDTH - sprite_w);
        positions[i].y = rand() % (WINDOW_HEIGHT - sprite_h);
        positions[i].w = sprite_w;
        positions[i].h = sprite_h;
        velocities[i].x = 0;
        velocities[i].y = 0;
        while (!velocities[i].x && !velocities[i].y) {
            velocities[i].x = (rand() % (MAX_SPEED * 2 + 1)) - MAX_SPEED;
            velocities[i].y = (rand() % (MAX_SPEED * 2 + 1)) - MAX_SPEED;
        }
    }

     
    done = 0;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop, 0, 1);
#else
    while (!done) {
        loop();
    }
#endif
    quit(0);

    return 0;  
}

 
