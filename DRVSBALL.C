/* Spaceball support for AVRIL */
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

static vrl_DeviceChannel spacebchannel_defaults[] =
	{
		/* center, deadzone, range, scale, accum */
		{ 0, 0, 32768 * 40L, float2scalar(1000), 1 },  /* trans X */
		{ 0, 0, 32768 * 40L, float2scalar(1000), 1 },  /* trans Y */
		{ 0, 0, 32768 * 40L, float2scalar(1000), 1 },  /* trans Z */
		{ 0, 0, -32768 * 40L, float2angle(20), 1 },  /* rot X */
		{ 0, 0, -32768 * 40L, float2angle(20), 1 },  /* rot Y */
		{ 0, 0, -32768 * 40L, float2angle(20), 1 }   /* rot Z */
	};

static int get_Spaceball_packet(vrl_SerialPort *port, vrl_DevicePacketBuffer *buff)
	{
	while (vrl_SerialCheck(port))
		{
		int c;
		c = vrl_SerialGetc(port) & 0xFF;
		switch (c)
			{
			case 0x0D:  /* end of packet */
				{
				int n = buff->ind;
				buff->ind = 0;
				return n;
				}
			case '^':   /* escape character */
				c = vrl_SerialGetc(port);
				if (c != '^')  /* not another '^', so this character is CTRLed */
					c &= 0x1F;
				break;
			case 17: /* control-Q */
			case 19: /* control-S */
				continue;    /* ignore flow control */
			default: break;
			}
		if (buff->ind < buff->buffsize)
			buff->buffer[buff->ind++] = c;
		}
	return 0;  /* no data waiting, packet not yet complete */
	}

int vrl_SpaceballOutput(vrl_Device *dev, int parm1, vrl_Scalar parm2)
	{
	vrl_32bit value = (vrl_32bit) scalar2float(parm2);
	if (parm1) return -1;
	vrl_SerialPutc('B', dev->port);
	vrl_SerialPutc(value ? (((value >> 3) & 0x1F) | 0x20) : 0, dev->port);
	vrl_SerialPutc('\r', dev->port);
	return 0;
	}

int vrl_SpaceballDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			if (device->port == NULL) return -4;
			device->nchannels = 6;
			device->channels = vrl_calloc(device->nchannels, sizeof(vrl_DeviceChannel));
			if (device->channels == NULL) return -1;
			device->localdata = vrl_DeviceCreatePacketBuffer(50);
			if (device->localdata == NULL)
				{
				vrl_free(device->channels);
				device->channels = NULL;
				return -1;
				}
			device->nbuttons = 9;
			device->noutput_channels = 1;
			device->outfunc = vrl_SpaceballOutput;
			device->desc = "Spaceball";
			device->version = 1;
		case VRL_DEVICE_RESET:
			vrl_SerialSetParameters(device->port, 9600, VRL_PARITY_NONE, 8, 1);
			vrl_SerialPutc(17, device->port);  /* XON */
			vrl_SerialSetDTR(device->port, 1);
			vrl_SerialSetRTS(device->port, 1);
			vrl_SerialFlush(device->port);
			vrl_SerialPutString("@RESET\r\r", device->port);
			vrl_TimerDelay(2000);
			while (vrl_SerialCheck(device->port))
				if (vrl_SerialGetc(device->port) == '@')
					if (vrl_SerialCheck(device->port))
						if (vrl_SerialGetc(device->port) == '1')
							break;
			if (!vrl_SerialCheck(device->port)) return -2;  /* didn't see "@1" */
			vrl_SerialFlush(device->port);
			/* enable Spatial freedom for both translation and rotation,
			   set Vector mode for rotations, Left-handed coordinates, and
			   multiply matrices Afterwards */
			vrl_SerialPutString("MSSVLA\r", device->port);
			vrl_TimerDelay(500);
			vrl_SerialPutString("CB\r", device->port);  /* Communications mode Binary */
			vrl_SerialPutString("Z\r", device->port);   /* Zero the device */
			/* set minimum and maximum packet rates to be 40 milliseconds */
			vrl_SerialPutc('P', device->port);
			vrl_SerialPutc(00, device->port);
			vrl_SerialPutc(40, device->port);
			vrl_SerialPutc(00, device->port);
			vrl_SerialPutc(40, device->port);
			vrl_SerialPutc('\r', device->port);
			vrl_TimerDelay(500);
			vrl_SerialFlush(device->port);
			memcpy(device->channels, spacebchannel_defaults, sizeof(spacebchannel_defaults));
			return 0;
		case VRL_DEVICE_POLL:
			while (get_Spaceball_packet(device->port, device->localdata))
				{
				unsigned char *p = vrl_DevicePacketGetBuffer(device->localdata);
				switch (p[0])
					{
					case 'K':
						{
						unsigned int b = ((p[1] & 0x1F) << 4) | (p[2] & 0x0F);
						device->bchanged = b ^ device->buttons;
						device->buttons = b;
						}
						break;
					case 'D':
						{
						int i;
						vrl_32bit period = (p[1] << 8) | p[2];
						device->channels[X].rawvalue = period * ((p[3] << 8) | p[4]);
						device->channels[Y].rawvalue = period * ((p[5] << 8) | p[6]);
						device->channels[Z].rawvalue = period * ((p[7] << 8) | p[8]);
						device->channels[XROT].rawvalue = period * ((p[9] << 8) | p[10]);
						device->channels[YROT].rawvalue = period * ((p[11] << 8) | p[12]);
						device->channels[ZROT].rawvalue = period * ((p[13] << 8) | p[14]);
						for (i = 0; i < device->nchannels; ++i)
							device->channels[i].changed = 1;
						}
						break;
					default: break;
					}
				}
			return 0;
		case VRL_DEVICE_QUIT:
			vrl_SerialPutc(19, device->port);  /* XOFF */
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
