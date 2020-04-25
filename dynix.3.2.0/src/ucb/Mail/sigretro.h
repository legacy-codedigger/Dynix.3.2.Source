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

/* $Header: sigretro.h 2.1 87/04/02 $ */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sigretro.h	5.1 (Berkeley) 6/6/85
 */

/*
 * Define extra stuff not found in signal.h
 */

#ifndef SIGRETRO

#define	SIGRETRO				/* Can use this for cond code */

#ifndef SIG_HOLD

#define	SIG_HOLD	(int (*)()) 3		/* Phony action to hold sig */
#define	BADSIG		(int (*)()) -1		/* Return value on error */

#endif SIG_HOLD

#endif SIGRETRO
