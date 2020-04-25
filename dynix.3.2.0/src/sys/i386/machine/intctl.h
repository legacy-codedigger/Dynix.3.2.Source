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
 * $Header: intctl.h 2.19 90/11/08 $
 */

#ifndef	_SYS_INTCTL_H_

/* $Log:	intctl.h,v $
 */

#define	TMPOS_GROUP	1		/* SLIC group used by processors */
#define	SLICBINS	8		/* 8 bins in current SLIC */
#define	MSGSPERBIN	256		/* Interrupt vectors per bin */

/*
 * SPL masks for SLIC.
 */

#define SPL0	0xFF			/* all interrupts enabled */
#define	SPL1	0xFE			/* bins 1-7 enabled, 0 disabled */
#define	SPL2	0xFC			/* bins 2-7 enabled, 0-1 disabled */
#define	SPL3	0xF8			/* bins 3-7 enabled, 0-2 disabled */
#define	SPL_HOLE 0xF0			/* bins 4-7 enabled, 0-3 disabled */
#define	SPL4	0xE0			/* bins 5-7 enabled, 0-4 disabled */
#define	SPL5	0xC0			/* bins 6-7 enabled, 0-5 disabled */
#define	SPL6	0x80			/* bin    7 enabled, 0-6 disabled */
#define	SPL7	0x00			/* all interrupts disabled */

/*
 * Mnemonics for various implementation SPL's.
 * SPLFD chosen to insure proper nesting of spl's in ioctl's.
 */

#define	SPLNET	SPL1			/* block network SW interrupts */
#define SPLFD	SPL1			/* file-descriptor manipulation */
#define SPLTTY	SPL4			/* block tty interrupts */
#define	SPLBUF	SPL6			/* buffer-cache: all but clock(s) off */
#define	SPLSWP	SPL6			/* swap-buf headers: similarly */
#define	SPLFS	SPL6			/* inodes-list/file-sys: similarly */
#define	SPLIMP	SPL6			/* network devices, etc */
#define	SPLMEM	SPL6			/* memory list manipulation */
#define	SPLHI	SPL7			/* block all interrupts */

/*
 * Bit positions of Software Interrupts
 */

#define	NETINTR		0x01
#define	SWINT_TLB_FLUSH	0x02		/* for FlushTLB() */
#define	SOFTCLOCK	0x10
#define	RESCHED		0x80

/*
 * NMI Interrupt messages
 */

#define PAUSESELF	0x01

/*
 * Bin_header structure used for bin 1-7 interrupts.
 * We allocate one for bin0 for convenience, although it isn't used.
 *
 * locore.s assumes this data-structure is 8-bytes big.  If this
 * changes, *MUST* change locore.s (dev_common handler).
 */

struct	bin_header {
	int	bh_size;		/* # entries */
	int	(**bh_hdlrtab)();	/* real interrupt handlers */
};

extern	struct	bin_header int_bin_table[];
extern	int	bin_alloc[];
extern	int	bin_intr[];
extern	u_char	ivecall();

#define I486BUG3
#ifdef I486BUG3
/*
 * On steps of the i486 up to C0 (Beta), we must inhibit interrupts until
 * we know that the SLIC lmask timing window is closed.  Errata #3 for the
 * i486 states that if interrupt is presented to the i486, but is removed
 * before the i486 can generate interrupt acknowledge, the chip will behave
 * in an undefined fashion.  The only way this happens on Symmetry is when
 * the interrupt arrives as the SLIC lmask is written--the interrupt gets
 * droppped when the mask takes effect, potentially before the interrupt
 * is acknowledged.  By hard-masking interrupt on the chip, we cause the
 * i486 to ignore the interrupt line, avoiding the problem entirely.
 *
 * The files containing this workaround are: asm.h, intctl.h, and mc_mutex.h
 */
#define BLOCK_INTR pushfl; cli
#define ALLOW_INTR popfl
#endif /* I486BUG3 */

/*
 * The following interfaces (ivecres, ivecpeek, and ivecinit) may
 * be used by custom hardware configuration software to ease setting
 * up interrupt handling.
 * 
 * ivecres:	reserve interrupt vector slots.
 * ivecpeek:	peek at the next interrupt vector to be allocated
 * iveninit:	assign an interrupt handler to a vector
 */

#define ivecres(bin, count)	bin_intr[(bin)] += (count)
#define ivecpeek(bin)		bin_alloc[(bin)]
#define ivecinit(bin, vector, handler) \
	int_bin_table[(bin)].bh_hdlrtab[(vector)] = (handler)

/*
 * In-line spl interfaces -- mask some set of interrupts, return previous spl.
 *
 * All insure SLIC can't accept an interrupt once the "spl" is complete.
 * Must do a read to synch with the write of new SLIC mask, then pad by
 * 10 cycles (12 at 20 Mhz).  This insures that a pending SLIC interrupt
 * (or one accepted while the mask is being written) is taken before the
 * spl() completes.  Reading the slic local mask provides the write-synch.
 * The actual delay is set per-processor; a countdown delay loop is used.
 *
 * The basic time to fall straight through the loop is:
 *
 *	Instruction			i386	i486
 *	movb	(%ecx),%ah		4	1
 *	movl	DELAYPTR,%ecx		4	1
 *	movl	(%ecx),%ecx		4	1
 * 0:	subl	$1,%ecx			2	1
 *	jg	0b			3/10	1/3
 *	popl	%ecx			4	4
 *					---	---
 *					21	9
 *
 * Each further iteration of the subl..jg loop takes an addition 12 clocks
 * on an i386, and 4 clocks on an i486.
 */

