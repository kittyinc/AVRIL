/* Read a WLD file; part of AVRIL */
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
#include <stdlib.h>   /* atof(), strtoul() */
#include <string.h>
#include <ctype.h>

#ifdef __TURBOC__
#include <alloc.h>   /* for coreleft() */
#endif

/* Start of globals shared with application */

char *wld_options = "";       /* set by "options" statement */

/* These all "accumlate" as world files are read */

char wld_title[20][80];       /* title to display at startup */
int wld_titlerows = 0;        /* number of rows in title */

/* End of globals shared with application */

/* List management routines */

typedef struct _vrl_listnode vrl_List;

struct _vrl_listnode
	{
	char *name;
	void *value;
	vrl_List *next;
	};

static vrl_List *named_surfaces = NULL;      /* list of named surface definitions */
static vrl_List *named_surfacemaps = NULL;   /* list of named surface maps */

static void *vrl_AddToList(vrl_List **list, char *name, void *value)
	{
	vrl_List *p = vrl_malloc(sizeof(vrl_List));
	if (p)
		{
		p->next = *list;
		*list = p;
		p->name = strdup(name);
		p->value = value;
		}
	return p;
	}

static void *vrl_FindOnList(vrl_List *list, char *name)
	{
	while (list)
		if (!stricmp(name, list->name))
			return list->value;
		else
			list = list->next;
	return NULL;
	}

/* end of list management routines */

static vrl_Surfacemap *currmap = NULL;  /* points to current map */
static int currmap_maxentries = 0;      /* number of entries allocated in current map */

static vrl_Surface default_surface = { VRL_SURF_SIMPLE, 15, 128 };

#define DEFAULT_SURFACEMAP_ENTRIES 10  /* if number of entries not given in surfacemap statement */

static vrl_Scalar default_hither = float2scalar(10);
static vrl_Scalar default_yon = float2scalar(1000000000L);

/* scanning and parsing */

static char loadpath[100];  /* initialized to a null string */

void vrl_FileSetLoadpath(char *path)
	{
	if (path)
		strcpy(loadpath, path);
	else
		loadpath[0] = '\0';
	}

char *vrl_FileFixupFilename(char *filename)
	{
	static char fixup_filename[100];
	if (*filename == '\0' || *filename == '\\' || *filename == '/' || loadpath[0] == '\0')
		return filename;
	sprintf(fixup_filename, "%s\\%s", loadpath, filename);
	return fixup_filename;
	}

static int getline(char *buff, int maxbytes, FILE *in)
	{
	char *p;
	if (fgets(buff, maxbytes, in) == NULL) return 0;
	if ((p = strchr(buff, '\n')) != NULL) *p = '\0';
	return 1;
	}

static int lookup(char *token, char *table[], int n)
	{
	int i;
	for (i = 0; i < n; ++i)
		if (!stricmp(token, table[i]))
			return i;
	return -1;
	}

static int parse(char *buff, char *seps, char *argv[])
	{
	char *p;
	int argc = 0;
	if ((p = strchr(buff, '#')) != NULL) *p = '\0';
	p = strtok(buff, seps);
	while (p)
		{
		argv[argc++] = p;
		p = strtok(NULL, seps);
		}
	return argc;
	}

static char *statement_table[] =
	{
	"object", "polyobj", "polyobj2", "camera", "hither", "yon",
	"light", "mlight", "ambient", "include", "loadpath",
	"anglestep", "spacestep", "stepsize", "flymode",
	"screencolor", "screenclear", "skycolor", "groundcolor", "palette",
	"surfacedef", "surfacemap", "surface", "usemap",
	"attach", "detach", "attachview", "position", "rotate", "segment",
	"figure", "options", "title", "worldscale", "version"
	};

static enum statements
	{
	st_object, st_polyobj, st_polyobj2, st_camera, st_hither, st_yon,
	st_light, st_mlight, st_ambient, st_include, st_loadpath,
	st_anglestep, st_spacestep, st_stepsize, st_flymode,
	st_screencolor, st_screenclear, st_skycolor, st_groundcolor, st_palette,
	st_surfacedef, st_surfacemap, st_surface, st_usemap,
	st_attach, st_detach, st_attachview, st_position, st_rotate, st_segment,
	st_figure, st_options, st_title, st_worldscale, st_version,
	ncmds
	};

