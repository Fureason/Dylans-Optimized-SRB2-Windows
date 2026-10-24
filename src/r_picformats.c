// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2005-2009 by Andrey "entryway" Budko.
// Copyright (C) 2018-2024 by Lactozilla.
// Copyright (C) 2019-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



                                                                              
                                                                             
                                                                    
#include "doomdef.h"

#include "byteptr.h"
#include "dehacked.h"
#include "i_video.h"
#include "r_data.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "r_textures.h"
#include "r_things.h"
#include "r_draw.h"
#include "v_video.h"
#include "z_zone.h"
#include "w_wad.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"
#endif

#ifdef HAVE_PNG

#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

#include "png.h"
#ifndef PNG_READ_SUPPORTED
#undef HAVE_PNG
#endif
#endif

#ifdef PICTURE_PNG_USELOOKUP
static colorlookup_t png_colorlookup;
#endif

                                           
   
                                         
                                      
                                           
                                     
                                                     
                                       
                                         
                                                               
                                                             
                                     
                                               
                            
                           
    
void *Picture_Convert(
	pictureformat_t informat, void *picture, pictureformat_t outformat,
	size_t insize, size_t *outsize,
	INT32 inwidth, INT32 inheight, INT32 inleftoffset, INT32 intopoffset,
	pictureflags_t flags)
{
	if (informat == PICFMT_NONE)
		I_Error("Picture_Convert: input format was PICFMT_NONE!");
	else if (outformat == PICFMT_NONE)
		I_Error("Picture_Convert: output format was PICFMT_NONE!");
	else if (informat == outformat)
		I_Error("Picture_Convert: input and output formats were the same!");

	if (Picture_IsPatchFormat(outformat))
	{
		void *converted = Picture_PatchConvert(informat, insize, picture, outformat, outsize, inwidth, inheight, inleftoffset, intopoffset, flags);
		if (converted == NULL)
			I_Error("Picture_Convert: conversion to patch did not result in a valid graphic");
		return converted;
	}
	else if (Picture_IsFlatFormat(outformat))
		return Picture_FlatConvert(informat, picture, outformat, outsize, inwidth, intopoffset, flags);
	else
		I_Error("Picture_Convert: unsupported input format!");

	return NULL;
}

static void *ReadPixelFunc_Patch(void *picture, pictureformat_t informat, INT32 x, INT32 y, INT32 inwidth, INT32 inheight, pictureflags_t flags)
{
	(void)inwidth;
	(void)inheight;
	return Picture_GetPatchPixel((patch_t*)picture, informat, x, y, flags);
}

static void *ReadPixelFunc_Flat_8bpp(void *picture, pictureformat_t informat, INT32 x, INT32 y, INT32 inwidth, INT32 inheight, pictureflags_t flags)
{
	(void)informat;
	(void)flags;
	(void)inheight;
	return (UINT8 *)picture + ((y * inwidth) + x);
}

static void *ReadPixelFunc_Flat_16bpp(void *picture, pictureformat_t informat, INT32 x, INT32 y, INT32 inwidth, INT32 inheight, pictureflags_t flags)
{
	(void)informat;
	(void)flags;
	(void)inheight;
	return (UINT16 *)picture + ((y * inwidth) + x);
}

static void *ReadPixelFunc_Flat_32bpp(void *picture, pictureformat_t informat, INT32 x, INT32 y, INT32 inwidth, INT32 inheight, pictureflags_t flags)
{
	(void)informat;
	(void)flags;
	(void)inheight;
	return (UINT32 *)picture + ((y * inwidth) + x);
}

static UINT8 GetAlphaFunc_32bpp(void *input, pictureflags_t flags)
{
	(void)flags;
	RGBA_t px = *(RGBA_t *)input;
	return px.s.alpha;
}

static UINT8 GetAlphaFunc_16bpp(void *input, pictureflags_t flags)
{
	(void)flags;
	UINT16 px = *(UINT16 *)input;
	return (px & 0xFF00) >> 8;
}

static UINT8 GetAlphaFunc_8bpp(void *input, pictureflags_t flags)
{
	UINT8 px = *(UINT8 *)input;
	if (px == TRANSPARENTPIXEL && (flags & PICFLAGS_USE_TRANSPARENTPIXEL))
		return 0;
	else
		return 255;
}


static void *WritePatchPixel_i32o32(void *ptr, void *input)
{
	RGBA_t px = *(RGBA_t *)input;
	WRITEUINT32(ptr, px.rgba);
	return ptr;
}


static void *WritePatchPixel_i16o32(void *ptr, void *input)
{
	RGBA_t px = pMasterPalette[*((UINT16 *)input) & 0xFF];
	WRITEUINT32(ptr, px.rgba);
	return ptr;
}


static void *WritePatchPixel_i8o32(void *ptr, void *input)
{
	RGBA_t px = pMasterPalette[*((UINT8 *)input) & 0xFF];
	WRITEUINT32(ptr, px.rgba);
	return ptr;
}


static void *WritePatchPixel_i32o16(void *ptr, void *input)
{
	RGBA_t in = *(RGBA_t *)input;
	UINT8 px = NearestColor(in.s.red, in.s.green, in.s.blue);
	WRITEUINT16(ptr, (0xFF00 | px));
	return ptr;
}


static void *WritePatchPixel_i16o16(void *ptr, void *input)
{
	WRITEUINT16(ptr, *(UINT16 *)input);
	return ptr;
}


static void *WritePatchPixel_i8o16(void *ptr, void *input)
{
	WRITEUINT16(ptr, (0xFF00 | (*(UINT8 *)input)));
	return ptr;
}


static void *WritePatchPixel_i32o8(void *ptr, void *input)
{
	RGBA_t in = *(RGBA_t *)input;
	UINT8 px = NearestColor(in.s.red, in.s.green, in.s.blue);
	WRITEUINT8(ptr, px);
	return ptr;
}


static void *WritePatchPixel_i16o8(void *ptr, void *input)
{
	UINT16 px = *(UINT16 *)input;
	WRITEUINT8(ptr, (px & 0xFF));
	return ptr;
}


