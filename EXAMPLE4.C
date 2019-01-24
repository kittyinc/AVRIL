/* EXAMPLE4 -- simple object behaviours */
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
#include <stdlib.h>  /* needed for rand() */

static vrl_Angle spinrate;
static vrl_Time bounce_period;
static vrl_Scalar maxheight;
static vrl_Time pulse_period;

static void spin(void)
	{
	vrl_ObjectRotY(vrl_TaskGetData(), vrl_TaskGetElapsed() * spinrate);
	vrl_SystemRequestRefresh();
	}

static void bounce(void)
	{
	vrl_Object *obj = vrl_TaskGetData();
	unsigned long off;
	vrl_Scalar height;
	off = (360 * (vrl_TaskGetTimeNow() % bounce_period)) / bounce_period;
	height = vrl_FactorMultiply(vrl_Sine(float2angle(off)), maxheight);
	vrl_ObjectMove(obj, vrl_ObjectGetWorldX(obj), height, vrl_ObjectGetWorldZ(obj));
	vrl_SystemRequestRefresh();
	}

static void pulsate(void)
	{
	vrl_Surface *surf = vrl_SurfacemapGetSurface((vrl_Surfacemap *) vrl_TaskGetData(), 0);
	unsigned long off;
	int brightness;
	off = (360 * (vrl_TaskGetTimeNow() % pulse_period)) / pulse_period;
	brightness = abs(vrl_FactorMultiply(vrl_Sine(float2angle(off)), 255));
	vrl_SurfaceSetBrightness(surf, brightness);
	vrl_SystemRequestRefresh();
	}

void main()
	{
	vrl_Light *light;
	vrl_Camera *camera;
	vrl_Shape *cube, *sphere, *cylinder;
	vrl_Surfacemap *cubemap, *pulsemap;
	int i;

	vrl_SystemStartup();
	
	cube = vrl_PrimitiveBox(100, 100, 100, NULL);
	sphere = vrl_PrimitiveSphere(100, 6, 6, NULL);
	cylinder = vrl_PrimitiveCylinder(100, 50, 100, 8, NULL);

	cubemap = vrl_SurfacemapCreate(1);
	vrl_SurfacemapSetSurface(cubemap, 0, vrl_SurfaceCreate(5));
	pulsemap = vrl_SurfacemapCreate(1);
	vrl_SurfacemapSetSurface(pulsemap, 0, vrl_SurfaceCreate(14));

	spinrate = float2angle(72.0 / vrl_TimerGetTickRate());  /* deg per tick */
	bounce_period = 4 * vrl_TimerGetTickRate();  /* four-second period */
	maxheight = float2scalar(400);    /* maximum height in units */
	pulse_period =  2 * vrl_TimerGetTickRate();  /* two-second period */

	light = vrl_LightCreate();
	vrl_LightRotY(light, float2angle(45));
	vrl_LightRotX(light, float2angle(45));

	camera = vrl_CameraCreate();
	vrl_CameraRotY(camera, float2angle(5));
	vrl_CameraMove(camera, 0, 200, -4400);

	for (i = 0; i < 10; ++i)
		{
		vrl_Object *obj = vrl_ObjectCreate(NULL);
		vrl_ObjectMove(obj, rand() % 1000, rand() % 1000, rand() % 1000);
		switch (i & 3)
			{
			case 0:
				vrl_ObjectSetShape(obj, cube);
				break;
			case 1:
				vrl_ObjectSetShape(obj, cube);
				vrl_ObjectSetSurfacemap(obj, cubemap);
				vrl_TaskCreate(spin, obj, 10);
				break;
			case 2:
				vrl_ObjectSetShape(obj, sphere);
				vrl_TaskCreate(bounce, obj, 10);
				break;
			case 3:
				vrl_ObjectSetShape(obj, cylinder);
				vrl_ObjectSetSurfacemap(obj, pulsemap);
				break;
			}
		vrl_TaskCreate(pulsate, pulsemap, 10);
		}

	vrl_SystemRun();
	}

