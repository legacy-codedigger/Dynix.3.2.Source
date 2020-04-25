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


/*
 * $Header: machine.h 2.0 86/01/28 $
 *
 * based on:
 *	version 1.4 of 12/6/82
 *	@(#)machine.h	1.4	(National Semiconductor)	12/6/82
 * 
 * $Log:	machine.h,v $
 */

/* operation type for ptrace and remotemachine access */

#define INIT 0
#define RMEM 1
#define RREG 3
#define WMEM 4
#define WREG 6
#define STEP 9
#define GOGO 7
#define KILL 8
#define GONT 10

#define	SIGBPT	999	/* from GENIX -- faked here */
