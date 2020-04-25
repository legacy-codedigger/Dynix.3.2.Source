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

#ident "$Header: asm.h 1.12 90/11/08 $" 

#ifndef	_SYS_ASM_H_

/*
 * asm.h
 *	Standard ASM use definitions.  i386 version.
 *
 * assym.h contains structure offsets and the like, generated by genassym.c
 */

/* $Log:	asm.h,v $
 */

/*
 * ENTRY() declares a text .globl and aligns the start of the procedure.
 */

#define	ENTRY(x)	.text; .globl _/**/x; .align 2; _/**/x:

/*
 * BP relative stack arguments for asm procedures that obey C calling
 * convention, establishing a stack frame.
 */

#define	BPARGOFF 8
#define	BPARG0	BPARGOFF+0(%ebp)
#define	BPARG1	BPARGOFF+4(%ebp)
#define	BPARG2	BPARGOFF+8(%ebp)
#define	BPARG3	BPARGOFF+12(%ebp)

/*
 * SP relative stack arguments for asm procedures that don't "enter".
 */

#define	SPARGOFF 4
#define	SPARG0	SPARGOFF+0(%esp)
#define	SPARG1	SPARGOFF+4(%esp)
#define	SPARG2	SPARGOFF+8(%esp)
#define	SPARG3	SPARGOFF+12(%esp)

/*
 * Call, return "instructions", in case these need to change.
 */

#define	CALL	call
#define	RETURN	ret

#if defined(SLIC_GATES)

/*
 * In-line ASM macros for portable gate and lock implementations.
 * Macros don't block interrupts; caller must do this.
 *
 * P_GATE_ASM	grab a gate, used in ASM code.
 * CP_GATE_ASM	conditionally grab a gate, used in ASM code.
 * V_GATE_ASM	release a gate in ASM code.
 *
 * These macros touch only EAX.
 */

/*
 * P_SLIC_GATE and V_SLIC_GATE lock/unlock slic gate whose number is in %al.
 * Since these may be called from (eg) p_gate() and cp_gate(), the must be
 * sure to save/restore interrupt enable flag.  This is ok in the prototype
 * (K20), and a non-issue on the real HW.
 */

#define	P_SLIC_GATE(gateno) \
	pushl	%ecx; \
	movl	_va_slic, %ecx; \
	pushfl; \
	cli; \
	movb	$GATE_GROUP, SL_DEST(%ecx); \
	movb	gateno, SL_SMESSAGE(%ecx); \
9:	movb	$SL_REQG, SL_CMD_STAT(%ecx); \
8:	testb	$SL_BUSY, SL_CMD_STAT(%ecx); \
	jne	8b; \
	testb	$SL_OK, SL_CMD_STAT(%ecx); \
	je	9b; \
	popl	$ecx

#define	V_SLIC_GATE(gateno) \
	pushl	%ecx; \
	movl	_va_slic, %ecx; \
	movb	$GATE_GROUP, SL_DEST(%ecx); \
	movb	gateno, SL_SMESSAGE(%ecx); \
	movb	$SL_RELG, SL_CMD_STAT(%ecx); \
9:	testb	$SL_BUSY, SL_CMD_STAT(%ecx); \
	jne	9b; \
	popl	%ecx; \
	popfl

#define	P_GATE_ASM(gaddr) \
	leal	gaddr, %eax; \
	shrb	$2, %al; \
	andb	$0x3f, %al; \
5:	cmpb	$G_UNLOCKED, gaddr; \
	jne	5b; \
	P_SLIC_GATE(%al); \
	movb	gaddr, %ah; \
	cmpb	$G_UNLOCKED, %ah; \
	jne	6f; \
	movb	$G_LOCKED, gaddr; \
6:	V_SLIC_GATE(%al); \
	cmpb	$G_UNLOCKED, %ah; \
	jne	5b

#define	CP_GATE_ASM(gaddr,fail) \
	leal	gaddr, %eax; \
	shrb	$2, %al; \
	andb	$0x3f, %al; \
	P_SLIC_GATE(%al); \
	movb	gaddr, %ah; \
	cmpb	$G_UNLOCKED, %ah; \
	jne	6f; \
	movb	$G_LOCKED, gaddr; \
6:	V_SLIC_GATE(%al); \
	cmpb	$G_UNLOCKED, %ah; \
	jne	fail

#define	V_GATE_ASM(gaddr)	movb	$G_UNLOCKED, gaddr

#else	/* Real HW */

/*
 * Gate and lock accesses on real HW are in-line expanded.
 * See machine/mutex.h
 */

#define	V_GATE_ASM(gaddr) \
	movb	$G_UNLOCKED, %al; \
	xchgb	%al, gaddr

#endif	/* SLIC_GATES */

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
#define ALLOW_INTR popfl
#endif /* I486BUG3 */

/*
 * V_LOCK_ASM() assumes gate and lock data-structure and values are idential.
 */

#define	V_LOCK_ASM(lock)	V_GATE_ASM(lock)

/*
 * SPL_ASM(new,old)	raise SPL to "new", put old value in "old" (mod's %ah).
 * SPLX_ASM(old)	lower SPL back to "old".
 *
 * See intctl.h for detail on spl synch with SLIC.
 * need 8 clocks at 16Mhz 10 at 20Mhz 12 at 24Mhz etc
 * Model-C requires an extra 50 ns.
 * Machine may be ran at 10% margins making
 * need 9 clocks at 16Mhz 12 at 20Mhz 15 at 24Mhz etc
 *
 * The basic time to fall straight through the loop is:
 *
 *	Instruction			i386	i486
 *	movb	(%ecx),%ah		4	1
 *	movl	PLOCAL,%ecx		4	1
 * 0:	subl	$1,%ecx			2	1
 *	jg	0b			3/10	1/3
 *	popl	%ecx			4	4
 *					---	---
 *					17	8
 *
 * Each further iteration of the subl..jg loop takes an addition 12 clocks
 * on an i386, and 4 clocks on an i486.
 */

#define	SPL_ASM(new,old) \
	pushl	%ecx; \
	movl	_va_slic_lmask, %ecx; \
	movb	(%ecx), old; \
	pushfl; \
	cli; \
	movb	new, (%ecx); \
	movb	(%ecx), %ah; \
	movl	VA_PLOCAL+L_SLIC_DELAY,%ecx; \
0:	subl	$1,%ecx; \
	jg	0b; \
	ALLOW_INTR; \
	popl	%ecx

#define	SPLX_ASM(old) \
	movl	old, %eax;			/* spl into KNOWN register */\
	pushl	%ecx;				/* save %ecx */\
	movl	_va_slic_lmask, %ecx;		/* slic mask address */\
	movb	%al, (%ecx);			/* write new mask */\
	popl	%ecx;				/* restore saved %ecx */\

#define	_SYS_ASM_H_
#endif	/* _SYS_ASM_H_ */