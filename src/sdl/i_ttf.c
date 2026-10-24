// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2011 by Callum Dickinson.
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



#if defined(HAVE_SDL) && defined(HAVE_TTF)
#include "SDL.h"
#include "SDL_ttf.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../netcode/d_netfil.h"
#include "../filesrch.h"
#include "i_ttf.h"


#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
#define FONTSEARCHPATH1 "/usr/share/fonts"
#define FONTSEARCHPATH2 "/usr/local/share/fonts"
#define FONTSEARCHPATH3 "/usr/games/SRB2"
#define FONTSEARCHPATH4 "/usr/local/games/SRB2"
#define FONTSEARCHPATH5 "/usr/local/share/games/SRB2"
#else
#define FONTSEARCHPATH1 "."
#endif

#define FONTHANDLE -1


SDL_Surface *TTFSurface = NULL;
SDL_Surface *TTFRendSurface = NULL;

SDL_Rect TTFRect;


SDL_Rect TTFRectCheck;

videoResolution res;

char textbuffer[8192];


static char *searchFont(const char *fontsearchDir)
{
	static char tempsw[256] = "";
	filestatus_t fstemp;

	strcpy(tempsw, FONTFILE);
	fstemp = filesearch(tempsw, fontsearchDir, NULL, true, 20);
	if (fstemp == FS_FOUND)
	{
		return tempsw;
	}
	return NULL;
}


INT32 I_TTFLoadFont(const char *file, UINT32 ptsize)
{
	TTF_Font *tmpfont = NULL;
	float fontsize;


	if (currentfont)
	{
		TTF_CloseFont(currentfont);
	}


	fontsize = (ptsize * 0.005f) * (res.width - res.height);

	tmpfont = TTF_OpenFont(file, fontsize);

	if (!tmpfont)
		return FONTHANDLE;


	currentfont = tmpfont;


	currentfontpoint = ptsize;


	currentfontstyle = TTF_GetFontStyle(currentfont);
	TTF_SetFontStyle(currentfont, currentfontstyle);


#if SDL_TTF_VERSION_ATLEAST(2,0,10)
	currentfontkerning = TTF_GetFontKerning(currentfont);
	TTF_SetFontKerning(currentfont, currentfontkerning);

	currentfonthinting = TTF_GetFontHinting(currentfont);
	TTF_SetFontHinting(currentfont, currentfonthinting);

	currentfontoutline = TTF_GetFontOutline(currentfont);
	TTF_SetFontOutline(currentfont, currentfontoutline);
#endif

	return 0;
}

static void I_TTFRendSurface(const char *textmsg, TTF_Font *font, TextQuality quality, SDL_Color fontfgcolor, SDL_Color fontbgcolor)
{



	switch (quality)
	{
		case solid:
			TTFRendSurface = TTF_RenderText_Solid(font, textmsg, fontfgcolor);
			break;



		case shaded:
			TTFRendSurface = TTF_RenderText_Shaded(font, textmsg, fontfgcolor, fontbgcolor);
			break;

		case blended:
			TTFRendSurface = TTF_RenderText_Blended(font, textmsg, fontfgcolor);
			break;
	}


	SDL_BlitSurface(TTFRendSurface, NULL, TTFSurface, &TTFRect);
	SDL_Flip(TTFSurface);
}