static int create_object(int argc, char *argv[])
	{
	char *name = NULL, *filename, *p;
	char fname[100];
	char *parent = NULL;
	float sx = 1, sy = 1, sz = 1;
	vrl_Angle rx = 0, ry = 0, rz = 0;
	vrl_Scalar tx = 0, ty = 0, tz = 0;
	vrl_Shape *shape;
	vrl_Surfacemap *map = NULL;
	FILE *in;
	switch (argc)
		{
		default: /* ignore extra arguments */
		case 14: parent = argv[13];
		case 13: map = vrl_FindOnList(named_surfacemaps, argv[12]);
		case 12: /* we ignore the depthtype field for now; should use it */
		case 11: tz = float2scalar(atof(argv[10]));
		case 10: ty = float2scalar(atof(argv[9]));
		case 9: tx = float2scalar(atof(argv[8]));
		case 8: rz = float2angle(atof(argv[7]));
		case 7: ry = float2angle(atof(argv[6]));
		case 6: rx = float2angle(atof(argv[5]));
		case 5: sz = atof(argv[4]);
		case 4: sy = atof(argv[3]);
		case 3: sx = atof(argv[2]);
		case 2: name = argv[1]; break;
		case 1: case 0: return -2;
		}
	filename = strchr(name, '=');
	if (filename)
		*filename++ = '\0';
	else
		{
		filename = name;
		name = NULL;
		}
	vrl_SetReadPLGscale(sx, sy, sz);
	vrl_SetReadPLGoffset(0, 0, 0);
	if ((p = strchr(filename, '.')) != NULL)
		*p = '\0';
	strcpy(fname, vrl_FileFixupFilename(filename));
	strcat(fname, ".plg");
	in = fopen(fname, "r");
	if (in == NULL) return -3;
	while ((shape = vrl_ReadPLG(in)) != NULL)
		{
		vrl_Object *obj;
		obj = vrl_ObjectCreate(shape);
		if (obj == NULL)
			{
			fclose(in);
			return -1;
			}
		/* if a map was specified, use it */
		if (map)
			vrl_ObjectSetSurfacemap(obj, map);
		/* otherwise, use the shape's default map if there is one */
		else if (vrl_ShapeGetSurfacemap(shape))
			vrl_ObjectSetSurfacemap(obj, vrl_ShapeGetSurfacemap(shape));
		/* otherwise, use the "current" map */
		else
			vrl_ObjectSetSurfacemap(obj, currmap);
		if (parent)
			{
			if (!stricmp(parent, "fixed"))
				{
#ifdef SUPPORT_FIXED
				vrl_MatrixCopy(obj->globalmat, obj->localmat);
				vrl_ObjectMakeFixed(obj);
#endif
				}
			else
				vrl_ObjectAttach(obj, vrl_WorldFindObject(parent));
			}
		vrl_ObjectRotY(obj, ry);
		vrl_ObjectRotX(obj, rx);
		vrl_ObjectRotZ(obj, rz);
		vrl_ObjectMove(obj, tx, ty, tz);
		if (name) vrl_ObjectSetName(obj, strdup(name));
		}
	fclose(in);
	return 0;
	}

