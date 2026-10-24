// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef _HWR_DATA_
#define _HWR_DATA_

#if defined (_WIN32) && !defined (__CYGWIN__)

#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif

#include "../doomdef.h"
#include "../screen.h"






typedef enum GLTextureFormat_e
{
	GL_TEXFMT_P_8                 = 0x01,  
	GL_TEXFMT_AP_88               = 0x02,  

	GL_TEXFMT_RGBA                = 0x10,  

	GL_TEXFMT_ALPHA_8             = 0x20,  
	GL_TEXFMT_INTENSITY_8         = 0x21,  
	GL_TEXFMT_ALPHA_INTENSITY_88  = 0x22,
} GLTextureFormat_t;


struct GLColormap_s
{
	const UINT8 *source;
	UINT8 data[256];
};
typedef struct GLColormap_s GLColormap_t;





struct GLMipmap_s
{

	GLTextureFormat_t     format;
	void                 *data;

	UINT32                flags;
	UINT16                height;
	UINT16                width;
	UINT32                downloaded;  

	struct GLMipmap_s    *nextcolormap;
	struct GLColormap_s  *colormap;
};
typedef struct GLMipmap_s GLMipmap_t;





struct GLMapTexture_s
{
	GLMipmap_t  mipmap;
	float       scaleX;  
	float       scaleY;
};
typedef struct GLMapTexture_s GLMapTexture_t;



struct GLPatch_s
{
	GLMipmap_t *mipmap;  
	GLMipmap_t *colormapcache[16];  
	float       max_s, max_t;
};
typedef struct GLPatch_s GLPatch_t;

#endif  
