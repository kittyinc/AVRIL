/* Shape updating routines; part of AVRIL */
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
#include <string.h>  /* stricmp() */
#include <math.h>    /* sqrt() */

static vrl_Shape *shapelist = NULL;

vrl_Shape *vrl_ShapeInit(vrl_Shape *shape)
	{
	if (shape == NULL) return NULL;
	vrl_VectorZero(shape->center);
	vrl_VectorZero(shape->minbound);
	vrl_VectorZero(shape->maxbound);
	shape->radius = 0;
	shape->default_surfacemap = NULL;
	shape->replist = NULL;
	shape->name = "No Name";
	shape->next = shapelist;
	shapelist = shape;
	return shape;
	}

void vrl_FacetComputeNormal(vrl_Facet *facet, vrl_Vector *vertices)
	{
	vrl_Scalar maxmag = 0;
	int i;
	facet->normal[X] = VRL_UNITY;  facet->normal[Y] = facet->normal[Z] = 0;
	if (facet->npoints < 3)
		return;
	for (i = 0; i < facet->npoints; ++i)
		{
		vrl_Vector v1, v2, tmp;
		vrl_Scalar m;
		int first = facet->points[i];
		int second = facet->points[(i+1) % facet->npoints];
		int third = facet->points[(i+2) % facet->npoints];
		vrl_VectorSub(v1, vertices[third], vertices[second]);
		vrl_VectorSub(v2, vertices[first], vertices[second]);
		m = vrl_VectorCrossproduct(tmp, v1, v2);
		if (m > maxmag)
			{
			maxmag = m;
			vrl_VectorCopy(facet->normal, tmp);
			}
		}
	}

void vrl_ShapeComputeBounds(vrl_Shape *shape)
	{
	vrl_Scalar minx, maxx, miny, maxy, minz, maxz;  /* bounding box */
	vrl_Vector farthest_point;  /* vector from center to farthest point */
	vrl_Scalar radius;          /* proportional to magnitude of farthest_point */
	vrl_Rep *rep;
	if (shape == NULL) return;
	if (shape->replist == NULL) return;
	minx = maxx = shape->replist->vertices[0][X];
	miny = maxy = shape->replist->vertices[0][Y];
	minz = maxz = shape->replist->vertices[0][Z];
	for (rep = shape->replist; rep; rep = rep->next)
		{
		int i;
		for (i = 0; i < rep->nvertices; ++i)
			{
			if (rep->vertices[i][X] < minx) minx = rep->vertices[i][X];
			if (rep->vertices[i][Y] < miny) miny = rep->vertices[i][Y];
			if (rep->vertices[i][Z] < minz) minz = rep->vertices[i][Z];
			if (rep->vertices[i][X] > maxx) maxx = rep->vertices[i][X];
			if (rep->vertices[i][Y] > maxy) maxy = rep->vertices[i][Y];
			if (rep->vertices[i][Z] > maxz) maxz = rep->vertices[i][Z];
			}
		}
	/* store the box's bounds */
	shape->minbound[X] = minx;  shape->minbound[Y] = miny;  shape->minbound[Z] = minz;
	shape->maxbound[X] = maxx;  shape->maxbound[Y] = maxy;  shape->maxbound[Z] = maxz;
	/* center of bounding sphere is center of bounding box */
	shape->center[X] = (maxx - minx) / 2 + minx;
	shape->center[Y] = (maxy - miny) / 2 + miny;
	shape->center[Z] = (maxz - minz) / 2 + minz;
	/* not the smartest bounding sphere in the world, but it works */
	vrl_VectorSub(farthest_point, shape->maxbound, shape->center);
	shape->radius = vrl_VectorMagnitude(farthest_point);
	}

static void recursive_recompute_normals(vrl_Facet *facet, vrl_Vector *vertices)
	{
	if (facet->nearside) recursive_recompute_normals(facet->nearside, vertices);
	if (facet->farside) recursive_recompute_normals(facet->farside, vertices);
	vrl_FacetComputeNormal(facet, vertices);
	}

static void linear_recompute_normals(vrl_Facet *facet, vrl_Vector *vertices)
	{
	vrl_Facet *f;
	for (f = facet; f; f = f->farside)
		vrl_FacetComputeNormal(f, vertices);
	}

void vrl_ShapeUpdate(vrl_Shape *shape)
	{
	vrl_Rep *rep;
	vrl_Facet *facet;
	for (rep = shape->replist; rep; rep = rep->next)
		if (rep->sorttype == VRL_SORT_OTHER)  /* probably a BSP tree */
			recursive_recompute_normals(rep->facets, rep->vertices);
		else
			linear_recompute_normals(rep->facets, rep->vertices);
	vrl_ShapeComputeBounds(shape);
	}

