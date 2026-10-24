// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "../doomdef.h"

#ifdef HWRENDER
#include "hw_glob.h"
#include "hw_drv.h"
#include "hw_batching.h"
#include "hw_main.h"

#include "../doomstat.h"     
#include "../i_system.h"
#include "../i_time.h"
#include "../i_video.h"      
#include "../r_data.h"
#include "../r_textures.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../v_video.h"
#include "../r_draw.h"
#include "../r_patch.h"
#include "../r_picformats.h"
#include "../p_setup.h"

INT32 patchformat = GL_TEXFMT_AP_88;  
INT32 textureformat = GL_TEXFMT_P_8;  

RGBA_t mapPalette[256] = {0};  


RGBA_t *HWR_GetTexturePalette(void)
{
	return HWR_ShouldUsePaletteRendering() ? mapPalette : pLocalPalette;
}

static INT32 format2bpp(GLTextureFormat_t format)
{
	if (format == GL_TEXFMT_RGBA)
		return 4;
	else if (format == GL_TEXFMT_ALPHA_INTENSITY_88 || format == GL_TEXFMT_AP_88)
		return 2;
	else
		return 1;
}




static void HWR_DrawColumnInCache(const column_t *patchcol, UINT8 *block, GLMipmap_t *mipmap,
								INT32 pblockheight, INT32 blockmodulo,
								fixed_t yfracstep, fixed_t scale_y,
								texpatch_t *originPatch, INT32 patchheight,
								INT32 bpp, RGBA_t *palette)
{
	fixed_t yfrac, position, count;
	UINT8 *dest;
	const UINT8 *source;
	INT32 originy = 0;


	RGBA_t colortemp;
	UINT8 alpha;
	UINT8 texel;
	UINT16 texelu16;

	(void)patchheight;  

	if (originPatch)  
		originy = originPatch->originy;

	for (unsigned i = 0; i < patchcol->num_posts; i++)
	{
		post_t *post = &patchcol->posts[i];
		source = patchcol->pixels + post->data_offset;
		count  = ((post->length * scale_y) + (FRACUNIT/2)) >> FRACBITS;
		position = originy + post->topdelta;

		yfrac = 0;
		if (position < 0)
		{
			yfrac = -position<<FRACBITS;
			count += (((position * scale_y) + (FRACUNIT/2)) >> FRACBITS);
			position = 0;
		}

		position = ((position * scale_y) + (FRACUNIT/2)) >> FRACBITS;

		if (position < 0)
			position = 0;

		if (position + count >= pblockheight)
			count = pblockheight - position;

		for (dest = block + (position*blockmodulo); count > 0; count--, dest += blockmodulo, yfrac += yfracstep)
		{
			texel = source[yfrac>>FRACBITS];
			alpha = 0xFF;


			if ((mipmap->flags & TF_CHROMAKEYED) && (texel == HWR_PATCHES_CHROMAKEY_COLORINDEX))
				alpha = 0x00;

			if (mipmap->colormap)
				texel = mipmap->colormap->data[texel];

			switch (bpp)
			{
				case 2:
				{
					texelu16 = *((UINT16*)dest);
					if ((originPatch != NULL) && (originPatch->style != AST_COPY))
					{
						if (originPatch->style == AST_TRANSLUCENT && originPatch->alpha < ASTTextureBlendingThreshold[0])
							continue;
						if (!(texelu16 & 0xFF00) && originPatch->alpha <= ASTTextureBlendingThreshold[1])
							continue;
						texel = ASTBlendPaletteIndexes(texelu16 & 0xFF, texel, originPatch->style, originPatch->alpha);
					}
					texelu16 = (UINT16)((alpha<<8) | texel);
					memcpy(dest, &texelu16, sizeof(UINT16));
					break;
				}
				case 3:
					colortemp = palette[texel];
					if ((originPatch != NULL) && (originPatch->style != AST_COPY))
					{
						RGBA_t rgbatexel;
						rgbatexel.rgba = *(UINT32 *)dest;
						colortemp.rgba = ASTBlendTexturePixel(rgbatexel, colortemp, originPatch->style, originPatch->alpha);
					}
					memcpy(dest, &colortemp, sizeof(RGBA_t)-sizeof(UINT8));
					break;
				case 4:
					colortemp = palette[texel];
					colortemp.s.alpha = alpha;
					if ((originPatch != NULL) && (originPatch->style != AST_COPY))
					{
						RGBA_t rgbatexel;
						rgbatexel.rgba = *(UINT32 *)dest;
						colortemp.rgba = ASTBlendTexturePixel(rgbatexel, colortemp, originPatch->style, originPatch->alpha);
					}
					memcpy(dest, &colortemp, sizeof(RGBA_t));
					break;

				default:
					if ((originPatch != NULL) && (originPatch->style != AST_COPY))
						*dest = ASTBlendPaletteIndexes(*dest, texel, originPatch->style, originPatch->alpha);
					else
						*dest = texel;
					break;
			}
		}
	}
}

static void HWR_DrawFlippedColumnInCache(const column_t *patchcol, UINT8 *block, GLMipmap_t *mipmap,
								INT32 pblockheight, INT32 blockmodulo,
								fixed_t yfracstep, fixed_t scale_y,
								texpatch_t *originPatch, INT32 patchheight,
								INT32 bpp, RGBA_t *palette)
{
	fixed_t yfrac, position, count;
	UINT8 *dest;
	const UINT8 *source;
	INT32 topdelta;
	INT32 originy = 0;


	RGBA_t colortemp;
	UINT8 alpha;
	UINT8 texel;
	UINT16 texelu16;

	if (originPatch)  
		originy = originPatch->originy;

	for (unsigned i = 0; i < patchcol->num_posts; i++)
	{
		post_t *post = &patchcol->posts[i];
		source = patchcol->pixels + post->data_offset;
		topdelta = patchheight-post->length-post->topdelta;
		count  = ((post->length * scale_y) + (FRACUNIT/2)) >> FRACBITS;
		position = originy + topdelta;

		yfrac = (post->length-1) << FRACBITS;

		if (position < 0)
		{
			yfrac += position<<FRACBITS;
			count += (((position * scale_y) + (FRACUNIT/2)) >> FRACBITS);
			position = 0;
		}

		position = ((position * scale_y) + (FRACUNIT/2)) >> FRACBITS;

		if (position < 0)
			position = 0;

		if (position + count >= pblockheight)
			count = pblockheight - position;

		for (dest = block + (position*blockmodulo); count > 0; count--, dest += blockmodulo, yfrac -= yfracstep)
		{
			texel = source[yfrac>>FRACBITS];
			alpha = 0xFF;


			if ((mipmap->flags & TF_CHROMAKEYED) && (texel == HWR_PATCHES_CHROMAKEY_COLORINDEX))
				alpha = 0x00;

			if (mipmap->colormap)
				texel = mipmap->colormap->data[texel];

			switch (bpp)
			{
				case 2:
					texelu16 = *((UINT16*)dest);
					if ((originPatch != NULL) && (originPatch->style != AST_COPY))
					{
						if (originPatch->style == AST_TRANSLUCENT && originPatch->alpha < ASTTextureBlendingThreshold[0])
							continue;
						if (!(texelu16 & 0xFF00) && originPatch->alpha <= ASTTextureBlendingThreshold[1])
							continue;
						texel = ASTBlendPaletteIndexes(texelu16 & 0xFF, texel, originPatch->style, originPatch->alpha);
					}
					texelu16 = (UINT16)((alpha<<8) | texel);
					memcpy(dest, &texelu16, sizeof(UINT16));
					break;
				case 3:
					colortemp = palette[texel];
					if ((originPatch != NULL) && (originPatch->style != AST_COPY))
					{
						RGBA_t rgbatexel;
						rgbatexel.rgba = *(UINT32 *)dest;
						colortemp.rgba = ASTBlendTexturePixel(rgbatexel, colortemp, originPatch->style, originPatch->alpha);
					}
					memcpy(dest, &colortemp, sizeof(RGBA_t)-sizeof(UINT8));
					break;
				case 4:
					colortemp = palette[texel];
					colortemp.s.alpha = alpha;
					if ((originPatch != NULL) && (originPatch->style != AST_COPY))
					{
						RGBA_t rgbatexel;
						rgbatexel.rgba = *(UINT32 *)dest;
						colortemp.rgba = ASTBlendTexturePixel(rgbatexel, colortemp, originPatch->style, originPatch->alpha);
					}
					memcpy(dest, &colortemp, sizeof(RGBA_t));
					break;

				default:
					if ((originPatch != NULL) && (originPatch->style != AST_COPY))
						*dest = ASTBlendPaletteIndexes(*dest, texel, originPatch->style, originPatch->alpha);
					else
						*dest = texel;
					break;
			}
		}
	}
}






