/* Simple demo of AVRIL */

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
#include <stdlib.h>  /* for getenv() */

void setup(void)
	{
	vrl_Object *obj = vrl_WorldFindObject("sphere");
	if (obj)
		{
		vrl_Shape *shape = vrl_ObjectGetShape(obj);
		if (shape)
			{
			vrl_Rep *rep = vrl_ShapeGetFirstRep(shape);
			vrl_RepComputeVertexNormals(rep);
			}
		}
	}

static void spinner(void)
	{
	static int count = 0;
	vrl_Object *obj = vrl_TaskGetData();
	if (obj == NULL) return;
	vrl_ObjectRotY(obj, float2angle(10));
	vrl_SystemRequestRefresh();
//	if (++count > 500)
//		vrl_SystemStopRunning();

	}

static void head(void)
	{
	vrl_Camera *cam = vrl_TaskGetData();
	vrl_CameraRotY(cam, float2angle(10));
	vrl_SystemRequestRefresh();
	}

static vrl_Object *hand_parts[12] = { NULL };

static void flex_digit(vrl_Object *obj, vrl_Angle angle)
	{
	if (obj)
		{
		vrl_ObjectRotReset(obj);
		vrl_ObjectRotX(obj, angle);
		}
	}

static void hand(void)
	{
	vrl_Device *glove = vrl_TaskGetData();
	flex_digit(hand_parts[1], vrl_DeviceGetValue(glove, 6));
	/* ignore upper part of thumb */
	flex_digit(hand_parts[3], vrl_DeviceGetValue(glove, 7));
	flex_digit(hand_parts[4], vrl_DeviceGetValue(glove, 7));
	flex_digit(hand_parts[5], vrl_DeviceGetValue(glove, 8));
	flex_digit(hand_parts[6], vrl_DeviceGetValue(glove, 8));
	flex_digit(hand_parts[7], vrl_DeviceGetValue(glove, 9));
	flex_digit(hand_parts[8], vrl_DeviceGetValue(glove, 9));
	flex_digit(hand_parts[9], vrl_DeviceGetValue(glove, 10));
	flex_digit(hand_parts[10], vrl_DeviceGetValue(glove, 10));
#ifdef __BORLANDC__
//	if (bioskey(2) & 0x01) vrl_DeviceReset(glove);
//	if (bioskey(2) & 0x02) vrl_DeviceSetRange(glove);
#endif
	}

#include "avrildrv.h"

void main(int argc, char *argv[])
	{
//	vrl_Device *glove = vrl_DeviceOpen(vrl_FifthDevice, (void *) 101);
	vrl_SystemStartup();
	vrl_ReadCFGfile(getenv("AVRIL"));
	vrl_SetReadFIGpartArray(hand_parts, 12);
	vrl_SystemCommandLine(argc, argv);
//	vrl_ReadFIGfile("hand.fig");
	setup();
	vrl_TaskCreate(spinner, vrl_WorldFindObject("thing"), 0);
//	vrl_TaskCreate(head, vrl_WorldGetCamera(), 0);
//	vrl_TaskCreate(hand, glove, 0);
	vrl_SystemRun();
	}
