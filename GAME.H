/* Game logic routines and global variables for "Boris" */
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
extern int game_flags[100];             /* the flags used by the game */

extern vrl_Scalar users_pelvis_height;  /* height of pelvis above ground */
extern vrl_Scalar users_chest_height;   /* height of head above pelvis */
extern float users_velocity;            /* speed at which we walk */

/* the area the user is currently in */
extern Area *current_area;

/* the user's body and head */
extern vrl_Object *users_body, *users_head;

/* the object we were looking at when we used the "select" button */
extern vrl_Object *target_object;

void game_setup(void);  /* called once at startup */
void game_bump(void);   /* called when we collide with a wall */

/* End of game.h */