static void HWR_DrawPatchInCache(GLMipmap_t *mipmap,
	INT32 pblockwidth, INT32 pblockheight,
	INT32 pwidth, INT32 pheight,
	const patch_t *realpatch)
{
	INT32 ncols;
	fixed_t xfrac, xfracstep;
	fixed_t yfracstep, scale_y;
	const column_t *patchcol;
	UINT8 *block = mipmap->data;
	INT32 bpp;
	INT32 blockmodulo;
	RGBA_t *palette;

	if (pwidth <= 0 || pheight <= 0)
		return;

	palette = HWR_GetTexturePalette();

	ncols = pwidth;


	xfrac = 0;
	xfracstep = FRACUNIT;
	yfracstep = FRACUNIT;
	scale_y   = FRACUNIT;

	bpp = format2bpp(mipmap->format);

	if (bpp < 1 || bpp > 4)
		I_Error("HWR_DrawPatchInCache: no drawer defined for this bpp (%d)\n",bpp);


	blockmodulo = pblockwidth*bpp;


	for (; ncols--; block += bpp, xfrac += xfracstep)
	{
		patchcol = &realpatch->columns[xfrac>>FRACBITS];

		HWR_DrawColumnInCache(patchcol, block, mipmap,
								pblockheight, blockmodulo,
								yfracstep, scale_y,
								NULL, pheight,  
								bpp, palette);
	}
}

static void HWR_BuildTrueColorPalettePixels(patch_t *patch)
{
	size_t x;
	size_t pixelcount;
	UINT8 *pixels;

	if (patch->hardware_palette_pixels || patch->width <= 0 || patch->height <= 0)
		return;

	pixelcount = (size_t)patch->width * (size_t)patch->height;
	pixels = Z_Malloc(pixelcount, PU_PATCH_DATA,
		(void **)&patch->hardware_palette_pixels);
	memset(pixels, TRANSPARENTPIXEL, pixelcount);

	                                                                           
                                                                             
                                                                         
	for (x = 0; x < (size_t)patch->width; x++)
	{
		const column_t *column = &patch->columns[x];
		unsigned int postindex;

		for (postindex = 0; postindex < column->num_posts; postindex++)
		{
			const post_t *post = &column->posts[postindex];
			size_t offset;

			for (offset = 0; offset < post->length; offset++)
			{
				const size_t y = post->topdelta + offset;
				if (y >= (size_t)patch->height)
					break;
				pixels[y * (size_t)patch->width + x]
					= column->pixels[post->data_offset + offset];
			}
		}
	}
}

static RGBA_t HWR_GetTrueColorPixel(patch_t *patch, GLMipmap_t *mipmap,
	INT32 x, INT32 y)
{
	const size_t offset = (size_t)y * (size_t)patch->width + (size_t)x;
	RGBA_t color = patch->hardware_pixels[offset];

	                                                                          
                                                                   
	if (color.s.alpha && mipmap->colormap)
	{
		if (!patch->hardware_palette_pixels)
			HWR_BuildTrueColorPalettePixels(patch);

		const UINT8 source = patch->hardware_palette_pixels[offset];
		const UINT8 translated = mipmap->colormap->data[source];
		if (translated != source)
		{
			const UINT8 alpha = color.s.alpha;
			color = HWR_GetTexturePalette()[translated];
			color.s.alpha = alpha;
		}
	}
	return color;
}

static void HWR_BlendTrueColorPixel(RGBA_t *destination, RGBA_t source,
	const texpatch_t *originPatch)
{
	UINT32 inverse, outputalpha;

	if (!source.s.alpha)
		return;
	if (originPatch && originPatch->style != AST_COPY)
	{
		UINT8 alpha = (UINT8)(((UINT32)originPatch->alpha
			* (UINT32)source.s.alpha + 127) / 255);
		source.s.alpha = 255;
		destination->rgba = ASTBlendTexturePixel(*destination, source,
			originPatch->style, alpha);
		return;
	}
	if (source.s.alpha == 255 || !destination->s.alpha)
	{
		*destination = source;
		return;
	}

	 
	inverse = 255 - source.s.alpha;
	outputalpha = source.s.alpha
		+ ((UINT32)destination->s.alpha * inverse + 127) / 255;
	if (!outputalpha)
	{
		destination->rgba = 0;
		return;
	}
	destination->s.red = (UINT8)(((UINT32)source.s.red * source.s.alpha
		+ ((UINT32)destination->s.red * destination->s.alpha * inverse + 127) / 255)
		/ outputalpha);
	destination->s.green = (UINT8)(((UINT32)source.s.green * source.s.alpha
		+ ((UINT32)destination->s.green * destination->s.alpha * inverse + 127) / 255)
		/ outputalpha);
	destination->s.blue = (UINT8)(((UINT32)source.s.blue * source.s.alpha
		+ ((UINT32)destination->s.blue * destination->s.alpha * inverse + 127) / 255)
		/ outputalpha);
	destination->s.alpha = (UINT8)outputalpha;
}

static void HWR_DrawTrueColorPatchInCache(GLMipmap_t *mipmap,
	INT32 pblockwidth, INT32 pblockheight, patch_t *patch)
{
	RGBA_t *block = (RGBA_t *)mipmap->data;
	INT32 x, y;

	for (y = 0; y < patch->height && y < pblockheight; y++)
		for (x = 0; x < patch->width && x < pblockwidth; x++)
			HWR_BlendTrueColorPixel(&block[(size_t)y * pblockwidth + x],
				HWR_GetTrueColorPixel(patch, mipmap, x, y), NULL);
}

