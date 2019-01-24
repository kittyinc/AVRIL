/* 5th Dimension glove support for AVRIL */
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
#include "abus.h"    /* include dos driver prototypes */

static vrl_DeviceChannel fifthchannel_defaults[] =
	{
		/* center, deadzone, range, scale, accum */
		{ 0, 0, 1, float2scalar(0), 0 },  /* trans X */
		{ 0, 0, 1, float2scalar(0), 0 },  /* trans Y */
		{ 0, 0, 1, float2scalar(0), 0 },  /* trans Z */
		{ 0, 0, 1, float2scalar(0), 0 },  /* rot X */
		{ 0, 0, 1, float2scalar(0), 0 },  /* rot Y */
		{ 0, 0, 1, float2scalar(0), 0 },  /* rot Z */
		/* sensors */
		{ -1800, 250, 2048, float2angle(90), 0 },
		{ -1800, 250, 2048, float2angle(90), 0 },
		{ -1800, 250, 2048, float2angle(90), 0 },
		{ -1800, 250, 2048, float2angle(90), 0 },
		{ -1800, 250, 2048, float2angle(90), 0 },
		{ -1800, 250, 2048, float2angle(90), 0 },
		{ -1800, 250, 2048, float2angle(90), 0 },
		{ -1800, 250, 2048, float2angle(90), 0 }
	};

int vrl_FifthDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	int i;
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			device->nchannels = 14;   /* bottom six unused for now */
			device->channels = vrl_calloc(device->nchannels, sizeof(vrl_DeviceChannel));
			if (device->channels == NULL) return -1;
			device->desc = "5th Dimension Glove";
			device->version = 1;
			device->rotation_mode = VRL_MOTION_NONE;
			device->translation_mode = VRL_MOTION_NONE;
		case VRL_DEVICE_RESET:
			if (!ABusDetect()) return -2;
			device->localdata = lpABusCardAddr;
			memcpy(device->channels, &fifthchannel_defaults, sizeof(fifthchannel_defaults));
			for (i = 0; i < 8; ++i)
				if (ABusDevicePresent(((int) device->port)+i))
					device->channels[6+i].centerpoint = ABusGetValue(((int) device->port)+i);
			return 0;
		case VRL_DEVICE_SET_RANGE:
			for (i = 0; i < 8; ++i)
				if (ABusDevicePresent(((int) device->port)+i))
					device->channels[6+i].range =
						ABusGetValue(((int) device->port)+i) - device->channels[6+i].centerpoint;
			return 0;
		case VRL_DEVICE_POLL:
			for (i = 0; i < 8; ++i)
				if (ABusDevicePresent(((int) device->port)+i))
					{
					device->channels[6+i].rawvalue = ABusGetValue(((int) device->port)+i);
					device->channels[6+i].changed = 1;
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
