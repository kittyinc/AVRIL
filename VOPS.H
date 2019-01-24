/* Macro versions of some of the routines in matrix.c; part of AVRIL */

/* Not sure yet if the performance improvement is worth it */
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

extern void vrl_InternalMatmult3x3(vrl_Matrix result, vrl_Matrix m1, vrl_Matrix m2);

#define vrl_VectorCreate(result, x, y, z) \
	{ \
	(result)[X] = (x); \
	(result)[Y] = (y); \
	(result)[Z] = (z); \
	}

#define vrl_VectorCopy(destination, source) \
	memcpy((destination), (source), sizeof(vrl_Vector))

#ifdef INLINE_ADD_SUB
#define vrl_VectorAdd(result, v1, v2) \
	{ \
	register vrl_Vector *vp1 = (vrl_Vector *) &(v1), *vp2 = (vrl_Vector *) &(v2), *rp = (vrl_Vector *) &(result); \
	(*rp)[X] = (*vp1)[X] + (*vp2)[X]; \
	(*rp)[Y] = (*vp1)[Y] + (*vp2)[Y]; \
	(*rp)[Z] = (*vp1)[Z] + (*vp2)[Z]; \
	}

#define vrl_VectorSub(result, v1, v2) \
	{ \
	register vrl_Vector *vp1 = (vrl_Vector *) &(v1), *vp2 = (vrl_Vector *) &(v2), *rp = (vrl_Vector *) &(result); \
	(*rp)[X] = (*vp1)[X] - (*vp2)[X]; \
	(*rp)[Y] = (*vp1)[Y] - (*vp2)[Y]; \
	(*rp)[Z] = (*vp1)[Z] - (*vp2)[Z]; \
	}
#endif

#define vrl_MatrixResetRotations(m) \
	{ \
	memset((m), 0, 9 * sizeof(vrl_Factor)); \
	(m)[0][0] = (m)[1][1] = (m)[2][2] = VRL_UNITY; \
	}

#define vrl_MatrixIdentity(m) \
	{ \
	register vrl_Matrix *mp = (vrl_Matrix *) &(m); \
	memset(mp, 0, 12 * sizeof(vrl_Factor)); \
	(*mp)[0][0] = (*mp)[1][1] = (*mp)[1][1] = VRL_UNITY; \
	}

#define vrl_MatrixCopy(result, m) \
	memcpy((result), (m), sizeof(vrl_Matrix))

#define vrl_MatrixInverse(result, m) \
	{ \
	(result)[0][0] = (m)[0][0];  (result)[0][1] = (m)[1][0];  (result)[0][2] = (m)[2][0]; \
	(result)[1][0] = (m)[0][1];  (result)[1][1] = (m)[1][1];  (result)[1][2] = (m)[2][1]; \
	(result)[2][0] = (m)[0][2];  (result)[2][1] = (m)[1][2];  (result)[2][2] = (m)[2][2]; \
	(result)[TRANS][X] = -vrl_VectorDotproduct((result)[X], (m)[TRANS]); \
	(result)[TRANS][Y] = -vrl_VectorDotproduct((result)[Y], (m)[TRANS]); \
	(result)[TRANS][Z] = -vrl_VectorDotproduct((result)[Z], (m)[TRANS]); \
	}

#define vrl_TransformX(m, v) \
	(vrl_VectorDotproduct((m)[X], (v)) + (m)[TRANS][X])

#define vrl_TransformY(m, v) \
	(vrl_VectorDotproduct((m)[Y], (v)) + (m)[TRANS][Y])

#define vrl_TransformZ(m, v) \
	(vrl_VectorDotproduct((m)[Z], (v)) + (m)[TRANS][Z])

#define vrl_Transform(result, m, v) \
	{ \
	(result)[X] = vrl_TransformX((m), (v)); \
	(result)[Y] = vrl_TransformY((m), (v)); \
	(result)[Z] = vrl_TransformZ((m), (v)); \
	}

#define vrl_MatrixMultiply(result, m1, m2) \
	{ \
	vrl_InternalMatmult3x3((result), (m1), (m2)); \
	vrl_Transform((result)[TRANS], (m1), (m2)[TRANS]); \
	}

