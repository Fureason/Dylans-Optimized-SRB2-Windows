// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
// Copyright (C) 2009 by Stephen McGranahan.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
















#include "tables.h"

unsigned SlopeDiv(unsigned num, unsigned den)
{
	unsigned ans;
	num <<= (FINE_FRACBITS-FRACBITS);
	den <<= (FINE_FRACBITS-FRACBITS);
	if (den < 512)
		return SLOPERANGE;
	ans = (num<<3) / (den>>8);
	return ans <= SLOPERANGE ? ans : SLOPERANGE;
}

UINT64 SlopeDivEx(unsigned int num, unsigned int den)
{
	UINT64 ans;
	if (den < 512)
		return SLOPERANGE;
	ans = ((UINT64)num<<3)/(den>>8);
	return ans <= SLOPERANGE ? ans : SLOPERANGE;
}

fixed_t AngleFixed(angle_t af)
{
	angle_t wa = ANGLE_180;
	fixed_t wf = 180*FRACUNIT;
	fixed_t rf = 0*FRACUNIT;

	while (af)
	{
		while (af < wa)
		{
			wa /= 2;
			wf /= 2;
		}
		rf += wf;
		af -= wa;
	}

	return rf;
}

static FUNCMATH angle_t AngleAdj(const fixed_t fa, const fixed_t wf,
                                 angle_t ra)
{
	const angle_t adj = 0x77;
	const boolean fan = fa < 0;
	const fixed_t sl = FixedDiv(fa, wf*2);
	const fixed_t lb = fa % (wf*2);
	const fixed_t lo = (wf*2)-lb;

	if (ra == 0)
	{
		if (lb == 0)
		{
			ra = FixedMul(FRACUNIT/512, sl);
			if (ra > FRACUNIT/64)
				return InvAngle(ra);
			return ra;
		}
		else if (lb > 0)
			return InvAngle(FixedMul(lo*FRACUNIT, adj));
		else
			return InvAngle(FixedMul(lo*FRACUNIT, adj));
	}

	if (fan)
		return InvAngle(ra);
	else
		return ra;
}

angle_t FixedAngleC(fixed_t fa, fixed_t factor)
{
	angle_t wa = ANGLE_180;
	fixed_t wf = 180*FRACUNIT;
	angle_t ra = 0;
	const fixed_t cfa = fa;
	fixed_t cwf = wf;

	if (fa == 0)
		return 0;



	if (fa == INT32_MIN)
		return 0;

	if (factor == 0)
		return FixedAngle(fa);
	else if (factor > 0)
		cwf = wf = FixedMul(wf, factor);
	else if (factor < 0)
		cwf = wf = FixedDiv(wf, -factor);

	fa = abs(fa);

	while (fa)
	{
		while (fa < wf)
		{
			wa /= 2;
			wf /= 2;
		}
		ra = ra + wa;
		fa = fa - wf;
	}

	return AngleAdj(cfa, cwf, ra);
}

angle_t FixedAngle(fixed_t fa)
{
	angle_t wa = ANGLE_180;
	fixed_t wf = 180*FRACUNIT;
	angle_t ra = 0;
	const fixed_t cfa = fa;
	const fixed_t cwf = wf;

	if (fa == 0)
		return 0;



	if (fa == INT32_MIN)
		return 0;

	fa = abs(fa);

	while (fa)
	{
		while (fa < wf)
		{
			wa /= 2;
			wf /= 2;
		}
		ra = ra + wa;
		fa = fa - wf;
	}

	return AngleAdj(cfa, cwf, ra);
}


#include "t_ftan.c"

#include "t_fsin.c"
fixed_t *finecosine = &finesine[FINEANGLES/4];

#include "t_tan2a.c"

#include "t_facon.c"


FUNCMATH angle_t FixedAcos(fixed_t x)
{
	if (-FRACUNIT > x || x >= FRACUNIT) return 0;
	return fineacon[((x<<(FINE_FRACBITS-FRACBITS)))+FRACUNIT];
}






angle_t FV2_AngleBetweenVectors(const vector2_t *Vector1, const vector2_t *Vector2)
{









	fixed_t dotProduct = FV2_Dot(Vector1, Vector2);


	fixed_t vectorsMagnitude = FixedMul(FV2_Magnitude(Vector1), FV2_Magnitude(Vector2));


	return FixedAcos(FixedDiv(dotProduct, vectorsMagnitude));
}

