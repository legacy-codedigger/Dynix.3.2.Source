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
static char rcsid[] = "$Header: nopff.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include <sys/vmsystm.h>

main(argc, argv)
	int argc;
	char *argv[];
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s command\n", argv[0]);
		exit(1);
	}
	if (vm_ctl(VM_PFFABLE, 0) < 0) {
		perror("nopff");
		exit(1);
	}
	setuid(getuid());
	execvp(argv[1], &argv[1]);
	perror(argv[1]);
	exit(1);
}
