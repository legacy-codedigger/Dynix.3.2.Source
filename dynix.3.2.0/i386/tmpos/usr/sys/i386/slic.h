/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*
 * $Header: slic.h 2.9 90/11/08 $
 *
 * This defines the offsets for the slic addresses and the base
 * address for accessing it.
 */

/* $Log:	slic.h,v $
 */

#ifndef __CHIPTYPES__
#include "../machine/chiptypes.h"
#endif

#ifdef	i386
#define LOAD_CPUSLICADDR	PHYS_SLIC	/* i386 uses virt==phys */
#endif	i386

#if defined(ns32000) && (CPU_TYPE == 32532)
#define LOAD_CPUSLICADDR	PHYS_SLIC	/* 532 uses virt==phys */
#endif ns32000

#if defined(ns32000) && (CPU_TYPE != 32532)
#define LOAD_CPUSLICADDR	0x0FF0000	/* 032 boot/loader uses this */
#endif	ns32000

struct	cpuslic {
	u_char	sl_cmd_stat,	d0[3];	/* RW W: command, R: status */
	u_char	sl_dest,	d1[3];	/* W */
	u_char	sl_smessage,	d2[3];	/* W   send message data */
	u_char	sl_b0int,	d3[3];	/* R   bin 0 interrupt */
	u_char	sl_binint,	d4[3];	/* RW  bin 1-7 interrupt */
	u_char	sl_nmiint,	d5[3];	/* R   NMI interrupt */
	u_char	sl_lmask,	d6[3];	/* RW  local interrupt mask */
	u_char	sl_gmask,	d7[3];	/* R   group interrupt mask */
	u_char	sl_ipl,		d8[3];	/* RW  interrupt priority level */
	u_char	sl_ictl,	d9[3];	/* RW  interrupt control */
	u_char	sl_tcont,	d10[3];	/* RW  timer contents */
	u_char	sl_trv,		d11[3];	/* RW  timer reload value */
	u_char	sl_tctl,	d12[3];	/* W   timer control */
	u_char	sl_sdr,		d13[3];	/* R   slave data register */
	u_char	sl_procgrp,	d14[3];	/* RW  processor group */
	u_char	sl_procid,	d15[3];	/* RW  processor id */
	u_char	sl_crl,		d16[3];	/* R   chip revision level */
};

#define	NUMGATES	64		/* number of slic gates */

/*
 * MAX_NUM_SLIC is the maximum number of different slic addresses possible.
 * Slic addresses are 0 thru MAX_NUM_SLIC-1.
 */

#define	MAX_NUM_SLIC	64

/* Commands: */
#define	SL_MINTR	0x10	/* transmit maskable interrupt */
#define	SL_INTRACK	0x20	/* interrupt acknowledge */
#define	SL_SETGM	0x30	/* set group interrupt mask */
#define	SL_REQG		0x40	/* request Gate */
#define	SL_RELG		0x50	/* release Gate */
#define	SL_NMINTR	0x60	/* transmit non-maskable interrupt */
#define	SL_RDDATA	0x70	/* read slave data */
#define	SL_WRDATA	0x80	/* write slave data */
#define	SL_WRADDR	0x90	/* write slave I/O address */

/* Returned command status: */
#define	SL_BUSY		0x80	/* SLIC busy */
#define	SL_GATEFREE	0x10	/* Gate[send_message_data] free */
#define	SL_WRBE		0x08	/* Processor write buffer empty */
#define	SL_PARITY	0x04	/* parity error during SLIC message */
#define	SL_EXISTS	0x02	/* destination SLIC's exist */
#define	SL_OK		0x01	/* command completed ok */

/* Destination id's */
#define	SL_GROUP	0x40
#define	SL_ALL		0x3F

/* Interrupt control */
#define	SL_HARDINT	0x80	/* hardware interrupts accepted */
#define	SL_SOFTINT	0x40	/* software interrupts accepted */
#define	SL_MODEACK	0x01	/* interrupt acknowledge mode */

#define	SL_GM_ALLON	0xFF	/* Group Mask all enabled */

/* Timer interrupts */
#define	SL_TIMERINT	0x80	/* enable timer interrupts */
#define	SL_TIM5MHZ	0x08	/* decrement timer at 5 MHz */
#define	SL_TIMERBIN	0x07	/* interrupt bin mask of timer */
#define	SL_TIMERFREQ	10000	/* counts per second */
#define	SL_TIMERDIV	1000	/* system clock divisor for one clock count */

/* Processor ID */
#define	SL_TESTM	0x80	/* enable test mode */
#define	SL_PROCID	0x3F	/* processor ID mask */

/* Chip version stuff */
#define	SL_VENDOR	0xE0	/* vendor number */
#define	SL_RELEASE	0x1C	/* release number */
#define	SL_STEPPING	0x03	/* step number */

/*
 * "va_slic" holds current virtual-address of SLIC for various functions
 * that access it both before and after final mapping is in place.
 */
extern	struct	cpuslic	*va_slic;

/* 
 * SLICPRI() macro programs current execution priority into SLIC for
 * interrupt arbitration.  Argument is runQ # (eg, 0-31); thus, we
 * shift by 3 bits to get this into the writable portion of the register.
 */

#define	SLICPRI(p)	va_slic->sl_ipl = (p) << 3
