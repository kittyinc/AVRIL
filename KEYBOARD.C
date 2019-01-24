/* Keyboard support for AVRIL applications */
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
#include "avrilkey.h"
#include <bios.h>

vrl_Boolean vrl_KeyboardCheck(void)
	{
	return _bios_keybrd(1);
	}

unsigned int vrl_KeyboardRead(void)
	{
	unsigned c = _bios_keybrd(0);              /* keycode */
	unsigned shift = _bios_keybrd(2) & 0x03;   /* left and right shift keys */
	if ((c & 0xFF) == 0)    /* extended keycode */
		return c | shift;   /* code in top byte, shift values in bottom */
	c &= 0xFF;              /* not extended, so clear top byte */
	if (shift)              /* check for shifted keypad */
		switch (c)
			{
			case '8': return UPKEY | shift;
			case '2': return DOWNKEY | shift;
			case '4': return LEFTKEY | shift;
			case '6': return RIGHTKEY | shift;
			default: break;
			}
	return c;
	}
