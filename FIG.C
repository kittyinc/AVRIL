/* Read a FIG file into a tree of objects; part of AVRIL */
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
#include <ctype.h>    /* isspace() */
#include <string.h>

int figload_err = 0;

static char *figload_errmsgs[] = {
	"No error",
	"Couldn't open plg file",
	"Error reading plg file",
	"Bad syntax",
	NULL };

#define match(a, b) (!strnicmp((a), (b), strlen(b)))

static vrl_Object **part_array = NULL;
static int part_array_size = 0;

void vrl_SetReadFIGpartArray(vrl_Object **ptr, int maxparts)
	{
	part_array = ptr;
	part_array_size = maxparts;
	}

static float xscale = 1, yscale = 1, zscale = 1;

void vrl_SetReadFIGscale(float x, float y, float z)
	{
	xscale = x;  yscale = y;  zscale = z;
	}

static int process_attribute(char *buff, vrl_Object *obj, char *rootname)
	{
	while (isspace(*buff)) ++buff;
	if (match(buff, "plgfile"))
		{
		char filename[100];
		FILE *in;
		float sx = 1, sy = 1, sz = 1;
		long tx = 0, ty = 0, tz = 0;
		vrl_Shape *shape;
		sscanf(buff, "plgfile = %s scale %f,%f,%f shift %ld,%ld,%ld", filename, &sx, &sy, &sz, &tx, &ty, &tz);
		vrl_SetReadPLGscale(sx*xscale, sy*yscale, sz*zscale);
		vrl_SetReadPLGoffset(tx*xscale, ty*yscale, tz*zscale);
		if ((in = fopen(vrl_FileFixupFilename(filename), "r")) == NULL)
			return figload_err = -1;
		if ((shape = vrl_ReadPLG(in)) == NULL)
			{
			fclose(in);
			return figload_err = -2;
			}
		fclose(in);
		vrl_ObjectSetShape(obj, shape);
		return figload_err = 0;
		}
	else if (match(buff, "pos"))
		{
		long tx, ty, tz;
		sscanf(buff, "pos = %ld,%ld,%ld", &tx, &ty, &tz);
		vrl_ObjectMove(obj, float2scalar(tx*xscale), float2scalar(ty*yscale), float2scalar(tz*zscale));
		}
	else if (match(buff, "rot"))
		{
		float rx, ry, rz;
		sscanf(buff, "rot = %f,%f,%f", &rx, &ry, &rz);
		vrl_ObjectRotY(obj, float2angle(ry));
		vrl_ObjectRotX(obj, float2angle(rx));
		vrl_ObjectRotZ(obj, float2angle(rz));
		}
	else if (match(buff, "name"))
		{
		char *p, partname[100];
		if ((p = strchr(buff, '=')) == NULL)
			{
			figload_err = -3;
			return -3;
			}
		do ++p;
		while (isspace(*p));
		if (rootname)
			{
			sprintf(partname, "%s.%s", rootname, p);
			vrl_ObjectSetName(obj, strdup(partname));
			}
		}
	else if (match(buff, "segnum"))
		{
		int n;
		sscanf(buff, "segnum = %d", &n);
		if (part_array && n < part_array_size) part_array[n] = obj;
		}
	else if (match(buff, "partnum"))
		{
		int n;
		sscanf(buff, "partnum = %d", &n);
		if (part_array && n < part_array_size) part_array[n] = obj;
		}
	/* ignore anything we don't understand */
	return 0;
	}

vrl_Object *vrl_ReadFIG(FILE *in, vrl_Object *parent, char *rootname)
	{
	char buff[256];
	int c, i = 0;
	vrl_Object *obj = vrl_ObjectCreate(NULL);
	if (obj == NULL) return NULL;
	if (parent) vrl_ObjectAttach(obj, parent);
	while ((c = getc(in)) != EOF)
		{
		switch (c) {
			case '#':   /* ignore comments */
				while ((c = getc(in)) != EOF)
					if (c == '\n')
						break;
				break;
			case '{':
				vrl_ReadFIG(in, obj, rootname);
				break;
			case '}':
				return obj;
			case ';':
				buff[i] = '\0';
				process_attribute(buff, obj, rootname);
				i = 0;
				break;
			default:
				if (i < sizeof(buff)-1) buff[i++] = c;
				break;
			}
		}
	return obj;
	}

vrl_Object *vrl_LoadFIGfile(char *filename)
	{
	FILE *in = fopen(vrl_FileFixupFilename(filename), "r");
	vrl_Object *obj;
	if (in == NULL)
		return NULL;
	obj = vrl_ReadFIG(in, NULL, NULL);
	fclose(in);
	return obj;
	}

