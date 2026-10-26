// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_batching.c
/// \brief Draw call batching and related things.

#ifdef HWRENDER
#include "hw_glob.h"
#include "hw_batching.h"
#include "../i_system.h"

// The texture for the next polygon given to HWR_ProcessPolygon.
// Set with HWR_SetCurrentTexture.
GLMipmap_t *current_texture = NULL;

boolean currently_batching = false;

UINT32* finalVertexIndexArray = NULL;// contains indexes for glDrawElements, taking into account fan->triangles conversion
int finalVertexIndexArrayAllocSize = 65536 * 3;
//GLubyte* colorArray = NULL;// contains color data to be sent to gpu, if needed
//int colorArrayAllocSize = 65536;
// not gonna use this for now, just sort by color and change state when it changes
// later maybe when using vertex attributes if it's needed

PolygonArrayEntry* polygonArray = NULL;// contains the polygon data from DrawPolygon, waiting to be processed
int polygonArraySize = 0;
UINT32* polygonIndexArray = NULL;// contains sorting pointers for polygonArray
static UINT32* polygonScratchIndexArray = NULL;// scratch space for stable radix sorting
int polygonArrayAllocSize = 65536;

FOutVector* unsortedVertexArray = NULL;// contains unsorted vertices and texture coordinates from DrawPolygon
int unsortedVertexArraySize = 0;
int unsortedVertexArrayAllocSize = 65536;

static void *HWR_ReallocArray(void *memory, size_t count, size_t elementsize,
	const char *description)
{
	void *resized;

	if (elementsize && count > (size_t)-1 / elementsize)
		I_Error("OpenGL batching %s allocation overflow", description);
	resized = realloc(memory, count * elementsize);
	if (!resized)
		I_Error("Out of memory growing OpenGL batching %s", description);
	return resized;
}

static int HWR_GrowBatchCapacity(int capacity, int required,
	const char *description)
{
	while (capacity < required)
	{
		if (capacity > INT32_MAX / 2)
			I_Error("OpenGL batching %s exceeded the supported map size", description);
		capacity *= 2;
	}
	return capacity;
}

// Enables batching mode. HWR_ProcessPolygon will collect polygons instead of passing them directly to the rendering backend.
// Call HWR_RenderBatches to render all the collected geometry.
void HWR_StartBatching(void)
{
	if (currently_batching)
		I_Error("Repeat call to HWR_StartBatching without HWR_RenderBatches");

	// init arrays if that has not been done yet
	if (!finalVertexIndexArray)
	{
		finalVertexIndexArray = HWR_ReallocArray(NULL,
			finalVertexIndexArrayAllocSize, sizeof(UINT32), "final indices");
		polygonArray = HWR_ReallocArray(NULL, polygonArrayAllocSize,
			sizeof(PolygonArrayEntry), "polygons");
		polygonIndexArray = HWR_ReallocArray(NULL, polygonArrayAllocSize,
			sizeof(UINT32), "polygon indices");
		polygonScratchIndexArray = HWR_ReallocArray(NULL, polygonArrayAllocSize,
			sizeof(UINT32), "polygon sort scratch indices");
		unsortedVertexArray = HWR_ReallocArray(NULL, unsortedVertexArrayAllocSize,
			sizeof(FOutVector), "unsorted vertices");
	}

	currently_batching = true;
}

// This replaces the direct calls to pfnSetTexture in cases where batching is available.
// The texture selection is saved for the next HWR_ProcessPolygon call.
// Doing this was easier than getting a texture pointer to HWR_ProcessPolygon.
void HWR_SetCurrentTexture(GLMipmap_t *texture)
{
    if (currently_batching)
    {
        current_texture = texture;
    }
    else
    {
        HWD.pfnSetTexture(texture);
    }
}