static void *WritePatchPixel_i8o8(void *ptr, void *input)
{
	WRITEUINT8(ptr, *(UINT8 *)input);
	return ptr;
}

                                  
   
                                         
                                      
                                           
                                                     
                                       
                                         
                                                               
                                                             
                                     
                                               
    
void *Picture_PatchConvert(
	pictureformat_t informat, size_t insize, void *picture,
	pictureformat_t outformat, size_t *outsize,
	INT32 inwidth, INT32 inheight, INT32 inleftoffset, INT32 intopoffset,
	pictureflags_t flags)
{

	if (informat == PICFMT_DOOMPATCH && outformat == PICFMT_PATCH && flags == 0)
	{
		if (outsize != NULL)
			*outsize = sizeof(patch_t);
		return Patch_CreateFromDoomPatch(picture, insize);
	}

	INT32 outbpp = Picture_FormatBPP(outformat);
	INT32 inbpp = Picture_FormatBPP(informat);
	patch_t *inpatch = NULL;

	if (informat == PICFMT_NONE)
		I_Error("Picture_PatchConvert: input format was PICFMT_NONE!");
	else if (outformat == PICFMT_NONE)
		I_Error("Picture_PatchConvert: output format was PICFMT_NONE!");
	else if (Picture_IsDoomPatchFormat(outformat))
		I_Error("Picture_PatchConvert: cannot convert to Doom patch!");
	else if (informat == outformat)
		I_Error("Picture_PatchConvert: input and output formats were the same!");

	if (inbpp == PICDEPTH_NONE)
		I_Error("Picture_PatchConvert: unknown input bits per pixel!");
	if (outbpp == PICDEPTH_NONE)
		I_Error("Picture_PatchConvert: unknown output bits per pixel!");


	if (Picture_IsPatchFormat(informat))
	{
		inpatch = (patch_t *)picture;
		if (Picture_IsDoomPatchFormat(informat))
		{
			softwarepatch_t *doompatch = (softwarepatch_t *)picture;
			inwidth = SHORT(doompatch->width);
			inheight = SHORT(doompatch->height);
			inleftoffset = SHORT(doompatch->leftoffset);
			intopoffset = SHORT(doompatch->topoffset);
		}
		else
		{
			inwidth = inpatch->width;
			inheight = inpatch->height;
			inleftoffset = inpatch->leftoffset;
			intopoffset = inpatch->topoffset;
		}
	}

	void *(*readPixelFunc)(void *, pictureformat_t, INT32, INT32, INT32, INT32, pictureflags_t) = NULL;
	UINT8 (*getAlphaFunc)(void *, pictureflags_t) = NULL;
	void *(*writePatchPixel)(void *, void *) = NULL;

	if (Picture_IsPatchFormat(informat))
		readPixelFunc = ReadPixelFunc_Patch;
	else if (Picture_IsFlatFormat(informat))
	{
		switch (informat)
		{
			case PICFMT_FLAT32:
				readPixelFunc = ReadPixelFunc_Flat_32bpp;
				break;
			case PICFMT_FLAT16:
				readPixelFunc = ReadPixelFunc_Flat_16bpp;
				break;
			case PICFMT_FLAT:
				readPixelFunc = ReadPixelFunc_Flat_8bpp;
				break;
			default:
				I_Error("Picture_PatchConvert: unsupported flat input format!");
				break;
		}
	}
	else
		I_Error("Picture_PatchConvert: unsupported input format!");

	if (inbpp == PICDEPTH_32BPP)
		getAlphaFunc = GetAlphaFunc_32bpp;
	else if (inbpp == PICDEPTH_16BPP)
		getAlphaFunc = GetAlphaFunc_16bpp;
	else if (inbpp == PICDEPTH_8BPP)
		getAlphaFunc = GetAlphaFunc_8bpp;

	switch (outformat)
	{
		case PICFMT_PATCH32:
		case PICFMT_DOOMPATCH32:
		{
			if (inbpp == PICDEPTH_32BPP)
				writePatchPixel = WritePatchPixel_i32o32;
			else if (inbpp == PICDEPTH_16BPP)
				writePatchPixel = WritePatchPixel_i16o32;
			else  
				writePatchPixel = WritePatchPixel_i8o32;
			break;
		}
		case PICFMT_PATCH16:
		case PICFMT_DOOMPATCH16:
			if (inbpp == PICDEPTH_32BPP)
				writePatchPixel = WritePatchPixel_i32o16;
			else if (inbpp == PICDEPTH_16BPP)
				writePatchPixel = WritePatchPixel_i16o16;
			else  
				writePatchPixel = WritePatchPixel_i8o16;
			break;
		default:  
		{
			if (inbpp == PICDEPTH_32BPP)
				writePatchPixel = WritePatchPixel_i32o8;
			else if (inbpp == PICDEPTH_16BPP)
				writePatchPixel = WritePatchPixel_i16o8;
			else  
				writePatchPixel = WritePatchPixel_i8o8;
			break;
		}
	}

	                                                                            
                                                                             
                                                                               
                                                            
	patch_t *out = Patch_Create((INT16)inwidth, (INT16)inheight);

	out->width = inwidth;
	out->height = inheight;
	out->leftoffset = inleftoffset;
	out->topoffset = intopoffset;

	size_t max_pixels = out->width * out->height;
	unsigned num_posts = 0;

	out->columns = Z_Calloc(sizeof(column_t) * out->width, PU_PATCH_DATA, NULL);
	out->pixels = Z_Calloc(max_pixels * (outbpp / 8), PU_PATCH_DATA, NULL);
	out->posts = NULL;

	UINT8 *imgptr = out->pixels;

	unsigned *column_posts = Z_Calloc(sizeof(unsigned) * inwidth, PU_STATIC, NULL);


	for (INT32 x = 0; x < inwidth; x++)
	{
		post_t *post = NULL;
		size_t post_data_offset = 0;
		boolean was_opaque = false;

		column_t *column = &out->columns[x];
		column->pixels = imgptr;
		column->posts = NULL;
		column->num_posts = 0;

		column_posts[x] = (unsigned)-1;


		for (INT32 y = 0; y < inheight; y++)
		{
			boolean opaque = false;


			void *input = readPixelFunc(picture, informat, x, y, inwidth, inheight, flags);


			if (input != NULL)
				opaque = getAlphaFunc(input, flags) > 0;


			if (!opaque)
			{
				was_opaque = false;
				continue;
			}

			if (!was_opaque)
			{
				num_posts++;

				out->posts = Z_Realloc(out->posts, sizeof(post_t) * num_posts, PU_PATCH_DATA, NULL);
				post = &out->posts[num_posts - 1];
				post->topdelta = (size_t)y;
				post->length = 0;
				post->data_offset = post_data_offset;
				if (column_posts[x] == (unsigned)-1)
					column_posts[x] = num_posts - 1;
				column->num_posts++;
			}

			was_opaque = true;


			UINT8 *last_ptr = imgptr;
			imgptr = writePatchPixel(last_ptr, input);

			post->length++;
			post_data_offset += imgptr - last_ptr;
		}
	}

	UINT8 *old_pixels = out->pixels;
	size_t total_pixels = imgptr - out->pixels;
	if (total_pixels != max_pixels)
		out->pixels = Z_Realloc(out->pixels, total_pixels, PU_PATCH_DATA, NULL);

	for (INT16 x = 0; x < inwidth; x++)
	{
		column_t *column = &out->columns[x];
		if (column->num_posts > 0)
			column->posts = &out->posts[column_posts[x]];
		if (old_pixels != out->pixels)
			column->pixels = out->pixels + (column->pixels - old_pixels);
	}

	Z_Free(column_posts);

	if (outsize != NULL)
		*outsize = sizeof(patch_t);

	return (void *)out;
}

                                 
   
                                         
                                      
                                           
                                                     
                                       
                                         
                                     
                                               
    
void *Picture_FlatConvert(
	pictureformat_t informat, void *picture, pictureformat_t outformat,
	size_t *outsize,
	INT32 inwidth, INT32 inheight,
	pictureflags_t flags)
{
	void *outflat;
	patch_t *inpatch = NULL;
	INT32 inbpp = Picture_FormatBPP(informat);
	INT32 outbpp = Picture_FormatBPP(outformat);
	INT32 x, y;
	size_t size;

	if (informat == PICFMT_NONE)
		I_Error("Picture_FlatConvert: input format was PICFMT_NONE!");
	else if (outformat == PICFMT_NONE)
		I_Error("Picture_FlatConvert: output format was PICFMT_NONE!");
	else if (informat == outformat)
		I_Error("Picture_FlatConvert: input and output formats were the same!");

	if (inbpp == PICDEPTH_NONE)
		I_Error("Picture_FlatConvert: unknown input bits per pixel?!");
	if (outbpp == PICDEPTH_NONE)
		I_Error("Picture_FlatConvert: unknown output bits per pixel?!");



	if (Picture_IsPatchFormat(informat))
	{
		inpatch = (patch_t *)picture;
		if (Picture_IsDoomPatchFormat(informat))
		{
			softwarepatch_t *doompatch = ((softwarepatch_t *)picture);
			inwidth = SHORT(doompatch->width);
			inheight = SHORT(doompatch->height);
		}
		else
		{
			inwidth = inpatch->width;
			inheight = inpatch->height;
		}
	}

	size = (inwidth * inheight) * (outbpp / 8);
	outflat = Z_Calloc(size, PU_STATIC, NULL);
	if (outsize)
		*outsize = size;


	if (outbpp == PICDEPTH_8BPP)
		memset(outflat, TRANSPARENTPIXEL, size);

	for (y = 0; y < inheight; y++)
		for (x = 0; x < inwidth; x++)
		{
			void *input = NULL;
			int sx = x;
			int sy = y;

			if (flags & PICFLAGS_XFLIP)
				sx = inwidth - x - 1;
			if (flags & PICFLAGS_YFLIP)
				sy = inheight - y - 1;

			size_t in_offs = ((sy * inwidth) + sx);
			size_t out_offs = ((y * inwidth) + x);


			if (Picture_IsPatchFormat(informat))
				input = Picture_GetPatchPixel(inpatch, informat, sx, sy, 0);
			else if (Picture_IsFlatFormat(informat))
				input = (UINT8 *)picture + (in_offs * (inbpp / 8));
			else
				I_Error("Picture_FlatConvert: unsupported input format!");

			if (!input)
				continue;

			switch (outformat)
			{
				case PICFMT_FLAT32:
				{
					UINT32 *f32 = (UINT32 *)outflat;
					if (inbpp == PICDEPTH_32BPP)
					{
						RGBA_t out = *(RGBA_t *)input;
						f32[out_offs] = out.rgba;
					}
					else if (inbpp == PICDEPTH_16BPP)
					{
						RGBA_t out = pMasterPalette[*((UINT16 *)input) & 0xFF];
						f32[out_offs] = out.rgba;
					}
					else  
					{
						RGBA_t out = pMasterPalette[*((UINT8 *)input) & 0xFF];
						f32[out_offs] = out.rgba;
					}
					break;
				}
				case PICFMT_FLAT16:
				{
					UINT16 *f16 = (UINT16 *)outflat;
					if (inbpp == PICDEPTH_32BPP)
					{
						RGBA_t in = *(RGBA_t *)input;
						UINT8 out = NearestColor(in.s.red, in.s.green, in.s.blue);
						f16[out_offs] = (0xFF00 | out);
					}
					else if (inbpp == PICDEPTH_16BPP)
						f16[out_offs] = *(UINT16 *)input;
					else  
						f16[out_offs] = (0xFF00 | *((UINT8 *)input));
					break;
				}
				case PICFMT_FLAT:
				{
					UINT8 *f8 = (UINT8 *)outflat;
					if (inbpp == PICDEPTH_32BPP)
					{
						RGBA_t in = *(RGBA_t *)input;
						UINT8 out = NearestColor(in.s.red, in.s.green, in.s.blue);
						f8[out_offs] = out;
					}
					else if (inbpp == PICDEPTH_16BPP)
					{
						UINT16 out = *(UINT16 *)input;
						f8[out_offs] = (out & 0xFF);
					}
					else  
						f8[out_offs] = *(UINT8 *)input;
					break;
				}
				default:
					I_Error("Picture_FlatConvert: unsupported output format!");
			}
		}

	return outflat;
}

                                 
   
                             
                                         
                              
                              
                                     
                                                                          
    
void *Picture_GetPatchPixel(
	patch_t *patch, pictureformat_t informat,
	INT32 x, INT32 y,
	pictureflags_t flags)
{
	INT32 inbpp = Picture_FormatBPP(informat);
	softwarepatch_t *doompatch = (softwarepatch_t *)patch;
	boolean isdoompatch = Picture_IsDoomPatchFormat(informat);

	if (patch == NULL)
		I_Error("Picture_GetPatchPixel: patch == NULL");

	INT16 width = (isdoompatch ? SHORT(doompatch->width) : patch->width);
	INT16 height = (isdoompatch ? SHORT(doompatch->height) : patch->height);

	if (x < 0 || x >= width || y < 0 || y >= height)
		return NULL;

	INT32 sx = (flags & PICFLAGS_XFLIP) ? (width-1)-x : x;
	INT32 sy = (flags & PICFLAGS_YFLIP) ? (height-1)-y : y;
	UINT8 *s8 = NULL;
	UINT16 *s16 = NULL;
	UINT32 *s32 = NULL;

	if (isdoompatch)
	{
		INT32 prevdelta = -1;
		INT32 colofs = LONG(doompatch->columnofs[sx]);


		doompost_t *column = (doompost_t *)((UINT8 *)doompatch + colofs);

		while (column->topdelta != 0xff)
		{
			INT32 topdelta = column->topdelta;
			if (topdelta <= prevdelta)
				topdelta += prevdelta;
			prevdelta = topdelta;

			size_t ofs = sy - topdelta;

			if (sy >= topdelta && ofs < column->length)
			{
				s8 = (UINT8 *)(column) + 3;
				switch (inbpp)
				{
					case PICDEPTH_32BPP:
						s32 = (UINT32 *)s8;
						return &s32[ofs];
					case PICDEPTH_16BPP:
						s16 = (UINT16 *)s8;
						return &s16[ofs];
					default:  
						return &s8[ofs];
				}
			}

			if (inbpp == PICDEPTH_32BPP)
				column = (doompost_t *)((UINT32 *)column + column->length);
			else if (inbpp == PICDEPTH_16BPP)
				column = (doompost_t *)((UINT16 *)column + column->length);
			else
				column = (doompost_t *)((UINT8 *)column + column->length);
			column = (doompost_t *)((UINT8 *)column + 4);
		}
	}
	else
	{
		column_t *column = &patch->columns[sx];
		for (unsigned i = 0; i < column->num_posts; i++)
		{
			post_t *post = &column->posts[i];

			size_t ofs = sy - post->topdelta;

			if (sy >= (INT32)post->topdelta && ofs < post->length)
			{
				s8 = column->pixels + post->data_offset;
				switch (inbpp)
				{
					case PICDEPTH_32BPP:
						s32 = (UINT32 *)s8;
						return &s32[ofs];
					case PICDEPTH_16BPP:
						s16 = (UINT16 *)s8;
						return &s16[ofs];
					default:  
						return &s8[ofs];
				}
			}
		}
	}

	return NULL;
}

                                                                         
   
                                       
                                                            
    
INT32 Picture_FormatBPP(pictureformat_t format)
{
	INT32 bpp = PICDEPTH_NONE;
	switch (format)
	{
		case PICFMT_PATCH32:
		case PICFMT_FLAT32:
		case PICFMT_DOOMPATCH32:
		case PICFMT_PNG:
			bpp = PICDEPTH_32BPP;
			break;
		case PICFMT_PATCH16:
		case PICFMT_FLAT16:
		case PICFMT_DOOMPATCH16:
			bpp = PICDEPTH_16BPP;
			break;
		case PICFMT_PATCH:
		case PICFMT_FLAT:
		case PICFMT_DOOMPATCH:
			bpp = PICDEPTH_8BPP;
			break;
		default:
			break;
	}
	return bpp;
}

                                                      
   
                                       
                                                                
    
boolean Picture_IsPatchFormat(pictureformat_t format)
{
	return (Picture_IsInternalPatchFormat(format) || Picture_IsDoomPatchFormat(format));
}

                                                                
   
                                       
                                                                          
    
boolean Picture_IsInternalPatchFormat(pictureformat_t format)
{
	switch (format)
	{
		case PICFMT_PATCH:
		case PICFMT_PATCH16:
		case PICFMT_PATCH32:
			return true;
		default:
			return false;
	}
}

                                                           
   
                                       
                                                                     
    
boolean Picture_IsDoomPatchFormat(pictureformat_t format)
{
	switch (format)
	{
		case PICFMT_DOOMPATCH:
		case PICFMT_DOOMPATCH16:
		case PICFMT_DOOMPATCH32:
			return true;
		default:
			return false;
	}
}

                                                     
   
                                       
                                                               
    
boolean Picture_IsFlatFormat(pictureformat_t format)
{
	return (format == PICFMT_FLAT || format == PICFMT_FLAT16 || format == PICFMT_FLAT32);
}

                                                   
   
                             
                                    
                                             
    
