/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.
 * Copyright (c) 1984, 1985, 1986, 1987 Sequent Computer Systems, Inc.
 * All rights reserved
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*
 * yanked from libm to add switch to extended precision
 * once libm is fixed this should be also.  Also cleaned
 * up for the 387 (was fitting 287 restrictions)
 * tigger -- Thu May 10 08:36:02 PDT 1990
 *
 *	AT&T matherr handling added by garyg July 12, 1990
 *	so we can use this in both universes.
 */
#include "PROF.h"

ONED0:	.long	0, 0x3ff00000		# 1.0d0

ecode:	.long	6

ENTRY(exp)
	fldl	4(%esp)			/* load argument */
	subl	$12,%esp
	fstcw	(%esp)			/* save old status, masks */
	movl	$0x0f3f,4(%esp) /* 80 bit round towards zero, no exceptions */
	fldcw	4(%esp)	
	fclex				/* clear any old exceptions */

	fldl2e				# Load log2(e)
	fmulp				# Multiply by value
	fld	%st(0)			# Duplicate X'
	frndint				# Get nearest integer
	fxch	%st(1)			# Swap st(1) with st(0)
	fsub	%st(1), %st		# integer part of X & fractional part of X
	f2xm1				# Compute EXP(X'')-1.0
	faddl	ONED0			# Now EXP(X'')
	fscale				# Now EXP(X)
	fstp	%st(1)		/* return result in stack-like manner */
/*
 *	check for overflow
 */
	fstl	4(%esp)		/* does it fit in double precision? */
	fstsw	4(%esp)
	testb	$0x8,4(%esp)	/* just check overflow */
	jne	overflow

	fldcw	(%esp)			/* restore precision control */
	addl	$12,%esp		/* clear control words from stack */
	ret

overflow:
	movl	%ebp,8(%esp)		/* belately build frame */
	leal	8(%esp),%ebp
	fstp	%st			/* clean up stack */
	fclex
	fldcw	(%esp)			/* restore precision control */
	pushl	$ecode
	leal	4(%ebp), %eax
	pushl	%eax
	call	_MATHERR
	addl	$8, %esp
	leave
	ret
