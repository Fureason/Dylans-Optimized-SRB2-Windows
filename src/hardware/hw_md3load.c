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
#include "hw_md3load.h"
#include "hw_model.h"
#include "../z_zone.h"

typedef struct
{
	int ident;			 
	int version;		 
	char name[64];
	int flags;
	int numFrames;		 
	int numTags;
	int numSurfaces;
	int numSkins;		 
	int offsetFrames;
	int offsetTags;
	int offsetSurfaces;
	int offsetEnd;		 
} md3modelHeader;

typedef struct
{
	float minBounds[3];		 
	float maxBounds[3];		 
	float localOrigin[3];	 
	float radius;			 
	char name[16];			 
} md3Frame;

typedef struct
{
	char name[64];		 
	float origin[3];	 
	float axis[9];		 
} md3Tag;

typedef struct
{
	int ident;
	char name[64];			 
	int flags;
	int numFrames;			 
	int numShaders;			 
	int numVerts;			 
	int numTriangles;		 
	int offsetTriangles;	 
	int offsetShaders;		 
	int offsetST;			 
	int offsetXYZNormal;	 
	int offsetEnd;			 
} md3Surface;

typedef struct
{
	char name[64];  
	int shaderIndex;  
} md3Shader;

typedef struct
{
	int index[3];  
} md3Triangle;

typedef struct
{
	float st[2];
} md3TexCoord;

typedef struct
{
	short x, y, z, n;
} md3Vertex;

static float latlnglookup[256][256][3];

static void GetNormalFromLatLong(short latlng, float *out)
{
	float *lookup = latlnglookup[(unsigned char)(latlng >> 8)][(unsigned char)(latlng & 255)];

	out[0] = *lookup++;
	out[1] = *lookup++;
	out[2] = *lookup++;
}

#if 0
static void NormalToLatLng(float *n, short *out)
{

	if (0.0f == n[0] && 0.0f == n[1])
	{
		if (n[2] > 0.0f)
			*out = 0;
		else
			*out = 128;
	}
	else
	{
		char x, y;

		x = (char)(57.2957795f * (atan2(n[1], n[0])) * (255.0f / 360.0f));
		y = (char)(57.2957795f * (acos(n[2])) * (255.0f / 360.0f));

		*out = (x << 8) + y;
	}
}
#endif

static inline void LatLngToNormal(short n, float *out)
{
	const float PI = (3.1415926535897932384626433832795f);
	float lat = (float)(n >> 8);
	float lng = (float)(n & 255);

	lat *= PI / 128.0f;
	lng *= PI / 128.0f;

	out[0] = cosf(lat) * sinf(lng);
	out[1] = sinf(lat) * sinf(lng);
	out[2] = cosf(lng);
}

static void LatLngInit(void)
{
	int i, j;
	for (i = 0; i < 256; i++)
	{
		for (j = 0; j < 256; j++)
			LatLngToNormal((short)((i << 8) + j), latlnglookup[i][j]);
	}
}

static boolean latlnginit = false;

