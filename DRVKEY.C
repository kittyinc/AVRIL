/* Arrow keypad support for AVRIL  */
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

#define UP_ARROW    0x48
#define DOWN_ARROW  0x50
#define LEFT_ARROW  0x4B
#define RIGHT_ARROW 0x4D
#define PGUP        0x49
#define PGDN        0x51

static int keys[6] = { 0 };  /* up, down, left, right, pgup, pgdn */

static void interrupt (*oldvector)() = NULL;

static void interrupt kbdisr(void)
	{
	int c = inp(0x60);
	switch (c & 0x7F)
		{
		case UP_ARROW: keys[0] = !(c & 0x80); break;
		case DOWN_ARROW: keys[1] = !(c & 0x80); break;
		case LEFT_ARROW: keys[2] = !(c & 0x80); break;
		case RIGHT_ARROW: keys[3] = !(c & 0x80); break;
		case PGUP: keys[4] = !(c & 0x80); break;
		case PGDN: keys[5] = !(c & 0x80); break;
#ifndef __386__
		default: _chain_intr(oldvector);  /* Borland C */
#endif
		}
	outp(0x20, (unsigned char) 0x20);
	}

static vrl_DeviceChannel keychannel_transdefaults =
	{
	0, 0,               /* center, deadzone */
	1, float2scalar(1000), 1          /* range, scale, accumulate */
	};

static vrl_DeviceChannel keychannel_rotdefaults =
	{
	0, 0,         /* center, deadzone */
	1, float2angle(20), 1  /* range, scale, accumulate */
	};

static vrl_Buttonmap default_buttonmap =
	{ { YROT, Z }, { ZROT, XROT }, { X, Y }, { 6, 7 } };

int vrl_KeypadDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	int i;
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			device->channels = vrl_calloc(6, sizeof(vrl_DeviceChannel));
			if (device->channels == NULL) return -1;
			device->nchannels = 6;
			device->desc = "Keypad arrows";
			device->version = 1;
#ifndef __386__
			if (oldvector == NULL)
				oldvector = _dos_getvect(0x09);
		case VRL_DEVICE_RESET:
			_dos_setvect(0x09, kbdisr);
#endif
			device->buttonmap = &default_buttonmap;
			memcpy(&device->channels[X], &keychannel_transdefaults, sizeof(vrl_DeviceChannel));
			memcpy(&device->channels[Y], &keychannel_transdefaults, sizeof(vrl_DeviceChannel));
			memcpy(&device->channels[Z], &keychannel_transdefaults, sizeof(vrl_DeviceChannel));
			memcpy(&device->channels[XROT], &keychannel_rotdefaults, sizeof(vrl_DeviceChannel));
			memcpy(&device->channels[YROT], &keychannel_rotdefaults, sizeof(vrl_DeviceChannel));
			memcpy(&device->channels[ZROT], &keychannel_rotdefaults, sizeof(vrl_DeviceChannel));
			return 0;
		case VRL_DEVICE_POLL:
			{
			unsigned char far *p = MK_FP(0x40, 0x17);  /* shift status byte */
			unsigned int buttons;
			int i;
			int updown = keys[0] ? 1 : (keys[1] ? -1 : 0);
			int leftright = keys[2] ? -1 : (keys[3] ? 1 : 0);
			for (i = 0; i < device->nchannels; ++i)
				{
				device->channels[i].rawvalue = 0;
				device->channels[i].changed = 1;
				}
			buttons = *p & 0x03;  /* left and right shift keys */
			device->channels[(*device->buttonmap)[buttons][X]].rawvalue = leftright;
			device->channels[(*device->buttonmap)[buttons][X]].changed = 1;
			device->channels[(*device->buttonmap)[buttons][Y]].rawvalue = updown;
			device->channels[(*device->buttonmap)[buttons][Y]].changed = 1;
			if (keys[4] || keys[5])
				{
				if (keys[4]) device->channels[Y].rawvalue = 1;
				else if (keys[5]) device->channels[Y].rawvalue = -1;
				}
			}
			return 0;
		case VRL_DEVICE_QUIT:
#ifndef __386__
			if (oldvector)
				_dos_setvect(0x09, oldvector);
#endif
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
