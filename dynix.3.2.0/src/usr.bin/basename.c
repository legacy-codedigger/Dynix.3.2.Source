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
static char rcsid[] = "$Header: basename.c 2.0 86/01/28 $";
#endif

#include	<stdio.h>

main(argc, argv)
char **argv;
{
	register char *p1, *p2, *p3;

	if (argc < 2) {
		putchar('\n');
		exit(1);
	}
	p1 = argv[1];
	p2 = p1;
	while (*p1) {
		if (*p1++ == '/')
			p2 = p1;
	}
	if (argc>2) {
		for(p3=argv[2]; *p3; p3++) 
			;
		while(p1>p2 && p3>argv[2])
			if(*--p3 != *--p1)
				goto output;
		*p1 = '\0';
	}
output:
	puts(p2, stdout);
	exit(0);
}
