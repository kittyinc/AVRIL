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
#include "avrildrv.h"
#include <stdlib.h>  /* exit() */

void main(void)
	{
	vrl_Device *vio;
	vrl_SerialPort *port = vrl_SerialOpen(0x2F8, 3, 2000);
	if (port == NULL)
		{
		printf("Couldn't open serial port\n");
		exit(1);
		}
	vrl_SerialSetParameters(port, 9600, VRL_PARITY_NONE, 8, 1);
	vio = vrl_DeviceOpen(vrl_VIODevice, port);
	if (vio == NULL)
		{
		printf("Couldn't open VIO device\n");
		exit(2);
		}
	vrl_MathInit();
	while (!vrl_KeyboardCheck())
		{
		vrl_DevicePollAll();
		printf("pitch = %f, roll = %f, yaw = %f\n",
			angle2float(vrl_DeviceGetValue(vio, XROT)),
			angle2float(vrl_DeviceGetValue(vio, ZROT)),
			angle2float(vrl_DeviceGetValue(vio, YROT)));
		}
	vrl_KeyboardRead();
	vrl_DeviceClose(vio);
	vrl_SerialClose(port);	
	}