model_t *MD3_LoadModel(const char *fileName, int ztag, boolean useFloat)
{
	const float WUNITS = 1.0f;
	model_t *retModel = NULL;
	md3Frame *frames = NULL;
	char *fname = NULL;
	md3modelHeader *mdh;
	long fileLen;
	long fileReadLen;
	char *buffer;
	int surfEnd;
	int i, t;
	int matCount;
	FILE *f;

	if (!latlnginit)
	{
		LatLngInit();
		latlnginit = true;
	}

	f = fopen(fileName, "rb");

	if (!f)
		return NULL;

	retModel = (model_t*)Z_Calloc(sizeof(model_t), ztag, 0);


	fseek(f, 0, SEEK_END);
	fileLen = ftell(f);
	fseek(f, 0, SEEK_SET);


	buffer = malloc(fileLen);
	fileReadLen = fread(buffer, fileLen, 1, f);
	fclose(f);

	(void)fileReadLen;  


	mdh = (md3modelHeader*)buffer;

	retModel->numMeshes = mdh->numSurfaces;

	retModel->numMaterials = 0;
	surfEnd = 0;
	for (i = 0; i < mdh->numSurfaces; i++)
	{
		md3Surface *mdS = (md3Surface*)&buffer[mdh->offsetSurfaces];
		surfEnd += mdS->offsetEnd;

		retModel->numMaterials += mdS->numShaders;
	}


	if (retModel->numMaterials <= 0)  
		retModel->numMaterials = 1;

	retModel->materials = (material_t*)Z_Calloc(sizeof(material_t)*retModel->numMaterials, ztag, 0);

	for (t = 0; t < retModel->numMaterials; t++)
	{
		retModel->materials[t].ambient[0] = 0.3686f;
		retModel->materials[t].ambient[1] = 0.3684f;
		retModel->materials[t].ambient[2] = 0.3684f;
		retModel->materials[t].ambient[3] = 1.0f;
		retModel->materials[t].diffuse[0] = 0.8863f;
		retModel->materials[t].diffuse[1] = 0.8850f;
		retModel->materials[t].diffuse[2] = 0.8850f;
		retModel->materials[t].diffuse[3] = 1.0f;
		retModel->materials[t].emissive[0] = 0.0f;
		retModel->materials[t].emissive[1] = 0.0f;
		retModel->materials[t].emissive[2] = 0.0f;
		retModel->materials[t].emissive[3] = 1.0f;
		retModel->materials[t].specular[0] = 0.4902f;
		retModel->materials[t].specular[1] = 0.4887f;
		retModel->materials[t].specular[2] = 0.4887f;
		retModel->materials[t].specular[3] = 1.0f;
		retModel->materials[t].shininess = 25.0f;
		retModel->materials[t].spheremap = false;
	}

	retModel->meshes = (mesh_t*)Z_Calloc(sizeof(mesh_t)*retModel->numMeshes, ztag, 0);

	frames = (md3Frame*)&buffer[mdh->offsetFrames];
	retModel->frameNames = (char*)Z_Calloc(mdh->numFrames*16, ztag, 0);
	fname = retModel->frameNames;
	for (i = 0; i < mdh->numFrames; i++)
	{
		memcpy(fname, frames->name, 16);
		fname += 16;
		frames++;
	}

	matCount = 0;
	for (i = 0, surfEnd = 0; i < mdh->numSurfaces; i++)
	{
		int j;
		md3Shader *mdShader;
		md3Surface *mdS = (md3Surface*)&buffer[mdh->offsetSurfaces + surfEnd];
		surfEnd += mdS->offsetEnd;

		mdShader = (md3Shader*)((char*)mdS + mdS->offsetShaders);

		for (j = 0; j < mdS->numShaders; j++, matCount++)
		{
			size_t len = strlen(mdShader[j].name);
			mdShader[j].name[len-1] = 'z';
			mdShader[j].name[len-2] = 'u';
			mdShader[j].name[len-3] = 'b';


                                                                                                

                    
    
                                  
                            
                             
                                            
                                
                                    
               
               
               
               
                 
                 
                 
                 
                 
                 
                  

                                                              
                                             
                                  
     
                                 
            

                                 
                            
     

                                                             
            
     
                          
                                                                                              
     
      
		}

		retModel->meshes[i].numFrames = mdS->numFrames;
		retModel->meshes[i].numTriangles = mdS->numTriangles;

		if (!useFloat)  
		{
			float tempNormal[3];
			float *uvptr;
			md3TexCoord *mdST;
			unsigned short *indexptr;
			md3Triangle *mdT;

			retModel->meshes[i].tinyframes = (tinyframe_t*)Z_Calloc(sizeof(tinyframe_t)*mdS->numFrames, ztag, 0);
			retModel->meshes[i].numVertices = mdS->numVerts;
			retModel->meshes[i].uvs = (float*)Z_Malloc(sizeof(float)*2*mdS->numVerts, ztag, 0);
			for (j = 0; j < mdS->numFrames; j++)
			{
				short *vertptr;
				char *normptr;

				int k;
				md3Vertex *mdV = (md3Vertex*)((char*)mdS + mdS->offsetXYZNormal + (mdS->numVerts*j*sizeof(md3Vertex)));
				retModel->meshes[i].tinyframes[j].vertices = (short*)Z_Malloc(sizeof(short)*3*mdS->numVerts, ztag, 0);
				retModel->meshes[i].tinyframes[j].normals = (char*)Z_Malloc(sizeof(char)*3*mdS->numVerts, ztag, 0);



				retModel->meshes[i].indices = (unsigned short*)Z_Malloc(sizeof(unsigned short) * 3 * mdS->numTriangles, ztag, 0);
				vertptr = retModel->meshes[i].tinyframes[j].vertices;
				normptr = retModel->meshes[i].tinyframes[j].normals;


				retModel->meshes[i].tinyframes[j].material = &retModel->materials[i];

				for (k = 0; k < mdS->numVerts; k++)
				{

					*vertptr = mdV[k].x;
					vertptr++;
					*vertptr = mdV[k].z;
					vertptr++;
					*vertptr = 1.0f - mdV[k].y;
					vertptr++;


					GetNormalFromLatLong(mdV[k].n, tempNormal);
					*normptr = (char)(tempNormal[0] * 127);
					normptr++;
					*normptr = (char)(tempNormal[2] * 127);
					normptr++;
					*normptr = (char)(tempNormal[1] * 127);
					normptr++;
				}
			}

			uvptr = (float*)retModel->meshes[i].uvs;
			mdST = (md3TexCoord*)((char*)mdS + mdS->offsetST);
			for (j = 0; j < mdS->numVerts; j++)
			{
				*uvptr = mdST[j].st[0];
				uvptr++;
				*uvptr = mdST[j].st[1];
				uvptr++;
			}

			indexptr = retModel->meshes[i].indices;
			mdT = (md3Triangle*)((char*)mdS + mdS->offsetTriangles);
			for (j = 0; j < mdS->numTriangles; j++, mdT++)
			{

				*indexptr = (unsigned short)mdT->index[0];
				indexptr++;
				*indexptr = (unsigned short)mdT->index[1];
				indexptr++;
				*indexptr = (unsigned short)mdT->index[2];
				indexptr++;
			}
		}
		else  
		{
			float dataScale = 0.015624f * WUNITS;
			float tempNormal[3];
			md3TexCoord *mdST;
			md3Triangle *mdT;
			float *uvptr;
			int k;

			retModel->meshes[i].numVertices = mdS->numTriangles * 3; 
			retModel->meshes[i].frames = (mdlframe_t*)Z_Calloc(sizeof(mdlframe_t)*mdS->numFrames, ztag, 0);
			retModel->meshes[i].uvs = (float*)Z_Malloc(sizeof(float)*2*mdS->numTriangles*3, ztag, 0);

			for (j = 0; j < mdS->numFrames; j++)
			{
				float *vertptr;
				float *normptr;
				md3Vertex *mdV = (md3Vertex*)((char*)mdS + mdS->offsetXYZNormal + (mdS->numVerts*j*sizeof(md3Vertex)));
				retModel->meshes[i].frames[j].vertices = (float*)Z_Malloc(sizeof(float)*3*mdS->numTriangles*3, ztag, 0);
				retModel->meshes[i].frames[j].normals = (float*)Z_Malloc(sizeof(float)*3*mdS->numTriangles*3, ztag, 0);


				vertptr = retModel->meshes[i].frames[j].vertices;
				normptr = retModel->meshes[i].frames[j].normals;
				retModel->meshes[i].frames[j].material = &retModel->materials[i];

				mdT = (md3Triangle*)((char*)mdS + mdS->offsetTriangles);

				for (k = 0; k < mdS->numTriangles; k++)
				{

					*vertptr = mdV[mdT->index[0]].x * dataScale;
					vertptr++;
					*vertptr = mdV[mdT->index[0]].z * dataScale;
					vertptr++;
					*vertptr = 1.0f - mdV[mdT->index[0]].y * dataScale;
					vertptr++;

					GetNormalFromLatLong(mdV[mdT->index[0]].n, tempNormal);
					*normptr = tempNormal[0];
					normptr++;
					*normptr = tempNormal[2];
					normptr++;
					*normptr = tempNormal[1];
					normptr++;


					*vertptr = mdV[mdT->index[1]].x * dataScale;
					vertptr++;
					*vertptr = mdV[mdT->index[1]].z * dataScale;
					vertptr++;
					*vertptr = 1.0f - mdV[mdT->index[1]].y * dataScale;
					vertptr++;

					GetNormalFromLatLong(mdV[mdT->index[1]].n, tempNormal);
					*normptr = tempNormal[0];
					normptr++;
					*normptr = tempNormal[2];
					normptr++;
					*normptr = tempNormal[1];
					normptr++;


					*vertptr = mdV[mdT->index[2]].x * dataScale;
					vertptr++;
					*vertptr = mdV[mdT->index[2]].z * dataScale;
					vertptr++;
					*vertptr = 1.0f - mdV[mdT->index[2]].y * dataScale;
					vertptr++;

					GetNormalFromLatLong(mdV[mdT->index[2]].n, tempNormal);
					*normptr = tempNormal[0];
					normptr++;
					*normptr = tempNormal[2];
					normptr++;
					*normptr = tempNormal[1];
					normptr++;

					mdT++;  
				}
			}

			mdST = (md3TexCoord*)((char*)mdS + mdS->offsetST);
			uvptr = (float*)retModel->meshes[i].uvs;
			mdT = (md3Triangle*)((char*)mdS + mdS->offsetTriangles);

			for (k = 0; k < mdS->numTriangles; k++)
			{
				*uvptr = mdST[mdT->index[0]].st[0];
				uvptr++;
				*uvptr = mdST[mdT->index[0]].st[1];
				uvptr++;

				*uvptr = mdST[mdT->index[1]].st[0];
				uvptr++;
				*uvptr = mdST[mdT->index[1]].st[1];
				uvptr++;

				*uvptr = mdST[mdT->index[2]].st[0];
				uvptr++;
				*uvptr = mdST[mdT->index[2]].st[1];
				uvptr++;

				mdT++;  
			}
		}
	}
	  
         
                                  
                                         
                                                                                             
                                                   
                                
                                     
  
        
                                                  
   
                                                                           
                                              
                                              
                                              
                                              
                                              
                                              
                                              
                                              
                                              
                                                         
                                                         
                                                         
                                    

                                                             
            
   
    


	free(buffer);

	return retModel;
}
