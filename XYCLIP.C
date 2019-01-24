/* XY clipping code; this can definitely be improved upon! */

/* Only used in draw_polygon() */
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

static vrl_OutputVertex a1[100], a2[100];

static vrl_ScreenCoord left, right, top, bottom;

void vrl_DisplaySetXYclip(vrl_ScreenPos l, vrl_ScreenPos t, vrl_ScreenPos r, vrl_ScreenPos b)
	{
	left = 0;  right = ((vrl_ScreenCoord) (r - l)) << VRL_SCREEN_FRACT_BITS;
	top = 0;  bottom = ((vrl_ScreenCoord) (b - t)) << VRL_SCREEN_FRACT_BITS;
	}

#define MAXPOINTS 20

vrl_OutputVertex *vrl_DisplayXYclipPoint(vrl_OutputVertex *vertices, unsigned int flags)
	{
	return (vertices->x < left || vertices->y < top
		|| vertices->x > right || vertices->y > bottom) ? NULL : vertices;
	}

vrl_OutputVertex *vrl_DisplayXYclipLine(vrl_OutputVertex *vertices, unsigned int flags)
	{
	return vrl_DisplayXYclipPoly(vertices, flags);
	}

vrl_OutputVertex *vrl_DisplayXYclipPoly(vrl_OutputVertex *vertices, unsigned int flags)
	{
	vrl_OutputVertex *src = a2, *dst = a1;
	int nout = 0, i, npts = 0;
	vrl_OutputVertex *v1, *v2, *v = vertices;
	do
		{
		src[npts++] = *v;
		v = v->next;
		} while (v != vertices && npts < MAXPOINTS);

	v1 = &src[npts-1];
	for (i = 0; i < npts; ++i)
		{
		v2 = &src[i];
		if (v1->x >= left && v2->x >= left)  /* on-screen */
			dst[nout++] = src[i];
		else if (v1->x < left && v2->x < left)  /* off-screen */
			;
		else if (v1->x >= left && v2->x < left)   /* leaving */
			{
			vrl_Scalar numer = left - v2->x;
			vrl_Scalar denom = v1->x - v2->x;
			dst[nout].x = left;
			dst[nout].y = v2->y + vrl_ScalarMultDiv(v1->y - v2->y, numer, denom);
			if (flags & VRL_DISPLAY_XYCLIP_Z)
				dst[nout].z = v2->z + vrl_ScalarMultDiv(v1->z - v2->z, numer, denom);
			if (flags & VRL_DISPLAY_XYCLIP_INTENSITY)
				dst[nout].intensity = v2->intensity
					+ vrl_ScalarMultDiv(v1->intensity - v2->intensity, numer, denom);
			else
				dst[nout].intensity = v2->intensity;
			++nout;
			}
		else  /* entering */
			{
			vrl_Scalar numer = left - v1->x;
			vrl_Scalar denom = v2->x - v1->x;
			dst[nout].x = left;
			dst[nout].y = v1->y + vrl_ScalarMultDiv(v2->y - v1->y, numer, denom);
			if (flags & VRL_DISPLAY_XYCLIP_Z)
				dst[nout].z = v1->z + vrl_ScalarMultDiv(v2->z - v1->z, numer, denom);
			if (flags & VRL_DISPLAY_XYCLIP_INTENSITY)
				dst[nout].intensity = v1->intensity
					+ vrl_ScalarMultDiv(v2->intensity - v1->intensity, numer, denom);
			else
				dst[nout].intensity = v1->intensity;
			++nout;
			dst[nout++] = src[i];
			}
		v1 = v2;
		}

	if (dst == a1) { dst = a2; src = a1; }
	else { dst = a1; src = a2; }
	npts = nout;
	nout = 0;
	v1 = &src[npts-1];
	for (i = 0; i < npts; ++i)
		{
		v2 = &src[i];
		if (v1->x <= right && v2->x <= right)  /* on-screen */
			dst[nout++] = src[i];
		else if (v1->x > right && v2->x > right)  /* off-screen */
			;
		else if (v1->x <= right && v2->x > right)   /* leaving */
			{
			vrl_Scalar numer = right - v2->x;
			vrl_Scalar denom = v1->x - v2->x;
			dst[nout].x = right;
			dst[nout].y = v2->y + vrl_ScalarMultDiv(v1->y - v2->y, numer, denom);
			if (flags & VRL_DISPLAY_XYCLIP_Z)
				dst[nout].z = v2->z + vrl_ScalarMultDiv(v1->z - v2->z, numer, denom);
			if (flags & VRL_DISPLAY_XYCLIP_INTENSITY)
				dst[nout].intensity = v2->intensity
					+ vrl_ScalarMultDiv(v1->intensity - v2->intensity, numer, denom);
			else
				dst[nout].intensity = v2->intensity;
			++nout;
			}
		else  /* entering */
			{
			vrl_Scalar numer = right - v1->x;
			vrl_Scalar denom = v2->x - v1->x;
			dst[nout].x = right;
			dst[nout].y = v1->y + vrl_ScalarMultDiv(v2->y - v1->y, numer, denom);
			if (flags & VRL_DISPLAY_XYCLIP_Z)
				dst[nout].z = v1->z + vrl_ScalarMultDiv(v2->z - v1->z, numer, denom);
			if (flags & VRL_DISPLAY_XYCLIP_INTENSITY)
				dst[nout].intensity = v1->intensity
					+ vrl_ScalarMultDiv(v2->intensity - v1->intensity, numer, denom);
			else
				dst[nout].intensity = v1->intensity;
			++nout;
			dst[nout++] = src[i];
			}
		v1 = v2;
		}

	if (dst == a1) { dst = a2; src = a1; }
	else { dst = a1; src = a2; }
	npts = nout;
	nout = 0;
	v1 = &src[npts-1];
	for (i = 0; i < npts; ++i)
		{
		v2 = &src[i];
		if (v1->y >= top && v2->y >= top)  /* on-screen */
			dst[nout++] = src[i];
		else if (v1->y < top && v2->y < top)  /* off-screen */
			;
		else if (v1->y >= top && v2->y < top)   /* leaving */
			{
			vrl_Scalar numer = top - v2->y;
			vrl_Scalar denom = v1->y - v2->y;
			dst[nout].x = v2->x + vrl_ScalarMultDiv(v1->x - v2->x, numer, denom);
			dst[nout].y = top;
			if (flags & VRL_DISPLAY_XYCLIP_Z)
				dst[nout].z = v2->z + vrl_ScalarMultDiv(v1->z - v2->z, numer, denom);
			if (flags & VRL_DISPLAY_XYCLIP_INTENSITY)
				dst[nout].intensity = v2->intensity
					+ vrl_ScalarMultDiv(v1->intensity - v2->intensity, numer, denom);
			else
				dst[nout].intensity = v2->intensity;
			++nout;
			}
		else  /* entering */
			{
			vrl_Scalar numer = top - v1->y;
			vrl_Scalar denom = v2->y - v1->y;
			dst[nout].x = v1->x + vrl_ScalarMultDiv(v2->x - v1->x, numer, denom);
			dst[nout].y = top;
			if (flags & VRL_DISPLAY_XYCLIP_Z)
				dst[nout].z = v1->z + vrl_ScalarMultDiv(v2->z - v1->z, numer, denom);
			if (flags & VRL_DISPLAY_XYCLIP_INTENSITY)
				dst[nout].intensity = v1->intensity
					+ vrl_ScalarMultDiv(v2->intensity - v1->intensity, numer, denom);
			else
				dst[nout].intensity = v1->intensity;
			++nout;
			dst[nout++] = src[i];
			}
		v1 = v2;
		}

	if (dst == a1) { dst = a2; src = a1; }
	else { dst = a1; src = a2; }
	npts = nout;
	nout = 0;
	v1 = &src[npts-1];
	for (i = 0; i < npts; ++i)
		{
		v2 = &src[i];
		if (v1->y <= bottom && v2->y <= bottom)  /* on-screen */
			dst[nout++] = src[i];
		else if (v1->y > bottom && v2->y > bottom)  /* off-screen */
			;
		else if (v1->y <= bottom && v2->y > bottom)   /* leaving */
			{
			vrl_Scalar numer = bottom - v2->y;
			vrl_Scalar denom = v1->y - v2->y;
			dst[nout].x = v2->x + vrl_ScalarMultDiv(v1->x - v2->x, numer, denom);
			dst[nout].y = bottom;
			if (flags & VRL_DISPLAY_XYCLIP_Z)
				dst[nout].z = v2->z + vrl_ScalarMultDiv(v1->z - v2->z, numer, denom);
			if (flags & VRL_DISPLAY_XYCLIP_INTENSITY)
				dst[nout].intensity = v2->intensity
					+ vrl_ScalarMultDiv(v1->intensity - v2->intensity, numer, denom);
			else
				dst[nout].intensity = v2->intensity;
			++nout;
			}
		else  /* entering */
			{
			vrl_Scalar numer = bottom - v1->y;
			vrl_Scalar denom = v2->y - v1->y;
			dst[nout].x = v1->x + vrl_ScalarMultDiv(v2->x - v1->x, numer, denom);
			dst[nout].y = bottom;
			if (flags & VRL_DISPLAY_XYCLIP_Z)
				dst[nout].z = v1->z + vrl_ScalarMultDiv(v2->z - v1->z, numer, denom);
			if (flags & VRL_DISPLAY_XYCLIP_INTENSITY)
				dst[nout].intensity = v1->intensity
					+ vrl_ScalarMultDiv(v2->intensity - v1->intensity, numer, denom);
			else
				dst[nout].intensity = v1->intensity;
			++nout;
			dst[nout++] = src[i];
			}
		v1 = v2;
		}
{
int i;
for (i = 0; i < nout; ++i)
	{
	dst[i].next = &dst[i+1];
	dst[i].prev = &dst[i-1];
	}
if (nout)
	{
	dst[nout-1].next = &dst[0];
	dst[0].prev = &dst[nout-1];
	}
}
	return nout ? dst : NULL;
	}

