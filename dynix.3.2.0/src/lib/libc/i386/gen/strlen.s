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

	.file	"srtlen.s"
	.text
	.asciz	"$Header: strlen.s 1.5 86/06/19 $"

/*
 * strlen(str)
 *	char *str;
 *
 * Returns the number of non-NULL bytes in string argument.
 */

#include "DEFS.h"

ENTRY(strlen)
	ENTER
	movl	%edi,%edx		# save register
	movl	FPARG0,%edi		# string
	xorb	%al,%al			# match null in %al
	movl	$-1,%ecx		# large count
	repnz;	scab			# scan forward till null byte
	leal	2(%ecx),%eax		# eax = -len
	negl	%eax			# eax = len
	movl	%edx,%edi		# restore register
	EXIT
	RETURN
