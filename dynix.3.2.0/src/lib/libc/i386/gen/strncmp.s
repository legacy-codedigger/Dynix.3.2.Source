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

	.file	"strncmp.s"
	.text
	.asciz	"$Header: strncmp.s 1.6 86/06/23 $"

/*
 * strncmp(s1, s2, n)
 *	char *s1, *s2;
 *
 * Compare strings (at most n bytes):  
 * returns  s1>s2: >0,  s1==s2: 0,  s1<s2: <0
 */

#include "DEFS.h"

#define STRNCMP \
	; slodb			/* 5, movb (%esi++),%al */ 	\
	; scab			/* 7, cmpb %al,(%edi++) */	\
	; jne	strncmp_failed	/* 3 or 7+M, still equal? */	\
	; testb	%al,%al		/* 2, null byte? */		\
	; jz	strncmp_null	/* 3 or 7+M */			\
	; decl	%ecx		/* 2 */				\
	; jz	strncmp_zero	/* 3 or 7+M */

ENTRY(strncmp)
	ENTER
	pushl	%esi
	pushl	%edi
	movl	FPARG0,%esi
	movl	FPARG1,%edi
	movl	FPARG2,%ecx
	xorl	%eax,%eax	# clear return value
	testl	%ecx,%ecx	# length of zero returns zero
	jz	strncmp_zero

strncmp_loop:
	STRNCMP			# 25
	STRNCMP			# 25
	STRNCMP			# 25
	STRNCMP			# 25
	STRNCMP			# 25
	STRNCMP			# 25
	STRNCMP			# 25
	STRNCMP			# 25
	jmp	strncmp_loop	# 8

strncmp_zero:
	xorl	%eax,%eax
	popl	%edi
	popl	%esi
	EXIT
	RETURN

strncmp_failed:
	movzbl	-1(%edi),%ecx
	subl	%ecx,%eax	# *s1 - *s2

strncmp_null:
	popl	%edi
	popl	%esi
	EXIT
	RETURN
