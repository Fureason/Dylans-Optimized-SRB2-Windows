

#define TEXT_ATLAS_SIZE 512
#define TEXT_ATLAS_PAGES 8
#define TEXT_GLYPHS 4096
#define TEXT_BATCH_QUADS 2048

typedef struct
{
	GLMipmap_t mipmap;
	INT32 x, y, rowheight, bpp;
	boolean used, dirty;
} text_atlas_page_t;

typedef struct
{
	const patch_t *patch;
	const UINT8 *colormap;
	UINT8 translation[256];
	text_atlas_page_t *page;
	INT16 x, y, width, height;
} text_glyph_t;

static text_atlas_page_t text_pages[TEXT_ATLAS_PAGES];
static text_glyph_t text_glyphs[TEXT_GLYPHS];
static UINT64 text_patch_bloom[64];
static size_t text_glyph_count, text_quad_count;
static INT32 text_batch_depth;
static RGBA_t text_palette[256];
static boolean text_palette_valid, text_indices_ready;
static text_atlas_page_t *text_batch_page;
static FBITFIELD text_batch_flags;
static UINT8 text_batch_alpha;
static FOutVector text_vertices[TEXT_BATCH_QUADS*4];
static UINT32 text_indices[TEXT_BATCH_QUADS*6];

static void HWR_FlushTextBatch(void)
{
	FSurfaceInfo surface = {0};
	if (!text_quad_count) return;
	if (text_batch_page->dirty && text_batch_page->mipmap.downloaded)
		HWD.pfnUpdateTexture(&text_batch_page->mipmap);
	else
		HWD.pfnSetTexture(&text_batch_page->mipmap);
	text_batch_page->dirty = false;
	surface.PolyColor.s.red = surface.PolyColor.s.green = surface.PolyColor.s.blue = 255;
	surface.PolyColor.s.alpha = text_batch_alpha;
	HWD.pfnDrawIndexedTriangles(text_batch_flags & PF_Modulated ? &surface : NULL,
		text_vertices, (FUINT)(text_quad_count*6), text_batch_flags, text_indices);
	text_quad_count = 0;
}

void HWR_ClearTextCache(void)
{
	HWR_FlushTextBatch();
	if (!text_glyph_count) return;
	for (size_t i = 0; i < TEXT_ATLAS_PAGES; i++)
	{
		if (text_pages[i].mipmap.downloaded && HWD.pfnDeleteTexture)
			HWD.pfnDeleteTexture(&text_pages[i].mipmap);
		Z_Free(text_pages[i].mipmap.data);
	}
	memset(text_pages, 0, sizeof text_pages);
	memset(text_glyphs, 0, sizeof text_glyphs);
	memset(text_patch_bloom, 0, sizeof text_patch_bloom);
	text_glyph_count = 0;
	text_batch_page = NULL;
	text_palette_valid = false;
}

void HWR_ForgetTextPatch(const patch_t *patch)
{
	size_t bit;
	if (!text_glyph_count) return;
	bit = ((size_t)patch >> 4) & 4095;
	if (!(text_patch_bloom[bit >> 6] & ((UINT64)1 << (bit & 63)))) return;
	for (size_t i = 0; i < TEXT_GLYPHS; i++)
		if (text_glyphs[i].patch == patch) { HWR_ClearTextCache(); return; }
}

void HWR_BeginTextBatch(void)
{
	if (rendermode != render_opengl || !cv_gluibatching.value || !HWD.pfnDrawIndexedTriangles) return;
	if (!text_batch_depth)
	{
		RGBA_t *palette = HWR_GetTexturePalette();
		if (!text_palette_valid || memcmp(text_palette, palette, sizeof text_palette))
		{
			HWR_ClearTextCache();
			memcpy(text_palette, palette, sizeof text_palette);
			text_palette_valid = true;
		}
	}
	text_batch_depth++;
}

void HWR_EndTextBatch(void)
{
	if (text_batch_depth && !--text_batch_depth) HWR_FlushTextBatch();
}

static text_atlas_page_t *HWR_TextAtlasSpace(GLTextureFormat_t format,
	UINT32 flags, INT32 width, INT32 height, INT32 bpp, INT32 *x, INT32 *y)
{
	for (size_t i = 0; i < TEXT_ATLAS_PAGES; i++)
	{
		text_atlas_page_t *page = &text_pages[i];
		INT32 nextx = page->x, nexty = page->y, rowheight = page->rowheight;
		if (page->used && (page->mipmap.format != format || page->mipmap.flags != flags)) continue;
		if (nextx + width + 2 > TEXT_ATLAS_SIZE)
		{
			nextx = 0; nexty += rowheight; rowheight = 0;
		}
		if (nexty + height + 2 > TEXT_ATLAS_SIZE) continue;
		if (!page->used)
		{
			page->mipmap.format = format;
			page->mipmap.flags = flags;
			page->mipmap.width = page->mipmap.height = TEXT_ATLAS_SIZE;
			page->mipmap.data = Z_Calloc(TEXT_ATLAS_SIZE*TEXT_ATLAS_SIZE*bpp, PU_STATIC, &page->mipmap.data);
			page->bpp = bpp; page->used = true;
		}
		*x = nextx+1; *y = nexty+1;
		page->x = nextx+width+2; page->y = nexty;
		page->rowheight = max(rowheight, height+2);
		return page;
	}
	return NULL;
}

