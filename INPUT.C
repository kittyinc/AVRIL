/* Simple keyboard and mouse support for AVRIL demos */
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
#include "avrilkey.h"
#include "avrildrv.h"
#include <math.h>  /* floor() */

vrl_Object *active_object = NULL;

static int showhud = 0;

#define BLACKCOLOR 0x00000000L
#define WHITECOLOR 0xFFFFFFFFL

void vrl_ApplicationDrawOver(vrl_RenderStatus *stat)
	{
	vrl_ScreenPos wleft, wtop, wright, wbottom;  /* window coordinates */
	vrl_ScreenPos wwidth, wheight;   /* window dimensions */
	vrl_Camera *cam = vrl_WorldGetCamera();
	char buff[100];
	vrl_DisplayGetWindow(&wleft, &wtop, &wright, &wbottom);
	wwidth = wright - wleft + 1;
	wheight = wbottom - wtop + 1;
	if (vrl_ConfigGetPositionDisplay())
		{
		sprintf(buff, "Position: %ld,%ld",
			(long) floor(scalar2float(vrl_CameraGetWorldX(cam))),
			(long) floor(scalar2float(vrl_CameraGetWorldZ(cam))));
		if (vrl_DisplayGetTextWidth(buff) + 10 < wwidth-1)
			vrl_UserInterfaceDropText(10, 10, WHITECOLOR, buff);
		}
	if (vrl_ConfigGetFramerateDisplay())
		{
		sprintf(buff, "Frames/sec: %ld", vrl_SystemGetFrameRate());
		if (vrl_DisplayGetTextWidth(buff) + 5 < wwidth-1)
			vrl_UserInterfaceDropText(5,
				wheight - vrl_DisplayGetTextHeight(buff) - 10,
				WHITECOLOR, buff);
		}
	if (vrl_ConfigGetCompassDisplay())
		vrl_UserInterfaceDrawCompass(cam, wwidth - 70, 40, 35);
	if (showhud)
		{
		sprintf(buff, "%c%c%c",
			stat->memory ?  'M' : ' ',
			stat->objects ? 'O' : ' ',
			stat->facets ?  'F' : ' ');
		if (vrl_DisplayGetTextWidth(buff) + 10 < wwidth-1)
			vrl_UserInterfaceDropText(10, 20, WHITECOLOR, buff);
		}
	if (vrl_MouseGetUsage())
		{
		vrl_Device *dev = vrl_MouseGetPointer();
		if (dev)
			{
			int x = vrl_DeviceGetCenter(dev, X);
			int y = vrl_DeviceGetCenter(dev, Y);
			int deadx = vrl_DeviceGetDeadzone(dev, X);
			int deady = vrl_DeviceGetDeadzone(dev, Y);
			/* white inner box */
			vrl_DisplayLine(x - deadx, y - deady, x + deadx, y - deady, WHITECOLOR);
			vrl_DisplayLine(x - deadx, y + deady, x + deadx, y + deady, WHITECOLOR);
			vrl_DisplayLine(x - deadx, y - deady, x - deadx, y + deady, WHITECOLOR);
			vrl_DisplayLine(x + deadx, y - deady, x + deadx, y + deady, WHITECOLOR);
			/* black outer box */
			vrl_DisplayLine(x-deadx-1, y-deady-1, x+deadx+1, y-deady-1, BLACKCOLOR);
			vrl_DisplayLine(x-deadx-1, y+deady+1, x+deadx+1, y+deady+1, BLACKCOLOR);
			vrl_DisplayLine(x-deadx-1, y-deady-1, x-deadx-1, y+deady+1, BLACKCOLOR);
			vrl_DisplayLine(x+deadx+1, y-deady-1, x+deadx+1, y+deady+1, BLACKCOLOR);
			}
		}
	}

static void process_key(int c)
	{
	vrl_Camera *cam = vrl_WorldGetCamera();
	switch (c)
		{
		case 'v':
			if (vrl_DisplayStereoGetViewEye() == VRL_STEREOEYE_RIGHT)
				vrl_DisplayStereoSetViewEye(VRL_STEREOEYE_LEFT);
			else
				vrl_DisplayStereoSetViewEye(VRL_STEREOEYE_RIGHT);
			break;
		case 's': vrl_WorldToggleStereo(); break;
		case '[':
			{
			vrl_StereoConfiguration *conf = vrl_WorldGetStereoConfiguration();
			int x = vrl_StereoGetLeftEyeShift(conf);
			++x;
			vrl_StereoSetLeftEyeShift(conf, x);
			vrl_StereoSetRightEyeShift(conf, x);
			}
			break;
		case ']':
			{
			vrl_StereoConfiguration *conf = vrl_WorldGetStereoConfiguration();
			int x = vrl_StereoGetLeftEyeShift(conf);
			--x;
			vrl_StereoSetLeftEyeShift(conf, x);
			vrl_StereoSetRightEyeShift(conf, x);
			}
			break;
		case 'o': cam->ortho = !cam->ortho; break;
		case ' ': vrl_MouseSetUsage(!vrl_MouseGetUsage()); break;
		case 'w': vrl_RenderSetDrawMode(!vrl_RenderGetDrawMode()); break;
		case 'q': case 0x1B: vrl_SystemStopRunning(); break;
		case 'f': vrl_ConfigToggleFramerateDisplay(); break;
		case 'c': vrl_ConfigToggleCompassDisplay(); break;
		case 'p': vrl_ConfigTogglePositionDisplay(); break;
		case 'd': showhud = !showhud; break;
		case '_': vrl_WorldToggleHorizon(); break;
		case '+': vrl_CameraSetZoom(cam, vrl_CameraGetZoom(cam) * 1.1); break;
		case '-': vrl_CameraSetZoom(cam, vrl_CameraGetZoom(cam) * 0.9); break;
		case '=': vrl_CameraSetZoom(cam, 1.0); break;
		case 'h':
			{
			vrl_Scalar newhither = vrl_CameraGetHither(cam) - vrl_WorldGetMovestep();
			if (newhither < 1) newhither = 1;
			vrl_CameraSetHither(cam, newhither);
			}
			break;
		case 'H':
			{
			vrl_Scalar newhither = vrl_CameraGetHither(cam) + vrl_WorldGetMovestep();
			if (newhither < 1) newhither = 1;
			vrl_CameraSetHither(cam, newhither);
			}
			break;
		default: break;
		}
	vrl_SystemRequestRefresh();
	}
	
