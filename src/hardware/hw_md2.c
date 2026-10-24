// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------




#ifdef __GNUC__
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../d_main.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../fastcmp.h"

#ifdef HWRENDER
#include "hw_drv.h"
#include "hw_light.h"
#include "hw_md2.h"
#include "../d_main.h"
#include "../r_bsp.h"
#include "../r_fps.h"
#include "../r_main.h"
#include "../m_misc.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../r_state.h"
#include "../r_things.h"
#include "../r_draw.h"
#include "../r_picformats.h"
#include "../p_tick.h"
#include "hw_model.h"

#include "hw_main.h"
#include "../v_video.h"
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
 #if PNG_LIBPNG_VER < 100207

 #endif
#endif

#ifndef errno
#include "errno.h"
#endif

md2_t md2_models[NUMSPRITES];
md2_t *md2_playermodels = NULL;
size_t md2_numplayermodels = 0;


  
             
   
#if 0
static void md2_freeModel (model_t *model)
{
	UnloadModel(model);
}
#endif






static model_t *md2_readModel(const char *filename)
{

	if (FIL_FileExists(va("%s"PATHSEP"%s", srb2home, filename)))
		return LoadModel(va("%s"PATHSEP"%s", srb2home, filename), PU_STATIC);

	if (FIL_FileExists(va("%s"PATHSEP"%s", srb2path, filename)))
		return LoadModel(va("%s"PATHSEP"%s", srb2path, filename), PU_STATIC);

	return NULL;
}

static inline void md2_printModelInfo (model_t *model)
{
#if 0
	INT32 i;

	CONS_Debug(DBG_RENDER, "magic:\t\t\t%c%c%c%c\n", model->header.magic>>24,
	            (model->header.magic>>16)&0xff,
	            (model->header.magic>>8)&0xff,
	             model->header.magic&0xff);
	CONS_Debug(DBG_RENDER, "version:\t\t%d\n", model->header.version);
	CONS_Debug(DBG_RENDER, "skinWidth:\t\t%d\n", model->header.skinWidth);
	CONS_Debug(DBG_RENDER, "skinHeight:\t\t%d\n", model->header.skinHeight);
	CONS_Debug(DBG_RENDER, "frameSize:\t\t%d\n", model->header.frameSize);
	CONS_Debug(DBG_RENDER, "numSkins:\t\t%d\n", model->header.numSkins);
	CONS_Debug(DBG_RENDER, "numVertices:\t\t%d\n", model->header.numVertices);
	CONS_Debug(DBG_RENDER, "numTexCoords:\t\t%d\n", model->header.numTexCoords);
	CONS_Debug(DBG_RENDER, "numTriangles:\t\t%d\n", model->header.numTriangles);
	CONS_Debug(DBG_RENDER, "numGlCommands:\t\t%d\n", model->header.numGlCommands);
	CONS_Debug(DBG_RENDER, "numFrames:\t\t%d\n", model->header.numFrames);
	CONS_Debug(DBG_RENDER, "offsetSkins:\t\t%d\n", model->header.offsetSkins);
	CONS_Debug(DBG_RENDER, "offsetTexCoords:\t%d\n", model->header.offsetTexCoords);
	CONS_Debug(DBG_RENDER, "offsetTriangles:\t%d\n", model->header.offsetTriangles);
	CONS_Debug(DBG_RENDER, "offsetFrames:\t\t%d\n", model->header.offsetFrames);
	CONS_Debug(DBG_RENDER, "offsetGlCommands:\t%d\n", model->header.offsetGlCommands);
	CONS_Debug(DBG_RENDER, "offsetEnd:\t\t%d\n", model->header.offsetEnd);

	for (i = 0; i < model->header.numFrames; i++)
		CONS_Debug(DBG_RENDER, "%s ", model->frames[i].name);
	CONS_Debug(DBG_RENDER, "\n");
#else
	(void)model;
#endif
}

#ifdef HAVE_PNG
static void PNG_error(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng error at %p: %s", PNG, pngtext);

}

static void PNG_warn(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng warning at %p: %s", PNG, pngtext);
}