void vrl_ShapeTransform(vrl_Matrix m, vrl_Shape *shape)
	{
	vrl_Rep *rep;
	for (rep = shape->replist; rep; rep = rep->next)
		{
		int i;
		for (i = 0; i < rep->nvertices; ++i)
			{
			vrl_Vector v;
			vrl_Transform(v, m, rep->vertices[i]);
			v[X] = vrl_ScalarRound(v[X]);
			v[Y] = vrl_ScalarRound(v[Y]);
			v[Z] = vrl_ScalarRound(v[Z]);
			vrl_VectorCopy(rep->vertices[i], v);
			}
		}
	vrl_ShapeUpdate(shape);
	}

vrl_Rep *vrl_ShapeGetRep(vrl_Shape *shape, vrl_Scalar size)
	{
	vrl_Rep *rep;
	for (rep = shape->replist; rep; rep = rep->next)
		if (size > rep->size)
			break;
	return rep;
	}

vrl_Facet *vrl_FacetInit(vrl_Facet *facet, int npts)
	{
	if (facet == NULL) return NULL;
	facet->surface = 0;  facet->id = 0;
	facet->highlight = 0;  facet->interior = 0;
	facet->details = NULL;  facet->nearside = facet->farside = NULL;
	facet->npoints = npts;
	facet->edges = NULL;
	if (npts)
		{
		facet->points = vrl_calloc(npts, sizeof(int));
		if (facet->points == NULL) return NULL;
		}
	return facet;
	}

void vrl_FacetTraverse(vrl_Facet *facet, int (*function)(vrl_Facet *f))
	{
	if (facet->nearside)
		vrl_FacetTraverse(facet->nearside, function);
	if ((*function)(facet))
		return;
	if (facet->farside)
		vrl_FacetTraverse(facet->farside, function);
	}

void vrl_ShapeAddRep(vrl_Shape *shape, vrl_Rep *rep, vrl_Scalar size)
	{
	vrl_Rep *p = shape->replist;
	rep->size = size;
	if (p == NULL)  /* rep is the only one */
		{
		rep->next = NULL;
		shape->replist = rep;
		return;
		}
	if (rep->size > p->size)  /* rep is bigger than biggest one */
		{
		rep->next = p;
		shape->replist = rep;
		return;
		}
	for (p = shape->replist; p->next; p = p->next)
		if (rep->size > p->next->size)  /* we're bigger than p->next */
			{
			rep->next = p->next;
			p->next = rep;
			return;
			}
	/* if we make it this far, we're the smallest rep so far */
	rep->next = NULL;
	p->next = rep;
	}

void vrl_ShapeTraverseReps(vrl_Shape *shape, int (*function)(vrl_Rep *rep))
	{
	vrl_Rep *r;
	for (r = shape->replist; r; r = r->next)
		if ((*function)(r))
			return;
	}

void vrl_RepTraverseVertices(vrl_Rep *rep, int (*function)(vrl_Vector *v, vrl_Vector *n))
	{
	int i;
	for (i = 0; i < rep->nvertices; ++i)
		if ((*function)(&rep->vertices[i], rep->normals ? &rep->normals[i] : NULL))
			return;
	}

static void recursive_traversal(vrl_Facet *facet, int (*function)(vrl_Facet *f))
	{
	if (facet->nearside) recursive_traversal(facet->nearside, function);
	(*function)(facet);
	if (facet->farside) recursive_traversal(facet->farside, function);
	}

vrl_Rep *vrl_RepInit(vrl_Rep *rep, int nvertices, vrl_Boolean has_normals)
	{
	if (rep == NULL) return NULL;
	rep->size = 0;  rep->sorttype = VRL_SORT_FARTHEST; 
	rep->nvertices = nvertices;  rep->vertices = vrl_calloc(nvertices, sizeof(vrl_Vector));
	if (rep->vertices == NULL) return NULL;
	if (has_normals)
		rep->normals = vrl_calloc(nvertices, sizeof(vrl_Vector));
	else
		rep->normals = NULL;
	rep->facets = NULL;  rep->next = NULL;
	rep->edges = NULL;
	return rep;
	}

void vrl_RepAddFacet(vrl_Rep *rep, vrl_Facet *facet)
	{
	facet->farside = rep->facets;
	rep->facets = facet;
	}

void vrl_RepTraverseFacets(vrl_Rep *rep, int (*function)(vrl_Facet *f))
	{
	if (rep->facets == NULL) return;
	if (rep->sorttype == VRL_SORT_OTHER)
		recursive_traversal(rep->facets, function);
	else  /* linear traversal */
		{
		vrl_Facet *f;
		for (f = rep->facets; f; f = f->farside)
			if ((*function)(f))
				return;
		}
	}