angle_t FV3_AngleBetweenVectors(const vector3_t *Vector1, const vector3_t *Vector2)
{









	fixed_t dotProduct = FV3_Dot(Vector1, Vector2);


	fixed_t vectorsMagnitude = FixedMul(FV3_Magnitude(Vector1), FV3_Magnitude(Vector2));


	return FixedAcos(FixedDiv(dotProduct, vectorsMagnitude));
}






boolean FV2_InsidePolygon(const vector2_t *vIntersection, const vector2_t *Poly, const INT32 vertexCount)
{
	INT32 i;
	UINT64 Angle = 0;					 
	vector2_t vA, vB;					 















	for (i = 0; i < vertexCount; i++)		 
	{
		FV2_Point2Vec(&Poly[i], vIntersection, &vA);	 

		FV2_Point2Vec(&Poly[(i + 1) % vertexCount], vIntersection, &vB);

		Angle += FV2_AngleBetweenVectors(&vA, &vB);	 
	}








	if(Angle >= ANGLE_MAX)	 
		return 1;  

	return 0;  
}

boolean FV3_InsidePolygon(const vector3_t *vIntersection, const vector3_t *Poly, const INT32 vertexCount)
{
	INT32 i;
	UINT64 Angle = 0;					 
	vector3_t vA, vB;					 















	for (i = 0; i < vertexCount; i++)		 
	{
		FV3_Point2Vec(&Poly[i], vIntersection, &vA);	 

		FV3_Point2Vec(&Poly[(i + 1) % vertexCount], vIntersection, &vB);

		Angle += FV3_AngleBetweenVectors(&vA, &vB);	 
	}








	if(Angle >= ANGLE_MAX)	 
		return 1;  

	return 0;  
}






boolean FV3_IntersectedPolygon(const vector3_t *vPoly, const vector3_t *vLine, const INT32 vertexCount, vector3_t *collisionPoint)
{
	vector3_t vNormal, vIntersection;
	fixed_t originDistance = 0*FRACUNIT;






	if(!FV3_IntersectedPlane(vPoly, vLine,   &vNormal,   &originDistance))
		return false;







	FV3_IntersectionPoint(&vNormal, vLine, originDistance, &vIntersection);





	if(FV3_InsidePolygon(&vIntersection, vPoly, vertexCount))
	{
		if (collisionPoint != NULL)  
		{
			collisionPoint->x = vIntersection.x;
			collisionPoint->y = vIntersection.y;
			collisionPoint->z = vIntersection.z;
		}
		return true;  
	}


	return false;
}






void FV3_Rotate(vector3_t *rotVec, const vector3_t *axisVec, const angle_t angle)
{

	fixed_t ux = FixedMul(axisVec->x, rotVec->x);
	fixed_t uy = FixedMul(axisVec->x, rotVec->y);
	fixed_t uz = FixedMul(axisVec->x, rotVec->z);
	fixed_t vx = FixedMul(axisVec->y, rotVec->x);
	fixed_t vy = FixedMul(axisVec->y, rotVec->y);
	fixed_t vz = FixedMul(axisVec->y, rotVec->z);
	fixed_t wx = FixedMul(axisVec->z, rotVec->x);
	fixed_t wy = FixedMul(axisVec->z, rotVec->y);
	fixed_t wz = FixedMul(axisVec->z, rotVec->z);
	fixed_t sa = FINESINE(angle);
	fixed_t ca = FINECOSINE(angle);
	fixed_t ua = ux+vy+wz;
	fixed_t ax = FixedMul(axisVec->x,ua);
	fixed_t ay = FixedMul(axisVec->y,ua);
	fixed_t az = FixedMul(axisVec->z,ua);
	fixed_t xs = FixedMul(axisVec->x,axisVec->x);
	fixed_t ys = FixedMul(axisVec->y,axisVec->y);
	fixed_t zs = FixedMul(axisVec->z,axisVec->z);
	fixed_t bx = FixedMul(rotVec->x,ys+zs);
	fixed_t by = FixedMul(rotVec->y,xs+zs);
	fixed_t bz = FixedMul(rotVec->z,xs+ys);
	fixed_t cx = FixedMul(axisVec->x,vy+wz);
	fixed_t cy = FixedMul(axisVec->y,ux+wz);
	fixed_t cz = FixedMul(axisVec->z,ux+vy);
	fixed_t dx = FixedMul(bx-cx, ca);
	fixed_t dy = FixedMul(by-cy, ca);
	fixed_t dz = FixedMul(bz-cz, ca);
	fixed_t ex = FixedMul(vz-wy, sa);
	fixed_t ey = FixedMul(wx-uz, sa);
	fixed_t ez = FixedMul(uy-vx, sa);

	rotVec->x = ax+dx+ex;
	rotVec->y = ay+dy+ey;
	rotVec->z = az+dz+ez;
}

