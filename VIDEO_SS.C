/* Video routines for AVRIL, using 7th Sense video card */
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
#include "compat.h"
#include <dos.h>
#include <string.h>

static vrl_unsigned16bit vrbase = 0x260;  /* default value */

static unsigned char code[] = {
	0x00, 0x08, 0xF0, 0x02, 0x20, 0x00, 0x55, 0x02,
	0x10, 0x04, 0x00, 0x00, 0x0C, 0x03, 0x00, 0xFF
	};

static unsigned char rgbMap[64] = {
	0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48,51,54,57,50,63,66,69,72,
	75,78,81,84,87,90,93,96,99,102,105,108,111,114,117,120,123,126,129,132,
	135,138,141,144,147,150,153,156,159,162,165,168,171,174,177,180,183,186,189
	};

static vrl_Raster our_raster =
	{
	320, 200, 8,       /* width, height, depth */
	0, 0, 319, 199,    /* window */
	320,               /* rowbytes */
	NULL
	};

vrl_32bit vrl_VideoDriver7thSense(vrl_VideoCommand cmd, vrl_32bit lparm, void *pparm)
	{
	union REGS regs;
	switch (cmd)
		{
		case VRL_VIDEO_GET_VERSION: return 1;
		case VRL_VIDEO_GET_DESCRIPTION: strncpy((char *) pparm, "7th Sense video driver", lparm); return 0;
		case VRL_VIDEO_SETUP:
			{
			int i;
			if (lparm)
				vrbase = lparm;
			if (inp(vrbase) != 0xF8)
				return -1;
			outp(vrbase, 8);       /* access address registers */
			outp(vrbase+6, 0);     /* select register 0 */
			outp(vrbase, 0x0E);    /* access data registers */
			for (i = 0; i < 16; ++i)
				outp(vrbase+6, code[i]);
			outp(vrbase, 0x0A);    /* access pixel mask */
			outp(vrbase+6, 0xFFF);  /* set pixel mask to 0xFF */
			printf("Output being sent to 7th Sense interface\n");
			}
			return 0;
		case VRL_VIDEO_GET_NPAGES: return 1;
		case VRL_VIDEO_HAS_PALETTE: return 1;
		case VRL_VIDEO_SET_PALETTE:
			{
			unsigned char *rgbtable = pparm;
			vrl_16bit low = lparm >> 16, high = lparm, i;
			outp(vrbase, 8);      /* access address registers */
			outp(vrbase+6, low);  /* starting index */
			outp(vrbase, 9);      /* access palette */
			for (i = low; i <= high; ++i)
				{
				outp(vrbase+6, rgbMap[rgbtable[i*3+0]]);
				outp(vrbase+6, rgbMap[rgbtable[i*3+1]]);
				outp(vrbase+6, rgbMap[rgbtable[i*3+2]]);
				}
			}
			break;
		case VRL_VIDEO_GET_RASTER: *((vrl_Raster **) pparm) = &our_raster; break;
		case VRL_VIDEO_BLIT:
			{
			vrl_unsigned16bit VRL_FARPTR *p = (vrl_unsigned16bit VRL_FARPTR *) vrl_RasterGetData((vrl_Raster *) pparm);
			int i;
			outpw(vrbase+2, 0);
			for (i = 0; i < 32000; ++i)
				outpw(vrbase+4, *p++);
			}
			break;
		default: break;
		}
	return 0;
	}