boolean Picture_CheckIfDoomPatch(softwarepatch_t *patch, size_t size)
{
	const UINT8 *data;
	const UINT8 *end;

	if (patch == NULL)
		return false;
	data = (const UINT8 *)patch;
	end = data + size;

	if (size < MIN_PATCH_LUMP_SIZE)
		return false;

	INT16 width = SHORT(patch->width);
	INT16 height = SHORT(patch->height);

	if (width <= 0 || height <= 0 || width > MAX_PATCH_DIMENSIONS || height > MAX_PATCH_DIMENSIONS)
		return false;
	if (!VALID_PATCH_LUMP_SIZE(size, width))
		return false;


	for (INT16 x = 0; x < width; x++)
	{
		UINT32 ofs = LONG(patch->columnofs[x]);
		const UINT8 *post;

		if (ofs < FIRST_PATCH_LUMP_COLUMN(width) || (size_t)ofs >= size)
			return false;

		                                                                   
                                                                         
                                                                         
                                                    
		post = data + ofs;
		for (;;)
		{
			size_t remaining;
			size_t postsize;

			if (post >= end)
				return false;
			if (post[0] == 0xff)
				break;

			remaining = (size_t)(end - post);
			if (remaining < 4)
				return false;

			postsize = (size_t)post[1] + 4;
			if (postsize > remaining)
				return false;

			post += postsize;
		}
	}

	return true;
}

                                 
   
                                     
                               
    
void *Picture_TextureToFlat(size_t texnum)
{
	texture_t *texture;

	UINT8 *converted;
	size_t flatsize;
	UINT8 *desttop, *dest, *deststop;
	UINT8 *source;

	if (texnum >= (unsigned)numtextures)
		I_Error("Picture_TextureToFlat: invalid texture number!");



	texture = textures[texnum];
	R_CheckTextureCache(texnum);


	flatsize = texture->width * texture->height;
	converted = Z_Malloc(flatsize, PU_STATIC, NULL);
	memset(converted, TRANSPARENTPIXEL, flatsize);


	desttop = converted;
	deststop = desttop + flatsize;

	for (size_t col = 0; col < (size_t)texture->width; col++, desttop++)
	{
		column_t *column = (column_t *)R_GetColumn(texnum, col);
		for (unsigned i = 0; i < column->num_posts; i++)
		{
			post_t *post = &column->posts[i];
			dest = desttop + (post->topdelta * texture->width);
			source = column->pixels + post->data_offset;
			for (size_t ofs = 0; dest < deststop && ofs < post->length; ofs++)
			{
				if (source[ofs] != TRANSPARENTPIXEL)
					*dest = source[ofs];
				dest += texture->width;
			}
		}
	}

	return converted;
}

                                            
   
                                    
                           
                                            
    
