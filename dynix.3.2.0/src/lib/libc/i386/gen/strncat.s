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

	.file	"strncat.s"
	.text
	.asciz	"$Header: strncat.s 1.5 86/07/10 $"

/*
 * strncat(s1, s2, n)
 *	char *s1, *s2;
 *
 * Concatenate s2 on the end of s1.  S1's space must be large enough.
 * At most n characters are moved (plus a null byte).
 * Return s1.
 */

#include "DEFS.h"

#define	STRNCAT \
	; slodb			/* 5, get byte, incr %esi */ 	\
	; sstob			/* 4, put byte, incr %edi */ 	\
	; testb	%al,%al		/* 2, end of string? */		\
	; jz	strncat_done	/* 3 or 7+M */			\
	; decl	%ecx		/* 2, decr count */		\
	; jz	strncat_zero	/* 3 or 7+M */

ENTRY(strncat)
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
	movl	FPARG2, %ecx		# get count
	testl	%ecx,%ecx		# zero count means no work to do
	jz	strncat_done

strncat_loop:
	STRNCAT				# 19 
	STRNCAT				# 19
	STRNCAT				# 19
	STRNCAT				# 19
	STRNCAT				# 19
	STRNCAT				# 19
	STRNCAT				# 19
	STRNCAT				# 19
	jmp	strncat_loop		# 8

strncat_zero:
	movb	$0,(%edi)		# write null byte
strncat_done:
	movl	FPARG0,%eax		# return s1
	popl	%edi
	popl	%esi
	EXIT
	RETURN
