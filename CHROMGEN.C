/* Create a palette for use with Chromadepth */
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
#include <stdio.h>
#include <mem.h>     /* memset() */
#include <ctype.h>   /* toupper() */
#include <dos.h>     /* MK_FP(), int86() */
#include <bios.h>
#include <conio.h>

static unsigned char far *screen = (unsigned char far *) MK_FP(0xA000, 0);
struct { unsigned char red, green, blue; } palette[256];

static void set_entry(int ind, int red, int green, int blue)
	{
	outp(0x3C8, ind);
	outp(0x3C9, red);
	outp(0x3C9, green);
	outp(0x3C9, blue);
	printf("\t%d, %d, %d,\n", red, green, blue);
	}

static unsigned char video(unsigned char cmd, unsigned char parm)
	{
	union REGS regs;
	regs.h.ah = cmd;
	regs.h.al = parm;
	int86(0x10, &regs, &regs);
	return regs.h.al;
	}

static void create_palette(void)
	{
	float red = 63, green = 0, blue = 0, step = 64.0/60.0;
	int i, ind = 16;
	/* red through orange to yellow */
	for (i = 0; i < 60; ++i)
		{
		set_entry(ind++, red, green, blue);
		green += step;
		}
	/* yellow to green */
	for (i = 0; i < 60; ++i)
		{
		set_entry(ind++, red, green, blue);
		red -= step;
		}
	/* green to light blue */
	for (i = 0; i < 60; ++i)
		{
		set_entry(ind++, red, green, blue);
		blue += step;
		}
	/* light blue to indigo */
	for (i = 0; i < 60; ++i)
		{
		set_entry(ind++, red, green, blue);
		green -= step;
		}
	}

void main(void)
	{
	int oldmode = video(0x0F, 0x00) & 0xFF, i;
	video(0x00, 0x13);
	create_palette();
	for (i = 0; i < 240; ++i)
		screen[50*320+i] = screen[51*320+i] = screen[52*320+i] = 16 + i;
	bioskey(0);
	video(0x00, oldmode);
	}