static void HWR_DrawTrueColorTexturePatchInCache(GLMipmap_t *mipmap,
	INT32 pblockwidth, INT32 pblockheight, texture_t *texture,
	texpatch_t *originPatch, patch_t *patch)
{
	RGBA_t *block = (RGBA_t *)mipmap->data;
	INT32 x, y;

	for (y = 0; y < pblockheight; y++)
	{
		INT32 texturey = (INT32)(((INT64)y * texture->height) / pblockheight);
		INT32 sourcey = texturey - originPatch->originy;
		if (sourcey < 0 || sourcey >= patch->height)
			continue;
		if (originPatch->flip & 2)
			sourcey = patch->height - 1 - sourcey;

		for (x = 0; x < pblockwidth; x++)
		{
			INT32 texturex = (INT32)(((INT64)x * texture->width) / pblockwidth);
			INT32 sourcex = texturex - originPatch->originx;
			if (sourcex < 0 || sourcex >= patch->width)
				continue;
			if (originPatch->flip & 1)
				sourcex = patch->width - 1 - sourcex;

			HWR_BlendTrueColorPixel(&block[(size_t)y * pblockwidth + x],
				HWR_GetTrueColorPixel(patch, mipmap, sourcex, sourcey), originPatch);
		}
	}
}


static void HWR_DrawTexturePatchInCache(GLMipmap_t *mipmap,
	INT32 pblockwidth, INT32 pblockheight,
	texture_t *texture, texpatch_t *patch,
	const patch_t *realpatch)
{
	INT32 x, x1, x2;
	INT32 col, ncols;
	fixed_t xfrac, xfracstep;
	fixed_t yfracstep, scale_y;
	const column_t *patchcol;
	UINT8 *block = mipmap->data;
	INT32 bpp;
	INT32 blockmodulo;
	INT32 width, height;
	RGBA_t *palette;

	static void (*ColumnDrawerPointer)(const column_t *patchcol, UINT8 *block, GLMipmap_t *mipmap,
								INT32 pblockheight, INT32 blockmodulo,
								fixed_t yfracstep, fixed_t scale_y,
								texpatch_t *originPatch, INT32 patchheight,
								INT32 bpp, RGBA_t *palette);

	if (texture->width <= 0 || texture->height <= 0)
		return;

	palette = HWR_GetTexturePalette();

	ColumnDrawerPointer = (patch->flip & 2) ? HWR_DrawFlippedColumnInCache : HWR_DrawColumnInCache;

	x1 = patch->originx;
	width = realpatch->width;
	height = realpatch->height;
	x2 = x1 + width;

	if (x1 > texture->width || x2 < 0)
		return;  

	if (patch->originy > texture->height || (patch->originy + height) < 0)
		return;  





	if (x1 < 0)
		x = 0;
	else
		x = x1;


	if (x2 > texture->width)
		x2 = texture->width;


	col = x * pblockwidth / texture->width;
	ncols = ((x2 - x) * pblockwidth) / texture->width;


	xfrac = 0;
	if (x1 < 0)
		xfrac = -x1<<FRACBITS;

	xfracstep = (texture->width << FRACBITS) / pblockwidth;
	yfracstep = (texture->height<< FRACBITS) / pblockheight;
	scale_y   = (pblockheight  << FRACBITS) / texture->height;

	bpp = format2bpp(mipmap->format);

	if (bpp < 1 || bpp > 4)
		I_Error("HWR_DrawTexturePatchInCache: no drawer defined for this bpp (%d)\n",bpp);


	blockmodulo = pblockwidth*bpp;


	for (block += col*bpp; ncols--; block += bpp, xfrac += xfracstep)
	{
		if (patch->flip & 1)
			patchcol = &realpatch->columns[(width-1)-(xfrac>>FRACBITS)];
		else
			patchcol = &realpatch->columns[xfrac>>FRACBITS];

		ColumnDrawerPointer(patchcol, block, mipmap,
								pblockheight, blockmodulo,
								yfracstep, scale_y,
								patch, height,
								bpp, palette);
	}
}

static UINT8 *MakeBlock(GLMipmap_t *grMipmap)
{
	UINT8 *block;
	INT32 bpp, i;
	UINT16 bu16 = ((0x00 <<8) | HWR_PATCHES_CHROMAKEY_COLORINDEX);
	INT32 blocksize = (grMipmap->width * grMipmap->height);

	bpp =  format2bpp(grMipmap->format);
	block = Z_Malloc(blocksize*bpp, PU_HWRCACHE, &(grMipmap->data));

	switch (bpp)
	{
		case 1: memset(block, HWR_PATCHES_CHROMAKEY_COLORINDEX, blocksize); break;
		case 2:

				for (i = 0; i < blocksize; i++)

					memcpy(block+i*sizeof(UINT16), &bu16, sizeof(UINT16));
				break;
		case 4: memset(block, 0x00, blocksize*sizeof(UINT32)); break;
	}

	return block;
}

static boolean HWR_TextureHasTrueColorPNG(texture_t *texture)
{
	INT32 i;

	if (!cv_gltruecolorpngs.value)
		return false;
	for (i = 0; i < texture->patchcount; i++)
	{
		texpatch_t *source = &texture->patches[i];
		UINT8 *data = W_CacheLumpNumPwad(source->wad, source->lump, PU_CACHE);
#ifndef NO_PNG_LUMPS
		if (Picture_IsLumpPNG(data, W_LumpLengthPwad(source->wad, source->lump)))
		{
			patch_t *patch = W_CachePatchNumPwad(source->wad, source->lump, PU_PATCH);
			if (patch && patch->hardware_pixels)
				return true;
		}
#else
		(void)data;
#endif
	}
	return false;
}

                                                                            
                                                                       
                                                                               
                                                                           
                                                                               
                                                                       
static void HWR_RepairTinyTrailingPNGEdge(RGBA_t *pixels, INT32 width,
	INT32 height)
{
	size_t i, transparent = 0;
	boolean candidate = true;
	const size_t count = (size_t)width * (size_t)height;

	if (!pixels || width < 2 || height < 4)
		return;

	for (i = 0; i < count; i++)
	{
		const UINT8 alpha = pixels[i].s.alpha;
		if (alpha == 0)
		{
			const size_t x = i % (size_t)width;
			const size_t y = i / (size_t)width;
			transparent++;
			if (transparent > 4 || x != (size_t)width - 1
			|| y < (size_t)height - 4)
				candidate = false;
		}
		else if (alpha != 255)
			candidate = false;
	}

	if (!candidate || !transparent)
		return;

	for (i = count - (size_t)width * 4; i < count; i++)
		if (pixels[i].s.alpha == 0)
			pixels[i] = pixels[i - 1];
}





