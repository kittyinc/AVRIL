/* InWorld CyberWand device support for AVRIL */
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

static vrl_DeviceChannel cyberwandchannel_defaults[] =
	{
		/* center, deadzone, range, scale, accum */
		{ 0, 0, 1, float2scalar(1000), 1 },  /* trans X */
		{ 0, 0, 1, float2scalar(1000), 1 },  /* trans Y */
		{ 0, 0, 1, float2scalar(1000), 1 },  /* trans Z */
		{ 0, 0, 1, float2angle(20), 1 },  /* rot X */
		{ 0, 0, 1, float2angle(20), 1 },  /* rot Y */
		{ 0, 0, 1, float2angle(20), 1 },  /* rot Z */
		{ 0, 0, 1, float2scalar(1000), 1 },  /* spare X */
		{ 0, 0, 1, float2scalar(1000), 1 }   /* spare Y */
	};

static vrl_unsigned16bit port_addresses[] =
	{ 0x201, 0x209, 0x203, 0x20B, 0x205, 0x20D, 0x207, 0x20F };

static vrl_16bit lowlevel_read(vrl_unsigned16bit portaddr)
	{
	vrl_16bit value = 0;
	disable();
	outp(portaddr, (unsigned char) 0x01);  /* trigger CyberWand */
	for (value = 0; value < 1000; ++value)
		if ((inp(portaddr) & 0x08) == 0)
			break;
	enable();
	return value;
	}

static vrl_Buttonmap default_buttonmap =
	{
	{ X, Z }, { ZROT, Y }, { YROT, XROT }, { 6, 7 }
	};

int vrl_CyberwandDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	vrl_unsigned16bit portaddr = port_addresses[(int) device->port];
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			{
			int c;
			outp(portaddr, (unsigned char) 0xFF);  /* trigger CyberWand */
			c = inp(portaddr) & 0x08;
			vrl_TimerDelay(3);
			c ^= (inp(portaddr) & 0x80);  /* see if bit has changed */
			if (c == 0) return -2;  /* no bits changed, so no CyberWand found */
			device->nbuttons = 4;
			device->nchannels = 8;
			device->channels = vrl_calloc(device->nchannels, sizeof(vrl_DeviceChannel));
			if (device->channels == NULL) return -1;
			device->localdata = malloc(sizeof(vrl_16bit));
			if (device->localdata == NULL) return -1;
			*((vrl_16bit *) device->localdata) = lowlevel_read(portaddr) / 82;
			device->period = vrl_TimerGetTickRate() / 10;  /* up to ten reads/second */
			device->desc = "InWorld CyberWand";
			device->version = 1;
			}
			/* fall through */
		case VRL_DEVICE_RESET:
			device->buttonmap = &default_buttonmap;
			memcpy(device->channels, cyberwandchannel_defaults, sizeof(cyberwandchannel_defaults));
			return 0;
		case VRL_DEVICE_POLL:
			{
			int i;
			vrl_unsigned16bit buttons, map_buttons;
			vrl_16bit value = lowlevel_read(portaddr), x = 0, y = 0;
			vrl_16bit divisor = *((vrl_16bit *) device->localdata);
			if (divisor) value /= divisor;
			if (value < 10) y = 1;
			else if (value <= 30) x = 1;
			else if (value <= 50) y = -1;
			else if (value <= 75) x = -1;
			buttons = ((~inp(portaddr)) >> 4) & 0x0F;
			for (i = 0; i < device->nchannels; ++i)
				{
				device->channels[i].rawvalue = device->channels[i].centerpoint;
				device->channels[i].changed = 1;
				}
			device->bchanged = device->buttons ^ buttons;
			device->buttons = buttons;
			map_buttons = (buttons >> 2) & 0x03;
			/* check mode */
			if (device->mode)
				{
				device->channels[X].rawvalue = x;
				device->channels[Y].rawvalue = y;
				return 0;
				}
			if ((*device->buttonmap)[map_buttons][X] == -1 && (*device->buttonmap)[buttons][Y] == -1)
				vrl_CyberwandDevice(VRL_DEVICE_RESET, device);
			else
				{
				device->channels[(*device->buttonmap)[map_buttons][X]].rawvalue = x;
				device->channels[(*device->buttonmap)[map_buttons][Y]].rawvalue = y;
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

