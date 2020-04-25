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

	.file	"index.s"
	.text
	.asciz	"$Header: index.s 1.4 86/06/19 $"

/*
 * index(sp, c)
 *	char *sp, c;
 *
 * Return the ptr in sp at which the character c appears;
 * NULL if not found.
 *
 */

#include "DEFS.h"

#define	INDEX \
	; slodb			/* 5, movb (%esi++),%al */	\
	; cmpb	%al,%cl		/* 2 */				\
	; je	index_match	/* 3 or 7+M */			\
	; testb	%al,%al		/* 2 */				\
	; jz	index_null	/* 3 or 7+M */

ENTRY(index)
	ENTER
	movl	%esi,%edx	# save register
	movl	FPARG0,%esi	# ptr to string
	movb	FPARG1,%cl	# char to look for

index_loop:
	INDEX			# 15
	INDEX			# 15
	INDEX			# 15
	INDEX			# 15
	INDEX			# 15
	INDEX			# 15
	INDEX			# 15
	INDEX			# 15
	jmp	index_loop	# 8

index_null:
	xorl	%esi,%esi	# return NULL if not found
	incl	%esi
index_match:
	movl	%esi,%eax
	decl	%eax
	movl	%edx,%esi	# restore register
	EXIT
	RETURN
