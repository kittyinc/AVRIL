/* Object manipulation routines for the AVRIL library */
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

vrl_Object *vrl_ObjectInit(vrl_Object *obj)
	{
	if (obj == NULL) return NULL;
	obj->shape = NULL;  obj->surfmap = NULL;
	vrl_MatrixIdentity(obj->localmat);
	vrl_MatrixIdentity(obj->globalmat);  /* does no harm */
	obj->parent = obj->siblings = obj->children = NULL;
	obj->layer = 0;  obj->invisible = 0;  obj->highlight = 0;
	obj->moved = 1;  obj->fixed = 0;  obj->rotate_box = 1;
	obj->applic_data = NULL;  obj->function = NULL;
	vrl_VectorCopy(obj->minbound, vrl_VectorNULL);
	vrl_VectorCopy(obj->maxbound, vrl_VectorNULL);
	obj->contents = NULL;
	obj->next = NULL;
	obj->forced_rep = NULL;
	obj->name = "No Name";
	vrl_WorldAddObject(obj);
	return obj;
	}

vrl_Object *vrl_ObjectCreate(vrl_Shape *shape)
	{
	vrl_Object *obj = vrl_malloc(sizeof(vrl_Object));
	if (obj == NULL) return NULL;
	vrl_ObjectInit(obj);
	vrl_ObjectSetShape(obj, shape);
	return obj;
	}

void vrl_ObjectMove(vrl_Object *obj, vrl_Scalar x, vrl_Scalar y, vrl_Scalar z)
	{
	vrl_Vector v;
	if (obj == NULL) return;
	if (vrl_ObjectIsFixed(obj)) return;
	vrl_VectorCreate(v, x, y, z);
	vrl_MatrixSetTranslation(obj->localmat, v);
	obj->moved = 1;
	}

void vrl_ObjectRelMove(vrl_Object *obj, vrl_Scalar x, vrl_Scalar y, vrl_Scalar z)
	{
	vrl_Vector v;
	if (obj == NULL) return;
	if (vrl_ObjectIsFixed(obj)) return;
	vrl_VectorCreate(v, x, y, z);
	vrl_MatrixTranslate(obj->localmat, v, 1);
	obj->moved = 1;
	}

void vrl_ObjectRotVector(vrl_Object *obj, vrl_Angle angle, vrl_Vector vector)
	{
	if (obj == NULL) return;
	if (vrl_ObjectIsFixed(obj)) return;
	vrl_MatrixRotVector(obj->localmat, angle, vector, 1);
	obj->moved = 1;
	}

void vrl_ObjectRotReset(vrl_Object *obj)
	{
	if (obj == NULL) return;
	if (vrl_ObjectIsFixed(obj)) return;
	vrl_MatrixResetRotations(obj->localmat);
	obj->moved = 1;
	}

vrl_Object *vrl_ObjectAttach(vrl_Object *obj, vrl_Object *newparent)
	{
	vrl_Matrix tmp;
	vrl_Object *oldparent;
	if (obj == NULL) return NULL;
	oldparent = obj->parent;
	if (vrl_ObjectIsFixed(obj))
		return oldparent;   /* fixed objects can't be attached */
	if (newparent == NULL)
		return oldparent;        /* no parent to attach to */
	if (vrl_ObjectIsFixed(newparent))
		return oldparent;   /* can't attach to a fixed object */
	if (oldparent)
		vrl_ObjectDetach(obj);
	vrl_WorldRemoveObject(obj);
	obj->siblings = newparent->children;
	newparent->children = obj;
	obj->parent = newparent;
	/*
	   Since our parent matrix times our local matrix gives our global
	   matrix, we premultiply by the inverse of the parent matrix to
	   get the local matrix that will produce our existing global matrix.
	   Whew!
	 */
	vrl_MatrixInverse(tmp, newparent->globalmat);
	vrl_MatrixMultiply(obj->localmat, tmp, obj->globalmat);
	obj->moved = 1;
	return oldparent;
	}

vrl_Object *vrl_ObjectDetach(vrl_Object *obj)
	{
	vrl_Object *oldparent;
	if (obj == NULL) return NULL;
	oldparent = obj->parent;
	if (oldparent == NULL) return oldparent;
	if (vrl_ObjectIsFixed(obj)) return oldparent;
	if (obj == oldparent->children)
		oldparent->children = obj->siblings;
	else
		{
		vrl_Object *p;
		for (p = oldparent->children; p; p = p->next)
			if (p->siblings == obj)
				{
				p->siblings = obj->siblings;
				break;
				}
		}
	obj->parent = NULL;  obj->siblings = NULL;
	vrl_WorldAddObject(obj);
	vrl_MatrixCopy(obj->localmat, obj->globalmat);
	obj->moved = 0;
	return oldparent;
	}

/*
   this next routine is based on TransBox, by Jim Arvo
   (published in the first volume of "Graphics Gems")
 */

