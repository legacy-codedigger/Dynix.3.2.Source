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

/* $Header: ranf.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)ranf.c	4.2	(Berkeley)	5/27/83";
#endif not lint

# include	<stdio.h>

ranf(max)
int	max;
{
	register int	t;

	if (max <= 0)
		return (0);
	t = rand() >> 5;
	return (t % max);
}


double franf()
{
	double		t;
	t = rand() & 077777;
	return (t / 32767.0);
}
