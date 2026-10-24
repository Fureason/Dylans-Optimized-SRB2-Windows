// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "doomdef.h"
#include "g_game.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "p_local.h"
#include "m_misc.h"
#include "r_data.h"
#include "r_textures.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_setup.h"  
#include "byteptr.h"
#include "dehacked.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"  
#endif

#include <errno.h>











INT32 numtextures = 0;  


texture_t **textures = NULL;
column_t **texturecolumns;  
UINT8 **texturecache;  

INT32 *texturewidth;
fixed_t *textureheight;  

INT32 *texturetranslation;


static struct {
	char name[9];
	UINT32 hash;
	INT32 id;
	UINT8 type;
} *tidcache = NULL;
static INT32 tidcachelen = 0;















static void R_DrawColumnInCache(column_t *column, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight, UINT8 *opaque_pixels)
{
	INT32 count, position;
	UINT8 *source;
	INT32 originy = originPatch->originy;

	(void)patchheight;  

	for (unsigned i = 0; i < column->num_posts; i++)
	{
		post_t *post = &column->posts[i];
		source = column->pixels + post->data_offset;
		count = post->length;
		position = originy + post->topdelta;

		if (position < 0)
		{
			count += position;
			source -= position;  
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		if (count > 0)
		{
			M_Memcpy(cache + position, source, count);
			memset(opaque_pixels + position, true, count);
		}
	}
}





static void R_DrawFlippedColumnInCache(column_t *column, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight, UINT8 *opaque_pixels)
{
	INT32 count, position;
	UINT8 *source, *dest;
	INT32 originy = originPatch->originy;
	INT32 topdelta;
	UINT8 *is_opaque;

	for (unsigned i = 0; i < column->num_posts; i++)
	{
		post_t *post = &column->posts[i];
		topdelta = patchheight - post->length - post->topdelta;
		source = column->pixels + post->data_offset + (post->length - 1);
		count = post->length;
		position = originy + topdelta;

		if (position < 0)
		{
			count += position;
			source += position;  
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		dest = cache + position;
		is_opaque = opaque_pixels + position;

		if (count > 0)
		{
			for (; dest < cache + position + count; --source, dest++, is_opaque++)
			{
				*dest = *source;
				*is_opaque = true;
			}
		}
	}
}





static void R_DrawBlendColumnInCache(column_t *column, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight, UINT8 *opaque_pixels)
{
	INT32 count, position;
	UINT8 *source, *dest;
	INT32 originy = originPatch->originy;
	UINT8 *is_opaque;

	(void)patchheight;  

	for (unsigned i = 0; i < column->num_posts; i++)
	{
		post_t *post = &column->posts[i];
		source = column->pixels + post->data_offset;
		count = post->length;
		position = originy + post->topdelta;

		if (position < 0)
		{
			count += position;
			source -= position;  
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		dest = cache + position;
		is_opaque = opaque_pixels + position;

		if (count > 0)
		{
			for (; dest < cache + position + count; source++, dest++, is_opaque++)
			{
				if (originPatch->alpha <= ASTTextureBlendingThreshold[1] && !(*is_opaque))
					continue;
				*dest = ASTBlendPaletteIndexes(*dest, *source, originPatch->style, originPatch->alpha);
				*is_opaque = true;
			}
		}
	}
}





static void R_DrawBlendFlippedColumnInCache(column_t *column, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight, UINT8 *opaque_pixels)
{
	INT32 count, position;
	UINT8 *source, *dest;
	INT32 originy = originPatch->originy;
	INT32 topdelta;
	UINT8 *is_opaque;

	for (unsigned i = 0; i < column->num_posts; i++)
	{
		post_t *post = &column->posts[i];
		topdelta = patchheight - post->length - post->topdelta;
		source = column->pixels + post->data_offset + (post->length - 1);
		count = post->length;
		position = originy + topdelta;

		if (position < 0)
		{
			count += position;
			source += position;  
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		dest = cache + position;
		is_opaque = opaque_pixels + position;

		if (count > 0)
		{
			for (; dest < cache + position + count; --source, dest++, is_opaque++)
			{
				if (originPatch->alpha <= ASTTextureBlendingThreshold[1] && !(*is_opaque))
					continue;
				*dest = ASTBlendPaletteIndexes(*dest, *source, originPatch->style, originPatch->alpha);
				*is_opaque = true;
			}
		}
	}
}












UINT8 *R_GenerateTexture(size_t texnum)
{
	UINT8 *block;
	UINT8 *blocktex;
	UINT8 *temp_block;
	texture_t *texture;
	texpatch_t *patch;
	int x, x1, x2, i, width, height;
	size_t blocksize;
	unsigned *column_posts;
	UINT8 *opaque_pixels;
	column_t *columns, *temp_columns;
	post_t *posts, *temp_posts = NULL;
	size_t total_posts = 0;
	size_t total_pixels = 0;

	I_Assert(texnum <= (size_t)numtextures);
	texture = textures[texnum];
	I_Assert(texture != NULL);
	texture->fullytransparent = false;


	if (texture->type == TEXTURETYPE_FLAT)
		goto multipatch;





	if (texture->patchcount == 1)
	{
		patch = &texture->patches[0];

		UINT16 wadnum = patch->wad;
		UINT16 lumpnum = patch->lump;
		size_t lumplength = W_LumpLengthPwad(wadnum, lumpnum);
		UINT8 *pdata = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
		softwarepatch_t *realpatch;

		if (pdata == NULL)
			goto multipatch;

#ifndef NO_PNG_LUMPS
		if (Picture_IsLumpPNG(pdata, lumplength))
			goto multipatch;
#endif

		realpatch = (softwarepatch_t *)pdata;
		if (!Picture_CheckIfDoomPatch(realpatch, lumplength))
		{
			CONS_Alert(CONS_WARNING,
				"Ignoring malformed texture patch %s from %s.\n",
				W_CheckNameForNumPwad(wadnum, lumpnum),
				wadfiles[wadnum]->filename);
			goto multipatch;
		}

		texture->transparency = false;


		if (texture->width > SHORT(realpatch->width) || texture->height > SHORT(realpatch->height))
			texture->transparency = true;
		else
		{
			UINT8 *colofs = (UINT8 *)realpatch->columnofs;
			for (x = 0; x < texture->width; x++)
			{
				doompost_t *col = (doompost_t *)((UINT8 *)realpatch + LONG(*(UINT32 *)&colofs[x<<2]));
				INT32 topdelta, prevdelta = -1, y = 0;
				while (col->topdelta != 0xff)
				{
					topdelta = col->topdelta;
					if (topdelta <= prevdelta)
						topdelta += prevdelta;
					prevdelta = topdelta;
					if (topdelta > y)
						break;
					y = topdelta + col->length + 1;
					col = (doompost_t *)((UINT8 *)col + col->length + 4);
				}
				if (y < texture->height)
					texture->transparency = true;  
			}
		}


		if (texture->transparency)
		{
			texture->flip = patch->flip;

			Patch_CalcDataSizes(realpatch, &total_pixels, &total_posts);
			texture->fullytransparent = (total_posts == 0);

			blocksize = (sizeof(column_t) * texture->width) + (sizeof(post_t) * total_posts) + (sizeof(UINT8) * total_pixels);
			texturememory += blocksize;

			block = Z_Calloc(blocksize, PU_STATIC, &texturecache[texnum]);
			blocktex = block;

			columns = (column_t *)(block + (sizeof(UINT8) * total_pixels));
			posts = (post_t *)(block + (sizeof(UINT8) * total_pixels) + (sizeof(column_t) * texture->width));

			texturecolumns[texnum] = columns;




			Patch_MakeColumns(realpatch, texture->width, texture->width, blocktex, columns, posts, patch->flip & 1);

			Z_Free(pdata);

			goto done;
		}


	}


	multipatch:
	texture->flip = 0;


	total_pixels = texture->width * texture->height;

	opaque_pixels = Z_Calloc(total_pixels * sizeof(UINT8), PU_STATIC, NULL);
	temp_columns = Z_Calloc(sizeof(column_t) * texture->width, PU_STATIC, NULL);
	temp_block = Z_Calloc(total_pixels, PU_STATIC, NULL);

#ifdef TEXTURE_255_IS_TRANSPARENT
	texture->transparency = false;


	memset(temp_block, TRANSPARENTPIXEL, total_pixels);
#else
	texture->transparency = true;
#endif

	for (x = 0; x < texture->width; x++)
	{
		column_t *column = &temp_columns[x];
		column->num_posts = 0;
		column->posts = NULL;
		column->pixels = temp_block + (texture->height * x);
	}


	for (i = 0, patch = texture->patches; i < texture->patchcount; i++, patch++)
	{
		static void (*columnDrawer)(column_t *, UINT8 *, texpatch_t *, INT32, INT32, UINT8 *);
		if (patch->style != AST_COPY)
			columnDrawer = (patch->flip & 2) ? R_DrawBlendFlippedColumnInCache : R_DrawBlendColumnInCache;
		else
			columnDrawer = (patch->flip & 2) ? R_DrawFlippedColumnInCache : R_DrawColumnInCache;

		UINT16 wadnum = patch->wad;
		lumpnum_t lumpnum = patch->lump;
		UINT8 *pdata = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
		patch_t *realpatch = NULL;
		boolean free_patch = true;

#ifndef NO_PNG_LUMPS
		size_t lumplength = W_LumpLengthPwad(wadnum, lumpnum);
		if (Picture_IsLumpPNG(pdata, lumplength))
			realpatch = (patch_t *)Picture_PNGConvert(pdata, PICFMT_PATCH, NULL, NULL, NULL, NULL, lumplength, NULL, 0);
		else
#endif
		if (texture->type == TEXTURETYPE_FLAT)
			realpatch = (patch_t *)Picture_Convert(PICFMT_FLAT, pdata, PICFMT_PATCH, 0, NULL, texture->width, texture->height, 0, 0, PICFLAGS_USE_TRANSPARENTPIXEL);
		else
		{

			realpatch = W_GetCachedPatchNumPwad(wadnum, lumpnum);
			free_patch = false;


			if (realpatch == NULL)
				realpatch = W_CachePatchNumPwad(wadnum, lumpnum, PU_PATCH);
		}

		if (realpatch == NULL)
			continue;

		x1 = patch->originx;
		width = realpatch->width;
		height = realpatch->height;
		x2 = x1 + width;

		if (x1 > texture->width || x2 < 0)
		{
			if (free_patch)
				Patch_Free(realpatch);
			continue;  
		}

		if (patch->originy > texture->height || (patch->originy + height) < 0)
		{
			if (free_patch)
				Patch_Free(realpatch);
			continue;  
		}





		if (x1 < 0)
			x = 0;
		else
			x = x1;


		if (x2 > texture->width)
			x2 = texture->width;

		for (; x < x2; x++)
		{
			INT32 colx;

			if (patch->flip & 1)
				colx = (x1+width-1)-x;
			else
				colx = x-x1;

			column_t *patchcol = &realpatch->columns[colx];

			if (patchcol->num_posts > 0)
				columnDrawer(patchcol, temp_columns[x].pixels, patch, texture->height, height, &opaque_pixels[x * texture->height]);
		}

		if (free_patch)
			Patch_Free(realpatch);
	}


	texture->fullytransparent = true;
	for (i = 0; i < (INT32)total_pixels; i++)
	{
		if (opaque_pixels[i])
		{
			texture->fullytransparent = false;
			break;
		}
	}
	column_posts = Z_Calloc(sizeof(unsigned) * texture->width, PU_STATIC, NULL);

#ifdef TEXTURE_255_IS_TRANSPARENT
	total_posts = texture->width;
	temp_posts = Z_Realloc(temp_posts, sizeof(post_t) * total_posts, PU_CACHE, NULL);
#endif

	for (x = 0; x < texture->width; x++)
	{
		post_t *post = NULL;

		column_t *column = &temp_columns[x];

#ifdef TEXTURE_255_IS_TRANSPARENT
		post = &temp_posts[x];
		post->topdelta = 0;
		post->length = texture->height;
		post->data_offset = 0;
		column_posts[x] = x;
		column->num_posts = 1;
#else
		boolean was_opaque = false;

		column_posts[x] = (unsigned)-1;

		for (INT32 y = 0; y < texture->height; y++)
		{

			if (!opaque_pixels[(x * texture->height) + y])
			{
				was_opaque = false;
				continue;
			}

			if (!was_opaque)
			{
				total_posts++;

				temp_posts = Z_Realloc(temp_posts, sizeof(post_t) * total_posts, PU_CACHE, NULL);
				post = &temp_posts[total_posts - 1];
				post->topdelta = (size_t)y;
				post->length = 0;
				post->data_offset = (size_t)y;
				if (column_posts[x] == (unsigned)-1)
					column_posts[x] = total_posts - 1;
				column->num_posts++;
			}

			was_opaque = true;

			post->length++;
		}
#endif
	}

	blocksize = (sizeof(column_t) * texture->width) + (sizeof(post_t) * total_posts) + (sizeof(UINT8) * total_pixels);
	texturememory += blocksize;

	block = Z_Calloc(blocksize, PU_STATIC, &texturecache[texnum]);
	blocktex = block;

	memcpy(blocktex, temp_block, total_pixels);

	Z_Free(temp_block);

	columns = (column_t *)(block + (sizeof(UINT8) * total_pixels));
	posts = (post_t *)(block + (sizeof(UINT8) * total_pixels) + (sizeof(column_t) * texture->width));

	memcpy(columns, temp_columns, sizeof(column_t) * texture->width);
	memcpy(posts, temp_posts, sizeof(post_t) * total_posts);

	texturecolumns[texnum] = columns;

	for (x = 0; x < texture->width; x++)
	{
		column_t *column = &columns[x];
		if (column->num_posts > 0)
			column->posts = &posts[column_posts[x]];
		column->pixels = blocktex + (texture->height * x);
	}

done:

	Z_ChangeTag(block, PU_CACHE);
	return blocktex;
}

UINT8 *R_GetFlatForTexture(size_t texnum)
{
	if (texnum >= (unsigned)numtextures)
		return NULL;

	texture_t *texture = textures[texnum];
	if (texture->flat != NULL)
		return texture->flat;


	if (texture->type == TEXTURETYPE_FLAT)
	{
		texpatch_t *patch = &texture->patches[0];
		UINT16 wadnum = patch->wad;
		lumpnum_t lumpnum = patch->lump;
		UINT8 *pdata = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
		size_t lumplength = W_LumpLengthPwad(wadnum, lumpnum);

#ifndef NO_PNG_LUMPS
		if (Picture_IsLumpPNG(pdata, lumplength))
		{
			texture->flat = Picture_PNGConvert(pdata, PICFMT_FLAT, NULL, NULL, NULL, NULL, lumplength, NULL, 0);
			if (!texture->flat)
				texture->flat = Z_Calloc((size_t)texture->width * (size_t)texture->height,
					PU_STATIC, NULL);
		}
		else
#endif
		{
			texture->flat = Z_Malloc(lumplength, PU_STATIC, NULL);
			memcpy(texture->flat, pdata, lumplength);
		}

		Z_SetUser(texture->flat, &texture->flat);

		Z_Free(pdata);
	}
	else
		texture->flat = (UINT8 *)Picture_TextureToFlat(texnum);

	flatmemory += texture->width * texture->height;

	return texture->flat;
}








INT32 R_GetTextureNum(INT32 texnum)
{
	if (texnum < 0 || texnum >= numtextures)
		return 0;
	return texturetranslation[texnum];
}







void R_CheckTextureCache(INT32 tex)
{
	if (!texturecache[tex])
		R_GenerateTexture(tex);
}

column_t *R_GetColumn(fixed_t tex, INT32 col)
{
	INT32 width = texturewidth[tex];
	if (width & (width - 1))
		col = (UINT32)col % width;
	else
		col &= (width - 1);

	return &texturecolumns[tex][col];
}

INT32 R_GetTextureNumForFlat(levelflat_t *levelflat)
{
	return texturetranslation[levelflat->texture_id];
}

void *R_GetFlat(levelflat_t *levelflat)
{
	if (levelflat->type == LEVELFLAT_NONE)
		return NULL;

	return R_GetFlatForTexture(R_GetTextureNumForFlat(levelflat));
}




boolean R_CheckPowersOfTwo(void)
{
	if (ds_flatwidth > 2048 || ds_flatheight > 2048)
		return false;

	boolean wpow2 = !(ds_flatwidth & (ds_flatwidth - 1));
	boolean hpow2 = !(ds_flatheight & (ds_flatheight - 1));

	return ds_flatwidth == ds_flatheight && wpow2 && hpow2;
}




boolean R_CheckSolidColorFlat(void)
{
	return ds_flatwidth == 1 && ds_flatheight == 1;
}




UINT16 R_GetFlatSize(size_t length)
{
	switch (length)
	{
		case 4194304:  
			return 2048;
		case 1048576:  
			return 1024;
		case 262144: 
			return 512;
		case 65536:  
			return 256;
		case 16384:  
			return 128;
		case 1024:  
			return 32;
		case 256:  
			return 16;
		case 64:  
			return 8;
		case 16:  
			return 4;
		case 4:  
			return 2;
		case 1:  
			return 1;
		default:  
			return 64;
	}
}




UINT8 R_GetFlatBits(INT32 size)
{
	switch (size)
	{
		case 2048: return 11;
		case 1024: return 10;
		case 512:  return 9;
		case 256:  return 8;
		case 128:  return 7;
		case 32:   return 5;
		case 16:   return 4;
		case 8:    return 3;
		case 4:    return 2;
		case 2:    return 1;
		case 1:    return 0;
		default:   return 6;  
	}
}

void R_SetFlatVars(size_t length)
{
	UINT16 size = R_GetFlatSize(length);
	UINT8 bits = R_GetFlatBits(size);

	ds_flatwidth = ds_flatheight = size;

	if (bits == 0)
		return;

	nflatshiftup = 16 - bits;
	nflatxshift = 16 + nflatshiftup;
	nflatyshift = nflatxshift - bits;
	nflatmask = (size - 1) * size;
}




void R_FlushTextureCache(void)
{
	INT32 i;

	if (numtextures)
		for (i = 0; i < numtextures; i++)
		{
			Z_Free(textures[i]->flat);
			Z_Free(texturecache[i]);
		}
}


int R_CountTexturesInTEXTURESLump(UINT16 wadNum, UINT16 lumpNum);
void R_ParseTEXTURESLump(UINT16 wadNum, UINT16 lumpNum, INT32 *index);

static void R_AddSinglePatchTexture(INT32 i, UINT16 wadnum, UINT16 lumpnum, INT16 width, INT16 height, unsigned type)
{
	texture_t *texture = Z_Calloc(sizeof(texture_t) + sizeof(texpatch_t), PU_STATIC, NULL);


	M_Memcpy(texture->name, W_CheckNameForNumPwad(wadnum, lumpnum), sizeof(texture->name));
	texture->hash = quickncasehash(texture->name, 8);

	texture->width = width;
	texture->height = height;

	texture->type = type;
	texture->patchcount = 1;
	texture->flip = 0;


	texpatch_t *patch = &texture->patches[0];

	patch->originx = patch->originy = 0;
	patch->wad = wadnum;
	patch->lump = lumpnum;
	patch->flip = 0;

	texturewidth[i] = texture->width;
	textureheight[i] = texture->height << FRACBITS;

	textures[i] = texture;
}

static INT32
Rloadflats (INT32 i, INT32 w)
{
	UINT16 j, numlumps = 0;
	UINT16 texstart = 0, texend = 0;
	UINT32 *list = NULL;


	if (W_FileHasFolders(wadfiles[w]))
	{
		W_GetFolderLumpsPwad("Flats/", (UINT16)w, &list, NULL, &numlumps);
	}
	else
	{

		texstart = W_CheckNumForMarkerStartPwad("F_START", (UINT16)w, 0);
		texend = W_CheckNumForNamePwad("F_END", (UINT16)w, texstart);
		if (texstart == INT16_MAX || texend == INT16_MAX)
			return i;

		numlumps = texend - texstart;
	}


	for (j = 0; j < numlumps; j++)
	{
		UINT16 wadnum = list ? WADFILENUM(list[j]) : (UINT16)w;
		UINT16 lumpnum = list ? LUMPNUM(list[j]) : (texstart + j);

		size_t lumplength = W_LumpLengthPwad(wadnum, lumpnum);
		size_t flatsize = R_GetFlatSize(lumplength);

		INT16 width = flatsize, height = flatsize;

#ifndef NO_PNG_LUMPS
		UINT8 header[PNG_DIMENSIONS_HEADER_SIZE];

		W_ReadLumpHeaderPwad(wadnum, lumpnum, header, sizeof header, 0);

		if (Picture_IsLumpPNG(header, lumplength))
		{
			INT32 texw, texh;
			if (Picture_PNGDimensionsFromHeader(header, lumplength, &texw, &texh))
			{
				width = (INT16)texw;
				height = (INT16)texh;
			}
			else
			{
				width = 1;
				height = 1;
			}
		}
#endif



		R_AddSinglePatchTexture(i, wadnum, lumpnum, width, height, TEXTURETYPE_FLAT);

		i++;
	}

	if (list)
		Z_Free(list);

	return i;
}

#define TX_START "TX_START"
#define TX_END "TX_END"

static INT32
Rloadtextures (INT32 i, INT32 w)
{
	UINT16 j, numlumps = 0;
	UINT16 texstart = 0, texend = 0;
	UINT16 texturesLumpPos;
	UINT32 *list = NULL;


	if (W_FileHasFolders(wadfiles[w]))
	{
		W_GetFolderLumpsPwad("Textures/", (UINT16)w, &list, NULL, &numlumps);

		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, 0);
		while (texturesLumpPos != INT16_MAX)
		{
			R_ParseTEXTURESLump(w, texturesLumpPos, &i);
			texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, texturesLumpPos + 1);
		}
	}

	else
	{
		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, 0);
		if (texturesLumpPos != INT16_MAX)
			R_ParseTEXTURESLump(w, texturesLumpPos, &i);

		texstart = W_CheckNumForMarkerStartPwad(TX_START, (UINT16)w, 0);
		texend = W_CheckNumForNamePwad(TX_END, (UINT16)w, texstart);
		if (texstart == INT16_MAX || texend == INT16_MAX)
			return i;

		numlumps = texend - texstart;
	}


	for (j = 0; j < numlumps; j++)
	{
		UINT16 wadnum = list ? WADFILENUM(list[j]) : (UINT16)w;
		UINT16 lumpnum = list ? LUMPNUM(list[j]) : (texstart + j);

		INT16 width = 0, height = 0;

		if (!W_ReadPatchHeaderPwad(wadnum, lumpnum, &width, &height, NULL, NULL))
		{
			width = 1;
			height = 1;
		}



		R_AddSinglePatchTexture(i, wadnum, lumpnum, width, height, TEXTURETYPE_SINGLEPATCH);

		i++;
	}

	if (list)
		Z_Free(list);

	return i;
}

static INT32
count_range
(		const char * marker_start,
		const char * marker_end,
		const char * folder,
		UINT16 wadnum)
{
	INT32 count = 0;

	if (W_FileHasFolders(wadfiles[wadnum]))
		count += W_CountFolderLumpsPwad(folder, wadnum);
	else
	{
		UINT16 texstart = W_CheckNumForMarkerStartPwad(marker_start, wadnum, 0);
		UINT16 texend = W_CheckNumForNamePwad(marker_end, wadnum, texstart);

		if (texstart != INT16_MAX && texend != INT16_MAX)
			count += (texend - texstart);
	}

	return count;
}

static INT32 R_CountTextures(UINT16 wadnum)
{
	UINT16 texturesLumpPos;
	INT32 count = 0;









	count += count_range("F_START", "F_END", "flats/", wadnum);


	texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", wadnum, 0);

	while (texturesLumpPos != INT16_MAX)
	{
		count += R_CountTexturesInTEXTURESLump(wadnum, texturesLumpPos);
		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", wadnum, texturesLumpPos + 1);
	}


	count += count_range(TX_START, TX_END, "textures/", wadnum);

	return count;
}

static void
recallocuser
(		void * user,
		size_t old,
		size_t new)
{
	char *p = Z_Realloc(*(void**)user,
			new, PU_STATIC, user);

	if (new > old)
		memset(&p[old], 0, (new - old));
}

static void R_AllocateTextures(INT32 add)
{
	const INT32 newtextures = (numtextures + add);
	const size_t newsize = newtextures * sizeof (void*);
	const size_t oldsize = numtextures * sizeof (void*);

	INT32 i;


	recallocuser(&textures, oldsize, newsize);


	recallocuser(&texturecolumns, oldsize, newsize);

	recallocuser(&texturecache, oldsize, newsize);

	recallocuser(&texturewidth, oldsize, newsize);

	recallocuser(&textureheight, oldsize, newsize);

	Z_Realloc(texturetranslation, (newtextures + 1) * sizeof(*texturetranslation), PU_STATIC, &texturetranslation);

	for (i = 0; i < numtextures; ++i)
	{



		Z_SetUser(texturecache[i],
				(void**)&texturecache[i]);
	}

	while (i < newtextures)
	{
		texturetranslation[i] = i;
		i++;
	}
}

static INT32 R_DefineTextures(INT32 i, UINT16 w)
{
	i = Rloadflats(i, w);
	return Rloadtextures(i, w);
}

static void R_FinishLoadingTextures(INT32 add)
{
	numtextures += add;

#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_LoadMapTextures(numtextures);
#endif
}





void R_LoadTextures(void)
{
	INT32 i, w;
	INT32 newtextures = 0;
	INT32 *wadtexturecounts = Z_Calloc(numwadfiles * sizeof(*wadtexturecounts), PU_STATIC, NULL);

	for (w = 0; w < numwadfiles; w++)
	{
		wadtexturecounts[w] = R_CountTextures((UINT16)w);
		newtextures += wadtexturecounts[w];
	}


	if (!newtextures)
		I_Error("No textures detected in any WADs!\n");

	R_AllocateTextures(newtextures);

	for (i = 0, w = 0; w < numwadfiles; w++)
	{
		if (wadtexturecounts[w])
		{
			W_BeginLumpHeaderBatch((UINT16)w);
			i = R_DefineTextures(i, w);
			W_EndLumpHeaderBatch();
		}
	}

	Z_Free(wadtexturecounts);

	R_FinishLoadingTextures(newtextures);
}

void R_LoadTexturesPwad(UINT16 wadnum)
{
	INT32 newtextures = R_CountTextures(wadnum);

	R_AllocateTextures(newtextures);
	if (newtextures)
	{
		W_BeginLumpHeaderBatch(wadnum);
		R_DefineTextures(numtextures, wadnum);
		W_EndLumpHeaderBatch();
	}
	R_FinishLoadingTextures(newtextures);
}

static lumpnum_t W_GetTexPatchLumpNum(const char *name)
{






	enum
	{
		USE_PATCHES,
		USE_TEXTURES,
		USE__MAX,
	};

	lumpnum_t lump = LUMPERROR;
	INT32 lump_type_it;


	for (lump_type_it = 0; lump_type_it < USE__MAX; lump_type_it++)
	{
		INT32 i;

		for (i = numwadfiles - 1; i >= 0; i--)  
		{
			lumpnum_t start = LUMPERROR;
			lumpnum_t end = LUMPERROR;

			switch (wadfiles[i]->type)
			{
				case RET_WAD:
					if (lump_type_it == USE_PATCHES)
					{
						if ((start = W_CheckNumForMarkerStartPwad("P_START", (UINT16)i, 0)) == INT16_MAX)
							continue;
						else if ((end = W_CheckNumForNamePwad("P_END", (UINT16)i, start)) == INT16_MAX)
							continue;
					}
					else if (lump_type_it == USE_TEXTURES)
					{
						if ((start = W_CheckNumForMarkerStartPwad("TX_START", (UINT16)i, 0)) == INT16_MAX)
							continue;
						else if ((end = W_CheckNumForNamePwad("TX_END", (UINT16)i, start)) == INT16_MAX)
							continue;
					}
					break;
				case RET_PK3:
				case RET_FOLDER:
					if (lump_type_it == USE_PATCHES)
					{
						if ((start = W_CheckNumForFolderStartPK3("Patches/", i, 0)) == INT16_MAX)
							continue;
						if ((end = W_CheckNumForFolderEndPK3("Patches/", i, start)) == INT16_MAX)
							continue;
					}
					else if (lump_type_it == USE_TEXTURES)
					{
						if ((start = W_CheckNumForFolderStartPK3("Textures/", i, 0)) == INT16_MAX)
							continue;
						if ((end = W_CheckNumForFolderEndPK3("Textures/", i, start)) == INT16_MAX)
							continue;
					}
					break;
				default:
					continue;
			}


			lump = W_CheckNumForNamePwad(name, (UINT16)i, start);
			if (lump < end)
			{
				lump += (i<<16);  
				break;
			}
			lump = LUMPERROR;
		}

		if (lump != LUMPERROR)
		{
			break;
		}
	}

	if (lump == LUMPERROR)
	{

		return W_CheckNumForPatchName(name);
	}

	return lump;
}

static texpatch_t *R_ParsePatch(boolean actuallyLoadPatch)
{
	char *texturesToken;
	size_t texturesTokenLength;
	char *endPos;
	char *patchName = NULL;
	INT16 patchXPos;
	INT16 patchYPos;
	UINT8 flip = 0;
	UINT8 alpha = 255;
	enum patchalphastyle style = AST_COPY;
	texpatch_t *resultPatch = NULL;
	lumpnum_t patchLumpNum;


	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch name should be");
	}
	texturesTokenLength = strlen(texturesToken);
	if (texturesTokenLength>8)
	{
		I_Error("Error parsing TEXTURES lump: Patch name \"%s\" exceeds 8 characters",texturesToken);
	}
	else
	{
		if (patchName != NULL)
		{
			Z_Free(patchName);
		}
		patchName = (char *)Z_Malloc((texturesTokenLength+1)*sizeof(char),PU_STATIC,NULL);
		M_Memcpy(patchName,texturesToken,texturesTokenLength*sizeof(char));
		patchName[texturesTokenLength] = '\0';
	}


	Z_Free(texturesToken);
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where comma after \"%s\"'s patch name should be",patchName);
	}
	if (strcmp(texturesToken,",")!=0)
	{
		I_Error("Error parsing TEXTURES lump: Expected \",\" after %s's patch name, got \"%s\"",patchName,texturesToken);
	}


	Z_Free(texturesToken);
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch \"%s\"'s x coordinate should be",patchName);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	patchXPos = strtol(texturesToken,&endPos,10);
	(void)patchXPos;  
	if (endPos == texturesToken  
		|| *endPos != '\0'  
#ifndef AVOID_ERRNO
		|| errno == ERANGE  
#endif
		)
	{
		I_Error("Error parsing TEXTURES lump: Expected an integer for patch \"%s\"'s x coordinate, got \"%s\"",patchName,texturesToken);
	}


	Z_Free(texturesToken);
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where comma after patch \"%s\"'s x coordinate should be",patchName);
	}
	if (strcmp(texturesToken,",")!=0)
	{
		I_Error("Error parsing TEXTURES lump: Expected \",\" after patch \"%s\"'s x coordinate, got \"%s\"",patchName,texturesToken);
	}


	Z_Free(texturesToken);
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch \"%s\"'s y coordinate should be",patchName);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	patchYPos = strtol(texturesToken,&endPos,10);
	(void)patchYPos;  
	if (endPos == texturesToken  
		|| *endPos != '\0'  
#ifndef AVOID_ERRNO
		|| errno == ERANGE  
#endif
		)
	{
		I_Error("Error parsing TEXTURES lump: Expected an integer for patch \"%s\"'s y coordinate, got \"%s\"",patchName,texturesToken);
	}
	Z_Free(texturesToken);





	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
		;  
	else
	{
		if (strcmp(texturesToken,"{")==0)
		{
			Z_Free(texturesToken);
			texturesToken = M_GetToken(NULL);
			if (texturesToken == NULL)
			{
				I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch \"%s\"'s parameters should be",patchName);
			}
			while (strcmp(texturesToken,"}")!=0)
			{
				if (stricmp(texturesToken, "ALPHA")==0)
				{
					Z_Free(texturesToken);
					texturesToken = M_GetToken(NULL);
					alpha = 255*strtof(texturesToken, NULL);
				}
				else if (stricmp(texturesToken, "STYLE")==0)
				{
					Z_Free(texturesToken);
					texturesToken = M_GetToken(NULL);
					if (stricmp(texturesToken, "TRANSLUCENT")==0)
						style = AST_TRANSLUCENT;
					else if (stricmp(texturesToken, "ADD")==0)
						style = AST_ADD;
					else if (stricmp(texturesToken, "SUBTRACT")==0)
						style = AST_SUBTRACT;
					else if (stricmp(texturesToken, "REVERSESUBTRACT")==0)
						style = AST_REVERSESUBTRACT;
					else if (stricmp(texturesToken, "MODULATE")==0)
						style = AST_MODULATE;
				}
				else if (stricmp(texturesToken, "FLIPX")==0)
					flip |= 1;
				else if (stricmp(texturesToken, "FLIPY")==0)
					flip |= 2;
				Z_Free(texturesToken);

				texturesToken = M_GetToken(NULL);
				if (texturesToken == NULL)
				{
					I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch \"%s\"'s parameters or right curly brace should be",patchName);
				}
			}
		}
		else
		{


			M_UnGetToken();
		}
		Z_Free(texturesToken);
	}

	if (actuallyLoadPatch == true)
	{

		patchLumpNum = W_GetTexPatchLumpNum(patchName);

		resultPatch = (texpatch_t *)Z_Malloc(sizeof(texpatch_t),PU_STATIC,NULL);
		resultPatch->originx = patchXPos;
		resultPatch->originy = patchYPos;
		resultPatch->lump = LUMPNUM(patchLumpNum);
		resultPatch->wad = WADFILENUM(patchLumpNum);
		resultPatch->flip = flip;
		resultPatch->alpha = alpha;
		resultPatch->style = style;

		Z_Free(patchName);

		return resultPatch;
	}
	else
	{
		Z_Free(patchName);
		return NULL;
	}
}

