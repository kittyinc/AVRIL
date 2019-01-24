/* Polhemus Isotrak support for AVRIL */
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

#define PACKETSIZE 20

static vrl_DeviceChannel isochannel_defaults[] =
	{
		/* center, deadzone, range, scale, accum */
		{ 0, 0, 19847, float2scalar(1000), 0 },  /* trans X */
		{ 0, 0, 19847, float2scalar(1000), 0 },  /* trans Y */
		{ 0, 0, 19847, float2scalar(1000), 0 },  /* trans Z */
		{ 0, 0, -25736, float2angle(180), 0 },  /* rot X */
		{ 0, 0, -25736, float2angle(180), 0 },  /* rot Y */
		{ 0, 0, -25736, float2angle(180), 0 }   /* rot Z */
	};

static void unpack(unsigned char *buff)
	{
	buff[0] &= 0x7F;       /* turn off framing bit */
	buff[0] = buff[0] | (((buff[7] >> 0) & 0x01) << 7);
	buff[1] = buff[1] | (((buff[7] >> 1) & 0x01) << 7);
	buff[2] = buff[2] | (((buff[7] >> 2) & 0x01) << 7);
	buff[3] = buff[3] | (((buff[7] >> 3) & 0x01) << 7);  /* X starts here */
	buff[4] = buff[4] | (((buff[7] >> 4) & 0x01) << 7);
	buff[5] = buff[5] | (((buff[7] >> 5) & 0x01) << 7);  /* Y starts here */
	buff[6] = buff[6] | (((buff[7] >> 6) & 0x01) << 7);

	buff[7]  = buff[8]  | (((buff[15] >> 0) & 0x01) << 7);  /* Z starts here */
	buff[8]  = buff[9]  | (((buff[15] >> 1) & 0x01) << 7);
	buff[9]  = buff[10] | (((buff[15] >> 2) & 0x01) << 7);  /* azim starts here */
	buff[10] = buff[11] | (((buff[15] >> 3) & 0x01) << 7);
	buff[11] = buff[12] | (((buff[15] >> 4) & 0x01) << 7);  /* elev starts here */
	buff[12] = buff[13] | (((buff[15] >> 5) & 0x01) << 7);
	buff[13] = buff[14] | (((buff[15] >> 6) & 0x01) << 7);  /* roll starts here */

	buff[14] = buff[16] | ((buff[19] & 0x01) << 7);
    /* 17 and 18 are carriage return, linefeed */
	}

int vrl_IsotrakDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			if (device->port == NULL) return -4;
			device->nchannels = 6;
			device->channels = vrl_calloc(device->nchannels, sizeof(vrl_DeviceChannel));
			if (device->channels == NULL) return -1;
			device->localdata = vrl_DeviceCreatePacketBuffer(PACKETSIZE);
			if (device->localdata == NULL)
				{
				vrl_free(device->channels);
				device->channels = NULL;
				return -1;
				}
			device->desc = "Polhemus Isotrak";
			device->version = 1;
			device->rotation_mode = VRL_MOTION_ABSOLUTE;
			device->translation_mode = VRL_MOTION_ABSOLUTE;
		case VRL_DEVICE_RESET:
			vrl_SerialSetParameters(device->port, 9600, VRL_PARITY_NONE, 8, 1);
			vrl_SerialPutc(19, device->port);   /* XOFF */
			vrl_SerialPutc('f', device->port);  /* format binary */
			vrl_SerialPutc('u', device->port);  /* unit centimeters */
			vrl_SerialPutc('C', device->port);  /* continuous updates */
			vrl_SerialPutc('k', device->port);  /* default output list */
			vrl_SerialPutString("H1,0,0,1\r", device->port);  /* define hemisphere */
			vrl_SerialPutc(17, device->port);   /* XON */
			vrl_SerialFlush(device->port);
			memcpy(device->channels, isochannel_defaults, sizeof(isochannel_defaults));
			return 0;
		case VRL_DEVICE_POLL:
			while (vrl_DeviceGetPacket(device->port, device->localdata))
				{
				unsigned char *p = vrl_DevicePacketGetBuffer(device->localdata);
				unpack(p);
				if (p[0] != '0' || p[1] != '1' || p[18] != 0x0A) return 0;
				device->channels[X].rawvalue = (p[3] & 0xFF) | (p[4] << 8);
				device->channels[X].changed = 1;
				device->channels[Z].rawvalue = (p[5] & 0xFF) | (p[6] << 8);
				device->channels[Z].changed = 1;
				device->channels[Y].rawvalue = (p[7] & 0xFF) | (p[8] << 8);
				device->channels[Y].changed = 1;
				device->channels[YROT].rawvalue = (p[9] & 0xFF) | (p[10] << 8);
				device->channels[YROT].changed = 1;
				device->channels[ZROT].rawvalue = (p[11] & 0xFF) | (p[12] << 8);
				device->channels[ZROT].changed = 1;
				device->channels[XROT].rawvalue = (p[13] & 0xFF) | (p[14] << 8);
				device->channels[XROT].changed = 1;
				}
			return 0;
		case VRL_DEVICE_QUIT:
			vrl_SerialPutc('c', device->port);  /* turn off continuous mode */
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
