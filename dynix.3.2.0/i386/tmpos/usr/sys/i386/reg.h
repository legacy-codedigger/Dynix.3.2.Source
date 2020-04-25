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
 * $Header: reg.h 2.4 87/04/10 $
 *
 * reg.h
 *	Defines location of the users' stored registers relative to R0.
 *
 * Usage is u.u_ar0[XX].
 */

/* $Log:	reg.h,v $
 */

/*
 * All kernel entries are thru a trap/interrupt stack frame.
 * This is SW extended to save/restore registers, etc.
 *
 * The error code pushed on some traps is poped into l.trap_err_code
 * to make stack frame consistent.
 *
 * A push-all instruction is used to push all user general registers,
 * thus syscall() and trap() handler can have arbitrary call frames.
 *
 * Currently don't save/restore user DS,ES on entry/exit; rather assume
 * user code never changes these, and set to constants on kernel exit.
 * FS, GS are totally ignored.
 *
 * Once in syscall/trap handler, stack looks like:
 *
 *	old SS, padded		only if inter-segment (user->kernel)
 *	old SP			only if inter-segment (user->kernel)
 *	flags
 *	CS, padded		sense user-mode entry from RPL field
 *	EIP			return context
 *	EAX			scratch registers
 *	ECX			ditto
 *	EDX			more such
 *	EBX			register variable
 *	(unused)		temp SP (from push-all instruction)
 *	EBP			of interrupted frame
 *	ESI			register variable
 *	EDI			register variable
 *	trap-type		trap only
 *	EIP			back to locore
 *	XXX			syscall or trap call frame
 */

#define	SS	(5)
#define	ESP	(4)
#define	FLAGS	(3)
#define	CS	(2)
#define	EIP	(1)
#define	EAX	(0)
#define	ECX	(-1)
#define	EDX	(-2)
#define	EBX	(-3)
/*temp	SP	(-4) */		/* due to "pushal" instruction */
#define	EBP	(-5)
#define	ESI	(-6)
#define	EDI	(-7)

#define	SP	ESP		/* portable name for stack pointer offset */
#define	PC	EIP		/* portable name for program counter offset */

/*
 * Offsets from SP to registers that must be accessed in locore.
 * Assumes SP points to EDI in the above frame.
 */

#define	SP_EIP		(EIP-EDI)
#define	SP_CS		(CS-EDI)
#define	SP_EAX		(EAX-EDI)
#define	SP_FLAGS	(FLAGS-EDI)

/*
 * Interrupts save scratch registers in a consistent order with a push-all;
 * save entry SPL below this.  Thus, after interrupt entry, stack looks like:
 *
 *	old SS, padded		only if inter-segment (user->kernel)
 *	old SP			only if inter-segment (user->kernel)
 *	flags
 *	CS, padded		sense user-mode entry from RPL field
 *	IP			return context
 *	EAX			scratch registers
 *	ECX			ditto
 *	EDX			more such
 *	old SPL			entry SLIC local mask  (esp points here)
 */

#define	INTR_SP_CS	(5)	/* need to look at saved CS from above frame */
