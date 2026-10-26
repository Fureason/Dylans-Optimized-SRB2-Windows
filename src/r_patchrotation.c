// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by Lactozilla.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_patchrotation.c
/// \brief Patch rotation.

/* patch_t contains build-dependent hardware-renderer fields.  Load the build
 * feature definitions before any header can declare patch_t, otherwise this
 * file writes rotated/rotatedcachecost at the non-hardware offsets. */
#include "doomdef.h"

#include "r_patchrotation.h"
#include "r_things.h" // FEETADJUST (todo: is this needed anymore? -- Monster Iestyn 21 Sep 2023 )
#include "z_zone.h"
#include "w_wad.h"
#include "r_main.h" // R_PointToAngle
#include "command.h"
#include "i_system.h"
#include "i_time.h"

#ifdef ROTSPRITE
fixed_t rollcosang[ROTANGLES];
fixed_t rollsinang[ROTANGLES];

static CV_PossibleValue_t rotspritecache_cons_t[] = {
	{-1, "Auto"}, {0, "Unlimited"}, {64, "64 MB"}, {128, "128 MB"},
	{256, "256 MB"}, {512, "512 MB"}, {1024, "1024 MB"}, {0, NULL}};

// This is an estimated combined cost: the indexed software patch plus the
// RGBA texture OpenGL may create for it. Keeping it bounded is particularly
// important to 32-bit builds, where address space is much smaller than RAM.
consvar_t cv_rotspritecache = CVAR_INIT ("rotspritecache", "Auto", CV_SAVE, rotspritecache_cons_t, NULL);

static size_t rotatedcachecost;
static size_t rotatedcachehighwater;
static size_t rotatedcacheentries;
static size_t rotatedcachefallbacks;
static size_t rotatedcacherejections;
static size_t rotatedcacheautolimit;
static tic_t rotatedcacheautocheck;

static size_t RotatedPatch_AutoCacheLimit(void)
{
	const size_t megabyte = (size_t)1 << 20;
	const size_t minimum = (sizeof(void *) >= 8 ? 128u : 64u) * megabyte;
	const size_t maximum = (sizeof(void *) >= 8 ? 512u : 192u) * megabyte;
	tic_t now = I_GetTime();
	size_t total = 0, available, limit;

	if (rotatedcacheautolimit && now >= rotatedcacheautocheck
	&& now - rotatedcacheautocheck < TICRATE)
		return rotatedcacheautolimit;

	available = I_GetFreeMem(&total);
	limit = total ? total / 32 : minimum;
	if (limit < minimum)
		limit = minimum;
	if (limit > maximum)
		limit = maximum;
	// Do not let this optional cache claim more than a quarter of currently
	// available memory. Keep a small floor so busy systems do not thrash it.
	if (available && limit > available / 4)
		limit = available / 4;
	if (limit < 32u * megabyte)
		limit = 32u * megabyte;

	rotatedcacheautolimit = limit;
	rotatedcacheautocheck = now;
	return limit;
}

static size_t RotatedPatch_CacheLimit(void)
{
	if (cv_rotspritecache.value < 0)
		return RotatedPatch_AutoCacheLimit();
	if (cv_rotspritecache.value == 0)
		return 0;

	if ((size_t)cv_rotspritecache.value > ((size_t)-1 >> 20))
		return (size_t)-1;

	return (size_t)cv_rotspritecache.value << 20;
}

static boolean RotatedPatch_CacheHasRoom(size_t cost)
{
	size_t limit = RotatedPatch_CacheLimit();

	if (!limit)
		return true;
	if (rotatedcachecost >= limit)
		return false;
	return cost <= limit - rotatedcachecost;
}

static patch_t *RotatedPatch_FindNearest(rotsprite_t *rotsprite, INT32 angle, boolean flip)
{
	INT32 distance;
	INT32 base = flip ? rotsprite->angles : 0;

	for (distance = 1; distance < rotsprite->angles; distance++)
	{
		INT32 lower = (angle - distance + rotsprite->angles) % rotsprite->angles;
		INT32 upper = (angle + distance) % rotsprite->angles;

		// Angle zero uses the original sprite and is not stored in this cache.
		if (lower && rotsprite->patches[base + lower])
			return rotsprite->patches[base + lower];
		if (upper && rotsprite->patches[base + upper])
			return rotsprite->patches[base + upper];
	}

	return NULL;
}