static GLTextureFormat_t PNG_Load(const char *filename, int *w, int *h, GLPatch_t *grpatch)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif
	volatile png_FILE_p png_FILE;

	char *pngfilename = va("%s"PATHSEP"models"PATHSEP"%s", srb2home, filename);

	FIL_ForceExtension(pngfilename, ".png");
	png_FILE = fopen(pngfilename, "rb");
	if (!png_FILE)
	{
		pngfilename = va("%s"PATHSEP"models"PATHSEP"%s", srb2path, filename);
		FIL_ForceExtension(pngfilename, ".png");
		png_FILE = fopen(pngfilename, "rb");

		if (!png_FILE)
			return 0;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
		PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Debug(DBG_RENDER, "PNG_Load: Error on initialize libpng\n");
		fclose(png_FILE);
		return 0;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		CONS_Debug(DBG_RENDER, "PNG_Load: Error on allocate for libpng\n");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(png_FILE);
		return 0;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{

		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		fclose(png_FILE);
		Z_Free(grpatch->mipmap->data);
		return 0;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof jmp_buf);
#endif

	png_init_io(png_ptr, png_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, MAX_PNG_DIMENSIONS, MAX_PNG_DIMENSIONS);
#endif

	png_read_info(png_ptr, png_info_ptr);

	png_get_IHDR(png_ptr, png_info_ptr, &width, &height, &bit_depth, &color_type,
	 NULL, NULL, NULL);
	if (!width || !height
	|| width > MAX_PNG_DIMENSIONS || height > MAX_PNG_DIMENSIONS
	|| (UINT64)width * (UINT64)height > (UINT64)MAX_PNG_PIXELS)
		png_error(png_ptr, "image dimensions exceed SRB2's safe PNG limits");

	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	else if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (png_get_valid(png_ptr, png_info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	else if (color_type != PNG_COLOR_TYPE_RGB_ALPHA && color_type != PNG_COLOR_TYPE_GRAY_ALPHA)
	{
#if PNG_LIBPNG_VER < 10207
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
#else
		png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
#endif
	}

	png_read_update_info(png_ptr, png_info_ptr);

	{
		png_uint_32 i, pitch = png_get_rowbytes(png_ptr, png_info_ptr);
		png_bytep PNG_image = Z_Malloc(pitch*height, PU_HWRMODELTEXTURE, &grpatch->mipmap->data);
		png_bytepp row_pointers = png_malloc(png_ptr, height * sizeof (png_bytep));
		for (i = 0; i < height; i++)
			row_pointers[i] = PNG_image + i*pitch;
		png_read_image(png_ptr, row_pointers);
		png_free(png_ptr, (png_voidp)row_pointers);
	}

	png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);

	fclose(png_FILE);
	*w = (int)width;
	*h = (int)height;
	return GL_TEXFMT_RGBA;
}
#endif

typedef struct
{
	UINT8 manufacturer;
	UINT8 version;
	UINT8 encoding;
	UINT8 bitsPerPixel;
	INT16 xmin;
	INT16 ymin;
	INT16 xmax;
	INT16 ymax;
	INT16 hDpi;
	INT16 vDpi;
	UINT8 colorMap[48];
	UINT8 reserved;
	UINT8 numPlanes;
	INT16 bytesPerLine;
	INT16 paletteInfo;
	INT16 hScreenSize;
	INT16 vScreenSize;
	UINT8 filler[54];
} PcxHeader;

static GLTextureFormat_t PCX_Load(const char *filename, int *w, int *h,
	GLPatch_t *grpatch)
{
	PcxHeader header;
#define PALSIZE 768
	UINT8 palette[PALSIZE];
	const UINT8 *pal;
	RGBA_t *image;
	size_t pw, ph, size, ptr = 0;
	INT32 ch, rep;
	FILE *file;

	char *pcxfilename = va("%s"PATHSEP"models"PATHSEP"%s", srb2home, filename);

	FIL_ForceExtension(pcxfilename, ".pcx");
	file = fopen(pcxfilename, "rb");
	if (!file)
	{
		pcxfilename = va("%s"PATHSEP"models"PATHSEP"%s", srb2path, filename);
		FIL_ForceExtension(pcxfilename, ".pcx");
		file = fopen(pcxfilename, "rb");
		if (!file)
			return 0;
	}

	if (fread(&header, sizeof (PcxHeader), 1, file) != 1)
	{
		fclose(file);
		return 0;
	}

	if (header.bitsPerPixel != 8)
	{
		fclose(file);
		return 0;
	}

	fseek(file, -PALSIZE, SEEK_END);

	pw = *w = header.xmax - header.xmin + 1;
	ph = *h = header.ymax - header.ymin + 1;
	image = Z_Malloc(pw*ph*4, PU_HWRMODELTEXTURE, &grpatch->mipmap->data);

	if (fread(palette, sizeof (UINT8), PALSIZE, file) != PALSIZE)
	{
		Z_Free(image);
		fclose(file);
		return 0;
	}
	fseek(file, sizeof (PcxHeader), SEEK_SET);

	size = pw * ph;
	while (ptr < size)
	{
		ch = fgetc(file);   
		if (ch >= 192)
		{
			rep = ch - 192;
			ch = fgetc(file);
		}
		else
		{
			rep = 1;
		}
		while (rep--)
		{
			pal = palette + ch*3;
			image[ptr].s.red   = *pal++;
			image[ptr].s.green = *pal++;
			image[ptr].s.blue  = *pal++;
			image[ptr].s.alpha = 0xFF;
			ptr++;
		}
	}
	fclose(file);
	return GL_TEXFMT_RGBA;
}




static void md2_loadTexture(md2_t *model)
{
	patch_t *patch;
	GLPatch_t *grPatch;
	const char *filename = model->filename;

	if (model->grpatch)
		patch = model->grpatch;
	else
		model->grpatch = patch = Patch_Create(0, 0);

	grPatch = (GLPatch_t *)Patch_AllocateHardwarePatch(patch);
	if (grPatch->mipmap->data)
		Z_Free(grPatch->mipmap->data);

	                                                                        
                                                                 
	grPatch = (GLPatch_t *)Patch_AllocateHardwarePatch(patch);

	if (!grPatch->mipmap->downloaded && !grPatch->mipmap->data)
	{
		int w = 0, h = 0;

#ifdef HAVE_PNG
		grPatch->mipmap->format = PNG_Load(filename, &w, &h, grPatch);
		if (grPatch->mipmap->format == 0)
#endif
		grPatch->mipmap->format = PCX_Load(filename, &w, &h, grPatch);
		if (grPatch->mipmap->format == 0)
		{
			model->notexturefile = true;  
			return;
		}

		grPatch->mipmap->downloaded = 0;
		grPatch->mipmap->flags = 0;

		patch->width = (INT16)w;
		patch->height = (INT16)h;
		grPatch->mipmap->width = (UINT16)w;
		grPatch->mipmap->height = (UINT16)h;


		if (!HWR_ShouldUsePaletteRendering())
		{
			UINT32 size;
			RGBA_t *image;

			image = grPatch->mipmap->data;
			size = w*h;
			while (size--)
			{
				V_CubeApply(&image->s.red, &image->s.green, &image->s.blue);
				image++;
			}
		}
	}
	HWD.pfnSetTexture(grPatch->mipmap);
}




static void md2_loadBlendTexture(md2_t *model)
{
	patch_t *patch;
	GLPatch_t *grPatch;
	char *filename = Z_Malloc(strlen(model->filename)+7, PU_STATIC, NULL);

	strcpy(filename, model->filename);
	FIL_ForceExtension(filename, "_blend.png");

	if (model->blendgrpatch)
		patch = model->blendgrpatch;
	else
		model->blendgrpatch = patch = Patch_Create(0, 0);

	grPatch = (GLPatch_t *)Patch_AllocateHardwarePatch(patch);
	if (grPatch->mipmap->data)
		Z_Free(grPatch->mipmap->data);

	grPatch = (GLPatch_t *)Patch_AllocateHardwarePatch(patch);

	if (!grPatch->mipmap->downloaded && !grPatch->mipmap->data)
	{
		int w = 0, h = 0;
#ifdef HAVE_PNG
		grPatch->mipmap->format = PNG_Load(filename, &w, &h, grPatch);
		if (grPatch->mipmap->format == 0)
#endif
		grPatch->mipmap->format = PCX_Load(filename, &w, &h, grPatch);
		if (grPatch->mipmap->format == 0)
		{
			model->noblendfile = true;  
			Z_Free(filename);
			return;
		}

		grPatch->mipmap->downloaded = 0;
		grPatch->mipmap->flags = 0;

		patch->width = (INT16)w;
		patch->height = (INT16)h;
		grPatch->mipmap->width = (UINT16)w;
		grPatch->mipmap->height = (UINT16)h;
	}
	HWD.pfnSetTexture(grPatch->mipmap);  

	Z_Free(filename);
}


static boolean nomd2s = false;

void HWR_InitModels(void)
{
	size_t i;

	for (i = 0; i < NUMSPRITES; i++)
	{
		md2_models[i].scale = -1.0f;
		md2_models[i].model = NULL;
		md2_models[i].grpatch = NULL;
		md2_models[i].notexturefile = false;
		md2_models[i].noblendfile = false;
		md2_models[i].found = false;
		md2_models[i].error = false;
	}

	if (numsprites && numskins)
		HWR_LoadModels();
}

void HWR_LoadModels(void)
{
	size_t i;
	INT32 s;
	FILE *f;

	char name[26], filename[32];



	float scale, offset;
	size_t prefixlen;

	if (nomd2s)
		return;


	if (numskins != (INT32)md2_numplayermodels)
	{
		md2_numplayermodels = (size_t)numskins;
		md2_playermodels = Z_Realloc(md2_playermodels, sizeof(md2_t) * md2_numplayermodels, PU_STATIC, NULL);

		for (s = 0; s < numskins; s++)
		{
			md2_playermodels[s].scale = -1.0f;
			md2_playermodels[s].model = NULL;
			md2_playermodels[s].grpatch = NULL;
			md2_playermodels[s].notexturefile = false;
			md2_playermodels[s].noblendfile = false;
			md2_playermodels[s].found = false;
			md2_playermodels[s].error = false;
		}
	}



	f = fopen(va("%s"PATHSEP"%s", srb2home, "models.dat"), "rt");

	if (!f)
	{
		f = fopen(va("%s"PATHSEP"%s", srb2path, "models.dat"), "rt");
		if (!f)
		{
			CONS_Printf("%s %s\n", M_GetText("Error while loading models.dat:"), strerror(errno));
			nomd2s = true;
			return;
		}
	}


	prefixlen = strlen(PLAYERMODELPREFIX);

	while (fscanf(f, "%25s %31s %f %f", name, filename, &scale, &offset) == 4)
	{
		char *skinname = name;
		size_t len = strlen(name);


		if (!strnicmp(name, PLAYERMODELPREFIX, prefixlen) && (len > prefixlen))
		{
			skinname += prefixlen;
			goto addskinmodel;
		}


		for (i = 0; i < numsprites; i++)
		{
			if (stricmp(name, sprnames[i]) == 0)
			{
				md2_models[i].scale = scale;
				md2_models[i].offset = offset;
				md2_models[i].found = true;
				strcpy(md2_models[i].filename, filename);
				goto modelfound;
			}
		}

addskinmodel:

		for (s = 0; s < numskins; s++)
		{
			if (stricmp(skinname, skins[s]->name) == 0)
			{
				md2_playermodels[s].scale = scale;
				md2_playermodels[s].offset = offset;
				md2_playermodels[s].found = true;
				strcpy(md2_playermodels[s].filename, filename);
				goto modelfound;
			}
		}

modelfound:

		continue;
	}

	fclose(f);
}






#define SETBRIGHTNESS(brightness,r,g,b) \
	brightness = (UINT8)(((1063*(UINT16)(r))/5000) + ((3576*(UINT16)(g))/5000) + ((361*(UINT16)(b))/5000))

static void HWR_CreateBlendedTexture(patch_t *gpatch, patch_t *blendgpatch, GLMipmap_t *grMipmap, INT32 skinnum, skincolornum_t color)
{
	GLPatch_t *hwrPatch = gpatch->hardware;
	GLPatch_t *hwrBlendPatch = blendgpatch->hardware;
	UINT16 w = gpatch->width, h = gpatch->height;
	UINT32 size = w*h;
	RGBA_t *image, *blendimage, *cur, blendcolor;
	UINT16 translation[16];  
	UINT8 cutoff[16];  
	UINT8 translen = 0;
	UINT8 i;

	blendcolor = V_GetColor(0);  
	memset(translation, 0, sizeof(translation));
	memset(cutoff, 0, sizeof(cutoff));

	if (grMipmap->width == 0)
	{
		grMipmap->width = gpatch->width;
		grMipmap->height = gpatch->height;


		grMipmap->flags = 0;


		grMipmap->format = GL_TEXFMT_RGBA;
	}

	if (grMipmap->data)
	{
		Z_Free(grMipmap->data);
		grMipmap->data = NULL;
	}

	cur = Z_Malloc(size*4, PU_HWRMODELTEXTURE, &grMipmap->data);
	memset(cur, 0x00, size*4);

	image = hwrPatch->mipmap->data;
	blendimage = hwrBlendPatch->mipmap->data;


	if (skinnum == TC_METALSONIC)
		color = SKINCOLOR_COBALT;

	if (color != SKINCOLOR_NONE && color < numskincolors)
	{
		UINT8 numdupes = 1;

		translation[translen] = skincolors[color].ramp[0];
		cutoff[translen] = 255;

		for (i = 1; i < 16; i++)
		{
			if (translation[translen] == skincolors[color].ramp[i])
			{
				numdupes++;
				continue;
			}

			if (translen > 0)
			{
				cutoff[translen] = cutoff[translen-1] - (256 / (16 / numdupes));
			}

			numdupes = 1;
			translen++;

			translation[translen] = (UINT16)skincolors[color].ramp[i];
		}

		translen++;
	}

	while (size--)
	{
		if (skinnum == TC_ALLWHITE)
		{

			cur->s.red = cur->s.green = cur->s.blue = 255;
			cur->s.alpha = image->s.alpha;
		}
		else
		{

			if (blendimage == NULL)
			{
				cur->rgba = image->rgba;
				goto skippixel;
			}


			if (skinnum == TC_DASHMODE)
			{
				if (image->s.alpha == 0 && blendimage->s.alpha == 0)
				{

					cur->rgba = image->rgba;
				}
				else
				{
					UINT8 ialpha = 255 - blendimage->s.alpha, balpha = blendimage->s.alpha;
					RGBA_t icolor = *image, bcolor;

					memset(&bcolor, 0x00, sizeof(RGBA_t));

					if (blendimage->s.alpha)
					{
						bcolor.s.blue = 0;
						bcolor.s.red = 255;
						bcolor.s.green = (blendimage->s.red + blendimage->s.green + blendimage->s.blue) / 3;
					}

					if (image->s.alpha && image->s.red > image->s.green << 1)  
					{
						icolor.s.red = image->s.blue;
						icolor.s.blue = image->s.red;
					}

					cur->s.red = (ialpha * icolor.s.red + balpha * bcolor.s.red)/255;
					cur->s.green = (ialpha * icolor.s.green + balpha * bcolor.s.green)/255;
					cur->s.blue = (ialpha * icolor.s.blue + balpha * bcolor.s.blue)/255;
					cur->s.alpha = image->s.alpha;
				}
			}
			else
			{

				UINT16 brightness;

				if (translen <= 0)
				{
					cur->rgba = image->rgba;
					goto skippixel;
				}


				if (skinnum == TC_RAINBOW)
				{
					if (image->s.alpha == 0 && blendimage->s.alpha == 0)
					{
						cur->rgba = image->rgba;
						goto skippixel;
					}
					else
					{
						UINT16 imagebright, blendbright;
						SETBRIGHTNESS(imagebright,image->s.red,image->s.green,image->s.blue);
						SETBRIGHTNESS(blendbright,blendimage->s.red,blendimage->s.green,blendimage->s.blue);

						brightness = (imagebright*(255-blendimage->s.alpha))/255 + (blendbright*blendimage->s.alpha)/255;
					}
				}
				else
				{
					if (blendimage->s.alpha == 0)
					{
						cur->rgba = image->rgba;
						goto skippixel;  
					}
					else
					{
						SETBRIGHTNESS(brightness,blendimage->s.red,blendimage->s.green,blendimage->s.blue);
					}
				}



				{
					RGBA_t nextcolor;
					UINT8 firsti, secondi, mul, mulmax;
					INT32 r, g, b;



					if (skinnum == TC_RAINBOW)
					{
						UINT16 brightdif = 256;
						UINT8 colorbrightnesses[16];
						INT32 compare, m, d;


						if (brightness > 253 || brightness < 2)
						{
							cur->rgba = image->rgba;
							cur++; image++; blendimage++;
							continue;
						}

						firsti = 0;
						mul = 0;
						mulmax = 1;

						for (i = 0; i < translen; i++)
						{
							RGBA_t tempc = V_GetColor(translation[i]);
							SETBRIGHTNESS(colorbrightnesses[i], tempc.s.red, tempc.s.green, tempc.s.blue);  
						}

						for (i = 0; i < translen; i++)
						{
							if (brightness > colorbrightnesses[i])  
								continue;

							compare = abs((INT16)(colorbrightnesses[i]) - (INT16)(brightness));

							if (compare < brightdif)
							{
								brightdif = (UINT16)compare;
								firsti = i;  
							}
						}

						secondi = firsti+1;  
						if (secondi >= translen)
						{
							m = (INT16)brightness;  
							d = (INT16)colorbrightnesses[firsti];  
						}
						else
						{
							m = (INT16)brightness - (INT16)colorbrightnesses[secondi];
							d = (INT16)colorbrightnesses[firsti] - (INT16)colorbrightnesses[secondi];
						}

						if (m >= d)
							m = d-1;

						mulmax = 16;


						if (m <= 0 || d <= 0)
							mul = 0;
						else
							mul = (mulmax-1) - ((m * mulmax) / d);
					}
					else
					{

						firsti = 0;

						for (i = 1; i < translen; i++)
						{
							if (brightness >= cutoff[i])
								break;
							firsti = i;
						}

						secondi = firsti+1;

						mulmax = cutoff[firsti];
						if (secondi < translen)
							mulmax -= cutoff[secondi];

						mul = cutoff[firsti] - brightness;
					}

					blendcolor = V_GetColor(translation[firsti]);

					if (secondi >= translen)
						mul = 0;

					if (mul > 0)  
					{
#if 0
						if (secondi >= translen)
						{

							nextcolor = V_GetColor(31);
						}
						else
#endif
							nextcolor = V_GetColor(translation[secondi]);


						r = (INT32)(nextcolor.s.red - blendcolor.s.red);
						g = (INT32)(nextcolor.s.green - blendcolor.s.green);
						b = (INT32)(nextcolor.s.blue - blendcolor.s.blue);


						r = ((mul * r) / mulmax);
						g = ((mul * g) / mulmax);
						b = ((mul * b) / mulmax);


						blendcolor.s.red += r;
						blendcolor.s.green += g;
						blendcolor.s.blue += b;
					}
				}

				if (skinnum == TC_RAINBOW)
				{
					UINT32 tempcolor;
					UINT16 colorbright;

					SETBRIGHTNESS(colorbright,blendcolor.s.red,blendcolor.s.green,blendcolor.s.blue);
					if (colorbright == 0)
						colorbright = 1;  

					tempcolor = (brightness * blendcolor.s.red) / colorbright;
					tempcolor = min(255, tempcolor);
					cur->s.red = (UINT8)tempcolor;

					tempcolor = (brightness * blendcolor.s.green) / colorbright;
					tempcolor = min(255, tempcolor);
					cur->s.green = (UINT8)tempcolor;

					tempcolor = (brightness * blendcolor.s.blue) / colorbright;
					tempcolor = min(255, tempcolor);
					cur->s.blue = (UINT8)tempcolor;
					cur->s.alpha = image->s.alpha;
				}
				else
				{

					INT32 tempcolor;

					tempcolor = ((image->s.red * (255-blendimage->s.alpha)) / 255) + ((blendcolor.s.red * blendimage->s.alpha) / 255);
					tempcolor = min(255, tempcolor);
					cur->s.red = (UINT8)tempcolor;

					tempcolor = ((image->s.green * (255-blendimage->s.alpha)) / 255) + ((blendcolor.s.green * blendimage->s.alpha) / 255);
					tempcolor = min(255, tempcolor);
					cur->s.green = (UINT8)tempcolor;

					tempcolor = ((image->s.blue * (255-blendimage->s.alpha)) / 255) + ((blendcolor.s.blue * blendimage->s.alpha) / 255);
					tempcolor = min(255, tempcolor);
					cur->s.blue = (UINT8)tempcolor;
					cur->s.alpha = image->s.alpha;
				}

skippixel:


				if (skinnum == TC_METALSONIC)
				{

					if (cur->s.alpha > 0 && cur->s.red == 0 && cur->s.green == 0 && cur->s.blue < 255 && cur->s.blue > 31)
					{

						cur->s.red = cur->s.green = (255 - cur->s.blue);
						cur->s.blue = 255;
					}

					cur->s.alpha = image->s.alpha;
				}
				else if (skinnum == TC_BOSS)
				{

					if ((image->s.red == image->s.green) && (image->s.green == image->s.blue) && image->s.blue < 127)
					{

						cur->s.red = cur->s.green = cur->s.blue = (255 - image->s.blue);
					}
				}
			}
		}

		cur++; image++;

		if (blendimage != NULL)
			blendimage++;
	}

	return;
}

#undef SETBRIGHTNESS

static void HWR_GetBlendedTexture(patch_t *patch, patch_t *blendpatch, INT32 skinnum, const UINT8 *colormap, skincolornum_t color)
{

	GLPatch_t *grPatch = patch->hardware;
	GLPatch_t *grBlendPatch = NULL;
	GLMipmap_t *grMipmap, *newMipmap;

	if (blendpatch == NULL || colormap == colormaps || colormap == NULL)
	{

		HWD.pfnSetTexture(grPatch->mipmap);
		return;
	}

	if ((blendpatch && (grBlendPatch = blendpatch->hardware) && grBlendPatch->mipmap->format)
		&& (patch->width != blendpatch->width || patch->height != blendpatch->height))
	{

		HWD.pfnSetTexture(grPatch->mipmap);
		return;
	}



	for (grMipmap = grPatch->mipmap; grMipmap->nextcolormap; )
	{
		grMipmap = grMipmap->nextcolormap;
		if (grMipmap->colormap && grMipmap->colormap->source == colormap)
		{
			if (grMipmap->downloaded && grMipmap->data)
			{
				if (memcmp(grMipmap->colormap->data, colormap, 256 * sizeof(UINT8)))
				{
					M_Memcpy(grMipmap->colormap->data, colormap, 256 * sizeof(UINT8));
					HWR_CreateBlendedTexture(patch, blendpatch, grMipmap, skinnum, color);
					HWD.pfnUpdateTexture(grMipmap);
				}
				else
					HWD.pfnSetTexture(grMipmap);  

				Z_ChangeTag(grMipmap->data, PU_HWRMODELTEXTURE_UNLOCKED);
				return;
			}
		}
	}








	newMipmap = calloc(1, sizeof (*newMipmap));
	if (newMipmap == NULL)
		I_Error("%s: Out of memory", "HWR_GetBlendedTexture");
	grMipmap->nextcolormap = newMipmap;

	newMipmap->colormap = Z_Calloc(sizeof(*newMipmap->colormap), PU_HWRPATCHCOLMIPMAP, NULL);
	newMipmap->colormap->source = colormap;
	M_Memcpy(newMipmap->colormap->data, colormap, 256 * sizeof(UINT8));

	HWR_CreateBlendedTexture(patch, blendpatch, newMipmap, skinnum, color);

	HWD.pfnSetTexture(newMipmap);
	Z_ChangeTag(newMipmap->data, PU_HWRMODELTEXTURE_UNLOCKED);
}

static boolean HWR_AllowModel(mobj_t *mobj)
{

	if (mobj->sprite2 == SPR2_SIGN || mobj->state-states == S_PLAY_SIGN)
		return false;


	return true;
}

static boolean HWR_CanInterpolateModel(mobj_t *mobj, model_t *model)
{
	if (cv_glmodelinterpolation.value == 2)  
		return true;
	return model->interpolate[(mobj->frame & FF_FRAMEMASK)];
}

static boolean HWR_CanInterpolateSprite2(modelspr2frames_t *spr2frame)
{
	if (cv_glmodelinterpolation.value == 2)  
		return true;
	return spr2frame->interpolate;
}

static modelspr2frames_t *HWR_GetModelSprite2Frames(md2_t *md2, UINT16 spr2)
{
	if (!md2 || !md2->model)
		return NULL;

	boolean is_super = spr2 & SPR2F_SUPER;

	spr2 &= SPR2F_MASK;

	if (spr2 >= free_spr2)
		return NULL;

	if (is_super)
	{
		modelspr2frames_t *frames = md2->model->superspr2frames;
		if (frames && md2->model->superspr2frames[spr2].numframes)
			return &md2->model->superspr2frames[spr2];
	}

	if (md2->model->spr2frames[spr2].numframes)
		return &md2->model->spr2frames[spr2];

	return NULL;
}

static UINT16 HWR_GetModelSprite2Num(md2_t *md2, skin_t *skin, UINT16 spr2, player_t *player)
{
	UINT16 super = 0;
	UINT8 i = 0;

	if (!md2 || !md2->model || !skin)
		return 0;

	while (!HWR_GetModelSprite2Frames(md2, spr2)
		&& spr2 != SPR2_STND
		&& ++i < 32)  
	{
		if (spr2 & SPR2F_SUPER)
		{
			super = SPR2F_SUPER;
			spr2 &= ~SPR2F_SUPER;
			continue;
		}

		switch(spr2)
		{

		case SPR2_JUMP:
			spr2 = ((player
					? player->charflags
					: skin->flags)
					& SF_NOJUMPSPIN) ? SPR2_SPNG : SPR2_ROLL;
			break;
		case SPR2_TIRE:
			spr2 = ((player
					? player->charability
					: skin->ability)
					== CA_SWIM) ? SPR2_SWIM : SPR2_FLY;
			break;

		default:
			spr2 = spr2defaults[spr2];
			break;
		}

		spr2 |= super;
	}

	if (i >= 32)  
		spr2 = 0;

	return spr2;
}


static void adjustTextureCoords(model_t *model, patch_t *patch)
{
	int i;
	GLPatch_t *gpatch = ((GLPatch_t *)patch->hardware);

	if (!gpatch)
		return;

	for (i = 0; i < model->numMeshes; i++)
	{
		int j;
		mesh_t *mesh = &model->meshes[i];
		int numVertices;
		float *uvReadPtr = mesh->originaluvs;
		float *uvWritePtr;


		if (!mesh->frames && !mesh->tinyframes) continue;

		if (mesh->frames)  
			numVertices = mesh->numTriangles * 3;
		else
			numVertices = mesh->numVertices;



		if (mesh->uvs == mesh->originaluvs)
			mesh->uvs = Z_Malloc(numVertices * 2 * sizeof(float), PU_STATIC, NULL);

		uvWritePtr = mesh->uvs;



		for (j = 0; j < numVertices; j++)
		{
			*uvWritePtr++ = *uvReadPtr++ * gpatch->max_s;
			*uvWritePtr++ = *uvReadPtr++ * gpatch->max_t;
		}
	}

	model->max_s = gpatch->max_s;
	model->max_t = gpatch->max_t;
}

static INT32 GetAnimDuration(mobj_t *mobj)  
{
	player_t *player = mobj->player;
	INT32 tics = mobj->state->tics;

	if (!(mobj->frame & FF_ANIMATE) && mobj->anim_duration)  
		return mobj->anim_duration;

	if (!player && mobj->type == MT_TAILSOVERLAY && mobj->tracer)  
		player = mobj->tracer->player;
	if (player)
	{
		if (player->panim == PA_EDGE && (player->charflags & SF_FASTEDGE))
			tics = 2;
		else if (player->powers[pw_tailsfly] && (!(player->mo->eflags & MFE_UNDERWATER) || (mobj->type == MT_PLAYER)))  
		{
			if (player->fly1 > 0)
				tics = 1;
			else if (!(player->mo->eflags & MFE_UNDERWATER))
				tics = 2;
			else
				tics = 4;
		}
		else if (!(disableSpeedAdjust || player->charflags & SF_NOSPEEDADJUST))
		{
			fixed_t speed; 
			if (player->panim == PA_FALL)
			{
				speed = FixedDiv(abs(mobj->momz), mobj->scale);
				if (speed < 10<<FRACBITS)
					tics = 4;
				else if (speed < 20<<FRACBITS)
					tics = 3;
				else if (speed < 30<<FRACBITS)
					tics = 2;
				else
					tics = 1;
			}
			else if (player->panim == PA_ABILITY2 && player->charability2 == CA2_SPINDASH)
			{
				fixed_t step = (player->maxdash - player->mindash)/4;
				speed = (player->dashspeed - player->mindash);
				if (speed > 3*step)
					tics = 1;
				else if (speed > step)
					tics = 2;
				else
					tics = 3;
			}
			else
			{
				speed = FixedDiv(player->speed, FixedMul(mobj->scale, player->mo->movefactor));
				if (player->panim == PA_ROLL || player->panim == PA_JUMP)
				{
					if (speed > 16<<FRACBITS)
						tics = 1;
					else
						tics = 2;
				}
				else if (P_IsObjectOnGround(mobj) || ((player->charability == CA_FLOAT || player->charability == CA_SLOWFALL) && player->secondjump == 1) || player->powers[pw_super])  
				{
					if (player->panim == PA_WALK)
					{
						if (speed > 12<<FRACBITS)
							tics = 2;
						else if (speed > 6<<FRACBITS)
							tics = 3;
						else
							tics = 4;
					}
					else if ((player->panim == PA_RUN) || (player->panim == PA_DASH))
					{
						if (speed > 52<<FRACBITS)
							tics = 1;
						else
							tics = 2;
					}
				}
			}
		}
	}
	return tics;
}





boolean HWR_DrawModel(gl_vissprite_t *spr)
{
	md2_t *md2;

	char filename[64];
	INT32 frame = 0;
	INT32 nextFrame = -1;
	modelspr2frames_t *spr2frames = NULL;
	FTransform p;
	FSurfaceInfo Surf;

	if (!cv_glmodels.value)
		return false;

	if (spr->precip)
		return false;


	if (!HWR_AllowModel(spr->mobj))
		return false;

	memset(&p, 0x00, sizeof(FTransform));



	if (spr->mobj->subsector)
	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 255;
		extracolormap_t *colormap = NULL;

		if (sector->numlights)
		{
			INT32 light;

			light = R_GetPlaneLight(sector, spr->mobj->z + spr->mobj->height, false);  

			if (R_ThingIsFullDark(spr->mobj))
				lightlevel = 0;
			else if (R_ThingIsSemiBright(spr->mobj))
				lightlevel = 128 + (*sector->lightlist[light].lightlevel>>1);
			else if (!R_ThingIsFullBright(spr->mobj))
				lightlevel = max(min(255, *sector->lightlist[light].lightlevel), 0);

			if (*sector->lightlist[light].extra_colormap)
				colormap = *sector->lightlist[light].extra_colormap;
		}
		else
		{
			if (R_ThingIsFullDark(spr->mobj))
				lightlevel = 0;
			else if (R_ThingIsSemiBright(spr->mobj))
				lightlevel = 128 + (sector->lightlevel>>1);
			else if (!R_ThingIsFullBright(spr->mobj))
				lightlevel = max(min(255, sector->lightlevel), 0);

			if (sector->extra_colormap)
				colormap = sector->extra_colormap;
		}

		HWR_Lighting(&Surf, lightlevel, colormap);
	}
	else
		Surf.PolyColor.rgba = 0xFFFFFFFF;


	{
		patch_t *gpatch, *blendgpatch;
		GLPatch_t *hwrPatch = NULL, *hwrBlendPatch = NULL;
		float durs = GetAnimDuration(spr->mobj);
		float tics = (float)spr->mobj->tics;
		const boolean papersprite = (R_ThingIsPaperSprite(spr->mobj) && !R_ThingIsFloorSprite(spr->mobj));
		const UINT8 flip = (UINT8)(!(spr->mobj->eflags & MFE_VERTICALFLIP) != !R_ThingVerticallyFlipped(spr->mobj));
		const UINT8 hflip = (UINT8)(!(spr->mobj->mirrored) != !R_ThingHorizontallyFlipped(spr->mobj));
		spritedef_t *sprdef;
		UINT16 spr2 = 0;
		spriteframe_t *sprframe;
		INT32 mod;
		interpmobjstate_t interp;

		if (R_UsingFrameInterpolation() && !paused)
		{
			R_InterpolateMobjState(spr->mobj, rendertimefrac, &interp);
		}
		else
		{
			R_InterpolateMobjState(spr->mobj, FRACUNIT, &interp);
		}


		if (tics > durs)
			durs = tics;


		fixed_t newalpha = spr->mobj->alpha;
		if ((spr->mobj->flags2 & MF2_LINKDRAW) && spr->mobj->tracer)
			newalpha = spr->mobj->tracer->alpha;

		INT32 blendmode;
		if (spr->mobj->frame & FF_BLENDMASK)
			blendmode = ((spr->mobj->frame & FF_BLENDMASK) >> FF_BLENDSHIFT) + 1;
		else
			blendmode = spr->mobj->blendmode;

		if (spr->mobj->frame & FF_TRANSMASK)
			Surf.PolyFlags = HWR_SurfaceBlend(blendmode, (spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
		else
		{
			Surf.PolyColor.s.alpha = (spr->mobj->flags2 & MF2_SHADOW) ? 0x40 : 0xff;
			                                                                
                                                                       
                                                             
			Surf.PolyFlags = (newalpha < FRACUNIT && (blendmode <= AST_COPY || blendmode >= AST_OVERLAY))
				? PF_Translucent : HWR_GetBlendModeFlag(blendmode);
		}

		Surf.PolyColor.s.alpha = FixedMul(newalpha, Surf.PolyColor.s.alpha);






		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)  
		{
			UINT8 skinnum = ((skin_t*)spr->mobj->skin)->skinnum;
			md2 = &md2_playermodels[skinnum];
		}
		else
		{
			md2 = &md2_models[spr->mobj->sprite];
		}



		gpatch = md2->grpatch;
		if (gpatch)
			hwrPatch = ((GLPatch_t *)gpatch->hardware);

		if (!gpatch || !hwrPatch || !hwrPatch->mipmap
		|| ((!hwrPatch->mipmap->format || !hwrPatch->mipmap->downloaded) && !md2->notexturefile))
			md2_loadTexture(md2);


		gpatch = md2->grpatch;
		if (gpatch)
			hwrPatch = ((GLPatch_t *)gpatch->hardware);


		blendgpatch = md2->blendgrpatch;
		if (blendgpatch)
			hwrBlendPatch = ((GLPatch_t *)blendgpatch->hardware);

		if ((gpatch && hwrPatch && hwrPatch->mipmap && hwrPatch->mipmap->format)  
			&& (!blendgpatch || !hwrBlendPatch || !hwrBlendPatch->mipmap
			|| ((!hwrBlendPatch->mipmap->format || !hwrBlendPatch->mipmap->downloaded) && !md2->noblendfile)))
			md2_loadBlendTexture(md2);


		blendgpatch = md2->blendgrpatch;
		if (blendgpatch)
			hwrBlendPatch = ((GLPatch_t *)blendgpatch->hardware);

		if (md2->error)
			return false;  
		if (!md2->model)
		{

			sprintf(filename, "models/%s", md2->filename);
			md2->model = md2_readModel(filename);

			if (md2->model)
			{
				md2_printModelInfo(md2->model);


				if (!gpatch || !hwrPatch || !hwrPatch->mipmap || !hwrPatch->mipmap->format)
					adjustTextureCoords(md2->model, spr->gpatch);

				md2->model->vbo_max_s = md2->model->max_s;
				md2->model->vbo_max_t = md2->model->max_t;
				HWD.pfnCreateModelVBOs(md2->model);
			}
			else
			{

				md2->error = true;  
				return false;
			}
		}




		if (gpatch && hwrPatch && hwrPatch->mipmap && hwrPatch->mipmap->format)  
		{
			INT32 skinnum = TC_DEFAULT;

			if ((spr->mobj->flags & (MF_ENEMY|MF_BOSS)) && (spr->mobj->flags2 & MF2_FRET) && !(spr->mobj->flags & MF_GRENADEBOUNCE) && (leveltime & 1))  
			{
				if (spr->mobj->type == MT_CYBRAKDEMON || spr->mobj->colorized)
					skinnum = TC_ALLWHITE;
				else if (spr->mobj->type == MT_METALSONIC_BATTLE)
					skinnum = TC_METALSONIC;
				else
					skinnum = TC_BOSS;
			}
			else if ((skincolornum_t)spr->mobj->color != SKINCOLOR_NONE)
			{
				if (spr->mobj->colorized)
					skinnum = TC_RAINBOW;
				else if (spr->mobj->player && spr->mobj->player->dashmode >= DASHMODE_THRESHOLD
					&& (spr->mobj->player->charflags & SF_DASHMODE)
					&& ((leveltime/2) & 1))
				{
					if (spr->mobj->player->charflags & SF_MACHINE)
						skinnum = TC_DASHMODE;
					else
						skinnum = TC_RAINBOW;
				}
				else if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
					skinnum = ((skin_t*)spr->mobj->skin)->skinnum;
				else
					skinnum = TC_DEFAULT;
			}


			HWR_GetBlendedTexture(gpatch, blendgpatch, skinnum, spr->colormap, (skincolornum_t)spr->mobj->color);
		}
		else  
		{





			if (memcmp(&(hwrPatch->max_s), &(md2->model->max_s), sizeof(md2->model->max_s)) != 0 ||
				memcmp(&(hwrPatch->max_t), &(md2->model->max_t), sizeof(md2->model->max_t)) != 0)
				adjustTextureCoords(md2->model, spr->gpatch);
			HWR_GetMappedSpritePatch(spr->gpatch, spr->colormap);
		}

		if (spr->mobj->frame & FF_ANIMATE)
		{

			durs = (float)spr->mobj->state->var2;
			tics = (float)spr->mobj->anim_duration;
		}

		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
			sprdef = P_GetSkinSpritedef(spr->mobj->skin, spr->mobj->sprite2);
		else
			sprdef = &sprites[spr->mobj->sprite];

		frame = (spr->mobj->frame & FF_FRAMEMASK);
		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
		{
			spr2 = HWR_GetModelSprite2Num(md2, spr->mobj->skin, spr->mobj->sprite2, spr->mobj->player);
			spr2frames = HWR_GetModelSprite2Frames(md2, spr2);
		}
		if (spr2frames)
		{
			spritedef_t *defaultdef = P_GetSkinSpritedef(spr->mobj->skin, spr2);
			mod = spr2frames->numframes;
#ifndef DONTHIDEDIFFANIMLENGTH  
			if (mod > (INT32)defaultdef->numframes)
				mod = defaultdef->numframes;
#endif
			if (!mod)
				mod = 1;
			frame = spr2frames->frames[frame % mod];
		}
		else
		{
			mod = md2->model->meshes[0].numFrames;
			if (!mod)
				mod = 1;
		}

#ifdef USE_MODEL_NEXTFRAME

		tics -= FixedToFloat(rendertimefrac);

#define INTERPOLERATION_LIMIT (TICRATE * 0.25f)

		if (cv_glmodelinterpolation.value && tics <= durs && tics <= INTERPOLERATION_LIMIT)
		{
			if (durs > INTERPOLERATION_LIMIT)
				durs = INTERPOLERATION_LIMIT;

			if (spr2frames)
			{
				UINT16 next_spr2 = P_GetStateSprite2(&states[spr->mobj->state->nextstate]);


				next_spr2 = P_ApplySuperFlagToSprite2(next_spr2, spr->mobj);

				if (HWR_CanInterpolateSprite2(spr2frames)
					&& (spr->mobj->frame & FF_ANIMATE
					|| (spr->mobj->state->nextstate != S_NULL
					&& states[spr->mobj->state->nextstate].sprite == SPR_PLAY
					&& ((P_GetSkinSprite2(spr->mobj->skin, next_spr2, spr->mobj->player) == spr->mobj->sprite2)))))
				{
					nextFrame = (spr->mobj->frame & FF_FRAMEMASK) + 1;
					if (nextFrame >= mod)
					{
						if (spr->mobj->state->frame & FF_SPR2ENDSTATE)
							nextFrame--;
						else
							nextFrame = 0;
					}
					if (frame || !(spr->mobj->state->frame & FF_SPR2ENDSTATE))
						nextFrame = spr2frames->frames[nextFrame];
					else
						nextFrame = -1;
				}
			}
			else if (HWR_CanInterpolateModel(spr->mobj, md2->model))
			{

				if (spr->mobj->frame & FF_ANIMATE)
				{
					nextFrame = (spr->mobj->frame & FF_FRAMEMASK) + 1;
					if (nextFrame >= (INT32)(spr->mobj->state->var1 + (spr->mobj->state->frame & FF_FRAMEMASK)))
						nextFrame = (spr->mobj->state->frame & FF_FRAMEMASK) % mod;
				}
				else
				{
					if (spr->mobj->state->nextstate != S_NULL && states[spr->mobj->state->nextstate].sprite != SPR_NULL
					&& !(spr->mobj->player && (spr->mobj->state->nextstate == S_PLAY_WAIT) && spr->mobj->state == &states[S_PLAY_STND]))
						nextFrame = (states[spr->mobj->state->nextstate].frame & FF_FRAMEMASK) % mod;
				}
			}
		}
#undef INTERPOLERATION_LIMIT
#endif

		if (spr->mobj->type == MT_OVERLAY)  
			R_ThingOffsetOverlay(spr->mobj, &interp.x, &interp.y);

		p.x = FIXED_TO_FLOAT(interp.x);
		p.y = FIXED_TO_FLOAT(interp.y)+md2->offset;

		if (flip)
			p.z = FIXED_TO_FLOAT(interp.z + interp.height);
		else
			p.z = FIXED_TO_FLOAT(interp.z);

		sprframe = &sprdef->spriteframes[spr->mobj->frame & FF_FRAMEMASK];

		if (sprframe->rotate || papersprite)
		{
			fixed_t anglef = AngleFixed(interp.angle);

			p.angley = FIXED_TO_FLOAT(anglef);
		}
		else
		{
			const fixed_t anglef = AngleFixed((R_PointToAngle(interp.x, interp.y))-ANGLE_180);
			p.angley = FIXED_TO_FLOAT(anglef);
		}

		{
			fixed_t anglef = AngleFixed(R_ModelRotationAngle(&interp));

			p.rollangle = 0.0f;

			if (anglef)
			{
				fixed_t camAngleDiff = AngleFixed(viewangle) - FLOAT_TO_FIXED(p.angley);  

				p.rollangle = FIXED_TO_FLOAT(anglef);
				p.roll = true;


				p.centerx = FIXED_TO_FLOAT(interp.radius / 2);
				p.centery = FIXED_TO_FLOAT(interp.height / 2);


				p.rollx = FIXED_TO_FLOAT(FINECOSINE(FixedAngle(camAngleDiff) >> ANGLETOFINESHIFT));
				p.rollz = FIXED_TO_FLOAT(FINESINE(FixedAngle(camAngleDiff) >> ANGLETOFINESHIFT));
			}
		}

#if 0
		p.anglez = FIXED_TO_FLOAT(AngleFixed(interp.pitch));
		p.anglex = FIXED_TO_FLOAT(AngleFixed(interp.roll));
#else
		p.anglez = 0.f;
		p.anglex = 0.f;
#endif

		p.flip = atransform.flip;
		p.mirror = atransform.mirror;

		if (HWR_UseShader())
			HWD.pfnSetShader(HWR_GetShaderFromTarget(SHADER_MODEL));
		{
			float this_scale = FIXED_TO_FLOAT(interp.scale);

			float xs = this_scale * FIXED_TO_FLOAT(interp.spritexscale);
			float ys = this_scale * FIXED_TO_FLOAT(interp.spriteyscale);

			float ox = xs * FIXED_TO_FLOAT(interp.spritexoffset);
			float oy = ys * FIXED_TO_FLOAT(interp.spriteyoffset);


			p.x -= ox * gl_viewsin;
			p.y += ox * gl_viewcos;
			p.z += oy;

			HWD.pfnDrawModel(md2->model, frame, durs, tics, nextFrame, &p, md2->scale * xs, md2->scale * ys, flip, hflip, &Surf);
		}
	}
	return true;
}

#endif  