void I_TTFDrawText(TTF_Font *font, TextQuality quality, INT32 fgR, INT32 fgG, INT32 fgB, INT32 fgA, INT32 bgR, INT32 bgG, INT32 bgB, INT32 bgA, const char *textmsg)
{


	char c, prevc = 0x0;

	char linebuffer[2];

	INT32 w, h;



	SDL_Color fontfgcolor = {fgR, fgG, fgB, fgA};
	SDL_Color fontbgcolor = {bgR, bgG, bgB, bgA};


	while (*textmsg != '\0')
	{

		c = *textmsg;

		if (c == '\n')
		{
			TTFRectCheck.x = 0;
			TTFRectCheck.y += (currentfontpoint + 1);
		}

		else
		{

			if (prevc == '\n')
			{
				if (textbuffer != NULL)
				{

					I_TTFRendSurface(textbuffer, font, quality, fontfgcolor, fontbgcolor);

					memset(textbuffer, '\0', 1);
				}
				TTFRect.x = TTFRectCheck.x;
				TTFRect.y = TTFRectCheck.y;
			}

			sprintf(textbuffer, "%s%c", textbuffer, c);

			sprintf(linebuffer, "%c", c);

			TTF_SizeText(currentfont, linebuffer, &w, &h);
			TTFRectCheck.x += w;
			if (TTFRectCheck.x >= res.width)
			{

				I_TTFRendSurface(textbuffer, font, quality, fontfgcolor, fontbgcolor);

				memset(textbuffer, '\0', 1);

				TTFRectCheck.x = 0;
				TTFRectCheck.y += (currentfontpoint + 1);

				TTFRect.x = TTFRectCheck.x;
				TTFRect.y = TTFRectCheck.y;
			}
		}

		textmsg++;

		prevc = c;
	}




	if (textbuffer != NULL)
	{

		I_TTFRendSurface(textbuffer, font, quality, fontfgcolor, fontbgcolor);

		memset(textbuffer, '\0', 1);

		TTFRect.x = TTFRectCheck.x;
		TTFRect.y = TTFRectCheck.y;
	}
}


void I_StartupTTF(UINT32 fontpointsize, Uint32 initflags, Uint32 vidmodeflags)
{
	char *fontpath = NULL;
	INT32 fontstatus = -1;
	res.width = 320;
	res.height = 200;
	bitsperpixel = 8;



	if (SDL_InitSubSystem(initflags) < 0)
		I_Error("Couldn't initialize SDL: %s\n", SDL_GetError());

	TTFSurface = SDL_SetVideoMode(res.width, res.height, bitsperpixel, vidmodeflags);
	if (!TTFSurface)
		I_Error("Couldn't set SDL Video resolution: %s\n", SDL_GetError());

	if (TTF_Init() < 0)
		I_Error("Couldn't start SDL_ttf: %s\n", TTF_GetError());


#ifdef FONTSEARCHPATH1
	fontpath = searchFont(FONTSEARCHPATH1);
	if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
#endif
#ifdef FONTSEARCHPATH2
	if (fontstatus < 0)
	{
		fontpath = searchFont(FONTSEARCHPATH2);
		if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
	}
#endif
#ifdef FONTSEARCHPATH3
	if (fontstatus < 0)
	{
		fontpath = searchFont(FONTSEARCHPATH3);
		if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
	}
#endif
#ifdef FONTSEARCHPATH4
	if (fontstatus < 0)
	{
		fontpath = searchFont(FONTSEARCHPATH4);
		if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
	}
#endif
#ifdef FONTSEARCHPATH5
	if (fontstatus < 0)
	{
		fontpath = searchFont(FONTSEARCHPATH5);
		if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
	}
#endif
#ifdef FONTSEARCHPATH6
	if (fontstatus < 0)
	{
		fontpath = searchFont(FONTSEARCHPATH6);
		if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
	}
#endif
#ifdef FONTSEARCHPATH7
	if (fontstatus < 0)
	{
		fontpath = searchFont(FONTSEARCHPATH7);
		if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
	}
#endif

	if (fontstatus < 0)
	{
		I_ShutdownTTF();
		CONS_Printf("Unable to find default font files! Not loading SDL_ttf\n");
	}
	else
	{

		SDL_version TTFcompiled;
		const SDL_version *TTFlinked;

		SDL_TTF_VERSION(&TTFcompiled);
		TTFlinked = TTF_Linked_Version();


		CONS_Printf("Compiled for SDL_ttf version: %d.%d.%d\n",
			    TTFcompiled.major, TTFcompiled.minor, TTFcompiled.patch);
		CONS_Printf("Linked with SDL_ttf version: %d.%d.%d\n",
			    TTFlinked->major, TTFlinked->minor, TTFlinked->patch);
	}
}

void I_ShutdownTTF(void)
{

	TTF_CloseFont(currentfont);

	TTF_Quit();


	SDL_FreeSurface(TTFSurface);
	SDL_FreeSurface(TTFRendSurface);
}
#endif