static void HWR_GenerateTexture(INT32 texnum, GLMapTexture_t *grtex, GLMipmap_t *mipmap)
{
	UINT8 *block;
	texture_t *texture;
	texpatch_t *patch;
	INT32 blockwidth, blockheight, blocksize;
	boolean copied_simple_png = false;

	INT32 i;

	texture = textures[texnum];

	blockwidth = texture->width;
	blockheight = texture->height;
	blocksize = blockwidth * blockheight;
	if (HWR_TextureHasTrueColorPNG(texture))
		mipmap->format = GL_TEXFMT_RGBA;
	block = MakeBlock(mipmap);

	                                                                           
                                                                          
                                                                           
                                                                            
                                       
	if (mipmap->format == GL_TEXFMT_RGBA && texture->patchcount == 1
	&& !texture->flip)
	{
		patch_t *realpatch;
		patch = &texture->patches[0];
		realpatch = W_GetCachedPatchNumPwad(patch->wad, patch->lump);
		if (realpatch && realpatch->hardware_pixels
		&& patch->originx == 0 && patch->originy == 0 && !patch->flip
		&& patch->style == AST_COPY
		&& realpatch->width == blockwidth && realpatch->height == blockheight)
		{
			memcpy(block, realpatch->hardware_pixels,
				(size_t)blocksize * sizeof (RGBA_t));
			copied_simple_png = true;
			mipmap->flags |= TF_PALETTE_INDEPENDENT;
		}
	}


	for (i = 0, patch = texture->patches;
		!copied_simple_png && i < texture->patchcount; i++, patch++)
	{
		UINT16 wadnum = patch->wad;
		lumpnum_t lumpnum = patch->lump;
		UINT8 *pdata = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
		patch_t *realpatch = NULL;
		boolean free_patch = true;

#ifndef NO_PNG_LUMPS
		size_t lumplength = W_LumpLengthPwad(wadnum, lumpnum);
		if (Picture_IsLumpPNG(pdata, lumplength))
		{
			realpatch = W_CachePatchNumPwad(wadnum, lumpnum, PU_PATCH);
			free_patch = false;
		}
		else
#endif
		if (texture->type == TEXTURETYPE_FLAT)
			realpatch = (patch_t *)Picture_Convert(PICFMT_FLAT, pdata, PICFMT_PATCH, 0, NULL, texture->width, texture->height, 0, 0, 0);
		else
		{

			realpatch = W_GetCachedPatchNumPwad(wadnum, lumpnum);
			free_patch = false;


			if (realpatch == NULL)
				realpatch = W_CachePatchNumPwad(wadnum, lumpnum, PU_PATCH);
		}

		if (realpatch != NULL)
		{
			if (mipmap->format == GL_TEXFMT_RGBA && cv_gltruecolorpngs.value
			&& realpatch->hardware_pixels)
				HWR_DrawTrueColorTexturePatchInCache(mipmap, blockwidth, blockheight,
					texture, patch, realpatch);
			else
				HWR_DrawTexturePatchInCache(mipmap, blockwidth, blockheight, texture, patch, realpatch);

			if (free_patch)
				Patch_Free(realpatch);
		}
	}
	if (copied_simple_png)
		HWR_RepairTinyTrailingPNGEdge((RGBA_t *)block, blockwidth, blockheight);

	if (format2bpp(mipmap->format)==4)
	{
		RGBA_t *pixels = (RGBA_t *)block;
		RGBA_t *palette = pMasterPalette ? pMasterPalette : HWR_GetTexturePalette();
		mipmap->flags &= ~(TF_OPAQUE|TF_TRANSPARENT);

		for (i = 0; i < blocksize; i++)
		{
			                                                                 
                                                                       
                                                                      
                                                                    
                                                                    
                                                      
			if ((mipmap->flags & TF_CHROMAKEYED)
			&& pixels[i].s.red == palette[HWR_PATCHES_CHROMAKEY_COLORINDEX].s.red
			&& pixels[i].s.green == palette[HWR_PATCHES_CHROMAKEY_COLORINDEX].s.green
			&& pixels[i].s.blue == palette[HWR_PATCHES_CHROMAKEY_COLORINDEX].s.blue)
				pixels[i].s.alpha = 0;

			if (pixels[i].s.alpha == 0)
				mipmap->flags |= TF_TRANSPARENT;
		}
		if (!(mipmap->flags & TF_TRANSPARENT))
			mipmap->flags |= TF_OPAQUE;
	}

	grtex->scaleX = 1.0f/(texture->width*FRACUNIT);
	grtex->scaleY = 1.0f/(texture->height*FRACUNIT);
}


void HWR_MakePatch (patch_t *patch, GLPatch_t *grPatch, GLMipmap_t *grMipmap, boolean makebitmap)
{
	if (grMipmap->width == 0)
	{
		grMipmap->width = grMipmap->height = 1;
		while (grMipmap->width < patch->width) grMipmap->width <<= 1;
		while (grMipmap->height < patch->height) grMipmap->height <<= 1;


		grMipmap->flags = 0;


		grMipmap->format = (cv_gltruecolorpngs.value && patch->hardware_pixels)
			? GL_TEXFMT_RGBA : patchformat;
		if (grMipmap->format == GL_TEXFMT_RGBA && patch->hardware_pixels)
			grMipmap->flags |= TF_PALETTE_INDEPENDENT;

		grPatch->max_s = (float)patch->width / (float)grMipmap->width;
		grPatch->max_t = (float)patch->height / (float)grMipmap->height;
	}

	Z_Free(grMipmap->data);
	grMipmap->data = NULL;

	if (makebitmap)
	{
		MakeBlock(grMipmap);

		if (grMipmap->format == GL_TEXFMT_RGBA && cv_gltruecolorpngs.value
		&& patch->hardware_pixels)
			HWR_DrawTrueColorPatchInCache(grMipmap,
				grMipmap->width, grMipmap->height, patch);
		else
			HWR_DrawPatchInCache(grMipmap,
				grMipmap->width, grMipmap->height,
				patch->width, patch->height,
				patch);
	}
}






static size_t gl_numtextures = 0;  
static GLMapTexture_t *gl_textures;  
static GLMapTexture_t *gl_flats;  
boolean gl_maptexturesloaded = false;

void HWR_FreeTextureData(patch_t *patch)
{
	GLPatch_t *grPatch;

	if (!patch || !patch->hardware)
		return;

	grPatch = patch->hardware;

	if (vid.glstate == VID_GL_LIBRARY_LOADED)
		HWD.pfnDeleteTexture(grPatch->mipmap);
	if (grPatch->mipmap->data)
		Z_Free(grPatch->mipmap->data);
}

void HWR_FreeTexture(patch_t *patch)
{
	if (!patch)
		return;

	if (patch->hardware)
	{
		GLPatch_t *grPatch = patch->hardware;

		HWR_FreeTextureColormaps(patch);

		if (grPatch->mipmap)
		{
			HWR_FreeTextureData(patch);
			Z_Free(grPatch->mipmap);
		}

		Z_Free(patch->hardware);
	}

	patch->hardware = NULL;
}


void HWR_FreeTextureColormaps(patch_t *patch)
{
	GLPatch_t *pat;


	if (!patch)
		return;

	pat = patch->hardware;
	if (!pat)
		return;
	memset(pat->colormapcache, 0, sizeof (pat->colormapcache));


	while (pat->mipmap)
	{



		GLMipmap_t *next = NULL;


		if (!pat->mipmap)
			break;


		if (!pat->mipmap->nextcolormap)
			break;


		next = pat->mipmap->nextcolormap;
		pat->mipmap->nextcolormap = next->nextcolormap;


		if (next->data)
			Z_Free(next->data);
		if (next->colormap)
			Z_Free(next->colormap);
		HWD.pfnDeleteTexture(next);


		free(next);
	}
}

