/* EXAMPLE7 -- Gouraud shading */
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

static void load_palette(char *filename)
	{
	FILE *infile = fopen(filename, "rb");
	if (infile)
		{
		vrl_PaletteRead(infile, vrl_WorldGetPalette());
		fclose(infile);
		}
	}

static vrl_Angle tumblerate;

void tumbler(void)
	{
	vrl_Object *obj = vrl_TaskGetData();
	vrl_Angle amount = vrl_TaskGetElapsed() * tumblerate;
	vrl_ObjectRotY(obj, amount);
	vrl_ObjectRotX(obj, amount);
	vrl_SystemRequestRefresh();
	}

void main()
	{
	vrl_Shape *smooth_shape;
	vrl_Object *thing;
	vrl_Light *light;
	vrl_Camera *camera;
	vrl_Surface *surf;

	vrl_SystemStartup();
	
	load_palette("shade32.pal");

	smooth_shape = vrl_PrimitiveCylinder(100, 25, 200, 16, NULL);
	vrl_ShapeComputeVertexNormals(smooth_shape);

	surf = vrl_SurfacemapGetSurface(vrl_ShapeGetSurfacemap(smooth_shape), 0);
	vrl_SurfaceSetType(surf, VRL_SURF_GOURAUD);
	vrl_SurfaceSetHue(surf, 4);
	vrl_SurfaceSetBrightness(surf, 243);

	thing = vrl_ObjectCreate(smooth_shape);
	vrl_ObjectRelMove(thing, 0, -100, 0);

	vrl_WorldSetAmbient(0);
	light = vrl_LightCreate();
	vrl_LightRotY(light, float2angle(45));

	camera = vrl_CameraCreate();
	vrl_CameraMove(camera, 0, 0, -1400);

	tumblerate = float2angle(72.0 / vrl_TimerGetTickRate());
	vrl_TaskCreate(tumbler, thing, 0);

	vrl_SystemRun();
	}