static size_t RotatedPatch_MemoryCost(patch_t *patch)
{
	size_t cost = sizeof(*patch);
	size_t pixels = 0;
	size_t posts = 0;
	size_t area;
	size_t cachetexturewidth = 1;
	size_t cachetextureheight = 1;
	INT32 x;

	if (patch->width <= 0 || patch->height <= 0)
		return cost;

	// The legacy OpenGL backend pads patches to power-of-two textures.
	while (cachetexturewidth < (size_t)patch->width)
		cachetexturewidth <<= 1;
	while (cachetextureheight < (size_t)patch->height)
		cachetextureheight <<= 1;
	if (cachetexturewidth > (size_t)-1 / cachetextureheight)
		return (size_t)-1;
	area = cachetexturewidth * cachetextureheight;
	if (area > ((size_t)-1 - cost) / 4)
		return (size_t)-1;
	cost += area * 4; // Conservative RGBA OpenGL texture estimate.

	if ((size_t)patch->width > ((size_t)-1 - cost) / sizeof(column_t))
		return (size_t)-1;
	cost += (size_t)patch->width * sizeof(column_t);

	for (x = 0; x < patch->width; x++)
	{
		size_t p;
		column_t *column = &patch->columns[x];

		if (column->num_posts > (size_t)-1 - posts)
			return (size_t)-1;
		posts += column->num_posts;

		for (p = 0; p < column->num_posts; p++)
		{
			if (column->posts[p].length > (size_t)-1 - pixels)
				return (size_t)-1;
			pixels += column->posts[p].length;
		}
	}

	if (posts > ((size_t)-1 - cost) / sizeof(post_t))
		return (size_t)-1;
	cost += posts * sizeof(post_t);
	if (pixels > (size_t)-1 - cost)
		return (size_t)-1;

	return cost + pixels;
}

void R_RotatedPatchCacheEntryFreed(size_t cost)
{
	if (cost >= rotatedcachecost)
		rotatedcachecost = 0;
	else
		rotatedcachecost -= cost;

	if (rotatedcacheentries)
		rotatedcacheentries--;
}

void R_RotatedPatchCacheStats_f(void)
{
	CONS_Printf("Rotated sprite cache: %lu MB used / ", (unsigned long)(rotatedcachecost >> 20));
	if (cv_rotspritecache.value < 0)
		CONS_Printf("auto (%lu MB current limit)\n",
			(unsigned long)(RotatedPatch_CacheLimit() >> 20));
	else if (cv_rotspritecache.value)
		CONS_Printf("%d MB limit\n", cv_rotspritecache.value);
	else
		CONS_Printf("unlimited\n");
	CONS_Printf("Entries: %lu, peak: %lu MB, nearest-angle fallbacks: %lu, rejected allocations: %lu\n",
		(unsigned long)rotatedcacheentries,
		(unsigned long)(rotatedcachehighwater >> 20),
		(unsigned long)rotatedcachefallbacks,
		(unsigned long)rotatedcacherejections);
}

angle_t R_ModelRotationAngle(interpmobjstate_t *interp)
{
	return interp->spriteroll;
}

angle_t R_SpriteRotationAngle(interpmobjstate_t *interp)
{
#if 0
	angle_t viewingAngle = R_PointToAngle(interp->x, interp->y);

	fixed_t pitchMul = -FINESINE(viewingAngle >> ANGLETOFINESHIFT);
	fixed_t rollMul = FINECOSINE(viewingAngle >> ANGLETOFINESHIFT);

	angle_t rollOrPitch = FixedMul(interp->pitch, pitchMul) + FixedMul(interp->roll, rollMul);

	return (rollOrPitch + R_ModelRotationAngle(interp));
#else
	return R_ModelRotationAngle(interp);
#endif
}

INT32 R_GetRollAngle(angle_t rollangle)
{
	INT32 ra = AngleFixed(rollangle)>>FRACBITS;
#if (ROTANGDIFF > 1)
	ra += (ROTANGDIFF/2);
#endif
	ra /= ROTANGDIFF;
	ra %= ROTANGLES;
	return ra;
}

patch_t *Patch_GetRotated(patch_t *patch, INT32 angle, boolean flip)
{
	rotsprite_t *rotsprite = patch->rotated;
	patch_t *rotated;
	if (rotsprite == NULL || angle < 1 || angle >= ROTANGLES)
		return NULL;

	if (flip)
		angle += rotsprite->angles;

	rotated = rotsprite->patches[angle];
	if (rotated)
		return rotated;

	return RotatedPatch_FindNearest(rotsprite, angle % rotsprite->angles, flip);
}

