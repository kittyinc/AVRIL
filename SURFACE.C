/* Routines for handling surface descriptors in AVRIL */
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

static vrl_Surface *surfaces = NULL;

vrl_Surface *vrl_SurfaceInit(vrl_Surface *surf)
	{
	if (surf == NULL) return NULL;
	surf->type = VRL_SURF_FLAT;
	surf->hue = 0;
	surf->brightness = 128;
	surf->exp = 0;
	surf->next = surfaces;
	surfaces = surf;
	return surf;
	}

vrl_Surface *vrl_SurfaceGetList(void)
	{
	return surfaces;
	}

vrl_Surface *vrl_SurfaceCreate(unsigned char hue)
	{
	vrl_Surface *s = vrl_SurfaceInit(vrl_malloc(sizeof(vrl_Surface)));
	if (s == NULL) return NULL;
	s->hue = hue;
	return s;
	}

vrl_Surface *vrl_SurfaceFromDesc(vrl_unsigned16bit desc, vrl_Surface *surf)
	{
	if (surf == NULL) return NULL;
	surf->type = (desc >> 12) & 0x0F;
	surf->hue = (desc >> 8) & 0x0F;
	surf->brightness = desc & 0xFF;
	if (surf->type == VRL_SURF_SIMPLE && surf->hue != 0)
		{
		surf->hue = 0;
		surf->brightness = (surf->hue << 4) | ((surf->brightness >> 4) & 0x0F);
		}
	if (surf->type == VRL_SURF_SPECULAR)
		{
		surf->exp = surf->brightness & 0x0F;
		surf->brightness &= 0xF0;
		}
	return surf;
	}

vrl_unsigned16bit vrl_SurfaceToDesc(vrl_Surface *surf)
	{
	vrl_unsigned16bit desc;
	desc = (surf->type << 12) | (surf->hue << 8) | (surf->brightness);
	if (surf->type == VRL_SURF_SPECULAR)
		desc |= surf->exp;
	return desc;
	}

static vrl_Surfacemap *maps = NULL;

vrl_Surfacemap *vrl_SurfacemapCreate(int n)
	{
	int i;
	vrl_Surfacemap *m = vrl_malloc(sizeof(vrl_Surfacemap));
	if (m == NULL) return NULL;
	m->nentries = n;
	m->entries = vrl_calloc(n, sizeof(vrl_Surface *));
	if (m->entries == NULL)
		{
		vrl_free(m);
		return NULL;
		}
	for (i = 0; i < n; ++i)
		m->entries[i] = NULL;
	m->next = maps;  maps = m;
	return m;
	}

vrl_Surfacemap *vrl_SurfacemapGetList(void)
	{
	return maps;
	}