static texture_t *R_ParseTexture(boolean actuallyLoadTexture)
{
	char *texturesToken;
	size_t texturesTokenLength;
	char *endPos;
	INT32 newTextureWidth;
	INT32 newTextureHeight;
	texture_t *resultTexture = NULL;
	texpatch_t *newPatch;
	char newTextureName[9];  


	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where texture name should be");
	}
	texturesTokenLength = strlen(texturesToken);
	if (texturesTokenLength>8)
	{
		I_Error("Error parsing TEXTURES lump: Texture name \"%s\" exceeds 8 characters",texturesToken);
	}
	else
	{
		memset(&newTextureName, 0, 9);
		M_Memcpy(newTextureName, texturesToken, texturesTokenLength);

		strupr(newTextureName);  
	}
	Z_Free(texturesToken);


	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where comma after texture \"%s\"'s name should be",newTextureName);
	}
	else if (strcmp(texturesToken,",")!=0)
	{
		I_Error("Error parsing TEXTURES lump: Expected \",\" after texture \"%s\"'s name, got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);


	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where texture \"%s\"'s width should be",newTextureName);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	newTextureWidth = strtol(texturesToken,&endPos,10);
	if (endPos == texturesToken  
		|| *endPos != '\0'  
#ifndef AVOID_ERRNO
		|| errno == ERANGE  
#endif
		|| newTextureWidth < 0)  
	{
		I_Error("Error parsing TEXTURES lump: Expected a positive integer for texture \"%s\"'s width, got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);


	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where comma after texture \"%s\"'s width should be",newTextureName);
	}
	if (strcmp(texturesToken,",")!=0)
	{
		I_Error("Error parsing TEXTURES lump: Expected \",\" after texture \"%s\"'s width, got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);


	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where texture \"%s\"'s height should be",newTextureName);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	newTextureHeight = strtol(texturesToken,&endPos,10);
	if (endPos == texturesToken  
		|| *endPos != '\0'  
#ifndef AVOID_ERRNO
		|| errno == ERANGE  
#endif
		|| newTextureHeight < 0)  
	{
		I_Error("Error parsing TEXTURES lump: Expected a positive integer for texture \"%s\"'s height, got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);


	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where open curly brace for texture \"%s\" should be",newTextureName);
	}
	if (strcmp(texturesToken,"{")==0)
	{
		if (actuallyLoadTexture)
		{

			resultTexture = (texture_t *)Z_Calloc(sizeof(texture_t),PU_STATIC,NULL);
			M_Memcpy(resultTexture->name, newTextureName, 8);
			resultTexture->hash = quickncasehash(newTextureName, 8);
			resultTexture->width = newTextureWidth;
			resultTexture->height = newTextureHeight;
			resultTexture->type = TEXTURETYPE_COMPOSITE;
		}
		Z_Free(texturesToken);
		texturesToken = M_GetToken(NULL);
		if (texturesToken == NULL)
		{
			I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch definition for texture \"%s\" should be",newTextureName);
		}
		while (strcmp(texturesToken,"}")!=0)
		{
			if (stricmp(texturesToken, "PATCH")==0)
			{
				Z_Free(texturesToken);
				if (resultTexture)
				{

					newPatch = R_ParsePatch(true);

					resultTexture = Z_Realloc(resultTexture, sizeof(texture_t) + (resultTexture->patchcount+1)*sizeof(texpatch_t), PU_STATIC, NULL);

					M_Memcpy(&resultTexture->patches[resultTexture->patchcount], newPatch, sizeof(texpatch_t));

					resultTexture->patchcount++;

					Z_Free(newPatch);
				}
				else
				{
					R_ParsePatch(false);
				}
			}
			else
			{
				I_Error("Error parsing TEXTURES lump: Expected \"PATCH\" in texture \"%s\", got \"%s\"",newTextureName,texturesToken);
			}

			texturesToken = M_GetToken(NULL);
			if (texturesToken == NULL)
			{
				I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch declaration or right curly brace for texture \"%s\" should be",newTextureName);
			}
		}
		if (resultTexture && resultTexture->patchcount == 0)
		{
			I_Error("Error parsing TEXTURES lump: Texture \"%s\" must have at least one patch",newTextureName);
		}
	}
	else
	{
		I_Error("Error parsing TEXTURES lump: Expected \"{\" for texture \"%s\", got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);

	if (actuallyLoadTexture) return resultTexture;
	else return NULL;
}


int R_CountTexturesInTEXTURESLump(UINT16 wadNum, UINT16 lumpNum)
{
	char *texturesLump;
	size_t texturesLumpLength;
	char *texturesText;
	UINT32 numTexturesInLump = 0;
	char *texturesToken;




	texturesLump = (char *)W_CacheLumpNumPwad(wadNum, lumpNum, PU_STATIC);

	if (texturesLump == NULL) return 0;

	texturesLumpLength = W_LumpLengthPwad(wadNum, lumpNum);
	texturesText = (char *)Z_Malloc((texturesLumpLength+1)*sizeof(char),PU_STATIC,NULL);

	memmove(texturesText,texturesLump,texturesLumpLength);

	texturesText[texturesLumpLength] = '\0';


	Z_Free(texturesLump);

	texturesToken = M_GetToken(texturesText);
	while (texturesToken != NULL)
	{
		if (stricmp(texturesToken, "WALLTEXTURE") == 0 || stricmp(texturesToken, "TEXTURE") == 0)
		{
			numTexturesInLump++;
			Z_Free(texturesToken);
			R_ParseTexture(false);
		}
		else
		{
			I_Error("Error parsing TEXTURES lump: Expected \"WALLTEXTURE\" or \"TEXTURE\", got \"%s\"",texturesToken);
		}
		texturesToken = M_GetToken(NULL);
	}
	Z_Free(texturesToken);
	Z_Free((void *)texturesText);

	return numTexturesInLump;
}


void R_ParseTEXTURESLump(UINT16 wadNum, UINT16 lumpNum, INT32 *texindex)
{
	char *texturesLump;
	size_t texturesLumpLength;
	char *texturesText;
	char *texturesToken;
	texture_t *newTexture;

	I_Assert(texindex != NULL);




	texturesLump = (char *)W_CacheLumpNumPwad(wadNum, lumpNum, PU_STATIC);

	if (texturesLump == NULL) return;

	texturesLumpLength = W_LumpLengthPwad(wadNum, lumpNum);
	texturesText = (char *)Z_Malloc((texturesLumpLength+1)*sizeof(char),PU_STATIC,NULL);

	memmove(texturesText,texturesLump,texturesLumpLength);

	texturesText[texturesLumpLength] = '\0';


	Z_Free(texturesLump);

	texturesToken = M_GetToken(texturesText);
	while (texturesToken != NULL)
	{
		if (stricmp(texturesToken, "WALLTEXTURE") == 0 || stricmp(texturesToken, "TEXTURE") == 0)
		{
			Z_Free(texturesToken);

			newTexture = R_ParseTexture(true);

			textures[*texindex] = newTexture;
			texturewidth[*texindex] = newTexture->width;
			textureheight[*texindex] = newTexture->height << FRACBITS;

			(*texindex)++;
		}
		else
		{
			I_Error("Error parsing TEXTURES lump: Expected \"WALLTEXTURE\" or \"TEXTURE\", got \"%s\"",texturesToken);
		}
		texturesToken = M_GetToken(NULL);
	}
	Z_Free(texturesToken);
	Z_Free((void *)texturesText);
}

void R_ClearTextureNumCache(boolean btell)
{
	if (tidcache)
		Z_Free(tidcache);
	tidcache = NULL;
	if (btell)
		CONS_Debug(DBG_SETUP, "Fun Fact: There are %d textures used in this map.\n", tidcachelen);
	tidcachelen = 0;
}

static void AddTextureToCache(const char *name, UINT32 hash, INT32 id, UINT8 type)
{
	tidcachelen++;
	Z_Realloc(tidcache, tidcachelen * sizeof(*tidcache), PU_STATIC, &tidcache);
	strncpy(tidcache[tidcachelen-1].name, name, 8);
	tidcache[tidcachelen-1].name[8] = '\0';
#ifndef ZDEBUG
	CONS_Debug(DBG_SETUP, "texture #%s: %s\n", sizeu1(tidcachelen), tidcache[tidcachelen-1].name);
#endif
	tidcache[tidcachelen-1].hash = hash;
	tidcache[tidcachelen-1].id = id;
	tidcache[tidcachelen-1].type = type;
}






INT32 R_CheckTextureNumForName(const char *name)
{
	INT32 i;
	UINT32 hash;


	if (name[0] == '-')
		return 0;

	hash = quickncasehash(name, 8);

	for (i = 0; i < tidcachelen; i++)
		if (tidcache[i].hash == hash && !strncasecmp(tidcache[i].name, name, 8))
			return tidcache[i].id;


	for (i = numtextures - 1; i >= 0; i--)
		if (textures[i]->hash == hash && !strncasecmp(textures[i]->name, name, 8))
		{
			AddTextureToCache(name, hash, i, textures[i]->type);
			return i;
		}

	return -1;
}







const char *R_CheckTextureNameForNum(INT32 num)
{
	if (num > 0 && num < numtextures)
		return textures[num]->name;

	return "-";
}






const char *R_TextureNameForNum(INT32 num)
{
	const char *result = R_CheckTextureNameForNum(num);

	if (strcmp(result, "-") == 0)
		return "REDWALL";

	return result;
}






INT32 R_TextureNumForName(const char *name)
{
	const INT32 i = R_CheckTextureNumForName(name);

	if (i == -1)
	{
		static INT32 redwall = -2;
		CONS_Debug(DBG_SETUP, "WARNING: R_TextureNumForName: %.8s not found\n", name);
		if (redwall == -2)
			redwall = R_CheckTextureNumForName("REDWALL");
		if (redwall != -1)
			return redwall;
		return 1;
	}
	return i;
}


INT32 R_CheckFlatNumForName(const char *name)
{
	INT32 i;
	UINT32 hash;


	if (name[0] == '-')
		return 0;

	hash = quickncasehash(name, 8);

	for (i = 0; i < tidcachelen; i++)
		if (tidcache[i].type == TEXTURETYPE_FLAT && tidcache[i].hash == hash && !strncasecmp(tidcache[i].name, name, 8))
			return tidcache[i].id;

	for (i = numtextures - 1; i >= 0; i--)
		if (textures[i]->hash == hash && !strncasecmp(textures[i]->name, name, 8) && textures[i]->type == TEXTURETYPE_FLAT)
		{
			AddTextureToCache(name, hash, i, TEXTURETYPE_FLAT);
			return i;
		}

	return -1;
}
