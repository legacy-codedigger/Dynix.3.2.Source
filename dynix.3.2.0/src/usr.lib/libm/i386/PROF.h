/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 *
 * $Header: PROF.h 1.5 91/01/16 $
 *
 * PROF.h
 *	Defines used to allow profiling in assembly files.
 *
 */

#ifndef	ALIGN
#define	ALIGN	.align	2
#define	STRING	.asciz
#endif

#ifdef MCOUNT
#define	ENTRY(x)	.text ; .globl _/**/x; .align 2; _/**/x: ; \
			.data; 1:; .long 0; .text; leal 1b,%eax; \
			pushl %ebp; movl %esp,%ebp; call mcount; leave
#else
#define	ENTRY(x)	.text ; .globl _/**/x; .align 2; _/**/x:
#endif
