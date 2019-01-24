/* Video routines for AVRIL, using mode Y (320x200x8, 4 pages) */
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
	80,                /* rowbytes */
	NULL
	};

static int oldmode = -1;

extern void ntsc(int mode);

/* Hooks into UNCHAIN so we can use Turbo Debugger */

#ifdef BORLANDC
static void unchain_setup(void)
	{
	union REGS regs;
	int i;
	regs.x.ax = 0xCB00;
	int86(0x10, &regs, &regs);
	for (i = 0; i < 4; ++i)
		{
		regs.x.ax = 0xCB00 | (i+1);
		regs.x.bx = 16384 * i;
		int86(0x10, &regs, &regs);
		}
	}

static void unchain_shutdown(void)
	{
	union REGS regs;
	regs.x.ax = 0xCB06;
	int86(0x10, &regs, &regs);
	}

static void unchain_init_palette(void)
	{
	union REGS regs;
	regs.x.ax = 0xCB05;
	int86(0x10, &regs, &regs);
	}
#else
#define unchain_setup()
#define unchain_shutdown()
#define unchain_init_palette()
#endif

unsigned int vrl_modey_drawbase = 0;
static unsigned int viewbase = 0;

static int old_mouse_x, mouse_center_x, old_mouse_y, mouse_center_y;
static int cursorflag = -1;

static unsigned char cursor_image[8] =
	{ 0x10, 0x10, 0x10, 0xFE, 0x10, 0x10, 0x10, 0x00 };
static unsigned char cursor_color = 255;

static unsigned char VRL_FARPTR *cursor_save_area;

static unsigned int cursor_dst;

static unsigned char rev_tab[] =
	{
	0x00, 0x08, 0x04, 0x0C,
	0x02, 0x0A, 0x06, 0x0E,
	0x01, 0x09, 0x05, 0x0D,
	0x03, 0x0B, 0x07, 0x0F
	};

static void draw_cursor(int x, int y)
	{
	unsigned char VRL_FARPTR *screen_buffer = our_raster.data;
	unsigned char VRL_FARPTR *save = cursor_save_area;
	unsigned char *src = cursor_image;
	unsigned int dst;
	int shift, i;
	x -= 4;  y -= 4;  /* shift center; x and y are now top-left corner */
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	shift = 4 - (x & 0x03);
	outpw(0x3CE, 0x0008);  /* source is latches */
	outpw(0x3C4, 0x0F02);  /* enable all planes */
	dst = cursor_dst = y * our_raster.rowbytes + (x >> 2) + viewbase;
	for (i = 0; i < 8; ++i)
		{
		*save++ = screen_buffer[dst];
		*save++ = screen_buffer[dst+1];
		*save++ = screen_buffer[dst+2];
		dst += our_raster.rowbytes;
		}
	outpw(0x3CE, 0xFF08);  /* source is CPU */
	dst = cursor_dst;
	outp(0x3C4, (unsigned char) 0x02);
	for (i = 0; i < 8; ++i)
		{
		unsigned int w = ((unsigned int) src[i]) << shift;
		outp(0x3C5, (unsigned char) (rev_tab[(w >> 8) & 0x0F]));
		screen_buffer[dst] = cursor_color;
		outp(0x3C5, (unsigned char) (rev_tab[(w >> 4) & 0x0F]));
		screen_buffer[dst+1] = cursor_color;
		outp(0x3C5, (unsigned char) (rev_tab[w & 0x0F]));
		screen_buffer[dst+2] = cursor_color;
		dst += our_raster.rowbytes;
		}
	outpw(0x3CE, 0x0008);  /* source is latches */
	}

static void erase_cursor(void)
	{
	unsigned char VRL_FARPTR *screen_buffer = our_raster.data;
	unsigned char VRL_FARPTR *save = cursor_save_area;
	unsigned int dst = cursor_dst;
	int i;
	outpw(0x3CE, 0x0008);  /* source is latches */
	outpw(0x3C4, 0x0F02);  /* enable all planes */
	for (i = 0; i < 8; ++i)
		{
		screen_buffer[dst] = *save++;
		screen_buffer[dst+1] = *save++;
		screen_buffer[dst+2] = *save++;
		dst += our_raster.rowbytes;
		}
	}

