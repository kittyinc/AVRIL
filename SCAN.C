/* #define DIRECT 1
 shouldn't re-compute buff in hfill each time
*/

/* Scan-conversion routines for AVRIL */
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
#include <stdlib.h>  /* abs() */
#include <string.h>  /* strncpy(), strlen() */
#include "compat.h"

#define SCREEN_UNITY (1L << VRL_SCREEN_FRACT_BITS)
#define SCREEN_HALF_UNITY (1L << (VRL_SCREEN_FRACT_BITS-1))
#define SCREEN_HALF_MASK (SCREEN_UNITY - 1)

#define s2i(a) ((a) >> VRL_SCREEN_FRACT_BITS)
//#define FITTING 1
#ifdef FITTING
#define round_down(a) (s2i(a) - (((a) & SCREEN_HALF_MASK) ? 0 : 1))
#define round_up(a) s2i((a) + SCREEN_UNITY - 1)
#else
#define round_up(a) (s2i((a)+SCREEN_HALF_UNITY))
#define round_down(a) s2i(a)
#endif

#define MAXPOINTS 20

static vrl_Raster *our_raster = NULL;

static unsigned int *line_table = NULL;
static vrl_Factor *slope_table = NULL;

#define screen_buffer ((unsigned char VRL_HUGEPTR *) vrl_RasterGetData(our_raster))

extern unsigned char vrl_scan_chartable[][8];

static vrl_ScreenPos top_border, left_border;

static vrl_OutputVertex *topvertex, *botvertex;

static vrl_ScreenPos starty;  /* starting Y coordinate for the poly being processed */
static vrl_ScreenPos endy;    /* ending Y coordinate for the poly being processed */

typedef struct
	{
	vrl_ScreenPos startx, endx;
	vrl_16bit startcolor, endcolor;
	} Span;

static Span spans[500];

#ifdef MODE_Y
#include "fill_y.c"
#else
#include "fill_13.c"
#endif

/* Specular support */

#define NUMSPECS 10
static unsigned char specular_table[NUMSPECS][256];
static unsigned char *spectable = NULL;

#include <math.h>  /* pow() */

static void specular_update(vrl_Palette *palette)
	{
	unsigned int hue, shade, specnum;
	vrl_Hue *huemap = vrl_PaletteGetHuemap(palette);
	for (specnum = 0; specnum < NUMSPECS; ++specnum)
		{
		double exponent = 1.0 + 7 * specnum / 100.0;
		for (shade = 0; shade <= huemap[0].maxshade; ++shade)
			specular_table[specnum][shade] = shade;
		for (hue = 1; hue < 256 && huemap[hue].maxshade; ++hue)
			{
			for (shade = 0; shade <= huemap[hue].maxshade; ++shade)
				{
				double value = pow(shade, exponent);
				if (value > huemap[hue].maxshade)
					value = huemap[hue].maxshade;
				specular_table[specnum][huemap[hue].start + shade] =
					huemap[hue].start + value;
				}
			}
		}
	}

/* End of specular support */

/* set the top and bottom vertices */

static void find_top(vrl_OutputVertex *list)
	{
	vrl_OutputVertex *v = topvertex = botvertex = list;
	do
		{
		if (v->y < topvertex->y)
			topvertex = v;
		else if (v->y > botvertex->y)
			botvertex = v;
		v = v->next;
		} while (v != list);
	/* find starting and ending scanline numbers */
	starty = round_up(topvertex->y) + top_border;
	endy = round_down(botvertex->y) + top_border;
	}

/* Flat-shading versions of the outline scanning routines */

/* scan an edge from x1,y1 to x2,y2; two versions (left edge and right edge) */

static int scan_left_edge(vrl_OutputVertex *first, vrl_OutputVertex *last)
	{
	vrl_ScreenCoord xincrement, x = first->x + (((vrl_ScreenCoord) left_border) << VRL_SCREEN_FRACT_BITS);
	vrl_ScreenPos y1 = round_up(first->y) + top_border, y2 = round_down(last->y) + top_border, y;
	if (y2 < y1) y2 = y1;
	xincrement = vrl_FactorMultiply(slope_table[y2 - y1], last->x - first->x);
	for (y = y1; y <= y2; ++y)
		{
		spans[y].startx = round_up(x);
		x += xincrement;
		}		
	return 0;
	}

