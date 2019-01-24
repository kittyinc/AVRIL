/* Read a PLG file into a Shape; part of AVRIL */
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
#include <stdlib.h>  /* strtoul(), atoi() */
#include <string.h>
#include <ctype.h>

/* maximum number of *unique* surface descriptors per object: */
#define MAX_SURFS_PER_SHAPE 200

static int nlines = 0;

static char *seps = " \t";  /* strtok() field separators are spaces and tabs */

static float sx = 1, sy = 1, sz = 1, tx = 0, ty = 0, tz = 0;

static int multi;  /* set while reading multi-rep PLG files */

void vrl_SetReadPLGscale(float x, float y, float z)
	{
	sx = x;  sy = y;  sz = z;
	}

void vrl_SetReadPLGoffset(float x, float y, float z)
	{
	tx = x;  ty = y;  tz = z;
	}

int getline(char *buff, int n, FILE *stream)
	{
	char *p;
	do
		{
		if (fgets(buff, n, stream) == NULL) return 0;
		++nlines;
		if ((p = strchr(buff, '\n')) != NULL) *p = '\0';  /* strip newline */
		if (!stricmp(buff, "##MULTI")) multi = 1;
		if ((p = strchr(buff, '#')) != NULL) *p = '\0';   /* strip comments */
		for (p = buff; *p && isspace(*p); ++p) ;
		} while (*p == '\0');
	return 1;
	}

#define default_surfdesc 0x0001

static unsigned int map_ascii_surface_name(char *buff)
	{
	char *p = strtok(buff, "_");
	unsigned int value;
	unsigned char hue, brightness;
	if (p == NULL) return default_surfdesc;  /* no string found; return color 1 */
	if (!stricmp(p, "indexed"))
		{
		p = strtok(NULL, "_");
		if (p) return 0x8000 | strtoul(p, NULL, 0);
		return default_surfdesc;
		}
	if (!stricmp(p, "shaded"))
		{
		p = strtok(NULL, "_");
		if (p == NULL) return default_surfdesc;
		hue = strtoul(p, NULL, 0) & 0x0F;
		p = strtok(NULL, "");
		if (p == NULL) return default_surfdesc;
		brightness = strtoul(p, NULL, 0) & 0xFF;
		return 0x1000 | (hue << 8) | brightness;
		}
	if (!stricmp(p, "metal"))
		{
		p = strtok(NULL, "_");
		if (p == NULL) return default_surfdesc;
		hue = strtoul(p, NULL, 0) & 0x0F;
		p = strtok(NULL, "");
		if (p == NULL) return default_surfdesc;
		brightness = strtoul(p, NULL, 0) & 0xFF;
		return 0x2000 | (hue << 8) | ((brightness & 0x1F) << 3);
		}
	if (!stricmp(p, "glass"))
		{
		p = strtok(NULL, "_");
		if (p == NULL) return default_surfdesc;
		hue = strtoul(p, NULL, 0) & 0x0F;
		p = strtok(NULL, "");
		if (p == NULL) return default_surfdesc;
		brightness = strtoul(p, NULL, 0) & 0xFF;
		return 0x3000 | (hue << 8) | ((brightness & 0x1F) << 3);
		}
	return default_surfdesc;
	}

