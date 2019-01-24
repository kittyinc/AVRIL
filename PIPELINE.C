/* Rendering pipeline for AVRIL */
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
/* First released: April, 1994 (pre-release) */
/* Release 1.0: August, 1994 */
/* Release 1.1: September, 1994 */

#ifdef COMMENTS

   Things to Do:
  
   Phase XI -- new features
     Go to protected mode
       Watcom?
       DJGPP?
     Windows port
     VDF
     Networking (Winsock)
     Fix object rotation and translation coordinate frames
     Pivots [object.c, vdf.c, pipeline.c?]
     Fix Gouraud shading bugs (palette wrap)
     [release 2.1?]
     Z-buffering (not in Borland, due to memory limitations)
       support for USE_ZBUFFER
     16-bit and 24-bit color (HiColor and Truecolor)
       support for HAS_PALETTE
     Texture mapping [plg.c, pipeline.c, xyclip.c, display modules]
     More stereo types
       SIRDS
       Field sequential anaglyph
       Field sequential using shutter glasses (separate pages)
     Stereoscopic distant-object sharing?
     Sound
     vertex selection (as for objects and facets)
     Point source lighting
     Scaling?
        keep per-object copies of scaled versions of vertices?
        fancy inverse function that can handle scaling?
     Re-write horizon routine [pipeline.c]
        horizon routine doesn't work unless RenderBegin has been called once
        eliminate bugs (and weird 0.85 stuff)
        make more efficient
        add "steps"
        horizon should respect screen window; so should PCX
        horizon should work with stereo
  
   RELEASE 2.5
     (Watcom, DJGPP?, Z-buffering, texture mapping,
     16- and 24-bit color, point source lighting,
     VDF, sound, more stereo modes, pivots, scaling,
     point source lighting, working horizon)

   Phase XII -- profile and optimize
     Align rmalloc'ed structures
     Should check ocodes in z_clip() and no_zclip() and return 1
     Backface testing should keep D around, not do vector subtraction
     See which optimizations are worthwhile and eliminate those that aren't:
       register variables
       vops.h
     Look for bottlenecks and fix them; reduce number of per-vertex operations
     Smarter xy clipping (use ocodes, doubly-linked lists)
     Avoid doing extra multiplies per vertex when scaling to screen space
     Cache lighting
     Cache outvertices
     Pre-calculated slopes
     Pre-calculated Gouraud gradients?
     Coarse subdivision? (using stuff in rendspec.txt)
     Check world-coordinate-system rotation
  
   Phase XIII -- better math
     Think about computing 1/divisor and doing a multiply, instead of two divs
     Integer square root (or Euler equation?)
     Faster magnitude and normal computations
     Point light sources
  
   Phase XIV - internal cleanup
     Ortho mode [pipeline.c]
     Fixed objects [pipeline.c, object.c, wld.c]
     Eliminate direct structure access in wld.c and possibly plg.c and fig.c
     "should"s
  
   Phase XV -- Establish portability
     Port display routines to Windows, WinG
     Port to Unix (Sun, Dec, Alpha?), X-Windows
     Port to Linux
  
   Release 3.0
     (features: increased speed, ortho mode, horizon steps, sound, VDF,
     pivots?, scaling?, portability)
  
   Phase XVI -- Sorting [pipeline.c, and plg.c or vdf.c]
     Sort front-to-back for certain types of display drivers (GetSortorder())
     Optimize so that things need not be processed if hidden
     Contained objects
       switch to inside and outside facet lists (for non-BSP objects)
       add support for contained objects and inside facets
     Better object sorting
       sort objects more simply, by farthest plane?
       eliminate objarray[] (?)
       object sorting dependency information
       smart sorting of objects (including object dependency)
       clean up object-sorting code
     Better facet sorting
       support sorting by average poly depth
       bounds on facets (x and y bounds from z-clipping?)
       smart sorting of facets
       detail facets
       BSP trees within objects
  
   RELEASE 3.5
     (features: more increased speed, smarter sorting)
  
   Phase XVII -- big changes (grad students?  others?)
     built-in high-level language (BOB?  Tcl?  Forth?)
     more sophisticated user interface (SUIT?)
     collision detection (Proxima?)
  
   RELEASE 4.0 -- Final!  (built-in lang, networking, userint, collision detection)

#endif

/* Copyright 1994 by Bernie Roehl */

/* You may use this code for your own non-commercial projects without
   paying any fees or royalties.  "Non-commercial", in this context,
   means that the software you write is given away for free to anyone
   who wants it.
   
   Commercial use, including shareware, requires a licensing
   fee and a specific written agreement with the author.

   All programs created using this software (both commercial and
   non-commercial) must acknowledge the use of the AVRIL library,
   both in the documentation and in a banner screen at the start or
   end of the program.

   For more information, contact Bernie Roehl (broehl@uwaterloo.ca).

*/

#include "avril.h"
#include <stdlib.h>
#include <string.h>  /* memset() */
#include <dos.h>     /* MK_FP() and FP_SEG() */

#define TRANS 3

#include "vops.h"

#include "vrlstats.h"

static vrl_Statistics Stats;

vrl_Statistics *vrl_RenderGetStats(void)
	{
	return &Stats;
	}

void vrl_StatsClear(void)
	{
	Stats.objects_processed = 0;
	Stats.objects_not_invisible = 0;
	Stats.objects_not_behind = 0;
	Stats.objects_not_leftright = 0;
	Stats.objects_not_abovebelow = 0;
	Stats.objects_not_toosmall = 0;
	Stats.lights_processed = 0;
	Stats.facets_processed = 0;
	Stats.facets_not_backfacing = 0;
	Stats.facets_flatshaded = 0;
	Stats.vertices_zchecked = 0;
	Stats.vertices_ztransformed = 0;
	Stats.facets_not_behind = 0;
	Stats.vertices_xychecked = 0;
	Stats.vertices_xytransformed = 0;
	Stats.vertices_projected_regular = 0;
	Stats.facets_gouraud_shaded = 0;
	Stats.vertex_pointlights = 0;
	Stats.vertex_dirlights = 0;
	Stats.detail_facets = 0;
	Stats.not_toomanyfacets = 0;
	Stats.facets_need_x_clipping = 0;
	Stats.facets_need_y_clipping = 0;
	Stats.facets_need_z_clipping = 0;
	}

static vrl_RenderStatus status;

#ifdef NEW_PROC
/* this next variable should be set non-zero to use the new processing
   functions at and below new_process().
 */

int vrl_process_direct = 0;
extern void new_process_begin_scene(void);
extern void new_process_end_scene(void);
extern void new_process_object(vrl_Object *obj, vrl_Rep *rep);
extern void new_process_facet(vrl_Facet *f);
#endif

typedef struct _vrl_outobject vrl_OutputObject;

struct _vrl_outobject
	{
	vrl_Object *original;          /* points back at the object we came from */
	vrl_OutputFacet **outfacets;   /* array of output facets */
	int noutfacets;                /* number of output facets */
	int not_drawn;                 /* set if object hasn't been drawn yet */
	};

static vrl_OutputObject **objarray = NULL;  /* array of object info (e.g. depth) */
static int maxobjs = 0;              /* maximum number of entries in the array */
static int nobjs;                    /* number of entries actually in the array */

static vrl_OutputFacet **outfacets = NULL;  /* array of outfacets for sorting */
static int maxfacets = 0;            /* maximum number of entries in outfacets[] */
static int noutfacets = 0;           /* number of entries currently in outfacets[] */

/* DEBUGGING ONLY: */
int pause = 0;       /* if set, pause before rendering each object */
int show_bb = 0;     /* if set, show the bounding box around objects */
vrl_Color bbox_color = 4;  /* bounding boxes in red */
/* END OF DEBUGGING VARIABLES */

/* Static globals, for speed: */

static vrl_Scalar hither;    /* current camera's near clipping plane distance */
static vrl_Scalar yon;       /* current camera's far clipping plane distance */
static unsigned char ortho;  /* non-zero if we're orthoscopic */
static vrl_Scalar orthodiv;  /* divisor (instead of v[Z]) for orthoscopic viewing */
static vrl_Matrix wtv;       /* current camera's world-to-view matrix */
static vrl_Matrix otv;       /* object-to-view matrix */
static vrl_ScreenCoord screenscale_x, screenscale_y;  /* view-to-screen scale factors */
static vrl_ScreenCoord window_halfwidth, window_halfheight;   /* offset into window of center of window */
static vrl_Factor aright, cright, btop, ctop;     /* coefficients of clipping planes */
static float aspect;

/* these next four variables are only used to see if we can skip xy clipping */
static vrl_ScreenCoord left_clip, right_clip, top_clip, bottom_clip;

/* These flags are set to non-zero if current object needs clipping */

static unsigned char need_x_clipping;
static unsigned char need_y_clipping;
static unsigned char need_z_clipping;

/* Static globals to keep track of display capabilities; these are updated
   by vrl_RenderBegin(), in case the information changed since the last
   frame */

static int display_clips = 0;
static int display_can_gouraud = 0;

#define gouraud_shading(surf) (((surf)->type == VRL_SURF_GOURAUD || (surf)->type == VRL_SURF_SPECULAR) && display_can_gouraud && !wireframe)

/* Miscellaneous globals */

