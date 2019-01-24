/* Routines for creating simple geometric primitives; part of AVRIL */
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
#include <math.h>    /* sin(), cos() */

#define PI 3.14159262

static vrl_Surface defsurf = { VRL_SURF_FLAT, 1, 255 };

static vrl_Facet *newfacet(int n, int *pts)
	{
	vrl_Facet *facet = vrl_FacetCreate(n);
	if (facet == NULL) return NULL;
	memcpy(facet->points, pts, n * sizeof(int));
	return facet;
	}

static int cubepoints[6][4] =
	{
	{ 0, 4, 5, 1 }, { 4, 7, 6, 5 }, { 2, 6, 7, 3 },
	{ 1, 5, 6, 2 }, { 3, 7, 4, 0 }, { 0, 1, 2, 3 }
	};

vrl_Shape *vrl_PrimitiveBox(vrl_Scalar width, vrl_Scalar height, vrl_Scalar depth, vrl_Surfacemap *map)
	{
	int i;
	vrl_Vector *verts;
	vrl_Shape *shape = vrl_ShapeCreate();
	vrl_Rep *rep = vrl_RepCreate(8, 0);
	if (shape == NULL || rep == NULL) return NULL;
	shape->replist = rep;
	if (map)
		shape->default_surfacemap = map;
	else
		{
		shape->default_surfacemap = vrl_SurfacemapCreate(1);
		vrl_SurfacemapSetSurface(shape->default_surfacemap, 0, &defsurf);
		}
	vrl_RepSetSorting(rep, VRL_SORT_NONE);
	verts = rep->vertices;
	verts[0][X] = verts[3][X] = verts[4][X] = verts[7][X] = -width/2;
	verts[1][X] = verts[2][X] = verts[5][X] = verts[6][X] = width/2;
	verts[0][Y] = verts[1][Y] = verts[2][Y] = verts[3][Y] = -height/2;
	verts[4][Y] = verts[5][Y] = verts[6][Y] = verts[7][Y] = height/2;
	verts[0][Z] = verts[1][Z] = verts[4][Z] = verts[5][Z] = -depth/2;
	verts[2][Z] = verts[3][Z] = verts[6][Z] = verts[7][Z] = depth/2;
	rep->facets = NULL;
	for (i = 0; i < 6; ++i)
		{
		vrl_Facet *facet = newfacet(4, cubepoints[i]);
		if (facet == NULL) return NULL;
		facet->farside = rep->facets;
		rep->facets = facet;
		}
	vrl_ShapeUpdate(shape);
	return shape;
	}

static int prismpoints[5][4] =
	{
	{ 5, 4, 3, 2 }, { 2, 3, 0, 1 }, { 4, 5, 1, 0 },
	{ 1, 5, 2, 0 }, { 4, 0, 3, 0 }
	};

vrl_Shape *vrl_PrimitivePrism(vrl_Scalar width, vrl_Scalar height, vrl_Scalar depth, vrl_Surfacemap *map)
	{
	int i;
	vrl_Vector *verts;
	vrl_Shape *shape = vrl_ShapeCreate();
	vrl_Rep *rep = vrl_RepCreate(6, 0);
	if (shape == NULL || rep == NULL) return NULL;
	shape->replist = rep;
	if (map)
		shape->default_surfacemap = map;
	else
		{
		shape->default_surfacemap = vrl_SurfacemapCreate(1);
		vrl_SurfacemapSetSurface(shape->default_surfacemap, 0, &defsurf);
		}
	vrl_RepSetSorting(rep, VRL_SORT_NONE);
	verts = rep->vertices;
	verts[0][X] = verts[3][X] = verts[4][X] = 0;
	verts[1][X] = verts[2][X] = verts[5][X] = width;
	verts[0][Y] = verts[1][Y] = verts[2][Y] = verts[3][Y] = 0;
	verts[4][Y] = verts[5][Y] = height;
	verts[0][Z] = verts[1][Z] = verts[4][Z] = verts[5][Z] = 0;
	verts[2][Z] = verts[3][Z] = depth;
	rep->facets = NULL;
	for (i = 0; i < 5; ++i)
		{
		vrl_Facet *facet = newfacet((i < 3) ? 4 : 3, prismpoints[i]);
		if (facet == NULL) return NULL;
		facet->farside = rep->facets;
		rep->facets = facet;
		}
	vrl_ShapeUpdate(shape);
	return shape;
	}

