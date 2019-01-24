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
#ifdef COMMENTS

   Things to do:

   Pelvis/chest heights

   Area stuff:
     Test "passages"
     Automatic placement of objects into areas after everything's loaded
     Area visibility stuff (mods to system.c?)

   "touching" algorithm for button B

   "behaviour" simulation

#endif

/* Mainline for "Boris" game (working title only) */

/* Written by Bernie Roehl, December 1994 */

#include "avril.h"
#include "avrilkey.h"
#include "avrildrv.h"
#include "area.h"
#include "game.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>  /* stricmp(), for area parsing */
#include <math.h>    /* sin() and cos() */

void main(int argc, char *argv[])
	{
	FILE *in = fopen("boris.wld", "r");
	if (in == NULL)
		{
		fprintf(stderr, "Could not open 'boris.wld'\n");
		exit(1);
		}
	vrl_SystemStartup();
	vrl_ReadCFGfile("boris.cfg");
	vrl_ReadWLD(in);
	fclose(in);
	vrl_CameraCreate();
	vrl_WorldUpdate();
	vrl_ApplicationInit();
	if (vrl_WorldGetStereoConfiguration())
		vrl_StereoConfigure(vrl_WorldGetStereoConfiguration());
	while (vrl_SystemIsRunning())
		{
		if (vrl_KeyboardCheck())
			if (vrl_KeyboardRead() == ESC)
				vrl_SystemStopRunning();
		vrl_TaskRun();
		vrl_DevicePollAll();
		if (vrl_SystemQueryRefresh())
			vrl_SystemRender(vrl_WorldUpdate());
		}
	}

typedef struct _selectable_entry selectable_entry;

struct _selectable_entry
	{
	vrl_Object *obj;
	selectable_entry *next;
	};

selectable_entry *sel_list = NULL;

int is_selectable(vrl_Object *obj)
	{
	selectable_entry *sel;
	for (sel = sel_list; sel; sel = sel->next)
		if (sel->obj == obj)
			return 1;
	return 0;
	}

Area *current_area = NULL;
vrl_Object *users_body = NULL, *users_head = NULL;
vrl_Object *target_object = NULL;

static vrl_Object *observed_object = NULL;

static vrl_Device *head_tracking_device, *control_device;

/* this function reads the head tracker, and updates the head and body orientation */

static int head_tracking_function(vrl_Object *unused)
	{
	if (users_head == NULL) return 0;  /* user has no head! */
	if (users_body == NULL) return 0;  /* user has no body! */
	if (head_tracking_device == NULL) return 0;  /* there's no head tracker */
	/* turning the head turns the body */
	vrl_ObjectRotReset(users_body);    /* device is absolute */
	vrl_ObjectRotate(users_body, vrl_DeviceGetValue(head_tracking_device, YROT), Y, VRL_COORD_LOCAL, NULL);
	/* tilting and rolling the head rotates just the head */
	vrl_ObjectRotReset(users_head);    /* device is absolute */
	vrl_ObjectRotate(users_head, vrl_DeviceGetValue(head_tracking_device, XROT), X, VRL_COORD_LOCAL, NULL);
	vrl_ObjectRotate(users_head, vrl_DeviceGetValue(head_tracking_device, ZROT), Z, VRL_COORD_LOCAL, NULL);
	vrl_SystemRequestRefresh();
	return 0;
	}

/* this task reads the control device, and moves the user */

static void movement_task(void)
	{
	if (users_body == NULL) return;  /* user has no body! */
	if (control_device == NULL) return;  /* there's no control device */
	if (vrl_DeviceGetButtons(control_device) &0x01)  /* first button down */
		{
		vrl_Angle body_rotx, body_roty, body_rotz;  /* body orientation */
		float direction;

		/* find out how long it's been since this task last ran */
		vrl_Time elapsed = vrl_TaskGetElapsed();

		/* find our current location */
		vrl_Scalar newx, newy, newz;
		vrl_Scalar oldx = vrl_ObjectGetWorldX(users_body);
		vrl_Scalar oldy = vrl_ObjectGetWorldY(users_body);
		vrl_Scalar oldz = vrl_ObjectGetWorldZ(users_body);

		/* if we're not in any area, see if we've entered one */
		if (current_area == NULL)
			current_area = area_which(oldx, oldy, oldz);

		/* get the orientation of our body */
		vrl_ObjectGetWorldRotations(users_body, NULL, &body_roty, NULL);

		/* find direction we're facing */
		direction = angle2float(body_roty) * 3.14159262 / 180.0;

		/* compute new location based on direction, velocity and time */
		newx = oldx + float2scalar(elapsed * users_velocity * sin(direction));
		newy = oldy;
		newz = oldz + float2scalar(elapsed * users_velocity * cos(direction));

		/* see if we're trying to leave the current area */
		if (!area_contains(current_area, newx, newy, newz))
			{
			/* find out what area, if any, we'd end up in */
			Area *new_area = area_move(current_area, newx, newy, newz);
			if (new_area == NULL)  /* can't move to newx, newy, newz */
				{
				game_bump();
				vrl_SystemRequestRefresh();
				return;
				}
			/* we've entered a new area */
			current_area = new_area;
			}

		/* adjust our height based on the "floor" of the current area */
		if (current_area)
			newy = area_height(current_area, newx, newz);
		else
			newy = 0;
		newy += users_pelvis_height;  /* figure height of user's pelvis */

		/* make the move */
		vrl_ObjectMove(users_body, newx, newy, newz);
		}
	vrl_SystemRequestRefresh();
	}

