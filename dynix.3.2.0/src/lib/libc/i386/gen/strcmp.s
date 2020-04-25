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

	.file	"strcmp.s"
	.text
	.asciz	"$Header: strcmp.s 1.5 86/06/19 $"

/*
 * strcmp(s1, s2)
 *	char *s1, *s2;
 *
 * Compare strings:  s1>s2: >0, s1==s2: 0, s1<s2: <0
 */

#include "DEFS.h"

#define STRCMP \
	; slodb			/* 5, movb (%esi++), %al */ 	\
	; scab			/* 7, compare %al, (%edi++) */	\
	; jne	strcmp_failed	/* 3 or 7+M, still equal? */	\
	; testb	%al,%al		/* 2, null byte? */		\
	; jz	strcmp_null	/* 3 or 7+M */


ENTRY(strcmp)
	ENTER
	pushl	%esi
	pushl	%edi
	movl	FPARG0,%esi
	movl	FPARG1,%edi
	xorl	%eax,%eax

strcmp_loop:
	STRCMP			# 20
	STRCMP			# 20
	STRCMP			# 20
	STRCMP			# 20
	STRCMP			# 20
	STRCMP			# 20
	STRCMP			# 20
	STRCMP			# 20
	jmp	strcmp_loop	# 8

strcmp_failed:
	movzbl	-1(%edi),%ecx
	subl	%ecx,%eax	# *s1 - *s2

strcmp_null:
	popl	%edi
	popl	%esi
	EXIT
	RETURN