static int create_polyobj(int argc, char *argv[], int nsides)
	{
	vrl_Shape *shape = vrl_ShapeCreate();
	vrl_Object *obj = vrl_ObjectCreate(shape);
	vrl_Rep *rep = vrl_RepCreate(atoi(argv[1]), 0);
	vrl_Facet *facet1 = vrl_FacetCreate(atoi(argv[1])), *facet2 = NULL;
	char *p;
	int i;
	if (obj == NULL || shape == NULL || rep == NULL || facet1 == NULL) return -1;
	if (nsides > 1) facet2 = vrl_FacetCreate(atoi(argv[1]));  /* if it fails, it fails */
	vrl_ShapeAddRep(shape, rep, 0);
	vrl_RepSetSorting(rep, VRL_SORT_NONE);
	vrl_RepAddFacet(rep, facet1);
	vrl_ShapeSetSurfacemap(shape, vrl_SurfacemapCreate(nsides));
	if (vrl_ShapeGetSurfacemap(shape) == NULL) return -1;
	vrl_ObjectSetSurfacemap(obj, vrl_ShapeGetSurfacemap(shape));
	if (isdigit(*argv[2]))
		{
		vrl_Surface *s = vrl_SurfaceCreate(0);
		if (s)
			vrl_SurfaceFromDesc(strtoul(argv[2], NULL, 0), s);  /* convert value to surface */
		vrl_SurfacemapSetSurface(vrl_ShapeGetSurfacemap(shape), 0, s);
		}
	else
		vrl_SurfacemapSetSurface(vrl_ShapeGetSurfacemap(shape), 0, vrl_FindOnList(named_surfaces, argv[2]));
	if (nsides > 1) { --argc; ++argv; }  /* shift to allow for second color */
	if (facet2)
		{
		vrl_FacetSetSurfnum(facet2, 1);
		if (isdigit(*argv[2]))
			{
			vrl_Surface *s = vrl_SurfaceCreate(0);
			if (s)
				vrl_SurfaceFromDesc(strtoul(argv[2], NULL, 0), s);  /* convert value to surface */
			vrl_SurfacemapSetSurface(vrl_ObjectGetSurfacemap(obj), 1, s);
			}
		else
			vrl_SurfacemapSetSurface(vrl_ObjectGetSurfacemap(obj), 1, vrl_FindOnList(named_surfaces, argv[2]));
		vrl_RepAddFacet(rep, facet2);
		}
	for (i = 0; i < vrl_RepCountVertices(rep); ++i)
		{
		rep->vertices[i][X] = float2scalar(atof(argv[3 + i * 3]));
		rep->vertices[i][Y] = float2scalar(atof(argv[3 + i * 3 + 1]));
		rep->vertices[i][Z] = float2scalar(atof(argv[3 + i * 3 + 2]));
		vrl_FacetSetPoint(facet1, i, i);
		if (facet2) vrl_FacetSetPoint(facet2, i, vrl_RepCountVertices(rep)-1 - i);
		}
	vrl_FacetComputeNormal(facet1, rep->vertices);
	if (facet2) vrl_FacetComputeNormal(facet2, rep->vertices);
	vrl_ShapeComputeBounds(shape);
	return 0;
	}

static int create_figure(int argc, char *argv[])
	{
	char *name = NULL, *filename;
	char fname[100];
	char *parent = NULL;
	float sx = 1, sy = 1, sz = 1;
	vrl_Angle rx = 0, ry = 0, rz = 0;
	vrl_Scalar tx = 0, ty = 0, tz = 0;
	vrl_Object *obj, *par = NULL;
	FILE *in;
	switch (argc)
		{
		default: /* ignore extra arguments */
		case 12: parent = argv[11];
		case 11: tz = float2scalar(atof(argv[10]));
		case 10: ty = float2scalar(atof(argv[9]));
		case 9: tx = float2scalar(atof(argv[8]));
		case 8: rz = float2angle(atof(argv[7]));
		case 7: ry = float2angle(atof(argv[6]));
		case 6: rx = float2angle(atof(argv[5]));
		case 5: sz = atof(argv[4]);
		case 4: sy = atof(argv[3]);
		case 3: sx = atof(argv[2]);
		case 2: name = argv[1]; break;
		case 1: case 0: return -2;
		}
	filename = strchr(name, '=');
	if (filename)
		*filename++ = '\0';
	else
		{
		filename = name;
		name = NULL;
		}
	if (parent)
		par = vrl_WorldFindObject(parent);
	vrl_SetReadFIGscale(sx, sy, sz);
	strcpy(fname, vrl_FileFixupFilename(filename));
	strcat(fname, ".fig");
	in = fopen(fname, "r");
	if (in == NULL) return -3;
	obj = vrl_ReadFIG(in, par, name);
	fclose(in);
	vrl_ObjectMove(obj, tx, ty, tz);
	vrl_ObjectRotY(obj, ry);  vrl_ObjectRotX(obj, rx);  vrl_ObjectRotZ(obj, rz);
	return 0;
	}

