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

/* $Id: tmps.h,v 1.1 88/09/02 11:48:31 ksb Exp $ */

/*
 * The following structure is used
 * to keep track of the amount of variable
 * storage required by each block.
 * "Max" is the high water mark, "off"
 * the current need. Temporaries for "for"
 * loops and "with" statements are allocated
 * in the local variable area and these
 * numbers are thereby changed if necessary.
 *
 * for the compiler,
 *	low_water is the lowest number register allocated of its type
 *	next_avail is the next available register of its type
 */

#if defined(PC)
#if defined(i386)
    /*
     *	the number of register types.
     *	the details of how many of each kind of register there is
     *	(and what they are for) is known in tmps.c
     */
#define	NUMREGTYPES	2
#define	REG_DATA	0
#define	REG_FLPT	1
#endif /* i386 */
#endif PC

struct om {
	long	om_max;
#if defined(PC)
	long	low_water[NUMREGTYPES];
#endif PC
	struct tmps {
	    long	om_off;
#if defined(PC)
	    long	next_avail[NUMREGTYPES];
#endif PC
	}	curtmps;
} sizes[DSPLYSZ];

    /*
     *	an enumeration for whether a temporary can be a register.  cf. tmps.c
     */
#define NOREG 0
#define REGOK 1
