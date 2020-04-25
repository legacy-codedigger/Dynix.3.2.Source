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
static char rcsid[] = "$Header: on.c 2.0 86/01/28 $";
#endif

/*
 * on.c
 *	Run a command on a given processor.
 */

#include <stdio.h>
#include <sys/tmp_ctl.h>

main(argc, argv)
	int	argc;
	char	**argv;
{
	if (argc < 3)  {
		fprintf(stderr, "usage: %s proc# command\n", argv[0]);
		exit(1);
	}
	if (tmp_affinity(atoi(argv[1])) == AFF_ERROR) {
		perror("tmp_affinity");
		exit(1);
	}
	setuid(getuid());
	execvp(argv[2], &argv[2]);
	perror(argv[2]);
	exit(1);
}