static vrl_Vector vp;        /* viewpoint in current object's coordinates */
static vrl_Vector viewdir;   /* the direction we're looking in */

typedef struct {
	vrl_Vector vertex;                    /* transformed vertex */
	vrl_ScreenCoord projected_x, projected_y;  /* projected X and Y values */
	vrl_Factor intensity;                 /* for Gouraud shading */
	enum { UNPROCESSED = 0, Z_DONE, XY_DONE, PROJECTED } status;
	} Tempvertex;
	
Tempvertex *tmpverts = NULL;

static int maxvertices = 0;              /* maximum number of entries in above four arrays */

static vrl_OutputObject *currobj;        /* current output object */
static vrl_Surfacemap *current_surfmap;  /* pointer to current object's surface map */

static vrl_Vector *vertarray;           /* points to current rep's vertices */
static vrl_Vector *normarray;           /* points to current rep's normals */

/* there should be functions to set these next two: */
static int object_sorting = 1;
static int facet_sorting = 1;

static vrl_ScreenCoord horizontal_shift = 0;

void vrl_RenderSetHorizontalShift(int n)
	{
	horizontal_shift = ((vrl_32bit) n) << VRL_SCREEN_FRACT_BITS;
	}

/********************************
 *  Highlighting and Wireframe  *
 ********************************/

static int highlight_facets = 0;   /* non-zero if current object is highlighted */
static vrl_Color highlight_color = 255;
static vrl_Color wireframe_color = 1;
static int wireframe = 0;

void vrl_RenderSetDrawMode(int mode)
	{
	wireframe = mode;
	}

int vrl_RenderGetDrawMode(void)
	{
	return wireframe;
	}

void vrl_RenderSetWireframeColor(vrl_Color color)
	{
	wireframe_color = color;
	}

vrl_Color vrl_RenderGetWireframeColor(void)
	{
	return wireframe_color;
	}

void vrl_RenderSetHighlightColor(vrl_Color color)
	{
	highlight_color = color;
	}

vrl_Color vrl_RenderGetHighlightColor(void)
	{
	return highlight_color;
	}

/********************************
 *       Layer support          *
 ********************************/

static unsigned char layerbits[32] = { 0xFF };    /* all layers visible initially */

