/* Display routines for AVRIL */
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
#include <string.h>

#include "palettes.h"

static unsigned char current_palette[256*3];

/* Stereo stuff: */

static vrl_ScreenPos left_x1, left_y1, left_x2, left_y2;      /* left-eye window */
static vrl_ScreenPos right_x1, right_y1, right_x2, right_y2;  /* right-eye window */

static unsigned long left_color = 4, right_color = 1, both_color = 255;
/* red and blue by default, with white for both */

static void (*cardfunction)(VRL_STEREO_EYE);  /* function to select card to write to */

static VRL_STEREO_EYE draw_eye = VRL_STEREOEYE_LEFT;
static VRL_STEREO_EYE view_eye = VRL_STEREOEYE_LEFT;
static VRL_STEREO_TYPE stereotype = VRL_STEREOTYPE_NONE;
static vrl_StereoConfiguration *stereo_conf = NULL;

/* Other stuff: */

static int viewpage = 0, drawpage = 0, npages = 1;
static int depth = 8;

extern vrl_DisplayDriverFunction vrl_DisplayDriverDefault;
extern vrl_VideoDriverFunction vrl_VideoDriverMode13;

vrl_DisplayDriverFunction *vrl_current_display_driver = vrl_DisplayDriverDefault;
vrl_VideoDriverFunction *vrl_current_video_driver = vrl_VideoDriverMode13;

static vrl_Boolean in_graphics_mode = 0;
static vrl_DisplayDriverFunction *old_driver = NULL;

static void real_SetPalette(int start, int end, void *palette)
	{
	(*vrl_current_video_driver)(VRL_VIDEO_SET_PALETTE, (((vrl_32bit) start) << 16) | end, palette);
	}

static void display_reset(void)
	{
	stereotype = VRL_STEREOTYPE_NONE;
	draw_eye = view_eye = VRL_STEREOEYE_LEFT;
	viewpage = drawpage = 0;
	depth = vrl_DisplayGetDepth();
	npages = (*vrl_current_video_driver)(VRL_VIDEO_GET_NPAGES, 0, NULL);
	if (vrl_DisplayGetDepth() == 4)
		{
		memcpy(current_palette, default_palette_16.data, 48);
		real_SetPalette(0, 15, &default_palette_16.data);
		vrl_DisplayUpdatePalette(&default_palette_16);
		}
	else  /* assume 8 */
		{
		memcpy(current_palette, default_palette_256.data, 256);
		real_SetPalette(0, 255, &default_palette_256.data);
		vrl_DisplayUpdatePalette(&default_palette_256);
		}
	vrl_DisplaySetWindow(0, 0, vrl_DisplayGetWidth()-1, vrl_DisplayGetHeight()-1);
	vrl_DisplayGetWindow(&left_x1, &left_y1, &left_x2, &left_y2);
	vrl_DisplayGetWindow(&right_x1, &right_y1, &right_x2, &right_y2);
	}

int vrl_DisplayInit(vrl_Raster *raster)
	{
	int n;
	if (vrl_current_video_driver == NULL)
		vrl_current_video_driver = vrl_VideoDriverMode13;
	if (vrl_current_display_driver == NULL)
		vrl_current_display_driver = vrl_DisplayDriverDefault;
	n = (vrl_current_display_driver)(VRL_DISPLAY_INIT, 0, raster);
	if (n)
		return n;
	display_reset();
	return 0;
	}

void vrl_DisplayQuit(void)
	{
	(*vrl_current_display_driver)(VRL_DISPLAY_QUIT, 0, NULL);
	}

void vrl_DisplayPoint(vrl_ScreenPos x, vrl_ScreenPos y, vrl_Color color)
	{
	vrl_OutputVertex v;
	v.x = x;  v.y = y;  v.z = 1;
	v.x <<= VRL_SCREEN_FRACT_BITS;  v.y <<= VRL_SCREEN_FRACT_BITS;
	v.next = v.prev = &v;
	v.red = color;
	(*vrl_current_display_driver)(VRL_DISPLAY_POINT, VRL_SURF_SIMPLE, &v);
	}

