/* World manipulation routines for the AVRIL library */
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
#include <stdlib.h>  /* max() */
#include <string.h>

static vrl_World world =
	{
	NULL,   /* objects */
	NULL,   /* lights */
	NULL,   /* cameras */
	NULL,   /* current camera */
	NULL,   /* left-eye camera */
	NULL,   /* right-eye camera */
	NULL,   /* stereo configuration */
	float2factor(0.5),  /* ambient light */
	float2scalar(1),    /* world scale */
	float2scalar(100),      /* movestep */
	float2angle(5),         /* turnstep */
	1,  /* movement mode */
	0,  /* stereoscopic mode */
	1,  /* screenclear */
	1,  /* horizon */
	{ 0x88, 3 },     /* horizon colors */
	2,               /* ncolors */
	{ 0 }            /* palette */
	};

vrl_World *vrl_current_world = &world;   /* the currently active world */

vrl_World *vrl_WorldInit(vrl_World *world)
	{
	int i;
	if (world == NULL) return NULL;
	world->objects = NULL;
	world->lights = NULL;
	world->camera = world->left_camera = world->right_camera = NULL;
	world->stereo = NULL;
	world->ambient = float2factor(0.5);
	world->screenclear = 1;  world->horizon = 1;
	world->movement_mode = 1;
	world->movestep = float2scalar(100);
	world->rotstep = float2angle(5);
	world->horizoncolors[0] = 0x88;  world->horizoncolors[1] = 3;
	world->nhorizoncolors = 2;
	world->scale = 1;
	world->use_stereo = 0;
	vrl_PaletteInit(&world->palette);
	if (vrl_current_world == NULL) vrl_current_world = world;
	return world;
	}

void vrl_WorldAddLight(vrl_Light *light)
	{
	if (vrl_current_world == NULL || light == NULL) return;
	light->next = vrl_current_world->lights;
	vrl_current_world->lights = light;
	}

void vrl_WorldRemoveLight(vrl_Light *light)
	{
	vrl_Light *p;
	if (vrl_current_world == NULL || light == NULL) return;
	if (vrl_current_world->lights == light)
		vrl_current_world->lights = light->next;
	else
		for (p = vrl_current_world->lights; p; p = p->next)
			if (p->next == light)
				{
				p->next = light->next;
				break;
				}
	light->next = NULL;
	}

vrl_Light *vrl_WorldFindLight(char *name)
	{
	vrl_Light *p;
	for (p = vrl_current_world->lights; p; p = p->next)
		if (!stricmp(p->name, name))
			return p;
	return NULL;
	}

void vrl_WorldAddCamera(vrl_Camera *camera)
	{
	if (vrl_current_world == NULL || camera == NULL) return;
	camera->next = vrl_current_world->cameras;
	vrl_current_world->cameras = camera;
	}

void vrl_WorldRemoveCamera(vrl_Camera *camera)
	{
	vrl_Camera *p;
	if (vrl_current_world == NULL || camera == NULL) return;
	if (vrl_current_world->cameras == camera)
		vrl_current_world->cameras = camera->next;
	else
		for (p = vrl_current_world->cameras; p; p = p->next)
			if (p->next == camera)
				{
				p->next = camera->next;
				break;
				}
	camera->next = NULL;
	}

vrl_Camera *vrl_WorldFindCamera(char *name)
	{
	vrl_Camera *p;
	for (p = vrl_current_world->cameras; p; p = p->next)
		if (!stricmp(p->name, name))
			return p;
	return NULL;
	}

void vrl_WorldAddObject(vrl_Object *obj)
	{
	if (vrl_current_world == NULL || obj == NULL) return;
	vrl_WorldRemoveObject(obj);
	obj->siblings = vrl_current_world->objects;
	vrl_current_world->objects = obj;
	}

void vrl_WorldRemoveObject(vrl_Object *obj)
	{
	vrl_Object *p;
	if (vrl_current_world == NULL || obj == NULL) return;
	vrl_ObjectDetach(obj);
	if (vrl_current_world->objects == obj)
		vrl_current_world->objects = obj->siblings;
	else
		for (p = vrl_current_world->objects; p; p = p->siblings)
			if (p->siblings == obj)
				{
				p->siblings = obj->siblings;
				break;
				}
	obj->siblings = NULL;
	}

