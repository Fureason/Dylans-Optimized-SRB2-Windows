// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __TABLES__
#define __TABLES__

#ifdef LINUX
#include <math.h>
#endif

#include "m_fixed.h"

#define FINEANGLES 8192
#define FINEMASK (FINEANGLES - 1)
#define TANMASK 4095
#define ANGLETOFINESHIFT 19  
#define FINEANGLE_C(x) ((FixedAngle((x)*FRACUNIT)>>ANGLETOFINESHIFT) & FINEMASK)  


extern fixed_t finesine[5*FINEANGLES/4];


extern fixed_t *finecosine;


extern fixed_t finetangent[FINEANGLES/2];

#define ANG1   0x00B60B61  
#define ANG2   0x016C16C1  
#define ANG10  0x071C71C7  
#define ANG15  0x0AAAAAAB  
#define ANG20  0x0E38E38E  
#define ANG30  0x15555555  
#define ANG60  0x2AAAAAAB  
#define ANG64h 0x2DDDDDDE  
#define ANG105 0x4AAAAAAB  
#define ANG210 0x95555555  
#define ANG255 0xB5555555  
#define ANG340 0xF1C71C72  
#define ANG350 0xF8E38E39  

#define ANGLE_11hh 0x08000000
#define ANGLE_22h  0x10000000
#define ANGLE_45   0x20000000
#define ANGLE_67h  0x30000000
#define ANGLE_90   0x40000000
#define ANGLE_112h 0x50000000
#define ANGLE_135  0x60000000
#define ANGLE_157h 0x70000000
#define ANGLE_180  0x80000000
#define ANGLE_202h 0x90000000
#define ANGLE_225  0xA0000000
#define ANGLE_247h 0xB0000000
#define ANGLE_270  0xC0000000
#define ANGLE_292h 0xD0000000
#define ANGLE_315  0xE0000000
#define ANGLE_337h 0xF0000000
#define ANGLE_MAX  0xFFFFFFFF

typedef UINT32 angle_t;





#define SLOPERANGE 2048
#define SLOPEBITS 11
#define DBITS (FRACBITS - SLOPEBITS)


extern angle_t tantoangle[SLOPERANGE+1];


FUNCMATH unsigned SlopeDiv(unsigned num, unsigned den);

UINT64 SlopeDivEx(unsigned int num, unsigned int den);


FUNCMATH FUNCINLINE static ATTRINLINE angle_t InvAngle(angle_t a)
{
	return (ANGLE_MAX-a)+1;
}

FUNCMATH fixed_t AngleFixed(angle_t af);

FUNCMATH angle_t FixedAngle(fixed_t fa);

FUNCMATH angle_t FixedAngleC(fixed_t fa, fixed_t factor);


FUNCMATH angle_t FixedAcos(fixed_t x);


angle_t FV2_AngleBetweenVectors(const vector2_t *Vector1, const vector2_t *Vector2);
angle_t FV3_AngleBetweenVectors(const vector3_t *Vector1, const vector3_t *Vector2);
boolean FV2_InsidePolygon(const vector2_t *vIntersection, const vector2_t *Poly, const INT32 vertexCount);
boolean FV3_InsidePolygon(const vector3_t *vIntersection, const vector3_t *Poly, const INT32 vertexCount);
boolean FV3_IntersectedPolygon(const vector3_t *vPoly, const vector3_t *vLine, const INT32 vertexCount, vector3_t *collisionPoint);
void FV3_Rotate(vector3_t *rotVec, const vector3_t *axisVec, const angle_t angle);

matrix_t *FM_Rotate(matrix_t *dest, angle_t angle, fixed_t x, fixed_t y, fixed_t z);
matrix_t *FM_RotateX(matrix_t *dest, angle_t rad);
matrix_t *FM_RotateY(matrix_t *dest, angle_t rad);
matrix_t *FM_RotateZ(matrix_t *dest, angle_t rad);


#define FINE_FRACBITS 16


#define FINESINE(n) (finesine[n]>>(FINE_FRACBITS-FRACBITS))
#define FINECOSINE(n) (finecosine[n]>>(FINE_FRACBITS-FRACBITS))
#define FINETANGENT(n) (finetangent[n]>>(FINE_FRACBITS-FRACBITS))

#endif
