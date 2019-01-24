/* The math routines for AVRIL, in C and 386 assembler */
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
#include <math.h>

#define PI 3.14159262

#ifdef VRL_USE_FIXED_POINT
#pragma inline
#endif

/* These next three macros will eventually go away,
   after the vrl_VectorCrossproduct() and vrl_VectorMagnitude()
   routines are rewritten */

#define sproduct(s1, s2) (scalar2float(s1) * scalar2float(s2))
#define fproduct(f1, f2) (factor2float(f1) * factor2float(f2))
#define sfproduct(s, f) (scalar2float(s) * factor2float(f))

#ifdef VRL_USE_FIXED_POINT

static vrl_Factor sintable[258];

void vrl_MathInit(void)
	{
	int i;
	/* Tricky stuff here:
	     Each entry in sintable[] is 1/256th of a quarter-circle;
	     since a quarter circle is PI/2 radians, each entry is therefore
	     1/256th of PI/2 radians.  We add a couple of entries at the end,
	     since we'll be interpolating between the two values that sandwich
	     our angle.
	 */
	for (i = 0; i < 256; ++i)
		sintable[i] = float2factor(sin(i * ((PI/2) / 256)));
	sintable[256] = sintable[257] = VRL_UNITY;
	}

vrl_Factor vrl_Sine(vrl_Angle angle)
	{
	unsigned char quadrant;
	unsigned int integer_part, fractional_part;
	vrl_Factor value, separation;
	/* More tricky stuff:
	   The angle we're given is in 16.16 format and is expressed in degrees.
	   The angle divided by 90 is equivalent to the index divided by 256;
	   we therefore multiply the angle by 256 and divide by 90 to get the
	   table index.
	  */
	angle = vrl_ScalarMultDiv(angle, 256, 90);
	/* These next three statements assume the angle is 16.16 */
	fractional_part = angle & 0xFFFF;
	integer_part = (angle >> 16) & 0xFF;  /* 0 to 255 */
	quadrant = (angle >> 24) & 0x03;
	/* quadrant 00:  0 to 90
	   quadrant 01:  90 to 180
	   quadrant 10: 180 to 270 (-90 to -180)
	   quadrant 11: 270 to 360 (0 to -90)
	 */
	if (quadrant & 0x01)
		{
		/* sin(90+x) == sin(90-x) */
		integer_part ^= 0xFF;
		fractional_part ^= 0xFFFF;
		}
	separation = sintable[integer_part+1] - sintable[integer_part];
	value = sintable[integer_part]
		+ vrl_ScalarMultDiv(separation, fractional_part, VRL_ANGLECONVERSION);
	if (quadrant & 0x02) value = -value;  /* sin(-angle) = -sin(angle) */
	return value;
	}

#else

void vrl_MathInit(void)
	{
	}

vrl_Factor vrl_Sine(vrl_Angle angle)
	{
	return float2factor(sin((PI * angle2float(angle)) / 180.0));
	}

vrl_Scalar vrl_ScalarRound(vrl_Scalar f)
	{
	return float2scalar(floor(scalar2float(f) + 0.5));
	}

#endif

vrl_Factor vrl_Cosine(vrl_Angle angle)
	{
	return vrl_Sine(angle + float2angle(90));
	}

#ifdef VRL_USE_FIXED_POINT

vrl_Factor vrl_ScalarDivide(vrl_Scalar alpha, vrl_Scalar beta)
	{
	asm {
		.386
		mov eax,alpha
		cdq
		shld edx,eax,29
		shl eax,29
		idiv DWORD PTR beta
		shld edx,eax,16
		}
	}

vrl_Scalar vrl_ScalarMultDiv(vrl_Scalar alpha, vrl_Scalar beta, vrl_Scalar gamma)
	{
	asm {
		.386
		mov eax,alpha
		imul DWORD PTR beta
		idiv DWORD PTR gamma
		shld edx,eax,16	
		}
	}	

vrl_Scalar vrl_FactorMultiply(vrl_Factor alpha, vrl_Scalar beta)
	{
	asm	{
		.386
		mov eax,alpha
		imul DWORD PTR beta
		shrd eax,edx,29
		adc eax,0
		shld edx,eax,16
		}
	}	

