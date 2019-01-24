/* Simple walkthru using the CyberMaxx HMD */

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


/* First joystick button moves you forward, second one toggles select */

#include "avril.h"
#include "avrilkey.h"
#include "avrildrv.h"

#include <stdlib.h>  /* for getenv() */
#include <dos.h>     /* inp() */

void main(int argc, char *argv[])
	{
	vrl_SystemStartup();
	vrl_ReadCFGfile(getenv("AVRIL"));
	vrl_SystemCommandLine(argc, argv);
	vrl_SystemRun();
	}

void vrl_ApplicationKey(unsigned int c)
	{
	vrl_Camera *cam = vrl_WorldGetCamera();
	switch (c)
		{
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
		case 'q': case 0x1B: vrl_SystemStopRunning(); break;
		case '+': vrl_CameraSetZoom(cam, vrl_CameraGetZoom(cam) * 1.1); break;
		case '-': vrl_CameraSetZoom(cam, vrl_CameraGetZoom(cam) * 0.9); break;
		case '=': vrl_CameraSetZoom(cam, 1.0); break;
		default: break;
		}
	vrl_SystemRequestRefresh();
	}
	
static int head_turner(vrl_Object *obj)
	{
	vrl_Device *dev;
	if (obj == NULL) return -1;
	dev = vrl_ObjectGetApplicationData(obj);
	if (dev == NULL) return -1;
	vrl_ObjectRotReset(obj);
	vrl_ObjectRotate(obj, vrl_DeviceGetValue(dev, YROT), Y, VRL_COORD_LOCAL, NULL);
	vrl_ObjectRotate(obj, vrl_DeviceGetValue(dev, XROT), X, VRL_COORD_LOCAL, NULL);
	vrl_ObjectRotate(obj, vrl_DeviceGetValue(dev, ZROT), Z, VRL_COORD_LOCAL, NULL);
	vrl_SystemRequestRefresh();
	return 0;
	}

#define read_buttons() (((~inp(0x201)) >> 4) & 0x03)

static vrl_Object *head, *body;
static vrl_Device *headdev;

static void body_task(void)
	{
	static float velocity = 1.0;  /* millimeters per millisecond */
	vrl_Time elapsed = vrl_TaskGetElapsed();
	if (read_buttons() & 0x01)
		{
		vrl_Angle yaw = vrl_DeviceGetValue(headdev, YROT);
		vrl_Vector v;
		float angle = angle2float(yaw) * 3.14159 / 180.0;
		v[X] = velocity * elapsed * sin(angle);
		v[Y] = 0;
		v[Z] = velocity * elapsed * cos(angle);
		vrl_ObjectTranslate(body, v, VRL_COORD_LOCAL, NULL);
		}
	}

static void control_task(void)
	{
	static enum 
		{ BUTTON_NOT_PRESSED, BUTTON_PRESSED }
		state = BUTTON_NOT_PRESSED;
	switch (state)
		{
		case BUTTON_NOT_PRESSED:
			if (read_buttons() & 0x02)
				state = BUTTON_PRESSED;
			break;
		case BUTTON_PRESSED:
			if (!(read_buttons() & 0x02))
				{
				vrl_Object *obj;
				vrl_RenderMonitorInit(160, 50);
				vrl_SystemRender(NULL);  /* redraw screen */
				if (vrl_RenderMonitorRead(&obj, NULL, NULL))
					vrl_ObjectToggleHighlight(obj);
				vrl_SystemRequestRefresh();
				state = BUTTON_NOT_PRESSED;
				}
		default: state = BUTTON_NOT_PRESSED; break;
		}
	}

void vrl_ApplicationInit(void)
	{
	/* find the head (the object the camera is associated with) and
	   set it up to track the head device */
	head = vrl_CameraGetObject(vrl_WorldGetCamera());
	headdev = vrl_DeviceFind("head");
	vrl_ObjectSetFunction(head, head_turner);
	vrl_ObjectSetApplicationData(head, headdev);

	/* if the application hasn't already attached the head to a torso,
	   then make a copy of the head to form the torso and attach the head
	   to it; make sure that the torso thus created has no shape (we don't
	   want a head on top of a another head to be the default) */
	body = vrl_ObjectFindRoot(head);
	if (body == head)  /* no torso yet */
		{
		body = vrl_ObjectCopy(head);      /* create one */
		vrl_ObjectAttach(head, body);     /* and attach the head to it */
		vrl_ObjectSetShape(body, NULL);   /* doesn't look like the head */
		}

	vrl_TaskCreate(body_task, body, 1);
	vrl_TaskCreate(control_task, NULL, 1);
	}
