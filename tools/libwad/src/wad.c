// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// libwad: Doom WAD format interface library.
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



#include "wad_static.h"




#define LUMPOFFSET 12





#define MAX_WADFILES 48

wad_t *wads[MAX_WADFILES];
wad_uint32_t wad_numwads;

wad_uint32_t wad_oldnumwads;
wad_uint32_t wad_removednum;

static inline void WAD_ChangeWADNum(wad_int32_t i, wad_t *wad)
{
	wads[i] = wad;
}

static inline void WAD_CacheWAD(wad_t *wad)
{
	WAD_ChangeWADNum(wad_numwads, wad);
	wad_oldnumwads = wad_numwads;
	wad_numwads++;
}

static void WAD_UncacheWAD(wad_t *wad)
{
	wad_uint32_t i;

	for (i = 0; i < wad_numwads; i++)
		if (WAD_WADByNum(i) == wad)
			break;

	WAD_ChangeWADNum(i, NULL);
	wad_removednum = i;

	for (; i < wad_numwads; i++)
	{
		if (WAD_WADByNum(i+1))
		{
			WAD_ChangeWADNum(i, WAD_WADByNum(i+1));
			WAD_ChangeWADNum(i+1, NULL);
		}
	}

	wad_oldnumwads = wad_numwads;
	wad_numwads--;
}

wad_uint32_t WAD_WADLoopAdvance(wad_uint32_t i)
{
	wad_uint32_t o;

	o = wad_oldnumwads;
	wad_oldnumwads = wad_numwads;

	return (o > wad_numwads && i <= wad_removednum) ?  i : i + 1;
}

wad_t *WAD_WADByNum(wad_uint16_t num)
{
	return (wads[num]) ? (wad_t *)wads[num] : NULL;
}

wad_t *WAD_WADByFileName(const char *filename)
{
	wad_uint32_t i;
	wad_t *w;

	for (i = 0; i < wad_numwads; i = WAD_WADLoopAdvance(i))
	{
		w = WAD_WADByNum(i);
		if (!strcmp(filename, w->filename))
			return w;
	}

	return NULL;
}












wad_t *WAD_OpenWAD(const char *filename)
{
	size_t i;

	wad_t *wad = NULL;


	FILE *file;			 
	wadheader_t *header;		 
	lump_t *l, *lumps = NULL;	 

	wad_uint64_t position;
	lumpdir_t *lumpdir;
	lump_t *ln = NULL;





	if (wad_numwads >= MAX_WADFILES)
		return NULL;




	if (!(file = fopen(filename, "rb")))
		return NULL;




	header = malloc(sizeof(*header));
	if (fread(header, 1, sizeof(*header), file) < sizeof(*header))
		return NULL;


	if (memcmp(header->id, "IWAD", 4) && memcmp(header->id, "PWAD", 4) && memcmp(header->id, "SDLL", 4))
	{
		WAD_FreeMemory(file, header, NULL, NULL, NULL);
		return NULL;
	}


	header->numlumps = WAD_INT32_Endianness(header->numlumps);
	header->lumpdir = WAD_INT32_Endianness(header->lumpdir);




	i = header->numlumps * sizeof(*lumpdir);
	lumpdir = malloc(i);

	if (fseek(file, header->lumpdir, SEEK_SET) < 0 || fread(lumpdir, i, 1, file) < 1)
	{
		WAD_FreeMemory(file, header, lumpdir, NULL, NULL);
		return NULL;
	}




	for (i = 0; i < header->numlumps; i++, lumpdir++)
	{
		if (!(l = malloc(sizeof(*l))))
		{
			WAD_FreeMemory(file, header, lumpdir, lumps, NULL);
			return NULL;
		}

		position = WAD_INT32_Endianness(lumpdir->position);

		strncpy(l->name, lumpdir->name, 8);
		l->size = l->disksize = WAD_INT32_Endianness(lumpdir->size);
		l->compressed = false;  

		if (l->disksize)
		{
			fseek(file, position, SEEK_SET);
			if (!(l->data = malloc(l->disksize)) || fread(l->data, l->disksize, 1, file) < 1)
			{
				WAD_FreeMemory(file, header, lumpdir, lumps, NULL);

				if (!l->data)
					return NULL;
				else
					return NULL;
			}
		}

		l->num = i;
		l->next = NULL;
		if (!lumps)
		{
			l->prev = NULL;
			ln = lumps = l;
		}
		else
		{
			l->prev = ln;
			ln = ln->next = l;
		}
	}





	if (!(wad = malloc(sizeof(*wad))))
	{
		WAD_FreeMemory(file, header, NULL, lumps, NULL);
		return NULL;
	}
	wad->filename = (char *)filename;

	wad->header = header;
	wad->compressed = false;

	wad->lumps = lumps;

	for (l = wad->lumps; l; l = l->next)
		l->wad = wad;

	fclose(file);




	WAD_CacheWAD(wad);




	return wad;
}











