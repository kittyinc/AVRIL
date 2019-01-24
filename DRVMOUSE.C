/* Mouse device support for AVRIL */
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

static vrl_DeviceChannel mousechannel_defaults[] =
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

static vrl_Buttonmap default_buttonmap =
	{ { YROT, Z }, { X, Y }, { ZROT, XROT }, { 6, 7 } };

int vrl_MouseDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			{
			int *centerdata;
			if (vrl_MouseInit())
				return -2;
			device->nchannels = 8;
			device->channels = vrl_calloc(device->nchannels, sizeof(vrl_DeviceChannel));
			device->localdata = vrl_calloc(2, sizeof(int));
			if (device->channels == NULL) return -1;
			if (device->localdata == NULL)
				{
				vrl_free(device->channels);
				device->channels = NULL;
				return -1;
				}
			centerdata = device->localdata;
			vrl_MouseRead(&centerdata[X], &centerdata[Y], NULL);
			device->desc = "Mouse";
			device->version = 1;
			vrl_MouseSetPointer(device);  /* give the mouse routines a pointer to us */
			}  /* fall through */
		case VRL_DEVICE_RESET:
			{
			int *centerdata = device->localdata;
			device->buttonmap = &default_buttonmap;
			memcpy(device->channels, mousechannel_defaults, sizeof(mousechannel_defaults));
			device->channels[X].centerpoint = centerdata[X];
			device->channels[Y].centerpoint = centerdata[Y];
			device->channels[Z].centerpoint = centerdata[Y];
			device->channels[XROT].centerpoint = centerdata[Y];
			device->channels[YROT].centerpoint = centerdata[X];
			device->channels[ZROT].centerpoint = centerdata[X];
			}
			return 0;
		case VRL_DEVICE_POLL:
			{
			int x, y, i;
			unsigned int buttons;
			if (!vrl_MouseGetUsage())
				{
				for (i = 0; i < device->nchannels; ++i)
					device->channels[i].value = 0;
				return 0;
				}
			for (i = 0; i < device->nchannels; ++i)
				{
				device->channels[i].rawvalue = device->channels[i].centerpoint;
				device->channels[i].changed = 1;
				}
			vrl_MouseRead(&x, &y, &buttons);
			if ((*device->buttonmap)[buttons][X] == -1 && (*device->buttonmap)[buttons][Y] == -1)
				vrl_MouseDevice(VRL_DEVICE_RESET, device);
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
			return 0;
		default: break;
		}
	return 1;  /* function not implemented */
	}