vrl_32bit vrl_VideoDriverModeY(vrl_VideoCommand cmd, vrl_32bit lparm, void *pparm)
	{
	switch (cmd)
		{
		case VRL_VIDEO_GET_VERSION: return 1;
		case VRL_VIDEO_GET_DESCRIPTION: strncpy((char *) pparm, "Mode Y driver", lparm); return 0;
		case VRL_VIDEO_SETUP:
			{
			unsigned char VRL_FARPTR *color_table = VRL_COMPAT_SCREEN(16000);
			int i;
			our_raster.data = VRL_COMPAT_SCREEN(0);
			cursor_save_area = VRL_COMPAT_SCREEN(32384);
			/* save current mode */
			oldmode = video(0x0F, 0x00) & 0xFF;
			/* set mode 0x13 */
			video(0x00, 0x13);
			outpw(0x3C4, 0x0604);  /* unchain */
			outpw(0x3D4, 0x0014);  /* doubleword off */
			outpw(0x3D4, 0xE317);  /* byte mode on */
			outpw(0x3CE, 0xFF08);  /* source is CPU */
			outpw(0x3C4, 0x0F02);  /* enable all planes */
			memset(VRL_COMPAT_SCREEN(0), 0, 65535u);  /* clear the display */
			for (i = 0; i < 256; ++i)
				color_table[i] = i;
			outpw(0x3CE, 0x0008);  /* source is latches */
			outpw(0x3D4, 0x000D);  /* low part of page address always 0x00 */
			outpw(0x3D4, 0x000C);  /* high part of page address initially 0x00 */
			unchain_setup();
			if (lparm & 0x100) ntsc(0x13);
			}
			return 0;
		case VRL_VIDEO_SHUTDOWN:
			if (oldmode != -1)
				{
				video(0x00, oldmode);
				oldmode = -1;
				}
			unchain_shutdown();
			return 0;
		case VRL_VIDEO_GET_MODE: return 0;  /* mode 13, submode 0 */
		case VRL_VIDEO_GET_NPAGES: return 4;
		case VRL_VIDEO_SET_DRAW_PAGE: vrl_modey_drawbase = lparm << 14; break;
		case VRL_VIDEO_SET_VIEW_PAGE:
			viewbase = lparm * 16384;
			outp(0x3D4, (unsigned char) 0x0D);
			outp(0x3D5, (unsigned char) viewbase);
			outp(0x3D4, (unsigned char) 0x0C);
			outp(0x3D5, (unsigned char) (viewbase >> 8));
			break;
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
			unchain_init_palette();
			}
			break;
		case VRL_VIDEO_SET_NTSC: ntsc(0x13); break;
		case VRL_VIDEO_CHECK_RETRACE: return inp(0x3DA) & 0x08;
		case VRL_VIDEO_GET_RASTER: *((vrl_Raster **) pparm) = &our_raster; break;
		case VRL_VIDEO_BLIT:
			if ((vrl_Raster *) pparm != &our_raster)
				{
				unsigned char VRL_FARPTR *screen = VRL_COMPAT_SCREEN(vrl_modey_drawbase);
				unsigned char VRL_FARPTR *buff = vrl_RasterGetData((vrl_Raster *) pparm);
				unsigned int pass;
				outpw(0x3CE, 0xFF08);  /* source is CPU */
				for (pass = 0; pass < 4; ++pass)
					{
					unsigned char VRL_FARPTR *b = &buff[pass];
					unsigned int src, dst = 0;
					outpw(0x3C4, (1 << (pass+8)) | 0x02);
					for (src = 0; src < 64000u; src += 16)
						{
						screen[dst++] = b[src];
						screen[dst++] = b[src+4];
						screen[dst++] = b[src+8];
						screen[dst++] = b[src+12];
						}
					}
				}
			break;
		case VRL_VIDEO_CURSOR_HIDE:
			if (cursorflag >= 0)
				erase_cursor();
			--cursorflag;
			break;
		case VRL_VIDEO_CURSOR_SHOW:
			if (++cursorflag == 0)
				draw_cursor(old_mouse_x, old_mouse_y);
			break;
		case VRL_VIDEO_CURSOR_RESET:
			old_mouse_x = 160;  old_mouse_y = 100;
			cursorflag = -1;
			break;
		case VRL_VIDEO_CURSOR_MOVE:
			old_mouse_x = lparm >> 16;
			old_mouse_y = lparm;
			if (cursorflag >= 0)
				{
				erase_cursor();
				draw_cursor(old_mouse_x, old_mouse_y);
				}
			break;
		case VRL_VIDEO_CURSOR_SET_APPEARANCE:
			memcpy(cursor_image, pparm, 8);
			cursor_color = lparm;
			break;
		default: break;
		}
	return 0;
	}
