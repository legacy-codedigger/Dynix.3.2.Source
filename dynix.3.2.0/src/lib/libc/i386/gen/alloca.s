/*
 * $Copyright:	$
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

	.file	"alloca.s"
	.text
	.asciz	"$Header: alloca.s 1.3 87/07/08 $"

#include "DEFS.h"

/*
 * Allocate local stack storage space allocating bytes
 * requested on the stack plus room to copy all possible
 * saved registers.  This allows the C compiler to assume
 * %esp is constant within functions and to use popl instead
 * of movl to restore registers when returning from functions.
 *
 * THIS IS HIGHLY MACHINE AND COMPILER DEPENDENT
 */
#define	FPU_REGS    6 * 8	/* compiler saves up to 6 double regs */
#define	USR_REGS    3 * 4	/* compiler saves up to 3 general regs */
#define	REG_SPACE   USR_REGS + FPU_REGS

ENTRY(alloca)
	popl	%ecx		/* return address */
	popl	%edx		/* requested amount */
	movl	%esp,%eax	/* get current stack pointer */
	subl	%edx,%eax	/* subrtact off requested amount */
	andb	$-4,%eax 	/* 4-byte align */
	/*
	 * Allocate additional space for a copy of all possible registers
	 * plus 4 bytes for caller to discard.
	 */
	subl	$REG_SPACE+4,%eax
	movl	%esp, %edx
	movl	%eax,%esp

	movl	0*4(%edx),%eax; 	movl	%eax,1*4(%esp)
	movl	1*4(%edx),%eax; 	movl	%eax,2*4(%esp)
	movl	2*4(%edx),%eax; 	movl	%eax,3*4(%esp)
	movl	3*4(%edx),%eax; 	movl	%eax,4*4(%esp)
	movl	4*4(%edx),%eax; 	movl	%eax,5*4(%esp)
	movl	5*4(%edx),%eax; 	movl	%eax,6*4(%esp)
	movl	6*4(%edx),%eax; 	movl	%eax,7*4(%esp)
	movl	7*4(%edx),%eax; 	movl	%eax,8*4(%esp)
	movl	8*4(%edx),%eax; 	movl	%eax,9*4(%esp)
	movl	9*4(%edx),%eax; 	movl	%eax,10*4(%esp)
	movl	10*4(%edx),%eax; 	movl	%eax,11*4(%esp)
	movl	11*4(%edx),%eax; 	movl	%eax,12*4(%esp)
	movl	12*4(%edx),%eax; 	movl	%eax,13*4(%esp)
	movl	13*4(%edx),%eax; 	movl	%eax,14*4(%esp)
	movl	14*4(%edx),%eax; 	movl	%eax,15*4(%esp)

#if REG_SPACE != (15*4)
	ERROR -- register save area out of date !!!
	.ABORT
#endif

	movl	%esp,%eax
	addl	$REG_SPACE+4,%eax	/* return value */
	jmp	*%ecx			/* return through a register */
