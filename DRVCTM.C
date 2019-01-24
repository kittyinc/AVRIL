/* CTM tracker support for AVRIL */
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

static vrl_DeviceChannel ctmchannel_defaults[] =
	{
		/* center, deadzone, range, scale, accum */
		{ 0, 0, 1, float2scalar(0), 0 },  /* trans X */
		{ 0, 0, 1, float2scalar(0), 0 },  /* trans Y */
		{ 0, 0, 1, float2scalar(0), 0 },  /* trans Z */
		{ 16383, 200, -32767, float2angle(180), 0 },   /* rot X */
		{ 0, 200, 32767, float2angle(360), 0, 0 },     /* rot Y */
		{ 16383, 200, -32767, float2angle(180), 0 }    /* rot Z */
	};

typedef struct
	{
	unsigned char previous_char;      /* keeps track of last char received */
	unsigned char buffer[6];          /* 6-byte packet (not counting FF's) */
	int ind;                          /* index into buffer[] */
	} CTM_data;

int vrl_CTMDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	int i;
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			if (device->port == NULL) return -4;
			device->nchannels = 6;
			device->channels = vrl_calloc(device->nchannels, sizeof(vrl_DeviceChannel));
			if (device->channels == NULL) return -1;
			device->localdata = vrl_malloc(sizeof(CTM_data));
			if (device->localdata == NULL)
				{
				vrl_free(device->channels);
				device->channels = NULL;
				return -1;
				}
			device->desc = "CTM tracker";
			device->version = 1;
			device->rotation_mode = VRL_MOTION_ABSOLUTE;
			device->translation_mode = VRL_MOTION_NONE;
		case VRL_DEVICE_RESET:
			memset(device->localdata, 0, sizeof(CTM_data));
			memcpy(device->channels, &ctmchannel_defaults, sizeof(ctmchannel_defaults));
			vrl_SerialSetParameters(device->port, 19200, VRL_PARITY_NONE, 8, 1);
			vrl_SerialFlush(device->port);
			vrl_SerialPutc('F', device->port);  /* binary mode */
			vrl_SerialFlush(device->port);
			vrl_TimerDelay(500);
			return 0;
		case VRL_DEVICE_POLL:
			{
			CTM_data *data = (CTM_data *) device->localdata;
			while (vrl_SerialCheck(device->port))
				{
				unsigned char c = vrl_SerialGetc(device->port);
				if (c == 0xFF && data->previous_char == 0xFF)
					data->ind = 0;  /* two FF's in a row are start of packet */
				else
					{
					if (data->ind < 6)
						{
						data->buffer[data->ind++] = c;   /* store the byte */
						if (data->ind == 6)  /* got complete packet; parse it */
							{
							device->channels[XROT].rawvalue = (data->buffer[2] << 8) | data->buffer[3];
							device->channels[YROT].rawvalue = (data->buffer[0] << 8) | data->buffer[1];
							device->channels[ZROT].rawvalue = (data->buffer[4] << 8) | data->buffer[5];
							device->channels[XROT].changed = 1;
							device->channels[YROT].changed = 1;
							device->channels[ZROT].changed = 1;
							}
						}
					}
				data->previous_char = c;
				}
			}
			return 0;
		case VRL_DEVICE_QUIT:
			vrl_SerialPutc('R', device->port);
			vrl_SerialFlush(device->port);
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