static int create_light(int argc, char *argv[])
	{
	char *name = NULL;
	vrl_Scalar x = 0, y = 0, z = 0;
	int spot = 0;
	vrl_Object *obj;
	vrl_Light *light = vrl_LightCreate();
	if (light == NULL) return -1;
	obj = vrl_ObjectCreate(NULL);
	if (obj == NULL)
		{
		vrl_WorldRemoveLight(light);
		vrl_free(light);
		return -1;
		}
	vrl_LightAssociate(light, obj);
	switch (argc)
		{
		default:  /* ignore extra arguments */
		case 7: name = argv[6];
		case 6: vrl_LightSetIntensity(light, float2factor(atof(argv[5])/128));
		case 5:
			if (!stricmp(argv[4], "spot")) spot = 1;
			else spot = atoi(argv[4]);
		case 4: z = float2scalar(atof(argv[3]));
		case 3: y = float2scalar(atof(argv[2]));
		case 2: x = float2scalar(atof(argv[1]));
			break;
		case 1: case 0: return -2;
		}
	if (name)
		{
		char *attachment = strchr(name, '=');
		if (attachment)
			{
			vrl_Object *parent;
			*attachment++ = '\0';
			parent = vrl_WorldFindObject(attachment);
			if (parent)
				vrl_ObjectAttach(obj, parent);
			}
		vrl_LightSetName(light, strdup(name));
		vrl_WorldAddLight(light);
		}
	if (spot)
		{
		vrl_Vector tmp, tmp2, tmp3;
		vrl_MatrixIdentity(obj->localmat);
		vrl_VectorCreate(tmp, x, y, z);
		if (vrl_VectorMagnitude(tmp) == 0)
			return -3;
		vrl_VectorNormalize(tmp);
		/* Z column is our normalized view vector */
		obj->localmat[X][Z] = tmp[X];
		obj->localmat[Y][Z] = tmp[Y];
		obj->localmat[Z][Z] = tmp[Z];
		/* Now we find a vector perpendicular to the view vector */
		if (x < 0) x = -x;  /* find absolute values of x and */
		if (y < 0) y = -y;  /* y (works in both floating and fixed) */
		if (x > y)  /* if X greater than Y, swap X and Z */
			vrl_VectorCreate(tmp2, tmp[Z], tmp[Y], -tmp[X]);
		else   /* otherwise, swap Y and Z */
			vrl_VectorCreate(tmp2, tmp[X], tmp[Z], -tmp[Y]);
		/* tmp2 is now a vector perpendicular to tmp, and like tmp is normalized */
		/* make it the Y column */
		obj->localmat[X][Y] = tmp2[X];
		obj->localmat[Y][Y] = tmp2[Y];
		obj->localmat[Z][Y] = tmp2[Z];
		/* the X column is just the cross product of the other two */
		vrl_VectorCrossproduct(tmp3, tmp2, tmp);
		obj->localmat[X][X] = tmp3[X];
		obj->localmat[Y][X] = tmp3[Y];
		obj->localmat[Z][X] = tmp3[Z];
		obj->moved = 1;
		vrl_LightSetType(light, VRL_LIGHT_DIRECTIONAL);
		}
	else
		{
		vrl_LightSetType(light, VRL_LIGHT_POINTSOURCE);
		vrl_ObjectMove(obj, x, y, z);
		}
	return 0;
	}

static vrl_Camera *most_recent_camera = NULL;

static int create_camera(int argc, char *argv[])
	{
	vrl_Scalar x = 0, y = 0, z = 0, hither = default_hither, yon = default_yon;
	vrl_Angle tilt = 0, pan = 0, roll = 0;
	float zoom = 4.0;
	char *name;
	vrl_Object *obj;
	vrl_Camera *camera = vrl_CameraCreate();
	if (camera == NULL) return -1;
	obj = vrl_ObjectCreate(NULL);
	if (obj == NULL)
		{
		vrl_WorldRemoveCamera(camera);
		vrl_free(camera);
		return -1;
		}
	vrl_CameraAssociate(camera, obj);
	switch (argc)
		{
		default:  /* ignore extra arguments */
		case 11: yon = float2scalar(atof(argv[10]));
		case 10: hither = float2scalar(atof(argv[9]));
		case 9: zoom = atof(argv[8]);
		case 8: roll = float2angle(atof(argv[7]));
		case 7: pan = float2angle(atof(argv[6]));
		case 6: tilt = float2angle(atof(argv[5]));
		case 5: z = float2scalar(atof(argv[4]));
		case 4: y = float2scalar(atof(argv[3]));
		case 3: x = float2scalar(atof(argv[2]));
		case 2: name = argv[1];
			break;
		case 1: case 0: return -2;
		}
	vrl_CameraSetHither(camera, hither);
	vrl_CameraSetYon(camera, yon);
	vrl_CameraSetZoom(camera, zoom);
	vrl_CameraSetName(camera, strdup(name));
	vrl_WorldAddCamera(camera);
	vrl_ObjectRotX(obj, tilt);
	vrl_ObjectRotY(obj, pan);
	vrl_ObjectRotZ(obj, roll);
	vrl_ObjectMove(obj, x, y, z);
	most_recent_camera = camera;
	if (vrl_WorldGetCamera() == NULL)
		vrl_WorldSetCamera(camera);
	return 0;
	}