vrl_Facet *vrl_RepGetFacet(vrl_Rep *rep, int n)
	{
	int i = 0;
	vrl_Facet *f;
	for (f = rep->facets; f; f = f->farside)
		if (i++ == n)
			break;
	return f;
	}

static float normal_x, normal_y, normal_z;
static int vertexnum;

static int addnormals(vrl_Facet *f)
	{
	vrl_Vector v;
	int n = vrl_FacetCountPoints(f), i;
	for (i = 0; i < n; ++i)
		if (vrl_FacetGetPoint(f, i) == vertexnum)
			break;
	if (i >= n) return 0;
	vrl_FacetGetNormal(f, v);
	normal_x += factor2float(v[X]);
	normal_y += factor2float(v[Y]);
	normal_z += factor2float(v[Z]);
	return 0;
	}

void vrl_RepComputeVertexNormals(vrl_Rep *rep)
	{
	int i;
	if (rep->normals == NULL)
		rep->normals = vrl_calloc(rep->nvertices, sizeof(vrl_Vector));
	for (vertexnum = 0; vertexnum < rep->nvertices; ++vertexnum)
		{
		int j;
		float mag;
		/* add up and count all the facet normals for facets sharing this vertex */
		normal_x = normal_y = normal_z = 0;
		vrl_RepTraverseFacets(rep, addnormals);
		mag = sqrt(normal_x * normal_x + normal_y * normal_y + normal_z * normal_z);
		if (mag)
			{
			rep->normals[vertexnum][X] = float2factor(normal_x / mag);
			rep->normals[vertexnum][Y] = float2factor(normal_y / mag);
			rep->normals[vertexnum][Z] = float2factor(normal_z / mag);
			}
		else
			{
			rep->normals[vertexnum][X] = 0;
			rep->normals[vertexnum][Y] = 0;
			rep->normals[vertexnum][Z] = 0;
			}
		}
	}

static int fcount;

static int facet_counter(vrl_Facet *f)
	{
	if (f)
		++fcount;
	return 0;
	}

int vrl_RepCountFacets(vrl_Rep *rep)
	{
	fcount = 0;
	vrl_RepTraverseFacets(rep, facet_counter);
	return fcount;
	}

int vrl_ShapeCountReps(vrl_Shape *shape)
	{
	int n = 0;
	vrl_Rep *rep;
	for (rep = shape->replist; rep; rep = rep->next)
		++n;
	return n;
	}

void vrl_ShapeRescale(vrl_Shape *shape, float sx, float sy, float sz)
	{
	vrl_Rep *rep;
	int i;
	if (shape == NULL) return;
	for (rep = shape->replist; rep; rep = rep->next)
		for (i = 0; i < rep->nvertices; ++i)
			{
			rep->vertices[i][X] *= sx;
			rep->vertices[i][Y] *= sy;
			rep->vertices[i][Z] *= sz;
			}
	vrl_ShapeUpdate(shape);
	}

void vrl_ShapeOffset(vrl_Shape *shape, vrl_Scalar tx, vrl_Scalar ty, vrl_Scalar tz)
	{
	vrl_Rep *rep;
	int i;
	if (shape == NULL) return;
	for (rep = shape->replist; rep; rep = rep->next)
		for (i = 0; i < rep->nvertices; ++i)
			{
			rep->vertices[i][X] += tx;
			rep->vertices[i][Y] += ty;
			rep->vertices[i][Z] += tz;
			}
	vrl_ShapeUpdate(shape);
	}

static int find_id;
static vrl_Facet *found_facet;

static int find_facet(vrl_Facet *facet)
	{
	if (facet)
		if (facet->id == find_id)
			{
			found_facet = facet;
			return 1;
			}
	return 0;
	}

vrl_Facet *vrl_RepFindFacet(vrl_Rep *rep, unsigned int id)
	{
	find_id = id;
	found_facet = NULL;
	vrl_RepTraverseFacets(rep, find_facet);
	return found_facet;
	}

vrl_Shape *vrl_ShapeFind(char *name)
	{
	vrl_Shape *p;
	for (p = shapelist; p; p = p->next)
		if (!stricmp(name, p->name))
			return p;
	return NULL;
	}

vrl_Shape *vrl_ShapeGetList(void)
	{
	return shapelist;
	}

void vrl_ShapeComputeVertexNormals(vrl_Shape *shape)
	{
	vrl_Rep *rep;
	for (rep = shape->replist; rep; rep = rep->next)
		vrl_RepComputeVertexNormals(rep);
	}