static boolean FreeTextureCallback(void *mem)
{
	patch_t *patch = (patch_t *)mem;
	HWR_FreeTexture(patch);
	return false;
}

static boolean FreeColormapsCallback(void *mem)
{
	patch_t *patch = (patch_t *)mem;
	HWR_FreeTextureColormaps(patch);
	return false;
}

static void HWR_FreePatchCache(boolean freeall)
{
	boolean (*callback)(void *mem) = FreeTextureCallback;

	if (!freeall)
		callback = FreeColormapsCallback;

	Z_IterateTags(PU_PATCH, PU_PATCH_ROTATED, callback);
	Z_IterateTags(PU_SPRITE, PU_HUDGFX, callback);
}


void HWR_ClearAllTextures(void)
{
	HWD.pfnClearMipMapCache();  
	HWR_FreePatchCache(true);
}






void HWR_ClearLevelTextures(void)
{
	if (!cv_glretaintextures.value)
	{
		HWR_ClearAllTextures();
		return;
	}

	Z_IterateTag(PU_PATCH, FreeColormapsCallback);
	Z_IterateTags(PU_PATCH_LOWPRIORITY, PU_PATCH_ROTATED, FreeTextureCallback);
	Z_IterateTags(PU_SPRITE, PU_HUDGFX, FreeColormapsCallback);
}

void HWR_FreeColormapCache(void)
{
	HWR_FreePatchCache(false);
}

void HWR_InitMapTextures(void)
{
	gl_textures = NULL;
	gl_flats = NULL;
	gl_maptexturesloaded = false;
}

static void DeleteTextureMipmap(GLMipmap_t *grMipmap, boolean delete_mipmap)
{
	HWD.pfnDeleteTexture(grMipmap);

	if (delete_mipmap)
		Z_Free(grMipmap->data);
}

static void FreeMapTexture(GLMapTexture_t *tex, boolean delete_chromakeys)
{
	if (tex->mipmap.nextcolormap)
	{
		DeleteTextureMipmap(tex->mipmap.nextcolormap, delete_chromakeys);
		free(tex->mipmap.nextcolormap);
		tex->mipmap.nextcolormap = NULL;
	}

	DeleteTextureMipmap(&tex->mipmap, true);
}

void HWR_FreeMapTextures(void)
{
	size_t i;

	for (i = 0; i < gl_numtextures; i++)
	{
		FreeMapTexture(&gl_textures[i], true);
		FreeMapTexture(&gl_flats[i], false);
	}



	if (gl_textures)
		free(gl_textures);
	if (gl_flats)
		free(gl_flats);
	gl_textures = NULL;
	gl_flats = NULL;
	gl_numtextures = 0;
	gl_maptexturesloaded = false;
}

void HWR_LoadMapTextures(size_t pnumtextures)
{

	HWR_FreeMapTextures();

	gl_numtextures = pnumtextures;
	gl_textures = calloc(gl_numtextures, sizeof(*gl_textures));
	gl_flats = calloc(gl_numtextures, sizeof(*gl_flats));

	if (gl_textures == NULL || gl_flats == NULL)
		I_Error("HWR_LoadMapTextures: ran out of memory for OpenGL textures");

	gl_maptexturesloaded = true;
}

                                                                           
                                                                               
                                                                             
                                          
boolean HWR_MapTexturesAreCurrent(size_t pnumtextures)
{
	return gl_maptexturesloaded && gl_textures && gl_flats
		&& gl_numtextures == pnumtextures;
}




GLMapTexture_t *HWR_GetTexture(INT32 tex, boolean chromakeyed)
{
	precise_t upload_started = 0;
	precise_t prepare_started = 0;
	precise_t driver_started = 0;
	char upload_operation[96];

	if (tex < 0 || tex >= (signed)gl_numtextures)
	{
#ifdef PARANOIA
		I_Error("HWR_GetTexture: Invalid texture ID %d", tex);
#else
		tex = 0;
#endif
	}

	GLMapTexture_t *grtex = &gl_textures[tex];

	GLMipmap_t *grMipmap = &grtex->mipmap;
	GLMipmap_t *originalMipmap = grMipmap;

	                                                                       
                                                                            
                                                                             
                                                                     
	if (!originalMipmap->width || !originalMipmap->height)
	{
		originalMipmap->flags = TF_WRAPXY;
		originalMipmap->width = (UINT16)textures[tex]->width;
		originalMipmap->height = (UINT16)textures[tex]->height;
		originalMipmap->format = textureformat;
	}

	                                                                          
                                                                            
                                                                             
                                                                          
	if (chromakeyed)
	{
		UINT32 alphaflags;

		if (!originalMipmap->nextcolormap)
		{
			GLMipmap_t *newMipmap = calloc(1, sizeof (*grMipmap));
			if (newMipmap == NULL)
				I_Error("%s: Out of memory", "HWR_GetTexture");

			newMipmap->flags = originalMipmap->flags | TF_CHROMAKEYED;
			newMipmap->width = originalMipmap->width;
			newMipmap->height = originalMipmap->height;
			newMipmap->format = originalMipmap->format;
			originalMipmap->nextcolormap = newMipmap;
		}


		grMipmap = originalMipmap->nextcolormap;
		                                                                    
                                                                            
                                                                      
		alphaflags = grMipmap->flags & (TF_OPAQUE|TF_TRANSPARENT);
		if (!alphaflags)
			alphaflags = originalMipmap->flags & (TF_OPAQUE|TF_TRANSPARENT);
		grMipmap->flags = (originalMipmap->flags
			& ~(TF_OPAQUE|TF_TRANSPARENT)) | TF_CHROMAKEYED | alphaflags;
		grMipmap->width = originalMipmap->width;
		grMipmap->height = originalMipmap->height;
		 
		if (!grMipmap->data)
			grMipmap->format = originalMipmap->format;
	}

	if (!grMipmap->downloaded)
		upload_started = I_GetPreciseTime();
	if (!grMipmap->data && !grMipmap->downloaded)
	{
		prepare_started = I_GetPreciseTime();
		HWR_GenerateTexture(tex, grtex, grMipmap);
		snprintf(upload_operation, sizeof upload_operation,
			"OpenGL texture prepare %.8s", textures[tex]->name);
		I_ReportHitch(upload_operation, prepare_started, 10);
	}

	if (!grMipmap->downloaded)
	{
		driver_started = I_GetPreciseTime();
		HWD.pfnSetTexture(grMipmap);
		snprintf(upload_operation, sizeof upload_operation,
			"OpenGL driver upload %.8s", textures[tex]->name);
		I_ReportHitch(upload_operation, driver_started, 10);
	}
	HWR_SetCurrentTexture(grMipmap);
	if (upload_started)
	{
		const precise_t upload_elapsed = I_GetPreciseTime() - upload_started;
		ps_hw_textureuploadtime.value.p += upload_elapsed;
		ps_hw_textureuploads.value.i++;
		snprintf(upload_operation, sizeof upload_operation,
			"OpenGL texture upload %.8s", textures[tex]->name);
		I_ReportHitch(upload_operation, upload_started, 20);
	}

	if (grMipmap->data)
		Z_ChangeTag(grMipmap->data, PU_HWRCACHE_UNLOCKED);

	return grtex;
}