#endif

/* This next routine is used by the three matrix rotation routines,
   and by the matrix multiply routine */

#ifdef VRL_USE_FIXED_POINT

void vrl_InternalMatmult3x3(vrl_Matrix result, vrl_Matrix m1, vrl_Matrix m2)
	{
	asm {
		.386

mult	macro i,j,k
		mov eax,es:si[(i*3+k)*4]
		imul DWORD PTR ds:di[(k*3+j)*4]
		endm

element	macro i,j
		mult i,j,0
		mov ebx,eax
		mov ecx,edx
		mult i,j,1
		add ebx,eax
		adc ecx,edx
		mult i,j,2
		add eax,ebx
		adc edx,ecx
		shrd eax,edx,29
		adc eax,0
		endm

store	macro i,j
		push es
		push si
		les si,DWORD PTR result
		mov es:si[(i*3+j)*4],eax
		pop si
		pop es
		endm

		push ds
		push esi
		push edi
		les si,DWORD PTR m1
		lds di,DWORD PTR m2
		element 0,0
		store 0,0
		element 0,1
		store 0,1
		element 0,2
		store 0,2
		element 1,0
		store 1,0
		element 1,1
		store 1,1
		element 1,2
		store 1,2
		element 2,0
		store 2,0
		element 2,1
		store 2,1
		element 2,2
		store 2,2
		pop edi
		pop esi
		pop ds
		}
	}

vrl_Scalar vrl_VectorDotproduct(vrl_Vector v1, vrl_Vector v2)
	{
	asm	{
		.386
		push es
		push ds
		push esi
		push edi
		les si,DWORD PTR v1
		lds di,DWORD PTR v2
		mov eax,es:si[0]
		imul DWORD PTR ds:di[0]
		mov ecx,edx
		mov ebx,eax
		mov eax,es:si[4]
		imul DWORD PTR ds:di[4]
		add ebx,eax
		adc ecx,edx
		mov eax,es:si[8]
		imul DWORD PTR ds:di[8]
		add eax,ebx
		adc edx,ecx
		shrd eax,edx,29
		adc eax,0
		shld edx,eax,16
		pop edi
		pop esi
		pop ds
		pop es
		}
	}

#else

void vrl_InternalMatmult3x3(vrl_Matrix result, vrl_Matrix m1, vrl_Matrix m2)
	{
	int i, j;
	for (i = 0; i < 3; ++i)
		for (j = 0; j < 3; ++j)
			result[i][j] = float2factor(fproduct(m1[i][0], m2[0][j])
				+ fproduct(m1[i][1], m2[1][j])
				+ fproduct(m1[i][2], m2[2][j]));
	}

vrl_Scalar vrl_VectorDotproduct(vrl_Vector v1, vrl_Vector v2)
	{
	return float2scalar(sfproduct(v1[X], v2[X])
		+ sfproduct(v1[Y], v2[Y]) + sfproduct(v1[Z], v2[Z]));
	}

#endif

/* These last two routines should really be re-coded; does anyone have
   a fast 64-bit square root algorithm? */

vrl_Scalar vrl_VectorCrossproduct(vrl_Vector result, vrl_Vector v1, vrl_Vector v2)
	{
	float xf, yf, zf, m;
	xf = sproduct(v1[Y], v2[Z]) - sproduct(v2[Y], v1[Z]);
	yf = sproduct(v2[X], v1[Z]) - sproduct(v1[X], v2[Z]);
	zf = sproduct(v1[X], v2[Y]) - sproduct(v2[X], v1[Y]);
	m = sqrt(xf * xf + yf * yf + zf * zf);
	if (m == 0) xf = yf = zf = 0;
	else { xf /= m;  yf /= m;  zf /= m; }
	result[X] = float2factor(xf);
	result[Y] = float2factor(yf);
	result[Z] = float2factor(zf);
	return float2scalar(m);
	}

vrl_Scalar vrl_VectorMagnitude(vrl_Vector v)
	{
	float m = sproduct(v[X], v[X]) + sproduct(v[Y], v[Y]) + sproduct(v[Z], v[Z]);
	return float2scalar(sqrt(m));
	}
