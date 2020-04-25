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

/* $Header: gsp.s 1.3 87/09/25 $
 *
 * gsp(start,dest,length,pc)
 *	unsigned length;
 *	char *entry,*start,*pc;
 *
 * Magic routine to string move on top of ourselves and still jump to pc.
 * Works because of the prefetch queue on the 386.
 */
	.text
	.globl	_gsp
	.align	2
_gsp:
	popl	%eax			/* drop old return pc */
	popl	%esi			/* start */
	popl	%edi			/* destination */
	popl	%ecx			/* length */
	popl	%eax			/* real return pc */
	.align	2			/* so next 4 bytes in 1 prefetch */
	rep;	smovb			/* does the magic */
	jmp	*%eax			/* use code in prefetch queue */
	/*NOTREACHED*/
