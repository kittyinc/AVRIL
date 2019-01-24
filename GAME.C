/* Game logic for "Boris" */
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
#include "area.h"
#include "game.h"

int game_flags[100] = { 0 };

vrl_Scalar users_pelvis_height = 2000;  /* pelvis is 2 meters off ground (!) */
vrl_Scalar users_chest_height = 1000;   /* head is 1 meter above pelvis */

float users_velocity = 1;   /* walk at 1 meter per second */

void game_setup(void)
	{
	}

void game_bump(void)
	{
	}

