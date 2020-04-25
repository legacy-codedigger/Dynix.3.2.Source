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

	.file	"strncpy.s"
	.text
	.asciz	"$Header: strncpy.s 1.6 86/06/24 $"

/*
 * strncpy(s1, s2, n)
 *     char *s1, *s2
 *
 * Copy s2 to s1, truncating or null-padding to always copy n bytes
 * Return s1.
 */

#include "DEFS.h"

#define	STRNCPY \
	; slodb			/* 5, movb (%esi++), %al */ 	\
	; sstob			/* 4, movb %al, (%edi++) */ 	\
	; testb	%al,%al		/* 2, end of string? */		\
	; jz	strncpy_done	/* 3 or 7+M */			\
	; decl	%ecx		/* 2, decr count */		\
	; jz	strncpy_nowork	/* 3 or 7+M */

ENTRY(strncpy)
	ENTER
	pushl	%edi
	pushl	%esi
	movl	FPARG0,%edi	# destination
	movl	FPARG1,%esi	# source
	movl	FPARG2,%ecx	# count
	testl	%ecx,%ecx	# any work to do?
	jz	strncpy_nowork

strncpy_loop:
	STRNCPY			# 25
	STRNCPY			# 25
	STRNCPY			# 25
	STRNCPY			# 25
	STRNCPY			# 25
	STRNCPY			# 25
	STRNCPY			# 25
	STRNCPY			# 25
	jmp	strncpy_loop	# 8

strncpy_done:
	decl	%ecx
	xorb	%al,%al
	rep;	sstob		# zero padd

strncpy_nowork:
	movl	FPARG0,%eax	# return value
	popl	%esi
	popl	%edi
	EXIT
	RETURN