boolean Picture_IsLumpPNG(const UINT8 *d, size_t s)
{
	if (s < PNG_MIN_SIZE)  
		return false;



	return (memcmp(&d[0], "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a", PNG_HEADER_SIZE) == 0);
}

#ifndef NO_PNG_LUMPS
static boolean Picture_PNGDimensionsAreSafe(UINT32 width, UINT32 height)
{
	return width > 0 && height > 0
		&& width <= MAX_PNG_DIMENSIONS && height <= MAX_PNG_DIMENSIONS
		&& (UINT64)width * (UINT64)height <= (UINT64)MAX_PNG_PIXELS;
}

                                                             
                                                                            
                                                                          
    
boolean Picture_PNGDimensionsFromHeader(const UINT8 *png, size_t size, INT32 *width, INT32 *height)
{
	UINT32 w, h;

	if (!Picture_IsLumpPNG(png, size) || memcmp(&png[12], "IHDR", 4))
		return false;

	w = ((UINT32)png[16] << 24) | ((UINT32)png[17] << 16)
		| ((UINT32)png[18] << 8) | (UINT32)png[19];
	h = ((UINT32)png[20] << 24) | ((UINT32)png[21] << 16)
		| ((UINT32)png[22] << 8) | (UINT32)png[23];
	if (!Picture_PNGDimensionsAreSafe(w, h))
		return false;

	if (width)
		*width = (INT32)w;
	if (height)
		*height = (INT32)h;

	return true;
}
#endif