vrl_Shape *vrl_ReadPLG(FILE *in)
	{
	char buff[100];
	vrl_Rep *lastrep = NULL;
	vrl_Surface *tempsurfs[MAX_SURFS_PER_SHAPE];
	int nsurfs_defined = 0, got_header = 0;
	int i;
	vrl_Shape *shape = NULL;
	nlines = 0;	
	multi = 0;
	while (getline(buff, sizeof(buff), in))  /* could read header line */
		{
		int nv, nf;
		char name[100], *p;
		vrl_Facet *parent = NULL;
		vrl_Rep *rep;
		got_header = 1;
		if (shape == NULL)
			{
			shape = vrl_ShapeCreate();
			if (shape == NULL)
				return NULL;
			}
		rep = vrl_malloc(sizeof(vrl_Rep));
		if (rep == NULL)
			return NULL;
		rep->next = NULL;
		if (lastrep) lastrep->next = rep;
		else shape->replist = rep;
		lastrep = rep;
		if (sscanf(buff, "%s %d %d %*d %d", name, &nv, &nf, &rep->sorttype) < 5)
			{
			if (nf < 6)   /* objects with fewer than 6 facets are almost always convex */
				rep->sorttype = VRL_SORT_NONE;
			else
				rep->sorttype = VRL_SORT_FARTHEST;
			}
		rep->size = 0;
		if ((p = strchr(name, '_')) != NULL)
			if (isdigit(p[1]))
				rep->size = atoi(&p[1]);
		rep->nvertices = nv;
		rep->vertices = vrl_calloc(nv, sizeof(vrl_Vector));
		if (rep->vertices == NULL)
			return NULL;
		rep->edges = NULL;
		rep->normals = NULL;
		rep->facets = NULL;
		for (i = 0; i < nv; ++i)
			{
			float x, y, z, nx, ny, nz;
			getline(buff, sizeof(buff), in);
			if (sscanf(buff, "%f %f %f %f %f %f", &x, &y, &z, &nx, &ny, &nz) == 6)
				{
				if (rep->normals == NULL)
					rep->normals = vrl_calloc(nv, sizeof(vrl_Vector));
				if (rep->normals)
					{
					rep->normals[i][X] = float2factor(nx);
					rep->normals[i][Y] = float2factor(ny);
					rep->normals[i][Z] = float2factor(nz);
					}
				}
			rep->vertices[i][X] = float2scalar(x * sx + tx);
			rep->vertices[i][Y] = float2scalar(y * sy + ty);
			rep->vertices[i][Z] = float2scalar(z * sz + tz);
			}
		for (i = 0; i < nf; ++i)
			{
			char *ptr;
			int j, detail = 0;
			unsigned int surfvalue;
			vrl_Facet *facet = vrl_malloc(sizeof(vrl_Facet));
			if (facet == NULL)
				return NULL;
			getline(buff, sizeof(buff), in);
			if (isdigit(buff[0]))
				surfvalue = strtoul(buff, &ptr, 0);
			else
				surfvalue = map_ascii_surface_name(buff);
			facet->id = 0;
			if (surfvalue & 0x8000)  /* indexed color */
				facet->surface = surfvalue & 0x7FFF;
			else
				{
				vrl_Surface surf;
				vrl_SurfaceFromDesc(surfvalue, &surf);  /* convert value to surface */
				for (j = 0; j < nsurfs_defined; ++j)  /* seen one of these before? */
					if (surf.type == tempsurfs[j]->type
						&& surf.hue == tempsurfs[j]->hue
						&& surf.brightness == tempsurfs[j]->brightness)
						break;
				facet->surface = j;
				if (j >= nsurfs_defined)  /* new one... add to tempsurfs[] */
					{
					vrl_Surface *s = vrl_malloc(sizeof(vrl_Surface));
					if (s == NULL)
						return NULL;
					vrl_SurfaceInit(s);
					tempsurfs[nsurfs_defined++] = s;
					s->type = surf.type;
					s->hue = surf.hue;
					s->brightness = surf.brightness;
					s->texture = NULL;
					}
				}
			facet->highlight = facet->interior = 0;
			facet->details = NULL;
			facet->nearside = facet->farside = NULL;
			facet->npoints = strtoul(ptr, &ptr, 0);
			facet->points = vrl_calloc(facet->npoints, sizeof(int));
			facet->edges = NULL;
			if (facet->points == NULL)
				return NULL;
			for (j = 0; j < facet->npoints; ++j)
				facet->points[facet->npoints-1-j] = strtoul(ptr, &ptr, 0);
			vrl_FacetComputeNormal(facet, rep->vertices);
			p = strtok(ptr, seps);
			while (p)
				{
				if (*p == 'i') facet->interior = 1;
				if (*p == 'd') detail = 1;
				/* for now, ignore other stuff, like Nfacet#, Ffacet#, Cr,g,b,i Ttexture */
				p = strtok(NULL, seps);
				}
			if (detail && parent)  /* if we're a detail facet */
				{                  /* attach us to our parent */
				facet->farside = parent->details;
				parent->details = facet;
				}
			else   /* otherwise, attach us to the rep's facet list */
				{
				facet->farside = rep->facets;
				rep->facets = facet;
				}
			if (!detail) parent = facet;  /* if we're not a detail, we're a (potenial) parent */
			}
		if (!multi) break;
		}
	if (!multi && !got_header)
		return NULL;
	if (nsurfs_defined)
		shape->default_surfacemap = vrl_SurfacemapCreate(nsurfs_defined);
	if (shape->default_surfacemap)
		for (i = 0; i < nsurfs_defined; ++i)
			vrl_SurfacemapSetSurface(shape->default_surfacemap, i, tempsurfs[i]);
	vrl_ShapeComputeBounds(shape);
	return shape;
	}