static int create_segment(int argc, char *argv[])
	{
	char *name = NULL;
	vrl_Angle rx = 0, ry = 0, rz = 0;
	vrl_Scalar tx = 0, ty = 0, tz = 0;
	vrl_Object *obj = vrl_ObjectCreate(NULL);
	if (obj == NULL) return -1;
	switch (argc)
		{
		default: /* ignore extra arguments */
		case 8: tz = float2scalar(atof(argv[7]));
		case 7: ty = float2scalar(atof(argv[6]));
		case 6: tx = float2scalar(atof(argv[5]));
		case 5: rz = float2angle(atof(argv[4]));
		case 4: ry = float2angle(atof(argv[3]));
		case 3: rx = float2angle(atof(argv[2]));
		case 2: name = argv[1];
			break;
		case 1: case 0: return -2;
		}
	if (name)
		{
		char *attachment = strchr(name, '=');
		if (attachment)
			{
			vrl_Object *parent;
			*attachment++ = '\0';
			parent = vrl_WorldFindObject(attachment);
			vrl_ObjectAttach(obj, parent);
			}
		vrl_ObjectSetName(obj, strdup(name));
		}
	vrl_ObjectRotY(obj, ry);
	vrl_ObjectRotX(obj, rx);
	vrl_ObjectRotZ(obj, rz);
	vrl_ObjectMove(obj, tx, ty, tz);
	return 0;
	}