static void target_task(void)
	{
	if (observed_object)
		{
		vrl_ObjectSetHighlight(observed_object, 0);
		observed_object = NULL;
		}
	vrl_RenderMonitorInit(vrl_DisplayGetWidth() / 2, vrl_DisplayGetHeight() / 2);
	vrl_SystemRender(NULL);
	vrl_RenderMonitorRead(&observed_object, NULL, NULL);
	if (is_selectable(observed_object))
		{
		vrl_ObjectSetHighlight(observed_object, 1);
		if (target_object == NULL)
			if (vrl_DeviceGetButtons(control_device) & 0x02)  /* second button down */
				target_object = observed_object;
		}
	vrl_SystemRequestRefresh();
	}

void vrl_ApplicationInit(void)
	{
	extern vrl_DeviceDriverFunction vrl_ControlDevice, vrl_FakeDevice;

	head_tracking_device = vrl_DeviceFind("head");
	if (head_tracking_device == NULL)
		head_tracking_device = vrl_DeviceOpen(vrl_FakeDevice, 0);
	control_device = vrl_DeviceOpen(vrl_ControlDevice, 0);

	/* if neither a head tracking device nor a users_body device was specified in the
	   configuration file, use the keypad as control head device */
	if (head_tracking_device == NULL && control_device == NULL)	
		control_device = vrl_DeviceOpen(vrl_KeypadDevice, 0);

	/* find the head (the object the camera is associated with) and
	   set it up to track the head device */
	users_head = vrl_CameraGetObject(vrl_WorldGetCamera());
	vrl_ObjectSetFunction(users_head, head_tracking_function);

	/* if the application hasn't already attached the head to a body,
	   then make a copy of the head to form the body and attach the head
	   to it; make sure that the body thus created has no shape (we don't
	   want a head on top of a another head to be the default!) */
	users_body = vrl_ObjectFindRoot(users_head);
	if (users_body == users_head)  /* no users_body yet */
		{
		users_body = vrl_ObjectCopy(users_head);    /* create one */
		vrl_ObjectAttach(users_head, users_body);   /* and attach the head to it */
		vrl_ObjectTranslate(users_head, 0, users_chest_height, 0);
		vrl_ObjectSetShape(users_body, NULL);       /* doesn't look like the head */
		}
	vrl_ObjectRotReset(users_body);
	vrl_ObjectRotY(users_body, float2angle(45));
	vrl_ObjectRotReset(users_head);

	vrl_TaskCreate(movement_task, NULL, 0);
	vrl_TaskCreate(target_task, NULL, 50);
	game_setup();
	vrl_SystemRequestRefresh();
	}

void vrl_ApplicationKey(unsigned int c)
	{
	switch (c)
		{
		case ESC: vrl_SystemStopRunning(); break;
		default:
			if (islower(c))
				vrl_LayerToggle(c - 'a');
			break;
		}
	vrl_SystemRequestRefresh();
	}

void vrl_ReadWLDfeature(int argc, char *argv[], char *rawtext)
	{
	static Area *current = NULL;
	if (argc < 1) return;
	if (!stricmp(argv[0], "area") && argc > 1)
		{
		current = area_create(atoi(argv[1]));
		if (current && argc > 2)
			area_setname(current, strdup(argv[2]));
		}
	else if (!stricmp(argv[0], "boundary") && argc > 4 && current)
		area_add_boundary(current, atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));
	else if (!stricmp(argv[0], "floor") && argc > 4 && current)
		{
		Boundary *f = area_add_boundary(current, atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));
		if (f) area_setfloor(current, f);
		}
	else if (!stricmp(argv[0], "passage") && argc > 2 && current)
		area_add_passage(current, atoi(argv[1]), (argc > 2) ? atoi(argv[2]) : 0);
	else if (!stricmp(argv[0], "visarea") && argc > 2 && current)
		area_add_visarea(current, atoi(argv[1]), (argc > 2) ? atoi(argv[2]) : 0);
	else if (!stricmp(argv[0], "flag") && argc > 2)
		game_flags[atoi(argv[1])] = atoi(argv[2]);
	else if (!stricmp(argv[0], "selectable") && argc > 1)
		{
		vrl_Object *obj = vrl_WorldFindObject(argv[1]);
		if (obj)
			{
			selectable_entry *sel = vrl_malloc(sizeof(selectable_entry));
			if (sel)
				{
				sel->obj = obj;
				sel->next = sel_list;
				sel_list = sel;
				}
			}
		}
	}

void vrl_ApplicationDrawOver(vrl_RenderStatus *stat)
	{
	int cx = vrl_DisplayGetWidth() / 2, cy = vrl_DisplayGetHeight() / 2;
	/* draw crosshairs */
	vrl_DisplayLine(cx - 5, cy, cx + 5, cy, 255);
	vrl_DisplayLine(cx, cy - 5, cx, cy + 5, 255);
	vrl_UserInterfaceDropText(135, 10, 255, "Boris!");
	}