static FILE *outfile;

static vrl_Surfacemap *map;

static int output_vertex(vrl_Vector *v, vrl_Vector *n)
	{
	fprintf(outfile, "%ld %ld %ld",
		(long) scalar2float((*v)[X]), (long) scalar2float((*v)[Y]), (long) scalar2float((*v)[Z]));
	if (n)
		fprintf(outfile, "%ld %ld %ld",
			(long) scalar2float((*n)[X]), (long) scalar2float((*n)[Y]), (long) scalar2float((*n)[Z]));
	fprintf(outfile, "\n");
	return 0;
	}

static int output_facet(vrl_Facet *facet)
	{
	vrl_Surface *surf = vrl_SurfacemapGetSurface(map, vrl_FacetGetSurfnum(facet));
	int np = vrl_FacetCountPoints(facet), i;
	fprintf(outfile, "0x%04.4X %d", vrl_SurfaceToDesc(surf), np);
	for (i = 0; i < np; ++i)
		fprintf(outfile, " %d", vrl_FacetGetPoint(facet, i));
	fprintf(outfile, "\n");
	return 0;
	}

static int output_rep(vrl_Rep *rep)
	{
	fprintf(outfile, "rep_%ld %d %d %d %d\n",
		(long) scalar2float(vrl_RepGetSize(rep)),
		vrl_RepCountVertices(rep), vrl_RepCountFacets(rep),
		0, vrl_RepGetSorting(rep));
	vrl_RepTraverseVertices(rep, output_vertex);
	vrl_RepTraverseFacets(rep, output_facet);
	return 0;
	}

int vrl_WritePLG(vrl_Shape *shape, FILE *out)
	{
	int n;
	if (shape == NULL) return -1;
	n = vrl_ShapeCountReps(shape);
	if (n == 0) return -2;
	if (n > 1)
		fprintf(out, "##MULTI\n");
	map = vrl_ShapeGetSurfacemap(shape);
	outfile = out;
	vrl_ShapeTraverseReps(shape, output_rep);
	return 0;
	}

vrl_Object *vrl_ObjectLoadPLGfile(char *filename)
	{
	vrl_Object *obj;
	FILE *in = fopen(vrl_FileFixupFilename(filename), "r");
	if (in == NULL)
		return NULL;
	obj = vrl_ReadObjectPLG(in);
	fclose(in);
	return obj;
	}

int vrl_ObjectSavePLGfile(vrl_Object *object, char *filename)
	{
	FILE *out;
	vrl_Shape *shape = object->shape;
	if (shape == NULL) return -2;
	out = fopen(filename, "r");
	if (out == NULL)
		return -1;
	vrl_WritePLG(shape, out);
	fclose(out);
	return 0;
	}

