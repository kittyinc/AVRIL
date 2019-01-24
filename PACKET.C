/* Device packet support for PC version of AVRIL */
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

vrl_DevicePacketBuffer *vrl_DeviceCreatePacketBuffer(int buffsize)
	{
	vrl_DevicePacketBuffer *p;
	p = vrl_malloc(sizeof(vrl_DevicePacketBuffer));
	if (p == NULL) return NULL;
	p->buffer = vrl_malloc(buffsize);
	if (p->buffer == NULL)
		{
		vrl_free(p);
		return NULL;
		}
	p->ind = 0;  p->buffsize = buffsize;
	return p;
	}

void vrl_DeviceDestroyPacketBuffer(vrl_DevicePacketBuffer *buff)
	{
	if (buff->buffer)
		vrl_free(buff->buffer);
	vrl_free(buff);
	}

vrl_Boolean vrl_DeviceGetPacket(vrl_SerialPort *port, vrl_DevicePacketBuffer *buff)
	{
	while (vrl_SerialCheck(port))
		{
		int c = vrl_SerialGetc(port);
		if (c & 0x80)  /* framing bit set */
			buff->ind = 0;
		if (buff->ind < buff->buffsize)
			buff->buffer[buff->ind++] = c;
		if (buff->ind == buff->buffsize)
			{
			++buff->ind;  /* make sure we only return it once */
			return 1;     /* complete packet received */
			}
		}
	return 0;  /* no data waiting, packet not yet complete */
	}
