// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2014-2023 by Sonic Team Junior.
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



#include "../v_video.h"

boolean OglSdlSurface(INT32 w, INT32 h);

void OglSdlSetVSync(INT32 mode, boolean report);
boolean OglSdlCompositorProvidesPacing(void);
void OglSdlFinishUpdate(INT32 vidwait);
void OglSdlPresentSoftware(const void *pixels, INT32 width, INT32 height, INT32 vidwait);
void OglSdlDestroySoftwarePresenter(void);

extern SDL_Renderer *renderer;
extern SDL_GLContext sdlglcontext;
extern Uint16      realwidth;
extern Uint16      realheight;

#ifdef _CREATE_DLL_
EXPORT void HWRAPI( OglSdlSetPalette ) (RGBA_t *palette);
#endif
