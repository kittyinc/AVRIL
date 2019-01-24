/* EXAMPLE6 -- using the configuration file to simplify setup */
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

static void object_manipulator(void)
	{
	extern vrl_Object *active_object;  /* defined in input.c */
	vrl_Device *dev = vrl_TaskGetData();
	vrl_Object *viewer = vrl_CameraGetObject(vrl_WorldGetCamera());
	vrl_Vector v;
	vrl_ObjectRotate(active_object, vrl_DeviceGetValue(dev, YROT), Y, VRL_COORD_OBJREL, viewer);
	vrl_ObjectRotate(active_object, vrl_DeviceGetValue(dev, XROT), X, VRL_COORD_OBJREL, viewer);
	vrl_ObjectRotate(active_object, vrl_DeviceGetValue(dev, ZROT), Z, VRL_COORD_OBJREL, viewer);
	vrl_VectorCreate(v, vrl_DeviceGetValue(dev, X), vrl_DeviceGetValue(dev, Y), vrl_DeviceGetValue(dev, Z));
	vrl_ObjectTranslate(active_object, v, VRL_COORD_OBJREL, viewer);
	vrl_SystemRequestRefresh();
	}

void main(int argc, char *argv[])
	{
	vrl_Device *dev;
	vrl_ConfigStartup("example6.cfg");
	vrl_SystemCommandLine(argc, argv);
	dev = vrl_DeviceFind("manipulator");
	if (dev)
		vrl_TaskCreate(object_manipulator, dev, 0);
	vrl_SystemRun();
	}
