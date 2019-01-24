/* Vector and Matrix routines for AVRIL; these do not care whether
   we're using floating-point or fixed-point.  The routines that do care
   are in math.c (or possibly math.asm) */

/* Should use cross product when doing matrix multipies */
/* Should have smarter rotation routines! */
/* 

   Copyright 1994, 1995, 1996, 1997, 1998, 1999  by Bernie Roehl 

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA,
   02111-1307, USA.

   For more information, contact Bernie Roehl <broehl@uwaterloo.ca> 
   or at Bernie Roehl, 68 Margaret Avenue North, Waterloo, Ontario, 
   N2J 3P7, Canada

*/

#include "avril.h"

#define TRANS 3

extern void vrl_InternalMatmult3x3(vrl_Matrix result, vrl_Matrix m1, vrl_Matrix m2);

vrl_Vector vrl_VectorNULL = { 0, 0, 0 };

/* Vector functions: */

void vrl_VectorCreate(vrl_Vector result, vrl_Scalar x, vrl_Scalar y, vrl_Scalar z)
	{
	result[X] = x;  result[Y] = y;  result[Z] = z;
	}

void vrl_VectorAdd(vrl_Vector result, vrl_Vector v1, vrl_Vector v2)
	{
	result[X] = v1[X] + v2[X];
	result[Y] = v1[Y] + v2[Y];
	result[Z] = v1[Z] + v2[Z];
	}

void vrl_VectorSub(vrl_Vector result, vrl_Vector v1, vrl_Vector v2)
	{
	result[X] = v1[X] - v2[X];
	result[Y] = v1[Y] - v2[Y];
	result[Z] = v1[Z] - v2[Z];
	}
	
void vrl_VectorNegate(vrl_Vector v)
	{
	v[X] = -v[X];
	v[Y] = -v[Y];
	v[Z] = -v[Z];
	}
	
/* Matrix functions: */

void vrl_MatrixIdentity(vrl_Matrix m)
	{
	vrl_MatrixResetRotations(m);
	m[TRANS][X] = m[TRANS][Y] = m[TRANS][Z] = 0;
	}

/* These next three are not very efficient */

/* These next three routines should be made smarter, since the matrices are sparse */

void vrl_MatrixRotX(vrl_Matrix m, vrl_Angle angle, vrl_Boolean leftside)
	{
	vrl_Matrix s, t;
	vrl_MatrixIdentity(s);
	s[2][2] = s[1][1] = vrl_Cosine(angle);
	s[1][2] = -(s[2][1] = vrl_Sine(angle));
	vrl_MatrixCopy(t, m);
	if (leftside)
		vrl_InternalMatmult3x3(m, s, t);
	else
		vrl_InternalMatmult3x3(m, t, s);
	}

void vrl_MatrixRotY(vrl_Matrix m, vrl_Angle angle, vrl_Boolean leftside)
	{
	vrl_Matrix s, t;
	vrl_MatrixIdentity(s);
	s[2][2] = s[0][0] = vrl_Cosine(angle);
	s[2][0] = -(s[0][2] = vrl_Sine(angle));
	vrl_MatrixCopy(t, m);
	if (leftside)
		vrl_InternalMatmult3x3(m, s, t);
	else
		vrl_InternalMatmult3x3(m, t, s);
	}

void vrl_MatrixRotZ(vrl_Matrix m, vrl_Angle angle, vrl_Boolean leftside)
	{
	vrl_Matrix s, t;
	vrl_MatrixIdentity(s);
	s[1][1] = s[0][0] = vrl_Cosine(angle);
	s[0][1] = -(s[1][0] = vrl_Sine(angle));
	vrl_MatrixCopy(t, m);
	if (leftside)
		vrl_InternalMatmult3x3(m, s, t);
	else
		vrl_InternalMatmult3x3(m, t, s);
	}

void vrl_MatrixRotVector(vrl_Matrix m, vrl_Angle angle, vrl_Vector v, vrl_Boolean leftside)
	{
	vrl_Matrix tmp1, tmp2;
	vrl_Factor s = vrl_Sine(-angle), c = vrl_Cosine(-angle);
	vrl_Factor t = VRL_UNITY - c;
	vrl_Factor x = v[X], y = v[Y], z = v[Z];
	tmp1[0][0] = vrl_FactorMultiply(vrl_FactorMultiply(t, x), x) + c;
	tmp1[0][1] = vrl_FactorMultiply(vrl_FactorMultiply(t, x), y) + vrl_FactorMultiply(s, z);
	tmp1[0][2] = vrl_FactorMultiply(vrl_FactorMultiply(t, x), z) - vrl_FactorMultiply(s, y);
	tmp1[1][0] = vrl_FactorMultiply(vrl_FactorMultiply(t, x), y) - vrl_FactorMultiply(s, z);
	tmp1[1][1] = vrl_FactorMultiply(vrl_FactorMultiply(t, y), y) + c;
	tmp1[1][2] = vrl_FactorMultiply(vrl_FactorMultiply(t, y), z) + vrl_FactorMultiply(s, x);
	tmp1[2][0] = vrl_FactorMultiply(vrl_FactorMultiply(t, x), z) + vrl_FactorMultiply(s, y);
	tmp1[2][1] = vrl_FactorMultiply(vrl_FactorMultiply(t, y), z) - vrl_FactorMultiply(s, x);
	tmp1[2][2] = vrl_FactorMultiply(vrl_FactorMultiply(t, z), z) + c;
	vrl_MatrixCopy(tmp2, m);
	if (leftside)
		vrl_InternalMatmult3x3(m, tmp1, tmp2);
	else
		vrl_InternalMatmult3x3(m, tmp2, tmp1);
	}

