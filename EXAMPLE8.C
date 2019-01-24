/* EXAMPLE7 -- Hierarchical objects */
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

typedef struct
	{
	vrl_Object object;
	vrl_Angle speed;
	} Planet_data;

static void orbit(void)
	{
	Planet_data *data = (Planet_data *) vrl_TaskGetData();
	vrl_ObjectRotate(&data->object, vrl_TaskGetElapsed() * data->speed,
		Y, VRL_COORD_PARENT, NULL);
	vrl_SystemRequestRefresh();
	}

void main()
	{
	vrl_Shape *planet_shape;
	vrl_Object *sun;
	vrl_Light *light;
	vrl_Camera *camera;
	vrl_Surface *surf;
	vrl_Surfacemap *surfmap;
	vrl_Vector v;
	Planet_data planet1, planet2, moon;

	/* startup the system, create a light and a camera */
	vrl_SystemStartup();
	vrl_WorldSetAmbient(0);
	light = vrl_LightCreate();
	vrl_LightRotY(light, float2angle(45));
	camera = vrl_CameraCreate();
//	vrl_CameraMove(camera, 0, 0, -800);
vrl_CameraRotX(camera, float2angle(90));
vrl_CameraMove(camera, 0, 2800, 0);

	/* create the geometry that all the celestial bodies will use */	
	planet_shape = vrl_PrimitiveSphere(30, 8, 8, NULL);

	/* create the sun */
	sun = vrl_ObjectCreate(planet_shape);
	/* create a surfacemap for the sun */
	surfmap = vrl_SurfacemapCreate(1);
	surf = vrl_SurfaceCreate(5);
	vrl_SurfacemapSetSurface(surfmap, 0, surf);
	vrl_ObjectSetSurfacemap(sun, surfmap);

	/* create a planet */
	vrl_ObjectInit(&planet1.object);
	vrl_ObjectSetShape(&planet1.object, planet_shape);
	/* give it a surfacemap */
	surfmap = vrl_SurfacemapCreate(1);
	surf = vrl_SurfaceCreate(7);
	vrl_SurfacemapSetSurface(surfmap, 0, surf);
	vrl_ObjectSetSurfacemap(&planet1.object, surfmap);
	/* attach it to the sun and displace it */
	vrl_ObjectAttach(&planet1.object, sun);
	vrl_VectorCreate(v, 0, 0, -250);
	vrl_ObjectTranslate(&planet1.object, v, VRL_COORD_PARENT, NULL);
	/* make it orbit */
	planet1.speed = float2angle(36.0 / vrl_TimerGetTickRate());
	vrl_TaskCreate(orbit, &planet1, 0);

	/* create a second planet */
	vrl_ObjectInit(&planet2.object);
	vrl_ObjectSetShape(&planet2.object, planet_shape);
	/* give it a surfacemap */
	surfmap = vrl_SurfacemapCreate(1);
	surf = vrl_SurfaceCreate(9);
	vrl_SurfacemapSetSurface(surfmap, 0, surf);
	vrl_ObjectSetSurfacemap(&planet2.object, surfmap);
	/* attach it to the sun and displace it */
	vrl_ObjectAttach(&planet2.object, sun);
	vrl_VectorCreate(v, 0, 0, -500);
	vrl_ObjectTranslate(&planet2.object, v, VRL_COORD_PARENT, NULL);
	/* make it orbit */
	planet2.speed = float2angle(18.0 / vrl_TimerGetTickRate());
	vrl_TaskCreate(orbit, &planet2, 0);

	/* create a moon for the second planet */
	vrl_ObjectInit(&moon.object);
	vrl_ObjectSetShape(&moon.object, planet_shape);
	/* give it a surfacemap */
	surfmap = vrl_SurfacemapCreate(1);
	surf = vrl_SurfaceCreate(11);
	vrl_SurfacemapSetSurface(surfmap, 0, surf);
	vrl_ObjectSetSurfacemap(&moon.object, surfmap);
	/* attach it to the sun and displace it */
	vrl_ObjectAttach(&moon.object, &planet2.object);
	vrl_VectorCreate(v, 0, 0, -80);
	vrl_ObjectTranslate(&moon.object, v, VRL_COORD_PARENT, NULL);
	/* make it orbit */
	moon.speed = float2angle(72.0 / vrl_TimerGetTickRate());
	vrl_TaskCreate(orbit, &moon, 0);

	vrl_SystemRun();
	}
