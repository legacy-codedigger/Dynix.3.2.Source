/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Header: sigvec.s 2.10 1992/03/13 21:13:38 $
 *
 * $Log: sigvec.s,v $
 *
 */

#include "SYS.h"

/*
 * sigcode(signo, code, scp)
 *	int signo;		signal number
 *	int code;		additional code to further distinguish different
 *				faults that map to the same signal.
 *	struct sigcontext *scp;	pointer to saved context (see signal.h)
 *
 * The following routine is entered when a signal occurs and a
 * user defined signal handler is to be called.
 *
 * Both the sigcontext and the sigframe structures (below) are pushed
 * onto the stack that the signal handler is going to run on at signal
 * delivery time.  The sigcontext structure is pushed on first, followed
 * by the sigframe structure.  The last three entries in the sigframe
 * structure (scp, code, signo) are the arguments to the signal handler.
 *
 *
 *						contents of sigframe structure
 *
 *							+----------+
 *							|	   |
 *							| fpasave  |
 *							|	   |
 *							+----------+
 *							|	   |
 * 	contents of sigcontext structure.		| fpusave  |
 *							|	   |
 *		+--------+				+----------+
 *		|   ip   |				| u_flags  |
 *		+--------+				+----------+
 *		| flags  |				| saved AX |
 *		+--------+ 				+----------+
 *		|   sp   |				| saved CX |
 *		+--------+ 				+----------+
 *		|  mask  |				| saved DX |
 *		+--------+				+----------+
 *		| onstack|<-----------------------------|   *scp   |
 *		+--------+				+----------+
 *							|   code   |
 *							+----------+
 *							|   signo  |
 *							+----------+
 *	During sigcleanup (after the user's signal handler has run),
 *	the pc and modpsr from the sigcontext structure is copied 
 *	onto the user stack that is going to be used after the signal
 *	handler has finished.  The sp entry in sigcontext point to the
 *	top of the stack where the pc and modpsr will be copied to.
 *	These will be used by __sigcode below to fake a kernel return.
 *
 *	sigcleanup has been replaced by sigreturn which uses the saved pc
 *	and flags from the original call.
 *
 * At entry, EAX contains actual signal handler address.
 */

	.globl	__sigcode
	.text
	.align	2
__sigcode:
	cld				/* assure direction flag correct */
	call	*%eax			/* call user defined signal handler */
	addl	$8, %esp		/* pop off signum and code */
	SVC(0,sigreturn)		/* 0-arg syscall */
/*
 * sig return no-longer returns here but uses the saved pc and flags
 * from the original trap.
 *
 * old code still calls sigcleanup.
 * Now on sigcontext stack.  Restore flags and return.
 */
	popfl				/* pop flags (condition codes, etc). */
	ret				/* and return from where signal came. */

/*
 * struct sigvec {
 *	int (*sv_handler)();
 *	int sv_mask;
 *	int sv_onstack;
 * };
 *
 * sigvec(sig, vec, ovec)
 *	int sig;
 *	struct sigvec *vec, *ovec;
 *
 * Setup software signal handler.
 */

ENTRY(sigvec)				/* standard entry... */
	pushl	$__sigcode		/* 4th arg = trampoline code address. */
	pushl	4+SPARG2		/* 3rd arg copy from caller. */
	pushl	8+SPARG1		/* 2nd arg copy from caller. */
	pushl	12+SPARG0		/* 1st arg copy from caller. */
	movl	%esp, %ecx		/* address of args. */
	SVC(4,sigvec)			/* 4-arg syscall. */
	jc	err			/* jump if error. */
	addl	$16, %esp		/* clear stack */
	ret				/* no error. */
err:
	addl	$16, %esp		/* clear stack */
	call	cerror
	ret				/* no error. */