void vrl_DisplayLine(vrl_ScreenPos x1, vrl_ScreenPos y1, vrl_ScreenPos x2, vrl_ScreenPos y2, vrl_Color color)
	{
	vrl_OutputVertex v1, v2, *v;
	v1.x = x1;  v1.y = y1;  v1.z = 1;
	v2.x = x2;  v2.y = y2;  v2.z = 1;
	v1.x <<= VRL_SCREEN_FRACT_BITS;  v1.y <<= VRL_SCREEN_FRACT_BITS;
	v2.x <<= VRL_SCREEN_FRACT_BITS;  v2.y <<= VRL_SCREEN_FRACT_BITS;
	v1.red = v2.red = color;
	v1.next = v1.prev = &v2;  v2.prev = v2.next = &v1;
	v = vrl_DisplayXYclipPoly(&v1, VRL_DISPLAY_XYCLIP_CLIP_X|VRL_DISPLAY_XYCLIP_CLIP_Y);
	if (v)
		(*vrl_current_display_driver)(VRL_DISPLAY_LINE, VRL_SURF_SIMPLE, v);
	}

void vrl_DisplayBox(vrl_ScreenPos x1, vrl_ScreenPos y1, vrl_ScreenPos x2, vrl_ScreenPos y2, vrl_Color color)
	{
	vrl_OutputVertex v1, v2;
	v1.x = x1;  v1.y = y1;  v1.z = 1;
	v2.x = x2;  v2.y = y2;  v2.z = 1;
	v1.x <<= VRL_SCREEN_FRACT_BITS;  v1.y <<= VRL_SCREEN_FRACT_BITS;
	v2.x <<= VRL_SCREEN_FRACT_BITS;  v2.y <<= VRL_SCREEN_FRACT_BITS;
	v1.red = v2.red = color;
	v1.next = v1.prev = &v2;  v2.prev = v2.next = &v1;
	(*vrl_current_display_driver)(VRL_DISPLAY_BOX, VRL_SURF_SIMPLE, &v1);
	}

void vrl_DisplayText(vrl_ScreenPos x, vrl_ScreenPos y, vrl_Color color, char *message)
	{
	(*vrl_current_display_driver)(VRL_DISPLAY_TEXT_POSITION, (((vrl_32bit) x) << 16) | y, NULL);
	(*vrl_current_display_driver)(VRL_DISPLAY_TEXT, color, message);
	}

static void real_SetWindow(vrl_ScreenPos x1, vrl_ScreenPos y1, vrl_ScreenPos x2, vrl_ScreenPos y2)
	{
	vrl_RasterSetWindow(vrl_DisplayGetRaster(), x1, y1, x2, y2);
	vrl_DisplaySetXYclip(x1, y1, x2, y2);
	}

void vrl_DisplaySetWindow(vrl_ScreenPos x1, vrl_ScreenPos y1, vrl_ScreenPos x2, vrl_ScreenPos y2)
	{
	switch (stereotype)
		{
		default: 
			real_SetWindow(x1, y1, x2, y2);
		case VRL_STEREOTYPE_FRESNEL:
		case VRL_STEREOTYPE_CYBERSCOPE:
		case VRL_STEREOTYPE_CRYSTALEYES:
		case VRL_STEREOTYPE_ANAGLYPH_WIRE_ALTERNATE:
		case VRL_STEREOTYPE_ANAGLYPH_SOLID_ALTERNATE:
		case VRL_STEREOTYPE_ENIGMA:
			break;
		}
	}

void vrl_DisplayGetWindow(vrl_ScreenPos *x1, vrl_ScreenPos *y1, vrl_ScreenPos *x2, vrl_ScreenPos *y2)
	{
	vrl_RasterGetWindow(vrl_DisplayGetRaster(), x1, y1, x2, y2);
	}

void vrl_VideoSetPalette(int start, int end, vrl_Palette *palette)
	{
	memcpy(&current_palette[3*start], &palette->data[3*start], 3*(end - start + 1));
	switch (stereotype)
		{
		default:
			real_SetPalette(start, end, palette->data);
			vrl_DisplayUpdatePalette(palette);
		case VRL_STEREOTYPE_ANAGLYPH_SEQUENTIAL:
		case VRL_STEREOTYPE_ANAGLYPH_WIRE_ALTERNATE:
		case VRL_STEREOTYPE_ANAGLYPH_SOLID_ALTERNATE:
		case VRL_STEREOTYPE_CHROMADEPTH:
		case VRL_STEREOTYPE_SIRDS:
			break;
		}
	}