// If batching is enabled, this function collects the polygon data and the chosen texture
// for later use in HWR_RenderBatches. Otherwise the rendering backend is used to
// render the polygon immediately.
void HWR_ProcessPolygon(FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, int shader_target, boolean horizonSpecial)
{
    if (currently_batching)
	{
		if (!pSurf)
			I_Error("Got a null FSurfaceInfo in batching");// nulls should not come in the stuff that batching currently applies to
		if (polygonArraySize >= polygonArrayAllocSize)
		{
			polygonArrayAllocSize = HWR_GrowBatchCapacity(polygonArrayAllocSize,
				polygonArraySize + 1, "polygon list");
			// realloc can extend these in place, avoiding a second full-sized copy
			// at the largest point of an unusually open custom map.
			polygonArray = HWR_ReallocArray(polygonArray, polygonArrayAllocSize,
				sizeof(PolygonArrayEntry), "polygons");
			polygonIndexArray = HWR_ReallocArray(polygonIndexArray,
				polygonArrayAllocSize, sizeof(UINT32), "polygon indices");
			polygonScratchIndexArray = HWR_ReallocArray(polygonScratchIndexArray,
				polygonArrayAllocSize, sizeof(UINT32), "polygon sort scratch indices");
		}

		if ((UINT64)unsortedVertexArraySize + (UINT64)iNumPts > INT32_MAX)
			I_Error("OpenGL batching vertex count exceeded the supported map size");
		if (unsortedVertexArraySize + (int)iNumPts > unsortedVertexArrayAllocSize)
		{
			unsortedVertexArrayAllocSize = HWR_GrowBatchCapacity(
				unsortedVertexArrayAllocSize, unsortedVertexArraySize + (int)iNumPts,
				"unsorted vertex list");
			unsortedVertexArray = HWR_ReallocArray(unsortedVertexArray,
				unsortedVertexArrayAllocSize, sizeof(FOutVector), "unsorted vertices");
		}

		// add the polygon data to the arrays

		polygonArray[polygonArraySize].surf = *pSurf;
		polygonArray[polygonArraySize].vertsIndex = unsortedVertexArraySize;
		polygonArray[polygonArraySize].numVerts = iNumPts;
		polygonArray[polygonArraySize].polyFlags = PolyFlags;
		polygonArray[polygonArraySize].texture = current_texture;
		polygonArray[polygonArraySize].shader = (shader_target != SHADER_NONE) ? HWR_GetShaderFromTarget(shader_target) : shader_target;
		polygonArray[polygonArraySize].horizonSpecial = horizonSpecial;
		// Put skybox and horizon polygons first while preserving their order.
		polygonArray[polygonArraySize].hash = INT32_MIN + polygonArraySize;
		polygonArraySize++;

		if (!(PolyFlags & PF_NoTexture) && !horizonSpecial)
		{
			// use FNV-1a to hash polygons for later sorting.
			INT32 hash = 0x811c9dc5;
#define DIGEST(h, x) h ^= (x); h *= 0x01000193
			if (current_texture)
			{
				DIGEST(hash, current_texture->downloaded);
			}
			DIGEST(hash, PolyFlags);
			DIGEST(hash, pSurf->PolyColor.rgba);
			if (cv_glshaders.value && gl_shadersavailable)
			{
				DIGEST(hash, shader_target);
				DIGEST(hash, pSurf->TintColor.rgba);
				DIGEST(hash, pSurf->FadeColor.rgba);
				DIGEST(hash, pSurf->LightInfo.light_level);
				DIGEST(hash, pSurf->LightInfo.fade_start);
				DIGEST(hash, pSurf->LightInfo.fade_end);
			}
#undef DIGEST
			// remove the sign bit to ensure that skybox and horizon line comes first.
			polygonArray[polygonArraySize-1].hash = (hash & INT32_MAX);
		}

		memcpy(&unsortedVertexArray[unsortedVertexArraySize], pOutVerts, iNumPts * sizeof(FOutVector));
		unsortedVertexArraySize += iNumPts;
	}
	else
	{
		HWD.pfnSetShader((shader_target != SHADER_NONE) ? HWR_GetShaderFromTarget(shader_target) : shader_target);
		HWD.pfnDrawPolygon(pSurf, pOutVerts, iNumPts, PolyFlags);
	}
}