static void transform_bbox(vrl_Object *object)
	{
	vrl_Vector *minb = &object->shape->minbound;
	vrl_Vector *maxb = &object->shape->maxbound;
	int i, j;
	if (!object->rotate_box && object->shape)
		{
		vrl_VectorAdd(object->minbound, object->shape->minbound, object->globalmat[TRANS]);
		vrl_VectorAdd(object->maxbound, object->shape->maxbound, object->globalmat[TRANS]);
		return;
		}
	vrl_VectorCopy(object->minbound, object->globalmat[TRANS]);
	vrl_VectorCopy(object->maxbound, object->globalmat[TRANS]);
	if (object->shape == NULL) return;
	for (i = 0; i < 3; ++i)
		for (j = 0; j < 3; ++j)
			{
			vrl_Scalar a = vrl_FactorMultiply(object->globalmat[i][j], (*minb)[j]);
			vrl_Scalar b = vrl_FactorMultiply(object->globalmat[i][j], (*maxb)[j]);
			if (a < b)
				{
				object->minbound[i] += a;
				object->maxbound[i] += b;
				}
			else
				{
				object->minbound[i] += b;
				object->maxbound[i] += a;
				}
			}
	}

static vrl_Object *objlist;

static void recursive_updater(vrl_Object *object, int flag)
	{
	while (object)
		{
		if (object->function)
			(*object->function)(object);
		if ((flag || object->moved) && !vrl_ObjectIsFixed(object))
			{
			if (object->parent == NULL)  /* no parent; our localmat *is* our globalmat */
				{
				vrl_MatrixCopy(object->globalmat, object->localmat);
				}
			else if (vrl_ObjectIsFixed(object->parent))  /* same if our parent is fixed */
				{
				vrl_MatrixCopy(object->globalmat, object->localmat);
				}
			else  /* normal case; multiply our parent's globalmat by our localmat to get our globalmat */
				vrl_MatrixMultiply(object->globalmat, object->parent->globalmat, object->localmat);
			transform_bbox(object);
			}
		if (object->children)
			recursive_updater(object->children, flag || object->moved);
		object->moved = 0;
		object->next = objlist;
		objlist = object;
		object = object->siblings;
		}
	}

vrl_Object *vrl_ObjectUpdate(vrl_Object *object)
	{
	if (object == NULL) return NULL;
	objlist = NULL;
	recursive_updater(object, 0);
	return objlist;
	}

void vrl_ObjectTraverse(vrl_Object *object, int (*function)(vrl_Object *obj))
	{
	vrl_Object *obj;
	for (obj = object; obj; obj = obj->siblings)
		{
		if ((*function)(obj))
			return;
		if (obj->children)
			vrl_ObjectTraverse(obj->children, function);
		}
	}

void vrl_ObjectMakeFixed(vrl_Object *object)
	{
	if (object == NULL) return;
	if (vrl_ObjectIsFixed(object)) return;
	vrl_ObjectDetach(object);
	if (object->shape)
		vrl_ShapeTransform(object->globalmat, object->shape);
	vrl_MatrixIdentity(object->globalmat);
	object->moved = 0;
	object->fixed = 1;
	}

void vrl_ObjectMakeMovable(vrl_Object *object)
	{
	vrl_Vector *vec;
	vrl_Shape *shape;
	if (object == NULL) return;
	shape = vrl_ObjectGetShape(object);
	vrl_MatrixIdentity(object->localmat);
	if (shape)
		{
		vrl_Rep *rep = vrl_ShapeGetRep(shape, 0);
		vrl_Vector v;
		vrl_RepGetVertex(rep, 0, v);
		vrl_VectorNegate(v);
		vrl_MatrixSetTranslation(object->localmat, v);
		vrl_ShapeTransform(object->localmat, shape);
		vrl_VectorNegate(v);
		vrl_MatrixSetTranslation(object->localmat, v);
		}
	object->moved = 1;
	object->fixed = 0;
	}

vrl_Object *vrl_ReadObjectPLG(FILE *in)
	{
	vrl_Shape *shape = vrl_ReadPLG(in);
	if (shape == NULL)
		return NULL;
	return vrl_ObjectCreate(shape);
	}

vrl_Object *vrl_ObjectFindRoot(vrl_Object *obj)
	{
	while (obj->parent)
		obj = obj->parent;
	return obj;
	}

void vrl_ObjectDestroy(vrl_Object *object)
	{
	vrl_ObjectDetach(object->children);
	vrl_ObjectDetach(object);
	vrl_free(object);
	}

vrl_Scalar vrl_ObjectComputeDistance(vrl_Object *obj1, vrl_Object *obj2)
	{
	vrl_Vector tmp1, tmp2, tmp3;
	if (obj1 == NULL || obj2 == NULL) return 0;
	vrl_VectorAdd(tmp1, obj1->minbound, obj1->maxbound);
	tmp1[X] /= 2;  tmp1[Y] /= 2;  tmp1[Z] /= 2;
	vrl_VectorAdd(tmp1, obj1->minbound, obj1->maxbound);
	tmp1[X] /= 2;  tmp1[Y] /= 2;  tmp1[Z] /= 2;
	vrl_VectorSub(tmp3, tmp1, tmp2);
	return vrl_VectorMagnitude(tmp3);
	}

