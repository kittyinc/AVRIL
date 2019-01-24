/* Stereo configuration routines for AVRIL */
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

vrl_StereoConfiguration *vrl_StereoInitConfiguration(vrl_StereoConfiguration *conf)
	{
	if (conf == NULL)
		return NULL;
	conf->type = VRL_STEREOTYPE_NONE;
	conf->eyespacing = 60.0 / vrl_WorldGetScale();
	conf->convergence = 600.0 / vrl_WorldGetScale();
	conf->leftrot = conf->rightrot = 0;
	conf->left_eye_shift = conf->right_eye_shift = 0;
	conf->center_offset = 0;
	conf->two_eyes = 1;
	return conf;
	}

/* Create the left-eye and right-eye stereoscopic cameras */

int vrl_StereoSetup(void)
	{
	vrl_Camera *left, *right, *cyclopean = vrl_WorldGetCamera();
	vrl_Object *head;

	if (vrl_WorldGetLeftCamera() && vrl_WorldGetRightCamera())
		return 0;

	if (cyclopean == NULL)
		return -1;
	head = vrl_CameraGetObject(cyclopean);
	if (head == NULL)
		return -2;
	left = vrl_CameraCreate();
	if (left == NULL)
		return -3;
	right = vrl_CameraCreate();
	if (right == NULL)
		return -4;

	/* left eye */
	vrl_CameraSetHither(left, vrl_CameraGetHither(cyclopean));
	vrl_CameraSetYon(left, vrl_CameraGetYon(cyclopean));
	vrl_CameraSetZoom(left, vrl_CameraGetZoom(cyclopean));
	vrl_CameraSetAspect(left, vrl_CameraGetAspect(cyclopean));
	vrl_CameraSetHither(left, vrl_CameraGetHither(cyclopean));
	vrl_CameraSetName(left, vrl_CameraGetName(cyclopean));
	vrl_CameraAttach(left, head);
	vrl_CameraRotReset(left);
	vrl_CameraMove(left, 0, 0, 0);
	vrl_WorldSetLeftCamera(left);

	/* right eye */
	vrl_CameraSetHither(right, vrl_CameraGetHither(cyclopean));
	vrl_CameraSetYon(right, vrl_CameraGetYon(cyclopean));
	vrl_CameraSetZoom(right, vrl_CameraGetZoom(cyclopean));
	vrl_CameraSetAspect(right, vrl_CameraGetAspect(cyclopean));
	vrl_CameraSetHither(right, vrl_CameraGetHither(cyclopean));
	vrl_CameraSetName(right, vrl_CameraGetName(cyclopean));
	vrl_CameraAttach(right, head);
	vrl_CameraRotReset(right);
	vrl_CameraMove(right, 0, 0, 0);
	vrl_WorldSetRightCamera(right);

	return 0;
	}

/* Configure the left-eye and right-eye cameras */

int vrl_StereoConfigure(vrl_StereoConfiguration *conf)
	{
	vrl_ScreenPos winx1, winy1, winx2, winy2;
	vrl_Scalar spacing = float2scalar((conf->eyespacing / scalar2float(vrl_WorldGetScale())));
	float zoom = vrl_CameraGetZoom(vrl_WorldGetCamera());
	float aspect = vrl_CameraGetAspect(vrl_WorldGetCamera());
	vrl_Camera *left, *right;

	left = vrl_WorldGetLeftCamera();
	right = vrl_WorldGetRightCamera();
	if (left == NULL || right == NULL)
		if (vrl_StereoSetup())
			return -1;
	left = vrl_WorldGetLeftCamera();
	right = vrl_WorldGetRightCamera();

	vrl_CameraSetZoom(left, zoom);
	vrl_CameraSetAspect(left, aspect);
	vrl_CameraRotReset(left);
	vrl_CameraMove(left, 0, 0, 0);

	vrl_CameraSetZoom(right, zoom);
	vrl_CameraSetAspect(right, aspect);
	vrl_CameraRotReset(right);
	vrl_CameraMove(right, 0, 0, 0);

	if (conf->type == VRL_STEREOTYPE_CYBERSCOPE)
		{
		vrl_CameraRotZ(left, float2angle(-90));
		vrl_CameraRotZ(right, float2angle(90));
		}

	if (conf->type == VRL_STEREOTYPE_CHROMADEPTH || conf->type == VRL_STEREOTYPE_SIRDS)
		conf->two_eyes = 0;
	else
		conf->two_eyes = 1;

	vrl_CameraRotY(left, conf->leftrot);
	vrl_CameraRotY(right, conf->rightrot);

	vrl_CameraMove(left, -spacing / 2, 0, 0);
	vrl_CameraMove(right, spacing / 2, 0, 0);

	vrl_DisplayStereoSetType(conf->type);
	vrl_DisplayGetWindow(&winx1, &winy1, &winx2, &winy2);
	conf->center_offset = (conf->eyespacing / 2) * zoom * ((winx2 - winx1 + 1)/2) / conf->convergence;
	return 0;
	}