void vrl_DisplayGetPalette(int start, int end, vrl_Palette *palette)
	{
	memcpy(&palette->data[3*start], &current_palette[3*start], 3*(end - start + 1));
	}

vrl_Color vrl_DisplayComputeColor(vrl_Surface *surf, vrl_Factor intensity, vrl_Factor ambient, vrl_Scalar depth)
	{
	vrl_Hue *huemap = vrl_PaletteGetHuemap(vrl_WorldGetPalette()), *hueptr;
	hueptr = &huemap[surf->hue];
	if (ambient)
		intensity = vrl_ScalarMultDiv(intensity + ambient, 1, 2);  /* should use a coefficient */
	if (intensity < 0) intensity = 0;
	if (intensity > VRL_UNITY) intensity = VRL_UNITY;
	switch (stereotype)
		{
		case VRL_STEREOTYPE_CHROMADEPTH:
			{
			vrl_Scalar chromanear, chromafar;
			if (stereo_conf == NULL) return 255;
			if (depth < (chromanear = vrl_StereoGetChromaNear(stereo_conf))) return 16;
			if (depth > (chromafar = vrl_StereoGetChromaFar(stereo_conf))) return 255;
			return (((depth-chromanear)*240)/(chromafar - chromanear) + 16);
			}
		case VRL_STEREOTYPE_ANAGLYPH_SEQUENTIAL:
		case VRL_STEREOTYPE_ANAGLYPH_SOLID_ALTERNATE:
			{
			vrl_unsigned16bit inten = vrl_FactorMultiply(intensity, 65535);
			return ((inten >> 10) & 0x3F) + ((draw_eye == VRL_STEREOEYE_LEFT) ? 16 : 80);
			}
		default: break;
		}
	if (depth == 4)
		{
		if (surf->type == VRL_SURF_SIMPLE)
			return (surf->brightness >> 4) & 0x0F;
		return surf->hue & 0x0F;
		}
	/* else depth == 8 */
	switch (surf->type)
		{
		case VRL_SURF_SIMPLE: return surf->brightness;
		case VRL_SURF_METAL:
		case VRL_SURF_GLASS:
			return hueptr->start + vrl_FactorMultiply(intensity, hueptr->maxshade);
		case VRL_SURF_FLAT:
		case VRL_SURF_GOURAUD:
		case VRL_SURF_SPECULAR:
			{
			vrl_unsigned16bit value;
			value = vrl_FactorMultiply(intensity, surf->brightness * hueptr->maxshade);
			return hueptr->start + ((value + 128) >> 8);  /* round up */
			}
		default:
			break;
		}
	return 1;  /* by default, return color 1 (blue in most palettes) */
	}

