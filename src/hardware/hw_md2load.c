/*
	From the 'Wizard2' engine by Spaddlewit Inc. ( http://www.spaddlewit.com )
	An experimental work-in-progress.

	Donated to Sonic Team Junior and adapted to work with
	Sonic Robo Blast 2. The license of this code matches whatever
	the licensing is for Sonic Robo Blast 2.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../doomdef.h"
#include "hw_md2load.h"
#include "hw_model.h"
#include "../z_zone.h"

#define NUMVERTEXNORMALS 162



float avertexnormals[NUMVERTEXNORMALS][3] = {
{-0.525731f, 0.000000f, 0.850651f},
{-0.442863f, 0.238856f, 0.864188f},
{-0.295242f, 0.000000f, 0.955423f},
{-0.309017f, 0.500000f, 0.809017f},
{-0.162460f, 0.262866f, 0.951056f},
{0.000000f, 0.000000f, 1.000000f},
{0.000000f, 0.850651f, 0.525731f},
{-0.147621f, 0.716567f, 0.681718f},
{0.147621f, 0.716567f, 0.681718f},
{0.000000f, 0.525731f, 0.850651f},
{0.309017f, 0.500000f, 0.809017f},
{0.525731f, 0.000000f, 0.850651f},
{0.295242f, 0.000000f, 0.955423f},
{0.442863f, 0.238856f, 0.864188f},
{0.162460f, 0.262866f, 0.951056f},
{-0.681718f, 0.147621f, 0.716567f},
{-0.809017f, 0.309017f, 0.500000f},
{-0.587785f, 0.425325f, 0.688191f},
{-0.850651f, 0.525731f, 0.000000f},
{-0.864188f, 0.442863f, 0.238856f},
{-0.716567f, 0.681718f, 0.147621f},
{-0.688191f, 0.587785f, 0.425325f},
{-0.500000f, 0.809017f, 0.309017f},
{-0.238856f, 0.864188f, 0.442863f},
{-0.425325f, 0.688191f, 0.587785f},
{-0.716567f, 0.681718f, -0.147621f},
{-0.500000f, 0.809017f, -0.309017f},
{-0.525731f, 0.850651f, 0.000000f},
{0.000000f, 0.850651f, -0.525731f},
{-0.238856f, 0.864188f, -0.442863f},
{0.000000f, 0.955423f, -0.295242f},
{-0.262866f, 0.951056f, -0.162460f},
{0.000000f, 1.000000f, 0.000000f},
{0.000000f, 0.955423f, 0.295242f},
{-0.262866f, 0.951056f, 0.162460f},
{0.238856f, 0.864188f, 0.442863f},
{0.262866f, 0.951056f, 0.162460f},
{0.500000f, 0.809017f, 0.309017f},
{0.238856f, 0.864188f, -0.442863f},
{0.262866f, 0.951056f, -0.162460f},
{0.500000f, 0.809017f, -0.309017f},
{0.850651f, 0.525731f, 0.000000f},
{0.716567f, 0.681718f, 0.147621f},
{0.716567f, 0.681718f, -0.147621f},
{0.525731f, 0.850651f, 0.000000f},
{0.425325f, 0.688191f, 0.587785f},
{0.864188f, 0.442863f, 0.238856f},
{0.688191f, 0.587785f, 0.425325f},
{0.809017f, 0.309017f, 0.500000f},
{0.681718f, 0.147621f, 0.716567f},
{0.587785f, 0.425325f, 0.688191f},
{0.955423f, 0.295242f, 0.000000f},
{1.000000f, 0.000000f, 0.000000f},
{0.951056f, 0.162460f, 0.262866f},
{0.850651f, -0.525731f, 0.000000f},
{0.955423f, -0.295242f, 0.000000f},
{0.864188f, -0.442863f, 0.238856f},
{0.951056f, -0.162460f, 0.262866f},
{0.809017f, -0.309017f, 0.500000f},
{0.681718f, -0.147621f, 0.716567f},
{0.850651f, 0.000000f, 0.525731f},
{0.864188f, 0.442863f, -0.238856f},
{0.809017f, 0.309017f, -0.500000f},
{0.951056f, 0.162460f, -0.262866f},
{0.525731f, 0.000000f, -0.850651f},
{0.681718f, 0.147621f, -0.716567f},
{0.681718f, -0.147621f, -0.716567f},
{0.850651f, 0.000000f, -0.525731f},
{0.809017f, -0.309017f, -0.500000f},
{0.864188f, -0.442863f, -0.238856f},
{0.951056f, -0.162460f, -0.262866f},
{0.147621f, 0.716567f, -0.681718f},
{0.309017f, 0.500000f, -0.809017f},
{0.425325f, 0.688191f, -0.587785f},
{0.442863f, 0.238856f, -0.864188f},
{0.587785f, 0.425325f, -0.688191f},
{0.688191f, 0.587785f, -0.425325f},
{-0.147621f, 0.716567f, -0.681718f},
{-0.309017f, 0.500000f, -0.809017f},
{0.000000f, 0.525731f, -0.850651f},
{-0.525731f, 0.000000f, -0.850651f},
{-0.442863f, 0.238856f, -0.864188f},
{-0.295242f, 0.000000f, -0.955423f},
{-0.162460f, 0.262866f, -0.951056f},
{0.000000f, 0.000000f, -1.000000f},
{0.295242f, 0.000000f, -0.955423f},
{0.162460f, 0.262866f, -0.951056f},
{-0.442863f, -0.238856f, -0.864188f},
{-0.309017f, -0.500000f, -0.809017f},
{-0.162460f, -0.262866f, -0.951056f},
{0.000000f, -0.850651f, -0.525731f},
{-0.147621f, -0.716567f, -0.681718f},
{0.147621f, -0.716567f, -0.681718f},
{0.000000f, -0.525731f, -0.850651f},
{0.309017f, -0.500000f, -0.809017f},
{0.442863f, -0.238856f, -0.864188f},
{0.162460f, -0.262866f, -0.951056f},
{0.238856f, -0.864188f, -0.442863f},
{0.500000f, -0.809017f, -0.309017f},
{0.425325f, -0.688191f, -0.587785f},
{0.716567f, -0.681718f, -0.147621f},
{0.688191f, -0.587785f, -0.425325f},
{0.587785f, -0.425325f, -0.688191f},
{0.000000f, -0.955423f, -0.295242f},
{0.000000f, -1.000000f, 0.000000f},
{0.262866f, -0.951056f, -0.162460f},
{0.000000f, -0.850651f, 0.525731f},
{0.000000f, -0.955423f, 0.295242f},
{0.238856f, -0.864188f, 0.442863f},
{0.262866f, -0.951056f, 0.162460f},
{0.500000f, -0.809017f, 0.309017f},
{0.716567f, -0.681718f, 0.147621f},
{0.525731f, -0.850651f, 0.000000f},
{-0.238856f, -0.864188f, -0.442863f},
{-0.500000f, -0.809017f, -0.309017f},
{-0.262866f, -0.951056f, -0.162460f},
{-0.850651f, -0.525731f, 0.000000f},
{-0.716567f, -0.681718f, -0.147621f},
{-0.716567f, -0.681718f, 0.147621f},
{-0.525731f, -0.850651f, 0.000000f},
{-0.500000f, -0.809017f, 0.309017f},
{-0.238856f, -0.864188f, 0.442863f},
{-0.262866f, -0.951056f, 0.162460f},
{-0.864188f, -0.442863f, 0.238856f},
{-0.809017f, -0.309017f, 0.500000f},
{-0.688191f, -0.587785f, 0.425325f},
{-0.681718f, -0.147621f, 0.716567f},
{-0.442863f, -0.238856f, 0.864188f},
{-0.587785f, -0.425325f, 0.688191f},
{-0.309017f, -0.500000f, 0.809017f},
{-0.147621f, -0.716567f, 0.681718f},
{-0.425325f, -0.688191f, 0.587785f},
{-0.162460f, -0.262866f, 0.951056f},
{0.442863f, -0.238856f, 0.864188f},
{0.162460f, -0.262866f, 0.951056f},
{0.309017f, -0.500000f, 0.809017f},
{0.147621f, -0.716567f, 0.681718f},
{0.000000f, -0.525731f, 0.850651f},
{0.425325f, -0.688191f, 0.587785f},
{0.587785f, -0.425325f, 0.688191f},
{0.688191f, -0.587785f, 0.425325f},
{-0.955423f, 0.295242f, 0.000000f},
{-0.951056f, 0.162460f, 0.262866f},
{-1.000000f, 0.000000f, 0.000000f},
{-0.850651f, 0.000000f, 0.525731f},
{-0.955423f, -0.295242f, 0.000000f},
{-0.951056f, -0.162460f, 0.262866f},
{-0.864188f, 0.442863f, -0.238856f},
{-0.951056f, 0.162460f, -0.262866f},
{-0.809017f, 0.309017f, -0.500000f},
{-0.864188f, -0.442863f, -0.238856f},
{-0.951056f, -0.162460f, -0.262866f},
{-0.809017f, -0.309017f, -0.500000f},
{-0.681718f, 0.147621f, -0.716567f},
{-0.681718f, -0.147621f, -0.716567f},
{-0.850651f, 0.000000f, -0.525731f},
{-0.688191f, 0.587785f, -0.425325f},
{-0.587785f, 0.425325f, -0.688191f},
{-0.425325f, 0.688191f, -0.587785f},
{-0.425325f, -0.688191f, -0.587785f},
{-0.587785f, -0.425325f, -0.688191f},
{-0.688191f, -0.587785f, -0.425325f},
};

typedef struct
{
	int ident;         
	int version;       
	int skinwidth;     
	int skinheight;    
	int framesize;     
	int numSkins;      
	int numXYZ;        
	int numST;         
	int numTris;       
	int numGLcmds;     
	int numFrames;     
	int offsetSkins;   
	int offsetST;      
	int offsetTris;    
	int offsetFrames;  
	int offsetGLcmds;  
	int offsetEnd;     
} md2header_t;

typedef struct
{
	unsigned short meshIndex[3];  
	unsigned short stIndex[3];    
} md2triangle_t;

typedef struct
{
	short s;
	short t;
} md2texcoord_t;

typedef struct
{
	unsigned char v[3];              
	unsigned char lightNormalIndex;  
} md2vertex_t;

typedef struct
{
	float scale[3];       
	float translate[3];   
	char name[16];        
} md2frame_t;


model_t *MD2_LoadModel(const char *fileName, int ztag, boolean useFloat)
{
	FILE *f;

	model_t *retModel = NULL;
	md2header_t *header;

	size_t fileLen;
	int i, j;
	size_t namelen;
	char *texturefilename;
	const char *texPos;

	char *buffer;

	const float WUNITS = 1.0f;
	float dataScale = WUNITS;

	md2triangle_t *tris;
	md2texcoord_t *texcoords;
	md2frame_t *frames;
	char *fname = NULL;
	int foffset = 0;

	int t;













	useFloat = true;

	f = fopen(fileName, "rb");

	if (!f)
		return NULL;

	retModel = (model_t*)Z_Calloc(sizeof(model_t), ztag, 0);







	texPos = strchr(fileName, '/');

	if (texPos)
	{
		texPos++;
		namelen = strlen(texPos) + 1;
		texturefilename = (char*)Z_Malloc(namelen, PU_CACHE, 0);
		strcpy(texturefilename, texPos);
	}
	else
	{
		namelen = strlen(fileName) + 1;
		texturefilename = (char*)Z_Malloc(namelen, PU_CACHE, 0);
		strcpy(texturefilename, fileName);
	}

	texturefilename[namelen - 2] = 'z';
	texturefilename[namelen - 3] = 'u';
	texturefilename[namelen - 4] = 'b';


	fseek(f, 0, SEEK_END);
	fileLen = ftell(f);
	fseek(f, 0, SEEK_SET);


	buffer = malloc(fileLen);
	if (fread(buffer, fileLen, 1, f)) { }  
	fclose(f);


	header = (md2header_t*)buffer;

	retModel->numMeshes = 1;  
	retModel->meshes = (mesh_t*)Z_Calloc(sizeof(mesh_t) * retModel->numMeshes, ztag, 0);
	retModel->meshes[0].numFrames = header->numFrames;




	tris = (md2triangle_t*)&buffer[header->offsetTris];
	texcoords = (md2texcoord_t*)&buffer[header->offsetST];
	frames = (md2frame_t*)&buffer[header->offsetFrames];

	retModel->frameNames = (char*)Z_Calloc(header->numFrames*16, ztag, 0);
	fname = retModel->frameNames;
	for (i = 0; i < header->numFrames; i++)
	{
		md2frame_t *fr = (md2frame_t*)&buffer[header->offsetFrames + foffset];
		memcpy(fname, fr->name, 16);
		foffset += sizeof(md2frame_t) + (sizeof(md2vertex_t) * header->numXYZ);
		fname += 16;
	}


	retModel->numMaterials = header->numSkins;

	if (retModel->numMaterials <= 0)  
		retModel->numMaterials = 1;

	retModel->materials = (material_t*)Z_Calloc(sizeof(material_t)*retModel->numMaterials, ztag, 0);


	for (t = 0; t < retModel->numMaterials; t++)
	{
		retModel->materials[t].ambient[0] = 0.8f;
		retModel->materials[t].ambient[1] = 0.8f;
		retModel->materials[t].ambient[2] = 0.8f;
		retModel->materials[t].ambient[3] = 1.0f;
		retModel->materials[t].diffuse[0] = 0.8f;
		retModel->materials[t].diffuse[1] = 0.8f;
		retModel->materials[t].diffuse[2] = 0.8f;
		retModel->materials[t].diffuse[3] = 1.0f;
		retModel->materials[t].emissive[0] = 0.0f;
		retModel->materials[t].emissive[1] = 0.0f;
		retModel->materials[t].emissive[2] = 0.0f;
		retModel->materials[t].emissive[3] = 1.0f;
		retModel->materials[t].specular[0] = 0.0f;
		retModel->materials[t].specular[1] = 0.0f;
		retModel->materials[t].specular[2] = 0.0f;
		retModel->materials[t].specular[3] = 1.0f;
		retModel->materials[t].shininess = 0.0f;
		retModel->materials[t].spheremap = false;

		                                                                                              

                     
     
                                   
                             
                              
                                            
                                        
                                     
                
                
                
                
                  
                  
                  
                  
                  
                  
                   

                                                               
                                              
                                   
      
                                  
             

                                  
                             
      

                                                              
             
      
                           
                                                                                        
      
       
	}

	retModel->meshes[0].numTriangles = header->numTris;

	if (!useFloat)  
	{
		char *ptr;

		md2triangle_t *trisPtr;
		unsigned short *indexptr;
		float *uvptr;

		dataScale = 0.015624f;  
		retModel->meshes[0].tinyframes = (tinyframe_t*)Z_Calloc(sizeof(tinyframe_t)*header->numFrames, ztag, 0);
		retModel->meshes[0].numVertices = header->numXYZ;
		retModel->meshes[0].uvs = (float*)Z_Malloc(sizeof(float) * 2 * retModel->meshes[0].numVertices, ztag, 0);

		ptr = (char*)frames;
		for (i = 0; i < header->numFrames; i++, ptr += header->framesize)
		{
			short *vertptr;
			char *normptr;


			md2vertex_t *vertex;

			md2frame_t *framePtr = (md2frame_t*)ptr;
			retModel->meshes[0].tinyframes[i].vertices = (short*)Z_Malloc(sizeof(short) * 3 * header->numXYZ, ztag, 0);
			retModel->meshes[0].tinyframes[i].normals = (char*)Z_Malloc(sizeof(char) * 3 * header->numXYZ, ztag, 0);



			retModel->meshes[0].indices = (unsigned short*)Z_Malloc(sizeof(unsigned short) * 3 * header->numTris, ztag, 0);

			vertptr = retModel->meshes[0].tinyframes[i].vertices;
			normptr = retModel->meshes[0].tinyframes[i].normals;


			retModel->meshes[0].tinyframes[i].material = &retModel->materials[0];

			framePtr++;  
			vertex = (md2vertex_t*)framePtr;
			framePtr--;
			for (j = 0; j < header->numXYZ; j++, vertex++)
			{
				*vertptr = (short)(((vertex->v[0] * framePtr->scale[0]) + framePtr->translate[0]) / dataScale);
				vertptr++;
				*vertptr = (short)(((vertex->v[2] * framePtr->scale[2]) + framePtr->translate[2]) / dataScale);
				vertptr++;
				*vertptr = -1.0f * (short)(((vertex->v[1] * framePtr->scale[1]) + framePtr->translate[1]) / dataScale);
				vertptr++;


				*normptr++ = (char)(avertexnormals[vertex->lightNormalIndex][0] * 127);
				*normptr++ = (char)(avertexnormals[vertex->lightNormalIndex][2] * 127);
				*normptr++ = (char)(avertexnormals[vertex->lightNormalIndex][1] * 127);
			}
		}


		trisPtr = tris;
		indexptr = retModel->meshes[0].indices;
		uvptr = (float*)retModel->meshes[0].uvs;
		for (j = 0; j < header->numTris; j++, trisPtr++)
		{
			*indexptr = trisPtr->meshIndex[0];
			indexptr++;
			*indexptr = trisPtr->meshIndex[1];
			indexptr++;
			*indexptr = trisPtr->meshIndex[2];
			indexptr++;

			uvptr[trisPtr->meshIndex[0] * 2] = texcoords[trisPtr->stIndex[0]].s / (float)header->skinwidth;
			uvptr[trisPtr->meshIndex[0] * 2 + 1] = (texcoords[trisPtr->stIndex[0]].t / (float)header->skinheight);
			uvptr[trisPtr->meshIndex[1] * 2] = texcoords[trisPtr->stIndex[1]].s / (float)header->skinwidth;
			uvptr[trisPtr->meshIndex[1] * 2 + 1] = (texcoords[trisPtr->stIndex[1]].t / (float)header->skinheight);
			uvptr[trisPtr->meshIndex[2] * 2] = texcoords[trisPtr->stIndex[2]].s / (float)header->skinwidth;
			uvptr[trisPtr->meshIndex[2] * 2 + 1] = (texcoords[trisPtr->stIndex[2]].t / (float)header->skinheight);
		}
	}
	else  
	{
		md2triangle_t *trisPtr;
		float *uvptr;

		char *ptr;

		retModel->meshes[0].numVertices = header->numTris * 3;
		retModel->meshes[0].frames = (mdlframe_t*)Z_Calloc(sizeof(mdlframe_t)*header->numFrames, ztag, 0);
		retModel->meshes[0].uvs = (float*)Z_Malloc(sizeof(float) * 2 * retModel->meshes[0].numVertices, ztag, 0);

		trisPtr = tris;
		uvptr = retModel->meshes[0].uvs;
		for (i = 0; i < retModel->meshes[0].numTriangles; i++, trisPtr++)
		{
			*uvptr++ = texcoords[trisPtr->stIndex[0]].s / (float)header->skinwidth;
			*uvptr++ = (texcoords[trisPtr->stIndex[0]].t / (float)header->skinheight);
			*uvptr++ = texcoords[trisPtr->stIndex[1]].s / (float)header->skinwidth;
			*uvptr++ = (texcoords[trisPtr->stIndex[1]].t / (float)header->skinheight);
			*uvptr++ = texcoords[trisPtr->stIndex[2]].s / (float)header->skinwidth;
			*uvptr++ = (texcoords[trisPtr->stIndex[2]].t / (float)header->skinheight);
		}

		ptr = (char*)frames;
		for (i = 0; i < header->numFrames; i++, ptr += header->framesize)
		{
			float *vertptr, *normptr;

			md2vertex_t *vertex;

			md2frame_t *framePtr = (md2frame_t*)ptr;
			retModel->meshes[0].frames[i].normals = (float*)Z_Malloc(sizeof(float) * 3 * header->numTris * 3, ztag, 0);
			retModel->meshes[0].frames[i].vertices = (float*)Z_Malloc(sizeof(float) * 3 * header->numTris * 3, ztag, 0);



			normptr = (float*)retModel->meshes[0].frames[i].normals;
			vertptr = (float*)retModel->meshes[0].frames[i].vertices;
			trisPtr = tris;

			retModel->meshes[0].frames[i].material = &retModel->materials[0];

			framePtr++;  
			vertex = (md2vertex_t*)framePtr;
			framePtr--;
			for (j = 0; j < header->numTris; j++, trisPtr++)
			{
				*vertptr = ((vertex[trisPtr->meshIndex[0]].v[0] * framePtr->scale[0]) + framePtr->translate[0]) * WUNITS;
				vertptr++;
				*vertptr = ((vertex[trisPtr->meshIndex[0]].v[2] * framePtr->scale[2]) + framePtr->translate[2]) * WUNITS;
				vertptr++;
				*vertptr = -1.0f * ((vertex[trisPtr->meshIndex[0]].v[1] * framePtr->scale[1]) + framePtr->translate[1]) * WUNITS;
				vertptr++;

				*vertptr = ((vertex[trisPtr->meshIndex[1]].v[0] * framePtr->scale[0]) + framePtr->translate[0]) * WUNITS;
				vertptr++;
				*vertptr = ((vertex[trisPtr->meshIndex[1]].v[2] * framePtr->scale[2]) + framePtr->translate[2]) * WUNITS;
				vertptr++;
				*vertptr = -1.0f * ((vertex[trisPtr->meshIndex[1]].v[1] * framePtr->scale[1]) + framePtr->translate[1]) * WUNITS;
				vertptr++;

				*vertptr = ((vertex[trisPtr->meshIndex[2]].v[0] * framePtr->scale[0]) + framePtr->translate[0]) * WUNITS;
				vertptr++;
				*vertptr = ((vertex[trisPtr->meshIndex[2]].v[2] * framePtr->scale[2]) + framePtr->translate[2]) * WUNITS;
				vertptr++;
				*vertptr = -1.0f * ((vertex[trisPtr->meshIndex[2]].v[1] * framePtr->scale[1]) + framePtr->translate[1]) * WUNITS;
				vertptr++;

				*normptr++ = avertexnormals[vertex[trisPtr->meshIndex[0]].lightNormalIndex][0];
				*normptr++ = avertexnormals[vertex[trisPtr->meshIndex[0]].lightNormalIndex][2];
				*normptr++ = avertexnormals[vertex[trisPtr->meshIndex[0]].lightNormalIndex][1];

				*normptr++ = avertexnormals[vertex[trisPtr->meshIndex[1]].lightNormalIndex][0];
				*normptr++ = avertexnormals[vertex[trisPtr->meshIndex[1]].lightNormalIndex][2];
				*normptr++ = avertexnormals[vertex[trisPtr->meshIndex[1]].lightNormalIndex][1];

				*normptr++ = avertexnormals[vertex[trisPtr->meshIndex[2]].lightNormalIndex][0];
				*normptr++ = avertexnormals[vertex[trisPtr->meshIndex[2]].lightNormalIndex][2];
				*normptr++ = avertexnormals[vertex[trisPtr->meshIndex[2]].lightNormalIndex][1];
			}
		}
	}

	free(buffer);
	return retModel;
}
