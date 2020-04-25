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

#include "assym.h"

/* $Header: strcat.s 1.2 87/02/06 $
 *
 * strcat(s1, s2)
 *	char *s1, *s2;
 *
 * Concatenate s2 on the end of s1.  S1's space must be large enough.
 * Return s1.
 */

#define	STRCAT \
	; slodb			/* 5, get byte, incr %esi */ 	\
	; sstob			/* 4, put byte, incr %edi */ 	\
	; testb	%al,%al		/* 2, end of string? */		\
	; jz	strcat_done	/* 3 or 7+M */

	.globl	_strcat
	.align	2
_strcat:
	movl	%esi,%eax		/* save registers */
	movl	%edi,%edx
	movl	$-1,%ecx		/* large count */
	movl	SPARG0,%edi		/* s1 */
	movl	SPARG1,%esi		/* s2 */
	pushl	%eax			/* save register */
	xorb	%al,%al			/* match null in %al */
	repnz;	scab			/* scan forward till null byte */
	decl	%edi			/* backup to null byte */

strcat_loop:
	STRCAT				/* 14 */
	jmp	strcat_loop		/* 8 */

strcat_done:
	popl	%esi			/* restore registers	*/
	movl	%edx,%edi
	movl	SPARG0,%eax		/* s1 is return value */
	RETURN
