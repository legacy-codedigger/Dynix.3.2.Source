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

/* $Header: factor.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)factor.c	4.1 (Wollongong) 6/13/83";
#endif

/*
 *              factor [ number ]
 *
 * Written to replace factor.s in Bell V7 distribution
 */

main(argc, argv)
char	*argv[];
{
	int	n;

	if (argc >= 2) {
		sscanf(argv[1], "%d", &n);
		if (n > 0)
			printfactors(n);
	} else {
		while (scanf("%d", &n) == 1)
			if (n > 0)
				printfactors(n);
	}
}

/*
 * Print all prime factors of integer n > 0, smallest first, one to a line
 */
printfactors(n)
	register int	n;
{
	register int	prime;

	if (n == 1)
		printf("\t1\n");
	else while (n != 1) {
		prime = factor(n);
		printf("\t%d\n", prime);
		n /= prime;
	}
}

/*
 * Return smallest prime factor of integer N > 0
 *
 * Algorithm from E.W. Dijkstra (A Discipline of Programming, Chapter 20)
 */

int
factor(N)
	int	N;
{
	int		p;
	register int	f;
	static struct {
		int	hib;
		int	val[24];
	} ar;

	{	register int	x, y;

		ar.hib = -1;
		x = N; y = 2;
		while (x != 0) {
			ar.val[++ar.hib] = x % y;
			x /= y;
			y += 1;
		}
	}

	f = 2;

	while (ar.val[0] != 0 && ar.hib > 1) {
		register int	i;

		f += 1;
		i = 0;
		while (i != ar.hib) {
			register int	j;

			j = i + 1;
			ar.val[i] -= j * ar.val[j];
			while (ar.val[i] < 0) {
				ar.val[i] += f + i;
				ar.val[j] -= 1;
			}
			i = j;
		}
		while (ar.val[ar.hib] == 0)
			ar.hib--;
	}

	if (ar.val[0] == 0)
		p = f;
	else
		p = N;

	return(p);
}