#define M(row,col) dest->m[row * 4 + col]
matrix_t *FM_Rotate(matrix_t *dest, angle_t angle, fixed_t x, fixed_t y, fixed_t z)
{
	const fixed_t sinA = FINESINE(angle>>ANGLETOFINESHIFT);
	const fixed_t cosA = FINECOSINE(angle>>ANGLETOFINESHIFT);
	const fixed_t invCosA = FRACUNIT - cosA;
	vector3_t nrm;
	fixed_t xSq, ySq, zSq;
	fixed_t sx, sy, sz;
	fixed_t sxy, sxz, syz;

	nrm.x = x;
	nrm.y = y;
	nrm.z = z;
	FV3_Normalize(&nrm);

	x = nrm.x;
	y = nrm.y;
	z = nrm.z;

	xSq = FixedMul(x, FixedMul(invCosA,x));
	ySq = FixedMul(y, FixedMul(invCosA,y));
	zSq = FixedMul(z, FixedMul(invCosA,z));

	sx = FixedMul(sinA, x);
	sy = FixedMul(sinA, y);
	sz = FixedMul(sinA, z);

	sxy = FixedMul(x, FixedMul(invCosA,y));
	sxz = FixedMul(x, FixedMul(invCosA,z));
	syz = FixedMul(y, FixedMul(invCosA,z));


	M(0, 0) = xSq + cosA;
	M(1, 0) = sxy - sz;
	M(2, 0) = sxz + sy;
	M(3, 0) = 0;

	M(0, 1) = sxy + sz;
	M(1, 1) = ySq + cosA;
	M(2, 1) = syz - sx;
	M(3, 1) = 0;

	M(0, 2) = sxz - sy;
	M(1, 2) = syz + sx;
	M(2, 2) = zSq + cosA;
	M(3, 2) = 0;

	M(0, 3) = 0;
	M(1, 3) = 0;
	M(2, 3) = 0;
	M(3, 3) = FRACUNIT;

	return dest;
}


matrix_t *FM_RotateX(matrix_t *dest, angle_t rad)
{
	const angle_t fa = rad>>ANGLETOFINESHIFT;
	const fixed_t cosrad = FINECOSINE(fa), sinrad = FINESINE(fa);

	M(0, 0) = FRACUNIT;
	M(0, 1) = 0;
	M(0, 2) = 0;
	M(0, 3) = 0;
	M(1, 0) = 0;
	M(1, 1) = cosrad;
	M(1, 2) = sinrad;
	M(1, 3) = 0;
	M(2, 0) = 0;
	M(2, 1) = -sinrad;
	M(2, 2) = cosrad;
	M(2, 3) = 0;
	M(3, 0) = 0;
	M(3, 1) = 0;
	M(3, 2) = 0;
	M(3, 3) = FRACUNIT;

	return dest;
}

matrix_t *FM_RotateY(matrix_t *dest, angle_t rad)
{
	const angle_t fa = rad>>ANGLETOFINESHIFT;
	const fixed_t cosrad = FINECOSINE(fa), sinrad = FINESINE(fa);

	M(0, 0) = cosrad;
	M(0, 1) = 0;
	M(0, 2) = -sinrad;
	M(0, 3) = 0;
	M(1, 0) = 0;
	M(1, 1) = FRACUNIT;
	M(1, 2) = 0;
	M(1, 3) = 0;
	M(2, 0) = sinrad;
	M(2, 1) = 0;
	M(2, 2) = cosrad;
	M(2, 3) = 0;
	M(3, 0) = 0;
	M(3, 1) = 0;
	M(3, 2) = 0;
	M(3, 3) =  FRACUNIT;

	return dest;
}

matrix_t *FM_RotateZ(matrix_t *dest, angle_t rad)
{
	const angle_t fa = rad>>ANGLETOFINESHIFT;
	const fixed_t cosrad = FINECOSINE(fa), sinrad = FINESINE(fa);

	M(0, 0) = cosrad;
	M(0, 1) = sinrad;
	M(0, 2) = 0;
	M(0, 3) = 0;
	M(1, 0) = -sinrad;
	M(1, 1) = cosrad;
	M(1, 2) = 0;
	M(1, 3) = 0;
	M(2, 0) = 0;
	M(2, 1) = 0;
	M(2, 2) = FRACUNIT;
	M(2, 3) = 0;
	M(3, 0) = 0;
	M(3, 1) = 0;
	M(3, 2) = 0;
	M(3, 3) = FRACUNIT;

	return dest;
}

#undef M