void vrl_DisplayComputeVertexColor(vrl_OutputVertex *v, vrl_Surface *surf, vrl_Factor intensity, vrl_Factor ambient, vrl_Scalar depth)
	{
	vrl_Hue *huemap = vrl_PaletteGetHuemap(vrl_WorldGetPalette()), *hueptr;
	hueptr = &huemap[surf->hue];
	if (ambient)
		intensity = vrl_ScalarMultDiv(intensity + ambient, 1, 2);  /* should use a coefficient */
	if (intensity < 0) intensity = 0;
	if (intensity > VRL_UNITY) intensity = VRL_UNITY;
	switch (stereotype)
		{
		case VRL_STEREOTYPE_CHROMADEPTH:
			{
			vrl_Scalar chromanear, chromafar;
			if (stereo_conf == NULL) v->red = 255;
			else if (depth < (chromanear = vrl_StereoGetChromaNear(stereo_conf))) v->red = 16 << 8;
			else if (depth > (chromafar = vrl_StereoGetChromaFar(stereo_conf))) v->red = 255 << 8;
			else v->red = (((depth-chromanear)*240)/(chromafar - chromanear) + 16);
			return;
			}
		case VRL_STEREOTYPE_ANAGLYPH_SEQUENTIAL:
		case VRL_STEREOTYPE_ANAGLYPH_SOLID_ALTERNATE:
			{
			vrl_unsigned16bit inten = vrl_FactorMultiply(intensity, 65535);
			v->red = ((inten >> 10) & 0x3F) + ((draw_eye == VRL_STEREOEYE_LEFT) ? 16 : 80);
			return;
			}
		default: break;
		}
	if (depth == 4)
		{
		if (surf->type == VRL_SURF_SIMPLE)
			v->red = (surf->brightness >> 4) & 0x0F;
		else
			v->red = surf->hue & 0x0F;
		return;
		}
	/* else depth == 8 */
	switch (surf->type)
		{
		case VRL_SURF_SIMPLE: v->red = surf->brightness; return;
		case VRL_SURF_METAL:
		case VRL_SURF_GLASS:
			v->red = hueptr->start + vrl_FactorMultiply(intensity, hueptr->maxshade);
			break;
		case VRL_SURF_FLAT:
		case VRL_SURF_GOURAUD:
		case VRL_SURF_SPECULAR:
			{
			vrl_unsigned16bit value;
			value = vrl_FactorMultiply(intensity, surf->brightness * (hueptr->maxshade-1));
			v->red = (hueptr->start << 8) + value + 256;
			}
			return;
		default:
			break;
		}
	v->red = 1;  /* by default, return color 1 (blue in most palettes) */
	return;
	}

int vrl_VideoGetNpages(void)
	{
	int n = npages;
	switch (stereotype)
		{
		case VRL_STEREOTYPE_SEQUENTIAL:
		case VRL_STEREOTYPE_ANAGLYPH_SEQUENTIAL: n >>= 1;  /* half the number of pages */
		default: break;
		}
	return n;
	}

void vrl_VideoSetDrawPage(int page)
	{
	drawpage = page;
	switch (stereotype)
		{
		case VRL_STEREOTYPE_SEQUENTIAL:
		case VRL_STEREOTYPE_ANAGLYPH_SEQUENTIAL:
			if (draw_eye == VRL_STEREOEYE_RIGHT)
				page += (npages >> 1);   /* in second half of real page set */
		default: break;
		}
	(*vrl_current_video_driver)(VRL_VIDEO_SET_DRAW_PAGE, page, NULL);
	}

int vrl_VideoGetDrawPage(void)
	{
	return drawpage;
	}

void vrl_VideoSetViewPage(int page)
	{
	viewpage = page;
	switch (stereotype)
		{
		case VRL_STEREOTYPE_SEQUENTIAL:
		case VRL_STEREOTYPE_ANAGLYPH_SEQUENTIAL:
			if (view_eye == VRL_STEREOEYE_RIGHT)
				page += (npages >> 1);   /* in second half of real page set */
			break;
		default: break;
		}
	(*vrl_current_video_driver)(VRL_VIDEO_SET_VIEW_PAGE, page, NULL);
	}

int vrl_VideoGetViewPage(void)
	{
	return viewpage;
	}

static void retrace_handler(void)
	{
	int page = viewpage;
	view_eye = (view_eye == VRL_STEREOEYE_LEFT) ? VRL_STEREOEYE_RIGHT : VRL_STEREOEYE_LEFT;
	if (view_eye == VRL_STEREOEYE_RIGHT)
		page += (npages >> 1);
	(*vrl_current_video_driver)(VRL_VIDEO_SET_VIEW_PAGE, page, NULL);
	}

