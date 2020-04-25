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
static char rcsid[] = "$Header: swapon.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include <fstab.h>

main(argc, argv)
	int argc;
	char *argv[];
{
	int stat = 0;

	dup2(1, 2);
	--argc, argv++;
	if (argc == 0) {
		fprintf(stderr, "usage: swapon -a | swapon name ...\n");
		exit(1);
	}
	if (argc == 1 && !strcmp(*argv, "-a")) {
		struct	fstab	*fsp;
		if (setfsent() == 0)
			perror(FSTAB), exit(1);
		while ( (fsp = getfsent()) != 0){
			if (strcmp(fsp->fs_type, FSTAB_SW) != 0)
				continue;
			printf("Adding %s as swap device\n",
			    fsp->fs_spec);
			if (swapon(fsp->fs_spec) == -1) {
				perror(fsp->fs_spec);
				stat = 1;
			}
		}
		endfsent();
		exit(stat);
	}
	do {
		if (swapon(*argv++) == -1) {
			perror(argv[-1]);
			stat = 1;
		}
		argc--;
	} while (argc > 0);
	exit(stat);
}