void vrl_ApplicationKey(unsigned int c)
	{
	static int lastkey = 0;
	if (c == INSKEY)
		{
		int i;
		for (i = 0; i < 100; ++i)
			{
			process_key(lastkey);
			vrl_SystemRender(vrl_WorldUpdate());
			}
		}
	else
		process_key(lastkey = c);
	}

void vrl_ApplicationMouseUp(int x, int y, unsigned int buttons)
	{
	vrl_Object *old_active = active_object;
	if ((buttons & 1) == 0)
		return;
	vrl_RenderMonitorInit(x, y);
	vrl_SystemRender(NULL);  /* redraw screen */
	if (vrl_RenderMonitorRead(&active_object, NULL, NULL))
		{
		if (active_object == old_active)
			active_object = NULL;
		else
			vrl_ObjectSetHighlight(active_object, 1);
		}
	if (old_active)
		vrl_ObjectSetHighlight(old_active, 0);
	vrl_SystemRequestRefresh();
	}

static int object_mover(vrl_Object *obj, vrl_Device *dev, vrl_CoordFrame frame)
	{
	vrl_Vector v;
	if (obj == NULL || dev == NULL) return -1;
	if (vrl_DeviceGetRotationMode(dev) == VRL_MOTION_ABSOLUTE)
		vrl_ObjectRotReset(obj);
	if (vrl_DeviceGetTranslationMode(dev) == VRL_MOTION_ABSOLUTE)
		vrl_ObjectMove(obj, 0, 0, 0);
	if (vrl_DeviceGetRotationMode(dev))
		{
		vrl_ObjectRotate(obj, vrl_DeviceGetValue(dev, YROT), Y, frame, NULL);
		vrl_ObjectRotate(obj, vrl_DeviceGetValue(dev, XROT), X, frame, NULL);
		vrl_ObjectRotate(obj, vrl_DeviceGetValue(dev, ZROT), Z, frame, NULL);
		}
	if (vrl_DeviceGetTranslationMode(dev))
		{
		vrl_VectorCreate(v, vrl_DeviceGetValue(dev, X), vrl_DeviceGetValue(dev, Y), vrl_DeviceGetValue(dev, Z));
		vrl_ObjectTranslate(obj, v, frame, NULL);
		}
	vrl_SystemRequestRefresh();
	return 0;
	}

static int object_move_locally(vrl_Object *obj)
	{
	return object_mover(obj, vrl_ObjectGetApplicationData(obj), VRL_COORD_LOCAL);
	}

void vrl_ApplicationInit(void)
	{
	vrl_Object *torso, *head;
	vrl_Device *headdev = vrl_DeviceFind("head");
	vrl_Device *torsodev = vrl_DeviceFind("body");

	/* if neither a head device nor a torso device was specified in the
	   configuration file, use the keypad as the head device */
	if (headdev == NULL && torsodev == NULL)	
		torsodev = vrl_DeviceOpen(vrl_KeypadDevice, 0);

	/* find the head (the object the camera is associated with) and
	   set it up to track the head device */
	head = vrl_CameraGetObject(vrl_WorldGetCamera());
	vrl_ObjectSetFunction(head, object_move_locally);
	vrl_ObjectSetApplicationData(head, headdev);

	/* if the application hasn't already attached the head to a torso,
	   then make a copy of the head to form the torso and attach the head
	   to it; make sure that the torso thus created has no shape (we don't
	   want a head on top of a another head to be the default) */
	torso = vrl_ObjectFindRoot(head);
	if (torso == head)  /* no torso yet */
		{
		torso = vrl_ObjectCopy(head);      /* create one */
		vrl_ObjectAttach(head, torso);     /* and attach the head to it */
		vrl_ObjectSetShape(torso, NULL);   /* doesn't look like the head */
		}

	/* let the torso track the "body" device */
	if (torsodev)
		{
		vrl_ObjectSetFunction(torso, object_move_locally);
		vrl_ObjectSetApplicationData(torso, torsodev);
		}
	}