#ifndef NO_PNG_LUMPS
#ifdef HAVE_PNG

                                
                                            
        
typedef struct
{
	const UINT8 *buffer;
	size_t size;
	size_t position;
} png_io_t;

static void PNG_IOReader(png_structp png_ptr, png_bytep data, png_size_t length)
{
	png_io_t *f = png_get_io_ptr(png_ptr);
	if (f->position > f->size || length > (f->size - f->position))
		png_error(png_ptr, "PNG_IOReader: buffer overrun");
	memcpy(data, f->buffer + f->position, length);
	f->position += length;
}

typedef struct
{
	char name[4];
	void *data;
	size_t size;
} png_chunk_t;

static png_byte *chunkname = NULL;
static png_chunk_t chunk;
static char png_error_message[256];

static int PNG_ChunkReader(png_structp png_ptr, png_unknown_chunkp chonk)
{
	(void)png_ptr;
	if (!memcmp(chonk->name, chunkname, 4))
	{
		                                                                   
                                                                          
		if (chonk->size != 8)
			return 1;
		memcpy(chunk.name, chonk->name, 4);
		chunk.size = chonk->size;
		chunk.data = Z_Malloc(chunk.size, PU_STATIC, NULL);
		memcpy(chunk.data, chonk->data, chunk.size);
		return 1;
	}
	return 0;
}

static void PNG_error(png_structp PNG, png_const_charp pngtext)
{
	strncpy(png_error_message, pngtext, sizeof png_error_message - 1);
	png_error_message[sizeof png_error_message - 1] = '\0';
	CONS_Debug(DBG_RENDER, "libpng error at %p: %s\n", PNG, pngtext);
}

static void PNG_warn(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng warning at %p: %s\n", PNG, pngtext);
}

static png_byte grAb_chunk[5] = {'g', 'r', 'A', 'b', (png_byte)'\0'};

typedef struct
{
	png_bytep *rows;
	size_t rows_allocated;
} png_rows_t;

static void PNG_FreeRows(png_rows_t *allocated)
{
	size_t i;

	if (!allocated)
		return;
	for (i = 0; i < allocated->rows_allocated; i++)
		free(allocated->rows[i]);
	free(allocated->rows);
	free(allocated);
}

static png_bytep *PNG_Read(
	const UINT8 *png,
	INT32 *w, INT32 *h, INT16 *topoffset, INT16 *leftoffset,
	boolean *use_palette, UINT8 *palette_alpha, size_t size)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type;
	png_uint_32 y;

	png_colorp palette;
	int palette_size;

	png_bytep trans = NULL;
	int num_trans = 0;

#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif

	png_io_t png_io;
	png_bytep *row_pointers;
	png_voidp *user_chunk_ptr;
	png_rows_t *allocated = calloc(1, sizeof (*allocated));

	if (!allocated)
	{
		CONS_Alert(CONS_ERROR, "PNG_Read: couldn't allocate decoder bookkeeping; graphic skipped.\n");
		return NULL;
	}

	png_error_message[0] = '\0';
	memset(&chunk, 0x00, sizeof(png_chunk_t));

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, PNG_error, PNG_warn);
	if (!png_ptr)
	{
		PNG_FreeRows(allocated);
		CONS_Alert(CONS_ERROR, "PNG_Read: couldn't initialize libpng; graphic skipped.\n");
		return NULL;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		PNG_FreeRows(allocated);
		CONS_Alert(CONS_ERROR, "PNG_Read: libpng couldn't allocate memory; graphic skipped.\n");
		return NULL;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		PNG_FreeRows(allocated);
		if (chunk.data)
		{
			Z_Free(chunk.data);
			chunk.data = NULL;
		}
		CONS_Alert(CONS_ERROR, "PNG graphic could not be decoded (%s); using a missing graphic instead.\n",
			png_error_message[0] ? png_error_message : "invalid image data");
		return NULL;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof jmp_buf);
#endif

	png_io.buffer = png;
	png_io.size = size;
	png_io.position = 0;
	png_set_read_fn(png_ptr, &png_io, PNG_IOReader);

	chunkname = grAb_chunk;

	user_chunk_ptr = png_get_user_chunk_ptr(png_ptr);
	png_set_read_user_chunk_fn(png_ptr, user_chunk_ptr, PNG_ChunkReader);
	png_set_keep_unknown_chunks(png_ptr, 2, chunkname, 1);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, MAX_PNG_DIMENSIONS, MAX_PNG_DIMENSIONS);