static int comparePolygons(const void *p1, const void *p2)
{
	unsigned int index1 = *(const unsigned int*)p1;
	unsigned int index2 = *(const unsigned int*)p2;
	PolygonArrayEntry* poly1 = &polygonArray[index1];
	PolygonArrayEntry* poly2 = &polygonArray[index2];
	// Compare the sign explicitly so subtracting opposite-signed hashes cannot
	// overflow and reorder horizon polygons behind ordinary geometry.
	if ((poly1->hash & 0x80000000) != (poly2->hash & 0x80000000))
		return (poly1->hash & 0x80000000) < (poly2->hash & 0x80000000) ? 1 : -1;
	return poly1->hash - poly2->hash;
}

static boolean HWR_PolygonIndicesAlreadySorted(void)
{
	int i;
	INT32 previous;

	if (polygonArraySize < 2)
		return true;
	previous = polygonArray[0].hash;
	for (i = 1; i < polygonArraySize; i++)
	{
		const INT32 current = polygonArray[i].hash;
		if (current < previous)
			return false;
		previous = current;
	}
	return true;
}

/* qsort spends a surprising amount of time repeatedly dereferencing polygon
 * state on large/open maps. Four stable byte passes sort the same signed
 * 32-bit hash in linear time and preserve the required horizon ordering. */
static void HWR_RadixSortPolygonIndices(void)
{
	UINT32 *source = polygonIndexArray;
	UINT32 *destination = polygonScratchIndexArray;
	unsigned int pass;

	for (pass = 0; pass < 4; pass++)
	{
		size_t counts[256] = {0};
		size_t offsets[256];
		const unsigned int shift = pass * 8;
		int i;

		for (i = 0; i < polygonArraySize; i++)
		{
			const UINT32 key = ((UINT32)polygonArray[source[i]].hash) ^ 0x80000000u;
			counts[(key >> shift) & 0xffu]++;
		}

		offsets[0] = 0;
		for (i = 1; i < 256; i++)
			offsets[i] = offsets[i - 1] + counts[i - 1];

		for (i = 0; i < polygonArraySize; i++)
		{
			const UINT32 index = source[i];
			const UINT32 key = ((UINT32)polygonArray[index].hash) ^ 0x80000000u;
			destination[offsets[(key >> shift) & 0xffu]++] = index;
		}

		{
			UINT32 *swap = source;
			source = destination;
			destination = swap;
		}
	}

	/* Four passes normally finish in polygonIndexArray. Keep this guard so the
	 * routine remains correct if its pass count is changed later. */
	if (source != polygonIndexArray)
		memcpy(polygonIndexArray, source, polygonArraySize * sizeof(UINT32));
}