vrl_Shape *vrl_PrimitiveCone(vrl_Scalar radius, vrl_Scalar height, int nsides, vrl_Surfacemap *map)
	{
	int i;
	vrl_Vector *verts, *norms;
	vrl_Shape *shape = vrl_ShapeCreate();
	vrl_Rep *rep = vrl_RepCreate(nsides + 1, 1);
	vrl_Facet *facet;
	if (nsides < 3) nsides = 3;
	if (shape == NULL || rep == NULL) return NULL;
	shape->replist = rep;
	if (map)
		shape->default_surfacemap = map;
	else
		{
		shape->default_surfacemap = vrl_SurfacemapCreate(1);
		vrl_SurfacemapSetSurface(shape->default_surfacemap, 0, &defsurf);
		}
	vrl_RepSetSorting(rep, VRL_SORT_NONE);
	verts = rep->vertices;  norms = rep->normals;
	verts[0][X] = verts[0][Z] = 0;  verts[0][Y] = height;
	norms[0][X] = 0;  norms[0][Z] = 0;  norms[0][Y] = VRL_UNITY;
	for (i = 1; i <= nsides; ++i)
		{
		verts[i][X] = radius * cos((i*2*PI)/nsides);
		verts[i][Y] = 0;
		verts[i][Z] = radius * sin((i*2*PI)/nsides);
		norms[i][X] = float2factor(cos((i*2*PI)/nsides));
		norms[i][Y] = 0;
		norms[i][Z] = float2factor(sin((i*2*PI)/nsides));
		}
	rep->facets = NULL;
	for (i = 1; i <= nsides; ++i)
		{
		int pts[3];
		pts[0] = i;  pts[1] = 0;  pts[2] = (i == nsides) ? 1 : (i + 1);
		facet = newfacet(3, pts);
		if (facet == NULL) return NULL;
		facet->farside = rep->facets;
		rep->facets = facet;
		}
	facet = vrl_malloc(sizeof(vrl_Facet));
	if (facet == NULL) return NULL;
	facet->farside = rep->facets;
	rep->facets = facet;
	facet->nearside = NULL;  facet->details = NULL;
	facet->interior = facet->highlight = 0;
	facet->surface = 0;  facet->id = 0;
	facet->npoints = nsides;
	facet->points = vrl_calloc(nsides, sizeof(int));
	if (facet->points == NULL) return NULL;
	facet->edges = NULL;
	for (i = 0; i < nsides; ++i)
		facet->points[i] = i+1;
	vrl_ShapeUpdate(shape);
	return shape;
	}

vrl_Shape *vrl_PrimitiveCylinder(vrl_Scalar bottom_radius, vrl_Scalar top_radius, vrl_Scalar height, int nsides, vrl_Surfacemap *map)
	{
	int i;
	vrl_Vector *verts, *norms;
	vrl_Shape *shape = vrl_ShapeCreate();
	vrl_Rep *rep;
	vrl_Facet *facet;
	if (top_radius == 0)
		return vrl_PrimitiveCone(bottom_radius, height, nsides, map);
	if (nsides < 3) nsides = 3;
	rep = vrl_RepCreate(nsides * 2, 1);
	if (shape == NULL || rep == NULL) return NULL;
	shape->replist = rep;
	if (map)
		shape->default_surfacemap = map;
	else
		{
		shape->default_surfacemap = vrl_SurfacemapCreate(1);
		vrl_SurfacemapSetSurface(shape->default_surfacemap, 0, &defsurf);
		}
	vrl_RepSetSorting(rep, VRL_SORT_NONE);
	verts = rep->vertices;  norms = rep->normals;
	for (i = 0; i < nsides; ++i)
		{
		verts[i][X] = bottom_radius * cos((i*2*PI)/nsides);
		verts[i][Y] = 0;
		verts[i][Z] = bottom_radius * sin((i*2*PI)/nsides);
		norms[i][X] = float2factor(cos((i*2*PI)/nsides));
		norms[i][Y] = 0;
		norms[i][Z] = float2factor(sin((i*2*PI)/nsides));
		verts[i+nsides][X] = top_radius * cos((i*2*PI)/nsides);
		verts[i+nsides][Y] = height;
		verts[i+nsides][Z] = top_radius * sin((i*2*PI)/nsides);
		norms[i+nsides][X] = float2factor(cos((i*2*PI)/nsides));
		norms[i+nsides][Y] = 0;
		norms[i+nsides][Z] = float2factor(sin((i*2*PI)/nsides));
		}
	rep->facets = NULL;
	for (i = 0; i < nsides; ++i)
		{
		int pts[4];
		pts[0] = i;  pts[1] = nsides+i;
		if (i == (nsides-1))
			{
			pts[2] = nsides;
			pts[3] = 0;
			}
		else
			{
			pts[2] = nsides + i + 1;
			pts[3] = i + 1;
			}
		facet = newfacet(4, pts);
		if (facet == NULL) return NULL;
		facet->farside = rep->facets;
		rep->facets = facet;
		}
	facet = vrl_malloc(sizeof(vrl_Facet));
	if (facet == NULL) return NULL;
	facet->farside = rep->facets;
	rep->facets = facet;
	facet->nearside = NULL;  facet->details = NULL;
	facet->interior = facet->highlight = 0;
	facet->surface = 0;  facet->id = 0;
	facet->npoints = nsides;
	facet->points = vrl_calloc(nsides, sizeof(int));
	if (facet->points == NULL) return NULL;
	facet->edges = NULL;
	for (i = 0; i < nsides; ++i)
		facet->points[i] = i;
	facet = vrl_malloc(sizeof(vrl_Facet));
	if (facet == NULL) return NULL;
	facet->farside = rep->facets;
	rep->facets = facet;
	facet->nearside = NULL;  facet->details = NULL;
	facet->interior = facet->highlight = 0;
	facet->surface = 0;  facet->id = 0;
	facet->npoints = nsides;
	facet->points = vrl_calloc(nsides, sizeof(int));
	if (facet->points == NULL) return NULL;
	facet->edges = NULL;
	for (i = 0; i < nsides; ++i)
		facet->points[i] = nsides - i - 1 + nsides;
	vrl_ShapeUpdate(shape);
	return shape;
	}

