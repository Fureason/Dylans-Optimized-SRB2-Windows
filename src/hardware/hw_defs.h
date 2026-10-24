// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef _HWR_DEFS_
#define _HWR_DEFS_
#include "../doomtype.h"
#include "../r_defs.h"

#define ZCLIP_PLANE 4.0f  
#define NZCLIP_PLANE 0.9f  





#define HWR_PALETTE_LUT_SIZE 64





typedef long            FINT;
typedef unsigned long   FUINT;
typedef unsigned char   FUBYTE;
typedef unsigned long   FBITFIELD;
#ifndef __MINGW32__
typedef float           FLOAT;
#endif
typedef unsigned char   FBOOLEAN;






#define HWR_PATCHES_CHROMAKEY_COLORINDEX   255



#define HWR_PATCHES_CHROMAKEY_COLORVALUE     (0x00000000)     


struct FRGBAFloat
{
	FLOAT   red;
	FLOAT   green;
	FLOAT   blue;
	FLOAT   alpha;
};
typedef struct FRGBAFloat FRGBAFloat;

struct FColorARGB
{
	FUBYTE  alpha;
	FUBYTE  red;
	FUBYTE  green;
	FUBYTE  blue;
};
typedef struct FColorARGB ARGB_t;
typedef struct FColorARGB FColorARGB;








typedef struct
{
	FLOAT x,y;
} F2DCoord, v2d_t;


typedef struct FVector
{
	FLOAT x,y,z;
} FVector;















#define USE_MODEL_NEXTFRAME

typedef struct
{
	FLOAT       x,y,z;            
	FLOAT       anglex,angley,anglez;    
	FLOAT       scalex,scaley,scalez;
	FLOAT       fovxangle, fovyangle;
	UINT8       splitscreen;
	boolean     flip;             
	boolean     roll;
	FLOAT       rollangle;  
	FLOAT       centerx, centery;
	FLOAT       rollx, rollz;
	boolean     mirror;           
	boolean     shearing;         
	float       viewaiming;       
} FTransform;


typedef struct
{
	FLOAT       x,y,z;
	FLOAT       s;             
	FLOAT       t;             
} FOutVector;

#ifdef GL_SHADERS




enum
{
	SHADER_NONE = -1,

	SHADER_FLOOR = 0,
	SHADER_WALL,
	SHADER_SPRITE,
	SHADER_MODEL,
	SHADER_WATER,
	SHADER_FOG,
	SHADER_SKY,
	SHADER_PALETTE_POSTPROCESS,
	SHADER_UI_COLORMAP_FADE,
	SHADER_UI_TINTED_WIPE,

	NUMSHADERTARGETS
};



#define HWR_MAXSHADERS NUMSHADERTARGETS*2


typedef struct
{
	const char *type;
	INT32 id;
} customshaderxlat_t;

#endif

typedef struct vbo_vertex_s
{
	float x, y, z;
	float u, v;
	unsigned char r, g, b, a;
} gl_skyvertex_t;

typedef enum gl_skyloopmode_e
{
	HWD_SKYLOOP_FAN,
	HWD_SKYLOOP_STRIP
} gl_skyloopmode_t;

typedef struct
{
	gl_skyloopmode_t mode;
	int vertexcount;
	int vertexindex;
	boolean use_texture;
} gl_skyloopdef_t;

typedef struct
{
	unsigned int vbo;
	int rows, columns;
	int loopcount;

	int detail, vertex_count;
	int texture, width, height;
	boolean rebuild;  

	gl_skyloopdef_t *loops;
	gl_skyvertex_t *data;
} gl_sky_t;







enum EPolyFlags
{

	PF_Masked           = 0x00000001,    
	PF_Translucent      = 0x00000002,    
	PF_Environment      = 0x00000004,    
	PF_Additive         = 0x00000008,    
	PF_Subtractive      = 0x00000010,    
	PF_ReverseSubtract  = 0x00000020,    
	PF_Multiplicative   = 0x00000040,    
	PF_Fog              = 0x20000000,    
	PF_NoAlphaTest      = 0x40000000,    
	PF_Blending         = (PF_Masked|PF_Translucent|PF_Environment|PF_Additive|PF_Subtractive|PF_ReverseSubtract|PF_Multiplicative|PF_Fog) & ~PF_NoAlphaTest,


