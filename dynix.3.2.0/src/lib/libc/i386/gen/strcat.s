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

	.file	"strcat.s"
	.text
	.asciz	"$Header: strcat.s 1.4 86/06/19 $"

/*
 * strcat(s1, s2)
 *	char *s1, *s2;
 *
 * Concatenate s2 on the end of s1.  S1's space must be large enough.
 * Return s1.
 */

#include "DEFS.h"

#define	STRCAT \
	; slodb			/* 5, get byte, incr %esi */ 	\
	; sstob			/* 4, put byte, incr %edi */ 	\
	; testb	%al,%al		/* 2, end of string? */		\
	; jz	strcat_done	/* 3 or 7+M */

ENTRY(strcat)
	ENTER
	pushl	%esi
	pushl	%edi
	xorl	%ecx,%ecx
	decl	%ecx			# large count
	movl	FPARG0,%edi		# s1
	movl	FPARG1,%esi		# s2
	xorb	%al,%al			# match null in %al

	repnz;	scab			# scan forward till null byte

	decl	%edi			# backup to null byte

strcat_loop:
	STRCAT				# 14
	STRCAT				# 14
	STRCAT				# 14
	STRCAT				# 14
	STRCAT				# 14
	STRCAT				# 14
	STRCAT				# 14
	STRCAT				# 14
	jmp	strcat_loop		# 8

strcat_done:
	movl	FPARG0,%eax		# s1 is return value
	popl	%edi
	popl	%esi
	EXIT
	RETURN
