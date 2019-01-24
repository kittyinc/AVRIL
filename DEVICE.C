/* Device support for AVRIL applications */
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
#include <stdlib.h>  /* labs() */
#include <string.h>  /* strdup() */

static vrl_Device *devicelist = NULL;

vrl_Device *vrl_DeviceOpen(vrl_DeviceDriverFunction fn, vrl_SerialPort *port)
	{
	vrl_Device *device;
	device = vrl_malloc(sizeof(vrl_Device));
	if (device == NULL) return NULL;
	device->nickname = "No nickname";
	device->version = 1;  device->desc = "Unknown device";  device->fn = fn;
	device->port = port;  device->mode = 0;
	device->rotation_mode = device->translation_mode = VRL_MOTION_RELATIVE;
	device->period = 40;  device->lastread = 0;
	device->nbuttons = 0;  device->buttons = device->bchanged = 0;
	device->buttonmap = NULL;
	device->nchannels = 0;  device->noutput_channels = 0;
	device->outfunc = NULL;
	device->channels = NULL;  device->localdata = NULL;
	/* note: INIT will return 0 on success, -1 if out of memory, -2 if
	   the device did not respond, and -3 if an unexpected response was
	   received from the device */
	if ((*fn)(VRL_DEVICE_INIT, device))
		{
		vrl_free(device);
		return NULL;
		}
	device->next = devicelist;  devicelist = device;
	return device;
	}

void vrl_DeviceClose(vrl_Device *device)
	{
	vrl_Device *p;
	if (device->fn)
		{
		/* make sure all output channels are set to zero */
		if (device->outfunc)
			{
			int i;
			for (i = 0; i < device->noutput_channels; ++i)
				(*device->outfunc)(device, i, 0);
			}
		(*device->fn)(VRL_DEVICE_QUIT, device);
		}
	if (device == devicelist)
		devicelist = device->next;
	else
		for (p = devicelist; p; p = p->next)
			if (p->next == device)
				{
				p->next = device->next;
				break;
				}
	vrl_free(device);
	}

int vrl_DevicePoll(vrl_Device *device)
	{
	int i, changed;
	vrl_Time now, elapsed;
	if (device->fn == NULL) return 0;
	now = vrl_TimerRead();
	if (now < device->lastread)   /* timer wrapped around */
		{
		device->lastread = now;
		return 0;
		}
	elapsed = vrl_ScalarMultDiv(now - device->lastread, 1000, vrl_TimerGetTickRate());
	if (elapsed < device->period)    /* not time yet; never true if period == 0 */
		return 0;
	for (i = 0; i < device->nchannels; ++i)
		device->channels[i].changed = 0;  /* mark all channels as unchanged */
	(*device->fn)(VRL_DEVICE_POLL, device);
	device->lastread = now;
	changed = device->bchanged;
	for (i = 0; i < device->nchannels; ++i)
		{
		vrl_DeviceChannel *ch = &device->channels[i];
		vrl_32bit v = ch->rawvalue;
		if (!ch->changed)
			continue;
 		/* shift and deadzone */
		v -= ch->centerpoint;
		/* deadzone */
		if (ch->accumulate)
			{
			if (abs32(v) < ch->deadzone)
				v = 0;
			}
		else if (abs32(v - ch->oldvalue) < ch->deadzone)
			{
			ch->changed = 0;
			continue;
			}
		ch->oldvalue = v;
		/* scale and divide by range */
		if (ch->range)
			v = vrl_ScalarMultDiv(v, ch->scale, ch->range);
		/* if necessary, scale by elapsed time */
		if (ch->accumulate)
			ch->value = vrl_ScalarMultDiv(v, elapsed, 1000);
		else
			ch->value = v;
		changed = 1;
		}
	return changed;
	}

int vrl_DeviceReset(vrl_Device *device)
	{
	if (device->fn)
		return (*device->fn)(VRL_DEVICE_RESET, device);
	return 0;
	}

int vrl_DeviceSetRange(vrl_Device *device)
	{
	if (device->fn)
		return (*device->fn)(VRL_DEVICE_SET_RANGE, device);
	return 0;
	}

void vrl_DeviceOutput(vrl_Device *device, int channel, vrl_Scalar value)
	{
	if (device->outfunc)
		(*device->outfunc)(device, channel, value);
	}

int vrl_DevicePollAll(void)
	{
	vrl_Device *p;
	int retvalue = 0;
	for (p = devicelist; p; p = p->next)
		retvalue |= vrl_DevicePoll(p);
	return retvalue;
	}

void vrl_DeviceCloseAll(void)
	{
	vrl_Device *p;
	for (p = devicelist; p; p = p->next)
		vrl_DeviceClose(p);
	}

vrl_Device *vrl_DeviceGetFirst(void)
	{
	return devicelist;
	}

vrl_Device *vrl_DeviceFind(char *nickname)
	{
	vrl_Device *d;
	for (d = devicelist; d; d = d->next)
		if (!stricmp(d->nickname, nickname))
			return d;
	return NULL;
	}