// This function organizes the geometry collected by HWR_ProcessPolygon calls into batches and uses
// the rendering backend to draw them.
void HWR_RenderBatches(void)
{
	int finalIndexWritePos = 0;// position in finalVertexIndexArray

	int polygonReadPos = 0;// position in polygonIndexArray

	int currentShader;
	int nextShader = 0;
	GLMipmap_t *currentTexture;
	GLMipmap_t *nextTexture = NULL;
	FBITFIELD currentPolyFlags = 0;
	FBITFIELD nextPolyFlags = 0;
	FSurfaceInfo currentSurfaceInfo;
	FSurfaceInfo nextSurfaceInfo;

	int i;
	precise_t phase_started;

    if (!currently_batching)
		I_Error("HWR_RenderBatches called without starting batching");

	nextSurfaceInfo.LightInfo.fade_end = 0;
	nextSurfaceInfo.LightInfo.fade_start = 0;
	nextSurfaceInfo.LightInfo.light_level = 0;

	currently_batching = false;// no longer collecting batches
	if (!polygonArraySize)
		return;// nothing to draw
	// init stats vars
	ps_hw_numpolys.value.i += polygonArraySize;
	ps_hw_numshaders.value.i++;
	ps_hw_numtextures.value.i++;
	ps_hw_numpolyflags.value.i++;
	ps_hw_numcolors.value.i++;
	// init polygonIndexArray
	for (i = 0; i < polygonArraySize; i++)
	{
		polygonIndexArray[i] = i;
	}

	// sort polygons
	phase_started = I_GetPreciseTime();
	/* A surprising number of simple maps already submit geometry in batch-key
	 * order. Avoid touching the scratch array at all in that common case. */
	if (!HWR_PolygonIndicesAlreadySorted())
	{
		if (polygonArraySize >= 256)
			HWR_RadixSortPolygonIndices();
		else
			qsort(polygonIndexArray, polygonArraySize, sizeof(unsigned int), comparePolygons);
	}
	ps_hw_batchsorttime.value.p += I_GetPreciseTime() - phase_started;
	// sort order
	// 1. shader
	// 2. texture
	// 3. polyflags
	// 4. colors + light level
	// not sure about what order of the last 2 should be, or if it even matters

	phase_started = I_GetPreciseTime();

	currentShader = polygonArray[polygonIndexArray[0]].shader;
	currentTexture = polygonArray[polygonIndexArray[0]].texture;
	currentPolyFlags = polygonArray[polygonIndexArray[0]].polyFlags;
	currentSurfaceInfo = polygonArray[polygonIndexArray[0]].surf;
	// For now, will sort and track the colors. Vertex attributes could be used instead of uniforms
	// and a color array could replace the color calls.

	// set state for first batch

	if (cv_glshaders.value && gl_shadersavailable)
	{
		HWD.pfnSetShader(currentShader);
	}

	if (currentPolyFlags & PF_NoTexture)
		currentTexture = NULL;
    else
	    HWD.pfnSetTexture(currentTexture);

	while (1)// note: remember handling notexture polyflag as having texture number 0 (also in comparePolygons)
	{
		int firstIndex;
		int lastIndex;

		boolean stopFlag = false;
		boolean changeState = false;
		boolean changeShader = false;
		boolean changeTexture = false;
		boolean changePolyFlags = false;
		boolean changeSurfaceInfo = false;

		// steps:
		// write vertices
		// check for changes or end, otherwise go back to writing
			// changes will affect the next vars and the change bools
			// end could set flag for stopping
		// execute draw call
		// could check ending flag here
		// change states according to next vars and change bools, updating the current vars and reseting the bools
		// reset write pos
		// repeat loop

		int index = polygonIndexArray[polygonReadPos++];
		int numVerts = polygonArray[index].numVerts;
		UINT64 polygonIndexCount;
		UINT64 requiredIndexCount;

		if (numVerts < 0)
			I_Error("OpenGL batching final vertex count overflow");
		polygonIndexCount = numVerts >= 3 ? (UINT64)(numVerts - 2) * 3 : 0;
		requiredIndexCount = (UINT64)finalIndexWritePos + polygonIndexCount;
		if (requiredIndexCount > INT32_MAX)
			I_Error("OpenGL batching final index count overflow");
		if ((int)requiredIndexCount > finalVertexIndexArrayAllocSize)
		{
			finalVertexIndexArrayAllocSize = HWR_GrowBatchCapacity(
				finalVertexIndexArrayAllocSize, (int)requiredIndexCount,
				"final index list");
			finalVertexIndexArray = HWR_ReallocArray(finalVertexIndexArray,
				finalVertexIndexArrayAllocSize, sizeof(UINT32), "final indices");
		}
		/* Vertices were already copied into unsortedVertexArray while the BSP was
		 * collected. Index that stable array directly instead of copying every
		 * visible polygon a second time after sorting. This cuts a full geometry
		 * memory pass on large and open maps without changing any coordinates. */
		firstIndex = polygonArray[index].vertsIndex;
		lastIndex = firstIndex + numVerts;
		firstIndex += 2;
		while (firstIndex < lastIndex)
		{
			finalVertexIndexArray[finalIndexWritePos++] = polygonArray[index].vertsIndex;
			finalVertexIndexArray[finalIndexWritePos++] = firstIndex - 1;
			finalVertexIndexArray[finalIndexWritePos++] = firstIndex++;
		}

		if (polygonReadPos >= polygonArraySize)
		{
			stopFlag = true;
		}
		else
		{
			// check if a state change is required, set the change bools and next vars
			int nextIndex = polygonIndexArray[polygonReadPos];
			if (polygonArray[index].hash != polygonArray[nextIndex].hash)
			{
				nextShader = polygonArray[nextIndex].shader;
				nextTexture = polygonArray[nextIndex].texture;
				nextPolyFlags = polygonArray[nextIndex].polyFlags;
				nextSurfaceInfo = polygonArray[nextIndex].surf;
				if (nextPolyFlags & PF_NoTexture)
					nextTexture = 0;
				if (currentShader != nextShader && cv_glshaders.value && gl_shadersavailable)
				{
					changeState = true;
					changeShader = true;
				}
				if (currentTexture != nextTexture)
				{
					changeState = true;
					changeTexture = true;
				}
				if (currentPolyFlags != nextPolyFlags)
				{
					changeState = true;
					changePolyFlags = true;
				}
				if (cv_glshaders.value && gl_shadersavailable)
				{
					if (currentSurfaceInfo.PolyColor.rgba != nextSurfaceInfo.PolyColor.rgba ||
						currentSurfaceInfo.TintColor.rgba != nextSurfaceInfo.TintColor.rgba ||
						currentSurfaceInfo.FadeColor.rgba != nextSurfaceInfo.FadeColor.rgba ||
						currentSurfaceInfo.LightInfo.light_level != nextSurfaceInfo.LightInfo.light_level ||
						currentSurfaceInfo.LightInfo.fade_start != nextSurfaceInfo.LightInfo.fade_start ||
						currentSurfaceInfo.LightInfo.fade_end != nextSurfaceInfo.LightInfo.fade_end)
					{
						changeState = true;
						changeSurfaceInfo = true;
					}
				}
				else
				{
					if (currentSurfaceInfo.PolyColor.rgba != nextSurfaceInfo.PolyColor.rgba)
					{
						changeState = true;
						changeSurfaceInfo = true;
					}
				}
			}
		}

		if (changeState || stopFlag)
		{
			// execute draw call
			HWD.pfnDrawIndexedTriangles(&currentSurfaceInfo, unsortedVertexArray, finalIndexWritePos, currentPolyFlags, finalVertexIndexArray);
			// update stats
			ps_hw_numcalls.value.i++;
			ps_hw_numverts.value.i += finalIndexWritePos;
			// reset write positions
			finalIndexWritePos = 0;
		}
		else continue;

		// if we're here then either its time to stop or time to change state
		if (stopFlag) break;

		// change state according to change bools and next vars, update current vars and reset bools
		if (changeState)
		{
			if (changeShader)
			{
				HWD.pfnSetShader(nextShader);
				currentShader = nextShader;
				changeShader = false;

				ps_hw_numshaders.value.i++;
			}
			if (changeTexture)
			{
				// texture should be already ready for use from calls to SetTexture during batch collection
			    HWD.pfnSetTexture(nextTexture);
				currentTexture = nextTexture;
				changeTexture = false;

				ps_hw_numtextures.value.i++;
			}
			if (changePolyFlags)
			{
				currentPolyFlags = nextPolyFlags;
				changePolyFlags = false;

				ps_hw_numpolyflags.value.i++;
			}
			if (changeSurfaceInfo)
			{
				currentSurfaceInfo = nextSurfaceInfo;
				changeSurfaceInfo = false;

				ps_hw_numcolors.value.i++;
			}
		}
		// and that should be it?
	}
	// reset the arrays (set sizes to 0)
	polygonArraySize = 0;
	unsortedVertexArraySize = 0;

	ps_hw_batchdrawtime.value.p += I_GetPreciseTime() - phase_started;
}


#endif // HWRENDER
