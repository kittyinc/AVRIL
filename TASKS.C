/* Support for "tasks" in AVRIL */
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

typedef struct _task Task;

struct _task
	{
	void (*function)(void);
	void *data;
	vrl_Time period;
	vrl_Time lastran;
	Task *next;
	};

static Task *tasklist = NULL;

vrl_Boolean vrl_TaskCreate(void (*function)(void), void *data, vrl_Time period)
	{
	Task *t = vrl_malloc(sizeof(Task));
	if (t == NULL) return -1;
	t->period = period;
	t->lastran = vrl_TimerRead();
	t->function = function;
	t->data = data;
	t->next = tasklist;
	tasklist = t;
	return 0;
	}

static void *current_task_data = NULL;
static vrl_Time current_task_elapsed;
static vrl_Time timenow;

/* Lets the current task get a pointer to its data */
void *vrl_TaskGetData(void)
	{
	return current_task_data;
	}

/* Lets the current task get the elapsed time since it last ran */
vrl_Time vrl_TaskGetElapsed(void)
	{
	return current_task_elapsed;
	}

/* Lets the current task get the current time (same time for all tasks) */
vrl_Time vrl_TaskGetTimeNow(void)
	{
	return timenow;
	}

void vrl_TaskRun(void)
	{
	Task *t;
	static vrl_Time lastran = 0L;
	timenow = vrl_TimerRead();
	if (timenow < lastran)  /* timer wrapper around */
		{
		lastran = timenow;
		return;
		}
	for (t = tasklist; t; t = t->next)
		{
		current_task_elapsed = timenow - t->lastran;
		current_task_data = t->data;
		if (t->period == 0 || current_task_elapsed > t->period)
			t->function();
		t->lastran = timenow;
		}
	}

