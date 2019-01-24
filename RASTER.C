/* Raster routines for AVRIL */
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

vrl_Raster *vrl_RasterCreate(vrl_ScreenPos width, vrl_ScreenPos height, vrl_unsigned16bit depth)
	{
	vrl_Raster *r = vrl_malloc(sizeof(vrl_Raster));
	unsigned int nbytes;
	if (r == NULL) return NULL;
	r->width = width;  r->height = height;  r->depth = depth;
	r->top = r->left = 0;  r->right = width-1;  r->bottom = height-1;
	r->rowbytes = width * ((depth+7)/8);
	nbytes = r->height * r->rowbytes;
	r->data = vrl_malloc(nbytes);
	if (r->data)
		return r;
	vrl_free(r);
	return NULL;
	}

void vrl_RasterDestroy(vrl_Raster *r)
	{
	if (r->data)
		vrl_free(r->data);
	vrl_free(r);
	}

void vrl_RasterSetWindow(vrl_Raster *raster, vrl_ScreenPos left, vrl_ScreenPos top, vrl_ScreenPos right, vrl_ScreenPos bottom)
	{
	raster->left = left;  raster->top = top;
	raster->right = right;  raster->bottom = bottom;
	}

void vrl_RasterGetWindow(vrl_Raster *raster, vrl_ScreenPos *left, vrl_ScreenPos *top, vrl_ScreenPos *right, vrl_ScreenPos *bottom)
	{
	if (left) *left = raster->left;
	if (top) *top = raster->top;
	if (right) *right = raster->right;
	if (bottom) *bottom = raster->bottom;
	}