#endif

	png_read_info(png_ptr, png_info_ptr);
	png_get_IHDR(png_ptr, png_info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
	if (!Picture_PNGDimensionsAreSafe((UINT32)width, (UINT32)height))
		png_error(png_ptr, "image dimensions exceed SRB2's safe PNG limits");

	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	palette = NULL;
	*use_palette = false;
	memset(palette_alpha, 0xFF, 256 * sizeof (*palette_alpha));

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	else if (color_type == PNG_COLOR_TYPE_PALETTE)
	{
		boolean usepal = false;



		if (png_get_PLTE(png_ptr, png_info_ptr, &palette, &palette_size))
		{
			if (palette_size == 256 && pMasterPalette)
			{
				png_colorp pal = palette;
				INT32 i;

				usepal = true;

				for (i = 0; i < 256; i++)
				{
					byteColor_t *curpal = &(pMasterPalette[i].s);
					if (pal->red != curpal->red || pal->green != curpal->green || pal->blue != curpal->blue)
					{
						usepal = false;
						break;
					}
					pal++;
				}
			}
		}

		                                                                 
                                                                       
                                                                      
                                                                       
                                                                     
                                             
		if (usepal)
		{
			png_uint_32 result = png_get_tRNS(png_ptr, png_info_ptr, &trans, &num_trans, NULL);

			if ((result & PNG_INFO_tRNS) && num_trans > 0 && trans != NULL)
				memcpy(palette_alpha, trans,
					(size_t)min(num_trans, 256) * sizeof (*palette_alpha));
		}

		if (usepal)
			*use_palette = true;
		else
			png_set_palette_to_rgb(png_ptr);
	}

	if (!*use_palette && png_get_valid(png_ptr, png_info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	else if (!*use_palette && color_type != PNG_COLOR_TYPE_RGB_ALPHA && color_type != PNG_COLOR_TYPE_GRAY_ALPHA)
	{
#if PNG_LIBPNG_VER < 10207
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
#else
		png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
#endif
	}

	png_read_update_info(png_ptr, png_info_ptr);


	allocated->rows = (png_bytep*)malloc(sizeof(png_bytep) * height);
	if (!allocated->rows)
		png_error(png_ptr, "couldn't allocate PNG row table");
	for (y = 0; y < height; y++)
	{
		allocated->rows[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, png_info_ptr));
		if (!allocated->rows[y])
			png_error(png_ptr, "couldn't allocate PNG row");
		allocated->rows_allocated++;
	}
	row_pointers = allocated->rows;
	png_read_image(png_ptr, row_pointers);


	if ((topoffset || leftoffset) && (chunk.data != NULL))
	{
		INT32 *offsets = (INT32 *)chunk.data;

		if (leftoffset != NULL)
			*leftoffset = (INT16)BIGENDIAN_LONG(*offsets);
		offsets++;

		if (topoffset != NULL)
			*topoffset = (INT16)BIGENDIAN_LONG(*offsets);
	}

	png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
	if (chunk.data)
	{
		Z_Free(chunk.data);
		chunk.data = NULL;
	}
	allocated->rows = NULL;
	allocated->rows_allocated = 0;
	free(allocated);

	*w = (INT32)width;
	*h = (INT32)height;

	return row_pointers;
}

                                
   
                             
                                                 
                                                      
                                                       
                                                                                
                                                                                  
                                           
                                                          
                                     
                                               
    
static void *Picture_PNGConvertInternal(
	const UINT8 *png, pictureformat_t outformat,
	INT32 *w, INT32 *h,
	INT16 *topoffset, INT16 *leftoffset,
	size_t insize, size_t *outsize,
	pictureflags_t flags, RGBA_t **rgbaout)
{
	void *flat;
	INT32 outbpp;
	size_t flatsize;
	png_uint_32 x, y;
	png_bytep row;
	boolean palette = false;
	UINT8 palette_alpha[256];
	png_bytep *row_pointers = NULL;
	png_uint_32 width, height;

	INT32 pngwidth, pngheight;
	INT16 loffs = 0, toffs = 0;

	if (png == NULL)
	{
		CONS_Alert(CONS_ERROR, "Picture_PNGConvert: picture was NULL; graphic skipped.\n");
		return NULL;
	}

	if (w == NULL)
		w = &pngwidth;
	if (h == NULL)
		h = &pngheight;
	if (topoffset == NULL)
		topoffset = &toffs;
	if (leftoffset == NULL)
		leftoffset = &loffs;

	row_pointers = PNG_Read(png, w, h, topoffset, leftoffset,
		&palette, palette_alpha, insize);
	width = *w;
	height = *h;

	if (row_pointers == NULL)
		return NULL;

	                                                                   
                                                                          
                                                                            
                               
	if (rgbaout)
	{
		RGBA_t *rgba = Z_Calloc((size_t)width * (size_t)height
			* sizeof (*rgba), PU_STATIC, NULL);

		if (palette)
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
				{
					const UINT8 index = row[x];
					rgba[(size_t)y * width + x] = V_GetColor(index);
					rgba[(size_t)y * width + x].s.alpha = palette_alpha[index];
				}
			}
		}
		else
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
				{
					const png_bytep px = &(row[x * 4]);
					RGBA_t *destination = &rgba[(size_t)y * width + x];
					destination->s.red = (UINT8)px[0];
					destination->s.green = (UINT8)px[1];
					destination->s.blue = (UINT8)px[2];
					destination->s.alpha = (UINT8)px[3];
				}
			}
		}
		*rgbaout = rgba;
	}


	outbpp = Picture_FormatBPP(outformat);


	if (Picture_IsPatchFormat(outformat))
	{

		if (outbpp == PICDEPTH_8BPP)
			outbpp = PICDEPTH_16BPP;
	}


	if (outbpp == PICDEPTH_NONE)
		I_Error("Picture_PNGConvert: unknown output bits per pixel?!");


	flatsize = (size_t)width * (size_t)height * (size_t)(outbpp / 8);
	if (outsize)
		*outsize = flatsize;


	flat = Z_Calloc(flatsize, PU_STATIC, NULL);


	if (outbpp == PICDEPTH_8BPP)
		memset(flat, TRANSPARENTPIXEL, (width * height));

#ifdef PICTURE_PNG_USELOOKUP
	if (outbpp != PICDEPTH_32BPP)
		InitColorLUT(&png_colorlookup, pMasterPalette, false);
#endif

	if (outbpp == PICDEPTH_32BPP)
	{
		RGBA_t out;
		UINT32 *outflat = (UINT32 *)flat;

		if (palette)
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
				{
					const UINT8 index = row[x];
					out = V_GetColor(index);
					out.s.alpha = palette_alpha[index];
					outflat[((y * width) + x)] = out.rgba;
				}
			}
		}
		else
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
				{
					png_bytep px = &(row[x * 4]);
					if ((UINT8)px[3])
					{
						out.s.red = (UINT8)px[0];
						out.s.green = (UINT8)px[1];
						out.s.blue = (UINT8)px[2];
						out.s.alpha = (UINT8)px[3];
						outflat[((y * width) + x)] = out.rgba;
					}
					else
						outflat[((y * width) + x)] = 0x00000000;
				}
			}
		}
	}
	else if (outbpp == PICDEPTH_16BPP)
	{
		UINT16 *outflat = (UINT16 *)flat;

		if (palette)
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
				{
					const UINT8 index = row[x];
					outflat[((y * width) + x)] = palette_alpha[index]
						? (0xFF << 8) | index : 0x0000;
				}
			}
		}
		else
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
				{
					png_bytep px = &(row[x * 4]);
					UINT8 red = (UINT8)px[0];
					UINT8 green = (UINT8)px[1];
					UINT8 blue = (UINT8)px[2];
					UINT8 alpha = (UINT8)px[3];

					if (alpha)
					{
#ifdef PICTURE_PNG_USELOOKUP
						UINT8 palidx = GetColorLUT(&png_colorlookup, red, green, blue);
#else
						UINT8 palidx = NearestColor(red, green, blue);
#endif
						outflat[((y * width) + x)] = (0xFF << 8) | palidx;
					}
					else
						outflat[((y * width) + x)] = 0x0000;
				}
			}
		}
	}
	else  
	{
		UINT8 *outflat = (UINT8 *)flat;

		if (palette)
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
				{
					const UINT8 index = row[x];
					outflat[((y * width) + x)] = palette_alpha[index]
						? index : TRANSPARENTPIXEL;
				}
			}
		}
		else
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
				{
					png_bytep px = &(row[x * 4]);
					UINT8 red = (UINT8)px[0];
					UINT8 green = (UINT8)px[1];
					UINT8 blue = (UINT8)px[2];
					UINT8 alpha = (UINT8)px[3];

					if (alpha)
					{
#ifdef PICTURE_PNG_USELOOKUP
						UINT8 palidx = GetColorLUT(&png_colorlookup, red, green, blue);
#else
						UINT8 palidx = NearestColor(red, green, blue);
#endif
						outflat[((y * width) + x)] = palidx;
					}
				}
			}
		}
	}


	for (y = 0; y < height; y++)
		free(row_pointers[y]);
	free(row_pointers);


	if (Picture_IsPatchFormat(outformat))
	{
		void *converted;
		pictureformat_t informat = PICFMT_NONE;


		switch (outbpp)
		{
			case 32:
				informat = PICFMT_FLAT32;
				break;
			case 16:
				informat = PICFMT_FLAT16;
				break;
			default:
				informat = PICFMT_FLAT;
				break;
		}


		converted = Picture_PatchConvert(informat, flatsize, flat, outformat, outsize, (INT32)width, (INT32)height, *leftoffset, *topoffset, flags);
		if (converted == NULL)
		{
			if (rgbaout && *rgbaout)
			{
				Z_Free(*rgbaout);
				*rgbaout = NULL;
			}
			Z_Free(flat);
			CONS_Alert(CONS_ERROR, "Picture_PNGConvert: conversion did not produce a valid graphic.\n");
			return NULL;
		}
		Z_Free(flat);
		return converted;
	}


	return flat;
}