patch_t *Patch_GetRotatedSprite(
	spriteframe_t *sprite,
	size_t frame, size_t spriteangle,
	boolean flip,
	void *info, INT32 rotationangle)
{
	rotsprite_t *rotsprite = sprite->rotated[spriteangle];
	spriteinfo_t *sprinfo = (spriteinfo_t *)info;
	INT32 idx = rotationangle;

	if (rotationangle < 1 || rotationangle >= ROTANGLES)
		return NULL;

	if (rotsprite == NULL)
	{
		rotsprite = RotatedPatch_Create(ROTANGLES);
		sprite->rotated[spriteangle] = rotsprite;
	}

	if (flip)
		idx += rotsprite->angles;

	if (rotsprite->patches[idx] == NULL)
	{
		patch_t *patch;
		INT32 xpivot = 0, ypivot = 0;
		lumpnum_t lump = sprite->lumppat[spriteangle];

		if (lump == LUMPERROR)
			return NULL;

		patch = W_CachePatchNum(lump, PU_SPRITE);

		if (sprinfo->available)
		{
			xpivot = sprinfo->pivot[frame].x;
			ypivot = sprinfo->pivot[frame].y;
		}
		else
		{
			xpivot = patch->leftoffset;
			ypivot = patch->height / 2;
		}

		if (!RotatedPatch_DoRotation(rotsprite, patch, rotationangle, xpivot, ypivot, flip))
		{
			rotatedcachefallbacks++;
			return RotatedPatch_FindNearest(rotsprite, rotationangle, flip);
		}
	}

	return rotsprite->patches[idx];
}

void Patch_Rotate(patch_t *patch, INT32 angle, INT32 xpivot, INT32 ypivot, boolean flip)
{
	if (patch->rotated == NULL)
		patch->rotated = RotatedPatch_Create(ROTANGLES);
	RotatedPatch_DoRotation(patch->rotated, patch, angle, xpivot, ypivot, flip);
}

rotsprite_t *RotatedPatch_Create(INT32 numangles)
{
	rotsprite_t *rotsprite = Z_Calloc(sizeof(rotsprite_t), PU_STATIC, NULL);
	rotsprite->angles = numangles;
	rotsprite->patches = Z_Calloc(rotsprite->angles * 2 * sizeof(void *), PU_STATIC, NULL);
	return rotsprite;
}

static void RotatedPatch_CalculateDimensions(
	INT32 width, INT32 height,
	fixed_t ca, fixed_t sa,
	INT32 *newwidth, INT32 *newheight)
{
	fixed_t fixedwidth = (width * FRACUNIT);
	fixed_t fixedheight = (height * FRACUNIT);

	fixed_t w1 = abs(FixedMul(fixedwidth, ca) - FixedMul(fixedheight, sa));
	fixed_t w2 = abs(FixedMul(-fixedwidth, ca) - FixedMul(fixedheight, sa));
	fixed_t h1 = abs(FixedMul(fixedwidth, sa) + FixedMul(fixedheight, ca));
	fixed_t h2 = abs(FixedMul(-fixedwidth, sa) + FixedMul(fixedheight, ca));

	w1 = FixedInt(FixedCeil(w1 + (FRACUNIT/2)));
	w2 = FixedInt(FixedCeil(w2 + (FRACUNIT/2)));
	h1 = FixedInt(FixedCeil(h1 + (FRACUNIT/2)));
	h2 = FixedInt(FixedCeil(h2 + (FRACUNIT/2)));

	*newwidth = max(width, max(w1, w2));
	*newheight = max(height, max(h1, h2));
}

