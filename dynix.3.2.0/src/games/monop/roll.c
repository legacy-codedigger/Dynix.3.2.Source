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
 *	This routine rolls ndie nside-sided dice.
 */

# define	reg	register

# if !defined(vax) && !defined(ns32000) && !defined(i386)
# define	MAXRAND	32767L

roll(ndie, nsides)
int	ndie, nsides; {

	reg long	tot;
	reg unsigned	n, r;

	tot = 0;
	n = ndie;
	while (n--)
		tot += rand();
	return (int) ((tot * (long) nsides) / ((long) MAXRAND + 1)) + ndie;
}

# else

roll(ndie, nsides)
reg int	ndie, nsides; {

	reg int		tot, r;
	reg double	num_sides;

	num_sides = nsides;
	tot = 0;
	while (ndie--)
		tot += (r = rand()) * (num_sides / 017777777777) + 1;
	return tot;
}
# endif
