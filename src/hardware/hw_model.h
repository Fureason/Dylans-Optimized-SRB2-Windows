/*
	From the 'Wizard2' engine by Spaddlewit Inc. ( http://www.spaddlewit.com )
	An experimental work-in-progress.

	Donated to Sonic Team Junior and adapted to work with
	Sonic Robo Blast 2. The license of this code matches whatever
	the licensing is for Sonic Robo Blast 2.
*/

#ifndef _HW_MODEL_H_
#define _HW_MODEL_H_

#include "../doomtype.h"

typedef struct
{
	float x, y, z;
} vector_t;

extern vector_t vectorXaxis;
extern vector_t vectorYaxis;
extern vector_t vectorZaxis;

void VectorRotate(vector_t *rotVec, const vector_t *axisVec, float angle);

typedef struct
{
	float ambient[4], diffuse[4], specular[4], emissive[4];
	float shininess;
	boolean spheremap;


} material_t;

typedef struct
{
	material_t *material;  
	float *vertices;
	float *normals;
	float *tangents;
	char *colors;
	unsigned int vboID;
	vector_t *polyNormals;
} mdlframe_t;

typedef struct
{
	material_t *material;
	short *vertices;
	char *normals;
	char *tangents;
	unsigned int vboID;
} tinyframe_t;


typedef struct mesh_s
{
	int numVertices;
	int numTriangles;

	float *uvs;




	float *originaluvs;
	float *lightuvs;

	int numFrames;
	mdlframe_t *frames;
	tinyframe_t *tinyframes;
	unsigned short *indices;

	                                                                          
                                                                            
                                                                              
                                
	void *lerpVertices;
	void *lerpNormals;
	INT32 lerpFrame;
	INT32 lerpNextFrame;
	float lerpFraction;
	boolean lerpValid;
} mesh_t;

typedef struct tag_s
{
	char name[64];

} tag_t;

#define MODEL_INTERPOLATION_FLAG "+i"

typedef struct
{
	INT32 frames[256];
	UINT8 numframes;
	boolean interpolate;
} modelspr2frames_t;

typedef struct model_s
{
	int maxNumFrames;

	int numMeshes;
	mesh_t *meshes;
	int numMaterials;
	material_t *materials;
	int numTags;
	tag_t *tags;

	char *frameNames;
	boolean interpolate[256];
	modelspr2frames_t *spr2frames;
	modelspr2frames_t *superspr2frames;



	float max_s;
	float max_t;


	float vbo_max_s;
	float vbo_max_t;
} model_t;

extern int numModels;
extern model_t *modelHead;

void HWR_ReloadModels(void);

tag_t *GetTagByName(model_t *model, char *name, int frame);
model_t *LoadModel(const char *filename, int ztag);
void UnloadModel(model_t *model);
void Optimize(model_t *model);
void LoadModelInterpolationSettings(model_t *model);
void LoadModelSprite2(model_t *model);
void GenerateVertexNormals(model_t *model);
void GeneratePolygonNormals(model_t *model, int ztag);
void CreateVBOTiny(mesh_t *mesh, tinyframe_t *frame);
void CreateVBO(mesh_t *mesh, mdlframe_t *frame);
void DeleteVBOs(model_t *model);

#endif
