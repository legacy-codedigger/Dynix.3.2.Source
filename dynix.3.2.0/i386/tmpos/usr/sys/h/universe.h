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
 * $Header: universe.h 2.1 87/04/03 $
 */

/*
 * $Log:	universe.h,v $
 */


/*
 * flags for universe definitions
 */


#define U_UCB	0
#define U_ATT	1
#define	U_NOACC	0x80		/* TEMP. Kludge to handle att_stat(dir).
				 * Will go away in next release. */
#define U_GET	-1
