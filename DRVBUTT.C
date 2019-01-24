/* Special device for reading mouse and/or joystick buttons */
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
#include <dos.h>  /* inp() */

static vrl_DeviceChannel channel_defaults[] =
	{
		/* center, deadzone, range, scale, accum */
		{ 160, 5,  319, float2scalar(1000), 1 },  /* trans X */
		{ 100, 5, -199, float2scalar(1000), 1 },  /* trans Y */
		{ 100, 5, -199, float2scalar(1000), 1 },  /* trans Z */
		{ 100, 5,  199, float2angle(40), 1 },  /* rot X */
		{ 160, 5,  319, float2angle(40), 1 },  /* rot Y */
		{ 160, 5,  319, float2angle(40), 1 },  /* rot Z */
		{ 160, 5,  319, float2scalar(1000), 0 },  /* spare X */
		{ 100, 5,  199, float2scalar(1000), 0 }   /* spare Y */
	};

int vrl_ControlDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			if (vrl_MouseInit())
				return -2;
			device->nchannels = 0;
			device->nbuttons = 4;
			device->channels = channel_defaults;
			device->desc = "Button device";
			device->version = 1;
			return 0;
		case VRL_DEVICE_POLL:
			{
			unsigned int buttons = 0;
			vrl_MouseRead(NULL, NULL, &buttons);
			buttons |= ((~inp(0x201)) >> 4) & 0x0F;
			device->buttons = buttons & 0x0F;
			}
			return 0;
		default: break;
		}
	return 1;  /* function not implemented */
	}