vrl_Object *vrl_ObjectCopy(vrl_Object *proto)
	{
	vrl_Object *obj = vrl_malloc(sizeof(vrl_Object));
	if (obj == NULL) return NULL;
	memcpy(obj, proto, sizeof(vrl_Object));
	obj->next = NULL;  obj->contents = NULL;  obj->children = NULL;
	proto->siblings = obj;
	return obj;
	}

void vrl_ObjectRotate(vrl_Object *obj, vrl_Angle angle, int axis, vrl_CoordFrame frame, vrl_Object *relative_to)
	{
	int left = 0;  /* local coordinates */
	if (obj == NULL) return;
	if (vrl_ObjectIsFixed(obj)) return;
	if (axis > 2) axis -= 3;  /* map XROT to X, etc */
	switch (frame)
		{
		case VRL_COORD_OBJREL:
			{
			vrl_Vector basis;
			if (relative_to == NULL) return;
			vrl_MatrixGetBasis(basis, relative_to->globalmat, axis);
			if (obj->parent)
				{
				vrl_Matrix m;
				vrl_Vector v;
				vrl_MatrixInverse(m, obj->parent->globalmat);
				vrl_Transform(v, m, basis);
				vrl_ObjectRotVector(obj, angle, v);
				}
			else
				vrl_ObjectRotVector(obj, angle, basis);
			}
			break;
		case VRL_COORD_WORLD:
			if (obj->parent)
				{
				vrl_Vector *v = &obj->parent->globalmat[axis];
				vrl_ObjectRotVector(obj, angle, *v);
				break;
				}
			/* else fall through */
		case VRL_COORD_PARENT: left = 1;  /* fall through */
		case VRL_COORD_LOCAL:
			switch (axis)
				{
				case X: case XROT: vrl_MatrixRotX(obj->localmat, angle, left); break;
				case Y: case YROT: vrl_MatrixRotY(obj->localmat, angle, left); break;
				case Z: case ZROT: vrl_MatrixRotZ(obj->localmat, angle, left); break;
				default: break;
				}
		default: break;
		}
	obj->moved = 1;
	}

void vrl_ObjectTranslate(vrl_Object *obj, vrl_Vector v, vrl_CoordFrame frame, vrl_Object *relative_to)
	{
	vrl_Vector our_v;
	if (obj == NULL) return;
	if (vrl_ObjectIsFixed(obj)) return;
	vrl_VectorCopy(our_v, v);
	switch (frame)
		{
		case VRL_COORD_OBJREL:
			{
			vrl_Matrix m;
			if (relative_to == NULL) return;
			vrl_MatrixCopy(m, relative_to->globalmat);
			vrl_VectorZero(our_v);
			vrl_MatrixSetTranslation(m, our_v);
			vrl_Transform(our_v, m, v);
			}
			/* fall through */
		case VRL_COORD_WORLD:
			if (obj->parent)
				{
				vrl_Matrix m;
				vrl_Vector vtmp;
				vrl_MatrixInverse(m, obj->parent->globalmat);
				vrl_VectorZero(vtmp);
				vrl_MatrixSetTranslation(m, vtmp);
				vrl_Transform(vtmp, m, our_v);
				vrl_MatrixTranslate(obj->localmat, vtmp, 1);
				break;
				}
			/* else fall through */
		case VRL_COORD_PARENT: vrl_MatrixTranslate(obj->localmat, our_v, 1); break;
		case VRL_COORD_LOCAL: vrl_MatrixTranslate(obj->localmat, our_v, 0); break;
		default: break;
		}
	obj->moved = 1;
	}

void vrl_ObjectLookAt(vrl_Object *obj, vrl_Vector forward, vrl_Vector up)
	{
	vrl_Vector x, y;
	vrl_Matrix mat;
	vrl_VectorCrossproduct(x, up, forward);
	vrl_VectorCrossproduct(y, forward, x);
	vrl_MatrixSetBasis(mat, x, X);	
	vrl_MatrixSetBasis(mat, y, Y);	
	vrl_MatrixSetBasis(mat, forward, Z);	
	vrl_VectorCopy(&mat[3], obj->globalmat[3]);
	if (obj->parent)
		{
		/*
		   Since our parent matrix times our local matrix gives our global
		   matrix, we premultiply by the inverse of the parent matrix to
		   get the local matrix that will produce our newly-computed global
		   matrix.  Whew!
		 */
		vrl_Matrix tmp;
		vrl_MatrixInverse(tmp, obj->parent->globalmat);
		vrl_MatrixMultiply(obj->localmat, tmp, mat);
		}
	else
		vrl_MatrixCopy(obj->localmat, mat);
	obj->moved = 1;
	}