static void HWR_CacheRawFlat(GLMipmap_t *grMipmap, lumpnum_t flatlumpnum)
{
	size_t size = W_LumpLength(flatlumpnum);
	UINT16 pflatsize = R_GetFlatSize(size);


	grMipmap->format = GL_TEXFMT_P_8;
	grMipmap->flags = TF_WRAPXY;

	grMipmap->width = pflatsize;
	grMipmap->height = pflatsize;


	W_ReadLump(flatlumpnum, Z_Malloc(size, PU_HWRCACHE, &grMipmap->data));
}


void HWR_GetRawFlat(lumpnum_t flatlumpnum)
{
	GLMipmap_t *grmip;
	patch_t *patch;

	if (flatlumpnum == LUMPERROR)
		return;

	patch = HWR_GetCachedGLPatch(flatlumpnum);
	grmip = ((GLPatch_t *)Patch_AllocateHardwarePatch(patch))->mipmap;
	if (!grmip->downloaded && !grmip->data)
		HWR_CacheRawFlat(grmip, flatlumpnum);


	if (!grmip->downloaded)
		HWD.pfnSetTexture(grmip);
	HWR_SetCurrentTexture(grmip);


	Z_ChangeTag(grmip->data, PU_HWRCACHE_UNLOCKED);
}

static void MakeLevelFlatMipmap(GLMipmap_t *grMipmap, INT32 texturenum, UINT16 flags)
{
	grMipmap->format = GL_TEXFMT_P_8;
	grMipmap->flags = flags;

	grMipmap->width  = (UINT16)textures[texturenum]->width;
	grMipmap->height = (UINT16)textures[texturenum]->height;
}

void HWR_GetLevelFlat(levelflat_t *levelflat, boolean chromakeyed)
{
	INT32 texturenum;
	precise_t upload_started = 0;
	char upload_operation[96];

	if (levelflat->type == LEVELFLAT_NONE || levelflat->texture_id < 0
		|| levelflat->texture_id >= numtextures
		|| !HWR_MapTexturesAreCurrent(numtextures))
	{
		HWR_SetCurrentTexture(NULL);
		return;
	}

	texturenum = texturetranslation[levelflat->texture_id];
	if (texturenum < 0 || texturenum >= (signed)gl_numtextures)
	{
		HWR_SetCurrentTexture(NULL);
		return;
	}

	GLMapTexture_t *grtex = &gl_flats[texturenum];

	GLMipmap_t *grMipmap = &grtex->mipmap;
	GLMipmap_t *originalMipmap = grMipmap;
	if (!originalMipmap->data || !originalMipmap->downloaded)
		upload_started = I_GetPreciseTime();

	                                                                           
                                                                            
                                                                         
                                                                   
	if (!originalMipmap->width || !originalMipmap->height)
		MakeLevelFlatMipmap(originalMipmap, texturenum, TF_WRAPXY);

	if (!originalMipmap->data)
	{
		if (HWR_TextureHasTrueColorPNG(textures[texturenum]))
			HWR_GenerateTexture(texturenum, grtex, originalMipmap);
		else
		{
			size_t size = originalMipmap->width * originalMipmap->height;
			memcpy(Z_Malloc(size, PU_HWRCACHE, &originalMipmap->data),
				R_GetFlatForTexture(texturenum), size);
		}
	}


	if (chromakeyed)
	{
		UINT32 alphaflags;
		if (!originalMipmap->data)
		{
			HWR_SetCurrentTexture(NULL);
			return;
		}


		if (!originalMipmap->nextcolormap)
		{
			GLMipmap_t *newMipmap = calloc(1, sizeof (*grMipmap));
			if (newMipmap == NULL)
				I_Error("%s: Out of memory", "HWR_GetLevelFlat");
			                                                                 
                                                                       
                                                                   
                                                                   
                                                                      
                   
			originalMipmap->nextcolormap = newMipmap;
		}

		                                                                     
                                                                          
                                                                  
		alphaflags = originalMipmap->nextcolormap->flags
			& (TF_OPAQUE|TF_TRANSPARENT);
		if (!alphaflags)
			alphaflags = originalMipmap->flags & (TF_OPAQUE|TF_TRANSPARENT);
		originalMipmap->nextcolormap->flags = (originalMipmap->flags
			& ~(TF_OPAQUE|TF_TRANSPARENT)) | TF_CHROMAKEYED | alphaflags;
		originalMipmap->nextcolormap->width = originalMipmap->width;
		originalMipmap->nextcolormap->height = originalMipmap->height;
		originalMipmap->nextcolormap->format = originalMipmap->format;


		grMipmap = originalMipmap->nextcolormap;




		grMipmap->data = originalMipmap->data;
	}

	if (!grMipmap->downloaded)
		HWD.pfnSetTexture(grMipmap);
	HWR_SetCurrentTexture(grMipmap);
	if (upload_started)
	{
		snprintf(upload_operation, sizeof upload_operation,
			"OpenGL flat upload %.8s", textures[texturenum]->name);
		I_ReportHitch(upload_operation, upload_started, 20);
	}

	                                                                            
                                                                            
                                                                             
	Z_ChangeTag(originalMipmap->data, PU_HWRCACHE_UNLOCKED);
}




static void HWR_LoadPatchMipmap(patch_t *patch, GLMipmap_t *grMipmap, UINT32 extraflags)
{
	GLPatch_t *grPatch = patch->hardware;
	precise_t upload_started = 0;
	char upload_operation[96];
	if ((extraflags & ~grMipmap->flags) || !grMipmap->downloaded)
		upload_started = I_GetPreciseTime();
	                                                                           
                                                                            
                                                                         
	if ((extraflags & ~grMipmap->flags) && grMipmap->downloaded)
		HWD.pfnDeleteTexture(grMipmap);
	if (!grMipmap->downloaded && !grMipmap->data)
		HWR_MakePatch(patch, grPatch, grMipmap, true);
	grMipmap->flags |= extraflags;


	if (!grMipmap->downloaded)
		HWD.pfnSetTexture(grMipmap);
	HWR_SetCurrentTexture(grMipmap);
	if (upload_started)
	{
		snprintf(upload_operation, sizeof upload_operation,
			"OpenGL patch upload %dx%d", patch->width, patch->height);
		I_ReportHitch(upload_operation, upload_started, 20);
	}


	if (grMipmap->data)
		Z_ChangeTag(grMipmap->data, PU_HWRCACHE_UNLOCKED);
}




