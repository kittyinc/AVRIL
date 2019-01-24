/* NTSC support for CyberMaxx in AVRIL */

/* Based on code provided by VictorMaxx Technologies */
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

#include <dos.h>
#include "cyberdrv.h"

static void setSR(unsigned char reg, unsigned char value)
	{
	outp(0x3C4, (unsigned char) reg);
	outp(0x3C5, (unsigned char) value);
	}

static void setCRTC(unsigned char reg, unsigned char value)
	{
	outp(0x3D4, (unsigned char) reg);
	outp(0x3D5, (unsigned char) value);
	}

void ntsc(int mode)
	{
	VGAatr registers;
	int i;
	switch (mode)
		{
		case 0x03: registers = VGA3; break;
		case 0x0e: registers = VGAe; break;
		case 0x10: registers = VGA10; break;
		case 0x12: registers = VGA12; break;
		case 0x13: registers = VGA13; break;
		default: return;
		}
	outp(0x3C2, (unsigned char) registers.MOR);
	setSR(0, 0);  /* reset */
	for (i = 1; i <= 4; ++i)
		setSR(i, registers.SR[i]);
	setSR(0, 3);
	setCRTC(0x11, 0);     /* enable write access to regs 0 to 7 of CRTC */
	setCRTC(0, registers.CRCT[0]);
	for (i = 3; i <= 7; ++i)
		setCRTC(i, registers.CRCT[i]);
	setCRTC(0x11, 0x80);  /* disable write access to regs 0 to 7 of CRTC */
	for (i = 9; i < 23; ++i)
		setCRTC(i, registers.CRCT[i]);
	}
