// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifdef HWRENDER
#include "hw_glob.h"
#include "hw_batching.h"
#include "../i_system.h"



GLMipmap_t *current_texture = NULL;

boolean currently_batching = false;

UINT32* finalVertexIndexArray = NULL; 
int finalVertexIndexArrayAllocSize = 65536 * 3;





PolygonArrayEntry* polygonArray = NULL; 
int polygonArraySize = 0;
UINT32* polygonIndexArray = NULL; 
static UINT32* polygonScratchIndexArray = NULL; 
int polygonArrayAllocSize = 65536;

FOutVector* unsortedVertexArray = NULL; 
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



void HWR_StartBatching(void)
{
	if (currently_batching)
		I_Error("Repeat call to HWR_StartBatching without HWR_RenderBatches");


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




void HWR_ProcessPolygon(FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, int shader_target, boolean horizonSpecial)
{
    if (currently_batching)
	{
		if (!pSurf)
			I_Error("Got a null FSurfaceInfo in batching"); 
		if (polygonArraySize >= polygonArrayAllocSize)
		{
			polygonArrayAllocSize = HWR_GrowBatchCapacity(polygonArrayAllocSize,
				polygonArraySize + 1, "polygon list");


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



		polygonArray[polygonArraySize].surf = *pSurf;
		polygonArray[polygonArraySize].vertsIndex = unsortedVertexArraySize;
		polygonArray[polygonArraySize].numVerts = iNumPts;
		polygonArray[polygonArraySize].polyFlags = PolyFlags;
		polygonArray[polygonArraySize].texture = current_texture;
		polygonArray[polygonArraySize].shader = (shader_target != SHADER_NONE) ? HWR_GetShaderFromTarget(shader_target) : shader_target;
		polygonArray[polygonArraySize].horizonSpecial = horizonSpecial;

		polygonArray[polygonArraySize].hash = INT32_MIN + polygonArraySize;
		polygonArraySize++;

		if (!(PolyFlags & PF_NoTexture) && !horizonSpecial)
		{

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

	                                                                           
                                                                  
	if (source != polygonIndexArray)
		memcpy(polygonIndexArray, source, polygonArraySize * sizeof(UINT32));
}



void HWR_RenderBatches(void)
{
	int finalIndexWritePos = 0; 

	int polygonReadPos = 0; 

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

	currently_batching = false; 
	if (!polygonArraySize)
		return; 

	ps_hw_numpolys.value.i += polygonArraySize;
	ps_hw_numshaders.value.i++;
	ps_hw_numtextures.value.i++;
	ps_hw_numpolyflags.value.i++;
	ps_hw_numcolors.value.i++;

	for (i = 0; i < polygonArraySize; i++)
	{
		polygonIndexArray[i] = i;
	}


	phase_started = I_GetPreciseTime();
	                                                                          
                                                                          
	if (!HWR_PolygonIndicesAlreadySorted())
	{
		if (polygonArraySize >= 256)
			HWR_RadixSortPolygonIndices();
		else
			qsort(polygonIndexArray, polygonArraySize, sizeof(unsigned int), comparePolygons);
	}
	ps_hw_batchsorttime.value.p += I_GetPreciseTime() - phase_started;







	phase_started = I_GetPreciseTime();

	currentShader = polygonArray[polygonIndexArray[0]].shader;
	currentTexture = polygonArray[polygonIndexArray[0]].texture;
	currentPolyFlags = polygonArray[polygonIndexArray[0]].polyFlags;
	currentSurfaceInfo = polygonArray[polygonIndexArray[0]].surf;





	if (cv_glshaders.value && gl_shadersavailable)
	{
		HWD.pfnSetShader(currentShader);
	}

	if (currentPolyFlags & PF_NoTexture)
		currentTexture = NULL;
    else
	    HWD.pfnSetTexture(currentTexture);

	while (1) 
	{
		int firstIndex;
		int lastIndex;

		boolean stopFlag = false;
		boolean changeState = false;
		boolean changeShader = false;
		boolean changeTexture = false;
		boolean changePolyFlags = false;
		boolean changeSurfaceInfo = false;












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

			HWD.pfnDrawIndexedTriangles(&currentSurfaceInfo, unsortedVertexArray, finalIndexWritePos, currentPolyFlags, finalVertexIndexArray);

			ps_hw_numcalls.value.i++;
			ps_hw_numverts.value.i += finalIndexWritePos;

			finalIndexWritePos = 0;
		}
		else continue;


		if (stopFlag) break;


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

	}

	polygonArraySize = 0;
	unsortedVertexArraySize = 0;

	ps_hw_batchdrawtime.value.p += I_GetPreciseTime() - phase_started;
}


#endif  