void *Picture_PNGConvert(
	const UINT8 *png, pictureformat_t outformat,
	INT32 *w, INT32 *h,
	INT16 *topoffset, INT16 *leftoffset,
	size_t insize, size_t *outsize,
	pictureflags_t flags)
{
	return Picture_PNGConvertInternal(png, outformat, w, h,
		topoffset, leftoffset, insize, outsize, flags, NULL);
}

void *Picture_PNGConvertWithRGBA(
	const UINT8 *png, pictureformat_t outformat,
	INT32 *w, INT32 *h,
	INT16 *topoffset, INT16 *leftoffset,
	size_t insize, size_t *outsize,
	pictureflags_t flags, RGBA_t **rgbaout)
{
	if (!rgbaout)
		I_Error("Picture_PNGConvertWithRGBA: rgbaout was NULL!");
	*rgbaout = NULL;
	return Picture_PNGConvertInternal(png, outformat, w, h,
		topoffset, leftoffset, insize, outsize, flags, rgbaout);
}

                                                                               
   
                             
                                                        
                                                          
                                                                      
                                                                         
                                         
                                                                   
    
boolean Picture_PNGDimensions(UINT8 *png, INT32 *width, INT32 *height, INT16 *topoffset, INT16 *leftoffset, size_t size)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_uint_32 w, h;
	int bit_depth, color_type;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif

	png_io_t png_io;
	png_voidp *user_chunk_ptr;

	png_error_message[0] = '\0';
	memset(&chunk, 0x00, sizeof(png_chunk_t));

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Alert(CONS_ERROR, "Picture_PNGDimensions: Couldn't initialize libpng!\n");
		return false;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		CONS_Alert(CONS_ERROR, "Picture_PNGDimensions: libpng couldn't allocate memory!\n");
		return false;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		if (chunk.data)
		{
			Z_Free(chunk.data);
			chunk.data = NULL;
		}
		CONS_Alert(CONS_ERROR, "Picture_PNGDimensions: couldn't read PNG (%s).\n",
			png_error_message[0] ? png_error_message : "invalid image data");
		return false;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof jmp_buf);
#endif

	png_io.buffer = png;
	png_io.size = size;
	png_io.position = 0;
	png_set_read_fn(png_ptr, &png_io, PNG_IOReader);

	chunkname = grAb_chunk;

	user_chunk_ptr = png_get_user_chunk_ptr(png_ptr);
	png_set_read_user_chunk_fn(png_ptr, user_chunk_ptr, PNG_ChunkReader);
	png_set_keep_unknown_chunks(png_ptr, 2, chunkname, 1);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, MAX_PNG_DIMENSIONS, MAX_PNG_DIMENSIONS);
#endif

	png_read_info(png_ptr, png_info_ptr);
	png_get_IHDR(png_ptr, png_info_ptr, &w, &h, &bit_depth, &color_type, NULL, NULL, NULL);
	if (!Picture_PNGDimensionsAreSafe((UINT32)w, (UINT32)h))
		png_error(png_ptr, "image dimensions exceed SRB2's safe PNG limits");


	if ((topoffset || leftoffset) && (chunk.data != NULL))
	{
		INT32 *offsets = (INT32 *)chunk.data;

		if (leftoffset != NULL)
			*leftoffset = (INT16)BIGENDIAN_LONG(*offsets);
		offsets++;

		if (topoffset != NULL)
			*topoffset = (INT16)BIGENDIAN_LONG(*offsets);
	}

	png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
	if (chunk.data)
	{
		Z_Free(chunk.data);
		chunk.data = NULL;
	}

	*width = (INT32)w;
	*height = (INT32)h;
	return true;
}
#endif
#endif






static void R_ParseSpriteInfoFrame(spriteinfo_t *info)
{
	char *sprinfoToken;
	size_t sprinfoTokenLength;
	char *frameChar = NULL;
	UINT8 frameFrame = 0xFF;
	INT16 frameXPivot = 0;
	INT16 frameYPivot = 0;


	sprinfoToken = M_GetToken(NULL);
	if (sprinfoToken == NULL)
	{
		I_Error("Error parsing SPRTINFO lump: Unexpected end of file where sprite frame should be");
	}
	sprinfoTokenLength = strlen(sprinfoToken);
	if (sprinfoTokenLength != 1)
	{
		I_Error("Error parsing SPRTINFO lump: Invalid frame \"%s\"",sprinfoToken);
	}
	else
		frameChar = sprinfoToken;

	frameFrame = R_Char2Frame(frameChar[0]);
	Z_Free(sprinfoToken);


	sprinfoToken = M_GetToken(NULL);
	if (sprinfoToken == NULL)
		I_Error("Error parsing SPRTINFO lump: Missing sprite info");
	else
	{
		if (strcmp(sprinfoToken,"{")==0)
		{
			Z_Free(sprinfoToken);
			sprinfoToken = M_GetToken(NULL);
			if (sprinfoToken == NULL)
			{
				I_Error("Error parsing SPRTINFO lump: Unexpected end of file where sprite info should be");
			}
			while (strcmp(sprinfoToken,"}")!=0)
			{
				if (stricmp(sprinfoToken, "XPIVOT")==0)
				{
					Z_Free(sprinfoToken);
					sprinfoToken = M_GetToken(NULL);
					frameXPivot = atoi(sprinfoToken);
				}
				else if (stricmp(sprinfoToken, "YPIVOT")==0)
				{
					Z_Free(sprinfoToken);
					sprinfoToken = M_GetToken(NULL);
					frameYPivot = atoi(sprinfoToken);
				}
				else if (stricmp(sprinfoToken, "ROTAXIS")==0)
				{
					Z_Free(sprinfoToken);
					sprinfoToken = M_GetToken(NULL);
				}
				Z_Free(sprinfoToken);

				sprinfoToken = M_GetToken(NULL);
				if (sprinfoToken == NULL)
				{
					I_Error("Error parsing SPRTINFO lump: Unexpected end of file where sprite info or right curly brace should be");
				}
			}
		}
		Z_Free(sprinfoToken);
	}


	info->pivot[frameFrame].x = frameXPivot;
	info->pivot[frameFrame].y = frameYPivot;
}






