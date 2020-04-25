
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

/* 
 * $Header: microtask.h 1.4 90/05/16 $
 *
 * microtask.h.h
 *	Definitions for use in microtasked parallel, shared-memory programs.
 */

/* 
 * $Log:	microtask.h,v $
 */

/*
 * Useful constants for microtasker
 */
#define	MAXPROCS	64			/* max # processes */
#define	MAXARGS		128			/* max # args for a m_fork() */
#define	MASTER		0			/* procid[] index of master */

/*
 * Special in-line assembly to correctly implement m_single/m_multi with
 * optimizer turned on.
 */
#ifndef lint
#ifndef _HIDE_NEW
#if i386
asm int m_single()
{
%lab parent;
/PEEPOFF
	call    __x_m_single
	orl     %eax,%eax
	je      parent
	cmpl    $-1, %eax
	je      parent
	jmp     *%eax
parent:
/PEEPON
}

extern long _x_m_single();		/* internal version of m_single() */
#endif /* i386 */
#endif /* _HIDE_NEW */
#endif /* not lint */

extern shared int m_numprocs;		/* number of "worker" processes */
extern private int m_myid;		/* my process identifier */
