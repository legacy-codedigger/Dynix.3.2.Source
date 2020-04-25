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

	.file	"bzero.s"
	.text
	.asciz	"$Header: bzero.s 1.4 86/06/19 $"

/*
 * bzero(base, length)
 *	char *base;
 *	unsigned int length;
 *
 * Fast clear of memory.
 *
 */

#include "DEFS.h"

ENTRY(bzero)
	ENTER
	movl	%edi,%edx	# save register
	movl	FPARG0,%edi	# base
	movl	FPARG1,%ecx	# length
	shrl	$2, %ecx	# # of double words
	xorl	%eax,%eax	# clear write register
	rep;	sstol		# write double words
	movb	FPARG1,%cl	# ECX = low byte of length
	andb	$3, %cl		# # bytes left to move.
	rep;	sstob		# clear remaining bytes.
	movl	%edx,%edi	# restore register
	EXIT
	RETURN
