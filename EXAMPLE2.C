/* EXAMPLE2 -- several asteroids, sharing the same geometry */
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

void main()
	{
	FILE *infile;
	vrl_Light *light;
	vrl_Camera *camera;
	vrl_Shape *asteroidshape = NULL;
	int i;

	vrl_SystemStartup();
	
	vrl_WorldSetHorizon(0);    /* turn off horizon */
	vrl_WorldSetSkyColor(0);   /* black sky */

	infile = fopen("asteroid.plg", "r");
	if (infile)
		{
		asteroidshape = vrl_ReadPLG(infile);
		fclose(infile);
		}

	light = vrl_LightCreate();
	vrl_LightRotY(light, float2angle(45));
	vrl_LightRotX(light, float2angle(45));
	vrl_LightSetIntensity(light, float2factor(0.9));

	camera = vrl_CameraCreate();
	vrl_CameraMove(camera, 0, 100, -50);

	for (i = 0; i < 5; ++i)
		{
		vrl_Object *obj = vrl_ObjectCreate(asteroidshape);
		vrl_ObjectMove(obj, rand() % 1000, rand() % 1000, rand() % 1000);
		}

	vrl_SystemRun();
	}