boolean RotatedPatch_DoRotation(rotsprite_t *rotsprite, patch_t *patch, INT32 angle, INT32 xpivot, INT32 ypivot, boolean flip)
{
	patch_t *rotated;
	UINT16 *rawdst, *rawconv;
	size_t size, scratchbytes, cachecost;
	pictureflags_t bflip = (flip) ? PICFLAGS_XFLIP : 0;

	INT32 width = patch->width;
	INT32 height = patch->height;
	INT32 leftoffset = patch->leftoffset;
	INT32 newwidth, newheight;

	fixed_t ca = rollcosang[angle];
	fixed_t sa = rollsinang[angle];
	fixed_t xcenter, ycenter;
	INT32 idx = angle;
	INT32 x, y;
	INT32 sx, sy;
	INT32 dx, dy;
	INT32 ox, oy;
	INT32 minx, miny, maxx, maxy;

	// Don't cache angle = 0
	if (angle < 1 || angle >= ROTANGLES)
		return false;

	if (flip)
	{
		idx += rotsprite->angles;
		xpivot = width - xpivot;
		leftoffset = width - leftoffset;
	}

	if (rotsprite->patches[idx])
		return true;

	if (!RotatedPatch_CacheHasRoom(1))
	{
		rotatedcacherejections++;
		return false;
	}

	// Find the dimensions of the rotated patch.
	RotatedPatch_CalculateDimensions(width, height, ca, sa, &newwidth, &newheight);

	xcenter = (xpivot * FRACUNIT);
	ycenter = (ypivot * FRACUNIT);

	if (xpivot != width / 2 || ypivot != height / 2)
	{
		if (newwidth > INT32_MAX / 2 || newheight > INT32_MAX / 2)
		{
			rotatedcacherejections++;
			return false;
		}
		newwidth *= 2;
		newheight *= 2;
	}

	if (newwidth <= 0 || newheight <= 0
		|| (size_t)newwidth > (size_t)-1 / (size_t)newheight)
	{
		rotatedcacherejections++;
		return false;
	}

	minx = newwidth;
	miny = newheight;
	maxx = 0;
	maxy = 0;

	// Draw the rotated sprite to a temporary buffer.
	size = (size_t)newwidth * (size_t)newheight;
	if (!size)
		size = (size_t)width * (size_t)height;
	if (size > (size_t)-1 / sizeof(UINT16))
	{
		rotatedcacherejections++;
		return false;
	}
	scratchbytes = size * sizeof(UINT16);
	if (RotatedPatch_CacheLimit() && scratchbytes > RotatedPatch_CacheLimit() / 2)
	{
		rotatedcacherejections++;
		return false;
	}
	rawdst = Z_Calloc(size * sizeof(UINT16), PU_STATIC, NULL);

	for (dy = 0; dy < newheight; dy++)
	{
		for (dx = 0; dx < newwidth; dx++)
		{
			x = (dx - (newwidth / 2)) * FRACUNIT;
			y = (dy - (newheight / 2)) * FRACUNIT;
			sx = FixedMul(x, ca) + FixedMul(y, sa) + xcenter;
			sy = -FixedMul(x, sa) + FixedMul(y, ca) + ycenter;

			sx >>= FRACBITS;
			sy >>= FRACBITS;

			if (sx >= 0 && sy >= 0 && sx < width && sy < height)
			{
				void *input = Picture_GetPatchPixel(patch, PICFMT_PATCH, sx, sy, bflip);
				if (input != NULL)
				{
					rawdst[((size_t)dy * (size_t)newwidth) + (size_t)dx] = (0xFF00 | (*(UINT8 *)input));
					if (dx < minx)
						minx = dx;
					if (dy < miny)
						miny = dy;
					if (dx > maxx)
						maxx = dx;
					if (dy > maxy)
						maxy = dy;
				}
			}
		}
	}

	ox = (newwidth / 2) + (leftoffset - xpivot);
	oy = (newheight / 2) + (patch->topoffset - ypivot);

	if (minx <= maxx && miny <= maxy)
	{
		width = maxx - minx + 1;
		height = maxy - miny + 1;

		if ((size_t)width * (size_t)height < size)
		{
			UINT16 *src, *dest;

			size = (size_t)width * (size_t)height;
			rawconv = Z_Calloc(size * sizeof(UINT16), PU_STATIC, NULL);

			src = &rawdst[((size_t)miny * (size_t)newwidth) + (size_t)minx];
			dest = rawconv;
			dy = height;

			while (dy--)
			{
				M_Memcpy(dest, src, (size_t)width * sizeof(UINT16));
				dest += width;
				src += newwidth;
			}

			ox -= minx;
			oy -= miny;

			Z_Free(rawdst);
		}
		else
			rawconv = rawdst;
	}
	else
	{
		// An entirely transparent sprite only needs a one-pixel placeholder.
		Z_Free(rawdst);
		width = height = 1;
		size = 1;
		ox = oy = 0;
		rawconv = Z_Calloc(sizeof(UINT16), PU_STATIC, NULL);
	}

	if (width > INT16_MAX || height > INT16_MAX
		|| size > ((size_t)-1 - sizeof(patch_t) - (size_t)width * sizeof(column_t)) / 5
		|| !RotatedPatch_CacheHasRoom(sizeof(patch_t) + (size_t)width * sizeof(column_t) + size * 5))
	{
		Z_Free(rawconv);
		rotatedcacherejections++;
		return false;
	}

	// make patch
	rotated = (patch_t *)Picture_Convert(PICFMT_FLAT16, rawconv, PICFMT_PATCH, 0, NULL, width, height, 0, 0, 0);
	cachecost = RotatedPatch_MemoryCost(rotated);
	if (!RotatedPatch_CacheHasRoom(cachecost))
	{
		Patch_Free(rotated);
		Z_Free(rawconv);
		rotatedcacherejections++;
		return false;
	}

	Z_ChangeTag(rotated, PU_PATCH_ROTATED);
	Z_SetUser(rotated, (void **)(&rotsprite->patches[idx]));
	Z_Free(rawconv);
	rotated->rotatedcachecost = cachecost;
	rotatedcachecost += cachecost;
	rotatedcacheentries++;
	if (rotatedcachecost > rotatedcachehighwater)
		rotatedcachehighwater = rotatedcachecost;

	rotated->leftoffset = ox;
	rotated->topoffset = oy;
	return true;
}
#endif