static int n;

static int count_objs(vrl_Object *obj)
	{
	if (obj)
		++n;
	return 0;
	}

int vrl_WorldCountObjects(void)
	{
	n = 0;
	vrl_ObjectTraverse(vrl_current_world->objects, count_objs);
	return n;
	}

static int count_facets(vrl_Object *obj)
	{
	vrl_Rep *rep;
	vrl_Shape *shape;
	if (obj == NULL) return 0;        /* no object */
	shape = vrl_ObjectGetShape(obj);
	if (shape == NULL) return 0;      /* object has no shape */
	rep = vrl_ObjectGetRep(obj);
	if (rep)    /* object has a "current" rep -- use that */
		{
		n += vrl_RepCountFacets(rep);
		return 0;
		}
	rep = vrl_ShapeGetFirstRep(shape);  /* otherwise use the first (most detailed) rep */
	if (rep)
		n += vrl_RepCountFacets(rep);
	return 0;
	}

int vrl_WorldCountFacets(void)
	{
	n = 0;
	vrl_ObjectTraverse(vrl_current_world->objects, count_facets);
	return n;
	}

int vrl_WorldCountLights(void)
	{
	vrl_Light *light;
	int n = 0;
	for (light = vrl_current_world->lights; light; light = light->next)
		++n;
	return n;
	}

int vrl_WorldCountCameras(void)
	{
	vrl_Camera *camera;
	int n = 0;
	for (camera = vrl_current_world->cameras; camera; camera = camera->next)
		++n;
	return n;
	}

static vrl_Object *found_obj;
static char *find_name;

static int find_obj(vrl_Object *obj)
	{
	if (obj == NULL) return 0;
	if (!stricmp(obj->name, find_name))
		{
		found_obj = obj;
		return 1;
		}
	return 0;
	}

vrl_Object *vrl_WorldFindObject(char *name)
	{
	found_obj = NULL;
	find_name = name;
	vrl_ObjectTraverse(vrl_current_world->objects, find_obj);
	return found_obj;
	}

static vrl_Scalar minx, maxx, miny, maxy, minz, maxz;

static int find_bounds(vrl_Object *obj)
	{
	vrl_Vector v;
	vrl_ObjectGetMinbounds(obj, v);
	if (v[X] < minx) minx = v[X];
	else if (v[X] > maxx) maxx = v[X];
	if (v[Y] < miny) miny = v[Y];
	else if (v[Y] > maxy) maxy = v[Y];
	if (v[Z] < minz) minz = v[Z];
	else if (v[Z] > maxz) maxz = v[Z];
	return 0;
	}

void vrl_WorldGetBounds(vrl_Vector v1, vrl_Vector v2)
	{
	vrl_Object *obj = vrl_WorldGetObjectTree();
	vrl_Vector v;
	vrl_ObjectGetMinbounds(obj, v);
	minx = maxx = v[X];
	miny = maxy = v[Y];
	minz = maxz = v[Z];
	vrl_ObjectTraverse(obj, find_bounds);
	v1[X] = minx;  v1[Y] = miny;  v1[Z] = minz;
	v2[X] = maxx;  v2[Y] = maxy;  v2[Z] = maxz;
	}

void vrl_WorldGetCenter(vrl_Vector v)
	{
	vrl_Vector v1, v2;
	vrl_WorldGetBounds(v1, v2);
	v[X] = (v1[X] + v2[X]) / 2;
	v[Y] = (v1[Y] + v2[Y]) / 2;
	v[Z] = (v1[Z] + v2[Z]) / 2;
	}

vrl_Scalar vrl_WorldGetSize(void)
	{
	vrl_Vector v1, v2, vc;
	vrl_WorldGetBounds(v1, v2);
	vrl_WorldGetCenter(vc);
	return max(vrl_VectorDistance(v1, vc), vrl_VectorDistance(v2, vc));
	}