vrl_Shape *vrl_PrimitiveSphere(vrl_Scalar radius, int vsides, int hsides, vrl_Surfacemap *map)
	{
	int i, j;
	vrl_Vector *verts, *norms;
	vrl_Shape *shape = vrl_ShapeCreate();
	vrl_Rep *rep;
	vrl_Facet *facet;
	if (vsides < 3) vsides = 3;
	if (hsides < 3) hsides = 3;
	rep = vrl_RepCreate((vsides - 1) * hsides + 2, 1);
	if (shape == NULL || rep == NULL) return NULL;
	shape->replist = rep;
	if (map)
		shape->default_surfacemap = map;
	else
		{
		shape->default_surfacemap = vrl_SurfacemapCreate(1);
		vrl_SurfacemapSetSurface(shape->default_surfacemap, 0, &defsurf);
		}
	vrl_RepSetSorting(rep, VRL_SORT_NONE);
	verts = rep->vertices;  norms = rep->normals;
	/* create poles */
	verts[0][X] = verts[0][Z] = verts[1][X] = verts[1][Z] = 0;
	norms[0][X] = norms[0][Z] = norms[1][X] = norms[1][Z] = 0;
	verts[0][Y] = radius;  verts[1][Y] = -radius;
	norms[0][Y] = VRL_UNITY;  norms[1][Y] = -VRL_UNITY;
	/* create latitudes and longitudes */
	for (i = 1; i < vsides; ++i)
		{
		vrl_Scalar r =  radius * sin((i * PI) / vsides);
		vrl_Scalar y = -radius * cos((i * PI) / vsides);
		for (j = 0; j < hsides; ++j)
			{
			verts[2+(i-1)*hsides+j][X] = r * cos((j*2*PI)/hsides);
			verts[2+(i-1)*hsides+j][Y] = y;
			verts[2+(i-1)*hsides+j][Z] = r * sin((j*2*PI)/hsides);
			norms[2+(i-1)*hsides+j][X] = r * cos((j*2*PI)/hsides);
			norms[2+(i-1)*hsides+j][Y] = y;
			norms[2+(i-1)*hsides+j][Z] = r * sin((j*2*PI)/hsides);
			vrl_VectorNormalize(norms[2+(i-1)*hsides+j]);
			}
		}
	rep->facets = NULL;
	for (i = 1; i < vsides-1; ++i)
		for (j = 0; j < hsides; ++j)
			{
			int pts[4];
			int n = (i-1) * hsides + j + 2;
			pts[0] = n;  pts[1] = n + hsides;
			if (j == (hsides - 1))
				{
				pts[2] = (i) * hsides + 2;
				pts[3] = (i-1) * hsides + 2;
				}
			else
				{
				pts[2] = n + 1 + hsides;
				pts[3] = n + 1;
				}
			facet = newfacet(4, pts);
			facet->farside = rep->facets;
			rep->facets = facet;
			}
	/* Triangular polar regions */
	for (i = 0; i < hsides; ++i)
		{
		int pts[3];
		/* north */
		pts[0] = 1;  pts[1] = i + 2;
		pts[2] = ((i == (hsides - 1)) ? 0 : (i + 1)) + 2;
		facet = newfacet(3, pts);
		facet->farside = rep->facets;
		rep->facets = facet;
		/* south */
		pts[0] = rep->nvertices - i - 1;  pts[1] = 0;
		pts[2] = (i == 0) ? (rep->nvertices - hsides) : (pts[0] + 1);
		facet = newfacet(3, pts);
		facet->farside = rep->facets;
		rep->facets = facet;
		}
	vrl_ShapeUpdate(shape);
	return shape;
	}