static int scan_right_edge(vrl_OutputVertex *first, vrl_OutputVertex *last)
	{
	vrl_ScreenCoord xincrement, x = first->x + (((vrl_ScreenCoord) left_border) << VRL_SCREEN_FRACT_BITS);
	vrl_ScreenPos y1 = round_up(first->y) + top_border, y2 = round_down(last->y) + top_border, y;
	if (y2 <= y1) y2 = y1;
	xincrement = vrl_FactorMultiply(slope_table[y2 - y1], last->x - first->x);
	for (y = y1; y <= y2; ++y)
		{
		spans[y].endx = round_down(x);
		x += xincrement;
		}		
	return 0;
	}

static int scan_outline(vrl_OutputVertex *list)
	{
	vrl_OutputVertex *v;
	find_top(list);
	if (starty >= endy) return 1;
	/* scan convert left-side edges */
	for (v = topvertex; v != botvertex; v = v->prev)
		if (scan_left_edge(v, v->prev))
			return 1;
	/* scan convert right-side edges */
	for (v = topvertex; v != botvertex; v = v->next)
		if (scan_right_edge(v, v->next))
			return 1;
	return 0;
	}

#ifndef MODE_Y
/* Gouraud-shading versions of the outline scanning routines */

#define DF(n) ((15-(n)) << 4)

static vrl_16bit dither[4][4] =
	{
	DF(0),  DF(8),  DF(2),  DF(10),
	DF(12), DF(4),  DF(14), DF(6),
	DF(3),  DF(11), DF(1),  DF(9),
	DF(15), DF(7),  DF(13), DF(5)
	};
	
static void render_gouraud_poly(void)
	{
	vrl_ScreenPos x, y;
	vrl_16bit color, cincrement;
	y = (endy + starty) >> 1;
	cincrement =
		vrl_FactorMultiply(slope_table[spans[y].endx - spans[y].startx],
			spans[y].endcolor - spans[y].startcolor);
	for (y = starty; y <= endy; ++y)
		{
		unsigned char VRL_HUGEPTR *ptr = &screen_buffer[line_table[y]];
		color = spans[y].startcolor;
if (bioskey(2) & 0x02)
{ptr[spans[y].startx] = spans[y].startcolor >> 8;
ptr[spans[y].endx] = spans[y].endcolor >> 8;
}else{
		for (x = spans[y].startx; x <= spans[y].endx; ++x)
			{
			ptr[x] = color >> 8;
			color += cincrement;
			}
}
		}
	}

static void render_dithered_poly(void)
	{
	vrl_ScreenPos x, y;
	vrl_16bit color, cincrement;
	y = (endy + starty) >> 1;
	cincrement =
		vrl_FactorMultiply(slope_table[spans[y].endx - spans[y].startx],
			spans[y].endcolor - spans[y].startcolor);
	for (y = starty; y <= endy; ++y)
		{
		unsigned char VRL_HUGEPTR *ptr = &screen_buffer[line_table[y]];
		vrl_16bit *dtable = dither[y & 3];
		color = spans[y].startcolor;
		for (x = spans[y].startx; x <= spans[y].endx; ++x)
			{
			ptr[x] = ((unsigned int) ((color + dtable[x&3]))) >> 8;
			color += cincrement;
			}
		}
	}

static void render_fast_dithered_poly(void)
	{
	vrl_ScreenPos x, y;
	vrl_16bit color, cincrement;
	y = (endy + starty) >> 1;
	cincrement =
		vrl_FactorMultiply(slope_table[spans[y].endx - spans[y].startx],
			spans[y].endcolor - spans[y].startcolor);
	for (y = starty; y <= endy; ++y)
		{
		unsigned char VRL_HUGEPTR *ptr = &screen_buffer[line_table[y]];
		vrl_16bit a, b;
		if (y & 1) { a = 1 << 6;  b = 3 << 6; }
		else { a = 2 << 6;  b = 0 << 6; }
		color = spans[y].startcolor;
		for (x = spans[y].startx; x <= spans[y].endx; ++x)
			{
			ptr[x] = ((unsigned int) ((color + ((x&1) ? a : b)))) >> 8;
			color += cincrement;
			}
		}
	}

