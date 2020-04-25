/* $Copyright:	$
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

#include "assym.h"

/* $Header: bcmp.s 1.2 87/02/06 $
 *
 * bcmp(s1, s2, len)
 *	char *s1, *s2;
 *	unsigned int len;
 *
 * Fast machine byte comparison
 *
 */
	.text
	.globl	_bcmp
	.align	2
_bcmp:
	movl	%edi,%eax	/* save registers */
	movl	%esi,%edx
	movl	SPARG0,%esi	/* s1 */
	movl	SPARG1,%edi	/* s2 */
	movl	SPARG2,%ecx	/* length */
	testl	%ecx,%ecx	/* any work to do? */
	jz	bcmp_count	/* nope */
	repz;	scmpb		/* do byte by byte comparison */
	jz	bcmp_count	/* quit due to count or cmp? */
	incl	%ecx		/* compare failed so adjust count */
bcmp_count:
	movl	%edx,%esi	/* restore registers */
	movl	%eax,%edi
	movl	%ecx,%eax	/* return value is count */
	RETURN