int vrl_ReadWLDProcessLine(char *buff)
	{
	char buffcopy[256];
	int argc;
	char *argv[20];
	strcpy(buffcopy, buff);  /* unparsed version for "title" statement */
	argc = parse(buff, " \t,", argv);
	if (argc < 1) return 0;  /* ignore blank lines */
	if (argc < 2) return 0;  /* for now, all statements have at least one argument */
	switch (lookup(argv[0], statement_table, ncmds))
		{
		case st_object: create_object(argc, argv); break;
		case st_polyobj: create_polyobj(argc, argv, 1); break;
		case st_polyobj2: create_polyobj(argc, argv, 2); break;
		case st_segment: create_segment(argc, argv); break;
		case st_figure: create_figure(argc, argv); break;
		case st_light:  /* fall through... same as mlight */
		case st_mlight: create_light(argc, argv); break;
		case st_camera: create_camera(argc, argv); break;
		case st_ambient: vrl_WorldSetAmbient(float2factor(atof(argv[1])/128.0)); break;
		case st_worldscale: vrl_WorldSetScale(float2scalar(atof(argv[1]))); break;
		case st_loadpath: strcpy(loadpath, argv[1]); break;
		case st_anglestep: vrl_WorldSetTurnstep(float2angle(atof(argv[1]))); break;
		case st_stepsize: case st_spacestep: vrl_WorldSetMovestep(float2scalar(atof(argv[1]))); break;
		case st_flymode: vrl_WorldSetMovementMode(strtoul(argv[1], NULL, 0) ? 1 : 0); break;
		case st_screenclear: vrl_WorldSetScreenClear(strtoul(argv[1], NULL, 0) ? 1 : 0); break;
		case st_screencolor:  /* fall through... same as skycolor */
		case st_skycolor: vrl_WorldSetSkyColor(strtoul(argv[1], NULL, 0)); break;
		case st_groundcolor: vrl_WorldSetGroundColor(strtoul(argv[1], NULL, 0)); break;
		case st_options:
			{
			char *p;
			wld_options = strdup(argv[1]);
			for (p = wld_options; *p; ++p)
				switch (*p)
					{
					case 'h': vrl_WorldSetHorizon(0); break;
					case 'H': vrl_WorldSetHorizon(1); break;
					default: break;
					}
			}
			break;
		case st_yon:
			{
			vrl_Scalar y = float2scalar(atof(argv[1]));
			if (most_recent_camera)
				vrl_CameraSetYon(most_recent_camera, y);
			default_yon = y;
			}
			break;
		case st_attach:
			{
			vrl_Object *obj = vrl_WorldFindObject(argv[1]);
			vrl_Object *parent = vrl_WorldFindObject(argv[2]);
			if (obj && parent)
				vrl_ObjectAttach(obj, parent);
			}
			break;
		case st_detach:
			{
			vrl_Object *obj = vrl_WorldFindObject(argv[1]);
			if (obj)
				vrl_ObjectDetach(obj);
			}
			break;
		case st_position:
			{
			vrl_Scalar x, y, z;
			vrl_Object *obj = vrl_WorldFindObject(argv[1]);
			if (obj == NULL) break;
			x = y = z = 0;
			if (argc > 2) x = float2scalar(atof(argv[2]));
			if (argc > 3) y = float2scalar(atof(argv[3]));
			if (argc > 4) z = float2scalar(atof(argv[4]));
			vrl_ObjectMove(obj, x, y, z);
			}
			break;
		case st_rotate:
			{
			vrl_Angle rx, ry, rz;
			vrl_Object *obj = vrl_WorldFindObject(argv[1]);
			if (obj == NULL) break;
			rx = ry = rz = 0;
			if (argc > 2) rx = float2angle(atof(argv[2]));
			if (argc > 3) ry = float2angle(atof(argv[3]));
			if (argc > 4) rz = float2angle(atof(argv[4]));
			vrl_ObjectRotReset(obj);
			vrl_ObjectRotY(obj, ry);
			vrl_ObjectRotX(obj, rx);
			vrl_ObjectRotZ(obj, rz);
			}
			break;
		case st_attachview:
			{
			vrl_Object *parent = vrl_WorldFindObject(argv[1]);
			if (parent && most_recent_camera)
				if (!vrl_ObjectIsFixed(parent))
					vrl_ObjectAttach(vrl_CameraGetObject(most_recent_camera), parent);
			}
			break;
		case st_title:
			if (stricmp(argv[1], "memory"))
				strncpy(wld_title[wld_titlerows++], buffcopy, sizeof(wld_title[0])-1);
#ifdef __TURBOC__
/* coreleft() is Borland-specific */
			else
				sprintf(wld_title[wld_titlerows++], "%ld bytes left", (long) coreleft());
#endif
			break;
		case st_palette:
			{
			vrl_Palette *pal = vrl_WorldGetPalette();
			if (pal)
				{
				FILE *palfile = fopen(vrl_FileFixupFilename(argv[1]), "rb");
				if (palfile)
					{
					vrl_PaletteRead(palfile, pal);
					fclose(palfile);
					}
				}
			}
			break;
		case st_include:
			{
			FILE *new = fopen(vrl_FileFixupFilename(argv[1]), "r");
			if (new)
				{
				vrl_ReadWLD(new);
				fclose(new);
				}
			}
			break;
		case st_surfacedef:
			{
			vrl_Surface *surf = vrl_malloc(sizeof(vrl_Surface));
			if (surf == NULL) break;
			vrl_SurfaceInit(surf);
			vrl_SurfaceFromDesc(strtoul(argv[2], NULL, 0), surf);
			vrl_AddToList(&named_surfaces, argv[1], surf);
			}
			break;
		case st_surfacemap:
			{
			int i;
			if (argc > 2) currmap_maxentries = atoi(argv[2]);
			else currmap_maxentries = DEFAULT_SURFACEMAP_ENTRIES;
			currmap = vrl_SurfacemapCreate(currmap_maxentries);
			if (currmap == NULL) break;
			vrl_AddToList(&named_surfacemaps, argv[1], currmap);
			for (i = 0; i < currmap_maxentries; ++i)
				vrl_SurfacemapSetSurface(currmap, i, &default_surface);
			}
			break;
		case st_usemap:
			currmap = vrl_FindOnList(named_surfacemaps, argv[1]);
			break;
		case st_surface:
			{
			int n = atoi(argv[1]);
			if (n >= currmap_maxentries) break;
			if (currmap == NULL) break;
			if (argc < 3) break;
			vrl_SurfacemapSetSurface(currmap, n, vrl_FindOnList(named_surfaces, argv[2]));
			if (vrl_SurfacemapGetSurface(currmap, n) == NULL)
				vrl_SurfacemapSetSurface(currmap, n, &default_surface);
			}
			break;
		st_version: break;
		default:
			vrl_ReadWLDfeature(argc, argv, buffcopy);
			break;
		}
	return 0;
	}

int vrl_ReadWLD(FILE *in)
	{
	char buff[256];
	vrl_Camera *cam;
	while (getline(buff, sizeof(buff), in))
		vrl_ReadWLDProcessLine(buff);
	cam = vrl_WorldFindCamera("1");
	if (cam)
		vrl_WorldSetCamera(cam);
	return 0;
	}

int vrl_LoadWLDfile(char *filename)
	{
	FILE *in = fopen(vrl_FileFixupFilename(filename), "r");
	int retvalue;
	if (in == NULL)
		return NULL;
	retvalue = vrl_ReadWLD(in);
	fclose(in);
	return retvalue;
	}

