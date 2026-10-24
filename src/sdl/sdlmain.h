// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2023 by Sonic Team Junior.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------



#ifndef __sdlmain__
#define __sdlmain__

extern SDL_bool consolevent;
extern SDL_bool framebuffer;

#include "../m_fixed.h"


#ifdef _MSC_VER
#define SDL2STUB() CONS_Printf("SDL2: stubbed: %s:%d\n", __FUNCTION__, __LINE__)
#else
#define SDL2STUB() CONS_Printf("SDL2: stubbed: %s:%d\n", __func__, __LINE__)
#endif


#define JOYSTICK_HOTPLUG

                               

                     
  
typedef struct SDLJoyInfo_s
{

	SDL_Joystick *dev;

	int oldjoy;

	int axises;

	INT32 scale;

	int buttons;

	int hats;

	int balls;

} SDLJoyInfo_t;

                                    
  
extern SDLJoyInfo_t JoyInfo;

                                 
  
#define SDL_JDEADZONE 153
#undef SDL_JDEADZONE

                                    
  
extern SDLJoyInfo_t JoyInfo2;


void I_ShutdownJoystick(void);
void I_ShutdownJoystick2(void);


INT32 I_GetJoystickDeviceIndex(SDL_Joystick *dev);

void I_GetConsoleEvents(void);

void SDLforceUngrabMouse(void);


extern SDL_Window *window;

#endif