void vrl_DisplayStereoSetType(VRL_STEREO_TYPE stype)
	{
	display_reset();
	stereotype = stype;
	draw_eye = view_eye = VRL_STEREOEYE_LEFT;
	stereo_conf = vrl_WorldGetStereoConfiguration();
	switch (stype)
		{
		case VRL_STEREOTYPE_FRESNEL:
			vrl_DisplayStereoSetLeftWindow(0, 0, vrl_DisplayGetWidth()/2-1, vrl_DisplayGetHeight()/2-1);
			vrl_DisplayStereoSetRightWindow(vrl_DisplayGetWidth()/2, 0, vrl_DisplayGetWidth()-1, vrl_DisplayGetHeight()/2-1);
			break;
		case VRL_STEREOTYPE_CYBERSCOPE:
			vrl_DisplayStereoSetLeftWindow(0, 0, vrl_DisplayGetWidth()/2-1, vrl_DisplayGetHeight()-1);
			vrl_DisplayStereoSetRightWindow(vrl_DisplayGetWidth()/2, 0, vrl_DisplayGetWidth()-1, vrl_DisplayGetHeight()-1);
			break;
		case VRL_STEREOTYPE_CRYSTALEYES:
			vrl_DisplayStereoSetLeftWindow(0, 0, vrl_DisplayGetWidth()-1, 95);
			vrl_DisplayStereoSetRightWindow(0, 100, vrl_DisplayGetWidth()-1, vrl_DisplayGetHeight()-1);
			break;
		case VRL_STEREOTYPE_CHROMADEPTH:
			real_SetPalette(0, 255, &chromadepth_palette.data);
			break;
		case VRL_STEREOTYPE_ANAGLYPH_SEQUENTIAL:
			real_SetPalette(0, 255, &anaglyph_palette.data);
		case VRL_STEREOTYPE_SEQUENTIAL:
			vrl_SetRetraceHandler(retrace_handler);
			break;
		case VRL_STEREOTYPE_ANAGLYPH_WIRE_ALTERNATE:
			vrl_RenderSetDrawMode(1);  /* wireframe */
		case VRL_STEREOTYPE_ANAGLYPH_SOLID_ALTERNATE:
			real_SetPalette(0, 255, &anaglyph_palette.data);
			vrl_WorldSetSkyColor(0);
			vrl_WorldSetGroundColor(0);
		case VRL_STEREOTYPE_ENIGMA:
			vrl_RasterSetRowbytes(vrl_DisplayGetRaster(), 2*vrl_RasterGetRowbytes(vrl_DisplayGetRaster()));
			(vrl_current_display_driver)(VRL_DISPLAY_INIT, 0, vrl_DisplayGetRaster());
			vrl_DisplayStereoSetLeftWindow(0, 0, vrl_DisplayGetWidth()-1, vrl_DisplayGetHeight()/2-1);
			vrl_DisplayStereoSetRightWindow(vrl_DisplayGetWidth(), 0, vrl_DisplayGetWidth()*2-1, vrl_DisplayGetHeight()/2-1);
			break;
		case VRL_STEREOTYPE_TWOCARDS:
		case VRL_STEREOTYPE_SIRDS:
			break;
		default:
			stereotype = VRL_STEREOTYPE_NONE;
			break;
		}
	}

VRL_STEREO_TYPE vrl_DisplayStereoGetType(void)
	{
	return stereotype;
	}

void vrl_DisplayStereoSetDrawEye(VRL_STEREO_EYE eye)
	{
	draw_eye = eye;
	switch (stereotype)
		{
		case VRL_STEREOTYPE_SEQUENTIAL:
		case VRL_STEREOTYPE_ANAGLYPH_SEQUENTIAL:
			{
			int page = drawpage;
			if (eye == VRL_STEREOEYE_RIGHT)
				page += (npages >> 1);
			(*vrl_current_video_driver)(VRL_VIDEO_SET_DRAW_PAGE, page, NULL);
			}
			break;
		case VRL_STEREOTYPE_ANAGLYPH_WIRE_ALTERNATE:
			switch (eye)
				{
				case VRL_STEREOEYE_LEFT: vrl_RenderSetWireframeColor(left_color); break;
				case VRL_STEREOEYE_RIGHT: vrl_RenderSetWireframeColor(right_color); break;
				case VRL_STEREOEYE_BOTH: vrl_RenderSetWireframeColor(both_color); break;
				default: break;
				}
			/* fall through! */
		case VRL_STEREOTYPE_FRESNEL:
		case VRL_STEREOTYPE_CYBERSCOPE:
		case VRL_STEREOTYPE_CRYSTALEYES:
		case VRL_STEREOTYPE_ANAGLYPH_SOLID_ALTERNATE:
		case VRL_STEREOTYPE_ENIGMA:
			switch (eye)
				{
				case VRL_STEREOEYE_LEFT: real_SetWindow(left_x1, left_y1, left_x2, left_y2); break;
				case VRL_STEREOEYE_RIGHT: real_SetWindow(right_x1, right_y1, right_x2, right_y2); break;
				default: break;
				}
			break;
		case VRL_STEREOTYPE_TWOCARDS:
			if (cardfunction)
				(*cardfunction)(eye);
			break;
		default: break;
		}
	}

