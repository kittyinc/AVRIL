/* Logitech Red Baron support for AVRIL */
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

static vrl_DeviceChannel redbchannel_defaults[] =
	{
		/* center, deadzone, range, scale, accum */
		{ 0, 0,  1048575, float2scalar(26634), 0 },    /* trans X */
		{ 0, 0,  1048575, float2scalar(26634), 0 },    /* trans Y */
		{ 0, 0, -1048575, float2scalar(26634), 0 },    /* trans Z */
		{ 0, 0, -14400, float2angle(360), 0 },  /* rot X */
		{ 0, 0, -14400, float2angle(360), 0 },  /* rot Y */
		{ 0, 0,  14400, float2angle(360), 0 }   /* rot Z */
	};

static void reset_redbaron(vrl_SerialPort *port)
	{
	vrl_SerialSetDTR(port, 0);  /* DTR low */
	vrl_TimerDelay(200);
	vrl_SerialSetRTS(port, 0);  /* RTS low */
	vrl_TimerDelay(200);
	vrl_SerialSetRTS(port, 1);  /* RTS high */
	vrl_TimerDelay(20);
	}

int vrl_RedbaronDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	int i;
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			if (device->port == NULL) return -4;
			device->nchannels = 6;
			device->channels = vrl_calloc(device->nchannels, sizeof(vrl_DeviceChannel));
			if (device->channels == NULL) return -1;
			device->localdata = vrl_DeviceCreatePacketBuffer(16);
			if (device->localdata == NULL)
				{
				vrl_free(device->channels);
				device->channels = NULL;
				return -1;
				}
			device->nbuttons = 5;
			device->desc = "Logitech Red Baron, direct serial";
			device->version = 1;
			device->rotation_mode = device->translation_mode = VRL_MOTION_ABSOLUTE;
		case VRL_DEVICE_RESET:
			reset_redbaron(device->port);
			vrl_SerialSetParameters(device->port, 19200, VRL_PARITY_NONE, 8, 1);
			vrl_SerialFlush(device->port);
			/* send the reset message */
			vrl_SerialPutc('*', device->port);  vrl_SerialPutc('R', device->port);
			vrl_TimerDelay(1000);
			vrl_SerialFlush(device->port);
			/* request a diagnostic report */
			vrl_SerialPutc('*', device->port);  vrl_SerialPutc(0x05, device->port);
			vrl_TimerDelay(1000);  /* wait for the report to come in */
			if (!vrl_SerialCheck(device->port)) return -2;
			if (vrl_SerialGetc(device->port) != 0xBF) return -3;
			if (!vrl_SerialCheck(device->port)) return -2;
			if (vrl_SerialGetc(device->port) != 0x3F) return -3;
			/* set incremental reporting mode */
			vrl_SerialPutc('*', device->port);
			vrl_SerialPutc('I', device->port);
			vrl_TimerDelay(300);
			vrl_SerialFlush(device->port);
			memcpy(device->channels, &redbchannel_defaults, sizeof(redbchannel_defaults));
			return 0;
		case VRL_DEVICE_POLL:
			while (vrl_DeviceGetPacket(device->port, device->localdata))
				{
				unsigned char *p = vrl_DevicePacketGetBuffer(device->localdata);
				int i;
				unsigned b = p[0] & 0x1F;
				device->bchanged = b ^ device->buttons;
				device->buttons = b;
				device->channels[X].rawvalue = (p[1] << 14) | (p[2] << 7) | p[3] | ((p[1] & 0x40) ? 0xFFE00000 : 0);
				device->channels[Y].rawvalue = (p[4] << 14) | (p[5] << 7) | p[6] | ((p[4] & 0x40) ? 0xFFE00000 : 0);
				device->channels[Z].rawvalue = (p[7] << 14) | (p[8] << 7) | p[9] | ((p[7] & 0x40) ? 0xFFE00000 : 0);
				device->channels[XROT].rawvalue = (p[10] << 7) | p[11];
				device->channels[YROT].rawvalue = (p[12] << 7) | p[13];
				device->channels[ZROT].rawvalue = (p[14] << 7) | p[15];
				for (i = 0; i < 6; ++i)
					device->channels[i].changed = 1;
				}
			return 0;
		case VRL_DEVICE_QUIT:
			reset_redbaron(device->port);
			if (device->localdata)
				{
				vrl_DeviceDestroyPacketBuffer(device->localdata);
				device->localdata = NULL;
				}
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
