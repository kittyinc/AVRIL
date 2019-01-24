/* THIS FILE IS #INCLUDED BY SCAN.C */

/* Horizontal fill and pixel set routines for Mode 13 */
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

#define setup_color_table()

#define set_color(n)

#define setpixel(x, y, color) screen_buffer[line_table[(y)+top_border]+(x)+left_border] = (color)

/* do a simple horizontal fill from x1 to x2 on line y using color */

#define hfill(x1, x2, y, color) \
	if (x1 <= x2) \
		memset(&screen_buffer[line_table[y] + (x1)], (color), (x2) - (x1) + 1)

static void glass_fill(vrl_ScreenPos y, int off, unsigned char color)
	{
	unsigned char VRL_HUGEPTR *ptr = &screen_buffer[line_table[y]];
	vrl_ScreenPos pixel;
	int x = spans[y].startx;
	for (pixel = x + ((x ^ y) & 0x01); pixel <= spans[y].endx; pixel += 2)
		ptr[pixel] = color | off;
	}

#define clear_display(color) memset(screen_buffer, (color), 64000u)

static unsigned char masks[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

static int render_char(vrl_ScreenPos x, vrl_ScreenPos y, unsigned char c, unsigned char color)
	{
	unsigned int dst = line_table[y+top_border] + x + left_border;
	unsigned char *src = vrl_scan_chartable[c];
	int i;
	for (i = 0; i < 8; ++i)
		{
		int j;
		for (j = 0; j < 8; ++j)
			if (src[i] & masks[j])
				screen_buffer[dst+j] = color;
		dst += vrl_RasterGetRowbytes(our_raster);
		}
	return 8;
	}