VRL_STEREO_EYE vrl_DisplayStereoGetDrawEye(void)
	{
	return draw_eye;
	}

void vrl_DisplayStereoSetViewEye(VRL_STEREO_EYE eye)
	{
	view_eye = eye;
	switch (stereotype)
		{
		case VRL_STEREOTYPE_SEQUENTIAL:
		case VRL_STEREOTYPE_ANAGLYPH_SEQUENTIAL:
			{
			int page = viewpage;
			if (eye == VRL_STEREOEYE_RIGHT)
				page += (npages >> 1);
			(*vrl_current_video_driver)(VRL_VIDEO_SET_VIEW_PAGE, page, NULL);
			}
			break;
		default: break;
		}
	}

VRL_STEREO_EYE vrl_DisplayStereoGetViewEye(void)
	{
	return view_eye;
	}

void vrl_DisplayStereoSetLeftWindow(vrl_ScreenPos x1, vrl_ScreenPos y1, vrl_ScreenPos x2, vrl_ScreenPos y2)
	{
	left_x1 = x1;  left_y1 = y1;
	left_x2 = x2;  left_y2 = y2;
	}

void vrl_DisplayStereoSetRightWindow(vrl_ScreenPos x1, vrl_ScreenPos y1, vrl_ScreenPos x2, vrl_ScreenPos y2)
	{
	right_x1 = x1;  right_y1 = y1;
	right_x2 = x2;  right_y2 = y2;
	}

void vrl_DisplayStereoSetCardFunction(void (*function)(VRL_STEREO_EYE))
	{
	cardfunction = function;
	}

void vrl_DisplayClear(vrl_Color color)
	{
	switch (stereotype)
		{
		case VRL_STEREOTYPE_SEQUENTIAL:
		case VRL_STEREOTYPE_ANAGLYPH_SEQUENTIAL:
			{
			/* clear the left-eye page */
			(*vrl_current_video_driver)(VRL_VIDEO_SET_DRAW_PAGE, drawpage, NULL);
			(*vrl_current_display_driver)(VRL_DISPLAY_CLEAR, color, NULL);
			/* clear the right-eye page */
			(*vrl_current_video_driver)(VRL_VIDEO_SET_DRAW_PAGE, drawpage + (npages >> 1), NULL);
			(*vrl_current_display_driver)(VRL_DISPLAY_CLEAR, color, NULL);
			}
			break;
		default:
			(*vrl_current_display_driver)(VRL_DISPLAY_CLEAR, color, NULL);
			break;
		}
	}
	
static char ddesc[100];

char *vrl_VideoGetDescription(void)
	{
	strcpy(ddesc, "Unknown video driver");
	(*vrl_current_video_driver)(VRL_VIDEO_GET_DESCRIPTION, sizeof(ddesc)-2, ddesc);
	return ddesc;
	}

static char vdesc[100];

char *vrl_DisplayGetDescription(void)
	{
	strcpy(vdesc, "Unknown video driver");
	(*vrl_current_display_driver)(VRL_DISPLAY_GET_DESCRIPTION, sizeof(vdesc)-2, vdesc);
	return vdesc;
	}

vrl_Raster *vrl_DisplayGetRaster(void)
	{
	vrl_Raster *r;
	(*vrl_current_display_driver)(VRL_DISPLAY_GET_RASTER, 0, &r);
	return r;
	}

vrl_Raster *vrl_VideoGetRaster(void)
	{
	vrl_Raster *r;
	(*vrl_current_video_driver)(VRL_VIDEO_GET_RASTER, 0, &r);
	return r;
	}

void vrl_VideoShutdown(void)
	{
	(*vrl_current_video_driver)(VRL_VIDEO_SHUTDOWN, 0, NULL);
	}

