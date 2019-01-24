/* Simple user-interface routines for AVRIL */
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

/* These routines will eventually be completely replaced by a better GUI */

#include "avril.h"
#include <stdlib.h>   /* min() */
#include <ctype.h>    /* isupper() */
#include <string.h>   /* strlen() */

/* Colors */

#define INTERIOR_COLOR   250
#define TEXT_COLOR	     234
#define TEXT_HIGHCOLOR     0

static void internal_box(int x1, int y1, int x2, int y2, vrl_Color color)
	{
	vrl_OutputVertex v[4];
	v[0].next = &v[1];  v[0].prev = &v[3];
	v[1].next = &v[2];  v[1].prev = &v[0];
	v[2].next = &v[3];  v[2].prev = &v[1];
	v[3].next = &v[0];  v[3].prev = &v[2];
	v[0].x = x1;  v[0].y = y1;  v[0].red = color;
	v[1].x = x2;  v[1].y = y1;  v[1].red = color;
	v[2].x = x2;  v[2].y = y2;  v[2].red = color;
	v[3].x = x1;  v[3].y = y2;  v[3].red = color;
	(*vrl_current_display_driver)(VRL_DISPLAY_POLY, VRL_SURF_SIMPLE, v);
	}

void vrl_UserInterfaceBox(int w, int h, int *xp, int *yp)
	{
	vrl_ScreenPos x, y, wleft, wtop, wright, wbottom;
	/* the "& 0xFFFC" in the next line ensures we're on a four-pixel
	   boundary; needed for VGA mode X
	 */
	vrl_DisplayGetWindow(&wleft, &wtop, &wright, &wbottom);
	x = (((wright - wleft + 1) - w) / 2) & 0xFFF8;
	y = ((wbottom - wtop + 1) - h) / 2;
	vrl_VideoCursorHide();
	internal_box(x-2, y-2, x+w+8, y+h+8, 0);
	internal_box(x-5, y-5, x+w+5, y+h+5, INTERIOR_COLOR);
	vrl_DisplayLine(x-3, y-3, x+w+3, y-3, 0);
	vrl_DisplayLine(x-3, y+h+3, x+w+3, y+h+3, 0);
	vrl_DisplayLine(x-3, y-3, x-3, y+h+3, 0);
	vrl_DisplayLine(x+w+3, y-3, x+w+3, y+h+3, 0);
	if (xp) *xp = x;
	if (yp) *yp = y;
	vrl_VideoCursorShow();
	}

void vrl_UserInterfacePopMsg(char *msg)
	{
	int x, y;
	vrl_UserInterfaceBox(
		vrl_DisplayGetTextWidth(msg) + vrl_DisplayGetTextWidth("WW"),
		vrl_DisplayGetTextHeight(msg) + vrl_DisplayGetTextHeight("W"),
		&x, &y);
	vrl_VideoCursorHide();
	vrl_DisplayText(
		x+vrl_DisplayGetTextWidth("W"),	y+vrl_DisplayGetTextHeight("W")/2,
		TEXT_COLOR, msg);
	vrl_VideoCursorShow();
	}

void vrl_UserInterfacePopText(char *text[])
	{
	int i, h = 0, w = 0, x, y;
	vrl_ScreenPos wleft, wtop, wright, wbottom;
	/* compute size needed for box */
	for (i = 0; text[i]; ++i)
		{
		int wid = vrl_DisplayGetTextWidth(text[i]);
		if (wid > w) w = wid;
		h += vrl_DisplayGetTextHeight(text[i]);
		}
	vrl_DisplayGetWindow(&wleft, &wtop, &wright, &wbottom);
	w = min(w, wright - wleft + 1 - 20);
	h = min(h, wbottom - wtop + 1 - 20);
	vrl_UserInterfaceBox(w, h, &x, &y);
	vrl_VideoCursorHide();
	for (i = 0; text[i]; ++i)
		{
		vrl_DisplayText(x, y, TEXT_COLOR, text[i]);
		y += vrl_DisplayGetTextHeight(text[i]);
		}
	vrl_VideoCursorShow();
	}