	PF_Occlude          = 0x00000100,    
	PF_NoDepthTest      = 0x00000200,    
	PF_Invisible        = 0x00000400,    
	PF_Decal            = 0x00000800,    
	PF_Modulated        = 0x00001000,    

	PF_NoTexture        = 0x00002000,    
	PF_Corona           = 0x00004000,    
	PF_ColorMapped      = 0x00008000,    
	PF_RemoveYWrap      = 0x00010000,    
	PF_ForceWrapX       = 0x00020000,    
	PF_ForceWrapY       = 0x00040000,    
	PF_Ripple           = 0x00100000,    
	PF_WireFrame        = 0x00200000,    
};


enum ESurfFlags
{
	SF_DYNLIGHT         = 0x00000001,
};

enum ETextureFlags
{
	TF_WRAPX       = 0x00000001,         
	TF_WRAPY       = 0x00000002,         
	TF_WRAPXY      = TF_WRAPY|TF_WRAPX,  
	TF_CHROMAKEYED = 0x00000010,         
	TF_OPAQUE      = 0x00000020,         
	TF_TRANSPARENT = 0x00000040,         
	TF_SPRITE      = 0x00000080,         
	TF_PALETTE_INDEPENDENT = 0x00000100,  
};

struct FTextureInfo
{
	UINT32 width, height;
	UINT32 downloaded;
	UINT32 format;

	struct GLMipmap_s *texture;
	struct FTextureInfo *prev, *next;
};
typedef struct FTextureInfo FTextureInfo;

struct FLightInfo
{
	FUINT			light_level;
	FUINT			fade_start;
	FUINT			fade_end;
};
typedef struct FLightInfo FLightInfo;


struct FSurfaceInfo
{
	FUINT			PolyFlags;
	RGBA_t			PolyColor;
	RGBA_t			TintColor;
	RGBA_t			FadeColor;
	UINT32			LightTableId;
	FLightInfo		LightInfo;
};
typedef struct FSurfaceInfo FSurfaceInfo;

#define GL_DEFAULTMIX 0x00000000
#define GL_DEFAULTFOG 0xFF000000


enum hwdsetspecialstate
{
	HWD_SET_MODEL_LIGHTING = 1,
	HWD_SET_SHADERS,
	HWD_SET_TEXTUREFILTERMODE,
	HWD_SET_SPRITEFILTERMODE,
	HWD_SET_TEXTUREANISOTROPICMODE,
	HWD_SET_WIREFRAME,
	HWD_SET_PORTALSTENCIL,
	HWD_SET_MULTISAMPLE,
	HWD_SET_COLORWRITE,
	HWD_NUMSTATE
};

typedef enum hwdsetspecialstate hwdspecialstate_t;

enum hwdshaderstage
{
	HWD_SHADERSTAGE_VERTEX,
	HWD_SHADERSTAGE_FRAGMENT,
};

typedef enum hwdshaderstage hwdshaderstage_t;



enum hwdshaderinfo
{
	HWD_SHADERINFO_LEVELTIME = 1,
};

typedef enum hwdshaderinfo hwdshaderinfo_t;

enum hwdfiltermode
{
	HWD_SET_TEXTUREFILTER_POINTSAMPLED,
	HWD_SET_TEXTUREFILTER_BILINEAR,
	HWD_SET_TEXTUREFILTER_TRILINEAR,
	HWD_SET_TEXTUREFILTER_MIXED1,
	HWD_SET_TEXTUREFILTER_MIXED2,
	HWD_SET_TEXTUREFILTER_MIXED3,
	HWD_SET_TEXTUREFILTER_SHARP,
};


enum hwdscreentexture
{
	HWD_SCREENTEXTURE_WIPE_START,  
	HWD_SCREENTEXTURE_WIPE_END,    
	HWD_SCREENTEXTURE_GENERIC1,    
	HWD_SCREENTEXTURE_GENERIC2,    
	HWD_SCREENTEXTURE_GENERIC3,    
	NUMSCREENTEXTURES,             
};

typedef enum hwdscreentexture hwdscreentexture_t;


#endif  
