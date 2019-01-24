/* EXAMPLE5 -- manipulating a cube with the Logitech Cyberman */
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
#include "avrildrv.h"

vrl_Object *cube = NULL;

static void cube_mover(void)
	{
	vrl_Device *dev = vrl_TaskGetData();
	vrl_Object *viewer = vrl_CameraGetObject(vrl_WorldGetCamera());
	vrl_Vector v;
	vrl_ObjectRotate(cube, vrl_DeviceGetValue(dev, YROT), Y, VRL_COORD_OBJREL, viewer);
	vrl_ObjectRotate(cube, vrl_DeviceGetValue(dev, XROT), X, VRL_COORD_OBJREL, viewer);
	vrl_ObjectRotate(cube, vrl_DeviceGetValue(dev, ZROT), Z, VRL_COORD_OBJREL, viewer);
	vrl_VectorCreate(v, vrl_DeviceGetValue(dev, X), vrl_DeviceGetValue(dev, Y), vrl_DeviceGetValue(dev, Z));
	vrl_ObjectTranslate(cube, v, VRL_COORD_OBJREL, viewer);
	vrl_SystemRequestRefresh();
	}

void main()
	{
	vrl_Light *light;
	vrl_Camera *camera;
	vrl_Device *dev;

	vrl_SystemStartup();

	cube = vrl_ObjectCreate(vrl_PrimitiveBox(100, 100, 100, NULL));
	vrl_ObjectRotY(cube, float2angle(45));

	light = vrl_LightCreate();
	vrl_LightRotY(light, float2angle(45));
	vrl_LightRotX(light, float2angle(45));

	camera = vrl_CameraCreate();
	vrl_CameraRotX(camera, float2angle(45));
	vrl_CameraMove(camera, 0, 500, -500);

	dev = vrl_DeviceOpen(vrl_CybermanDevice, vrl_SerialOpen(0x2F8, 3, 2000));
	if (dev)
		{
		vrl_DeviceSetScale(dev, X, float2scalar(50));
		vrl_DeviceSetScale(dev, Y, float2scalar(50));
		vrl_DeviceSetScale(dev, Z, float2scalar(50));
		vrl_TaskCreate(cube_mover, dev, 0);
		}

	vrl_SystemRun();
	}

