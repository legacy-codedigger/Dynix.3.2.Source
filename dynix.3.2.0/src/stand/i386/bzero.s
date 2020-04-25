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

#include "assym.h"

/* $Header: bzero.s 1.3 87/02/06 $
 *
 * bzero(base,count)
 *	char *base;
 *	unsigned count;
 *
 * Fast byte zero.  Smaller than libc's version.
 *
 */
	.text
	.globl	_bzero
	.align	2
_bzero:
	movl	%edi,%edx		/* save register */
	movl	SPARG0,%edi		/* base */
	movl	SPARG1,%ecx		/* length */
	shrl	$2,%ecx			/* number of longs */
	xorl	%eax,%eax		/* write zeros */
	rep;	sstol			/* clear double words */
	movb	SPARG1,%cl		/* low byte of length */
	andb	$3,%cl			/* number of bytes left */
	rep;	sstob			/* clear remaining bytes */
	movl	%edx,%edi		/* restore register */
	RETURN
