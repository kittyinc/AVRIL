/* 7th Sense tracker support for AVRIL */
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
#include <dos.h>  /* inp() and friends */

static vrl_DeviceChannel ss_channel_defaults[] =
	{
		/* center, deadzone, range, scale, accum */
		{ 0, 0, 1, float2scalar(0), 0 },  /* trans X */
		{ 0, 0, 1, float2scalar(0), 0 },  /* trans Y */
		{ 0, 0, 1, float2scalar(0), 0 },  /* trans Z */
		{ 0, 10, -600, float2angle(60), 0 },   /* rot X */
		{ 0, 10, 3600, float2angle(360), 0 },  /* rot Y */
		{ 0, 10, 600, float2angle(60), 0 }     /* rot Z */
	};

static vrl_unsigned16bit base_addresses[] = { 0x260, 0x250, 0x240, 0x230 };

static read_byte(vrl_unsigned16bit vrbase, int idx)
	{
	int count;
	outp(vrbase, idx);
	outp(vrbase+7, 0);
	for (count = 0; inp(vrbase) & 0x01; ++count)
		if (count > 1000)
			return 0;
	return inp(vrbase+7);
	}

static int read_data(vrl_unsigned16bit vrbase, int ind)
	{
	return (read_byte(vrbase, ind) << 8) | read_byte(vrbase, ind & 0xEF);
	}

int vrl_7thSenseDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	int i;
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			device->nchannels = 6;
			device->channels = vrl_calloc(device->nchannels, sizeof(vrl_DeviceChannel));
			if (device->channels == NULL) return -1;
			device->desc = "7th Sense tracker";
			device->version = 1;
			device->rotation_mode = VRL_MOTION_ABSOLUTE;
			device->translation_mode = VRL_MOTION_NONE;
			for (i = 0; i < 4; ++i)
				if (inp(base_addresses[i]) == 0xF8)
					break;
			device->localdata = (void *) base_addresses[i];
		case VRL_DEVICE_RESET:
			memcpy(device->channels, &ss_channel_defaults, sizeof(ss_channel_defaults));
			return 0;
		case VRL_DEVICE_POLL:
			{
			vrl_unsigned16bit vrbase = (vrl_unsigned16bit) device->localdata;
			device->channels[XROT].rawvalue = read_data(vrbase, 0xD8);
			device->channels[YROT].rawvalue = read_data(vrbase, 0x98);
			device->channels[ZROT].rawvalue = read_data(vrbase, 0xB8);
			device->channels[XROT].changed = 1;
			device->channels[YROT].changed = 1;
			device->channels[ZROT].changed = 1;
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