#define	splnet	spl1
#define	splimp	spl6
#define	splhi	spl7			/* block all interrupts */

#if	!defined(GENASSYM) && !defined(lint)

asm spl0()
{
/PEEPOFF
	movl	_va_slic_lmask,%ecx		/* slic mask address */
	movb	(%ecx), %al			/* old interrupt mask */
	movb	$SPL0, (%ecx)			/* write new interrupt mask */
/PEEPON
}

/*
 * "plocal_slic_delay" is an alias for "l.slic_delay" and is setup
 * in "start.s".
 */
asm spl1()
{
/PEEPOFF
	movl	_va_slic_lmask, %ecx		/* slic mask address */
	movb	(%ecx), %al			/* old interrupt mask */
	pushfl
	cli
	movb	$SPL1, (%ecx)			/* write new mask */
	movb	(%ecx), %dl			/* sync write */
	movl	_plocal_slic_delay,%ecx
	movl	(%ecx),%ecx
0:	subl	$1,%ecx
	jg	0b
	ALLOW_INTR
/PEEPON
}

asm spl2()
{
/PEEPOFF
	movl	_va_slic_lmask, %ecx		/* slic mask address */
	movb	(%ecx), %al			/* old interrupt mask */
	pushfl
	cli
	movb	$SPL2, (%ecx)			/* write new mask */
	movb	(%ecx), %dl			/* sync write */
	movl	_plocal_slic_delay,%ecx
	movl	(%ecx),%ecx
0:	subl	$1,%ecx
	jg	0b
	ALLOW_INTR
/PEEPON
}

asm spl3()
{
/PEEPOFF
	movl	_va_slic_lmask, %ecx		/* slic mask address */
	movb	(%ecx), %al			/* old interrupt mask */
	pushfl
	cli
	movb	$SPL3, (%ecx)			/* +0: write new mask */
	movb	(%ecx), %dl			/* sync write */
	movl	_plocal_slic_delay,%ecx
	movl	(%ecx),%ecx
0:	subl	$1,%ecx
	jg	0b
	ALLOW_INTR
/PEEPON
}

asm spl4()
{
/PEEPOFF
	movl	_va_slic_lmask, %ecx		/* slic mask address */
	movb	(%ecx), %al			/* old interrupt mask */
	pushfl
	cli
	movb	$SPL4, (%ecx)			/* +0: write new mask */
	movb	(%ecx), %dl			/* sync write */
	movl	_plocal_slic_delay,%ecx
	movl	(%ecx),%ecx
0:	subl	$1,%ecx
	jg	0b
	ALLOW_INTR
/PEEPON
}

asm spl5()
{
/PEEPOFF
	movl	_va_slic_lmask, %ecx		/* slic mask address */
	movb	(%ecx), %al			/* old interrupt mask */
	pushfl
	cli
	movb	$SPL5, (%ecx)			/* +0: write new mask */
	movb	(%ecx), %dl			/* sync write */
	movl	_plocal_slic_delay,%ecx
	movl	(%ecx),%ecx
0:	subl	$1,%ecx
	jg	0b
	ALLOW_INTR
/PEEPON
}

asm spl6()
{
/PEEPOFF
	movl	_va_slic_lmask, %ecx		/* slic mask address */
	movb	(%ecx), %al			/* old interrupt mask */
	pushfl
	cli
	movb	$SPL6, (%ecx)			/* +0: write new mask */
	movb	(%ecx), %dl			/* sync write */
	movl	_plocal_slic_delay,%ecx
	movl	(%ecx),%ecx
0:	subl	$1,%ecx
	jg	0b
	ALLOW_INTR
/PEEPON
}

asm spl7()
{
/PEEPOFF
	movl	_va_slic_lmask, %ecx		/* slic mask address */
	movb	(%ecx), %al			/* old interrupt mask */
	pushfl
	cli
	movb	$SPL7, (%ecx)			/* +0: write new mask */
	movb	(%ecx), %dl			/* sync write */
	movl	_plocal_slic_delay,%ecx
	movl	(%ecx),%ecx
0:	subl	$1,%ecx
	jg	0b
	ALLOW_INTR
/PEEPON
}


/*
 * getspl()
 *	Return current spl mask.
 */

asm getspl()
{
/PEEPOFF
	movl	_va_slic_lmask, %ecx		/* slic mask address */
	movb	(%ecx), %al			/* old mask */
	andl	$0xff, %eax			/* clear upper 3 bytes */
/PEEPON
}
/*
 * splx() lowers interrupt mask to a previous value.  No write-synch or padding
 * necessary since mask is allowing more interrupts, not masking more.
 * DEBUG uses out-of-line version that tests mask is lowering.
 */

#ifndef	DEBUG
asm splx(oldspl)
{
%mem oldspl;
/PEEPOFF
	movl	oldspl, %eax			/* spl into KNOWN register */
	movl	_va_slic_lmask, %ecx		/* slic mask address */
	movb	%al, (%ecx)			/* write new mask */
/PEEPON
}
#endif /* DEBUG */

#endif	!GENASSYM && !lint
#define	_SYS_INTCTL_H_
#endif	/* _SYS_INTCTL_H_ */
