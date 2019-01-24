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

#define gc() (bioskey(0) & 0x7F)

static unsigned char far *screen = MK_FP(0xA000, 0);

/* first index is R, G, or B */
/* second index is height, position, left, right */

int RED_HEIGHT = 63;
int RED_POSITION = 239;
int GREEN_HEIGHT = 63;
int GREEN_POSITION = 120;
int GREEN_LEFT = 0;
int GREEN_RIGHT = 239;
int BLUE_HEIGHT = 63;
int BLUE_POSITION = 0;

struct { unsigned char red, green, blue; } palette[256];

static void set_palette(void)
	{
	int i;
	outp(0x3C8, 16);
	for (i = 16; i < 256; ++i)
		{
		outp(0x3C9, (unsigned char) palette[i].red);
		outp(0x3C9, (unsigned char) palette[i].green);
		outp(0x3C9, (unsigned char) palette[i].blue);
		}
	}

static unsigned char video(unsigned char cmd, unsigned char parm)
	{
	union REGS regs;
	regs.h.ah = cmd;
	regs.h.al = parm;
	int86(0x10, &regs, &regs);
	return regs.h.al;
	}

static void output(char *s)
	{
	union REGS regs;
	while (*s)
		{
		regs.h.ah = 0x0E;
		regs.h.al = *s++;
		regs.h.bh = 0;
		regs.h.bl = 15;
		int86(0x10, &regs, &regs);
		}
	}

static void poscursor(int row, int col)
	{
	union REGS regs;
	regs.h.ah = 0x02;
	regs.h.bh = 0;
	regs.h.dh = row;
	regs.h.dl = col;
	int86(0x10, &regs, &regs);
	}

static void redraw(void)
	{
	char buff[100];
	int i;
	for (i = 0; i < 240; ++i)
		{
		/* RED */
		if (i <= RED_POSITION)
			palette[i+16].red = RED_HEIGHT - (i * (float) RED_HEIGHT) / RED_POSITION;
		else
			palette[i+16].red = 0;
		/* GREEN */
		if (i < GREEN_LEFT || i > GREEN_RIGHT)
			palette[i+16].green = 0;
		else if (i <= GREEN_POSITION)
			palette[i+16].green = (i - GREEN_LEFT) * ((float) GREEN_HEIGHT) / (GREEN_POSITION - GREEN_LEFT);
		else
			palette[i+16].green = GREEN_HEIGHT - (i * (float) GREEN_HEIGHT) / (GREEN_RIGHT - GREEN_POSITION);
		/* BLUE */
		if (i >= BLUE_POSITION)
			palette[i+16].blue = (i - BLUE_POSITION) * ((float) BLUE_HEIGHT) / (240 - BLUE_POSITION);
		else
			palette[i+16].blue = 0;
		}
	set_palette();
	memset(screen, 0, 64000u);
	for (i = 0; i < 240; ++i)
		screen[50*320+i] = screen[51*320+i] = screen[52*320+i] = 16 + i;
	poscursor(15, 0);
	sprintf(buff, "A) Red height = %d\r\n", RED_HEIGHT); output(buff);
	sprintf(buff, "B) Green height = %d\r\n", GREEN_HEIGHT); output(buff);
	sprintf(buff, "C) Blue height = %d\r\n", BLUE_HEIGHT); output(buff);
	sprintf(buff, "D) Red position = %d\r\n", RED_POSITION); output(buff);
	sprintf(buff, "E) Green left = %d\r\n", GREEN_LEFT); output(buff);
	sprintf(buff, "F) Green right = %d\r\n", GREEN_RIGHT); output(buff);
	sprintf(buff, "G) Green position = %d\r\n", GREEN_POSITION); output(buff);
	sprintf(buff, "H) Blue position = %d\r\n", BLUE_POSITION); output(buff);
	}

static void increment_height(int *value)
	{
	int t = (*value)+1;
	if (t < 64) *value = t;
	}

static void decrement_height(int *value)
	{
	int t = (*value)-1;
	if (t >= 0) *value = t;
	}

static void increment_position(int *value)
	{
	int t = (*value)+1;
	if (t < 240) *value = t;
	}

static void decrement_position(int *value)
	{
	int t = (*value)-1;
	if (t >= 0) *value = t;
	}

void main(void)
	{
	int c;
	int oldmode = video(0x0F, 0x00) & 0xFF;
	video(0x00, 0x13);
	redraw();
	while ((c = gc()) != 0x1B)
		{
		switch (c)
			{
			case 'a': increment_height(&RED_HEIGHT); break;
			case 'A': decrement_height(&RED_HEIGHT); break;
			case 'b': increment_height(&GREEN_HEIGHT); break;
			case 'B': decrement_height(&GREEN_HEIGHT); break;
			case 'c': increment_height(&BLUE_HEIGHT); break;
			case 'C': decrement_height(&BLUE_HEIGHT); break;
			case 'd': increment_position(&RED_POSITION); break;
			case 'D': decrement_position(&RED_POSITION); break;
			case 'e': increment_position(&GREEN_LEFT); break;
			case 'E': decrement_position(&GREEN_LEFT); break;
			case 'f': increment_position(&GREEN_RIGHT); break;
			case 'F': decrement_position(&GREEN_RIGHT); break;
			case 'g': increment_position(&GREEN_POSITION); break;
			case 'G': decrement_position(&GREEN_POSITION); break;
			case 'h': increment_position(&BLUE_POSITION); break;
			case 'H': decrement_position(&BLUE_POSITION); break;
			}
		redraw();
		}
	video(0x00, oldmode);
	}