static text_glyph_t *HWR_TextGlyph(patch_t *patch, const UINT8 *colormap)
{
	GLMipmap_t temporary = {0};
	GLPatch_t hardware = {0};
	GLColormap_t translation;
	text_glyph_t *glyph = NULL;
	INT32 bpp, x, y;
	boolean transparent = false;
	UINT32 flags;
	size_t hash;
	if (!text_batch_depth || patch->width <= 0 || patch->height <= 0
	|| patch->width > 64 || patch->height > 64) return NULL;
	if (colormap == colormaps) colormap = NULL;
	hash = (((size_t)patch >> 4) ^ ((size_t)colormap >> 4)) & (TEXT_GLYPHS-1);
	for (size_t i = 0; i < TEXT_GLYPHS; i++)
	{
		glyph = &text_glyphs[(hash+i) & (TEXT_GLYPHS-1)];
		if (!glyph->patch) break;
		if (glyph->patch == patch && glyph->colormap == colormap)
		{
			if (glyph->width != patch->width || glyph->height != patch->height
			|| (colormap && memcmp(glyph->translation, colormap, 256)))
			{
				HWR_ClearTextCache();
				return HWR_TextGlyph(patch, colormap);
			}
			return glyph->page ? glyph : NULL;
		}
		glyph = NULL;
	}
	if (!glyph || text_glyph_count >= TEXT_GLYPHS) return NULL;
	glyph->patch = patch; glyph->colormap = colormap;
	glyph->width = patch->width; glyph->height = patch->height;
	text_glyph_count++;
	{
		size_t bit = ((size_t)patch >> 4) & 4095;
		text_patch_bloom[bit >> 6] |= (UINT64)1 << (bit & 63);
	}
	if (colormap)
	{
		translation.source = colormap;
		memcpy(translation.data, colormap, 256);
		memcpy(glyph->translation, colormap, 256);
		temporary.colormap = &translation;
	}
	hardware.mipmap = &temporary;
	HWR_MakePatch(patch, &hardware, &temporary, true);
	bpp = temporary.format == GL_TEXFMT_RGBA ? 4 : temporary.format == GL_TEXFMT_AP_88 ? 2 : 0;
	if (!bpp) { Z_Free(temporary.data); return NULL; }


	for (size_t i = bpp-1; i < (size_t)temporary.width*temporary.height*bpp; i += bpp)
		if (!((UINT8 *)temporary.data)[i]) { transparent = true; break; }
	if (!transparent) { Z_Free(temporary.data); return NULL; }
	flags = TF_TRANSPARENT | (temporary.flags & TF_PALETTE_INDEPENDENT);
	glyph->page = HWR_TextAtlasSpace(temporary.format, flags, patch->width, patch->height, bpp, &x, &y);
	if (glyph->page)
	{
		for (INT32 row = 0; row < patch->height; row++)
			memcpy((UINT8 *)glyph->page->mipmap.data + ((y+row)*TEXT_ATLAS_SIZE+x)*bpp,
				(UINT8 *)temporary.data + row*temporary.width*bpp, patch->width*bpp);
		glyph->x = (INT16)x; glyph->y = (INT16)y;
		glyph->page->dirty = true;
	}
	Z_Free(temporary.data);
	return glyph->page ? glyph : NULL;
}

static void HWR_QueueTextQuad(text_glyph_t *glyph, FOutVector vertices[4], FBITFIELD flags, UINT8 alpha)
{
	if (!text_indices_ready)
	{
		for (size_t i = 0; i < TEXT_BATCH_QUADS; i++)
		{
			UINT32 base = (UINT32)i*4, *indices = &text_indices[i*6];
			indices[0]=base; indices[1]=base+1; indices[2]=base+2;
			indices[3]=base; indices[4]=base+2; indices[5]=base+3;
		}
		text_indices_ready = true;
	}
	if (text_quad_count && (text_batch_page != glyph->page || text_batch_flags != flags
	|| text_batch_alpha != alpha || text_quad_count == TEXT_BATCH_QUADS)) HWR_FlushTextBatch();
	text_batch_page = glyph->page; text_batch_flags = flags; text_batch_alpha = alpha;
	for (size_t i = 0; i < 4; i++)
	{
		vertices[i].s = ((float)glyph->x + vertices[i].s*glyph->width) / TEXT_ATLAS_SIZE;
		vertices[i].t = ((float)glyph->y + vertices[i].t*glyph->height) / TEXT_ATLAS_SIZE;
	}
	memcpy(text_vertices + text_quad_count*4, vertices, 4*sizeof(*vertices));
	text_quad_count++;
}
