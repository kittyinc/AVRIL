/* The "vrl_System" functions for AVRIL applications */
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
#include <stdlib.h>  /* atexit(), exit() */
#include <string.h>  /* strstr() */
#include <signal.h>

static vrl_Boolean system_running = 0;   /* non-zero while system is initialized and running */

void vrl_SystemStartRunning(void)
	{
	system_running = 1;
	}

void vrl_SystemStopRunning(void)
	{
	system_running = 0;
	}

vrl_Boolean vrl_SystemIsRunning(void)
	{
	return system_running;
	}

static vrl_Boolean need_to_redraw = 0;   /* non-zero if the screen needs updating */

void vrl_SystemRequestRefresh(void)
	{
	need_to_redraw = 1;
	}

vrl_Boolean vrl_SystemQueryRefresh(void)
	{
	return need_to_redraw;
	}

vrl_Boolean vrl_SystemStartup(void)
	{
	vrl_MathInit();
	vrl_WorldInit(vrl_WorldGetCurrent());
	if (vrl_VideoSetup(0))
		{
		printf("Could not enter graphics mode!\n");
		return -1;
		}
	atexit(vrl_VideoShutdown);
	if (vrl_DisplayInit(NULL))
		return -1;
	atexit(vrl_DisplayQuit);
	vrl_MouseInit();
	atexit(vrl_MouseQuit);
	if (vrl_TimerInit())
		return -2;
	atexit(vrl_TimerQuit);
	if (vrl_RenderInit(800, 800, 500, 5, 65000))
		return -3;
	atexit(vrl_RenderQuit);
	atexit(vrl_DeviceCloseAll);
	atexit(vrl_SerialCloseAll);
	/* make sure that exit() [and therefore the atexit() functions] get
	   called if there are any fatal errors */
	signal(SIGABRT, exit);
	signal(SIGFPE, exit);
	signal(SIGILL, exit);
	signal(SIGINT, exit);
	signal(SIGSEGV, exit);
	signal(SIGTERM, exit);
	vrl_SystemStartRunning();
	vrl_SystemRequestRefresh();
	vrl_SystemRender(NULL);
	return 0;
	}

static void check_mouse(void)
	{
	unsigned int mouse_buttons;
	if (vrl_MouseGetUsage())  /* being used as 6D pointing device */
		return;
	if (!vrl_MouseRead(NULL, NULL, NULL))  /* mouse hasn't changed */
		return;
	vrl_MouseRead(NULL, NULL, &mouse_buttons);
	if (mouse_buttons)  /* button down */
		{
		int mouse_x, mouse_y;
		vrl_ScreenPos win_x, win_y;
		vrl_unsigned16bit down_buttons = mouse_buttons;
		vrl_DisplayGetWindow(&win_x, &win_y, NULL, NULL);
		while (mouse_buttons)  /* wait for button release */
			vrl_MouseRead(&mouse_x, &mouse_y, &mouse_buttons);
		if (down_buttons & 0x07)
			vrl_ApplicationMouseUp(mouse_x - win_x, mouse_y - win_y, down_buttons);
		}
	}

void vrl_SystemRun(void)
	{
	vrl_ApplicationInit();
	if (vrl_WorldGetStereoConfiguration())
		vrl_StereoConfigure(vrl_WorldGetStereoConfiguration());
	vrl_SystemRequestRefresh();
	while (vrl_SystemIsRunning())
		{
		vrl_Object *list;
		if (vrl_KeyboardCheck())
			vrl_ApplicationKey(vrl_KeyboardRead());
		check_mouse();
		vrl_TaskRun();
		vrl_DevicePollAll();
		list = vrl_WorldUpdate();
		if (vrl_SystemQueryRefresh())
			vrl_SystemRender(list);
		}
	}

static vrl_Time last_render_ticks;

vrl_Time vrl_SystemGetRenderTime(void)
	{
	return last_render_ticks;
	}

vrl_Time vrl_SystemGetFrameRate(void)
	{
	if (last_render_ticks == 0) last_render_ticks = 1;  /* so we don't divide by zero! */
	return vrl_TimerGetTickRate() / last_render_ticks;
	}

