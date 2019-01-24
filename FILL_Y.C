/* THIS FILE IS #INCLUDED BY SCAN.C */

/* Horizontal fill and pixel set routines for Mode Y */
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
#include <dos.h>     /* MK_FP() */

#define DIRECT 1   /* mode Y implies direct */

extern unsigned int vrl_modey_drawbase;

static unsigned char VRL_FARPTR *color_table;

static void setup_color_table(void)
	{
	color_table = VRL_COMPAT_SCREEN(16000);
	}

static unsigned char set_color(int n)
	{
	return color_table[n];
	}

static void setpixel(vrl_ScreenPos x, vrl_ScreenPos y, vrl_16bit color)
	{
	outp(0x3C4, 0x02);
	outp(0x3C5, (unsigned char) (1 << (x & 0x03)));
	x += left_border;
	screen_buffer[line_table[y+top_border]+(x>>2)+vrl_modey_drawbase] = color;
	}

static unsigned char startmask[] = { 0x0F, 0x0E, 0x0C, 0x08 };
static unsigned char endmask[] = { 0x01, 0x03, 0x07, 0x0F };
static unsigned char midmask[] =
	{
	0x01, 0x03, 0x07, 0x0F, 0x00, 0x02, 0x06, 0x0E,
	0x00, 0x00, 0x04, 0x0C, 0x00, 0x00, 0x00, 0x08
	};

static void hfill(vrl_ScreenPos x1, vrl_ScreenPos x2, vrl_ScreenPos y, vrl_Color color)
	{
	unsigned char VRL_HUGEPTR *buff = &screen_buffer[line_table[y]+vrl_modey_drawbase];
	if (x1 > x2) return;
	outp(0x3C4, (unsigned char) 0x02);
	if (((x1 ^ x2) & ~0x03) == 0)  /* same byte */
		{
		unsigned int v = ((x1 << 2) | (x2 & 0x03)) & 0x0F;
		outp(0x3C5, midmask[v]);
		buff[x1 >> 2] = color;
		return;
		}
	if (x1 & 0x03)
		{
		outp(0x3C5, startmask[x1 & 0x03]);
		buff[x1 >> 2] = color;
		x1 += 4;
		}
	if ((x2 & 0x03) != 0x03)
		{
		outp(0x3C5, endmask[x2 & 0x03]);
		buff[x2 >> 2] = color;
		x2 -= 4;
		}
	x1 >>= 2;
	x2 >>= 2;
	outp(0x3C5, 0x0F);
	if (x1 <= x2)
		memset(&buff[x1], color, x2 - x1 + 1);
	}

static void glass_fill(vrl_ScreenPos y, int off, unsigned char color)
	{
	unsigned char VRL_HUGEPTR *buff = &screen_buffer[line_table[y]+vrl_modey_drawbase];
	vrl_ScreenPos x1 = spans[y].startx, x2 = spans[y].endx;
	vrl_16bit glassmask = (y & 0x01) ? 0xAA : 0x55;
	set_color(color | off);
	if (x1 > x2) return;
	outp(0x3C4, 0x02);
	if (((x1 ^ x2) & ~0x03) == 0)  /* same byte */
		{
		unsigned int v = ((x1 << 2) | (x2 & 0x03)) & 0x0F;
		outp(0x3C5, (unsigned char) (midmask[v] & glassmask));
		buff[x1 >> 2] = color | off;
		return;
		}
	if (x1 & 0x03)
		{
		outp(0x3C5, (unsigned char) (startmask[x1 & 0x03] & glassmask));
		buff[x1 >> 2] = color | off;
		x1 += 4;
		}
	if ((x2 & 0x03) != 0x03)
		{
		outp(0x3C5, (unsigned char) (endmask[x2 & 0x03] & glassmask));
		buff[x2 >> 2] = color | off;
		x2 -= 4;
		}
	x1 >>= 2;  x2 >>= 2;
	outp(0x3C5, (unsigned char) (0x0F & glassmask));
	if (x1 <= x2)
		memset(&screen_buffer[line_table[y] + x1 + vrl_modey_drawbase], color | off, x2 - x1 + 1);
	}

static void clear_display(vrl_Color color)
	{
	set_color(color);
	outpw(0x3C4, 0x0F02);  /* enable all planes */
	memset(&screen_buffer[vrl_modey_drawbase], 0, 16000);
	}

static unsigned char rev_tab[] =
	{
	0x00, 0x08, 0x04, 0x0C,
	0x02, 0x0A, 0x06, 0x0E,
	0x01, 0x09, 0x05, 0x0D,
	0x03, 0x0B, 0x07, 0x0F
	};

static int render_char(vrl_ScreenPos x, vrl_ScreenPos y, unsigned char c, unsigned char color)
	{
	unsigned int dst = line_table[y+top_border] + ((x+left_border) >> 2) + vrl_modey_drawbase;
	int shift = 4 - ((x+left_border) & 0x03);
	unsigned char *src = vrl_scan_chartable[c];
	int i;
	set_color(color);
	outp(0x3C4, 0x02);
	for (i = 0; i < 8; ++i)
		{
		unsigned int w = ((unsigned int) src[i]) << shift;
		outp(0x3C5, (unsigned char) (rev_tab[(w >> 8) & 0x0F]));
		screen_buffer[dst] = color;
		outp(0x3C5, (unsigned char) (rev_tab[(w >> 4) & 0x0F]));
		screen_buffer[dst+1] = color;
		outp(0x3C5, (unsigned char) (rev_tab[w & 0x0F]));
		screen_buffer[dst+2] = color;
		dst += vrl_RasterGetRowbytes(our_raster);
		}
	return 8;
	}
