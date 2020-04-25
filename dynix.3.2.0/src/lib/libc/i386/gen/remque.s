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

	.file	"remque.s"
	.text
	.asciz	"$Header: remque.s 1.4 86/06/19 $"

/*
 * struct list {
 *	struct list *link;
 *	struct list *rlink;
 * }
 *
 * remque(linkptr)
 *	struct list *linkptr;
 *
 * Unlink item from doubly-linked list. Assumes forward pointer is first item
 * in structure. Back pointer is second item.
 */

#include "DEFS.h"

ENTRY(remque)
	ENTER
	movl	FPARG0,%eax
	movl	4(%eax),%ecx		# %ecx has predecessor
	movl	0(%eax),%edx
	movl	%edx,0(%ecx)		# forward link of predecessor
	movl	0(%eax),%ecx		# %ecx has successor
	movl	4(%eax),%edx
	movl	%edx,4(%ecx)		# backward link of successor
	EXIT
	RETURN
