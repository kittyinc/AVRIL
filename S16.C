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
#include <dos.h>

#define	FCR	2		/* FIFO control register (16550A only) */

#define	IIR	2		/* Interrupt ident register */
#define IIR_FIFO_ENABLED 0xc0	/* FIFO enabled (FCR0,1 = 1) - 16550A only */

/* 16550A FIFO control register values */
#define	FIFO_ENABLE	0x01	/* enable TX & RX fifo */
#define	FIFO_CLR_RX	0x02	/* clear RX fifo */
#define	FIFO_CLR_TX	0x04	/* clear TX fifo */
#define	FIFO_START_DMA	0x08	/* enable TXRDY/RXRDY pin DMA handshake */
#define FIFO_SIZE_1	0x00	/* RX fifo trigger levels */
#define FIFO_SIZE_4	0x40
#define FIFO_SIZE_8	0x80
#define FIFO_SIZE_14	0xC0
#define FIFO_SIZE_MASK	0xC0

#define FIFO_SETUP     (FIFO_ENABLE|FIFO_CLR_RX|FIFO_CLR_TX|FIFO_SIZE_4)

void main(void)
	{
	unsigned int base = 0x2F8;
	int is_16550a = 0;
	vrl_SerialPort *port = vrl_SerialOpen(base, 3, 2000);
	atexit(vrl_SerialCloseAll);

	outp(base+FCR, (unsigned char) FIFO_ENABLE);
	/* According to National ap note AN-493, the FIFO in the 16550 chip
	 * is broken and must not be used. To determine if this is a 16550A
	 * (which has a good FIFO implementation) check that both bits 7
	 * and 6 of the IIR are 1 after setting the fifo enable bit. If
	 * not, don't try to use the FIFO.
	 */
	if ((inp(base+IIR) & IIR_FIFO_ENABLED) == IIR_FIFO_ENABLED)
		{
		is_16550a = 1;
		outp(base+FCR, (unsigned char) FIFO_SETUP);
		}
	else
		{
		/* Chip is not a 16550A. In case it's a 16550 (which has a
		 * broken FIFO), turn off the FIFO bit.
		 */
		outp(base+FCR, (unsigned char) 0);
		is_16550a = 0;
	 	}
	if (is_16550a) printf("Yes\n");
	else printf("No\n");	
//flush
//	if (is_16550a)
//		outp(base+FCR, (unsigned char) FIFO_SETUP);
	}
