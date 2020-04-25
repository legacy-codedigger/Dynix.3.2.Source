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
static char rcsid[] = "$Header: mknod.c 2.3 87/04/09 $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

main(argc, argv)
int argc;
char **argv;
{
	register int m, a, b;

	if(argc == 3 && !strcmp(argv[2], "p")) { /* fifo */
		a = mknod(argv[1], S_IFIFO|0666, 0);
		chown(argv[1], getuid(), getgid());
		if(a)
			perror("mknod");
		exit(a == 0?  0: 2);
	}
	if(argc != 5) {
		fprintf(stderr, "mknod: arg count\n");
		goto usage;
	}
	if(getuid()) {
		fprintf(stderr, "mknod: must be super-user\n");
		exit(2);
	}
	if(*argv[2] == 'b')
		m = S_IFBLK|0666; else
	if(*argv[2] == 'c')
		m = S_IFCHR|0666; else
		goto usage;
	a = atoi(argv[3]);
	b = atoi(argv[4]);
	if(mknod(argv[1], m, makedev(a, minor(b))) < 0)
		perror("mknod");
	exit(0);

usage:
	fprintf(stderr, "usage: mknod name b/c major minor\n");
	fprintf(stderr, "       mknod name p\n");
	exit(2);
}
