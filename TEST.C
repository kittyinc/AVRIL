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
#include <stdio.h>
#include <malloc.h>

void mal(unsigned int nbytes)
	{
	unsigned char *p = malloc(nbytes);
	printf("%p to ", p);
	printf("%p", p + nbytes);
	printf(" (%d bytes)\n", nbytes);
	}

void main(void)
	{
	mal(20);
	mal(64000u);
	mal(1280);
	mal(400);
	mal(65000);
	mal(3200);
	mal(20000);
	mal(2000);
	mal(265);
	mal(27);
	mal(167);
	mal(167);
	mal(55);
	mal(167);
	mal(167);
	}
