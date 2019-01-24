/* PCX file i/o routines for the AVRIL library */
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

typedef struct
	{
	unsigned char manu, hard, encod, bitpx;
	unsigned int x1, y1, x2, y2;
	unsigned int hres, vres;
	unsigned char palette[48];
	unsigned char vmode, nplanes;
	unsigned int bytesPerLine;
	char unused[128-68];
	} PCX_header;
	
static int getbyte(int *c, int *count, FILE *in)
	{
	if (feof(in)) return EOF;
	*c = getc(in) & 0xFF;
	if ((*c & 0xC0) == 0xC0)
		{
		*count = *c & 0x3F;
		if (feof(in)) return EOF;
		*c = getc(in) & 0xFF;
		}
	else
		*count = 1;
	return NULL;
	}

vrl_Boolean vrl_ReadPCX(FILE *in)
	{
	int c, count;
	unsigned char buff[1024];
	unsigned char *buffer = &buff[0];
	unsigned nread = 0;
	int scanline = 0, maxline = vrl_DisplayGetHeight();
	int width = vrl_DisplayGetWidth();
	fseek(in, 128L, SEEK_SET);  /* skip PCX header */
	while (getbyte(&c, &count, in) != EOF)
		while (count--)
			{
			*buffer++ = c;
			if (++nread == width)
				{
				vrl_RasterWriteScanline(vrl_DisplayGetRaster(), scanline++, buffer);
				if (scanline >= maxline)
					break;
				buffer = &buff[0];
				nread = 0;
				}
			}
	return 0;
	}

static PCX_header header =
		{ 10, 5, 1, 8, 0, 0, 319, 199, 75, 75, { 0 }, 0x13, 1, 320, 0 };

static void putbyte(int c, int count, FILE *out)
	{
	if (count == 0)
		return;
	if (count > 1 || (c & 0xC0) == 0xC0)
		putc(count | 0xC0, out);
	putc(c, out);
	}

static unsigned char buffer[1280];

vrl_Boolean vrl_WritePCX(FILE *out)
	{
	vrl_Palette pal;
	int nlines = vrl_DisplayGetHeight(), width = vrl_DisplayGetWidth();
	int scanline, i;
	unsigned int c, oldc, count;
	int old_drawpage = vrl_VideoGetDrawPage();
	vrl_VideoSetDrawPage(vrl_VideoGetViewPage());
	header.x2 = vrl_DisplayGetWidth()-1;
	header.y2 = vrl_DisplayGetHeight()-1;
	header.bytesPerLine = vrl_DisplayGetWidth();  /* byte-per-color */
	fwrite(&header, 128, 1, out);
	for (scanline = 0; scanline < nlines; ++scanline)
		{
		vrl_RasterReadScanline(vrl_DisplayGetRaster(), scanline, buffer);
		oldc = buffer[0];  count = 1;
		for (i = 1; i < width; ++i)
			{
			c = buffer[i];
			if (c != oldc)
				{
				putbyte(oldc, count, out);
				oldc = c;
				count = 1;
				}
			else if (++count >= 63)
				{
				putbyte(oldc, count, out);
				count = 0;
				}
			}
		putbyte(oldc, count, out);
		}
	putc(0x0C, out);
	vrl_VideoGetPalette(0, 255, &pal);
	for (i = 0; i < 256; ++i)
		{
		putc(pal.data[i][0] << 2, out);
		putc(pal.data[i][1] << 2, out);
		putc(pal.data[i][2] << 2, out);
		}
	vrl_VideoSetDrawPage(old_drawpage);
	return 0;
	}
