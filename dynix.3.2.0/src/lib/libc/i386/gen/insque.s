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

	.file	"insque.s"
	.text
	.asciz	"$Header: insque.s 1.4 86/06/19 $"

/*
 * struct list {
 *	struct list *link;
 *	struct list *rlink;
 * }
 *
 * insque( entry, predecessor )
 *	struct list *entry, *predecessor;
 *
 * Insert entry into a doubly-linked list. Similar rules apply as in remque.
 */

#include "DEFS.h"

ENTRY(insque)
	ENTER
	movl	FPARG0,%eax	# entry
	movl	FPARG1,%ecx	# predecessor
	movl	0(%ecx),%edx
	movl	%edx,0(%eax)	# foward link of entry
	movl	%ecx,4(%eax)	# backward link of entry
	movl	%eax,4(%edx)	# backward link of successor
	movl	%eax,0(%ecx)	# forward link of predecessor
	EXIT
	RETURN
