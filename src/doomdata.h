// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------






#ifndef __DOOMDATA__
#define __DOOMDATA__


#include "doomtype.h"


#include "doomdef.h"

#include "taglist.h"
#include "m_fixed.h"  









enum
{
	ML_LABEL,     
	ML_THINGS,    
	ML_LINEDEFS,  
	ML_SIDEDEFS,  
	ML_VERTEXES,  
	ML_SEGS,      
	ML_SSECTORS,  
	ML_NODES,     
	ML_SECTORS,   
	ML_REJECT,     
	ML_BLOCKMAP,   
};


#define MTF_EXTRA 1


#define MTF_OBJECTFLIP 2


#define MTF_OBJECTSPECIAL 4


#define MTF_AMBUSH 8





#define MTF_ABSOLUTEZ 16

#if defined(_MSC_VER)
#pragma pack(1)
#endif


typedef struct
{
	INT16 x, y;
}ATTRPACK  mapvertex_t;



typedef struct
{
	INT16 textureoffset, rowoffset;
	char toptexture[8], bottomtexture[8], midtexture[8];

	INT16 sector;
} ATTRPACK mapsidedef_t;



typedef struct
{
	INT16 v1, v2;
	INT16 flags;
	INT16 special;
	INT16 tag;

	UINT16 sidenum[2];
} ATTRPACK maplinedef_t;






#define ML_IMPASSIBLE           1


#define ML_BLOCKMONSTERS        2


#define ML_TWOSIDED             4











#define ML_DONTPEGTOP           8


#define ML_DONTPEGBOTTOM       16

#define ML_SKEWTD              32


#define ML_NOCLIMB             64

#define ML_NOSKEW             128
#define ML_MIDPEG             256
#define ML_MIDSOLID           512
#define ML_WRAPMIDTEX        1024

#define ML_NETONLY           2048  
#define ML_NONET             4096  
#define ML_EFFECT6           8192


#define ML_BOUNCY           16384

#define ML_TFERLINE         32768


typedef struct
{
	INT16 floorheight;
	INT16 ceilingheight;
	char floorpic[8];
	char ceilingpic[8];
	INT16 lightlevel;
	INT16 special;
	INT16 tag;
} ATTRPACK mapsector_t;


typedef struct
{
	UINT16 numsegs;

	UINT16 firstseg;
} ATTRPACK mapsubsector_t;




typedef struct
{
	INT16 v1, v2;
	INT16 angle;
	INT16 linedef;
	INT16 side;
	INT16 offset;
} ATTRPACK mapseg_t;




#define NF_SUBSECTOR 0x8000

typedef struct
{

	INT16 x, y;
	INT16 dx, dy;


	INT16 bbox[2][4];


	UINT16 children[2];
} ATTRPACK mapnode_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

#define NUMMAPTHINGARGS 10
#define NUMMAPTHINGSTRINGARGS 2



typedef struct
{
	INT16 x, y;
	INT16 angle, pitch, roll;
	UINT16 type;
	UINT16 options;
	INT16 z;
	UINT8 extrainfo;
	taglist_t tags;
	fixed_t scale;
	fixed_t spritexscale, spriteyscale;
	INT32 args[NUMMAPTHINGARGS];
	char *stringargs[NUMMAPTHINGSTRINGARGS];
	struct mobj_s *mobj;
} mapthing_t;

#define ZSHIFT 4

#define NUMMAPS 1035

#endif  
