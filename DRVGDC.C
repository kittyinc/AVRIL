/* Global Devices Controller support for AVRIL */
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

static vrl_DeviceChannel gdcchannel_defaults[] =
	{
		/* center, deadzone, range, scale, accum */
		{ 0, 0,  1, float2scalar(1000), 1 },  /* trans X */
		{ 0, 0,  1, float2scalar(1000), 1 },  /* trans Y */
		{ 0, 0,  1, float2scalar(1000), 1 },  /* trans Z */
		{ 0, 0,  1, float2angle(20), 1 },  /* rot X */
		{ 0, 0, -1, float2angle(20), 1 },  /* rot Y */
		{ 0, 0,  1, float2angle(20), 1 }   /* rot Z */
	};

int vrl_GlobalOutput(vrl_Device *dev, int parm1, vrl_Scalar parm2)
	{
	vrl_32bit value = (vrl_32bit) scalar2float(parm2);
	if (parm1) return -1;
	vrl_SerialPutc(0xA0 | ((value >> 4) & 0x0F), dev->port);
	vrl_TimerDelay(20);
	return 0;
	}

int vrl_GlobalDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			if (device->port == NULL) return -4;
			device->nchannels = 6;
			device->channels = vrl_calloc(device->nchannels, sizeof(vrl_DeviceChannel));
			if (device->channels == NULL) return -1;
			device->noutput_channels = 1;
			device->outfunc = vrl_GlobalOutput;
			device->desc = "GDC controller";
			device->version = 1;
		case VRL_DEVICE_RESET:
			vrl_SerialSetParameters(device->port, 9600, VRL_PARITY_NONE, 8, 1);
			vrl_SerialPutc(0x8E, device->port);  /* single-byte exception mode */
			vrl_TimerDelay(20);
			if (!vrl_SerialCheck(device->port)) return -2;  /* no response */
			if (vrl_SerialGetc(device->port) != 0x3E) return -3;  /* bad config byte */
			memcpy(device->channels, gdcchannel_defaults, sizeof(gdcchannel_defaults));
			return 0;
		case VRL_DEVICE_POLL:
			while (vrl_SerialCheck(device->port))
				{
				int c, invc;
				unsigned int b;
				c = vrl_SerialGetc(device->port);
				invc = ~c;
				switch (c & 0xC0)
					{
					case 0x80:
						b = invc & 0x3F;
						device->bchanged = b ^ device->buttons;
						device->buttons = b;
						break;
					case 0x40:  /* linear byte */
						device->channels[X].rawvalue = (invc & 0x04) ? 1 : ((invc & 0x08) ? -1 : 0);
						device->channels[Y].rawvalue = (invc & 0x10) ? 1 : ((invc & 0x20) ? -1 : 0);
						device->channels[Z].rawvalue = (invc & 0x01) ? 1 : ((invc & 0x02) ? -1 : 0);
						device->channels[X].changed = 1;
						device->channels[Y].changed = 1;
						device->channels[Z].changed = 1;
						break;
					case 0xC0:  /* rotational byte */
						device->channels[XROT].rawvalue = (invc & 0x04) ? -1 : ((invc & 0x08) ? 1 : 0);
						device->channels[YROT].rawvalue = (invc & 0x01) ? -1 : ((invc & 0x02) ? 1 : 0);
						device->channels[ZROT].rawvalue = (invc & 0x10) ? -1 : ((invc & 0x20) ? 1 : 0);
						device->channels[XROT].changed = 1;
						device->channels[YROT].changed = 1;
						device->channels[ZROT].changed = 1;
						break;
					default: break;
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
