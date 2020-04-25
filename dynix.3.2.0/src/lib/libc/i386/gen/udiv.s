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

	.file	"udiv.s"
	.text
	.asciz	"$Header: udiv.s 1.6 86/08/30 $"

/*
 * udiv(dividend, divisor)
 *	
 * Returns the unsigned quotient of 64 bit division.
 */

#include "DEFS.h"

#undef ENTRY
#ifdef PROF
#define	ENTRY(x)	.text ; .globl x; \
			.align 2; x:; \
			.data; Lcnt:; .long 0; .text; pushl %ebp; \
			movl %esp,%ebp; leal Lcnt,%eax; CALL mcount; leave
#else
#define	ENTRY(x)	.text; .globl x; \
			.align 2; x:
#endif

ENTRY(udiv)
	ENTER
	movl	FPARG0,%eax		# get dividend
	xorl	%edx,%edx		# clear upper dword
	divl	FPARG1			# perform divide
	EXIT
	RETURN
