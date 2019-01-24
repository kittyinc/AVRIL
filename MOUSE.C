/* Mouse support for AVRIL applications */
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

/*
   The routines in this module are only called by the application, and by
   the vrl_System() functions.  The only exception is the usage stuff, which
   will get called by the mouse vrl_Device.
 */

#include "avril.h"
#include <dos.h>
#include "compat.h"

static vrl_Boolean mouse_installed = 0, mouse_initialized = 0;
static int oldx, oldy;
static unsigned int oldbuttons;
static long dwidth, dheight;

vrl_Boolean vrl_MouseInit(void)
	{
	if (mouse_initialized) return !mouse_installed;
	mouse_initialized = 1;
	return vrl_MouseReset();
	}

void vrl_MouseQuit(void)
	{
	vrl_VideoCursorHide();
	}

vrl_Boolean vrl_MouseReset(void)
	{
	union REGS regs;
	regs.h.ah = regs.h.al = 0;
	int86(0x33, &regs, &regs);
	mouse_installed = regs.h.al;
	if (!mouse_installed) return 1;
	vrl_VideoCursorReset();
	vrl_VideoCursorShow();
	dwidth = vrl_DisplayGetWidth();
	dheight = vrl_DisplayGetHeight();
	return 0;
	}

vrl_Boolean vrl_MouseRead(int *x, int *y, unsigned int *buttons)
	{
	union REGS regs;
	int newx, newy, changed = 0;
	unsigned int newbuttons;
	if (!mouse_installed)
		{
		if (x) *x = 0;
		if (y) *y = 0;
		if (buttons) *buttons = 0;
		return 0;
		}
	regs.h.ah = 0;
	regs.h.al = 3;
	int86(0x33, &regs, &regs);
	newx = (regs.h.ch << 8) | regs.h.cl;
	newy = (regs.h.dh << 8) | regs.h.dl;
	newbuttons = regs.h.bl;
	newx = (newx * dwidth) / 640;   /* convert to screen size */
	newy = (newy * dheight) / 200;  /* convert to screen size */
	if (newx != oldx || newy != oldy || newbuttons != oldbuttons)
		changed = 1;
	if (newx != oldx || newy != oldy)
		vrl_VideoCursorMove(newx, newy);
	oldx = newx;  oldy = newy;  oldbuttons = newbuttons;
	if (x) *x = newx;  if (y) *y = newy;
	if (buttons) *buttons = newbuttons;
	return changed;
	}

/* These next two routines allow the mouse to be used as either a 2D screen-
   oriented pointing device, or a 6D input device */

static int usage = 0;  /* 0 for mouse, 1 for 6D pointing device */

void vrl_MouseSetUsage(int u)
	{
	usage = u;
	}

int vrl_MouseGetUsage(void)
	{
	return usage;
	}

/* These next two routines keep track of which vrl_Device is using the
   mouse; the void * type is used to make these routines less dependent
   on the rest of Avril, but could easily be replaced by a vrl_Device *
   type. */
   
static void *pointer;     /* pointer to whatever's using the mouse */

void vrl_MouseSetPointer(void *u)
	{
	pointer = u;
	}

void *vrl_MouseGetPointer(void)
	{
	return pointer;
	}
