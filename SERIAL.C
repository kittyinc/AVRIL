/* Serial device support for PC version of AVRIL */
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

static vrl_SerialPort *portlist = NULL;

static int buff_is_full(vrl_SerialPort *p)
	{
	if (p->in == (p->out-1)) return 1;
	if (p->in == (p->buffsize-1) && p->out == 0) return 1;
	return 0;
	}

static void interrupt far commhandler(void)
	{
	vrl_SerialPort *p;
	for (p = portlist; p; p = p->next)
		{
		if (p->address == 0) continue;  /* port not in use */
		if (p->buffer == NULL) continue;  /* not interrupt-driven */
		while (inp(p->address + 5) & 0x01)
			{
			unsigned int c = inp(p->address);
			if (!buff_is_full(p))
				{
				p->buffer[p->in++] = c;
				if (p->in >= p->buffsize)
					p->in = 0;
				}
			}
		}
	outp(0x20, (unsigned char) 0x20);
	}

void vrl_SerialSetParameters(vrl_SerialPort *p, unsigned int baud, vrl_ParityType parity, int databits, int stopbits)
	{
	union REGS regs;
	unsigned int div, word = 0;
	switch (baud)
		{
		case 110: div = 0x0417; break; 
		case 150: div = 0x0300; break; 
		case 300: div = 0x0180; break; 
		case 600: div = 0x00C0; break;
		case 1200: div = 0x0060; break;
		case 2400: div = 0x0030; break;
		case 4800: div = 0x0018; break;
		case 9600: div = 0x000C; break;
		case 19200: div = 0x0006; break;
		default: div = 0; break;
		}
	if (div)
		{
		outp(p->address + 3, (unsigned char) (inp(p->address + 3) | 0x80));
		outp(p->address, (unsigned char) (div & 0xFF));
		outp(p->address + 1, (unsigned char) (div >> 8));
		outp(p->address + 3, (unsigned char) (inp(p->address + 3) & 0x7F));
		}
	switch (parity)
		{
		case VRL_PARITY_EVEN: word |= 0x18; break; 
		case VRL_PARITY_ODD: word |= 0x08; break;
		case VRL_PARITY_NONE: break;
		default: break;
		}
	word |= (databits - 5);
	if (stopbits == 2) word |= 4;
	outp(p->address + 3, (unsigned char) word);
	}

static void interrupt far (*oldvectors[16])() = { NULL };

static unsigned char masks[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

vrl_SerialPort *vrl_SerialOpen(unsigned int address, int irq, unsigned int buffsize)
	{
	vrl_SerialPort *p;
	if (irq < 0 || irq >= 16)  /* check for valid IRQ */
		return NULL;
	for (p = portlist; p; p = p->next)
		if (p->address == address)  /* address already in use */
			return NULL;
	p = vrl_malloc(sizeof(vrl_SerialPort));
	if (p == NULL) return NULL;
	p->buffsize = buffsize;
	if (buffsize == 0)  /* no interrupts */
		{
		p->irq = 0;
		p->buffer = NULL;
		}
	else   /* use interrupts */
		{
#ifndef __386__
		p->irq = irq;
		p->buffer = vrl_malloc(p->buffsize = buffsize);
		if (p->buffer == NULL)
			{
			vrl_free(p);
			return NULL;  /* couldn't allocate buffer */
			}
		p->in = p->out = 0;
		if (oldvectors[irq] == NULL)
			{
			oldvectors[irq] = _dos_getvect(irq + 8);
			_dos_setvect(irq + 8, commhandler);
			}
		outp(address + 1, (unsigned char) 0x01);  /* enable char-received interrupts */
		outp(address + 4, (unsigned char) (inp(address + 4) & 0xEF));  /* turn off loop-back */
		outp(address + 4, (unsigned char) (inp(address + 4) | 0x08));  /* enable 8250 interrupt generation */
		outp(0x21, (unsigned char) (inp(0x21) & ~masks[irq]));   /* enable irq */
		inp(address);  inp(address);
#endif
		}
	p->address = address;
	p->next = portlist;  portlist = p;   /* link into list */
	vrl_SerialSetParameters(p, 9600, VRL_PARITY_NONE, 8, 1);
	return p;
	}

void vrl_SerialClose(vrl_SerialPort *p)
	{
	vrl_SerialPort *p2;
	if (p->address == 0) return;
	outp(p->address + 4, (unsigned char) (inp(p->address + 4) & 0xF7));  /* disable 8250 interrupt generation */
	if (p->buffer == NULL) return;
	vrl_free(p->buffer);  p->buffer = NULL;
	if (p == portlist)
		portlist = p->next;
	else
		for (p2 = portlist; p2; p2 = p2->next)
			if (p2->next == p)
				{
				p2->next = p->next;
				break;
				}
	/* see if we can detach the interrupt */
	for (p2 = portlist; p2; p2 = p2->next)
		if (p2->address && p2->irq == p->irq && p2 != p)  /* somebody else also using this irq */
			return;
#ifndef __386__
	outp(0x21, (unsigned char) (inp(0x21) | masks[p->irq]));  /* mask off irq */
	_dos_setvect(p->irq + 8, oldvectors[p->irq]);
#endif
	}

vrl_Boolean vrl_SerialCheck(vrl_SerialPort *p)
	{
	if (p->buffer == NULL)
		return inp(p->address + 5) & 0x01;
	return p->in != p->out;
	}

unsigned int vrl_SerialGetc(vrl_SerialPort *p)
	{
	int c;
	if (p->buffer == NULL)  /* not interrupt driven */
		{
		while ((inp(p->address + 5) & 0x01) == 0);
		return inp(p->address);
		}
	if (p->out == p->in) return 0;
	c = p->buffer[p->out++];
	if (p->out >= p->buffsize)
		p->out = 0;
	return c & 0xFF;
	}

void vrl_SerialFlush(vrl_SerialPort *p)
	{
	while (vrl_SerialCheck(p))
		vrl_SerialGetc(p);
	}
	
void vrl_SerialPutc(unsigned int c, vrl_SerialPort *p)
	{
	while ((inp(p->address + 5) & 0x20) == 0);
	outp(p->address, (unsigned char) c);
	}

void vrl_SerialPutString(unsigned char *s, vrl_SerialPort *p)
	{
	while (*s)
		vrl_SerialPutc(*s++, p);
	}

void vrl_SerialModemControl(int value, vrl_SerialPort *p)
	{
	outp(p->address + 4, (unsigned char) (inp(p->address + 4) & 0xEC) | ((~value) & 0x03));
	}

void vrl_SerialSetRTS(vrl_SerialPort *p, vrl_Boolean value)
	{
	outp(p->address + 4, (unsigned char) (inp(p->address + 4) & 0xED) | (value ? 0x02 : 0x00));
	}

void vrl_SerialSetDTR(vrl_SerialPort *p, vrl_Boolean value)
	{
	outp(p->address + 4, (unsigned char) (inp(p->address + 4) & 0xEE) | (value ? 0x01 : 0x00));
	}

void vrl_SerialCloseAll(void)
	{
	vrl_SerialPort *p;
	for (p = portlist; p; p = p->next)
		if (p->address)
			vrl_SerialClose(p);
	}

void vrl_SerialFifo(vrl_SerialPort *p, int n)
	{
	int fifo;
	switch (n)
		{
		default:
		case 0: fifo = 0x00; break;
		case 1: fifo = 0x01; break;
		case 4: fifo = 0x41; break;
		case 8: fifo = 0x81; break;
		case 14: fifo = 0xC1; break;
		}
	outp(p->address + 2, (unsigned char) fifo);
	}
