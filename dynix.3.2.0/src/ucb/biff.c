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
static char rcsid[] = "$Header: biff.c 2.0 86/01/28 $";
#endif

/*
 * biff
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

char	*ttyname();

main(argc, argv)
	int argc;
	char **argv;
{
	char *cp = ttyname(2);
	struct stat stb;

	argc--, argv++;
	if (cp == 0)
		fprintf(stderr, "Where are you?\n"), exit(1);
	if (stat(cp, &stb) < 0)
		perror(cp), exit(1);
	if (argc == 0) {
		printf("is %s\n", stb.st_mode&0100 ? "y" : "n");
		exit((stb.st_mode&0100) ? 0 : 1);
	}
	switch (argv[0][0]) {

	case 'y':
		if (chmod(cp, stb.st_mode|0100) < 0)
			perror(cp);
		break;

	case 'n':
		if (chmod(cp, stb.st_mode&~0100) < 0)
			perror(cp);
		break;

	default:
		fprintf(stderr, "usage: biff [y] [n]\n");
	}
	exit((stb.st_mode&0100) ? 0 : 1);
}
