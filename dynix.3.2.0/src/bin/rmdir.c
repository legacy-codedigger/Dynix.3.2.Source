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

#ifndef lint
static char rcsid[] = "$Header: rmdir.c 2.3 90/04/10 $";
#endif

/*
 * Remove directory
 */
#include <stdio.h>

main(argc,argv)
	int argc;
	char **argv;
{
	int errors = 0;
	register char *p;
	register int x;

	if (argc < 2) {
		fprintf(stderr, "usage: rmdir  directory ...\n");
		exit(1);
	}
	while (--argc) {
		p = *++argv;

		/*
		 * Trim trailing '/'s off directory name--they confuse
		 * the rmdir() system call, but don't change the meaning
		 * of what directory you're talking about.
		 */
		while (p[x = strlen(p)-1] == '/')
			p[x] = '\0';

		if (rmdir(p) < 0) {
			fprintf(stderr, "rmdir: ");
			perror(p);;
			errors++;
		}
	}
	exit(errors != 0);
}
