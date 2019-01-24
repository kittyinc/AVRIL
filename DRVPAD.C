/* Game pad support for AVRIL; uses adapter cable from July 1990 Byte */
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
#include <dos.h>

static vrl_DeviceChannel padchannel_defaults[] =
	{
		/* center, deadzone, range, scale, accum */
		{ 0, 0,  1, float2scalar(1000), 1 },  /* trans X */
		{ 0, 0,  1, float2scalar(1000), 1 },  /* trans Y */
		{ 0, 0,  1, float2scalar(1000), 1 },  /* trans Z */
		{ 0, 0,  1, float2angle(20), 1 },  /* rot X */
		{ 0, 0,  1, float2angle(20), 1 },  /* rot Y */
		{ 0, 0,  1, float2angle(20), 1 }   /* rot Z */
	};

#define CLOCK_HI 0x01
#define CLOCK_LO 0x00
#define LATCH_HI 0x02
#define LATCH_LO 0x00

#define DATA_IN  0x10

int vrl_PadDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	int port = device->port ? 0x278 : 0x378;
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			device->nchannels = 6;
			device->channels = vrl_calloc(device->nchannels, sizeof(vrl_DeviceChannel));
			if (device->channels == NULL) return -1;
			device->desc = "Game control pad";
			device->version = 1;
			device->nbuttons = 4;
		case VRL_DEVICE_RESET:
			memcpy(device->channels, padchannel_defaults, sizeof(padchannel_defaults));
			return 0;
		case VRL_DEVICE_POLL:
			{
			unsigned char value = 0;
			int i;
			outp(port, (unsigned char) (LATCH_LO+CLOCK_HI));
			outp(port, (unsigned char) (LATCH_HI+CLOCK_HI));
			outp(port, (unsigned char) (LATCH_LO+CLOCK_HI));
			for (i = 0; i < 8; ++i)
				{
				value = (value << 1) | ((inp(port+1) & DATA_IN) >> 4);
				outp(port, (unsigned char) (LATCH_LO+CLOCK_LO));
				outp(port, (unsigned char) (LATCH_LO+CLOCK_HI));
				}
			value ^= 0xFF;  /* flip the bits */
			device->buttons = value & 0x0F;
			device->channels[YROT].rawvalue = device->channels[Z].rawvalue = 0;
			if (value & 0x01)
				device->channels[YROT].rawvalue = device->channels[YROT].range;
			else if (value & 0x02)
				device->channels[YROT].rawvalue = -device->channels[YROT].range;
			if (value & 0x04)
				device->channels[Z].rawvalue = -device->channels[Z].range;
			else if (value & 0x08)
				device->channels[Z].rawvalue = device->channels[Z].range;
			device->channels[YROT].changed = device->channels[Z].changed = 1;
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