void vrl_MatrixTranslate(vrl_Matrix result, vrl_Vector v, vrl_Boolean leftside)
	{
	if (leftside)
		{
		result[TRANS][X] += v[X];
		result[TRANS][Y] += v[Y];
		result[TRANS][Z] += v[Z];
		}
	else
		{
		vrl_Vector vtmp;
		vrl_Transform(vtmp, result, v);
		vrl_VectorCopy(result[TRANS], vtmp);
		}
	}

/* This next routine should really implement the cross-product trick */

void vrl_MatrixMultiply(vrl_Matrix result, vrl_Matrix m1, vrl_Matrix m2)
	{
	vrl_InternalMatmult3x3(result, m1, m2);
	vrl_Transform(result[TRANS], m1, m2[TRANS]);
	}

void vrl_MatrixInverse(vrl_Matrix result, vrl_Matrix m)
	{
	/* transpose the rotation part */
	result[0][0] = m[0][0];  result[0][1] = m[1][0];  result[0][2] = m[2][0];
	result[1][0] = m[0][1];  result[1][1] = m[1][1];  result[1][2] = m[2][1];
	result[2][0] = m[0][2];  result[2][1] = m[1][2];  result[2][2] = m[2][2];
	/* transform the translation part */
	result[TRANS][X] = -vrl_VectorDotproduct(result[X], m[TRANS]);
	result[TRANS][Y] = -vrl_VectorDotproduct(result[Y], m[TRANS]);
	result[TRANS][Z] = -vrl_VectorDotproduct(result[Z], m[TRANS]);
	}

/* Transformation functions: */

void vrl_Transform(vrl_Vector result, vrl_Matrix m, vrl_Vector v)
	{
	result[X] = vrl_TransformX(m, v);
	result[Y] = vrl_TransformY(m, v);
	result[Z] = vrl_TransformZ(m, v);
	}

vrl_Scalar vrl_TransformX(vrl_Matrix m, vrl_Vector v)
	{
	return vrl_VectorDotproduct(m[X], v) + m[TRANS][X];
	}

vrl_Scalar vrl_TransformY(vrl_Matrix m, vrl_Vector v)
	{
	return vrl_VectorDotproduct(m[Y], v) + m[TRANS][Y];
	}

vrl_Scalar vrl_TransformZ(vrl_Matrix m, vrl_Vector v)
	{
	return vrl_VectorDotproduct(m[Z], v) + m[TRANS][Z];
	}

void vrl_MatrixResetRotations(vrl_Matrix m)
	{
	memset(m, 0, 9 * sizeof(vrl_Factor));
	m[0][0] = m[1][1] = m[2][2] = VRL_UNITY;
	}

void vrl_VectorNormalize(vrl_Vector v)
	{
	vrl_Scalar mag = vrl_VectorMagnitude(v);
	v[X] = vrl_ScalarDivide(v[X], mag);
	v[Y] = vrl_ScalarDivide(v[Y], mag);
	v[Z] = vrl_ScalarDivide(v[Z], mag);
	}

void vrl_MatrixGetBasis(vrl_Vector v, vrl_Matrix m, int axis)
	{
	v[X] = m[X][axis];
	v[Y] = m[Y][axis];
	v[Z] = m[Z][axis];
	}

void vrl_MatrixSetBasis(vrl_Matrix m, vrl_Vector v, int axis)
	{
	m[X][axis] = v[X];
	m[Y][axis] = v[Y];
	m[Z][axis] = v[Z];
	}

/* This next routine is really, really slow -- don't use it often */

#include <math.h>

#define PI 3.14159262

/* This next routine was written by Edwin Klement (ek@cs.brown.edu) */

#define epsilon 0.000001

void vrl_MatrixGetRotations(vrl_Matrix m, vrl_Angle *rx, vrl_Angle *ry, vrl_Angle *rz)
	{
	float m00 = factor2float(m[0][0]);
	float m01 = factor2float(m[0][1]);
	float m10 = factor2float(m[1][0]);
	float m11 = factor2float(m[1][1]);
	float m20 = factor2float(m[2][0]);
	float m21 = factor2float(m[2][1]);
	float m22 = factor2float(m[2][2]);
    if ((1.0 - fabs(m21)) < epsilon)
	    {
		if (rx) *rx = float2angle((m21 < 0) ? 90 : -90);
		if (ry) *ry = float2angle(0.0);
		if (rz) *rz = float2angle(-atan2(m10, m00) * 180 / PI);
	    }
	else
		{
		float rotz, a;
		if (ry) *ry = float2angle(atan2(-m20, m22) * 180 / PI);
		if (rz) *rz = float2angle(atan2(-m01, m11) * 180 / PI);
		rotz = atan2(-m01, m11);
		if (fabs(fabs(rotz) - PI / 2.0) < epsilon)
	    	a = m01 / sin(rotz);
		else
			a = m11 / cos(rotz);
		if (rx) *rx = -float2angle(atan2(-m21, a) * 180 / PI);
		}
	}

void vrl_VectorScale(vrl_Vector v, vrl_Scalar newmag)
	{
	v[X] = vrl_FactorMultiply(v[X], newmag);
	v[Y] = vrl_FactorMultiply(v[Y], newmag);
	v[Z] = vrl_FactorMultiply(v[Z], newmag);
	}

void vrl_VectorRescale(vrl_Vector v, vrl_Scalar newmag)
	{
	vrl_VectorNormalize(v);
	vrl_VectorScale(v, newmag);
	}

vrl_Scalar vrl_VectorDistance(vrl_Vector v1, vrl_Vector v2)
	{
	vrl_Vector v;
	vrl_VectorSub(v, v1, v2);
	return vrl_VectorMagnitude(v);
	}
