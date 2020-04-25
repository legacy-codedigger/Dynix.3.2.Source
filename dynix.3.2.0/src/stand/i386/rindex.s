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

/* $Header: rindex.s 1.2 87/02/06 $
 *
 * rindex(sp, c)
 *	char *sp, c;
 *
 * Return the ptr in sp at which the character c last
 * appears; NULL if not found.
 */
	.text
	.globl	_rindex
	.align	2
_rindex:
	pushl	%edi
	movl	$-1,%ecx		/* large count */
	movl	SPARG0,%edi		/* source is sp */
	movl	%edi,%edx		/* save source string address */
	xorb	%al,%al			/* match null in %al */
	repnz;	scab			/* scan forward till null byte */
	movl	%edi,%ecx		/* %edi == "match" address + 1 */
	subl	%edx,%ecx		/* length is "match"+1 minus "source" */
	decl	%edi			/* start looking at "match" */
	movb	SPARG1,%al		/* match character in %al */
	std				/* set direction flag (for backwards) */
	repnz;	scab			/* scan backwards looking for match */
	cld				/* unset direction flag (for forwards) */
	je	rindex_match		/* match on last character? */
	xorl	%eax,%eax		/* return NULL if count went to zero */
	jmp	rindex_ret
rindex_match:
	movl	%edi,%eax		/* "match" - 1 */
	incl	%eax			/* "match" */
rindex_ret:
	popl	%edi
	RETURN