static void render_specular_poly(void)
	{
	vrl_ScreenPos x, y;
	vrl_16bit color, cincrement;
	y = (endy + starty) >> 1;
	cincrement =
		vrl_FactorMultiply(slope_table[spans[y].endx - spans[y].startx],
			spans[y].endcolor - spans[y].startcolor);
	for (y = starty; y <= endy; ++y)
		{
		unsigned char VRL_HUGEPTR *ptr = &screen_buffer[line_table[y]];
		vrl_16bit *dtable = dither[y & 3];
		color = spans[y].startcolor;
		for (x = spans[y].startx; x <= spans[y].endx; ++x)
			{
			ptr[x] = spectable[((unsigned int) ((color + dtable[x&3]))) >> 8];
			color += cincrement;
			}
		}
	}

static void (*shadefunc)(void) = render_dithered_poly;

/* scan an edge from x1,y1 to x2,y2; two versions (left edge and right edge) */

static void barf(void)
	{
	}
	
static int gscan_left_edge(vrl_OutputVertex *first, vrl_OutputVertex *last)
	{
	vrl_ScreenCoord xincrement, x = first->x + (((vrl_ScreenCoord) left_border) << VRL_SCREEN_FRACT_BITS);
	vrl_ScreenPos y1 = round_up(first->y) + top_border, y2 = round_down(last->y) + top_border, y;
	vrl_16bit color = first->red, endcolor = last->red;
	vrl_16bit cincrement;
vrl_16bit lastcolor;
	if (y2 < y1) y2 = y1;
	xincrement = vrl_FactorMultiply(slope_table[y2 - y1], last->x - first->x);
	cincrement = vrl_FactorMultiply(slope_table[y2 - y1], endcolor - color);
	for (y = y1; y <= y2; ++y)
		{
		spans[y].startx = round_up(x);
		spans[y].startcolor = color;
		x += xincrement;
lastcolor = color;
		color += cincrement;
		}		
if ((lastcolor & 0xFF00) != (endcolor & 0xFF00)) barf();
	return 0;
	}

static int gscan_right_edge(vrl_OutputVertex *first, vrl_OutputVertex *last)
	{
	vrl_ScreenCoord xincrement, x = first->x + (((vrl_ScreenCoord) left_border) << VRL_SCREEN_FRACT_BITS);
	vrl_ScreenPos y1 = round_up(first->y) + top_border, y2 = round_down(last->y) + top_border, y;
	vrl_16bit color = first->red, endcolor = last->red;
	vrl_16bit cincrement;
vrl_16bit lastcolor;
	if (y2 < y1) y2 = y1;
	xincrement = vrl_FactorMultiply(slope_table[y2 - y1], last->x - first->x);
	cincrement = vrl_FactorMultiply(slope_table[y2 - y1], endcolor - color);
	for (y = y1; y <= y2; ++y)
		{
		spans[y].endx = round_down(x);
		spans[y].endcolor = color;
		x += xincrement;
lastcolor = color;
		color += cincrement;
		}		
if ((lastcolor & 0xFF00) != (endcolor & 0xFF00)) barf();
	return 0;
	}

static int gscan_outline(vrl_OutputVertex *list, void (*render_func)(void))
	{
	vrl_OutputVertex *v1 = list, *v2 = list->next, *v3 = list->next->next, *vn;
	while (v3 != v1)
		{
		vrl_OutputVertex *v;
		vn = v3->next;
		v1->next = v2;  v2->prev = v1;
		v2->next = v3;  v3->prev = v2;
		v3->next = v1;  v1->prev = v3;
		find_top(v1);
		if (starty >= endy) return 1;
		/* scan convert left-side edges */
		for (v = topvertex; v != botvertex; v = v->prev)
			if (gscan_left_edge(v, v->prev))
				return 1;
		/* scan convert right-side edges */
		for (v = topvertex; v != botvertex; v = v->next)
			if (gscan_right_edge(v, v->next))
				return 1;
		render_func();
		v2 = v3;
		v3 = vn;
		}
	return 0;
	}
#endif

/* Low-level span-filling routines */

static void render_flat_poly(vrl_Color color)
	{
	vrl_ScreenPos y;
	set_color(color);
	for (y = starty; y <= endy; ++y)
		hfill(spans[y].startx, spans[y].endx, y, color);
	}