static void HWR_UpdatePatchMipmap(patch_t *patch, GLMipmap_t *grMipmap, UINT32 extraflags)
{
	GLPatch_t *grPatch = patch->hardware;
	HWR_MakePatch(patch, grPatch, grMipmap, true);
	if ((extraflags & ~grMipmap->flags) && grMipmap->downloaded)
		HWD.pfnDeleteTexture(grMipmap);
	grMipmap->flags |= extraflags;



	if (!grMipmap->downloaded)
		HWD.pfnSetTexture(grMipmap);
	else
		HWD.pfnUpdateTexture(grMipmap);
	HWR_SetCurrentTexture(grMipmap);


	Z_ChangeTag(grMipmap->data, PU_HWRCACHE_UNLOCKED);
}




static void HWR_GetPatchWithFlags(patch_t *patch, UINT32 extraflags)
{
	                                                                     
                                                                          
                                                              
	if (!patch)
	{
		HWR_SetCurrentTexture(NULL);
		return;
	}

	if (!patch->hardware)
		Patch_CreateGL(patch);
	HWR_LoadPatchMipmap(patch, ((GLPatch_t *)patch->hardware)->mipmap, extraflags);
}

void HWR_GetPatch(patch_t *patch)
{
	HWR_GetPatchWithFlags(patch, 0);
}

void HWR_GetSpritePatch(patch_t *patch)
{
	HWR_GetPatchWithFlags(patch, TF_SPRITE);
}




static void HWR_GetMappedPatchWithFlags(patch_t *patch, const UINT8 *colormap, UINT32 extraflags)
{
	GLPatch_t *grPatch;
	GLMipmap_t *grMipmap, *newMipmap;
	size_t cacheindex;

	if (!patch)
	{
		HWR_SetCurrentTexture(NULL);
		return;
	}

	if (!patch->hardware)
		Patch_CreateGL(patch);
	grPatch = patch->hardware;

	if (colormap == colormaps || colormap == NULL)
	{

		HWR_GetPatchWithFlags(patch, extraflags);
		return;
	}




	cacheindex = ((size_t)colormap >> 4) & 15;
	grMipmap = grPatch->colormapcache[cacheindex];
	if (grMipmap && grMipmap->colormap && grMipmap->colormap->source == colormap)
	{
		if (memcmp(grMipmap->colormap->data, colormap, 256 * sizeof(UINT8)))
		{
			M_Memcpy(grMipmap->colormap->data, colormap, 256 * sizeof(UINT8));
			HWR_UpdatePatchMipmap(patch, grMipmap, extraflags);
		}
		else
			HWR_LoadPatchMipmap(patch, grMipmap, extraflags);
		return;
	}



	for (grMipmap = grPatch->mipmap; grMipmap->nextcolormap; )
	{
		grMipmap = grMipmap->nextcolormap;
		if (grMipmap->colormap && grMipmap->colormap->source == colormap)
		{
			grPatch->colormapcache[cacheindex] = grMipmap;
			if (memcmp(grMipmap->colormap->data, colormap, 256 * sizeof(UINT8)))
			{
				M_Memcpy(grMipmap->colormap->data, colormap, 256 * sizeof(UINT8));
				HWR_UpdatePatchMipmap(patch, grMipmap, extraflags);
			}
			else
				HWR_LoadPatchMipmap(patch, grMipmap, extraflags);
			return;
		}
	}






	newMipmap = calloc(1, sizeof (*newMipmap));
	if (newMipmap == NULL)
		I_Error("%s: Out of memory", "HWR_GetMappedPatch");
	grMipmap->nextcolormap = newMipmap;

	newMipmap->colormap = Z_Calloc(sizeof(*newMipmap->colormap), PU_HWRPATCHCOLMIPMAP, NULL);
	newMipmap->colormap->source = colormap;
	M_Memcpy(newMipmap->colormap->data, colormap, 256 * sizeof(UINT8));
	grPatch->colormapcache[cacheindex] = newMipmap;

	HWR_LoadPatchMipmap(patch, newMipmap, extraflags);
}

void HWR_GetMappedPatch(patch_t *patch, const UINT8 *colormap)
{
	HWR_GetMappedPatchWithFlags(patch, colormap, 0);
}

void HWR_GetMappedSpritePatch(patch_t *patch, const UINT8 *colormap)
{
	HWR_GetMappedPatchWithFlags(patch, colormap, TF_SPRITE);
}

void HWR_UnlockCachedPatch(GLPatch_t *gpatch)
{
	if (!gpatch)
		return;

	Z_ChangeTag(gpatch->mipmap->data, PU_HWRCACHE_UNLOCKED);
}

patch_t *HWR_GetCachedGLPatchPwad(UINT16 wadnum, UINT16 lumpnum)
{
	lumpcache_t *lumpcache = wadfiles[wadnum]->patchcache;
	if (!lumpcache[lumpnum])
	{
		void *ptr = Patch_Create(0, 0);
		Z_SetUser(ptr, &lumpcache[lumpnum]);
		Patch_AllocateHardwarePatch(ptr);
	}
	return (patch_t *)(lumpcache[lumpnum]);
}

patch_t *HWR_GetCachedGLPatch(lumpnum_t lumpnum)
{
	return HWR_GetCachedGLPatchPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum));
}


static void HWR_DrawFadeMaskInCache(GLMipmap_t *mipmap, INT32 pblockwidth, INT32 pblockheight,
	lumpnum_t fademasklumpnum, UINT16 fmwidth, UINT16 fmheight)
{
	INT32 i,j;
	fixed_t posx, posy, stepx, stepy;
	UINT8 *block = mipmap->data;  
	UINT8 *flat;
	UINT8 *dest, *src, texel;
	RGBA_t col;
	RGBA_t *palette = HWR_GetTexturePalette();


	W_ReadLump(fademasklumpnum, Z_Malloc(W_LumpLength(fademasklumpnum),
		PU_HWRCACHE, &flat));

	stepy = ((INT32)fmheight<<FRACBITS)/pblockheight;
	stepx = ((INT32)fmwidth<<FRACBITS)/pblockwidth;
	posy = 0;
	for (j = 0; j < pblockheight; j++)
	{
		posx = 0;
		dest = &block[j*(mipmap->width)];  
		src = &flat[(posy>>FRACBITS)*SHORT(fmwidth)];
		for (i = 0; i < pblockwidth;i++)
		{

			texel = src[(posx)>>FRACBITS];
			col = palette[texel];
			*dest = col.s.red;  

			dest++;
			posx += stepx;
		}
		posy += stepy;
	}

	Z_Free(flat);
}

static void HWR_CacheFadeMask(GLMipmap_t *grMipmap, lumpnum_t fademasklumpnum)
{
	size_t size;
	UINT16 fmheight = 0, fmwidth = 0;


	grMipmap->format = GL_TEXFMT_ALPHA_8;  
	grMipmap->flags = 0;

	size = W_LumpLength(fademasklumpnum);

	switch (size)
	{

		case 256000:  
			fmwidth = 640;
			fmheight = 400;
			break;
		case 64000:  
			fmwidth = 320;
			fmheight = 200;
			break;
		case 16000:  
			fmwidth = 160;
			fmheight = 100;
			break;
		case 4000:  
			fmwidth = 80;
			fmheight = 50;
			break;
		default:  
			CONS_Alert(CONS_WARNING, "Fade mask lump of incorrect size, ignored\n");  
			break;
	}


	grMipmap->width  = fmwidth;
	grMipmap->height = fmheight;

	MakeBlock(grMipmap);

	HWR_DrawFadeMaskInCache(grMipmap, fmwidth, fmheight, fademasklumpnum, fmwidth, fmheight);


}


