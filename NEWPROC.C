/* New rendering code for AVRIL */
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

extern int vrl_process_direct;

static int count;

static int counter(vrl_Facet *facet)
	{
	count += facet->npoints;
	return 0;
	}

static vrl_Rep *thisrep;
static int nedges_added;

static void insert_edge(int v1, int v2)
	{
	int i;
	for (i = 0; i < nedges_added; ++i)
		if ((v1 == thisrep->edges[i].v1 && v2 == thisrep->edges[i].v2)
			|| (v1 == thisrep->edges[i].v2 && v2 == thisrep->edges[i].v1))
			return;
	thisrep->edges[nedges_added].v1 = v1;
	thisrep->edges[nedges_added].v2 = v2;
	++nedges_added;
	}

static int addedge(vrl_Facet *facet)
	{
	int i;
	if (facet->edges == NULL)
		{
		facet->edges = vrl_calloc(facet->npoints, sizeof(int));
		if (facet->edges == NULL)
			return 1;
		}
	for (i = 0; i < facet->npoints - 1; ++i)
		{
		insert_edge(facet->points[i], facet->points[i+1]);
		facet->edges[i] = nedges_added-1;
		}
	insert_edge(facet->points[i], facet->points[0]);
	facet->edges[i] = nedges_added-1;
	return 0;
	}

void vrl_RepBuildEdges(vrl_Rep *rep)
	{
	count = 0;
	vrl_RepTraverseFacets(rep, counter);
	rep->edges = vrl_calloc(count / 2, sizeof(vrl_Edge));
	if (rep->edges == NULL) return;
	thisrep = rep;
	nedges_added = 0;
	vrl_RepTraverseFacets(rep, addedge);
	}

void new_process_begin_scene(void)
	{
	}

void new_process_end_scene(void)
	{
	}

void new_process_object(vrl_Object *obj, vrl_Rep *rep)
	{
	if (rep->edges == NULL)
		{
		vrl_RepBuildEdges(rep);
		if (rep->edges == NULL)
			{
			vrl_process_direct = 0;
			return;
			}
		}		
	}

void new_process_facet(vrl_Facet *facet)
	{
	}

