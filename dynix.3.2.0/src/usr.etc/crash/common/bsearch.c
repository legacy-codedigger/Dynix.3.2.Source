/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*	$Header: bsearch.c 2.2 1991/05/18 00:31:37 $@(#)bsearch.c	1.5	*/

#ifdef BSD	/* System V has bsearch in libc */
#ifndef lint
static char rcsid[] = "$Header: bsearch.c 2.2 1991/05/18 00:31:37 $";
#endif

char *
bsearch(key, base, nel, width, compar)
char   *key;			/* Key to be located */
char   *base;			/* Beginning of table */
unsigned nel;			/* Number of elements in the table */
unsigned width;			/* Width of an element (bytes) */
int	(*compar)();		/* Comparison function */
{
	int two_width = width + width;
	char *last = base + width * (nel - 1); /* Last element in table */

	while (last >= base) {

		register char *p = base + width * ((last - base)/two_width);
		register int res = (*compar)(key, p);

		if (res == 0)
			return (p);	/* Key found */
		if (res < 0)
			last = p - width;
		else
			base = p + width;
	}
	return ((char *) 0);		/* Key not found */
}
#endif