static void R_ParseSpriteInfo(boolean spr2)
{
	spriteinfo_t *info;
	char *sprinfoToken;
	size_t sprinfoTokenLength;
	char newSpriteName[MAXSPRITENAME + 1];  
	spritenum_t sprnum = NUMSPRITES;
	playersprite_t spr2num = NUMPLAYERSPRITES;
	INT32 i;
	UINT8 *skinnumbers = NULL;
	INT32 foundskins = 0;


	sprinfoToken = M_GetToken(NULL);
	if (sprinfoToken == NULL)
	{
		I_Error("Error parsing SPRTINFO lump: Unexpected end of file where sprite name should be");
	}
	sprinfoTokenLength = strlen(sprinfoToken);
	if (sprinfoTokenLength > MAXSPRITENAME)
		I_Error("Error parsing SPRTINFO lump: Sprite name \"%s\" is longer than %d characters", sprinfoToken, MAXSPRITENAME);
	strcpy(newSpriteName, sprinfoToken);
	strupr(newSpriteName);  
	Z_Free(sprinfoToken);

	if (!spr2)
	{
		sprnum = R_GetSpriteNumByName(newSpriteName);
		if (sprnum == NUMSPRITES)
			I_Error("Error parsing SPRTINFO lump: Unknown sprite name \"%s\"", newSpriteName);
	}
	else
	{
		for (i = 0; i <= NUMPLAYERSPRITES; i++)
		{
			if (i == NUMPLAYERSPRITES)
				I_Error("Error parsing SPRTINFO lump: Unknown sprite2 name \"%s\"", newSpriteName);
			if (!memcmp(newSpriteName,spr2names[i],4))
			{
				spr2num = i;
				break;
			}
		}
	}


	info = Z_Calloc(sizeof(spriteinfo_t), PU_STATIC, NULL);
	info->available = true;


	sprinfoToken = M_GetToken(NULL);
	if (sprinfoToken == NULL)
	{
		I_Error("Error parsing SPRTINFO lump: Unexpected end of file where open curly brace for sprite \"%s\" should be",newSpriteName);
	}
	if (strcmp(sprinfoToken,"{")==0)
	{
		Z_Free(sprinfoToken);
		sprinfoToken = M_GetToken(NULL);
		if (sprinfoToken == NULL)
		{
			I_Error("Error parsing SPRTINFO lump: Unexpected end of file where definition for sprite \"%s\" should be",newSpriteName);
		}
		while (strcmp(sprinfoToken,"}")!=0)
		{
			if (stricmp(sprinfoToken, "SKIN")==0)
			{
				INT32 skinnum;
				char *skinName = NULL;
				if (!spr2)
					I_Error("Error parsing SPRTINFO lump: \"SKIN\" token found outside of a sprite2 definition");

				Z_Free(sprinfoToken);


				sprinfoToken = M_GetToken(NULL);
				if (sprinfoToken == NULL)
				{
					I_Error("Error parsing SPRTINFO lump: Unexpected end of file where skin frame should be");
				}


				sprinfoTokenLength = strlen(sprinfoToken);
				skinName = (char *)Z_Malloc((sprinfoTokenLength+1)*sizeof(char),PU_STATIC,NULL);
				M_Memcpy(skinName,sprinfoToken,sprinfoTokenLength*sizeof(char));
				skinName[sprinfoTokenLength] = '\0';
				strlwr(skinName);
				Z_Free(sprinfoToken);

				skinnum = R_SkinAvailable(skinName);
				if (skinnum == -1)
					I_Error("Error parsing SPRTINFO lump: Unknown skin \"%s\"", skinName);

				if (skinnumbers == NULL)
					skinnumbers = Z_Malloc(sizeof(UINT8) * numskins, PU_STATIC, NULL);
				skinnumbers[foundskins] = (UINT8)skinnum;
				foundskins++;
			}
			else if (stricmp(sprinfoToken, "FRAME")==0)
			{
				R_ParseSpriteInfoFrame(info);
				Z_Free(sprinfoToken);
				if (spr2)
				{
					if (!foundskins)
						I_Error("Error parsing SPRTINFO lump: No skins specified in this sprite2 definition");
					for (i = 0; i < foundskins; i++)
					{
						skin_t *skin = skins[skinnumbers[i]];
						spriteinfo_t *sprinfo = skin->sprinfo;
						M_Memcpy(&sprinfo[spr2num], info, sizeof(spriteinfo_t));
					}
				}
				else
					M_Memcpy(&spriteinfo[sprnum], info, sizeof(spriteinfo_t));
			}
			else
			{
				I_Error("Error parsing SPRTINFO lump: Unknown keyword \"%s\" in sprite %s",sprinfoToken,newSpriteName);
			}

			sprinfoToken = M_GetToken(NULL);
			if (sprinfoToken == NULL)
			{
				I_Error("Error parsing SPRTINFO lump: Unexpected end of file where sprite info or right curly brace for sprite \"%s\" should be",newSpriteName);
			}
		}
	}
	else
	{
		I_Error("Error parsing SPRTINFO lump: Expected \"{\" for sprite \"%s\", got \"%s\"",newSpriteName,sprinfoToken);
	}

	Z_Free(sprinfoToken);
	Z_Free(info);
	if (skinnumbers)
		Z_Free(skinnumbers);
}






void R_ParseSPRTINFOLump(UINT16 wadNum, UINT16 lumpNum)
{
	char *sprinfoLump;
	size_t sprinfoLumpLength;
	char *sprinfoText;
	char *sprinfoToken;




	sprinfoLump = (char *)W_CacheLumpNumPwad(wadNum, lumpNum, PU_STATIC);

	if (sprinfoLump == NULL) return;

	sprinfoLumpLength = W_LumpLengthPwad(wadNum, lumpNum);
	sprinfoText = (char *)Z_Malloc((sprinfoLumpLength+1)*sizeof(char),PU_STATIC,NULL);

	memmove(sprinfoText,sprinfoLump,sprinfoLumpLength);

	sprinfoText[sprinfoLumpLength] = '\0';


	Z_Free(sprinfoLump);

	sprinfoToken = M_GetToken(sprinfoText);
	while (sprinfoToken != NULL)
	{
		if (!stricmp(sprinfoToken, "SPRITE"))
			R_ParseSpriteInfo(false);
		else if (!stricmp(sprinfoToken, "SPRITE2"))
			R_ParseSpriteInfo(true);
		else
			I_Error("Error parsing SPRTINFO lump: Unknown keyword \"%s\"", sprinfoToken);
		Z_Free(sprinfoToken);
		sprinfoToken = M_GetToken(NULL);
	}
	Z_Free((void *)sprinfoText);
}






void R_LoadSpriteInfoLumps(UINT16 wadnum, UINT16 numlumps)
{
	lumpinfo_t *lumpinfo = wadfiles[wadnum]->lumpinfo;
	UINT16 i;
	char *name;

	for (i = 0; i < numlumps; i++, lumpinfo++)
	{
		name = lumpinfo->name;

		if (!memcmp(name, "SPRTINFO", 8) || !memcmp(name, "SPR_", 4))
			R_ParseSPRTINFOLump(wadnum, i);
	}
}
