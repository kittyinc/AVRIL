/* High-resolution timer routines for the AVRIL library */
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

#ifdef __386__
int notimer = 1;
#else
int notimer = 0;
#endif

#include "avril.h"
#include <dos.h>

static void interrupt (*old_isr)(void) = NULL;
static void (*retrace_handler)(void);

#define TICKRATE 1192500L  /* ticks per second */

static unsigned int interrupts_per_second;
static unsigned int interrupts_per_frame;
static unsigned int ticks_per_interrupt;  /* value written to the chip  */
static unsigned int ticks_per_frame;
static unsigned int milliseconds_per_interrupt;

static vrl_Time milliseconds = 0;  /* current time in milliseconds */
static vrl_Time doscount = 0;

static int frame_count = 0;
static int frame_sync_count = 0;
static int frame_sync_period = 5;

static unsigned int read_timer(void)
	{
	unsigned int n;
	_disable();
	outp(0x43, (unsigned char) 0);
	n = inp(0x40) & 0xFF;
	n |= (inp(0x40) << 8);
	_enable();
	return n;
	}

static void write_timer(unsigned int value)
	{
	outp(0x43, (unsigned char) 0x34);
	outp(0x40, (unsigned char) value);
	outp(0x40, (unsigned char) (value >> 8));
	}

static void wait_for_retrace(void)
	{
	while (vrl_VideoCheckRetrace());    /* wait for no retrace */
	while (!vrl_VideoCheckRetrace());   /* wait for start of retrace */
	}

static void interrupt isr()
	{
	milliseconds += milliseconds_per_interrupt;  /* update the time */
	if (--frame_count <= 0)
		{
		frame_count = interrupts_per_frame;
		if (retrace_handler)
			{
			if (--frame_sync_count <= 0)
				{
				frame_sync_count = frame_sync_period;
				write_timer(0);
#ifdef RETRACE
				while (!vrl_VideoCheckRetrace());   /* wait for start of retrace */
#endif
				while ((inp(0x3DA) & 0x08) == 0);
				write_timer(ticks_per_interrupt);
				retrace_handler();
				}
			}
		}
	doscount += ticks_per_interrupt;
	if (doscount > 65536L)
		{
		doscount &= 0xFFFF;  /* make it wrap around */
#ifndef __386__
		_chain_intr(*old_isr);
#endif
		}
	else
		outp(0x20, (unsigned char) 0x20);   /* non-specific EOI */
	}

static unsigned int time_retrace(void)
	{
	unsigned int period;
	_disable();
	write_timer(0);
	wait_for_retrace();
	_enable();
	wait_for_retrace();
	period = read_timer();
	wait_for_retrace();
	wait_for_retrace();
	period -= read_timer();
	period /= 2;
	return period;
	}

vrl_Boolean vrl_TimerInit(void)
	{
if (notimer) return 0;
	/* first, we get an approximate ticks per interrupt value */
	ticks_per_interrupt = TICKRATE/200;  /* aim for 200 ticks per second */

	/* now time the retrace, and find out how many ticks between retraces */
	ticks_per_frame = time_retrace();
	interrupts_per_frame = ticks_per_frame / ticks_per_interrupt;

	/* now re-compute the ticks per interrupt to be a multiple of ticks per frame */
	ticks_per_interrupt = ticks_per_frame / interrupts_per_frame;

	/* finally, compute the number of interrupts per second */
	interrupts_per_second = TICKRATE/ticks_per_interrupt;

	/* and the number of milliseconds per interrupt */
	milliseconds_per_interrupt = 1000/interrupts_per_second;

#ifdef DEBUGGING
printf("t/i = %u, t/f = %u, i/f = %u, i/s = %u, m/i = %u\n",
		ticks_per_interrupt, ticks_per_frame, interrupts_per_frame,
		interrupts_per_second, milliseconds_per_interrupt);
#endif
	frame_count = 0;
	frame_sync_count = 0;
#ifndef __386__
	old_isr = _dos_getvect(0x08);    /* snarf copy of vector */
	_dos_setvect(0x08, isr);         /* install our handler */
	_disable();
	write_timer(ticks_per_interrupt);
	_enable();
#endif
	return 0;  /* all's well */
	}

void vrl_TimerQuit(void)
	{
if (notimer) return;
	_disable();
	write_timer(0);   /* restore timer divisor to old value of zero */
	_enable();
#ifndef __386__
	if (old_isr)
		_dos_setvect(0x08, old_isr);   /* restore original handler */
#endif
	}

static vrl_Time count = 0;  /* notimer */

vrl_Time vrl_TimerRead(void)
	{
	vrl_Time t;
if (notimer) return count++;
	_disable();
	/* time right now is time in milliseconds since we initialized,
	   plus the number of timer ticks since the last interrupt (converted
	   to milliseconds) */
	t = milliseconds + (1000L*(ticks_per_interrupt - read_timer()))/TICKRATE;
	_enable();
	return t;
	}

void vrl_TimerDelay(vrl_Time millisecs)
	{
	vrl_Time start = vrl_TimerRead();
	vrl_Time end = start + millisecs;
	vrl_Time now;
if (notimer) return;
	do
		now = vrl_TimerRead();
	while (now >= start && now < end);
	}

void vrl_SetRetraceHandler(void (*function)(void))
	{
	retrace_handler = function;
	}