static void render_metal_poly(vrl_Color color)
	{
	vrl_ScreenPos y;
	int dir = 1;
	int off = (color & 0x0F) + starty;
	if (off & 0x10) off = ~off;
	off &= 0x0F;
	color &= 0xF0;
	for (y = starty; y <= endy; ++y)
		{
		set_color(color | off);
		hfill(spans[y].startx, spans[y].endx, y, color | off);
		off += dir;
		if (off > 15) { off = 15;  dir = -1; }
		else if (off < 0) { off = 0; dir = 1; }
		}
	}

static void render_glass_poly(vrl_Color color)
	{
	vrl_ScreenPos y;
	int dir = 1;
	int off = (color & 0x0F) + starty;
	if (off & 0x10) off = ~off;
	off &= 0x0F;
	color &= 0xF0;
	for (y = starty; y <= endy; ++y)
		{
		glass_fill(y, off, color);
		off += dir;
		if (off > 15) { off = 15;  dir = -1; }
		else if (off < 0) { off = 0; dir = 1; }
		}
	}

/* This routine is based on an example by Mark Feldman, from the PC-GPE */

/* Should allow gouraud-shaded lines */

static int render_lines(vrl_OutputVertex *list, int closed)
	{
	vrl_OutputVertex *v = list, *last = closed ? list : list->prev;
	do
		{
		vrl_ScreenCoord deltax = labs(v->next->x - v->x);
		vrl_ScreenCoord deltay = labs(v->next->y - v->y);
		vrl_ScreenCoord d, dinc1, dinc2;
		vrl_ScreenCoord x, xinc1, xinc2, y, yinc1, yinc2;
		vrl_ScreenCoord i, numpixels;
		vrl_16bit color = v->red;
		set_color(color);
		if (deltax >= deltay)
			{
			numpixels = s2i(deltax);
			d = (deltay << 1) - deltax;
			dinc1 = deltay << 1;
			dinc2 = (deltay - deltax) << 1;
			xinc1 = SCREEN_UNITY;  xinc2 = SCREEN_UNITY;
			yinc1 = 0;  yinc2 = SCREEN_UNITY;
			}
		else
			{
			numpixels = s2i(deltay);
			d = (deltax << 1) - deltay;
			dinc1 = deltax << 1;
			dinc2 = (deltax - deltay) << 1;
			xinc1 = 0;  xinc2 = SCREEN_UNITY;
			yinc1 = SCREEN_UNITY;  yinc2 = SCREEN_UNITY;
			}
		if (v->x > v->next->x)
			{
			xinc1 = -xinc1;
			xinc2 = -xinc2;
			}
		if (v->y > v->next->y)
			{
			yinc1 = -yinc1;
			yinc2 = -yinc2;
			}
		x = v->x;  y = v->y;
		for (i = 0; i < numpixels; ++i)
			{
			setpixel(s2i(x), s2i(y), color);
			if (d < 0)
				{
				d += dinc1;
				x += xinc1;
				y += yinc1;
				}
			else
				{
				d += dinc2;
				x += xinc2;
				y += yinc2;
				}
			}
		v = v->next;
		} while (v != last);
	return 0;
	}

static int compute_line_table(void)
	{
	int i;
	if (line_table)
		vrl_free(line_table);
	line_table = vrl_calloc(vrl_RasterGetHeight(our_raster), sizeof(vrl_16bit));
	if (line_table == NULL) return -1;
	for (i = 0; i < vrl_RasterGetHeight(our_raster); ++i)
		line_table[i] = vrl_RasterGetRowbytes(our_raster) * i;
	return 0;
	}

static int compute_slope_table(void)
	{
	int i, n = max(vrl_RasterGetHeight(our_raster), vrl_RasterGetWidth(our_raster));
	if (slope_table)
		vrl_free(slope_table);
	slope_table = vrl_calloc(n, sizeof(vrl_Factor));
	if (slope_table == NULL) return -1;
	slope_table[0] = 0;
	for (i = 1; i < n; ++i)
		slope_table[i] = float2factor(1.0 / i);
	return 0;
	}

#define get_poly_color(p) (((vrl_OutputVertex *) (p))->color)

static vrl_ScreenPos text_x = 0, text_y = 0;

