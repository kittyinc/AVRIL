/* Video routines for AVRIL, using mode 0x13 */
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
#include <dos.h>     /* MK_FP(), int86() */
#include <string.h>  /* strncpy() */
#include "compat.h"

static unsigned char video(unsigned char cmd, unsigned char parm)
	{
	union REGS regs;
	regs.h.ah = cmd;
	regs.h.al = parm;
	int86(0x10, &regs, &regs);
	return regs.h.al;
	}

static vrl_Raster our_raster =
	{
	320, 200, 8,       /* width, height, depth */
	0, 0, 319, 199,    /* window */
	320,               /* rowbytes */
	NULL
	};

static int oldmode = -1;

extern void ntsc(int mode);

static void mouse(int n)
	{
	union REGS regs;
	regs.h.ah = 0;
	regs.h.al = n;
	int86(0x33, &regs, &regs);
	}

vrl_32bit vrl_VideoDriverMode13(vrl_VideoCommand cmd, vrl_32bit lparm, void *pparm)
	{
	union REGS regs;
	switch (cmd)
		{
		case VRL_VIDEO_GET_VERSION: return 1;
		case VRL_VIDEO_GET_DESCRIPTION: strncpy((char *) pparm, "Mode 0x13 driver", lparm); return 0;
		case VRL_VIDEO_SETUP:
			our_raster.data = VRL_COMPAT_SCREEN(0);
			/* save current mode */
			oldmode = video(0x0F, 0x00) & 0xFF;
			/* set mode 0x13 */
			video(0x00, 0x13);
			if (lparm & 0x100) ntsc(0x13);
			return 0;
		case VRL_VIDEO_SHUTDOWN:
			if (oldmode != -1)
				{
				video(0x00, oldmode);
				oldmode = -1;
				}
			return 0;
		case VRL_VIDEO_GET_MODE: return 0;  /* mode 13, submode 0 */
		case VRL_VIDEO_GET_NPAGES: return 1;
		case VRL_VIDEO_HAS_PALETTE: return 1;
		case VRL_VIDEO_SET_PALETTE:
			{
			unsigned char *rgbtable = pparm;
			vrl_16bit low = lparm >> 16, high = lparm, i;
			outp(0x3C8, (unsigned char) low);
			for (i = low; i <= high; ++i)
				{
				outp(0x3C9, (unsigned char) rgbtable[i*3+0]);
				outp(0x3C9, (unsigned char) rgbtable[i*3+1]);
				outp(0x3C9, (unsigned char) rgbtable[i*3+2]);
				}
			}
			break;
		case VRL_VIDEO_SET_NTSC: ntsc(0x13); break;
		case VRL_VIDEO_CHECK_RETRACE: return inp(0x3DA) & 0x08;
		case VRL_VIDEO_GET_RASTER: *((vrl_Raster **) pparm) = &our_raster; break;
		case VRL_VIDEO_BLIT:
			if ((vrl_Raster *) pparm != &our_raster)
				memcpy(our_raster.data, vrl_RasterGetData((vrl_Raster *) pparm), 64000u);
			break;
		case VRL_VIDEO_CURSOR_HIDE: mouse(2); break;
		case VRL_VIDEO_CURSOR_SHOW: mouse(1); break;
		case VRL_VIDEO_CURSOR_RESET: mouse(0); break;
		default: break;
		}
	return 0;
	}
