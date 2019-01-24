/* Camera and Lighting routines for the AVRIL library */

/* Written by Bernie Roehl, January 1994 */

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

vrl_Light *vrl_LightInit(vrl_Light *light)
	{
	if (light == NULL) return NULL;
	light->type = VRL_LIGHT_DIRECTIONAL;
	light->on = 1;
	light->intensity = float2factor(0.5);
	light->object = NULL;
	light->name = "No Name";
	light->applic_data = NULL;
	vrl_WorldAddLight(light);
	return light;
	}

void vrl_LightDestroy(vrl_Light *light)
	{
	vrl_WorldRemoveLight(light);
	vrl_free(light);
	}

vrl_Light *vrl_LightCreate(void)
	{
	vrl_Light *light = vrl_malloc(sizeof(vrl_Light));
	vrl_Object *obj = vrl_ObjectCreate(NULL);
	if (light == NULL || obj == NULL) return NULL;
	vrl_LightInit(light);
	vrl_LightAssociate(light, obj);
	return light;
	}

vrl_Camera *vrl_CameraInit(vrl_Camera *camera)
	{
	if (camera == NULL) return NULL;
	camera->hither = float2scalar(10);
	camera->yon = float2scalar(1000000000L);
	camera->zoom = 4;  camera->aspect = 1.33;
	camera->ortho = 0;  camera->orthodist = 0;
	camera->object = NULL;
	camera->name = "No Name";
	camera->need_updating = 1;
	return camera;
	}

vrl_Camera *vrl_CameraCreate(void)
	{
	vrl_Camera *camera = vrl_malloc(sizeof(vrl_Camera));
	vrl_Object *obj = vrl_ObjectCreate(NULL);
	if (camera == NULL || obj == NULL) return NULL;
	vrl_CameraInit(camera);
	vrl_CameraAssociate(camera, obj);
	if (vrl_WorldGetCamera() == NULL)
		vrl_WorldSetCamera(camera);
	return camera;
	}

void vrl_CameraDestroy(vrl_Camera *camera)
	{
	vrl_WorldRemoveCamera(camera);
	vrl_free(camera);
	}