static unsigned char bitmasks[] =
	{ 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

void vrl_LayerOn(int n)
	{
	layerbits[n >> 3] |= bitmasks[n & 7];
	}

void vrl_LayerOff(int n)
	{
	layerbits[n >> 3] &= ~bitmasks[n & 7];
	}

void vrl_LayerToggle(int n)
	{
	layerbits[n >> 3] ^= bitmasks[n & 7];
	}

vrl_Boolean vrl_LayerIsOn(int n)
	{
	return layerbits[n >> 3] & bitmasks[n & 7];
	}

void vrl_LayerAllOn(void)
	{
	memset(layerbits, 0xFFFF, sizeof(layerbits));
	}

void vrl_LayerAllOff(void)
	{
	memset(layerbits, 0x0000, sizeof(layerbits));
	}


/********************************
 *  Memory allocation routines  * 
 ********************************/

static unsigned char *our_pool = NULL, *our_highwater = NULL;
static unsigned int memspace = 0, memused = 0;

static int rinit(unsigned int maxmem)
	{
	our_pool = (unsigned char *) vrl_malloc(memspace = maxmem);
	if (our_pool == NULL) return -1;
	our_highwater = our_pool;
	memused = 0;
	return 0;
	}

static void rquit(void)
	{
	if (our_pool)
		vrl_free(our_pool);
	memspace = 0;
	}

static void rclear(void)
	{
	our_highwater = our_pool;
	memused = 0;
	}

/* note that n can be zero (returns pointer to next malloc's return value) */

static void *rmalloc(int n)
	{
	void *p = our_highwater;
	if (memused + n > memspace)
		{
		status.memory = 1;
		return NULL;
		}
	memused += n;
	our_highwater = &our_pool[memused];
	return p;
	}

static void *rmark(void)
	{
	return our_highwater;
	}

static void rrelease(void *ptr)
	{
	our_highwater = ptr;
	}

/********************************
 *          Z clipper           *
 ********************************/

/* stores output vertex in array for processing by display routines */

static vrl_OutputVertex *firstpoint = NULL, *lastpoint = NULL;

static int addvertex(vrl_ScreenCoord x, vrl_ScreenCoord y, vrl_ScreenCoord z, vrl_Factor intensity, unsigned char ocode, vrl_OutputFacet *of, int ind)
	{
	register vrl_OutputVertex *p = rmalloc(sizeof(vrl_OutputVertex));
	if (p == NULL) return -1;
	p->next = of->points;  /* last vertex always points to first */
	of->points->prev = p;  /* and first always points to last */
	if (lastpoint)
		{
		p->prev = lastpoint;  /* our previous is the current last */
		lastpoint->next = p;  /* last's next is us */
		}
	else
		{
		p->prev = of->points;  /* initially, points to first one */
		lastpoint = p;         /* and we're the last one */
		}
	lastpoint = p;   /* we're now the last one */
	/* convert to window coordinates */
	x += window_halfwidth;  y += window_halfheight;
	p->x = x;
	p->y = y; 
	p->z = z;
	p->intensity = intensity;
	if (ind == 0)
		{
		of->minbound[X] = of->maxbound[X] = x;
		of->minbound[Y] = of->maxbound[Y] = y;
		of->minbound[Z] = of->maxbound[Z] = z;
		}
	else
		{
		if (x < of->minbound[X]) of->minbound[X] = x;
		if (y < of->minbound[Y]) of->minbound[Y] = y;
		if (z < of->minbound[Z]) of->minbound[Z] = z;
		if (x > of->maxbound[X]) of->maxbound[X] = x;
		if (y > of->maxbound[Y]) of->maxbound[Y] = y;
		if (z > of->maxbound[Z]) of->maxbound[Z] = z;
		}
	p->outcode = ocode;
	return 0;
	}

/* zclip: does z clipping, computes perspective */
/* Produces clipped vertices, and */
/* returns 0 normally */
/* returns 1 if facet is entirely clipped in X or Y */
/* returns 2 if no XY clipping is needed */

/* Uses outcodes; here are the bit values: */

#define IS_ABOVE  0x01
#define IS_BELOW  0x02
#define IS_LEFT   0x04
#define IS_RIGHT  0x08

static int zclip(vrl_Facet *facet, vrl_OutputFacet *of)
	{
	vrl_Scalar x, y, t;  /* temporaries */
	int i;
	int thisone, prevone = facet->points[facet->npoints - 1];
	unsigned char ocode;
	unsigned char ocode_or = 0;
	unsigned char ocode_and = (IS_ABOVE|IS_BELOW|IS_LEFT|IS_RIGHT);
	++Stats.facets_need_z_clipping;
	for (i = 0; i < facet->npoints; ++i)
		{
		thisone = facet->points[i];
		if (tmpverts[prevone].vertex[Z] >= hither && tmpverts[thisone].vertex[Z] >= hither)  /* edge on-screen */
			{
			if (tmpverts[thisone].status < PROJECTED)
				{
				if (ortho)
					{
					tmpverts[thisone].projected_x = vrl_ScalarMultDiv(tmpverts[thisone].vertex[X], screenscale_x, orthodiv);
					tmpverts[thisone].projected_x += horizontal_shift;
					tmpverts[thisone].projected_y = vrl_ScalarMultDiv(tmpverts[thisone].vertex[Y], screenscale_y, orthodiv);
					}
				else
					{
					tmpverts[thisone].projected_x = vrl_ScalarMultDiv(tmpverts[thisone].vertex[X], screenscale_x, tmpverts[thisone].vertex[Z]);
					tmpverts[thisone].projected_x += horizontal_shift;
					tmpverts[thisone].projected_y = vrl_ScalarMultDiv(tmpverts[thisone].vertex[Y], screenscale_y, tmpverts[thisone].vertex[Z]);
					}
				tmpverts[thisone].status = PROJECTED;
				}
			if (!display_clips && (need_x_clipping || need_y_clipping))
				{
				ocode = 0;
				if (tmpverts[thisone].projected_x < left_clip) ocode |= IS_LEFT;
				else if (tmpverts[thisone].projected_x > right_clip) ocode |= IS_RIGHT;
				if (tmpverts[thisone].projected_y < bottom_clip) ocode |= IS_BELOW;
				else if (tmpverts[thisone].projected_y > top_clip) ocode |= IS_ABOVE;
				ocode_or |= ocode;
				ocode_and &= ocode;
				}
			if (addvertex(tmpverts[thisone].projected_x, tmpverts[thisone].projected_y, tmpverts[thisone].vertex[Z], tmpverts[thisone].intensity, ocode, of, i))
				return 1;
			}
		else if (tmpverts[prevone].vertex[Z] < hither && tmpverts[thisone].vertex[Z] < hither)  /* edge off-screen */
			;
		else if (tmpverts[prevone].vertex[Z] >= hither && tmpverts[thisone].vertex[Z] < hither)  /* leaving */
			{
			vrl_Factor intensity;
			vrl_Scalar numer = hither - tmpverts[thisone].vertex[Z];
			vrl_Scalar denom = tmpverts[prevone].vertex[Z] - tmpverts[thisone].vertex[Z];
			x = tmpverts[thisone].vertex[X]
				+ vrl_ScalarMultDiv(tmpverts[prevone].vertex[X] - tmpverts[thisone].vertex[X], numer, denom);
			y = tmpverts[thisone].vertex[Y]
				+ vrl_ScalarMultDiv(tmpverts[prevone].vertex[Y] - tmpverts[thisone].vertex[Y], numer, denom);
			if (gouraud_shading(of->surface))
				intensity = tmpverts[thisone].intensity
					+ vrl_ScalarMultDiv(tmpverts[prevone].intensity - tmpverts[thisone].intensity, numer, denom);
			else
				intensity = of->intensity;  /* otherwise, use facet's intensity */
			if (ortho)
				{
				x = vrl_ScalarMultDiv(x, screenscale_x, orthodiv); 
				x += horizontal_shift;
				y = vrl_ScalarMultDiv(y, screenscale_y, orthodiv); 
				}
			else
				{
				x = vrl_ScalarMultDiv(x, screenscale_x, hither); 
				x += horizontal_shift;
				y = vrl_ScalarMultDiv(y, screenscale_y, hither); 
				}
			if (!display_clips && (need_x_clipping || need_y_clipping))
				{
				ocode = 0;
				if (x < left_clip) ocode |= IS_LEFT;
				else if (x > right_clip) ocode |= IS_RIGHT;
				if (y < bottom_clip) ocode |= IS_BELOW;
				else if (y > top_clip) ocode |= IS_ABOVE;
				ocode_or |= ocode;
				ocode_and &= ocode;
				}
			if (addvertex(x, y, ortho ? orthodiv : hither, intensity, ocode, of, i))
				return 1;
			}
		else /* if (tmpverts[prevone].vertex[Z] < hither && tmpverts[thisone].vertex[Z] >= hither)  /* entering */
			{
			vrl_Factor intensity;
			vrl_Scalar numer = hither - tmpverts[prevone].vertex[Z];
			vrl_Scalar denom = tmpverts[thisone].vertex[Z] - tmpverts[prevone].vertex[Z];
			x = tmpverts[prevone].vertex[X]
				+ vrl_ScalarMultDiv(tmpverts[thisone].vertex[X] - tmpverts[prevone].vertex[X], numer, denom);
			y = tmpverts[prevone].vertex[Y]
				+ vrl_ScalarMultDiv(tmpverts[thisone].vertex[Y] - tmpverts[prevone].vertex[Y], numer, denom);
			if (gouraud_shading(of->surface))
				intensity = tmpverts[prevone].intensity
					+ vrl_ScalarMultDiv(tmpverts[thisone].intensity - tmpverts[prevone].intensity, numer, denom);
			else
				intensity = of->intensity;  /* otherwise, use facet's intensity */
			if (ortho)
				{
				x = vrl_ScalarMultDiv(x, screenscale_x, orthodiv); 
				x += horizontal_shift;
				y = vrl_ScalarMultDiv(y, screenscale_y, orthodiv); 
				}
			else
				{
				x = vrl_ScalarMultDiv(x, screenscale_x, hither); 
				x += horizontal_shift;
				y = vrl_ScalarMultDiv(y, screenscale_y, hither); 
				}
			if (!display_clips && (need_x_clipping || need_y_clipping))
				{
				ocode = 0;
				if (x < left_clip) ocode |= IS_LEFT;
				else if (x > right_clip) ocode |= IS_RIGHT;
				if (y < bottom_clip) ocode |= IS_BELOW;
				else if (y > top_clip) ocode |= IS_ABOVE;
				ocode_or |= ocode;
				ocode_and &= ocode;
				}
			if (addvertex(x, y, ortho ? orthodiv : hither, intensity, ocode, of, i))
				return 1;
			if (tmpverts[thisone].status < PROJECTED)
				{
				if (ortho)
					{
					tmpverts[thisone].projected_x = vrl_ScalarMultDiv(tmpverts[thisone].vertex[X], screenscale_x, orthodiv);
					tmpverts[thisone].projected_x += horizontal_shift;
					tmpverts[thisone].projected_y = vrl_ScalarMultDiv(tmpverts[thisone].vertex[Y], screenscale_y, orthodiv);
					}
				else
					{
					tmpverts[thisone].projected_x = vrl_ScalarMultDiv(tmpverts[thisone].vertex[X], screenscale_x, tmpverts[thisone].vertex[Z]);
					tmpverts[thisone].projected_x += horizontal_shift;
					tmpverts[thisone].projected_y = vrl_ScalarMultDiv(tmpverts[thisone].vertex[Y], screenscale_y, tmpverts[thisone].vertex[Z]);
					}
				tmpverts[thisone].status = PROJECTED;
				}
			if (!display_clips && (need_x_clipping || need_y_clipping))
				{
				ocode = 0;
				if (tmpverts[thisone].projected_x < left_clip) ocode |= IS_LEFT;
				else if (tmpverts[thisone].projected_x > right_clip) ocode |= IS_RIGHT;
				if (tmpverts[thisone].projected_y < bottom_clip) ocode |= IS_BELOW;
				else if (tmpverts[thisone].projected_y > top_clip) ocode |= IS_ABOVE;
				ocode_or |= ocode;
				ocode_and &= ocode;
				}
			if (addvertex(tmpverts[thisone].projected_x, tmpverts[thisone].projected_y, tmpverts[thisone].vertex[Z], tmpverts[thisone].intensity, ocode, of, i))
				return 1;
			}
		prevone = thisone;
		}
	if (ocode_or == 0) return 2;  /* needs no xy clipping */
#ifdef ENTIRELY_CLIPPED
	if (ocode_and) return 1;      /* entirely clipped */
#endif
	return 0;
	}

/* this next routine gets called instead of zclip() if we know that
   no z-clipping is needed */

static int no_zclip(vrl_Facet *facet, vrl_OutputFacet *of)
	{
	int i;
	unsigned char ocode;
	unsigned char ocode_or = 0;
	unsigned char ocode_and = (IS_ABOVE|IS_BELOW|IS_LEFT|IS_RIGHT);
	for (i = 0; i < facet->npoints; ++i)
		{
		int thisone = facet->points[i];
		if (tmpverts[thisone].status < PROJECTED)
			{
			if (ortho)
				{
				tmpverts[thisone].projected_x = vrl_ScalarMultDiv(tmpverts[thisone].vertex[X], screenscale_x, orthodiv);
				tmpverts[thisone].projected_x += horizontal_shift;
				tmpverts[thisone].projected_y = vrl_ScalarMultDiv(tmpverts[thisone].vertex[Y], screenscale_y, orthodiv);
				}
			else
				{
				tmpverts[thisone].projected_x = vrl_ScalarMultDiv(tmpverts[thisone].vertex[X], screenscale_x, tmpverts[thisone].vertex[Z]);
				tmpverts[thisone].projected_x += horizontal_shift;
				tmpverts[thisone].projected_y = vrl_ScalarMultDiv(tmpverts[thisone].vertex[Y], screenscale_y, tmpverts[thisone].vertex[Z]);
				}
			tmpverts[thisone].status = PROJECTED;
			}
		if (!display_clips && (need_x_clipping || need_y_clipping))
			{
			ocode = 0;
			if (tmpverts[thisone].projected_x < left_clip) ocode |= IS_LEFT;
			else if (tmpverts[thisone].projected_x > right_clip) ocode |= IS_RIGHT;
			if (tmpverts[thisone].projected_y < bottom_clip) ocode |= IS_BELOW;
			else if (tmpverts[thisone].projected_y > top_clip) ocode |= IS_ABOVE;
			ocode_or |= ocode;
			ocode_and &= ocode;
			}
		if (addvertex(tmpverts[thisone].projected_x, tmpverts[thisone].projected_y, tmpverts[thisone].vertex[Z], tmpverts[thisone].intensity, ocode, of, i))
			return 1;
		}
	if (ocode_or == 0) return 2;  /* needs no xy clipping */
#ifdef ENTIRELY_CLIPPED
	if (ocode_and) return 1;      /* entirely clipped */
#endif
	return 0;
	}

/********************************
 *     Lighting Calculations    *
 ********************************/

static vrl_Light *lightlist = NULL;    /* point to (world) list of lights */
int nlights = 0;                       /* number of lights in the scene */

typedef struct
	{
	vrl_LightingType type;
	vrl_Factor intensity;
	vrl_Matrix mat;	  /* transforms from light space into object space */
	} LocalLight;

static LocalLight* locallights = NULL;
static int maxlights = 0;

static vrl_Factor ambient = float2factor(0.5);

void vrl_RenderSetAmbient(vrl_Factor amb)
	{
	ambient = amb;
	}

static vrl_Factor compute_lighting(vrl_Vector normal, vrl_Vector vertex)
	{
	vrl_Factor intensity = 0;
	vrl_Factor lightscale = vrl_ScalarDivide(1, nlights);
	/* lightscale is the amount by which to scale the intensity of
	   each source, so that the total doesn't exceed VRL_UNITY. */
	vrl_Vector tmpvec;
	int i;
	for (i = 0; i < nlights; ++i)
		{
		LocalLight *lgt = &locallights[i];
		vrl_Factor inten = vrl_FactorMultiply(lightscale, lgt->intensity);
		/* inten is the fraction of the total maximum intensity that this
		   light is producing */
		switch (lgt->type)
			{
			case VRL_LIGHT_AMBIENT:
				break;
#ifdef WORKING
			case VRL_LIGHT_POINTSOURCE:
				/* find vector from light to this vertex */
				vrl_VectorSub(tmpvec, lgt->mat[TRANS], vertex);
				vrl_VectorNormalize(tmpvec);
				/* scale the intensity of the light by the angle to the source */
				inten = vrl_FactorMultiply(inten, -vrl_VectorDotproduct(tmpvec, normal));
				break;
#else
			case VRL_LIGHT_POINTSOURCE:   /* for now, treat them all as directional */
#endif
			case VRL_LIGHT_DIRECTIONAL:
				tmpvec[X] = lgt->mat[X][Z];
				tmpvec[Y] = lgt->mat[Y][Z];
				tmpvec[Z] = lgt->mat[Z][Z];
				/* scale the intensity of the light by the angle to the direction of the light */
				inten = vrl_FactorMultiply(inten, -vrl_VectorDotproduct(tmpvec, normal));
				break;
			default:   /* unknown light type */
				inten = 0;
				break;
			}
		if (inten > VRL_UNITY)
			inten = VRL_UNITY;  /* clip */
		if (inten > 0)
			intensity += inten;
		}
	if (intensity < 0)
		intensity = 0;
	else if (intensity > VRL_UNITY)
		intensity = VRL_UNITY;
	return intensity;
	}

static char *copyright[] =
	{
	"AVRIL -- copyright (c) 1995 by Bernie Roehl.  All rights reserved.",
	"This version not intended for commercial use (2/20/95)."
	};

/********************************
 *       Backface Removal       *
 ********************************/

static int backfacing(vrl_Facet *facet)  /* return non-zero if facet is backfacing */
	{
	vrl_Vector v;
	register vrl_Scalar result;  /* kludge to get around bug in Borland C++ 3.00 */
	if (facet->npoints < 3) return 0;  /* points or lines */
	if (ortho && 0)
		result = vrl_VectorDotproduct(vp, facet->normal);
	else
		{
		vrl_VectorSub(v, vp, vertarray[facet->points[0]]);
		result = vrl_VectorDotproduct(v, facet->normal);
		}
	return result < 0;
	}

/********************************
 *       Facet Processing       *
 ********************************/

/* parent will be passed as non-NULL if we're a detail facet */

static void process_facet(vrl_Facet *facet, vrl_OutputFacet *parent)
	{
	vrl_OutputFacet *of;   /* output facet */
	vrl_Facet *detail;     /* points to details on current facet */
	vrl_Factor intensity;  /* total amount of light hitting this facet */
	int i;
	vrl_Surface *surf = vrl_SurfacemapGetSurface(current_surfmap, facet->surface);
	++Stats.facets_not_backfacing;
	/* handle lighting calculations for flat shading */
	if (!wireframe && (surf->type == VRL_SURF_FLAT || surf->type == VRL_SURF_METAL || surf->type == VRL_SURF_GLASS))
		{
		if (parent)
			intensity = parent->intensity;
		else
			intensity = compute_lighting(facet->normal, vertarray[facet->points[0]]);
		++Stats.facets_flatshaded;
		}
	if (!need_z_clipping)
		{
		++Stats.facets_not_behind;
		for (i = 0; i < facet->npoints; ++i)
			{
			int vertnum = facet->points[i];
			register Tempvertex *tv = &tmpverts[vertnum];
			++Stats.vertices_zchecked;  ++Stats.vertices_xychecked;
			if (tv->status < XY_DONE)  /* not already z-transformed */
				{
				vrl_Transform(tv->vertex, otv, vertarray[vertnum]);
				tv->status = XY_DONE;
				++Stats.vertices_ztransformed;
				++Stats.vertices_xytransformed;
				}
			if (normarray && gouraud_shading(surf))
				{
				tv->intensity = compute_lighting(normarray[vertnum], tv->vertex);
				++Stats.facets_gouraud_shaded;
				}
			else
				tv->intensity = intensity;  /* use facet's intensity */
			}
		}
	else
		{
		/* transform Z components of vertices, keeping track of maxdepth */
		vrl_Scalar maxdepth = hither - 1;  /* if nothing closer than this, skip this facet */
		for (i = 0; i < facet->npoints; ++i)
			{
			vrl_Scalar z;
			int vertnum = facet->points[i];
			register Tempvertex *tv = &tmpverts[vertnum];
			++Stats.vertices_zchecked;
			if (tv->status < Z_DONE)  /* not already z-transformed */
				{
				tv->vertex[Z] = vrl_TransformZ(otv, vertarray[vertnum]);
				tv->status = Z_DONE;
				++Stats.vertices_ztransformed;
				}
			if (tv->vertex[Z] > maxdepth) maxdepth = tv->vertex[Z];
			}
		if (maxdepth < hither) return;  /* entirely behind us */
		++Stats.facets_not_behind;
		/* transform X and Y components of the vertices */
		for (i = 0; i < facet->npoints; ++i)
			{
			int vertnum = facet->points[i];
			register Tempvertex *tv = &tmpverts[vertnum];
			++Stats.vertices_xychecked;
			if (tv->status >= XY_DONE) continue;  /* already XY-transformed */
			tv->vertex[X] = vrl_TransformX(otv, vertarray[vertnum]);
			tv->vertex[Y] = vrl_TransformY(otv, vertarray[vertnum]);
			tv->status = XY_DONE;
			++Stats.vertices_xytransformed;
			if (normarray && gouraud_shading(surf))
				{
				tv->intensity = compute_lighting(normarray[vertnum], tv->vertex);
				++Stats.facets_gouraud_shaded;
				}
			else
				tv->intensity = intensity;  /* use facet's intensity */
			}
		}
	/* create a new output facet */
	of = rmalloc(sizeof(vrl_OutputFacet));
	if (of == NULL) return;  /* out of memory */
	of->xclip = need_x_clipping ? VRL_DISPLAY_XYCLIP_CLIP_X : 0;
	of->yclip = need_y_clipping ? VRL_DISPLAY_XYCLIP_CLIP_Y : 0;
	if (of->xclip) ++Stats.facets_need_x_clipping;
	if (of->yclip) ++Stats.facets_need_y_clipping;
	of->highlight = (facet->highlight | highlight_facets) ? 1 : 0;
	of->surface = surf;         /* surface descriptor */
	of->intensity = intensity;
	of->details = NULL;         /* no detail facets (yet) */
	of->original = facet;       /* point back at original facet */
	of->outobj = currobj;       /* point to this output facet's output object */
	firstpoint = rmalloc(0);
	of->points = firstpoint;    /* points to vertices */
	if (firstpoint == NULL) return;
	lastpoint = NULL;
	if (need_z_clipping)
		switch (zclip(facet, of))
			{
			case 2: of->xclip = of->yclip = 0;  /* no xy clipping needed */
			case 0: break;   /* needs xy clipping */
			case 1: return;  /* entirely xy clipped */
			}
	else
		switch (no_zclip(facet, of))
			{
			case 2: of->xclip = of->yclip = 0;  /* no xy clipping needed */
			case 0: break;   /* needs xy clipping */
			case 1: return;  /* entirely xy clipped */
			}
	if (parent)  /* we're a detail facet; attach us to our new parent */
		{
		of->details = parent->details;
		parent->details = of;
		return;  /* details don't have details */
		}		
	else  /* we need to be put into the outfacets[] array */
		{
		if (noutfacets >= maxfacets)
			{
			status.facets = 1;
			return;  /* table full */
			}
		outfacets[noutfacets++] = of;
		++Stats.not_toomanyfacets;
		++currobj->noutfacets;
		}
	/* now process our detail facets; should save this for "showtime" */
	for (detail = facet->details; detail; detail = detail->farside)
		{
		process_facet(detail, of);
		++Stats.detail_facets;
		}
	}

/********************************
 *        Facet Sorting         *
 ********************************/

/* This next routine should be re-coded to be non-recursive, for speed */

static void bsp_process_facet(vrl_Facet *facet)
	{
	if (backfacing(facet))
		{
		if (facet->nearside) bsp_process_facet(facet->nearside);
		if (facet->farside) bsp_process_facet(facet->farside);
		}
	else
		{
		if (facet->farside) bsp_process_facet(facet->farside);
		process_facet(facet, NULL);
		if (facet->nearside) bsp_process_facet(facet->nearside);
		}
	}

static int facet_sort_nearest(const void *a, const void *b)
	{
#define f1 (*((vrl_OutputFacet **) a))
#define f2 (*((vrl_OutputFacet **) b))
	if (f1->minbound[Z] < f2->minbound[Z]) return  1;
	if (f1->minbound[Z] > f2->minbound[Z]) return -1;
	return 0;
	}

static int facet_sort_farthest(const void *a, const void *b)
	{
#define f1 (*((vrl_OutputFacet **) a))
#define f2 (*((vrl_OutputFacet **) b))
	if (f1->maxbound[Z] < f2->maxbound[Z]) return  1;
	if (f1->maxbound[Z] > f2->maxbound[Z]) return -1;
	return 0;
	}

/********************************
 *      Object Processing       *
 ********************************/

static void process_object(vrl_Object *obj)
	{
	vrl_Matrix vto, wto;
	vrl_Vector v;
	vrl_Rep *rep;
	vrl_Light *lgt;
	register int i;
	++Stats.objects_processed;
#ifdef NEW_PROC
	if (nobjs >= maxobjs && !vrl_process_direct)
#else
	if (nobjs >= maxobjs)
#endif
		{
		status.objects = 1;
		return;
		}
	if (obj->invisible) return;
	if (obj->layer)
		if ((layerbits[obj->layer >> 3] & bitmasks[obj->layer & 7]) == 0)
			return;
	if (obj->shape == NULL) return;
	++Stats.objects_not_invisible;
	/* set up a pointer to our array of pointers to surfaces */
	current_surfmap = obj->surfmap;
	if (current_surfmap == NULL)
		current_surfmap = obj->shape->default_surfacemap;
	if (current_surfmap == NULL)
		return;
	/* compute object-to-view matrix */
	if (vrl_ObjectIsFixed(obj))
		vrl_MatrixCopy(otv, wtv);
	else
		vrl_MatrixMultiply(otv, wtv, obj->globalmat);
	/* cull objects behind us or too far in front of us */
	v[Z] = vrl_TransformZ(otv, obj->shape->center);
	if (v[Z] + obj->shape->radius < hither) return;
	if (v[Z] - obj->shape->radius > yon) return;
	need_z_clipping = 1;
	if (v[Z] - obj->shape->radius > hither) need_z_clipping = 0;
	++Stats.objects_not_behind;
	need_x_clipping = need_y_clipping = 1;  /* need both x and y */
	v[X] = vrl_TransformX(otv, obj->shape->center);
/*	v[X] = vrl_TransformX(otv, obj->shape->center);  ???? */
	if (ortho && 0)
		{  /* simpler box check */
		if (v[X] - obj->shape->radius > cright) return;
		if (v[X] + obj->shape->radius < -cright) return;
		if (v[X] - obj->shape->radius > -cright
			&& v[X] + obj->shape->radius < cright)
			need_x_clipping = 0;  /* don't need to clip X */
		}
	else
		{
		/* this next bit's tricky; we're substituting the object center into
		   the plane equation for the clipping planes in order to find the
		   distance of the point to the plane, since we know that if that
		   distance is greater than the radius of the bounding sphere then
		   the object can be culled
		 */
		if (vrl_FactorMultiply(aright, v[X]) + vrl_FactorMultiply(cright, v[Z]) > obj->shape->radius) return;
		if (vrl_FactorMultiply(-aright, v[X]) + vrl_FactorMultiply(cright, v[Z]) > obj->shape->radius) return;
		/* now we reverse the test to see if the object is entirely *inside*
		   the clipping volume; if it is, we don't need to do facet clipping
		   later on.
		 */
		if (vrl_FactorMultiply(-aright, v[X]) + vrl_FactorMultiply(-cright, v[Z]) > obj->shape->radius
			&& vrl_FactorMultiply(aright, v[X]) + vrl_FactorMultiply(-cright, v[Z]) > obj->shape->radius)
			need_x_clipping = 0;  /* don't need to clip X */
		}
	++Stats.objects_not_leftright;
	v[Y] = vrl_TransformY(otv, obj->shape->center);
	if (ortho && 0)
		{  /* simpler box check */
		if (v[Y] - obj->shape->radius > ctop) return;
		if (v[Y] + obj->shape->radius < -ctop) return;
		if (v[Y] - obj->shape->radius > -ctop
			&& v[Y] + obj->shape->radius < ctop)
			need_y_clipping = 0;  /* don't need to clip Y */
		}
	else
		{
		if (vrl_FactorMultiply(btop, v[Y]) + vrl_FactorMultiply(ctop, v[Z]) > obj->shape->radius) return;
		if (vrl_FactorMultiply(-btop, v[Y]) + vrl_FactorMultiply(ctop, v[Z]) > obj->shape->radius) return;
		if (vrl_FactorMultiply(-btop, v[Y]) + vrl_FactorMultiply(-ctop, v[Z]) > obj->shape->radius
			&& vrl_FactorMultiply(btop, v[Y]) + vrl_FactorMultiply(-ctop, v[Z]) > obj->shape->radius)
			need_y_clipping = 0;  /* don't need to clip Y */
		}
	++Stats.objects_not_abovebelow;
	/* select a representation (i.e., level of detail) */
	if (obj->forced_rep)  /* if there's a forced rep, use it */
		rep = obj->forced_rep;
	else if (v[Z] <= 0)   /* the object is *very* close; use first rep */
		rep = obj->shape->replist;
	else   /* search for a suitable rep */
		{
		vrl_Scalar r = vrl_ScalarMultDiv(obj->shape->radius, max(screenscale_x, screenscale_y) >> VRL_SCREEN_FRACT_BITS, v[Z]);
		rep = vrl_ShapeGetRep(obj->shape, r);
		}
	if (rep == NULL)
		return;
	++Stats.objects_not_toosmall;
#ifdef NEW_PROC
	if (!vrl_process_direct)
		{
#endif
		/* allocate a new output object */
		currobj = rmalloc(sizeof(vrl_OutputObject));
		if (currobj == NULL) return;
		objarray[nobjs++] = currobj;  /* add to array */
		currobj->original = obj;
		currobj->outfacets = &outfacets[noutfacets];
		currobj->noutfacets = 0;
		currobj->not_drawn = 1;
		/* move bounding box into world coordinates */
#ifdef OLD
		if (!vrl_ObjectIsFixed(obj))
			{
			vrl_VectorAdd(currobj->minbound, obj->shape->minbound, obj->globalmat[TRANS]);
			vrl_VectorAdd(currobj->maxbound, obj->shape->maxbound, obj->globalmat[TRANS]);
			}
#endif
#ifdef NEW_PROC
		}
#endif
	/* if the object is highlighted, all its facets must be as well */
	highlight_facets = obj->highlight ? 1 : 0;
	/* find the view-to-object transform and the view in object coordinates */
	vrl_MatrixInverse(vto, otv);
	/* a little magic here... we want to transform the view vector
	[0,0,1] into object space, but that just means selecting the third
	column of vto and adding the translations; since the third column
	of vto is the third row of otv, it's easy, and saves a transform! */
	if (ortho && 0)
		{
		vrl_VectorAdd(vp, otv[2], vto[3]);   /* only used by backfacing() */
		}
	else
		{
		vrl_VectorCopy(vp, vto[3]);  /* only used by backfacing() */
		}
	/* initialize vertarray[], normarray[] */
	vertarray = rep->vertices;
	normarray = rep->normals;
	for (i = 0; i < rep->nvertices; ++i)
		{
		tmpverts[i].intensity = 0;
		tmpverts[i].status = UNPROCESSED;
		}
	/* Find the world-to-object transform and use it to compute the position
	   of each light source in object space */
	if (!vrl_ObjectIsFixed(obj))
		vrl_MatrixInverse(wto, obj->globalmat);
	nlights = 0;
	if (!wireframe)
		{
		for (lgt = lightlist; lgt && nlights < maxlights; lgt = lgt->next)
			{
			if (lgt->on)
				{
				locallights[nlights].type = lgt->type;
				locallights[nlights].intensity = lgt->intensity;
				if (lgt->object == NULL)   /* lights with no parents are ambient by definition */
					locallights[nlights].type = VRL_LIGHT_AMBIENT;
				else if (vrl_ObjectIsFixed(lgt->object))   /* same for lights attached to fixed objects */
					locallights[nlights].type = VRL_LIGHT_AMBIENT;
				/* if we're a fixed object, the light's world position is what we use */
				else if (vrl_ObjectIsFixed(obj))
					vrl_MatrixCopy(locallights[nlights].mat, lgt->object->globalmat);
				else
					vrl_MatrixMultiply(locallights[nlights].mat, wto, lgt->object->globalmat);
				++nlights;
				}
			}
		}
	/* process the facets */
#ifdef NEW_PROC
	if (vrl_process_direct)
		{
		vrl_Facet *f;
		new_process_object(obj, rep);
		for (f = rep->facets; f; f = f->farside)
			new_process_facet(f);
		}
	else
#endif
	if (rep->sorttype == VRL_SORT_OTHER)  /* if they're in a BSP tree, use it */
		bsp_process_facet(rep->facets);
	else  /* otherwise, just copy them over; only farside is used */
		{
		vrl_Facet *f;
		/* this next bit becomes two consecutive for() loops eventually,
		   to support inside and outside facet lists */
		for (f = rep->facets; f; f = f->farside)
			{
			++Stats.facets_processed;
			if (!backfacing(f))
				process_facet(f, NULL);
			}
		if (facet_sorting)
			switch (rep->sorttype)
				{
				case VRL_SORT_NEAREST:
					qsort(currobj->outfacets, currobj->noutfacets, sizeof(vrl_OutputFacet *), facet_sort_nearest);
					break;
				case VRL_SORT_FARTHEST:
					qsort(currobj->outfacets, currobj->noutfacets, sizeof(vrl_OutputFacet *), facet_sort_farthest);
					break;
				case VRL_SORT_AVERAGE:
				case VRL_SORT_NONE:
				case VRL_SORT_OTHER:
				default: break;
				}
		}
	}

/********************************
 *      Encrypted Signature     *
 ********************************/

static char m_m[] = { 'x', 'Y', 'z' };

static unsigned char m_0[] =
	{ 0xC1, 0xD6, 0xD2, 0xC9, 0xCC, 0xA0, 0xB2, 0xAE, 0xB0, 0xA0, 0xAD, 0xAD, 0xA0, 0xE3, 0xEF, 0xF0, 0xF9, 0xF2, 0xE9, 0xE7, 0xE8, 0xF4, 0xA0, 0xA8, 0xE3, 0xA9, 0xA0, 0xB1, 0xB9, 0xB9, 0xB5, 0xA0, 0xE2, 0xF9, 0xA0, 0xC2, 0xE5, 0xF2, 0xEE, 0xE9, 0xE5, 0xA0, 0xD2, 0xEF, 0xE5, 0xE8, 0xEC, 0xAE, 0xA0, 0xA0, 0xC1, 0xEC, 0xEC, 0xA0, 0xF2, 0xE9, 0xE7, 0xE8, 0xF4, 0xF3, 0xA0, 0xF2, 0xE5, 0xF3, 0xE5, 0xF2, 0xF6, 0xE5, 0xE4, 0xAE, 0 };

static unsigned char m_1[] =
	{ 0xD4, 0xE8, 0xE9, 0xF3, 0xA0, 0xF6, 0xE5, 0xF2, 0xF3, 0xE9, 0xEF, 0xEE, 0xA0, 0xEE, 0xEF, 0xF4, 0xA0, 0xE9, 0xEE, 0xF4, 0xE5, 0xEE, 0xE4, 0xE5, 0xE4, 0xA0, 0xE6, 0xEF, 0xF2, 0xA0, 0xE3, 0xEF, 0xED, 0xED, 0xE5, 0xF2, 0xE3, 0xE9, 0xE1, 0xEC, 0xA0, 0xF5, 0xF3, 0xE5, 0xA0, 0xA8, 0xB2, 0xAF, 0xB2, 0xB0, 0xAF, 0xB9, 0xB5, 0xA9, 0xAE, 0 };

static unsigned char m_2[] =
	{ 0 };

/********************************
 *       Screen Monitoring      *
 ********************************/

static int monitor_x, monitor_y, monitoring = 0;
static vrl_OutputFacet *monitor_facet;

void vrl_RenderMonitorInit(int x, int y)
	{
	monitoring = 1;
	monitor_x = x;  monitor_y = y;
	monitor_facet = NULL;
	}

vrl_Boolean vrl_RenderMonitorRead(vrl_Object **obj, vrl_Facet **facet, int *vertnum)
	{
	monitoring = 0;
	if (monitor_facet == NULL)
		{
		if (obj) *obj = NULL;
		if (facet) *facet = NULL;
		if (vertnum) *vertnum = NULL;
		return 0;
		}
	if (facet) *facet = monitor_facet->original;
	if (obj) *obj = ((vrl_OutputObject *) (monitor_facet->outobj))->original;
	if (vertnum) *vertnum = 0;   /* should determine which vertex it was */
	return 1;
	}

/* This next routine is based on an algorithm by William Randolph Franklin
   (wrf@ecse.rpi.edu), modified by Scott Anguish (sanguish@digifix.com).
 */

static void monitor(vrl_OutputFacet *of)
	{
	int c = 0;
	vrl_OutputVertex *p = of->points, *q;
	vrl_ScreenCoord xi, xj, yi, yj;
	if (p)
		do
			{
			if (p->next) q = p->next;  else q = of->points;
			xi = q->x >> VRL_SCREEN_FRACT_BITS;
			xj = p->x >> VRL_SCREEN_FRACT_BITS;
			yi = q->y >> VRL_SCREEN_FRACT_BITS;
			yj = p->y >> VRL_SCREEN_FRACT_BITS;
			if (((yi <= monitor_y) && (monitor_y < yj))
				|| ((yj <= monitor_y) && (monitor_y < yi)))
				{
				vrl_32bit r = vrl_ScalarMultDiv((vrl_32bit) xj - xi,
					(vrl_32bit) (monitor_y - yi),
					(vrl_32bit) (yj - yi)) + xi;
				if (monitor_x < r)
					c = !c;
				}
			p = p->next;
			} while (p != of->points);
	if (c)
		monitor_facet = of;
	}

/********************************
 *        Facet output          *
 ********************************/

static int show_facet(vrl_OutputFacet *of)
	{
	vrl_OutputFacet *detail;
	int stat = 0;
	int xyclip = of->xclip | of->yclip;
	vrl_OutputVertex *vertices = of->points, *v;
	
	if (vertices->next == vertices)    /* one vertex => point */
		{
		if (xyclip && !display_clips)
			vertices = vrl_DisplayXYclipPoint(vertices, xyclip);
		if (vertices == NULL) return 0;
		if (of->highlight)
			vertices->red = highlight_color;
		else if (wireframe)
			vertices->red = wireframe_color;
		else
			vertices->red = vrl_DisplayComputeColor(of->surface, vertices->intensity, ambient, vertices->z);
		stat = (*vrl_current_display_driver)(VRL_DISPLAY_POINT, 0, vertices);
		}
	else if (vertices->next->next == vertices)  /* two vertices => line */
		{
		if (xyclip && !display_clips)
			vertices = vrl_DisplayXYclipLine(vertices, xyclip);
		if (vertices == NULL) return 0;
		if (of->highlight)
			vertices->red = vertices->next->red = highlight_color;
		else if (wireframe)
			vertices->red = vertices->next->red = wireframe_color;
		else
			{
			vertices->red = vrl_DisplayComputeColor(of->surface, vertices->intensity, ambient, vertices->z);
			vertices->next->red = vrl_DisplayComputeColor(of->surface, vertices->next->intensity, ambient, vertices->next->z);
			}
		stat = (*vrl_current_display_driver)(VRL_DISPLAY_LINE, 0, vertices);
		}
	else  /* more than two vertices => actual polygon! */
		{
		if (xyclip && !display_clips)
			vertices = vrl_DisplayXYclipPoly(vertices, (gouraud_shading(of->surface) ? VRL_DISPLAY_XYCLIP_INTENSITY : 0) | xyclip);
		if (vertices == NULL) return 0;
		of->points = vertices;
		if (wireframe)
			{
			vrl_16bit ourcolor = of->highlight ? highlight_color : wireframe_color;
			v = vertices;
			do
				{
				v->red = ourcolor;
				v = v->next;
				} while (v != vertices);
			(*vrl_current_display_driver)(VRL_DISPLAY_CLOSED_LINE,
				of->surface->type, vertices);
			}
		else
			{
			if (gouraud_shading(of->surface))
				{
				v = vertices;
				do
					{
					vrl_DisplayComputeVertexColor(v, of->surface, v->intensity, ambient, of->maxbound[Z]);
					v = v->next;
					} while (v != vertices);
				}
			else  /* flat shading */
				of->color = vrl_DisplayComputeColor(of->surface, of->intensity, ambient, of->maxbound[Z]);
			(*vrl_current_display_driver)(VRL_DISPLAY_POLY, 0, of);
			if (of->highlight)
				{
				v = vertices;
				do
					{
					v->red = highlight_color;
					v = v->next;
					} while (v != vertices);
				(*vrl_current_display_driver)(VRL_DISPLAY_CLOSED_LINE,
					of->surface->type, vertices);
				}
			}
		}
	if (monitoring)
		monitor(of);
	for (detail = of->details; detail && !stat; detail = detail->next)
		stat = show_facet(detail);
	return stat;
	}

/********************************
 *        Object output         *
 ********************************/

/* should drop this inline */

static void DrawObj(vrl_OutputObject *obj)
	{
	register int j;
	vrl_OutputFacet **ofs = obj->outfacets;
	if (obj->original->contents)
		{
		void *p;
		int tnf, tno;
		if (!obj->original->invisible)
			{
			/* display inside facets */
			for (j = 0; j < obj->noutfacets; ++j)
				{
				vrl_OutputFacet *of = ofs[j];
				if (of->original->interior)
					show_facet(of);
				}
			}
		/* "push" the noutfacets and nobjs variables, mark memory */
		tnf = noutfacets;
		tno = nobjs;
		p = rmark();
		/* recursively process object's contents */
		vrl_RenderObjlist(obj->original->contents);
		/* "pop" the variables and release the mark */
		rrelease(p);
		nobjs = tno;
		noutfacets = tnf;
		if (!obj->original->invisible)
			{
			/* display outside facets */
			for (j = 0; j < obj->noutfacets; ++j)
				{
				vrl_OutputFacet *of = ofs[j];
				if (!(of->original->interior))
					show_facet(of);
				}
			}
		}
	else
		{
		for (j = 0; j < obj->noutfacets; ++j)  /* display facets */
			show_facet(ofs[j]);
		}
	if (show_bb)
		{
		void show_bbox(vrl_OutputObject *obj);
		}
	}	

/********************************
 *        Object sorting        *
 ********************************/

static vrl_Vector viewpoint;   /* viewpoint in world coordinates */

/* should drop this inline: */

static int Obscures(vrl_OutputObject *a, vrl_OutputObject *b)
	{
	int i, overlap = 0;
	vrl_Matrix tmpmat;
	vrl_Vector tmpvect, tmpvect2, tmpvp;
	for (i = 0; i < 3; ++i)
		{
		/* check if b is on our side of a (either min or max sides) */
		if (viewpoint[i] <= a->original->minbound[i]
			&& b->original->maxbound[i] <= a->original->minbound[i])
			return 0;
		if (viewpoint[i] >= a->original->maxbound[i]
			&& b->original->minbound[i] >= a->original->maxbound[i])
			return 0;  /* viewpoint and b both greater than a's maxbound */
		if (viewpoint[i] >= b->original->minbound[i]
			&& a->original->maxbound[i] < b->original->minbound[i])
			return 0;
		if (viewpoint[i] <= b->original->maxbound[i]
			&& a->original->minbound[i] > b->original->maxbound[i])
			return 0;
		/* check if a and b overlap in this axis */
		if (b->original->maxbound[i] > a->original->minbound[i]
			&& b->original->minbound[i] < a->original->maxbound[i])
			++overlap;
		}
	if (overlap == 3)  /* bounding boxes overlap */
		for (i = 0; i < 3; ++i)
			{
			/* check if centroid of b is on our side of a */
			if (viewpoint[i] <= a->original->minbound[i]
				&& ((b->original->maxbound[i]+b->original->minbound[i])/2) < a->original->minbound[i])
					return 0;
			if (viewpoint[i] >= a->original->maxbound[i]
				&& ((b->original->maxbound[i]+b->original->minbound[i])/2) >= a->original->minbound[i])
					return 0;
			}
	return 1;   /* otherwise assume that a obscures b */
	}

/* this sorting algorithm is courtesy of Devin Kilminster
   (devink@tartarus.uwa.edu.au).
   Should also check out Newell-Newell-Sancha.
 */

static int first;  /* counts through objarray[] */

/* should re-code this next routine to be non-recursive */

static void Draw(vrl_OutputObject *obj)
	{
	register int i;
	obj->not_drawn = 0;  /* remove from consideration */
	for (i = first; i < nobjs; ++i)  /* draw all obscured objects */
		{
		vrl_OutputObject *p = objarray[i];
		if (p->not_drawn)
			if (Obscures(obj, p))
				Draw(p);
		}
	DrawObj(obj);   /* draw this object */
	}

/********************************
 * Top-Level Rendering Routines *
 ********************************/

/* should merge the two loops in this next routine, and get rid of
   objarray[] altogether, and ... */

vrl_RenderStatus *vrl_RenderObjlist(vrl_Object *objects)
	{
	register vrl_Object *obj;
	for (obj = objects; obj; obj = obj->next)
		process_object(obj);
#ifdef NEW_PROC
	if (vrl_process_direct)
		{
		new_process_end_scene();
		return &status;
		}
#endif
	if (object_sorting)  /* no need to sort; just draw */
		for (first = 0; first < nobjs; ++first)
			{
			register vrl_OutputObject *p = objarray[first];
			if (p->not_drawn)
				Draw(objarray[first]);
			}
	else  /* need to sort */
		for (first = 0; first < nobjs; ++first)
			DrawObj(objarray[first]);
	return &status;
	}

/* Initialize the rendering engine.  Parameters are:
     maxf: maximum number of renderable facets per frame
     maxvert: maximum number of vertices per object (after Z-clipping)
     maxobjs: maximum number of objects for sorting
     maxlgts: maximum number of lights per scene
     mempoolsize: amount of memory to allocate for per-frame data
 */

vrl_Boolean vrl_RenderInit(int maxvert, int maxf, int maxob, int maxlgts, unsigned int mempoolsize)
	{
	if (rinit(mempoolsize)) return -1;
	outfacets = vrl_calloc(maxfacets = maxf, sizeof(vrl_OutputFacet *));
	tmpverts = vrl_calloc(maxvertices = maxvert, sizeof(Tempvertex));
	objarray = vrl_calloc(maxobjs = maxob, sizeof(vrl_OutputObject *));
	locallights = vrl_calloc(maxlights = maxlgts, sizeof(LocalLight));
	if (outfacets == NULL || tmpverts == NULL
		|| objarray == NULL || locallights == NULL)
		{
		vrl_RenderQuit();
		return -1;
		}
	return 0;
	}

void vrl_RenderQuit(void)
	{
	if (locallights) vrl_free(locallights);
	if (objarray) vrl_free(objarray);
	if (tmpverts) vrl_free(tmpverts);
	if (outfacets) vrl_free(outfacets);
	rquit();
	}

/* compute the normals to the right and top clipping planes */
/* (we assume symmetry for the left and bottom clipping planes) */

static void camera_update(vrl_Camera *camera)
	{
	vrl_Vector right, up;
	vrl_VectorCreate(right, float2scalar(camera->zoom * 10000), 0, float2scalar(-10000));
	vrl_VectorNormalize(right);
	camera->aright = right[X];
	camera->cright = right[Z];
	vrl_VectorCreate(up, 0, float2scalar(camera->zoom * camera->aspect * 10000), float2scalar(-10000));
	vrl_VectorNormalize(up);
	camera->btop = up[Y];
	camera->ctop = up[Z];
	}

void vrl_RenderBegin(vrl_Camera *camera, vrl_Light *lights)
	{
	vrl_ScreenPos window_left, window_right, window_top, window_bottom;
	vrl_Matrix cammat;
	if (camera == NULL) return;
	if (camera->need_updating) camera_update(camera);
	display_can_gouraud = vrl_DisplayCanGouraud();
	display_clips = vrl_DisplayCanXYclip();
	vrl_DisplayGetWindow(&window_left, &window_top, &window_right, &window_bottom);
	/* compute offset of screen center from top left of viewport */
	window_halfwidth = ((vrl_ScreenCoord) (window_right - window_left + 1) / 2) << VRL_SCREEN_FRACT_BITS;
	window_halfheight = ((vrl_ScreenCoord) (window_bottom - window_top + 1) / 2) << VRL_SCREEN_FRACT_BITS;
	aspect = camera->aspect;
	/* these _clip variables are only used to check if we need to xy_clip facets */
	right_clip = window_halfwidth - 1;
	left_clip = -right_clip;
	bottom_clip = window_halfheight - 1;
	top_clip = -bottom_clip;
/* should eventually nuke cammat altogether */
	if (camera->object == NULL)  /* no parent; we're at the origin, looking into +Z */
		{
		vrl_MatrixIdentity(cammat);
		}
	else   /* normal case; we inherit our parent's globalmat */
		{
		vrl_MatrixCopy(cammat, camera->object->globalmat);
		}
	/* set up a few globals that are constant for this scene */
	vrl_VectorCopy(viewpoint, cammat[TRANS]);
	viewdir[X] = cammat[X][Z];
	viewdir[Y] = cammat[Y][Z];
	viewdir[Z] = cammat[Z][Z];
	hither = camera->hither;
	if (hither < 1) hither = 1;
	yon = camera->yon;
	if (yon < 1) yon = 1;
	vrl_MatrixInverse(wtv, cammat);
	ortho = camera->ortho;
	orthodiv = camera->orthodist;
	if (orthodiv == 0) orthodiv = camera->hither;
	/* compute screen scale factors and set clipping plane coefficients */
	if (ortho && 0)
		{
		screenscale_x = float2scalar(window_halfwidth);
		screenscale_y = float2scalar(-camera->aspect * window_halfheight);
		/* cright = window_halfwidth * orthodiv / screenscale_x */
		cright = float2factor(orthodiv);
		/* ctop = window_halfheight * orthodiv / screenscale_y */
		ctop = float2factor(orthodiv / camera->aspect);
		}
	else
		{
		screenscale_x = float2scalar(camera->zoom * window_halfwidth);
		screenscale_y = float2scalar(-camera->zoom * camera->aspect * window_halfheight);
		/* copy clipping plane coefficients for object culling */
		aright = camera->aright;  cright = camera->cright;
		btop = camera->btop;  ctop = camera->ctop;
		}
	rclear();  /* free up memory */
	nobjs = 0;       /* no output objects yet */
	noutfacets = 0;  /* no output facets yet */
	lightlist = lights;   /* store a copy of the list of lights */
	status.memory = status.objects = status.facets = 0;
#ifdef NEW_PROC
	if (vrl_process_direct)
		new_process_begin_scene();
#endif
	}

void vrl_TransformVertexToScreen(vrl_ScreenCoord *x, vrl_ScreenCoord *y, vrl_Object *obj, vrl_Vector vertex)
	{
	vrl_Matrix otv;
	vrl_Vector v;
	if (vrl_ObjectIsFixed(obj))
		vrl_MatrixCopy(otv, wtv);
	else
		vrl_MatrixMultiply(otv, wtv, obj->globalmat);
	vrl_Transform(v, otv, vertex);
	if (ortho)
		{
		*x = vrl_ScalarMultDiv(v[X], screenscale_x, orthodiv) + horizontal_shift;
		*y = vrl_ScalarMultDiv(v[Y], screenscale_y, orthodiv);
		}
	else
		{
		*x = vrl_ScalarMultDiv(v[X], screenscale_x, v[Z]) + horizontal_shift;
		*y = vrl_ScalarMultDiv(v[Y], screenscale_y, v[Z]);
		}
	*x += window_halfwidth;  *y += window_halfheight;
	}

void show_bbox(vrl_OutputObject *obj)
	{
	vrl_Vector m[8], s[8];
	int i;
	/* define the vertices of the object's bounding box: */
	m[0][X] = m[3][X] = m[4][X] = m[7][X] = obj->original->minbound[X];
	m[1][X] = m[2][X] = m[5][X] = m[6][X] = obj->original->maxbound[X];
	m[0][Y] = m[1][Y] = m[2][Y] = m[3][Y] = obj->original->minbound[Y];
	m[4][Y] = m[5][Y] = m[6][Y] = m[7][Y] = obj->original->maxbound[Y];
	m[0][Z] = m[1][Z] = m[4][Z] = m[5][Z] = obj->original->minbound[Z];
	m[2][Z] = m[3][Z] = m[6][Z] = m[7][Z] = obj->original->maxbound[Z];
	for (i = 0; i < 8; ++i)
		{
		vrl_Transform(s[i], wtv, m[i]);
		if (s[i][Z] < hither) return;  /* don't divide by zero! */
		s[i][X] = vrl_ScalarMultDiv(s[i][X], screenscale_x, s[i][Z]) + window_halfwidth;
		s[i][X] += horizontal_shift;
		s[i][Y] = vrl_ScalarMultDiv(s[i][Y], screenscale_y, s[i][Z]) + window_halfheight;
		}

	vrl_DisplayLine(s[0][X], s[0][Y], s[1][X], s[1][Y], bbox_color);
	vrl_DisplayLine(s[1][X], s[1][Y], s[2][X], s[2][Y], bbox_color);
	vrl_DisplayLine(s[2][X], s[2][Y], s[3][X], s[3][Y], bbox_color);
	vrl_DisplayLine(s[3][X], s[3][Y], s[0][X], s[0][Y], bbox_color);

	vrl_DisplayLine(s[4][X], s[4][Y], s[5][X], s[5][Y], bbox_color);
	vrl_DisplayLine(s[5][X], s[5][Y], s[6][X], s[6][Y], bbox_color);
	vrl_DisplayLine(s[6][X], s[6][Y], s[7][X], s[7][Y], bbox_color);
	vrl_DisplayLine(s[7][X], s[7][Y], s[4][X], s[4][Y], bbox_color);

	vrl_DisplayLine(s[0][X], s[0][Y], s[4][X], s[4][Y], bbox_color);
	vrl_DisplayLine(s[1][X], s[1][Y], s[5][X], s[5][Y], bbox_color);
	vrl_DisplayLine(s[2][X], s[2][Y], s[6][X], s[6][Y], bbox_color);
	vrl_DisplayLine(s[3][X], s[3][Y], s[7][X], s[7][Y], bbox_color);
	}

/********************************
 *       Horizon routines       *
 ********************************/

static void horizon_poly(vrl_OutputVertex *vertices, vrl_Color color)
	{
	vrl_OutputVertex *v = vertices;
	do
		{
		v->red = color;
		v = v->next;
		} while (v != vertices);
	if (!display_clips)
		vertices = vrl_DisplayXYclipPoly(vertices, VRL_DISPLAY_XYCLIP_CLIP_X|VRL_DISPLAY_XYCLIP_CLIP_Y);
	if (vertices)
		(*vrl_current_display_driver)(VRL_DISPLAY_POLY, VRL_SURF_SIMPLE, vertices);
	}

static void draw_horizon(vrl_ScreenCoord x1, vrl_ScreenCoord y1, vrl_ScreenCoord x2, vrl_ScreenCoord y2)
	{
	vrl_OutputVertex v[4];
	vrl_ScreenCoord rise = y2 - y1, run = x2 - x1;
	int sky, ground;
	v[0].next = &v[1];  v[0].prev = &v[3];
	v[1].next = &v[2];  v[1].prev = &v[0];
	v[2].next = &v[3];  v[2].prev = &v[1];
	v[3].next = &v[0];  v[3].prev = &v[2];
	if (rise == 0 && run == 0)
		{
		vrl_DisplayClear((viewdir[Y] < 0) ?
			vrl_WorldGetGroundColor() : vrl_WorldGetSkyColor());
		return;
		}
	if (abs(rise) < abs(run))
		{
		if (run < 0)
			{
			ground = vrl_WorldGetSkyColor();
			sky = vrl_WorldGetGroundColor();
			}
		else
			{
			sky = vrl_WorldGetSkyColor();
			ground = vrl_WorldGetGroundColor();
			}
		v[0].x = left_clip - 1 + window_halfwidth;
		v[0].y = y1 + vrl_ScalarMultDiv(rise, left_clip - x1, run) + window_halfheight;
		v[1].x = left_clip - 1 + window_halfwidth;
		v[1].y = -((vrl_ScreenCoord) 32767) << VRL_SCREEN_FRACT_BITS;
		v[2].x = right_clip + window_halfwidth;
		v[2].y = -((vrl_ScreenCoord) 32767) << VRL_SCREEN_FRACT_BITS;
		v[3].x = right_clip + window_halfwidth;
		v[3].y = y2 + vrl_ScalarMultDiv(rise, right_clip - x2, run) + window_halfheight;
		horizon_poly(v, sky);
		v[3].next = &v[2];  v[3].prev = &v[0];
		v[2].next = &v[1];  v[2].prev = &v[3];
		v[1].next = &v[0];  v[1].prev = &v[2];
		v[0].next = &v[3];  v[0].prev = &v[1];
		v[1].y = ((vrl_ScreenCoord) 32767) << VRL_SCREEN_FRACT_BITS;
		v[2].y = ((vrl_ScreenCoord) 32767) << VRL_SCREEN_FRACT_BITS;
		horizon_poly(&v[3], ground);
		}
	else /* rise > run */
		{
		if (rise < 0)
			{
			sky = vrl_WorldGetSkyColor();
			ground = vrl_WorldGetGroundColor();
			}
		else
			{
			ground = vrl_WorldGetSkyColor();
			sky = vrl_WorldGetGroundColor();
			}
		v[0].y = bottom_clip + 1 + window_halfheight;
		v[0].x = x1 + vrl_ScalarMultDiv(run, bottom_clip - y1, rise) + window_halfwidth;
		v[1].y = bottom_clip + 1 + window_halfheight;
		v[1].x = -((vrl_ScreenCoord) 32767) << VRL_SCREEN_FRACT_BITS;
		v[2].y = top_clip - 1 + window_halfheight;
		v[2].x = -((vrl_ScreenCoord) 32767) << VRL_SCREEN_FRACT_BITS;
		v[3].y = top_clip - 1 + window_halfheight;
		v[3].x = x2 + vrl_ScalarMultDiv(run, top_clip - y2, rise) + window_halfwidth;
		horizon_poly(v, sky);
		v[3].next = &v[2];  v[3].prev = &v[0];
		v[2].next = &v[1];  v[2].prev = &v[3];
		v[1].next = &v[0];  v[1].prev = &v[2];
		v[0].next = &v[3];  v[0].prev = &v[1];
		v[1].x = ((vrl_ScreenCoord) 32767) << VRL_SCREEN_FRACT_BITS;
		v[2].x = ((vrl_ScreenCoord) 32767) << VRL_SCREEN_FRACT_BITS;
		horizon_poly(&v[3], ground);
		}
	}

void vrl_RenderHorizon(void)
	{
	vrl_Vector p, q, a, b;
vrl_DisplayClear(vrl_WorldGetSkyColor());  // debugging only
return;  // debugging only
	/* compute endpoints of "chord" of horizon circle, in world space */
	if (viewdir[X] == 0 && viewdir[Y] == 0 && viewdir[Z] == 0)
		return;
	p[X] = vrl_FactorMultiply(viewdir[X] - viewdir[Z], float2scalar(32000));
	p[Y] = 0;
	p[Z] = vrl_FactorMultiply(viewdir[Z] + viewdir[X], float2scalar(32000));
	q[X] = vrl_FactorMultiply(viewdir[X] + viewdir[Z], float2scalar(32000));
	q[Y] = 0;
	q[Z] = vrl_FactorMultiply(viewdir[Z] - viewdir[X], float2scalar(32000));
	if (viewdir[Y] < -float2factor(0.85))
		{
		vrl_DisplayClear(vrl_WorldGetGroundColor());
		return;
		}
	if (viewdir[Y] > float2factor(0.85))
		{
		vrl_DisplayClear(vrl_WorldGetSkyColor());
		return;
		}
	/* Transform to viewspace */
	vrl_Transform(a, wtv, p);
	vrl_Transform(b, wtv, q);
	/* project and scale to screen space */
	if (a[Z] < 1) a[Z] = 1;
	if (b[Z] < 1) b[Z] = 1;
	a[X] = vrl_ScalarMultDiv(a[X], screenscale_x, a[Z]);
	a[Y] = vrl_ScalarMultDiv(a[Y], -screenscale_y, a[Z]);
	b[X] = vrl_ScalarMultDiv(b[X], screenscale_x, b[Z]);
	b[Y] = vrl_ScalarMultDiv(b[Y], -screenscale_y, b[Z]);
	draw_horizon(a[X], a[Y], b[X], b[Y]);
	}
