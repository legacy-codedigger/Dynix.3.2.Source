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
static char rcsid[] = "$Header: vfw.c 2.0 86/01/28 $";
#endif

/*
 * Quick hack to see the values in a troff width table.
 */

#include <stdio.h>

main(argc,argv)
char **argv;
{
	FILE *f;
	int c;
	int i;

	if (argc != 2) {
		printf("usage: vfw ftX\n");
		exit(1);
	}
	f = fopen(argv[1], "r");
	if (f == NULL) {
		printf("Can't open %s\n", argv[1]);
		exit(1);
	}
	fseek(f, 32L, 0);
	for (i=0; !feof(f); i++) {
		c = getc(f);
		printf("%d\t%d\n", i, c&255);
	}
	exit(0);
}