int vrl_UserInterfacePopMenu(char *text[])
	{
	int i, h = 0, w = 0, x, y, yt, c, mx, my;
	vrl_ScreenPos wleft, wtop, wright, wbottom;
	unsigned int buttons;
	/* compute size needed for box */
	for (i = 0; text[i]; ++i)
		{
		int wid = vrl_DisplayGetTextWidth(text[i]);
		if (wid > w) w = wid;
		h += vrl_DisplayGetTextHeight(text[i]);
		}
	vrl_DisplayGetWindow(&wleft, &wtop, &wright, &wbottom);
	w = min(w, wright - wleft + 1 - 20);
	h = min(h, wbottom - wtop + 1 - 20);
	vrl_UserInterfaceBox(w, h, &x, &y);
	/* now actually display the menu */
	yt = y;
	vrl_VideoCursorHide();
	for (i = 0; text[i]; ++i)
		{
		int j;
		vrl_DisplayText(x, yt, TEXT_COLOR, text[i]);
		/* highlight the first uppercase character; assumes monospace font */
		for (j = 0; text[i][j]; ++j)
			if (isupper(text[i][j]))
				{
				char up[2];
				up[0] = text[i][j]; up[1] = '\0';
				vrl_DisplayText(x + j * vrl_DisplayGetTextWidth("W"), yt, TEXT_HIGHCOLOR, up);
				break;
				}
		yt += vrl_DisplayGetTextHeight(text[i]);
		}
	vrl_VideoCursorShow();
	do {  /* if buttons are already down, wait for them to go up */
		vrl_MouseRead(NULL, NULL, &buttons);
		} while (buttons);
	/* look for mouse clicks or keypresses */
	while (!buttons)
		{
		vrl_MouseRead(NULL, NULL, &buttons);
		if (vrl_KeyboardCheck())
			{
			int i;
			c = toupper(vrl_KeyboardRead());
			if (c == 0x1B) return 0;
			for (i = 0; text[i]; ++i)
				{
				char *p = text[i];
				while (*p)
					if (isupper(*p) && *p == toupper(c))
						return c;
					else
						++p;
				}
			}
		}
	while (buttons)  /* wait for release */
		vrl_MouseRead(&mx, &my, &buttons);
	/* if outside of box, quit */
	if (mx < x || mx >= (x + w) || my < y || my >= (y + h))
		return 0;
	yt = y;
	for (i = 0; text[i]; ++i)
		{
		yt += vrl_DisplayGetTextHeight(text[i]);
		if (my < yt)
			{
			char *p = text[i];
			while (*p)
				if (isupper(*p))
					return *p;
				else
					++p;
			return 0;
			}
		}
	return 0;
	}

int vrl_UserInterfaceMenuDispatch(char *text[], int (**funcs)(void))
	{
	int n = vrl_UserInterfacePopMenu(text);
	if (n < 0) return n;
	return (*funcs[n])();
	}

unsigned vrl_UserInterfacePopPrompt(char *prompt, char *buff, int n)
	{
	unsigned c;
	int x, y, i;
	if (n + strlen(prompt) > 36)
		n = 36 - strlen(prompt);
	vrl_UserInterfaceBox(
		vrl_DisplayGetTextWidth(prompt) + n * vrl_DisplayGetTextWidth("W"),
		vrl_DisplayGetTextHeight(prompt) + vrl_DisplayGetTextHeight("W")/2, &x, &y);
	vrl_VideoCursorHide();
	vrl_DisplayText(x, y, TEXT_COLOR, prompt);
	x += vrl_DisplayGetTextWidth(prompt);
	buff[i = 0] = '\0';
	do {
		switch (c = vrl_KeyboardRead())
			{
			case 0x1B: buff[0] = 0; break;
			case '\b':
				if (i > 0)
					{
					buff[--i] = '\0';
					vrl_DisplayBox(x, y, x + vrl_DisplayGetTextWidth(buff),
						y + vrl_DisplayGetTextHeight(buff), INTERIOR_COLOR);
					vrl_DisplayText(x, y, TEXT_COLOR, buff);
					}
				break;
			default:
				if (isprint(c) && i < n)
					{
					buff[i++] = c;
					buff[i] = '\0';
					vrl_DisplayText(x, y, TEXT_COLOR, buff);
					}
				break;
			}
		} while (c != '\r' && c != '\n' && c != 0x1B);
	vrl_VideoCursorShow();
	return c;
	}

static int axiscolors[] = { 15, 13, 10 };
static char *axisnames[] = { "x", "y", "z" };

void vrl_UserInterfaceDrawCompass(vrl_Camera *camera, int compass_x, int compass_y, int compass_armlen)
	{
	int x, y, axis;
	vrl_Object *obj;
	if (camera == NULL) return;
	obj = vrl_CameraGetObject(camera);
	if (obj == NULL) return;
	for (axis = 0; axis < 3; ++axis)
		{
		x = scalar2float(vrl_FactorMultiply(obj->globalmat[axis][X], compass_armlen));
		y = scalar2float(vrl_FactorMultiply(obj->globalmat[axis][Y], compass_armlen));
		vrl_DisplayLine(compass_x, compass_y, compass_x + x, compass_y - y, 0);
		vrl_DisplayText(compass_x + x + 5, compass_y - y, 0, axisnames[axis]);
		vrl_DisplayLine(compass_x + 1, compass_y + 1, compass_x + x + 1, compass_y - y + 1, axiscolors[axis]);
		vrl_DisplayText(compass_x + x + 5 + 1, compass_y - y + 1, 255, axisnames[axis]);
		}
	}

void vrl_UserInterfaceDropText(int x, int y, vrl_Color color, char *text)
	{
	vrl_DisplayText(x + 1, y + 1, 0, text);  /* draw shadow */
	vrl_DisplayText(x, y, color, text);      /* draw text */
	}

int vrl_UserInterfaceDismiss(void)
	{
	while (!vrl_KeyboardCheck())
		{
		unsigned int buttons;
		vrl_MouseRead(NULL, NULL, &buttons);
		if (buttons)
			{
			while (buttons)  /* wait for release */
				vrl_MouseRead(NULL, NULL, &buttons);
			return 0;
			}
		}
	return vrl_KeyboardRead();
	}
