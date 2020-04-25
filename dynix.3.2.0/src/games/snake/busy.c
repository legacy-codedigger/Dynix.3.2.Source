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

/* $Header: busy.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)busy.c	4.1 (Berkeley) 7/4/83";
#endif

/*
 * busy: print an indication of how busy the system is for games.
 */
#ifndef MAX
# define MAX 30
#endif

#include <stdio.h>
main(argc, argv)
char **argv;
{
	double la[3];
	double max;

	loadav(la);
	max = la[0];
	if (la[1] > max) max = la[1];
	if (la[2] > max) max = la[2];
	if (argc > 1)
		printf("1=%g, 5=%g, 15=%g, max=%g\n", la[0], la[1], la[2], max);
	if (max > MAX)
		printf("100\n");	/* incredibly high, no games allowed */
	else
		printf("0\n");
	exit(0);
}

#include <sys/types.h>
#include <a.out.h>

struct	nlist nl[] = {
	{ "_avenrun" },
	{ 0 },
};

loadav(avenrun)
double	*avenrun;
{
	register int i;
	int	kmem;

	if ((kmem = open("/dev/kmem", 0)) < 0) {
		fprintf(stderr, "No kmem\n");
		exit(1);
	}
	nlist("/dynix", nl);
	if (nl[0].n_type==0) {
		fprintf(stderr, "No namelist\n");
		exit(1);
	}

	lseek(kmem, (long)nl[0].n_value, 0);
	read(kmem, avenrun, 3*sizeof(*avenrun));
}