void HWR_GetFadeMask(lumpnum_t fademasklumpnum)
{
	patch_t *patch = HWR_GetCachedGLPatch(fademasklumpnum);
	GLMipmap_t *grmip = ((GLPatch_t *)Patch_AllocateHardwarePatch(patch))->mipmap;
	if (!grmip->downloaded && !grmip->data)
		HWR_CacheFadeMask(grmip, fademasklumpnum);

	HWD.pfnSetTexture(grmip);


	Z_ChangeTag(grmip->data, PU_HWRCACHE_UNLOCKED);
}





void HWR_SetPalette(RGBA_t *palette)
{
	if (HWR_ShouldUsePaletteRendering())
	{


		if (cv_glpalettedepth.value == 16)
		{




			RGBA_t crushed_palette[256];
			int i;
			for (i = 0; i < 256; i++)
			{
				float fred = (float)(palette[i].s.red >> 3);
				float fgreen = (float)(palette[i].s.green >> 2);
				float fblue = (float)(palette[i].s.blue >> 3);
				crushed_palette[i].s.red = (UINT8)(fred / 31.0f * 255.0f);
				crushed_palette[i].s.green = (UINT8)(fgreen / 63.0f * 255.0f);
				crushed_palette[i].s.blue = (UINT8)(fblue / 31.0f * 255.0f);
				crushed_palette[i].s.alpha = 255;
			}
			HWD.pfnSetScreenPalette(crushed_palette);
		}
		else
		{
			HWD.pfnSetScreenPalette(palette);
		}


		if (!(gamestate == GS_LEVEL || (gamestate == GS_TITLESCREEN && titlemapinaction)))
			HWR_SetMapPalette();
	}
	else
	{

		HWD.pfnSetTexturePalette(palette);

		memset(mapPalette, 0, sizeof(mapPalette));


		if (patchformat == GL_TEXFMT_RGBA || textureformat == GL_TEXFMT_RGBA)
		{
			Z_FreeTag(PU_HWRCACHE);
			Z_FreeTag(PU_HWRCACHE_UNLOCKED);
		}
	}
}

static void HWR_SetPaletteLookup(RGBA_t *palette)
{
	int r, g, b;
	UINT8 *lut = Z_Malloc(
		HWR_PALETTE_LUT_SIZE*HWR_PALETTE_LUT_SIZE*HWR_PALETTE_LUT_SIZE*sizeof(UINT8),
		PU_STATIC, NULL);
#define STEP_SIZE (256/HWR_PALETTE_LUT_SIZE)
	for (b = 0; b < HWR_PALETTE_LUT_SIZE; b++)
	{
		for (g = 0; g < HWR_PALETTE_LUT_SIZE; g++)
		{
			for (r = 0; r < HWR_PALETTE_LUT_SIZE; r++)
			{
				lut[b*HWR_PALETTE_LUT_SIZE*HWR_PALETTE_LUT_SIZE+g*HWR_PALETTE_LUT_SIZE+r] =
					NearestPaletteColor(r*STEP_SIZE, g*STEP_SIZE, b*STEP_SIZE, palette);
			}
		}
	}
#undef STEP_SIZE
	HWD.pfnSetPaletteLookup(lut);
	Z_Free(lut);
}




void HWR_SetMapPalette(void)
{
	RGBA_t RGBA_converted[256];
	RGBA_t *palette;
	int i;

	if (!(gamestate == GS_LEVEL || (gamestate == GS_TITLESCREEN && titlemapinaction)))
	{

		palette = pMasterPalette;
	}
	else
	{


		lumpnum_t lumpnum = W_GetNumForName(GetPalette());
		size_t palsize = W_LumpLength(lumpnum);
		UINT8 *RGB_data;
		if (palsize < 768)  
			I_Error("HWR_SetMapPalette: A programmer assumed palette lumps are at least 768 bytes long, but apparently this was a wrong assumption!\n");
		RGB_data = W_CacheLumpNum(lumpnum, PU_CACHE);

		for (i = 0; i < 256; i++)
		{
			RGBA_converted[i].s.red = *(RGB_data++);
			RGBA_converted[i].s.green = *(RGB_data++);
			RGBA_converted[i].s.blue = *(RGB_data++);
			RGBA_converted[i].s.alpha = 255;
		}
		palette = RGBA_converted;
	}


	if (memcmp(mapPalette, palette, sizeof(mapPalette)))
	{
		memcpy(mapPalette, palette, sizeof(mapPalette));


		HWR_SetPaletteLookup(mapPalette);
		HWD.pfnSetTexturePalette(mapPalette);
		if (patchformat == GL_TEXFMT_RGBA || textureformat == GL_TEXFMT_RGBA)
		{
			Z_FreeTag(PU_HWRCACHE);
			Z_FreeTag(PU_HWRCACHE_UNLOCKED);
		}
	}
}



UINT32 HWR_CreateLightTable(UINT8 *lighttable, RGBA_t *hw_lighttable)
{
	UINT32 i;
	RGBA_t *palette = HWR_GetTexturePalette();



	for (i = 0; i < 256 * 32; i++)
		hw_lighttable[i] = palette[lighttable[i]];

	return HWD.pfnCreateLightTable(hw_lighttable);
}


void HWR_UpdateLightTable(UINT32 id, UINT8 *lighttable, RGBA_t *hw_lighttable)
{
	UINT32 i;
	RGBA_t *palette = HWR_GetTexturePalette();

	for (i = 0; i < 256 * 32; i++)
		hw_lighttable[i] = palette[lighttable[i]];

	HWD.pfnUpdateLightTable(id, hw_lighttable);
}


UINT32 HWR_GetLightTableID(extracolormap_t *colormap)
{
	boolean default_colormap = false;
	if (!colormap)
	{
		colormap = R_GetDefaultColormap();  

		default_colormap = true;
	}

	UINT8 *colormap_pointer;

	if (default_colormap)
		colormap_pointer = colormaps;  
	else
		colormap_pointer = colormap->colormap;


	if (colormap->gl_lighttable.data == NULL)
	{
		Z_Malloc(256 * 32 * sizeof(RGBA_t), PU_HWRLIGHTTABLEDATA, &colormap->gl_lighttable.data);
	}


	if (!colormap->gl_lighttable.id)
	{
		colormap->gl_lighttable.id = HWR_CreateLightTable(colormap_pointer, colormap->gl_lighttable.data);
	}

	else if (colormap->gl_lighttable.needs_update)
	{
		HWR_UpdateLightTable(colormap->gl_lighttable.id, colormap_pointer, colormap->gl_lighttable.data);
	}

	colormap->gl_lighttable.needs_update = false;

	return colormap->gl_lighttable.id;
}



void HWR_ClearLightTables(void)
{
	Z_FreeTag(PU_HWRLIGHTTABLEDATA);

	if (vid.glstate == VID_GL_LIBRARY_LOADED)
		HWD.pfnClearLightTables();
}

#endif  