#ifdef MODE_Y
vrl_32bit vrl_DisplayDriverModeY(vrl_DisplayCommand cmd, vrl_32bit lparm1, void *pparm1)
#else
vrl_32bit vrl_DisplayDriverDefault(vrl_DisplayCommand cmd, vrl_32bit lparm1, void *pparm1)
#endif
	{
	switch (cmd)
		{
		case VRL_DISPLAY_GET_VERSION: return 1;
		case VRL_DISPLAY_GET_DESCRIPTION: strncpy((char *) pparm1, "Default scan-conversion routines", lparm1); return 0;
		case VRL_DISPLAY_INIT:
			{
			vrl_Raster *hardware_raster = vrl_VideoGetRaster();
			setup_color_table();
#ifdef DIRECT
			our_raster = hardware_raster;
#else
			if (pparm1) our_raster = pparm1;
			else
				{
				our_raster = vrl_RasterCreate(
					vrl_RasterGetWidth(hardware_raster),
					vrl_RasterGetHeight(hardware_raster),
					vrl_RasterGetDepth(hardware_raster)
					);
				if (our_raster == NULL) return -1;
				}
#endif
			if (compute_slope_table()) return -2;
			if (compute_line_table()) return -3;
			}
			/* fall through */
		case VRL_DISPLAY_CLEAR: clear_display(lparm1); break;
		case VRL_DISPLAY_POINT:
			{
			vrl_OutputVertex *vertlist = pparm1, *v = pparm1;
			do
				{
				setpixel(s2i(v->x), s2i(v->y), v->red);
				v = v->next;
				} while (v != vertlist);
			}
			break;
		case VRL_DISPLAY_LINE: render_lines(pparm1, 0); break;
		case VRL_DISPLAY_CLOSED_LINE: render_lines(pparm1, 1); break;
		case VRL_DISPLAY_POLY:
			{
			vrl_OutputFacet *f = pparm1;
			switch (f->surface->type)
				{
				case VRL_SURF_METAL:
					if (!scan_outline(f->points))
						render_metal_poly(f->color);
					break;
				case VRL_SURF_GLASS:
					if (!scan_outline(f->points))
						render_glass_poly(f->color);
					break;
#ifndef MODE_Y
				case VRL_SURF_SPECULAR:
					spectable = specular_table[f->surface->exp];
					gscan_outline(f->points, render_specular_poly);
					break;
				case VRL_SURF_GOURAUD:
					if (shadefunc)
						{
						gscan_outline(f->points, shadefunc);
						break;
						}
#endif
				default:
					if (!scan_outline(f->points))
						render_flat_poly(f->color);
					break;
				}
			}
			break;
		case VRL_DISPLAY_BOX:
			{
			vrl_OutputVertex *v = pparm1;
			vrl_ScreenPos row, endrow = s2i(v->next->y);
			vrl_ScreenPos xstart = s2i(v->x), xend = s2i(v->next->x);
			for (row = s2i(v->y); row <= endrow; ++row)
				hfill(xstart, xend, row, v->red);
			}
			break;
		case VRL_DISPLAY_BEGIN_FRAME:
			vrl_RasterGetWindow(our_raster, &left_border, &top_border, NULL, NULL);
			break;
		case VRL_DISPLAY_TEXT:
			{
			char *s = pparm1;
			while (*s)
				text_x += render_char(text_x, text_y, *s++, lparm1);
			}
			break;
		case VRL_DISPLAY_TEXT_POSITION: text_x = lparm1 >> 16; text_y = lparm1; break;
		case VRL_DISPLAY_GET_TEXTWIDTH: return strlen(pparm1) * 8;
		case VRL_DISPLAY_GET_TEXTHEIGHT: return 8;
		case VRL_DISPLAY_SET_RASTER: our_raster = pparm1; break;
		case VRL_DISPLAY_GET_RASTER: *((vrl_Raster **) pparm1) = our_raster; break;
#ifndef MODE_Y
		case VRL_DISPLAY_CAN_GOURAUD: return 1;
		case VRL_DISPLAY_UPDATE_PALETTE: specular_update(pparm1); break;
		case VRL_DISPLAY_SET_SHADING:
			switch (lparm1)
				{
				case 0: shadefunc = NULL; break;
				case 1: shadefunc = render_gouraud_poly; break;
				case 2: shadefunc = render_fast_dithered_poly; break;
				case 3: shadefunc = render_dithered_poly; break;
				default: break;
				}
			break;
#endif
		default: break;
		}
	return 0;
	}
