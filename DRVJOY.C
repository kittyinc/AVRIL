/* Joystick device support for AVRIL */
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
#include <stdlib.h>  /* max() */
#include <dos.h>

static vrl_DeviceChannel joystickchannel_defaults[] =
	{
		/* center, deadzone, range, scale, accum */
		{ 0, 50, 1000, float2scalar(1000), 1 },  /* trans X */
		{ 0, 50, 1000, float2scalar(1000), 1 },  /* trans Y */
		{ 0, 50, 1000, float2scalar(1000), 1 },  /* trans Z */
		{ 0, 50, 1000, float2angle(20), 1 },  /* rot X */
		{ 0, 50, 1000, float2angle(20), 1 },  /* rot Y */
		{ 0, 50, 1000, float2angle(20), 1 },  /* rot Z */
		{ 0, 50, 1000, float2scalar(1000), 1 },  /* spare X */
		{ 0, 50, 1000, float2scalar(1000), 1 }   /* spare Y */
	};

struct _limits { int minx, maxx, xcenter, miny, maxy, ycenter; };

static int read_timer(void)
	{
	unsigned int n;
	outp(0x43, (unsigned char) 0);
	n = inp(0x40) & 0xFF;
	return n | (inp(0x40) << 8);
	}

static void lowlevel_read(int *x, int *y,
	unsigned char xmask, unsigned char ymask)
	{
	int start, current, c;
	*x = *y = 0;
	do
		{
		start = read_timer();
		outp(0x201, (unsigned char) 0xFF);  /* trigger joystick */
		inp(0x201);   /* let settle */
		while ((c = inp(0x201) & (xmask | ymask)) != 0)
			{
			current = read_timer();
			if (c & xmask) *x = start - current;
			if (c & ymask) *y = start - current;
			}
		} while (current > start);  /* force re-read if timer wrapped */
	}

static vrl_Buttonmap default_buttonmap =
	{ { YROT, Z }, { X, Y }, { ZROT, XROT }, { 6, 7 } };

int vrl_JoystickDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	unsigned char xmask, ymask, bshift;
	if (device->port)
		{
		xmask = 0x10;
		ymask = 0x20;
		bshift = 6;
		}
	else
		{
		xmask = 0x01;
		ymask = 0x02;
		bshift = 4;
		}
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			{
			struct _limits *lim;
			int c;
			int x, y;
			outp(0x201, (unsigned char) 0xFF);  /* trigger joystick */
			c = inp(0x201) & (xmask | ymask);
			vrl_TimerDelay(3);
			c ^= (inp(0x201) & (xmask | ymask));  /* any bits that changed are now set in c */
			if (c == 0) return -2;  /* no bits changed, so no joystick found */
			device->nbuttons = 2;
			device->nchannels = 8;
			device->channels = vrl_calloc(device->nchannels, sizeof(vrl_DeviceChannel));
			if (device->channels == NULL) return -1;
			device->localdata = vrl_malloc(sizeof(struct _limits));
			if (device->localdata == NULL) return -1;
			device->period = vrl_TimerGetTickRate() / 10;  /* up to ten reads/second */
			device->desc = "Analog joystick";
			device->version = 1;
			/* initialize limits */
			lowlevel_read(&x, &y, xmask, ymask);
			lim = device->localdata;
			lim->minx = lim->maxx = x;
			lim->miny = lim->maxy = y;
			}
			/* fall through */
		case VRL_DEVICE_RESET:
			{
			struct _limits *lim = device->localdata;
			int x, y;
			device->buttonmap = &default_buttonmap;
			memcpy(device->channels, joystickchannel_defaults, sizeof(joystickchannel_defaults));
			/* use current value as center */
			lowlevel_read(&x, &y, xmask, ymask);
			lim->xcenter = x;
			lim->ycenter = y;
			device->channels[X].centerpoint = x;
			device->channels[Y].centerpoint = y;
			device->channels[Z].centerpoint = y;
			device->channels[XROT].centerpoint = y;
			device->channels[YROT].centerpoint = x;
			device->channels[ZROT].centerpoint = x;
			device->channels[6].centerpoint = x;
			device->channels[7].centerpoint = y;
			}
			return 0;
		case VRL_DEVICE_POLL:
			{
			int x, y, i, xrange, yrange;
			unsigned int buttons;
			struct _limits *lim = device->localdata;
			for (i = 0; i < device->nchannels; ++i)
				{
				device->channels[i].rawvalue = device->channels[i].centerpoint;
				device->channels[i].changed = 1;
				}
			lowlevel_read(&x, &y, xmask, ymask);
			buttons = ((~inp(0x201)) >> bshift) & 0x03;
			/* find new limits and set ranges */
			if (x < lim->minx) lim->minx = x;
			else if (x > lim->maxx) lim->maxx = x;
			if (y < lim->miny) lim->miny = y;
			else if (y > lim->maxy) lim->maxy = y;
			xrange = max(abs(lim->maxx - lim->xcenter), abs(lim->xcenter - lim->minx));
			yrange = max(abs(lim->maxy - lim->ycenter), abs(lim->ycenter - lim->miny));
			device->channels[X].range = xrange;
			device->channels[Y].range = -yrange;
			device->channels[Z].range = -yrange;
			device->channels[XROT].range = -yrange;
			device->channels[YROT].range = xrange;
			device->channels[ZROT].range = -xrange;
			device->channels[6].range = xrange;
			device->channels[7].range = -yrange;
			device->bchanged = device->buttons ^ buttons;
			device->buttons = buttons;
			/* check mode */
			if (device->mode)
				{
				device->channels[X].rawvalue = x;
				device->channels[Y].rawvalue = y;
				return 0;
				}
			if ((*device->buttonmap)[buttons][X] == -1 && (*device->buttonmap)[buttons][Y] == -1)
				vrl_JoystickDevice(VRL_DEVICE_RESET, device);
			else
				{
				device->channels[(*device->buttonmap)[buttons][X]].rawvalue = x;
				device->channels[(*device->buttonmap)[buttons][Y]].rawvalue = y;
				}
			}
			return 0;
		case VRL_DEVICE_QUIT:
			if (device->channels)
				{
				vrl_free(device->channels);
				device->channels = NULL;
				}
			if (device->localdata)
				{
				vrl_free(device->localdata);
				device->localdata = NULL;
				}
			return 0;
		default: break;
		}
	return 1;  /* function not implemented */
	}

