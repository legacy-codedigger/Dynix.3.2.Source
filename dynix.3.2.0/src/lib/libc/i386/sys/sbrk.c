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

/* $Header: sbrk.c 2.1 86/03/19 $
 *
 * sbrk.c
 *	A real C-procedure to do sbrk.
 *
 * This does the same function as the VAX interface; in particular,
 * there is no test for negative incr.
 */

/*
 * $Log:	sbrk.c,v $
 */

sbrk(incr)
	int	incr;
{
	int	val;
	extern	char *_curbrk;

	val = (int)_curbrk;
	if (incr) { 	/* optimization for sbrk(0) */
		if (brk(_curbrk + incr) < 0)
			return(-1);
	}
	return(val);
}