vrl_RenderStatus *vrl_SystemRender(vrl_Object *list)
	{
	static vrl_Object *lastlist = NULL;
	vrl_Palette *pal;
	vrl_StereoConfiguration *conf;
	vrl_RenderStatus *stat;
	int pagenum;
	int two_eyes = 0;
	vrl_Time render_start = vrl_TimerRead();
	if (list == NULL)
		list = lastlist;
	else
		lastlist = list;
	pal = vrl_WorldGetPalette();
	if (vrl_PaletteHasChanged(pal))
		{
		vrl_VideoSetPalette(0, 255, pal);
		vrl_PaletteSetChanged(pal, 0);
		}
	pagenum = vrl_VideoGetDrawPage();
	if (++pagenum >= vrl_VideoGetNpages())
		pagenum = 0;
	vrl_VideoSetDrawPage(pagenum);
	vrl_RenderSetAmbient(vrl_WorldGetAmbient());
	vrl_DisplayStereoSetDrawEye(VRL_STEREOEYE_BOTH);
	if (vrl_WorldGetScreenClear())
		{
		vrl_DisplayBeginFrame();
		if (vrl_WorldGetHorizon() && !vrl_RenderGetDrawMode())
			vrl_RenderHorizon();
		else
			vrl_DisplayClear(vrl_WorldGetSkyColor());
		vrl_DisplayEndFrame();
		}
	vrl_ApplicationDrawUnder();
	conf = vrl_WorldGetStereoConfiguration();
	if (conf)
		two_eyes = vrl_StereoGetNeyes(conf);
	if (vrl_WorldGetStereo() && vrl_WorldGetLeftCamera() && vrl_WorldGetRightCamera() && two_eyes)
		{
		/* draw left-eye image */
		vrl_DisplayStereoSetDrawEye(VRL_STEREOEYE_LEFT);
		vrl_RenderSetHorizontalShift(vrl_StereoGetTotalLeftShift(conf));
		vrl_DisplayBeginFrame();
		vrl_RenderBegin(vrl_WorldGetLeftCamera(), vrl_WorldGetLights());
		stat = vrl_RenderObjlist(list);
		vrl_DisplayEndFrame();

		/* draw right-eye image */
		vrl_DisplayStereoSetDrawEye(VRL_STEREOEYE_RIGHT);
		vrl_RenderSetHorizontalShift(vrl_StereoGetTotalRightShift(conf));
		vrl_RenderBegin(vrl_WorldGetRightCamera(), vrl_WorldGetLights());
		vrl_DisplayBeginFrame();
		stat = vrl_RenderObjlist(list);
		vrl_DisplayEndFrame();
		}
	else  /* not two-eye stereo */
		{
		vrl_DisplayStereoSetDrawEye(VRL_STEREOEYE_BOTH);
		vrl_RenderSetHorizontalShift(0);
		vrl_DisplayBeginFrame();
		vrl_RenderBegin(vrl_WorldGetCamera(), vrl_WorldGetLights());
		stat = vrl_RenderObjlist(list);
		vrl_DisplayEndFrame();
		}
	vrl_DisplayStereoSetDrawEye(VRL_STEREOEYE_BOTH);
	vrl_RenderSetHorizontalShift(0);
	vrl_ApplicationDrawOver(stat);
	vrl_VideoCursorHide();
	vrl_DisplayUpdate();
	vrl_VideoSetViewPage(pagenum);
	vrl_VideoCursorShow();
	last_render_ticks = vrl_TimerRead() - render_start;
	need_to_redraw = 0;
	return stat;
	}

void vrl_SystemCommandLine(int argc, char *argv[])
	{
	int i;
	vrl_Camera *cam;
	for (i = 1; i < argc; ++i)  /* i = 1 to skip argv[0] */
		{
		FILE *in = fopen(argv[i], "r");
		if (in == NULL) continue;
		if (strstr(argv[i], ".wld"))
			vrl_ReadWLD(in);
		else if (strstr(argv[i], ".fig"))
			vrl_ReadFIG(in, NULL, NULL);
		else if (strstr(argv[i], ".plg"))
			vrl_ReadObjectPLG(in);
		/* ignore anything else */
		fclose(in);
		}
	if (!vrl_WorldGetCamera())   /* need to have a camera */
		vrl_CameraCreate();
	vrl_WorldUpdate();
	}