wad_int32_t WAD_SaveWAD(wad_t *wad)
{
	lump_t *l;
	FILE *file;
	char *newfilename;
	wad_int64_t diroffset, lumpoffset;

	if (!wad)
		return 1;

	newfilename = WAD_VA("%s%s", wad->filename, ".bak");

	remove(newfilename);
	rename(wad->filename, newfilename);

	if (!(file = fopen(wad->filename, "wb")))
		return 2;




	if(fwrite(wad->header->id, 4, 1, file) < 1)
		return 3;




	if (fwrite(&wad->header->numlumps, 4, 1, file) < 1)
		return 4;





	if (fwrite(&wad->header->numlumps, 4, 1, file) < 1)
		return 5;




	for (l = wad->lumps; l; l = l->next)
	{


		if (!l->data)
			continue;


		if (fwrite(l->data, l->disksize, 1, file) < 1)
			return 6;
	}




	diroffset = ftell(file);




	lumpoffset = LUMPOFFSET;




	for (l = wad->lumps; l; l = l->next)
	{

		if (fwrite(&lumpoffset, 4, 1, file) < 1)
			return 7;


		if (fwrite(&(l->disksize), 4, 1, file) < 1)
			return 8;


		if (fwrite(&(l->name), 8, 1, file) < 1)
			return 9;


		lumpoffset += l->disksize;
	}




	fseek(file, 8, SEEK_SET);
	if (fwrite(&diroffset, 4, 1, file) < 1)
		return 10;




	return 0;
}

inline wad_int32_t WAD_SaveWADByNum(wad_uint16_t num)
{
	return WAD_SaveWAD(WAD_WADByNum(num));
}

inline wad_int32_t WAD_SaveWADByFileName(const char *filename)
{
	return WAD_SaveWAD(WAD_WADByFileName(filename));
}

inline wad_int32_t WAD_SaveCloseWAD(wad_t *wad)
{
	wad_int32_t ret;

	if (!(ret = WAD_SaveWAD(wad)))
		return ret;

	WAD_CloseWAD(wad);
	return ret;
}

inline wad_int32_t WAD_SaveCloseWADByNum(wad_uint16_t num)
{
	return WAD_SaveCloseWAD(WAD_WADByNum(num));
}

inline wad_int32_t WAD_SaveCloseWADByFileName(const char *filename)
{
	return WAD_SaveCloseWAD(WAD_WADByFileName(filename));
}

void WAD_CloseWAD(wad_t *wad)
{
	if (!wad)
		return;




	WAD_FreeMemory(NULL, wad->header, NULL, wad->lumps, wad);




	WAD_UncacheWAD(wad);
}

inline void WAD_CloseWADByNum(wad_uint16_t num)
{
	WAD_CloseWAD(WAD_WADByNum(num));
}

inline void WAD_CloseWADByFileName(const char *name)
{
	WAD_CloseWAD(WAD_WADByFileName(name));
}
